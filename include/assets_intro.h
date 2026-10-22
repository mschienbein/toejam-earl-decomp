#ifndef TJE_ASSETS_INTRO_H
#define TJE_ASSETS_INTRO_H

#include "sprites.h"

/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern char CrashScreen[];
extern short CrashScreenSize;
extern char *IntroAnimationPointers[4];
extern char IntroFrameIndexLists[124];
extern struct held_frame2 IntroHeldFrameStreams[22];
extern char LogoScreen[];
extern short LogoScreenSize;
extern struct render_frame *Spr_IntroAsteroid[2];
extern struct render_frame *Spr_IntroCollision[2];
extern struct render_frame *Spr_IntroEarlDriving[7];
extern struct render_frame *Spr_IntroRocketshipDodging[4];
extern struct render_frame *Spr_IntroRocketshipFalling[5];
extern struct render_frame *Spr_IntroRocketshipImpact[6];
extern struct render_frame *Spr_Planet[19];
extern struct render_frame Spr_PlanetFrame00;
extern struct render_frame *Spr_RocketshipExhaust[7];
extern struct render_frame *Spr_TitleBigPlayers[22];
extern char TAndELogoScreen[];
extern short TAndELogoScreenSize;
extern unsigned short TextMapping[25][40];
extern unsigned short TitleLogoTilemap[17][40];
extern unsigned char dat_03071d[3];
extern char dat_030720[];
extern char dat_030723[];
extern char dat_030736[];
extern char dat_030744[];
extern char dat_030748[];
extern char dat_030753[];
extern char dat_030759[];
extern char dat_03075d[];
extern char dat_030760[];
extern char dat_030762[];
extern char dat_030764[];
extern char dat_030766[];
extern char dat_030768[];
extern char dat_03076a[];
extern char dat_03076c[];
extern char dat_03076e[];
extern unsigned char dat_030770[8];
extern struct held_frame2 dat_03079a;
extern struct held_frame2 dat_0307a2;
extern struct held_frame2 dat_0307a4;
extern struct held_frame2 dat_0307ae[8];
extern struct held_frame2 dat_0307be[3];
extern struct held_frame2 dat_0307c2;
extern char rom_030726[5];
extern char rom_03072b[5];
extern char rom_030730[];
extern struct render_frame *rom_03350e[8];
extern struct render_frame *rom_03352e[5];
extern struct render_frame *rom_033542[5];

#endif
