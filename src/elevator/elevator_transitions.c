#define TJE_LOCAL_FUNC_001262

#include "entities.h"
#include "elevator.h"
#include "audio_calls.h"
#include "elevator_calls.h"
#include "ram_elevator.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "random.h"

/* ORIGINAL BUG: this consumer keeps a int result view of func_001262,
 * whose canonical return is short. The observed caller omits that
 * narrowing; the historical missing declaration is [INFERENCE]. */
extern int func_001262();
/* Report input or nearby players on this elevator's level, marking nearby
 * players in ram_ffda82 before the elevator changes state. */
int func_013a3e(elevator, flag)
struct elevator_record *elevator;
char flag;
{
    short player, distance_y;
    int occupied = 0;
    struct object_state *entity;

    for (player = 0; player <= 1; ++player) {
        entity = &gPlayerInfo[player];
        if (entity->level == elevator->level) {
            if (flag == ELEVATOR_FLAG[player]) {
                if (P1_PROCESSED_INPUT[player] & 3)
                    occupied = 1;
            } else if ((short)abs(entity->x - elevator->x) < 10) {
                distance_y = entity->y - elevator->y;
                if (distance_y < 20 &&
                    ((elevator->state <= 1 && distance_y >= 10) ||
                     (elevator->state == 3 && distance_y > 0))) {
                    ram_ffda82[player] = 1;
                    occupied = 1;
                }
            }
        }
    }
    return occupied;
}

/* Raise the departure entity in four-pixel steps, wrapping at 28 and
 * shortening the interval after each completed cycle. */
int func_013af8(elevator)
struct elevator_record *elevator;
{
    struct object_state *entity;

    if (--DEPARTING_ELEV_TIME <= 0) {
        entity = &gPlayerInfo[elevator->entity + 2];
        if ((entity->height += 4) > 28) {
            entity->height = 0;
            --DEPARTING_ELEV_CYCLES;
            PlaySound(17, 50);
        }
        DEPARTING_ELEV_TIME = DEPARTING_ELEV_CYCLES / 2;
    }
    if (DEPARTING_ELEV_CYCLES == 0)
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
        elevator = &ELEVATOR_LOCATION[elevator_number][0];
        state = elevator->state;
        if (state == 11)
            continue;
        next_state = -1;
        if (state == 13) {
            if (ARRIVING_ELEV_LEVEL != elevator->level + 1 &&
                ram_ffda66 != elevator->level + 1) {
                next_state = 10;
                entity = gPlayerInfo[0].level == elevator->level ?
                    &gPlayerInfo[0] : &gPlayerInfo[1];
                distance_or_frame = abs(entity->x - elevator->x);
                distance_y = abs(entity->y - (elevator->y + 5));
                if (distance_or_frame < 22 && distance_y < 7)
                    entity->y = elevator->y + 12;
            }
        } else if (state == 10) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                if (flag == ELEVATOR_FLAG[0]) {
                    next_state = 1;
                    gPlayerInfo[0].flags_4d &= ~64;
                } else if (flag == ELEVATOR_FLAG[1]) {
                    gPlayerInfo[1].flags_4d &= ~64;
                    next_state = 1;
                } else
                    next_state = 0;
            }
        } else if (state == 2) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration)
                next_state = 3;
        } else if (state == 4) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration) {
                if (flag == ELEVATOR_FLAG[0] || flag == ELEVATOR_FLAG[1])
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
                DEPARTING_ELEV_CYCLES = 5;
                DEPARTING_ELEV_TIME = 2;
                PlaySound(17, 50);
                if ((gPlayerLives[0] < 0 || flag == ELEVATOR_FLAG[0]) &&
                    (gPlayer2Lives < 0 || flag == ELEVATOR_FLAG[1])) {
                    func_03e43c();
                    func_03e40e();
                }
            } else if (distance_or_frame < 12 && !((distance_or_frame + 1) & 3))
                PlaySound(7, 50);
        } else if (state == 5) {
            if (elevator->entity && func_013af8(elevator)) {
                next_state = 7;
                PlaySound(3, 65);
            }
        } else if (state == 7) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                if (gPlayerInfo[0].level == gPlayerInfo[1].level &&
                    ELEVATOR_FLAG[0] != ELEVATOR_FLAG[1])
                    next_state = 10;
                else {
                    next_state = 11;
                    PlaySound(0, 98);
                }
            }
        } else if ((flag == ELEVATOR_FLAG[0] && flag == ELEVATOR_FLAG[1] &&
                    !P1_MENU_OPEN[0] && !HBLANK_FLAG_1) ||
                   (flag == ELEVATOR_FLAG[0] && !P1_MENU_OPEN[0] &&
                    gPlayerInfo[0].level < gPlayerInfo[1].level) ||
                   (flag == ELEVATOR_FLAG[1] && !HBLANK_FLAG_1 &&
                    gPlayerInfo[1].level < gPlayerInfo[0].level)) {
            if (state == 3)
                next_state = 4;
            else if (state == 1) {
                ram_ffda6c = 1;
                next_state = 6;
                if (flag == ELEVATOR_FLAG[0])
                    gPlayerInfo[0].flags_4d |= 64;
                if (flag == ELEVATOR_FLAG[1])
                    gPlayerInfo[1].flags_4d |= 64;
            }
        } else if (func_013a3e(elevator, flag)) {
            if (state != 3 && state != 2) {
                next_state = 2;
                PlaySound(24, 30);
            }
        } else if (state > 1 && state != 4) {
            next_state = 4;
            PlaySound(24, 30);
        }
        for (player = 0; player <= 1; ++player) {
            if (flag == ELEVATOR_FLAG[player]) {
                entity = &gPlayerInfo[player];
                if (!entity->sprite_state && entity->frame == 1)
                    entity->frame_duration = 0;
                entity->x = elevator->x;
            }
        }
        if (next_state != -1)
            func_012694(elevator, next_state);
        if (next_state == 11)
            func_013786(flag);
    }
}

/* Advance the two arrival elevators, releasing attached player flags and
 * delaying the final close state with ARRIVING_ELEV_TIME. */
void ProcessElevatorStep2()
{
    short elevator_number, player, distance_x, distance_y;
    char flag, state, next_state;
    struct elevator_record *elevator;
    struct object_state *entity;

    for (elevator_number = 0; elevator_number <= 1; ++elevator_number) {
        flag = elevator_number * 2 + 2;
        elevator = &ELEVATOR_LOCATION[elevator_number][1];
        state = elevator->state;
        if (state == 11)
            continue;
        next_state = -1;
        if (state == 8) {
            if (elevator->entity && !gPlayerInfo[elevator->entity].frame_duration) {
                next_state = 1;
                entity = 0;
                if (flag == ELEVATOR_FLAG[0])
                    gPlayerInfo[0].flags_4d &= ~64;
                else if (gPlayerInfo[0].level == elevator->level)
                    entity = &gPlayerInfo[0];
                if (flag == ELEVATOR_FLAG[1])
                    gPlayerInfo[1].flags_4d &= ~64;
                else if (gPlayerInfo[1].level == elevator->level)
                    entity = &gPlayerInfo[1];
                if (entity) {
                    distance_x = abs(entity->x - elevator->x);
                    distance_y = abs(entity->y - (elevator->y + 5));
                    if (distance_x < 22 && distance_y < 7)
                        entity->y = elevator->y + 12;
                }
                PlaySound(15, 50);
                if (VBLANK_FLAG_8 == 6)
                    VBLANK_FLAG_7 = func_001262(gHighestLevel);
            }
        } else if (state == 2) {
            if (elevator->entity && !gPlayerInfo[elevator->entity + 2].frame_duration)
                next_state = 3;
        } else if (state == 4) {
            if (!elevator->entity || !gPlayerInfo[elevator->entity + 2].frame_duration) {
                if (flag == ELEVATOR_FLAG[0] || flag == ELEVATOR_FLAG[1])
                    next_state = 1;
                else if (!elevator->entity)
                    next_state = 11;
                else {
                    next_state = 0;
                    ARRIVING_ELEV_TIME = 30;
                }
            }
        } else if (state == 0 && --ARRIVING_ELEV_TIME <= 0)
            next_state = 9;
        else if (state == 9) {
            if (!elevator->entity || !gPlayerInfo[elevator->entity].frame_duration)
                next_state = 11;
        } else if (func_013a3e(elevator, flag)) {
            if (state != 3 && state != 2) {
                next_state = 2;
                PlaySound(24, 30);
            }
        } else if (state > 1 && state != 4) {
            next_state = 4;
            PlaySound(24, 30);
        }
        for (player = 0; player <= 1; ++player) {
            if (flag == ELEVATOR_FLAG[player]) {
                entity = &gPlayerInfo[player];
                if (!entity->sprite_state && entity->frame == 1)
                    entity->frame_duration = 0;
                entity->x = elevator->x;
            }
        }
        if (next_state != -1)
            func_012694(elevator, next_state);
    }
}

/* Run departure processing before arrival processing each update. */
void ProcessElevators()
{
    ProcessElevatorStep1();
    ProcessElevatorStep2();
}
