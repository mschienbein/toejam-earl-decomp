#ifndef TJE_SHIP_H
#define TJE_SHIP_H

/* Shared K&R ship return interfaces. */
extern void ShipPieceAbility();
extern void func_01faf4();
extern void func_01fb40();
extern void func_01fc58();
extern void func_01fd58();
extern void func_01fe68();
extern void func_020182();
extern void func_020310();
extern void func_0204ce();
extern void func_020840();
extern void func_020e2a();

/* Represented C definitions establish these return types. */
extern void func_01f98e();
extern void func_020d3e();
extern int func_020e70();
extern char func_020eaa();

/* ROM-only ship-screen interfaces retain their current caller spellings;
 * the ABI alone does not uniquely establish historical C return types. */
extern void ShowShipPieces();
extern void func_037470();

#endif
