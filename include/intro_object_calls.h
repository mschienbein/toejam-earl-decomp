#ifndef TJE_INTRO_OBJECT_CALLS_H
#define TJE_INTRO_OBJECT_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void IntroCreateCollisionSceneEntities();
extern void IntroReleaseCollisionSceneEntities();
extern void EndingCreateActivePlayerEntities();
extern void EndingReleaseActivePlayerEntities();
#ifndef TJE_LOCAL_FUNC_0289DA
extern short EndingGetFirstPlayerAnimationFrame();
#endif
extern void IntroCreateHiddenFrameEntities();
extern void IntroRevealFrameEntities();
extern void IntroReleaseFrameEntitiesAndRefresh();
extern void CutsceneAllocateRocketshipPair();
extern void CutsceneReleaseRocketshipPair();
extern void CutsceneSetRocketshipPairPose();
extern void CutsceneSetForegroundRocketshipPose();
extern void CutsceneMoveRocketshipPairLeft();
extern void CutscenePositionRocketshipPair();
extern void EndingCreatePoseComposite();
extern void EndingSetPoseComposite();
extern void EndingReleasePoseComposite();
extern void EndingCreateSixPieceComposite();
extern void EndingPositionSixPieceComposite();
extern void EndingReleaseSixPieceComposite();
extern void EndingCreateRightOffsetComposite();
extern void EndingPositionRightOffsetComposite();
extern void EndingReleaseRightOffsetComposite();
extern void IntroCreateFallingRocketship();
extern void IntroReleaseFallingRocketship();
extern void IntroPositionFallingRocketship();
extern void EndingCreateElevatedThreePieceComposite();
extern void EndingReleaseElevatedThreePieceComposite();
extern void EndingRestartElevatedCompositeSequence();
extern void CutsceneCreateDriftingEntities();
extern void CutsceneReleaseDriftingEntities();
extern void CutsceneUpdateDriftingEntities();
extern void IntroAllocateFlyingEntitySlots();
extern void IntroReleaseFlyingEntitySlots();
extern void IntroUpdateRightwardFlyingEntity();
extern void IntroUpdateFlyingEntityMotion();
extern void IntroCreateRocketshipImpact();
extern void IntroReleaseRocketshipImpact();
extern void CutsceneCreatePlanetEntity();
extern void CutsceneReleasePlanetEntity();
extern void EndingCreateStationaryComposite();
extern void EndingReleaseStationaryComposite();

#endif
