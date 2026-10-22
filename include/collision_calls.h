#ifndef TJE_COLLISION_CALLS_H
#define TJE_COLLISION_CALLS_H

/* ROM-only object/terrain collision queries at 025bce-025ff9, used by player
 * movement, earthling movement, and projectile/present effects. The int results
 * retain their current caller interfaces; ABI normalization alone does not
 * uniquely recover the historical C return spellings. */
extern int EntityFindNarrowCollisionAtPosition();
extern int EntityFindMenuAwareCollisionAtPosition();
extern int EntityFindVolumeCollisionAtPosition();
extern int ProjectileFindCollisionExcludingOwner();
extern int EntityFindHorizontalCollisionAtX();

#endif
