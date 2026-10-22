#include "entities.h"
#include "sprites.h"
#include "engine_calls.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Copy complete 32-byte tiles to the next transfer-buffer position. */
void ClearLOC_00FFB8DA(source, tile_count)
long *source;
short tile_count;
{
    long *destination;
    short longword_count;

    destination = (long *)&ram_ffb8da[SPRITE_STATE_TABLE];
    longword_count = tile_count << 3;
    while (longword_count > 0) {
        *destination++ = *source++;
        --longword_count;
    }
    SPRITE_STATE_TABLE += tile_count << 5;
}

/* Call frame preparation callbacks within the 112-tile frame budget. */
void func_00cb54()
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
        if (!(entity->draw_flags & 1) || func_01dee8(entity_number))
            continue;
        if (entity->frame_duration - 1 == 0) {
            if (entity->tile_count + tile_total > 112)
                continue;
            next_frame_or_needs_tiles = entity->frame + 1;
            sequence_entry = entity->sequence[next_frame_or_needs_tiles];
            if (sequence_entry == 255)
                next_frame_or_needs_tiles = 0;
            else if (sequence_entry == 254)
                continue;
            {
                char frame_number;
                frame_number = entity->sequence[next_frame_or_needs_tiles] & 127;
                frame = entity->frames[frame_number];
            }
            /* The sequence index is no longer needed; keep the upload predicate in this byte. */
            next_frame_or_needs_tiles = 1;
            if (frame->reusable && entity->sprite_ready)
                next_frame_or_needs_tiles = 0;
            if (entity->mode_15 & 3)
                (*entity->process)(frame, entity);
            if ((entity->draw_flags & 6) && next_frame_or_needs_tiles)
                tile_total += entity->tile_count;
        } else if (!entity->sprite_ready && (entity->draw_flags & 6) &&
                   entity->tile_count + tile_total > 112) {
            /* ORIGINAL BUG: this path adds the count only when it is already over 112. */
            tile_total += entity->tile_count;
        }
    }
}
