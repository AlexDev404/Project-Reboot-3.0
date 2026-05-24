#pragma once

// ENet-based Network Driver - Replaces UE4's UNetDriver
// Handles client connections, actor replication, and RPC routing

#include "core/platform.h"
#include "core/object_system.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>

#ifdef WITH_ENET
#include <enet/enet.h>
#endif

// Forward declarations
class AActor;

// =============================================================================
// Packet types for the Reboot protocol
// =============================================================================

enum class EPacketType : uint8
{
    // Connection
    Hello = 0x00,
    Welcome = 0x01,
    Login = 0x02,
    LoginAccepted = 0x03,
    Disconnect = 0x04,

    // Replication
    ActorSpawn = 0x10,
    ActorDestroy = 0x11,
    ActorUpdate = 0x12,
    PropertyUpdate = 0x13,

    // RPCs
    ServerRPC = 0x20,
    ClientRPC = 0x21,
    MulticastRPC = 0x22,

    // Game state
    GameStateUpdate = 0x30,
    PlayerStateUpdate = 0x31,
    InventoryUpdate = 0x32,

    // Input/Movement
    PlayerMove = 0x40,
    PlayerAction = 0x41,

    // Matchmaking
    ReadyToStart = 0x50,
    MatchStarted = 0x51,
    MatchEnded = 0x52,
};

// =============================================================================
// Network Packet
// =============================================================================

struct FNetPacket
{
    EPacketType Type;
    uint32 ChannelId = 0;
    std::vector<uint8> Data;

    // Serialization helpers
    void WriteUInt8(uint8 Val);
    void WriteUInt16(uint16 Val);
    void WriteUInt32(uint32 Val);
    void WriteInt32(int32 Val);
    void WriteFloat(float Val);
    void WriteString(const std::string& Str);
    void WriteVector(const FVector& Vec);
    void WriteRotator(const FRotator& Rot);
    void WriteBytes(const uint8* Data, size_t Len);

    uint8 ReadUInt8(size_t& Offset) const;
    uint16 ReadUInt16(size_t& Offset) const;
    uint32 ReadUInt32(size_t& Offset) const;
    int32 ReadInt32(size_t& Offset) const;
    float ReadFloat(size_t& Offset) const;
    std::string ReadString(size_t& Offset) const;
    FVector ReadVector(size_t& Offset) const;
    FRotator ReadRotator(size_t& Offset) const;
};

// =============================================================================
// Net Connection - Represents a connected client
// =============================================================================

namespace Reboot {

class UNetConnection
{
public:
    UNetConnection() = default;
    ~UNetConnection();

    // Connection state
    enum class EState { Pending, Open, Closed };
    EState GetState() const { return State; }
    bool IsValid() const { return State == EState::Open; }

    // Identity
    uint32 GetConnectionId() const { return ConnectionId; }
    std::string GetPlayerName() const { return PlayerName; }
    void SetPlayerName(const std::string& Name) { PlayerName = Name; }

    // Associated game objects
    class AFortPlayerControllerAthena* GetPlayerController() const { return PlayerController; }
    void SetPlayerController(class AFortPlayerControllerAthena* PC) { PlayerController = PC; }

    // Sending
    void SendPacket(const FNetPacket& Packet, bool bReliable = true);
    void SendRPC(const std::string& FunctionName, const std::vector<uint8>& Params);

    // Stats
    float GetPing() const { return Ping; }
    uint64 GetBytesSent() const { return BytesSent; }
    uint64 GetBytesReceived() const { return BytesReceived; }

    // Kick
    void Close(const std::string& Reason = "");

private:
    friend class UNetDriver;

    EState State = EState::Pending;
    uint32 ConnectionId = 0;
    std::string PlayerName;
    std::string Address;
    float Ping = 0.f;
    uint64 BytesSent = 0;
    uint64 BytesReceived = 0;

    class AFortPlayerControllerAthena* PlayerController = nullptr;

#ifdef WITH_ENET
    ENetPeer* Peer = nullptr;
#endif
};

// =============================================================================
// Net Driver - Main networking subsystem
// =============================================================================

class UNetDriver
{
public:
    UNetDriver() = default;
    ~UNetDriver();

    // Lifecycle
    bool Initialize(uint16 Port);
    void Shutdown();

    // Tick (called every frame)
    void TickFlush(float DeltaTime);

    // Connection management
    const std::vector<std::unique_ptr<UNetConnection>>& GetClientConnections() const { return ClientConnections; }
    UNetConnection* FindConnection(uint32 ConnectionId) const;
    void KickPlayer(UNetConnection* Connection, const std::string& Reason);
    int32 GetNumConnections() const { return static_cast<int32>(ClientConnections.size()); }

    // Replication
    void ReplicateActor(AActor* Actor);
    void ReplicateActorToAll(AActor* Actor);
    void DestroyActorOnClients(AActor* Actor);

    // Broadcasting
    void BroadcastPacket(const FNetPacket& Packet, UNetConnection* Exclude = nullptr);

    // Packet handlers
    using FPacketHandler = std::function<void(UNetConnection* Conn, const FNetPacket& Packet)>;
    void RegisterPacketHandler(EPacketType Type, FPacketHandler Handler);

    // State
    bool IsListening() const { return bListening; }
    uint16 GetPort() const { return ListenPort; }

private:
    // ENet
#ifdef WITH_ENET
    ENetHost* Server = nullptr;
#endif

    bool bListening = false;
    uint16 ListenPort = 0;
    uint32 NextConnectionId = 1;

    std::vector<std::unique_ptr<UNetConnection>> ClientConnections;
    std::unordered_map<EPacketType, FPacketHandler> PacketHandlers;

    // Internal
    void ProcessIncomingPackets();
    void OnClientConnected(UNetConnection* Connection);
    void OnClientDisconnected(UNetConnection* Connection);
    void OnPacketReceived(UNetConnection* Connection, const uint8* Data, size_t Length);
};

} // namespace Reboot
