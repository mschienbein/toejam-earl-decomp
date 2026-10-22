#include "hardware.h"
#include "revision.h"
#include "entities.h"
#include "assets_audio.h"
#include "assets_cutscene.h"
#include "assets_intro.h"
#include "assets_palettes.h"
#include "audio_calls.h"
#include "graphics_calls.h"
#include "hardware_calls.h"
#include "intro_object_calls.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "ram_system.h"
#include "sprite_calls.h"
#include "title_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Clear CRAM and prepare the two logo tilemap planes. */
void func_03a48a()
{
    ClearColourRAM();
    VDP_COMMAND = 0x7fe00001L;
    MoveDataToVRAM(VRAMClearingData, 16);
    VDP_COMMAND = 0x60000002L;
    func_026fc2(0xe3ff, 2048);
    VDP_COMMAND = 0x60000003L;
    func_026fc2(0x63ff, 2048);
}

/* Cycle the logo palette, then move the animated character across the screen. */
void SEGAVideo()
{
    short palette_frame, character_x, character_y;
    unsigned short palette_time, start_time, frame;

    SEGA_PAL_LOAD_FLAG = 0;
    ram_ffe402 = 0;
    gVBlankFunc = SegaLogoVBlank;
    func_03a48a();
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    func_00046a();
    VDP_COMMAND = 0x40000000L;
    MoveDataToVRAM(SplashSegaLogo, 784);
    func_027004(0xa51c, SEGALogoTileMap, 12, 4, 64);
    SEGA_PAL_TO_LOAD = SegaLogo_Pal;
    SEGA_PAL_LOAD_FLAG = 1;
    ram_ffe402 = 1;
    frame = TIME_COUNTER;
    while (frame == TIME_COUNTER)
        ;
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
#if REVISION >= 2
    TIME_COUNTER = 0;
#endif
    palette_time = TIME_COUNTER;
    start_time = TIME_COUNTER;
    palette_frame = 0;
    while (TIME_COUNTER < start_time + 120) {
        if (TIME_COUNTER >= palette_time + 3) {
            palette_time = TIME_COUNTER;
            SEGA_PAL_TO_LOAD += 16;
            if (++palette_frame >= 20) {
                SEGA_PAL_TO_LOAD = SegaLogo_Pal;
                palette_frame = 0;
            }
            SEGA_PAL_LOAD_FLAG = 1;
        }
    }
    if (!ram_ffe404) {
        frame = TIME_COUNTER;
        while (frame == TIME_COUNTER)
            ;
        ram_ffe402 = 0;
        SEGA_PAL_LOAD_FLAG = 0;
        func_03a70e();
        func_028b80();
        CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
        character_x = 280;
        character_y = 150;
        func_028c8a(0, character_x, character_y);
        ram_ffe402 = 1;
        palette_time = TIME_COUNTER;
        while (character_x > -50) {
            func_00cc82();
            frame = TIME_COUNTER;
            while (frame == TIME_COUNTER)
                ;
            if (character_x == 257)
                PlaySoundEffect((long)Sfx_Achoo, 50, 0);
            if (character_x < 57 && character_x > 44)
                character_y += 2;
            else if (character_x > 178 && character_x < 191) {
                character_y -= 2;
                if (character_x == 190)
                    PlaySound(21, 99);
            }
            func_028e36(--character_x, character_y);
            if (TIME_COUNTER >= palette_time + 3) {
                palette_time = TIME_COUNTER;
                SEGA_PAL_TO_LOAD += 16;
                if (++palette_frame >= 20) {
                    SEGA_PAL_TO_LOAD = SegaLogo_Pal;
                    palette_frame = 0;
                }
                SEGA_PAL_LOAD_FLAG = 1;
            }
        }
        func_028c4c();
        func_03a784();
        ram_ffe404 = 1;
    }
    frame = TIME_COUNTER;
    while (frame == TIME_COUNTER)
        ;
    gVBlankFunc = 0;
}

/* Allocate and initialize the trademark sprite used below the logo. */
void func_03a70e()
{
    struct object_state *trademark;

    if (ram_ffe400 = (int)func_00d904()) {
        trademark = &gPlayerInfo[ram_ffe400];
        trademark->tile = 0x8000;
        trademark->current_frame = Spr_Trademark[0];
        trademark->frames = Spr_Trademark;
        trademark->tile_count = 108;
        trademark->sprite_count = 10;
        trademark->x = 160;
        trademark->y = 166;
        trademark->sprite_ready = 0;
        trademark->sequence = (char *)IntroHeldFrameStreams;
        trademark->frame_duration = 1;
        trademark->frame = -1;
        trademark->frame_phase = 6;
        func_00d776(trademark, 0);
    }
}

/* Release the trademark sprite when the logo sequence finishes. */
void func_03a784()
{
    if (ram_ffe400)
        func_00d970(ram_ffe400);
    ram_ffe400 = 0;
}
