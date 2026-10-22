#include "revision.h"

struct map_sprite {
    short y;
    unsigned char size, link;
    unsigned short tile;
    short x;
};
struct map_player {
    short x, y;
    char reserved04[72], level, reserved4d[51];
};
extern unsigned char gMode2RegSoftcopy, VBLANK_FLAG_2;
extern char SPLIT_SCREEN, P1_MENU_OPEN[];
extern short P1_IDLE_COUNTER[], ram_ff935a;
extern struct map_sprite ram_ffb65a[80];
extern struct map_player gPlayerInfo[40];
extern short MapScreenTileCount;
extern unsigned char MapScreenTiles[], LevelTiles[];
extern unsigned short PaletteMapLine1[], PaletteMapLine2[], PaletteMapLine3[];
extern unsigned short PaletteGameLine1[], PaletteGameLine2[], PaletteGameLine3[];
extern void WaitForNextVBlank(), VDPWritesRoutine1(), ClearSkipVHBlankFlags();
extern void MoveDataToVRAM(), CopyPaletteToCRAMSlot(), Map_DrawElevator();
extern void func_008232(), func_0078a8(), LoadMapIntoVRAM();
extern void Map_DrawPlayer(), Map_DrawShipPiece(), PreGenerateRandoms();
extern void func_00871a(), DrawMapCharacters();
extern void func_00046a(), DrawHUD(), func_0090c4(), ShowShipPieces(), SetSkipVHBlankFlags();
extern long MovePadInputIntoD0();
#define VDP_CONTROL (*(unsigned long *)0xc00004L)
#define VDP_CONTROL_WORD (*(unsigned short *)0xc00004L)
#define VDP_DATA (*(unsigned short *)0xc00000L)

/* Show the map, wait for the selecting player to dismiss it, then restore
 * the game or ship-piece display and the normal blanking flags. */
void DrawGameMap(player)
short player;
{
    struct map_sprite sprites[80];
    short map_number, buttons;

    map_number = 0;
    if (player == 1 && SPLIT_SCREEN)
        map_number = 1;
#if REVISION >= 2
    /* REV 02 resets the opening player's idle counter; REV 00 does not. */
    P1_IDLE_COUNTER[player] = 0;
#endif
    if (VBLANK_FLAG_2) {
        while (VBLANK_FLAG_2)
            ;
    } else
        WaitForNextVBlank(1);
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    VDP_CONTROL = 0x40200000L;
    MoveDataToVRAM(MapScreenTiles, MapScreenTileCount << 4);
    func_008232(sprites);
    func_0078a8(map_number);
    LoadMapIntoVRAM(map_number, sprites);
    VDP_CONTROL = 0x40020010L;
    VDP_DATA = 8;
    VDP_CONTROL = 0x7c020003L;
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
    func_00871a(sprites, 11, 1);
    func_00871a(sprites, 0, 0);
    VDPWritesRoutine1(sprites, 0xf800, 320);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    if (ram_ff935a)
        for (map_number = 1; map_number < 12; ++map_number) {
            WaitForNextVBlank(4);
            func_00871a(sprites, map_number, 0);
        }
    PreGenerateRandoms();
    while (MovePadInputIntoD0(player) & 0x20)
        WaitForNextVBlank(1);
    while (!((buttons = MovePadInputIntoD0(player)) & 0x70))
        DrawMapCharacters(sprites);
    buttons &= 0x70;
    while (MovePadInputIntoD0(player) & buttons)
        WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    func_00046a();
    VDPWritesRoutine1(ram_ffb65a, 0xf800, 320);
    if (ram_ff935a || (buttons & 0x20)) {
        VDP_CONTROL = 0x40000000L;
        MoveDataToVRAM(LevelTiles, (MapScreenTileCount + 41) << 4);
        VDP_CONTROL_WORD = 0x9100;
        VDP_CONTROL_WORD = 0x9200;
        DrawHUD();
        CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
        CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
        CopyPaletteToCRAMSlot(PaletteGameLine3, 2);
        if (P1_MENU_OPEN[player ^ 1])
            func_0090c4(player ^ 1);
    } else
        ShowShipPieces(-1, player);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
}
