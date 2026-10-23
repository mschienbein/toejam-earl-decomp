#ifndef TJE_RAM_JAM_H
#define TJE_RAM_JAM_H

/* StartJamOut releases sequencer channels for selection zero and selects random samples for seven. */
#define JAM_SONG_NO_TRACK 0
#define JAM_SONG_RANDOM_SAMPLES 7
/* JamOutRandomSampleVBlank wraps the song cycle at seven to its released-channel state. */
#define JAM_SONG_CYCLE_NO_TRACK 0
#define JAM_SONG_CYCLE_RELEASE_CHANNELS 7
/* JamOutRandomSampleVBlank releases channels, selects the normal bank, or selects the alternate bank. */
#define JAM_TEMPLATE_CHANNELS_RELEASED 0
#define JAM_TEMPLATE_NORMAL_BANK 1
#define JAM_TEMPLATE_ALTERNATE_BANK 2

/* JamOutUpdateLayoutsAndScroll dispatches these layout and scrolling phases in order. */
#define JAM_LAYOUT_LEAD_PAIR 0
#define JAM_LAYOUT_FOUR_PAIRS 1
#define JAM_LAYOUT_NINE_PAIRS 2
#define JAM_LAYOUT_SCROLL_STEP_POSITIVE 3
#define JAM_LAYOUT_SCROLL_STEP_NEGATIVE 4
#define JAM_LAYOUT_STOP_SCROLL 5


/* Exact observed views; unspecified outer bounds do not claim allocations. */
extern short gJamLayoutCountdown;
extern short gJamLayoutPhase;
extern short gJamHorizontalScroll;
extern short gJamVerticalScroll;
extern short gJamScrollStep;
extern short gJamToejamEntities[9];
extern short gJamEarlEntities[9];
extern char *gJamToejamAnimationSequence;
extern short gJamPlayer1PreviousInput;
extern short gJamToejamAnimationFrame;
extern char *gJamEarlAnimationSequence;
extern short gJamPlayer2PreviousInput;
extern short gJamEarlAnimationFrame;
extern short gJamPlayer1PendingAnimationInput;
extern short gJamPlayer2PendingAnimationInput;
extern short gJamToejamAnimationLocked;
extern short gJamEarlAnimationLocked;
extern short gJamSelectedSong;
extern unsigned short gJamAudioRoute1PendingInput;
extern unsigned short gJamAudioRoute2PendingInput;
extern short gJamSongCycleIndex;
extern short gJamTemplateBankCycleIndex;
extern unsigned short gJamAudioRoute2ButtonASampleIndex;
extern unsigned short gJamAudioRoute2ButtonBSampleIndex;
extern unsigned short gJamAudioRoute2ButtonCSampleIndex;
extern unsigned short gJamAudioRoute1ButtonASampleIndex;
extern unsigned short gJamAudioRoute1ButtonBSampleIndex;
extern unsigned short gJamAudioRoute1ButtonCSampleIndex;
extern short gMusicAlternateTemplateBankEnabled;

#endif
