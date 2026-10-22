#ifndef PRESENTS_H
#define PRESENTS_H

/* Target layouts use two-byte shorts and four-byte pointers. */

/* Two bytes per identity; byte 01 is read and written as identified. */
struct present_info {
    char sprite, identified;
};

/* Eight-byte world-item record, including the dropped-present banks. */
struct level_item {
    char kind, level, object, reserved_03;
    short x, y;
};

/* Four-byte ROM coordinate pair used by Level1PresentLocations. */
struct item_position {
    short x, y;
};

/* Eight-byte telephone record; object is a gPlayerInfo slot, not a pointer. */
struct telephone_state {
    char reserved_00, level, object, reserved_03;
    short x, y;
};

#endif
