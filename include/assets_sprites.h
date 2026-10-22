#ifndef TJE_ASSETS_SPRITES_H
#define TJE_ASSETS_SPRITES_H

#include "sprites.h"

/* Shared sprite and animation ROM tables, translated for each revision. */
extern struct timed_animation EarthlingCompanionAnimations[9];
extern struct animation_descriptor TomatoProjectileAnimations[3];
extern struct timed_animation PlayerControlRemapAnimations[5];
extern struct timed_animation ActiveTerrainEffectAnimations[3];
extern struct animation_descriptor TelephoneAnimations[2];
extern struct timed_animation PlayerWakeEffectAnimations[2];
extern struct animation_descriptor BoomBoxAnimations[1];
extern char WakeWhisperAnimationStreams[4];
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
/* Separate descriptor-table view used for the second player. */
extern struct timed_animation Player2AnimationDescriptors[121];
extern struct animation_descriptor ShipPieceCollectionAnimation;
extern struct animation_descriptor PlayerInteractionChoiceAnimations[1];
extern struct animation_descriptor MoleEffectAnimations[1];

#endif
