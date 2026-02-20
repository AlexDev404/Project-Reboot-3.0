/**
 * FMutator - Game mutators and modifiers
 * 
 * Provides access to game mutators that modify gameplay mechanics.
 * Based on Project Reboot's FortAthenaMutator functionality.
 * 
 * Mutators are special game modifiers that can:
 * - Override inventory loadouts
 * - Change game rules (Gun Game, Team Deathmatch, etc.)
 * - Spawn special items at game phases
 * - Modify storm/barrier behavior
 * 
 * @example
 * ```ts
 * import { FMutator } from "@trail-blaze/retroflex";
 * 
 * // Get all active mutators
 * const mutators = FMutator.getAll();
 * 
 * // Enable a specific mutator
 * FMutator.enable("gunGame");
 * 
 * // Configure inventory override
 * FMutator.setInventoryLoadout([
 *   { itemId: "WID_Assault_Auto_Athena_SR_Ore_T03", count: 1 }
 * ]);
 * ```
 */

import { nativeBindings } from './bindings';

/**
 * Mutator types based on Project Reboot
 */
export type MutatorType = 
  | 'base'
  | 'bots'
  | 'barrier'
  | 'disco'
  | 'gunGame'
  | 'heist'
  | 'inventoryOverride'
  | 'itemDropOnDeath'
  | 'giveItemsAtPhase'
  | 'tdm'
  | 'loadoutSwap';

/**
 * Mutator information
 */
export interface MutatorInfo {
  id: string;
  type: MutatorType;
  name: string;
  isEnabled: boolean;
}

/**
 * Loadout item entry
 */
export interface LoadoutItem {
  itemId: string;
  count: number;
}

/**
 * Inventory loadout configuration
 */
export interface InventoryLoadout {
  items: LoadoutItem[];
  teamIndex?: number;
  spawnOverride?: 'always' | 'initial_spawn' | 'aircraft_phase_only';
  dropOnDeath?: boolean;
}

/**
 * Gun Game configuration
 */
export interface GunGameConfig {
  weapons: {
    itemId: string;
    killsRequired: number;
  }[];
  finalWeapon?: string;
  demotion?: boolean;
}

/**
 * Items to give at specific game phase
 */
export interface PhaseItemGrant {
  phase: number;
  items: LoadoutItem[];
}

export class FMutator {
  /**
   * Get all active mutators
   */
  static getAll(): MutatorInfo[] {
    if (nativeBindings.has('FMutator_GetAll')) {
      return nativeBindings.call('FMutator_GetAll') as MutatorInfo[];
    }
    return [];
  }
  
  /**
   * Get mutator by type
   * @param type Mutator type
   */
  static get(type: MutatorType): MutatorInfo | undefined {
    if (nativeBindings.has('FMutator_Get')) {
      return nativeBindings.call('FMutator_Get', type) as MutatorInfo;
    }
    return undefined;
  }
  
  /**
   * Enable a mutator
   * @param type Mutator type
   */
  static enable(type: MutatorType): boolean {
    if (nativeBindings.has('FMutator_Enable')) {
      return nativeBindings.call('FMutator_Enable', type) as boolean;
    }
    return false;
  }
  
  /**
   * Disable a mutator
   * @param type Mutator type
   */
  static disable(type: MutatorType): boolean {
    if (nativeBindings.has('FMutator_Disable')) {
      return nativeBindings.call('FMutator_Disable', type) as boolean;
    }
    return false;
  }
  
  // ==================== Inventory Override ====================
  
  /**
   * Set inventory loadout for all players
   * @param items Items to give
   * @param options Loadout options
   */
  static setInventoryLoadout(
    items: LoadoutItem[],
    options: Partial<Omit<InventoryLoadout, 'items'>> = {}
  ): void {
    if (nativeBindings.has('FMutator_SetInventoryLoadout')) {
      nativeBindings.call('FMutator_SetInventoryLoadout', { items, ...options });
    }
  }
  
  /**
   * Set team-specific inventory loadout
   * @param teamIndex Team index
   * @param loadout Inventory loadout
   */
  static setTeamLoadout(teamIndex: number, loadout: InventoryLoadout): void {
    if (nativeBindings.has('FMutator_SetTeamLoadout')) {
      nativeBindings.call('FMutator_SetTeamLoadout', teamIndex, loadout);
    }
  }
  
  // ==================== Gun Game ====================
  
  /**
   * Configure Gun Game mutator
   * @param config Gun Game configuration
   */
  static configureGunGame(config: GunGameConfig): void {
    if (nativeBindings.has('FMutator_ConfigureGunGame')) {
      nativeBindings.call('FMutator_ConfigureGunGame', config);
    }
  }
  
  /**
   * Get player's current Gun Game weapon index
   * @param playerId Player ID
   */
  static getGunGameProgress(playerId: string): number {
    if (nativeBindings.has('FMutator_GetGunGameProgress')) {
      return nativeBindings.call('FMutator_GetGunGameProgress', playerId) as number;
    }
    return 0;
  }
  
  /**
   * Set player's Gun Game weapon index
   * @param playerId Player ID
   * @param index Weapon index
   */
  static setGunGameProgress(playerId: string, index: number): void {
    if (nativeBindings.has('FMutator_SetGunGameProgress')) {
      nativeBindings.call('FMutator_SetGunGameProgress', playerId, index);
    }
  }
  
  // ==================== Phase Item Grants ====================
  
  /**
   * Configure items to give at specific game phases
   * @param grants Phase item grants
   */
  static setPhaseItemGrants(grants: PhaseItemGrant[]): void {
    if (nativeBindings.has('FMutator_SetPhaseItemGrants')) {
      nativeBindings.call('FMutator_SetPhaseItemGrants', grants);
    }
  }
  
  // ==================== Item Drop on Death ====================
  
  /**
   * Configure items dropped when players die
   * @param items Items to drop
   */
  static setDeathDropItems(items: LoadoutItem[]): void {
    if (nativeBindings.has('FMutator_SetDeathDropItems')) {
      nativeBindings.call('FMutator_SetDeathDropItems', items);
    }
  }
  
  /**
   * Set whether players drop all items on death
   * @param dropAll Whether to drop all items
   */
  static setDropAllOnDeath(dropAll: boolean): void {
    if (nativeBindings.has('FMutator_SetDropAllOnDeath')) {
      nativeBindings.call('FMutator_SetDropAllOnDeath', dropAll);
    }
  }
  
  // ==================== TDM (Team Deathmatch) ====================
  
  /**
   * Configure Team Deathmatch settings
   * @param scoreLimit Score limit to win
   * @param timeLimit Time limit in seconds
   */
  static configureTDM(scoreLimit: number, timeLimit?: number): void {
    if (nativeBindings.has('FMutator_ConfigureTDM')) {
      nativeBindings.call('FMutator_ConfigureTDM', scoreLimit, timeLimit);
    }
  }
  
  /**
   * Get team scores in TDM
   */
  static getTDMScores(): { teamId: number; score: number }[] {
    if (nativeBindings.has('FMutator_GetTDMScores')) {
      return nativeBindings.call('FMutator_GetTDMScores') as { teamId: number; score: number }[];
    }
    return [];
  }
  
  // ==================== Disco (Disco Domination) ====================
  
  /**
   * Get control point data for Disco Domination
   */
  static getDiscoControlPoints(): { id: string; team: number; progress: number }[] {
    if (nativeBindings.has('FMutator_GetDiscoControlPoints')) {
      return nativeBindings.call('FMutator_GetDiscoControlPoints') as { id: string; team: number; progress: number }[];
    }
    return [];
  }
}
