/**
 * @trail-blaze/icarus-core
 * 
 * Icarus Core - Module system and runtime for Fortnite modifications
 * 
 * This is the core foundation of Icarus that enables JavaScript/TypeScript
 * modules to interact with Project Reboot's C++ game functions.
 * 
 * ## Key Concepts
 * 
 * ### Modules
 * Modules are function-driven scripts that can be loaded to extend the backend.
 * Each module must implement three functions:
 * - `ThreadStart`: Executed when the module is invoked
 * - `ThreadExit`: Executed when the module exits
 * - `ErrorHandler`: Executed when an error occurs
 * 
 * ### Example Module
 * ```ts
 * import { FWorld } from "@trail-blaze/retroflex";
 * import { Flare, FlareUtils } from "@trail-blaze/flare";
 * 
 * ThreadStart = (args) => {
 *   FWorld.setProperty("damage", "false");
 *   FWorld.getPawnList().forEach(pawn => pawn.move(700, 700, 700));
 *   return 0;
 * };
 * 
 * ThreadExit = (args) => {
 *   FWorld.getPawnList().forEach(pawn => pawn.kill());
 *   return 0;
 * };
 * 
 * ErrorHandler = (error: Flare) => {
 *   console.error(FlareUtils.parse(error).getReason());
 *   return 1;
 * };
 * ```
 * 
 * @packageDocumentation
 */

export * from './types';
export * from './Module';
export * from './ModuleManager';
export * from './ModuleLoader';
export * from './Runtime';
