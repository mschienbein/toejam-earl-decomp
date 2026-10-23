#ifndef TJE_RAM_UI_H
#define TJE_RAM_UI_H

#include "entities.h"

#define INVENTORY_CLOSED 0 /* ToggleInventory clears gPlayerInventoryOpen on close. */
#define INVENTORY_PRESENTS 1 /* ToggleInventory opens the ordinary present inventory. */
#define INVENTORY_MAIL_ORDER 2 /* InventoryOpenMailOrderMenu selects the mailbox offers. */
#define INVENTORY_IDENTIFY 3 /* InventoryOpenIdentificationMenu selects paid identification. */

#define INVENTORY_BANNER_OPEN 0 /* DrawInventory selects this banner for ordinary presents. */
#define INVENTORY_BANNER_DROP 1 /* ProcessInventory dispatches this banner to dropping. */
#define INVENTORY_BANNER_MAIL_ORDER 2 /* DrawInventory selects this banner for mail order. */
#define INVENTORY_BANNER_IDENTIFY 3 /* DrawInventory selects this banner for identification. */

#define INVENTORY_REDRAW_CONTENTS 1 /* InventoryServiceRedrawPhase ends with the present/offer contents. */
#define INVENTORY_REDRAW_STATS 2 /* InventoryServiceRedrawPhase draws stats, banner and HUD here. */
#define INVENTORY_REDRAW_BACKGROUND_LOWER 3 /* InventoryServiceRedrawPhase uploads each view's lower seven rows. */
#define INVENTORY_REDRAW_BACKGROUND_UPPER 4 /* InventoryServiceRedrawPhase starts with each view's upper seven rows. */

#define INVENTORY_SELECTOR_TOEJAM 26 /* InventoryInitializeDisplayEntities starts ToeJam's seven-record group here. */
#define INVENTORY_SELECTOR_EARL 33 /* InventoryInitializeDisplayEntities starts Earl's seven-record group here. */

#define INTERACTION_CHOICE_ACCEPT 0 /* CheckAButton's Up selection returns the accepting response. */
#define INTERACTION_CHOICE_DECLINE 1 /* CheckAButton's Down selection returns the declining response. */
#define INTERACTION_RESPONSE_NONE 0 /* Choice display creation and cancellation clear the response. */
#define INTERACTION_RESPONSE_DECLINE 1 /* CheckAButton returns this for the Down-selected display state. */
#define INTERACTION_RESPONSE_ACCEPT 2 /* CheckAButton returns this to the mail-order/purchase handlers. */

/* Inventory control and text state keeps its canonical byte views.
 * Incomplete outer bounds are retained, not inferred from adjacent symbols.
 * gPlayerInventoryOpen[1] also has the scalar gPlayer2InventoryOpen name in ram_system.h. */
extern char gTextFontBank;
extern char gTextFontAnimationTick;
extern char gPlayerInventoryOpen[];
extern char gPlayerInventoryScrollRow[];
extern char gPlayerInventorySelectionRow[];
extern char gPlayerInventorySelectionColumn[];
extern char gInventoryBannerMode[];
extern char gPendingUnfallPlayer;
extern short gInventorySavedCamera1X;
extern short gInventorySavedCamera1Y;
extern short gInventorySavedCamera2X;
extern short gInventorySavedCamera2Y;
extern char gInventoryRedrawPlayer;
extern char gPlayerMailboxItems[][3];
extern char gPlayerMailboxPrices[][3];
extern char gInventoryLabelsSuppressed;
extern char gTextFontBuffers[][1888];

/* Existing inventory-object aliases within gPlayerInfo[40]: selector
 * records 26 and 33, and six visible-item records 27..32 and 34..39.
 * The scalar flag bytes alias the selectors' behavior_flags fields; these are
 * overlapping views, not additional object or byte allocations. */
extern struct object_state gPlayer1InventorySelectorEntity;
extern char gPlayer1InventorySelectorFlags;
extern struct object_state gPlayer1InventoryItemEntities[6];
extern struct object_state gPlayer2InventorySelectorEntity;
extern char gPlayer2InventorySelectorFlags;
extern struct object_state gPlayer2InventoryItemEntities[6];

/* One-time present-display drawing state. */
extern char gPresentDisplayPanelDrawn;

/* Per-player two-choice dialog handles/results and input inhibition.
 * Existing incomplete bounds and independent parallel arrays are retained. */
extern char gPlayerInteractionChoiceEntities[];
extern char gPlayerInteractionResponses[];
extern char gPlayerInteractionChoiceActive[];
extern char gPlayerChoiceActionReleaseLatches[];

/* Life-request dialog state and its display object handle. */
extern short gLifeRequestPending;
extern short gLifeRequestDisplayEntity;

/* Entity handles for the main-menu cursor and heading. */
extern short gMenuCursorEntity;
extern short gMenuHeaderEntity;

#endif
