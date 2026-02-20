/**
 * FCreative - Creative mode utilities
 * 
 * Provides methods for creative mode functionality including islands,
 * devices, and creative-specific features.
 * 
 * @example
 * ```ts
 * import { FCreative } from "@trail-blaze/retroflex";
 * 
 * // Teleport all players to an island
 * FCreative.teleportAllToIsland("main");
 * 
 * // Reset the current island
 * FCreative.resetIsland();
 * 
 * // Set infinite resources
 * FCreative.setInfiniteResources(true);
 * ```
 */

import { FVector, FRotator } from './types';
import { nativeBindings } from './bindings';

/**
 * Creative island info
 */
export interface IslandInfo {
  id: string;
  name: string;
  ownerId?: string;
  playerCount: number;
  maxPlayers: number;
  isPublished: boolean;
}

/**
 * Creative device info
 */
export interface DeviceInfo {
  id: string;
  type: string;
  location: FVector;
  rotation: FRotator;
  isEnabled: boolean;
  settings: Record<string, unknown>;
}

export class FCreative {
  /**
   * Get current island info
   */
  static getCurrentIsland(): IslandInfo | undefined {
    if (nativeBindings.has('FCreative_GetCurrentIsland')) {
      return nativeBindings.call('FCreative_GetCurrentIsland') as IslandInfo;
    }
    return undefined;
  }
  
  /**
   * Get all available islands
   */
  static getAllIslands(): IslandInfo[] {
    if (nativeBindings.has('FCreative_GetAllIslands')) {
      return nativeBindings.call('FCreative_GetAllIslands') as IslandInfo[];
    }
    return [];
  }
  
  /**
   * Teleport all players to an island
   * @param islandId Island ID
   */
  static teleportAllToIsland(islandId: string): void {
    if (nativeBindings.has('FCreative_TeleportAll')) {
      nativeBindings.call('FCreative_TeleportAll', islandId);
    }
  }
  
  /**
   * Teleport a player to an island
   * @param playerId Player ID
   * @param islandId Island ID
   */
  static teleportPlayerToIsland(playerId: string, islandId: string): void {
    if (nativeBindings.has('FCreative_TeleportPlayer')) {
      nativeBindings.call('FCreative_TeleportPlayer', playerId, islandId);
    }
  }
  
  /**
   * Reset the current island to its initial state
   */
  static resetIsland(): void {
    if (nativeBindings.has('FCreative_ResetIsland')) {
      nativeBindings.call('FCreative_ResetIsland');
    }
  }
  
  /**
   * Set infinite resources for all players
   * @param enabled Whether infinite resources are enabled
   */
  static setInfiniteResources(enabled: boolean): void {
    if (nativeBindings.has('FCreative_SetInfiniteResources')) {
      nativeBindings.call('FCreative_SetInfiniteResources', enabled);
    }
  }
  
  /**
   * Set infinite ammo for all players
   * @param enabled Whether infinite ammo is enabled
   */
  static setInfiniteAmmo(enabled: boolean): void {
    if (nativeBindings.has('FCreative_SetInfiniteAmmo')) {
      nativeBindings.call('FCreative_SetInfiniteAmmo', enabled);
    }
  }
  
  /**
   * Enable or disable flight for all players
   * @param enabled Whether flight is enabled
   */
  static setFlightEnabled(enabled: boolean): void {
    if (nativeBindings.has('FCreative_SetFlight')) {
      nativeBindings.call('FCreative_SetFlight', enabled);
    }
  }
  
  /**
   * Get all devices on the island
   */
  static getDevices(): DeviceInfo[] {
    if (nativeBindings.has('FCreative_GetDevices')) {
      return nativeBindings.call('FCreative_GetDevices') as DeviceInfo[];
    }
    return [];
  }
  
  /**
   * Get device by ID
   * @param deviceId Device ID
   */
  static getDevice(deviceId: string): DeviceInfo | undefined {
    if (nativeBindings.has('FCreative_GetDevice')) {
      return nativeBindings.call('FCreative_GetDevice', deviceId) as DeviceInfo;
    }
    return undefined;
  }
  
  /**
   * Enable or disable a device
   * @param deviceId Device ID
   * @param enabled Whether device is enabled
   */
  static setDeviceEnabled(deviceId: string, enabled: boolean): void {
    if (nativeBindings.has('FCreative_SetDeviceEnabled')) {
      nativeBindings.call('FCreative_SetDeviceEnabled', deviceId, enabled);
    }
  }
  
  /**
   * Trigger a device
   * @param deviceId Device ID
   * @param triggeredBy Player ID that triggered it (optional)
   */
  static triggerDevice(deviceId: string, triggeredBy?: string): void {
    if (nativeBindings.has('FCreative_TriggerDevice')) {
      nativeBindings.call('FCreative_TriggerDevice', deviceId, triggeredBy);
    }
  }
  
  /**
   * Set device setting
   * @param deviceId Device ID
   * @param setting Setting name
   * @param value Setting value
   */
  static setDeviceSetting(deviceId: string, setting: string, value: unknown): void {
    if (nativeBindings.has('FCreative_SetDeviceSetting')) {
      nativeBindings.call('FCreative_SetDeviceSetting', deviceId, setting, value);
    }
  }
  
  /**
   * Start the island game
   */
  static startGame(): void {
    if (nativeBindings.has('FCreative_StartGame')) {
      nativeBindings.call('FCreative_StartGame');
    }
  }
  
  /**
   * End the island game
   */
  static endGame(): void {
    if (nativeBindings.has('FCreative_EndGame')) {
      nativeBindings.call('FCreative_EndGame');
    }
  }
  
  /**
   * Set game time limit
   * @param seconds Time limit in seconds (0 = no limit)
   */
  static setTimeLimit(seconds: number): void {
    if (nativeBindings.has('FCreative_SetTimeLimit')) {
      nativeBindings.call('FCreative_SetTimeLimit', seconds);
    }
  }
  
  /**
   * Set player spawn point
   * @param location Spawn location
   * @param rotation Spawn rotation
   */
  static setSpawnPoint(location: FVector, rotation?: FRotator): void {
    if (nativeBindings.has('FCreative_SetSpawnPoint')) {
      nativeBindings.call('FCreative_SetSpawnPoint', location, rotation);
    }
  }
}
