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
        PREGEN_RANDOMS[random_index] = GetRandomNumber();
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

    range_row = MapGenerationRangeRow[gLevel912A - 2];
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
    copy_directions = GetRandomNumber() % 100 < 10 + 3 * gLevel912A;
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
            gMapDataA[row][column] |= direction | 0x2000;
            if (copy_directions)
                gMapDataD[row][column] |= direction;
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
            gMapDataA[row][column] |= opposite_direction | 0x2000;
            if (copy_directions)
                gMapDataD[row][column] |= opposite_direction;
            previous_direction = direction;
            --direction_steps;
            ++steps;
            if (gLevel912A > GetRandomNumber() % 100)
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
                tile = gMapDataA[row + row_offset][column + column_offset];
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
                if (gMapDataA[row + row_offset][column + column_offset] != region) {
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
        if (ELEVATOR_POS_TABLE[0].x <= column + 1 && ELEVATOR_POS_TABLE[0].x >= column &&
            ELEVATOR_POS_TABLE[0].y <= row + 1 && ELEVATOR_POS_TABLE[0].y >= row)
            break;
        if (ELEVATOR_POS_TABLE[1].x <= column + 1 && ELEVATOR_POS_TABLE[1].x >= column &&
            ELEVATOR_POS_TABLE[1].y <= row + 1 && ELEVATOR_POS_TABLE[1].y >= row)
            break;
        if (ELEVATOR_POS_TABLE[2].x <= column + 1 && ELEVATOR_POS_TABLE[2].x >= column &&
            ELEVATOR_POS_TABLE[2].y <= row + 1 && ELEVATOR_POS_TABLE[2].y >= row)
            break;
        for (row_offset = -1; row_offset < 3; ++row_offset)
            for (column_offset = -1; column_offset < 3; ++column_offset) {
                tile = gMapDataA[row + row_offset][column + column_offset];
                if (tile != region && tile != ((region & 0x700) | 0x1000))
                    return 0;
            }
        for (row_offset = 0; row_offset < 2; ++row_offset)
            for (column_offset = 0; column_offset < 2; ++column_offset)
                if (gMapDataA[row + row_offset][column + column_offset] != region) {
                    *row_position = row;
                    *column_position = column;
                    return 1;
                }
    }
    return 0;
}
