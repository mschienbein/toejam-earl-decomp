#include "ram_sprites.h"
#include "sprite_calls.h"

/* Copy complete 32-byte tiles to the next transfer-buffer position. */
void ClearLOC_00FFB8DA(source, tile_count)
long *source;
short tile_count;
{
    long *destination;
    short longword_count;

    destination = (long *)&ram_ffb8da[SPRITE_STATE_TABLE];
    longword_count = tile_count << 3;
    while (longword_count > 0) {
        *destination++ = *source++;
        --longword_count;
    }
    SPRITE_STATE_TABLE += tile_count << 5;
}
