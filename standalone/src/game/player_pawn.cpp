// Player Pawn implementation

#include "game/player_pawn.h"
#include "game/player_controller.h"
#include "game/player_state.h"
#include "util/logging.h"

#include <algorithm>

float AFortPlayerPawnAthena::TakeDamage(const FDamageEvent& DamageEvent)
{
    if (!IsAlive()) return 0.f;

    float ActualDamage = DamageEvent.Damage;

    // Headshot multiplier is already applied by caller
    // Apply damage to shield first, then health
    if (Stats.Shield > 0.f)
    {
        float ShieldDamage = std::min(Stats.Shield, ActualDamage);
        Stats.Shield -= ShieldDamage;
        ActualDamage -= ShieldDamage;
    }

    if (ActualDamage > 0.f)
    {
        Stats.Health -= ActualDamage;
        Stats.Health = std::max(Stats.Health, 0.f);
    }

    bNetDirty = true;

    if (Stats.Health <= 0.f)
    {
        // In squads/duos, enter DBNO instead of dying
        if (DamageEvent.bShouldKnock && !bIsDBNO)
        {
            EnterDBNO();
        }
        else
        {
            Die(DamageEvent.InstigatorController);
        }
    }

    return DamageEvent.Damage;
}

void AFortPlayerPawnAthena::Die(AFortPlayerControllerAthena* Killer)
{
    if (!IsAlive() && !bIsDBNO) return;

    Stats.Health = 0.f;
    bIsDBNO = false;
    bNetDirty = true;

    LOG_INFO(LogGame, "Pawn died (killed by {})",
        Killer ? Killer->GetPlayerName() : "environment");

    // Notify controller
    if (Controller)
    {
        Controller->bIsAlive = false;
        // Death report would be sent to client
        Controller->ClientOnPawnDied({});
    }
}

void AFortPlayerPawnAthena::EnterDBNO()
{
    bIsDBNO = true;
    DBNOTime = 0.f;
    Stats.Health = 100.f; // DBNO health
    bNetDirty = true;

    LOG_INFO(LogGame, "Player entered DBNO state");
}

void AFortPlayerPawnAthena::ServerReviveFromDBNO(AFortPlayerControllerAthena* Rescuer)
{
    if (!bIsDBNO) return;

    bIsDBNO = false;
    Stats.Health = 30.f; // Revive at 30 HP
    bNetDirty = true;

    LOG_INFO(LogGame, "Player revived by {}",
        Rescuer ? Rescuer->GetPlayerName() : "unknown");
}

void AFortPlayerPawnAthena::ServerSendZiplineState(const FZiplineState& State)
{
    ZiplineState = State;
    Stats.bIsOnZipline = State.bIsOnZipline;
    bNetDirty = true;
}

void AFortPlayerPawnAthena::ServerOnExitVehicle()
{
    CurrentVehicle = nullptr;
    Stats.bIsInVehicle = false;
    bNetDirty = true;
}

void AFortPlayerPawnAthena::EquipWeapon(const std::string& WeaponDefPath, int32 Slot)
{
    CurrentWeaponDef = WeaponDefPath;
    bNetDirty = true;
}

void AFortPlayerPawnAthena::GrantAbility(const std::string& AbilityClass)
{
    if (std::find(GrantedAbilities.begin(), GrantedAbilities.end(), AbilityClass) == GrantedAbilities.end())
    {
        GrantedAbilities.push_back(AbilityClass);
    }
}

void AFortPlayerPawnAthena::RemoveAbility(const std::string& AbilityClass)
{
    GrantedAbilities.erase(
        std::remove(GrantedAbilities.begin(), GrantedAbilities.end(), AbilityClass),
        GrantedAbilities.end()
    );
}

void AFortPlayerPawnAthena::Tick(float DeltaTime)
{
    // DBNO bleed-out timer
    if (bIsDBNO)
    {
        DBNOTime += DeltaTime;
        if (DBNOTime >= MaxDBNOTime)
        {
            Die(nullptr); // Bled out
        }
    }

    // Apply velocity/movement
    if (Velocity.SizeSquared() > 0.f)
    {
        Location = Location + Velocity * DeltaTime;
    }
}
