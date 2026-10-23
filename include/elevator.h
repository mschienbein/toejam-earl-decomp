#ifndef TJE_ELEVATOR_H
#define TJE_ELEVATOR_H

#define ELEVATOR_TYPE_DEPARTURE 0 /* SetupElevator initializes this type closed; ProcessElevatorStep1 advances it. */
#define ELEVATOR_TYPE_ARRIVAL 1 /* SetupElevator initializes this type arriving; ProcessElevatorStep2 advances it. */
#define ELEVATOR_BANK_PRIMARY 0 /* gPrimaryArrivalElevatorLevel aliases this gElevatorRecords bank. */
#define ELEVATOR_BANK_SECONDARY 1 /* gSecondaryArrivalElevatorLevel aliases this gElevatorRecords bank. */
#define ELEVATOR_HANDLE_PRIMARY_DEPARTURE 1 /* SetupElevator adds this one-based bias to a primary record's type. */
#define ELEVATOR_HANDLE_PRIMARY_ARRIVAL 2 /* ProcessElevatorStep2 adds this base to its two-record bank stride. */
#define ELEVATOR_HANDLE_SECONDARY_DEPARTURE 3 /* SetupElevator adds this bias to an alternate record's type. */
#define ELEVATOR_HANDLE_TYPE_MASK 1 /* ProcessElevator masks the zero-based handle to obtain its type. */
#define ELEVATOR_ENTITY_PRIMARY_A 2 /* ElevatorCreateEntityPair first tries this reserved primary entity slot. */
#define ELEVATOR_ENTITY_PRIMARY_B 3 /* ElevatorCreateEntityPair falls back to this reserved primary entity slot. */
#define ELEVATOR_ENTITY_SECONDARY_OFFSET 2 /* ElevatorSetState and pair creation address the secondary half at primary + 2. */

/* Eight-byte records occur in primary/secondary pairs. The entity byte is
 * a slot in gPlayerInfo; its linked objects use struct object_state. */
struct elevator_record {
    short x, y;                      /* 00/02: world pixels; elevator setup writes. */
    char level, state, entity, timer; /* 04..07: level, animation state, entity handle and tick counter; elevator logic writes. */
};

/* Two-byte map-cell coordinate pair, not a world-position short pair. */
struct elevator_position {
    char x, y;                       /* 00/01: metatile-cell coordinates; level setup writes. */
};

#endif
