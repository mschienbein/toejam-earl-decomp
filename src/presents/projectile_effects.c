#include "player_animation.h"
#include "present_types.h"
#include "terrain.h"
#include "psg_effects.h"
#include "fm_effects.h"
#include "hardware.h"
#define TJE_LOCAL_GETMAPTERRAINATWORLDPOSITION
#define TJE_LOCAL_PRESENTREGISTERDECOYHIT

#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "assets_audio.h"
#include "assets_earthlings.h"
#include "assets_map.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "collision_calls.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "sprite_calls.h"

/* MATCHING QUALIFICATION: explicit (int) result views of AllocateEntity()
 * and AllocateEntityWithEviction() retain the observed call-result widths.
 * Shared short returns stay canonical; historical spelling is unknown [INFERENCE]. */

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared unsigned char declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int GetMapTerrainAtWorldPosition();

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * MATCHING QUALIFICATION: both the shared short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int PresentRegisterDecoyHit();
/* Launch a tomato from an earthling toward a velocity-adjusted target.
 * The distance estimate controls the vertical acceleration. */
void EarthlingLaunchAimedProjectile(target, earthling)
struct object_state *target;
struct earthling_state *earthling;
{
    short slot, distance_x, distance_y, speed_x, speed_y, direction;
    struct object_state *projectile;
    struct animation_descriptor *animation;

    slot = 4;
    do {
        if (!gSharedProjectileEntities[slot])
            break;
        ++slot;
    } while (slot < 12);
    if (slot == 12)
        return;
    if (gSharedProjectileEntities[slot] = (int)AllocateEntity()) {
        distance_x = target->x - earthling->x;
        distance_y = target->y - earthling->y;
        distance_x += target->velocity_x >> 3;
        distance_y += target->velocity_y >> 4;
        if (!distance_x) {
            speed_x = 0;
            speed_y = 800;
        } else {
            speed_x = abs(distance_y * 100 / distance_x);
            direction = 0;
            do {
                if (speed_x < EarthlingThresholds[direction])
                    break;
                ++direction;
            } while (direction < 8);
            speed_x = EarthlingVectorPairs[direction][0];
            speed_y = EarthlingVectorPairs[direction][1];
            if (distance_x < 0)
                speed_x = -speed_x;
        }
        if (distance_y < 0)
            speed_y = -speed_y;
        gSharedProjectileOwners[slot] = earthling->object;
        projectile = &gPlayerInfo[gSharedProjectileEntities[slot]];
        projectile->x = earthling->x;
        projectile->y = earthling->y;
        if (earthling->state == 8)
            projectile->height = 10;
        else if (earthling->state == 9)
            projectile->height = 15;
        else {
            projectile->height = 27;
            if (earthling->state == 10)
                projectile->x -= 10;
            else
                projectile->x += 10;
        }
        /* MATCHING QUALIFICATION: this explicit narrow expression
         * keeps speed widening in scratch D1 rather than clobbering D6.
         * The bare speed and simple scalar casts do not reproduce the copy. */
        projectile->velocity_x = GetRandomNumber() % 101 + -50 + (short)(speed_x + 0);
        projectile->velocity_y = GetRandomNumber() % 61 + -30 + speed_y;
        projectile->velocity_height = 768;
        distance_x = abs(distance_x);
        distance_y = abs(distance_y);
        if (distance_x > distance_y)
            distance_x += distance_y >> 1;
        else {
            distance_x >>= 1;
            distance_x += distance_y;
        }
        distance_x = ((distance_x - 40) >> 3) - 64;
        if (distance_x > -16)
            distance_x = -16;
        else if (distance_x < -64)
            distance_x = -64;
        projectile->acceleration_height = distance_x;
        projectile->level = earthling->level;
        projectile->width = 9;
        projectile->height_pixels = 7;
        projectile->sprite_count = 1;
        projectile->tile_count = 16;
        projectile->tile = VDP_TILE_PRIORITY;
        animation = TomatoProjectileAnimations;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 0;
        projectile->draw_flags |= ENTITY_SHADOW;
        projectile->sprite_state = -1;
        if (!gPcmEffectCooldownTicks)
            PlaySoundEffect((long)Sfx_ChickenCannonFire, 40, 0);
    }
}

/* Process all twelve tomato handles, changing landed or collided tomatoes
 * into their impact animation and releasing completed objects. */
void ProcessTomatoRain()
{
    short slot, object_number;
    struct object_state *projectile;
    struct timed_animation *animation;

    for (slot = 0; slot < 12; ++slot) {
        if (object_number = gSharedProjectileEntities[slot]) {
            projectile = &gPlayerInfo[object_number];
            if (projectile->height < 10 && !(projectile->draw_flags & ENTITY_EITHER_VIEW)) {
                FreeEntity(object_number);
                gSharedProjectileEntities[slot] = 0;
            } else if (projectile->sprite_state == -1) {
                if (projectile->height <= 0) {
                    if (projectile->tile & VDP_TILE_PRIORITY) {
                        projectile->draw_flags &= ~ENTITY_SHADOW;
                        object_number = 0;
                        if (projectile->draw_flags & ENTITY_SECONDARY_VIEW)
                            object_number = 1;
                        object_number = GetMapTerrainAtWorldPosition(object_number, projectile->x, projectile->y);
                        if (object_number == 144) {
                            projectile->tile &= ~VDP_TILE_PRIORITY;
                            projectile->sprite_ready = 0;
                            projectile->sprite_state = 21;
                        } else {
                            projectile->velocity_x = 0;
                            projectile->velocity_y = 0;
                            projectile->height = 0;
                            projectile->velocity_height = 0;
                            projectile->acceleration_height = 0;
                            if (object_number != 128)
                                animation = &TomatoGroundImpactAnimation;
                            else
                                animation = &TomatoTerrain128ImpactAnimation;
                            projectile->sequence = animation->sequence;
                            projectile->frames = animation->frames;
                            projectile->frame_phase = animation->period;
                            projectile->sprite_state = 0;
                            projectile->frame = -1;
                            projectile->frame_duration = 1;
                            if (!gFmEffectTemplateStatus)
                                AudioRequestChannelTemplateEffect(FM_EFFECT_TOMATO_LAND);
                        }
                    }
                } else if (projectile->height <= 25) {
                    object_number = ProjectileFindCollisionExcludingOwner(projectile, gSharedProjectileOwners[slot]);
                    if (object_number) {
                        if (--object_number <= 1 && !gPlayerElevatorFlags[object_number] &&
                            !gPlayerInventoryOpen[object_number] && !gPlayerInteractionChoiceActive[object_number]) {
                            if (gSharedProjectileOwners[slot] <= 1)
                                gPlayerHealth[object_number] -= 4;
                            else
                                gPlayerHealth[object_number] -= 6;
                        } else if (!PresentRegisterDecoyHit(object_number))
                            DamageEarthling(gPlayerInfo[object_number].earthling);
                        projectile->velocity_x = 0;
                        projectile->velocity_y = 0;
                        projectile->velocity_height = 0;
                        projectile->acceleration_height = 0;
                        projectile->draw_flags &= ~ENTITY_SHADOW;
                        animation = &TomatoTargetImpactAnimation;
                        projectile->sequence = animation->sequence;
                        projectile->frames = animation->frames;
                        projectile->frame_phase = animation->period;
                        projectile->frame = -1;
                        projectile->frame_duration = 1;
                        projectile->sprite_state = 0;
                        AudioRequestChannelTemplateEffect(FM_EFFECT_TOMATO_HIT);
                    }
                }
            } else if (!projectile->frame_duration && projectile->sprite_state != 21) {
                FreeEntity(object_number);
                gSharedProjectileEntities[slot] = 0;
            }
        }
    }
    EarthlingUpdateControlRemapProjectiles();
}

/* Choose a counter in [50, 80] and clear its companion byte. */
void SetTomatoRainTomatoes(index)
int index;
{
    gPlayerTomatoRainTicks[index] = GetRandomNumber() % 31 + 50;
    gPlayerTomatoRainSpawnDelay[index] = 0;
}

/* While a player's rain counter is active, allocate a falling tomato after
 * the inter-tomato delay expires. Allocation failure leaves the count intact. */
void CheckTomatoRainTimer(player_number)
int player_number;
{
    short slot;
    struct object_state *projectile, *player;
    struct animation_descriptor *animation;

    if (!gPlayerTomatoRainTicks[player_number] || --gPlayerTomatoRainSpawnDelay[player_number] > 0)
        return;
    gPlayerTomatoRainSpawnDelay[player_number] = GetRandomNumber() % 5 + 4;
    slot = 4;
    do {
        if (!gSharedProjectileEntities[slot])
            break;
        ++slot;
    } while (slot < 12);
    if (slot == 12)
        return;
    if (gSharedProjectileEntities[slot] = (int)AllocateEntity()) {
        gSharedProjectileOwners[slot] = -1;
        player = &gPlayerInfo[player_number];
        projectile = &gPlayerInfo[gSharedProjectileEntities[slot]];
        projectile->x = GetRandomNumber() % 201 + -100 + player->x;
        projectile->y = GetRandomNumber() % 101 + -50 + player->y;
        projectile->height = 224;
        projectile->velocity_height = -384;
        projectile->acceleration_height = -18;
        projectile->level = player->level;
        projectile->width = 9;
        projectile->height_pixels = 7;
        projectile->sprite_count = 1;
        projectile->tile_count = 16;
        projectile->tile = VDP_TILE_PRIORITY;
        animation = TomatoProjectileAnimations;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 0;
        projectile->draw_flags |= ENTITY_SHADOW;
        projectile->sprite_state = -1;
        --gPlayerTomatoRainTicks[player_number];
    }
}

/* Release the first occupied rain/earthling tomato handle, if one exists. */
int PresentReleaseFirstSharedTomato()
{
    short slot;

    for (slot = 4; slot < 12; ++slot) {
        if (gSharedProjectileEntities[slot]) {
            FreeEntity(gSharedProjectileEntities[slot]);
            gSharedProjectileEntities[slot] = 0;
            return 1;
        }
    }
    return 0;
}

/* Allocate one of four projectiles and launch it left or right from the earthling. */
void EarthlingLaunchControlRemapProjectile(earthling, source)
struct earthling_state *earthling;
struct object_state *source;
{
    short slot;
    struct object_state *projectile;
    struct animation_descriptor *animation;

    slot = 0;
    do {
        if (!gControlRemapProjectileEntities[slot])
            break;
        ++slot;
    } while (slot < 4);
    if (slot == 4)
        return;
    if (gControlRemapProjectileEntities[slot] = (int)AllocateEntity()) {
        projectile = &gPlayerInfo[gControlRemapProjectileEntities[slot]];
        projectile->x = source->x;
        projectile->y = source->y - 5;
        projectile->height = source->height;
        if (earthling->state == 10) {
            projectile->x -= 22;
            projectile->velocity_x = -896;
            animation = &ControlRemapProjectileLeftAnimation;
        } else {
            projectile->x += 22;
            projectile->velocity_x = 896;
            animation = &ControlRemapProjectileRightAnimation;
        }
        projectile->acceleration_height = -32;
        projectile->level = source->level;
        projectile->width = 16;
        projectile->height_pixels = 8;
        projectile->sprite_count = 1;
        projectile->tile_count = 2;
        projectile->tile = VDP_TILE_PRIORITY;
        projectile->sequence = animation->sequence;
        projectile->frames = animation->frames;
        projectile->frame_phase = 9;
        projectile->frame = -1;
        projectile->frame_duration = 1;
        projectile->draw_flags |= ENTITY_SHADOW;
        projectile->sprite_state = -1;
        AudioRequestChannelTemplateEffect(FM_EFFECT_CONTROL_REMAP_SHOT);
    }
}

/* Return one plus the first active player's or decoy's object index in range. */
short EntityFindProjectileTargetHit(projectile)
struct object_state *projectile;
{
    short pass, index, reach;
    struct object_state *target;

    target = gPlayerInfo;
    for (pass = 0; pass < 2; ++pass) {
        for (index = 0; index <= 1; ++index, ++target) {
            if (pass) {
                if (!gPlayerDecoyEntities[index])
                    continue;
                target = &gPlayerInfo[gPlayerDecoyEntities[index]];
            }
            if (!(target->draw_flags & ENTITY_ACTIVE) || !(target->behavior_flags & ENTITY_COLLIDABLE) ||
                projectile->level != target->level)
                continue;
            reach = projectile->width / 2 + target->width / 2 - 5;
            if (reach > abs(projectile->x - target->x) &&
                abs(projectile->y - target->y) < reach * 3 / 4 &&
                target->height + target->height_pixels - 10 > projectile->height) {
                if (!pass)
                    return index + 1;
                else
                    return gPlayerDecoyEntities[index] + 1;
            }
        }
    }
    return 0;
}

/* Allocate the player's control-effect object and refresh its timer and mapping. */
void PlayerApplyControlRemapEffect(player_number)
short player_number;
{
    struct object_state *effect, *player;
    struct timed_animation *animation;

    if (!gPlayerControlRemapEntities[player_number]) {
        if (gPlayerControlRemapEntities[player_number] = (int)AllocateEntityWithEviction(0)) {
            effect = &gPlayerInfo[gPlayerControlRemapEntities[player_number]];
            player = &gPlayerInfo[player_number];
            effect->behavior_flags |= ENTITY_ATTACHED;
            effect->owner = player_number;
            effect->level = player->level;
            effect->tile |= VDP_TILE_PRIORITY;
            effect->width = 32;
            effect->height_pixels = 32;
            effect->sprite_count = 1;
            effect->tile_count = 16;
            animation = PlayerControlRemapAnimations;
            effect->sequence = animation->sequence;
            effect->frames = animation->frames;
            effect->frame_phase = animation->period;
            effect->frame = -1;
            effect->frame_duration = 1;
        }
    }
    if (gPlayerControlRemapEntities[player_number]) {
        gPlayerControlRemapTimers[player_number] = 240;
        gPlayerControlRemapIndices[player_number] = GetRandomNumber() & 3;
    }
}

/* Keep action bits and remap a nonzero direction nibble through the selected row.
 * Callers use the shared unsigned-char return declaration. */
unsigned char PlayerRemapDirectionalInput(player_number)
short player_number;
{
    unsigned char input, direction;

    input = gPlayerProcessedInput[player_number];
    if (!(input & INPUT_DIRECTION_MASK))
        return input;
    direction = ControlRemapDirections[gPlayerControlRemapIndices[player_number]][(input & INPUT_DIRECTION_MASK) - 1];
    return (input & INPUT_BUTTON_MASK) | direction;
}

/* Expire the control-effect object, or occasionally play its animation sound. */
void PlayerUpdateControlRemapEffect(player)
struct object_state *player;
{
    short player_number;
    struct object_state *effect;

    player_number = player->entity_index;
    if (!gPlayerControlRemapEntities[player_number])
        return;
    effect = &gPlayerInfo[gPlayerControlRemapEntities[player_number]];
    effect->level = player->level;
    if (--gPlayerControlRemapTimers[player_number] <= 0 || gPlayerLives[player_number] < 0 ||
        player->sprite_state == PLAYER_ANIM_GHOST || player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST) {
        FreeEntity(gPlayerControlRemapEntities[player_number]);
        gPlayerControlRemapEntities[player_number] = 0;
    } else if (!gPlayerElevatorFlags[player_number] && (effect->frame_duration & 2) &&
               ((short)GetRandomNumber() & 1)) {
        PlaySound(PSG_DOUBLE_TONE_SWEEP, 20);
    }
}

/* Update four projectiles, applying player/decoy hits or removing landed objects. */
void EarthlingUpdateControlRemapProjectiles()
{
    short slot, object_number, hit;
    struct object_state *projectile;

    for (slot = 0; slot < 4; ++slot) {
        if (object_number = gControlRemapProjectileEntities[slot]) {
            projectile = &gPlayerInfo[object_number];
            if (!(projectile->draw_flags & ENTITY_EITHER_VIEW)) {
                if (projectile->height < 5) {
                    FreeEntity(object_number);
                    gControlRemapProjectileEntities[slot] = 0;
                }
            } else if (projectile->sprite_state == -1) {
                if (projectile->height <= 0) {
                    if (projectile->tile & VDP_TILE_PRIORITY) {
                        projectile->draw_flags &= ~ENTITY_SHADOW;
                        hit = 0;
                        if (projectile->draw_flags & ENTITY_SECONDARY_VIEW)
                            hit = 1;
                        hit = GetMapTerrainAtWorldPosition(hit, projectile->x, projectile->y);
                        if (hit == 0x90) {
                            projectile->tile &= ~0x8000;
                            projectile->sprite_ready = 0;
                        } else {
                            FreeEntity(object_number);
                            gControlRemapProjectileEntities[slot] = 0;
                        }
                    }
                } else if (projectile->height <= 25) {
                    if (hit = EntityFindProjectileTargetHit(projectile)) {
                        if (--hit <= 1 && !gPlayerElevatorFlags[hit] && !gPlayerInventoryOpen[hit])
                            PlayerApplyControlRemapEffect(hit);
                        else
                            PresentRegisterDecoyHit(hit);
                        FreeEntity(object_number);
                        gControlRemapProjectileEntities[slot] = 0;
                    }
                }
            }
        }
    }
}

/* Place an active terrain effect one pixel behind the supplied player. */
void PresentCreateActiveTerrainEffect(player, player_number)
struct object_state *player;
short player_number;
{
    char object_number;
    struct object_state *effect;
    struct timed_animation *animation;

    if (object_number = (int)AllocateEntityWithEviction(0)) {
        gPlayerActiveTerrainEffectEntities[player_number] = object_number;
        effect = &gPlayerInfo[object_number];
        effect->x = player->x;
        effect->y = player->y - 1;
        effect->level = player->level;
        effect->tile |= VDP_TILE_PRIORITY;
        effect->width = 24;
        effect->height_pixels = 20;
        effect->sprite_count = 1;
        effect->tile_count = 16;
        animation = ActiveTerrainEffectAnimations;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame = -1;
        effect->frame_duration = 1;
        effect->behavior_flags |= ENTITY_COLLIDABLE;
        AudioRequestChannelTemplateEffect(FM_EFFECT_TERRAIN_GROW);
    }
}

/* Replace the previous retiring effect and switch the current one to its final animation. */
void PresentRetireTerrainEffect(player_number)
short player_number;
{
    char object_number;
    struct object_state *effect;
    struct timed_animation *animation;

    if (object_number = gPlayerOneShotTerrainEffectEntities[player_number])
        FreeEntity(object_number);
    object_number = gPlayerActiveTerrainEffectEntities[player_number];
    gPlayerOneShotTerrainEffectEntities[player_number] = object_number;
    gPlayerActiveTerrainEffectEntities[player_number] = 0;
    effect = &gPlayerInfo[object_number];
    animation = &RetiringTerrainEffectAnimation;
    effect->sequence = animation->sequence;
    effect->frames = animation->frames;
    effect->frame_phase = animation->period;
    effect->frame = -1;
    effect->frame_duration = 1;
}

/* Allocate a temporary terrain-effect object at the player position. */
void PresentCreateTemporaryTerrainEffect(player, player_number)
struct object_state *player;
short player_number;
{
    char object_number;
    struct object_state *effect;
    struct timed_animation *animation;

    if (object_number = (int)AllocateEntity()) {
        gPlayerOneShotTerrainEffectEntities[player_number] = object_number;
        effect = &gPlayerInfo[object_number];
        effect->x = player->x;
        effect->y = player->y;
        effect->level = player->level;
        effect->tile |= VDP_TILE_PRIORITY;
        effect->width = 24;
        effect->height_pixels = 20;
        effect->sprite_count = 1;
        effect->tile_count = 16;
        animation = &TemporaryTerrainEffectAnimation;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame = -1;
        effect->frame_duration = 1;
        effect->behavior_flags |= ENTITY_COLLIDABLE;
        AudioRequestChannelTemplateEffect(FM_EFFECT_TERRAIN_GROW);
    }
}

/* Spawn terrain effects on a fresh action press and apply nearby effect damage. */
void PresentUpdateTerrainEffects(player, terrain)
struct object_state *player;
unsigned short terrain;
{
    short player_number;
    char object_number;
    struct object_state *effect;
    struct earthling_state source;

    player_number = player->entity_index;
    if (gPlayerPowerups[player_number] != PRESENT_ROSE_BUSHES || gPlayerElevatorFlags[player_number] ||
        player->sprite_state == PLAYER_ANIM_GHOST || player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST) {
        if (gPlayerActiveTerrainEffectEntities[player_number])
            PresentRetireTerrainEffect(player_number);
    } else if (!(gPlayerPreviousProcessedInput[player_number] & INPUT_BUTTON_A) &&
               (gPlayerProcessedInput[player_number] & INPUT_BUTTON_A)) {
        if (terrain == TERRAIN_GROUND || terrain == TERRAIN_WALKABLE_MAX) {
            if (gPlayerActiveTerrainEffectEntities[player_number])
                PresentRetireTerrainEffect(player_number);
            PresentCreateActiveTerrainEffect(player, player_number);
        } else if (terrain < TERRAIN_WATER && !gPlayerOneShotTerrainEffectEntities[player_number]) {
            PresentCreateTemporaryTerrainEffect(player, player_number);
        }
    } else if (object_number = gPlayerActiveTerrainEffectEntities[player_number]) {
        effect = &gPlayerInfo[object_number];
        if (!effect->frame_duration && (effect->draw_flags & ENTITY_EITHER_VIEW)) {
            source.kind = 1;
            source.x = effect->x;
            source.y = effect->y;
            source.object = object_number;
            if (!gPlayerElevatorFlags[player_number] && EntityIsWithinRange(effect, player, 25, 10))
                Player_Youch(player_number, &source);
            if (!gPlayerElevatorFlags[player_number ^ 1] &&
                EntityIsWithinRange(effect, &gPlayerInfo[player_number ^ 1], 25, 10))
                Player_Youch(player_number ^ 1, &source);
        }
    }
    if (object_number = gPlayerOneShotTerrainEffectEntities[player_number]) {
        effect = &gPlayerInfo[object_number];
        if (!effect->frame_duration) {
            FreeEntity(object_number);
            gPlayerOneShotTerrainEffectEntities[player_number] = 0;
        }
    }
}

/* Shoot at frame two; on animation completion restore the matching facing state. */
void EarthlingUpdateControlRemapAttack(earthling, object)
struct earthling_state *earthling;
struct object_state *object;
{
    short next_state;

    if (object->frame == 2) {
        ++object->frame;
        EarthlingLaunchControlRemapProjectile(earthling, object);
    } else if (!object->frame_duration) {
        if (earthling->state == 10)
            next_state = 0;
        else
            next_state = 1;
        SetEarthlingState(earthling, next_state);
    }
}

/* During the early firing frames, pick an eligible player or active decoy ahead. */
void EarthlingUpdateAimedProjectileAttack(earthling, object)
struct earthling_state *earthling;
struct object_state *object;
{
    register short player_number;
    short decoy, attempt, distance;
    struct object_state *target;

    if (object->frame < 16 && (object->frame & 3) == 2) {
        ++object->frame;
        player_number = GetRandomNumber() & 1;
        for (attempt = 0; attempt < 2; player_number ^= 1, ++attempt) {
            decoy = gPlayerDecoyEntities[player_number];
            target = decoy && (gPlayerInfo[decoy].draw_flags & ENTITY_EITHER_VIEW) ?
                     &gPlayerInfo[decoy] : &gPlayerInfo[player_number];
            if (gPlayerLives[player_number] < 0)
                continue;
            if (earthling->state == 8) {
                if ((distance = earthling->y - target->y) > 40 && distance < 200)
                    break;
            } else if (earthling->state == 9) {
                if ((distance = target->y - earthling->y) > 40 && distance < 200)
                    break;
            } else if (earthling->state == 10) {
                if ((distance = earthling->x - target->x) > 50 && distance < 320)
                    break;
            } else {
                if ((distance = target->x - earthling->x) > 50 && distance < 320)
                    break;
            }
        }
        if (attempt != 2)
            EarthlingLaunchAimedProjectile(target, earthling);
    } else if (!object->frame_duration) {
        SetEarthlingState(earthling, 0);
        PlaySoundEffect((long)Sfx_ChickenCluck, AUDIO_PRIORITY_NORMAL, 0);
    }
}
