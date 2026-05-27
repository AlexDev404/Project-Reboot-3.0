// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/DataChannel.cpp
// Bunch serialization/deserialization

#include "UE4Net/Channel/DataBunch.h"
#include "UE4Net/Channel/NetConnection.h"

#include <cstdio>

// Toggleable bit-level trace for diagnosing bunch-header decode failures on
// real Fortnite 17.50 wire data. Each step logs the reader position BEFORE
// the read so the failure site can be back-tracked into the packet hex.
#define UE4NET_BUNCH_TRACE 1
#if UE4NET_BUNCH_TRACE
  #define BTRACE(...) do { std::fprintf(stderr, "[bunch] " __VA_ARGS__); std::fprintf(stderr, "\n"); std::fflush(stderr); } while (0)
#else
  #define BTRACE(...) (void)0
#endif

// ============================================================================
// FInBunch
// ============================================================================

FInBunch::FInBunch(UNetConnection* InConnection, uint8* Src, int64 CountBits)
    : FBitReader(Src, CountBits)
    , PacketId(0)
    , Connection(InConnection)
    , ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(false)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , bHasGUIDs(false)
    , Next(nullptr)
{
}

// UE 4.26 wire constants (Engine/Source/Runtime/Engine/Classes/Engine/NetConnection.h)
namespace
{
    constexpr uint32 UE4_MAX_CHSEQUENCE       = 1024;  // Power of 2 > RELIABLE_BUFFER
    constexpr uint32 UE4_CLOSE_REASON_MAX     = 15;    // EChannelCloseReason::MAX
    constexpr uint32 UE4_OLD_MAX_ACTOR_CHANNELS = 10240; // pre-HISTORY_MAX_ACTOR_CHANNELS_CUSTOMIZATION
    constexpr uint32 UE4_CHTYPE_MAX           = 8;     // pre-HISTORY_CHANNEL_NAMES

    // Fortnite 17.50 reports EngineNetVer=18 but Binja disassembly of
    // UNetConnection::ReceivedPacket (sub_140F79464) shows the bunch reader
    // uses the MODERN UE paths (FName for ChName, SerializeIntPacked for
    // ChIndex, 4-bit CloseReason) -- the version thresholds for those
    // history flags are all <= 18 in this fork.
    constexpr bool USE_PRE_CHANNEL_NAMES         = false;
    constexpr bool USE_PRE_MAX_ACTOR_CHANNELS    = false;
    constexpr bool USE_PRE_CHANNEL_CLOSE_REASON  = false;
}

// UE 4.26 incoming bunch header layout, copied verbatim from
// UNetConnection::ReceivedPacket in Engine/Source/Runtime/Engine/Private/NetConnection.cpp.
// Order is wire-load-bearing — do not reorder fields without checking the source.
//
//   bControl                             (1 bit)
//   bOpen   = bControl ? ReadBit : 0     (1 bit conditional)
//   bClose  = bControl ? ReadBit : 0     (1 bit conditional)
//   CloseReason if bClose                (ReadInt(15) = 4 bits)
//   bIsReplicationPaused                 (1 bit)
//   bReliable                            (1 bit)
//   ChIndex                              (SerializeIntPacked, variable)
//   bHasPackageMapExports                (1 bit)
//   bHasMustBeMappedGUIDs                (1 bit)
//   bPartial                             (1 bit)
//   ChSequence if bReliable              (ReadInt(MAX_CHSEQUENCE=1024) = 10 bits, then MakeRelative)
//   bPartialInitial if bPartial          (1 bit)
//   bPartialFinal   if bPartial          (1 bit)
//   ChName if (bReliable || bOpen)       (UPackageMap::StaticSerializeName — 1 bit hardcoded flag,
//                                         then packed int OR length-prefixed string + int32)
//   <BunchDataBits is read by the CALLER via ReadInt(MaxPacket * 8) — not part of header>
bool FInBunch::ReadBunchHeader(FBitReader& PacketReader, FBunchHeader& OutHeader)
{
    OutHeader = FBunchHeader{}; // zero-init all flags / fields

    const auto Pos = [&]() { return (long long)PacketReader.GetPosBits(); };
    BTRACE("=== ReadBunchHeader start pos=%lld bitsLeft=%lld",
           Pos(), (long long)PacketReader.GetBitsLeft());

    const uint8 bControl = PacketReader.ReadBit();
    BTRACE("  bControl=%u (after pos=%lld)", bControl, Pos());
    if (PacketReader.IsError()) { BTRACE("  ERROR after bControl"); return false; }

    OutHeader.bOpen                = bControl ? (PacketReader.ReadBit() != 0) : false;
    OutHeader.bIsReplicationPaused = bControl ? (PacketReader.ReadBit() != 0) : false;
    OutHeader.bClose               = bControl ? (PacketReader.ReadBit() != 0) : false;
    BTRACE("  bOpen=%d bIsReplPaused=%d bClose=%d (after pos=%lld)",
           (int)OutHeader.bOpen, (int)OutHeader.bIsReplicationPaused,
           (int)OutHeader.bClose, Pos());

    if (OutHeader.bClose)
    {
        if (USE_PRE_CHANNEL_CLOSE_REASON)
        {
            OutHeader.bDormant = PacketReader.ReadBit() != 0;
            BTRACE("  bDormant=%u (after pos=%lld)", (unsigned)OutHeader.bDormant, Pos());
        }
        else
        {
            uint32 CloseReason = 0;
            PacketReader.SerializeInt(CloseReason, UE4_CLOSE_REASON_MAX);
            OutHeader.bDormant = (CloseReason == 1);
            BTRACE("  CloseReason=%u (after pos=%lld)", CloseReason, Pos());
        }
    }

    // ReadBit5 removed for the bOpen=1 path — testing showed it pushes the FName
    // position 1 bit too late (bChNameIsHardcoded reads 0 → bail). The HLIL bit
    // may belong to a different code path (non-open bunches?) or be conditional.

    OutHeader.bReliable = PacketReader.ReadBit() != 0;
    BTRACE("  bReliable=%d (after pos=%lld)", (int)OutHeader.bReliable, Pos());

    // Re-applying the "FName-in-place-of-ChIndex for bOpen" hypothesis. Our
    // 8-bit SerializeIntPacked read here returned 41 = NAME_Control's hardcoded
    // FName index, suggesting this position carries the FName, not a ChIndex.
    // ChIndex is derived from the FName lookup (Control name → ChIndex=0).
    uint32 ChIndex = 0;
    if (!OutHeader.bOpen)
    {
        const long long preChIndex = Pos();
        if (USE_PRE_MAX_ACTOR_CHANNELS)
            ChIndex = PacketReader.ReadInt(UE4_OLD_MAX_ACTOR_CHANNELS);
        else
            PacketReader.SerializeIntPacked(ChIndex);
        OutHeader.ChIndex = static_cast<int32>(ChIndex);
        BTRACE("  ChIndex=%u from wire (read %lld bits, after pos=%lld)",
               ChIndex, Pos() - preChIndex, Pos());
    }
    else
    {
        BTRACE("  bOpen=1 -> ChIndex deferred (FName lookup follows)");
        OutHeader.bChNameIsValid     = true;
        OutHeader.bChNameIsHardcoded = PacketReader.ReadBit() != 0;
        BTRACE("  bChNameIsHardcoded=%d (after pos=%lld)",
               (int)OutHeader.bChNameIsHardcoded, Pos());
        if (OutHeader.bChNameIsHardcoded)
        {
            const long long preName = Pos();
            PacketReader.SerializeIntPacked(OutHeader.ChNameIndex);
            BTRACE("  ChNameIndex=%u (read %lld bits, after pos=%lld)",
                   OutHeader.ChNameIndex, Pos() - preName, Pos());
        }
        else
        {
            BTRACE("  bChNameIsHardcoded=0 -> string FName not supported, BAIL");
            return false;
        }
        // Derive ChIndex from name. Without the full hardcoded name table,
        // assume Control for the NMT_Hello case (NameIndex=41).
        OutHeader.ChIndex = 0;
        OutHeader.ChType  = EChannelType::CHTYPE_Control;
    }

    OutHeader.bHasPackageMapExports = PacketReader.ReadBit() != 0;
    OutHeader.bHasMustBeMappedGUIDs = PacketReader.ReadBit() != 0;
    OutHeader.bPartial              = PacketReader.ReadBit() != 0;
    BTRACE("  bHasPMExports=%d bHasMBMGuids=%d bPartial=%d (after pos=%lld)",
           (int)OutHeader.bHasPackageMapExports, (int)OutHeader.bHasMustBeMappedGUIDs,
           (int)OutHeader.bPartial, Pos());

    // ChSequence: in UE 4.26 this is read here, BEFORE bPartialInitial/bPartialFinal —
    // previous versions read it later. The MakeRelative step needs the per-channel
    // InReliable counter which we don't track yet; the raw value is fine for the
    // first NMT_Hello bunch (InReliable starts at 0).
    if (OutHeader.bReliable)
    {
        OutHeader.ChSequence = static_cast<int32>(PacketReader.ReadInt(UE4_MAX_CHSEQUENCE));
        BTRACE("  ChSequence=%d (after pos=%lld)", OutHeader.ChSequence, Pos());
    }

    if (OutHeader.bPartial)
    {
        OutHeader.bPartialInitial = PacketReader.ReadBit() != 0;
        OutHeader.bPartialFinal   = PacketReader.ReadBit() != 0;
        BTRACE("  bPartialInitial=%d bPartialFinal=%d (after pos=%lld)",
               (int)OutHeader.bPartialInitial, (int)OutHeader.bPartialFinal, Pos());
    }

    // ChName was read above for bOpen bunches. Only read here for reliable
    // non-open bunches.
    if (OutHeader.bReliable && !OutHeader.bOpen)
    {
        OutHeader.bChNameIsValid = true;
        if (USE_PRE_CHANNEL_NAMES)
        {
            const long long preCh = Pos();
            uint32 ChType = PacketReader.ReadInt(UE4_CHTYPE_MAX);
            OutHeader.ChType = static_cast<EChannelType>(ChType);
            BTRACE("  ChType=%u (read %lld bits, after pos=%lld)",
                   ChType, Pos() - preCh, Pos());
        }
        else
        {
            OutHeader.bChNameIsHardcoded = PacketReader.ReadBit() != 0;
            BTRACE("  bChNameIsHardcoded=%d (after pos=%lld)",
                   (int)OutHeader.bChNameIsHardcoded, Pos());
            if (OutHeader.bChNameIsHardcoded)
            {
                const long long preName = Pos();
                PacketReader.SerializeIntPacked(OutHeader.ChNameIndex);
                BTRACE("  ChNameIndex=%u (read %lld bits, after pos=%lld)",
                       OutHeader.ChNameIndex, Pos() - preName, Pos());
            }
            else
            {
                BTRACE("  bChNameIsHardcoded=0 -> string-path FName not supported, BAIL");
                return false;
            }
            // Heuristic for modern path: ChIndex 0 => Control
            OutHeader.ChType = (OutHeader.ChIndex == 0)
                ? EChannelType::CHTYPE_Control
                : EChannelType::CHTYPE_Actor;
        }
    }
    BTRACE("=== ReadBunchHeader OK final pos=%lld", Pos());

    // Diagnostic: dump remaining bits as a binary string so we can manually
    // locate the BunchDataBits boundary. We're trying to find the real header
    // end position; ground truth from Fortnite log is 56-bit header + 80-bit
    // data for the NMT_Hello bunch.
    {
        FBitReaderMark mark(PacketReader);
        const long long savedPos = Pos();
        const long long bitsLeft = (long long)PacketReader.GetBitsLeft();
        const long long dumpBits = bitsLeft < 160 ? bitsLeft : 160;
        char buf[200];
        int bi = 0;
        for (long long i = 0; i < dumpBits && bi < (int)sizeof(buf) - 2; ++i)
        {
            uint8 b = PacketReader.ReadBit();
            buf[bi++] = (b ? '1' : '0');
            if (((i + 1) % 8) == 0 && bi < (int)sizeof(buf) - 2) buf[bi++] = ' ';
        }
        buf[bi] = 0;
        BTRACE("  remaining %lld bits from pos=%lld:", dumpBits, savedPos);
        BTRACE("  %s", buf);
        mark.Pop(PacketReader);
    }

    return !PacketReader.IsError();
}

// ============================================================================
// FOutBunch
// ============================================================================

FOutBunch::FOutBunch()
    : FBitWriter(0, true)
    , Channel(nullptr)
    , ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(false)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , Next(nullptr)
{
}

FOutBunch::FOutBunch(UChannel* InChannel, bool bInClose)
    : FBitWriter(1024, true)
    , Channel(InChannel)
    , ChIndex(InChannel ? InChannel->ChIndex : 0)
    , ChType(InChannel ? InChannel->ChType : EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(bInClose)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , Next(nullptr)
{
}

FOutBunch::FOutBunch(int64 InMaxBits)
    : FBitWriter(InMaxBits, true)
    , Channel(nullptr)
    , ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(false)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , Next(nullptr)
{
}

void FOutBunch::WriteBunchHeader(FBitWriter& PacketWriter) const
{
    bool bControl = bOpen || bClose;
    PacketWriter.WriteBit(bControl ? 1 : 0);

    if (bControl)
    {
        PacketWriter.WriteBit(bOpen ? 1 : 0);
        PacketWriter.WriteBit(bClose ? 1 : 0);
        if (bClose)
        {
            PacketWriter.WriteBit(bDormant ? 1 : 0);
        }
    }

    PacketWriter.WriteBit(bIsReplicationPaused ? 1 : 0);
    PacketWriter.WriteBit(bReliable ? 1 : 0);

    uint32 ChIndexPacked = static_cast<uint32>(ChIndex);
    PacketWriter.SerializeIntPacked(ChIndexPacked);

    PacketWriter.WriteBit(bHasPackageMapExports ? 1 : 0);
    PacketWriter.WriteBit(bHasMustBeMappedGUIDs ? 1 : 0);

    PacketWriter.WriteBit(bPartial ? 1 : 0);
    if (bPartial)
    {
        PacketWriter.WriteBit(bPartialInitial ? 1 : 0);
        PacketWriter.WriteBit(bPartialFinal ? 1 : 0);
    }

    // Channel type
    if (bOpen || bPartialInitial)
    {
        uint32 TypeVal = static_cast<uint32>(ChType);
        PacketWriter.SerializeInt(TypeVal, static_cast<uint32>(EChannelType::CHTYPE_MAX));
    }

    // Reliable sequence
    if (bReliable)
    {
        uint32 Seq = static_cast<uint32>(ChSequence);
        PacketWriter.SerializeIntPacked(Seq);
    }
}

int64 FOutBunch::GetTotalBits() const
{
    // Estimate header bits + data bits
    int64 HeaderBits = 16; // Approximate header
    return HeaderBits + GetNumBits();
}
