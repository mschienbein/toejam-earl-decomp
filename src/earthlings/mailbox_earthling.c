#include "earthling_types.h"
#include "terrain.h"
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
          gMailboxLevelStates[0].mode != MAILBOX_MODE_BITING) ||
         (gMailboxLevelStates[1].level == earthling->level &&
          gMailboxLevelStates[1].mode != MAILBOX_MODE_BITING)))
        return 1;
    return 0;
}

/* Configure the bank for levels two and above. Levels four and above, except
 * twelve, select mode one for half the random residues; other cases clear it. */
char EarthlingConfigureMailboxLevelBank(bank, level)
short bank, level;
{
    unsigned short random;

    if (level >= MAILBOX_FIRST_LEVEL) {
        gMailboxLevelStates[bank].level = level;
        if (level >= MAILBOX_FIRST_BITING_LEVEL && level != MAILBOX_ALWAYS_MAIL_ORDER_LEVEL &&
            (random = GetRandomNumber()) % 100 < 50)
            gMailboxLevelStates[bank].mode = MAILBOX_MODE_BITING;
        else {
            gMailboxLevelStates[bank].mode = MAILBOX_MODE_MAIL_ORDER;
            gMailboxLevelStates[bank].contact[PLAYER_TOEJAM] = EARTHLING_INTERACTION_IDLE;
            gMailboxLevelStates[bank].contact[PLAYER_EARL] = EARTHLING_INTERACTION_IDLE;
        }
        return 1;
    }
    gMailboxLevelStates[bank].level = LEVEL_ZERO;
    return 0;
}
