// Engine implementation - Main game loop and subsystem management

#include "core/engine.h"
#include "core/object_globals.h"
#include "core/asset_registry.h"
#include "net/net_driver.h"
#include "net/replication.h"
#include "game/game_mode.h"
#include "game/game_state.h"
#include "util/logging.h"
#include "util/config.h"

#include <thread>
#include <chrono>
#include <spdlog/fmt/fmt.h>

// Static game objects (owned by engine)
static std::unique_ptr<UNetDriver> GNetDriver;
static std::unique_ptr<AFortGameModeAthena> GGameMode;
static std::unique_ptr<AFortGameStateAthena> GGameState;

UEngine& UEngine::Get()
{
    static UEngine Instance;
    return Instance;
}

bool UEngine::Initialize()
{
    LOG_INFO(LogInit, "Initializing engine subsystems...");

    // Create net driver
    GNetDriver = std::make_unique<UNetDriver>();
    NetDriver = GNetDriver.get();

    if (!NetDriver->Initialize(GServerConfig.Port))
    {
        LOG_ERROR(LogInit, "Failed to initialize network driver on port {}", GServerConfig.Port);
        return false;
    }

    LOG_INFO(LogInit, "Network driver initialized on port {}", GServerConfig.Port);

    // Create game state
    GGameState = std::make_unique<AFortGameStateAthena>();

    // Create game mode
    GGameMode = std::make_unique<AFortGameModeAthena>();
    GGameMode->SetPlaylist(GServerConfig.PlaylistPath);
    GGameMode->InitGame();

    LOG_INFO(LogInit, "Game mode initialized");

    LastTickTime = std::chrono::high_resolution_clock::now();

    return true;
}

float UEngine::GetMaxTickRate() const
{
    return static_cast<float>(GServerConfig.TickRate);
}

void UEngine::Tick(float InDeltaTime)
{
    DeltaTime = InDeltaTime;
    TimeSeconds += DeltaTime;
    FrameCount++;

    // Network tick - process incoming packets and replicate
    if (NetDriver)
    {
        NetDriver->TickFlush(DeltaTime);
    }

    // Game mode tick
    if (GGameMode)
    {
        GGameMode->Tick(DeltaTime);
    }

    // Replication tick
    if (NetDriver && NetDriver->IsListening())
    {
        UReplicationManager::Get().ServerReplicateActors(NetDriver, DeltaTime);
    }
}

void UEngine::Run()
{
    const double TargetTickInterval = 1.0 / GServerConfig.TickRate;

    while (!bShutdownRequested)
    {
        auto Now = std::chrono::high_resolution_clock::now();
        auto Elapsed = std::chrono::duration<double>(Now - LastTickTime).count();

        if (Elapsed >= TargetTickInterval)
        {
            Tick(static_cast<float>(Elapsed));
            LastTickTime = Now;
        }
        else
        {
            // Sleep for remaining time to avoid busy-waiting
            auto SleepTime = TargetTickInterval - Elapsed;
            if (SleepTime > 0.001) // Only sleep if > 1ms remaining
            {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<int64>((SleepTime - 0.001) * 1000000.0))
                );
            }
        }
    }
}

void UEngine::Shutdown()
{
    LOG_INFO(LogInit, "Engine shutdown initiated...");

    if (NetDriver)
    {
        NetDriver->Shutdown();
        NetDriver = nullptr;
    }

    GNetDriver.reset();
    GGameMode.reset();
    GGameState.reset();

    LOG_INFO(LogInit, "Engine shutdown complete");
}
