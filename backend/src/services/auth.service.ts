import jwt from 'jsonwebtoken';
import { v4 as uuidv4 } from 'uuid';
import { pool } from '../config/database';
import config from '../config';
import { accountService } from './account.service';
import { totpService } from './totp.service';
import { 
  Account, 
  PublicAccount, 
  LoginInput, 
  AuthResult, 
  JWTPayload,
  Session
} from '../types/account';

/**
 * Authentication Service - Handles login, logout, and session management
 */
export class AuthService {
  /**
   * Authenticate user with credentials and optional 2FA
   */
  async login(input: LoginInput, ipAddress?: string, userAgent?: string): Promise<AuthResult> {
    const { username, password, totp_code } = input;
    
    // Find account by username
    const account = await accountService.findByUsername(username);
    
    if (!account) {
      return { success: false, error: 'Invalid username or password' };
    }
    
    // Check if account is banned
    if (account.is_banned) {
      return { success: false, error: `Account is banned: ${account.ban_reason || 'No reason provided'}` };
    }
    
    // Verify password
    const isValidPassword = await accountService.verifyPassword(account, password);
    
    if (!isValidPassword) {
      return { success: false, error: 'Invalid username or password' };
    }
    
    // Check 2FA if enabled
    if (account.totp_enabled) {
      if (!totp_code) {
        return { success: false, requires_2fa: true, error: '2FA code required' };
      }
      
      const isValid2FA = await totpService.verify2FA(account, totp_code);
      
      if (!isValid2FA) {
        return { success: false, error: 'Invalid 2FA code' };
      }
    }
    
    // Generate JWT token
    const token = this.generateToken(account);
    
    // Create session
    await this.createSession(account.id, token, ipAddress, userAgent);
    
    // Update last login
    await accountService.updateLastLogin(account.id);
    
    return {
      success: true,
      token,
      account: this.toPublicAccount(account)
    };
  }
  
  /**
   * Logout - Invalidate session
   */
  async logout(token: string): Promise<boolean> {
    const result = await pool.query(
      'UPDATE sessions SET is_valid = FALSE WHERE token = $1',
      [token]
    );
    return (result.rowCount ?? 0) > 0;
  }
  
  /**
   * Logout all sessions for an account
   */
  async logoutAll(accountId: string): Promise<number> {
    const result = await pool.query(
      'UPDATE sessions SET is_valid = FALSE WHERE account_id = $1 AND is_valid = TRUE',
      [accountId]
    );
    return result.rowCount ?? 0;
  }
  
  /**
   * Verify JWT token and return account
   */
  async verifyToken(token: string): Promise<Account | null> {
    try {
      const payload = jwt.verify(token, config.jwt.secret) as JWTPayload;
      
      // Check if session is valid
      const sessionResult = await pool.query(
        'SELECT is_valid, expires_at FROM sessions WHERE token = $1',
        [token]
      );
      
      const session = sessionResult.rows[0];
      
      if (!session || !session.is_valid || new Date(session.expires_at) < new Date()) {
        return null;
      }
      
      return accountService.findById(payload.sub);
    } catch {
      return null;
    }
  }
  
  /**
   * Generate JWT token for account
   */
  private generateToken(account: Account): string {
    const payload: Omit<JWTPayload, 'iat' | 'exp'> = {
      sub: account.id,
      username: account.username
    };
    
    return jwt.sign(payload, config.jwt.secret, {
      expiresIn: config.jwt.expiresIn
    });
  }
  
  /**
   * Create a new session
   */
  private async createSession(
    accountId: string, 
    token: string, 
    ipAddress?: string, 
    userAgent?: string
  ): Promise<Session> {
    // Parse expiration from JWT config
    const expiresIn = config.jwt.expiresIn;
    let expiresMs = 24 * 60 * 60 * 1000; // Default 24h
    
    if (expiresIn.endsWith('h')) {
      expiresMs = parseInt(expiresIn) * 60 * 60 * 1000;
    } else if (expiresIn.endsWith('d')) {
      expiresMs = parseInt(expiresIn) * 24 * 60 * 60 * 1000;
    } else if (expiresIn.endsWith('m')) {
      expiresMs = parseInt(expiresIn) * 60 * 1000;
    } else if (expiresIn.endsWith('s')) {
      expiresMs = parseInt(expiresIn) * 1000;
    } else if (/^\d+$/.test(expiresIn)) {
      // If just a number, assume seconds (JWT default)
      expiresMs = parseInt(expiresIn) * 1000;
    }
    
    const expiresAt = new Date(Date.now() + expiresMs);
    
    const result = await pool.query<Session>(
      `INSERT INTO sessions (account_id, token, ip_address, user_agent, expires_at)
       VALUES ($1, $2, $3, $4, $5)
       RETURNING *`,
      [accountId, token, ipAddress, userAgent, expiresAt]
    );
    
    return result.rows[0];
  }
  
  /**
   * Get active sessions for an account
   */
  async getActiveSessions(accountId: string): Promise<Session[]> {
    const result = await pool.query<Session>(
      `SELECT * FROM sessions 
       WHERE account_id = $1 AND is_valid = TRUE AND expires_at > CURRENT_TIMESTAMP
       ORDER BY created_at DESC`,
      [accountId]
    );
    return result.rows;
  }
  
  /**
   * Convert Account to PublicAccount
   */
  private toPublicAccount(account: Account): PublicAccount {
    return {
      id: account.id,
      username: account.username,
      email: account.email,
      display_name: account.display_name,
      totp_enabled: account.totp_enabled,
      created_at: account.created_at,
      last_login: account.last_login
    };
  }
}

export const authService = new AuthService();
