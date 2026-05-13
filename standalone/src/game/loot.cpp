// Loot system - handles loot spawning and loot table resolution

#include "game/game_mode.h"
#include "util/config.h"
#include "util/logging.h"
#include "core/object_system.h"

#include <random>

namespace Loot
{

// Resolve a loot table into actual items
struct FLootResult
{
    std::string ItemDefinitionPath;
    int32 Count;
};

std::vector<FLootResult> ResolveLootTable(const std::string& LootTableName)
{
    std::vector<FLootResult> Results;

    const auto* Table = Config::Get().FindLootTable(LootTableName);
    if (!Table)
    {
        LOG_WARN(LogLoot, "Loot table '{}' not found", LootTableName);
        return Results;
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());

    for (const auto& Pool : Table->Pools)
    {
        if (Pool.Items.empty() || Pool.Weights.empty()) continue;

        // Determine count
        std::uniform_int_distribution<int32> countDist(Pool.MinCount, Pool.MaxCount);
        int32 Count = countDist(gen);

        for (int32 i = 0; i < Count; i++)
        {
            // Weighted random selection
            std::discrete_distribution<int> dist(Pool.Weights.begin(), Pool.Weights.end());
            int SelectedIndex = dist(gen);

            if (SelectedIndex < static_cast<int>(Pool.Items.size()))
            {
                FLootResult Result;
                Result.ItemDefinitionPath = Pool.Items[SelectedIndex];
                Result.Count = 1;
                Results.push_back(Result);
            }
        }
    }

    return Results;
}

} // namespace Loot
