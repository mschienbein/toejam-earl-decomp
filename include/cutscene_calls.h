#ifndef TJE_CUTSCENE_CALLS_H
#define TJE_CUTSCENE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void PlayEndingSequence();
extern short EndingPlayDialogueAndDeparture();
extern short EndingPlayUpwardRocketshipDeparture();
extern short EndingPlayScrollingCredits();
extern short EndingPlayDescendingRocketshipScene();
extern short EndingPlayBouncingRocketshipScene();

#endif
