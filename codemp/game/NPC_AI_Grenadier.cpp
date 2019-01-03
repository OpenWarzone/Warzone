#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

extern qboolean BG_SabersOff( playerState_t *ps );

extern void CG_DrawAlert( vec3_t origin, float rating );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern void NPC_AimAdjust( gentity_t *aiEnt, int change );
extern qboolean FlyingCreature( gentity_t *ent );

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

static qboolean enemyLOS3;
static qboolean enemyCS3;
static qboolean faceEnemy3;
static qboolean move3;
static qboolean shoot3;
static float	enemyDist3;

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void Grenadier_ClearTimers( gentity_t *ent )
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

void NPC_Grenadier_PlayConfusionSound( gentity_t *self )
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

void NPC_Grenadier_Pain(gentity_t *self, gentity_t *attacker, int damage)
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

static void Grenadier_HoldPosition(gentity_t *aiEnt)
{
	NPC_FreeCombatPoint(aiEnt, aiEnt->NPC->combatPoint, qtrue );
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

static qboolean Grenadier_Move(gentity_t *aiEnt)
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
			Grenadier_HoldPosition(aiEnt);
		}
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{//couldn't get to enemy
		if ( (aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) && aiEnt->client->ps.weapon == WP_THERMAL && aiEnt->NPC->goalEntity && aiEnt->NPC->goalEntity == aiEnt->enemy )
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
		Grenadier_HoldPosition(aiEnt);
	}

	return moved;
}

/*
-------------------------
NPC_BSGrenadier_Patrol
-------------------------
*/

void NPC_BSGrenadier_Patrol(gentity_t *aiEnt)
{//FIXME: pick up on bodies of dead buddies?
	if ( aiEnt->NPC->confusionTime < level.time )
	{
		//Look for any enemies
		if ( aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			if ( NPC_CheckPlayerTeamStealth(aiEnt) )
			{
				//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be automatic now
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
							TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
						}
					}
					else
					{//FIXME: get more suspicious over time?
						//Save the position for movement (if necessary)
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
			{//FIXME: walk over to it, maybe?  Not if not chase enemies
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
NPC_BSGrenadier_Idle
-------------------------
*/
/*
void NPC_BSGrenadier_Idle( gentity_t *aiEnt )
{
	//FIXME: check for other alert events?

	//Is there danger nearby?
	if ( NPC_CheckForDanger( NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	TIMER_Set( NPC, "roamTime", 2000 + Q_irand( 1000, 2000 ) );

	NPC_UpdateAngles( qtrue, qtrue );
}
*/
/*
-------------------------
ST_CheckMoveState
-------------------------
*/

static void Grenadier_CheckMoveState(gentity_t *aiEnt)
{
	//See if we're a scout
	if ( !(aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES) )//behaviorState == BS_STAND_AND_SHOOT )
	{
		if ( aiEnt->NPC->goalEntity == aiEnt->enemy )
		{
			move3 = qfalse;
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
			faceEnemy3 = qfalse;
		}
	}
	/*
	else if ( NPCInfo->squadState == SQUAD_IDLE )
	{
		if ( !NPCInfo->goalEntity )
		{
			move3 = qfalse;
			return;
		}
		//Should keep moving toward player when we're out of range... right?
	}
	*/

	//See if we're moving towards a goal, not the enemy
	if ( ( aiEnt->NPC->goalEntity != aiEnt->enemy ) && ( aiEnt->NPC->goalEntity != NULL ) )
	{
		//Did we make it?
		if ( NAV_HitNavGoal( aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, aiEnt->NPC->goalEntity->r.currentOrigin, 16, FlyingCreature( aiEnt ) ) ||
			( aiEnt->NPC->squadState == SQUAD_SCOUT && enemyLOS3 && enemyDist3 <= 10000 ) )
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
			TIMER_Set( aiEnt, "attackDelay", Q_irand( 250, 500 ) );	//FIXME: Slant for difficulty levels
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

	if ( !aiEnt->NPC->goalEntity )
	{
		if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
		{
			aiEnt->NPC->goalEntity = aiEnt->enemy;
		}
	}
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void Grenadier_CheckFireState(gentity_t *aiEnt)
{
	if ( enemyCS3 )
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
	/*
	if ( !Q_irand( 0, 1 ) && NPCInfo->enemyLastSeenTime && level.time - NPCInfo->enemyLastSeenTime < 4000 )
	{
		//Fire on the last known position
		vec3_t	muzzle, dir, angles;

		CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
		VectorSubtract( NPCInfo->enemyLastSeenLocation, muzzle, dir );

		VectorNormalize( dir );

		vectoangles( dir, angles );

		NPCInfo->desiredYaw		= angles[YAW];
		NPCInfo->desiredPitch	= angles[PITCH];
		//FIXME: they always throw toward enemy, so this will be very odd...
		shoot3 = qtrue;
		faceEnemy3 = qfalse;

		return;
	}
	*/
}

qboolean Grenadier_EvaluateShot(gentity_t *aiEnt, int hit )
{
	if ( !aiEnt->enemy )
	{
		return qfalse;
	}

	if ( hit == aiEnt->enemy->s.number || (&g_entities[hit] != NULL && (g_entities[hit].r.svFlags&SVF_GLASS_BRUSH)) )
	{//can hit enemy or will hit glass, so shoot anyway
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
NPC_BSGrenadier_Attack
-------------------------
*/

void NPC_BSGrenadier_Attack(gentity_t *aiEnt)
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
		NPC_BSGrenadier_Patrol(aiEnt);//FIXME: or patrol?
		return;
	}

	if ( TIMER_Done( aiEnt, "flee" ) && NPC_CheckForDanger(aiEnt, NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
	{//going to run
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	if ( !aiEnt->enemy )
	{//WTF?  somehow we lost our enemy?
		NPC_BSGrenadier_Patrol(aiEnt);//FIXME: or patrol?
		return;
	}

	enemyLOS3 = enemyCS3 = qfalse;
	move3 = qtrue;
	faceEnemy3 = qfalse;
	shoot3 = qfalse;
	enemyDist3 = DistanceSquared( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin );

	//See if we should switch to melee attack
	if ( enemyDist3 < 16384 //128
		&& (!aiEnt->enemy->client
			|| aiEnt->enemy->client->ps.weapon != WP_SABER
			|| BG_SabersOff( &aiEnt->enemy->client->ps )
			)
		)
	{//enemy is close and not using saber
		if ( aiEnt->client->ps.weapon == WP_THERMAL )
		{//grenadier
			trace_t	trace;
			trap->Trace ( &trace, aiEnt->r.currentOrigin, aiEnt->enemy->r.mins, aiEnt->enemy->r.maxs, aiEnt->enemy->r.currentOrigin, aiEnt->s.number, aiEnt->enemy->clipmask, qfalse, 0, 0 );
			if ( !trace.allsolid && !trace.startsolid && (trace.fraction == 1.0 || trace.entityNum == aiEnt->enemy->s.number ) )
			{//I can get right to him
				//reset fire-timing variables
				NPC_ChangeWeapon(aiEnt, WP_MELEE );
				if ( !(aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )//NPCInfo->behaviorState == BS_STAND_AND_SHOOT )
				{//FIXME: should we be overriding scriptFlags?
					aiEnt->NPC->scriptFlags |= SCF_CHASE_ENEMIES;//NPCInfo->behaviorState = BS_HUNT_AND_KILL;
				}
			}
		}
	}
	else if ( enemyDist3 > 65536 || (aiEnt->enemy->client && aiEnt->enemy->client->ps.weapon == WP_SABER && !aiEnt->enemy->client->ps.saberHolstered) )//256
	{//enemy is far or using saber
		if ( aiEnt->client->ps.weapon == WP_MELEE && HaveWeapon(&aiEnt->client->ps, WP_THERMAL) )
		{//fisticuffs, make switch to thermal if have it
			//reset fire-timing variables
			NPC_ChangeWeapon(aiEnt, WP_THERMAL );
		}
	}

	//can we see our target?
	if ( NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
	{
		aiEnt->NPC->enemyLastSeenTime = level.time;
		enemyLOS3 = qtrue;

		if ( aiEnt->client->ps.weapon == WP_MELEE )
		{
			if ( enemyDist3 <= 4096 && InFOV3( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->client->ps.viewangles, 90, 45 ) )//within 64 & infront
			{
				VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
				enemyCS3 = qtrue;
			}
		}
		else if ( InFOV3( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->client->ps.viewangles, 45, 90 ) )
		{//in front of me
			//can we shoot our target?
			//FIXME: how accurate/necessary is this check?
			int hit = NPC_ShotEntity(aiEnt, aiEnt->enemy, NULL );
			gentity_t *hitEnt = &g_entities[hit];
			if ( hit == aiEnt->enemy->s.number
				|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == aiEnt->client->enemyTeam ) )
			{
				float enemyHorzDist;

				VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
				enemyHorzDist = DistanceHorizontalSquared( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin );
				if ( enemyHorzDist < 1048576 )
				{//within 1024
					enemyCS3 = qtrue;
					NPC_AimAdjust( aiEnt, 2 );//adjust aim better longer we have clear shot at enemy
				}
				else
				{
					NPC_AimAdjust( aiEnt, 1 );//adjust aim better longer we can see enemy
				}
			}
		}
	}
	else
	{
		NPC_AimAdjust( aiEnt, -1 );//adjust aim worse longer we cannot see enemy
	}
	/*
	else if ( trap->InPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy3 = qtrue;
	}
	*/

	if ( enemyLOS3 )
	{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy3 = qtrue;
	}

	if ( enemyCS3 )
	{
		shoot3 = qtrue;
		if ( aiEnt->client->ps.weapon == WP_THERMAL )
		{//don't chase and throw
			move3 = qfalse;
		}
		else if ( aiEnt->client->ps.weapon == WP_MELEE && enemyDist3 < (aiEnt->r.maxs[0]+aiEnt->enemy->r.maxs[0]+16)*(aiEnt->r.maxs[0]+aiEnt->enemy->r.maxs[0]+16) )
		{//close enough
			move3 = qfalse;
		}
	}//this should make him chase enemy when out of range...?

	//Check for movement to take care of
	Grenadier_CheckMoveState(aiEnt);

	//See if we should override shooting decision with any special considerations
	Grenadier_CheckFireState(aiEnt);

	if ( move3 )
	{//move toward goal
		if ( aiEnt->NPC->goalEntity )//&& ( NPCInfo->goalEntity != NPC->enemy || enemyDist3 > 10000 ) )//100 squared
		{
			move3 = Grenadier_Move(aiEnt);
		}
		else
		{
			move3 = qfalse;
		}
	}

	if ( !move3 )
	{
		if ( !TIMER_Done( aiEnt, "duck" ) )
		{
			aiEnt->client->pers.cmd.upmove = -127;
		}
		//FIXME: what about leaning?
	}
	else
	{//stop ducking!
		TIMER_Set( aiEnt, "duck", -1 );
	}

	if ( !faceEnemy3 )
	{//we want to face in the dir we're running
		if ( move3 )
		{//don't run away and shoot
			aiEnt->NPC->desiredYaw = aiEnt->NPC->lastPathAngles[YAW];
			aiEnt->NPC->desiredPitch = 0;
			shoot3 = qfalse;
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
	}
	else// if ( faceEnemy3 )
	{//face the enemy
		NPC_FaceEnemy(aiEnt, qtrue);
	}

	if ( aiEnt->NPC->scriptFlags&SCF_DONT_FIRE )
	{
		shoot3 = qfalse;
	}

	//FIXME: don't shoot right away!
	if ( shoot3 )
	{//try to shoot if it's time
		if ( TIMER_Done( aiEnt, "attackDelay" ) )
		{
			if( !(aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
			{
				WeaponThink(aiEnt, qtrue );
				TIMER_Set( aiEnt, "attackDelay", aiEnt->NPC->shotTime-level.time );
			}
		}
	}
}

void NPC_BSGrenadier_Default(gentity_t *aiEnt)
{
	if( aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink(aiEnt, qtrue );
	}

	if( !aiEnt->enemy )
	{//don't have an enemy, look for one
		NPC_BSGrenadier_Patrol(aiEnt);
	}
	else//if ( NPC->enemy )
	{//have an enemy
		NPC_BSGrenadier_Attack(aiEnt);
	}
}
