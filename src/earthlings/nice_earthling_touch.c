#include "psg_effects.h"
#include "player_animation.h"
#include "player_animation_groups.h"
#include "earthling_types.h"
#include "entities.h"
#include "earthlings.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_audio.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "ram_camera.h"
#include "ram_earthlings.h"
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

/* MATCHING QUALIFICATION: the (int)AllocateEntityWithEviction() and
 * (int)AllocateEntity() result views retain the observed call-result widths.
 * The shared short declarations stay canonical; historical spelling is unknown [INFERENCE]. */

/* Interaction choices use one display object and one result byte per player. */

/* Allocate the two-choice display, copying the player's current level. */
void PlayerCreateInteractionChoiceDisplay(player_number)
short player_number;
{
    struct object_state *display;
    struct animation_descriptor *animation;

    gPlayerInteractionResponses[player_number] = 0;
    if (gPlayerInteractionChoiceEntities[player_number] = (int)AllocateEntityWithEviction(0)) {
        display = &gPlayerInfo[gPlayerInteractionChoiceEntities[player_number]];
        display->level = (&gPlayerInfo[player_number])->level;
        display->behavior_flags |= ENTITY_ATTACHED;
        display->owner = player_number;
        display->attachment_y_or_latch = 10;
        display->attachment_height = 10;
        display->tile |= VDP_TILE_PRIORITY;
        display->width = 32;
        display->height_pixels = 32;
        display->sprite_count = 1;
        display->tile_count = 16;
        animation = PlayerInteractionChoiceAnimations;
        display->sequence = animation->sequence;
        display->frames = animation->frames;
        display->frame = -1;
        display->frame_duration = 1;
        gPlayerInteractionChoiceActive[player_number] = 1;
    }
}

/* Resolve A-button choices, cancelling while the player is in sprite 5 or 6. */
void CheckAButton()
{
    short player_number;
    struct object_state *display;
    char choice, cancel;

    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
        if (gPlayerInteractionChoiceActive[player_number]) {
            display = &gPlayerInfo[gPlayerInteractionChoiceEntities[player_number]];
            choice = display->sprite_state;
            cancel = (&gPlayerInfo[player_number])->sprite_state == PLAYER_ANIM_GHOST ||
                     (&gPlayerInfo[player_number])->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST;
            if (cancel || ((gPlayerProcessedInput[player_number] & INPUT_BUTTON_A) &&
                           !(gPlayerPreviousProcessedInput[player_number] & INPUT_BUTTON_A))) {
                if (cancel)
                    gPlayerInteractionResponses[player_number] = 0;
                else if (display->sprite_state == 1)
                    gPlayerInteractionResponses[player_number] = 1;
                else
                    gPlayerInteractionResponses[player_number] = 2;
                FreeEntity(gPlayerInteractionChoiceEntities[player_number]);
                gPlayerInteractionChoiceEntities[player_number] = 0;
                gPlayerInteractionChoiceActive[player_number] = 0;
                gPlayerPreviousProcessedInput[player_number] = INPUT_BUTTON_A;
                gPlayerProcessedInput[player_number] = INPUT_BUTTON_A;
                gPlayerChoiceActionReleaseLatches[player_number] = 1;
            } else {
                if (gPlayerProcessedInput[player_number] & INPUT_UP)
                    choice = 0;
                else if (gPlayerProcessedInput[player_number] & INPUT_DOWN)
                    choice = 1;
                if (choice != display->sprite_state) {
                    display->frame = choice - 1;
                    display->frame_duration = 1;
                    display->sprite_state = choice;
                }
                gPlayerProcessedInput[player_number] = 0;
            }
        }
    }
}

/* A mailbox offers mail order until activated, then checks its biting animation. */
void AI_MailboxTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct special_earthling_state *mailbox;
    struct object_state *player, *object;

    if (gPlayerInteractionChoiceActive[player_number])
        return;
    if (gMailboxLevelStates[0].level == earthling->level)
        mailbox = gMailboxLevelStates;
    else
        mailbox = &gMailboxLevelStates[1];
    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (mailbox->mode != 1) {
        if ((!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
             (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
             (player->sprite_state == PLAYER_ANIM_SPRING_SHOES_DOWN && !player->velocity_y)) &&
            EntityIsWithinRange(object, player, 30, 15)) {
            if (!mailbox->contact[player_number]) {
                GenerateSpeech(earthling->object, "Mail Order?", 0);
                PlayerCreateInteractionChoiceDisplay(player_number);
                mailbox->contact[player_number] = 1;
            } else if (mailbox->contact[player_number] == 1) {
                if (gPlayerInteractionResponses[player_number] == 2)
                    InventoryOpenMailOrderMenu(player_number);
                mailbox->contact[player_number] = 2;
                gPlayerInteractionResponses[player_number] = 0;
            }
        } else
            mailbox->contact[player_number] = 0;
        return;
    }
    if ((earthling->state & 252) == 4) {
        if (EntityIsWithinRange(object, player, 20, 10))
            Player_Youch(player_number, earthling);
        if (!--mailbox->timer) {
            SetEarthlingState(earthling, 0);
            object->frame = -1;
            mailbox->timer = 175;
        } else if (!gPcmEffectCooldownTicks) {
            PlaySoundEffect((long)Sfx_MailboxRoar, AUDIO_PRIORITY_NORMAL, 0);
            earthling->saved_x_or_status = 1;
        }
    } else if (!earthling->state) {
        if (mailbox->timer)
            --mailbox->timer;
        else if (player->sprite_state != PLAYER_ANIM_GHOST && player->sprite_state != PLAYER_ANIM_LAST_LIFE_GHOST &&
                 EntityIsWithinRange(object, player, 100, 40))
            SetEarthlingState(earthling, 3);
        else if ((GetRandomNumber() & 255) == 1)
            SetEarthlingState(earthling, 1);
    } else if (earthling->state == 1) {
        if (!object->frame_duration) {
            SetEarthlingState(earthling, 0);
            object->frame = -1;
        }
    } else if (earthling->state == 3) {
        if (!object->frame_duration) {
            SetEarthlingState(earthling, 4);
            earthling->timer = 0;
            mailbox->timer = 33;
            earthling->saved_x_or_status = 0;
        }
    }
}

/* The wizard charges one buck and advances health one unit at a time. */
void AI_WizardTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char next_state = -1;
    struct object_state *player, *object;

    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (!((!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
           (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
           player->sprite_state == PLAYER_ANIM_EXTRA_LIFE ||
           (player->sprite_state == PLAYER_ANIM_SPRING_SHOES_DOWN && !player->velocity_y)) &&
          EntityIsWithinRange(object, player, 35, 15))) {
        gPlayerWizardInteractionStates[player_number] = 0;
        return;
    }
    if (gPlayerWizardInteractionStates[player_number] == 2)
        return;
    earthling->state_delay = 1;
    if (gPlayerInteractionChoiceActive[player_number] ||
        (gPlayerInfo[PLAYER_TOEJAM].level == gPlayerInfo[PLAYER_EARL].level &&
         gPlayerWizardInteractionStates[player_number ^ 1] && gPlayerWizardInteractionStates[player_number ^ 1] != 2))
        return;
    if (!gPlayerWizardInteractionStates[player_number]) {
        if ((earthling->state == 4 || earthling->state == 5) &&
            !object->frame_duration)
            earthling->state += 2;
        if (earthling->state >= 6)
            next_state = EarthlingSelectTurnState(earthling);
        else if (earthling->state <= 1 && object->frame > 4) {
            GenerateSpeech(earthling->object, "Got a buck?", 0);
            PlayerCreateInteractionChoiceDisplay(player_number);
            gPlayerWizardInteractionStates[player_number] = 1;
        }
    } else if (!object->frame_duration) {
        if (gPlayerWizardInteractionStates[player_number] == 1) {
            if (gPlayerInteractionResponses[player_number] == 2) {
                if (gPlayerBucks[player_number] > 0) {
                    next_state = 2;
                    gPlayerWizardInteractionStates[player_number] = 3;
                    --gPlayerBucks[player_number];
                    SetSprite(player, PLAYER_ANIM_EXTRA_LIFE);
                    PlaySoundEffect((long)Sfx_Hallelujah, 92, 1);
                } else
                    PlaySound(PSG_PERIODIC_TONE2_NOISE, AUDIO_PRIORITY_NORMAL);
            } else {
                gPlayerWizardInteractionStates[player_number] = 2;
                next_state = 5;
                object->velocity_x = 320;
            }
            gPlayerInteractionResponses[player_number] = 0;
        } else if (gPlayerWizardInteractionStates[player_number] == 3) {
            if (gPlayerHealth[player_number] < PlayerGetMaximumHealth(player_number))
                ++gPlayerHealth[player_number];
            else {
                gPlayerWizardInteractionStates[player_number] = 2;
                next_state = 3;
                SetSprite(player, GetActivePresentSprite(player));
            }
        } else if (earthling->state == 3) {
            next_state = 5;
            object->velocity_x = 320;
        }
    }
    if (next_state != -1) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* The wiseman opens the identification menu after a positive response. */
void AI_WisemanTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char next_state = -1;
    struct object_state *player, *object;

    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (!((!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
           (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
           (player->sprite_state == PLAYER_ANIM_SPRING_SHOES_DOWN && !player->velocity_y)) &&
          EntityIsWithinRange(object, player, 35, 15))) {
        gPlayerWisemanInteractionStates[player_number] = 0;
        return;
    }
    if (gPlayerWisemanInteractionStates[player_number] == 2)
        return;
    earthling->state_delay = 1;
    if (gPlayerInteractionChoiceActive[player_number] ||
        (gPlayerInfo[PLAYER_TOEJAM].level == gPlayerInfo[PLAYER_EARL].level &&
         gPlayerWisemanInteractionStates[player_number ^ 1] && gPlayerWisemanInteractionStates[player_number ^ 1] != 2))
        return;
    if (!gPlayerWisemanInteractionStates[player_number]) {
        if (earthling->state >= 2)
            next_state = EarthlingSelectTurnState(earthling);
        else if (earthling->state <= 1) {
            GenerateSpeech(earthling->object, "Got 2 bucks?", 0);
            PlayerCreateInteractionChoiceDisplay(player_number);
            gPlayerWisemanInteractionStates[player_number] = 1;
        }
    } else if (gPlayerWisemanInteractionStates[player_number] == 1) {
        if (gPlayerInteractionResponses[player_number] == 2) {
            PlaySoundEffect((long)Sfx_WisemanThink, 64, 1);
            InventoryOpenIdentificationMenu(player_number);
        }
        gPlayerWisemanInteractionStates[player_number] = 3;
        gPlayerInteractionResponses[player_number] = 0;
    } else if (gPlayerWisemanInteractionStates[player_number] == 3 && !gPlayerInventoryOpen[player_number]) {
        gPlayerWisemanInteractionStates[player_number] = 2;
        next_state = 2;
        object->velocity_x = -320;
    }
    if (next_state != -1) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* Three bucks starts the singer's animation and the nearby-earthling ability. */
void AI_OperaSingerTouch(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    char next_state = -1;
    struct object_state *player, *object;

    player = &gPlayerInfo[player_number];
    object = &gPlayerInfo[earthling->object];
    if (!((!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
           (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
           player->sprite_state == PLAYER_ANIM_COVER_EARS ||
           (player->sprite_state == PLAYER_ANIM_SPRING_SHOES_DOWN && !player->velocity_y)) &&
          EntityIsWithinRange(object, player, 35, 15))) {
        gPlayerOperaSingerInteractionStates[player_number] = 0;
        return;
    }
    if (gPlayerOperaSingerInteractionStates[player_number] == 2)
        return;
    earthling->state_delay = 1;
    if (gPlayerInteractionChoiceActive[player_number] ||
        (gPlayerInfo[PLAYER_TOEJAM].level == gPlayerInfo[PLAYER_EARL].level &&
         gPlayerOperaSingerInteractionStates[player_number ^ 1] && gPlayerOperaSingerInteractionStates[player_number ^ 1] != 2))
        return;
    if (!gPlayerOperaSingerInteractionStates[player_number]) {
        if (earthling->state >= 4)
            next_state = 0;
        else if (earthling->state <= 1) {
            GenerateSpeech(earthling->object, "Got 3 bucks?", 0);
            PlayerCreateInteractionChoiceDisplay(player_number);
            gPlayerOperaSingerInteractionStates[player_number] = 1;
        }
    } else if (gPlayerOperaSingerInteractionStates[player_number] == 1) {
        if (gPlayerInteractionResponses[player_number] == 2) {
            if (gPlayerBucks[player_number] >= 3) {
                next_state = 3;
                gPlayerOperaSingerInteractionStates[player_number] = 3;
                gPlayerBucks[player_number] -= 3;
                SetSprite(player, PLAYER_ANIM_COVER_EARS);
                player_number ^= 1;
                player = &gPlayerInfo[player_number];
                if (gPlayerLives[player_number] >= 0 && !gSplitScreenEnabled &&
                    (!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
                     (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
                     (player->animation_group_mask & PLAYER_GROUP_SIMPLE_REACTION_MOTION)))
                    SetSprite(player, PLAYER_ANIM_COVER_EARS);
                PlaySoundEffect((long)Sfx_OperaSing, 90, 1);
            } else
                PlaySound(PSG_PERIODIC_TONE2_NOISE, AUDIO_PRIORITY_NORMAL);
        } else {
            gPlayerOperaSingerInteractionStates[player_number] = 2;
            next_state = 0;
        }
        /* The index is the other player after a successful paid response. */
        gPlayerInteractionResponses[player_number] = 0;
    } else if (gPlayerOperaSingerInteractionStates[player_number] == 3) {
        if (!object->frame_duration) {
            gPlayerOperaSingerInteractionStates[player_number] = 2;
            next_state = 0;
            SetSprite(player, GetActivePresentSprite(player));
            player_number ^= 1;
            player = &gPlayerInfo[player_number];
            if (gPlayerLives[player_number] >= 0 && !gSplitScreenEnabled &&
                player->sprite_state == PLAYER_ANIM_COVER_EARS)
                SetSprite(player, GetActivePresentSprite(player));
        } else
            DoVikingAbility(player->draw_flags & ENTITY_EITHER_VIEW);
    }
    if (next_state != -1) {
        SetEarthlingState(earthling, next_state);
        object->frame = -1;
    }
}

/* These earthling states preserve the last hit point in DamageEarthling. */
int NiceEarthlingCheck(earthling)
struct earthling_state *earthling;
{
    if (earthling->kind == EARTHLING_OPERA_SINGER) {
        if (earthling->state == 3)
            return 1;
    } else if (earthling->kind == EARTHLING_WIZARD) {
        if (earthling->state == 2)
            return 1;
    } else if (earthling->kind == EARTHLING_WISEMAN && earthling->state <= 1 &&
               (gPlayerWisemanInteractionStates[PLAYER_TOEJAM] == 3 || gPlayer2WisemanInteractionState == 3))
        return 1;
    return 0;
}

/* Place a player animation at camera offset (80,224), retaining draw mask 2. */
int PlayerCreateLifeRequestDisplay(player_number)
short player_number;
{
    struct object_state *object;
    struct timed_animation *animation;

    if (gLifeRequestDisplayEntity = (int)AllocateEntity()) {
        object = &gPlayerInfo[gLifeRequestDisplayEntity];
        object->x = gCamera1PendingX + 80;
        object->y = gCamera1PendingY + 224;
        object->height = 112;
        object->level = gPlayer1MapLevel;
        object->width = 32;
        object->height_pixels = 48;
        object->sprite_count = 4;
        object->tile_count = 40;
        object->tile |= VDP_TILE_PRIORITY;
        if (!player_number)
            animation = &PlayerAnimationDescriptors[PLAYER_ANIM_GHOST];
        else
            animation = &Player2AnimationDescriptors[PLAYER_ANIM_GHOST];
        object->sequence = animation->sequence;
        object->frames = animation->frames;
        object->frame_phase = animation->period;
        object->frame_duration = 1;
        object->frame = -1;
        UpdateEntityViewportSprites(gLifeRequestDisplayEntity);
        if (object->draw_flags & ENTITY_PRIMARY_VIEW)
            return 0;
        FreeEntity(gLifeRequestDisplayEntity);
    }
    return 1;
}

/* An eliminated player may ask the other player for a life. */
void PlayerProcessLifeRequest(player_number)
short player_number;
{
    short other_player;
    struct object_state *player;

    if (gPlayer1MapLevel == LEVEL_FINAL)
        return;
    other_player = player_number ^ 1;
    player = &gPlayerInfo[other_player];
    if (!gLifeRequestDisplayEntity && gPlayerLives[other_player] > 0 &&
        !gPlayerInteractionChoiceActive[other_player] && !gPlayerElevatorFlags[other_player] &&
        (!player->sprite_state || player->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
         (player->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || player->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
         (player->animation_group_mask & (PLAYER_GROUP_WALK | PLAYER_GROUP_SNEAK | PLAYER_GROUP_SPRING_SHOES))) &&
        !(player->animation_group_mask & PLAYER_GROUP_STATIONARY) &&
        (ReadMappedPlayerInput(player_number) & INPUT_ACTION_MASK) &&
        !PlayerCreateLifeRequestDisplay(player_number)) {
        SetSprite(player, GetActivePresentSprite(player));
        player = &gPlayerInfo[gLifeRequestDisplayEntity];
        player->height += 30;
        SetNextSpeechLifetime(64);
        if (gPlayerSelection != 2)
            GenerateSpeech(gLifeRequestDisplayEntity, "Can I play?", 0);
        else
            GenerateSpeech(gLifeRequestDisplayEntity, "Got a life?", 0);
        player->height -= 30;
        PlayerCreateInteractionChoiceDisplay(other_player);
        gLifeRequestPending = 1;
    }
    if (gLifeRequestPending) {
        if (!gPlayerInteractionResponses[other_player])
            return;
        FreeEntity(gLifeRequestDisplayEntity);
        gLifeRequestDisplayEntity = 0;
        if (gPlayerInteractionResponses[other_player] == 2) {
            while (gSpriteGraphicsUploadPending)
                ;
            DisableBlankingGraphics();
            gMode2RegSoftcopy &= 0x3c;
            VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
            if (gPlayerSelection == 2) {
                --gPlayerLives[other_player];
                HudLoadLivesSpriteTiles(other_player);
            }
            CopyLevelBetweenPlayers(PLAYER_TOEJAM);
            gPlayer2MapLevel = gPlayer1MapLevel;
            player = &gPlayerInfo[player_number];
            player->draw_flags = ENTITY_ACTIVE;
            player->behavior_flags = ENTITY_COLLIDABLE;
            player->sprite_state = PLAYER_ANIM_IDLE;
            PlayerResetRankAndResources(player_number);
            StopPlayer(player);
            player->animation_motion_axes = ANIMATION_MOTION_NONE;
            gPlayerSelection = 2;
            gSplitScreenEnabled = 1;
            PlayerRelocateToPartnerOrNextLevel(player_number);
        }
        gLifeRequestPending = 0;
        gPlayerInteractionResponses[other_player] = 0;
    }
}
