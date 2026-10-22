struct earthling_state;
struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15;
    void (*process)();
    char *sequence;
    char **frames;
    char reserved_22[4];
    char frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[27], sprite, level;
    unsigned char flags_4d;
    struct earthling_state *earthling;
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer, x, y, reserved_0a, reserved_0c;
    char reserved_0e, second_object, reserved_10[2];
};

struct timed_animation {
    char *sequence;
    char **frames;
    char period, spare;
};
struct earthling_definition {
    struct timed_animation *states;
    void (*update)(), (*ability)(), (*touch)();
    char mode_15;
    unsigned char width, height_pixels;
    char draw_offset, draw_attribute, reserved_15[3];
    unsigned char flags_18, radius, distance_x, distance_y;
    char reserved_1c[8], health, reserved_25;
};

extern struct object_state gPlayerInfo[40];
extern struct earthling_state EARTHLINGS_IN_LEVEL[][20];
extern struct earthling_state EARTHLINGS_IN_LEVEL_COPY[20];
extern struct earthling_definition EARTHLING_DATA[24];
extern struct timed_animation AnimationDescriptors_0AEF9C[];
extern char EarthlingCoordinatePairs[];
extern char EARTHLING_LOAD_FLAG[];
extern char EARTHLING_LEVEL[];
extern unsigned short gMapDataA[26][19];
extern unsigned short gMapDataB[26][19];
extern unsigned short gLevelSeeds[26];
extern char gLevel912C;
extern char gLevel912E;
extern char SPLIT_SCREEN;
extern char ram_ff911e;
extern char ram_ff911f;
extern char ram_ffe212[10];
extern char gHighestLevel;
extern char gShipPieceLevels[10];
extern char LevelEarthlingChoices[26][20];
extern unsigned long ram_ffe142[26];
extern char gPlayerLives[];
extern char gPlayer2Lives;
extern short ram_ffe1ae;
extern short TOEJAM_CAMERA_NEWX;
extern short TOEJAM_CAMERA_NEWY;
extern short EARL_CAMERA_NEWX;
extern short EARL_CAMERA_NEWY;
extern unsigned short PREGEN_RANDOMS[59];
extern unsigned short ram_ff91e0;
extern unsigned short GetRandomNumber();
extern long StackBasedRandom();
extern int abs();
extern int func_0144a6();
extern int func_020e70();
extern int TerrainValidation();
extern int func_012db8();
/* ORIGINAL BUG: the original allocation callers use int, while the accepted
 * func_00d904 definition returns short; only the stored byte/word is used. */
extern int func_00d904();
/* ORIGINAL BUG: the original unit uses an int declaration for func_020eaa;
 * its body normalizes a char result, but OpenEarthlingPresent tests D0.L. */
extern int func_020eaa();
extern void func_00d970();
extern void func_023cb4();
extern void func_01f98e();
extern void func_01e94e();

/* Reject elevator overlap and any occupied earthling inside the combined radii.
 * The original normalizes the overlap predicate to zero or one before testing it. */
int CheckEarthlingOverlap(definition, bank, x, y, level)
struct earthling_definition *definition;
short bank, x, y, level;
{
    short record_number, clearance;
    struct earthling_state *earthling;
    struct earthling_definition *other_definition;

    if (!func_0144a6(x, y, level))
        return 0;
    for (record_number = 0; record_number < 20; ++record_number) {
        earthling = &EARTHLINGS_IN_LEVEL[bank][record_number];
        if (earthling->reserved != -1) {
            other_definition = &EARTHLING_DATA[earthling->reserved];
            clearance = (short)other_definition->radius << 2;
            clearance += (short)definition->radius << 2;
            if (clearance > abs(x - earthling->x) &&
                clearance > abs(y - earthling->y) ? 1 : 0)
                return 0;
        }
    }
    return 1;
}

/* Test the first two records in the bank selected for this level. */
int func_01771a(x, y, level)
short x, y, level;
{
    short bank, record_number, distance_x, distance_y;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == level)
        bank = 0;
    else
        bank = 1;
    earthling = EARTHLINGS_IN_LEVEL[bank];
    for (record_number = 0; record_number < 2; ++record_number, ++earthling) {
        if (!earthling->reserved || func_020e70(earthling)) {
            distance_x = abs(x - earthling->x);
            distance_y = abs(y - earthling->y);
            if (distance_x < 340 && distance_y < 200)
                return 1;
        }
    }
    return 0;
}

/* Try ten random metatiles, then search every tile cyclically for a clear spawn. */
int FindSpawnLocationForEarthling(kind, bank, output_x, output_y, level)
short kind, bank;
short *output_x, *output_y;
short level;
{
    unsigned short metatile;
    short start_column, width, height;
    unsigned short *map;
    short position_x, position_y;
    short column, row, search_counter;
    register unsigned short random;
    unsigned char terrain_mask;
    struct earthling_definition *definition;

    if (gLevel912C == level)
        map = gMapDataA[0];
    else
        map = gMapDataB[0];
    definition = &EARTHLING_DATA[kind];
    terrain_mask = 3;
    if (!kind || kind == 12 || kind == 19 || kind == 13 || kind == 18) {
        width = 16;
        height = 8;
    } else if (kind == 8) {
        terrain_mask = 2;
        width = 2;
        height = 2;
    } else if (kind == 14) {
        terrain_mask = 8;
        width = 3;
        height = 2;
    } else {
        width = 8;
        height = 2;
    }
    for (search_counter = 0; search_counter < 10; ++search_counter) {
        random = GetRandomNumber();
        row = random % 26;
        random = GetRandomNumber();
        column = random % 19;
        metatile = map[row * 19 + column];
        if (TerrainValidation(metatile, width, height, &position_x, &position_y, terrain_mask)) {
            position_y += (row + 1) * 64;
            position_x += (column + 1) * 128;
            if (CheckEarthlingOverlap(definition, bank, position_x, position_y, level)) {
                *output_x = position_x;
                *output_y = position_y;
                return 0;
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
            if (TerrainValidation(metatile, width, height, &position_x, &position_y, terrain_mask)) {
                position_y += (row + 1) * 64;
                position_x += (column + 1) * 128;
                if (CheckEarthlingOverlap(definition, bank, position_x, position_y, level)) {
                    *output_x = position_x;
                    *output_y = position_y;
                    return 0;
                }
            }
            column = (column + 1) % 19;
        } while (column != start_column);
        row = (row + 1) % 26;
    } while (row != search_counter);
    return 1;
}

/* Convert the stored byte coordinates to the ship-piece world position. */
void func_017a94(output_x, output_y)
short *output_x, *output_y;
{
    short map_x, map_y;

    map_x = ram_ff911e;
    map_y = ram_ff911f;
    *output_x = (map_x + 2) * 16 * 8;
    *output_y = (map_y + 2) * 8 * 8 + 8;
}

/* Retain an already loaded bank, or populate an unused bank from level choices. */
void func_017ac8(level)
short level;
{
    char ship_piece = 0;
    short bank, record_number, position_x, position_y;
    char special, kind;
    long saved_random;

    if (level >= 26)
        return;
    for (bank = 0; bank < 2; ++bank) {
        if (EARTHLING_LOAD_FLAG[bank] && EARTHLING_LEVEL[bank] == level) {
            ++EARTHLING_LOAD_FLAG[bank];
            return;
        }
    }
    bank = 0;
    if (EARTHLING_LOAD_FLAG[0])
        bank = 1;
    saved_random = StackBasedRandom(gLevelSeeds[level]);
    if (level) {
        for (record_number = 0; record_number < 10; ++record_number) {
            if (ram_ffe212[record_number] == level)
                ship_piece = 1;
        }
    }
    special = func_020eaa(bank, level);
    for (record_number = 0; record_number < 20; ++record_number) {
        if (level == 1) {
            if (saved_random) {
                StackBasedRandom(saved_random);
                saved_random = 0;
            }
            if (!record_number)
                break;
            kind = 18;
        } else if (special) {
            kind = 9;
            special = 0;
        } else if (ship_piece) {
            kind = 0;
            func_017a94(&position_x, &position_y);
            ship_piece = 0;
            if (gHighestLevel == level)
                gShipPieceLevels[4] = level;
        } else {
            if (saved_random) {
                StackBasedRandom(saved_random);
                saved_random = 0;
            }
            kind = LevelEarthlingChoices[level][record_number];
            if (kind == 20 && gHighestLevel > level)
                kind = 1;
        }
        if (kind > 0) {
            if (!level)
                func_023cb4(kind, &position_x, &position_y);
            else if (FindSpawnLocationForEarthling(kind, bank, &position_x, &position_y, level))
                ram_ffe142[level] |= (unsigned long)0x80000000 >> record_number;
        }
        if (!kind || (kind >= 0 && !(ram_ffe142[level] & ((unsigned long)0x80000000 >> record_number)))) {
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved = kind;
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved_2 = level;
            EARTHLINGS_IN_LEVEL[bank][record_number].x = position_x;
            EARTHLINGS_IN_LEVEL[bank][record_number].y = position_y;
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved_0c = position_y;
            if (kind == 19 || kind == 9) {
                EARTHLINGS_IN_LEVEL[bank][record_number].reserved_0a = 0;
                EARTHLINGS_IN_LEVEL[bank][record_number].reserved_0c = 0;
            } else if (kind == 7)
                EARTHLINGS_IN_LEVEL[bank][record_number].reserved_0a = 1;
            else
                EARTHLINGS_IN_LEVEL[bank][record_number].reserved_0a = position_x;
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved_10[0] = EARTHLING_DATA[kind].health;
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved_10[1] = 0;
        } else
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved = -1;
    }
    ++EARTHLING_LOAD_FLAG[bank];
    EARTHLING_LEVEL[bank] = level;
}

/* Mark both banks empty and request level one for every player with lives. */
void SetupInitialEarthlings()
{
    short record_number;

    for (record_number = 0; record_number < 20; ++record_number) {
        EARTHLINGS_IN_LEVEL[0][record_number].reserved = -1;
        EARTHLINGS_IN_LEVEL_COPY[record_number].reserved = -1;
    }
    if (gPlayerLives[0] >= 0)
        func_017ac8(1);
    if (gPlayer2Lives >= 0)
        func_017ac8(1);
    func_01f98e();
}

/* Install the state animation, scan its frame controls, and release a secondary
 * object when leaving state 2. */
void SetEarthlingState(earthling, state)
struct earthling_state *earthling;
short state;
{
    struct timed_animation *animation;
    struct object_state *object;
    struct earthling_definition *data;
    short sequence_index;

    earthling->state = state;
    if (!earthling->object)
        return;
    data = &EARTHLING_DATA[earthling->reserved];
    animation = &data->states[state];
    object = &gPlayerInfo[earthling->object];
    object->sequence = animation->sequence;
    object->frames = animation->frames;
    object->frame_phase = animation->period;
    object->frame_duration = 1;
    object->sprite = state;
    if (object->frame >= 0)
        --object->frame;
    for (sequence_index = 0; sequence_index <= object->frame; ++sequence_index)
        if ((object->sequence[sequence_index] & 0xf0) == 0xf0)
            object->frame = -1;
    if (state != 2 && earthling->second_object) {
        func_00d970(earthling->second_object);
        earthling->second_object = 0;
    }
}

/* Release both displayed objects, then choose a spawn point outside elevators. */
void func_01804e(earthling)
struct earthling_state *earthling;
{
    char other_level = 0;

    if (earthling->object) {
        func_00d970(earthling->object);
        earthling->object = 0;
    }
    if (earthling->second_object) {
        func_00d970(earthling->second_object);
        earthling->second_object = 0;
    }
    if (!EARTHLING_LOAD_FLAG[0] || EARTHLING_LEVEL[0] != earthling->reserved_2)
        other_level = 1;
    do {
        FindSpawnLocationForEarthling(earthling->reserved, other_level,
                                     &earthling->x, &earthling->y,
                                     earthling->reserved_2);
    } while (func_012db8(earthling->x, earthling->y, earthling->reserved_2));
}

/* Release displayed objects and restore the two saved coordinates. */
void func_0180fe(earthling)
struct earthling_state *earthling;
{
    if (earthling->object) {
        func_00d970(earthling->object);
        earthling->object = 0;
    }
    if (earthling->second_object) {
        func_00d970(earthling->second_object);
        earthling->second_object = 0;
    }
    earthling->x = earthling->reserved_0a;
    earthling->y = earthling->reserved_0c;
}

/* Allocate display objects and install the initial animation and type fields. */
void func_01814c(earthling)
struct earthling_state *earthling;
{
    struct earthling_definition *data;
    struct object_state *object;
    struct timed_animation *animation;

    data = &EARTHLING_DATA[earthling->reserved];
    if (earthling->object = func_00d904()) {
        object = &gPlayerInfo[earthling->object];
        object->earthling = earthling;
        object->process = data->update;
        object->mode_15 = data->mode_15;
        object->x = earthling->x;
        object->y = earthling->y;
        if (earthling->reserved == 7)
            object->height = 40;
        object->level = earthling->reserved_2;
        object->width = data->width;
        object->height_pixels = data->height_pixels;
        object->draw_attribute = data->draw_attribute;
        object->draw_offset = data->draw_offset;
        if (func_020e70(earthling)) {
            object->draw_attribute = 1;
            object->draw_offset = 12;
            object->mode_15 = 0;
            object->draw_flags |= 8;
        }
        object->tile = 0x8000;
        if (earthling->reserved != 14)
            object->flags_4d |= 1;
        if ((data->flags_18 & 1) &&
            (PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                           (ram_ff91e0 = 0) : ++ram_ff91e0] & 1) &&
            (earthling->second_object = func_00d904())) {
            SetEarthlingState(earthling, 2);
            object = &gPlayerInfo[earthling->second_object];
            object->x = earthling->x;
            object->y = earthling->y + 1;
            object->x += EarthlingCoordinatePairs[earthling->reserved * 2];
            object->height = EarthlingCoordinatePairs[earthling->reserved * 2 + 1];
            object->level = earthling->reserved_2;
            object->width = 32;
            object->height_pixels = 32;
            object->draw_attribute = 1;
            object->draw_offset = 9;
            object->tile = 0x8000;
            animation = AnimationDescriptors_0AEF9C;
            object->sequence = animation->sequence;
            object->frames = animation->frames;
            object->frame_phase = animation->period;
            object->frame_duration = 1;
        } else if (earthling->reserved == 8) {
            object->flags_4d = 64;
            SetEarthlingState(earthling, 3);
        } else
            SetEarthlingState(earthling, 0);
        earthling->timer = 0;
    } else if (!earthling->reserved)
        func_01e94e();
    else
        func_01804e(earthling);
}

/* Refresh alternating earthlings and load objects visible to either camera. */
void LoadInEarthlings(earthlings)
struct earthling_state *earthlings;
{
    short index, toejam_x, toejam_y, earl_x, earl_y;
    struct earthling_state *earthling;
    struct object_state *object;

    for (index = ram_ffe1ae; index < 20; index += 2) {
        earthling = &earthlings[index];
        if (earthling->object) {
            object = &gPlayerInfo[earthling->object];
            earthling->x = object->x;
            earthling->y = object->y;
        }
        toejam_x = earthling->x - TOEJAM_CAMERA_NEWX;
        toejam_y = earthling->y - TOEJAM_CAMERA_NEWY;
        earl_x = earthling->x - EARL_CAMERA_NEWX;
        earl_y = earthling->y - EARL_CAMERA_NEWY;
        if (earthling->reserved != -1) {
            if (earthling->reserved_2 == gLevel912C &&
                toejam_x + 160 > 0 && toejam_x < 480 &&
                toejam_y + 112 > 0 && toejam_y < 336)
                earthling->reserved_10[1] |= 2;
            else
                earthling->reserved_10[1] &= ~2;
            if (earthling->reserved_2 == gLevel912E && SPLIT_SCREEN &&
                earl_x + 160 > 0 && earl_x < 480 &&
                earl_y + 112 > 0 && earl_y < 336)
                earthling->reserved_10[1] |= 4;
            else
                earthling->reserved_10[1] &= ~4;
        } else
            earthling->reserved_10[1] = 0;
        if (earthling->reserved_10[1] & 6) {
            if (!earthling->object)
                func_01814c(earthling);
        } else {
            if (earthling->object) {
                func_00d970(earthling->object);
                earthling->object = 0;
            }
            if (earthling->second_object) {
                func_00d970(earthling->second_object);
                earthling->second_object = 0;
            }
        }
    }
}
