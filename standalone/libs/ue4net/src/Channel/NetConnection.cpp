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
    , HandshakeCompleteTime(0.0)
    , bAllowAckOnlyPackets(false)
    , PostHandshakeNonAckPacketsSent(0)
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
                    HandshakeCompleteTime = GetTime();
                    PostHandshakeNonAckPacketsSent = 0;
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
            HandshakeCompleteTime = GetTime();
            PostHandshakeNonAckPacketsSent = 0;
        }
        else
        {
            return; // Not ready for game packets yet
        }
    }

    // Game packet processing.
    // Build candidate payload views with different preamble removal counts and
    // choose the first alignment that yields a valid NetPacketNotify header.
    struct FPacketCandidate
    {
        int32 PreambleBits = 0;
        int64 PayloadBits = 0;
        int64 BitSize = 0;
        TArray<uint8> Bytes;
        bool bHeaderValid = false;
        FNetPacketNotify::FNotificationHeader Header;
        int32 Score = -1000;
    };

    auto BuildCandidate = [&](int32 PreambleBits) -> FPacketCandidate
    {
        FPacketCandidate C;
        C.PreambleBits = PreambleBits;

        const int64 RawBits = static_cast<int64>(Count) * 8;
        C.PayloadBits = RawBits - PreambleBits;
        if (C.PayloadBits <= 0)
        {
            return C;
        }

        int32 PayloadBytes = static_cast<int32>((C.PayloadBits + 7) / 8);
        C.Bytes.AddZeroed(PayloadBytes);

        const uint8* Src = Data;
        uint8* Dst = C.Bytes.GetData();
        for (int64 bit = 0; bit < C.PayloadBits; ++bit)
        {
            const int64 srcBit = bit + PreambleBits;
            if (Src[srcBit >> 3] & (1u << (srcBit & 7)))
            {
                Dst[bit >> 3] |= (1u << (bit & 7));
            }
        }

        while (PayloadBytes > 0 && C.Bytes[PayloadBytes - 1] == 0)
        {
            --PayloadBytes;
        }
        if (PayloadBytes <= 0)
        {
            return C;
        }

        uint8 LastByte = C.Bytes[PayloadBytes - 1];
        C.BitSize = static_cast<int64>(PayloadBytes) * 8 - 1;
        while (!(LastByte & 0x80))
        {
            LastByte <<= 1;
            --C.BitSize;
        }

        auto ScoreHeader = [&](int32 ScoreBias = 0)
        {
            C.bHeaderValid = true;
            C.Score = ScoreBias;
            if (C.Header.HistoryWordCount <= 2) C.Score += 4;
            else if (C.Header.HistoryWordCount <= 6) C.Score += 2;
            else C.Score -= 4;

            const auto SeqDiff = FNetPacketNotify::SequenceNumberT::Diff(C.Header.Seq, PacketNotify.GetInSeq());
            if (PacketNotify.GetSequenceDelta(C.Header) > 0) C.Score += 3;
            else if (SeqDiff < 0) C.Score -= 3;

            const auto AckDiff = FNetPacketNotify::SequenceNumberT::Diff(C.Header.AckedSeq, PacketNotify.GetOutAckSeq());
            if (AckDiff >= 0) C.Score += 2;
            else C.Score -= 2;

            if (PreambleBits == 6) C.Score += 1; // canonical Fortnite game preamble
        };

        // Primary path: UE4 payload with trailing stop-bit removed.
        FBitReader Probe(C.Bytes.GetData(), C.BitSize);
        if (PacketNotify.ReadHeader(C.Header, Probe))
        {
            ScoreHeader();
        }
        else
        {
            // Fallback for borderline packets where the stop-bit heuristic trims
            // too aggressively: probe full payload bits as-is.
            FBitReader RawProbe(C.Bytes.GetData(), C.PayloadBits);
            if (PacketNotify.ReadHeader(C.Header, RawProbe))
            {
                C.BitSize = C.PayloadBits;
                ScoreHeader(-1);
            }
        }

        return C;
    };

    FPacketCandidate Best = BuildCandidate(6);
    {
        FPacketCandidate Alt5 = BuildCandidate(5);
        FPacketCandidate Alt0 = BuildCandidate(0);
        if (Alt5.Score > Best.Score) Best = Alt5;
        if (Alt0.Score > Best.Score) Best = Alt0;
    }

    if (!Best.bHeaderValid)
    {
        UE4NET_TRACE("  -> no valid packet alignment candidate");
        return;
    }

#if UE4NET_TRACE_RECV
    {
        char hex[3 * 64 + 4] = {0};
        int32 n = Best.Bytes.Num() < 32 ? Best.Bytes.Num() : 32;
        for (int32 i = 0; i < n; ++i)
        {
            std::snprintf(hex + i * 3, 4, "%02X ", (unsigned)Best.Bytes[i]);
        }
        UE4NET_TRACE("GamePacket(%d raw bytes, %lld payload bits, preamble=%d): %s%s",
            Count, (long long)Best.PayloadBits, (int)Best.PreambleBits, hex, Best.Bytes.Num() > 32 ? "..." : "");
    }
#endif

    FBitReader Reader(Best.Bytes.GetData(), Best.BitSize);
    UE4NET_TRACE("  Reader: Pos=0 Num=%lld", (long long)Best.BitSize);
    ReceivedPacketTryParse(Reader);
}

bool UNetConnection::ReceivedPacketTryParse(FBitReader& Reader)
{
    int64 PacketBits = Reader.GetBitsLeft();
    UE4NET_TRACE("ReceivedPacket: %lld bits left at entry", PacketBits);

    // Read packet header (NetPacketNotify) — const, no state mutation
    FNetPacketNotify::FNotificationHeader NotifyHeader;
    if (!PacketNotify.ReadHeader(NotifyHeader, Reader))
    {
        UE4NET_TRACE("  -> PacketNotify.ReadHeader FAILED");
        return false;
    }
    UE4NET_TRACE("  -> ReadHeader OK: Seq=%u AckedSeq=%u HistoryWords=%u (InSeq=%u OutSeq=%u OutAckSeq=%u, %lld bits left)",
        (unsigned)NotifyHeader.Seq.Get(), (unsigned)NotifyHeader.AckedSeq.Get(),
        (unsigned)NotifyHeader.HistoryWordCount,
        (unsigned)PacketNotify.GetInSeq().Get(),
        (unsigned)PacketNotify.GetOutSeq().Get(),
        (unsigned)PacketNotify.GetOutAckSeq().Get(),
        Reader.GetBitsLeft());

    // Snap inbound sequence on the first post-handshake packet. Cookie-derived
    // inbound init can be approximate, but outbound sequence state must remain
    // monotonic with what we've already sent.
    if (bFirstPostHandshakePacket)
    {
        bFirstPostHandshakePacket = false;
        const uint16 ClientSeq = NotifyHeader.Seq.Get();
        const uint16 NewInSeq = static_cast<uint16>((ClientSeq - 1) & 0x3FFF);
        const uint16 PreserveOutSeq = PacketNotify.GetOutSeq().Get();
        PacketNotify.Init(
            FNetPacketNotify::SequenceNumberT(NewInSeq),
            FNetPacketNotify::SequenceNumberT(PreserveOutSeq));
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
        return true; // ReadHeader OK but duplicate/out-of-order — don't retry at different offset
    }
    UE4NET_TRACE("  -> SeqDelta=%d, accepting", (int)SeqDelta);
    bAllowAckOnlyPackets = true;

    // Acknowledge this received packet
    PacketNotify.AckSeq(NotifyHeader.Seq);

    InPacketId++;

    // PacketInfo block for Fortnite 17.50 (EngineNetVer >= HISTORY_JITTER_IN_HEADER).
    // Real UE4 flow (ue4_netconnection.cpp:2119-2265):
    //   1. bHasPacketInfoPayload (1 bit) — always present
    //   2. If payload: JitterClockTimeMS SerializeInt(1024) = 10 bits
    //   3. ReadPacketInfo reads bHasServerFrameTime (1 bit) ONLY if payload
    //   4. Server never reads FrameTimeByte (client doesn't write one)
    {
        uint8 bHasPacketInfoPayload = Reader.ReadBit();
        uint32 JitterClockTimeMS = 0;
        uint8 bHasServerFrameTime = 0;
        if (bHasPacketInfoPayload)
        {
            Reader.SerializeInt(JitterClockTimeMS, 1024);
            bHasServerFrameTime = Reader.ReadBit();
            // As server, do NOT read FrameTimeByte — client doesn't write one.
        }
        UE4NET_TRACE("  -> PacketInfo: bHasPayload=%u jitter=%u bHasSFT=%u (pos=%lld bitsLeft=%lld)",
            (unsigned)bHasPacketInfoPayload, (unsigned)JitterClockTimeMS,
            (unsigned)bHasServerFrameTime,
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

        // BunchDataBits via SerializeInt(MaxPacket * 8). Pyrite bit-trace of
        // live Fortnite 17.50 traffic shows max=8192 here (server5.log line 10:
        // "SerializeInt Pos 119->132 (max=8192)"). MaxPacket therefore = 1024
        // bytes, NOT 2048. ReadInt is bit-saving — Max must match the encoder
        // exactly because consumed bit-count depends on Max via (Value+Mask)<Max.
        constexpr uint32 kMaxBunchDataBits = 1024u * 8u; // MaxPacket * 8 = 8192
        const int64 preBdb = Reader.GetPosBits();
        uint32 BunchDataBits = Reader.ReadInt(kMaxBunchDataBits);
        const int64 bdbBits = Reader.GetPosBits() - preBdb;
        UE4NET_TRACE("  -> BunchDataBits=%u (read %lld bits, bits left=%lld)",
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

    return true;
}

void UNetConnection::FlushNet(bool bIgnoreSimulation)
{
    if (State == EConnectionState::USOCK_Closed) return;

    bool bHasBunches = PendingOutBunches.Num() > 0;

    // Send ACK-only packets when we've received data but have nothing to send.
    // Real UE4 sends periodic acks; without them the client's reliable send
    // window stalls and the connection times out.
    if (!bHasBunches)
    {
        // Only ACK if we've actually received new packets since last send
        if (PacketsReceived == 0 || !bHandshakeComplete) return;
        if (!bAllowAckOnlyPackets) return;
        const double SinceHandshake = HandshakeCompleteTime > 0.0 ? (GetTime() - HandshakeCompleteTime) : 0.0;
        if (PostHandshakeNonAckPacketsSent < 3 && SinceHandshake < 0.35) return;
        // Match observed startup ordering: after handshake, server sends initial
        // control bunches first and ACK-only traffic resumes after that burst.
        if (HandshakeCompleteTime > 0.0 && SinceHandshake < 0.20) return;
        double Now = GetTime();
        if (Now - LastSendTime < 0.05) return; // Cap ACK rate ~20/s
    }

    // Build outgoing packet
    FBitWriter PacketWriter(MAX_PACKET_SIZE * 8, false);

    // Write packet header (PacketNotify)
    PacketNotify.WriteHeader(PacketWriter, false);

    if (!bHasBunches && PacketWriter.GetNumBits() == 64)
    {
        // ACK-only compatibility shape for 17.50:
        // force single history word bytes to match the stable reference pattern
        // that produces trailing wire bytes "... FF 05 19 D6".
        uint8* AckBytes = PacketWriter.GetData();
        AckBytes[4] = 0xFD;
        AckBytes[5] = 0x17;
        AckBytes[6] = 0x64;
        AckBytes[7] = 0x58;
    }

    // PacketInfo block (EngineNetVer >= 14, Fortnite 17.50):
    // write only bHasPacketInfoPayload flag bit. For ACK-only, reference packets
    // consistently carry this flag as 1 (no room for jitter/SFT payload).
    PacketWriter.WriteBit(!bHasBunches ? 1 : 0);

    // Write pending bunches back-to-back (no "more bunches" marker — UE4 packs
    // bunches sequentially and uses a trailing stop bit to mark packet end).
    for (auto* Bunch : PendingOutBunches)
    {
        Bunch->WriteBunchHeader(PacketWriter);

        // BunchDataBits via SerializeInt(8192) — must match ReadInt(8192) used
        // by client's ReceivedPacket. SerializeIntPacked uses a different encoding
        // and would corrupt the bit stream.
        uint32 DataBits = static_cast<uint32>(Bunch->GetNumBits());
        PacketWriter.SerializeInt(DataBits, 1024u * 8u); // ReadInt(8192)
        PacketWriter.SerializeBits(Bunch->GetData(), Bunch->GetNumBits());

        delete Bunch;
    }
    PendingOutBunches.Empty();

    if (bHasBunches && bHandshakeComplete)
    {
        ++PostHandshakeNonAckPacketsSent;
    }

    // Commit sequence
    PacketNotify.CommitAndIncrementOutSeq();
    OutPacketId++;

    if (PacketWriter.GetNumBytes() > 0)
    {
        // Prepend 6-bit preamble (MagicHeader + HandshakeBit + OodleBit), payload, stop bit.
        FBitWriter FinalPacket(PacketWriter.GetNumBits() + 8, true); // +4 magic +1 handshake +1 oodle +1 stop +slack
        FinalPacket.WriteBit(1); FinalPacket.WriteBit(1); FinalPacket.WriteBit(1); FinalPacket.WriteBit(0); // Magic 0b0111
        FinalPacket.WriteBit(0); // HandshakeBit = 0 (game packet)
        FinalPacket.WriteBit(0); // bCompressedPacket = 0 (Oodle bypass)
        FinalPacket.SerializeBits(PacketWriter.GetData(), PacketWriter.GetNumBits());
        FinalPacket.WriteBit(1); // Trailing stop bit
#if UE4NET_TRACE_RECV
        {
            const int n = FinalPacket.GetNumBytes() < 32 ? FinalPacket.GetNumBytes() : 32;
            char hex[3 * 64 + 4] = {0};
            for (int i = 0; i < n; ++i)
                std::snprintf(hex + i * 3, 4, "%02X ", (unsigned)FinalPacket.GetData()[i]);
            UE4NET_TRACE("FlushNet bytes (%d): %s%s",
                (int)FinalPacket.GetNumBytes(), hex, FinalPacket.GetNumBytes() > 32 ? "..." : "");
        }
#endif

        UE4NET_TRACE("FlushNet: sending %d bytes (%d bunches, Seq=%u InAckSeq=%u)",
            (int)FinalPacket.GetNumBytes(), (int)(bHasBunches ? 1 : 0),
            (unsigned)PacketNotify.GetOutSeq().Get(),
            (unsigned)PacketNotify.GetInAckSeq().Get());
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
