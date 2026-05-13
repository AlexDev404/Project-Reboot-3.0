// Player Controller implementation

#include "game/player_controller.h"
#include "game/player_pawn.h"
#include "game/player_state.h"
#include "net/net_driver.h"
#include "util/logging.h"

void AFortPlayerControllerAthena::Possess(AFortPlayerPawnAthena* InPawn)
{
    Pawn = InPawn;
    if (Pawn)
    {
        Pawn->SetController(this);
    }
}

void AFortPlayerControllerAthena::ServerCreateBuildingActor(EFortBuildingType Type, const FVector& Location, const FRotator& Rotation)
{
    if (!bIsAlive) return;

    // Check resources
    int32 Cost = 10; // Base cost
    auto& Stats = Pawn ? Pawn->GetStats() : *(FPlayerStats*)nullptr;
    if (!Pawn) return;

    if (GServerConfig.bInfiniteMaterials)
    {
        // Free building
    }
    else
    {
        // TODO: Check and deduct resources based on material type
    }

    LOG_DEBUG(LogGame, "{} built {} at ({}, {}, {})",
        PlayerName, static_cast<int>(Type), Location.X, Location.Y, Location.Z);

    // TODO: Create building actor, add to replication
}

void AFortPlayerControllerAthena::ServerBeginEditingBuildingActor(UObject* BuildingActor)
{
    // Begin edit mode for a building piece
    LOG_DEBUG(LogGame, "{} began editing building", PlayerName);
}

void AFortPlayerControllerAthena::ServerEditBuildingActor(UObject* BuildingActor, UObject* NewClass, int32 RotationIterations)
{
    // Apply edit to building
    LOG_DEBUG(LogGame, "{} edited building", PlayerName);
}

void AFortPlayerControllerAthena::ServerEndEditingBuildingActor(UObject* BuildingActor)
{
    LOG_DEBUG(LogGame, "{} finished editing building", PlayerName);
}

void AFortPlayerControllerAthena::ServerRepairBuildingActor(UObject* BuildingActor)
{
    LOG_DEBUG(LogGame, "{} repaired building", PlayerName);
}

void AFortPlayerControllerAthena::ServerExecuteInventoryItem(const FGuid& ItemGuid)
{
    auto* Item = FindInventoryItem(ItemGuid);
    if (!Item) return;

    LOG_DEBUG(LogGame, "{} equipped item: {}", PlayerName, Item->ItemDefinitionPath);

    if (Pawn)
    {
        Pawn->EquipWeapon(Item->ItemDefinitionPath, Item->Slot);
    }
}

void AFortPlayerControllerAthena::ServerAttemptInventoryDrop(const FGuid& ItemGuid, int32 Count)
{
    auto* Item = FindInventoryItem(ItemGuid);
    if (!Item) return;

    LOG_DEBUG(LogGame, "{} dropped {} x{}", PlayerName, Item->ItemDefinitionPath, Count);

    // TODO: Spawn pickup in world at pawn location
    RemoveItem(ItemGuid, Count);
}

void AFortPlayerControllerAthena::ServerDropAllItems()
{
    LOG_DEBUG(LogGame, "{} dropped all items", PlayerName);
    // Drop all items on death
    for (auto& Item : Inventory)
    {
        // TODO: Spawn pickups in world
    }
    Inventory.clear();
}

void AFortPlayerControllerAthena::ServerAttemptInteract(UObject* ReceivingActor, bool bForcePickup)
{
    LOG_DEBUG(LogGame, "{} interacted with object", PlayerName);
    // Handle pickups, chests, ammo boxes, etc.
}

void AFortPlayerControllerAthena::ServerPlayEmoteItem(const std::string& EmoteDefinitionPath)
{
    LOG_DEBUG(LogGame, "{} played emote: {}", PlayerName, EmoteDefinitionPath);
    // Broadcast emote to nearby players
}

void AFortPlayerControllerAthena::ServerPlaySprayItem(const std::string& SprayDefinitionPath)
{
    LOG_DEBUG(LogGame, "{} used spray: {}", PlayerName, SprayDefinitionPath);
}

void AFortPlayerControllerAthena::ServerReadyToStartMatch()
{
    bHasServerFinishedLoading = true;
    LOG_INFO(LogGame, "{} is ready to start match", PlayerName);
}

void AFortPlayerControllerAthena::ServerLoadingScreenDropped()
{
    LOG_INFO(LogGame, "{} loading screen dropped", PlayerName);
}

void AFortPlayerControllerAthena::ServerAttemptAircraftJump(const FRotator& JumpRotation)
{
    // Handled by game mode
}

void AFortPlayerControllerAthena::ServerSuicide()
{
    if (!bIsAlive || !Pawn) return;
    LOG_INFO(LogGame, "{} committed suicide", PlayerName);
    Pawn->Die(this);
}

void AFortPlayerControllerAthena::ServerPlaySquadQuickChatMessage(int32 MessageIndex)
{
    LOG_DEBUG(LogGame, "{} sent quick chat message: {}", PlayerName, MessageIndex);
}

void AFortPlayerControllerAthena::ServerCheat(const std::string& Command)
{
    LOG_INFO(LogGame, "{} executed cheat: {}", PlayerName, Command);
    // TODO: Admin/operator command handling
}

void AFortPlayerControllerAthena::ServerRequestSeatChange(int32 SeatIndex)
{
    LOG_DEBUG(LogGame, "{} requested seat change to {}", PlayerName, SeatIndex);
}

void AFortPlayerControllerAthena::ServerGiveCreativeItem(const std::string& ItemPath)
{
    GiveItem(ItemPath, 1);
}

void AFortPlayerControllerAthena::ServerRestartPlayer()
{
    LOG_INFO(LogGame, "{} requesting respawn", PlayerName);
}

void AFortPlayerControllerAthena::ServerClientIsReadyToRespawn()
{
    LOG_INFO(LogGame, "{} ready to respawn", PlayerName);
}

// =============================================================================
// Client RPCs
// =============================================================================

void AFortPlayerControllerAthena::ClientOnPawnDied(const std::vector<uint8>& DeathReport)
{
    if (Connection)
    {
        FNetPacket Packet;
        Packet.Type = EPacketType::ClientRPC;
        Packet.WriteString("ClientOnPawnDied");
        Packet.WriteUInt32(static_cast<uint32>(DeathReport.size()));
        if (!DeathReport.empty())
            Packet.WriteBytes(DeathReport.data(), DeathReport.size());
        Connection->SendPacket(Packet, true);
    }
}

void AFortPlayerControllerAthena::ClientSendMessage(const std::string& Message)
{
    if (Connection)
    {
        FNetPacket Packet;
        Packet.Type = EPacketType::ClientRPC;
        Packet.WriteString("ClientMessage");
        Packet.WriteString(Message);
        Connection->SendPacket(Packet, true);
    }
}

// =============================================================================
// Inventory
// =============================================================================

void AFortPlayerControllerAthena::GiveItem(const std::string& ItemDefinitionPath, int32 Count, int32 Ammo)
{
    FFortItemEntry Entry;
    Entry.ItemGuid = FGuid::NewGuid();
    Entry.ItemDefinitionPath = ItemDefinitionPath;
    Entry.Count = Count;
    Entry.LoadedAmmo = Ammo >= 0 ? Ammo : 0;
    Entry.Slot = NextSlot++;

    Inventory.push_back(Entry);

    LOG_DEBUG(LogGame, "Gave {} x{} to {}", ItemDefinitionPath, Count, PlayerName);
    UpdateInventoryOnClient();
}

void AFortPlayerControllerAthena::RemoveItem(const FGuid& ItemGuid, int32 Count)
{
    for (auto It = Inventory.begin(); It != Inventory.end(); ++It)
    {
        if (It->ItemGuid == ItemGuid)
        {
            if (Count < 0 || Count >= It->Count)
            {
                Inventory.erase(It);
            }
            else
            {
                It->Count -= Count;
            }
            UpdateInventoryOnClient();
            return;
        }
    }
}

FFortItemEntry* AFortPlayerControllerAthena::FindInventoryItem(const FGuid& Guid)
{
    for (auto& Item : Inventory)
    {
        if (Item.ItemGuid == Guid) return &Item;
    }
    return nullptr;
}

void AFortPlayerControllerAthena::UpdateInventoryOnClient()
{
    if (!Connection) return;

    FNetPacket Packet;
    Packet.Type = EPacketType::InventoryUpdate;
    Packet.WriteUInt16(static_cast<uint16>(Inventory.size()));

    for (const auto& Item : Inventory)
    {
        Packet.WriteString(Item.ItemDefinitionPath);
        Packet.WriteInt32(Item.Count);
        Packet.WriteInt32(Item.LoadedAmmo);
        Packet.WriteInt32(Item.Slot);
    }

    Connection->SendPacket(Packet, true);
}
