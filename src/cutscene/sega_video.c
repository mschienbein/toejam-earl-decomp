#include "psg_effects.h"
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
void SegaLogoClearVideoPlanes()
{
    ClearColourRAM();
    VDP_COMMAND = 0x7fe00001L;
    MoveDataToVRAM(VRAMClearingData, 16);
    VDP_COMMAND = VDP_WRITE_PLANE_A;
    FillVdpDataWords(0xe3ff, 2048);
    VDP_COMMAND = VDP_WRITE_PLANE_B;
    FillVdpDataWords(0x63ff, 2048);
}

/* Cycle the logo palette, then move the animated character across the screen. */
void SEGAVideo()
{
    short palette_frame, character_x, character_y;
    unsigned short palette_time, start_time, frame;

    SEGA_PAL_LOAD_FLAG = 0;
    gSegaVBlankSpriteUpdatesEnabled = 0;
    gVBlankFunc = SegaLogoVBlank;
    SegaLogoClearVideoPlanes();
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank0, 0);
    DrawStarfieldPlane();
    VDP_COMMAND = VDP_WRITE_VRAM_BASE;
    MoveDataToVRAM(SplashSegaLogo, 784);
    UploadVramWordRectangle(0xa51c, SEGALogoTileMap, 12, 4, 64);
    SEGA_PAL_TO_LOAD = SegaLogo_Pal;
    SEGA_PAL_LOAD_FLAG = 1;
    gSegaVBlankSpriteUpdatesEnabled = 1;
    frame = TIME_COUNTER;
    while (frame == TIME_COUNTER)
        ;
    gMode2RegSoftcopy |= 64;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
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
    if (!gSegaRocketPassShown) {
        frame = TIME_COUNTER;
        while (frame == TIME_COUNTER)
            ;
        gSegaVBlankSpriteUpdatesEnabled = 0;
        SEGA_PAL_LOAD_FLAG = 0;
        SegaLogoCreateTrademarkEntity();
        CutsceneAllocateRocketshipPair();
        CopyPaletteToCRAMSlot(PaletteSpaceScreen_bank2, 2);
        character_x = 280;
        character_y = 150;
        CutsceneSetRocketshipPairPose(0, character_x, character_y);
        gSegaVBlankSpriteUpdatesEnabled = 1;
        palette_time = TIME_COUNTER;
        while (character_x > -50) {
            UpdateAndQueueEntitySprites();
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
                    PlaySound(PSG_TONE_RISE, 99);
            }
            CutscenePositionRocketshipPair(--character_x, character_y);
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
        CutsceneReleaseRocketshipPair();
        SegaLogoReleaseTrademarkEntity();
        gSegaRocketPassShown = 1;
    }
    frame = TIME_COUNTER;
    while (frame == TIME_COUNTER)
        ;
    gVBlankFunc = 0;
}

/* Allocate and initialize the trademark sprite used below the logo. */
void SegaLogoCreateTrademarkEntity()
{
    struct object_state *trademark;

    if (gSegaTrademarkEntity = (int)AllocateEntity()) {
        trademark = &gPlayerInfo[gSegaTrademarkEntity];
        trademark->tile = VDP_TILE_PRIORITY;
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
        AllocateEntityViewportSprites(trademark, 0);
    }
}

/* Release the trademark sprite when the logo sequence finishes. */
void SegaLogoReleaseTrademarkEntity()
{
    if (gSegaTrademarkEntity)
        FreeEntity(gSegaTrademarkEntity);
    gSegaTrademarkEntity = 0;
}
