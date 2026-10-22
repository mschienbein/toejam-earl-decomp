#ifndef TJE_TITLE_CALLS_H
#define TJE_TITLE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void SEGAAndIntroVideoRoutine();
extern void SEGAVideo();
extern void SegaLogoVBlank();
extern unsigned short TitleScreenVideo();
extern void AnimateTitleTransitionBackground();
extern void InitializeTitleTransitionTilemap();
extern void ClearUpperVramTilemap();
extern void ClearLowerVramTilemap();
extern void TitleDrawSelectedLogo();
extern void CutsceneDrawRandomizedScreenBorder();
extern void CutsceneDrawRandomizedRectangleBorder();
extern void CutsceneDrawMetatileBlock();
extern void EndingDrawNineMetatileBackground();
extern void EndingFillRepeatedMetatileBackground();
extern void CutsceneReleaseFlaggedEntityRange();
extern int CutsceneStepFrameAndMusicCadence();
extern void SegaLogoClearVideoPlanes();
extern void SegaLogoCreateTrademarkEntity();
extern void SegaLogoReleaseTrademarkEntity();

#endif
