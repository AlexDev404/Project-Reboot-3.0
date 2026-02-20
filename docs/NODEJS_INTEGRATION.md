# Node.js Integration for Icarus

This directory contains the libnode integration that enables running JavaScript/TypeScript modules directly within the Project Reboot DLL process.

## Overview

The NodeJSWindow class embeds a Node.js runtime into the DLL and creates a separate console window where JavaScript code can run. This allows the Icarus JavaScript bindings to execute in a full Node.js environment with access to:

- All Node.js built-in modules (fs, path, http, etc.)
- npm packages installed in the project
- Native C++ bindings through the `@trail-blaze/icarus-addon` package

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
│  │  │  - Hook game    │    │  - Run JS modules     │  │ │
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

### Current Status: Mock Mode

The current implementation runs in **mock mode**, which means:
- A console window is created and shows status messages
- No actual Node.js runtime is embedded (yet)
- All API calls are simulated and logged

### Enabling Full libnode Support

To enable actual Node.js embedding:

1. **Download libnode**:
   - Get the libnode headers and library from [nodejs.org](https://nodejs.org/)
   - Or build from source: [Node.js GitHub](https://github.com/nodejs/node)

2. **Place files in vendor directory**:
   ```
   vendor/
     libnode/
       include/
         node.h
         uv.h
         v8.h
         (other headers...)
       lib/
         libnode.lib  (or node.lib on Windows)
   ```

3. **Update the project**:
   - Add `ENABLE_LIBNODE` preprocessor definition to vcxproj
   - Add include path: `vendor/libnode/include`
   - Add library path: `vendor/libnode/lib`
   - Link against `libnode.lib`

4. **Rebuild**:
   ```bash
   # Open Project Reboot 3.0.sln in Visual Studio
   # Build > Rebuild Solution
   ```

### vcxproj Changes Needed

Add to the Release configuration:

```xml
<ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
  <ClCompile>
    <PreprocessorDefinitions>ENABLE_LIBNODE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
    <AdditionalIncludeDirectories>../vendor/libnode/include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
  </ClCompile>
  <Link>
    <AdditionalLibraryDirectories>../vendor/libnode/lib;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
    <AdditionalDependencies>libnode.lib;%(AdditionalDependencies)</AdditionalDependencies>
  </Link>
</ItemDefinitionGroup>
```

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

- Check that `showConsole` is set to `true` in the config
- Verify the DLL is actually being loaded
- Check the Project Reboot logs for initialization errors

### "libnode support not compiled in" Message

- This is expected in mock mode
- Follow the "Enabling Full libnode Support" steps above to add actual Node.js

### JavaScript Code Not Executing

- Ensure the entry point file exists at the specified path
- Check the console window for JavaScript errors
- Verify the event loop is running (should see heartbeat in logs)

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
