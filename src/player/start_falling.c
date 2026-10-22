#include "entities.h"
#include "ram.h"
#include "assets.h"
#include "player.h"
#include "sprite_calls.h"
#include "romcalls.h"
/* Start falling in the current facing direction, nudging away from a
 * neighboring supported tile before the vblank terrain-priority update. */
void SetToejamFalling(player, player_number)
struct object_state *player;
short player_number;
{
    unsigned char *nearby_land;
    char sprite;
    short center_tile;

    nearby_land = player_number == 0 ? &LAND_NEAR_TOEJAM[0][0] : &LAND_NEAR_EARL[0][0];
    TOEJAM_FALL_STATE[player_number] = 1;
    sprite = (player->sprite_state & 3) + 20;
    center_tile = 12;
    if (sprite == 20 || sprite == 21) {
        if (nearby_land[center_tile - 1] < 0x80)
            player->x += 8;
        else if (nearby_land[center_tile + 1] < 0x80)
            player->x -= 8;
    }
    player->acceleration_height = -128;
    SetSprite(player, sprite);
    player->mode_15 = 0;
    player->tile &= 0x7fff;
    ram_ffda32 = player_number;
    WAIT_FOR_VBLANK_FLAG = 1;
    while (WAIT_FOR_VBLANK_FLAG)
        ;
    GenerateSpeech(player_number, "Bummer", 0);
    PlaySoundEffect((long)Sfx_Falling, 50, 0);
}

/* Launch a short directional fall and remember that it has started. */
void func_00f8de(player)
struct object_state *player;
{
    short player_number;
    char sprite;

    player_number = player->entity_number;
    player->height = 5;
    player->velocity_height = 512;
    player->acceleration_height = -80;
    player->mode_15 = 0;
    sprite = (player->sprite_state & 3) + 20;
    ram_ffda30[player_number] = 1;
    player->velocity_x = 0;
    player->velocity_y = 0;
    if (sprite == 20)
        player->velocity_y = -256;
    else if (sprite == 21)
        player->velocity_y = 256;
    else if (sprite == 22)
        player->velocity_x = -512;
    else if (sprite == 23)
        player->velocity_x = 512;
    SetSprite(player, sprite);
}

/* Count the teetering frames and play the observed randomized speech sounds.
 * After thirty frames, start the short fall unless the shared flag delays it. */
void func_00f96c(player)
struct object_state *player;
{
    short player_number;
    unsigned short sound_choice;

    player_number = player->entity_number;
    if (player->flags_52 & 16) {
        if (++TOEJAM_JUMP_COUNTER[player_number] > 30) {
            if (ram_ffda6c)
                --TOEJAM_JUMP_COUNTER[player_number];
            else
                func_00f8de(player);
        } else if (TOEJAM_JUMP_COUNTER[player_number] == 1) {
            if (PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0] & 1)
                PlaySoundEffect((long)Sfx_Teetering1, 86, 0);
            else
                PlaySoundEffect((long)Sfx_Teetering2, 86, 0);
        } else if (!TOEJAM_SPEECH_COUNTER) {
            sound_choice = PREGEN_RANDOMS[ram_ff91e0 == 58 ? (ram_ff91e0 = 0) : ++ram_ff91e0] & 15;
            if (sound_choice == 0)
                PlaySoundEffect((long)Sfx_Teetering1, 20, 0);
            else if (sound_choice == 1)
                PlaySoundEffect((long)Sfx_Teetering2, 20, 0);
            else if (sound_choice == 2)
                PlaySoundEffect((long)Sfx_Teetering3, 20, 0);
        }
    }
}
