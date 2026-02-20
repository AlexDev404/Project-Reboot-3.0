#pragma once

#include <napi.h>

class FStormBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
private:
    static Napi::Value GetCurrentPhase(const Napi::CallbackInfo& info);
    static Napi::Value Pause(const Napi::CallbackInfo& info);
    static Napi::Value Resume(const Napi::CallbackInfo& info);
    static Napi::Value NextPhase(const Napi::CallbackInfo& info);
    static Napi::Value SkipToPhase(const Napi::CallbackInfo& info);
    static Napi::Value GetPhaseConfig(const Napi::CallbackInfo& info);
    static Napi::Value SetPhaseConfig(const Napi::CallbackInfo& info);
};
