extern unsigned short LevelMetatileMaps[122][8][16];
extern unsigned char PlayerTilePropertyLookup[398];
extern unsigned short GetRandomNumber();
extern unsigned short gMapDataA[26][19], gMapDataB[26][19];
extern char gPlayerLives[], gPlayer2Lives;
extern int func_012db8();

struct placement_entity {
    short x, y;
    char reserved04[72];
    char level;
    char reserved4d[51];
};
extern struct placement_entity gPlayerInfo[40];

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

    if ((metatile & 0x1f00) && !(flags & 4))
        return 0;
    if (metatile & 0x80)
        mirrored = 1;
    metatile &= 0x7f;
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
                    if (!(((flags & 1) && property == 16) ||
                          ((flags & 2) && property == 32) ||
                          ((flags & 8) && property == 128))) {
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

    if (gPlayerLives[0] < 0)
        player = 1;
    else if (gPlayer2Lives < 0)
        player = 0;
    if (player == 2) {
        width = 12;
        level = gPlayerInfo[0].level;
    } else {
        width = 4;
        level = gPlayerInfo[player].level;
    }
    if (level <= 1) {
        width = 16;
        height = 8;
    }
    if (player == 1 && gPlayerLives[0] >= 0)
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
                if (!func_012db8(position_x - 32, position_y, level) &&
                    !func_012db8(position_x + 32, position_y, level))
                    goto set_position;
            } else if (!func_012db8(position_x, position_y, level))
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
                    if (!func_012db8(position_x - 32, position_y, level) &&
                        !func_012db8(position_x + 32, position_y, level))
                        goto set_position;
                } else if (!func_012db8(position_x, position_y, level))
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
        gPlayerInfo[0].y = position_y;
        gPlayerInfo[0].x = position_x;
    }
    if (player == 1 || player == 2) {
        gPlayerInfo[1].y = position_y;
        gPlayerInfo[1].x = position_x;
        if (player == 2)
            gPlayerInfo[1].x += 64;
    }
}
