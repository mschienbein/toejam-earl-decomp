#ifndef TJE_RAM_PLAYER_H
#define TJE_RAM_PLAYER_H

#include "entities.h"

/* PlayerGetMaximumHealth adds four health units per rank to these character bases. */
#define PLAYER_TOEJAM_BASE_HEALTH 23
#define PLAYER_EARL_BASE_HEALTH 31

/* PlayerResetRankAndResources starts at 0; CheckPromotionScore stops promotions at 8. */
#define PLAYER_RANK_INITIAL 0
#define PLAYER_RANK_MAX 8
/* PlayerFinalizeLastLifeExit writes rank 9 and level 99 after releasing a player's level. */
#define PLAYER_RANK_ELIMINATED 9
#define PLAYER_LEVEL_ELIMINATED 99

/* PlayerStartFalling selects 1; CheckPlayerFalling handles landing in 2 and clears to 0. */
#define PLAYER_FALL_NONE 0
#define PLAYER_FALL_DESCENDING 1
#define PLAYER_FALL_LANDING 2

/* Player statistics keep their canonical byte views and incomplete
 * outer bounds. The player-two scalars alias element one of the
 * corresponding lives, bucks, rank, health and health-restore arrays. */
extern char gPlayerLives[];
extern char gPlayer2Lives;
extern char gPlayerBucks[];
extern char gPlayer2Bucks;
extern unsigned short gPlayerPoints[2];
extern char gPlayerRank[];
extern char gPlayer2Rank;
extern char gPlayerHealth[];
extern char gPlayer2Health;
extern char gHudDisplayedHealth[];
extern char gHudDisplayedLives[];
extern char gPlayerHealthBeforeSwimming[];
extern char gPlayer2HealthBeforeSwimming;
extern char gPlayerSelection;

/* Forty shared 128-byte object records. gPlayer2Entity is the existing
 * record-one alias; gToejamLevel and gEarlLevel name the level bytes of
 * records zero and one. Inventory views of records 26..39 are in ram_ui.h. */
extern struct object_state gPlayerInfo[ENTITY_SLOT_COUNT];
extern struct object_state gPlayer2Entity;
extern char gToejamLevel;
extern char gEarlLevel;

/* The LAND_UNDER scalars alias the centers, [2][2], of the existing
 * five-by-five per-player terrain neighborhoods. */
extern unsigned char gPlayer1NearbyTerrain[5][5];
extern unsigned char gPlayer1TerrainUnderfoot;
extern unsigned char gPlayer2NearbyTerrain[5][5];
extern unsigned char gPlayer2TerrainUnderfoot;

/* Per-player movement, falling and sleep state. Incomplete outer
 * bounds are retained where no finite allocation is established.
 * gPlayer2FallState is the existing scalar alias of gPlayerFallStates[1]. */
extern char gPlayerPreDeathSpriteState[];
extern char gPlayerJumpCounters[];
extern char gPlayerFallStates[];
extern char gPlayer2FallState;
extern char gPlayerTerrainSinking[];
extern char gPlayerPhasedJumpPhase[];
extern char gPlayerPhasedJumpPhaseTicks[];
extern char gPlayerPendingHurtDamage[];
extern char gPlayerLandingCounterSeed[];
extern short gTerrainPriorityUpdatePlayer;
extern char gPlayerPhasedJumpArmed[];
extern char gPlayerPhasedJumpSpeechUsed[];
extern char gPlayerRocketTurnDelay[];
extern char gPlayerBookSleepDelay[];
extern char gPlayerWakeEffectsLeft[];
extern char gPlayerWakeEffectEntityHandles[];
extern short gPlayerIdleTicks[];

/* Hula contact/dance state retains its existing byte-array views. */
extern char gPlayerHulaDanceTicks[];
extern char gPlayerHulaContactDelay[];
extern char gHulaSpeechCooldown;

/* First-player animation alias, respawn suppression, and shared speech/sound latches. */
extern char gPlayer1SpriteState;
extern char gPlayerRespawnSuppressionTicks[];
extern char gSinkingHelpSpeechDelay;
extern short gTerrainEdgeMeepPending;

#endif
