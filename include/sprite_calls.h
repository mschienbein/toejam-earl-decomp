#ifndef TJE_SPRITE_CALLS_H
#define TJE_SPRITE_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void AppendSpriteTilesToBuffer();
extern void ReleaseEntityViewportSprites();
extern void UpdateEntityShadows();
extern void ReleaseEntityShadows();
extern void Process2FEntityFlag();
extern void SetRAMIndexes();
extern void SetSprite();
extern void SpriteLoadingStuff();
extern void SpriteUnpackInterleavedRLE();
extern void SpriteUnpackRLE();
#ifndef TJE_LOCAL_FUNC_00BDD8
extern short IsEntityVisibleInViewport();
#endif
extern void UpdateEntityViewportSprites();
extern void UpdateAllEntityViewportSprites();
extern void UpdateAttachedEntityPosition();
extern void PositionEntitySprites();
extern void BuildViewportSpriteLinks();
extern void SortEntitiesForSpriteDraw();
extern int QueueSpritePieceTiles();
extern short ClipSpritePixelsBelowCutoff();
extern void ClearSpriteMaskPixels();
extern void PrepareEntitySpriteFrame();
extern void ProcessEntityAnimationMotion();
extern void UpdateAndQueueEntitySprites();
extern void SelectSpriteChainStart();
extern void TerminateSpriteChainAtHead();
extern void TerminatePrimarySpriteChain();
extern void SelectPrimaryViewportSprites();
extern void SelectSecondaryViewportSprites();
extern void LinkPlayerSpritesForSplitWipe();
extern void HideFlaggedPlayerSprites();
extern int AllocateSpriteSlots();
extern char AllocateSpriteSlot();
extern void FreeSpriteSlots();
extern void FreeSpriteSlot();
extern int IsSpriteSlotNonReclaimable();
extern short AllocateSpriteTileRun();
extern void FreeSpriteTileRun();
extern void AllocateEntityViewportSprites();
extern void ResetEntityRuntimeState();
#ifndef TJE_LOCAL_FUNC_00D904
extern short AllocateEntity();
#endif
extern void FreeEntity();

#endif
