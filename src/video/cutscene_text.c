#define TJE_LOCAL_RAM_FFB8DA

#include "entities.h"
#include "sprites.h"
#include "assets_ui.h"
#include "cutscene_text_calls.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* Compatible incomplete-array compiler view. The actual complete shared
 * declaration changes the observed instructions; historical source spelling
 * is unknown [INFERENCE]. */
extern char ram_ffb8da[];

/* Both runtime text objects contain the common four-byte frame prefix and
 * eight ten-byte pieces (84 bytes each), not a fixed generic frame tail.
 * Text slot bytes retain the short allocator result after int promotion. */

/* Remap both packed font-pixel nibbles into the text sprite's palette. */
void func_027bc2(source, tile_count)
char *source;
short tile_count;
{
    short byte_count;
    unsigned char nibble, pixels;
    char *destination;

    destination = &ram_ffb8da[SPRITE_STATE_TABLE];
    for (byte_count = tile_count << 5; byte_count > 0; --byte_count) {
        pixels = 0;
        nibble = *source & 15;
        if (nibble == 11)
            pixels = 1;
        else if (nibble == 10)
            pixels = 2;
        else if (nibble == 9)
            pixels = 2;
        else if (nibble == 8)
            pixels = 15;
        nibble = *source & 0xf0;
        if (nibble == 0xb0)
            pixels |= 0x10;
        else if (nibble == 0xa0)
            pixels |= 0x20;
        else if (nibble == 0x90)
            pixels |= 0x20;
        else if (nibble == 0x80)
            pixels |= 0xf0;
        *destination++ = pixels;
        ++source;
    }
    SPRITE_STATE_TABLE += tile_count << 5;
}

/* Center a palette-remapped line in a 32-tile transfer and queue it. */
void func_027c64(text, length, tile_base)
char text[];
short length, tile_base;
{
    char glyph;
    short index, padding;
    int character;
    long free_tiles;

    free_tiles = 32 - length;
    padding = free_tiles / 2;
    for (index = 0; index < padding; ++index)
        ClearLOC_00FFB8DA(RuntimeTextFontTiles, 1);
    for (index = 0; index < length; ++index) {
        character = text[index];
        glyph = func_009e2e(character);
        if (glyph == 27)
            ++glyph;
        func_027bc2(RuntimeTextFontTiles + (glyph << 5), 1);
    }
    if (length & 1)
        ++padding;
    for (index = 0; index < padding; ++index)
        ClearLOC_00FFB8DA(RuntimeTextFontTiles, 1);
    ram_ffd8dc[ram_ffd92e++] = tile_base;
    ram_ffd8dc[ram_ffd92e++] = 32;
}

/* The alternate text line copies the font palette without remapping. */
void func_027d52(text, length, tile_base)
char text[];
short length, tile_base;
{
    char glyph;
    short index, padding;
    int character;
    long free_tiles;

    free_tiles = 32 - length;
    padding = free_tiles / 2;
    for (index = 0; index < padding; ++index)
        ClearLOC_00FFB8DA(RuntimeTextFontTiles, 1);
    for (index = 0; index < length; ++index) {
        character = text[index];
        glyph = func_009e2e(character);
        if (glyph == 27)
            ++glyph;
        ClearLOC_00FFB8DA(RuntimeTextFontTiles + (glyph << 5), 1);
    }
    if (length & 1)
        ++padding;
    for (index = 0; index < padding; ++index)
        ClearLOC_00FFB8DA(RuntimeTextFontTiles, 1);
    ram_ffd8dc[ram_ffd92e++] = tile_base;
    ram_ffd8dc[ram_ffd92e++] = 32;
}

/* Release both remembered text entities. */
void func_027e42()
{
    if (ram_ffe3ca) {
        func_00d970(ram_ffe3ca);
        ram_ffe3ca = 0;
    }
    if (ram_ffe3cc) {
        func_00d970(ram_ffe3cc);
        ram_ffe3cc = 0;
    }
}

/* Replace existing text with a first line, capped at 32 glyphs. */
void CutsceneTextLoad(text, x, y)
char *text;
short x, y;
{
    short length;
    struct object_state *entity;

    func_027e42();
    for (length = 0; text[length]; ++length)
        ;
    if (length > 32)
        length = 32;
    if (ram_ffe3ca = (int)func_00d904()) {
        entity = &gPlayerInfo[ram_ffe3ca];
        entity->x = x;
        entity->y = y + 100;
        entity->height = 100;
        entity->width = length << 3;
        entity->height_pixels = 8;
        entity->sprite_count = 8;
        entity->tile_count = 32;
        entity->tile = 0x8000;
        entity->current_frame = &RuntimeTextMetasprite_0AFB08;
        func_00d776(entity, 0);
        func_027c64(text, length, entity->tile & 0x7ff);
        func_00c920(entity, &RuntimeTextMetasprite_0AFB08, 1, 0);
        entity->sprite_ready = 1;
    }
}

/* Add a second line without discarding the first text entity. */
void func_027f5e(text, x, y)
char *text;
short x, y;
{
    short length;
    struct object_state *entity;

    for (length = 0; text[length]; ++length)
        ;
    if (length > 32)
        length = 32;
    if (ram_ffe3cc = (int)func_00d904()) {
        entity = &gPlayerInfo[ram_ffe3cc];
        entity->x = x;
        entity->y = y + 100;
        entity->height = 100;
        entity->width = length << 3;
        entity->height_pixels = 8;
        entity->sprite_count = 8;
        entity->tile_count = 32;
        entity->tile = 0x8000;
        entity->current_frame = &RuntimeTextMetasprite_0AFB08;
        func_00d776(entity, 0);
        func_027c64(text, length, entity->tile & 0x7ff);
        func_00c920(entity, &RuntimeTextMetasprite_0AFB08, 1, 0);
        entity->sprite_ready = 1;
    }
}

/* Replace existing text using the alternate, zero-elevation metasprite. */
void func_028034(text, x, y)
char *text;
short x, y;
{
    short length;
    struct object_state *entity;

    func_027e42();
    for (length = 0; text[length]; ++length)
        ;
    if (length > 32)
        length = 32;
    if (ram_ffe3ca = (int)func_00d904()) {
        entity = &gPlayerInfo[ram_ffe3ca];
        entity->x = x;
        entity->y = y;
        entity->height = 0;
        entity->width = length << 3;
        entity->height_pixels = 8;
        entity->sprite_count = 8;
        entity->tile_count = 32;
        entity->tile = 0x8000;
        entity->current_frame = &RuntimeTextMetasprite_0AFB5C;
        func_00d776(entity, 0);
        func_027d52(text, length, entity->tile & 0x7ff);
        func_00c920(entity, &RuntimeTextMetasprite_0AFB5C, 1, 0);
        entity->sprite_ready = 1;
    }
}