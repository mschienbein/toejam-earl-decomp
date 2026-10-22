#ifndef TJE_RAM_EFFECTS_H
#define TJE_RAM_EFFECTS_H

/* Active present and effect state retains its independent parallel
 * arrays, not a speculative record layout. Incomplete outer bounds are
 * retained where consumers do not establish a finite allocation.
 * gPlayer2Powerup keeps its scalar alias of gPlayer1Powerup[1]. */
extern char gPlayer1Powerup[];
extern char gPlayer2Powerup;
extern unsigned short gPlayer1PowerupTimer[];
extern char ram_ffde56[];
extern char ram_ffde58[];
extern char ram_ffde5a[];
extern char ram_ffde5c[];
extern char ram_ffde5e[];

/* ram_ffde61 is the existing scalar alias of ram_ffde60[1]. */
extern char ram_ffde60[];
extern char ram_ffde61;

/* Burp, rain-cloud and other present effect handles and timers. */
extern char gPlayer1BurpsLeft[];
extern char gPlayer1BurpTimer[];
extern char gPlayer1RainCloudState[];
extern short gPlayer1RainCloudTimer[];
extern char ram_ffde6c[];
extern short ram_ffde6e[];

/* Whirlwind and two-slot effect state keeps the canonical word/byte views. */
extern short ram_ffe1b6[];
extern char ram_ffe1ba[];
extern char ram_ffe1bc[];
extern char ram_ffe1be[];

/* Projectile pools retain their known 12-slot and four-slot array
 * bounds; the related per-player arrays keep their incomplete bounds. */
extern char ram_ffe1c0[12];
extern char ram_ffe1cc[12];
extern char ram_ffe1d8[4];
extern char ram_ffe1dc[];
extern short ram_ffe1de[];
extern char ram_ffe1e2[];
extern char ram_ffe1e4[];
extern char ram_ffe1e6[];
extern char gPlayer1TomatoRainTimer[2];
extern char TOMATO_RAIN_TIMER[2];

/* Six mole-effect object handles shared with land/effect cleanup. */
extern char ram_ffe244[6];

/* Additional selected final-batch storage and resource interfaces. */
extern char gPlayer2BurpsLeft;
extern char ram_ffe396[];

#endif
