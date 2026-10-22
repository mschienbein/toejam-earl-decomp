#ifndef TJE_SHIP_H
#define TJE_SHIP_H

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
extern void WaitForShipScreenAcknowledgement();
extern void ShipUpdatePendingPieceIndicator();
extern void ShowRandomUncollectedShipPiece();
extern int EarthlingIsMailOrderMailbox();
#ifndef TJE_LOCAL_FUNC_020EAA
extern char EarthlingConfigureMailboxLevelBank();
#endif

#endif
