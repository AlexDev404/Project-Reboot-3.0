/**
 * Icarus Runtime Implementation
 * 
 * This file implements the JavaScript runtime that executes modules
 * and manages the binding layer between JavaScript and C++.
 * 
 * Uses QuickJS as the embedded JavaScript engine for:
 * - Small footprint
 * - No external dependencies
 * - Full ES2020 support
 * - Easy C integration
 */

#include "IcarusRuntime.h"
#include "IcarusBindings.h"
#include "../log.h"

#include <random>
#include <chrono>
#include <fstream>
#include <sstream>

// QuickJS headers would be included here in actual implementation
// #include "quickjs.h"

namespace Icarus {

// ============================================================================
// Flare Implementation
// ============================================================================

std::string Flare::generateSparkId() {
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

JSValue Flare::toJSValue() const {
    JSValue result = JSValue::Object();
    result.objectValue["spark_id"] = JSValue(sparkId);
    result.objectValue["severity"] = JSValue(severity);
    result.objectValue["did_you_know"] = JSValue(reason);
    
    // Create attention object with trace
    JSValue attention = JSValue::Object();
    JSValue trace = JSValue::Array();
    
    if (!nativeFunction.empty()) {
        JSValue traceEntry = JSValue::Object();
        traceEntry.objectValue["function"] = JSValue(nativeFunction);
        traceEntry.objectValue["native"] = JSValue(true);
        trace.arrayValue.push_back(traceEntry);
    }
    
    attention.objectValue["trace"] = trace;
    result.objectValue["attention"] = attention;
    
    return result;
}

// ============================================================================
// Runtime Implementation
// ============================================================================

// Global runtime instance
static Runtime* g_runtime = nullptr;

Runtime& getRuntime() {
    if (!g_runtime) {
        g_runtime = new Runtime();
    }
    return *g_runtime;
}

Runtime::Runtime() {
    LOG_INFO(LogIcarus, "Icarus Runtime created");
}

Runtime::~Runtime() {
    shutdown();
}

bool Runtime::initialize() {
    if (m_initialized) {
        return true;
    }
    
    LOG_INFO(LogIcarus, "Initializing Icarus JavaScript Runtime...");
    
    // In actual implementation:
    // m_runtime = JS_NewRuntime();
    // m_context = JS_NewContext(m_runtime);
    
    // For now, we'll simulate initialization
    m_initialized = true;
    
    // Register all native bindings
    Bindings::registerAllBindings(*this);
    
    // Set up the global namespace for @trail-blaze/retroflex
    setupGlobalNamespace();
    
    LOG_INFO(LogIcarus, "Icarus Runtime initialized successfully");
    
    return true;
}

void Runtime::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    LOG_INFO(LogIcarus, "Shutting down Icarus Runtime...");
    
    // In actual implementation:
    // JS_FreeContext(m_context);
    // JS_FreeRuntime(m_runtime);
    
    m_context = nullptr;
    m_runtime = nullptr;
    m_initialized = false;
    m_bindings.clear();
    
    LOG_INFO(LogIcarus, "Icarus Runtime shut down");
}

void Runtime::registerFunction(const std::string& name, NativeFunction fn) {
    m_bindings[name] = fn;
    LOG_INFO(LogIcarus, "Registered native function: {}", name);
}

JSValue Runtime::evaluate(const std::string& code, const std::string& filename) {
    if (!m_initialized) {
        m_lastError.sparkId = Flare::generateSparkId();
        m_lastError.severity = "CRITICAL";
        m_lastError.reason = "Runtime not initialized";
        return JSValue();
    }
    
    // In actual implementation with QuickJS:
    // JSValue result = JS_Eval(m_context, code.c_str(), code.length(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
    // if (JS_IsException(result)) {
    //     // Handle exception, create Flare
    // }
    
    LOG_INFO(LogIcarus, "Evaluating code from: {}", filename);
    
    return JSValue(true);  // Placeholder
}

bool Runtime::loadModule(const std::string& modulePath) {
    if (!m_initialized) {
        return false;
    }
    
    LOG_INFO(LogIcarus, "Loading module: {}", modulePath);
    
    // Read module file
    std::ifstream file(modulePath);
    if (!file.is_open()) {
        m_lastError.sparkId = Flare::generateSparkId();
        m_lastError.severity = "HIGH";
        m_lastError.reason = "Failed to open module file: " + modulePath;
        LOG_ERROR(LogIcarus, "Failed to open module: {}", modulePath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();
    file.close();
    
    // Evaluate the module code
    JSValue result = evaluate(code, modulePath);
    
    if (result.isUndefined() && !m_lastError.sparkId.empty()) {
        return false;
    }
    
    LOG_INFO(LogIcarus, "Module loaded successfully: {}", modulePath);
    return true;
}

int Runtime::callThreadStart(const std::vector<JSValue>& args) {
    if (!m_initialized) {
        return 1;
    }
    
    LOG_INFO(LogIcarus, "Calling ThreadStart...");
    
    // In actual implementation:
    // JSValue global = JS_GetGlobalObject(m_context);
    // JSValue threadStart = JS_GetPropertyStr(m_context, global, "ThreadStart");
    // JSValue result = JS_Call(m_context, threadStart, global, args.size(), jsArgs);
    
    return 0;
}

int Runtime::callThreadExit(const std::vector<JSValue>& args) {
    if (!m_initialized) {
        return 1;
    }
    
    LOG_INFO(LogIcarus, "Calling ThreadExit...");
    
    return 0;
}

int Runtime::callErrorHandler(const Flare& flare) {
    if (!m_initialized) {
        return 1;
    }
    
    LOG_WARN(LogIcarus, "Calling ErrorHandler with Flare [{}]: {}", flare.severity, flare.reason);
    
    // In actual implementation, call the module's ErrorHandler function
    // with the Flare object
    
    return 1;  // Default to error
}

// ============================================================================
// Global Namespace Setup
// ============================================================================

void Runtime::setupGlobalNamespace() {
    // This sets up the JavaScript global namespace with the SDK objects
    // that modules can import.
    //
    // In actual implementation, this would create JS objects like:
    // - globalThis.FWorld
    // - globalThis.FPawn  
    // - globalThis.FGame
    // etc.
    //
    // Each object would have methods that call our registered native functions.
    
    const char* setupCode = R"(
        // @trail-blaze/retroflex SDK setup
        
        // Helper to call native bindings with error handling
        function callNative(name, ...args) {
            try {
                return __native_call(name, ...args);
            } catch (error) {
                // Error will be caught and converted to Flare
                throw error;
            }
        }
        
        // FWorld - World state and pawn management
        globalThis.FWorld = {
            getProperty: function(name) {
                return callNative('FWorld_GetProperty', name);
            },
            setProperty: function(name, value) {
                return callNative('FWorld_SetProperty', name, value);
            },
            getPawnList: function() {
                const rawList = callNative('FWorld_GetPawnList');
                // Wrap each pawn in an FPawn instance
                return (rawList || []).map(p => new FPawnInstance(p));
            },
            getPawnByUsername: function(username) {
                const pawnData = callNative('FWorld_GetPawnByUsername', username);
                return pawnData ? new FPawnInstance(pawnData) : undefined;
            },
            getPawnById: function(id) {
                return callNative('FWorld_GetPawnById', id);
            }
        };
        
        // FPawn wrapper class
        class FPawnInstance {
            constructor(data) {
                this._id = data.id;
                this._data = data;
            }
            
            get id() { return this._id; }
            get username() { return this._data.username; }
            get location() { return this._data.location; }
            get health() { return this._data.health; }
            get maxHealth() { return this._data.maxHealth; }
            get shield() { return this._data.shield; }
            get isAlive() { return this._data.isAlive; }
            get teamId() { return this._data.teamId; }
            
            move(x, y, z) {
                return callNative('FPawn_Move', this._id, x, y, z);
            }
            
            teleport(x, y, z) {
                return callNative('FPawn_Teleport', this._id, x, y, z);
            }
            
            setHealth(health) {
                return callNative('FPawn_SetHealth', this._id, health);
            }
            
            setShield(shield) {
                return callNative('FPawn_SetShield', this._id, shield);
            }
            
            kill() {
                return callNative('FPawn_Kill', this._id);
            }
            
            costume(costumeId) {
                return callNative('FPawn_SetCostume', this._id, costumeId);
            }
            
            giveItem(itemId, count = 1) {
                return callNative('FInventory_GiveItem', this._id, itemId, count);
            }
        }
        globalThis.FPawnInstance = FPawnInstance;
        
        // FGame - Match and game state control
        globalThis.FGame = {
            startMatch: function() {
                return callNative('FGame_StartMatch');
            },
            endMatch: function(options) {
                return callNative('FGame_EndMatch', options);
            },
            getMatchState: function() {
                return callNative('FGame_GetMatchState');
            },
            setPlayersLeft: function(count) {
                return callNative('FGame_SetPlayersLeft', count);
            }
        };
        
        // FStorm - Storm/safe zone management
        globalThis.FStorm = {
            getCurrentPhase: function() {
                return callNative('FStorm_GetCurrentPhase');
            },
            pause: function() {
                return callNative('FStorm_Pause');
            },
            resume: function() {
                return callNative('FStorm_Resume');
            },
            nextPhase: function() {
                return callNative('FStorm_NextPhase');
            },
            skipToPhase: function(phase) {
                return callNative('FStorm_SkipToPhase', phase);
            }
        };
        
        // FInventory - Inventory management
        globalThis.FInventory = {
            giveItem: function(playerId, itemId, count, showToast) {
                return callNative('FInventory_GiveItem', playerId, itemId, count, showToast);
            },
            removeItem: function(playerId, itemGuid, count) {
                return callNative('FInventory_RemoveItem', playerId, itemGuid, count);
            },
            clearInventory: function(playerId) {
                return callNative('FInventory_Clear', playerId);
            },
            giveResources: function(playerId, wood, stone, metal) {
                return callNative('FInventory_GiveResources', playerId, wood, stone, metal);
            }
        };
        
        // FAdmin - Server administration
        globalThis.FAdmin = {
            kick: function(playerId, reason) {
                return callNative('FAdmin_Kick', playerId, reason);
            },
            ban: function(playerId, reason, duration) {
                return callNative('FAdmin_Ban', playerId, reason, duration);
            },
            broadcast: function(message) {
                return callNative('FAdmin_Broadcast', message);
            },
            isOperator: function(playerId) {
                return callNative('FAdmin_IsOperator', playerId);
            }
        };
        
        console.log('[Icarus] SDK namespace initialized');
    )";
    
    // In actual implementation, evaluate this setup code
    // evaluate(setupCode, "<icarus-setup>");
    
    LOG_INFO(LogIcarus, "Global namespace setup complete");
}

// ============================================================================
// Module Entry Point
// ============================================================================

/**
 * Initialize Icarus - called from Project Reboot's main DLL entry
 */
void initializeIcarus() {
    auto& runtime = getRuntime();
    
    if (!runtime.initialize()) {
        LOG_ERROR(LogIcarus, "Failed to initialize Icarus runtime!");
        return;
    }
    
    LOG_INFO(LogIcarus, "Icarus initialized successfully");
    LOG_INFO(LogIcarus, "TypeScript/JavaScript modules are now available");
}

/**
 * Shutdown Icarus - called when DLL unloads
 */
void shutdownIcarus() {
    if (g_runtime) {
        g_runtime->shutdown();
        delete g_runtime;
        g_runtime = nullptr;
    }
    
    LOG_INFO(LogIcarus, "Icarus shut down");
}

/**
 * Load and execute a module file
 */
bool loadIcarusModule(const std::string& modulePath) {
    auto& runtime = getRuntime();
    
    if (!runtime.isInitialized()) {
        LOG_ERROR(LogIcarus, "Cannot load module - runtime not initialized");
        return false;
    }
    
    return runtime.loadModule(modulePath);
}

/**
 * Execute inline JavaScript code
 */
JSValue executeIcarusCode(const std::string& code) {
    auto& runtime = getRuntime();
    
    if (!runtime.isInitialized()) {
        LOG_ERROR(LogIcarus, "Cannot execute code - runtime not initialized");
        return JSValue();
    }
    
    return runtime.evaluate(code);
}

} // namespace Icarus
