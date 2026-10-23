#include "elevator.h"
#include "elevator_states.h"
#include "sprites.h"
#include "hardware.h"
#include "assets_map.h"
#include "hardware_calls.h"
#include "map_calls.h"
#include "ram_elevator.h"

/* Upload and position the first elevator icon on the requested level,
 * except for elevators blocked by an arrival on the next level. */
void Map_DrawElevator(level, sprite)
short level;
struct hardware_sprite *sprite;
{
    short elevator_number, world_x, world_y, tile;
    struct elevator_record *elevator;

    for (elevator_number = 0; elevator_number <= 1; ++elevator_number) {
        elevator = &gElevatorRecords[elevator_number][0];
        if (elevator->level != level || elevator->state == ELEVATOR_STATE_BLOCKED_BY_ARRIVAL)
            continue;
        world_x = elevator->x;
        world_x -= 128;
        world_y = elevator->y;
        world_y -= 64;
        tile = sprite->tile & VDP_TILE_INDEX_MASK;
        VDP_COMMAND = VDP_WRITE_VRAM_BASE + (((tile << 5) & 0x3fff) << 16) +
                      (((tile << 5) & 0xc000) >> 14);
        MoveDataToVRAM(ElevatorMapIcon, 16);
        sprite->tile |= VDP_TILE_PALETTE_3;
        sprite->x = world_x / 8 + 130;
        sprite->y = world_y / 8 + 128;
        return;
    }
}
