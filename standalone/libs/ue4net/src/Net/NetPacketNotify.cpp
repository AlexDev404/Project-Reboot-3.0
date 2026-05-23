// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Core/Private/Net/NetPacketNotify.cpp

#include "UE4Net/Net/NetPacketNotify.h"
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"

enum { HistoryWordCountBits = FMath::CeilLogTwoHelper<FNetPacketNotify::MaxSequenceHistoryLength / FNetPacketNotify::SequenceHistoryT::BitsPerWord>::Value + 1 };
enum { SeqMask = (1 << FNetPacketNotify::SequenceNumberBits) - 1 };

static SIZE_T GetHistoryWordCount(const FNetPacketNotify::FNotificationHeader& Data)
{
return Data.HistoryWordCount;
}

FNetPacketNotify::FNetPacketNotify()
: AckRecord(64)
, WrittenHistoryWordCount(0)
, WrittenInAckSeq(0)
, InSeq(0)
, InAckSeq(0)
, InAckSeqAck(0)
, OutSeq(0)
, OutAckSeq(0)
{
}

void FNetPacketNotify::Init(SequenceNumberT InitialInSeq, SequenceNumberT InitialOutSeq)
{
InSeqHistory.Reset();
InSeq = InitialInSeq;
InAckSeq = InitialInSeq;
InAckSeqAck = InitialInSeq;
OutSeq = InitialOutSeq;
OutAckSeq = InitialOutSeq;
}

FNetPacketNotify::SequenceNumberT FNetPacketNotify::CommitAndIncrementOutSeq()
{
FSentAckData AckData;
AckData.OutSeq = OutSeq;
AckData.InAckSeq = InAckSeq;
AckRecord.Enqueue(AckData);
++OutSeq;
return AckData.OutSeq;
}

bool FNetPacketNotify::WriteHeader(FBitWriter& Writer, bool bRefresh)
{
SIZE_T HistoryWordCount = GetCurrentSequenceHistoryLength() / SequenceHistoryT::BitsPerWord;
HistoryWordCount = FMath::Clamp<SIZE_T>(HistoryWordCount, 1, SequenceHistoryT::WordCount);

WrittenHistoryWordCount = HistoryWordCount;
WrittenInAckSeq = InAckSeq;

// Write header
uint32 PackedHeader = 0u;
PackedHeader |= OutSeq.Get() & SeqMask;
PackedHeader |= (InAckSeq.Get() & SeqMask) << SequenceNumberBits;
PackedHeader |= (static_cast<uint32>(HistoryWordCount - 1)) << (SequenceNumberBits * 2);

Writer.SerializeInt(PackedHeader, (1u << (SequenceNumberBits * 2 + HistoryWordCountBits)));
InSeqHistory.Write(Writer, HistoryWordCount);

return !Writer.IsError();
}

bool FNetPacketNotify::ReadHeader(FNotificationHeader& Data, FBitReader& Reader) const
{
uint32 PackedHeader = 0u;
Reader.SerializeInt(PackedHeader, (1u << (SequenceNumberBits * 2 + HistoryWordCountBits)));

Data.Seq = SequenceNumberT(PackedHeader & SeqMask);
Data.AckedSeq = SequenceNumberT((PackedHeader >> SequenceNumberBits) & SeqMask);
Data.HistoryWordCount = ((PackedHeader >> (SequenceNumberBits * 2)) & ((1u << HistoryWordCountBits) - 1u)) + 1;

Data.History.Read(Reader, Data.HistoryWordCount);

return !Reader.IsError();
}

FNetPacketNotify::SequenceNumberT FNetPacketNotify::UpdateInAckSeqAck(SequenceNumberT::DifferenceT AckCount, SequenceNumberT AckedSeq)
{
SequenceNumberT NewInAckSeqAck = InAckSeqAck;

while (!AckRecord.IsEmpty())
{
const FSentAckData& EarliestAck = AckRecord.PeekNoCheck();
if (EarliestAck.OutSeq > AckedSeq)
{
break;
}
NewInAckSeqAck = EarliestAck.InAckSeq;
AckRecord.PopNoCheck();
}

return NewInAckSeqAck;
}

void FNetPacketNotify::AckSeq(SequenceNumberT AckedSeq, bool IsAck)
{
while (InAckSeq != AckedSeq)
{
++InAckSeq;
InSeqHistory.AddDeliveryStatus(InAckSeq == AckedSeq ? IsAck : false);
}
}

SIZE_T FNetPacketNotify::GetCurrentSequenceHistoryLength() const
{
SequenceNumberT::DifferenceT Diff = SequenceNumberT::Diff(OutSeq, OutAckSeq);
if (Diff <= 0) return 0;
return static_cast<SIZE_T>(FMath::Min((SequenceNumberT::DifferenceT)MaxSequenceHistoryLength, Diff));
}
