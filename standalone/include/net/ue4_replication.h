#pragma once

// UE4 Native Replication Manager
// Bridges the game's actor/property system with ue4net's FRepLayout
// and channel-based replication.
//
// Phase 4: Property replication via FRepLayout (delta serialization)
// Phase 5: RPC dispatch via actor channels + FRPCDispatcher
//
// NOTE: This file uses void* for actor pointers to avoid name conflicts
// between ue4net's stub AActor and the game's actual AActor class.

#include "net/ue4_net_driver.h"

#include <UE4Net/Replication/RepLayout.h>
#include <UE4Net/RPC/RPCDispatch.h>

// Undefine ue4net's log category macros that conflict with our logging system
#undef LogNet
#undef LogNetSerialization
#undef LogNetTraffic
#undef LogHandshake
#undef LogSerialization

#include <vector>
#include <unordered_map>
#include <unordered_set>

class UE4NetConnection;
class UE4NetDriver;

// We use void* for actor pointers to avoid the naming conflict between
// ue4net's stub AActor (in CoreMinimal.h) and the game's actual AActor class.
// Callers should cast to their game's AActor* when using this API.
using GameActorPtr = void*;

// =============================================================================
// Network Relevancy
// =============================================================================

struct FUE4NetworkRelevancyInfo
{
    float NetCullDistanceSquared = 225000000.f;
    bool bAlwaysRelevant = false;
    bool bOnlyRelevantToOwner = false;
    bool bNetUseOwnerRelevancy = false;
};

// =============================================================================
// Actor Replication Info - Per-actor metadata for the native protocol
// =============================================================================

struct FUE4ActorReplicationInfo
{
    GameActorPtr Actor = nullptr;
    FNetworkGUID NetGUID;
    FName ClassName;                      // Used to lookup FRepLayout

    // Shadow state per-connection (for delta property comparison)
    struct FPerConnectionState
    {
        bool bSpawned = false;
        double LastReplicateTime = 0.0;
        FRepState RepState;               // Shadow buffer for delta compare
    };
    std::unordered_map<uint32_t, FPerConnectionState> ConnectionStates;

    // Track dirty properties
    bool bDirtyAll = true;                // Force full replication (initial)
    std::unordered_set<std::string> DirtyProperties;
};

// =============================================================================
// UE4 Native Replication Manager
// =============================================================================

class UE4ReplicationManager
{
public:
    static UE4ReplicationManager& Get();

    // Set the driver (must be called before use)
    void SetDriver(UE4NetDriver* InDriver) { Driver = InDriver; }

    // Actor registration
    void AddActor(GameActorPtr Actor, const FName& ClassName, FNetworkGUID NetGUID);
    void RemoveActor(GameActorPtr Actor);
    void MarkActorDirty(GameActorPtr Actor, const FName& PropertyName = FName());
    void MarkActorFullDirty(GameActorPtr Actor);

    // Register a rep layout for a class (must be done before replicating actors of that class)
    void RegisterClassLayout(const FName& ClassName, const TArray<FRepLayoutCmd>& Cmds, const TArray<FRepParentCmd>& Parents);

    // Main replication tick - called once per server frame
    void ServerReplicateActors(float DeltaTime);

    // Net GUID management
    FNetworkGUID AssignNetGUID(GameActorPtr Actor);
    GameActorPtr FindActorByNetGUID(FNetworkGUID NetGUID) const;
    FNetworkGUID GetNetGUID(GameActorPtr Actor) const;

    // Get info
    int32 GetNumReplicatedActors() const { return static_cast<int32>(ReplicatedActors.size()); }

private:
    UE4ReplicationManager() = default;

    UE4NetDriver* Driver = nullptr;

    // All actors registered for replication
    std::vector<FUE4ActorReplicationInfo> ReplicatedActors;
    std::unordered_map<GameActorPtr, size_t> ActorToIndex;
    std::unordered_map<uint32_t, GameActorPtr> NetGUIDToActor;
    uint32_t NextNetGUID = 2; // Start at 2 (1 reserved, odd = static, even = dynamic)

    // Rep layouts by class name
    std::unordered_map<std::string, FRepLayout> ClassLayouts;

    // Internal
    void ReplicateActorToConnection(FUE4ActorReplicationInfo& Info, UE4NetConnection* Connection);
    void SpawnActorOnConnection(FUE4ActorReplicationInfo& Info, UE4NetConnection* Connection);
    void SendPropertyUpdates(FUE4ActorReplicationInfo& Info, UE4NetConnection* Connection);
    bool IsActorRelevantToConnection(GameActorPtr Actor, UE4NetConnection* Connection) const;
};
