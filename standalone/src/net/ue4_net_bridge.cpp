// UE4 Net Bridge - Isolates ue4net includes from the rest of the codebase
// This file provides C-style functions that the engine can call without
// including ue4net headers (which conflict with our object_system types).

#include "net/ue4_net_driver.h"
#include "net/ue4_replication.h"
#include "util/logging.h"

#include <spdlog/fmt/fmt.h>

// Forward-declared function implemented in engine.cpp (avoids header conflicts)
extern void Engine_OnPlayerConnected(uint32_t ConnectionId);
extern void Engine_OnPlayerDisconnected(uint32_t ConnectionId, const std::string& PlayerName);

// =============================================================================
// Global driver pointer for cross-module access
// =============================================================================

static UE4NetDriver* s_GlobalUE4NetDriver = nullptr;

static void SetGlobalDriver(UE4NetDriver* Driver)
{
    s_GlobalUE4NetDriver = Driver;
}

// Global helper callable from main.cpp without including ue4net headers
int32_t UE4NetBridge_GetNumConnections()
{
    return s_GlobalUE4NetDriver ? s_GlobalUE4NetDriver->GetNumConnections() : 0;
}

// =============================================================================
// Bridge namespace - called by engine.cpp
// =============================================================================

namespace UE4NetBridge {

UE4NetDriver* CreateDriver()
{
    return new UE4NetDriver();
}

void DestroyDriver(UE4NetDriver* Driver)
{
    delete Driver;
}

bool InitializeDriver(UE4NetDriver* Driver, uint16_t Port)
{
    if (!Driver) return false;

    if (!Driver->Initialize(Port)) return false;

    // Set up the replication manager
    UE4ReplicationManager::Get().SetDriver(Driver);

    // Store global reference for bridge access
    SetGlobalDriver(Driver);

    // Set up player connection callbacks
    Driver->OnPlayerConnected = [](UE4NetConnection* Conn) {
        LOG_INFO(LogNet, "UE4: Player connected (ID={})", Conn->GetConnectionId());
        Engine_OnPlayerConnected(Conn->GetConnectionId());
    };

    Driver->OnPlayerDisconnected = [](UE4NetConnection* Conn) {
        LOG_INFO(LogNet, "UE4: Player disconnected: {} (ID={})",
            Conn->GetPlayerName(), Conn->GetConnectionId());
        Engine_OnPlayerDisconnected(Conn->GetConnectionId(), Conn->GetPlayerName());
    };

    Driver->OnPlayerLogin = [](UE4NetConnection* Conn, const std::string& URL, const std::string& UniqueId) {
        LOG_INFO(LogNet, "UE4: Player login: UniqueId={}", UniqueId);
    };

    return true;
}

void ShutdownDriver(UE4NetDriver* Driver)
{
    if (Driver) Driver->Shutdown();
    SetGlobalDriver(nullptr);
}

void TickDriver(UE4NetDriver* Driver, float DeltaTime)
{
    if (Driver) Driver->TickFlush(DeltaTime);
}

void TickReplication(float DeltaTime)
{
    UE4ReplicationManager::Get().ServerReplicateActors(DeltaTime);
}

int32_t GetNumConnections(UE4NetDriver* Driver)
{
    return Driver ? Driver->GetNumConnections() : 0;
}

bool IsListening(UE4NetDriver* Driver)
{
    return Driver ? Driver->IsListening() : false;
}

} // namespace UE4NetBridge
