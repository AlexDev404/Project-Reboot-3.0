import dotenv from 'dotenv';
import path from 'path';

// Load environment variables from .env file
dotenv.config({ path: path.resolve(__dirname, '../../.env') });

export interface DatabaseConfig {
  host: string;
  port: number;
  database: string;
  user: string;
  password: string;
  connectionString?: string;
}

export interface JWTConfig {
  secret: string;
  expiresIn: string;
}

export interface TOTPConfig {
  issuer: string;
}

export interface Config {
  port: number;
  host: string;
  nodeEnv: string;
  database: DatabaseConfig;
  jwt: JWTConfig;
  totp: TOTPConfig;
  bcryptRounds: number;
}

const config: Config = {
  port: parseInt(process.env.PORT || '3000', 10),
  host: process.env.HOST || '0.0.0.0',
  nodeEnv: process.env.NODE_ENV || 'development',
  
  database: {
    host: process.env.DB_HOST || 'localhost',
    port: parseInt(process.env.DB_PORT || '5432', 10),
    database: process.env.DB_NAME || 'icarus',
    user: process.env.DB_USER || 'user',
    password: process.env.DB_PASSWORD || 'password',
    connectionString: process.env.DATABASE_URL
  },
  
  jwt: {
    secret: process.env.JWT_SECRET || 'default-secret-change-in-production',
    expiresIn: process.env.JWT_EXPIRES_IN || '24h'
  },
  
  totp: {
    issuer: process.env.TOTP_ISSUER || 'Icarus'
  },
  
  bcryptRounds: parseInt(process.env.BCRYPT_ROUNDS || '12', 10)
};

export default config;
