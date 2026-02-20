import { Router, Request, Response } from 'express';
import { authService, accountService } from '../services';
import { authenticate, strictRateLimit, authRateLimit } from '../middleware';

const router = Router();

/**
 * POST /auth/register
 * Register a new account
 */
router.post('/register', authRateLimit, async (req: Request, res: Response) => {
  try {
    const { username, email, password, display_name } = req.body;
    
    // Validate required fields
    if (!username || !email || !password) {
      res.status(400).json({ error: 'Username, email, and password are required' });
      return;
    }
    
    // Validate username format
    if (!/^[a-zA-Z0-9_]{3,20}$/.test(username)) {
      res.status(400).json({ 
        error: 'Username must be 3-20 characters and contain only letters, numbers, and underscores' 
      });
      return;
    }
    
    // Validate email format
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
      res.status(400).json({ error: 'Invalid email format' });
      return;
    }
    
    // Validate password strength
    if (password.length < 8) {
      res.status(400).json({ error: 'Password must be at least 8 characters' });
      return;
    }
    
    // Check if username exists
    const existingUsername = await accountService.findByUsername(username);
    if (existingUsername) {
      res.status(409).json({ error: 'Username already exists' });
      return;
    }
    
    // Check if email exists
    const existingEmail = await accountService.findByEmail(email);
    if (existingEmail) {
      res.status(409).json({ error: 'Email already exists' });
      return;
    }
    
    const account = await accountService.createAccount({
      username,
      email,
      password,
      display_name
    });
    
    res.status(201).json({
      message: 'Account created successfully',
      account
    });
  } catch (error) {
    console.error('[Icarus] Registration error:', error);
    res.status(500).json({ error: 'Failed to create account' });
  }
});

/**
 * POST /auth/login
 * Login with credentials
 */
router.post('/login', strictRateLimit, async (req: Request, res: Response) => {
  try {
    const { username, password, totp_code } = req.body;
    
    if (!username || !password) {
      res.status(400).json({ error: 'Username and password are required' });
      return;
    }
    
    const ipAddress = req.ip || req.socket.remoteAddress;
    const userAgent = req.headers['user-agent'];
    
    const result = await authService.login(
      { username, password, totp_code },
      ipAddress,
      userAgent
    );
    
    if (!result.success) {
      const statusCode = result.requires_2fa ? 428 : 401;
      res.status(statusCode).json({
        error: result.error,
        requires_2fa: result.requires_2fa
      });
      return;
    }
    
    res.json({
      message: 'Login successful',
      token: result.token,
      account: result.account
    });
  } catch (error) {
    console.error('[Icarus] Login error:', error);
    res.status(500).json({ error: 'Login failed' });
  }
});

/**
 * POST /auth/logout
 * Logout current session
 */
router.post('/logout', authenticate, async (req: Request, res: Response) => {
  try {
    const authHeader = req.headers.authorization;
    const token = authHeader?.substring(7);
    
    if (token) {
      await authService.logout(token);
    }
    
    res.json({ message: 'Logged out successfully' });
  } catch (error) {
    console.error('[Icarus] Logout error:', error);
    res.status(500).json({ error: 'Logout failed' });
  }
});

/**
 * POST /auth/logout-all
 * Logout all sessions
 */
router.post('/logout-all', authenticate, async (req: Request, res: Response) => {
  try {
    const count = await authService.logoutAll(req.account!.id);
    res.json({ 
      message: 'All sessions logged out',
      sessions_invalidated: count
    });
  } catch (error) {
    console.error('[Icarus] Logout all error:', error);
    res.status(500).json({ error: 'Failed to logout all sessions' });
  }
});

/**
 * GET /auth/sessions
 * Get active sessions
 */
router.get('/sessions', authenticate, async (req: Request, res: Response) => {
  try {
    const sessions = await authService.getActiveSessions(req.account!.id);
    
    // Remove sensitive token data
    const sanitizedSessions = sessions.map(session => ({
      id: session.id,
      ip_address: session.ip_address,
      user_agent: session.user_agent,
      created_at: session.created_at,
      expires_at: session.expires_at
    }));
    
    res.json({ sessions: sanitizedSessions });
  } catch (error) {
    console.error('[Icarus] Get sessions error:', error);
    res.status(500).json({ error: 'Failed to get sessions' });
  }
});

/**
 * POST /auth/password-reset-request
 * Request password reset token
 */
router.post('/password-reset-request', strictRateLimit, async (req: Request, res: Response) => {
  try {
    const { email } = req.body;
    
    if (!email) {
      res.status(400).json({ error: 'Email is required' });
      return;
    }
    
    const account = await accountService.findByEmail(email);
    
    // Always return success to prevent email enumeration
    if (account) {
      const token = await accountService.createPasswordResetToken(account.id);
      // TODO: Send this token via email in production
      // Do NOT log tokens in production - this is for development only
      if (process.env.NODE_ENV === 'development') {
        console.log(`[Icarus] Password reset token for ${email}: ${token}`);
      }
    }
    
    res.json({ message: 'If the email exists, a reset link will be sent' });
  } catch (error) {
    console.error('[Icarus] Password reset request error:', error);
    res.status(500).json({ error: 'Failed to process password reset request' });
  }
});

/**
 * POST /auth/password-reset
 * Reset password with token
 */
router.post('/password-reset', strictRateLimit, async (req: Request, res: Response) => {
  try {
    const { token, new_password } = req.body;
    
    if (!token || !new_password) {
      res.status(400).json({ error: 'Token and new password are required' });
      return;
    }
    
    if (new_password.length < 8) {
      res.status(400).json({ error: 'Password must be at least 8 characters' });
      return;
    }
    
    const accountId = await accountService.verifyPasswordResetToken(token);
    
    if (!accountId) {
      res.status(400).json({ error: 'Invalid or expired reset token' });
      return;
    }
    
    await accountService.updatePassword(accountId, new_password);
    
    // Invalidate all existing sessions
    await authService.logoutAll(accountId);
    
    res.json({ message: 'Password reset successfully' });
  } catch (error) {
    console.error('[Icarus] Password reset error:', error);
    res.status(500).json({ error: 'Failed to reset password' });
  }
});

export default router;
