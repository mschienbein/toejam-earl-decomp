#include "terrain.h"
#include "entities.h"
#include "assets_map.h"
#include "graphics_calls.h"
#include "map_calls.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_ui.h"

/* Read the high terrain nibble at a world position. A map coordinate of
 * zero or past the upper bound reads as terrain 0x80 on levels 1 and 26 and
 * 0x90 elsewhere ([INFERENCE] callers pass nonnegative coordinates; a
 * negative one would index the map). */
unsigned char GetMapTerrainAtWorldPosition(player, world_x, world_y)
short player, world_x, world_y;
{
    char special_level = 0;
    short tile_y, tile_x;
    unsigned short *map;
    short map_y, map_x, tile_number;

    if (!player) {
        map = gMapDataA[0];
        if (gLevel912C == 1 || gLevel912C == 26)
            special_level = 1;
    } else {
        map = gMapDataB[0];
        if (gLevel912E == 1 || gLevel912E == 26)
            special_level = 1;
    }
    tile_y = (world_y >> 3) & 7;
    tile_x = (world_x >> 3) & 15;
    map_y = world_y >> 6;
    map_x = world_x >> 7;
    if (!map_x || map_x > 19 || !map_y || map_y > 26) {
        if (special_level)
            return 0x80;
        else
            return 0x90;
    } else {
        --map_x;
        --map_y;
        gTerrainQueryMetatileWord = map[map_y * 19 + map_x];
        tile_number = GetLevelMetatileTile(gTerrainQueryMetatileLowByte, tile_y, tile_x);
        tile_number &= 0x1ff;
        return PlayerTilePropertyLookup[tile_number] & TERRAIN_CLASS_MASK;
    }
}

/* Refresh the five-by-five terrain cache centered on the player's tile. */
void UpdateLandAroundPlayer(player)
short player;
{
    struct object_state *entity;
    unsigned char *land;
    unsigned short *map;
    /* Matching qualification: reuse the column word for its encoded cell,
     * and one byte for overlay, base, and final terrain. These register
     * qualifications keep the word in d2 and the shared byte in d0;
     * separate byte/word temporaries spill and do not reproduce the ROM. */
    register short map_cell, tile_y, tile_x;
    register unsigned short tile_number;
    short sample_x;
    short sample_y, center_x, center_y, map_y;
    register unsigned char terrain;

    if (gPlayerLives[player] < 0 || P1_MENU_OPEN[player])
        return;
    map = &gMapDataA[0][0];
    if (!player) {
        entity = &gPlayerInfo[PLAYER_TOEJAM];
        land = &LAND_NEAR_TOEJAM[0][0];
    } else {
        entity = &gEarlPositionX;
        land = &LAND_NEAR_EARL[0][0];
        if (gPlayerLives[PLAYER_TOEJAM] >= 0 && SPLIT_SCREEN)
            map = &gMapDataB[0][0];
    }
    center_y = entity->y;
    center_y >>= 3;
    center_x = entity->x;
    center_x >>= 3;
    for (sample_y = center_y - 2; sample_y < center_y + 3; ++sample_y) {
        for (sample_x = center_x - 2; sample_x < center_x + 3; ++sample_x) {
            if (PresentFindDoorwayOwnerAtTile(entity, sample_y, sample_x))
                terrain = 16;
            else {
                tile_y = sample_y & 7;
                tile_x = sample_x & 15;
                map_y = sample_y >> 3;
                map_cell = sample_x >> 4;
                if (!map_cell || map_cell > 19 || !map_y || map_y > 26) {
                    if (entity->level == 1 || entity->level == 26)
                        terrain = 0x80;
                    else
                        terrain = 0x90;
                } else {
                    --map_cell;
                    --map_y;
                    tile_number = 0;
                    map_cell = map[map_y * 19 + map_cell];
                    terrain = ((unsigned short)map_cell & 0x1f00) >> 8;
                    if (terrain)
                        tile_number = GetOverlayMetatileTile(terrain, tile_y, tile_x);
                    if (!tile_number) {
                        terrain = map_cell;
                        tile_number = GetLevelMetatileTile(terrain, tile_y, tile_x);
                    }
                    terrain = PlayerTilePropertyLookup[tile_number & 0x7ff];
                }
            }
            *land++ = terrain;
        }
    }
}
