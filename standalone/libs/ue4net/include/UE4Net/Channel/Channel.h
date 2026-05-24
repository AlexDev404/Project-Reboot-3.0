// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Classes/Engine/Channel.h
// Adapted for standalone compilation

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Channel/DataBunch.h"

class UNetConnection;

/**
 * UChannel - Base class for network channels
 * UE4's channel system multiplexes different data streams over a single connection.
 * Channel 0 is always the control channel.
 */
class ENGINE_API UChannel
{
public:
    UChannel();
    virtual ~UChannel();

    // Channel info
    int32 ChIndex;
    EChannelType ChType;
    UNetConnection* Connection;

    // State
    bool bOpened;
    bool bClosed;
    bool bBroken;
    bool bPendingDormancy;
    int32 OpenedLocally;    // Who opened: 0=unknown, 1=local, 2=remote

    // Reliable sequencing
    int32 NumInRec;         // Number of reliable incoming messages received
    int32 NumOutRec;        // Number of reliable outgoing messages sent
    int32 OutReliable[256]; // Map of reliable outgoing sequence -> bunch

    // Virtual interface
    virtual void Init(UNetConnection* InConnection, int32 InChIndex, EChannelType InChType);
    virtual void Close();
    virtual void Tick();

    // Send a bunch on this channel
    virtual int64 SendBunch(FOutBunch* Bunch, bool Merge);

    // Process incoming bunch
    virtual void ReceivedBunch(FInBunch& Bunch) = 0;

    // Called when channel is opened
    virtual void ReceivedOpenedBunch() {}

    // Partial bunch assembly
    bool ReceivedSequencedBunch(FInBunch& Bunch);
    bool ReceivedNextBunch(FInBunch& Bunch, bool& bOutSkipAck);

    // Reliable bunch tracking
    int32 IsNetReady(bool Saturate) const;

    // Get the sequence for next reliable bunch
    int32 GetNextReliableSequence() { return ++NumOutRec; }

protected:
    // Partial bunch reassembly storage
    FInBunch* InPartialBunch;
};
