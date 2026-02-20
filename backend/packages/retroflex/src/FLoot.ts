/**
 * FLoot - Loot spawning and management
 * 
 * Provides methods for spawning items, managing loot pools, and supply drops.
 * 
 * @example
 * ```ts
 * import { FLoot } from "@trail-blaze/retroflex";
 * 
 * // Spawn an item on the ground
 * FLoot.spawnItem("WID_Assault_Auto_Athena_SR_Ore_T03", { x: 100, y: 200, z: 300 });
 * 
 * // Spawn a supply drop
 * FLoot.spawnSupplyDrop({ x: 0, y: 0, z: 5000 });
 * 
 * // Spawn a llama
 * FLoot.spawnLlama({ x: 500, y: 500, z: 300 });
 * ```
 */

import { FVector } from './types';
import { nativeBindings, BindingNames } from './bindings';

/**
 * Loot item info
 */
export interface LootItemInfo {
  id: string;
  itemId: string;
  quantity: number;
  location: FVector;
  isPickedUp: boolean;
}

/**
 * Supply drop info
 */
export interface SupplyDropInfo {
  id: string;
  location: FVector;
  isLanded: boolean;
  isOpened: boolean;
  contents: { itemId: string; quantity: number }[];
}

/**
 * Chest/Container info
 */
export interface ContainerInfo {
  id: string;
  type: 'chest' | 'ammo_box' | 'llama' | 'safe' | 'cooler';
  location: FVector;
  isOpened: boolean;
}

/**
 * Item spawn options
 */
export interface ItemSpawnOptions {
  quantity?: number;
  /** Time in seconds before item despawns (0 = never) */
  despawnTime?: number;
}

export class FLoot {
  /**
   * Spawn an item on the ground
   * @param itemId Item definition ID
   * @param location World location
   * @param options Spawn options
   * @returns Loot item info
   */
  static spawnItem(
    itemId: string,
    location: FVector,
    options: ItemSpawnOptions = {}
  ): LootItemInfo | undefined {
    if (nativeBindings.has(BindingNames.GAME_SPAWN_LOOT)) {
      return nativeBindings.call(BindingNames.GAME_SPAWN_LOOT, itemId, location, options) as LootItemInfo;
    }
    console.log(`[FLoot] Spawning ${itemId} at (${location.x}, ${location.y}, ${location.z})`);
    return undefined;
  }
  
  /**
   * Spawn multiple items at a location
   * @param items Array of item IDs and quantities
   * @param location World location
   * @param spread Spread radius for items
   */
  static spawnItems(
    items: { itemId: string; quantity: number }[],
    location: FVector,
    spread: number = 50
  ): LootItemInfo[] {
    if (nativeBindings.has('FLoot_SpawnItems')) {
      return nativeBindings.call('FLoot_SpawnItems', items, location, spread) as LootItemInfo[];
    }
    return [];
  }
  
  /**
   * Spawn a supply drop
   * @param location Target landing location
   * @param contents Optional custom contents
   * @returns Supply drop info
   */
  static spawnSupplyDrop(
    location: FVector,
    contents?: { itemId: string; quantity: number }[]
  ): SupplyDropInfo | undefined {
    if (nativeBindings.has('FLoot_SpawnSupplyDrop')) {
      return nativeBindings.call('FLoot_SpawnSupplyDrop', location, contents) as SupplyDropInfo;
    }
    console.log(`[FLoot] Spawning supply drop at (${location.x}, ${location.y}, ${location.z})`);
    return undefined;
  }
  
  /**
   * Spawn a llama
   * @param location World location
   * @returns Container info
   */
  static spawnLlama(location: FVector): ContainerInfo | undefined {
    if (nativeBindings.has('FLoot_SpawnLlama')) {
      return nativeBindings.call('FLoot_SpawnLlama', location) as ContainerInfo;
    }
    console.log(`[FLoot] Spawning llama at (${location.x}, ${location.y}, ${location.z})`);
    return undefined;
  }
  
  /**
   * Spawn a chest
   * @param location World location
   * @param tier Chest tier (1-3)
   */
  static spawnChest(location: FVector, tier: number = 1): ContainerInfo | undefined {
    if (nativeBindings.has('FLoot_SpawnChest')) {
      return nativeBindings.call('FLoot_SpawnChest', location, tier) as ContainerInfo;
    }
    return undefined;
  }
  
  /**
   * Get all ground loot items
   */
  static getAllItems(): LootItemInfo[] {
    if (nativeBindings.has('FLoot_GetAllItems')) {
      return nativeBindings.call('FLoot_GetAllItems') as LootItemInfo[];
    }
    return [];
  }
  
  /**
   * Get all supply drops
   */
  static getAllSupplyDrops(): SupplyDropInfo[] {
    if (nativeBindings.has('FLoot_GetAllSupplyDrops')) {
      return nativeBindings.call('FLoot_GetAllSupplyDrops') as SupplyDropInfo[];
    }
    return [];
  }
  
  /**
   * Get all containers (chests, llamas, etc.)
   */
  static getAllContainers(): ContainerInfo[] {
    if (nativeBindings.has('FLoot_GetAllContainers')) {
      return nativeBindings.call('FLoot_GetAllContainers') as ContainerInfo[];
    }
    return [];
  }
  
  /**
   * Destroy a loot item
   * @param itemId Loot item ID
   */
  static destroyItem(itemId: string): boolean {
    if (nativeBindings.has('FLoot_DestroyItem')) {
      return nativeBindings.call('FLoot_DestroyItem', itemId) as boolean;
    }
    return false;
  }
  
  /**
   * Clear all ground loot
   * @returns Number of items cleared
   */
  static clearAllItems(): number {
    if (nativeBindings.has('FLoot_ClearAll')) {
      return nativeBindings.call('FLoot_ClearAll') as number;
    }
    return 0;
  }
  
  /**
   * Set loot spawn rate multiplier
   * @param multiplier Spawn rate multiplier (1.0 = normal)
   */
  static setSpawnRateMultiplier(multiplier: number): void {
    if (nativeBindings.has('FLoot_SetSpawnRate')) {
      nativeBindings.call('FLoot_SetSpawnRate', multiplier);
    }
  }
  
  /**
   * Set supply drop frequency
   * @param intervalSeconds Seconds between supply drops (0 = disabled)
   */
  static setSupplyDropInterval(intervalSeconds: number): void {
    if (nativeBindings.has('FLoot_SetSupplyDropInterval')) {
      nativeBindings.call('FLoot_SetSupplyDropInterval', intervalSeconds);
    }
  }
  
  /**
   * Force open a container
   * @param containerId Container ID
   */
  static openContainer(containerId: string): void {
    if (nativeBindings.has('FLoot_OpenContainer')) {
      nativeBindings.call('FLoot_OpenContainer', containerId);
    }
  }
  
  /**
   * Reset all chests (make them unopened again)
   */
  static resetAllChests(): void {
    if (nativeBindings.has('FLoot_ResetChests')) {
      nativeBindings.call('FLoot_ResetChests');
    }
  }
}
