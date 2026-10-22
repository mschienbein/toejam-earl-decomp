#include "entities.h"
#include "sprites.h"
#include "hardware_calls.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Chain both players' primary sprite lists and transfer the hardware table. */
void LinkPlayerSpritesForSplitWipe()
{
    char index, sprite_count, sprite_number, next_sprite;
    char *sprite_list;

    sprite_count = gPlayerInfo[PLAYER_TOEJAM].current_frame
        ->piece_count;
    sprite_list = gPlayerInfo[PLAYER_TOEJAM].primary_sprites;
    sprite_number = *sprite_list;
    gHardwareSpriteTable[23].link = sprite_number;
    for (index = 0; index < sprite_count; ++index) {
        sprite_number = sprite_list[index];
        if (index >= sprite_count - 1)
            break;
        next_sprite = sprite_list[index + 1];
        gHardwareSpriteTable[sprite_number].link = next_sprite;
    }
    sprite_count = gPlayerInfo[PLAYER_EARL].current_frame->piece_count;
    sprite_list = gPlayerInfo[PLAYER_EARL].primary_sprites;
    gHardwareSpriteTable[sprite_number].link = *sprite_list;
    for (index = 0; index < sprite_count; ++index) {
        sprite_number = sprite_list[index];
        if (index < sprite_count - 1)
            next_sprite = sprite_list[index + 1];
        else
            next_sprite = 79;
        gHardwareSpriteTable[sprite_number].link = next_sprite;
    }
    DmaCopyWordsToVram(gHardwareSpriteTable, 0xf800, 320);
}
