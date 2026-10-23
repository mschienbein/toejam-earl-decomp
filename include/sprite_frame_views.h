#ifndef TJE_SPRITE_FRAME_VIEWS_H
#define TJE_SPRITE_FRAME_VIEWS_H

#include "sprites.h"

/* PrepareEntitySpriteFrame routes tile_format bit six to SpriteUnpackRaw. */
#define SPRITE_FORMAT_RAW_TILES 0x40
/* PrepareEntitySpriteFrame shifts tile_format's low two palette bits into the VDP tile word. */
#define SPRITE_FORMAT_PALETTE_MASK 3

/* Variable-payload access views, never allocation or array-stride types.
 * Only header.piece_count pieces are present. Twelve is the entity's maximum
 * piece capacity; four is the smaller consumers' maximum, not a fixed stride.
 * MATCHING QUALIFICATION: bounded members preserve MCC68K word indexing;
 * neither bound establishes a complete allocation for every ROM frame.
 * Keep these views separate from sprites.h: unrelated large translation
 * units need only the prefix and descriptors and approach MCC68K's DOS heap. */
struct render_frame_piece_view {
    struct render_frame header;
    struct render_piece pieces[12];
};

struct render_frame_four_piece_view {
    struct render_frame header;
    struct render_piece pieces[4];
};

#endif
