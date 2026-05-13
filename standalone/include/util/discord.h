#pragma once

// Discord Webhook Support - kept from original project
// Sends match events (kills, wins, player joins) to Discord

#include "core/platform.h"
#include <string>
#include <vector>

#ifdef WITH_DISCORD

namespace Discord
{
    bool Initialize(const std::string& WebhookURL);
    void Shutdown();

    // Match events
    void SendMatchStarted(int32 PlayerCount);
    void SendMatchEnded(const std::string& WinnerName, int32 Kills);
    void SendPlayerJoined(const std::string& PlayerName, int32 CurrentCount, int32 MaxCount);
    void SendPlayerKill(const std::string& Killer, const std::string& Victim, const std::string& Weapon);
    void SendServerStatus(const std::string& Status);

    // Custom message
    void SendEmbed(const std::string& Title, const std::string& Description, int32 Color = 0x00FF00);
}

#else

// Stub when Discord is disabled
namespace Discord
{
    inline bool Initialize(const std::string&) { return false; }
    inline void Shutdown() {}
    inline void SendMatchStarted(int32) {}
    inline void SendMatchEnded(const std::string&, int32) {}
    inline void SendPlayerJoined(const std::string&, int32, int32) {}
    inline void SendPlayerKill(const std::string&, const std::string&, const std::string&) {}
    inline void SendServerStatus(const std::string&) {}
    inline void SendEmbed(const std::string&, const std::string&, int32 = 0) {}
}

#endif
