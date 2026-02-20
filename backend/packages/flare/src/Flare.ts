import { Flare, FlareSeverity, FlareTrace, FlareOptions } from './types';

/**
 * Characters used for spark_id generation
 */
const SPARK_ID_CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
const SPARK_ID_LENGTH = 43;

/**
 * Generate a unique spark ID for error tracking
 * @returns 43-character unique identifier
 */
export function generateSparkId(): string {
  let result = '';
  for (let i = 0; i < SPARK_ID_LENGTH; i++) {
    result += SPARK_ID_CHARS.charAt(Math.floor(Math.random() * SPARK_ID_CHARS.length));
  }
  return result;
}

/**
 * Parse stack trace string into FlareTrace array
 * @param stack Error stack trace string
 * @returns Array of trace entries
 */
export function parseStackTrace(stack?: string): FlareTrace[] {
  const traces: FlareTrace[] = [];
  
  if (!stack) return traces;
  
  const stackLines = stack.split('\n').slice(1); // Skip first line (error message)
  
  for (const line of stackLines) {
    // Match patterns like:
    // "    at functionName (file.ts:10:5)"
    // "    at file.ts:10:5"
    // "    at Object.<anonymous> (/path/to/file.ts:10:5)"
    const match = line.match(/at\s+(?:.*?\s+)?[\(]?(.+?):(\d+):(\d+)[\)]?$/);
    
    if (match) {
      traces.push({
        file: match[1],
        line: parseInt(match[2], 10),
        column: parseInt(match[3], 10)
      });
    }
  }
  
  return traces;
}

/**
 * Parse an Error object into a Flare
 * @param error The error to parse
 * @param severity Severity level (default: MEDIUM)
 * @returns Flare object
 * 
 * @example
 * ```ts
 * try {
 *   throw new Error("Something went wrong");
 * } catch (error) {
 *   const flare = parseFlare(error as Error, "HIGH");
 *   console.log(flare.spark_id);
 *   console.log(flare.did_you_know); // "Something went wrong"
 * }
 * ```
 */
export function parseFlare(error: Error, severity: FlareSeverity = 'MEDIUM'): Flare {
  return {
    spark_id: generateSparkId(),
    severity,
    attention: {
      trace: parseStackTrace(error.stack)
    },
    did_you_know: error.message
  };
}

/**
 * Create a Flare from scratch
 * @param message Error message
 * @param options Flare options
 * @returns Flare object
 * 
 * @example
 * ```ts
 * const flare = createFlare("Invalid configuration", { severity: "HIGH" });
 * ```
 */
export function createFlare(message: string, options: FlareOptions = {}): Flare {
  // Capture current stack trace
  const stackError = new Error();
  const traces = parseStackTrace(stackError.stack).slice(1); // Skip createFlare frame
  
  return {
    spark_id: options.sparkId || generateSparkId(),
    severity: options.severity || 'MEDIUM',
    attention: { trace: traces },
    did_you_know: message
  };
}

/**
 * Serialize Flare to JSON string
 * @param flare Flare object
 * @param pretty Whether to pretty-print (default: false)
 * @returns JSON string
 */
export function serializeFlare(flare: Flare, pretty: boolean = false): string {
  return JSON.stringify(flare, null, pretty ? 2 : undefined);
}

/**
 * Deserialize JSON string to Flare
 * @param json JSON string
 * @returns Flare object
 * @throws Error if JSON is invalid
 */
export function deserializeFlare(json: string): Flare {
  const obj = JSON.parse(json);
  
  // Validate required fields
  if (!obj.spark_id || !obj.severity || !obj.attention || !obj.did_you_know) {
    throw new Error('Invalid Flare JSON: missing required fields');
  }
  
  return obj as Flare;
}

// Re-export types
export { Flare, FlareSeverity, FlareTrace, FlareAttention, FlareOptions } from './types';
