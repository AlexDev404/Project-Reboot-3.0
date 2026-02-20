/**
 * FWorld - Static class for interacting with the game world
 * 
 * Provides access to world properties, pawn management, and game state.
 * This is the primary entry point for most SDK operations.
 * 
 * @example
 * ```ts
 * import { FWorld } from "@trail-blaze/retroflex";
 * 
 * // Get world state
 * const state = FWorld.getProperty("state");
 * // state: "disengaged" | "engaged" | "inProgress"
 * 
 * // Disable fall damage
 * FWorld.setProperty("damage", "false");
 * 
 * // Get all pawns and move them
 * FWorld.getPawnList().forEach((pawn) => {
 *   pawn.move(700, 700, 700);
 * });
 * 
 * // Get specific pawn by username
 * const myPawn = FWorld.getPawnByUsername("Array0x");
 * myPawn.costume("CID_016_Athena_Commando_F");
 * ```
 */

import { WorldProperties, WorldState, FVector } from './types';
import { FPawn } from './FPawn';
import { nativeBindings, BindingNames } from './bindings';

/**
 * Default world properties (used in mock mode)
 */
const defaultWorldProperties: WorldProperties = {
  state: 'disengaged',
  game_mode: 'athena',
  playlist: 'playlist_defaultsolo',
  damage: 'true',
  safe_zone_phase: 0,
  time_remaining: 0,
  player_count: 0,
  players_alive: 0
};

/**
 * In-memory mock state for development
 */
let mockProperties: WorldProperties = { ...defaultWorldProperties };
const mockPawns: Map<string, FPawn> = new Map();

/**
 * FWorld - World interaction interface
 */
export class FWorld {
  /**
   * Get world property/properties
   * @param property Optional property name. If not provided, returns all properties.
   * @returns The property value or all properties
   * 
   * @example
   * ```ts
   * // Get all properties
   * const props = FWorld.getProperty();
   * 
   * // Get specific property
   * const state = FWorld.getProperty("state");
   * ```
   */
  static getProperty(): WorldProperties;
  static getProperty<K extends keyof WorldProperties>(property: K): WorldProperties[K];
  static getProperty<K extends keyof WorldProperties>(property?: K): WorldProperties | WorldProperties[K] {
    if (nativeBindings.has(BindingNames.WORLD_GET_PROPERTY)) {
      const result = nativeBindings.call(BindingNames.WORLD_GET_PROPERTY, property);
      return result as WorldProperties | WorldProperties[K];
    }
    
    // Mock mode
    if (property === undefined) {
      return { ...mockProperties };
    }
    return mockProperties[property];
  }
  
  /**
   * Set a world property
   * @param property Property name
   * @param value Property value
   * 
   * @example
   * ```ts
   * // Disable fall damage
   * FWorld.setProperty("damage", "false");
   * ```
   */
  static setProperty<K extends keyof WorldProperties>(
    property: K, 
    value: WorldProperties[K]
  ): void {
    if (nativeBindings.has(BindingNames.WORLD_SET_PROPERTY)) {
      nativeBindings.call(BindingNames.WORLD_SET_PROPERTY, property, value);
    } else {
      // Mock mode
      (mockProperties as Record<string, unknown>)[property] = value;
      console.log(`[FWorld] Set property '${property}' to '${value}'`);
    }
  }
  
  /**
   * Get list of all pawns in the world
   * @returns Array of FPawn objects
   * 
   * @example
   * ```ts
   * FWorld.getPawnList().forEach((pawn) => {
   *   pawn.move(70, 70, 70);
   * });
   * ```
   */
  static getPawnList(): FPawn[] {
    if (nativeBindings.has(BindingNames.WORLD_GET_PAWN_LIST)) {
      const rawPawns = nativeBindings.call(BindingNames.WORLD_GET_PAWN_LIST) as Array<{
        id: string;
        username: string;
        accountId: string;
        teamId: number;
      }>;
      
      return rawPawns.map(p => new FPawn(p.id, p.username, p.accountId, p.teamId));
    }
    
    // Mock mode
    return Array.from(mockPawns.values());
  }
  
  /**
   * Get a pawn by its unique ID
   * @param id Pawn ID
   * @returns FPawn or undefined if not found
   */
  static getPawnById(id: string): FPawn | undefined {
    if (nativeBindings.has(BindingNames.WORLD_GET_PAWN_BY_ID)) {
      const raw = nativeBindings.call(BindingNames.WORLD_GET_PAWN_BY_ID, id) as {
        id: string;
        username: string;
        accountId: string;
        teamId: number;
      } | null;
      
      if (raw) {
        return new FPawn(raw.id, raw.username, raw.accountId, raw.teamId);
      }
      return undefined;
    }
    
    // Mock mode
    return mockPawns.get(id);
  }
  
  /**
   * Get a pawn by username
   * @param username Player username
   * @returns FPawn or undefined if not found
   * 
   * @example
   * ```ts
   * const myPawn = FWorld.getPawnByUsername("Array0x");
   * myPawn.move(600, 600, 600);
   * myPawn.costume("CID_016_Athena_Commando_F");
   * myPawn.kill();
   * ```
   */
  static getPawnByUsername(username: string): FPawn | undefined {
    if (nativeBindings.has(BindingNames.WORLD_GET_PAWN_BY_USERNAME)) {
      const raw = nativeBindings.call(BindingNames.WORLD_GET_PAWN_BY_USERNAME, username) as {
        id: string;
        username: string;
        accountId: string;
        teamId: number;
      } | null;
      
      if (raw) {
        return new FPawn(raw.id, raw.username, raw.accountId, raw.teamId);
      }
      return undefined;
    }
    
    // Mock mode
    for (const pawn of mockPawns.values()) {
      if (pawn.username.toLowerCase() === username.toLowerCase()) {
        return pawn;
      }
    }
    return undefined;
  }
  
  /**
   * Get current world state
   */
  static getState(): WorldState {
    return FWorld.getProperty('state');
  }
  
  /**
   * Check if game is in progress
   */
  static isGameInProgress(): boolean {
    return FWorld.getState() === 'inProgress';
  }
  
  /**
   * Get number of players alive
   */
  static getPlayersAlive(): number {
    return FWorld.getProperty('players_alive');
  }
  
  /**
   * Get current safe zone phase
   */
  static getSafeZonePhase(): number {
    return FWorld.getProperty('safe_zone_phase');
  }
  
  // ==================== Mock Mode Helpers ====================
  
  /**
   * Add a mock pawn (for development/testing)
   * @internal
   */
  static _addMockPawn(pawn: FPawn): void {
    mockPawns.set(pawn.id, pawn);
  }
  
  /**
   * Clear all mock pawns (for development/testing)
   * @internal
   */
  static _clearMockPawns(): void {
    mockPawns.clear();
  }
  
  /**
   * Reset mock properties to defaults (for development/testing)
   * @internal
   */
  static _resetMockProperties(): void {
    mockProperties = { ...defaultWorldProperties };
  }
}
