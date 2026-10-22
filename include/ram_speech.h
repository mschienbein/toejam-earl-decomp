#ifndef TJE_RAM_SPEECH_H
#define TJE_RAM_SPEECH_H

/* Speech cooldown and four parallel bubble handle/lifetime slots. */
extern short gPcmEffectCooldownTicks;
extern char gSpeechBubbleEntities[4];
extern char gSpeechBubbleLifetimes[4];
extern char gReunionDialoguePending;

/* Two delayed-reply timers, text pointers and speaker bytes. */
extern short gDelayedSpeechReplyTimers[2];
extern char *gDelayedSpeechReplyTexts[2];
extern char gDelayedSpeechReplySpeakers[2];

/* Existing canonical alias for gDelayedSpeechReplyTimers[1], the second reply timer. */
extern short gSecondSpeechReplyTimer;

/* Existing scalar byte alias of gDelayedSpeechReplySpeakers[1], the second reply speaker. */
extern char gSecondDelayedSpeechReplySpeaker;

/* Word lifetime override consumed by the next allocated bubble. */
extern short gNextSpeechLifetimeOverride;

/* Low byte of the word lifetime override gNextSpeechLifetimeOverride on this big-endian target. */
extern char gNextSpeechLifetimeOverrideLowByte;

#endif
