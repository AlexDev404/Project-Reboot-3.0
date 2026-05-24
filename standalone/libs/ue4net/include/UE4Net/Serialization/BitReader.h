// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted for standalone compilation with UE4Net compatibility shim.

#pragma once

#include "UE4Net/CoreMinimal.h"

CORE_API void appBitsCpy( uint8* Dest, int32 DestBit, uint8* Src, int32 SrcBit, int32 BitCount );

/*-----------------------------------------------------------------------------
FBitReader.
-----------------------------------------------------------------------------*/

struct CORE_API FBitReader : public FBitArchive
{
friend struct FBitReaderMark;

public:
FBitReader( uint8* Src = nullptr, int64 CountBits = 0 );

FBitReader(const FBitReader&) = default;
FBitReader& operator=(const FBitReader&) = default;
FBitReader(FBitReader&&) = default;
FBitReader& operator=(FBitReader&&) = default;

void SetData( FBitReader& Src, int64 CountBits );
void SetData( uint8* Src, int64 CountBits );
void SetData( TArray<uint8>&& Src, int64 CountBits );

FORCEINLINE_DEBUGGABLE void SerializeBits( void* Dest, int64 LengthBits )
{
if ( IsError() || Pos+LengthBits > Num)
{
if (!IsError())
{
SetOverflowed(LengthBits);
}
FMemory::Memzero( Dest, (LengthBits+7)>>3 );
return;
}
if( LengthBits == 1 )
{
((uint8*)Dest)[0] = 0;
if( Buffer[(int32)(Pos>>3)] & Shift(Pos&7) )
((uint8*)Dest)[0] |= 0x01;
Pos++;
}
else if (LengthBits != 0)
{
((uint8*)Dest)[((LengthBits+7)>>3) - 1] = 0;
appBitsCpy((uint8*)Dest, 0, Buffer.GetData(), (int32)Pos, (int32)LengthBits);
Pos += LengthBits;
}
}

virtual void SerializeBitsWithOffset( void* Dest, int32 DestBit, int64 LengthBits ) override;

FORCEINLINE_DEBUGGABLE void SerializeInt(uint32& OutValue, uint32 ValueMax)
{
if (!IsError())
{
uint32 Value = 0;
int64 LocalPos = Pos;
const int64 LocalNum = Num;

for (uint32 Mask=1; (Value + Mask) < ValueMax && Mask; Mask *= 2, LocalPos++)
{
if (LocalPos >= LocalNum)
{
SetOverflowed(LocalPos - Pos);
break;
}
if (Buffer[(int32)(LocalPos >> 3)] & Shift(LocalPos & 7))
{
Value |= Mask;
}
}
Pos = LocalPos;
OutValue = Value;
}
}

virtual void SerializeIntPacked(uint32& Value) override;

FORCEINLINE_DEBUGGABLE uint32 ReadInt(uint32 Max)
{
uint32 Value = 0;
SerializeInt(Value, Max);
return Value;
}

FORCEINLINE_DEBUGGABLE uint8 ReadBit()
{
uint8 Bit=0;
if ( !IsError() )
{
int64 LocalPos = Pos;
const int64 LocalNum = Num;
if (LocalPos >= LocalNum)
{
SetOverflowed(1);
}
else
{
Bit = !!(Buffer[(int32)(LocalPos>>3)] & Shift(LocalPos&7));
Pos++;
}
}
return Bit;
}

FORCEINLINE_DEBUGGABLE void Serialize( void* Dest, int64 LengthBytes )
{
SerializeBits( Dest, LengthBytes*8 );
}

FORCEINLINE_DEBUGGABLE uint8* GetData() { return Buffer.GetData(); }
FORCEINLINE_DEBUGGABLE const uint8* GetData() const { return Buffer.GetData(); }
FORCEINLINE_DEBUGGABLE const TArray<uint8>& GetBuffer() const { return Buffer; }

FORCEINLINE_DEBUGGABLE uint8* GetDataPosChecked()
{
check(Pos % 8 == 0);
return &Buffer[(int32)(Pos >> 3)];
}

FORCEINLINE_DEBUGGABLE int64 GetBytesLeft() const { return ((Num - Pos) + 7) >> 3; }
FORCEINLINE_DEBUGGABLE int64 GetBitsLeft() const { return (Num - Pos); }
FORCEINLINE_DEBUGGABLE bool AtEnd() { return IsError() || Pos>=Num; }
FORCEINLINE_DEBUGGABLE int64 GetNumBytes() const { return (Num+7)>>3; }
FORCEINLINE_DEBUGGABLE int64 GetNumBits() const { return Num; }
FORCEINLINE_DEBUGGABLE int64 GetPosBits() const { return Pos; }

FORCEINLINE_DEBUGGABLE void EatByteAlign()
{
int64 PrePos = Pos;
Pos = (Pos+7) & (~0x07);
if ( Pos > Num )
{
SetOverflowed(Pos - PrePos);
}
}

void SetOverflowed(int64 LengthBits);
void SetAtEnd() { Pos = Num; }

void AppendDataFromChecked( FBitReader& Src );
void AppendDataFromChecked( uint8* Src, uint32 NumBits );
void AppendTo( TArray<uint8> &Buffer );

virtual void CountMemory(FArchive& Ar) const;

protected:
TArray<uint8> Buffer;
int64 Num;
int64 Pos;

private:
FORCEINLINE uint8 Shift(uint8 Cnt) { return (uint8)(1<<Cnt); }
};

struct CORE_API FBitReaderMark
{
public:
FBitReaderMark() : Pos(0) { }
FBitReaderMark( FBitReader& Reader ) : Pos(Reader.Pos) { }

FORCEINLINE_DEBUGGABLE int64 GetPos() const { return Pos; }
FORCEINLINE_DEBUGGABLE void Pop( FBitReader& Reader ) { Reader.Pos = Pos; }
void Copy( FBitReader& Reader, TArray<uint8> &Buffer );

private:
int64 Pos;
};
