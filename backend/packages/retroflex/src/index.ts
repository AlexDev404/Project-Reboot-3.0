/**
 * @trail-blaze/retroflex
 * 
 * Fortnite SDK - TypeScript bindings for Project Reboot C++ functions
 * 
 * This package is the core foundation of Icarus, providing JavaScript/TypeScript
 * interfaces to interact with the Fortnite game world through Project Reboot's
 * C++ backend.
 * 
 * ## Core Concept
 * 
 * Retroflex binds C++ functions from Project Reboot to JavaScript, enabling
 * developers to write Fortnite modifications in modern TypeScript/JavaScript
 * through the Icarus module system.
 * 
 * ## Main Components
 * 
 * - **FWorld** - World state and pawn management
 * - **FPawn** - Player/bot character control
 * - **FGame** - Match and game state control
 * - **FStorm** - Storm/safe zone management
 * - **FBuilding** - Building placement and editing
 * - **FVehicle** - Vehicle spawning and control
 * - **FLoot** - Loot and item spawning
 * - **FInventory** - Player inventory management
 * - **FTeam** - Team management
 * - **FBots** - AI bot spawning and control
 * - **FAdmin** - Server administration
 * - **FServer** - Server configuration
 * - **FMutator** - Game mutators/modifiers
 * - **FCreative** - Creative mode utilities
 * - **FEvents** - Event system for hooks
 * 
 * @example
 * ```ts
 * import { FWorld, FPawn, FGame } from "@trail-blaze/retroflex";
 * 
 * // Get world properties
 * const state = FWorld.getProperty("state");
 * 
 * // Disable fall damage
 * FWorld.setProperty("damage", "false");
 * 
 * // Move all pawns
 * FWorld.getPawnList().forEach((pawn) => {
 *   pawn.move(70, 70, 70);
 * });
 * 
 * // Start the match
 * FGame.startMatch();
 * ```
 * 
 * @packageDocumentation
 */

// Core types and bindings
export * from './types';
export * from './bindings';

// World and Pawns
export * from './FWorld';
export * from './FPawn';

// Game Control
export * from './FGame';
export * from './FStorm';

// Building and Environment
export * from './FBuilding';
export * from './FVehicle';
export * from './FLoot';

// Player Management
export * from './FInventory';
export * from './FTeam';

// Bots
export * from './FBots';

// Administration
export * from './FAdmin';
export * from './FServer';
export * from './FMutator';

// Creative Mode
export * from './FCreative';

// Events
export * from './FEvents';
