#ifndef TJE_DIALOGUE_IDS_H
#define TJE_DIALOGUE_IDS_H

#define DIALOGUE_SNEEZE 0 /* UpdateSpeechAndDialogue pairs this random response with Sfx_Achoo. */
#define DIALOGUE_REUNION 1 /* UpdateSpeechAndDialogue requests this when separated players reunite. */

#define DIALOGUE_SHIP_PIECE_COLLECTED 2 /* ShipPieceAbility requests this response after showing the collected piece. */
#define DIALOGUE_TOEJAM_ELEVATOR_REUNION 3 /* UpdateSpeechAndDialogue selects this when ToeJam has an odd elevator handle. */
#define DIALOGUE_EARL_ELEVATOR_REUNION 4 /* UpdateSpeechAndDialogue selects this when Earl has an odd elevator handle. */
#define DIALOGUE_GROUND_ITEM_BASE 5 /* PlayerCollectNearbyItems adds this to the ground-item-relative kind. */
#define DIALOGUE_FUDGE_SUNDAE 6 /* EatFudgeSundae requests this response after adding health. */
#define DIALOGUE_TOEJAM_CONTROL_REMAP_CONTACT 22 /* UpdateSpeechAndDialogue selects this for ToeJam's close control-remap contact. */
#define DIALOGUE_EARL_CONTROL_REMAP_CONTACT 23 /* UpdateSpeechAndDialogue's Earl contact branch selects this, despite its first-player-only loop. */
#define DIALOGUE_BURP 24 /* PresentUpdateBurps requests this before the first delayed burp. */
#define DIALOGUE_INVENTORY_FULL 25 /* Pickup and mail-order paths request this when all inventory slots are occupied. */
#define DIALOGUE_TOMATOES 26 /* SetupSelectedPresent requests this for PRESENT_TOMATOES. */
#define DIALOGUE_SLING_SHOT 27 /* SetupSelectedPresent requests this for PRESENT_SLING_SHOT. */
#define DIALOGUE_SUPER_HITOPS 28 /* SetupSelectedPresent requests this for PRESENT_SUPER_HITOPS. */
#define DIALOGUE_EXTRA_BUCK 29 /* OpenSelectedPresent requests this after PRESENT_EXTRA_BUCK. */
#define DIALOGUE_JACKPOT 30 /* OpenSelectedPresent requests this after PRESENT_JACKPOT. */

#endif
