#pragma once

//B_local.h
//re-added by MCG

#include "g_local.h"
#include "b_public.h"
#include "say.h"

#include "ai_dominance.h"

#define	AI_TIMERS 0//turn on to see print-outs of AI/nav timing
//
// Navigation susbsystem
//

#define NAVF_DUCK			0x00000001
#define NAVF_JUMP			0x00000002
#define NAVF_HOLD			0x00000004
#define NAVF_SLOW			0x00000008

#define DEBUG_LEVEL_DETAIL	4
#define DEBUG_LEVEL_INFO	3
#define DEBUG_LEVEL_WARNING	2
#define DEBUG_LEVEL_ERROR	1
#define DEBUG_LEVEL_NONE	0

#define MAX_GOAL_REACHED_DIST_SQUARED	256//16 squared
#define MIN_ANGLE_ERROR 0.01f

#define MIN_ROCKET_DIST_SQUARED 16384//128*128
//
// NPC.cpp
//
// ai debug cvars
extern void NPC_Think ( gentity_t *self);

//NPC_reactions.cpp
extern void NPC_Pain(gentity_t *self, gentity_t *attacker, int damage);
extern void NPC_Touch( gentity_t *self, gentity_t *other, trace_t *trace );
extern void NPC_Use( gentity_t *self, gentity_t *other, gentity_t *activator );
extern float NPC_GetPainChance( gentity_t *self, int damage );

//
// NPC_misc.cpp
//
extern void Debug_Printf( vmCvar_t *cv, int level, char *fmt, ... );
extern void Debug_NPCPrintf( gentity_t *printNPC, vmCvar_t *cv, int debugLevel, char *fmt, ... );

//MCG - Begin============================================================
//NPC_ai variables - shared by NPC.cpp and the following modules
//OJKFIXME: Should probably construct these at the NPC entry points and pass as arguments to any function that needs them
typedef struct npcStatic_s {
	gentity_t		*NPC;
	gNPC_t			*NPCInfo;
	gclient_t		*client;
	usercmd_t		 ucmd;
	visibility_t	 enemyVisibility;
} npcStatic_t;
extern npcStatic_t NPCS;

//AI_Default
extern qboolean NPC_CheckInvestigate(gentity_t *aiEnt, int alertEventNum );
extern qboolean NPC_StandTrackAndShoot (gentity_t *NPC, qboolean canDuck);
extern void NPC_BSIdle(gentity_t *aiEnt);
extern void NPC_BSPointShoot(gentity_t *aiEnt, qboolean shoot);
extern void NPC_BSStandGuard (gentity_t *aiEnt);
extern void NPC_BSPatrol (gentity_t *aiEnt);
extern void NPC_BSHuntAndKill (gentity_t *aiEnt);
extern void NPC_BSStandAndShoot (gentity_t *aiEnt);
extern void NPC_BSRunAndShoot (gentity_t *aiEnt);
extern void NPC_BSWait(gentity_t *aiEnt);
extern void NPC_BSDefault(gentity_t *aiEnt);
extern void NPC_CheckEvasion(gentity_t *aiEnt);

//NPC_behavior
extern void NPC_BSAdvanceFight (gentity_t *aiEnt);
extern void NPC_BSInvestigate (gentity_t *aiEnt);
extern void NPC_BSSleep(gentity_t *aiEnt);
extern void NPC_BSFlee (gentity_t *aiEnt);
extern void NPC_BSFollowLeader (gentity_t *aiEnt);
extern void NPC_BSJump (gentity_t *aiEnt);
extern void NPC_BSRemove (gentity_t *aiEnt);
extern void NPC_BSSearch (gentity_t *aiEnt);
extern void NPC_BSSearchStart (gentity_t *aiEnt, int	homeWp, bState_t bState);
extern void NPC_BSWander (gentity_t *aiEnt);
extern void NPC_BSFlee(gentity_t *aiEnt);
extern void NPC_StartFlee(gentity_t *aiEnt, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax );
extern void G_StartFlee(gentity_t *self, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax );

//NPC_combat
extern int ChooseBestWeapon(gentity_t *aiEnt);
extern void NPC_ChangeWeapon( gentity_t *aiEnt, int newWeapon );
extern void ShootThink(gentity_t *aiEnt);
extern void WeaponThink(gentity_t *aiEnt, qboolean inCombat );
extern qboolean CanShoot ( gentity_t *ent, gentity_t *shooter );
extern void NPC_CheckPossibleEnemy(gentity_t *aiEnt, gentity_t *other, visibility_t vis );
extern gentity_t *NPC_PickEnemy (gentity_t *aiEnt, gentity_t *closestTo, int enemyTeam, qboolean checkVis, qboolean findPlayersFirst, qboolean findClosest);
extern gentity_t *NPC_CheckEnemy (gentity_t *aiEnt, qboolean findNew, qboolean tooFarOk, qboolean setEnemy ); //setEnemy = qtrue
extern qboolean NPC_CheckAttack (gentity_t *aiEnt, float scale);
extern qboolean NPC_CheckDefend (gentity_t *aiEnt, float scale);
extern qboolean NPC_CheckCanAttack (gentity_t *aiEnt, float attack_scale, qboolean stationary);
extern int NPC_AttackDebounceForWeapon (gentity_t *aiEnt);
extern qboolean EntIsGlass (gentity_t *check);
extern qboolean ShotThroughGlass (trace_t *tr, gentity_t *target, vec3_t spot, int mask);
extern qboolean ValidEnemy (gentity_t *aiEnt, gentity_t *ent);
extern void G_ClearEnemy (gentity_t *self);
extern gentity_t *NPC_PickAlly (gentity_t *aiEnt, qboolean facingEachOther, float range, qboolean ignoreGroup, qboolean movingOnly );
extern void NPC_LostEnemyDecideChase(gentity_t *aiEnt);
extern float NPC_MaxDistSquaredForWeapon(gentity_t *aiEnt);
extern qboolean NPC_EvaluateShot(gentity_t *aiEnt, int hit, qboolean glassOK );
extern int NPC_ShotEntity(gentity_t *aiEnt, gentity_t *ent, vec3_t impactPos ); //impactedPos = NULL
extern qboolean NPC_IsAlive (gentity_t *self, gentity_t *NPC);
extern qboolean NPC_EntityIsBreakable ( gentity_t *self, gentity_t *ent );

//NPC_formation
extern qboolean NPC_SlideMoveToGoal (gentity_t *aiEnt);
extern float NPC_FindClosestTeammate (gentity_t *self);
extern void NPC_CalcClosestFormationSpot(gentity_t *self);
extern void G_MaintainFormations (gentity_t *self);
extern void NPC_BSFormation (void);
extern void NPC_CreateFormation (gentity_t *self);
extern void NPC_DropFormation (gentity_t *self);
extern void NPC_ReorderFormation (gentity_t *self);
extern void NPC_InsertIntoFormation (gentity_t *self);
extern void NPC_DeleteFromFormation (gentity_t *self);

#define COLLISION_RADIUS 32
#define NUM_POSITIONS 30

//NPC spawnflags
#define SFB_SMALLHULL	1

#define SFB_RIFLEMAN	2
#define SFB_OLDBORG		2//Borg
#define SFB_PHASER		4
#define SFB_GUN			4//Borg
#define	SFB_TRICORDER	8
#define	SFB_TASER		8//Borg
#define	SFB_DRILL		16//Borg

#define	SFB_CINEMATIC	32
#define	SFB_NOTSOLID	64
#define	SFB_STARTINSOLID 128

//NPC_goal
extern void SetGoal(gentity_t *aiEnt, gentity_t *goal, float rating );
extern void NPC_SetGoal(gentity_t *aiEnt, gentity_t *goal, float rating );
extern void NPC_ClearGoal(gentity_t *aiEnt);
extern void NPC_ReachedGoal(gentity_t *aiEnt);
extern qboolean ReachedGoal(gentity_t *aiEnt, gentity_t *goal );
extern gentity_t *UpdateGoal(gentity_t *aiEnt);
extern qboolean NPC_ClearPathToGoal(gentity_t *aiEnt, vec3_t dir, gentity_t *goal);
extern qboolean NPC_MoveToGoal(gentity_t *aiEnt, qboolean tryStraight );

//NPC_reactions

//NPC_senses
#define	ALERT_CLEAR_TIME	200
#define CHECK_PVS		1
#define CHECK_360		2
#define CHECK_FOV		4
#define CHECK_SHOOT		8
#define CHECK_VISRANGE	16

extern qboolean CanSee (gentity_t *aiEnt, gentity_t *ent );
extern qboolean InFOV ( gentity_t *ent, gentity_t *from, int hFOV, int vFOV );
extern qboolean InFOV2( vec3_t origin, gentity_t *from, int hFOV, int vFOV );
extern qboolean InFOV3( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );
extern visibility_t NPC_CheckVisibility (gentity_t *aiEnt, gentity_t *ent, int flags );
extern qboolean InVisrange (gentity_t *aiEnt, gentity_t *ent );

//NPC_spawn
extern void NPC_Spawn ( gentity_t *ent, gentity_t *other, gentity_t *activator );

//NPC_stats
extern int NPC_ReactionTime (gentity_t *aiEnt);
extern qboolean NPC_ParseParms( const char *NPCName, gentity_t *NPC );
extern void NPC_LoadParms( void );

//NPC_utils
extern int	teamNumbers[FACTION_NUM_FACTIONS];
extern int	teamStrength[FACTION_NUM_FACTIONS];
extern int	teamCounter[FACTION_NUM_FACTIONS];
extern void CalcEntitySpot ( const gentity_t *ent, const spot_t spot, vec3_t point );
extern qboolean NPC_UpdateAngles (gentity_t *aiEnt, qboolean doPitch, qboolean doYaw );
extern void NPC_UpdateShootAngles (gentity_t *aiEnt, vec3_t angles, qboolean doPitch, qboolean doYaw );
extern qboolean NPC_UpdateFiringAngles (gentity_t *aiEnt, qboolean doPitch, qboolean doYaw );
extern void SetTeamNumbers (void);
extern qboolean G_ActivateBehavior (gentity_t *self, int bset );
extern void NPC_AimWiggle(gentity_t *aiEnt, vec3_t enemy_org );
extern void NPC_ClearLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );

//g_nav.cpp
extern int NAV_FindClosestWaypointForEnt (gentity_t *ent, int targWp);
extern qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t *trace, int clipmask );

//NPC_combat
extern float IdealDistance ( gentity_t *self );

//g_squad
extern void NPC_SetSayState (gentity_t *self, gentity_t *to, int saying);

//g_utils
extern qboolean G_CheckInSolid (gentity_t *self, qboolean fix);

//MCG - End============================================================

// NPC.cpp
extern void NPC_SetAnim(gentity_t *ent, int type, int anim, int priority);
extern qboolean NPC_EnemyTooFar(gentity_t *aiEnt, gentity_t *enemy, float dist, qboolean toShoot);

// ==================================================================

//rww - special system for sync'ing bone angles between client and server.
void NPC_SetBoneAngles(gentity_t *ent, char *bone, vec3_t angles);

//rww - and another method of automatically managing surface status for the client and server at once
void NPC_SetSurfaceOnOff(gentity_t *ent, const char *surfaceName, int surfaceFlags);

extern qboolean NPC_ClearLOS(gentity_t *aiEnt, const vec3_t start, const vec3_t end );
extern qboolean NPC_ClearLOS5(gentity_t *aiEnt, const vec3_t end );
extern qboolean NPC_ClearLOS4(gentity_t *aiEnt, gentity_t *ent ) ;
extern qboolean NPC_ClearLOS3(gentity_t *aiEnt, const vec3_t start, gentity_t *ent );
extern qboolean NPC_ClearLOS2(gentity_t *ent, const vec3_t end );

extern qboolean NPC_ClearShot(gentity_t *aiEnt, gentity_t *ent );

extern int NPC_FindCombatPoint(gentity_t *aiEnt, const vec3_t position, const vec3_t avoidPosition, vec3_t enemyPosition, const int flags, const float avoidDist, const int ignorePoint ); //ignorePoint = -1


extern qboolean NPC_ReserveCombatPoint( int combatPointID );
extern qboolean NPC_FreeCombatPoint(gentity_t *aiEnt, int combatPointID, qboolean failed ); //failed = qfalse
extern qboolean NPC_SetCombatPoint(gentity_t *aiEnt, int combatPointID );

#define	CP_ANY			0			//No flags
#define	CP_COVER		0x00000001	//The enemy cannot currently shoot this position
#define CP_CLEAR		0x00000002	//This cover point has a clear shot to the enemy
#define CP_FLEE			0x00000004	//This cover point is marked as a flee point
#define CP_DUCK			0x00000008	//This cover point is marked as a duck point
#define CP_NEAREST		0x00000010	//Find the nearest combat point
#define CP_AVOID_ENEMY	0x00000020	//Avoid our enemy
#define CP_INVESTIGATE	0x00000040	//A special point worth enemy investigation if searching
#define	CP_SQUAD		0x00000080	//Squad path
#define	CP_AVOID		0x00000100	//Avoid supplied position
#define	CP_APPROACH_ENEMY 0x00000200	//Try to get closer to enemy
#define	CP_CLOSEST		0x00000400	//Take the closest combatPoint to the enemy that's available
#define	CP_FLANK		0x00000800	//Pick a combatPoint behind the enemy
#define	CP_HAS_ROUTE	0x00001000	//Pick a combatPoint that we have a route to
#define	CP_SNIPE		0x00002000	//Pick a combatPoint that is marked as a sniper spot
#define	CP_SAFE			0x00004000	//Pick a combatPoint that is not have dangerTime
#define	CP_HORZ_DIST_COLL 0x00008000	//Collect combat points within *horizontal* dist
#define	CP_NO_PVS		0x00010000	//A combat point out of the PVS of enemy pos
#define	CP_RETREAT		0x00020000	//Try to get farther from enemy

#define CPF_NONE		0
#define	CPF_DUCK		0x00000001
#define	CPF_FLEE		0x00000002
#define	CPF_INVESTIGATE	0x00000004
#define	CPF_SQUAD		0x00000008
#define	CPF_LEAN		0x00000010
#define	CPF_SNIPE		0x00000020

#define	MAX_COMBAT_POINT_CHECK	32

extern qboolean NPC_ValidEnemy(gentity_t *aiEnt, gentity_t *ent );
extern qboolean NPC_ValidEnemy2( gentity_t *self, gentity_t *ent );
extern qboolean NPC_CheckEnemyExt(gentity_t *aiEnt, qboolean checkAlerts ); //checkAlerts = qfalse
extern qboolean NPC_CheckCanAttackExt(gentity_t *aiEnt);

extern int NPC_CheckAlertEvents(gentity_t *aiEnt, qboolean checkSight, qboolean checkSound, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel ); //ignoreAlert = -1, mustHaveOwner = qfalse, minAlertLevel = AEL_MINOR
extern qboolean NPC_CheckForDanger(gentity_t *aiEnt, int alertEvent );
extern void G_AlertTeam( gentity_t *victim, gentity_t *attacker, float radius, float soundDist );

extern int NPC_FindSquadPoint( vec3_t position );

extern void ClearPlayerAlertEvents( void );

extern qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
extern qboolean NAV_HitNavGoal( vec3_t point, vec3_t mins, vec3_t maxs, vec3_t dest, int radius, qboolean flying );

extern void NPC_SetMoveGoal( gentity_t *ent, vec3_t point, int radius, qboolean isNavGoal, int combatPoint, gentity_t *targetEnt ); //isNavGoal = qfalse, combatPoint = -1, targetEnt = NULL

extern qboolean NAV_ClearPathToPoint(gentity_t *self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEnt );
extern void NPC_ApplyWeaponFireDelay(gentity_t *aiEnt);

//NPC_FaceXXX suite
extern qboolean NPC_FacePosition(gentity_t *aiEnt, vec3_t position, qboolean doPitch ); //doPitch = qtrue
extern qboolean NPC_FaceEntity(gentity_t *aiEnt, gentity_t *ent, qboolean doPitch ); //doPitch = qtrue
extern qboolean NPC_FaceEnemy(gentity_t *aiEnt, qboolean doPitch ); //doPitch = qtrue

//Skill level cvar
extern vmCvar_t	g_npcspskill;

#define	NIF_NONE		0x00000000
#define	NIF_FAILED		0x00000001	//failed to find a way to the goal
#define	NIF_MACRO_NAV	0x00000002	//using macro navigation
#define	NIF_COLLISION	0x00000004	//resolving collision with an entity
#define NIF_BLOCKED		0x00000008	//blocked from moving

/*
-------------------------
struct navInfo_s
-------------------------
*/

typedef struct navInfo_s
{
	gentity_t	*blocker;
	vec3_t		direction;
	vec3_t		pathDirection;
	float		distance;
	trace_t		trace;
	int			flags;
} navInfo_t;

/*
-------------------------
Storm Trooper Speech
-------------------------
*/

enum
{
	SPEECH_CHASE,
	SPEECH_CONFUSED,
	SPEECH_COVER,
	SPEECH_DETECTED,
	SPEECH_GIVEUP,
	SPEECH_LOOK,
	SPEECH_LOST,
	SPEECH_OUTFLANK,
	SPEECH_ESCAPING,
	SPEECH_SIGHT,
	SPEECH_SOUND,
	SPEECH_SUSPICIOUS,
	SPEECH_YELL,
	SPEECH_PUSHED,
	SPEECH_MAX
};

extern stringID_table_t speechTable[SPEECH_MAX+1];


extern qboolean NPC_Jump( gentity_t *NPC, vec3_t dest );
extern qboolean NPC_TryJump( gentity_t *NPC, vec3_t goal );

extern qboolean UQ1_UcmdMoveForDir ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest );
extern qboolean UQ1_UcmdMoveForDir_NoAvoidance ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest );
extern int	NAV_MoveToGoal( gentity_t *self, navInfo_t *info );
extern void NAV_GetLastMove( navInfo_t *info );
extern qboolean NAV_AvoidCollision( gentity_t *self, gentity_t *goal, navInfo_t *info );
extern int DOM_GetNearWP(vec3_t org, int badwp);
extern qboolean NPC_CombatMoveToGoal( gentity_t *aiEnt, qboolean tryStraight, qboolean retreat );
extern qboolean Jedi_Jump(gentity_t *aiEnt, vec3_t dest, int goalEntNum );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );

extern void NPC_SelectMoveAnimation(gentity_t *aiEnt, qboolean walk);
extern void NPC_PickRandomIdleAnimantion(gentity_t *NPC);
extern qboolean NPC_SelectMoveRunAwayAnimation(gentity_t *aiEnt);

extern qboolean NPC_IsCivilian(gentity_t *NPC);
extern qboolean NPC_IsCivilianHumanoid(gentity_t *NPC);
extern qboolean NPC_IsVendor(gentity_t *NPC);
extern qboolean NPC_IsHumanoid ( gentity_t *self );
extern qboolean NPC_IsJedi ( gentity_t *self );
extern qboolean NPC_IsBoss(gentity_t *self);
extern qboolean NPC_IsLightJedi ( gentity_t *self );
extern qboolean NPC_IsDarkJedi ( gentity_t *self );
extern qboolean NPC_IsAdvancedGunner ( gentity_t *self );
extern qboolean NPC_IsBountyHunter ( gentity_t *self );
extern qboolean NPC_IsCommando ( gentity_t *self );
extern qboolean NPC_IsGunner(gentity_t *self);
extern qboolean NPC_IsNative(gentity_t *self);
extern qboolean NPC_IsFollowerGunner(gentity_t *self);
extern qboolean NPC_IsStormtrooper ( gentity_t *self );
extern qboolean NPC_HasGrenades ( gentity_t *self );

// NPC_AI_Cower.c
extern void NPC_CivilianCowerPoint( gentity_t *enemy, vec3_t position );
extern qboolean NPC_FuturePathIsSafe( gentity_t *self );

// NPC_AI_Jetpack.c
extern void NPC_CheckFlying (gentity_t *aiEnt);
extern qboolean NPC_IsJetpacking ( gentity_t *self );
extern qboolean NPC_JetpackFallingEmergencyCheck (gentity_t *NPC);

// NPC_AI_Padawan.c
extern qboolean NPC_PadawanMove(gentity_t *aiEnt);
extern qboolean NPC_NeedPadawan_Spawn (gentity_t *player);
extern qboolean NPC_NeedFollower_Spawn(gentity_t *player);
extern qboolean Padawan_CheckForce (gentity_t *aiEnt);
extern void NPC_DoPadawanStuff (gentity_t *aiEnt);
extern int NPC_FindPadawanGoal( gentity_t *NPC );
extern void TeleportNPC( gentity_t *player, vec3_t origin, vec3_t angles );

// NPC_AI_Path.c
extern int NPC_GetNextNode(gentity_t *NPC);
extern qboolean NPC_FollowRoutes(gentity_t *aiEnt);
extern qboolean NPC_FollowEnemyRoute(gentity_t *aiEnt);
extern qboolean NPC_ShortenJump(gentity_t *NPC, int node);
extern void NPC_ShortenPath(gentity_t *NPC);
extern qboolean NPC_FindNewWaypoint(gentity_t *aiEnt);
extern void NPC_SetEnemyGoal(gentity_t *aiEnt);
extern qboolean NPC_CopyPathFromNearbyNPC(gentity_t *aiEnt);
extern int NPC_FindGoal( gentity_t *NPC );
extern int NPC_FindTeamGoal( gentity_t *NPC );
extern void NPC_SetNewGoalAndPath(gentity_t *aiEnt);
extern void NPC_SetNewEnemyGoalAndPath(gentity_t *aiEnt);
extern qboolean DOM_NPC_ClearPathToSpot( gentity_t *NPC, vec3_t dest, int impactEntNum );
extern qboolean NPC_PointIsMoverLocation( vec3_t org );
extern void NPC_ClearPathData ( gentity_t *NPC );
extern qboolean NPC_RoutingJumpWaypoint ( int wpLast, int wpCurrent );
extern qboolean NPC_RoutingIncreaseCost ( int wpLast, int wpCurrent );
extern int CheckForFuncAbove(vec3_t org, int ignore);
extern int CheckForFunc(vec3_t org, int ignore);
extern int WaitingForNow(gentity_t *aiEnt, vec3_t goalpos);
extern qboolean NPC_HaveValidEnemy(gentity_t *aiEnt);
extern qboolean NPC_MoverCrushCheck ( gentity_t *NPC );
extern qboolean NPC_GetOffPlayer ( gentity_t *NPC );

// NPC_AI_Patrol.c
extern qboolean NPC_PatrolArea(gentity_t *aiEnt);
