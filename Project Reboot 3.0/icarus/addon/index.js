/**
 * Icarus Native Add-on Loader
 * 
 * This module loads the compiled native add-on (.node file) and exports
 * all the binding modules with proper TypeScript types.
 * 
 * @example
 * ```ts
 * import { FWorld, FPawn, FGame } from '@trail-blaze/icarus-addon';
 * 
 * // Get all pawns and move them
 * const pawns = FWorld.getPawnList();
 * pawns.forEach(pawn => {
 *   FPawn.move(pawn.id, 100, 200, 300);
 * });
 * 
 * // Start the match
 * FGame.startMatch();
 * ```
 */

const bindings = require('bindings');

// Load the native add-on
let addon;

try {
    addon = bindings('icarus');
} catch (error) {
    // If we can't load the native add-on, provide mock implementations
    console.warn('[Icarus] Native add-on not available, using mock mode');
    addon = createMockAddon();
}

/**
 * Create mock implementations for development/testing
 */
function createMockAddon() {
    return {
        version: '1.0.0',
        name: 'icarus-mock',
        
        FWorld: {
            getProperty: (prop) => {
                const props = { state: 'disengaged', playersLeft: 0, teamsLeft: 0, safeZonePaused: false };
                return prop ? props[prop] : props;
            },
            setProperty: () => true,
            getPawnList: () => [],
            getPawnById: () => undefined,
            getPawnByUsername: () => undefined,
            spawnActor: () => undefined,
            destroyActor: () => false,
        },
        
        FPawn: {
            move: () => true,
            teleport: () => true,
            setRotation: () => true,
            getLocation: () => ({ x: 0, y: 0, z: 0 }),
            getRotation: () => ({ pitch: 0, yaw: 0, roll: 0 }),
            setCostume: () => true,
            getCosmetics: () => ({ character: '', backpack: '', pickaxe: '', glider: '', contrail: '' }),
            kill: () => true,
            revive: () => true,
            applyDamage: () => true,
            setHealth: () => true,
            setShield: () => true,
            getStats: () => ({ health: 100, maxHealth: 100, shield: 0, maxShield: 100, isDBNO: false, isAlive: true }),
            giveItem: () => true,
            removeItem: () => false,
            getInventory: () => [],
            playEmote: () => false,
            sendMessage: () => true,
            getState: () => ({ isDBNO: false, isAlive: true, isFalling: false, isInVehicle: false }),
        },
        
        FGame: {
            startMatch: () => true,
            endMatch: () => true,
            getMatchState: () => 'setup',
            setPlayersLeft: () => true,
            startAircraft: () => true,
            startSafeZone: () => true,
        },
        
        FStorm: {
            getCurrentPhase: () => 0,
            pause: () => true,
            resume: () => true,
            nextPhase: () => true,
            skipToPhase: () => true,
            getPhaseConfig: () => ({ shrinkTime: 120, holdTime: 60, damagePerSecond: 1, radius: 10000 }),
            setPhaseConfig: () => true,
        },
        
        FInventory: {
            giveItem: () => true,
            removeItem: () => false,
            clearInventory: () => false,
            getInventory: () => [],
            giveResources: () => true,
            setResources: () => false,
            giveAmmo: () => false,
            maxAmmo: () => false,
        },
        
        FAdmin: {
            kick: () => true,
            ban: () => false,
            unban: () => false,
            broadcast: () => true,
            getPlayers: () => [],
            isOperator: () => false,
            setOperator: () => false,
        },
        
        FBots: {
            spawn: () => undefined,
            remove: () => false,
            removeAll: () => 0,
            getAll: () => [],
            fillLobby: () => 0,
        },
        
        Flare: {
            parse: (flare) => ({
                getReason: () => flare.did_you_know || 'Unknown error',
                getSeverity: () => flare.severity || 'UNKNOWN',
                getSparkId: () => flare.spark_id || '',
                isCritical: () => flare.severity === 'CRITICAL',
                _flare: flare,
            }),
            create: (reason, severity = 'MEDIUM') => ({
                spark_id: 'MOCK' + Math.random().toString(36).substr(2, 40),
                severity,
                attention: { trace: [] },
                did_you_know: reason,
            }),
        },
    };
}

// Export everything
module.exports = addon;
module.exports.default = addon;
