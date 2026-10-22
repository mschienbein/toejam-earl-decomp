#include "player_animation.h"
#include "player_animation_groups.h"
#include "present_types.h"
#include "terrain.h"
#include "entities.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"

/* Select the player's idle sprite from the active present and terrain,
 * resetting the animation and horizontal velocity for a new selection. */
int GetActivePresentSprite(player)
struct object_state *player;
{
    register unsigned short land;
    short player_number;
    char sprite;

    player_number = player->entity_index;
    land = player_number == PLAYER_TOEJAM ? gPlayer1TerrainUnderfoot : gPlayer2TerrainUnderfoot;
    land &= TERRAIN_CLASS_MASK;
    if (gPlayerPowerups[player_number] == PRESENT_INNERTUBE) {
        if (land == TERRAIN_WATER)
            sprite = PLAYER_ANIM_INNERTUBE_WATER_IDLE;
        else if (!gPlayerElevatorFlags[player_number])
            sprite = PLAYER_ANIM_INNERTUBE_IDLE;
        else {
            sprite = PLAYER_ANIM_IDLE;
            player->frame = -1;
        }
    } else if (gPlayerPowerups[player_number] == PRESENT_ROCKET_SKATES) {
        OpenRocketSkates(player, player_number, land);
        sprite = -1;
        return sprite;
    } else if (land == TERRAIN_WATER && !player->height)
        sprite = PLAYER_ANIM_SWIM_IDLE;
    else if (gPlayerPowerups[player_number] == PRESENT_SPRING_SHOES && !gPlayerElevatorFlags[player_number])
        sprite = PLAYER_ANIM_SPRING_SHOES_DOWN;
    else if (!gPlayerPowerups[player_number] && !gPlayerElevatorFlags[player_number]) {
        if ((player->animation_group_mask & PLAYER_GROUP_ICARUS_FLY) && player->height > 0) {
            sprite = -1;
            return sprite;
        } else {
            player->draw_flags |= ENTITY_SHADOW;
            sprite = PLAYER_ANIM_ICARUS_IDLE;
        }
    } else {
        sprite = PLAYER_ANIM_IDLE;
        player->frame = -1;
    }
    player->velocity_x = 0;
    player->velocity_y = 0;
    if (sprite != PLAYER_ANIM_ICARUS_IDLE)
        player->draw_flags &= ~ENTITY_SHADOW;
    return sprite;
}


#include "entities.h"
#include "ram_player.h"
#include "ram_camera.h"
#include "ram_system.h"
#include "assets_map.h"
#include "hardware.h"
#include "player.h"

/* Clear tile priority around the falling player, stopping each column
 * after a low-property tile below the player's row. */
void MapClearTilePriorityNearPlayer()
{
    short viewport, column, center_row, bottom_row, right_column, top_row;
    short row, wrapped_row, wrapped_column;
    unsigned short tile;
    short first_column;
    short player_x, player_y;
    unsigned char property;
    struct object_state *player;

    player = &gPlayerInfo[gTerrainPriorityUpdatePlayer];
    player_x = player->x;
    player_y = player->y;
    center_row = player_y >> 3;
    top_row = center_row - 6;
    bottom_row = center_row + 6;
    first_column = (player_x >> 3) - 4;
    right_column = first_column + 9;
    viewport = 0;
    if (gTerrainPriorityUpdatePlayer == 1 && gSplitScreenEnabled)
        viewport = 1;
    VDP_CONTROL_WORD = 0x8c81;
    for (column = first_column; column <= right_column; ++column) {
        for (row = top_row; row <= bottom_row; ++row) {
            wrapped_row = row & 31;
            wrapped_column = column & 63;
            if (!viewport)
                VDP_COMMAND = ((wrapped_row << 23) + 0x20000002L) + (wrapped_column << 17);
            else
                VDP_COMMAND = ((wrapped_row << 23) + 3L) + (wrapped_column << 17);
            tile = VDP_DATA;
            if (row > center_row) {
                property = PlayerTilePropertyLookup[tile & 0x7ff];
                if (property < 0x90)
                    break;
            }
            tile &= 0x7fff;
            if (!viewport)
                VDP_COMMAND = ((wrapped_row << 23) + 0x60000002L) + (wrapped_column << 17);
            else
                VDP_COMMAND = ((wrapped_row << 23) + 0x40000003L) + (wrapped_column << 17);
            VDP_DATA = tile;
        }
    }
    gTerrainPriorityUpdatePending = 0;
}


#include "player_animation.h"
#include "player_animation_groups.h"
#include "terrain.h"
#include "entities.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "speech.h"
#include "sprite_calls.h"

/* Start falling in the current facing direction, nudging away from a
 * neighboring supported tile before the vblank terrain-priority update. */
void PlayerStartFalling(player, player_number)
struct object_state *player;
short player_number;
{
    unsigned char *nearby_land;
    char sprite;
    short center_tile;

    nearby_land = player_number == PLAYER_TOEJAM ? &gPlayer1NearbyTerrain[0][0] : &gPlayer2NearbyTerrain[0][0];
    gPlayerFallStates[player_number] = 1;
    sprite = (player->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_FALL_UP;
    center_tile = 12;
    if (sprite == PLAYER_ANIM_FALL_UP || sprite == PLAYER_ANIM_FALL_DOWN) {
        if (nearby_land[center_tile - 1] < TERRAIN_WATER)
            player->x += 8;
        else if (nearby_land[center_tile + 1] < TERRAIN_WATER)
            player->x -= 8;
    }
    player->acceleration_height = -128;
    SetSprite(player, sprite);
    player->animation_motion_axes = ANIMATION_MOTION_NONE;
    player->tile &= ~VDP_TILE_PRIORITY;
    gTerrainPriorityUpdatePlayer = player_number;
    gTerrainPriorityUpdatePending = 1;
    while (gTerrainPriorityUpdatePending)
        ;
    GenerateSpeech(player_number, "Bummer", 0);
    PlaySoundEffect((long)Sfx_Falling, AUDIO_PRIORITY_NORMAL, 0);
}

/* Launch a short directional fall and remember that it has started. */
void PlayerStartTeeteringFall(player)
struct object_state *player;
{
    short player_number;
    char sprite;

    player_number = player->entity_index;
    player->height = 5;
    player->velocity_height = 512;
    player->acceleration_height = -80;
    player->animation_motion_axes = ANIMATION_MOTION_NONE;
    sprite = (player->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_FALL_UP;
    gPlayerLandingCounterSeed[player_number] = 1;
    player->velocity_x = 0;
    player->velocity_y = 0;
    if (sprite == PLAYER_ANIM_FALL_UP)
        player->velocity_y = -256;
    else if (sprite == PLAYER_ANIM_FALL_DOWN)
        player->velocity_y = 256;
    else if (sprite == PLAYER_ANIM_FALL_LEFT)
        player->velocity_x = -512;
    else if (sprite == PLAYER_ANIM_FALL_RIGHT)
        player->velocity_x = 512;
    SetSprite(player, sprite);
}

/* Count the teetering frames and play the observed randomized speech sounds.
 * After thirty frames, start the short fall unless the shared flag delays it. */
void PlayerUpdateTeetering(player)
struct object_state *player;
{
    short player_number;
    unsigned short sound_choice;

    player_number = player->entity_index;
    if (player->animation_group_mask & PLAYER_GROUP_LEAN) {
        if (++gPlayerJumpCounters[player_number] > 30) {
            if (gElevatorDepartureInProgress)
                --gPlayerJumpCounters[player_number];
            else
                PlayerStartTeeteringFall(player);
        } else if (gPlayerJumpCounters[player_number] == 1) {
            if (gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1)
                PlaySoundEffect((long)Sfx_Teetering1, 86, 0);
            else
                PlaySoundEffect((long)Sfx_Teetering2, 86, 0);
        } else if (!gPcmEffectCooldownTicks) {
            sound_choice = gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 15;
            if (sound_choice == 0)
                PlaySoundEffect((long)Sfx_Teetering1, 20, 0);
            else if (sound_choice == 1)
                PlaySoundEffect((long)Sfx_Teetering2, 20, 0);
            else if (sound_choice == 2)
                PlaySoundEffect((long)Sfx_Teetering3, 20, 0);
        }
    }
}


#include "entities.h"
#include "earthlings.h"
#include "ram_player.h"
#include "ram_effects.h"
#include "ram_ui.h"
#include "ram_camera.h"
#include "ram_system.h"
#include "ram_elevator.h"
#include "assets_audio.h"
#include "player.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "sprite_calls.h"
#include "audio_calls.h"
#include "speech.h"
#include "random.h"

/* Advance both falling players, handle their landing animation, and test
 * whether the falling player overlaps the other player's position. */
/* MATCHING QUALIFICATION: one char sprite ID is deliberately shared by the
 * current-player and partner selections. These lifetimes do not overlap: the
 * partner decision initializes it after the early current-player update, and
 * the last current-player update follows the partner update. This retains the
 * original byte-promotion sequence; independent result locals normalize
 * differently. The canonical int GetActivePresentSprite interface is unchanged.
 * This is a meaningful scalar-role reuse, not an unused value or fake ABI. */
void CheckPlayerFalling()
{
  char falling[PLAYER_COUNT];
  short player_number;
  long other_number;
  struct object_state *player;
  struct object_state *other;
  unsigned short land;
  char direction;
  char selected_sprite;
  falling[0] = 0;
  falling[1] = 0;
  for (player_number = 0; player_number <= PLAYER_EARL; ++player_number)
  {
    if (gPlayerLives[player_number] < 0)
      continue;
    player = &gPlayerInfo[player_number];
    other_number = (unsigned long) (player_number ^ 1);
    other = &gPlayerInfo[other_number];
    if (gPlayerFallStates[player_number] == 1)
    {
      if (player->height < (-224))
      {
        player->velocity_height = 0;
        player->acceleration_height = 0;
        falling[player_number] = 1;
        if (gPlayerPowerups[player_number] == PRESENT_ROCKET_SKATES)
          gPlayerJumpCounters[player_number] = 1;
        else
          gPlayerJumpCounters[player_number] = 3;
      }
      else
        player->acceleration_height = -128;
    }
    else
      if (gPlayerFallStates[player_number] == 2)
    {
      if (player_number == 0)
        land = gPlayer1TerrainUnderfoot;
      else
        land = gPlayer2TerrainUnderfoot;
      direction = player->sprite_state & DIRECTION_MASK;
      if (land >= TERRAIN_WATER)
      {
        if (!(player->animation_group_mask & PLAYER_GROUP_FALL))
        {
          SetSprite(player, direction + PLAYER_ANIM_FALL_UP);
          player->velocity_height = 0;
          player->acceleration_height = -128;
        }
        gPlayerFallStates[player_number] = 0;
        continue;
      }
      if (gPlayerJumpCounters[player_number] <= 0)
      {
        player->height = 0;
        player->velocity_height = 0;
        player->acceleration_height = 0;
        gPlayerFallStates[player_number] = 0;
        selected_sprite = GetActivePresentSprite(player);
        SetSprite(player, selected_sprite);
      }
      else
        if ((player->velocity_height < 0) && (player->height < 4))
      {
        SetSprite(player, direction + PLAYER_ANIM_LAND_UP);
        player->height = 0;
        player->velocity_height = 0;
        player->acceleration_height = 0;
      }
      else
        if (!player->frame_duration)
      {
        short jump_count;
        SetSprite(player, direction + PLAYER_ANIM_FALL_UP);
        player->velocity_height = 512;
        player->acceleration_height = -64;
        jump_count = gPlayerJumpCounters[player_number];
        player->velocity_height += jump_count << 8;
        --gPlayerJumpCounters[player_number];
      }
      selected_sprite = -1;
      if (((((((other->sprite_state == PLAYER_ANIM_IDLE) ||
        (other->sprite_state == PLAYER_ANIM_ICARUS_IDLE)) ||
        (other->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY))) ||
        (other->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE)) ||
        (other->sprite_state == PLAYER_ANIM_TOMATO_RAIN)) ||
        ((other->sprite_state == PLAYER_ANIM_SPRING_SHOES_DOWN) &&
        (!other->velocity_y))) &&
        EntityIsNearPosition(other, player->x, player->y, player->level))
      {
        if (player->height < 20)
        {
          selected_sprite = PLAYER_ANIM_FLATTENED;
          other->draw_flags |= ENTITY_DRAW_LATE;
          PlaySoundEffect((long) Sfx_Squish, AUDIO_PRIORITY_NORMAL, 0);
        }
        else
          if (((other->sprite_state != PLAYER_ANIM_TOMATO_RAIN) && (!(other->animation_group_mask & PLAYER_GROUP_STATIONARY))) && (player->height < 300))
          selected_sprite = PLAYER_ANIM_TOMATO_RAIN;
      }
      else
        if (other->sprite_state == PLAYER_ANIM_TOMATO_RAIN)
        selected_sprite = GetActivePresentSprite(other);
      if (selected_sprite != (-1))
      {
        SetSprite(other, selected_sprite);
        other->velocity_x = 0;
        other->velocity_y = 0;
      }
    }
    if ((player->sprite_state == PLAYER_ANIM_FLATTENED) && (!player->frame_duration))
    {
      selected_sprite = GetActivePresentSprite(player);
      SetSprite(player, selected_sprite);
      player->draw_flags &= ~ENTITY_DRAW_LATE;
      player->behavior_flags |= ENTITY_PLAYER_SUPPRESSED;
      gPlayerRespawnSuppressionTicks[player_number] = 20;
    }
  }

  MakePlayerFall(falling);
}




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
        (falling[0] && !gPlayerFallStates[PLAYER_EARL]) ||
        (falling[1] && !gPlayerFallStates[PLAYER_TOEJAM])) {
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
                gPlayerFallStates[player_number] = 2;
            }
            if (gPlayerLives[player_number] < 0)
                ElevatorInvalidateSecondaryRecords();
        }
        if (falling[0]) {
            MapLoadPlayerLevel(0, gPlayerInfo[PLAYER_TOEJAM].level);
            SetupElevator(gPlayerInfo[PLAYER_TOEJAM].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_TOEJAM].level);
            LoadLevelItemBank(gPlayerInfo[PLAYER_TOEJAM].level);
        }
        if (falling[1]) {
            MapLoadPlayerLevel(1, gPlayerInfo[PLAYER_EARL].level);
            SetupElevator(gPlayerInfo[PLAYER_EARL].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_EARL].level);
            LoadLevelItemBank(gPlayerInfo[PLAYER_EARL].level);
        }
        DisableBlankingGraphics();
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
                if (gPlayer2TerrainUnderfoot >= TERRAIN_WATER ||
                    ((long)player->animation_group_mask & (long)PLAYER_GROUP_STATIONARY) || player->sprite_state == PLAYER_ANIM_GHOST ||
                    player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST || gPlayer2Lives < 0)
                    SetPositionOnNewLevel(0);
                else {
                    gPlayerInfo[PLAYER_TOEJAM].x = gPlayerInfo[PLAYER_EARL].x;
                    gPlayerInfo[PLAYER_TOEJAM].y = gPlayerInfo[PLAYER_EARL].y;
                    if (gPlayerElevatorFlags[PLAYER_EARL])
                        gPlayerInfo[PLAYER_TOEJAM].y += 12;
                }
            } else {
                player = &gPlayerInfo[PLAYER_TOEJAM];
                if (gPlayer1TerrainUnderfoot >= TERRAIN_WATER ||
                    ((long)player->animation_group_mask & (long)PLAYER_GROUP_STATIONARY) || player->sprite_state == PLAYER_ANIM_GHOST ||
                    player->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST || gPlayerLives[PLAYER_TOEJAM] < 0)
                    SetPositionOnNewLevel(1);
                else {
                    gPlayerInfo[PLAYER_EARL].x = gPlayerInfo[PLAYER_TOEJAM].x;
                    gPlayerInfo[PLAYER_EARL].y = gPlayerInfo[PLAYER_TOEJAM].y;
                    if (gPlayerElevatorFlags[PLAYER_TOEJAM])
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
        EnableBlankingGraphics();
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
    if (!player->level || gPlayerPowerups[player_number] == PRESENT_SPRING_SHOES)
        return;
    sprite = -1;
    if (player->animation_group_mask & PLAYER_GROUP_JUMP_WINDUP) {
        if (++gPlayerJumpCounters[player_number] > 9) {
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
            PlaySoundEffect((long)Sfx_Splash, AUDIO_PRIORITY_NORMAL, 0);
        } else {
            gPlayerJumpCounters[player_number] = 1;
            gPlayerFallStates[player_number] = 2;
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
        if (gPlayerPowerups[player_number] == PRESENT_INNERTUBE)
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
                if ((short)(player_number == PLAYER_TOEJAM ? gPlayer1NearbyTerrain[row][column] :
                            gPlayer2NearbyTerrain[row][column]) != 0x80) {
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
    if (land == TERRAIN_SINKING && player->height <= 0 && !(player->animation_group_mask & (unsigned long)(PLAYER_GROUP_FALL | PLAYER_GROUP_LAND)) &&
        !gPlayerElevatorFlags[player_number]) {
        if (!gPlayerTerrainSinking[player_number]) {
            gPlayerTerrainSinking[player_number] = 1;
            player->height = -3;
        }
        player->draw_flags |= ENTITY_SHADOW;
        /* MATCHING QUALIFICATION: the counter goes through the otherwise unused
         * int `timer` because only that assignment reproduces the ROM's copy-
         * then-mask instruction order; masking gFrameCounter directly (as int or
         * unsigned long) schedules differently in both revisions. The historical
         * spelling is unknown. */
        if (!((timer = gFrameCounter) & (unsigned long)60)) {
            if ((player_number == PLAYER_TOEJAM && player->height > -16) ||
                (player_number == PLAYER_EARL && player->height > -20))
                --player->height;
            else if (!gSinkingHelpSpeechDelay) {
                if (!gSplitScreenEnabled && gPlayerLives[player_number ^ 1] >= 0) {
                    if (gCachedRandomValues[gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) :
                                      ++gRandomCacheIndex] & 1) {
                        GenerateSpeech(player_number, "Help", 1);
                        /* MATCHING QUALIFICATION: 256 deliberately truncates to zero
                         * in this byte store, selecting the observed MOVE.B
                         * instead of CLR.B. The historical literal is unknown;
                         * no original bug or intended cooldown is asserted. */
                        gSinkingHelpSpeechDelay = 256;
                    }
                }
            } else
                --gSinkingHelpSpeechDelay;
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

/* Complete hurt recovery, select the landing or splash pose, and vary Earl's idle. */
void PlayerUpdateHurtRecoveryAndIdle(player, land)
struct object_state *player;
unsigned short land;
{
    short player_number;
    char direction;
    unsigned char *nearby_land;

    player_number = player->entity_index;
    if (!player_number)
        nearby_land = &gPlayer1NearbyTerrain[0][0];
    else
        nearby_land = &gPlayer2NearbyTerrain[0][0];
    if (((player->animation_group_mask & PLAYER_GROUP_HURT) || player->sprite_state == PLAYER_ANIM_ZAPPED) &&
        !player->frame_duration) {
        player->draw_flags &= ~ENTITY_SHADOW;
        gPlayerHealth[player_number] -= gPlayerPendingHurtDamage[player_number];
        direction = player->sprite_state & DIRECTION_MASK;
        if (gPlayerPowerups[player_number] == PRESENT_ROCKET_SKATES)
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
                PlaySoundEffect((long)Sfx_Splash, AUDIO_PRIORITY_NORMAL, 0);
            } else {
                if (gPlayerHealth[player_number] <= 0)
                    gPlayerHealth[player_number] = 1;
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
    if (gPlayerPowerups[player_number] != PRESENT_SPRING_SHOES ||
        gPlayerFallStates[player_number] || gPlayerElevatorFlags[player_number])
        return;
    sprite = -1;
    direction = player->sprite_state & DIRECTION_MASK;
    if (player->animation_group_mask & PLAYER_GROUP_SPRING_SHOES) {
        if ((gPlayerProcessedInput[player_number] & INPUT_BUTTON_A) &&
            (player->velocity_x || player->velocity_y)) {
            sprite = direction + PLAYER_ANIM_JUMP_WINDUP_UP;
            gPlayerJumpCounters[player_number] = 0;
        }
    } else if (player->animation_group_mask & PLAYER_GROUP_JUMP_WINDUP) {
        if (++gPlayerJumpCounters[player_number] > 4) {
            if (player->velocity_x > 0)
                sprite = PLAYER_ANIM_JUMP_TAKEOFF_RIGHT;
            else if (player->velocity_x < 0)
                sprite = PLAYER_ANIM_JUMP_TAKEOFF_LEFT;
            else
                sprite = player->sprite_state + 4;
            player->height = 1;
            PlaySoundEffect((long)Sfx_Springing, AUDIO_PRIORITY_NORMAL, 0);
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
    if (gPlayerFallStates[player_number])
        return;
    if ((player->animation_group_mask & PLAYER_GROUP_FALL) || (player->animation_group_mask & PLAYER_GROUP_LEAP)) {
        sprite = -1;
        direction = player->sprite_state & DIRECTION_MASK;
        if (land == TERRAIN_VOID)
            player->draw_flags &= ~ENTITY_SHADOW;
        else
            player->draw_flags |= ENTITY_SHADOW;
        if (gPlayerPowerups[player_number] == PRESENT_SPRING_SHOES && player->velocity_height > 0) {
            if (player->height > 60 || !(gPlayerProcessedInput[player_number] & INPUT_BUTTON_A)) {
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
                    PlaySoundEffect((long)Sfx_Splash, AUDIO_PRIORITY_NORMAL, 0);
                }
            } else if (gPlayerPowerups[player_number] == PRESENT_SUPER_HITOPS) {
                player->height = 0;
                player->acceleration_height = 0;
                player->velocity_height = 0;
                if (gPlayerProcessedInput[player_number] & INPUT_DIRECTION_MASK)
                    sprite = direction + PLAYER_ANIM_HITOPS_UP;
                else
                    sprite = GetActivePresentSprite(player);
                player->draw_flags &= ~ENTITY_SHADOW;
            } else {
                gPlayerJumpCounters[player_number] = gPlayerLandingCounterSeed[player_number];
                player->acceleration_height = -16;
                gPlayerFallStates[player_number] = 2;
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
    LoadLevelItemBank(player->level);
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
        if (gPlayerElevatorFlags[player_number ^ 1])
            player->y += 15;
        player->height = 70;
    } else
        player->height = 0;
    if (gSplitScreenEnabled && player->level != other_player->level) {
        EarthlingReleaseLevelBank(player->level);
        MapReleaseLevelItemBank(player->level);
        player->level = other_player->level;
        if (player_number == PLAYER_TOEJAM) {
            MapLoadPlayerLevel(0, gPlayerInfo[PLAYER_TOEJAM].level);
            SetupElevator(gPlayerInfo[PLAYER_TOEJAM].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_TOEJAM].level);
            LoadLevelItemBank(gPlayerInfo[PLAYER_TOEJAM].level);
        } else {
            MapLoadPlayerLevel(1, gPlayerInfo[PLAYER_EARL].level);
            SetupElevator(gPlayerInfo[PLAYER_EARL].level, 0);
            EarthlingRetainLevelBank(gPlayerInfo[PLAYER_EARL].level);
            LoadLevelItemBank(gPlayerInfo[PLAYER_EARL].level);
        }
    }
redraw:
    WaitForVBlankWithGraphicsPaused();
    DisableBlankingGraphics();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    HudLoadLevelNumberSprites(player_number);
    UpdateScreenSplitMode(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    EnableBlankingGraphics();
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
            gPlayerFallStates[player_number] = 2;
            gPlayerJumpCounters[player_number] = 3;
        } else
            player->y += 20;
        PresentStartTransientEffect(player_number, player->x, player->y + 1, player->height, player->level);
    } else {
        StopPlayer(player);
        gPlayerElevatorFlags[player_number] = gPlayerElevatorFlags[player_number ^ 1];
        player->behavior_flags |= ENTITY_HIDDEN;
        SetSprite(player, PLAYER_ANIM_IDLE);
    }
}

/* Start the death animation, cancel a held powerup, and remember the previous sprite state. */
void KillThePlayer(player_number)
short player_number;
{
    struct object_state *player;

    if (gPlayerInventoryOpen[player_number])
        return;
    player = &gPlayerInfo[player_number];
    StopPlayer(player);
    player->velocity_height = 256;
    player->animation_motion_axes = ANIMATION_MOTION_NONE;
    if (gPlayerPowerups[player_number] == PRESENT_ROCKET_SKATES) {
        PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
        PlaySound(PSG_SILENCE, 98);
        gPlayerPowerups[player_number] = PRESENT_NONE;
        HudLoadActionIconSprite(player_number);
        HudPositionActionIconSprite(player_number);
    }
    if (player->sprite_state == PLAYER_ANIM_SWIM_IDLE || player->sprite_state == PLAYER_ANIM_SWIM_NEAR_SHORE || (player->animation_group_mask & PLAYER_GROUP_SWIM))
        player->height = -58;
    else
        player->height = -16;
    player->draw_flags &= ~(ENTITY_SHADOW | ENTITY_DRAW_LATE);
    gPlayerPreDeathSpriteState[player_number] = player->sprite_state;
    SetSprite(player, PLAYER_ANIM_GHOST);
    if (!gPlayerLives[player_number])
        player->behavior_flags |= ENTITY_LAST_LIFE_EXIT;
    gPlayerJumpCounters[player_number] = 0;
    gPlayerTerrainSinking[player_number] = 0;
    player->behavior_flags &= ~ENTITY_COLLIDABLE;
}

/* Remove a player whose last-life animation has left the screen, then restore the HUD. */
void PlayerFinalizeLastLifeExit(player, player_number)
struct object_state *player;
short player_number;
{
    DisableBlankingGraphics();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    if (gPlayerLives[player_number ^ 1] < 0) {
        TerminateSpriteChainAtHead();
        ShowGameOverAndReset();
    }
    --gPlayerLives[player_number];
    EarthlingReleaseLevelBank(player->level);
    MapReleaseLevelItemBank(player->level);
    gSplitScreenEnabled = 0;
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
    EnableBlankingGraphics();
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
        if (!gSplitScreenEnabled || !player_number)
            camera_y = gCamera1PendingY;
        else
            camera_y = gCamera2PendingY;
        if (player->y - player->height - camera_y < 0) {
            player->velocity_height = 0;
            if (player->behavior_flags & ENTITY_LAST_LIFE_EXIT) {
                if (!gPlayerInventoryOpen[player_number ^ 1])
                    PlayerFinalizeLastLifeExit(player, player_number);
                return;
            }
            player->height = 0;
            player->behavior_flags |= ENTITY_COLLIDABLE;
            if (gPlayerPreDeathSpriteState[player_number] == 2) {
                player->behavior_flags |= ENTITY_PLAYER_SUPPRESSED;
                gPlayerRespawnSuppressionTicks[player_number] = 20;
            }
            /* MATCHING QUALIFICATION: assignment within the
             * argument retains the original D0-to-D1 result copy; a
             * separate assignment or direct call omits it. */
            if (terrain != TERRAIN_VOID)
                SetSprite(player, (active_sprite = GetActivePresentSprite(player)));
            else
                PlayerStartFalling(player, player_number);
        } else if (player->height < 0) {
            if (!--gPlayerJumpCounters[player_number])
                gPlayerJumpCounters[player_number] = 6;
            if (gPlayerJumpCounters[player_number] > 3)
                player->frame = 0;
            else
                player->frame = -1;
            player->sprite_ready = 0;
        } else if ((player->behavior_flags & ENTITY_LAST_LIFE_EXIT) && player->height >= 20) {
            if (player->sprite_state == PLAYER_ANIM_GHOST) {
                SetSprite(player, PLAYER_ANIM_LAST_LIFE_GHOST);
                player->velocity_height = 0;
                gPlayerJumpCounters[player_number] = 30;
                if (!player_number)
                    GenerateSpeech(1, "Bye Toejam", 1);
                else
                    GenerateSpeech(0, "Bye Earl", 1);
            } else if (!player->velocity_height && --gPlayerJumpCounters[player_number] <= 0) {
                SetSprite(player, PLAYER_ANIM_GHOST);
                player->sprite_state = PLAYER_ANIM_LAST_LIFE_GHOST;
                player->velocity_height = 256;
            }
        }
    }
    if (gPlayerRespawnSuppressionTicks[player_number] && !--gPlayerRespawnSuppressionTicks[player_number])
        player->behavior_flags &= ~ENTITY_PLAYER_SUPPRESSED;
}

/* Dispatch player-state handlers, then update common promotion and collision processing. */
void ProcessPlayers()
{
    unsigned short terrain;
    struct object_state *player;
    short player_number;

    terrain = gPlayer1TerrainUnderfoot & TERRAIN_CLASS_MASK;
    player = gPlayerInfo;
    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL;
         terrain = gPlayer2TerrainUnderfoot & TERRAIN_CLASS_MASK, ++player, ++player_number) {
        if (gPlayerLives[player_number] < 0)
            PlayerProcessLifeRequest(player_number);
        else {
            if (gPlayerInventoryOpen[player_number])
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
            PlayerUpdateHealthAndLives(player, terrain);
            if (gPlayerElevatorFlags[player_number] || (gDoorwayPlayerContactFlags[0] & 3) == player_number + 1 ||
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
    if (gPlayerSelection == 2)
        PlayerUpdateHealthSharingClap();
}

