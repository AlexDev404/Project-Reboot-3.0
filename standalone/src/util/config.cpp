// Configuration system implementation

#include "util/config.h"
#include "util/logging.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

Config& Config::Get()
{
    static Config Instance;
    return Instance;
}

bool Config::Load(const std::string& ConfigPath)
{
    if (!fs::exists(ConfigPath))
    {
        // Create default config
        Save(ConfigPath);
        return false;
    }

    try
    {
        std::ifstream File(ConfigPath);
        json J;
        File >> J;

        // Server settings
        if (J.contains("server"))
        {
            auto& S = J["server"];
            if (S.contains("name")) GServerConfig.ServerName = S["name"];
            if (S.contains("port")) GServerConfig.Port = S["port"];
            if (S.contains("maxPlayers")) GServerConfig.MaxPlayers = S["maxPlayers"];
            if (S.contains("tickRate")) GServerConfig.TickRate = S["tickRate"];
            if (S.contains("playlist")) GServerConfig.PlaylistPath = S["playlist"];
            if (S.contains("pakDirectory")) GServerConfig.PakDirectory = S["pakDirectory"];
            if (S.contains("configDirectory")) GServerConfig.ConfigDirectory = S["configDirectory"];
        }

        // Game settings
        if (J.contains("game"))
        {
            auto& G = J["game"];
            if (G.contains("infiniteMaterials")) GServerConfig.bInfiniteMaterials = G["infiniteMaterials"];
            if (G.contains("infiniteAmmo")) GServerConfig.bInfiniteAmmo = G["infiniteAmmo"];
            if (G.contains("lateGame")) GServerConfig.bLateGame = G["lateGame"];
            if (G.contains("enableBots")) GServerConfig.bEnableBots = G["enableBots"];
        }

        // Discord
        if (J.contains("discord"))
        {
            auto& D = J["discord"];
            if (D.contains("enabled")) GServerConfig.bEnableDiscord = D["enabled"];
            if (D.contains("webhookUrl")) GServerConfig.DiscordWebhookURL = D["webhookUrl"];
        }

        // Storm phases
        if (J.contains("storm"))
        {
            StormPhases.clear();
            for (auto& Phase : J["storm"])
            {
                FStormPhase SP;
                SP.WaitTime = Phase.value("waitTime", 60.f);
                SP.ShrinkTime = Phase.value("shrinkTime", 60.f);
                SP.RadiusPercent = Phase.value("radiusPercent", 0.5f);
                SP.Damage = Phase.value("damage", 1.f);
                StormPhases.push_back(SP);
            }
        }

        return true;
    }
    catch (const std::exception& E)
    {
        LOG_ERROR(LogConfig, "Failed to load config: {}", E.what());
        return false;
    }
}

bool Config::Save(const std::string& ConfigPath) const
{
    try
    {
        // Ensure directory exists
        fs::path Path(ConfigPath);
        if (Path.has_parent_path())
        {
            fs::create_directories(Path.parent_path());
        }

        json J;

        // Server
        J["server"] = {
            {"name", GServerConfig.ServerName},
            {"port", GServerConfig.Port},
            {"maxPlayers", GServerConfig.MaxPlayers},
            {"tickRate", GServerConfig.TickRate},
            {"playlist", GServerConfig.PlaylistPath},
            {"pakDirectory", GServerConfig.PakDirectory},
            {"configDirectory", GServerConfig.ConfigDirectory},
        };

        // Game
        J["game"] = {
            {"infiniteMaterials", GServerConfig.bInfiniteMaterials},
            {"infiniteAmmo", GServerConfig.bInfiniteAmmo},
            {"lateGame", GServerConfig.bLateGame},
            {"enableBots", GServerConfig.bEnableBots},
        };

        // Discord
        J["discord"] = {
            {"enabled", GServerConfig.bEnableDiscord},
            {"webhookUrl", GServerConfig.DiscordWebhookURL},
        };

        // Storm (default phases)
        J["storm"] = json::array();
        if (StormPhases.empty())
        {
            // Write defaults
            struct Def { float w, s, r, d; };
            Def defaults[] = {
                {120, 90, 0.60f, 1}, {90, 75, 0.40f, 1}, {60, 60, 0.25f, 2},
                {45, 45, 0.15f, 5}, {30, 30, 0.08f, 8}, {20, 20, 0.04f, 10},
                {15, 15, 0.01f, 10}, {10, 10, 0.00f, 10}
            };
            for (auto& d : defaults)
            {
                json phase;
                phase["waitTime"] = d.w;
                phase["shrinkTime"] = d.s;
                phase["radiusPercent"] = d.r;
                phase["damage"] = d.d;
                J["storm"].push_back(phase);
            }
        }
        else
        {
            for (const auto& Phase : StormPhases)
            {
                json phase;
                phase["waitTime"] = Phase.WaitTime;
                phase["shrinkTime"] = Phase.ShrinkTime;
                phase["radiusPercent"] = Phase.RadiusPercent;
                phase["damage"] = Phase.Damage;
                J["storm"].push_back(phase);
            }
        }

        std::ofstream File(ConfigPath);
        File << J.dump(2);
        return true;
    }
    catch (const std::exception& E)
    {
        LOG_ERROR(LogConfig, "Failed to save config: {}", E.what());
        return false;
    }
}

bool Config::LoadLootTables(const std::string& LootTablePath)
{
    if (!fs::exists(LootTablePath))
    {
        LOG_WARN(LogConfig, "Loot table file not found: {}", LootTablePath);
        return false;
    }

    try
    {
        std::ifstream File(LootTablePath);
        json J;
        File >> J;

        LootTables.clear();
        for (auto& [Name, TableJson] : J.items())
        {
            FLootTable Table;
            Table.Name = Name;

            if (TableJson.contains("pools"))
            {
                for (auto& PoolJson : TableJson["pools"])
                {
                    FLootPool Pool;
                    Pool.Name = PoolJson.value("name", "");
                    Pool.MinCount = PoolJson.value("minCount", 1);
                    Pool.MaxCount = PoolJson.value("maxCount", 1);

                    if (PoolJson.contains("items"))
                    {
                        for (auto& Item : PoolJson["items"])
                        {
                            Pool.Items.push_back(Item.value("path", ""));
                            Pool.Weights.push_back(Item.value("weight", 1.0f));
                        }
                    }

                    Table.Pools.push_back(Pool);
                }
            }

            LootTables.push_back(Table);
        }

        LOG_INFO(LogConfig, "Loaded {} loot tables", LootTables.size());
        return true;
    }
    catch (const std::exception& E)
    {
        LOG_ERROR(LogConfig, "Failed to load loot tables: {}", E.what());
        return false;
    }
}

const Config::FLootTable* Config::FindLootTable(const std::string& Name) const
{
    for (const auto& Table : LootTables)
    {
        if (Table.Name == Name) return &Table;
    }
    return nullptr;
}
