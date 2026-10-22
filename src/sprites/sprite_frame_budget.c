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
