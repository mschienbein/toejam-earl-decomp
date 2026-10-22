#include "entities.h"
#include "sprites.h"
#include "assets_scenes.h"
#include "engine_calls.h"
#include "graphics_calls.h"
#include "player.h"
#include "ram_camera.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "speech.h"
#include "sprite_calls.h"

/* Create an object from one placement and one scene descriptor. */
void func_023086(placement_number, descriptor_number)
short placement_number, descriptor_number;
{
    short object_number;
    struct object_state *object;
    struct scene_descriptor *descriptor;

    if (object_number = func_00d904()) {
        ram_ffe264[placement_number] = object_number;
        object = &gPlayerInfo[object_number];
        descriptor = &FunkatronSceneDescriptors[descriptor_number];
        object->frames = descriptor->frames;
        object->sequence = descriptor->idle_sequence;
        object->frame_phase = descriptor->frame_phase;
        object->width = descriptor->width;
        object->height_pixels = descriptor->height;
        object->sprite_count = descriptor->sprite_count;
        object->tile_count = descriptor->tile_count;
        object->frame_duration = 1;
        object->frame = -1;
        object->x = FunkatronScenePlacements[placement_number].x;
        object->y = FunkatronScenePlacements[placement_number].y;
        object->level = 26;
        object->draw_flags |= 8;
        if (descriptor_number == 6)
            object->draw_flags |= 128;
        else if (descriptor_number != 13)
            object->flags_4d |= 1;
    }
}

/* Zero-x placement records delimit descriptor groups. Each call visits one
 * quarter of the groups, creates visible objects, and handles proximity-based
 * animation and interactions with the first two player records. */
void func_023154()
{
    short descriptor_number = 0;
    short placement_number, screen_x, maximum_y, half_width;
    short screen_y;
    short object_number, player_number, interaction;
    struct scene_descriptor *descriptor;
    struct object_state *object;

    descriptor = FunkatronSceneDescriptors;
    for (placement_number = 0; descriptor_number < 30; ++placement_number) {
        if (!FunkatronScenePlacements[placement_number].x) {
            ++descriptor_number;
            ++descriptor;
            continue;
        }
        if (ram_ffe390 != (descriptor_number & 3))
            continue;
        if (descriptor_number == 28 && PLAYER_INDICATOR != 1)
            continue;
        if (descriptor_number == 29 && PLAYER_INDICATOR != 0)
            continue;
        half_width = descriptor->width / 2;
        half_width += 5;
        maximum_y = descriptor->height;
        maximum_y += 224;
        screen_x = FunkatronScenePlacements[placement_number].x;
        screen_x -= TOEJAM_CAMERA_NEWX;
        screen_y = FunkatronScenePlacements[placement_number].y;
        screen_y -= TOEJAM_CAMERA_NEWY;
        object_number = ram_ffe264[placement_number];
        if (screen_x + half_width > 0 && screen_x < half_width + 320 &&
            screen_y + 5 > 0 && screen_y < maximum_y) {
            if (!object_number) {
                func_023086(placement_number, descriptor_number);
                object_number = ram_ffe264[placement_number];
            }
            if (descriptor_number >= 15 && object_number) {
                object = &gPlayerInfo[object_number];
                if (func_00dd1e(object, &gPlayerInfo[0], 80, 50) ||
                    func_00dd1e(object, &gPlayerInfo[1], 80, 50)) {
                    if (!object->sprite_state) {
                        object->sequence = descriptor->interaction_sequence;
                        object->frame = -1;
                        object->sprite_state = 1;
                    }
                    player_number = 0;
                    if (!object->flag_57 &&
                        (interaction = FunkatronScenePlacements[placement_number].interaction)) {
                        if (func_00dd1e(object, &gPlayerInfo[0], 30, 20) ||
                            (func_00dd1e(object, &gPlayerInfo[1], 30, 20) &&
                             (player_number = 1))) {
                            if (!((interaction == 31 && player_number == 0) ||
                                  (interaction == 53 && player_number == 0) ||
                                  (interaction == 54 && player_number == 1))) {
                                if (func_01e38e(interaction, object->entity_number) >= 0)
                                    object->flag_57 = 1;
                            }
                            if ((interaction == 31 && player_number == 1) ||
                                (interaction == 54 && player_number == 0))
                                func_01da0a();
                        }
                    }
                } else if (object->sprite_state) {
                    object->sequence = descriptor->idle_sequence;
                    object->frame = -1;
                    object->sprite_state = 0;
                }
            }
        } else if (object_number) {
            func_00d970(object_number);
            ram_ffe264[placement_number] = 0;
        }
    }
    ram_ffe390 = (ram_ffe390 + 1) & 3;
}
