// ENet-based Network Driver implementation

#include "net/net_driver.h"
#include "net/replication.h"
#include "util/logging.h"
#include "game/player_controller.h"

#include <spdlog/fmt/fmt.h>
#include <cstring>

// =============================================================================
// FNetPacket serialization helpers
// =============================================================================

void FNetPacket::WriteUInt8(uint8 Val) { Data.push_back(Val); }
void FNetPacket::WriteUInt16(uint16 Val) {
    Data.push_back(static_cast<uint8>(Val & 0xFF));
    Data.push_back(static_cast<uint8>((Val >> 8) & 0xFF));
}
void FNetPacket::WriteUInt32(uint32 Val) {
    Data.push_back(static_cast<uint8>(Val & 0xFF));
    Data.push_back(static_cast<uint8>((Val >> 8) & 0xFF));
    Data.push_back(static_cast<uint8>((Val >> 16) & 0xFF));
    Data.push_back(static_cast<uint8>((Val >> 24) & 0xFF));
}
void FNetPacket::WriteInt32(int32 Val) { WriteUInt32(static_cast<uint32>(Val)); }
void FNetPacket::WriteFloat(float Val) {
    uint32 Bits;
    std::memcpy(&Bits, &Val, sizeof(Bits));
    WriteUInt32(Bits);
}
void FNetPacket::WriteString(const std::string& Str) {
    WriteUInt16(static_cast<uint16>(Str.size()));
    Data.insert(Data.end(), Str.begin(), Str.end());
}
void FNetPacket::WriteVector(const FVector& Vec) {
    WriteFloat(Vec.X); WriteFloat(Vec.Y); WriteFloat(Vec.Z);
}
void FNetPacket::WriteRotator(const FRotator& Rot) {
    WriteFloat(Rot.Pitch); WriteFloat(Rot.Yaw); WriteFloat(Rot.Roll);
}
void FNetPacket::WriteBytes(const uint8* InData, size_t Len) {
    Data.insert(Data.end(), InData, InData + Len);
}

uint8 FNetPacket::ReadUInt8(size_t& Offset) const {
    if (Offset >= Data.size()) return 0;
    return Data[Offset++];
}
uint16 FNetPacket::ReadUInt16(size_t& Offset) const {
    uint16 Val = 0;
    if (Offset + 1 < Data.size()) {
        Val = Data[Offset] | (static_cast<uint16>(Data[Offset + 1]) << 8);
        Offset += 2;
    }
    return Val;
}
uint32 FNetPacket::ReadUInt32(size_t& Offset) const {
    uint32 Val = 0;
    if (Offset + 3 < Data.size()) {
        Val = Data[Offset] | (static_cast<uint32>(Data[Offset+1]) << 8) |
              (static_cast<uint32>(Data[Offset+2]) << 16) | (static_cast<uint32>(Data[Offset+3]) << 24);
        Offset += 4;
    }
    return Val;
}
int32 FNetPacket::ReadInt32(size_t& Offset) const { return static_cast<int32>(ReadUInt32(Offset)); }
float FNetPacket::ReadFloat(size_t& Offset) const {
    uint32 Bits = ReadUInt32(Offset);
    float Val;
    std::memcpy(&Val, &Bits, sizeof(Val));
    return Val;
}
std::string FNetPacket::ReadString(size_t& Offset) const {
    uint16 Len = ReadUInt16(Offset);
    if (Offset + Len > Data.size()) return "";
    std::string Str(Data.begin() + Offset, Data.begin() + Offset + Len);
    Offset += Len;
    return Str;
}
FVector FNetPacket::ReadVector(size_t& Offset) const {
    return {ReadFloat(Offset), ReadFloat(Offset), ReadFloat(Offset)};
}
FRotator FNetPacket::ReadRotator(size_t& Offset) const {
    return {ReadFloat(Offset), ReadFloat(Offset), ReadFloat(Offset)};
}

// =============================================================================
// UNetConnection
// =============================================================================

UNetConnection::~UNetConnection()
{
    Close("Connection destroyed");
}

void UNetConnection::SendPacket(const FNetPacket& Packet, bool bReliable)
{
    if (State != EState::Open) return;

#ifdef WITH_ENET
    if (!Peer) return;

    // Build raw packet: [type:1][channelId:4][data...]
    std::vector<uint8> RawData;
    RawData.push_back(static_cast<uint8>(Packet.Type));
    // Channel ID as 4 bytes
    RawData.push_back(static_cast<uint8>(Packet.ChannelId & 0xFF));
    RawData.push_back(static_cast<uint8>((Packet.ChannelId >> 8) & 0xFF));
    RawData.push_back(static_cast<uint8>((Packet.ChannelId >> 16) & 0xFF));
    RawData.push_back(static_cast<uint8>((Packet.ChannelId >> 24) & 0xFF));
    RawData.insert(RawData.end(), Packet.Data.begin(), Packet.Data.end());

    ENetPacket* EPacket = enet_packet_create(
        RawData.data(), RawData.size(),
        bReliable ? ENET_PACKET_FLAG_RELIABLE : 0
    );

    enet_peer_send(Peer, 0, EPacket);
    BytesSent += RawData.size();
#endif
}

void UNetConnection::SendRPC(const std::string& FunctionName, const std::vector<uint8>& Params)
{
    FNetPacket Packet;
    Packet.Type = EPacketType::ClientRPC;
    Packet.WriteString(FunctionName);
    Packet.WriteUInt32(static_cast<uint32>(Params.size()));
    if (!Params.empty())
        Packet.WriteBytes(Params.data(), Params.size());
    SendPacket(Packet, true);
}

void UNetConnection::Close(const std::string& Reason)
{
    if (State == EState::Closed) return;
    State = EState::Closed;

#ifdef WITH_ENET
    if (Peer)
    {
        enet_peer_disconnect(Peer, 0);
        Peer = nullptr;
    }
#endif

    if (!Reason.empty())
    {
        LOG_INFO(LogNet, "Connection {} closed: {}", ConnectionId, Reason);
    }
}

// =============================================================================
// UNetDriver
// =============================================================================

UNetDriver::~UNetDriver()
{
    Shutdown();
}

bool UNetDriver::Initialize(uint16 Port)
{
#ifdef WITH_ENET
    if (enet_initialize() != 0)
    {
        LOG_ERROR(LogNet, "Failed to initialize ENet");
        return false;
    }

    ENetAddress Address;
    Address.host = ENET_HOST_ANY;
    Address.port = Port;

    Server = enet_host_create(&Address, GServerConfig.MaxPlayers, 2, 0, 0);
    if (!Server)
    {
        LOG_ERROR(LogNet, "Failed to create ENet server on port {}", Port);
        enet_deinitialize();
        return false;
    }

    bListening = true;
    ListenPort = Port;

    LOG_INFO(LogNet, "ENet server listening on port {}", Port);
    return true;
#else
    LOG_ERROR(LogNet, "ENet not available - networking disabled");
    return false;
#endif
}

void UNetDriver::Shutdown()
{
#ifdef WITH_ENET
    if (Server)
    {
        // Disconnect all clients
        for (auto& Conn : ClientConnections)
        {
            if (Conn && Conn->GetState() == UNetConnection::EState::Open)
            {
                Conn->Close("Server shutting down");
            }
        }

        enet_host_flush(Server);
        enet_host_destroy(Server);
        Server = nullptr;
        enet_deinitialize();
    }
#endif

    ClientConnections.clear();
    bListening = false;
    LOG_INFO(LogNet, "Network driver shut down");
}

void UNetDriver::TickFlush(float DeltaTime)
{
    ProcessIncomingPackets();

#ifdef WITH_ENET
    if (Server)
    {
        enet_host_flush(Server);
    }
#endif
}

void UNetDriver::ProcessIncomingPackets()
{
#ifdef WITH_ENET
    if (!Server) return;

    ENetEvent Event;
    while (enet_host_service(Server, &Event, 0) > 0)
    {
        switch (Event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                LOG_INFO(LogNet, "Client connecting from {}:{}",
                    Event.peer->address.host, Event.peer->address.port);

                auto Connection = std::make_unique<UNetConnection>();
                Connection->State = UNetConnection::EState::Open;
                Connection->ConnectionId = NextConnectionId++;
                Connection->Peer = Event.peer;
                Connection->Address = fmt::format("{}:{}", Event.peer->address.host, Event.peer->address.port);

                Event.peer->data = Connection.get();

                OnClientConnected(Connection.get());
                ClientConnections.push_back(std::move(Connection));
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                auto* Connection = static_cast<UNetConnection*>(Event.peer->data);
                if (Connection)
                {
                    Connection->BytesReceived += Event.packet->dataLength;
                    OnPacketReceived(Connection, Event.packet->data, Event.packet->dataLength);
                }
                enet_packet_destroy(Event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                auto* Connection = static_cast<UNetConnection*>(Event.peer->data);
                if (Connection)
                {
                    OnClientDisconnected(Connection);
                    Connection->State = UNetConnection::EState::Closed;
                    Connection->Peer = nullptr;
                }
                Event.peer->data = nullptr;

                // Remove from connections list
                ClientConnections.erase(
                    std::remove_if(ClientConnections.begin(), ClientConnections.end(),
                        [Connection](const std::unique_ptr<UNetConnection>& C) {
                            return C.get() == Connection;
                        }),
                    ClientConnections.end()
                );
                break;
            }

            default:
                break;
        }
    }

    // Update pings
    for (auto& Conn : ClientConnections)
    {
        if (Conn && Conn->Peer)
        {
            Conn->Ping = static_cast<float>(Conn->Peer->roundTripTime);
        }
    }
#endif
}

UNetConnection* UNetDriver::FindConnection(uint32 ConnectionId) const
{
    for (auto& Conn : ClientConnections)
    {
        if (Conn && Conn->GetConnectionId() == ConnectionId)
            return Conn.get();
    }
    return nullptr;
}

void UNetDriver::KickPlayer(UNetConnection* Connection, const std::string& Reason)
{
    if (!Connection) return;

    LOG_INFO(LogNet, "Kicking player {} ({}): {}",
        Connection->GetPlayerName(), Connection->GetConnectionId(), Reason);

    // Send disconnect message first
    FNetPacket DisconnectPacket;
    DisconnectPacket.Type = EPacketType::Disconnect;
    DisconnectPacket.WriteString(Reason);
    Connection->SendPacket(DisconnectPacket, true);

    Connection->Close(Reason);
}

void UNetDriver::BroadcastPacket(const FNetPacket& Packet, UNetConnection* Exclude)
{
    for (auto& Conn : ClientConnections)
    {
        if (Conn.get() != Exclude && Conn->IsValid())
        {
            Conn->SendPacket(Packet, true);
        }
    }
}

void UNetDriver::RegisterPacketHandler(EPacketType Type, FPacketHandler Handler)
{
    PacketHandlers[Type] = std::move(Handler);
}

void UNetDriver::OnClientConnected(UNetConnection* Connection)
{
    LOG_INFO(LogNet, "Client {} connected (ID: {})",
        Connection->Address, Connection->GetConnectionId());

    // Send welcome packet
    FNetPacket Welcome;
    Welcome.Type = EPacketType::Welcome;
    Welcome.WriteUInt32(Connection->GetConnectionId());
    Welcome.WriteString("Project Reboot 3.0");
    Welcome.WriteFloat(static_cast<float>(Fortnite_Version));
    Connection->SendPacket(Welcome, true);
}

void UNetDriver::OnClientDisconnected(UNetConnection* Connection)
{
    LOG_INFO(LogNet, "Client {} disconnected ({})",
        Connection->GetPlayerName().empty() ? "Unknown" : Connection->GetPlayerName(),
        Connection->GetConnectionId());

    // Notify game mode
    // TODO: Call game mode's player disconnect handler
}

void UNetDriver::OnPacketReceived(UNetConnection* Connection, const uint8* Data, size_t Length)
{
    if (Length < 5) return; // Minimum: type(1) + channelId(4)

    FNetPacket Packet;
    Packet.Type = static_cast<EPacketType>(Data[0]);
    Packet.ChannelId = Data[1] | (static_cast<uint32>(Data[2]) << 8) |
                       (static_cast<uint32>(Data[3]) << 16) | (static_cast<uint32>(Data[4]) << 24);
    if (Length > 5)
    {
        Packet.Data.assign(Data + 5, Data + Length);
    }

    // Route to handler
    auto It = PacketHandlers.find(Packet.Type);
    if (It != PacketHandlers.end())
    {
        It->second(Connection, Packet);
    }
    else
    {
        LOG_DEBUG(LogNet, "No handler for packet type 0x{:02X} from connection {}",
            static_cast<int>(Packet.Type), Connection->GetConnectionId());
    }
}

void UNetDriver::ReplicateActor(AActor* Actor)
{
    UReplicationManager::Get().MarkActorDirty(Actor);
}

void UNetDriver::ReplicateActorToAll(AActor* Actor)
{
    UReplicationManager::Get().MarkActorDirty(Actor);
}

void UNetDriver::DestroyActorOnClients(AActor* Actor)
{
    UReplicationManager::Get().RemoveActor(Actor);
}
