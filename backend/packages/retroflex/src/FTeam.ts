/**
 * FTeam - Team management
 * 
 * Provides methods for managing teams, squads, and team-related settings.
 * 
 * @example
 * ```ts
 * import { FTeam } from "@trail-blaze/retroflex";
 * 
 * // Get all teams
 * const teams = FTeam.getAll();
 * 
 * // Move a player to a different team
 * FTeam.setPlayerTeam("player-123", 5);
 * 
 * // Get team members
 * const members = FTeam.getMembers(1);
 * ```
 */

import { TeamInfo } from './types';
import { nativeBindings } from './bindings';

/**
 * Team configuration
 */
export interface TeamConfig {
  /** Maximum team size */
  maxSize: number;
  /** Whether friendly fire is enabled */
  friendlyFire: boolean;
  /** Team fill enabled */
  fillEnabled: boolean;
}

/**
 * Extended team info with members
 */
export interface TeamDetails extends TeamInfo {
  /** Player IDs of team members */
  memberIds: string[];
  /** Is team eliminated */
  isEliminated: boolean;
  /** Team placement/rank */
  placement?: number;
}

export class FTeam {
  /**
   * Get all teams
   */
  static getAll(): TeamDetails[] {
    if (nativeBindings.has('FTeam_GetAll')) {
      return nativeBindings.call('FTeam_GetAll') as TeamDetails[];
    }
    return [];
  }
  
  /**
   * Get team info by ID
   * @param teamId Team ID
   */
  static getInfo(teamId: number): TeamDetails | undefined {
    if (nativeBindings.has('FTeam_GetInfo')) {
      return nativeBindings.call('FTeam_GetInfo', teamId) as TeamDetails;
    }
    return undefined;
  }
  
  /**
   * Get all members of a team
   * @param teamId Team ID
   * @returns Array of player IDs
   */
  static getMembers(teamId: number): string[] {
    if (nativeBindings.has('FTeam_GetMembers')) {
      return nativeBindings.call('FTeam_GetMembers', teamId) as string[];
    }
    return [];
  }
  
  /**
   * Get a player's team ID
   * @param playerId Player ID
   */
  static getPlayerTeam(playerId: string): number | undefined {
    if (nativeBindings.has('FTeam_GetPlayerTeam')) {
      return nativeBindings.call('FTeam_GetPlayerTeam', playerId) as number;
    }
    return undefined;
  }
  
  /**
   * Set a player's team
   * @param playerId Player ID
   * @param teamId Target team ID
   */
  static setPlayerTeam(playerId: string, teamId: number): boolean {
    if (nativeBindings.has('FTeam_SetPlayerTeam')) {
      return nativeBindings.call('FTeam_SetPlayerTeam', playerId, teamId) as boolean;
    }
    return false;
  }
  
  /**
   * Create a new team
   * @returns New team ID
   */
  static createTeam(): number {
    if (nativeBindings.has('FTeam_Create')) {
      return nativeBindings.call('FTeam_Create') as number;
    }
    return -1;
  }
  
  /**
   * Eliminate a team
   * @param teamId Team ID
   */
  static eliminateTeam(teamId: number): void {
    if (nativeBindings.has('FTeam_Eliminate')) {
      nativeBindings.call('FTeam_Eliminate', teamId);
    }
  }
  
  /**
   * Check if two players are on the same team
   * @param playerId1 First player ID
   * @param playerId2 Second player ID
   */
  static areTeammates(playerId1: string, playerId2: string): boolean {
    if (nativeBindings.has('FTeam_AreTeammates')) {
      return nativeBindings.call('FTeam_AreTeammates', playerId1, playerId2) as boolean;
    }
    return false;
  }
  
  /**
   * Get number of teams remaining
   */
  static getTeamsRemaining(): number {
    if (nativeBindings.has('FTeam_GetRemaining')) {
      return nativeBindings.call('FTeam_GetRemaining') as number;
    }
    return 0;
  }
  
  /**
   * Set team configuration
   * @param config Team configuration
   */
  static setConfig(config: Partial<TeamConfig>): void {
    if (nativeBindings.has('FTeam_SetConfig')) {
      nativeBindings.call('FTeam_SetConfig', config);
    }
  }
  
  /**
   * Enable or disable friendly fire
   * @param enabled Whether friendly fire is enabled
   */
  static setFriendlyFire(enabled: boolean): void {
    if (nativeBindings.has('FTeam_SetFriendlyFire')) {
      nativeBindings.call('FTeam_SetFriendlyFire', enabled);
    }
  }
  
  /**
   * Set maximum team size
   * @param size Max team size
   */
  static setMaxTeamSize(size: number): void {
    if (nativeBindings.has('FTeam_SetMaxSize')) {
      nativeBindings.call('FTeam_SetMaxSize', size);
    }
  }
  
  /**
   * Scramble teams (randomize team assignments)
   */
  static scrambleTeams(): void {
    if (nativeBindings.has('FTeam_Scramble')) {
      nativeBindings.call('FTeam_Scramble');
    }
  }
  
  /**
   * Balance teams (even out team sizes)
   */
  static balanceTeams(): void {
    if (nativeBindings.has('FTeam_Balance')) {
      nativeBindings.call('FTeam_Balance');
    }
  }
}
