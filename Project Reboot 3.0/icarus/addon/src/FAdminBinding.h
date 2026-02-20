#pragma once

#include <napi.h>

class FAdminBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
private:
    static Napi::Value Kick(const Napi::CallbackInfo& info);
    static Napi::Value Ban(const Napi::CallbackInfo& info);
    static Napi::Value Unban(const Napi::CallbackInfo& info);
    static Napi::Value Broadcast(const Napi::CallbackInfo& info);
    static Napi::Value GetPlayers(const Napi::CallbackInfo& info);
    static Napi::Value IsOperator(const Napi::CallbackInfo& info);
    static Napi::Value SetOperator(const Napi::CallbackInfo& info);
};
