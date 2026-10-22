#include "entities.h"
#include "ram.h"
#include "player.h"
#include "present_calls.h"
#include "ui_calls.h"
#include "romcalls.h"
/* Dispatch player-state handlers, then update common promotion and collision processing. */
void ProcessPlayers()
{
    unsigned short terrain;
    struct object_state *player;
    short player_number;

    terrain = LAND_UNDER_TOEJAM & 0xf0;
    player = gPlayerInfo;
    for (player_number = 0; player_number <= 1;
         terrain = LAND_UNDER_EARL & 0xf0, ++player, ++player_number) {
        if (gPlayerLives[player_number] < 0)
            func_021d32(player_number);
        else {
            if (P1_MENU_OPEN[player_number])
                continue;
            func_00f96c(player);
            func_01044e(player, terrain);
            func_01062c(player, terrain);
            func_0107b2(player, terrain);
            func_011ac0(player, terrain);
            func_01189c(player, terrain);
            func_010086(player, terrain);
            func_0101b4(player, terrain);
            func_010294(player, terrain);
            func_010ffc(player, terrain);
            func_01c188(player, terrain);
            ProcessSleep(player);
            func_01b8ba(player, terrain);
            HealthAndLivesChecks(player, terrain);
            if (ELEVATOR_FLAG[player_number] || (ram_ffde60[0] & 3) == player_number + 1 ||
                (ram_ffde61 & 3) == player_number + 1)
                player->flags_4d |= 32;
            else if (player->sprite_state != 70 && player->sprite_state != 120)
                player->flags_4d &= ~32;
        }
        BummerLifePromo(player, terrain);
        func_01ccd2(player);
        func_01d084(player, terrain);
    }
    CheckPlayerFalling();
    if (PLAYER_INDICATOR == 2)
        func_012116();
}
