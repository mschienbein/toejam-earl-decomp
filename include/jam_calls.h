#ifndef TJE_JAM_CALLS_H
#define TJE_JAM_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void JamOutVBlank();
extern void StartJamOut();
extern void JamOutRandomSampleVBlank();
extern void JamOutUpdateLeadAnimations();
extern void JamOutUpdateLayoutsAndScroll();
extern void JamOutSynchronizeEntityCopies();
extern void JamOutCreateCharacterCopies();
extern void JamOutReleaseCharacterCopies();
extern void JamOutPositionToejamCopy();
extern void JamOutPositionEarlCopy();
extern void JamOutHideToejamCopy();
extern void JamOutHideEarlCopy();
extern void JamOutSetToejamSequence();
extern void JamOutSetEarlSequence();
extern void JamOutShowLeadPairLayout();
extern void JamOutDrawPackedBackgroundBlock();
extern void JamOutShowFourPairLayout();
extern void JamOutShowNinePairLayout();

#endif
