#ifndef EARTHLINGS_H
#define EARTHLINGS_H

struct timed_animation;

/* MCC68K target layout: 18 bytes; twenty records occupy one 360-byte bank. */
struct earthling_state {
    char kind;                       /* 00: EARTHLING ID or -1 unused; bank setup writes. */
    char object;                     /* 01: primary gPlayerInfo handle, 0 absent; allocator writes. */
    char level;                      /* 02: world level, 0..25; bank setup writes. */
    char state;                      /* 03: kind-specific animation/AI state; SetEarthlingState writes. */
    short timer;                     /* 04: AI countdown ticks, sometimes reused; kind callback writes. */
    short x, y;                      /* 06/08: world pixels; bank setup and motion write. */
    short saved_x_or_status;         /* 0a: spawn X, terrain class or one-shot latch; kind owner writes. */
    short saved_y;                   /* 0c: saved spawn Y, zeroed for some kinds; setup/relocation writes. */
    char state_delay;                /* 0e: AI state-change delay ticks; kind callback writes. */
    char second_object;              /* 0f: secondary entity handle, 0 absent; allocator writes. */
    /* Keep the two-byte array: separate scalar members alter LoadInEarthlings. */
    char health_visibility[2];       /* 10/11: health/phase countdown and viewport bits 2/4; AI/loader write. */
};

/* MCC68K target layout: 38 bytes; immutable EARTHLING_DATA ROM records.
 * Record 24's unused byte also begins EarthlingCoordinatePairs. */
struct earthling_definition {
    struct timed_animation *states;  /* 00: kind-specific animation table; ROM-authored. */
    void (*update)();                /* 04: installed frame-motion callback; ROM-authored. */
    void (*ability)();               /* 08: AI callback taking an earthling record; ROM-authored. */
    void (*touch)();                 /* 0c: contact callback taking player and record; ROM-authored. */
    char movement_mode;              /* 10: ANIMATION_MOTION axis bits copied to entity; ROM-authored. */
    unsigned char width;             /* 11: entity width in world pixels; ROM-authored. */
    unsigned char height_pixels;     /* 12: entity height in pixels, not elevation; ROM-authored. */
    char tile_count;                 /* 13: reserved 8x8 tile count; ROM-authored. */
    char sprite_count;               /* 14: reserved hardware-piece count; ROM-authored. */
    char target_chance;              /* 15: random target-selection threshold out of 100; ROM-authored. */
    unsigned char target_distance;   /* 16: targeting range before caller scaling; ROM-authored. */
    char turn_chance;                /* 17: random turn-selection threshold out of 100; ROM-authored. */
    unsigned char behavior_flags;    /* 18: secondary-object/contact and turning policy bits; ROM-authored. */
    unsigned char radius;            /* 19: spawn clearance in four-pixel units; ROM-authored. */
    unsigned char contact_distance_x;/* 1a: horizontal contact range in pixels; ROM-authored. */
    unsigned char contact_distance_y;/* 1b: vertical contact range in world pixels; ROM-authored. */
    char damage;                     /* 1c: health units removed on hurt; ROM-authored. */
    unsigned char target_time_min;   /* 1d: minimum target-state ticks; ROM-authored. */
    unsigned char target_time_max;   /* 1e: maximum target-state ticks; ROM-authored. */
    unsigned char turn_time_min;     /* 1f: minimum turn-state ticks; ROM-authored. */
    unsigned char turn_time_max;     /* 20: maximum turn-state ticks; ROM-authored. */
    unsigned char move_time_min;     /* 21: minimum movement-state ticks; ROM-authored. */
    unsigned char move_time_max;     /* 22: maximum movement-state ticks; ROM-authored. */
    char state_delay;                /* 23: initial state-change delay ticks; ROM-authored. */
    char health;                     /* 24: initial health/phase byte; ROM-authored. */
    char unused;                     /* 25: no represented consumer; final record overlaps next asset. */
};

/* Six-byte special-earthling state shared by mailbox contact and predicates. */
struct special_earthling_state {
    char mode, level;                /* 00/01: bank mode and level; mailbox bank configuration writes. */
    short timer;                     /* 02: mailbox interaction countdown ticks; touch handler writes. */
    char contact[2];                 /* 04/05: per-player contact latch; mailbox touch handler writes. */
};

#endif
