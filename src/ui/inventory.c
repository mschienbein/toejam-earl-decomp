#include "present_types.h"
#include "dialogue_ids.h"
#include "player_animation.h"
#include "psg_effects.h"
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
    /* MATCHING QUALIFICATION: materialize zero in the named short black
     * to retain MOVE.W rather than CLR.W at the VDP data port. Literal-zero
     * alternatives differ; a hardware rationale is unproved [INFERENCE]. */

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
    long *destination = (long *)gTextFontBuffers;
    short bank, word;

    for (bank = 0; bank < 2; bank++) {
        for (word = 0; word < 472; word++)
            *destination++ = *source++;
        source = ImageTextChars2;
    }
}

/* Enable alternate font-bank animation for inventory text. */
void EnableTextFontAnimation()
{
    gTextFontAnimationEnabled = 1;
}

/* Keep text animation enabled while either menu uses it. */
void DisableTextFontAnimationIfMenusClosed()
{
    if (!gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayer2InventoryOpen)
        gTextFontAnimationEnabled = 0;
}

/* Alternate the two RAM font banks on every rate-th animation tick. */
int TextAnimateFontBanks(rate)
short rate;
{
    if (!gTextFontAnimationEnabled) {
        if (!gTextFontBank)
            return 0;
        gTextFontAnimationTick = 0;
    }
    if (!(gTextFontAnimationTick++ % rate)) {
        gTextFontBank ^= 1;
        DmaCopyWordsToVram(gTextFontBuffers[gTextFontBank], 0xb680, 944);
        return 1;
    }
    return 0;
}

/* Bracket a single vertical blank with the graphics update flag. */
void WaitForVBlankWithGraphicsPaused()
{
    gVBlankGraphicsPaused = 1;
    WaitForNextVBlank(1);
    gVBlankGraphicsPaused = 0;
}

/* Hide active sprite links belonging to the requested viewport. */
void EntityClearViewportSprites(viewport)
long viewport;
{
    short object_number;
    char viewport_mask;
    struct object_state *object;

    while (gSpriteGraphicsUploadPending)
        ;
    if (viewport == VIEWPORT_PRIMARY)
        viewport_mask = ENTITY_PRIMARY_VIEW;
    else
        viewport_mask = ENTITY_SECONDARY_VIEW;
    object = gPlayerInfo;
    for (object_number = 0; object_number < GAMEPLAY_ENTITY_SLOT_COUNT; object_number++, object++) {
        if ((object->draw_flags & ENTITY_ACTIVE) && (object->draw_flags & viewport_mask))
            ReleaseEntityViewportSprites(object, viewport);
    }
    PositionEntitySprites(gEntityDrawListHead);
    BuildViewportSpriteLinks();
    gSpriteGraphicsUploadPending = 1;
    WaitForNextVBlank(1);
}

/* Advance the four-part background upload and then draw menu contents. */
void InventoryServiceRedrawPhase()
{
    short column, row, row_end;
    unsigned short tile;

    if (gInventoryRedrawPhase >= INVENTORY_REDRAW_BACKGROUND_LOWER) {
        if (!gInventoryRedrawPlayer) {
            if (gInventoryRedrawPhase == INVENTORY_REDRAW_BACKGROUND_UPPER) {
                row = 0;
                row_end = 7;
            } else {
                row = 7;
                row_end = 14;
            }
        } else {
            if (gInventoryRedrawPhase == INVENTORY_REDRAW_BACKGROUND_UPPER) {
                row = 14;
                row_end = 21;
            } else {
                row = 21;
                row_end = 28;
            }
        }
        for (; row < row_end; row++) {
            VDP_COMMAND = VDP_WRITE_WINDOW + ((long)row << 23);
            for (column = 0; column < 40; column++) {
                tile = FontBackgroundByteMap[row][column];
                if (tile & BYTEMAP_HORIZONTAL_FLIP)
                    tile |= VDP_TILE_HORIZONTAL_FLIP;
                if (tile & BYTEMAP_VERTICAL_FLIP)
                    tile |= VDP_TILE_VERTICAL_FLIP;
                tile &= BYTEMAP_FLIP_CLEAR_MASK;
                VDP_DATA = tile + 0x8580;
            }
        }
    } else if (gInventoryRedrawPhase == INVENTORY_REDRAW_STATS) {
        ForceTextBackgroundColorToBlack(1);
        InventoryDrawPlayerStats(gInventoryRedrawPlayer);
        InventoryDrawActionBanner(gInventoryRedrawPlayer, gInventoryBannerMode[gInventoryRedrawPlayer], 1);
        DrawHUD();
    } else if (gPlayerInventoryOpen[gInventoryRedrawPlayer] == INVENTORY_MAIL_ORDER)
        InventoryDrawMailOrderOffers(gInventoryRedrawPlayer);
    else {
        LoadPresentsIntoInventory(gInventoryRedrawPlayer);
        if (gPlayerInventoryOpen[gInventoryRedrawPlayer] == INVENTORY_IDENTIFY)
            InventoryDrawBucks(gInventoryRedrawPlayer);
    }
    gInventoryRedrawPhase--;
}

/* Pick three distinct non-excluded mailbox gifts and prices with a level seed. */
void SetMailOrderPresents(player_number, level)
short player_number, level;
{
    short present, slot;
    long saved_random = StackBasedRandom(gLevelSeeds[level]);

    for (slot = 0; slot < 3; slot++) {
        present = SelectRandomPresent();
        if (present == PRESENT_EXTRA_LIFE || present == PRESENT_PROMOTION || present == PRESENT_JACKPOT ||
            (slot > 0 && gPlayerMailboxItems[player_number][0] == present) ||
            (slot > 1 && gPlayerMailboxItems[player_number][1] == present)) {
            slot--;
            continue;
        }
        gPlayerMailboxItems[player_number][slot] = present;
        gPlayerMailboxPrices[player_number][slot] = GetRandomNumber() % 3 + 3;
    }
    StackBasedRandom(saved_random);
}

/* Build one inventory view and wait for its blanking-driven draw phases. */
void DrawInventory(player_number, menu_kind)
short player_number, menu_kind;
{
    short viewport;
    struct object_state *player;

    if (player_number == PLAYER_TOEJAM)
        viewport = VIEWPORT_PRIMARY;
    else
        viewport = VIEWPORT_SECONDARY;
    player = &gPlayerInfo[player_number];
    if (gSplitScreenEnabled) {
        EntityClearViewportSprites(viewport);
        player->behavior_flags |= ENTITY_PLAYER_SUPPRESSED;
        player->frame_duration = 0;
    }
    if (menu_kind == INVENTORY_PRESENTS)
        gInventoryBannerMode[player_number] = INVENTORY_BANNER_OPEN;
    else if (menu_kind == INVENTORY_MAIL_ORDER) {
        gInventoryBannerMode[player_number] = INVENTORY_BANNER_MAIL_ORDER;
        SetMailOrderPresents(player_number, player->level);
    } else
        gInventoryBannerMode[player_number] = INVENTORY_BANNER_IDENTIFY;
#if REVISION >= 2
    gPlayerIdleTicks[player_number] = 0;
#endif
    InventoryInitializeDisplayEntities(player_number);
    gInventoryRedrawPlayer = player_number;
    gInventoryRedrawPhase = INVENTORY_REDRAW_BACKGROUND_UPPER;
    while (gInventoryRedrawPhase) {
        if (!gPlayerInventoryOpen[player_number] && gInventoryRedrawPhase <= INVENTORY_REDRAW_STATS)
            gPlayerInventoryOpen[player_number] = menu_kind;
    }
    if (!gSplitScreenEnabled) {
        gCamera2PendingX = 0;
        gCamera2PendingY = 112;
    }
    EnableTextFontAnimation();
}

/* Save and replace the open inventory cameras, or restore their positions. */
void InventorySetCameraSuspension(suspend)
int suspend;
{
    if (!gSplitScreenEnabled)
        return;
    if (gPlayerInventoryOpen[PLAYER_TOEJAM]) {
        if (suspend) {
            gInventorySavedCamera1X = gCamera1PendingX;
            gInventorySavedCamera1Y = gCamera1PendingY;
            gCamera1PendingX = 0;
            gCamera1PendingY = 0;
        } else {
            gCamera1PendingX = gInventorySavedCamera1X;
            gCamera1PendingY = gInventorySavedCamera1Y;
        }
    }
    if (gPlayer2InventoryOpen) {
        if (suspend) {
            gInventorySavedCamera2X = gCamera2PendingX;
            gInventorySavedCamera2Y = gCamera2PendingY;
            gCamera2PendingX = 0;
            gCamera2PendingY = 112;
        } else {
            gCamera2PendingX = gInventorySavedCamera2X;
            gCamera2PendingY = gInventorySavedCamera2Y;
        }
    }
}

/* Request redraw mode four and service it until the VBlank flag clears. */
void InventoryRedrawSynchronously(player_number)
int player_number;
{
    gInventoryRedrawPlayer = player_number;
    gInventoryRedrawPhase = INVENTORY_REDRAW_BACKGROUND_UPPER;
    while (gInventoryRedrawPhase)
        InventoryServiceRedrawPhase();
}

/* Wait for the outstanding redraw, request mode one, and wait again. */
void InventoryRefreshContentsAndWait(player_number)
short player_number;
{
    while (gInventoryRedrawPhase)
        ;
    gInventoryRedrawPlayer = player_number;
    gInventoryRedrawPhase = INVENTORY_REDRAW_CONTENTS;
    while (gInventoryRedrawPhase)
        ;
}

/* Close an existing inventory, restoring its camera/display state, or open it. */
void ToggleInventory(player_number)
short player_number;
{
    struct object_state *object;

    if (gPlayerInventoryOpen[player_number]) {
        InventoryDeactivateDisplayEntities(player_number);
        if (gSplitScreenEnabled) {
            gPlayerInventoryOpen[player_number] = INVENTORY_CLOSED;
            if (!gPlayerInventoryOpen[player_number ^ 1] && !PlayersRequireSeparateViews())
                UpdateScreenSplitMode(1);
            else
                gHudRedrawPending = 1;
            object = &gPlayerInfo[player_number];
            object->behavior_flags &= ~ENTITY_PLAYER_SUPPRESSED;
            if (object->frame_phase)
                object->frame_duration = 1;
            UpdateEntityViewportSprites(player_number);
        } else {
            if (!gPlayerInventoryOpen[player_number ^ 1]) {
                gCamera2PendingX = gCamera1PendingX;
                gCamera2PendingY = gCamera1PendingY;
            }
            gPlayerInventoryOpen[player_number] = INVENTORY_CLOSED;
            WaitForVBlankWithGraphicsPaused();
            DisableBlankingGraphics();
            DrawHUD();
            if (!gPlayerInventoryOpen[player_number ^ 1])
                ForceTextBackgroundColorToBlack(0);
            EnableBlankingGraphics();
        }
        DisableTextFontAnimationIfMenusClosed();
    } else if (!gPlayerElevatorFlags[player_number] || !gElevatorDepartureInProgress)
        DrawInventory(player_number, INVENTORY_PRESENTS);
}

/* Apply a just-opened present to the other living player when appropriate. */
void CheckIfOpenForBoth(opening)
short *opening;
{
    short present, player_number;

    if (!opening[PLAYER_TOEJAM] && !opening[PLAYER_EARL])
        return;
    if (opening[PLAYER_TOEJAM])
        player_number = PLAYER_TOEJAM;
    else
        player_number = PLAYER_EARL;
    present = OpenPresent(player_number);
    if (present && gPlayerLives[player_number ^ 1] >= 0) {
        if (!PlayersRequireSeparateViews()) {
            --present;
            if (gPlayerInventoryOpen[player_number ^ 1])
                ToggleInventory(player_number ^ 1);
            if (present != PRESENT_DOORWAY && present != PRESENT_DECOY && present != PRESENT_TOGETHERNESS &&
                present != PRESENT_TELEPHONE && present != PRESENT_EARTHLING && present != PRESENT_BOOMBOX) {
                if (!OpenSelectedPresent(player_number ^ 1, present) &&
                    present == PRESENT_TOTAL_BUMMER)
                    GenerateSpeech(player_number ^ 1, "Thanks a lot", 1);
            }
        } else if (gPlayerInventoryOpen[player_number ^ 1])
            InventoryRefreshContentsAndWait(player_number ^ 1);
    }
}

/* Replace occupied slots except present 27 for each player. */
void RandomizeInventory()
{
    short slot, player_number;

    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number)
        for (slot = 0; slot < 16; ++slot)
            if (gPlayerInventory[player_number][slot] != PRESENT_NONE &&
                gPlayerInventory[player_number][slot] != PRESENT_BONUS_HITOPS)
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
        if (!gPlayerInventoryOpen[player_number ^ 1]) {
            DrawInventory(player_number ^ 1, INVENTORY_PRESENTS);
            InventoryUpdateSelectorEntity(player_number ^ 1);
            close_other = 1;
        } else
            InventoryRefreshContentsAndWait(player_number ^ 1);
    }
    gPlayer1InventorySelectorFlags |= ENTITY_HIDDEN;
    gPlayer2InventorySelectorFlags |= ENTITY_HIDDEN;
    gInventoryLabelsSuppressed = 1;
    PlaySoundEffect((long)Sfx_Randomize, 96, 1);
    for (frame = 0; frame < 60; ++frame) {
        RandomizeInventory();
        InventoryRefreshContentsAndWait(player_number);
        UpdateAndQueueEntitySprites();
        InventoryRefreshContentsAndWait(player_number ^ 1);
        UpdateAndQueueEntitySprites();
        WaitForNextVBlank(frame / 5);
        PlaySound(PSG_TONE_PULSE, AUDIO_PRIORITY_NORMAL);
    }
    gInventoryLabelsSuppressed = 0;
    PresentClearWorldEntityReadiness();
    if (close_other)
        ToggleInventory(player_number ^ 1);
}

/* Process button edges and inventory cursor movement for the two players. */
void ProcessInventory()
{
    short opening[PLAYER_COUNT];
    short player_number;
    struct object_state *object;

    opening[PLAYER_TOEJAM] = 0;
    opening[PLAYER_EARL] = 0;
    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number) {
        object = &gPlayerInfo[player_number];
        if (gPlayerLives[player_number] < 0 || object->sprite_state == PLAYER_ANIM_GHOST ||
            object->sprite_state == PLAYER_ANIM_LAST_LIFE_GHOST)
            continue;
        if (!(gPlayerPreviousProcessedInput[player_number] & INPUT_BUTTON_B) &&
            (gPlayerProcessedInput[player_number] & INPUT_BUTTON_B)) {
            ToggleInventory(player_number);
            return;
        }
        if (gPlayerInventoryOpen[player_number]) {
            if (gPlayerInventoryOpen[player_number] == INVENTORY_PRESENTS &&
                !(gPlayerPreviousProcessedInput[player_number] & INPUT_BUTTON_C) &&
                (gPlayerProcessedInput[player_number] & INPUT_BUTTON_C)) {
                gInventoryBannerMode[player_number] ^= INVENTORY_BANNER_DROP;
                InventoryDrawActionBanner(player_number, gInventoryBannerMode[player_number], 0);
                PlaySound(PSG_TONE_PULSE, AUDIO_PRIORITY_NORMAL);
            } else if (!(gPlayerPreviousProcessedInput[player_number] & INPUT_BUTTON_A) &&
                       (gPlayerProcessedInput[player_number] & INPUT_BUTTON_A)) {
                if (gPlayerInventoryOpen[player_number] == INVENTORY_MAIL_ORDER) {
                    ToggleInventory(player_number);
                    InventoryBuySelectedMailOrderPresent(player_number);
                } else if (gPlayerInventoryOpen[player_number] == INVENTORY_IDENTIFY) {
                    if (InventoryIdentifySelectedPresent(player_number)) {
                        InventoryRefreshContentsAndWait(player_number);
                        if (gPlayerInventoryOpen[player_number ^ 1])
                            InventoryRefreshContentsAndWait(player_number ^ 1);
                    }
                } else if (gInventoryBannerMode[player_number] == INVENTORY_BANNER_DROP) {
                    InventoryDropSelectedPresent(player_number);
                    InventoryRefreshContentsAndWait(player_number);
                } else {
                    gPlayerPreviousProcessedInput[player_number] |= INPUT_BUTTON_A;
                    if (InventoryGetSelectedPresent(player_number) == PRESENT_RANDOMIZER)
                        RandomizePresents(player_number);
                    else {
                        ToggleInventory(player_number);
                        opening[player_number] = 1;
                    }
                }
            } else if (!(gPlayerPreviousProcessedInput[player_number] & INPUT_DIRECTION_MASK) &&
                       (gPlayerProcessedInput[player_number] & INPUT_DIRECTION_MASK)) {
                if (gPlayerProcessedInput[player_number] & INPUT_UP) {
                    if (gPlayerInventorySelectionRow[player_number] > 0)
                        --gPlayerInventorySelectionRow[player_number];
                    else if (gPlayerInventoryOpen[player_number] != INVENTORY_MAIL_ORDER &&
                             gPlayerInventoryScrollRow[player_number] > 0) {
                        --gPlayerInventoryScrollRow[player_number];
                        InventoryRefreshContentsAndWait(player_number);
                    }
                } else if (gPlayerProcessedInput[player_number] & INPUT_DOWN) {
                    if (gPlayerInventorySelectionRow[player_number] < 2)
                        ++gPlayerInventorySelectionRow[player_number];
                    else if (gPlayerInventoryOpen[player_number] != INVENTORY_MAIL_ORDER &&
                             gPlayerInventoryScrollRow[player_number] < 5 &&
                             gPlayerInventory[player_number][(gPlayerInventoryScrollRow[player_number] + 3) * 2] != PRESENT_NONE) {
                        ++gPlayerInventoryScrollRow[player_number];
                        InventoryRefreshContentsAndWait(player_number);
                    }
                } else if (gPlayerInventoryOpen[player_number] != INVENTORY_MAIL_ORDER)
                    gPlayerInventorySelectionColumn[player_number] ^= 1;
                PlaySound(PSG_TONE_PULSE, AUDIO_PRIORITY_NORMAL);
            }
            if (gPlayerInventoryOpen[player_number])
                InventoryUpdateSelectorEntity(player_number);
            if (opening[player_number])
                break;
        }
    }
    CheckIfOpenForBoth(opening);
    if (gPendingUnfallPlayer) {
        player_number = gPendingUnfallPlayer - 1;
        if (gPlayerInventoryOpen[player_number ^ 1])
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
    if ((gPlayerInventoryOpen[PLAYER_TOEJAM] || gPlayerInventoryOpen[PLAYER_EARL]) && !gSplitScreenEnabled) {
        while (gPlayerInventoryOpen[PLAYER_TOEJAM] || gPlayerInventoryOpen[PLAYER_EARL]) {
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
    DrawInventory(player_number, INVENTORY_MAIL_ORDER);
    gPlayerProcessedInput[player_number] = INPUT_BUTTON_A;
    gPlayerPreviousProcessedInput[player_number] = INPUT_BUTTON_A;
    CheckBButton();
}

/* Draw the inventory, set both processed-input bytes to the present-action
 * bit (0x40), then run CheckBButton. */
void InventoryOpenIdentificationMenu(player)
short player;
{
    DrawInventory(player, INVENTORY_IDENTIFY);
    gPlayerProcessedInput[player] = INPUT_BUTTON_A;
    gPlayerPreviousProcessedInput[player] = INPUT_BUTTON_A;
    CheckBButton();
}

/* Return the present byte at the current scrolled inventory selection. */
char InventoryGetSelectedPresent(player)
int player;
{
    char slot;

    slot = (gPlayerInventoryScrollRow[player] + gPlayerInventorySelectionRow[player]) * 2 +
           gPlayerInventorySelectionColumn[player];
    return gPlayerInventory[player][slot];
}

/* Resolve random presents, open the selected one, and close its inventory gap. */
short OpenPresent(player)
short player;
{
    char slot, present;

    slot = (gPlayerInventoryScrollRow[player] + gPlayerInventorySelectionRow[player]) * 2 +
           gPlayerInventorySelectionColumn[player];
    present = gPlayerInventory[player][slot];
    while (present == PRESENT_MYSTERY)
        present = SelectRandomPresent();
    if (present == PRESENT_NONE || OpenSelectedPresent(player, present)) {
        PlaySound(PSG_PERIODIC_TONE2_NOISE, AUDIO_PRIORITY_NORMAL);
        return 0;
    }
    if (gPlayerInventory[player][slot] != PRESENT_MYSTERY)
        gPresentInfo[present].identified = 1;
    while (slot < 15 && gPlayerInventory[player][slot + 1] != PRESENT_NONE) {
        gPlayerInventory[player][slot] = gPlayerInventory[player][slot + 1];
        ++slot;
    }
    gPlayerInventory[player][slot] = PRESENT_NONE;
    return present + 1;
}

/* Reject dropping in the elevator; otherwise remove and drop the selection. */
void InventoryDropSelectedPresent(player)
short player;
{
    char slot, present;

    if (gPlayerElevatorFlags[player]) {
        PlaySound(PSG_PERIODIC_TONE2_NOISE, AUDIO_PRIORITY_NORMAL);
        return;
    }
    slot = (gPlayerInventoryScrollRow[player] + gPlayerInventorySelectionRow[player]) * 2 +
           gPlayerInventorySelectionColumn[player];
    present = gPlayerInventory[player][slot];
    while (slot < 15 && gPlayerInventory[player][slot + 1] != PRESENT_NONE) {
        gPlayerInventory[player][slot] = gPlayerInventory[player][slot + 1];
        ++slot;
    }
    gPlayerInventory[player][slot] = PRESENT_NONE;
    if (present != PRESENT_NONE)
        DropPresent(player, present);
}

/* Buy the selected mailbox item when both inventory space and bucks allow it. */
void InventoryBuySelectedMailOrderPresent(player)
short player;
{
    char slot, selection;

    for (slot = 0; slot < 16 && gPlayerInventory[player][slot] != PRESENT_NONE; ++slot)
        ;
    selection = gPlayerInventorySelectionRow[player];
    if (slot < 16 && gPlayerBucks[player] >= gPlayerMailboxPrices[player][selection]) {
        gPlayerBucks[player] -= gPlayerMailboxPrices[player][selection];
        gPlayerInventory[player][slot] = gPlayerMailboxItems[player][selection];
        GenerateSpeech(player, "Got it!", 1);
    } else {
        PlaySound(PSG_PERIODIC_TONE2_NOISE, AUDIO_PRIORITY_NORMAL);
        if (slot == 16)
            StartDialogue(DIALOGUE_INVENTORY_FULL, player);
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

    slot = (gPlayerInventoryScrollRow[player] + gPlayerInventorySelectionRow[player]) * 2 +
           gPlayerInventorySelectionColumn[player];
    present = gPlayerInventory[player][slot];
    if (gPlayerBucks[player] < 2 || present == PRESENT_NONE) {
        PlaySound(PSG_PERIODIC_TONE2_NOISE, AUDIO_PRIORITY_NORMAL);
        return 0;
    }
    while (present == PRESENT_MYSTERY) {
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
        PlaySound(PSG_PERIODIC_TONE2_NOISE, AUDIO_PRIORITY_NORMAL);
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
        character = (gCachedRandomValues[gRandomCacheIndex == 58 ?
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
    if (player_number == PLAYER_TOEJAM) {
        VDP_COMMAND = 0x56000003L + (column << 17);
        if (gPlayerBucks[PLAYER_TOEJAM] < 100)
            VDP_DATA = 0x85b4;
        TextWriteDecimalTiles(gPlayerBucks[PLAYER_TOEJAM]);
        if (gPlayerBucks[PLAYER_TOEJAM] < 10)
            VDP_DATA = 0x85b4;
    } else {
        VDP_COMMAND = 0x5c800003L + (column << 17);
        if (gPlayerBucks[PLAYER_EARL] < 100)
            VDP_DATA = 0x85b4;
        TextWriteDecimalTiles(gPlayerBucks[PLAYER_EARL]);
        if (gPlayerBucks[PLAYER_EARL] < 10)
            VDP_DATA = 0x85b4;
    }
}

/* Render currency, points and the player's rank glyph sequence. */
void InventoryDrawPlayerStats(player_number)
short player_number;
{
    short column = 32;
    InventoryDrawBucks(player_number);
    if (player_number == PLAYER_TOEJAM) {
        if (gPlayerPoints[PLAYER_TOEJAM] > 99)
            --column;
        VDP_COMMAND = 0x56000003L + (column << 17);
        TextWriteDecimalTiles(gPlayerPoints[PLAYER_TOEJAM]);
        if (gPlayerPoints[PLAYER_TOEJAM] > 999)
            TextWriteStringTiles(" pts");
        VDP_COMMAND = 0x55a80003L;
        TextWriteStringTiles(RankCharsTable[gPlayerRank[PLAYER_TOEJAM]]);
    } else {
        if (gPlayerPoints[PLAYER_EARL] > 99)
            --column;
        VDP_COMMAND = 0x5c800003L + (column << 17);
        TextWriteDecimalTiles(gPlayerPoints[PLAYER_EARL]);
        if (gPlayerPoints[PLAYER_EARL] > 999)
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

    if (gPlayerInventoryOpen[player_number] != INVENTORY_MAIL_ORDER) {
        while (gPlayerInventory[player_number][(gPlayerInventoryScrollRow[player_number] + gPlayerInventorySelectionRow[player_number]) * 2 +
         gPlayerInventorySelectionColumn[player_number]] == PRESENT_NONE) {
            if (gPlayerInventorySelectionRow[player_number] == 0 &&
                gPlayerInventorySelectionColumn[player_number] == 0) {
                exhausted = 1;
                break;
            }
            if (gPlayerInventorySelectionColumn[player_number] > 0)
                --gPlayerInventorySelectionColumn[player_number];
            else {
                --gPlayerInventorySelectionRow[player_number];
                ++gPlayerInventorySelectionColumn[player_number];
            }
        }
    }
    if (player_number == PLAYER_TOEJAM)
        selector = &gPlayer1InventorySelectorEntity;
    else
        selector = &gPlayer2InventorySelectorEntity;
    if (!exhausted) {
        selector->x = gPlayerInventorySelectionColumn[player_number] * 160 + 7;
        selector->y = gPlayerInventorySelectionRow[player_number] * 16 + 63;
        selector->behavior_flags &= ~ENTITY_HIDDEN;
        if (player_number == PLAYER_EARL)
            selector->y += 104;
    } else
        selector->behavior_flags |= ENTITY_HIDDEN;
}

/* Activate seven inventory objects, waiting for their selected draw flag. */
void InventoryInitializeDisplayEntities(player_number)
short player_number;
{
    short group, first_object, object_number;
    char ready_mask;
    struct object_state *object;

    gPlayerInventorySelectionRow[player_number] = 0;
    gPlayerInventorySelectionColumn[player_number] = 0;
    gPlayerInventoryScrollRow[player_number] = 0;
    group = VIEWPORT_SECONDARY;
    ready_mask = ENTITY_SECONDARY_VIEW;
    if (player_number == PLAYER_TOEJAM) {
        first_object = INVENTORY_SELECTOR_TOEJAM;
        if (gSplitScreenEnabled) {
            group = VIEWPORT_PRIMARY;
            ready_mask = ENTITY_PRIMARY_VIEW;
        }
    } else
        first_object = INVENTORY_SELECTOR_EARL;
    object = &gPlayerInfo[first_object];
    for (object_number = first_object; object_number < first_object + 7;
         ++object_number, ++object) {
        object->draw_flags = ENTITY_ACTIVE;
        for (AllocateEntityViewportSprites(object, group); !(object->draw_flags & ready_mask);
             AllocateEntityViewportSprites(object, group))
            EntityReleaseOneByPriority();
        object->sprite_ready = 0;
        object->behavior_flags |= ENTITY_HIDDEN;
    }
    gPlayerInfo[player_number].draw_flags |= ENTITY_SKIP_PHYSICS;
}

/* Remove each inventory object from its draw group and clear player flag 8. */
void InventoryDeactivateDisplayEntities(player_number)
short player_number;
{
    short group, first_object, object_number;
    struct object_state *object;

    group = VIEWPORT_SECONDARY;
    if (player_number == PLAYER_TOEJAM) {
        first_object = INVENTORY_SELECTOR_TOEJAM;
        if (gSplitScreenEnabled)
            group = VIEWPORT_PRIMARY;
    } else
        first_object = INVENTORY_SELECTOR_EARL;
    for (object_number = first_object; object_number < first_object + 7;
         ++object_number) {
        object = &gPlayerInfo[object_number];
        ReleaseEntityViewportSprites(object, group);
        object->draw_flags = 0;
    }
    gPlayerInfo[player_number].draw_flags &= ~ENTITY_SKIP_PHYSICS;
}

/* Refresh six present sprites and their visible names or unidentified glyphs. */
void LoadPresentsIntoInventory(player_number)
short player_number;
{
    short row, column, base_row, character;
    char present_number, present_sprite, *name;
    struct object_state *object;

    if (player_number == PLAYER_TOEJAM) {
        base_row = 5;
        object = gPlayer1InventoryItemEntities;
    } else {
        base_row = 18;
        object = gPlayer2InventoryItemEntities;
    }
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 2; ++column, ++object) {
            present_number = gPlayerInventoryScrollRow[player_number] * 2 + row * 2 + column;
            present_number = gPlayerInventory[player_number][present_number];
            if (present_number != PRESENT_NONE) {
                present_sprite = gPresentInfo[present_number].appearance;
                object->frames = &PresentSpriteTable[present_sprite];
                object->sprite_ready = 0;
                object->y = (row * 2 + base_row + 1) * 8;
                object->x = column * 160 + 21;
                object->behavior_flags &= ~ENTITY_HIDDEN;
                object->frame = 0;
            }
            if (!gInventoryLabelsSuppressed) {
                VDP_COMMAND = (((row * 2 + base_row) << 23) + VDP_WRITE_WINDOW) +
                              ((column * 20 + 5) << 17);
                if (present_number != PRESENT_NONE) {
                    if (gPresentInfo[present_number].identified) {
                        if (present_number == PRESENT_TOGETHERNESS && gPlayerLives[player_number ^ 1] < 0)
                            name = InventoryCharsUnFall;
                        else
                            name = InventoryCharsTable[present_number];
                        for (character = 0; character < 13; ++character)
                            VDP_DATA = 0x85b4 + *name++;
                    } else {
                        for (character = 0; character < 5; ++character)
                            VDP_DATA = 0x85b4 + (char)((gCachedRandomValues[
                                gRandomCacheIndex == 58 ? (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 3) + 27);
                        for (; character < 13; ++character)
                            VDP_DATA = 0x85b4;
                    }
                } else {
                    object->behavior_flags |= ENTITY_HIDDEN;
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

    if (player_number == PLAYER_TOEJAM) {
        base_row = 5;
        object = gPlayer1InventoryItemEntities;
    } else {
        base_row = 18;
        object = gPlayer2InventoryItemEntities;
    }
    for (row = 0; row < 3; ++row, ++object) {
        present_number = gPlayerMailboxItems[player_number][row];
        present_sprite = gPresentInfo[present_number].appearance;
        object->frames = &PresentSpriteTable[present_sprite];
        object->sprite_ready = 0;
        object->y = (row * 2 + base_row + 1) * 8;
        object->x = 21;
        object->behavior_flags &= ~ENTITY_HIDDEN;
        object->frame = 0;
        if (!gInventoryLabelsSuppressed) {
            VDP_COMMAND = ((row * 2 + base_row) << 23) + 0x500a0003L;
            if (gPresentInfo[present_number].identified) {
                if (present_number == PRESENT_TOGETHERNESS && gPlayerLives[player_number ^ 1] < 0)
                    TextWriteStringTiles("un-fall .....");
                else
                    TextWriteStringTiles(InventoryNameStringTable[present_number]);
            } else
                TextWriteStringTiles("????? .......");
            TextWriteStringTiles("...............  ");
            TextWriteDecimalTiles(gPlayerMailboxPrices[player_number][row]);
        }
    }
}

/* Draw one of four compact inventory banners and remember its selection. */
void InventoryDrawActionBanner(player_number, menu, already_blanked)
short player_number, menu, already_blanked;
{
    short row, column, base_row, character;
    unsigned short tile;

    base_row = player_number == PLAYER_TOEJAM ? 1 : 14;
    if (!already_blanked) {
        WaitForVBlankWithGraphicsPaused();
        DisableBlankingGraphics();
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = ((base_row + row) << 23) + 0x501c0003L;
            for (character = 0; character < 13; ++character)
                VDP_DATA = 0x824e;
        }
    }
    if (menu == INVENTORY_BANNER_OPEN) {
        column = 14;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + VDP_WRITE_WINDOW) + (column << 17);
            for (character = 0; character < 13; ++character) {
                tile = InventoryCompactMapA[row][character];
                if (tile & BYTEMAP_HORIZONTAL_FLIP)
                    tile |= VDP_TILE_HORIZONTAL_FLIP;
                tile &= ~(BYTEMAP_HORIZONTAL_FLIP | BYTEMAP_VERTICAL_FLIP);
                VDP_DATA = 0x8233 + tile;
            }
        }
    } else if (menu == INVENTORY_BANNER_DROP) {
        column = 14;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + VDP_WRITE_WINDOW) + (column << 17);
            for (character = 0; character < 13; ++character) {
                tile = InventoryCompactMapB[row][character];
                if (tile & BYTEMAP_HORIZONTAL_FLIP)
                    tile |= VDP_TILE_HORIZONTAL_FLIP;
                tile &= ~(BYTEMAP_HORIZONTAL_FLIP | BYTEMAP_VERTICAL_FLIP);
                VDP_DATA = 0xe24e + tile;
            }
        }
    } else if (menu == INVENTORY_BANNER_MAIL_ORDER) {
        column = 15;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + VDP_WRITE_WINDOW) + (column << 17);
            for (character = 0; character < 11; ++character) {
                tile = InventoryCompactMapC[row][character];
                if (tile & BYTEMAP_HORIZONTAL_FLIP)
                    tile |= VDP_TILE_HORIZONTAL_FLIP;
                tile &= ~(BYTEMAP_HORIZONTAL_FLIP | BYTEMAP_VERTICAL_FLIP);
                VDP_DATA = 0x8267 + tile;
            }
        }
    } else {
        column = 9;
        for (row = 0; row < 3; ++row) {
            VDP_COMMAND = (((base_row + row) << 23) + VDP_WRITE_WINDOW) + (column << 17);
            for (character = 0; character < 22; ++character) {
                tile = InventoryCompactMapD[row][character];
                if (tile & BYTEMAP_HORIZONTAL_FLIP)
                    tile |= VDP_TILE_HORIZONTAL_FLIP;
                tile &= ~(BYTEMAP_HORIZONTAL_FLIP | BYTEMAP_VERTICAL_FLIP);
                VDP_DATA = 0xc1cd + tile;
            }
        }
    }
    if (!already_blanked)
        EnableBlankingGraphics();
    gInventoryBannerMode[player_number] = menu;
}
