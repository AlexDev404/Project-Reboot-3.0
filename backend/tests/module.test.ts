import { moduleManager, ModuleManager } from '../src/modules';
import { IcarusModule, ModuleState } from '../src/types/module';
import { Flare } from '../src/types/flare';

describe('Module Manager', () => {
  let manager: ModuleManager;
  
  beforeEach(() => {
    manager = new ModuleManager();
  });
  
  const createTestModule = (name: string, version: string = '1.0.0'): IcarusModule => ({
    name,
    version,
    description: 'Test module',
    ThreadStart: jest.fn().mockResolvedValue(0),
    ThreadExit: jest.fn().mockResolvedValue(0),
    ErrorHandler: jest.fn().mockResolvedValue(1)
  });
  
  describe('registerModule', () => {
    it('should register a module', () => {
      const module = createTestModule('test-module');
      manager.registerModule(module);
      
      expect(manager.getModule('test-module')).toBe(module);
      expect(manager.getState('test-module')).toBe(ModuleState.LOADED);
    });
    
    it('should store module metadata', () => {
      const module = createTestModule('test-module', '2.0.0');
      manager.registerModule(module, true, '/path/to/module');
      
      const metadata = manager.getMetadata('test-module');
      expect(metadata).toBeDefined();
      expect(metadata?.name).toBe('test-module');
      expect(metadata?.version).toBe('2.0.0');
      expect(metadata?.isServerModule).toBe(true);
      expect(metadata?.path).toBe('/path/to/module');
    });
    
    it('should allow overwriting existing modules', () => {
      const module1 = createTestModule('test-module', '1.0.0');
      const module2 = createTestModule('test-module', '2.0.0');
      
      manager.registerModule(module1);
      manager.registerModule(module2);
      
      expect(manager.getModule('test-module')).toBe(module2);
    });
  });
  
  describe('executeThreadStart', () => {
    it('should execute ThreadStart and return result', async () => {
      const module = createTestModule('test-module');
      manager.registerModule(module);
      
      const result = await manager.executeThreadStart('test-module', ['arg1', 'arg2']);
      
      expect(result.success).toBe(true);
      expect(result.returnCode).toBe(0);
      expect(module.ThreadStart).toHaveBeenCalledWith(['arg1', 'arg2']);
      expect(manager.getState('test-module')).toBe(ModuleState.RUNNING);
    });
    
    it('should handle ThreadStart errors', async () => {
      const module = createTestModule('test-module');
      (module.ThreadStart as jest.Mock).mockRejectedValue(new Error('Test error'));
      manager.registerModule(module);
      
      const result = await manager.executeThreadStart('test-module');
      
      expect(result.success).toBe(false);
      expect(result.returnCode).toBe(-1);
      expect(result.error).toBeDefined();
      expect(result.error?.did_you_know).toBe('Test error');
      expect(module.ErrorHandler).toHaveBeenCalled();
      expect(manager.getState('test-module')).toBe(ModuleState.ERROR);
    });
    
    it('should return error for non-existent module', async () => {
      const result = await manager.executeThreadStart('non-existent');
      
      expect(result.success).toBe(false);
      expect(result.error?.did_you_know).toContain('not found');
    });
  });
  
  describe('executeThreadExit', () => {
    it('should execute ThreadExit and return result', async () => {
      const module = createTestModule('test-module');
      manager.registerModule(module);
      
      const result = await manager.executeThreadExit('test-module', ['cleanup']);
      
      expect(result.success).toBe(true);
      expect(result.returnCode).toBe(0);
      expect(module.ThreadExit).toHaveBeenCalledWith(['cleanup']);
      expect(manager.getState('test-module')).toBe(ModuleState.STOPPED);
    });
  });
  
  describe('getAllModules', () => {
    it('should return all registered modules', () => {
      manager.registerModule(createTestModule('module1'));
      manager.registerModule(createTestModule('module2'));
      manager.registerModule(createTestModule('module3'));
      
      const modules = manager.getAllModules();
      expect(modules).toHaveLength(3);
      expect(modules.map(m => m.name)).toContain('module1');
      expect(modules.map(m => m.name)).toContain('module2');
      expect(modules.map(m => m.name)).toContain('module3');
    });
  });
  
  describe('unregisterModule', () => {
    it('should unregister a module', async () => {
      const module = createTestModule('test-module');
      manager.registerModule(module);
      
      const result = await manager.unregisterModule('test-module');
      
      expect(result).toBe(true);
      expect(manager.getModule('test-module')).toBeUndefined();
      expect(manager.getState('test-module')).toBe(ModuleState.UNLOADED);
    });
    
    it('should call ThreadExit for running modules', async () => {
      const module = createTestModule('test-module');
      manager.registerModule(module);
      await manager.executeThreadStart('test-module');
      
      await manager.unregisterModule('test-module');
      
      expect(module.ThreadExit).toHaveBeenCalled();
    });
    
    it('should return false for non-existent module', async () => {
      const result = await manager.unregisterModule('non-existent');
      expect(result).toBe(false);
    });
  });
});
