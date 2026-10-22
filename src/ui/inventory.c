#include "entities.h"
#include "presents.h"
#include "hardware.h"
#include "revision.h"
#include "assets_audio.h"
#include "assets_presents.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "hardware_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Select black or dark blue for the text background colour. */
void ForceTextBackgroundColorToBlack(force_black)
short force_black;
{
    short black = 0;
    /* The observed zero write is MOVE.W, not CLR.W. Keeping a word colour
     * also avoids a CLR access to the VDP data port. [INFERENCE] This may
     * be why the original materializes zero in a register. */

    VDP_COMMAND = 0xc01e0000L;
    if (force_black)
        VDP_DATA = black;
    else
        VDP_DATA = 0x200;
}

/* Copy both 1,888-byte font banks into consecutive RAM buffers. */
void TextCopyFontBanksToRam()
{
    long *source = ImageTextChars1;
    long *destination = (long *)TEXT_SPRITE_DATA_START;
    short bank, word;

    for (bank = 0; bank < 2; bank++) {
        for (word = 0; word < 472; word++)
            *destination++ = *source++;
        source = ImageTextChars2;
    }
}

void SetVHBlankFlag2()
{
    VHBLANK_FLAG_2 = 1;
}

/* Keep text animation enabled while either menu uses it. */
void ClearVHBlankFlag2()
{
    if (!P1_MENU_OPEN[0] && !HBLANK_FLAG_1)
        VHBLANK_FLAG_2 = 0;
}

/* Alternate the two RAM font banks on every rate-th animation tick. */
int TextAnimateFontBanks(rate)
short rate;
{
    if (!VHBLANK_FLAG_2) {
        if (!P1_TEXT_WIGGLE)
            return 0;
        P1_MENU_COUNTER = 0;
    }
    if (!(P1_MENU_COUNTER++ % rate)) {
        P1_TEXT_WIGGLE ^= 1;
        VDPWritesRoutine1(TEXT_SPRITE_DATA_START[P1_TEXT_WIGGLE], 0xb680, 944);
        return 1;
    }
    return 0;
}

/* Bracket a single vertical blank with the graphics update flag. */
void WaitForVBlankWithGraphicsPaused()
{
    VBLANK_FLAG_3 = 1;
    WaitForNextVBlank(1);
    VBLANK_FLAG_3 = 0;
}

/* Hide active sprite links belonging to the requested viewport. */
void EntityClearViewportSprites(viewport)
long viewport;
{
    short object_number;
    char viewport_mask;
    struct object_state *object;

    while (VBLANK_FLAG_2)
        ;
    if (viewport == 0)
        viewport_mask = 2;
    else
        viewport_mask = 4;
    object = gPlayerInfo;
    for (object_number = 0; object_number < 26; object_number++, object++) {
        if ((object->draw_flags & 1) && (object->draw_flags & viewport_mask))
            ReleaseEntityViewportSprites(object, viewport);
    }
    PositionEntitySprites(gEntityDrawListHead);
    BuildViewportSpriteLinks();
    VBLANK_FLAG_2 = 1;
    WaitForNextVBlank(1);
}

/* Advance the four-part background upload and then draw menu contents. */
void InventoryServiceRedrawPhase()
{
    short column, row, row_end;
    unsigned short tile;

    if (VBLANK_FLAG_4 >= 3) {
        if (!gInventoryRedrawPlayer) {
            if (VBLANK_FLAG_4 == 4) {
                row = 0;
                row_end = 7;
            } else {
                row = 7;
                row_end = 14;
            }
        } else {
            if (VBLANK_FLAG_4 == 4) {
                row = 14;
                row_end = 21;
            } else {
                row = 21;
                row_end = 28;
            }
        }
        for (; row < row_end; row++) {
            VDP_COMMAND = 0x50000003L + ((long)row << 23);
            for (column = 0; column < 40; column++) {
                tile = FontBackgroundByteMap[row][column];
                if (tile & 0x40)
                    tile |= 0x800;
                if (tile & 0x80)
                    tile |= 0x1000;
                tile &= 0xff3f;
                VDP_DATA = tile + 0x8580;
            }
        }
    } else if (VBLANK_FLAG_4 == 2) {
        ForceTextBackgroundColorToBlack(1);
        InventoryDrawPlayerStats(gInventoryRedrawPlayer);
        InventoryDrawActionBanner(gInventoryRedrawPlayer, gInventoryBannerMode[gInventoryRedrawPlayer], 1);
        DrawHUD();
    } else if (P1_MENU_OPEN[gInventoryRedrawPlayer] == 2)
        InventoryDrawMailOrderOffers(gInventoryRedrawPlayer);
    else {
        LoadPresentsIntoInventory(gInventoryRedrawPlayer);
        if (P1_MENU_OPEN[gInventoryRedrawPlayer] == 3)
            InventoryDrawBucks(gInventoryRedrawPlayer);
    }
    VBLANK_FLAG_4--;
}

/* Pick three distinct non-excluded mailbox gifts and prices with a level seed. */
void SetMailOrderPresents(player_number, level)
short player_number, level;
{
    short present, slot;
    long saved_random = StackBasedRandom(gLevelSeeds[level]);

    for (slot = 0; slot < 3; slot++) {
        present = SelectRandomPresent();
        if (present == 17 || present == 11 || present == 21 ||
            (slot > 0 && gPlayer1MailboxItems[player_number][0] == present) ||
            (slot > 1 && gPlayer1MailboxItems[player_number][1] == present)) {
            slot--;
            continue;
        }
        gPlayer1MailboxItems[player_number][slot] = present;
        gPlayer1MailboxPrices[player_number][slot] = GetRandomNumber() % 3 + 3;
    }
    StackBasedRandom(saved_random);
}

/* Build one inventory view and wait for its blanking-driven draw phases. */
void DrawInventory(player_number, menu_kind)
short player_number, menu_kind;
{
    short viewport;
    struct object_state *player;

    if (player_number == 0)
        viewport = 0;
    else
        viewport = 1;
    player = &gPlayerInfo[player_number];
    if (SPLIT_SCREEN) {
        EntityClearViewportSprites(viewport);
        player->flags_4d |= 16;
        player->frame_duration = 0;
    }
    if (menu_kind == 1)
        gInventoryBannerMode[player_number] = 0;
    else if (menu_kind == 2) {
        gInventoryBannerMode[player_number] = 2;
        SetMailOrderPresents(player_number, player->level);
    } else
        gInventoryBannerMode[player_number] = 3;
#if REVISION >= 2
    P1_IDLE_COUNTER[player_number] = 0;
#endif
    InventoryInitializeDisplayEntities(player_number);
    gInventoryRedrawPlayer = player_number;
    VBLANK_FLAG_4 = 4;
    while (VBLANK_FLAG_4) {
        if (!P1_MENU_OPEN[player_number] && VBLANK_FLAG_4 <= 2)
            P1_MENU_OPEN[player_number] = menu_kind;
    }
    if (!SPLIT_SCREEN) {
        EARL_CAMERA_NEWX = 0;
        EARL_CAMERA_NEWY = 112;
    }
    SetVHBlankFlag2();
}

/* Save and replace the open inventory cameras, or restore their positions. */
void InventorySetCameraSuspension(suspend)
int suspend;
{
    if (!SPLIT_SCREEN)
        return;
    if (P1_MENU_OPEN[0]) {
        if (suspend) {
            gInventorySavedCamera1X = TOEJAM_CAMERA_NEWX;
            gInventorySavedCamera1Y = TOEJAM_CAMERA_NEWY;
            TOEJAM_CAMERA_NEWX = 0;
            TOEJAM_CAMERA_NEWY = 0;
        } else {
            TOEJAM_CAMERA_NEWX = gInventorySavedCamera1X;
            TOEJAM_CAMERA_NEWY = gInventorySavedCamera1Y;
        }
    }
    if (HBLANK_FLAG_1) {
        if (suspend) {
            gInventorySavedCamera2X = EARL_CAMERA_NEWX;
            gInventorySavedCamera2Y = EARL_CAMERA_NEWY;
            EARL_CAMERA_NEWX = 0;
            EARL_CAMERA_NEWY = 112;
        } else {
            EARL_CAMERA_NEWX = gInventorySavedCamera2X;
            EARL_CAMERA_NEWY = gInventorySavedCamera2Y;
        }
    }
}

/* Request redraw mode four and service it until the VBlank flag clears. */
void InventoryRedrawSynchronously(player_number)
int player_number;
{
    gInventoryRedrawPlayer = player_number;
    VBLANK_FLAG_4 = 4;
    while (VBLANK_FLAG_4)
        InventoryServiceRedrawPhase();
}

/* Wait for the outstanding redraw, request mode one, and wait again. */
void InventoryRefreshContentsAndWait(player_number)
short player_number;
{
    while (VBLANK_FLAG_4)
        ;
    gInventoryRedrawPlayer = player_number;
    VBLANK_FLAG_4 = 1;
    while (VBLANK_FLAG_4)
        ;
}

/* Close an existing inventory, restoring its camera/display state, or open it. */
void ToggleInventory(player_number)
short player_number;
{
    struct object_state *object;

    if (P1_MENU_OPEN[player_number]) {
        InventoryDeactivateDisplayEntities(player_number);
        if (SPLIT_SCREEN) {
            P1_MENU_OPEN[player_number] = 0;
            if (!P1_MENU_OPEN[player_number ^ 1] && !PlayersRequireSeparateViews())
                UpdateScreenSplitMode(1);
            else
                VBLANK_FLAG_5 = 1;
            object = &gPlayerInfo[player_number];
            object->flags_4d &= ~16;
            if (object->frame_phase)
                object->frame_duration = 1;
            UpdateEntityViewportSprites(player_number);
        } else {
            if (!P1_MENU_OPEN[player_number ^ 1]) {
                EARL_CAMERA_NEWX = TOEJAM_CAMERA_NEWX;
                EARL_CAMERA_NEWY = TOEJAM_CAMERA_NEWY;
            }
            P1_MENU_OPEN[player_number] = 0;
            WaitForVBlankWithGraphicsPaused();
            ClearSkipVHBlankFlags();
            DrawHUD();
            if (!P1_MENU_OPEN[player_number ^ 1])
                ForceTextBackgroundColorToBlack(0);
            SetSkipVHBlankFlags();
        }
        ClearVHBlankFlag2();
    } else if (!ELEVATOR_FLAG[player_number] || !gElevatorDepartureInProgress)
        DrawInventory(player_number, 1);
}

/* Apply a just-opened present to the other living player when appropriate. */
void CheckIfOpenForBoth(opening)
short *opening;
{
    short present, player_number;

    if (!opening[0] && !opening[1])
        return;
    if (opening[0])
        player_number = 0;
    else
        player_number = 1;
    present = OpenPresent(player_number);
    if (present && gPlayerLives[player_number ^ 1] >= 0) {
        if (!PlayersRequireSeparateViews()) {
            --present;
            if (P1_MENU_OPEN[player_number ^ 1])
                ToggleInventory(player_number ^ 1);
            if (present != 8 && present != 15 && present != 12 &&
                present != 19 && present != 23 && present != 25) {
                if (!OpenSelectedPresent(player_number ^ 1, present) &&
                    present == 16)
                    GenerateSpeech(player_number ^ 1, "Thanks a lot", 1);
            }
        } else if (P1_MENU_OPEN[player_number ^ 1])
            InventoryRefreshContentsAndWait(player_number ^ 1);
    }
}

/* Replace occupied slots except present 27 for each player. */
void RandomizeInventory()
{
    short slot, player_number;

    for (player_number = 0; player_number <= 1; ++player_number)
        for (slot = 0; slot < 16; ++slot)
            if (gPlayerInventory[player_number][slot] != -1 &&
                gPlayerInventory[player_number][slot] != 27)
                gPlayerInventory[player_number][slot] = SelectRandomPresent();
}

/* Animate sixty inventory rerolls and restore an inventory opened here. */
void RandomizePresents(player_number)
int player_number;
{
    short frame, close_other = 0;

    SetupPresentIdentities();
    InventoryRefreshContentsAndWait(player_number);
    if (gPlayerLives[player_number ^ 1] >= 0) {
        if (!P1_MENU_OPEN[player_number ^ 1]) {
            DrawInventory(player_number ^ 1, 1);
            InventoryUpdateSelectorEntity(player_number ^ 1);
            close_other = 1;
        } else
            InventoryRefreshContentsAndWait(player_number ^ 1);
    }
    gPlayer1InventorySelectorFlags |= 64;
    gPlayer2InventorySelectorFlags |= 64;
    gInventoryLabelsSuppressed = 1;
    PlaySoundEffect((long)Sfx_Randomize, 96, 1);
    for (frame = 0; frame < 60; ++frame) {
        RandomizeInventory();
        InventoryRefreshContentsAndWait(player_number);
        UpdateAndQueueEntitySprites();
        InventoryRefreshContentsAndWait(player_number ^ 1);
        UpdateAndQueueEntitySprites();
        WaitForNextVBlank(frame / 5);
        PlaySound(7, 50);
    }
    gInventoryLabelsSuppressed = 0;
    PresentClearWorldEntityReadiness();
    if (close_other)
        ToggleInventory(player_number ^ 1);
}

/* Process button edges and inventory cursor movement for the two players. */
void ProcessInventory()
{
    short opening[2];
    short player_number;
    struct object_state *object;

    opening[0] = 0;
    opening[1] = 0;
    for (player_number = 0; player_number <= 1; ++player_number) {
        object = &gPlayerInfo[player_number];
        if (gPlayerLives[player_number] < 0 || object->sprite_state == 5 ||
            object->sprite_state == 6)
            continue;
        if (!(P1_PREV_PROCESSED_INPUT[player_number] & 16) &&
            (P1_PROCESSED_INPUT[player_number] & 16)) {
            ToggleInventory(player_number);
            return;
        }
        if (P1_MENU_OPEN[player_number]) {
            if (P1_MENU_OPEN[player_number] == 1 &&
                !(P1_PREV_PROCESSED_INPUT[player_number] & 32) &&
                (P1_PROCESSED_INPUT[player_number] & 32)) {
                gInventoryBannerMode[player_number] ^= 1;
                InventoryDrawActionBanner(player_number, gInventoryBannerMode[player_number], 0);
                PlaySound(7, 50);
            } else if (!(P1_PREV_PROCESSED_INPUT[player_number] & 64) &&
                       (P1_PROCESSED_INPUT[player_number] & 64)) {
                if (P1_MENU_OPEN[player_number] == 2) {
                    ToggleInventory(player_number);
                    InventoryBuySelectedMailOrderPresent(player_number);
                } else if (P1_MENU_OPEN[player_number] == 3) {
                    if (InventoryIdentifySelectedPresent(player_number)) {
                        InventoryRefreshContentsAndWait(player_number);
                        if (P1_MENU_OPEN[player_number ^ 1])
                            InventoryRefreshContentsAndWait(player_number ^ 1);
                    }
                } else if (gInventoryBannerMode[player_number] == 1) {
                    InventoryDropSelectedPresent(player_number);
                    InventoryRefreshContentsAndWait(player_number);
                } else {
                    P1_PREV_PROCESSED_INPUT[player_number] |= 64;
                    if (InventoryGetSelectedPresent(player_number) == 18)
                        RandomizePresents(player_number);
                    else {
                        ToggleInventory(player_number);
                        opening[player_number] = 1;
                    }
                }
            } else if (!(P1_PREV_PROCESSED_INPUT[player_number] & 15) &&
                       (P1_PROCESSED_INPUT[player_number] & 15)) {
                if (P1_PROCESSED_INPUT[player_number] & 1) {
                    if (P1_MENU_SEL_ROW[player_number] > 0)
                        --P1_MENU_SEL_ROW[player_number];
                    else if (P1_MENU_OPEN[player_number] != 2 &&
                             P1_MENU_SCROLL[player_number] > 0) {
                        --P1_MENU_SCROLL[player_number];
                        InventoryRefreshContentsAndWait(player_number);
                    }
                } else if (P1_PROCESSED_INPUT[player_number] & 2) {
                    if (P1_MENU_SEL_ROW[player_number] < 2)
                        ++P1_MENU_SEL_ROW[player_number];
                    else if (P1_MENU_OPEN[player_number] != 2 &&
                             P1_MENU_SCROLL[player_number] < 5 &&
                             gPlayerInventory[player_number]
                                 [(P1_MENU_SCROLL[player_number] + 3) * 2] != -1) {
                        ++P1_MENU_SCROLL[player_number];
                        InventoryRefreshContentsAndWait(player_number);
                    }
                } else if (P1_MENU_OPEN[player_number] != 2)
                    P1_MENU_SEL_COL[player_number] ^= 1;
                PlaySound(7, 50);
            }
            if (P1_MENU_OPEN[player_number])
                InventoryUpdateSelectorEntity(player_number);
            if (opening[player_number])
                break;
        }
    }
    CheckIfOpenForBoth(opening);
    if (gPendingUnfallPlayer) {
        player_number = gPendingUnfallPlayer - 1;
        if (P1_MENU_OPEN[player_number ^ 1])
            ToggleInventory(player_number ^ 1);
        PlayerRelocateToPartnerOrNextLevel(player_number);
        gPendingUnfallPlayer = 0;
    }
}

/* Run the non-split inventory input loop until both menus close. */
void CheckBButton()
{
    /* ORIGINAL BUG: the modulo-three service counter is not initialized. */
    short frame;

    ProcessInventory();
    if ((P1_MENU_OPEN[0] || P1_MENU_OPEN[1]) && !SPLIT_SCREEN) {
        while (P1_MENU_OPEN[0] || P1_MENU_OPEN[1]) {
            if (++frame % 3 == 0)
                UpdateAndQueueEntitySprites();
            else
                WaitForNextVBlank(1);
            GetPlayerInput();
            ProcessInventory();
        }
    }
}

/* Open inventory mode two with the present-action button already held. */
void InventoryOpenMailOrderMenu(player_number)
short player_number;
{
    DrawInventory(player_number, 2);
    P1_PROCESSED_INPUT[player_number] = 64;
    P1_PREV_PROCESSED_INPUT[player_number] = 64;
    CheckBButton();
}

/* Draw the inventory, set both processed-input bytes to the present-action
 * bit (0x40), then run CheckBButton. */
void InventoryOpenIdentificationMenu(player)
short player;
{
    DrawInventory(player, 3);
    P1_PROCESSED_INPUT[player] = 64;
    P1_PREV_PROCESSED_INPUT[player] = 64;
    CheckBButton();
}

/* Return the present byte at the current scrolled inventory selection. */
char InventoryGetSelectedPresent(player)
int player;
{
    char slot;

    slot = (P1_MENU_SCROLL[player] + P1_MENU_SEL_ROW[player]) * 2 +
           P1_MENU_SEL_COL[player];
    return gPlayerInventory[player][slot];
}

/* Resolve random presents, open the selected one, and close its inventory gap. */
short OpenPresent(player)
short player;
{
    char slot, present;

    slot = (P1_MENU_SCROLL[player] + P1_MENU_SEL_ROW[player]) * 2 +
           P1_MENU_SEL_COL[player];
    present = gPlayerInventory[player][slot];
    while (present == 26)
        present = SelectRandomPresent();
    if (present == -1 || OpenSelectedPresent(player, present)) {
        PlaySound(26, 50);
        return 0;
    }
    if (gPlayerInventory[player][slot] != 26)
        gPresentInfo[present].identified = 1;
    while (slot < 15 && gPlayerInventory[player][slot + 1] != -1) {
        gPlayerInventory[player][slot] = gPlayerInventory[player][slot + 1];
        ++slot;
    }
    gPlayerInventory[player][slot] = -1;
    return present + 1;
}

/* Reject dropping in the elevator; otherwise remove and drop the selection. */
void InventoryDropSelectedPresent(player)
short player;
{
    char slot, present;

    if (ELEVATOR_FLAG[player]) {
        PlaySound(26, 50);
        return;
    }
    slot = (P1_MENU_SCROLL[player] + P1_MENU_SEL_ROW[player]) * 2 +
           P1_MENU_SEL_COL[player];
    present = gPlayerInventory[player][slot];
    while (slot < 15 && gPlayerInventory[player][slot + 1] != -1) {
        gPlayerInventory[player][slot] = gPlayerInventory[player][slot + 1];
        ++slot;
    }
    gPlayerInventory[player][slot] = -1;
    if (present != -1)
        DropPresent(player, present);
}

/* Buy the selected mailbox item when both inventory space and bucks allow it. */
void InventoryBuySelectedMailOrderPresent(player)
short player;
{
    char slot, selection;

    for (slot = 0; slot < 16 && gPlayerInventory[player][slot] != -1; ++slot)
        ;
    selection = P1_MENU_SEL_ROW[player];
    if (slot < 16 && gPlayerBucks[player] >= gPlayer1MailboxPrices[player][selection]) {
        gPlayerBucks[player] -= gPlayer1MailboxPrices[player][selection];
        gPlayerInventory[player][slot] = gPlayer1MailboxItems[player][selection];
        GenerateSpeech(player, "Got it!", 1);
    } else {
        PlaySound(26, 50);
        if (slot == 16)
            StartDialogue(25, player);
        else
            GenerateSpeech(player, "Need Bucks", 1);
    }
}

/* Resolve and identify the selected present, charging two bucks only on change. */
int InventoryIdentifySelectedPresent(player)
short player;
{
    int changed = 0;
    char slot, present;

    slot = (P1_MENU_SCROLL[player] + P1_MENU_SEL_ROW[player]) * 2 +
           P1_MENU_SEL_COL[player];
    present = gPlayerInventory[player][slot];
    if (gPlayerBucks[player] < 2 || present == -1) {
        PlaySound(26, 50);
        return 0;
    }
    while (present == 26) {
        present = SelectRandomPresent();
        changed = 1;
    }
    gPlayerInventory[player][slot] = present;
    if (!gPresentInfo[present].identified) {
        gPresentInfo[present].identified = 1;
        changed = 1;
    }
    if (changed)
        gPlayerBucks[player] -= 2;
    else
        PlaySound(26, 50);
    return changed;
}

/* Map ASCII and embedded control characters to font glyphs. Some controls
 * emit preceding tile words before returning the final glyph. The question
 * mark cycles through 59 words to choose one of four replacement glyphs.
 * MCC68K 3.3K has no volatile keyword; these explicit MMIO writes are retained
 * by its matching default optimization profile. */
char TextMapCharacterToGlyph(character)
char character;
{
    if (character >= 'a' && character <= 'z')
        character += 1 - 'a';
    else if (character >= 'A' && character <= 'Z')
        character += 1 - 'A';
    else if (character >= '0' && character <= '9')
        character += 36 - '0';
    else switch (character) {
    case '?':
        character = (PREGEN_RANDOMS[gRandomCacheIndex == 58 ?
                     (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 3) + 27;
        break;
    case '.': character = 31; break;
    case ',': character = 32; break;
    case ':':
    case '\'': character = 33; break;
    case '-': character = 34; break;
    case '!': character = 35; break;
    case 1: character = 46; break;
    case 2:
        VDP_DATA = 0x85e3;
        VDP_DATA = 0x85e4;
        character = 49;
        break;
    case 3:
        VDP_DATA = 0x85e6;
        character = 51;
        break;
    case 4:
        VDP_DATA = 0x85e8;
        character = 53;
        break;
    case 5:
        VDP_DATA = 0x85ea;
        VDP_DATA = 0x85eb;
        character = 56;
        break;
    case 6:
        VDP_DATA = 0x85ed;
        character = 58;
        break;
    default: character = 0;
    }
    return character;
}

/* Emit glyph tile words for a zero-terminated character string. */
void TextWriteStringTiles(text)
char *text;
{
    char glyph;

    while (*text) {
        glyph = TextMapCharacterToGlyph(*text++);
        VDP_DATA = 0x85b4 + glyph;
    }
}

/* Suppress leading zeroes, retaining one digit and a preceding minus sign. */
void TextWriteDecimalTiles(value)
short value;
{
    short count, divisor, position, started, digit;
    char text[10];

    count = 0;
    started = 0;
    if (value < 0) {
        text[count++] = '-';
        /* ORIGINAL BUG: negating -32768 narrows back to a negative short. */
        value = -value;
    }
    divisor = 10000;
    for (position = 0; position < 5; ++position) {
        if ((digit = value / divisor) != 0 || started) {
            value %= divisor;
            text[count++] = digit + '0';
            started = 1;
        }
        divisor /= 10;
        if (position == 3)
            started = 1;
    }
    text[count] = 0;
    TextWriteStringTiles(text);
}

/* Render a three-character currency field, padding short values with blanks. */
void InventoryDrawBucks(player_number)
short player_number;
{
    short column = 6;
    if (player_number == 0) {
        VDP_COMMAND = 0x56000003L + (column << 17);
        if (gPlayerBucks[0] < 100)
            VDP_DATA = 0x85b4;
        TextWriteDecimalTiles(gPlayerBucks[0]);
        if (gPlayerBucks[0] < 10)
            VDP_DATA = 0x85b4;
    } else {
        VDP_COMMAND = 0x5c800003L + (column << 17);
        if (gPlayerBucks[1] < 100)
            VDP_DATA = 0x85b4;
        TextWriteDecimalTiles(gPlayerBucks[1]);
        if (gPlayerBucks[1] < 10)
            VDP_DATA = 0x85b4;
    }
}

/* Render currency, points and the player's rank glyph sequence. */
void InventoryDrawPlayerStats(player_number)
short player_number;
{
    short column = 32;
    InventoryDrawBucks(player_number);
    if (player_number == 0) {
        if (gPlayerPoints[0] > 99)
            --column;
        VDP_COMMAND = 0x56000003L + (column << 17);
        TextWriteDecimalTiles(gPlayerPoints[0]);
        if (gPlayerPoints[0] > 999)
            TextWriteStringTiles(" pts");
        VDP_COMMAND = 0x55a80003L;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[0]]);
    } else {
        if (gPlayerPoints[1] > 99)
            --column;
        VDP_COMMAND = 0x5c800003L + (column << 17);
        TextWriteDecimalTiles(gPlayerPoints[1]);
        if (gPlayerPoints[1] > 999)
            TextWriteStringTiles(" pts");
        VDP_COMMAND = 0x5c280003L;
        TextWriteStringTiles(RankCharsTable[gPlayer2Rank]);
    }
}

/* Retreat over empty inventory slots and position or hide the selector. */
void InventoryUpdateSelectorEntity(player_number)
short player_number;
{
    char exhausted = 0;
    struct object_state *selector;

    if (P1_MENU_OPEN[player_number] != 2) {
        while (gPlayerInventory[player_number]
               [(P1_MENU_SCROLL[player_number] + P1_MENU_SEL_ROW[player_number]) * 2 +
                P1_MENU_SEL_COL[player_number]] == -1) {
            if (P1_MENU_SEL_ROW[player_number] == 0 &&
                P1_MENU_SEL_COL[player_number] == 0) {
                exhausted = 1;
                break;
            }
            if (P1_MENU_SEL_COL[player_number] > 0)
                --P1_MENU_SEL_COL[player_number];
            else {
                --P1_MENU_SEL_ROW[player_number];
                ++P1_MENU_SEL_COL[player_number];
            }
        }
    }
    if (player_number == 0)
        selector = &gPlayer1InventorySelectorEntity;
    else
        selector = &gPlayer2InventorySelectorEntity;
    if (!exhausted) {
        selector->x = P1_MENU_SEL_COL[player_number] * 160 + 7;
        selector->y = P1_MENU_SEL_ROW[player_number] * 16 + 63;
        selector->flags_4d &= ~64;
        if (player_number == 1)
            selector->y += 104;
    } else
        selector->flags_4d |= 64;
}

/* Activate seven inventory objects, waiting for their selected draw flag. */
void InventoryInitializeDisplayEntities(player_number)
short player_number;
{
    short group, first_object, object_number;
    char ready_mask;
    struct object_state *object;

    P1_MENU_SEL_ROW[player_number] = 0;
    P1_MENU_SEL_COL[player_number] = 0;
    P1_MENU_SCROLL[player_number] = 0;
    group = 1;
    ready_mask = 4;
    if (player_number == 0) {
        first_object = 26;
        if (SPLIT_SCREEN) {
            group = 0;
            ready_mask = 2;
        }
    } else
        first_object = 33;
    object = &gPlayerInfo[first_object];
    for (object_number = first_object; object_number < first_object + 7;
         ++object_number, ++object) {
        object->draw_flags = 1;
        for (AllocateEntityViewportSprites(object, group); !(object->draw_flags & ready_mask);
             AllocateEntityViewportSprites(object, group))
            EntityReleaseOneByPriority();
        object->sprite_ready = 0;
        object->flags_4d |= 64;
    }
    gPlayerInfo[player_number].draw_flags |= 8;
}

/* Remove each inventory object from its draw group and clear player flag 8. */
void InventoryDeactivateDisplayEntities(player_number)
short player_number;
{
    short group, first_object, object_number;
    struct object_state *object;

    group = 1;
    if (player_number == 0) {
        first_object = 26;
        if (SPLIT_SCREEN)
            group = 0;
    } else
        first_object = 33;
    for (object_number = first_object; object_number < first_object + 7;
         ++object_number) {
        object = &gPlayerInfo[object_number];
        ReleaseEntityViewportSprites(object, group);
        object->draw_flags = 0;
    }
    gPlayerInfo[player_number].draw_flags &= ~8;
}

/* Refresh six present sprites and their visible names or unidentified glyphs. */
void LoadPresentsIntoInventory(player_number)
short player_number;
{
    short row, column, base_row, character;
    char present_number, present_sprite, *name;
    struct object_state *object;

    if (player_number == 0) {
        base_row = 5;
        object = gPlayer1InventoryItemEntities;
    } else {
        base_row = 18;
        object = gPlayer2InventoryItemEntities;
    }
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 2; ++column, ++object) {
            present_number = P1_MENU_SCROLL[player_number] * 2 + row * 2 + column;
            present_number = gPlayerInventory[player_number][present_number];
            if (present_number != -1) {
                present_sprite = gPresentInfo[present_number].sprite;
                object->frames = &PresentSpriteTable[present_sprite];
                object->sprite_ready = 0;
                object->y = (row * 2 + base_row + 1) * 8;
                object->x = column * 160 + 21;
                object->flags_4d &= ~64;
                object->frame = 0;
            }
            if (!gInventoryLabelsSuppressed) {
                VDP_COMMAND = (((row * 2 + base_row) << 23) + 0x50000003L) +
                              ((column * 20 + 5) << 17);
                if (present_number != -1) {
                    if (gPresentInfo[present_number].identified) {
                        if (present_number == 12 && gPlayerLives[player_number ^ 1] < 0)
                            name = InventoryCharsUnFall;
                        else
                            name = InventoryCharsTable[present_number];
                        for (character = 0; character < 13; ++character)
                            VDP_DATA = 0x85b4 + *name++;
                    } else {
                        for (character = 0; character < 5; ++character)
                            VDP_DATA = 0x85b4 + (char)((PREGEN_RANDOMS[
                                gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 3) + 27);
                        for (; character < 13; ++character)
                            VDP_DATA = 0x85b4;
                    }
                } else {
                    object->flags_4d |= 64;
                    TextWriteStringTiles("             ");
                }
            }
        }
    }
}

/* Refresh three mailbox offers with present sprites, labels and prices. */
void InventoryDrawMailOrderOffers(player_number)
short player_number;
{
    short row, base_row;
    char present_number, present_sprite;
    struct object_state *object;

    if (player_number == 0) {
        base_row = 5;
        object = gPlayer1InventoryItemEntities;
    } else {
        base_row = 18;
        object = gPlayer2InventoryItemEntities;
    }
    for (row = 0; row < 3; ++row, ++object) {
        present_number = gPlayer1MailboxItems[player_number][row];
        present_sprite = gPresentInfo[present_number].sprite;
        object->frames = &PresentSpriteTable[present_sprite];
        object->sprite_ready = 0;
        object->y = (row * 2 + base_row + 1) * 8;
        object->x = 21;
        object->flags_4d &= ~64;
        object->frame = 0;
        if (!gInventoryLabelsSuppressed) {
            VDP_COMMAND = ((row * 2 + base_row) << 23) + 0x500a0003L;
            if (gPresentInfo[present_number].identified) {
                if (present_number == 12 && gPlayerLives[player_number ^ 1] < 0)
                    TextWriteStringTiles("un-fall .....");
                else
                    TextWriteStringTiles(InventoryNameStringTable[present_number]);
            } else
                TextWriteStringTiles("????? .......");
            TextWriteStringTiles("...............  ");
            TextWriteDecimalTiles(gPlayer1MailboxPrices[player_number][row]);
        }
    }
}

/* Draw one of four compact inventory banners and remember its selection. */
void InventoryDrawActionBanner(player_number, menu, already_blanked)
short player_number, menu, already_blanked;
{
    short row, column, base_row, character;
    unsigned short tile;

    base_row = player_number == 0 ? 1 : 14;
    if (!already_blanked) {
        WaitForVBlankWithGraphicsPaused();
        ClearSkipVHBlankFlags();
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = ((base_row + row) << 23) + 0x501c0003L;
            for (character = 0; character < 13; ++character)
                VDP_DATA = 0x824e;
        }
    }
    if (menu == 0) {
        column = 14;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + 0x50000003L) + (column << 17);
            for (character = 0; character < 13; ++character) {
                tile = InventoryCompactMapA[row][character];
                if (tile & 64)
                    tile |= 0x800;
                tile &= ~0xc0;
                VDP_DATA = 0x8233 + tile;
            }
        }
    } else if (menu == 1) {
        column = 14;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + 0x50000003L) + (column << 17);
            for (character = 0; character < 13; ++character) {
                tile = InventoryCompactMapB[row][character];
                if (tile & 64)
                    tile |= 0x800;
                tile &= ~0xc0;
                VDP_DATA = 0xe24e + tile;
            }
        }
    } else if (menu == 2) {
        column = 15;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + 0x50000003L) + (column << 17);
            for (character = 0; character < 11; ++character) {
                tile = InventoryCompactMapC[row][character];
                if (tile & 64)
                    tile |= 0x800;
                tile &= ~0xc0;
                VDP_DATA = 0x8267 + tile;
            }
        }
    } else {
        column = 9;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + 0x50000003L) + (column << 17);
            for (character = 0; character < 22; ++character) {
                tile = InventoryCompactMapD[row][character];
                if (tile & 64)
                    tile |= 0x800;
                tile &= ~0xc0;
                VDP_DATA = 0xc1cd + tile;
            }
        }
    }
    if (!already_blanked)
        SetSkipVHBlankFlags();
    gInventoryBannerMode[player_number] = menu;
}
