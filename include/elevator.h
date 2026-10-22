#ifndef ELEVATOR_H
#define ELEVATOR_H

/* Eight-byte records occur in primary/secondary pairs. The entity byte is
 * a slot in gPlayerInfo; its linked objects use struct object_state. */
struct elevator_record {
    short x, y;
    char level, state, entity, timer;
};

/* Two-byte map-cell coordinate pair, not a world-position short pair. */
struct elevator_position {
    char x, y;
};

#endif
