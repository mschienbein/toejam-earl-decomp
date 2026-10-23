#ifndef TJE_RAM_CUTSCENE_H
#define TJE_RAM_CUTSCENE_H

/* Intro and ending scene loops stop when their script dispatch stores this shared sentinel. */
#define CUTSCENE_SCRIPT_DONE (-1)


/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern short gSegaPaletteUploadPending;
extern unsigned short *gSegaPaletteUploadSource;
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
