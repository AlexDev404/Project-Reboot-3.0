#pragma once

// Replication System - Handles actor state synchronization to clients
// Replaces UE4's ServerReplicateActors / ReplicationGraph

#include "core/object_system.h"
#include "net_driver.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

class AActor;
class UNetConnection;

// =============================================================================
// Network Relevancy
// =============================================================================

struct FNetworkRelevancyInfo
{
    float NetCullDistanceSquared = 225000000.f; // Default ~15000 units
    bool bAlwaysRelevant = false;
    bool bOnlyRelevantToOwner = false;
    bool bNetUseOwnerRelevancy = false;
};

// =============================================================================
// Actor Replication State
// =============================================================================

struct FActorReplicationState
{
    AActor* Actor = nullptr;
    uint32 NetGUID = 0;
    double LastReplicationTime = 0.0;
    bool bPendingDestroy = false;

    // Track which properties are dirty
    std::unordered_set<FName, FName::Hash> DirtyProperties;

    // Per-connection state
    struct FPerConnectionState {
        bool bSpawned = false;
        double LastUpdateTime = 0.0;
        bool bIsRelevant = false;
    };
    std::unordered_map<uint32, FPerConnectionState> ConnectionStates;
};

// =============================================================================
// Replication Manager
// =============================================================================

class UReplicationManager
{
public:
    static UReplicationManager& Get();

    // Actor registration
    void AddActor(AActor* Actor);
    void RemoveActor(AActor* Actor);
    void MarkActorDirty(AActor* Actor, const FName& PropertyName = FName());

    // Main replication tick
    void ServerReplicateActors(UNetDriver* NetDriver, float DeltaTime);

    // Net GUID management
    uint32 AssignNetGUID(AActor* Actor);
    AActor* FindActorByNetGUID(uint32 NetGUID) const;
    uint32 GetNetGUID(AActor* Actor) const;

    // Relevancy
    bool IsActorRelevantToConnection(AActor* Actor, UNetConnection* Connection) const;

private:
    UReplicationManager() = default;

    std::vector<FActorReplicationState> ReplicatedActors;
    std::unordered_map<AActor*, size_t> ActorToIndex;
    std::unordered_map<uint32, AActor*> NetGUIDToActor;
    uint32 NextNetGUID = 1;

    // Internal
    void ReplicateActorToConnection(FActorReplicationState& State, UNetConnection* Connection, UNetDriver* NetDriver);
    void SpawnActorOnConnection(FActorReplicationState& State, UNetConnection* Connection, UNetDriver* NetDriver);
    void DestroyActorOnConnection(FActorReplicationState& State, UNetConnection* Connection, UNetDriver* NetDriver);
    FNetPacket BuildPropertyUpdatePacket(FActorReplicationState& State);
};
