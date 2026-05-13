// Game Mode implementation - Athena Battle Royale
// Extracted from hook functions in the original DLL's dllmain.cpp

#include "game/game_mode.h"
#include "game/game_state.h"
#include "game/player_controller.h"
#include "game/player_pawn.h"
#include "game/player_state.h"
#include "net/net_driver.h"
#include "core/engine.h"
#include "util/logging.h"
#include "util/discord.h"
#include "util/config.h"

#include <algorithm>
#include <random>
#include <cmath>

void AFortGameModeAthena::InitGame()
{
    LOG_INFO(LogGame, "Initializing Athena game mode");
    CurrentPhase = EAthenaGamePhase::Setup;
    CurrentPhaseStep = EAthenaGamePhaseStep::Setup;
    InitZonePhases();
}

void AFortGameModeAthena::InitZonePhases()
{
    // v17.50 storm phase configuration (from extracted game data)
    const auto& ConfigPhases = Config::Get().GetStormPhases();

    if (!ConfigPhases.empty())
    {
        for (const auto& Phase : ConfigPhases)
        {
            ZonePhases.push_back({Phase.WaitTime, Phase.ShrinkTime, Phase.RadiusPercent, Phase.Damage});
        }
    }
    else
    {
        // Default v17.50 storm timings
        ZonePhases = {
            {120.f, 90.f,  0.60f, 1.f},    // Phase 1
            {90.f,  75.f,  0.40f, 1.f},     // Phase 2
            {60.f,  60.f,  0.25f, 2.f},     // Phase 3
            {45.f,  45.f,  0.15f, 5.f},     // Phase 4
            {30.f,  30.f,  0.08f, 8.f},     // Phase 5
            {20.f,  20.f,  0.04f, 10.f},    // Phase 6
            {15.f,  15.f,  0.01f, 10.f},    // Phase 7
            {10.f,  10.f,  0.00f, 10.f},    // Phase 8 (closing)
        };
    }
}

void AFortGameModeAthena::SetPlaylist(const std::string& PlaylistPath)
{
    LOG_INFO(LogGame, "Setting playlist: {}", PlaylistPath);
    // Load playlist configuration
    // This would load the playlist definition from assets
}

void AFortGameModeAthena::StartMatch()
{
    if (bMatchStarted) return;

    bMatchStarted = true;
    TotalPlayersAtStart = static_cast<int32>(AllPlayers.size());
    CurrentPhase = EAthenaGamePhase::Warmup;
    CurrentPhaseStep = EAthenaGamePhaseStep::Warmup;
    PhaseStartTime = UEngine::Get().GetTimeSeconds();

    LOG_INFO(LogGame, "Match started with {} players", TotalPlayersAtStart);
    Discord::SendMatchStarted(TotalPlayersAtStart);
}

void AFortGameModeAthena::EndMatch()
{
    if (bMatchEnded) return;
    bMatchEnded = true;
    CurrentPhase = EAthenaGamePhase::EndGame;
    CurrentPhaseStep = EAthenaGamePhaseStep::EndGame;

    LOG_INFO(LogGame, "Match ended!");

    // Determine winner
    if (!AlivePlayers.empty())
    {
        auto* Winner = AlivePlayers[0];
        auto* WinnerState = Winner->GetPlayerState();
        if (WinnerState)
        {
            WinnerState->SetPlace(1);
            LOG_INFO(LogGame, "Winner: {} ({} kills)",
                WinnerState->GetPlayerName(), WinnerState->GetKillScore());
            Discord::SendMatchEnded(WinnerState->GetPlayerName(), WinnerState->GetKillScore());
        }
    }
}

void AFortGameModeAthena::Tick(float DeltaTime)
{
    if (!bMatchStarted || bMatchEnded) return;

    double CurrentTime = UEngine::Get().GetTimeSeconds();
    double PhaseElapsed = CurrentTime - PhaseStartTime;

    switch (CurrentPhase)
    {
        case EAthenaGamePhase::Warmup:
        {
            if (PhaseElapsed >= WarmupDuration)
            {
                StartAircraftPhase();
            }
            break;
        }
        case EAthenaGamePhase::Aircraft:
        {
            TickAircraft(DeltaTime);
            if (PhaseElapsed >= AircraftDuration)
            {
                // Force everyone out of the bus
                CurrentPhase = EAthenaGamePhase::SafeZones;
                CurrentPhaseStep = EAthenaGamePhaseStep::StormForming;
                PhaseStartTime = CurrentTime;
                StartSafeZone();
            }
            break;
        }
        case EAthenaGamePhase::SafeZones:
        {
            TickSafeZone(DeltaTime);
            TickStormDamage(DeltaTime);
            break;
        }
        default:
            break;
    }

    CheckForWinner();
}

void AFortGameModeAthena::HandleStartingNewPlayer(AFortPlayerControllerAthena* NewPlayer)
{
    if (!NewPlayer) return;

    LOG_INFO(LogGame, "New player joining: {}", NewPlayer->GetPlayerName());

    // Assign team
    uint8 Team = PickTeam(NewPlayer);
    NewPlayer->SetTeamIndex(Team);

    // Create player state
    auto* PS = NewPlayer->GetPlayerState();
    if (PS)
    {
        PS->SetTeamIndex(Team);
        PS->SetPlayerName(NewPlayer->GetPlayerName());
    }

    // Add to players list
    AllPlayers.push_back(NewPlayer);
    AlivePlayers.push_back(NewPlayer);

    // Give starting items (pickaxe)
    NewPlayer->GiveItem("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01", 1);

    // Spawn pawn
    SpawnDefaultPawnFor(NewPlayer);

    Discord::SendPlayerJoined(NewPlayer->GetPlayerName(),
        static_cast<int32>(AllPlayers.size()),
        GServerConfig.MaxPlayers);
}

AFortPlayerPawnAthena* AFortGameModeAthena::SpawnDefaultPawnFor(AFortPlayerControllerAthena* Controller)
{
    if (!Controller) return nullptr;

    auto* Pawn = new AFortPlayerPawnAthena();
    Controller->Possess(Pawn);

    // Set spawn location (lobby island during warmup, etc.)
    if (CurrentPhase == EAthenaGamePhase::Warmup || CurrentPhase == EAthenaGamePhase::Setup)
    {
        // Spawn on lobby island
        Pawn->SetLocation(FVector(0.f, 0.f, 500.f)); // Placeholder
    }

    LOG_INFO(LogGame, "Spawned pawn for player: {}", Controller->GetPlayerName());
    return Pawn;
}

void AFortGameModeAthena::OnPlayerDied(AFortPlayerControllerAthena* DeadPlayer, AFortPlayerControllerAthena* Killer)
{
    if (!DeadPlayer) return;

    RemoveFromAlivePlayers(DeadPlayer);
    DeadPlayer->bIsAlive = false;

    auto* DeadState = DeadPlayer->GetPlayerState();
    if (DeadState)
    {
        DeadState->SetPlace(GetAlivePlayers() + 1);
    }

    // Credit kill
    if (Killer && Killer != DeadPlayer)
    {
        auto* KillerState = Killer->GetPlayerState();
        if (KillerState)
        {
            KillerState->AddKill();
            Discord::SendPlayerKill(
                KillerState->GetPlayerName(),
                DeadPlayer->GetPlayerName(),
                "weapon" // TODO: actual weapon name
            );
        }
    }

    LOG_INFO(LogGame, "Player died: {} ({} remaining)",
        DeadPlayer->GetPlayerName(), GetAlivePlayers());
}

void AFortGameModeAthena::RemoveFromAlivePlayers(AFortPlayerControllerAthena* Player)
{
    AlivePlayers.erase(
        std::remove(AlivePlayers.begin(), AlivePlayers.end(), Player),
        AlivePlayers.end()
    );
}

uint8 AFortGameModeAthena::PickTeam(AFortPlayerControllerAthena* Player)
{
    (void)Player;
    // Solo mode: each player gets their own team
    return NextTeamIndex++;
}

bool AFortGameModeAthena::ReadyToStartMatch() const
{
    // Start when at least 2 players are ready (configurable)
    int32 ReadyCount = 0;
    for (auto* Player : AllPlayers)
    {
        if (Player->bHasServerFinishedLoading)
            ReadyCount++;
    }
    return ReadyCount >= 2;
}

void AFortGameModeAthena::OnPlayerReadyToStartMatch(AFortPlayerControllerAthena* Player)
{
    Player->bHasServerFinishedLoading = true;
    LOG_INFO(LogGame, "Player ready: {}", Player->GetPlayerName());

    if (!bMatchStarted && ReadyToStartMatch())
    {
        StartMatch();
    }
}

// =============================================================================
// Aircraft
// =============================================================================

void AFortGameModeAthena::StartAircraftPhase()
{
    CurrentPhase = EAthenaGamePhase::Aircraft;
    CurrentPhaseStep = EAthenaGamePhaseStep::BusFlying;
    PhaseStartTime = UEngine::Get().GetTimeSeconds();

    // Generate random flight path across the map
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-50000.f, 50000.f);

    AircraftData.StartLocation = FVector(dist(gen), dist(gen), 50000.f);
    AircraftData.EndLocation = FVector(dist(gen), dist(gen), 50000.f);
    AircraftData.CurrentLocation = AircraftData.StartLocation;
    AircraftData.bIsFlying = true;
    AircraftData.Speed = 2500.f; // Bus speed
    AircraftData.FlightTime = 0.f;
    AircraftData.MaxFlightTime = AircraftDuration;

    // Put all players in aircraft
    for (auto* Player : AllPlayers)
    {
        Player->bIsInAircraft = true;
        auto* PS = Player->GetPlayerState();
        if (PS) PS->SetInAircraft(true);
    }

    LOG_INFO(LogGame, "Aircraft phase started");
}

void AFortGameModeAthena::OnAircraftEnteredDropZone()
{
    // Players can now jump
    LOG_INFO(LogGame, "Aircraft entered drop zone");
}

void AFortGameModeAthena::ServerAttemptAircraftJump(AFortPlayerControllerAthena* Player, const FRotator& JumpRotation)
{
    if (!Player || !Player->bIsInAircraft) return;

    Player->bIsInAircraft = false;
    auto* PS = Player->GetPlayerState();
    if (PS) PS->SetInAircraft(false);

    // Set pawn location to bus position and begin skydive
    auto* Pawn = Player->GetPawn();
    if (Pawn)
    {
        Pawn->SetLocation(AircraftData.CurrentLocation);
        Pawn->GetStats().bIsSkydiving = true;
    }

    LOG_INFO(LogGame, "Player {} jumped from aircraft", Player->GetPlayerName());
}

void AFortGameModeAthena::TickAircraft(float DeltaTime)
{
    if (!AircraftData.bIsFlying) return;

    AircraftData.FlightTime += DeltaTime;
    float Alpha = AircraftData.FlightTime / AircraftData.MaxFlightTime;
    Alpha = std::min(Alpha, 1.f);

    // Lerp position
    AircraftData.CurrentLocation = FVector(
        AircraftData.StartLocation.X + (AircraftData.EndLocation.X - AircraftData.StartLocation.X) * Alpha,
        AircraftData.StartLocation.Y + (AircraftData.EndLocation.Y - AircraftData.StartLocation.Y) * Alpha,
        AircraftData.StartLocation.Z
    );

    if (Alpha >= 1.f)
    {
        AircraftData.bIsFlying = false;
    }
}

// =============================================================================
// Safe Zone (Storm)
// =============================================================================

void AFortGameModeAthena::StartSafeZone()
{
    SafeZoneData.CurrentPhase = 0;

    // Initial zone covers entire map
    SafeZoneData.Center = FVector(0.f, 0.f, 0.f);
    SafeZoneData.Radius = 100000.f; // Very large initial radius

    SetZoneToIndex(0);
    LOG_INFO(LogGame, "Safe zone system activated");
}

void AFortGameModeAthena::SetZoneToIndex(int32 Index)
{
    if (Index < 0 || Index >= static_cast<int32>(ZonePhases.size())) return;

    SafeZoneData.CurrentPhase = Index;
    const auto& Phase = ZonePhases[Index];

    SafeZoneData.WaitTime = Phase.WaitTime;
    SafeZoneData.ShrinkTime = Phase.ShrinkTime;
    SafeZoneData.DamagePerSecond = Phase.Damage;

    // Generate next zone center (random within current zone)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    float MaxOffset = SafeZoneData.Radius * 0.3f;
    std::uniform_real_distribution<float> dist(-MaxOffset, MaxOffset);

    SafeZoneData.NextCenter = FVector(
        SafeZoneData.Center.X + dist(gen),
        SafeZoneData.Center.Y + dist(gen),
        0.f
    );
    SafeZoneData.NextRadius = SafeZoneData.Radius * Phase.RadiusPercent;

    LOG_INFO(LogGame, "Storm phase {} - wait: {}s, shrink: {}s, damage: {}/s",
        Index, Phase.WaitTime, Phase.ShrinkTime, Phase.Damage);
}

void AFortGameModeAthena::TickSafeZone(float DeltaTime)
{
    // Simplified storm tick - would need timer management for wait/shrink states
    (void)DeltaTime;
}

void AFortGameModeAthena::TickStormDamage(float DeltaTime)
{
    if (SafeZoneData.Radius <= 0.f) return;

    for (auto* Player : AlivePlayers)
    {
        if (!Player || !Player->GetPawn()) continue;

        auto* Pawn = Player->GetPawn();
        FVector PlayerPos = Pawn->GetLocation();
        float DistFromCenter = FVector::Dist(PlayerPos, SafeZoneData.Center);

        if (DistFromCenter > SafeZoneData.Radius)
        {
            // Player is in the storm
            FDamageEvent Damage;
            Damage.Damage = SafeZoneData.DamagePerSecond * DeltaTime;
            Pawn->TakeDamage(Damage);
        }
    }
}

void AFortGameModeAthena::CheckForWinner()
{
    if (bMatchEnded) return;

    // Solo mode: if 1 player left, they win
    if (AlivePlayers.size() <= 1 && TotalPlayersAtStart > 1)
    {
        EndMatch();
    }
}

void AFortGameModeAthena::SpawnLoot()
{
    LOG_INFO(LogGame, "Spawning loot...");
    // TODO: Load loot spawn locations from map data
    // Use loot tables from config to determine items
}

void AFortGameModeAthena::SpawnSupplyDrop()
{
    LOG_INFO(LogGame, "Spawning supply drop");
    // TODO: Spawn supply drop at random location in safe zone
}
