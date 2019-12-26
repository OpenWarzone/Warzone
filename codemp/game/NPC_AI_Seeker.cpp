#include "b_local.h"
#include "g_nav.h"

extern void Boba_FireDecide(gentity_t *aiEnt);
extern qboolean Boba_DoFlameThrower(gentity_t *self);

void Seeker_Strafe(gentity_t *aiEnt);

#define VELOCITY_DECAY		0.7f

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		80
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define SEEKER_STRAFE_VEL	100
#define SEEKER_STRAFE_DIS	200
#define SEEKER_UPWARD_PUSH	32

#define SEEKER_FORWARD_BASE_SPEED	10
#define SEEKER_FORWARD_MULTIPLIER	2

#define SEEKER_SEEK_RADIUS			1024

//------------------------------------
void NPC_Seeker_Precache(void)
{
	G_SoundIndex("sound/chars/seeker/misc/fire.wav");
	G_SoundIndex( "sound/chars/seeker/misc/hiss.wav");
	G_EffectIndex( "env/small_explode");
}

//------------------------------------
void NPC_Seeker_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	if ( !(self->NPC->aiFlags&NPCAI_CUSTOM_GRAVITY ))
	{
		G_Damage( self, NULL, NULL, (float*)vec3_origin, (float*)vec3_origin, 999, 0, MOD_FALLING );
	}

	Seeker_Strafe(self);
	NPC_Pain( self, attacker, damage );
}

//------------------------------------
void Seeker_MaintainHeight( gentity_t *aiEnt)
{
	float	dif;

	// Update our angles regardless
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if ( aiEnt->enemy )
	{
		if (TIMER_Done( aiEnt, "heightChange" ))
		{
			float difFactor;

			TIMER_Set( aiEnt,"heightChange",Q_irand( 1000, 3000 ));

			// Find the height difference
			dif = (aiEnt->enemy->r.currentOrigin[2] +  flrand( aiEnt->enemy->r.maxs[2]/2, aiEnt->enemy->r.maxs[2]+8 )) - aiEnt->r.currentOrigin[2];

			difFactor = 1.0f;
			if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
			{
				if ( TIMER_Done( aiEnt, "flameTime" ) )
				{
					difFactor = 10.0f;
				}
			}

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 2*difFactor )
			{
				if ( fabs( dif ) > 24*difFactor )
				{
					dif = ( dif < 0 ? -24*difFactor : 24*difFactor );
				}

				aiEnt->client->ps.velocity[2] = (aiEnt->client->ps.velocity[2]+dif)/2;
			}
			if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
			{
				aiEnt->client->ps.velocity[2] *= flrand( 0.85f, 3.0f );
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

//------------------------------------
void Seeker_Strafe(gentity_t *aiEnt)
{
	int		side;
	vec3_t	end, right, dir;
	trace_t	tr;

	if ( random() > 0.7f || !aiEnt->enemy || !aiEnt->enemy->client )
	{
		// Do a regular style strafe
		AngleVectors( aiEnt->client->renderInfo.eyeAngles, NULL, right, NULL );

		// Pick a random strafe direction, then check to see if doing a strafe would be
		//	reasonably valid
		side = ( rand() & 1 ) ? -1 : 1;
		VectorMA( aiEnt->r.currentOrigin, SEEKER_STRAFE_DIS * side, right, end );

		trap->Trace( &tr, aiEnt->r.currentOrigin, NULL, NULL, end, aiEnt->s.number, MASK_SOLID, qfalse, 0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float vel = SEEKER_STRAFE_VEL;
			float upPush = SEEKER_UPWARD_PUSH;
			if ( aiEnt->client->NPC_class != CLASS_BOBAFETT )
			{
				G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
			else
			{
				vel *= 3.0f;
				upPush *= 4.0f;
			}
			VectorMA( aiEnt->client->ps.velocity, vel*side, right, aiEnt->client->ps.velocity );
			// Add a slight upward push
			aiEnt->client->ps.velocity[2] += upPush;

			aiEnt->NPC->standTime = level.time + 1000 + random() * 500;
		}
	}
	else
	{
		float stDis;

		// Do a strafe to try and keep on the side of their enemy
		AngleVectors( aiEnt->enemy->client->renderInfo.eyeAngles, dir, right, NULL );

		// Pick a random side
		side = ( rand() & 1 ) ? -1 : 1;
		stDis = SEEKER_STRAFE_DIS;
		if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
		{
			stDis *= 2.0f;
		}
		VectorMA( aiEnt->enemy->r.currentOrigin, stDis * side, right, end );

		// then add a very small bit of random in front of/behind the player action
		VectorMA( end, crandom() * 25, dir, end );

		trap->Trace( &tr, aiEnt->r.currentOrigin, NULL, NULL, end, aiEnt->s.number, MASK_SOLID, qfalse, 0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float dis, upPush;

			VectorSubtract( tr.endpos, aiEnt->r.currentOrigin, dir );
			dir[2] *= 0.25; // do less upward change
			dis = VectorNormalize( dir );

			// Try to move the desired enemy side
			VectorMA( aiEnt->client->ps.velocity, dis, dir, aiEnt->client->ps.velocity );

			upPush = SEEKER_UPWARD_PUSH;
			if ( aiEnt->client->NPC_class != CLASS_BOBAFETT )
			{
				G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
			else
			{
				upPush *= 4.0f;
			}

			// Add a slight upward push
			aiEnt->client->ps.velocity[2] += upPush;

			aiEnt->NPC->standTime = level.time + 2500 + random() * 500;
		}
	}
}

//------------------------------------
void Seeker_Hunt(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	NPC_FaceEnemy(aiEnt, qtrue );

	// If we're not supposed to stand still, pursue the player
	if ( aiEnt->NPC->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Seeker_Strafe(aiEnt);
			return;
		}
	}

	// If we don't want to advance, stop here
	if ( advance == qfalse )
	{
		return;
	}

	// Only try and navigate if the player is visible
	if ( visible == qfalse )
	{
		// Move towards our goal
		aiEnt->NPC->goalEntity = aiEnt->enemy;
		aiEnt->NPC->goalRadius = 24;

		// Get our direction from the navigator if we can't see our target
		if ( NPC_GetMoveDirection(aiEnt, forward, &distance ) == qfalse )
		{
			return;
		}
	}
	else
	{
		VectorSubtract( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, forward );
		/*distance = */VectorNormalize( forward );
	}

	speed = SEEKER_FORWARD_BASE_SPEED + SEEKER_FORWARD_MULTIPLIER * g_npcspskill.integer;
	VectorMA( aiEnt->client->ps.velocity, speed, forward, aiEnt->client->ps.velocity );
}

//------------------------------------
void Seeker_Fire(gentity_t *aiEnt)
{
	vec3_t		dir, enemy_org, muzzle;
	gentity_t	*missile;

	CalcEntitySpot( aiEnt->enemy, SPOT_HEAD, enemy_org );
	VectorSubtract( enemy_org, aiEnt->r.currentOrigin, dir );
	VectorNormalize( dir );

	// move a bit forward in the direction we shall shoot in so that the bolt doesn't poke out the other side of the seeker
	VectorMA( aiEnt->r.currentOrigin, 15, dir, muzzle );

	missile = CreateMissile( muzzle, dir, 1000, 10000, aiEnt, qfalse );

	G_PlayEffectID( G_EffectIndex("blaster/muzzle_flash"), aiEnt->r.currentOrigin, dir );

	missile->classname = "blaster";
	missile->s.weapon = WP_MODULIZED_WEAPON;

	missile->damage = 5;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	if ( aiEnt->r.ownerNum < ENTITYNUM_NONE )
	{
		missile->r.ownerNum = aiEnt->r.ownerNum;
	}
}

//------------------------------------
void Seeker_Ranged(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	if ( aiEnt->client->NPC_class != CLASS_BOBAFETT )
	{
		if ( aiEnt->count > 0 )
		{
			if ( TIMER_Done( aiEnt, "attackDelay" ))	// Attack?
			{
				TIMER_Set( aiEnt, "attackDelay", Q_irand( 250, 2500 ));
				Seeker_Fire(aiEnt);
				aiEnt->count--;
			}
		}
		else
		{
			// out of ammo, so let it die...give it a push up so it can fall more and blow up on impact
	//		NPC->client->ps.gravity = 900;
	//		NPC->svFlags &= ~SVF_CUSTOM_GRAVITY;
	//		NPC->client->ps.velocity[2] += 16;
			G_Damage( aiEnt, aiEnt, aiEnt, NULL, NULL, 999, 0, MOD_UNKNOWN );
		}
	}

	if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Seeker_Hunt(aiEnt, visible, advance );
	}
}

//------------------------------------
void Seeker_Attack(gentity_t *aiEnt)
{
	float		distance;
	qboolean	visible, advance;

	// Always keep a good height off the ground
	Seeker_MaintainHeight(aiEnt);

	// Rate our distance to the target, and our visibilty
	distance	= DistanceHorizontalSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4(aiEnt,  aiEnt->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
	{
		advance = (qboolean)(distance>(200.0f*200.0f));
	}

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
		{
			Seeker_Hunt(aiEnt, visible, advance );
			return;
		}
	}

	if (aiEnt->client->NPC_class == CLASS_GLIDER)
	{// UQ1: Meh, this will do for now...
		Boba_DoFlameThrower(aiEnt);
	}
	else
	{
		Seeker_Ranged(aiEnt, visible, advance);
	}
}

//------------------------------------
void Seeker_FindEnemy(gentity_t *aiEnt)
{
	int			numFound;
	float		dis, bestDis = SEEKER_SEEK_RADIUS * SEEKER_SEEK_RADIUS + 1;
	vec3_t		mins, maxs;
	int			entityList[MAX_GENTITIES];
	gentity_t	*ent, *best = NULL;
	int			i;

	VectorSet( maxs, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS );
	VectorScale( maxs, -1, mins );

	numFound = trap->EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0 ; i < numFound ; i++ )
	{
		ent = &g_entities[entityList[i]];

		if ( ent->s.number == aiEnt->s.number
			|| !ent->client //&& || !ent->NPC
			|| ent->health <= 0
			|| !ent->inuse )
		{
			continue;
		}

		if ( ent->client->playerTeam == aiEnt->client->playerTeam || ent->client->playerTeam == NPCTEAM_NEUTRAL ) // don't attack same team or bots
		{
			continue;
		}

		// try to find the closest visible one
		if ( !NPC_ClearLOS4(aiEnt, ent ))
		{
			continue;
		}

		dis = DistanceHorizontalSquared( aiEnt->r.currentOrigin, ent->r.currentOrigin );

		if ( dis <= bestDis )
		{
			bestDis = dis;
			best = ent;
		}
	}

	if ( best )
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		aiEnt->random = random() * 6.3f; // roughly 2pi

		aiEnt->enemy = best;
	}
}

//------------------------------------
void Seeker_FollowOwner(gentity_t *aiEnt)
{
	float	dis, minDistSqr;
	vec3_t	pt, dir;
	gentity_t	*owner = &g_entities[aiEnt->s.owner];

	Seeker_MaintainHeight(aiEnt);

	if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
	{
		owner = aiEnt->enemy;
	}
	if ( !owner || owner == aiEnt || !owner->client )
	{
		return;
	}
	//rwwFIXMEFIXME: Care about all clients not just 0
	dis	= DistanceHorizontalSquared( aiEnt->r.currentOrigin, owner->r.currentOrigin );

	minDistSqr = MIN_DISTANCE_SQR;

	if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( TIMER_Done( aiEnt, "flameTime" ) )
		{
			minDistSqr = 200*200;
		}
	}

	if ( dis < minDistSqr )
	{
		// generally circle the player closely till we take an enemy..this is our target point
		if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
		{
			pt[0] = owner->r.currentOrigin[0] + cos( level.time * 0.001f + aiEnt->random ) * 250;
			pt[1] = owner->r.currentOrigin[1] + sin( level.time * 0.001f + aiEnt->random ) * 250;
			if ( aiEnt->client->jetPackTime < level.time )
			{
				pt[2] = aiEnt->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = owner->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = owner->r.currentOrigin[0] + cos( level.time * 0.001f + aiEnt->random ) * 56;
			pt[1] = owner->r.currentOrigin[1] + sin( level.time * 0.001f + aiEnt->random ) * 56;
			pt[2] = owner->r.currentOrigin[2] + 40;
		}

		VectorSubtract( pt, aiEnt->r.currentOrigin, dir );
		VectorMA( aiEnt->client->ps.velocity, 0.8f, dir, aiEnt->client->ps.velocity );
	}
	else
	{
		if ( aiEnt->client->NPC_class != CLASS_BOBAFETT )
		{
			if ( TIMER_Done( aiEnt, "seekerhiss" ))
			{
				TIMER_Set( aiEnt, "seekerhiss", 1000 + random() * 1000 );
				G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
		}

		// Hey come back!
		aiEnt->NPC->goalEntity = owner;
		aiEnt->NPC->goalRadius = 32;
		NPC_MoveToGoal(aiEnt, qtrue );
		aiEnt->parent = owner;
	}

	if ( aiEnt->NPC->enemyCheckDebounceTime < level.time )
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy(aiEnt);
		aiEnt->NPC->enemyCheckDebounceTime = level.time + 500;
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

//------------------------------------
void NPC_BSSeeker_Default(gentity_t *aiEnt)
{
	/*
	if ( in_camera )
	{
		if ( NPC->client->NPC_class != CLASS_BOBAFETT )
		{
			// cameras make me commit suicide....
			G_Damage( NPC, NPC, NPC, NULL, NULL, 999, 0, MOD_UNKNOWN );
		}
	}
	*/
	//N/A for MP.
	if ( aiEnt->r.ownerNum < ENTITYNUM_NONE )
	{
		//OJKFIXME: clientnum 0
		gentity_t *owner = &g_entities[0];
		if ( owner->health <= 0
			|| (owner->client && owner->client->pers.connected == CON_DISCONNECTED) )
		{//owner is dead or gone
			//remove me
			G_Damage( aiEnt, NULL, NULL, NULL, NULL, 10000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
			return;
		}
	}

	if ( aiEnt->random == 0.0f )
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		aiEnt->random = random() * 6.3f; // roughly 2pi
	}

	if ( aiEnt->enemy && aiEnt->enemy->health && aiEnt->enemy->inuse )
	{
		//OJKFIXME: clientnum 0
		/*if ( aiEnt->client->NPC_class != CLASS_BOBAFETT && ( aiEnt->enemy->s.number == 0 || ( aiEnt->enemy->client && aiEnt->enemy->client->NPC_class == CLASS_SEEKER )) )
		{
			//hacked to never take the player as an enemy, even if the player shoots at it
			aiEnt->enemy = NULL;
		}
		else*/
		{
			Seeker_Attack(aiEnt);
			if ( aiEnt->client->NPC_class == CLASS_BOBAFETT )
			{
				Boba_FireDecide(aiEnt);
			}
			return;
		}
	}

	// In all other cases, follow the player and look for enemies to take on
	Seeker_FollowOwner(aiEnt);
}
