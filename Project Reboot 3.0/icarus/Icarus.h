#pragma once

/**
 * Icarus - JavaScript/TypeScript Binding Layer for Project Reboot
 * 
 * This header provides the main interface for integrating Icarus into
 * Project Reboot. Include this header and call the initialization functions
 * to enable JavaScript module support.
 * 
 * ## Quick Start
 * 
 * ```cpp
 * #include "icarus/Icarus.h"
 * 
 * // In DLL initialization:
 * Icarus::initializeIcarus();
 * 
 * // Load a module:
 * Icarus::loadIcarusModule("modules/my_module.js");
 * 
 * // Execute inline code:
 * Icarus::executeIcarusCode("FWorld.getPawnList().forEach(p => p.setHealth(100));");
 * 
 * // On shutdown:
 * Icarus::shutdownIcarus();
 * ```
 * 
 * ## Module Structure
 * 
 * Modules are JavaScript files that define three functions:
 * - ThreadStart(args): Called when the module is started
 * - ThreadExit(args): Called when the module exits
 * - ErrorHandler(flare): Called when a native binding throws an error
 * 
 * ## SDK Objects (Available in Modules)
 * 
 * - FWorld: World state and pawn management
 * - FPawn: Player/bot character control
 * - FGame: Match and game state control
 * - FStorm: Storm/safe zone management
 * - FBuilding: Building placement and editing
 * - FVehicle: Vehicle spawning and control
 * - FLoot: Loot and item spawning
 * - FInventory: Player inventory management
 * - FTeam: Team management
 * - FBots: AI bot spawning and control
 * - FAdmin: Server administration
 * - FServer: Server configuration
 * - FMutator: Game mutators/modifiers
 * - FCreative: Creative mode utilities
 * - FEvents: Event system for hooks
 * 
 * ## Error Handling
 * 
 * When a native C++ function fails, the error is wrapped in a Flare object
 * and passed to the module's ErrorHandler. This prevents C++ exceptions
 * from crashing the JavaScript runtime.
 * 
 * Flare structure:
 * ```js
 * {
 *   spark_id: "unique-error-id",
 *   severity: "LOW" | "MEDIUM" | "HIGH" | "CRITICAL",
 *   attention: { trace: [...] },
 *   did_you_know: "Human-readable error message"
 * }
 * ```
 */

#include "IcarusRuntime.h"
#include "IcarusBindings.h"

// Re-export main functions
namespace Icarus {

/**
 * Initialize the Icarus JavaScript runtime
 * Call this once during Project Reboot initialization
 */
void initializeIcarus();

/**
 * Shutdown the Icarus runtime and free resources
 * Call this when Project Reboot is shutting down
 */
void shutdownIcarus();

/**
 * Load and execute a JavaScript module file
 * @param modulePath Path to the module file
 * @return true if the module was loaded successfully
 */
bool loadIcarusModule(const std::string& modulePath);

/**
 * Execute inline JavaScript code
 * @param code JavaScript code to execute
 * @return Result of the evaluation
 */
JSValue executeIcarusCode(const std::string& code);

/**
 * Get the global runtime instance
 */
Runtime& getRuntime();

} // namespace Icarus
