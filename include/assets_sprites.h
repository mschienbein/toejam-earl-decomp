#ifndef TJE_ASSETS_SPRITES_H
#define TJE_ASSETS_SPRITES_H

#include "sprites.h"

/* Shared sprite and animation ROM tables. Address-spelled names retain
 * their REV 02 identities when the build translates them for REV 00. */
extern struct timed_animation AnimationDescriptors_0AEF9C[9];
extern struct animation_descriptor AnimationDescriptors_0AEFF6[3];
extern struct timed_animation AnimationDescriptors_0AF00E[5];
extern struct timed_animation AnimationDescriptors_0AF076[3];
extern struct animation_descriptor AnimationDescriptors_0AF0A8[2];
extern struct timed_animation AnimationDescriptors_0AF0B8[2];
extern struct animation_descriptor AnimationDescriptors_0AF0D0[1];
extern char InlineAnimationPrograms_AF0CC[4];
extern struct timed_animation PlayerAnimationDescriptors[121];
extern struct animation_descriptor SleepAnimationDescriptors[23];
extern unsigned char SpritePriorityLookup[26];
extern unsigned char SpriteTileAllocationLookup[8];
extern struct timed_animation dat_0aee98[13];
extern struct timed_animation dat_0aef1a[13];
extern struct timed_animation dat_0aefa6[2];
extern struct timed_animation dat_0aefba[];
extern struct timed_animation dat_0aefc4[];
extern struct timed_animation dat_0aefce[];
extern struct timed_animation dat_0aefd8[];
extern struct timed_animation dat_0aefec;
extern struct animation_descriptor dat_0aeffe;
extern struct animation_descriptor dat_0af006;
extern struct timed_animation dat_0af018;
extern struct timed_animation dat_0af022;
extern struct timed_animation dat_0af02c;
extern struct timed_animation dat_0af036;
extern struct timed_animation dat_0af05a[];
extern struct timed_animation dat_0af080;
extern struct timed_animation dat_0af08a;
extern struct animation_descriptor dat_0af0b0;
/* Retain the address-spelled view of the player animation table. */
extern struct timed_animation rom_0ae9de[121];
extern struct animation_descriptor AnimationDescriptors_0AF048;
extern struct animation_descriptor AnimationDescriptors_0AF06E[1];
extern struct animation_descriptor AnimationDescriptors_0AF094[1];

#endif
