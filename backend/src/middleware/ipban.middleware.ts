import { Request, Response, NextFunction } from 'express';
import { pool } from '../config/database';

/**
 * IP Ban middleware - checks if the request IP is banned
 */
export async function checkIPBan(
  req: Request, 
  res: Response, 
  next: NextFunction
): Promise<void> {
  const ip = req.ip || req.socket.remoteAddress || '';
  
  try {
    const result = await pool.query(
      `SELECT reason, expires_at FROM banned_ips 
       WHERE ip_address = $1 AND (expires_at IS NULL OR expires_at > CURRENT_TIMESTAMP)`,
      [ip]
    );
    
    if (result.rows[0]) {
      const ban = result.rows[0];
      res.status(403).json({ 
        error: 'IP address is banned',
        reason: ban.reason,
        expires_at: ban.expires_at
      });
      return;
    }
    
    next();
  } catch {
    // On error, allow request to proceed
    next();
  }
}

/**
 * Ban an IP address
 */
export async function banIP(
  ipAddress: string, 
  reason?: string, 
  bannedBy?: string,
  expiresAt?: Date
): Promise<boolean> {
  try {
    await pool.query(
      `INSERT INTO banned_ips (ip_address, reason, banned_by, expires_at)
       VALUES ($1, $2, $3, $4)
       ON CONFLICT (ip_address) 
       DO UPDATE SET reason = $2, banned_by = $3, expires_at = $4`,
      [ipAddress, reason, bannedBy, expiresAt]
    );
    return true;
  } catch {
    return false;
  }
}

/**
 * Unban an IP address
 */
export async function unbanIP(ipAddress: string): Promise<boolean> {
  const result = await pool.query(
    'DELETE FROM banned_ips WHERE ip_address = $1',
    [ipAddress]
  );
  return (result.rowCount ?? 0) > 0;
}
