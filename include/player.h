#ifndef TJE_PLAYER_H
#define TJE_PLAYER_H

/* PlayerApplyDirectionalMovementAnimation resolves request 0 with GetActivePresentSprite. */
#define PLAYER_MOVEMENT_ACTIVE_PRESENT 0

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void AI_BoogieTouch();
extern void CheckPlayerFalling();
extern void ChickenAbility();
extern void CupidAbility();
extern void DoMoleAbility();
extern int GetActivePresentSprite();
extern void HottubAbility();
extern void HulaAbility();
extern void KillThePlayer();
extern void LemonadeAbility();
extern void MakePlayerFall();
extern void MoleAbility();
extern void OpenRocketSkates();
extern void OpenSchoolBook();
extern void Player_Squish();
extern int Player_Youch();
extern void ProcessMovement();
extern void ProcessPlayers();
extern void ProcessSleep();
extern void PlayerStartFalling();
extern void SetupPlayer();
extern void SpriteChecks();
extern void StopPlayer();
extern void WhirlWindAbility();
extern void SetCameraPosition();
extern void AnimatePlayersIntoSplitViews();
extern int EntityIsNearPosition();
extern int EntityIsWithinRange();
extern void PlayerProcessTerrainMotion();
extern void PlayerApplyTerrainDestination();
extern void Player1ProcessTerrainMotion();
extern void Player2ProcessTerrainMotion();
extern void PlayerChooseDirectionalMovement();
extern void PlayerApplyDirectionalMovementAnimation();
extern void MapClearTilePriorityNearPlayer();
extern void PlayerStartTeeteringFall();
extern void PlayerUpdateTeetering();
extern void PlayerUpdateJumpLandingAnimation();
extern void PlayerUpdatePresentCompletionAndTerrainPose();
extern void PlayerUpdateTerrainSinking();
extern void PlayerUpdateHurtRecoveryAndIdle();
extern void PlayerUpdateSpringShoeJump();
extern void PlayerResolveDescendingTerrain();
extern void PlayerAdvanceToNextLevel();
extern void PlayerRelocateToPartnerOrNextLevel();
extern void PlayerFinalizeLastLifeExit();
extern void PlayerUpdateDeathAnimation();
extern void PlayerUpdateRocketSkatesSprite();
extern void PlayerApplyRocketSkatesInput();
extern void PlayerUpdateRocketSkatesMotion();
extern void PlayerInitializePhasedJumpMotion();
extern void PlayerUpdatePhasedJumpMotion();
extern void PlayerApplyPhasedJumpInput();
extern int PlayerCheckTerrainEdgeHop();
extern void PlayerUpdateHealthSharingClap();
extern char EarthlingSteerTowardNearestTarget();
extern int EarthlingCanTargetPlayerState();
extern char EntitySetVelocityTowardDelta();
extern char EntitySteerWithHorizontalDrift();
extern char EntitySteerWithVerticalDeadZone();
extern void EntityTriggerTargetStatusEventOnce();
extern int PlayerCanReceiveEarthlingEffect();
extern int EarthlingStartHorizontalAttack();
extern void EarthlingUpdateBouncingDamageState();
extern void PlayerUpdateWhirlwindAndRecovery();
extern void EarthlingCheckDecoyAttackRange();
extern int EntityHasFreeTimedEffectSlot();
extern int EntitySpawnTimedEffect();
extern void EntityReleaseFinishedTimedEffect();
extern void PlayerLaunchPresentProjectile();
extern void PlayerUpdatePresentShooting();
extern void InitializeLevelZeroFixedItems();
extern void GetLevelZeroEarthlingPosition();

#endif
