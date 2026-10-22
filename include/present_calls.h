#ifndef TJE_PRESENT_CALLS_H
#define TJE_PRESENT_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void BummerLifePromo();
extern void CheckForTelephone();
extern void CheckTomatoRainTimer();
extern void DropPresent();
extern void EatFudgeSundae();
extern void EatYuckyFood();
extern void EatYummyFood();
extern void MapFindItemPlacementPosition();
extern void NoOp();
extern void OpenBoomBox();
extern void OpenDecoy();
extern int OpenSelectedPresent();
extern void OpenTelephone();
extern void PerformPromotion();
extern void PlaceStuffInLevel();
extern void PresentIsDoorway();
extern void PresentIsRainCloud();
extern void ProcessPresents();
extern void ProcessTomatoRain();
extern void ReducePresentTimer();
extern int SelectRandomPresent();
extern void SetTomatoRainTomatoes();
extern void SetupInitialInventory();
extern void SetupPresentIdentities();
extern void SetupSelectedPresent();
extern void MapReleaseLevelItemBank();
extern int ElevatorAllowsWorldPlacement();
extern int MapIsLargePlacementTerrainExcluded();
extern void MapPlaceLargeLevelItems();
extern void EntityCreateLevelItem();
extern void EntityUpdateLevelItemVisibility();
extern void PresentReleaseDroppedEntities();
extern void PlayerCollectNearbyItems();
extern void PlayerCheckLevelItemHazards();
extern void PresentTryEndActivePowerup();
extern void PresentClearWorldEntityReadiness();
extern void MapUpdateItemsAndPresents();
extern int PresentStartTransientEffect();
extern int PresentUpdateTransientEffect();
#ifndef TJE_LOCAL_FUNC_015DA6
extern short PresentRegisterDecoyHit();
#endif
extern void PresentUpdateDecoy();
extern int PresentFindDoorwayOwnerAtTile();
extern void PlayerTeleportThroughDoorway();
extern void PresentUpdateDoorway();
extern void PresentUpdateBurps();
extern void PresentUpdateRainCloud();
extern void PresentUpdateBoomBox();
extern void EarthlingLaunchAimedProjectile();
extern int PresentReleaseFirstSharedTomato();
extern void EarthlingLaunchControlRemapProjectile();
extern short EntityFindProjectileTargetHit();
extern void PlayerApplyControlRemapEffect();
extern unsigned char PlayerRemapDirectionalInput();
extern void PlayerUpdateControlRemapEffect();
extern void EarthlingUpdateControlRemapProjectiles();
extern void PresentCreateActiveTerrainEffect();
extern void PresentRetireTerrainEffect();
extern void PresentCreateTemporaryTerrainEffect();
extern void PresentUpdateTerrainEffects();
extern void EarthlingUpdateControlRemapAttack();
extern void EarthlingUpdateAimedProjectileAttack();
extern void TelephonePlaceOutsidePlayerViews();
extern void TelephoneCreateDisplayEntity();
extern void TelephoneStartRing();
extern void TelephoneClearAll();

#endif
