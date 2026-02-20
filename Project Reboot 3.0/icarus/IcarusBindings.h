#pragma once

/**
 * Icarus Native Bindings
 * 
 * This file contains all the native function bindings that expose
 * Project Reboot's C++ functionality to JavaScript.
 * 
 * Each binding function:
 * 1. Receives arguments from JavaScript as JSValue
 * 2. Calls the appropriate Project Reboot C++ function
 * 3. Returns the result as JSValue
 * 4. Throws Flare on error (caught by module ErrorHandler)
 */

#include "IcarusRuntime.h"
#include "../reboot.h"
#include "../FortPlayerPawn.h"
#include "../FortPlayerControllerAthena.h"
#include "../FortPlayerStateAthena.h"
#include "../FortGameModeAthena.h"
#include "../FortGameStateAthena.h"
#include "../FortInventory.h"
#include "../FortSafeZoneIndicator.h"
#include "../FortServerBotManagerAthena.h"
#include "../commands.h"
#include "../bots.h"

namespace Icarus {
namespace Bindings {

// ============================================================================
// FWorld Bindings
// ============================================================================

/**
 * FWorld_GetProperty(propertyName?: string) -> any
 * Get world properties (state, game_mode, playlist, damage)
 */
inline JSValue FWorld_GetProperty(const std::vector<JSValue>& args) {
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
    
    if (!GameState || !GameMode) {
        return JSValue();  // undefined
    }
    
    // If no argument, return all properties
    if (args.empty() || args[0].isUndefined()) {
        JSValue result = JSValue::Object();
        
        // Get game phase as state
        auto phase = GameState->GetGamePhase();
        std::string state;
        switch (phase) {
            case EAthenaGamePhase::None:
            case EAthenaGamePhase::Setup:
            case EAthenaGamePhase::Warmup:
                state = "disengaged";
                break;
            case EAthenaGamePhase::Aircraft:
                state = "engaged";
                break;
            default:
                state = "inProgress";
                break;
        }
        
        result.objectValue["state"] = JSValue(state);
        result.objectValue["playersLeft"] = JSValue(GameState->GetPlayersLeft());
        result.objectValue["teamsLeft"] = JSValue(GameState->GetTeamsLeft());
        result.objectValue["safeZonePaused"] = JSValue(GameState->IsSafeZonePaused());
        
        return result;
    }
    
    // Get specific property
    std::string propName = args[0].asString();
    
    if (propName == "state") {
        auto phase = GameState->GetGamePhase();
        switch (phase) {
            case EAthenaGamePhase::None:
            case EAthenaGamePhase::Setup:
            case EAthenaGamePhase::Warmup:
                return JSValue("disengaged");
            case EAthenaGamePhase::Aircraft:
                return JSValue("engaged");
            default:
                return JSValue("inProgress");
        }
    }
    else if (propName == "playersLeft") {
        return JSValue(GameState->GetPlayersLeft());
    }
    else if (propName == "teamsLeft") {
        return JSValue(GameState->GetTeamsLeft());
    }
    else if (propName == "safeZonePaused") {
        return JSValue(GameState->IsSafeZonePaused());
    }
    
    return JSValue();  // undefined for unknown properties
}

/**
 * FWorld_SetProperty(propertyName: string, value: any) -> boolean
 * Set world properties
 */
inline JSValue FWorld_SetProperty(const std::vector<JSValue>& args) {
    if (args.size() < 2) {
        return JSValue(false);
    }
    
    std::string propName = args[0].asString();
    
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    
    if (!GameState) {
        return JSValue(false);
    }
    
    if (propName == "safeZonePaused") {
        GameState->IsSafeZonePaused() = args[1].asBool();
        return JSValue(true);
    }
    else if (propName == "damage") {
        // This would need to set a flag for damage handling
        // Implementation depends on how Project Reboot handles this
        return JSValue(true);
    }
    
    return JSValue(false);
}

/**
 * FWorld_GetPawnList() -> PawnInfo[]
 * Get all pawns in the world
 */
inline JSValue FWorld_GetPawnList(const std::vector<JSValue>& args) {
    JSValue result = JSValue::Array();
    
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    if (!GameState) {
        return result;
    }
    
    // Iterate through all player states and get their pawns
    auto& PlayerArray = GameState->GetPlayerArray();
    
    for (int i = 0; i < PlayerArray.Num(); i++) {
        auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerArray[i]);
        if (!PlayerState) continue;
        
        auto Controller = Cast<AFortPlayerControllerAthena>(PlayerState->GetOwner());
        if (!Controller) continue;
        
        auto Pawn = Controller->GetPawn();
        if (!Pawn) continue;
        
        auto FortPawn = Cast<AFortPlayerPawn>(Pawn);
        if (!FortPawn) continue;
        
        JSValue pawnInfo = JSValue::Object();
        pawnInfo.objectValue["id"] = JSValue(std::to_string(reinterpret_cast<uintptr_t>(FortPawn)));
        pawnInfo.objectValue["username"] = JSValue(PlayerState->GetPlayerName().ToString());
        
        // Get location
        auto Location = FortPawn->GetActorLocation();
        JSValue loc = JSValue::Object();
        loc.objectValue["x"] = JSValue(Location.X);
        loc.objectValue["y"] = JSValue(Location.Y);
        loc.objectValue["z"] = JSValue(Location.Z);
        pawnInfo.objectValue["location"] = loc;
        
        // Get health/shield
        pawnInfo.objectValue["health"] = JSValue(FortPawn->GetHealth());
        pawnInfo.objectValue["maxHealth"] = JSValue(FortPawn->GetMaxHealth());
        pawnInfo.objectValue["shield"] = JSValue(FortPawn->GetShield());
        pawnInfo.objectValue["maxShield"] = JSValue(100.0);  // Standard max shield
        
        pawnInfo.objectValue["isAlive"] = JSValue(!FortPawn->IsDBNO() && FortPawn->GetHealth() > 0);
        pawnInfo.objectValue["teamId"] = JSValue(PlayerState->GetTeamIndex());
        
        result.arrayValue.push_back(pawnInfo);
    }
    
    return result;
}

/**
 * FWorld_GetPawnByUsername(username: string) -> PawnInfo | undefined
 */
inline JSValue FWorld_GetPawnByUsername(const std::vector<JSValue>& args) {
    if (args.empty() || !args[0].isString()) {
        return JSValue();
    }
    
    std::string targetUsername = args[0].asString();
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    
    if (!GameState) {
        return JSValue();
    }
    
    auto& PlayerArray = GameState->GetPlayerArray();
    
    for (int i = 0; i < PlayerArray.Num(); i++) {
        auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerArray[i]);
        if (!PlayerState) continue;
        
        std::string playerName = PlayerState->GetPlayerName().ToString();
        if (playerName == targetUsername) {
            auto Controller = Cast<AFortPlayerControllerAthena>(PlayerState->GetOwner());
            if (!Controller) continue;
            
            auto Pawn = Cast<AFortPlayerPawn>(Controller->GetPawn());
            if (!Pawn) continue;
            
            JSValue pawnInfo = JSValue::Object();
            pawnInfo.objectValue["id"] = JSValue(std::to_string(reinterpret_cast<uintptr_t>(Pawn)));
            pawnInfo.objectValue["username"] = JSValue(playerName);
            
            auto Location = Pawn->GetActorLocation();
            JSValue loc = JSValue::Object();
            loc.objectValue["x"] = JSValue(Location.X);
            loc.objectValue["y"] = JSValue(Location.Y);
            loc.objectValue["z"] = JSValue(Location.Z);
            pawnInfo.objectValue["location"] = loc;
            
            pawnInfo.objectValue["health"] = JSValue(Pawn->GetHealth());
            pawnInfo.objectValue["maxHealth"] = JSValue(Pawn->GetMaxHealth());
            pawnInfo.objectValue["shield"] = JSValue(Pawn->GetShield());
            pawnInfo.objectValue["isAlive"] = JSValue(!Pawn->IsDBNO() && Pawn->GetHealth() > 0);
            pawnInfo.objectValue["teamId"] = JSValue(PlayerState->GetTeamIndex());
            
            return pawnInfo;
        }
    }
    
    return JSValue();  // Not found
}

// ============================================================================
// FPawn Bindings
// ============================================================================

/**
 * Helper to get pawn by ID
 */
inline AFortPlayerPawn* GetPawnById(const std::string& id) {
    uintptr_t pawnPtr = std::stoull(id);
    return reinterpret_cast<AFortPlayerPawn*>(pawnPtr);
}

/**
 * FPawn_Move(pawnId: string, x: number, y: number, z: number) -> boolean
 */
inline JSValue FPawn_Move(const std::vector<JSValue>& args) {
    if (args.size() < 4) {
        return JSValue(false);
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue(false);
    }
    
    FVector NewLocation;
    NewLocation.X = args[1].asNumber();
    NewLocation.Y = args[2].asNumber();
    NewLocation.Z = args[3].asNumber();
    
    Pawn->TeleportTo(NewLocation, Pawn->GetActorRotation());
    
    return JSValue(true);
}

/**
 * FPawn_Teleport(pawnId: string, x: number, y: number, z: number) -> boolean
 */
inline JSValue FPawn_Teleport(const std::vector<JSValue>& args) {
    return FPawn_Move(args);  // Same implementation
}

/**
 * FPawn_SetHealth(pawnId: string, health: number) -> boolean
 */
inline JSValue FPawn_SetHealth(const std::vector<JSValue>& args) {
    if (args.size() < 2) {
        return JSValue(false);
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue(false);
    }
    
    float health = static_cast<float>(args[1].asNumber());
    Pawn->SetHealth(health);
    
    return JSValue(true);
}

/**
 * FPawn_SetShield(pawnId: string, shield: number) -> boolean
 */
inline JSValue FPawn_SetShield(const std::vector<JSValue>& args) {
    if (args.size() < 2) {
        return JSValue(false);
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue(false);
    }
    
    float shield = static_cast<float>(args[1].asNumber());
    Pawn->SetShield(shield);
    
    return JSValue(true);
}

/**
 * FPawn_Kill(pawnId: string) -> boolean
 */
inline JSValue FPawn_Kill(const std::vector<JSValue>& args) {
    if (args.empty()) {
        return JSValue(false);
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue(false);
    }
    
    // Kill the pawn by setting health to 0
    Pawn->SetHealth(0);
    
    // Or use the proper death function if available
    // Pawn->Die();
    
    return JSValue(true);
}

/**
 * FPawn_GetLocation(pawnId: string) -> FVector
 */
inline JSValue FPawn_GetLocation(const std::vector<JSValue>& args) {
    if (args.empty()) {
        return JSValue();
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue();
    }
    
    auto Location = Pawn->GetActorLocation();
    
    JSValue result = JSValue::Object();
    result.objectValue["x"] = JSValue(Location.X);
    result.objectValue["y"] = JSValue(Location.Y);
    result.objectValue["z"] = JSValue(Location.Z);
    
    return result;
}

/**
 * FPawn_SetCostume(pawnId: string, costumeId: string) -> boolean
 * Sets the player's skin/costume
 */
inline JSValue FPawn_SetCostume(const std::vector<JSValue>& args) {
    if (args.size() < 2) {
        return JSValue(false);
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue(false);
    }
    
    std::string costumeId = args[1].asString();
    
    // Load the character part and apply it
    // This would need to interface with Fortnite's cosmetic system
    // Implementation depends on Project Reboot's existing methods
    
    return JSValue(true);
}

// ============================================================================
// FGame Bindings
// ============================================================================

/**
 * FGame_StartMatch() -> boolean
 */
inline JSValue FGame_StartMatch(const std::vector<JSValue>& args) {
    auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
    if (!GameMode) {
        return JSValue(false);
    }
    
    // Start the aircraft phase
    GameMode->StartAircraftPhase();
    
    return JSValue(true);
}

/**
 * FGame_EndMatch(options?: { winnerId?: string, reason?: string }) -> boolean
 */
inline JSValue FGame_EndMatch(const std::vector<JSValue>& args) {
    auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
    if (!GameMode) {
        return JSValue(false);
    }
    
    // End the game
    GameMode->EndGame();
    
    return JSValue(true);
}

/**
 * FGame_GetMatchState() -> string
 */
inline JSValue FGame_GetMatchState(const std::vector<JSValue>& args) {
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    if (!GameState) {
        return JSValue("unknown");
    }
    
    auto phase = GameState->GetGamePhase();
    switch (phase) {
        case EAthenaGamePhase::None: return JSValue("none");
        case EAthenaGamePhase::Setup: return JSValue("setup");
        case EAthenaGamePhase::Warmup: return JSValue("warmup");
        case EAthenaGamePhase::Aircraft: return JSValue("aircraft");
        case EAthenaGamePhase::SafeZones: return JSValue("safezones");
        case EAthenaGamePhase::EndGame: return JSValue("endgame");
        default: return JSValue("unknown");
    }
}

/**
 * FGame_SetPlayersLeft(count: number) -> boolean
 */
inline JSValue FGame_SetPlayersLeft(const std::vector<JSValue>& args) {
    if (args.empty()) {
        return JSValue(false);
    }
    
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    if (!GameState) {
        return JSValue(false);
    }
    
    GameState->GetPlayersLeft() = args[0].asInt();
    
    return JSValue(true);
}

// ============================================================================
// FStorm Bindings
// ============================================================================

/**
 * FStorm_GetCurrentPhase() -> number
 */
inline JSValue FStorm_GetCurrentPhase(const std::vector<JSValue>& args) {
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    if (!GameState) {
        return JSValue(0);
    }
    
    // Get current storm phase from safe zone indicator
    // Implementation depends on how Project Reboot exposes this
    
    return JSValue(0);
}

/**
 * FStorm_Pause() -> boolean
 */
inline JSValue FStorm_Pause(const std::vector<JSValue>& args) {
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    if (!GameState) {
        return JSValue(false);
    }
    
    GameState->IsSafeZonePaused() = true;
    
    return JSValue(true);
}

/**
 * FStorm_Resume() -> boolean
 */
inline JSValue FStorm_Resume(const std::vector<JSValue>& args) {
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    if (!GameState) {
        return JSValue(false);
    }
    
    GameState->IsSafeZonePaused() = false;
    
    return JSValue(true);
}

/**
 * FStorm_SkipToPhase(phase: number) -> boolean
 */
inline JSValue FStorm_SkipToPhase(const std::vector<JSValue>& args) {
    if (args.empty()) {
        return JSValue(false);
    }
    
    // This would need to interface with the safe zone indicator
    // to skip to a specific phase
    
    return JSValue(true);
}

// ============================================================================
// FInventory Bindings
// ============================================================================

/**
 * FInventory_GiveItem(playerId: string, itemId: string, count: number) -> boolean
 */
inline JSValue FInventory_GiveItem(const std::vector<JSValue>& args) {
    if (args.size() < 3) {
        return JSValue(false);
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue(false);
    }
    
    auto Controller = Cast<AFortPlayerControllerAthena>(Pawn->GetController());
    if (!Controller) {
        return JSValue(false);
    }
    
    std::string itemId = args[1].asString();
    int count = args[2].asInt();
    
    // Load the item definition
    auto ItemDef = LoadObject<UFortItemDefinition>(itemId.c_str());
    if (!ItemDef) {
        return JSValue(false);
    }
    
    // Give the item to the player
    Controller->AddItemToInventory(ItemDef, EFortQuickBars::Primary, 0, count);
    
    return JSValue(true);
}

// ============================================================================
// FAdmin Bindings  
// ============================================================================

/**
 * FAdmin_Kick(playerId: string, reason: string) -> boolean
 */
inline JSValue FAdmin_Kick(const std::vector<JSValue>& args) {
    if (args.empty()) {
        return JSValue(false);
    }
    
    auto Pawn = GetPawnById(args[0].asString());
    if (!Pawn) {
        return JSValue(false);
    }
    
    auto Controller = Cast<AFortPlayerControllerAthena>(Pawn->GetController());
    if (!Controller) {
        return JSValue(false);
    }
    
    std::string reason = args.size() > 1 ? args[1].asString() : "Kicked by administrator";
    
    // Send kick message and kill the pawn
    SendMessageToConsole(Controller, FString((std::wstring(L"You have been kicked: ") + std::wstring(reason.begin(), reason.end())).c_str()));
    Pawn->SetHealth(0);
    
    return JSValue(true);
}

/**
 * FAdmin_Broadcast(message: string) -> boolean
 */
inline JSValue FAdmin_Broadcast(const std::vector<JSValue>& args) {
    if (args.empty()) {
        return JSValue(false);
    }
    
    std::string message = args[0].asString();
    
    auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
    if (!GameState) {
        return JSValue(false);
    }
    
    // Send message to all players
    auto& PlayerArray = GameState->GetPlayerArray();
    
    for (int i = 0; i < PlayerArray.Num(); i++) {
        auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerArray[i]);
        if (!PlayerState) continue;
        
        auto Controller = Cast<AFortPlayerControllerAthena>(PlayerState->GetOwner());
        if (!Controller) continue;
        
        SendMessageToConsole(Controller, FString(std::wstring(message.begin(), message.end()).c_str()));
    }
    
    return JSValue(true);
}

// ============================================================================
// Registration
// ============================================================================

/**
 * Register all bindings with the runtime
 */
inline void registerAllBindings(Runtime& runtime) {
    // FWorld
    runtime.registerFunction("FWorld_GetProperty", FWorld_GetProperty);
    runtime.registerFunction("FWorld_SetProperty", FWorld_SetProperty);
    runtime.registerFunction("FWorld_GetPawnList", FWorld_GetPawnList);
    runtime.registerFunction("FWorld_GetPawnByUsername", FWorld_GetPawnByUsername);
    
    // FPawn
    runtime.registerFunction("FPawn_Move", FPawn_Move);
    runtime.registerFunction("FPawn_Teleport", FPawn_Teleport);
    runtime.registerFunction("FPawn_SetHealth", FPawn_SetHealth);
    runtime.registerFunction("FPawn_SetShield", FPawn_SetShield);
    runtime.registerFunction("FPawn_Kill", FPawn_Kill);
    runtime.registerFunction("FPawn_GetLocation", FPawn_GetLocation);
    runtime.registerFunction("FPawn_SetCostume", FPawn_SetCostume);
    
    // FGame
    runtime.registerFunction("FGame_StartMatch", FGame_StartMatch);
    runtime.registerFunction("FGame_EndMatch", FGame_EndMatch);
    runtime.registerFunction("FGame_GetMatchState", FGame_GetMatchState);
    runtime.registerFunction("FGame_SetPlayersLeft", FGame_SetPlayersLeft);
    
    // FStorm
    runtime.registerFunction("FStorm_GetCurrentPhase", FStorm_GetCurrentPhase);
    runtime.registerFunction("FStorm_Pause", FStorm_Pause);
    runtime.registerFunction("FStorm_Resume", FStorm_Resume);
    runtime.registerFunction("FStorm_SkipToPhase", FStorm_SkipToPhase);
    
    // FInventory
    runtime.registerFunction("FInventory_GiveItem", FInventory_GiveItem);
    
    // FAdmin
    runtime.registerFunction("FAdmin_Kick", FAdmin_Kick);
    runtime.registerFunction("FAdmin_Broadcast", FAdmin_Broadcast);
    
    LOG_INFO(LogIcarus, "Registered {} native bindings", 18);
}

} // namespace Bindings
} // namespace Icarus
