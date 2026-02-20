# Quick Start: Node.js 22.22 Setup

**Project Reboot requires Node.js 22.22.0 specifically for the Icarus JavaScript bindings.**

## Download Links

### Windows x64 (Required for Project Reboot)
- **Direct Download**: [node-v22.22.0-win-x64.zip](https://nodejs.org/download/release/v22.22.0/node-v22.22.0-win-x64.zip)
- **Release Page**: [Node.js v22.22.0 Release](https://nodejs.org/download/release/v22.22.0/)

### Linux x64 (For development/testing)
- [node-v22.22.0-linux-x64.tar.xz](https://nodejs.org/download/release/v22.22.0/node-v22.22.0-linux-x64.tar.xz)

### macOS (For development/testing)
- [node-v22.22.0-darwin-x64.tar.gz](https://nodejs.org/download/release/v22.22.0/node-v22.22.0-darwin-x64.tar.gz)
- [node-v22.22.0-darwin-arm64.tar.gz](https://nodejs.org/download/release/v22.22.0/node-v22.22.0-darwin-arm64.tar.gz) (Apple Silicon)

## Quick Install Steps

### 1. Download
```bash
# Windows (PowerShell)
curl -o node-v22.22.0-win-x64.zip https://nodejs.org/download/release/v22.22.0/node-v22.22.0-win-x64.zip

# Or download manually from the link above
```

### 2. Extract Files
```
node-v22.22.0-win-x64.zip
  ↓ Extract to Project-Reboot-3.0/vendor/libnode/
```

### 3. Organize Directory Structure
```
Project-Reboot-3.0/
├── vendor/
│   └── libnode/
│       ├── include/
│       │   └── node/
│       │       ├── node.h
│       │       ├── node_api.h
│       │       ├── v8.h
│       │       ├── uv.h
│       │       └── (all other .h files)
│       └── lib/
│           └── node.lib
└── Project Reboot 3.0/
    └── node.exe
```

### 4. Enable in Visual Studio

Add to **Project Properties → C/C++ → Preprocessor**:
```
ENABLE_LIBNODE
```

Add to **C/C++ → General → Additional Include Directories**:
```
$(ProjectDir)..\vendor\libnode\include\node
```

Add to **Linker → General → Additional Library Directories**:
```
$(ProjectDir)..\vendor\libnode\lib
```

Add to **Linker → Input → Additional Dependencies**:
```
node.lib
```

### 5. Build
```
Build → Rebuild Solution
```

## Verify Installation

When the DLL loads, you should see:
```
========================================
 Icarus Node.js Runtime
 Project Reboot JavaScript Bindings
========================================

Node.js version: v22.22.0
Working directory: C:\...\Project-Reboot-3.0

Icarus runtime is ready!
========================================
```

**If you see "Mock Mode" instead**: The ENABLE_LIBNODE flag is not set or libnode files are missing.

**If you see a different version**: You have the wrong Node.js version installed. Delete vendor/libnode/ and reinstall 22.22.0.

## Why Version 22.22.0?

Node.js 22.22.0 is specifically required because:

1. **ABI Compatibility**: The native bindings are compiled against v22.22's Node-API
2. **V8 Engine Version**: Specific V8 features used by Icarus are available in this version
3. **Stability**: This version has been tested and verified to work with Project Reboot
4. **API Changes**: Newer/older versions may have incompatible API changes

Using a different version will result in:
- Runtime crashes
- Undefined behavior
- Failed initialization
- Missing features

## Troubleshooting

### "Cannot open include file: 'node.h'"
- Verify `vendor/libnode/include/node/` contains header files
- Check include path in project properties

### "Unresolved external symbol"
- Verify `vendor/libnode/lib/node.lib` exists
- Check library path in linker settings
- Ensure building for x64 (not x86)

### "The specified module could not be found"
- Copy `node.exe` to same directory as the compiled DLL
- Or add Node.js bin directory to PATH

### Mock Mode Still Shows
- Verify `ENABLE_LIBNODE` is in preprocessor definitions
- Clean and rebuild solution
- Check that all files are in the correct locations

## Additional Resources

- [Full Setup Guide](./LIBNODE_SETUP.md)
- [Node.js Integration Architecture](./NODEJS_INTEGRATION.md)
- [Icarus SDK Documentation](./ICARUS_SDK.md)

## Checksum Verification (Optional)

To verify your download is correct:

**SHA256 for node-v22.22.0-win-x64.zip**:
Check the SHASUMS256.txt file at the release page for the official checksum.

```powershell
# Windows PowerShell
Get-FileHash node-v22.22.0-win-x64.zip -Algorithm SHA256
```

Compare the output with the official SHASUMS256.txt file.
