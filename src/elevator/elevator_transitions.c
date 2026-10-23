#include "psg_effects.h"
#include "music_ids.h"
#include "entities.h"
#include "elevator.h"
#include "elevator_states.h"
#include "audio_calls.h"
#include "core_calls.h"
#include "elevator_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"

/* MATCHING QUALIFICATION: the (int)GetLevelMusicIndex() result view
 * retains the observed call-result width with the shared short declaration.
 * Exact historical spelling remains unknown [INFERENCE]. */

/* Report input or nearby players on this elevator's level, marking nearby
 * players in gPlayerElevatorApproachInputGate before the elevator changes state. */
int ElevatorCheckPlayerProximityAndInput(elevator, flag)
struct elevator_record *elevator;
char flag;
{
    short player, distance_y;
    int occupied = 0;
    struct object_state *entity;

    for (player = 0; player <= PLAYER_EARL; ++player) {
        entity = &gPlayerInfo[player];
        if (entity->level == elevator->level) {
            if (flag == gPlayerElevatorFlags[player]) {
                if (gPlayerProcessedInput[player] & INPUT_VERTICAL_MASK)
                    occupied = 1;
            } else if ((short)abs(entity->x - elevator->x) < 10) {
                distance_y = entity->y - elevator->y;
                if (distance_y < 20 &&
                    ((elevator->state <= ELEVATOR_STATE_CLOSED_READY && distance_y >= 10) ||
                     (elevator->state == ELEVATOR_STATE_OPEN && distance_y > 0))) {
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
            PlaySound(PSG_GATED_TONE_SWEEP, AUDIO_PRIORITY_NORMAL);
        }
        gDepartureElevatorShakeDelay = gDepartureElevatorShakeCycles / 2;
    }
    if (gDepartureElevatorShakeCycles == 0)
        return 1;
    return 0;
}

/* Advance the two departure elevators and keep attached players centered.
 * State names describe transition roles, not recovered animation artwork. */
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
        if (state == ELEVATOR_STATE_ABSENT)
            continue;
        next_state = ELEVATOR_STATE_UNCHANGED;
        if (state == ELEVATOR_STATE_BLOCKED_BY_ARRIVAL) {
            if (gPrimaryArrivalElevatorLevel != elevator->level + 1 &&
                gSecondaryArrivalElevatorLevel != elevator->level + 1) {
                next_state = ELEVATOR_STATE_REACTIVATING;
                entity = gPlayerInfo[PLAYER_TOEJAM].level == elevator->level ?
                    &gPlayerInfo[PLAYER_TOEJAM] : &gPlayerInfo[PLAYER_EARL];
                distance_or_frame = abs(entity->x - elevator->x);
                distance_y = abs(entity->y - (elevator->y + 5));
                if (distance_or_frame < 22 && distance_y < 7)
                    entity->y = elevator->y + 12;
            }
        } else if (state == ELEVATOR_STATE_REACTIVATING) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM]) {
                    next_state = ELEVATOR_STATE_CLOSED_READY;
                    gPlayerInfo[PLAYER_TOEJAM].behavior_flags &= ~64;
                } else if (flag == gPlayerElevatorFlags[PLAYER_EARL]) {
                    gPlayerInfo[PLAYER_EARL].behavior_flags &= ~64;
                    next_state = ELEVATOR_STATE_CLOSED_READY;
                } else
                    next_state = ELEVATOR_STATE_CLOSED_EMPTY;
            }
        } else if (state == ELEVATOR_STATE_OPENING) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration)
                next_state = ELEVATOR_STATE_OPEN;
        } else if (state == ELEVATOR_STATE_CLOSING) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration) {
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM] || flag == gPlayerElevatorFlags[PLAYER_EARL])
                    next_state = ELEVATOR_STATE_CLOSED_READY;
                else
                    next_state = ELEVATOR_STATE_CLOSED_EMPTY;
            }
        } else if (state == ELEVATOR_STATE_DEPARTURE_START) {
            if (!elevator->entity)
                continue;
            distance_or_frame = gPlayerInfo[elevator->entity + 2].frame;
            if (!gPlayerInfo[elevator->entity + 2].frame_duration) {
                next_state = ELEVATOR_STATE_DEPARTURE_CYCLES;
                gDepartureElevatorShakeCycles = 5;
                gDepartureElevatorShakeDelay = 2;
                PlaySound(PSG_GATED_TONE_SWEEP, AUDIO_PRIORITY_NORMAL);
                if ((gPlayerLives[PLAYER_TOEJAM] < 0 || flag == gPlayerElevatorFlags[PLAYER_TOEJAM]) &&
                    (gPlayer2Lives < 0 || flag == gPlayerElevatorFlags[PLAYER_EARL])) {
                    AudioRequestFmChannelRelease();
                    MusicRequestTrackPointerReset();
                }
            } else if (distance_or_frame < 12 && !((distance_or_frame + 1) & 3))
                PlaySound(PSG_TONE_PULSE, AUDIO_PRIORITY_NORMAL);
        } else if (state == ELEVATOR_STATE_DEPARTURE_CYCLES) {
            if (elevator->entity && ElevatorStepDepartureHeightCycles(elevator)) {
                next_state = ELEVATOR_STATE_DEPARTURE_FINISH;
                PlaySound(PSG_NOISE_SWEEP, 65);
            }
        } else if (state == ELEVATOR_STATE_DEPARTURE_FINISH) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                if (gPlayerInfo[PLAYER_TOEJAM].level == gPlayerInfo[PLAYER_EARL].level &&
                    gPlayerElevatorFlags[PLAYER_TOEJAM] != gPlayerElevatorFlags[PLAYER_EARL])
                    next_state = ELEVATOR_STATE_REACTIVATING;
                else {
                    next_state = ELEVATOR_STATE_ABSENT;
                    PlaySound(PSG_SILENCE, 98);
                }
            }
        } else if ((flag == gPlayerElevatorFlags[PLAYER_TOEJAM] && flag == gPlayerElevatorFlags[PLAYER_EARL] &&
                    !gPlayerInventoryOpen[PLAYER_TOEJAM] && !gPlayer2InventoryOpen) ||
                   (flag == gPlayerElevatorFlags[PLAYER_TOEJAM] && !gPlayerInventoryOpen[PLAYER_TOEJAM] &&
                    gPlayerInfo[PLAYER_TOEJAM].level < gPlayerInfo[PLAYER_EARL].level) ||
                   (flag == gPlayerElevatorFlags[PLAYER_EARL] && !gPlayer2InventoryOpen &&
                    gPlayerInfo[PLAYER_EARL].level < gPlayerInfo[PLAYER_TOEJAM].level)) {
            if (state == ELEVATOR_STATE_OPEN)
                next_state = ELEVATOR_STATE_CLOSING;
            else if (state == ELEVATOR_STATE_CLOSED_READY) {
                gElevatorDepartureInProgress = 1;
                next_state = ELEVATOR_STATE_DEPARTURE_START;
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM])
                    gPlayerInfo[PLAYER_TOEJAM].behavior_flags |= ENTITY_HIDDEN;
                if (flag == gPlayerElevatorFlags[PLAYER_EARL])
                    gPlayerInfo[PLAYER_EARL].behavior_flags |= ENTITY_HIDDEN;
            }
        } else if (ElevatorCheckPlayerProximityAndInput(elevator, flag)) {
            if (state != ELEVATOR_STATE_OPEN && state != ELEVATOR_STATE_OPENING) {
                next_state = ELEVATOR_STATE_OPENING;
                PlaySound(PSG_NOISE_ACCENT, 30);
            }
        } else if (state > ELEVATOR_STATE_CLOSED_READY && state != ELEVATOR_STATE_CLOSING) {
            next_state = ELEVATOR_STATE_CLOSING;
            PlaySound(PSG_NOISE_ACCENT, 30);
        }
        for (player = 0; player <= PLAYER_EARL; ++player) {
            if (flag == gPlayerElevatorFlags[player]) {
                entity = &gPlayerInfo[player];
                if (!entity->sprite_state && entity->frame == 1)
                    entity->frame_duration = 0;
                entity->x = elevator->x;
            }
        }
        if (next_state != ELEVATOR_STATE_UNCHANGED)
            ElevatorSetState(elevator, next_state);
        if (next_state == ELEVATOR_STATE_ABSENT)
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
        if (state == ELEVATOR_STATE_ABSENT)
            continue;
        next_state = ELEVATOR_STATE_UNCHANGED;
        if (state == ELEVATOR_STATE_ARRIVAL_START) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                next_state = ELEVATOR_STATE_CLOSED_READY;
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
                PlaySound(PSG_DUAL_TONE_FADE, AUDIO_PRIORITY_NORMAL);
                if (gCurrentMusicSelection == MUSIC_TRANSITION)
                    gRequestedMusicSelection = (int)GetLevelMusicIndex(gHighestLevel);
            }
        } else if (state == ELEVATOR_STATE_OPENING) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration)
                next_state = ELEVATOR_STATE_OPEN;
        } else if (state == ELEVATOR_STATE_CLOSING) {
            if (!elevator->entity || !gPlayerInfo[elevator->entity + 2].frame_duration) {
                if (flag == gPlayerElevatorFlags[PLAYER_TOEJAM] || flag == gPlayerElevatorFlags[PLAYER_EARL])
                    next_state = ELEVATOR_STATE_CLOSED_READY;
                else if (!elevator->entity)
                    next_state = ELEVATOR_STATE_ABSENT;
                else {
                    next_state = ELEVATOR_STATE_CLOSED_EMPTY;
                    gArrivalElevatorCloseDelay = 30;
                }
            }
        } else if (state == ELEVATOR_STATE_CLOSED_EMPTY && --gArrivalElevatorCloseDelay <= 0)
            next_state = ELEVATOR_STATE_ARRIVAL_FINISH;
        else if (state == ELEVATOR_STATE_ARRIVAL_FINISH) {
            if (!elevator->entity || !gPlayerInfo[elevator->entity].frame_duration)
                next_state = ELEVATOR_STATE_ABSENT;
        } else if (ElevatorCheckPlayerProximityAndInput(elevator, flag)) {
            if (state != ELEVATOR_STATE_OPEN && state != ELEVATOR_STATE_OPENING) {
                next_state = ELEVATOR_STATE_OPENING;
                PlaySound(PSG_NOISE_ACCENT, 30);
            }
        } else if (state > ELEVATOR_STATE_CLOSED_READY && state != ELEVATOR_STATE_CLOSING) {
            next_state = ELEVATOR_STATE_CLOSING;
            PlaySound(PSG_NOISE_ACCENT, 30);
        }
        for (player = 0; player <= PLAYER_EARL; ++player) {
            if (flag == gPlayerElevatorFlags[player]) {
                entity = &gPlayerInfo[player];
                if (!entity->sprite_state && entity->frame == 1)
                    entity->frame_duration = 0;
                entity->x = elevator->x;
            }
        }
        if (next_state != ELEVATOR_STATE_UNCHANGED)
            ElevatorSetState(elevator, next_state);
    }
}

/* Run departure processing before arrival processing each update. */
void ProcessElevators()
{
    ProcessElevatorStep1();
    ProcessElevatorStep2();
}
