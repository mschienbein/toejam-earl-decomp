#ifndef TJE_RAM_CUTSCENE_H
#define TJE_RAM_CUTSCENE_H


/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern short SEGA_PAL_LOAD_FLAG;
extern unsigned short *SEGA_PAL_TO_LOAD;
extern unsigned char gEndingPlayer1Entity;
extern unsigned char gEndingPlayer2Entity;
extern char gCutsceneFirstTextEntity;
extern char gCutsceneSecondTextEntity;
extern unsigned char gEndingPoseCompositeEntity;
extern unsigned char gEndingSixPieceCompositeEntity;
extern unsigned char gEndingRightOffsetCompositeEntity;
extern unsigned char gEndingElevatedThreePieceCompositeEntity;
extern unsigned short gEndingStationaryCompositeEntity;
extern short gCutsceneScriptState;
extern short gCutsceneMusicActorAnimationEnabled;
extern short gCutsceneScriptCountdown;
extern short gCutsceneAdvanceButtonMask;
extern unsigned short gSegaTrademarkEntity;
extern short gSegaVBlankSpriteUpdatesEnabled;
extern short gSegaRocketPassShown;

#endif
