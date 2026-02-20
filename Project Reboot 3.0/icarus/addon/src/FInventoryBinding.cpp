/**
 * FInventory Native Binding Implementation
 */

#include "FInventoryBinding.h"
#include "FlareBinding.h"

#ifdef PROJECT_REBOOT_BUILD
#include "reboot.h"
#include "FortPlayerPawn.h"
#include "FortPlayerControllerAthena.h"
#include "FortInventory.h"
#include "FortItemDefinition.h"
#endif

void FInventoryBinding::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object finv = Napi::Object::New(env);
    
    finv.Set("giveItem", Napi::Function::New(env, GiveItem));
    finv.Set("removeItem", Napi::Function::New(env, RemoveItem));
    finv.Set("clearInventory", Napi::Function::New(env, ClearInventory));
    finv.Set("getInventory", Napi::Function::New(env, GetInventory));
    finv.Set("giveResources", Napi::Function::New(env, GiveResources));
    finv.Set("setResources", Napi::Function::New(env, SetResources));
    finv.Set("giveAmmo", Napi::Function::New(env, GiveAmmo));
    finv.Set("maxAmmo", Napi::Function::New(env, MaxAmmo));
    
    exports.Set("FInventory", finv);
}

Napi::Value FInventoryBinding::GiveItem(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected at least 2 arguments: playerId, itemId").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string playerId = info[0].As<Napi::String>().Utf8Value();
        std::string itemId = info[1].As<Napi::String>().Utf8Value();
        int count = info.Length() > 2 ? info[2].As<Napi::Number>().Int32Value() : 1;
        
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
        
        auto ItemDef = LoadObject<UFortItemDefinition>(itemId.c_str());
        if (!ItemDef) {
            return Napi::Boolean::New(env, false);
        }
        
        Controller->AddItemToInventory(ItemDef, EFortQuickBars::Primary, 0, count);
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FInventory_GiveItem", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FInventoryBinding::RemoveItem(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement
    return Napi::Boolean::New(env, false);
}

Napi::Value FInventoryBinding::ClearInventory(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement
    return Napi::Boolean::New(env, false);
}

Napi::Value FInventoryBinding::GetInventory(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        Napi::Array result = Napi::Array::New(env);
        // TODO: Get inventory items
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FInventory_GetInventory", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FInventoryBinding::GiveResources(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 4) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string playerId = info[0].As<Napi::String>().Utf8Value();
        int wood = info[1].As<Napi::Number>().Int32Value();
        int stone = info[2].As<Napi::Number>().Int32Value();
        int metal = info[3].As<Napi::Number>().Int32Value();
        
        // TODO: Give resources
        return Napi::Boolean::New(env, true);
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FInventory_GiveResources", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FInventoryBinding::SetResources(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement
    return Napi::Boolean::New(env, false);
}

Napi::Value FInventoryBinding::GiveAmmo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement
    return Napi::Boolean::New(env, false);
}

Napi::Value FInventoryBinding::MaxAmmo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement
    return Napi::Boolean::New(env, false);
}
