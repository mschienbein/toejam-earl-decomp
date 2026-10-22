#define TJE_LOCAL_FUNC_00D9DC

#include "entities.h"
#include "earthlings.h"
#include "ram.h"
#include "earthling_calls.h"
#include "map_calls.h"
#include "player.h"
#include "romcalls.h"

/* ORIGINAL BUG: this consumer keeps a int result view of func_00d9dc,
 * whose canonical return is unsigned char. The observed caller omits that
 * narrowing; the historical missing declaration is [INFERENCE]. */
extern int func_00d9dc();
/* Decrement the hit-point byte; nice-earthling handling can preserve one hit.
 * Removal also creates the effect at the record's saved position. */
void DamageEarthling(earthling)
struct earthling_state *earthling;
{
    if (earthling && earthling->kind > 0) {
        if (!func_020e70(earthling) && earthling->kind != 20 &&
            earthling->kind < 23 && --earthling->reserved_10[0] <= 0) {
            if (earthling->kind == 17 || earthling->kind == 10 ||
                earthling->kind == 11) {
                if (NiceEarthlingCheck(earthling)) {
                    earthling->reserved_10[0] = 1;
                    return;
                }
            }
            func_018da4(earthling);
            func_01bde8(earthling->x, earthling->y, earthling->level);
        } else if (earthling->second_object) {
            SetEarthlingState(earthling, 0);
            earthling->timer = 0;
        }
    }
}

/* Remove eligible earthlings whose saved coordinates now lie on terrain 0x90. */
void func_018f1e(player_number, level)
short player_number, level;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->kind > 0 && earthling->object &&
            earthling->kind != 6 && earthling->kind != 22 &&
            earthling->kind != 7 && earthling->kind != 20 &&
            earthling->kind != 8 && earthling->kind != 19 &&
            func_00d9dc(player_number, earthling->x, earthling->y) == 0x90) {
            earthling->reserved_10[0] = 0;
            DamageEarthling(earthling);
        }
    }
}

/* Damage eligible allocated earthlings whose object matches the draw mask. */
void DoVikingAbility(draw_mask)
char draw_mask;
{
    short buffer, slot;
    struct earthling_state *earthling;

    for (buffer = 0; buffer < 2; ++buffer) {
        if (EARTHLING_LOAD_FLAG[buffer]) {
            for (slot = 0; slot < 20; ++slot) {
                earthling = &EARTHLINGS_IN_LEVEL[buffer][slot];
                if (earthling->kind != -1 && earthling->kind != 17 &&
                    earthling->object &&
                    (gPlayerInfo[earthling->object].draw_flags & draw_mask) &&
                    (earthling->reserved_10[0] > 1 || func_01bdd0()))
                    DamageEarthling(earthling);
            }
        }
    }
}

/* Clear the player's per-mole contact flag for every allocated mole object. */
void func_019086(player)
struct object_state *player;
{
    short slot, buffer;
    struct earthling_state *earthling;

    if (EARTHLING_LOAD_FLAG[0] && EARTHLING_LEVEL[0] == player->level)
        buffer = 0;
    else
        buffer = 1;
    earthling = EARTHLINGS_IN_LEVEL[buffer];
    for (slot = 0; slot < 20; ++slot, ++earthling) {
        if (earthling->kind == 18 && earthling->object) {
            if (!player->entity_number)
                gPlayerInfo[earthling->object].flag_57 = 0;
            else
                gPlayerInfo[earthling->object].flag_58 = 0;
        }
    }
}

/* Advance loaded earthlings unless both the menu and HBlank flags are set. */
void ProcessEarthlings()
{
    if (P1_MENU_OPEN[0] && HBLANK_FLAG_1)
        return;
    if (EARTHLING_LOAD_FLAG[0])
        LoadInEarthlings(EARTHLINGS_IN_LEVEL[0]);
    if (EARTHLING_LOAD_FLAG_COPY)
        LoadInEarthlings(EARTHLINGS_IN_LEVEL_COPY);
    func_018834();
    func_0188ee();
    func_018a6c();
    func_020d3e();
    func_0223f6();
    ram_ffe1ae ^= 1;
}
