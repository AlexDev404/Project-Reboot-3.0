// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Classes/Engine/NetConnection.h
// Simplified for standalone compilation

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"
#include "UE4Net/Net/NetPacketNotify.h"
#include "UE4Net/PacketHandlers/StatelessConnectHandlerComponent.h"
#include "UE4Net/Channel/Channel.h"
#include "UE4Net/Channel/ControlChannel.h"
#include "UE4Net/Channel/ActorChannel.h"

class UNetDriver;

// Connection state enum
enum class EConnectionState : uint8
{
    USOCK_Invalid   = 0, // Connection invalid
    USOCK_Closed    = 1, // Connection closed
    USOCK_Pending   = 2, // Connection pending (handshake)
    USOCK_Open      = 3, // Connection open and running
};

/**
 * UNetConnection - A single network connection
 * Contains the channel array, packet assembly, and drives the
 * handshake + NMT flow for connection setup.
 */
class ENGINE_API UNetConnection
{
public:
    static constexpr int32 MAX_CHANNELS = 32767;
    static constexpr int32 MAX_PACKET_SIZE = 1024; // Max UDP payload
    static constexpr int32 MAX_BUNCH_HEADER_BITS = 64;

    UNetConnection();
    virtual ~UNetConnection();

    // Initialization
    virtual void InitBase(UNetDriver* InDriver, class FSocket* InSocket, const FString& InURL, EConnectionState InState);

    // State
    EConnectionState State;
    UNetDriver* Driver;
    FString URL;
    FString RemoteAddressStr;
    double LastReceiveTime;
    double LastSendTime;

    // Packet notification system
    FNetPacketNotify PacketNotify;

    // Handshake
    FStatelessConnectHandlerComponent HandshakeHandler;
    bool bHandshakeComplete;

    // Channels
    UChannel* Channels[MAX_CHANNELS];
    UControlChannel* GetControlChannel() { return static_cast<UControlChannel*>(Channels[0]); }

    // Channel management
    UChannel* CreateChannel(EChannelType Type, int32 ChIndex = -1);
    void DestroyChannel(int32 ChIndex);
    UActorChannel* FindActorChannel(FNetworkGUID NetGUID);

    // Packet handling
    virtual void ReceivedRawPacket(const uint8* Data, int32 Count);
    virtual void FlushNet(bool bIgnoreSimulation = false);

    // Internal: Process a received packet (after handshake)
    void ReceivedPacket(FBitReader& Reader);

    // Internal: Assemble and send a packet
    void SendPacket(FBitWriter& Writer);

    // Low-level send (override for different transports)
    virtual void LowLevelSend(const uint8* Data, int32 Count);

    // Tick
    virtual void Tick(float DeltaTime);

    // Close connection
    virtual void Close();
    bool IsConnected() const { return State == EConnectionState::USOCK_Open; }

    // Sequence tracking
    int32 InPacketId;
    int32 OutPacketId;
    int32 OutAckPacketId;

    // Statistics
    int64 TotalBytesSent;
    int64 TotalBytesReceived;
    int32 PacketsSent;
    int32 PacketsReceived;

    // Callbacks
    using FOnConnectionStateChanged = std::function<void(EConnectionState NewState)>;
    using FOnBunchReceived = std::function<void(FInBunch& Bunch)>;

    FOnConnectionStateChanged OnStateChanged;

private:
    // Pending outgoing bunches (buffered before flush)
    TArray<FOutBunch*> PendingOutBunches;

    // Next channel index to allocate
    int32 NextChannelIndex;
};
