struct hidden_entity {
    char reserved00[46];
    unsigned char sprite_count, flags;
    char primary_sprites[12], secondary_sprites[12];
    char reserved48[2];
    char sprite_active;
    char reserved4b[2];
    unsigned char state_flags;
    char reserved4e[50];
};
extern struct hidden_entity gPlayerInfo[26];
#define VDP_CONTROL (*(unsigned long *)0xc00004L)
#define VDP_DATA (*(unsigned short *)0xc00000L)

/* Move the selected active sprite list to hardware Y coordinate one. */
void func_00d152(player)
short player;
{
    struct hidden_entity *entity;
    char *sprite_list;
    short sprite_count;
    char sprite_number;

    entity = &gPlayerInfo[player];
    if (!(entity->state_flags & 16) || !entity->sprite_active)
        return;
    if (entity->flags & 2)
        sprite_list = entity->primary_sprites;
    else {
        if (!(entity->flags & 4))
            return;
        sprite_list = entity->secondary_sprites;
    }
    for (sprite_count = entity->sprite_count; sprite_count > 0; --sprite_count) {
        sprite_number = *sprite_list++;
        VDP_CONTROL = (sprite_number << 19) + 0x78000003L;
        VDP_DATA = 1;
    }
}
