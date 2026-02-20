/**
 * Flare Native Binding Implementation
 * 
 * Provides error handling utilities for catching C++ exceptions
 * and converting them to JavaScript-compatible Flare objects.
 */

#include "FlareBinding.h"
#include <sstream>

void FlareBinding::Init(Napi::Env env, Napi::Object exports) {
    Napi::Object flare = Napi::Object::New(env);
    
    flare.Set("parse", Napi::Function::New(env, Parse));
    flare.Set("create", Napi::Function::New(env, Create));
    
    exports.Set("Flare", flare);
}

std::string FlareBinding::GenerateSparkId() {
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::string result;
    result.reserve(43);
    for (int i = 0; i < 43; i++) {
        result += chars[dis(gen)];
    }
    return result;
}

std::string FlareBinding::CreateFlareMessage(
    const std::string& functionName,
    const std::string& reason,
    const std::string& severity
) {
    std::stringstream ss;
    ss << "{";
    ss << "\"spark_id\":\"" << GenerateSparkId() << "\",";
    ss << "\"severity\":\"" << severity << "\",";
    ss << "\"attention\":{\"trace\":[{\"function\":\"" << functionName << "\",\"native\":true}]},";
    ss << "\"did_you_know\":\"" << reason << "\"";
    ss << "}";
    return ss.str();
}

Napi::Value FlareBinding::Parse(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        return env.Undefined();
    }
    
    // If it's already an object, return utilities for it
    if (info[0].IsObject()) {
        Napi::Object flare = info[0].As<Napi::Object>();
        Napi::Object utils = Napi::Object::New(env);
        
        // Create utility methods
        utils.Set("getReason", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
            Napi::Env env = info.Env();
            if (info.Length() > 0 && info[0].IsObject()) {
                Napi::Object flare = info[0].As<Napi::Object>();
                if (flare.Has("did_you_know")) {
                    return flare.Get("did_you_know");
                }
            }
            return Napi::String::New(env, "Unknown error");
        }));
        
        utils.Set("getSeverity", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
            Napi::Env env = info.Env();
            if (info.Length() > 0 && info[0].IsObject()) {
                Napi::Object flare = info[0].As<Napi::Object>();
                if (flare.Has("severity")) {
                    return flare.Get("severity");
                }
            }
            return Napi::String::New(env, "UNKNOWN");
        }));
        
        utils.Set("getSparkId", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
            Napi::Env env = info.Env();
            if (info.Length() > 0 && info[0].IsObject()) {
                Napi::Object flare = info[0].As<Napi::Object>();
                if (flare.Has("spark_id")) {
                    return flare.Get("spark_id");
                }
            }
            return Napi::String::New(env, "");
        }));
        
        utils.Set("isCritical", Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
            Napi::Env env = info.Env();
            if (info.Length() > 0 && info[0].IsObject()) {
                Napi::Object flare = info[0].As<Napi::Object>();
                if (flare.Has("severity")) {
                    std::string sev = flare.Get("severity").As<Napi::String>().Utf8Value();
                    return Napi::Boolean::New(env, sev == "CRITICAL");
                }
            }
            return Napi::Boolean::New(env, false);
        }));
        
        // Store the flare reference
        utils.Set("_flare", flare);
        
        return utils;
    }
    
    return env.Undefined();
}

Napi::Value FlareBinding::Create(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    std::string reason = info.Length() > 0 && info[0].IsString() 
        ? info[0].As<Napi::String>().Utf8Value() 
        : "Unknown error";
    
    std::string severity = info.Length() > 1 && info[1].IsString()
        ? info[1].As<Napi::String>().Utf8Value()
        : "MEDIUM";
    
    Napi::Object flare = Napi::Object::New(env);
    flare.Set("spark_id", Napi::String::New(env, GenerateSparkId()));
    flare.Set("severity", Napi::String::New(env, severity));
    flare.Set("did_you_know", Napi::String::New(env, reason));
    
    Napi::Object attention = Napi::Object::New(env);
    attention.Set("trace", Napi::Array::New(env));
    flare.Set("attention", attention);
    
    return flare;
}
