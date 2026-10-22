#include "presents.h"
#include "present_calls.h"
#include "ram_presents.h"
#include "sprite_calls.h"

/* Remove either telephone's object and clear its level and ringing state. */
void TelephoneClearAll()
{
    short telephone_number;
    struct telephone_state *telephone;

    for (telephone_number = 0; telephone_number <= 1; ++telephone_number) {
        telephone = &TELEPHONE_1[telephone_number];
        if (telephone->level) {
            if (telephone->object) {
                FreeEntity(telephone->object);
                telephone->object = 0;
            }
            telephone->level = 0;
            gTelephoneActivated[telephone_number] = 0;
            gTelephoneRingsLeft[telephone_number] = 0;
            gTelephoneTimeToRing[telephone_number] = 0;
        }
    }
}
