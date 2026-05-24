// Copyright Epic Games, Inc. All Rights Reserved.
// RPC dispatch implementation
// Based on UE4's ProcessRemoteFunction flow

#include "UE4Net/RPC/RPCDispatch.h"

FRPCDispatcher::FRPCDispatcher()
    : NextFunctionIndex(1)
{
}

FRPCDispatcher::~FRPCDispatcher()
{
}

void FRPCDispatcher::RegisterRPC(const FRPCDefinition& Definition)
{
    FRPCDefinition Def = Definition;
    if (Def.FunctionIndex == 0)
    {
        Def.FunctionIndex = NextFunctionIndex++;
    }
    RPCDefinitions.Add(std::move(Def));
}

void FRPCDispatcher::RegisterHandler(const FName& FunctionName, FRPCCallback Callback, void* UserData)
{
    FRPCHandlerEntry Entry;
    Entry.FunctionName = FunctionName;
    Entry.Callback = std::move(Callback);
    Entry.UserData = UserData;
    Handlers.Add(std::move(Entry));
}

bool FRPCDispatcher::SerializeRPC(const FName& FunctionName, const uint8* ParamData, int32 ParamSize, FBitWriter& OutWriter) const
{
    const FRPCDefinition* Def = FindRPC(FunctionName);
    if (!Def) return false;

    // Write function index
    uint32 FuncIdx = Def->FunctionIndex;
    OutWriter.SerializeIntPacked(FuncIdx);

    // Serialize parameters using the RPC's RepLayout
    if (ParamData && ParamSize > 0)
    {
        // Write param data size in bits
        uint32 ParamBits = static_cast<uint32>(ParamSize * 8);
        OutWriter.SerializeIntPacked(ParamBits);

        // Use RepLayout to serialize
        Def->ParamLayout.SerializeProperties(OutWriter, nullptr, ParamData, ParamSize);
    }
    else
    {
        // No params
        uint32 Zero = 0;
        OutWriter.SerializeIntPacked(Zero);
    }

    return !OutWriter.IsError();
}

bool FRPCDispatcher::DispatchRPC(const FName& FunctionName, FBitReader& InReader)
{
    // Find handler
    for (auto& Handler : Handlers)
    {
        if (Handler.FunctionName == FunctionName)
        {
            Handler.Callback(FunctionName, InReader, Handler.UserData);
            return true;
        }
    }
    return false; // No handler registered
}

const FRPCDefinition* FRPCDispatcher::FindRPC(const FName& FunctionName) const
{
    for (int32 i = 0; i < RPCDefinitions.Num(); i++)
    {
        if (RPCDefinitions[i].FunctionName == FunctionName)
        {
            return &RPCDefinitions[i];
        }
    }
    return nullptr;
}

const FRPCDefinition* FRPCDispatcher::FindRPCByIndex(uint32 FunctionIndex) const
{
    for (int32 i = 0; i < RPCDefinitions.Num(); i++)
    {
        if (RPCDefinitions[i].FunctionIndex == FunctionIndex)
        {
            return &RPCDefinitions[i];
        }
    }
    return nullptr;
}
