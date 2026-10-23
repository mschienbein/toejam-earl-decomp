#include "hardware.h"
#include "player_animation.h"
#include "entities.h"
#include "sprites.h"
#include "assets_presents.h"
#include "assets_sprites.h"
#include "player.h"
#include "ram_player.h"
#include "ram_sprites.h"

/* Initialize the hardware sprite links, the two players, and the two
 * selector-plus-six-present record chains beginning at record 26. */
void SetupPlayer()
{
    short sprite_number, present_number;
    unsigned short tile;
    struct object_state *entity;

    gHardwareSpriteTable[0].link = 11;
    for (sprite_number = 0; sprite_number < 10; ++sprite_number) {
        tile = 0x85a9;
        if (sprite_number & 1)
            tile |= (VDP_TILE_HORIZONTAL_FLIP | VDP_TILE_VERTICAL_FLIP);
        gHardwareSpriteTable[sprite_number + 1].x = (sprite_number << 5) + 128;
        gHardwareSpriteTable[sprite_number + 1].y = 236;
        gHardwareSpriteTable[sprite_number + 1].size = VDP_SPRITE_SIZE_4X1;
        gHardwareSpriteTable[sprite_number + 1].link = sprite_number + 2;
        gHardwareSpriteTable[sprite_number + 1].tile = tile;
    }
    for (sprite_number = 11; sprite_number < 23; ++sprite_number)
        gHardwareSpriteTable[sprite_number].link = sprite_number + 1;
    gHardwareSpriteTable[sprite_number].link = 23;
    gHardwareSpriteTable[23].link = 79;
    gPrimaryViewportUploadedLink = 79;
    gHardwareSpriteTable[24].link = 79;
    gSecondaryViewportUploadedLink = 79;
    gHardwareSpriteTable[79].link = 0;
    entity = &gPlayerInfo[PLAYER_TOEJAM];
    entity->process = Player1ProcessTerrainMotion;
    entity->sequence = PlayerAnimationDescriptors[PLAYER_ANIM_IDLE].sequence;
    entity->frames = PlayerAnimationDescriptors[PLAYER_ANIM_IDLE].frames;
    entity->frame_phase = 10;
    entity->frame_duration = 1;
    entity->width = 24;
    entity->height_pixels = 32;
    entity->sprite_count = 3;
    entity->tile_count = 40;
    entity->tile = VDP_TILE_PRIORITY;
    entity->draw_flags = ENTITY_ACTIVE;
    entity->behavior_flags = ENTITY_COLLIDABLE;
    entity = &gPlayerInfo[PLAYER_EARL];
    entity->process = Player2ProcessTerrainMotion;
    entity->sequence = Player2AnimationDescriptors[PLAYER_ANIM_IDLE].sequence;
    entity->frames = Player2AnimationDescriptors[PLAYER_ANIM_IDLE].frames;
    entity->frame_phase = 10;
    entity->frame_duration = 1;
    entity->width = 30;
    entity->height_pixels = 40;
    entity->sprite_count = 4;
    entity->tile_count = 40;
    entity->tile = VDP_TILE_PRIORITY;
    entity->draw_flags = ENTITY_ACTIVE;
    entity->behavior_flags = ENTITY_COLLIDABLE;
    entity = &gPlayerInfo[GAMEPLAY_ENTITY_SLOT_COUNT];
    gInventoryDrawListHead = entity;
    for (sprite_number = 0; sprite_number < 2; ++sprite_number) {
        entity->frames = Spr_PresentSelector;
        entity->sequence = SingleFrameHoldAnimation;
        entity->width = 32;
        entity->height_pixels = 24;
        entity->sprite_count = 1;
        entity->tile_count = 12;
        entity->tile = VDP_TILE_PRIORITY;
        entity->next = entity + 1;
        entity = entity->next;
        for (present_number = 0; present_number < 6; ++present_number) {
            entity->frames = PresentSpriteTable;
            entity->sequence = SingleFrameHoldAnimation;
            entity->width = 24;
            entity->height_pixels = 16;
            entity->sprite_count = 1;
            entity->tile_count = 6;
            entity->tile = VDP_TILE_PRIORITY;
            entity->next = entity + 1;
            entity = entity->next;
        }
    }
    entity[-1].next = 0;
}
