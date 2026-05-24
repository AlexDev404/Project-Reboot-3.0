// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Net/Core/Public/Net/Core/Misc/ResizableCircularQueue.h
// Simplified for standalone compilation.

#pragma once

#include "UE4Net/CoreMinimal.h"

template<typename T, typename AllocatorT = void>
class TResizableCircularQueue
{
public:
typedef T ElementT;
typedef uint32 IndexT;

explicit TResizableCircularQueue(SIZE_T InitialCapacity = 0)
: Head(0u), Tail(0u), IndexMask(0u)
{
if (InitialCapacity > 0) SetCapacity(InitialCapacity);
}

~TResizableCircularQueue() { Reset(); Storage.Empty(); }

bool IsEmpty() const { return Head == Tail; }
SIZE_T Count() const { return Head - Tail; }
SIZE_T AllocatedCapacity() const { return Storage.Num(); }

void Enqueue(const ElementT& SrcData)
{
const SIZE_T RequiredCapacity = Count() + 1;
if (RequiredCapacity > AllocatedCapacity()) SetCapacity(RequiredCapacity);
const IndexT MaskedIndex = Head++ & IndexMask;
new (Storage.GetData() + MaskedIndex) T(SrcData);
}

void Pop() { if (ensure(Count() > 0)) PopNoCheck(); }

void PopNoCheck()
{
Storage.GetData()[Tail & IndexMask].~T();
++Tail;
}

void PopNoCheck(SIZE_T PopCount)
{
for (SIZE_T i = 0; i < PopCount; ++i) PopNoCheck();
}

const ElementT& PeekNoCheck() const { return Storage.GetData()[Tail & IndexMask]; }
const ElementT& PeekAtOffset(SIZE_T Offset) const { return Storage.GetData()[(Tail + Offset) & IndexMask]; }

void Reset() { while (!IsEmpty()) PopNoCheck(); Head = 0; Tail = 0; }
void Empty() { Reset(); IndexMask = 0; Storage.Empty(); }

private:
void SetCapacity(SIZE_T RequiredCapacity)
{
SIZE_T NewCapacity = static_cast<SIZE_T>(FMath::RoundUpToPowerOfTwo(static_cast<uint64>(RequiredCapacity)));
if (NewCapacity == static_cast<SIZE_T>(Storage.Num()) || NewCapacity < Count()) return;

if (Storage.Num() > 0)
{
std::vector<T> NewStorage(NewCapacity);
SIZE_T OldCount = Count();
for (SIZE_T i = 0; i < OldCount; ++i)
{
NewStorage[i] = Storage.GetData()[(Tail + i) & IndexMask];
}
// Move into our TArray-based storage
Storage.Empty();
Storage.AddUninitialized(static_cast<int32>(NewCapacity));
for (SIZE_T i = 0; i < OldCount; ++i)
{
new (Storage.GetData() + i) T(std::move(NewStorage[i]));
}
IndexMask = static_cast<IndexT>(NewCapacity - 1);
Tail = 0u;
Head = static_cast<IndexT>(OldCount);
}
else
{
IndexMask = static_cast<IndexT>(NewCapacity - 1);
Storage.AddUninitialized(static_cast<int32>(NewCapacity));
}
}

IndexT Head;
IndexT Tail;
IndexT IndexMask;
TArray<ElementT> Storage;
};
