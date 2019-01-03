#include "b_local.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		54
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		128
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

/*
-------------------------
NPC_MineMonster_Precache
-------------------------
*/
void NPC_MineMonster_Precache( void )
{
	int i;

	for ( i = 0; i < 4; i++ )
	{
		G_SoundIndex( va("sound/chars/mine/misc/bite%i.wav", i+1 ));
		G_SoundIndex( va("sound/chars/mine/misc/miss%i.wav", i+1 ));
	}
}


/*
-------------------------
MineMonster_Idle
-------------------------
*/
void MineMonster_Idle(gentity_t *aiEnt)
{
	if ( UpdateGoal(aiEnt) )
	{
		aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal(aiEnt, qtrue );
	}
}


/*
-------------------------
MineMonster_Patrol
-------------------------
*/
void MineMonster_Patrol(gentity_t *aiEnt)
{
	vec3_t dif;

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

	//rwwFIXMEFIXME: Care about all clients, not just client 0
	//OJKFIXME: clietnum 0
	VectorSubtract( g_entities[0].r.currentOrigin, aiEnt->r.currentOrigin, dif );

	if ( VectorLengthSquared( dif ) < 256 * 256 )
	{
		G_SetEnemy( aiEnt, &g_entities[0] );
	}

	if ( NPC_CheckEnemyExt(aiEnt, qtrue ) == qfalse )
	{
		MineMonster_Idle(aiEnt);
		return;
	}
}

/*
-------------------------
MineMonster_Move
-------------------------
*/
void MineMonster_Move(gentity_t *aiEnt, qboolean visible )
{
	if ( aiEnt->NPC->localState != LSTATE_WAITING )
	{
		aiEnt->NPC->goalEntity = aiEnt->enemy;
		NPC_MoveToGoal(aiEnt, qtrue );
		aiEnt->NPC->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
void MineMonster_TryDamage(gentity_t *aiEnt, gentity_t *enemy, int damage )
{
	vec3_t	end, dir;
	trace_t	tr;

	if ( !enemy )
	{
		return;
	}

	AngleVectors( aiEnt->client->ps.viewangles, dir, NULL, NULL );
	VectorMA( aiEnt->r.currentOrigin, MIN_DISTANCE, dir, end );

	// Should probably trace from the mouth, but, ah well.
	trap->Trace( &tr, aiEnt->r.currentOrigin, vec3_origin, vec3_origin, end, aiEnt->s.number, MASK_SHOT, qfalse, 0, 0 );

	if ( tr.entityNum >= 0 && tr.entityNum < ENTITYNUM_NONE )
	{
		G_Damage( &g_entities[tr.entityNum], aiEnt, aiEnt, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
		G_Sound( aiEnt, CHAN_AUTO, G_EffectIndex(va("sound/chars/mine/misc/bite%i.wav", Q_irand(1,4))));
	}
	else
	{
		G_Sound( aiEnt, CHAN_AUTO, G_EffectIndex(va("sound/chars/mine/misc/miss%i.wav", Q_irand(1,4))));
	}
}

//------------------------------
void MineMonster_Attack(gentity_t *aiEnt)
{
	if ( !TIMER_Exists( aiEnt, "attacking" ))
	{
		// usually try and play a jump attack if the player somehow got above them....or just really rarely
		if ( aiEnt->enemy && ((aiEnt->enemy->r.currentOrigin[2] - aiEnt->r.currentOrigin[2] > 10 && random() > 0.1f )
						|| random() > 0.8f ))
		{
			// Going to do ATTACK4
			TIMER_Set( aiEnt, "attacking", 1750 + random() * 200 );
			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_ATTACK4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

			TIMER_Set( aiEnt, "attack2_dmg", 950 ); // level two damage
		}
		else if ( random() > 0.5f )
		{
			if ( random() > 0.8f )
			{
				// Going to do ATTACK3, (rare)
				TIMER_Set( aiEnt, "attacking", 850 );
				NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_ATTACK3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

				TIMER_Set( aiEnt, "attack2_dmg", 400 ); // level two damage
			}
			else
			{
				// Going to do ATTACK1
				TIMER_Set( aiEnt, "attacking", 850 );
				NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

				TIMER_Set( aiEnt, "attack1_dmg", 450 ); // level one damage
			}
		}
		else
		{
			// Going to do ATTACK2
			TIMER_Set( aiEnt, "attacking", 1250 );
			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

			TIMER_Set( aiEnt, "attack1_dmg", 700 ); // level one damage
		}
	}
	else
	{
		// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
		if ( TIMER_Done2( aiEnt, "attack1_dmg", qtrue ))
		{
			MineMonster_TryDamage(aiEnt, aiEnt->enemy, 5 );
		}
		else if ( TIMER_Done2( aiEnt, "attack2_dmg", qtrue ))
		{
			MineMonster_TryDamage(aiEnt, aiEnt->enemy, 10 );
		}
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( aiEnt, "attacking", qtrue );
}

//----------------------------------
void MineMonster_Combat(gentity_t *aiEnt)
{
	float distance;
	qboolean advance;

	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS4(aiEnt, aiEnt->enemy ) || UpdateGoal(aiEnt))
	{
		aiEnt->NPC->combatMove = qtrue;
		aiEnt->NPC->goalEntity = aiEnt->enemy;
		aiEnt->NPC->goalRadius = MAX_DISTANCE;	// just get us within combat range

		NPC_MoveToGoal(aiEnt, qtrue );
		return;
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(aiEnt, qtrue );

	distance	= DistanceHorizontalSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );

	advance = (qboolean)( distance > MIN_DISTANCE_SQR ? qtrue : qfalse  );

	if (( advance || aiEnt->NPC->localState == LSTATE_WAITING ) && TIMER_Done( aiEnt, "attacking" )) // waiting monsters can't attack
	{
		if ( TIMER_Done2( aiEnt, "takingPain", qtrue ))
		{
			aiEnt->NPC->localState = LSTATE_CLEAR;
		}
		else
		{
			MineMonster_Move(aiEnt, qtrue );
		}
	}
	else
	{
		MineMonster_Attack(aiEnt);
	}
}

/*
-------------------------
NPC_MineMonster_Pain
-------------------------
*/
void NPC_MineMonster_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	G_AddEvent( self, EV_PAIN, floor((float)self->health/self->client->pers.maxHealth*100.0f) );

	if ( damage >= 10 )
	{
		TIMER_Remove( self, "attacking" );
		TIMER_Remove( self, "attacking1_dmg" );
		TIMER_Remove( self, "attacking2_dmg" );
		TIMER_Set( self, "takingPain", 1350 );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		if ( self->NPC )
		{
			self->NPC->localState = LSTATE_WAITING;
		}
	}
}


/*
-------------------------
NPC_BSMineMonster_Default
-------------------------
*/
void NPC_BSMineMonster_Default(gentity_t *aiEnt)
{
	if ( aiEnt->enemy )
	{
		MineMonster_Combat(aiEnt);
	}
	else if ( aiEnt->NPC->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		MineMonster_Patrol(aiEnt);
	}
	else
	{
		MineMonster_Idle(aiEnt);
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}
