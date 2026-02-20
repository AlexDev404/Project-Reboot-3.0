# Project Reboot 3.0 Documentation

Welcome to the Project Reboot 3.0 documentation. This documentation is organized into sections for different audiences.

## Quick Links

| Document | Description | Audience |
|----------|-------------|----------|
| [Architecture Guide](./ARCHITECTURE.md) | Technical deep-dive into the codebase | Maintainers & Contributors |
| [User Guide](./USER_GUIDE.md) | How to build, configure, and run the server | End Users |
| [Icarus SDK](./ICARUS_SDK.md) | JavaScript/TypeScript module development | Module Developers |
| [Node.js 22.22 Setup](./NODE_22_22_SETUP.md) | Quick start for Node.js 22.22 installation | All Users |
| [libnode Setup Guide](./LIBNODE_SETUP.md) | Detailed Node.js embedding setup | Advanced Users |
| [Node.js Integration](./NODEJS_INTEGRATION.md) | Architecture and technical details | Maintainers |

## What is Project Reboot?

Project Reboot is a game server implementation for Fortnite, supporting Seasons 3 through 15 (with varying compatibility for other seasons). It allows hosting private Fortnite matches outside of Epic Games' official infrastructure.

### Key Features

- **Multi-Season Support**: Full support for S3-S15, with experimental support for other seasons
- **Icarus Module System**: JavaScript/TypeScript extensibility through native bindings
- **Backend Services**: Account management, authentication, analytics, and more
- **Bot Support**: AI-controlled players to fill lobbies
- **Creative Mode**: Support for creative islands and game modes

## Repository Structure

```
Project-Reboot-3.0/
├── Project Reboot 3.0/       # C++ game server DLL
│   ├── icarus/               # JavaScript binding layer
│   │   ├── addon/            # Node.js native addon
│   │   ├── Icarus.h          # Main Icarus header
│   │   ├── IcarusRuntime.*   # JS runtime implementation
│   │   └── IcarusBindings.h  # Native C++ → JS bindings
│   ├── Fort*.h/cpp           # Fortnite class implementations
│   ├── reboot.h              # Core utilities
│   └── dllmain.cpp           # DLL entry point
├── backend/                  # Node.js backend services
│   ├── src/                  # TypeScript source
│   ├── packages/             # Monorepo packages
│   └── tests/                # Unit tests
├── vendor/                   # Third-party dependencies
├── docs/                     # Documentation (you are here)
└── build-linux.sh            # Linux build script (via Wine)
```

## Getting Started

### For Users
See the [User Guide](./USER_GUIDE.md) for step-by-step instructions on building and running the server.

### For Maintainers
See the [Architecture Guide](./ARCHITECTURE.md) for a technical overview of the codebase.

### For Module Developers
See the [Icarus SDK Documentation](./ICARUS_SDK.md) for information on extending the server with JavaScript/TypeScript modules.

## License

This project is licensed under the BSD-3-Clause license. See the [LICENSE](../LICENSE) file for details.

## Community

- **Discord**: [discord.gg/rebootmp](https://discord.gg/rebootmp)
- **GitHub Issues**: Report bugs and request features

---

*Project Reboot is a community project and is not affiliated with Epic Games.*
