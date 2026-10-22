#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_audio.h"
#include "assets_intro.h"
#include "assets_jam.h"
#include "assets_palettes.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "cutscene_text_calls.h"
#include "hardware_calls.h"
#include "jam_calls.h"
#include "ram_camera.h"
#include "ram_jam.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "random.h"
#include "sprite_calls.h"
#include "title_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Run the jam scene until Start has been held for 51 consecutive updates. */
void StartJamOut(song)
short song;
{
    short start_hold;

    while (GetCtrl1Input() | GetCtrl2Input())
        WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    PlaySoundEffect((long)Sfx_Jammin, 90, 0);
    ram_ffe456 = song;
    gVBlankFunc = JamOutVBlank;
    ram_ffe476 = 1;
    VDP_COMMAND = 0x46600001L;
    MoveDataToVRAM(CrashScreen, CrashScreenSize << 4);
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    ram_ffd936 = 1;
    func_03b564();
    VDP_CONTROL_WORD = 0x8c89;
    ram_ff8196 = 0;
    func_03b39e(1);
    start_hold = 0;
    if (song == 7) {
        ram_ffe466 = (int)GetRandomNumber() % 42;
        ram_ffe468 = (int)GetRandomNumber() % 42;
        ram_ffe46a = (int)GetRandomNumber() % 42;
        ram_ffe460 = (int)GetRandomNumber() % 42;
        ram_ffe462 = (int)GetRandomNumber() % 42;
        ram_ffe464 = (int)GetRandomNumber() % 42;
        gVBlankFunc = func_03aec2;
    }
    if (song) {
        --song;
        WaitForNextVBlank(1);
        VBLANK_FLAG_8 = !song;
        VBLANK_FLAG_7 = song;
    } else {
        func_03e43c();
        func_03e40e();
    }
    WaitForNextVBlank(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    CutsceneTextLoad("press and hold start to exit", 160, 30);
    while (start_hold < 51) {
        func_02791e(3, 0);
        func_03b39e(0);
        func_03b27a();
        if ((GetCtrl1Input() | GetCtrl2Input()) & 128)
            ++start_hold;
        else
            start_hold = 0;
        if (ram_ffe44e) {
            if (ram_ffe44e & 64) {
                if ((int)GetRandomNumber() & 1)
                    func_03b836(JamOutCharacterAnimations, 6);
                else
                    func_03b836(dat_093831, 6);
            }
            if (ram_ffe44e & 16) {
                if ((int)GetRandomNumber() & 1)
                    func_03b836(dat_09383a, 6);
                else
                    func_03b836(dat_093837, 6);
            }
            if (ram_ffe44e & 32) {
                func_03b836(dat_09383d, 3);
                ram_ffe452 = 1;
            }
            if ((ram_ffe44e & 15) || (ram_ffe44e & 128)) {
                func_03b836(dat_093842, 2);
                ram_ffe452 = 1;
            }
            ram_ffe44e = 0;
        }
        if (ram_ffe450) {
            if (ram_ffe450 & 64) {
                if ((int)GetRandomNumber() & 1)
                    func_03b880(JamOutCharacterAnimations, 6);
                else
                    func_03b880(dat_093831, 6);
            }
            if (ram_ffe450 & 16) {
                if ((int)GetRandomNumber() & 1)
                    func_03b880(dat_09383a, 6);
                else
                    func_03b880(dat_093837, 6);
            }
            if (ram_ffe450 & 32) {
                func_03b880(dat_09383d, 3);
                ram_ffe454 = 1;
            }
            if ((ram_ffe450 & 15) || (ram_ffe450 & 128)) {
                func_03b880(dat_093842, 2);
                ram_ffe454 = 1;
            }
            ram_ffe450 = 0;
        }
    }
    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    gVBlankFunc = 0;
    ram_ffe476 = 0;
    func_027e42();
    func_02743a();
    func_03b39e(1);
    func_02747c();
    WaitForNextVBlank(1);
    VBLANK_FLAG_8 = 1;
    VBLANK_FLAG_7 = 0;
    func_03b6d2();
    ram_ffd936 = 0;
    func_0278e8();
}

/* Alternate controller polling and route each player's new input to samples
 * or channel-template effects. PLAYER_INDICATOR 1 swaps the routed words. */
void JamOutVBlank()
{
    unsigned short saved_input;
    if (VBLANK_FLAG_8 != VBLANK_FLAG_7) {
        func_03e46a(VBLANK_FLAG_7);
        VBLANK_FLAG_8 = VBLANK_FLAG_7;
    } else
        MassiveRoutine();
    ++TIME_COUNTER;
    if (INPUT_READING_FLAG_LO & 1) {
        P1_PROCESSED_INPUT[0] = ram_ff801c ? GetCtrl2Input() : GetCtrl1Input();
        if (P1_PROCESSED_INPUT[0] != ram_ffe442) {
            ram_ffe458 = ram_ffe44e = P1_PROCESSED_INPUT[0];
            ram_ffe442 = P1_PROCESSED_INPUT[0];
        }
    } else {
        P1_PROCESSED_INPUT[1] = ram_ff801c ? GetCtrl1Input() : GetCtrl2Input();
        if (P1_PROCESSED_INPUT[1] != ram_ffe44a) {
            ram_ffe45a = ram_ffe450 = P1_PROCESSED_INPUT[1];
            ram_ffe44a = P1_PROCESSED_INPUT[1];
        }
    }
    if (PLAYER_INDICATOR == 1) {
        saved_input = ram_ffe458;
        ram_ffe458 = ram_ffe45a;
        ram_ffe45a = saved_input;
        ++TIME_COUNTER;
    }
    if (INPUT_READING_FLAG_LO & 1) {
        if (ram_ffe458) {
            if (ram_ffe458 & 64) {
                if (!ram_ffe456)
                    gMusicUnknownE488 = 1;
                PlaySoundEffect((long)Sfx_JamBassDrum, 90, 0);
            }
            if (ram_ffe458 & 16)
                PlaySoundEffect((long)Sfx_JamSnareDrum, 90, 0);
            if (ram_ffe458 & 32)
                PlaySoundEffect((long)Sfx_JamClap, 90, 0);
            if (ram_ffe458 & 15) {
                if (ram_ffe458 & 1)
                    PlaySoundEffect((long)Sfx_JamToejam, 90, 0);
                if (ram_ffe458 & 2)
                    PlaySoundEffect((long)Sfx_JamBigEarl, 90, 0);
                if (ram_ffe458 & 4)
                    PlaySoundEffect((long)Sfx_JamRap, 90, 0);
                if (ram_ffe458 & 8)
                    PlaySoundEffect((long)Sfx_Jammin, 90, 0);
            }
            if (ram_ffe458 & 128)
                PlaySoundEffect((long)Sfx_JamRecordScratch, 90, 0);
            ram_ffe458 = 0;
        }
    } else {
        if (ram_ffe45a) {
            if (ram_ffe45a & 64) {
                func_03e4f8(22);
                if (!ram_ffe456)
                    gMusicUnknownE488 = 1;
            }
            if (ram_ffe45a & 16)
                func_03e4f8(20);
            if (ram_ffe45a & 32)
                func_03e4f8(19);
            if ((ram_ffe45a & 15) || (ram_ffe45a & 128))
                func_03e4f8(21);
            ram_ffe45a = 0;
        }
    }
    if (PLAYER_INDICATOR == 1)
        --TIME_COUNTER;
}

/* Poll both controllers, select sample groups, and cycle song/template modes. */
void func_03aec2()
{
    unsigned short saved_input;
    if (VBLANK_FLAG_8 != VBLANK_FLAG_7) {
        func_03e46a(VBLANK_FLAG_7);
        VBLANK_FLAG_8 = VBLANK_FLAG_7;
    } else
        MassiveRoutine();
    ++TIME_COUNTER;
    P1_PROCESSED_INPUT[0] = ram_ff801c ? GetCtrl2Input() : GetCtrl1Input();
    if (P1_PROCESSED_INPUT[0] != ram_ffe442) {
        ram_ffe458 = ram_ffe44e = P1_PROCESSED_INPUT[0];
        ram_ffe442 = P1_PROCESSED_INPUT[0];
    }
    P1_PROCESSED_INPUT[1] = ram_ff801c ? GetCtrl1Input() : GetCtrl2Input();
    if (P1_PROCESSED_INPUT[1] != ram_ffe44a) {
        ram_ffe45a = ram_ffe450 = P1_PROCESSED_INPUT[1];
        ram_ffe44a = P1_PROCESSED_INPUT[1];
    }
    if (PLAYER_INDICATOR == 1) {
        saved_input = ram_ffe458;
        ram_ffe458 = ram_ffe45a;
        ram_ffe45a = saved_input;
        ++TIME_COUNTER;
    }
    if (ram_ffe458) {
        if (ram_ffe458 & 128) {
            if (ram_ffe45c == 7) {
                func_03e43c();
                func_03e40e();
                ram_ffe45c = 0;
            } else
                VBLANK_FLAG_7 = ram_ffe45c++;
        }
        if (ram_ffe458 & 64) {
            if (!ram_ffe45c)
                gMusicUnknownE488 = 1;
            PlaySoundEffect(PCMPlaybackPointers[ram_ffe466], 90, 0);
        }
        if (ram_ffe458 & 16)
            PlaySoundEffect(PCMPlaybackPointers[ram_ffe468], 90, 0);
        if (ram_ffe458 & 32)
            PlaySoundEffect(PCMPlaybackPointers[ram_ffe46a], 90, 0);
        if (ram_ffe458 & 15) {
            ram_ffe466 = (int)GetRandomNumber() % 42;
            ram_ffe468 = (int)GetRandomNumber() % 42;
            ram_ffe46a = (int)GetRandomNumber() % 42;
        }
        ram_ffe458 = 0;
    }
    if (ram_ffe45a) {
        if (ram_ffe45a & 128) {
            if (++ram_ffe45e == 3)
                ram_ffe45e = 0;
            if (!ram_ffe45e) {
                func_03e43c();
                func_03e40e();
            } else {
                if (ram_ffe45e == 1)
                    ram_ffe476 = 0;
                else if (ram_ffe45e == 2)
                    ram_ffe476 = 1;
                VBLANK_FLAG_7 = VBLANK_FLAG_8;
                VBLANK_FLAG_8 = !VBLANK_FLAG_7;
            }
        }
        if (ram_ffe45a & 64) {
            if (!ram_ffe45c)
                gMusicUnknownE488 = 1;
            PlaySoundEffect(PCMPlaybackPointers[ram_ffe460], 90, 0);
        }
        if (ram_ffe45a & 16)
            PlaySoundEffect(PCMPlaybackPointers[ram_ffe462], 90, 0);
        if (ram_ffe45a & 32)
            PlaySoundEffect(PCMPlaybackPointers[ram_ffe464], 90, 0);
        if (ram_ffe45a & 15) {
            ram_ffe460 = (int)GetRandomNumber() % 42;
            ram_ffe462 = (int)GetRandomNumber() % 42;
            ram_ffe464 = (int)GetRandomNumber() % 42;
        }
        ram_ffe45a = 0;
    }
    if (PLAYER_INDICATOR == 1)
        --TIME_COUNTER;
}

/* Alternate completed lead animations and release their input lock. */
void func_03b27a()
{
    struct object_state *object;
    if (ram_ffe41a[0]) {
        object = &gPlayerInfo[ram_ffe41a[0]];
        if (!object->frame_duration) {
            if (ram_ffe43e == dat_09384e) {
                if (ram_ffe446 == dat_093834)
                    object->frame = ram_ffe44c;
                object->sequence = dat_093834;
            } else
                object->sequence = dat_09384e;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            ram_ffe43e = object->sequence;
        }
        ram_ffe444 = object->frame;
    }
    if (ram_ffe42c[0]) {
        object = &gPlayerInfo[ram_ffe42c[0]];
        if (!object->frame_duration) {
            if (ram_ffe446 == dat_09384e) {
                if (ram_ffe43e == dat_093834)
                    object->frame = ram_ffe444;
                object->sequence = dat_093834;
            } else
                object->sequence = dat_09384e;
            object->frame_duration = 1;
            object->frame = -1;
            object->frame_phase = 6;
            ram_ffe446 = object->sequence;
        }
        ram_ffe44c = object->frame;
    }
    if (ram_ffe43e == dat_09384e || ram_ffe43e == dat_093834)
        ram_ffe452 = 0;
    if (ram_ffe446 == dat_09384e || ram_ffe446 == dat_093834)
        ram_ffe454 = 0;
}

/* Switch object layouts every 170 or 68 updates and maintain plane scroll. */
void func_03b39e(reset)
short reset;
{
    if (reset) {
        ram_ffe412 = 0;
        ram_ffe410 = 170;
        ram_ffe414 = 0;
        ram_ffe416 = 0;
        ram_ffe418 = 0;
        VDP_COMMAND = 0x7c000003L;
        VDP_DATA = ram_ffe414;
        VDP_COMMAND = 0x40000010L;
        VDP_DATA = ram_ffe416;
        func_03b8ca();
    } else {
        if (ram_ffe418) {
            ram_ffe414 -= ram_ffe418;
            ram_ffe416 += ram_ffe418;
        } else {
            ram_ffe414 = 0;
            ram_ffe416 = 0;
        }
        VDP_COMMAND = 0x7c000003L;
        VDP_DATA = ram_ffe414;
        VDP_COMMAND = 0x40000010L;
        VDP_DATA = ram_ffe416;
        if (!--ram_ffe410) {
            ram_ffe410 = 170;
            if (++ram_ffe412 >= 6)
                ram_ffe412 = 0;
            ram_ffe452 = 0;
            ram_ffe454 = 0;
            switch (ram_ffe412) {
            case 0:
                func_03b8ca();
                goto stop_scroll;
            case 1:
                func_03ba14();
            /* The first two layouts converge on this scroll reset in the ROM.
             * Separate resets produce MCC68K's opposite tail-sharing order. */
            stop_scroll:
                ram_ffe418 = 0;
                break;
            case 2:
                func_027e42();
                func_03bae6();
                ram_ffe410 = 68;
                break;
            case 3:
                ram_ffe418 = 1;
                ram_ffe410 = 68;
                break;
            case 4:
                ram_ffe418 = -1;
                ram_ffe410 = 68;
                break;
            case 5:
                ram_ffe418 = 0;
                ram_ffe410 = 68;
                break;
            }
        }
    }
}

/* Copies mirror the lead object's frame, sprite size and tile attributes. */
void func_03b4c4()
{
    short object_number, piece, piece_count;
    struct object_state *object, *parent;
    struct hardware_sprite *parent_sprite, *sprite;
    for (object_number = 0, object = gPlayerInfo; object_number < 40;
         ++object_number, ++object) {
        if ((object->draw_flags & 1) && (object->flags_4d & 4)) {
            parent = &gPlayerInfo[object->parent];
            object->frame = parent->frame;
            object->sprite_ready = parent->sprite_ready;
            object->current_frame = parent->current_frame;
            piece_count = object->current_frame->count;
            for (piece = 0; piece < piece_count; ++piece) {
                parent_sprite = &ram_ffb65a[parent->primary_sprites[piece]];
                sprite = &ram_ffb65a[object->primary_sprites[piece]];
                sprite->size = parent_sprite->size;
                sprite->tile = parent_sprite->tile;
            }
        }
    }
}

/* Allocate nine copies per character, sharing the lead copy's tile storage. */
void func_03b564()
{
    short slot, tile_count, sprite_count;
    char parent_number;
    unsigned short lead_tile;
    struct object_state *object;
    tile_count = 16;
    sprite_count = 1;
    for (slot = 0; slot < 9; ++slot) {
        if (ram_ffe41a[slot] = (int)func_00d904()) {
            object = &gPlayerInfo[ram_ffe41a[slot]];
            object->tile = 0x8000;
            object->tile_count = tile_count;
            object->sprite_count = sprite_count;
            object->frames = Spr_JamoutToejam;
            object->x = 160;
            object->y = -50;
            object->sequence = dat_093834;
            object->frame_phase = 6;
            object->frame_duration = 1;
            object->frame = -1;
            if (slot)
                object->flags_4d |= 4;
            object->sprite_ready = 0;
            func_00d776(object, 0);
            if (!slot) {
                lead_tile = object->tile;
                parent_number = ram_ffe41a[slot];
            } else {
                object->tile = lead_tile;
                object->parent = parent_number;
            }
        }
    }
    tile_count = 19;
    sprite_count = 2;
    for (slot = 0; slot < 9; ++slot) {
        if (ram_ffe42c[slot] = (int)func_00d904()) {
            object = &gPlayerInfo[ram_ffe42c[slot]];
            object->tile = 0x8000;
            object->tile_count = tile_count;
            object->sprite_count = sprite_count;
            object->frames = Spr_JamoutEarl;
            object->x = 160;
            object->y = -50;
            object->sequence = dat_093834;
            object->frame_phase = 6;
            object->frame_duration = 1;
            object->frame = -1;
            if (slot)
                object->flags_4d |= 4;
            object->sprite_ready = 0;
            func_00d776(object, 0);
            if (!slot) {
                lead_tile = object->tile;
                parent_number = ram_ffe42c[slot];
            } else {
                object->tile = lead_tile;
                object->parent = parent_number;
            }
        }
    }
}

/* QUALIFICATION: assigning the handle to an unused temporary retains the ROM's
 * extra word reloads. This compiler-coaxing spelling is retained as an explicit qualification. */
void func_03b6d2()
{
    short slot, object_number;
    for (slot = 0; slot < 9; ++slot) {
        if (ram_ffe41a[slot]) {
            object_number = ram_ffe41a[slot];
            func_00d970(ram_ffe41a[slot]);
        }
        if (ram_ffe42c[slot]) {
            object_number = ram_ffe42c[slot];
            func_00d970(ram_ffe42c[slot]);
        }
    }
}

/* Move one allocated ToeJam copy. */
void func_03b73e(slot, x, y)
int slot, x, y;
{
    struct object_state *object;
    if (ram_ffe41a[slot]) {
        object = &gPlayerInfo[ram_ffe41a[slot]];
        object->x = x;
        object->y = y;
    }
}

/* Move one allocated Earl copy. */
void func_03b782(slot, x, y)
int slot, x, y;
{
    struct object_state *object;
    if (ram_ffe42c[slot]) {
        object = &gPlayerInfo[ram_ffe42c[slot]];
        object->x = x;
        object->y = y;
    }
}

/* Put a ToeJam copy at the common off-screen location. */
void func_03b7c6(slot)
int slot;
{
    struct object_state *object;
    if (ram_ffe41a[slot]) {
        object = &gPlayerInfo[ram_ffe41a[slot]];
        object->x = 160;
        object->y = -50;
    }
}

/* Put an Earl copy at the common off-screen location. */
void func_03b7fe(slot)
int slot;
{
    struct object_state *object;
    if (ram_ffe42c[slot]) {
        object = &gPlayerInfo[ram_ffe42c[slot]];
        object->x = 160;
        object->y = -50;
    }
}

/* Change the lead ToeJam animation unless its current action is locked. */
void func_03b836(sequence, period)
char *sequence;
short period;
{
    struct object_state *object;
    if (ram_ffe452)
        return;
    if (ram_ffe41a[0]) {
        object = &gPlayerInfo[ram_ffe41a[0]];
        object->sequence = sequence;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = period;
    }
    ram_ffe43e = sequence;
}

/* Change the lead Earl animation unless its current action is locked. */
void func_03b880(sequence, period)
char *sequence;
short period;
{
    struct object_state *object;
    if (ram_ffe454)
        return;
    if (ram_ffe42c[0]) {
        object = &gPlayerInfo[ram_ffe42c[0]];
        object->sequence = sequence;
        object->frame_duration = 1;
        object->frame = -1;
        object->frame_phase = period;
    }
    ram_ffe446 = sequence;
}

/* Show the two lead objects and upload the 16 by 22 packed background. */
void func_03b8ca()
{
    short row, column, tile;
    for (row = 1; row < 9; ++row) {
        func_03b7c6(row);
        func_03b7fe(row);
    }
    func_03b782(0, 174, 127);
    func_03b73e(0, 144, 127);
    func_0273ae();
    func_02743a();
    for (row = 0; row < 16; ++row) {
        VDP_COMMAND = 0x60120003L + ((row + 6) << 23);
        for (column = 0; column < 22; ++column) {
            tile = CrashPackedTilemapA[row][column];
            if (tile & 64)
                tile |= 0x800;
            if (tile & 128)
                tile |= 0x1000;
            tile &= 0xff3f;
            VDP_DATA = tile + 0xa233;
        }
    }
}

/* Upload the 14 by 20 packed background at a tile coordinate. */
void func_03b988(x, y)
short x, y;
{
    short row, column, tile;
    for (row = 0; row < 14; ++row) {
        VDP_COMMAND = (((y + row) << 23) + 0x60000003L) + (x << 17);
        for (column = 0; column < 20; ++column) {
            tile = CrashPackedTilemapB[row][column];
            if (tile & 64)
                tile |= 0x800;
            if (tile & 128)
                tile |= 0x1000;
            tile &= 0xff3f;
            VDP_DATA = tile + 0xa233;
        }
    }
}

/* Place four object pairs and draw four copies of the packed background. */
void func_03ba14()
{
    short slot;
    for (slot = 4; slot < 9; ++slot) {
        func_03b7c6(slot);
        func_03b7fe(slot);
    }
    func_03b73e(0, 65, 70);
    func_03b782(0, 95, 70);
    func_03b73e(1, 225, 70);
    func_03b782(1, 255, 70);
    func_03b73e(2, 65, 182);
    func_03b782(2, 95, 182);
    func_03b73e(3, 225, 182);
    func_03b782(3, 255, 182);
    func_02743a();
    func_03b988(0, 0);
    func_03b988(20, 0);
    func_03b988(0, 14);
    func_03b988(20, 14);
}

/* Arrange all nine copies of each character in a three by three grid. */
void func_03bae6()
{
    func_03b73e(0, 38, 53);
    func_03b782(0, 68, 53);
    func_03b73e(1, 144, 53);
    func_03b782(1, 174, 53);
    func_03b73e(2, 250, 53);
    func_03b782(2, 280, 53);
    func_03b73e(3, 38, 127);
    func_03b782(3, 68, 127);
    func_03b73e(4, 144, 127);
    func_03b782(4, 174, 127);
    func_03b73e(5, 250, 127);
    func_03b782(5, 280, 127);
    func_03b73e(6, 38, 201);
    func_03b782(6, 68, 201);
    func_03b73e(7, 144, 201);
    func_03b782(7, 174, 201);
    func_03b73e(8, 250, 201);
    func_03b782(8, 280, 201);
    func_02743a();
    func_0273ae();
}
