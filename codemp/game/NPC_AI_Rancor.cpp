#include "b_local.h"

extern void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex );

// These define the working combat range for these suckers
#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		1024
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

void Rancor_SetBolts( gentity_t *self )
{
	if ( self && self->client )
	{
		renderInfo_t *ri = &self->client->renderInfo;
		ri->handRBolt = trap->G2API_AddBolt( self->ghoul2, 0, "*r_hand" );
		ri->handLBolt = trap->G2API_AddBolt( self->ghoul2, 0, "*l_hand" );
		ri->headBolt = trap->G2API_AddBolt( self->ghoul2, 0, "*head_eyes" );
		ri->torsoBolt = trap->G2API_AddBolt( self->ghoul2, 0, "jaw_bone" );
	}
}

/*
-------------------------
NPC_Rancor_Precache
-------------------------
*/
void NPC_Rancor_Precache( void )
{
	int i;
	for ( i = 1; i < 3; i ++ )
	{
		G_SoundIndex( va("sound/chars/rancor/snort_%d.wav", i) );
	}
	G_SoundIndex( "sound/chars/rancor/swipehit.wav" );
	G_SoundIndex( "sound/chars/rancor/chomp.wav" );
}


/*
-------------------------
Rancor_Idle
-------------------------
*/
void Rancor_Idle(gentity_t *aiEnt)
{
	aiEnt->NPC->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal(aiEnt) )
	{
		aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal(aiEnt, qtrue );
	}
}


qboolean Rancor_CheckRoar( gentity_t *self )
{
	if ( !self->wait )
	{//haven't ever gotten mad yet
		self->wait = 1;//do this only once
		self->client->ps.eFlags2 |= EF2_ALERTED;
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
		TIMER_Set( self, "rageTime", self->client->ps.legsTimer );
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Rancor_Patrol
-------------------------
*/
void Rancor_Patrol(gentity_t *aiEnt)
{
	aiEnt->NPC->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal(aiEnt) )
	{
		aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal(aiEnt, qtrue );
	}
	else
	{
		if ( TIMER_Done( aiEnt, "patrolTime" ))
		{
			TIMER_Set( aiEnt, "patrolTime", crandom() * 5000 + 5000 );
		}
	}

	if ( NPC_CheckEnemyExt(aiEnt, qtrue ) == qfalse )
	{
		Rancor_Idle(aiEnt);
		return;
	}
	Rancor_CheckRoar( aiEnt );
	TIMER_Set( aiEnt, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
}

/*
-------------------------
Rancor_Move
-------------------------
*/
void Rancor_Move(gentity_t *aiEnt, qboolean visible )
{
	if ( aiEnt->NPC->localState != LSTATE_WAITING )
	{
		aiEnt->NPC->goalEntity = aiEnt->enemy;
		if ( !NPC_MoveToGoal(aiEnt, qtrue ) )
		{
			aiEnt->NPC->consecutiveBlockedMoves++;
		}
		else
		{
			aiEnt->NPC->consecutiveBlockedMoves = 0;
		}
		aiEnt->NPC->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
extern void G_Knockdown( gentity_t *victim );
extern void G_Dismember( gentity_t *ent, gentity_t *enemy, vec3_t point, int limbType, float limbRollBase, float limbPitchBase, int deathAnim, qboolean postDeath );
extern float NPC_EntRangeFromBolt(gentity_t *aiEnt, gentity_t *targEnt, int boltIndex );
extern int NPC_GetEntsNearBolt( gentity_t *aiEnt, int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg );

void Rancor_DropVictim( gentity_t *self )
{
	//FIXME: if Rancor dies, it should drop its victim.
	//FIXME: if Rancor is removed, it must remove its victim.
	if ( self->activator )
	{
		if ( self->activator->client )
		{
			self->activator->client->ps.eFlags2 &= ~EF2_HELD_BY_MONSTER;
			self->activator->client->ps.hasLookTarget = qfalse;
			self->activator->client->ps.lookTarget = ENTITYNUM_NONE;
			self->activator->client->ps.viewangles[ROLL] = 0;
			SetClientViewAngle( self->activator, self->activator->client->ps.viewangles );
			self->activator->r.currentAngles[PITCH] = self->activator->r.currentAngles[ROLL] = 0;
			G_SetAngles( self->activator, self->activator->r.currentAngles );
		}
		if ( self->activator->health <= 0 )
		{
			//if ( self->activator->s.number )
			{//never free player
				if ( self->count == 1 )
				{//in my hand, just drop them
					if ( self->activator->client )
					{
						self->activator->client->ps.legsTimer = self->activator->client->ps.torsoTimer = 0;
						//FIXME: ragdoll?
					}
				}
				else
				{
					if ( self->activator->client )
					{
						self->activator->client->ps.eFlags |= EF_NODRAW;//so his corpse doesn't drop out of me...
					}
					//G_FreeEntity( self->activator );
				}
			}
		}
		else
		{
			if ( self->activator->NPC )
			{//start thinking again
				self->activator->NPC->nextBStateThink = level.time;
			}
			//clear their anim and let them fall
			self->activator->client->ps.legsTimer = self->activator->client->ps.torsoTimer = 0;
		}
		if ( self->enemy == self->activator )
		{
			self->enemy = NULL;
		}
		self->activator = NULL;
	}
	self->count = 0;//drop him
}

void Rancor_Swing(gentity_t *aiEnt, qboolean tryGrab )
{
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue);

	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 128;// 88;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;

	numEnts = NPC_GetEntsNearBolt(aiEnt, radiusEntNums, radius, aiEnt->client->renderInfo.handRBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}

		if ( radiusEnt == aiEnt )
		{//Skip the rancor ent
			continue;
		}

		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one already being held
			continue;
		}

		if ( DistanceSquared( radiusEnt->r.currentOrigin, boltOrg ) <= radiusSquared )
		{
			if ( tryGrab
				&& aiEnt->count != 1 //don't have one in hand or in mouth already - FIXME: allow one in hand and any number in mouth!
				&& radiusEnt->client->NPC_class != CLASS_RANCOR
				&& radiusEnt->client->NPC_class != CLASS_GALAKMECH
				&& radiusEnt->client->NPC_class != CLASS_ATST
				&& radiusEnt->client->NPC_class != CLASS_GONK
				&& radiusEnt->client->NPC_class != CLASS_R2D2
				&& radiusEnt->client->NPC_class != CLASS_R5D2
				&& radiusEnt->client->NPC_class != CLASS_MARK1
				&& radiusEnt->client->NPC_class != CLASS_MARK2
				&& radiusEnt->client->NPC_class != CLASS_MOUSE
				&& radiusEnt->client->NPC_class != CLASS_PROBE
				&& radiusEnt->client->NPC_class != CLASS_SEEKER
				&& radiusEnt->client->NPC_class != CLASS_REMOTE
				&& radiusEnt->client->NPC_class != CLASS_SENTRY
				&& radiusEnt->client->NPC_class != CLASS_INTERROGATOR
				&& radiusEnt->client->NPC_class != CLASS_VEHICLE
				&& radiusEnt->client->NPC_class != CLASS_REEK
				&& radiusEnt->client->NPC_class != CLASS_ACKLAY)
			{//grab
				if ( aiEnt->count == 2 )
				{//have one in my mouth, remove him
					TIMER_Remove( aiEnt, "clearGrabbed" );
					Rancor_DropVictim( aiEnt );
				}
				aiEnt->enemy = radiusEnt;//make him my new best friend
				radiusEnt->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;
				//FIXME: this makes it so that the victim can't hit us with shots!  Just use activator or something
				radiusEnt->client->ps.hasLookTarget = qtrue;
				radiusEnt->client->ps.lookTarget = aiEnt->s.number;
				aiEnt->activator = radiusEnt;//remember him
				aiEnt->count = 1;//in my hand
				//wait to attack
				TIMER_Set( aiEnt, "attacking", aiEnt->client->ps.legsTimer + Q_irand(500, 2500) );
				if ( radiusEnt->health > 0 && radiusEnt->pain )
				{//do pain on enemy
					radiusEnt->pain( radiusEnt, aiEnt, 100 );
					//GEntity_PainFunc( radiusEnt, NPC, NPC, radiusEnt->r.currentOrigin, 0, MOD_CRUSH );
				}
				else if ( radiusEnt->client )
				{
					radiusEnt->client->ps.forceHandExtend = HANDEXTEND_NONE;
					radiusEnt->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				}
			}
			else
			{//smack
				vec3_t pushDir;
				vec3_t angs;

				G_Sound( radiusEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
				//actually push the enemy
				/*
				//VectorSubtract( radiusEnt->r.currentOrigin, boltOrg, pushDir );
				VectorSubtract( radiusEnt->r.currentOrigin, NPC->r.currentOrigin, pushDir );
				pushDir[2] = Q_flrand( 100, 200 );
				VectorNormalize( pushDir );
				*/
				VectorCopy( aiEnt->client->ps.viewangles, angs );
				angs[YAW] += flrand( 25, 50 );
				angs[PITCH] = flrand( -25, -15 );
				AngleVectors( angs, pushDir, NULL, NULL );
				if ( radiusEnt->client->NPC_class != aiEnt->client->NPC_class)
				{
					G_Damage( radiusEnt, aiEnt, aiEnt, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 35, 90 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
					
					if (aiEnt->client->NPC_class != CLASS_RANCOR)
						G_Throw( radiusEnt, pushDir, 50 );
					else
						G_Throw(radiusEnt, pushDir, 250);

					if ( radiusEnt->health > 0 )
					{//do pain on enemy
						G_Knockdown( radiusEnt );//, NPC, pushDir, 100, qtrue );
					}
				}
			}
		}
	}
}

void Rancor_Smash(gentity_t *aiEnt)
{
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue);

	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 128;
	const float	halfRadSquared = ((radius/2)*(radius/2));
	const float	radiusSquared = (radius*radius);
	float		distSq;
	int			i;
	vec3_t		boltOrg;

	AddSoundEvent( aiEnt, aiEnt->r.currentOrigin, 512, AEL_DANGER, qfalse );//, qtrue );

	numEnts = NPC_GetEntsNearBolt(aiEnt, radiusEntNums, radius, aiEnt->client->renderInfo.handLBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}

		if ( radiusEnt == aiEnt )
		{//Skip the rancor ent
			continue;
		}

		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one being held
			continue;
		}

		distSq = DistanceSquared( radiusEnt->r.currentOrigin, boltOrg );
		if ( distSq <= radiusSquared )
		{
			G_Sound( radiusEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
			if ( distSq < halfRadSquared )
			{//close enough to do damage, too
				G_Damage( radiusEnt, aiEnt, aiEnt, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 30, 85 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			}
			if ( radiusEnt->health > 0
				&& radiusEnt->client
				&& radiusEnt->client->NPC_class != CLASS_RANCOR
				&& radiusEnt->client->NPC_class != CLASS_ATST
				&& radiusEnt->client->NPC_class != CLASS_REEK
				&& radiusEnt->client->NPC_class != CLASS_ACKLAY)
			{
				if ( distSq < halfRadSquared
					|| radiusEnt->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//within range of my fist or withing ground-shaking range and not in the air
					G_Knockdown( radiusEnt );//, NPC, vec3_origin, 100, qtrue );
				}
			}
		}
	}
}

void Rancor_Bite(gentity_t *aiEnt)
{
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue);

	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 100;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;

	numEnts = NPC_GetEntsNearBolt(aiEnt, radiusEntNums, radius, aiEnt->client->renderInfo.crotchBolt, boltOrg );//was gutBolt?

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}

		if ( radiusEnt == aiEnt )
		{//Skip the rancor ent
			continue;
		}

		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//can't be one already being held
			continue;
		}

		if ( DistanceSquared( radiusEnt->r.currentOrigin, boltOrg ) <= radiusSquared )
		{
			G_Damage( radiusEnt, aiEnt, aiEnt, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 35, 80 ), DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			if ( radiusEnt->health <= 0 && radiusEnt->client )
			{//killed them, chance of dismembering
				if ( !Q_irand( 0, 1 ) )
				{//bite something off
					int hitLoc = Q_irand( G2_MODELPART_HEAD, G2_MODELPART_RLEG );
					if ( hitLoc == G2_MODELPART_HEAD )
					{
						NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_DEATH17, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					else if ( hitLoc == G2_MODELPART_WAIST )
					{
						NPC_SetAnim( radiusEnt, SETANIM_BOTH, BOTH_DEATHBACKWARD2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
					}
					//radiusEnt->client->dismembered = qfalse;
					//FIXME: the limb should just disappear, cuz I ate it
					G_Dismember( radiusEnt, aiEnt, radiusEnt->r.currentOrigin, hitLoc, 90, 0, radiusEnt->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( radiusEnt, radiusEnt->r.currentOrigin, MOD_SABER, 1000, hitLoc, qtrue );
					G_Damage(radiusEnt, aiEnt, aiEnt, radiusEnt->r.currentAngles, radiusEnt->r.currentOrigin, 100000, DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE); // kill them - angles dont matter
				}
			}
			G_Sound( radiusEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/rancor/chomp.wav" ) );
		}
	}
}
//------------------------------
extern void TossClientItems( gentity_t *self );
void Rancor_Attack(gentity_t *aiEnt, float distance, qboolean doCharge )
{
	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue);

	if (!TIMER_Exists(aiEnt, "attacking"))
	{
		if (aiEnt->count == 2 && aiEnt->activator)
		{
			if (!ValidEnemy(aiEnt, aiEnt->activator) || !ValidEnemy(aiEnt, aiEnt->enemy))
			{// Find a new enemy...
				aiEnt->count = 0;
				aiEnt->activator = NULL;
				aiEnt->enemy = NULL;
				return;
			}
			else
			{
				return;
			}
		}

		if (aiEnt->count == 1 && aiEnt->activator)
		{//holding enemy
			if (aiEnt->activator->health > 0 && Q_irand(0, 1))
			{//quick bite
				NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(aiEnt, "attack_dmg", 450);
			}
			else
			{//full eat
				NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_ATTACK3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(aiEnt, "attack_dmg", 900);
				//Make victim scream in fright
				if (aiEnt->activator->health > 0 && aiEnt->activator->client)
				{
					G_AddEvent(aiEnt->activator, Q_irand(EV_DEATH1, EV_DEATH3), 0);
					NPC_SetAnim(aiEnt->activator, SETANIM_TORSO, BOTH_FALLDEATH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					if (aiEnt->activator->NPC)
					{//no more thinking for you
						TossClientItems(aiEnt);
						aiEnt->activator->NPC->nextBStateThink = Q3_INFINITE;
					}
				}
			}
		}
		/*else if (aiEnt->enemy->health > 0 && doCharge)
		{//charge
			vec3_t	fwd, yawAng;
			VectorSet(yawAng, 0, aiEnt->client->ps.viewangles[YAW], 0);
			AngleVectors(yawAng, fwd, NULL, NULL);
			VectorScale(fwd, distance*1.5f, aiEnt->client->ps.velocity);
			aiEnt->client->ps.velocity[2] = distance - 96;// 150;
			aiEnt->client->ps.groundEntityNum = ENTITYNUM_NONE;

			NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_MELEE2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(aiEnt, "attack_dmg", 1250);
		}*/
		else if (Q_irand(0, 5))
		{//smash
			NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_MELEE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(aiEnt, "attack_dmg", 1000);
		}
		else
		{//try to grab
			NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(aiEnt, "attack_dmg", 1000);
		}

		TIMER_Set(aiEnt, "attacking", aiEnt->client->ps.legsTimer + random() * 200);
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks

	if (TIMER_Done2(aiEnt, "attack_dmg", qtrue))
	{
		vec3_t shakePos;
		switch (aiEnt->client->ps.legsAnim)
		{
		case BOTH_MELEE1:
			Rancor_Smash(aiEnt);
			G_GetBoltPosition(aiEnt, aiEnt->client->renderInfo.handLBolt, shakePos, 0);
			G_ScreenShake(shakePos, NULL, 4.0f, 1000, qfalse);
			//CGCam_Shake( 1.0f*playerDist/128.0f, 1000 );
			break;
		case BOTH_MELEE2:
			Rancor_Bite(aiEnt);
			TIMER_Set(aiEnt, "attack_dmg2", 450);
			break;
		case BOTH_ATTACK1:
			if (aiEnt->count == 1 && aiEnt->activator)
			{
				G_Damage(aiEnt->activator, aiEnt, aiEnt, vec3_origin, aiEnt->activator->r.currentOrigin, Q_irand(55, 140), DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE);
				if (aiEnt->activator->health <= 0)
				{//killed him
				 //make it look like we bit his head off
				 //NPC->activator->client->dismembered = qfalse;
					G_Dismember(aiEnt->activator, aiEnt, aiEnt->activator->r.currentOrigin, G2_MODELPART_HEAD, 90, 0, aiEnt->activator->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( NPC->activator, NPC->activator->r.currentOrigin, MOD_SABER, 1000, HL_HEAD, qtrue );
					aiEnt->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					aiEnt->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim(aiEnt->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_Damage(aiEnt->activator, aiEnt, aiEnt, aiEnt->activator->r.currentAngles, aiEnt->activator->r.currentOrigin, 100000, DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE); // kill them - angles dont matter
				}
				G_Sound(aiEnt->activator, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/chomp.wav"));
			}
			break;
		case BOTH_ATTACK2:
			//try to grab
			Rancor_Swing(aiEnt, qtrue);
			break;
		case BOTH_ATTACK3:
			if (aiEnt->count == 1 && aiEnt->activator)
			{
				//cut in half
				if (aiEnt->activator->client)
				{
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember(aiEnt->activator, aiEnt, aiEnt->activator->r.currentOrigin, G2_MODELPART_WAIST, 90, 0, aiEnt->activator->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( NPC->activator, NPC->enemy->r.currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
				}
				//KILL
				G_Damage(aiEnt->activator, aiEnt, aiEnt, vec3_origin, aiEnt->activator->r.currentOrigin, 100000, DAMAGE_NO_PROTECTION | DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC, MOD_MELEE);//, HL_NONE );//
				if (aiEnt->activator->client)
				{
					aiEnt->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					aiEnt->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim(aiEnt->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				TIMER_Set(aiEnt, "attack_dmg2", 1350);
				G_Sound(aiEnt->activator, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/swipehit.wav"));
				G_AddEvent(aiEnt->activator, EV_JUMP, aiEnt->activator->health);
			}
			break;
		}
	}
	else if (TIMER_Done2(aiEnt, "attack_dmg2", qtrue)
		&& (aiEnt->client->ps.legsAnim == BOTH_MELEE2 || aiEnt->client->ps.legsAnim == BOTH_ATTACK3))
	{
		switch (aiEnt->client->ps.legsAnim)
		{
		case BOTH_MELEE1:
			break;
		case BOTH_MELEE2:
			Rancor_Bite(aiEnt);
			break;
		case BOTH_ATTACK1:
			break;
		case BOTH_ATTACK2:
			break;
		case BOTH_ATTACK3:
			if (aiEnt->count == 1 && aiEnt->activator)
			{//swallow victim
				G_Sound(aiEnt->activator, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/chomp.wav"));
				//FIXME: sometimes end up with a live one in our mouths?
				//just make sure they're dead
				if (aiEnt->activator->health > 0)
				{
					//cut in half
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember(aiEnt->activator, aiEnt, aiEnt->activator->r.currentOrigin, G2_MODELPART_WAIST, 90, 0, aiEnt->activator->client->ps.torsoAnim, qtrue);
					//G_DoDismemberment( NPC->activator, NPC->enemy->r.currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
					//KILL
					G_Damage(aiEnt->activator, aiEnt, aiEnt, vec3_origin, aiEnt->activator->r.currentOrigin, 100000, DAMAGE_NO_PROTECTION | DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC, MOD_MELEE);//, HL_NONE );
					aiEnt->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					aiEnt->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim(aiEnt->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_AddEvent(aiEnt->activator, EV_JUMP, aiEnt->activator->health);
				}
				if (aiEnt->activator->client)
				{//*sigh*, can't get tags right, just remove them?
					aiEnt->activator->client->ps.eFlags |= EF_NODRAW;
				}
				aiEnt->count = 2;
				TIMER_Set(aiEnt, "clearGrabbed", 2600);
			}
			break;
		}
	}
	else if (aiEnt->client->ps.legsAnim == BOTH_ATTACK2)
	{
		if (aiEnt->client->ps.legsTimer >= 1200 && aiEnt->client->ps.legsTimer <= 1350)
		{
			if (Q_irand(0, 2))
			{
				Rancor_Swing(aiEnt, qfalse);
			}
			else
			{
				Rancor_Swing(aiEnt, qtrue);
			}
		}
		else if (aiEnt->client->ps.legsTimer >= 1100 && aiEnt->client->ps.legsTimer <= 1550)
		{
			Rancor_Swing(aiEnt, qtrue);
		}
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2(aiEnt, "attacking", qtrue);
}

extern void Jedi_Advance(gentity_t *aiEnt);
extern void Jedi_Retreat(gentity_t *aiEnt);

//----------------------------------
void Rancor_Combat(gentity_t *aiEnt)
{
	if (!TIMER_Done(aiEnt, "takingPain")) return;

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue);

	if ( aiEnt->count )
	{//holding my enemy
		if ( TIMER_Done2( aiEnt, "takingPain", qtrue ))
		{
			//aiEnt->NPC->localState = LSTATE_CLEAR;
		}
		else
		{
			Rancor_Attack(aiEnt, 0, qfalse );
		}

		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	float distance = DistanceHorizontal(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin);
	qboolean inRange = (qboolean)(distance < 256 ? qtrue : qfalse);

	if (TIMER_Done2(aiEnt, "takingPain", qtrue))
	{

	}
	else if (!inRange)
	{
		//Jedi_Advance(aiEnt);
		Rancor_Attack(aiEnt, distance, qtrue);
	}
	else
	{
		Rancor_Attack(aiEnt, distance, qfalse);
	}
}

/*
-------------------------
NPC_Rancor_Pain
-------------------------
*/
void NPC_Rancor_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if (!TIMER_Done(self, "painIgnoreTimer")) return;
	TIMER_Set(self, "painIgnoreTimer", 1000 + Q_irand(1000, 2500));

	qboolean hitByRancor = qfalse;
	if ( attacker 
		&& attacker->client 
		&& attacker->client->NPC_class == self->client->NPC_class )
	{
		hitByRancor = qtrue;
	}
	if ( attacker
		&& attacker->inuse
		&& attacker != self->enemy
		&& !(attacker->flags&FL_NOTARGET) )
	{
		if ( !self->count )
		{
			if ( (!attacker->s.number && !Q_irand(0,3))
				|| !self->enemy
				|| self->enemy->health == 0
				|| (self->enemy->client && self->enemy->client->NPC_class == self->client->NPC_class)
				|| (self->NPC && self->NPC->consecutiveBlockedMoves>=10 && DistanceSquared( attacker->r.currentOrigin, self->r.currentOrigin ) < DistanceSquared( self->enemy->r.currentOrigin, self->r.currentOrigin )) )
			{//if my enemy is dead (or attacked by player) and I'm not still holding/eating someone, turn on the attacker
				//FIXME: if can't nav to my enemy, take this guy if I can nav to him
				G_SetEnemy( self, attacker );
				TIMER_Set( self, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
				if ( hitByRancor )
				{//stay mad at this Rancor for 2-5 secs before looking for attacker enemies
					TIMER_Set( self, "rancorInfight", Q_irand( 2000, 5000 ) );
				}

			}
		}
	}
	if ( (hitByRancor|| (self->count==1&&self->activator&&!Q_irand(0,4)) || Q_irand( 0, 200 ) < damage )//hit by rancor, hit while holding live victim, or took a lot of damage
		&& self->client->ps.legsAnim != BOTH_STAND1TO2
		&& TIMER_Done( self, "takingPain" ) )
	{
		if ( !Rancor_CheckRoar( self ) )
		{
			if ( self->client->ps.legsAnim != BOTH_MELEE1
				&& self->client->ps.legsAnim != BOTH_MELEE2
				&& self->client->ps.legsAnim != BOTH_ATTACK2 )
			{//cant interrupt one of the big attack anims
				{//if going to bite our victim, only victim can interrupt that anim
					if ( self->health > 100 || hitByRancor )
					{
						TIMER_Remove( self, "attacking" );

						VectorCopy( self->NPC->lastPathAngles, self->s.angles );

						if ( self->count == 1 )
						{
							NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
						}
						else
						{
							NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
						}
						TIMER_Set( self, "takingPain", self->client->ps.legsTimer+Q_irand(0, 500) );

						if ( self->NPC )
						{
							self->NPC->localState = LSTATE_WAITING;
						}
					}
				}
			}
		}
		//let go
		if ( !Q_irand( 0, 8 ) && self->count == 1 )
		{
			Rancor_DropVictim( self );
		}
	}
}

void Rancor_CheckDropVictim(gentity_t *aiEnt)
{
	vec3_t mins, maxs;
	vec3_t start, end;
	trace_t	trace;

	VectorSet( mins, aiEnt->activator->r.mins[0]-1, aiEnt->activator->r.mins[1]-1, 0 );
	VectorSet( maxs, aiEnt->activator->r.maxs[0]+1, aiEnt->activator->r.maxs[1]+1, 1 );
	VectorSet( start, aiEnt->activator->r.currentOrigin[0], aiEnt->activator->r.currentOrigin[1], aiEnt->activator->r.absmin[2] );
	VectorSet( end, aiEnt->activator->r.currentOrigin[0], aiEnt->activator->r.currentOrigin[1], aiEnt->activator->r.absmax[2]-1 );

	trap->Trace( &trace, start, mins, maxs, end, aiEnt->activator->s.number, aiEnt->activator->clipmask, qfalse, 0, 0 );
	if ( !trace.allsolid && !trace.startsolid && trace.fraction >= 1.0f )
	{
		Rancor_DropVictim( aiEnt );
	}
}

//if he's stepping on things then crush them -rww
void Rancor_Crush(gentity_t *aiEnt)
{
	gentity_t *crush;

	if (!aiEnt ||
		!aiEnt->client ||
		aiEnt->client->ps.groundEntityNum >= ENTITYNUM_WORLD)
	{ //nothing to crush
		return;
	}

	crush = &g_entities[aiEnt->client->ps.groundEntityNum];
	if (crush->inuse && crush->client && !crush->localAnimIndex)
	{ //a humanoid, smash them good.
		G_Damage(crush, aiEnt, aiEnt, NULL, aiEnt->r.currentOrigin, 200, 0, MOD_TRIGGER_HURT/*MOD_CRUSH*/);
	}
}

/*
-------------------------
NPC_BSRancor_Default
-------------------------
*/
void NPC_BSRancor_Default(gentity_t *aiEnt)
{
	AddSightEvent( aiEnt, aiEnt->r.currentOrigin, 1024, AEL_DANGER_GREAT, 50 );

	Rancor_Crush(aiEnt);

	aiEnt->client->ps.eFlags2 &= ~(EF2_USE_ALT_ANIM|EF2_GENERIC_NPC_FLAG);
	if ( aiEnt->count )
	{//holding someone
		aiEnt->client->ps.eFlags2 |= EF2_USE_ALT_ANIM;
		if ( aiEnt->count == 2 )
		{//in my mouth
			aiEnt->client->ps.eFlags2 |= EF2_GENERIC_NPC_FLAG;
		}
	}
	else
	{
		aiEnt->client->ps.eFlags2 &= ~(EF2_USE_ALT_ANIM|EF2_GENERIC_NPC_FLAG);
	}

	if ( TIMER_Done2( aiEnt, "clearGrabbed", qtrue ) )
	{
		Rancor_DropVictim( aiEnt );
	}
	else if ( aiEnt->client->ps.legsAnim == BOTH_PAIN2
		&& aiEnt->count == 1
		&& aiEnt->activator )
	{
		if ( !Q_irand( 0, 3 ) )
		{
			Rancor_CheckDropVictim(aiEnt);
		}
	}
	if ( !TIMER_Done( aiEnt, "rageTime" ) )
	{//do nothing but roar first time we see an enemy
		AddSoundEvent( aiEnt, aiEnt->r.currentOrigin, 1024, AEL_DANGER_GREAT, qfalse );//, qfalse );
		NPC_FaceEnemy(aiEnt, qtrue );
		return;
	}

	if (aiEnt->enemy && !ValidEnemy(aiEnt, aiEnt->enemy))
	{
		if (aiEnt->count == 2 && aiEnt->client->ps.legsAnim == BOTH_ATTACK3)
		{//we're still chewing our enemy up
			NPC_UpdateAngles(aiEnt, qtrue, qtrue);
			return;
		}

		aiEnt->count = 0;
		aiEnt->activator = NULL;
		aiEnt->enemy = NULL;
	}

	if ( aiEnt->enemy )
	{
		/*
		if ( NPC->enemy->client //enemy is a client
			&& (NPC->enemy->client->NPC_class == CLASS_UGNAUGHT || NPC->enemy->client->NPC_class == CLASS_JAWA )//enemy is a lowly jawa or ugnaught
			&& NPC->enemy->enemy != NPC//enemy's enemy is not me
			&& (!NPC->enemy->enemy || !NPC->enemy->enemy->client || NPC->enemy->enemy->client->NPC_class!=CLASS_RANCOR) )//enemy's enemy is not a client or is not a rancor (which is as scary as me anyway)
		{//they should be scared of ME and no-one else
			G_SetEnemy( NPC->enemy, NPC );
		}
		*/
		if ( TIMER_Done(aiEnt,"angrynoise") )
		{
			G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex( va("sound/chars/rancor/misc/anger%d.wav", Q_irand(1, 3))) );

			TIMER_Set( aiEnt, "angrynoise", Q_irand( 5000, 10000 ) );
		}
		else
		{
			AddSoundEvent( aiEnt, aiEnt->r.currentOrigin, 512, AEL_DANGER_GREAT, qfalse );//, qfalse );
		}
		if ( aiEnt->count == 2 && aiEnt->client->ps.legsAnim == BOTH_ATTACK3 )
		{//we're still chewing our enemy up
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return;
		}
		//else, if he's in our hand, we eat, else if he's on the ground, we keep attacking his dead body for a while
		if( aiEnt->enemy->client 
			&& (aiEnt->enemy->client->NPC_class == aiEnt->client->NPC_class))
		{//got mad at another Rancor, look for a valid enemy
			if ( TIMER_Done( aiEnt, "rancorInfight" ) )
			{
				NPC_CheckEnemyExt(aiEnt, qtrue );
			}
		}
		/*else if ( !aiEnt->count )
		{
			if ( ValidEnemy(aiEnt, aiEnt->enemy ) == qfalse )
			{
				TIMER_Remove( aiEnt, "lookForNewEnemy" );//make them look again right now
				if ( !aiEnt->enemy->inuse || level.time - aiEnt->enemy->s.time > Q_irand( 10000, 15000 ) )
				{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
					aiEnt->enemy = NULL;
					Rancor_Patrol(aiEnt);
					NPC_UpdateAngles(aiEnt, qtrue, qtrue );
					return;
				}
			}
			if ( TIMER_Done( aiEnt, "lookForNewEnemy" ) )
			{
				gentity_t *newEnemy, *sav_enemy = aiEnt->enemy;//FIXME: what about NPC->lastEnemy?
				aiEnt->enemy = NULL;
				newEnemy = NPC_CheckEnemy(aiEnt, (qboolean)(aiEnt->NPC->confusionTime < level.time), qfalse, qfalse );
				aiEnt->enemy = sav_enemy;
				if ( newEnemy && newEnemy != sav_enemy )
				{//picked up a new enemy!
					aiEnt->lastEnemy = aiEnt->enemy;
					G_SetEnemy( aiEnt, newEnemy );
					//hold this one for at least 5-15 seconds
					TIMER_Set( aiEnt, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
				}
				else
				{//look again in 2-5 secs
					TIMER_Set( aiEnt, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
				}
			}
		}*/
		Rancor_Combat(aiEnt);
	}
	else
	{
		if ( TIMER_Done(aiEnt,"idlenoise") )
		{
			G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex( va("sound/chars/rancor/snort_%d.wav", Q_irand(1, 2))) );

			TIMER_Set( aiEnt, "idlenoise", Q_irand( 2000, 4000 ) );
			AddSoundEvent( aiEnt, aiEnt->r.currentOrigin, 384, AEL_DANGER, qfalse );//, qfalse );
		}
		if ( aiEnt->NPC->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		{
			Rancor_Patrol(aiEnt);
		}
		else
		{
			Rancor_Idle(aiEnt);
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}
