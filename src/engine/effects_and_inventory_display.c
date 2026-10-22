#include "hardware.h"
#include "ram.h"
#include "assets.h"
#include "core_calls.h"
#include "engine_calls.h"
#include "random.h"
#include "present_calls.h"
#include "sprite_calls.h"
#include "romcalls.h"
/* Release occupied effect-object slots and clear the associated per-player state. */
void func_01d436()
{
    short slot;

    for (slot = 0; slot < 12; ++slot) {
        if (ram_ffe1c0[slot]) {
            func_00d970(ram_ffe1c0[slot]);
            ram_ffe1c0[slot] = 0;
        }
    }
    for (slot = 0; slot < 4; ++slot) {
        if (ram_ffe1d8[slot]) {
            func_00d970(ram_ffe1d8[slot]);
            ram_ffe1d8[slot] = 0;
        }
    }
    for (slot = 0; slot < 6; ++slot) {
        if (ram_ffe244[slot]) {
            func_00d970(ram_ffe244[slot]);
            ram_ffe244[slot] = 0;
        }
    }
    for (slot = 0; slot < 4; ++slot) {
        if (ram_ffe1ee[slot]) {
            func_00d970(ram_ffe1ee[slot]);
            ram_ffe1ee[slot] = 0;
        }
    }
    if (ram_ffe242) {
        func_00d970(ram_ffe242);
        ram_ffe242 = 0;
        ram_ffe240 = 0;
    }
    for (slot = 0; slot <= 1; ++slot) {
        if (ram_ffe1e4[slot]) {
            func_00d970(ram_ffe1e4[slot]);
            ram_ffe1e4[slot] = 0;
        }
        if (ram_ffe1e6[slot]) {
            func_00d970(ram_ffe1e6[slot]);
            ram_ffe1e6[slot] = 0;
        }
        if (ram_ffde56[slot]) {
            func_00d970(ram_ffde56[slot]);
            ram_ffde56[slot] = 0;
        }
        if (ram_ffde58[slot]) {
            func_00d970(ram_ffde58[slot]);
            ram_ffde58[slot] = 0;
        }
        if (ram_ffde5a[slot]) {
            func_00d970(ram_ffde5a[slot]);
            ram_ffde5a[slot] = 0;
        }
        if (ram_ffde5c[slot]) {
            func_00d970(ram_ffde5c[slot]);
            ram_ffde5c[slot] = 0;
        }
        if (gPlayer1RainCloudState[slot]) {
            func_00d970(gPlayer1RainCloudState[slot]);
            gPlayer1RainCloudState[slot] = 0;
        }
        if (ram_ffe1be[slot]) {
            func_00d970(ram_ffe1be[slot]);
            ram_ffe1be[slot] = 0;
        }
        if (ram_ffe1dc[slot]) {
            func_00d970(ram_ffe1dc[slot]);
            ram_ffe1dc[slot] = 0;
        }
        if (ram_ffda42[slot]) {
            func_00d970(ram_ffda42[slot]);
            ram_ffda42[slot] = 0;
        }
        if (ram_ffde6c[slot]) {
            func_00d970(ram_ffde6c[slot]);
            ram_ffde6c[slot] = 0;
        }
        if (ram_ffe232[slot]) {
            func_00d970(ram_ffe232[slot]);
            ram_ffe232[slot] = 0;
            ram_ffe236[slot] = 0;
            ram_ffe234[slot] = 0;
        }
        gPlayer1TomatoRainTimer[slot] = 0;
    }
    func_01528a();
    func_023012();
}

/* Fill a 40-by-28 plane with a randomized border and a selectable interior. */
void func_01d802(filled)
short filled;
{
    short row, column;
    unsigned short tile;

    for (row = 0; row < 28; ++row) {
        VDP_COMMAND = 0x50000003L + ((long)row << 23);
        for (column = 0; column < 40; ++column) {
            if (row == 0 || row == 27 || column == 0 || column == 39) {
                tile = GetRandomNumber() & 3;
                tile += 41;
                if (GetRandomNumber() & 1) {
                    tile |= 0x800;
                    tile |= 0x1000;
                }
                VDP_DATA = 0x8580 + tile;
            } else if (filled)
                VDP_DATA = 0x85b4;
            else
                VDP_DATA = 0x8000;
        }
    }
    VDP_CONTROL_WORD = 0x911e;
    VDP_CONTROL_WORD = 0x921c;
}

/* Upload auxiliary tiles and draw a nested border. */
void func_01d8ac()
{
    short row, column;
    unsigned short tile;

    VDP_COMMAND = 0x79a00000L;
    MoveDataToVRAM(InventoryAuxiliaryTiles, 80);
    VDP_COMMAND = 0x7a400000L;
    MoveDataToVRAM(InventoryAuxiliaryTilesLarge,
                   (InventoryAuxiliaryTileCounts[1] + InventoryAuxiliaryTileCounts[0]) << 4);
    for (row = 0; row < 28; ++row) {
        VDP_COMMAND = 0x60000003L + ((long)row << 23);
        for (column = 0; column < 40; ++column) {
            if (row == 0 || row == 27 || column == 0 || column == 39)
                tile = 0xe1d1;
            else if (row == 1 || row == 26 || column == 1 || column == 38)
                tile = 0xe1cd;
            else if ((column == 2 || column == 37) && row > 2 && row < 25) {
                tile = 0xe1ce;
                if (column != 2)
                    tile |= 0x800;
            } else if ((row == 2 || row == 25) && column > 2 && column < 37) {
                tile = 0xe1cf;
                if (row != 2)
                    tile |= 0x1000;
            } else if (row == 2 && column == 2)
                tile = 0xe1d0;
            else if (row == 25 && column == 2)
                tile = 0xf1d0;
            else if (row == 2 && column == 37)
                tile = 0xe9d0;
            else if (row == 25 && column == 37)
                tile = 0xf9d0;
            else
                tile = 0x8000;
            VDP_DATA = tile;
        }
    }
}

/* Draw the present display once, translating its horizontal-flip flag to VDP format. */
void func_01da0a()
{
    short row, column;
    unsigned short tile;

    if (ram_ffe1ec)
        return;
    ClearSkipVHBlankFlags();
    for (row = 0; row < 5; ++row) {
        VDP_COMMAND = 0x60140003L + ((row + 5) << 23);
        for (column = 0; column < 19; ++column) {
            tile = PresentDisplayTilemap[row][column];
            if (tile & 0x80) {
                tile &= 0x7f;
                tile |= 0x800;
            }
            VDP_DATA = 0xc1d2 + tile;
        }
    }
    SetSkipVHBlankFlags();
    ram_ffe1ec = 1;
}

/* Draw one six-by-eight tilemap, or clear it for a negative selection. */
void func_01da9c(selection)
short selection;
{
    short row, column;
    unsigned short tile_base;

    ClearSkipVHBlankFlags();
    /* The original subtracts this negative tile offset; adding 465 emits ADDI instead. */
    tile_base = InventoryAuxiliaryTileCounts[1] - -465;
    for (row = 0; row < 6; ++row) {
        VDP_COMMAND = 0x60200003L + ((row + 11) << 23);
        for (column = 0; column < 8; ++column) {
            if (selection < 0)
                VDP_DATA = 0x8000;
            else
                VDP_DATA = 0x8000 + InventoryVDPTilemap[selection][row][column] + tile_base;
        }
    }
    SetSkipVHBlankFlags();
}
