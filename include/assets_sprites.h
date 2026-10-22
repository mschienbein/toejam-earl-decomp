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
extern struct timed_animation ElevatorPrimaryAnimations[13];
extern struct timed_animation ElevatorSecondaryAnimations[13];
extern struct timed_animation PlayerDecoyAnimations[2];
extern struct timed_animation HealthDrainDisplayAnimation[];
extern struct timed_animation ExtraLifeDisplayAnimation[];
extern struct timed_animation RankPromotionDisplayAnimation[];
extern struct timed_animation DoorwayAnimations[];
extern struct timed_animation TransientEffectAnimation;
extern struct animation_descriptor ControlRemapProjectileLeftAnimation;
extern struct animation_descriptor ControlRemapProjectileRightAnimation;
extern struct timed_animation TomatoTargetImpactAnimation;
extern struct timed_animation TomatoGroundImpactAnimation;
extern struct timed_animation TomatoTerrain128ImpactAnimation;
extern struct timed_animation TimedEntityEffectAnimation;
extern struct timed_animation PlayerRainCloudAnimations[];
extern struct timed_animation RetiringTerrainEffectAnimation;
extern struct timed_animation TemporaryTerrainEffectAnimation;
extern struct animation_descriptor PauseDisplayAnimation;
/* Retain the address-spelled view of the player animation table. */
extern struct timed_animation Player2AnimationDescriptors[121];
extern struct animation_descriptor AnimationDescriptors_0AF048;
extern struct animation_descriptor AnimationDescriptors_0AF06E[1];
extern struct animation_descriptor AnimationDescriptors_0AF094[1];

#endif
