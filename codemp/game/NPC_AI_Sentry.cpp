#include "b_local.h"
#include "g_nav.h"

extern gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

#define MIN_DISTANCE		256
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define SENTRY_FORWARD_BASE_SPEED	10
#define SENTRY_FORWARD_MULTIPLIER	5

#define SENTRY_VELOCITY_DECAY	0.85f
#define SENTRY_STRAFE_VEL		256
#define SENTRY_STRAFE_DIS		200
#define SENTRY_UPWARD_PUSH		32
#define SENTRY_HOVER_HEIGHT		24

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_ASLEEP,
	LSTATE_WAKEUP,
	LSTATE_ACTIVE,
	LSTATE_POWERING_UP,
	LSTATE_ATTACKING,
};

/*
-------------------------
NPC_Sentry_Precache
-------------------------
*/
void NPC_Sentry_Precache(void)
{
	int i;

	G_SoundIndex( "sound/chars/sentry/misc/sentry_explo" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_pain" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_shield_open" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_shield_close" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_1_lp" );
	G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_2_lp" );

	for ( i = 1; i < 4; i++)
	{
		G_SoundIndex( va( "sound/chars/sentry/misc/talk%d", i ) );
	}

	G_EffectIndex( "bryar/muzzle_flash");
	G_EffectIndex( "env/med_explode");

#ifndef __MMO__
	RegisterItem( BG_FindItemForAmmo( AMMO_BLASTER ));
#endif //__MMO__
}

/*
================
sentry_use
================
*/
void sentry_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	self->flags &= ~FL_SHIELDED;
	NPC_SetAnim( self, SETANIM_BOTH, BOTH_POWERUP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
//	self->NPC->localState = LSTATE_WAKEUP;
	self->NPC->localState = LSTATE_ACTIVE;
}

/*
-------------------------
NPC_Sentry_Pain
-------------------------
*/
void NPC_Sentry_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	int mod = gPainMOD;

	NPC_Pain( self, attacker, damage );

	if ( mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT )
	{
		self->NPC->burstCount = 0;
		TIMER_Set( self, "attackDelay", Q_irand( 9000, 12000) );
		self->flags |= FL_SHIELDED;
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_FLY_SHIELDED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/chars/sentry/misc/sentry_pain") );

		self->NPC->localState = LSTATE_ACTIVE;
	}

	// You got hit, go after the enemy
//	if (self->NPC->localState == LSTATE_ASLEEP)
//	{
//		G_Sound( self, G_SoundIndex("sound/chars/sentry/misc/shieldsopen.wav"));
//
//		self->flags &= ~FL_SHIELDED;
//		NPC_SetAnim( self, SETANIM_BOTH, BOTH_POWERUP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
//		self->NPC->localState = LSTATE_WAKEUP;
//	}
}

/*
-------------------------
Sentry_Fire
-------------------------
*/
void Sentry_Fire (gentity_t *aiEnt)
{
	vec3_t	muzzle;
	static	vec3_t	forward, vright, up;
	gentity_t	*missile;
	mdxaBone_t	boltMatrix;
	int			bolt, which;

	aiEnt->flags &= ~FL_SHIELDED;

	if ( aiEnt->NPC->localState == LSTATE_POWERING_UP )
	{
		if ( TIMER_Done( aiEnt, "powerup" ))
		{
			aiEnt->NPC->localState = LSTATE_ATTACKING;
			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else
		{
			// can't do anything right now
			return;
		}
	}
	else if ( aiEnt->NPC->localState == LSTATE_ACTIVE )
	{
		aiEnt->NPC->localState = LSTATE_POWERING_UP;

		G_Sound( aiEnt, CHAN_AUTO, G_SoundIndex("sound/chars/sentry/misc/sentry_shield_open") );
		NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_POWERUP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		TIMER_Set( aiEnt, "powerup", 250 );
		return;
	}
	else if ( aiEnt->NPC->localState != LSTATE_ATTACKING )
	{
		// bad because we are uninitialized
		aiEnt->NPC->localState = LSTATE_ACTIVE;
		return;
	}

	// Which muzzle to fire from?
	which = aiEnt->NPC->burstCount % 3;
	switch( which )
	{
	case 0:
		bolt = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "*flash1");
		break;
	case 1:
		bolt = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "*flash2");
		break;
	case 2:
	default:
		bolt = trap->G2API_AddBolt(aiEnt->ghoul2, 0, "*flash03");
	}

	trap->G2API_GetBoltMatrix( aiEnt->ghoul2, 0,
				bolt,
				&boltMatrix, aiEnt->r.currentAngles, aiEnt->r.currentOrigin, level.time,
				NULL, aiEnt->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, muzzle );

	AngleVectors( aiEnt->r.currentAngles, forward, vright, up );
//	G_Sound( NPC, G_SoundIndex("sound/chars/sentry/misc/shoot.wav"));

	G_PlayEffectID( G_EffectIndex("bryar/muzzle_flash"), muzzle, forward );

	missile = CreateMissile( muzzle, forward, 1600, 10000, aiEnt, qfalse );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_MODULIZED_WEAPON;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	aiEnt->NPC->burstCount++;
	aiEnt->attackDebounceTime = level.time + 50;
	missile->damage = 5;

	// now scale for difficulty
	if ( g_npcspskill.integer == 0 )
	{
		aiEnt->attackDebounceTime += 200;
		missile->damage = 1;
	}
	else if ( g_npcspskill.integer == 1 )
	{
		aiEnt->attackDebounceTime += 100;
		missile->damage = 3;
	}
}

/*
-------------------------
Sentry_MaintainHeight
-------------------------
*/
void Sentry_MaintainHeight(gentity_t *aiEnt)
{
	float	dif;

	aiEnt->s.loopSound = G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_1_lp" );

	// Update our angles regardless
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	// If we have an enemy, we should try to hover at about enemy eye level
	if ( aiEnt->enemy )
	{
		// Find the height difference
		dif = (aiEnt->enemy->r.currentOrigin[2]+aiEnt->enemy->r.maxs[2]) - aiEnt->r.currentOrigin[2];

		// cap to prevent dramatic height shifts
		if ( fabs( dif ) > 8 )
		{
			if ( fabs( dif ) > SENTRY_HOVER_HEIGHT )
			{
				dif = ( dif < 0 ? -24 : 24 );
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

		if (goal)
		{
			dif = goal->r.currentOrigin[2] - aiEnt->r.currentOrigin[2];

			if ( fabs( dif ) > SENTRY_HOVER_HEIGHT )
			{
				aiEnt->client->pers.cmd.upmove = ( aiEnt->client->pers.cmd.upmove < 0 ? -4 : 4 );
			}
			else
			{
				if ( aiEnt->client->ps.velocity[2] )
				{
					aiEnt->client->ps.velocity[2] *= SENTRY_VELOCITY_DECAY;

					if ( fabs( aiEnt->client->ps.velocity[2] ) < 2 )
					{
						aiEnt->client->ps.velocity[2] = 0;
					}
				}
			}
		}
		// Apply friction to Z
		else if ( aiEnt->client->ps.velocity[2] )
		{
			aiEnt->client->ps.velocity[2] *= SENTRY_VELOCITY_DECAY;

			if ( fabs( aiEnt->client->ps.velocity[2] ) < 1 )
			{
				aiEnt->client->ps.velocity[2] = 0;
			}
		}
	}

	// Apply friction
	if ( aiEnt->client->ps.velocity[0] )
	{
		aiEnt->client->ps.velocity[0] *= SENTRY_VELOCITY_DECAY;

		if ( fabs( aiEnt->client->ps.velocity[0] ) < 1 )
		{
			aiEnt->client->ps.velocity[0] = 0;
		}
	}

	if ( aiEnt->client->ps.velocity[1] )
	{
		aiEnt->client->ps.velocity[1] *= SENTRY_VELOCITY_DECAY;

		if ( fabs( aiEnt->client->ps.velocity[1] ) < 1 )
		{
			aiEnt->client->ps.velocity[1] = 0;
		}
	}

	NPC_FaceEnemy(aiEnt, qtrue );
}

/*
-------------------------
Sentry_Idle
-------------------------
*/
void Sentry_Idle(gentity_t *aiEnt)
{
	Sentry_MaintainHeight(aiEnt);

	// Is he waking up?
	if (aiEnt->NPC->localState == LSTATE_WAKEUP)
	{
		if (aiEnt->client->ps.torsoTimer<=0)
		{
			aiEnt->NPC->scriptFlags |= SCF_LOOK_FOR_ENEMIES;
			aiEnt->NPC->burstCount = 0;
		}
	}
	else
	{
		NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_SLEEP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		aiEnt->flags |= FL_SHIELDED;

		NPC_BSIdle(aiEnt);
	}
}

/*
-------------------------
Sentry_Strafe
-------------------------
*/
void Sentry_Strafe(gentity_t *aiEnt)
{
	int		dir;
	vec3_t	end, right;
	trace_t	tr;

	AngleVectors( aiEnt->client->renderInfo.eyeAngles, NULL, right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = ( rand() & 1 ) ? -1 : 1;
	VectorMA( aiEnt->r.currentOrigin, SENTRY_STRAFE_DIS * dir, right, end );

	trap->Trace( &tr, aiEnt->r.currentOrigin, NULL, NULL, end, aiEnt->s.number, MASK_SOLID, qfalse, 0, 0 );

	// Close enough
	if ( tr.fraction > 0.9f )
	{
		VectorMA( aiEnt->client->ps.velocity, SENTRY_STRAFE_VEL * dir, right, aiEnt->client->ps.velocity );

		// Add a slight upward push
		aiEnt->client->ps.velocity[2] += SENTRY_UPWARD_PUSH;

		// Set the strafe start time so we can do a controlled roll
	//	NPC->fx_time = level.time;
		aiEnt->NPC->standTime = level.time + 3000 + random() * 500;
	}
}

/*
-------------------------
Sentry_Hunt
-------------------------
*/
void Sentry_Hunt(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	//If we're not supposed to stand still, pursue the player
	if ( aiEnt->NPC->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Sentry_Strafe(aiEnt);
			return;
		}
	}

	//If we don't want to advance, stop here
	if ( !advance && visible )
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

	speed = SENTRY_FORWARD_BASE_SPEED + SENTRY_FORWARD_MULTIPLIER * g_npcspskill.integer;
	VectorMA( aiEnt->client->ps.velocity, speed, forward, aiEnt->client->ps.velocity );
}

/*
-------------------------
Sentry_RangedAttack
-------------------------
*/
void Sentry_RangedAttack(gentity_t *aiEnt, qboolean visible, qboolean advance )
{
	if ( TIMER_Done( aiEnt, "attackDelay" ) && aiEnt->attackDebounceTime < level.time && visible )	// Attack?
	{
		if ( aiEnt->NPC->burstCount > 6 )
		{
			if ( !aiEnt->fly_sound_debounce_time )
			{//delay closing down to give the player an opening
				aiEnt->fly_sound_debounce_time = level.time + Q_irand( 500, 2000 );
			}
			else if ( aiEnt->fly_sound_debounce_time < level.time )
			{
				aiEnt->NPC->localState = LSTATE_ACTIVE;
				aiEnt->fly_sound_debounce_time = aiEnt->NPC->burstCount = 0;
				TIMER_Set( aiEnt, "attackDelay", Q_irand( 2000, 3500) );
				aiEnt->flags |= FL_SHIELDED;
				NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_FLY_SHIELDED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				G_SoundOnEnt( aiEnt, CHAN_AUTO, "sound/chars/sentry/misc/sentry_shield_close" );
			}
		}
		else
		{
			Sentry_Fire(aiEnt);
		}
	}

	if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Sentry_Hunt(aiEnt, visible, advance );
	}
}

/*
-------------------------
Sentry_AttackDecision
-------------------------
*/
void Sentry_AttackDecision(gentity_t *aiEnt)
{
	float		distance;
	qboolean	visible, advance;

	// Always keep a good height off the ground
	Sentry_MaintainHeight(aiEnt);

	aiEnt->s.loopSound = G_SoundIndex( "sound/chars/sentry/misc/sentry_hover_2_lp" );

	//randomly talk
	if ( TIMER_Done(aiEnt,"patrolNoise") )
	{
		if (TIMER_Done(aiEnt,"angerNoise"))
		{
			G_SoundOnEnt( aiEnt, CHAN_AUTO, va("sound/chars/sentry/misc/talk%d", Q_irand(1, 3)) );

			TIMER_Set( aiEnt, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// He's dead.
	if (aiEnt->enemy->health<1)
	{
		aiEnt->enemy = NULL;
		Sentry_Idle(aiEnt);
		return;
	}

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )
	{
		Sentry_Idle(aiEnt);
		return;
	}

	// Rate our distance to the target and visibilty
	distance	= (int) DistanceHorizontalSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
	visible		= NPC_ClearLOS4(aiEnt, aiEnt->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
		{
			Sentry_Hunt(aiEnt, visible, advance );
			return;
		}
	}

	NPC_FaceEnemy(aiEnt, qtrue );

	Sentry_RangedAttack(aiEnt, visible, advance );
}

qboolean NPC_CheckPlayerTeamStealth(gentity_t *aiEnt);

/*
-------------------------
NPC_Sentry_Patrol
-------------------------
*/
void NPC_Sentry_Patrol(gentity_t *aiEnt)
{
	Sentry_MaintainHeight(aiEnt);

	//If we have somewhere to go, then do that
	if (!aiEnt->enemy)
	{
		if ( NPC_CheckPlayerTeamStealth(aiEnt) )
		{
			//NPC_AngerSound();
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return;
		}

		if ( UpdateGoal(aiEnt) )
		{
			//start loop sound once we move
			aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal(aiEnt, qtrue );
		}

		//randomly talk
		if (TIMER_Done(aiEnt,"patrolNoise"))
		{
			G_SoundOnEnt( aiEnt, CHAN_AUTO, va("sound/chars/sentry/misc/talk%d", Q_irand(1, 3)) );

			TIMER_Set( aiEnt, "patrolNoise", Q_irand( 2000, 4000 ) );
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

/*
-------------------------
NPC_BSSentry_Default
-------------------------
*/
void NPC_BSSentry_Default(gentity_t *aiEnt)
{
	if ( aiEnt->targetname )
	{
		aiEnt->use = sentry_use;
	}

	if (( aiEnt->enemy ) && (aiEnt->NPC->localState != LSTATE_WAKEUP))
	{
		// Don't attack if waking up or if no enemy
		Sentry_AttackDecision(aiEnt);
	}
	else if ( aiEnt->NPC->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		NPC_Sentry_Patrol(aiEnt);
	}
	else
	{
		Sentry_Idle(aiEnt);
	}
}
