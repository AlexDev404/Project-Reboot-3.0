#pragma once

// PAK File Parser - Reads Unreal Engine .pak archives
// Used to extract game data (DataTables, item definitions, etc.)
// Based on the UE4 PAK format specification

#include "core/platform.h"
#include "core/object_system.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <memory>

// =============================================================================
// PAK file structures (UE4 format)
// =============================================================================

// PAK magic: 0x5A6F12E1
constexpr uint32 PAK_MAGIC = 0x5A6F12E1;

enum class EPakVersion : int32
{
    Initial = 1,
    NoTimestamps = 2,
    CompressionEncryption = 3,
    IndexEncryption = 4,
    RelativeChunkOffsets = 5,
    DeleteRecords = 6,
    EncryptionKeyGuid = 7,
    FNameBasedCompressionMethod = 8,
    FrozenIndex = 9,
    PathHashIndex = 10,
    Fnv64BugFix = 11,

    Last,
    Latest = Last - 1,
};

struct FPakInfo
{
    uint32 Magic = 0;
    int32 Version = 0;
    int64 IndexOffset = 0;
    int64 IndexSize = 0;
    uint8 IndexHash[20] = {};
    bool bEncryptedIndex = false;
    FGuid EncryptionKeyGuid;
};

struct FPakCompressedBlock
{
    int64 CompressedStart = 0;
    int64 CompressedEnd = 0;
};

struct FPakEntry
{
    std::string Filename;
    int64 Offset = 0;
    int64 Size = 0;
    int64 UncompressedSize = 0;
    uint32 CompressionMethodIndex = 0;
    uint8 Hash[20] = {};
    std::vector<FPakCompressedBlock> CompressionBlocks;
    uint32 Flags = 0;
    uint32 CompressionBlockSize = 0;

    bool IsCompressed() const { return CompressionMethodIndex != 0; }
    bool IsEncrypted() const { return (Flags & 0x01) != 0; }
};

// =============================================================================
// PAK File Reader
// =============================================================================

class FPakFileReader
{
public:
    FPakFileReader() = default;
    ~FPakFileReader();

    // Open a .pak file
    bool Open(const std::string& PakPath);
    void Close();
    bool IsOpen() const { return FileStream.is_open(); }

    // Get info
    const FPakInfo& GetInfo() const { return Info; }
    const std::string& GetPath() const { return FilePath; }

    // Index
    bool ReadIndex();
    const std::vector<FPakEntry>& GetEntries() const { return Entries; }
    const FPakEntry* FindEntry(const std::string& Path) const;

    // Reading file data
    std::vector<uint8> ReadEntry(const FPakEntry& Entry);
    std::vector<uint8> ReadFile(const std::string& Path);

    // List contents
    std::vector<std::string> ListFiles(const std::string& DirectoryFilter = "") const;

private:
    std::string FilePath;
    std::ifstream FileStream;
    FPakInfo Info;
    std::vector<FPakEntry> Entries;
    std::unordered_map<std::string, size_t> EntryMap; // path -> index

    bool ReadPakInfo();
    bool ReadPakIndex();
    std::vector<uint8> DecompressData(const FPakEntry& Entry, const std::vector<uint8>& CompressedData);
};

// =============================================================================
// PAK Manager - manages multiple mounted PAK files
// =============================================================================

class FPakManager
{
public:
    static FPakManager& Get();

    // Mount/unmount PAK files
    bool MountPak(const std::string& PakPath);
    void UnmountPak(const std::string& PakPath);
    void UnmountAll();

    // Mount all PAKs in a directory
    int32 MountDirectory(const std::string& Directory);

    // Read files from any mounted PAK
    std::vector<uint8> ReadFile(const std::string& GamePath) const;
    bool FileExists(const std::string& GamePath) const;

    // List files across all PAKs
    std::vector<std::string> ListFiles(const std::string& DirectoryFilter = "") const;

    // Info
    int32 GetMountedPakCount() const { return static_cast<int32>(MountedPaks.size()); }
    std::vector<std::string> GetMountedPakPaths() const;

private:
    FPakManager() = default;
    std::vector<std::unique_ptr<FPakFileReader>> MountedPaks;
};
