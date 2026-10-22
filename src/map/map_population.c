extern unsigned short gMapDataA[26][19];
extern char ELEVATOR_POS_TABLE, ram_ff911b, ram_ff911c, ram_ff911d;

/* Mark tiles with 0x0800 while searching for the coordinates at ff911c/d.
 * Terrain class 0x1000 follows matching terrain or an incoming road bit;
 * class 0x2000 follows its outgoing road bits. The latter has no coordinate
 * bounds checks: [INFERENCE] map generation keeps those bits inside the map.
 */
int func_0072f0(map_y, map_x)
short map_y, map_x;
{
    if (gMapDataA[map_y][map_x] & 0x0800)
        return 0;
    gMapDataA[map_y][map_x] |= 0x0800;
    if (ram_ff911d == map_y && ram_ff911c == map_x)
        return 1;
    if ((gMapDataA[map_y][map_x] & 0x3000) == 0x1000) {
        if (map_y > 0 &&
            (gMapDataA[map_y - 1][map_x] ==
                 (gMapDataA[map_y][map_x] & ~0x0800) ||
             (gMapDataA[map_y - 1][map_x] & 4)) &&
            func_0072f0(map_y - 1, map_x))
            return 1;
        if (map_y < 25 &&
            (gMapDataA[map_y + 1][map_x] ==
                 (gMapDataA[map_y][map_x] & ~0x0800) ||
             (gMapDataA[map_y + 1][map_x] & 1)) &&
            func_0072f0(map_y + 1, map_x))
            return 1;
        if (map_x > 0 &&
            (gMapDataA[map_y][map_x - 1] ==
                 (gMapDataA[map_y][map_x] & ~0x0800) ||
             (gMapDataA[map_y][map_x - 1] & 2)) &&
            func_0072f0(map_y, map_x - 1))
            return 1;
        if (map_x < 18 &&
            (gMapDataA[map_y][map_x + 1] ==
                 (gMapDataA[map_y][map_x] & ~0x0800) ||
             (gMapDataA[map_y][map_x + 1] & 8)) &&
            func_0072f0(map_y, map_x + 1))
            return 1;
    } else if ((gMapDataA[map_y][map_x] & 0x3000) == 0x2000) {
        if ((gMapDataA[map_y][map_x] & 1) &&
            ((gMapDataA[map_y - 1][map_x] & 0x3000) == 0x1000 ||
             (gMapDataA[map_y - 1][map_x] & 4)) &&
            func_0072f0(map_y - 1, map_x))
            return 1;
        if ((gMapDataA[map_y][map_x] & 4) &&
            ((gMapDataA[map_y + 1][map_x] & 0x3000) == 0x1000 ||
             (gMapDataA[map_y + 1][map_x] & 1)) &&
            func_0072f0(map_y + 1, map_x))
            return 1;
        if ((gMapDataA[map_y][map_x] & 8) &&
            ((gMapDataA[map_y][map_x - 1] & 0x3000) == 0x1000 ||
             (gMapDataA[map_y][map_x - 1] & 2)) &&
            func_0072f0(map_y, map_x - 1))
            return 1;
        if ((gMapDataA[map_y][map_x] & 2) &&
            ((gMapDataA[map_y][map_x + 1] & 0x3000) == 0x1000 ||
             (gMapDataA[map_y][map_x + 1] & 8)) &&
            func_0072f0(map_y, map_x + 1))
            return 1;
    }
    return 0;
}

/* Return nonzero when the elevator coordinate cannot reach ff911c/d. */
int func_0077ae()
{
    if (func_0072f0(ram_ff911b, ELEVATOR_POS_TABLE))
        return 0;
    else
        return 1;
}
