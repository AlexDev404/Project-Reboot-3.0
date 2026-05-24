// Object System implementation

#include "core/object_system.h"
#include "core/object_globals.h"
#include <cmath>
#include <random>
#include <algorithm>

// =============================================================================
// FVector
// =============================================================================

FVector FVector::ZeroVector = {0.f, 0.f, 0.f};
FRotator FRotator::ZeroRotator = {0.f, 0.f, 0.f};

float FVector::Size() const
{
    return std::sqrt(X * X + Y * Y + Z * Z);
}

float FVector::Dist(const FVector& A, const FVector& B)
{
    return (A - B).Size();
}

FVector FRotator::Vector() const
{
    const float PitchRad = Pitch * 3.14159265f / 180.f;
    const float YawRad = Yaw * 3.14159265f / 180.f;
    return FVector(
        std::cos(PitchRad) * std::cos(YawRad),
        std::cos(PitchRad) * std::sin(YawRad),
        std::sin(PitchRad)
    );
}

// =============================================================================
// FGuid
// =============================================================================

FGuid FGuid::NewGuid()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32> dist;

    return { dist(gen), dist(gen), dist(gen), dist(gen) };
}

// =============================================================================
// UObject
// =============================================================================

std::string UObject::GetPathName() const
{
    if (Outer)
    {
        return Outer->GetPathName() + "." + GetName();
    }
    return GetName();
}

std::string UObject::GetFullName() const
{
    std::string ClassName = ObjectClass ? ObjectClass->GetName() : "Object";
    return ClassName + " " + GetPathName();
}

bool UObject::IsA(UClass* TestClass) const
{
    if (!ObjectClass || !TestClass) return false;
    return ObjectClass->IsChildOf(TestClass);
}

bool UObject::IsA(const std::string& ClassName) const
{
    if (!ObjectClass) return false;
    return ObjectClass->GetName() == ClassName;
}

FPropertyValue UObject::GetProperty(const FName& PropertyName) const
{
    auto It = Properties.find(PropertyName);
    if (It != Properties.end()) return It->second;
    return FPropertyValue{}; // monostate = null
}

void UObject::SetProperty(const FName& PropertyName, const FPropertyValue& Value)
{
    Properties[PropertyName] = Value;
    bNetDirty = true;
}

bool UObject::HasProperty(const FName& PropertyName) const
{
    return Properties.find(PropertyName) != Properties.end();
}

void UObject::ProcessEvent(UFunction* Function, void* Params)
{
    if (!Function) return;
    Function->Invoke(this, Params);
}

UObject* UObject::StaticClass()
{
    static UClass ObjectClass("Object");
    return &ObjectClass;
}

// =============================================================================
// UClass
// =============================================================================

UClass::UClass(const std::string& InName, UClass* InSuperClass)
    : SuperClass(InSuperClass)
{
    ObjectName = FName(InName);
}

bool UClass::IsChildOf(const UClass* TestClass) const
{
    if (this == TestClass) return true;
    if (SuperClass) return SuperClass->IsChildOf(TestClass);
    return false;
}

UFunction* UClass::FindFunction(const FName& FuncName) const
{
    auto It = Functions.find(FuncName);
    if (It != Functions.end()) return It->second.get();
    if (SuperClass) return SuperClass->FindFunction(FuncName);
    return nullptr;
}

void UClass::AddFunction(const FName& FuncName, std::shared_ptr<UFunction> Func)
{
    Functions[FuncName] = std::move(Func);
}

UObject* UClass::CreateDefaultSubobject(const FName& SubobjectName)
{
    // Simplified - just create a UObject with this as outer
    auto Obj = std::make_shared<UObject>();
    Obj->ObjectName = SubobjectName;
    Obj->ObjectClass = this;
    return Obj.get();
}

void UClass::RegisterProperty(const FPropertyInfo& Info)
{
    RegisteredProperties.push_back(Info);
}

// =============================================================================
// UFunction
// =============================================================================

UFunction::UFunction(const std::string& InName, FNativeFunc InFunc)
    : NativeFunc(std::move(InFunc))
{
    ObjectName = FName(InName);
}

void UFunction::Invoke(UObject* Context, void* Params)
{
    if (NativeFunc)
    {
        NativeFunc(Context, Params);
    }
}

// =============================================================================
// UObjectGlobals
// =============================================================================

UObjectGlobals& UObjectGlobals::Get()
{
    static UObjectGlobals Instance;
    return Instance;
}

void UObjectGlobals::RegisterObject(const std::string& Path, std::shared_ptr<UObject> Object)
{
    ObjectMap[Path] = Object;
    ObjectList.push_back(Object);
    Object->InternalIndex = NextIndex++;
}

void UObjectGlobals::UnregisterObject(const std::string& Path)
{
    auto It = ObjectMap.find(Path);
    if (It != ObjectMap.end())
    {
        ObjectMap.erase(It);
    }
}

UObject* UObjectGlobals::FindObject(const std::string& Path) const
{
    auto It = ObjectMap.find(Path);
    if (It != ObjectMap.end()) return It->second.get();
    return nullptr;
}

UObject* UObjectGlobals::FindObject(UClass* Class, UObject* Outer, const std::string& Name) const
{
    for (const auto& [Path, Obj] : ObjectMap)
    {
        if (Obj->GetName() == Name)
        {
            if (Class && !Obj->IsA(Class)) continue;
            if (Outer && Obj->GetOuter() != Outer) continue;
            return Obj.get();
        }
    }
    return nullptr;
}

UObject* UObjectGlobals::LoadObject(const std::string& Path)
{
    // First check if already registered
    auto* Existing = FindObject(Path);
    if (Existing) return Existing;

    // TODO: Load from asset registry
    return nullptr;
}

void UObjectGlobals::RegisterClass(const std::string& Path, std::shared_ptr<UClass> Class)
{
    ClassMap[Path] = Class;
    RegisterObject(Path, Class);
}

UClass* UObjectGlobals::FindClass(const std::string& Path) const
{
    auto It = ClassMap.find(Path);
    if (It != ClassMap.end()) return It->second.get();
    return nullptr;
}

void UObjectGlobals::CollectGarbage()
{
    // Simplified GC - remove objects with no external references
    // In practice, game objects are managed by the game mode
}
