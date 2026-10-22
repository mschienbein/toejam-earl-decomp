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

    gHardwareSpriteTable[0].link = 23;
    gHardwareSpriteTable[23].link = 79;
    gHardwareSpriteTable[24].link = 79;
    entity = gPlayerInfo;
    entity_number = 0;
    do {
        entity->entity_index = entity_number;
        ++entity_number;
        ++entity;
    } while (entity_number < 40);
}
