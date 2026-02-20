# Architecture Guide

This document provides a technical deep-dive into Project Reboot's architecture for maintainers and contributors.

## Table of Contents

1. [Overview](#overview)
2. [Core Components](#core-components)
3. [C++ Game Server (DLL)](#c-game-server-dll)
4. [Icarus Binding Layer](#icarus-binding-layer)
5. [Backend Services](#backend-services)
6. [Build System](#build-system)
7. [Code Patterns](#code-patterns)

---

## Overview

Project Reboot consists of two main components:

1. **C++ DLL** (`Project Reboot 3.0/`): Injected into the Fortnite client, this intercepts and modifies game behavior to enable custom server functionality.

2. **Node.js Backend** (`backend/`): Provides supporting services like account management, authentication, and the Icarus module runtime.

```
┌─────────────────────────────────────────────────────────────┐
│                    Fortnite Client                          │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              Project Reboot DLL (Injected)              ││
│  │  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  ││
│  │  │  Hooking    │  │  Game Logic  │  │    Icarus     │  ││
│  │  │   Layer     │  │  Overrides   │  │   Bindings    │  ││
│  │  └─────────────┘  └──────────────┘  └───────────────┘  ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              │ Network/IPC
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Backend Services                          │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │   Express   │  │   Accounts   │  │  Icarus Module    │  │
│  │   Server    │  │   Service    │  │     Runtime       │  │
│  └─────────────┘  └──────────────┘  └───────────────────┘  │
│                              │                               │
│                      ┌───────────────┐                       │
│                      │   PostgreSQL  │                       │
│                      └───────────────┘                       │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Object System

Project Reboot works with Unreal Engine's object system. Key files:

| File | Purpose |
|------|---------|
| `Object.h/cpp` | Base UObject wrapper |
| `Class.h/cpp` | UClass reflection support |
| `UObjectGlobals.h/cpp` | Global object functions |
| `UObjectArray.h` | Object iteration |

**Key Functions:**

```cpp
// Find an object by path
template <typename T>
T* FindObject(const TCHAR* Name, UClass* Class = nullptr);

// Load an object from disk
template <typename T>
T* LoadObject(const TCHAR* Name, UClass* Class = T::StaticClass());

// Safe casting with type checking
template <typename T>
T* Cast(UObject* Object);
```

### 2. Hooking System

The DLL hooks into Fortnite's functions using MinHook. See `hooking.h`:

```cpp
// Hook a function at a memory address
MH_CreateHook(TargetAddress, DetourFunction, &OriginalFunction);
```

**Common Hook Patterns:**

- **Net Mode Hook**: Returns `NM_DedicatedServer` to simulate a dedicated server
- **Match State Hooks**: Control game flow (warmup, aircraft, zones)
- **Player Spawn Hooks**: Customize player initialization

### 3. Fortnite Classes

The `Fort*.h/cpp` files implement wrappers for Fortnite's game classes:

| Class | Purpose |
|-------|---------|
| `AFortPlayerPawn` | Player character |
| `AFortPlayerController` | Player input handling |
| `AFortPlayerState` | Player state/stats |
| `AFortGameMode` | Match rules |
| `AFortGameState` | Match state |
| `AFortInventory` | Player inventory |

**Example: Getting a Player's Health**

```cpp
auto Pawn = Cast<AFortPlayerPawn>(Controller->GetPawn());
float health = Pawn->GetHealth();
```

---

## C++ Game Server (DLL)

### Entry Point: `dllmain.cpp`

The DLL attaches to the Fortnite process and initializes:

1. **Logger Setup** (`InitLogger()`)
2. **Pattern Scanning** (`finder.h`) - Locates function addresses
3. **Hook Installation** - Redirects game functions
4. **Icarus Initialization** - Sets up JS runtime

### Key Systems

#### Networking (`NetDriver.h/cpp`, `NetConnection.h/cpp`)

Handles network replication between server and clients:

```cpp
// Get the net driver
auto NetDriver = GetWorld()->GetNetDriver();

// Iterate connections
for (auto Connection : NetDriver->GetClientConnections()) {
    // Handle each connected client
}
```

#### Inventory System (`FortInventory.h/cpp`, `FortItem.h/cpp`)

Manages player items:

```cpp
// Give an item to a player
Controller->AddItemToInventory(ItemDef, EFortQuickBars::Primary, Slot, Count);

// Remove an item
Inventory->RemoveItem(ItemGuid, Count);
```

#### Zone System (`FortSafeZoneIndicator.h/cpp`)

Controls the storm/safe zone:

```cpp
auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
GameState->IsSafeZonePaused() = true;  // Pause storm
```

#### Bot System (`bots.h`, `FortServerBotManagerAthena.h/cpp`)

Spawns and controls AI players:

```cpp
// Spawn a bot at location
SpawnBot(Location, Rotation, CosmeticLoadout);
```

---

## Icarus Binding Layer

Icarus bridges C++ game functions to JavaScript, enabling module development.

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     JavaScript Module                        │
│     FWorld.getPawnList().forEach(p => p.setHealth(100))     │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Icarus Runtime                            │
│         (IcarusRuntime.h/cpp - QuickJS embedded)            │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Native Bindings                            │
│    (IcarusBindings.h - C++ functions exposed to JS)         │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                Project Reboot C++ Functions                  │
│           (AFortPlayerPawn, AFortGameMode, etc.)            │
└─────────────────────────────────────────────────────────────┘
```

### Key Files

| File | Purpose |
|------|---------|
| `Icarus.h` | Main header, initialization API |
| `IcarusRuntime.h/cpp` | JS runtime (QuickJS integration) |
| `IcarusBindings.h` | Native function bindings |
| `addon/` | Node.js N-API addon |

### Native Binding Pattern

Each binding follows this pattern:

```cpp
inline JSValue FPawn_SetHealth(const std::vector<JSValue>& args) {
    // 1. Validate arguments
    if (args.size() < 2) {
        return JSValue(false);
    }
    
    // 2. Extract values from JS
    auto Pawn = GetPawnById(args[0].asString());
    float health = static_cast<float>(args[1].asNumber());
    
    // 3. Call Project Reboot C++ function
    Pawn->SetHealth(health);
    
    // 4. Return result to JS
    return JSValue(true);
}
```

### Error Handling: Flare

When a native function fails, errors are wrapped in "Flare" objects:

```cpp
struct Flare {
    std::string sparkId;      // Unique error ID
    std::string severity;     // "LOW", "MEDIUM", "HIGH", "CRITICAL"
    std::string reason;       // Human-readable message
    std::string nativeFunction;
    std::string nativeTrace;
};
```

Modules handle Flares in their `ErrorHandler` function.

---

## Backend Services

The Node.js backend (`backend/`) provides:

### Packages

| Package | Description |
|---------|-------------|
| `@trail-blaze/retroflex` | TypeScript SDK for Icarus bindings |
| `@trail-blaze/flare` | Error handling utilities |
| `@trail-blaze/icarus-core` | Module loading and runtime |

### Routes

| Route | Purpose |
|-------|---------|
| `/auth/*` | Authentication (login, register, tokens) |
| `/account/*` | Account management (2FA, settings) |
| `/analytics/*` | Telemetry and data collection |

### Services

| Service | Purpose |
|---------|---------|
| `AccountService` | User account CRUD |
| `AuthService` | JWT + TOTP authentication |
| `SessionService` | Session management |
| `ModuleManager` | Icarus module lifecycle |

### Database Schema (PostgreSQL)

```sql
-- accounts table
CREATE TABLE accounts (
    id UUID PRIMARY KEY,
    email VARCHAR(255) UNIQUE,
    password_hash VARCHAR(255),
    display_name VARCHAR(64),
    totp_secret VARCHAR(32),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- sessions table
CREATE TABLE sessions (
    id UUID PRIMARY KEY,
    account_id UUID REFERENCES accounts(id),
    token VARCHAR(255),
    expires_at TIMESTAMP,
    ip_address INET
);
```

---

## Build System

### Windows (Primary)

Uses Visual Studio 2022 with MSBuild:

```bash
# Open solution in VS2022
Project Reboot 3.0.sln

# Or build from command line
msbuild "Project Reboot 3.0/Project Reboot 3.0.vcxproj" /p:Configuration=Release /p:Platform=x64
```

### Linux (via Wine)

Requires MSVC toolchain via Wine:

```bash
./build-linux.sh
```

### Backend

```bash
cd backend
npm install
npm run build
```

---

## Code Patterns

### Logging

Use the centralized logger from `log.h`:

```cpp
LOG_INFO(LogDev, "Player {} joined", PlayerName);
LOG_WARN(LogNet, "Connection timeout for {}", ClientId);
LOG_ERROR(LogGame, "Failed to spawn pawn: {}", ErrorMessage);
```

Available log categories: `LogDev`, `LogNet`, `LogPlayer`, `LogLoot`, `LogGame`, `LogAI`, `LogIcarus`, etc.

### Offset Finding

Use the finder system (`finder.h`) to locate memory addresses:

```cpp
auto Offset = Object->GetOffset("PropertyName");
auto Value = Object->Get<Type>(Offset);
```

### Safe Casting

Always use `Cast<T>()` for type-safe casts:

```cpp
auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
if (!GameState) {
    LOG_ERROR(LogDev, "Failed to get game state");
    return;
}
```

### Memory Allocation

Use the `Alloc<T>()` helper:

```cpp
auto Memory = Alloc<MyStruct>();
// Memory is allocated with VirtualAlloc
```

---

## Contributing

1. **Code Style**: Follow existing patterns in the codebase
2. **Testing**: Test changes thoroughly with multiple Fortnite versions
3. **Documentation**: Update docs when adding new features
4. **Pull Requests**: Provide clear descriptions of changes

### Development Workflow

1. Fork the repository
2. Create a feature branch
3. Make changes and test
4. Submit a pull request

For questions, reach out on [Discord](https://discord.gg/rebootmp).
