#define TJE_LOCAL_FUNC_00D9DC
#define TJE_LOCAL_FUNC_020EAA
#define TJE_LOCAL_GETRANDOMNUMBER

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "assets_earthlings.h"
#include "assets_sprites.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_earthlings.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_ship.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "ship.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * Both the shared unsigned char declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int func_00d9dc();

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * Both the shared char declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int func_020eaa();

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * Both the shared unsigned short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int GetRandomNumber();

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
        if (earthling->kind != -1) {
            other_definition = &EARTHLING_DATA[earthling->kind];
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
        if (!earthling->kind || func_020e70(earthling)) {
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
        random = (unsigned short)GetRandomNumber();
        row = random % 26;
        random = (unsigned short)GetRandomNumber();
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
    random = (unsigned short)GetRandomNumber();
    search_counter = random % 26;
    random = (unsigned short)GetRandomNumber();
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
            EARTHLINGS_IN_LEVEL[bank][record_number].kind = kind;
            EARTHLINGS_IN_LEVEL[bank][record_number].level = level;
            EARTHLINGS_IN_LEVEL[bank][record_number].x = position_x;
            EARTHLINGS_IN_LEVEL[bank][record_number].y = position_y;
            EARTHLINGS_IN_LEVEL[bank][record_number].value_0c = position_y;
            if (kind == 19 || kind == 9) {
                EARTHLINGS_IN_LEVEL[bank][record_number].value_0a = 0;
                EARTHLINGS_IN_LEVEL[bank][record_number].value_0c = 0;
            } else if (kind == 7)
                EARTHLINGS_IN_LEVEL[bank][record_number].value_0a = 1;
            else
                EARTHLINGS_IN_LEVEL[bank][record_number].value_0a = position_x;
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved_10[0] = EARTHLING_DATA[kind].health;
            EARTHLINGS_IN_LEVEL[bank][record_number].reserved_10[1] = 0;
        } else
            EARTHLINGS_IN_LEVEL[bank][record_number].kind = -1;
    }
    ++EARTHLING_LOAD_FLAG[bank];
    EARTHLING_LEVEL[bank] = level;
}

/* Mark both banks empty and request level one for every player with lives. */
void SetupInitialEarthlings()
{
    short record_number;

    for (record_number = 0; record_number < 20; ++record_number) {
        EARTHLINGS_IN_LEVEL[0][record_number].kind = -1;
        EARTHLINGS_IN_LEVEL_COPY[record_number].kind = -1;
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
    data = &EARTHLING_DATA[earthling->kind];
    animation = &data->states[state];
    object = &gPlayerInfo[earthling->object];
    object->sequence = animation->sequence;
    object->frames = animation->frames;
    object->frame_phase = animation->period;
    object->frame_duration = 1;
    object->sprite_state = state;
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
    if (!EARTHLING_LOAD_FLAG[0] || EARTHLING_LEVEL[0] != earthling->level)
        other_level = 1;
    do {
        FindSpawnLocationForEarthling(earthling->kind, other_level,
                                     &earthling->x, &earthling->y,
                                     earthling->level);
    } while (func_012db8(earthling->x, earthling->y, earthling->level));
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
    earthling->x = earthling->value_0a;
    earthling->y = earthling->value_0c;
}

/* Allocate display objects and install the initial animation and type fields. */
void func_01814c(earthling)
struct earthling_state *earthling;
{
    struct earthling_definition *data;
    struct object_state *object;
    struct timed_animation *animation;

    data = &EARTHLING_DATA[earthling->kind];
    if (earthling->object = (int)func_00d904()) {
        object = &gPlayerInfo[earthling->object];
        object->earthling = earthling;
        object->process = data->update;
        object->mode_15 = data->movement_mode;
        object->x = earthling->x;
        object->y = earthling->y;
        if (earthling->kind == 7)
            object->height = 40;
        object->level = earthling->level;
        object->width = data->width;
        object->height_pixels = data->height_pixels;
        object->sprite_count = data->sprite_count;
        object->tile_count = data->tile_count;
        if (func_020e70(earthling)) {
            object->sprite_count = 1;
            object->tile_count = 12;
            object->mode_15 = 0;
            object->draw_flags |= 8;
        }
        object->tile = 0x8000;
        if (earthling->kind != 14)
            object->flags_4d |= 1;
        if ((data->flags_18 & 1) &&
            (PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                           (ram_ff91e0 = 0) : ++ram_ff91e0] & 1) &&
            (earthling->second_object = (int)func_00d904())) {
            SetEarthlingState(earthling, 2);
            object = &gPlayerInfo[earthling->second_object];
            object->x = earthling->x;
            object->y = earthling->y + 1;
            object->x += EarthlingCoordinatePairs[earthling->kind * 2];
            object->height = EarthlingCoordinatePairs[earthling->kind * 2 + 1];
            object->level = earthling->level;
            object->width = 32;
            object->height_pixels = 32;
            object->sprite_count = 1;
            object->tile_count = 9;
            object->tile = 0x8000;
            animation = AnimationDescriptors_0AEF9C;
            object->sequence = animation->sequence;
            object->frames = animation->frames;
            object->frame_phase = animation->period;
            object->frame_duration = 1;
        } else if (earthling->kind == 8) {
            object->flags_4d = 64;
            SetEarthlingState(earthling, 3);
        } else
            SetEarthlingState(earthling, 0);
        earthling->timer = 0;
    } else if (!earthling->kind)
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
        if (earthling->kind != -1) {
            if (earthling->level == gLevel912C &&
                toejam_x + 160 > 0 && toejam_x < 480 &&
                toejam_y + 112 > 0 && toejam_y < 336)
                earthling->reserved_10[1] |= 2;
            else
                earthling->reserved_10[1] &= ~2;
            if (earthling->level == gLevel912E && SPLIT_SCREEN &&
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

/* Choose a nearby position and an available or inactive earthling record. */
void OpenEarthlingPresent(player, terrain)
struct object_state *player;
short terrain;
{
    short x, y, bank = 0, player_number = player->entity_number;
    short index, selection, height, object_number, terrain_player;
    unsigned short random;
    struct earthling_state *earthling;

    for (index = 0; index < 4; ++index) {
        x = player->x;
        if ((unsigned short)GetRandomNumber() & 1)
            x += 40;
        else
            x -= 40;
        y = player->y;
        if ((unsigned short)GetRandomNumber() & 1)
            y += 25;
        else
            y -= 25;
        if (player->draw_flags & 2)
            terrain_player = 0;
        else
            terrain_player = 1;
        selection = func_00d9dc(terrain_player, x, y);
        if ((terrain == 0x80 && selection == 0x80) ||
            (terrain != 0x80 && selection != 0x80))
            break;
    }
    terrain = selection;
    if (!EARTHLING_LOAD_FLAG[bank] || EARTHLING_LEVEL[bank] != player->level)
        ++bank;
    earthling = EARTHLINGS_IN_LEVEL[bank];
    for (index = 0; index < 20; ++earthling, ++index)
        if (earthling->kind == -1)
            break;
    /* ORIGINAL BUG: an exhausted scan leaves earthling one past this bank;
     * the original tests that record before looking for an inactive slot. */
    if (earthling->kind != -1) {
        earthling = EARTHLINGS_IN_LEVEL[bank];
        for (index = 0; index < 20; ++earthling, ++index)
            if (earthling->kind != 0 && earthling->kind != 9 &&
                !earthling->object)
                break;
    }
    /* ORIGINAL BUG: exhaustion of the second scan is not checked either. */
    if (!player->level)
        earthling->kind = 5;
    else if (terrain == 0x80)
        earthling->kind = 14;
    else if (terrain == 0x90 || terrain == 0x50)
        earthling->kind = 22;
    else {
        selection = 0;
        while (!selection) {
            earthling->kind = (random = GetRandomNumber()) % 22 + 1;
            if (earthling->kind == 17 || earthling->kind == 20 ||
                earthling->kind == 11 || earthling->kind == 10 ||
                earthling->kind == 9) {
                for (index = 0; index < 20; ++index) {
                    if (earthling != &EARTHLINGS_IN_LEVEL[bank][index] &&
                        EARTHLINGS_IN_LEVEL[bank][index].kind == earthling->kind) {
                        if (!EARTHLINGS_IN_LEVEL[bank][index].object) {
                            earthling = &EARTHLINGS_IN_LEVEL[bank][index];
                            selection = 1;
                        }
                        break;
                    }
                }
                if (index == 20) {
                    if (earthling->kind != 9 || func_020eaa(bank, player->level))
                        selection = 1;
                }
            } else if (earthling->kind != 14 &&
                       (earthling->kind != 8 || terrain == 32) &&
                       (earthling->kind != 13 || terrain == 16))
                selection = 1;
        }
    }
    earthling->level = player->level;
    earthling->x = x;
    earthling->y = y;
    earthling->value_0c = y;
    height = 5;
    if (earthling->kind == 18 || earthling->kind == 19) {
        earthling->value_0a = 0;
        earthling->value_0c = 0;
    } else if (earthling->kind == 7) {
        earthling->value_0a = 1;
        height = 30;
    } else
        earthling->value_0a = x;
    earthling->reserved_10[0] = EARTHLING_DATA[earthling->kind].health;
    earthling->reserved_10[1] = 0;
    func_015b66(player_number, earthling->x, earthling->y + 1,
                height, earthling->level);
    if (object_number = (int)func_00d904())
        func_00d970(object_number);
    else if (object_number = func_01e736(1))
        func_00d970(object_number);
}

/* Invoke eligible loaded earthlings' per-type abilities on alternating slots. */
void func_018834()
{
    short bank, index;
    struct earthling_state *earthling;
    struct earthling_definition *data;

    for (bank = 0; bank < 2; ++bank) {
        if (EARTHLING_LOAD_FLAG[bank]) {
            for (index = ram_ffe1ae; index < 20; index += 2) {
                earthling = &EARTHLINGS_IN_LEVEL[bank][index];
                if (earthling->kind != -1 && earthling->object &&
                    earthling->state != 99 && !func_020e70(earthling)) {
                    data = &EARTHLING_DATA[earthling->kind];
                    if (data->ability && !func_0193fa(earthling))
                        (*data->ability)(earthling);
                }
            }
        }
    }
}

/* Visit alternating occupied earthling slots and test both eligible players
 * against the type's distance limits before calling its touch handler. */
void func_0188ee()
{
    short buffer, slot, player_number, trial;
    struct earthling_state *earthling;
    struct earthling_definition *data;
    struct object_state *object, *player;

    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer]) {
            for (slot = ram_ffe1ae; slot < 20; slot += 2) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->kind == -1 || !earthling->object ||
                    earthling->state == 99)
                    continue;
                data = &EARTHLING_DATA[earthling->kind];
                object = &gPlayerInfo[earthling->object];
                player_number = PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                                (ram_ff91e0 = 0) : ++ram_ff91e0] & 1;
                for (trial = 0; trial < 2; ++trial) {
                    player = &gPlayerInfo[player_number];
                    if (!P1_MENU_OPEN[player_number] &&
                        !ELEVATOR_FLAG[player_number] &&
                        !(player->flags_4d & 16) &&
                        func_00dd1e(object, player, data->contact_distance_x,
                                    data->contact_distance_y)) {
                        if ((data->flags_18 & 1) && earthling->state == 2) {
                            SetEarthlingState(earthling, 0);
                            earthling->timer = 0;
                        }
                        (*data->touch)(player_number, earthling);
                    }
                    player_number ^= 1;
                }
                if (earthling->kind == 7)
                    func_01bb62(earthling, data);
            }
        }
    }
}

/* Nearby active objects in ram_ffde6c put eligible earthlings into state 99,
 * install their sleep animation, and release any secondary object. */
void func_018a6c()
{
    short source, buffer, slot;
    struct earthling_state *earthling;
    struct object_state *source_object, *object;

    for (source = 0; source <= 1; ++source) {
        if (ram_ffde6c[source]) {
            source_object = &gPlayerInfo[ram_ffde6c[source]];
            if (EARTHLING_LOAD_FLAG[0] &&
                EARTHLING_LEVEL[0] == source_object->level)
                buffer = 0;
            else
                buffer = 1;
            for (slot = ram_ffe1ae; slot < 20; slot += 2) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->kind == -1 || earthling->state == 99 ||
                    !earthling->object || earthling->kind == 0 ||
                    earthling->kind == 8 || earthling->kind == 14 ||
                    earthling->kind == 20 || earthling->kind == 23 ||
                    earthling->kind == 24 || func_020e70(earthling))
                    continue;
                if (earthling->kind == 7) {
                    if (earthling->state == 3)
                        continue;
                } else if (earthling->kind == 18) {
                    if (earthling->state == 3)
                        continue;
                } else if (earthling->kind == 11) {
                    if (earthling->state < 2)
                        continue;
                } else if (earthling->kind == 17 || earthling->kind == 10) {
                    if ((earthling->state & 0xfc) != 4)
                        continue;
                }
                if (abs(earthling->x - source_object->x) >= 320 ||
                    abs(earthling->y - source_object->y) >= 224)
                    continue;
                object = &gPlayerInfo[earthling->object];
                object->velocity_x = 0;
                object->velocity_y = 0;
                object->sequence = SleepAnimationDescriptors[earthling->kind].sequence;
                object->frames = SleepAnimationDescriptors[earthling->kind].frames;
                object->frame_phase = 6;
                object->frame_duration = 1;
                object->frame = -1;
                object->sprite_state = 99;
                earthling->state = 99;
                if (earthling->second_object) {
                    func_00d970(earthling->second_object);
                    earthling->second_object = 0;
                }
            }
        }
    }
}

/* Wake each occupied state-99 record in the selected loaded level. */
void func_018c74(level)
short level;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->kind != -1 && earthling->state == 99)
            SetEarthlingState(earthling, 0);
    }
}

/* Reset state-2 earthlings whose type flag and object draw mask permit it. */
int func_018ce2(draw_mask)
char draw_mask;
{
    short buffer, slot;
    int changed;
    struct earthling_state *earthling;
    struct earthling_definition *data;

    changed = 0;
    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer]) {
            for (slot = 0; slot < 20; ++slot) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->kind != -1 && earthling->object &&
                    earthling->state == 2) {
                    data = &EARTHLING_DATA[earthling->kind];
                    if ((data->flags_18 & 1) &&
                        (gPlayerInfo[earthling->object].draw_flags & draw_mask)) {
                        SetEarthlingState(earthling, 0);
                        earthling->timer = 0;
                        changed = 1;
                    }
                }
            }
        }
    }
    return changed;
}

/* Release both allocated objects, mark the record empty, and record its slot
 * in the level's high-bit-first removed-earthling mask. */
void func_018da4(earthling)
struct earthling_state *earthling;
{
    short buffer, slot;

    if (earthling->object) {
        func_00d970(earthling->object);
        earthling->object = 0;
    }
    if (earthling->second_object) {
        func_00d970(earthling->second_object);
        earthling->second_object = 0;
    }
    earthling->kind = -1;
    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer] &&
            EARTHLING_LEVEL[buffer] == earthling->level) {
            for (slot = 0; slot < 20; ++slot) {
                if (earthling == &EARTHLINGS_IN_LEVEL[buffer][slot]) {
                    short level;
                    level = EARTHLING_LEVEL[buffer];
                    ram_ffe142[level] |= (unsigned long)0x80000000 >> slot;
                }
            }
        }
    }
}

/* Decrement the hit-point byte; nice-earthling handling can preserve one hit.
 * Removal also creates the effect at the record's saved position. */
void DamageEarthling(earthling)
struct earthling_state *earthling;
{
    if (earthling && earthling->kind > 0) {
        if (!func_020e70(earthling) && earthling->kind != 20 &&
            earthling->kind < 23 && --earthling->reserved_10[0] <= 0) {
            if (earthling->kind == 17 || earthling->kind == 10 ||
                earthling->kind == 11) {
                if (NiceEarthlingCheck(earthling)) {
                    earthling->reserved_10[0] = 1;
                    return;
                }
            }
            func_018da4(earthling);
            func_01bde8(earthling->x, earthling->y, earthling->level);
        } else if (earthling->second_object) {
            SetEarthlingState(earthling, 0);
            earthling->timer = 0;
        }
    }
}

/* Remove eligible earthlings whose saved coordinates now lie on terrain 0x90. */
void func_018f1e(player_number, level)
short player_number, level;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->kind > 0 && earthling->object &&
            earthling->kind != 6 && earthling->kind != 22 &&
            earthling->kind != 7 && earthling->kind != 20 &&
            earthling->kind != 8 && earthling->kind != 19 &&
            func_00d9dc(player_number, earthling->x, earthling->y) == 0x90) {
            earthling->reserved_10[0] = 0;
            DamageEarthling(earthling);
        }
    }
}

/* Damage eligible allocated earthlings whose object matches the draw mask. */
void DoVikingAbility(draw_mask)
char draw_mask;
{
    short buffer, slot;
    struct earthling_state *earthling;

    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer]) {
            for (slot = 0; slot < 20; ++slot) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->kind != -1 && earthling->kind != 17 &&
                    earthling->object &&
                    (gPlayerInfo[earthling->object].draw_flags & draw_mask) &&
                    (earthling->reserved_10[0] > 1 || func_01bdd0()))
                    DamageEarthling(earthling);
            }
        }
    }
}

/* Clear the player's per-mole contact flag for every allocated mole object. */
void func_019086(player)
struct object_state *player;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == player->level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->kind == 18 && earthling->object) {
            if (!player->entity_number)
                gPlayerInfo[earthling->object].flag_57 = 0;
            else
                gPlayerInfo[earthling->object].flag_58 = 0;
        }
    }
}

/* Advance loaded earthlings unless both the menu and HBlank flags are set. */
void ProcessEarthlings()
{
    if (P1_MENU_OPEN[0] && HBLANK_FLAG_1)
        return;
    if (EARTHLING_LOAD_FLAG[0])
        LoadInEarthlings(EARTHLINGS_IN_LEVEL[0]);
    if (EARTHLING_LOAD_FLAG_COPY)
        LoadInEarthlings(EARTHLINGS_IN_LEVEL_COPY);
    func_018834();
    func_0188ee();
    func_018a6c();
    func_020d3e();
    func_0223f6();
    ram_ffe1ae ^= 1;
}
