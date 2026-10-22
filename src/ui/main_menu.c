#include "psg_effects.h"
#include "ui.h"
#include "hardware.h"
#include "entities.h"
#include "assets_palettes.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "engine_calls.h"
#include "hardware_calls.h"
#include "intro_calls.h"
#include "jam_calls.h"
#include "menu_calls.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Select players, then run auxiliary menus until a world is selected.
 * ORIGINAL BUG: FurtherInitialisation passes no argument, but this incoming
 * short slot is read at entry before being overwritten by the first menu.
 * A no-argument definition removes the observed four-byte stack load. */
void NewGameMainMenu(selection)
short selection;
{
select_players:
    selection = MenuWhoHandler();
    gControllerPortsSwapped = 0;
    switch (selection) {
    case 0:
        gPlayerLives[PLAYER_TOEJAM] = 0;
        gPlayerRank[PLAYER_TOEJAM] = 0;
        gPlayerInfo[PLAYER_TOEJAM].level = 0;
        gPlayerLives[PLAYER_EARL] = 0;
        gPlayerRank[PLAYER_EARL] = 0;
        gPlayerInfo[PLAYER_EARL].level = 0;
        gPlayerSelection = 2;
        gControllerPortsSwapped = 0;
        break;
    case 1:
        gPlayerLives[PLAYER_TOEJAM] = 0;
        gPlayerRank[PLAYER_TOEJAM] = 0;
        gPlayerInfo[PLAYER_TOEJAM].level = 0;
        gPlayerLives[PLAYER_EARL] = -1;
        gPlayerRank[PLAYER_EARL] = 9;
        gPlayerInfo[PLAYER_EARL].level = 99;
        gPlayerSelection = 0;
        if (GetCtrl2Input()) {
            gControllerPortsSwapped = 1;
            break;
        } else {
            gControllerPortsSwapped = 0;
            break;
        }
    case 2:
        gPlayerLives[PLAYER_TOEJAM] = -1;
        gPlayerRank[PLAYER_TOEJAM] = 9;
        gPlayerInfo[PLAYER_TOEJAM].level = 99;
        gPlayerLives[PLAYER_EARL] = 0;
        gPlayerRank[PLAYER_EARL] = 0;
        gPlayerInfo[PLAYER_EARL].level = 0;
        gPlayerSelection = 1;
        if (GetCtrl1Input()) {
            gControllerPortsSwapped = 1;
            break;
        } else {
            gControllerPortsSwapped = 0;
            break;
        }
    }
select_game:
    switch ((short)MenuWhatHandler()) {
    case 0:
        break;
    case 1:
        StartJamOut(MenuJamOutHandler());
        goto select_players;
    case 2:
        IntroVideo();
        goto select_players;
    case 3:
        gControllerButtonsAcSwapped = MenuControllerOptionsHandler();
        goto select_game;
    case 4:
        gWorldType = 1;
        break;
    default:
        goto select_game;
    }
}

/* Display the game-over text until a button or timeout, then reset. */
void ShowGameOverAndReset()
{
    DrawMenuWindowFrame(0);
    VDP_COMMAND = 0x57160003L;
    TextWriteStringTiles("bogus ... game over");
    ForceTextBackgroundColorToBlack(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    EnableTextFontAnimation();
    gFrameCounter = 0;
    for (;;) {
        do {
            WaitForNextVBlank(1);
            TextAnimateFontBanks(8);
        } while (gFrameCounter <= 600 &&
            !((ReadMappedPlayerInput(0) | ReadMappedPlayerInput(1)) & 0xf0));
        PlaySound(PSG_STOP_BACKGROUND_NOISE, 99);
        PlaySound(PSG_SILENCE, 98);
        AudioRequestFmChannelRelease();
        MusicRequestTrackPointerReset();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
        ClearColourRAM();
        ClearVerticalScrollRam();
        ClearVideoRAM();
        RestartFromResetEntry();
    }
}

/* Show the player-selection menu under its header and return the chosen item. */
int MenuWhoHandler()
{
    int selection;
    AddMenuHeader(0);
    selection = GeneralMenuHandler(2, MenuWhoItems, MenuWhoItemCount);
    RemoveMenuHeader();
    return selection;
}

/* Show the controller-options menu under its header and return the chosen item. */
int MenuControllerOptionsHandler()
{
    int selection;
    AddMenuHeader(3);
    selection = GeneralMenuHandler(2, MenuControllerOptionsItems, MenuControllerOptionsItemCount);
    RemoveMenuHeader();
    return selection;
}

/* Show the game-mode menu under its header and return the chosen item. */
int MenuWhatHandler()
{
    int selection;
    AddMenuHeader(1);
    selection = GeneralMenuHandler(2, MenuWhatItems, MenuWhatItemCount);
    RemoveMenuHeader();
    return selection;
}

/* Show the Jam Out menu under its header and return the chosen item. */
int MenuJamOutHandler()
{
    int selection;
    AddMenuHeader(2);
    selection = GeneralMenuHandler(2, MenuJamOutItems, MenuJamOutItemCount);
    RemoveMenuHeader();
    return selection;
}

/* Check both controller ports with each direction setting. */
int AreBothControllersConnected()
{
    int controllers_present = 1;
    int delay;
    unsigned char first_read;
    char second_read;

    Z80_BUS_REQUEST = 0x100;
    CONTROL_1_DIRECTION = 0x40;
    CONTROL_1_DATA = 0;
    for (delay = 0; delay < 10; delay++)
        ;
    first_read = CONTROL_1_DATA;
    CONTROL_1_DIRECTION = 0;
    CONTROL_1_DATA = 0;
    for (delay = 0; delay < 10; delay++)
        ;
    second_read = CONTROL_1_DATA;
    if (((unsigned char)second_read & first_read & 15) == 15)
        controllers_present = 0;
    if (controllers_present) {
        CONTROL_2_DIRECTION = 0x40;
        CONTROL_2_DATA = 0;
        for (delay = 0; delay < 10; delay++)
            ;
        first_read = CONTROL_2_DATA;
        CONTROL_2_DIRECTION = 0;
        CONTROL_2_DATA = 0;
        for (delay = 0; delay < 10; delay++)
            ;
        second_read = CONTROL_2_DATA;
        if (((unsigned char)second_read & first_read & 15) == 15)
            controllers_present = 0;
    }
    Z80_BUS_REQUEST = 0;
    return controllers_present;
}

/* Draw the menu, reject disconnected two-player selection, and track edges. */
int GeneralMenuHandler(cursor_column, items, item_count)
register short cursor_column, item_count;
register struct menu_item *items;
{
    short selected_item;
    unsigned char previous_input = 0, input;
    struct menu_item *selected;

    WaitForNextVBlank(1);
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    DrawStarfieldPlane();
    DrawMenuWindowFrame(0);
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    ForceTextBackgroundColorToBlack(1);
    MenuDrawTextItems(items, item_count);
    UpdateAndQueueEntitySprites();
    WaitForNextVBlank(1);
    UploadQueuedSpriteGraphics(0, 0);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    EnableTextFontAnimation();
    while ((GetCtrl1Input() | GetCtrl2Input()) & 0xf0) {
        WaitForNextVBlank(1);
        TextAnimateFontBanks(8);
    }
    CreateMenuCursorEntity();
    selected_item = 0;
    selected = &items[0];
    for (;;) {
        gSpriteGraphicsUploadPending = 0;
        UpdateAndQueueEntitySprites();
        WaitForNextVBlank(1);
        UploadQueuedSpriteGraphics(0, 0);
        TextAnimateFontBanks(8);
        SelectViewportSpriteChain(0);
        GetRandomNumber();
        input = (unsigned char)GetCtrl1Input() | GetCtrl2Input();
        if (input != previous_input) {
            if (input & 0xf0) {
                if (items != MenuWhoItems || selected->selection || AreBothControllersConnected())
                    break;
                PlaySound(PSG_PERIODIC_TONE2_NOISE, 50);
                VDP_COMMAND = 0x5c040003L;
                TextWriteStringTiles("need two controllers for two players");
            } else if (input & 1) {
                --selected_item;
                PlaySound(PSG_TONE_PULSE, 50);
                if (selected_item < 0 || items[selected_item].selection < 0)
                    ++selected_item;
            } else if (input & 2) {
                PlaySound(PSG_TONE_PULSE, 50);
                ++selected_item;
                if (selected_item >= item_count || items[selected_item].selection < 0)
                    --selected_item;
            }
            previous_input = input;
        }
        selected = &items[selected_item];
        PositionMenuCursorEntity(cursor_column, selected->y);
    }
    DisableTextFontAnimationIfMenusClosed();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | VDP_MODE2_REGISTER;
    ReleaseMenuCursorEntity();
    ForceTextBackgroundColorToBlack(0);
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    return selected->selection;
}

/* Advance over eight-byte menu records while printing each item's text. */
void MenuDrawTextItems(item, item_count)
struct menu_item *item;
short item_count;
{
    short item_number;
    for (item_number = 0; item_number < item_count; ++item, ++item_number) {
        VDP_COMMAND = VDP_WRITE_WINDOW + (item->y << 23) + (item->x << 17);
        TextWriteStringTiles(item->text);
    }
}

/* Allocate and initialize the animated menu cursor object. */
void CreateMenuCursorEntity()
{
    struct object_state *cursor;
    if (gMenuCursorEntity = (int)AllocateEntity()) {
        cursor = &gPlayerInfo[gMenuCursorEntity];
        cursor->tile_count = 12;
        cursor->sprite_count = 1;
        cursor->x = 20;
        cursor->y = -50;
        cursor->sequence = MenuCursorAnimation;
        cursor->frames = Spr_MenuCursor;
        cursor->frame_phase = 8;
        cursor->frame_duration = 1;
        cursor->frame = -1;
        cursor->sprite_ready = 0;
        AllocateEntityViewportSprites(cursor, 0);
    }
}

/* Release the cursor entity if its stored handle is nonzero. */
void ReleaseMenuCursorEntity()
{
    if (gMenuCursorEntity)
        FreeEntity(gMenuCursorEntity);
}

/* Place the cursor at the selected tile coordinate with a six-pixel offset. */
void PositionMenuCursorEntity(column, row)
short column, row;
{
    struct object_state *cursor;
    if (gMenuCursorEntity) {
        cursor = &gPlayerInfo[gMenuCursorEntity];
        cursor->x = column << 3;
        cursor->y = (row << 3) + 6;
    }
}

/* Allocate a menu title object using the selected two-byte animation. */
void AddMenuHeader(header_number)
short header_number;
{
    struct object_state *header;
    CopyPaletteToCRAMSlot(MenuHeaderPalette, 1);
    if (gMenuHeaderEntity = (int)AllocateEntity()) {
        header = &gPlayerInfo[gMenuHeaderEntity];
        header->tile_count = -128;
        header->sprite_count = 11;
        header->x = 160;
        header->y = 40;
        header->sequence = MenuHeaderAnimations[header_number];
        header->frames = Spr_MenuHeader;
        header->frame_phase = 8;
        header->frame_duration = 1;
        header->frame = -1;
        header->sprite_ready = 0;
        AllocateEntityViewportSprites(header, 0);
    }
}

/* Release the menu-header entity if its stored handle is nonzero. */
void RemoveMenuHeader()
{
    if (gMenuHeaderEntity)
        FreeEntity(gMenuHeaderEntity);
}
