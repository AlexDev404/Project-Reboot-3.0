// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/NetConnection.cpp
// UNetConnection implementation

#include "UE4Net/Channel/NetConnection.h"
#include "UE4Net/Channel/NetDriver.h"

#include <chrono>

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

    // Check if this is a handshake packet
    if (!bHandshakeComplete)
    {
        if (FStatelessConnectHandlerComponent::IsHandshakePacket(Data, Count))
        {
            FBitReader Reader(const_cast<uint8*>(Data), static_cast<int64>(Count) * 8);

            // Skip the first bit (game/handshake discriminator)
            Reader.ReadBit();

            if (HandshakeHandler.GetState() == EHandshakeState::InitializedOnLocal)
            {
                // Server: This is a client's initial packet or challenge response
                if (HandshakeHandler.ProcessChallengeResponse(Reader, RemoteAddressStr))
                {
                    bHandshakeComplete = true;
                    State = EConnectionState::USOCK_Open;
                    if (OnStateChanged) OnStateChanged(State);
                }
            }
            return;
        }
        else if (HandshakeHandler.IsHandshakeComplete())
        {
            bHandshakeComplete = true;
        }
        else
        {
            return; // Not ready for game packets yet
        }
    }

    // Process as game packet
    FBitReader Reader(const_cast<uint8*>(Data), static_cast<int64>(Count) * 8);
    ReceivedPacket(Reader);
}

void UNetConnection::ReceivedPacket(FBitReader& Reader)
{
    // Read packet header (NetPacketNotify)
    FNetPacketNotify::FNotificationHeader NotifyHeader;
    if (!PacketNotify.ReadHeader(NotifyHeader, Reader))
    {
        return; // Invalid packet
    }

    // Process acks
    auto AckFunc = [](FNetPacketNotify::SequenceNumberT Seq, bool bDelivered) {
        // Handle delivery notifications for sent packets
    };

    auto SeqDelta = PacketNotify.Update(NotifyHeader, AckFunc);
    if (SeqDelta <= 0)
    {
        return; // Duplicate or out of order
    }

    // Acknowledge this received packet
    PacketNotify.AckSeq(NotifyHeader.Seq);

    InPacketId++;

    // Read bunches from packet
    while (!Reader.AtEnd() && !Reader.IsError())
    {
        // Check for end-of-packet marker (1 bit = 0 means more bunches, 1 = end)
        uint8 bHasMoreBunches = Reader.ReadBit();
        if (!bHasMoreBunches || Reader.IsError())
        {
            break;
        }

        // Read bunch header
        FBunchHeader BunchHeader;
        if (!FInBunch::ReadBunchHeader(Reader, BunchHeader))
        {
            break;
        }

        // Read bunch data size
        uint32 BunchDataBits = 0;
        Reader.SerializeIntPacked(BunchDataBits);
        if (Reader.IsError() || BunchDataBits > (uint32)Reader.GetBitsLeft())
        {
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
        LowLevelSend(PacketWriter.GetData(), static_cast<int32>(PacketWriter.GetNumBytes()));
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
