#include "terrain.h"
#include "entities.h"
#include "assets_map.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "ram_map.h"
#include "ram_player.h"
#include "random.h"

/* Search all rectangle origins, beginning at a random origin. Bit 7 of the
 * metatile mirrors the returned X coordinate; flags select tile properties. */
int TerrainValidation(metatile, width, height, position_x, position_y, flags)
unsigned short metatile;
short width, height;
short *position_x, *position_y;
unsigned char flags;
{
    short column, origin_x, row, origin_y;
    char mirrored = 0;
    short start_x, start_y;
    unsigned short tile_word;
    unsigned char property;
    char valid;

    if ((metatile & METATILE_OVERLAY_MASK) && !(flags & 4))
        return 0;
    if (metatile & METATILE_MIRROR_X)
        mirrored = 1;
    metatile &= METATILE_BASE_MASK;
    if (metatile == 34 || metatile == 82 || metatile == 84)
        return 0;
    if (width >= 16 && height >= 8) {
        if (((flags & 1) && (metatile == 30 || metatile == 33)) ||
            ((flags & 8) && (metatile == 75 || metatile == 76 || metatile == 80)) ||
            ((flags & 2) && (metatile == 53 || (metatile >= 56 && metatile <= 58)))) {
            *position_x = 63;
            *position_y = 31;
            return 1;
        }
        return 0;
    }
    start_y = (unsigned short)GetRandomNumber() % (9 - height);
    start_x = (unsigned short)GetRandomNumber() % (17 - width);
    origin_y = start_y;
    do {
        origin_x = start_x;
        do {
            valid = 1;
            for (row = origin_y; row < origin_y + height; ++row) {
                for (column = origin_x; column < origin_x + width; ++column) {
                    tile_word = LevelMetatileMaps[metatile][row][column];
                    property = PlayerTilePropertyLookup[tile_word & 0x7ff];
                    if (!(((flags & 1) && property == TERRAIN_GROUND) ||
                          ((flags & 2) && property == TERRAIN_SINKING) ||
                          ((flags & 8) && property == TERRAIN_WATER))) {
                        valid = 0;
                        break;
                    }
                }
                if (!valid)
                    break;
            }
            if (valid) {
                *position_y = origin_y * 8 + height * 8 - 2;
                *position_x = origin_x * 8 + width * 4;
                if (mirrored)
                    *position_x = 128 - *position_x;
                return 1;
            }
            origin_x = (origin_x + 1) % (17 - width);
        } while (origin_x != start_x);
        origin_y = (origin_y + 1) % (9 - height);
    } while (origin_y != start_y);
    return 0;
}

/* Try ten random unmarked metatiles, then scan cyclically from a random
 * metatile. Mode 2 places both players, separated by 64 world X units. */
void SetPositionOnNewLevel(player)
register int player;
{
    unsigned short metatile;
    short start_column, width, height = 2;
    unsigned short *map;
    short position_x, position_y;
    short column, row, search_counter;
    register unsigned short random;
    char level;

    if (gPlayerLives[PLAYER_TOEJAM] < 0)
        player = 1;
    else if (gPlayer2Lives < 0)
        player = 0;
    if (player == 2) {
        width = 12;
        level = gPlayerInfo[PLAYER_TOEJAM].level;
    } else {
        width = 4;
        level = gPlayerInfo[player].level;
    }
    if (level <= 1) {
        width = 16;
        height = 8;
    }
    if (player == 1 && gPlayerLives[PLAYER_TOEJAM] >= 0)
        map = gMapDataB[0];
    else
        map = gMapDataA[0];
    for (search_counter = 0; search_counter < 10; ++search_counter) {
        random = GetRandomNumber();
        row = random % 26;
        random = GetRandomNumber();
        column = random % 19;
        metatile = map[row * 19 + column];
        if (metatile & 0x4000) {
            --search_counter;
            continue;
        }
        if (TerrainValidation(metatile, width, height, &position_x, &position_y, 3)) {
            position_y += (row + 1) * 64;
            position_x += (column + 1) * 128;
            if (player == 2) {
                if (!ElevatorContainsPosition(position_x - 32, position_y, level) &&
                    !ElevatorContainsPosition(position_x + 32, position_y, level))
                    goto set_position;
            } else if (!ElevatorContainsPosition(position_x, position_y, level))
                goto set_position;
        }
    }
    /* Reuse the attempt counter as the exhaustive scan's starting row. */
    random = GetRandomNumber();
    search_counter = random % 26;
    random = GetRandomNumber();
    start_column = random % 19;
    row = search_counter;
    do {
        column = start_column;
        do {
            metatile = map[row * 19 + column];
            if (!(metatile & 0x4000) &&
                TerrainValidation(metatile, width, height, &position_x, &position_y, 3)) {
                position_y += (row + 1) * 64;
                position_x += (column + 1) * 128;
                if (player == 2) {
                    if (!ElevatorContainsPosition(position_x - 32, position_y, level) &&
                        !ElevatorContainsPosition(position_x + 32, position_y, level))
                        goto set_position;
                } else if (!ElevatorContainsPosition(position_x, position_y, level))
                    goto set_position;
            }
            column = (column + 1) % 19;
        } while (column != start_column);
        row = (row + 1) % 26;
    } while (row != search_counter);
set_position:
    /* ORIGINAL BUG: If the exhaustive scan finds no valid position, its last
     * output coordinates are still used. The random loop also does not count
     * marked metatiles, so an entirely marked map never reaches the scan. */
    if (player == 0 || player == 2) {
        gPlayerInfo[PLAYER_TOEJAM].y = position_y;
        gPlayerInfo[PLAYER_TOEJAM].x = position_x;
    }
    if (player == 1 || player == 2) {
        gPlayerInfo[PLAYER_EARL].y = position_y;
        gPlayerInfo[PLAYER_EARL].x = position_x;
        if (player == 2)
            gPlayerInfo[PLAYER_EARL].x += 64;
    }
}
