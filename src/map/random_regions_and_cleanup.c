#include "elevator.h"
#include "assets_map.h"
#include "map_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_system.h"
#include "random.h"

/* Park-Miller generator, using Schrage's split to avoid overflowing 31 bits.
 * The original public result is the low unsigned 16 bits of the new seed. */
unsigned short GetRandomNumber()
{
    long high, low, next;

    high = gRNGState / 127773L;
    low = gRNGState % 127773L;
    next = 16807L * low - 2836L * high;
    if (next > 0)
        gRNGState = next;
    else
        gRNGState = next + 2147483647L;
    return gRNGState;
}

/* Normalize the seed and return the preceding generator state.
 * ORIGINAL BUG: negating the minimum 32-bit seed leaves it negative on
 * the 68000, so that input is not normalized into the positive range. */
long StackBasedRandom(seed)
long seed;
{
    long previous;

    previous = gRNGState;
    if (seed < 0)
        seed = -seed;
    if (seed >= 2147483647L)
        seed -= 2147483647L;
    if (!seed)
        seed = 1;
    gRNGState = seed;
    return previous;
}

/* Cache 59 draws without advancing the externally visible generator state. */
void PreGenerateRandoms()
{
    long saved_state;
    short random_index;

    saved_state = gRNGState;
    for (random_index = 0; random_index < 59; ++random_index)
        gCachedRandomValues[random_index] = GetRandomNumber();
    gRandomCacheIndex = 0;
    gRNGState = saved_state;
}

/* Draw uniformly from the observed inclusive per-level min/max pair.
 * Keep the full-width selected value until the unsigned-word result, then
 * preserve the existing wide public return interface. */
unsigned long MapGetRandomGenerationParameter(range_index)
short range_index;
{
    char range_row;
    unsigned short minimum;
    unsigned short maximum;
    unsigned short random;

    range_row = MapGenerationRangeRow[gRequestedMapLevel - 2];
    minimum = MapGenerationRangePairs[range_row][range_index][0];
    maximum = MapGenerationRangePairs[range_row][range_index][1];
    random = GetRandomNumber();
    {
        int span;
        unsigned long selected;
        unsigned short result;
        span = maximum - minimum + 1;
        selected = random % span + minimum;
        result = selected;
        return result;
    }
}

/* Walk between two map cells, adding reciprocal direction bits and 0x2000
 * to the word map. A level-dependent draw also copies direction bits to the
 * byte map; turns are selected from the observed lookup table. */
void MapCarveDirectionalPath(start_row, start_column, target_row, target_column,
                 random_steps, turn_chance)
short start_row, start_column, target_row, target_column;
short random_steps, turn_chance;
{
    short direction_steps = 0;
    /* The absent-direction sentinels are signed words. */
    short opposite_direction = -1;
    short row, column;
    short previous_direction = -1;
    short repeated_steps = 0;
    short steps = 0;
    short direction, next_direction;
    char copy_directions;

    row = start_row;
    column = start_column;
    copy_directions = GetRandomNumber() % 100 < 10 + 3 * gRequestedMapLevel;
    direction = 1 << (GetRandomNumber() & 3);
    while (row != target_row || column != target_column) {
        if (direction_steps <= 0 || !turn_chance) {
            direction_steps = GetRandomNumber() % 3 + 2;
            if (turn_chance > GetRandomNumber() % 100)
                next_direction = MapPathTurnDirections[GetRandomNumber() % 3 + 1 + direction];
            else if (GetRandomNumber() % 100 < 50) {
                if (target_row - row > 0)
                    next_direction = 4;
                else if (target_row != row)
                    next_direction = 1;
                else if (GetRandomNumber() % 100 < 50)
                    next_direction = 1;
                else
                    next_direction = 4;
            } else {
                if (target_column - column > 0)
                    next_direction = 2;
                else if (column != target_column)
                    next_direction = 8;
                else if (GetRandomNumber() % 100 < 50)
                    next_direction = 8;
                else
                    next_direction = 2;
            }
        }
        if (!turn_chance || GetRandomNumber() % 100 < 80)
            direction = next_direction;
        else
            direction = MapPathTurnDirections[GetRandomNumber() % 3 + 1 + next_direction];
        if (previous_direction == direction) {
            if (repeated_steps >= direction_steps)
                direction = MapPathTurnDirections[GetRandomNumber() % 3 + 1 + direction];
            else
                ++repeated_steps;
        } else
            repeated_steps = 0;
        if (direction != opposite_direction &&
            ((direction == 1 && row > 2) ||
             (direction == 4 && row < 24) ||
             (direction == 8 && column > 2) ||
             (direction == 2 && column < 17))) {
            gPlayer1LevelMap[row][column] |= direction | 0x2000;
            if (copy_directions)
                gMapPathDirectionBanks[row][column] |= direction;
            if (direction == 1) {
                --row;
                opposite_direction = 4;
            } else if (direction == 4) {
                ++row;
                opposite_direction = 1;
            } else if (direction == 8) {
                --column;
                opposite_direction = 2;
            } else if (direction == 2) {
                ++column;
                opposite_direction = 8;
            }
            gPlayer1LevelMap[row][column] |= opposite_direction | 0x2000;
            if (copy_directions)
                gMapPathDirectionBanks[row][column] |= opposite_direction;
            previous_direction = direction;
            --direction_steps;
            ++steps;
            if (gRequestedMapLevel > GetRandomNumber() % 100)
                copy_directions = 1;
        } else
            direction_steps = 0;
        if (steps >= random_steps)
            turn_chance = 0;
    }
}

/* Advance a 2x2 region origin. Different 0x1000 regions may stop the scan
 * after two overlapping cells, according to the supplied percentage. */
short MapFindRegionGrowthOrigin(row_position, column_position, direction, region, join_chance)
short *row_position, *column_position;
short direction, region, join_chance;
{
    short row, column, overlaps, row_offset, column_offset, tile;

    row = *row_position;
    column = *column_position;
    while (1) {
        if (direction == 1)
            --row;
        if (direction == 4)
            ++row;
        if (direction == 8)
            --column;
        if (direction == 2)
            ++column;
        if (row < 0 || column < 0 || row >= 25 || column >= 18)
            break;
        if ((row == 0 || row == 24) && (direction & 10))
            return 0;
        if ((column == 0 || column == 17) && (direction & 5))
            return 0;
        overlaps = 0;
        for (row_offset = 0; row_offset < 2; ++row_offset)
            for (column_offset = 0; column_offset < 2; ++column_offset) {
                tile = gPlayer1LevelMap[row + row_offset][column + column_offset];
                if ((tile & 0x3000) == 0x1000 && tile != region) {
                    ++overlaps;
                    if (overlaps >= 2 && join_chance > GetRandomNumber() % 100) {
                        *row_position = row;
                        *column_position = column;
                        return 1;
                    }
                }
            }
        if (overlaps)
            return 0;
        for (row_offset = 0; row_offset < 2; ++row_offset)
            for (column_offset = 0; column_offset < 2; ++column_offset)
                if (gPlayer1LevelMap[row + row_offset][column + column_offset] != region) {
                    *row_position = row;
                    *column_position = column;
                    return 1;
                }
    }
    return 0;
}

/* Advance a 2x2 region while its surrounding 4x4 neighbourhood contains
 * only this region or the corresponding 0x1000 base. Avoid all three
 * coordinate pairs in the elevator-position table. */
short MapFindInteriorGrowthOrigin(row_position, column_position, direction, region)
short *row_position, *column_position;
short direction, region;
{
    short row, column, row_offset, column_offset, tile;

    row = *row_position;
    column = *column_position;
    while (1) {
        if (direction == 1)
            --row;
        if (direction == 4)
            ++row;
        if (direction == 8)
            --column;
        if (direction == 2)
            ++column;
        if (row < 1 || column < 1 || row >= 24 || column >= 17)
            break;
        if (gLevelAnchorPositions[0].x <= column + 1 && gLevelAnchorPositions[0].x >= column &&
            gLevelAnchorPositions[0].y <= row + 1 && gLevelAnchorPositions[0].y >= row)
            break;
        if (gLevelAnchorPositions[1].x <= column + 1 && gLevelAnchorPositions[1].x >= column &&
            gLevelAnchorPositions[1].y <= row + 1 && gLevelAnchorPositions[1].y >= row)
            break;
        if (gLevelAnchorPositions[2].x <= column + 1 && gLevelAnchorPositions[2].x >= column &&
            gLevelAnchorPositions[2].y <= row + 1 && gLevelAnchorPositions[2].y >= row)
            break;
        for (row_offset = -1; row_offset < 3; ++row_offset)
            for (column_offset = -1; column_offset < 3; ++column_offset) {
                tile = gPlayer1LevelMap[row + row_offset][column + column_offset];
                if (tile != region && tile != ((region & 0x700) | 0x1000))
                    return 0;
            }
        for (row_offset = 0; row_offset < 2; ++row_offset)
            for (column_offset = 0; column_offset < 2; ++column_offset)
                if (gPlayer1LevelMap[row + row_offset][column + column_offset] != region) {
                    *row_position = row;
                    *column_position = column;
                    return 1;
                }
    }
    return 0;
}


#include "ram_map.h"
#include "assets_map.h"
#include "random.h"
#include "map_calls.h"

/* Grow a labelled 0x1000 or 0x3000 region in 2x2 blocks, joining prior
 * 0x1000 regions by replacing every occurrence of their old label. */
/* MATCHING QUALIFICATION: the four discarded sizeof-address observations
 * allocate direction/preferred_direction/join_chance/stop_chance after the
 * three ordinary post-register spills. Their reverse declaration order gives
 * the original frame slots without an extra copied direction or padding.
 * Each observation is necessary in the measured source. These are compiler-
 * coaxing spellings, not claimed historical source. */
void MapGrowTerrainRegion(terrain_kind, region_number, minimum_cells, row, column)
short terrain_kind;
short region_number;
short minimum_cells;
short row;
short column;
{
  short row_offset;
  short column_offset;
  unsigned short region;
  short remaining_steps;
  unsigned short stop_chance;
  short next_direction;
  unsigned short join_chance;
  short preferred_direction;
  short direction = 1 << (GetRandomNumber() & 3);
  short tried_directions;
  short cell_count;
  unsigned short tile;
  sizeof(&direction);
  sizeof(&preferred_direction);
  sizeof(&join_chance);
  sizeof(&stop_chance);
  region = region_number << 8;
  region |= terrain_kind;
  if (terrain_kind == 0x1000)
  {
    stop_chance = MapGetRandomGenerationParameter(0);
    join_chance = MapGetRandomGenerationParameter(2);
  }
  else
    if (terrain_kind == 0x3000)
    stop_chance = MapGetRandomGenerationParameter(1);
  remaining_steps = 0;
  while (1)
  {
    if (terrain_kind == 0x1000)
    {
      for (row_offset = 0; row_offset < 2; ++row_offset)
        for (column_offset = 0; column_offset < 2; ++column_offset)
      {
        tile = gPlayer1LevelMap[row + row_offset][column + column_offset];
        if (((tile & 0x3000) == 0x1000) && (tile != region))
        {
          for (row_offset = 0; row_offset < 26; ++row_offset)
            for (column_offset = 0; column_offset < 19; ++column_offset)
            if (tile == gPlayer1LevelMap[row_offset][column_offset])
            gPlayer1LevelMap[row_offset][column_offset] = region;


          row_offset = (column_offset = 2);
        }
      }


    }
    gPlayer1LevelMap[row][column] = region;
    gPlayer1LevelMap[row][column + 1] = region;
    gPlayer1LevelMap[row + 1][column] = region;
    gPlayer1LevelMap[row + 1][column + 1] = region;
    if (remaining_steps <= 0)
    {
      remaining_steps = GetRandomNumber() & 1;
      if (terrain_kind == 0x3000)
        ++remaining_steps;
      preferred_direction = MapPathTurnDirections[((GetRandomNumber() % 3) + 1) + direction];
    }
    if ((GetRandomNumber() % 100) < 80)
      direction = preferred_direction;
    tried_directions = 0;
    while (1)
    {
      if ((terrain_kind == 0x1000) && MapFindRegionGrowthOrigin(&row, &column, direction, region, join_chance))
        break;
      if ((terrain_kind != 0x1000) && MapFindInteriorGrowthOrigin(&row, &column, direction, region))
        break;
      if ((tried_directions |= direction) != 15)
      {
        do
        {
          next_direction = MapPathTurnDirections[((GetRandomNumber() % 3) + 1) + direction];
        }
        while (tried_directions & next_direction);
      }
      else
        return;
      direction = next_direction;
    }

    --remaining_steps;
    if (stop_chance > (GetRandomNumber() % 100))
    {
      cell_count = 0;
      if (minimum_cells > 0)
        for (row_offset = 0; row_offset < 26; ++row_offset)
        for (column_offset = 0; column_offset < 19; ++column_offset)
        if (terrain_kind == (gPlayer1LevelMap[row_offset][column_offset] & 0x3000))
        ++cell_count;


      if (cell_count >= minimum_cells)
        break;
    }
  }

}




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
            tile = gPlayer1LevelMap[row][column];
            if ((tile & 0x3000) == 0x1000 && tile != replacement) {
                for (map_row = 0; map_row < MAP_ROWS; ++map_row)
                    for (map_column = 0; map_column < MAP_COLUMNS; ++map_column)
                        if (gPlayer1LevelMap[map_row][map_column] == tile)
                            gPlayer1LevelMap[map_row][map_column] = replacement;
            } else
                gPlayer1LevelMap[row][column] = replacement;
        }
}

/* Expand adjacent 0x1000 terrain into two-by-three rectangles, merging
 * differing region values through MapFillRectangleAndMergeRegions. Return whether any changed. */
short MapExpandRegionsBesidePaths(row, column)
short row, column;
{
    short changed = 0;
    short terrain;

    if ((gPlayer1LevelMap[row][column] & 1) &&
        ((terrain = gPlayer1LevelMap[row - 1][column]) & 0x3000) == 0x1000 &&
        (gPlayer1LevelMap[row - 1][column - 1] != terrain ||
         gPlayer1LevelMap[row - 1][column + 1] != terrain ||
         gPlayer1LevelMap[row - 2][column - 1] != terrain ||
         gPlayer1LevelMap[row - 2][column + 1] != terrain)) {
        MapFillRectangleAndMergeRegions(row - 2, column - 1, 2, 3, terrain);
        changed = 1;
    }
    if ((gPlayer1LevelMap[row][column] & 4) &&
        ((terrain = gPlayer1LevelMap[row + 1][column]) & 0x3000) == 0x1000 &&
        (gPlayer1LevelMap[row + 1][column - 1] != terrain ||
         gPlayer1LevelMap[row + 1][column + 1] != terrain ||
         gPlayer1LevelMap[row + 2][column - 1] != terrain ||
         gPlayer1LevelMap[row + 2][column + 1] != terrain)) {
        MapFillRectangleAndMergeRegions(row + 1, column - 1, 2, 3, terrain);
        changed = 1;
    }
    if ((gPlayer1LevelMap[row][column] & 8) &&
        ((terrain = gPlayer1LevelMap[row][column - 1]) & 0x3000) == 0x1000 &&
        (gPlayer1LevelMap[row - 1][column - 1] != terrain ||
         gPlayer1LevelMap[row + 1][column - 1] != terrain ||
         gPlayer1LevelMap[row - 1][column - 2] != terrain ||
         gPlayer1LevelMap[row + 1][column - 2] != terrain)) {
        MapFillRectangleAndMergeRegions(row - 1, column - 2, 3, 2, terrain);
        changed = 1;
    }
    if ((gPlayer1LevelMap[row][column] & 2) &&
        ((terrain = gPlayer1LevelMap[row][column + 1]) & 0x3000) == 0x1000 &&
        (gPlayer1LevelMap[row - 1][column + 1] != terrain ||
         gPlayer1LevelMap[row + 1][column + 1] != terrain ||
         gPlayer1LevelMap[row - 1][column + 2] != terrain ||
         gPlayer1LevelMap[row + 1][column + 2] != terrain)) {
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
        for (column = 0; column < MAP_COLUMNS; column++) {
            terrain = gPlayer1LevelMap[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (row == 0 || (gPlayer1LevelMap[row - 1][column] & 0x3000) != 0x1000)) {
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
                    gPlayer1LevelMap[row - 1][column - 2] = terrain;
                    gPlayer1LevelMap[row - 1][column - 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gPlayer1LevelMap[row][column - 2] = 0;
                    run_length = 2;
                }
            }
        }
    }
    for (row = 1; row < MAP_ROWS; row++) {
        run_length = 0;
        previous_terrain = 0xffff;
        for (column = 0; column < MAP_COLUMNS; column++) {
            terrain = gPlayer1LevelMap[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (row == 25 || (gPlayer1LevelMap[row + 1][column] & 0x3000) != 0x1000)) {
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
                    gPlayer1LevelMap[row + 1][column - 2] = terrain;
                    gPlayer1LevelMap[row + 1][column - 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gPlayer1LevelMap[row][column - 2] = 0;
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
        for (row = 0; row < MAP_ROWS; row++) {
            terrain = gPlayer1LevelMap[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (column == 0 || (gPlayer1LevelMap[row][column - 1] & 0x3000) != 0x1000)) {
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
                    gPlayer1LevelMap[row - 2][column - 1] = terrain;
                    gPlayer1LevelMap[row - 1][column - 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gPlayer1LevelMap[row - 2][column] = 0;
                    run_length = 2;
                }
            }
        }
    }
    for (column = 1; column < MAP_COLUMNS; column++) {
        run_length = 0;
        previous_terrain = 0xffff;
        for (row = 0; row < MAP_ROWS; row++) {
            terrain = gPlayer1LevelMap[row][column];
            if ((terrain & 0x3000) == 0x1000 &&
                (column == 18 || (gPlayer1LevelMap[row][column + 1] & 0x3000) != 0x1000)) {
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
                    gPlayer1LevelMap[row - 2][column + 1] = terrain;
                    gPlayer1LevelMap[row - 1][column + 1] = terrain;
                    run_length = 1;
                } else if (run_length >= 5) {
                    gPlayer1LevelMap[row - 2][column] = 0;
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
        for (row = 0; row < MAP_ROWS; ++row) {
            for (column = 0; column < MAP_COLUMNS; ++column) {
                if ((gPlayer1LevelMap[row][column] & 0x3000) == 0x2000) {
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

    for (row = 0; row < MAP_ROWS; ++row) {
        for (column = 0; column < MAP_COLUMNS; ++column)
            gPlayer1LevelMap[row][column] &= 0xf7ff;
    }
    probability = MapGetRandomGenerationParameter(4);
    for (row = 0; row < 23; ++row) {
        for (column = 0; column < 16; ++column) {
            cell = gPlayer1LevelMap[row][column];
            if ((cell & 0x3000) == 0x1000) {
                eligible = 1;
                replacement = (cell & 0x700) | 0x3000;
                for (scan_row = row; scan_row < row + 4; ++scan_row) {
                    for (scan_column = column; scan_column < column + 4;
                         ++scan_column) {
                        if ((gPlayer1LevelMap[scan_row][scan_column] != cell &&
                             gPlayer1LevelMap[scan_row][scan_column] != replacement) ||
                            (gLevelAnchorPositions[0].y == scan_row &&
                             gLevelAnchorPositions[0].x == scan_column) ||
                            (gLevelAnchorPositions[1].y == scan_row &&
                             gLevelAnchorPositions[1].x == scan_column) ||
                            (gLevelAnchorPositions[2].y == scan_row &&
                             gLevelAnchorPositions[2].x == scan_column)) {
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
        for (row = 0; row < MAP_ROWS; ++row) {
            for (column = 0; column < MAP_COLUMNS; ++column) {
                cell = gPlayer1LevelMap[row][column];
                if (((cell & 0x3000) == 0x1000 ||
                     (cell & 0x3000) == 0x3000) &&
                    region == ((cell >> 8) & 7)) {
                    gPlayer1LevelMap[row][column] |= 0x800;
                    if (upper_table)
                        gPlayer1LevelMap[row][column] |= 0x8000;
                } else
                    gPlayer1LevelMap[row][column] &= 0xf7ff;
            }
        }
        for (row = 0; row < MAP_ROWS; ++row) {
            for (column = 0; column < MAP_COLUMNS; ++column) {
                cell = gPlayer1LevelMap[row][column];
                if (cell & 0x800) {
                    corner_mask = 0;
                    if (row > 0 && column > 0 &&
                        (gPlayer1LevelMap[row][column - 1] & 0x800) &&
                        (gPlayer1LevelMap[row - 1][column - 1] & 0x800) &&
                        (gPlayer1LevelMap[row - 1][column] & 0x800))
                        corner_mask |= 1;
                    if (row > 0 && column < 18 &&
                        (gPlayer1LevelMap[row - 1][column] & 0x800) &&
                        (gPlayer1LevelMap[row - 1][column + 1] & 0x800) &&
                        (gPlayer1LevelMap[row][column + 1] & 0x800))
                        corner_mask |= 2;
                    if (row < 25 && column < 18 &&
                        (gPlayer1LevelMap[row][column + 1] & 0x800) &&
                        (gPlayer1LevelMap[row + 1][column + 1] & 0x800) &&
                        (gPlayer1LevelMap[row + 1][column] & 0x800))
                        corner_mask |= 4;
                    if (row < 25 && column > 0 &&
                        (gPlayer1LevelMap[row + 1][column] & 0x800) &&
                        (gPlayer1LevelMap[row + 1][column - 1] & 0x800) &&
                        (gPlayer1LevelMap[row][column - 1] & 0x800))
                        corner_mask |= 8;
                    if ((cell & 0x3000) == 0x1000) {
                        if (upper_table)
                            gPlayer1LevelMap[row][column] |=
                                MapGenerationLookupTables.edge_alternate[corner_mask];
                        else
                            gPlayer1LevelMap[row][column] |=
                                MapGenerationLookupTables.edge_plain[corner_mask];
                    }
                }
            }
        }
    }
}

