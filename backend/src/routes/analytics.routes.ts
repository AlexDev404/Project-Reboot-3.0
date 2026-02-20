import { Router, Request, Response } from 'express';
import { pool } from '../config/database';
import { optionalAuthenticate } from '../middleware';

const router = Router();

/**
 * POST /data_router
 * Analytics endpoint for collecting user information
 */
router.post('/', optionalAuthenticate, async (req: Request, res: Response) => {
  try {
    const { event_type, event_data } = req.body;
    
    if (!event_type) {
      res.status(400).json({ error: 'Event type is required' });
      return;
    }
    
    const ipAddress = req.ip || req.socket.remoteAddress;
    const userAgent = req.headers['user-agent'];
    
    // Extract client info from user agent or request body
    const clientVersion = req.body.client_version || extractClientVersion(userAgent);
    const os = req.body.os || extractOS(userAgent);
    
    await pool.query(
      `INSERT INTO analytics (account_id, ip_address, client_version, os, event_type, event_data)
       VALUES ($1, $2, $3, $4, $5, $6)`,
      [
        req.account?.id || null,
        ipAddress,
        clientVersion,
        os,
        event_type,
        JSON.stringify(event_data || {})
      ]
    );
    
    res.json({ success: true });
  } catch (error) {
    console.error('[Icarus] Analytics error:', error);
    res.status(500).json({ error: 'Failed to record analytics' });
  }
});

/**
 * Extract client version from user agent
 */
function extractClientVersion(userAgent?: string): string | null {
  if (!userAgent) return null;
  
  // Match patterns like "Fortnite/++Fortnite+Release-X.XX" or similar
  const match = userAgent.match(/Fortnite[/+\-].*?(\d+\.\d+)/i);
  return match ? match[1] : null;
}

/**
 * Extract OS from user agent
 */
function extractOS(userAgent?: string): string | null {
  if (!userAgent) return null;
  
  if (userAgent.includes('Windows')) return 'Windows';
  if (userAgent.includes('Mac')) return 'macOS';
  if (userAgent.includes('Linux')) return 'Linux';
  if (userAgent.includes('Android')) return 'Android';
  if (userAgent.includes('iOS') || userAgent.includes('iPhone')) return 'iOS';
  
  return null;
}

export default router;
