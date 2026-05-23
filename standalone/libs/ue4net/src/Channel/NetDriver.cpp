// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/NetDriver.cpp
// UNetDriver implementation

#include "UE4Net/Channel/NetDriver.h"

#include <chrono>

static double GetTime()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

UNetDriver::UNetDriver()
    : ServerConnection(nullptr)
    , MaxClientRate(15000)
    , NetServerMaxTickRate(30)
    , ElapsedTime(0.0f)
    , InBytesPerSecond(0)
    , OutBytesPerSecond(0)
    , Socket(nullptr)
    , ListenPort(0)
{
}

UNetDriver::~UNetDriver()
{
    Shutdown();
}

bool UNetDriver::Init(const FString& ListenURL, int32 Port)
{
    ListenPort = Port;
    NetDriverName = "UE4NetDriver";
    return true;
}

void UNetDriver::Shutdown()
{
    // Close all client connections
    for (auto* Conn : ClientConnections)
    {
        if (Conn)
        {
            Conn->Close();
            delete Conn;
        }
    }
    ClientConnections.Empty();

    // Close server connection
    if (ServerConnection)
    {
        ServerConnection->Close();
        delete ServerConnection;
        ServerConnection = nullptr;
    }
}

void UNetDriver::TickDispatch(float DeltaTime)
{
    ElapsedTime += DeltaTime;

    // In a full implementation, this would:
    // 1. Poll the socket for incoming data
    // 2. Route packets to the correct UNetConnection
    // 3. Handle new connections via ProcessIncomingConnection
}

void UNetDriver::TickFlush(float DeltaTime)
{
    // Tick all connections
    if (ServerConnection)
    {
        ServerConnection->Tick(DeltaTime);
    }

    for (auto* Conn : ClientConnections)
    {
        if (Conn)
        {
            Conn->Tick(DeltaTime);
        }
    }
}

void UNetDriver::ProcessIncomingConnection(const uint8* Data, int32 Count, const FString& FromAddress)
{
    // Check if we already have a connection from this address
    UNetConnection* ExistingConn = FindConnectionByAddress(FromAddress);

    if (ExistingConn)
    {
        // Route to existing connection
        ExistingConn->ReceivedRawPacket(Data, Count);
    }
    else
    {
        // New connection - begin handshake
        UNetConnection* NewConn = CreateConnection(FromAddress);
        if (NewConn)
        {
            // Send challenge
            FBitWriter ChallengePacket(256, true);
            NewConn->HandshakeHandler.CreateChallengePacket(ChallengePacket, FromAddress);
            NewConn->LowLevelSend(ChallengePacket.GetData(), static_cast<int32>(ChallengePacket.GetNumBytes()));
        }
    }
}

UNetConnection* UNetDriver::FindConnectionByAddress(const FString& Address)
{
    for (auto* Conn : ClientConnections)
    {
        if (Conn && Conn->RemoteAddressStr == Address)
        {
            return Conn;
        }
    }
    return nullptr;
}

UNetConnection* UNetDriver::CreateConnection(const FString& Address)
{
    UNetConnection* NewConn = new UNetConnection();
    NewConn->RemoteAddressStr = Address;
    NewConn->InitBase(this, Socket, Address, EConnectionState::USOCK_Pending);

    ClientConnections.Add(NewConn);

    if (OnConnectionAccepted)
    {
        OnConnectionAccepted(NewConn);
    }

    return NewConn;
}
