// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Classes/Engine/ControlChannel.h
// Adapted for standalone compilation

#pragma once

#include "UE4Net/Channel/Channel.h"

// NMT (Net Message Type) - Control channel message types
// These are the messages exchanged during connection setup
enum class ENMTType : uint8
{
    NMT_Hello           = 0,    // Client → Server: Initial hello with version info
    NMT_Welcome         = 1,    // Server → Client: Map name, game class, URL
    NMT_Upgrade         = 2,    // Server → Client: Version upgrade required
    NMT_Challenge       = 3,    // Server → Client: Authentication challenge
    NMT_Netspeed        = 4,    // Client → Server: Client netspeed
    NMT_Login           = 5,    // Client → Server: Login request with URL
    NMT_Failure         = 6,    // Server → Client: Login failure reason
    NMT_Join            = 9,    // Client → Server: Join request
    NMT_JoinSplit       = 10,   // Client → Server: Split screen player join
    NMT_Skip            = 12,   // Skip
    NMT_Abort           = 13,   // Abort connection
    NMT_PCSwap          = 15,   // Player controller swap
    NMT_ActorChannelFailure = 16,
    NMT_DebugText       = 17,
    NMT_NetGUIDAssign   = 18,
    NMT_SecurityViolation = 19,
    NMT_EncryptionAck   = 26,
    NMT_DestructionInfo = 27,
};

/**
 * UControlChannel - Channel 0 on every connection
 * Handles the NMT message flow for connection setup:
 *   Client: NMT_Hello → NMT_Login → NMT_Join
 *   Server: NMT_Challenge → NMT_Welcome → (map load ack)
 */
class ENGINE_API UControlChannel : public UChannel
{
public:
    UControlChannel();
    virtual ~UControlChannel() override;

    virtual void Init(UNetConnection* InConnection, int32 InChIndex, EChannelType InChType) override;
    virtual void ReceivedBunch(FInBunch& Bunch) override;
    virtual void Tick() override;

    // Send a control message (NMT)
    bool SendControlMessage(ENMTType MessageType, FBitWriter& MessageData);

    // High-level message senders
    bool SendHello(int32 ProtocolVersion, bool bEncrypted, const FString& EncryptionToken);
    bool SendChallenge(const FString& ChallengeString);
    bool SendLogin(const FString& URL, const FString& UniqueId, const FString& OnlinePlatformName);
    bool SendWelcome(const FString& MapName, const FString& GameName, const FString& URL);
    bool SendNetspeed(int32 Rate);
    bool SendFailure(const FString& Reason);

    // Callbacks (override in derived classes or set delegates)
    using FOnControlMessage = std::function<void(ENMTType Type, FBitReader& MessageData)>;
    FOnControlMessage OnControlMessage;

private:
    // Queue of pending outgoing messages
    struct FPendingControlMessage
    {
        ENMTType Type;
        TArray<uint8> Data;
    };
    TArray<FPendingControlMessage> QueuedMessages;
};
