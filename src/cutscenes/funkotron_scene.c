#include "entities.h"
#include "terrain.h"
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
void FunkatronCreateSceneEntity(placement_number, descriptor_number)
short placement_number, descriptor_number;
{
    short object_number;
    struct object_state *object;
    struct scene_descriptor *descriptor;

    if (object_number = AllocateEntity()) {
        gFunkatronPlacementEntities[placement_number] = object_number;
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
        object->frame = SPRITE_FRAME_BEFORE_FIRST;
        object->x = FunkatronScenePlacements[placement_number].x;
        object->y = FunkatronScenePlacements[placement_number].y;
        object->level = LEVEL_FINAL;
        object->draw_flags |= ENTITY_SKIP_PHYSICS;
        if (descriptor_number == 6)
            object->draw_flags |= ENTITY_DRAW_LATE;
        else if (descriptor_number != 13)
            object->behavior_flags |= ENTITY_COLLIDABLE;
    }
}

/* Zero-x placement records delimit descriptor groups. Each call visits one
 * quarter of the groups, creates visible objects, and handles proximity-based
 * animation and interactions with the first two player records. */
void FunkatronUpdateSceneEntities()
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
        if (gFunkatronUpdatePhase != (descriptor_number & 3))
            continue;
        if (descriptor_number == 28 && gPlayerSelection != PLAYER_EARL)
            continue;
        if (descriptor_number == 29 && gPlayerSelection != PLAYER_TOEJAM)
            continue;
        half_width = descriptor->width / 2;
        half_width += 5;
        maximum_y = descriptor->height;
        maximum_y += 224;
        screen_x = FunkatronScenePlacements[placement_number].x;
        screen_x -= gCamera1PendingX;
        screen_y = FunkatronScenePlacements[placement_number].y;
        screen_y -= gCamera1PendingY;
        object_number = gFunkatronPlacementEntities[placement_number];
        if (screen_x + half_width > 0 && screen_x < half_width + 320 &&
            screen_y + 5 > 0 && screen_y < maximum_y) {
            if (!object_number) {
                FunkatronCreateSceneEntity(placement_number, descriptor_number);
                object_number = gFunkatronPlacementEntities[placement_number];
            }
            if (descriptor_number >= 15 && object_number) {
                object = &gPlayerInfo[object_number];
                if (EntityIsWithinRange(object, &gPlayerInfo[PLAYER_TOEJAM], 80, 50) ||
                    EntityIsWithinRange(object, &gPlayerInfo[PLAYER_EARL], 80, 50)) {
                    if (!object->sprite_state) {
                        object->sequence = descriptor->interaction_sequence;
                        object->frame = SPRITE_FRAME_BEFORE_FIRST;
                        object->sprite_state = FUNKATRON_SPRITE_INTERACTION;
                    }
                    player_number = PLAYER_TOEJAM;
                    if (!object->attachment_x_or_latch &&
                        (interaction = FunkatronScenePlacements[placement_number].interaction)) {
                        if (EntityIsWithinRange(object, &gPlayerInfo[PLAYER_TOEJAM], 30, 20) ||
                            (EntityIsWithinRange(object, &gPlayerInfo[PLAYER_EARL], 30, 20) &&
                             (player_number = PLAYER_EARL))) {
                            if (!((interaction == 31 && player_number == PLAYER_TOEJAM) ||
                                  (interaction == 53 && player_number == PLAYER_TOEJAM) ||
                                  (interaction == 54 && player_number == PLAYER_EARL))) {
                                if (StartDialogue(interaction, object->entity_index) >= 0)
                                    object->attachment_x_or_latch = 1;
                            }
                            if ((interaction == 31 && player_number == PLAYER_EARL) ||
                                (interaction == 54 && player_number == PLAYER_TOEJAM))
                                DrawPresentDisplayPanel();
                        }
                    }
                } else if (object->sprite_state) {
                    object->sequence = descriptor->idle_sequence;
                    object->frame = SPRITE_FRAME_BEFORE_FIRST;
                    object->sprite_state = FUNKATRON_SPRITE_IDLE;
                }
            }
        } else if (object_number) {
            FreeEntity(object_number);
            gFunkatronPlacementEntities[placement_number] = 0;
        }
    }
    gFunkatronUpdatePhase = (gFunkatronUpdatePhase + 1) & 3;
}
