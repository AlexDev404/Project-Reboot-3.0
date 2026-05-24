#pragma once

// Game Mode - Athena Battle Royale game mode
// Extracted from hooked functions in the original DLL

#include "core/object_system.h"
#include "core/platform.h"
#include <vector>
#include <memory>

class AFortPlayerControllerAthena;
class AFortPlayerPawnAthena;
namespace Reboot { class UNetConnection; }
using Reboot::UNetConnection;

// =============================================================================
// Game Phase
// =============================================================================

enum class EAthenaGamePhase : uint8
{
    None = 0,
    Setup = 1,
    Warmup = 2,
    Aircraft = 3,
    SafeZones = 4,
    EndGame = 5,
    Count = 6,
};

enum class EAthenaGamePhaseStep : uint8
{
    None = 0,
    Setup = 1,
    Warmup = 2,
    BusLocked = 3,
    BusFlying = 4,
    StormForming = 5,
    StormHolding = 6,
    StormShrinking = 7,
    Countdown = 8,
    FinalCountdown = 9,
    EndGame = 10,
};

// =============================================================================
// Aircraft (Battle Bus)
// =============================================================================

struct FAircraftData
{
    FVector StartLocation;
    FVector EndLocation;
    FVector CurrentLocation;
    FRotator Rotation;
    float Speed = 0.f;
    float FlightTime = 0.f;
    float MaxFlightTime = 60.f;
    bool bIsFlying = false;
};

// =============================================================================
// Safe Zone (Storm)
// =============================================================================

struct FSafeZoneData
{
    FVector Center;
    float Radius = 0.f;
    FVector NextCenter;
    float NextRadius = 0.f;
    float ShrinkTime = 0.f;
    float WaitTime = 0.f;
    float DamagePerSecond = 1.f;
    int32 CurrentPhase = 0;
};

// =============================================================================
// AFortGameModeAthena
// =============================================================================

class AFortGameModeAthena
{
public:
    AFortGameModeAthena() = default;
    virtual ~AFortGameModeAthena() = default;

    // Lifecycle
    void InitGame();
    void StartMatch();
    void EndMatch();
    void Tick(float DeltaTime);

    // Player management
    void HandleStartingNewPlayer(AFortPlayerControllerAthena* NewPlayer);
    AFortPlayerPawnAthena* SpawnDefaultPawnFor(AFortPlayerControllerAthena* Controller);
    void OnPlayerDied(AFortPlayerControllerAthena* DeadPlayer, AFortPlayerControllerAthena* Killer);
    void RemoveFromAlivePlayers(AFortPlayerControllerAthena* Player);

    // Aircraft
    void StartAircraftPhase();
    void OnAircraftEnteredDropZone();
    void ServerAttemptAircraftJump(AFortPlayerControllerAthena* Player, const FRotator& JumpRotation);
    const FAircraftData& GetAircraftData() const { return AircraftData; }

    // Safe Zone
    void StartSafeZone();
    void SetZoneToIndex(int32 Index);
    const FSafeZoneData& GetSafeZoneData() const { return SafeZoneData; }

    // Match state
    EAthenaGamePhase GetCurrentPhase() const { return CurrentPhase; }
    EAthenaGamePhaseStep GetCurrentPhaseStep() const { return CurrentPhaseStep; }
    int32 GetAlivePlayers() const { return static_cast<int32>(AlivePlayers.size()); }
    int32 GetTotalPlayers() const { return TotalPlayersAtStart; }

    // Team management
    uint8 PickTeam(AFortPlayerControllerAthena* Player);

    // Ready to start
    bool ReadyToStartMatch() const;
    void OnPlayerReadyToStartMatch(AFortPlayerControllerAthena* Player);

    // Loot
    void SpawnLoot();
    void SpawnSupplyDrop();

    // Configuration
    void SetPlaylist(const std::string& PlaylistPath);

private:
    EAthenaGamePhase CurrentPhase = EAthenaGamePhase::None;
    EAthenaGamePhaseStep CurrentPhaseStep = EAthenaGamePhaseStep::None;

    std::vector<AFortPlayerControllerAthena*> AlivePlayers;
    std::vector<AFortPlayerControllerAthena*> AllPlayers;
    int32 TotalPlayersAtStart = 0;

    FAircraftData AircraftData;
    FSafeZoneData SafeZoneData;

    // Timers
    double PhaseStartTime = 0.0;
    double WarmupDuration = 60.0;
    double AircraftDuration = 60.0;

    // Safe zone phases (v17.50 default timings)
    struct FSafeZonePhaseConfig {
        float WaitTime;
        float ShrinkTime;
        float RadiusPercent;
        float Damage;
    };
    std::vector<FSafeZonePhaseConfig> ZonePhases;

    uint8 NextTeamIndex = 0;
    bool bMatchStarted = false;
    bool bMatchEnded = false;

    void TickAircraft(float DeltaTime);
    void TickSafeZone(float DeltaTime);
    void TickStormDamage(float DeltaTime);
    void CheckForWinner();
    void InitZonePhases();
};
