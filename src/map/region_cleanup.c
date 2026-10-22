#include "elevator.h"
#include "map.h"
#include "assets_map.h"
#include "map_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "random.h"

/* Fill a rectangle. Replacing a different labelled 0x1000 cell replaces
 * every occurrence of that old label throughout the complete map. */
void MapFillRectangleAndMergeRegions(start_row, start_column, height, width, replacement)
short start_row, start_column, height, width, replacement;
{
    short map_row, map_column, tile, column, row;

    for (row = start_row; row < start_row + height; ++row)
        for (column = start_column; column < start_column + width; ++column) {
            tile = gMapDataA[row][column];
            if ((tile & 0x3000) == 0x1000 && tile != replacement) {
                for (map_row = 0; map_row < 26; ++map_row)
                    for (map_column = 0; map_column < 19; ++map_column)
                        if (gMapDataA[map_row][map_column] == tile)
                            gMapDataA[map_row][map_column] = replacement;
            } else
                gMapDataA[row][column] = replacement;
        }
}

/* Expand adjacent 0x1000 terrain into two-by-three rectangles, merging
 * differing region values through MapFillRectangleAndMergeRegions. Return whether any changed. */
short MapExpandRegionsBesidePaths(row, column)
short row, column;
{
    short changed = 0;
    short terrain;

    if ((gMapDataA[row][column] & 1) &&
        ((terrain = gMapDataA[row - 1][column]) & 0x3000) == 0x1000 &&
        (gMapDataA[row - 1][column - 1] != terrain ||
         gMapDataA[row - 1][column + 1] != terrain ||
         gMapDataA[row - 2][column - 1] != terrain ||
         gMapDataA[row - 2][column + 1] != terrain)) {
        MapFillRectangleAndMergeRegions(row - 2, column - 1, 2, 3, terrain);
        changed = 1;
    }
    if ((gMapDataA[row][column] & 4) &&
        ((terrain = gMapDataA[row + 1][column]) & 0x3000) == 0x1000 &&
        (gMapDataA[row + 1][column - 1] != terrain ||
         gMapDataA[row + 1][column + 1] != terrain ||
         gMapDataA[row + 2][column - 1] != terrain ||
         gMapDataA[row + 2][column + 1] != terrain)) {
        MapFillRectangleAndMergeRegions(row + 1, column - 1, 2, 3, terrain);
        changed = 1;
    }
    if ((gMapDataA[row][column] & 8) &&
        ((terrain = gMapDataA[row][column - 1]) & 0x3000) == 0x1000 &&
        (gMapDataA[row - 1][column - 1] != terrain ||
         gMapDataA[row + 1][column - 1] != terrain ||
         gMapDataA[row - 1][column - 2] != terrain ||
         gMapDataA[row + 1][column - 2] != terrain)) {
        MapFillRectangleAndMergeRegions(row - 1, column - 2, 3, 2, terrain);
        changed = 1;
    }
    if ((gMapDataA[row][column] & 2) &&
        ((terrain = gMapDataA[row][column + 1]) & 0x3000) == 0x1000 &&
        (gMapDataA[row - 1][column + 1] != terrain ||
         gMapDataA[row + 1][column + 1] != terrain ||
         gMapDataA[row - 1][column + 2] != terrain ||
         gMapDataA[row + 1][column + 2] != terrain)) {
        MapFillRectangleAndMergeRegions(row - 1, column + 1, 3, 2, terrain);
        changed = 1;
    }
    return changed;
}

/* Scan horizontal stretches of 0x1000 terrain along both vertical boundaries.
 * A counter reaching four extends two cells into the neighboring row; at five
 * the middle cell is cleared. A non-qualifying cell resets the counter to one
 * but keeps the remembered terrain label, so a label, a gap and three cells of
 * that label already reach the threshold ([INFERENCE] unintended; the code is
 * kept as the ROM has it). */
void MapWidenHorizontalRegionEdges()
{
    short run_length, row, column;
    unsigned short terrain, previous_terrain;

    for (row = 0; row < 25; row++) {
        run_length = 0;
        previous_terrain = 0xffff;
        for (column = 0; column < 19; column++) {
            terrain = gMapDataA[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (row == 0 || (gMapDataA[row - 1][column] & 0x3000) != 0x1000)) {
                if (terrain == previous_terrain)
                    run_length++;
                else {
                    previous_terrain = terrain;
                    run_length = 1;
                }
            } else
                run_length = 1;
            if (run_length >= 4) {
                if (row > 0) {
                    gMapDataA[row - 1][column - 2] = terrain;
                    gMapDataA[row - 1][column - 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gMapDataA[row][column - 2] = 0;
                    run_length = 2;
                }
            }
        }
    }
    for (row = 1; row < 26; row++) {
        run_length = 0;
        previous_terrain = 0xffff;
        for (column = 0; column < 19; column++) {
            terrain = gMapDataA[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (row == 25 || (gMapDataA[row + 1][column] & 0x3000) != 0x1000)) {
                if (terrain == previous_terrain)
                    run_length++;
                else {
                    previous_terrain = terrain;
                    run_length = 1;
                }
            } else
                run_length = 1;
            if (run_length >= 4) {
                if (row < 25) {
                    gMapDataA[row + 1][column - 2] = terrain;
                    gMapDataA[row + 1][column - 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gMapDataA[row][column - 2] = 0;
                    run_length = 2;
                }
            }
        }
    }
}

/* Scan vertical stretches along both horizontal boundaries, with the same
 * counter thresholds and reset-to-one quirk as MapWidenHorizontalRegionEdges. */
void MapWidenVerticalRegionEdges()
{
    short run_length, column, row;
    unsigned short terrain, previous_terrain;

    for (column = 0; column < 18; column++) {
        run_length = 0;
        previous_terrain = 0xffff;
        for (row = 0; row < 26; row++) {
            terrain = gMapDataA[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (column == 0 || (gMapDataA[row][column - 1] & 0x3000) != 0x1000)) {
                if (terrain == previous_terrain)
                    run_length++;
                else {
                    previous_terrain = terrain;
                    run_length = 1;
                }
            } else
                run_length = 1;
            if (run_length >= 4) {
                if (column > 0) {
                    gMapDataA[row - 2][column - 1] = terrain;
                    gMapDataA[row - 1][column - 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gMapDataA[row - 2][column] = 0;
                    run_length = 2;
                }
            }
        }
    }
    for (column = 1; column < 19; column++) {
        run_length = 0;
        previous_terrain = 0xffff;
        for (row = 0; row < 26; row++) {
            terrain = gMapDataA[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (column == 18 || (gMapDataA[row][column + 1] & 0x3000) != 0x1000)) {
                if (terrain == previous_terrain)
                    run_length++;
                else {
                    previous_terrain = terrain;
                    run_length = 1;
                }
            } else
                run_length = 1;
            if (run_length >= 4) {
                if (column < 18) {
                    gMapDataA[row - 2][column + 1] = terrain;
                    gMapDataA[row - 1][column + 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gMapDataA[row - 2][column] = 0;
                    run_length = 2;
                }
            }
        }
    }
}

/* Run the prepasses, then visit 0x2000 cells until no helper reports a change. */
void MapStabilizeRegionConnections()
{
    short row, column, changed = 1;

    MapWidenHorizontalRegionEdges();
    MapWidenVerticalRegionEdges();
    while (changed) {
        changed = 0;
        for (row = 0; row < 26; ++row) {
            for (column = 0; column < 19; ++column) {
                if ((gMapDataA[row][column] & 0x3000) == 0x2000) {
                    if (MapExpandRegionsBesidePaths(row, column))
                        changed = 1;
                }
            }
        }
    }
}

/* Clear temporary 0x0800 marks, then check uniform 4-by-4 regions without
 * elevator cells before requesting 0x3000 placement at the inner offset. */
void MapSeedFilledTerrainRegions()
{
    short row, column, scan_row, scan_column;
    short eligible, cell, replacement;
    unsigned short probability;

    for (row = 0; row < 26; ++row) {
        for (column = 0; column < 19; ++column)
            gMapDataA[row][column] &= 0xf7ff;
    }
    probability = MapGetRandomGenerationParameter(4);
    for (row = 0; row < 23; ++row) {
        for (column = 0; column < 16; ++column) {
            cell = gMapDataA[row][column];
            if ((cell & 0x3000) == 0x1000) {
                eligible = 1;
                replacement = (cell & 0x700) | 0x3000;
                for (scan_row = row; scan_row < row + 4; ++scan_row) {
                    for (scan_column = column; scan_column < column + 4;
                         ++scan_column) {
                        if ((gMapDataA[scan_row][scan_column] != cell &&
                             gMapDataA[scan_row][scan_column] != replacement) ||
                            (ELEVATOR_POS_TABLE[0].y == scan_row &&
                             ELEVATOR_POS_TABLE[0].x == scan_column) ||
                            (ELEVATOR_POS_TABLE[1].y == scan_row &&
                             ELEVATOR_POS_TABLE[1].x == scan_column) ||
                            (ELEVATOR_POS_TABLE[2].y == scan_row &&
                             ELEVATOR_POS_TABLE[2].x == scan_column)) {
                            eligible = 0;
                            scan_row = row + 4;
                            scan_column = column + 4;
                        }
                    }
                }
                if (eligible && probability > GetRandomNumber() % 100)
                    MapGrowTerrainRegion(0x3000, (cell >> 8) & 7, 0,
                                row + 1, column + 1);
            }
        }
    }
}

/* Mark one region at a time, then OR in a lookup byte indexed by the
 * four complete 2-by-2 neighborhoods surrounding each 0x1000 cell. */
void MapBuildRegionEdgeShapes(region_count)
short region_count;
{
    unsigned short cell;
    short upper_table, region, row, column, corner_mask;

    for (region = 0; region < region_count; ++region) {
        upper_table = MapGetRandomGenerationParameter(5);
        upper_table = upper_table > GetRandomNumber() % 100;
        for (row = 0; row < 26; ++row) {
            for (column = 0; column < 19; ++column) {
                cell = gMapDataA[row][column];
                if (((cell & 0x3000) == 0x1000 ||
                     (cell & 0x3000) == 0x3000) &&
                    region == ((cell >> 8) & 7)) {
                    gMapDataA[row][column] |= 0x800;
                    if (upper_table)
                        gMapDataA[row][column] |= 0x8000;
                } else
                    gMapDataA[row][column] &= 0xf7ff;
            }
        }
        for (row = 0; row < 26; ++row) {
            for (column = 0; column < 19; ++column) {
                cell = gMapDataA[row][column];
                if (cell & 0x800) {
                    corner_mask = 0;
                    if (row > 0 && column > 0 &&
                        (gMapDataA[row][column - 1] & 0x800) &&
                        (gMapDataA[row - 1][column - 1] & 0x800) &&
                        (gMapDataA[row - 1][column] & 0x800))
                        corner_mask |= 1;
                    if (row > 0 && column < 18 &&
                        (gMapDataA[row - 1][column] & 0x800) &&
                        (gMapDataA[row - 1][column + 1] & 0x800) &&
                        (gMapDataA[row][column + 1] & 0x800))
                        corner_mask |= 2;
                    if (row < 25 && column < 18 &&
                        (gMapDataA[row][column + 1] & 0x800) &&
                        (gMapDataA[row + 1][column + 1] & 0x800) &&
                        (gMapDataA[row + 1][column] & 0x800))
                        corner_mask |= 4;
                    if (row < 25 && column > 0 &&
                        (gMapDataA[row + 1][column] & 0x800) &&
                        (gMapDataA[row + 1][column - 1] & 0x800) &&
                        (gMapDataA[row][column - 1] & 0x800))
                        corner_mask |= 8;
                    if ((cell & 0x3000) == 0x1000) {
                        if (upper_table)
                            gMapDataA[row][column] |=
                                MapGenerationLookupTables.edge_alternate[corner_mask];
                        else
                            gMapDataA[row][column] |=
                                MapGenerationLookupTables.edge_plain[corner_mask];
                    }
                }
            }
        }
    }
}
