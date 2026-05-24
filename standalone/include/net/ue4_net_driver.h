#pragma once

// UE4 Native Protocol Network Driver
// Integrates the ue4net library (full UE4 networking stack) into the standalone server.
// Replaces ENet with raw UDP + UE4's own reliability/ordering/handshake.
//
// This implements all 5 phases:
//   Phase 1: FBitReader/FBitWriter (via ue4net)
//   Phase 2: Raw UDP + FNetPacketNotify + StatelessConnect handshake
//   Phase 3: Channel system (UControlChannel, UActorChannel)
//   Phase 4: FRepLayout property replication
//   Phase 5: RPC dispatch via FRPCDispatcher

// Include UE4Net first - it provides its own core types (uint8, FName, UObject, etc.)
// These are minimal stubs compatible with our needs.
#include <UE4Net/UE4Net.h>

// Undefine ue4net's log category macros that conflict with our logging system
#undef LogNet
#undef LogNetSerialization
#undef LogNetTraffic
#undef LogHandshake
#undef LogSerialization

// Undefine UE4 macros that conflict with standard C++ libraries (spdlog/fmt uses `check`)
#undef check
#undef checkSlow
#undef checkf

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>
#include <cstdint>
#include <atomic>
#include <chrono>

// Forward declarations for game types
class AFortPlayerControllerAthena;

// =============================================================================
// UE4 Native Connection - Wraps ue4net's UNetConnection for game use
// =============================================================================

class UE4NetConnection
{
public:
    UE4NetConnection();
    ~UE4NetConnection();

    // Connection state (mapped from ue4net's EConnectionState)
    enum class EState { Pending, Open, Closed };
    EState GetState() const { return State; }
    bool IsValid() const { return State == EState::Open; }

    // Identity
    uint32_t GetConnectionId() const { return ConnectionId; }
    std::string GetPlayerName() const { return PlayerName; }
    void SetPlayerName(const std::string& Name) { PlayerName = Name; }

    // Associated game objects
    AFortPlayerControllerAthena* GetPlayerController() const { return PlayerController; }
    void SetPlayerController(AFortPlayerControllerAthena* PC) { PlayerController = PC; }

    // High-level sending (uses ue4net channels underneath)
    void SendRPC(const FName& FunctionName, const uint8* ParamData, int32 ParamSize);
    void SendPropertyUpdate(FNetworkGUID NetGUID, uint16 Handle, const uint8* Data, int32 NumBits);

    // Stats
    float GetPing() const { return Ping; }
    uint64_t GetBytesSent() const { return InternalConnection ? InternalConnection->TotalBytesSent : 0; }
    uint64_t GetBytesReceived() const { return InternalConnection ? InternalConnection->TotalBytesReceived : 0; }

    // Close
    void Close(const std::string& Reason = "");

    // Access to the underlying ue4net connection
    UNetConnection* GetInternalConnection() const { return InternalConnection; }

    // Open an actor channel for replication
    UActorChannel* OpenActorChannel(FNetworkGUID NetGUID, int32 ChIndex = -1);
    UActorChannel* FindActorChannel(FNetworkGUID NetGUID);

private:
    friend class UE4NetDriver;

    EState State = EState::Pending;
    uint32_t ConnectionId = 0;
    std::string PlayerName;
    std::string Address;
    float Ping = 0.f;

    AFortPlayerControllerAthena* PlayerController = nullptr;

    // The ue4net connection object (owned by the ue4net driver)
    UNetConnection* InternalConnection = nullptr;

    // Track actor channels by NetGUID
    std::unordered_map<uint32_t, int32> NetGUIDToChannelIndex;
    int32 NextActorChannelIndex = 1; // Channel 0 is always Control
};

// =============================================================================
// UE4 Native Net Driver - Full UE4 protocol over raw UDP
// =============================================================================

class UE4NetDriver
{
public:
    UE4NetDriver();
    ~UE4NetDriver();

    // Lifecycle
    bool Initialize(uint16_t Port);
    void Shutdown();

    // Tick (called every frame)
    void TickFlush(float DeltaTime);

    // Connection management
    const std::vector<std::unique_ptr<UE4NetConnection>>& GetClientConnections() const { return ClientConnections; }
    UE4NetConnection* FindConnection(uint32_t ConnectionId) const;
    void KickPlayer(UE4NetConnection* Connection, const std::string& Reason);
    int32_t GetNumConnections() const { return static_cast<int32_t>(ClientConnections.size()); }

    // Replication interface
    UActorChannel* GetOrCreateActorChannel(UE4NetConnection* Connection, FNetworkGUID NetGUID);
    void CloseActorChannel(UE4NetConnection* Connection, FNetworkGUID NetGUID);

    // RPC registration and dispatch
    void RegisterRPC(const FRPCDefinition& Definition);
    void RegisterRPCHandler(const FName& FunctionName, FRPCDispatcher::FRPCCallback Callback, void* UserData = nullptr);
    FRPCDispatcher& GetRPCDispatcher() { return RPCDispatcher; }

    // Rep layout registration
    void RegisterRepLayout(const FName& ClassName, const FRepLayout& Layout);
    const FRepLayout* FindRepLayout(const FName& ClassName) const;

    // Broadcasting
    void BroadcastRPC(const FName& FunctionName, const uint8* ParamData, int32 ParamSize, UE4NetConnection* Exclude = nullptr);

    // State
    bool IsListening() const { return bListening; }
    uint16_t GetPort() const { return ListenPort; }

    // Callbacks for game code
    using FOnPlayerConnected = std::function<void(UE4NetConnection* Connection)>;
    using FOnPlayerDisconnected = std::function<void(UE4NetConnection* Connection)>;
    using FOnPlayerLogin = std::function<void(UE4NetConnection* Connection, const std::string& URL, const std::string& UniqueId)>;

    FOnPlayerConnected OnPlayerConnected;
    FOnPlayerDisconnected OnPlayerDisconnected;
    FOnPlayerLogin OnPlayerLogin;

private:
    // The ue4net driver
    UNetDriver InternalDriver;

    // RPC system
    FRPCDispatcher RPCDispatcher;

    // Rep layouts by class name
    std::unordered_map<std::string, FRepLayout> RepLayouts;

    // UDP socket (raw, no ENet)
    int SocketFD = -1;
    bool bListening = false;
    uint16_t ListenPort = 0;
    uint32_t NextConnectionId = 1;

    // Our connections
    std::vector<std::unique_ptr<UE4NetConnection>> ClientConnections;

    // Map from address string to connection for quick lookup
    std::unordered_map<std::string, UE4NetConnection*> AddressToConnection;

    // Handshake secret rotation timer
    double LastSecretRotationTime = 0.0;
    static constexpr double SECRET_ROTATION_INTERVAL = 15.0;

    // Internal methods
    bool CreateSocket(uint16_t Port);
    void CloseSocket();
    void PollSocket();
    void SendRawTo(const uint8* Data, int32 Count, const std::string& Address);

    // Connection lifecycle
    UE4NetConnection* CreateNewConnection(const std::string& FromAddress);
    void RemoveConnection(UE4NetConnection* Connection);

    // NMT (control channel) message handling
    void HandleControlMessage(UE4NetConnection* Connection, ENMTType Type, FBitReader& Data);
    void HandleHello(UE4NetConnection* Connection, FBitReader& Data);
    void HandleLogin(UE4NetConnection* Connection, FBitReader& Data);
    void HandleNetspeed(UE4NetConnection* Connection, FBitReader& Data);
    void HandleJoin(UE4NetConnection* Connection, FBitReader& Data);
};
