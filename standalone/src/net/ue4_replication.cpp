// UE4 Native Replication Manager - Implementation
// Bridges game actors with ue4net's channel-based replication + FRepLayout

#include "net/ue4_replication.h"
#include "util/logging.h"

#include <spdlog/fmt/fmt.h>

// =============================================================================
// Singleton
// =============================================================================

UE4ReplicationManager& UE4ReplicationManager::Get()
{
    static UE4ReplicationManager Instance;
    return Instance;
}

// =============================================================================
// Actor Registration
// =============================================================================

void UE4ReplicationManager::AddActor(GameActorPtr Actor, const FName& ClassName, FNetworkGUID NetGUID)
{
    if (!Actor) return;
    if (ActorToIndex.count(Actor)) return; // Already registered

    FUE4ActorReplicationInfo Info;
    Info.Actor = Actor;
    Info.ClassName = ClassName;
    Info.NetGUID = NetGUID.IsValid() ? NetGUID : AssignNetGUID(Actor);
    Info.bDirtyAll = true; // First replication should be full

    size_t Index = ReplicatedActors.size();
    ReplicatedActors.push_back(std::move(Info));
    ActorToIndex[Actor] = Index;
    NetGUIDToActor[ReplicatedActors[Index].NetGUID.Value] = Actor;

    LOG_INFO(LogNet, "Registered actor for UE4 replication: class={} NetGUID={}",
        ClassName.ToString(), ReplicatedActors[Index].NetGUID.Value);
}

void UE4ReplicationManager::RemoveActor(GameActorPtr Actor)
{
    if (!Actor) return;

    auto It = ActorToIndex.find(Actor);
    if (It == ActorToIndex.end()) return;

    size_t Index = It->second;
    FUE4ActorReplicationInfo& Info = ReplicatedActors[Index];

    // Close actor channels on all connections
    if (Driver)
    {
        for (auto& Conn : Driver->GetClientConnections())
        {
            Driver->CloseActorChannel(Conn.get(), Info.NetGUID);
        }
    }

    NetGUIDToActor.erase(Info.NetGUID.Value);
    ActorToIndex.erase(It);

    // Swap-remove from vector
    if (Index < ReplicatedActors.size() - 1)
    {
        ReplicatedActors[Index] = std::move(ReplicatedActors.back());
        ActorToIndex[ReplicatedActors[Index].Actor] = Index;
    }
    ReplicatedActors.pop_back();
}

void UE4ReplicationManager::MarkActorDirty(GameActorPtr Actor, const FName& PropertyName)
{
    if (!Actor) return;
    auto It = ActorToIndex.find(Actor);
    if (It == ActorToIndex.end()) return;

    FUE4ActorReplicationInfo& Info = ReplicatedActors[It->second];
    if (!PropertyName.IsNone())
    {
        Info.DirtyProperties.insert(PropertyName.ToString());
    }
    else
    {
        Info.bDirtyAll = true;
    }
}

void UE4ReplicationManager::MarkActorFullDirty(GameActorPtr Actor)
{
    if (!Actor) return;
    auto It = ActorToIndex.find(Actor);
    if (It == ActorToIndex.end()) return;
    ReplicatedActors[It->second].bDirtyAll = true;
}

// =============================================================================
// Rep Layout Registration
// =============================================================================

void UE4ReplicationManager::RegisterClassLayout(const FName& ClassName,
    const TArray<FRepLayoutCmd>& Cmds, const TArray<FRepParentCmd>& Parents)
{
    FRepLayout Layout;
    Layout.InitFromPropertyList(ClassName, Cmds, Parents);
    ClassLayouts[ClassName.ToString()] = std::move(Layout);

    // Also register with the driver
    if (Driver)
    {
        Driver->RegisterRepLayout(ClassName, ClassLayouts[ClassName.ToString()]);
    }

    LOG_INFO(LogNet, "Registered FRepLayout for class '{}' with {} properties",
        ClassName.ToString(), Cmds.Num());
}

// =============================================================================
// Net GUID Management
// =============================================================================

FNetworkGUID UE4ReplicationManager::AssignNetGUID(GameActorPtr Actor)
{
    // Dynamic GUIDs are even numbers (bit 0 = 0 means dynamic)
    uint32_t GUID = NextNetGUID;
    NextNetGUID += 2; // Skip by 2 to keep them even (dynamic)
    return FNetworkGUID(GUID);
}

GameActorPtr UE4ReplicationManager::FindActorByNetGUID(FNetworkGUID NetGUID) const
{
    auto It = NetGUIDToActor.find(NetGUID.Value);
    return (It != NetGUIDToActor.end()) ? It->second : nullptr;
}

FNetworkGUID UE4ReplicationManager::GetNetGUID(GameActorPtr Actor) const
{
    auto It = ActorToIndex.find(Actor);
    if (It == ActorToIndex.end()) return FNetworkGUID(0);
    return ReplicatedActors[It->second].NetGUID;
}

// =============================================================================
// Main Replication Tick
// =============================================================================

void UE4ReplicationManager::ServerReplicateActors(float DeltaTime)
{
    if (!Driver) return;

    for (auto& Info : ReplicatedActors)
    {
        if (!Info.Actor) continue;

        // Skip if nothing is dirty
        if (!Info.bDirtyAll && Info.DirtyProperties.empty()) continue;

        // Replicate to each connected client
        for (auto& Conn : Driver->GetClientConnections())
        {
            if (!Conn->IsValid()) continue;

            // Check relevancy
            if (!IsActorRelevantToConnection(Info.Actor, Conn.get())) continue;

            ReplicateActorToConnection(Info, Conn.get());
        }

        // Clear dirty flags after replicating to all
        Info.bDirtyAll = false;
        Info.DirtyProperties.clear();
    }
}

// =============================================================================
// Per-Connection Replication
// =============================================================================

void UE4ReplicationManager::ReplicateActorToConnection(FUE4ActorReplicationInfo& Info, UE4NetConnection* Connection)
{
    uint32_t ConnId = Connection->GetConnectionId();
    auto& ConnState = Info.ConnectionStates[ConnId];

    if (!ConnState.bSpawned)
    {
        // First time seeing this actor on this connection - send spawn
        SpawnActorOnConnection(Info, Connection);
        ConnState.bSpawned = true;
        ConnState.LastReplicateTime = 0.0;

        // Initialize shadow state for delta comparison
        auto LayoutIt = ClassLayouts.find(Info.ClassName.ToString());
        if (LayoutIt != ClassLayouts.end())
        {
            ConnState.RepState.MarkAllDirty(LayoutIt->second);
        }
    }

    // Send property updates via the actor channel
    SendPropertyUpdates(Info, Connection);
}

void UE4ReplicationManager::SpawnActorOnConnection(FUE4ActorReplicationInfo& Info, UE4NetConnection* Connection)
{
    // Open an actor channel for this actor on this connection
    UActorChannel* Ch = Driver->GetOrCreateActorChannel(Connection, Info.NetGUID);
    if (!Ch)
    {
        LOG_WARN(LogNet, "Failed to open actor channel for NetGUID {} on connection {}",
            Info.NetGUID.Value, Connection->GetConnectionId());
        return;
    }

    // Serialize the initial actor spawn info
    FBitWriter SpawnWriter(1024);
    Ch->SerializeNewActor(SpawnWriter);

    LOG_INFO(LogNet, "Spawned actor (NetGUID={}) on connection {} via actor channel",
        Info.NetGUID.Value, Connection->GetConnectionId());
}

void UE4ReplicationManager::SendPropertyUpdates(FUE4ActorReplicationInfo& Info, UE4NetConnection* Connection)
{
    // Find the rep layout for this actor's class
    auto LayoutIt = ClassLayouts.find(Info.ClassName.ToString());
    if (LayoutIt == ClassLayouts.end()) return;

    const FRepLayout& Layout = LayoutIt->second;

    // Get the actor channel
    UActorChannel* Ch = Connection->FindActorChannel(Info.NetGUID);
    if (!Ch) return;

    // Use FRepLayout to delta-serialize changed properties
    // In a full implementation, we'd compare the actor's current memory against
    // the per-connection shadow state and serialize only differences.
    //
    // For now, trigger the actor channel's replication which uses the layout internally
    Ch->ReplicateActor();
}

bool UE4ReplicationManager::IsActorRelevantToConnection(GameActorPtr Actor, UE4NetConnection* Connection) const
{
    // TODO: Implement proper relevancy based on distance, ownership, etc.
    // For now, all actors are always relevant (matches Fortnite's behavior for
    // game state, player states, and most gameplay actors within the safe zone)
    return true;
}
