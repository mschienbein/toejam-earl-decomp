#include "player_animation.h"
#include "music_ids.h"
#include "entities.h"
#include "elevator.h"
#include "elevator_states.h"
#include "sprites.h"
#include "sprite_frame_views.h"
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
void ElevatorSetState(elevator, state)
struct elevator_record *elevator;
char state;
{
    struct object_state *entity;
    struct timed_animation *animation;
    short x, y;

    elevator->state = state;
    if (state == ELEVATOR_STATE_ABSENT) {
        elevator->level = -1;
        return;
    }
    if (!elevator->entity)
        return;
    entity = &gPlayerInfo[elevator->entity];
    animation = &ElevatorPrimaryAnimations[state];
    entity->sequence = animation->sequence;
    entity->frames = animation->frames;
    entity->frame_phase = animation->period;
    entity->frame_duration = 1;
    entity->frame = -1;
    entity->sprite_state = state;
    x = entity->x;
    y = entity->y;
    entity = entity + 2;
    animation = &ElevatorSecondaryAnimations[state];
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
void PlayerSetPositionFromSavedMapCell(player)
struct object_state *player;
{
    char map_x, map_y;

    map_x = gArrivalElevatorMapX;
    map_y = gArrivalElevatorMapY;
    player->x = (map_x + 2) * 16 * 8 - 16;
    player->y = (map_y + 2) * 8 * 8 - 5;
}

/* Mark both secondary elevator records as absent. */
void ElevatorInvalidateSecondaryRecords()
{
    gElevatorRecords[0][1].level = -1;
    gElevatorRecords[1][1].level = -1;
    gElevatorRecords[0][1].state = ELEVATOR_STATE_ABSENT;
    gElevatorRecords[1][1].state = ELEVATOR_STATE_ABSENT;
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
    elevator = &gElevatorRecords[0][elevator_type];
    alternate = &gElevatorRecords[1][elevator_type];
    if (level == elevator->level)
        return elevator_number;
    if (level == alternate->level)
        return elevator_type + 3;
    if (gPlayerInfo[PLAYER_TOEJAM].level != gPlayerInfo[PLAYER_EARL].level &&
        ((level == gPlayerInfo[PLAYER_TOEJAM].level && elevator->level == gPlayerInfo[PLAYER_EARL].level) ||
         (level == gPlayerInfo[PLAYER_EARL].level && elevator->level == gPlayerInfo[PLAYER_TOEJAM].level))) {
        elevator = alternate;
        elevator_number = elevator_type + 3;
    }
    elevator->level = level;
    if (elevator->entity) {
        FreeEntity(elevator->entity);
        FreeEntity(elevator->entity + 2);
        elevator->entity = 0;
    }
    if (!elevator_type) {
        if (gElevatorRecords[0][1].level == level + 1 ||
            gElevatorRecords[1][1].level == level + 1)
            elevator->state = ELEVATOR_STATE_BLOCKED_BY_ARRIVAL;
        else
            elevator->state = ELEVATOR_STATE_CLOSED_EMPTY;
    } else
        elevator->state = ELEVATOR_STATE_ARRIVAL_START;
    if (level == 1) {
        map_x = 9;
        map_y = 10;
    } else if (level == LEVEL_FINAL) {
        map_x = 2;
        map_y = 21;
    } else {
        map_x = gLevelAnchorPositions[elevator_type].x;
        map_y = gLevelAnchorPositions[elevator_type].y;
    }
    elevator->x = (map_x + 2) * 16 * 8 - 16;
    elevator->y = (map_y + 2) * 8 * 8 - 5;
    return elevator_number;
}

/* Set up the level-one elevator and invalidate the remaining records. */
void SetupInitialElevator()
{
    SetupElevator(1, 0);
    gElevatorRecords[0][1].state = ELEVATOR_STATE_ABSENT;
    gElevatorRecords[0][1].level = -1;
    gElevatorRecords[1][0].state = ELEVATOR_STATE_ABSENT;
    gElevatorRecords[1][0].level = -1;
    gElevatorRecords[1][1].state = ELEVATOR_STATE_ABSENT;
    gElevatorRecords[1][1].level = -1;
}

/* Claim one of the two elevator entity pairs and initialize both halves. */
void ElevatorCreateEntityPair(elevator)
struct elevator_record *elevator;
{
    char entity_number = 2;
    struct object_state *entity;

    if (gPlayerInfo[2].draw_flags & ENTITY_ACTIVE) {
        entity_number = 3;
        if (gPlayerInfo[3].draw_flags & ENTITY_ACTIVE)
            return;
    }
    elevator->entity = entity_number;
    entity = &gPlayerInfo[entity_number];
    ResetEntityRuntimeState(entity);
    entity->x = elevator->x;
    entity->y = elevator->y;
    entity->height = 0;
    entity->level = elevator->level;
    entity->width = 40;
    entity->height_pixels = 48;
    entity->sprite_count = 2;
    entity->tile_count = 28;
    entity->tile = VDP_TILE_PRIORITY;
    entity->draw_flags = ENTITY_ACTIVE;
    entity->behavior_flags |= ENTITY_COLLIDABLE;
    entity = entity + 2;
    ResetEntityRuntimeState(entity);
    entity->x = elevator->x;
    entity->y = elevator->y;
    entity->height = 0;
    entity->level = elevator->level;
    entity->width = 40;
    entity->height_pixels = 48;
    entity->sprite_count = 2;
    entity->tile_count = 28;
    entity->tile = VDP_TILE_PRIORITY;
    entity->draw_flags = ENTITY_ACTIVE;
    entity->behavior_flags |= ENTITY_COLLIDABLE;
    ElevatorSetState(elevator, elevator->state);
}

/* Allocate elevator sprites near either active camera and release pairs
 * that leave both camera regions or enter an inactive state. */
void ElevatorUpdateEntityVisibility()
{
    short pair, elevator_type;
    short primary_x, primary_y, secondary_x, secondary_y;
    struct elevator_record *elevator;

    for (pair = 0; pair <= 1; ++pair)
        for (elevator_type = 0; elevator_type <= 1; ++elevator_type) {
            elevator = &gElevatorRecords[pair][elevator_type];
            primary_x = elevator->x - gCamera1PendingX;
            primary_y = elevator->y - gCamera1PendingY;
            secondary_x = elevator->x - gCamera2PendingX;
            secondary_y = elevator->y - gCamera2PendingY;
            if (elevator->state != ELEVATOR_STATE_ABSENT && elevator->state != ELEVATOR_STATE_BLOCKED_BY_ARRIVAL &&
                elevator->level > 0 &&
                ((elevator->level == gPlayer1MapLevel && primary_x + 160 > 0 &&
                  primary_x < 480 && primary_y + 112 > 0 && primary_y < 336) ||
                 (elevator->level == gPlayer2MapLevel && gSplitScreenEnabled &&
                  secondary_x + 160 > 0 && secondary_x < 480 &&
                  secondary_y + 112 > 0 && secondary_y < 336))) {
                if (!elevator->entity)
                    ElevatorCreateEntityPair(elevator);
            } else {
                if (elevator->entity) {
                    FreeEntity(elevator->entity);
                    FreeEntity(elevator->entity + 2);
                    elevator->entity = 0;
                }
                if (elevator->state == ELEVATOR_STATE_ABSENT)
                    elevator->level = -1;
            }
        }
}

/* Test passage across elevator thresholds and update the player's current
 * elevator identifier when crossing into an open elevator. */
int ElevatorBlocksPlayerMovement(player_number, x, y, elevation)
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
            elevator = &gElevatorRecords[pair][elevator_type];
            if (elevator->state != ELEVATOR_STATE_ABSENT && elevator->state != ELEVATOR_STATE_BLOCKED_BY_ARRIVAL &&
                player->level == elevator->level) {
                distance_x = abs(x - elevator->x);
                if (gPlayerElevatorFlags[player_number] == elevator_number) {
                    if (elevator->state != ELEVATOR_STATE_OPEN)
                        return 1;
                    if (y <= elevator->y || distance_x)
                        return 1;
                    if (y >= elevator->y + 10)
                        gPlayerElevatorFlags[player_number] = 0;
                } else if (distance_x < 20) {
                    if ((player->y > elevator->y && y < elevator->y) ||
                        (player->y < elevator->y && y > elevator->y))
                        return 1;
                    distance_y = abs(y - (elevator->y + 5));
                    if (distance_y < 7) {
                        if (player->height > 0)
                            return 1;
                        if (elevator->state == ELEVATOR_STATE_OPEN && distance_x < 10 &&
                            player->y > elevator->y) {
                            if (!elevation && y < elevator->y + 8)
                                gPlayerElevatorFlags[player_number] = elevator_number;
                        } else
                            return 1;
                    }
                }
            }
        }
    return 0;
}

/* Test movement across an active same-level elevator threshold. */
int ElevatorBlocksEntityMovement(player, x, y)
struct object_state *player;
short x, y;
{
    /* The original reads this character byte, but its value does not
     * contribute to the threshold predicate below. */
    char player_number = player->entity_index;
    struct elevator_record *elevator;
    short record_number, distance_y;

    elevator = &gElevatorRecords[0][0];
    for (record_number = 0; record_number < 4; ++record_number, ++elevator) {
        if (elevator->state != ELEVATOR_STATE_ABSENT && elevator->state != ELEVATOR_STATE_BLOCKED_BY_ARRIVAL &&
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
int ElevatorContainsPosition(x, y, level)
short x, y, level;
{
    struct elevator_record *elevator;
    short record_number;

    elevator = &gElevatorRecords[0][0];
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

    VDP_COMMAND = VDP_WRITE_VRAM_BASE;
    MoveDataToVRAM(LevelTiles, LevelTilesSize << 4);
    TextCopyFontBanksToRam();
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
    DrawNestedWindowFrame();
    scroll = 0;
    VDP_COMMAND = 0x40020010L;
    VDP_DATA = scroll;
    VDP_COMMAND = 0x7c020003L;
    VDP_DATA = scroll;
    HudHidePlayerSprites(2);
    SetCameraTilePriorityEnabled(0);
}

/* Remove active-level objects and present overlays, then reset four elevators. */
void MapClearActiveLevelObjects()
{
    short player_number, elevator_number;
    struct object_state *player;
    struct elevator_record *elevator;

    if (gPlayerLives[PLAYER_TOEJAM] >= 0) {
        EarthlingReleaseLevelBank(gPlayerInfo[PLAYER_TOEJAM].level);
        MapReleaseLevelItemBank(gPlayerInfo[PLAYER_TOEJAM].level);
        gPlayerElevatorFlags[PLAYER_TOEJAM] = 0;
    }
    if (gPlayer2Lives >= 0) {
        EarthlingReleaseLevelBank(gPlayerInfo[PLAYER_EARL].level);
        MapReleaseLevelItemBank(gPlayerInfo[PLAYER_EARL].level);
        gPlayer2ElevatorId = 0;
    }
    player = gPlayerInfo;
    for (player_number = PLAYER_TOEJAM; player_number <= PLAYER_EARL; ++player_number, ++player) {
        if (player->draw_flags & ENTITY_ACTIVE) {
            if (player->draw_flags & ENTITY_PRIMARY_VIEW)
                ReleaseEntityViewportSprites(player, 0);
            if (player->draw_flags & ENTITY_SECONDARY_VIEW)
                ReleaseEntityViewportSprites(player, 1);
        }
    }
    elevator = &gElevatorRecords[0][0];
    for (elevator_number = 0; elevator_number < 4; ++elevator_number, ++elevator) {
        if (elevator->entity) {
            FreeEntity(elevator->entity);
            FreeEntity(elevator->entity + 2);
            elevator->entity = 0;
        }
        ElevatorSetState(elevator, ELEVATOR_STATE_ABSENT);
    }
    ClearTransientEntities();
}

/* Move both active players to level 26, reset sprites, and load the final map. */
void MapEnterFinalLevel()
{
    struct object_state *player;

    if (gPlayerLives[PLAYER_TOEJAM] >= 0) {
        gPlayerInfo[PLAYER_TOEJAM].level = LEVEL_FINAL;
        MapLoadPlayerLevel(PLAYER_TOEJAM, gPlayerInfo[PLAYER_TOEJAM].level);
        player = &gPlayerInfo[PLAYER_TOEJAM];
        gPlayerInfo[PLAYER_TOEJAM].height = 0;
        StopPlayer(player);
        gPlayerFallStates[PLAYER_TOEJAM] = 0;
        SetSprite(player, PLAYER_ANIM_IDLE);
        player->behavior_flags &= ~(ENTITY_PLAYER_SUPPRESSED | ENTITY_HIDDEN);
        player->tile &= ~VDP_TILE_PRIORITY;
    }
    if (gPlayer2Lives >= 0) {
        gPlayerInfo[PLAYER_EARL].level = LEVEL_FINAL;
        MapLoadPlayerLevel(PLAYER_EARL, gPlayerInfo[PLAYER_EARL].level);
        player = &gPlayerInfo[PLAYER_EARL];
        gPlayerInfo[PLAYER_EARL].height = 0;
        StopPlayer(player);
        gPlayer2FallState = 0;
        SetSprite(player, PLAYER_ANIM_IDLE);
        player->behavior_flags &= ~(ENTITY_PLAYER_SUPPRESSED | ENTITY_HIDDEN);
        player->tile &= ~VDP_TILE_PRIORITY;
    }
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    LoadFinalLevelMap();
    ClearSpeechBubblesAndReplies();
    gPlayerInfo[PLAYER_TOEJAM].x = 496;
    gPlayerInfo[PLAYER_TOEJAM].y = 1467;
    gPlayerInfo[PLAYER_EARL].x = gPlayerInfo[PLAYER_TOEJAM].x + 32;
    gPlayerInfo[PLAYER_EARL].y = gPlayerInfo[PLAYER_TOEJAM].y;
    UpdateScreenSplitMode(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    EnableBlankingGraphics();
}

/* Unpack four background parts and queue the resulting 64-tile run. */
void ElevatorLoadRideBackgroundTiles()
{
    short part_number;
    char packing;
    /* MATCHING QUALIFICATION: the four-piece access view retains MCC68K
     * word indexing. Four pieces exist in these transition frames; the
     * common frame type remains only its four-byte prefix. */
    struct render_frame *frame;
    struct render_piece *part;

    frame = Spr_TransitionBackground[gPlayer1MapLevel * 2];
    for (part_number = 0; part_number < 4; ++part_number) {
        part = &((struct render_frame_four_piece_view *)frame)->pieces[part_number];
        packing = part->tile_format;
        if (packing < 0)
            SpriteUnpackInterleavedRLE(part, 16);
        else
            SpriteUnpackRLE(part, 16);
        gSpriteTileBufferOffset += 512;
    }
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 1;
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 64;
    UploadQueuedSpriteGraphics(1, 1);
}

/* Alternate the two ride sprites every sixteen ticks and transfer the table. */
void ElevatorAlternateRideEntities()
{
    struct object_state *first, *second;

    first = gElevatorRideEntity;
    second = first + 2;
    if (!(gFrameCounter & 15)) {
        if (first->sprite_ready) {
            first->sprite_ready = 0;
            second->sprite_ready = 1;
        } else {
            first->sprite_ready = 1;
            second->sprite_ready = 0;
        }
        BuildViewportSpriteLinks();
        DmaCopyWordsToVram(gHardwareSpriteTable, 0xf800, 320);
        gPrimaryViewportUploadedLink = gPrimaryViewportSentinelLink;
    }
}

/* Choose an unused two-character exchange, create its lines, and hide them. */
void ElevatorPrepareRideDialogue(entity)
struct object_state *entity;
{
    short speech_number, attempt, first_line, second_line = 0;
    short reply_first_line, reply_second_line = 0;
    unsigned short random_value;

    if (gPlayerLives[PLAYER_TOEJAM] >= 0 && gPlayer2Lives >= 0 &&
        (random_value = GetRandomNumber()) % 100 < 75) {
        speech_number = GetRandomNumber() % 14;
        for (attempt = 0; attempt < 14; ++attempt) {
            if (!((1L << speech_number) & gElevatorRideUsedDialogueMask))
                break;
            speech_number = (speech_number + 1) % 14;
        }
        if (attempt != 14) {
            gElevatorRideUsedDialogueMask |= 1L << speech_number;
            entity->x -= 24;
            first_line = GenerateSpeech(entity->entity_index, SpeechStringPointers[speech_number].first_line, 0);
            if (SpeechStringPointers[speech_number].second_line)
                second_line = GenerateSpeech(entity->entity_index, SpeechStringPointers[speech_number].second_line, 0);
            entity->x += 40;
            reply_first_line = GenerateSpeech(entity->entity_index, SpeechStringPointers[speech_number].reply_first_line, 0);
            if (SpeechStringPointers[speech_number].reply_second_line)
                reply_second_line = GenerateSpeech(entity->entity_index, SpeechStringPointers[speech_number].reply_second_line, 0);
            entity->x -= 16;
            UpdateAndQueueEntitySprites();
            if (gElevatorRideSpeechEntities[0] = first_line)
                gPlayerInfo[first_line].sprite_ready = 0;
            if (gElevatorRideSpeechEntities[1] = second_line)
                gPlayerInfo[second_line].sprite_ready = 0;
            if (gElevatorRideSpeechEntities[2] = reply_first_line)
                gPlayerInfo[reply_first_line].sprite_ready = 0;
            if (gElevatorRideSpeechEntities[3] = reply_second_line)
                gPlayerInfo[reply_second_line].sprite_ready = 0;
            BuildViewportSpriteLinks();
            return;
        }
    }
    gElevatorRideSpeechEntities[0] = 0;
    gElevatorRideSpeechEntities[1] = 0;
    gElevatorRideSpeechEntities[2] = 0;
    gElevatorRideSpeechEntities[3] = 0;
}

/* Reveal and hide each speech line at its fixed point in the ride. */
void ElevatorUpdateRideDialogue()
{
    short line;

    if (gFrameCounter == 60) {
        if (line = gElevatorRideSpeechEntities[0])
            gPlayerInfo[line].sprite_ready = 1;
    } else if (gFrameCounter == 120) {
        if (line = gElevatorRideSpeechEntities[0])
            gPlayerInfo[line].sprite_ready = 0;
        if (line = gElevatorRideSpeechEntities[1])
            gPlayerInfo[line].sprite_ready = 1;
        else
            gFrameCounter = 180;
    } else if (gFrameCounter == 180) {
        if (line = gElevatorRideSpeechEntities[1])
            gPlayerInfo[line].sprite_ready = 0;
    } else if (gFrameCounter == 300) {
        if (line = gElevatorRideSpeechEntities[2])
            gPlayerInfo[line].sprite_ready = 1;
    } else if (gFrameCounter == 360) {
        if (line = gElevatorRideSpeechEntities[2])
            gPlayerInfo[line].sprite_ready = 0;
        if (line = gElevatorRideSpeechEntities[3])
            gPlayerInfo[line].sprite_ready = 1;
        else
            gFrameCounter = 419;
    } else if (gFrameCounter == 420)
        ClearSpeechBubblesAndReplies();
}

/* Integrate the ride's scroll speed and run its per-blank sprite work. */
void ElevatorRideVBlank()
{
    if (gElevatorRideScrollAcceleration < 4096)
        ++gElevatorRideScrollAcceleration;
    if (gElevatorRideScrollAcceleration < 0 || gElevatorRideScrollVelocity < 2048)
        gElevatorRideScrollVelocity += gElevatorRideScrollAcceleration >> 8;
    gElevatorRideScrollYFixed -= gElevatorRideScrollVelocity;
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = gElevatorRideScrollYFixed >> 8;
    ElevatorUpdateRideDialogue();
    ElevatorAlternateRideEntities();
    SelectPrimaryViewportSprites();
    MusicUpdateSequencer();
    ++gFrameCounter;
}

/* Wait for the elevator sprites, fill the ride background, and install VBlank. */
void ElevatorBeginRideScene(elevator)
struct elevator_record *elevator;
{
    short band, row, repeat, column;
    unsigned short tile;
    struct object_state *first, *second;

    gElevatorRideEntity = &gPlayerInfo[elevator->entity];
    first = gElevatorRideEntity;
    second = first + 2;
    first->x = gCamera1PendingX + 160;
    first->y = gCamera1PendingY + 144;
    first->level = gPlayer1MapLevel;
    first->sprite_ready = 0;
    second->level = gPlayer1MapLevel;
    second->sprite_ready = 0;
    ElevatorSetState(elevator, ELEVATOR_STATE_RIDE_SCENE);
    ClearTransientEntities();
    do {
        UpdateAndQueueEntitySprites();
        while (gSpriteGraphicsUploadPending)
            ;
    } while (!first->sprite_ready || !second->sprite_ready);
    DisableBlankingGraphics();
    ElevatorPrepareRideDialogue(first);
    WaitForVBlankWithGraphicsPaused();
    for (band = 0; band < 4; ++band)
        for (row = 0; row < 8; ++row) {
            VDP_COMMAND = (((band << 3) + row) << 23) + VDP_WRITE_PLANE_A;
            for (repeat = 0; repeat < 6; ++repeat)
                for (column = 0; column < 8; ++column) {
                    tile = TransitionBackgroundTileOffsets[row][column] + 0x2001;
                    if (SpritePriorityLookup[gPlayer1MapLevel])
                        tile |= VDP_TILE_PRIORITY;
                    VDP_DATA = tile;
                }
        }
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    CopyPaletteToCRAMSlot(PaletteTitleScreen_bank1, 1);
    ElevatorLoadRideBackgroundTiles();
    HudHidePlayerSprites(2);
    gElevatorRideScrollYFixed = 0;
    VDP_COMMAND = VDP_WRITE_VSRAM_BASE;
    VDP_DATA = gElevatorRideScrollYLowWord;
    gElevatorRideScrollVelocity = 128;
    gElevatorRideScrollAcceleration = 1024;
    VDP_COMMAND = 0x7c000003L;
    VDP_DATA = 0xffff;
    gFrameCounter = 0;
    gVBlankFunc = ElevatorRideVBlank;
}

/* Wait for peak speed, decelerate, and restore the normal palette and tiles. */
void ElevatorEndRideScene()
{
    while (gElevatorRideScrollVelocity < 2048)
        ;
    gElevatorRideScrollAcceleration = -2048;
    while (gElevatorRideScrollVelocity > 0)
        ;
    gVBlankFunc = 0;
    ClearSpeechBubblesAndReplies();
    DisableBlankingGraphics();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    CopyPaletteToCRAMSlot(PaletteGameLine2, 1);
    VDP_COMMAND = VDP_WRITE_VRAM_BASE;
    MoveDataToVRAM(LevelTiles, 1040);
}

/* Advance the selected elevator's players, show the shared ride when possible,
 * then create the destination elevator and restore the game display. */
void ElevatorAdvanceRidersToNextLevel(elevator_number)
char elevator_number;
{
    char destination_level, first_rides = 0, second_rides = 0;
    short destination_y, destination_x;
    struct elevator_record *elevator;

    if (elevator_number == gPlayerElevatorFlags[PLAYER_TOEJAM]) {
        EarthlingReleaseLevelBank(gPlayerInfo[PLAYER_TOEJAM].level);
        MapReleaseLevelItemBank(gPlayerInfo[PLAYER_TOEJAM].level);
        destination_level = ++gPlayerInfo[PLAYER_TOEJAM].level;
        first_rides = 1;
    }
    if (elevator_number == gPlayer2ElevatorId) {
        EarthlingReleaseLevelBank(gPlayerInfo[PLAYER_EARL].level);
        MapReleaseLevelItemBank(gPlayerInfo[PLAYER_EARL].level);
        destination_level = ++gPlayerInfo[PLAYER_EARL].level;
        second_rides = 1;
    }
    if ((gPlayerLives[PLAYER_TOEJAM] < 0 || first_rides) && (gPlayer2Lives < 0 || second_rides)) {
        gRequestedMusicSelection = MUSIC_TRANSITION;
        while (gCurrentMusicSelection != gRequestedMusicSelection)
            ;
        elevator = &gElevatorRecords[(elevator_number - 1) >> 1][0];
        ElevatorBeginRideScene(elevator);
    }
    if (first_rides) {
        MapLoadPlayerLevel(PLAYER_TOEJAM, gPlayerInfo[PLAYER_TOEJAM].level);
        EarthlingRetainLevelBank(gPlayerInfo[PLAYER_TOEJAM].level);
        LoadLevelItemBank(gPlayerInfo[PLAYER_TOEJAM].level);
    }
    if (second_rides) {
        MapLoadPlayerLevel(PLAYER_EARL, gPlayerInfo[PLAYER_EARL].level);
        EarthlingRetainLevelBank(gPlayerInfo[PLAYER_EARL].level);
        LoadLevelItemBank(gPlayerInfo[PLAYER_EARL].level);
    }
    if (gVBlankFunc) {
        ElevatorEndRideScene();
        ElevatorSetState(elevator, ELEVATOR_STATE_ABSENT);
    }
    DisableBlankingGraphics();
    gMode2RegSoftcopy &= 0x3c;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    if (destination_level < 25)
        SetupElevator(destination_level, 0);
    elevator_number = SetupElevator(destination_level, 1);
    elevator = &gElevatorRecords[(elevator_number - 1) >> 1][1];
    destination_y = elevator->y + 5;
    destination_x = elevator->x;
    if (second_rides) {
        gPlayerInfo[PLAYER_EARL].y = destination_y;
        gPlayerInfo[PLAYER_EARL].x = destination_x;
        HudLoadLevelNumberSprites(PLAYER_EARL);
        gPlayer2ElevatorId = elevator_number;
        if (destination_level < LEVEL_FINAL)
            TelephonePlaceOutsidePlayerViews(PLAYER_EARL);
    }
    if (first_rides) {
        gPlayerInfo[PLAYER_TOEJAM].y = destination_y;
        gPlayerInfo[PLAYER_TOEJAM].x = destination_x;
        HudLoadLevelNumberSprites(PLAYER_TOEJAM);
        gPlayerElevatorFlags[PLAYER_TOEJAM] = elevator_number;
        if (destination_level < LEVEL_FINAL)
            TelephonePlaceOutsidePlayerViews(PLAYER_TOEJAM);
    }
    if (destination_level == LEVEL_FINAL)
        LoadFinalLevelMap();
    gElevatorDepartureInProgress = 0;
    UpdateScreenSplitMode(1);
    gMode2RegSoftcopy |= 0x40;
    VDP_CONTROL_WORD = VDP_MODE2_REGISTER | gMode2RegSoftcopy;
    EnableBlankingGraphics();
    if (gCurrentMusicSelection == MUSIC_TRANSITION) {
        AudioRequestFmChannelRelease();
        MusicRequestTrackPointerReset();
    }
}
