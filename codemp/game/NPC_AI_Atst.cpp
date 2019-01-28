#include "b_local.h"

#define	MIN_MELEE_RANGE		640
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define TURN_OFF			0x00000100//G2SURFACEFLAG_NODESCENDANTS

#define LEFT_ARM_HEALTH 40
#define RIGHT_ARM_HEALTH 40

extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
/*
-------------------------
NPC_ATST_Precache
-------------------------
*/
void NPC_ATST_Precache(void)
{
	G_SoundIndex( "sound/chars/atst/atst_damaged1" );
	G_SoundIndex( "sound/chars/atst/atst_damaged2" );

//	RegisterItem( BG_FindItemForWeapon( WP_ATST_MAIN ));	//precache the weapon
	//rwwFIXMEFIXME: add this weapon
	RegisterItem( BG_FindItemForWeapon( WP_MODULIZED_WEAPON ));	//precache the weapon
	//RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ));	//precache the weapon

	G_EffectIndex( "env/med_explode2" );
//	G_EffectIndex( "smaller_chunks" );
	G_EffectIndex( "blaster/smoke_bolton" );
	G_EffectIndex( "explosions/droidexplosion1" );
}

#define ATST_VALID_ATTACK_CONE 30

extern void G_Knockdown(gentity_t *victim);

qboolean ATST_UpdateAngles(gentity_t *aiEnt, qboolean doPitch, qboolean doYaw)
{
	float		error;
	float		decay;
	float		targetPitch = 0;
	float		targetYaw = 0;
	float		yawSpeed;
	qboolean	exact = qtrue;

	// if angle changes are locked; just keep the current angles
	// aimTime isn't even set anymore... so this code was never reached, but I need a way to lock NPC's yaw, so instead of making a new SCF_ flag, just use the existing render flag... - dmv
	//if (!aiEnt->enemy && ((level.time < aiEnt->NPC->aimTime) /*|| NPC->client->renderInfo.renderFlags & RF_LOCKEDANGLE*/))
	//{
	//	if (doPitch)
	//		targetPitch = aiEnt->NPC->lockedDesiredPitch;

	//	if (doYaw)
	//		targetYaw = aiEnt->NPC->lockedDesiredYaw;
	//}
	//else
	{
		// we're changing the lockedDesired Pitch/Yaw below so it's lost it's original meaning, get rid of the lock flag
		//	NPC->client->renderInfo.renderFlags &= ~RF_LOCKEDANGLE;

		if (doPitch)
		{
			targetPitch = aiEnt->NPC->desiredPitch;
			aiEnt->NPC->lockedDesiredPitch = aiEnt->NPC->desiredPitch;
		}

		if (doYaw)
		{
			targetYaw = aiEnt->NPC->desiredYaw;
			aiEnt->NPC->lockedDesiredYaw = aiEnt->NPC->desiredYaw;
		}
	}

	if (aiEnt->s.weapon == WP_EMPLACED_GUN)
	{
		// FIXME: this seems to do nothing, actually...
		yawSpeed = 20;
	}
	else
	{
		yawSpeed = aiEnt->NPC->stats.yawSpeed;
	}

	if (aiEnt->s.weapon == WP_SABER && aiEnt->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
	{
		char buf[128];
		float tFVal = 0;

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);

		yawSpeed *= 1.0f / tFVal;
}

	if (doYaw)
	{
		// decay yaw error
		error = AngleDelta(aiEnt->client->ps.viewangles[YAW], targetYaw);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0f / 1000.0f;//msec

				if (error < 0.0)
				{
					error += decay;
					if (error > 0.0)
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if (error < 0.0)
					{
						error = 0.0;
					}
				}
			}
		}

		aiEnt->client->pers.cmd.angles[YAW] = ANGLE2SHORT(targetYaw + error) - aiEnt->client->ps.delta_angles[YAW];
	}

	//FIXME: have a pitchSpeed?
	if (doPitch)
	{
		// decay pitch error
		error = AngleDelta(aiEnt->client->ps.viewangles[PITCH], targetPitch);
		if (fabs(error) > MIN_ANGLE_ERROR)
		{
			if (error)
			{
				exact = qfalse;

				decay = 60.0 + yawSpeed * 3;
				decay *= 50.0f / 1000.0f;//msec

				if (error < 0.0)
				{
					error += decay;
					if (error > 0.0)
					{
						error = 0.0;
					}
				}
				else
				{
					error -= decay;
					if (error < 0.0)
					{
						error = 0.0;
					}
				}
			}
		}

		aiEnt->client->pers.cmd.angles[PITCH] = ANGLE2SHORT(targetPitch + error) - aiEnt->client->ps.delta_angles[PITCH];
	}

	aiEnt->client->pers.cmd.angles[ROLL] = ANGLE2SHORT(aiEnt->client->ps.viewangles[ROLL]) - aiEnt->client->ps.delta_angles[ROLL];

#ifndef __NO_ICARUS__
	if (exact && trap->ICARUS_TaskIDPending((sharedEntity_t *)aiEnt, TID_ANGLE_FACE))
	{
		trap->ICARUS_TaskIDComplete((sharedEntity_t *)aiEnt, TID_ANGLE_FACE);
	}
#endif //__NO_ICARUS__

	return exact;
}

qboolean ATST_FacePosition(gentity_t *aiEnt, vec3_t position, qboolean doPitch)
{
	vec3_t		muzzle;
	vec3_t		angles;
	float		yawDelta;
	qboolean	facing = qtrue;

	//Get the positions
	//if (aiEnt->client && (aiEnt->client->NPC_class == CLASS_RANCOR || aiEnt->client->NPC_class == CLASS_WAMPA))// || NPC->client->NPC_class == CLASS_SAND_CREATURE) )
	//{
		//CalcEntitySpot(aiEnt, SPOT_ORIGIN, muzzle);
	VectorCopy(aiEnt->r.currentOrigin, muzzle);
		muzzle[2] += aiEnt->r.maxs[2] * 0.75f;
	//}
	//else if (aiEnt->client && aiEnt->client->NPC_class == CLASS_GALAKMECH)
	//{
	//	CalcEntitySpot(aiEnt, SPOT_WEAPON, muzzle);
	//}
	//else
	//{
	//	CalcEntitySpot(aiEnt, SPOT_HEAD_LEAN, muzzle);//SPOT_HEAD
	//}

	//Find the desired angles
	GetAnglesForDirection(muzzle, position, angles);


	/*if (aiEnt->client->NPC_class == CLASS_ATST)
	{// ATST's are just fucking wierd...
		angles[YAW] -= aiEnt->r.currentAngles[YAW];

		if (doPitch)
		{
			angles[ROLL] -= aiEnt->r.currentAngles[ROLL];
			angles[PITCH] -= aiEnt->r.currentAngles[PITCH];
		}
	}*/

	//aiEnt->r.currentAngles[YAW] = angles[YAW];
	//aiEnt->r.currentAngles[ROLL] = angles[ROLL];
	//aiEnt->r.currentAngles[PITCH] = angles[PITCH];

	aiEnt->NPC->desiredYaw = AngleNormalize360(angles[YAW] - aiEnt->r.currentAngles[YAW]);
	aiEnt->NPC->desiredPitch = AngleNormalize360(angles[PITCH] - aiEnt->r.currentAngles[PITCH]);


	if (aiEnt->enemy && aiEnt->enemy->client && aiEnt->enemy->client->NPC_class == CLASS_ATST)
	{
		// FIXME: this is kind of dumb, but it was the easiest way to get it to look sort of ok
		aiEnt->NPC->desiredYaw += flrand(-5, 5) + sin(level.time * 0.004f) * 7;
		aiEnt->NPC->desiredPitch += flrand(-2, 2);
	}

	//Face that yaw
	ATST_UpdateAngles(aiEnt, doPitch, qtrue);

	//Find the delta between our goal and our current facing
	yawDelta = AngleNormalize360(aiEnt->NPC->desiredYaw - (SHORT2ANGLE(aiEnt->client->pers.cmd.angles[YAW] + aiEnt->client->ps.delta_angles[YAW])));

	//See if we are facing properly
	if (fabs(yawDelta) > ATST_VALID_ATTACK_CONE)
		facing = qfalse;

	if (doPitch)
	{
		//Find the delta between our goal and our current facing
		float currentAngles = (SHORT2ANGLE(aiEnt->client->pers.cmd.angles[PITCH] + aiEnt->client->ps.delta_angles[PITCH]));
		float pitchDelta = aiEnt->NPC->desiredPitch - currentAngles;

		//See if we are facing properly
		if (fabs(pitchDelta) > ATST_VALID_ATTACK_CONE)
			facing = qfalse;
	}

	return facing;
}

qboolean ATST_FaceEnemy(gentity_t *aiEnt, qboolean doPitch)
{
	vec3_t		position;

	CalcEntitySpot(aiEnt->enemy, SPOT_CHEST, position);
	//VectorCopy(aiEnt->enemy->r.currentOrigin, position);

	return ATST_FacePosition(aiEnt, position, doPitch);
}

qboolean ATST_MoveToGoal(gentity_t *aiEnt, qboolean tryStraight)
{
	int			radius = 256;
	int			halfRadius = radius / 2;
	vec3_t		fwdangles, forward, right, center, mins, maxs;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities = 0;

	VectorCopy(aiEnt->r.currentAngles, fwdangles);
	AngleVectors(fwdangles, forward, right, NULL);

	if (!aiEnt->enemy || !NPC_HaveValidEnemy(aiEnt))
	{// When no valid enemy, reset our head toward move direction...
		vec3_t position;
		VectorMA(aiEnt->r.currentOrigin, 256.0, forward, position);
		ATST_FacePosition(aiEnt, position, qfalse);
	}
	else
	{// Face our target...
		ATST_FaceEnemy(aiEnt, qtrue);
	}

	//
	// Push away anyone in our way...
	//
	VectorCopy(aiEnt->r.currentOrigin, center);

	for (int i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	numListedEntities = trap->EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (int i = 0; i < numListedEntities; i++)
	{
		gentity_t *ent = &g_entities[i];

		// Only NPCs or PLAYERS...
		if (!ent || !ent->client) 
			continue;

		if (ent == aiEnt)
			continue;

		// Not if this is a vehicle or a player in a vehicle...
		if (ent->client->ps.m_iVehicleNum || (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_VEHICLE))
			continue;

		if (ent->client->NPC_class == CLASS_RANCOR
			|| ent->client->NPC_class == CLASS_ATST
			|| ent->client->NPC_class == CLASS_REEK
			|| ent->client->NPC_class == CLASS_ACKLAY
			|| ent->client->NPC_class == CLASS_ATST)
		{
			continue;
		}

		float dist = Distance(ent->r.currentOrigin, aiEnt->r.currentOrigin);

		if (dist > radius)
			continue;

		// Needs to be in the "push arc"...
		if (!InFOV3(ent->r.currentOrigin, aiEnt->r.currentOrigin, fwdangles, 120, 120))
			continue;

		// In the arc, and is a NPC or PLAYER...
		vec3_t pushDir;
		VectorSubtract(ent->r.currentOrigin, aiEnt->r.currentOrigin, pushDir);
		VectorNormalize(pushDir);
		
		if (dist < halfRadius)
		{//close enough to do damage, too
			G_Damage(ent, aiEnt, aiEnt, vec3_origin, ent->r.currentOrigin, Q_irand( 30, 85 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
		}

		// Throw them away...
		//vmCvar_t npc_atstpush;
		//trap->Cvar_Register(&npc_atstpush, "npc_atstpush", "64.0", CVAR_ARCHIVE);
		ent->client->ps.velocity[0] = aiEnt->client->ps.velocity[0] * 8.0;
		ent->client->ps.velocity[1] = aiEnt->client->ps.velocity[1] * 8.0;
		ent->client->ps.velocity[2] = 64.0;// npc_atstpush.value;

		// Knock them down...
		G_Knockdown(ent);
	}

	// And finally, move...
	return NPC_CombatMoveToGoal(aiEnt, tryStraight, qfalse); // UQ1: Check for falling...
}

//-----------------------------------------------------------------
#if 0
static void ATST_PlayEffect( gentity_t *self, const int boltID, const char *fx )
{
	if ( boltID >=0 && fx && fx[0] )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir;

		trap->G2API_GetBoltMatrix( self->ghoul2, 0,
					boltID,
					&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
					NULL, self->modelScale );

		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
		BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );

		G_PlayEffectID( G_EffectIndex((char *)fx), org, dir );
	}
}
#endif

/*
-------------------------
G_ATSTCheckPain

Called by NPC's and player in an ATST
-------------------------
*/

void G_ATSTCheckPain( gentity_t *self, gentity_t *other, int damage )
{
	//int newBolt;
	//int hitLoc = gPainHitLoc;

	if ( rand() & 1 )
	{
		G_SoundOnEnt( self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged1" );
	}
	else
	{
		G_SoundOnEnt( self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged2" );
	}

	/*
	if ((hitLoc==HL_ARM_LT) && (self->locationDamage[HL_ARM_LT] > LEFT_ARM_HEALTH))
	{
		if (self->locationDamage[hitLoc] >= LEFT_ARM_HEALTH)	// Blow it up?
		{
			newBolt = trap->G2API_AddBolt( self->ghoul2, 0, "*flash3" );
			if ( newBolt != -1 )
			{
//				G_PlayEffect( "small_chunks", self->playerModel, self->genericBolt1, self->s.number);
				ATST_PlayEffect( self, trap->G2API_AddBolt(self->ghoul2, 0, "*flash3"), "env/med_explode2" );
				//G_PlayEffectID( G_EffectIndex("blaster/smoke_bolton"), self->playerModel, newBolt, self->s.number);
				//Maybe bother with this some other time.
			}

			NPC_SetSurfaceOnOff( self, "head_light_blaster_cann", TURN_OFF );
		}
	}
	else if ((hitLoc==HL_ARM_RT) && (self->locationDamage[HL_ARM_RT] > RIGHT_ARM_HEALTH))	// Blow it up?
	{
		if (self->locationDamage[hitLoc] >= RIGHT_ARM_HEALTH)
		{
			newBolt = trap->G2API_AddBolt( self->ghoul2, 0, "*flash4" );
			if ( newBolt != -1 )
			{
//				G_PlayEffect( "small_chunks", self->playerModel, self->genericBolt2, self->s.number);
				ATST_PlayEffect( self, trap->G2API_AddBolt(self->ghoul2, 0, "*flash4"), "env/med_explode2" );
				//G_PlayEffect( "blaster/smoke_bolton", self->playerModel, newBolt, self->s.number);
			}

			NPC_SetSurfaceOnOff( self, "head_concussion_charger", TURN_OFF );
		}
	}
	*/
}
/*
-------------------------
NPC_ATST_Pain
-------------------------
*/
void NPC_ATST_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	G_ATSTCheckPain( self, attacker, damage );
	NPC_Pain( self, attacker, damage );
}

/*
-------------------------
ATST_Hunt
-------------------------`
*/
void ATST_Hunt(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	if ( aiEnt->NPC->goalEntity == NULL )
	{//hunt
		aiEnt->NPC->goalEntity = aiEnt->enemy;
	}

	aiEnt->NPC->combatMove = qtrue;

	ATST_MoveToGoal(aiEnt, qtrue );
}

/*
-------------------------
ATST_Ranged
-------------------------
*/
void ATST_Ranged(gentity_t *aiEnt, qboolean visible, qboolean advance, qboolean altAttack )
{
	if (altAttack)
	{
		if (TIMER_Done(aiEnt, "atkAltDelay") && visible)	// Attack?
		{
			aiEnt->s.weapon = aiEnt->client->ps.weapon = WP_MODULIZED_WEAPON;
			TIMER_Set(aiEnt, "atkAltDelay", 500);
			FireWeapon(aiEnt, qtrue);
		}
	}
	else
	{
		if (TIMER_Done(aiEnt, "atkDelay") && visible)	// Attack?
		{
			aiEnt->s.weapon = aiEnt->client->ps.weapon = WP_MODULIZED_WEAPON;
			TIMER_Set(aiEnt, "atkDelay", 100);
			FireWeapon(aiEnt, qfalse);
		}
	}

	if ( /*(aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES) ||*/ advance )
	{
		ATST_Hunt(aiEnt, visible, advance );
	}
}

/*
-------------------------
ATST_Attack
-------------------------
*/
void ATST_Attack(gentity_t *aiEnt)
{
	qboolean	altAttack=qfalse;
	int			blasterTest,chargerTest,weapon;
	float		distance;
	distance_e	distRate;
	qboolean	visible;
	qboolean	advance;

	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )
	{
		aiEnt->enemy = NULL;
		return;
	}

	if (!NPC_HaveValidEnemy(aiEnt))
	{
		aiEnt->enemy = NULL;
		return;
	}

	ATST_FaceEnemy(aiEnt, qtrue);

	// Rate our distance to the target, and our visibilty
	distance	= (int) Distance( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
	distRate = DIST_LONG;// (distance > MIN_MELEE_RANGE_SQR) ? DIST_LONG : DIST_MELEE;
	visible = qtrue;// NPC_ClearLOS4(aiEnt, aiEnt->enemy);
	advance		= (qboolean)(distance > 1024.0);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
		{
			ATST_Hunt(aiEnt, visible, advance );
			return;
		}
	}

#if 1
	// Decide what type of attack to do
	switch ( distRate )
	{
	case DIST_MELEE:
//		NPC_ChangeWeapon( WP_ATST_MAIN );
		break;

	case DIST_LONG:

//		NPC_ChangeWeapon( WP_ATST_SIDE );
		//rwwFIXMEFIXME: make atst weaps work.

		// See if the side weapons are there
		blasterTest = trap->G2API_GetSurfaceRenderStatus( aiEnt->ghoul2, 0, "head_light_blaster_cann" );
		chargerTest = trap->G2API_GetSurfaceRenderStatus( aiEnt->ghoul2, 0, "head_concussion_charger" );

		// It has both side weapons
		if ( blasterTest != -1
			&& !(blasterTest&TURN_OFF)
			&& chargerTest != -1
			&& !(chargerTest&TURN_OFF))
		{
			weapon = Q_irand( 0, 1);	// 0 is blaster, 1 is charger (ALT SIDE)

			if (weapon)				// Fire charger
			{
				altAttack = qtrue;
			}
			else
			{
				altAttack = qfalse;
			}

		}
		else if (blasterTest != -1
			&& !(blasterTest & TURN_OFF))	// Blaster is on
		{
			altAttack = qfalse;
		}
		else if (chargerTest != -1
			&&!(chargerTest & TURN_OFF))	// Blaster is on
		{
			altAttack = qtrue;
		}
		else
		{
			//NPC_ChangeWeapon(aiEnt, WP_NONE );
		}
		break;
	}
#endif

	ATST_FaceEnemy(aiEnt, qtrue);

	aiEnt->noWaypointTime = level.time + 2000;
	ATST_Ranged(aiEnt, visible, advance, altAttack );
}

/*
-------------------------
ATST_Patrol
-------------------------
*/
void ATST_Patrol(gentity_t *aiEnt)
{
	if ( NPC_CheckPlayerTeamStealth(aiEnt) )
	{
		ATST_UpdateAngles(aiEnt, qfalse, qtrue );
		return;
	}

	/*
	//If we have somewhere to go, then do that
	if (!aiEnt->enemy)
	{
		if ( UpdateGoal(aiEnt) )
		{
			aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			ATST_MoveToGoal(aiEnt, qtrue );
			ATST_UpdateAngles(aiEnt, qtrue, qtrue );
		}
	}
	*/

	NPC_PatrolArea(aiEnt);
}

/*
-------------------------
ATST_Idle
-------------------------
*/
void ATST_Idle(gentity_t *aiEnt)
{

	//NPC_BSIdle(aiEnt);
	//NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL );

	NPC_PatrolArea(aiEnt);
}

/*
-------------------------
NPC_BSDroid_Default
-------------------------
*/
void NPC_BSATST_Default(gentity_t *aiEnt)
{
	if ( aiEnt->enemy && NPC_HaveValidEnemy(aiEnt) )
	{
		if( (aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES) )
		{
			aiEnt->NPC->goalEntity = aiEnt->enemy;
		}
		ATST_Attack(aiEnt);
	}
	else if ( aiEnt->NPC->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		ATST_Patrol(aiEnt);
	}
	else
	{
		ATST_Idle(aiEnt);
	}
}
