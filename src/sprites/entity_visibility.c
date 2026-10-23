#include "entities.h"
#include "engine_calls.h"
#include "ram_camera.h"
#include "ram_map.h"
#include "ram_player.h"
#include "ram_sprites.h"
#include "ram_system.h"
#include "ram_ui.h"
#include "sprite_calls.h"

/* Test the entity's level and bounds against the chosen camera rectangle. */
short IsEntityVisibleInViewport(entity, viewport)
struct object_state *entity;
short viewport;
{
    short relative_x, relative_y, viewport_height, viewport_width;

    viewport_height = 112;
    viewport_width = 320;
    if (viewport == 0) {
        if (gPlayerInventoryOpen[PLAYER_TOEJAM] || entity->level != gPlayer1MapLevel)
            return 0;
        relative_x = entity->x - gCamera1PendingX;
        relative_y = entity->y - gCamera1PendingY;
        if (!gSplitScreenEnabled)
            viewport_height = 224;
    } else {
        if (gSplitScreenEnabled) {
            if (gPlayer2InventoryOpen || entity->level != gPlayer2MapLevel)
                return 0;
        } else
            return 0;
        relative_x = entity->x - gCamera2PendingX;
        relative_y = entity->y - gCamera2PendingY;
    }
    if (entity->width + relative_x >= 0 &&
        viewport_width >= relative_x - entity->width &&
        relative_y + 10 > 0 &&
        viewport_height >= relative_y - entity->height_pixels - entity->height)
        return 1;
    return 0;
}

/* Add or release a camera-specific sprite when visibility changes. */
void UpdateEntityViewportSprites(entity_number)
int entity_number;
{
    char add_primary = 0, add_secondary = 0;
    struct object_state *entity;

    entity = &gPlayerInfo[entity_number];
    if (entity->draw_flags & ENTITY_ACTIVE) {
        if (IsEntityVisibleInViewport(entity, 0)) {
            if (!IsSpeechBubbleEntity(entity_number) && !(entity->draw_flags & ENTITY_PRIMARY_VIEW))
                add_primary = 1;
        } else if (entity->draw_flags & ENTITY_PRIMARY_VIEW)
            ReleaseEntityViewportSprites(entity, 0);
        if (IsEntityVisibleInViewport(entity, 1)) {
            if (!IsSpeechBubbleEntity(entity_number) && !(entity->draw_flags & ENTITY_SECONDARY_VIEW))
                add_secondary = 1;
        } else if (entity->draw_flags & ENTITY_SECONDARY_VIEW)
            ReleaseEntityViewportSprites(entity, 1);
        if (add_primary) {
            if (entity_number == PLAYER_TOEJAM && (entity->draw_flags & ENTITY_SECONDARY_VIEW))
                ReleaseEntityViewportSprites(entity, 1);
            if (entity_number != PLAYER_EARL || !(entity->draw_flags & ENTITY_SECONDARY_VIEW)) {
                entity->sprite_ready = 0;
                AllocateEntityViewportSprites(entity, 0);
            }
        }
        if (add_secondary) {
            if (entity_number == PLAYER_EARL && (entity->draw_flags & ENTITY_PRIMARY_VIEW))
                ReleaseEntityViewportSprites(entity, 0);
            if (entity_number != PLAYER_TOEJAM || !(entity->draw_flags & ENTITY_PRIMARY_VIEW)) {
                entity->sprite_ready = 0;
                AllocateEntityViewportSprites(entity, 1);
            }
        }
    }
}

/* Refresh viewport allocation for 26 gameplay entities, or all 40 on the final level. */
void UpdateAllEntityViewportSprites()
{
    short entity_number, entity_count;

    if (gPlayer1MapLevel == LEVEL_FINAL) {
        entity_count = ENTITY_SLOT_COUNT;
        gInventoryDrawListHead = 0;
    } else
        entity_count = GAMEPLAY_ENTITY_SLOT_COUNT;
    for (entity_number = 0; entity_number < entity_count; ++entity_number)
        UpdateEntityViewportSprites(entity_number);
}
