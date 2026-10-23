#include "elevator.h"
#include "map.h"
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
    if (gPlayer1LevelMap[map_y][map_x] & MAP_REGION_VISITED)
        return 0;
    gPlayer1LevelMap[map_y][map_x] |= MAP_REGION_VISITED;
    if (gLevelAnchorPositions[1].y == map_y && gLevelAnchorPositions[1].x == map_x)
        return 1;
    if ((gPlayer1LevelMap[map_y][map_x] & MAP_REGION_KIND_MASK) == MAP_REGION_BOUNDARY) {
        if (map_y > 0 &&
            (gPlayer1LevelMap[map_y - 1][map_x] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~MAP_REGION_VISITED) ||
             (gPlayer1LevelMap[map_y - 1][map_x] & MAP_PATH_DOWN)) &&
            MapFindRouteToSecondAnchor(map_y - 1, map_x))
            return 1;
        if (map_y < 25 &&
            (gPlayer1LevelMap[map_y + 1][map_x] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~MAP_REGION_VISITED) ||
             (gPlayer1LevelMap[map_y + 1][map_x] & MAP_PATH_UP)) &&
            MapFindRouteToSecondAnchor(map_y + 1, map_x))
            return 1;
        if (map_x > 0 &&
            (gPlayer1LevelMap[map_y][map_x - 1] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~MAP_REGION_VISITED) ||
             (gPlayer1LevelMap[map_y][map_x - 1] & MAP_PATH_RIGHT)) &&
            MapFindRouteToSecondAnchor(map_y, map_x - 1))
            return 1;
        if (map_x < 18 &&
            (gPlayer1LevelMap[map_y][map_x + 1] ==
                 (gPlayer1LevelMap[map_y][map_x] & ~MAP_REGION_VISITED) ||
             (gPlayer1LevelMap[map_y][map_x + 1] & MAP_PATH_LEFT)) &&
            MapFindRouteToSecondAnchor(map_y, map_x + 1))
            return 1;
    } else if ((gPlayer1LevelMap[map_y][map_x] & MAP_REGION_KIND_MASK) == MAP_REGION_PATH) {
        if ((gPlayer1LevelMap[map_y][map_x] & MAP_PATH_UP) &&
            ((gPlayer1LevelMap[map_y - 1][map_x] & MAP_REGION_KIND_MASK) == MAP_REGION_BOUNDARY ||
             (gPlayer1LevelMap[map_y - 1][map_x] & MAP_PATH_DOWN)) &&
            MapFindRouteToSecondAnchor(map_y - 1, map_x))
            return 1;
        if ((gPlayer1LevelMap[map_y][map_x] & MAP_PATH_DOWN) &&
            ((gPlayer1LevelMap[map_y + 1][map_x] & MAP_REGION_KIND_MASK) == MAP_REGION_BOUNDARY ||
             (gPlayer1LevelMap[map_y + 1][map_x] & MAP_PATH_UP)) &&
            MapFindRouteToSecondAnchor(map_y + 1, map_x))
            return 1;
        if ((gPlayer1LevelMap[map_y][map_x] & MAP_PATH_LEFT) &&
            ((gPlayer1LevelMap[map_y][map_x - 1] & MAP_REGION_KIND_MASK) == MAP_REGION_BOUNDARY ||
             (gPlayer1LevelMap[map_y][map_x - 1] & MAP_PATH_RIGHT)) &&
            MapFindRouteToSecondAnchor(map_y, map_x - 1))
            return 1;
        if ((gPlayer1LevelMap[map_y][map_x] & MAP_PATH_RIGHT) &&
            ((gPlayer1LevelMap[map_y][map_x + 1] & MAP_REGION_KIND_MASK) == MAP_REGION_BOUNDARY ||
             (gPlayer1LevelMap[map_y][map_x + 1] & MAP_PATH_LEFT)) &&
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
