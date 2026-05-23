// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Public/Net/Util/SequenceNumber.h

#pragma once

#include "UE4Net/CoreMinimal.h"

template <SIZE_T NumBits, typename SequenceType>
class TSequenceNumber
{
static_assert(TIsSigned<SequenceType>::Value == false, "The base type for sequence numbers must be unsigned");

public:
typedef SequenceType SequenceT;
typedef int32 DifferenceT;

enum { SeqNumberBits = NumBits };
enum { SeqNumberCount = SequenceT(1) << NumBits };
enum { SeqNumberHalf = SequenceT(1) << (NumBits - 1) };
enum { SeqNumberMax = SeqNumberCount - 1u };
enum { SeqNumberMask = SeqNumberMax };

TSequenceNumber() : Value(0u) {}
TSequenceNumber(SequenceT ValueIn) : Value(ValueIn & SeqNumberMask) {}

SequenceT Get() const { return Value; }

static DifferenceT Diff(TSequenceNumber A, TSequenceNumber B);

bool operator>(const TSequenceNumber& Other) const { return (Value != Other.Value) && (((Value - Other.Value) & SeqNumberMask) < SeqNumberHalf); }
bool operator>=(const TSequenceNumber& Other) const { return ((Value - Other.Value) & SeqNumberMask) < SeqNumberHalf; }
bool operator==(const TSequenceNumber& Other) const { return Value == Other.Value; }
bool operator!=(const TSequenceNumber& Other) const { return Value != Other.Value; }

TSequenceNumber& operator++() { Increment(1u); return *this; }
TSequenceNumber operator++(int) { TSequenceNumber Tmp(*this); Increment(1u); return Tmp; }

private:
void Increment(SequenceT InValue) { *this = TSequenceNumber(Value + InValue); }
SequenceT Value;
};

template <SIZE_T NumBits, typename SequenceType>
typename TSequenceNumber<NumBits, SequenceType>::DifferenceT TSequenceNumber<NumBits, SequenceType>::Diff(TSequenceNumber A, TSequenceNumber B)
{
constexpr SIZE_T ShiftValue = sizeof(DifferenceT)*8 - NumBits;

const SequenceT ValueA = A.Value;
const SequenceT ValueB = B.Value;

return (DifferenceT)((ValueA - ValueB) << ShiftValue) >> ShiftValue;
}
