// Project Reboot 3.0 - Standalone Server
// Main entry point - replaces DllMain
//
// This is a standalone dedicated game server for Fortnite v17.50
// No DLL injection, no process hooking, no MinHook, no Memcury.

#include "core/platform.h"
#include "core/engine.h"
#include "core/object_globals.h"
#include "core/asset_registry.h"
#include "core/event_dispatcher.h"
#include "net/net_driver.h"
#include "net/replication.h"
#include "game/game_mode.h"
#include "game/game_state.h"
#include "game/player_controller.h"
#include "util/config.h"
#include "util/logging.h"
#include "util/discord.h"
#include "util/pak_parser.h"

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>

// Global config
ServerConfig GServerConfig;

// Signal handler for graceful shutdown
static void SignalHandler(int Signal)
{
    fmt::print("\n[Server] Received signal {}, shutting down...\n", Signal);
    UEngine::Get().RequestShutdown();
}

// =============================================================================
// CLI Command Handler
// =============================================================================

void ProcessCommand(const std::string& Command)
{
    if (Command == "quit" || Command == "exit" || Command == "stop")
    {
        UEngine::Get().RequestShutdown();
    }
    else if (Command == "status")
    {
        auto* NetDriver = UEngine::Get().GetNetDriver();
        fmt::print("[Server] Status:\n");
        fmt::print("  Port: {}\n", GServerConfig.Port);
        fmt::print("  Players: {}/{}\n",
            NetDriver ? NetDriver->GetNumConnections() : 0,
            GServerConfig.MaxPlayers);
        fmt::print("  Tick Rate: {:.1f}\n", GServerConfig.TickRate);
        fmt::print("  Uptime: {:.1f}s\n", UEngine::Get().GetTimeSeconds());
    }
    else if (Command == "players")
    {
        auto* NetDriver = UEngine::Get().GetNetDriver();
        if (!NetDriver) { fmt::print("  No net driver\n"); return; }
        fmt::print("[Server] Connected players:\n");
        for (auto& Conn : NetDriver->GetClientConnections())
        {
            fmt::print("  [{}] {} (ping: {:.0f}ms)\n",
                Conn->GetConnectionId(),
                Conn->GetPlayerName(),
                Conn->GetPing());
        }
    }
    else if (Command.starts_with("kick "))
    {
        // TODO: Implement kick by name/id
        fmt::print("[Server] Kick not yet implemented\n");
    }
    else if (Command == "help")
    {
        fmt::print("[Server] Available commands:\n");
        fmt::print("  status   - Show server status\n");
        fmt::print("  players  - List connected players\n");
        fmt::print("  kick <n> - Kick player by name\n");
        fmt::print("  quit     - Stop the server\n");
        fmt::print("  help     - Show this help\n");
    }
    else if (!Command.empty())
    {
        fmt::print("[Server] Unknown command: '{}'. Type 'help' for available commands.\n", Command);
    }
}

// =============================================================================
// CLI Input Thread
// =============================================================================

void CLIThread()
{
    std::string Line;
    while (UEngine::Get().IsRunning())
    {
        if (std::getline(std::cin, Line))
        {
            ProcessCommand(Line);
        }
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main(int argc, char* argv[])
{
    // Print banner
    fmt::print("╔══════════════════════════════════════════╗\n");
    fmt::print("║   Project Reboot 3.0 - Standalone Server ║\n");
    fmt::print("║   Target: Fortnite v17.50 (S17)          ║\n");
    fmt::print("║   Engine: UE4 4.26                       ║\n");
    fmt::print("╚══════════════════════════════════════════╝\n\n");

    // Initialize logging
    Logging::Initialize("server.log");
    LOG_INFO(LogInit, "Starting Project Reboot 3.0 Standalone Server");
    LOG_INFO(LogInit, "Built on {} {}", __DATE__, __TIME__);
    LOG_INFO(LogInit, "Target: Fortnite v{:.2f} (Engine {})", Fortnite_Version, Engine_Version);

    // Setup signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Load configuration
    std::string ConfigPath = "config/server.json";
    for (int i = 1; i < argc; i++)
    {
        std::string Arg = argv[i];
        if (Arg == "--config" && i + 1 < argc)
        {
            ConfigPath = argv[++i];
        }
        else if (Arg == "--port" && i + 1 < argc)
        {
            GServerConfig.Port = static_cast<uint16>(std::stoi(argv[++i]));
        }
        else if (Arg == "--max-players" && i + 1 < argc)
        {
            GServerConfig.MaxPlayers = static_cast<uint16>(std::stoi(argv[++i]));
        }
        else if (Arg == "--playlist" && i + 1 < argc)
        {
            GServerConfig.PlaylistPath = argv[++i];
        }
        else if (Arg == "--pak-dir" && i + 1 < argc)
        {
            GServerConfig.PakDirectory = argv[++i];
        }
        else if (Arg == "--help" || Arg == "-h")
        {
            fmt::print("Usage: {} [options]\n", argv[0]);
            fmt::print("Options:\n");
            fmt::print("  --config <path>       Path to config file (default: config/server.json)\n");
            fmt::print("  --port <port>         Server port (default: 7777)\n");
            fmt::print("  --max-players <n>     Maximum players (default: 100)\n");
            fmt::print("  --playlist <path>     Playlist path\n");
            fmt::print("  --pak-dir <path>      PAK files directory\n");
            fmt::print("  --help, -h            Show this help\n");
            return 0;
        }
    }

    if (Config::Get().Load(ConfigPath))
    {
        LOG_INFO(LogConfig, "Loaded config from: {}", ConfigPath);
    }
    else
    {
        LOG_WARN(LogConfig, "No config file found at '{}', using defaults", ConfigPath);
    }

    // Initialize PAK file system
    LOG_INFO(LogInit, "Mounting PAK files from: {}", GServerConfig.PakDirectory);
    int32 PakCount = FPakManager::Get().MountDirectory(GServerConfig.PakDirectory);
    LOG_INFO(LogInit, "Mounted {} PAK file(s)", PakCount);

    // Initialize asset registry
    if (!UAssetRegistry::Get().Initialize(GServerConfig.PakDirectory, GServerConfig.ConfigDirectory))
    {
        LOG_WARN(LogInit, "Asset registry initialization incomplete - some features may not work");
    }

    // Initialize Discord webhooks
    if (GServerConfig.bEnableDiscord && !GServerConfig.DiscordWebhookURL.empty())
    {
        if (Discord::Initialize(GServerConfig.DiscordWebhookURL))
        {
            LOG_INFO(LogInit, "Discord webhooks enabled");
            Discord::SendServerStatus("Server starting...");
        }
    }

    // Initialize engine
    if (!UEngine::Get().Initialize())
    {
        LOG_ERROR(LogInit, "Failed to initialize engine!");
        return 1;
    }

    LOG_INFO(LogInit, "Engine initialized successfully");
    LOG_INFO(LogInit, "Listening on port {}", GServerConfig.Port);
    LOG_INFO(LogInit, "Max players: {}", GServerConfig.MaxPlayers);
    LOG_INFO(LogInit, "Playlist: {}", GServerConfig.PlaylistPath);

    fmt::print("\n[Server] Ready! Type 'help' for commands.\n");

    Discord::SendServerStatus(fmt::format("Server ready on port {}", GServerConfig.Port));

    // Start CLI thread
    std::thread CLIInput(CLIThread);
    CLIInput.detach();

    // Run main game loop
    UEngine::Get().Run();

    // Shutdown
    LOG_INFO(LogInit, "Shutting down...");
    Discord::SendServerStatus("Server shutting down");
    Discord::Shutdown();
    UEngine::Get().Shutdown();
    Logging::Shutdown();

    fmt::print("[Server] Goodbye!\n");
    return 0;
}
