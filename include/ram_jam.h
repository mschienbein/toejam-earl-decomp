#ifndef TJE_RAM_JAM_H
#define TJE_RAM_JAM_H


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
