#ifndef TJE_UI_CALLS_H
#define TJE_UI_CALLS_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void CheckBButton();
extern void CheckIfOpenForBoth();
extern int CheckPromotionScore();
extern void ClearVHBlankFlag2();
extern void DrawHUD();
extern void DrawInventory();
extern void ForceTextBackgroundColorToBlack();
extern void GamePaused();
extern short GetScoreForPromo();
extern void HealthAndLivesChecks();
extern void LoadPresentsIntoInventory();
extern short OpenPresent();
extern void InventoryDeactivateDisplayEntities();
extern void ProcessInventory();
extern void RandomizeInventory();
extern void RandomizePresents();
extern void SetMailOrderPresents();
extern void SetVHBlankFlag2();
extern void SetupTheMainGame();
extern void ToggleInventory();
extern void TextCopyFontBanksToRam();
extern int TextAnimateFontBanks();
extern void WaitForVBlankWithGraphicsPaused();
extern void EntityClearViewportSprites();
extern void InventoryServiceRedrawPhase();
extern void InventorySetCameraSuspension();
extern void InventoryRedrawSynchronously();
extern void InventoryRefreshContentsAndWait();
extern void InventoryOpenMailOrderMenu();
extern void InventoryOpenIdentificationMenu();
extern char InventoryGetSelectedPresent();
extern void InventoryDropSelectedPresent();
extern void InventoryBuySelectedMailOrderPresent();
extern int InventoryIdentifySelectedPresent();
extern char TextMapCharacterToGlyph();
extern void TextWriteStringTiles();
extern void TextWriteDecimalTiles();
extern void InventoryDrawBucks();
extern void InventoryDrawPlayerStats();
extern void InventoryUpdateSelectorEntity();
extern void InventoryInitializeDisplayEntities();
extern void InventoryDrawMailOrderOffers();
extern void InventoryDrawActionBanner();
extern void HudLoadLevelNumberSprites();
extern void HudPositionLevelNumberSprites();
extern void HudLoadActionIconSprite();
extern void HudPositionActionIconSprite();
extern int PlayerGetMaximumHealth();
extern void HudInitializeHealthBar();
extern void HudPositionHealthBarSprites();
extern void HudAnimateHealthBar();
extern void HudLoadLivesSpriteTiles();
extern void HudPositionLivesSprite();
extern void HudDrawRankLabels();
extern void HudHidePlayerSprites();
extern void PlayerResetRankAndResources();
extern void RunFinalLevelPauseMenu();

#endif
