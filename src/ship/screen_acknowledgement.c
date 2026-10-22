#include "entities.h"
#include "core_calls.h"
#include "ram_system.h"
#include "ship.h"

/* Animate for at most 240 ticks, or accept and release A/B/C from an enabled pad. */
void WaitForShipScreenAcknowledgement(player_number)
short player_number;
{
    short pressed_buttons;

    TIME_COUNTER = 0;
    while (TIME_COUNTER < 240) {
        if (player_number == PLAYER_TOEJAM || player_number == 2) {
            pressed_buttons = MovePadInputIntoD0(0);
            pressed_buttons &= 0x70;
            if (pressed_buttons) {
                while (MovePadInputIntoD0(0) & 0x70 & pressed_buttons)
                    AnimateShipScreenBackground(0);
                break;
            }
        }
        if (player_number == PLAYER_EARL || player_number == 2) {
            pressed_buttons = MovePadInputIntoD0(1);
            pressed_buttons &= 0x70;
            if (pressed_buttons) {
                while (MovePadInputIntoD0(1) & 0x70 & pressed_buttons)
                    AnimateShipScreenBackground(0);
                break;
            }
        }
        AnimateShipScreenBackground(0);
    }
}
