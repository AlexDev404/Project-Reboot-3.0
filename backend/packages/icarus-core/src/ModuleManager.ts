/**
 * ModuleManager - Central registry and manager for all loaded modules
 */

import { Module } from './Module';
import {
  IcarusModule,
  ModuleMetadata,
  ModuleState,
  ModuleExecutionResult,
  ModuleConfig,
  ModuleLoadOptions
} from './types';

/**
 * ModuleManager - Manages the lifecycle of all Icarus modules
 * 
 * @example
 * ```ts
 * const manager = new ModuleManager();
 * 
 * // Register a module
 * manager.register({
 *   name: "my-module",
 *   version: "1.0.0",
 *   ThreadStart: (args) => 0,
 *   ThreadExit: (args) => 0,
 *   ErrorHandler: (error) => 1
 * });
 * 
 * // Start the module
 * await manager.start("my-module");
 * 
 * // Stop all modules
 * await manager.stopAll();
 * ```
 */
export class ModuleManager {
  private modules: Map<string, Module> = new Map();
  private defaultConfig: ModuleConfig = {};
  
  /**
   * Create a new ModuleManager
   * @param config Default configuration for all modules
   */
  constructor(config: ModuleConfig = {}) {
    this.defaultConfig = config;
  }
  
  /**
   * Register a module
   * @param module IcarusModule to register
   * @param options Load options
   */
  register(module: IcarusModule, options: ModuleLoadOptions = {}): void {
    const { isServerModule = false, config = {} } = options;
    
    if (this.modules.has(module.name)) {
      console.warn(`[ModuleManager] Module '${module.name}' already registered, overwriting`);
    }
    
    const wrappedModule = new Module(
      module,
      options.isServerModule ? 'server' : 'local',
      isServerModule,
      { ...this.defaultConfig, ...config }
    );
    
    this.modules.set(module.name, wrappedModule);
    console.log(`[ModuleManager] Registered module '${module.name}' v${module.version}`);
  }
  
  /**
   * Unregister a module
   * @param name Module name
   * @returns True if module was unregistered
   */
  async unregister(name: string): Promise<boolean> {
    const module = this.modules.get(name);
    
    if (!module) {
      console.warn(`[ModuleManager] Module '${name}' not found`);
      return false;
    }
    
    // Stop the module if running
    if (module.isRunning()) {
      await module.stop();
    }
    
    this.modules.delete(name);
    console.log(`[ModuleManager] Unregistered module '${name}'`);
    return true;
  }
  
  /**
   * Get a module by name
   * @param name Module name
   */
  get(name: string): Module | undefined {
    return this.modules.get(name);
  }
  
  /**
   * Check if a module is registered
   * @param name Module name
   */
  has(name: string): boolean {
    return this.modules.has(name);
  }
  
  /**
   * Start a module
   * @param name Module name
   * @param args Arguments to pass to ThreadStart
   */
  async start(name: string, args: unknown[] = []): Promise<ModuleExecutionResult> {
    const module = this.modules.get(name);
    
    if (!module) {
      return {
        success: false,
        returnCode: -1,
        executionTime: 0,
        error: {
          spark_id: 'MODULE_NOT_FOUND',
          severity: 'HIGH',
          attention: { trace: [] },
          did_you_know: `Module '${name}' not found`
        }
      };
    }
    
    return module.start(args);
  }
  
  /**
   * Stop a module
   * @param name Module name
   * @param args Arguments to pass to ThreadExit
   */
  async stop(name: string, args: unknown[] = []): Promise<ModuleExecutionResult> {
    const module = this.modules.get(name);
    
    if (!module) {
      return {
        success: false,
        returnCode: -1,
        executionTime: 0,
        error: {
          spark_id: 'MODULE_NOT_FOUND',
          severity: 'HIGH',
          attention: { trace: [] },
          did_you_know: `Module '${name}' not found`
        }
      };
    }
    
    return module.stop(args);
  }
  
  /**
   * Start all registered modules
   * @param args Arguments to pass to all ThreadStart functions
   */
  async startAll(args: unknown[] = []): Promise<Map<string, ModuleExecutionResult>> {
    const results = new Map<string, ModuleExecutionResult>();
    
    for (const [name, module] of this.modules) {
      results.set(name, await module.start(args));
    }
    
    return results;
  }
  
  /**
   * Stop all running modules
   * @param args Arguments to pass to all ThreadExit functions
   */
  async stopAll(args: unknown[] = []): Promise<Map<string, ModuleExecutionResult>> {
    const results = new Map<string, ModuleExecutionResult>();
    
    for (const [name, module] of this.modules) {
      if (module.isRunning()) {
        results.set(name, await module.stop(args));
      }
    }
    
    return results;
  }
  
  /**
   * Get all registered module names
   */
  list(): string[] {
    return Array.from(this.modules.keys());
  }
  
  /**
   * Get all module metadata
   */
  getAllMetadata(): ModuleMetadata[] {
    return Array.from(this.modules.values()).map(m => m.getMetadata());
  }
  
  /**
   * Get all running modules
   */
  getRunning(): Module[] {
    return Array.from(this.modules.values()).filter(m => m.isRunning());
  }
  
  /**
   * Get module state
   * @param name Module name
   */
  getState(name: string): ModuleState | undefined {
    return this.modules.get(name)?.getState();
  }
  
  /**
   * Get count of registered modules
   */
  get size(): number {
    return this.modules.size;
  }
}

// Default singleton instance
export const moduleManager = new ModuleManager();
