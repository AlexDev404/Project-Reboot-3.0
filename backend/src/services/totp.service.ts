import { authenticator } from 'otplib';
import crypto from 'crypto';
import { pool } from '../config/database';
import config from '../config';
import { Account } from '../types/account';

/**
 * TOTP Service - Handles Two-Factor Authentication
 */
export class TOTPService {
  constructor() {
    // Configure TOTP options
    authenticator.options = {
      window: 1, // Allow 1 step before/after for clock drift
      step: 30   // 30-second time step
    };
  }
  
  /**
   * Generate a new TOTP secret for an account
   */
  generateSecret(): string {
    return authenticator.generateSecret();
  }
  
  /**
   * Generate the otpauth:// URI for QR code generation
   */
  generateKeyUri(username: string, secret: string): string {
    return authenticator.keyuri(username, config.totp.issuer, secret);
  }
  
  /**
   * Verify a TOTP code
   */
  verifyCode(secret: string, code: string): boolean {
    try {
      return authenticator.verify({ token: code, secret });
    } catch {
      return false;
    }
  }
  
  /**
   * Enable 2FA for an account (store secret and set flag)
   */
  async enable2FA(accountId: string, secret: string): Promise<boolean> {
    const result = await pool.query(
      `UPDATE accounts 
       SET totp_secret = $1, totp_enabled = TRUE, updated_at = CURRENT_TIMESTAMP 
       WHERE id = $2`,
      [secret, accountId]
    );
    return (result.rowCount ?? 0) > 0;
  }
  
  /**
   * Disable 2FA for an account
   */
  async disable2FA(accountId: string): Promise<boolean> {
    const result = await pool.query(
      `UPDATE accounts 
       SET totp_secret = NULL, totp_enabled = FALSE, updated_at = CURRENT_TIMESTAMP 
       WHERE id = $1`,
      [accountId]
    );
    return (result.rowCount ?? 0) > 0;
  }
  
  /**
   * Check if account has 2FA enabled
   */
  async is2FAEnabled(accountId: string): Promise<boolean> {
    const result = await pool.query(
      'SELECT totp_enabled FROM accounts WHERE id = $1',
      [accountId]
    );
    return result.rows[0]?.totp_enabled ?? false;
  }
  
  /**
   * Verify 2FA code for an account
   */
  async verify2FA(account: Account, code: string): Promise<boolean> {
    if (!account.totp_enabled || !account.totp_secret) {
      return true; // 2FA not enabled, consider it verified
    }
    
    return this.verifyCode(account.totp_secret, code);
  }
  
  /**
   * Generate backup codes (one-time use recovery codes)
   * Uses cryptographically secure random number generation
   */
  generateBackupCodes(count: number = 10): string[] {
    const codes: string[] = [];
    for (let i = 0; i < count; i++) {
      // Generate 8 bytes of cryptographically secure random data
      const randomBytes = crypto.randomBytes(6);
      // Convert to alphanumeric string
      const code = randomBytes.toString('base64').replace(/[+/=]/g, '').substring(0, 8).toUpperCase();
      codes.push(code);
    }
    return codes;
  }
}

export const totpService = new TOTPService();
