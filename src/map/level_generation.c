#define TJE_LOCAL_TILEALTERNATIVEPOINTERS
#include "map.h"
#include "ram.h"
#include "assets.h"
#include "map_calls.h"
#include "random.h"
#include "romcalls.h"

/* Compiler bound qualification: the compatible incomplete-array view
 * preserves the observed long index arithmetic. The shared complete bound
 * changes index width. Historical declaration spelling is unknown [INFERENCE]. */
extern unsigned char *TileAlternativePointers[];

/* Refine marked map cells using the low-byte shapes of adjacent cells. */
void func_00334a()
{
    short row, column;
    unsigned short shape;
    char upper_right, upper_left, lower_right, lower_left;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gMapDataA[row][column];
            if (shape & 0x800) {
                shape &= 0xff;
                if (shape == 3) {
                    shape = gMapDataA[row][column - 1] & 0xff;
                    if ((gMapDataA[row][column + 1] & 0xff) != 7 &&
                        (shape == 11 || shape == 19))
                        gMapDataA[row][column] |= 0x10;
                }
            }
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gMapDataA[row][column];
            if (shape & 0x800) {
                shape &= 0xff;
                if (shape == 12) {
                    shape = gMapDataA[row][column - 1] & 0xff;
                    if (shape == 4 || shape == 28)
                        gMapDataA[row][column] |= 0x10;
                }
            }
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gMapDataA[row][column];
            if ((shape & 0x800) && (shape & 15) == 15) {
                upper_right = upper_left = 0;
                lower_right = lower_left = 0;
                shape = gMapDataA[row - 1][column] & 31;
                if (shape == 21 || shape == 13 || shape == 12)
                    ++upper_left;
                if (shape == 14 || shape == 28 || shape == 26)
                    ++upper_right;
                shape = gMapDataA[row + 1][column] & 31;
                if (shape == 7 || shape == 3)
                    ++lower_left;
                if (shape == 19 || shape == 11)
                    ++lower_right;
                if (upper_right) {
                    if (lower_left)
                        shape = 23;
                    else if (lower_right)
                        shape = 26;
                    else
                        shape = 20;
                } else if (upper_left) {
                    if (lower_right)
                        shape = 24;
                    else if (lower_left)
                        shape = 21;
                    else
                        shape = 20;
                } else
                    shape = 20;
                gMapDataA[row][column] &= 0xff00;
                gMapDataA[row][column] |= shape;
            }
        }
}

/* Mark the cells with class 3, compute their filled corner bits, and select
 * a low-byte shape from the two lookup banks. */
void func_003604()
{
    short row, column;
    unsigned short shape;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gMapDataA[row][column];
            if ((shape & 0x3000) == 0x3000)
                gMapDataA[row][column] |= 0x800;
            else
                gMapDataA[row][column] &= 0xf7ff;
            if ((shape & 0x3000) != 0x2000)
                gMapDataD[row][column] &= 0xf0;
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gMapDataA[row][column];
            if (shape & 0x800) {
                shape = 0;
                if (row > 0 && column > 0 &&
                    (gMapDataA[row][column - 1] & 0x800) &&
                    (gMapDataA[row - 1][column - 1] & 0x800) &&
                    (gMapDataA[row - 1][column] & 0x800))
                    shape |= 1;
                if (row > 0 && column < 18 &&
                    (gMapDataA[row - 1][column] & 0x800) &&
                    (gMapDataA[row - 1][column + 1] & 0x800) &&
                    (gMapDataA[row][column + 1] & 0x800))
                    shape |= 2;
                if (row < 25 && column < 18 &&
                    (gMapDataA[row][column + 1] & 0x800) &&
                    (gMapDataA[row + 1][column + 1] & 0x800) &&
                    (gMapDataA[row + 1][column] & 0x800))
                    shape |= 4;
                if (row < 25 && column > 0 &&
                    (gMapDataA[row + 1][column] & 0x800) &&
                    (gMapDataA[row + 1][column - 1] & 0x800) &&
                    (gMapDataA[row][column - 1] & 0x800))
                    shape |= 8;
                gMapDataA[row][column] |= shape;
            }
        }
    func_00334a();
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gMapDataA[row][column];
            if (shape & 0x800) {
                shape &= 31;
                gMapDataA[row][column] &= 0xff00;
                if (gMapDataA[row][column] & 0x8000)
                    gMapDataA[row][column] |=
                        (unsigned short)MapGenerationLookupTables.filled_alternate[shape];
                else
                    gMapDataA[row][column] |=
                        (unsigned short)MapGenerationLookupTables.filled_plain[shape];
            }
        }
}

/* Convert class-1 boundary cells next to class-2 cells, then fold the
 * per-cell corner mask into the class-2 shape lookup. */
void func_0039e8()
{
    short row, column, shape;
    short replacement;
    unsigned char corners;

    for (row = 1; row < 25; ++row)
        for (column = 1; column < 18; ++column) {
            shape = gMapDataA[row][column];
            if ((shape & 0x3000) == 0x1000) {
                replacement = 0;
                if ((gMapDataA[row - 1][column] & 0x3004) == 0x2004) {
                    if (gMapDataD[row - 1][column] & 4)
                        gMapDataA[row][column] |= 0x4000;
                    else
                        replacement = 16;
                } else if ((gMapDataA[row][column + 1] & 0x3008) == 0x2008) {
                    if (gMapDataD[row][column + 1] & 8)
                        gMapDataA[row][column] |= 0x4000;
                    else
                        replacement = 17;
                } else if ((gMapDataA[row + 1][column] & 0x3001) == 0x2001) {
                    if (gMapDataD[row + 1][column] & 1)
                        gMapDataA[row][column] |= 0x4000;
                    else
                        replacement = 18;
                } else if ((gMapDataA[row][column - 1] & 0x3002) == 0x2002) {
                    if (gMapDataD[row][column - 1] & 2)
                        gMapDataA[row][column] |= 0x4000;
                    else
                        replacement = 19;
                }
                if (replacement) {
                    if ((gMapDataA[row][column] &= 0xff00) & 0x8000)
                        gMapDataA[row][column] |=
                            (unsigned short)dat_08bf6e[replacement];
                    else
                        gMapDataA[row][column] |=
                            (unsigned short)dat_08bf5a[replacement];
                }
            }
        }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            shape = gMapDataA[row][column];
            corners = gMapDataD[row][column] & 15;
            if (ram_ff91ea)
                corners = 0;
            if ((shape & 0x3000) == 0x2000) {
                shape ^= (unsigned short)corners;
                gMapDataA[row][column] =
                    MapGenerationLookupTables.tiles[shape & 15] | 0x2000;
                if (corners)
                    gMapDataA[row][column] |= 0x4000;
            }
        }
}

/* Populate unmasked cells with individual shapes or a four-cell block.
 * The surrounding class-1 cells and the random thresholds select the bank. */
void func_003cb6()
{
    short row, column, scan_row, scan_column;
    short nearby_plain, nearby_alternate, nearby_feature, alternate_chance;
    unsigned short shape;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            if (!(gMapDataD[row][column] & 15)) {
                shape = gMapDataA[row][column] & 0xff;
                if (!shape) {
                    nearby_plain = 0;
                    nearby_alternate = 0;
                    nearby_feature = 0;
                    if (row > 0 &&
                        (shape = gMapDataA[row - 1][column]) != 27 &&
                        shape != 45 && column > 0 &&
                        (shape = gMapDataA[row][column - 1]) != 27 &&
                        shape != 45) {
                        for (scan_row = row - 3; scan_row < row + 3; ++scan_row)
                            for (scan_column = column - 2; scan_column < column + 2; ++scan_column)
                                if (scan_row >= 0 && scan_column >= 0 &&
                                    scan_row < 26 && scan_column < 19) {
                                    shape = gMapDataA[scan_row][scan_column];
                                    if ((shape & 0x3000) == 0x1000) {
                                        if (shape & 0x8000)
                                            nearby_alternate = 1;
                                        else
                                            nearby_plain = 1;
                                    }
                                    if (shape == 27 || shape == 45)
                                        nearby_feature = 1;
                                }
                        if ((nearby_alternate || nearby_plain) &&
                            ((nearby_feature && GetRandomNumber() % 100 < 18) ||
                             GetRandomNumber() % 100 < 7)) {
                            if (nearby_alternate)
                                alternate_chance = 70;
                            else
                                alternate_chance = 20;
                            if (row < 25 && column < 18 &&
                                GetRandomNumber() % 100 < 50 &&
                                !gMapDataA[row][column + 1] &&
                                !gMapDataA[row + 1][column] &&
                                !gMapDataA[row + 1][column + 1])
                                shape = 1;
                            else
                                shape = 0;
                            if (alternate_chance > GetRandomNumber() % 100) {
                                if (shape) {
                                    gMapDataA[row][column] = 0x801e;
                                    gMapDataA[row][column + 1] = 0x801f;
                                    gMapDataA[row + 1][column] = 0x801d;
                                    gMapDataA[row + 1][column + 1] = 0x8020;
                                } else
                                    gMapDataA[row][column] = 45;
                            } else {
                                if (shape) {
                                    gMapDataA[row][column] = 2;
                                    gMapDataA[row][column + 1] = 3;
                                    gMapDataA[row + 1][column] = 1;
                                    gMapDataA[row + 1][column + 1] = 4;
                                } else
                                    gMapDataA[row][column] = 27;
                            }
                        }
                    }
                } else if (shape == 46) {
                    if (GetRandomNumber() % 100 < 10)
                        gMapDataA[row][column] = 0x902f;
                } else if (GetRandomNumber() % 100 < func_004f50(7)) {
                    if (shape == 75)
                        gMapDataA[row][column] = 73;
                    else if (shape == 76)
                        gMapDataA[row][column] = 74;
                    else if (shape == 80)
                        gMapDataA[row][column] = 77;
                }
            }
        }
}

/* Choose a tile alternative, using the transition descriptor when the
 * map word carries a descriptor number in bits 8 through 12. */
void func_004174()
{
    short row, column, choice, shape_number, shape_count;
    unsigned char old_shape, transition, bank;
    unsigned short new_cell;
    unsigned char *shapes, *alternatives;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            old_shape = gMapDataA[row][column] & 0xff;
            new_cell = gMapDataA[row][column] & 0xc000;
            if (!(gMapDataA[row][column] & 0x1f00)) {
                alternatives = TileAlternativePointers[old_shape];
                choice = GetRandomNumber() % alternatives[0];
                ++choice;
            } else {
                transition = (gMapDataA[row][column] & 0x1f00) >> 8;
                --transition;
                bank = MapTransitionDescriptors[transition].bank;
                shapes = MapTransitionDescriptors[transition].shapes;
                alternatives = MapTransitionDescriptors[transition].alternatives;
                shape_count = *shapes++;
                for (shape_number = 0; shape_number < shape_count; ++shape_number) {
                    if (old_shape == *shapes) {
                        choice = GetRandomNumber();
                        choice &= 1;
                        if (shape_number >= shape_count / 2 && (bank & 15) != 15)
                            bank += 7;
                        new_cell |= (unsigned short)(bank << 8);
                        break;
                    }
                    ++shapes;
                    alternatives += 2;
                }
            }
            new_cell |= (unsigned short)alternatives[choice];
            if (new_cell & 0x4000)
                new_cell |= 0x2000;
            gMapDataA[row][column] = new_cell;
        }
}

/* Exchange the two map planes and exchange each visibility byte's nibbles. */
void func_00433e()
{
    unsigned short row, column, saved_cell;
    unsigned char saved_mask;

    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            saved_cell = gMapDataA[row][column];
            gMapDataA[row][column] = gMapDataB[row][column];
            gMapDataB[row][column] = saved_cell;
            saved_mask = gMapDataD[row][column];
            gMapDataD[row][column] <<= 4;
            gMapDataD[row][column] |= saved_mask >> 4;
        }
}
