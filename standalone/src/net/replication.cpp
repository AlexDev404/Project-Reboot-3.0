// Replication System implementation

#include "net/replication.h"
#include "net/net_driver.h"
#include "util/logging.h"

#include <algorithm>

UReplicationManager& UReplicationManager::Get()
{
    static UReplicationManager Instance;
    return Instance;
}

void UReplicationManager::AddActor(AActor* Actor)
{
    if (!Actor) return;
    if (ActorToIndex.find(Actor) != ActorToIndex.end()) return; // Already tracked

    FActorReplicationState State;
    State.Actor = Actor;
    State.NetGUID = AssignNetGUID(Actor);

    size_t Index = ReplicatedActors.size();
    ReplicatedActors.push_back(State);
    ActorToIndex[Actor] = Index;

    LOG_DEBUG(LogReplication, "Added actor for replication (NetGUID: {})", State.NetGUID);
}

void UReplicationManager::RemoveActor(AActor* Actor)
{
    if (!Actor) return;
    auto It = ActorToIndex.find(Actor);
    if (It == ActorToIndex.end()) return;

    size_t Index = It->second;
    ReplicatedActors[Index].bPendingDestroy = true;
    // Don't erase immediately - let replication tick handle cleanup
}

void UReplicationManager::MarkActorDirty(AActor* Actor, const FName& PropertyName)
{
    auto It = ActorToIndex.find(Actor);
    if (It == ActorToIndex.end())
    {
        // Actor not registered yet - add it
        AddActor(Actor);
        return;
    }

    auto& State = ReplicatedActors[It->second];
    if (!PropertyName.IsNone())
    {
        State.DirtyProperties.insert(PropertyName);
    }
}

uint32 UReplicationManager::AssignNetGUID(AActor* Actor)
{
    uint32 GUID = NextNetGUID++;
    NetGUIDToActor[GUID] = Actor;
    return GUID;
}

AActor* UReplicationManager::FindActorByNetGUID(uint32 NetGUID) const
{
    auto It = NetGUIDToActor.find(NetGUID);
    if (It != NetGUIDToActor.end()) return It->second;
    return nullptr;
}

uint32 UReplicationManager::GetNetGUID(AActor* Actor) const
{
    auto It = ActorToIndex.find(Actor);
    if (It != ActorToIndex.end())
    {
        return ReplicatedActors[It->second].NetGUID;
    }
    return 0;
}

bool UReplicationManager::IsActorRelevantToConnection(AActor* Actor, UNetConnection* Connection) const
{
    // Simplified relevancy - always relevant for now
    // Full implementation would check distance, owner, etc.
    (void)Actor;
    (void)Connection;
    return true;
}

void UReplicationManager::ServerReplicateActors(UNetDriver* NetDriver, float DeltaTime)
{
    if (!NetDriver) return;
    const auto& Connections = NetDriver->GetClientConnections();
    if (Connections.empty()) return;

    for (auto& State : ReplicatedActors)
    {
        if (State.bPendingDestroy)
        {
            // Send destroy to all connections
            for (auto& Conn : Connections)
            {
                if (!Conn->IsValid()) continue;
                DestroyActorOnConnection(State, Conn.get(), NetDriver);
            }
            continue;
        }

        // Replicate to each connection
        for (auto& Conn : Connections)
        {
            if (!Conn->IsValid()) continue;
            ReplicateActorToConnection(State, Conn.get(), NetDriver);
        }

        // Clear dirty state after replicating to all
        State.DirtyProperties.clear();
    }

    // Remove destroyed actors
    ReplicatedActors.erase(
        std::remove_if(ReplicatedActors.begin(), ReplicatedActors.end(),
            [](const FActorReplicationState& S) { return S.bPendingDestroy; }),
        ReplicatedActors.end()
    );

    // Rebuild index
    ActorToIndex.clear();
    for (size_t i = 0; i < ReplicatedActors.size(); i++)
    {
        ActorToIndex[ReplicatedActors[i].Actor] = i;
    }
}

void UReplicationManager::ReplicateActorToConnection(
    FActorReplicationState& State,
    UNetConnection* Connection,
    UNetDriver* NetDriver)
{
    uint32 ConnId = Connection->GetConnectionId();
    auto& ConnState = State.ConnectionStates[ConnId];

    // Check relevancy
    if (!IsActorRelevantToConnection(State.Actor, Connection))
    {
        ConnState.bIsRelevant = false;
        return;
    }

    ConnState.bIsRelevant = true;

    // First time seeing this actor - spawn it
    if (!ConnState.bSpawned)
    {
        SpawnActorOnConnection(State, Connection, NetDriver);
        ConnState.bSpawned = true;
        return;
    }

    // Send property updates if dirty
    if (!State.DirtyProperties.empty())
    {
        FNetPacket Packet = BuildPropertyUpdatePacket(State);
        Connection->SendPacket(Packet, true);
    }
}

void UReplicationManager::SpawnActorOnConnection(
    FActorReplicationState& State,
    UNetConnection* Connection,
    UNetDriver* NetDriver)
{
    FNetPacket Packet;
    Packet.Type = EPacketType::ActorSpawn;
    Packet.WriteUInt32(State.NetGUID);
    // TODO: Write class name, initial properties, location, etc.
    Connection->SendPacket(Packet, true);
}

void UReplicationManager::DestroyActorOnConnection(
    FActorReplicationState& State,
    UNetConnection* Connection,
    UNetDriver* NetDriver)
{
    FNetPacket Packet;
    Packet.Type = EPacketType::ActorDestroy;
    Packet.WriteUInt32(State.NetGUID);
    Connection->SendPacket(Packet, true);
}

FNetPacket UReplicationManager::BuildPropertyUpdatePacket(FActorReplicationState& State)
{
    FNetPacket Packet;
    Packet.Type = EPacketType::PropertyUpdate;
    Packet.WriteUInt32(State.NetGUID);
    Packet.WriteUInt16(static_cast<uint16>(State.DirtyProperties.size()));

    for (const auto& PropName : State.DirtyProperties)
    {
        Packet.WriteString(PropName.ToString());
        // TODO: Serialize the actual property value
    }

    return Packet;
}
