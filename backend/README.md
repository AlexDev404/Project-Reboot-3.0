# Icarus - The Blaze Backend

> *"Icarus laughed as he fell. Threw his head back and yelled into the winds, arms spread wide, teeth bared to the world."*

**Icarus is a JavaScript/TypeScript binding layer that exposes Project Reboot's C++ game functions to JavaScript**, enabling developers to write Fortnite modifications using modern JavaScript through a technology known as "modules".

## Core Architecture

### The Foundation: C++ to JavaScript Bindings

At its core, Icarus binds all the core functions of Project Reboot into JavaScript with TypeScript definitions. This allows developers to:

- Write game modifications in TypeScript/JavaScript instead of C++
- Use modern development tools and practices
- Create reusable modules that extend server functionality

### Packages

| Package | Description |
|---------|-------------|
| `@trail-blaze/retroflex` | **Core SDK** - TypeScript bindings for Project Reboot C++ functions |
| `@trail-blaze/flare` | Error handling system for catching C++ binding exceptions |
| `@trail-blaze/icarus-core` | Module system and runtime for loading/executing modules |

### Account Management & More (Built on top of the core)

The backend also includes supplementary features:
- Robust account handling (create, delete, reset passwords)
- Two-Factor Authentication (TOTP)
- Session management
- Analytics (data_router)
- IP banning

**Database**: PostgreSQL (for account storage)

## The Retroflex SDK (`@trail-blaze/retroflex`)

The SDK provides access to all game functionality through typed JavaScript classes:

### World & Pawns
```ts
import { FWorld, FPawn } from "@trail-blaze/retroflex";

// Get world state
const state = FWorld.getProperty("state"); // "disengaged" | "engaged" | "inProgress"

// Disable fall damage  
FWorld.setProperty("damage", "false");

// Get all players and move them
FWorld.getPawnList().forEach((pawn) => {
  pawn.move(700, 700, 700);
  pawn.costume("CID_016_Athena_Commando_F");
});

// Get specific player
const myPawn = FWorld.getPawnByUsername("Array0x");
myPawn.setHealth(100);
myPawn.giveItem("WID_Assault_Auto_Athena_SR_Ore_T03", 1);
myPawn.kill();
```

### Game Control
```ts
import { FGame, FStorm } from "@trail-blaze/retroflex";

// Start/end match
FGame.startMatch();
FGame.endMatch({ winnerId: "player-123", reason: "last_standing" });

// Storm control
FStorm.nextPhase();
FStorm.pause();
FStorm.setPhaseConfig(2, { shrinkTime: 120, damagePerSecond: 5 });
```

### Building & Environment
```ts
import { FBuilding, FVehicle, FLoot } from "@trail-blaze/retroflex";

// Place buildings
FBuilding.place("wall", { x: 100, y: 200, z: 300 }, { material: "metal" });
FBuilding.destroyInRadius({ x: 0, y: 0, z: 0 }, 1000);

// Spawn vehicles
FVehicle.spawn("vehicle_car", { x: 100, y: 200, z: 300 });

// Spawn loot
FLoot.spawnItem("WID_Shotgun_Standard_Athena_UC_Ore_T03", { x: 0, y: 0, z: 100 });
FLoot.spawnSupplyDrop({ x: 0, y: 0, z: 5000 });
FLoot.spawnLlama({ x: 500, y: 500, z: 300 });
```

### Inventory & Teams
```ts
import { FInventory, FTeam } from "@trail-blaze/retroflex";

// Inventory management
FInventory.giveItem("player-123", "WID_Assault_Auto_Athena_SR_Ore_T03", 1);
FInventory.giveResources("player-123", 999, 999, 999);
FInventory.maxAmmo("player-123");

// Team management
FTeam.setPlayerTeam("player-123", 5);
FTeam.setFriendlyFire(false);
```

### Bots & Administration
```ts
import { FBots, FAdmin } from "@trail-blaze/retroflex";

// Spawn bots
FBots.spawn({ location: { x: 100, y: 200, z: 300 }, behavior: "aggressive" });
FBots.fillLobby(100); // Fill to 100 players with bots

// Administration
FAdmin.kick("player-123", "AFK");
FAdmin.ban("player-123", "Cheating");
FAdmin.broadcast("Server restarting in 5 minutes!");
```

### Events
```ts
import { FEvents } from "@trail-blaze/retroflex";

// Listen for player eliminations
FEvents.on("player.eliminated", (data) => {
  console.log(`${data.victimUsername} was eliminated by ${data.killerUsername}`);
});

// Listen for match events
FEvents.on("match.start", () => console.log("Match started!"));
FEvents.on("storm.phase_change", (data) => console.log(`Storm phase ${data.phase}`));
```

## The Module System

Modules are function-driven scripts that extend the backend. Each module implements three functions:

- **`ThreadStart`**: Executed when the module is invoked
- **`ThreadExit`**: Executed when the module exits
- **`ErrorHandler`**: Handles errors from C++ bindings (receives Flare objects)

### Example Module

```ts
import { FWorld } from "@trail-blaze/retroflex";
import { Flare, FlareUtils } from "@trail-blaze/flare";

// Executed when module starts
ThreadStart = (args) => {
  // Disable fall damage
  FWorld.setProperty("damage", "false");
  
  // Move all pawns to a location
  FWorld.getPawnList().forEach((pawn) => {
    pawn.move(700, 700, 700);
  });
  
  return 0; // Success
};

// Executed when module exits  
ThreadExit = (args) => {
  // Cleanup - kill all pawns
  FWorld.getPawnList().forEach((pawn) => {
    pawn.kill();
  });
  
  return 0;
};

// Handles C++ binding errors
ErrorHandler = (error: Flare) => {
  console.error(FlareUtils.parse(error).getReason());
  
  // Attempt recovery
  try {
    ThreadStart([]);
  } catch {
    console.error("Recovery failed, cleaning up...");
    ThreadExit([]);
  }
  
  return 1;
};
```

## Flare Error Handling

**Flare catches C/C++ binding exceptions** so they don't crash the JavaScript runtime.

When a native C++ function fails, the error is wrapped in a Flare object:

```json
{
  "spark_id": "IANCBAA4xnQDOyUEUIQA7gAUiCASny5R+AJ4D2AI0L4",
  "severity": "HIGH",
  "attention": {
    "trace": [
      { "file": "path/to/file.ts", "line": 23, "column": 51 }
    ]
  },
  "did_you_know": "Native function FPawn_Move failed: Invalid pawn ID"
}
```

Flares are **only used in the ErrorHandler** of modules:

```ts
ErrorHandler = (error: Flare) => {
  const utils = FlareUtils.parse(error);
  
  console.error(`[${utils.getSeverity()}] ${utils.getReason()}`);
  console.error(`Spark ID: ${utils.getSparkId()}`);
  
  if (utils.isCritical()) {
    // Critical error - shutdown module
    return 1;
  }
  
  // Attempt recovery
  return 0;
};
```

## Getting Started

### Prerequisites

- Node.js 18+
- PostgreSQL 14+ (for account management features)

### Installation

```bash
cd backend
npm install
```

### Configuration

```bash
cp .env.example .env
# Edit .env with your database credentials
```

### Build & Run

```bash
npm run build
npm start
```

## Tech Stack

- **Runtime**: Node.js with TypeScript
- **Database**: PostgreSQL  
- **Authentication**: JWT + bcrypt + TOTP
- **Web Framework**: Express.js

## License

BSD-3-Clause
