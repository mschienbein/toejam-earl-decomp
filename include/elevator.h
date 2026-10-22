#ifndef TJE_ELEVATOR_H
#define TJE_ELEVATOR_H

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
