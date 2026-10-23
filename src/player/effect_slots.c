#include "psg_effects.h"
#include "hardware.h"
#include "entities.h"
#include "sprites.h"
#include "assets_sprites.h"
#include "audio_calls.h"
#include "player.h"
#include "ram_effects.h"
#include "ram_player.h"
#include "sprite_calls.h"

/* MATCHING QUALIFICATION: the explicit (int)AllocateEntity() result view
 * retains the observed call-result width with the shared short declaration.
 * Exact historical spelling remains unknown [INFERENCE]. */

/* Report whether either of the two effect slots is available. */
int EntityHasFreeTimedEffectSlot()
{
    if (!gTimedEffectEntities[0] || !gTimedEffectEntities[1])
        return 1;
    else
        return 0;
}

/* Claim the first available effect slot and start its animation. */
int EntitySpawnTimedEffect(x, y, level)
short x, y, level;
{
    short effect_slot;
    struct object_state *effect;
    struct timed_animation *animation;

    if (!gTimedEffectEntities[0])
        effect_slot = 0;
    else if (!gTimedEffectEntities[1])
        effect_slot = 1;
    else
        return 1;
    if (gTimedEffectEntities[effect_slot] = (int)AllocateEntity()) {
        effect = &gPlayerInfo[gTimedEffectEntities[effect_slot]];
        effect->x = x;
        effect->y = y;
        effect->height = 5;
        effect->level = level;
        effect->width = 32;
        effect->height_pixels = 32;
        effect->sprite_count = 1;
        effect->tile_count = 16;
        effect->tile |= VDP_TILE_PRIORITY;
        animation = &TimedEntityEffectAnimation;
        effect->sequence = animation->sequence;
        effect->frames = animation->frames;
        effect->frame_phase = animation->period;
        effect->frame_duration = 1;
        effect->frame = SPRITE_FRAME_BEFORE_FIRST;
        PlaySound(PSG_TWO_NOTES, AUDIO_PRIORITY_NORMAL);
        return 0;
    }
    return 1;
}

/* Release a slot after its object's animation has stopped. */
void EntityReleaseFinishedTimedEffect(effect_slot)
short effect_slot;
{
    if (gTimedEffectEntities[effect_slot] &&
        !gPlayerInfo[gTimedEffectEntities[effect_slot]].frame_duration) {
        FreeEntity(gTimedEffectEntities[effect_slot]);
        gTimedEffectEntities[effect_slot] = 0;
    }
}
