#ifndef TJE_ENTITIES_H
#define TJE_ENTITIES_H

struct earthling_state;
struct render_frame;

/* Record indices and animation-facing order are shared by all object users. */
#define PLAYER_TOEJAM 0
#define PLAYER_EARL 1
#define PLAYER_COUNT 2
#define DIRECTION_UP 0
#define DIRECTION_DOWN 1
#define DIRECTION_LEFT 2
#define DIRECTION_RIGHT 3
#define DIRECTION_MASK 3

/* A set bit delegates that axis to the animation's motion callback. */
#define ANIMATION_MOTION_NONE 0
#define ANIMATION_MOTION_X 1
#define ANIMATION_MOTION_Y 2
#define ANIMATION_MOTION_XY 3

/* draw_flags: allocation/viewport state plus movement and render policy. */
#define ENTITY_ACTIVE 0x01
#define ENTITY_PRIMARY_VIEW 0x02
#define ENTITY_SECONDARY_VIEW 0x04
#define ENTITY_EITHER_VIEW 0x06
#define ENTITY_SKIP_PHYSICS 0x08
#define ENTITY_BLOCKED_X 0x10
#define ENTITY_BLOCKED_Y_OR_HEIGHT 0x20
#define ENTITY_SHADOW 0x40
#define ENTITY_DRAW_LATE 0x80

/* behavior_flags: collision, shared graphics, attachment and pixel policy. */
#define ENTITY_COLLIDABLE 0x01
#define ENTITY_SHARED_TILES 0x04
#define ENTITY_ATTACHED 0x08
#define ENTITY_PLAYER_SUPPRESSED 0x10
#define ENTITY_MASK_PIXELS 0x20
#define ENTITY_HIDDEN 0x40
#define ENTITY_LAST_LIFE_EXIT 0x80

/* Shared 128-byte storage, using MCC68K's two-byte record alignment.
 * Declared widths and signedness are part of the ROM interface, not enums.
 * Auxiliary bytes are deliberately not given a single type-specific meaning.
 */
struct object_state {
    short x;                              /* 00: world pixels; setup and movement write. */
    short y;                              /* 02: world pixels; setup and movement write. */
    short height;                         /* 04: elevation pixels, negative when sunk; motion writes. */
    short velocity_x;                     /* 06: 1/256 pixel per tick/frame-motion unit; movement writes. */
    short velocity_y;                     /* 08: 1/256 pixel per tick/frame-motion unit; movement writes. */
    short velocity_height;                /* 0a: 1/256 elevation pixel per tick; jump/fall writes. */
    short acceleration_x;                 /* 0c: velocity delta per physics tick; movement writes. */
    short acceleration_y;                 /* 0e: velocity delta per physics tick; movement writes. */
    short acceleration_height;            /* 10: vertical velocity delta per tick; jump/fall writes. */
    unsigned char fraction_x;             /* 12: fractional world X, 0..255; fixed-point motion writes. */
    unsigned char fraction_y;             /* 13: fractional world Y, 0..255; fixed-point motion writes. */
    unsigned char fraction_height;        /* 14: fractional elevation, 0..255; physics writes. */
    char animation_motion_axes;           /* 15: ANIMATION_MOTION bits; player/earthling setup writes. */
    void (*process)();                    /* 16: frame-motion callback; entity-specific setup writes. */
    char *sequence;                       /* 1a: animation bytecode; state selection/setup writes. */
    struct render_frame **frames;         /* 1e: frame-pointer table; state selection/setup writes. */
    struct render_frame *current_frame;   /* 22: decoded metasprite; animation preparation writes. */
    char frame;                           /* 26: sequence index, -1 before first frame; animation writes. */
    char frame_phase;                     /* 27: ticks per frame, 0 for motion-controlled timing; setup writes. */
    char frame_duration;                  /* 28: frame countdown, 0 when finished; animation writes. */
    unsigned char width;                  /* 29: world/sprite bounds in pixels; entity setup writes. */
    unsigned char height_pixels;          /* 2a: vertical extent in pixels, not elevation; setup writes. */
    unsigned char tile_count;             /* 2b: reserved 8x8 tiles; graphics setup writes. */
    unsigned short tile;                  /* 2c: VDP tile index/priority/flip attributes; renderer writes. */
    unsigned char sprite_count;           /* 2e: reserved hardware pieces, at most 12; setup writes. */
    unsigned char draw_flags;             /* 2f: ENTITY draw/allocation bits; renderer/physics write. */
    char primary_sprites[12];             /* 30: first viewport's sprite handles; allocator writes. */
    char secondary_sprites[12];           /* 3c: second viewport's sprite handles; allocator writes. */
    char primary_shadow;                  /* 48: first viewport shadow handle, 0 absent; renderer writes. */
    char secondary_shadow;                /* 49: second viewport shadow handle, 0 absent; renderer writes. */
    char sprite_ready;                    /* 4a: nonzero when current sprite frame is ready; renderer writes. */
    char sprite_state;                    /* 4b: player animation ID or kind-specific state; selectors write. */
    char level;                           /* 4c: world level, 0..26 in gameplay; setup/transitions write. */
    unsigned char behavior_flags;         /* 4d: ENTITY collision/attachment/pixel bits; subsystem owners write. */
    struct earthling_state *earthling;    /* 4e: owning earthling record, 0 for other entities; setup writes. */
    unsigned long animation_group_mask;   /* 52: SetSprite writes 1 << (sprite_state >> 2), not free flags. */
    char owner;                           /* 56: owner/entity index, or -1/-2 camera anchor; effect setup writes. */
    char attachment_x_or_latch;            /* 57: X offset or one-shot/player-zero contact latch; owner writes. */
    char attachment_y_or_latch;            /* 58: Y offset or player-one contact latch; owner writes. */
    unsigned char attachment_height;      /* 59: signed-view parent elevation offset in pixels; effect setup writes. */
    struct object_state *next;            /* 5a: render-order link, 0 terminates; sorting/setup writes. */
    char entity_index;                    /* 5e: index in gPlayerInfo, 0..39; SetRAMIndexes writes. */
    char unused_tail[33];                 /* 5f..7f: no reads/writes observed in represented code. */
};

#endif
