# Project Reboot 3.0 - Standalone Server

A standalone dedicated game server extracted from the Project Reboot 3.0 DLL injection project. This version runs as an independent executable without requiring a Fortnite client process.

## Target Version

**Fortnite v17.50** (Season 7 Chapter 2, Unreal Engine 4.26)

## Architecture

```
standalone/
├── CMakeLists.txt          # Build system (CMake)
├── config/                 # Configuration files
│   ├── server.json         # Server settings
│   └── loot_tables.json    # Loot table definitions
├── include/                # Header files
│   ├── core/               # Engine core (object system, assets, events)
│   ├── game/               # Game logic (game mode, players, building)
│   ├── net/                # Networking (ENet-based)
│   └── util/               # Utilities (config, logging, Discord, PAK parser)
└── src/                    # Implementation files
    ├── main.cpp            # Entry point (CLI application)
    ├── core/               # Core implementations
    ├── game/               # Game logic implementations
    ├── net/                # Network implementations
    └── util/               # Utility implementations
```

## Key Changes from Original DLL

| Original (DLL) | Standalone (EXE) |
|----------------|-----------------|
| `DllMain()` + `CreateThread()` | `main()` with game loop |
| MinHook function detours | Direct C++ method calls |
| Memcury pattern scanning | No scanning needed |
| UE4 live object system | Self-contained object system |
| Process memory offsets | Property map (`TMap<FName, Value>`) |
| UE4 `ProcessEvent()` | Custom event dispatcher |
| UE4 Replication Graph | ENet-based replication |
| ImGui overlay | CLI interface |
| Multi-version support | v17.50 only |

## Dependencies

- **ENet** (v1.3.18) - UDP networking library
- **nlohmann/json** (v3.11.3) - JSON configuration
- **spdlog** (v1.13.0) - Logging
- **libcurl** - Discord webhooks (optional)
- **fmt** - String formatting

All dependencies are fetched automatically via CMake FetchContent.

## Building

### Prerequisites

- CMake 3.16+
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- libcurl development headers (optional, for Discord)

### Build Commands

```bash
cd standalone
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Build Options

```bash
cmake .. -DBUILD_WITH_DISCORD=OFF   # Disable Discord webhooks
cmake .. -DBUILD_WITH_ENET=OFF      # Disable networking (testing only)
```

## Running

```bash
./ProjectReboot3 [options]

Options:
  --config <path>       Path to config file (default: config/server.json)
  --port <port>         Server port (default: 7777)
  --max-players <n>     Maximum players (default: 100)
  --playlist <path>     Playlist asset path
  --pak-dir <path>      Directory containing .pak files
  --help, -h            Show help
```

### CLI Commands

Once running, the server accepts these commands:

- `status` - Show server status
- `players` - List connected players
- `kick <name>` - Kick a player
- `quit` / `exit` - Stop the server
- `help` - Show available commands

## Configuration

### server.json

Server configuration including port, player count, tick rate, and game settings.

### loot_tables.json

Defines loot pools with weighted item selection for floor loot, chests, and supply drops.

## PAK File Support

Place Fortnite v17.50 `.pak` files in the configured PAK directory. The server will:
1. Mount all PAK files in the directory
2. Read the PAK index to discover available assets
3. Load DataTables, item definitions, and map data as needed

**Note:** Encrypted PAK files require the AES key to be provided (not yet implemented).

## Current Status

### Implemented
- [x] Build system (CMake, cross-platform)
- [x] CLI entry point (replaces DllMain)
- [x] Self-contained UE4 object system replacement
- [x] Property storage system (replaces memory offsets)
- [x] Event dispatcher (replaces ProcessEvent hooks)
- [x] ENet networking layer
- [x] Actor replication system
- [x] Game mode (Athena BR) - phases, aircraft, storm
- [x] Player controller with all server RPCs
- [x] Player pawn (health, damage, DBNO, movement)
- [x] Inventory system
- [x] PAK file parser
- [x] JSON-based asset loading
- [x] Configuration system
- [x] Discord webhook integration
- [x] Loot table resolution
- [x] Logging with spdlog

### TODO
- [ ] Full UAsset deserialization
- [ ] PAK file decryption (AES)
- [ ] Oodle/Zlib decompression for PAK entries
- [ ] Complete building structural support system
- [ ] Vehicle physics/state machine
- [ ] Bot AI
- [ ] Full weapon mechanics (fire rate, spread, damage falloff)
- [ ] Client compatibility testing
- [ ] Map data loading (spawn points, loot locations)

## Protocol

The server uses a custom binary protocol over ENet UDP:

| Packet Type | Direction | Purpose |
|-------------|-----------|---------|
| `0x00` Hello | C→S | Initial connection |
| `0x01` Welcome | S→C | Server info |
| `0x10` ActorSpawn | S→C | New actor replicated |
| `0x11` ActorDestroy | S→C | Actor removed |
| `0x12` ActorUpdate | S→C | Actor state change |
| `0x20` ServerRPC | C→S | Client calls server function |
| `0x21` ClientRPC | S→C | Server calls client function |
| `0x30` GameStateUpdate | S→C | Match phase, storm, etc. |

## License

Same as the parent Project Reboot 3.0 repository.
