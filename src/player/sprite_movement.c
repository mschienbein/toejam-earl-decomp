#include "entities.h"
#include "assets_audio.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_elevator.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* Select movement speeds, directional sprite, and animation mode. */
void func_00f00c(player, player_number, input, land)
struct object_state *player;
short player_number, input;
unsigned short land;
{
    short velocity_x, velocity_y, animation_mode, sprite;
    short speed_x, speed_y, current_sprite;

    sprite = -1;
    speed_x = 320;
    speed_y = 240;
    if (land == 48) {
        speed_x = 400;
        speed_y = 300;
        if (ram_ffda24[player_number]) {
            player->height = 0;
            player->draw_flags &= ~64;
        }
    }
    velocity_x = player->height;
    if (ram_ffda24[player_number] && velocity_x < 0) {
        if (velocity_x < -16)
            velocity_x = -16;
        speed_x += velocity_x * 10;
        speed_y += velocity_x * 8;
    }
    if (ram_ffda82[player_number]) {
        if (input & 1)
            input = 1;
        else
            ram_ffda82[player_number] = 0;
    }
    if (!(player->flags_52 & 0x8000L)) {
        velocity_x = player->velocity_x;
        velocity_y = player->velocity_y;
    } else {
        velocity_x = player->acceleration_x;
        velocity_y = player->acceleration_y;
    }
    if (player->flags_52 & 0x8000000L) {
        speed_x *= 3;
        speed_y *= 3;
    }
    current_sprite = player->sprite_state;
    animation_mode = player->mode_15;
    if (player->flags_52 & 0x80L) {
        if ((current_sprite == 28 && (input & 1)) ||
            (current_sprite == 29 && (input & 2)) ||
            (current_sprite == 30 && (input & 4)) ||
            (current_sprite == 31 && (input & 8)))
            return;
        sprite = 0;
    } else if (!(input & 15) && (velocity_x || velocity_y)) {
        sprite = 0;
        velocity_x = 0;
        velocity_y = 0;
        animation_mode = 0;
    } else if (input & 12) {
        if (((input & 4) && velocity_x >= 0) ||
            ((input & 8) && velocity_x <= 0)) {
            if (input & 4)
                sprite = 10;
            else
                sprite = 11;
            velocity_y = 0;
            animation_mode = 3;
        }
        if (input & 3) {
            if (input & 1)
                velocity_y = -speed_y;
            else
                velocity_y = speed_y;
            if (input & 4)
                velocity_x = -speed_y;
            else
                velocity_x = speed_y;
        } else {
            velocity_y = 0;
            if (input & 4)
                velocity_x = -speed_x;
            else
                velocity_x = speed_x;
        }
    } else if (input & 3) {
        if (velocity_x || ((input & 1) && velocity_y >= 0) ||
            ((input & 2) && velocity_y <= 0)) {
            if (input & 1)
                sprite = 8;
            else
                sprite = 9;
            animation_mode = 2;
            velocity_x = 0;
        }
        if (input & 1)
            velocity_y = -speed_x;
        else
            velocity_y = speed_x;
    }
    func_00f282(player, player_number, input, sprite, velocity_x, velocity_y, animation_mode);
}

/* Apply movement, select the active animation family, and preserve its phase. */
void func_00f282(player, player_number, input, sprite, velocity_x, velocity_y, mode)
struct object_state *player;
short player_number, input, sprite, velocity_x, velocity_y, mode;
{
    char frame, frame_duration;
    short sequence_index;

    frame = player->frame;
    if (player->flags_52 & 0x8000L) {
        player->acceleration_x = velocity_x >> 4;
        player->acceleration_y = velocity_y >> 4;
        --frame;
    } else if (player->flags_52 & 0x8000000L) {
        player->velocity_x = velocity_x;
        player->velocity_y = velocity_y >> 1;
        player->mode_15 = 0;
    } else {
        player->velocity_x = velocity_x;
        player->velocity_y = velocity_y;
        player->mode_15 = mode;
    }
    if (sprite != -1) {
        if (!sprite) {
            sprite = GetActivePresentSprite(player);
            frame = -1;
        } else {
            sprite &= 3;
            if ((player->flags_52 & 0x40000L) || player->sprite_state == 77)
                sprite += 72;
            else if ((player->flags_52 & 0x800L) || player->sprite_state == 3 || player->sprite_state == 7)
                sprite += 44;
            else if (player->flags_52 & 0x4000L)
                sprite += 56;
            else if (player->flags_52 & 0x8000L)
                sprite += 60;
            else if ((input & 64) && !ELEVATOR_FLAG[player_number] &&
                     gPlayer1Powerup[player_number] == 7) {
                sprite += 108;
                if (!(player->flags_52 & 0x8000000L))
                    PlaySoundEffect((long)Sfx_Running, 50, 0);
            } else if ((input & 64) && gPlayer1Powerup[player_number] == -1)
                sprite += 12;
            else {
                sprite += 8;
                if (player->flags_52 & 0x8000000L)
                    player->mode_15 = 3;
            }
        }
    } else if (sprite == -1) {
        if (input & 64) {
            if (player->flags_52 & 4) {
                if (gPlayer1Powerup[player_number] == -1)
                    sprite = player->sprite_state + 4;
                else if (gPlayer1Powerup[player_number] == 7 && !ELEVATOR_FLAG[player_number]) {
                    sprite = (player->sprite_state & 3) + 108;
                    PlaySoundEffect((long)Sfx_Running, 50, 0);
                }
            }
        } else if ((player->flags_52 & 0x8000008L) && player->frame_duration <= 1) {
            sprite = (player->sprite_state & 3) + 8;
            player->mode_15 = 3;
            if (player->flags_52 & 0x8000000L) {
                player->velocity_x = velocity_x / 3;
                player->velocity_y = velocity_y / 3;
            }
        }
    }
    if (sprite != -1) {
        if ((sprite & 252) == (player->sprite_state & 252)) {
            frame_duration = player->frame_duration;
            if (!frame_duration)
                frame_duration = 1;
        } else {
            if (!player->sprite_state || player->sprite_state == 4 ||
                (player->flags_52 & 0x20400000L) || player->sprite_state == 76)
                frame = -1;
            frame_duration = 1;
        }
        SetSprite(player, sprite);
        player->frame_duration = frame_duration;
        player->frame = frame;
        for (sequence_index = 1; sequence_index <= frame + 1; ++sequence_index) {
            if ((player->sequence[sequence_index] & 240) == 240) {
                player->frame = -1;
                break;
            }
        }
    }
}
