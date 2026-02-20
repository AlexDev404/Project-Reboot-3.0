/**
 * FAdmin - Server administration and moderation
 * 
 * Provides methods for server administration including:
 * - Kicking and banning players
 * - Managing operator privileges
 * - Server announcements
 * - Player management
 * 
 * Based on Project Reboot's commands.h, moderation.h functionality.
 * 
 * @example
 * ```ts
 * import { FAdmin } from "@trail-blaze/retroflex";
 * 
 * // Kick a player
 * FAdmin.kick("player-123", "AFK");
 * 
 * // Ban a player
 * FAdmin.ban("player-123", "Cheating");
 * 
 * // Check if player is operator
 * if (FAdmin.isOperator("player-123")) {
 *   // Allow admin commands
 * }
 * ```
 */

import { nativeBindings } from './bindings';

/**
 * Ban record
 */
export interface BanRecord {
  playerId: string;
  username: string;
  ipAddress: string;
  reason: string;
  bannedAt: Date;
  bannedBy?: string;
  expiresAt?: Date;
}

/**
 * Player info for admin purposes
 */
export interface AdminPlayerInfo {
  id: string;
  username: string;
  ipAddress: string;
  isOperator: boolean;
  teamId: number;
  ping: number;
  connectionTime: Date;
}

export class FAdmin {
  /**
   * Kick a player from the server
   * @param playerId Player ID to kick
   * @param reason Kick reason (shown to player)
   */
  static kick(playerId: string, reason: string = 'Kicked by administrator'): boolean {
    if (nativeBindings.has('FAdmin_Kick')) {
      return nativeBindings.call('FAdmin_Kick', playerId, reason) as boolean;
    }
    console.log(`[FAdmin] Kicking ${playerId}: ${reason}`);
    return false;
  }
  
  /**
   * Ban a player
   * @param playerId Player ID to ban
   * @param reason Ban reason
   * @param duration Ban duration in seconds (0 = permanent)
   */
  static ban(playerId: string, reason: string = 'Banned', duration: number = 0): boolean {
    if (nativeBindings.has('FAdmin_Ban')) {
      return nativeBindings.call('FAdmin_Ban', playerId, reason, duration) as boolean;
    }
    console.log(`[FAdmin] Banning ${playerId}: ${reason}`);
    return false;
  }
  
  /**
   * Ban an IP address directly
   * @param ipAddress IP address to ban
   * @param reason Ban reason
   * @param duration Duration in seconds (0 = permanent)
   */
  static banIP(ipAddress: string, reason: string, duration: number = 0): boolean {
    if (nativeBindings.has('FAdmin_BanIP')) {
      return nativeBindings.call('FAdmin_BanIP', ipAddress, reason, duration) as boolean;
    }
    return false;
  }
  
  /**
   * Unban a player
   * @param playerId Player ID or IP address
   */
  static unban(playerId: string): boolean {
    if (nativeBindings.has('FAdmin_Unban')) {
      return nativeBindings.call('FAdmin_Unban', playerId) as boolean;
    }
    return false;
  }
  
  /**
   * Check if a player is banned
   * @param playerId Player ID or IP address
   */
  static isBanned(playerId: string): boolean {
    if (nativeBindings.has('FAdmin_IsBanned')) {
      return nativeBindings.call('FAdmin_IsBanned', playerId) as boolean;
    }
    return false;
  }
  
  /**
   * Get all ban records
   */
  static getBanList(): BanRecord[] {
    if (nativeBindings.has('FAdmin_GetBanList')) {
      return nativeBindings.call('FAdmin_GetBanList') as BanRecord[];
    }
    return [];
  }
  
  /**
   * Check if a player is an operator (admin)
   * @param playerId Player ID
   */
  static isOperator(playerId: string): boolean {
    if (nativeBindings.has('FAdmin_IsOperator')) {
      return nativeBindings.call('FAdmin_IsOperator', playerId) as boolean;
    }
    return false;
  }
  
  /**
   * Grant operator status to a player
   * @param playerId Player ID
   */
  static setOperator(playerId: string, isOp: boolean = true): boolean {
    if (nativeBindings.has('FAdmin_SetOperator')) {
      return nativeBindings.call('FAdmin_SetOperator', playerId, isOp) as boolean;
    }
    return false;
  }
  
  /**
   * Send a console message to a player
   * @param playerId Player ID
   * @param message Message to send
   */
  static sendConsoleMessage(playerId: string, message: string): void {
    if (nativeBindings.has('FAdmin_SendConsoleMessage')) {
      nativeBindings.call('FAdmin_SendConsoleMessage', playerId, message);
    }
  }
  
  /**
   * Broadcast a message to all players
   * @param message Message to broadcast
   */
  static broadcast(message: string): void {
    if (nativeBindings.has('FAdmin_Broadcast')) {
      nativeBindings.call('FAdmin_Broadcast', message);
    } else {
      console.log(`[FAdmin] Broadcast: ${message}`);
    }
  }
  
  /**
   * Get all connected players
   */
  static getPlayers(): AdminPlayerInfo[] {
    if (nativeBindings.has('FAdmin_GetPlayers')) {
      return nativeBindings.call('FAdmin_GetPlayers') as AdminPlayerInfo[];
    }
    return [];
  }
  
  /**
   * Get player info
   * @param playerId Player ID
   */
  static getPlayerInfo(playerId: string): AdminPlayerInfo | undefined {
    if (nativeBindings.has('FAdmin_GetPlayerInfo')) {
      return nativeBindings.call('FAdmin_GetPlayerInfo', playerId) as AdminPlayerInfo;
    }
    return undefined;
  }
  
  /**
   * Execute a server command
   * @param command Command string
   * @param executorId Player ID executing the command (for permission checks)
   */
  static executeCommand(command: string, executorId?: string): string {
    if (nativeBindings.has('FAdmin_ExecuteCommand')) {
      return nativeBindings.call('FAdmin_ExecuteCommand', command, executorId) as string;
    }
    return '';
  }
  
  /**
   * Check if an IP is private (LAN)
   * @param ipAddress IP address to check
   */
  static isPrivateIP(ipAddress: string): boolean {
    return ipAddress.startsWith('192.168.') || 
           ipAddress.startsWith('10.') || 
           ipAddress.startsWith('172.16.') ||
           ipAddress === '127.0.0.1';
  }
  
  /**
   * Set whether private IPs are treated as operators
   * @param enabled Whether to enable this feature
   */
  static setPrivateIPsAsOperators(enabled: boolean): void {
    if (nativeBindings.has('FAdmin_SetPrivateIPsAsOps')) {
      nativeBindings.call('FAdmin_SetPrivateIPsAsOps', enabled);
    }
  }
}
