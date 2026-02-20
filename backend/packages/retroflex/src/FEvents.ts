/**
 * FEvents - Event system for hooks and callbacks
 * 
 * Provides a pub/sub event system for hooking into game events.
 * 
 * @example
 * ```ts
 * import { FEvents } from "@trail-blaze/retroflex";
 * 
 * // Listen for player eliminations
 * FEvents.on("player.eliminated", (data) => {
 *   console.log(`${data.victimId} was eliminated by ${data.killerId}`);
 * });
 * 
 * // Listen for match start
 * FEvents.on("match.start", () => {
 *   console.log("Match started!");
 * });
 * ```
 */

import { nativeBindings } from './bindings';

/**
 * Event types
 */
export type GameEventType =
  // Player events
  | 'player.join'
  | 'player.leave'
  | 'player.spawn'
  | 'player.eliminated'
  | 'player.dbno'
  | 'player.revived'
  | 'player.damage'
  | 'player.heal'
  // Match events
  | 'match.start'
  | 'match.end'
  | 'match.phase_change'
  // Storm events
  | 'storm.shrink_start'
  | 'storm.shrink_end'
  | 'storm.phase_change'
  // Building events
  | 'building.placed'
  | 'building.destroyed'
  | 'building.edited'
  // Loot events
  | 'loot.item_spawned'
  | 'loot.item_picked_up'
  | 'loot.chest_opened'
  | 'loot.supply_drop_landed'
  // Vehicle events
  | 'vehicle.spawned'
  | 'vehicle.destroyed'
  | 'vehicle.entered'
  | 'vehicle.exited';

/**
 * Event data for player elimination
 */
export interface PlayerEliminatedEvent {
  victimId: string;
  victimUsername: string;
  killerId?: string;
  killerUsername?: string;
  weaponId?: string;
  damageType?: string;
  distance?: number;
  isHeadshot?: boolean;
}

/**
 * Event data for player damage
 */
export interface PlayerDamageEvent {
  victimId: string;
  attackerId?: string;
  damage: number;
  weaponId?: string;
  damageType: string;
  isHeadshot: boolean;
  resultingHealth: number;
  resultingShield: number;
}

/**
 * Event data for player join/leave
 */
export interface PlayerConnectionEvent {
  playerId: string;
  username: string;
  teamId?: number;
}

/**
 * Event handler function type
 */
export type EventHandler<T = unknown> = (data: T) => void | Promise<void>;

/**
 * Event subscription
 */
interface EventSubscription {
  id: string;
  eventType: GameEventType;
  handler: EventHandler;
}

/**
 * Internal event storage
 */
const subscriptions: Map<GameEventType, EventSubscription[]> = new Map();
let subscriptionCounter = 0;

export class FEvents {
  /**
   * Subscribe to an event
   * @param eventType Event type to listen for
   * @param handler Handler function
   * @returns Subscription ID (use to unsubscribe)
   * 
   * @example
   * ```ts
   * const subId = FEvents.on("player.eliminated", (data) => {
   *   console.log(`${data.victimUsername} was eliminated!`);
   * });
   * ```
   */
  static on<T = unknown>(eventType: GameEventType, handler: EventHandler<T>): string {
    const id = `sub_${++subscriptionCounter}`;
    
    const subscription: EventSubscription = {
      id,
      eventType,
      handler: handler as EventHandler
    };
    
    if (!subscriptions.has(eventType)) {
      subscriptions.set(eventType, []);
    }
    
    subscriptions.get(eventType)!.push(subscription);
    
    // Register with native if available
    if (nativeBindings.has('FEvents_Subscribe')) {
      nativeBindings.call('FEvents_Subscribe', eventType, id);
    }
    
    return id;
  }
  
  /**
   * Subscribe to an event (one-time only)
   * @param eventType Event type
   * @param handler Handler function
   * @returns Subscription ID
   */
  static once<T = unknown>(eventType: GameEventType, handler: EventHandler<T>): string {
    const id = FEvents.on<T>(eventType, (data) => {
      FEvents.off(id);
      handler(data);
    });
    return id;
  }
  
  /**
   * Unsubscribe from an event
   * @param subscriptionId Subscription ID returned from on()
   */
  static off(subscriptionId: string): boolean {
    for (const [eventType, subs] of subscriptions) {
      const index = subs.findIndex(s => s.id === subscriptionId);
      if (index !== -1) {
        subs.splice(index, 1);
        
        // Unregister with native if available
        if (nativeBindings.has('FEvents_Unsubscribe')) {
          nativeBindings.call('FEvents_Unsubscribe', eventType, subscriptionId);
        }
        
        return true;
      }
    }
    return false;
  }
  
  /**
   * Remove all subscriptions for an event type
   * @param eventType Event type
   */
  static offAll(eventType: GameEventType): void {
    subscriptions.delete(eventType);
    
    if (nativeBindings.has('FEvents_UnsubscribeAll')) {
      nativeBindings.call('FEvents_UnsubscribeAll', eventType);
    }
  }
  
  /**
   * Emit an event (primarily for internal/testing use)
   * @param eventType Event type
   * @param data Event data
   */
  static emit<T = unknown>(eventType: GameEventType, data: T): void {
    const subs = subscriptions.get(eventType);
    
    if (subs) {
      for (const sub of subs) {
        try {
          sub.handler(data);
        } catch (error) {
          console.error(`[FEvents] Error in handler for ${eventType}:`, error);
        }
      }
    }
  }
  
  /**
   * Get number of subscriptions for an event type
   * @param eventType Event type
   */
  static getSubscriptionCount(eventType: GameEventType): number {
    return subscriptions.get(eventType)?.length ?? 0;
  }
  
  /**
   * Clear all event subscriptions
   */
  static clearAll(): void {
    subscriptions.clear();
    
    if (nativeBindings.has('FEvents_ClearAll')) {
      nativeBindings.call('FEvents_ClearAll');
    }
  }
  
  /**
   * Wait for an event (Promise-based)
   * @param eventType Event type
   * @param timeout Timeout in ms (0 = no timeout)
   * @returns Promise that resolves with event data
   */
  static waitFor<T = unknown>(eventType: GameEventType, timeout: number = 0): Promise<T> {
    return new Promise((resolve, reject) => {
      let timeoutId: NodeJS.Timeout | undefined;
      
      const subId = FEvents.once<T>(eventType, (data) => {
        if (timeoutId) clearTimeout(timeoutId);
        resolve(data);
      });
      
      if (timeout > 0) {
        timeoutId = setTimeout(() => {
          FEvents.off(subId);
          reject(new Error(`Timeout waiting for event: ${eventType}`));
        }, timeout);
      }
    });
  }
}

/**
 * Internal function called by native code to dispatch events
 * @internal
 */
export function _dispatchNativeEvent(eventType: GameEventType, data: unknown): void {
  FEvents.emit(eventType, data);
}
