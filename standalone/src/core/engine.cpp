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

// UE4 native protocol - included via a separate bridge to avoid type conflicts
#ifdef WITH_UE4NET
// Forward declarations only - actual UE4 net code lives in its own translation units
class UE4NetDriver;
class UE4ReplicationManager;

// These functions are defined in ue4_net_bridge.cpp to isolate the ue4net includes
namespace UE4NetBridge {
    UE4NetDriver* CreateDriver();
    void DestroyDriver(UE4NetDriver* Driver);
    bool InitializeDriver(UE4NetDriver* Driver, uint16_t Port);
    void ShutdownDriver(UE4NetDriver* Driver);
    void TickDriver(UE4NetDriver* Driver, float DeltaTime);
    void TickReplication(float DeltaTime);
    int32_t GetNumConnections(UE4NetDriver* Driver);
    bool IsListening(UE4NetDriver* Driver);
}
#endif

#include <thread>
#include <chrono>
#include <spdlog/fmt/fmt.h>

// Static game objects (owned by engine)
static std::unique_ptr<UNetDriver> GNetDriver;
#ifdef WITH_UE4NET
static UE4NetDriver* GUE4NetDriver = nullptr;
#endif
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

#ifdef WITH_UE4NET
    // Prefer UE4 native protocol if available
    GUE4NetDriver = UE4NetBridge::CreateDriver();
    UE4Driver = GUE4NetDriver;

    if (GUE4NetDriver && UE4NetBridge::InitializeDriver(GUE4NetDriver, GServerConfig.Port))
    {
        LOG_INFO(LogInit, "UE4 Native Protocol driver initialized on port {}", GServerConfig.Port);
    }
    else
    {
        LOG_WARN(LogInit, "UE4 Native Protocol driver failed, falling back to ENet");
        if (GUE4NetDriver)
        {
            UE4NetBridge::DestroyDriver(GUE4NetDriver);
            GUE4NetDriver = nullptr;
        }
        UE4Driver = nullptr;
    }

    // Fall through to ENet if UE4 native failed
    if (!UE4Driver)
#endif
    {
        // Legacy ENet driver
        GNetDriver = std::make_unique<UNetDriver>();
        NetDriver = GNetDriver.get();

        if (!NetDriver->Initialize(GServerConfig.Port))
        {
            LOG_ERROR(LogInit, "Failed to initialize network driver on port {}", GServerConfig.Port);
            return false;
        }

        LOG_INFO(LogInit, "ENet network driver initialized on port {}", GServerConfig.Port);
    }

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

#ifdef WITH_UE4NET
    // UE4 native protocol tick
    if (UE4Driver)
    {
        UE4NetBridge::TickDriver(GUE4NetDriver, DeltaTime);
        UE4NetBridge::TickReplication(DeltaTime);
    }
    else
#endif
    {
        // Legacy ENet tick
        if (NetDriver)
        {
            NetDriver->TickFlush(DeltaTime);
        }

        // Legacy replication tick
        if (NetDriver && NetDriver->IsListening())
        {
            UReplicationManager::Get().ServerReplicateActors(NetDriver, DeltaTime);
        }
    }

    // Game mode tick
    if (GGameMode)
    {
        GGameMode->Tick(DeltaTime);
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

#ifdef WITH_UE4NET
    if (UE4Driver)
    {
        UE4NetBridge::ShutdownDriver(GUE4NetDriver);
        UE4NetBridge::DestroyDriver(GUE4NetDriver);
        GUE4NetDriver = nullptr;
        UE4Driver = nullptr;
    }
#endif

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
