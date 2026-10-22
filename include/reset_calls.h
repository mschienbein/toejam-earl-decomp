#ifndef TJE_RESET_CALLS_H
#define TJE_RESET_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void IntroAllocateLargePlayerParts();
extern void IntroReleaseLargePlayerParts();
extern void IntroPositionLargePlayerPart();
extern void IntroSetLargePlayerPartSequence();
extern void IntroReleaseLargePlayerPartSprites();
extern void IntroPositionLargePlayerParts();
extern void IntroReleaseLargePlayerSpritesAndRefresh();
extern void IntroAllocateDrivingParts();
extern void IntroReleaseDrivingParts();
extern void IntroPositionDrivingPart();
extern void IntroSetDrivingPartSequence();
extern void IntroReleaseDrivingPartSprites();
extern void IntroCreateAsteroidEntity();
extern void IntroReleaseAsteroidEntity();

#endif
