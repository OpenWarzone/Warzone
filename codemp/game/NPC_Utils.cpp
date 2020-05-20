//NPC_utils.cpp

#include "b_local.h"
#include "icarus/Q3_Interface.h"
#include "ghoul2/G2.h"
#include "ai_dominance_main.h"

int	teamNumbers[FACTION_NUM_FACTIONS];
int	teamStrength[FACTION_NUM_FACTIONS];
int	teamCounter[FACTION_NUM_FACTIONS];

#define	VALID_ATTACK_CONE	2.0f	//Degrees
extern void G_DebugPrint( int level, const char *format, ... );
extern qboolean G_EntIsBreakable( int entityNum );
extern vmCvar_t npc_pathing;

/*
void CalcEntitySpot ( gentity_t *ent, spot_t spot, vec3_t point )

Added: Uses shootAngles if a NPC has them

*/
void CalcEntitySpot ( const gentity_t *ent, const spot_t spot, vec3_t point )
{
	vec3_t	forward, up, right;
	vec3_t	start, end, org;
	trace_t	tr;

	if ( !ent )
	{
		return;
	}

	if (ent && G_EntIsBreakable(ent->s.number))
	{
		VectorCopy ( ent->breakableOrigin, point );
		return;
	}

	VectorCopy(ent->r.currentOrigin, org);

	//if (G_EntIsBreakable(ent->s.number)) VectorCopy(ent->breakableOrigin, org);

	switch ( spot )
	{
	case SPOT_ORIGIN:
		if(VectorCompare(org, vec3_origin))
		{//brush
			VectorSubtract(ent->r.absmax, ent->r.absmin, point);//size
			VectorMA(ent->r.absmin, 0.5, point, point);
		}
		else
		{
			VectorCopy ( org, point );
		}
		break;

	case SPOT_CHEST:
	case SPOT_HEAD:
		if (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_ATAT)
		{
			VectorCopy(org, point);
			point[2] += 448.0;// 384.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_ATST)
		{
			VectorCopy(org, point);
			point[2] += 256.0;// 220.0;// 172.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_ATPT)
		{
			VectorCopy(org, point);
			point[2] += 176.0;// 128.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class != CLASS_VEHICLE && ent->s.m_iVehicleNum && ent->s.NPC_class == CLASS_STORMTROOPER_ATST_PILOT)
		{// NPC driving an ATST...
			VectorCopy(org, point);
			point[2] += 172.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class != CLASS_VEHICLE && ent->s.m_iVehicleNum && ent->s.NPC_class == CLASS_STORMTROOPER_ATAT_PILOT)
		{// NPC driving an ATAT...
			VectorCopy(org, point);
			point[2] += 384.0;
		}
		else if ( ent->client && VectorLengthSquared( ent->client->renderInfo.eyePoint ) /*&& (ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD)*/ )
		{//Actual tag_head eyespot!
			//FIXME: Stasis aliens may have a problem here...
			VectorCopy( ent->client->renderInfo.eyePoint, point );
			if ( ent->client->NPC_class == CLASS_ATST_OLD || ent->client->NPC_class == CLASS_ATST || ent->client->NPC_class == CLASS_ATPT)
			{//adjust up some
				point[2] += 28;//magic number :)
			}
			if (ent->client->NPC_class == CLASS_ATAT)
			{//adjust up some
				point[2] += 96;// 28;//magic number :)
			}
			if ( ent->NPC )
			{//always aim from the center of my bbox, so we don't wiggle when we lean forward or backwards
				point[0] = org[0];
				point[1] = org[1];
			}
			/*
			else if (ent->s.eType == ET_PLAYER )
			{
				SubtractLeanOfs( ent, point );
			}
			*/
		}
		else
		{
			VectorCopy ( org, point );
			if ( ent->client )
			{
				point[2] += ent->client->ps.viewheight;
			}
		}
		if ( spot == SPOT_CHEST && ent->client )
		{
			if ( ent->client->NPC_class != CLASS_ATST_OLD )
			{//adjust up some
				point[2] -= ent->r.maxs[2]*0.2f;
			}
			if (ent->client->NPC_class != CLASS_ATST)
			{//adjust up some
				point[2] -= ent->r.maxs[2] * 0.2f;
			}
		}
		break;

	case SPOT_HEAD_LEAN:
		if (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_ATAT)
		{
			VectorCopy(org, point);
			point[2] += 448.0;// 384.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_ATST)
		{
			VectorCopy(org, point);
			point[2] += 256.0;// 220.0;// 172.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_ATPT)
		{
			VectorCopy(org, point);
			point[2] += 176.0;// 128.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class != CLASS_VEHICLE && ent->s.m_iVehicleNum && ent->s.NPC_class == CLASS_STORMTROOPER_ATST_PILOT)
		{// NPC driving an ATST...
			VectorCopy(org, point);
			point[2] += 172.0; //192.0
		}
		else if (ent->s.eType == ET_NPC && ent->s.NPC_class != CLASS_VEHICLE && ent->s.m_iVehicleNum && ent->s.NPC_class == CLASS_STORMTROOPER_ATAT_PILOT)
		{// NPC driving an ATAT...
			VectorCopy(org, point);
			point[2] += 384.0;
		}
		else if ( ent->client && VectorLengthSquared( ent->client->renderInfo.eyePoint ) /*&& (ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD*/ )
		{//Actual tag_head eyespot!
			//FIXME: Stasis aliens may have a problem here...
			VectorCopy( ent->client->renderInfo.eyePoint, point );
			if ( ent->client->NPC_class == CLASS_ATST_OLD || ent->client->NPC_class == CLASS_ATST )
			{//adjust up some
				point[2] += 28;//magic number :)
			}
			if (ent->client->NPC_class == CLASS_ATPT)
			{//adjust up some
				point[2] += 28;//magic number :)
			}
			if ( ent->client->NPC_class == CLASS_ATAT )
			{//adjust up some
				point[2] += 96;//magic number :)
			}
			if ( ent->NPC )
			{//always aim from the center of my bbox, so we don't wiggle when we lean forward or backwards
				point[0] = org[0];
				point[1] = org[1];
			}
			/*
			else if ( ent->s.eType == ET_PLAYER )
			{
				SubtractLeanOfs( ent, point );
			}
			*/
			//NOTE: automatically takes leaning into account!
		}
		else
		{
			VectorCopy ( org, point );
			if ( ent->client )
			{
				point[2] += ent->client->ps.viewheight;
			}
			//AddLeanOfs ( ent, point );
		}
		break;

	//FIXME: implement...
	//case SPOT_CHEST:
		//Returns point 3/4 from tag_torso to tag_head?
		//break;

	case SPOT_LEGS:
		VectorCopy ( org, point );
		point[2] += (ent->r.mins[2] * 0.5);
		break;

	case SPOT_WEAPON:
		if( ent->NPC && !VectorCompare( ent->NPC->shootAngles, vec3_origin ) && !VectorCompare( ent->NPC->shootAngles, ent->client->ps.viewangles ))
		{
			AngleVectors( ent->NPC->shootAngles, forward, right, up );
		}
		else
		{
			AngleVectors( ent->client->ps.viewangles, forward, right, up );
		}
		CalcMuzzlePoint( (gentity_t*)ent, forward, right, up, point );
		//NOTE: automatically takes leaning into account!
		break;

	case SPOT_GROUND:
		// if entity is on the ground, just use it's absmin
		if ( ent->s.groundEntityNum != ENTITYNUM_NONE )
		{
			VectorCopy( org, point );
			point[2] = ent->r.absmin[2];
			break;
		}

		// if it is reasonably close to the ground, give the point underneath of it
		VectorCopy( org, start );
		start[2] = ent->r.absmin[2];
		VectorCopy( start, end );
		end[2] -= 64;
		trap->Trace( &tr, start, ent->r.mins, ent->r.maxs, end, ent->s.number, MASK_PLAYERSOLID, qfalse, 0, 0 );
		if ( tr.fraction < 1.0 )
		{
			VectorCopy( tr.endpos, point);
			break;
		}

		// otherwise just use the origin
		VectorCopy( org, point );
		break;

	default:
		VectorCopy ( org, point );
		break;
	}
}


//===================================================================================

/*
qboolean NPC_UpdateAngles ( qboolean doPitch, qboolean doYaw )

Added: option to do just pitch or just yaw

Does not include "aim" in it's calculations

FIXME: stop compressing angles into shorts!!!!
*/
qboolean NPC_UpdateAngles ( gentity_t *aiEnt, qboolean doPitch, qboolean doYaw )
{
	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	float		yawSpeed;
	qboolean	exact = qtrue;

	// if angle changes are locked; just keep the current angles
	// aimTime isn't even set anymore... so this code was never reached, but I need a way to lock NPC's yaw, so instead of making a new SCF_ flag, just use the existing render flag... - dmv
	if ( !aiEnt->enemy && ( (level.time < aiEnt->NPC->aimTime) /*|| NPC->client->renderInfo.renderFlags & RF_LOCKEDANGLE*/) )
	{
		if(doPitch)
			targetPitch = aiEnt->NPC->lockedDesiredPitch;

		if(doYaw)
			targetYaw = aiEnt->NPC->lockedDesiredYaw;
	}
	else
	{
		// we're changing the lockedDesired Pitch/Yaw below so it's lost it's original meaning, get rid of the lock flag
	//	NPC->client->renderInfo.renderFlags &= ~RF_LOCKEDANGLE;

		if(doPitch)
		{
			targetPitch = aiEnt->NPC->desiredPitch;
			aiEnt->NPC->lockedDesiredPitch = aiEnt->NPC->desiredPitch;
		}

		if(doYaw)
		{
			targetYaw = aiEnt->NPC->desiredYaw;
			aiEnt->NPC->lockedDesiredYaw = aiEnt->NPC->desiredYaw;
		}
	}

	if ( aiEnt->s.weapon == WP_EMPLACED_GUN )
	{
		// FIXME: this seems to do nothing, actually...
		yawSpeed = 20;
	}
	else
	{
		yawSpeed = aiEnt->NPC->stats.yawSpeed;
	}

	if ( aiEnt->s.weapon == WP_SABER && aiEnt->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
	{
		char buf[128];
		float tFVal = 0;

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);

		yawSpeed *= 1.0f/tFVal;
	}

	if( doYaw )
	{
		// decay yaw error
		error = AngleDelta ( aiEnt->client->ps.viewangles[YAW], targetYaw );
		if( fabs(error) > MIN_ANGLE_ERROR )
		{
			if ( error )
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0f / 1000.0f;//msec

				if ( error < 0.0 )
				{
					error += decay;
					if ( error > 0.0 )
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if ( error < 0.0 )
					{
						error = 0.0;
					}
				}
			}
		}

		aiEnt->client->pers.cmd.angles[YAW] = ANGLE2SHORT( targetYaw + error ) - aiEnt->client->ps.delta_angles[YAW];
	}

	//FIXME: have a pitchSpeed?
	if( doPitch )
	{
		// decay pitch error
		error = AngleDelta ( aiEnt->client->ps.viewangles[PITCH], targetPitch );
		if ( fabs(error) > MIN_ANGLE_ERROR )
		{
			if ( error )
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0f / 1000.0f;//msec

				if ( error < 0.0 )
				{
					error += decay;
					if ( error > 0.0 )
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if ( error < 0.0 )
					{
						error = 0.0;
					}
				}
			}
		}

		aiEnt->client->pers.cmd.angles[PITCH] = ANGLE2SHORT( targetPitch + error ) - aiEnt->client->ps.delta_angles[PITCH];
	}

	aiEnt->client->pers.cmd.angles[ROLL] = ANGLE2SHORT ( aiEnt->client->ps.viewangles[ROLL] ) - aiEnt->client->ps.delta_angles[ROLL];

#ifndef __NO_ICARUS__
	if ( exact && trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_ANGLE_FACE ) )
	{
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)aiEnt, TID_ANGLE_FACE );
	}
#endif //__NO_ICARUS__
	return exact;
}

void NPC_AimWiggle( gentity_t *aiEnt, vec3_t enemy_org )
{
	//shoot for somewhere between the head and torso
	//NOTE: yes, I know this looks weird, but it works
	if ( aiEnt->NPC->aimErrorDebounceTime < level.time )
	{
		aiEnt->NPC->aimOfs[0] = 0.3*flrand(aiEnt->enemy->r.mins[0], aiEnt->enemy->r.maxs[0]);
		aiEnt->NPC->aimOfs[1] = 0.3*flrand(aiEnt->enemy->r.mins[1], aiEnt->enemy->r.maxs[1]);
		if ( aiEnt->enemy->r.maxs[2] > 0 )
		{
			aiEnt->NPC->aimOfs[2] = aiEnt->enemy->r.maxs[2]*flrand(0.0f, -1.0f);
		}
	}
	VectorAdd( enemy_org, aiEnt->NPC->aimOfs, enemy_org );
}

/*
qboolean NPC_UpdateFiringAngles ( qboolean doPitch, qboolean doYaw )

  Includes aim when determining angles - so they don't always hit...
  */
qboolean NPC_UpdateFiringAngles ( gentity_t *aiEnt, qboolean doPitch, qboolean doYaw )
{
	float		error, diff;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	qboolean	exact = qtrue;

	// if angle changes are locked; just keep the current angles
	if ( level.time < aiEnt->NPC->aimTime )
	{
		if(doPitch)
			targetPitch = aiEnt->NPC->lockedDesiredPitch;
		if(doYaw)
			targetYaw = aiEnt->NPC->lockedDesiredYaw;
	}
	else
	{
		if(doPitch)
			targetPitch = aiEnt->NPC->desiredPitch;
		if(doYaw)
			targetYaw = aiEnt->NPC->desiredYaw;

//		NPCInfo->aimTime = level.time + 250;
		if(doPitch)
			aiEnt->NPC->lockedDesiredPitch = aiEnt->NPC->desiredPitch;
		if(doYaw)
			aiEnt->NPC->lockedDesiredYaw = aiEnt->NPC->desiredYaw;
	}

	if ( aiEnt->NPC->aimErrorDebounceTime < level.time )
	{
		if ( Q_irand(0, 1 ) )
		{
			aiEnt->NPC->lastAimErrorYaw = ((float)(6 - aiEnt->NPC->stats.aim)) * flrand(-1, 1);
		}
		if ( Q_irand(0, 1 ) )
		{
			aiEnt->NPC->lastAimErrorPitch = ((float)(6 - aiEnt->NPC->stats.aim)) * flrand(-1, 1);
		}
		aiEnt->NPC->aimErrorDebounceTime = level.time + Q_irand(250, 2000);
	}

	if(doYaw)
	{
		// decay yaw diff
		diff = AngleDelta ( aiEnt->client->ps.viewangles[YAW], targetYaw );

		if ( diff)
		{
			exact = qfalse;

			decay = 60.0 + 80.0;
			decay *= 50.0f / 1000.0f;//msec
			if ( diff < 0.0 )
			{
				diff += decay;
				if ( diff > 0.0 )
				{
					diff = 0.0;
				}
			}
			else
			{
				diff -= decay;
				if ( diff < 0.0 )
				{
					diff = 0.0;
				}
			}
		}

		// add yaw error based on NPCInfo->aim value
		error = aiEnt->NPC->lastAimErrorYaw;

		/*
		if(Q_irand(0, 1))
		{
			error *= -1;
		}
		*/

		aiEnt->client->pers.cmd.angles[YAW] = ANGLE2SHORT( targetYaw + diff + error ) - aiEnt->client->ps.delta_angles[YAW];
	}

	if(doPitch)
	{
		// decay pitch diff
		diff = AngleDelta ( aiEnt->client->ps.viewangles[PITCH], targetPitch );
		if ( diff)
		{
			exact = qfalse;

			decay = 60.0 + 80.0;
			decay *= 50.0f / 1000.0f;//msec
			if ( diff < 0.0 )
			{
				diff += decay;
				if ( diff > 0.0 )
				{
					diff = 0.0;
				}
			}
			else
			{
				diff -= decay;
				if ( diff < 0.0 )
				{
					diff = 0.0;
				}
			}
		}

		error = aiEnt->NPC->lastAimErrorPitch;

		aiEnt->client->pers.cmd.angles[PITCH] = ANGLE2SHORT( targetPitch + diff + error ) - aiEnt->client->ps.delta_angles[PITCH];
	}

	aiEnt->client->pers.cmd.angles[ROLL] = ANGLE2SHORT ( aiEnt->client->ps.viewangles[ROLL] ) - aiEnt->client->ps.delta_angles[ROLL];

	return exact;
}
//===================================================================================

/*
static void NPC_UpdateShootAngles (vec3_t angles, qboolean doPitch, qboolean doYaw )

Does update angles on shootAngles
*/

void NPC_UpdateShootAngles (gentity_t *aiEnt, vec3_t angles, qboolean doPitch, qboolean doYaw )
{//FIXME: shoot angles either not set right or not used!
	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;

	if(doPitch)
		targetPitch = angles[PITCH];
	if(doYaw)
		targetYaw = angles[YAW];


	if(doYaw)
	{
		// decay yaw error
		error = AngleDelta ( aiEnt->NPC->shootAngles[YAW], targetYaw );
		if ( error )
		{
			decay = 60.0 + 80.0 * aiEnt->NPC->stats.aim;
			decay *= 100.0f / 1000.0f;//msec
			if ( error < 0.0 )
			{
				error += decay;
				if ( error > 0.0 )
				{
					error = 0.0;
				}
			}
			else
			{
				error -= decay;
				if ( error < 0.0 )
				{
					error = 0.0;
				}
			}
		}
		aiEnt->NPC->shootAngles[YAW] = targetYaw + error;
	}

	if(doPitch)
	{
		// decay pitch error
		error = AngleDelta ( aiEnt->NPC->shootAngles[PITCH], targetPitch );
		if ( error )
		{
			decay = 60.0 + 80.0 * aiEnt->NPC->stats.aim;
			decay *= 100.0f / 1000.0f;//msec
			if ( error < 0.0 )
			{
				error += decay;
				if ( error > 0.0 )
				{
					error = 0.0;
				}
			}
			else
			{
				error -= decay;
				if ( error < 0.0 )
				{
					error = 0.0;
				}
			}
		}
		aiEnt->NPC->shootAngles[PITCH] = targetPitch + error;
	}
}

/*
void SetTeamNumbers (void)

Sets the number of living clients on each team

FIXME: Does not account for non-respawned players!
FIXME: Don't include medics?
*/
void SetTeamNumbers (void)
{
	gentity_t	*found;
	int			i;

	for( i = 0; i < FACTION_NUM_FACTIONS; i++ )
	{
		teamNumbers[i] = 0;
		teamStrength[i] = 0;
	}

	//OJKFIXME: clientnum 0
	for( i = 0; i < 1 ; i++ )
	{
		found = &g_entities[i];

		if( found->client )
		{
			if( found->health > 0 )//FIXME: or if a player!
			{
				teamNumbers[found->client->playerTeam]++;
				teamStrength[found->client->playerTeam] += found->health;
			}
		}
	}

	for( i = 0; i < FACTION_NUM_FACTIONS; i++ )
	{//Get the average health
		teamStrength[i] = floor( ((float)(teamStrength[i])) / ((float)(teamNumbers[i])) );
	}
}

extern stringID_table_t BSTable[];
extern stringID_table_t BSETTable[];
qboolean G_ActivateBehavior (gentity_t *self, int bset )
{
	bState_t	bSID = (bState_t)-1;
	char *bs_name = NULL;

	if ( !self )
	{
		return qfalse;
	}

	bs_name = self->behaviorSet[bset];

	if( !(VALIDSTRING( bs_name )) )
	{
		return qfalse;
	}

	if ( self->NPC )
	{
		bSID = (bState_t)(GetIDForString( BSTable, bs_name ));
	}

	if(bSID != (bState_t)-1)
	{
		self->NPC->tempBehavior = BS_DEFAULT;
		self->NPC->behaviorState = bSID;
	}
	else
	{
		/*
		char			newname[MAX_FILENAME_LENGTH];
		sprintf((char *) &newname, "%s/%s", Q3_SCRIPT_DIR, bs_name );
		*/

		//FIXME: between here and actually getting into the ICARUS_RunScript function, the stack gets blown!
		//if ( ( ICARUS_entFilter == -1 ) || ( ICARUS_entFilter == self->s.number ) )
		if (0)
		{
			G_DebugPrint( WL_VERBOSE, "%s attempting to run bSet %s (%s)\n", self->targetname, GetStringForID( BSETTable, bset ), bs_name );
		}
#ifndef __NO_ICARUS__
		trap->ICARUS_RunScript( (sharedEntity_t *)self, va( "%s/%s", Q3_SCRIPT_DIR, bs_name ) );
#endif //__NO_ICARUS__
	}
	return qtrue;
}


/*
=============================================================================

	Extended Functions

=============================================================================
*/

//rww - special system for sync'ing bone angles between client and server.
void NPC_SetBoneAngles(gentity_t *ent, char *bone, vec3_t angles)
{
	int *thebone = &ent->s.boneIndex1;
	int *firstFree = NULL;
	int i = 0;
	int boneIndex = G_BoneIndex(bone);
	int flags, up, right, forward;
	vec3_t *boneVector = &ent->s.boneAngles1;
	vec3_t *freeBoneVec = NULL;

	while (thebone)
	{
		if (!*thebone && !firstFree)
		{ //if the value is 0 then this index is clear, we can use it if we don't find the bone we want already existing.
			firstFree = thebone;
			freeBoneVec = boneVector;
		}
		else if (*thebone)
		{
			if (*thebone == boneIndex)
			{ //this is it
				break;
			}
		}

		switch (i)
		{
		case 0:
			thebone = &ent->s.boneIndex2;
			boneVector = &ent->s.boneAngles2;
			break;
		case 1:
			thebone = &ent->s.boneIndex3;
			boneVector = &ent->s.boneAngles3;
			break;
		case 2:
			thebone = &ent->s.boneIndex4;
			boneVector = &ent->s.boneAngles4;
			break;
		default:
			thebone = NULL;
			boneVector = NULL;
			break;
		}

		i++;
	}

	if (!thebone)
	{ //didn't find it, create it
		if (!firstFree)
		{ //no free bones.. can't do a thing then.
			Com_Printf("WARNING: NPC has no free bone indexes\n");
			return;
		}

		thebone = firstFree;

		*thebone = boneIndex;
		boneVector = freeBoneVec;
	}

	//If we got here then we have a vector and an index.

	//Copy the angles over the vector in the entitystate, so we can use the corresponding index
	//to set the bone angles on the client.
	VectorCopy(angles, *boneVector);

	//Now set the angles on our server instance if we have one.

	if (!ent->ghoul2)
	{
		return;
	}

	flags = BONE_ANGLES_POSTMULT;
	up = POSITIVE_X;
	right = NEGATIVE_Y;
	forward = NEGATIVE_Z;

	//first 3 bits is forward, second 3 bits is right, third 3 bits is up
	ent->s.boneOrient = ((forward)|(right<<3)|(up<<6));

	trap->G2API_SetBoneAngles(ent->ghoul2, 0, bone, angles, flags, up, right, forward, NULL, 100, level.time);
}

//rww - and another method of automatically managing surface status for the client and server at once
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100

void NPC_SetSurfaceOnOff(gentity_t *ent, const char *surfaceName, int surfaceFlags)
{
	int i = 0;
	qboolean foundIt = qfalse;

	while (i < BG_NUM_TOGGLEABLE_SURFACES && bgToggleableSurfaces[i])
	{
		if (!Q_stricmp(surfaceName, bgToggleableSurfaces[i]))
		{ //got it
			foundIt = qtrue;
			break;
		}
		i++;
	}

	if (!foundIt)
	{
		Com_Printf("WARNING: Tried to toggle NPC surface that isn't in toggleable surface list (%s)\n", surfaceName);
		return;
	}

	if (surfaceFlags == TURN_ON)
	{ //Make sure the entitystate values reflect this surface as on now.
		ent->s.surfacesOn |= (1 << i);
		ent->s.surfacesOff &= ~(1 << i);
	}
	else
	{ //Otherwise make sure they're off.
		ent->s.surfacesOn &= ~(1 << i);
		ent->s.surfacesOff |= (1 << i);
	}

	if (!ent->ghoul2)
	{
		return;
	}

	trap->G2API_SetSurfaceOnOff(ent->ghoul2, surfaceName, surfaceFlags);
}

//rww - cheap check to see if an armed client is looking in our general direction
qboolean NPC_SomeoneLookingAtMe(gentity_t *ent)
{
	int i = 0;
	gentity_t *pEnt;

	while (i < MAX_CLIENTS)
	{
		pEnt = &g_entities[i];

		if (pEnt && pEnt->inuse && pEnt->client && pEnt->client->sess.sessionTeam != FACTION_SPECTATOR &&
			pEnt->client->tempSpectate < level.time && !(pEnt->client->ps.pm_flags & PMF_FOLLOW) && pEnt->s.weapon != WP_NONE)
		{
			if (trap->InPVS(ent->r.currentOrigin, pEnt->r.currentOrigin))
			{
				if (InFOV( ent, pEnt, 30, 30 ))
				{ //I'm in a 30 fov or so cone from this player.. that's enough I guess.
					return qtrue;
				}
			}
		}

		i++;
	}

	return qfalse;
}

qboolean NPC_ClearLOS( gentity_t *aiEnt, const vec3_t start, const vec3_t end )
{
	return G_ClearLOS( aiEnt, start, end );
}
qboolean NPC_ClearLOS5(gentity_t *aiEnt, const vec3_t end )
{
	return G_ClearLOS5( aiEnt, end );
}
qboolean NPC_ClearLOS4(gentity_t *aiEnt, gentity_t *ent )
{
	return G_ClearLOS4( aiEnt, ent );
}
qboolean NPC_ClearLOS3(gentity_t *aiEnt, const vec3_t start, gentity_t *ent )
{
	return G_ClearLOS3( aiEnt, start, ent );
}
qboolean NPC_ClearLOS2(gentity_t *ent, const vec3_t end )
{
	return G_ClearLOS2( ent, ent, end );
}

/*
-------------------------
ValidEnemy
-------------------------
*/
extern qboolean NPC_IsAlive (gentity_t *self, gentity_t *NPC);
extern qboolean NPC_IsAnimalEnemyFaction ( gentity_t *self );

/*
qboolean ValidEnemy(gentity_t *aiEnt, gentity_t *ent)
{
	if (ent == NULL)
		return qfalse;

	if (ent == aiEnt)
		return qfalse;

	if (ent->flags & FL_NOTARGET)
	{
		return qfalse;
	}

	//if FACTION_FREE, maybe everyone is an enemy?
	//if ( !NPC->client->enemyTeam )
	//	return qfalse;

	if (NPC_IsAlive(aiEnt, ent))
	{
		if (!ent->client)
		{
			return qtrue;
		}
		else if (ent->client->sess.sessionTeam == FACTION_SPECTATOR)
		{//don't go after spectators
			return qfalse;
		}
		else if (ent->client->tempSpectate >= level.time)
		{//don't go after spectators
			return qfalse;
		}
		else if (ent->s.eType == ET_NPC && (ent->s.NPC_class == CLASS_VEHICLE || ent->client->NPC_class == CLASS_VEHICLE || ent->m_pVehicle))
		{// Don't go after empty vehicles :)
			return qfalse;
		}
		else if (ent == aiEnt->padawan)
		{
			return qfalse;
		}
		else if (ent == aiEnt->parent)
		{
			return qfalse;
		}
		else if (aiEnt == ent->padawan)
		{
			return qfalse;
		}
		else if (aiEnt == ent->parent)
		{
			return qfalse;
		}
		else
		{
			if (!OnSameTeam(ent, aiEnt))
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}
*/

//#define __DEBUG_VALIDENEMY__

qboolean ValidEnemy( gentity_t *self, gentity_t *ent )
{
	if ( !self )
	{//Must be a valid pointer
		return qfalse;
	}

	if ( !ent )
	{//Must be a valid pointer
		return qfalse;
	}

	if ( ent == self )
	{//Must not be me
		return qfalse;
	}

	if ( !ent->inuse )
	{//Must not be deleted
		return qfalse;
	}

	if ( !NPC_IsAlive(self, ent) )
	{//Must be alive
#ifdef __DEBUG_VALIDENEMY__
		if (ent->s.eType == ET_PLAYER)
		{
			Com_Printf("NPC_IsAlive\n");
		}
#endif //__DEBUG_VALIDENEMY__
		return qfalse;
	}

	if (ent->padawan && self == ent->padawan)
	{
		return qfalse;
	}
	
	if (ent->parent && self == ent->parent)
	{
		return qfalse;
	}
	
	if (self->padawan && ent == self->padawan)
	{
		return qfalse;
	}
	
	if (self->parent && ent == self->parent)
	{
		return qfalse;
	}

	if (OnSameTeam(ent, self))
	{
#ifdef __DEBUG_VALIDENEMY__
		if (ent->s.eType == ET_PLAYER)
		{
			Com_Printf("OnSameTeam\n");
		}
#endif //__DEBUG_VALIDENEMY__
		return qfalse;
	}

	if ( ent->flags & FL_NOTARGET )
	{//In case they're in notarget mode
#ifdef __DEBUG_VALIDENEMY__
		if (ent->s.eType == ET_PLAYER)
		{
			Com_Printf("FL_NOTARGET\n");
		}
#endif //__DEBUG_VALIDENEMY__

		return qfalse;
	}

	if (npc_pathing.integer == 0 && VectorLength(ent->spawn_pos) != 0 && Distance(ent->spawn_pos, self->r.currentOrigin) > 12000.0/*6000.0*/)
	{
#ifdef __DEBUG_VALIDENEMY__
		if (ent->s.eType == ET_PLAYER)
		{
			Com_Printf("> 6000.\n");
		}
#endif //__DEBUG_VALIDENEMY__
		return qfalse;
	}

	if ( self && NPC_EntityIsBreakable(self, ent) )
	{// Breakables are perfectly valid targets...
		return qtrue;
	}

	if ( ent->client )
	{// Special client checks...
		if ( ent->client->sess.sessionTeam == FACTION_SPECTATOR )
		{//don't go after spectators
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("FACTION_SPECTATOR\n");
			}
#endif //__DEBUG_VALIDENEMY__
			return qfalse;
		}

		if ( ent->client->tempSpectate >= level.time )
		{//don't go after spectators
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("tempSpectate\n");
			}
#endif //__DEBUG_VALIDENEMY__

			return qfalse;
		}

		if (ent->client->ps.pm_type == PM_SPECTATOR) 
		{
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("PM_SPECTATOR\n");
			}
#endif //__DEBUG_VALIDENEMY__
			return qfalse;
		}

		if (NPC_IsCivilian(self) || NPC_IsCivilian(ent))
		{// These guys have no enemies...
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("NPC_IsCivilian\n");
			}
#endif //__DEBUG_VALIDENEMY__
			return qfalse;
		}

		if (NPC_IsVendor(self) || NPC_IsVendor(ent))
		{// These guys have no enemies...
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("NPC_IsVendor\n");
			}
#endif //__DEBUG_VALIDENEMY__
			return qfalse;
		}
	}

	if (self && self->client && self->isPadawan)
	{
		if (ent == self->parent || ent == self->padawan)
		{// A padawan and his jedi are never enemies...
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("self->padawan\n");
			}
#endif //__DEBUG_VALIDENEMY__
			return qfalse;
		}

		if (self->parent && NPC_IsAlive(self, self->parent))
		{// They just copy their master's enemy instead...
			if (Distance(self->r.currentOrigin, ent->r.currentOrigin) > 1024
				|| Distance(self->parent->r.currentOrigin, ent->r.currentOrigin) > 1024)
			{// Too far from me or my master...
#ifdef __DEBUG_VALIDENEMY__
				if (ent->s.eType == ET_PLAYER)
				{
					Com_Printf("self->parent\n");
				}
#endif //__DEBUG_VALIDENEMY__
				return qfalse;
			}
		}
	}

#if 1
	if (ent->enemy 
		&& NPC_IsAlive(ent, ent->enemy) 
		&& ent->s.weapon == WP_SABER 
		&& ent->enemy->s.weapon == WP_SABER)
	{// Check for saber dueling...
		if (ent->enemy == self)
		{// they are attacking me...

		}
		else if (ent->enemy == self->parent && NPC_IsAlive(ent, self->parent))
		{// padawans will assist their jedi... 

		}
		else if (ent->enemy == self->padawan && NPC_IsAlive(ent, self->padawan))
		{// jedi will assist their padawans... 

		}
		else if (ent->enemy->enemy == ent->enemy && ent->enemy == ent)
		{// Because things seem to default to the player entity, and not NULL after respawns... This should bypass this craziness...
			G_ClearEnemy(ent);
		}
		else if (ent->enemy->enemy == ent && NPC_IsAlive(ent, ent->enemy->enemy))
		{
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("DUELING. ent %i (%s). ent->enemy %i (%s). ent->enemy->enemy %i (%s).\n"
					, ent->s.number, ent->classname
					, ent->enemy->s.number, ent->enemy->classname
					, ent->enemy->enemy->s.number, ent->enemy->enemy->classname);
			}
#endif //__DEBUG_VALIDENEMY__

			return qfalse;
		}
	}
#endif

	if ( self->client && ent->client )
	{// Special client checks...
		if ( NPC_IsAnimalEnemyFaction( self ) && !NPC_IsAnimalEnemyFaction( ent ) )
		{//I get mad at anyone and this guy isn't the same faction as me
			return qtrue;
		}
	}
	
	if (level.gametype < GT_TEAM)
	{// Non-Team Gametypes...
		if (ent->s.eType == ET_PLAYER)
		{// In non-team games all BotNPCs/Players are enemies to eachother...
			return qtrue;
		}

		if ( !ent->client )
		{
			if (ent->s.eType != ET_NPC)
			{//still potentially valid
				if ( ent->alliedTeam == self->client->playerTeam )
				{
#ifdef __DEBUG_VALIDENEMY__
					if (ent->s.eType == ET_PLAYER)
					{
						Com_Printf("alliedTeam\n");
					}
#endif //__DEBUG_VALIDENEMY__
					return qfalse;
				}
				else
				{
					return qtrue;
				}
			}
		}

		//Can't be on the same team
		if ( ent->client->playerTeam == self->client->playerTeam )
		{
#ifdef __DEBUG_VALIDENEMY__
			if (ent->s.eType == ET_PLAYER)
			{
				Com_Printf("playerTeam\n");
			}
#endif //__DEBUG_VALIDENEMY__
			return qfalse;
		}

		if ( self->client->enemyTeam != FACTION_FREE ) //simplest case: they're on my enemy team
		{
			return qtrue;
		}
	}
	else
	{// Team Gametypes...
		// In team games all NPCs and BotNPCs are enemies to other team...
		if (!OnSameTeam(ent, self)) 
			return qtrue;
	}
	
#ifdef __DEBUG_VALIDENEMY__
	if (ent->s.eType == ET_PLAYER)
	{
		Com_Printf("playerTeam\n");
	}
#endif //__DEBUG_VALIDENEMY__
	return qfalse;
}

/*
-------------------------
NPC_TargetVisible
-------------------------
*/

qboolean NPC_TargetVisible( gentity_t *aiEnt, gentity_t *ent )
{
	qboolean IS_BREAKABLE = qfalse;

	if (aiEnt && NPC_EntityIsBreakable(aiEnt, ent))
	{
		IS_BREAKABLE = qtrue;
	}

	//Make sure we're in a valid range
	//if ( !IS_BREAKABLE && DistanceSquared( ent->r.currentOrigin, aiEnt->r.currentOrigin ) > ( aiEnt->NPC->stats.visrange * aiEnt->NPC->stats.visrange ) )
	if ( aiEnt && !IS_BREAKABLE && Distance( ent->r.currentOrigin, aiEnt->r.currentOrigin ) > 3192.0 )
	{
		return qfalse;
	}

	//if ( IS_BREAKABLE && DistanceSquared( ent->breakableOrigin, aiEnt->r.currentOrigin ) > ( aiEnt->NPC->stats.visrange * aiEnt->NPC->stats.visrange ) )
	if ( aiEnt && IS_BREAKABLE && Distance( ent->breakableOrigin, aiEnt->r.currentOrigin ) > 1024.0 )
	{
		//if (IS_BREAKABLE) trap->Print("IS_BREAKABLE failed DIST\n");
		return qfalse;
	}

	//Check our FOV
	//if ( !InFOV( ent, aiEnt, aiEnt->NPC->stats.hfov, aiEnt->NPC->stats.vfov ) )
	/*if ( aiEnt && !InFOV( ent, aiEnt, 120, 180 ) )
	{
		//if (IS_BREAKABLE) trap->Print("IS_BREAKABLE failed FOV\n");
		return qfalse;
	}*/

	//Check for sight
	if ( !IS_BREAKABLE && NPC_ClearLOS4(aiEnt, ent ) == qfalse )
	{
		//if (IS_BREAKABLE) trap->Print("IS_BREAKABLE failed LOS\n");
		return qfalse;
	}

	// UQ1: Also check if we can actually shoot him...
	if (NPC_CheckVisibility(aiEnt, ent, CHECK_VISRANGE|CHECK_SHOOT ) < VIS_SHOOT)
	{
		//if (IS_BREAKABLE) trap->Print("IS_BREAKABLE failed VIS\n");
		return qfalse;
	}

	return qtrue;
}

/*
-------------------------
NPC_GetCheckDelta
-------------------------
*/

/*
#define	CHECK_TIME_BASE				250
#define CHECK_TIME_BASE_SQUARED		( CHECK_TIME_BASE * CHECK_TIME_BASE )

static int NPC_GetCheckDelta( void )
{
	if ( ValidEnemy( NPC->enemy ) == qfalse )
	{
		int distance = DistanceSquared( NPC->r.currentOrigin, g_entities[0].currentOrigin );

		distance /= CHECK_TIME_BASE_SQUARED;

		return ( CHECK_TIME_BASE * distance );
	}

	return 0;
}
*/

/*
-------------------------
NPC_FindNearestEnemy
-------------------------
*/

#define	MAX_RADIUS_ENTS			256	//NOTE: This can cause entities to be lost

int NPC_FindNearestEnemy( gentity_t *ent )
{
	int			iradiusEnts[ MAX_RADIUS_ENTS ];
	gentity_t	*radEnt = NULL;
	gentity_t	*bestJedi = NULL;
	vec3_t		mins, maxs;
	int			nearestEntID = -1;
	float		nearestDist = (float)WORLD_SIZE;
	float		distance;
	int			numEnts = 0;
	int			i;
	float		maxRange = (ent->s.NPC_class == CLASS_ATST || ent->s.NPC_class == CLASS_ATPT || ent->s.NPC_class == CLASS_ATAT) ? 6000.0 : 2048.0/*4096.0*/;//aiEnt->NPC->stats.visrange;

	//Setup the bbox to search in
	for ( i = 0; i < 3; i++ )
	{
		mins[i] = ent->r.currentOrigin[i] - maxRange;
		maxs[i] = ent->r.currentOrigin[i] + maxRange;
	}

	//Get a number of entities in a given space
	numEnts = trap->EntitiesInBox( mins, maxs, iradiusEnts, MAX_RADIUS_ENTS );

	for ( i = 0; i < numEnts; i++ )
	{
		radEnt = &g_entities[iradiusEnts[i]];

		//Don't consider self
		if ( radEnt == ent )
			continue;

		//Must be valid
		if (!ValidEnemy(ent, radEnt))
		{
			continue;
		}

		if (EntIsGlass(radEnt))
		{
			continue;
		}

		if (NPC_EntityIsBreakable(ent, radEnt))
		{
			distance = Distance( ent->r.currentOrigin, radEnt->breakableOrigin );
		}
		else
		{
			distance = Distance( ent->r.currentOrigin, radEnt->r.currentOrigin );
		}

		//Found one closer to us
		if ( distance < nearestDist && distance < maxRange)
		{
			//Must be visible - UQ1: meh, screw it, let them path...
			//if ( NPC_TargetVisible( ent, radEnt ) == qfalse )
			//	continue;

			nearestEntID = radEnt->s.number;
			nearestDist = distance;

			if (radEnt->s.weapon == WP_SABER)
			{
				bestJedi = radEnt;
			}
		}
	}

	if (ent->s.weapon == WP_SABER && bestJedi) 
	{// Jedi prefer to fight other jedi...
		return bestJedi->s.number;
	}

	return nearestEntID;
}

/*
-------------------------
NPC_PickEnemyExt
-------------------------
*/

gentity_t *NPC_PickEnemyExt( gentity_t *aiEnt, qboolean checkAlerts )
{
	//If we've asked for the closest enemy
	int entID = NPC_FindNearestEnemy( aiEnt );

	//If we have a valid enemy, use it
	if (entID >= 0 && entID != aiEnt->s.number)
	{
		return &g_entities[entID];
	}

	if ( checkAlerts )
	{
		int alertEvent = NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qtrue, AEL_DISCOVERED );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			alertEvent_t *event = &level.alertEvents[alertEvent];

			//Don't pay attention to our own alerts
			if ( event->owner == aiEnt )
				return NULL;

			if ( event->level >= AEL_DISCOVERED )
			{
				//If it's the player, attack him
				//OJKFIXME: clientnum 0
				if (event->owner && event->owner->s.eType == ET_PLAYER && ValidEnemy(aiEnt, event->owner))
				{
					return event->owner;
				}

				//If it's on our team, then take its enemy as well
				if (event->owner && event->owner->client && ValidEnemy(aiEnt, event->owner->enemy))
				{
					return event->owner->enemy;
				}
			}
		}
	}

	G_ClearEnemy(aiEnt);
	return NULL;
}

/*
-------------------------
NPC_FindEnemy
-------------------------
*/

qboolean NPC_FindEnemy( gentity_t *aiEnt, qboolean checkAlerts )
{
	gentity_t *newenemy;

	//We're ignoring all enemies for now
	//if( NPC->svFlags & SVF_IGNORE_ENEMIES )
	if (0) //rwwFIXMEFIXME: support for flag
	{
		G_ClearEnemy( aiEnt );
		return qfalse;
	}

	//we can't pick up any enemies for now
	if( aiEnt->NPC->confusionTime > level.time )
	{
		G_ClearEnemy(aiEnt);
		return qfalse;
	}

#if 0
	if (aiEnt->enemy)
	{// In case we get mixed up somewhere... the whole playerTeam thing, *sigh*
		if (!ValidEnemy(aiEnt, aiEnt->enemy))
		{
			G_ClearEnemy(aiEnt);
		}
	}

	//If we've gotten here alright, then our target it still valid
	if (aiEnt->enemy && ValidEnemy(aiEnt, aiEnt->enemy) && aiEnt->next_enemy_check_time > level.time)
	{
		return qtrue;
	}
#else
	G_ClearEnemy(aiEnt);
#endif

	newenemy = NPC_PickEnemyExt(aiEnt, checkAlerts );

	//if we found one, take it as the enemy
	if( ValidEnemy(aiEnt, newenemy ) )
	{
		G_SetEnemy( aiEnt, newenemy );
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
NPC_CheckEnemyExt
-------------------------
*/

qboolean NPC_CheckEnemyExt( gentity_t *aiEnt, qboolean checkAlerts )
{
	if (aiEnt->enemy && ValidEnemy(aiEnt, aiEnt->enemy))
	{
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
NPC_FacePosition
-------------------------
*/
qboolean NPC_FacePosition( gentity_t *aiEnt, vec3_t position, qboolean doPitch )
{
	vec3_t		muzzle;
	vec3_t		angles;
	float		yawDelta;
	qboolean	facing = qtrue;

#if 0
	if (aiEnt->s.NPC_class == CLASS_ATST_OLD)
	{
		extern qboolean ATST_FacePosition(gentity_t *aiEnt, vec3_t position, qboolean doPitch);
		return ATST_FacePosition(aiEnt, position, doPitch);
	}
#endif

	//Get the positions
	if ( aiEnt->client && (aiEnt->client->NPC_class == CLASS_RANCOR || aiEnt->client->NPC_class == CLASS_WAMPA) )// || NPC->client->NPC_class == CLASS_SAND_CREATURE) )
	{
		CalcEntitySpot( aiEnt, SPOT_ORIGIN, muzzle );
		muzzle[2] += aiEnt->r.maxs[2] * 0.75f;
	}
	else if ( aiEnt->client && aiEnt->client->NPC_class == CLASS_GALAKMECH )
	{
		CalcEntitySpot( aiEnt, SPOT_WEAPON, muzzle );
	}
	else
	{
		CalcEntitySpot( aiEnt, SPOT_HEAD_LEAN, muzzle );//SPOT_HEAD
	}

	//Find the desired angles
	GetAnglesForDirection( muzzle, position, angles );

	if (aiEnt->client && aiEnt->client->NPC_class == CLASS_ATST_OLD)
	{
		VectorSubtract(angles, aiEnt->s.apos.trBase, angles);

		angles[ROLL] = 0;
		
		if (!doPitch)
		{
			angles[PITCH] = 0;
		}
	}

	if (!doPitch)
	{
		angles[ROLL] = 0;
		angles[PITCH] = 0;
		VectorCopy(angles, aiEnt->client->ps.viewangles);
	}


	if (aiEnt->client && aiEnt->client->NPC_class == CLASS_ATST)
	{
		char *craniumBone = "cranium";

		vec3_t lookAngles;
		VectorCopy(angles, lookAngles);
		lookAngles[YAW] -= aiEnt->client->ps.viewangles[YAW];

		NPC_SetBoneAngles(aiEnt, craniumBone, lookAngles);
	}
	else if (aiEnt->client && aiEnt->client->NPC_class == CLASS_ATPT)
	{
		char *craniumBone = "cranium";

		vec3_t lookAngles;
		VectorCopy(angles, lookAngles);
		lookAngles[YAW] -= aiEnt->client->ps.viewangles[YAW];

		NPC_SetBoneAngles(aiEnt, craniumBone, lookAngles);
	}
	else if (aiEnt->client && aiEnt->client->NPC_class == CLASS_ATAT)
	{
		char *craniumBone = "cranium";

		vec3_t lookAngles;

		// YAW
		lookAngles[PITCH] = angles[YAW] - aiEnt->client->ps.viewangles[YAW];
		
		//lookAngles[ROLL] = angles[PITCH] - aiEnt->client->ps.viewangles[PITCH];
		lookAngles[ROLL] = 0;
		lookAngles[YAW] = 0;

		NPC_SetBoneAngles(aiEnt, craniumBone, lookAngles);
	}
	

	aiEnt->NPC->desiredYaw		= AngleNormalize360( angles[YAW] );
	aiEnt->NPC->desiredPitch	= AngleNormalize360( angles[PITCH] );

	if ( aiEnt->enemy 
		&& aiEnt->enemy->client
		&& (aiEnt->enemy->client->NPC_class == CLASS_ATST_OLD || aiEnt->enemy->client->NPC_class == CLASS_ATST || aiEnt->enemy->client->NPC_class == CLASS_ATPT || aiEnt->enemy->client->NPC_class == CLASS_ATAT) )
	{
		// FIXME: this is kind of dumb, but it was the easiest way to get it to look sort of ok
		aiEnt->NPC->desiredYaw	+= flrand( -5, 5 ) + sin( level.time * 0.004f ) * 7;
		aiEnt->NPC->desiredPitch += flrand( -2, 2 );
	}
	//Face that yaw
	NPC_UpdateAngles(aiEnt, doPitch, qtrue );

	//Find the delta between our goal and our current facing
	yawDelta = AngleNormalize360( aiEnt->NPC->desiredYaw - ( SHORT2ANGLE( aiEnt->client->pers.cmd.angles[YAW] + aiEnt->client->ps.delta_angles[YAW] ) ) );

	//See if we are facing properly
	if ( fabs( yawDelta ) > VALID_ATTACK_CONE )
		facing = qfalse;

	if ( doPitch )
	{
		//Find the delta between our goal and our current facing
		float currentAngles = ( SHORT2ANGLE( aiEnt->client->pers.cmd.angles[PITCH] + aiEnt->client->ps.delta_angles[PITCH] ) );
		float pitchDelta = aiEnt->NPC->desiredPitch - currentAngles;

		//See if we are facing properly
		if ( fabs( pitchDelta ) > VALID_ATTACK_CONE )
			facing = qfalse;
	}
	else
	{// UQ1: Force pitch always to go back to 0...
		aiEnt->NPC->desiredPitch = 0;

		{
			//Find the delta between our goal and our current facing
			float currentAngles = ( SHORT2ANGLE( aiEnt->client->pers.cmd.angles[PITCH] + aiEnt->client->ps.delta_angles[PITCH] ) );
			float pitchDelta = aiEnt->NPC->desiredPitch - currentAngles;

			//See if we are facing properly
			if ( fabs( pitchDelta ) > VALID_ATTACK_CONE )
				facing = qfalse;
		}
	}

	if (aiEnt->s.eType == ET_PLAYER)
	{
		trap->EA_View(aiEnt->s.number, angles);
	}

	return facing;
}

/*
-------------------------
NPC_FaceEntity
-------------------------
*/

qboolean NPC_FaceEntity( gentity_t *aiEnt, gentity_t *ent, qboolean doPitch )
{
	vec3_t		entPos;

	//Get the positions
	CalcEntitySpot(ent, SPOT_HEAD_LEAN, entPos);

	return NPC_FacePosition(aiEnt, entPos, doPitch );
}

/*
-------------------------
NPC_FaceEnemy
-------------------------
*/

extern qboolean ATST_FaceEnemy(gentity_t *aiEnt, qboolean doPitch);

qboolean NPC_FaceEnemy( gentity_t *aiEnt, qboolean doPitch )
{
	if ( aiEnt == NULL )
		return qfalse;

	if ( aiEnt->enemy == NULL )
		return qfalse;

#if 0
	if (aiEnt->s.NPC_class == CLASS_ATST_OLD)
	{
		return ATST_FaceEnemy(aiEnt, qtrue/*doPitch*/);
	}
#endif

	return NPC_FaceEntity(aiEnt, aiEnt->enemy, doPitch );
}

/*
-------------------------
NPC_CheckCanAttackExt
-------------------------
*/

qboolean NPC_CheckCanAttackExt( gentity_t *aiEnt)
{
	//We don't want them to shoot
	if( aiEnt->NPC->scriptFlags & SCF_DONT_FIRE )
		return qfalse;

	//Turn to face
	if ( NPC_FaceEnemy(aiEnt, qtrue ) == qfalse )
		return qfalse;

	//Must have a clear line of sight to the target
	if ( NPC_ClearShot(aiEnt, aiEnt->enemy ) == qfalse )
		return qfalse;

	return qtrue;
}

/*
-------------------------
NPC_ClearLookTarget
-------------------------
*/

void NPC_ClearLookTarget( gentity_t *self )
{
	if ( !self->client )
	{
		return;
	}

	if ( (self->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
	{//lookTarget is set by and to the monster that's holding you, no other operations can change that
		return;
	}

	self->client->renderInfo.lookTarget = ENTITYNUM_NONE;//ENTITYNUM_WORLD;
	self->client->renderInfo.lookTargetClearTime = 0;
}

/*
-------------------------
NPC_SetLookTarget
-------------------------
*/
void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime )
{
	if ( !self->client )
	{
		return;
	}

	if ( (self->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
	{//lookTarget is set by and to the monster that's holding you, no other operations can change that
		return;
	}

	self->client->renderInfo.lookTarget = entNum;
	self->client->renderInfo.lookTargetClearTime = clearTime;
}

/*
-------------------------
NPC_CheckLookTarget
-------------------------
*/
qboolean NPC_CheckLookTarget( gentity_t *self )
{
	if ( self->client )
	{
		if ( self->client->renderInfo.lookTarget >= 0 && self->client->renderInfo.lookTarget < ENTITYNUM_WORLD )
		{//within valid range
			if ( (&g_entities[self->client->renderInfo.lookTarget] == NULL) || !g_entities[self->client->renderInfo.lookTarget].inuse )
			{//lookTarget not inuse or not valid anymore
				NPC_ClearLookTarget( self );
			}
			else if ( self->client->renderInfo.lookTargetClearTime && self->client->renderInfo.lookTargetClearTime < level.time )
			{//Time to clear lookTarget
				NPC_ClearLookTarget( self );
			}
			else if ( g_entities[self->client->renderInfo.lookTarget].client && self->enemy && (&g_entities[self->client->renderInfo.lookTarget] != self->enemy) )
			{//should always look at current enemy if engaged in battle... FIXME: this could override certain scripted lookTargets...???
				NPC_ClearLookTarget( self );
			}
			else
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_CheckCharmed
-------------------------
*/
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
void NPC_CheckCharmed( gentity_t *aiEnt)
{
	if ( aiEnt->NPC->charmedTime && aiEnt->NPC->charmedTime < level.time && aiEnt->client )
	{//we were charmed, set us back!
		aiEnt->client->playerTeam	= (npcteam_t)aiEnt->genericValue1;
		aiEnt->client->enemyTeam		= (npcteam_t)aiEnt->genericValue2;
		aiEnt->s.teamowner			= aiEnt->genericValue3;

		aiEnt->client->leader = NULL;
		if ( aiEnt->NPC->tempBehavior == BS_FOLLOW_LEADER )
		{
			aiEnt->NPC->tempBehavior = BS_DEFAULT;
		}
		G_ClearEnemy( aiEnt );
		aiEnt->NPC->charmedTime = 0;
		//say something to let player know you've snapped out of it
		G_AddVoiceEvent( aiEnt, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000 );
	}
}

void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex )
{
	mdxaBone_t	boltMatrix;
	vec3_t		result, angles;

	if (!self || !self->inuse)
	{
		return;
	}

	if (self->client)
	{ //clients don't actually even keep r.currentAngles maintained
		VectorSet(angles, 0, self->client->ps.viewangles[YAW], 0);
	}
	else
	{
		VectorSet(angles, 0, self->r.currentAngles[YAW], 0);
	}

	if ( /*!self || ...haha (sorry, i'm tired)*/ !self->ghoul2 )
	{
		return;
	}

	trap->G2API_GetBoltMatrix( self->ghoul2, modelIndex,
				boltIndex,
				&boltMatrix, angles, self->r.currentOrigin, level.time,
				NULL, self->modelScale );
	if ( pos )
	{
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, result );
		VectorCopy( result, pos );
	}
}

float NPC_EntRangeFromBolt( gentity_t *aiEnt, gentity_t *targEnt, int boltIndex )
{
	vec3_t	org;

	if ( !targEnt )
	{
		return Q3_INFINITE;
	}

	G_GetBoltPosition( aiEnt, boltIndex, org, 0 );

	return (Distance( targEnt->r.currentOrigin, org ));
}

float NPC_EnemyRangeFromBolt( gentity_t *aiEnt, int boltIndex )
{
	return (NPC_EntRangeFromBolt(aiEnt, aiEnt->enemy, boltIndex ));
}

int NPC_GetEntsNearBolt( gentity_t *aiEnt, int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg )
{
	vec3_t		mins, maxs;
	int			i;

	//get my handRBolt's position
	vec3_t	org;

	G_GetBoltPosition( aiEnt, boltIndex, org, 0 );

	VectorCopy( org, boltOrg );

	//Setup the bbox to search in
	for ( i = 0; i < 3; i++ )
	{
		mins[i] = boltOrg[i] - radius;
		maxs[i] = boltOrg[i] + radius;
	}

	//Get the number of entities in a given space
	return (trap->EntitiesInBox( mins, maxs, radiusEnts, 128 ));
}
