#ifndef ENTITIES_H
#define ENTITIES_H

struct earthling_state;
struct render_frame;

/* Shared entity storage: 128 bytes with MCC68K's two-byte record alignment.
 * Offsets below use 16-bit short and 32-bit long, data and function pointers.
 * The height word is world elevation; height_pixels is the sprite extent.
 * Byte 0x56 also holds an effect owner, and 0x57/0x58 also hold unsigned
 * attachment offsets. Those consumers must retain their observed byte view.
 */
struct object_state {
    short x;                              /* 0x00 */
    short y;                              /* 0x02 */
    short height;                         /* 0x04 */
    short velocity_x;                     /* 0x06 */
    short velocity_y;                     /* 0x08 */
    short velocity_height;                /* 0x0a */
    short acceleration_x;                 /* 0x0c */
    short acceleration_y;                 /* 0x0e */
    short acceleration_height;            /* 0x10 */
    unsigned char fraction_x;             /* 0x12 */
    unsigned char fraction_y;             /* 0x13 */
    unsigned char fraction_height;        /* 0x14 */
    char mode_15;                         /* 0x15 */
    void (*process)();                    /* 0x16 */
    char *sequence;                       /* 0x1a */
    struct render_frame **frames;         /* 0x1e */
    struct render_frame *current_frame;   /* 0x22 */
    char frame;                           /* 0x26 */
    char frame_phase;                     /* 0x27 */
    char frame_duration;                  /* 0x28 */
    unsigned char width;                  /* 0x29 */
    unsigned char height_pixels;          /* 0x2a */
    unsigned char tile_count;             /* 0x2b */
    unsigned short tile;                  /* 0x2c */
    unsigned char sprite_count;           /* 0x2e */
    unsigned char draw_flags;             /* 0x2f */
    char primary_sprites[12];             /* 0x30 */
    char secondary_sprites[12];           /* 0x3c */
    char primary_shadow;                  /* 0x48 */
    char secondary_shadow;                /* 0x49 */
    char sprite_ready;                    /* 0x4a */
    char sprite_state;                    /* 0x4b */
    char level;                           /* 0x4c */
    unsigned char flags_4d;               /* 0x4d */
    struct earthling_state *earthling;    /* 0x4e */
    unsigned long flags_52;               /* 0x52 */
    char parent;                          /* 0x56 */
    char flag_57;                         /* 0x57 */
    char flag_58;                         /* 0x58 */
    unsigned char offset_height;          /* 0x59 */
    struct object_state *next;            /* 0x5a */
    char entity_number;                   /* 0x5e */
    char reserved_5f[33];                 /* 0x5f..0x7f */
};

#endif
