#ifndef TJE_MENU_CALLS_H
#define TJE_MENU_CALLS_H

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
extern void func_023554();
extern int func_023740();
extern void func_023b08();
extern void func_023b70();
extern void func_023b8c();

#endif
