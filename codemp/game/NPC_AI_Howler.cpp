#include "b_local.h"

//
// UQ1: This is now a generic quadruped animal AI...
//

extern void Jedi_Advance(gentity_t *aiEnt);
extern void Jedi_Retreat(gentity_t *aiEnt);

extern int NPC_GetEntsNearBolt(gentity_t *aiEnt, int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg);
extern void G_Knockdown(gentity_t *victim);

/*
-------------------------
NPC_Howler_Precache
-------------------------
*/
void NPC_Howler_Precache( void )
{
}

void Howler_SetBolts(gentity_t *aiEnt)
{
	if (aiEnt && aiEnt->client)
	{
		renderInfo_t *ri = &aiEnt->client->renderInfo;

		if (!ri->handRBolt)
		{
			ri->handRBolt = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "*r_hand");
			ri->handLBolt = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "*l_hand");
			ri->headBolt = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "*head_eyes");
			ri->torsoBolt = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "jaw_bone");
		}
	}
}

/*
-------------------------
Howler_Idle
-------------------------
*/
void Howler_Idle(gentity_t *aiEnt) {
	TIMER_Remove(aiEnt, "attacking");
	NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	Howler_SetBolts(aiEnt);
}

void Howler_Swipe(gentity_t *aiEnt)
{
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue);

	int			radiusEntNums[128];
	int			numEnts;
	int			i;
	vec3_t		boltOrg;

	float		attackRange = 128;
	int			damage = 30;
	int			throwDist = 10;
	int			anim = BOTH_ATTACK1;

	switch (aiEnt->client->NPC_class)
	{
	case CLASS_REEK:
		attackRange = 96;
		damage = irand(40, 70);
		throwDist = 10;
		anim = BOTH_VT_BUCK;// BOTH_VT_ATB;
		break;
	case CLASS_NEXU:
		attackRange = 96;
		damage = irand(40, 70);
		throwDist = 10;
		anim = BOTH_ATTACK1 + irand(0, 8);
		break;
	case CLASS_ACKLAY:
		attackRange = 128;
		damage = irand(40, 90);
		throwDist = 20;
		anim = BOTH_ATTACK1 + irand(0, 2);
		break;
	case CLASS_HOWLER:
	default:
		attackRange = 72;
		damage = irand(20, 30);
		throwDist = 5;
		break;
	}

	NPC_SetAnim(aiEnt, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	numEnts = NPC_GetEntsNearBolt(aiEnt, radiusEntNums, attackRange, aiEnt->client->renderInfo.handRBolt, boltOrg);

	for (i = 0; i < numEnts; i++)
	{// FIXME: Find correct anims for this...
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];

		if (!radiusEnt->inuse)
		{
			continue;
		}

		if (radiusEnt == aiEnt)
		{//Skip the rancor ent
			continue;
		}

		if (radiusEnt->client == NULL)
		{//must be a client
			continue;
		}

		if ((radiusEnt->client->ps.eFlags2 & EF2_HELD_BY_MONSTER))
		{//can't be one already being held
			continue;
		}

		if (!ValidEnemy(aiEnt, radiusEnt))
		{
			continue;
		}

		if (radiusEnt->client->NPC_class != aiEnt->client->NPC_class)
		{
			vec3_t pushDir;
			vec3_t angs;

			G_Sound(radiusEnt, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/swipehit.wav"));

			//actually push the enemy
			VectorCopy(aiEnt->client->ps.viewangles, angs);
			angs[YAW] += flrand(25, 50);
			angs[PITCH] = flrand(-25, -15);
			AngleVectors(angs, pushDir, NULL, NULL);

			G_Damage(radiusEnt, aiEnt, aiEnt, vec3_origin, radiusEnt->r.currentOrigin, damage, DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE);

			G_Throw(radiusEnt, pushDir, throwDist);

			if (radiusEnt->health > 0 && throwDist > 5 && damage >= 40)
			{//do pain on enemy
				G_Knockdown(radiusEnt);
			}
		}
	}
}

void Howler_Slam(gentity_t *aiEnt)
{
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue);

	int			radiusEntNums[128];
	int			numEnts;
	int			i;
	vec3_t		boltOrg;

	float		attackRange = 128;
	int			damage = 30;
	int			throwDist = 10;
	int			anim = BOTH_ATTACK1;

	switch (aiEnt->client->NPC_class)
	{// FIXME: Find correct anims for this...
	case CLASS_REEK:
		attackRange = 96;
		damage = irand(30, 50);
		throwDist = 5;
		anim = BOTH_VT_BUCK;// BOTH_VT_ATB;
		break;
	case CLASS_NEXU:
		attackRange = 96;
		damage = irand(30, 50);
		throwDist = 5;
		anim = BOTH_ATTACK1 + irand(0, 8);
		break;
	case CLASS_ACKLAY:
		attackRange = 128;
		damage = irand(20, 50);
		throwDist = 10;
		anim = BOTH_ATTACK1 + irand(0, 2);
		break;
	case CLASS_HOWLER:
	default:
		attackRange = 64;
		damage = irand(10, 15);
		throwDist = 3;
		anim = BOTH_ATTACK1;
		break;
	}

	NPC_SetAnim(aiEnt, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	numEnts = NPC_GetEntsNearBolt(aiEnt, radiusEntNums, attackRange, aiEnt->client->renderInfo.handRBolt, boltOrg);

	for (i = 0; i < numEnts; i++)
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];

		if (!radiusEnt->inuse)
		{
			continue;
		}

		if (radiusEnt == aiEnt)
		{//Skip the rancor ent
			continue;
		}

		if (radiusEnt->client == NULL)
		{//must be a client
			continue;
		}

		if ((radiusEnt->client->ps.eFlags2 & EF2_HELD_BY_MONSTER))
		{//can't be one already being held
			continue;
		}

		if (!ValidEnemy(aiEnt, radiusEnt))
		{
			continue;
		}

		if (radiusEnt->client->NPC_class != aiEnt->client->NPC_class)
		{
			G_Sound(radiusEnt, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/swipehit.wav"));

			G_Damage(radiusEnt, aiEnt, aiEnt, vec3_origin, radiusEnt->r.currentOrigin, damage, DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE);

			if (radiusEnt->health > 0 && throwDist > 3 && damage >= 30)
			{//do pain on enemy
				G_Knockdown(radiusEnt);
			}
		}
	}
}

void Howler_Charge(gentity_t *aiEnt)
{
	NPC_FaceEnemy(aiEnt, qtrue);

	vec3_t	fwd, yawAng;
	
	float distance = DistanceHorizontal(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin);

	VectorSet(yawAng, 0, aiEnt->client->ps.viewangles[YAW], 0);
	AngleVectors(yawAng, fwd, NULL, NULL);
	VectorScale(fwd, distance*1.5f, aiEnt->client->ps.velocity);
	aiEnt->client->ps.velocity[2] = distance - 128;// 150;
	aiEnt->client->ps.groundEntityNum = ENTITYNUM_NONE;

	NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_MELEE2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
}

//---------------------------------------------------------
void Howler_TryDamage(gentity_t *aiEnt, gentity_t *enemy, int damage )
{
	vec3_t	dir;
	AngleVectors(aiEnt->r.currentAngles, dir, NULL, NULL);
	G_Damage(enemy, aiEnt, aiEnt, dir, enemy->r.currentOrigin, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE);
}

//------------------------------
void Howler_Attack(gentity_t *aiEnt)
{
	NPC_FaceEnemy(aiEnt, qtrue);

	if ( !TIMER_Exists( aiEnt, "attacking" ) || TIMER_Done(aiEnt, "attacking"))
	{
		// Going to do ATTACK1
		TIMER_Set( aiEnt, "attacking", 1700 + irand(0, 200) );

		int attackChoice = irand(0, 12);

		if (aiEnt->client->NPC_class == CLASS_HOWLER)
		{// Howlers are too small to do the big moves...
			attackChoice = 12;
		}

		if (attackChoice <= 1)
		{
			Howler_Swipe(aiEnt);
		}
		else if (attackChoice <= 5)
		{
			Howler_Slam(aiEnt);
		}
		else
		{
			int damage = 20;
			int anim = BOTH_ATTACK1;

			switch (aiEnt->client->NPC_class)
			{
			case CLASS_REEK:
				damage = 50;
				anim = BOTH_VT_BUCK;// BOTH_VT_ATB;
				break;
			case CLASS_NEXU:
				damage = 60;
				anim = BOTH_ATTACK1 + irand(0, 8);
				break;
			case CLASS_ACKLAY:
				damage = 80;
				anim = BOTH_ATTACK1 + irand(0, 2);
				break;
			case CLASS_HOWLER:
			default:
				damage = 20;
				anim = BOTH_ATTACK1;
				break;
			}

			NPC_SetAnim(aiEnt, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			Howler_TryDamage(aiEnt, aiEnt->enemy, damage);
		}
	}
}

//----------------------------------
void Howler_Combat(gentity_t *aiEnt)
{
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue );

	float attackRange;

	switch (aiEnt->client->NPC_class)
	{
	case CLASS_REEK:
	case CLASS_NEXU:
		attackRange = 172;
		break;
	case CLASS_ACKLAY:
		attackRange = 256;
		break;
	case CLASS_HOWLER:
	default:
		attackRange = 96;
		break;
	}

	float distance	= Distance( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );

	if (TIMER_Done2(aiEnt, "takingPain", qtrue))
	{
		TIMER_Remove(aiEnt, "attacking");
	}
	else if (distance > attackRange)
	{
		TIMER_Remove(aiEnt, "attacking");

		//if (distance < attackRange * 1.5)
		//	Howler_Charge(aiEnt);
		//else
			Jedi_Advance(aiEnt);
	}
	else
	{
		Howler_Attack(aiEnt);
	}
}

/*
-------------------------
NPC_Howler_Pain
-------------------------
*/
void NPC_Howler_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( damage >= 50 )
	{
		TIMER_Remove( self, "attacking" );
		TIMER_Set( self, "takingPain", 2900 );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
	}
}


/*
-------------------------
NPC_BSHowler_Default
-------------------------
*/
void NPC_BSHowler_Default(gentity_t *aiEnt)
{
	if (aiEnt->enemy && ValidEnemy(aiEnt, aiEnt->enemy))
	{
		Howler_Combat(aiEnt);
		return;
	}
	else
	{
		aiEnt->enemy = NULL;
		Howler_Idle(aiEnt);
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}
