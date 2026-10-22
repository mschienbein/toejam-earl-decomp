# Sprites: entities, graphics residency, and hardware chains

## Purpose and evidence

Sprites translates shared object records into per-view metasprite pieces,
allocates scarce hardware slots and tile runs, and maintains draw order, shadows,
and uploads. World simulation and renderer residency share the same records.

Evidence is `include/entities.h`, `include/sprites.h`,
`include/ram_sprites.h`, and the source files listed below. Paths refer to the
current repository-root layout. An interface declaration alone is not claimed
to reveal a missing function's implementation.

## Files and roles

- `src/sprites/sprite_slots.c`: hardware-slot,
  tile-run, viewport, and dynamic-object allocation/release.
- `entity_visibility.c`: view/level tests and residency transitions.
- `entity_sprite_order.c`: attachments, positioning, links, and Y sorting.
- `entity_depth.c`: optional shadows and their elevation-dependent appearance.
- `sprite_frame_budget.c`: raw tile buffering and animation-motion budget.
- `sprite_pixel_masks.c`: buffered pixel clipping/mask removal.
- `load_sprites.c`: hardware table and queued tile-run DMA uploads.
- `set_sprite.c`: selection/reset of player animation descriptors.
- `entity_indexes.c`: stable object indices and initial sprite sentinels.
- `select_first_link.c`, `sprite_links_vdp.c`: live VDP chain-root changes.
- `link_player_sprites.c`: the special chain used during split-screen wipe.
- `hide_player_sprites.c`: direct hiding for a flagged player.
- `src/asm/rle.s`: sprite decompression; see `docs/subsystems/platform.md`.

## Data structures and RAM

`object_state` is a 128-byte shared record (`entities.h`). Relevant fields are:

- World `x/y`, `height`, fixed-point velocity/acceleration and fraction bytes.
- `process`, `sequence`, `frames`, and `current_frame` animation interfaces.
- `frame`, `frame_phase`, and `frame_duration` animation progress.
- `width`, `height_pixels`, `tile_count`, `tile`, and `sprite_count` reservations.
- `draw_flags`: active, two viewport-residency bits, physics policy, shadow,
  and draw-late bits; it is not purely a visibility field.
- `behavior_flags`: shared graphics, attachment, player suppression, masking,
  hidden state, and collision policy.
- Two 12-handle sprite arrays, two shadow handles, and `sprite_ready`.
- `owner` plus three offset/latch bytes, `next`, and stable `entity_index`.

`render_frame` is a four-byte prefix followed by `count` ten-byte pieces;
there is no fixed 12-piece stride in the ROM frame format.
Its prefix contains piece count, X/Y motion multipliers, and tile-reuse flag.
`render_piece` contains tile dimensions, signed pixel offsets, flipped X,
a format byte, and a raw/packed tile-data pointer (`sprites.h`).
A `timed_animation` pairs sequence/frame pointers with period and Y offset.

`hardware_sprite` is the eight-byte VDP entry: Y, size/link bytes, tile word, X.
`gHardwareSpriteTable[80]` is its RAM mirror. Aliases name record-zero's link,
record-eleven's byte region, and record-23's primary sentinel link.
`gViewportSpriteSlotFlags[54]`, `gSpriteTileGroupAllocated[80]`, and
`gDynamicEntitySlotAllocated[34]` are distinct resource pools.
`gSpriteTileUploadRuns[40]` holds destination/count word pairs.
The declared 8192-byte upload-buffer span is inferred from adjacent state,
not a recovered independent allocator bound (`ram_sprites.h` comment).

## Object and tile allocation

`AllocateEntity` normally examines 20 dynamic flags, or 34 on level 26.
It returns entity index six plus the selected flag index, or zero on failure.
`ResetEntityRuntimeState` clears 47 words, not all 128 bytes; the entity's stable
index and unused tail are not indiscriminately zeroed by that primitive.
The allocated object receives `ENTITY_ACTIVE`.
`FreeEntity` preserves indices zero/one, releases both resident views, clears
active flags, and releases a dynamic flag for indices six and above.

Entity eviction is implemented in `src/map/land_processing.c`:
`AllocateEntityWithEviction` first tries ordinary allocation unless `release_first`
is set, then reclaims the first available mole effect, speech bubble, shared
tomato, or item object, in that order. It next visits each loaded earthling bank,
trying secondary objects then primary objects with nonzero kind (excluding the
ship-piece kind). After freeing one candidate it retries allocation.
`EntityReleaseOneByPriority` wraps this with release-first mode and frees the
returned replacement, leaving an available entity slot.

`AllocateSpriteTileRun` rounds the requested tile count to groups of eight,
finds consecutive free groups, and returns a tile starting at base 640.
`FreeSpriteTileRun` reverses that calculation. A zero return means failure.
These tile indices occupy packed VDP words alongside flip/palette/priority bits.

`AllocateSpriteSlots` scans 54 flags, returns public handles `25..78`, and
supports a reversed search for secondary selection. It chooses unused slots
first and, when allowed, slots whose second state bit is clear.
The return convention is zero success, one failure.
`AllocateSpriteSlot` converts failure to handle zero.
`IsSpriteSlotLocked` tests `gViewportSpriteSlotFlags[item - 25] & 2`; it does
not treat the whole flag byte as a generic boolean.

## Per-view resource lifecycle

`AllocateEntityViewportSprites` obtains tiles only when neither viewport is
resident and graphics are not shared. It preserves the high tile attributes,
then allocates that viewport's sprite list and sets the corresponding draw bit.
The second view ordinarily shares the entity's tile data, not another tile run.
`ReleaseEntityViewportSprites` releases its handles and reclaimable shadow;
only the final view releases non-shared tiles.
`ReleaseEntityShadows` drops stale/reclaimable cached shadow handles.

`IsEntityVisibleInViewport` uses width 320, height 224 for a shared primary view,
and height 112 for each split view. It checks the displayed level and suppresses
a view when its player's menu is open. Bounds include entity width, pixel
height, elevation, and a ten-pixel lower margin.
`UpdateEntityViewportSprites` allocates/releases views on those results and
excludes speech-bubble entities from ordinary allocation. Special handling
prevents the two player records from retaining the wrong dual-view arrangement.
`UpdateAllEntityViewportSprites` visits 26 records normally, 40 on level 26,
and clears the inventory draw-list head in the latter case.

## Draw order, attachments, and positioning

`SortEntitiesForSpriteDraw` builds two descending-world-Y linked lists,
normal and `ENTITY_DRAW_LATE`, then appends the late list after normal records.
Its record count is 26 or 40 according to level 26.
`BuildViewportSpriteLinks` emits ordinary entity pieces, then shadows, then
active inventory pieces, with sentinel records 23/24 and terminal handle 79.
Those categories and link ordering are observable; a modern painter's-order
interpretation must account for the target renderer's priority rules.

`UpdateAttachedEntityPosition` follows another entity using signed offsets,
including elevation and inherited tile-priority bit. Owner `-1` and `-2` instead
anchor to pending camera origins using unsigned X/Y byte offsets.
`PositionEntitySprites` uses primary screen bias `(128,128)` and secondary bias
`(128,240)`, subtracting each pending camera and entity elevation.
Horizontal flip selects the piece's authored `flipped_x` offset.
Hidden pieces use hardware Y one; elevation outside `-224..224` uses Y zero.
Shadows stay at ground-relative Y and depend on the sprite size bits.

`UpdateEntityShadows` allocates a reclaimable slot per resident view, quantizes
elevation into size/tile variants, and selects a special sunk image for negative
height. `Process2FEntityFlag` performs this dispatch for 26 records.
`HideFlaggedPlayerSprites` writes Y one directly to VRAM for a ready player
with `ENTITY_PLAYER_SUPPRESSED`; core alternates its serviced player in VBlank.

## Animation and upload flow

`SetSprite` chooses ToeJam descriptors only for entity index zero, otherwise
player-two descriptors; it resets phase/index and computes
`animation_group_mask = 1 << (sprite_state >> 2)`.
`ANIMATION_HOLD` is 254, `ANIMATION_LOOP` 255, and frame selection masks with 127.

`ProcessEntityAnimationMotion` skips inactive and speech-bubble records.
At an imminent frame boundary it enforces a nominal 112-tile total, resolves
loop/hold commands, identifies reusable graphics, and invokes the motion callback
for animation-controlled axes. This function is not the complete decoder.
`AppendSpriteTilesToBuffer` copies complete 32-byte tiles and advances the offset.
`ClipSpritePixelsBelowCutoff` blanks packed rows below the cutoff, preserving a
partial boundary mask; `ClearSpriteMaskPixels` removes selected all-ones patterns.

`SpriteLoadingStuff` is gated by `VBLANK_FLAG_2`, force, or tiles-only request.
It DMA-uploads the complete 80-record table to VRAM `0xf800` unless tiles-only,
caches sentinel links, then uploads destination/count pairs from the tile queue.
It resets the buffer offset, queue word count, and `_2` after consumption.
`SelectPrimaryViewportSprites`, `SelectSecondaryViewportSprites`, and chain
termination helpers patch live VRAM links, enabling HBlank view switching.

`QueueSpritePieceTiles`, `PrepareEntitySpriteFrame`, and
`UpdateAndQueueEntitySprites` are declared/called but lack bodies in the supplied
integration tree. Their complete decode/queue ordering remains an evidence gap.
Core calls the last function in its odd iteration and uses VBlank for loading.

## Interactions

- Player and earthling movement share the object record with rendering state.
- Speech creates custom tile/frame data and bypasses normal visibility allocation.
- Inventory adds a separate late hardware-link list, not a separate renderer.
- Camera transitions may evict lower-priority entities to guarantee player slots.
- Level 26 expands the active record count for the ending/Funkatron presentation.
- Runtime masks and RLE decoding alter tile buffers before transfer, not ROM assets.

## ORIGINAL BUG list

- `AllocateEntityViewportSprites`: if shared tiles are set, no view is resident,
  and sprite-slot allocation fails, it passes uninitialized `tile` to release.
- `ProcessEntityAnimationMotion`: the not-ready path adds its tile count only
  when the total already exceeds 112, contrary to the nominal budget.

## Open questions

- Missing queue/preparation bodies prevent a complete account of format-bit
  decoding, animation countdown advancement, and upload-buffer overflow policy.
- The upload buffer's declared span must not be confused with a checked capacity.
- Shared flag bytes encode rendering and gameplay policy; a clean separation
  requires tracing all consumers, not merely replacing the hardware table.

## Port notes

- Platform layer: replace `SpriteLoadingStuff` VDP table/tile DMA and live chain
  writes with a metasprite renderer and decoded resources.
- Shared-screen coupling: decouple `AllocateEntityViewportSprites` per-view
  reservations from one 54-slot budget; preserve authored ordering without
  global menu hiding.
- Determinism: keep `ProcessEntityAnimationMotion` render-budget/allocation
  outcomes out of authoritative movement callbacks, or preserve them explicitly.
- Content tables: export frames, pieces, sequence bytecode, animation groups,
  shadow variants, and flip offsets into validated data-driven definitions.
- Camera/visibility: `ENTITY_EITHER_VIEW` residency gates graphics work;
  audit its gameplay consumers before widening culling or relaxing resource limits.
