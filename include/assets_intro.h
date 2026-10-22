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
extern unsigned char IntroDrivingInitialFrameIndices[3];
extern char IntroDrivingActorTwoLoopAnimation[];
extern char IntroAsteroidLoopAnimation[];
extern char IntroLargePlayerPartFiveFramePulse[];
extern char IntroLargePlayerPartFiveAscendingFrames[];
extern char IntroLargePlayerPartFiveDescendingFrames[];
extern char IntroLargePlayerPartOneHeldFrameThree[];
extern char IntroLargePlayerPartSevenAscendingFrames[];
extern char IntroLargePlayerPartThreeLoopAnimation[];
extern char IntroLargePlayerPartOneHeldFrameNineteen[];
extern char IntroLargePlayerPartSixHeldFrameFourteen[];
extern char IntroLargePlayerPartSixHeldFrameFifteen[];
extern char IntroLargePlayerPartSevenHeldFrameSixteen[];
extern char IntroLargePlayerPartSevenHeldFrameSeventeen[];
extern char IntroLargePlayerPartSevenHeldFrameEighteen[];
extern char IntroLargePlayerPartOneHeldFrameOne[];
extern char IntroLargePlayerPartOneHeldFrameTwo[];
extern unsigned char IntroLargePlayerPartInitialFrameIndices[8];
extern struct held_frame2 IntroHeldFrameOne;
extern struct held_frame2 IntroHeldFrameFive;
extern struct held_frame2 IntroHeldFrameSix;
extern struct held_frame2 IntroLargePlayerPartHeldFrameStreams[8];
extern struct held_frame2 IntroDrivingPartHeldFrameStreams[3];
extern struct held_frame2 IntroHeldFrameThree;
extern char IntroFlyingForwardFramesAnimation[5];
extern char IntroFlyingFlippedFramesAnimation[5];
extern char IntroFlyingSceneFourAnimation[];
extern struct render_frame *CutsceneDriftingEntityFrames[8];
extern struct render_frame *IntroFlyingAlternateEntityFrames[5];
extern struct render_frame *IntroFlyingEntityFrames[5];

#endif
