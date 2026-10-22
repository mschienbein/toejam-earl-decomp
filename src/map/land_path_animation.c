#include "entities.h"
#include "sprites.h"
#include "revision.h"
#include "assets_sprites.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "ram_camera.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "sprite_calls.h"

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
