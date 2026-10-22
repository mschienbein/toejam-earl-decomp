#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Preserve the observed signed/unsigned value views of shared storage.
 * These casts do not imply a different record layout; historical spelling
 * is unknown [INFERENCE]. */
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
            entity->tile &= 0x7fff;
    } else if (entity->owner == -1) {
        entity->x = TOEJAM_CAMERA_NEWX + (unsigned char)entity->attachment_x_or_latch;
        entity->y = TOEJAM_CAMERA_NEWY + (unsigned char)entity->attachment_y_or_latch;
    } else if (entity->owner == -2) {
        entity->x = EARL_CAMERA_NEWX + (unsigned char)entity->attachment_x_or_latch;
        entity->y = EARL_CAMERA_NEWY + (unsigned char)entity->attachment_y_or_latch;
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
        piece_count = frame->count;
        elevation = entity->height;
        if (entity->draw_flags & ENTITY_PRIMARY_VIEW) {
            screen_x = entity->x + 128 - TOEJAM_CAMERA_NEWX;
            screen_y = entity->y + 128 - TOEJAM_CAMERA_NEWY;
            for (piece_number = 0; piece_number < piece_count; ++piece_number) {
                sprite_number = entity->primary_sprites[piece_number];
                if (entity->behavior_flags & ENTITY_HIDDEN)
                    gHardwareSpriteTable[sprite_number].y = 1;
                else {
                    piece = &((struct render_piece (*)[12])(frame + 1))[0][piece_number];
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
            screen_x = entity->x + 128 - EARL_CAMERA_NEWX;
            screen_y = entity->y + 240 - EARL_CAMERA_NEWY;
            for (piece_number = 0; piece_number < piece_count; ++piece_number) {
                sprite_number = entity->secondary_sprites[piece_number];
                if (entity->behavior_flags & ENTITY_HIDDEN)
                    gHardwareSpriteTable[sprite_number].y = 1;
                else {
                    piece = &((struct render_piece (*)[12])(frame + 1))[0][piece_number];
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
            sprite_count = entity->current_frame->count;
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
            sprite_count = entity->current_frame->count;
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

    if (gLevel912C == 26)
        entity_count = 40;
    else
        entity_count = 26;
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
