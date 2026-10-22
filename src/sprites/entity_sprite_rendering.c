#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "sprite_frame_views.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* MATCHING QUALIFICATION: UpdateAttachedEntityPosition uses signed-char
 * parent-relative offsets, but reads attachment_x_or_latch and
 * attachment_y_or_latch as unsigned bytes for camera owners -1 and -2.
 * Direct reads of the shared char X/Y fields sign-extend instead, changing
 * the camera-offset arithmetic and code. The unsigned height field is read
 * as char for a parent offset. These are scalar views, not alternate record
 * declarations; historical spelling is unknown [INFERENCE]. */
/* Follow another entity, or use unsigned offsets from one of the cameras. */
void UpdateAttachedEntityPosition(entity)
struct object_state *entity;
{
    struct object_state *parent;

    if (entity->owner >= 0) {
        parent = &gPlayerInfo[entity->owner];
        entity->x = (char)entity->attachment_x_or_latch + parent->x;
        entity->y = (char)entity->attachment_y_or_latch + parent->y;
        entity->height = (char)entity->attachment_height + parent->height;
        if (parent->tile & VDP_TILE_PRIORITY)
            entity->tile |= VDP_TILE_PRIORITY;
        else
            entity->tile &= ~VDP_TILE_PRIORITY;
    } else if (entity->owner == -1) {
        entity->x = gCamera1PendingX + (unsigned char)entity->attachment_x_or_latch;
        entity->y = gCamera1PendingY + (unsigned char)entity->attachment_y_or_latch;
    } else if (entity->owner == -2) {
        entity->x = gCamera2PendingX + (unsigned char)entity->attachment_x_or_latch;
        entity->y = gCamera2PendingY + (unsigned char)entity->attachment_y_or_latch;
    }
}

/* Position every piece and optional shadow in each active viewport. */
void PositionEntitySprites(first_entity)
struct object_state *first_entity;
{
    short shadow_width, sprite_number, screen_y, screen_x, piece_number;
    short elevation;
    char piece_count;
    struct object_state *entity;
    /* The 12-piece payload view preserves MCC68K word indexing. Twelve is
     * the entity sprite-list capacity, not a claimed fixed ROM-frame extent.
     * The common frame type remains its four-byte prefix. */
    struct render_frame *frame;
    struct render_piece *piece;

    for (entity = first_entity; entity; entity = entity->next) {
        if (!(entity->draw_flags & ENTITY_ACTIVE))
            continue;
        if (entity->behavior_flags & ENTITY_ATTACHED)
            UpdateAttachedEntityPosition(entity);
        if (!entity->sprite_ready)
            continue;
        frame = entity->current_frame;
        piece_count = frame->piece_count;
        elevation = entity->height;
        if (entity->draw_flags & ENTITY_PRIMARY_VIEW) {
            screen_x = entity->x + 128 - gCamera1PendingX;
            screen_y = entity->y + 128 - gCamera1PendingY;
            for (piece_number = 0; piece_number < piece_count; ++piece_number) {
                sprite_number = entity->primary_sprites[piece_number];
                if (entity->behavior_flags & ENTITY_HIDDEN)
                    gHardwareSpriteTable[sprite_number].y = 1;
                else {
                    piece = &((struct render_frame_piece_view *)frame)->pieces[piece_number];
                    if (entity->tile & VDP_TILE_HORIZONTAL_FLIP)
                        gHardwareSpriteTable[sprite_number].x = piece->flipped_x + screen_x;
                    else
                        gHardwareSpriteTable[sprite_number].x = piece->x + screen_x;
                    if (elevation > 224 || elevation < -224)
                        gHardwareSpriteTable[sprite_number].y = 0;
                    else
                        gHardwareSpriteTable[sprite_number].y = piece->y + screen_y - elevation;
                }
            }
            if ((sprite_number = entity->primary_shadow) && !(entity->behavior_flags & ENTITY_HIDDEN)) {
                if (!IsSpriteSlotLocked(sprite_number)) {
                    shadow_width = (gHardwareSpriteTable[sprite_number].size & 12) + 4;
                    gHardwareSpriteTable[sprite_number].x = screen_x - shadow_width;
                    if (elevation < 0)
                        screen_y -= 2;
                    else
                        screen_y -= 4;
                    gHardwareSpriteTable[sprite_number].y = screen_y;
                } else
                    entity->primary_shadow = 0;
            }
        }
        if (entity->draw_flags & ENTITY_SECONDARY_VIEW) {
            screen_x = entity->x + 128 - gCamera2PendingX;
            screen_y = entity->y + 240 - gCamera2PendingY;
            for (piece_number = 0; piece_number < piece_count; ++piece_number) {
                sprite_number = entity->secondary_sprites[piece_number];
                if (entity->behavior_flags & ENTITY_HIDDEN)
                    gHardwareSpriteTable[sprite_number].y = 1;
                else {
                    piece = &((struct render_frame_piece_view *)frame)->pieces[piece_number];
                    if (entity->tile & VDP_TILE_HORIZONTAL_FLIP)
                        gHardwareSpriteTable[sprite_number].x = piece->flipped_x + screen_x;
                    else
                        gHardwareSpriteTable[sprite_number].x = piece->x + screen_x;
                    if (elevation > 224 || elevation < -224)
                        gHardwareSpriteTable[sprite_number].y = 0;
                    else
                        gHardwareSpriteTable[sprite_number].y = piece->y + screen_y - elevation;
                }
            }
            if ((sprite_number = entity->secondary_shadow) && !(entity->behavior_flags & ENTITY_HIDDEN)) {
                if (!IsSpriteSlotLocked(sprite_number)) {
                    shadow_width = (gHardwareSpriteTable[sprite_number].size & 12) + 4;
                    gHardwareSpriteTable[sprite_number].x = screen_x - shadow_width;
                    if (elevation < 0)
                        screen_y -= 2;
                    else
                        screen_y -= 4;
                    gHardwareSpriteTable[sprite_number].y = screen_y;
                } else
                    entity->secondary_shadow = 0;
            }
        }
    }
}

/* Rebuild the hardware link order: pieces, shadows, then menu pieces. */
void BuildViewportSpriteLinks()
{
    short piece_number, primary_count = 0, secondary_count = 0;
    short sprite_count, previous_sprite, sprite_number;
    short primary_sprites[54], secondary_sprites[54];
    struct object_state *entity;

    for (entity = gEntityDrawListHead; entity; entity = entity->next) {
        if (entity->sprite_ready) {
            sprite_count = entity->current_frame->piece_count;
            if (entity->draw_flags & ENTITY_PRIMARY_VIEW)
                for (piece_number = 0; piece_number < sprite_count; ++piece_number)
                    primary_sprites[primary_count++] = entity->primary_sprites[piece_number];
            if (entity->draw_flags & ENTITY_SECONDARY_VIEW)
                for (piece_number = 0; piece_number < sprite_count; ++piece_number)
                    secondary_sprites[secondary_count++] = entity->secondary_sprites[piece_number];
        }
    }
    for (entity = gEntityDrawListHead; entity; entity = entity->next) {
        if (entity->sprite_ready && (entity->draw_flags & ENTITY_SHADOW)) {
            if (entity->primary_shadow)
                primary_sprites[primary_count++] = entity->primary_shadow;
            if (entity->secondary_shadow)
                secondary_sprites[secondary_count++] = entity->secondary_shadow;
        }
    }
    for (entity = gInventoryDrawListHead; entity; entity = entity->next) {
        if ((entity->draw_flags & ENTITY_ACTIVE) && entity->sprite_ready) {
            sprite_count = entity->current_frame->piece_count;
            if (entity->draw_flags & ENTITY_PRIMARY_VIEW)
                for (piece_number = 0; piece_number < sprite_count; ++piece_number)
                    primary_sprites[primary_count++] = entity->primary_sprites[piece_number];
            if (entity->draw_flags & ENTITY_SECONDARY_VIEW)
                for (piece_number = 0; piece_number < sprite_count; ++piece_number)
                    secondary_sprites[secondary_count++] = entity->secondary_sprites[piece_number];
        }
    }
    gHardwareSpriteTable[23].link = 79;
    previous_sprite = 23;
    for (piece_number = 0; piece_number < primary_count; ++piece_number) {
        sprite_number = primary_sprites[piece_number];
        gHardwareSpriteTable[sprite_number].link = 79;
        gHardwareSpriteTable[previous_sprite].link = sprite_number;
        previous_sprite = sprite_number;
    }
    gHardwareSpriteTable[24].link = 79;
    gHardwareSpriteTable[previous_sprite].link = 24;
    previous_sprite = 24;
    for (piece_number = 0; piece_number < secondary_count; ++piece_number) {
        sprite_number = secondary_sprites[piece_number];
        gHardwareSpriteTable[sprite_number].link = 79;
        gHardwareSpriteTable[previous_sprite].link = sprite_number;
        previous_sprite = sprite_number;
    }
}

/* Sort active records by descending Y, keeping flag-7 records after the rest. */
void SortEntitiesForSpriteDraw()
{
    struct object_state *normal_head = 0, *normal_tail = 0;
    struct object_state *late_head = 0, *late_tail = 0;
    struct object_state *entity, *previous, *current;
    short entity_number, entity_count;

    if (gPlayer1MapLevel == LEVEL_FINAL)
        entity_count = ENTITY_SLOT_COUNT;
    else
        entity_count = GAMEPLAY_ENTITY_SLOT_COUNT;
    entity = gPlayerInfo;
    for (entity_number = 0; entity_number < entity_count; ++entity_number, ++entity) {
        if (!(entity->draw_flags & ENTITY_ACTIVE))
            continue;
        if (entity->draw_flags & ENTITY_DRAW_LATE) {
            previous = 0;
            current = late_head;
            while (current) {
                if (entity->y > current->y) {
                    if (!previous) {
                        entity->next = late_head;
                        late_head = entity;
                    } else {
                        entity->next = current;
                        previous->next = entity;
                    }
                    break;
                }
                previous = current;
                current = current->next;
            }
            if (!current) {
                if (!late_head)
                    late_head = entity;
                else
                    late_tail->next = entity;
                entity->next = 0;
                late_tail = entity;
            }
        } else {
            previous = 0;
            current = normal_head;
            while (current) {
                if (entity->y > current->y) {
                    if (!previous) {
                        entity->next = normal_head;
                        normal_head = entity;
                    } else {
                        entity->next = current;
                        previous->next = entity;
                    }
                    break;
                }
                previous = current;
                current = current->next;
            }
            if (!current) {
                if (!normal_head)
                    normal_head = entity;
                else
                    normal_tail->next = entity;
                entity->next = 0;
                normal_tail = entity;
            }
        }
    }
    if (normal_head) {
        gEntityDrawListHead = normal_head;
        normal_tail->next = late_head;
    } else
        gEntityDrawListHead = late_head;
}


#include "ram_system.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Decode one piece with VBlank graphics uploads temporarily suppressed. */
int QueueSpritePieceTiles(piece, destination_tile)
struct render_piece *piece;
short destination_tile;
{
    unsigned short tile_count;
    short saved_transfer_flag;

    gVBlankGraphicsPaused = 1;
    saved_transfer_flag = (char)gSpriteGraphicsUploadPending;
    gSpriteGraphicsUploadPending = 0;
    gVBlankGraphicsPaused = 0;
    tile_count = piece->width * piece->height;
    if (piece->tile_format < 0)
        SpriteUnpackInterleavedRLE(piece, tile_count);
    else
        SpriteUnpackRLE(piece, tile_count);
    gSpriteTileBufferOffset += tile_count * VDP_TILE_BYTES;
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = destination_tile;
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = tile_count;
    gSpriteGraphicsUploadPending = saved_transfer_flag;
    return (short)tile_count;
}


#include "sprites.h"
#include "sprite_frame_views.h"
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
    for (piece_number = 0; piece_number < frame->piece_count; ++piece_number) {
        /* Twelve is the entity sprite-list capacity, not a fixed ROM-frame
         * extent; this inherited bounded payload view retains word indexing. */
        piece = &((struct render_frame_piece_view *)frame)->pieces[piece_number];
        format = piece->tile_format;
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
                ClearSpriteMaskPixels(tile_count * VDP_TILE_BYTES);
            if (!discard)
                gSpriteTileBufferOffset += tile_count * VDP_TILE_BYTES;
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

    if (gPlayer1MapLevel == LEVEL_FINAL)
        entity_count = ENTITY_SLOT_COUNT;
    else
        entity_count = GAMEPLAY_ENTITY_SLOT_COUNT;
    entity = gPlayerInfo;
    for (entity_number = 0; entity_number < entity_count; ++entity_number, ++entity) {
        if (!(entity->draw_flags & ENTITY_ACTIVE) || IsSpeechBubbleEntity(entity_number))
            continue;
        if (entity->frame_duration - 1 == 0) {
            if (entity->tile_count + tile_total > SPRITE_FRAME_TILE_BUDGET)
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
            if (frame->reuse_tiles && entity->sprite_ready)
                next_frame_or_needs_tiles = 0;
            if (entity->animation_motion_axes & ANIMATION_MOTION_XY)
                (*entity->process)(frame, entity);
            if ((entity->draw_flags & ENTITY_EITHER_VIEW) && next_frame_or_needs_tiles)
                tile_total += entity->tile_count;
        } else if (!entity->sprite_ready && (entity->draw_flags & ENTITY_EITHER_VIEW) &&
                   entity->tile_count + tile_total > SPRITE_FRAME_TILE_BUDGET) {
            /* ORIGINAL BUG: this path adds the count only when it is already over 112. */
            tile_total += entity->tile_count;
        }
    }
}


#include "hardware.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_ui.h"
#include "ram_system.h"
#include "ram_camera.h"
#include "sprite_calls.h"
#include "ui_calls.h"
#include "engine_calls.h"
#include "jam_calls.h"

/* Advance animation sequences, upload each viewport, and publish the sprite links. */
void UpdateAndQueueEntitySprites()
{
    char next_frame;
    char sequence_entry;
    unsigned short tile_total = 0;
    short entity_number;
    struct object_state *entity;
    struct render_frame *frame;

    InventorySetCameraSuspension(1);
    while (gSpriteGraphicsUploadPending)
        ;
    if (!gSplitScreenEnabled && (gPlayerInventoryOpen[PLAYER_TOEJAM] || gPlayerInventoryOpen[PLAYER_EARL]))
        entity_number = GAMEPLAY_ENTITY_SLOT_COUNT;
    else
        entity_number = 0;
    entity = &gPlayerInfo[entity_number];
    for (; entity_number < ENTITY_SLOT_COUNT; ++entity_number, ++entity) {
        if (!(entity->draw_flags & ENTITY_ACTIVE) || (entity->behavior_flags & ENTITY_SHARED_TILES) ||
            IsSpeechBubbleEntity(entity_number) || (!entity->frame_duration && entity->sprite_ready))
            continue;
        if (tile_total && entity->tile_count + tile_total > (unsigned long)SPRITE_FRAME_TILE_BUDGET)
            continue;
        if (entity->frame_duration && !--entity->frame_duration) {
            entity->frame_duration = entity->frame_phase;
            next_frame = entity->frame + 1;
            sequence_entry = entity->sequence[next_frame];
            if (sequence_entry == (char)ANIMATION_LOOP)
                next_frame = 0;
            else if (sequence_entry == (char)ANIMATION_HOLD) {
                entity->frame_duration = 0;
                continue;
            }
load_frame:
            {
                unsigned short reuse_tiles, already_uploaded;
                entity->frame = next_frame;
                sequence_entry = entity->sequence[next_frame];
                if (sequence_entry & ANIMATION_FLIP_HORIZONTAL)
                    entity->tile |= VDP_TILE_HORIZONTAL_FLIP;
                else
                    entity->tile &= ~VDP_TILE_HORIZONTAL_FLIP;
                sequence_entry &= ANIMATION_FRAME_MASK;
                frame = entity->frames[sequence_entry];
                reuse_tiles = 0;
                already_uploaded = 0;
                if (frame->reuse_tiles && entity->sprite_ready) {
                    reuse_tiles = 1;
                    already_uploaded = 1;
                }
                entity->current_frame = frame;
                if (entity->draw_flags & ENTITY_PRIMARY_VIEW) {
                    PrepareEntitySpriteFrame(entity, frame, reuse_tiles, 0);
                    reuse_tiles = 1;
                    entity->sprite_ready = 1;
                }
                if (entity->draw_flags & ENTITY_SECONDARY_VIEW) {
                    PrepareEntitySpriteFrame(entity, frame, reuse_tiles, 1);
                    reuse_tiles = 1;
                    entity->sprite_ready = 1;
                }
                if (reuse_tiles && !already_uploaded)
                    tile_total += entity->tile_count;
            }
        } else if (!entity->sprite_ready) {
            next_frame = entity->frame;
            if (next_frame == -1)
                ++next_frame;
            goto load_frame;
        }
    }
    if (gSharedTileSpriteCopiesEnabled)
        JamOutSynchronizeEntityCopies();
    SortEntitiesForSpriteDraw();
    PositionEntitySprites(gEntityDrawListHead);
    if (gPlayerInventoryOpen[PLAYER_TOEJAM] || gPlayerInventoryOpen[PLAYER_EARL]) {
        PositionEntitySprites(gInventoryDrawListHead);
        InventorySetCameraSuspension(0);
    }
    BuildViewportSpriteLinks();
    gSpriteGraphicsUploadPending = 1;
}
