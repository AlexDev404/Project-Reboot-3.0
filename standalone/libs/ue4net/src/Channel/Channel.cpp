// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/DataChannel.cpp
// UChannel base implementation

#include "UE4Net/Channel/Channel.h"
#include "UE4Net/Channel/NetConnection.h"

UChannel::UChannel()
    : ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , Connection(nullptr)
    , bOpened(false)
    , bClosed(false)
    , bBroken(false)
    , bPendingDormancy(false)
    , OpenedLocally(0)
    , NumInRec(0)
    , NumOutRec(0)
    , InPartialBunch(nullptr)
{
    FMemory::Memzero(OutReliable, sizeof(OutReliable));
}

UChannel::~UChannel()
{
    if (InPartialBunch)
    {
        delete InPartialBunch;
        InPartialBunch = nullptr;
    }
}

void UChannel::Init(UNetConnection* InConnection, int32 InChIndex, EChannelType InChType)
{
    Connection = InConnection;
    ChIndex = InChIndex;
    ChType = InChType;
}

void UChannel::Close()
{
    if (!bClosed)
    {
        bClosed = true;
        // Send close bunch
        FOutBunch CloseBunch(this, true);
        CloseBunch.bReliable = true;
        CloseBunch.bClose = true;
        SendBunch(&CloseBunch, false);
    }
}

void UChannel::Tick()
{
    // Base implementation does nothing
}

int64 UChannel::SendBunch(FOutBunch* Bunch, bool Merge)
{
    if (!Connection || bBroken) return 0;

    Bunch->ChIndex = ChIndex;
    Bunch->ChType = ChType;

    if (Bunch->bReliable)
    {
        Bunch->ChSequence = GetNextReliableSequence();
    }

    // Write to connection's send buffer
    // In UE4, this goes through partial bunch splitting if > MTU
    int64 BitsWritten = Bunch->GetNumBits();

    // For now, queue on connection for next flush
    // Full implementation would handle partial bunches here
    return BitsWritten;
}

bool UChannel::ReceivedSequencedBunch(FInBunch& Bunch)
{
    // Handle partial bunch reassembly
    if (Bunch.bPartial)
    {
        if (Bunch.bPartialInitial)
        {
            // Start of new partial bunch
            if (InPartialBunch)
            {
                delete InPartialBunch;
            }
            // Store first partial
            InPartialBunch = new FInBunch(Bunch.Connection, Bunch.GetData(), Bunch.GetNumBits());
            InPartialBunch->bPartialInitial = true;
            InPartialBunch->bOpen = Bunch.bOpen;
            InPartialBunch->bClose = Bunch.bClose;
            InPartialBunch->ChIndex = Bunch.ChIndex;
            InPartialBunch->ChType = Bunch.ChType;
            return true; // Wait for more
        }
        else if (InPartialBunch)
        {
            // Append to existing partial
            // In full UE4, this merges bit data
            if (Bunch.bPartialFinal)
            {
                // Complete! Deliver the assembled bunch
                ReceivedBunch(*InPartialBunch);
                delete InPartialBunch;
                InPartialBunch = nullptr;
                return true;
            }
            return true; // Still waiting for final
        }
        else
        {
            // Got continuation without initial - discard
            return false;
        }
    }

    // Not partial - deliver directly
    ReceivedBunch(Bunch);
    return true;
}

bool UChannel::ReceivedNextBunch(FInBunch& Bunch, bool& bOutSkipAck)
{
    bOutSkipAck = false;

    if (Bunch.bReliable)
    {
        // Check if this is the next expected reliable sequence
        if (Bunch.ChSequence == NumInRec + 1)
        {
            NumInRec = Bunch.ChSequence;
            return ReceivedSequencedBunch(Bunch);
        }
        else if (Bunch.ChSequence <= NumInRec)
        {
            // Duplicate - already received
            return true;
        }
        else
        {
            // Out of order - would need to queue (simplified: skip)
            bOutSkipAck = true;
            return false;
        }
    }
    else
    {
        // Unreliable - deliver immediately
        return ReceivedSequencedBunch(Bunch);
    }
}

int32 UChannel::IsNetReady(bool Saturate) const
{
    return (Connection != nullptr && !bBroken) ? 1 : 0;
}
