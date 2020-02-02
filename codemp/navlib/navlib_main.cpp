/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include <windows.h>
#include "navlib_api.h"

//tells if all navmeshes loaded successfully
#define			PCL_NONE			0
#define			PCL_HUMANOID		1
#define			PCL_NUM_CLASSES		PCL_HUMANOID

bool			navMeshLoaded = false;

int				PCL_HUMANOID_NAVHANDLE = 0;

extern float navmeshScale;
extern float navmeshScaleInv;

/*
========================
Navigation Mesh Loading
========================
*/

void Navlib::G_NavlibNavInit( void )
{
	if (!navMeshLoaded)
	{
		navlibClass_t bot;
		strcpy(bot.name, "humanoid");
		bot.polyFlagsInclude = POLYFLAGS_WALK;
		bot.polyFlagsExclude = POLYFLAGS_DISABLED;

		if (!Navlib::NavlibSetup(&bot, &PCL_HUMANOID_NAVHANDLE))
		{
			return;
		}

		navMeshLoaded = true;
	}
}

void Navlib::G_NavlibNavCleanup( void )
{
	Navlib::NavlibShutdown();
	navMeshLoaded = false;
}

bool Navlib::G_NavmeshIsLoaded( void )
{
	return navMeshLoaded;
}

extern float navmeshScaleInv;

void Navlib::G_NavlibDisableArea( vec3_t origin, vec3_t mins, vec3_t maxs )
{
	vec3_t org, min, max;
	VectorScale(origin, navmeshScaleInv, org);
	VectorScale(mins, navmeshScaleInv, min);
	VectorScale(maxs, navmeshScaleInv, max);
	Navlib::NavlibDisableArea( org, min, max );
}

void Navlib::G_NavlibEnableArea( vec3_t origin, vec3_t mins, vec3_t maxs )
{
	vec3_t org, min, max;
	VectorScale(origin, navmeshScaleInv, org);
	VectorScale(mins, navmeshScaleInv, min);
	VectorScale(maxs, navmeshScaleInv, max);
	Navlib::NavlibEnableArea(org, min, max);
}

void Navlib::NavlibSetNavmesh( gentity_t  *self, class_t newClass )
{
	int navHandle;

	if ( newClass == PCL_NONE )
	{
		return;
	}

	navHandle = PCL_HUMANOID_NAVHANDLE;
	/*navHandle = model->navMeshClass
	          ? BG_ClassModelConfig( model->navMeshClass )->navHandle
	          : model->navHandle;*/

	Navlib::NavlibSetNavMesh( self->s.number, navHandle );
}

/*
========================
Navlib Navigation Querys
========================
*/
float RadiusFromBounds2D( vec3_t mins, vec3_t maxs )
{
	float rad1s = Square( mins[0] ) + Square( mins[1] );
	float rad2s = Square( maxs[0] ) + Square( maxs[1] );
	return sqrt( max( rad1s, rad2s ) );
}

qboolean Navlib::NavlibTargetIsEntity(navlibTargetGoal_t *goal)
{
	if (goal->ent)
		return qtrue;

	return qfalse;
}

void Navlib::NavlibGetTargetPos(navlibTargetGoal_t *goal, vec3_t pos)
{
	if (NavlibTargetIsEntity(goal))
	{
		VectorCopy(goal->ent->r.currentOrigin, pos);
		return;
	}

	VectorCopy(goal->origin, pos);
}

float Navlib::NavlibGetGoalRadius( gentity_t *self )
{
	if ( NavlibTargetIsEntity( &self->client->navigation.goal ) )
	{
		navlibTargetGoal_t *t = &self->client->navigation.goal;
		return (RadiusFromBounds2D( t->ent->r.mins, t->ent->r.maxs ) + RadiusFromBounds2D( self->r.mins, self->r.maxs )) * 4.0 * navmeshScaleInv;
	}
	else
	{
		return RadiusFromBounds2D( self->r.mins, self->r.maxs ) * 4.0 * navmeshScaleInv;
	}
}

gentity_t *G_IterateEntitiesWithinRadius(gentity_t *entity, vec3_t origin, float radius)
{
	vec3_t eorg;
	int    j;

	if (!entity)
	{
		entity = g_entities;
	}
	else
	{
		entity++;
	}

	for (; entity < &g_entities[level.num_entities]; entity++)
	{
		if (!entity->inuse)
		{
			continue;
		}

		for (j = 0; j < 3; j++)
		{
			eorg[j] = origin[j] - (entity->r.currentOrigin[j] + (entity->r.mins[j] + entity->r.maxs[j]) * 0.5);
		}

		if (VectorLength(eorg) > radius)
		{
			continue;
		}

		return entity;
	}

	return nullptr;
}

bool Navlib::GoalInRange( gentity_t *self, float r )
{
	gentity_t *ent = nullptr;

	if ( !NavlibTargetIsEntity( &self->client->navigation.goal ) )
	{
		vec3_t goalPos;
		VectorCopy(self->client->navigation.nav.tpos, goalPos);

		if (self->client->ps.eFlags & EF_JETPACK_ACTIVE)
		{// Jetpack is active, move the goal height to our height for the distance check...
			goalPos[2] = self->r.currentOrigin[2];
		}

		if (self->waterlevel > 1)
		{// In water, move the goal to our height for the distance check...
			goalPos[2] = self->r.currentOrigin[2];
		}

		return ( Distance( self->r.currentOrigin, goalPos ) < r*navmeshScale);
	}

	if ((self->client->ps.eFlags & EF_JETPACK_ACTIVE) || self->waterlevel > 1)
	{// Jetpack or in water, move the goal height to our height for the distance check...
		vec3_t goalPos;
		VectorCopy(self->client->navigation.goal.ent->r.currentOrigin, goalPos);
		goalPos[2] = self->r.currentOrigin[2];
		return (Distance(self->r.currentOrigin, goalPos) < r*navmeshScale);
	}
	else
	{
		while ((ent = G_IterateEntitiesWithinRadius(ent, self->r.currentOrigin, r*navmeshScale)))
		{
			if (ent == self->client->navigation.goal.ent)
			{
				return true;
			}
		}
	}
	return false;
}

int Navlib::DistanceToGoal2DSquared( gentity_t *self )
{
	vec3_t vec;
	vec3_t goalPos;

	NavlibGetTargetPos( &self->client->navigation.goal, goalPos );

	VectorSubtract( goalPos, self->r.currentOrigin, vec );

	return Square( vec[ 0 ] ) + Square( vec[ 1 ] );
}

int Navlib::DistanceToGoal( gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	/*if ( !( self->client->navigation ) )
	{
		return -1;
	}*/
	NavlibGetTargetPos( &self->client->navigation.goal, targetPos );
	VectorCopy( self->r.currentOrigin, selfPos );
	return Distance( selfPos, targetPos );
}

int Navlib::DistanceToGoalSquared( gentity_t *self )
{
	vec3_t targetPos;
	vec3_t selfPos;
	//safety check for morons who use this incorrectly
	/*if ( !( self->client->navigation ) )
	{
		return -1;
	}*/
	NavlibGetTargetPos( &self->client->navigation.goal, targetPos );
	VectorCopy( self->r.currentOrigin, selfPos );
	return DistanceSquared( selfPos, targetPos );
}

void NavlibGetClientNormal(playerState_t *ps, vec3_t normal)
{
	//AngleVectors(ps->viewangles, normal, NULL, NULL);
	VectorSet(normal, 0.0f, 0.0f, 1.0f);
}

bool Navlib::NavlibPathIsWalkable( gentity_t *self, navlibTargetGoal_t target )
{
	vec3_t selfPos, targetPos;
	vec3_t viewNormal;
	navlibTrace_t trace;

	NavlibGetClientNormal( &self->client->ps, viewNormal );
	VectorMA( self->r.currentOrigin, self->r.mins[2], viewNormal, selfPos );
	NavlibGetTargetPos( &target, targetPos );

	if ( !Navlib::NavlibNavTrace( &trace, selfPos, targetPos, self->s.number) )
	{
		return false;
	}

	if ( trace.frac >= 1.0f )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Navlib::NavlibFindRandomPointOnMesh( gentity_t *self, vec3_t point )
{
	if (!self)
		Navlib::NavlibFindRandomPoint( -1, point) ;
	else
		Navlib::NavlibFindRandomPoint( self->s.number, point );
}

qboolean usercmdButtonPressed(int cmdButtons, int button)
{
	if (cmdButtons & button)
		return qtrue;

	return qfalse;
}

void usercmdPressButton(int buttons, int button)
{
	buttons |= button;
}

void usercmdReleaseButton(int buttons, int button)
{
	buttons &= ~button;
}

/*
========================
Local Navlib Navigation
========================
*/

signed char Navlib::NavlibGetMaxMoveSpeed( gentity_t *self )
{
	if ( usercmdButtonPressed( self->client->pers.cmd.buttons, BUTTON_WALKING ) )
	{
		return 63;
	}

	return 127;
}

void Navlib::NavlibStrafeDodge( gentity_t *self )
{
	usercmd_t *cmdBuffer = &self->client->pers.cmd;
	signed char speed = NavlibGetMaxMoveSpeed( self );

	if (self->bot_strafe_right_timer)
	{
		cmdBuffer->rightmove = speed;
	}
	else
	{
		cmdBuffer->rightmove = -speed;
	}

#if 0
	if ( ( self->client->time10000 % 2000 ) < 1000 )
	{
		cmdBuffer->rightmove *= -1;
	}

	if ( ( self->client->time1000 % 300 ) >= 100 && ( self->client->time10000 % 3000 ) > 2000 )
	{
		cmdBuffer->rightmove = 0;
	}
#endif
}

void Navlib::NavlibMoveInDir( gentity_t *self, uint32_t dir )
{
	usercmd_t *cmdBuffer = &self->client->pers.cmd;
	signed char speed = NavlibGetMaxMoveSpeed( self );

	if ( dir & MOVE_FORWARD )
	{
		cmdBuffer->forwardmove = speed;
	}
	else if ( dir & MOVE_BACKWARD )
	{
		cmdBuffer->forwardmove = -speed;
	}

	if ( dir & MOVE_RIGHT )
	{
		cmdBuffer->rightmove = speed;
	}
	else if ( dir & MOVE_LEFT )
	{
		cmdBuffer->rightmove = -speed;
	}
}

void Navlib::NavlibAlternateStrafe( gentity_t *self )
{
	usercmd_t *cmdBuffer = &self->client->pers.cmd;
	signed char speed = NavlibGetMaxMoveSpeed( self );

	if ( level.time % 8000 < 4000 )
	{
		cmdBuffer->rightmove = speed;
	}
	else
	{
		cmdBuffer->rightmove = -speed;
	}
}

bool Navlib::NavlibJump( gentity_t *self )
{
	self->client->pers.cmd.upmove = 127;

	if (self->s.eType == ET_PLAYER)
	{
		trap->EA_Jump(self->s.number);
	}

	return true;
}

void Navlib::NavlibWalk( gentity_t *self, bool enable )
{
	usercmd_t *cmdBuffer = &self->client->pers.cmd;

	if ( !enable )
	{
		if ( usercmdButtonPressed( cmdBuffer->buttons, BUTTON_WALKING ) )
		{
			usercmdReleaseButton( cmdBuffer->buttons, BUTTON_WALKING );
			cmdBuffer->forwardmove *= 2;
			cmdBuffer->rightmove *= 2;
		}
		return;
	}

	if ( !usercmdButtonPressed( cmdBuffer->buttons, BUTTON_WALKING ) )
	{
		usercmdPressButton( cmdBuffer->buttons, BUTTON_WALKING );
		cmdBuffer->forwardmove /= 2;
		cmdBuffer->rightmove /= 2;
	}
}

void Navlib::NavlibStandStill(gentity_t *self)
{
	usercmd_t *cmdBuffer = &self->client->pers.cmd;

	NavlibWalk(self, false);
	//NavlibSprint( self, false );
	cmdBuffer->forwardmove = 0;
	cmdBuffer->rightmove = 0;
	cmdBuffer->upmove = 0;
}

gentity_t* Navlib::NavlibGetPathBlocker( gentity_t *self, const vec3_t dir )
{
	vec3_t playerMins, playerMaxs;
	vec3_t end;
	trace_t trace;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;

	if ( !( self && self->client ) )
	{
		return nullptr;
	}

	VectorCopy(self->r.mins, playerMins);
	VectorCopy(self->r.maxs, playerMaxs);

	//account for how large we can step
	playerMins[2] += STEPSIZE /*+ 24.0*/;
	playerMaxs[2] += STEPSIZE /*+ 24.0*/;

	VectorMA( self->r.currentOrigin, TRACE_LENGTH, dir, end );

	trap->Trace( &trace, self->r.currentOrigin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0, 0, 0 );

	if ( trace.fraction < 1.0f && trace.plane.normal[ 2 ] < 0.7f )
	{
		return &g_entities[trace.entityNum];
	}

	return nullptr;
}

bool Navlib::NavlibShouldJump( gentity_t *self, gentity_t *blocker, const vec3_t dir )
{
	vec3_t playerMins;
	vec3_t playerMaxs;
	float jumpMagnitude;
	trace_t trace;
	const int TRACE_LENGTH = BOT_OBSTACLE_AVOID_RANGE;
	vec3_t end;

	//blocker is not on our team, so ignore
	if (blocker && !OnSameTeam(self, blocker))
	{
		return false;
	}

	VectorCopy(self->r.mins, playerMins);
	VectorCopy(self->r.maxs, playerMaxs);

	playerMins[2] += STEPSIZE /*+ 24.0*/;
	playerMaxs[2] += STEPSIZE /*+ 24.0*/;

	VectorMA( self->r.currentOrigin, TRACE_LENGTH, dir, end );

	//make sure we are moving into a block
	trap->Trace( &trace, self->r.currentOrigin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0, 0, 0 );

	if ( trace.fraction >= 1.0f || (blocker && (trace.entityNum != ENTITYNUM_WORLD) && blocker != &g_entities[trace.entityNum]) )
	{
		return false;
	}

	jumpMagnitude = 16.0;

	while (jumpMagnitude < 512.0)
	{
		//find the actual height of our jump
		//jumpMagnitude = Square(jumpMagnitude) / (self->client->ps.gravity * 2);

		//prepare for trace
		playerMins[2] += jumpMagnitude;
		playerMaxs[2] += jumpMagnitude;

		//check if jumping will clear us of entity
		trap->Trace(&trace, self->r.currentOrigin, playerMins, playerMaxs, end, self->s.number, MASK_SHOT, 0, 0, 0);

		//if we can jump over it, then jump
		//note that we also test for a blocking barricade because barricades will collapse to let us through
		if (trace.fraction == 1.0f || (jumpMagnitude <= 64 && (trace.startsolid || trace.allsolid)))
		{
			return true;
		}

		jumpMagnitude += jumpMagnitude;
	}

	return qfalse;
}

bool Navlib::NavlibFindSteerTarget( gentity_t *self, vec3_t dir )
{
#if 0
	vec3_t forward;
	vec3_t testPoint1, testPoint2;
	vec3_t playerMins, playerMaxs;
	float yaw1, yaw2;
	trace_t trace1, trace2;
	int i;
	vec3_t angles;

	if ( !( self && self->client ) )
	{
		return false;
	}

	//get bbox
	VectorCopy(self->r.mins, playerMins);
	VectorCopy(self->r.maxs, playerMaxs);

	//account for stepsize
	playerMins[2] += STEPSIZE /*+ 24.0*/;
	playerMaxs[2] += STEPSIZE /*+ 24.0*/;

	//get the yaw (left/right) we dont care about up/down
	vectoangles( dir, angles );
	yaw1 = yaw2 = angles[ YAW ];

	//directly infront of us is blocked, so dont test it
	yaw1 -= 15;
	yaw2 += 15;

	//forward vector is 2D
	forward[ 2 ] = 0;

	//find an unobstructed position
	//we check the full 180 degrees in front of us
	for ( i = 0; i < 5; i++, yaw1 -= 15 , yaw2 += 15 )
	{
		//compute forward for right
		forward[0] = cos( DEG2RAD( yaw1 ) );
		forward[1] = sin( DEG2RAD( yaw1 ) );
		//forward is already normalized
		//try the right
		VectorMA( self->r.currentOrigin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint1 );

		//test it
		trap->Trace( &trace1, self->r.currentOrigin, playerMins, playerMaxs, testPoint1, self->s.number, MASK_SHOT, 0, 0, 0 );

		//check if unobstructed
		if ( trace1.fraction >= 1.0f )
		{
			VectorCopy( forward, dir );
			return true;
		}

		//compute forward for left
		forward[0] = cos( DEG2RAD( yaw2 ) );
		forward[1] = sin( DEG2RAD( yaw2 ) );
		//forward is already normalized
		//try the left
		VectorMA( self->r.currentOrigin, BOT_OBSTACLE_AVOID_RANGE, forward, testPoint2 );

		//test it
		trap->Trace( &trace2, self->r.currentOrigin, playerMins, playerMaxs, testPoint2, self->s.number, MASK_SHOT, 0, 0, 0 );

		//check if unobstructed
		if ( trace2.fraction >= 1.0f )
		{
			VectorCopy( forward, dir );
			return true;
		}
	}

	//we couldnt find a new position
	return false;
#else
	return true;
#endif
}

bool Navlib::NavlibAvoidObstacles( gentity_t *self, vec3_t dir )
{
	gentity_t *blocker;

	blocker = NavlibGetPathBlocker( self, dir );

	if ( blocker )
	{
		if ( NavlibShouldJump( self, blocker, dir ) )
		{
			NavlibJump( self );
			return false;
		}
		else if ( !NavlibFindSteerTarget( self, dir ) )
		{
			vec3_t angles;
			vec3_t right;
			vectoangles( dir, angles );
			AngleVectors( angles, dir, right, nullptr );

			if ( self->bot_strafe_right_timer )
			{
				VectorCopy( right, dir );
				blocker->bot_strafe_left_timer = level.time + 500;
			}
			else
			{
				VectorNegate( right, dir );
				blocker->bot_strafe_right_timer = level.time + 500;
			}

			dir[ 2 ] = 0;
			VectorNormalize( dir );
		}

		return true;
	}
	else
	{
		if (NavlibShouldJump(self, NULL, dir))
		{
			NavlibJump(self);
			return false;
		}
		/*else if (!NavlibFindSteerTarget(self, dir))
		{
			vec3_t angles;
			vec3_t right;
			vectoangles(dir, angles);
			AngleVectors(angles, dir, right, nullptr);

			if (self->bot_strafe_right_timer)
			{
				VectorCopy(right, dir);
			}
			else
			{
				VectorNegate(right, dir);
			}

			dir[2] = 0;
			VectorNormalize(dir);
		}*/
	}

	return false;
}

//copy of PM_CheckLadder in bg_pmove.c
bool Navlib::NavlibOnLadder( gentity_t *self )
{
#if 0
	vec3_t forward, end;
	vec3_t mins, maxs;
	trace_t trace;

	if ( !BG_ClassHasAbility( ( class_t ) self->client->ps.stats[ STAT_CLASS ], SCA_CANUSELADDERS ) )
	{
		return false;
	}

	AngleVectors( self->client->ps.viewangles, forward, nullptr, nullptr );

	forward[ 2 ] = 0.0f;
	BG_ClassBoundingBox( ( class_t ) self->client->ps.stats[ STAT_CLASS ], mins, maxs, nullptr, nullptr, nullptr );
	VectorMA( self->r.currentOrigin, 1.0f, forward, end );

	trap->Trace( &trace, self->r.currentOrigin, mins, maxs, end, self->s.number, MASK_PLAYERSOLID, 0 );

	if ( trace.fraction < 1.0f && trace.surfaceFlags & SURF_LADDER )
	{
		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

void Navlib::NavlibDirectionToUsercmd( gentity_t *self, vec3_t dir, usercmd_t *cmd )
{
	vec3_t forward;
	vec3_t right;

	float forwardmove;
	float rightmove;
	signed char speed = NavlibGetMaxMoveSpeed( self );

	AngleVectors( self->client->ps.viewangles, forward, right, nullptr );
	forward[2] = 0;
	VectorNormalize( forward );
	right[2] = 0;
	VectorNormalize( right );

	// get direction and non-optimal magnitude
	forwardmove = speed * DotProduct( forward, dir );
	rightmove = speed * DotProduct( right, dir );

	// find optimal magnitude to make speed as high as possible
	if ( Q_fabs( forwardmove ) > Q_fabs( rightmove ) )
	{
		float highestforward = forwardmove < 0 ? -speed : speed;

		float highestright = highestforward * rightmove / forwardmove;

		cmd->forwardmove = ClampChar( highestforward );
		cmd->rightmove = ClampChar( highestright );
	}
	else
	{
		float highestright = rightmove < 0 ? -speed : speed;

		float highestforward = highestright * forwardmove / rightmove;

		cmd->forwardmove = ClampChar( highestforward );
		cmd->rightmove = ClampChar( highestright );
	}

	if (self->s.eType == ET_PLAYER)
	{
		if (self->client->pers.cmd.buttons & BUTTON_WALKING)
		{
			trap->EA_Action(self->s.number, 0x0080000);
			trap->EA_Move(self->s.number, dir, 100);

			if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(self->s.number);
		}
		else
		{
			trap->EA_Move(self->s.number, dir, 200);

			if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(self->s.number);
		}
	}
}

void Navlib::NavlibFacePosition(gentity_t *self, vec3_t seekPos)
{
	VectorCopy(seekPos, self->client->navigation.nav.lookPos);
}

void Navlib::NavlibSeek( gentity_t *self, vec3_t direction )
{
	vec3_t viewOrigin;
	vec3_t seekPos;

	VectorCopy( self->client->ps.origin, viewOrigin );

	VectorNormalize( direction );

	VectorMA(viewOrigin, 100, direction, seekPos);

	// slowly change our aim to point to the target
	NavlibFacePosition(self, seekPos);

	// move directly toward the target
#ifdef __USE_NAVLIB_INTERNAL_MOVEMENT__
	NavlibDirectionToUsercmd( self, direction, &self->client->pers.cmd );
#endif //__USE_NAVLIB_INTERNAL_MOVEMENT__
	
	//VectorCopy(direction, self->client->navigation.nav.dir);
	//VectorCopy(seekPos, self->client->navigation.nav.pos);
}

/*
=========================
Global Navlib Navigation
=========================
*/

void Navlib::NavlibClampPos( gentity_t *self )
{
	float height = self->client->ps.origin[ 2 ];
	vec3_t origin;
	trace_t trace;
	vec3_t mins, maxs;
	VectorSet( origin, self->client->navigation.nav.pos[ 0 ], self->client->navigation.nav.pos[ 1 ], height );
	VectorCopy(self->r.mins, mins);
	VectorCopy(self->r.maxs, maxs);
	trap->Trace( &trace, self->client->ps.origin, mins, maxs, origin, self->client->ps.clientNum, MASK_PLAYERSOLID, 0, 0, 0 );
	G_SetOrigin( self, trace.endpos );
	VectorCopy( trace.endpos, self->client->ps.origin );
}

void Navlib::NavlibMoveToGoal( gentity_t *self )
{
//	int    staminaJumpCost;
	vec3_t dir;

	navlibRouteTarget_t routeTarget;
	NavlibTargetToRouteTarget(self, self->client->navigation.goal, &routeTarget);
	NavlibUpdateCorridor(self->s.number, &routeTarget, &self->client->navigation.nav);

	VectorCopy( self->client->navigation.nav.dir, dir );

	if ( dir[ 2 ] < 0 )
	{
		dir[ 2 ] = 0;
		VectorNormalize( dir );
	}

	NavlibSeek(self, dir);
#ifdef __USE_NAVLIB_INTERNAL_MOVEMENT__
	NavlibAvoidObstacles( self, dir );
#endif //__USE_NAVLIB_INTERNAL_MOVEMENT__

#if 0
	staminaJumpCost = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->staminaJumpCost;

	//dont sprint or dodge if we dont have enough stamina and are about to slow
	if ( self->client->pers.team == TEAM_HUMANS
	     && self->client->ps.stats[ STAT_STAMINA ] < staminaJumpCost )
	{
		usercmd_t *cmdBuffer = &self->client->pers.cmd.buttons;

		usercmdReleaseButton( cmdBuffer->buttons, BUTTON_SPRINT );
		usercmdReleaseButton( cmdBuffer->buttons, BUTTON_DODGE );

		// walk to regain stamina
		NavlibWalk( self, true );
	}
#endif
}

void Navlib::NavlibTargetToRouteTarget(gentity_t *self, navlibTargetGoal_t target, navlibRouteTarget_t *routeTarget)
{
	vec3_t mins, maxs;
	int i;

	if (NavlibTargetIsEntity(&target))
	{
		VectorCopy(target.ent->r.mins, mins);
		VectorCopy(target.ent->r.maxs, maxs);

		if (target.ent->s.eType == ET_PLAYER || target.ent->s.eType == ET_NPC)
		{
			routeTarget->type = navlibRouteTargetType_t::BOT_TARGET_DYNAMIC;
		}
		else
		{
			routeTarget->type = navlibRouteTargetType_t::BOT_TARGET_STATIC;
		}
	}
	else
	{
		// point target
		VectorSet(maxs, 96, 96, 96);
		VectorSet(mins, -96, -96, -96);
		routeTarget->type = navlibRouteTargetType_t::BOT_TARGET_STATIC;
	}

	for (i = 0; i < 3; i++)
	{
		routeTarget->polyExtents[i] = max(Q_fabs(mins[i]), maxs[i]);
	}

	NavlibGetTargetPos(&target, routeTarget->pos);

	// move center a bit lower so we don't get polys above the object
	// and get polys below the object on a slope
	routeTarget->pos[2] -= routeTarget->polyExtents[2] / 2;

	// account for buildings on walls or cielings
	if (NavlibTargetIsEntity(&target))
	{
		if (target.ent->s.eType == ET_PLAYER || target.ent->s.eType == ET_NPC)
		{
			// building on wall or cieling ( 0.7 == MIN_WALK_NORMAL )
			if (target.ent->r.currentOrigin[2] < 0.7)
			{
				vec3_t targetPos;
				vec3_t end;
				vec3_t invNormal = { 0, 0, -1 };
				trace_t trace;

				routeTarget->polyExtents[0] += 25;
				routeTarget->polyExtents[1] += 25;
				routeTarget->polyExtents[2] += 300;

				// try to find a position closer to the ground
				NavlibGetTargetPos(&target, targetPos);
				VectorMA(targetPos, 600, invNormal, end);
				trap->Trace(&trace, targetPos, mins, maxs, end, target.ent->s.number, CONTENTS_SOLID, 0, 0, 0);
				VectorCopy(trace.endpos, routeTarget->pos);
			}
		}
	}

	// increase extents a little to account for obstacles cutting into the navmesh
	// also accounts for navmesh erosion at mesh boundrys
	routeTarget->polyExtents[0] += self->r.maxs[0] + 10;
	routeTarget->polyExtents[1] += self->r.maxs[1] + 10;
}

bool Navlib::NavlibFindRouteToTarget( gentity_t *self, navlibTargetGoal_t target, bool allowPartial )
{
	navlibRouteTarget_t routeTarget;
	NavlibTargetToRouteTarget( self, target, &routeTarget );
	return Navlib::NavlibFindRoute( self->s.number, &routeTarget, allowPartial );
}
