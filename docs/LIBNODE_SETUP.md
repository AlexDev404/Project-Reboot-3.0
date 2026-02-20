# Setting Up libnode for Icarus

This guide explains how to add full Node.js support to Project Reboot using libnode.

## Required Version

**Node.js 22.22 is required** for Icarus. This specific version ensures compatibility with the Project Reboot bindings and the embedded runtime.

## Current Status

By default, the Node.js integration runs in **mock mode**:
- ✅ A console window appears showing status messages
- ✅ The infrastructure is in place for running JavaScript
- ❌ No actual Node.js runtime is embedded yet
- ❌ JavaScript code cannot be executed

To enable full Node.js functionality, follow this guide to add libnode.

## Prerequisites

- Visual Studio 2022 with C++ build tools
- **Node.js 22.22 specifically** (not older or newer versions)
- Basic understanding of C++ project configuration
- ~150MB of disk space for libnode files

## Option 1: Using Pre-built libnode (Recommended)

### Step 1: Download Node.js 22.22

1. Go to [nodejs.org/download/release/v22.22.0/](https://nodejs.org/download/release/v22.22.0/)
2. Download **node-v22.22.0-win-x64.zip**
3. **Important**: Do not use v23.x, v21.x, or any other version - only v22.22.0 is supported

### Step 2: Extract Required Files

Extract the following from the Node.js archive:

```
node-v22.22.0-win-x64/
  ├── node.exe           → Copy to Project Reboot 3.0/
  ├── node.lib           → vendor/libnode/lib/
  └── include/
      └── node/
          ├── node.h     → vendor/libnode/include/node/
          ├── node_api.h
          ├── v8.h
          ├── uv.h
          └── (all .h files)
```

Create this directory structure in your Project Reboot folder:

```
Project-Reboot-3.0/
  ├── vendor/
  │   └── libnode/
  │       ├── include/
  │       │   └── node/
  │       │       └── *.h (all header files)
  │       └── lib/
  │           └── node.lib
  └── Project Reboot 3.0/
      └── node.exe
```

### Step 3: Update Visual Studio Project

1. Open `Project Reboot 3.0.sln` in Visual Studio 2022

2. Right-click on "Project Reboot 3.0" project → Properties

3. Go to **Configuration Properties → C/C++ → Preprocessor**
   - Add `ENABLE_LIBNODE` to "Preprocessor Definitions"

4. Go to **C/C++ → General**
   - Add `$(ProjectDir)..\vendor\libnode\include\node` to "Additional Include Directories"

5. Go to **Linker → General**
   - Add `$(ProjectDir)..\vendor\libnode\lib` to "Additional Library Directories"

6. Go to **Linker → Input**
   - Add `node.lib` to "Additional Dependencies"

7. Click OK to save changes

### Step 4: Rebuild Project

```
Build → Rebuild Solution
```

If successful, you should see:
```
Build succeeded.
1 succeeded, 0 failed, 0 up-to-date, 0 skipped
```

### Step 5: Test

1. Run the Project Reboot DLL with a Fortnite client
2. You should see a new console window titled "Icarus - Project Reboot JavaScript Runtime"
3. The console should display:
   ```
   ========================================
    Icarus Node.js Runtime
    Project Reboot JavaScript Bindings
   ========================================
   
   Node.js version: v22.22.0
   Working directory: C:\...\Project-Reboot-3.0
   
   Icarus runtime is ready!
   ```

**Important**: Verify the version shows `v22.22.0`. If it shows a different version, you have the wrong Node.js installation.

## Option 2: Building libnode from Source

For advanced users who want to customize Node.js or build with specific configurations.

### Requirements

- Visual Studio 2022
- Python 3.x
- Git

### Build Steps

1. Clone Node.js repository:
   ```bash
   git clone https://github.com/nodejs/node.git
   cd node
   git checkout v22.22.0  # Specific version required
   ```

2. Configure build as a library:
   ```bash
   python configure --shared
   ```

3. Build with Visual Studio:
   ```bash
   vcbuild.bat release x64
   ```

4. Copy output files:
   ```bash
   # From node/out/Release/
   copy node.lib → Project-Reboot-3.0/vendor/libnode/lib/
   copy libnode.dll → Project-Reboot-3.0/Project Reboot 3.0/
   
   # From node/src/
   copy all .h files → Project-Reboot-3.0/vendor/libnode/include/node/
   
   # From node/deps/
   copy v8/include/*.h → Project-Reboot-3.0/vendor/libnode/include/node/
   copy uv/include/*.h → Project-Reboot-3.0/vendor/libnode/include/node/
   ```

5. Follow Step 3-5 from Option 1

## Verifying Installation

To verify libnode is properly integrated:

1. Check that `ENABLE_LIBNODE` is defined:
   ```cpp
   // In NodeJSWindow.cpp, you should see this code being compiled:
   #ifdef ENABLE_LIBNODE
       // Real implementation
   #else
       // Mock mode
   #endif
   ```

2. Console window should show real Node.js version, not mock messages

3. You can execute JavaScript:
   ```cpp
   NodeJS::getNodeJSWindow().executeCode("console.log('Hello from Node.js!')");
   ```

## Troubleshooting

### Build Error: "Cannot open include file: 'node.h'"

**Solution**: Verify the include path is correct in project properties.

The path should point to the directory *containing* the node folder:
- ✅ `vendor/libnode/include/node`
- ❌ `vendor/libnode/include/node/node.h`

### Linker Error: "unresolved external symbol"

**Solution**: 
1. Verify `node.lib` is in `vendor/libnode/lib/`
2. Check that the library directory is added to Linker settings
3. Ensure you're building for x64 (not x86)

### Runtime Error: "node.exe not found" or DLL errors

**Solution**: 
1. Copy `node.exe` to the same directory as the compiled DLL
2. Or add the Node.js bin directory to your PATH

### Console Window Shows "Mock Mode" Message

**Solution**: `ENABLE_LIBNODE` is not defined. Go back to Step 3 of Option 1.

### Node.js Runtime Crashes Immediately

**Possible causes**:
1. **Version mismatch**: Ensure you're using Node.js 22.22.0 specifically. Other versions are not supported.
2. **Header/lib mismatch**: Make sure headers and lib file are from the same Node.js 22.22.0 build
3. **Architecture mismatch**: Ensure both the DLL and node.lib are 64-bit
4. **Missing DLLs**: Node.js may require additional DLLs (check with Dependency Walker)

### Wrong Node.js Version Detected

**Solution**: 
1. Delete the existing `vendor/libnode/` directory
2. Download Node.js 22.22.0 specifically from [nodejs.org/download/release/v22.22.0/](https://nodejs.org/download/release/v22.22.0/)
3. Follow the setup steps again with the correct version

## Alternative: Using Node.js as External Process

If embedding libnode proves difficult, you can run Node.js as a separate process:

1. Keep mock mode (no `ENABLE_LIBNODE`)
2. Modify `NodeJSWindow.cpp` to launch `node.exe` as a child process
3. Use IPC (named pipes or sockets) for communication

This is simpler but has higher overhead and latency.

## Next Steps

Once libnode is integrated:

1. **Write JavaScript modules** in `Project Reboot 3.0/icarus/modules/`
2. **Load modules** from `main.js`
3. **Use the Icarus SDK** to interact with the game
4. **Debug with DevTools** by adding `--inspect` flag

See [ICARUS_SDK.md](ICARUS_SDK.md) for JavaScript API documentation.

## Support

If you encounter issues:
1. Check the [Project Reboot Discord](https://discord.gg/rebootmp)
2. Open an issue on GitHub with:
   - Visual Studio version
   - Node.js version (must be 22.22.0)
   - Build output/error messages
   - Steps to reproduce

## References

- [Node.js 22.22.0 Download](https://nodejs.org/download/release/v22.22.0/)
- [Node.js Embedding Guide](https://nodejs.org/api/embedding.html)
- [Node.js GitHub Repository](https://github.com/nodejs/node)
- [V8 Embedder's Guide](https://v8.dev/docs/embed)
