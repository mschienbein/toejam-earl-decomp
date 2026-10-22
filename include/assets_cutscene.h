#ifndef TJE_ASSETS_CUTSCENE_H
#define TJE_ASSETS_CUTSCENE_H

#include "sprites.h"

/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern struct render_frame *EndingStationaryCompositeFrames[1];
extern struct render_frame *EndingPoseCompositeFrames[2];
extern struct render_frame *EndingSixPieceCompositeFrames[1];
extern struct render_frame *EndingRightOffsetCompositeFrames[2];
extern struct render_frame *EndingElevatedCompositeFrames[3];
extern struct render_frame *EndingPlayer2Frames[28];
extern struct render_frame *EndingPlayer1Frames[37];
extern char EndingFrameIndexStreams[254];
extern unsigned short SEGALogoTileMap[];
extern char SplashSegaLogo[];
extern struct render_frame *Spr_Trademark[];
extern char VRAMClearingData[];
extern char EndingPlayer1SoloAnimation[];
extern char EndingPlayer2PairedAnimation[];
extern char EndingPlayer1PairedAnimation[];

#endif
