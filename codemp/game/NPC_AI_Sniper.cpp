#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

//#define __SNIPER_DEBUG__

extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern qboolean FlyingCreature( gentity_t *ent );

#define	SPF_NO_HIDE			2

#define	MAX_VIEW_DIST		1024
#define MAX_VIEW_SPEED		250
#define	MAX_LIGHT_INTENSITY 255
#define	MIN_LIGHT_THRESHOLD	0.1

#define	DISTANCE_SCALE		0.25f
#define	DISTANCE_THRESHOLD	0.075f
#define	SPEED_SCALE			0.25f
#define	FOV_SCALE			0.5f
#define	LIGHT_SCALE			0.25f

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75 )

qboolean NPC_CheckPlayerTeamStealth(gentity_t *aiEnt);

static qboolean enemyLOS2;
static qboolean enemyCS2;
static qboolean faceEnemy2;
static qboolean move2;
static qboolean shoot2;
static float	enemyDist2;

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void Sniper_ClearTimers( gentity_t *ent )
{
	TIMER_Set( ent, "chatter", 0 );
	TIMER_Set( ent, "duck", 0 );
	TIMER_Set( ent, "stand", 0 );
	TIMER_Set( ent, "shuffleTime", 0 );
	TIMER_Set( ent, "sleepTime", 0 );
	TIMER_Set( ent, "enemyLastVisible", 0 );
	TIMER_Set( ent, "roamTime", 0 );
	TIMER_Set( ent, "hideTime", 0 );
	TIMER_Set( ent, "attackDelay", 0 );	//FIXME: Slant for difficulty levels
	TIMER_Set( ent, "stick", 0 );
	TIMER_Set( ent, "scoutTime", 0 );
	TIMER_Set( ent, "flee", 0 );
}

void NPC_Sniper_PlayConfusionSound( gentity_t *self )
{//FIXME: make this a custom sound in sound set
	if ( self->health > 0 )
	{
		G_AddVoiceEvent( self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000 );
	}
	//reset him to be totally unaware again
	TIMER_Set( self, "enemyLastVisible", 0 );
	TIMER_Set( self, "flee", 0 );
	self->NPC->squadState = SQUAD_IDLE;
	self->NPC->tempBehavior = BS_DEFAULT;

	//self->NPC->behaviorState = BS_PATROL;
	G_ClearEnemy( self );//FIXME: or just self->enemy = NULL;?

	self->NPC->investigateCount = 0;
}


/*
-------------------------
NPC_ST_Pain
-------------------------
*/

void NPC_Sniper_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stand", 2000 );

	NPC_Pain( self, attacker, damage );

	if ( !damage && self->health > 0 )
	{//FIXME: better way to know I was pushed
		G_AddVoiceEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
	}
}

/*
-------------------------
ST_HoldPosition
-------------------------
*/

static void Sniper_HoldPosition(gentity_t *aiEnt)
{
	NPC_FreeCombatPoint( aiEnt, aiEnt->NPC->combatPoint, qtrue );
	aiEnt->NPC->goalEntity = NULL;

	/*if ( TIMER_Done( NPC, "stand" ) )
	{//FIXME: what if can't shoot from this pos?
		TIMER_Set( NPC, "duck", Q_irand( 2000, 4000 ) );
	}
	*/
}

/*
-------------------------
ST_Move
-------------------------
*/

static qboolean Sniper_Move( gentity_t *aiEnt)
{
	qboolean	moved;
	navInfo_t	info;

	aiEnt->NPC->combatMove = qtrue;//always move straight toward our goal

	moved = NPC_MoveToGoal(aiEnt, qtrue );

	//Get the move info
	NAV_GetLastMove( &info );

	//FIXME: if we bump into another one of our guys and can't get around him, just stop!
	//If we hit our target, then stop and fire!
	if ( info.flags & NIF_COLLISION )
	{
		if ( info.blocker == aiEnt->enemy )
		{
			Sniper_HoldPosition(aiEnt);
		}
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{//couldn't get to enemy
		if ( (aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) && aiEnt->NPC->goalEntity && aiEnt->NPC->goalEntity == aiEnt->enemy )
		{//we were running after enemy
			//Try to find a combat point that can hit the enemy
			int cpFlags = (CP_CLEAR|CP_HAS_ROUTE);
			int cp;
			if ( aiEnt->NPC->scriptFlags&SCF_USE_CP_NEAREST )
			{
				cpFlags &= ~(CP_FLANK|CP_APPROACH_ENEMY|CP_CLOSEST);
				cpFlags |= CP_NEAREST;
			}
			cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, cpFlags, 32, -1 );
			if ( cp == -1 && !(aiEnt->NPC->scriptFlags&SCF_USE_CP_NEAREST) )
			{//okay, try one by the enemy
				cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin, CP_CLEAR|CP_HAS_ROUTE|CP_HORZ_DIST_COLL, 32, -1 );
			}
			//NOTE: there may be a perfectly valid one, just not one within CP_COLLECT_RADIUS of either me or him...
			if ( cp != -1 )
			{//found a combat point that has a clear shot to enemy
				NPC_SetCombatPoint(aiEnt, cp );
				NPC_SetMoveGoal( aiEnt, level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
				return moved;
			}
		}
		//just hang here
		Sniper_HoldPosition(aiEnt);
	}

	return moved;
}

/*
-------------------------
NPC_BSSniper_Patrol
-------------------------
*/

void NPC_BSSniper_Patrol( gentity_t *aiEnt)
{//FIXME: pick up on bodies of dead buddies?
	aiEnt->count = 0;

	if ( aiEnt->NPC->confusionTime < level.time )
	{
		//Look for any enemies
		if ( aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			if ( NPC_CheckPlayerTeamStealth(aiEnt) )
			{
				//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//Should be auto now
				//NPC_AngerSound();
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
		}

		if ( !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
		{
			//Is there danger nearby
			int alertEvent = NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_SUSPICIOUS );
			if ( NPC_CheckForDanger(aiEnt, alertEvent ) )
			{
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
			else
			{//check for other alert events
				//There is an event to look at
				if ( alertEvent >= 0 && level.alertEvents[alertEvent].ID != aiEnt->NPC->lastAlertID )
				{
					aiEnt->NPC->lastAlertID = level.alertEvents[alertEvent].ID;
					if ( level.alertEvents[alertEvent].level == AEL_DISCOVERED )
					{
						if ( level.alertEvents[alertEvent].owner &&
							level.alertEvents[alertEvent].owner->client &&
							level.alertEvents[alertEvent].owner->health >= 0 &&
							level.alertEvents[alertEvent].owner->client->playerTeam == aiEnt->client->enemyTeam )
						{//an enemy
							G_SetEnemy( aiEnt, level.alertEvents[alertEvent].owner );
							//NPCInfo->enemyLastSeenTime = level.time;
							TIMER_Set( aiEnt, "attackDelay", Q_irand( (6-aiEnt->NPC->stats.aim)*100, (6-aiEnt->NPC->stats.aim)*500 ) );
						}
					}
					else
					{//FIXME: get more suspicious over time?
						//Save the position for movement (if necessary)
						//FIXME: sound?
						VectorCopy( level.alertEvents[alertEvent].position, aiEnt->NPC->investigateGoal );
						aiEnt->NPC->investigateDebounceTime = level.time + Q_irand( 500, 1000 );
						if ( level.alertEvents[alertEvent].level == AEL_SUSPICIOUS )
						{//suspicious looks longer
							aiEnt->NPC->investigateDebounceTime += Q_irand( 500, 2500 );
						}
					}
				}
			}

			if ( aiEnt->NPC->investigateDebounceTime > level.time )
			{//FIXME: walk over to it, maybe?  Not if not chase enemies flag
				//NOTE: stops walking or doing anything else below
				vec3_t	dir, angles;
				float	o_yaw, o_pitch;

				VectorSubtract( aiEnt->NPC->investigateGoal, aiEnt->client->renderInfo.eyePoint, dir );
				vectoangles( dir, angles );

				o_yaw = aiEnt->NPC->desiredYaw;
				o_pitch = aiEnt->NPC->desiredPitch;
				aiEnt->NPC->desiredYaw = angles[YAW];
				aiEnt->NPC->desiredPitch = angles[PITCH];

				NPC_UpdateAngles(aiEnt, qtrue, qtrue );

				aiEnt->NPC->desiredYaw = o_yaw;
				aiEnt->NPC->desiredPitch = o_pitch;
				return;
			}
		}
	}

	//If we have somewhere to go, then do that
	if ( UpdateGoal(aiEnt) )
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal(aiEnt, qtrue );
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

/*
-------------------------
NPC_BSSniper_Idle
-------------------------
*/
/*
void NPC_BSSniper_Idle( gentity_t *aiEnt )
{
	//reset our shotcount
	NPC->count = 0;

	//FIXME: check for other alert events?

	//Is there danger nearby?
	if ( NPC_CheckForDanger( NPC_CheckAlertEvents( qtrue, qtrue ) ) )
	{
		NPC_UpdateAngles( aiEnt, qtrue, qtrue );
		return;
	}

	TIMER_Set( NPC, "roamTime", 2000 + Q_irand( 1000, 2000 ) );

	NPC_UpdateAngles( aiEnt, qtrue, qtrue );
}
*/
/*
-------------------------
ST_CheckMoveState
-------------------------
*/

static void Sniper_CheckMoveState( gentity_t *aiEnt)
{

	//See if we're a scout
	if ( !(aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES) )//NPCInfo->behaviorState == BS_STAND_AND_SHOOT )
	{
		if ( aiEnt->NPC->goalEntity == aiEnt->enemy )
		{
			move2 = qfalse;
			return;
		}
	}
	//See if we're running away
	else if ( aiEnt->NPC->squadState == SQUAD_RETREAT )
	{
		if ( TIMER_Done( aiEnt, "flee" ) )
		{
			aiEnt->NPC->squadState = SQUAD_IDLE;
		}
		else
		{
			faceEnemy2 = qfalse;
		}
	}
	else if ( aiEnt->NPC->squadState == SQUAD_IDLE )
	{
		if ( !aiEnt->NPC->goalEntity )
		{
			move2 = qfalse;
			return;
		}
	}

	//See if we're moving towards a goal, not the enemy
	if ( ( aiEnt->NPC->goalEntity != aiEnt->enemy ) && ( aiEnt->NPC->goalEntity != NULL ) )
	{
		//Did we make it?
		if ( NAV_HitNavGoal( aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, aiEnt->NPC->goalEntity->r.currentOrigin, 16, FlyingCreature( aiEnt ) ) ||
			( aiEnt->NPC->squadState == SQUAD_SCOUT && enemyLOS2 && enemyDist2 <= 10000 ) )
		{
		//	int	newSquadState = SQUAD_STAND_AND_SHOOT;
			//we got where we wanted to go, set timers based on why we were running
			switch ( aiEnt->NPC->squadState )
			{
			case SQUAD_RETREAT://was running away
				TIMER_Set( aiEnt, "duck", (aiEnt->client->pers.maxHealth - aiEnt->health) * 100 );
				TIMER_Set( aiEnt, "hideTime", Q_irand( 3000, 7000 ) );
			//	newSquadState = SQUAD_COVER;
				break;
			case SQUAD_TRANSITION://was heading for a combat point
				TIMER_Set( aiEnt, "hideTime", Q_irand( 2000, 4000 ) );
				break;
			case SQUAD_SCOUT://was running after player
				break;
			default:
				break;
			}
			NPC_ReachedGoal(aiEnt);
			//don't attack right away
			TIMER_Set( aiEnt, "attackDelay", Q_irand( (6-aiEnt->NPC->stats.aim)*50, (6-aiEnt->NPC->stats.aim)*100 ) );	//FIXME: Slant for difficulty levels, too?
			//don't do something else just yet
			TIMER_Set( aiEnt, "roamTime", Q_irand( 1000, 4000 ) );
			//stop fleeing
			if ( aiEnt->NPC->squadState == SQUAD_RETREAT )
			{
				TIMER_Set( aiEnt, "flee", -level.time );
				aiEnt->NPC->squadState = SQUAD_IDLE;
			}
			return;
		}

		//keep going, hold of roamTimer until we get there
		TIMER_Set( aiEnt, "roamTime", Q_irand( 4000, 8000 ) );
	}
}

static void Sniper_ResolveBlockedShot( gentity_t *aiEnt)
{
	if ( TIMER_Done( aiEnt, "duck" ) )
	{//we're not ducking
		if ( TIMER_Done( aiEnt, "roamTime" ) )
		{//not roaming
			//FIXME: try to find another spot from which to hit the enemy
			if ( (aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) && (!aiEnt->NPC->goalEntity || aiEnt->NPC->goalEntity == aiEnt->enemy) )
			{//we were running after enemy
				//Try to find a combat point that can hit the enemy
				int cpFlags = (CP_CLEAR|CP_HAS_ROUTE);
				int cp;

				if ( aiEnt->NPC->scriptFlags&SCF_USE_CP_NEAREST )
				{
					cpFlags &= ~(CP_FLANK|CP_APPROACH_ENEMY|CP_CLOSEST);
					cpFlags |= CP_NEAREST;
				}
				cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, cpFlags, 32, -1 );
				if ( cp == -1 && !(aiEnt->NPC->scriptFlags&SCF_USE_CP_NEAREST) )
				{//okay, try one by the enemy
					cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin, CP_CLEAR|CP_HAS_ROUTE|CP_HORZ_DIST_COLL, 32, -1 );
				}
				//NOTE: there may be a perfectly valid one, just not one within CP_COLLECT_RADIUS of either me or him...
				if ( cp != -1 )
				{//found a combat point that has a clear shot to enemy
					NPC_SetCombatPoint(aiEnt, cp );
					NPC_SetMoveGoal( aiEnt, level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
					TIMER_Set( aiEnt, "duck", -1 );
					TIMER_Set( aiEnt, "attackDelay", Q_irand( 1000, 3000 ) );
					return;
				}
			}
		}
	}
	/*
	else
	{//maybe we should stand
		if ( TIMER_Done( NPC, "stand" ) )
		{//stand for as long as we'll be here
			TIMER_Set( NPC, "stand", Q_irand( 500, 2000 ) );
			return;
		}
	}
	//Hmm, can't resolve this by telling them to duck or telling me to stand
	//We need to move!
	TIMER_Set( NPC, "roamTime", -1 );
	TIMER_Set( NPC, "stick", -1 );
	TIMER_Set( NPC, "duck", -1 );
	TIMER_Set( NPC, "attackDelay", Q_irand( 1000, 3000 ) );
	*/
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void Sniper_CheckFireState( gentity_t *aiEnt)
{
	if ( enemyCS2 )
	{//if have a clear shot, always try
		return;
	}

	if ( aiEnt->NPC->squadState == SQUAD_RETREAT || aiEnt->NPC->squadState == SQUAD_TRANSITION || aiEnt->NPC->squadState == SQUAD_SCOUT )
	{//runners never try to fire at the last pos
		return;
	}

	if ( !VectorCompare( aiEnt->client->ps.velocity, vec3_origin ) )
	{//if moving at all, don't do this
		return;
	}

	//continue to fire on their last position
	if ( !Q_irand( 0, 1 ) && aiEnt->NPC->enemyLastSeenTime && level.time - aiEnt->NPC->enemyLastSeenTime < ((5-aiEnt->NPC->stats.aim)*1000) )//FIXME: incorporate skill too?
	{
		if ( !VectorCompare( vec3_origin, aiEnt->NPC->enemyLastSeenLocation ) )
		{
			//Fire on the last known position
			vec3_t	muzzle, dir, angles;

			CalcEntitySpot( aiEnt, SPOT_WEAPON, muzzle );
			VectorSubtract( aiEnt->NPC->enemyLastSeenLocation, muzzle, dir );

			VectorNormalize( dir );

			vectoangles( dir, angles );

			aiEnt->NPC->desiredYaw		= angles[YAW];
			aiEnt->NPC->desiredPitch	= angles[PITCH];

			shoot2 = qtrue;
			//faceEnemy2 = qfalse;
		}
		return;
	}
	else if ( level.time - aiEnt->NPC->enemyLastSeenTime > 10000 )
	{//next time we see him, we'll miss few times first
		aiEnt->count = 0;
	}
}

qboolean Sniper_EvaluateShot( gentity_t *aiEnt, int hit )
{
	gentity_t *hitEnt;

	if ( !aiEnt->enemy )
	{
		return qfalse;
	}

	hitEnt = &g_entities[hit];
	if ( hit == aiEnt->enemy->s.number
		|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == aiEnt->client->enemyTeam )
		|| ( hitEnt && hitEnt->takedamage && ((hitEnt->r.svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||aiEnt->s.weapon == WP_EMPLACED_GUN) )
		|| ( hitEnt && (hitEnt->r.svFlags&SVF_GLASS_BRUSH)) )
	{//can hit enemy or will hit glass, so shoot anyway
		return qtrue;
	}
	return qfalse;
}

void Sniper_FaceEnemy( gentity_t *aiEnt)
{
	//FIXME: the ones behind kill holes are facing some arbitrary direction and not firing
	//FIXME: If actually trying to hit enemy, don't fire unless enemy is at least in front of me?
	//FIXME: need to give designers option to make them not miss first few shots
	if ( aiEnt->enemy )
	{
		vec3_t	muzzle, target, angles, forward, right, up;
		//Get the positions
		AngleVectors( aiEnt->client->ps.viewangles, forward, right, up );
		CalcMuzzlePoint( aiEnt, forward, right, up, muzzle );
		//CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
		CalcEntitySpot( aiEnt->enemy, SPOT_ORIGIN, target );

		if ( enemyDist2 > 65536 && aiEnt->NPC->stats.aim < 5 )//is 256 squared, was 16384 (128*128)
		{
			if ( aiEnt->count < (5-aiEnt->NPC->stats.aim) )
			{//miss a few times first
				if ( shoot2 && TIMER_Done( aiEnt, "attackDelay" ) && level.time >= aiEnt->NPC->shotTime )
				{//ready to fire again
					qboolean	aimError = qfalse;
					qboolean	hit = qtrue;
					int			tryMissCount = 0;
					trace_t		trace;

					GetAnglesForDirection( muzzle, target, angles );
					AngleVectors( angles, forward, right, up );

					while ( hit && tryMissCount < 10 )
					{
						tryMissCount++;
						if ( !Q_irand( 0, 1 ) )
						{
							aimError = qtrue;
							if ( !Q_irand( 0, 1 ) )
							{
								VectorMA( target, aiEnt->enemy->r.maxs[2]*flrand(1.5, 4), right, target );
							}
							else
							{
								VectorMA( target, aiEnt->enemy->r.mins[2]*flrand(1.5, 4), right, target );
							}
						}
						if ( !aimError || !Q_irand( 0, 1 ) )
						{
							if ( !Q_irand( 0, 1 ) )
							{
								VectorMA( target, aiEnt->enemy->r.maxs[2]*flrand(1.5, 4), up, target );
							}
							else
							{
								VectorMA( target, aiEnt->enemy->r.mins[2]*flrand(1.5, 4), up, target );
							}
						}
						trap->Trace( &trace, muzzle, vec3_origin, vec3_origin, target, aiEnt->s.number, MASK_SHOT, qfalse, 0, 0 );
						hit = Sniper_EvaluateShot(aiEnt, trace.entityNum );
					}
					aiEnt->count++;
				}
				else
				{
					if ( !enemyLOS2 )
					{
						NPC_UpdateAngles(aiEnt, qtrue, qtrue );
						return;
					}
				}
			}
			else
			{//based on distance, aim value, difficulty and enemy movement, miss
				//FIXME: incorporate distance as a factor?
				int missFactor = 8-(aiEnt->NPC->stats.aim+g_npcspskill.integer) * 3;
				if ( missFactor > ENEMY_POS_LAG_STEPS )
				{
					missFactor = ENEMY_POS_LAG_STEPS;
				}
				else if ( missFactor < 0 )
				{//???
					missFactor = 0 ;
				}
				VectorCopy( aiEnt->NPC->enemyLaggedPos[missFactor], target );
			}
			GetAnglesForDirection( muzzle, target, angles );
		}
		else
		{
			target[2] += flrand( 0, aiEnt->enemy->r.maxs[2] );
			//CalcEntitySpot( NPC->enemy, SPOT_HEAD_LEAN, target );
			GetAnglesForDirection( muzzle, target, angles );
		}

		aiEnt->NPC->desiredYaw		= AngleNormalize360( angles[YAW] );
		aiEnt->NPC->desiredPitch	= AngleNormalize360( angles[PITCH] );
	}
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

void Sniper_UpdateEnemyPos( gentity_t *aiEnt)
{
	int index;
	int i;

	for ( i = MAX_ENEMY_POS_LAG-ENEMY_POS_LAG_INTERVAL; i >= 0; i -= ENEMY_POS_LAG_INTERVAL )
	{
		index = i/ENEMY_POS_LAG_INTERVAL;
		if ( !index )
		{
			CalcEntitySpot( aiEnt->enemy, SPOT_HEAD_LEAN, aiEnt->NPC->enemyLaggedPos[index] );
			aiEnt->NPC->enemyLaggedPos[index][2] -= flrand( 2, 16 );
		}
		else
		{
			VectorCopy( aiEnt->NPC->enemyLaggedPos[index-1], aiEnt->NPC->enemyLaggedPos[index] );
		}
	}
}

/*
-------------------------
NPC_BSSniper_Attack
-------------------------
*/

void Sniper_StartHide( gentity_t *aiEnt)
{
	int duckTime = Q_irand( 2000, 5000 );

	TIMER_Set( aiEnt, "duck", duckTime );
	TIMER_Set( aiEnt, "watch", 500 );
	TIMER_Set( aiEnt, "attackDelay", duckTime + Q_irand( 500, 2000 ) );
}

void NPC_BSSniper_Attack( gentity_t *aiEnt)
{
	//Don't do anything if we're hurt
	if ( aiEnt->painDebounceTime > level.time )
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	//NPC_CheckEnemy( qtrue, qfalse );
	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )//!NPC->enemy )//
	{
		aiEnt->enemy = NULL;
		NPC_BSSniper_Patrol(aiEnt);//FIXME: or patrol?
#ifdef __SNIPER_DEBUG__
		trap->Print("SNIPER DEBUG: %s has no enemy.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
		return;
	}

	if ( TIMER_Done( aiEnt, "flee" ) && NPC_CheckForDanger(aiEnt, NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
	{//going to run
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
#ifdef __SNIPER_DEBUG__
		trap->Print("SNIPER DEBUG: %s should be fleeing.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
		return;
	}

	if ( !aiEnt->enemy )
	{//WTF?  somehow we lost our enemy?
		NPC_BSSniper_Patrol(aiEnt);//FIXME: or patrol?
#ifdef __SNIPER_DEBUG__
		trap->Print("SNIPER DEBUG: %s has lost it's enemy.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
		return;
	}

	enemyLOS2 = enemyCS2 = qfalse;
	move2 = qtrue;
	faceEnemy2 = qfalse;
	shoot2 = qfalse;
	enemyDist2 = DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
	
	if ( enemyDist2 < 16384 )//128 squared
	{//too close, so switch to primary fire
		if (WeaponIsSniperCharge(aiEnt->client->ps.weapon))
		{//sniping... should be assumed
			if ( aiEnt->NPC->scriptFlags & SCF_ALT_FIRE )
			{//use primary fire
				trace_t	trace;
				trap->Trace ( &trace, aiEnt->enemy->r.currentOrigin, aiEnt->enemy->r.mins, aiEnt->enemy->r.maxs, aiEnt->r.currentOrigin, aiEnt->enemy->s.number, aiEnt->enemy->clipmask, qfalse, 0, 0 );
				if ( !trace.allsolid && !trace.startsolid && (trace.fraction == 1.0 || trace.entityNum == aiEnt->s.number ) )
				{//he can get right to me
					aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
					//reset fire-timing variables
					NPC_ChangeWeapon(aiEnt, aiEnt->client->ps.weapon );
					NPC_UpdateAngles(aiEnt, qtrue, qtrue );
#ifdef __SNIPER_DEBUG__
					trap->Print("SNIPER DEBUG: %s disabled alt fire.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
					return;
				}
			}
			//FIXME: switch back if he gets far away again?
		}
	}
	else if ( enemyDist2 > 65536 )//256 squared
	{
		if (WeaponIsSniperCharge(aiEnt->client->ps.weapon))
		{//sniping... should be assumed
			if ( !(aiEnt->NPC->scriptFlags&SCF_ALT_FIRE) )
			{//use primary fire
				aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;
				//reset fire-timing variables
				NPC_ChangeWeapon(aiEnt, aiEnt->client->ps.weapon );
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
#ifdef __SNIPER_DEBUG__
				trap->Print("SNIPER DEBUG: %s enabled alt fire.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
				return;
			}
		}
	}

	Sniper_UpdateEnemyPos(aiEnt);

	//can we see our target?
	if ( NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )//|| (NPCInfo->stats.aim >= 5 && trap->inPVS( NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin )) )
	{
		float maxShootDist;

		aiEnt->NPC->enemyLastSeenTime = level.time;
		VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
		enemyLOS2 = qtrue;
		maxShootDist = NPC_MaxDistSquaredForWeapon(aiEnt);
		if ( enemyDist2 < maxShootDist )
		{
			vec3_t fwd, right, up, muzzle, end;
			trace_t	tr;
			int hit;

			AngleVectors( aiEnt->client->ps.viewangles, fwd, right, up );
			CalcMuzzlePoint( aiEnt, fwd, right, up, muzzle );
			VectorMA( muzzle, 8192, fwd, end );
			trap->Trace ( &tr, muzzle, NULL, NULL, end, aiEnt->s.number, MASK_SHOT, qfalse, 0, 0 );

			hit = tr.entityNum;
			//can we shoot our target?
			if ( Sniper_EvaluateShot(aiEnt, hit ) )
			{
				enemyCS2 = qtrue;
			}
		}
	}
	/*
	else if ( trap->inPVS( NPC->enemy->currentOrigin, NPC->currentOrigin ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy2 = qtrue;
	}
	*/

	if ( enemyLOS2 )
	{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy2 = qtrue;
	}
	if ( enemyCS2 )
	{
		shoot2 = qtrue;
	}
	else if ( level.time - aiEnt->NPC->enemyLastSeenTime > 3000 )
	{//Hmm, have to get around this bastard... FIXME: this NPCInfo->enemyLastSeenTime builds up when ducked seems to make them want to run when they uncrouch
		Sniper_ResolveBlockedShot(aiEnt);
	}

	//Check for movement to take care of
	Sniper_CheckMoveState(aiEnt);

	//See if we should override shooting decision with any special considerations
	Sniper_CheckFireState(aiEnt);

	if ( move2 )
	{//move toward goal
#if 1
		if ( aiEnt->NPC->goalEntity )//&& ( NPCInfo->goalEntity != NPC->enemy || enemyDist2 > 10000 ) )//100 squared
		{
			move2 = Sniper_Move(aiEnt);
#ifdef __SNIPER_DEBUG__
			trap->Print("SNIPER DEBUG: %s is moving.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
		}
#else //!0
		if (enemyDist2 < 256*256 && aiEnt->NPC->goalEntity)
		{// UQ1: How about only when they are too close???
			move2 = Sniper_Move(aiEnt);
#ifdef __SNIPER_DEBUG__
			trap->Print("SNIPER DEBUG: %s is moving.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
		}
#endif //0
		else
		{
			move2 = qfalse;
		}
	}

	if ( !move2 )
	{
		if ( !TIMER_Done( aiEnt, "duck" ) )
		{
			if ( TIMER_Done( aiEnt, "watch" ) )
			{//not while watching
				aiEnt->client->pers.cmd.upmove = -127;
			}
		}
		//FIXME: what about leaning?
		//FIXME: also, when stop ducking, start looking, if enemy can see me, chance of ducking back down again
	}
	else
	{//stop ducking!
		TIMER_Set( aiEnt, "duck", -1 );
	}

	if ( TIMER_Done( aiEnt, "duck" )
		&& TIMER_Done( aiEnt, "watch" )
		&& (TIMER_Get( aiEnt, "attackDelay" )-level.time) > 1000
		&& aiEnt->attackDebounceTime < level.time )
	{
		if ( enemyLOS2 && (aiEnt->NPC->scriptFlags&SCF_ALT_FIRE) )
		{
			if ( aiEnt->fly_sound_debounce_time < level.time )
			{
				aiEnt->fly_sound_debounce_time = level.time + 2000;
			}
		}
	}

	if ( !faceEnemy2 )
	{//we want to face in the dir we're running
		if ( move2 )
		{//don't run away and shoot
			aiEnt->NPC->desiredYaw = aiEnt->NPC->lastPathAngles[YAW];
			aiEnt->NPC->desiredPitch = 0;
			shoot2 = qfalse;
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
	}
	else// if ( faceEnemy2 )
	{//face the enemy
		Sniper_FaceEnemy(aiEnt);
	}

	if ( aiEnt->NPC->scriptFlags & SCF_DONT_FIRE )
	{
		shoot2 = qfalse;
	}

	//FIXME: don't shoot right away!
	if ( shoot2 )
	{//try to shoot if it's time
		if ( TIMER_Done( aiEnt, "attackDelay" ) )
		{
			WeaponThink(aiEnt, qtrue );

			if ( aiEnt->client->pers.cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK) )
			{
				G_SoundOnEnt( aiEnt, CHAN_WEAPON, "sound/null.wav" );
#ifdef __SNIPER_DEBUG__
				trap->Print("SNIPER DEBUG: %s has shot at enemy.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
			}

			//took a shot, now hide
			if ( !(aiEnt->spawnflags & SPF_NO_HIDE) && !Q_irand( 0, 1 ) )
			{
				//FIXME: do this if in combat point and combat point has duck-type cover... also handle lean-type cover
				Sniper_StartHide(aiEnt);
#ifdef __SNIPER_DEBUG__
				trap->Print("SNIPER DEBUG: %s should take cover.\n", aiEnt->client->pers.netname);
#endif //__SNIPER_DEBUG__
			}
			else
			{
				TIMER_Set( aiEnt, "attackDelay", aiEnt->NPC->shotTime-level.time );
			}
		}
	}
}

void NPC_BSSniper_Default( gentity_t *aiEnt )
{
	if( !aiEnt->enemy )
	{//don't have an enemy, look for one
		NPC_BSSniper_Patrol(aiEnt);
	}
	else//if ( NPC->enemy )
	{//have an enemy
		NPC_BSSniper_Attack(aiEnt);
	}
}
