#ifndef TJE_SHIP_H
#define TJE_SHIP_H

#define SHIP_PIECE_WINDSHIELD 0 /* ShowShipPieceName selects "rocketship windshield". */
#define SHIP_PIECE_LEFT_SPEAKER 1 /* ShowShipPieceName selects "left megawatt speaker". */
#define SHIP_PIECE_AMPLAMATOR 2 /* ShowShipPieceName selects "super funkomatic amplamator". */
#define SHIP_PIECE_CONNECTOR_FIN 3 /* ShowShipPieceName selects "amplamator connector fin". */
#define SHIP_PIECE_FORWARD_STABILIZER 4 /* ShowShipPieceName selects "forward stabilizing unit". */
#define SHIP_PIECE_REAR_LEG 5 /* ShowShipPieceName selects "rear leg". */
#define SHIP_PIECE_SNOWBOARD 6 /* ShowShipPieceName selects "awesome snowboard". */
#define SHIP_PIECE_RAPMASTER_CAPSULE 7 /* ShowShipPieceName selects "righteous rapmaster capsule". */
#define SHIP_PIECE_RIGHT_SPEAKER 8 /* ShowShipPieceName selects "right megawatt speaker". */
#define SHIP_PIECE_THRUSTER 9 /* ShowShipPieceName selects "hyperfunk thruster". */

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void ShipPieceAbility();
extern void ShowShipPieces();
extern void AssignShipPieceLevels();
extern void FillSpriteOpaquePixels();
extern void AnimateShipScreenBackground();
extern void RestoreGameplayGraphics();
extern void AnimateTransitionBackground();
extern void PlayShipCompletionSequence();
extern void InitializeShipScreenTilemapAndPalettes();
extern void ShowShipPieceName();
extern void ShowMissingShipPieceSummary();
extern void AnimateShipScreenPalette();
extern void WaitForShipScreenAcknowledgement();
extern void ShipUpdatePendingPieceIndicator();
extern void ShowRandomUncollectedShipPiece();
extern int EarthlingIsMailOrderMailbox();
#ifndef TJE_LOCAL_EARTHLINGCONFIGUREMAILBOXLEVELBANK
extern char EarthlingConfigureMailboxLevelBank();
#endif

#endif
