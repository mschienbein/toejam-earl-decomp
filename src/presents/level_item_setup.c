struct present_info {
    char sprite, identified;
};
struct level_item {
    char kind, level, object, reserved;
    short x, y;
};
struct elevator_position { char x, y; };
struct item_position { short x, y; };
extern struct present_info gPresentInfo[28];
extern char gPlayerInventory[][16], gPlayerLives[], gPlayer2Lives;
extern struct level_item gObjectTable[][32];
extern struct level_item LEVEL_ITEMS_2[32], gDroppedPresentTable[32];
extern char LEVEL_LOADED_1[], LEVEL_LOADED_2, LEVEL_PLAYERS_FLAG_1[];
extern struct elevator_position ELEVATOR_POS_TABLE[8];
extern struct item_position Level1PresentLocations[12];
extern unsigned short gMapDataA[26][19], gMapDataB[26][19];
extern unsigned short gLevelSeeds[26];
extern unsigned long ram_ffdde8[26];
extern char gLevel912C, dat_08bfce[23], PresentWeightTable[27];
extern unsigned short GetRandomNumber();
extern long StackBasedRandom();
/* ORIGINAL BUG: func_00d9dc returns unsigned char in terrain_at_position.c,
 * but this caller treats it as int ([INFERENCE] an implicit declaration). */
extern int abs(), TerrainValidation(), func_01771a(), func_00d9dc();
extern void PlaceStuffInLevel(), func_00d970(), func_023c6e();

/* Reserve the two fixed identities, then assign the other present sprites
 * to random unused identities, scanning forward through collisions. */
void SetupPresentIdentities()
{
    short present_number, identity;

    for (present_number = 0; present_number < 27; ++present_number) {
        gPresentInfo[present_number].sprite = -1;
        gPresentInfo[present_number].identified = 0;
    }
    gPresentInfo[26].sprite = 26;
    gPresentInfo[27].sprite = 27;
    gPresentInfo[27].identified = 1;
    for (present_number = 0; present_number < 26; ++present_number) {
        identity = GetRandomNumber() % 27;
        while (gPresentInfo[identity].sprite != -1)
            identity = (identity + 1) % 27;
        gPresentInfo[identity].sprite = present_number;
    }
}

/* Clear both inventories and the three world-item banks. Each active player
 * receives two fixed presents, or four when the other player is absent. */
void SetupInitialInventory()
{
    short slot;

    for (slot = 0; slot < 16; ++slot) {
        gPlayerInventory[0][slot] = -1;
        gPlayerInventory[1][slot] = -1;
    }
    for (slot = 0; slot < 32; ++slot) {
        gDroppedPresentTable[slot].kind = -1;
        gObjectTable[0][slot].kind = -1;
        LEVEL_ITEMS_2[slot].kind = -1;
    }
    LEVEL_LOADED_1[0] = -1;
    LEVEL_LOADED_2 = -1;
    SetupPresentIdentities();
    if (gPlayerLives[0] >= 0) {
        PlaceStuffInLevel(1);
        gPlayerInventory[0][0] = 27;
        gPlayerInventory[0][1] = 27;
        if (gPlayer2Lives < 0) {
            gPlayerInventory[0][2] = 27;
            gPlayerInventory[0][3] = 27;
        }
    }
    if (gPlayer2Lives >= 0) {
        PlaceStuffInLevel(1);
        gPlayerInventory[1][0] = 27;
        gPlayerInventory[1][1] = 27;
        if (gPlayerLives[0] < 0) {
            gPlayerInventory[1][2] = 27;
            gPlayerInventory[1][3] = 27;
        }
    }
}

/* Release a loaded item bank after its last player leaves this level. */
void func_0143e6(level)
short level;
{
    short bank, item_number;
    char object;

    for (bank = 0; bank < 2; ++bank) {
        if (LEVEL_PLAYERS_FLAG_1[bank] && LEVEL_LOADED_1[bank] == level) {
            --LEVEL_PLAYERS_FLAG_1[bank];
            if (!LEVEL_PLAYERS_FLAG_1[bank]) {
                LEVEL_LOADED_1[bank] = -1;
                for (item_number = 0; item_number < 32; ++item_number) {
                    object = gObjectTable[bank][item_number].object;
                    gObjectTable[bank][item_number].kind = -1;
                    if (object) {
                        func_00d970(object);
                        gObjectTable[bank][item_number].object = 0;
                    }
                }
            }
        }
    }
}

/* Level 1 always permits placement. On other levels reject positions within
 * 30 X units and 20 Y units of either converted elevator coordinate. */
int func_0144a6(world_x, world_y, level)
short world_x, world_y, level;
{
    short elevator_number, elevator_x, elevator_y;
    char map_x, map_y;

    if (level == 1)
        return 1;
    for (elevator_number = 0; elevator_number < 2; ++elevator_number) {
        map_x = ELEVATOR_POS_TABLE[elevator_number].x;
        map_y = ELEVATOR_POS_TABLE[elevator_number].y;
        elevator_x = (map_x + 2) * 16 * 8;
        elevator_x -= 16;
        elevator_y = (map_y + 2) * 8 * 8;
        elevator_y -= 5;
        if (abs(world_x - elevator_x) < 30 &&
            abs(world_y - (elevator_y + 5)) < 20)
            return 0;
    }
    return 1;
}

/* Test the low seven terrain bits against a 23-entry terrain table. */
int func_01454a(terrain)
short terrain;
{
    short entry;

    terrain &= 0x7f;
    for (entry = 0; entry < 23; ++entry)
        if (dat_08bfce[entry] == terrain)
            return 1;
    return 0;
}

/* Try ten random eligible metatiles, then scan cyclically from a random tile. */
void LoadLevel2AndUp(output_x, output_y, level, large)
short *output_x, *output_y;
short level, large;
{
    unsigned short metatile;
    short start_column, width, height;
    unsigned short *map;
    short position_x, position_y;
    short column, row, search_counter;
    register unsigned short random;

    if (gLevel912C == level)
        map = gMapDataA[0];
    else
        map = gMapDataB[0];
    if (large) {
        width = 4;
        height = 2;
    } else {
        width = 3;
        height = 1;
    }
    for (search_counter = 0; search_counter < 10; ++search_counter) {
        random = GetRandomNumber();
        row = random % 26;
        random = GetRandomNumber();
        column = random % 19;
        metatile = map[row * 19 + column];
        if ((metatile & 0x2000) || (large && func_01454a(metatile))) {
            /* ORIGINAL BUG: rejected marked tiles do not consume an attempt. */
            --search_counter;
            continue;
        }
        if (TerrainValidation(metatile, width, height, &position_x, &position_y, 3)) {
            position_y += (row + 1) * 64;
            position_x += (column + 1) * 128;
            if (func_0144a6(position_x, position_y, level)) {
                *output_x = position_x;
                *output_y = position_y;
                return;
            }
        }
    }
    random = GetRandomNumber();
    search_counter = random % 26;
    random = GetRandomNumber();
    start_column = random % 19;
    row = search_counter;
    do {
        column = start_column;
        do {
            metatile = map[row * 19 + column];
            if ((metatile & 0x2000) || (large && func_01454a(metatile))) {
                column = (column + 1) % 19;
                continue;
            }
            if (TerrainValidation(metatile, width, height, &position_x, &position_y, 3)) {
                position_y += (row + 1) * 64;
                position_x += (column + 1) * 128;
                if (func_0144a6(position_x, position_y, level)) {
                    *output_x = position_x;
                    *output_y = position_y;
                    return;
                }
            }
            column = (column + 1) % 19;
        } while (column != start_column);
        row = (row + 1) % 26;
    } while (row != search_counter);
}

/* Place four special objects after the first 28 item slots, avoiding nearby
 * items, elevators and positions rejected by func_01771a. */
void func_014850(items, level)
struct level_item *items;
short level;
{
    short current_x, current_y, slot, attempts, map_index;
    short position_x, position_y, item_number, blocked;
    char map_x, map_y;

    if (gLevel912C == level)
        map_index = 0;
    else
        map_index = 1;
    for (slot = 28; slot < 32; ++slot) {
        for (attempts = 0; attempts < 10; ++attempts) {
            LoadLevel2AndUp(&position_x, &position_y, level, 1);
            blocked = 0;
            for (item_number = 28; item_number < slot; ++item_number) {
                current_x = items[item_number].x;
                current_y = items[item_number].y;
                if (abs(position_x - current_x) < 340 &&
                    abs(position_y - current_y) < 150) {
                    blocked = 1;
                    break;
                }
            }
            if (blocked)
                continue;
            for (item_number = 0; item_number < 28; ++item_number) {
                if (items[item_number].kind != -1) {
                    current_x = items[item_number].x;
                    current_y = items[item_number].y;
                    if (abs(position_x - current_x) < 16 &&
                        abs(position_y - current_y) < 10) {
                        blocked = 1;
                        break;
                    }
                }
            }
            if (blocked)
                continue;
            for (item_number = 0; item_number < 2; ++item_number) {
                map_x = ELEVATOR_POS_TABLE[item_number].x;
                map_y = ELEVATOR_POS_TABLE[item_number].y;
                current_x = (map_x + 2) * 16 * 8;
                current_x -= 16;
                current_y = (map_y + 2) * 8 * 8;
                current_y -= 5;
                if (abs(position_x - current_x) < 340 &&
                    abs(position_y - current_y) < 150) {
                    blocked = 1;
                    break;
                }
            }
            if (blocked)
                continue;
            if (func_01771a(position_x, position_y, level))
                blocked = 1;
            if (!blocked)
                break;
        }
        if (attempts == 10)
            items[slot].kind = -1;
        else {
            if (func_00d9dc(map_index, position_x, position_y) == 32) {
                if (GetRandomNumber() & 1)
                    items[slot].kind = 82;
                else
                    items[slot].kind = 83;
            } else
                items[slot].kind = 81;
            items[slot].level = level;
            items[slot].x = position_x;
            items[slot].y = position_y;
        }
    }
}

/* Select a present using the 27 cumulative byte weights, retrying a zero draw. */
int SelectRandomPresent()
{
    short total, selected, present;
    register unsigned short random;

    total = 0;
    for (present = 0; present < 27; ++present)
        total += PresentWeightTable[present];
    do {
        random = GetRandomNumber();
        selected = random % (total + 1);
    } while (!selected);
    total = 0;
    for (present = 0; present < 27; ++present) {
        total += PresentWeightTable[present];
        if (selected <= total)
            return present;
    }
    return present;
}

/* Reuse an already loaded item bank or populate a free bank from its level seed. */
void PlaceStuffInLevel(level)
short level;
{
    short bank, slot, count;
    short position_x, position_y;
    char kind;
    long saved_seed;

    if (level >= 26)
        return;
    for (bank = 0; bank < 2; ++bank) {
        if (LEVEL_PLAYERS_FLAG_1[bank] && LEVEL_LOADED_1[bank] == level) {
            ++LEVEL_PLAYERS_FLAG_1[bank];
            return;
        }
    }
    bank = 0;
    if (LEVEL_PLAYERS_FLAG_1[0])
        bank = 1;
    saved_seed = StackBasedRandom(gLevelSeeds[level]);
    if (gPlayerLives[0] < 0 || gPlayer2Lives < 0)
        count = 12;
    else
        count = 16;
    if (level > 2)
        count += level - 2;
    if (count > 28)
        count = 28;
    for (slot = 0; slot < count; ++slot) {
        if (!level)
            break;
        if (level == 1) {
            if (slot == 12)
                break;
            position_x = Level1PresentLocations[slot].x;
            position_y = Level1PresentLocations[slot].y;
        } else
            LoadLevel2AndUp(&position_x, &position_y, level, 0);
        if (level == 1)
            goto get_present;
        if (GetRandomNumber() % 100 < 50) {
get_present:
            kind = SelectRandomPresent();
            if (level == 1 && slot < 4) {
                while (kind == 0 || kind == 2 || kind == 5 || kind == 26 ||
                       kind == 18 || kind == 16)
                    kind = SelectRandomPresent();
            }
        } else {
            if (GetRandomNumber() % 100 < 75) {
                kind = GetRandomNumber() % 16;
                kind += 64;
            } else
                kind = 80;
        }
        if (!(ram_ffdde8[level] & ((unsigned long)0x80000000 >> slot))) {
            gObjectTable[bank][slot].kind = kind;
            gObjectTable[bank][slot].level = level;
            gObjectTable[bank][slot].x = position_x;
            gObjectTable[bank][slot].y = position_y;
        } else {
            gObjectTable[bank][slot].kind = -1;
        }
    }
    if (!level)
        func_023c6e(&gObjectTable[bank][28]);
    else if (level != 1)
        func_014850(gObjectTable[bank], level);
    ++LEVEL_PLAYERS_FLAG_1[bank];
    LEVEL_LOADED_1[bank] = level;
    StackBasedRandom(saved_seed);
}
