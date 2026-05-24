#pragma once

// Event Dispatcher - Replaces UE4's ProcessEvent system
// In the original DLL, ProcessEvent calls into UE4's Blueprint VM.
// Here we route events to registered native C++ handlers.

#include "object_system.h"
#include <unordered_map>
#include <functional>
#include <vector>

using FEventHandler = std::function<void(UObject* Context, void* Params)>;

class UEventDispatcher
{
public:
    static UEventDispatcher& Get();

    // Register a handler for a specific function path
    // e.g., "/Script/FortniteGame.FortPlayerController.ServerAttemptInteract"
    void RegisterHandler(const std::string& FunctionPath, FEventHandler Handler);

    // Register handler by class + function name
    void RegisterHandler(const std::string& ClassName, const std::string& FuncName, FEventHandler Handler);

    // Dispatch an event (called by UObject::ProcessEvent)
    bool DispatchEvent(UObject* Context, UFunction* Function, void* Params);

    // RPC routing
    enum class ERPCType { Server, Client, Multicast };

    struct FRPCInfo {
        std::string FunctionName;
        ERPCType Type;
        bool bReliable;
        FEventHandler Handler;
    };

    void RegisterRPC(const FRPCInfo& Info);
    const FRPCInfo* FindRPC(const std::string& FunctionName) const;

private:
    UEventDispatcher() = default;

    std::unordered_map<std::string, FEventHandler> HandlerMap;
    std::unordered_map<std::string, FRPCInfo> RPCMap;
};
