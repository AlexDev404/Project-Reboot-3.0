// Abilities system - placeholder
// Handles gameplay abilities (sprinting, jumping, gliding, weapons, etc.)

#include "game/player_pawn.h"
#include "util/logging.h"

// In the original code, abilities are handled by:
// - UAbilitySystemComponent::InternalServerTryActivateAbility
// - GiveAbility / GiveAbilityAndActivateOnce
// - ClearAbility
// - CanActivateAbility
//
// For the standalone server, abilities primarily control:
// - Whether a player can sprint/build/edit
// - Weapon activation and firing
// - Consumable usage (medkits, shields)
// - Emotes and traversal
