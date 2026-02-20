#pragma once

/**
 * FPawn Native Binding
 * 
 * Exposes FPawn functionality to JavaScript via N-API.
 */

#include <napi.h>

class FPawnBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
private:
    // Static methods exposed to JavaScript
    static Napi::Value Move(const Napi::CallbackInfo& info);
    static Napi::Value Teleport(const Napi::CallbackInfo& info);
    static Napi::Value SetRotation(const Napi::CallbackInfo& info);
    static Napi::Value GetLocation(const Napi::CallbackInfo& info);
    static Napi::Value GetRotation(const Napi::CallbackInfo& info);
    static Napi::Value SetCostume(const Napi::CallbackInfo& info);
    static Napi::Value GetCosmetics(const Napi::CallbackInfo& info);
    static Napi::Value Kill(const Napi::CallbackInfo& info);
    static Napi::Value Revive(const Napi::CallbackInfo& info);
    static Napi::Value ApplyDamage(const Napi::CallbackInfo& info);
    static Napi::Value SetHealth(const Napi::CallbackInfo& info);
    static Napi::Value SetShield(const Napi::CallbackInfo& info);
    static Napi::Value GetStats(const Napi::CallbackInfo& info);
    static Napi::Value GiveItem(const Napi::CallbackInfo& info);
    static Napi::Value RemoveItem(const Napi::CallbackInfo& info);
    static Napi::Value GetInventory(const Napi::CallbackInfo& info);
    static Napi::Value PlayEmote(const Napi::CallbackInfo& info);
    static Napi::Value SendMessage(const Napi::CallbackInfo& info);
    static Napi::Value GetState(const Napi::CallbackInfo& info);
};
