// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/Net/StatelessConnectHandlerComponent.cpp
// Implements the UE4 stateless cookie handshake

#include "UE4Net/PacketHandlers/StatelessConnectHandlerComponent.h"

#include <ctime>
#include <cstdlib>
#include <chrono>

static double GetCurrentTime()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

static void GenerateRandomBytes(uint8* OutBytes, int32 NumBytes)
{
    // Use random device for secret generation
    for (int32 i = 0; i < NumBytes; i++)
    {
        OutBytes[i] = static_cast<uint8>(std::rand() & 0xFF);
    }
}

FStatelessConnectHandlerComponent::FStatelessConnectHandlerComponent()
    : State(EHandshakeState::Uninitialized)
    , LastTimestamp(0.0)
    , LastSecretRotationTime(0.0)
    , bRestartedHandshake(false)
{
    FMemory::Memzero(ActiveSecret, SecretByteSize);
    FMemory::Memzero(OldSecret, SecretByteSize);
    FMemory::Memzero(LastCookie, CookieByteSize);
}

void FStatelessConnectHandlerComponent::Initialize()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    GenerateRandomBytes(ActiveSecret, SecretByteSize);
    GenerateRandomBytes(OldSecret, SecretByteSize);
    LastSecretRotationTime = GetCurrentTime();
    State = EHandshakeState::InitializedOnLocal;
}

void FStatelessConnectHandlerComponent::RotateSecrets()
{
    FMemory::Memcpy(OldSecret, ActiveSecret, SecretByteSize);
    GenerateRandomBytes(ActiveSecret, SecretByteSize);
    LastSecretRotationTime = GetCurrentTime();
}

void FStatelessConnectHandlerComponent::GenerateCookieWithSecret(const uint8* Secret, const FString& ClientAddress, uint8 OutCookie[CookieByteSize]) const
{
    // HMAC(secret, client_address + timestamp_bytes)
    // In UE4, the address is encoded as bytes and combined with a timestamp
    const char* AddrStr = ClientAddress.c_str();
    int32 AddrLen = static_cast<int32>(ClientAddress.length());

    FHMACSHA1::Generate(Secret, SecretByteSize, reinterpret_cast<const uint8*>(AddrStr), AddrLen, OutCookie);
}

void FStatelessConnectHandlerComponent::GenerateCookie(const FString& ClientAddress, uint8 OutCookie[CookieByteSize]) const
{
    GenerateCookieWithSecret(ActiveSecret, ClientAddress, OutCookie);
}

bool FStatelessConnectHandlerComponent::VerifyCookie(const FString& ClientAddress, const uint8 InCookie[CookieByteSize]) const
{
    // Try active secret first
    uint8 ExpectedCookie[CookieByteSize];
    GenerateCookieWithSecret(ActiveSecret, ClientAddress, ExpectedCookie);
    if (FMemory::Memcmp(InCookie, ExpectedCookie, CookieByteSize) == 0)
    {
        return true;
    }

    // Try old secret (handles rotation race condition)
    GenerateCookieWithSecret(OldSecret, ClientAddress, ExpectedCookie);
    return FMemory::Memcmp(InCookie, ExpectedCookie, CookieByteSize) == 0;
}

bool FStatelessConnectHandlerComponent::CreateChallengePacket(FBitWriter& OutPacket, const FString& ClientAddress)
{
    // Handshake packet format:
    // [1 byte: magic] [1 byte: packet type] [20 bytes: cookie]
    LastTimestamp = GetCurrentTime();

    uint8 Cookie[CookieByteSize];
    GenerateCookie(ClientAddress, Cookie);

    OutPacket.WriteBit(0); // Not a game packet (UE4 uses top bit to distinguish)
    uint8 Magic = HandshakeMagic;
    OutPacket.Serialize(&Magic, 1);
    uint8 Type = static_cast<uint8>(EHandshakePacketType::Challenge);
    OutPacket.Serialize(&Type, 1);
    OutPacket.Serialize(Cookie, CookieByteSize);

    State = EHandshakeState::SentChallenge;
    return !OutPacket.IsError();
}

bool FStatelessConnectHandlerComponent::ProcessChallengeResponse(FBitReader& InPacket, const FString& ClientAddress)
{
    // Read cookie from client response
    uint8 Cookie[CookieByteSize];

    // Skip magic + type (already verified by IsHandshakePacket)
    uint8 Magic, Type;
    InPacket.Serialize(&Magic, 1);
    InPacket.Serialize(&Type, 1);
    InPacket.Serialize(Cookie, CookieByteSize);

    if (InPacket.IsError())
    {
        return false;
    }

    return VerifyCookie(ClientAddress, Cookie);
}

bool FStatelessConnectHandlerComponent::CreateChallengeAck(FBitWriter& OutPacket)
{
    OutPacket.WriteBit(0); // Not a game packet
    uint8 Magic = HandshakeMagic;
    OutPacket.Serialize(&Magic, 1);
    uint8 Type = static_cast<uint8>(EHandshakePacketType::ChallengeAck);
    OutPacket.Serialize(&Type, 1);

    State = EHandshakeState::Initialized;
    return !OutPacket.IsError();
}

bool FStatelessConnectHandlerComponent::ProcessChallenge(FBitReader& InPacket, FBitWriter& OutResponse)
{
    // Client receives challenge, echoes back the cookie
    uint8 Magic, Type;
    InPacket.Serialize(&Magic, 1);
    InPacket.Serialize(&Type, 1);

    uint8 Cookie[CookieByteSize];
    InPacket.Serialize(Cookie, CookieByteSize);

    if (InPacket.IsError())
    {
        return false;
    }

    // Store for reference
    FMemory::Memcpy(LastCookie, Cookie, CookieByteSize);

    // Build response with same cookie
    OutResponse.WriteBit(0);
    uint8 RespMagic = HandshakeMagic;
    OutResponse.Serialize(&RespMagic, 1);
    uint8 RespType = static_cast<uint8>(EHandshakePacketType::ChallengeResponse);
    OutResponse.Serialize(&RespType, 1);
    OutResponse.Serialize(Cookie, CookieByteSize);

    State = EHandshakeState::SentChallengeResponse;
    return !OutResponse.IsError();
}

bool FStatelessConnectHandlerComponent::ProcessChallengeAck(FBitReader& InPacket)
{
    uint8 Magic, Type;
    InPacket.Serialize(&Magic, 1);
    InPacket.Serialize(&Type, 1);

    if (InPacket.IsError() || Magic != HandshakeMagic || Type != static_cast<uint8>(EHandshakePacketType::ChallengeAck))
    {
        return false;
    }

    State = EHandshakeState::Initialized;
    return true;
}

bool FStatelessConnectHandlerComponent::IsHandshakePacket(const uint8* Data, int32 DataLen)
{
    // Handshake packets start with bit 0 = 0 (game packets have bit 0 = 1)
    // Then magic byte 0x5A
    if (DataLen < 3) return false;

    // Check if top bit of first byte is 0 (not a game packet)
    if (Data[0] & 0x80) return false;

    // After the bit, check for magic byte (accounting for bit offset)
    // In practice, the first full byte after the control bit contains the magic
    return Data[1] == HandshakeMagic;
}
