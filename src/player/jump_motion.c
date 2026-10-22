#include "player_animation.h"
#include "player_animation_groups.h"
#include "terrain.h"
#include "psg_effects.h"
#include "entities.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"

/* Advance the alternating jump phase, handle landing and water/space,
 * and delegate the input-dependent horizontal velocity update. */
void PlayerUpdatePhasedJumpMotion(state, terrain)
struct object_state *state;
unsigned short terrain;
{
    short player;
    char direction, phase, jump;

    player = state->entity_index;
    if (gPlayerPowerups[player])
        return;
    if (!gPlayerTerrainSinking[player] && (state->animation_group_mask & (PLAYER_GROUP_WALK | PLAYER_GROUP_SNEAK))) {
        state->height = 0;
        state->draw_flags &= ~ENTITY_SHADOW;
        return;
    }
    if (gPlayerFallStates[player] ||
        !(state->sprite_state == PLAYER_ANIM_IDLE || state->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
          (state->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || state->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE ||
          (state->animation_group_mask & PLAYER_GROUP_ICARUS_FLY)) || gPlayerElevatorFlags[player] ||
        (gPlayerTerrainSinking[player] && state->height < -4))
        return;
    direction = state->sprite_state & DIRECTION_MASK;
    phase = -1;
    jump = gPlayerProcessedInput[player] & 0x40;
    if (!jump) {
        gPlayerPhasedJumpArmed[player] = 1;
        gPlayerChoiceActionReleaseLatches[player] = 0;
    }
    if (gPlayerChoiceActionReleaseLatches[player])
        return;
    if (state->sprite_state == PLAYER_ANIM_IDLE || state->sprite_state == PLAYER_ANIM_ICARUS_IDLE ||
        (state->animation_group_mask & (PLAYER_GROUP_SOCIAL | PLAYER_GROUP_STATIONARY)) || state->sprite_state == PLAYER_ANIM_INNERTUBE_IDLE) {
        if (!jump || !gPlayerPhasedJumpArmed[player])
            return;
        phase = 1;
    } else if (!gPlayerPhasedJumpPhase[player] && !jump) {
        if (gPlayerPhasedJumpPhaseTicks[player] > 1)
            phase = 1;
    } else if (gPlayerPhasedJumpPhase[player] == 1 && jump) {
        phase = 0;
        gPlayerPhasedJumpPhaseTicks[player] = 0;
        PlaySound(PSG_SHORT_NOISE_CRESCENDO, 20);
    } else if (state->height < 4) {
        state->velocity_x = 0;
        state->acceleration_x = 0;
        state->velocity_y = 0;
        state->acceleration_y = 0;
        phase = -1;
        if (state->velocity_height < 0) {
            state->height = 0;
            state->velocity_height = 0;
            state->acceleration_height = 0;
            state->draw_flags &= ~ENTITY_SHADOW;
            if (terrain >= TERRAIN_WATER) {
                if (terrain == TERRAIN_VOID)
                    gPlayerJumpCounters[player] = 0;
                else {
                    phase = direction + PLAYER_ANIM_FALL_SPLASH_UP;
                    PlaySoundEffect((long)Sfx_Splash, 50, 0);
                }
            } else {
                phase = GetActivePresentSprite(state);
                state->animation_motion_axes = ANIMATION_MOTION_XY;
                gPlayerPhasedJumpArmed[player] = 0;
            }
            if (phase != -1) {
                SetSprite(state, phase);
                phase = -1;
            }
        } else if (terrain == TERRAIN_VOID) {
            if (gPlayerJumpCounters[player] > 10)
                PlayerStartFalling(state, player);
            else if (gPlayerPhasedJumpPhaseTicks[player] > 1) {
                phase = gPlayerPhasedJumpPhase[player] ^ 1;
                gPlayerPhasedJumpPhaseTicks[player] = 0;
                ++gPlayerJumpCounters[player];
            }
        }
        if (phase == -1)
            return;
    }
    if (gPlayerPhasedJumpPhaseTicks[player] < 64)
        ++gPlayerPhasedJumpPhaseTicks[player];
    if (!(state->animation_group_mask & PLAYER_GROUP_ICARUS_FLY)) {
        SetSprite(state, PLAYER_ANIM_ICARUS_FLY_DOWN);
        state->draw_flags |= ENTITY_SHADOW;
        state->animation_motion_axes = ANIMATION_MOTION_NONE;
    } else if (terrain == TERRAIN_VOID)
        state->draw_flags &= ~ENTITY_SHADOW;
    else
        state->draw_flags |= ENTITY_SHADOW;
    PlayerApplyPhasedJumpInput(state, player, phase, jump);
}

/* Apply directional boosts at phase changes, decelerate against released
 * directions, clamp both horizontal axes, and speak once after rising. */
void PlayerApplyPhasedJumpInput(state, player, phase, jump)
struct object_state *state;
short player;
char phase, jump;
{
    unsigned char input;
    short velocity_x, velocity_y;
    char left, right, up, down;
    short maximum_speed;

    if (gPlayerControlRemapEntities[player])
        input = PlayerRemapDirectionalInput(player);
    else
        input = gPlayerProcessedInput[player];
    velocity_x = state->velocity_x;
    velocity_y = state->velocity_y;
    left = input & 4;
    right = input & 8;
    up = input & 1;
    down = input & 2;
    if (phase != -1) {
        gPlayerPhasedJumpPhase[player] = phase;
        state->frame = phase - 1;
        state->frame_duration = 1;
        if (jump) {
            if (state->height < 50) {
                state->velocity_height = 256;
                state->acceleration_height = -10;
            } else
                state->velocity_height = 0;
            if ((velocity_x > 0 && right) || (velocity_x < 0 && left))
                velocity_x <<= 1;
            else if ((velocity_x < 0 && right) || (velocity_x > 0 && left))
                velocity_x = 0;
            if ((velocity_y > 0 && down) || (velocity_y < 0 && up))
                velocity_y <<= 1;
            else if ((velocity_y < 0 && down) || (velocity_y > 0 && up))
                velocity_y = 0;
        }
    }
    if ((velocity_x > 0 && !right && (velocity_x -= 10) < 0) ||
        (velocity_x < 0 && !left && (velocity_x += 10) > 0))
        velocity_x = 0;
    if ((velocity_y > 0 && !down && (velocity_y -= 10) < 0) ||
        (velocity_y < 0 && !up && (velocity_y += 10) > 0))
        velocity_y = 0;
    if (velocity_y)
        maximum_speed = 384;
    else
        maximum_speed = 512;
    if (abs(velocity_x) > maximum_speed) {
        if (velocity_x > 0)
            velocity_x = maximum_speed;
        else
            velocity_x = -maximum_speed;
    }
    if (velocity_x)
        maximum_speed -= 128;
    else
        maximum_speed -= 64;
    if (abs(velocity_y) > maximum_speed) {
        if (velocity_y > 0)
            velocity_y = maximum_speed;
        else
            velocity_y = -maximum_speed;
    }
    state->velocity_x = velocity_x;
    state->velocity_y = velocity_y;
    if (!gPlayerPhasedJumpSpeechUsed[player] && state->height > 10) {
        GenerateSpeech(player, "Awesome!!", 1);
        gPlayerPhasedJumpSpeechUsed[player] = 1;
    }
}
