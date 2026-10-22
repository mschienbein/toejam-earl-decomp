#include "player.h"
#include "ram_player.h"

/* Initialize the four per-player bytes used by func_011ac0. */
void func_011a7c(player)
short player;
{
    ram_ffda26[player] = 1;
    ram_ffda28[player] = 16;
    ram_ffda36[player] = 1;
    ram_ffda38[player] = 0;
}
