// PAK file parser implementation

#include "util/pak_parser.h"
#include "util/logging.h"

#include <filesystem>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

// =============================================================================
// FPakFileReader
// =============================================================================

FPakFileReader::~FPakFileReader()
{
    Close();
}

bool FPakFileReader::Open(const std::string& PakPath)
{
    Close();

    FilePath = PakPath;
    FileStream.open(PakPath, std::ios::binary);

    if (!FileStream.is_open())
    {
        LOG_ERROR(LogAsset, "Failed to open PAK file: {}", PakPath);
        return false;
    }

    if (!ReadPakInfo())
    {
        LOG_ERROR(LogAsset, "Failed to read PAK info from: {}", PakPath);
        Close();
        return false;
    }

    if (!ReadIndex())
    {
        LOG_WARN(LogAsset, "Failed to read PAK index from: {} (may be encrypted)", PakPath);
        // Don't close - some operations might still work
    }

    return true;
}

void FPakFileReader::Close()
{
    if (FileStream.is_open())
    {
        FileStream.close();
    }
    Entries.clear();
    EntryMap.clear();
    FilePath.clear();
}

bool FPakFileReader::ReadPakInfo()
{
    // PAK info is at the end of the file
    // Seek to end - sizeof(FPakInfo) and read

    FileStream.seekg(0, std::ios::end);
    auto FileSize = FileStream.tellg();

    if (FileSize < 44) return false; // Minimum PAK info size

    // Try reading the footer (varies by version)
    // Version 8+ footer: Magic(4) + Version(4) + IndexOffset(8) + IndexSize(8) + Hash(20) + Encrypted(1) + GUID(16)
    const int64 FooterSize = 221; // Maximum footer size for latest versions
    int64 FooterOffset = static_cast<int64>(FileSize) - FooterSize;
    if (FooterOffset < 0) FooterOffset = 0;

    // Scan backwards for magic
    for (int64 Offset = static_cast<int64>(FileSize) - 44; Offset >= FooterOffset; Offset--)
    {
        FileStream.seekg(Offset);
        uint32 Magic = 0;
        FileStream.read(reinterpret_cast<char*>(&Magic), 4);

        if (Magic == PAK_MAGIC)
        {
            Info.Magic = Magic;
            FileStream.read(reinterpret_cast<char*>(&Info.Version), 4);
            FileStream.read(reinterpret_cast<char*>(&Info.IndexOffset), 8);
            FileStream.read(reinterpret_cast<char*>(&Info.IndexSize), 8);
            FileStream.read(reinterpret_cast<char*>(Info.IndexHash), 20);

            if (Info.Version >= static_cast<int32>(EPakVersion::EncryptionKeyGuid))
            {
                uint8 bEncrypted = 0;
                FileStream.read(reinterpret_cast<char*>(&bEncrypted), 1);
                Info.bEncryptedIndex = bEncrypted != 0;

                FileStream.read(reinterpret_cast<char*>(&Info.EncryptionKeyGuid), 16);
            }

            LOG_INFO(LogAsset, "PAK version: {}, index offset: {}, index size: {}",
                Info.Version, Info.IndexOffset, Info.IndexSize);
            return true;
        }
    }

    LOG_ERROR(LogAsset, "Could not find PAK magic in file");
    return false;
}

bool FPakFileReader::ReadIndex()
{
    if (Info.bEncryptedIndex)
    {
        LOG_WARN(LogAsset, "PAK index is encrypted - decryption not yet supported");
        return false;
    }

    if (Info.IndexOffset <= 0 || Info.IndexSize <= 0)
    {
        return false;
    }

    FileStream.seekg(Info.IndexOffset);

    // Read mount point
    int32 MountPointLen = 0;
    FileStream.read(reinterpret_cast<char*>(&MountPointLen), 4);

    if (MountPointLen < 0 || MountPointLen > 4096)
    {
        LOG_ERROR(LogAsset, "Invalid mount point length: {}", MountPointLen);
        return false;
    }

    std::string MountPoint(MountPointLen, '\0');
    FileStream.read(MountPoint.data(), MountPointLen);
    // Trim null terminator
    if (!MountPoint.empty() && MountPoint.back() == '\0')
        MountPoint.pop_back();

    // Read entry count
    int32 EntryCount = 0;
    FileStream.read(reinterpret_cast<char*>(&EntryCount), 4);

    if (EntryCount < 0 || EntryCount > 1000000) // Sanity check
    {
        LOG_ERROR(LogAsset, "Invalid entry count: {}", EntryCount);
        return false;
    }

    LOG_INFO(LogAsset, "PAK has {} entries, mount: '{}'", EntryCount, MountPoint);

    Entries.reserve(EntryCount);
    for (int32 i = 0; i < EntryCount; i++)
    {
        FPakEntry Entry;

        // Read filename
        int32 FilenameLen = 0;
        FileStream.read(reinterpret_cast<char*>(&FilenameLen), 4);

        if (FilenameLen <= 0 || FilenameLen > 4096) break;

        Entry.Filename.resize(FilenameLen);
        FileStream.read(Entry.Filename.data(), FilenameLen);
        if (!Entry.Filename.empty() && Entry.Filename.back() == '\0')
            Entry.Filename.pop_back();

        // Prepend mount point
        if (!MountPoint.empty() && MountPoint != "../../../")
        {
            Entry.Filename = MountPoint + Entry.Filename;
        }

        // Read entry info
        FileStream.read(reinterpret_cast<char*>(&Entry.Offset), 8);
        FileStream.read(reinterpret_cast<char*>(&Entry.Size), 8);
        FileStream.read(reinterpret_cast<char*>(&Entry.UncompressedSize), 8);
        FileStream.read(reinterpret_cast<char*>(&Entry.CompressionMethodIndex), 4);

        if (Info.Version < static_cast<int32>(EPakVersion::NoTimestamps))
        {
            int64 Timestamp = 0;
            FileStream.read(reinterpret_cast<char*>(&Timestamp), 8);
        }

        FileStream.read(reinterpret_cast<char*>(Entry.Hash), 20);

        if (Entry.CompressionMethodIndex != 0)
        {
            int32 BlockCount = 0;
            FileStream.read(reinterpret_cast<char*>(&BlockCount), 4);
            Entry.CompressionBlocks.resize(BlockCount);
            for (int32 b = 0; b < BlockCount; b++)
            {
                FileStream.read(reinterpret_cast<char*>(&Entry.CompressionBlocks[b].CompressedStart), 8);
                FileStream.read(reinterpret_cast<char*>(&Entry.CompressionBlocks[b].CompressedEnd), 8);
            }
        }

        FileStream.read(reinterpret_cast<char*>(&Entry.Flags), 1);
        FileStream.read(reinterpret_cast<char*>(&Entry.CompressionBlockSize), 4);

        size_t Index = Entries.size();
        EntryMap[Entry.Filename] = Index;
        Entries.push_back(std::move(Entry));
    }

    LOG_INFO(LogAsset, "Successfully read {} PAK entries", Entries.size());
    return true;
}

const FPakEntry* FPakFileReader::FindEntry(const std::string& Path) const
{
    auto It = EntryMap.find(Path);
    if (It != EntryMap.end())
    {
        return &Entries[It->second];
    }

    // Try case-insensitive search
    std::string LowerPath = Path;
    std::transform(LowerPath.begin(), LowerPath.end(), LowerPath.begin(), ::tolower);

    for (const auto& Entry : Entries)
    {
        std::string LowerEntry = Entry.Filename;
        std::transform(LowerEntry.begin(), LowerEntry.end(), LowerEntry.begin(), ::tolower);
        if (LowerEntry == LowerPath) return &Entry;
    }

    return nullptr;
}

std::vector<uint8> FPakFileReader::ReadEntry(const FPakEntry& Entry)
{
    if (!FileStream.is_open()) return {};
    if (Entry.IsEncrypted())
    {
        LOG_WARN(LogAsset, "Cannot read encrypted entry: {}", Entry.Filename);
        return {};
    }

    if (Entry.IsCompressed())
    {
        // Read compressed data and decompress
        std::vector<uint8> CompressedData(Entry.Size);
        FileStream.seekg(Entry.Offset);
        // Skip entry header in data (offset points to serialized header + data)
        // For simplicity, skip the 53-byte inline header
        FileStream.seekg(Entry.Offset + 53);
        FileStream.read(reinterpret_cast<char*>(CompressedData.data()), Entry.Size);
        return DecompressData(Entry, CompressedData);
    }

    // Uncompressed read
    std::vector<uint8> Data(Entry.UncompressedSize > 0 ? Entry.UncompressedSize : Entry.Size);
    FileStream.seekg(Entry.Offset + 53); // Skip inline header
    FileStream.read(reinterpret_cast<char*>(Data.data()), Data.size());

    return Data;
}

std::vector<uint8> FPakFileReader::ReadFile(const std::string& Path)
{
    const FPakEntry* Entry = FindEntry(Path);
    if (!Entry) return {};
    return ReadEntry(*Entry);
}

std::vector<std::string> FPakFileReader::ListFiles(const std::string& DirectoryFilter) const
{
    std::vector<std::string> Result;
    for (const auto& Entry : Entries)
    {
        if (DirectoryFilter.empty() || Entry.Filename.find(DirectoryFilter) == 0)
        {
            Result.push_back(Entry.Filename);
        }
    }
    return Result;
}

std::vector<uint8> FPakFileReader::DecompressData(const FPakEntry& Entry, const std::vector<uint8>& CompressedData)
{
    // TODO: Implement Zlib/Oodle decompression based on CompressionMethodIndex
    // For now, return empty - compressed PAK entries won't work yet
    LOG_WARN(LogAsset, "Decompression not yet implemented for: {}", Entry.Filename);
    return {};
}

// =============================================================================
// FPakManager
// =============================================================================

FPakManager& FPakManager::Get()
{
    static FPakManager Instance;
    return Instance;
}

bool FPakManager::MountPak(const std::string& PakPath)
{
    auto Reader = std::make_unique<FPakFileReader>();
    if (!Reader->Open(PakPath))
    {
        return false;
    }

    LOG_INFO(LogAsset, "Mounted PAK: {} ({} files)",
        fs::path(PakPath).filename().string(), Reader->GetEntries().size());

    MountedPaks.push_back(std::move(Reader));
    return true;
}

void FPakManager::UnmountPak(const std::string& PakPath)
{
    MountedPaks.erase(
        std::remove_if(MountedPaks.begin(), MountedPaks.end(),
            [&PakPath](const std::unique_ptr<FPakFileReader>& P) {
                return P->GetPath() == PakPath;
            }),
        MountedPaks.end()
    );
}

void FPakManager::UnmountAll()
{
    MountedPaks.clear();
}

int32 FPakManager::MountDirectory(const std::string& Directory)
{
    if (!fs::exists(Directory) || !fs::is_directory(Directory))
    {
        LOG_WARN(LogAsset, "PAK directory does not exist: {}", Directory);
        return 0;
    }

    int32 Count = 0;
    for (const auto& Entry : fs::directory_iterator(Directory))
    {
        if (Entry.is_regular_file())
        {
            auto Ext = Entry.path().extension().string();
            std::transform(Ext.begin(), Ext.end(), Ext.begin(), ::tolower);
            if (Ext == ".pak")
            {
                if (MountPak(Entry.path().string()))
                {
                    Count++;
                }
            }
        }
    }

    return Count;
}

std::vector<uint8> FPakManager::ReadFile(const std::string& GamePath) const
{
    // Search all mounted PAKs in reverse order (later PAKs override earlier ones)
    for (auto It = MountedPaks.rbegin(); It != MountedPaks.rend(); ++It)
    {
        auto Data = (*It)->ReadFile(GamePath);
        if (!Data.empty()) return Data;
    }
    return {};
}

bool FPakManager::FileExists(const std::string& GamePath) const
{
    for (const auto& Pak : MountedPaks)
    {
        if (Pak->FindEntry(GamePath)) return true;
    }
    return false;
}

std::vector<std::string> FPakManager::ListFiles(const std::string& DirectoryFilter) const
{
    std::vector<std::string> Result;
    for (const auto& Pak : MountedPaks)
    {
        auto Files = Pak->ListFiles(DirectoryFilter);
        Result.insert(Result.end(), Files.begin(), Files.end());
    }
    // Remove duplicates
    std::sort(Result.begin(), Result.end());
    Result.erase(std::unique(Result.begin(), Result.end()), Result.end());
    return Result;
}

std::vector<std::string> FPakManager::GetMountedPakPaths() const
{
    std::vector<std::string> Paths;
    for (const auto& Pak : MountedPaks)
    {
        Paths.push_back(Pak->GetPath());
    }
    return Paths;
}
