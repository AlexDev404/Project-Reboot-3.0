import express, { Express, Request, Response, NextFunction } from 'express';
import cors from 'cors';
import helmet from 'helmet';
import config from './config';
import { initializeDatabase, closeDatabase } from './config/database';
import { authRoutes, accountRoutes, analyticsRoutes } from './routes';
import { checkIPBan } from './middleware';

/**
 * Icarus - The Blaze Backend
 * 
 * An entire rewrite equipped with account management, 2FA authentication and more!
 * Uses PostgreSQL for database handling.
 */

const app: Express = express();

// Security middleware
app.use(helmet());
app.use(cors());

// Parse JSON bodies
app.use(express.json());

// IP ban check
app.use(checkIPBan);

// Health check endpoint
app.get('/health', (_req: Request, res: Response) => {
  res.json({ 
    status: 'healthy',
    service: 'icarus',
    version: '1.0.0',
    timestamp: new Date().toISOString()
  });
});

// API Routes
app.use('/auth', authRoutes);
app.use('/account', accountRoutes);
app.use('/data_router', analyticsRoutes);

// 404 handler
app.use((_req: Request, res: Response) => {
  res.status(404).json({ error: 'Not found' });
});

// Error handler
app.use((err: Error, _req: Request, res: Response, _next: NextFunction) => {
  console.error('[Icarus] Unhandled error:', err);
  res.status(500).json({ error: 'Internal server error' });
});

/**
 * Start the server
 */
async function start(): Promise<void> {
  try {
    // Initialize database
    await initializeDatabase();
    
    // Start HTTP server
    app.listen(config.port, config.host, () => {
      console.log(`
╔═══════════════════════════════════════════════════════════╗
║                                                           ║
║   ██╗ ██████╗ █████╗ ██████╗ ██╗   ██╗███████╗           ║
║   ██║██╔════╝██╔══██╗██╔══██╗██║   ██║██╔════╝           ║
║   ██║██║     ███████║██████╔╝██║   ██║███████╗           ║
║   ██║██║     ██╔══██║██╔══██╗██║   ██║╚════██║           ║
║   ██║╚██████╗██║  ██║██║  ██║╚██████╔╝███████║           ║
║   ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝           ║
║                                                           ║
║   The Blaze Backend - v1.0.0                              ║
║   Server running on http://${config.host}:${config.port}              ║
║                                                           ║
╚═══════════════════════════════════════════════════════════╝
      `);
    });
    
    // Graceful shutdown
    process.on('SIGINT', shutdown);
    process.on('SIGTERM', shutdown);
  } catch (error) {
    console.error('[Icarus] Failed to start server:', error);
    process.exit(1);
  }
}

/**
 * Graceful shutdown
 */
async function shutdown(): Promise<void> {
  console.log('\n[Icarus] Shutting down gracefully...');
  await closeDatabase();
  process.exit(0);
}

// Export for testing
export { app, start };

// Start if run directly
if (require.main === module) {
  start();
}
