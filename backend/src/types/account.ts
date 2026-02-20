/**
 * Account interface representing a user account
 */
export interface Account {
  id: string;
  username: string;
  email: string;
  password_hash: string;
  display_name: string | null;
  totp_secret: string | null;
  totp_enabled: boolean;
  is_banned: boolean;
  ban_reason: string | null;
  created_at: Date;
  updated_at: Date;
  last_login: Date | null;
}

/**
 * Public account data (without sensitive fields)
 */
export interface PublicAccount {
  id: string;
  username: string;
  email: string;
  display_name: string | null;
  totp_enabled: boolean;
  created_at: Date;
  last_login: Date | null;
}

/**
 * Session interface for user sessions
 */
export interface Session {
  id: string;
  account_id: string;
  token: string;
  ip_address: string | null;
  user_agent: string | null;
  client_version: string | null;
  os: string | null;
  expires_at: Date;
  created_at: Date;
  is_valid: boolean;
}

/**
 * Password reset token interface
 */
export interface PasswordResetToken {
  id: string;
  account_id: string;
  token: string;
  expires_at: Date;
  used: boolean;
  created_at: Date;
}

/**
 * Banned IP interface
 */
export interface BannedIP {
  id: string;
  ip_address: string;
  reason: string | null;
  banned_by: string | null;
  expires_at: Date | null;
  created_at: Date;
}

/**
 * Analytics event interface
 */
export interface AnalyticsEvent {
  id: string;
  account_id: string | null;
  ip_address: string | null;
  client_version: string | null;
  os: string | null;
  event_type: string;
  event_data: Record<string, unknown>;
  created_at: Date;
}

/**
 * Account creation input
 */
export interface CreateAccountInput {
  username: string;
  email: string;
  password: string;
  display_name?: string;
}

/**
 * Account update input
 */
export interface UpdateAccountInput {
  display_name?: string;
  email?: string;
}

/**
 * Login input
 */
export interface LoginInput {
  username: string;
  password: string;
  totp_code?: string;
}

/**
 * JWT payload
 */
export interface JWTPayload {
  sub: string;       // account_id
  username: string;
  iat: number;
  exp: number;
}

/**
 * Authentication result
 */
export interface AuthResult {
  success: boolean;
  token?: string;
  account?: PublicAccount;
  requires_2fa?: boolean;
  error?: string;
}
