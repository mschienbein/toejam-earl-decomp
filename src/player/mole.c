struct object_state {
    short x, y, height;
    short velocity_x, velocity_y, velocity_height;
    short acceleration_x, acceleration_y, acceleration_height;
    char reserved_12[3], mode_15, reserved_16[4];
    char *sequence;
    char **frames;
    char reserved_22[4];
    char frame, frame_phase, frame_duration;
    unsigned char width, height_pixels;
    char draw_offset;
    unsigned short tile;
    char draw_attribute;
    unsigned char draw_flags;
    char reserved_30[27], sprite, level;
    unsigned char flags_4d;
    char reserved_4e[4];
    unsigned long flags_52;
    char reserved_56, flag_57, flag_58, reserved_59[5], value_5e;
    char reserved_5f[33];
};
struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer;
};
struct present_info {
    char sprite, reserved;
};
extern struct object_state gPlayerInfo[40];
extern char gPlayerInventory[][16];
extern struct present_info gPresentInfo[28];
extern char *PresentSpriteTable[28];
extern unsigned short PREGEN_RANDOMS[59], ram_ff91e0;
extern char ram_ffe24a[], ram_ffe24c[], dat_0ae4a0[];
/* ORIGINAL BUG: this file calls func_00d904 as int (an implicit declaration in the original
 * [INFERENCE]); its definition in sprites/entity_slots_tu14.c returns short. The caller keeps
 * the low byte, so behaviour is unaffected. */
extern int func_00d904();
extern int GetActivePresentSprite(), func_01b0aa(), func_012db8();
extern void SetSprite(), SetEarthlingState();

/* Remove a random occupied inventory slot, replacing it with the last slot,
 * and launch a present object from the victim's position when allocation succeeds. */
void DoMoleAbility(victim, player_number)
struct object_state *victim;
short player_number;
{
    short inventory_count, selected_slot, present_number, present_sprite;
    struct object_state *present;

    inventory_count = 0;
    do {
        if (gPlayerInventory[player_number][inventory_count] < 0)
            break;
        ++inventory_count;
    } while (inventory_count < 16);
    if (!inventory_count)
        return;
    if (--inventory_count > 0)
        selected_slot = PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                       (ram_ff91e0 = 0) : ++ram_ff91e0] % (inventory_count + 1);
    else
        selected_slot = 0;
    present_number = gPlayerInventory[player_number][selected_slot];
    if (selected_slot < inventory_count)
        gPlayerInventory[player_number][selected_slot] =
            gPlayerInventory[player_number][inventory_count];
    gPlayerInventory[player_number][inventory_count] = -1;
    if (ram_ffe24a[player_number] = func_00d904()) {
        present = &gPlayerInfo[ram_ffe24a[player_number]];
        present->x = victim->x;
        present->y = victim->y - 3;
        present->height = 20;
        present->acceleration_height = -32;
        if (PREGEN_RANDOMS[ram_ff91e0 == 58 ?
            (ram_ff91e0 = 0) : ++ram_ff91e0] & 1)
            present->velocity_x = 256;
        else
            present->velocity_x = -256;
        present->level = victim->level;
        present->width = 24;
        present->height_pixels = 16;
        present->draw_attribute = 1;
        present->draw_offset = 6;
        present->tile = 0x8000;
        present_sprite = gPresentInfo[present_number].sprite;
        present->frames = &PresentSpriteTable[present_sprite];
        present->sequence = dat_0ae4a0;
        present->frame = -1;
        present->frame_duration = 1;
        present->draw_flags |= 64;
    }
    ++ram_ffe24c[player_number];
}

/* A mole in state 3 releases the victim and periodically removes presents.
 * Otherwise, an eligible nearby player enters that state with the mole. */
void MoleAbility(player_number, earthling)
short player_number;
struct earthling_state *earthling;
{
    struct object_state *victim, *mole;

    victim = &gPlayerInfo[player_number];
    if (earthling->state == 3) {
        if (victim->sprite == 70) {
            if (earthling->timer < 15) {
                SetSprite(victim, GetActivePresentSprite(victim));
                victim->flags_4d &= ~32;
            } else if (!ram_ffe24a[player_number]) {
                if (!ram_ffe24c[player_number] ||
                    (ram_ffe24c[player_number] < 3 &&
                     PREGEN_RANDOMS[ram_ff91e0 == 58 ?
                     (ram_ff91e0 = 0) : ++ram_ff91e0] % 100 < 8))
                    DoMoleAbility(victim, player_number);
            }
        }
        return;
    }
    mole = &gPlayerInfo[earthling->object];
    if (gPlayerInventory[player_number][0] == -1 ||
        (player_number == 0 && mole->flag_57) ||
        (player_number == 1 && mole->flag_58) ||
        !func_01b0aa(victim, player_number))
        return;
    if (!func_012db8(mole->x, mole->y, victim->level)) {
        victim->x = mole->x;
        victim->y = mole->y;
    } else {
        mole->x = victim->x;
        mole->y = victim->y;
    }
    SetEarthlingState(earthling, 3);
    earthling->timer = 30;
    mole->frame = -1;
    mole->flags_4d |= 1;
    mole->draw_flags |= 128;
    if (player_number == 0)
        ++mole->flag_57;
    else
        ++mole->flag_58;
    SetSprite(victim, 70);
    victim->mode_15 = 3;
    victim->flags_4d |= 32;
    ram_ffe24c[player_number] = 0;
}
