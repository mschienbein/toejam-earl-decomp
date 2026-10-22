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
    if ((gMapDataA[row][column] & 0x1f00) ||
        (ELEVATOR_POS_TABLE[0].x == column && ELEVATOR_POS_TABLE[0].y == row) ||
        (ELEVATOR_POS_TABLE[1].x == column && ELEVATOR_POS_TABLE[1].y == row) ||
        (ELEVATOR_POS_TABLE[2].x == column && ELEVATOR_POS_TABLE[2].y == row))
        return 0;
    ++gTransitionPathVisitCount;
    if (incoming == 8 && outgoing == 8) {
        if (MapTransitionContainsMetatile(3, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row, column - 1, 8, 8)) {
            gMapDataA[row][column] |= 0x300;
            return 1;
        }
        if (MapTransitionContainsMetatile(4, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row + 1, column, 4, 8)) {
            gMapDataA[row][column] |= 0x400;
            return 1;
        }
        if (MapTransitionContainsMetatile(2, gMapDataA[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gMapDataA[row][column] |= 0x200;
            return 1;
        }
    } else if (incoming == 2 && outgoing == 2) {
        if (MapTransitionContainsMetatile(5, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row, column + 1, 2, 2)) {
            gMapDataA[row][column] |= 0x500;
            return 1;
        }
        if (MapTransitionContainsMetatile(6, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row + 1, column, 4, 2)) {
            gMapDataA[row][column] |= 0x600;
            return 1;
        }
        if (MapTransitionContainsMetatile(1, gMapDataA[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gMapDataA[row][column] |= 0x100;
            return 1;
        }
    } else if (incoming == 4 && outgoing == 8) {
        if (MapTransitionContainsMetatile(7, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row, column - 1, 8, 4)) {
            gMapDataA[row][column] |= 0x700;
            return 1;
        }
        if (MapTransitionContainsMetatile(8, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row + 1, column, 4, 2)) {
            gMapDataA[row][column] |= 0x800;
            return 1;
        }
    } else if (incoming == 4 && outgoing == 2) {
        if (MapTransitionContainsMetatile(9, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row, column + 1, 2, 4)) {
            gMapDataA[row][column] |= 0x900;
            return 1;
        }
        if (MapTransitionContainsMetatile(10, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row + 1, column, 4, 8)) {
            gMapDataA[row][column] |= 0xa00;
            return 1;
        }
    } else if (incoming == 8 && outgoing == 4) {
        if (MapTransitionContainsMetatile(11, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row, column - 1, 8, 8)) {
            gMapDataA[row][column] |= 0xb00;
            return 1;
        }
        if (MapTransitionContainsMetatile(12, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row + 1, column, 4, 8)) {
            gMapDataA[row][column] |= 0xc00;
            return 1;
        }
        if (MapTransitionContainsMetatile(13, gMapDataA[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gMapDataA[row][column] |= 0xd00;
            return 1;
        }
    } else if (incoming == 2 && outgoing == 4) {
        if (MapTransitionContainsMetatile(14, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row, column + 1, 2, 2)) {
            gMapDataA[row][column] |= 0xe00;
            return 1;
        }
        if (MapTransitionContainsMetatile(15, gMapDataA[row][column]) &&
            MapTraceTransitionChain(row + 1, column, 4, 2)) {
            gMapDataA[row][column] |= 0xf00;
            return 1;
        }
        if (MapTransitionContainsMetatile(16, gMapDataA[row][column]) &&
            gTransitionPathVisitCount >= gTransitionPathMinimumVisits) {
            gMapDataA[row][column] |= 0x1000;
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

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column)
            gMapDataA[row][column] &= 0xe0ff;
    gTransitionPathMinimumVisits = 25;
    for (pass = 0; pass < 2; ++pass) {
        for (row = 0; row < 26; ++row)
            for (column = 0; column < 19; ++column) {
                gTransitionPathVisitCount = 0;
                if (MapTransitionContainsMetatile(1, gMapDataA[row][column])) {
                    if (MapTraceTransitionChain(row, column - 1, 8, 8)) {
                        gMapDataA[row][column] |= 0x100;
                        return;
                    }
                    if (MapTraceTransitionChain(row, column + 1, 2, 2)) {
                        gMapDataA[row][column] |= 0x200;
                        return;
                    }
                }
            }
        gTransitionPathMinimumVisits = 16;
    }
}
