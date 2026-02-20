#pragma once

#include <napi.h>

class FBotsBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
private:
    static Napi::Value Spawn(const Napi::CallbackInfo& info);
    static Napi::Value Remove(const Napi::CallbackInfo& info);
    static Napi::Value RemoveAll(const Napi::CallbackInfo& info);
    static Napi::Value GetAll(const Napi::CallbackInfo& info);
    static Napi::Value FillLobby(const Napi::CallbackInfo& info);
};
