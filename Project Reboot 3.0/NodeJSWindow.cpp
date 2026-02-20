/**
 * NodeJSWindow Implementation
 * 
 * Embeds libnode (Node.js library) into the DLL and creates a separate
 * console window for running JavaScript/TypeScript modules via Icarus.
 */

#include "NodeJSWindow.h"
#include "log.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Node.js embedding headers
#include <node.h>
#include <node_platform.h>
#include <v8.h>
#include <uv.h>

// Link against node.lib
#pragma comment(lib, "node.lib")

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
    LOG_INFO(LogDev, "[NodeJS] Initializing Node.js 22.22.0 runtime...");

    try {
        // Initialize V8 platform
        int argc = 1;
        const char* argv[] = { "node" };
        
        std::vector<std::string> args(argv, argv + argc);
        std::vector<std::string> exec_args;
        std::vector<std::string> errors;
        
        // Setup Node.js initialization parameters
        node::InitializationResult result = node::InitializeOncePerProcess(args, {
            node::ProcessInitializationFlags::kNoInitializeV8,
            node::ProcessInitializationFlags::kNoInitializeNodeV8Platform
        });
        
        if (result.early_return) {
            LOG_ERROR(LogDev, "[NodeJS] Node.js initialization returned early");
            return false;
        }
        
        if (result.exit_code != 0) {
            LOG_ERROR(LogDev, "[NodeJS] Node.js initialization failed with code: {}", result.exit_code);
            return false;
        }
        
        // Create the Node.js platform
        m_platform = node::MultiIsolatePlatform::Create(4);
        v8::V8::InitializePlatform(static_cast<node::MultiIsolatePlatform*>(m_platform));
        v8::V8::Initialize();
        
        // Create isolate
        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = node::CreateArrayBufferAllocator();
        
        v8::Isolate* isolate = v8::Isolate::New(create_params);
        m_isolate = isolate;
        
        if (!isolate) {
            LOG_ERROR(LogDev, "[NodeJS] Failed to create V8 isolate");
            return false;
        }
        
        // Enter isolate
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        
        // Create context
        v8::Local<v8::Context> context = node::NewContext(isolate);
        if (context.IsEmpty()) {
            LOG_ERROR(LogDev, "[NodeJS] Failed to create V8 context");
            return false;
        }
        
        v8::Context::Scope context_scope(context);
        
        // Create Node.js environment
        node::Environment* env = node::CreateEnvironment(
            node::GetCurrentEnvironment(context),
            context,
            args,
            exec_args
        );
        
        m_env = env;
        
        if (!env) {
            LOG_ERROR(LogDev, "[NodeJS] Failed to create Node.js environment");
            return false;
        }
        
        LOG_INFO(LogDev, "[NodeJS] Node.js 22.22.0 runtime initialized successfully");
        LOG_INFO(LogDev, "[NodeJS] V8 version: {}", v8::V8::GetVersion());
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(LogDev, "[NodeJS] Exception during initialization: {}", e.what());
        return false;
    } catch (...) {
        LOG_ERROR(LogDev, "[NodeJS] Unknown exception during initialization");
        return false;
    }
}

void NodeJSWindow::runEventLoop() {
    LOG_INFO(LogDev, "[NodeJS] Starting Node.js event loop...");
    
    v8::Isolate* isolate = static_cast<v8::Isolate*>(m_isolate);
    node::Environment* env = static_cast<node::Environment*>(m_env);
    
    if (!isolate || !env) {
        LOG_ERROR(LogDev, "[NodeJS] Invalid isolate or environment");
        return;
    }
    
    try {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Context::Scope context_scope(env->context());
        
        // Load environment (this runs the entry point)
        node::LoadEnvironment(
            env,
            "const publicRequire = require('module').createRequire(process.cwd() + '/');"
            "globalThis.require = publicRequire;"
        );
        
        // Run the event loop
        uv_loop_t* loop = env->event_loop();
        node::MultiIsolatePlatform* platform = static_cast<node::MultiIsolatePlatform*>(m_platform);
        
        LOG_INFO(LogDev, "[NodeJS] Event loop started");
        
        while (m_running) {
            // Run one iteration of the event loop
            uv_run(loop, UV_RUN_ONCE);
            
            // Process V8 platform tasks
            if (platform) {
                platform->DrainTasks(isolate);
            }
            
            // Check if there's more work
            bool has_more_work = uv_loop_alive(loop);
            if (!has_more_work) {
                // Keep alive for a bit more
                Sleep(100);
            }
        }
        
        LOG_INFO(LogDev, "[NodeJS] Event loop stopped");
        
    } catch (const std::exception& e) {
        LOG_ERROR(LogDev, "[NodeJS] Exception in event loop: {}", e.what());
    } catch (...) {
        LOG_ERROR(LogDev, "[NodeJS] Unknown exception in event loop");
    }
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

        // Cleanup
        node::Environment* env = static_cast<node::Environment*>(m_env);
        v8::Isolate* isolate = static_cast<v8::Isolate*>(m_isolate);
        node::MultiIsolatePlatform* platform = static_cast<node::MultiIsolatePlatform*>(m_platform);
        
        if (env) {
            node::FreeEnvironment(env);
            m_env = nullptr;
        }
        
        if (isolate) {
            bool platform_finished = false;
            if (platform) {
                platform->DrainTasks(isolate);
                platform->CancelPendingDelayedTasks(isolate);
                platform->UnregisterIsolate(isolate);
            }
            
            isolate->Dispose();
            m_isolate = nullptr;
        }
        
        if (platform) {
            v8::V8::Dispose();
            v8::V8::DisposePlatform();
            delete platform;
            m_platform = nullptr;
        }

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

    v8::Isolate* isolate = static_cast<v8::Isolate*>(m_isolate);
    node::Environment* env = static_cast<node::Environment*>(m_env);
    
    if (!isolate || !env) {
        LOG_ERROR(LogDev, "[NodeJS] Invalid isolate or environment");
        return 1;
    }
    
    try {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Context::Scope context_scope(env->context());
        
        v8::Local<v8::String> source = v8::String::NewFromUtf8(
            isolate, code.c_str(), v8::NewStringType::kNormal, code.length()
        ).ToLocalChecked();
        
        v8::Local<v8::Script> script = v8::Script::Compile(env->context(), source).ToLocalChecked();
        v8::Local<v8::Value> result = script->Run(env->context()).ToLocalChecked();
        
        LOG_INFO(LogDev, "[NodeJS] Executed code: {}", code.substr(0, 50));
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR(LogDev, "[NodeJS] Exception executing code: {}", e.what());
        return 1;
    }
}

bool NodeJSWindow::loadModule(const std::string& modulePath) {
    if (!m_running) {
        LOG_ERROR(LogDev, "[NodeJS] Cannot load module - runtime not running");
        return false;
    }

    LOG_INFO(LogDev, "[NodeJS] Loading module: {}", modulePath);

    v8::Isolate* isolate = static_cast<v8::Isolate*>(m_isolate);
    node::Environment* env = static_cast<node::Environment*>(m_env);
    
    if (!isolate || !env) {
        LOG_ERROR(LogDev, "[NodeJS] Invalid isolate or environment");
        return false;
    }
    
    try {
        // Read module file
        std::ifstream file(modulePath);
        if (!file.is_open()) {
            LOG_ERROR(LogDev, "[NodeJS] Failed to open module: {}", modulePath);
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string code = buffer.str();
        file.close();
        
        // Execute the module code
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Context::Scope context_scope(env->context());
        
        v8::Local<v8::String> source = v8::String::NewFromUtf8(
            isolate, code.c_str(), v8::NewStringType::kNormal, code.length()
        ).ToLocalChecked();
        
        v8::Local<v8::Script> script = v8::Script::Compile(env->context(), source).ToLocalChecked();
        v8::Local<v8::Value> result = script->Run(env->context()).ToLocalChecked();
        
        LOG_INFO(LogDev, "[NodeJS] Module loaded successfully: {}", modulePath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(LogDev, "[NodeJS] Exception loading module: {}", e.what());
        return false;
    }
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
