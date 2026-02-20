/**
 * FPawn - Represents a player pawn in the game world
 * 
 * Provides methods to interact with player characters including:
 * - Movement and positioning
 * - Cosmetics and appearance
 * - Health and damage
 * - Inventory management
 * - Communication
 * 
 * @example
 * ```ts
 * const pawn = FWorld.getPawnByUsername("Player1");
 * pawn.move(100, 200, 300);
 * pawn.costume("CID_016_Athena_Commando_F");
 * pawn.setHealth(100);
 * pawn.giveItem("WID_Assault_Auto_Athena_SR_Ore_T03", 1);
 * ```
 */

import { 
  FVector, 
  FRotator, 
  CosmeticLoadout, 
  InventoryItem, 
  PawnStats,
  PawnState 
} from './types';
import { nativeBindings, BindingNames } from './bindings';

export class FPawn {
  /** Unique pawn identifier */
  readonly id: string;
  
  /** Player's username/display name */
  readonly username: string;
  
  /** Account ID associated with this pawn */
  readonly accountId: string;
  
  /** Team information */
  readonly teamId: number;
  
  constructor(id: string, username: string, accountId: string, teamId: number = 0) {
    this.id = id;
    this.username = username;
    this.accountId = accountId;
    this.teamId = teamId;
  }
  
  // ==================== Movement ====================
  
  /**
   * Move the pawn to specified coordinates
   * @param x X coordinate
   * @param y Y coordinate  
   * @param z Z coordinate
   */
  move(x: number, y: number, z: number): void {
    if (nativeBindings.has(BindingNames.PAWN_MOVE)) {
      nativeBindings.call(BindingNames.PAWN_MOVE, this.id, x, y, z);
    } else {
      console.log(`[FPawn] Moving ${this.username} to (${x}, ${y}, ${z})`);
    }
  }
  
  /**
   * Teleport the pawn instantly to coordinates
   * @param x X coordinate
   * @param y Y coordinate
   * @param z Z coordinate
   */
  teleport(x: number, y: number, z: number): void {
    if (nativeBindings.has(BindingNames.PAWN_TELEPORT)) {
      nativeBindings.call(BindingNames.PAWN_TELEPORT, this.id, x, y, z);
    } else {
      console.log(`[FPawn] Teleporting ${this.username} to (${x}, ${y}, ${z})`);
    }
  }
  
  /**
   * Set pawn rotation
   * @param pitch Pitch angle
   * @param yaw Yaw angle
   * @param roll Roll angle
   */
  setRotation(pitch: number, yaw: number, roll: number): void {
    if (nativeBindings.has(BindingNames.PAWN_SET_ROTATION)) {
      nativeBindings.call(BindingNames.PAWN_SET_ROTATION, this.id, pitch, yaw, roll);
    }
  }
  
  /**
   * Get current pawn location
   */
  getLocation(): FVector {
    if (nativeBindings.has(BindingNames.PAWN_GET_LOCATION)) {
      return nativeBindings.call(BindingNames.PAWN_GET_LOCATION, this.id) as FVector;
    }
    return { x: 0, y: 0, z: 0 };
  }
  
  /**
   * Get current pawn rotation
   */
  getRotation(): FRotator {
    if (nativeBindings.has(BindingNames.PAWN_GET_ROTATION)) {
      return nativeBindings.call(BindingNames.PAWN_GET_ROTATION, this.id) as FRotator;
    }
    return { pitch: 0, yaw: 0, roll: 0 };
  }
  
  // ==================== Cosmetics ====================
  
  /**
   * Set the pawn's costume/skin
   * Also updates the value in the profile's directory
   * @param cosmeticId Cosmetic ID (e.g., "CID_016_Athena_Commando_F")
   */
  costume(cosmeticId: string): void {
    if (nativeBindings.has(BindingNames.PAWN_COSTUME)) {
      nativeBindings.call(BindingNames.PAWN_COSTUME, this.id, cosmeticId);
    } else {
      console.log(`[FPawn] Setting ${this.username} costume to ${cosmeticId}`);
    }
  }
  
  /**
   * Set full cosmetic loadout
   * @param loadout Cosmetic loadout configuration
   */
  setLoadout(loadout: CosmeticLoadout): void {
    if (loadout.character) this.costume(loadout.character);
    // Additional cosmetics would use similar native calls
    if (nativeBindings.has(BindingNames.PAWN_COSTUME)) {
      if (loadout.backpack) {
        nativeBindings.call(BindingNames.PAWN_COSTUME, this.id, loadout.backpack);
      }
      if (loadout.pickaxe) {
        nativeBindings.call(BindingNames.PAWN_COSTUME, this.id, loadout.pickaxe);
      }
    }
  }
  
  /**
   * Get current cosmetic loadout
   */
  getCosmetics(): CosmeticLoadout {
    if (nativeBindings.has(BindingNames.PAWN_GET_COSMETICS)) {
      return nativeBindings.call(BindingNames.PAWN_GET_COSMETICS, this.id) as CosmeticLoadout;
    }
    return {};
  }
  
  // ==================== Health & Combat ====================
  
  /**
   * Kill/eliminate the pawn (kick from world)
   */
  kill(): void {
    if (nativeBindings.has(BindingNames.PAWN_KILL)) {
      nativeBindings.call(BindingNames.PAWN_KILL, this.id);
    } else {
      console.log(`[FPawn] Killing ${this.username}`);
    }
  }
  
  /**
   * Revive a downed pawn
   */
  revive(): void {
    if (nativeBindings.has(BindingNames.PAWN_REVIVE)) {
      nativeBindings.call(BindingNames.PAWN_REVIVE, this.id);
    } else {
      console.log(`[FPawn] Reviving ${this.username}`);
    }
  }
  
  /**
   * Apply damage to the pawn
   * @param amount Damage amount
   * @param damageType Optional damage type
   */
  applyDamage(amount: number, damageType?: string): void {
    if (nativeBindings.has(BindingNames.PAWN_APPLY_DAMAGE)) {
      nativeBindings.call(BindingNames.PAWN_APPLY_DAMAGE, this.id, amount, damageType);
    }
  }
  
  /**
   * Set pawn health
   * @param health Health value (0-100)
   */
  setHealth(health: number): void {
    if (nativeBindings.has(BindingNames.PAWN_SET_HEALTH)) {
      nativeBindings.call(BindingNames.PAWN_SET_HEALTH, this.id, Math.max(0, Math.min(100, health)));
    }
  }
  
  /**
   * Set pawn shield
   * @param shield Shield value (0-100)
   */
  setShield(shield: number): void {
    if (nativeBindings.has(BindingNames.PAWN_SET_SHIELD)) {
      nativeBindings.call(BindingNames.PAWN_SET_SHIELD, this.id, Math.max(0, Math.min(100, shield)));
    }
  }
  
  /**
   * Get pawn stats
   */
  getStats(): PawnStats {
    if (nativeBindings.has(BindingNames.PAWN_GET_STATS)) {
      return nativeBindings.call(BindingNames.PAWN_GET_STATS, this.id) as PawnStats;
    }
    return {
      health: 100,
      maxHealth: 100,
      shield: 0,
      maxShield: 100,
      kills: 0,
      assists: 0,
      revives: 0,
      damageDealt: 0,
      damageTaken: 0
    };
  }
  
  /**
   * Get current pawn state
   */
  getState(): PawnState {
    if (nativeBindings.has(BindingNames.PAWN_GET_STATE)) {
      return nativeBindings.call(BindingNames.PAWN_GET_STATE, this.id) as PawnState;
    }
    return 'alive';
  }
  
  // ==================== Inventory ====================
  
  /**
   * Give an item to the pawn
   * @param itemId Item definition ID
   * @param quantity Item quantity
   * @param slot Optional slot index
   */
  giveItem(itemId: string, quantity: number = 1, slot?: number): void {
    if (nativeBindings.has(BindingNames.PAWN_GIVE_ITEM)) {
      nativeBindings.call(BindingNames.PAWN_GIVE_ITEM, this.id, itemId, quantity, slot);
    } else {
      console.log(`[FPawn] Giving ${this.username} ${quantity}x ${itemId}`);
    }
  }
  
  /**
   * Remove an item from the pawn's inventory
   * @param itemId Item definition ID
   * @param quantity Quantity to remove (0 = all)
   */
  removeItem(itemId: string, quantity: number = 0): void {
    if (nativeBindings.has(BindingNames.PAWN_REMOVE_ITEM)) {
      nativeBindings.call(BindingNames.PAWN_REMOVE_ITEM, this.id, itemId, quantity);
    }
  }
  
  /**
   * Get pawn's inventory
   */
  getInventory(): InventoryItem[] {
    if (nativeBindings.has(BindingNames.PAWN_GET_INVENTORY)) {
      return nativeBindings.call(BindingNames.PAWN_GET_INVENTORY, this.id) as InventoryItem[];
    }
    return [];
  }
  
  // ==================== Communication ====================
  
  /**
   * Play an emote on the pawn
   * @param emoteId Emote ID (e.g., "EID_Floss")
   */
  playEmote(emoteId: string): void {
    if (nativeBindings.has(BindingNames.PAWN_PLAY_EMOTE)) {
      nativeBindings.call(BindingNames.PAWN_PLAY_EMOTE, this.id, emoteId);
    }
  }
  
  /**
   * Send a message to the pawn's client
   * @param message Message text
   * @param type Message type (chat, system, etc.)
   */
  sendMessage(message: string, type: 'chat' | 'system' | 'whisper' = 'system'): void {
    if (nativeBindings.has(BindingNames.PAWN_SEND_MESSAGE)) {
      nativeBindings.call(BindingNames.PAWN_SEND_MESSAGE, this.id, message, type);
    }
  }
  
  /**
   * Check if pawn is alive
   */
  isAlive(): boolean {
    const state = this.getState();
    return state === 'alive';
  }
  
  /**
   * Check if pawn is downed (DBNO)
   */
  isDBNO(): boolean {
    const state = this.getState();
    return state === 'dbno';
  }
}
