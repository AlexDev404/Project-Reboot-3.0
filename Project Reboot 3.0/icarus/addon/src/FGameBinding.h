#pragma once

#include <napi.h>

class FGameBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
private:
    static Napi::Value StartMatch(const Napi::CallbackInfo& info);
    static Napi::Value EndMatch(const Napi::CallbackInfo& info);
    static Napi::Value GetMatchState(const Napi::CallbackInfo& info);
    static Napi::Value SetPlayersLeft(const Napi::CallbackInfo& info);
    static Napi::Value StartAircraft(const Napi::CallbackInfo& info);
    static Napi::Value StartSafeZone(const Napi::CallbackInfo& info);
};
