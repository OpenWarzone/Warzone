#include "b_local.h"

#define	ATST_VALID_ATTACK_CONE		2.0f	//Degrees

void ATST_Patrol(gentity_t *aiEnt);

extern void G_SoundOnEnt(gentity_t *ent, soundChannel_t channel, const char *soundPath);
extern qboolean NPC_MoveIntoOptimalAttackPosition(gentity_t *aiEnt);

/*
-------------------------
NPC_ATST_Precache
-------------------------
*/
void NPC_ATST_Precache(void)
{
	G_SoundIndex("sound/chars/atst/atst_damaged1");
	G_SoundIndex("sound/chars/atst/atst_damaged2");

	G_EffectIndex("env/med_explode2");
	G_EffectIndex("blaster/smoke_bolton");
	G_EffectIndex("explosions/droidexplosion1");
}

/*
-------------------------
G_ATSTCheckPain
Called by NPC's and player in an ATST
-------------------------
*/

void G_ATSTCheckPain(gentity_t *self, gentity_t *other, int damage)
{
	if (rand() & 1)
	{
		G_SoundOnEnt(self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged1");
	}
	else
	{
		G_SoundOnEnt(self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged2");
	}
}

/*
-------------------------
NPC_ATST_Pain
-------------------------
*/
void NPC_ATST_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	G_ATSTCheckPain(self, attacker, damage);
	NPC_Pain(self, attacker, damage);
}

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
	if (!aiEnt->enemy && ((level.time < aiEnt->NPC->aimTime) /*|| NPC->client->renderInfo.renderFlags & RF_LOCKEDANGLE*/))
	{
		if (doPitch)
			targetPitch = aiEnt->NPC->lockedDesiredPitch;

		if (doYaw)
			targetYaw = aiEnt->NPC->lockedDesiredYaw;
	}
	else
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
		//aiEnt->s.apos.trBase[YAW] = 0;// targetYaw + error;
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
		//aiEnt->s.apos.trBase[PITCH] = 0;// targetPitch + error;
	}

	aiEnt->client->pers.cmd.angles[ROLL] = ANGLE2SHORT(aiEnt->client->ps.viewangles[ROLL]) - aiEnt->client->ps.delta_angles[ROLL];
	//aiEnt->s.apos.trBase[ROLL] = 0;// aiEnt->client->ps.viewangles[ROLL];

#ifndef __NO_ICARUS__
	if (exact && trap->ICARUS_TaskIDPending((sharedEntity_t *)aiEnt, TID_ANGLE_FACE))
	{
		trap->ICARUS_TaskIDComplete((sharedEntity_t *)aiEnt, TID_ANGLE_FACE);
	}
#endif //__NO_ICARUS__
	return exact;
}

/*
-------------------------
NPC_FacePosition
-------------------------
*/

qboolean ATST_FacePosition(gentity_t *aiEnt, vec3_t position, qboolean doPitch)
{
	vec3_t		muzzle;
	vec3_t		angles;
	float		yawDelta;
	qboolean	facing = qtrue;

#if 1
	//Get the positions
	CalcEntitySpot(aiEnt, SPOT_HEAD_LEAN, muzzle);//SPOT_HEAD

												  //Find the desired angles
												  //GetAnglesForDirection(muzzle, position, angles);
	vec3_t dir;
	VectorSubtract(muzzle, position, dir);
	VectorNormalize(dir);
	vectoangles(dir, angles);

	if (!doPitch)
	{
		angles[ROLL] = 0;
		angles[PITCH] = 0;
		//VectorCopy(angles, aiEnt->client->ps.viewangles);
	}
	else
	{
		angles[ROLL] = 0;
		//VectorCopy(angles, aiEnt->client->ps.viewangles);
	}

	G_SetAngles(aiEnt, angles);

	//SetClientViewAngle(aiEnt, angles);
	//aiEnt->client->pers.cmd.angles[0] = aiEnt->client->pers.cmd.angles[1] = aiEnt->client->pers.cmd.angles[2] = 0;

	//aiEnt->client->pers.cmd.angles[YAW] = ANGLE2SHORT(aiEnt->client->ps.viewangles[YAW]) - aiEnt->client->ps.delta_angles[YAW];

	aiEnt->NPC->desiredYaw = (AngleNormalize360(angles[YAW]) - 180.0) * 0.5; // UQ1: WTF, these models have inverted/scaled bones or something???
	aiEnt->NPC->desiredPitch = (AngleNormalize360(angles[PITCH]) -180.0) * 0.5;

	/*if (g_testvalue3.integer)
	{
		Com_Printf("current p: %f y: %f. desired p: %f. y: %f. angles: p: %f. y: %f. view p: %f. y: %f.\n"
			, aiEnt->s.apos.trBase[PITCH], aiEnt->s.apos.trBase[YAW]
			, aiEnt->NPC->desiredPitch, aiEnt->NPC->desiredYaw
			, angles[PITCH], angles[YAW]
			, aiEnt->client->ps.viewangles[PITCH], aiEnt->client->ps.viewangles[YAW]);
	}*/

	if (aiEnt->enemy && aiEnt->enemy->client && aiEnt->enemy->client->NPC_class == CLASS_ATST)
	{
		// FIXME: this is kind of dumb, but it was the easiest way to get it to look sort of ok
		aiEnt->NPC->desiredYaw += flrand(-5, 5) + sin(level.time * 0.004f) * 7;
		aiEnt->NPC->desiredPitch += flrand(-2, 2);
	}

	//Face that yaw
	NPC_UpdateAngles(aiEnt, doPitch, qtrue);

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
	else
	{// UQ1: Force pitch always to go back to 0...
		aiEnt->NPC->desiredPitch = 0;

		{
			//Find the delta between our goal and our current facing
			float currentAngles = (SHORT2ANGLE(aiEnt->client->pers.cmd.angles[PITCH] + aiEnt->client->ps.delta_angles[PITCH]));
			float pitchDelta = aiEnt->NPC->desiredPitch - currentAngles;

			//See if we are facing properly
			if (fabs(pitchDelta) > ATST_VALID_ATTACK_CONE)
				facing = qfalse;
		}
	}
#elif 1
	//Get the positions
	CalcEntitySpot(aiEnt, SPOT_HEAD_LEAN, muzzle);//SPOT_HEAD

												  //Find the desired angles
												  //GetAnglesForDirection(muzzle, position, angles);
	vec3_t dir;
	VectorSubtract(position, muzzle, dir);
	vectoangles(dir, angles);

	if (!doPitch)
	{
		angles[ROLL] = 0;
		angles[PITCH] = 0;
	}
	else
	{
		angles[ROLL] = 0;
	}

	VectorClear(aiEnt->client->ps.viewangles);
	VectorClear(aiEnt->s.apos.trBase);
	VectorClear(aiEnt->s.apos.trDelta);
	VectorClear(aiEnt->s.angles);
	VectorClear(aiEnt->s.angles2);
	VectorClear(aiEnt->r.currentAngles);
	aiEnt->client->ps.delta_angles[PITCH] = aiEnt->client->ps.delta_angles[ROLL] = aiEnt->client->ps.delta_angles[YAW] = 0;
	aiEnt->client->pers.cmd.angles[PITCH] = aiEnt->client->pers.cmd.angles[ROLL] = aiEnt->client->pers.cmd.angles[YAW] = 0;

	//aiEnt->client->ps.viewangles[YAW] = g_testvalue0.value;
	//aiEnt->client->ps.delta_angles[YAW] = ANGLE2SHORT(g_testvalue1.value);
	//aiEnt->s.apos.trBase[YAW] = g_testvalue2.value;
	//aiEnt->s.apos.trDelta[YAW] = g_testvalue3.value;

	//aiEnt->s.angles[YAW] = bg_testvalue0.value;
	//aiEnt->s.angles2[YAW] = bg_testvalue1.value;
	//aiEnt->r.currentAngles[YAW] = bg_testvalue2.value;

	//aiEnt->client->ps.delta_angles[YAW] = ANGLE2SHORT(g_testvalue1.value);
	//aiEnt->client->pers.cmd.angles[YAW] = ANGLE2SHORT(g_testvalue2.value);

	vec3_t nAngles;

	nAngles[YAW] = AngleNormalize360(angles[YAW]);
	nAngles[PITCH] = AngleNormalize360(angles[PITCH]);
	nAngles[ROLL] = AngleNormalize360(angles[ROLL]);

	aiEnt->client->ps.viewangles[YAW] = nAngles[YAW];
	aiEnt->client->ps.viewangles[PITCH] = nAngles[PITCH];

	//aiEnt->client->ps.delta_angles[YAW] = ANGLE2SHORT(aiEnt->client->ps.viewangles[YAW]) + aiEnt->client->pers.cmd.angles[YAW];
	aiEnt->client->ps.delta_angles[YAW] = 0;// ANGLE2SHORT((aiEnt->client->ps.viewangles[YAW] - 180.0) * 0.5);

	/*if (g_testvalue0.integer)
	{
		aiEnt->client->ps.delta_angles[YAW] = (nAngles[YAW]);
	}
	else
	{
		aiEnt->client->ps.delta_angles[YAW] = ANGLE2SHORT(nAngles[YAW]);
	}*/

	/*
	if (g_testvalue0.integer)
	{
		aiEnt->client->ps.delta_angles[YAW] = (nAngles[YAW]);
		aiEnt->client->pers.cmd.angles[YAW] = (-nAngles[YAW]);

		if (doPitch)
		{
			aiEnt->client->ps.delta_angles[PITCH] = (nAngles[PITCH]);
			aiEnt->client->pers.cmd.angles[PITCH] = (-nAngles[PITCH]);
		}
	}
	else
	{
		aiEnt->client->ps.delta_angles[YAW] = (-nAngles[YAW]);
		aiEnt->client->pers.cmd.angles[YAW] = (nAngles[YAW]);

		if (doPitch)
		{
			aiEnt->client->ps.delta_angles[PITCH] = (-nAngles[PITCH]);
			aiEnt->client->pers.cmd.angles[PITCH] = (nAngles[PITCH]);
		}
	}
	*/

	aiEnt->NPC->desiredYaw = aiEnt->NPC->lockedDesiredYaw = nAngles[YAW];
	aiEnt->NPC->desiredPitch = aiEnt->NPC->lockedDesiredPitch = nAngles[PITCH];

	NPC_UpdateAngles(aiEnt, qtrue, qtrue);

	if (g_testvalue0.integer)
		aiEnt->client->pers.cmd.angles[YAW] = 360.0-aiEnt->client->pers.cmd.angles[YAW];
	else
		aiEnt->client->pers.cmd.angles[YAW] = (180.0 - aiEnt->client->pers.cmd.angles[YAW]) * 0.5;

	trap->Print("SET: delta_angles %f. cmd_angles %f. viewangles %f. trDelta %f. trBase %f.\n", SHORT2ANGLE(aiEnt->client->ps.delta_angles[YAW]), SHORT2ANGLE(aiEnt->client->pers.cmd.angles[YAW]), aiEnt->client->ps.viewangles[YAW], aiEnt->s.apos.trDelta[YAW], aiEnt->s.apos.trBase[YAW]);
#else
	//Get the positions
	CalcEntitySpot(aiEnt, SPOT_HEAD_LEAN, muzzle);//SPOT_HEAD

												  //Find the desired angles
												  //GetAnglesForDirection(muzzle, position, angles);
	vec3_t dir;
	VectorSubtract(position, muzzle, dir);
	vectoangles(dir, angles);

	if (!doPitch)
	{
		angles[ROLL] = 0;
		angles[PITCH] = 0;
	}
	else
	{
		angles[ROLL] = 0;
	}

	if (!aiEnt->enemy || !aiEnt->enemy->inuse || (aiEnt->enemy->NPC && aiEnt->enemy->health <= 0))
	{//FIXME: should still keep shooting for a second or two after they actually die...
#ifndef __NO_ICARUS__
		trap->ICARUS_TaskIDComplete((sharedEntity_t *)aiEnt, TID_BSTATE);
#endif //__NO_ICARUS__
		goto finished;
		return qfalse;
	}

	//trap->Print("BEGIN: delta_angles %f. viewangles %f. trDelta %f. trBase %f.\n", SHORT2ANGLE(aiEnt->client->ps.delta_angles[YAW]), aiEnt->client->ps.viewangles[YAW], aiEnt->s.apos.trDelta[YAW], aiEnt->s.apos.trBase[YAW]);

	//if (g_testvalue0.integer)
	//	aiEnt->client->ps.delta_angles[YAW] = 0;
	//else
	//	aiEnt->client->ps.delta_angles[YAW] = ANGLE2SHORT(AngleNormalize180(angles[YAW]));

	aiEnt->NPC->desiredPitch = aiEnt->NPC->lockedDesiredPitch = AngleNormalize360(angles[PITCH]);
	aiEnt->NPC->desiredYaw = aiEnt->NPC->lockedDesiredYaw = AngleNormalize360(angles[YAW]);

	if (ATST_UpdateAngles(aiEnt, qtrue, qtrue))
	{//FIXME: if angles clamped, this may never work!
#ifndef __NO_ICARUS__
		trap->ICARUS_TaskIDComplete((sharedEntity_t *)aiEnt, TID_BSTATE);
#endif //__NO_ICARUS__
		goto finished;
	}

	//trap->Print("END2: delta_angles %f. viewangles %f. trDelta %f. trBase %f.\n", SHORT2ANGLE(aiEnt->client->ps.delta_angles[YAW]), aiEnt->client->ps.viewangles[YAW], aiEnt->s.apos.trDelta[YAW], aiEnt->s.apos.trBase[YAW]);
	return qfalse;

finished:
	//trap->Print("END1: delta_angles %f. viewangles %f. trDelta %f. trBase %f.\n", SHORT2ANGLE(aiEnt->client->ps.delta_angles[YAW]), aiEnt->client->ps.viewangles[YAW], aiEnt->s.apos.trDelta[YAW], aiEnt->s.apos.trBase[YAW]);

	aiEnt->NPC->desiredYaw = aiEnt->client->ps.viewangles[YAW];
	aiEnt->NPC->desiredPitch = aiEnt->client->ps.viewangles[PITCH];

	aiEnt->NPC->aimTime = 0;//ok to turn normally now
#endif

	return facing;
}

/*
-------------------------
ATST_FaceEntity
-------------------------
*/

qboolean ATST_FaceEntity(gentity_t *aiEnt, gentity_t *ent, qboolean doPitch)
{
	vec3_t		entPos;

	//Get the positions
	CalcEntitySpot(ent, SPOT_HEAD_LEAN, entPos);

	return ATST_FacePosition(aiEnt, entPos, doPitch);
}

/*
-------------------------
ATST_FaceEnemy
-------------------------
*/
qboolean ATST_FaceEnemy(gentity_t *aiEnt, qboolean doPitch)
{
	if (aiEnt == NULL)
		return qfalse;

	if (aiEnt->enemy == NULL)
		return qfalse;

	return ATST_FaceEntity(aiEnt, aiEnt->enemy, doPitch);
}

/*
-------------------------
ATST_Hunt
-------------------------`
*/
void ATST_Hunt(gentity_t *aiEnt)
{
	NPC_MoveIntoOptimalAttackPosition(aiEnt);
}

/*
-------------------------
ATST_Ranged
-------------------------
*/
void ATST_Ranged(gentity_t *aiEnt, qboolean altAttack)
{
	if (altAttack)
	{
		if (TIMER_Done(aiEnt, "atkAltDelay"))
		{
			aiEnt->s.weapon = aiEnt->client->ps.weapon = WP_MODULIZED_WEAPON;// WP_TURRET;// WP_MODULIZED_WEAPON;
			TIMER_Set(aiEnt, "atkAltDelay", irand(0,8 == 0) ? 3000 : 500);
			FireWeapon(aiEnt, qtrue);
		}
	}
	else
	{
		if (TIMER_Done(aiEnt, "atkDelay"))
		{
			aiEnt->s.weapon = aiEnt->client->ps.weapon = WP_MODULIZED_WEAPON;// WP_TURRET;// WP_MODULIZED_WEAPON;
			TIMER_Set(aiEnt, "atkDelay", irand(0, 8 == 0) ? 3000 : 500);
			FireWeapon(aiEnt, qfalse);
		}
	}
}

/*
-------------------------
ATST_Attack
-------------------------
*/
void ATST_Attack(gentity_t *aiEnt)
{
	if (NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse)
	{
		aiEnt->enemy = NULL;
		ATST_Patrol(aiEnt);
		return;
	}

	// Rate our distance to the target, and our visibilty
	//visibility_t visible = NPC_CheckVisibility(aiEnt, aiEnt->enemy, /*CHECK_VISRANGE | CHECK_360 | CHECK_FOV |*/ CHECK_SHOOT);
	visibility_t visible = NPC_CheckVisibility(aiEnt, aiEnt->enemy, CHECK_360);

	//if (visible == VIS_SHOOT)
	if (visible == VIS_360)
	{
		//NPC_SetLookTarget(aiEnt, aiEnt->enemy->s.number, 5000);
		
		if (ATST_FaceEnemy(aiEnt, qtrue))
		{
			ATST_Ranged(aiEnt, qfalse);
		}
	}
	else
	{// If we cannot see our target, move to see it
		//NPC_ClearLookTarget(aiEnt);
		ATST_Hunt(aiEnt);
	}
}

/*
-------------------------
ATST_Patrol
-------------------------
*/
void ATST_Patrol(gentity_t *aiEnt)
{
	if (!NPC_HaveValidEnemy(aiEnt))
	{
		aiEnt->enemy = NULL;
		NPC_PatrolArea(aiEnt);
	}
	else
	{
		ATST_Attack(aiEnt);
	}
}

/*
-------------------------
NPC_BSDroid_Default
-------------------------
*/
void NPC_BSATST_Default(gentity_t *aiEnt)
{
	switch (aiEnt->NPC->behaviorState)
	{
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
	default:
		ATST_Patrol(aiEnt);
		break;
	}

	// ATST's never jump!
	aiEnt->client->pers.cmd.upmove = 0;

	//gentity_t *G_ScreenShake(vec3_t org, gentity_t *target, float intensity, int duration, qboolean global)
}
