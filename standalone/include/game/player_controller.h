#pragma once

// Player Controller - Handles client input and server-side player actions
// Each connected client has one PlayerController

#include "core/object_system.h"
#include <string>
#include <vector>

class AFortPlayerPawnAthena;
class AFortPlayerStateAthena;
class UNetConnection;
struct FGuid;

// =============================================================================
// Building types
// =============================================================================

enum class EFortBuildingType : uint8
{
    Wall = 0,
    Floor = 1,
    Stair = 2,
    Roof = 3,
    Count = 4,
};

enum class EFortResourceType : uint8
{
    Wood = 0,
    Stone = 1,
    Metal = 2,
    Count = 3,
};

// =============================================================================
// Inventory Item
// =============================================================================

struct FFortItemEntry
{
    FGuid ItemGuid;
    std::string ItemDefinitionPath;
    int32 Count = 1;
    int32 Level = 0;
    int32 LoadedAmmo = 0;
    int32 Slot = -1;
    bool bIsDirty = false;
};

// =============================================================================
// AFortPlayerControllerAthena
// =============================================================================

class AFortPlayerControllerAthena
{
public:
    AFortPlayerControllerAthena() = default;
    virtual ~AFortPlayerControllerAthena() = default;

    // Connection
    UNetConnection* GetNetConnection() const { return Connection; }
    void SetNetConnection(UNetConnection* Conn) { Connection = Conn; }

    // Pawn
    AFortPlayerPawnAthena* GetPawn() const { return Pawn; }
    void SetPawn(AFortPlayerPawnAthena* InPawn) { Pawn = InPawn; }
    void Possess(AFortPlayerPawnAthena* InPawn);

    // State
    AFortPlayerStateAthena* GetPlayerState() const { return PlayerState; }

    // Identity
    std::string GetPlayerName() const { return PlayerName; }
    void SetPlayerName(const std::string& Name) { PlayerName = Name; }
    uint8 GetTeamIndex() const { return TeamIndex; }
    void SetTeamIndex(uint8 Index) { TeamIndex = Index; }

    // === SERVER RPCs (called by client, executed on server) ===

    // Building
    void ServerCreateBuildingActor(EFortBuildingType Type, const FVector& Location, const FRotator& Rotation);
    void ServerBeginEditingBuildingActor(UObject* BuildingActor);
    void ServerEditBuildingActor(UObject* BuildingActor, UObject* NewClass, int32 RotationIterations);
    void ServerEndEditingBuildingActor(UObject* BuildingActor);
    void ServerRepairBuildingActor(UObject* BuildingActor);

    // Inventory
    void ServerExecuteInventoryItem(const FGuid& ItemGuid);
    void ServerAttemptInventoryDrop(const FGuid& ItemGuid, int32 Count);
    void ServerDropAllItems();

    // Interaction
    void ServerAttemptInteract(UObject* ReceivingActor, bool bForcePickup);

    // Emotes
    void ServerPlayEmoteItem(const std::string& EmoteDefinitionPath);
    void ServerPlaySprayItem(const std::string& SprayDefinitionPath);

    // Match
    void ServerReadyToStartMatch();
    void ServerLoadingScreenDropped();
    void ServerAttemptAircraftJump(const FRotator& JumpRotation);

    // Combat
    void ServerSuicide();

    // Chat
    void ServerPlaySquadQuickChatMessage(int32 MessageIndex);

    // Admin/Cheat
    void ServerCheat(const std::string& Command);

    // Vehicles
    void ServerRequestSeatChange(int32 SeatIndex);

    // Creative
    void ServerGiveCreativeItem(const std::string& ItemPath);

    // === Client RPCs (sent from server to this client) ===
    void ClientOnPawnDied(const std::vector<uint8>& DeathReport);
    void ClientSendMessage(const std::string& Message);

    // === Gameplay ===
    void GiveItem(const std::string& ItemDefinitionPath, int32 Count = 1, int32 Ammo = -1);
    void RemoveItem(const FGuid& ItemGuid, int32 Count = -1);
    const std::vector<FFortItemEntry>& GetInventory() const { return Inventory; }
    FFortItemEntry* FindInventoryItem(const FGuid& Guid);

    // Respawning
    void ServerRestartPlayer();
    void ServerClientIsReadyToRespawn();

    // State flags
    bool bHasServerFinishedLoading = false;
    bool bIsInAircraft = true;
    bool bIsAlive = true;
    bool bIsDBNO = false;
    bool bIsGhostMode = false;

private:
    UNetConnection* Connection = nullptr;
    AFortPlayerPawnAthena* Pawn = nullptr;
    AFortPlayerStateAthena* PlayerState = nullptr;
    std::string PlayerName;
    uint8 TeamIndex = 0;

    // Inventory
    std::vector<FFortItemEntry> Inventory;
    int32 NextSlot = 0;

    void UpdateInventoryOnClient();
};
