#include "terrain.h"
#include "hardware.h"
#include "entities.h"
#include "assets_map.h"
#include "graphics_calls.h"
#include "map_calls.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_ui.h"

/* Read the high terrain nibble at a world position. Zero or an upper-bound
 * overflow returns water on levels 1/26, void elsewhere.
 * ORIGINAL BUG: negative map coordinates are not rejected before indexing.
 * Whether an original caller can supply them is unknown [INFERENCE]. */
unsigned char GetMapTerrainAtWorldPosition(player, world_x, world_y)
short player, world_x, world_y;
{
    char special_level = 0;
    short tile_y, tile_x;
    unsigned short *map;
    short map_y, map_x, tile_number;

    if (!player) {
        map = gPlayer1LevelMap[0];
        if (gPlayer1MapLevel == 1 || gPlayer1MapLevel == LEVEL_FINAL)
            special_level = 1;
    } else {
        map = gPlayer2LevelMap[0];
        if (gPlayer2MapLevel == 1 || gPlayer2MapLevel == LEVEL_FINAL)
            special_level = 1;
    }
    tile_y = (world_y >> 3) & 7;
    tile_x = (world_x >> 3) & 15;
    map_y = world_y >> 6;
    map_x = world_x >> 7;
    if (!map_x || map_x > MAP_COLUMNS || !map_y || map_y > MAP_ROWS) {
        if (special_level)
            return TERRAIN_WATER;
        else
            return TERRAIN_VOID;
    } else {
        --map_x;
        --map_y;
        gTerrainQueryMetatileWord = map[map_y * MAP_COLUMNS + map_x];
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
    /* MATCHING QUALIFICATION: reuse the column word for its encoded cell,
     * and one byte for overlay, base, and final terrain. These register
     * qualifications keep the word in d2 and the shared byte in d0;
     * separate byte/word temporaries spill and do not reproduce the ROM. */
    register short map_cell, tile_y, tile_x;
    register unsigned short tile_number;
    short sample_x;
    short sample_y, center_x, center_y, map_y;
    register unsigned char terrain;

    if (gPlayerLives[player] < 0 || gPlayerInventoryOpen[player])
        return;
    map = &gPlayer1LevelMap[0][0];
    if (!player) {
        entity = &gPlayerInfo[PLAYER_TOEJAM];
        land = &gPlayer1NearbyTerrain[0][0];
    } else {
        entity = &gPlayer2Entity;
        land = &gPlayer2NearbyTerrain[0][0];
        if (gPlayerLives[PLAYER_TOEJAM] >= 0 && gSplitScreenEnabled)
            map = &gPlayer2LevelMap[0][0];
    }
    center_y = entity->y;
    center_y >>= 3;
    center_x = entity->x;
    center_x >>= 3;
    for (sample_y = center_y - 2; sample_y < center_y + 3; ++sample_y) {
        for (sample_x = center_x - 2; sample_x < center_x + 3; ++sample_x) {
            if (PresentFindDoorwayOwnerAtTile(entity, sample_y, sample_x))
                terrain = TERRAIN_GROUND;
            else {
                tile_y = sample_y & 7;
                tile_x = sample_x & 15;
                map_y = sample_y >> 3;
                map_cell = sample_x >> 4;
                if (!map_cell || map_cell > MAP_COLUMNS || !map_y || map_y > MAP_ROWS) {
                    if (entity->level == 1 || entity->level == LEVEL_FINAL)
                        terrain = TERRAIN_WATER;
                    else
                        terrain = TERRAIN_VOID;
                } else {
                    --map_cell;
                    --map_y;
                    tile_number = 0;
                    map_cell = map[map_y * MAP_COLUMNS + map_cell];
                    terrain = ((unsigned short)map_cell & METATILE_OVERLAY_MASK) >> 8;
                    if (terrain)
                        tile_number = GetOverlayMetatileTile(terrain, tile_y, tile_x);
                    if (!tile_number) {
                        terrain = map_cell;
                        tile_number = GetLevelMetatileTile(terrain, tile_y, tile_x);
                    }
                    terrain = PlayerTilePropertyLookup[tile_number & VDP_TILE_INDEX_MASK];
                }
            }
            *land++ = terrain;
        }
    }
}
