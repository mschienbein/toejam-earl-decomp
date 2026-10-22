#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "psg_effects.h"
#include "fm_effects.h"
#include "music_ids.h"
#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "revision.h"
#include "assets_audio.h"
#include "assets_sprites.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "collision_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Create or restart the player's short-lived effect at the given position. */
int PresentStartTransientEffect(player_number, x, y, height, level)
short player_number, x, y, height;
char level;
{
    struct object_state *effect;
    struct timed_animation *animation;

    if (!gPlayerTransientEffectEntities[player_number])
        gPlayerTransientEffectEntities[player_number] = (int)AllocateEntity();
    if (gPlayerTransientEffectEntities[player_number]) {
        effect = &gPlayerInfo[gPlayerTransientEffectEntities[player_number]];
        effect->x = x;
        effect->y = y;
        effect->height = height;
        effect->level = level;
        effect->width = 32;
        effect->height_pixels = 32;
        effect->sprite_count = 1;
        effect->tile_count = 16;
        effect->tile |= VDP_TILE_PRIORITY;
        animation = &TransientEffectAnimation;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame_duration = 1;
        effect->frame = -1;
        PlaySound(PSG_SHORT_NOISE_FADE_RATE2, AUDIO_PRIORITY_NORMAL);
        return 0;
    }
    return 1;
}

/* Place a decoy just behind the player, moving right until terrain permits it. */
void OpenDecoy(player_number)
short player_number;
{
    struct object_state *decoy, *player;
    struct timed_animation *animation;

    if (!gPlayerDecoyEntities[player_number])
        gPlayerDecoyEntities[player_number] = (int)AllocateEntityWithEviction(0);
    if (gPlayerDecoyEntities[player_number]) {
        decoy = &gPlayerInfo[gPlayerDecoyEntities[player_number]];
        player = &gPlayerInfo[player_number];
        decoy->x = player->x;
        decoy->y = player->y + 1;
        decoy->level = player->level;
        while (ElevatorContainsPosition(decoy->x, decoy->y, decoy->level))
            decoy->x += 8;
        decoy->width = 16;
        decoy->height_pixels = 32;
        decoy->sprite_count = 2;
        decoy->tile_count = 24;
        decoy->tile |= VDP_TILE_PRIORITY;
        animation = &PlayerDecoyAnimations[player_number];
        decoy->sequence = animation->sequence;
        decoy->frames = animation->frames;
        decoy->frame_phase = animation->period;
        decoy->frame_duration = 1;
        decoy->behavior_flags |= ENTITY_COLLIDABLE;
        gPlayerDecoyHitCounts[player_number] = 0;
        if (!PresentStartTransientEffect(player_number, decoy->x, decoy->y + 1, 5, decoy->level))
            decoy->behavior_flags |= ENTITY_HIDDEN;
    }
}

/* Release an expired effect; return zero while its first three frames play. */
int PresentUpdateTransientEffect(player_number)
int player_number;
{
    char object_number;
    struct object_state *effect;

    if (object_number = gPlayerTransientEffectEntities[player_number]) {
        effect = &gPlayerInfo[object_number];
        if (!effect->frame_duration) {
            FreeEntity(object_number);
            gPlayerTransientEffectEntities[player_number] = 0;
        } else if (effect->frame <= 2)
            return 0;
    }
    return 1;
}

/* Count a hit against whichever player's active decoy owns this object. */
short PresentRegisterDecoyHit(object_number)
short object_number;
{
    short player_number;

    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
        if (gPlayerDecoyEntities[player_number] == object_number) {
            ++gPlayerDecoyHitCounts[player_number];
            return 1;
        }
    }
    return 0;
}

/* Reveal a decoy after its effect, and remove it on level change or three hits. */
void PresentUpdateDecoy(player_number)
short player_number;
{
    char object_number;
    struct object_state *decoy;

    if (object_number = gPlayerDecoyEntities[player_number]) {
        if (!PresentUpdateTransientEffect(player_number))
            return;
        decoy = &gPlayerInfo[object_number];
        decoy->behavior_flags &= ~ENTITY_HIDDEN;
        if (gPlayerInfo[player_number].level != decoy->level) {
            FreeEntity(object_number);
            gPlayerDecoyEntities[player_number] = 0;
        }
        if (gPlayerDecoyHitCounts[player_number] >= 3) {
            FreeEntity(object_number);
            EntitySpawnTimedEffect(decoy->x, decoy->y, decoy->level);
            gPlayerDecoyEntities[player_number] = 0;
        }
    }
}

/* Place a doorway to either side, aligned to an eight-pixel grid. */
void PresentIsDoorway(player_number)
short player_number;
{
    struct object_state *doorway, *player;
    struct timed_animation *animation;

    if (!gPlayerDoorwayEntities[player_number])
        gPlayerDoorwayEntities[player_number] = (int)AllocateEntityWithEviction(0);
    if (gPlayerDoorwayEntities[player_number]) {
        doorway = &gPlayerInfo[gPlayerDoorwayEntities[player_number]];
        player = &gPlayerInfo[player_number];
        doorway->x = player->x & 0xfff8;
        if (GetRandomNumber() & 1)
            doorway->x -= 32;
        else
            doorway->x += 32;
        doorway->y = player->y & 0xfff8;
        doorway->x += 16;
        doorway->y -= 14;
        doorway->level = player->level;
        while (ElevatorContainsPosition(doorway->x, doorway->y, doorway->level))
            doorway->x += 8;
        doorway->width = 24;
        doorway->height_pixels = 48;
        doorway->sprite_count = 2;
        doorway->tile_count = 28;
        doorway->tile |= VDP_TILE_PRIORITY;
        animation = &DoorwayAnimations[0];
        doorway->sequence = animation->sequence;
        doorway->frames = animation->frames;
        doorway->frame_phase = animation->period;
        doorway->frame_duration = 1;
        doorway->frame = -1;
        doorway->sprite_state = 0;
        doorway->behavior_flags |= ENTITY_COLLIDABLE;
        if (!PresentStartTransientEffect(player_number, doorway->x, doorway->y + 1, 5, doorway->level))
            doorway->behavior_flags |= ENTITY_HIDDEN;
    }
}

/* Return a one-based doorway owner when its three-by-four tile area overlaps. */
int PresentFindDoorwayOwnerAtTile(player, row, column)
struct object_state *player;
short row, column;
{
    short player_number, doorway_row, doorway_column;
    char object_number;
    struct object_state *doorway;

    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
        if (object_number = gPlayerDoorwayEntities[player_number]) {
            doorway = &gPlayerInfo[object_number];
            if (player->level == doorway->level &&
                (doorway->sprite_state || player->y >= doorway->y)) {
                doorway_row = (doorway->y - 10) >> 3;
                doorway_column = (doorway->x - 16) >> 3;
                if (row >= doorway_row && row < doorway_row + 3 &&
                    column >= doorway_column && column < doorway_column + 4)
                    return player_number + 1;
            }
        }
    }
    return 0;
}

/* Choose a terrain-safe destination and redraw if it is outside the camera. */
void PlayerTeleportThroughDoorway(player_number, player)
short player_number;
struct object_state *player;
{
    short camera_x, camera_y;

    camera_x = gCamera1PendingX;
    camera_y = gCamera1PendingY;
    if (gPlayerLives[PLAYER_TOEJAM] >= 0 && player_number == PLAYER_EARL && gSplitScreenEnabled) {
        camera_x = gCamera2PendingX;
        camera_y = gCamera2PendingY;
    }
    do {
        SetPositionOnNewLevel(player_number);
    } while (EntityFindNarrowCollisionAtPosition(player, player->x, player->y, 2));
    if (!gSplitScreenEnabled && player->x > camera_x + 20 &&
        player->x < camera_x + 320 - 20 && player->y > camera_y + 40 &&
        player->y < camera_y + 224 - 40)
        return;
    WaitForVBlankWithGraphicsPaused();
    DisableBlankingGraphics();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    UpdateScreenSplitMode(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    EnableBlankingGraphics();
}

/* Reveal the doorway, track a player aligned with it, and teleport on entry. */
void PresentUpdateDoorway(doorway_owner)
short doorway_owner;
{
    char object_number, active;
    struct object_state *object, *player;
    struct timed_animation *animation;

    if (object_number = gPlayerDoorwayEntities[doorway_owner]) {
        if (!PresentUpdateTransientEffect(doorway_owner))
            return;
        object = &gPlayerInfo[object_number];
        if (object->behavior_flags & ENTITY_HIDDEN)
            object->behavior_flags &= ~ENTITY_HIDDEN;
        if (!(object->draw_flags & ENTITY_EITHER_VIEW) && !gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayer2InventoryOpen) {
            FreeEntity(object_number);
            gPlayerDoorwayEntities[doorway_owner] = 0;
        } else {
            if (gDoorwayPlayerContactFlags[doorway_owner] & 4) {
                object_number = (gDoorwayPlayerContactFlags[doorway_owner] & 3) - 1;
                player = &gPlayerInfo[object_number];
                if (player->y < object->y) {
                    PlayerTeleportThroughDoorway(object_number, player);
                    PresentStartTransientEffect(object_number, player->x, player->y + 1,
                                5, player->level);
                }
            }
            active = 0;
            gDoorwayPlayerContactFlags[doorway_owner] = 0;
            for (object_number = 0; object_number <= 1; ++object_number) {
                player = &gPlayerInfo[object_number];
                if (PresentFindDoorwayOwnerAtTile(player, (short)((long)player->y >> 3),
                               (short)((long)player->x >> 3)) == doorway_owner + 1) {
                    gDoorwayPlayerContactFlags[doorway_owner] = object_number + 1;
                    if (player->x > object->x - 6 && player->x < object->x + 6) {
                        active = 1;
                        gDoorwayPlayerContactFlags[doorway_owner] |= 4;
                    }
                }
            }
            if (active != object->sprite_state) {
                animation = &DoorwayAnimations[active];
                object->sequence = animation->sequence;
                object->frames = animation->frames;
                object->frame_phase = animation->period;
                object->frame_duration = 1;
                object->frame = -1;
                object->sprite_state = active;
                if (active)
                    object->behavior_flags &= ~ENTITY_COLLIDABLE;
                else {
                    object->behavior_flags |= ENTITY_COLLIDABLE;
                    if (gDoorwayPlayerContactFlags[doorway_owner]) {
                        object_number = (gDoorwayPlayerContactFlags[doorway_owner] & 3) - 1;
                        player = &gPlayerInfo[object_number];
                        if ((gDoorwayPlayerContactFlags[doorway_owner] & 3) == object_number + 1 &&
                            player->y - object->y < 8)
                            player->y = object->y + 8;
                    }
                }
            }
        }
    }
}

/* Create the promotion display, or apply its immediate effect if unavailable. */
void PerformPromotion(player_number, promotion, terrain)
short player_number, promotion;
unsigned short terrain;
{
    struct object_state *display, *player;
    struct timed_animation *animation;

    if (!gPlayerPromotionDisplayEntities[player_number] &&
        (gPlayerPromotionDisplayEntities[player_number] = (int)AllocateEntityWithEviction(0))) {
        display = &gPlayerInfo[gPlayerPromotionDisplayEntities[player_number]];
        player = &gPlayerInfo[player_number];
        display->behavior_flags |= ENTITY_ATTACHED;
        display->owner = player_number;
        display->level = player->level;
        display->tile |= VDP_TILE_PRIORITY;
        display->width = 32;
        display->height_pixels = 32;
        if (promotion == PRESENT_PROMOTION) {
            animation = RankPromotionDisplayAnimation;
            display->attachment_y_or_latch = 8;
            display->sprite_count = 2;
            display->tile_count = 32;
        } else {
            display->sprite_count = 1;
            display->tile_count = 16;
            if (promotion == PRESENT_TOTAL_BUMMER)
                animation = HealthDrainDisplayAnimation;
            else
                animation = ExtraLifeDisplayAnimation;
            display->attachment_y_or_latch = 6;
            display->attachment_height = 1;
        }
        display->sequence = animation->sequence;
        display->frames = animation->frames;
        display->frame_phase = animation->period;
        display->frame = -1;
        display->frame_duration = 1;
        display->sprite_state = promotion;
        if (promotion != PRESENT_PROMOTION && terrain < TERRAIN_WATER && player->height <= 0 &&
            player->sprite_state != PLAYER_ANIM_GHOST && player->sprite_state != PLAYER_ANIM_LAST_LIFE_GHOST && player->sprite_state != PLAYER_ANIM_WHIRLWIND &&
            player->sprite_state != PLAYER_ANIM_WHIRLWIND_RECOVERY && player->sprite_state != PLAYER_ANIM_INNERTUBE_WATER_IDLE &&
            !(player->animation_group_mask & PLAYER_GROUP_BLOCK_RESOURCE_REACTION)) {
            SetSprite(player, (char)(promotion == PRESENT_EXTRA_LIFE ? PLAYER_ANIM_EXTRA_LIFE : PLAYER_ANIM_ITEM_REACTION));
            player->animation_motion_axes = ANIMATION_MOTION_XY;
            StopPlayer(player);
        }
    } else {
        if (promotion == PRESENT_EXTRA_LIFE ||
            (promotion == PRESENT_PROMOTION && (gPlayerRank[player_number] - 2) % 3 == 0)) {
            if (gPlayerLives[player_number] < 8) {
                ++gPlayerLives[player_number];
                gHudRedrawPending = 1;
            }
        } else if (promotion == PRESENT_TOTAL_BUMMER) {
            PlaySound(PSG_TONE_NOISE_BURSTS, 40);
            gPlayerHealth[player_number] = 0;
        }
        if (promotion == PRESENT_PROMOTION) {
            /* ORIGINAL BUG: this path passes the object pointer where other
             * speech calls pass the player number. */
            GenerateSpeech(&gPlayerInfo[player_number],
                           RankNameStrTable[gPlayerRank[player_number]], 1);
            PlaySoundEffect((long)Sfx_YeahAllRight, 90, 0);
        }
    }
    if (promotion == PRESENT_EXTRA_LIFE)
        PlaySoundEffect((long)Sfx_ExtraLifeGlitz, 87, 0);
}

/* Advance a promotion display, then apply the completed display's effect. */
void BummerLifePromo(player, terrain)
struct object_state *player;
unsigned short terrain;
{
    short player_number, phase, promotion_complete = 0;
    struct object_state *display;

    player_number = player->entity_index;
    if (!gPlayerPromotionDisplayEntities[player_number])
        return;
    display = &gPlayerInfo[gPlayerPromotionDisplayEntities[player_number]];
    display->level = player->level;
    if (display->frame_duration) {
        if (display->sprite_state == PRESENT_TOTAL_BUMMER) {
            if (gPlayerHealth[player_number] > 1)
                --gPlayerHealth[player_number];
            PlaySound(PSG_TONE_NOISE_BURSTS, 40);
        } else if (display->sprite_state == PRESENT_EXTRA_LIFE && gPcmEffectCooldownTicks < 4)
            PlaySoundEffect((long)Sfx_ExtraLifeGlitz, 87, 0);
        else if (display->sprite_state == PRESENT_PROMOTION &&
                 (gCachedRandomValues[gRandomCacheIndex == 58 ?
                  (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 2)) {
            phase = display->frame % 14;
            if (phase == 12 || (phase > 1 && phase < 11 && phase != 7))
                PlaySoundEffect((long)Sfx_JamSnareDrum, 90, 0);
        }
    } else {
        if (display->sprite_state == PRESENT_EXTRA_LIFE) {
            if (gPlayerLives[player_number] < 8) {
                ++gPlayerLives[player_number];
                gHudRedrawPending = 1;
            }
        } else if (display->sprite_state == PRESENT_TOTAL_BUMMER) {
            if (player->sprite_state != PLAYER_ANIM_GHOST && player->sprite_state != PLAYER_ANIM_LAST_LIFE_GHOST) {
                gPlayerHealth[player_number] = 0;
                GenerateSpeech(player_number, "Bogus", 0);
            }
        } else if (display->sprite_state == PRESENT_PROMOTION) {
            GenerateSpeech(player_number, RankNameStrTable[gPlayerRank[player_number]], 1);
            promotion_complete = 1;
            PlaySoundEffect((long)Sfx_YeahAllRight, 90, 0);
        }
        if (player->sprite_state == PLAYER_ANIM_EXTRA_LIFE || player->sprite_state == PLAYER_ANIM_ITEM_REACTION)
            SetSprite(player, GetActivePresentSprite(player));
        FreeEntity(gPlayerPromotionDisplayEntities[player_number]);
        gPlayerPromotionDisplayEntities[player_number] = 0;
        if (promotion_complete && (gPlayerRank[player_number] - 2) % 3 == 0)
            PerformPromotion(player_number, 17, terrain);
    }
}

/* Advance the delayed burps, displaying speech and resetting the random delay. */
void PresentUpdateBurps(player_number)
short player_number;
{
    if (gPlayerBurpsLeft[player_number]) {
        if (gPlayerBurpTimers[player_number] == -1) {
            StartDialogue(24, player_number);
            AudioRequestChannelTemplateEffect(FM_EFFECT_BURP);
            gPlayerBurpTimers[player_number] = 50;
        } else if (--gPlayerBurpTimers[player_number] < 0) {
            EarthlingResetStateTwoByDrawMask(gPlayerInfo[player_number].draw_flags & ENTITY_EITHER_VIEW);
            GenerateSpeech(player_number, "Burp", 1);
            --gPlayerBurpsLeft[player_number];
            gPlayerBurpTimers[player_number] = 20 + GetRandomNumber() % 81;
            if (!gPcmEffectCooldownTicks)
                PlaySoundEffect((long)Sfx_Burp, AUDIO_PRIORITY_NORMAL, 0);
        }
    }
}

/* Allocate the player's cloud object if needed and restart its countdown. */
void PresentIsRainCloud(player_number)
short player_number;
{
    struct object_state *cloud, *player;
    struct timed_animation *animation;

    if (!gPlayerRainCloudStates[player_number]) {
        if (gPlayerRainCloudStates[player_number] = (int)AllocateEntityWithEviction(0)) {
            cloud = &gPlayerInfo[gPlayerRainCloudStates[player_number]];
            player = &gPlayerInfo[player_number];
            cloud->level = player->level;
            cloud->behavior_flags |= ENTITY_ATTACHED;
            cloud->owner = player_number;
            cloud->attachment_y_or_latch = 10;
            cloud->attachment_height = 10;
            cloud->tile |= VDP_TILE_PRIORITY;
            cloud->width = 32;
            cloud->height_pixels = 32;
            cloud->sprite_count = 1;
            cloud->tile_count = 16;
            animation = PlayerRainCloudAnimations;
            cloud->sequence = animation->sequence;
            cloud->frames = animation->frames;
            cloud->frame_phase = animation->period;
            cloud->frame = -1;
            cloud->frame_duration = 1;
        }
    }
    gPlayerRainCloudTimers[player_number] = GetRandomNumber() % 101 + 400;
}

/* Advance the cloud animation, applying its strike to eligible player sprites. */
void PresentUpdateRainCloud(player_number)
short player_number;
{
    struct timed_animation *animation = 0;
    struct timed_animation *animations = PlayerRainCloudAnimations;
    struct object_state *cloud, *player;
    short cloud_number;

    if (!(cloud_number = gPlayerRainCloudStates[player_number]))
        return;
    cloud = &gPlayerInfo[cloud_number];
    player = &gPlayerInfo[player_number];
    cloud->level = player->level;
    if (gPlayerElevatorFlags[player_number] || gPlayerInventoryOpen[player_number]) {
        if (cloud->sprite_state) {
            animation = &animations[0];
            cloud->sprite_state = 0;
            gPlayerHealth[player_number] -= 2;
        }
    } else if (--gPlayerRainCloudTimers[player_number] <= 0 ||
               gPlayerLives[player_number] < 0 ||
               player->sprite_state == PLAYER_ANIM_GHOST || player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST) {
        FreeEntity(cloud_number);
        gPlayerRainCloudStates[player_number] = 0;
    } else if (cloud->sprite_state) {
        if (!cloud->frame_duration) {
            animation = &animations[0];
            cloud->sprite_state = 0;
            if (!gPlayerFallStates[player_number] &&
                (player->sprite_state == PLAYER_ANIM_IDLE || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
                 (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
                 (player->animation_group_mask & PLAYER_GROUP_RAIN_CLOUD_ZAP))) {
                gPlayerPendingHurtDamage[player_number] = 2;
                SetSprite(player, PLAYER_ANIM_ZAPPED);
                player->animation_motion_axes = ANIMATION_MOTION_XY;
                player->velocity_height = 0;
                player->acceleration_height = 0;
                PlaySound(PSG_TONE_NOISE_BURSTS, 40);
            } else if (player->animation_group_mask & (PLAYER_GROUP_JUMP_WINDUP | PLAYER_GROUP_JUMP_TAKEOFF | PLAYER_GROUP_JUMP_DESCEND))
                gPlayerHealth[player_number] -= 2;
        }
    } else if ((GetRandomNumber() & 63) == 1) {
        animation = &animations[1];
        cloud->sprite_state = 1;
    }
    if (animation) {
        cloud->sequence = animation->sequence;
        cloud->frames = animation->frames;
        cloud->frame_phase = animation->period;
        cloud->frame = -1;
        cloud->frame_duration = 1;
    }
}

/* Place and animate the boom box, start its timer, and select its music. */
void OpenBoomBox(player_number)
short player_number;
{
    struct object_state *box, *player;
    struct animation_descriptor *animation;

    if (!gPlayerBoomBoxEntities[player_number])
        gPlayerBoomBoxEntities[player_number] = (int)AllocateEntityWithEviction(0);
    if (gPlayerBoomBoxEntities[player_number]) {
        box = &gPlayerInfo[gPlayerBoomBoxEntities[player_number]];
        player = &gPlayerInfo[player_number];
        box->x = player->x;
        box->y = player->y + 4;
        box->level = player->level;
        box->width = 32;
        box->height_pixels = 24;
        box->sprite_count = 1;
        box->tile_count = 16;
        box->tile |= VDP_TILE_PRIORITY;
        animation = BoomBoxAnimations;
        box->sequence = animation->sequence;
        box->frames = animation->frames;
        box->frame_phase = 2;
        box->frame_duration = 1;
        box->behavior_flags |= ENTITY_COLLIDABLE;
        PresentStartTransientEffect(player_number, box->x, box->y + 1, 1, box->level);
        gPlayerBoomBoxTimers[player_number] = 300;
        gRequestedMusicSelection = MUSIC_BOOMBOX;
        gBoomBoxMusicTemplateHold = 1;
    }
}

/* Remove an expired boom box and restore music once neither player has one. */
void PresentUpdateBoomBox(player_number)
short player_number;
{
    struct object_state *box;

    if (gPlayerBoomBoxEntities[player_number]) {
        box = &gPlayerInfo[gPlayerBoomBoxEntities[player_number]];
        if (--gPlayerBoomBoxTimers[player_number] <= 0) {
            EarthlingWakeLevel(box->level);
            PresentStartTransientEffect(player_number, box->x, box->y + 1, 1, box->level);
            FreeEntity(gPlayerBoomBoxEntities[player_number]);
            gPlayerBoomBoxEntities[player_number] = 0;
            if (!gPlayerBoomBoxEntities[player_number ^ 1]) {
                gRequestedMusicSelection = (int)GetLevelMusicIndex(gHighestLevel);
                gBoomBoxMusicTemplateHold = 0;
            }
        } else if (gMusicAnimationEvent) {
            box->frame = -1;
            box->frame_duration = 1;
            gMusicAnimationEvent = 0;
        }
    }
}

/* Update each player's active present effects and nearby telephone. */
void ProcessPresents()
{
    short player_number;

    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
        PresentUpdateDecoy(player_number);
        PresentUpdateBoomBox(player_number);
        PresentUpdateDoorway(player_number);
        PresentUpdateBurps(player_number);
        PresentUpdateRainCloud(player_number);
        PresentUpdateTransientEffect(player_number);
        EntityReleaseFinishedTimedEffect(player_number);
        CheckForTelephone(player_number);
        CheckTomatoRainTimer(player_number);
    }
}

/* Apply the selected present, rejecting it in states where it cannot open. */
int OpenSelectedPresent(player_number, present_number)
short player_number, present_number;
{
    struct object_state *player;
    unsigned short terrain;

    player = &gPlayerInfo[player_number];
    if (gPlayerElevatorFlags[player_number] || player->sprite_state == PLAYER_ANIM_GHOST || player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST)
        return 1;
    terrain = player_number == PLAYER_TOEJAM ? gPlayer1TerrainUnderfoot : gPlayer2TerrainUnderfoot;
    if (present_number == PRESENT_BONUS_HITOPS)
        present_number = PRESENT_SUPER_HITOPS;
    else if (present_number == PRESENT_DECOY) {
        if (terrain >= TERRAIN_WATER)
            return 1;
        OpenDecoy(player_number);
    } else if (present_number == PRESENT_DOORWAY)
        PresentIsDoorway(player_number);
    else if (present_number == PRESENT_EXTRA_LIFE || present_number == PRESENT_TOTAL_BUMMER)
        PerformPromotion(player_number, present_number, terrain);
    else if (present_number == PRESENT_PROMOTION)
        gPlayerPoints[player_number] = GetScoreForPromo(gPlayerRank[player_number]) - 2;
    else if (present_number == PRESENT_FOOD) {
        if (GetRandomNumber() % 100 < 70)
            EatYummyFood(player_number);
        else
            EatYuckyFood(player_number);
    } else if (present_number == PRESENT_ROOTBEER) {
        gPlayerBurpTimers[player_number] = -1;
        gPlayerBurpsLeft[player_number] += GetRandomNumber() % 7 + 12;
        gPlayerHealth[player_number] += 4;
    } else if (present_number == PRESENT_TOMATO_RAIN) {
        SetTomatoRainTomatoes(player_number);
        GenerateSpeech(player_number, "Uh oh?!?", 1);
        if (player->sprite_state == PLAYER_ANIM_IDLE || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
            (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
            (player->animation_group_mask & PLAYER_GROUP_SIMPLE_REACTION_MOTION)) {
            SetSprite(player, PLAYER_ANIM_TOMATO_RAIN);
            StopPlayer(player);
            player->animation_motion_axes = ANIMATION_MOTION_NONE;
        }
    } else if (present_number == PRESENT_TOGETHERNESS) {
        if (gPendingUnfallPlayer)
            return 1;
        gPendingUnfallPlayer = player_number + 1;
    } else if (present_number == PRESENT_RAIN_CLOUD)
        PresentIsRainCloud(player_number);
    else if (present_number == PRESENT_FUDGE_SUNDAE)
        EatFudgeSundae(player_number);
    else if (present_number == PRESENT_TELEPHONE) {
        if (terrain >= TERRAIN_WATER)
            return 1;
        OpenTelephone(player_number);
    } else if (present_number == PRESENT_EXTRA_BUCK) {
        ++gPlayerBucks[player_number];
        StartDialogue(29, player_number);
    } else if (present_number == PRESENT_JACKPOT) {
        gPlayerBucks[player_number] += 5;
        StartDialogue(30, player_number);
    } else if (present_number == PRESENT_EARTHLING)
        OpenEarthlingPresent(player, terrain);
    else if (present_number == PRESENT_SCHOOL_BOOK) {
        if (terrain >= TERRAIN_WATER || player->height > 0)
            return 1;
        OpenSchoolBook(player);
    } else if (present_number == PRESENT_BOOMBOX) {
        if (terrain >= TERRAIN_WATER)
            return 1;
        OpenBoomBox(player_number);
    } else if (present_number == PRESENT_SPRING_SHOES && !player->level)
        return 1;
    SetupSelectedPresent(player, player_number, present_number, terrain);
    return 0;
}

/* Add opening points and initialize the present timer and eligible sprite. */
void SetupSelectedPresent(player, player_number, present_number, terrain)
struct object_state *player;
short player_number, present_number;
unsigned short terrain;
{
    char sprite;

    if (present_number < 8 && present_number != PRESENT_ROCKET_SKATES && player->height > 0) {
        if (!gPlayerPowerups[player_number] && (player->animation_group_mask & PLAYER_GROUP_ICARUS_FLY) &&
            present_number != PRESENT_ICARUS_WINGS)
            PresentTryEndActivePowerup(player_number);
        else if (gPlayerPowerups[player_number] == PRESENT_SPRING_SHOES &&
                 !gPlayerFallStates[player_number] &&
                 (player->animation_group_mask & PLAYER_GROUP_FALL) && present_number != PRESENT_SPRING_SHOES) {
            StopPlayer(player);
            player->acceleration_height = -32;
            gPlayerLandingCounterSeed[player_number] = 2;
        }
    }
    gPlayerPoints[player_number] += 2;
    if (present_number >= 8)
        return;
    gPlayerPowerups[player_number] = present_number;
    HudLoadActionIconSprite(player_number);
    gPlayerPowerupTimers[player_number] = 1000;
    sprite = -1;
    switch (present_number) {
    case PRESENT_ROCKET_SKATES:
        gPlayerPowerupTimers[player_number] >>= 2;
#if REVISION >= 2
        if (!gPlayerFallStates[player_number]) {
#endif
            PlaySound(PSG_START_BACKGROUND_NOISE, 94);
            OpenRocketSkates(player, player_number, terrain);
#if REVISION >= 2
        }
#endif
        break;
    case PRESENT_ICARUS_WINGS:
        gPlayerPowerupTimers[player_number] >>= 1;
        PlayerInitializePhasedJumpMotion(player_number);
        break;
    case PRESENT_TOMATOES:
        StartDialogue(26, player_number);
        break;
    case PRESENT_SLING_SHOT:
        StartDialogue(27, player_number);
        break;
    case PRESENT_ROSE_BUSHES:
        GenerateSpeech(player_number, "rosebushes!", 1);
        break;
    case PRESENT_SUPER_HITOPS:
        gPlayerPowerupTimers[player_number] = 750;
        StartDialogue(28, player_number);
        break;
    }
    if (present_number != PRESENT_ROCKET_SKATES)
        sprite = GetActivePresentSprite(player);
    if (sprite != -1 && !(player->animation_group_mask & PLAYER_GROUP_STATIONARY) &&
        (player->sprite_state == PLAYER_ANIM_IDLE || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
         (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
         player->sprite_state == PLAYER_ANIM_SWIM_IDLE || player->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE ||
#if REVISION >= 2
         (player->animation_group_mask & (PLAYER_GROUP_REFRESH_PRESENT | PLAYER_GROUP_HITOPS)))) {
#else
         (player->animation_group_mask & PLAYER_GROUP_REFRESH_PRESENT))) {
#endif
        SetSprite(player, sprite);
        player->velocity_x = 0;
        player->velocity_y = 0;
    }
}
