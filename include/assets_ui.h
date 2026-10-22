#ifndef TJE_ASSETS_UI_H
#define TJE_ASSETS_UI_H

#include "sprites.h"
#include "ui.h"

/* Shared font, HUD, inventory, and text ROM assets. Address-spelled names
 * retain their REV 02 identities in both revision builds. */
extern unsigned char FontBackgroundByteMap[32][40];
extern short FontDataSize;
extern char HUDAdditionalTiles[];
extern char HUDAdditionalTilesSmall[];
extern unsigned char ImageLevelNumbers[10][128];
extern char ImageTextBase[];
extern long ImageTextChars1[];
extern long ImageTextChars2[];
extern short InventoryAuxiliaryTileCounts[2];
extern char InventoryAuxiliaryTiles[];
extern char InventoryAuxiliaryTilesLarge[];
extern char *InventoryCharsTable[28];
extern char InventoryCharsUnFall[13];
extern unsigned char InventoryCompactMapA[3][13];
extern unsigned char InventoryCompactMapB[3][13];
extern unsigned char InventoryCompactMapC[3][11];
extern unsigned char InventoryCompactMapD[3][22];
extern char InventoryLabelIdentify[];
extern short InventoryLabelIdentifySize;
extern char InventoryLabelOpen[];
extern short InventoryLabelOpenSize;
extern char *InventoryNameStringTable[27];
extern unsigned short InventoryVDPTilemap[][6][8];
extern unsigned char PresentDisplayTilemap[5][19];
#ifndef TJE_LOCAL_RANKCHARSTABLE
extern char *RankCharsTable[9];
#endif
extern char *RankNameStrTable[];
extern struct ride_speech SpeechStringPointers[14];
extern struct render_piece *Spr_ActionIcons[10];
extern unsigned char rom_0a836c[4][40];
/* Four-byte prefix of the observed 44-byte runtime text frame (four pieces). */
extern struct render_frame RuntimeTextMetasprite_0AFADC;

#endif
