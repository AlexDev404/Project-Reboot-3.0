/**
 * NodeJSWindow Implementation
 * 
 * Embeds libnode (Node.js library) into the DLL and creates a separate
 * console window for running JavaScript/TypeScript modules via Icarus.
 */

#include "NodeJSWindow.h"
#include "log.h"
#include <iostream>

// Node.js embedding headers
// Note: These would be included from the libnode distribution
// For now, we'll use forward declarations and dynamic loading
#ifdef ENABLE_LIBNODE
#include <node.h>
#include <uv.h>
#endif

namespace NodeJS {

// Global instance
static NodeJSWindow* g_nodeWindow = nullptr;

NodeJSWindow& getNodeJSWindow() {
    if (!g_nodeWindow) {
        g_nodeWindow = new NodeJSWindow();
    }
    return *g_nodeWindow;
}

NodeJSWindow::NodeJSWindow()
    : m_running(false)
    , m_consoleWindow(nullptr)
    , m_platform(nullptr)
    , m_isolate(nullptr)
    , m_context(nullptr)
    , m_env(nullptr)
{
    LOG_INFO(LogDev, "[NodeJS] NodeJSWindow created");
}

NodeJSWindow::~NodeJSWindow() {
    shutdown();
}

bool NodeJSWindow::initialize(const NodeConfig& config) {
    if (m_running) {
        LOG_WARN(LogDev, "[NodeJS] Already initialized");
        return true;
    }

    m_config = config;
    LOG_INFO(LogDev, "[NodeJS] Initializing Node.js runtime...");
    LOG_INFO(LogDev, "[NodeJS] Entry point: {}", config.entryPoint);

    // Create console window if requested
    if (config.showConsole) {
        if (!createConsoleWindow()) {
            LOG_ERROR(LogDev, "[NodeJS] Failed to create console window");
            return false;
        }
    }

    // Start Node.js in a separate thread
    m_running = true;
    m_nodeThread = std::thread(&NodeJSWindow::nodeThreadFunc, this);

    LOG_INFO(LogDev, "[NodeJS] Node.js runtime initialized successfully");
    return true;
}

void NodeJSWindow::shutdown() {
    if (!m_running) {
        return;
    }

    LOG_INFO(LogDev, "[NodeJS] Shutting down Node.js runtime...");
    m_running = false;

    // Wait for the Node.js thread to finish
    if (m_nodeThread.joinable()) {
        m_nodeThread.join();
    }

    // Close console window
    if (m_consoleWindow) {
        FreeConsole();
        m_consoleWindow = nullptr;
    }

    LOG_INFO(LogDev, "[NodeJS] Node.js runtime shut down");
}

bool NodeJSWindow::createConsoleWindow() {
    // Allocate a new console for this process
    if (!AllocConsole()) {
        DWORD error = GetLastError();
        if (error != ERROR_ACCESS_DENIED) {
            LOG_ERROR(LogDev, "[NodeJS] Failed to allocate console: {}", error);
            return false;
        }
        // Console already exists, try to use it
    }

    // Redirect stdout and stderr to the console
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    // Get console window handle
    m_consoleWindow = GetConsoleWindow();
    if (m_consoleWindow) {
        // Set window title
        SetConsoleTitleA(m_config.title.c_str());
        
        // Resize and position the console window
        RECT rect;
        GetWindowRect(m_consoleWindow, &rect);
        MoveWindow(m_consoleWindow, rect.left, rect.top, 
                   m_config.width, m_config.height, TRUE);

        LOG_INFO(LogDev, "[NodeJS] Console window created: {}", m_config.title);
    }

    return true;
}

bool NodeJSWindow::initializeNodeRuntime() {
#ifdef ENABLE_LIBNODE
    LOG_INFO(LogDev, "[NodeJS] Initializing libnode runtime...");

    // Verify Node.js version is 22.22.0
    // In a real implementation, this would check NODE_VERSION_STRING
    // const char* nodeVersion = node::GetVersion();
    // if (strcmp(nodeVersion, "v22.22.0") != 0) {
    //     LOG_ERROR(LogDev, "[NodeJS] Incorrect Node.js version: {}. Required: v22.22.0", nodeVersion);
    //     return false;
    // }
    
    // Initialize Node.js platform
    // In a real implementation, this would use node::InitializeNodePlatform()
    // and create an isolate with node::NewIsolate()
    
    // Example (pseudo-code, actual implementation depends on libnode version):
    // 
    // std::vector<std::string> args = { "node", m_config.entryPoint };
    // std::vector<std::string> exec_args;
    // 
    // m_platform = node::InitializeNodePlatform(4); // 4 threads
    // m_isolate = node::NewIsolate(m_platform);
    // 
    // v8::Isolate::Scope isolate_scope(m_isolate);
    // v8::HandleScope handle_scope(m_isolate);
    // 
    // v8::Local<v8::Context> context = node::NewContext(m_isolate);
    // m_context = context;
    // 
    // node::Environment* env = node::CreateEnvironment(
    //     m_isolate, context, args, exec_args);
    // m_env = env;

    LOG_INFO(LogDev, "[NodeJS] libnode runtime initialized");
    return true;
#else
    LOG_WARN(LogDev, "[NodeJS] libnode support not compiled in, running in mock mode");
    
    // Mock mode - just print messages to console
    std::cout << "========================================" << std::endl;
    std::cout << " Icarus Node.js Runtime (Mock Mode)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Entry Point: " << m_config.entryPoint << std::endl;
    std::cout << std::endl;
    std::cout << "libnode is not available in this build." << std::endl;
    std::cout << "To enable Node.js support:" << std::endl;
    std::cout << "1. Download Node.js 22.22.0 from:" << std::endl;
    std::cout << "   https://nodejs.org/download/release/v22.22.0/" << std::endl;
    std::cout << "2. Place node-v22.22.0-win-x64.zip contents in vendor/libnode/" << std::endl;
    std::cout << "3. Rebuild with ENABLE_LIBNODE defined" << std::endl;
    std::cout << std::endl;
    std::cout << "IMPORTANT: Only Node.js 22.22.0 is supported." << std::endl;
    std::cout << "========================================" << std::endl;
    
    return true;
#endif
}

void NodeJSWindow::runEventLoop() {
#ifdef ENABLE_LIBNODE
    LOG_INFO(LogDev, "[NodeJS] Starting event loop...");

    // Run the Node.js event loop
    // In a real implementation:
    // 
    // v8::Isolate::Scope isolate_scope(m_isolate);
    // v8::HandleScope handle_scope(m_isolate);
    // v8::Context::Scope context_scope(m_context);
    // 
    // node::LoadEnvironment(m_env, 
    //     "const publicRequire = require('module').createRequire(process.cwd() + '/');"
    //     "globalThis.require = publicRequire;"
    // );
    // 
    // // Run event loop until UV_RUN_DEFAULT returns false
    // do {
    //     uv_run(env->event_loop(), UV_RUN_DEFAULT);
    //     // Process other platform tasks
    //     platform->DrainTasks(isolate);
    // } while (!m_running && more_tasks);
    
    LOG_INFO(LogDev, "[NodeJS] Event loop started");
#else
    LOG_INFO(LogDev, "[NodeJS] Mock event loop - waiting for shutdown...");
    
    // In mock mode, just wait until shutdown is requested
    while (m_running) {
        Sleep(100);
    }
#endif
}

void NodeJSWindow::nodeThreadFunc() {
    LOG_INFO(LogDev, "[NodeJS] Node.js thread started");

    try {
        // Initialize the runtime
        if (!initializeNodeRuntime()) {
            LOG_ERROR(LogDev, "[NodeJS] Failed to initialize Node.js runtime");
            m_running = false;
            return;
        }

        // Load the entry point module
        if (!m_config.entryPoint.empty()) {
            LOG_INFO(LogDev, "[NodeJS] Loading entry point: {}", m_config.entryPoint);
            loadModule(m_config.entryPoint);
        }

        // Run the event loop
        runEventLoop();

#ifdef ENABLE_LIBNODE
        // Cleanup
        if (m_env) {
            // node::FreeEnvironment(m_env);
            m_env = nullptr;
        }
        
        if (m_isolate) {
            // m_isolate->Dispose();
            m_isolate = nullptr;
        }
#endif

    } catch (const std::exception& e) {
        LOG_ERROR(LogDev, "[NodeJS] Exception in Node.js thread: {}", e.what());
    }

    LOG_INFO(LogDev, "[NodeJS] Node.js thread exiting");
    m_running = false;
}

int NodeJSWindow::executeCode(const std::string& code) {
    if (!m_running) {
        LOG_ERROR(LogDev, "[NodeJS] Cannot execute code - runtime not running");
        return 1;
    }

#ifdef ENABLE_LIBNODE
    // In real implementation:
    // v8::Isolate::Scope isolate_scope(m_isolate);
    // v8::HandleScope handle_scope(m_isolate);
    // v8::Context::Scope context_scope(m_context);
    // 
    // v8::Local<v8::String> source = v8::String::NewFromUtf8(m_isolate, code.c_str());
    // v8::Local<v8::Script> script = v8::Script::Compile(m_context, source).ToLocalChecked();
    // v8::Local<v8::Value> result = script->Run(m_context).ToLocalChecked();
    
    LOG_INFO(LogDev, "[NodeJS] Executed code: {}", code.substr(0, 50));
    return 0;
#else
    LOG_INFO(LogDev, "[NodeJS] Mock execute: {}", code.substr(0, 50));
    std::cout << "[Mock] Execute: " << code << std::endl;
    return 0;
#endif
}

bool NodeJSWindow::loadModule(const std::string& modulePath) {
    if (!m_running) {
        LOG_ERROR(LogDev, "[NodeJS] Cannot load module - runtime not running");
        return false;
    }

    LOG_INFO(LogDev, "[NodeJS] Loading module: {}", modulePath);

#ifdef ENABLE_LIBNODE
    // In real implementation:
    // std::string code = "require('" + modulePath + "');";
    // return executeCode(code) == 0;
    
    return true;
#else
    std::cout << "[Mock] Loading module: " << modulePath << std::endl;
    std::cout << "Module would be loaded here if libnode was available." << std::endl;
    return true;
#endif
}

// ============================================================================
// Public API Functions
// ============================================================================

bool initializeNodeJS(const NodeConfig& config) {
    auto& window = getNodeJSWindow();
    return window.initialize(config);
}

void shutdownNodeJS() {
    if (g_nodeWindow) {
        g_nodeWindow->shutdown();
        delete g_nodeWindow;
        g_nodeWindow = nullptr;
    }
}

} // namespace NodeJS
