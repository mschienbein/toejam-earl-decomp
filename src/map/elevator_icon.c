#include "elevator.h"
#include "sprites.h"
#include "hardware.h"
#include "ram.h"
#include "assets.h"
#include "map_calls.h"
#include "romcalls.h"
/* Upload and position the first elevator icon on the requested level,
 * except for elevators in state 13. */
void Map_DrawElevator(level, sprite)
short level;
struct hardware_sprite *sprite;
{
    short elevator_number, world_x, world_y, tile;
    struct elevator_record *elevator;

    for (elevator_number = 0; elevator_number <= 1; ++elevator_number) {
        elevator = &ELEVATOR_LOCATION[elevator_number][0];
        if (elevator->level != level || elevator->state == 13)
            continue;
        world_x = elevator->x;
        world_x -= 128;
        world_y = elevator->y;
        world_y -= 64;
        tile = sprite->tile & 0x7ff;
        VDP_COMMAND = 0x40000000 + (((tile << 5) & 0x3fff) << 16) +
                      (((tile << 5) & 0xc000) >> 14);
        MoveDataToVRAM(ElevatorMapIcon, 16);
        sprite->tile |= 0x6000;
        sprite->x = world_x / 8 + 130;
        sprite->y = world_y / 8 + 128;
        return;
    }
}
