#include "sprites.h"
#include "hardware_calls.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "sprite_calls.h"

/* Transfer the sprite table unless suppressed, then the queued tile runs. */
void UploadQueuedSpriteGraphics(force, tiles_only)
short force, tiles_only;
{
    unsigned short *tile_source;
    short queue_index;
    unsigned short word_count;

    if (!gSpriteGraphicsUploadPending && !force && !tiles_only)
        return;
    if (!tiles_only) {
        DmaCopyWordsToVram(gHardwareSpriteTable, 0xf800, 320);
        gPrimaryViewportUploadedLink = gHardwareSpriteTable[23].link;
        gSecondaryViewportUploadedLink = gHardwareSpriteTable[24].link;
    }
    tile_source = (unsigned short *)gSpriteTileUploadBuffer;
    for (queue_index = 0; queue_index < gSpriteTileUploadQueueWordCount; queue_index += 2) {
        word_count = gSpriteTileUploadRuns[queue_index + 1] << 4;
        DmaCopyWordsToVram(tile_source, gSpriteTileUploadRuns[queue_index] << 5, word_count);
        tile_source += word_count;
    }
    gSpriteTileBufferOffset = 0;
    gSpriteTileUploadQueueWordCount = 0;
    gSpriteGraphicsUploadPending = 0;
}
