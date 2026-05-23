// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Public/Net/NetPacketNotify.h
// and PacketHandlers/StatelessConnectHandlerComponent

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"
#include "UE4Net/Crypto/SHA1.h"

// UE4's handshake state machine
enum class EHandshakeState : uint8
{
    Uninitialized,
    InitializedOnLocal,
    SentChallenge,
    SentChallengeResponse,
    SentChallengeAck,
    Initialized
};

// The handshake packet types (first byte of packet)
enum class EHandshakePacketType : uint8
{
    InitialPacket = 0,
    Challenge = 1,
    ChallengeResponse = 2,
    ChallengeAck = 3
};

/**
 * StatelessConnectHandlerComponent
 * Implements UE4's stateless challenge/response handshake that occurs
 * BEFORE NMT_Hello/Challenge/Login/Welcome messages.
 *
 * The cookie prevents connection spoofing by requiring the client to
 * echo back a server-generated HMAC that encodes the client's address.
 */
class ENGINE_API FStatelessConnectHandlerComponent
{
public:
    static constexpr int32 SecretByteSize = 64;
    static constexpr int32 CookieByteSize = FSHA1::DigestSize;  // 20 bytes
    static constexpr uint8 HandshakeMagic = 0x5A;  // Magic byte identifying handshake packets

    FStatelessConnectHandlerComponent();

    // Initialize with random secrets
    void Initialize();

    // Server: Generate a cookie for a given client address
    void GenerateCookie(const FString& ClientAddress, uint8 OutCookie[CookieByteSize]) const;

    // Server: Verify a cookie from a client
    bool VerifyCookie(const FString& ClientAddress, const uint8 InCookie[CookieByteSize]) const;

    // Server: Create challenge packet to send to client
    bool CreateChallengePacket(FBitWriter& OutPacket, const FString& ClientAddress);

    // Server: Process a challenge response from client, returns true if valid
    bool ProcessChallengeResponse(FBitReader& InPacket, const FString& ClientAddress);

    // Server: Create challenge ack packet (final handshake step before NMT_Hello)
    bool CreateChallengeAck(FBitWriter& OutPacket);

    // Client: Process a challenge from server, fill response
    bool ProcessChallenge(FBitReader& InPacket, FBitWriter& OutResponse);

    // Client: Process challenge ack from server
    bool ProcessChallengeAck(FBitReader& InPacket);

    // Utility: Check if a raw packet is a handshake packet (vs game data)
    static bool IsHandshakePacket(const uint8* Data, int32 DataLen);

    // Get current state
    EHandshakeState GetState() const { return State; }
    bool IsHandshakeComplete() const { return State == EHandshakeState::Initialized; }

    // Rotate secrets periodically (server should call this every ~15 seconds)
    void RotateSecrets();

    // Get the timestamp used in cookie
    double GetTimestamp() const { return LastTimestamp; }

private:
    // Server secrets for HMAC (double-buffered for rotation)
    uint8 ActiveSecret[SecretByteSize];
    uint8 OldSecret[SecretByteSize];

    // Client cookie storage
    uint8 LastCookie[CookieByteSize];

    // State
    EHandshakeState State;
    double LastTimestamp;
    double LastSecretRotationTime;
    bool bRestartedHandshake;

    // Generate cookie with specific secret
    void GenerateCookieWithSecret(const uint8* Secret, const FString& ClientAddress, uint8 OutCookie[CookieByteSize]) const;
};
