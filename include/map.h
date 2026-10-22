#ifndef MAP_H
#define MAP_H

/* Complete 196-byte lookup bank at REV 02 08bf4a. The final three tables
 * are also addressed by region_walk, level_item_setup and projectile_effects. */
struct map_generation_tables {
    unsigned char tiles[16];                         /* 00: shape-to-metatile IDs; ROM-authored. */
    unsigned char edge_plain[20], edge_alternate[20]; /* 10/24: boundary shape lookups; ROM-authored. */
    unsigned char filled_plain[32], filled_alternate[32]; /* 38/58: filled shape lookups; ROM-authored. */
    unsigned char direction_choices[12];             /* 78: path turn choices; ROM-authored. */
    char present_terrain[23];                        /* 84: excluded large-item metatile IDs; ROM-authored. */
    unsigned char unused;                            /* 9b: no represented consumer; ROM-authored. */
    unsigned char input_directions[4][10];            /* 9c: control-remap directional masks; ROM-authored. */
};

/* Ten bytes with two-byte target alignment: pointer fields are at 02/06.
 * shapes points to a count byte followed by that many shape bytes. */
struct map_transition_descriptor {
    unsigned char bank, flags;                       /* 00/01: shape-bank selector and chain flags; ROM-authored. */
    unsigned char *shapes, *alternatives;             /* 02/06: counted shapes and replacement IDs; ROM-authored. */
};

#endif
