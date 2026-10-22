#ifndef TJE_ENGINE_CALLS_H
#define TJE_ENGINE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void ClearTransientEntities();
extern void DrawMenuWindowFrame();
extern void DrawNestedWindowFrame();
extern void DrawPresentDisplayPanel();
extern void DrawPauseChoicePanel();
extern int IsSpeechBubbleEntity();

#endif
