#ifndef TJE_RAM_UI_H
#define TJE_RAM_UI_H

#include "entities.h"

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
