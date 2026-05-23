#pragma once

// Engine - Main engine class that manages the game loop and subsystems
// Replaces UEngine that was accessed via FindObject("/Engine/Transient.FortEngine_0")

#include "platform.h"
#include "object_system.h"
#include <atomic>
#include <chrono>

class UNetDriver;
class UE4NetDriver;
class UWorld;
class AGameMode;

class UEngine
{
public:
    static UEngine& Get();

    // Lifecycle
    bool Initialize();
    void Tick(float DeltaTime);
    void Shutdown();

    // Main loop
    void Run();
    void RequestShutdown() { bShutdownRequested = true; }
    bool IsRunning() const { return !bShutdownRequested; }

    // Subsystems
    UWorld* GetWorld() const { return World; }
    UNetDriver* GetNetDriver() const { return NetDriver; }
#ifdef WITH_UE4NET
    UE4NetDriver* GetUE4NetDriver() const { return UE4Driver; }
#endif

    // Time
    float GetDeltaTime() const { return DeltaTime; }
    double GetTimeSeconds() const { return TimeSeconds; }
    uint64 GetFrameCount() const { return FrameCount; }

    // Tick rate
    float GetMaxTickRate() const;

private:
    UEngine() = default;

    UWorld* World = nullptr;
    UNetDriver* NetDriver = nullptr;
#ifdef WITH_UE4NET
    UE4NetDriver* UE4Driver = nullptr;
#endif

    std::atomic<bool> bShutdownRequested{false};
    float DeltaTime = 0.f;
    double TimeSeconds = 0.0;
    uint64 FrameCount = 0;

    std::chrono::high_resolution_clock::time_point LastTickTime;
};
