/**
 * ModuleLoader - Handles loading modules from various sources
 */

import { IcarusModule, ModuleLoadOptions } from './types';
import { ModuleManager, moduleManager } from './ModuleManager';

/**
 * Module source types
 */
export type ModuleSource = 'file' | 'string' | 'object';

/**
 * ModuleLoader - Loads modules from files, strings, or objects
 * 
 * @example
 * ```ts
 * const loader = new ModuleLoader(moduleManager);
 * 
 * // Load from object
 * loader.loadFromObject({
 *   name: "my-module",
 *   version: "1.0.0",
 *   ThreadStart: () => 0,
 *   ThreadExit: () => 0,
 *   ErrorHandler: () => 1
 * });
 * ```
 */
export class ModuleLoader {
  private manager: ModuleManager;
  
  constructor(manager: ModuleManager = moduleManager) {
    this.manager = manager;
  }
  
  /**
   * Load a module from a module object
   * @param module IcarusModule object
   * @param options Load options
   */
  loadFromObject(module: IcarusModule, options: ModuleLoadOptions = {}): void {
    this.validateModule(module);
    this.manager.register(module, options);
  }
  
  /**
   * Load multiple modules
   * @param modules Array of modules to load
   * @param options Load options applied to all
   */
  loadMultiple(modules: IcarusModule[], options: ModuleLoadOptions = {}): void {
    for (const module of modules) {
      this.loadFromObject(module, options);
    }
  }
  
  /**
   * Validate a module has all required properties
   * @throws Error if module is invalid
   */
  private validateModule(module: IcarusModule): void {
    const errors: string[] = [];
    
    if (!module.name || typeof module.name !== 'string') {
      errors.push('Module must have a string "name" property');
    }
    
    if (!module.version || typeof module.version !== 'string') {
      errors.push('Module must have a string "version" property');
    }
    
    if (typeof module.ThreadStart !== 'function') {
      errors.push('Module must have a "ThreadStart" function');
    }
    
    if (typeof module.ThreadExit !== 'function') {
      errors.push('Module must have a "ThreadExit" function');
    }
    
    if (typeof module.ErrorHandler !== 'function') {
      errors.push('Module must have an "ErrorHandler" function');
    }
    
    if (errors.length > 0) {
      throw new Error(`Invalid module:\n${errors.join('\n')}`);
    }
  }
  
  /**
   * Create a minimal valid module
   * @param name Module name
   * @param version Module version
   * @param threadStart ThreadStart implementation
   */
  static createModule(
    name: string,
    version: string,
    threadStart: (args: unknown[]) => number | Promise<number>
  ): IcarusModule {
    return {
      name,
      version,
      ThreadStart: threadStart,
      ThreadExit: () => 0,
      ErrorHandler: (error) => {
        console.error(`[${name}] Error:`, error.did_you_know);
        return 1;
      }
    };
  }
}

// Default singleton instance
export const moduleLoader = new ModuleLoader();
