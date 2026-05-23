// Copyright Epic Games, Inc. All Rights Reserved.
// RPC dispatch framework for UE4 networking
// In UE4, RPCs are dispatched via ProcessRemoteFunction in NetDriver.cpp

#pragma once

#include "UE4Net/CoreMinimal.h"
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"
#include "UE4Net/Replication/RepLayout.h"

// RPC execution modes
enum class ERPCMode : uint8
{
    Client      = 0,    // Server → Client (Client RPC)
    Server      = 1,    // Client → Server (Server RPC)
    Multicast   = 2,    // Server → All Clients
    NetMulticast = 2,   // Alias
};

// RPC reliability
enum class ERPCReliability : uint8
{
    Unreliable = 0,
    Reliable   = 1,
};

/**
 * FRPCDefinition - Describes a single RPC function
 */
struct ENGINE_API FRPCDefinition
{
    FName FunctionName;         // Function name (e.g., "ServerFireWeapon")
    uint32 FunctionIndex;       // Network function index (for fast lookup)
    ERPCMode Mode;              // Who receives this RPC
    ERPCReliability Reliability;
    FRepLayout ParamLayout;     // How to serialize/deserialize parameters

    FRPCDefinition() : FunctionIndex(0), Mode(ERPCMode::Server), Reliability(ERPCReliability::Unreliable) {}
};

/**
 * FRPCDispatcher - Central RPC registry and dispatch system
 *
 * Usage:
 *   1. Register RPC definitions (function name, mode, parameter layout)
 *   2. When sending: Serialize params using the RepLayout, send via ActorChannel
 *   3. When receiving: Look up handler, deserialize params, invoke callback
 */
class ENGINE_API FRPCDispatcher
{
public:
    using FRPCCallback = std::function<void(const FName& FuncName, FBitReader& Params, void* UserData)>;

    FRPCDispatcher();
    ~FRPCDispatcher();

    // Register an RPC function definition
    void RegisterRPC(const FRPCDefinition& Definition);

    // Register a handler callback for when an RPC is received
    void RegisterHandler(const FName& FunctionName, FRPCCallback Callback, void* UserData = nullptr);

    // Serialize an RPC call into a bit writer
    bool SerializeRPC(const FName& FunctionName, const uint8* ParamData, int32 ParamSize, FBitWriter& OutWriter) const;

    // Deserialize and dispatch a received RPC
    bool DispatchRPC(const FName& FunctionName, FBitReader& InReader);

    // Lookup
    const FRPCDefinition* FindRPC(const FName& FunctionName) const;
    const FRPCDefinition* FindRPCByIndex(uint32 FunctionIndex) const;

    // Get all registered RPCs
    const TArray<FRPCDefinition>& GetAllRPCs() const { return RPCDefinitions; }

private:
    struct FRPCHandlerEntry
    {
        FName FunctionName;
        FRPCCallback Callback;
        void* UserData;
    };

    TArray<FRPCDefinition> RPCDefinitions;
    TArray<FRPCHandlerEntry> Handlers;
    uint32 NextFunctionIndex;
};
