#pragma once

#include <napi.h>

class FInventoryBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
private:
    static Napi::Value GiveItem(const Napi::CallbackInfo& info);
    static Napi::Value RemoveItem(const Napi::CallbackInfo& info);
    static Napi::Value ClearInventory(const Napi::CallbackInfo& info);
    static Napi::Value GetInventory(const Napi::CallbackInfo& info);
    static Napi::Value GiveResources(const Napi::CallbackInfo& info);
    static Napi::Value SetResources(const Napi::CallbackInfo& info);
    static Napi::Value GiveAmmo(const Napi::CallbackInfo& info);
    static Napi::Value MaxAmmo(const Napi::CallbackInfo& info);
};
