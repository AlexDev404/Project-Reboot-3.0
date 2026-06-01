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
        CloseBunch.bControl = true;
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

    // EName hardcoded indices for built-in channel names (verified from server6.log).
    // WriteBunchHeader writes these via SIP when (bReliable || bOpen).
    if (ChType == EChannelType::CHTYPE_Control)
        Bunch->ChNameIndex = 255;   // EName for "Control" — SIP value 255 → name.cmp=714
    else if (ChType == EChannelType::CHTYPE_Actor)
        Bunch->ChNameIndex = 26;    // EName for "Actor"
    else if (ChType == EChannelType::CHTYPE_Voice)
        Bunch->ChNameIndex = 193;   // EName for "Voice"

    if (Bunch->bReliable)
    {
        Bunch->ChSequence = GetNextReliableSequence();
    }

    int64 BitsWritten = Bunch->GetNumBits();

    auto* Copy = new FOutBunch(Bunch->GetNumBits() + 256);
    Copy->Channel = Bunch->Channel;
    Copy->ChIndex = Bunch->ChIndex;
    Copy->ChType = Bunch->ChType;
    Copy->ChSequence = Bunch->ChSequence;
    Copy->bControl = Bunch->bControl;
    Copy->bOpen = Bunch->bOpen;
    Copy->bClose = Bunch->bClose;
    Copy->bDormant = Bunch->bDormant;
    Copy->bIsReplicationPaused = Bunch->bIsReplicationPaused;
    Copy->bReliable = Bunch->bReliable;
    Copy->bPartial = Bunch->bPartial;
    Copy->bPartialInitial = Bunch->bPartialInitial;
    Copy->bPartialFinal = Bunch->bPartialFinal;
    Copy->bHasPackageMapExports = Bunch->bHasPackageMapExports;
    Copy->bHasMustBeMappedGUIDs = Bunch->bHasMustBeMappedGUIDs;
    Copy->ChNameIndex = Bunch->ChNameIndex;
    if (Bunch->GetNumBits() > 0)
    {
        Copy->SerializeBits(Bunch->GetData(), Bunch->GetNumBits());
    }

    Connection->PendingOutBunches.Add(Copy);
    return BitsWritten;
}

bool UChannel::ReceivedSequencedBunch(FInBunch& Bunch)
{
    // Non-partial: deliver immediately. Drop any orphaned partial state in case
    // we missed a prior bPartialFinal.
    if (!Bunch.bPartial)
    {
        if (InPartialBunch) { delete InPartialBunch; InPartialBunch = nullptr; }
        ReceivedBunch(Bunch);
        return true;
    }

    // bPartialInitial: start a fresh accumulator. The initial fragment carries
    // the header flags that the assembled bunch should inherit.
    if (Bunch.bPartialInitial)
    {
        if (InPartialBunch) { delete InPartialBunch; InPartialBunch = nullptr; }
        InPartialBunch = new FInBunch(Bunch.Connection, Bunch.GetData(), Bunch.GetNumBits());
        InPartialBunch->PacketId              = Bunch.PacketId;
        InPartialBunch->ChIndex               = Bunch.ChIndex;
        InPartialBunch->ChType                = Bunch.ChType;
        InPartialBunch->ChSequence            = Bunch.ChSequence;
        InPartialBunch->bOpen                 = Bunch.bOpen;
        InPartialBunch->bClose                = Bunch.bClose;
        InPartialBunch->bDormant              = Bunch.bDormant;
        InPartialBunch->bReliable             = Bunch.bReliable;
        InPartialBunch->bHasPackageMapExports = Bunch.bHasPackageMapExports;
        InPartialBunch->bHasMustBeMappedGUIDs = Bunch.bHasMustBeMappedGUIDs;
        InPartialBunch->bIsReplicationPaused  = Bunch.bIsReplicationPaused;

        if (Bunch.bPartialFinal)
        {
            // Single-fragment "partial" bunch: deliver immediately.
            ReceivedBunch(*InPartialBunch);
            delete InPartialBunch;
            InPartialBunch = nullptr;
        }
        return true;
    }

    // Continuation/final fragment without a prior initial. This happens when we
    // joined mid-stream or missed earlier packets. Fall back to delivering the
    // fragment directly so dispatch still fires for diagnostics; once sequence
    // tracking and packet replay are solid, this fallback can be removed.
    if (!InPartialBunch)
    {
        ReceivedBunch(Bunch);
        return true;
    }

    // Append this fragment's payload bits to the accumulator. UE4 guarantees
    // mid-stream fragments are byte-aligned so AppendDataFromChecked is safe;
    // the final fragment's trailing partial byte is masked by the helper.
    if (Bunch.GetNumBits() > 0)
    {
        InPartialBunch->AppendDataFromChecked(Bunch.GetData(), (uint32)Bunch.GetNumBits());
    }

    if (Bunch.bPartialFinal)
    {
        ReceivedBunch(*InPartialBunch);
        delete InPartialBunch;
        InPartialBunch = nullptr;
    }
    return true;
}

bool UChannel::ReceivedNextBunch(FInBunch& Bunch, bool& bOutSkipAck)
{
    bOutSkipAck = false;

    if (Bunch.bReliable)
    {
        // Bootstrap: on a freshly-opened channel (NumInRec == 0), accept the
        // first reliable bunch regardless of its ChSequence value and adopt
        // that value as the baseline. UE4 normally applies MakeRelative against
        // an InReliable counter; we approximate by latching the first observed
        // value. Subsequent bunches must be NumInRec+1 (wrapped).
        if (NumInRec == 0)
        {
            NumInRec = Bunch.ChSequence;
            return ReceivedSequencedBunch(Bunch);
        }

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
