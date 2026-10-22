#define TJE_LOCAL_FUNC_00D904

#include "entities.h"
#include "assets_cutscene.h"
#include "assets_intro.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_intro.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* ORIGINAL BUG: this consumer retains the observed int result view.
 * Both the shared short declaration and ordinary integral promotion
 * fail the complete-file match; historical missing declarations are [INFERENCE]. */
extern int func_00d904();

/* Set both selected objects to the same position. */
void func_028e36(x, y)
int x, y;
{
    struct object_state *object;

    object = &gPlayerInfo[ram_ffe3de];
    object->x = x;
    object->y = y;
    object = &gPlayerInfo[ram_ffe3e0];
    object->x = x;
    object->y = y;
}

/* Allocate the first composite object and start its held-frame stream. */
void func_028e74()
{
    struct object_state *object;

    if (ram_ffe3da = func_00d904()) {
        object = &gPlayerInfo[ram_ffe3da];
        object->tile = 0;
        object->current_frame = CompositeFrames_038AA2[0];
        object->frames = CompositeFrames_038AA2;
        object->tile_count = 30;
        object->sprite_count = 3;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->tile |= 0x8000;
        object->sequence = (char *)&IntroHeldFrameStreams[0];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        func_00d776(object, 0);
    }
}

/* Position the composite five pixels before the supplied coordinates and
 * select a two-byte held-frame stream. */
void func_028ef0(frame, x, y)
short frame, x, y;
{
    struct object_state *object;

    if (ram_ffe3da) {
        object = &gPlayerInfo[ram_ffe3da];
        object->x = x - 5;
        object->y = y - 5;
        object->tile |= 0x8000;
        object->draw_flags |= 0x80;
        object->sequence = (char *)&IntroHeldFrameStreams[frame];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
    }
}

/* Release the selected composite and clear its allocation slot. */
void func_028f62()
{
    if (ram_ffe3da)
        func_00d970(ram_ffe3da);
    ram_ffe3da = 0;
}

/* Allocate the second composite with its held frame and tile attributes. */
void func_028f82()
{
    struct object_state *object;

    if (ram_ffe3dc = func_00d904()) {
        object = &gPlayerInfo[ram_ffe3dc];
        object->current_frame = CompositeFrames_038E2A[0];
        object->frames = CompositeFrames_038E2A;
        object->tile_count = 69;
        object->sprite_count = 6;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->tile |= 0x8000;
        object->sequence = (char *)&IntroHeldFrameStreams[0];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        func_00d776(object, 0);
    }
}

/* Place the second composite at the requested x and 100 pixels below y. */
void func_028ffa(x, y)
short x, y;
{
    struct object_state *object;

    if (ram_ffe3dc) {
        object = &gPlayerInfo[ram_ffe3dc];
        object->x = x;
        object->y = y + 100;
        object->height = 100;
    }
}

/* Release the second composite and clear its allocation slot. */
void func_029036()
{
    if (ram_ffe3dc)
        func_00d970(ram_ffe3dc);
    ram_ffe3dc = 0;
}

/* Allocate the ending composite with the first ending frame-index stream. */
void func_029056()
{
    struct object_state *object;

    if (ram_ffe3e2 = func_00d904()) {
        object = &gPlayerInfo[ram_ffe3e2];
        object->tile = 0;
        object->current_frame = CompositeFrames_0393E8[0];
        object->frames = CompositeFrames_0393E8;
        object->tile_count = 43;
        object->sprite_count = 4;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->tile |= 0x8000;
        object->sequence = &EndingFrameIndexStreams[242];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        func_00d776(object, 0);
    }
}

/* Offset the ending composite 50 pixels to the right. */
void func_0290d2(x, y)
short x, y;
{
    struct object_state *object;

    if (ram_ffe3e2) {
        object = &gPlayerInfo[ram_ffe3e2];
        object->x = x + 50;
        object->y = y;
    }
}

/* Release the ending composite and clear its allocation slot. */
void func_029108()
{
    if (ram_ffe3e2)
        func_00d970(ram_ffe3e2);
    ram_ffe3e2 = 0;
}

/* Allocate the falling rocketship with the corresponding intro stream. */
void func_029128()
{
    struct object_state *object;

    if (ram_ffe3d8 = func_00d904()) {
        object = &gPlayerInfo[ram_ffe3d8];
        object->tile = 0;
        object->current_frame = Spr_IntroRocketshipFalling[0];
        object->frames = Spr_IntroRocketshipFalling;
        object->tile_count = 12;
        object->sprite_count = 2;
        object->x = 104;
        object->y = -50;
        object->sprite_ready = 0;
        object->sequence = (char *)&IntroFrameIndexLists[7];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 6;
        func_00d776(object, 0);
    }
}

/* Release the falling rocketship and clear its allocation slot. */
void func_02919e()
{
    if (ram_ffe3d8)
        func_00d970(ram_ffe3d8);
    ram_ffe3d8 = 0;
}

/* Set the falling rocketship's position if it is allocated. */
void func_0291be(x, y)
short x, y;
{
    struct object_state *object;

    if (ram_ffe3d8) {
        object = &gPlayerInfo[ram_ffe3d8];
        object->x = x;
        object->y = y;
    }
}

/* Allocate the three-piece ending composite at an elevated position. */
void func_0291ec(x, y)
unsigned short x, y;
{
    struct object_state *object;

    if (ram_ffe3e4 = func_00d904()) {
        object = &gPlayerInfo[ram_ffe3e4];
        object->tile = 0;
        object->current_frame = CompositeFrames_0396EA[0];
        object->frames = CompositeFrames_0396EA;
        object->tile = 0x8000;
        object->tile_count = 30;
        object->sprite_count = 3;
        object->x = x;
        object->y = y + 50;
        object->height = 50;
        object->sprite_ready = 0;
        object->sequence = &EndingFrameIndexStreams[245];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 4;
        func_00d776(object, 0);
    }
}

/* Release the elevated ending composite and clear its allocation slot. */
void func_029284()
{
    if (ram_ffe3e4)
        func_00d970(ram_ffe3e4);
    ram_ffe3e4 = 0;
}

/* Restart the elevated composite on the alternate ending stream. */
void func_0292a4()
{
    struct object_state *object;

    if (ram_ffe3e4) {
        object = &gPlayerInfo[ram_ffe3e4];
        object->sprite_ready = 0;
        object->sequence = &EndingFrameIndexStreams[249];
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = 4;
    }
}
