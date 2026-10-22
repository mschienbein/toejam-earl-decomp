#include "hardware.h"
#include "ram.h"
#include "core_calls.h"
/* Write the VDP register initialization sequence, then enter game setup.
 * ResetVDP also matches plain C exactly despite its previous assembly label. */

void ResetVDP()
{
    VDP_CONTROL_WORD = 0x8004;
    VDP_CONTROL_WORD = 0x8144;
    VDP_CONTROL_WORD = 0x8228;
    VDP_CONTROL_WORD = 0x8334;
    VDP_CONTROL_WORD = 0x8407;
    VDP_CONTROL_WORD = 0x857c;
    VDP_CONTROL_WORD = 0x8700;
    VDP_CONTROL_WORD = 0x8a00;
    VDP_CONTROL_WORD = 0x8b00;
    VDP_CONTROL_WORD = 0x8c89;
    VDP_CONTROL_WORD = 0x8d3f;
    VDP_CONTROL_WORD = 0x8f02;
    VDP_CONTROL_WORD = 0x9001;
    VDP_CONTROL_WORD = 0x9100;
    VDP_CONTROL_WORD = 0x9200;
    VDP_CONTROL_WORD = 0x9302;
    VDP_CONTROL_WORD = 0x9400;
    VDP_CONTROL_WORD = 0x9500;
    VDP_CONTROL_WORD = 0x9600;
    VDP_CONTROL_WORD = 0x9700;
}

void VDPInitialisation()
{
    ResetVDP();
    FurtherInitialisation();
}

/* Select the physical controller and, when requested, exchange B and C. */
long MovePadInputIntoD0(player)
short player;
{
    long input, mapped;
    if (player == 0) {
        if (!ram_ff801c) input = P1_PAD_INPUT;
        else input = P2_PAD_INPUT;
    } else {
        if (!ram_ff801c) input = P2_PAD_INPUT;
        else input = P1_PAD_INPUT;
    }
    if (ram_ff8030) {
        mapped = input & 0x9f;
        if (input & 0x40)
            mapped |= 0x20;
        if (input & 0x20)
            mapped |= 0x40;
        return mapped;
    }
    return input;
}
