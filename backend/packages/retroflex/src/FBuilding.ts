/**
 * FBuilding - Building placement and management
 * 
 * Provides methods for creating, modifying, and destroying building pieces.
 * 
 * @example
 * ```ts
 * import { FBuilding } from "@trail-blaze/retroflex";
 * 
 * // Place a wall
 * const wall = FBuilding.place("wall", { x: 100, y: 200, z: 300 });
 * 
 * // Set building health
 * FBuilding.setHealth(wall.id, 500);
 * 
 * // Destroy all buildings in area
 * FBuilding.destroyInRadius({ x: 0, y: 0, z: 0 }, 1000);
 * ```
 */

import { FVector, FRotator, BuildingPiece } from './types';
import { nativeBindings, BindingNames } from './bindings';

/**
 * Building material types
 */
export type BuildingMaterial = 'wood' | 'stone' | 'metal';

/**
 * Building instance info
 */
export interface BuildingInfo {
  id: string;
  type: BuildingPiece;
  material: BuildingMaterial;
  location: FVector;
  rotation: FRotator;
  health: number;
  maxHealth: number;
  ownerId?: string;
  teamId?: number;
}

/**
 * Building place options
 */
export interface BuildingPlaceOptions {
  material?: BuildingMaterial;
  rotation?: FRotator;
  health?: number;
  ownerId?: string;
  teamId?: number;
}

export class FBuilding {
  /**
   * Place a building piece
   * @param type Building piece type
   * @param location World location
   * @param options Build options
   * @returns Building info or undefined on failure
   */
  static place(
    type: BuildingPiece, 
    location: FVector, 
    options: BuildingPlaceOptions = {}
  ): BuildingInfo | undefined {
    if (nativeBindings.has(BindingNames.BUILD_PLACE)) {
      return nativeBindings.call(BindingNames.BUILD_PLACE, type, location, options) as BuildingInfo;
    }
    console.log(`[FBuilding] Placing ${type} at (${location.x}, ${location.y}, ${location.z})`);
    return undefined;
  }
  
  /**
   * Destroy a building piece
   * @param buildingId Building ID
   * @returns True if destroyed
   */
  static destroy(buildingId: string): boolean {
    if (nativeBindings.has(BindingNames.BUILD_DESTROY)) {
      return nativeBindings.call(BindingNames.BUILD_DESTROY, buildingId) as boolean;
    }
    return false;
  }
  
  /**
   * Destroy all buildings in a radius
   * @param center Center position
   * @param radius Destruction radius
   * @returns Number of buildings destroyed
   */
  static destroyInRadius(center: FVector, radius: number): number {
    if (nativeBindings.has('FBuild_DestroyInRadius')) {
      return nativeBindings.call('FBuild_DestroyInRadius', center, radius) as number;
    }
    return 0;
  }
  
  /**
   * Get building info by ID
   * @param buildingId Building ID
   */
  static getInfo(buildingId: string): BuildingInfo | undefined {
    if (nativeBindings.has('FBuild_GetInfo')) {
      return nativeBindings.call('FBuild_GetInfo', buildingId) as BuildingInfo;
    }
    return undefined;
  }
  
  /**
   * Set building health
   * @param buildingId Building ID
   * @param health New health value
   */
  static setHealth(buildingId: string, health: number): void {
    if (nativeBindings.has(BindingNames.BUILD_SET_HEALTH)) {
      nativeBindings.call(BindingNames.BUILD_SET_HEALTH, buildingId, health);
    }
  }
  
  /**
   * Get all buildings in an area
   * @param center Center position
   * @param radius Search radius
   */
  static getBuildingsInRadius(center: FVector, radius: number): BuildingInfo[] {
    if (nativeBindings.has('FBuild_GetInRadius')) {
      return nativeBindings.call('FBuild_GetInRadius', center, radius) as BuildingInfo[];
    }
    return [];
  }
  
  /**
   * Get all buildings owned by a player
   * @param ownerId Player ID
   */
  static getBuildingsByOwner(ownerId: string): BuildingInfo[] {
    if (nativeBindings.has('FBuild_GetByOwner')) {
      return nativeBindings.call('FBuild_GetByOwner', ownerId) as BuildingInfo[];
    }
    return [];
  }
  
  /**
   * Set material health multiplier
   * @param material Material type
   * @param multiplier Health multiplier (1.0 = normal)
   */
  static setMaterialHealthMultiplier(material: BuildingMaterial, multiplier: number): void {
    if (nativeBindings.has('FBuild_SetMaterialHealth')) {
      nativeBindings.call('FBuild_SetMaterialHealth', material, multiplier);
    }
  }
  
  /**
   * Set material build time multiplier
   * @param material Material type
   * @param multiplier Build time multiplier (1.0 = normal)
   */
  static setMaterialBuildTimeMultiplier(material: BuildingMaterial, multiplier: number): void {
    if (nativeBindings.has('FBuild_SetMaterialBuildTime')) {
      nativeBindings.call('FBuild_SetMaterialBuildTime', material, multiplier);
    }
  }
  
  /**
   * Enable or disable building editing
   * @param enabled Whether editing is allowed
   */
  static setEditingEnabled(enabled: boolean): void {
    if (nativeBindings.has('FBuild_SetEditingEnabled')) {
      nativeBindings.call('FBuild_SetEditingEnabled', enabled);
    }
  }
  
  /**
   * Clear all buildings on the map
   * @returns Number of buildings cleared
   */
  static clearAll(): number {
    if (nativeBindings.has('FBuild_ClearAll')) {
      return nativeBindings.call('FBuild_ClearAll') as number;
    }
    return 0;
  }
}
