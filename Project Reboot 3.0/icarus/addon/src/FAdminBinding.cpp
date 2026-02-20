/**
 * FAdmin Native Binding Implementation
 */

#include "FAdminBinding.h"
#include "FlareBinding.h"

#ifdef PROJECT_REBOOT_BUILD
#include "reboot.h"
#include "FortGameStateAthena.h"
#include "FortPlayerStateAthena.h"
#include "FortPlayerControllerAthena.h"
#include "FortPlayerPawn.h"
#endif

void FAdminBinding::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object fadmin = Napi::Object::New(env);
    
    fadmin.Set("kick", Napi::Function::New(env, Kick));
    fadmin.Set("ban", Napi::Function::New(env, Ban));
    fadmin.Set("unban", Napi::Function::New(env, Unban));
    fadmin.Set("broadcast", Napi::Function::New(env, Broadcast));
    fadmin.Set("getPlayers", Napi::Function::New(env, GetPlayers));
    fadmin.Set("isOperator", Napi::Function::New(env, IsOperator));
    fadmin.Set("setOperator", Napi::Function::New(env, SetOperator));
    
    exports.Set("FAdmin", fadmin);
}

Napi::Value FAdminBinding::Kick(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string playerId = info[0].As<Napi::String>().Utf8Value();
        std::string reason = info.Length() > 1 ? info[1].As<Napi::String>().Utf8Value() : "Kicked by administrator";
        
#ifdef PROJECT_REBOOT_BUILD
        uintptr_t pawnPtr = std::stoull(playerId);
        auto Pawn = reinterpret_cast<AFortPlayerPawn*>(pawnPtr);
        
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        auto Controller = Cast<AFortPlayerControllerAthena>(Pawn->GetController());
        if (!Controller) {
            return Napi::Boolean::New(env, false);
        }
        
        Controller->ClientReturnToMainMenuWithTextReason(FText::FromString(FString(reason.c_str())));
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FAdmin_Kick", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FAdminBinding::Ban(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement ban with duration
    return Napi::Boolean::New(env, false);
}

Napi::Value FAdminBinding::Unban(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement
    return Napi::Boolean::New(env, false);
}

Napi::Value FAdminBinding::Broadcast(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string message = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        if (!GameState) {
            return Napi::Boolean::New(env, false);
        }
        
        auto& PlayerArray = GameState->GetPlayerArray();
        
        for (int i = 0; i < PlayerArray.Num(); i++) {
            auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerArray[i]);
            if (!PlayerState) continue;
            
            auto Controller = Cast<AFortPlayerControllerAthena>(PlayerState->GetOwner());
            if (!Controller) continue;
            
            Controller->ClientMessage(FString(message.c_str()));
        }
        
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FAdmin_Broadcast", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FAdminBinding::GetPlayers(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
        Napi::Array result = Napi::Array::New(env);
        
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
            
            Napi::Object playerInfo = Napi::Object::New(env);
            playerInfo.Set("id", Napi::String::New(env, Pawn ? std::to_string(reinterpret_cast<uintptr_t>(Pawn)) : ""));
            playerInfo.Set("username", Napi::String::New(env, PlayerState->GetPlayerName().ToString()));
            playerInfo.Set("teamId", Napi::Number::New(env, PlayerState->GetTeamIndex()));
            playerInfo.Set("isAlive", Napi::Boolean::New(env, Pawn && Pawn->GetHealth() > 0));
            
            result.Set(index++, playerInfo);
        }
#endif
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FAdmin_GetPlayers", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Array::New(env);
    }
}

Napi::Value FAdminBinding::IsOperator(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement operator check
    return Napi::Boolean::New(env, false);
}

Napi::Value FAdminBinding::SetOperator(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement
    return Napi::Boolean::New(env, false);
}
