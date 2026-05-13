#pragma once

/**
 * NodeJSWindow - Embedded Node.js Runtime Window
 * 
 * This class creates a separate console window and embeds a Node.js runtime
 * within it, allowing JavaScript/TypeScript modules to run alongside the
 * Fortnite game process via the Icarus bindings.
 * 
 * The Node.js process runs in its own thread and can load modules that
 * interact with the game through the native C++ bindings exposed by Icarus.
 */

#include <Windows.h>
#include <string>
#include <thread>
#include <atomic>

namespace NodeJS {

// Required Node.js version
constexpr const char* REQUIRED_NODE_VERSION = "v22.22.0";
constexpr int REQUIRED_NODE_MAJOR = 22;
constexpr int REQUIRED_NODE_MINOR = 22;
constexpr int REQUIRED_NODE_PATCH = 0;

/**
 * Configuration for the Node.js runtime
 */
struct NodeConfig {
    std::string title = "Icarus Node.js Runtime";
    std::string entryPoint = "icarus/main.js";
    bool showConsole = true;
    int width = 800;
    int height = 600;
};

/**
 * NodeJSWindow - Manages the embedded Node.js runtime
 */
class NodeJSWindow {
public:
    NodeJSWindow();
    ~NodeJSWindow();

    /**
     * Initialize and start the Node.js runtime in a separate window
     * @param config Configuration for the runtime
     * @return true if initialization was successful
     */
    bool initialize(const NodeConfig& config = NodeConfig());

    /**
     * Shutdown the Node.js runtime and close the window
     */
    void shutdown();

    /**
     * Check if the Node.js runtime is running
     */
    bool isRunning() const { return m_running; }

    /**
     * Execute JavaScript code in the Node.js runtime
     * @param code JavaScript code to execute
     * @return Result of the execution (0 = success)
     */
    int executeCode(const std::string& code);

    /**
     * Load and execute a JavaScript module
     * @param modulePath Path to the module file
     * @return true if the module was loaded successfully
     */
    bool loadModule(const std::string& modulePath);

private:
    /**
     * Thread function that runs the Node.js event loop
     */
    void nodeThreadFunc();

    /**
     * Create and show the console window
     */
    bool createConsoleWindow();

    /**
     * Initialize the libnode runtime
     */
    bool initializeNodeRuntime();

    /**
     * Run the Node.js event loop
     */
    void runEventLoop();

    std::atomic<bool> m_running;
    std::thread m_nodeThread;
    NodeConfig m_config;
    HWND m_consoleWindow;
    
    // Node.js runtime handles
    void* m_platform;  // node::MultiIsolatePlatform*
    void* m_isolate;   // v8::Isolate*
    void* m_env;       // node::Environment*
};

/**
 * Global instance accessor
 */
NodeJSWindow& getNodeJSWindow();

/**
 * Initialize Node.js for Icarus
 */
bool initializeNodeJS(const NodeConfig& config = NodeConfig());

/**
 * Shutdown Node.js
 */
void shutdownNodeJS();

} // namespace NodeJS
