// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Public/Net/RepLayout.h
// Simplified for standalone - provides property replication framework

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"

// Property replication types
enum class ERepLayoutCmdType : uint8
{
    DynamicArray     = 0,
    Return           = 1,
    Property_Bool    = 2,
    Property_Float   = 3,
    Property_Int     = 4,
    Property_Byte    = 5,
    Property_Name    = 6,
    Property_Object  = 7,
    Property_UInt32  = 8,
    Property_Vector  = 9,
    Property_Rotator = 10,
    Property_String  = 11,
    Property_UInt64  = 12,
    PropertyNetId    = 13,
    RepMovement      = 14,
    Property_Vector100 = 15,
    Property_Vector10  = 16,
    Property_VectorNormal = 17,
    Property_VectorQ   = 18,
    Property_UInt16    = 19,
    Property_Int8      = 20,
    Property_Int16     = 21,
};

// A single property command in the replication layout
struct FRepLayoutCmd
{
    uint16 Handle;              // Network property handle
    uint16 Offset;              // Offset into object memory
    uint16 ElementSize;         // Size of one element
    ERepLayoutCmdType Type;     // Property type
    FName PropertyName;         // Name for debugging

    // For arrays
    uint16 EndCmd;              // Index of Return cmd for this array
    int32 ArrayNum;             // Static array count (>1 for fixed arrays)
};

// Parent property info (maps to UProperty in UE4)
struct FRepParentCmd
{
    uint16 CmdStart;            // First cmd index for this parent
    uint16 CmdEnd;              // Last cmd index + 1
    uint16 Handle;              // Network handle
    int32 Offset;               // Memory offset
    FName PropertyName;
    uint32 Flags;               // EPropertyFlags subset
};

/**
 * FRepLayout - Describes how to serialize/deserialize an object's replicated properties
 *
 * In UE4, this is built from UClass reflection data. For standalone, we support
 * both manual registration (for known classes) and runtime building from
 * property descriptors.
 */
class ENGINE_API FRepLayout
{
public:
    FRepLayout();
    ~FRepLayout();

    // Build from manual property list
    void InitFromPropertyList(const FName& ClassName, const TArray<FRepLayoutCmd>& InCmds, const TArray<FRepParentCmd>& InParents);

    // Serialize properties that have changed (compare state)
    bool SerializeProperties(FBitWriter& Writer, const uint8* OldData, const uint8* NewData, int32 DataSize) const;

    // Deserialize received properties into object memory
    bool DeserializeProperties(FBitReader& Reader, uint8* DestData, int32 DataSize) const;

    // Serialize a single property by handle
    bool SerializePropertyByHandle(FBitWriter& Writer, uint16 Handle, const uint8* Data) const;

    // Get info
    const FName& GetClassName() const { return ClassName; }
    int32 GetNumProperties() const { return static_cast<int32>(Cmds.Num()); }
    const FRepLayoutCmd* GetCmd(int32 Index) const { return (Index >= 0 && Index < Cmds.Num()) ? &Cmds[Index] : nullptr; }

    // Find property by name
    int32 FindPropertyHandle(const FName& PropName) const;

private:
    FName ClassName;
    TArray<FRepLayoutCmd> Cmds;
    TArray<FRepParentCmd> Parents;

    // Serialize a property value based on type
    void SerializePropertyValue(FBitWriter& Writer, const FRepLayoutCmd& Cmd, const uint8* Data) const;
    void DeserializePropertyValue(FBitReader& Reader, const FRepLayoutCmd& Cmd, uint8* Data) const;
};

/**
 * FRepState - Tracks replication state for a connection+object pair
 * Used for delta comparison to only send changed properties
 */
struct ENGINE_API FRepState
{
    FRepState();
    ~FRepState();

    // Shadow state (last sent values)
    TArray<uint8> StaticBuffer;

    // Which properties are dirty
    TArray<uint16> DirtyHandles;

    // Compare and mark dirty
    void CompareProperties(const FRepLayout& Layout, const uint8* CurrentData, int32 DataSize);

    // Mark all as dirty (for initial replication)
    void MarkAllDirty(const FRepLayout& Layout);
};
