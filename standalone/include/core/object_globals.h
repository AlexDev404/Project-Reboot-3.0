#pragma once

// UObject Global Functions - Replaces StaticFindObject/StaticLoadObject
// Instead of searching UE4's GUObjectArray in a live process,
// we maintain our own object registry.

#include "object_system.h"
#include <unordered_map>
#include <memory>
#include <vector>

class UObjectGlobals
{
public:
    static UObjectGlobals& Get();

    // Object registration
    void RegisterObject(const std::string& Path, std::shared_ptr<UObject> Object);
    void UnregisterObject(const std::string& Path);

    // Object lookup (replaces StaticFindObject)
    UObject* FindObject(const std::string& Path) const;
    UObject* FindObject(UClass* Class, UObject* Outer, const std::string& Name) const;

    template<typename T = UObject>
    T* FindObject(const std::string& Path) const {
        return static_cast<T*>(FindObject(Path));
    }

    // Object loading (replaces StaticLoadObject)
    UObject* LoadObject(const std::string& Path);

    template<typename T = UObject>
    T* LoadObject(const std::string& Path) {
        return static_cast<T*>(LoadObject(Path));
    }

    // Class registry
    void RegisterClass(const std::string& Path, std::shared_ptr<UClass> Class);
    UClass* FindClass(const std::string& Path) const;

    // Object creation
    template<typename T = UObject>
    T* NewObject(UClass* Class, UObject* Outer = nullptr, const FName& Name = FName()) {
        auto Obj = std::make_shared<T>();
        Obj->ObjectClass = Class;
        Obj->Outer = Outer;
        Obj->ObjectName = Name.IsNone() ? FName(Class->GetName()) : Name;
        Obj->InternalIndex = NextIndex++;

        std::string Path = Obj->GetPathName();
        ObjectMap[Path] = Obj;
        ObjectList.push_back(Obj);
        return Obj.get();
    }

    // Object iteration
    const std::vector<std::shared_ptr<UObject>>& GetAllObjects() const { return ObjectList; }
    int32 GetObjectCount() const { return static_cast<int32>(ObjectList.size()); }

    // Garbage collection (simplified)
    void CollectGarbage();

private:
    UObjectGlobals() = default;

    std::unordered_map<std::string, std::shared_ptr<UObject>> ObjectMap;
    std::unordered_map<std::string, std::shared_ptr<UClass>> ClassMap;
    std::vector<std::shared_ptr<UObject>> ObjectList;
    int32 NextIndex = 0;
};

// Convenience global functions matching the original API
template<typename T = UObject>
T* StaticFindObject(const std::string& Path) {
    return UObjectGlobals::Get().FindObject<T>(Path);
}

template<typename T = UObject>
T* StaticLoadObject(const std::string& Path) {
    return UObjectGlobals::Get().LoadObject<T>(Path);
}
