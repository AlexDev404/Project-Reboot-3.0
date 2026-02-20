/**
 * FServer - Server configuration and management
 * 
 * Provides methods for configuring the game server, network settings,
 * and server-wide options.
 * 
 * @example
 * ```ts
 * import { FServer } from "@trail-blaze/retroflex";
 * 
 * // Set server name
 * FServer.setServerName("My Custom Server");
 * 
 * // Configure network settings
 * FServer.setNetTickRate(60);
 * 
 * // Get server info
 * const info = FServer.getInfo();
 * ```
 */

import { nativeBindings } from './bindings';

/**
 * Net mode types
 */
export type NetMode = 
  | 'standalone'
  | 'dedicated_server'
  | 'listen_server'
  | 'client';

/**
 * Server information
 */
export interface ServerInfo {
  name: string;
  version: string;
  netMode: NetMode;
  playerCount: number;
  maxPlayers: number;
  uptime: number;
  tickRate: number;
  region: string;
  buildVersion: string;
}

/**
 * Network statistics
 */
export interface NetworkStats {
  inBytes: number;
  outBytes: number;
  inPackets: number;
  outPackets: number;
  inPacketsLost: number;
  outPacketsLost: number;
  avgPing: number;
  connectionCount: number;
}

/**
 * Server configuration
 */
export interface ServerConfig {
  serverName?: string;
  maxPlayers?: number;
  tickRate?: number;
  enableMCP?: boolean;
  enableAnalytics?: boolean;
  privateIPsAsOperators?: boolean;
  autoSaveInterval?: number;
}

export class FServer {
  /**
   * Get server information
   */
  static getInfo(): ServerInfo {
    if (nativeBindings.has('FServer_GetInfo')) {
      return nativeBindings.call('FServer_GetInfo') as ServerInfo;
    }
    return {
      name: 'Project Reboot',
      version: '3.0',
      netMode: 'dedicated_server',
      playerCount: 0,
      maxPlayers: 100,
      uptime: 0,
      tickRate: 30,
      region: 'unknown',
      buildVersion: 'unknown'
    };
  }
  
  /**
   * Set server name
   * @param name Server name
   */
  static setServerName(name: string): void {
    if (nativeBindings.has('FServer_SetName')) {
      nativeBindings.call('FServer_SetName', name);
    }
  }
  
  /**
   * Get network statistics
   */
  static getNetworkStats(): NetworkStats {
    if (nativeBindings.has('FServer_GetNetworkStats')) {
      return nativeBindings.call('FServer_GetNetworkStats') as NetworkStats;
    }
    return {
      inBytes: 0,
      outBytes: 0,
      inPackets: 0,
      outPackets: 0,
      inPacketsLost: 0,
      outPacketsLost: 0,
      avgPing: 0,
      connectionCount: 0
    };
  }
  
  /**
   * Set network tick rate
   * @param tickRate Ticks per second
   */
  static setNetTickRate(tickRate: number): void {
    if (nativeBindings.has('FServer_SetNetTickRate')) {
      nativeBindings.call('FServer_SetNetTickRate', Math.max(1, Math.min(120, tickRate)));
    }
  }
  
  /**
   * Set maximum tick rate
   * @param tickRate Maximum tick rate
   */
  static setMaxTickRate(tickRate: number): void {
    if (nativeBindings.has('FServer_SetMaxTickRate')) {
      nativeBindings.call('FServer_SetMaxTickRate', tickRate);
    }
  }
  
  /**
   * Enable or disable MCP (Master Control Program/Backend)
   * @param enabled Whether MCP is enabled
   */
  static setMCPEnabled(enabled: boolean): void {
    if (nativeBindings.has('FServer_SetMCPEnabled')) {
      nativeBindings.call('FServer_SetMCPEnabled', enabled);
    }
  }
  
  /**
   * Apply server configuration
   * @param config Configuration options
   */
  static configure(config: ServerConfig): void {
    if (nativeBindings.has('FServer_Configure')) {
      nativeBindings.call('FServer_Configure', config);
    }
  }
  
  /**
   * Get current Fortnite version
   */
  static getFortniteVersion(): number {
    if (nativeBindings.has('FServer_GetFortniteVersion')) {
      return nativeBindings.call('FServer_GetFortniteVersion') as number;
    }
    return 0;
  }
  
  /**
   * Get engine version
   */
  static getEngineVersion(): number {
    if (nativeBindings.has('FServer_GetEngineVersion')) {
      return nativeBindings.call('FServer_GetEngineVersion') as number;
    }
    return 0;
  }
  
  /**
   * Get server uptime in seconds
   */
  static getUptime(): number {
    if (nativeBindings.has('FServer_GetUptime')) {
      return nativeBindings.call('FServer_GetUptime') as number;
    }
    return 0;
  }
  
  /**
   * Restart the match/server
   */
  static restart(): void {
    if (nativeBindings.has('FServer_Restart')) {
      nativeBindings.call('FServer_Restart');
    }
  }
  
  /**
   * Shutdown the server
   * @param reason Shutdown reason
   * @param delay Delay in seconds before shutdown
   */
  static shutdown(reason: string = 'Server shutdown', delay: number = 0): void {
    if (nativeBindings.has('FServer_Shutdown')) {
      nativeBindings.call('FServer_Shutdown', reason, delay);
    }
  }
  
  /**
   * Force garbage collection
   */
  static collectGarbage(): void {
    if (nativeBindings.has('FServer_CollectGarbage')) {
      nativeBindings.call('FServer_CollectGarbage');
    }
  }
  
  /**
   * Check if server is in standalone mode
   */
  static isStandalone(): boolean {
    return FServer.getInfo().netMode === 'standalone';
  }
  
  /**
   * Check if server is dedicated
   */
  static isDedicatedServer(): boolean {
    return FServer.getInfo().netMode === 'dedicated_server';
  }
  
  /**
   * Check if server is a listen server
   */
  static isListenServer(): boolean {
    return FServer.getInfo().netMode === 'listen_server';
  }
  
  /**
   * Log a message to the server console
   * @param message Message to log
   * @param level Log level
   */
  static log(message: string, level: 'info' | 'warn' | 'error' = 'info'): void {
    if (nativeBindings.has('FServer_Log')) {
      nativeBindings.call('FServer_Log', message, level);
    } else {
      console.log(`[FServer] ${message}`);
    }
  }
}
