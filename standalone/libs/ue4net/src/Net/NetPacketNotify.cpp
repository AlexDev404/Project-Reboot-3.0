// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Core/Private/Net/NetPacketNotify.cpp

#include "UE4Net/Net/NetPacketNotify.h"
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"

// Bit layout per real UE4 4.27 source (FPackedHeader):
//   bits  0-3:  HistoryWordCount
//   bits  4-17: AckedSeq (14 bits)
//   bits 18-31: Seq (14 bits)
namespace
{
    enum { HistoryWordCountBits = 4 };
    enum { SeqMask = (1u << FNetPacketNotify::SequenceNumberBits) - 1u };
    enum { HistoryWordCountMask = (1u << HistoryWordCountBits) - 1u };
    enum { AckSeqShift = HistoryWordCountBits };
    enum { SeqShift = AckSeqShift + FNetPacketNotify::SequenceNumberBits };

    static uint32 PackHeader(FNetPacketNotify::SequenceNumberT Seq,
                             FNetPacketNotify::SequenceNumberT AckedSeq,
                             SIZE_T HistoryWordCount)
    {
        uint32 Packed = 0u;
        Packed |= Seq.Get() << SeqShift;
        Packed |= AckedSeq.Get() << AckSeqShift;
        Packed |= HistoryWordCount & HistoryWordCountMask;
        return Packed;
    }
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
// Real UE4: OutAckSeq is initialized to InitialOutSeq - 1 so the first
// CommitAndIncrementOutSeq produces seq == InitialOutSeq with a valid history window.
OutAckSeq = SequenceNumberT(static_cast<SequenceNumberT::SequenceT>(InitialOutSeq.Get() - 1));
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
SIZE_T HistoryWordCount = FMath::Clamp<SIZE_T>(
    (GetCurrentSequenceHistoryLength() + SequenceHistoryT::BitsPerWord - 1u) / SequenceHistoryT::BitsPerWord,
    1u, SequenceHistoryT::WordCount);

WrittenHistoryWordCount = bRefresh ? WrittenHistoryWordCount : HistoryWordCount;
WrittenInAckSeq = InAckSeq;

// Real UE4 packs HistoryWordCount-1 (so 1 word == 0 in the field, 16 words == 15)
uint32 PackedHeader = PackHeader(OutSeq, InAckSeq, WrittenHistoryWordCount - 1);

// Must match real UE4 4.26: uses stream operator (raw 32-bit LE bytes),
// NOT SerializeInt(which uses a different variable-length bit encoding).
Writer << PackedHeader;
InSeqHistory.Write(Writer, WrittenHistoryWordCount);

return !Writer.IsError();
}

bool FNetPacketNotify::ReadHeader(FNotificationHeader& Data, FBitReader& Reader) const
{
uint32 PackedHeader = 0u;
int64 PrePos = Reader.GetPosBits();
Reader << PackedHeader;

Data.Seq = SequenceNumberT((PackedHeader >> SeqShift) & SeqMask);
Data.AckedSeq = SequenceNumberT((PackedHeader >> AckSeqShift) & SeqMask);
Data.HistoryWordCount = (PackedHeader & HistoryWordCountMask) + 1;

std::fprintf(stderr, "[ue4net]   ReadHeader: PackedHeader=0x%08X Seq=%u AckedSeq=%u HWC=%u "
    "(bitsAtEntry=%lld bitsAfterPH=%lld)\n",
    (unsigned)PackedHeader, (unsigned)Data.Seq.Get(), (unsigned)Data.AckedSeq.Get(),
    (unsigned)Data.HistoryWordCount,
    (long long)PrePos, (long long)Reader.GetPosBits());

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
// Real UE4 uses `AckedSeq > InAckSeq` (not !=) so wraparound or AckedSeq<InAckSeq
// won't loop forever / underflow the history.
while (AckedSeq > InAckSeq)
{
++InAckSeq;
InSeqHistory.AddDeliveryStatus(InAckSeq == AckedSeq ? IsAck : false);
}
}

SIZE_T FNetPacketNotify::GetCurrentSequenceHistoryLength() const
{
// Real UE4 measures the gap between InAckSeq (what we've acked) and
// InAckSeqAck (what the remote has confirmed seeing our acks for).
if (InAckSeq >= InAckSeqAck)
{
    SequenceNumberT::DifferenceT Diff = SequenceNumberT::Diff(InAckSeq, InAckSeqAck);
    return static_cast<SIZE_T>(FMath::Min(Diff, (SequenceNumberT::DifferenceT)SequenceHistoryT::Size));
}
// Worst case: history wraps; send full
return SequenceHistoryT::Size;
}
