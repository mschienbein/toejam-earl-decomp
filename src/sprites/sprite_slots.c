#include "hardware.h"
#include "entities.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Select unused slots first, then reusable slots when requested. The returned
 * identifiers are the flag-array indices plus 25. */
int AllocateSpriteSlots(count, selection_mode, include_secondary, output)
short count, selection_mode;
char include_secondary;
char *output;
{
    char flags;
    short slot;
    char free_count = 0, secondary_count = 0;
    short index, limit;
    char free_slots[ENTITY_SPRITES_PER_VIEW], secondary_slots[ENTITY_SPRITES_PER_VIEW];

    flags = SPRITE_SLOT_ALLOCATED;
    if (include_secondary)
        flags |= SPRITE_SLOT_LOCKED;
    if (selection_mode == 1)
        flags |= SPRITE_SLOT_SECONDARY_VIEW;
    limit = VIEWPORT_SPRITE_SLOT_COUNT;
    for (index = 0; index < limit; ++index) {
        char state;

        slot = index;
        if (selection_mode == 1)
            slot = VIEWPORT_SPRITE_SLOT_COUNT - 1 - index;
        state = gViewportSpriteSlotFlags[slot];
        if (!(state & SPRITE_SLOT_ALLOCATED))
            free_slots[free_count++] = slot;
        else if (include_secondary && !(state & SPRITE_SLOT_LOCKED) &&
                 secondary_count < count - free_count)
            secondary_slots[secondary_count++] = slot;
        if (free_count == count)
            break;
    }
    if (count > free_count + secondary_count)
        return 1;
    for (index = 0; index < free_count; ++index) {
        char selected = free_slots[index];

        gViewportSpriteSlotFlags[selected] = flags;
        *output++ = selected + VIEWPORT_SPRITE_FIRST;
    }
    if (free_count < count) {
        count -= free_count;
        for (index = 0; index < count; ++index) {
            char selected = secondary_slots[index];

            gViewportSpriteSlotFlags[selected] = flags;
            *output++ = selected + VIEWPORT_SPRITE_FIRST;
        }
    }
    return 0;
}

/* Ask the selector for one byte. A nonzero status suppresses the result.
 * The ROM calls AllocateSpriteSlots PC-relative; [INFERENCE] the two were in one
 * original source file, since the assembler used PC-relative calls only
 * within a file (config/rev02/tus.json). match.py assembles it that way. */
char AllocateSpriteSlot(selection_mode, include_secondary)
short selection_mode;
int include_secondary;
{
    char value;

    if (AllocateSpriteSlots(1, selection_mode, include_secondary, &value))
        return 0;
    else
        return value;
}

/* Release each identifier in a viewport's slot list. */
void FreeSpriteSlots(count, slots)
short count;
char *slots;
{
    short index;
    char slot;

    for (index = 0; index < count; ++index) {
        slot = slots[index] - VIEWPORT_SPRITE_FIRST;
        gViewportSpriteSlotFlags[slot] = 0;
    }
}

/* Clear the allocation and lock flags for a sprite handle numbered from 25. */
void FreeSpriteSlot(item)
char item;
{
    item -= VIEWPORT_SPRITE_FIRST;
    gViewportSpriteSlotFlags[item] = 0;
}

/* Return whether the selected viewport sprite slot has its lock bit set. */
int IsSpriteSlotLocked(item)
int item;
{
    if (gViewportSpriteSlotFlags[item - VIEWPORT_SPRITE_FIRST] & SPRITE_SLOT_LOCKED)
        return 1;
    else
        return 0;
}

/* Reserve a consecutive run of eight-tile groups, returning its first tile. */
short AllocateSpriteTileRun(count)
short count;
{
    short groups, run, index, mark_index, tile;

    groups = (count - 1) / SPRITE_TILES_PER_GROUP + 1;
    run = 0;
    for (index = 0; index < SPRITE_TILE_GROUP_COUNT; ++index) {
        if (!gSpriteTileGroupAllocated[index]) {
            ++run;
            if (run == groups) {
                index = index + 1 - run;
                tile = (index << 3) + SPRITE_TILE_POOL_FIRST;
                for (mark_index = 0; mark_index < run; ++mark_index)
                    gSpriteTileGroupAllocated[index++] = 1;
                return tile;
            }
        } else
            run = 0;
    }
    return 0;
}

/* Release the eight-tile groups occupied by a tile range. */
void FreeSpriteTileRun(tile, count)
short tile, count;
{
    short groups, index;

    groups = (count - 1) / SPRITE_TILES_PER_GROUP + 1;
    tile -= SPRITE_TILE_POOL_FIRST;
    tile /= SPRITE_TILES_PER_GROUP;
    for (index = 0; index < groups; ++index)
        gSpriteTileGroupAllocated[tile + index] = 0;
}

/* Drop cached sprite identifiers whose slot is no longer marked secondary. */
void ReleaseEntityShadows(entity)
struct object_state *entity;
{
    if (entity->primary_shadow && !IsSpriteSlotLocked(entity->primary_shadow)) {
        FreeSpriteSlot(entity->primary_shadow);
        entity->primary_shadow = 0;
    }
    if (entity->secondary_shadow && !IsSpriteSlotLocked(entity->secondary_shadow)) {
        FreeSpriteSlot(entity->secondary_shadow);
        entity->secondary_shadow = 0;
    }
}

/* Allocate the tile range when necessary, then a viewport's sprite slots. */
void AllocateEntityViewportSprites(entity, viewport)
struct object_state *entity;
short viewport;
{
    char viewport_bit;
    short tile, tile_count;
    char *slots;

    tile_count = entity->tile_count;
    if (!(entity->draw_flags & ENTITY_EITHER_VIEW) && !(entity->behavior_flags & ENTITY_SHARED_TILES)) {
        tile = AllocateSpriteTileRun(tile_count);
        if (!tile)
            return;
        entity->tile &= 0xf800;
        entity->tile |= tile;
    }
    if (!viewport) {
        viewport_bit = ENTITY_PRIMARY_VIEW;
        slots = entity->primary_sprites;
    } else {
        viewport_bit = ENTITY_SECONDARY_VIEW;
        slots = entity->secondary_sprites;
    }
    if (!AllocateSpriteSlots(entity->sprite_count, viewport, 1, slots))
        entity->draw_flags |= viewport_bit;
    else if (!(entity->draw_flags & ENTITY_EITHER_VIEW))
        /* ORIGINAL BUG: With behavior_flags bit 2 set and no viewport
         * active, a selector failure reads tile without assigning it. */
        FreeSpriteTileRun(tile, tile_count);
}

/* Release one viewport's slots and, after the last viewport, its tile range. */
void ReleaseEntityViewportSprites(entity, viewport)
struct object_state *entity;
short viewport;
{
    char *slots;

    if (!(entity->draw_flags & ENTITY_ACTIVE))
        return;
    if (!viewport) {
        entity->draw_flags &= ~ENTITY_PRIMARY_VIEW;
        slots = entity->primary_sprites;
        if (entity->primary_shadow && !IsSpriteSlotLocked(entity->primary_shadow)) {
            FreeSpriteSlot(entity->primary_shadow);
            entity->primary_shadow = 0;
        }
    } else {
        entity->draw_flags &= ~ENTITY_SECONDARY_VIEW;
        slots = entity->secondary_sprites;
        if (entity->secondary_shadow && !IsSpriteSlotLocked(entity->secondary_shadow)) {
            FreeSpriteSlot(entity->secondary_shadow);
            entity->secondary_shadow = 0;
        }
    }
    FreeSpriteSlots(entity->sprite_count, slots);
    if (!(entity->draw_flags & ENTITY_EITHER_VIEW) && !(entity->behavior_flags & ENTITY_SHARED_TILES)) {
        short tile_count, tile;

        tile_count = entity->tile_count;
        tile = entity->tile & VDP_TILE_INDEX_MASK;
        FreeSpriteTileRun(tile, tile_count);
    }
}

/* The caller supplies storage for 47 words. */
void ResetEntityRuntimeState(words)
short *words;
{
    short index;

    for (index = 0; index < 47; ++index)
        *words++ = 0;
}

/* Claim an entity from the pool beginning at record six. */
short AllocateEntity()
{
    short count, index;

    if (gPlayer1MapLevel == LEVEL_FINAL)
        count = DYNAMIC_ENTITY_SLOT_COUNT;
    else
        count = DYNAMIC_ENTITY_GAMEPLAY_SLOTS;
    for (index = 0; index < count; ++index) {
        if (!gDynamicEntitySlotAllocated[index]) {
            gDynamicEntitySlotAllocated[index] = 1;
            count = index + DYNAMIC_ENTITY_FIRST_SLOT;
            ResetEntityRuntimeState(&gPlayerInfo[count]);
            gPlayerInfo[count].draw_flags = ENTITY_ACTIVE;
            return count;
        }
    }
    return 0;
}

/* Players zero and one are retained; other active records release both views. */
void FreeEntity(entity_number)
short entity_number;
{
    struct object_state *entity;

    if (entity_number <= PLAYER_EARL)
        return;
    entity = &gPlayerInfo[entity_number];
    if (entity->draw_flags & ENTITY_ACTIVE) {
        if (entity->draw_flags & ENTITY_PRIMARY_VIEW)
            ReleaseEntityViewportSprites(entity, VIEWPORT_PRIMARY);
        if (entity->draw_flags & ENTITY_SECONDARY_VIEW)
            ReleaseEntityViewportSprites(entity, VIEWPORT_SECONDARY);
        entity->draw_flags = 0;
        if (entity_number >= DYNAMIC_ENTITY_FIRST_SLOT)
            gDynamicEntitySlotAllocated[entity_number - DYNAMIC_ENTITY_FIRST_SLOT] = 0;
    }
}
