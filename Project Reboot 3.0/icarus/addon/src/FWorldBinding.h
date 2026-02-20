#pragma once

/**
 * FWorld Native Binding
 * 
 * Exposes FWorld functionality to JavaScript via N-API.
 */

#include <napi.h>

class FWorldBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
private:
    // Static methods exposed to JavaScript
    static Napi::Value GetProperty(const Napi::CallbackInfo& info);
    static Napi::Value SetProperty(const Napi::CallbackInfo& info);
    static Napi::Value GetPawnList(const Napi::CallbackInfo& info);
    static Napi::Value GetPawnById(const Napi::CallbackInfo& info);
    static Napi::Value GetPawnByUsername(const Napi::CallbackInfo& info);
    static Napi::Value SpawnActor(const Napi::CallbackInfo& info);
    static Napi::Value DestroyActor(const Napi::CallbackInfo& info);
};
