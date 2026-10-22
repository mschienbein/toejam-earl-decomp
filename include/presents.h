#ifndef PRESENTS_H
#define PRESENTS_H

/* Target layouts retain two-byte shorts and four-byte pointers. */
struct present_info {
    char appearance;                  /* 00: wrapping sprite index, 0..27 or -1; identity setup writes. */
    char identified;                  /* 01: nonzero after identification; inventory/setup write. */
};

/* Eight-byte world item, also used by both dropped-present banks. */
struct level_item {
    char kind;                        /* 00: PRESENT identity or ITEM class, -1 unused; setup/pickup write. */
    char level;                       /* 01: world level, 0..25; placement writes. */
    char object;                      /* 02: gPlayerInfo handle, 0 absent; visibility allocator writes. */
    char unused;                      /* 03: no reads/writes observed in represented code. */
    short x, y;                       /* 04/06: world pixels; placement writes. */
};

/* Four-byte ROM coordinate pair used by Level1PresentLocations. */
struct item_position {
    short x, y;                       /* 00/02: world pixels; ROM-authored. */
};

/* Eight-byte telephone record; object is an entity handle, not a pointer. */
struct telephone_state {
    char unused_kind;                 /* 00: no represented reader/writer. */
    char level;                       /* 01: world level, 0 when cleared; telephone setup writes. */
    char object;                      /* 02: gPlayerInfo handle, 0 absent; telephone setup writes. */
    char unused;                      /* 03: no represented reader/writer. */
    short x, y;                       /* 04/06: world pixels; telephone placement writes. */
};

#endif
