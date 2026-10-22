#include "entities.h"
#include "sprites.h"
#include "assets_sprites.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Update each active viewport sprite's size bits and tile from elevation. */
void UpdateEntityShadows(entity)
struct object_state *entity;
{
    char viewport_bit, sprite_number;
    unsigned char depth;
    short viewport;

    viewport_bit = 2;
    sprite_number = entity->primary_shadow;
    viewport = 0;
    do {
        if (entity->draw_flags & viewport_bit) {
            if (IsSpriteSlotLocked(sprite_number))
                sprite_number = 0;
            if (!sprite_number) {
                sprite_number = AllocateSpriteSlot(viewport, 0);
                if (sprite_number) {
                    gHardwareSpriteTable[sprite_number].size &= 0x8f;
                    if (!viewport)
                        entity->primary_shadow = sprite_number;
                    else
                        entity->secondary_shadow = sprite_number;
                } else
                    return;
            }
            if (sprite_number) {
                if (entity->height < 0 && entity->sprite_state != -1) {
                    gHardwareSpriteTable[sprite_number].size = 12;
                    gHardwareSpriteTable[sprite_number].tile = 0x87f9;
                } else {
                    unsigned char old_depth = gHardwareSpriteTable[sprite_number].size & 0x70;

                    if (entity->height >= 128 || entity->sprite_state == -1)
                        depth = 16;
                    else
                        depth = 127 - entity->height;
                    depth &= 0x70;
                    if (depth != old_depth) {
                        gHardwareSpriteTable[sprite_number].size = depth;
                        depth >>= 4;
                        gHardwareSpriteTable[sprite_number].size |= (depth << 1) & 12;
                        gHardwareSpriteTable[sprite_number].tile = SpriteTileAllocationLookup[depth - 1] + 0xe7a0;
                    }
                }
            }
        }
        viewport_bit = 4;
        sprite_number = entity->secondary_shadow;
        ++viewport;
    } while (viewport <= 1);
}

/* Walk the 26 entity records and dispatch the active record's flag mode. */
void UpdateAllEntityShadows()
{
    struct object_state *entity;
    short entity_number;

    entity = gPlayerInfo;
    entity_number = 0;
    do {
        if (entity->draw_flags & ENTITY_ACTIVE) {
            if (entity->draw_flags & ENTITY_SHADOW)
                UpdateEntityShadows(entity);
            else
                ReleaseEntityShadows(entity);
        }
        ++entity_number;
        ++entity;
    } while (entity_number < 26);
}
