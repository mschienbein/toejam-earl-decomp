extern unsigned short gMapDataA[], gMapDataB[];
extern char gLevel912C, gLevel912E;
extern unsigned short ram_ffda1c;
extern unsigned char ram_ffda1d;
extern unsigned char PlayerTilePropertyLookup[398];
extern unsigned short func_001612();

/* Read the high terrain nibble at a world position. Levels 1 and 26 use
 * terrain 0x80 outside the map; the other levels use terrain 0x90. */
unsigned char func_00d9dc(player, world_x, world_y)
short player, world_x, world_y;
{
    char special_level = 0;
    short tile_y, tile_x;
    unsigned short *map;
    short map_y, map_x, tile_number;

    if (!player) {
        map = gMapDataA;
        if (gLevel912C == 1 || gLevel912C == 26)
            special_level = 1;
    } else {
        map = gMapDataB;
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
