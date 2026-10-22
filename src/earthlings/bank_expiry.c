#include "earthling_types.h"
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

    for (bank = 0; bank < EARTHLING_BANK_COUNT; ++bank) {
        if (gEarthlingBankUsers[bank] && gEarthlingBankLevels[bank] == level) {
            --gEarthlingBankUsers[bank];
            if (!gEarthlingBankUsers[bank]) {
                gEarthlingBankLevels[bank] = -1;
                for (record_number = 0; record_number < EARTHLINGS_PER_BANK; ++record_number) {
                    object_number = gEarthlingBanks[bank][record_number].object;
                    /* Byte zero is set to the released-record marker. */
                    gEarthlingBanks[bank][record_number].kind = EARTHLING_UNUSED;
                    if (object_number) {
                        FreeEntity(object_number);
                        gEarthlingBanks[bank][record_number].object = 0;
                    }
                    if (object_number = gEarthlingBanks[bank][record_number].second_object) {
                        FreeEntity(object_number);
                        gEarthlingBanks[bank][record_number].second_object = 0;
                    }
                }
            }
        }
    }
}
