#ifndef TJE_CUTSCENE_TEXT_CALLS_H
#define TJE_CUTSCENE_TEXT_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void CutsceneTextLoad();
extern void CutsceneAppendRemappedFontTiles();
extern void CutsceneQueueCenteredRemappedTextLine();
extern void CutsceneQueueCenteredOriginalPaletteTextLine();
extern void CutsceneReleaseTextEntities();
extern void CutsceneLoadSecondTextLine();
extern void CutsceneLoadOriginalPaletteTextLine();

#endif
