#include "elevator.h"
#include "map_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"

/* Mark tiles with 0x0800 while searching for the coordinates at ff911c/d.
 * Terrain class 0x1000 follows matching terrain or an incoming road bit;
 * class 0x2000 follows its outgoing road bits. The latter has no coordinate
 * bounds checks: [INFERENCE] map generation keeps those bits inside the map.
 */
int MapFindRouteToSecondAnchor(map_y, map_x)
short map_y, map_x;
{
    if (gPlayer1LevelMap[map_y][map_x] & 0x0800)
        return 0;
    gPlayer1LevelMap[map_y][map_x] |= 0x0800;
    if (gLevelAnchorPositions[1].y == map_y && gLevelAnchorPositions[1].x == map_x)
        return 1;
    if ((gPlayer1LevelMap[map_y][map_x] & 0x3000) == 0x1000) {
        if (map_y > 0 &&
            (gPlayer1LevelMap[map_y - 1][map_x] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~0x0800) ||
             (gPlayer1LevelMap[map_y - 1][map_x] & 4)) &&
            MapFindRouteToSecondAnchor(map_y - 1, map_x))
            return 1;
        if (map_y < 25 &&
            (gPlayer1LevelMap[map_y + 1][map_x] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~0x0800) ||
             (gPlayer1LevelMap[map_y + 1][map_x] & 1)) &&
            MapFindRouteToSecondAnchor(map_y + 1, map_x))
            return 1;
        if (map_x > 0 &&
            (gPlayer1LevelMap[map_y][map_x - 1] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~0x0800) ||
             (gPlayer1LevelMap[map_y][map_x - 1] & 2)) &&
            MapFindRouteToSecondAnchor(map_y, map_x - 1))
            return 1;
        if (map_x < 18 &&
            (gPlayer1LevelMap[map_y][map_x + 1] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~0x0800) ||
             (gPlayer1LevelMap[map_y][map_x + 1] & 8)) &&
            MapFindRouteToSecondAnchor(map_y, map_x + 1))
            return 1;
    } else if ((gPlayer1LevelMap[map_y][map_x] & 0x3000) == 0x2000) {
        if ((gPlayer1LevelMap[map_y][map_x] & 1) &&
            ((gPlayer1LevelMap[map_y - 1][map_x] & 0x3000) == 0x1000 ||
             (gPlayer1LevelMap[map_y - 1][map_x] & 4)) &&
            MapFindRouteToSecondAnchor(map_y - 1, map_x))
            return 1;
        if ((gPlayer1LevelMap[map_y][map_x] & 4) &&
            ((gPlayer1LevelMap[map_y + 1][map_x] & 0x3000) == 0x1000 ||
             (gPlayer1LevelMap[map_y + 1][map_x] & 1)) &&
            MapFindRouteToSecondAnchor(map_y + 1, map_x))
            return 1;
        if ((gPlayer1LevelMap[map_y][map_x] & 8) &&
            ((gPlayer1LevelMap[map_y][map_x - 1] & 0x3000) == 0x1000 ||
             (gPlayer1LevelMap[map_y][map_x - 1] & 2)) &&
            MapFindRouteToSecondAnchor(map_y, map_x - 1))
            return 1;
        if ((gPlayer1LevelMap[map_y][map_x] & 2) &&
            ((gPlayer1LevelMap[map_y][map_x + 1] & 0x3000) == 0x1000 ||
             (gPlayer1LevelMap[map_y][map_x + 1] & 8)) &&
            MapFindRouteToSecondAnchor(map_y, map_x + 1))
            return 1;
    }
    return 0;
}

/* Return nonzero when the elevator coordinate cannot reach ff911c/d. */
int MapIsAnchorConnectionMissing()
{
    if (MapFindRouteToSecondAnchor(gLevelAnchorPositions[0].y, gLevelAnchorPositions[0].x))
        return 0;
    else
        return 1;
}
