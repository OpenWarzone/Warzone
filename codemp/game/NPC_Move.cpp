//
// NPC_move.cpp
//
#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

void G_Cylinder( vec3_t start, vec3_t end, float radius, vec3_t color );

qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
int NAV_Steer( gentity_t *self, vec3_t dir, float distance );
extern int GetTime ( int lastTime );

navInfo_t	frameNavInfo;
extern qboolean FlyingCreature( gentity_t *ent );

extern qboolean PM_InKnockDown( playerState_t *ps );

/*
-------------------------
NPC_ClearPathToGoal
-------------------------
*/

qboolean NPC_ClearPathToGoal(gentity_t *aiEnt, vec3_t dir, gentity_t *goal )
{
	trace_t	trace;
	float radius, dist, tFrac;

	//FIXME: What does do about area portals?  THIS IS BROKEN
	//if ( trap->inPVS( NPC->r.currentOrigin, goal->r.currentOrigin ) == qfalse )
	//	return qfalse;

	//Look ahead and see if we're clear to move to our goal position
	if ( NAV_CheckAhead( aiEnt, goal->r.currentOrigin, &trace, ( aiEnt->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
	{
		//VectorSubtract( goal->r.currentOrigin, NPC->r.currentOrigin, dir );
		return qtrue;
	}

	if (!FlyingCreature(aiEnt))
	{
		//See if we're too far above
		if ( fabs( aiEnt->r.currentOrigin[2] - goal->r.currentOrigin[2] ) > 48 )
			return qfalse;
	}

	//This is a work around
	radius = ( aiEnt->r.maxs[0] > aiEnt->r.maxs[1] ) ? aiEnt->r.maxs[0] : aiEnt->r.maxs[1];
	dist = Distance( aiEnt->r.currentOrigin, goal->r.currentOrigin );
	tFrac = 1.0f - ( radius / dist );

	if ( trace.fraction >= tFrac )
		return qtrue;

	//See if we're looking for a navgoal
	if ( goal->flags & FL_NAVGOAL )
	{
		//Okay, didn't get all the way there, let's see if we got close enough:
		if ( NAV_HitNavGoal( trace.endpos, aiEnt->r.mins, aiEnt->r.maxs, goal->r.currentOrigin, aiEnt->NPC->goalRadius, FlyingCreature( aiEnt ) ) )
		{
			//VectorSubtract(goal->r.currentOrigin, NPC->r.currentOrigin, dir);
			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_CheckCombatMove
-------------------------
*/

QINLINE qboolean NPC_CheckCombatMove(gentity_t *aiEnt)
{
	//return NPCInfo->combatMove;
	if ( ( aiEnt->NPC->goalEntity && aiEnt->enemy && aiEnt->NPC->goalEntity == aiEnt->enemy ) || ( aiEnt->NPC->combatMove ) )
	{
		return qtrue;
	}

	if ( aiEnt->NPC->goalEntity && aiEnt->NPC->watchTarget )
	{
		if ( aiEnt->NPC->goalEntity != aiEnt->NPC->watchTarget )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_LadderMove
-------------------------
*/

static void NPC_LadderMove(gentity_t *aiEnt, vec3_t dir )
{
	//FIXME: this doesn't guarantee we're facing ladder
	//ALSO: Need to be able to get off at top
	//ALSO: Need to play an anim
	//ALSO: Need transitionary anims?

	if ( ( dir[2] > 0 ) || ( dir[2] < 0 && aiEnt->client->ps.groundEntityNum == ENTITYNUM_NONE ) )
	{
		//Set our movement direction
		aiEnt->client->pers.cmd.upmove = (dir[2] > 0) ? 127 : -127;

		//Don't move around on XY
		aiEnt->client->pers.cmd.forwardmove = aiEnt->client->pers.cmd.rightmove = 0;
	}
}

/*
-------------------------
NPC_GetMoveInformation
-------------------------
*/

QINLINE qboolean NPC_GetMoveInformation(gentity_t *aiEnt, vec3_t dir, float *distance )
{
	//NOTENOTE: Use path stacks!

	//Make sure we have somewhere to go
	if ( aiEnt->NPC->goalEntity == NULL )
		return qfalse;

	//Get our move info
	VectorSubtract( aiEnt->NPC->goalEntity->r.currentOrigin, aiEnt->r.currentOrigin, dir );
	*distance = VectorNormalize( dir );

	VectorCopy( aiEnt->NPC->goalEntity->r.currentOrigin, aiEnt->NPC->blockedDest );

	return qtrue;
}

/*
-------------------------
NAV_GetLastMove
-------------------------
*/

void NAV_GetLastMove( navInfo_t *info )
{
	*info = frameNavInfo;
}

/*
-------------------------
NPC_GetMoveDirection
-------------------------
*/

qboolean NPC_GetMoveDirection(gentity_t *aiEnt, vec3_t out, float *distance )
{
	vec3_t		angles;

	//Clear the struct
	memset( &frameNavInfo, 0, sizeof( frameNavInfo ) );

	//Get our movement, if any
	if ( NPC_GetMoveInformation(aiEnt, frameNavInfo.direction, &frameNavInfo.distance ) == qfalse )
		return qfalse;

	//Setup the return value
	*distance = frameNavInfo.distance;

	//For starters
	VectorCopy( frameNavInfo.direction, frameNavInfo.pathDirection );

	//If on a ladder, move appropriately
	if ( aiEnt->watertype & CONTENTS_LADDER )
	{
		NPC_LadderMove(aiEnt, frameNavInfo.direction );
		return qtrue;
	}

	//Attempt a straight move to goal
	if ( NPC_ClearPathToGoal(aiEnt, frameNavInfo.direction, aiEnt->NPC->goalEntity ) == qfalse )
	{
		//See if we're just stuck
		if ( NAV_MoveToGoal( aiEnt, &frameNavInfo ) == WAYPOINT_NONE )
		{
			//Can't reach goal, just face
			vectoangles( frameNavInfo.direction, angles );
			aiEnt->NPC->desiredYaw	= AngleNormalize360( angles[YAW] );
			VectorCopy( frameNavInfo.direction, out );
			*distance = frameNavInfo.distance;
			return qfalse;
		}

		frameNavInfo.flags |= NIF_MACRO_NAV;
	}

	//Avoid any collisions on the way
	if ( NAV_AvoidCollision( aiEnt, aiEnt->NPC->goalEntity, &frameNavInfo ) == qfalse )
	{
		//FIXME: Emit a warning, this is a worst case scenario
		//FIXME: if we have a clear path to our goal (exluding bodies), but then this
		//			check (against bodies only) fails, shouldn't we fall back
		//			to macro navigation?  Like so:
		if ( !(frameNavInfo.flags&NIF_MACRO_NAV) )
		{//we had a clear path to goal and didn't try macro nav, but can't avoid collision so try macro nav here
			//See if we're just stuck
			if ( NAV_MoveToGoal( aiEnt, &frameNavInfo ) == WAYPOINT_NONE )
			{
				//Can't reach goal, just face
				vectoangles( frameNavInfo.direction, angles );
				aiEnt->NPC->desiredYaw	= AngleNormalize360( angles[YAW] );
				VectorCopy( frameNavInfo.direction, out );
				*distance = frameNavInfo.distance;
				return qfalse;
			}

			frameNavInfo.flags |= NIF_MACRO_NAV;
		}
	}

	//Setup the return values
	VectorCopy( frameNavInfo.direction, out );
	*distance = frameNavInfo.distance;

	return qtrue;
}

/*
-------------------------
NPC_GetMoveDirectionAltRoute
-------------------------
*/
extern int	NAVNEW_MoveToGoal( gentity_t *self, navInfo_t *info );
extern qboolean NAVNEW_AvoidCollision( gentity_t *self, gentity_t *goal, navInfo_t *info, qboolean setBlockedInfo, int blockedMovesLimit );
qboolean NPC_GetMoveDirectionAltRoute(gentity_t *aiEnt, vec3_t out, float *distance, qboolean tryStraight )
{
	vec3_t		angles;

	aiEnt->NPC->aiFlags &= ~NPCAI_BLOCKED;

	//Clear the struct
	memset( &frameNavInfo, 0, sizeof( frameNavInfo ) );

	//Get our movement, if any
	if ( NPC_GetMoveInformation(aiEnt, frameNavInfo.direction, &frameNavInfo.distance ) == qfalse )
		return qfalse;

	//Setup the return value
	*distance = frameNavInfo.distance;

	//For starters
	VectorCopy( frameNavInfo.direction, frameNavInfo.pathDirection );

	//If on a ladder, move appropriately
	if ( aiEnt->watertype & CONTENTS_LADDER )
	{
		NPC_LadderMove(aiEnt, frameNavInfo.direction );
		return qtrue;
	}

	//Attempt a straight move to goal
	if ( !tryStraight || NPC_ClearPathToGoal(aiEnt, frameNavInfo.direction, aiEnt->NPC->goalEntity ) == qfalse )
	{//blocked
		//Can't get straight to goal, use macro nav
		if ( NAVNEW_MoveToGoal( aiEnt, &frameNavInfo ) == WAYPOINT_NONE )
		{
			//Can't reach goal, just face
			vectoangles( frameNavInfo.direction, angles );
			aiEnt->NPC->desiredYaw	= AngleNormalize360( angles[YAW] );
			VectorCopy( frameNavInfo.direction, out );
			*distance = frameNavInfo.distance;
			return qfalse;
		}
		//else we are on our way
		frameNavInfo.flags |= NIF_MACRO_NAV;
	}
	else
	{//we have no architectural problems, see if there are ents inthe way and try to go around them
		//not blocked
		if ( d_altRoutes.integer )
		{//try macro nav
			navInfo_t	tempInfo;
			memcpy( &tempInfo, &frameNavInfo, sizeof( tempInfo ) );
			if ( NAVNEW_AvoidCollision( aiEnt, aiEnt->NPC->goalEntity, &tempInfo, qtrue, 5 ) == qfalse )
			{//revert to macro nav
				//Can't get straight to goal, dump tempInfo and use macro nav
				if ( NAVNEW_MoveToGoal( aiEnt, &frameNavInfo ) == WAYPOINT_NONE )
				{
					//Can't reach goal, just face
					vectoangles( frameNavInfo.direction, angles );
					aiEnt->NPC->desiredYaw	= AngleNormalize360( angles[YAW] );
					VectorCopy( frameNavInfo.direction, out );
					*distance = frameNavInfo.distance;
					return qfalse;
				}
				//else we are on our way
				frameNavInfo.flags |= NIF_MACRO_NAV;
			}
			else
			{//otherwise, either clear or can avoid
				memcpy( &frameNavInfo, &tempInfo, sizeof( frameNavInfo ) );
			}
		}
		else
		{//OR: just give up
			if ( NAVNEW_AvoidCollision( aiEnt, aiEnt->NPC->goalEntity, &frameNavInfo, qtrue, 30 ) == qfalse )
			{//give up
				return qfalse;
			}
		}
	}

	//Setup the return values
	VectorCopy( frameNavInfo.direction, out );
	*distance = frameNavInfo.distance;

	return qtrue;
}

extern qboolean UQ1_UcmdMoveForDir ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest );

void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir, vec3_t dest )
{
#if 0
	vec3_t	forward, right;
	float	fDot, rDot;

	AngleVectors( self->r.currentAngles, forward, right, NULL );

	dir[2] = 0;
	VectorNormalize( dir );
	//NPCs cheat and store this directly because converting movement into a ucmd loses precision
	VectorCopy( dir, self->client->ps.moveDir );

	fDot = DotProduct( forward, dir ) * 127.0f;
	rDot = DotProduct( right, dir ) * 127.0f;
	//Must clamp this because DotProduct is not guaranteed to return a number within -1 to 1, and that would be bad when we're shoving this into a signed byte
	if ( fDot > 127.0f )
	{
		fDot = 127.0f;
	}
	if ( fDot < -127.0f )
	{
		fDot = -127.0f;
	}
	if ( rDot > 127.0f )
	{
		rDot = 127.0f;
	}
	if ( rDot < -127.0f )
	{
		rDot = -127.0f;
	}
	cmd->forwardmove = floor(fDot);
	cmd->rightmove = floor(rDot);

	/*
	vec3_t	wishvel;
	for ( int i = 0 ; i < 3 ; i++ )
	{
		wishvel[i] = forward[i]*cmd->forwardmove + right[i]*cmd->rightmove;
	}
	VectorNormalize( wishvel );
	if ( !VectorCompare( wishvel, dir ) )
	{
		Com_Printf( "PRECISION LOSS: %s != %s\n", vtos(wishvel), vtos(dir) );
	}
	*/
#else
	// UQ1: Use my method instead to check for falling, etc...
	qboolean walk = qfalse;

	if (self->client->pers.cmd.buttons&BUTTON_WALKING) walk = qtrue;

	UQ1_UcmdMoveForDir( self, &self->client->pers.cmd, dir, walk, dest );
#endif
}

/*
-------------------------
NPC_CombatMoveToGoal

  Now assumes goal is goalEntity, was no reason for it to be otherwise - UQ1: This version checks for falling and reachability...
-------------------------
*/
#if	AI_TIMERS
extern int navTime;
#endif//	AI_TIMERS

extern qboolean NPC_CheckFall(gentity_t *NPC, vec3_t dir);
extern int NPC_CheckFallJump(gentity_t *NPC, vec3_t dest, usercmd_t *cmd);
extern qboolean NPC_MoveDirClear(gentity_t *aiEnt, int forwardmove, int rightmove, qboolean reset );

qboolean NPC_CombatMoveToGoal(gentity_t *aiEnt, qboolean tryStraight, qboolean retreat )
{
	float	distance;
	vec3_t	dir;

#if	AI_TIMERS
	int	startTime = GetTime(0);
#endif//	AI_TIMERS
	//If taking full body pain, don't move
	if ( PM_InKnockDown( &aiEnt->client->ps ) || ( ( aiEnt->s.legsAnim >= BOTH_PAIN1 ) && ( aiEnt->s.legsAnim <= BOTH_PAIN18 ) ) )
	{
		return qtrue;
	}

	/*
	if( NPC->s.eFlags & EF_LOCKED_TO_WEAPON )
	{//If in an emplaced gun, never try to navigate!
		return qtrue;
	}
	*/
	//rwwFIXMEFIXME: emplaced support

	//FIXME: if can't get to goal & goal is a target (enemy), try to find a waypoint that has line of sight to target, at least?
	//Get our movement direction
#if 1
	if ( NPC_GetMoveDirectionAltRoute(aiEnt, dir, &distance, tryStraight ) == qfalse )
#else
	if ( NPC_GetMoveDirection(aiEnt, dir, &distance ) == qfalse )
#endif
		return qfalse;

	aiEnt->NPC->distToGoal		= distance;

	//Convert the move to angles
	vectoangles( dir, aiEnt->NPC->lastPathAngles );

	if (retreat)
	{
		dir[0] *= -1.0;
		dir[1] *= -1.0;
		dir[2] *= -1.0;
	}

	if ( (aiEnt->client->pers.cmd.buttons&BUTTON_WALKING) )
	{
		aiEnt->client->ps.speed = aiEnt->NPC->stats.walkSpeed;
	}
	else
	{
		aiEnt->client->ps.speed = aiEnt->NPC->stats.runSpeed;
	}

	// UQ1: Actually check if this would make us fall!!!
	if (NPC_CheckFall(aiEnt, dir))
	{
		int JUMP_RESULT = 0;
		
		if (!retreat && aiEnt->NPC->goalEntity)
		{// Can only ever jump if we have a goal and are not retreating...
			JUMP_RESULT = NPC_CheckFallJump(aiEnt, aiEnt->NPC->goalEntity->r.currentOrigin, &aiEnt->client->pers.cmd);
		}

		if (JUMP_RESULT == 2)
		{// We can jump there... JKA method...
			return qtrue;
		}
		else if (JUMP_RESULT == 1)
		{// We can jump there... My method...
			aiEnt->client->pers.cmd.upmove = 127.0;
			if (aiEnt->s.eType == ET_PLAYER) trap->EA_Jump(aiEnt->s.number);
		}
		else
		{// Moving here would cause us to fall (or 18 forward is blocked)... Wait!
			aiEnt->client->pers.cmd.forwardmove = 0;
			aiEnt->client->pers.cmd.rightmove = 0;
			aiEnt->client->pers.cmd.upmove = 0;
			return qfalse;
		}
	}

	//FIXME: still getting ping-ponging in certain cases... !!!  Nav/avoidance error?  WTF???!!!
	//If in combat move, then move directly towards our goal
	if ( NPC_CheckCombatMove(aiEnt) || aiEnt->s.eType == ET_PLAYER )
	{//keep current facing
		G_UcmdMoveForDir( aiEnt, &aiEnt->client->pers.cmd, dir, aiEnt->NPC->goalEntity->r.currentOrigin );

		if (aiEnt->s.eType == ET_PLAYER)
		{
			if (aiEnt->client->pers.cmd.buttons & BUTTON_WALKING)
			{
				//trap->EA_Action(aiEnt->s.number, 0x0080000);
				trap->EA_Move(aiEnt->s.number, dir, 5000.0);
			}
			else
			{
				trap->EA_Move(aiEnt->s.number, dir, 5000.0);
			}
		}
	}
	else
	{//face our goal
		//FIXME: strafe instead of turn if change in dir is small and temporary
		aiEnt->NPC->desiredPitch	= 0.0f;
		aiEnt->NPC->desiredYaw		= AngleNormalize360( aiEnt->NPC->lastPathAngles[YAW] );

		//Pitch towards the goal and also update if flying or swimming
		if ( (aiEnt->client->ps.eFlags2&EF2_FLYING) )//moveType == MT_FLYSWIM )
		{
			aiEnt->NPC->desiredPitch = AngleNormalize360( aiEnt->NPC->lastPathAngles[PITCH] );

			if ( dir[2] )
			{
				float scale = (dir[2] * distance);
				if ( scale > 64 )
				{
					scale = 64;
				}
				else if ( scale < -64 )
				{
					scale = -64;
				}
				aiEnt->client->ps.velocity[2] = scale;
				//NPC->client->ps.velocity[2] = (dir[2] > 0) ? 64 : -64;
			}
		}

		//Set any final info
		if (retreat)
		{
			if (NPC_MoveDirClear(aiEnt , -127, aiEnt->client->pers.cmd.rightmove, qfalse )) // UQ1: Only if safe!
			{
				aiEnt->client->pers.cmd.forwardmove = -127;

				if (aiEnt->s.eType == ET_PLAYER)
				{
					if (aiEnt->client->pers.cmd.buttons & BUTTON_WALKING)
					{
						trap->EA_MoveBack(aiEnt->s.number);
					}
					else
					{
						trap->EA_MoveBack(aiEnt->s.number);
					}
				}
				return qtrue;
			}
		}
		else
		{
			if (NPC_MoveDirClear(aiEnt, 127, aiEnt->client->pers.cmd.rightmove, qfalse )) // UQ1: Only if safe!
			{
				aiEnt->client->pers.cmd.forwardmove = 127;

				if (aiEnt->s.eType == ET_PLAYER)
				{
					if (aiEnt->client->pers.cmd.buttons & BUTTON_WALKING)
					{
						trap->EA_MoveForward(aiEnt->s.number);
					}
					else
					{
						trap->EA_MoveForward(aiEnt->s.number);
					}
				}
				return qtrue;
			}
		}

		//
		// Non-Combat move has failed! Force combat move...
		//

		G_UcmdMoveForDir( aiEnt, &aiEnt->client->pers.cmd, dir, aiEnt->NPC->goalEntity->r.currentOrigin );

		if (aiEnt->s.eType == ET_PLAYER)
		{
			if (aiEnt->client->pers.cmd.buttons & BUTTON_WALKING)
			{
				//trap->EA_Action(aiEnt->s.number, 0x0080000);
				trap->EA_Move(aiEnt->s.number, dir, 5000.0);
			}
			else
			{
				trap->EA_Move(aiEnt->s.number, dir, 5000.0);
			}
		}
	}

#if	AI_TIMERS
	navTime += GetTime( startTime );
#endif//	AI_TIMERS

	/*
	if (retreat)
	{// UQ1: Isn't this done above???
		VectorScale( aiEnt->client->ps.moveDir, -1, aiEnt->client->ps.moveDir );
	}
	*/

	return qtrue;
}

/*
-------------------------
NPC_MoveToGoal

  Now assumes goal is goalEntity, was no reason for it to be otherwise
-------------------------
*/

qboolean NPC_MoveToGoal(gentity_t *aiEnt, qboolean tryStraight )
{
#if 0
	float	distance;
	vec3_t	dir;

#if	AI_TIMERS
	int	startTime = GetTime(0);
#endif//	AI_TIMERS
	//If taking full body pain, don't move
	if ( PM_InKnockDown( &aiEnt->client->ps ) || ( ( aiEnt->s.legsAnim >= BOTH_PAIN1 ) && ( aiEnt->s.legsAnim <= BOTH_PAIN18 ) ) )
	{
		return qtrue;
	}

	/*
	if( NPC->s.eFlags & EF_LOCKED_TO_WEAPON )
	{//If in an emplaced gun, never try to navigate!
		return qtrue;
	}
	*/
	//rwwFIXMEFIXME: emplaced support

	//FIXME: if can't get to goal & goal is a target (enemy), try to find a waypoint that has line of sight to target, at least?
	//Get our movement direction
#if 1
	if ( NPC_GetMoveDirectionAltRoute(aiEnt, dir, &distance, tryStraight ) == qfalse )
#else
	if ( NPC_GetMoveDirection(aiEnt, dir, &distance ) == qfalse )
#endif
		return qfalse;

	aiEnt->NPC->distToGoal		= distance;

	//Convert the move to angles
	vectoangles( dir, aiEnt->NPC->lastPathAngles );
	if ( (aiEnt->client->pers.cmd.buttons&BUTTON_WALKING) )
	{
		aiEnt->client->ps.speed = aiEnt->NPC->stats.walkSpeed;
	}
	else
	{
		aiEnt->client->ps.speed = aiEnt->NPC->stats.runSpeed;
	}

	//FIXME: still getting ping-ponging in certain cases... !!!  Nav/avoidance error?  WTF???!!!
	//If in combat move, then move directly towards our goal
	if ( NPC_CheckCombatMove() || aiEnt->s.eType == ET_PLAYER )
	{//keep current facing
		G_UcmdMoveForDir( aiEnt, &aiEnt->client->pers.cmd, dir );

		if (aiEnt->s.eType == ET_PLAYER)
		{
			if (aiEnt->client->pers.cmd.buttons & BUTTON_WALKING)
			{
				//trap->EA_Action(aiEnt->s.number, 0x0080000);
				trap->EA_Move(aiEnt->s.number, dir, 5000.0);
			}
			else
			{
				trap->EA_Move(aiEnt->s.number, dir, 5000.0);
			}
		}
	}
	else
	{//face our goal
		//FIXME: strafe instead of turn if change in dir is small and temporary
		aiEnt->NPC->desiredPitch	= 0.0f;
		aiEnt->NPC->desiredYaw		= AngleNormalize360( aiEnt->NPC->lastPathAngles[YAW] );

		//Pitch towards the goal and also update if flying or swimming
		if ( (aiEnt->client->ps.eFlags2&EF2_FLYING) )//moveType == MT_FLYSWIM )
		{
			aiEnt->NPC->desiredPitch = AngleNormalize360( aiEnt->NPC->lastPathAngles[PITCH] );

			if ( dir[2] )
			{
				float scale = (dir[2] * distance);
				if ( scale > 64 )
				{
					scale = 64;
				}
				else if ( scale < -64 )
				{
					scale = -64;
				}
				aiEnt->client->ps.velocity[2] = scale;
				//NPC->client->ps.velocity[2] = (dir[2] > 0) ? 64 : -64;
			}
		}

		//Set any final info
		aiEnt->client->pers.cmd.forwardmove = 127;
	}

#if	AI_TIMERS
	navTime += GetTime( startTime );
#endif//	AI_TIMERS
	return qtrue;
#endif //0

	return NPC_CombatMoveToGoal(aiEnt, tryStraight, qfalse ); // UQ1: Check for falling...
}

/*
-------------------------
void NPC_SlideMoveToGoal( void )

  Now assumes goal is goalEntity, if want to use tempGoal, you set that before calling the func
-------------------------
*/
qboolean NPC_SlideMoveToGoal(gentity_t *aiEnt)
{
	float	saveYaw = aiEnt->client->ps.viewangles[YAW];
	qboolean ret;

	aiEnt->NPC->combatMove = qtrue;

	ret = NPC_MoveToGoal( aiEnt, qtrue );

	aiEnt->NPC->desiredYaw	= saveYaw;

	return ret;
}


/*
-------------------------
NPC_ApplyRoff
-------------------------
*/

void NPC_ApplyRoff(gentity_t *aiEnt)
{
	BG_PlayerStateToEntityState( &aiEnt->client->ps, &aiEnt->s, qfalse );
	//VectorCopy ( NPC->r.currentOrigin, NPC->lastOrigin );
	//rwwFIXMEFIXME: Any significance to this?

	// use the precise origin for linking
	trap->LinkEntity((sharedEntity_t *)aiEnt);
}
