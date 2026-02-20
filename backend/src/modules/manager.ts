import { IcarusModule, ModuleMetadata, ModuleState, ModuleExecutionResult } from '../types/module';
import { Flare, parseFlare } from '../types/flare';

/**
 * Module Manager - Handles loading and executing Icarus modules
 */
export class ModuleManager {
  private modules: Map<string, IcarusModule> = new Map();
  private metadata: Map<string, ModuleMetadata> = new Map();
  private states: Map<string, ModuleState> = new Map();
  
  /**
   * Register a module
   */
  registerModule(module: IcarusModule, isServerModule: boolean = false, path: string = ''): void {
    const { name, version, description } = module;
    
    if (this.modules.has(name)) {
      console.warn(`[Icarus] Module '${name}' is already registered. Overwriting.`);
    }
    
    this.modules.set(name, module);
    this.metadata.set(name, {
      name,
      version,
      description,
      path,
      isServerModule,
      loadedAt: new Date()
    });
    this.states.set(name, ModuleState.LOADED);
    
    console.log(`[Icarus] Module '${name}' v${version} registered`);
  }
  
  /**
   * Unregister a module
   */
  async unregisterModule(name: string): Promise<boolean> {
    const module = this.modules.get(name);
    
    if (!module) {
      console.warn(`[Icarus] Module '${name}' not found`);
      return false;
    }
    
    // Call ThreadExit before unloading
    const state = this.states.get(name);
    if (state === ModuleState.RUNNING) {
      await this.executeThreadExit(name);
    }
    
    this.modules.delete(name);
    this.metadata.delete(name);
    this.states.set(name, ModuleState.UNLOADED);
    
    console.log(`[Icarus] Module '${name}' unregistered`);
    return true;
  }
  
  /**
   * Execute ThreadStart for a module
   */
  async executeThreadStart(name: string, args: unknown[] = []): Promise<ModuleExecutionResult> {
    const module = this.modules.get(name);
    
    if (!module) {
      return {
        success: false,
        returnCode: -1,
        executionTime: 0,
        error: parseFlare(new Error(`Module '${name}' not found`), 'HIGH')
      };
    }
    
    const startTime = Date.now();
    this.states.set(name, ModuleState.RUNNING);
    
    try {
      const returnCode = await module.ThreadStart(args);
      const executionTime = Date.now() - startTime;
      
      console.log(`[Icarus] Module '${name}' ThreadStart completed in ${executionTime}ms`);
      
      return {
        success: returnCode === 0,
        returnCode,
        executionTime
      };
    } catch (error) {
      this.states.set(name, ModuleState.ERROR);
      const executionTime = Date.now() - startTime;
      const flare = parseFlare(error as Error, 'HIGH');
      
      console.error(`[Icarus] Module '${name}' ThreadStart failed:`, (error as Error).message);
      
      // Call ErrorHandler
      try {
        await module.ErrorHandler(flare);
      } catch (handlerError) {
        console.error(`[Icarus] Module '${name}' ErrorHandler also failed:`, (handlerError as Error).message);
      }
      
      return {
        success: false,
        returnCode: -1,
        executionTime,
        error: flare
      };
    }
  }
  
  /**
   * Execute ThreadExit for a module
   */
  async executeThreadExit(name: string, args: unknown[] = []): Promise<ModuleExecutionResult> {
    const module = this.modules.get(name);
    
    if (!module) {
      return {
        success: false,
        returnCode: -1,
        executionTime: 0,
        error: parseFlare(new Error(`Module '${name}' not found`), 'HIGH')
      };
    }
    
    const startTime = Date.now();
    
    try {
      const returnCode = await module.ThreadExit(args);
      const executionTime = Date.now() - startTime;
      
      this.states.set(name, ModuleState.STOPPED);
      console.log(`[Icarus] Module '${name}' ThreadExit completed in ${executionTime}ms`);
      
      return {
        success: returnCode === 0,
        returnCode,
        executionTime
      };
    } catch (error) {
      this.states.set(name, ModuleState.ERROR);
      const executionTime = Date.now() - startTime;
      const flare = parseFlare(error as Error, 'HIGH');
      
      console.error(`[Icarus] Module '${name}' ThreadExit failed:`, (error as Error).message);
      
      return {
        success: false,
        returnCode: -1,
        executionTime,
        error: flare
      };
    }
  }
  
  /**
   * Get module by name
   */
  getModule(name: string): IcarusModule | undefined {
    return this.modules.get(name);
  }
  
  /**
   * Get module metadata
   */
  getMetadata(name: string): ModuleMetadata | undefined {
    return this.metadata.get(name);
  }
  
  /**
   * Get module state
   */
  getState(name: string): ModuleState {
    return this.states.get(name) || ModuleState.UNLOADED;
  }
  
  /**
   * Get all registered modules
   */
  getAllModules(): ModuleMetadata[] {
    return Array.from(this.metadata.values());
  }
  
  /**
   * Get all running modules
   */
  getRunningModules(): ModuleMetadata[] {
    return Array.from(this.metadata.entries())
      .filter(([name]) => this.states.get(name) === ModuleState.RUNNING)
      .map(([, meta]) => meta);
  }
}

// Singleton instance
export const moduleManager = new ModuleManager();
