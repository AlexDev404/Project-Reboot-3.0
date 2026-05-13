// Game State implementation

#include "game/game_state.h"
#include "game/player_state.h"
#include "core/engine.h"

#include <algorithm>

void AFortGameStateAthena::AddKillFeedEntry(const std::string& Killer, const std::string& Victim, const std::string& WeaponIcon)
{
    FKillFeedEntry Entry;
    Entry.Killer = Killer;
    Entry.Victim = Victim;
    Entry.WeaponIcon = WeaponIcon;
    Entry.Timestamp = UEngine::Get().GetTimeSeconds();
    KillFeed.push_back(Entry);
    bNetDirty = true;
}

void AFortGameStateAthena::RemovePlayerState(AFortPlayerStateAthena* PS)
{
    PlayerStates.erase(
        std::remove(PlayerStates.begin(), PlayerStates.end(), PS),
        PlayerStates.end()
    );
}
