/**
 * Module - Wrapper class for IcarusModule with lifecycle management
 */

import { Flare, parseFlare, FlareUtils } from '@trail-blaze/flare';
import {
  IcarusModule,
  ModuleMetadata,
  ModuleState,
  ModuleExecutionResult,
  ModuleConfig
} from './types';

/**
 * Default module configuration
 */
const DEFAULT_CONFIG: Required<ModuleConfig> = {
  timeout: 30000,      // 30 seconds
  autoRestart: false,
  maxRestarts: 3,
  verbose: false
};

/**
 * Module wrapper class
 * Provides lifecycle management and error handling for IcarusModule
 */
export class Module {
  private readonly module: IcarusModule;
  private readonly metadata: ModuleMetadata;
  private readonly config: Required<ModuleConfig>;
  
  private state: ModuleState = ModuleState.LOADED;
  private restartCount: number = 0;
  private lastError?: Flare;
  
  constructor(
    module: IcarusModule,
    path: string,
    isServerModule: boolean = false,
    config: ModuleConfig = {}
  ) {
    this.module = module;
    this.config = { ...DEFAULT_CONFIG, ...config };
    
    this.metadata = {
      name: module.name,
      version: module.version,
      description: module.description,
      author: module.author,
      path,
      isServerModule,
      loadedAt: new Date()
    };
    
    if (this.config.verbose) {
      console.log(`[Module] Loaded ${this.module.name} v${this.module.version}`);
    }
  }
  
  /**
   * Get module name
   */
  get name(): string {
    return this.module.name;
  }
  
  /**
   * Get module version
   */
  get version(): string {
    return this.module.version;
  }
  
  /**
   * Get current module state
   */
  getState(): ModuleState {
    return this.state;
  }
  
  /**
   * Get module metadata
   */
  getMetadata(): ModuleMetadata {
    return { ...this.metadata };
  }
  
  /**
   * Get last error (if any)
   */
  getLastError(): Flare | undefined {
    return this.lastError;
  }
  
  /**
   * Execute ThreadStart
   * @param args Arguments to pass to ThreadStart
   */
  async start(args: unknown[] = []): Promise<ModuleExecutionResult> {
    if (this.state === ModuleState.RUNNING) {
      return {
        success: false,
        returnCode: -1,
        executionTime: 0,
        error: parseFlare(new Error(`Module ${this.name} is already running`), 'MEDIUM')
      };
    }
    
    this.state = ModuleState.RUNNING;
    const startTime = Date.now();
    
    try {
      const returnCode = await this.executeWithTimeout(
        () => this.module.ThreadStart(args),
        this.config.timeout
      );
      
      const executionTime = Date.now() - startTime;
      
      if (this.config.verbose) {
        console.log(`[Module] ${this.name} ThreadStart completed in ${executionTime}ms (code: ${returnCode})`);
      }
      
      return {
        success: returnCode === 0,
        returnCode,
        executionTime
      };
    } catch (error) {
      return this.handleError(error as Error, Date.now() - startTime);
    }
  }
  
  /**
   * Execute ThreadExit
   * @param args Arguments to pass to ThreadExit
   */
  async stop(args: unknown[] = []): Promise<ModuleExecutionResult> {
    const startTime = Date.now();
    
    try {
      const returnCode = await this.executeWithTimeout(
        () => this.module.ThreadExit(args),
        this.config.timeout
      );
      
      this.state = ModuleState.STOPPED;
      const executionTime = Date.now() - startTime;
      
      if (this.config.verbose) {
        console.log(`[Module] ${this.name} ThreadExit completed in ${executionTime}ms (code: ${returnCode})`);
      }
      
      return {
        success: returnCode === 0,
        returnCode,
        executionTime
      };
    } catch (error) {
      return this.handleError(error as Error, Date.now() - startTime);
    }
  }
  
  /**
   * Execute with timeout
   */
  private async executeWithTimeout<T>(
    fn: () => T | Promise<T>,
    timeout: number
  ): Promise<T> {
    return new Promise<T>((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(`Execution timed out after ${timeout}ms`));
      }, timeout);
      
      Promise.resolve(fn())
        .then((result) => {
          clearTimeout(timer);
          resolve(result);
        })
        .catch((error) => {
          clearTimeout(timer);
          reject(error);
        });
    });
  }
  
  /**
   * Handle execution error
   */
  private async handleError(error: Error, executionTime: number): Promise<ModuleExecutionResult> {
    this.state = ModuleState.ERROR;
    const flare = parseFlare(error, 'HIGH');
    this.lastError = flare;
    
    if (this.config.verbose) {
      console.error(`[Module] ${this.name} error:`, error.message);
    }
    
    // Call ErrorHandler
    try {
      await this.module.ErrorHandler(flare);
    } catch (handlerError) {
      console.error(`[Module] ${this.name} ErrorHandler failed:`, (handlerError as Error).message);
    }
    
    // Handle auto-restart
    if (this.config.autoRestart && this.restartCount < this.config.maxRestarts) {
      this.restartCount++;
      console.log(`[Module] ${this.name} auto-restarting (attempt ${this.restartCount}/${this.config.maxRestarts})`);
      return this.start();
    }
    
    return {
      success: false,
      returnCode: -1,
      executionTime,
      error: flare
    };
  }
  
  /**
   * Reset restart counter
   */
  resetRestartCount(): void {
    this.restartCount = 0;
  }
  
  /**
   * Check if module is running
   */
  isRunning(): boolean {
    return this.state === ModuleState.RUNNING;
  }
  
  /**
   * Check if module has errored
   */
  hasError(): boolean {
    return this.state === ModuleState.ERROR;
  }
}
