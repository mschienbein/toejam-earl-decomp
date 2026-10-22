#define TJE_LOCAL_TILEALTERNATIVEPOINTERS

#include "entities.h"
#include "earthlings.h"
#include "presents.h"
#include "map.h"
#include "sprites.h"
#include "revision.h"
#include "assets_map.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "earthling_calls.h"
#include "graphics_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_earthlings.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "sprite_calls.h"

/* Compatible incomplete-array compiler view. The actual complete shared
 * declaration changes the observed instructions; historical source spelling
 * is unknown [INFERENCE]. */
extern unsigned char *TileAlternativePointers[];

/* Obtain a free entity, releasing one existing secondary entity if needed.
 * The nonzero mode skips the initial free-slot allocation attempt. */
short func_01e736(release_first)
short release_first;
{
    short object_number, bank, entry;

    if (!release_first && (object_number = func_00d904()))
        return object_number;
    for (bank = 0; bank < 6; ++bank)
        if (object_number = ram_ffe244[bank]) {
            func_00d970(object_number);
            ram_ffe244[bank] = 0;
            return (short)func_00d904();
        }
    for (bank = 0; bank < 4; ++bank)
        if (object_number = ram_ffe1ee[bank]) {
            func_00d970(object_number);
            ram_ffe1ee[bank] = 0;
            return (short)func_00d904();
        }
    if (func_01c928())
        return (short)func_00d904();
    for (bank = 0; bank < 2; ++bank)
        for (entry = 0; entry < 32; ++entry)
            if (object_number = gObjectTable[bank][entry].object) {
                func_00d970(object_number);
                gObjectTable[bank][entry].object = 0;
                return (short)func_00d904();
            }
    for (bank = 0; bank < 2; ++bank)
        if (EARTHLING_LOAD_FLAG[bank]) {
            for (entry = 0; entry < 20; ++entry)
                if (object_number = EARTHLINGS_IN_LEVEL[bank][entry].second_object) {
                    func_00d970(object_number);
                    EARTHLINGS_IN_LEVEL[bank][entry].second_object = 0;
                    return (short)func_00d904();
                }
            for (entry = 0; entry < 20; ++entry)
                if (EARTHLINGS_IN_LEVEL[bank][entry].kind &&
                    (object_number = EARTHLINGS_IN_LEVEL[bank][entry].object)) {
                    func_00d970(object_number);
                    EARTHLINGS_IN_LEVEL[bank][entry].object = 0;
                    return (short)func_00d904();
                }
        }
    return (short)0;
}

/* Release one entity through the same eviction priority as the allocator. */
void func_01e94e()
{
    short object_number;

    if (object_number = func_01e736(1))
        func_00d970(object_number);
}

/* Reverse the four direction bits; -1 remains the no-direction sentinel. */
char func_01e96c(direction)
int direction;
{
    if (direction != -1) {
        if (direction == 2)
            direction = 8;
        else if (direction == 8)
            direction = 2;
        else if (direction == 1)
            direction = 4;
        else
            direction = 1;
    }
    return direction;
}

/* Find which base shape owns the supplied alternative metatile. */
char func_01e9a0(metatile)
unsigned char metatile;
{
    short shape, alternative;
    unsigned char *alternatives;
    unsigned char base_shape;

    for (shape = 0; shape < 16; ++shape) {
        base_shape = MapGenerationLookupTables.tiles[shape];
        alternatives = TileAlternativePointers[base_shape];
        for (alternative = 1; alternative <= *alternatives; ++alternative)
            if (metatile == *(alternatives + alternative))
                return shape;
    }
    return (char)shape;
}

/* The two map banks share one byte per cell, one direction nibble each. */
unsigned char func_01ea06(bank, row, column)
short bank, row, column;
{
    if (!bank)
        return gMapDataD[row][column] & 15;
    else
        return gMapDataD[row][column] >> 4;
}

/* Remove the pending direction from a marked cell and choose an alternative
 * of the resulting base shape. A result of 1 ends the current path. */
char func_01ea66(bank, row, column)
short bank, row, column;
{
    unsigned short cell;
    short finished = 0;
    unsigned short *map;
    unsigned char directions, shape, metatile;
    char opposite;
    unsigned char *alternatives;
    short alternative;

    if (!bank)
        map = &gMapDataA[0][0];
    else
        map = &gMapDataB[0][0];
    cell = map[row * 19 + column];
    if (!(cell & 0x4000))
        return -1;
    directions = func_01ea06(bank, row, column);
    if (!directions) {
        if (ram_ffe210 != -1)
            finished = 1;
        directions = func_01ea06(bank, row - 1, column);
        if (ram_ffe210 == 4 || (directions & 4)) {
            if (ram_ffe210 == -1)
                ram_ffe210 = 1;
            shape = 16;
        } else {
            directions = func_01ea06(bank, row, column + 1);
            if (ram_ffe210 == 8 || (directions & 8)) {
                if (ram_ffe210 == -1)
                    ram_ffe210 = 2;
                shape = 17;
            } else {
                directions = func_01ea06(bank, row + 1, column);
                if (ram_ffe210 == 1 || (directions & 1)) {
                    if (ram_ffe210 == -1)
                        ram_ffe210 = 4;
                    shape = 18;
                } else {
                    directions = func_01ea06(bank, row, column - 1);
                    if (ram_ffe210 == 2 || (directions & 2)) {
                        if (ram_ffe210 == -1)
                            ram_ffe210 = 8;
                        shape = 19;
                    }
                }
            }
        }
        /* ORIGINAL BUG: if no pending or adjacent direction supplies an edge,
         * shape is read uninitialized here. */
        if (cell & 0x8000)
            shape = MapGenerationLookupTables.edge_alternate[shape];
        else
            shape = MapGenerationLookupTables.edge_plain[shape];
        directions = 0;
    } else {
        metatile = cell & 0xff;
        shape = func_01e9a0(metatile);
        if ((opposite = (char)func_01e96c(ram_ffe210)) != -1) {
            shape |= opposite;
            directions &= ~opposite;
        }
        if (ram_ffe210 == -1 || !(ram_ffe210 & directions)) {
            if (directions & 1)
                ram_ffe210 = 1;
            else if (directions & 2)
                ram_ffe210 = 2;
            else if (directions & 4)
                ram_ffe210 = 4;
            else if (directions & 8)
                ram_ffe210 = 8;
            else {
                if (ram_ffe210 == -1)
                    return -1;
                finished = 1;
            }
        }
        if (!finished)
            shape |= ram_ffe210;
        directions &= ~shape;
        shape = MapGenerationLookupTables.tiles[shape];
        if (!bank) {
            gMapDataD[row][column] &= 0xf0;
            gMapDataD[row][column] |= directions;
        } else {
            gMapDataD[row][column] &= 15;
            gMapDataD[row][column] |= directions << 4;
        }
    }
    alternatives = TileAlternativePointers[shape];
    alternative = GetRandomNumber() % alternatives[0];
    ++alternative;
    cell &= 0x9f00;
    cell |= (unsigned short)alternatives[alternative];
    if (directions)
        cell |= 0x4000;
    cell |= 0x2000;
    map[row * 19 + column] = cell;
    return finished;
}

/* Queue the visible part of one metatile row, then await the VBlank transfer. */
void func_01eddc(cell, row, column, local_row, camera, first_column)
short cell, row, column, local_row, camera, first_column;
{
    short count = 0;
    unsigned short tile;
    short local_column;

    row += local_row;
    count = 0;
    for (local_column = 0; local_column < 16; ++local_column) {
        if (first_column > column + local_column)
            continue;
        if (column + local_column > first_column + 45 - 1)
            break;
        tile = 0;
        if (cell & 0x1f00)
            tile = func_001670((cell & 0x1f00) >> 8, local_row, local_column);
        if (!tile)
            tile = func_001612(cell & 0xff, local_row, local_column);
        if (!count)
            ram_ff8104[camera] = (column + local_column) & 63;
        ram_ff804e[camera][count++] = tile | 0x8000;
    }
    ram_ff8102[camera] = row & 31;
    ram_ff8108[camera] = count;
    ram_ff8106[camera] = 1;
    while (ram_ff8106[camera])
        ;
}

/* Queue the visible part of one metatile column and await its VBlank transfer. */
void func_01ef56(cell, row, column, local_column, camera, first_row)
short cell, row, column, local_column, camera, first_row;
{
    short count = 0;
    unsigned short tile;
    short local_row;

    column += local_column;
    for (local_row = 0; local_row < 8; ++local_row) {
        if (first_row > row + local_row)
            continue;
        if (row + local_row > first_row + ram_ff8198 - 1)
            break;
        tile = 0;
        if (cell & 0x1f00)
            tile = func_001670((cell & 0x1f00) >> 8, local_row, local_column);
        if (!tile)
            tile = func_001612(cell & 0xff, local_row, local_column);
        if (!count)
            ram_ff818a[camera] = (row + local_row) & 31;
        ram_ff810a[camera][count++] = tile | 0x8000;
    }
    ram_ff818c[camera] = column & 63;
    ram_ff8190[camera] = count;
    ram_ff818e[camera] = 1;
    while (ram_ff818e[camera])
        ;
}

/* Start one available path animation at the supplied world position. */
void func_01f0c4(camera, x, y)
short camera, x, y;
{
    short animation;
    struct object_state *object;

    for (animation = 0; animation < 8; ++animation) {
        object = &gPlayerInfo[ram_ffe208[animation]];
        if (!object->frame_duration) {
            object->frame = -1;
            object->frame_duration = 1;
            object->x = x;
            object->y = y;
            if (ram_ffe210 & 5)
                object->x += (short)(GetRandomNumber() % 97) + 16;
            else
                object->y += (short)(GetRandomNumber() % 33) + 16;
            if (!SPLIT_SCREEN || !P1_MENU_OPEN[camera])
                object->flags_4d &= ~64;
            PlaySound(16, 50);
            break;
        }
    }
    func_00cc82();
}

/* Redraw a changing metatile in viewport order and start path effects.
 * Each sweep owns its tile offset; the effect offset is shared by both sweeps.
 * This scope split reproduces the original D6 tile / D5 effect allocation. */
void func_01f1b0(camera, row, column)
short camera, row, column;
{
    short tile_row, tile_column, first_row, first_column;
    unsigned short cell;
    short sweep_step, effect_offset;

    if (!camera) {
        cell = gMapDataA[row][column];
        first_row = ram_ff803e;
        first_column = ram_ff8040;
    } else {
        cell = gMapDataB[row][column];
        first_row = ram_ff8042;
        first_column = ram_ff8044;
    }
    tile_row = (row + 1) << 3;
    sweep_step = column;
    tile_column = (sweep_step + 1) << 4;
    if (first_row > tile_row + 7 ||
        tile_row > ram_ff8198 + first_row - 1 ||
        first_column > tile_column + 15 ||
        tile_column > first_column + 45 - 1)
        return;
    if (ram_ffe210 & 5) {
        for (sweep_step = 0; sweep_step < 8; ++sweep_step) {
            short tile_offset;
            if (ram_ffe210 & 1) {
                tile_offset = 7 - sweep_step;
                effect_offset = tile_offset - 2;
            } else {
                tile_offset = sweep_step;
                effect_offset = tile_offset + 2;
            }
            if (first_row > tile_row + tile_offset)
                continue;
            if (tile_row + tile_offset > first_row + ram_ff8198 - 1)
                continue;
            func_01f0c4(camera, (tile_column << 3) + 8,
                        ((tile_row + effect_offset) << 3) + 8);
            func_01eddc(cell, tile_row, tile_column, tile_offset, camera, first_column);
        }
    } else {
        for (sweep_step = 0; sweep_step < 16; ++sweep_step) {
            short tile_offset;
            if (ram_ffe210 & 8) {
                tile_offset = 15 - sweep_step;
                effect_offset = tile_offset - 2;
            } else {
                tile_offset = sweep_step;
                effect_offset = tile_offset + 2;
            }
            if (first_column > tile_column + tile_offset)
                continue;
            if (tile_column + tile_offset > first_column + 45 - 1)
                continue;
            func_01f0c4(camera, ((tile_column + effect_offset) << 3) + 12,
                        (tile_row << 3) + 8);
            func_01ef56(cell, tile_row, tile_column, tile_offset, camera, first_row);
        }
    }
}

/* Reserve eight animated entities while both tile-transfer queues are idle.
 * The two unused coordinate arguments are present in the observed interface. */
void func_01f418(camera, row, column)
short camera, row, column;
{
    short level, animation, object_number;
    struct object_state *object;
    struct timed_animation *animation_data;

    while (ram_ff8106[camera] || ram_ff818e[camera])
        ;
    if (!camera)
        level = gLevel912C;
    else
        level = gLevel912E;
    for (animation = 0; animation < 8; ++animation) {
        object_number = func_01e736(0);
        ram_ffe208[animation] = object_number;
        object = &gPlayerInfo[object_number];
        object->level = level;
        object->width = 32;
        object->height_pixels = 32;
        object->sprite_count = 1;
        object->tile_count = 16;
        object->tile |= 0x8000;
        animation_data = &dat_0aefec;
        object->sequence = animation_data->sequence;
        object->frames = animation_data->frames;
        object->frame_phase = 2;
        for (;;) {
            func_00d776(object, camera);
            if (object->draw_flags & 6)
                break;
            func_01e94e();
        }
        object->draw_flags |= 128;
        object->flags_4d |= 64;
    }
}

/* Follow all marked branches from one map cell, animating both map banks when
 * they show the same level. Existing frame durations are restored on exit. */
void func_01f4f8(camera, row, column)
short camera, row, column;
{
    short current_row, current_column, object_number, bank_count;
    char saved_durations[26];
    char result;
    short animation;
    struct object_state *object;

    for (animation = 0; animation < 26; ++animation) {
        saved_durations[animation] = gPlayerInfo[animation].frame_duration;
        gPlayerInfo[animation].frame_duration = 0;
    }
    bank_count = 0;
    do {
        /* ORIGINAL BUG: these unused arguments are uninitialized on the first
         * call. func_01f418 reads but does not use either coordinate. */
        func_01f418(camera, current_row, current_column);
        result = 0;
        while (result != -1) {
            current_row = row;
            current_column = column;
            ram_ffe210 = -1;
            result = 0;
            while (!result) {
                result = func_01ea66(camera, current_row, current_column);
                if (result != -1)
                    func_01f1b0(camera, current_row, current_column);
                if (ram_ffe210 == 2)
                    ++current_column;
                else if (ram_ffe210 == 8)
                    --current_column;
                else if (ram_ffe210 == 1)
                    --current_row;
                else
                    ++current_row;
            }
        }
        for (animation = 0; animation < 8; ++animation) {
            object_number = ram_ffe208[animation];
            object = &gPlayerInfo[object_number];
            while (object->frame_duration) {
                func_00cc82();
                while (VBLANK_FLAG_2)
                    ;
            }
            func_00d970(object_number);
            ram_ffe208[animation] = 0;
        }
        if (gLevel912C != gLevel912E)
            break;
        if (!SPLIT_SCREEN) {
            CopyLevelBetweenPlayers(camera);
            break;
        }
        camera ^= 1;
        ++bank_count;
    } while (bank_count < 2);
    for (animation = 0; animation < 26; ++animation)
        gPlayerInfo[animation].frame_duration = saved_durations[animation];
}

/* Either player is near an edge when one of the four adjacent terrain cells
 * has property 0x50 or greater. The historical name mentions only ToeJam. */
int IsToejamNearEdge(player)
short player;
{
    unsigned char *terrain;

    if (!player)
        terrain = LAND_NEAR_TOEJAM[0];
    else
        terrain = LAND_NEAR_EARL[0];
    if (terrain[7] >= 0x50 || terrain[11] >= 0x50 ||
        terrain[13] >= 0x50 || terrain[17] >= 0x50)
        return 1;
    else
        return 0;
}

/* Process marked land cells beneath grounded players, then re-evaluate falling
 * after the path changes. The shared screen uses bank zero for both players. */
void ProcessLand()
{
    short row, column;
    struct object_state *player;

    if (gPlayerLives[0] >= 0 && gPlayerInfo[0].height <= 4 &&
        gPlayerInfo[0].level > 1 && !ELEVATOR_FLAG[0]) {
        row = (gPlayerInfo[0].y >> 6) - 1;
        column = (gPlayerInfo[0].x >> 7) - 1;
        if (row >= 0 && column >= 0 && row < 26 && column < 19 &&
            (gMapDataA[row][column] & (unsigned long)0x4000) && IsToejamNearEdge(0)) {
            func_01f4f8(0, row, column);
            func_018f1e(0, gLevel912C);
            player = &gPlayerInfo[0];
            UpdateLandAroundPlayer(0);
            if (LAND_UNDER_TOEJAM == 0x90 && !gPlayerInfo[0].height &&
#if REVISION >= 2
                player->sprite_state != 78 && player->sprite_state != 79 &&
#endif
                !(player->flags_52 & 0x8020)) {
                StopPlayer(player);
                SetToejamFalling(player, 0);
            }
        }
    }
    if (gPlayerLives[1] >= 0 && gPlayerInfo[1].height <= 4 &&
        gPlayerInfo[1].level > 1 && !ELEVATOR_FLAG[1]) {
        row = (gPlayerInfo[1].y >> 6) - 1;
        column = (gPlayerInfo[1].x >> 7) - 1;
        if (row >= 0 && column >= 0 && row < 26 && column < 19) {
            if (SPLIT_SCREEN) {
                if ((gMapDataB[row][column] & (unsigned long)0x4000) && IsToejamNearEdge(1)) {
                    func_01f4f8(1, row, column);
                    func_018f1e(1, gLevel912E);
                    player = &gPlayerInfo[1];
                    UpdateLandAroundPlayer(1);
                    if (LAND_UNDER_EARL == 0x90 && !gPlayerInfo[1].height &&
#if REVISION >= 2
                        player->sprite_state != 78 && player->sprite_state != 79 &&
#endif
                        !(gPlayerInfo[1].flags_52 & 0x8020)) {
                        StopPlayer(player);
                        SetToejamFalling(player, 1);
                    }
                }
            } else {
                if ((gMapDataA[row][column] & (unsigned long)0x4000) && IsToejamNearEdge(1)) {
                    func_01f4f8(0, row, column);
                    func_018f1e(0, gLevel912C);
                    player = &gPlayerInfo[1];
                    UpdateLandAroundPlayer(1);
                    if (LAND_UNDER_EARL == 0x90 && !gPlayerInfo[1].height &&
#if REVISION >= 2
                        player->sprite_state != 78 && player->sprite_state != 79 &&
#endif
                        !(player->flags_52 & 0x8020)) {
                        StopPlayer(player);
                        SetToejamFalling(player, 1);
                    }
                }
            }
        }
    }
}
