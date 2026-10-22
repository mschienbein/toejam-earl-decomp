#ifndef TJE_COLLISION_CALLS_H
#define TJE_COLLISION_CALLS_H

/* ROM-only object/terrain collision queries at 025bce-025ff9, used by player
 * movement, earthling movement, and projectile/present effects. The int results
 * retain their current caller interfaces; ABI normalization alone does not
 * uniquely recover the historical C return spellings. */
extern int func_025bce();
extern int func_025ca4();
extern int func_025d80();
extern int func_025e72();
extern int func_025f42();

#endif
