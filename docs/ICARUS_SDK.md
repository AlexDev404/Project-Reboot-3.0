# Icarus SDK Documentation

Icarus is the JavaScript/TypeScript binding layer that exposes Project Reboot's C++ game functions to JavaScript, enabling developers to write Fortnite modifications using modern JavaScript through "modules".

## Table of Contents

1. [Getting Started](#getting-started)
2. [Module Structure](#module-structure)
3. [SDK Reference](#sdk-reference)
4. [Error Handling with Flare](#error-handling-with-flare)
5. [Examples](#examples)

---

## Getting Started

### What is Icarus?

Icarus binds Project Reboot's C++ functions to JavaScript, allowing you to:

- Write game modifications in TypeScript/JavaScript instead of C++
- Use modern development tools and practices
- Create reusable modules that extend server functionality

### Packages

| Package | Description |
|---------|-------------|
| `@trail-blaze/retroflex` | Core SDK - TypeScript bindings for C++ functions |
| `@trail-blaze/flare` | Error handling for catching C++ exceptions |
| `@trail-blaze/icarus-core` | Module system and runtime |

### Installing the SDK

```bash
npm install @trail-blaze/retroflex @trail-blaze/flare
```

---

## Module Structure

Every Icarus module must implement three functions:

### ThreadStart

Called when the module is started.

```typescript
ThreadStart = (args: any[]) => {
  // Module initialization logic
  return 0; // 0 = success, non-zero = error
};
```

### ThreadExit

Called when the module exits (cleanup).

```typescript
ThreadExit = (args: any[]) => {
  // Cleanup logic
  return 0;
};
```

### ErrorHandler

Called when a native C++ binding throws an error.

```typescript
ErrorHandler = (error: Flare) => {
  console.error(error.did_you_know);
  return 1; // Return code
};
```

### Complete Module Template

```typescript
import { FWorld } from "@trail-blaze/retroflex";
import { Flare, FlareUtils } from "@trail-blaze/flare";

// Called when the module starts
ThreadStart = (args: any[]) => {
  console.log("Module started!");
  
  // Your initialization logic here
  FWorld.getPawnList().forEach((pawn) => {
    pawn.setHealth(100);
  });
  
  return 0;
};

// Called when the module exits
ThreadExit = (args: any[]) => {
  console.log("Module cleaning up...");
  return 0;
};

// Handles errors from native bindings
ErrorHandler = (error: Flare) => {
  const utils = FlareUtils.parse(error);
  console.error(`[${utils.getSeverity()}] ${utils.getReason()}`);
  
  if (utils.isCritical()) {
    return 1; // Exit on critical errors
  }
  
  return 0; // Continue on non-critical errors
};
```

---

## SDK Reference

### FWorld - World State & Pawn Management

#### getProperty

Get world properties.

```typescript
// Get all properties
const props = FWorld.getProperty();
// Returns: { state, playersLeft, teamsLeft, safeZonePaused }

// Get specific property
const state = FWorld.getProperty("state");
// Returns: "disengaged" | "engaged" | "inProgress"
```

#### setProperty

Set world properties.

```typescript
FWorld.setProperty("safeZonePaused", true);
FWorld.setProperty("damage", "false"); // Disable fall damage
```

#### getPawnList

Get all player pawns in the world.

```typescript
const pawns = FWorld.getPawnList();
pawns.forEach((pawn) => {
  console.log(pawn.username, pawn.health);
});
```

#### getPawnByUsername

Find a specific player by username.

```typescript
const pawn = FWorld.getPawnByUsername("PlayerName");
if (pawn) {
  pawn.setHealth(100);
}
```

#### getPawnById

Find a pawn by its internal ID.

```typescript
const pawn = FWorld.getPawnById("123456789");
```

---

### FPawn - Player/Bot Character Control

Every pawn object from `FWorld.getPawnList()` has these methods:

#### Properties

```typescript
pawn.id          // string - Internal pawn ID
pawn.username    // string - Player display name
pawn.location    // { x, y, z } - Current position
pawn.health      // number - Current health
pawn.maxHealth   // number - Maximum health
pawn.shield      // number - Current shield
pawn.isAlive     // boolean - Is the pawn alive
pawn.teamId      // number - Team index
```

#### move / teleport

Move the pawn to a new location.

```typescript
pawn.move(100, 200, 300);      // x, y, z coordinates
pawn.teleport(100, 200, 300);  // Same as move
```

#### setHealth / setShield

Set health or shield values.

```typescript
pawn.setHealth(100);
pawn.setShield(50);
```

#### kill

Kill the pawn.

```typescript
pawn.kill();
```

#### costume

Set the player's skin/costume.

```typescript
pawn.costume("CID_016_Athena_Commando_F");
```

#### giveItem

Give an item to the player.

```typescript
pawn.giveItem("WID_Assault_Auto_Athena_SR_Ore_T03", 1);
```

---

### FGame - Match Control

#### startMatch

Start the match (begins aircraft phase).

```typescript
FGame.startMatch();
```

#### endMatch

End the current match.

```typescript
FGame.endMatch();
FGame.endMatch({ winnerId: "player-123", reason: "last_standing" });
```

#### getMatchState

Get the current match state.

```typescript
const state = FGame.getMatchState();
// Returns: "none" | "setup" | "warmup" | "aircraft" | "safezones" | "endgame"
```

#### setPlayersLeft

Set the displayed player count.

```typescript
FGame.setPlayersLeft(50);
```

---

### FStorm - Storm/Safe Zone Management

#### getCurrentPhase

Get the current storm phase number.

```typescript
const phase = FStorm.getCurrentPhase();
```

#### pause / resume

Pause or resume the storm.

```typescript
FStorm.pause();
FStorm.resume();
```

#### nextPhase

Skip to the next storm phase.

```typescript
FStorm.nextPhase();
```

#### skipToPhase

Skip to a specific phase.

```typescript
FStorm.skipToPhase(3);
```

#### getPhaseConfig / setPhaseConfig

Get or set storm phase configuration.

```typescript
const config = FStorm.getPhaseConfig(2);
// Returns: { shrinkTime, holdTime, damagePerSecond, radius }

FStorm.setPhaseConfig(2, {
  shrinkTime: 120,
  damagePerSecond: 5
});
```

---

### FInventory - Inventory Management

#### giveItem

Give an item to a player.

```typescript
FInventory.giveItem("player-id", "WID_Shotgun_Standard", 1);
FInventory.giveItem("player-id", "WID_Shotgun_Standard", 1, true); // Show toast
```

#### removeItem

Remove an item from inventory.

```typescript
FInventory.removeItem("player-id", "item-guid", 1);
```

#### clearInventory

Clear a player's entire inventory.

```typescript
FInventory.clearInventory("player-id");
```

#### giveResources

Give building materials.

```typescript
FInventory.giveResources("player-id", 999, 999, 999); // wood, stone, metal
```

---

### FBots - AI Bot Control

#### spawn

Spawn a bot.

```typescript
FBots.spawn({
  location: { x: 100, y: 200, z: 300 },
  behavior: "aggressive"
});
```

#### fillLobby

Fill the lobby with bots to a target count.

```typescript
const spawned = FBots.fillLobby(100);
console.log(`Spawned ${spawned} bots`);
```

#### removeAll

Remove all bots.

```typescript
FBots.removeAll();
```

#### getAll

Get all current bots.

```typescript
const bots = FBots.getAll();
bots.forEach((bot) => {
  console.log(bot.id, bot.location);
});
```

---

### FAdmin - Server Administration

#### kick

Kick a player from the server.

```typescript
FAdmin.kick("player-id", "AFK");
```

#### ban

Ban a player.

```typescript
FAdmin.ban("player-id", "Cheating", "24h");
```

#### broadcast

Send a message to all players.

```typescript
FAdmin.broadcast("Server restarting in 5 minutes!");
```

#### getPlayers

Get all connected players.

```typescript
const players = FAdmin.getPlayers();
players.forEach((p) => {
  console.log(p.username, p.isAlive);
});
```

#### isOperator

Check if a player is an operator (admin).

```typescript
if (FAdmin.isOperator("player-id")) {
  // Allow admin actions
}
```

---

### FEvents - Event System

#### on

Subscribe to game events.

```typescript
FEvents.on("player.eliminated", (data) => {
  console.log(`${data.victimUsername} was eliminated by ${data.killerUsername}`);
});

FEvents.on("match.start", () => {
  console.log("Match started!");
});

FEvents.on("storm.phase_change", (data) => {
  console.log(`Storm phase ${data.phase}`);
});
```

#### Available Events

| Event | Data |
|-------|------|
| `player.join` | `{ playerId, username }` |
| `player.leave` | `{ playerId, username }` |
| `player.eliminated` | `{ victimId, killerId, weapon }` |
| `player.downed` | `{ victimId, attackerId }` |
| `player.revived` | `{ playerId, reviverId }` |
| `match.start` | `{}` |
| `match.end` | `{ winnerId, winnerTeam }` |
| `storm.phase_change` | `{ phase, radius }` |

---

## Error Handling with Flare

When a native C++ function fails, the error is wrapped in a **Flare** object.

### Flare Structure

```typescript
interface Flare {
  spark_id: string;      // Unique error identifier
  severity: "LOW" | "MEDIUM" | "HIGH" | "CRITICAL";
  attention: {
    trace: Array<{
      file?: string;
      line?: number;
      column?: number;
      function?: string;
      native?: boolean;
    }>;
  };
  did_you_know: string;  // Human-readable error message
}
```

### Using FlareUtils

```typescript
import { Flare, FlareUtils } from "@trail-blaze/flare";

ErrorHandler = (error: Flare) => {
  const utils = FlareUtils.parse(error);
  
  console.error(`Spark ID: ${utils.getSparkId()}`);
  console.error(`Severity: ${utils.getSeverity()}`);
  console.error(`Reason: ${utils.getReason()}`);
  
  if (utils.isCritical()) {
    console.error("Critical error - shutting down");
    return 1;
  }
  
  // Attempt recovery
  try {
    ThreadStart([]);
  } catch {
    return 1;
  }
  
  return 0;
};
```

---

## Examples

### Example 1: Heal All Players

```typescript
import { FWorld } from "@trail-blaze/retroflex";

ThreadStart = () => {
  FWorld.getPawnList().forEach((pawn) => {
    pawn.setHealth(100);
    pawn.setShield(100);
  });
  return 0;
};
```

### Example 2: One-Hit Kill Mode

```typescript
import { FWorld, FEvents } from "@trail-blaze/retroflex";

ThreadStart = () => {
  // Set all players to 1 HP
  FWorld.getPawnList().forEach((pawn) => {
    pawn.setHealth(1);
    pawn.setShield(0);
  });
  return 0;
};
```

### Example 3: Custom Zone Configuration

```typescript
import { FStorm } from "@trail-blaze/retroflex";

ThreadStart = () => {
  // Fast-paced zones
  for (let i = 1; i <= 9; i++) {
    FStorm.setPhaseConfig(i, {
      shrinkTime: 30,
      holdTime: 15,
      damagePerSecond: i * 2
    });
  }
  return 0;
};
```

### Example 4: Welcome Message

```typescript
import { FEvents, FAdmin, FWorld } from "@trail-blaze/retroflex";

ThreadStart = () => {
  FEvents.on("player.join", (data) => {
    FAdmin.broadcast(`Welcome ${data.username} to the server!`);
    
    // Give the new player a gift
    const pawn = FWorld.getPawnByUsername(data.username);
    if (pawn) {
      pawn.giveItem("WID_Assault_Auto_Athena_SR_Ore_T03", 1);
    }
  });
  return 0;
};
```

### Example 5: Elimination Tracker

```typescript
import { FEvents, FAdmin } from "@trail-blaze/retroflex";

const killCounts: Record<string, number> = {};

ThreadStart = () => {
  FEvents.on("player.eliminated", (data) => {
    if (data.killerUsername) {
      killCounts[data.killerUsername] = (killCounts[data.killerUsername] || 0) + 1;
      const kills = killCounts[data.killerUsername];
      
      if (kills % 5 === 0) {
        FAdmin.broadcast(`${data.killerUsername} is on a ${kills} kill streak!`);
      }
    }
  });
  return 0;
};

ThreadExit = () => {
  // Print final stats
  Object.entries(killCounts).forEach(([player, kills]) => {
    console.log(`${player}: ${kills} kills`);
  });
  return 0;
};
```

---

## Best Practices

1. **Always handle errors**: Implement `ErrorHandler` to prevent crashes
2. **Clean up resources**: Use `ThreadExit` for cleanup
3. **Check for null**: Pawns and other objects may be null
4. **Use TypeScript**: Get type checking and IDE support
5. **Test thoroughly**: Test modules with multiple players and scenarios

---

## Debugging

### Enable Debug Logging

Set `ICARUS_DEBUG=true` in your environment.

### Console Output

Use `console.log()`, `console.warn()`, and `console.error()` for debugging.

### Flare Spark IDs

Every error has a unique `spark_id` for tracking issues.

---

## Further Reading

- [Architecture Guide](./ARCHITECTURE.md) - Technical deep-dive
- [User Guide](./USER_GUIDE.md) - Running the server
- [Backend README](../backend/README.md) - More details on the Icarus backend
