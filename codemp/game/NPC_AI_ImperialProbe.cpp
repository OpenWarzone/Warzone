#include "b_local.h"
#include "g_nav.h"

gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_BACKINGUP,
	LSTATE_SPINNING,
	LSTATE_PAIN,
	LSTATE_DROP
};

void ImperialProbe_Idle(gentity_t *aiEnt);

void NPC_Probe_Precache(void)
{
	int i;

	for ( i = 1; i < 4; i++)
	{
		G_SoundIndex( va( "sound/chars/probe/misc/probetalk%d", i ) );
	}
	G_SoundIndex( "sound/chars/probe/misc/probedroidloop" );
	G_SoundIndex("sound/chars/probe/misc/anger1");
	G_SoundIndex("sound/chars/probe/misc/fire");

	G_EffectIndex( "chunks/probehead" );
	G_EffectIndex( "env/med_explode2" );
	G_EffectIndex( "explosions/probeexplosion1");
	G_EffectIndex( "bryar/muzzle_flash" );

#ifndef __MMO__
	RegisterItem( BG_FindItemForAmmo( AMMO_BLASTER ));
	RegisterItem( BG_FindItemForWeapon(WP_MODULIZED_WEAPON) );
#endif //__MMO__
}
/*
-------------------------
Hunter_MaintainHeight
-------------------------
*/

#define VELOCITY_DECAY	0.85f

void ImperialProbe_MaintainHeight( gentity_t *aiEnt)
{
	float	dif;
//	vec3_t	endPos;
//	trace_t	trace;

	// Update our angles regardless
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	// If we have an enemy, we should try to hover at about enemy eye level
	if ( aiEnt->enemy )
	{
		// Find the height difference
		dif = aiEnt->enemy->r.currentOrigin[2] - aiEnt->r.currentOrigin[2];

		// cap to prevent dramatic height shifts
		if ( fabs( dif ) > 8 )
		{
			if ( fabs( dif ) > 16 )
				dif = ( dif < 0 ? -16 : 16 );

			aiEnt->client->ps.velocity[2] = (aiEnt->client->ps.velocity[2]+dif)/2;
		}
	}
	else
	{
		gentity_t *goal = NULL;

		if ( aiEnt->NPC->goalEntity )	// Is there a goal?
			goal = aiEnt->NPC->goalEntity;
		else
			goal = aiEnt->NPC->lastGoalEntity;

		if ( goal )
		{
			dif = goal->r.currentOrigin[2] - aiEnt->r.currentOrigin[2];

			if ( fabs( dif ) > 24 ) {
				aiEnt->client->pers.cmd.upmove = ( aiEnt->client->pers.cmd.upmove < 0 ? -4 : 4 );
			}
			else {
				if ( aiEnt->client->ps.velocity[2] )
				{
					aiEnt->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( aiEnt->client->ps.velocity[2] ) < 2 )
						aiEnt->client->ps.velocity[2] = 0;
				}
			}
		}
		// Apply friction
		else if ( aiEnt->client->ps.velocity[2] )
		{
			aiEnt->client->ps.velocity[2] *= VELOCITY_DECAY;

			if ( fabsf( aiEnt->client->ps.velocity[2] ) < 1 )
				aiEnt->client->ps.velocity[2] = 0;
		}

		// Stay at a given height until we take on an enemy
/*		VectorSet( endPos, NPC->r.currentOrigin[0], NPC->r.currentOrigin[1], NPC->r.currentOrigin[2] - 512 );
		trap->Trace( &trace, NPC->r.currentOrigin, NULL, NULL, endPos, NPC->s.number, MASK_SOLID );

		if ( trace.fraction != 1.0f )
		{
			float	length = ( trace.fraction * 512 );

			if ( length < 80 )
			{
				ucmd.upmove = 32;
			}
			else if ( length > 120 )
			{
				ucmd.upmove = -32;
			}
			else
			{
				if ( NPC->client->ps.velocity[2] )
				{
					NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( NPC->client->ps.velocity[2] ) < 1 )
					{
						NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		} */
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

/*
-------------------------
ImperialProbe_Strafe
-------------------------
*/

#define HUNTER_STRAFE_VEL	256
#define HUNTER_STRAFE_DIS	200
#define HUNTER_UPWARD_PUSH	32

void ImperialProbe_Strafe(gentity_t *aiEnt)
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;

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
		aiEnt->client->ps.velocity[2] += HUNTER_UPWARD_PUSH;

		// Set the strafe start time so we can do a controlled roll
		//NPC->fx_time = level.time;
		aiEnt->NPC->standTime = level.time + 3000 + random() * 500;
	}
}

/*
-------------------------
ImperialProbe_Hunt
-------------------------`
*/

#define HUNTER_FORWARD_BASE_SPEED	10
#define HUNTER_FORWARD_MULTIPLIER	5

void ImperialProbe_Hunt(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	//If we're not supposed to stand still, pursue the player
	if ( aiEnt->NPC->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			ImperialProbe_Strafe(aiEnt);
			return;
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
		if ( NPC_GetMoveDirection( aiEnt, forward, &distance ) == qfalse )
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

/*
-------------------------
ImperialProbe_FireBlaster
-------------------------
*/
void ImperialProbe_FireBlaster(gentity_t *aiEnt)
{
	vec3_t	muzzle1,enemy_org1,delta1,angleToEnemy1;
	static	vec3_t	forward, vright, up;
//	static	vec3_t	muzzle;
	int genBolt1;
	gentity_t	*missile;
	mdxaBone_t	boltMatrix;

	genBolt1 = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "*flash");

	//FIXME: use {0, NPC->client->ps.legsYaw, 0}
	trap->G2API_GetBoltMatrix( aiEnt->ghoul2, 0,
				genBolt1,
				&boltMatrix, aiEnt->r.currentAngles, aiEnt->r.currentOrigin, level.time,
				NULL, aiEnt->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle1 );

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), muzzle1, vec3_origin );

	G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/probe/misc/fire" ));

	if (aiEnt->health)
	{
		CalcEntitySpot( aiEnt->enemy, SPOT_CHEST, enemy_org1 );
		enemy_org1[0]+= Q_irand(0,10);
		enemy_org1[1]+= Q_irand(0,10);
		VectorSubtract (enemy_org1, muzzle1, delta1);
		vectoangles ( delta1, angleToEnemy1 );
		AngleVectors (angleToEnemy1, forward, vright, up);
	}
	else
	{
		AngleVectors (aiEnt->r.currentAngles, forward, vright, up);
	}

	missile = CreateMissile( muzzle1, forward, 1600, 10000, aiEnt, qfalse );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_MODULIZED_WEAPON;

	if ( g_npcspskill.integer <= 1 )
	{
		missile->damage = 5;
	}
	else
	{
		missile->damage = 10;
	}


	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_UNKNOWN;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

}

/*
-------------------------
ImperialProbe_Ranged
-------------------------
*/
void ImperialProbe_Ranged(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	int	delay_min, delay_max;

	if ( TIMER_Done( aiEnt, "attackDelay" ) )	// Attack?
	{

		if ( g_npcspskill.integer == 0 )
		{
			delay_min = 500;
			delay_max = 3000;
		}
		else if ( g_npcspskill.integer > 1 )
		{
			delay_min = 500;
			delay_max = 2000;
		}
		else
		{
			delay_min = 300;
			delay_max = 1500;
		}

		TIMER_Set( aiEnt, "attackDelay", Q_irand( delay_min, delay_max ) );
		ImperialProbe_FireBlaster(aiEnt);
//		ucmd.buttons |= BUTTON_ATTACK;
	}

	if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
	{
		ImperialProbe_Hunt(aiEnt, visible, advance );
	}
}

/*
-------------------------
ImperialProbe_AttackDecision
-------------------------
*/

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

void ImperialProbe_AttackDecision(gentity_t *aiEnt)
{
	float		distance;
	qboolean	visible, advance;

	// Always keep a good height off the ground
	ImperialProbe_MaintainHeight(aiEnt);

	//randomly talk
	if ( TIMER_Done(aiEnt,"patrolNoise") )
	{
		if (TIMER_Done(aiEnt,"angerNoise"))
		{
			G_SoundOnEnt( aiEnt, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d", Q_irand(1, 3)) );

			TIMER_Set( aiEnt, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )
	{
		ImperialProbe_Idle(aiEnt);
		return;
	}

	NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_NORMAL);

	// Rate our distance to the target, and our visibilty
	distance	= (int) DistanceHorizontalSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4(aiEnt, aiEnt->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
		{
			ImperialProbe_Hunt(aiEnt, visible, advance );
			return;
		}
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt,  qtrue );

	// Decide what type of attack to do
	ImperialProbe_Ranged(aiEnt, visible, advance );
}

/*
-------------------------
NPC_BSDroid_Pain
-------------------------
*/
void NPC_Probe_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	float	pain_chance;
	gentity_t *other = attacker;
	int mod = gPainMOD;

	VectorCopy( self->NPC->lastPathAngles, self->s.angles );

	if ( self->health < 30 || mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT ) // demp2 always messes them up real good
	{
		vec3_t endPos;
		trace_t	trace;

		VectorSet( endPos, self->r.currentOrigin[0], self->r.currentOrigin[1], self->r.currentOrigin[2] - 128 );
		trap->Trace( &trace, self->r.currentOrigin, NULL, NULL, endPos, self->s.number, MASK_SOLID, qfalse, 0, 0 );

		if ( trace.fraction == 1.0f || mod == MOD_DEMP2 ) // demp2 always does this
		{
			/*
			if (self->client->clientInfo.headModel != 0)
			{
				vec3_t origin;

				VectorCopy(self->r.currentOrigin,origin);
				origin[2] +=50;
//				G_PlayEffect( "small_chunks", origin );
				G_PlayEffect( "chunks/probehead", origin );
				G_PlayEffect( "env/med_explode2", origin );
				self->client->clientInfo.headModel = 0;
				self->client->moveType = MT_RUNJUMP;
				self->client->ps.gravity = g_gravity->value*.1;
			}
			*/

			if ( (mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT) && other )
			{
				vec3_t dir;

				NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);

				VectorSubtract( self->r.currentOrigin, other->r.currentOrigin, dir );
				VectorNormalize( dir );

				VectorMA( self->client->ps.velocity, 550, dir, self->client->ps.velocity );
				self->client->ps.velocity[2] -= 127;
			}

			//self->s.powerups |= ( 1 << PW_SHOCKED );
			//self->client->ps.powerups[PW_SHOCKED] = level.time + 3000;
			self->client->ps.electrifyTime = level.time + 3000;

			self->NPC->localState = LSTATE_DROP;
		}
	}
	else
	{
		pain_chance = NPC_GetPainChance( self, damage );

		if ( random() < pain_chance )	// Spin around in pain?
		{
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE);
		}
	}

	NPC_Pain( self, attacker, damage );
}

/*
-------------------------
ImperialProbe_Idle
-------------------------
*/

void ImperialProbe_Idle(gentity_t *aiEnt)
{
	ImperialProbe_MaintainHeight(aiEnt);

	NPC_BSIdle(aiEnt);
}

/*
-------------------------
NPC_BSImperialProbe_Patrol
-------------------------
*/
void ImperialProbe_Patrol(gentity_t *aiEnt)
{
	ImperialProbe_MaintainHeight(aiEnt);

	if ( NPC_CheckPlayerTeamStealth(aiEnt) )
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	//If we have somewhere to go, then do that
	if (!aiEnt->enemy)
	{
		NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_NORMAL );

		if ( UpdateGoal(aiEnt) )
		{
			//start loop sound once we move
			aiEnt->s.loopSound = G_SoundIndex( "sound/chars/probe/misc/probedroidloop" );
			aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal(aiEnt, qtrue );
		}
		//randomly talk
		if (TIMER_Done(aiEnt,"patrolNoise"))
		{
			G_SoundOnEnt( aiEnt, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d", Q_irand(1, 3)) );

			TIMER_Set( aiEnt, "patrolNoise", Q_irand( 2000, 4000 ) );
		}
	}
	else	// He's got an enemy. Make him angry.
	{
		G_SoundOnEnt( aiEnt, CHAN_AUTO, "sound/chars/probe/misc/anger1" );
		TIMER_Set( aiEnt, "angerNoise", Q_irand( 2000, 4000 ) );
		//NPCInfo->behaviorState = BS_HUNT_AND_KILL;
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

/*
-------------------------
ImperialProbe_Wait
-------------------------
*/
void ImperialProbe_Wait(gentity_t *aiEnt)
{
	if ( aiEnt->NPC->localState == LSTATE_DROP )
	{
		vec3_t endPos;
		trace_t	trace;

		aiEnt->NPC->desiredYaw = AngleNormalize360( aiEnt->NPC->desiredYaw + 25 );

		VectorSet( endPos, aiEnt->r.currentOrigin[0], aiEnt->r.currentOrigin[1], aiEnt->r.currentOrigin[2] - 32 );
		trap->Trace( &trace, aiEnt->r.currentOrigin, NULL, NULL, endPos, aiEnt->s.number, MASK_SOLID, qfalse, 0, 0 );

		if ( trace.fraction != 1.0f )
		{
			G_Damage(aiEnt, aiEnt->enemy, aiEnt->enemy, NULL, NULL, 2000, 0,MOD_UNKNOWN);
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

/*
-------------------------
NPC_BSImperialProbe_Default
-------------------------
*/
void NPC_BSImperialProbe_Default(gentity_t *aiEnt)
{

	if ( aiEnt->enemy )
	{
		aiEnt->NPC->goalEntity = aiEnt->enemy;
		ImperialProbe_AttackDecision(aiEnt);
	}
	else if ( aiEnt->NPC->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		ImperialProbe_Patrol(aiEnt);
	}
	else if ( aiEnt->NPC->localState == LSTATE_DROP )
	{
		ImperialProbe_Wait(aiEnt);
	}
	else
	{
		ImperialProbe_Idle(aiEnt);
	}
}
