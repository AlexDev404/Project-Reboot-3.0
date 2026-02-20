/**
 * Types for the Icarus module system
 */

import { Flare } from '@trail-blaze/flare';

/**
 * Thread function signature
 * @param args Array of arguments passed to the function
 * @returns Exit code (0 for success, non-zero for error)
 */
export type ThreadFunction = (args: unknown[]) => number | Promise<number>;

/**
 * Error handler function signature
 * @param error Flare error object
 * @returns Exit code
 */
export type ErrorHandlerFunction = (error: Flare) => number | Promise<number>;

/**
 * Module interface - must be implemented by all Icarus modules
 * 
 * @example
 * ```ts
 * const myModule: IcarusModule = {
 *   name: "my-module",
 *   version: "1.0.0",
 *   ThreadStart: (args) => { console.log("Started"); return 0; },
 *   ThreadExit: (args) => { console.log("Exiting"); return 0; },
 *   ErrorHandler: (error) => { console.error(error); return 1; }
 * };
 * ```
 */
export interface IcarusModule {
  /** Module name (unique identifier) */
  name: string;
  
  /** Module version (semver format) */
  version: string;
  
  /** Optional description */
  description?: string;
  
  /** Optional author */
  author?: string;
  
  /**
   * Executed when the module is invoked after startup
   * @param args Array of arguments
   * @returns Exit code (0 = success)
   */
  ThreadStart: ThreadFunction;
  
  /**
   * Executed when the module decides to exit
   * @param args Array of arguments
   * @returns Exit code (0 = success)
   */
  ThreadExit: ThreadFunction;
  
  /**
   * Executed when an error is encountered
   * @param error Formatted Flare error object
   * @returns Exit code
   */
  ErrorHandler: ErrorHandlerFunction;
}

/**
 * Module metadata
 */
export interface ModuleMetadata {
  /** Module name */
  name: string;
  
  /** Module version */
  version: string;
  
  /** Module description */
  description?: string;
  
  /** Module author */
  author?: string;
  
  /** Path to module file */
  path: string;
  
  /** Whether this is a server-hosted module */
  isServerModule: boolean;
  
  /** When the module was loaded */
  loadedAt: Date;
}

/**
 * Module lifecycle state
 */
export enum ModuleState {
  /** Module has not been loaded */
  UNLOADED = 'UNLOADED',
  
  /** Module is being loaded */
  LOADING = 'LOADING',
  
  /** Module is loaded but not running */
  LOADED = 'LOADED',
  
  /** Module is currently running */
  RUNNING = 'RUNNING',
  
  /** Module has been stopped */
  STOPPED = 'STOPPED',
  
  /** Module encountered an error */
  ERROR = 'ERROR'
}

/**
 * Module execution result
 */
export interface ModuleExecutionResult {
  /** Whether execution was successful */
  success: boolean;
  
  /** Return code from the thread function */
  returnCode: number;
  
  /** Execution time in milliseconds */
  executionTime: number;
  
  /** Flare error if execution failed */
  error?: Flare;
}

/**
 * Module configuration options
 */
export interface ModuleConfig {
  /** Maximum execution time before timeout (ms) */
  timeout?: number;
  
  /** Whether to auto-restart on error */
  autoRestart?: boolean;
  
  /** Maximum auto-restart attempts */
  maxRestarts?: number;
  
  /** Whether to log execution details */
  verbose?: boolean;
}

/**
 * Module load options
 */
export interface ModuleLoadOptions {
  /** Whether this is a server-hosted module */
  isServerModule?: boolean;
  
  /** Module configuration */
  config?: ModuleConfig;
}
