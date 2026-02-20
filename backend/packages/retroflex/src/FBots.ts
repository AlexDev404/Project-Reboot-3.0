/**
 * FBots - AI Bot management
 * 
 * Provides methods for spawning and controlling AI bots in the game.
 * Based on Project Reboot's bots.h and FortServerBotManagerAthena functionality.
 * 
 * @example
 * ```ts
 * import { FBots } from "@trail-blaze/retroflex";
 * 
 * // Spawn a bot
 * const bot = FBots.spawn({ x: 100, y: 200, z: 300 });
 * 
 * // Configure bot behavior
 * FBots.setBehavior(bot.id, "aggressive");
 * 
 * // Remove all bots
 * FBots.removeAll();
 * ```
 */

import { FVector, FRotator, CosmeticLoadout } from './types';
import { nativeBindings } from './bindings';

/**
 * Bot behavior types
 */
export type BotBehavior = 
  | 'passive'      // Won't attack unless attacked
  | 'defensive'    // Defends position, attacks threats
  | 'aggressive'   // Actively seeks and attacks players
  | 'builder'      // Focuses on building
  | 'looter'       // Prioritizes looting
  | 'camper';      // Stays in position

/**
 * Bot difficulty levels
 */
export type BotDifficulty = 'easy' | 'medium' | 'hard' | 'expert';

/**
 * Bot spawn options
 */
export interface BotSpawnOptions {
  /** Spawn location */
  location: FVector;
  /** Spawn rotation */
  rotation?: FRotator;
  /** Bot display name */
  name?: string;
  /** Team ID to assign */
  teamId?: number;
  /** Cosmetic loadout */
  cosmetics?: CosmeticLoadout;
  /** Initial behavior */
  behavior?: BotBehavior;
  /** Difficulty level */
  difficulty?: BotDifficulty;
  /** Whether to give starting inventory */
  giveInventory?: boolean;
}

/**
 * Bot information
 */
export interface BotInfo {
  id: string;
  name: string;
  pawnId: string;
  teamId: number;
  behavior: BotBehavior;
  difficulty: BotDifficulty;
  location: FVector;
  health: number;
  shield: number;
  kills: number;
  isAlive: boolean;
}

/**
 * Bot POI (Point of Interest) for navigation
 */
export interface BotPOI {
  name: string;
  center: FVector;
  radius: number;
}

export class FBots {
  /**
   * Spawn a bot
   * @param options Spawn options
   * @returns Bot info or undefined on failure
   */
  static spawn(options: BotSpawnOptions): BotInfo | undefined {
    if (nativeBindings.has('FBots_Spawn')) {
      return nativeBindings.call('FBots_Spawn', options) as BotInfo;
    }
    console.log(`[FBots] Spawning bot at (${options.location.x}, ${options.location.y}, ${options.location.z})`);
    return undefined;
  }
  
  /**
   * Spawn multiple bots
   * @param count Number of bots to spawn
   * @param options Base spawn options (location will be randomized)
   * @param spreadRadius Radius to spread bots around the location
   */
  static spawnMultiple(
    count: number,
    options: BotSpawnOptions,
    spreadRadius: number = 500
  ): BotInfo[] {
    if (nativeBindings.has('FBots_SpawnMultiple')) {
      return nativeBindings.call('FBots_SpawnMultiple', count, options, spreadRadius) as BotInfo[];
    }
    return [];
  }
  
  /**
   * Remove a bot
   * @param botId Bot ID
   */
  static remove(botId: string): boolean {
    if (nativeBindings.has('FBots_Remove')) {
      return nativeBindings.call('FBots_Remove', botId) as boolean;
    }
    return false;
  }
  
  /**
   * Remove all bots
   * @returns Number of bots removed
   */
  static removeAll(): number {
    if (nativeBindings.has('FBots_RemoveAll')) {
      return nativeBindings.call('FBots_RemoveAll') as number;
    }
    return 0;
  }
  
  /**
   * Get all bots
   */
  static getAll(): BotInfo[] {
    if (nativeBindings.has('FBots_GetAll')) {
      return nativeBindings.call('FBots_GetAll') as BotInfo[];
    }
    return [];
  }
  
  /**
   * Get bot info by ID
   * @param botId Bot ID
   */
  static getInfo(botId: string): BotInfo | undefined {
    if (nativeBindings.has('FBots_GetInfo')) {
      return nativeBindings.call('FBots_GetInfo', botId) as BotInfo;
    }
    return undefined;
  }
  
  /**
   * Set bot behavior
   * @param botId Bot ID
   * @param behavior Target behavior
   */
  static setBehavior(botId: string, behavior: BotBehavior): void {
    if (nativeBindings.has('FBots_SetBehavior')) {
      nativeBindings.call('FBots_SetBehavior', botId, behavior);
    }
  }
  
  /**
   * Set bot difficulty
   * @param botId Bot ID
   * @param difficulty Difficulty level
   */
  static setDifficulty(botId: string, difficulty: BotDifficulty): void {
    if (nativeBindings.has('FBots_SetDifficulty')) {
      nativeBindings.call('FBots_SetDifficulty', botId, difficulty);
    }
  }
  
  /**
   * Command a bot to move to a location
   * @param botId Bot ID
   * @param location Target location
   */
  static moveTo(botId: string, location: FVector): void {
    if (nativeBindings.has('FBots_MoveTo')) {
      nativeBindings.call('FBots_MoveTo', botId, location);
    }
  }
  
  /**
   * Command a bot to attack a target
   * @param botId Bot ID
   * @param targetId Target player/bot ID
   */
  static attackTarget(botId: string, targetId: string): void {
    if (nativeBindings.has('FBots_AttackTarget')) {
      nativeBindings.call('FBots_AttackTarget', botId, targetId);
    }
  }
  
  /**
   * Command a bot to loot nearby
   * @param botId Bot ID
   */
  static commandLoot(botId: string): void {
    if (nativeBindings.has('FBots_CommandLoot')) {
      nativeBindings.call('FBots_CommandLoot', botId);
    }
  }
  
  /**
   * Set bot's cosmetic loadout
   * @param botId Bot ID
   * @param loadout Cosmetic loadout
   */
  static setCosmetics(botId: string, loadout: CosmeticLoadout): void {
    if (nativeBindings.has('FBots_SetCosmetics')) {
      nativeBindings.call('FBots_SetCosmetics', botId, loadout);
    }
  }
  
  /**
   * Get number of alive bots
   */
  static getAliveCount(): number {
    if (nativeBindings.has('FBots_GetAliveCount')) {
      return nativeBindings.call('FBots_GetAliveCount') as number;
    }
    return 0;
  }
  
  /**
   * Initialize bot classes (must be called before spawning)
   */
  static initialize(): boolean {
    if (nativeBindings.has('FBots_Initialize')) {
      return nativeBindings.call('FBots_Initialize') as boolean;
    }
    return false;
  }
  
  /**
   * Set global bot count limit
   * @param maxBots Maximum number of bots allowed
   */
  static setMaxBots(maxBots: number): void {
    if (nativeBindings.has('FBots_SetMaxBots')) {
      nativeBindings.call('FBots_SetMaxBots', maxBots);
    }
  }
  
  /**
   * Fill empty player slots with bots
   * @param targetPlayerCount Target total player count
   */
  static fillLobby(targetPlayerCount: number): number {
    if (nativeBindings.has('FBots_FillLobby')) {
      return nativeBindings.call('FBots_FillLobby', targetPlayerCount) as number;
    }
    return 0;
  }
}
