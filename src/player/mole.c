#include "entities.h"
#include "earthlings.h"
#include "presents.h"
#include "assets_presents.h"
#include "earthling_calls.h"
#include "elevator_calls.h"
#include "player.h"
#include "ram_earthlings.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "ram_system.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

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
        selected_slot = PREGEN_RANDOMS[gRandomCacheIndex == 58 ?
                       (gRandomCacheIndex = 0) : ++gRandomCacheIndex] % (inventory_count + 1);
    else
        selected_slot = 0;
    present_number = gPlayerInventory[player_number][selected_slot];
    if (selected_slot < inventory_count)
        gPlayerInventory[player_number][selected_slot] =
            gPlayerInventory[player_number][inventory_count];
    gPlayerInventory[player_number][inventory_count] = -1;
    if (gPlayerStolenPresentEntities[player_number] = (int)AllocateEntity()) {
        present = &gPlayerInfo[gPlayerStolenPresentEntities[player_number]];
        present->x = victim->x;
        present->y = victim->y - 3;
        present->height = 20;
        present->acceleration_height = -32;
        if (PREGEN_RANDOMS[gRandomCacheIndex == 58 ?
            (gRandomCacheIndex = 0) : ++gRandomCacheIndex] & 1)
            present->velocity_x = 256;
        else
            present->velocity_x = -256;
        present->level = victim->level;
        present->width = 24;
        present->height_pixels = 16;
        present->sprite_count = 1;
        present->tile_count = 6;
        present->tile = 0x8000;
        present_sprite = gPresentInfo[present_number].sprite;
        present->frames = &PresentSpriteTable[present_sprite];
        present->sequence = SingleFrameHoldAnimation;
        present->frame = -1;
        present->frame_duration = 1;
        present->draw_flags |= 64;
    }
    ++gPlayerMoleTheftCounts[player_number];
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
        if (victim->sprite_state == 70) {
            if (earthling->timer < 15) {
                SetSprite(victim, GetActivePresentSprite(victim));
                victim->flags_4d &= ~32;
            } else if (!gPlayerStolenPresentEntities[player_number]) {
                if (!gPlayerMoleTheftCounts[player_number] ||
                    (gPlayerMoleTheftCounts[player_number] < 3 &&
                     PREGEN_RANDOMS[gRandomCacheIndex == 58 ?
                     (gRandomCacheIndex = 0) : ++gRandomCacheIndex] % 100 < 8))
                    DoMoleAbility(victim, player_number);
            }
        }
        return;
    }
    mole = &gPlayerInfo[earthling->object];
    if (gPlayerInventory[player_number][0] == -1 ||
        (player_number == 0 && mole->flag_57) ||
        (player_number == 1 && mole->flag_58) ||
        !PlayerCanReceiveEarthlingEffect(victim, player_number))
        return;
    if (!ElevatorContainsPosition(mole->x, mole->y, victim->level)) {
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
    gPlayerMoleTheftCounts[player_number] = 0;
}
