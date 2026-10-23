#ifndef TJE_MENU_CALLS_H
#define TJE_MENU_CALLS_H

#define MENU_WHO_BOTH 0 /* NewGameMainMenu activates both players for this Who selection. */
#define MENU_WHO_TOEJAM 1 /* NewGameMainMenu activates only ToeJam for this Who selection. */
#define MENU_WHO_EARL 2 /* NewGameMainMenu activates only Earl for this Who selection. */
#define MENU_WHAT_RANDOM_WORLD 0 /* NewGameMainMenu leaves the auxiliary-menu loop for this selection. */
#define MENU_WHAT_JAM_OUT 1 /* NewGameMainMenu dispatches this What selection to StartJamOut. */
#define MENU_WHAT_INTRO 2 /* NewGameMainMenu dispatches this What selection to IntroVideo. */
#define MENU_WHAT_CONTROLLERS 3 /* NewGameMainMenu dispatches this selection to controller options. */
#define MENU_WHAT_FIXED_WORLD 4 /* NewGameMainMenu sets gWorldType for this What selection. */
#define MENU_HEADER_WHO 0 /* MenuWhoHandler selects this MenuHeaderAnimations row. */
#define MENU_HEADER_WHAT 1 /* MenuWhatHandler selects this MenuHeaderAnimations row. */
#define MENU_HEADER_JAM_OUT 2 /* MenuJamOutHandler selects this MenuHeaderAnimations row. */
#define MENU_HEADER_CONTROLLERS 3 /* MenuControllerOptionsHandler selects this header row. */
#define WORLD_TYPE_FIXED 1 /* NewGameMainMenu stores this value for the fixed-world selection. */

/* K&R return interfaces. Represented C definitions establish their types;
 * remaining ROM-only declarations describe the observed ABI, not a uniquely
 * recovered historical C spelling. */
extern void AddMenuHeader();
extern int GeneralMenuHandler();
extern int MenuControllerOptionsHandler();
extern void MenuDrawTextItems();
extern int MenuJamOutHandler();
extern int MenuWhatHandler();
extern int MenuWhoHandler();
extern void NewGameMainMenu();
extern void RemoveMenuHeader();
extern void ShowGameOverAndReset();
extern int AreBothControllersConnected();
extern void CreateMenuCursorEntity();
extern void ReleaseMenuCursorEntity();
extern void PositionMenuCursorEntity();

#endif
