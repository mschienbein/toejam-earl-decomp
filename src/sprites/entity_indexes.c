#include "entities.h"
#include "sprites.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Initialize the sprite sentinels and store each entity's own slot number. */
void SetRAMIndexes()
{
    struct object_state *entity;
    short entity_number;

    gHardwareSpriteTable[SPRITE_CHAIN_ROOT_SLOT].link = SPRITE_CHAIN_PRIMARY_HEAD_SLOT;
    gHardwareSpriteTable[SPRITE_CHAIN_PRIMARY_HEAD_SLOT].link = SPRITE_CHAIN_TERMINATOR_SLOT;
    gHardwareSpriteTable[SPRITE_CHAIN_SECONDARY_HEAD_SLOT].link = SPRITE_CHAIN_TERMINATOR_SLOT;
    entity = gPlayerInfo;
    entity_number = 0;
    do {
        entity->entity_index = entity_number;
        ++entity_number;
        ++entity;
    } while (entity_number < ENTITY_SLOT_COUNT);
}
