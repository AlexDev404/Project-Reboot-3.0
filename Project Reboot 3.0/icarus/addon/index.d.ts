/**
 * Icarus Native Add-on TypeScript Declarations
 * 
 * These declarations provide TypeScript types for the native add-on
 * when used from Node.js.
 */

// ============================================================================
// Vector Types
// ============================================================================

export interface FVector {
    x: number;
    y: number;
    z: number;
}

export interface FRotator {
    pitch: number;
    yaw: number;
    roll: number;
}

// ============================================================================
// Pawn Types
// ============================================================================

export interface PawnInfo {
    id: string;
    username: string;
    accountId: string;
    teamId: number;
    location: FVector;
    health: number;
    maxHealth: number;
    shield: number;
    maxShield: number;
    isAlive: boolean;
}

export interface PawnStats {
    health: number;
    maxHealth: number;
    shield: number;
    maxShield: number;
    isDBNO: boolean;
    isAlive: boolean;
}

export interface PawnState {
    isDBNO: boolean;
    isAlive: boolean;
    isFalling: boolean;
    isInVehicle: boolean;
}

export interface CosmeticLoadout {
    character: string;
    backpack: string;
    pickaxe: string;
    glider: string;
    contrail: string;
}

// ============================================================================
// World Types
// ============================================================================

export interface WorldProperties {
    state: 'disengaged' | 'engaged' | 'inProgress';
    playersLeft: number;
    teamsLeft: number;
    safeZonePaused: boolean;
}

// ============================================================================
// Storm Types
// ============================================================================

export interface StormPhaseConfig {
    shrinkTime: number;
    holdTime: number;
    damagePerSecond: number;
    radius: number;
}

// ============================================================================
// Flare Types
// ============================================================================

export interface Flare {
    spark_id: string;
    severity: 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';
    attention: {
        trace: Array<{
            function?: string;
            file?: string;
            line?: number;
            column?: number;
            native?: boolean;
        }>;
    };
    did_you_know: string;
}

export interface FlareUtils {
    getReason(): string;
    getSeverity(): string;
    getSparkId(): string;
    isCritical(): boolean;
    _flare: Flare;
}

// ============================================================================
// FWorld
// ============================================================================

export interface IFWorld {
    getProperty(): WorldProperties;
    getProperty(property: keyof WorldProperties): WorldProperties[keyof WorldProperties];
    setProperty(property: string, value: unknown): boolean;
    getPawnList(): PawnInfo[];
    getPawnById(id: string): PawnInfo | undefined;
    getPawnByUsername(username: string): PawnInfo | undefined;
    spawnActor(className: string, location: FVector): string | undefined;
    destroyActor(actorId: string): boolean;
}

// ============================================================================
// FPawn
// ============================================================================

export interface IFPawn {
    move(pawnId: string, x: number, y: number, z: number): boolean;
    teleport(pawnId: string, x: number, y: number, z: number): boolean;
    setRotation(pawnId: string, pitch: number, yaw: number, roll: number): boolean;
    getLocation(pawnId: string): FVector | undefined;
    getRotation(pawnId: string): FRotator | undefined;
    setCostume(pawnId: string, costumeId: string): boolean;
    getCosmetics(pawnId: string): CosmeticLoadout | undefined;
    kill(pawnId: string): boolean;
    revive(pawnId: string): boolean;
    applyDamage(pawnId: string, damage: number): boolean;
    setHealth(pawnId: string, health: number): boolean;
    setShield(pawnId: string, shield: number): boolean;
    getStats(pawnId: string): PawnStats | undefined;
    giveItem(pawnId: string, itemId: string, count?: number): boolean;
    removeItem(pawnId: string, itemGuid: string): boolean;
    getInventory(pawnId: string): unknown[];
    playEmote(pawnId: string, emoteId: string): boolean;
    sendMessage(pawnId: string, message: string): boolean;
    getState(pawnId: string): PawnState | undefined;
}

// ============================================================================
// FGame
// ============================================================================

export interface IFGame {
    startMatch(): boolean;
    endMatch(options?: { winnerId?: string; reason?: string }): boolean;
    getMatchState(): string;
    setPlayersLeft(count: number): boolean;
    startAircraft(): boolean;
    startSafeZone(): boolean;
}

// ============================================================================
// FStorm
// ============================================================================

export interface IFStorm {
    getCurrentPhase(): number;
    pause(): boolean;
    resume(): boolean;
    nextPhase(): boolean;
    skipToPhase(phase: number): boolean;
    getPhaseConfig(phase: number): StormPhaseConfig | undefined;
    setPhaseConfig(phase: number, config: Partial<StormPhaseConfig>): boolean;
}

// ============================================================================
// FInventory
// ============================================================================

export interface IFInventory {
    giveItem(playerId: string, itemId: string, count?: number): boolean;
    removeItem(playerId: string, itemGuid: string): boolean;
    clearInventory(playerId: string): boolean;
    getInventory(playerId: string): unknown[];
    giveResources(playerId: string, wood: number, stone: number, metal: number): boolean;
    setResources(playerId: string, wood: number, stone: number, metal: number): boolean;
    giveAmmo(playerId: string, ammoType: string, count: number): boolean;
    maxAmmo(playerId: string): boolean;
}

// ============================================================================
// FAdmin
// ============================================================================

export interface IFAdmin {
    kick(playerId: string, reason?: string): boolean;
    ban(playerId: string, reason?: string, duration?: number): boolean;
    unban(playerId: string): boolean;
    broadcast(message: string): boolean;
    getPlayers(): Array<{
        id: string;
        username: string;
        teamId: number;
        isAlive: boolean;
    }>;
    isOperator(playerId: string): boolean;
    setOperator(playerId: string, isOp: boolean): boolean;
}

// ============================================================================
// FBots
// ============================================================================

export interface BotSpawnOptions {
    location: FVector;
    rotation?: FRotator;
    name?: string;
    teamId?: number;
}

export interface IFBots {
    spawn(options: BotSpawnOptions): unknown;
    remove(botId: string): boolean;
    removeAll(): number;
    getAll(): unknown[];
    fillLobby(targetCount: number): number;
}

// ============================================================================
// Flare
// ============================================================================

export interface IFlare {
    parse(flare: Flare): FlareUtils;
    create(reason: string, severity?: string): Flare;
}

// ============================================================================
// Module Exports
// ============================================================================

export const version: string;
export const name: string;

export const FWorld: IFWorld;
export const FPawn: IFPawn;
export const FGame: IFGame;
export const FStorm: IFStorm;
export const FInventory: IFInventory;
export const FAdmin: IFAdmin;
export const FBots: IFBots;
export const Flare: IFlare;
