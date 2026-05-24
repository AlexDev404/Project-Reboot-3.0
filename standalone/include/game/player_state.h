#pragma once

// Player State - Replicated player information visible to all clients
// Kill count, team, cosmetics, placement, etc.

#include "core/object_system.h"
#include <string>

class AFortPlayerControllerAthena;

// =============================================================================
// Death Info (replicated)
// =============================================================================

struct FDeathInfo
{
    bool bDBNO = false;
    bool bInitialized = false;
    uint8 DeathCause = 0; // EDeathCause
    float Distance = 0.f;
    FVector DeathLocation;
    AFortPlayerControllerAthena* FinisherOrDowner = nullptr;
};

// =============================================================================
// AFortPlayerStateAthena
// =============================================================================

class AFortPlayerStateAthena
{
public:
    AFortPlayerStateAthena() = default;

    // Identity
    std::string GetPlayerName() const { return PlayerName; }
    void SetPlayerName(const std::string& Name) { PlayerName = Name; }
    uint8 GetTeamIndex() const { return TeamIndex; }
    void SetTeamIndex(uint8 Index) { TeamIndex = Index; }
    int32 GetPlayerId() const { return PlayerId; }
    void SetPlayerId(int32 Id) { PlayerId = Id; }

    // Score
    int32 GetKillScore() const { return KillScore; }
    void AddKill() { KillScore++; TeamKillScore++; bNetDirty = true; }
    int32 GetTeamKillScore() const { return TeamKillScore; }
    int32 GetPlace() const { return Place; }
    void SetPlace(int32 InPlace) { Place = InPlace; bNetDirty = true; }

    // State
    bool IsInAircraft() const { return bIsInAircraft; }
    void SetInAircraft(bool bVal) { bIsInAircraft = bVal; bNetDirty = true; }
    bool HasThankedBusDriver() const { return bThankedBusDriver; }
    void ThankBusDriver() { bThankedBusDriver = true; bNetDirty = true; }
    bool IsBot() const { return bIsBot; }
    void SetIsBot(bool bVal) { bIsBot = bVal; }

    // Death
    const FDeathInfo& GetDeathInfo() const { return DeathInfo; }
    void SetDeathInfo(const FDeathInfo& Info) { DeathInfo = Info; bNetDirty = true; }

    // Cosmetics (for replication to other players)
    std::string SkinPath;
    std::string BackblingPath;
    std::string PickaxePath;
    std::string GliderPath;
    std::string ContrailPath;
    std::string LoadingScreenPath;

    bool bNetDirty = false;

private:
    std::string PlayerName;
    uint8 TeamIndex = 0;
    int32 PlayerId = 0;
    int32 KillScore = 0;
    int32 TeamKillScore = 0;
    int32 Place = 0;
    bool bIsInAircraft = true;
    bool bThankedBusDriver = false;
    bool bIsBot = false;
    FDeathInfo DeathInfo;
};
