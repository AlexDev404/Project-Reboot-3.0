// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/Net/StatelessConnectHandlerComponent.cpp
// Implements the UE4 4.26 stateless cookie handshake (matches real Fortnite 17.50 client)

#include "UE4Net/PacketHandlers/StatelessConnectHandlerComponent.h"

#include <ctime>
#include <cstdlib>
#include <chrono>

// UE4 4.26 handshake packet sizes (in bits, after MagicHeader)
// Format: [HandshakeBit:1][RestartBit:1][SecretIdBit:1][Timestamp:64][Cookie:160] = 227 bits
// Plus 1 termination bit when sent = 228 bits = 29 bytes on the wire
static constexpr int32 HANDSHAKE_PACKET_SIZE_BITS = 227;
static constexpr int32 COOKIE_BYTE_SIZE = 20; // SHA1 digest size

// Secret update timing
static constexpr float SECRET_UPDATE_TIME = 15.f;
static constexpr float SECRET_UPDATE_TIME_VARIANCE = 5.f;
static constexpr float MAX_COOKIE_LIFETIME = (SECRET_UPDATE_TIME + SECRET_UPDATE_TIME_VARIANCE) * 2.f;

static double GetCurrentTime()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

static double GElapsedTime = 0.0;

static void GenerateRandomBytes(uint8* OutBytes, int32 NumBytes)
{
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
    GElapsedTime = 0.0;
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
    // Real UE4: Cookie = HMAC(Secret, Timestamp + ClientAddress)
    // We combine the timestamp bytes with the address string for the HMAC input
    TArray<uint8> CookieData;

    // Serialize timestamp
    double Timestamp = LastTimestamp;
    const uint8* TimestampBytes = reinterpret_cast<const uint8*>(&Timestamp);
    for (int32 i = 0; i < 8; i++)
    {
        CookieData.Add(TimestampBytes[i]);
    }

    // Serialize address
    const char* AddrStr = ClientAddress.c_str();
    int32 AddrLen = static_cast<int32>(ClientAddress.length());
    for (int32 i = 0; i < AddrLen; i++)
    {
        CookieData.Add(static_cast<uint8>(AddrStr[i]));
    }

    FHMACSHA1::Generate(Secret, SecretByteSize, CookieData.GetData(), CookieData.Num(), OutCookie);
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

// Helper: write Fortnite's 4-bit MagicHeader (0b0111 LSB-first) at the start of every packet
static void WriteFortniteMagicHeader(FBitWriter& Out)
{
    Out.WriteBit(1); Out.WriteBit(1); Out.WriteBit(1); Out.WriteBit(0);
}

// Helper: skip the 4-bit MagicHeader during reads
static void SkipFortniteMagicHeader(FBitReader& In)
{
    In.ReadBit(); In.ReadBit(); In.ReadBit(); In.ReadBit();
}

bool FStatelessConnectHandlerComponent::CreateChallengePacket(FBitWriter& OutPacket, const FString& ClientAddress)
{
    // Fortnite 17.50 challenge format (29 bytes / 232 bits):
    // [MagicHeader:4=0x7][HandshakeBit:1=1][RestartBit:1=0][SecretIdBit:1=1][Timestamp:64][Cookie:160][TermBit:1]

    GElapsedTime += 0.001;
    LastTimestamp = GElapsedTime;
    double Timestamp = LastTimestamp;

    uint8 Cookie[CookieByteSize];
    GenerateCookie(ClientAddress, Cookie);

    WriteFortniteMagicHeader(OutPacket);
    OutPacket.WriteBit(1);                          // HandshakeBit
    OutPacket.WriteBit(0);                          // RestartHandshakeBit
    OutPacket.WriteBit(1);                          // SecretIdBit (using active secret)
    OutPacket << Timestamp;                         // 64-bit double
    OutPacket.Serialize(Cookie, CookieByteSize);    // 20-byte cookie (SHA1)
    OutPacket.WriteBit(1);                          // Termination

    State = EHandshakeState::SentChallenge;
    return !OutPacket.IsError();
}

bool FStatelessConnectHandlerComponent::ProcessChallengeResponse(FBitReader& InPacket, const FString& ClientAddress)
{
    // Client response format (after MagicHeader + HandshakeBit already consumed by caller):
    // [RestartBit:1][SecretIdBit:1][Timestamp:64][Cookie:160]

    uint8 bRestartHandshake = InPacket.ReadBit();
    uint8 SecretId = InPacket.ReadBit();

    double Timestamp = 0.0;
    InPacket << Timestamp;

    uint8 Cookie[CookieByteSize];
    InPacket.Serialize(Cookie, CookieByteSize);

    if (InPacket.IsError())
    {
        return false;
    }

    // This is a challenge response (Timestamp != 0)
    if (Timestamp == 0.0)
    {
        return false; // This is an initial connect, not a response
    }

    // Verify: regenerate cookie with the timestamp from the packet
    double SavedTimestamp = LastTimestamp;
    LastTimestamp = Timestamp;

    uint8 RegenCookie[CookieByteSize];
    GenerateCookieWithSecret(ActiveSecret, ClientAddress, RegenCookie);

    bool bSuccess = FMemory::Memcmp(Cookie, RegenCookie, CookieByteSize) == 0;

    if (!bSuccess)
    {
        // Try old secret
        GenerateCookieWithSecret(OldSecret, ClientAddress, RegenCookie);
        bSuccess = FMemory::Memcmp(Cookie, RegenCookie, CookieByteSize) == 0;
    }

    if (!bSuccess)
    {
        LastTimestamp = SavedTimestamp;
    }
    else
    {
        // Store the verified cookie for the ack — the client expects to receive back
        // the exact same cookie it just sent.
        FMemory::Memcpy(LastCookie, Cookie, CookieByteSize);
    }

    return bSuccess;
}

bool FStatelessConnectHandlerComponent::CreateChallengeAck(FBitWriter& OutPacket)
{
    // Ack: [Magic:4][HandshakeBit:1=1][RestartBit:1=0][SecretIdBit:1=1][Timestamp:64=-1.0][Cookie:160][TermBit:1]

    double Timestamp = -1.0;
    uint8 Cookie[CookieByteSize];
    FMemory::Memcpy(Cookie, LastCookie, CookieByteSize);

    WriteFortniteMagicHeader(OutPacket);
    OutPacket.WriteBit(1); // HandshakeBit
    OutPacket.WriteBit(0); // RestartBit
    OutPacket.WriteBit(1); // SecretIdBit
    OutPacket << Timestamp;
    OutPacket.Serialize(Cookie, CookieByteSize);
    OutPacket.WriteBit(1); // Termination

    State = EHandshakeState::Initialized;
    return !OutPacket.IsError();
}

bool FStatelessConnectHandlerComponent::ProcessChallenge(FBitReader& InPacket, FBitWriter& OutResponse)
{
    // Client side: receives [RestartBit][SecretId][Timestamp][Cookie] (after Magic+HandshakeBit consumed)

    uint8 bRestartHandshake = InPacket.ReadBit();
    uint8 SecretId = InPacket.ReadBit();

    double Timestamp = 0.0;
    InPacket << Timestamp;

    uint8 Cookie[CookieByteSize];
    InPacket.Serialize(Cookie, CookieByteSize);

    if (InPacket.IsError() || Timestamp <= 0.0)
    {
        return false;
    }

    FMemory::Memcpy(LastCookie, Cookie, CookieByteSize);

    WriteFortniteMagicHeader(OutResponse);
    OutResponse.WriteBit(1); // HandshakeBit
    OutResponse.WriteBit(0); // RestartBit
    OutResponse.WriteBit(SecretId);
    OutResponse << Timestamp;
    OutResponse.Serialize(Cookie, CookieByteSize);
    OutResponse.WriteBit(1); // Termination

    State = EHandshakeState::SentChallengeResponse;
    return !OutResponse.IsError();
}

bool FStatelessConnectHandlerComponent::ProcessChallengeAck(FBitReader& InPacket)
{
    // Ack from server (after Magic + HandshakeBit consumed by caller):
    // [RestartBit:1][SecretIdBit:1][Timestamp:64=-1.0][Cookie:160]

    uint8 bRestartHandshake = InPacket.ReadBit();
    uint8 SecretId = InPacket.ReadBit();

    double Timestamp = 0.0;
    InPacket << Timestamp;

    if (InPacket.IsError() || Timestamp >= 0.0)
    {
        return false; // Ack has negative timestamp
    }

    State = EHandshakeState::Initialized;
    return true;
}

void FStatelessConnectHandlerComponent::GetChallengeSequenceList(uint16& OutServerSequence, uint16& OutClientSequence) const
{
    // Real UE4 4.26 derives both initial sequence numbers from the authorized
    // cookie so the two peers agree without any extra round-trip. The 14-bit
    // sequence space matches FNetPacketNotify::SequenceNumberBits.
    static constexpr uint16 SequenceMask = (1u << 14) - 1u; // 0x3FFF
    const uint16 RawServer = static_cast<uint16>(LastCookie[0]) | (static_cast<uint16>(LastCookie[1]) << 8);
    const uint16 RawClient = static_cast<uint16>(LastCookie[2]) | (static_cast<uint16>(LastCookie[3]) << 8);
    OutServerSequence = RawServer & SequenceMask;
    OutClientSequence = RawClient & SequenceMask;
}

bool FStatelessConnectHandlerComponent::IsHandshakePacket(const uint8* Data, int32 DataLen)
{
    // Fortnite 17.50 prepends 4-bit MagicHeader (0b0111 LSB-first = bits 0,1,2 set, bit 3 clear)
    // followed by the HandshakeBit (bit 4). Mask = 0x17 (bits 0,1,2,4), expected = 0x17.
    if (DataLen < 1) return false;
    return (Data[0] & 0x17) == 0x17;
}
