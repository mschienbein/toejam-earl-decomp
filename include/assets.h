#ifndef TJE_ASSETS_H
#define TJE_ASSETS_H

#include "entities.h"
#include "earthlings.h"
#include "elevator.h"
#include "map.h"
#include "presents.h"
#include "sprites.h"
#include "ui.h"

/* Shared ROM assets and descriptor tables. Address-spelled identities are
 * always REV 02 identities; the build translates them for REV 00. */
/* EARTHLING_DATA contains 25 logical 38-byte entries. Entry 24's final
 * reserved byte overlaps EarthlingCoordinatePairs[0]; asset partitions are
 * not logical array bounds. */
extern struct timed_animation AnimationDescriptors_0AEF9C[9];
extern struct animation_descriptor AnimationDescriptors_0AEFF6[3];
extern struct timed_animation AnimationDescriptors_0AF00E[5];
extern struct timed_animation AnimationDescriptors_0AF076[3];
extern struct animation_descriptor AnimationDescriptors_0AF0A8[2];
extern struct timed_animation AnimationDescriptors_0AF0B8[2];
extern struct animation_descriptor AnimationDescriptors_0AF0D0[1];
extern struct earthling_definition EARTHLING_DATA[25];
extern unsigned short EarlMapIcon[16];
extern char EarthlingCoordinatePairs[];
extern unsigned short ElevatorMapIcon[16];
extern unsigned short FixedWorldSeeds[26];
extern unsigned char FontBackgroundByteMap[32][40];
extern short FontDataSize;
extern struct render_frame *FramePointers_AF09C[3];
extern unsigned char FunkatronExtraTiles[];
extern unsigned short FunkatronMapData[26][19];
extern unsigned short FunkatronPaletteA[];
extern unsigned short FunkatronPaletteB[];
extern struct scene_descriptor FunkatronSceneDescriptors[30];
extern struct scene_placement FunkatronScenePlacements[277];
extern char HUDAdditionalTiles[];
extern char HUDAdditionalTilesSmall[];
extern unsigned char ImageLevelNumbers[10][128];
extern char ImageTextBase[];
extern long ImageTextChars1[];
extern long ImageTextChars2[];
extern char ImgStarfieldLarge[];
extern char InlineAnimationPrograms_AF0CC[4];
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
extern unsigned char Level0MapData[5][4];
extern unsigned char Level1MapData[26][19];
extern struct item_position Level1PresentLocations[12];
extern char LevelEarthlingChoices[26][20];
extern unsigned short LevelMetatileMaps[122][8][16];
extern unsigned char LevelNumberPalettes[10];
extern unsigned char LevelTiles[];
extern short LevelTilesSize;
extern unsigned char MapCoverLayout[4][5];
extern struct map_generation_tables MapGenerationLookupTables;
extern short MapScreenTileCount;
extern unsigned char MapScreenTiles[];
extern struct map_transition_descriptor MapTransitionDescriptors[16];
extern unsigned short OverlayMetatileMaps[15][8][16];
extern short PaletteAnimationFramesA[4][4];
extern short PaletteAnimationFramesB[4][4];
extern unsigned short PaletteGameLine1[];
extern unsigned short PaletteGameLine2[];
extern unsigned short PaletteGameLine3[];
extern unsigned short PaletteGameLine4[];
extern unsigned short PaletteMapLine1[];
extern unsigned short PaletteMapLine2[];
extern unsigned short PaletteMapLine3[];
extern unsigned short PaletteShipScreen_bank3[];
extern unsigned short PaletteTitleScreen_bank1[];
extern struct timed_animation PlayerAnimationDescriptors[121];
extern unsigned char PlayerTilePropertyLookup[398];
extern char PresentAuxiliarySignedLookup[16];
extern unsigned char PresentDisplayTilemap[5][19];
extern struct render_frame *PresentSpriteTable[28];
extern char PresentWeightTable[27];
extern char ProjectilePlacementTriples[][2][4][3];
#ifndef TJE_LOCAL_RANKCHARSTABLE
extern char *RankCharsTable[9];
#endif
extern char *RankNameStrTable[];
extern char Sfx_Aloha[];
extern char Sfx_BoogieBoogie[];
extern char Sfx_Burp[];
extern char Sfx_ChickenCluck[];
extern char Sfx_ChowDown[];
extern char Sfx_CupidSing[];
extern char Sfx_DentistLaugh[];
extern char Sfx_ExtraLifeGlitz[];
extern char Sfx_Falling[];
extern char Sfx_IceCreamHonk[];
extern char Sfx_JamClap[];
extern char Sfx_JamSnareDrum[];
extern char Sfx_Money[];
extern char Sfx_Randomize[];
extern char Sfx_Running[];
extern char Sfx_Snoring[];
extern char Sfx_Splash[];
extern char Sfx_Springing[];
extern char Sfx_Squish[];
extern char Sfx_Teetering1[];
extern char Sfx_Teetering2[];
extern char Sfx_Teetering3[];
extern char Sfx_Telephone[];
extern char Sfx_WakeUpShout[];
extern char Sfx_WakeUpWhisper[];
extern char Sfx_YeahAllRight[];
extern unsigned short ShipPieceMapIcon[64];
extern unsigned char ShipRelatedData[8][8];
extern struct animation_descriptor SleepAnimationDescriptors[23];
extern struct ride_speech SpeechStringPointers[14];
extern struct render_piece *Spr_ActionIcons[10];
extern struct render_frame *Spr_GroundItem[17];
extern struct render_frame *Spr_PresentSelector[1];
extern struct render_frame *Spr_TransitionBackground[50];
extern unsigned char SpritePriorityLookup[26];
extern unsigned char SpriteTileAllocationLookup[8];
extern short StarryBackgroundSize;
#ifndef TJE_LOCAL_TILEALTERNATIVEPOINTERS
extern unsigned char *TileAlternativePointers[98];
#endif
extern unsigned char TileLookupAuxiliary[20];
extern unsigned short ToejamMapIcon[16];
extern unsigned char dat_08bf5a[20];
extern unsigned char dat_08bf6e[20];
extern unsigned char dat_08bfc2[];
extern char dat_08bfce[23];
extern unsigned char dat_08bfe6[4][10];
extern char dat_0ae4a0[];
extern struct timed_animation dat_0aee98[13];
extern struct timed_animation dat_0aef1a[13];
extern struct timed_animation dat_0aefa6[2];
extern struct timed_animation dat_0aefba[];
extern struct timed_animation dat_0aefc4[];
extern struct timed_animation dat_0aefce[];
extern struct timed_animation dat_0aefd8[];
extern struct timed_animation dat_0aefec;
extern struct animation_descriptor dat_0aeffe;
extern struct animation_descriptor dat_0af006;
extern struct timed_animation dat_0af018;
extern struct timed_animation dat_0af022;
extern struct timed_animation dat_0af02c;
extern struct timed_animation dat_0af036;
extern struct timed_animation dat_0af05a[];
extern struct timed_animation dat_0af080;
extern struct timed_animation dat_0af08a;
extern struct animation_descriptor dat_0af0b0;
extern unsigned char rom_0a836c[4][40];
extern char rom_0ae4a0[];
extern struct timed_animation rom_0ae9de[121];

#endif
