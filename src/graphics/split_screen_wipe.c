struct wipe_entity {
    char reserved00[47];
    unsigned char flags;
    char reserved30[26];
    char sprite_active;
    char reserved4b[53];
};
extern struct wipe_entity gPlayerInfo[26];
extern short ram_ff803e;
extern void func_00d776(), func_01e94e(), func_00cc82();
extern void WaitForNextVBlank(), SpriteLoadingStuff(), func_00d07a(), func_00cfba();
#define VDP_CONTROL (*(unsigned long *)0xc00004L)
#define VDP_CONTROL_WORD (*(unsigned short *)0xc00004L)
#define VDP_DATA (*(unsigned short *)0xc00000L)

/* Ensure both primary sprites are resident, wipe paired rows inward, then
 * fill the 28-by-40 destination and write 0x1c to VDP register 18. */
void func_002e6a()
{
    short pass, step, lower_row, upper_row, column;
    struct wipe_entity *entity;

    entity = &gPlayerInfo[0];
    for (step = 0; step < 2; ++step, ++entity) {
        while (!(entity->flags & 2)) {
            entity->sprite_active = 0;
            func_00d776(entity, 0);
            if (!(entity->flags & 2))
                func_01e94e();
        }
    }
    while (!gPlayerInfo[0].sprite_active || !gPlayerInfo[1].sprite_active) {
        func_00cc82();
        WaitForNextVBlank(1);
        SpriteLoadingStuff(0, 0);
    }
    WaitForNextVBlank(1);
    func_00d07a();
    upper_row = ram_ff803e & 31;
    lower_row = (ram_ff803e + 29) & 31;
    for (step = 0; step < 15; ++step) {
        WaitForNextVBlank(1);
        VDP_CONTROL = (upper_row << 23) + 0x60000002L;
        for (pass = 0; pass < 2; ++pass) {
            for (column = 0; column < 64; ++column)
                VDP_DATA = 0x25b4;
            VDP_CONTROL = (lower_row << 23) + 0x60000002L;
        }
        if (step == 14)
            break;
        upper_row = (upper_row + 1) & 31;
        lower_row = (lower_row - 1) & 31;
        VDP_CONTROL = (upper_row << 23) + 0x60000002L;
        for (pass = 0; pass < 2; ++pass) {
            for (column = 0; column < 64; ++column)
                VDP_DATA = 0x5ab;
            VDP_CONTROL = (lower_row << 23) + 0x60000002L;
        }
    }
    func_00cfba(1);
    for (pass = 0; pass < 28; ++pass) {
        VDP_CONTROL = (pass << 23) + 0x50000003L;
        for (column = 0; column < 40; ++column)
            VDP_DATA = 0xa5b4;
    }
    VDP_CONTROL_WORD = 0x921c;
}
