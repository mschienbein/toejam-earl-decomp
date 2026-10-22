#include "earthlings.h"
#include "ram_earthlings.h"
#include "random.h"
#include "ship.h"

/* Identify a category-nine earthling whose level has a non-one bank mode. */
int func_020e70(earthling)
struct earthling_state *earthling;
{
    if (earthling->kind == 9 &&
        ((ram_ffe226[0].level == earthling->level &&
          ram_ffe226[0].mode != 1) ||
         (ram_ffe226[1].level == earthling->level &&
          ram_ffe226[1].mode != 1)))
        return 1;
    return 0;
}

/* Configure the bank for levels two and above. Levels four and above, except
 * twelve, select mode one for half the random residues; other cases clear it. */
char func_020eaa(bank, level)
short bank, level;
{
    unsigned short random;

    if (level >= 2) {
        ram_ffe226[bank].level = level;
        if (level >= 4 && level != 12 &&
            (random = GetRandomNumber()) % 100 < 50)
            ram_ffe226[bank].mode = 1;
        else {
            ram_ffe226[bank].mode = 0;
            ram_ffe226[bank].contact[0] = 0;
            ram_ffe226[bank].contact[1] = 0;
        }
        return 1;
    }
    ram_ffe226[bank].level = 0;
    return 0;
}
