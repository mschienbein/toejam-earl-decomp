#ifndef TJE_RAM_PLAYER_H
#define TJE_RAM_PLAYER_H

#include "entities.h"

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
extern char TOEJAMS_HEALTH[];
extern char EARLS_HEALTH;
extern char TOEJAMS_MAX_HEALTH[];
extern char ram_ffa256[];
extern char TOEJAM_RESTORE_HEALTH[];
extern char EARL_RESTORE_HEALTH;
extern char PLAYER_INDICATOR;

/* Forty shared 128-byte object records. gEarlPositionX is the existing
 * record-one alias; gToejamLevel and gEarlLevel name the level bytes of
 * records zero and one. Inventory views of records 26..39 are in ram_ui.h. */
extern struct object_state gPlayerInfo[40];
extern struct object_state gEarlPositionX;
extern char gToejamLevel;
extern char gEarlLevel;

/* The LAND_UNDER scalars alias the centers, [2][2], of the existing
 * five-by-five per-player terrain neighborhoods. */
extern unsigned char LAND_NEAR_TOEJAM[5][5];
extern unsigned char LAND_UNDER_TOEJAM;
extern unsigned char LAND_NEAR_EARL[5][5];
extern unsigned char LAND_UNDER_EARL;

/* Per-player movement, falling and sleep state. Incomplete outer
 * bounds are retained where no finite allocation is established.
 * ram_ffda23 is the existing scalar alias of TOEJAM_FALL_STATE[1]. */
extern char ram_ffda1e[];
extern char TOEJAM_JUMP_COUNTER[];
extern char TOEJAM_FALL_STATE[];
extern char ram_ffda23;
extern char ram_ffda24[];
extern char ram_ffda26[];
extern char ram_ffda28[];
extern char ram_ffda2c[];
extern char ram_ffda30[];
extern short ram_ffda32;
extern char ram_ffda36[];
extern char ram_ffda38[];
extern char ram_ffda3a[];
extern char ram_ffda3e[];
extern char ram_ffda40[];
extern char ram_ffda42[];
extern short P1_IDLE_COUNTER[];

/* Hula contact/dance state retains its existing byte-array views. */
extern char HULA_DANCE[];
extern char HULA_DANCE_TIMER[];
extern char ram_ffe1b4;

/* Additional selected final-batch storage and resource interfaces. */
extern char TOEJAM_SPRITE;
extern char ram_ffda2a[];
extern char ram_ffda2e;
extern short ram_ffda3c;

#endif
