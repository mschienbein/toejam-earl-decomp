#include "entities.h"
#include "sprites.h"
#include "assets_intro.h"
#include "core_calls.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "ram_system.h"
#include "reset_calls.h"
#include "sprite_calls.h"

/* Twelve is the entity's sprite-list capacity, not a claimed fixed frame
 * extent. Each frame's four-byte prefix is followed by its counted pieces;
 * the bounded payload view retains the existing word-index arithmetic. */

/* Allocate eight actor records and total each selected frame's tile area. */
void func_028104()
{
    short index, object_number, piece;
    struct render_frame *frame;
    struct object_state *entity;

    for (index = 0; index < 8; ++index) {
        frame = Spr_TitleBigPlayers[dat_030770[index]];
        if (object_number = (int)func_00d904()) {
            entity = &gPlayerInfo[object_number];
            entity->sprite_count = frame->count;
            entity->tile_count = 0;
            for (piece = 0; piece < frame->count; ++piece)
                entity->tile_count += ((struct render_piece (*)[12])(frame + 1))[0][piece].width *
                                      ((struct render_piece (*)[12])(frame + 1))[0][piece].height;
            entity->tile = 0x8000;
            entity->current_frame = frame;
        }
        ram_ffe3ae[index] = object_number;
    }
}

/* Release the eight actor records and clear every selector slot. */
void func_0281a8()
{
    short index, object_number;

    for (index = 0; index < 8; ++index) {
        if (object_number = ram_ffe3ae[index])
            func_00d970(object_number);
        ram_ffe3ae[index] = 0;
    }
}

/* Position an actor and select its held-frame animation stream. */
void func_0281e4(index, x, y)
short index, x, y;
{
    short object_number;
    struct render_frame *frame;
    struct object_state *entity;

    if (index >= 8)
        return;
    frame = Spr_TitleBigPlayers[index];
    object_number = ram_ffe3ae[index];
    entity = &gPlayerInfo[object_number];
    entity->x = x;
    entity->y = y;
    if (index == 3) {
        entity->y += 5;
        entity->height = 5;
    }
    entity->sequence = (char *)&dat_0307ae[index];
    entity->frames = Spr_TitleBigPlayers;
    entity->frame_phase = 10;
    entity->frame_duration = 1;
    entity->frame = -1;
    func_00d776(entity, 0);
    entity->sprite_ready = 0;
}

/* Replace one actor's sequence and restart its frame timer. */
void func_02827e(index, sequence, period)
short index;
char *sequence;
char period;
{
    struct object_state *entity;

    entity = &gPlayerInfo[ram_ffe3ae[index]];
    entity->sequence = sequence;
    entity->frame_phase = period;
    entity->frame_duration = 1;
    entity->frame = -1;
}

/* Release the primary viewport state of an allocated actor. */
void func_0282c0(index)
short index;
{
    struct object_state *entity;

    if (index >= 8)
        return;
    entity = &gPlayerInfo[ram_ffe3ae[index]];
    if (!(entity->draw_flags & 1))
        return;
    OpeningPresents7(entity, 0);
}

/* Place the seven visible actors in selector order. */
void func_0282fc()
{
    func_0281e4(0, 148, 112);
    func_0281e4(4, 148, 112);
    func_0281e4(1, 148, 112);
    func_0281e4(2, 148, 112);
    func_0281e4(5, 148, 112);
    func_0281e4(6, 148, 112);
    func_0281e4(7, 148, 112);
}

/* Keep the selector order: the repeated target is hoisted into a preserved
 * address register, and the original combines the argument stack cleanups. */
void func_02836c()
{
    func_0282c0(0);
    func_0282c0(4);
    func_0282c0(1);
    func_0282c0(2);
    func_0282c0(5);
    func_0282c0(6);
    func_0282c0(7);
    VBLANK_FLAG_2 = 0;
    func_00cc82();
    WaitForNextVBlank(1);
    SpriteLoadingStuff(0, 0);
}

/* Allocate the three driving-scene actors and total their frame tile areas. */
void func_0283c8()
{
    short index, object_number, piece;
    struct render_frame *frame;
    struct object_state *entity;

    for (index = 0; index < 3; ++index) {
        frame = Spr_IntroEarlDriving[dat_03071d[index]];
        if (object_number = (int)func_00d904()) {
            entity = &gPlayerInfo[object_number];
            entity->sprite_count = frame->count;
            entity->tile_count = 0;
            for (piece = 0; piece < frame->count; ++piece)
                entity->tile_count += ((struct render_piece (*)[12])(frame + 1))[0][piece].width *
                                      ((struct render_piece (*)[12])(frame + 1))[0][piece].height;
            entity->tile = 0x8000;
            entity->current_frame = frame;
        }
        ram_ffe3c6[index] = object_number;
    }
}

/* Release all driving-scene actor records and clear their selector slots. */
void func_02846c()
{
    short index, object_number;

    for (index = 0; index < 3; ++index) {
        if (object_number = ram_ffe3c6[index])
            func_00d970(object_number);
        ram_ffe3c6[index] = 0;
    }
}

/* Position a driving-scene actor and select its held-frame stream. */
void func_0284a8(index, x, y)
short index, x, y;
{
    short object_number;
    struct render_frame *frame;
    struct object_state *entity;

    if (index >= 3)
        return;
    frame = Spr_IntroEarlDriving[index];
    object_number = ram_ffe3c6[index];
    entity = &gPlayerInfo[object_number];
    entity->x = x;
    entity->y = y;
    if (index == 1) {
        entity->y += 100;
        entity->height += 100;
    } else if (index == 2) {
        entity->y += 101;
        entity->height += 101;
    }
    entity->sequence = (char *)&dat_0307be[index];
    entity->frames = Spr_IntroEarlDriving;
    entity->frame_phase = 10;
    entity->frame_duration = 1;
    entity->frame = -1;
    func_00d776(entity, 0);
    entity->sprite_ready = 0;
}

/* Replace a driving actor's sequence and restart its frame timer. */
void func_028558(index, sequence, period)
short index;
char *sequence;
char period;
{
    struct object_state *entity;

    entity = &gPlayerInfo[ram_ffe3c6[index]];
    entity->sequence = sequence;
    entity->frame_phase = period;
    entity->frame_duration = 1;
    entity->frame = -1;
}

/* Release the primary viewport state when either low flag bit is set. */
void func_02859a(index)
short index;
{
    struct object_state *entity;

    if (index >= 3)
        return;
    entity = &gPlayerInfo[ram_ffe3c6[index]];
    if (!(entity->draw_flags & 3))
        return;
    OpeningPresents7(entity, 0);
}

/* Create and initialize the asteroid actor used in the driving scene. */
void func_0285da()
{
    struct object_state *entity;

    if (ram_ffe3c4 = (int)func_00d904()) {
        entity = &gPlayerInfo[ram_ffe3c4];
        entity->tile = 0;
        entity->current_frame = Spr_IntroAsteroid[0];
        entity->frames = Spr_IntroAsteroid;
        entity->tile_count = 160;
        entity->sprite_count = 12;
        entity->x = 104;
        entity->y = 151;
        entity->sprite_ready = 0;
        entity->sequence = dat_030723;
        entity->frame_duration = 1;
        entity->frame = -1;
        entity->frame_phase = 6;
        func_00d776(entity, 0);
    }
}

/* Release the asteroid actor and clear its stored object identifier. */
void func_028650()
{
    if (ram_ffe3c4)
        func_00d970(ram_ffe3c4);
    ram_ffe3c4 = 0;
}