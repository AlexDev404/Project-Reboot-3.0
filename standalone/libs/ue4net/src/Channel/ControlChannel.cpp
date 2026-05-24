// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/DataChannel.cpp
// UControlChannel implementation

#include "UE4Net/Channel/ControlChannel.h"
#include "UE4Net/Channel/NetConnection.h"

UControlChannel::UControlChannel()
{
}

UControlChannel::~UControlChannel()
{
}

void UControlChannel::Init(UNetConnection* InConnection, int32 InChIndex, EChannelType InChType)
{
    UChannel::Init(InConnection, InChIndex, EChannelType::CHTYPE_Control);
    bOpened = true; // Control channel is always open
    OpenedLocally = 1;
}

void UControlChannel::ReceivedBunch(FInBunch& Bunch)
{
    // Read control message type
    uint8 MessageType = 0;
    Bunch.Serialize(&MessageType, 1);
    if (Bunch.IsError()) return;

    // Dispatch to handler
    if (OnControlMessage)
    {
        OnControlMessage(static_cast<ENMTType>(MessageType), Bunch);
    }
}

void UControlChannel::Tick()
{
    // Flush queued messages
    for (auto& Msg : QueuedMessages)
    {
        FOutBunch Bunch(this, false);
        Bunch.bReliable = true;
        uint8 Type = static_cast<uint8>(Msg.Type);
        Bunch.Serialize(&Type, 1);
        if (Msg.Data.Num() > 0)
        {
            Bunch.Serialize(Msg.Data.GetData(), Msg.Data.Num());
        }
        SendBunch(&Bunch, false);
    }
    QueuedMessages.Empty();
}

bool UControlChannel::SendControlMessage(ENMTType MessageType, FBitWriter& MessageData)
{
    FOutBunch Bunch(this, false);
    Bunch.bReliable = true;

    uint8 Type = static_cast<uint8>(MessageType);
    Bunch.Serialize(&Type, 1);

    if (MessageData.GetNumBytes() > 0)
    {
        Bunch.SerializeBits(MessageData.GetData(), MessageData.GetNumBits());
    }

    return SendBunch(&Bunch, false) > 0;
}

bool UControlChannel::SendHello(int32 ProtocolVersion, bool bEncrypted, const FString& EncryptionToken)
{
    FBitWriter Writer(256, true);

    // NMT_Hello format: [Version:int32] [bEncrypted:bit] [EncryptionToken:FString]
    Writer.Serialize(&ProtocolVersion, sizeof(int32));
    Writer.WriteBit(bEncrypted ? 1 : 0);

    // Write FString (UE4 format: length + chars)
    int32 Len = static_cast<int32>(EncryptionToken.length());
    Writer.Serialize(&Len, sizeof(int32));
    if (Len > 0)
    {
        Writer.Serialize(const_cast<char*>(EncryptionToken.c_str()), Len);
    }

    return SendControlMessage(ENMTType::NMT_Hello, Writer);
}

bool UControlChannel::SendChallenge(const FString& ChallengeString)
{
    FBitWriter Writer(256, true);
    int32 Len = static_cast<int32>(ChallengeString.length());
    Writer.Serialize(&Len, sizeof(int32));
    if (Len > 0)
    {
        Writer.Serialize(const_cast<char*>(ChallengeString.c_str()), Len);
    }
    return SendControlMessage(ENMTType::NMT_Challenge, Writer);
}

bool UControlChannel::SendLogin(const FString& URL, const FString& UniqueId, const FString& OnlinePlatformName)
{
    FBitWriter Writer(1024, true);

    // Write URL
    int32 URLLen = static_cast<int32>(URL.length());
    Writer.Serialize(&URLLen, sizeof(int32));
    if (URLLen > 0) Writer.Serialize(const_cast<char*>(URL.c_str()), URLLen);

    // Write UniqueId
    int32 IdLen = static_cast<int32>(UniqueId.length());
    Writer.Serialize(&IdLen, sizeof(int32));
    if (IdLen > 0) Writer.Serialize(const_cast<char*>(UniqueId.c_str()), IdLen);

    // Write Platform
    int32 PlatLen = static_cast<int32>(OnlinePlatformName.length());
    Writer.Serialize(&PlatLen, sizeof(int32));
    if (PlatLen > 0) Writer.Serialize(const_cast<char*>(OnlinePlatformName.c_str()), PlatLen);

    return SendControlMessage(ENMTType::NMT_Login, Writer);
}

bool UControlChannel::SendWelcome(const FString& MapName, const FString& GameName, const FString& URL)
{
    FBitWriter Writer(1024, true);

    int32 MapLen = static_cast<int32>(MapName.length());
    Writer.Serialize(&MapLen, sizeof(int32));
    if (MapLen > 0) Writer.Serialize(const_cast<char*>(MapName.c_str()), MapLen);

    int32 GameLen = static_cast<int32>(GameName.length());
    Writer.Serialize(&GameLen, sizeof(int32));
    if (GameLen > 0) Writer.Serialize(const_cast<char*>(GameName.c_str()), GameLen);

    int32 URLLen = static_cast<int32>(URL.length());
    Writer.Serialize(&URLLen, sizeof(int32));
    if (URLLen > 0) Writer.Serialize(const_cast<char*>(URL.c_str()), URLLen);

    return SendControlMessage(ENMTType::NMT_Welcome, Writer);
}

bool UControlChannel::SendNetspeed(int32 Rate)
{
    FBitWriter Writer(32, true);
    Writer.Serialize(&Rate, sizeof(int32));
    return SendControlMessage(ENMTType::NMT_Netspeed, Writer);
}

bool UControlChannel::SendFailure(const FString& Reason)
{
    FBitWriter Writer(512, true);
    int32 Len = static_cast<int32>(Reason.length());
    Writer.Serialize(&Len, sizeof(int32));
    if (Len > 0) Writer.Serialize(const_cast<char*>(Reason.c_str()), Len);
    return SendControlMessage(ENMTType::NMT_Failure, Writer);
}
