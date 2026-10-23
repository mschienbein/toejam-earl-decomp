#include "earthling_types.h"
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
short MapChooseBackgroundTileBucket()
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
void MapDrawFixedBackgroundDecorations()
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
void MapDrawBackgroundAndTerrain(player)
short player;
{
    short column, row;
    unsigned short left_tile, map_cell, right_tile, palette;
    unsigned short random_flips;
    short tile_number, random_tile, half;
    short level;
    unsigned short *map;
    /* MATCHING QUALIFICATION: assign the selected level to unused short
     * level. Removing either read changes both revisions' drawing output. */

    if (!player) {
        map = gPlayer1LevelMap[0];
        level = gPlayer1MapLevel;
    } else {
        map = gPlayer2LevelMap[0];
        level = gPlayer2MapLevel;
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
    MapDrawFixedBackgroundDecorations();
    for (row = 0; row < MAP_ROWS; ++row) {
        VDP_COMMAND = 0x60040003 + ((row + 2) << 23);
        for (column = 0; column < MAP_COLUMNS; ++column) {
            map_cell = map[row * MAP_COLUMNS + column];
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
                if (gMapDirectionalPathMode && (gMapPathDirectionBanks[row][column] & 15))
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
                    random_tile = MapChooseBackgroundTileBucket();
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
int MapCoverBlockContainsTerrain(map, cover_row, cover_column)
unsigned short *map;
int cover_row, cover_column;
{
    short row_offset, column_offset, row, column;

    if (gMapRevealMode) {
        cover_row = cover_row * 4 - 1;
        cover_column = cover_column * 5 - 1;
        for (row_offset = 0; row_offset < 4; ++row_offset) {
            for (column_offset = 0; column_offset < 5; ++column_offset) {
                row = cover_row + row_offset;
                column = (cover_column + column_offset) / 2;
                if (row >= 0 && row < MAP_ROWS && column >= 0 && column < MAP_COLUMNS &&
                        (map[row * MAP_COLUMNS + column] & 0x7f) != 34)
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
    if (gMapDirectionalPathMode)
        return;
    traversal = GetRandomNumber() & 3;
    for (row = 0; row < 7; ++row) {
        if (traversal & 1)
            row = 6 - row;
        for (column = 0; column < 8; ++column) {
            if (traversal & 2)
                column = 7 - column;
            if (!(gMapVisitedCoverBits[level][row] & (0x80 >> column))) {
                if (gMapRevealedCoverBits[level][row] & (0x80 >> column))
                    MapAppendCoverBlockSprites(row, column, sprites, 1);
                else if (revealed < 8 && MapCoverBlockContainsTerrain(map, row, column) &&
                        gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] % 100 < 25 - revealed) {
                    MapAppendCoverBlockSprites(row, column, sprites, 0);
                    gMapRevealedCoverBits[level][row] |= 0x80 >> column;
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
            if (!(gMapVisitedCoverBits[level][row] & (0x80 >> column)) &&
                    !(gMapRevealedCoverBits[level][row] & (0x80 >> column))) {
                if (revealed < 2 && MapCoverBlockContainsTerrain(map, row, column)) {
                    MapAppendCoverBlockSprites(row, column, sprites, 0);
                    gMapRevealedCoverBits[level][row] |= 0x80 >> column;
                    ++revealed;
                } else {
                    for (cover_row = 0; cover_row < 4; ++cover_row) {
                        VDP_COMMAND = VDP_WRITE_WINDOW + ((row * 4 + cover_row) << 23) + ((column * 5) << 17);
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
        map = gPlayer1LevelMap[0];
        level = gPlayer1MapLevel;
    } else {
        map = gPlayer2LevelMap[0];
        level = gPlayer2MapLevel;
    }
    VDP_COMMAND = VDP_WRITE_WINDOW;
    for (column = 0; column < 40; ++column)
        VDP_DATA = 0x8000;
    for (row = 0; row < MAP_ROWS; ++row) {
        VDP_COMMAND = VDP_WRITE_WINDOW + ((row + 1) << 23);
        VDP_DATA = 0x8000;
        for (column = 0; column < MAP_COLUMNS; ++column) {
            overlay = (map[row * MAP_COLUMNS + column] & 0x1f00) >> 8;
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

    for (row = 0; row < MAP_ROWS; ++row) {
        for (column = 0; column < MAP_COLUMNS; ++column) {
            gPlayer1LevelMap[row][column] = Level1MapData[row][column];
            gPlayer2LevelMap[row][column] = Level1MapData[row][column];
        }
    }
}

/* Append two sprites covering one map block, retaining tiles when requested. */
int MapAppendCoverBlockSprites(row, column, sprites, retain_tiles)
short row, column;
struct hardware_sprite *sprites;
short retain_tiles;
{
    if (gMapScreenNextSpriteIndex > 78)
        return 1;
    sprites[gMapScreenNextSpriteIndex].x = column * 40 + 128;
    sprites[gMapScreenNextSpriteIndex].y = row * 32 + 128;
    if (!retain_tiles)
        sprites[gMapScreenNextSpriteIndex].tile = 0xc136;
    ++gMapScreenNextSpriteIndex;
    sprites[gMapScreenNextSpriteIndex].x = column * 40 + 160;
    sprites[gMapScreenNextSpriteIndex].y = row * 32 + 128;
    if (!retain_tiles)
        sprites[gMapScreenNextSpriteIndex].tile = 0xc146;
    ++gMapScreenNextSpriteIndex;
    return 0;
}

/* Allocate eight icon tiles and initialize all eighty sprite records. */
void MapInitializeScreenSprites(sprites)
struct hardware_sprite *sprites;
{
    short slot, sprite;
    /* MATCHING QUALIFICATION: retain slot and sprite as lockstep counters;
     * a one-counter simplification changes both revisions' output. */
    unsigned short first_tile;

    first_tile = AllocateSpriteTileRun(8);
    if (!first_tile) {
        EntityClearViewportSprites(0);
        first_tile = AllocateSpriteTileRun(8);
        /* ORIGINAL BUG: after a second failure, the stronger recovery is not
         * followed by another allocation; first_tile remains zero below. */
        if (!first_tile)
            EntityClearViewportSprites(1);
    }
    FreeSpriteTileRun(first_tile, 8);
    gMapScreenNextSpriteIndex = 7;
    slot = 0;
    sprite = 0;
    while (slot < 80) {
        if (slot < gMapScreenNextSpriteIndex) {
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
int PlayerHasOddElevatorState(player)
short player;
{
    int in_elevator = 0;

    if (gPlayerElevatorFlags[player] && !((gPlayerElevatorFlags[player] - 1) & 1))
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

    for (player = 0; player <= PLAYER_EARL; ++player) {
        position = &gPlayerInfo[player];
        if (!PlayerHasOddElevatorState(player) && position->level == level) {
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
        if (!gEarthlingBankUsers[bank] || gEarthlingBankLevels[bank] != level)
            continue;
        if (!gEarthlingBanks[bank][0].kind)
            piece = 0;
        else if (!gEarthlingBanks[bank][1].kind)
            piece = 1;
        else
            continue;
        world_x = gEarthlingBanks[bank][piece].x;
        world_y = gEarthlingBanks[bank][piece].y;
        tile = sprites[0].tile & VDP_TILE_INDEX_MASK;
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
    if (gFrameCounter & 24) {
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


#include "sprites.h"
#include "sprite_frame_views.h"
#include "assets_map.h"
#include "ram_sprites.h"
#include "sprite_calls.h"
#include "core_calls.h"
#include "hardware_calls.h"
#include "map_calls.h"

/* Unpack both pieces of a map-uncover frame, queue their tile transfer, and
 * copy the supplied sprite table to VRAM. Frame seven changes palette bits. */
void MapUploadCoverAnimationFrame(sprites, frame_number, use_first_tiles)
struct hardware_sprite *sprites;
short frame_number, use_first_tiles;
{
    char flags;
    short tile_count;
    short sprite_number;
    struct render_piece *piece;
    struct render_frame *frame;

    frame = Spr_MapTileUncover[frame_number];
    if (frame_number == 7)
        for (sprite_number = 7; sprite_number < HARDWARE_SPRITE_COUNT; ++sprite_number)
            sprites[sprite_number].tile |= VDP_TILE_PALETTE_3;
    for (sprite_number = 0; sprite_number < MAP_UNCOVER_PIECE_COUNT; ++sprite_number) {
        /* The shared renderer's twelve-slot view preserves word indexing;
         * twelve is not asserted to be this ROM frame's piece extent. */
        piece = &((struct render_frame_piece_view *)frame)->pieces[sprite_number];
        flags = piece->tile_format;
        tile_count = piece->width * piece->height;
        if (flags < 0)
            SpriteUnpackInterleavedRLE(piece, tile_count);
        else
            SpriteUnpackRLE(piece, tile_count);
        gSpriteTileBufferOffset += tile_count << 5;
    }
    if (use_first_tiles)
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 0x122;
    else
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 0x136;
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 20;
    WaitForNextVBlank(1);
    DmaCopyWordsToVram(&sprites[0], 0xf800, 320);
    UploadQueuedSpriteGraphics(1, 1);
}


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
    struct hardware_sprite sprites[HARDWARE_SPRITE_COUNT];
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
        for (map_number = 1; map_number < MAP_UNCOVER_FRAME_COUNT; ++map_number) {
            WaitForNextVBlank(4);
            MapUploadCoverAnimationFrame(sprites, map_number, 0);
        }
    PreGenerateRandoms();
    while (ReadMappedPlayerInput(player) & INPUT_BUTTON_C)
        WaitForNextVBlank(1);
    while (!((buttons = ReadMappedPlayerInput(player)) & 0x70))
        DrawMapCharacters(sprites);
    buttons &= INPUT_ACTION_MASK;
    while (ReadMappedPlayerInput(player) & buttons)
        WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    DrawStarfieldPlane();
    DmaCopyWordsToVram(gHardwareSpriteTable, 0xf800, 320);
    if (gMapRevealMode || (buttons & INPUT_BUTTON_C)) {
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
