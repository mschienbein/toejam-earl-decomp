struct indexed_sprite {
    short y;
    unsigned char size, link;
    unsigned short tile;
    short x;
};
struct indexed_entity {
    char reserved00[94];
    char entity_number;
    char reserved5f[33];
};
extern struct indexed_sprite ram_ffb65a[80];
extern struct indexed_entity gPlayerInfo[40];

/* Initialize the sprite sentinels and store each entity's own slot number. */
void SetRAMIndexes()
{
    struct indexed_entity *entity;
    short entity_number;

    ram_ffb65a[0].link = 23;
    ram_ffb65a[23].link = 79;
    ram_ffb65a[24].link = 79;
    entity = gPlayerInfo;
    entity_number = 0;
    do {
        entity->entity_number = entity_number;
        ++entity_number;
        ++entity;
    } while (entity_number < 40);
}
