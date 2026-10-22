#include "player.h"
#include "ram_player.h"

/* Initialize the four per-player bytes used by PlayerUpdatePhasedJumpMotion. */
void PlayerInitializePhasedJumpMotion(player)
short player;
{
    gPlayerPhasedJumpPhase[player] = 1;
    gPlayerPhasedJumpPhaseTicks[player] = 16;
    gPlayerPhasedJumpArmed[player] = 1;
    gPlayerPhasedJumpSpeechUsed[player] = 0;
}
