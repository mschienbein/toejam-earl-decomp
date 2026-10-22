struct linked_sprite {
    short y;
    unsigned char size;
    char link;
    unsigned short tile;
    short x;
};
struct linked_entity {
    char reserved00[34];
    char *sprite_descriptor;
    char reserved26[10];
    char primary_sprites[12], secondary_sprites[12];
    char reserved48[56];
};
extern struct linked_sprite ram_ffb65a[80];
extern struct linked_entity gPlayerInfo[26];
extern void VDPWritesRoutine1();

/* Chain both players' primary sprite lists and transfer the hardware table. */
void func_00d07a()
{
    char index, sprite_count, sprite_number, next_sprite;
    char *sprite_list;

    sprite_count = *gPlayerInfo[0].sprite_descriptor;
    sprite_list = gPlayerInfo[0].primary_sprites;
    sprite_number = *sprite_list;
    ram_ffb65a[23].link = sprite_number;
    for (index = 0; index < sprite_count; ++index) {
        sprite_number = sprite_list[index];
        if (index >= sprite_count - 1)
            break;
        next_sprite = sprite_list[index + 1];
        ram_ffb65a[sprite_number].link = next_sprite;
    }
    sprite_count = *gPlayerInfo[1].sprite_descriptor;
    sprite_list = gPlayerInfo[1].primary_sprites;
    ram_ffb65a[sprite_number].link = *sprite_list;
    for (index = 0; index < sprite_count; ++index) {
        sprite_number = sprite_list[index];
        if (index < sprite_count - 1)
            next_sprite = sprite_list[index + 1];
        else
            next_sprite = 79;
        ram_ffb65a[sprite_number].link = next_sprite;
    }
    VDPWritesRoutine1(ram_ffb65a, 0xf800, 320);
}
