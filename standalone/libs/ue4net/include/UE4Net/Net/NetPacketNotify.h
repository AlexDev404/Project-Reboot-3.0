// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Public/Net/NetPacketNotify.h

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Net/Core/Misc/ResizableCircularQueue.h"
#include "UE4Net/Net/Util/SequenceNumber.h"
#include "UE4Net/Net/Util/SequenceHistory.h"

#define UE_LOG_PACKET_NOTIFY(...)
#define UE_LOG_PACKET_NOTIFY_WARNING(Format, ...)

struct FBitWriter;
struct FBitReader;

class FNetPacketNotify
{
public:
enum { SequenceNumberBits = 14 };
enum { MaxSequenceHistoryLength = 256 };

typedef TSequenceNumber<SequenceNumberBits, uint16> SequenceNumberT;
typedef TSequenceHistory<MaxSequenceHistoryLength> SequenceHistoryT;

struct FNotificationHeader
{
SequenceHistoryT History;
SIZE_T HistoryWordCount;
SequenceNumberT Seq;
SequenceNumberT AckedSeq;
};

FNetPacketNotify();

void Init(SequenceNumberT InitialInSeq, SequenceNumberT InitialOutSeq);

void AckSeq(SequenceNumberT Seq) { AckSeq(Seq, true); }
void NakSeq(SequenceNumberT Seq) { AckSeq(Seq, false); }

SequenceNumberT CommitAndIncrementOutSeq();

bool WriteHeader(FBitWriter& Writer, bool bRefresh = false);
bool ReadHeader(FNotificationHeader& Data, FBitReader& Reader) const;

SequenceNumberT::DifferenceT GetSequenceDelta(const FNotificationHeader& NotificationData)
{
if (NotificationData.Seq > InSeq && NotificationData.AckedSeq >= OutAckSeq && OutSeq > NotificationData.AckedSeq)
{
return SequenceNumberT::Diff(NotificationData.Seq, InSeq);
}
return 0;
}

template<class Functor>
SequenceNumberT::DifferenceT Update(const FNotificationHeader& NotificationData, Functor&& InFunc);

const SequenceHistoryT& GetInSeqHistory() const { return InSeqHistory; }
SequenceNumberT GetInSeq() const { return InSeq; }
SequenceNumberT GetInAckSeq() const { return InAckSeq; }
SequenceNumberT GetOutSeq() const { return OutSeq; }
SequenceNumberT GetOutAckSeq() const { return OutAckSeq; }
bool CanSend() const { SequenceNumberT NextOutSeq = OutSeq; ++NextOutSeq; return NextOutSeq >= OutAckSeq; }
SIZE_T GetCurrentSequenceHistoryLength() const;

private:
struct FSentAckData
{
SequenceNumberT OutSeq;
SequenceNumberT InAckSeq;
};
typedef TResizableCircularQueue<FSentAckData, TInlineAllocator<128>> AckRecordT;

AckRecordT AckRecord;
SIZE_T WrittenHistoryWordCount;
SequenceNumberT WrittenInAckSeq;

SequenceHistoryT InSeqHistory;
SequenceNumberT InSeq;
SequenceNumberT InAckSeq;
SequenceNumberT InAckSeqAck;

SequenceNumberT OutSeq;
SequenceNumberT OutAckSeq;

private:
SequenceNumberT UpdateInAckSeqAck(SequenceNumberT::DifferenceT AckCount, SequenceNumberT AckedSeq);

template<class Functor>
inline void ProcessReceivedAcks(const FNotificationHeader& NotificationData, Functor&& InFunc);
void AckSeq(SequenceNumberT AckedSeq, bool IsAck);
};

// Template implementations
template<class Functor>
FNetPacketNotify::SequenceNumberT::DifferenceT FNetPacketNotify::Update(const FNotificationHeader& NotificationData, Functor&& InFunc)
{
const SequenceNumberT::DifferenceT InSeqDelta = GetSequenceDelta(NotificationData);
if (InSeqDelta > 0)
{
ProcessReceivedAcks(NotificationData, InFunc);
InSeq = NotificationData.Seq;
return InSeqDelta;
}
return 0;
}

template<class Functor>
void FNetPacketNotify::ProcessReceivedAcks(const FNotificationHeader& NotificationData, Functor&& InFunc)
{
if (NotificationData.AckedSeq > OutAckSeq)
{
SequenceNumberT::DifferenceT AckCount = SequenceNumberT::Diff(NotificationData.AckedSeq, OutAckSeq);
InAckSeqAck = UpdateInAckSeqAck(AckCount, NotificationData.AckedSeq);

SequenceNumberT CurrentAck(OutAckSeq);
++CurrentAck;

while (AckCount > (SequenceNumberT::DifferenceT)(SequenceHistoryT::Size))
{
--AckCount;
InFunc(CurrentAck, false);
++CurrentAck;
}

while (AckCount > 0)
{
--AckCount;
InFunc(CurrentAck, NotificationData.History.IsDelivered(AckCount));
++CurrentAck;
}
OutAckSeq = NotificationData.AckedSeq;
}
}
