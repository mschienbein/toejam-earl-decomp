#ifndef SPRITES_H
#define SPRITES_H

/* Eight-byte VDP sprite attribute-table record. */
struct hardware_sprite {
    short y;
    unsigned char size, link;
    unsigned short tile;
    short x;
};

/* Ten-byte ROM sprite piece, including action icons and transition parts.
 * format remains char because the transition uploader tests its high bit
 * with a negative comparison. The data pointer is at byte 06. */
struct render_piece {
    unsigned char width, height;
    char x, y, flipped_x, format;
    char *data;
};

/* Exact four-byte metasprite prefix. Storage continues with count ten-byte
 * render_piece records, beginning at (struct render_piece *)(frame + 1).
 * There is no common fixed piece count, so this prefix is not an array stride
 * for complete frames and deliberately has no speculative pieces[12] tail. */
/* Bytes 01/02 multiply entity velocity in camera and movement callbacks. */
struct render_frame {
    unsigned char count;
    char motion_x, motion_y, reusable;
};

/* Eight-byte sequence/frame-table pair; distinct from timed_animation. */
struct animation_descriptor {
    char *sequence;
    struct render_frame **frames;
};

/* Ten-byte descriptor. Byte 09 is used as a Y offset by elevator parts;
 * player and effect consumers do not read that byte. */
struct timed_animation {
    char *sequence;
    struct render_frame **frames;
    char period, y_offset;
};

/* Eighteen-byte Funkatron scene descriptor, followed separately by placement
 * records. These are external ROM interfaces even though one C unit uses them. */
struct scene_descriptor {
    struct render_frame **frames;
    char *idle_sequence, *interaction_sequence;
    char frame_phase;
    unsigned char width, height;
    char draw_offset, draw_attribute, reserved_11;
};

/* Six-byte placement record; a zero x separates descriptor groups. */
struct scene_placement {
    short x, y, interaction;
};

#endif
