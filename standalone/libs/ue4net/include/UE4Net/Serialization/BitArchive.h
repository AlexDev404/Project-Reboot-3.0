// Compatibility shim - FBitArchive base class
#pragma once
#include "UE4Net/CoreMinimal.h"

// CVar used by BitWriter
inline TAutoConsoleVariable CVarMaxNetStringSize(TEXT("net.MaxNetStringSize"), 16 * 1024 * 1024, TEXT("Maximum allowed size for strings sent/received by the netcode (in bytes)."));
