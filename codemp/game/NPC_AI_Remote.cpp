#include "b_local.h"
#include "g_nav.h"

void Remote_Strafe(gentity_t *aiEnt);

#define VELOCITY_DECAY	0.85f


//Local state enums
enum
{
	LSTATE_NONE = 0,
};

void Remote_Idle(gentity_t *aiEnt);

void NPC_Remote_Precache(void)
{
	G_SoundIndex("sound/chars/remote/misc/fire.wav");
	G_SoundIndex( "sound/chars/remote/misc/hiss.wav");
	G_EffectIndex( "env/small_explode");
}

/*
-------------------------
NPC_Remote_Pain
-------------------------
*/
void NPC_Remote_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	Remote_Strafe(self);

	NPC_Pain( self, attacker, damage );
}

/*
-------------------------
Remote_MaintainHeight
-------------------------
*/
void Remote_MaintainHeight( gentity_t *aiEnt)
{
	float	dif;

	// Update our angles regardless
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	if ( aiEnt->client->ps.velocity[2] )
	{
		aiEnt->client->ps.velocity[2] *= VELOCITY_DECAY;

		if ( fabs( aiEnt->client->ps.velocity[2] ) < 2 )
		{
			aiEnt->client->ps.velocity[2] = 0;
		}
	}
	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if ( aiEnt->enemy )
	{
		if (TIMER_Done( aiEnt, "heightChange"))
		{
			TIMER_Set( aiEnt,"heightChange",Q_irand( 1000, 3000 ));

			// Find the height difference
			dif = (aiEnt->enemy->r.currentOrigin[2] +  Q_irand( 0, aiEnt->enemy->r.maxs[2]+8 )) - aiEnt->r.currentOrigin[2];

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 2 )
			{
				if ( fabs( dif ) > 24 )
				{
					dif = ( dif < 0 ? -24 : 24 );
				}
				dif *= 10;
				aiEnt->client->ps.velocity[2] = (aiEnt->client->ps.velocity[2]+dif)/2;
			//	NPC->fx_time = level.time;
				G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex("sound/chars/remote/misc/hiss.wav"));
			}
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
				dif = ( dif < 0 ? -24 : 24 );
				aiEnt->client->ps.velocity[2] = (aiEnt->client->ps.velocity[2]+dif)/2;
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

#define REMOTE_STRAFE_VEL	256
#define REMOTE_STRAFE_DIS	200
#define REMOTE_UPWARD_PUSH	32

/*
-------------------------
Remote_Strafe
-------------------------
*/
void Remote_Strafe(gentity_t *aiEnt)
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;

	AngleVectors( aiEnt->client->renderInfo.eyeAngles, NULL, right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = ( rand() & 1 ) ? -1 : 1;
	VectorMA( aiEnt->r.currentOrigin, REMOTE_STRAFE_DIS * dir, right, end );

	trap->Trace( &tr, aiEnt->r.currentOrigin, NULL, NULL, end, aiEnt->s.number, MASK_SOLID, qfalse, 0, 0 );

	// Close enough
	if ( tr.fraction > 0.9f )
	{
		VectorMA( aiEnt->client->ps.velocity, REMOTE_STRAFE_VEL * dir, right, aiEnt->client->ps.velocity );

		G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex("sound/chars/remote/misc/hiss.wav"));

		// Add a slight upward push
		aiEnt->client->ps.velocity[2] += REMOTE_UPWARD_PUSH;

		// Set the strafe start time so we can do a controlled roll
	//	NPC->fx_time = level.time;
		aiEnt->NPC->standTime = level.time + 3000 + random() * 500;
	}
}

#define REMOTE_FORWARD_BASE_SPEED	10
#define REMOTE_FORWARD_MULTIPLIER	5

/*
-------------------------
Remote_Hunt
-------------------------
*/
void Remote_Hunt(gentity_t *aiEnt, qboolean visible, qboolean advance, qboolean retreat )
{
	float	distance, speed;
	vec3_t	forward;

	//If we're not supposed to stand still, pursue the player
	if ( aiEnt->NPC->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Remote_Strafe(aiEnt);
			return;
		}
	}

	//If we don't want to advance, stop here
	if ( advance == qfalse && visible == qtrue )
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

	speed = REMOTE_FORWARD_BASE_SPEED + REMOTE_FORWARD_MULTIPLIER * g_npcspskill.integer;
	if ( retreat == qtrue )
	{
		speed *= -1;
	}
	VectorMA( aiEnt->client->ps.velocity, speed, forward, aiEnt->client->ps.velocity );
}


/*
-------------------------
Remote_Fire
-------------------------
*/
void Remote_Fire (gentity_t *aiEnt)
{
	vec3_t	delta1, enemy_org1, muzzle1;
	vec3_t	angleToEnemy1;
	static	vec3_t	forward, vright, up;
//	static	vec3_t	muzzle;
	gentity_t	*missile;

	CalcEntitySpot( aiEnt->enemy, SPOT_HEAD, enemy_org1 );
	VectorCopy( aiEnt->r.currentOrigin, muzzle1 );

	VectorSubtract (enemy_org1, muzzle1, delta1);

	vectoangles ( delta1, angleToEnemy1 );
	AngleVectors (angleToEnemy1, forward, vright, up);

	missile = CreateMissile( aiEnt->r.currentOrigin, forward, 1000, 10000, aiEnt, qfalse );

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), aiEnt->r.currentOrigin, forward );

	missile->classname = "briar";
	missile->s.weapon = WP_MODULIZED_WEAPON;

	missile->damage = 10;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

}

/*
-------------------------
Remote_Ranged
-------------------------
*/
void Remote_Ranged(gentity_t *aiEnt, qboolean visible, qboolean advance, qboolean retreat )
{
	if ( TIMER_Done( aiEnt, "attackDelay" ) )	// Attack?
	{
		TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 3000 ) );
		Remote_Fire(aiEnt);
	}

	if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Remote_Hunt(aiEnt, visible, advance, retreat );
	}
}

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		80
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

/*
-------------------------
Remote_Attack
-------------------------
*/
void Remote_Attack(gentity_t *aiEnt)
{
	float		distance;
	qboolean	visible;
	float		idealDist;
	qboolean	advance, retreat;

	if ( TIMER_Done(aiEnt,"spin") )
	{
		TIMER_Set( aiEnt, "spin", Q_irand( 250, 1500 ) );
		aiEnt->NPC->desiredYaw += Q_irand( -200, 200 );
	}
	// Always keep a good height off the ground
	Remote_MaintainHeight(aiEnt);

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )
	{
		Remote_Idle(aiEnt);
		return;
	}

	// Rate our distance to the target, and our visibilty
	distance	= (int) DistanceHorizontalSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4(aiEnt, aiEnt->enemy );
	idealDist	= MIN_DISTANCE_SQR+(MIN_DISTANCE_SQR*flrand( 0, 1 ));
	advance		= (qboolean)(distance > idealDist*1.25);
	retreat		= (qboolean)(distance < idealDist*0.75);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
		{
			Remote_Hunt(aiEnt, visible, advance, retreat );
			return;
		}
	}

	Remote_Ranged(aiEnt, visible, advance, retreat );

}

/*
-------------------------
Remote_Idle
-------------------------
*/
void Remote_Idle(gentity_t *aiEnt)
{
	Remote_MaintainHeight(aiEnt);

	NPC_BSIdle(aiEnt);
}

/*
-------------------------
Remote_Patrol
-------------------------
*/
void Remote_Patrol(gentity_t *aiEnt)
{
	Remote_MaintainHeight(aiEnt);

	//If we have somewhere to go, then do that
	if (!aiEnt->enemy)
	{
		if ( UpdateGoal(aiEnt) )
		{
			//start loop sound once we move
			aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal(aiEnt, qtrue );
		}
	}

	NPC_UpdateAngles(aiEnt,  qtrue, qtrue );
}


/*
-------------------------
NPC_BSRemote_Default
-------------------------
*/
void NPC_BSRemote_Default(gentity_t *aiEnt)
{
	if ( aiEnt->enemy )
		Remote_Attack(aiEnt);
	else if ( aiEnt->NPC->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		Remote_Patrol(aiEnt);
	else
		Remote_Idle(aiEnt);
}
