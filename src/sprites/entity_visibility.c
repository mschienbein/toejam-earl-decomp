#define TJE_LOCAL_GPLAYERINFO
#include "entities.h"
#include "ram.h"
#include "engine_calls.h"
#include "sprite_calls.h"

/* Compiler bound qualification: the compatible incomplete-array view
 * preserves the observed index arithmetic. The complete bound changes width;
 * a pointer cast changes address-register allocation. Historical declaration
 * spelling is unknown [INFERENCE]. */
extern struct object_state gPlayerInfo[];

/* Test the entity's level and bounds against the chosen camera rectangle. */
short func_00bdd8(entity, viewport)
struct object_state *entity;
short viewport;
{
    short relative_x, relative_y, viewport_height, viewport_width;

    viewport_height = 112;
    viewport_width = 320;
    if (viewport == 0) {
        if (P1_MENU_OPEN[0] || entity->level != gLevel912C)
            return 0;
        relative_x = entity->x - TOEJAM_CAMERA_NEWX;
        relative_y = entity->y - TOEJAM_CAMERA_NEWY;
        if (!SPLIT_SCREEN)
            viewport_height = 224;
    } else {
        if (SPLIT_SCREEN) {
            if (HBLANK_FLAG_1 || entity->level != gLevel912E)
                return 0;
        } else
            return 0;
        relative_x = entity->x - EARL_CAMERA_NEWX;
        relative_y = entity->y - EARL_CAMERA_NEWY;
    }
    if (entity->width + relative_x >= 0 &&
        viewport_width >= relative_x - entity->width &&
        relative_y + 10 > 0 &&
        viewport_height >= relative_y - entity->height_pixels - entity->height)
        return 1;
    return 0;
}

/* Add or release a camera-specific sprite when visibility changes. */
void func_00beb2(entity_number)
int entity_number;
{
    char add_primary = 0, add_secondary = 0;
    struct object_state *entity;

    entity = &gPlayerInfo[entity_number];
    if (entity->draw_flags & 1) {
        if (func_00bdd8(entity, 0)) {
            if (!func_01dee8(entity_number) && !(entity->draw_flags & 2))
                add_primary = 1;
        } else if (entity->draw_flags & 2)
            OpeningPresents7(entity, 0);
        if (func_00bdd8(entity, 1)) {
            if (!func_01dee8(entity_number) && !(entity->draw_flags & 4))
                add_secondary = 1;
        } else if (entity->draw_flags & 4)
            OpeningPresents7(entity, 1);
        if (add_primary) {
            if (entity_number == 0 && (entity->draw_flags & 4))
                OpeningPresents7(entity, 1);
            if (entity_number != 1 || !(entity->draw_flags & 4)) {
                entity->sprite_ready = 0;
                func_00d776(entity, 0);
            }
        }
        if (add_secondary) {
            if (entity_number == 1 && (entity->draw_flags & 2))
                OpeningPresents7(entity, 0);
            if (entity_number != 0 || !(entity->draw_flags & 2)) {
                entity->sprite_ready = 0;
                func_00d776(entity, 1);
            }
        }
    }
}

void func_00bfc6()
{
    short entity_number, entity_count;

    if (gLevel912C == 26) {
        entity_count = 40;
        ram_ffd93c = 0;
    } else
        entity_count = 26;
    for (entity_number = 0; entity_number < entity_count; ++entity_number)
        func_00beb2(entity_number);
}
