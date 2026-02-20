import { parseFlare, FlareUtils } from '../src/types/flare';
import type { Flare } from '../src/types/flare';

describe('Flare Error Handling', () => {
  describe('parseFlare', () => {
    it('should parse an error into a Flare object', () => {
      const error = new Error('Test error message');
      const flare = parseFlare(error);
      
      expect(flare).toHaveProperty('spark_id');
      expect(flare.spark_id).toHaveLength(43);
      expect(flare.severity).toBe('MEDIUM');
      expect(flare.did_you_know).toBe('Test error message');
      expect(flare.attention).toHaveProperty('trace');
      expect(Array.isArray(flare.attention.trace)).toBe(true);
    });
    
    it('should use custom severity', () => {
      const error = new Error('Critical error');
      const flare = parseFlare(error, 'CRITICAL');
      
      expect(flare.severity).toBe('CRITICAL');
    });
    
    it('should generate unique spark_ids', () => {
      const error1 = new Error('Error 1');
      const error2 = new Error('Error 2');
      
      const flare1 = parseFlare(error1);
      const flare2 = parseFlare(error2);
      
      expect(flare1.spark_id).not.toBe(flare2.spark_id);
    });
  });
  
  describe('FlareUtils', () => {
    const testFlare: Flare = {
      spark_id: 'TEST123456789012345678901234567890123456789',
      severity: 'HIGH',
      attention: {
        trace: [
          { file: '/test/file.ts', line: 10, column: 5 },
          { file: '/test/other.ts', line: 20, column: 10 }
        ]
      },
      did_you_know: 'Test error explanation'
    };
    
    it('should get reason from Flare', () => {
      const utils = FlareUtils.parse(testFlare);
      expect(utils.getReason()).toBe('Test error explanation');
    });
    
    it('should get severity from Flare', () => {
      const utils = FlareUtils.parse(testFlare);
      expect(utils.getSeverity()).toBe('HIGH');
    });
    
    it('should get spark_id from Flare', () => {
      const utils = FlareUtils.parse(testFlare);
      expect(utils.getSparkId()).toBe(testFlare.spark_id);
    });
    
    it('should get trace from Flare', () => {
      const utils = FlareUtils.parse(testFlare);
      const trace = utils.getTrace();
      
      expect(trace).toHaveLength(2);
      expect(trace[0].file).toBe('/test/file.ts');
      expect(trace[0].line).toBe(10);
    });
    
    it('should convert Flare to JSON', () => {
      const utils = FlareUtils.parse(testFlare);
      const json = utils.toJSON();
      
      expect(typeof json).toBe('string');
      const parsed = JSON.parse(json);
      expect(parsed.spark_id).toBe(testFlare.spark_id);
    });
  });
  
  describe('Flare in ErrorHandler context', () => {
    it('should be usable in ErrorHandler pattern', () => {
      // Simulate how Flare is used in a module's ErrorHandler
      const mockErrorHandler = (error: Flare): number => {
        const utils = FlareUtils.parse(error);
        
        // Log the error
        const reason = utils.getReason();
        const severity = utils.getSeverity();
        
        // Check severity and decide action
        if (utils.isCritical()) {
          return 1; // Critical - abort
        }
        
        return 0; // Recovered
      };
      
      const testFlare: Flare = {
        spark_id: 'TEST123456789012345678901234567890123456789',
        severity: 'MEDIUM',
        attention: { trace: [] },
        did_you_know: 'Non-critical error'
      };
      
      const result = mockErrorHandler(testFlare);
      expect(result).toBe(0); // Should recover from non-critical
      
      const criticalFlare: Flare = {
        ...testFlare,
        severity: 'CRITICAL'
      };
      
      const criticalResult = mockErrorHandler(criticalFlare);
      expect(criticalResult).toBe(1); // Should abort on critical
    });
  });
});
