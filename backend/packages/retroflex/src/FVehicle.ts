/**
 * FVehicle - Vehicle spawning and control
 * 
 * Provides methods for spawning, managing, and controlling vehicles.
 * 
 * @example
 * ```ts
 * import { FVehicle } from "@trail-blaze/retroflex";
 * 
 * // Spawn a vehicle
 * const car = FVehicle.spawn("vehicle_car", { x: 100, y: 200, z: 300 });
 * 
 * // Get all vehicles
 * const vehicles = FVehicle.getAll();
 * 
 * // Destroy all vehicles
 * FVehicle.destroyAll();
 * ```
 */

import { FVector, FRotator } from './types';
import { nativeBindings, BindingNames } from './bindings';

/**
 * Vehicle types
 */
export type VehicleType = 
  | 'vehicle_car'
  | 'vehicle_atk'
  | 'vehicle_quadcrasher'
  | 'vehicle_plane'
  | 'vehicle_boat'
  | 'vehicle_helicopter'
  | 'vehicle_ufo'
  | 'vehicle_tank'
  | 'vehicle_motorcycle'
  | 'vehicle_shopping_cart'
  | 'vehicle_cannon'
  | 'vehicle_driftboard';

/**
 * Vehicle information
 */
export interface VehicleInfo {
  id: string;
  type: VehicleType;
  location: FVector;
  rotation: FRotator;
  health: number;
  maxHealth: number;
  fuel: number;
  maxFuel: number;
  driverId?: string;
  passengerIds: string[];
  velocity: FVector;
}

/**
 * Vehicle spawn options
 */
export interface VehicleSpawnOptions {
  rotation?: FRotator;
  health?: number;
  fuel?: number;
}

export class FVehicle {
  /**
   * Spawn a vehicle
   * @param type Vehicle type
   * @param location Spawn location
   * @param options Spawn options
   * @returns Vehicle info or undefined on failure
   */
  static spawn(
    type: VehicleType,
    location: FVector,
    options: VehicleSpawnOptions = {}
  ): VehicleInfo | undefined {
    if (nativeBindings.has(BindingNames.GAME_SPAWN_VEHICLE)) {
      return nativeBindings.call(BindingNames.GAME_SPAWN_VEHICLE, type, location, options) as VehicleInfo;
    }
    console.log(`[FVehicle] Spawning ${type} at (${location.x}, ${location.y}, ${location.z})`);
    return undefined;
  }
  
  /**
   * Get vehicle info by ID
   * @param vehicleId Vehicle ID
   */
  static getInfo(vehicleId: string): VehicleInfo | undefined {
    if (nativeBindings.has('FVehicle_GetInfo')) {
      return nativeBindings.call('FVehicle_GetInfo', vehicleId) as VehicleInfo;
    }
    return undefined;
  }
  
  /**
   * Get all vehicles in the world
   */
  static getAll(): VehicleInfo[] {
    if (nativeBindings.has('FVehicle_GetAll')) {
      return nativeBindings.call('FVehicle_GetAll') as VehicleInfo[];
    }
    return [];
  }
  
  /**
   * Get vehicles in a radius
   * @param center Center position
   * @param radius Search radius
   */
  static getInRadius(center: FVector, radius: number): VehicleInfo[] {
    if (nativeBindings.has('FVehicle_GetInRadius')) {
      return nativeBindings.call('FVehicle_GetInRadius', center, radius) as VehicleInfo[];
    }
    return [];
  }
  
  /**
   * Destroy a vehicle
   * @param vehicleId Vehicle ID
   */
  static destroy(vehicleId: string): boolean {
    if (nativeBindings.has('FVehicle_Destroy')) {
      return nativeBindings.call('FVehicle_Destroy', vehicleId) as boolean;
    }
    return false;
  }
  
  /**
   * Destroy all vehicles
   * @returns Number of vehicles destroyed
   */
  static destroyAll(): number {
    if (nativeBindings.has('FVehicle_DestroyAll')) {
      return nativeBindings.call('FVehicle_DestroyAll') as number;
    }
    return 0;
  }
  
  /**
   * Set vehicle health
   * @param vehicleId Vehicle ID
   * @param health New health value
   */
  static setHealth(vehicleId: string, health: number): void {
    if (nativeBindings.has('FVehicle_SetHealth')) {
      nativeBindings.call('FVehicle_SetHealth', vehicleId, health);
    }
  }
  
  /**
   * Set vehicle fuel
   * @param vehicleId Vehicle ID
   * @param fuel New fuel value
   */
  static setFuel(vehicleId: string, fuel: number): void {
    if (nativeBindings.has('FVehicle_SetFuel')) {
      nativeBindings.call('FVehicle_SetFuel', vehicleId, fuel);
    }
  }
  
  /**
   * Teleport a vehicle
   * @param vehicleId Vehicle ID
   * @param location New location
   * @param rotation Optional new rotation
   */
  static teleport(vehicleId: string, location: FVector, rotation?: FRotator): void {
    if (nativeBindings.has('FVehicle_Teleport')) {
      nativeBindings.call('FVehicle_Teleport', vehicleId, location, rotation);
    }
  }
  
  /**
   * Eject all passengers from a vehicle
   * @param vehicleId Vehicle ID
   */
  static ejectAll(vehicleId: string): void {
    if (nativeBindings.has('FVehicle_EjectAll')) {
      nativeBindings.call('FVehicle_EjectAll', vehicleId);
    }
  }
  
  /**
   * Force a player into a vehicle
   * @param vehicleId Vehicle ID
   * @param playerId Player ID
   * @param seat Seat index (0 = driver)
   */
  static enterVehicle(vehicleId: string, playerId: string, seat: number = 0): void {
    if (nativeBindings.has('FVehicle_Enter')) {
      nativeBindings.call('FVehicle_Enter', vehicleId, playerId, seat);
    }
  }
  
  /**
   * Set vehicle boost/nitro amount
   * @param vehicleId Vehicle ID
   * @param boost Boost amount (0-100)
   */
  static setBoost(vehicleId: string, boost: number): void {
    if (nativeBindings.has('FVehicle_SetBoost')) {
      nativeBindings.call('FVehicle_SetBoost', vehicleId, Math.max(0, Math.min(100, boost)));
    }
  }
  
  /**
   * Enable or disable vehicle fuel consumption
   * @param enabled Whether fuel consumption is enabled
   */
  static setFuelConsumptionEnabled(enabled: boolean): void {
    if (nativeBindings.has('FVehicle_SetFuelConsumption')) {
      nativeBindings.call('FVehicle_SetFuelConsumption', enabled);
    }
  }
}
