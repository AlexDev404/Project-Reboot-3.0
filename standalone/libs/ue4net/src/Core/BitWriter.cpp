// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Core/Private/Serialization/BitWriter.cpp

#include "UE4Net/Serialization/BitWriter.h"
#include "UE4Net/Serialization/BitReader.h"

extern const uint8 GShift[8];
extern const uint8 GMask[8];

/*-----------------------------------------------------------------------------
FBitWriter.
-----------------------------------------------------------------------------*/

FBitWriter::FBitWriter( int64 InMaxBits, bool InAllowResize )
: Num(0)
, Max(InMaxBits)
, bAllowOverflow(false)
{
Buffer.AddUninitialized( (InMaxBits+7)>>3 );
bAllowResize = InAllowResize;
FMemory::Memzero(Buffer.GetData(), Buffer.Num());
this->SetIsSaving(true);
this->SetIsPersistent(true);
ArIsNetArchive = true;
}

FBitWriter::FBitWriter(void)
: Num(0)
, Max(0)
, bAllowResize(false)
, bAllowOverflow(false)
{
this->SetIsSaving(true);
this->SetIsPersistent(true);
ArIsNetArchive = true;
}

void FBitWriter::Reset(void)
{
FArchive::Reset();
Num = 0;
FMemory::Memzero(Buffer.GetData(), Buffer.Num());
this->SetIsSaving(true);
this->SetIsPersistent(true);
ArIsNetArchive = true;
}

void FBitWriter::SerializeBits( void* Src, int64 LengthBits )
{
if( AllowAppend(LengthBits) )
{
if( LengthBits == 1 )
{
if( ((uint8*)Src)[0] & 0x01 )
Buffer[Num>>3] |= GShift[Num&7];
Num++;
}
else
{
appBitsCpy(Buffer.GetData(), Num, (uint8*)Src, 0, LengthBits);
Num += LengthBits;
}
}
else
{
SetOverflowed(LengthBits);
}
}

void FBitWriter::SerializeBitsWithOffset( void* Src, int32 SourceBit, int64 LengthBits )
{
if( AllowAppend(LengthBits) )
{
appBitsCpy(Buffer.GetData(), Num, (uint8*)Src, SourceBit, LengthBits);
Num += LengthBits;
}
else
{
SetOverflowed(LengthBits);
}
}

void FBitWriter::Serialize( void* Src, int64 LengthBytes )
{
int64 LengthBits = LengthBytes*8;
if( AllowAppend(LengthBits) )
{
appBitsCpy(Buffer.GetData(), Num, (uint8*)Src, 0, LengthBits);
Num += LengthBits;
}
else
{
SetOverflowed(LengthBits);
}
}

void FBitWriter::SerializeInt(uint32& Value, uint32 ValueMax)
{
check(ValueMax >= 2);

const int32 LengthBits = FMath::CeilLogTwo(ValueMax);
uint32 WriteValue = Value;

if (WriteValue >= ValueMax)
{
WriteValue = ValueMax - 1;
}

if (AllowAppend(LengthBits))
{
uint32 NewValue = 0;
int64 LocalNum = Num;

for (uint32 Mask=1; (NewValue + Mask) < ValueMax && Mask; Mask*=2, LocalNum++)
{
if (WriteValue & Mask)
{
Buffer[LocalNum >> 3] += GShift[LocalNum & 7];
NewValue += Mask;
}
}
Num = LocalNum;
}
else
{
SetOverflowed(LengthBits);
}
}

void FBitWriter::WriteIntWrapped(uint32 Value, uint32 ValueMax)
{
check(ValueMax >= 2);

const int32 LengthBits = FMath::CeilLogTwo(ValueMax);

if (AllowAppend(LengthBits))
{
uint32 NewValue = 0;
for (uint32 Mask=1; NewValue+Mask < ValueMax && Mask; Mask*=2, Num++)
{
if (Value & Mask)
{
Buffer[Num>>3] += GShift[Num&7];
NewValue += Mask;
}
}
}
else
{
SetOverflowed(LengthBits);
}
}

void FBitWriter::WriteBit( uint8 In )
{
if( AllowAppend(1) )
{
if( In )
Buffer[Num>>3] |= GShift[Num&7];
Num++;
}
else
{
SetOverflowed(1);
}
}

void FBitWriter::SetOverflowed(int32 LengthBits)
{
if (!bAllowOverflow)
{
// Log overflow
}
SetError();
}

void FBitWriter::CountMemory(FArchive& Ar) const
{
Buffer.CountBytes(Ar);
Ar.CountBytes(sizeof(*this), sizeof(*this));
}

void FBitWriter::SerializeIntPacked(uint32& InValue)
{
uint32 Value = InValue;
uint32 BytesAsWords[5];
uint32 ByteCount = 0;
for (unsigned It = 0; (It == 0) | (Value != 0); ++It, Value = Value >> 7U)
{
const uint32 NextByteIndicator = (Value & ~0x7FU) != 0;
const uint32 ByteAsWord = ((Value & 0x7FU) << 1U) | NextByteIndicator;
BytesAsWords[ByteCount++] = ByteAsWord;
}

const int64 LengthBits = ByteCount * 8;
if (!AllowAppend(LengthBits))
{
SetOverflowed(LengthBits);
return;
}

const uint32 BitCountUsedInByte = Num & 7;
const uint32 BitCountLeftInByte = 8 - (Num & 7);
const uint8 DestMaskByte0 = uint8((1U << BitCountUsedInByte) - 1U);
const uint8 DestMaskByte1 = 0xFFU ^ DestMaskByte0;
const bool bStraddlesTwoBytes = (BitCountUsedInByte != 0);
uint8* Dest = Buffer.GetData() + (Num >> 3U);

Num += LengthBits;
for (uint32 ByteIt = 0; ByteIt != ByteCount; ++ByteIt)
{
const uint32 ByteAsWord = BytesAsWords[ByteIt];

*Dest = (*Dest & DestMaskByte0) | uint8(ByteAsWord << BitCountUsedInByte);
++Dest;
if (bStraddlesTwoBytes)
*Dest = (*Dest & DestMaskByte1) | uint8(ByteAsWord >> BitCountLeftInByte);
}
}

/*-----------------------------------------------------------------------------
FBitWriterMark.
-----------------------------------------------------------------------------*/

void FBitWriterMark::Pop( FBitWriter& Writer )
{
checkSlow(Num<=Writer.Num);
checkSlow(Num<=Writer.Max);

if( Num&7 )
{
Writer.Buffer[Num>>3] &= GMask[Num&7];
}
int32 Start = (Num       +7)>>3;
int32 End   = (Writer.Num+7)>>3;
if( End != Start )
{
checkSlow(Start<Writer.Buffer.Num());
checkSlow(End<=Writer.Buffer.Num());
FMemory::Memzero( &Writer.Buffer[Start], End-Start );
}

if (Overflowed)
{
Writer.SetError();
}
else
{
Writer.ClearError();
}
Writer.Num = Num;
}

void FBitWriterMark::Copy( FBitWriter& Writer, TArray<uint8> &Buffer )
{
checkSlow(Num<=Writer.Num);
checkSlow(Num<=Writer.Max);

int32 Bytes = (Writer.Num - Num + 7) >> 3;
if( Bytes > 0 )
{
Buffer.SetNumUninitialized(Bytes);
Buffer[Bytes-1] = 0;
appBitsCpy(Buffer.GetData(), 0, Writer.Buffer.GetData(), Num, Writer.Num - Num);
}
}
