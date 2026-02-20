/**
 * Runtime - Main entry point for the Icarus module runtime
 */

import { ModuleManager, moduleManager } from './ModuleManager';
import { ModuleLoader, moduleLoader } from './ModuleLoader';
import { IcarusModule, ModuleConfig } from './types';

export interface RuntimeOptions {
  /** Whether to enable verbose logging */
  verbose?: boolean;
  
  /** Default module configuration */
  moduleConfig?: ModuleConfig;
  
  /** Modules to auto-load on startup */
  autoLoadModules?: IcarusModule[];
}

/**
 * Runtime - The main Icarus runtime environment
 * 
 * The Runtime manages the module lifecycle and provides the entry point
 * for running Icarus modules.
 * 
 * @example
 * ```ts
 * const runtime = new Runtime({ verbose: true });
 * 
 * runtime.loadModule({
 *   name: "hello-world",
 *   version: "1.0.0",
 *   ThreadStart: () => { console.log("Hello!"); return 0; },
 *   ThreadExit: () => { console.log("Goodbye!"); return 0; },
 *   ErrorHandler: () => 1
 * });
 * 
 * await runtime.start();
 * // ... later
 * await runtime.shutdown();
 * ```
 */
export class Runtime {
  private manager: ModuleManager;
  private loader: ModuleLoader;
  private options: RuntimeOptions;
  private isRunning: boolean = false;
  
  constructor(options: RuntimeOptions = {}) {
    this.options = options;
    this.manager = new ModuleManager(options.moduleConfig);
    this.loader = new ModuleLoader(this.manager);
    
    // Auto-load modules if specified
    if (options.autoLoadModules) {
      for (const module of options.autoLoadModules) {
        this.loadModule(module);
      }
    }
    
    if (options.verbose) {
      console.log('[Runtime] Icarus runtime initialized');
    }
  }
  
  /**
   * Load a module into the runtime
   * @param module Module to load
   */
  loadModule(module: IcarusModule): void {
    this.loader.loadFromObject(module, {
      config: this.options.moduleConfig
    });
  }
  
  /**
   * Unload a module from the runtime
   * @param name Module name
   */
  async unloadModule(name: string): Promise<boolean> {
    return this.manager.unregister(name);
  }
  
  /**
   * Start the runtime and all loaded modules
   */
  async start(): Promise<void> {
    if (this.isRunning) {
      console.warn('[Runtime] Runtime is already running');
      return;
    }
    
    this.isRunning = true;
    
    if (this.options.verbose) {
      console.log(`[Runtime] Starting ${this.manager.size} modules...`);
    }
    
    const results = await this.manager.startAll();
    
    let successCount = 0;
    let failCount = 0;
    
    for (const [name, result] of results) {
      if (result.success) {
        successCount++;
      } else {
        failCount++;
        console.error(`[Runtime] Module '${name}' failed to start:`, result.error?.did_you_know);
      }
    }
    
    if (this.options.verbose) {
      console.log(`[Runtime] Started ${successCount} modules, ${failCount} failed`);
    }
  }
  
  /**
   * Start a specific module
   * @param name Module name
   * @param args Arguments to pass
   */
  async startModule(name: string, args: unknown[] = []): Promise<void> {
    const result = await this.manager.start(name, args);
    
    if (!result.success) {
      console.error(`[Runtime] Module '${name}' failed to start:`, result.error?.did_you_know);
    }
  }
  
  /**
   * Stop a specific module
   * @param name Module name
   * @param args Arguments to pass
   */
  async stopModule(name: string, args: unknown[] = []): Promise<void> {
    const result = await this.manager.stop(name, args);
    
    if (!result.success) {
      console.error(`[Runtime] Module '${name}' failed to stop:`, result.error?.did_you_know);
    }
  }
  
  /**
   * Shutdown the runtime and all modules
   */
  async shutdown(): Promise<void> {
    if (!this.isRunning) {
      return;
    }
    
    if (this.options.verbose) {
      console.log('[Runtime] Shutting down...');
    }
    
    await this.manager.stopAll();
    this.isRunning = false;
    
    if (this.options.verbose) {
      console.log('[Runtime] Shutdown complete');
    }
  }
  
  /**
   * Get the module manager
   */
  getManager(): ModuleManager {
    return this.manager;
  }
  
  /**
   * Get the module loader
   */
  getLoader(): ModuleLoader {
    return this.loader;
  }
  
  /**
   * Check if runtime is running
   */
  getIsRunning(): boolean {
    return this.isRunning;
  }
  
  /**
   * Get list of loaded modules
   */
  listModules(): string[] {
    return this.manager.list();
  }
}

// Default runtime instance
export const runtime = new Runtime();
