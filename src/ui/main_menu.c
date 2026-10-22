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
    ram_ff801c = 0;
    switch (selection) {
    case 0:
        gPlayerLives[0] = 0;
        gPlayerRank[0] = 0;
        gPlayerInfo[0].level = 0;
        gPlayerLives[1] = 0;
        gPlayerRank[1] = 0;
        gPlayerInfo[1].level = 0;
        PLAYER_INDICATOR = 2;
        ram_ff801c = 0;
        break;
    case 1:
        gPlayerLives[0] = 0;
        gPlayerRank[0] = 0;
        gPlayerInfo[0].level = 0;
        gPlayerLives[1] = -1;
        gPlayerRank[1] = 9;
        gPlayerInfo[1].level = 99;
        PLAYER_INDICATOR = 0;
        if (GetCtrl2Input()) {
            ram_ff801c = 1;
            break;
        } else {
            ram_ff801c = 0;
            break;
        }
    case 2:
        gPlayerLives[0] = -1;
        gPlayerRank[0] = 9;
        gPlayerInfo[0].level = 99;
        gPlayerLives[1] = 0;
        gPlayerRank[1] = 0;
        gPlayerInfo[1].level = 0;
        PLAYER_INDICATOR = 1;
        if (GetCtrl1Input()) {
            ram_ff801c = 1;
            break;
        } else {
            ram_ff801c = 0;
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
        ram_ff8030 = MenuControllerOptionsHandler();
        goto select_game;
    case 4:
        gWorldType = 1;
        break;
    default:
        goto select_game;
    }
}

/* Display the game-over text until a button or timeout, then reset. */
void func_023554()
{
    func_01d802(0);
    VDP_COMMAND = 0x57160003L;
    func_009f8a("bogus ... game over");
    ForceTextBackgroundColorToBlack(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    SetVHBlankFlag2();
    TIME_COUNTER = 0;
    for (;;) {
        do {
            WaitForNextVBlank(1);
            func_008be4(8);
        } while (TIME_COUNTER <= 600 &&
            !((MovePadInputIntoD0(0) | MovePadInputIntoD0(1)) & 0xf0));
        PlaySound(6, 99);
        PlaySound(0, 98);
        func_03e43c();
        func_03e40e();
        gMode2RegSoftcopy &= 0x3c;
        VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
        ClearColourRAM();
        ClearSRAM();
        ClearVideoRAM();
        func_00034e();
    }
}

int MenuWhoHandler()
{
    int selection;
    AddMenuHeader(0);
    selection = GeneralMenuHandler(2, MenuWhoItems, MenuWhoItemCount);
    RemoveMenuHeader();
    return selection;
}

int MenuControllerOptionsHandler()
{
    int selection;
    AddMenuHeader(3);
    selection = GeneralMenuHandler(2, MenuControllerOptionsItems, MenuControllerOptionsItemCount);
    RemoveMenuHeader();
    return selection;
}

int MenuWhatHandler()
{
    int selection;
    AddMenuHeader(1);
    selection = GeneralMenuHandler(2, MenuWhatItems, MenuWhatItemCount);
    RemoveMenuHeader();
    return selection;
}

int MenuJamOutHandler()
{
    int selection;
    AddMenuHeader(2);
    selection = GeneralMenuHandler(2, MenuJamOutItems, MenuJamOutItemCount);
    RemoveMenuHeader();
    return selection;
}

/* Check both controller ports with each direction setting. */
int func_023740()
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
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_00046a();
    func_01d802(0);
    CopyPaletteToCRAMSlot(PaletteGameLine1, 0);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    ForceTextBackgroundColorToBlack(1);
    MenuDrawTextItems(items, item_count);
    func_00cc82();
    WaitForNextVBlank(1);
    SpriteLoadingStuff(0, 0);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    SetVHBlankFlag2();
    while ((GetCtrl1Input() | GetCtrl2Input()) & 0xf0) {
        WaitForNextVBlank(1);
        func_008be4(8);
    }
    func_023b08();
    selected_item = 0;
    selected = &items[0];
    for (;;) {
        VBLANK_FLAG_2 = 0;
        func_00cc82();
        WaitForNextVBlank(1);
        SpriteLoadingStuff(0, 0);
        func_008be4(8);
        func_00059e(0);
        GetRandomNumber();
        input = (unsigned char)GetCtrl1Input() | GetCtrl2Input();
        if (input != previous_input) {
            if (input & 0xf0) {
                if (items != MenuWhoItems || selected->selection || func_023740())
                    break;
                PlaySound(26, 50);
                VDP_COMMAND = 0x5c040003L;
                func_009f8a("need two controllers for two players");
            } else if (input & 1) {
                --selected_item;
                PlaySound(7, 50);
                if (selected_item < 0 || items[selected_item].selection < 0)
                    ++selected_item;
            } else if (input & 2) {
                PlaySound(7, 50);
                ++selected_item;
                if (selected_item >= item_count || items[selected_item].selection < 0)
                    --selected_item;
            }
            previous_input = input;
        }
        selected = &items[selected_item];
        func_023b8c(cursor_column, selected->y);
    }
    ClearVHBlankFlag2();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = gMode2RegSoftcopy | 0x8104;
    func_023b70();
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
        VDP_COMMAND = 0x50000003L + (item->y << 23) + (item->x << 17);
        func_009f8a(item->text);
    }
}

/* Allocate and initialize the animated menu cursor object. */
void func_023b08()
{
    struct object_state *cursor;
    if (gMenuCursorObjId = (int)func_00d904()) {
        cursor = &gPlayerInfo[gMenuCursorObjId];
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
        func_00d776(cursor, 0);
    }
}

void func_023b70()
{
    if (gMenuCursorObjId)
        func_00d970(gMenuCursorObjId);
}

/* Place the cursor at the selected tile coordinate with a six-pixel offset. */
void func_023b8c(column, row)
short column, row;
{
    struct object_state *cursor;
    if (gMenuCursorObjId) {
        cursor = &gPlayerInfo[gMenuCursorObjId];
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
    if (gMenuHeaderObjId = (int)func_00d904()) {
        header = &gPlayerInfo[gMenuHeaderObjId];
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
        func_00d776(header, 0);
    }
}

void RemoveMenuHeader()
{
    if (gMenuHeaderObjId)
        func_00d970(gMenuHeaderObjId);
}
