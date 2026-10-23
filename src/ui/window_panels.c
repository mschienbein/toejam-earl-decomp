#include "hardware.h"
#include "assets_ui.h"
#include "core_calls.h"
#include "engine_calls.h"
#include "hardware_calls.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_ui.h"
#include "random.h"
#include "sprite_calls.h"

/* Release occupied effect-object slots and clear the associated per-player state. */
void ClearTransientEntities()
{
    short slot;

    for (slot = 0; slot < 12; ++slot) {
        if (gSharedProjectileEntities[slot]) {
            FreeEntity(gSharedProjectileEntities[slot]);
            gSharedProjectileEntities[slot] = 0;
        }
    }
    for (slot = 0; slot < 4; ++slot) {
        if (gControlRemapProjectileEntities[slot]) {
            FreeEntity(gControlRemapProjectileEntities[slot]);
            gControlRemapProjectileEntities[slot] = 0;
        }
    }
    for (slot = 0; slot < 6; ++slot) {
        if (gMoleEffectEntities[slot]) {
            FreeEntity(gMoleEffectEntities[slot]);
            gMoleEffectEntities[slot] = 0;
        }
    }
    for (slot = 0; slot < 4; ++slot) {
        if (gSpeechBubbleEntities[slot]) {
            FreeEntity(gSpeechBubbleEntities[slot]);
            gSpeechBubbleEntities[slot] = 0;
        }
    }
    if (gLifeRequestDisplayEntity) {
        FreeEntity(gLifeRequestDisplayEntity);
        gLifeRequestDisplayEntity = 0;
        gLifeRequestPending = 0;
    }
    for (slot = PLAYER_TOEJAM; slot <= PLAYER_EARL; ++slot) {
        if (gPlayerActiveTerrainEffectEntities[slot]) {
            FreeEntity(gPlayerActiveTerrainEffectEntities[slot]);
            gPlayerActiveTerrainEffectEntities[slot] = 0;
        }
        if (gPlayerOneShotTerrainEffectEntities[slot]) {
            FreeEntity(gPlayerOneShotTerrainEffectEntities[slot]);
            gPlayerOneShotTerrainEffectEntities[slot] = 0;
        }
        if (gPlayerTransientEffectEntities[slot]) {
            FreeEntity(gPlayerTransientEffectEntities[slot]);
            gPlayerTransientEffectEntities[slot] = 0;
        }
        if (gPlayerDecoyEntities[slot]) {
            FreeEntity(gPlayerDecoyEntities[slot]);
            gPlayerDecoyEntities[slot] = 0;
        }
        if (gPlayerPromotionDisplayEntities[slot]) {
            FreeEntity(gPlayerPromotionDisplayEntities[slot]);
            gPlayerPromotionDisplayEntities[slot] = 0;
        }
        if (gPlayerDoorwayEntities[slot]) {
            FreeEntity(gPlayerDoorwayEntities[slot]);
            gPlayerDoorwayEntities[slot] = 0;
        }
        if (gPlayerRainCloudStates[slot]) {
            FreeEntity(gPlayerRainCloudStates[slot]);
            gPlayerRainCloudStates[slot] = 0;
        }
        if (gTimedEffectEntities[slot]) {
            FreeEntity(gTimedEffectEntities[slot]);
            gTimedEffectEntities[slot] = 0;
        }
        if (gPlayerControlRemapEntities[slot]) {
            FreeEntity(gPlayerControlRemapEntities[slot]);
            gPlayerControlRemapEntities[slot] = 0;
        }
        if (gPlayerWakeEffectEntityHandles[slot]) {
            FreeEntity(gPlayerWakeEffectEntityHandles[slot]);
            gPlayerWakeEffectEntityHandles[slot] = 0;
        }
        if (gPlayerBoomBoxEntities[slot]) {
            FreeEntity(gPlayerBoomBoxEntities[slot]);
            gPlayerBoomBoxEntities[slot] = 0;
        }
        if (gPlayerInteractionChoiceEntities[slot]) {
            FreeEntity(gPlayerInteractionChoiceEntities[slot]);
            gPlayerInteractionChoiceEntities[slot] = 0;
            gPlayerInteractionChoiceActive[slot] = 0;
            gPlayerInteractionResponses[slot] = 0;
        }
        gPlayerTomatoRainTicks[slot] = 0;
    }
    PresentReleaseDroppedEntities();
    TelephoneClearAll();
}

/* Fill a 40-by-28 plane with a randomized border and a selectable interior. */
void DrawMenuWindowFrame(filled)
short filled;
{
    short row, column;
    unsigned short tile;

    for (row = 0; row < 28; ++row) {
        VDP_COMMAND = VDP_WRITE_WINDOW + ((long)row << 23);
        for (column = 0; column < 40; ++column) {
            if (row == 0 || row == 27 || column == 0 || column == 39) {
                tile = GetRandomNumber() & 3;
                tile += 41;
                if (GetRandomNumber() & 1) {
                    tile |= VDP_TILE_HORIZONTAL_FLIP;
                    tile |= VDP_TILE_VERTICAL_FLIP;
                }
                VDP_DATA = 0x8580 + tile;
            } else if (filled)
                VDP_DATA = 0x85b4;
            else
                VDP_DATA = VDP_TILE_PRIORITY;
        }
    }
    VDP_CONTROL_WORD = 0x911e;
    VDP_CONTROL_WORD = 0x921c;
}

/* Upload auxiliary tiles and draw a nested border. */
void DrawNestedWindowFrame()
{
    short row, column;
    unsigned short tile;

    VDP_COMMAND = 0x79a00000L;
    MoveDataToVRAM(InventoryAuxiliaryTiles, 80);
    VDP_COMMAND = 0x7a400000L;
    MoveDataToVRAM(InventoryAuxiliaryTilesLarge,
                   (InventoryAuxiliaryTileCounts[1] + InventoryAuxiliaryTileCounts[0]) << 4);
    for (row = 0; row < 28; ++row) {
        VDP_COMMAND = VDP_WRITE_PLANE_B + ((long)row << 23);
        for (column = 0; column < 40; ++column) {
            if (row == 0 || row == 27 || column == 0 || column == 39)
                tile = 0xe1d1;
            else if (row == 1 || row == 26 || column == 1 || column == 38)
                tile = 0xe1cd;
            else if ((column == 2 || column == 37) && row > 2 && row < 25) {
                tile = 0xe1ce;
                if (column != 2)
                    tile |= VDP_TILE_HORIZONTAL_FLIP;
            } else if ((row == 2 || row == 25) && column > 2 && column < 37) {
                tile = 0xe1cf;
                if (row != 2)
                    tile |= VDP_TILE_VERTICAL_FLIP;
            } else if (row == 2 && column == 2)
                tile = 0xe1d0;
            else if (row == 25 && column == 2)
                tile = 0xf1d0;
            else if (row == 2 && column == 37)
                tile = 0xe9d0;
            else if (row == 25 && column == 37)
                tile = 0xf9d0;
            else
                tile = VDP_TILE_PRIORITY;
            VDP_DATA = tile;
        }
    }
}

/* Draw the present display once, translating its horizontal-flip flag to VDP format. */
void DrawPresentDisplayPanel()
{
    short row, column;
    unsigned short tile;

    if (gPresentDisplayPanelDrawn)
        return;
    DisableBlankingGraphics();
    for (row = 0; row < 5; ++row) {
        VDP_COMMAND = 0x60140003L + ((row + 5) << 23);
        for (column = 0; column < 19; ++column) {
            tile = PresentDisplayTilemap[row][column];
            if (tile & 0x80) {
                tile &= 0x7f;
                tile |= VDP_TILE_HORIZONTAL_FLIP;
            }
            VDP_DATA = 0xc1d2 + tile;
        }
    }
    EnableBlankingGraphics();
    gPresentDisplayPanelDrawn = 1;
}

/* Draw one six-by-eight tilemap, or clear it for a negative selection. */
void DrawPauseChoicePanel(selection)
short selection;
{
    short row, column;
    unsigned short tile_base;

    DisableBlankingGraphics();
    /* The original subtracts this negative tile offset; adding 465 emits ADDI instead. */
    tile_base = InventoryAuxiliaryTileCounts[1] - -465;
    for (row = 0; row < 6; ++row) {
        VDP_COMMAND = 0x60200003L + ((row + 11) << 23);
        for (column = 0; column < 8; ++column) {
            if (selection < 0)
                VDP_DATA = VDP_TILE_PRIORITY;
            else
                VDP_DATA = VDP_TILE_PRIORITY + InventoryVDPTilemap[selection][row][column] + tile_base;
        }
    }
    EnableBlankingGraphics();
}
