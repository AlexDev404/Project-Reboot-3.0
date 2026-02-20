/**
 * Flare severity levels
 * 
 * These indicate the severity of errors from the C/C++ binding layer:
 * - LOW: Minor issues that don't affect functionality
 * - MEDIUM: Issues that may affect some functionality
 * - HIGH: Serious errors that prevent operations from completing
 * - CRITICAL: Fatal errors that may require module restart
 */
export type FlareSeverity = 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';

/**
 * Native error codes from C++ bindings
 */
export enum NativeErrorCode {
  /** No error */
  SUCCESS = 0,
  /** Generic/unknown error */
  UNKNOWN = 1,
  /** Invalid argument passed to native function */
  INVALID_ARGUMENT = 2,
  /** Requested object/resource not found */
  NOT_FOUND = 3,
  /** Operation not permitted */
  PERMISSION_DENIED = 4,
  /** Native function not implemented */
  NOT_IMPLEMENTED = 5,
  /** Memory allocation failed */
  OUT_OF_MEMORY = 6,
  /** Operation timed out */
  TIMEOUT = 7,
  /** Resource is busy/locked */
  BUSY = 8,
  /** Network/connection error */
  NETWORK_ERROR = 9,
  /** Invalid game state for operation */
  INVALID_STATE = 10,
  /** Null pointer encountered */
  NULL_POINTER = 11,
  /** Array index out of bounds */
  INDEX_OUT_OF_BOUNDS = 12,
}

/**
 * Stack trace entry
 */
export interface FlareTrace {
  /** File path */
  file: string;
  /** Line number */
  line: number;
  /** Column number */
  column: number;
}

/**
 * Attention block containing trace information
 */
export interface FlareAttention {
  /** Array of stack trace entries */
  trace: FlareTrace[];
}

/**
 * Flare - Structured error object
 * 
 * @example
 * ```json
 * {
 *   "spark_id": "IANCBAA4xnQDOyUEUIQA7gAUiCASny5R+AJ4D2AI0L4",
 *   "severity": "HIGH",
 *   "attention": {
 *     "trace": [
 *       { "file": "path/to/file.ts", "line": 23, "column": 51 }
 *     ]
 *   },
 *   "did_you_know": "Syntax Error."
 * }
 * ```
 */
export interface Flare {
  /** Unique error ID stored in the database */
  spark_id: string;
  /** Error severity level */
  severity: FlareSeverity;
  /** Stack trace information */
  attention: FlareAttention;
  /** Human-readable error description */
  did_you_know: string;
}

/**
 * Options for creating a Flare
 */
export interface FlareOptions {
  /** Override default severity */
  severity?: FlareSeverity;
  /** Custom spark_id (auto-generated if not provided) */
  sparkId?: string;
  /** Additional context data */
  context?: Record<string, unknown>;
}

/**
 * Native binding error - thrown by C++ layer and converted to Flare
 */
export interface NativeBindingError {
  /** Native error code */
  code: NativeErrorCode;
  /** Native function that failed */
  function: string;
  /** Error message from C++ */
  message: string;
  /** Additional native context */
  nativeTrace?: string;
}
