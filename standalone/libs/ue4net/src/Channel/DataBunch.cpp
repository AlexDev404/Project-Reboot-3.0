// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/DataChannel.cpp
// Bunch serialization/deserialization

#include "UE4Net/Channel/DataBunch.h"
#include "UE4Net/Channel/NetConnection.h"

// ============================================================================
// FInBunch
// ============================================================================

FInBunch::FInBunch(UNetConnection* InConnection, uint8* Src, int64 CountBits)
    : FBitReader(Src, CountBits)
    , PacketId(0)
    , Connection(InConnection)
    , ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(false)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , bHasGUIDs(false)
    , Next(nullptr)
{
}

bool FInBunch::ReadBunchHeader(FBitReader& PacketReader, FBunchHeader& OutHeader)
{
    // UE4 bunch header format (bit-packed):
    // bControl (1 bit) - indicates if this is channel open/close
    // bOpen (1 bit if bControl)
    // bClose (1 bit if bControl)
    // bDormant (1 bit if bClose)
    // bIsReplicationPaused (1 bit)
    // bReliable (1 bit)
    // ChIndex (variable, typically 10-15 bits using packed int)
    // bHasPackageMapExports (1 bit)
    // bHasMustBeMappedGUIDs (1 bit)
    // bPartial (1 bit)
    // bPartialInitial (1 bit if bPartial)
    // bPartialFinal (1 bit if bPartial)
    // ChType (if bPartialInitial or bOpen) (4 bits)
    // ChSequence (if bReliable) (packed int)
    // DataBitSize (packed int)

    uint8 bControl = PacketReader.ReadBit();
    if (PacketReader.IsError()) return false;

    OutHeader.bOpen = false;
    OutHeader.bClose = false;
    OutHeader.bDormant = false;

    if (bControl)
    {
        OutHeader.bOpen = PacketReader.ReadBit() != 0;
        OutHeader.bClose = PacketReader.ReadBit() != 0;
        if (OutHeader.bClose)
        {
            OutHeader.bDormant = PacketReader.ReadBit() != 0;
        }
    }

    OutHeader.bIsReplicationPaused = PacketReader.ReadBit() != 0;
    OutHeader.bReliable = PacketReader.ReadBit() != 0;

    // Channel index (packed)
    uint32 ChIndex = 0;
    PacketReader.SerializeIntPacked(ChIndex);
    OutHeader.ChIndex = static_cast<int32>(ChIndex);

    OutHeader.bHasPackageMapExports = PacketReader.ReadBit() != 0;
    OutHeader.bHasMustBeMappedGUIDs = PacketReader.ReadBit() != 0;

    OutHeader.bPartial = PacketReader.ReadBit() != 0;

    if (OutHeader.bPartial)
    {
        OutHeader.bPartialInitial = PacketReader.ReadBit() != 0;
        OutHeader.bPartialFinal = PacketReader.ReadBit() != 0;
    }
    else
    {
        OutHeader.bPartialInitial = false;
        OutHeader.bPartialFinal = false;
    }

    // Channel type (only present on open or partial initial)
    if (OutHeader.bOpen || OutHeader.bPartialInitial)
    {
        uint32 TypeVal = 0;
        PacketReader.SerializeInt(TypeVal, static_cast<uint32>(EChannelType::CHTYPE_MAX));
        OutHeader.ChType = static_cast<EChannelType>(TypeVal);
    }
    else
    {
        OutHeader.ChType = EChannelType::CHTYPE_None;
    }

    // Reliable sequence
    if (OutHeader.bReliable)
    {
        uint32 Seq = 0;
        PacketReader.SerializeIntPacked(Seq);
        OutHeader.ChSequence = static_cast<int32>(Seq);
    }
    else
    {
        OutHeader.ChSequence = 0;
    }

    return !PacketReader.IsError();
}

// ============================================================================
// FOutBunch
// ============================================================================

FOutBunch::FOutBunch()
    : FBitWriter(0, true)
    , Channel(nullptr)
    , ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(false)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , Next(nullptr)
{
}

FOutBunch::FOutBunch(UChannel* InChannel, bool bInClose)
    : FBitWriter(1024, true)
    , Channel(InChannel)
    , ChIndex(InChannel ? InChannel->ChIndex : 0)
    , ChType(InChannel ? InChannel->ChType : EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(bInClose)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , Next(nullptr)
{
}

FOutBunch::FOutBunch(int64 InMaxBits)
    : FBitWriter(InMaxBits, true)
    , Channel(nullptr)
    , ChIndex(0)
    , ChType(EChannelType::CHTYPE_None)
    , ChSequence(0)
    , bOpen(false)
    , bClose(false)
    , bDormant(false)
    , bIsReplicationPaused(false)
    , bReliable(false)
    , bPartial(false)
    , bPartialInitial(false)
    , bPartialFinal(false)
    , bHasPackageMapExports(false)
    , bHasMustBeMappedGUIDs(false)
    , Next(nullptr)
{
}

void FOutBunch::WriteBunchHeader(FBitWriter& PacketWriter) const
{
    bool bControl = bOpen || bClose;
    PacketWriter.WriteBit(bControl ? 1 : 0);

    if (bControl)
    {
        PacketWriter.WriteBit(bOpen ? 1 : 0);
        PacketWriter.WriteBit(bClose ? 1 : 0);
        if (bClose)
        {
            PacketWriter.WriteBit(bDormant ? 1 : 0);
        }
    }

    PacketWriter.WriteBit(bIsReplicationPaused ? 1 : 0);
    PacketWriter.WriteBit(bReliable ? 1 : 0);

    uint32 ChIndexPacked = static_cast<uint32>(ChIndex);
    PacketWriter.SerializeIntPacked(ChIndexPacked);

    PacketWriter.WriteBit(bHasPackageMapExports ? 1 : 0);
    PacketWriter.WriteBit(bHasMustBeMappedGUIDs ? 1 : 0);

    PacketWriter.WriteBit(bPartial ? 1 : 0);
    if (bPartial)
    {
        PacketWriter.WriteBit(bPartialInitial ? 1 : 0);
        PacketWriter.WriteBit(bPartialFinal ? 1 : 0);
    }

    // Channel type
    if (bOpen || bPartialInitial)
    {
        uint32 TypeVal = static_cast<uint32>(ChType);
        PacketWriter.SerializeInt(TypeVal, static_cast<uint32>(EChannelType::CHTYPE_MAX));
    }

    // Reliable sequence
    if (bReliable)
    {
        uint32 Seq = static_cast<uint32>(ChSequence);
        PacketWriter.SerializeIntPacked(Seq);
    }
}

int64 FOutBunch::GetTotalBits() const
{
    // Estimate header bits + data bits
    int64 HeaderBits = 16; // Approximate header
    return HeaderBits + GetNumBits();
}
