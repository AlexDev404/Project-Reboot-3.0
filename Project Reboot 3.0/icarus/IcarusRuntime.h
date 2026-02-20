#pragma once

/**
 * Icarus JavaScript Engine Integration
 * 
 * This header provides the interface for integrating a JavaScript engine
 * (QuickJS) with Project Reboot, enabling TypeScript/JavaScript modules
 * to control the game through native bindings.
 * 
 * The binding layer exposes C++ functions to JavaScript, allowing:
 * - Pawn manipulation (move, teleport, set health, etc.)
 * - World state queries and modifications
 * - Game state control (start match, end match, storm phases)
 * - Inventory management
 * - Bot spawning and control
 * - And more...
 */

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

// Forward declarations
class AFortPlayerPawn;
class AFortPlayerControllerAthena;
class AFortPlayerStateAthena;
class AFortGameModeAthena;
class AFortGameStateAthena;

namespace Icarus {

/**
 * JavaScript value wrapper for type-safe interactions
 */
struct JSValue {
    enum class Type {
        Undefined,
        Null,
        Boolean,
        Number,
        String,
        Object,
        Array,
        Function
    };
    
    Type type = Type::Undefined;
    
    // Value storage
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JSValue> arrayValue;
    std::unordered_map<std::string, JSValue> objectValue;
    
    // Constructors
    JSValue() : type(Type::Undefined) {}
    JSValue(std::nullptr_t) : type(Type::Null) {}
    JSValue(bool v) : type(Type::Boolean), boolValue(v) {}
    JSValue(int v) : type(Type::Number), numberValue(static_cast<double>(v)) {}
    JSValue(double v) : type(Type::Number), numberValue(v) {}
    JSValue(const std::string& v) : type(Type::String), stringValue(v) {}
    JSValue(const char* v) : type(Type::String), stringValue(v) {}
    
    // Type checkers
    bool isUndefined() const { return type == Type::Undefined; }
    bool isNull() const { return type == Type::Null; }
    bool isBoolean() const { return type == Type::Boolean; }
    bool isNumber() const { return type == Type::Number; }
    bool isString() const { return type == Type::String; }
    bool isObject() const { return type == Type::Object; }
    bool isArray() const { return type == Type::Array; }
    
    // Value getters
    bool asBool() const { return boolValue; }
    double asNumber() const { return numberValue; }
    int asInt() const { return static_cast<int>(numberValue); }
    const std::string& asString() const { return stringValue; }
    
    // Static constructors for complex types
    static JSValue Object() {
        JSValue v;
        v.type = Type::Object;
        return v;
    }
    
    static JSValue Array() {
        JSValue v;
        v.type = Type::Array;
        return v;
    }
};

/**
 * Native function signature for bindings
 */
using NativeFunction = std::function<JSValue(const std::vector<JSValue>&)>;

/**
 * Flare - Error object passed to module ErrorHandler
 */
struct Flare {
    std::string sparkId;
    std::string severity;  // "LOW", "MEDIUM", "HIGH", "CRITICAL"
    std::string reason;
    std::string nativeFunction;
    std::string nativeTrace;
    
    JSValue toJSValue() const;
    static std::string generateSparkId();
};

/**
 * Module definition
 */
struct Module {
    std::string name;
    std::string version;
    std::string source;
    bool isLoaded = false;
    bool hasError = false;
    Flare lastError;
};

/**
 * JavaScript Runtime
 * 
 * Manages the JavaScript engine and provides the binding interface.
 */
class Runtime {
public:
    Runtime();
    ~Runtime();
    
    /**
     * Initialize the JavaScript runtime
     * @return true if successful
     */
    bool initialize();
    
    /**
     * Shutdown the runtime and cleanup resources
     */
    void shutdown();
    
    /**
     * Register a native function that can be called from JavaScript
     * @param name Function name (e.g., "FWorld_GetProperty")
     * @param fn The native function implementation
     */
    void registerFunction(const std::string& name, NativeFunction fn);
    
    /**
     * Execute JavaScript code
     * @param code JavaScript source code
     * @param filename Optional filename for error messages
     * @return Result value
     */
    JSValue evaluate(const std::string& code, const std::string& filename = "<eval>");
    
    /**
     * Load and execute a module
     * @param modulePath Path to the module file
     * @return true if successful
     */
    bool loadModule(const std::string& modulePath);
    
    /**
     * Call a module's ThreadStart function
     * @param args Arguments to pass
     * @return Return value (0 = success)
     */
    int callThreadStart(const std::vector<JSValue>& args);
    
    /**
     * Call a module's ThreadExit function
     * @param args Arguments to pass
     * @return Return value (0 = success)
     */
    int callThreadExit(const std::vector<JSValue>& args);
    
    /**
     * Call a module's ErrorHandler with a Flare
     * @param flare The error to handle
     * @return Return value from handler
     */
    int callErrorHandler(const Flare& flare);
    
    /**
     * Get the last error that occurred
     */
    const Flare& getLastError() const { return m_lastError; }
    
    /**
     * Check if runtime is initialized
     */
    bool isInitialized() const { return m_initialized; }
    
private:
    bool m_initialized = false;
    Flare m_lastError;
    std::unordered_map<std::string, NativeFunction> m_bindings;
    
    // Implementation details would include the actual JS engine context
    void* m_context = nullptr;  // JSContext* in actual implementation
    void* m_runtime = nullptr;  // JSRuntime* in actual implementation
};

/**
 * Global runtime instance
 */
Runtime& getRuntime();

/**
 * Initialize all native bindings
 * This registers all the FWorld, FPawn, FGame, etc. functions
 */
void initializeBindings(Runtime& runtime);

} // namespace Icarus
