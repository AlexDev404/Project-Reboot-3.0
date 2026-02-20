/**
 * FWorld Native Binding Implementation
 * 
 * This file implements the N-API bindings for FWorld, connecting
 * Project Reboot's C++ functions to JavaScript.
 */

#include "FWorldBinding.h"
#include "FlareBinding.h"

// Include Project Reboot headers when building with the full project
#ifdef PROJECT_REBOOT_BUILD
#include "reboot.h"
#include "FortGameStateAthena.h"
#include "FortGameModeAthena.h"
#include "FortPlayerStateAthena.h"
#include "FortPlayerControllerAthena.h"
#include "FortPlayerPawn.h"
#endif

void FWorldBinding::Init(Napi::Env env, Napi::Object exports) {
    // Create FWorld object
    Napi::Object fworld = Napi::Object::New(env);
    
    // Register methods
    fworld.Set("getProperty", Napi::Function::New(env, GetProperty));
    fworld.Set("setProperty", Napi::Function::New(env, SetProperty));
    fworld.Set("getPawnList", Napi::Function::New(env, GetPawnList));
    fworld.Set("getPawnById", Napi::Function::New(env, GetPawnById));
    fworld.Set("getPawnByUsername", Napi::Function::New(env, GetPawnByUsername));
    fworld.Set("spawnActor", Napi::Function::New(env, SpawnActor));
    fworld.Set("destroyActor", Napi::Function::New(env, DestroyActor));
    
    exports.Set("FWorld", fworld);
}

Napi::Value FWorldBinding::GetProperty(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
        
        if (!GameState || !GameMode) {
            return env.Undefined();
        }
        
        // If no argument, return all properties
        if (info.Length() == 0 || info[0].IsUndefined()) {
            Napi::Object result = Napi::Object::New(env);
            
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
            
            result.Set("state", Napi::String::New(env, state));
            result.Set("playersLeft", Napi::Number::New(env, GameState->GetPlayersLeft()));
            result.Set("teamsLeft", Napi::Number::New(env, GameState->GetTeamsLeft()));
            result.Set("safeZonePaused", Napi::Boolean::New(env, GameState->IsSafeZonePaused()));
            
            return result;
        }
        
        // Get specific property
        std::string propName = info[0].As<Napi::String>().Utf8Value();
        
        if (propName == "state") {
            auto phase = GameState->GetGamePhase();
            switch (phase) {
                case EAthenaGamePhase::None:
                case EAthenaGamePhase::Setup:
                case EAthenaGamePhase::Warmup:
                    return Napi::String::New(env, "disengaged");
                case EAthenaGamePhase::Aircraft:
                    return Napi::String::New(env, "engaged");
                default:
                    return Napi::String::New(env, "inProgress");
            }
        }
        else if (propName == "playersLeft") {
            return Napi::Number::New(env, GameState->GetPlayersLeft());
        }
        else if (propName == "teamsLeft") {
            return Napi::Number::New(env, GameState->GetTeamsLeft());
        }
        else if (propName == "safeZonePaused") {
            return Napi::Boolean::New(env, GameState->IsSafeZonePaused());
        }
#else
        // Mock mode - return default values
        if (info.Length() == 0 || info[0].IsUndefined()) {
            Napi::Object result = Napi::Object::New(env);
            result.Set("state", Napi::String::New(env, "disengaged"));
            result.Set("playersLeft", Napi::Number::New(env, 0));
            result.Set("teamsLeft", Napi::Number::New(env, 0));
            result.Set("safeZonePaused", Napi::Boolean::New(env, false));
            return result;
        }
        
        std::string propName = info[0].As<Napi::String>().Utf8Value();
        if (propName == "state") return Napi::String::New(env, "disengaged");
        if (propName == "playersLeft") return Napi::Number::New(env, 0);
        if (propName == "teamsLeft") return Napi::Number::New(env, 0);
        if (propName == "safeZonePaused") return Napi::Boolean::New(env, false);
#endif
        
        return env.Undefined();
    } catch (const std::exception& e) {
        // Create and throw a Flare
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FWorld_GetProperty", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FWorldBinding::SetProperty(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string propName = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        
        if (!GameState) {
            return Napi::Boolean::New(env, false);
        }
        
        if (propName == "safeZonePaused") {
            GameState->IsSafeZonePaused() = info[1].As<Napi::Boolean>().Value();
            return Napi::Boolean::New(env, true);
        }
#else
        // Mock mode - just log and return success
        if (info[1].IsBoolean()) {
            // Mock implementation
        } else if (info[1].IsString()) {
            // Mock implementation
        }
        return Napi::Boolean::New(env, true);
#endif
        
        return Napi::Boolean::New(env, false);
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FWorld_SetProperty", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FWorldBinding::GetPawnList(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env);
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        if (!GameState) {
            return result;
        }
        
        auto& PlayerArray = GameState->GetPlayerArray();
        uint32_t index = 0;
        
        for (int i = 0; i < PlayerArray.Num(); i++) {
            auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerArray[i]);
            if (!PlayerState) continue;
            
            auto Controller = Cast<AFortPlayerControllerAthena>(PlayerState->GetOwner());
            if (!Controller) continue;
            
            auto Pawn = Cast<AFortPlayerPawn>(Controller->GetPawn());
            if (!Pawn) continue;
            
            Napi::Object pawnInfo = Napi::Object::New(env);
            
            // ID is the pointer address as string
            pawnInfo.Set("id", Napi::String::New(env, std::to_string(reinterpret_cast<uintptr_t>(Pawn))));
            pawnInfo.Set("username", Napi::String::New(env, PlayerState->GetPlayerName().ToString()));
            pawnInfo.Set("accountId", Napi::String::New(env, PlayerState->GetUniqueId().ToString()));
            pawnInfo.Set("teamId", Napi::Number::New(env, PlayerState->GetTeamIndex()));
            
            // Location
            auto Location = Pawn->GetActorLocation();
            Napi::Object loc = Napi::Object::New(env);
            loc.Set("x", Napi::Number::New(env, Location.X));
            loc.Set("y", Napi::Number::New(env, Location.Y));
            loc.Set("z", Napi::Number::New(env, Location.Z));
            pawnInfo.Set("location", loc);
            
            // Health and shield
            pawnInfo.Set("health", Napi::Number::New(env, Pawn->GetHealth()));
            pawnInfo.Set("maxHealth", Napi::Number::New(env, Pawn->GetMaxHealth()));
            pawnInfo.Set("shield", Napi::Number::New(env, Pawn->GetShield()));
            pawnInfo.Set("maxShield", Napi::Number::New(env, 100.0));
            pawnInfo.Set("isAlive", Napi::Boolean::New(env, !Pawn->IsDBNO() && Pawn->GetHealth() > 0));
            
            result.Set(index++, pawnInfo);
        }
#else
        // Mock mode - return empty array
#endif
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FWorld_GetPawnList", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return result;
    }
}

Napi::Value FWorldBinding::GetPawnById(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        return env.Undefined();
    }
    
    try {
        std::string id = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        uintptr_t pawnPtr = std::stoull(id);
        auto Pawn = reinterpret_cast<AFortPlayerPawn*>(pawnPtr);
        
        if (!Pawn) {
            return env.Undefined();
        }
        
        // Get player state
        auto Controller = Cast<AFortPlayerControllerAthena>(Pawn->GetController());
        if (!Controller) return env.Undefined();
        
        auto PlayerState = Cast<AFortPlayerStateAthena>(Controller->GetPlayerState());
        if (!PlayerState) return env.Undefined();
        
        Napi::Object pawnInfo = Napi::Object::New(env);
        pawnInfo.Set("id", Napi::String::New(env, id));
        pawnInfo.Set("username", Napi::String::New(env, PlayerState->GetPlayerName().ToString()));
        pawnInfo.Set("accountId", Napi::String::New(env, PlayerState->GetUniqueId().ToString()));
        pawnInfo.Set("teamId", Napi::Number::New(env, PlayerState->GetTeamIndex()));
        
        auto Location = Pawn->GetActorLocation();
        Napi::Object loc = Napi::Object::New(env);
        loc.Set("x", Napi::Number::New(env, Location.X));
        loc.Set("y", Napi::Number::New(env, Location.Y));
        loc.Set("z", Napi::Number::New(env, Location.Z));
        pawnInfo.Set("location", loc);
        
        pawnInfo.Set("health", Napi::Number::New(env, Pawn->GetHealth()));
        pawnInfo.Set("maxHealth", Napi::Number::New(env, Pawn->GetMaxHealth()));
        pawnInfo.Set("shield", Napi::Number::New(env, Pawn->GetShield()));
        pawnInfo.Set("isAlive", Napi::Boolean::New(env, !Pawn->IsDBNO() && Pawn->GetHealth() > 0));
        
        return pawnInfo;
#else
        return env.Undefined();
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FWorld_GetPawnById", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FWorldBinding::GetPawnByUsername(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        return env.Undefined();
    }
    
    try {
        std::string targetUsername = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        if (!GameState) return env.Undefined();
        
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
                
                Napi::Object pawnInfo = Napi::Object::New(env);
                pawnInfo.Set("id", Napi::String::New(env, std::to_string(reinterpret_cast<uintptr_t>(Pawn))));
                pawnInfo.Set("username", Napi::String::New(env, playerName));
                pawnInfo.Set("accountId", Napi::String::New(env, PlayerState->GetUniqueId().ToString()));
                pawnInfo.Set("teamId", Napi::Number::New(env, PlayerState->GetTeamIndex()));
                
                auto Location = Pawn->GetActorLocation();
                Napi::Object loc = Napi::Object::New(env);
                loc.Set("x", Napi::Number::New(env, Location.X));
                loc.Set("y", Napi::Number::New(env, Location.Y));
                loc.Set("z", Napi::Number::New(env, Location.Z));
                pawnInfo.Set("location", loc);
                
                pawnInfo.Set("health", Napi::Number::New(env, Pawn->GetHealth()));
                pawnInfo.Set("maxHealth", Napi::Number::New(env, Pawn->GetMaxHealth()));
                pawnInfo.Set("shield", Napi::Number::New(env, Pawn->GetShield()));
                pawnInfo.Set("isAlive", Napi::Boolean::New(env, !Pawn->IsDBNO() && Pawn->GetHealth() > 0));
                
                return pawnInfo;
            }
        }
#endif
        
        return env.Undefined();
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FWorld_GetPawnByUsername", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FWorldBinding::SpawnActor(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement actor spawning
    return env.Undefined();
}

Napi::Value FWorldBinding::DestroyActor(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement actor destruction
    return Napi::Boolean::New(env, false);
}
