#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "psg_effects.h"
#include "entities.h"
#include "hardware.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "map_calls.h"
#include "menu_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Change levels once both falling players, or the only falling player,
 * are ready. A supported partner supplies the landing coordinates. */
void MakePlayerFall(falling)
char *falling;
{
    short player_number;
    struct object_state *player;

    if ((falling[0] && falling[1]) ||
        (falling[0] && !TOEJAM_FALL_STATE[PLAYER_EARL]) ||
        (falling[1] && !TOEJAM_FALL_STATE[PLAYER_TOEJAM])) {
        for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
            player = &gPlayerInfo[player_number];
            if (falling[player_number]) {
                player->tile |= VDP_TILE_PRIORITY;
                player->height = 600;
                player->velocity_height = -640;
                player->acceleration_height = -16;
                player->draw_flags |= ENTITY_SHADOW;
                EarthlingReleaseLevelBank(player->level);
                MapReleaseLevelItemBank(player->level);
                if (--player->level < 0)
                    player->level = gHighestLevel;
                TOEJAM_FALL_STATE[player_number] = 2;
            }
            if (gPlayerLives[player_number] < 0)
                ElevatorInvalidateSecondaryRecords();
        }
        if (falling[0]) {
            MapLoadPlayerLevel(0, gPlayerInfo[PLAYER_TOEJAM].level);
            SetupElevator(gPlayerInfo[PLAYER_TOEJAM].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_TOEJAM].level);
            PlaceStuffInLevel(gPlayerInfo[PLAYER_TOEJAM].level);
        }
        if (falling[1]) {
            MapLoadPlayerLevel(1, gPlayerInfo[PLAYER_EARL].level);
            SetupElevator(gPlayerInfo[PLAYER_EARL].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_EARL].level);
            PlaceStuffInLevel(gPlayerInfo[PLAYER_EARL].level);
        }
        ClearSkipVHBlankFlags();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
        if (gPlayerInfo[PLAYER_TOEJAM].level == gPlayerInfo[PLAYER_EARL].level ||
            gPlayerLives[PLAYER_TOEJAM] < 0 || gPlayer2Lives < 0) {
            if (falling[0] && falling[1])
                SetPositionOnNewLevel(2);
            else if (falling[0]) {
                player = &gPlayerInfo[PLAYER_EARL];
                /* (long) view of animation_group_mask: the signed 32-bit test reproduces the ROM's D0 mask and allocation;
                 * the stored field stays unsigned ([INFERENCE] the original spelling is unknown). */
                if (LAND_UNDER_EARL >= TERRAIN_WATER ||
                    ((long)player->animation_group_mask & (long)PLAYER_GROUP_STATIONARY) || player->sprite_state == PLAYER_ANIM_GHOST ||
                    player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST || gPlayer2Lives < 0)
                    SetPositionOnNewLevel(0);
                else {
                    gPlayerInfo[PLAYER_TOEJAM].x = gPlayerInfo[PLAYER_EARL].x;
                    gPlayerInfo[PLAYER_TOEJAM].y = gPlayerInfo[PLAYER_EARL].y;
                    if (ELEVATOR_FLAG[PLAYER_EARL])
                        gPlayerInfo[PLAYER_TOEJAM].y += 12;
                }
            } else {
                player = &gPlayerInfo[PLAYER_TOEJAM];
                if (LAND_UNDER_TOEJAM >= TERRAIN_WATER ||
                    ((long)player->animation_group_mask & (long)PLAYER_GROUP_STATIONARY) || player->sprite_state == PLAYER_ANIM_GHOST ||
                    player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST || gPlayerLives[PLAYER_TOEJAM] < 0)
                    SetPositionOnNewLevel(1);
                else {
                    gPlayerInfo[PLAYER_EARL].x = gPlayerInfo[PLAYER_TOEJAM].x;
                    gPlayerInfo[PLAYER_EARL].y = gPlayerInfo[PLAYER_TOEJAM].y;
                    if (ELEVATOR_FLAG[PLAYER_TOEJAM])
                        gPlayerInfo[PLAYER_EARL].y += 12;
                }
            }
        } else {
            if (falling[0])
                SetPositionOnNewLevel(0);
            if (falling[1])
                SetPositionOnNewLevel(1);
        }
        UpdateScreenSplitMode(1);
        HudLoadLevelNumberSprites(0);
        HudPositionLevelNumberSprites(0);
        HudLoadLevelNumberSprites(1);
        HudPositionLevelNumberSprites(1);
        VDP_CONTROL_WORD = 0x8c89;
        gMode2RegSoftcopy |= 0x40;
        VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
        SetSkipVHBlankFlags();
    }
}

/* Advance a jump or landing animation and select the next sprite. */
void PlayerUpdateJumpLandingAnimation(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char sprite;

    player_number = player->entity_index;
    if (!player->level || gPlayer1Powerup[player_number] == PRESENT_SPRING_SHOES)
        return;
    sprite = -1;
    if (player->animation_group_mask & PLAYER_GROUP_JUMP_WINDUP) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 9) {
            sprite = player->sprite_state + 4;
            player->height = 1;
        }
    } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_TAKEOFF) {
        if (!player->frame_duration) {
            StopPlayer(player);
            player->height = 35;
            player->draw_flags |= ENTITY_SHADOW;
            sprite = player->sprite_state + 4;
            player->acceleration_height = -64;
        }
    } else if ((player->animation_group_mask & PLAYER_GROUP_JUMP_DESCEND) && player->height < 4) {
        if (land == TERRAIN_WATER) {
            StopPlayer(player);
            player->height = 0;
            player->draw_flags &= ~ENTITY_SHADOW;
            sprite = player->sprite_state + 4;
            PlaySoundEffect((long)Sfx_Splash, 50, 0);
        } else {
            TOEJAM_JUMP_COUNTER[player_number] = 1;
            TOEJAM_FALL_STATE[player_number] = 2;
            sprite = player->sprite_state + 0xf0;
        }
    }
    if (sprite != -1)
        SetSprite(player, sprite);
}

/* Finish present animations and leave sprite 3 when nearby ground appears. */
void PlayerUpdatePresentCompletionAndTerrainPose(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number, row, column;
    char sprite;

    player_number = player->entity_index;
    sprite = -1;
    if (player->animation_group_mask & PLAYER_GROUP_CLIMB) {
        if (!player->frame_duration)
            sprite = GetActivePresentSprite(player);
    } else if ((player->animation_group_mask & (PLAYER_GROUP_JUMP_SPLASH | PLAYER_GROUP_FALL_SPLASH)) && player->level &&
               !player->frame_duration) {
        if (gPlayer1Powerup[player_number] == PRESENT_INNERTUBE)
            sprite = PLAYER_ANIM_INNERTUBE_WATER_IDLE;
        else
            sprite = PLAYER_ANIM_SWIM_IDLE;
        player->velocity_x = 0;
        player->velocity_y = 0;
        player->animation_motion_axes = ANIMATION_MOTION_NONE;
    }
    if (player->sprite_state == PLAYER_ANIM_SWIM_IDLE) {
        for (row = 0; row < 5; ++row) {
            for (column = 0; column < 5; ++column) {
                if ((short)(player_number == PLAYER_TOEJAM ? LAND_NEAR_TOEJAM[row][column] :
                            LAND_NEAR_EARL[row][column]) != 0x80) {
                    sprite = PLAYER_ANIM_SWIM_NEAR_SHORE;
                    row = 4;
                    break;
                }
            }
        }
    }
    if (sprite != -1)
        SetSprite(player, sprite);
}

/* Lower the player on terrain 0x20, with speech and partner overlap checks. */
void PlayerUpdateTerrainSinking(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number, previous_height;
    int timer;

    player_number = player->entity_index;
    if (gPlayerInteractionChoiceActive[player_number] || (player->animation_group_mask & PLAYER_GROUP_REACTION) ||
        player->sprite_state == PLAYER_ANIM_WHIRLWIND || player->sprite_state == PLAYER_ANIM_WHIRLWIND_RECOVERY ||
        player->sprite_state == PLAYER_ANIM_GHOST || player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST)
        return;
    previous_height = player->height;
    if (land == TERRAIN_SINKING && player->height <= 0 && !(player->animation_group_mask & (unsigned long)0x60) &&
        !ELEVATOR_FLAG[player_number]) {
        if (!gPlayerTerrainSinking[player_number]) {
            gPlayerTerrainSinking[player_number] = 1;
            player->height = -3;
        }
        player->draw_flags |= ENTITY_SHADOW;
        /* MATCHING QUALIFICATION: the counter goes through the otherwise unused
         * int `timer` because only that assignment reproduces the ROM's copy-
         * then-mask instruction order; masking TIME_COUNTER directly (as int or
         * unsigned long) schedules differently in both revisions. The historical
         * spelling is unknown. */
        if (!((timer = TIME_COUNTER) & (unsigned long)60)) {
            if ((player_number == PLAYER_TOEJAM && player->height > -16) ||
                (player_number == PLAYER_EARL && player->height > -20))
                --player->height;
            else if (!ram_ffda2e) {
                if (!SPLIT_SCREEN && gPlayerLives[player_number ^ 1] >= 0) {
                    if (PREGEN_RANDOMS[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) :
                                      ++gRandomCacheIndex] & 1) {
                        GenerateSpeech(player_number, "Help", 1);
                        /* MATCHING QUALIFICATION: 256 deliberately truncates to zero
                         * in this byte store, selecting the observed MOVE.B
                         * instead of CLR.B. The historical literal is unknown;
                         * no original bug or intended cooldown is asserted. */
                        ram_ffda2e = 256;
                    }
                }
            } else
                --ram_ffda2e;
        }
        if (EntityIsWithinRange(player, &gPlayerInfo[player_number ^ 1], 18, 7))
            player->height = -3;
    } else if (gPlayerTerrainSinking[player_number]) {
        player->height = 0;
        gPlayerTerrainSinking[player_number] = 0;
        player->draw_flags &= ~ENTITY_SHADOW;
    }
    if (previous_height != player->height)
        player->sprite_ready = 0;
}

/* Apply the active sinking animation, then handle Earl's idle variation. */
void PlayerUpdateHurtRecoveryAndIdle(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char direction;
    unsigned char *nearby_land;

    player_number = player->entity_index;
    if (!player_number)
        nearby_land = &LAND_NEAR_TOEJAM[0][0];
    else
        nearby_land = &LAND_NEAR_EARL[0][0];
    if (((player->animation_group_mask & PLAYER_GROUP_HURT) || player->sprite_state == PLAYER_ANIM_ZAPPED) &&
        !player->frame_duration) {
        player->draw_flags &= ~ENTITY_SHADOW;
        TOEJAMS_HEALTH[player_number] -= gPlayerPendingHurtDamage[player_number];
        direction = player->sprite_state & DIRECTION_MASK;
        if (gPlayer1Powerup[player_number] == PRESENT_ROCKET_SKATES)
            OpenRocketSkates(player, player_number, land);
        else if (player->height > 4) {
            StopPlayer(player);
            player->animation_motion_axes = ANIMATION_MOTION_NONE;
            player->acceleration_height = -32;
            gPlayerLandingCounterSeed[player_number] = 2;
            if (player->sprite_state == PLAYER_ANIM_ZAPPED)
                direction = 1;
            SetSprite(player, direction + PLAYER_ANIM_FALL_UP);
        } else if (nearby_land[12] < TERRAIN_WATER)
            SetSprite(player, GetActivePresentSprite(player));
        else {
            if (nearby_land[11] < TERRAIN_WATER)
                player->x += 16;
            if (nearby_land[13] < TERRAIN_WATER)
                player->x -= 16;
            if (nearby_land[12] == TERRAIN_WATER) {
                if (player->sprite_state == PLAYER_ANIM_ZAPPED)
                    direction = 1;
                else if (direction & 2)
                    direction ^= 1;
                SetSprite(player, direction + PLAYER_ANIM_FALL_SPLASH_UP);
                player->height = 0;
                PlaySoundEffect((long)Sfx_Splash, 50, 0);
            } else {
                if (TOEJAMS_HEALTH[player_number] <= 0)
                    TOEJAMS_HEALTH[player_number] = 1;
                StopPlayer(player);
                PlayerStartFalling(player, player_number);
            }
        }
    }
    if (player_number == PLAYER_EARL) {
        if (!player->sprite_state && player->frame) {
            if ((GetRandomNumber() & 0x7ff) == 1)
                SetSprite(player, PLAYER_ANIM_IDLE_VARIANT);
        } else if (player->sprite_state == PLAYER_ANIM_IDLE_VARIANT && !player->frame_duration) {
            SetSprite(player, PLAYER_ANIM_IDLE);
            player->frame = 1;
        }
    }
}

/* Start and complete spring-shoe jumps while the powerup is active. */
void PlayerUpdateSpringShoeJump(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char sprite, direction;

    player_number = player->entity_index;
    if (gPlayer1Powerup[player_number] != PRESENT_SPRING_SHOES ||
        TOEJAM_FALL_STATE[player_number] || ELEVATOR_FLAG[player_number])
        return;
    sprite = -1;
    direction = player->sprite_state & DIRECTION_MASK;
    if (player->animation_group_mask & PLAYER_GROUP_SPRING_SHOES) {
        if ((P1_PROCESSED_INPUT[player_number] & 64) &&
            (player->velocity_x || player->velocity_y)) {
            sprite = direction + PLAYER_ANIM_JUMP_WINDUP_UP;
            TOEJAM_JUMP_COUNTER[player_number] = 0;
        }
    } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_WINDUP) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 4) {
            if (player->velocity_x > 0)
                sprite = PLAYER_ANIM_JUMP_TAKEOFF_RIGHT;
            else if (player->velocity_x < 0)
                sprite = PLAYER_ANIM_JUMP_TAKEOFF_LEFT;
            else
                sprite = player->sprite_state + 4;
            player->height = 1;
            PlaySoundEffect((long)Sfx_Springing, 50, 0);
        }
    } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_TAKEOFF) {
        if (!player->frame_duration) {
            player->height = 35;
            player->velocity_height = 256;
            player->acceleration_height = -4;
            player->velocity_x <<= 1;
            player->animation_motion_axes = ANIMATION_MOTION_NONE;
            player->draw_flags |= ENTITY_SHADOW;
            sprite = direction + PLAYER_ANIM_FALL_UP;
            if (sprite & 2)
                sprite ^= 1;
            gPlayerLandingCounterSeed[player_number] = 2;
            gPlayerTerrainSinking[player_number] = 0;
        }
    } else if (player->animation_group_mask & PLAYER_GROUP_WALK)
        sprite = direction + PLAYER_ANIM_SPRING_SHOES_UP;
    if (sprite != -1)
        SetSprite(player, sprite);
}

/* Resolve a falling or descending player against terrain and present state. */
void PlayerResolveDescendingTerrain(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char sprite, direction;

    player_number = player->entity_index;
    if (TOEJAM_FALL_STATE[player_number])
        return;
    if ((player->animation_group_mask & PLAYER_GROUP_FALL) || (player->animation_group_mask & PLAYER_GROUP_LEAP)) {
        sprite = -1;
        direction = player->sprite_state & DIRECTION_MASK;
        if (land == TERRAIN_VOID)
            player->draw_flags &= ~ENTITY_SHADOW;
        else
            player->draw_flags |= ENTITY_SHADOW;
        if (gPlayer1Powerup[player_number] == PRESENT_SPRING_SHOES && player->velocity_height > 0) {
            if (player->height > 60 || !(P1_PROCESSED_INPUT[player_number] & 64)) {
                player->velocity_height = 0;
                player->acceleration_height = -64;
            }
        } else if (player->height < 4) {
            if (land >= TERRAIN_WATER) {
                if (land == TERRAIN_VOID)
                    PlayerStartFalling(player, player_number);
                else {
                    player->height = 0;
                    player->acceleration_height = 0;
                    player->velocity_height = 0;
                    if (direction & 2)
                        direction ^= 1;
                    sprite = direction + PLAYER_ANIM_FALL_SPLASH_UP;
                    player->draw_flags &= ~ENTITY_SHADOW;
                    PlaySoundEffect((long)Sfx_Splash, 50, 0);
                }
            } else if (gPlayer1Powerup[player_number] == PRESENT_SUPER_HITOPS) {
                player->height = 0;
                player->acceleration_height = 0;
                player->velocity_height = 0;
                if (P1_PROCESSED_INPUT[player_number] & 15)
                    sprite = direction + PLAYER_ANIM_HITOPS_UP;
                else
                    sprite = GetActivePresentSprite(player);
                player->draw_flags &= ~ENTITY_SHADOW;
            } else {
                TOEJAM_JUMP_COUNTER[player_number] = gPlayerLandingCounterSeed[player_number];
                player->acceleration_height = -16;
                TOEJAM_FALL_STATE[player_number] = 2;
            }
            if ((sprite & 0xfc) != 108) {
                player->velocity_x = 0;
                player->velocity_y = 0;
            }
        }
        if (sprite != -1)
            SetSprite(player, sprite);
    }
}

/* Advance the player's level after waiting for the associated object to finish. */
void PlayerAdvanceToNextLevel(player, player_number)
struct object_state *player;
short player_number;
{
    struct object_state *effect;

    PresentStartTransientEffect(player_number, player->x, player->y + 1, player->height + 9, player->level);
    effect = &gPlayerInfo[gPlayerTransientEffectEntities[player_number]];
    UpdateEntityViewportSprites(gPlayerTransientEffectEntities[player_number]);
    if (effect->draw_flags & ENTITY_PRIMARY_VIEW) {
        while (gPlayerTransientEffectEntities[player_number]) {
            if (PresentUpdateTransientEffect(player_number))
                player->behavior_flags |= ENTITY_HIDDEN;
            WaitForNextVBlank(2);
            UpdateAndQueueEntitySprites();
        }
        player->behavior_flags &= ~ENTITY_HIDDEN;
    }
    EarthlingReleaseLevelBank(player->level);
    MapReleaseLevelItemBank(player->level);
    ++player->level;
    MapLoadPlayerLevel(0, player->level);
    SetupElevator(player->level, 0);
    EarthlingRetainLevelBank(player->level);
    PlaceStuffInLevel(player->level);
    if (player->level > 1)
        PlayerSetPositionFromSavedMapCell(player);
    else
        SetPositionOnNewLevel(player_number);
    player->height = 70;
}

/* Bring the player back at the other player's position, rebuilding a split level if needed. */
void PlayerRelocateToPartnerOrNextLevel(player_number)
short player_number;
{
    struct object_state *player, *other_player;
    short terrain;

    player = &gPlayerInfo[player_number];
    gPlayerTerrainSinking[player_number] = 0;
    if (gPlayerLives[player_number ^ 1] >= 0)
        other_player = &gPlayerInfo[player_number ^ 1];
    else {
        if (player->level != gHighestLevel) {
            PlayerAdvanceToNextLevel(player, player_number);
            goto redraw;
        }
        other_player = player;
    }
    player->x = other_player->x;
    player->y = other_player->y;
    /* (long) view of animation_group_mask: the signed 32-bit test reproduces the ROM's D0 mask and allocation;
     * the stored field stays unsigned ([INFERENCE] the original spelling is unknown). */
    if ((long)other_player->animation_group_mask & (long)PLAYER_GROUP_STATIONARY)
        player->y += 10;
    if (!gElevatorDepartureInProgress) {
        if (ELEVATOR_FLAG[player_number ^ 1])
            player->y += 15;
        player->height = 70;
    } else
        player->height = 0;
    if (SPLIT_SCREEN && player->level != other_player->level) {
        EarthlingReleaseLevelBank(player->level);
        MapReleaseLevelItemBank(player->level);
        player->level = other_player->level;
        if (player_number == PLAYER_TOEJAM) {
            MapLoadPlayerLevel(0, gPlayerInfo[PLAYER_TOEJAM].level);
            SetupElevator(gPlayerInfo[PLAYER_TOEJAM].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_TOEJAM].level);
            PlaceStuffInLevel(gPlayerInfo[PLAYER_TOEJAM].level);
        } else {
            MapLoadPlayerLevel(1, gPlayerInfo[PLAYER_EARL].level);
            SetupElevator(gPlayerInfo[PLAYER_EARL].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_EARL].level);
            PlaceStuffInLevel(gPlayerInfo[PLAYER_EARL].level);
        }
    }
redraw:
    WaitForVBlankWithGraphicsPaused();
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    HudLoadLevelNumberSprites(player_number);
    UpdateScreenSplitMode(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
    UpdateLandAroundPlayer(player_number);
    if (!gElevatorDepartureInProgress) {
        SetSprite(player, PLAYER_ANIM_FALL_DOWN);
        player->velocity_x = 0;
        player->velocity_y = 0;
        player->velocity_height = -128;
        player->acceleration_height = -16;
        player->draw_flags |= ENTITY_SHADOW;
        terrain = GetMapTerrainAtWorldPosition(0, player->x, player->y);
        if (terrain != TERRAIN_VOID && terrain != TERRAIN_WATER) {
            TOEJAM_FALL_STATE[player_number] = 2;
            TOEJAM_JUMP_COUNTER[player_number] = 3;
        } else
            player->y += 20;
        PresentStartTransientEffect(player_number, player->x, player->y + 1, player->height, player->level);
    } else {
        StopPlayer(player);
        ELEVATOR_FLAG[player_number] = ELEVATOR_FLAG[player_number ^ 1];
        player->behavior_flags |= ENTITY_HIDDEN;
        SetSprite(player, PLAYER_ANIM_IDLE);
    }
}

/* Start the death animation, cancel a held powerup, and remember the previous sprite state. */
void KillThePlayer(player_number)
short player_number;
{
    struct object_state *player;

    if (P1_MENU_OPEN[player_number])
        return;
    player = &gPlayerInfo[player_number];
    StopPlayer(player);
    player->velocity_height = 256;
    player->animation_motion_axes = ANIMATION_MOTION_NONE;
    if (gPlayer1Powerup[player_number] == PRESENT_ROCKET_SKATES) {
        PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
        PlaySound(PSG_SILENCE, 98);
        gPlayer1Powerup[player_number] = PRESENT_NONE;
        HudLoadActionIconSprite(player_number);
        HudPositionActionIconSprite(player_number);
    }
    if (player->sprite_state == PLAYER_ANIM_SWIM_IDLE || player->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE || (player->animation_group_mask & PLAYER_GROUP_SWIM))
        player->height = -58;
    else
        player->height = -16;
    player->draw_flags &= 0x3f;
    gPlayerPreDeathSpriteState[player_number] = player->sprite_state;
    SetSprite(player, PLAYER_ANIM_GHOST);
    if (!gPlayerLives[player_number])
        player->behavior_flags |= ENTITY_LAST_LIFE_EXIT;
    TOEJAM_JUMP_COUNTER[player_number] = 0;
    gPlayerTerrainSinking[player_number] = 0;
    player->behavior_flags &= ~ENTITY_COLLIDABLE;
}

/* Remove a player whose last-life animation has left the screen, then restore the HUD. */
void PlayerFinalizeLastLifeExit(player, player_number)
struct object_state *player;
short player_number;
{
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    if (gPlayerLives[player_number ^ 1] < 0) {
        TerminateSpriteChainAtHead();
        ShowGameOverAndReset();
    }
    --gPlayerLives[player_number];
    EarthlingReleaseLevelBank(player->level);
    MapReleaseLevelItemBank(player->level);
    SPLIT_SCREEN = 0;
    SelectSpriteChainStart(0);
    player->level = 99;
    if (player->draw_flags & ENTITY_PRIMARY_VIEW)
        ReleaseEntityViewportSprites(player, 0);
    if (player->draw_flags & ENTITY_SECONDARY_VIEW)
        ReleaseEntityViewportSprites(player, 1);
    player->draw_flags = 0;
    gPlayerRank[player_number] = 9;
    HudHidePlayerSprites(player_number);
    player = &gPlayerInfo[player_number ^ 1];
    MapLoadPlayerLevel(0, player->level);
    UpdateScreenSplitMode(1);
    DrawHUD();
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
}

/* Complete death and return animations and age the temporary flag timer. */
void PlayerUpdateDeathAnimation(player, terrain)
struct object_state *player;
short terrain;
{
    short player_number, camera_y;
    int active_sprite;

    player_number = player->entity_index;
    if (player->sprite_state == PLAYER_ANIM_GHOST || player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST) {
        if (!SPLIT_SCREEN || !player_number)
            camera_y = TOEJAM_CAMERA_NEWY;
        else
            camera_y = EARL_CAMERA_NEWY;
        if (player->y - player->height - camera_y < 0) {
            player->velocity_height = 0;
            if (player->behavior_flags & ENTITY_LAST_LIFE_EXIT) {
                if (!P1_MENU_OPEN[player_number ^ 1])
                    PlayerFinalizeLastLifeExit(player, player_number);
                return;
            }
            player->height = 0;
            player->behavior_flags |= ENTITY_COLLIDABLE;
            if (gPlayerPreDeathSpriteState[player_number] == 2) {
                player->behavior_flags |= ENTITY_PLAYER_SUPPRESSED;
                ram_ffda2a[player_number] = 20;
            }
            /* Compiler-spelling qualification: assignment within the
             * argument retains the original D0-to-D1 result copy; a
             * separate assignment or direct call omits it. */
            if (terrain != TERRAIN_VOID)
                SetSprite(player, (active_sprite = GetActivePresentSprite(player)));
            else
                PlayerStartFalling(player, player_number);
        } else if (player->height < 0) {
            if (!--TOEJAM_JUMP_COUNTER[player_number])
                TOEJAM_JUMP_COUNTER[player_number] = 6;
            if (TOEJAM_JUMP_COUNTER[player_number] > 3)
                player->frame = 0;
            else
                player->frame = -1;
            player->sprite_ready = 0;
        } else if ((player->behavior_flags & ENTITY_LAST_LIFE_EXIT) && player->height >= 20) {
            if (player->sprite_state == PLAYER_ANIM_GHOST) {
                SetSprite(player, PLAYER_ANIM_LAST_LIFE_GHOST);
                player->velocity_height = 0;
                TOEJAM_JUMP_COUNTER[player_number] = 30;
                if (!player_number)
                    GenerateSpeech(1, "Bye Toejam", 1);
                else
                    GenerateSpeech(0, "Bye Earl", 1);
            } else if (!player->velocity_height && --TOEJAM_JUMP_COUNTER[player_number] <= 0) {
                SetSprite(player, PLAYER_ANIM_GHOST);
                player->sprite_state = PLAYER_ANIM_LAST_LIFE_GHOST;
                player->velocity_height = 256;
            }
        }
    }
    if (ram_ffda2a[player_number] && !--ram_ffda2a[player_number])
        player->behavior_flags &= ~ENTITY_PLAYER_SUPPRESSED;
}

/* Dispatch player-state handlers, then update common promotion and collision processing. */
void ProcessPlayers()
{
    unsigned short terrain;
    struct object_state *player;
    short player_number;

    terrain = LAND_UNDER_TOEJAM & TERRAIN_CLASS_MASK;
    player = gPlayerInfo;
    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL;
         terrain = LAND_UNDER_EARL & TERRAIN_CLASS_MASK, ++player, ++player_number) {
        if (gPlayerLives[player_number] < 0)
            PlayerProcessLifeRequest(player_number);
        else {
            if (P1_MENU_OPEN[player_number])
                continue;
            PlayerUpdateTeetering(player);
            PlayerUpdateHurtRecoveryAndIdle(player, terrain);
            PlayerUpdateSpringShoeJump(player, terrain);
            PlayerResolveDescendingTerrain(player, terrain);
            PlayerUpdatePhasedJumpMotion(player, terrain);
            PlayerUpdateRocketSkatesMotion(player, terrain);
            PlayerUpdateJumpLandingAnimation(player, terrain);
            PlayerUpdatePresentCompletionAndTerrainPose(player, terrain);
            PlayerUpdateTerrainSinking(player, terrain);
            PlayerUpdateDeathAnimation(player, terrain);
            PlayerUpdatePresentShooting(player, terrain);
            ProcessSleep(player);
            PlayerUpdateWhirlwindAndRecovery(player, terrain);
            HealthAndLivesChecks(player, terrain);
            if (ELEVATOR_FLAG[player_number] || (gDoorwayPlayerContactFlags[0] & 3) == player_number + 1 ||
                (gSecondDoorwayPlayerContactFlags & 3) == player_number + 1)
                player->behavior_flags |= ENTITY_MASK_PIXELS;
            else if (player->sprite_state != PLAYER_ANIM_ITEM_REACTION && player->sprite_state != PLAYER_ANIM_HOT_TUB_EXIT)
                player->behavior_flags &= ~ENTITY_MASK_PIXELS;
        }
        BummerLifePromo(player, terrain);
        PlayerUpdateControlRemapEffect(player);
        PresentUpdateTerrainEffects(player, terrain);
    }
    CheckPlayerFalling();
    if (PLAYER_INDICATOR == 2)
        PlayerUpdateHealthSharingClap();
}
