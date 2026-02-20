#pragma once

/**
 * Flare - Error handling for native binding exceptions
 * 
 * When a C++ binding function throws an exception, it's wrapped
 * in a Flare error message that can be caught by JavaScript.
 */

#include <napi.h>
#include <string>
#include <random>

class FlareBinding {
public:
    static void Init(Napi::Env env, Napi::Object exports);
    
    /**
     * Create a Flare error message string
     * This creates a JSON-serialized Flare object that can be thrown
     * and caught by the module's ErrorHandler.
     */
    static std::string CreateFlareMessage(
        const std::string& functionName,
        const std::string& reason,
        const std::string& severity = "MEDIUM"
    );
    
    /**
     * Generate a unique spark ID for error tracking
     */
    static std::string GenerateSparkId();
    
private:
    // JavaScript API
    static Napi::Value Parse(const Napi::CallbackInfo& info);
    static Napi::Value Create(const Napi::CallbackInfo& info);
};
