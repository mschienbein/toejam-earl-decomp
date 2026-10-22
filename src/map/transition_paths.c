#include "elevator.h"
#include "map.h"
#include "assets_map.h"
#include "map_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"

/* ROM descriptors contain a count-prefixed byte list at offset two. */

int func_006b22(list_number, value)
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
int func_006b74(row, column, incoming, outgoing)
short row, column, incoming, outgoing;
{
    if ((gMapDataA[row][column] & 0x1f00) ||
        (ELEVATOR_POS_TABLE[0].x == column && ELEVATOR_POS_TABLE[0].y == row) ||
        (ELEVATOR_POS_TABLE[1].x == column && ELEVATOR_POS_TABLE[1].y == row) ||
        (ELEVATOR_POS_TABLE[2].x == column && ELEVATOR_POS_TABLE[2].y == row))
        return 0;
    ++ram_ff91e6;
    if (incoming == 8 && outgoing == 8) {
        if (func_006b22(3, gMapDataA[row][column]) &&
            func_006b74(row, column - 1, 8, 8)) {
            gMapDataA[row][column] |= 0x300;
            return 1;
        }
        if (func_006b22(4, gMapDataA[row][column]) &&
            func_006b74(row + 1, column, 4, 8)) {
            gMapDataA[row][column] |= 0x400;
            return 1;
        }
        if (func_006b22(2, gMapDataA[row][column]) &&
            ram_ff91e6 >= ram_ff91e8) {
            gMapDataA[row][column] |= 0x200;
            return 1;
        }
    } else if (incoming == 2 && outgoing == 2) {
        if (func_006b22(5, gMapDataA[row][column]) &&
            func_006b74(row, column + 1, 2, 2)) {
            gMapDataA[row][column] |= 0x500;
            return 1;
        }
        if (func_006b22(6, gMapDataA[row][column]) &&
            func_006b74(row + 1, column, 4, 2)) {
            gMapDataA[row][column] |= 0x600;
            return 1;
        }
        if (func_006b22(1, gMapDataA[row][column]) &&
            ram_ff91e6 >= ram_ff91e8) {
            gMapDataA[row][column] |= 0x100;
            return 1;
        }
    } else if (incoming == 4 && outgoing == 8) {
        if (func_006b22(7, gMapDataA[row][column]) &&
            func_006b74(row, column - 1, 8, 4)) {
            gMapDataA[row][column] |= 0x700;
            return 1;
        }
        if (func_006b22(8, gMapDataA[row][column]) &&
            func_006b74(row + 1, column, 4, 2)) {
            gMapDataA[row][column] |= 0x800;
            return 1;
        }
    } else if (incoming == 4 && outgoing == 2) {
        if (func_006b22(9, gMapDataA[row][column]) &&
            func_006b74(row, column + 1, 2, 4)) {
            gMapDataA[row][column] |= 0x900;
            return 1;
        }
        if (func_006b22(10, gMapDataA[row][column]) &&
            func_006b74(row + 1, column, 4, 8)) {
            gMapDataA[row][column] |= 0xa00;
            return 1;
        }
    } else if (incoming == 8 && outgoing == 4) {
        if (func_006b22(11, gMapDataA[row][column]) &&
            func_006b74(row, column - 1, 8, 8)) {
            gMapDataA[row][column] |= 0xb00;
            return 1;
        }
        if (func_006b22(12, gMapDataA[row][column]) &&
            func_006b74(row + 1, column, 4, 8)) {
            gMapDataA[row][column] |= 0xc00;
            return 1;
        }
        if (func_006b22(13, gMapDataA[row][column]) &&
            ram_ff91e6 >= ram_ff91e8) {
            gMapDataA[row][column] |= 0xd00;
            return 1;
        }
    } else if (incoming == 2 && outgoing == 4) {
        if (func_006b22(14, gMapDataA[row][column]) &&
            func_006b74(row, column + 1, 2, 2)) {
            gMapDataA[row][column] |= 0xe00;
            return 1;
        }
        if (func_006b22(15, gMapDataA[row][column]) &&
            func_006b74(row + 1, column, 4, 2)) {
            gMapDataA[row][column] |= 0xf00;
            return 1;
        }
        if (func_006b22(16, gMapDataA[row][column]) &&
            ram_ff91e6 >= ram_ff91e8) {
            gMapDataA[row][column] |= 0x1000;
            return 1;
        }
    }
    --ram_ff91e6;
    return 0;
}

/* Clear transition markings, then scan twice for a chain. The first scan
 * requires 25 visits; the second lowers the requirement to 16. */
void func_0071e8()
{
    short column, row, pass;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column)
            gMapDataA[row][column] &= 0xe0ff;
    ram_ff91e8 = 25;
    for (pass = 0; pass < 2; ++pass) {
        for (row = 0; row < 26; ++row)
            for (column = 0; column < 19; ++column) {
                ram_ff91e6 = 0;
                if (func_006b22(1, gMapDataA[row][column])) {
                    if (func_006b74(row, column - 1, 8, 8)) {
                        gMapDataA[row][column] |= 0x100;
                        return;
                    }
                    if (func_006b74(row, column + 1, 2, 2)) {
                        gMapDataA[row][column] |= 0x200;
                        return;
                    }
                }
            }
        ram_ff91e8 = 16;
    }
}
