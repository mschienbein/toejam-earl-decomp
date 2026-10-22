#include "entities.h"
#include "presents.h"
#include "present_calls.h"
#include "ram_player.h"
#include "ram_presents.h"
#include "random.h"

/* Place a telephone away from either player unless one already serves the level. */
void TelephonePlaceOutsidePlayerViews(player_number)
short player_number;
{
    register struct object_state *player;
    struct object_state *other_player;
    struct telephone_state *telephone;
    short telephone_x, telephone_y;

    player = &gPlayerInfo[player_number];
    if (gPlayerLives[player_number ^ 1] >= 0)
        other_player = &gPlayerInfo[player_number ^ 1];
    else
        other_player = 0;
    if (gTelephoneStates[player_number].level == player->level ||
        gTelephoneStates[player_number ^ 1].level == player->level)
        return;
    telephone = &gTelephoneStates[player_number];
    do {
        MapFindItemPlacementPosition(&telephone_x, &telephone_y, player->level, 0);
    } while ((abs(player->x - telephone_x) < 480 &&
              abs(player->y - telephone_y) < 336) ||
             (other_player && abs(other_player->x - telephone_x) < 320 &&
              abs(other_player->y - telephone_y) < 224));
    telephone->level = player->level;
    telephone->x = telephone_x;
    telephone->y = telephone_y;
    gTelephoneActivated[player_number] = 0;
}

/* Put a telephone behind the player, moving it clear of the other player. */
void OpenTelephone(player_number)
short player_number;
{
    struct telephone_state *telephone;
    struct object_state *player;
    short telephone_number;

    player = &gPlayerInfo[player_number];
    if (!gTelephoneStates[player_number].level)
        telephone_number = player_number;
    else if (!gTelephoneStates[player_number ^ 1].level)
        telephone_number = player_number ^ 1;
    else if (!gTelephoneActivated[player_number])
        telephone_number = player_number;
    else if (!gTelephoneActivated[player_number ^ 1])
        telephone_number = player_number ^ 1;
    else
        telephone_number = player_number;
    telephone = &gTelephoneStates[telephone_number];
    telephone->level = player->level;
    telephone->x = player->x;
    telephone->y = player->y + 10;
    if (gPlayerLives[player_number ^ 1] >= 0) {
        player = &gPlayerInfo[player_number ^ 1];
        if (player->level == telephone->level &&
            16 > abs(telephone->x - player->x)) {
            while (8 > abs(telephone->y - player->y))
                ++telephone->y;
        }
    }
    gTelephoneActivated[telephone_number] = 0;
    PresentStartTransientEffect(player_number, telephone->x, telephone->y + 1, 0,
                telephone->level);
}
