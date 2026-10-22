#ifndef TJE_EARTHLING_CALLS_H
#define TJE_EARTHLING_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void AI_GenericEnemyThink();
extern void AI_MailboxTouch();
extern void AI_OperaSingerTouch();
extern void AI_WisemanTouch();
extern void AI_WizardTouch();
extern void CheckAButton();
extern int CheckEarthlingOverlap();
extern void DamageEarthling();
extern void DoVikingAbility();
extern void DropPresentsAbility();
extern int FindSpawnLocationForEarthling();
extern void LoadInEarthlings();
extern int NiceEarthlingCheck();
extern void OpenEarthlingPresent();
extern void ProcessEarthlings();
extern void SantaLogic();
extern void SetEarthlingState();
extern void SetupInitialEarthlings();
extern void EarthlingReleaseLevelBank();
extern int EarthlingIsSpecialRecordNearby();
extern void MapGetShipPieceWorldPosition();
extern void EarthlingRetainLevelBank();
extern void EarthlingRelocateRecord();
extern void EarthlingRestoreSavedPosition();
extern void EarthlingAllocateEntities();
extern void EarthlingDispatchAbilities();
extern void EarthlingDispatchPlayerContacts();
extern void EarthlingApplyNearbySleep();
extern void EarthlingWakeLevel();
extern int EarthlingResetStateTwoByDrawMask();
extern void EarthlingRemoveFromLevel();
extern void EarthlingDamageOnHazardTerrain();
extern void EarthlingClearMolePlayerContactFlags();
extern int EarthlingProbeDirectionalTerrain();
extern void EarthlingMoveOnAllowedTerrain();
extern int EarthlingIsUpdateBlocked();
extern void EntityUpdateTerrainDrawFlag();
extern void EarthlingMoveWithinMapBounds();
extern char EarthlingSelectTurnState();
extern int EarthlingProcessSpecialThinkStates();
extern char EarthlingSelectTerrainAnimationState();
extern void EarthlingUpdateTerrainAnimationState();
extern void EarthlingThinkTimedMovement();
extern void EarthlingThinkHorizontalWander();
extern void EarthlingAdvanceHorizontalTurnAnimation();
extern void EarthlingThinkHorizontalReversal();
extern void EarthlingThinkTargetedResetCycle();
extern void PlayerCreateInteractionChoiceDisplay();
extern int PlayerCreateLifeRequestDisplay();
extern void PlayerProcessLifeRequest();
extern void EarthlingSpawnMoleEffect();
extern void ReleaseFinishedMoleEffectsAndStolenPresents();

#endif
