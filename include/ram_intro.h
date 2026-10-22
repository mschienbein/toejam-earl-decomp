#ifndef TJE_RAM_INTRO_H
#define TJE_RAM_INTRO_H


/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern unsigned short gSceneLastMusicEventTime;
extern unsigned short gSceneBackgroundAdvanceCounter;
extern short gSceneUpdateCounter;
extern char gSceneBackgroundFramePhase;
extern char gSceneBackgroundFrameTimer;
extern char gSceneBackgroundPairIndex;
extern char gTitleLogoSelector;
extern unsigned char gIntroLargePlayerPartEntities[8];
extern unsigned char gIntroCollisionParentEntity;
extern unsigned char gIntroCollisionAuxiliaryEntities[3];
extern unsigned char gIntroCollisionAttachedEntities[3];
extern unsigned char gIntroAsteroidEntity;
extern unsigned char gIntroDrivingPartEntities[3];
extern unsigned char gIntroComponentEntities[10];
extern unsigned char gIntroFallingRocketshipEntity;
extern unsigned char gCutsceneRocketshipEntity;
extern unsigned char gCutsceneRocketshipExhaustEntity;
extern unsigned char gCutsceneDriftingEntities[8];
extern unsigned char gIntroFlyingEntities[4];
extern unsigned short gIntroRocketshipImpactEntity;
extern unsigned short gCutscenePlanetEntity;

#endif
