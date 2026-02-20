import { Request, Response, NextFunction } from 'express';
import { authService } from '../services';
import { Account } from '../types/account';

// Extend Express Request to include account
declare global {
  namespace Express {
    interface Request {
      account?: Account;
    }
  }
}

/**
 * Authentication middleware - verifies JWT token
 */
export async function authenticate(
  req: Request, 
  res: Response, 
  next: NextFunction
): Promise<void> {
  const authHeader = req.headers.authorization;
  
  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    res.status(401).json({ error: 'Authentication required' });
    return;
  }
  
  const token = authHeader.substring(7);
  
  try {
    const account = await authService.verifyToken(token);
    
    if (!account) {
      res.status(401).json({ error: 'Invalid or expired token' });
      return;
    }
    
    if (account.is_banned) {
      res.status(403).json({ error: `Account is banned: ${account.ban_reason || 'No reason provided'}` });
      return;
    }
    
    req.account = account;
    next();
  } catch {
    res.status(401).json({ error: 'Invalid token' });
  }
}

/**
 * Optional authentication middleware - attaches account if token is valid
 */
export async function optionalAuthenticate(
  req: Request, 
  res: Response, 
  next: NextFunction
): Promise<void> {
  const authHeader = req.headers.authorization;
  
  if (authHeader && authHeader.startsWith('Bearer ')) {
    const token = authHeader.substring(7);
    
    try {
      const account = await authService.verifyToken(token);
      if (account && !account.is_banned) {
        req.account = account;
      }
    } catch {
      // Token invalid, continue without account
    }
  }
  
  next();
}
