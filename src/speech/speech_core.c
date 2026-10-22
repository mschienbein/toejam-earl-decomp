#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_ui.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_speech.h"
#include "ram_system.h"
#include "speech.h"
#include "sprite_calls.h"

/* Explicit integral value promotions preserve the observed int-valued call
 * uses with correct shared return declarations; exact historical spelling
 * remains unknown [INFERENCE]. */

/* Set the lifetime override consumed by the next allocated speech bubble. */
void SetNextSpeechLifetime(lifetime)
short lifetime;
{
    gNextSpeechLifetimeOverride = lifetime;
}

/* Allocate one of four speech bubbles, replacing the oldest when necessary.
 * Attached bubbles retain their speaker so later updates can follow it. */
short GenerateSpeech(speaker, text, replace_attached)
short speaker;
char *text;
char replace_attached;
{
    struct object_state *source, *bubble;
    short slot, entity_number, index, length, viewport;

    source = &gPlayerInfo[speaker];
    if (!(source->draw_flags & ENTITY_EITHER_VIEW))
        return 0;
    if (speaker <= 1 && source->sprite_state == 117)
        return 0;
    if (replace_attached) {
        for (slot = 0; slot < 4; ++slot) {
            if (entity_number = gSpeechBubbleEntities[slot]) {
                bubble = &gPlayerInfo[entity_number];
                if ((bubble->behavior_flags & ENTITY_ATTACHED) && bubble->owner == speaker) {
                    FreeEntity(entity_number);
                    gSpeechBubbleEntities[slot] = 0;
                }
            }
        }
    }
    for (slot = 0; slot < 4; ++slot)
        if (!gSpeechBubbleEntities[slot])
            break;
    if (slot == 4) {
        slot = 0;
        for (index = 1; index < 4; ++index)
            if (gSpeechBubbleLifetimes[index] < gSpeechBubbleLifetimes[slot])
                slot = index;
        FreeEntity(gSpeechBubbleEntities[slot]);
    }
    for (length = 0; text[length]; ++length)
        ;
    if (length > 12)
        length = 12;
    if (gSpeechBubbleEntities[slot] = (int)AllocateEntity()) {
        bubble = &gPlayerInfo[gSpeechBubbleEntities[slot]];
        bubble->x = source->x;
        bubble->y = source->y + 15;
        bubble->height = source->height + 15;
        bubble->level = source->level;
        bubble->width = length << 3;
        bubble->height_pixels = 16;
        bubble->sprite_count = 4;
        bubble->tile_count = 13;
        if (gLevel912C < 26)
            bubble->tile = VDP_TILE_PRIORITY;
        bubble->current_frame = &RuntimeTextMetasprite_0AFADC;
        if (source->draw_flags & ENTITY_PRIMARY_VIEW) {
            AllocateEntityViewportSprites(bubble, 0);
            viewport = 0;
        } else {
            AllocateEntityViewportSprites(bubble, 1);
            viewport = 1;
        }
        if (!(bubble->draw_flags & ENTITY_EITHER_VIEW)) {
            FreeEntity(gSpeechBubbleEntities[slot]);
            gSpeechBubbleEntities[slot] = 0;
            return 0;
        }
        bubble->draw_flags |= ENTITY_SKIP_PHYSICS;
        while (VBLANK_FLAG_2)
            ;
        QueueSpeechTextTiles(text, length, bubble->tile & VDP_TILE_INDEX_MASK);
        PrepareEntitySpriteFrame(bubble, &RuntimeTextMetasprite_0AFADC, 1, viewport);
        bubble->sprite_ready = 1;
        if (replace_attached) {
            bubble->behavior_flags |= ENTITY_ATTACHED;
            bubble->owner = speaker;
            bubble->attachment_y_or_latch = 15;
            bubble->attachment_height = 15;
        }
        if (gNextSpeechLifetimeOverride) {
            gSpeechBubbleLifetimes[slot] = gNextSpeechLifetimeOverrideLowByte;
            gNextSpeechLifetimeOverride = 0;
        } else
            gSpeechBubbleLifetimes[slot] = 40;
        return gSpeechBubbleEntities[slot];
    }
    return 0;
}

/* Release active speech objects and discard both pending replies. */
void ClearSpeechBubblesAndReplies()
{
    short slot;

    for (slot = 0; slot < 4; ++slot) {
        if (gSpeechBubbleEntities[slot]) {
            FreeEntity(gSpeechBubbleEntities[slot]);
            gSpeechBubbleEntities[slot] = 0;
        }
    }
    gDelayedSpeechReplyTimers[0] = 0;
    gSecondSpeechReplyTimer = 0;
    gReunionDialoguePending = 0;
}

/* Report either an existing bubble or a pending reply for this speaker. */
int HasSpeakerSpeech(speaker)
short speaker;
{
    short slot;
    struct object_state *bubble;

    for (slot = 0; slot < 4; ++slot) {
        if (gSpeechBubbleEntities[slot]) {
            bubble = &gPlayerInfo[gSpeechBubbleEntities[slot]];
            if ((bubble->draw_flags & ENTITY_SKIP_PHYSICS) && bubble->owner == speaker)
                return 1;
        }
    }
    if ((gDelayedSpeechReplyTimers[0] && gDelayedSpeechReplySpeakers[0] == speaker) ||
        (gSecondSpeechReplyTimer && gSecondDelayedSpeechReplySpeaker == speaker))
        return 1;
    return 0;
}
