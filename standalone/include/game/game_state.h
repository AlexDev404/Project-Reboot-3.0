#pragma once

// Game State - Global match state replicated to all clients
// Storm, players alive, match time, etc.

#include "core/object_system.h"
#include "game_mode.h"
#include <vector>
#include <string>

class AFortPlayerStateAthena;

// =============================================================================
// AFortGameStateAthena
// =============================================================================

class AFortGameStateAthena
{
public:
    AFortGameStateAthena() = default;

    // Match info
    EAthenaGamePhase GetGamePhase() const { return GamePhase; }
    void SetGamePhase(EAthenaGamePhase Phase) { GamePhase = Phase; bNetDirty = true; }

    int32 GetPlayersLeft() const { return PlayersLeft; }
    void SetPlayersLeft(int32 Count) { PlayersLeft = Count; bNetDirty = true; }

    int32 GetTotalPlayers() const { return TotalPlayers; }
    void SetTotalPlayers(int32 Count) { TotalPlayers = Count; }

    // Safe Zone
    FVector GetSafeZoneCenter() const { return SafeZoneCenter; }
    float GetSafeZoneRadius() const { return SafeZoneRadius; }
    FVector GetNextSafeZoneCenter() const { return NextSafeZoneCenter; }
    float GetNextSafeZoneRadius() const { return NextSafeZoneRadius; }

    void SetSafeZone(const FVector& Center, float Radius) {
        SafeZoneCenter = Center;
        SafeZoneRadius = Radius;
        bNetDirty = true;
    }
    void SetNextSafeZone(const FVector& Center, float Radius) {
        NextSafeZoneCenter = Center;
        NextSafeZoneRadius = Radius;
        bNetDirty = true;
    }

    // Aircraft
    float GetAircraftStartTime() const { return AircraftStartTime; }
    void SetAircraftStartTime(float Time) { AircraftStartTime = Time; bNetDirty = true; }

    // Time
    float GetElapsedTime() const { return ElapsedTime; }
    void SetElapsedTime(float Time) { ElapsedTime = Time; }

    // Event log
    void AddKillFeedEntry(const std::string& Killer, const std::string& Victim, const std::string& WeaponIcon);
    struct FKillFeedEntry {
        std::string Killer;
        std::string Victim;
        std::string WeaponIcon;
        double Timestamp;
    };
    const std::vector<FKillFeedEntry>& GetKillFeed() const { return KillFeed; }

    // Player states
    void AddPlayerState(AFortPlayerStateAthena* PS) { PlayerStates.push_back(PS); }
    void RemovePlayerState(AFortPlayerStateAthena* PS);
    const std::vector<AFortPlayerStateAthena*>& GetPlayerStates() const { return PlayerStates; }

    bool bNetDirty = false;

private:
    EAthenaGamePhase GamePhase = EAthenaGamePhase::None;
    int32 PlayersLeft = 0;
    int32 TotalPlayers = 0;

    FVector SafeZoneCenter;
    float SafeZoneRadius = 0.f;
    FVector NextSafeZoneCenter;
    float NextSafeZoneRadius = 0.f;

    float AircraftStartTime = 0.f;
    float ElapsedTime = 0.f;

    std::vector<FKillFeedEntry> KillFeed;
    std::vector<AFortPlayerStateAthena*> PlayerStates;
};
