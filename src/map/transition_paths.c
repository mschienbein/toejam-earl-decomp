#include "elevator.h"
#include "map.h"
#include "assets_map.h"
#include "map_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"

/* ROM descriptors contain a count-prefixed byte list at offset two. */

int MapTransitionContainsMetatile(list_number, value)
unsigned char list_number, value;
{
    unsigned char *list;
    short index;

    list = MapTransitionDescriptors[list_number - 1].shapes;
    for (index = 1; index <= list[0]; ++index)
        if (list[index] == value)
            return 1;
    return 0;
}

/* Walk candidate map transitions recursively. An accepted tile receives its
 * transition number in bits 8 through 12. Elevator coordinates and tiles that
 * already have those bits set are not visited. */
int MapTraceTransitionChain(row, column, incoming, outgoing)
short row, column, incoming, outgoing;
{
    if ((gPlayer1LevelMap[row][column] & MAP_TRANSITION_MASK) ||
        (gLevelAnchorPositions[0].x == column && gLevelAnchorPositions[0].y == row) ||
        (gLevelAnchorPositions[1].x == column && gLevelAnchorPositions[1].y == row) ||
        (gLevelAnchorPositions[2].x == column && gLevelAnchorPositions[2].y == row))
        return 0;
    ++gTransitionPathVisitCount;
    if (incoming == MAP_PATH_LEFT && outgoing == MAP_PATH_LEFT) {
        if (MapTransitionContainsMetatile(3, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row, column - 1, MAP_PATH_LEFT, MAP_PATH_LEFT)) {
            gPlayer1LevelMap[row][column] |= 0x300;
            return 1;
        }
        if (MapTransitionContainsMetatile(4, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row + 1, column, MAP_PATH_DOWN, MAP_PATH_LEFT)) {
            gPlayer1LevelMap[row][column] |= 0x400;
            return 1;
        }
        if (MapTransitionContainsMetatile(2, gPlayer1LevelMap[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gPlayer1LevelMap[row][column] |= 0x200;
            return 1;
        }
    } else if (incoming == MAP_PATH_RIGHT && outgoing == MAP_PATH_RIGHT) {
        if (MapTransitionContainsMetatile(5, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row, column + 1, MAP_PATH_RIGHT, MAP_PATH_RIGHT)) {
            gPlayer1LevelMap[row][column] |= 0x500;
            return 1;
        }
        if (MapTransitionContainsMetatile(6, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row + 1, column, MAP_PATH_DOWN, MAP_PATH_RIGHT)) {
            gPlayer1LevelMap[row][column] |= 0x600;
            return 1;
        }
        if (MapTransitionContainsMetatile(1, gPlayer1LevelMap[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gPlayer1LevelMap[row][column] |= 0x100;
            return 1;
        }
    } else if (incoming == MAP_PATH_DOWN && outgoing == MAP_PATH_LEFT) {
        if (MapTransitionContainsMetatile(7, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row, column - 1, MAP_PATH_LEFT, MAP_PATH_DOWN)) {
            gPlayer1LevelMap[row][column] |= 0x700;
            return 1;
        }
        if (MapTransitionContainsMetatile(8, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row + 1, column, MAP_PATH_DOWN, MAP_PATH_RIGHT)) {
            gPlayer1LevelMap[row][column] |= 0x800;
            return 1;
        }
    } else if (incoming == MAP_PATH_DOWN && outgoing == MAP_PATH_RIGHT) {
        if (MapTransitionContainsMetatile(9, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row, column + 1, MAP_PATH_RIGHT, MAP_PATH_DOWN)) {
            gPlayer1LevelMap[row][column] |= 0x900;
            return 1;
        }
        if (MapTransitionContainsMetatile(10, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row + 1, column, MAP_PATH_DOWN, MAP_PATH_LEFT)) {
            gPlayer1LevelMap[row][column] |= 0xa00;
            return 1;
        }
    } else if (incoming == MAP_PATH_LEFT && outgoing == MAP_PATH_DOWN) {
        if (MapTransitionContainsMetatile(11, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row, column - 1, MAP_PATH_LEFT, MAP_PATH_LEFT)) {
            gPlayer1LevelMap[row][column] |= 0xb00;
            return 1;
        }
        if (MapTransitionContainsMetatile(12, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row + 1, column, MAP_PATH_DOWN, MAP_PATH_LEFT)) {
            gPlayer1LevelMap[row][column] |= 0xc00;
            return 1;
        }
        if (MapTransitionContainsMetatile(13, gPlayer1LevelMap[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gPlayer1LevelMap[row][column] |= 0xd00;
            return 1;
        }
    } else if (incoming == MAP_PATH_RIGHT && outgoing == MAP_PATH_DOWN) {
        if (MapTransitionContainsMetatile(14, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row, column + 1, MAP_PATH_RIGHT, MAP_PATH_RIGHT)) {
            gPlayer1LevelMap[row][column] |= 0xe00;
            return 1;
        }
        if (MapTransitionContainsMetatile(15, gPlayer1LevelMap[row][column]) &&
            MapTraceTransitionChain(row + 1, column, MAP_PATH_DOWN, MAP_PATH_RIGHT)) {
            gPlayer1LevelMap[row][column] |= 0xf00;
            return 1;
        }
        if (MapTransitionContainsMetatile(16, gPlayer1LevelMap[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gPlayer1LevelMap[row][column] |= 0x1000;
            return 1;
        }
    }
    --gTransitionPathVisitCount;
    return 0;
}

/* Clear transition markings, then scan twice for a chain. The first scan
 * requires 25 visits; the second lowers the requirement to 16. */
void MapAssignTransitionChain()
{
    short column, row, pass;

    for (row = 0; row < MAP_ROWS; ++row)
        for (column = 0; column < MAP_COLUMNS; ++column)
            gPlayer1LevelMap[row][column] &= MAP_TRANSITION_CLEAR_MASK;
    gTransitionPathMinimumVisits = 25;
    for (pass = 0; pass < 2; ++pass) {
        for (row = 0; row < MAP_ROWS; ++row)
            for (column = 0; column < MAP_COLUMNS; ++column) {
                gTransitionPathVisitCount = 0;
                if (MapTransitionContainsMetatile(1, gPlayer1LevelMap[row][column])) {
                    if (MapTraceTransitionChain(row, column - 1, MAP_PATH_LEFT, MAP_PATH_LEFT)) {
                        gPlayer1LevelMap[row][column] |= 0x100;
                        return;
                    }
                    if (MapTraceTransitionChain(row, column + 1, MAP_PATH_RIGHT, MAP_PATH_RIGHT)) {
                        gPlayer1LevelMap[row][column] |= 0x200;
                        return;
                    }
                }
            }
        gTransitionPathMinimumVisits = 16;
    }
}
