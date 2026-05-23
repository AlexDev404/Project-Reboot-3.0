// Copyright Epic Games, Inc. All Rights Reserved.
// From: Engine/Source/Runtime/Engine/Classes/Engine/NetDriver.h
// Simplified for standalone compilation

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Channel/NetConnection.h"

class FSocket;

/**
 * UNetDriver - The network driver base class
 * Manages connections, ticks the network, and handles the transport layer.
 * In UE4, UIpNetDriver extends this for UDP transport.
 */
class ENGINE_API UNetDriver
{
public:
    UNetDriver();
    virtual ~UNetDriver();

    // Initialization
    virtual bool Init(const FString& ListenURL, int32 Port);
    virtual void Shutdown();

    // Tick all connections
    virtual void TickDispatch(float DeltaTime);
    virtual void TickFlush(float DeltaTime);

    // Accept new connections
    virtual void ProcessIncomingConnection(const uint8* Data, int32 Count, const FString& FromAddress);

    // Connection management
    UNetConnection* ServerConnection;   // Client-side: our connection to server
    TArray<UNetConnection*> ClientConnections; // Server-side: all client connections

    // Find connection by address
    UNetConnection* FindConnectionByAddress(const FString& Address);

    // Create a new connection
    virtual UNetConnection* CreateConnection(const FString& Address);

    // Configuration
    int32 MaxClientRate;
    int32 NetServerMaxTickRate;
    FString NetDriverName;

    // Statistics
    float ElapsedTime;
    int32 InBytesPerSecond;
    int32 OutBytesPerSecond;

    // Transport
    FSocket* Socket;
    int32 ListenPort;

    // Callbacks
    using FOnConnectionAccepted = std::function<void(UNetConnection* NewConnection)>;
    using FOnConnectionLost = std::function<void(UNetConnection* LostConnection)>;

    FOnConnectionAccepted OnConnectionAccepted;
    FOnConnectionLost OnConnectionLost;

    // Is this driver a server?
    bool IsServer() const { return ServerConnection == nullptr; }
};
