#include "sprites.h"
#include "ram_sprites.h"
#include "sprite_calls.h"

/* Remove the packed rows below the supplied vertical cutoff. */
short func_00c85a(piece, cutoff)
struct render_piece *piece;
short cutoff;
{
    short column, row, row_width, first_hidden;
    long *pixels;

    row_width = piece->height * 8;
    row = piece->y - 2 - cutoff;
    if (row >= 0)
        return 1;
    if (row_width + row <= 0)
        return 0;
    first_hidden = -row;
    for (row = 0; row < piece->width; ++row) {
        pixels = (long *)&ram_ffb8da[(row * row_width * 4 +
                 SPRITE_STATE_TABLE) + first_hidden * 4];
        *pixels++ &= 0x0f000ff0L;
        for (column = first_hidden + 1; column < row_width; ++column)
            *pixels++ = 0;
    }
    return 0;
}

/* Discard pixels whose packed byte is an all-ones nibble mask. */
void func_00c8ec(byte_count)
unsigned short byte_count;
{
    char *pixels;
    char packed_pixels;

    pixels = &ram_ffb8da[SPRITE_STATE_TABLE];
    while (byte_count > 0) {
        packed_pixels = *pixels;
        if (packed_pixels == -1 || packed_pixels == (char)240 || packed_pixels == 15)
            packed_pixels = 0;
        *pixels++ = packed_pixels;
        --byte_count;
    }
}
