#include "assets_map.h"
#include "graphics_calls.h"
#include "map_calls.h"
#include "ram_map.h"

/* Read the high terrain nibble at a world position. A map coordinate of
 * zero or past the upper bound reads as terrain 0x80 on levels 1 and 26 and
 * 0x90 elsewhere ([INFERENCE] callers pass nonnegative coordinates; a
 * negative one would index the map). */
unsigned char func_00d9dc(player, world_x, world_y)
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
        ram_ffda1c = map[map_y * 19 + map_x];
        tile_number = func_001612(ram_ffda1d, tile_y, tile_x);
        tile_number &= 0x1ff;
        return PlayerTilePropertyLookup[tile_number] & 0xf0;
    }
}
