#ifndef TJE_RAM_EFFECTS_H
#define TJE_RAM_EFFECTS_H

/* Active present and effect state retains its independent parallel
 * arrays, not a speculative record layout. Incomplete outer bounds are
 * retained where consumers do not establish a finite allocation.
 * gPlayer2Powerup keeps its scalar alias of gPlayer1Powerup[1]. */
extern char gPlayer1Powerup[];
extern char gPlayer2Powerup;
extern unsigned short gPlayer1PowerupTimer[];
extern char gPlayerTransientEffectEntities[];
extern char gPlayerDecoyEntities[];
extern char gPlayerPromotionDisplayEntities[];
extern char gPlayerDoorwayEntities[];
extern char gPlayerDecoyHitCounts[];

/* gSecondDoorwayPlayerContactFlags is the existing scalar alias of gDoorwayPlayerContactFlags[1]. */
extern char gDoorwayPlayerContactFlags[];
extern char gSecondDoorwayPlayerContactFlags;

/* Burp, rain-cloud and other present effect handles and timers. */
extern char gPlayer1BurpsLeft[];
extern char gPlayer1BurpTimer[];
extern char gPlayer1RainCloudState[];
extern short gPlayer1RainCloudTimer[];
extern char gPlayerBoomBoxEntities[];
extern short gPlayerBoomBoxTimers[];

/* Whirlwind and two-slot effect state keeps the canonical word/byte views. */
extern short gPlayerWhirlwindTimers[];
extern char gPlayerWhirlwindDirectionTimers[];
extern char gPlayerWhirlwindRecoveryTimers[];
extern char gTimedEffectEntities[];

/* Projectile pools retain their known 12-slot and four-slot array
 * bounds; the related per-player arrays keep their incomplete bounds. */
extern char gSharedProjectileEntities[12];
extern char gSharedProjectileOwners[12];
extern char gControlRemapProjectileEntities[4];
extern char gPlayerControlRemapEntities[];
extern short gPlayerControlRemapTimers[];
extern char gPlayerControlRemapIndices[];
extern char gPlayerActiveTerrainEffectEntities[];
extern char gPlayerOneShotTerrainEffectEntities[];
extern char gPlayer1TomatoRainTimer[2];
extern char TOMATO_RAIN_TIMER[2];

/* Six mole-effect object handles shared with land/effect cleanup. */
extern char gMoleEffectEntities[6];

/* Additional selected final-batch storage and resource interfaces. */
extern char gPlayer2BurpsLeft;
extern char gPlayerLemonadeRewardStates[];

#endif
