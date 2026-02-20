# Node.js Integration for Icarus

This directory contains the libnode integration that enables running JavaScript/TypeScript modules directly within the Project Reboot DLL process.

## Overview

The NodeJSWindow class embeds Node.js 22.22.0 runtime into the DLL and creates a separate console window where JavaScript code can run. This allows the Icarus JavaScript bindings to execute in a full Node.js environment with access to:

- All Node.js built-in modules (fs, path, http, etc.)
- npm packages installed in the project
- Native C++ bindings through the `@trail-blaze/icarus-addon` package

## Requirements

**Node.js 22.22.0 is required** to build and run this project. You must set up libnode before building.

See [LIBNODE_SETUP.md](../../../docs/LIBNODE_SETUP.md) or [NODE_22_22_SETUP.md](../../../docs/NODE_22_22_SETUP.md) for setup instructions.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│          Fortnite Process (FortniteClient-Win64-Shipping.exe)          │
│                                                           │
│  ┌─────────────────────────────────────────────────────┐ │
│  │    Project Reboot DLL (injected)                    │ │
│  │                                                       │ │
│  │  ┌─────────────────┐    ┌───────────────────────┐  │ │
│  │  │  Main Thread    │    │  Node.js Thread       │  │ │
│  │  │  (Game Hooks)   │◄───┤  (Separate Window)   │  │ │
│  │  │                 │    │                       │  │ │
│  │  │  - Hook game    │    │  - V8 JavaScript      │  │ │
│  │  │    functions    │    │  - Icarus bindings    │  │ │
│  │  │  - Handle       │    │  - Event loop         │  │ │
│  │  │    network      │    │  - Console I/O        │  │ │
│  │  └─────────────────┘    └───────────────────────┘  │ │
│  └─────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

## Files

| File | Description |
|------|-------------|
| `NodeJSWindow.h` | Header file with class definition and API |
| `NodeJSWindow.cpp` | Implementation of the Node.js runtime wrapper |
| `icarus/main.js` | Entry point JavaScript file loaded by Node.js |

## Usage

### Initialization

The Node.js runtime is automatically initialized when the DLL loads:

```cpp
// In dllmain.cpp Main() function
NodeJS::NodeConfig nodeConfig;
nodeConfig.title = "Icarus - Project Reboot JavaScript Runtime";
nodeConfig.entryPoint = "icarus/main.js";
nodeConfig.showConsole = true;

if (NodeJS::initializeNodeJS(nodeConfig)) {
    LOG_INFO(LogInit, "Icarus Node.js runtime started successfully");
}
```

### Executing Code

You can execute JavaScript code from C++:

```cpp
auto& nodeWindow = NodeJS::getNodeJSWindow();

// Execute inline code
nodeWindow.executeCode("console.log('Hello from C++!');");

// Load a module
nodeWindow.loadModule("./scripts/my-module.js");
```

### Shutdown

The runtime is automatically shut down when the DLL is unloaded:

```cpp
// In DLL_PROCESS_DETACH
NodeJS::shutdownNodeJS();
```

## Building with libnode

### Setup Required

The Node.js 22.22.0 runtime is fully integrated into the code. **You must install libnode files before building.**

1. **Download libnode**:
   - Get Node.js 22.22.0 from [nodejs.org/download/release/v22.22.0/](https://nodejs.org/download/release/v22.22.0/)
   - Download: `node-v22.22.0-win-x64.zip`

2. **Place files in vendor directory**:
   ```
   vendor/
     libnode/
       include/node/
         node.h
         v8.h
         uv.h
         (other headers...)
       lib/
         node.lib
   ```

3. **Build the project**:
   The vcxproj is already configured with the correct paths and link settings.
   ```bash
   # Open Project Reboot 3.0.sln in Visual Studio 2022
   # Build > Rebuild Solution
   ```

See [NODE_22_22_SETUP.md](./NODE_22_22_SETUP.md) for detailed setup instructions with screenshots.

## Entry Point (main.js)

The `icarus/main.js` file is the entry point for the Node.js runtime. It:

1. Sets up global error handlers
2. Initializes the Icarus SDK
3. Provides a REPL or loads user modules
4. Keeps the event loop alive

Example `main.js`:

```javascript
console.log('Icarus Node.js Runtime');

// Setup global error handling
process.on('uncaughtException', (error) => {
    console.error('Uncaught Exception:', error);
});

// Load native addon
const icarus = require('@trail-blaze/icarus-addon');

// Access game through bindings
const pawns = icarus.FWorld.getPawnList();
console.log(`${pawns.length} players in game`);

// Keep runtime alive
setInterval(() => {}, 60000);
```

## Security Considerations

- The Node.js runtime runs with the same privileges as the Fortnite process
- Any JavaScript code has full access to the file system and system APIs
- Only load trusted modules and packages
- Consider sandboxing or restricting the Node.js environment in production

## Troubleshooting

### Console Window Doesn't Appear

- Verify the DLL is being loaded into the game process
- Check the Project Reboot logs for initialization errors
- Ensure the working directory is correct

### Build Errors: "Cannot open include file: 'node.h'"

- Verify `vendor/libnode/include/node/` contains the Node.js headers
- Check that the include paths are correct in vcxproj
- Make sure you downloaded Node.js 22.22.0 specifically

### Linker Errors: "unresolved external symbol"

- Verify `vendor/libnode/lib/node.lib` exists
- Check that you're building for x64 (not x86)
- Ensure node.lib is from Node.js 22.22.0

### Runtime Error: Node.js Crashes on Startup

- Ensure all Node.js DLLs are in the correct location
- Verify Node.js version is exactly 22.22.0
- Check logs for specific initialization errors

## Future Enhancements

Potential improvements for the Node.js integration:

1. **Dynamic Module Loading**: Hot-reload modules without restarting
2. **Bidirectional Communication**: Better C++ ↔ JS messaging
3. **Debugger Support**: Enable Chrome DevTools or VS Code debugging
4. **Performance Monitoring**: Track JS execution impact on game performance
5. **Sandboxing**: Restrict file system and network access for safety

## References

- [Node.js Embedding Guide](https://nodejs.org/api/embedding.html)
- [V8 Embedder's Guide](https://v8.dev/docs/embed)
- [libuv Documentation](http://docs.libuv.org/)
- [N-API Documentation](https://nodejs.org/api/n-api.html)
