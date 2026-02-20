/**
 * FInventory - Inventory management utilities
 * 
 * Provides methods for managing player inventories, items, and quickbars.
 * Based on Project Reboot's FortInventory.h/cpp functionality.
 * 
 * @example
 * ```ts
 * import { FInventory } from "@trail-blaze/retroflex";
 * 
 * // Give item to player
 * FInventory.giveItem("player-123", "WID_Assault_Auto_Athena_SR_Ore_T03", 1);
 * 
 * // Remove all weapons
 * FInventory.removeAllOfType("player-123", "weapon");
 * 
 * // Swap items between slots
 * FInventory.swapSlots("player-123", 0, 1);
 * ```
 */

import { InventoryItem } from './types';
import { nativeBindings } from './bindings';

/**
 * Item types
 */
export type ItemType = 
  | 'weapon'
  | 'consumable'
  | 'resource'
  | 'ammo'
  | 'trap'
  | 'building'
  | 'cosmetic'
  | 'gadget';

/**
 * Quickbar types
 */
export type QuickbarType = 'primary' | 'secondary';

/**
 * Item rarity
 */
export type ItemRarity = 
  | 'common'
  | 'uncommon'
  | 'rare'
  | 'epic'
  | 'legendary'
  | 'mythic';

/**
 * Detailed item information
 */
export interface DetailedItemInfo extends InventoryItem {
  name: string;
  type: ItemType;
  rarity: ItemRarity;
  maxStackSize: number;
  durability?: number;
  maxDurability?: number;
  loadedAmmo?: number;
  maxLoadedAmmo?: number;
  ammoType?: string;
}

/**
 * Inventory state
 */
export interface InventoryState {
  items: DetailedItemInfo[];
  wood: number;
  stone: number;
  metal: number;
  lightAmmo: number;
  mediumAmmo: number;
  heavyAmmo: number;
  shellsAmmo: number;
  rocketAmmo: number;
}

export class FInventory {
  /**
   * Give an item to a player
   * @param playerId Player ID
   * @param itemId Item definition ID
   * @param quantity Quantity to give
   * @param showToast Whether to show pickup toast
   * @returns The created item info or undefined
   */
  static giveItem(
    playerId: string, 
    itemId: string, 
    quantity: number = 1,
    showToast: boolean = true
  ): DetailedItemInfo | undefined {
    if (nativeBindings.has('FInventory_GiveItem')) {
      return nativeBindings.call('FInventory_GiveItem', playerId, itemId, quantity, showToast) as DetailedItemInfo;
    }
    console.log(`[FInventory] Giving ${playerId} ${quantity}x ${itemId}`);
    return undefined;
  }
  
  /**
   * Remove an item from a player's inventory
   * @param playerId Player ID
   * @param itemGuid Item GUID (use getInventory to find)
   * @param count Count to remove (-1 = all)
   */
  static removeItem(playerId: string, itemGuid: string, count: number = -1): boolean {
    if (nativeBindings.has('FInventory_RemoveItem')) {
      return nativeBindings.call('FInventory_RemoveItem', playerId, itemGuid, count) as boolean;
    }
    return false;
  }
  
  /**
   * Remove all items of a specific type
   * @param playerId Player ID
   * @param itemType Item type to remove
   */
  static removeAllOfType(playerId: string, itemType: ItemType): number {
    if (nativeBindings.has('FInventory_RemoveAllOfType')) {
      return nativeBindings.call('FInventory_RemoveAllOfType', playerId, itemType) as number;
    }
    return 0;
  }
  
  /**
   * Clear entire inventory (except pickaxe)
   * @param playerId Player ID
   * @param includePickaxe Whether to also remove pickaxe
   */
  static clearInventory(playerId: string, includePickaxe: boolean = false): void {
    if (nativeBindings.has('FInventory_Clear')) {
      nativeBindings.call('FInventory_Clear', playerId, includePickaxe);
    }
  }
  
  /**
   * Get player's full inventory
   * @param playerId Player ID
   */
  static getInventory(playerId: string): InventoryState | undefined {
    if (nativeBindings.has('FInventory_GetInventory')) {
      return nativeBindings.call('FInventory_GetInventory', playerId) as InventoryState;
    }
    return undefined;
  }
  
  /**
   * Get specific item by GUID
   * @param playerId Player ID
   * @param itemGuid Item GUID
   */
  static getItem(playerId: string, itemGuid: string): DetailedItemInfo | undefined {
    if (nativeBindings.has('FInventory_GetItem')) {
      return nativeBindings.call('FInventory_GetItem', playerId, itemGuid) as DetailedItemInfo;
    }
    return undefined;
  }
  
  /**
   * Find item by definition ID
   * @param playerId Player ID
   * @param itemId Item definition ID
   */
  static findItem(playerId: string, itemId: string): DetailedItemInfo | undefined {
    if (nativeBindings.has('FInventory_FindItem')) {
      return nativeBindings.call('FInventory_FindItem', playerId, itemId) as DetailedItemInfo;
    }
    return undefined;
  }
  
  /**
   * Swap items between slots
   * @param playerId Player ID
   * @param slotA First slot index
   * @param slotB Second slot index
   */
  static swapSlots(playerId: string, slotA: number, slotB: number): boolean {
    if (nativeBindings.has('FInventory_SwapSlots')) {
      return nativeBindings.call('FInventory_SwapSlots', playerId, slotA, slotB) as boolean;
    }
    return false;
  }
  
  /**
   * Set item quantity
   * @param playerId Player ID
   * @param itemGuid Item GUID
   * @param quantity New quantity
   */
  static setQuantity(playerId: string, itemGuid: string, quantity: number): boolean {
    if (nativeBindings.has('FInventory_SetQuantity')) {
      return nativeBindings.call('FInventory_SetQuantity', playerId, itemGuid, quantity) as boolean;
    }
    return false;
  }
  
  /**
   * Set loaded ammo for a weapon
   * @param playerId Player ID
   * @param itemGuid Item GUID
   * @param ammoCount Loaded ammo count
   */
  static setLoadedAmmo(playerId: string, itemGuid: string, ammoCount: number): boolean {
    if (nativeBindings.has('FInventory_SetLoadedAmmo')) {
      return nativeBindings.call('FInventory_SetLoadedAmmo', playerId, itemGuid, ammoCount) as boolean;
    }
    return false;
  }
  
  /**
   * Give resources (wood, stone, metal)
   * @param playerId Player ID
   * @param wood Wood amount
   * @param stone Stone amount
   * @param metal Metal amount
   */
  static giveResources(playerId: string, wood: number, stone: number, metal: number): void {
    if (nativeBindings.has('FInventory_GiveResources')) {
      nativeBindings.call('FInventory_GiveResources', playerId, wood, stone, metal);
    }
  }
  
  /**
   * Set resources to specific amounts
   * @param playerId Player ID
   * @param wood Wood amount
   * @param stone Stone amount
   * @param metal Metal amount
   */
  static setResources(playerId: string, wood: number, stone: number, metal: number): void {
    if (nativeBindings.has('FInventory_SetResources')) {
      nativeBindings.call('FInventory_SetResources', playerId, wood, stone, metal);
    }
  }
  
  /**
   * Give ammo of a specific type
   * @param playerId Player ID
   * @param ammoType Ammo type ID
   * @param quantity Amount to give
   */
  static giveAmmo(playerId: string, ammoType: string, quantity: number): void {
    if (nativeBindings.has('FInventory_GiveAmmo')) {
      nativeBindings.call('FInventory_GiveAmmo', playerId, ammoType, quantity);
    }
  }
  
  /**
   * Set all ammo to max
   * @param playerId Player ID
   */
  static maxAmmo(playerId: string): void {
    if (nativeBindings.has('FInventory_MaxAmmo')) {
      nativeBindings.call('FInventory_MaxAmmo', playerId);
    }
  }
  
  /**
   * Equip item to quickbar slot
   * @param playerId Player ID
   * @param itemGuid Item GUID
   * @param slot Target slot
   */
  static equipToSlot(playerId: string, itemGuid: string, slot: number): boolean {
    if (nativeBindings.has('FInventory_EquipToSlot')) {
      return nativeBindings.call('FInventory_EquipToSlot', playerId, itemGuid, slot) as boolean;
    }
    return false;
  }
  
  /**
   * Force update inventory (sync with client)
   * @param playerId Player ID
   */
  static forceUpdate(playerId: string): void {
    if (nativeBindings.has('FInventory_ForceUpdate')) {
      nativeBindings.call('FInventory_ForceUpdate', playerId);
    }
  }
  
  /**
   * Drop an item from inventory
   * @param playerId Player ID
   * @param itemGuid Item GUID
   * @param count Amount to drop (-1 = all)
   */
  static dropItem(playerId: string, itemGuid: string, count: number = -1): boolean {
    if (nativeBindings.has('FInventory_DropItem')) {
      return nativeBindings.call('FInventory_DropItem', playerId, itemGuid, count) as boolean;
    }
    return false;
  }
  
  /**
   * Check if inventory is full (no empty weapon slots)
   * @param playerId Player ID
   */
  static isFull(playerId: string): boolean {
    if (nativeBindings.has('FInventory_IsFull')) {
      return nativeBindings.call('FInventory_IsFull', playerId) as boolean;
    }
    return false;
  }
  
  /**
   * Get the pickaxe instance
   * @param playerId Player ID
   */
  static getPickaxe(playerId: string): DetailedItemInfo | undefined {
    if (nativeBindings.has('FInventory_GetPickaxe')) {
      return nativeBindings.call('FInventory_GetPickaxe', playerId) as DetailedItemInfo;
    }
    return undefined;
  }
}
