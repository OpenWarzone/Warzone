#include "b_local.h"
#include "g_nav.h"

void Interrogator_Idle( gentity_t *aiEnt );
void DeathFX( gentity_t *ent );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

enum
{
LSTATE_BLADESTOP=0,
LSTATE_BLADEUP,
LSTATE_BLADEDOWN,
};

/*
-------------------------
NPC_Interrogator_Precache
-------------------------
*/
void NPC_Interrogator_Precache(gentity_t *self)
{
	G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_lp" );
	G_SoundIndex("sound/chars/mark1/misc/anger.wav");
	G_SoundIndex( "sound/chars/probe/misc/talk");
	G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_inject" );
	G_SoundIndex( "sound/chars/interrogator/misc/int_droid_explo" );
	G_EffectIndex( "explosions/droidexplosion1" );
}
/*
-------------------------
Interrogator_die
-------------------------
*/
void Interrogator_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	self->client->ps.velocity[2] = -100;
	/*
	self->locationDamage[HL_NONE] += damage;
	if (self->locationDamage[HL_NONE] > 40)
	{
		DeathFX(self);
		self->client->ps.eFlags |= EF_NODRAW;
		self->contents = CONTENTS_CORPSE;
	}
	else
	*/
	{
		self->client->ps.eFlags2 &= ~EF2_FLYING;//moveType = MT_WALK;
		self->client->ps.velocity[0] = Q_irand( -10, -20 );
		self->client->ps.velocity[1] = Q_irand( -10, -20 );
		self->client->ps.velocity[2] = -100;
	}
	//self->takedamage = qfalse;
	//self->client->ps.eFlags |= EF_NODRAW;
	//self->contents = 0;
	return;
}

/*
-------------------------
Interrogator_PartsMove
-------------------------
*/
void Interrogator_PartsMove(gentity_t *aiEnt)
{
	// Syringe
	if ( TIMER_Done(aiEnt,"syringeDelay") )
	{
		aiEnt->pos1[1] = AngleNormalize360( aiEnt->pos1[1]);

		if ((aiEnt->pos1[1] < 60) || (aiEnt->pos1[1] > 300))
		{
			aiEnt->pos1[1]+=Q_irand( -20, 20 );	// Pitch
		}
		else if (aiEnt->pos1[1] > 180)
		{
			aiEnt->pos1[1]=Q_irand( 300, 360 );	// Pitch
		}
		else
		{
			aiEnt->pos1[1]=Q_irand( 0, 60 );	// Pitch
		}

	//	trap->G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone1, NPC->pos1, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );
		NPC_SetBoneAngles(aiEnt, "left_arm", aiEnt->pos1);

		TIMER_Set( aiEnt, "syringeDelay", Q_irand( 100, 1000 ) );
	}

	// Scalpel
	if ( TIMER_Done(aiEnt,"scalpelDelay") )
	{
		// Change pitch
		if ( aiEnt->NPC->localState == LSTATE_BLADEDOWN )	// Blade is moving down
		{
			aiEnt->pos2[0]-= 30;
			if (aiEnt->pos2[0] < 180)
			{
				aiEnt->pos2[0] = 180;
				aiEnt->NPC->localState = LSTATE_BLADEUP;	// Make it move up
			}
		}
		else											// Blade is coming back up
		{
			aiEnt->pos2[0]+= 30;
			if (aiEnt->pos2[0] >= 360)
			{
				aiEnt->pos2[0] = 360;
				aiEnt->NPC->localState = LSTATE_BLADEDOWN;	// Make it move down
				TIMER_Set( aiEnt, "scalpelDelay", Q_irand( 100, 1000 ) );
			}
		}

		aiEnt->pos2[0] = AngleNormalize360( aiEnt->pos2[0]);
	//	trap->G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone2, NPC->pos2, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );

		NPC_SetBoneAngles(aiEnt, "right_arm", aiEnt->pos2);
	}

	// Claw
	aiEnt->pos3[1] += Q_irand( 10, 30 );
	aiEnt->pos3[1] = AngleNormalize360( aiEnt->pos3[1]);
	//trap->G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone3, NPC->pos3, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );

	NPC_SetBoneAngles(aiEnt, "claw", aiEnt->pos3);

}

#define VELOCITY_DECAY	0.85f
#define HUNTER_UPWARD_PUSH	2

/*
-------------------------
Interrogator_MaintainHeight
-------------------------
*/
void Interrogator_MaintainHeight(gentity_t *aiEnt)
{
	float	dif;
//	vec3_t	endPos;
//	trace_t	trace;

	aiEnt->s.loopSound = G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_lp" );
	// Update our angles regardless
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	// If we have an enemy, we should try to hover at about enemy eye level
	if ( aiEnt->enemy )
	{
		// Find the height difference
		dif = (aiEnt->enemy->r.currentOrigin[2] + aiEnt->enemy->r.maxs[2]) - aiEnt->r.currentOrigin[2];

		// cap to prevent dramatic height shifts
		if ( fabs( dif ) > 2 )
		{
			if ( fabs( dif ) > 16 )
			{
				dif = ( dif < 0 ? -16 : 16 );
			}

			aiEnt->client->ps.velocity[2] = (aiEnt->client->ps.velocity[2]+dif)/2;
		}
	}
	else
	{
		gentity_t *goal = NULL;

		if ( aiEnt->NPC->goalEntity )	// Is there a goal?
		{
			goal = aiEnt->NPC->goalEntity;
		}
		else
		{
			goal = aiEnt->NPC->lastGoalEntity;
		}
		if ( goal )
		{
			dif = goal->r.currentOrigin[2] - aiEnt->r.currentOrigin[2];

			if ( fabs( dif ) > 24 )
			{
				aiEnt->client->pers.cmd.upmove = ( aiEnt->client->pers.cmd.upmove < 0 ? -4 : 4 );
			}
			else
			{
				if ( aiEnt->client->ps.velocity[2] )
				{
					aiEnt->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( aiEnt->client->ps.velocity[2] ) < 2 )
					{
						aiEnt->client->ps.velocity[2] = 0;
					}
				}
			}
		}
		// Apply friction
		else if ( aiEnt->client->ps.velocity[2] )
		{
			aiEnt->client->ps.velocity[2] *= VELOCITY_DECAY;

			if ( fabs( aiEnt->client->ps.velocity[2] ) < 1 )
			{
				aiEnt->client->ps.velocity[2] = 0;
			}
		}
	}

	// Apply friction
	if ( aiEnt->client->ps.velocity[0] )
	{
		aiEnt->client->ps.velocity[0] *= VELOCITY_DECAY;

		if ( fabs( aiEnt->client->ps.velocity[0] ) < 1 )
		{
			aiEnt->client->ps.velocity[0] = 0;
		}
	}

	if ( aiEnt->client->ps.velocity[1] )
	{
		aiEnt->client->ps.velocity[1] *= VELOCITY_DECAY;

		if ( fabs( aiEnt->client->ps.velocity[1] ) < 1 )
		{
			aiEnt->client->ps.velocity[1] = 0;
		}
	}
}

#define HUNTER_STRAFE_VEL	32
#define HUNTER_STRAFE_DIS	200
/*
-------------------------
Interrogator_Strafe
-------------------------
*/
void Interrogator_Strafe(gentity_t *aiEnt)
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;
	float	dif;

	AngleVectors( aiEnt->client->renderInfo.eyeAngles, NULL, right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = ( rand() & 1 ) ? -1 : 1;
	VectorMA( aiEnt->r.currentOrigin, HUNTER_STRAFE_DIS * dir, right, end );

	trap->Trace( &tr, aiEnt->r.currentOrigin, NULL, NULL, end, aiEnt->s.number, MASK_SOLID, qfalse, 0, 0 );

	// Close enough
	if ( tr.fraction > 0.9f )
	{
		VectorMA( aiEnt->client->ps.velocity, HUNTER_STRAFE_VEL * dir, right, aiEnt->client->ps.velocity );

		// Add a slight upward push
		if ( aiEnt->enemy )
		{
			// Find the height difference
			dif = (aiEnt->enemy->r.currentOrigin[2] + 32) - aiEnt->r.currentOrigin[2];

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 8 )
			{
				dif = ( dif < 0 ? -HUNTER_UPWARD_PUSH : HUNTER_UPWARD_PUSH );
			}

			aiEnt->client->ps.velocity[2] += dif;

		}

		// Set the strafe start time
		//aiEnt->fx_time = level.time;
		aiEnt->NPC->standTime = level.time + 3000 + random() * 500;
	}
}

/*
-------------------------
Interrogator_Hunt
-------------------------`
*/

#define HUNTER_FORWARD_BASE_SPEED	10
#define HUNTER_FORWARD_MULTIPLIER	2

void Interrogator_Hunt(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	Interrogator_PartsMove(aiEnt);

	NPC_FaceEnemy(aiEnt, qfalse);

	//If we're not supposed to stand still, pursue the player
	if ( aiEnt->NPC->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Interrogator_Strafe(aiEnt);
			if ( aiEnt->NPC->standTime > level.time )
			{//successfully strafed
				return;
			}
		}
	}

	//If we don't want to advance, stop here
	if ( advance == qfalse )
		return;

	//Only try and navigate if the player is visible
	if ( visible == qfalse )
	{
		// Move towards our goal
		aiEnt->NPC->goalEntity = aiEnt->enemy;
		aiEnt->NPC->goalRadius = 12;

		//Get our direction from the navigator if we can't see our target
		if ( NPC_GetMoveDirection(aiEnt, forward, &distance ) == qfalse )
			return;
	}
	else
	{
		VectorSubtract( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, forward );
		/*distance = */VectorNormalize( forward );
	}

	speed = HUNTER_FORWARD_BASE_SPEED + HUNTER_FORWARD_MULTIPLIER * g_npcspskill.integer;
	VectorMA( aiEnt->client->ps.velocity, speed, forward, aiEnt->client->ps.velocity );
}

#define MIN_DISTANCE		64

/*
-------------------------
Interrogator_Melee
-------------------------
*/
void Interrogator_Melee(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	if ( TIMER_Done( aiEnt, "attackDelay" ) )	// Attack?
	{
		// Make sure that we are within the height range before we allow any damage to happen
		if ( aiEnt->r.currentOrigin[2] >= aiEnt->enemy->r.currentOrigin[2]+aiEnt->enemy->r.mins[2] && aiEnt->r.currentOrigin[2]+aiEnt->r.mins[2]+8 < aiEnt->enemy->r.currentOrigin[2]+aiEnt->enemy->r.maxs[2] )
		{
			//gentity_t *tent;

			TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 3000 ) );
			G_Damage( aiEnt->enemy, aiEnt, aiEnt, 0, 0, 2, DAMAGE_NO_KNOCKBACK, MOD_MELEE );

		//	NPC->enemy->client->poisonDamage = 18;
		//	NPC->enemy->client->poisonTime = level.time + 1000;

			// Drug our enemy up and do the wonky vision thing
//			tent = G_TempEntity( NPC->enemy->r.currentOrigin, EV_DRUGGED );
//			tent->owner = NPC->enemy;

			//rwwFIXMEFIXME: poison damage

			G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_inject.mp3" ));
		}
	}

	if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Interrogator_Hunt(aiEnt, visible, advance );
	}
}

/*
-------------------------
Interrogator_Attack
-------------------------
*/
void Interrogator_Attack(gentity_t *aiEnt)
{
	float		distance;
	qboolean	visible;
	qboolean	advance;

	// Always keep a good height off the ground
	Interrogator_MaintainHeight(aiEnt);

	//randomly talk
	if ( TIMER_Done(aiEnt,"patrolNoise") )
	{
		if (TIMER_Done(aiEnt,"angerNoise"))
		{
			G_SoundOnEnt( aiEnt, CHAN_AUTO, va("sound/chars/probe/misc/talk.wav",	Q_irand(1, 3)) );

			TIMER_Set( aiEnt, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )
	{
		Interrogator_Idle(aiEnt);
		return;
	}

	// Rate our distance to the target, and our visibilty
	distance	= (int) DistanceHorizontalSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4(aiEnt, aiEnt->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE*MIN_DISTANCE );

	if ( !visible )
	{
		advance = qtrue;
	}
	if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Interrogator_Hunt(aiEnt, visible, advance );
	}

	NPC_FaceEnemy(aiEnt, qtrue );

	if (!advance)
	{
		Interrogator_Melee(aiEnt, visible, advance );
	}
}

/*
-------------------------
Interrogator_Idle
-------------------------
*/
void Interrogator_Idle(gentity_t *aiEnt)
{
	if ( NPC_CheckPlayerTeamStealth(aiEnt) )
	{
		G_SoundOnEnt( aiEnt, CHAN_AUTO, "sound/chars/mark1/misc/anger.wav" );
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	Interrogator_MaintainHeight(aiEnt);

	NPC_BSIdle(aiEnt);
}

/*
-------------------------
NPC_BSInterrogator_Default
-------------------------
*/
void NPC_BSInterrogator_Default(gentity_t *aiEnt)
{
	//NPC->e_DieFunc = dieF_Interrogator_die;

	if ( aiEnt->enemy )
	{
		Interrogator_Attack(aiEnt);
	}
	else
	{
		Interrogator_Idle(aiEnt);
	}

}
