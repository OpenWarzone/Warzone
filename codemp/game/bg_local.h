#pragma once

// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_local.h -- local definitions for the bg (both games) files

#define	MIN_WALK_NORMAL	0.7f		// can't walk on very steep slopes

#define	TIMER_LAND		130
#define	TIMER_GESTURE	(34*66+50)

#define	OVERCLIP		1.001f
qboolean BG_HaveWeapon ( const playerState_t *ps, int weapon );
qboolean WeaponIsSniperCharge(int weapon);
qboolean WeaponIsSniperNoCharge ( int weapon );
qboolean IsRollWithPistols(int weapon);

//#define __SABER_ANIMATION_SLOW__	// UQ1: Slows down fast, medium, desann and tavion by a little, and dual and staff by a bit more...

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct
{
	vec3_t		forward, right, up;
	float		frametime;

	int			msec;

	qboolean	walking;
	qboolean	ladder;
	qboolean	groundPlane;
	trace_t		groundTrace;

	float		impactSpeed;

	qboolean    ladderforward;
	vec3_t      laddervec;

	vec3_t		previous_origin;
	vec3_t		previous_velocity;
	int			previous_waterlevel;
} pml_t;


extern	pml_t		pml;

// movement parameters
extern	float	pm_stopspeed;
extern	float	pm_duckScale;
extern	float	pm_swimScale;
extern	float	pm_wadeScale;

extern	float	pm_accelerate;
extern	float	pm_airaccelerate;
extern	float	pm_wateraccelerate;
extern	float	pm_flyaccelerate;

extern	float	pm_friction;
extern	float	pm_waterfriction;
extern	float	pm_flightfriction;

extern	int		c_pmove;

extern int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];

//PM anim utility functions:
qboolean PM_SaberInParry( int move );
qboolean PM_SaberInKnockaway( int move );
//[SaberSys]
qboolean PM_KnockawayAnim(int anim);
//[/SaberSys]
qboolean PM_SaberInReflect( int move );
qboolean PM_SaberInStart( int move );
qboolean PM_InSaberAnim( int anim );
qboolean PM_InKnockDown( playerState_t *ps );
qboolean PM_PainAnim( int anim );
qboolean PM_JumpingAnim( int anim );
qboolean PM_LandingAnim( int anim );
qboolean PM_SpinningAnim( int anim );
qboolean PM_InOnGroundAnim ( int anim );
qboolean PM_InRollComplete( playerState_t *ps, int anim );

int PM_AnimLength( int index, animNumber_t anim );

//[NewSaberSys]
//int PM_GetSaberStance(void);
int PM_GetSaberStance();
//[/NewSaberSys]
float PM_GroundDistance(void);
qboolean PM_SomeoneInFront(trace_t *tr);
saberMoveName_t PM_SaberFlipOverAttackMove(void);
saberMoveName_t PM_SaberJumpAttackMove( void );

void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce );
void PM_AddTouchEnt( int entityNum );
void PM_AddEvent( int newEvent );

qboolean	PM_SlideMove( qboolean gravity );
void		PM_StepSlideMove( qboolean gravity );

void PM_StartTorsoAnim( int anim, int blendTime = 500);
void PM_ContinueLegsAnim( int anim );
void PM_ForceLegsAnim( int anim );

void PM_BeginWeaponChange( int weapon );
void PM_FinishWeaponChange( void );

void PM_SetAnim(int setAnimParts, int anim, int setAnimFlags, int blendTime = 100);

void PM_WeaponLightsaber(void);
void PM_SetSaberMove(short newMove);

void PM_SetForceJumpZStart(float value);

void BG_CycleInven(playerState_t *ps, int direction);

//[Melee]
qboolean PM_DoKick(void); //pm function for performing kicks
//[/Melee]

 //[NewSaberSys]
qboolean PM_InGetUpAnimation(int anim);
qboolean BG_InKnockDown(int anim);
qboolean BG_SuperBreakWinAnim(int anim);
qboolean BG_InGetUpAnim(playerState_t *ps);
qboolean PM_StaggerAnim(int anim);
//[/NewSaberSys]

//[SaberSys]
//for now, I've dramatically reduced the cost of the saber special moves to
//racc - force cost of doing cartwheels.
#define SABER_ALT_ATTACK_POWER		50//75?
#define SABER_ALT_ATTACK_POWER_LR	10//30?
#define SABER_ALT_ATTACK_POWER_FB	25//30/50?
//[/SaberSys]