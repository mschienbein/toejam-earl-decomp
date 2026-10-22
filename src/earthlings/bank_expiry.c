#include "earthlings.h"
#include "earthling_calls.h"
#include "ram_earthlings.h"
#include "sprite_calls.h"

/* Count down matching earthling banks, then release their displayed objects. */
void EarthlingReleaseLevelBank(level)
short level;
{
    short bank, record_number;
    char object_number;

    for (bank = 0; bank < 2; ++bank) {
        if (EARTHLING_LOAD_FLAG[bank] && EARTHLING_LEVEL[bank] == level) {
            --EARTHLING_LOAD_FLAG[bank];
            if (!EARTHLING_LOAD_FLAG[bank]) {
                EARTHLING_LEVEL[bank] = -1;
                for (record_number = 0; record_number < 20; ++record_number) {
                    object_number = EARTHLINGS_IN_LEVEL[bank][record_number].object;
                    /* Byte zero is set to the released-record marker. */
                    EARTHLINGS_IN_LEVEL[bank][record_number].kind = -1;
                    if (object_number) {
                        FreeEntity(object_number);
                        EARTHLINGS_IN_LEVEL[bank][record_number].object = 0;
                    }
                    if (object_number = EARTHLINGS_IN_LEVEL[bank][record_number].second_object) {
                        FreeEntity(object_number);
                        EARTHLINGS_IN_LEVEL[bank][record_number].second_object = 0;
                    }
                }
            }
        }
    }
}
