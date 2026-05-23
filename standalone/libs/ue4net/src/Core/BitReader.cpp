// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Core/Private/Serialization/BitReader.cpp

#include "UE4Net/Serialization/BitReader.h"

// Table.
extern const uint8 GShift[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
extern const uint8 GMask [8]={0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f};

// Optimized arbitrary bit range memory copy routine.
void appBitsCpy( uint8* Dest, int32 DestBit, uint8* Src, int32 SrcBit, int32 BitCount )
{
if( BitCount==0 ) return;

if( BitCount <= 8 )
{
uint32 DestIndex    = DestBit/8;
uint32 SrcIndex     = SrcBit /8;
uint32 LastDest     =( DestBit+BitCount-1 )/8;
uint32 LastSrc      =( SrcBit +BitCount-1 )/8;
uint32 ShiftSrc     = SrcBit & 7;
uint32 ShiftDest    = DestBit & 7;
uint32 FirstMask    = 0xFF << ShiftDest;
uint32 LastMask     = 0xFE << ((DestBit + BitCount-1) & 7);
uint32 Accu;

if( SrcIndex == LastSrc )
Accu = (Src[SrcIndex] >> ShiftSrc);
else
Accu =( (Src[SrcIndex] >> ShiftSrc) | (Src[LastSrc ] << (8-ShiftSrc)) );

if( DestIndex == LastDest )
{
uint32 MultiMask = FirstMask & ~LastMask;
Dest[DestIndex] = ( ( Dest[DestIndex] & ~MultiMask ) | ((Accu << ShiftDest) & MultiMask) );
}
else
{
Dest[DestIndex] = (uint8)( ( Dest[DestIndex] & ~FirstMask ) | (( Accu << ShiftDest) & FirstMask) ) ;
Dest[LastDest ] = (uint8)( ( Dest[LastDest ] & LastMask  )  | (( Accu >> (8-ShiftDest)) & ~LastMask) ) ;
}
return;
}

// Main copier
uint32 DestIndex     = DestBit/8;
uint32 FirstSrcMask  = 0xFF << ( DestBit & 7);
uint32 LastDest      = ( DestBit+BitCount )/8;
uint32 LastSrcMask   = 0xFF << ((DestBit + BitCount) & 7);
uint32 SrcIndex      = SrcBit/8;
uint32 LastSrc       = ( SrcBit+BitCount )/8;
int32   ShiftCount    = (DestBit & 7) - (SrcBit & 7);
int32   DestLoop      = LastDest-DestIndex;
int32   SrcLoop       = LastSrc -SrcIndex;
uint32 FullLoop;
uint32 BitAccu;

if( ShiftCount>=0 )
{
FullLoop  = FMath::Max(DestLoop, SrcLoop);
BitAccu   = Src[SrcIndex] << ShiftCount;
ShiftCount += 8;
}
else
{
ShiftCount +=8;
FullLoop  = FMath::Max(DestLoop, SrcLoop-1);
BitAccu   = Src[SrcIndex] << ShiftCount;
SrcIndex++;
ShiftCount += 8;
BitAccu = ( ( (uint32)Src[SrcIndex] << ShiftCount ) + (BitAccu)) >> 8;
}

Dest[DestIndex] = (uint8) (( BitAccu & FirstSrcMask) | ( Dest[DestIndex] &  ~FirstSrcMask ) );
SrcIndex++;
DestIndex++;

for(; FullLoop>1; FullLoop--)
{
BitAccu = (( (uint32)Src[SrcIndex] << ShiftCount ) + (BitAccu)) >> 8;
SrcIndex++;
Dest[DestIndex] = (uint8) BitAccu;
DestIndex++;
}

if( LastSrcMask != 0xFF)
{
if ((uint32)(SrcBit+BitCount-1)/8 == SrcIndex )
{
BitAccu = ( ( (uint32)Src[SrcIndex] << ShiftCount ) + (BitAccu)) >> 8;
}
else
{
BitAccu = BitAccu >> 8;
}
Dest[DestIndex] = (uint8)( ( Dest[DestIndex] & LastSrcMask ) | (BitAccu & ~LastSrcMask) );
}
}

/*-----------------------------------------------------------------------------
FBitReader.
-----------------------------------------------------------------------------*/

FBitReader::FBitReader(uint8* Src, int64 CountBits)
: Num(CountBits)
, Pos(0)
{
Buffer.AddUninitialized((CountBits + 7) >> 3);

this->SetIsLoading(true);
this->SetIsPersistent(true);
ArIsNetArchive = true;

if (Src != nullptr)
{
FMemory::Memcpy(Buffer.GetData(), Src, (CountBits + 7) >> 3);
if (Num & 7)
{
Buffer[Num >> 3] &= GMask[Num & 7];
}
}
}

void FBitReader::SetData( uint8* Src, int64 CountBits )
{
Num = CountBits;
Pos = 0;
ClearError();

Buffer.Empty();
Buffer.AddUninitialized( (Num+7)>>3 );

if (Src != nullptr)
{
FMemory::Memcpy(Buffer.GetData(), Src, (Num + 7) >> 3);
if (Num & 7)
{
Buffer[Num >> 3] &= GMask[Num & 7];
}
}
}

void FBitReader::SetData( TArray<uint8>&& Src, int64 CountBits )
{
Num = CountBits;
Pos = 0;
ClearError();

Buffer = std::move(Src);

if (Num & 7)
{
Buffer[Num >> 3] &= GMask[Num & 7];
}
}

void FBitReader::SetData( FBitReader& Src, int64 CountBits )
{
Num = CountBits;
Pos = 0;
ClearError();

this->SetEngineNetVer(Src.EngineNetVer());
this->SetGameNetVer(Src.GameNetVer());

Buffer.Empty();
Buffer.AddUninitialized( (CountBits+7)>>3 );
Src.SerializeBits(Buffer.GetData(), CountBits);
}

void FBitReader::AppendDataFromChecked( FBitReader& Src )
{
check(Num % 8 == 0);
Src.AppendTo(Buffer);
Num += Src.GetNumBits();
}

void FBitReader::AppendDataFromChecked( uint8* Src, uint32 NumBits )
{
check(Num % 8 == 0);

uint32 NumBytes = (NumBits+7) >> 3;
Buffer.AddUninitialized(NumBytes);
FMemory::Memcpy( &Buffer[Num >> 3], Src, NumBytes );

Num += NumBits;

if (Num & 7)
{
Buffer[Num >> 3] &= GMask[Num & 7];
}
}

void FBitReader::AppendTo( TArray<uint8> &DestBuffer )
{
DestBuffer.Append(Buffer);
}

void FBitReader::CountMemory(FArchive& Ar) const
{
Buffer.CountBytes(Ar);
Ar.CountBytes(sizeof(*this), sizeof(*this));
}

void FBitReader::SetOverflowed(int64 LengthBits)
{
SetError();
}

void FBitReader::SerializeBitsWithOffset( void* Dest, int32 DestBit, int64 LengthBits )
{
if ( IsError() || Pos+LengthBits > Num)
{
if (!IsError())
{
SetOverflowed(LengthBits);
}
return;
}

if (LengthBits != 0)
{
appBitsCpy((uint8*)Dest, DestBit, Buffer.GetData(), (int32)Pos, (int32)LengthBits);
Pos += LengthBits;
}
}

void FBitReader::SerializeIntPacked(uint32& OutValue)
{
if (IsError())
{
OutValue = 0;
return;
}

const uint8* Src = Buffer.GetData() + (Pos >> 3U);
const uint32 BitCountUsedInByte = Pos & 7;
const uint32 BitCountLeftInByte = 8 - (Pos & 7);
const uint8 SrcMaskByte0 = uint8((1U << BitCountLeftInByte) - 1U);
const uint8 SrcMaskByte1 = uint8((1U << BitCountUsedInByte) - 1U);
const uint32 NextSrcIndex = (BitCountUsedInByte != 0);

uint32 Value = 0;
for (unsigned It = 0, ShiftCount = 0; It < 5; ++It, ShiftCount += 7)
{
if (Pos + 8 > Num)
{
SetOverflowed(8);
break;
}

Pos += 8;

const uint8 Byte = ((Src[0] >> BitCountUsedInByte) & SrcMaskByte0) | ((Src[NextSrcIndex] & SrcMaskByte1) << (BitCountLeftInByte & 7));
const uint8 NextByteIndicator = Byte & 1;
const uint32 ByteAsWord = Byte >> 1U;
Value = (ByteAsWord << ShiftCount) | Value;
++Src;

if (!NextByteIndicator)
{
break;
}
}

OutValue = Value;
}

/*-----------------------------------------------------------------------------
FBitReaderMark.
-----------------------------------------------------------------------------*/

void FBitReaderMark::Copy( FBitReader& Reader, TArray<uint8> &Buffer )
{
checkSlow(Pos<=Reader.Pos);

int32 Bytes = (Reader.Pos - Pos + 7) >> 3;
if( Bytes > 0 )
{
Buffer.SetNumUninitialized(Bytes);
Buffer[Bytes-1] = 0;
appBitsCpy(Buffer.GetData(), 0, Reader.Buffer.GetData(), Pos, Reader.Pos - Pos);
}
}
