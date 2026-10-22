#ifndef TJE_RAM_SYSTEM_H
#define TJE_RAM_SYSTEM_H

/* Decoded controller bits: ReadMappedPlayerInput exchanges only A and C.
 * CheckAButton/CheckBButton and the movement dispatch establish these roles. */
#define INPUT_UP 0x01
#define INPUT_DOWN 0x02
#define INPUT_LEFT 0x04
#define INPUT_RIGHT 0x08
#define INPUT_BUTTON_B 0x10
#define INPUT_BUTTON_C 0x20
#define INPUT_BUTTON_A 0x40
#define INPUT_START 0x80
#define INPUT_DIRECTION_MASK 0x0f
#define INPUT_VERTICAL_MASK (INPUT_UP | INPUT_DOWN)
#define INPUT_HORIZONTAL_MASK (INPUT_LEFT | INPUT_RIGHT)
#define INPUT_ACTION_MASK (INPUT_BUTTON_A | INPUT_BUTTON_B | INPUT_BUTTON_C)
#define INPUT_BUTTON_MASK (INPUT_ACTION_MASK | INPUT_START)

/* Frame synchronization and interrupt override. gFrameCounterLowByte
 * is the low-byte alias of gFrameCounter on the big-endian target. */
extern unsigned short gFrameCounter;
extern char gFrameCounterLowByte;
extern void (*gVBlankFunc)();
extern char gHBlankGraphicsEnabled;
extern char gVBlankGraphicsEnabled;
extern unsigned char gSpriteGraphicsUploadPending;
extern char gVBlankGraphicsPaused;
extern char gInventoryRedrawPhase;
extern char gHudRedrawPending;
extern char gVBlankAudioInputOverride;
extern char gRequestedMusicSelection;
extern char gCurrentMusicSelection;
extern char gTextFontAnimationEnabled;
extern short gTerrainPriorityUpdatePending;

/* Existing scalar byte alias of gPlayerInventoryOpen[1] in ram_ui.h; this is
 * not a separate interrupt-state allocation. */
extern char gPlayer2InventoryOpen;

/* Input state keeps its existing byte-array and word views. Incomplete
 * outer bounds are retained; no adjacent-symbol guess establishes capacity.
 * gPlayer2PreviousProcessedInput also names gPlayerPreviousProcessedInput[1]. */
extern unsigned short gController1RawInput;
extern unsigned short gController2RawInput;
extern unsigned char gPlayerProcessedInput[];
extern unsigned char gPlayerPreviousProcessedInput[];
extern unsigned char gPlayerSleepPreviousInput[];
extern char gControllerPortsSwapped;
extern unsigned char gPlayer2PreviousProcessedInput;
extern char gControllerButtonsAcSwapped;

/* VDP register soft copy and main-loop palette/frame state. */
extern unsigned char gMode2RegSoftcopy;
extern char gVBlankPlayerUpdateParity;
extern char gLevelPaletteAnimationFrame;
extern char gLevelPaletteAnimationDelay;

/* Shared random cache, its current index and generator state. */
extern unsigned short gCachedRandomValues[59];
extern unsigned short gRandomCacheIndex;
extern long gRNGState;

/* Existing shared sound-driver state views. */
extern short gFmEffectTemplateStatus;
extern short gBoomBoxMusicTemplateHold;
extern char gMusicAnimationEvent;

/* Deferred FM requests, animation-event channel, and second-player input alias. */
extern short gFmChannelReleaseRequested;
extern short gMusicTrackResetRequested;
extern short gPendingFmEffect;
extern char gMusicAnimationChannel;
extern unsigned char gPlayer2ProcessedInput;

#endif
