struct sprite_definition {
    char *sequence, *frames;
    char period, reserved09;
};
struct animated_entity {
    char reserved00[26];
    char *sequence, *frames;
    char reserved22[4];
    char frame, frame_period, frame_ticks;
    char reserved29[34];
    char sprite_state;
    char reserved4c[6];
    long sprite_mask;
    char reserved56[8];
    char character;
};
extern struct sprite_definition PlayerAnimationDescriptors[], rom_0ae9de[];

/* Select a character's ten-byte sprite descriptor and reset its animation. */
void SetSprite(entity, sprite_state)
struct animated_entity *entity;
char sprite_state;
{
    struct sprite_definition *definitions, *definition;

    if (sprite_state < 0)
        return;
    if (!entity->character)
        definitions = PlayerAnimationDescriptors;
    else
        definitions = rom_0ae9de;
    definition = &definitions[sprite_state];
    entity->sequence = definition->sequence;
    entity->frames = definition->frames;
    entity->frame_period = definition->period;
    entity->frame_ticks = 1;
    entity->frame = -1;
    entity->sprite_state = sprite_state;
    entity->sprite_mask = 1 << (entity->sprite_state >> 2);
}
