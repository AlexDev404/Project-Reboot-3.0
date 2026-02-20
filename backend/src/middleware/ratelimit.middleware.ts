import { Request, Response, NextFunction } from 'express';

/**
 * Simple in-memory rate limiter
 * For production, use a distributed rate limiter like Redis
 */

interface RateLimitEntry {
  count: number;
  resetTime: number;
}

const rateLimitStore: Map<string, RateLimitEntry> = new Map();

// Cleanup old entries every 5 minutes
setInterval(() => {
  const now = Date.now();
  for (const [key, entry] of rateLimitStore) {
    if (entry.resetTime < now) {
      rateLimitStore.delete(key);
    }
  }
}, 5 * 60 * 1000);

/**
 * Create a rate limiter middleware
 * @param maxRequests Maximum requests allowed in the time window
 * @param windowMs Time window in milliseconds
 * @param keyPrefix Prefix for the rate limit key (to separate different limiters)
 */
export function rateLimit(
  maxRequests: number = 100,
  windowMs: number = 15 * 60 * 1000, // 15 minutes
  keyPrefix: string = 'global'
) {
  return (req: Request, res: Response, next: NextFunction): void => {
    const ip = req.ip || req.socket.remoteAddress || 'unknown';
    const key = `${keyPrefix}:${ip}`;
    const now = Date.now();
    
    let entry = rateLimitStore.get(key);
    
    if (!entry || entry.resetTime < now) {
      // Create new entry
      entry = {
        count: 1,
        resetTime: now + windowMs
      };
      rateLimitStore.set(key, entry);
    } else {
      entry.count++;
    }
    
    // Set rate limit headers
    res.setHeader('X-RateLimit-Limit', maxRequests.toString());
    res.setHeader('X-RateLimit-Remaining', Math.max(0, maxRequests - entry.count).toString());
    res.setHeader('X-RateLimit-Reset', Math.ceil(entry.resetTime / 1000).toString());
    
    if (entry.count > maxRequests) {
      const retryAfter = Math.ceil((entry.resetTime - now) / 1000);
      res.setHeader('Retry-After', retryAfter.toString());
      res.status(429).json({
        error: 'Too many requests',
        retryAfter
      });
      return;
    }
    
    next();
  };
}

/**
 * Strict rate limiter for sensitive endpoints (login, password reset)
 * 5 requests per 15 minutes
 */
export const strictRateLimit = rateLimit(5, 15 * 60 * 1000, 'strict');

/**
 * Auth rate limiter for authentication endpoints
 * 20 requests per 15 minutes
 */
export const authRateLimit = rateLimit(20, 15 * 60 * 1000, 'auth');

/**
 * API rate limiter for general API endpoints
 * 100 requests per 15 minutes
 */
export const apiRateLimit = rateLimit(100, 15 * 60 * 1000, 'api');
