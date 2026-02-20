/**
 * FStorm Native Binding Implementation
 */

#include "FStormBinding.h"
#include "FlareBinding.h"

#ifdef PROJECT_REBOOT_BUILD
#include "reboot.h"
#include "FortGameStateAthena.h"
#include "FortSafeZoneIndicator.h"
#endif

void FStormBinding::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object fstorm = Napi::Object::New(env);
    
    fstorm.Set("getCurrentPhase", Napi::Function::New(env, GetCurrentPhase));
    fstorm.Set("pause", Napi::Function::New(env, Pause));
    fstorm.Set("resume", Napi::Function::New(env, Resume));
    fstorm.Set("nextPhase", Napi::Function::New(env, NextPhase));
    fstorm.Set("skipToPhase", Napi::Function::New(env, SkipToPhase));
    fstorm.Set("getPhaseConfig", Napi::Function::New(env, GetPhaseConfig));
    fstorm.Set("setPhaseConfig", Napi::Function::New(env, SetPhaseConfig));
    
    exports.Set("FStorm", fstorm);
}

Napi::Value FStormBinding::GetCurrentPhase(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        // Get current storm phase from safe zone indicator
        return Napi::Number::New(env, 0);
#else
        return Napi::Number::New(env, 0);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FStorm_GetCurrentPhase", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }
}

Napi::Value FStormBinding::Pause(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        if (!GameState) {
            return Napi::Boolean::New(env, false);
        }
        
        GameState->IsSafeZonePaused() = true;
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FStorm_Pause", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FStormBinding::Resume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
#ifdef PROJECT_REBOOT_BUILD
        auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
        if (!GameState) {
            return Napi::Boolean::New(env, false);
        }
        
        GameState->IsSafeZonePaused() = false;
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FStorm_Resume", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FStormBinding::NextPhase(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
        // TODO: Implement next phase
        return Napi::Boolean::New(env, true);
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FStorm_NextPhase", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FStormBinding::SkipToPhase(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        int phase = info[0].As<Napi::Number>().Int32Value();
        
        // TODO: Implement skip to phase
        return Napi::Boolean::New(env, true);
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FStorm_SkipToPhase", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FStormBinding::GetPhaseConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        int phase = info[0].As<Napi::Number>().Int32Value();
        
        Napi::Object result = Napi::Object::New(env);
        result.Set("shrinkTime", Napi::Number::New(env, 120));
        result.Set("holdTime", Napi::Number::New(env, 60));
        result.Set("damagePerSecond", Napi::Number::New(env, 1));
        result.Set("radius", Napi::Number::New(env, 10000));
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FStorm_GetPhaseConfig", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FStormBinding::SetPhaseConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        int phase = info[0].As<Napi::Number>().Int32Value();
        Napi::Object config = info[1].As<Napi::Object>();
        
        // TODO: Apply phase config
        return Napi::Boolean::New(env, true);
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FStorm_SetPhaseConfig", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}
