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
        (falling[0] && !TOEJAM_FALL_STATE[1]) ||
        (falling[1] && !TOEJAM_FALL_STATE[0])) {
        for (player_number = 0; player_number <= 1; ++player_number) {
            player = &gPlayerInfo[player_number];
            if (falling[player_number]) {
                player->tile |= 0x8000;
                player->height = 600;
                player->velocity_height = -640;
                player->acceleration_height = -16;
                player->draw_flags |= 0x40;
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
            MapLoadPlayerLevel(0, gPlayerInfo[0].level);
            SetupElevator(gPlayerInfo[0].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[0].level);
            PlaceStuffInLevel(gPlayerInfo[0].level);
        }
        if (falling[1]) {
            MapLoadPlayerLevel(1, gPlayerInfo[1].level);
            SetupElevator(gPlayerInfo[1].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[1].level);
            PlaceStuffInLevel(gPlayerInfo[1].level);
        }
        ClearSkipVHBlankFlags();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
        if (gPlayerInfo[0].level == gPlayerInfo[1].level ||
            gPlayerLives[0] < 0 || gPlayer2Lives < 0) {
            if (falling[0] && falling[1])
                SetPositionOnNewLevel(2);
            else if (falling[0]) {
                player = &gPlayerInfo[1];
                /* (long) view of flags_52: the signed 32-bit test reproduces the ROM's D0 mask and allocation;
                 * the stored field stays unsigned ([INFERENCE] the original spelling is unknown). */
                if (LAND_UNDER_EARL >= 0x80 ||
                    ((long)player->flags_52 & 0x20000000L) || player->sprite_state == 5 ||
                    player->sprite_state == 6 || gPlayer2Lives < 0)
                    SetPositionOnNewLevel(0);
                else {
                    gPlayerInfo[0].x = gPlayerInfo[1].x;
                    gPlayerInfo[0].y = gPlayerInfo[1].y;
                    if (ELEVATOR_FLAG[1])
                        gPlayerInfo[0].y += 12;
                }
            } else {
                player = &gPlayerInfo[0];
                if (LAND_UNDER_TOEJAM >= 0x80 ||
                    ((long)player->flags_52 & 0x20000000L) || player->sprite_state == 5 ||
                    player->sprite_state == 6 || gPlayerLives[0] < 0)
                    SetPositionOnNewLevel(1);
                else {
                    gPlayerInfo[1].x = gPlayerInfo[0].x;
                    gPlayerInfo[1].y = gPlayerInfo[0].y;
                    if (ELEVATOR_FLAG[0])
                        gPlayerInfo[1].y += 12;
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
        VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
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

    player_number = player->entity_number;
    if (!player->level || gPlayer1Powerup[player_number] == 1)
        return;
    sprite = -1;
    if (player->flags_52 & 0x80) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 9) {
            sprite = player->sprite_state + 4;
            player->height = 1;
        }
    } else if (player->flags_52 & 0x100) {
        if (!player->frame_duration) {
            StopPlayer(player);
            player->height = 35;
            player->draw_flags |= 64;
            sprite = player->sprite_state + 4;
            player->acceleration_height = -64;
        }
    } else if ((player->flags_52 & 0x200) && player->height < 4) {
        if (land == 0x80) {
            StopPlayer(player);
            player->height = 0;
            player->draw_flags &= ~64;
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

    player_number = player->entity_number;
    sprite = -1;
    if (player->flags_52 & 0x1000) {
        if (!player->frame_duration)
            sprite = GetActivePresentSprite(player);
    } else if ((player->flags_52 & 0x10400) && player->level &&
               !player->frame_duration) {
        if (gPlayer1Powerup[player_number] == 2)
            sprite = 77;
        else
            sprite = 3;
        player->velocity_x = 0;
        player->velocity_y = 0;
        player->mode_15 = 0;
    }
    if (player->sprite_state == 3) {
        for (row = 0; row < 5; ++row) {
            for (column = 0; column < 5; ++column) {
                if ((short)(player_number == 0 ? LAND_NEAR_TOEJAM[row][column] :
                            LAND_NEAR_EARL[row][column]) != 0x80) {
                    sprite = 7;
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

    player_number = player->entity_number;
    if (gPlayerInteractionChoiceActive[player_number] || (player->flags_52 & 0x20000) ||
        player->sprite_state == 79 || player->sprite_state == 78 ||
        player->sprite_state == 5 || player->sprite_state == 6)
        return;
    previous_height = player->height;
    if (land == 0x20 && player->height <= 0 && !(player->flags_52 & (unsigned long)0x60) &&
        !ELEVATOR_FLAG[player_number]) {
        if (!gPlayerTerrainSinking[player_number]) {
            gPlayerTerrainSinking[player_number] = 1;
            player->height = -3;
        }
        player->draw_flags |= 64;
        /* MATCHING QUALIFICATION: the counter goes through the otherwise unused
         * int `timer` because only that assignment reproduces the ROM's copy-
         * then-mask instruction order; masking TIME_COUNTER directly (as int or
         * unsigned long) schedules differently in both revisions. The historical
         * spelling is unknown. */
        if (!((timer = TIME_COUNTER) & (unsigned long)60)) {
            if ((player_number == 0 && player->height > -16) ||
                (player_number == 1 && player->height > -20))
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
        player->draw_flags &= ~64;
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

    player_number = player->entity_number;
    if (!player_number)
        nearby_land = &LAND_NEAR_TOEJAM[0][0];
    else
        nearby_land = &LAND_NEAR_EARL[0][0];
    if (((player->flags_52 & 0x2000) || player->sprite_state == 68) &&
        !player->frame_duration) {
        player->draw_flags &= ~64;
        TOEJAMS_HEALTH[player_number] -= gPlayerPendingHurtDamage[player_number];
        direction = player->sprite_state & 3;
        if (gPlayer1Powerup[player_number] == 5)
            OpenRocketSkates(player, player_number, land);
        else if (player->height > 4) {
            StopPlayer(player);
            player->mode_15 = 0;
            player->acceleration_height = -32;
            gPlayerLandingCounterSeed[player_number] = 2;
            if (player->sprite_state == 68)
                direction = 1;
            SetSprite(player, direction + 20);
        } else if (nearby_land[12] < 0x80)
            SetSprite(player, GetActivePresentSprite(player));
        else {
            if (nearby_land[11] < 0x80)
                player->x += 16;
            if (nearby_land[13] < 0x80)
                player->x -= 16;
            if (nearby_land[12] == 0x80) {
                if (player->sprite_state == 68)
                    direction = 1;
                else if (direction & 2)
                    direction ^= 1;
                SetSprite(player, direction + 64);
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
    if (player_number == 1) {
        if (!player->sprite_state && player->frame) {
            if ((GetRandomNumber() & 0x7ff) == 1)
                SetSprite(player, 88);
        } else if (player->sprite_state == 88 && !player->frame_duration) {
            SetSprite(player, 0);
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

    player_number = player->entity_number;
    if (gPlayer1Powerup[player_number] != 1 ||
        TOEJAM_FALL_STATE[player_number] || ELEVATOR_FLAG[player_number])
        return;
    sprite = -1;
    direction = player->sprite_state & 3;
    if (player->flags_52 & 0x4000) {
        if ((P1_PROCESSED_INPUT[player_number] & 64) &&
            (player->velocity_x || player->velocity_y)) {
            sprite = direction + 28;
            TOEJAM_JUMP_COUNTER[player_number] = 0;
        }
    } else if (player->flags_52 & 0x80) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 4) {
            if (player->velocity_x > 0)
                sprite = 35;
            else if (player->velocity_x < 0)
                sprite = 34;
            else
                sprite = player->sprite_state + 4;
            player->height = 1;
            PlaySoundEffect((long)Sfx_Springing, 50, 0);
        }
    } else if (player->flags_52 & 0x100) {
        if (!player->frame_duration) {
            player->height = 35;
            player->velocity_height = 256;
            player->acceleration_height = -4;
            player->velocity_x <<= 1;
            player->mode_15 = 0;
            player->draw_flags |= 64;
            sprite = direction + 20;
            if (sprite & 2)
                sprite ^= 1;
            gPlayerLandingCounterSeed[player_number] = 2;
            gPlayerTerrainSinking[player_number] = 0;
        }
    } else if (player->flags_52 & 4)
        sprite = direction + 56;
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

    player_number = player->entity_number;
    if (TOEJAM_FALL_STATE[player_number])
        return;
    if ((player->flags_52 & 0x20) || (player->flags_52 & 0x10000000)) {
        sprite = -1;
        direction = player->sprite_state & 3;
        if (land == 0x90)
            player->draw_flags &= ~64;
        else
            player->draw_flags |= 64;
        if (gPlayer1Powerup[player_number] == 1 && player->velocity_height > 0) {
            if (player->height > 60 || !(P1_PROCESSED_INPUT[player_number] & 64)) {
                player->velocity_height = 0;
                player->acceleration_height = -64;
            }
        } else if (player->height < 4) {
            if (land >= 0x80) {
                if (land == 0x90)
                    PlayerStartFalling(player, player_number);
                else {
                    player->height = 0;
                    player->acceleration_height = 0;
                    player->velocity_height = 0;
                    if (direction & 2)
                        direction ^= 1;
                    sprite = direction + 64;
                    player->draw_flags &= ~64;
                    PlaySoundEffect((long)Sfx_Splash, 50, 0);
                }
            } else if (gPlayer1Powerup[player_number] == 7) {
                player->height = 0;
                player->acceleration_height = 0;
                player->velocity_height = 0;
                if (P1_PROCESSED_INPUT[player_number] & 15)
                    sprite = direction + 108;
                else
                    sprite = GetActivePresentSprite(player);
                player->draw_flags &= ~64;
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
    if (effect->draw_flags & 2) {
        while (gPlayerTransientEffectEntities[player_number]) {
            if (PresentUpdateTransientEffect(player_number))
                player->flags_4d |= 64;
            WaitForNextVBlank(2);
            UpdateAndQueueEntitySprites();
        }
        player->flags_4d &= ~64;
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
    /* (long) view of flags_52: the signed 32-bit test reproduces the ROM's D0 mask and allocation;
     * the stored field stays unsigned ([INFERENCE] the original spelling is unknown). */
    if ((long)other_player->flags_52 & 0x20000000L)
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
        if (player_number == 0) {
            MapLoadPlayerLevel(0, gPlayerInfo[0].level);
            SetupElevator(gPlayerInfo[0].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[0].level);
            PlaceStuffInLevel(gPlayerInfo[0].level);
        } else {
            MapLoadPlayerLevel(1, gPlayerInfo[1].level);
            SetupElevator(gPlayerInfo[1].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[1].level);
            PlaceStuffInLevel(gPlayerInfo[1].level);
        }
    }
redraw:
    WaitForVBlankWithGraphicsPaused();
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    HudLoadLevelNumberSprites(player_number);
    UpdateScreenSplitMode(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
    UpdateLandAroundPlayer(player_number);
    if (!gElevatorDepartureInProgress) {
        SetSprite(player, 21);
        player->velocity_x = 0;
        player->velocity_y = 0;
        player->velocity_height = -128;
        player->acceleration_height = -16;
        player->draw_flags |= 64;
        terrain = GetMapTerrainAtWorldPosition(0, player->x, player->y);
        if (terrain != 0x90 && terrain != 0x80) {
            TOEJAM_FALL_STATE[player_number] = 2;
            TOEJAM_JUMP_COUNTER[player_number] = 3;
        } else
            player->y += 20;
        PresentStartTransientEffect(player_number, player->x, player->y + 1, player->height, player->level);
    } else {
        StopPlayer(player);
        ELEVATOR_FLAG[player_number] = ELEVATOR_FLAG[player_number ^ 1];
        player->flags_4d |= 64;
        SetSprite(player, 0);
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
    player->mode_15 = 0;
    if (gPlayer1Powerup[player_number] == 5) {
        PlaySound(6, 99);
        PlaySound(0, 98);
        gPlayer1Powerup[player_number] = -1;
        HudLoadActionIconSprite(player_number);
        HudPositionActionIconSprite(player_number);
    }
    if (player->sprite_state == 3 || player->sprite_state == 7 || (player->flags_52 & 0x800))
        player->height = -58;
    else
        player->height = -16;
    player->draw_flags &= 0x3f;
    gPlayerPreDeathSpriteState[player_number] = player->sprite_state;
    SetSprite(player, 5);
    if (!gPlayerLives[player_number])
        player->flags_4d |= 128;
    TOEJAM_JUMP_COUNTER[player_number] = 0;
    gPlayerTerrainSinking[player_number] = 0;
    player->flags_4d &= ~1;
}

/* Remove a player whose last-life animation has left the screen, then restore the HUD. */
void PlayerFinalizeLastLifeExit(player, player_number)
struct object_state *player;
short player_number;
{
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
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
    if (player->draw_flags & 2)
        ReleaseEntityViewportSprites(player, 0);
    if (player->draw_flags & 4)
        ReleaseEntityViewportSprites(player, 1);
    player->draw_flags = 0;
    gPlayerRank[player_number] = 9;
    HudHidePlayerSprites(player_number);
    player = &gPlayerInfo[player_number ^ 1];
    MapLoadPlayerLevel(0, player->level);
    UpdateScreenSplitMode(1);
    DrawHUD();
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
}

/* Complete death and return animations and age the temporary flag timer. */
void PlayerUpdateDeathAnimation(player, terrain)
struct object_state *player;
short terrain;
{
    short player_number, camera_y;
    int active_sprite;

    player_number = player->entity_number;
    if (player->sprite_state == 5 || player->sprite_state == 6) {
        if (!SPLIT_SCREEN || !player_number)
            camera_y = TOEJAM_CAMERA_NEWY;
        else
            camera_y = EARL_CAMERA_NEWY;
        if (player->y - player->height - camera_y < 0) {
            player->velocity_height = 0;
            if (player->flags_4d & 128) {
                if (!P1_MENU_OPEN[player_number ^ 1])
                    PlayerFinalizeLastLifeExit(player, player_number);
                return;
            }
            player->height = 0;
            player->flags_4d |= 1;
            if (gPlayerPreDeathSpriteState[player_number] == 2) {
                player->flags_4d |= 16;
                ram_ffda2a[player_number] = 20;
            }
            /* Compiler-spelling qualification: assignment within the
             * argument retains the original D0-to-D1 result copy; a
             * separate assignment or direct call omits it. */
            if (terrain != 0x90)
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
        } else if ((player->flags_4d & 128) && player->height >= 20) {
            if (player->sprite_state == 5) {
                SetSprite(player, 6);
                player->velocity_height = 0;
                TOEJAM_JUMP_COUNTER[player_number] = 30;
                if (!player_number)
                    GenerateSpeech(1, "Bye Toejam", 1);
                else
                    GenerateSpeech(0, "Bye Earl", 1);
            } else if (!player->velocity_height && --TOEJAM_JUMP_COUNTER[player_number] <= 0) {
                SetSprite(player, 5);
                player->sprite_state = 6;
                player->velocity_height = 256;
            }
        }
    }
    if (ram_ffda2a[player_number] && !--ram_ffda2a[player_number])
        player->flags_4d &= ~16;
}

/* Dispatch player-state handlers, then update common promotion and collision processing. */
void ProcessPlayers()
{
    unsigned short terrain;
    struct object_state *player;
    short player_number;

    terrain = LAND_UNDER_TOEJAM & 0xf0;
    player = gPlayerInfo;
    for (player_number = 0; player_number <= 1;
         terrain = LAND_UNDER_EARL & 0xf0, ++player, ++player_number) {
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
                player->flags_4d |= 32;
            else if (player->sprite_state != 70 && player->sprite_state != 120)
                player->flags_4d &= ~32;
        }
        BummerLifePromo(player, terrain);
        PlayerUpdateControlRemapEffect(player);
        PresentUpdateTerrainEffects(player, terrain);
    }
    CheckPlayerFalling();
    if (PLAYER_INDICATOR == 2)
        PlayerUpdateHealthSharingClap();
}
