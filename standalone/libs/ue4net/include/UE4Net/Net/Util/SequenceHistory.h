// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Public/Net/Util/SequenceHistory.h

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Serialization/BitWriter.h"
#include "UE4Net/Serialization/BitReader.h"

template <SIZE_T HistorySize>
class TSequenceHistory
{
public:
typedef uint32 WordT;

static constexpr SIZE_T BitsPerWord = sizeof(WordT) * 8;
static constexpr SIZE_T WordCount = HistorySize / BitsPerWord;
static constexpr SIZE_T MaxSizeInBits = WordCount * BitsPerWord;
static constexpr SIZE_T Size = HistorySize;

static_assert(HistorySize > 0, "HistorySize must be > 0");
static_assert(HistorySize % BitsPerWord == 0, "InMaxHistorySize must be a modulo of the wordsize");

public:
TSequenceHistory() { Reset(); }

void Reset() { FPlatformMemory::Memset(&Storage[0], 0, WordCount * sizeof(WordT)); }

void AddDeliveryStatus(bool Delivered)
{
WordT Carry = Delivered ? 1u : 0u;
const WordT ValueMask = 1u << (BitsPerWord - 1);

for (SIZE_T CurrentWordIt = 0; CurrentWordIt < WordCount; ++CurrentWordIt)
{
const WordT OldValue = Carry;
Carry = (Storage[CurrentWordIt] & ValueMask) >> (BitsPerWord - 1);
Storage[CurrentWordIt] = (Storage[CurrentWordIt] << 1u) | OldValue;
}
}

bool IsDelivered(SIZE_T Index) const
{
check(Index < Size);
const SIZE_T WordIndex = Index / BitsPerWord;
const WordT WordMask = (WordT(1) << (Index & (BitsPerWord - 1)));
return (Storage[WordIndex] & WordMask) != 0u;
}

bool operator==(const TSequenceHistory& Other) const { return FMemory::Memcmp(Storage, Other.Storage, WordCount * sizeof(WordT)) == 0; }
bool operator!=(const TSequenceHistory& Other) const { return FMemory::Memcmp(Storage, Other.Storage, WordCount * sizeof(WordT)) != 0; }

void Write(FBitWriter& Writer, SIZE_T NumWords) const
{
NumWords = FPlatformMath::Min(NumWords, WordCount);
for (SIZE_T CurrentWordIt = 0; CurrentWordIt < NumWords; ++CurrentWordIt)
{
WordT temp = Storage[CurrentWordIt];
Writer << temp;
}
}

void Read(FBitReader& Reader, SIZE_T NumWords)
{
NumWords = FPlatformMath::Min(NumWords, WordCount);
for (SIZE_T CurrentWordIt = 0; CurrentWordIt < NumWords; ++CurrentWordIt)
{
Reader << Storage[CurrentWordIt];
}
}

private:
WordT Storage[WordCount];
};
