# User Guide

This guide explains how to build, configure, and run Project Reboot 3.0 to host your own Fortnite private server.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building the Project](#building-the-project)
3. [Configuration](#configuration)
4. [Running the Server](#running-the-server)
5. [Connecting Clients](#connecting-clients)
6. [In-Game Commands](#in-game-commands)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Hardware Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU | 4 cores | 8+ cores |
| RAM | 8 GB | 16 GB |
| Network | 10 Mbps | 100 Mbps |
| Storage | 10 GB | 50 GB |

### Software Requirements

#### For Building

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with:
  - Desktop development with C++
  - Windows 10/11 SDK
  - MSVC v143 build tools
- **Git** for cloning the repository

#### For Running

- **Fortnite Client** (supported version S3-S15)
- **Reboot Launcher** (for injecting the DLL)
- **Node.js 18+** (for backend services)
- **PostgreSQL 14+** (for account management)

---

## Building the Project

### Step 1: Clone the Repository

```bash
git clone https://github.com/AlexDev404/Project-Reboot-3.0.git
cd Project-Reboot-3.0
```

### Step 2: Build the C++ DLL

#### Using Visual Studio (Recommended)

1. Open `Project Reboot 3.0.sln` in Visual Studio 2022
2. Select **Release** configuration and **x64** platform
3. Build the solution (Ctrl+Shift+B)
4. The DLL will be output to `Build/` or the configured output directory

#### Using Command Line

```bash
# Open Developer Command Prompt for VS 2022
msbuild "Project Reboot 3.0/Project Reboot 3.0.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:OutDir=../Build
```

#### Using Linux (via Wine)

If you're on Linux, you can build using MSVC through Wine:

```bash
# Requires msvc-wine setup (https://github.com/mstorsjo/msvc-wine)
./build-linux.sh
```

### Step 3: Build the Backend

```bash
cd backend
npm install
npm run build
```

---

## Configuration

### Backend Configuration

1. Copy the example environment file:

```bash
cd backend
cp .env.example .env
```

2. Edit `.env` with your settings:

```env
# Database Configuration
DB_HOST=localhost
DB_PORT=5432
DB_NAME=reboot
DB_USER=reboot_user
DB_PASSWORD=your_secure_password

# Server Configuration
PORT=3000
JWT_SECRET=your_jwt_secret_key

# Icarus Configuration
ICARUS_MODULES_PATH=./modules
ICARUS_DEBUG=false
```

### Database Setup

1. Install PostgreSQL 14 or later
2. Create a database and user:

```sql
CREATE USER reboot_user WITH PASSWORD 'your_secure_password';
CREATE DATABASE reboot OWNER reboot_user;
```

3. Run migrations (if available):

```bash
cd backend
npm run migrate
```

---

## Running the Server

### Step 1: Start the Backend

```bash
cd backend
npm start
```

You should see:
```
[INFO] Backend server started on port 3000
[INFO] Database connected successfully
[INFO] Icarus module runtime initialized
```

### Step 2: Launch Fortnite with Reboot Launcher

1. Download the Reboot Launcher from the community Discord
2. Configure the launcher:
   - Set the path to your Fortnite installation
   - Set the DLL path to the built `Project Reboot 3.0.dll`
3. Click **Launch**

### Step 3: Verify Connection

Once Fortnite launches:
1. The game should show the lobby
2. The server console should show connection messages
3. You can now create/join matches

---

## Connecting Clients

### Single Player Testing

The server host can test alone by simply launching the game.

### Multiplayer Setup

For other players to connect:

1. **Port Forwarding**: Forward these ports on your router:
   - UDP 7777 (Game Server)
   - TCP 3000 (Backend API)

2. **Share Connection Info**: Provide players with:
   - Your public IP address
   - The configured ports

3. **Client Configuration**: Players need to:
   - Use a compatible Fortnite build
   - Configure their launcher to connect to your server IP

### LAN Play

For local network play, use your local IP address (e.g., `192.168.1.x`).

---

## In-Game Commands

Project Reboot provides console commands for server administration.

### Opening the Console

Press **`** (backtick) or **F1** to open the in-game console.

### Player Commands

| Command | Description |
|---------|-------------|
| `event start` | Starts the match/aircraft phase |
| `event stop` | Stops the current event |
| `storm pause` | Pauses the storm |
| `storm resume` | Resumes the storm |
| `storm skip` | Skips to the next zone |

### Admin Commands

| Command | Description |
|---------|-------------|
| `kick <player>` | Kicks a player from the server |
| `ban <player>` | Bans a player |
| `give <item> [count]` | Gives an item to yourself |
| `teleport <x> <y> <z>` | Teleports to coordinates |
| `setplayers <count>` | Sets the player count display |

### Bot Commands

| Command | Description |
|---------|-------------|
| `spawnbot` | Spawns a bot at your location |
| `spawnbots <count>` | Spawns multiple bots |
| `killbots` | Removes all bots |

---

## Troubleshooting

### Common Issues

#### "DLL injection failed"

**Cause**: Antivirus blocking the DLL or wrong architecture.

**Solution**:
1. Add the DLL to your antivirus exclusions
2. Ensure you built for x64 platform
3. Run the launcher as administrator

#### "Connection timed out"

**Cause**: Network configuration issues.

**Solution**:
1. Check firewall settings
2. Verify port forwarding
3. Ensure backend is running

#### "Version mismatch"

**Cause**: Incompatible Fortnite version.

**Solution**:
1. Use a supported version (S3-S15 recommended)
2. Check the repository for version-specific branches

#### "Backend failed to start"

**Cause**: Database connection or configuration issues.

**Solution**:
1. Verify PostgreSQL is running
2. Check `.env` configuration
3. Run `npm run test` to diagnose issues

#### "Crash on game start"

**Cause**: Pattern scanning failures for your Fortnite version.

**Solution**:
1. Check the console for error messages
2. Verify your Fortnite version is supported
3. Report the issue with your `reboot.log` file

### Getting Help

If you're still having issues:

1. Check the `reboot.log` file for detailed error messages
2. Search existing GitHub issues
3. Ask in the [Discord server](https://discord.gg/rebootmp)
4. Open a GitHub issue with:
   - Your Fortnite version
   - Your OS and specs
   - The `reboot.log` file
   - Steps to reproduce

---

## How It Works

### Overview

Project Reboot works by injecting a DLL into the Fortnite client that intercepts and modifies game behavior. Here's what happens:

1. **DLL Injection**: The launcher injects `Project Reboot 3.0.dll` into `FortniteClient-Win64-Shipping.exe`

2. **Initialization**: The DLL:
   - Scans for function addresses using pattern matching
   - Installs hooks to redirect game functions
   - Initializes the Icarus JavaScript runtime

3. **Net Mode Override**: The game is tricked into thinking it's a dedicated server (`NM_DedicatedServer`)

4. **Match Control**: Game phases (lobby, aircraft, zones) are controlled through hooked functions

5. **Replication**: Player positions, actions, and game state are replicated to all connected clients

### The Icarus Module System

Icarus allows extending the server with JavaScript/TypeScript:

```typescript
// Example module that heals all players
import { FWorld } from "@trail-blaze/retroflex";

ThreadStart = (args) => {
  FWorld.getPawnList().forEach((pawn) => {
    pawn.setHealth(100);
  });
  return 0;
};
```

See the [Icarus SDK Documentation](./ICARUS_SDK.md) for more details.

---

## Version Compatibility

| Season | Status | Notes |
|--------|--------|-------|
| S1-S2 | ⚠️ Partial | May have issues |
| S3-S15 | ✅ Full | Recommended |
| S16-S19 | ⚠️ Partial | Some features may not work |
| S20+ | 🧪 Experimental | Use alternate branch |

---

## Security Considerations

### Server Security

- Change default ports if possible
- Use strong JWT secrets
- Keep your server IP private if not hosting publicly
- Regularly update the project

### Account Security

- Enable 2FA for admin accounts
- Use strong passwords
- Don't share admin credentials

---

## Next Steps

- Learn about [module development](./ICARUS_SDK.md) to extend the server
- Join the [Discord community](https://discord.gg/rebootmp) for support
- Contribute improvements back to the project
