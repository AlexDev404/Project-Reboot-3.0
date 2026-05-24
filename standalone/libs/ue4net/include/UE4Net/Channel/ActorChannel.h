// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Classes/Engine/ActorChannel.h
// Adapted for standalone compilation

#pragma once

#include "UE4Net/Channel/Channel.h"

class UObject;

// Forward declarations for replication structures
struct FRepLayout;
struct FObjectReplicator;

// FNetworkGUID is defined in CoreMinimal.h

/**
 * UActorChannel - Channel for replicating a single actor
 * Each replicated actor gets its own channel. This handles:
 * - Initial actor spawn/open
 * - Property replication (via FRepLayout)
 * - Subobject replication
 * - RPC dispatch
 * - Actor close/destroy
 */
class ENGINE_API UActorChannel : public UChannel
{
public:
    UActorChannel();
    virtual ~UActorChannel() override;

    virtual void Init(UNetConnection* InConnection, int32 InChIndex, EChannelType InChType) override;
    virtual void ReceivedBunch(FInBunch& Bunch) override;
    virtual void Close() override;
    virtual void Tick() override;

    // Actor info
    FNetworkGUID ActorNetGUID;
    UObject* Actor;  // The replicated actor (server-side)
    bool bHasInitialSpawnInfo;

    // Replication
    void SetChannelActor(UObject* InActor, FNetworkGUID InNetGUID);
    bool ReplicateActor();

    // RPC handling
    using FRPCHandler = std::function<void(FBitReader& Payload)>;
    void RegisterRPC(const FName& FunctionName, FRPCHandler Handler);
    void ProcessRPC(FBitReader& Bunch, const FName& FunctionName);
    void SendRPC(const FName& FunctionName, FBitWriter& Payload, bool bReliable);

    // Object spawn/export
    bool SerializeNewActor(FBitWriter& Ar);
    bool ProcessNewActor(FBitReader& Ar);

    // Property replication data
    struct FPropertyReplication
    {
        uint16 PropertyHandle;
        TArray<uint8> Data;
    };

    // Callbacks
    using FOnActorChannelOpened = std::function<void(FNetworkGUID NetGUID, FBitReader& SpawnData)>;
    using FOnPropertyReceived = std::function<void(uint16 Handle, const uint8* Data, int32 NumBits)>;
    using FOnRPCReceived = std::function<void(const FName& FuncName, FBitReader& Params)>;

    FOnActorChannelOpened OnActorChannelOpened;
    FOnPropertyReceived OnPropertyReceived;
    FOnRPCReceived OnRPCReceived;

private:
    // RPC handlers map
    struct FRPCEntry { FName Name; FRPCHandler Handler; };
    TArray<FRPCEntry> RPCHandlers;
};
