#include "earthling_types.h"
#include "entities.h"
#include "earthlings.h"
#include "ram_earthlings.h"
#include "random.h"
#include "ship.h"

/* Identify a category-nine earthling whose level has a non-one bank mode. */
int EarthlingIsMailOrderMailbox(earthling)
struct earthling_state *earthling;
{
    if (earthling->kind == EARTHLING_MAILBOX &&
        ((gMailboxLevelStates[0].level == earthling->level &&
          gMailboxLevelStates[0].mode != 1) ||
         (gMailboxLevelStates[1].level == earthling->level &&
          gMailboxLevelStates[1].mode != 1)))
        return 1;
    return 0;
}

/* Configure the bank for levels two and above. Levels four and above, except
 * twelve, select mode one for half the random residues; other cases clear it. */
char EarthlingConfigureMailboxLevelBank(bank, level)
short bank, level;
{
    unsigned short random;

    if (level >= 2) {
        gMailboxLevelStates[bank].level = level;
        if (level >= 4 && level != 12 &&
            (random = GetRandomNumber()) % 100 < 50)
            gMailboxLevelStates[bank].mode = 1;
        else {
            gMailboxLevelStates[bank].mode = 0;
            gMailboxLevelStates[bank].contact[PLAYER_TOEJAM] = 0;
            gMailboxLevelStates[bank].contact[PLAYER_EARL] = 0;
        }
        return 1;
    }
    gMailboxLevelStates[bank].level = 0;
    return 0;
}
