/**
 * FGame - Game state and match control
 * 
 * Provides methods to control the overall game state, match flow,
 * and server-wide settings.
 * 
 * @example
 * ```ts
 * import { FGame } from "@trail-blaze/retroflex";
 * 
 * // Start the match
 * FGame.startMatch();
 * 
 * // End the match with a winner
 * FGame.endMatch({ winnerId: "player-123", reason: "last_standing" });
 * 
 * // Set server tickrate
 * FGame.setTickRate(30);
 * ```
 */

import { FVector } from './types';
import { nativeBindings, BindingNames } from './bindings';

/**
 * Match end reason
 */
export type MatchEndReason = 
  | 'last_standing'
  | 'time_limit'
  | 'score_limit'
  | 'admin_ended'
  | 'error';

/**
 * Game phase
 */
export type GamePhase = 
  | 'none'
  | 'setup'
  | 'warmup'
  | 'bus'
  | 'aircraft'
  | 'safezones'
  | 'endgame'
  | 'count';

/**
 * Match end options
 */
export interface MatchEndOptions {
  winnerId?: string;
  winnerTeamId?: number;
  reason: MatchEndReason;
}

/**
 * Aircraft/Bus path configuration
 */
export interface AircraftPath {
  startLocation: FVector;
  endLocation: FVector;
  flightSpeed?: number;
  dropEnabled?: boolean;
}

/**
 * Loot pool entry
 */
export interface LootPoolEntry {
  itemId: string;
  weight: number;
  minCount?: number;
  maxCount?: number;
}

export class FGame {
  /**
   * Start the match
   * Transitions from warmup/lobby to active gameplay
   */
  static startMatch(): void {
    if (nativeBindings.has(BindingNames.GAME_START_MATCH)) {
      nativeBindings.call(BindingNames.GAME_START_MATCH);
    } else {
      console.log('[FGame] Starting match');
    }
  }
  
  /**
   * End the match
   * @param options Match end options
   */
  static endMatch(options: MatchEndOptions): void {
    if (nativeBindings.has(BindingNames.GAME_END_MATCH)) {
      nativeBindings.call(BindingNames.GAME_END_MATCH, options);
    } else {
      console.log('[FGame] Ending match:', options);
    }
  }
  
  /**
   * Get current game phase
   */
  static getPhase(): GamePhase {
    if (nativeBindings.has('FGame_GetPhase')) {
      return nativeBindings.call('FGame_GetPhase') as GamePhase;
    }
    return 'none';
  }
  
  /**
   * Set game phase
   * @param phase Target phase
   */
  static setPhase(phase: GamePhase): void {
    if (nativeBindings.has('FGame_SetPhase')) {
      nativeBindings.call('FGame_SetPhase', phase);
    }
  }
  
  /**
   * Get elapsed match time in seconds
   */
  static getMatchTime(): number {
    if (nativeBindings.has('FGame_GetMatchTime')) {
      return nativeBindings.call('FGame_GetMatchTime') as number;
    }
    return 0;
  }
  
  /**
   * Set server tick rate
   * @param tickRate Ticks per second (default: 30)
   */
  static setTickRate(tickRate: number): void {
    if (nativeBindings.has('FGame_SetTickRate')) {
      nativeBindings.call('FGame_SetTickRate', Math.max(1, Math.min(120, tickRate)));
    }
  }
  
  /**
   * Configure the battle bus/aircraft path
   * @param path Aircraft path configuration
   */
  static setAircraftPath(path: AircraftPath): void {
    if (nativeBindings.has('FGame_SetAircraftPath')) {
      nativeBindings.call('FGame_SetAircraftPath', path);
    }
  }
  
  /**
   * Force all players to drop from aircraft
   */
  static forceDropAllPlayers(): void {
    if (nativeBindings.has('FGame_ForceDropAll')) {
      nativeBindings.call('FGame_ForceDropAll');
    }
  }
  
  /**
   * Set the current playlist
   * @param playlistId Playlist identifier
   */
  static setPlaylist(playlistId: string): void {
    if (nativeBindings.has('FGame_SetPlaylist')) {
      nativeBindings.call('FGame_SetPlaylist', playlistId);
    }
  }
  
  /**
   * Get current playlist ID
   */
  static getPlaylist(): string {
    if (nativeBindings.has('FGame_GetPlaylist')) {
      return nativeBindings.call('FGame_GetPlaylist') as string;
    }
    return 'playlist_defaultsolo';
  }
  
  /**
   * Set maximum players allowed
   * @param count Max player count
   */
  static setMaxPlayers(count: number): void {
    if (nativeBindings.has('FGame_SetMaxPlayers')) {
      nativeBindings.call('FGame_SetMaxPlayers', count);
    }
  }
  
  /**
   * Enable or disable building
   * @param enabled Whether building is allowed
   */
  static setBuildingEnabled(enabled: boolean): void {
    if (nativeBindings.has('FGame_SetBuildingEnabled')) {
      nativeBindings.call('FGame_SetBuildingEnabled', enabled);
    }
  }
  
  /**
   * Enable or disable harvesting
   * @param enabled Whether harvesting is allowed
   */
  static setHarvestingEnabled(enabled: boolean): void {
    if (nativeBindings.has('FGame_SetHarvestingEnabled')) {
      nativeBindings.call('FGame_SetHarvestingEnabled', enabled);
    }
  }
  
  /**
   * Set respawn enabled
   * @param enabled Whether respawning is allowed
   * @param delay Respawn delay in seconds
   */
  static setRespawnEnabled(enabled: boolean, delay: number = 5): void {
    if (nativeBindings.has('FGame_SetRespawn')) {
      nativeBindings.call('FGame_SetRespawn', enabled, delay);
    }
  }
  
  /**
   * Pause the game
   * @param paused Whether to pause
   */
  static setPaused(paused: boolean): void {
    if (nativeBindings.has('FGame_SetPaused')) {
      nativeBindings.call('FGame_SetPaused', paused);
    }
  }
  
  /**
   * Check if game is paused
   */
  static isPaused(): boolean {
    if (nativeBindings.has('FGame_IsPaused')) {
      return nativeBindings.call('FGame_IsPaused') as boolean;
    }
    return false;
  }
  
  /**
   * Set gravity scale
   * @param scale Gravity multiplier (1.0 = normal)
   */
  static setGravityScale(scale: number): void {
    if (nativeBindings.has('FGame_SetGravityScale')) {
      nativeBindings.call('FGame_SetGravityScale', scale);
    }
  }
  
  /**
   * Set time of day
   * @param hour Hour (0-24)
   */
  static setTimeOfDay(hour: number): void {
    if (nativeBindings.has('FGame_SetTimeOfDay')) {
      nativeBindings.call('FGame_SetTimeOfDay', hour % 24);
    }
  }
  
  /**
   * Broadcast a message to all players
   * @param message Message text
   * @param duration Display duration in seconds
   */
  static broadcastMessage(message: string, duration: number = 5): void {
    if (nativeBindings.has('FGame_Broadcast')) {
      nativeBindings.call('FGame_Broadcast', message, duration);
    } else {
      console.log(`[FGame] Broadcast: ${message}`);
    }
  }
}
