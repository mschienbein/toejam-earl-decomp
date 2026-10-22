#include "psg_effects.h"
#include "music_ids.h"
#include "entities.h"
#include "elevator.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "elevator_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Report input or nearby players on this elevator's level, marking nearby
 * players in gPlayerElevatorApproachInputGate before the elevator changes state. */
int ElevatorCheckPlayerProximityAndInput(elevator, flag)
struct elevator_record *elevator;
char flag;
{
    short player, distance_y;
    int occupied = 0;
    struct object_state *entity;

    for (player = 0; player <= 1; ++player) {
        entity = &gPlayerInfo[player];
        if (entity->level == elevator->level) {
            if (flag == gPlayerElevatorFlags[player]) {
                if (gPlayerProcessedInput[player] & 3)
                    occupied = 1;
            } else if ((short)abs(entity->x - elevator->x) < 10) {
                distance_y = entity->y - elevator->y;
                if (distance_y < 20 &&
                    ((elevator->state <= 1 && distance_y >= 10) ||
                     (elevator->state == 3 && distance_y > 0))) {
                    gPlayerElevatorApproachInputGate[player] = 1;
                    occupied = 1;
                }
            }
        }
    }
    return occupied;
}

/* Raise the departure entity in four-pixel steps, wrapping at 28 and
 * shortening the interval after each completed cycle. */
int ElevatorStepDepartureHeightCycles(elevator)
struct elevator_record *elevator;
{
    struct object_state *entity;

    if (--gDepartureElevatorShakeDelay <= 0) {
        entity = &gPlayerInfo[elevator->entity + 2];
        if ((entity->height += 4) > 28) {
            entity->height = 0;
            --gDepartureElevatorShakeCycles;
            PlaySound(PSG_GATED_TONE_SWEEP, 50);
        }
        gDepartureElevatorShakeDelay = gDepartureElevatorShakeCycles / 2;
    }
    if (gDepartureElevatorShakeCycles == 0)
        return 1;
    return 0;
}

/* Advance the two departure elevators and keep attached players centered.
 * State values and sounds are observed; their narrative roles are [INFERENCE]. */
void ProcessElevatorStep1()
{
    short elevator_number, player, distance_or_frame, distance_y;
    char flag, state, next_state;
    struct elevator_record *elevator;
    struct object_state *entity;

    for (elevator_number = 0; elevator_number <= 1; ++elevator_number) {
        flag = elevator_number * 2 + 1;
        elevator = &gElevatorRecords[elevator_number][0];
        state = elevator->state;
        if (state == 11)
            continue;
        next_state = -1;
        if (state == 13) {
            if (gPrimaryArrivalElevatorLevel != elevator->level + 1 &&
                gSecondaryArrivalElevatorLevel != elevator->level + 1) {
                next_state = 10;
                entity = gPlayerInfo[PLAYER_TOEJAM].level == elevator->level ?
                    &gPlayerInfo[PLAYER_TOEJAM] : &gPlayerInfo[PLAYER_EARL];
                distance_or_frame = abs(entity->x - elevator->x);
                distance_y = abs(entity->y - (elevator->y + 5));
                if (distance_or_frame < 22 && distance_y < 7)
                    entity->y = elevator->y + 12;
            }
        } else if (state == 10) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM]) {
                    next_state = 1;
                    gPlayerInfo[PLAYER_TOEJAM].behavior_flags &= ~64;
                } else if (flag == gPlayerElevatorFlags[PLAYER_EARL]) {
                    gPlayerInfo[PLAYER_EARL].behavior_flags &= ~64;
                    next_state = 1;
                } else
                    next_state = 0;
            }
        } else if (state == 2) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration)
                next_state = 3;
        } else if (state == 4) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration) {
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM] || flag == gPlayerElevatorFlags[PLAYER_EARL])
                    next_state = 1;
                else
                    next_state = 0;
            }
        } else if (state == 6) {
            if (!elevator->entity)
                continue;
            distance_or_frame = gPlayerInfo[elevator->entity + 2].frame;
            if (!gPlayerInfo[elevator->entity + 2].frame_duration) {
                next_state = 5;
                gDepartureElevatorShakeCycles = 5;
                gDepartureElevatorShakeDelay = 2;
                PlaySound(PSG_GATED_TONE_SWEEP, 50);
                if ((gPlayerLives[PLAYER_TOEJAM] < 0 || flag == gPlayerElevatorFlags[PLAYER_TOEJAM]) &&
                    (gPlayer2Lives < 0 || flag == gPlayerElevatorFlags[PLAYER_EARL])) {
                    AudioRequestFmChannelRelease();
                    MusicRequestTrackPointerReset();
                }
            } else if (distance_or_frame < 12 && !((distance_or_frame + 1) & 3))
                PlaySound(PSG_TONE_PULSE, 50);
        } else if (state == 5) {
            if (elevator->entity && ElevatorStepDepartureHeightCycles(elevator)) {
                next_state = 7;
                PlaySound(PSG_NOISE_SWEEP, 65);
            }
        } else if (state == 7) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                if (gPlayerInfo[PLAYER_TOEJAM].level == gPlayerInfo[PLAYER_EARL].level &&
                    gPlayerElevatorFlags[PLAYER_TOEJAM] != gPlayerElevatorFlags[PLAYER_EARL])
                    next_state = 10;
                else {
                    next_state = 11;
                    PlaySound(PSG_SILENCE, 98);
                }
            }
        } else if ((flag == gPlayerElevatorFlags[PLAYER_TOEJAM] && flag == gPlayerElevatorFlags[PLAYER_EARL] &&
                    !gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayer2InventoryOpen) ||
                   (flag == gPlayerElevatorFlags[PLAYER_TOEJAM] && !gPlayerInventoryOpen[PLAYER_TOEJAM] &&
                    gPlayerInfo[PLAYER_TOEJAM].level < gPlayerInfo[PLAYER_EARL].level) ||
                   (flag == gPlayerElevatorFlags[PLAYER_EARL] && !gPlayer2InventoryOpen &&
                    gPlayerInfo[PLAYER_EARL].level < gPlayerInfo[PLAYER_TOEJAM].level)) {
            if (state == 3)
                next_state = 4;
            else if (state == 1) {
                gElevatorDepartureInProgress = 1;
                next_state = 6;
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM])
                    gPlayerInfo[PLAYER_TOEJAM].behavior_flags |= ENTITY_HIDDEN;
                if (flag == gPlayerElevatorFlags[PLAYER_EARL])
                    gPlayerInfo[PLAYER_EARL].behavior_flags |= ENTITY_HIDDEN;
            }
        } else if (ElevatorCheckPlayerProximityAndInput(elevator, flag)) {
            if (state != 3 && state != 2) {
                next_state = 2;
                PlaySound(PSG_NOISE_ACCENT, 30);
            }
        } else if (state > 1 && state != 4) {
            next_state = 4;
            PlaySound(PSG_NOISE_ACCENT, 30);
        }
        for (player = 0; player <= 1; ++player) {
            if (flag == gPlayerElevatorFlags[player]) {
                entity = &gPlayerInfo[player];
                if (!entity->sprite_state && entity->frame == 1)
                    entity->frame_duration = 0;
                entity->x = elevator->x;
            }
        }
        if (next_state != -1)
            ElevatorSetState(elevator, next_state);
        if (next_state == 11)
            ElevatorAdvanceRidersToNextLevel(flag);
    }
}

/* Advance the two arrival elevators, releasing attached player flags and
 * delaying the final close state with gArrivalElevatorCloseDelay. */
void ProcessElevatorStep2()
{
    short elevator_number, player, distance_x, distance_y;
    char flag, state, next_state;
    struct elevator_record *elevator;
    struct object_state *entity;

    for (elevator_number = 0; elevator_number <= 1; ++elevator_number) {
        flag = elevator_number * 2 + 2;
        elevator = &gElevatorRecords[elevator_number][1];
        state = elevator->state;
        if (state == 11)
            continue;
        next_state = -1;
        if (state == 8) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                next_state = 1;
                entity = 0;
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM])
                    gPlayerInfo[PLAYER_TOEJAM].behavior_flags &= ~64;
                else if (gPlayerInfo[PLAYER_TOEJAM].level == elevator->level)
                    entity = &gPlayerInfo[PLAYER_TOEJAM];
                if (flag == gPlayerElevatorFlags[PLAYER_EARL])
                    gPlayerInfo[PLAYER_EARL].behavior_flags &= ~64;
                else if (gPlayerInfo[PLAYER_EARL].level == elevator->level)
                    entity = &gPlayerInfo[PLAYER_EARL];
                if (entity) {
                    distance_x = abs(entity->x - elevator->x);
                    distance_y = abs(entity->y - (elevator->y + 5));
                    if (distance_x < 22 && distance_y < 7)
                        entity->y = elevator->y + 12;
                }
                PlaySound(PSG_DUAL_TONE_FADE, 50);
                if (gCurrentMusicSelection == MUSIC_TRANSITION)
                    gRequestedMusicSelection = (int)GetLevelMusicIndex(gHighestLevel);
            }
        } else if (state == 2) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration)
                next_state = 3;
        } else if (state == 4) {
            if (!elevator->entity || !gPlayerInfo[elevator->entity + 2].frame_duration) {
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM] || flag == gPlayerElevatorFlags[PLAYER_EARL])
                    next_state = 1;
                else if (!elevator->entity)
                    next_state = 11;
                else {
                    next_state = 0;
                    gArrivalElevatorCloseDelay = 30;
                }
            }
        } else if (state == 0 && --gArrivalElevatorCloseDelay <= 0)
            next_state = 9;
        else if (state == 9) {
            if (!elevator->entity || !gPlayerInfo[elevator->entity].frame_duration)
                next_state = 11;
        } else if (ElevatorCheckPlayerProximityAndInput(elevator, flag)) {
            if (state != 3 && state != 2) {
                next_state = 2;
                PlaySound(PSG_NOISE_ACCENT, 30);
            }
        } else if (state > 1 && state != 4) {
            next_state = 4;
            PlaySound(PSG_NOISE_ACCENT, 30);
        }
        for (player = 0; player <= 1; ++player) {
            if (flag == gPlayerElevatorFlags[player]) {
                entity = &gPlayerInfo[player];
                if (!entity->sprite_state && entity->frame == 1)
                    entity->frame_duration = 0;
                entity->x = elevator->x;
            }
        }
        if (next_state != -1)
            ElevatorSetState(elevator, next_state);
    }
}

/* Run departure processing before arrival processing each update. */
void ProcessElevators()
{
    ProcessElevatorStep1();
    ProcessElevatorStep2();
}
