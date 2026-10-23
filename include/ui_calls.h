#ifndef TJE_UI_CALLS_H
#define TJE_UI_CALLS_H

/* The health bar occupies two four-tile-wide hardware sprites. */
#define HUD_HEALTH_BAR_TILES 8

#define HUD_SPRITE_TOEJAM_LEVEL_UNITS 11 /* HudLoadLevelNumberSprites assigns ToeJam's units digit here. */
#define HUD_SPRITE_TOEJAM_LEVEL_TENS 12 /* HudLoadLevelNumberSprites assigns ToeJam's next sprite its tens digit. */
#define HUD_SPRITE_EARL_LEVEL_UNITS 13 /* HudLoadLevelNumberSprites assigns Earl's units digit here. */
#define HUD_SPRITE_EARL_LEVEL_TENS 14 /* HudLoadLevelNumberSprites assigns Earl's next sprite its tens digit. */
#define HUD_SPRITE_TOEJAM_HEALTH_LEFT 15 /* HudPositionHealthBarSprites positions ToeJam's first bar sprite here. */
#define HUD_SPRITE_TOEJAM_HEALTH_RIGHT 16 /* HudPositionHealthBarSprites places ToeJam's next sprite 32 pixels right. */
#define HUD_SPRITE_EARL_HEALTH_LEFT 17 /* HudPositionHealthBarSprites positions Earl's first bar sprite here. */
#define HUD_SPRITE_EARL_HEALTH_RIGHT 18 /* HudPositionHealthBarSprites places Earl's next sprite 32 pixels right. */
#define HUD_SPRITE_TOEJAM_LIVES 19 /* HudLoadLivesSpriteTiles assigns ToeJam's life-icon tiles here. */
#define HUD_SPRITE_EARL_LIVES 20 /* HudLoadLivesSpriteTiles assigns Earl's life-icon tiles here. */
#define HUD_SPRITE_TOEJAM_ACTION 21 /* HudLoadActionIconSprite assigns ToeJam's present/default icon here. */
#define HUD_SPRITE_EARL_ACTION 22 /* HudLoadActionIconSprite assigns Earl's present/default icon here. */

#define PAUSE_CHOICE_CONTINUE 0 /* RunFinalLevelPauseMenu starts here and returns without resetting. */
#define PAUSE_CHOICE_RESET 1 /* RunFinalLevelPauseMenu selects this panel before RestartFromResetEntry. */
#define PAUSE_CHOICE_HIDDEN (-1) /* RunFinalLevelPauseMenu passes this to clear the choice panel. */

/* DrawPresentDisplayPanel's byte format differs from the shared BYTEMAP_* flip encoding. */
#define PRESENT_PANEL_HORIZONTAL_FLIP 0x80 /* DrawPresentDisplayPanel expands this bit to VDP horizontal flip. */
#define PRESENT_PANEL_TILE_INDEX_MASK 0x7f /* DrawPresentDisplayPanel strips that flag before adding the tile base. */

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void CheckBButton();
extern void CheckIfOpenForBoth();
extern int CheckPromotionScore();
extern void DisableTextFontAnimationIfMenusClosed();
extern void DrawHUD();
extern void DrawInventory();
extern void ForceTextBackgroundColorToBlack();
extern void GamePaused();
extern short GetScoreForPromo();
extern void PlayerUpdateHealthAndLives();
extern void LoadPresentsIntoInventory();
extern short OpenPresent();
extern void InventoryDeactivateDisplayEntities();
extern void ProcessInventory();
extern void RandomizeInventory();
extern void RandomizePresents();
extern void SetMailOrderPresents();
extern void EnableTextFontAnimation();
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
