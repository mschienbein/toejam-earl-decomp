#include "entities.h"
#include "elevator.h"
#include "sprites.h"
#include "ui.h"
#include "hardware.h"
#include "assets_map.h"
#include "assets_palettes.h"
#include "assets_scenes.h"
#include "assets_sprites.h"
#include "assets_ui.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "engine_calls.h"
#include "hardware_calls.h"
#include "map_calls.h"
#include "player.h"
#include "present_calls.h"
#include "ram_camera.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "random.h"
#include "speech.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Select the two animation descriptors for an elevator's state and keep
 * the second entity at its descriptor's vertical offset from the first. */
void func_012694(elevator, state)
struct elevator_record *elevator;
char state;
{
    struct object_state *entity;
    struct timed_animation *animation;
    short x, y;

    elevator->state = state;
    if (state == 11) {
        elevator->level = -1;
        return;
    }
    if (!elevator->entity)
        return;
    entity = &gPlayerInfo[elevator->entity];
    animation = &dat_0aee98[state];
    entity->sequence = animation->sequence;
    entity->frames = animation->frames;
    entity->frame_phase = animation->period;
    entity->frame_duration = 1;
    entity->frame = -1;
    entity->sprite_state = state;
    x = entity->x;
    y = entity->y;
    entity = entity + 2;
    animation = &dat_0aef1a[state];
    entity->sequence = animation->sequence;
    entity->frames = animation->frames;
    entity->frame_phase = animation->period;
    entity->y = animation->y_offset + y;
    entity->x = x;
    entity->frame_duration = 1;
    entity->frame = -1;
    entity->sprite_state = state;
}

/* Convert the saved map-cell coordinates into the player's position. */
void func_012762(player)
struct object_state *player;
{
    char map_x, map_y;

    map_x = ram_ff911c;
    map_y = ram_ff911d;
    player->x = (map_x + 2) * 16 * 8 - 16;
    player->y = (map_y + 2) * 8 * 8 - 5;
}

/* Mark both secondary elevator records as absent. */
void LOAD_ELEVATOR_1()
{
    ELEVATOR_LOCATION[0][1].level = -1;
    ELEVATOR_LOCATION[1][1].level = -1;
    ELEVATOR_LOCATION[0][1].state = 11;
    ELEVATOR_LOCATION[1][1].state = 11;
}

/* Reuse the matching level record, or replace one of the two records for
 * this elevator type. Coordinates are converted from map cells to pixels. */
char SetupElevator(level, elevator_type)
char level, elevator_type;
{
    struct elevator_record *elevator, *alternate;
    char elevator_number, map_x, map_y;

    /* ORIGINAL BUG: an out-of-range ordinary level returns without a value. */
    if ((level < 1 || level >= 25) && !elevator_type)
        return;
    elevator_number = elevator_type + 1;
    elevator = &ELEVATOR_LOCATION[0][elevator_type];
    alternate = &ELEVATOR_LOCATION[1][elevator_type];
    if (level == elevator->level)
        return elevator_number;
    if (level == alternate->level)
        return elevator_type + 3;
    if (gPlayerInfo[0].level != gPlayerInfo[1].level &&
        ((level == gPlayerInfo[0].level && elevator->level == gPlayerInfo[1].level) ||
         (level == gPlayerInfo[1].level && elevator->level == gPlayerInfo[0].level))) {
        elevator = alternate;
        elevator_number = elevator_type + 3;
    }
    elevator->level = level;
    if (elevator->entity) {
        func_00d970(elevator->entity);
        func_00d970(elevator->entity + 2);
        elevator->entity = 0;
    }
    if (!elevator_type) {
        if (ELEVATOR_LOCATION[0][1].level == level + 1 ||
            ELEVATOR_LOCATION[1][1].level == level + 1)
            elevator->state = 13;
        else
            elevator->state = 0;
    } else
        elevator->state = 8;
    if (level == 1) {
        map_x = 9;
        map_y = 10;
    } else if (level == 26) {
        map_x = 2;
        map_y = 21;
    } else {
        map_x = ELEVATOR_POS_TABLE[elevator_type].x;
        map_y = ELEVATOR_POS_TABLE[elevator_type].y;
    }
    elevator->x = (map_x + 2) * 16 * 8 - 16;
    elevator->y = (map_y + 2) * 8 * 8 - 5;
    return elevator_number;
}

/* Set up the level-one elevator and invalidate the remaining records. */
void SetupInitialElevator()
{
    SetupElevator(1, 0);
    ELEVATOR_LOCATION[0][1].state = 11;
    ELEVATOR_LOCATION[0][1].level = -1;
    ELEVATOR_LOCATION[1][0].state = 11;
    ELEVATOR_LOCATION[1][0].level = -1;
    ELEVATOR_LOCATION[1][1].state = 11;
    ELEVATOR_LOCATION[1][1].level = -1;
}

/* Claim one of the two elevator entity pairs and initialize both halves. */
void func_012974(elevator)
struct elevator_record *elevator;
{
    char entity_number = 2;
    struct object_state *entity;

    if (gPlayerInfo[2].draw_flags & 1) {
        entity_number = 3;
        if (gPlayerInfo[3].draw_flags & 1)
            return;
    }
    elevator->entity = entity_number;
    entity = &gPlayerInfo[entity_number];
    func_00d8f2(entity);
    entity->x = elevator->x;
    entity->y = elevator->y;
    entity->height = 0;
    entity->level = elevator->level;
    entity->width = 40;
    entity->height_pixels = 48;
    entity->sprite_count = 2;
    entity->tile_count = 28;
    entity->tile = 0x8000;
    entity->draw_flags = 1;
    entity->flags_4d |= 1;
    entity = entity + 2;
    func_00d8f2(entity);
    entity->x = elevator->x;
    entity->y = elevator->y;
    entity->height = 0;
    entity->level = elevator->level;
    entity->width = 40;
    entity->height_pixels = 48;
    entity->sprite_count = 2;
    entity->tile_count = 28;
    entity->tile = 0x8000;
    entity->draw_flags = 1;
    entity->flags_4d |= 1;
    func_012694(elevator, elevator->state);
}

/* Allocate elevator sprites near either active camera and release pairs
 * that leave both camera regions or enter an inactive state. */
void func_012a56()
{
    short pair, elevator_type;
    short primary_x, primary_y, secondary_x, secondary_y;
    struct elevator_record *elevator;

    for (pair = 0; pair <= 1; ++pair)
        for (elevator_type = 0; elevator_type <= 1; ++elevator_type) {
            elevator = &ELEVATOR_LOCATION[pair][elevator_type];
            primary_x = elevator->x - TOEJAM_CAMERA_NEWX;
            primary_y = elevator->y - TOEJAM_CAMERA_NEWY;
            secondary_x = elevator->x - EARL_CAMERA_NEWX;
            secondary_y = elevator->y - EARL_CAMERA_NEWY;
            if (elevator->state != 11 && elevator->state != 13 &&
                elevator->level > 0 &&
                ((elevator->level == gLevel912C && primary_x + 160 > 0 &&
                  primary_x < 480 && primary_y + 112 > 0 && primary_y < 336) ||
                 (elevator->level == gLevel912E && SPLIT_SCREEN &&
                  secondary_x + 160 > 0 && secondary_x < 480 &&
                  secondary_y + 112 > 0 && secondary_y < 336))) {
                if (!elevator->entity)
                    func_012974(elevator);
            } else {
                if (elevator->entity) {
                    func_00d970(elevator->entity);
                    func_00d970(elevator->entity + 2);
                    elevator->entity = 0;
                }
                if (elevator->state == 11)
                    elevator->level = -1;
            }
        }
}

/* Test passage across elevator thresholds and update the player's current
 * elevator identifier when crossing into an open elevator. */
int func_012b80(player_number, x, y, elevation)
short player_number, x, y, elevation;
{
    short elevator_type, pair, distance_x, distance_y;
    char elevator_number;
    struct object_state *player;
    struct elevator_record *elevator;

    player = &gPlayerInfo[player_number];
    for (elevator_type = 0; elevator_type <= 1; ++elevator_type)
        for (pair = 0; pair <= 1; ++pair) {
            elevator_number = pair * 2 + elevator_type + 1;
            elevator = &ELEVATOR_LOCATION[pair][elevator_type];
            if (elevator->state != 11 && elevator->state != 13 &&
                player->level == elevator->level) {
                distance_x = abs(x - elevator->x);
                if (ELEVATOR_FLAG[player_number] == elevator_number) {
                    if (elevator->state != 3)
                        return 1;
                    if (y <= elevator->y || distance_x)
                        return 1;
                    if (y >= elevator->y + 10)
                        ELEVATOR_FLAG[player_number] = 0;
                } else if (distance_x < 20) {
                    if ((player->y > elevator->y && y < elevator->y) ||
                        (player->y < elevator->y && y > elevator->y))
                        return 1;
                    distance_y = abs(y - (elevator->y + 5));
                    if (distance_y < 7) {
                        if (player->height > 0)
                            return 1;
                        if (elevator->state == 3 && distance_x < 10 &&
                            player->y > elevator->y) {
                            if (!elevation && y < elevator->y + 8)
                                ELEVATOR_FLAG[player_number] = elevator_number;
                        } else
                            return 1;
                    }
                }
            }
        }
    return 0;
}

/* Test movement across an active same-level elevator threshold. */
int func_012d0c(player, x, y)
struct object_state *player;
short x, y;
{
    /* The original reads this character byte, but its value does not
     * contribute to the threshold predicate below. */
    char player_number = player->entity_number;
    struct elevator_record *elevator;
    short record_number, distance_y;

    elevator = &ELEVATOR_LOCATION[0][0];
    for (record_number = 0; record_number < 4; ++record_number, ++elevator) {
        if (elevator->state != 11 && elevator->state != 13 &&
            player->level == elevator->level && abs(x - elevator->x) < 20) {
            distance_y = abs(y - (elevator->y + 5));
            if (distance_y < 7 &&
                ((y < player->y && player->y > elevator->y) ||
                 (y > player->y && player->y < elevator->y)))
                return 1;
        }
    }
    return 0;
}

/* Test the four elevator regions on the requested level. This predicate
 * does not filter records by elevator state. */
int func_012db8(x, y, level)
short x, y, level;
{
    struct elevator_record *elevator;
    short record_number;

    elevator = &ELEVATOR_LOCATION[0][0];
    for (record_number = 0; record_number < 4; ++record_number, ++elevator)
        if (elevator->level == level &&
            abs(elevator->y - y + 5) < 15 &&
            abs(elevator->x - x) < 20)
            return 1;
    return 0;
}

/* Upload the final-level tiles, extra runs, palettes and scroll values. */
void LoadFinalLevelMap()
{
    short scroll;

    VDP_COMMAND = 0x40000000L;
    MoveDataToVRAM(LevelTiles, LevelTilesSize << 4);
    func_008b98();
    VDP_COMMAND = 0x42800000L;
    MoveDataToVRAM(FunkatronExtraTiles, 80);
    VDP_COMMAND = 0x48200000L;
    MoveDataToVRAM(&FunkatronExtraTiles[160], 16);
    VDP_COMMAND = 0x52400000L;
    MoveDataToVRAM(&FunkatronExtraTiles[192], 48);
    VDP_COMMAND = 0x6a800000L;
    MoveDataToVRAM(&FunkatronExtraTiles[288], 896);
    CopyPaletteToCRAMSlot(PaletteShipScreen_bank3, 0);
    CopyPaletteToCRAMSlot(FunkatronPaletteA, 1);
    CopyPaletteToCRAMSlot(FunkatronPaletteB, 2);
    CopyPaletteToCRAMSlot(PaletteGameLine4, 3);
    VDP_CONTROL_WORD = 0x9200;
    func_01d8ac();
    scroll = 0;
    VDP_COMMAND = 0x40020010L;
    VDP_DATA = scroll;
    VDP_COMMAND = 0x7c020003L;
    VDP_DATA = scroll;
    func_00b95c(2);
    ToggleLOC_00FF8196(0);
}

/* Remove active-level objects and present overlays, then reset four elevators. */
void func_012f34()
{
    short player_number, elevator_number;
    struct object_state *player;
    struct elevator_record *elevator;

    if (gPlayerLives[0] >= 0) {
        func_0174b6(gPlayerInfo[0].level);
        func_0143e6(gPlayerInfo[0].level);
        ELEVATOR_FLAG[0] = 0;
    }
    if (gPlayer2Lives >= 0) {
        func_0174b6(gPlayerInfo[1].level);
        func_0143e6(gPlayerInfo[1].level);
        ram_ffda6b = 0;
    }
    player = gPlayerInfo;
    for (player_number = 0; player_number <= 1; ++player_number, ++player) {
        if (player->draw_flags & 1) {
            if (player->draw_flags & 2)
                OpeningPresents7(player, 0);
            if (player->draw_flags & 4)
                OpeningPresents7(player, 1);
        }
    }
    elevator = &ELEVATOR_LOCATION[0][0];
    for (elevator_number = 0; elevator_number < 4; ++elevator_number, ++elevator) {
        if (elevator->entity) {
            func_00d970(elevator->entity);
            func_00d970(elevator->entity + 2);
            elevator->entity = 0;
        }
        func_012694(elevator, 11);
    }
    func_01d436();
}

/* Move both active players to level 26, reset sprites, and load the final map. */
void func_013036()
{
    struct object_state *player;

    if (gPlayerLives[0] >= 0) {
        gPlayerInfo[0].level = 26;
        func_004dfe(0, gPlayerInfo[0].level);
        player = &gPlayerInfo[0];
        gPlayerInfo[0].height = 0;
        StopPlayer(player);
        TOEJAM_FALL_STATE[0] = 0;
        SetSprite(player, 0);
        player->flags_4d &= 0xaf;
        player->tile &= 0x7fff;
    }
    if (gPlayer2Lives >= 0) {
        gPlayerInfo[1].level = 26;
        func_004dfe(1, gPlayerInfo[1].level);
        player = &gPlayerInfo[1];
        gPlayerInfo[1].height = 0;
        StopPlayer(player);
        ram_ffda23 = 0;
        SetSprite(player, 0);
        player->flags_4d &= 0xaf;
        player->tile &= 0x7fff;
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    LoadFinalLevelMap();
    func_01e2c4();
    gPlayerInfo[0].x = 496;
    gPlayerInfo[0].y = 1467;
    gPlayerInfo[1].x = gPlayerInfo[0].x + 32;
    gPlayerInfo[1].y = gPlayerInfo[0].y;
    func_00105a(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
}

/* Unpack four background parts and queue the resulting 64-tile run. */
void func_013160()
{
    short part_number;
    char packing;
    /* The 4-piece payload view preserves MCC68K word indexing. Four
     * pieces are observed in each transition frame; the common frame type remains
     * only its four-byte prefix. */
    struct render_frame *frame;
    struct render_piece *part;

    frame = Spr_TransitionBackground[gLevel912C * 2];
    for (part_number = 0; part_number < 4; ++part_number) {
        part = &((struct render_piece (*)[4])(frame + 1))[0][part_number];
        packing = part->format;
        if (packing < 0)
            SpriteUnpackInterleavedRLE(part, 16);
        else
            SpriteUnpackRLE(part, 16);
        SPRITE_STATE_TABLE += 512;
    }
    ram_ffd8dc[ram_ffd92e++] = 1;
    ram_ffd8dc[ram_ffd92e++] = 64;
    SpriteLoadingStuff(1, 1);
}

/* Alternate the two ride sprites every sixteen ticks and transfer the table. */
void func_01320e()
{
    struct object_state *first, *second;

    first = ram_ffda6e;
    second = first + 2;
    if (!(TIME_COUNTER & 15)) {
        if (first->sprite_ready) {
            first->sprite_ready = 0;
            second->sprite_ready = 1;
        } else {
            first->sprite_ready = 1;
            second->sprite_ready = 0;
        }
        func_00c4fa();
        VDPWritesRoutine1(ram_ffb65a, 0xf800, 320);
        ram_ffd932 = ram_ffb715;
    }
}

/* Choose an unused two-character exchange, create its lines, and hide them. */
void func_01326e(entity)
struct object_state *entity;
{
    short speech_number, attempt, first_line, second_line = 0;
    short reply_first_line, reply_second_line = 0;
    unsigned short random_value;

    if (gPlayerLives[0] >= 0 && gPlayer2Lives >= 0 &&
        (random_value = GetRandomNumber()) % 100 < 75) {
        speech_number = GetRandomNumber() % 14;
        for (attempt = 0; attempt < 14; ++attempt) {
            if (!((1L << speech_number) & ram_ffda7e))
                break;
            speech_number = (speech_number + 1) % 14;
        }
        if (attempt != 14) {
            ram_ffda7e |= 1L << speech_number;
            entity->x -= 24;
            first_line = GenerateSpeech(entity->entity_number, SpeechStringPointers[speech_number].first_line, 0);
            if (SpeechStringPointers[speech_number].second_line)
                second_line = GenerateSpeech(entity->entity_number, SpeechStringPointers[speech_number].second_line, 0);
            entity->x += 40;
            reply_first_line = GenerateSpeech(entity->entity_number, SpeechStringPointers[speech_number].reply_first_line, 0);
            if (SpeechStringPointers[speech_number].reply_second_line)
                reply_second_line = GenerateSpeech(entity->entity_number, SpeechStringPointers[speech_number].reply_second_line, 0);
            entity->x -= 16;
            func_00cc82();
            if (ram_ffda7a[0] = first_line)
                gPlayerInfo[first_line].sprite_ready = 0;
            if (ram_ffda7a[1] = second_line)
                gPlayerInfo[second_line].sprite_ready = 0;
            if (ram_ffda7a[2] = reply_first_line)
                gPlayerInfo[reply_first_line].sprite_ready = 0;
            if (ram_ffda7a[3] = reply_second_line)
                gPlayerInfo[reply_second_line].sprite_ready = 0;
            func_00c4fa();
            return;
        }
    }
    ram_ffda7a[0] = 0;
    ram_ffda7a[1] = 0;
    ram_ffda7a[2] = 0;
    ram_ffda7a[3] = 0;
}

/* Reveal and hide each speech line at its fixed point in the ride. */
void func_013450()
{
    short line;

    if (TIME_COUNTER == 60) {
        if (line = ram_ffda7a[0])
            gPlayerInfo[line].sprite_ready = 1;
    } else if (TIME_COUNTER == 120) {
        if (line = ram_ffda7a[0])
            gPlayerInfo[line].sprite_ready = 0;
        if (line = ram_ffda7a[1])
            gPlayerInfo[line].sprite_ready = 1;
        else
            TIME_COUNTER = 180;
    } else if (TIME_COUNTER == 180) {
        if (line = ram_ffda7a[1])
            gPlayerInfo[line].sprite_ready = 0;
    } else if (TIME_COUNTER == 300) {
        if (line = ram_ffda7a[2])
            gPlayerInfo[line].sprite_ready = 1;
    } else if (TIME_COUNTER == 360) {
        if (line = ram_ffda7a[2])
            gPlayerInfo[line].sprite_ready = 0;
        if (line = ram_ffda7a[3])
            gPlayerInfo[line].sprite_ready = 1;
        else
            TIME_COUNTER = 419;
    } else if (TIME_COUNTER == 420)
        func_01e2c4();
}

/* Integrate the ride's scroll speed and run its per-blank sprite work. */
void ElevatorRideVBlank()
{
    if (ram_ffda78 < 4096)
        ++ram_ffda78;
    if (ram_ffda78 < 0 || ram_ffda76 < 2048)
        ram_ffda76 += ram_ffda78 >> 8;
    ram_ffda72 -= ram_ffda76;
    VDP_COMMAND = 0x40000010L;
    VDP_DATA = ram_ffda72 >> 8;
    func_013450();
    func_01320e();
    func_00d022();
    MassiveRoutine();
    ++TIME_COUNTER;
}

/* Wait for the elevator sprites, fill the ride background, and install VBlank. */
void func_013570(elevator)
struct elevator_record *elevator;
{
    short band, row, repeat, column;
    unsigned short tile;
    struct object_state *first, *second;

    ram_ffda6e = &gPlayerInfo[elevator->entity];
    first = ram_ffda6e;
    second = first + 2;
    first->x = TOEJAM_CAMERA_NEWX + 160;
    first->y = TOEJAM_CAMERA_NEWY + 144;
    first->level = gLevel912C;
    first->sprite_ready = 0;
    second->level = gLevel912C;
    second->sprite_ready = 0;
    func_012694(elevator, 12);
    func_01d436();
    do {
        func_00cc82();
        while (VBLANK_FLAG_2)
            ;
    } while (!first->sprite_ready || !second->sprite_ready);
    ClearSkipVHBlankFlags();
    func_01326e(first);
    func_008c5c();
    for (band = 0; band < 4; ++band)
        for (row = 0; row < 8; ++row) {
            VDP_COMMAND = (((band << 3) + row) << 23) + 0x60000002L;
            for (repeat = 0; repeat < 6; ++repeat)
                for (column = 0; column < 8; ++column) {
                    tile = ShipRelatedData[row][column] + 0x2001;
                    if (SpritePriorityLookup[gLevel912C])
                        tile |= 0x8000;
                    VDP_DATA = tile;
                }
        }
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    func_013160();
    func_00b95c(2);
    ram_ffda72 = 0;
    VDP_COMMAND = 0x40000010L;
    VDP_DATA = ram_ffda74;
    ram_ffda76 = 128;
    ram_ffda78 = 1024;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = 0xffff;
    TIME_COUNTER = 0;
    gVBlankFunc = ElevatorRideVBlank;
}

/* Wait for peak speed, decelerate, and restore the normal palette and tiles. */
void func_01370e()
{
    while (ram_ffda76 < 2048)
        ;
    ram_ffda78 = -2048;
    while (ram_ffda76 > 0)
        ;
    gVBlankFunc = 0;
    func_01e2c4();
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    VDP_COMMAND = 0x40000000L;
    MoveDataToVRAM(LevelTiles, 1040);
}

/* Advance the selected elevator's players, show the shared ride when possible,
 * then create the destination elevator and restore the game display. */
void func_013786(elevator_number)
char elevator_number;
{
    char destination_level, first_rides = 0, second_rides = 0;
    short destination_y, destination_x;
    struct elevator_record *elevator;

    if (elevator_number == ELEVATOR_FLAG[0]) {
        func_0174b6(gPlayerInfo[0].level);
        func_0143e6(gPlayerInfo[0].level);
        destination_level = ++gPlayerInfo[0].level;
        first_rides = 1;
    }
    if (elevator_number == ram_ffda6b) {
        func_0174b6(gPlayerInfo[1].level);
        func_0143e6(gPlayerInfo[1].level);
        destination_level = ++gPlayerInfo[1].level;
        second_rides = 1;
    }
    if ((gPlayerLives[0] < 0 || first_rides) && (gPlayer2Lives < 0 || second_rides)) {
        VBLANK_FLAG_7 = 6;
        while (VBLANK_FLAG_8 != VBLANK_FLAG_7)
            ;
        elevator = &ELEVATOR_LOCATION[(elevator_number - 1) >> 1][0];
        func_013570(elevator);
    }
    if (first_rides) {
        func_004dfe(0, gPlayerInfo[0].level);
        func_017ac8(gPlayerInfo[0].level);
        PlaceStuffInLevel(gPlayerInfo[0].level);
    }
    if (second_rides) {
        func_004dfe(1, gPlayerInfo[1].level);
        func_017ac8(gPlayerInfo[1].level);
        PlaceStuffInLevel(gPlayerInfo[1].level);
    }
    if (gVBlankFunc) {
        func_01370e();
        func_012694(elevator, 11);
    }
    ClearSkipVHBlankFlags();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    if (destination_level < 25)
        SetupElevator(destination_level, 0);
    elevator_number = SetupElevator(destination_level, 1);
    elevator = &ELEVATOR_LOCATION[(elevator_number - 1) >> 1][1];
    destination_y = elevator->y + 5;
    destination_x = elevator->x;
    if (second_rides) {
        gPlayerInfo[1].y = destination_y;
        gPlayerInfo[1].x = destination_x;
        func_00aa5a(1);
        ram_ffda6b = elevator_number;
        if (destination_level < 26)
            func_02297c(1);
    }
    if (first_rides) {
        gPlayerInfo[0].y = destination_y;
        gPlayerInfo[0].x = destination_x;
        func_00aa5a(0);
        ELEVATOR_FLAG[0] = elevator_number;
        if (destination_level < 26)
            func_02297c(0);
    }
    if (destination_level == 26)
        LoadFinalLevelMap();
    ram_ffda6c = 0;
    func_00105a(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = 0x8104 | gMode2RegSoftcopy;
    SetSkipVHBlankFlags();
    if (VBLANK_FLAG_8 == 6) {
        func_03e43c();
        func_03e40e();
    }
}
