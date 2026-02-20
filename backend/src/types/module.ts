import { Flare } from './flare';

/**
 * Module interface for Icarus modules
 * 
 * Modules are small function-driven scripts that interact with the backend.
 * Each module must implement three main functions:
 * - ThreadStart: Executed when the module is invoked after startup
 * - ThreadExit: Executed when the module decides to exit
 * - ErrorHandler: Executed when an error is encountered
 */

export type ThreadFunction = (args: unknown[]) => number | Promise<number>;
export type ErrorHandlerFunction = (error: Flare) => number | Promise<number>;

export interface IcarusModule {
  name: string;
  version: string;
  description?: string;
  
  ThreadStart: ThreadFunction;
  ThreadExit: ThreadFunction;
  ErrorHandler: ErrorHandlerFunction;
}

/**
 * Module metadata
 */
export interface ModuleMetadata {
  name: string;
  version: string;
  description?: string;
  author?: string;
  path: string;
  isServerModule: boolean;
  loadedAt: Date;
}

/**
 * Module execution result
 */
export interface ModuleExecutionResult {
  success: boolean;
  returnCode: number;
  executionTime: number;
  error?: Flare;
}

/**
 * Module state
 */
export enum ModuleState {
  UNLOADED = 'UNLOADED',
  LOADING = 'LOADING',
  LOADED = 'LOADED',
  RUNNING = 'RUNNING',
  STOPPED = 'STOPPED',
  ERROR = 'ERROR'
}
