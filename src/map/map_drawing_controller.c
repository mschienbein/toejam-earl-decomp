#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "revision.h"
#include "assets_map.h"
#include "assets_palettes.h"
#include "core_calls.h"
#include "hardware_calls.h"
#include "map_calls.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "ship.h"
#include "ui_calls.h"

/* Show the map, wait for the selecting player to dismiss it, then restore
 * the game or ship-piece display and the normal blanking flags. */
void DrawGameMap(player)
short player;
{
    struct hardware_sprite sprites[80];
    short map_number, buttons;

    map_number = 0;
    if (player == 1 && gSplitScreenEnabled)
        map_number = 1;
#if REVISION >= 2
    /* REV 02 resets the opening player's idle counter; REV 00 does not. */
    gPlayerIdleTicks[player] = 0;
#endif
    if (gSpriteGraphicsUploadPending) {
        while (gSpriteGraphicsUploadPending)
            ;
    } else
        WaitForNextVBlank(1);
    DisableBlankingGraphics();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    VDP_COMMAND = 0x40200000L;
    MoveDataToVRAM(MapScreenTiles, MapScreenTileCount << 4);
    MapInitializeScreenSprites(sprites);
    MapDrawBackgroundAndTerrain(map_number);
    LoadMapIntoVRAM(map_number, sprites);
    VDP_COMMAND = 0x40020010L;
    VDP_DATA = 8;
    VDP_COMMAND = 0x7c020003L;
    VDP_DATA = -8;
    VDP_CONTROL_WORD = 0x911e;
    VDP_CONTROL_WORD = 0x921c;
    CopyPaletteToCRAMSlot(PaletteMapLine1, 0);
    CopyPaletteToCRAMSlot(PaletteMapLine2, 1);
    CopyPaletteToCRAMSlot(PaletteMapLine3, 2);
    map_number = gPlayerInfo[player].level;
    Map_DrawPlayer(map_number, sprites);
    Map_DrawShipPiece(map_number, &sprites[2]);
    Map_DrawElevator(map_number, &sprites[6]);
    MapUploadCoverAnimationFrame(sprites, 11, 1);
    MapUploadCoverAnimationFrame(sprites, 0, 0);
    DmaCopyWordsToVram(sprites, 0xf800, 320);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    if (gMapRevealMode)
        for (map_number = 1; map_number < 12; ++map_number) {
            WaitForNextVBlank(4);
            MapUploadCoverAnimationFrame(sprites, map_number, 0);
        }
    PreGenerateRandoms();
    while (ReadMappedPlayerInput(player) & 0x20)
        WaitForNextVBlank(1);
    while (!((buttons = ReadMappedPlayerInput(player)) & 0x70))
        DrawMapCharacters(sprites);
    buttons &= 0x70;
    while (ReadMappedPlayerInput(player) & buttons)
        WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    DrawStarfieldPlane();
    DmaCopyWordsToVram(gHardwareSpriteTable, 0xf800, 320);
    if (gMapRevealMode || (buttons & 0x20)) {
        VDP_COMMAND = VDP_WRITE_VRAM_BASE;
        MoveDataToVRAM(LevelTiles, (MapScreenTileCount + 41) << 4);
        VDP_CONTROL_WORD = 0x9100;
        VDP_CONTROL_WORD = 0x9200;
        DrawHUD();
        CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
        CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
        CopyPaletteToCRAMSlot(PaletteGameLine3, 2);
        if (gPlayerInventoryOpen[player ^ 1])
            InventoryRedrawSynchronously(player ^ 1);
    } else
        ShowShipPieces(-1, player);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    EnableBlankingGraphics();
}
