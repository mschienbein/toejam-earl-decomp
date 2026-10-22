struct setup_sprite {
    short y;
    unsigned char size, link;
    unsigned short tile;
    short x;
};
struct setup_entity {
    char reserved00[22];
    void (*process)();
    char *sequence, *frames;
    char reserved22[5];
    char frame_period, frame_ticks;
    unsigned char half_width, height, tile_count;
    unsigned short tile_base;
    char sprite_count, flags;
    char reserved30[29];
    char state_flags;
    char reserved4e[12];
    struct setup_entity *next;
    char reserved5e[34];
};
struct setup_definition {
    char *sequence, *frames;
    char period, reserved09;
};
extern struct setup_sprite ram_ffb65a[80];
extern struct setup_entity gPlayerInfo[40], *ram_ffd93c;
extern struct setup_definition PlayerAnimationDescriptors[], rom_0ae9de[];
extern char Spr_PresentSelector[], PresentSpriteTable[], rom_0ae4a0[];
extern unsigned char ram_ffd932, ram_ffd934;
extern void func_00e400(), func_00e410();

/* Initialize the hardware sprite links, the two players, and the two
 * selector-plus-six-present record chains beginning at record 26. */
void SetupPlayer()
{
    short sprite_number, present_number;
    unsigned short tile;
    struct setup_entity *entity;

    ram_ffb65a[0].link = 11;
    for (sprite_number = 0; sprite_number < 10; ++sprite_number) {
        tile = 0x85a9;
        if (sprite_number & 1)
            tile |= 0x1800;
        ram_ffb65a[sprite_number + 1].x = (sprite_number << 5) + 128;
        ram_ffb65a[sprite_number + 1].y = 236;
        ram_ffb65a[sprite_number + 1].size = 12;
        ram_ffb65a[sprite_number + 1].link = sprite_number + 2;
        ram_ffb65a[sprite_number + 1].tile = tile;
    }
    for (sprite_number = 11; sprite_number < 23; ++sprite_number)
        ram_ffb65a[sprite_number].link = sprite_number + 1;
    ram_ffb65a[sprite_number].link = 23;
    ram_ffb65a[23].link = 79;
    ram_ffd932 = 79;
    ram_ffb65a[24].link = 79;
    ram_ffd934 = 79;
    ram_ffb65a[79].link = 0;
    entity = &gPlayerInfo[0];
    entity->process = func_00e400;
    entity->sequence = PlayerAnimationDescriptors[0].sequence;
    entity->frames = PlayerAnimationDescriptors[0].frames;
    entity->frame_period = 10;
    entity->frame_ticks = 1;
    entity->half_width = 24;
    entity->height = 32;
    entity->sprite_count = 3;
    entity->tile_count = 40;
    entity->tile_base = 0x8000;
    entity->flags = 1;
    entity->state_flags = 1;
    entity = &gPlayerInfo[1];
    entity->process = func_00e410;
    entity->sequence = rom_0ae9de[0].sequence;
    entity->frames = rom_0ae9de[0].frames;
    entity->frame_period = 10;
    entity->frame_ticks = 1;
    entity->half_width = 30;
    entity->height = 40;
    entity->sprite_count = 4;
    entity->tile_count = 40;
    entity->tile_base = 0x8000;
    entity->flags = 1;
    entity->state_flags = 1;
    entity = &gPlayerInfo[26];
    ram_ffd93c = entity;
    for (sprite_number = 0; sprite_number < 2; ++sprite_number) {
        entity->frames = Spr_PresentSelector;
        entity->sequence = rom_0ae4a0;
        entity->half_width = 32;
        entity->height = 24;
        entity->sprite_count = 1;
        entity->tile_count = 12;
        entity->tile_base = 0x8000;
        entity->next = entity + 1;
        entity = entity->next;
        for (present_number = 0; present_number < 6; ++present_number) {
            entity->frames = PresentSpriteTable;
            entity->sequence = rom_0ae4a0;
            entity->half_width = 24;
            entity->height = 16;
            entity->sprite_count = 1;
            entity->tile_count = 6;
            entity->tile_base = 0x8000;
            entity->next = entity + 1;
            entity = entity->next;
        }
    }
    entity[-1].next = 0;
}
