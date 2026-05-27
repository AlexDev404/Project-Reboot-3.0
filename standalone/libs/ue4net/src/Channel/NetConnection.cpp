// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/NetConnection.cpp
// UNetConnection implementation

#include "UE4Net/Channel/NetConnection.h"
#include "UE4Net/Channel/NetDriver.h"

#include <chrono>
#include <cstdio>

// Temporary diagnostic — set to 1 to trace packet parsing
#define UE4NET_TRACE_RECV 1
#if UE4NET_TRACE_RECV
  #define UE4NET_TRACE(...) do { std::fprintf(stderr, "[ue4net] " __VA_ARGS__); std::fprintf(stderr, "\n"); std::fflush(stderr); } while (0)
#else
  #define UE4NET_TRACE(...) (void)0
#endif

// UE 4.26 FOodleHandlerComponent::Outgoing only writes its bCompressedPacket
// marker bit for packets at or above net.OodleMinSizeForCompression (default 19
// bytes payload after the StatelessConnect 5-bit preamble). Even with the
// Oodle bypass installed in Pyrite, that conditional bit may still be present
// in the wire format depending on how the bypass intercepted the function.
// Flip this to 1 if the >=19-byte stream is one bit offset relative to the
// <19-byte stream.
#ifndef UE4NET_CONSUME_OODLE_MARKER_BIT
#define UE4NET_CONSUME_OODLE_MARKER_BIT 0
#endif

// Threshold above which the Oodle marker bit is expected (raw UDP payload).
// net.OodleMinSizeForCompression=19 from Fortnite 17.50 CVar dump.
static constexpr int32 GOodleCompressionThresholdBytes = 19;

static double GetTime()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

UNetConnection::UNetConnection()
    : State(EConnectionState::USOCK_Invalid)
    , Driver(nullptr)
    , LastReceiveTime(0.0)
    , LastSendTime(0.0)
    , bHandshakeComplete(false)
    , bFirstPostHandshakePacket(true)
    , InPacketId(0)
    , OutPacketId(0)
    , OutAckPacketId(0)
    , TotalBytesSent(0)
    , TotalBytesReceived(0)
    , PacketsSent(0)
    , PacketsReceived(0)
    , NextChannelIndex(1) // 0 is reserved for control channel
{
    FMemory::Memzero(Channels, sizeof(Channels));
}

UNetConnection::~UNetConnection()
{
    // Clean up channels
    for (int32 i = 0; i < MAX_CHANNELS; i++)
    {
        if (Channels[i])
        {
            delete Channels[i];
            Channels[i] = nullptr;
        }
    }

    // Clean up pending bunches
    for (auto* Bunch : PendingOutBunches)
    {
        delete Bunch;
    }
    PendingOutBunches.Empty();
}

void UNetConnection::InitBase(UNetDriver* InDriver, FSocket* InSocket, const FString& InURL, EConnectionState InState)
{
    Driver = InDriver;
    URL = InURL;
    State = InState;
    LastReceiveTime = GetTime();
    LastSendTime = GetTime();

    // Initialize handshake
    HandshakeHandler.Initialize();

    // Initialize packet notify
    PacketNotify.Init(FNetPacketNotify::SequenceNumberT(0), FNetPacketNotify::SequenceNumberT(0));

    // Create control channel (channel 0)
    CreateChannel(EChannelType::CHTYPE_Control, 0);
}

UChannel* UNetConnection::CreateChannel(EChannelType Type, int32 ChIndex)
{
    if (ChIndex < 0)
    {
        ChIndex = NextChannelIndex++;
        if (NextChannelIndex >= MAX_CHANNELS)
        {
            return nullptr;
        }
    }

    if (ChIndex >= MAX_CHANNELS || Channels[ChIndex] != nullptr)
    {
        return nullptr;
    }

    UChannel* NewChannel = nullptr;
    switch (Type)
    {
        case EChannelType::CHTYPE_Control:
            NewChannel = new UControlChannel();
            break;
        case EChannelType::CHTYPE_Actor:
            NewChannel = new UActorChannel();
            break;
        default:
            NewChannel = nullptr;
            break;
    }

    if (NewChannel)
    {
        NewChannel->Init(this, ChIndex, Type);
        Channels[ChIndex] = NewChannel;
    }

    return NewChannel;
}

void UNetConnection::DestroyChannel(int32 ChIndex)
{
    if (ChIndex >= 0 && ChIndex < MAX_CHANNELS && Channels[ChIndex])
    {
        delete Channels[ChIndex];
        Channels[ChIndex] = nullptr;
    }
}

UActorChannel* UNetConnection::FindActorChannel(FNetworkGUID NetGUID)
{
    for (int32 i = 1; i < MAX_CHANNELS; i++)
    {
        if (Channels[i] && Channels[i]->ChType == EChannelType::CHTYPE_Actor)
        {
            UActorChannel* ActorCh = static_cast<UActorChannel*>(Channels[i]);
            if (ActorCh->ActorNetGUID == NetGUID)
            {
                return ActorCh;
            }
        }
    }
    return nullptr;
}

void UNetConnection::ReceivedRawPacket(const uint8* Data, int32 Count)
{
    if (Count == 0) return;

    LastReceiveTime = GetTime();
    TotalBytesReceived += Count;
    PacketsReceived++;

    // Fortnite 17.50: MagicHeader(4) + HandshakeBit(1) = handshake packet
    if (FStatelessConnectHandlerComponent::IsHandshakePacket(Data, Count))
    {
        if (!bHandshakeComplete)
        {
            FBitReader Reader(const_cast<uint8*>(Data), static_cast<int64>(Count) * 8);

            // Skip 4-bit MagicHeader + 1-bit HandshakeBit (already known to be 1)
            Reader.ReadBit(); Reader.ReadBit(); Reader.ReadBit(); Reader.ReadBit();
            Reader.ReadBit();

            if (HandshakeHandler.GetState() == EHandshakeState::InitializedOnLocal ||
                HandshakeHandler.GetState() == EHandshakeState::SentChallenge)
            {
                // Server: This is a client's challenge response
                if (HandshakeHandler.ProcessChallengeResponse(Reader, RemoteAddressStr))
                {
                    // Derive cookie-based initial sequence numbers BEFORE sending
                    // the challenge ack — the ack itself is our first packet at
                    // OutSeq = ServerSequence, and client uses the same cookie to
                    // expect that exact sequence value.
                    uint16 ServerSeq = 0;
                    uint16 ClientSeq = 0;
                    HandshakeHandler.GetChallengeSequenceList(ServerSeq, ClientSeq);
                    PacketNotify.Init(
                        FNetPacketNotify::SequenceNumberT(ClientSeq),
                        FNetPacketNotify::SequenceNumberT(ServerSeq));
                    UE4NET_TRACE("Handshake complete: cookie-derived ClientSeq=%u ServerSeq=%u",
                        (unsigned)ClientSeq, (unsigned)ServerSeq);

                    // Send challenge ack
                    FBitWriter AckPacket(256, true);
                    HandshakeHandler.CreateChallengeAck(AckPacket);
                    LowLevelSend(AckPacket.GetData(), static_cast<int32>(AckPacket.GetNumBytes()));

                    bHandshakeComplete = true;
                    State = EConnectionState::USOCK_Open;
                    if (OnStateChanged) OnStateChanged(State);
                }
            }
        }
        return;
    }

    if (!bHandshakeComplete)
    {
        if (HandshakeHandler.IsHandshakeComplete())
        {
            bHandshakeComplete = true;
        }
        else
        {
            return; // Not ready for game packets yet
        }
    }

    // Game packet: skip MagicHeader(4) + HandshakeBit(0 = game)
#if UE4NET_TRACE_RECV
    {
        char hex[3 * 64 + 4] = {0};
        int n = Count < 32 ? Count : 32;
        for (int i = 0; i < n; ++i)
        {
            std::snprintf(hex + i * 3, 4, "%02X ", (unsigned)Data[i]);
        }
        UE4NET_TRACE("RawGamePacket(%d bytes): %s%s",
            Count, hex, Count > 32 ? "..." : "");
    }
#endif
    // UE4 packet framing: FlushNet writes a trailing termination bit (a `1`)
    // as the last bit of every outgoing packet, then byte-aligns with zeros.
    // The receiver must locate that trailing `1` (= the highest set bit in
    // the packet) and treat it as end-of-data — bits AT and AFTER it are not
    // bunch content. Without this, the parser keeps trying to read "bunches"
    // out of the stop-bit + zero padding after the last real bunch.
    int64 PacketBitCount = static_cast<int64>(Count) * 8;
    for (int i = Count - 1; i >= 0; --i)
    {
        if (Data[i] == 0) { PacketBitCount -= 8; continue; }
        // Find the highest set bit in this byte (LSB-first stream convention:
        // bit 0 of a byte is its low bit, bit 7 is its high bit).
        for (int b = 7; b >= 0; --b)
        {
            if (Data[i] & (1u << b)) { PacketBitCount = static_cast<int64>(i) * 8 + b; break; }
        }
        break;
    }
    FBitReader Reader(const_cast<uint8*>(Data), PacketBitCount);
    // Strip the 4-bit Fortnite MagicHeader (0b0111 LSB-first). Do NOT strip a
    // separate "handshake bit" -- bit 4 in the wire format is a discriminator
    // used by IsHandshakePacket() at the byte level, but for non-handshake
    // game packets that bit position is the LSB of PacketNotify's HWC field
    // and must be left in the stream.
    Reader.ReadBit(); Reader.ReadBit(); Reader.ReadBit(); Reader.ReadBit();

#if UE4NET_CONSUME_OODLE_MARKER_BIT
    // Empirically (Fortnite 17.50, Pyrite Oodle hook in place): only packets
    // whose post-magic UDP payload is >= net.OodleMinSizeForCompression (19)
    // carry the bCompressedPacket marker bit. Smaller packets (e.g. 11-byte
    // KeepAlive/acks) have no Oodle bit at all -- consuming one corrupts the
    // PackedHeader HWC field. Gate by raw packet size.
    if (Count >= GOodleCompressionThresholdBytes)
    {
        uint8 OodleBit = Reader.ReadBit();
        UE4NET_TRACE("Consumed Oodle marker bit (val=%u, size=%d)", (unsigned)OodleBit, Count);
    }
#endif

    ReceivedPacket(Reader);
}

void UNetConnection::ReceivedPacket(FBitReader& Reader)
{
    int64 PacketBits = Reader.GetBitsLeft();
    UE4NET_TRACE("ReceivedPacket: %lld bits left at entry", PacketBits);

    // Read packet header (NetPacketNotify)
    FNetPacketNotify::FNotificationHeader NotifyHeader;
    if (!PacketNotify.ReadHeader(NotifyHeader, Reader))
    {
        UE4NET_TRACE("  -> PacketNotify.ReadHeader FAILED");
        return; // Invalid packet
    }
    UE4NET_TRACE("  -> ReadHeader OK: Seq=%u AckedSeq=%u HistoryWords=%u (InSeq=%u OutSeq=%u OutAckSeq=%u, %lld bits left)",
        (unsigned)NotifyHeader.Seq.Get(), (unsigned)NotifyHeader.AckedSeq.Get(),
        (unsigned)NotifyHeader.HistoryWordCount,
        (unsigned)PacketNotify.GetInSeq().Get(),
        (unsigned)PacketNotify.GetOutSeq().Get(),
        (unsigned)PacketNotify.GetOutAckSeq().Get(),
        Reader.GetBitsLeft());

    // Snap to the client's view of sequence numbers on the first post-handshake
    // packet. Cookie-derived init is approximate; trusting the first observed
    // header guarantees the strict GetSequenceDelta check passes.
    if (bFirstPostHandshakePacket)
    {
        bFirstPostHandshakePacket = false;
        const uint16 ClientSeq = NotifyHeader.Seq.Get();
        const uint16 ClientAckedSeq = NotifyHeader.AckedSeq.Get();
        const uint16 NewInSeq = static_cast<uint16>((ClientSeq - 1) & 0x3FFF);
        const uint16 NewOutSeq = static_cast<uint16>((ClientAckedSeq + 1) & 0x3FFF);
        PacketNotify.Init(
            FNetPacketNotify::SequenceNumberT(NewInSeq),
            FNetPacketNotify::SequenceNumberT(NewOutSeq));
        UE4NET_TRACE("  -> snap: InSeq=%u OutSeq=%u OutAckSeq=%u",
            (unsigned)PacketNotify.GetInSeq().Get(),
            (unsigned)PacketNotify.GetOutSeq().Get(),
            (unsigned)PacketNotify.GetOutAckSeq().Get());
    }

    // Process acks
    auto AckFunc = [](FNetPacketNotify::SequenceNumberT Seq, bool bDelivered) {
        // Handle delivery notifications for sent packets
    };

    auto SeqDelta = PacketNotify.Update(NotifyHeader, AckFunc);
    if (SeqDelta <= 0)
    {
        UE4NET_TRACE("  -> SeqDelta=%d (rejected)", (int)SeqDelta);
        return; // Duplicate or out of order
    }
    UE4NET_TRACE("  -> SeqDelta=%d, accepting", (int)SeqDelta);

    // Acknowledge this received packet
    PacketNotify.AckSeq(NotifyHeader.Seq);

    InPacketId++;

    // PacketInfo block for Fortnite 17.50 (EngineNetVer=18).
    // Verified by Binja disassembly of UNetConnection::ReceivedPacket
    // (sub_140F79464): the layout is NOT the mainline UE 4.26 layout. It is:
    //   bHasServerFrameTime  : 1 bit
    //   if bHasServerFrameTime:
    //       jitter           : SerializeInt(max=1024) = 10 bits
    // Total 1 OR 11 bits, gated on EngineNetVer >= 14 (which v18 satisfies).
    // No RemoteInKBytesPerSecondByte read in this fork.
    // Bunch start = bit 69 (bHasFT=0) or bit 79 (bHasFT=1).
    {
        uint8 bHasServerFrameTime = Reader.ReadBit();
        uint32 PacketJitterClockTimeMS = 0;
        if (bHasServerFrameTime)
        {
            Reader.SerializeInt(PacketJitterClockTimeMS, 1024);
        }
        UE4NET_TRACE("  -> PacketInfo (NetVer18): bHasFT=%u jitter=%u (pos=%lld bits left=%lld)",
            (unsigned)bHasServerFrameTime, (unsigned)PacketJitterClockTimeMS,
            (long long)Reader.GetPosBits(), Reader.GetBitsLeft());
    }

    int BunchCount = 0;
    // UE 4.26 bunch loop: there is NO per-bunch "more bunches" marker bit.
    // Bunches are parsed back-to-back until the reader runs out. The packet-
    // trailer bit (set in UNetConnection::FlushNet via SendBuffer.WriteBit(1))
    // is stripped above via the highest-set-bit scan, so Num points just
    // before the stop bit.
    //
    // Smallest legal bunch: ~5 flag bits + 8 ChIndex + 8 BDB field + ≥1 data bit
    // ≈ 22 bits. Bail before entering ReadBunchHeader if fewer remain — those
    // are leftover sub-bit-boundary fragments after the last real bunch.
    constexpr int64 kMinBunchBits = 22;
    while (!Reader.AtEnd() && !Reader.IsError())
    {
        if (Reader.GetBitsLeft() < kMinBunchBits)
        {
            UE4NET_TRACE("  -> bunch loop done: %lld trailing bits (below min, post-bunch padding)",
                Reader.GetBitsLeft());
            break;
        }
        FBunchHeader BunchHeader;
        if (!FInBunch::ReadBunchHeader(Reader, BunchHeader))
        {
            UE4NET_TRACE("  -> ReadBunchHeader FAILED (bunches so far=%d)", BunchCount);
            break;
        }
        UE4NET_TRACE("  -> Bunch %d: ChIndex=%d ChType=%d bOpen=%d bClose=%d bReliable=%d "
                     "bPartial=%d ChSeq=%d ChNameHardcoded=%d ChNameIdx=%u",
            BunchCount, (int)BunchHeader.ChIndex, (int)BunchHeader.ChType,
            (int)BunchHeader.bOpen, (int)BunchHeader.bClose, (int)BunchHeader.bReliable,
            (int)BunchHeader.bPartial, (int)BunchHeader.ChSequence,
            (int)BunchHeader.bChNameIsHardcoded, (unsigned)BunchHeader.ChNameIndex);
        BunchCount++;

        // BunchDataBits: experiment with SerializeIntPacked (1+ bytes) instead
        // of fixed-width ReadInt. The 13-bit ReadInt approach has consistently
        // produced invalid values for plaintext NMT_Hello bunches.
        uint32 BunchDataBits = 0;
        const int64 preBdb = Reader.GetPosBits();
        Reader.SerializeIntPacked(BunchDataBits);
        const int64 bdbBits = Reader.GetPosBits() - preBdb;
        UE4NET_TRACE("  -> BunchDataBits=%u (read %lld bits via SerializeIntPacked, bits left=%lld)",
            (unsigned)BunchDataBits, (long long)bdbBits, (long long)Reader.GetBitsLeft());
        if (Reader.IsError() || BunchDataBits > (uint32)Reader.GetBitsLeft())
        {
            UE4NET_TRACE("  -> BunchDataBits invalid: %u (bits left=%lld, error=%d)",
                (unsigned)BunchDataBits, Reader.GetBitsLeft(), (int)Reader.IsError());
            break;
        }

        // Read bunch data
        TArray<uint8> BunchData;
        int32 BunchDataBytes = static_cast<int32>((BunchDataBits + 7) / 8);
        BunchData.AddZeroed(BunchDataBytes);
        Reader.SerializeBits(BunchData.GetData(), BunchDataBits);

        if (Reader.IsError()) break;

        // Find or create channel
        UChannel* Channel = nullptr;
        if (BunchHeader.ChIndex >= 0 && BunchHeader.ChIndex < MAX_CHANNELS)
        {
            Channel = Channels[BunchHeader.ChIndex];

            if (!Channel && BunchHeader.bOpen)
            {
                // Create new channel
                Channel = CreateChannel(BunchHeader.ChType, BunchHeader.ChIndex);
            }
        }

        if (Channel)
        {
            UE4NET_TRACE("  -> delivering bunch to channel %d (type %d)",
                (int)BunchHeader.ChIndex, (int)Channel->ChType);
            // Create FInBunch and deliver to channel
            FInBunch InBunch(this, BunchData.GetData(), BunchDataBits);
            InBunch.PacketId = InPacketId;
            InBunch.ChIndex = BunchHeader.ChIndex;
            InBunch.ChType = BunchHeader.ChType;
            InBunch.ChSequence = BunchHeader.ChSequence;
            InBunch.bOpen = BunchHeader.bOpen;
            InBunch.bClose = BunchHeader.bClose;
            InBunch.bDormant = BunchHeader.bDormant;
            InBunch.bReliable = BunchHeader.bReliable;
            InBunch.bPartial = BunchHeader.bPartial;
            InBunch.bPartialInitial = BunchHeader.bPartialInitial;
            InBunch.bPartialFinal = BunchHeader.bPartialFinal;
            InBunch.bHasPackageMapExports = BunchHeader.bHasPackageMapExports;
            InBunch.bHasMustBeMappedGUIDs = BunchHeader.bHasMustBeMappedGUIDs;
            InBunch.bIsReplicationPaused = BunchHeader.bIsReplicationPaused;

            bool bSkipAck = false;
            Channel->ReceivedNextBunch(InBunch, bSkipAck);
        }
    }
}

void UNetConnection::FlushNet(bool bIgnoreSimulation)
{
    if (State == EConnectionState::USOCK_Closed) return;

    // Don't send empty packets — real UE4 servers stay silent until they have data
    // to send or acks to deliver. Spamming empty PacketNotify headers triggers the
    // client to disconnect (it interprets them as malformed traffic).
    if (PendingOutBunches.Num() == 0)
    {
        return;
    }

    // Build outgoing packet
    FBitWriter PacketWriter(MAX_PACKET_SIZE * 8, false);

    // Write packet header
    PacketNotify.WriteHeader(PacketWriter, false);

    // Write pending bunches
    bool bHasBunches = false;
    for (auto* Bunch : PendingOutBunches)
    {
        // Write "more bunches" marker
        PacketWriter.WriteBit(1);
        Bunch->WriteBunchHeader(PacketWriter);

        // Write data size and data
        uint32 DataBits = static_cast<uint32>(Bunch->GetNumBits());
        PacketWriter.SerializeIntPacked(DataBits);
        PacketWriter.SerializeBits(Bunch->GetData(), Bunch->GetNumBits());

        bHasBunches = true;
        delete Bunch;
    }
    PendingOutBunches.Empty();

    // Write end marker
    PacketWriter.WriteBit(0);

    // Commit sequence
    PacketNotify.CommitAndIncrementOutSeq();
    OutPacketId++;

    // Send
    if (PacketWriter.GetNumBytes() > 0)
    {
        // Prepend Fortnite MagicHeader(4=0x7) + HandshakeBit(0 = game packet)
        FBitWriter FinalPacket(PacketWriter.GetNumBits() + 5, true);
        FinalPacket.WriteBit(1); FinalPacket.WriteBit(1); FinalPacket.WriteBit(1); FinalPacket.WriteBit(0); // Magic 0b0111
        FinalPacket.WriteBit(0); // HandshakeBit = 0
        FinalPacket.SerializeBits(PacketWriter.GetData(), PacketWriter.GetNumBits());

        LowLevelSend(FinalPacket.GetData(), static_cast<int32>(FinalPacket.GetNumBytes()));
    }
}

void UNetConnection::SendPacket(FBitWriter& Writer)
{
    LowLevelSend(Writer.GetData(), static_cast<int32>(Writer.GetNumBytes()));
}

void UNetConnection::LowLevelSend(const uint8* Data, int32 Count)
{
    // Base implementation - override for actual transport
    // In UIpNetDriver/UIpConnection, this calls FSocket::SendTo
    TotalBytesSent += Count;
    PacketsSent++;
    LastSendTime = GetTime();
}

void UNetConnection::Tick(float DeltaTime)
{
    if (State == EConnectionState::USOCK_Closed) return;

    // Tick all channels
    for (int32 i = 0; i < MAX_CHANNELS; i++)
    {
        if (Channels[i])
        {
            Channels[i]->Tick();
        }
    }

    // Flush pending data
    FlushNet();
}

void UNetConnection::Close()
{
    if (State != EConnectionState::USOCK_Closed)
    {
        State = EConnectionState::USOCK_Closed;
        if (OnStateChanged) OnStateChanged(State);
    }
}
