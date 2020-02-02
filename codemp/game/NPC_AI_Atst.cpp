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
