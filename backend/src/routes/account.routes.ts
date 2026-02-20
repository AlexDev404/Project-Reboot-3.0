import { Router, Request, Response } from 'express';
import { accountService, totpService } from '../services';
import { authenticate, apiRateLimit } from '../middleware';

const router = Router();

/**
 * GET /account
 * Get current account details
 */
router.get('/', apiRateLimit, authenticate, async (req: Request, res: Response) => {
  try {
    const account = await accountService.getPublicAccountById(req.account!.id);
    res.json({ account });
  } catch (error) {
    console.error('[Icarus] Get account error:', error);
    res.status(500).json({ error: 'Failed to get account' });
  }
});

/**
 * PATCH /account
 * Update account details
 */
router.patch('/', apiRateLimit, authenticate, async (req: Request, res: Response) => {
  try {
    const { display_name, email } = req.body;
    
    // If email is being changed, check if it's already taken
    if (email) {
      const existingEmail = await accountService.findByEmail(email);
      if (existingEmail && existingEmail.id !== req.account!.id) {
        res.status(409).json({ error: 'Email already exists' });
        return;
      }
    }
    
    const account = await accountService.updateAccount(req.account!.id, {
      display_name,
      email
    });
    
    res.json({ 
      message: 'Account updated successfully',
      account 
    });
  } catch (error) {
    console.error('[Icarus] Update account error:', error);
    res.status(500).json({ error: 'Failed to update account' });
  }
});

/**
 * DELETE /account
 * Delete account
 */
router.delete('/', apiRateLimit, authenticate, async (req: Request, res: Response) => {
  try {
    const { password } = req.body;
    
    if (!password) {
      res.status(400).json({ error: 'Password confirmation required' });
      return;
    }
    
    // Verify password before deletion
    const isValid = await accountService.verifyPassword(req.account!, password);
    
    if (!isValid) {
      res.status(401).json({ error: 'Invalid password' });
      return;
    }
    
    await accountService.deleteAccount(req.account!.id);
    
    res.json({ message: 'Account deleted successfully' });
  } catch (error) {
    console.error('[Icarus] Delete account error:', error);
    res.status(500).json({ error: 'Failed to delete account' });
  }
});

/**
 * POST /account/change-password
 * Change password
 */
router.post('/change-password', apiRateLimit, authenticate, async (req: Request, res: Response) => {
  try {
    const { current_password, new_password } = req.body;
    
    if (!current_password || !new_password) {
      res.status(400).json({ error: 'Current and new password are required' });
      return;
    }
    
    if (new_password.length < 8) {
      res.status(400).json({ error: 'New password must be at least 8 characters' });
      return;
    }
    
    // Verify current password
    const isValid = await accountService.verifyPassword(req.account!, current_password);
    
    if (!isValid) {
      res.status(401).json({ error: 'Current password is incorrect' });
      return;
    }
    
    await accountService.updatePassword(req.account!.id, new_password);
    
    res.json({ message: 'Password changed successfully' });
  } catch (error) {
    console.error('[Icarus] Change password error:', error);
    res.status(500).json({ error: 'Failed to change password' });
  }
});

/**
 * POST /account/2fa/setup
 * Generate 2FA secret and QR code URI
 */
router.post('/2fa/setup', apiRateLimit, authenticate, async (req: Request, res: Response) => {
  try {
    if (req.account!.totp_enabled) {
      res.status(400).json({ error: '2FA is already enabled' });
      return;
    }
    
    const secret = totpService.generateSecret();
    const keyUri = totpService.generateKeyUri(req.account!.username, secret);
    
    // Store secret temporarily (not enabled yet)
    // In production, you might want to store this in a temporary table
    
    res.json({
      secret,
      key_uri: keyUri,
      message: 'Scan the QR code with your authenticator app, then verify with /account/2fa/verify'
    });
  } catch (error) {
    console.error('[Icarus] 2FA setup error:', error);
    res.status(500).json({ error: 'Failed to setup 2FA' });
  }
});

/**
 * POST /account/2fa/verify
 * Verify and enable 2FA
 */
router.post('/2fa/verify', apiRateLimit, authenticate, async (req: Request, res: Response) => {
  try {
    const { secret, code } = req.body;
    
    if (!secret || !code) {
      res.status(400).json({ error: 'Secret and verification code are required' });
      return;
    }
    
    // Verify the code with the secret
    const isValid = totpService.verifyCode(secret, code);
    
    if (!isValid) {
      res.status(400).json({ error: 'Invalid verification code' });
      return;
    }
    
    // Enable 2FA
    await totpService.enable2FA(req.account!.id, secret);
    
    // Generate backup codes
    const backupCodes = totpService.generateBackupCodes();
    
    res.json({
      message: '2FA enabled successfully',
      backup_codes: backupCodes,
      warning: 'Save these backup codes in a safe place. They can be used to recover your account if you lose access to your authenticator.'
    });
  } catch (error) {
    console.error('[Icarus] 2FA verify error:', error);
    res.status(500).json({ error: 'Failed to enable 2FA' });
  }
});

/**
 * POST /account/2fa/disable
 * Disable 2FA
 */
router.post('/2fa/disable', apiRateLimit, authenticate, async (req: Request, res: Response) => {
  try {
    const { password, code } = req.body;
    
    if (!password) {
      res.status(400).json({ error: 'Password is required' });
      return;
    }
    
    // Verify password
    const isValidPassword = await accountService.verifyPassword(req.account!, password);
    
    if (!isValidPassword) {
      res.status(401).json({ error: 'Invalid password' });
      return;
    }
    
    // Verify 2FA code if 2FA is enabled
    if (req.account!.totp_enabled && req.account!.totp_secret) {
      if (!code) {
        res.status(400).json({ error: '2FA code is required' });
        return;
      }
      
      const isValid2FA = totpService.verifyCode(req.account!.totp_secret, code);
      
      if (!isValid2FA) {
        res.status(400).json({ error: 'Invalid 2FA code' });
        return;
      }
    }
    
    await totpService.disable2FA(req.account!.id);
    
    res.json({ message: '2FA disabled successfully' });
  } catch (error) {
    console.error('[Icarus] 2FA disable error:', error);
    res.status(500).json({ error: 'Failed to disable 2FA' });
  }
});

export default router;
