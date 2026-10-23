#ifndef TJE_SPEECH_H
#define TJE_SPEECH_H

/* A speech upload contains one reserved prefix tile and at most twelve glyphs. */
#define SPEECH_TEXT_MAX_GLYPHS 12
#define SPEECH_TEXT_UPLOAD_TILES 13
#define SPEECH_BUBBLE_PREFIX_GLYPH 27

/* StartDialogue returns -1 when no reply slot or eligible player speaker is available. */
#define DIALOGUE_SLOT_UNAVAILABLE (-1)

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
