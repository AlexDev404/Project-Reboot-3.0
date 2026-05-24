// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted for standalone compilation with UE4Net compatibility shim.

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Serialization/BitReader.h"

extern const uint8 GShift[8];

CORE_API void appBitsCpy( uint8* Dest, int32 DestBit, uint8* Src, int32 SrcBit, int32 BitCount );

/*-----------------------------------------------------------------------------
FBitWriter.
-----------------------------------------------------------------------------*/

struct CORE_API FBitWriter : public FBitArchive
{
friend struct FBitWriterMark;

public:
FBitWriter(void);
FBitWriter( int64 InMaxBits, bool AllowResize = false );

FBitWriter(FBitWriter&) = default;
FBitWriter& operator=(const FBitWriter&) = default;
FBitWriter(FBitWriter&&) = default;
FBitWriter& operator=(FBitWriter&&) = default;

virtual void SerializeBits( void* Src, int64 LengthBits ) override;
virtual void SerializeBitsWithOffset( void* Src, int32 SourceBit, int64 LengthBits ) override;
virtual void SerializeInt(uint32& Value, uint32 Max) override;
virtual void SerializeIntPacked(uint32& Value) override;

void WriteIntWrapped(uint32 Value, uint32 ValueMax);
void WriteBit( uint8 In );
virtual void Serialize( void* Src, int64 LengthBytes ) override;

FORCEINLINE uint8* GetData(void) { return Buffer.GetData(); }
FORCEINLINE const uint8* GetData(void) const { return Buffer.GetData(); }
FORCEINLINE const TArray<uint8>* GetBuffer(void) const { return &Buffer; }

FORCEINLINE int64 GetNumBytes(void) const { return (Num+7)>>3; }
FORCEINLINE int64 GetNumBits(void) const { return Num; }
FORCEINLINE int64 GetMaxBits(void) const { return Max; }

void SetOverflowed(int32 LengthBits);

FORCEINLINE void SetAllowOverflow(bool bInAllow) { bAllowOverflow = bInAllow; }

FORCEINLINE bool AllowAppend(int64 LengthBits)
{
if (Num+LengthBits > Max)
{
if (bAllowResize)
{
Max = FMath::Max<int64>(Max<<1,Num+LengthBits);
int64 ByteMax = (Max+7)>>3;
Buffer.AddZeroed((int32)(ByteMax - Buffer.Num()));
return true;
}
else
{
return false;
}
}
return true;
}

FORCEINLINE void SetAllowResize(bool NewResize) { bAllowResize = NewResize; }

void Reset() override;

FORCEINLINE void WriteAlign() { Num = ( Num + 7 ) & ( ~0x07 ); }

virtual void CountMemory(FArchive& Ar) const;

private:
TArray<uint8> Buffer;
int64   Num;
int64   Max;
bool    bAllowResize;
bool    bAllowOverflow;
};

struct CORE_API FBitWriterMark
{
public:
FBitWriterMark() : Overflowed(false), Num(0) { }
FBitWriterMark( FBitWriter& Writer ) { Init(Writer); }

FORCEINLINE_DEBUGGABLE int64 GetNumBits() const { return Num; }

FORCEINLINE_DEBUGGABLE void Init( FBitWriter& Writer)
{
Num = Writer.Num;
Overflowed = Writer.IsError();
}

void Reset() { Overflowed = false; Num = 0; }
void Pop( FBitWriter& Writer );
void Copy( FBitWriter& Writer, TArray<uint8> &Buffer );

FORCEINLINE_DEBUGGABLE void PopWithoutClear( FBitWriter& Writer ) { Writer.Num = Num; }

private:
bool Overflowed;
int64 Num;
};

// FNetBitWriter / FNetBitReader for networking layer
struct FNetBitWriter : public FBitWriter
{
    using FBitWriter::FBitWriter;
};

struct FNetBitReader : public FBitReader
{
    using FBitReader::FBitReader;
};
