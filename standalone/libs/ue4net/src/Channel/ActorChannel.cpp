// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/DataChannel.cpp
// UActorChannel implementation

#include "UE4Net/Channel/ActorChannel.h"
#include "UE4Net/Channel/NetConnection.h"

UActorChannel::UActorChannel()
    : Actor(nullptr)
    , bHasInitialSpawnInfo(false)
{
}

UActorChannel::~UActorChannel()
{
}

void UActorChannel::Init(UNetConnection* InConnection, int32 InChIndex, EChannelType InChType)
{
    UChannel::Init(InConnection, InChIndex, EChannelType::CHTYPE_Actor);
}

void UActorChannel::ReceivedBunch(FInBunch& Bunch)
{
    // Actor channel bunch processing:
    // 1. If channel is being opened (bOpen), read actor spawn info
    // 2. Read property replications (RepLayout handles)
    // 3. Read RPC calls

    if (Bunch.bOpen && !bHasInitialSpawnInfo)
    {
        // Read actor net GUID
        uint32 GuidVal = 0;
        Bunch.SerializeIntPacked(GuidVal);
        ActorNetGUID = FNetworkGUID(GuidVal);

        bHasInitialSpawnInfo = true;
        bOpened = true;

        if (OnActorChannelOpened)
        {
            OnActorChannelOpened(ActorNetGUID, Bunch);
        }
    }

    // Process remaining data as properties/RPCs
    while (!Bunch.AtEnd() && !Bunch.IsError())
    {
        // Read property handle or RPC marker
        // In UE4, a handle of 0 means "end of properties, RPCs follow"
        uint16 Handle = 0;
        Bunch.Serialize(&Handle, sizeof(uint16));

        if (Bunch.IsError()) break;

        if (Handle == 0)
        {
            // RPC section
            // Read function name hash
            uint32 FuncNameHash = 0;
            Bunch.SerializeIntPacked(FuncNameHash);
            if (Bunch.IsError()) break;

            // Find RPC name from hash (simplified - in full UE4 uses FName)
            FName FuncName;  // Would need lookup
            if (OnRPCReceived)
            {
                OnRPCReceived(FuncName, Bunch);
            }
            break; // For now, only handle one RPC per bunch
        }
        else
        {
            // Property replication
            // Read property data size
            uint32 NumBits = 0;
            Bunch.SerializeIntPacked(NumBits);
            if (Bunch.IsError() || NumBits > (uint32)Bunch.GetBitsLeft()) break;

            if (OnPropertyReceived && NumBits > 0)
            {
                // Read property data
                TArray<uint8> PropData;
                int32 NumBytes = static_cast<int32>((NumBits + 7) / 8);
                PropData.AddZeroed(NumBytes);
                Bunch.SerializeBits(PropData.GetData(), NumBits);
                OnPropertyReceived(Handle, PropData.GetData(), static_cast<int32>(NumBits));
            }
            else
            {
                // Skip property data
                int64 CurrentBit = Bunch.GetPosBits();
                // Cannot easily skip without reading - simplified
                break;
            }
        }
    }

    if (Bunch.bClose)
    {
        Close();
    }
}

void UActorChannel::Close()
{
    UChannel::Close();
}

void UActorChannel::Tick()
{
    // Actor channels don't need independent ticking in this implementation
}

void UActorChannel::SetChannelActor(UObject* InActor, FNetworkGUID InNetGUID)
{
    Actor = InActor;
    ActorNetGUID = InNetGUID;
    bHasInitialSpawnInfo = true;
}

bool UActorChannel::ReplicateActor()
{
    if (!Actor || !Connection) return false;

    // Would serialize all dirty properties via FRepLayout
    // For now, this is a placeholder for the full replication flow
    return true;
}

void UActorChannel::RegisterRPC(const FName& FunctionName, FRPCHandler Handler)
{
    FRPCEntry Entry;
    Entry.Name = FunctionName;
    Entry.Handler = std::move(Handler);
    RPCHandlers.Add(std::move(Entry));
}

void UActorChannel::ProcessRPC(FBitReader& Bunch, const FName& FunctionName)
{
    for (auto& Entry : RPCHandlers)
    {
        if (Entry.Name == FunctionName)
        {
            Entry.Handler(Bunch);
            return;
        }
    }
}

void UActorChannel::SendRPC(const FName& FunctionName, FBitWriter& Payload, bool bReliable)
{
    FOutBunch Bunch(this, false);
    Bunch.bReliable = bReliable;

    // Write end-of-properties marker
    uint16 ZeroHandle = 0;
    Bunch.Serialize(&ZeroHandle, sizeof(uint16));

    // Write function identifier (simplified - UE4 uses FName network index)
    uint32 FuncHash = 0; // Would be computed from FunctionName
    Bunch.SerializeIntPacked(FuncHash);

    // Write payload
    if (Payload.GetNumBits() > 0)
    {
        Bunch.SerializeBits(Payload.GetData(), Payload.GetNumBits());
    }

    SendBunch(&Bunch, false);
}

bool UActorChannel::SerializeNewActor(FBitWriter& Ar)
{
    // Write actor spawn data for initial replication
    uint32 GuidVal = ActorNetGUID.Value;
    Ar.SerializeIntPacked(GuidVal);
    return !Ar.IsError();
}

bool UActorChannel::ProcessNewActor(FBitReader& Ar)
{
    uint32 GuidVal = 0;
    Ar.SerializeIntPacked(GuidVal);
    ActorNetGUID = FNetworkGUID(GuidVal);
    bHasInitialSpawnInfo = true;
    return !Ar.IsError();
}
