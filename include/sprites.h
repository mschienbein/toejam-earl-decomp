#ifndef TJE_SPRITES_H
#define TJE_SPRITES_H

/* Sequence command bytes, compared through the caller's existing byte view. */
#define ANIMATION_HOLD 254
#define ANIMATION_LOOP 255
#define ANIMATION_FRAME_MASK 127

/* Eight-byte VDP sprite attribute-table record, populated by the renderer. */
struct hardware_sprite {
    short y;                         /* 00: hardware Y plus 128 bias; sprite positioning writes. */
    unsigned char size, link;         /* 02/03: VDP size and next handle, 0 terminates; renderer writes. */
    unsigned short tile;             /* 04: VDP tile/flip/palette/priority word; renderer writes. */
    short x;                         /* 06: hardware X plus 128 bias; sprite positioning writes. */
};

/* Ten-byte ROM sprite piece; signed format preserves the compression high-bit test. */
struct render_piece {
    unsigned char width, height;     /* 00/01: 8x8 tile dimensions; ROM-authored. */
    char x, y, flipped_x, tile_format;/* 02..05: pixel offsets and compression/palette flags; ROM-authored. */
    char *tile_data;                 /* 06: packed/raw tile stream; ROM-authored. */
};

/* Four-byte prefix followed by piece_count ten-byte pieces, not a fixed-array stride. */
struct render_frame {
    unsigned char piece_count;       /* 00: hardware-piece count; ROM-authored (runtime text also writes). */
    char motion_x, motion_y, reuse_tiles; /* 01..03: velocity multipliers and tile-reuse flag; ROM-authored. */
};

/* Eight-byte sequence/frame-table pair, distinct from timed_animation. */
struct animation_descriptor {
    char *sequence;                  /* 00: animation bytecode stream; ROM-authored. */
    struct render_frame **frames;    /* 04: metasprite-pointer table; ROM-authored. */
};

/* Ten-byte descriptor; elevator parts also consume the final signed Y offset. */
struct timed_animation {
    char *sequence;                  /* 00: animation bytecode stream; ROM-authored. */
    struct render_frame **frames;    /* 04: metasprite-pointer table; ROM-authored. */
    char period, y_offset;           /* 08/09: ticks per frame and pixel Y offset; ROM-authored. */
};

/* Eighteen-byte Funkatron descriptor, followed separately by placement records. */
struct scene_descriptor {
    struct render_frame **frames;    /* 00: scene metasprite-pointer table; ROM-authored. */
    char *idle_sequence, *interaction_sequence; /* 04/08: idle and contact bytecode; ROM-authored. */
    char frame_phase;                /* 0c: ticks per animation frame; ROM-authored. */
    unsigned char width, height;     /* 0d/0e: entity bounds in pixels; ROM-authored. */
    char tile_count, sprite_count, unused; /* 0f..11: tile/piece reservations, then unused byte; ROM-authored. */
};

/* Six-byte placement; zero X separates descriptor groups. */
struct scene_placement {
    short x, y, interaction;         /* 00/02/04: world pixels and dialogue ID, 0 absent; ROM-authored. */
};

/* Two-byte frame selection followed by the FE hold command. */
struct held_frame2 {
    char frame, hold;                /* 00/01: frame selector and hold byte; ROM-authored. */
};

#endif
