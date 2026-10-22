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
    char free_slots[12], secondary_slots[12];

    flags = 1;
    if (include_secondary)
        flags |= 2;
    if (selection_mode == 1)
        flags |= 4;
    limit = 54;
    for (index = 0; index < limit; ++index) {
        char state;

        slot = index;
        if (selection_mode == 1)
            slot = 53 - index;
        state = gViewportSpriteSlotFlags[slot];
        if (!(state & 1))
            free_slots[free_count++] = slot;
        else if (include_secondary && !(state & 2) &&
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
        *output++ = selected + 25;
    }
    if (free_count < count) {
        count -= free_count;
        for (index = 0; index < count; ++index) {
            char selected = secondary_slots[index];

            gViewportSpriteSlotFlags[selected] = flags;
            *output++ = selected + 25;
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
        slot = slots[index] - 25;
        gViewportSpriteSlotFlags[slot] = 0;
    }
}

/* The selection helper at 0x00d406 scans 54 flag bytes.
 * Public item identifiers start at 25; bit 1 is queried by the second routine. */
void FreeSpriteSlot(item)
char item;
{
    item -= 25;
    gViewportSpriteSlotFlags[item] = 0;
}

int IsSpriteSlotLocked(item)
int item;
{
    if (gViewportSpriteSlotFlags[item - 25] & 2)
        return 1;
    else
        return 0;
}

/* Reserve a consecutive run of eight-tile groups, returning its first tile. */
short AllocateSpriteTileRun(count)
short count;
{
    short groups, run, index, mark_index, tile;

    groups = (count - 1) / 8 + 1;
    run = 0;
    for (index = 0; index < 80; ++index) {
        if (!gSpriteTileGroupAllocated[index]) {
            ++run;
            if (run == groups) {
                index = index + 1 - run;
                tile = (index << 3) + 640;
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

    groups = (count - 1) / 8 + 1;
    tile -= 640;
    tile /= 8;
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
        viewport_bit = 2;
        slots = entity->primary_sprites;
    } else {
        viewport_bit = 4;
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

    if (gLevel912C == 26)
        count = 34;
    else
        count = 20;
    for (index = 0; index < count; ++index) {
        if (!gDynamicEntitySlotAllocated[index]) {
            gDynamicEntitySlotAllocated[index] = 1;
            count = index + 6;
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

    if (entity_number <= 1)
        return;
    entity = &gPlayerInfo[entity_number];
    if (entity->draw_flags & ENTITY_ACTIVE) {
        if (entity->draw_flags & ENTITY_PRIMARY_VIEW)
            ReleaseEntityViewportSprites(entity, 0);
        if (entity->draw_flags & ENTITY_SECONDARY_VIEW)
            ReleaseEntityViewportSprites(entity, 1);
        entity->draw_flags = 0;
        if (entity_number >= 6)
            gDynamicEntitySlotAllocated[entity_number - 6] = 0;
    }
}
