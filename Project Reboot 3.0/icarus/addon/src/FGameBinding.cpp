/**
 * FGame Native Binding Implementation
 */

#include "FGameBinding.h"
#include "FlareBinding.h"

#ifdef PROJECT_REBOOT_BUILD
#include "reboot.h"
#include "FortGameModeAthena.h"
#include "FortGameStateAthena.h"
#endif

void FGameBinding::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object fgame = Napi::Object::New(env);
    
    fgame.Set("startMatch", Napi::Function::New(env, StartMatch));
    fgame.Set("endMatch", Napi::Function::New(env, EndMatch));
    fgame.Set("getMatchState", Napi::Function::New(env, GetMatchState));
    fgame.Set("setPlayersLeft", Napi::Function::New(env, SetPlayersLeft));
    fgame.Set("startAircraft", Napi::Function::New(env, StartAircraft));
    fgame.Set("startSafeZone", Napi::Function::New(env, StartSafeZone));
    
    exports.Set("FGame", fgame);
}

Napi::Value FGameBinding::StartMatch(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
        if (!GameMode) {
            return Napi::Boolean::New(env, false);
        }
        
        GameMode->StartAircraftPhase();
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FGame_StartMatch", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FGameBinding::EndMatch(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
        if (!GameMode) {
            return Napi::Boolean::New(env, false);
        }
        
        GameMode->EndGame();
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FGame_EndMatch", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FGameBinding::GetMatchState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        if (!GameState) {
            return Napi::String::New(env, "unknown");
        }
        
        auto phase = GameState->GetGamePhase();
        switch (phase) {
            case EAthenaGamePhase::None: return Napi::String::New(env, "none");
            case EAthenaGamePhase::Setup: return Napi::String::New(env, "setup");
            case EAthenaGamePhase::Warmup: return Napi::String::New(env, "warmup");
            case EAthenaGamePhase::Aircraft: return Napi::String::New(env, "aircraft");
            case EAthenaGamePhase::SafeZones: return Napi::String::New(env, "safezones");
            case EAthenaGamePhase::EndGame: return Napi::String::New(env, "endgame");
            default: return Napi::String::New(env, "unknown");
        }
#else
        return Napi::String::New(env, "setup");
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FGame_GetMatchState", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::String::New(env, "unknown");
    }
}

Napi::Value FGameBinding::SetPlayersLeft(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        int count = info[0].As<Napi::Number>().Int32Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        if (!GameState) {
            return Napi::Boolean::New(env, false);
        }
        
        GameState->GetPlayersLeft() = count;
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FGame_SetPlayersLeft", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FGameBinding::StartAircraft(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
        if (!GameMode) {
            return Napi::Boolean::New(env, false);
        }
        
        GameMode->StartAircraftPhase();
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FGame_StartAircraft", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FGameBinding::StartSafeZone(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
        if (!GameMode) {
            return Napi::Boolean::New(env, false);
        }
        
        GameMode->StartSafeZonePhase();
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FGame_StartSafeZone", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}
