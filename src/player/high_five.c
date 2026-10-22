#include "player_animation.h"
#include "entities.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "random.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Compare the proposed position with the player's 5-by-5 local terrain
 * cache. High-bit terrain starts a jump; otherwise alternate the meep sound. */
int PlayerCheckTerrainEdgeHop(state, next_x, next_y)
struct object_state *state;
short next_x, next_y;
{
    unsigned char *nearby_land;
    short cell_y;
    /* MATCHING QUALIFICATION: `register` on the X cell lets the compiler reuse
     * its input register as the original does; the historical spelling is
     * unknown. */
    register short cell_x;
    unsigned char tile;

    if (!state->entity_index)
        nearby_land = &LAND_NEAR_TOEJAM[0][0];
    else
        nearby_land = &LAND_NEAR_EARL[0][0];
    cell_x = next_x >> 3;
    cell_x -= state->x >> 3;
    cell_x += 2;
    cell_y = next_y >> 3;
    cell_y -= state->y >> 3;
    cell_y += 2;
    tile = nearby_land[cell_y * 5 + cell_x];
    if (tile >= 0x80) {
        state->velocity_height = 512;
        state->acceleration_height = -37;
        state->draw_flags |= ENTITY_SHADOW;
        SetSprite(state, (char)((state->sprite_state & DIRECTION_MASK) + PLAYER_ANIM_LEAP_UP));
        return 1;
    }
    if (!gPcmEffectCooldownTicks) {
        if (ram_ffda3c) {
            PlaySoundEffect((long)Sfx_Meep, 40, 0);
            ram_ffda3c = 0;
        } else if (!(GetRandomNumber() & 0x1ff)) {
            PlaySoundEffect((long)Sfx_Meep, 40, 0);
            ram_ffda3c = 1;
        }
    }
    return 0;
}

/* Align complementary facing sprites into a shared animation. When Earl's
 * animation completes, redistribute the players' health using the average
 * and the two values returned by PlayerGetMaximumHealth. */
void PlayerUpdateHealthSharingClap()
{
    struct object_state *toejam, *earl;
    short average_health, separation_y, separation_x;

    toejam = &gPlayerInfo[PLAYER_TOEJAM];
    earl = &gEarlPositionX;
    if (toejam->level != earl->level)
        return;
    if ((toejam->sprite_state & 0xfe) == 90) {
        if (!toejam->frame_duration)
            SetSprite(toejam, PLAYER_ANIM_IDLE);
        else if (toejam->frame == 2) {
            PlaySoundEffect((long)Sfx_JamClap, 95, 1);
            ++toejam->frame;
        }
    }
    if ((earl->sprite_state & 0xfe) == 90) {
        if (!earl->frame_duration) {
            SetSprite(earl, PLAYER_ANIM_IDLE);
            average_health = (TOEJAMS_HEALTH[PLAYER_TOEJAM] + TOEJAMS_HEALTH[PLAYER_EARL] + 1) >> 1;
            /* ORIGINAL BUG: These branches do not cap the compared player's
             * health to PlayerGetMaximumHealth's result ([INFERENCE] its health limit). */
            if (average_health > PlayerGetMaximumHealth(0)) {
                TOEJAMS_HEALTH[PLAYER_TOEJAM] = average_health;
                TOEJAMS_HEALTH[PLAYER_EARL] = (average_health << 1) - PlayerGetMaximumHealth(0);
            } else if (average_health > PlayerGetMaximumHealth(1)) {
                TOEJAMS_HEALTH[PLAYER_EARL] = average_health;
                TOEJAMS_HEALTH[PLAYER_TOEJAM] = (average_health << 1) - PlayerGetMaximumHealth(1);
            } else {
                TOEJAMS_HEALTH[PLAYER_TOEJAM] = average_health;
                TOEJAMS_HEALTH[PLAYER_EARL] = average_health;
            }
        } else if (earl->frame == 2)
            ++earl->frame;
        return;
    }
    separation_y = earl->y - toejam->y;
    if (separation_y < 0 || separation_y > 6)
        return;
    separation_x = earl->x - toejam->x;
    if (toejam->sprite_state == PLAYER_ANIM_WALK_RIGHT && earl->sprite_state == PLAYER_ANIM_WALK_LEFT) {
        if (separation_x > 0 && separation_x < 25) {
            SetSprite(toejam, PLAYER_ANIM_HIGH_FIVE_RIGHT);
            toejam->x = earl->x - 23;
            toejam->y = earl->y - 3;
            SetSprite(earl, PLAYER_ANIM_HIGH_FIVE_LEFT);
            toejam->height = 0;
            earl->height = 0;
        }
    } else if (earl->sprite_state == PLAYER_ANIM_WALK_RIGHT && toejam->sprite_state == PLAYER_ANIM_WALK_LEFT &&
               separation_x < 0 && separation_x > -25) {
        SetSprite(toejam, PLAYER_ANIM_HIGH_FIVE_LEFT);
        toejam->x = earl->x + 23;
        toejam->y = earl->y - 3;
        SetSprite(earl, PLAYER_ANIM_HIGH_FIVE_RIGHT);
        toejam->height = 0;
        earl->height = 0;
    }
}
