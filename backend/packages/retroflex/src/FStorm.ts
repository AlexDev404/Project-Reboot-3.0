/**
 * FStorm - Storm/Safe Zone management
 * 
 * Provides control over the storm circles, safe zones, and related mechanics.
 * 
 * @example
 * ```ts
 * import { FStorm } from "@trail-blaze/retroflex";
 * 
 * // Skip to next storm phase
 * FStorm.nextPhase();
 * 
 * // Pause the storm
 * FStorm.pause();
 * 
 * // Set custom storm configuration
 * FStorm.setPhaseConfig(2, {
 *   shrinkTime: 120,
 *   waitTime: 60,
 *   damagePerSecond: 5
 * });
 * ```
 */

import { FVector } from './types';
import { nativeBindings } from './bindings';

/**
 * Storm phase configuration
 */
export interface StormPhaseConfig {
  /** Time for storm to shrink to next circle (seconds) */
  shrinkTime: number;
  /** Wait time before shrinking starts (seconds) */
  waitTime: number;
  /** Damage per second while in storm */
  damagePerSecond: number;
  /** Radius of the safe zone */
  radius?: number;
  /** Center of the safe zone */
  center?: FVector;
}

/**
 * Storm state information
 */
export interface StormState {
  /** Current phase index */
  currentPhase: number;
  /** Total number of phases */
  totalPhases: number;
  /** Is storm currently shrinking */
  isShrinking: boolean;
  /** Time remaining in current state (seconds) */
  timeRemaining: number;
  /** Current safe zone center */
  currentCenter: FVector;
  /** Current safe zone radius */
  currentRadius: number;
  /** Next safe zone center */
  nextCenter: FVector;
  /** Next safe zone radius */
  nextRadius: number;
}

export class FStorm {
  /**
   * Get current storm state
   */
  static getState(): StormState {
    if (nativeBindings.has('FStorm_GetState')) {
      return nativeBindings.call('FStorm_GetState') as StormState;
    }
    return {
      currentPhase: 0,
      totalPhases: 9,
      isShrinking: false,
      timeRemaining: 0,
      currentCenter: { x: 0, y: 0, z: 0 },
      currentRadius: 50000,
      nextCenter: { x: 0, y: 0, z: 0 },
      nextRadius: 25000
    };
  }
  
  /**
   * Get current storm phase
   */
  static getCurrentPhase(): number {
    return FStorm.getState().currentPhase;
  }
  
  /**
   * Skip to next storm phase
   */
  static nextPhase(): void {
    if (nativeBindings.has('FStorm_NextPhase')) {
      nativeBindings.call('FStorm_NextPhase');
    } else {
      console.log('[FStorm] Advancing to next phase');
    }
  }
  
  /**
   * Skip to a specific phase
   * @param phase Target phase index
   */
  static skipToPhase(phase: number): void {
    if (nativeBindings.has('FStorm_SkipToPhase')) {
      nativeBindings.call('FStorm_SkipToPhase', phase);
    }
  }
  
  /**
   * Pause storm progression
   */
  static pause(): void {
    if (nativeBindings.has('FStorm_Pause')) {
      nativeBindings.call('FStorm_Pause');
    }
  }
  
  /**
   * Resume storm progression
   */
  static resume(): void {
    if (nativeBindings.has('FStorm_Resume')) {
      nativeBindings.call('FStorm_Resume');
    }
  }
  
  /**
   * Check if storm is paused
   */
  static isPaused(): boolean {
    if (nativeBindings.has('FStorm_IsPaused')) {
      return nativeBindings.call('FStorm_IsPaused') as boolean;
    }
    return false;
  }
  
  /**
   * Set configuration for a specific phase
   * @param phase Phase index
   * @param config Phase configuration
   */
  static setPhaseConfig(phase: number, config: Partial<StormPhaseConfig>): void {
    if (nativeBindings.has('FStorm_SetPhaseConfig')) {
      nativeBindings.call('FStorm_SetPhaseConfig', phase, config);
    }
  }
  
  /**
   * Set the next safe zone location manually
   * @param center Center position
   * @param radius Zone radius
   */
  static setNextZone(center: FVector, radius: number): void {
    if (nativeBindings.has('FStorm_SetNextZone')) {
      nativeBindings.call('FStorm_SetNextZone', center, radius);
    }
  }
  
  /**
   * Disable storm completely
   */
  static disable(): void {
    if (nativeBindings.has('FStorm_Disable')) {
      nativeBindings.call('FStorm_Disable');
    }
  }
  
  /**
   * Enable storm
   */
  static enable(): void {
    if (nativeBindings.has('FStorm_Enable')) {
      nativeBindings.call('FStorm_Enable');
    }
  }
  
  /**
   * Check if a position is in the safe zone
   * @param position Position to check
   */
  static isInSafeZone(position: FVector): boolean {
    if (nativeBindings.has('FStorm_IsInSafeZone')) {
      return nativeBindings.call('FStorm_IsInSafeZone', position) as boolean;
    }
    return true;
  }
  
  /**
   * Get storm damage at a position
   * @param position Position to check
   */
  static getDamageAtPosition(position: FVector): number {
    if (nativeBindings.has('FStorm_GetDamageAt')) {
      return nativeBindings.call('FStorm_GetDamageAt', position) as number;
    }
    return 0;
  }
  
  /**
   * Set global storm damage multiplier
   * @param multiplier Damage multiplier (1.0 = normal)
   */
  static setDamageMultiplier(multiplier: number): void {
    if (nativeBindings.has('FStorm_SetDamageMultiplier')) {
      nativeBindings.call('FStorm_SetDamageMultiplier', multiplier);
    }
  }
}
