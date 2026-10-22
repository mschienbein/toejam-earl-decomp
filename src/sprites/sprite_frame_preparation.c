#include "sprites.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Remove the packed rows below the supplied vertical cutoff. */
short ClipSpritePixelsBelowCutoff(piece, cutoff)
struct render_piece *piece;
short cutoff;
{
    short column, row, row_width, first_hidden;
    long *pixels;

    row_width = piece->height * 8;
    row = piece->y - 2 - cutoff;
    if (row >= 0)
        return 1;
    if (row_width + row <= 0)
        return 0;
    first_hidden = -row;
    for (row = 0; row < piece->width; ++row) {
        pixels = (long *)&gSpriteTileUploadBuffer[(row * row_width * 4 +
                 gSpriteTileBufferOffset) + first_hidden * 4];
        *pixels++ &= 0x0f000ff0L;
        for (column = first_hidden + 1; column < row_width; ++column)
            *pixels++ = 0;
    }
    return 0;
}

/* Discard pixels whose packed byte is an all-ones nibble mask. */
void ClearSpriteMaskPixels(byte_count)
unsigned short byte_count;
{
    char *pixels;
    char packed_pixels;

    pixels = &gSpriteTileUploadBuffer[gSpriteTileBufferOffset];
    while (byte_count > 0) {
        packed_pixels = *pixels;
        if (packed_pixels == -1 || packed_pixels == (char)240 || packed_pixels == 15)
            packed_pixels = 0;
        *pixels++ = packed_pixels;
        --byte_count;
    }
}


#include "entities.h"
#include "sprites.h"
#include "hardware.h"
#include "player_animation_groups.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Decode a frame's pieces, assign tile attributes and queue the resulting run.
 * MATCHING QUALIFICATION: the explicit int promotion of the width and the
 * explicit int-to-short height ownership below are redundant in C. Together
 * they retain the original multiply operand and packed-size scratch lifetime.
 * The redundant height-to-word mask retains the observed extraction sequence.
 * Ordinary direct-byte/short spellings did not match; no record type changes.
 */
void PrepareEntitySpriteFrame(entity, frame, reuse_tiles, viewport)
struct object_state *entity;
struct render_frame *frame;
int reuse_tiles, viewport;
{
    short sprite_number;
    unsigned short tile_offset = 0;
    short piece_number, discard = 0, elevation = 0;
    unsigned short tile_base = entity->tile;
    char format;
    unsigned short tile_count;
    short sprite_size, height_in_tiles;
    struct render_piece *piece;

    if (entity->entity_index <= PLAYER_EARL)
        elevation = entity->height;
    for (piece_number = 0; piece_number < frame->count; ++piece_number) {
        /* Twelve is the entity sprite-list capacity, not a fixed ROM-frame
         * extent; this inherited bounded payload view retains word indexing. */
        piece = &((struct render_piece (*)[12])(frame + 1))[0][piece_number];
        format = piece->format;
        tile_count = piece->height;
        tile_count &= 255;
        tile_count *= (int)piece->width;
        if (!reuse_tiles) {
            if (format < 0)
                SpriteUnpackInterleavedRLE(piece, tile_count);
            else if (format & 64)
                SpriteUnpackRaw(piece, tile_count);
            else
                SpriteUnpackRLE(piece, tile_count);
            if (elevation < 0 && !((unsigned char)entity->animation_group_mask & PLAYER_GROUP_FALL))
                discard = ClipSpritePixelsBelowCutoff(piece, elevation);
            if (entity->behavior_flags & ENTITY_MASK_PIXELS)
                ClearSpriteMaskPixels(tile_count * 32);
            if (!discard)
                gSpriteTileBufferOffset += tile_count * 32;
        }
        if (!viewport)
            sprite_number = entity->primary_sprites[piece_number];
        else
            sprite_number = entity->secondary_sprites[piece_number];
        if (!discard) {
            sprite_size = (piece->width - 1) * 4;
            height_in_tiles = (int)piece->height;
            sprite_size += height_in_tiles - 1;
            gHardwareSpriteTable[sprite_number].size = sprite_size;
            gHardwareSpriteTable[sprite_number].tile = ((short)(format & 3) << 13) + tile_base + tile_offset;
            tile_offset += tile_count;
        } else {
            gHardwareSpriteTable[sprite_number].size = 0;
            gHardwareSpriteTable[sprite_number].tile = 0;
        }
    }
    if (!reuse_tiles && tile_offset > 0) {
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = tile_base & VDP_TILE_INDEX_MASK;
        gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = tile_offset;
    }
}


#include "entities.h"
#include "sprites.h"
#include "engine_calls.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Copy complete 32-byte tiles to the next transfer-buffer position. */
void AppendSpriteTilesToBuffer(source, tile_count)
long *source;
short tile_count;
{
    long *destination;
    short longword_count;

    destination = (long *)&gSpriteTileUploadBuffer[gSpriteTileBufferOffset];
    longword_count = tile_count << 3;
    while (longword_count > 0) {
        *destination++ = *source++;
        --longword_count;
    }
    gSpriteTileBufferOffset += tile_count << 5;
}

/* Call frame preparation callbacks within the 112-tile frame budget. */
void ProcessEntityAnimationMotion()
{
    char next_frame_or_needs_tiles;
    short tile_total = 0, entity_number, entity_count;
    struct object_state *entity;
    struct render_frame *frame;
    unsigned char sequence_entry;

    if (gLevel912C == 26)
        entity_count = 40;
    else
        entity_count = 26;
    entity = gPlayerInfo;
    for (entity_number = 0; entity_number < entity_count; ++entity_number, ++entity) {
        if (!(entity->draw_flags & ENTITY_ACTIVE) || IsSpeechBubbleEntity(entity_number))
            continue;
        if (entity->frame_duration - 1 == 0) {
            if (entity->tile_count + tile_total > 112)
                continue;
            next_frame_or_needs_tiles = entity->frame + 1;
            sequence_entry = entity->sequence[next_frame_or_needs_tiles];
            if (sequence_entry == ANIMATION_LOOP)
                next_frame_or_needs_tiles = 0;
            else if (sequence_entry == ANIMATION_HOLD)
                continue;
            {
                char frame_number;
                frame_number = entity->sequence[next_frame_or_needs_tiles] & ANIMATION_FRAME_MASK;
                frame = entity->frames[frame_number];
            }
            /* The sequence index is no longer needed; keep the upload predicate in this byte. */
            next_frame_or_needs_tiles = 1;
            if (frame->reusable && entity->sprite_ready)
                next_frame_or_needs_tiles = 0;
            if (entity->animation_motion_axes & ANIMATION_MOTION_XY)
                (*entity->process)(frame, entity);
            if ((entity->draw_flags & ENTITY_EITHER_VIEW) && next_frame_or_needs_tiles)
                tile_total += entity->tile_count;
        } else if (!entity->sprite_ready && (entity->draw_flags & ENTITY_EITHER_VIEW) &&
                   entity->tile_count + tile_total > 112) {
            /* ORIGINAL BUG: this path adds the count only when it is already over 112. */
            tile_total += entity->tile_count;
        }
    }
}
