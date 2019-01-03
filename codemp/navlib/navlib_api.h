#pragma once

#ifndef __NAVLIB_API_H
#define __NAVLIB_API_H

#ifdef NAVLIB
#include "../game/g_local.h"
#endif //NAVLIB

#ifdef __USE_NAVLIB__

//#define __USE_NAVLIB_INTERNAL_MOVEMENT__

#define MOVE_NONE					0
#define MOVE_FORWARD				1
#define MOVE_BACKWARD				2
#define MOVE_RIGHT					3
#define MOVE_LEFT					4

#define BOT_OBSTACLE_AVOID_RANGE	20.0f

enum navlibPolyFlags
{
	POLYFLAGS_DISABLED = 0,
	POLYFLAGS_WALK = 1 << 0,
	POLYFLAGS_JUMP = 1 << 1,
	POLYFLAGS_POUNCE = 1 << 2,
	POLYFLAGS_WALLWALK = 1 << 3,
	POLYFLAGS_LADDER = 1 << 4,
	POLYFLAGS_DROPDOWN = 1 << 5,
	POLYFLAGS_DOOR = 1 << 6,
	POLYFLAGS_TELEPORT = 1 << 7,
	POLYFLAGS_CROUCH = 1 << 8,
	POLYFLAGS_SWIM = 1 << 9,
	POLYFLAGS_ALL = 0xffff, // All abilities.
};

enum navlibPolyAreas
{
	POLYAREA_GROUND = 1 << 0,
	POLYAREA_LADDER = 1 << 1,
	POLYAREA_WATER = 1 << 2,
	POLYAREA_DOOR = 1 << 3,
	POLYAREA_JUMPPAD = 1 << 4,
	POLYAREA_TELEPORTER = 1 << 5,
};

//route status flags
#define ROUTE_FAILED  ( 1u << 31 )
#define	ROUTE_SUCCEED ( 1u << 30 )
#define	ROUTE_PARTIAL ( 1 << 6 )

struct navlibTrace_t
{
	float frac;
	float normal[3];
};

enum class navlibRouteTargetType_t
{
	BOT_TARGET_STATIC, // target stays in one place always
	BOT_TARGET_DYNAMIC // target can move
};

// type: determines if the object can move or not
// pos: the object's position 
// polyExtents: how far away from pos to search for a nearby navmesh polygon for finding a route
struct navlibRouteTarget_t
{
	navlibRouteTargetType_t type;
	float pos[3];
	float polyExtents[3];
};

struct navlibClass_t
{
	char						name[64];
	unsigned short				polyFlagsInclude;
	unsigned short				polyFlagsExclude;
};

#ifndef NAVLIB
// output from navigation - Moved to g_local.h, because includes in 2 directions is a fucking bitch...
struct navlibNavCmd_t
{
	float						pos[3];
	float						tpos[3];
	float						dir[3];
	float						lookPos[3];
	int							directPathToGoal;
	int							havePath;
};

typedef struct navlibTargetGoal_t
{
	float						origin[3];
	gentity_t					*ent;
	qboolean					haveGoal;
};

typedef struct navlibTarget_t
{
	navlibTargetGoal_t			goal;
	navlibNavCmd_t				nav;
};
#endif //NAVLIB

//
// API Commands... Most stuff should use the second section...
//


namespace Navlib
{
	extern bool			NavlibSetup(const navlibClass_t *botClass, qhandle_t *navHandle);
	extern void			NavlibShutdown(void);
	extern bool			G_NavmeshIsLoaded(void);
	extern void			NavlibDisableArea(const vec3_t origin, const vec3_t mins, const vec3_t maxs);
	extern void			NavlibEnableArea(const vec3_t origin, const vec3_t mins, const vec3_t maxs);
	extern void			NavlibSetNavMesh(int npcEntityNum, qhandle_t nav);
	extern bool			NavlibFindRoute(int npcEntityNum, const navlibRouteTarget_t *target, bool allowPartial);
	extern void			NavlibUpdateCorridor(int npcEntityNum, const navlibRouteTarget_t *target, navlibNavCmd_t *cmd);
	extern void			NavlibFindRandomPoint(int npcEntityNum, vec3_t point);
	extern void			NavlibFindRandomPatrolPoint(int npcEntityNum, vec3_t point);
	extern bool			NavlibFindRandomPointInRadius(int npcEntityNum, const vec3_t origin, vec3_t point, float radius);
	extern bool			NavlibNavTrace(navlibTrace_t *trace, vec3_t start, vec3_t end, int npcEntityNum);
	extern void			NavlibAddObstacle(const vec3_t mins, const vec3_t maxs, qhandle_t *obstacleHandle);
	extern void			NavlibRemoveObstacle(qhandle_t obstacleHandle);
	extern void			NavlibUpdateObstacles(void);

	extern void			G_NavlibNavInit(void);
	extern void			G_NavlibNavCleanup(void);
	extern void			G_NavlibDisableArea(vec3_t origin, vec3_t mins, vec3_t maxs);
	extern void			G_NavlibEnableArea(vec3_t origin, vec3_t mins, vec3_t maxs);
	extern void			NavlibSetNavmesh(gentity_t  *self, class_t newClass);
	extern qboolean		NavlibTargetIsEntity(navlibTargetGoal_t *goal);
	extern void			NavlibGetTargetPos(navlibTargetGoal_t *goal, vec3_t pos);
	extern float		NavlibGetGoalRadius(gentity_t *self);
	extern bool			GoalInRange(gentity_t *self, float r);
	extern int			DistanceToGoal2DSquared(gentity_t *self);
	extern int			DistanceToGoal(gentity_t *self);
	extern int			DistanceToGoalSquared(gentity_t *self);
	extern bool			NavlibPathIsWalkable(gentity_t *self, navlibTargetGoal_t target);
	extern void			NavlibFindRandomPointOnMesh(gentity_t *self, vec3_t point);
	extern signed char	NavlibGetMaxMoveSpeed(gentity_t *self);
	extern void			NavlibStrafeDodge(gentity_t *self);
	extern void			NavlibMoveInDir(gentity_t *self, uint32_t dir);
	extern void			NavlibAlternateStrafe(gentity_t *self);
	extern bool			NavlibJump(gentity_t *self);
	extern void			NavlibWalk(gentity_t *self, bool enable);
	extern void			NavlibStandStill(gentity_t *self);
	extern gentity_t*	NavlibGetPathBlocker(gentity_t *self, const vec3_t dir);
	extern bool			NavlibShouldJump(gentity_t *self, gentity_t *blocker, const vec3_t dir);
	extern bool			NavlibFindSteerTarget(gentity_t *self, vec3_t dir);
	extern bool			NavlibAvoidObstacles(gentity_t *self, vec3_t dir);
	extern bool			NavlibOnLadder(gentity_t *self);
	extern void			NavlibDirectionToUsercmd(gentity_t *self, vec3_t dir, usercmd_t *cmd);
	extern void			NavlibFacePosition(gentity_t *self, vec3_t seekPos);
	extern void			NavlibSeek(gentity_t *self, vec3_t direction);
	extern void			NavlibClampPos(gentity_t *self);
	extern void			NavlibMoveToGoal(gentity_t *self);
	extern void			NavlibTargetToRouteTarget(gentity_t *self, navlibTargetGoal_t target, navlibRouteTarget_t *routeTarget);
	extern bool			NavlibFindRouteToTarget(gentity_t *self, navlibTargetGoal_t target, bool allowPartial);
}

using namespace Navlib;

#endif //__USE_NAVLIB__

#endif //__NAVLIB_API_H
