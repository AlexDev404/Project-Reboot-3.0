// Property System implementation
// Provides the self-contained property storage that replaces UE4's offset-based memory access

#include "core/object_system.h"
#include "core/object_globals.h"
#include "util/logging.h"

// This file exists as a compilation unit for the property system.
// The core property logic is in object_system.cpp (UObject::GetProperty/SetProperty).
// This file provides additional helpers for property iteration and serialization.

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace PropertySystem
{

// Serialize an object's properties to JSON (for network replication or save)
json SerializeProperties(const UObject* Object)
{
    json J;
    // We'd iterate the object's property map here
    // For now this is a placeholder for the replication system
    J["__class"] = Object->GetClass() ? Object->GetClass()->GetName() : "UObject";
    J["__name"] = Object->GetName();
    return J;
}

// Deserialize JSON properties onto an object
void DeserializeProperties(UObject* Object, const json& J)
{
    if (!Object) return;

    for (auto& [Key, Value] : J.items())
    {
        if (Key.starts_with("__")) continue; // Skip metadata

        FName PropName(Key);
        if (Value.is_string())
            Object->SetProperty(PropName, FPropertyValue(Value.get<std::string>()));
        else if (Value.is_number_integer())
            Object->SetProperty(PropName, FPropertyValue(static_cast<int32>(Value.get<int64_t>())));
        else if (Value.is_number_float())
            Object->SetProperty(PropName, FPropertyValue(Value.get<float>()));
        else if (Value.is_boolean())
            Object->SetProperty(PropName, FPropertyValue(Value.get<bool>()));
    }
}

} // namespace PropertySystem
