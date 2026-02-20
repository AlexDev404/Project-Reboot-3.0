/**
 * FPawn Native Binding Implementation
 * 
 * This file implements the N-API bindings for FPawn, connecting
 * Project Reboot's C++ pawn functions to JavaScript.
 */

#include "FPawnBinding.h"
#include "FlareBinding.h"

#ifdef PROJECT_REBOOT_BUILD
#include "reboot.h"
#include "FortPlayerPawn.h"
#include "FortPlayerControllerAthena.h"
#include "FortInventory.h"
#endif

// Helper function to get pawn from ID string
#ifdef PROJECT_REBOOT_BUILD
static AFortPlayerPawn* GetPawnFromId(const std::string& id) {
    try {
        uintptr_t pawnPtr = std::stoull(id);
        return reinterpret_cast<AFortPlayerPawn*>(pawnPtr);
    } catch (...) {
        return nullptr;
    }
}
#endif

void FPawnBinding::Init(Napi::Env env, Napi::Object exports) {
    // Create FPawn object with static methods
    Napi::Object fpawn = Napi::Object::New(env);
    
    // Register methods
    fpawn.Set("move", Napi::Function::New(env, Move));
    fpawn.Set("teleport", Napi::Function::New(env, Teleport));
    fpawn.Set("setRotation", Napi::Function::New(env, SetRotation));
    fpawn.Set("getLocation", Napi::Function::New(env, GetLocation));
    fpawn.Set("getRotation", Napi::Function::New(env, GetRotation));
    fpawn.Set("setCostume", Napi::Function::New(env, SetCostume));
    fpawn.Set("getCosmetics", Napi::Function::New(env, GetCosmetics));
    fpawn.Set("kill", Napi::Function::New(env, Kill));
    fpawn.Set("revive", Napi::Function::New(env, Revive));
    fpawn.Set("applyDamage", Napi::Function::New(env, ApplyDamage));
    fpawn.Set("setHealth", Napi::Function::New(env, SetHealth));
    fpawn.Set("setShield", Napi::Function::New(env, SetShield));
    fpawn.Set("getStats", Napi::Function::New(env, GetStats));
    fpawn.Set("giveItem", Napi::Function::New(env, GiveItem));
    fpawn.Set("removeItem", Napi::Function::New(env, RemoveItem));
    fpawn.Set("getInventory", Napi::Function::New(env, GetInventory));
    fpawn.Set("playEmote", Napi::Function::New(env, PlayEmote));
    fpawn.Set("sendMessage", Napi::Function::New(env, SendMessage));
    fpawn.Set("getState", Napi::Function::New(env, GetState));
    
    exports.Set("FPawn", fpawn);
}

Napi::Value FPawnBinding::Move(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Expected 4 arguments: pawnId, x, y, z").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        double x = info[1].As<Napi::Number>().DoubleValue();
        double y = info[2].As<Napi::Number>().DoubleValue();
        double z = info[3].As<Napi::Number>().DoubleValue();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        FVector NewLocation;
        NewLocation.X = x;
        NewLocation.Y = y;
        NewLocation.Z = z;
        
        Pawn->TeleportTo(NewLocation, Pawn->GetActorRotation());
        return Napi::Boolean::New(env, true);
#else
        // Mock mode
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_Move", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::Teleport(const Napi::CallbackInfo& info) {
    // Same as Move
    return Move(info);
}

Napi::Value FPawnBinding::SetRotation(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 4) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        double pitch = info[1].As<Napi::Number>().DoubleValue();
        double yaw = info[2].As<Napi::Number>().DoubleValue();
        double roll = info[3].As<Napi::Number>().DoubleValue();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        FRotator NewRotation;
        NewRotation.Pitch = pitch;
        NewRotation.Yaw = yaw;
        NewRotation.Roll = roll;
        
        Pawn->SetActorRotation(NewRotation);
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_SetRotation", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::GetLocation(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return env.Undefined();
        }
        
        auto Location = Pawn->GetActorLocation();
        
        Napi::Object result = Napi::Object::New(env);
        result.Set("x", Napi::Number::New(env, Location.X));
        result.Set("y", Napi::Number::New(env, Location.Y));
        result.Set("z", Napi::Number::New(env, Location.Z));
        
        return result;
#else
        Napi::Object result = Napi::Object::New(env);
        result.Set("x", Napi::Number::New(env, 0));
        result.Set("y", Napi::Number::New(env, 0));
        result.Set("z", Napi::Number::New(env, 0));
        return result;
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_GetLocation", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FPawnBinding::GetRotation(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return env.Undefined();
        }
        
        auto Rotation = Pawn->GetActorRotation();
        
        Napi::Object result = Napi::Object::New(env);
        result.Set("pitch", Napi::Number::New(env, Rotation.Pitch));
        result.Set("yaw", Napi::Number::New(env, Rotation.Yaw));
        result.Set("roll", Napi::Number::New(env, Rotation.Roll));
        
        return result;
#else
        Napi::Object result = Napi::Object::New(env);
        result.Set("pitch", Napi::Number::New(env, 0));
        result.Set("yaw", Napi::Number::New(env, 0));
        result.Set("roll", Napi::Number::New(env, 0));
        return result;
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_GetRotation", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FPawnBinding::SetCostume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        std::string costumeId = info[1].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        // Apply costume - this would need to interface with Fortnite's cosmetic system
        // The actual implementation depends on Project Reboot's cosmetic handling
        
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_SetCostume", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::GetCosmetics(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        
        Napi::Object result = Napi::Object::New(env);
        result.Set("character", Napi::String::New(env, ""));
        result.Set("backpack", Napi::String::New(env, ""));
        result.Set("pickaxe", Napi::String::New(env, ""));
        result.Set("glider", Napi::String::New(env, ""));
        result.Set("contrail", Napi::String::New(env, ""));
        
        // TODO: Get actual cosmetics from pawn
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_GetCosmetics", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FPawnBinding::Kill(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        // Kill the pawn
        Pawn->SetHealth(0);
        
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_Kill", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::Revive(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        // Revive the pawn
        Pawn->SetHealth(Pawn->GetMaxHealth());
        
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_Revive", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::ApplyDamage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        double damage = info[1].As<Napi::Number>().DoubleValue();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        float currentHealth = Pawn->GetHealth();
        float newHealth = currentHealth - damage;
        Pawn->SetHealth(newHealth > 0 ? newHealth : 0);
        
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_ApplyDamage", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::SetHealth(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        double health = info[1].As<Napi::Number>().DoubleValue();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        Pawn->SetHealth(static_cast<float>(health));
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_SetHealth", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::SetShield(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        double shield = info[1].As<Napi::Number>().DoubleValue();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        Pawn->SetShield(static_cast<float>(shield));
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_SetShield", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::GetStats(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        
        Napi::Object result = Napi::Object::New(env);
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return env.Undefined();
        }
        
        result.Set("health", Napi::Number::New(env, Pawn->GetHealth()));
        result.Set("maxHealth", Napi::Number::New(env, Pawn->GetMaxHealth()));
        result.Set("shield", Napi::Number::New(env, Pawn->GetShield()));
        result.Set("maxShield", Napi::Number::New(env, 100.0));
        result.Set("isDBNO", Napi::Boolean::New(env, Pawn->IsDBNO()));
        result.Set("isAlive", Napi::Boolean::New(env, Pawn->GetHealth() > 0));
#else
        result.Set("health", Napi::Number::New(env, 100));
        result.Set("maxHealth", Napi::Number::New(env, 100));
        result.Set("shield", Napi::Number::New(env, 0));
        result.Set("maxShield", Napi::Number::New(env, 100));
        result.Set("isDBNO", Napi::Boolean::New(env, false));
        result.Set("isAlive", Napi::Boolean::New(env, true));
#endif
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_GetStats", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FPawnBinding::GiveItem(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        std::string itemId = info[1].As<Napi::String>().Utf8Value();
        int count = info.Length() > 2 ? info[2].As<Napi::Number>().Int32Value() : 1;
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        auto Controller = Cast<AFortPlayerControllerAthena>(Pawn->GetController());
        if (!Controller) {
            return Napi::Boolean::New(env, false);
        }
        
        // Load item definition and add to inventory
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
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_GiveItem", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::RemoveItem(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement item removal
    return Napi::Boolean::New(env, false);
}

Napi::Value FPawnBinding::GetInventory(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        Napi::Array result = Napi::Array::New(env);
        
        // TODO: Get inventory items from pawn
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_GetInventory", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value FPawnBinding::PlayEmote(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // TODO: Implement emote playing
    return Napi::Boolean::New(env, false);
}

Napi::Value FPawnBinding::SendMessage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        return Napi::Boolean::New(env, false);
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        std::string message = info[1].As<Napi::String>().Utf8Value();
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return Napi::Boolean::New(env, false);
        }
        
        auto Controller = Cast<AFortPlayerControllerAthena>(Pawn->GetController());
        if (!Controller) {
            return Napi::Boolean::New(env, false);
        }
        
        Controller->ClientMessage(FString(message.c_str()));
        return Napi::Boolean::New(env, true);
#else
        return Napi::Boolean::New(env, true);
#endif
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_SendMessage", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value FPawnBinding::GetState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    try {
        std::string pawnId = info[0].As<Napi::String>().Utf8Value();
        
        Napi::Object result = Napi::Object::New(env);
        
#ifdef PROJECT_REBOOT_BUILD
        auto Pawn = GetPawnFromId(pawnId);
        if (!Pawn) {
            return env.Undefined();
        }
        
        result.Set("isDBNO", Napi::Boolean::New(env, Pawn->IsDBNO()));
        result.Set("isAlive", Napi::Boolean::New(env, Pawn->GetHealth() > 0));
        result.Set("isFalling", Napi::Boolean::New(env, false)); // TODO: Check falling state
        result.Set("isInVehicle", Napi::Boolean::New(env, false)); // TODO: Check vehicle state
#else
        result.Set("isDBNO", Napi::Boolean::New(env, false));
        result.Set("isAlive", Napi::Boolean::New(env, true));
        result.Set("isFalling", Napi::Boolean::New(env, false));
        result.Set("isInVehicle", Napi::Boolean::New(env, false));
#endif
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, FlareBinding::CreateFlareMessage("FPawn_GetState", e.what(), "HIGH")).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}
