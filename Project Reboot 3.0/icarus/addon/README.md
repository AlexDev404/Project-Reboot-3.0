# @trail-blaze/icarus-addon

Native Node.js add-on for Icarus - C++ bindings for Project Reboot.

This package provides N-API bindings that connect JavaScript/TypeScript to Project Reboot's C++ functions, enabling developers to write Fortnite game modifications using modern JavaScript.

## Installation

```bash
npm install @trail-blaze/icarus-addon
```

### Prerequisites

- Node.js 22.22 (specifically version 22.22.0)
- node-gyp and build tools:
  - **Windows**: `npm install -g windows-build-tools` or Visual Studio Build Tools
  - **Linux**: `sudo apt-get install build-essential`
  - **macOS**: Xcode Command Line Tools

## Usage

```typescript
import { FWorld, FPawn, FGame, FStorm, FAdmin } from '@trail-blaze/icarus-addon';

// Get all players
const pawns = FWorld.getPawnList();
console.log(`${pawns.length} players in game`);

// Move all players to a location
pawns.forEach(pawn => {
    FPawn.move(pawn.id, 700, 700, 700);
});

// Get a specific player
const myPawn = FWorld.getPawnByUsername("Array0x");
if (myPawn) {
    FPawn.setHealth(myPawn.id, 100);
    FPawn.setShield(myPawn.id, 100);
}

// Start the match
FGame.startMatch();

// Pause the storm
FStorm.pause();

// Broadcast a message
FAdmin.broadcast("Welcome to the server!");
```

## API Reference

### FWorld

| Method | Description |
|--------|-------------|
| `getProperty(name?)` | Get world property or all properties |
| `setProperty(name, value)` | Set a world property |
| `getPawnList()` | Get all pawns in the world |
| `getPawnById(id)` | Get pawn by ID |
| `getPawnByUsername(username)` | Get pawn by username |

### FPawn

| Method | Description |
|--------|-------------|
| `move(id, x, y, z)` | Move/teleport pawn to location |
| `setHealth(id, health)` | Set pawn health |
| `setShield(id, shield)` | Set pawn shield |
| `kill(id)` | Kill the pawn |
| `revive(id)` | Revive the pawn |
| `giveItem(id, itemId, count)` | Give item to pawn |
| `sendMessage(id, message)` | Send message to player |

### FGame

| Method | Description |
|--------|-------------|
| `startMatch()` | Start the match (aircraft phase) |
| `endMatch(options?)` | End the match |
| `getMatchState()` | Get current match state |
| `setPlayersLeft(count)` | Set players left count |

### FStorm

| Method | Description |
|--------|-------------|
| `getCurrentPhase()` | Get current storm phase |
| `pause()` | Pause the storm |
| `resume()` | Resume the storm |
| `nextPhase()` | Skip to next phase |
| `skipToPhase(phase)` | Skip to specific phase |

### FInventory

| Method | Description |
|--------|-------------|
| `giveItem(playerId, itemId, count)` | Give item to player |
| `removeItem(playerId, itemGuid)` | Remove item |
| `clearInventory(playerId)` | Clear player inventory |
| `giveResources(playerId, wood, stone, metal)` | Give resources |

### FAdmin

| Method | Description |
|--------|-------------|
| `kick(playerId, reason?)` | Kick a player |
| `ban(playerId, reason?, duration?)` | Ban a player |
| `broadcast(message)` | Broadcast message to all players |
| `getPlayers()` | Get all connected players |

### FBots

| Method | Description |
|--------|-------------|
| `spawn(options)` | Spawn a bot |
| `remove(botId)` | Remove a bot |
| `removeAll()` | Remove all bots |
| `fillLobby(targetCount)` | Fill lobby with bots |

### Flare (Error Handling)

```typescript
import { Flare } from '@trail-blaze/icarus-addon';

try {
    // Some operation that might fail
} catch (error) {
    const flare = Flare.parse(error);
    console.error(`[${flare.getSeverity()}] ${flare.getReason()}`);
    console.error(`Spark ID: ${flare.getSparkId()}`);
    
    if (flare.isCritical()) {
        // Handle critical error
    }
}
```

## Building

```bash
npm install
npm run build
```

For debug builds:
```bash
npm run build:debug
```

## Integration with Project Reboot

When building with Project Reboot, define `PROJECT_REBOOT_BUILD` to enable actual C++ function calls:

```gyp
"defines": [ "PROJECT_REBOOT_BUILD" ]
```

Without this define, the add-on runs in mock mode for development/testing.

## License

BSD-3-Clause
