#include "entities.h"
#include "sprites.h"
#include "assets.h"
#include "sprite_calls.h"
/* Select a character's ten-byte sprite descriptor and reset its animation. */
void SetSprite(entity, sprite_state)
struct object_state *entity;
char sprite_state;
{
    struct timed_animation *definitions, *definition;

    if (sprite_state < 0)
        return;
    if (!entity->entity_number)
        definitions = PlayerAnimationDescriptors;
    else
        definitions = rom_0ae9de;
    definition = &definitions[sprite_state];
    entity->sequence = definition->sequence;
    entity->frames = definition->frames;
    entity->frame_phase = definition->period;
    entity->frame_duration = 1;
    entity->frame = -1;
    entity->sprite_state = sprite_state;
    entity->flags_52 = 1 << (entity->sprite_state >> 2);
}
