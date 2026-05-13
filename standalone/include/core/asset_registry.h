#pragma once

// Asset Registry - Replaces StaticLoadObject for loading game data
// Loads assets from PAK files or extracted JSON/binary data

#include "object_system.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <filesystem>

// Forward declarations
class UDataTable;
class UCurveTable;

// =============================================================================
// Asset types we need to support
// =============================================================================

enum class EAssetType
{
    Unknown,
    DataTable,
    CurveTable,
    ItemDefinition,
    WeaponDefinition,
    PlaylistDefinition,
    CosmeticDefinition,
    Map,
    Blueprint,
};

// =============================================================================
// Asset Registry
// =============================================================================

class UAssetRegistry
{
public:
    static UAssetRegistry& Get();

    // Initialize from config
    bool Initialize(const std::string& PakDirectory, const std::string& DataDirectory);

    // Asset loading
    UObject* LoadAsset(const std::string& AssetPath);
    bool IsAssetLoaded(const std::string& AssetPath) const;

    // Typed loaders
    UDataTable* LoadDataTable(const std::string& Path);

    // Bulk operations
    void LoadAllAssetsOfType(EAssetType Type);

    // Query
    std::vector<std::string> GetAssetPaths(EAssetType Type) const;
    std::vector<UObject*> GetAllAssetsOfClass(UClass* Class) const;

    // PAK file support
    bool MountPakFile(const std::string& PakPath);
    std::vector<std::string> GetMountedPaks() const;

private:
    UAssetRegistry() = default;

    struct FAssetData {
        std::string Path;
        EAssetType Type;
        std::shared_ptr<UObject> LoadedAsset;
        bool bLoaded = false;
    };

    std::unordered_map<std::string, FAssetData> AssetMap;
    std::vector<std::string> MountedPaks;
    std::string PakDir;
    std::string DataDir;

    // Internal loaders
    UObject* LoadFromJson(const std::string& JsonPath);
    UObject* LoadFromPak(const std::string& AssetPath);
};
