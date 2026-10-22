#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_map.h"
#include "assets_ui.h"
#include "core_calls.h"
#include "hardware_calls.h"
#include "map_calls.h"
#include "ram_earthlings.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "random.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Bucket a ten-bit random value with descending thresholds 256, 224, ... . */
short func_0077d8()
{
    short random_value, threshold, bucket;

    random_value = GetRandomNumber() & 0x3ff;
    threshold = 256;
    for (bucket = 0; bucket < 8; ++bucket) {
        if (random_value > threshold)
            break;
        threshold -= 32;
    }
    return bucket;
}

/* Install the nine fixed decorations in the map background plane. */
void func_007804()
{
    VDP_COMMAND = 0x65820003;
    VDP_DATA = 0x458f;
    VDP_COMMAND = 0x6c020003;
    VDP_DATA = 0x458f;
    VDP_COMMAND = 0x6d820003;
    VDP_DATA = 0x458f;
    VDP_COMMAND = 0x65d00003;
    VDP_DATA = 0x4d8f;
    VDP_COMMAND = 0x6c500003;
    VDP_DATA = 0x4d8f;
    VDP_COMMAND = 0x6dd00003;
    VDP_DATA = 0x4d8f;
    VDP_COMMAND = 0x6e140003;
    VDP_DATA = 0x459b;
    VDP_COMMAND = 0x6e380003;
    VDP_DATA = 0x459b;
    VDP_COMMAND = 0x6e3e0003;
    VDP_DATA = 0x459b;
}

/* Draw the packed font/background map and the two-tile terrain cells. */
void func_0078a8(player)
short player;
{
    short column, row;
    unsigned short left_tile, map_cell, right_tile, palette;
    unsigned short random_flips;
    short tile_number, random_tile, half;
    short level;
    unsigned short *map;
    /* The selected level byte is read but not used by the drawing loops. */

    if (!player) {
        map = gMapDataA[0];
        level = gLevel912C;
    } else {
        map = gMapDataB[0];
        level = gLevel912E;
    }
    for (row = 0; row < 28; ++row) {
        VDP_COMMAND = 0x60020003 + ((row + 1) << 23);
        for (column = 0; column < 40; ++column) {
            if (row <= 26)
                left_tile = FontBackgroundByteMap[row][column];
            else
                left_tile = FontBackgroundByteMap[row - 1][column];
            if (left_tile & 0x40)
                left_tile |= 0x800;
            if (left_tile & 0x80)
                left_tile |= 0x1000;
            left_tile &= 0xff3f;
            VDP_DATA = left_tile + 0x4580;
        }
    }
    func_007804();
    for (row = 0; row < 26; ++row) {
        VDP_COMMAND = 0x60040003 + ((row + 2) << 23);
        for (column = 0; column < 19; ++column) {
            map_cell = map[row * 19 + column];
            map_cell &= 0xff;
            if (map_cell != 34) {
                tile_number = map_cell & 0x7f;
                left_tile = tile_number * 2 + 1;
                right_tile = tile_number * 2 + 2;
                palette = 0;
                if (tile_number < 64)
                    palette = 1;
                else if (tile_number > 101)
                    palette = TileLookupAuxiliary[tile_number - 102];
                if (ram_ff91ea && (gMapDataD[row][column] & 15))
                    palette = 3;
                left_tile |= palette << 13;
                right_tile |= palette << 13;
                if (map_cell & 0x80) {
                    VDP_DATA = right_tile | 0x800;
                    VDP_DATA = left_tile | 0x800;
                } else {
                    VDP_DATA = left_tile;
                    VDP_DATA = right_tile;
                }
            } else {
                for (half = 0; half < 2; ++half) {
                    random_tile = func_0077d8();
                    if (!random_tile)
                        VDP_DATA = 0x2045;
                    else {
                        random_flips = GetRandomNumber() & 0x1800;
                        VDP_DATA = random_flips + 0x422b + random_tile - 1;
                    }
                }
            }
        }
    }
}

/* Test the clipped five-by-four map-cell area beneath one cover block. */
int func_007b24(map, cover_row, cover_column)
unsigned short *map;
int cover_row, cover_column;
{
    short row_offset, column_offset, row, column;

    if (ram_ff935a) {
        cover_row = cover_row * 4 - 1;
        cover_column = cover_column * 5 - 1;
        for (row_offset = 0; row_offset < 4; ++row_offset) {
            for (column_offset = 0; column_offset < 5; ++column_offset) {
                row = cover_row + row_offset;
                column = (cover_column + column_offset) / 2;
                if (row >= 0 && row < 26 && column >= 0 && column < 19 &&
                        (map[row * 19 + column] & 0x7f) != 34)
                    return 1;
            }
        }
    }
    return 0;
}

/* Retain revealed blocks, select additional blocks, and draw the remainder. */
void DisplayMap(map, sprites, level)
unsigned short *map;
struct hardware_sprite *sprites;
short level;
{
    short column, row, revealed = 0, traversal;
    short cover_row, cover_column;

    if (!level)
        return;
    if (ram_ff91ea)
        return;
    traversal = GetRandomNumber() & 3;
    for (row = 0; row < 7; ++row) {
        if (traversal & 1)
            row = 6 - row;
        for (column = 0; column < 8; ++column) {
            if (traversal & 2)
                column = 7 - column;
            if (!(ram_ff91ec[level][row] & (0x80 >> column))) {
                if (ram_ff92a2[level][row] & (0x80 >> column))
                    func_00815c(row, column, sprites, 1);
                else if (revealed < 8 && func_007b24(map, row, column) &&
                        PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0] % 100 < 25 - revealed) {
                    func_00815c(row, column, sprites, 0);
                    ram_ff92a2[level][row] |= 0x80 >> column;
                    ++revealed;
                }
            }
            if (traversal & 2)
                column = 7 - column;
        }
        if (traversal & 1)
            row = 6 - row;
    }
    for (row = 0; row < 7; ++row) {
        for (column = 0; column < 8; ++column) {
            if (!(ram_ff91ec[level][row] & (0x80 >> column)) &&
                    !(ram_ff92a2[level][row] & (0x80 >> column))) {
                if (revealed < 2 && func_007b24(map, row, column)) {
                    func_00815c(row, column, sprites, 0);
                    ram_ff92a2[level][row] |= 0x80 >> column;
                    ++revealed;
                } else {
                    for (cover_row = 0; cover_row < 4; ++cover_row) {
                        VDP_COMMAND = 0x50000003 + ((row * 4 + cover_row) << 23) + ((column * 5) << 17);
                        for (cover_column = 0; cover_column < 5; ++cover_column)
                            VDP_DATA = MapCoverLayout[cover_row][cover_column] + 0xc112;
                    }
                }
            }
        }
    }
}

/* Draw overlay tiles in the foreground plane, leaving a one-tile border. */
void LoadMapIntoVRAM(player, sprites)
short player;
struct hardware_sprite *sprites;
{
    short row, level;
    short column, overlay, overlay_number;
    unsigned short left_tile, right_tile;
    unsigned short *map;

    if (!player) {
        map = gMapDataA[0];
        level = gLevel912C;
    } else {
        map = gMapDataB[0];
        level = gLevel912E;
    }
    VDP_COMMAND = 0x50000003;
    for (column = 0; column < 40; ++column)
        VDP_DATA = 0x8000;
    for (row = 0; row < 26; ++row) {
        VDP_COMMAND = 0x50000003 + ((row + 1) << 23);
        VDP_DATA = 0x8000;
        for (column = 0; column < 19; ++column) {
            overlay = (map[row * 19 + column] & 0x1f00) >> 8;
            if (overlay) {
                overlay_number = overlay & 15;
                left_tile = (overlay_number + 121) * 2 + 1;
                right_tile = (overlay_number + 121) * 2 + 2;
                if (overlay & 16) {
                    VDP_DATA = right_tile | 0x8800;
                    VDP_DATA = left_tile | 0x8800;
                } else {
                    VDP_DATA = left_tile | 0x8000;
                    VDP_DATA = right_tile | 0x8000;
                }
            } else {
                VDP_DATA = 0x8000;
                VDP_DATA = 0x8000;
            }
        }
        VDP_DATA = 0x8000;
    }
    VDP_COMMAND = 0x5d800003;
    for (column = 0; column < 40; ++column)
        VDP_DATA = 0x8000;
    DisplayMap(map, sprites, level);
}

/* Copy the fixed 19-by-26 level-one map to both player map buffers. */
void LoadLevel1Tiles()
{
    short column, row;

    for (row = 0; row < 26; ++row) {
        for (column = 0; column < 19; ++column) {
            gMapDataA[row][column] = Level1MapData[row][column];
            gMapDataB[row][column] = Level1MapData[row][column];
        }
    }
}

/* Append two sprites covering one map block, retaining tiles when requested. */
int func_00815c(row, column, sprites, retain_tiles)
short row, column;
struct hardware_sprite *sprites;
short retain_tiles;
{
    if (ram_ff9358 > 78)
        return 1;
    sprites[ram_ff9358].x = column * 40 + 128;
    sprites[ram_ff9358].y = row * 32 + 128;
    if (!retain_tiles)
        sprites[ram_ff9358].tile = 0xc136;
    ++ram_ff9358;
    sprites[ram_ff9358].x = column * 40 + 160;
    sprites[ram_ff9358].y = row * 32 + 128;
    if (!retain_tiles)
        sprites[ram_ff9358].tile = 0xc146;
    ++ram_ff9358;
    return 0;
}

/* Allocate eight icon tiles and initialize all eighty sprite records. */
void func_008232(sprites)
struct hardware_sprite *sprites;
{
    short slot, sprite;
    /* The original keeps allocation slot and sprite index as separate counters. */
    unsigned short first_tile;

    first_tile = func_00d630(8);
    if (!first_tile) {
        func_008c78(0);
        first_tile = func_00d630(8);
        /* ORIGINAL BUG: after a second failure, the stronger recovery is not
         * followed by another allocation; first_tile remains zero below. */
        if (!first_tile)
            func_008c78(1);
    }
    func_00d6b2(first_tile, 8);
    ram_ff9358 = 7;
    slot = 0;
    sprite = 0;
    while (slot < 80) {
        if (slot < ram_ff9358) {
            sprites[sprite].tile = first_tile + slot;
            sprites[sprite].size = 0;
        } else if (slot & 1) {
            sprites[sprite].tile = 0xe122;
            sprites[sprite].size = 15;
        } else {
            sprites[sprite].tile = 0xe132;
            sprites[sprite].size = 3;
        }
        sprites[sprite].link = sprite + 1;
        sprites[sprite].y = 1;
        ++sprite;
        ++slot;
    }
    sprites[sprite - 1].link = 0;
}

/* Report a nonzero odd elevator state for the selected player. */
int func_008356(player)
short player;
{
    int in_elevator = 0;

    if (ELEVATOR_FLAG[player] && !((ELEVATOR_FLAG[player] - 1) & 1))
        in_elevator = 1;
    return in_elevator;
}

/* Upload each visible player's icon and position its map sprite. */
void Map_DrawPlayer(level, sprites)
int level;
struct hardware_sprite *sprites;
{
    short player, tile;
    struct object_state *position;

    for (player = 0; player <= 1; ++player) {
        position = &gPlayerInfo[player];
        if (!func_008356(player) && position->level == level) {
            tile = sprites[player].tile;
            if (!player) {
                VDP_COMMAND = 0x40000000 + (((tile << 5) & 0x3fff) << 16) + (((tile << 5) & 0xc000) >> 14);
                MoveDataToVRAM(ToejamMapIcon, 16);
            } else {
                VDP_COMMAND = 0x40000000 + (((tile << 5) & 0x3fff) << 16) + (((tile << 5) & 0xc000) >> 14);
                MoveDataToVRAM(EarlMapIcon, 16);
            }
            sprites[player].tile |= 0xe000;
            sprites[player].x = (position->x - 128) / 8 + 130;
            sprites[player].y = (position->y - 64) / 8 + 128;
        }
    }
}

/* Draw the first loaded ship-piece record on the requested level. */
void Map_DrawShipPiece(level, sprites)
short level;
struct hardware_sprite *sprites;
{
    short bank, piece, world_x, world_y;
    unsigned short tile;

    for (bank = 0; bank < 2; ++bank) {
        if (!EARTHLING_LOAD_FLAG[bank] || EARTHLING_LEVEL[bank] != level)
            continue;
        if (!EARTHLINGS_IN_LEVEL[bank][0].kind)
            piece = 0;
        else if (!EARTHLINGS_IN_LEVEL[bank][1].kind)
            piece = 1;
        else
            continue;
        world_x = EARTHLINGS_IN_LEVEL[bank][piece].x;
        world_y = EARTHLINGS_IN_LEVEL[bank][piece].y;
        tile = sprites[0].tile & 0x7ff;
        VDP_COMMAND = 0x40000000 + (((tile << 5) & 0x3fff) << 16) + (((tile << 5) & 0xc000) >> 14);
        MoveDataToVRAM(ShipPieceMapIcon, 64);
        world_x = (world_x - 128) / 8 + 128 - 2;
        world_y = (world_y - 64) / 8 + 128;
        for (piece = 0; piece < 4; piece += 2) {
            sprites[piece].tile |= 0x4000;
            sprites[piece].size = 4;
            ++sprites[piece].link;
            sprites[piece].x = world_x;
            sprites[piece].y = world_y;
        }
        return;
    }
}

/* Blink the first three map markers using bits 3 and 4 of the time counter. */
void DrawMapCharacters(sprites)
struct hardware_sprite *sprites;
{
    WaitForNextVBlank(1);
    if (TIME_COUNTER & 24) {
        VDP_COMMAND = 0x78000003L;
        VDP_DATA = sprites[0].y;
        VDP_COMMAND = 0x78080003L;
        VDP_DATA = sprites[1].y;
        VDP_COMMAND = 0x78100003L;
        VDP_DATA = sprites[2].y;
    } else {
        VDP_COMMAND = 0x78000003L;
        VDP_DATA = 1;
        VDP_COMMAND = 0x78080003L;
        VDP_DATA = 1;
        VDP_COMMAND = 0x78100003L;
        VDP_DATA = 1;
    }
}
