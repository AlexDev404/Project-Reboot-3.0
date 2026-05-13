#pragma once

// Project Reboot 3.0 - Standalone Server
// Target: Fortnite v17.50 (Season 7 Chapter 2, Engine 4.26)

#include <cstdint>
#include <string>
#include <memory>

// Platform defines
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_LINUX 0
#else
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX 1
#endif

// Type aliases matching UE4 conventions
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

// Forward declarations
class UObject;
class UClass;
class UStruct;
class UFunction;
class UProperty;
class UWorld;
class UEngine;
class AActor;

// Version constants (hardcoded for v17.50)
constexpr double Fortnite_Version = 17.50;
constexpr int Engine_Version = 426;
constexpr int Fortnite_CL = 0; // Not needed in standalone mode

// Server configuration
struct ServerConfig
{
    std::string ServerName = "Project Reboot 3.0";
    uint16 Port = 7777;
    uint16 MaxPlayers = 100;
    std::string PlaylistPath = "/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo";
    bool bEnableBots = false;
    bool bEnableDiscord = false;
    std::string DiscordWebhookURL;
    std::string PakDirectory = "./paks";
    std::string ConfigDirectory = "./config";
    double TickRate = 30.0;
    bool bInfiniteMaterials = false;
    bool bInfiniteAmmo = false;
    bool bLateGame = false;
};

extern ServerConfig GServerConfig;
