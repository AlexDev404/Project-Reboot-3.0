/**
 * Native Bindings System
 * 
 * This module handles the communication between JavaScript and the C++ backend.
 * Native functions are registered from C++ and can be called from JavaScript.
 * 
 * **Error Handling**: When a native function fails or throws an exception,
 * the error is caught and converted to a Flare object. This prevents C++
 * exceptions from crashing the JavaScript runtime. The Flare is then passed
 * to the module's ErrorHandler for graceful handling.
 * 
 * In the actual implementation, these bindings are populated by the C++ runtime
 * when the JavaScript engine is initialized within Project Reboot.
 */

import { NativeFunction, BindingRegistry } from './types';

/**
 * Native binding error - thrown when a C++ function fails
 */
export interface NativeError {
  code: number;
  function: string;
  message: string;
  nativeTrace?: string;
}

/**
 * Result wrapper for native function calls
 * Success: { success: true, value: T }
 * Failure: { success: false, error: NativeError }
 */
export type NativeResult<T> = 
  | { success: true; value: T }
  | { success: false; error: NativeError };

/**
 * Global binding registry
 * In production, this is populated by the C++ runtime
 */
class NativeBindings implements BindingRegistry {
  private bindings: Map<string, NativeFunction> = new Map();
  private isInitialized: boolean = false;
  private lastError: NativeError | null = null;
  
  /**
   * Register a native function from C++
   */
  register(name: string, fn: NativeFunction): void {
    this.bindings.set(name, fn);
  }
  
  /**
   * Call a native function with error handling
   * Catches any C++ exceptions and converts them to NativeError
   * @throws NativeError if binding doesn't exist or call fails
   */
  call(name: string, ...args: unknown[]): unknown {
    const fn = this.bindings.get(name);
    
    if (!fn) {
      // In development/mock mode, log and return undefined
      if (!this.isInitialized) {
        console.warn(`[Retroflex] Native binding '${name}' not found. Running in mock mode.`);
        return undefined;
      }
      
      this.lastError = {
        code: 5, // NOT_IMPLEMENTED
        function: name,
        message: `Native binding '${name}' not registered`
      };
      throw this.lastError;
    }
    
    try {
      this.lastError = null;
      return fn(...args);
    } catch (error) {
      // Convert native exception to NativeError
      this.lastError = {
        code: 1, // UNKNOWN
        function: name,
        message: error instanceof Error ? error.message : String(error),
        nativeTrace: error instanceof Error ? error.stack : undefined
      };
      throw this.lastError;
    }
  }
  
  /**
   * Safely call a native function, returning a result wrapper
   * This never throws - errors are returned in the result
   */
  safeCall<T>(name: string, ...args: unknown[]): NativeResult<T> {
    try {
      const value = this.call(name, ...args) as T;
      return { success: true, value };
    } catch (error) {
      return { success: false, error: error as NativeError };
    }
  }
  
  /**
   * Get the last error that occurred
   */
  getLastError(): NativeError | null {
    return this.lastError;
  }
  
  /**
   * Clear the last error
   */
  clearLastError(): void {
    this.lastError = null;
  }
  
  /**
   * Check if a binding exists
   */
  has(name: string): boolean {
    return this.bindings.has(name);
  }
  
  /**
   * Get all registered binding names
   */
  list(): string[] {
    return Array.from(this.bindings.keys());
  }
  
  /**
   * Mark bindings as initialized (called by C++ runtime)
   */
  initialize(): void {
    this.isInitialized = true;
    console.log(`[Retroflex] Native bindings initialized with ${this.bindings.size} functions`);
  }
  
  /**
   * Check if running in native mode or mock mode
   */
  isNativeMode(): boolean {
    return this.isInitialized;
  }
}

// Singleton instance - will be populated by C++ runtime
export const nativeBindings = new NativeBindings();

/**
 * Decorator for native-bound methods
 * Automatically routes calls to the native binding
 */
export function nativeBound(bindingName: string) {
  return function (
    _target: unknown,
    _propertyKey: string,
    descriptor: PropertyDescriptor
  ) {
    const originalMethod = descriptor.value;
    
    descriptor.value = function (...args: unknown[]) {
      if (nativeBindings.has(bindingName)) {
        return nativeBindings.call(bindingName, ...args);
      }
      // Fall back to mock implementation
      return originalMethod.apply(this, args);
    };
    
    return descriptor;
  };
}

/**
 * Native binding names used by the SDK
 * These must match the C++ registration
 */
export const BindingNames = {
  // World bindings
  WORLD_GET_PROPERTY: 'FWorld_GetProperty',
  WORLD_SET_PROPERTY: 'FWorld_SetProperty',
  WORLD_GET_PAWN_LIST: 'FWorld_GetPawnList',
  WORLD_GET_PAWN_BY_ID: 'FWorld_GetPawnById',
  WORLD_GET_PAWN_BY_USERNAME: 'FWorld_GetPawnByUsername',
  WORLD_SPAWN_ACTOR: 'FWorld_SpawnActor',
  WORLD_DESTROY_ACTOR: 'FWorld_DestroyActor',
  
  // Pawn bindings
  PAWN_MOVE: 'FPawn_Move',
  PAWN_TELEPORT: 'FPawn_Teleport',
  PAWN_SET_ROTATION: 'FPawn_SetRotation',
  PAWN_GET_LOCATION: 'FPawn_GetLocation',
  PAWN_GET_ROTATION: 'FPawn_GetRotation',
  PAWN_COSTUME: 'FPawn_SetCostume',
  PAWN_GET_COSMETICS: 'FPawn_GetCosmetics',
  PAWN_KILL: 'FPawn_Kill',
  PAWN_REVIVE: 'FPawn_Revive',
  PAWN_APPLY_DAMAGE: 'FPawn_ApplyDamage',
  PAWN_SET_HEALTH: 'FPawn_SetHealth',
  PAWN_SET_SHIELD: 'FPawn_SetShield',
  PAWN_GET_STATS: 'FPawn_GetStats',
  PAWN_GIVE_ITEM: 'FPawn_GiveItem',
  PAWN_REMOVE_ITEM: 'FPawn_RemoveItem',
  PAWN_GET_INVENTORY: 'FPawn_GetInventory',
  PAWN_PLAY_EMOTE: 'FPawn_PlayEmote',
  PAWN_SEND_MESSAGE: 'FPawn_SendMessage',
  PAWN_GET_STATE: 'FPawn_GetState',
  
  // Building bindings
  BUILD_PLACE: 'FBuild_Place',
  BUILD_DESTROY: 'FBuild_Destroy',
  BUILD_SET_HEALTH: 'FBuild_SetHealth',
  
  // Game state bindings
  GAME_START_MATCH: 'FGame_StartMatch',
  GAME_END_MATCH: 'FGame_EndMatch',
  GAME_SET_STORM: 'FGame_SetStorm',
  GAME_SPAWN_LOOT: 'FGame_SpawnLoot',
  GAME_SPAWN_VEHICLE: 'FGame_SpawnVehicle',
} as const;

export type BindingName = typeof BindingNames[keyof typeof BindingNames];
