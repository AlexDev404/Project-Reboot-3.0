/**
 * Flare - Error handling system for Icarus modules
 * 
 * Flare provides structured error information including:
 * - spark_id: Unique error identifier stored in database
 * - severity: Error severity level (LOW, MEDIUM, HIGH, CRITICAL)
 * - attention: Stack trace information
 * - did_you_know: Human-readable error description
 */

export type FlareSeverity = 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';

export interface FlareTrace {
  file: string;
  line: number;
  column: number;
}

export interface FlareAttention {
  trace: FlareTrace[];
}

export interface Flare {
  spark_id: string;
  severity: FlareSeverity;
  attention: FlareAttention;
  did_you_know: string;
}

/**
 * Parse error information into a Flare object
 */
export function parseFlare(error: Error, severity: FlareSeverity = 'MEDIUM'): Flare {
  const traces: FlareTrace[] = [];
  
  if (error.stack) {
    const stackLines = error.stack.split('\n').slice(1);
    for (const line of stackLines) {
      const match = line.match(/at\s+.*\s+\((.+):(\d+):(\d+)\)/);
      if (match) {
        traces.push({
          file: match[1],
          line: parseInt(match[2], 10),
          column: parseInt(match[3], 10)
        });
      }
    }
  }
  
  return {
    spark_id: generateSparkId(),
    severity,
    attention: { trace: traces },
    did_you_know: error.message
  };
}

/**
 * Generate a unique spark ID for error tracking
 */
function generateSparkId(): string {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  for (let i = 0; i < 43; i++) {
    result += chars.charAt(Math.floor(Math.random() * chars.length));
  }
  return result;
}

/**
 * Flare utility class for working with Flare objects
 */
export class FlareUtils {
  private flare: Flare;
  
  constructor(flare: Flare) {
    this.flare = flare;
  }
  
  static parse(flare: Flare): FlareUtils {
    return new FlareUtils(flare);
  }
  
  getReason(): string {
    return this.flare.did_you_know;
  }
  
  getSeverity(): FlareSeverity {
    return this.flare.severity;
  }
  
  getSparkId(): string {
    return this.flare.spark_id;
  }
  
  getTrace(): FlareTrace[] {
    return this.flare.attention.trace;
  }
  
  /**
   * Check if this is a critical error
   */
  isCritical(): boolean {
    return this.flare.severity === 'CRITICAL';
  }
  
  toJSON(): string {
    return JSON.stringify(this.flare, null, 2);
  }
}
