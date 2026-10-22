#include "sprites.h"
#include "ram.h"
#include "sprite_calls.h"
#include "romcalls.h"

/* Transfer the sprite table unless suppressed, then the queued tile runs. */
void SpriteLoadingStuff(force, tiles_only)
short force, tiles_only;
{
    unsigned short *tile_source;
    short queue_index;
    unsigned short word_count;

    if (!VBLANK_FLAG_2 && !force && !tiles_only)
        return;
    if (!tiles_only) {
        VDPWritesRoutine1(ram_ffb65a, 0xf800, 320);
        ram_ffd932 = ram_ffb65a[23].link;
        ram_ffd934 = ram_ffb65a[24].link;
    }
    tile_source = (unsigned short *)ram_ffb8da;
    for (queue_index = 0; queue_index < ram_ffd92e; queue_index += 2) {
        word_count = ram_ffd8dc[queue_index + 1] << 4;
        VDPWritesRoutine1(tile_source, ram_ffd8dc[queue_index] << 5, word_count);
        tile_source += word_count;
    }
    SPRITE_STATE_TABLE = 0;
    ram_ffd92e = 0;
    VBLANK_FLAG_2 = 0;
}
