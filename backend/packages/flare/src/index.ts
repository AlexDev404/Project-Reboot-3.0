/**
 * @trail-blaze/flare
 * 
 * Flare - Error handling system for C/C++ binding exceptions in Icarus modules
 * 
 * Flare's purpose is to catch SDK errors or exceptions from the C/C++ native
 * binding layer so they don't cause runtime errors in the JavaScript environment.
 * When a native function call fails, the error is wrapped in a Flare object and
 * passed to the module's ErrorHandler function.
 * 
 * Each Flare contains:
 * - spark_id: Unique error identifier for database storage/tracking
 * - severity: Error severity level (LOW, MEDIUM, HIGH, CRITICAL)
 * - attention: Stack trace information showing where the error occurred
 * - did_you_know: Human-readable error description
 * 
 * **Important**: Flares are ONLY used in the ErrorHandler function of modules.
 * They wrap native binding failures to allow graceful error handling.
 * 
 * @example
 * ```ts
 * import { Flare, FlareUtils } from "@trail-blaze/flare";
 * 
 * // In a module's ErrorHandler:
 * ErrorHandler = (error: Flare) => {
 *   console.error(FlareUtils.parse(error).getReason());
 *   // Attempt recovery or cleanup
 *   return 1;
 * };
 * ```
 * 
 * @packageDocumentation
 */

export * from './types';
export * from './Flare';
export * from './FlareUtils';
