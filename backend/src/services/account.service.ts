import bcrypt from 'bcrypt';
import { v4 as uuidv4 } from 'uuid';
import { pool } from '../config/database';
import config from '../config';
import { 
  Account, 
  PublicAccount, 
  CreateAccountInput, 
  UpdateAccountInput 
} from '../types/account';

/**
 * Account Service - Handles account CRUD operations
 */
export class AccountService {
  /**
   * Create a new account
   */
  async createAccount(input: CreateAccountInput): Promise<PublicAccount> {
    const { username, email, password, display_name } = input;
    
    // Hash password using bcrypt
    const password_hash = await bcrypt.hash(password, config.bcryptRounds);
    
    const result = await pool.query<Account>(
      `INSERT INTO accounts (username, email, password_hash, display_name)
       VALUES ($1, $2, $3, $4)
       RETURNING *`,
      [username.toLowerCase(), email.toLowerCase(), password_hash, display_name || username]
    );
    
    return this.toPublicAccount(result.rows[0]);
  }
  
  /**
   * Find account by ID
   */
  async findById(id: string): Promise<Account | null> {
    const result = await pool.query<Account>(
      'SELECT * FROM accounts WHERE id = $1',
      [id]
    );
    return result.rows[0] || null;
  }
  
  /**
   * Find account by username
   */
  async findByUsername(username: string): Promise<Account | null> {
    const result = await pool.query<Account>(
      'SELECT * FROM accounts WHERE username = $1',
      [username.toLowerCase()]
    );
    return result.rows[0] || null;
  }
  
  /**
   * Find account by email
   */
  async findByEmail(email: string): Promise<Account | null> {
    const result = await pool.query<Account>(
      'SELECT * FROM accounts WHERE email = $1',
      [email.toLowerCase()]
    );
    return result.rows[0] || null;
  }
  
  /**
   * Update account
   */
  async updateAccount(id: string, input: UpdateAccountInput): Promise<PublicAccount | null> {
    const updates: string[] = [];
    const values: (string | null)[] = [];
    let paramIndex = 1;
    
    if (input.display_name !== undefined) {
      updates.push(`display_name = $${paramIndex++}`);
      values.push(input.display_name);
    }
    
    if (input.email !== undefined) {
      updates.push(`email = $${paramIndex++}`);
      values.push(input.email.toLowerCase());
    }
    
    if (updates.length === 0) {
      return this.getPublicAccountById(id);
    }
    
    updates.push(`updated_at = CURRENT_TIMESTAMP`);
    values.push(id);
    
    const result = await pool.query<Account>(
      `UPDATE accounts SET ${updates.join(', ')} WHERE id = $${paramIndex} RETURNING *`,
      values
    );
    
    return result.rows[0] ? this.toPublicAccount(result.rows[0]) : null;
  }
  
  /**
   * Delete account
   */
  async deleteAccount(id: string): Promise<boolean> {
    const result = await pool.query(
      'DELETE FROM accounts WHERE id = $1',
      [id]
    );
    return (result.rowCount ?? 0) > 0;
  }
  
  /**
   * Update password
   */
  async updatePassword(id: string, newPassword: string): Promise<boolean> {
    const password_hash = await bcrypt.hash(newPassword, config.bcryptRounds);
    
    const result = await pool.query(
      `UPDATE accounts SET password_hash = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2`,
      [password_hash, id]
    );
    
    return (result.rowCount ?? 0) > 0;
  }
  
  /**
   * Verify password
   */
  async verifyPassword(account: Account, password: string): Promise<boolean> {
    return bcrypt.compare(password, account.password_hash);
  }
  
  /**
   * Update last login timestamp
   */
  async updateLastLogin(id: string): Promise<void> {
    await pool.query(
      'UPDATE accounts SET last_login = CURRENT_TIMESTAMP WHERE id = $1',
      [id]
    );
  }
  
  /**
   * Ban account
   */
  async banAccount(id: string, reason: string): Promise<boolean> {
    const result = await pool.query(
      `UPDATE accounts SET is_banned = TRUE, ban_reason = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2`,
      [reason, id]
    );
    return (result.rowCount ?? 0) > 0;
  }
  
  /**
   * Unban account
   */
  async unbanAccount(id: string): Promise<boolean> {
    const result = await pool.query(
      `UPDATE accounts SET is_banned = FALSE, ban_reason = NULL, updated_at = CURRENT_TIMESTAMP WHERE id = $1`,
      [id]
    );
    return (result.rowCount ?? 0) > 0;
  }
  
  /**
   * Get public account by ID
   */
  async getPublicAccountById(id: string): Promise<PublicAccount | null> {
    const account = await this.findById(id);
    return account ? this.toPublicAccount(account) : null;
  }
  
  /**
   * Convert account to public account (remove sensitive fields)
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
  
  /**
   * Create password reset token
   */
  async createPasswordResetToken(accountId: string): Promise<string> {
    const token = uuidv4();
    const expiresAt = new Date(Date.now() + 3600000); // 1 hour
    
    await pool.query(
      `INSERT INTO password_reset_tokens (account_id, token, expires_at)
       VALUES ($1, $2, $3)`,
      [accountId, token, expiresAt]
    );
    
    return token;
  }
  
  /**
   * Verify and use password reset token
   */
  async verifyPasswordResetToken(token: string): Promise<string | null> {
    const result = await pool.query(
      `SELECT account_id FROM password_reset_tokens 
       WHERE token = $1 AND used = FALSE AND expires_at > CURRENT_TIMESTAMP`,
      [token]
    );
    
    if (result.rows[0]) {
      await pool.query(
        'UPDATE password_reset_tokens SET used = TRUE WHERE token = $1',
        [token]
      );
      return result.rows[0].account_id;
    }
    
    return null;
  }
}

export const accountService = new AccountService();
