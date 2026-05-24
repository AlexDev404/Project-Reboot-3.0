// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Classes/Engine/DataBunch.h
// Adapted for standalone compilation

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"

class UChannel;
class UNetConnection;

// Channel types (UE4 uses EChannelType in newer versions, int in older)
enum class EChannelType : int32
{
    CHTYPE_None       = 0,
    CHTYPE_Control    = 1,
    CHTYPE_Actor      = 2,
    CHTYPE_File       = 3,
    CHTYPE_Voice      = 4,
    CHTYPE_MAX        = 8
};

// Bunch header flags
struct FBunchHeader
{
    bool bOpen;
    bool bClose;
    bool bDormant;       // Close dormancy (if bClose)
    bool bIsReplicationPaused;
    bool bReliable;
    bool bPartial;
    bool bPartialInitial;
    bool bPartialFinal;
    bool bHasPackageMapExports;
    bool bHasMustBeMappedGUIDs;

    int32 ChIndex;
    EChannelType ChType;
    int32 ChSequence;    // Reliable sequence number (if reliable)
};

/**
 * FInBunch - Incoming data bunch (received from network)
 * In UE4 this extends FNetBitReader
 */
class ENGINE_API FInBunch : public FBitReader
{
public:
    FInBunch(UNetConnection* InConnection, uint8* Src = nullptr, int64 CountBits = 0);

    // Bunch metadata
    int32 PacketId;
    UNetConnection* Connection;
    int32 ChIndex;
    EChannelType ChType;
    int32 ChSequence;

    bool bOpen;
    bool bClose;
    bool bDormant;
    bool bIsReplicationPaused;
    bool bReliable;
    bool bPartial;
    bool bPartialInitial;
    bool bPartialFinal;
    bool bHasPackageMapExports;
    bool bHasMustBeMappedGUIDs;
    bool bHasGUIDs;

    // Linked list for partial bunch assembly
    FInBunch* Next;

    // Read bunch header from a packet reader
    static bool ReadBunchHeader(FBitReader& PacketReader, FBunchHeader& OutHeader);
};

/**
 * FOutBunch - Outgoing data bunch (to send over network)
 * In UE4 this extends FNetBitWriter
 */
class ENGINE_API FOutBunch : public FBitWriter
{
public:
    FOutBunch();
    FOutBunch(UChannel* InChannel, bool bInClose);
    FOutBunch(int64 InMaxBits);

    // Bunch metadata
    UChannel* Channel;
    int32 ChIndex;
    EChannelType ChType;
    int32 ChSequence;

    bool bOpen;
    bool bClose;
    bool bDormant;
    bool bIsReplicationPaused;
    bool bReliable;
    bool bPartial;
    bool bPartialInitial;
    bool bPartialFinal;
    bool bHasPackageMapExports;
    bool bHasMustBeMappedGUIDs;

    // For reliable ordering
    FOutBunch* Next;

    // Write bunch header into packet writer
    void WriteBunchHeader(FBitWriter& PacketWriter) const;

    // Get total header+data size
    int64 GetTotalBits() const;
};
