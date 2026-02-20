/**
 * Icarus Native Add-on Entry Point
 * 
 * This is the main entry point for the Node.js native add-on.
 * It registers all the native binding modules and exports them to JavaScript.
 * 
 * Usage in JavaScript/TypeScript:
 * ```ts
 * const icarus = require('@trail-blaze/icarus-addon');
 * // or
 * import * as icarus from '@trail-blaze/icarus-addon';
 * 
 * icarus.FWorld.getProperty('state');
 * ```
 */

#include <napi.h>
#include "FWorldBinding.h"
#include "FPawnBinding.h"
#include "FGameBinding.h"
#include "FStormBinding.h"
#include "FInventoryBinding.h"
#include "FAdminBinding.h"
#include "FBotsBinding.h"
#include "FlareBinding.h"

/**
 * Initialize the native add-on
 * This function is called when the module is loaded by Node.js
 */
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Initialize all binding modules
    FWorldBinding::Init(env, exports);
    FPawnBinding::Init(env, exports);
    FGameBinding::Init(env, exports);
    FStormBinding::Init(env, exports);
    FInventoryBinding::Init(env, exports);
    FAdminBinding::Init(env, exports);
    FBotsBinding::Init(env, exports);
    FlareBinding::Init(env, exports);
    
    // Export version info
    exports.Set("version", Napi::String::New(env, "1.0.0"));
    exports.Set("name", Napi::String::New(env, "icarus"));
    
    return exports;
}

// Register the module with Node.js
NODE_API_MODULE(icarus, Init)
