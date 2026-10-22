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
        effect->tile |= 0x8000;
        animation = &TransientEffectAnimation;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame_duration = 1;
        effect->frame = -1;
        PlaySound(16, 50);
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
        decoy->tile |= 0x8000;
        animation = &PlayerDecoyAnimations[player_number];
        decoy->sequence = animation->sequence;
        decoy->frames = animation->frames;
        decoy->frame_phase = animation->period;
        decoy->frame_duration = 1;
        decoy->flags_4d |= 1;
        gPlayerDecoyHitCounts[player_number] = 0;
        if (!PresentStartTransientEffect(player_number, decoy->x, decoy->y + 1, 5, decoy->level))
            decoy->flags_4d |= 64;
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

    for (player_number = 0; player_number <= 1; ++player_number) {
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
        decoy->flags_4d &= ~64;
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
        doorway->tile |= 0x8000;
        animation = &DoorwayAnimations[0];
        doorway->sequence = animation->sequence;
        doorway->frames = animation->frames;
        doorway->frame_phase = animation->period;
        doorway->frame_duration = 1;
        doorway->frame = -1;
        doorway->sprite_state = 0;
        doorway->flags_4d |= 1;
        if (!PresentStartTransientEffect(player_number, doorway->x, doorway->y + 1, 5, doorway->level))
            doorway->flags_4d |= 64;
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

    for (player_number = 0; player_number <= 1; ++player_number) {
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

    camera_x = TOEJAM_CAMERA_NEWX;
    camera_y = TOEJAM_CAMERA_NEWY;
    if (gPlayerLives[0] >= 0 && player_number == 1 && SPLIT_SCREEN) {
        camera_x = EARL_CAMERA_NEWX;
        camera_y = EARL_CAMERA_NEWY;
    }
    do {
        SetPositionOnNewLevel(player_number);
    } while (EntityFindNarrowCollisionAtPosition(player, player->x, player->y, 2));
    if (!SPLIT_SCREEN && player->x > camera_x + 20 &&
        player->x < camera_x + 320 - 20 && player->y > camera_y + 40 &&
        player->y < camera_y + 224 - 40)
        return;
    WaitForVBlankWithGraphicsPaused();
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    UpdateScreenSplitMode(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
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
        if (object->flags_4d & 64)
            object->flags_4d &= ~64;
        if (!(object->draw_flags & 6) && !P1_MENU_OPEN[0] && !HBLANK_FLAG_1) {
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
                    object->flags_4d &= ~1;
                else {
                    object->flags_4d |= 1;
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
        display->flags_4d |= 8;
        display->parent = player_number;
        display->level = player->level;
        display->tile |= 0x8000;
        display->width = 32;
        display->height_pixels = 32;
        if (promotion == 11) {
            animation = RankPromotionDisplayAnimation;
            display->flag_58 = 8;
            display->sprite_count = 2;
            display->tile_count = 32;
        } else {
            display->sprite_count = 1;
            display->tile_count = 16;
            if (promotion == 16)
                animation = HealthDrainDisplayAnimation;
            else
                animation = ExtraLifeDisplayAnimation;
            display->flag_58 = 6;
            display->offset_height = 1;
        }
        display->sequence = animation->sequence;
        display->frames = animation->frames;
        display->frame_phase = animation->period;
        display->frame = -1;
        display->frame_duration = 1;
        display->sprite_state = promotion;
        if (promotion != 11 && terrain < 128 && player->height <= 0 &&
            player->sprite_state != 5 && player->sprite_state != 6 && player->sprite_state != 79 &&
            player->sprite_state != 78 && player->sprite_state != 77 &&
            !(player->flags_52 & 0x7785bf60L)) {
            SetSprite(player, (char)(promotion == 17 ? 71 : 70));
            player->mode_15 = 3;
            StopPlayer(player);
        }
    } else {
        if (promotion == 17 ||
            (promotion == 11 && (gPlayerRank[player_number] - 2) % 3 == 0)) {
            if (gPlayerLives[player_number] < 8) {
                ++gPlayerLives[player_number];
                VBLANK_FLAG_5 = 1;
            }
        } else if (promotion == 16) {
            PlaySound(18, 40);
            TOEJAMS_HEALTH[player_number] = 0;
        }
        if (promotion == 11) {
            /* ORIGINAL BUG: this path passes the object pointer where other
             * speech calls pass the player number. */
            GenerateSpeech(&gPlayerInfo[player_number],
                           RankNameStrTable[gPlayerRank[player_number]], 1);
            PlaySoundEffect((long)Sfx_YeahAllRight, 90, 0);
        }
    }
    if (promotion == 17)
        PlaySoundEffect((long)Sfx_ExtraLifeGlitz, 87, 0);
}

/* Advance a promotion display, then apply the completed display's effect. */
void BummerLifePromo(player, terrain)
struct object_state *player;
unsigned short terrain;
{
    short player_number, phase, promotion_complete = 0;
    struct object_state *display;

    player_number = player->entity_number;
    if (!gPlayerPromotionDisplayEntities[player_number])
        return;
    display = &gPlayerInfo[gPlayerPromotionDisplayEntities[player_number]];
    display->level = player->level;
    if (display->frame_duration) {
        if (display->sprite_state == 16) {
            if (TOEJAMS_HEALTH[player_number] > 1)
                --TOEJAMS_HEALTH[player_number];
            PlaySound(18, 40);
        } else if (display->sprite_state == 17 && gPcmEffectCooldownTicks < 4)
            PlaySoundEffect((long)Sfx_ExtraLifeGlitz, 87, 0);
        else if (display->sprite_state == 11 &&
                 (PREGEN_RANDOMS[gRandomCacheIndex == 58 ?
                  (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 2)) {
            phase = display->frame % 14;
            if (phase == 12 || (phase > 1 && phase < 11 && phase != 7))
                PlaySoundEffect((long)Sfx_JamSnareDrum, 90, 0);
        }
    } else {
        if (display->sprite_state == 17) {
            if (gPlayerLives[player_number] < 8) {
                ++gPlayerLives[player_number];
                VBLANK_FLAG_5 = 1;
            }
        } else if (display->sprite_state == 16) {
            if (player->sprite_state != 5 && player->sprite_state != 6) {
                TOEJAMS_HEALTH[player_number] = 0;
                GenerateSpeech(player_number, "Bogus", 0);
            }
        } else if (display->sprite_state == 11) {
            GenerateSpeech(player_number, RankNameStrTable[gPlayerRank[player_number]], 1);
            promotion_complete = 1;
            PlaySoundEffect((long)Sfx_YeahAllRight, 90, 0);
        }
        if (player->sprite_state == 71 || player->sprite_state == 70)
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
    if (gPlayer1BurpsLeft[player_number]) {
        if (gPlayer1BurpTimer[player_number] == -1) {
            StartDialogue(24, player_number);
            AudioRequestChannelTemplateEffect(14);
            gPlayer1BurpTimer[player_number] = 50;
        } else if (--gPlayer1BurpTimer[player_number] < 0) {
            EarthlingResetStateTwoByDrawMask(gPlayerInfo[player_number].draw_flags & 6);
            GenerateSpeech(player_number, "Burp", 1);
            --gPlayer1BurpsLeft[player_number];
            gPlayer1BurpTimer[player_number] = 20 + GetRandomNumber() % 81;
            if (!gPcmEffectCooldownTicks)
                PlaySoundEffect((long)Sfx_Burp, 50, 0);
        }
    }
}

/* Allocate the player's cloud object if needed and restart its countdown. */
void PresentIsRainCloud(player_number)
short player_number;
{
    struct object_state *cloud, *player;
    struct timed_animation *animation;

    if (!gPlayer1RainCloudState[player_number]) {
        if (gPlayer1RainCloudState[player_number] = (int)AllocateEntityWithEviction(0)) {
            cloud = &gPlayerInfo[gPlayer1RainCloudState[player_number]];
            player = &gPlayerInfo[player_number];
            cloud->level = player->level;
            cloud->flags_4d |= 8;
            cloud->parent = player_number;
            cloud->flag_58 = 10;
            cloud->offset_height = 10;
            cloud->tile |= 0x8000;
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
    gPlayer1RainCloudTimer[player_number] = GetRandomNumber() % 101 + 400;
}

/* Advance the cloud animation, applying its strike to eligible player sprites. */
void PresentUpdateRainCloud(player_number)
short player_number;
{
    struct timed_animation *animation = 0;
    struct timed_animation *animations = PlayerRainCloudAnimations;
    struct object_state *cloud, *player;
    short cloud_number;

    if (!(cloud_number = gPlayer1RainCloudState[player_number]))
        return;
    cloud = &gPlayerInfo[cloud_number];
    player = &gPlayerInfo[player_number];
    cloud->level = player->level;
    if (ELEVATOR_FLAG[player_number] || P1_MENU_OPEN[player_number]) {
        if (cloud->sprite_state) {
            animation = &animations[0];
            cloud->sprite_state = 0;
            TOEJAMS_HEALTH[player_number] -= 2;
        }
    } else if (--gPlayer1RainCloudTimer[player_number] <= 0 ||
               gPlayerLives[player_number] < 0 ||
               player->sprite_state == 5 || player->sprite_state == 6) {
        FreeEntity(cloud_number);
        gPlayer1RainCloudState[player_number] = 0;
    } else if (cloud->sprite_state) {
        if (!cloud->frame_duration) {
            animation = &animations[0];
            cloud->sprite_state = 0;
            if (!TOEJAM_FALL_STATE[player_number] &&
                (player->sprite_state == 0 || player->sprite_state == 4 ||
                 (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
                 (player->flags_52 & 0x0834d06c))) {
                gPlayerPendingHurtDamage[player_number] = 2;
                SetSprite(player, 68);
                player->mode_15 = 3;
                player->velocity_height = 0;
                player->acceleration_height = 0;
                PlaySound(18, 40);
            } else if (player->flags_52 & 0x380)
                TOEJAMS_HEALTH[player_number] -= 2;
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
        box->tile |= 0x8000;
        animation = AnimationDescriptors_0AF0D0;
        box->sequence = animation->sequence;
        box->frames = animation->frames;
        box->frame_phase = 2;
        box->frame_duration = 1;
        box->flags_4d |= 1;
        PresentStartTransientEffect(player_number, box->x, box->y + 1, 1, box->level);
        gPlayerBoomBoxTimers[player_number] = 300;
        VBLANK_FLAG_7 = 14;
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
                VBLANK_FLAG_7 = (int)GetLevelMusicIndex(gHighestLevel);
                gBoomBoxMusicTemplateHold = 0;
            }
        } else if (gMusicUnknownE488) {
            box->frame = -1;
            box->frame_duration = 1;
            gMusicUnknownE488 = 0;
        }
    }
}

/* Update each player's active present effects and nearby telephone. */
void ProcessPresents()
{
    short player_number;

    for (player_number = 0; player_number <= 1; ++player_number) {
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
    if (ELEVATOR_FLAG[player_number] || player->sprite_state == 5 || player->sprite_state == 6)
        return 1;
    terrain = player_number == 0 ? LAND_UNDER_TOEJAM : LAND_UNDER_EARL;
    if (present_number == 27)
        present_number = 7;
    else if (present_number == 15) {
        if (terrain >= 128)
            return 1;
        OpenDecoy(player_number);
    } else if (present_number == 8)
        PresentIsDoorway(player_number);
    else if (present_number == 17 || present_number == 16)
        PerformPromotion(player_number, present_number, terrain);
    else if (present_number == 11)
        gPlayerPoints[player_number] = GetScoreForPromo(gPlayerRank[player_number]) - 2;
    else if (present_number == 9) {
        if (GetRandomNumber() % 100 < 70)
            EatYummyFood(player_number);
        else
            EatYuckyFood(player_number);
    } else if (present_number == 10) {
        gPlayer1BurpTimer[player_number] = -1;
        gPlayer1BurpsLeft[player_number] += GetRandomNumber() % 7 + 12;
        TOEJAMS_HEALTH[player_number] += 4;
    } else if (present_number == 22) {
        SetTomatoRainTomatoes(player_number);
        GenerateSpeech(player_number, "Uh oh?!?", 1);
        if (player->sprite_state == 0 || player->sprite_state == 4 ||
            (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
            (player->flags_52 & 0x0830400c)) {
            SetSprite(player, 1);
            StopPlayer(player);
            player->mode_15 = 0;
        }
    } else if (present_number == 12) {
        if (gPendingUnfallPlayer)
            return 1;
        gPendingUnfallPlayer = player_number + 1;
    } else if (present_number == 13)
        PresentIsRainCloud(player_number);
    else if (present_number == 14)
        EatFudgeSundae(player_number);
    else if (present_number == 19) {
        if (terrain >= 128)
            return 1;
        OpenTelephone(player_number);
    } else if (present_number == 20) {
        ++gPlayerBucks[player_number];
        StartDialogue(29, player_number);
    } else if (present_number == 21) {
        gPlayerBucks[player_number] += 5;
        StartDialogue(30, player_number);
    } else if (present_number == 23)
        OpenEarthlingPresent(player, terrain);
    else if (present_number == 24) {
        if (terrain >= 128 || player->height > 0)
            return 1;
        OpenSchoolBook(player);
    } else if (present_number == 25) {
        if (terrain >= 128)
            return 1;
        OpenBoomBox(player_number);
    } else if (present_number == 1 && !player->level)
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

    if (present_number < 8 && present_number != 5 && player->height > 0) {
        if (!gPlayer1Powerup[player_number] && (player->flags_52 & 0x8000) &&
            present_number != 0)
            PresentTryEndActivePowerup(player_number);
        else if (gPlayer1Powerup[player_number] == 1 &&
                 !TOEJAM_FALL_STATE[player_number] &&
                 (player->flags_52 & 32) && present_number != 1) {
            StopPlayer(player);
            player->acceleration_height = -32;
            gPlayerLandingCounterSeed[player_number] = 2;
        }
    }
    gPlayerPoints[player_number] += 2;
    if (present_number >= 8)
        return;
    gPlayer1Powerup[player_number] = present_number;
    HudLoadActionIconSprite(player_number);
    gPlayer1PowerupTimer[player_number] = 1000;
    sprite = -1;
    switch (present_number) {
    case 5:
        gPlayer1PowerupTimer[player_number] >>= 2;
#if REVISION >= 2
        if (!TOEJAM_FALL_STATE[player_number]) {
#endif
            PlaySound(5, 94);
            OpenRocketSkates(player, player_number, terrain);
#if REVISION >= 2
        }
#endif
        break;
    case 0:
        gPlayer1PowerupTimer[player_number] >>= 1;
        PlayerInitializePhasedJumpMotion(player_number);
        break;
    case 3:
        StartDialogue(26, player_number);
        break;
    case 4:
        StartDialogue(27, player_number);
        break;
    case 6:
        GenerateSpeech(player_number, "rosebushes!", 1);
        break;
    case 7:
        gPlayer1PowerupTimer[player_number] = 750;
        StartDialogue(28, player_number);
        break;
    }
    if (present_number != 5)
        sprite = GetActivePresentSprite(player);
    if (sprite != -1 && !(player->flags_52 & 0x20000000) &&
        (player->sprite_state == 0 || player->sprite_state == 4 ||
         (player->flags_52 & 0x20400000) || player->sprite_state == 76 ||
         player->sprite_state == 3 || player->sprite_state == 7 ||
#if REVISION >= 2
         (player->flags_52 & 0x0804c81c))) {
#else
         (player->flags_52 & 0x0004c81c))) {
#endif
        SetSprite(player, sprite);
        player->velocity_x = 0;
        player->velocity_y = 0;
    }
}
