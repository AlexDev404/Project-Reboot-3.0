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
    // ReadInt(4) consumes exactly 2 bits (mask: 1<4→read, 2<4→read, 4≥4→stop).
    // Derived empirically: packet with bOpen=1,bClose=1 has 6-bit flag gap before
    // ChIndex SIP. Layout bOpen(1)+bClose(1)+CloseReason(2)+bIsReplPaused(1)+bReliable(1)=6. ✓
    constexpr uint32 UE4_CLOSE_REASON_MAX     = 4;     // EChannelCloseReason::MAX in Fortnite 17.50
    constexpr uint32 UE4_OLD_MAX_ACTOR_CHANNELS = 10240; // pre-HISTORY_MAX_ACTOR_CHANNELS_CUSTOMIZATION
    constexpr uint32 UE4_CHTYPE_MAX           = 8;     // pre-HISTORY_CHANNEL_NAMES

    // Fortnite 17.50 (EngineNetVer=18) uses the modern UE 4.26 wire paths
    // for these fields. Toggle to true to switch to the pre-history-flag
    // legacy paths (uncommon in modern UE codebases but kept here for fork
    // compatibility).
    constexpr bool USE_PRE_CHANNEL_NAMES         = false;
    constexpr bool USE_PRE_MAX_ACTOR_CHANNELS    = false;
    constexpr bool USE_PRE_CHANNEL_CLOSE_REASON  = false;
}

// Incoming bunch header layout for Fortnite 17.50 (UE 4.26.1 fork).
// Verified from server6.log [bit] position traces (Project Reboot reading
// Fortnite client packets). Gap between PacketInfo end and ChIndex SIP:
//   bControl=0                        → 3 bits  (bControl + bIsReplPaused + bReliable)
//   bControl=1, bClose=0             → 5 bits  (bControl + bOpen + bClose + bIsReplPaused + bReliable)
//   bControl=1, bClose=1             → 7 bits  (+ CloseReason ReadInt(4) = 2 bits)
//
// Wire layout:
//   bControl                       (1 bit)   ← FIRST bit of bunch header
//   if bControl:
//     bOpen                         (1 bit)
//     bClose                        (1 bit)
//     if bClose:
//       CloseReason ReadInt(4)      (2 bits)
//   bIsReplicationPaused            (1 bit)   ← always present
//   bReliable                       (1 bit)   ← always present
//   ChIndex via SerializeIntPacked             (variable)
//   bHasPackageMapExports, bHasMustBeMappedGUIDs, bPartial  (3 bits)
//   ChSequence via ReadInt(1024) if bReliable               (10 bits)
//   bPartialInitial, bPartialFinal if bPartial              (2 bits)
//   if (bOpen || bReliable):
//     FName: bHardcoded(1 bit) + SerializeIntPacked index, or string FName
//   BunchDataBits ReadInt(8192)  -- read by caller (NetConnection)
bool FInBunch::ReadBunchHeader(FBitReader& PacketReader, FBunchHeader& OutHeader)
{
    OutHeader = FBunchHeader{}; // zero-init all flags / fields

    const auto Pos = [&]() { return (long long)PacketReader.GetPosBits(); };
    BTRACE("=== ReadBunchHeader start pos=%lld bitsLeft=%lld",
           Pos(), (long long)PacketReader.GetBitsLeft());

    OutHeader.bControl = PacketReader.ReadBit() != 0;
    BTRACE("  bControl=%d (after pos=%lld)", (int)OutHeader.bControl, Pos());
    if (PacketReader.IsError()) { BTRACE("  ERROR after bControl"); return false; }

    if (OutHeader.bControl)
    {
        OutHeader.bOpen = PacketReader.ReadBit() != 0;
        BTRACE("  bOpen=%d (after pos=%lld)", (int)OutHeader.bOpen, Pos());

        OutHeader.bClose = PacketReader.ReadBit() != 0;
        BTRACE("  bClose=%d (after pos=%lld)", (int)OutHeader.bClose, Pos());

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
    }

    OutHeader.bIsReplicationPaused = PacketReader.ReadBit() != 0;
    BTRACE("  bIsReplPaused=%d (after pos=%lld)", (int)OutHeader.bIsReplicationPaused, Pos());

    OutHeader.bReliable = PacketReader.ReadBit() != 0;
    BTRACE("  bReliable=%d (after pos=%lld)", (int)OutHeader.bReliable, Pos());

    auto ReadFNameInto = [&](FBunchHeader& H) -> bool {
        H.bChNameIsValid     = true;
        H.bChNameIsHardcoded = PacketReader.ReadBit() != 0;
        BTRACE("  bChNameIsHardcoded=%d (after pos=%lld)",
               (int)H.bChNameIsHardcoded, Pos());
        if (H.bChNameIsHardcoded)
        {
            const long long preName = Pos();
            PacketReader.SerializeIntPacked(H.ChNameIndex);
            BTRACE("  ChNameIndex=%u (read %lld bits, after pos=%lld)",
                   H.ChNameIndex, Pos() - preName, Pos());
            return !PacketReader.IsError();
        }
        const long long preName = Pos();
        int32 SaveNum = 0;
        PacketReader.SerializeBits(&SaveNum, 32);
        if (PacketReader.IsError()) { BTRACE("  string FName: error reading SaveNum"); return false; }
        constexpr int32 kMaxChars = 1024;
        const int32 absLen = SaveNum < 0 ? -SaveNum : SaveNum;
        if (absLen > kMaxChars) {
            BTRACE("  string FName: SaveNum=%d exceeds cap %d", SaveNum, kMaxChars);
            return false;
        }
        if (SaveNum > 0) {
            std::string buf(SaveNum, '\0');
            PacketReader.SerializeBits(buf.data(), SaveNum * 8);
            if (!buf.empty() && buf.back() == '\0') buf.pop_back();
            H.ChNameString = std::move(buf);
        } else if (SaveNum < 0) {
            const int32 numChars = -SaveNum;
            std::wstring wbuf(numChars, L'\0');
            PacketReader.SerializeBits(wbuf.data(), numChars * 16);
            if (!wbuf.empty() && wbuf.back() == L'\0') wbuf.pop_back();
            std::string narrow(wbuf.size(), '?');
            for (size_t i = 0; i < wbuf.size(); ++i) {
                if (wbuf[i] < 128) narrow[i] = static_cast<char>(wbuf[i]);
            }
            H.ChNameString = std::move(narrow);
        }
        H.ChNameNumber = 0;
        PacketReader.SerializeBits(&H.ChNameNumber, 32);
        BTRACE("  ChNameString=\"%s\" Number=%u (read %lld bits, after pos=%lld)",
               H.ChNameString.c_str(), (unsigned)H.ChNameNumber,
               Pos() - preName, Pos());
        return !PacketReader.IsError();
    };

    // ChIndex is ALWAYS read from the wire (verified against UE source
    // build/ue4_netconnection.cpp:2326-2343). No bOpen-special-case.
    uint32 ChIndex = 0;
    {
        const long long preChIndex = Pos();
        if (USE_PRE_MAX_ACTOR_CHANNELS)
            ChIndex = PacketReader.ReadInt(UE4_OLD_MAX_ACTOR_CHANNELS);
        else
            PacketReader.SerializeIntPacked(ChIndex);
        OutHeader.ChIndex = static_cast<int32>(ChIndex);
        BTRACE("  ChIndex=%u (read %lld bits, after pos=%lld)",
               ChIndex, Pos() - preChIndex, Pos());
    }

    OutHeader.bHasPackageMapExports = PacketReader.ReadBit() != 0;
    OutHeader.bHasMustBeMappedGUIDs = PacketReader.ReadBit() != 0;
    OutHeader.bPartial              = PacketReader.ReadBit() != 0;
    BTRACE("  bHasPMExports=%d bHasMBMGuids=%d bPartial=%d (after pos=%lld)",
           (int)OutHeader.bHasPackageMapExports, (int)OutHeader.bHasMustBeMappedGUIDs,
           (int)OutHeader.bPartial, Pos());

    // ChSequence gated on bReliable per Binja HLIL — var_364=18 satisfies no
    // override that would make it unconditional.
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

    // ChName read for (bReliable || bOpen) bunches per UE source.
    if (OutHeader.bReliable || OutHeader.bOpen)
    {
        if (USE_PRE_CHANNEL_NAMES)
        {
            const long long preCh = Pos();
            uint32 ChType = PacketReader.ReadInt(UE4_CHTYPE_MAX);
            OutHeader.ChType = static_cast<EChannelType>(ChType);
            OutHeader.bChNameIsValid = true;
            BTRACE("  ChType=%u (read %lld bits, after pos=%lld)",
                   ChType, Pos() - preCh, Pos());
        }
        else
        {
            if (!ReadFNameInto(OutHeader)) return false;
            // Heuristic for modern path: ChIndex 0 => Control
            OutHeader.ChType = (OutHeader.ChIndex == 0)
                ? EChannelType::CHTYPE_Control
                : EChannelType::CHTYPE_Actor;
        }
    }
    BTRACE("=== ReadBunchHeader OK final pos=%lld", Pos());
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
    , bControl(false)
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
    , ChNameIndex(0)
    , Next(nullptr)
{
}

FOutBunch::FOutBunch(UChannel* InChannel, bool bInClose)
    : FBitWriter(1024, true)
    , Channel(InChannel)
    , ChIndex(InChannel ? InChannel->ChIndex : 0)
    , ChType(InChannel ? InChannel->ChType : EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bControl(false)
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
    , ChNameIndex(0)
    , Next(nullptr)
{
}

FOutBunch::FOutBunch(int64 InMaxBits)
    : FBitWriter(InMaxBits, true)
    , Channel(nullptr)
    , ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bControl(false)
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
    , ChNameIndex(0)
    , Next(nullptr)
{
}

void FOutBunch::WriteBunchHeader(FBitWriter& PacketWriter) const
{
    // Mirror of ReadBunchHeader. Wire layout verified from server6.log [bit] traces.
    PacketWriter.WriteBit(bControl ? 1 : 0);
    if (bControl)
    {
        PacketWriter.WriteBit(bOpen ? 1 : 0);
        PacketWriter.WriteBit(bClose ? 1 : 0);
        if (bClose)
        {
            if (USE_PRE_CHANNEL_CLOSE_REASON)
            {
                PacketWriter.WriteBit(bDormant ? 1 : 0);
            }
            else
            {
                uint32 CloseReason = bDormant ? 1u : 0u;
                PacketWriter.SerializeInt(CloseReason, UE4_CLOSE_REASON_MAX); // 2 bits
            }
        }
    }

    PacketWriter.WriteBit(bIsReplicationPaused ? 1 : 0);
    PacketWriter.WriteBit(bReliable ? 1 : 0);

    uint32 ChIndexPacked = static_cast<uint32>(ChIndex);
    PacketWriter.SerializeIntPacked(ChIndexPacked);

    PacketWriter.WriteBit(bHasPackageMapExports ? 1 : 0);
    PacketWriter.WriteBit(bHasMustBeMappedGUIDs ? 1 : 0);
    PacketWriter.WriteBit(bPartial ? 1 : 0);

    if (bReliable)
    {
        uint32 Seq = static_cast<uint32>(ChSequence);
        PacketWriter.SerializeInt(Seq, UE4_MAX_CHSEQUENCE); // ReadInt(1024) = 10 bits
    }

    if (bPartial)
    {
        PacketWriter.WriteBit(bPartialInitial ? 1 : 0);
        PacketWriter.WriteBit(bPartialFinal ? 1 : 0);
    }

    if (bOpen || bReliable)
    {
        // FName: bHardcoded=1 always for built-in channel names
        PacketWriter.WriteBit(1);
        uint32 NameIdx = static_cast<uint32>(ChNameIndex);
        PacketWriter.SerializeIntPacked(NameIdx);
    }

    // BunchDataBits written by caller (NetConnection) after packet payload
}

int64 FOutBunch::GetTotalBits() const
{
    // Estimate header bits + data bits
    int64 HeaderBits = 16; // Approximate header
    return HeaderBits + GetNumBits();
}
