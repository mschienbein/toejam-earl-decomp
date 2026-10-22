# Speech: text bubbles and delayed conversations

## Purpose and evidence

Speech creates short text metasprites associated with world entities, expires
them, delivers delayed replies, and triggers ambient player conversation.
It is visual text speech; sampled voice/noise playback belongs to audio.
The two meet at explicit `PlaySoundEffect` calls, such as the sneeze event.

Evidence is the speech C/header implementation and canonical-name records in
`config/rev02/names.json` where identified below. Paths are repository-root paths.
Text packing, bubble ownership, and dialogue scheduling now have matched bodies
in the complete speech unit; canonical names are not claimed historical spellings.

## Files and roles

- `src/speech/speech_and_dialogue.c`: text tile uploads, lifetime override, bubble
  construction/clearing, speaker occupancy, dialogue scheduling, expiration/reply
  timers, sneeze/reunion triggers, and the control-remap conversation branch.
- `src/speech/speech_bubble_lookup.c`: `IsSpeechBubbleEntity`.
- `include/speech.h`: K&R interfaces, including `StartDialogue` and text queuing.
- `include/ram_speech.h`: four bubble slots, two replies, lifetime override/aliases.
- `include/entities.h`: shared object fields and attachment conventions.
- `include/sprites.h`: runtime frame/piece format.
- `include/assets_ui.h`: `SpeechBubbleTextFrame`, font resources, and
  `DialogueDescriptors[58]`.
- `include/ui.h`: `dialogue_descriptor` fields and reply-speaker policy.
- `src/ui/inventory.c`: shared character mapping/font behavior, not bubble ownership.
- `src/sprites/entity_visibility.c`: deliberately excludes bubbles from ordinary
  automatic viewport allocation.

## Key RAM state

Four parallel slots contain `gSpeechBubbleEntities[4]` and signed-byte
`gSpeechBubbleLifetimes[4]`. A zero handle means an unused speech slot.
They refer to ordinary dynamic entries in `gPlayerInfo`; there is no second
speech-only object allocator (`GenerateSpeech`).

Two reply slots contain short timers, text pointers, and speaker bytes:
`gDelayedSpeechReplyTimers`, `gDelayedSpeechReplyTexts`, and
`gDelayedSpeechReplySpeakers`.
`gSecondSpeechReplyTimer` and `gSecondDelayedSpeechReplySpeaker` alias the second
entries, not additional queues (`ram_speech.h`).

`gNextSpeechLifetimeOverride` is a word set by `SetNextSpeechLifetime`.
Its low-byte alias is copied into the signed bubble lifetime on successful
construction; the word is then cleared. Only the next allocated bubble consumes it.
`gReunionDialoguePending` remembers that the players have been separated.
`gPcmEffectCooldownTicks` is declared in this header but belongs to the sampled
sound path, not the bubble-expiration loop shown here.

The bubble's ordinary object record carries level, geometry, graphics reservation,
view bits, frame readiness, and optional speaker attachment.
The runtime text frame is a shared frame resource, not an independent animation
sequence for each line (`speech_and_dialogue.c`, `assets_ui.h`).

`dialogue_descriptor` contains `first_speaker`, `reply_speaker`, `first_line`,
`delay`, and `reply`. `DialogueDescriptors` is a declared 58-entry table of those
records. These scripted exchanges are distinct from the elevator
`ride_speech` records (`include/ui.h`).

## Bubble creation flow

`GenerateSpeech(speaker,text,replace_attached)` performs these operations:

1. Resolve the source object from the supplied speaker index.
2. Reject a source without either viewport-residency bit.
3. Reject player speakers in `PLAYER_ANIM_SLEEP`.
4. When replacement/attachment is requested, remove existing attached bubbles
   whose owner is this speaker.
5. Choose the first free speech slot, or the slot with smallest remaining
   lifetime when all four are occupied, freeing that slot's old entity.
6. Measure zero-terminated text and clamp its rendered length to 12 characters.
7. Allocate an ordinary dynamic entity; failure returns zero.
8. Copy source level and position, offset Y/elevation by 15, use pixel height 16,
   and reserve four sprite pieces and 13 tiles.
9. Set tile priority on levels below 26 and select the runtime text frame.
10. Allocate primary sprites when the source has primary residency, otherwise
    secondary sprites. It does not automatically allocate both views.
11. If allocation produced no resident view, free the entity and clear its slot.
12. Mark skip-physics, wait for pending sprite upload ownership to clear, queue
    text tiles, prepare the runtime frame, and mark ready.
13. If requested, attach to the source with owner and Y/elevation offsets 15.
14. Assign the override's low-byte lifetime or the default value 40, then return
    the dynamic entity handle.

The source comments describe choosing the oldest bubble; the actual selection
compares remaining lifetimes. With overrides, lowest remaining lifetime is the
precise observable rule, not necessarily earliest creation time.

`replace_attached` combines two behaviors: replacing earlier attached speech and
making the new speech follow its owner. False leaves a positioned, unattached
bubble, rather than creating a persistent tracked speaker relationship.
The source-residency requirement means off-camera speech can fail before any
allocation, even when the text/event itself is otherwise valid.

## Positioning and renderer integration

Attached bubbles use `UpdateAttachedEntityPosition` in the sprite subsystem.
Their owner is an entity index, and the three attachment bytes are interpreted
as offsets by that renderer path (`src/sprites/entity_sprite_rendering.c`).
`GenerateSpeech` calls `PrepareEntitySpriteFrame` directly and sets readiness;
normal animation/visibility machinery recognizes bubble entities specially.
`IsSpeechBubbleEntity` scans the four handles only for entity indices six or
above. It does not identify arbitrary text frames by their graphics pointer.
`ProcessEntityAnimationMotion` and `UpdateEntityViewportSprites` use this predicate
instead of treating speech as an ordinary animated resident (`sprites` sources).

`QueueSpeechTextTiles` copies the reserved bubble-prefix tile, then centers the
caller-clamped text within `SPEECH_TEXT_MAX_GLYPHS` (12) columns using blank tiles
before and after it. `TextMapCharacterToGlyph` maps each input byte; a glyph equal
to `SPEECH_BUBBLE_PREFIX_GLYPH` is advanced so it does not reuse the reserved
prefix. Odd lengths add one trailing pad tile. The function queues a
`SPEECH_TEXT_UPLOAD_TILES` (13) run from `RuntimeTextFontTiles`; it does not
independently clamp a caller-supplied length.

## Expiration, clearing, and reply delivery

`UpdateSpeechAndDialogue` visits four slots. It decrements each signed lifetime
and frees a bubble when it becomes negative or loses both viewport bits.
It then visits two delayed-reply timers and calls attached `GenerateSpeech`
when a nonzero timer decrements to zero. A failed reply creation is not retried
by any code in this update loop.

`ClearSpeechBubblesAndReplies` frees all held bubble entities, clears both timers,
and clears the reunion latch. It does not need to zero unused text pointers to
make the timer-controlled replies inactive.
`HasSpeakerSpeech` scans existing bubble handles and tests skip-physics plus
owner equality, then checks both active reply timers/speaker values.
The observed flag test is skip-physics, not explicitly `ENTITY_ATTACHED`.
No additional safety interpretation is assigned to that historical choice.

## Ambient conversation rules

Each `UpdateSpeechAndDialogue` performs the random sneeze test
`(GetRandomNumber() & 0x1fff) == 1`, then randomly chooses a player.
It calls `EarthlingResetStateTwoByDrawMask` using that player's viewport mask;
on success, dialogue zero is started. Any dialogue result other than `-1`
triggers `Sfx_Achoo`; split screen cancels the associated delayed reply timer.

When players occupy the same level, absolute X/Y separation is computed.
A pending reunion can trigger only when not split, with X below 260 and Y below
174. Odd elevator-state predicates plus random tests select dialogue 3 or 4,
otherwise dialogue 1. The missing speaker arguments are preserved original bugs.
Distances above 640 X or 448 Y arm the reunion latch; differing levels also arm it.

A final loop checks only slot zero for a control-remap entity, proximity below
30 X/20 Y, and a one-in-256 masked-random match to value two. Its second-player
branch is unreachable with the observed loop bound. The source marks this bug.

`StartDialogue` chooses the first free delayed-reply slot, returning -1 when
both are occupied. The descriptor can force the opening speaker to either
player; otherwise the caller's speaker is retained, with a duplicate-other-slot
check. Reply speaker values zero/one force a player,
`DIALOGUE_REPLY_FIRST_SPEAKER` reuses the resolved opening speaker, and other
values use the resolved speaker XOR one. An absent/menu-open player cannot open
dialogue.

It immediately requests the opening line when present. A reply is scheduled
with the descriptor's `delay` only when the view is shared or both lines use
the same speaker. A player reply recipient must also be alive with no menu open. The return value is the
selected slot, not a bubble handle: failure to allocate the opening bubble does
not itself change that result or create an allocation retry.

## Interactions

- Core calls speech updating in the common tail of its even iterations, not
  once per raw VBlank (`FurtherInitialisation` main loop).
- Player/present/earthling/ship code can explicitly generate speech or dialogue.
- Elevator cutscenes use additional exchange records and hide/reveal bubbles;
  those scripts are separate from the ordinary two delayed-reply timers.
- UI transient cleanup frees speech handles during scene transitions.
- Sprites supplies allocation, tile upload, attachment motion, and display order.
- Random ambient dialogue consumes the same RNG used elsewhere in gameplay.

## ORIGINAL BUG list

- `UpdateSpeechAndDialogue`: reunion `StartDialogue(3)`, `(4)`, and `(1)` calls
  omit their speaker argument, as the source comment explicitly records.
- `UpdateSpeechAndDialogue`: the control-remap dialogue loop visits only the
  first player (`slot < 1`), leaving the second-player alternative unreachable.

## Open questions

- The lifetime override narrows to a signed byte; accepted content ranges are
  not validated by the shown setter or constructor.
- No retry/queue policy beyond two reply slots is observed.

## Port notes

- Platform layer: replace `GenerateSpeech` tile/upload preparation with a text
  renderer; retain authored short-line positioning and timing as optional style.
- Shared-screen coupling: make `GenerateSpeech` speaker/recipient/audience
  explicit instead of suppressing a partner's dialogue via viewport/menu state.
- Determinism: advance `gDelayedSpeechReplyTimers` and the ambient events in
  `UpdateSpeechAndDialogue` on simulation ticks; separate cosmetic sneeze RNG
  and define speakers explicitly rather than retaining indeterminate arguments.
- Content tables: export dialogue descriptors, strings, lifetime, speaker policy,
  and font metrics; support localization beyond the original 12-character clamp.
- Camera/visibility: decide whether off-camera speech is discarded, queued, or
  visible in each player's UI. That is a behavior change from residency-gated
  generation and should not silently depend on widescreen dimensions.
