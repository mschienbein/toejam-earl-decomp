extern unsigned short gMapDataA[26][19], gMapDataB[26][19], gMapDataC[26][19];
extern unsigned char gMapDataD[52][19];
extern unsigned char ram_ff8f2c[26][19];
extern char gLevel912C, gLevel912E, gLevel9130;
extern char gWorldType, gHighestLevel, gLevel912A;
extern unsigned short gLevelSeeds[26], FixedWorldSeeds[26];
extern unsigned short GetRandomNumber();
extern unsigned long func_004f50();
extern long abs();
struct elevator_position { char x, y; };
extern struct elevator_position ELEVATOR_POS_TABLE[8];
/* Exchange a player's map with the saved map, preserving the other
 * player's visibility nibble in the active mask plane. */
void func_0044e2(player)
short player;
{
    unsigned short row, column, saved_cell;
    unsigned short *map, *saved_map = gMapDataC[0];
    /* QUALIFICATION: the original stores the active-mask pointer on its
     * stack frame and never reads that slot. Preserve the unused initializer. */
    unsigned char (*active_masks)[19] = gMapDataD;
    unsigned char mask;

    if (!player) {
        gLevel9130 = gLevel912C;
        map = gMapDataA[0];
    } else {
        gLevel9130 = gLevel912E;
        map = gMapDataB[0];
    }
    for (row = 0; row < 26; ++row)
        for (column = 0; column < 19; ++column) {
            saved_cell = *map;
            *map++ = *saved_map;
            *saved_map++ = saved_cell;
            mask = gMapDataD[row][column];
            if (!player) {
                gMapDataD[row][column] = (mask & 0xf0) | (ram_ff8f2c[row][column] & 15);
                ram_ff8f2c[row][column] = mask;
            } else {
                gMapDataD[row][column] = (ram_ff8f2c[row][column] << 4) | (mask & 15);
                ram_ff8f2c[row][column] = mask >> 4;
            }
        }
}

/* Install the fixed seed table or draw a seed for each of the 26 levels. */
void GenerateLevelSeeds()
{
    short level;

    for (level = 0; level < 26; ++level)
        if (gWorldType)
            gLevelSeeds[level] = FixedWorldSeeds[level];
        else
            gLevelSeeds[level] = GetRandomNumber();
    gLevel912C = 1;
    gLevel912E = 1;
    gLevel9130 = -1;
    gHighestLevel = 1;
}

/* Generate separated positions inside the level-dependent coordinate
 * bounds and return the requested position count. */
int func_0046c6()
{
    short previous, position, min_y, min_x, count, max_y, max_x;

    for (position = 0; position < 8; ++position) {
        ELEVATOR_POS_TABLE[position].x = 0;
        ELEVATOR_POS_TABLE[position].y = 0;
    }
    if (gLevel912A < 4) {
        min_x = 5;
        max_x = 13;
        min_y = 8;
        max_y = 17;
    } else {
        min_x = 2;
        max_x = 16;
        min_y = 2;
        max_y = 23;
    }
    count = func_004f50(6);
    for (position = 0; position < count; ++position) {
        ELEVATOR_POS_TABLE[position].x =
            GetRandomNumber() % (max_x - min_x + 1) + min_x;
        ELEVATOR_POS_TABLE[position].y =
            GetRandomNumber() % (max_y - min_y + 1) + min_y;
        for (previous = position - 1; previous >= 0; --previous)
            if (abs(ELEVATOR_POS_TABLE[position].x - ELEVATOR_POS_TABLE[previous].x) < 3 &&
                abs(ELEVATOR_POS_TABLE[position].y - ELEVATOR_POS_TABLE[previous].y) < 5) {
                --position;
                break;
            }
    }
    return count;
}

