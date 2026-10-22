#ifndef TJE_SPEECH_H
#define TJE_SPEECH_H

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern short GenerateSpeech();
extern void QueueSpeechTextTiles();
extern void SetNextSpeechLifetime();
extern void ClearSpeechBubblesAndReplies();
extern int HasSpeakerSpeech();
extern int StartDialogue();
extern void UpdateSpeechAndDialogue();

#endif
