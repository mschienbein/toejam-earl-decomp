#include "hardware.h"
#define TJE_LOCAL_GSPRITETILEUPLOADBUFFER

#include "entities.h"
#include "sprites.h"
#include "assets_ui.h"
#include "cutscene_text_calls.h"
#include "ram_cutscene.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "sprite_calls.h"
#include "ui_calls.h"

/* MATCHING QUALIFICATION: this compatible incomplete-array declaration
 * preserves long-index/offset instructions. The shared complete [8192]
 * bound changes them; historical source spelling is unknown [INFERENCE]. */
extern char gSpriteTileUploadBuffer[];

/* Both runtime text objects contain the common four-byte frame prefix and
 * eight ten-byte pieces (84 bytes each), not a fixed generic frame tail.
 * MATCHING QUALIFICATION: the explicit (int)AllocateEntity() result views
 * retain the observed call-result width with the shared short declaration.
 * Text slot bytes still store the short allocator result after promotion. */

/* Remap both packed font-pixel nibbles into the text sprite's palette. */
void CutsceneAppendRemappedFontTiles(source, tile_count)
char *source;
short tile_count;
{
    short byte_count;
    unsigned char nibble, pixels;
    char *destination;

    destination = &gSpriteTileUploadBuffer[gSpriteTileBufferOffset];
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
    gSpriteTileBufferOffset += tile_count << 5;
}

/* Center a palette-remapped line in a 32-tile transfer and queue it. */
void CutsceneQueueCenteredRemappedTextLine(text, length, tile_base)
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
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles, 1);
    for (index = 0; index < length; ++index) {
        character = text[index];
        glyph = TextMapCharacterToGlyph(character);
        if (glyph == 27)
            ++glyph;
        CutsceneAppendRemappedFontTiles(RuntimeTextFontTiles + (glyph << 5), 1);
    }
    if (length & 1)
        ++padding;
    for (index = 0; index < padding; ++index)
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles, 1);
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = tile_base;
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 32;
}

/* The alternate text line copies the font palette without remapping. */
void CutsceneQueueCenteredOriginalPaletteTextLine(text, length, tile_base)
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
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles, 1);
    for (index = 0; index < length; ++index) {
        character = text[index];
        glyph = TextMapCharacterToGlyph(character);
        if (glyph == 27)
            ++glyph;
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles + (glyph << 5), 1);
    }
    if (length & 1)
        ++padding;
    for (index = 0; index < padding; ++index)
        AppendSpriteTilesToBuffer(RuntimeTextFontTiles, 1);
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = tile_base;
    gSpriteTileUploadRuns[gSpriteTileUploadQueueWordCount++] = 32;
}

/* Release both remembered text entities. */
void CutsceneReleaseTextEntities()
{
    if (gCutsceneFirstTextEntity) {
        FreeEntity(gCutsceneFirstTextEntity);
        gCutsceneFirstTextEntity = 0;
    }
    if (gCutsceneSecondTextEntity) {
        FreeEntity(gCutsceneSecondTextEntity);
        gCutsceneSecondTextEntity = 0;
    }
}

/* Replace existing text with a first line, capped at 32 glyphs. */
void CutsceneTextLoad(text, x, y)
char *text;
short x, y;
{
    short length;
    struct object_state *entity;

    CutsceneReleaseTextEntities();
    for (length = 0; text[length]; ++length)
        ;
    if (length > 32)
        length = 32;
    if (gCutsceneFirstTextEntity = (int)AllocateEntity()) {
        entity = &gPlayerInfo[gCutsceneFirstTextEntity];
        entity->x = x;
        entity->y = y + 100;
        entity->height = 100;
        entity->width = length << 3;
        entity->height_pixels = 8;
        entity->sprite_count = 8;
        entity->tile_count = 32;
        entity->tile = VDP_TILE_PRIORITY;
        entity->current_frame = &CutsceneRemappedTextFrame;
        AllocateEntityViewportSprites(entity, VIEWPORT_PRIMARY);
        CutsceneQueueCenteredRemappedTextLine(text, length, entity->tile & VDP_TILE_INDEX_MASK);
        PrepareEntitySpriteFrame(entity, &CutsceneRemappedTextFrame, 1, VIEWPORT_PRIMARY);
        entity->sprite_ready = 1;
    }
}

/* Add a second line without discarding the first text entity. */
void CutsceneLoadSecondTextLine(text, x, y)
char *text;
short x, y;
{
    short length;
    struct object_state *entity;

    for (length = 0; text[length]; ++length)
        ;
    if (length > 32)
        length = 32;
    if (gCutsceneSecondTextEntity = (int)AllocateEntity()) {
        entity = &gPlayerInfo[gCutsceneSecondTextEntity];
        entity->x = x;
        entity->y = y + 100;
        entity->height = 100;
        entity->width = length << 3;
        entity->height_pixels = 8;
        entity->sprite_count = 8;
        entity->tile_count = 32;
        entity->tile = VDP_TILE_PRIORITY;
        entity->current_frame = &CutsceneRemappedTextFrame;
        AllocateEntityViewportSprites(entity, VIEWPORT_PRIMARY);
        CutsceneQueueCenteredRemappedTextLine(text, length, entity->tile & VDP_TILE_INDEX_MASK);
        PrepareEntitySpriteFrame(entity, &CutsceneRemappedTextFrame, 1, VIEWPORT_PRIMARY);
        entity->sprite_ready = 1;
    }
}

/* Replace existing text using the alternate, zero-elevation metasprite. */
void CutsceneLoadOriginalPaletteTextLine(text, x, y)
char *text;
short x, y;
{
    short length;
    struct object_state *entity;

    CutsceneReleaseTextEntities();
    for (length = 0; text[length]; ++length)
        ;
    if (length > 32)
        length = 32;
    if (gCutsceneFirstTextEntity = (int)AllocateEntity()) {
        entity = &gPlayerInfo[gCutsceneFirstTextEntity];
        entity->x = x;
        entity->y = y;
        entity->height = 0;
        entity->width = length << 3;
        entity->height_pixels = 8;
        entity->sprite_count = 8;
        entity->tile_count = 32;
        entity->tile = VDP_TILE_PRIORITY;
        entity->current_frame = &CutsceneOriginalPaletteTextFrame;
        AllocateEntityViewportSprites(entity, VIEWPORT_PRIMARY);
        CutsceneQueueCenteredOriginalPaletteTextLine(text, length, entity->tile & VDP_TILE_INDEX_MASK);
        PrepareEntitySpriteFrame(entity, &CutsceneOriginalPaletteTextFrame, 1, VIEWPORT_PRIMARY);
        entity->sprite_ready = 1;
    }
}