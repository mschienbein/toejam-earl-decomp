#include "entities.h"
#include "sprites.h"
#include "assets_ui.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "speech.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Set the lifetime override consumed by the next allocated speech bubble. */
void func_01e012(lifetime)
short lifetime;
{
    ram_ffe206 = lifetime;
}

/* Allocate one of four speech bubbles, replacing the oldest when necessary.
 * Attached bubbles retain their speaker so later updates can follow it. */
short GenerateSpeech(speaker, text, replace_attached)
short speaker;
char *text;
char replace_attached;
{
    struct object_state *source, *bubble;
    short slot, entity_number, index, length, viewport;

    source = &gPlayerInfo[speaker];
    if (!(source->draw_flags & 6))
        return 0;
    if (speaker <= 1 && source->sprite_state == 117)
        return 0;
    if (replace_attached) {
        for (slot = 0; slot < 4; ++slot) {
            if (entity_number = ram_ffe1ee[slot]) {
                bubble = &gPlayerInfo[entity_number];
                if ((bubble->flags_4d & 8) && bubble->parent == speaker) {
                    func_00d970(entity_number);
                    ram_ffe1ee[slot] = 0;
                }
            }
        }
    }
    for (slot = 0; slot < 4; ++slot)
        if (!ram_ffe1ee[slot])
            break;
    if (slot == 4) {
        slot = 0;
        for (index = 1; index < 4; ++index)
            if (ram_ffe1f2[index] < ram_ffe1f2[slot])
                slot = index;
        func_00d970(ram_ffe1ee[slot]);
    }
    for (length = 0; text[length]; ++length)
        ;
    if (length > 12)
        length = 12;
    if (ram_ffe1ee[slot] = (int)func_00d904()) {
        bubble = &gPlayerInfo[ram_ffe1ee[slot]];
        bubble->x = source->x;
        bubble->y = source->y + 15;
        bubble->height = source->height + 15;
        bubble->level = source->level;
        bubble->width = length << 3;
        bubble->height_pixels = 16;
        bubble->sprite_count = 4;
        bubble->tile_count = 13;
        if (gLevel912C < 26)
            bubble->tile = 0x8000;
        bubble->current_frame = &RuntimeTextMetasprite_0AFADC;
        if (source->draw_flags & 2) {
            func_00d776(bubble, 0);
            viewport = 0;
        } else {
            func_00d776(bubble, 1);
            viewport = 1;
        }
        if (!(bubble->draw_flags & 6)) {
            func_00d970(ram_ffe1ee[slot]);
            ram_ffe1ee[slot] = 0;
            return 0;
        }
        bubble->draw_flags |= 8;
        while (VBLANK_FLAG_2)
            ;
        func_01df1a(text, length, bubble->tile & 0x7ff);
        func_00c920(bubble, &RuntimeTextMetasprite_0AFADC, 1, viewport);
        bubble->sprite_ready = 1;
        if (replace_attached) {
            bubble->flags_4d |= 8;
            bubble->parent = speaker;
            bubble->flag_58 = 15;
            bubble->offset_height = 15;
        }
        if (ram_ffe206) {
            ram_ffe1f2[slot] = ram_ffe207;
            ram_ffe206 = 0;
        } else
            ram_ffe1f2[slot] = 40;
        return ram_ffe1ee[slot];
    }
    return 0;
}

/* Release active speech objects and discard both pending replies. */
void func_01e2c4()
{
    short slot;

    for (slot = 0; slot < 4; ++slot) {
        if (ram_ffe1ee[slot]) {
            func_00d970(ram_ffe1ee[slot]);
            ram_ffe1ee[slot] = 0;
        }
    }
    ram_ffe1f8[0] = 0;
    gPlayer1TomatoRainThrottle = 0;
    ram_ffe1f6 = 0;
}

/* Report either an existing bubble or a pending reply for this speaker. */
int func_01e312(speaker)
short speaker;
{
    short slot;
    struct object_state *bubble;

    for (slot = 0; slot < 4; ++slot) {
        if (ram_ffe1ee[slot]) {
            bubble = &gPlayerInfo[ram_ffe1ee[slot]];
            if ((bubble->draw_flags & 8) && bubble->parent == speaker)
                return 1;
        }
    }
    if ((ram_ffe1f8[0] && ram_ffe204[0] == speaker) ||
        (gPlayer1TomatoRainThrottle && ram_ffe205 == speaker))
        return 1;
    return 0;
}
