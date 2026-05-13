#pragma once

// Player Pawn - The physical character in the world
// Handles movement, health, abilities, and combat

#include "core/object_system.h"
#include <string>
#include <vector>

class AFortPlayerControllerAthena;

// =============================================================================
// Player Stats
// =============================================================================

struct FPlayerStats
{
    float Health = 100.f;
    float MaxHealth = 100.f;
    float Shield = 0.f;
    float MaxShield = 100.f;

    // Resources
    int32 Wood = 0;
    int32 Stone = 0;
    int32 Metal = 0;

    // Movement
    float MoveSpeed = 600.f;
    bool bIsSprinting = false;
    bool bIsJumping = false;
    bool bIsCrouching = false;
    bool bIsOnZipline = false;
    bool bIsInVehicle = false;
    bool bIsSkydiving = false;
    bool bIsParachuting = false;
};

// =============================================================================
// Damage info
// =============================================================================

struct FDamageEvent
{
    float Damage = 0.f;
    FVector HitLocation;
    FVector ShotDirection;
    AFortPlayerControllerAthena* InstigatorController = nullptr;
    UObject* DamageCauser = nullptr; // Weapon or trap
    bool bIsHeadshot = false;
    bool bShouldKnock = false; // DBNO in squads
};

// =============================================================================
// Zipline state
// =============================================================================

struct FZiplineState
{
    bool bIsOnZipline = false;
    UObject* ZiplineActor = nullptr;
    float SocketOffset = 0.f;
    float Speed = 0.f;
    bool bJumpedOff = false;
};

// =============================================================================
// AFortPlayerPawnAthena
// =============================================================================

class AFortPlayerPawnAthena
{
public:
    AFortPlayerPawnAthena() = default;
    virtual ~AFortPlayerPawnAthena() = default;

    // Position & Movement
    FVector GetLocation() const { return Location; }
    void SetLocation(const FVector& NewLocation) { Location = NewLocation; }
    FRotator GetRotation() const { return Rotation; }
    void SetRotation(const FRotator& NewRotation) { Rotation = NewRotation; }
    FVector GetVelocity() const { return Velocity; }
    void SetVelocity(const FVector& NewVelocity) { Velocity = NewVelocity; }

    // Health/Shield
    float GetHealth() const { return Stats.Health; }
    float GetShield() const { return Stats.Shield; }
    void SetHealth(float Val) { Stats.Health = val_clamp(Val, 0.f, Stats.MaxHealth); }
    void SetShield(float Val) { Stats.Shield = val_clamp(Val, 0.f, Stats.MaxShield); }

    bool IsAlive() const { return Stats.Health > 0.f; }
    bool IsDBNO() const { return bIsDBNO; }
    bool IsSkydiving() const { return Stats.bIsSkydiving; }

    // Damage
    float TakeDamage(const FDamageEvent& DamageEvent);
    void Die(AFortPlayerControllerAthena* Killer);
    void EnterDBNO();
    void ServerReviveFromDBNO(AFortPlayerControllerAthena* Rescuer);

    // Controller
    AFortPlayerControllerAthena* GetController() const { return Controller; }
    void SetController(AFortPlayerControllerAthena* InController) { Controller = InController; }

    // Zipline
    void ServerSendZiplineState(const FZiplineState& State);
    const FZiplineState& GetZiplineState() const { return ZiplineState; }

    // Vehicle
    void ServerOnExitVehicle();
    UObject* GetVehicle() const { return CurrentVehicle; }

    // Weapons
    void EquipWeapon(const std::string& WeaponDefPath, int32 Slot);
    std::string GetCurrentWeaponDef() const { return CurrentWeaponDef; }

    // Abilities
    void GrantAbility(const std::string& AbilityClass);
    void RemoveAbility(const std::string& AbilityClass);

    // Customization
    void SetSkin(const std::string& SkinPath) { CosmeticSkin = SkinPath; }
    void SetBackbling(const std::string& BackPath) { CosmeticBackbling = BackPath; }
    void SetPickaxe(const std::string& PickaxePath) { CosmeticPickaxe = PickaxePath; }

    // Tick
    void Tick(float DeltaTime);

    // Stats access
    FPlayerStats& GetStats() { return Stats; }
    const FPlayerStats& GetStats() const { return Stats; }

    // Replication
    bool bNetDirty = false;

private:
    FVector Location;
    FRotator Rotation;
    FVector Velocity;
    FPlayerStats Stats;

    bool bIsDBNO = false;
    float DBNOTime = 0.f;
    float MaxDBNOTime = 54.f; // Bleed-out timer

    AFortPlayerControllerAthena* Controller = nullptr;
    UObject* CurrentVehicle = nullptr;
    FZiplineState ZiplineState;

    std::string CurrentWeaponDef;
    std::vector<std::string> GrantedAbilities;

    // Cosmetics
    std::string CosmeticSkin;
    std::string CosmeticBackbling;
    std::string CosmeticPickaxe;
    std::string CosmeticGlider;
    std::string CosmeticContrail;

    static float val_clamp(float val, float min, float max) {
        return val < min ? min : (val > max ? max : val);
    }
};
