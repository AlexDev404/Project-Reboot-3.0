// Event Dispatcher implementation

#include "core/event_dispatcher.h"
#include "util/logging.h"

UEventDispatcher& UEventDispatcher::Get()
{
    static UEventDispatcher Instance;
    return Instance;
}

void UEventDispatcher::RegisterHandler(const std::string& FunctionPath, FEventHandler Handler)
{
    HandlerMap[FunctionPath] = std::move(Handler);
}

void UEventDispatcher::RegisterHandler(const std::string& ClassName, const std::string& FuncName, FEventHandler Handler)
{
    std::string Path = ClassName + "." + FuncName;
    HandlerMap[Path] = std::move(Handler);
}

bool UEventDispatcher::DispatchEvent(UObject* Context, UFunction* Function, void* Params)
{
    if (!Function) return false;

    std::string FuncPath = Function->GetPathName();

    auto It = HandlerMap.find(FuncPath);
    if (It != HandlerMap.end())
    {
        It->second(Context, Params);
        return true;
    }

    // Also try just the function name
    std::string FuncName = Function->GetName();
    It = HandlerMap.find(FuncName);
    if (It != HandlerMap.end())
    {
        It->second(Context, Params);
        return true;
    }

    return false;
}

void UEventDispatcher::RegisterRPC(const FRPCInfo& Info)
{
    RPCMap[Info.FunctionName] = Info;
}

const UEventDispatcher::FRPCInfo* UEventDispatcher::FindRPC(const std::string& FunctionName) const
{
    auto It = RPCMap.find(FunctionName);
    if (It != RPCMap.end()) return &It->second;
    return nullptr;
}
