#pragma once

// Configuration system - loads settings from JSON/TOML files
// Replaces hardcoded globals.h values

#include "core/platform.h"
#include <string>
#include <vector>

class Config
{
public:
    static Config& Get();

    bool Load(const std::string& ConfigPath);
    bool Save(const std::string& ConfigPath) const;

    // Access the server config
    ServerConfig& GetServerConfig() { return GServerConfig; }
    const ServerConfig& GetServerConfig() const { return GServerConfig; }

    // Loot tables
    struct FLootPool {
        std::string Name;
        std::vector<std::string> Items;
        std::vector<float> Weights;
        int32 MinCount = 1;
        int32 MaxCount = 1;
    };

    struct FLootTable {
        std::string Name;
        std::vector<FLootPool> Pools;
    };

    bool LoadLootTables(const std::string& LootTablePath);
    const std::vector<FLootTable>& GetLootTables() const { return LootTables; }
    const FLootTable* FindLootTable(const std::string& Name) const;

    // Storm config
    struct FStormPhase {
        float WaitTime;
        float ShrinkTime;
        float RadiusPercent; // 0.0 - 1.0
        float Damage;
    };
    const std::vector<FStormPhase>& GetStormPhases() const { return StormPhases; }

private:
    Config() = default;
    std::vector<FLootTable> LootTables;
    std::vector<FStormPhase> StormPhases;
};
