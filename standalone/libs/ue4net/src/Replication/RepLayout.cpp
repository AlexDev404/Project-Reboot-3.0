// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted from Engine/Source/Runtime/Engine/Private/RepLayout.cpp
// Property replication layout implementation

#include "UE4Net/Replication/RepLayout.h"

FRepLayout::FRepLayout()
{
}

FRepLayout::~FRepLayout()
{
}

void FRepLayout::InitFromPropertyList(const FName& InClassName, const TArray<FRepLayoutCmd>& InCmds, const TArray<FRepParentCmd>& InParents)
{
    ClassName = InClassName;
    Cmds = InCmds;
    Parents = InParents;
}

bool FRepLayout::SerializeProperties(FBitWriter& Writer, const uint8* OldData, const uint8* NewData, int32 DataSize) const
{
    if (Cmds.Num() == 0) return true;

    bool bWroteAny = false;

    for (int32 i = 0; i < Cmds.Num(); i++)
    {
        const FRepLayoutCmd& Cmd = Cmds[i];
        if (Cmd.Type == ERepLayoutCmdType::Return || Cmd.Type == ERepLayoutCmdType::DynamicArray)
            continue;

        const uint8* OldProp = OldData ? (OldData + Cmd.Offset) : nullptr;
        const uint8* NewProp = NewData + Cmd.Offset;

        // Check if changed
        bool bChanged = (OldProp == nullptr) ||
                        (FMemory::Memcmp(OldProp, NewProp, Cmd.ElementSize) != 0);

        if (bChanged)
        {
            // Write handle
            uint16 Handle = Cmd.Handle;
            Writer.Serialize(&Handle, sizeof(uint16));

            // Write property
            SerializePropertyValue(Writer, Cmd, NewProp);
            bWroteAny = true;
        }
    }

    // Write terminator (handle = 0)
    if (bWroteAny)
    {
        uint16 Zero = 0;
        Writer.Serialize(&Zero, sizeof(uint16));
    }

    return !Writer.IsError();
}

bool FRepLayout::DeserializeProperties(FBitReader& Reader, uint8* DestData, int32 DataSize) const
{
    while (!Reader.AtEnd() && !Reader.IsError())
    {
        uint16 Handle = 0;
        Reader.Serialize(&Handle, sizeof(uint16));

        if (Reader.IsError()) return false;
        if (Handle == 0) break; // End of properties

        // Find the cmd for this handle
        const FRepLayoutCmd* Cmd = nullptr;
        for (int32 i = 0; i < Cmds.Num(); i++)
        {
            if (Cmds[i].Handle == Handle)
            {
                Cmd = &Cmds[i];
                break;
            }
        }

        if (Cmd && DestData)
        {
            DeserializePropertyValue(Reader, *Cmd, DestData + Cmd->Offset);
        }
        else
        {
            // Unknown property - can't skip properly without size info
            return false;
        }
    }

    return !Reader.IsError();
}

bool FRepLayout::SerializePropertyByHandle(FBitWriter& Writer, uint16 Handle, const uint8* Data) const
{
    for (int32 i = 0; i < Cmds.Num(); i++)
    {
        if (Cmds[i].Handle == Handle)
        {
            SerializePropertyValue(Writer, Cmds[i], Data + Cmds[i].Offset);
            return !Writer.IsError();
        }
    }
    return false;
}

int32 FRepLayout::FindPropertyHandle(const FName& PropName) const
{
    for (int32 i = 0; i < Cmds.Num(); i++)
    {
        if (Cmds[i].PropertyName == PropName)
        {
            return Cmds[i].Handle;
        }
    }
    return -1;
}

void FRepLayout::SerializePropertyValue(FBitWriter& Writer, const FRepLayoutCmd& Cmd, const uint8* Data) const
{
    switch (Cmd.Type)
    {
        case ERepLayoutCmdType::Property_Bool:
        {
            uint8 Val = *Data ? 1 : 0;
            Writer.WriteBit(Val);
            break;
        }
        case ERepLayoutCmdType::Property_Byte:
        case ERepLayoutCmdType::Property_Int8:
        {
            Writer.Serialize(const_cast<uint8*>(Data), 1);
            break;
        }
        case ERepLayoutCmdType::Property_UInt16:
        case ERepLayoutCmdType::Property_Int16:
        {
            Writer.Serialize(const_cast<uint8*>(Data), 2);
            break;
        }
        case ERepLayoutCmdType::Property_Int:
        case ERepLayoutCmdType::Property_UInt32:
        case ERepLayoutCmdType::Property_Float:
        {
            Writer.Serialize(const_cast<uint8*>(Data), 4);
            break;
        }
        case ERepLayoutCmdType::Property_UInt64:
        {
            Writer.Serialize(const_cast<uint8*>(Data), 8);
            break;
        }
        case ERepLayoutCmdType::Property_Vector:
        case ERepLayoutCmdType::Property_Rotator:
        {
            Writer.Serialize(const_cast<uint8*>(Data), 12); // 3 floats
            break;
        }
        case ERepLayoutCmdType::Property_String:
        {
            // String: length-prefixed
            const FString* Str = reinterpret_cast<const FString*>(Data);
            int32 Len = static_cast<int32>(Str->length());
            Writer.SerializeIntPacked(*reinterpret_cast<uint32*>(&Len));
            if (Len > 0)
            {
                Writer.Serialize(const_cast<char*>(Str->c_str()), Len);
            }
            break;
        }
        case ERepLayoutCmdType::Property_Name:
        {
            const FName* Name = reinterpret_cast<const FName*>(Data);
            const std::string& NameStr = Name->ToString();
            int32 Len = static_cast<int32>(NameStr.length());
            Writer.SerializeIntPacked(*reinterpret_cast<uint32*>(&Len));
            if (Len > 0)
            {
                Writer.Serialize(const_cast<char*>(NameStr.c_str()), Len);
            }
            break;
        }
        case ERepLayoutCmdType::Property_Object:
        case ERepLayoutCmdType::PropertyNetId:
        {
            // Network GUID (packed uint32)
            uint32 NetGUID = *reinterpret_cast<const uint32*>(Data);
            Writer.SerializeIntPacked(NetGUID);
            break;
        }
        default:
        {
            // Generic: write raw bytes
            Writer.Serialize(const_cast<uint8*>(Data), Cmd.ElementSize);
            break;
        }
    }
}

void FRepLayout::DeserializePropertyValue(FBitReader& Reader, const FRepLayoutCmd& Cmd, uint8* Data) const
{
    switch (Cmd.Type)
    {
        case ERepLayoutCmdType::Property_Bool:
        {
            *Data = Reader.ReadBit() ? 1 : 0;
            break;
        }
        case ERepLayoutCmdType::Property_Byte:
        case ERepLayoutCmdType::Property_Int8:
        {
            Reader.Serialize(Data, 1);
            break;
        }
        case ERepLayoutCmdType::Property_UInt16:
        case ERepLayoutCmdType::Property_Int16:
        {
            Reader.Serialize(Data, 2);
            break;
        }
        case ERepLayoutCmdType::Property_Int:
        case ERepLayoutCmdType::Property_UInt32:
        case ERepLayoutCmdType::Property_Float:
        {
            Reader.Serialize(Data, 4);
            break;
        }
        case ERepLayoutCmdType::Property_UInt64:
        {
            Reader.Serialize(Data, 8);
            break;
        }
        case ERepLayoutCmdType::Property_Vector:
        case ERepLayoutCmdType::Property_Rotator:
        {
            Reader.Serialize(Data, 12);
            break;
        }
        case ERepLayoutCmdType::Property_String:
        {
            FString* Str = reinterpret_cast<FString*>(Data);
            uint32 Len = 0;
            Reader.SerializeIntPacked(Len);
            if (Len > 0 && Len < 1024*1024) // Sanity limit
            {
                std::string Buf(Len, '\0');
                Reader.Serialize(&Buf[0], Len);
                *Str = Buf;
            }
            else
            {
                *Str = "";
            }
            break;
        }
        case ERepLayoutCmdType::Property_Name:
        {
            FName* Name = reinterpret_cast<FName*>(Data);
            uint32 Len = 0;
            Reader.SerializeIntPacked(Len);
            if (Len > 0 && Len < 1024*1024)
            {
                std::string Buf(Len, '\0');
                Reader.Serialize(&Buf[0], Len);
                *Name = FName(Buf);
            }
            break;
        }
        case ERepLayoutCmdType::Property_Object:
        case ERepLayoutCmdType::PropertyNetId:
        {
            uint32* NetGUID = reinterpret_cast<uint32*>(Data);
            Reader.SerializeIntPacked(*NetGUID);
            break;
        }
        default:
        {
            Reader.Serialize(Data, Cmd.ElementSize);
            break;
        }
    }
}

// FRepState implementation

FRepState::FRepState()
{
}

FRepState::~FRepState()
{
}

void FRepState::CompareProperties(const FRepLayout& Layout, const uint8* CurrentData, int32 DataSize)
{
    DirtyHandles.Empty();

    if (StaticBuffer.Num() == 0)
    {
        // First time - mark all dirty
        MarkAllDirty(Layout);
        // Save current state
        StaticBuffer.AddZeroed(DataSize);
        FMemory::Memcpy(StaticBuffer.GetData(), CurrentData, DataSize);
        return;
    }

    // Compare each property
    for (int32 i = 0; i < Layout.GetNumProperties(); i++)
    {
        const FRepLayoutCmd* Cmd = Layout.GetCmd(i);
        if (!Cmd || Cmd->Type == ERepLayoutCmdType::Return || Cmd->Type == ERepLayoutCmdType::DynamicArray)
            continue;

        if (Cmd->Offset + Cmd->ElementSize <= static_cast<uint16>(DataSize))
        {
            const uint8* OldProp = StaticBuffer.GetData() + Cmd->Offset;
            const uint8* NewProp = CurrentData + Cmd->Offset;

            if (FMemory::Memcmp(OldProp, NewProp, Cmd->ElementSize) != 0)
            {
                DirtyHandles.Add(Cmd->Handle);
            }
        }
    }

    // Update shadow state
    FMemory::Memcpy(StaticBuffer.GetData(), CurrentData, DataSize);
}

void FRepState::MarkAllDirty(const FRepLayout& Layout)
{
    DirtyHandles.Empty();
    for (int32 i = 0; i < Layout.GetNumProperties(); i++)
    {
        const FRepLayoutCmd* Cmd = Layout.GetCmd(i);
        if (Cmd && Cmd->Type != ERepLayoutCmdType::Return && Cmd->Type != ERepLayoutCmdType::DynamicArray)
        {
            DirtyHandles.Add(Cmd->Handle);
        }
    }
}
