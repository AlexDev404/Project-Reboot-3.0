import { Flare, FlareSeverity, FlareTrace } from './types';

/**
 * FlareUtils - Utility class for working with Flare objects
 * 
 * Provides convenient methods for extracting information from Flare errors.
 * 
 * @example
 * ```ts
 * import { FlareUtils, parseFlare } from "@trail-blaze/flare";
 * 
 * const flare = parseFlare(new Error("Test error"));
 * const utils = FlareUtils.parse(flare);
 * 
 * console.log(utils.getReason());     // "Test error"
 * console.log(utils.getSeverity());   // "MEDIUM"
 * console.log(utils.getSparkId());    // "ABC123..."
 * ```
 */
export class FlareUtils {
  private readonly flare: Flare;
  
  /**
   * Create FlareUtils instance
   * @param flare Flare object to wrap
   */
  constructor(flare: Flare) {
    this.flare = flare;
  }
  
  /**
   * Static factory method (matches pattern from spec)
   * @param flare Flare object
   * @returns FlareUtils instance
   * 
   * @example
   * ```ts
   * FlareUtils.parse(flare).getReason();
   * ```
   */
  static parse(flare: Flare): FlareUtils {
    return new FlareUtils(flare);
  }
  
  /**
   * Get the error reason/message
   * @returns The did_you_know field
   */
  getReason(): string {
    return this.flare.did_you_know;
  }
  
  /**
   * Get error severity
   * @returns Severity level
   */
  getSeverity(): FlareSeverity {
    return this.flare.severity;
  }
  
  /**
   * Get the spark ID
   * @returns Unique error identifier
   */
  getSparkId(): string {
    return this.flare.spark_id;
  }
  
  /**
   * Get the stack trace
   * @returns Array of trace entries
   */
  getTrace(): FlareTrace[] {
    return this.flare.attention.trace;
  }
  
  /**
   * Get raw Flare object
   * @returns The underlying Flare
   */
  getFlare(): Flare {
    return this.flare;
  }
  
  /**
   * Format trace as string
   * @returns Human-readable stack trace
   */
  formatTrace(): string {
    return this.flare.attention.trace
      .map(t => `  at ${t.file}:${t.line}:${t.column}`)
      .join('\n');
  }
  
  /**
   * Get formatted error string
   * @returns Full error description with trace
   */
  format(): string {
    const lines: string[] = [
      `[${this.flare.severity}] ${this.flare.did_you_know}`,
      `Spark ID: ${this.flare.spark_id}`,
    ];
    
    if (this.flare.attention.trace.length > 0) {
      lines.push('Stack trace:');
      lines.push(this.formatTrace());
    }
    
    return lines.join('\n');
  }
  
  /**
   * Check if severity is at or above a threshold
   * @param threshold Minimum severity to check
   * @returns True if severity meets or exceeds threshold
   */
  isSeverityAtLeast(threshold: FlareSeverity): boolean {
    const levels: Record<FlareSeverity, number> = {
      'LOW': 0,
      'MEDIUM': 1,
      'HIGH': 2,
      'CRITICAL': 3
    };
    
    return levels[this.flare.severity] >= levels[threshold];
  }
  
  /**
   * Check if this is a critical error
   */
  isCritical(): boolean {
    return this.flare.severity === 'CRITICAL';
  }
  
  /**
   * Convert to JSON string
   * @param pretty Pretty-print if true
   * @returns JSON string
   */
  toJSON(pretty: boolean = true): string {
    return JSON.stringify(this.flare, null, pretty ? 2 : undefined);
  }
  
  /**
   * Log the flare to console with appropriate level
   */
  log(): void {
    const formatted = this.format();
    
    switch (this.flare.severity) {
      case 'CRITICAL':
      case 'HIGH':
        console.error(formatted);
        break;
      case 'MEDIUM':
        console.warn(formatted);
        break;
      case 'LOW':
        console.log(formatted);
        break;
    }
  }
}
