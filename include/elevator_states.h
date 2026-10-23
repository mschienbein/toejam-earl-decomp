#ifndef TJE_ELEVATOR_STATES_H
#define TJE_ELEVATOR_STATES_H

/* Byte-state protocol shared by the record, both animation tables and callers.
 * Names describe observed transition roles, not recovered animation artwork. */
#define ELEVATOR_STATE_CLOSED_EMPTY 0
#define ELEVATOR_STATE_CLOSED_READY 1
#define ELEVATOR_STATE_OPENING 2
#define ELEVATOR_STATE_OPEN 3
#define ELEVATOR_STATE_CLOSING 4
#define ELEVATOR_STATE_DEPARTURE_CYCLES 5
#define ELEVATOR_STATE_DEPARTURE_START 6
#define ELEVATOR_STATE_DEPARTURE_FINISH 7
#define ELEVATOR_STATE_ARRIVAL_START 8
#define ELEVATOR_STATE_ARRIVAL_FINISH 9
#define ELEVATOR_STATE_REACTIVATING 10
#define ELEVATOR_STATE_ABSENT 11
#define ELEVATOR_STATE_RIDE_SCENE 12
#define ELEVATOR_STATE_BLOCKED_BY_ARRIVAL 13
/* Scheduler sentinel only; never installed as a record's animation state. */
#define ELEVATOR_STATE_UNCHANGED -1

#endif
