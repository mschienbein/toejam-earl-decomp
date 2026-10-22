#ifndef EARTHLINGS_H
#define EARTHLINGS_H

struct timed_animation;

/* MCC68K target layout: 18 bytes; twenty records occupy one 360-byte bank. */
struct earthling_state {
    char kind;                       /* 00: -1 marks an unused record */
    char object;                     /* 01: primary object handle */
    char level;                      /* 02 */
    char state;                      /* 03 */
    short timer;                     /* 04 */
    short x, y;                      /* 06, 08 */
    /* Saved coordinates for some kinds; terrain or one-shot values for others. */
    short value_0a, value_0c;         /* 0a, 0c */
    char state_delay;                /* 0e */
    char second_object;              /* 0f */
    /* [0]: health or animation phase timer; [1]: camera visibility bits 2/4.
     * Keep this neutral array: scalarizing it changes LoadInEarthlings code. */
    char reserved_10[2];             /* 10, 11 */
};

/* MCC68K target layout: 38 bytes. EARTHLING_DATA has 25 logical records;
 * record 24's reserved_25 is also EarthlingCoordinatePairs[0]. */
struct earthling_definition {
    struct timed_animation *states;  /* 00 */
    void (*update)();                /* 04: installed as the object's process */
    void (*ability)();               /* 08: called with the earthling record */
    void (*touch)();                 /* 0c: called with player number, record */
    char movement_mode;              /* 10: copied to object movement mode */
    unsigned char width;             /* 11 */
    unsigned char height_pixels;     /* 12 */
    char draw_offset;                /* 13 */
    char draw_attribute;             /* 14 */
    char target_chance;              /* 15 */
    unsigned char target_distance;   /* 16 */
    char turn_chance;                /* 17 */
    unsigned char flags_18;          /* 18: shared movement/state flags */
    unsigned char radius;            /* 19: spawn clearance in four-pixel units */
    unsigned char contact_distance_x;/* 1a */
    unsigned char contact_distance_y;/* 1b */
    char damage;                     /* 1c */
    unsigned char target_time_min;   /* 1d */
    unsigned char target_time_max;   /* 1e */
    unsigned char turn_time_min;     /* 1f */
    unsigned char turn_time_max;     /* 20 */
    unsigned char move_time_min;     /* 21 */
    unsigned char move_time_max;     /* 22 */
    char state_delay;                /* 23 */
    char health;                     /* 24 */
    char reserved_25;                /* 25 */
};

#endif
