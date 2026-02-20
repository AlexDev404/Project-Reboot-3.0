/**
 * Core types for the Retroflex SDK
 */

/**
 * World state enumeration
 * - disengaged: Inactive/lobby
 * - engaged: On spawn island
 * - inProgress: Game is in progress
 */
export type WorldState = 'disengaged' | 'engaged' | 'inProgress';

/**
 * World properties structure
 */
export interface WorldProperties {
  /** Current world state */
  state: WorldState;
  /** Current game mode identifier */
  game_mode: string;
  /** Current playlist identifier */
  playlist: string;
  /** Whether damage is enabled ("true" or "false") */
  damage: string;
  /** Current safe zone phase */
  safe_zone_phase: number;
  /** Time remaining in current phase (seconds) */
  time_remaining: number;
  /** Total players in the match */
  player_count: number;
  /** Players still alive */
  players_alive: number;
}

/**
 * 3D Vector representation
 */
export interface FVector {
  x: number;
  y: number;
  z: number;
}

/**
 * Rotation representation
 */
export interface FRotator {
  pitch: number;
  yaw: number;
  roll: number;
}

/**
 * Transform (position + rotation + scale)
 */
export interface FTransform {
  location: FVector;
  rotation: FRotator;
  scale: FVector;
}

/**
 * Cosmetic item types
 */
export type CosmeticType = 
  | 'character'      // CID_xxx
  | 'backpack'       // BID_xxx
  | 'pickaxe'        // Pickaxe_xxx
  | 'glider'         // Glider_xxx
  | 'contrail'       // Trails_xxx
  | 'emote'          // EID_xxx
  | 'wrap'           // Wrap_xxx
  | 'loadingscreen'; // LSID_xxx

/**
 * Pawn cosmetic loadout
 */
export interface CosmeticLoadout {
  character?: string;
  backpack?: string;
  pickaxe?: string;
  glider?: string;
  contrail?: string;
  emotes?: string[];
  wraps?: string[];
}

/**
 * Pawn inventory item
 */
export interface InventoryItem {
  /** Item definition ID */
  itemId: string;
  /** Item quantity */
  quantity: number;
  /** Slot index (0-5 for weapons, -1 for resources) */
  slot: number;
  /** Is item equipped */
  isEquipped: boolean;
}

/**
 * Pawn stats
 */
export interface PawnStats {
  health: number;
  maxHealth: number;
  shield: number;
  maxShield: number;
  kills: number;
  assists: number;
  revives: number;
  damageDealt: number;
  damageTaken: number;
}

/**
 * Pawn state
 */
export type PawnState = 
  | 'alive'
  | 'dbno'        // Down But Not Out
  | 'dead'
  | 'spectating'
  | 'disconnected';

/**
 * Building piece types
 */
export type BuildingPiece = 'wall' | 'floor' | 'stairs' | 'roof' | 'cone';

/**
 * Team information
 */
export interface TeamInfo {
  teamId: number;
  teamIndex: number;
  memberCount: number;
  membersAlive: number;
}

/**
 * Native binding function signature
 * These are implemented in C++ and exposed to JavaScript
 */
export type NativeFunction = (...args: unknown[]) => unknown;

/**
 * Binding registry for C++ <-> JS communication
 */
export interface BindingRegistry {
  /** Register a native function */
  register(name: string, fn: NativeFunction): void;
  /** Call a native function */
  call(name: string, ...args: unknown[]): unknown;
  /** Check if a binding exists */
  has(name: string): boolean;
  /** Get all registered binding names */
  list(): string[];
}
