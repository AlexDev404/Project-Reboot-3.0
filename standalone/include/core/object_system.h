#pragma once

// Standalone UE4 Object System Replacement
// Instead of reading live UE4 objects from Fortnite's process memory,
// we implement a self-contained object system with its own property storage.

#include "platform.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <any>
#include <variant>

// =============================================================================
// FName - Lightweight name identifier
// =============================================================================

class FName
{
public:
    FName() = default;
    FName(const std::string& InName) : Name(InName) {}
    FName(const char* InName) : Name(InName) {}

    const std::string& ToString() const { return Name; }
    bool operator==(const FName& Other) const { return Name == Other.Name; }
    bool operator!=(const FName& Other) const { return Name != Other.Name; }
    bool IsNone() const { return Name.empty() || Name == "None"; }

    struct Hash {
        size_t operator()(const FName& name) const {
            return std::hash<std::string>{}(name.Name);
        }
    };

private:
    std::string Name;
};

// =============================================================================
// FGuid
// =============================================================================

struct FGuid
{
    uint32 A = 0, B = 0, C = 0, D = 0;

    bool operator==(const FGuid& Other) const {
        return A == Other.A && B == Other.B && C == Other.C && D == Other.D;
    }
    bool operator!=(const FGuid& Other) const { return !(*this == Other); }
    bool IsValid() const { return A != 0 || B != 0 || C != 0 || D != 0; }

    static FGuid NewGuid();
};

// =============================================================================
// Math types
// =============================================================================

struct FVector
{
    float X = 0.f, Y = 0.f, Z = 0.f;

    FVector() = default;
    FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}

    FVector operator+(const FVector& V) const { return {X + V.X, Y + V.Y, Z + V.Z}; }
    FVector operator-(const FVector& V) const { return {X - V.X, Y - V.Y, Z - V.Z}; }
    FVector operator*(float Scale) const { return {X * Scale, Y * Scale, Z * Scale}; }
    float SizeSquared() const { return X*X + Y*Y + Z*Z; }
    float Size() const;
    static float Dist(const FVector& A, const FVector& B);
    static FVector ZeroVector;
};

struct FRotator
{
    float Pitch = 0.f, Yaw = 0.f, Roll = 0.f;

    FRotator() = default;
    FRotator(float InPitch, float InYaw, float InRoll) : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}
    FVector Vector() const;
    static FRotator ZeroRotator;
};

struct FTransform
{
    FVector Translation;
    FRotator Rotation;
    FVector Scale3D = {1.f, 1.f, 1.f};
};

struct FVector2D
{
    float X = 0.f, Y = 0.f;
};

// =============================================================================
// Property Value - variant type for property storage
// =============================================================================

using FPropertyValue = std::variant<
    std::monostate,     // None/null
    bool,
    int8,
    int16,
    int32,
    int64,
    uint8,
    uint16,
    uint32,
    uint64,
    float,
    double,
    std::string,
    FName,
    FGuid,
    FVector,
    FRotator,
    FTransform,
    UObject*,
    std::vector<uint8>,      // Raw data / arrays
    std::shared_ptr<void>    // Complex types
>;

// =============================================================================
// UObject - Base class for all objects
// =============================================================================

class UObject : public std::enable_shared_from_this<UObject>
{
public:
    UObject() = default;
    virtual ~UObject() = default;

    // Identity
    FName GetFName() const { return ObjectName; }
    std::string GetName() const { return ObjectName.ToString(); }
    std::string GetPathName() const;
    std::string GetFullName() const;
    UClass* GetClass() const { return ObjectClass; }
    UObject* GetOuter() const { return Outer; }

    // Type checking
    bool IsA(UClass* TestClass) const;
    bool IsA(const std::string& ClassName) const;

    // Property access (replaces offset-based memory reads)
    FPropertyValue GetProperty(const FName& PropertyName) const;
    void SetProperty(const FName& PropertyName, const FPropertyValue& Value);
    bool HasProperty(const FName& PropertyName) const;

    template<typename T>
    T GetPropertyValue(const FName& PropertyName) const {
        auto val = GetProperty(PropertyName);
        if (auto* ptr = std::get_if<T>(&val)) return *ptr;
        return T{};
    }

    template<typename T>
    void SetPropertyValue(const FName& PropertyName, const T& Value) {
        SetProperty(PropertyName, FPropertyValue(Value));
    }

    // Event system (replaces ProcessEvent)
    virtual void ProcessEvent(UFunction* Function, void* Params = nullptr);

    // Replication
    bool bNetDirty = false;
    void MarkDirtyForReplication() { bNetDirty = true; }

    // Object flags
    uint32 ObjectFlags = 0;
    int32 InternalIndex = -1;

    // Static helpers
    static UObject* StaticClass();

    // Object identity (public for access by object system)
    FName ObjectName;
    UClass* ObjectClass = nullptr;
    UObject* Outer = nullptr;

protected:
    std::unordered_map<FName, FPropertyValue, FName::Hash> Properties;

    friend class UObjectGlobals;
    friend class UAssetRegistry;
};

// =============================================================================
// UClass - Describes object types
// =============================================================================

class UClass : public UObject
{
public:
    UClass() = default;
    UClass(const std::string& InName, UClass* InSuperClass = nullptr);

    UClass* GetSuperClass() const { return SuperClass; }
    bool IsChildOf(const UClass* TestClass) const;

    // Function registry
    UFunction* FindFunction(const FName& FuncName) const;
    void AddFunction(const FName& FuncName, std::shared_ptr<UFunction> Func);

    // Default object
    UObject* GetDefaultObject() const { return DefaultObject; }
    void SetDefaultObject(UObject* CDO) { DefaultObject = CDO; }

    // Factory
    UObject* CreateDefaultSubobject(const FName& SubobjectName);

    // Properties metadata
    struct FPropertyInfo {
        FName Name;
        std::string TypeName;
        uint32 Size = 0;
        uint32 Offset = 0;
        bool bReplicated = false;
    };

    void RegisterProperty(const FPropertyInfo& Info);
    const std::vector<FPropertyInfo>& GetProperties() const { return RegisteredProperties; }

private:
    UClass* SuperClass = nullptr;
    UObject* DefaultObject = nullptr;
    std::unordered_map<FName, std::shared_ptr<UFunction>, FName::Hash> Functions;
    std::vector<FPropertyInfo> RegisteredProperties;
};

// =============================================================================
// UFunction - Callable function on an object
// =============================================================================

using FNativeFunc = std::function<void(UObject* Context, void* Params)>;

class UFunction : public UObject
{
public:
    UFunction() = default;
    UFunction(const std::string& InName, FNativeFunc InFunc);

    void Invoke(UObject* Context, void* Params = nullptr);

    // Function flags matching UE4
    enum EFunctionFlags : uint32
    {
        FUNC_None = 0x00000000,
        FUNC_Net = 0x00000040,
        FUNC_NetReliable = 0x00000080,
        FUNC_NetServer = 0x00200000,   // Server RPC
        FUNC_NetClient = 0x01000000,   // Client RPC
        FUNC_NetMulticast = 0x00004000,
        FUNC_BlueprintCallable = 0x04000000,
    };

    uint32 FunctionFlags = FUNC_None;
    uint16 ParmsSize = 0;

private:
    FNativeFunc NativeFunc;
};

// =============================================================================
// UStruct - Base for struct types (properties container)
// =============================================================================

class UStruct : public UObject
{
public:
    UStruct() = default;

    uint32 GetPropertiesSize() const { return PropertiesSize; }
    UStruct* GetSuperStruct() const { return SuperStruct; }

protected:
    UStruct* SuperStruct = nullptr;
    uint32 PropertiesSize = 0;
};
