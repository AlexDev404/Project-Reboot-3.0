// Asset Registry implementation

#include "core/asset_registry.h"
#include "core/object_globals.h"
#include "util/pak_parser.h"
#include "util/logging.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

UAssetRegistry& UAssetRegistry::Get()
{
    static UAssetRegistry Instance;
    return Instance;
}

bool UAssetRegistry::Initialize(const std::string& PakDirectory, const std::string& DataDirectory)
{
    PakDir = PakDirectory;
    DataDir = DataDirectory;

    LOG_INFO(LogAsset, "Initializing asset registry...");
    LOG_INFO(LogAsset, "  PAK directory: {}", PakDir);
    LOG_INFO(LogAsset, "  Data directory: {}", DataDir);

    // Load JSON data files from the data directory
    if (fs::exists(DataDir))
    {
        for (const auto& Entry : fs::recursive_directory_iterator(DataDir))
        {
            if (Entry.is_regular_file() && Entry.path().extension() == ".json")
            {
                std::string RelPath = fs::relative(Entry.path(), DataDir).string();
                // Convert filesystem path to game asset path
                std::string GamePath = "/Game/" + RelPath;
                // Remove .json extension
                GamePath = GamePath.substr(0, GamePath.length() - 5);

                FAssetData AssetData;
                AssetData.Path = GamePath;
                AssetData.Type = EAssetType::Unknown; // Will be determined on load
                AssetMap[GamePath] = AssetData;
            }
        }

        LOG_INFO(LogAsset, "Discovered {} JSON data files", AssetMap.size());
    }
    else
    {
        LOG_WARN(LogAsset, "Data directory '{}' does not exist", DataDir);
    }

    return true;
}

UObject* UAssetRegistry::LoadAsset(const std::string& AssetPath)
{
    // Check if already loaded
    auto It = AssetMap.find(AssetPath);
    if (It != AssetMap.end() && It->second.bLoaded)
    {
        return It->second.LoadedAsset.get();
    }

    // Try to load from JSON first
    std::string JsonPath = DataDir + "/" + AssetPath.substr(6) + ".json"; // Remove "/Game/"
    if (fs::exists(JsonPath))
    {
        return LoadFromJson(JsonPath);
    }

    // Try PAK files
    return LoadFromPak(AssetPath);
}

bool UAssetRegistry::IsAssetLoaded(const std::string& AssetPath) const
{
    auto It = AssetMap.find(AssetPath);
    return It != AssetMap.end() && It->second.bLoaded;
}

UDataTable* UAssetRegistry::LoadDataTable(const std::string& Path)
{
    return reinterpret_cast<UDataTable*>(LoadAsset(Path));
}

bool UAssetRegistry::MountPakFile(const std::string& PakPath)
{
    if (FPakManager::Get().MountPak(PakPath))
    {
        MountedPaks.push_back(PakPath);
        return true;
    }
    return false;
}

std::vector<std::string> UAssetRegistry::GetMountedPaks() const
{
    return MountedPaks;
}

std::vector<std::string> UAssetRegistry::GetAssetPaths(EAssetType Type) const
{
    std::vector<std::string> Result;
    for (const auto& [Path, Data] : AssetMap)
    {
        if (Data.Type == Type)
        {
            Result.push_back(Path);
        }
    }
    return Result;
}

std::vector<UObject*> UAssetRegistry::GetAllAssetsOfClass(UClass* Class) const
{
    std::vector<UObject*> Result;
    for (const auto& [Path, Data] : AssetMap)
    {
        if (Data.bLoaded && Data.LoadedAsset && Data.LoadedAsset->IsA(Class))
        {
            Result.push_back(Data.LoadedAsset.get());
        }
    }
    return Result;
}

void UAssetRegistry::LoadAllAssetsOfType(EAssetType Type)
{
    for (auto& [Path, Data] : AssetMap)
    {
        if (Data.Type == Type && !Data.bLoaded)
        {
            LoadAsset(Path);
        }
    }
}

UObject* UAssetRegistry::LoadFromJson(const std::string& JsonPath)
{
    try
    {
        std::ifstream File(JsonPath);
        if (!File.is_open()) return nullptr;

        json J;
        File >> J;

        // Create a UObject to hold the data
        auto Obj = std::make_shared<UObject>();

        // Parse JSON properties into the object
        if (J.contains("Name"))
        {
            // Object name from JSON
        }

        if (J.contains("Properties"))
        {
            for (auto& [Key, Value] : J["Properties"].items())
            {
                if (Value.is_string())
                    Obj->SetProperty(FName(Key), FPropertyValue(Value.get<std::string>()));
                else if (Value.is_number_integer())
                    Obj->SetProperty(FName(Key), FPropertyValue(Value.get<int32>()));
                else if (Value.is_number_float())
                    Obj->SetProperty(FName(Key), FPropertyValue(Value.get<float>()));
                else if (Value.is_boolean())
                    Obj->SetProperty(FName(Key), FPropertyValue(Value.get<bool>()));
            }
        }

        // Store in registry
        std::string GamePath = "/Game/" + fs::relative(JsonPath, DataDir).string();
        GamePath = GamePath.substr(0, GamePath.length() - 5); // remove .json

        FAssetData Data;
        Data.Path = GamePath;
        Data.LoadedAsset = Obj;
        Data.bLoaded = true;
        AssetMap[GamePath] = Data;

        // Also register in the global object system
        UObjectGlobals::Get().RegisterObject(GamePath, Obj);

        return Obj.get();
    }
    catch (const std::exception& E)
    {
        LOG_ERROR(LogAsset, "Failed to load JSON asset '{}': {}", JsonPath, E.what());
        return nullptr;
    }
}

UObject* UAssetRegistry::LoadFromPak(const std::string& AssetPath)
{
    // Read raw data from PAK
    auto Data = FPakManager::Get().ReadFile(AssetPath + ".uasset");
    if (Data.empty())
    {
        // Try with .umap extension for maps
        Data = FPakManager::Get().ReadFile(AssetPath + ".umap");
    }

    if (Data.empty())
    {
        LOG_DEBUG(LogAsset, "Asset not found in PAK files: {}", AssetPath);
        return nullptr;
    }

    // TODO: Full UAsset deserialization
    // For now, create a placeholder object
    auto Obj = std::make_shared<UObject>();

    FAssetData AssetData;
    AssetData.Path = AssetPath;
    AssetData.LoadedAsset = Obj;
    AssetData.bLoaded = true;
    AssetMap[AssetPath] = AssetData;

    UObjectGlobals::Get().RegisterObject(AssetPath, Obj);

    return Obj.get();
}
