#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void AI_GroupUpdateSquadstates( AIGroupInfo_t *group, gentity_t *member, int newSquadState );
extern qboolean AI_GroupContainsEntNum( AIGroupInfo_t *group, int entNum );
extern void AI_GroupUpdateEnemyLastSeen( AIGroupInfo_t *group, vec3_t spot );
extern void AI_GroupUpdateClearShotTime( AIGroupInfo_t *group );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
extern void NPC_CheckGetNewWeapon( gentity_t *aiEnt);
extern int GetTime ( int lastTime );
extern void NPC_AimAdjust( gentity_t *aiEnt, int change );
extern qboolean FlyingCreature( gentity_t *ent );
extern qboolean NPC_CanUseAdvancedFighting(gentity_t *aiEnt);

#define	MAX_VIEW_DIST		1024
#define MAX_VIEW_SPEED		250
#define	MAX_LIGHT_INTENSITY 255
#define	MIN_LIGHT_THRESHOLD	0.1
#define	ST_MIN_LIGHT_THRESHOLD 30
#define	ST_MAX_LIGHT_THRESHOLD 180
#define	DISTANCE_THRESHOLD	0.075f

#define	DISTANCE_SCALE		0.35f	//These first three get your base detection rating, ideally add up to 1
#define	FOV_SCALE			0.40f	//
#define	LIGHT_SCALE			0.25f	//

#define	SPEED_SCALE			0.25f	//These next two are bonuses
#define	TURNING_SCALE		0.25f	//

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75 )

qboolean NPC_CheckPlayerTeamStealth(gentity_t *aiEnt);

static qboolean enemyLOS;
static qboolean enemyCS;
static qboolean enemyInFOV;
static qboolean hitAlly;
static qboolean faceEnemy;
static qboolean move;
static qboolean shoot;
static float	enemyDist;
static vec3_t	impactPos;

int groupSpeechDebounceTime[FACTION_NUM_FACTIONS];//used to stop several group AI from speaking all at once

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void ST_AggressionAdjust( gentity_t *self, int change )
{
	int	upper_threshold, lower_threshold;

	self->NPC->stats.aggression += change;

	//FIXME: base this on initial NPC stats
	if ( self->client->playerTeam == NPCTEAM_PLAYER )
	{//good guys are less aggressive
		upper_threshold = 7;
		lower_threshold = 1;
	}
	else
	{//bad guys are more aggressive
		upper_threshold = 10;
		lower_threshold = 3;
	}

	if ( self->NPC->stats.aggression > upper_threshold )
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if ( self->NPC->stats.aggression < lower_threshold )
	{
		self->NPC->stats.aggression = lower_threshold;
	}
}

void ST_ClearTimers( gentity_t *ent )
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
	TIMER_Set( ent, "interrogating", 0 );
	TIMER_Set( ent, "verifyCP", 0 );
}

extern char *Get_NPC_Name ( int NAME_ID );

stringID_table_t speechTable[SPEECH_MAX+1] =
{
	ENUM2STRING(SPEECH_CHASE),
	ENUM2STRING(SPEECH_CONFUSED),
	ENUM2STRING(SPEECH_COVER),
	ENUM2STRING(SPEECH_DETECTED),
	ENUM2STRING(SPEECH_GIVEUP),
	ENUM2STRING(SPEECH_LOOK),
	ENUM2STRING(SPEECH_LOST),
	ENUM2STRING(SPEECH_OUTFLANK),
	ENUM2STRING(SPEECH_ESCAPING),
	ENUM2STRING(SPEECH_SIGHT),
	ENUM2STRING(SPEECH_SOUND),
	ENUM2STRING(SPEECH_SUSPICIOUS),
	ENUM2STRING(SPEECH_YELL),
	ENUM2STRING(SPEECH_PUSHED),
	ENUM2STRING(SPEECH_MAX),
};

void ST_Speech( gentity_t *self, int speechType, float failChance )
{
	//trap->Print("NPC %i trying to talk %i.\n", self->s.number, speechType);

	if ( random() < failChance )
	{
		return;
	}

	//trap->Print("NPC %s trying to talk %s - ", Get_NPC_Name(self->s.NPC_NAME_ID), speechTable[speechType]);

	if ( failChance >= 0 )
	{//a negative failChance makes it always talk
		if ( self->NPC->group )
		{//group AI speech debounce timer
			if ( self->NPC->group->speechDebounceTime > level.time )
			{
				//trap->Print("RESULT: Blocked group speech Debounce\n");
				return;
			}
			/*
			else if ( !self->NPC->group->enemy )
			{
				if ( groupSpeechDebounceTime[self->client->playerTeam] > level.time )
				{
					return;
				}
			}
			*/
			else
			{//So they don't all speak at once...
				//FIXME: if they're not yet mad, they have no group, so distracting a group of them makes them all speak!
				self->NPC->group->speechDebounceTime = level.time + Q_irand( 2000, 4000 );
			}
		}
		else
		{
			if ( !TIMER_Done( self, "chatter" ) )
			{//personal timer
				//trap->Print("RESULT: Blocked speech chatter\n");
				return;
			}
			else
			{
				TIMER_Set( self, "chatter", Q_irand( 2000, 4000 ) );
			}
		}

//#if 0
		if ( groupSpeechDebounceTime[self->client->playerTeam] > level.time )
		{//for those not in group AI
			//FIXME: let certain speech types interrupt others?  Let closer NPCs interrupt farther away ones?
			return;
		}
		else
		{
			groupSpeechDebounceTime[self->client->playerTeam] = level.time + Q_irand( 2000, 4000 );
		}
//#endif //0
	}

	if ( self->NPC->blockedSpeechDebounceTime > level.time )
	{
		//trap->Print("RESULT: Blocked speech Debounce\n");
		return;
	}

	switch( speechType )
	{
	case SPEECH_CHASE:
		G_AddVoiceEvent( self, Q_irand(EV_CHASE1, EV_CHASE3), 2000 );
		break;
	case SPEECH_CONFUSED:
		G_AddVoiceEvent( self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000 );
		break;
	case SPEECH_COVER:
		G_AddVoiceEvent( self, Q_irand(EV_COVER1, EV_COVER5), 2000 );
		break;
	case SPEECH_DETECTED:
		G_AddVoiceEvent( self, Q_irand(EV_DETECTED1, EV_DETECTED5), 2000 );
		break;
	case SPEECH_GIVEUP:
		G_AddVoiceEvent( self, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 2000 );
		break;
	case SPEECH_LOOK:
		G_AddVoiceEvent( self, Q_irand(EV_LOOK1, EV_LOOK2), 2000 );
		break;
	case SPEECH_LOST:
		G_AddVoiceEvent( self, EV_LOST1, 2000 );
		break;
	case SPEECH_OUTFLANK:
		G_AddVoiceEvent( self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000 );
		break;
	case SPEECH_ESCAPING:
		G_AddVoiceEvent( self, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 2000 );
		break;
	case SPEECH_SIGHT:
		G_AddVoiceEvent( self, Q_irand(EV_SIGHT1, EV_SIGHT3), 2000 );
		break;
	case SPEECH_SOUND:
		G_AddVoiceEvent( self, Q_irand(EV_SOUND1, EV_SOUND3), 2000 );
		break;
	case SPEECH_SUSPICIOUS:
		G_AddVoiceEvent( self, Q_irand(EV_SUSPICIOUS1, EV_SUSPICIOUS5), 2000 );
		break;
	case SPEECH_YELL:
		G_AddVoiceEvent( self, Q_irand( EV_ANGER1, EV_ANGER3 ), 2000 );
		break;
	case SPEECH_PUSHED:
		G_AddVoiceEvent( self, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 2000 );
		break;
	default:
		break;
	}

	//trap->Print("RESULT: Talked\n");

	self->NPC->blockedSpeechDebounceTime = level.time + 2000;
}

void ST_MarkToCover( gentity_t *self )
{
	if ( !self || !self->NPC )
	{
		return;
	}
	self->NPC->localState = LSTATE_UNDERFIRE;
	TIMER_Set( self, "attackDelay", Q_irand( 500, 2500 ) );
	ST_AggressionAdjust( self, -3 );
	if ( self->NPC->group && self->NPC->group->numGroup > 1 )
	{
		ST_Speech( self, SPEECH_COVER, 0 );//FIXME: flee sound?
	}
}

void ST_StartFlee( gentity_t *self, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int minTime, int maxTime )
{
	if ( !self || !self->NPC )
	{
		return;
	}
	G_StartFlee(self, enemy, dangerPoint, dangerLevel, minTime, maxTime );
	if ( self->NPC->group && self->NPC->group->numGroup > 1 )
	{
		ST_Speech( self, SPEECH_COVER, 0 );//FIXME: flee sound?
	}
}
/*
-------------------------
NPC_ST_Pain
-------------------------
*/

void NPC_ST_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "hideTime", -1 );
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

static void ST_HoldPosition(gentity_t *aiEnt)
{

	if ( aiEnt->NPC->squadState == SQUAD_RETREAT )
	{
		TIMER_Set( aiEnt, "flee", -level.time );
	}
	TIMER_Set( aiEnt, "verifyCP", Q_irand( 1000, 3000 ) );//don't look for another one for a few seconds
	NPC_FreeCombatPoint(aiEnt, aiEnt->NPC->combatPoint, qtrue );
	//NPCInfo->combatPoint = -1;//???
#ifndef __NO_ICARUS__
	if ( !trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) )
#endif //__NO_ICARUS__
	{//don't have a script waiting for me to get to my point, okay to stop trying and stand
		AI_GroupUpdateSquadstates( aiEnt->NPC->group, aiEnt, SQUAD_STAND_AND_SHOOT );
		aiEnt->NPC->goalEntity = NULL;
	}

	/*if ( TIMER_Done( NPC, "stand" ) )
	{//FIXME: what if can't shoot from this pos?
		TIMER_Set( NPC, "duck", Q_irand( 2000, 4000 ) );
	}
	*/
}

void NPC_ST_SayMovementSpeech(gentity_t *aiEnt)
{

	if ( !aiEnt->NPC->movementSpeech )
	{
		return;
	}
	if ( aiEnt->NPC->group &&
		aiEnt->NPC->group->commander &&
		aiEnt->NPC->group->commander->client &&
		aiEnt->NPC->group->commander->client->NPC_class == CLASS_IMPERIAL &&
		!Q_irand( 0, 3 ) )
	{//imperial (commander) gives the order
		ST_Speech( aiEnt->NPC->group->commander, aiEnt->NPC->movementSpeech, aiEnt->NPC->movementSpeechChance );
	}
	else
	{//really don't want to say this unless we can actually get there...
		ST_Speech( aiEnt, aiEnt->NPC->movementSpeech, aiEnt->NPC->movementSpeechChance );
	}

	aiEnt->NPC->movementSpeech = 0;
	aiEnt->NPC->movementSpeechChance = 0.0f;
}

void NPC_ST_StoreMovementSpeech(gentity_t *aiEnt, int speech, float chance )
{
	aiEnt->NPC->movementSpeech = speech;
	aiEnt->NPC->movementSpeechChance = chance;
}
/*
-------------------------
ST_Move
-------------------------
*/
void ST_TransferMoveGoal( gentity_t *self, gentity_t *other );
static qboolean ST_Move(gentity_t *aiEnt)
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
			ST_HoldPosition(aiEnt);
		}
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{//FIXME: if we're going to a combat point, need to pick a different one
#ifndef __NO_ICARUS__
		if ( !trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) )
#endif //__NO_ICARUS__
		{//can't transfer movegoal or stop when a script we're running is waiting to complete
			if ( info.blocker && info.blocker->NPC && aiEnt->NPC->group != NULL && info.blocker->NPC->group == aiEnt->NPC->group )//(NPCInfo->aiFlags&NPCAI_BLOCKED) && NPCInfo->group != NULL )
			{//dammit, something is in our way
				//see if it's one of ours
				int j;

				for ( j = 0; j < aiEnt->NPC->group->numGroup; j++ )
				{
					if ( aiEnt->NPC->group->member[j].number == aiEnt->NPC->blockingEntNum )
					{//we're being blocked by one of our own, pass our goal onto them and I'll stand still
						ST_TransferMoveGoal( aiEnt, &g_entities[aiEnt->NPC->group->member[j].number] );
						break;
					}
				}
			}

			ST_HoldPosition(aiEnt);
		}
	}
	else
	{
		//First time you successfully move, say what it is you're doing
		NPC_ST_SayMovementSpeech(aiEnt);
	}

	return moved;
}


/*
-------------------------
NPC_ST_SleepShuffle
-------------------------
*/

static void NPC_ST_SleepShuffle(gentity_t *aiEnt)
{
	//Play an awake script if we have one
	if ( G_ActivateBehavior( aiEnt, BSET_AWAKE) )
	{
		return;
	}

	//Automate some movement and noise
	if ( TIMER_Done( aiEnt, "shuffleTime" ) )
	{

		//TODO: Play sleeping shuffle animation

		//int	soundIndex = Q_irand( 0, 1 );

		/*
		switch ( soundIndex )
		{
		case 0:
			G_Sound( NPC, G_SoundIndex("sound/chars/imperialsleeper1/scav4/hunh.mp3") );
			break;

		case 1:
			G_Sound( NPC, G_SoundIndex("sound/chars/imperialsleeper3/scav4/tryingtosleep.wav") );
			break;
		}
		*/

		TIMER_Set( aiEnt, "shuffleTime", 4000 );
		TIMER_Set( aiEnt, "sleepTime", 2000 );
		return;
	}

	//They made another noise while we were stirring, see if we can see them
	if ( TIMER_Done( aiEnt, "sleepTime" ) )
	{
		NPC_CheckPlayerTeamStealth(aiEnt);
		TIMER_Set( aiEnt, "sleepTime", 2000 );
	}
}

/*
-------------------------
NPC_ST_Sleep
-------------------------
*/

void NPC_BSST_Sleep(gentity_t *aiEnt)
{
	int alertEvent = NPC_CheckAlertEvents(aiEnt, qfalse, qtrue, -1, qfalse, AEL_MINOR );//only check sounds since we're alseep!

	//There is an event we heard
	if ( alertEvent >= 0 )
	{
		//See if it was enough to wake us up
		if ( level.alertEvents[alertEvent].level == AEL_DISCOVERED && (aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
		{
			int			i;
			float		dist;
			float		bestDist	= 16384.0f;
			gentity_t	*bestCl		= NULL;
			gentity_t	*ent		= NULL;

			for ( i=0; i<MAX_CLIENTS; i++ ) {
				ent = &g_entities[i];
				if ( ent->inuse && ent->health > 0 && !(ent->client->ps.eFlags & EF_DEAD) &&
#ifdef __NPC_CPU_USAGE_TWEAKS__
					G_ClearLOS4( aiEnt, ent ) 
#else //!__NPC_CPU_USAGE_TWEAKS__
					G_ClearLOS(aiEnt, aiEnt->s.origin, ent->s.origin)
#endif //__NPC_CPU_USAGE_TWEAKS__
					)
				{
					if ( ( dist = Distance( aiEnt->s.origin, ent->s.origin ) ) < bestDist )
					{
						bestCl		= ent;
						bestDist	= dist;
					}
				}
			}

			if ( bestCl )
			{
				G_SetEnemy( aiEnt, bestCl );
				return;
			}
		}

		//Otherwise just stir a bit
		NPC_ST_SleepShuffle(aiEnt);
		return;
	}
}

/*
-------------------------
NPC_CheckEnemyStealth
-------------------------
*/

qboolean NPC_CheckEnemyStealth(gentity_t *aiEnt, gentity_t *target )
{
	float		target_dist, minDist = 40;//any closer than 40 and we definitely notice
	float		maxViewDist;
	qboolean	clearLOS;

	//In case we aquired one some other way
	if ( aiEnt->enemy != NULL )
		return qtrue;

	//Ignore notarget
	if ( target->flags & FL_NOTARGET )
		return qfalse;

	if ( target->health <= 0 )
	{
		return qfalse;
	}

	if ( target->client->ps.weapon == WP_SABER && !target->client->ps.saberHolstered && !target->client->ps.saberInFlight )
	{//if target has saber in hand and activated, we wake up even sooner even if not facing him
		minDist = 100;
	}

	target_dist = DistanceSquared( target->r.currentOrigin, aiEnt->r.currentOrigin );

	//If the target is this close, then wake up regardless
	if ( !(target->client->ps.pm_flags&PMF_DUCKED)
		&& (aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES)
		&& (target_dist) < (minDist*minDist) )
	{
		G_SetEnemy( aiEnt, target );
		aiEnt->NPC->enemyLastSeenTime = level.time;
		TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}

	maxViewDist			= MAX_VIEW_DIST;

	if ( aiEnt->NPC->stats.visrange > maxViewDist )
	{//FIXME: should we always just set maxViewDist to this?
		maxViewDist = aiEnt->NPC->stats.visrange;
	}

	if ( target_dist > (maxViewDist*maxViewDist) )
	{//out of possible visRange
		return qfalse;
	}

	//Check FOV first
	if ( InFOV( target, aiEnt, aiEnt->NPC->stats.hfov, aiEnt->NPC->stats.vfov ) == qfalse )
		return qfalse;

	//clearLOS = ( target->client->ps.leanofs ) ? NPC_ClearLOS5( target->client->renderInfo.eyePoint ) : NPC_ClearLOS4( target );
	clearLOS = NPC_ClearLOS4(aiEnt, target );

	//Now check for clear line of vision
	if ( clearLOS )
	{
		vec3_t	targ_org;
		int		target_crouching;
		float	hAngle_perc, vAngle_perc, FOV_perc;
		float	target_speed;
		float	dist_rating, speed_rating, turning_rating;
		float	light_level;
		float	vis_rating, target_rating;
		float	dist_influence, fov_influence, light_influence;
		float	realize, cautious;
		int		contents;

		if ( target->client->NPC_class == CLASS_ATST_OLD)
		{//can't miss 'em!
			G_SetEnemy( aiEnt, target );
			TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
			return qtrue;
		}
		VectorSet(targ_org, target->r.currentOrigin[0],target->r.currentOrigin[1],target->r.currentOrigin[2]+target->r.maxs[2]-4);
		hAngle_perc			= NPC_GetHFOVPercentage( targ_org, aiEnt->client->renderInfo.eyePoint, aiEnt->client->renderInfo.eyeAngles, aiEnt->NPC->stats.hfov );
		vAngle_perc			= NPC_GetVFOVPercentage( targ_org, aiEnt->client->renderInfo.eyePoint, aiEnt->client->renderInfo.eyeAngles, aiEnt->NPC->stats.vfov );

		//Scale them vertically some, and horizontally pretty harshly
		vAngle_perc *= vAngle_perc;//( vAngle_perc * vAngle_perc );
		hAngle_perc *= ( hAngle_perc * hAngle_perc );

		//Cap our vertical vision severely
		//if ( vAngle_perc <= 0.3f ) // was 0.5f
		//	return qfalse;

		//Assess the player's current status
		target_dist			= Distance( target->r.currentOrigin, aiEnt->r.currentOrigin );

		target_speed		= VectorLength( target->client->ps.velocity );
		target_crouching	= ( target->client->pers.cmd.upmove < 0 );
		dist_rating			= ( target_dist / maxViewDist );
		speed_rating		= ( target_speed / MAX_VIEW_SPEED );
		turning_rating		= 5.0f;//AngleDelta( target->client->ps.viewangles[PITCH], target->lastAngles[PITCH] )/180.0f + AngleDelta( target->client->ps.viewangles[YAW], target->lastAngles[YAW] )/180.0f;
		light_level			= (255/MAX_LIGHT_INTENSITY); //( target->lightLevel / MAX_LIGHT_INTENSITY );
		FOV_perc			= 1.0f - ( hAngle_perc + vAngle_perc ) * 0.5f;	//FIXME: Dunno about the average...
		vis_rating			= 0.0f;

		//Too dark
		if ( light_level < MIN_LIGHT_THRESHOLD )
			return qfalse;

		//Too close?
		if ( dist_rating < DISTANCE_THRESHOLD )
		{
			G_SetEnemy( aiEnt, target );
			TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
			return qtrue;
		}

		//Out of range
		if ( dist_rating > 1.0f )
			return qfalse;

		//Cap our speed checks
		if ( speed_rating > 1.0f )
			speed_rating = 1.0f;


		//Calculate the distance, fov and light influences
		//...Visibilty linearly wanes over distance
		dist_influence	= DISTANCE_SCALE * ( ( 1.0f - dist_rating ) );
		//...As the percentage out of the FOV increases, straight perception suffers on an exponential scale
		fov_influence		= FOV_SCALE * ( 1.0f - FOV_perc );
		//...Lack of light hides, abundance of light exposes
		light_influence	= ( light_level - 0.5f ) * LIGHT_SCALE;

		//Calculate our base rating
		target_rating		= dist_influence + fov_influence + light_influence;

		//Now award any final bonuses to this number
		contents = trap->PointContents( targ_org, target->s.number );
		if ( contents&CONTENTS_WATER )
		{
			int myContents = trap->PointContents( aiEnt->client->renderInfo.eyePoint, aiEnt->s.number );
			if ( !(myContents&CONTENTS_WATER) )
			{//I'm not in water
				if ( aiEnt->client->NPC_class == CLASS_SWAMPTROOPER )
				{//these guys can see in in/through water pretty well
					vis_rating = 0.10f;//10% bonus
				}
				else
				{
					vis_rating = 0.35f;//35% bonus
				}
			}
			else
			{//else, if we're both in water
				if ( aiEnt->client->NPC_class == CLASS_SWAMPTROOPER )
				{//I can see him just fine
				}
				else
				{
					vis_rating = 0.15f;//15% bonus
				}
			}
		}
		else
		{//not in water
			if ( contents&CONTENTS_FOG )
			{
				vis_rating = 0.15f;//15% bonus
			}
		}

		target_rating *= (1.0f - vis_rating);

		//...Motion draws the eye quickly
		target_rating += speed_rating * SPEED_SCALE;
		target_rating += turning_rating * TURNING_SCALE;
		//FIXME: check to see if they're animating, too?  But can we do something as simple as frame != oldframe?

		//...Smaller targets are harder to indentify
		if ( target_crouching )
		{
			target_rating *= 0.9f;	//10% bonus
		}

		//If he's violated the threshold, then realize him
		//float difficulty_scale = 1.0f + (2.0f-g_npcspskill.value);//if playing on easy, 20% harder to be seen...?
		if ( aiEnt->client->NPC_class == CLASS_SWAMPTROOPER )
		{//swamptroopers can see much better
			realize = (float)CAUTIOUS_THRESHOLD/**difficulty_scale*/;
			cautious = (float)CAUTIOUS_THRESHOLD * 0.75f/**difficulty_scale*/;
		}
		else
		{
			realize = (float)REALIZE_THRESHOLD/**difficulty_scale*/;
			cautious = (float)CAUTIOUS_THRESHOLD * 0.75f/**difficulty_scale*/;
		}

		if ( target_rating > realize && (aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
		{
			G_SetEnemy( aiEnt, target );
			aiEnt->NPC->enemyLastSeenTime = level.time;
			TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
			return qtrue;
		}

		//If he's above the caution threshold, then realize him in a few seconds unless he moves to cover
		if ( target_rating > cautious && !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
		{//FIXME: ambushing guys should never talk
			if ( TIMER_Done( aiEnt, "enemyLastVisible" ) )
			{//If we haven't already, start the counter
				int	lookTime = Q_irand( 4500, 8500 );
				//NPCInfo->timeEnemyLastVisible = level.time + 2000;
				TIMER_Set( aiEnt, "enemyLastVisible", lookTime );
				//TODO: Play a sound along the lines of, "Huh?  What was that?"
				ST_Speech( aiEnt, SPEECH_SIGHT, 0 );
				NPC_TempLookTarget( aiEnt, target->s.number, lookTime, lookTime );
				//FIXME: set desired yaw and pitch towards this guy?
			}
			else if ( TIMER_Get( aiEnt, "enemyLastVisible" ) <= level.time + 500 && (aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES) )	//FIXME: Is this reliable?
			{
				if ( aiEnt->NPC->rank < RANK_LT && !Q_irand( 0, 2 ) )
				{
					int	interrogateTime = Q_irand( 2000, 4000 );
					ST_Speech( aiEnt, SPEECH_SUSPICIOUS, 0 );
					TIMER_Set( aiEnt, "interrogating", interrogateTime );
					G_SetEnemy( aiEnt, target );
					aiEnt->NPC->enemyLastSeenTime = level.time;
					TIMER_Set( aiEnt, "attackDelay", interrogateTime );
					TIMER_Set( aiEnt, "stand", interrogateTime );
				}
				else
				{
					G_SetEnemy( aiEnt, target );
					aiEnt->NPC->enemyLastSeenTime = level.time;
					//FIXME: ambush guys (like those popping out of water) shouldn't delay...
					TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
					TIMER_Set( aiEnt, "stand", Q_irand( 500, 2500 ) );
				}
				return qtrue;
			}

			return qfalse;
		}
	}

	return qfalse;
}

qboolean NPC_CheckPlayerTeamStealth(gentity_t *aiEnt)
{
	/*
	//NOTENOTE: For now, all stealh checks go against the player, since
	//			he is the main focus.  Squad members and rivals do not
	//			fall into this category and will be ignored.

	NPC_CheckEnemyStealth( &g_entities[0] );	//Change this pointer to assess other entities
	*/
	gentity_t *enemy;
	int i;

	for ( i = 0; i < ENTITYNUM_WORLD; i++ )
	{
		enemy = &g_entities[i];

		if (!enemy->inuse)
		{
			continue;
		}

		if ( enemy && enemy->client && ValidEnemy( aiEnt, enemy ) && enemy->client->playerTeam == aiEnt->client->enemyTeam )
		{
			if ( NPC_CheckEnemyStealth( aiEnt, enemy ) )	//Change this pointer to assess other entities
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}
/*
-------------------------
NPC_ST_InvestigateEvent
-------------------------
*/

#define	MAX_CHECK_THRESHOLD	1

static qboolean NPC_ST_InvestigateEvent(gentity_t *aiEnt, int eventID, qboolean extraSuspicious )
{
	//If they've given themselves away, just take them as an enemy
	if ( aiEnt->NPC->confusionTime < level.time )
	{
		if ( level.alertEvents[eventID].level == AEL_DISCOVERED && (aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
		{
			aiEnt->NPC->lastAlertID = level.alertEvents[eventID].ID;
			if ( !level.alertEvents[eventID].owner ||
				!level.alertEvents[eventID].owner->client ||
				level.alertEvents[eventID].owner->health <= 0 ||
				level.alertEvents[eventID].owner->client->playerTeam != aiEnt->client->enemyTeam )
			{//not an enemy
				return qfalse;
			}
			//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
			//ST_Speech( NPC, SPEECH_CHARGE, 0 );
			G_SetEnemy( aiEnt, level.alertEvents[eventID].owner );
			aiEnt->NPC->enemyLastSeenTime = level.time;
			TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
			if ( level.alertEvents[eventID].type == AET_SOUND )
			{//heard him, didn't see him, stick for a bit
				TIMER_Set( aiEnt, "roamTime", Q_irand( 500, 2500 ) );
			}
			return qtrue;
		}
	}

	//don't look at the same alert twice
	if ( level.alertEvents[eventID].ID == aiEnt->NPC->lastAlertID )
	{
		return qfalse;
	}
	aiEnt->NPC->lastAlertID = level.alertEvents[eventID].ID;

	//Must be ready to take another sound event
	/*
	if ( NPCInfo->investigateSoundDebounceTime > level.time )
	{
		return qfalse;
	}
	*/

	if ( level.alertEvents[eventID].type == AET_SIGHT )
	{//sight alert, check the light level
		if ( level.alertEvents[eventID].light < Q_irand( ST_MIN_LIGHT_THRESHOLD, ST_MAX_LIGHT_THRESHOLD ) )
		{//below my threshhold of potentially seeing
			return qfalse;
		}
	}

	//Save the position for movement (if necessary)
	VectorCopy( level.alertEvents[eventID].position, aiEnt->NPC->investigateGoal );

	//First awareness of it
	aiEnt->NPC->investigateCount += ( extraSuspicious ) ? 2 : 1;

	//Clamp the value
	if ( aiEnt->NPC->investigateCount > 4 )
		aiEnt->NPC->investigateCount = 4;

	//See if we should walk over and investigate
	if ( level.alertEvents[eventID].level > AEL_MINOR && aiEnt->NPC->investigateCount > 1 && (aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )
	{
		//make it so they can walk right to this point and look at it rather than having to use combatPoints
		if ( G_ExpandPointToBBox( aiEnt->NPC->investigateGoal, aiEnt->r.mins, aiEnt->r.maxs, aiEnt->s.number, ((aiEnt->clipmask&~CONTENTS_BODY)|CONTENTS_BOTCLIP) ) )
		{//we were able to move the investigateGoal to a point in which our bbox would fit
			//drop the goal to the ground so we can get at it
			vec3_t	end;
			trace_t	trace;
			VectorCopy( aiEnt->NPC->investigateGoal, end );
			end[2] -= 512;//FIXME: not always right?  What if it's even higher, somehow?
			trap->Trace( &trace, aiEnt->NPC->investigateGoal, aiEnt->r.mins, aiEnt->r.maxs, end, ENTITYNUM_NONE, ((aiEnt->clipmask&~CONTENTS_BODY)|CONTENTS_BOTCLIP), qfalse, 0, 0 );
			if ( trace.fraction >= 1.0f )
			{//too high to even bother
				//FIXME: look at them???
			}
			else
			{
				VectorCopy( trace.endpos, aiEnt->NPC->investigateGoal );
				NPC_SetMoveGoal( aiEnt, aiEnt->NPC->investigateGoal, 16, qtrue, -1, NULL );
				aiEnt->NPC->localState = LSTATE_INVESTIGATE;
			}
		}
		else
		{
			int id = NPC_FindCombatPoint(aiEnt, aiEnt->NPC->investigateGoal, aiEnt->NPC->investigateGoal, aiEnt->NPC->investigateGoal, CP_INVESTIGATE|CP_HAS_ROUTE, 0, -1 );

			if ( id != -1 )
			{
				NPC_SetMoveGoal( aiEnt, level.combatPoints[id].origin, 16, qtrue, id, NULL );
				aiEnt->NPC->localState = LSTATE_INVESTIGATE;
			}
		}
		//Say something
		//FIXME: only if have others in group... these should be responses?
		if ( aiEnt->NPC->investigateDebounceTime+aiEnt->NPC->pauseTime > level.time )
		{//was already investigating
			if ( aiEnt->NPC->group &&
				aiEnt->NPC->group->commander &&
				aiEnt->NPC->group->commander->client &&
				aiEnt->NPC->group->commander->client->NPC_class == CLASS_IMPERIAL &&
				!Q_irand( 0, 3 ) )
			{
				ST_Speech( aiEnt->NPC->group->commander, SPEECH_LOOK, 0 );//FIXME: "I'll go check it out" type sounds
			}
			else
			{
				ST_Speech( aiEnt, SPEECH_LOOK, 0 );//FIXME: "I'll go check it out" type sounds
			}
		}
		else
		{
			if ( level.alertEvents[eventID].type == AET_SIGHT )
			{
				ST_Speech( aiEnt, SPEECH_SIGHT, 0 );
			}
			else if ( level.alertEvents[eventID].type == AET_SOUND )
			{
				ST_Speech( aiEnt, SPEECH_SOUND, 0 );
			}
		}
		//Setup the debounce info
		aiEnt->NPC->investigateDebounceTime		= aiEnt->NPC->investigateCount * 5000;
		aiEnt->NPC->investigateSoundDebounceTime	= level.time + 2000;
		aiEnt->NPC->pauseTime						= level.time;
	}
	else
	{//just look?
		//Say something
		if ( level.alertEvents[eventID].type == AET_SIGHT )
		{
			ST_Speech( aiEnt, SPEECH_SIGHT, 0 );
		}
		else if ( level.alertEvents[eventID].type == AET_SOUND )
		{
			ST_Speech( aiEnt, SPEECH_SOUND, 0 );
		}
		//Setup the debounce info
		aiEnt->NPC->investigateDebounceTime		= aiEnt->NPC->investigateCount * 1000;
		aiEnt->NPC->investigateSoundDebounceTime	= level.time + 1000;
		aiEnt->NPC->pauseTime						= level.time;
		VectorCopy( level.alertEvents[eventID].position, aiEnt->NPC->investigateGoal );
	}

	if ( level.alertEvents[eventID].level >= AEL_DANGER )
	{
		aiEnt->NPC->investigateDebounceTime = Q_irand( 500, 2500 );
	}

	//Start investigating
	aiEnt->NPC->tempBehavior = BS_INVESTIGATE;
	return qtrue;
}

/*
-------------------------
ST_OffsetLook
-------------------------
*/

static void ST_OffsetLook(gentity_t *aiEnt, float offset, vec3_t out )
{
	vec3_t	angles, forward, temp;

	GetAnglesForDirection( aiEnt->r.currentOrigin, aiEnt->NPC->investigateGoal, angles );
	angles[YAW] += offset;
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( aiEnt->r.currentOrigin, 64, forward, out );

	CalcEntitySpot( aiEnt, SPOT_HEAD, temp );
	out[2] = temp[2];
}

/*
-------------------------
ST_LookAround
-------------------------
*/

static void ST_LookAround(gentity_t *aiEnt)
{
	vec3_t	lookPos;
	float	perc = (float) ( level.time - aiEnt->NPC->pauseTime ) / (float) aiEnt->NPC->investigateDebounceTime;

	//Keep looking at the spot
	if ( perc < 0.25 )
	{
		VectorCopy( aiEnt->NPC->investigateGoal, lookPos );
	}
	else if ( perc < 0.5f )		//Look up but straight ahead
	{
		ST_OffsetLook(aiEnt, 0.0f, lookPos );
	}
	else if ( perc < 0.75f )	//Look right
	{
		ST_OffsetLook(aiEnt, 45.0f, lookPos );
	}
	else	//Look left
	{
		ST_OffsetLook(aiEnt,  -45.0f, lookPos );
	}

	NPC_FacePosition(aiEnt,  lookPos, qtrue );
}

/*
-------------------------
NPC_BSST_Investigate
-------------------------
*/

void NPC_BSST_Investigate(gentity_t *aiEnt)
{
	//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
	AI_GetGroup( aiEnt );

	if( aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink(aiEnt, qtrue );
	}

	if ( aiEnt->NPC->confusionTime < level.time )
	{
		if ( aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			//Look for an enemy
			if ( NPC_CheckPlayerTeamStealth(aiEnt) )
			{
				//NPCInfo->behaviorState	= BS_HUNT_AND_KILL;//should be auto now
				ST_Speech( aiEnt, SPEECH_DETECTED, 0 );
				aiEnt->NPC->tempBehavior	= BS_DEFAULT;
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
		}
	}

	if ( !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		int alertEvent = NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, aiEnt->NPC->lastAlertID, qfalse, AEL_MINOR );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			if ( aiEnt->NPC->confusionTime < level.time )
			{
				if ( NPC_CheckForDanger(aiEnt,  alertEvent ) )
				{//running like hell
					ST_Speech( aiEnt, SPEECH_COVER, 0 );//FIXME: flee sound?
					return;
				}
			}

			if ( level.alertEvents[alertEvent].ID != aiEnt->NPC->lastAlertID )
			{
				NPC_ST_InvestigateEvent(aiEnt, alertEvent, qtrue );
			}
		}
	}

	//If we're done looking, then just return to what we were doing
	if ( ( aiEnt->NPC->investigateDebounceTime + aiEnt->NPC->pauseTime ) < level.time )
	{
		aiEnt->NPC->tempBehavior = BS_DEFAULT;
		aiEnt->NPC->goalEntity = UpdateGoal(aiEnt);

		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		//Say something
		ST_Speech( aiEnt, SPEECH_GIVEUP, 0 );
		return;
	}

	//FIXME: else, look for new alerts

	//See if we're searching for the noise's origin
	if ( aiEnt->NPC->localState == LSTATE_INVESTIGATE && (aiEnt->NPC->goalEntity!=NULL) )
	{
		//See if we're there
		if ( NAV_HitNavGoal( aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, aiEnt->NPC->goalEntity->r.currentOrigin, 32, FlyingCreature( aiEnt ) ) == qfalse )
		{
			aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;

			//Try and move there
			if ( NPC_MoveToGoal(aiEnt, qtrue )  )
			{
				//Bump our times
				aiEnt->NPC->investigateDebounceTime	= aiEnt->NPC->investigateCount * 5000;
				aiEnt->NPC->pauseTime					= level.time;

				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
		}

		//Otherwise we're done or have given up
		//Say something
		//ST_Speech( NPC, SPEECH_LOOK, 0.33f );
		aiEnt->NPC->localState = LSTATE_NONE;
	}

	//Look around
	ST_LookAround(aiEnt);
}

/*
-------------------------
NPC_BSST_Patrol
-------------------------
*/

void NPC_BSST_Patrol(gentity_t *aiEnt)
{//FIXME: pick up on bodies of dead buddies?

	//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
	AI_GetGroup( aiEnt );

	if ( aiEnt->NPC->confusionTime < level.time )
	{
		//Look for any enemies
		if ( aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			if ( NPC_CheckPlayerTeamStealth(aiEnt) )
			{
				//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
				//NPC_AngerSound();
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
		}
	}

	if ( !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		int alertEvent = NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_MINOR );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			if ( NPC_ST_InvestigateEvent(aiEnt, alertEvent, qfalse ) )
			{//actually going to investigate it
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
		}
	}

	//If we have somewhere to go, then do that
	if ( UpdateGoal(aiEnt) )
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		//ST_Move( NPCInfo->goalEntity );
		NPC_MoveToGoal(aiEnt, qtrue );
	}
	else// if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		if ( aiEnt->client->NPC_class != CLASS_IMPERIAL && aiEnt->client->NPC_class != CLASS_IMPWORKER )
		{//imperials do not look around
			if ( TIMER_Done( aiEnt, "enemyLastVisible" ) )
			{//nothing suspicious, look around
				if ( !Q_irand( 0, 30 ) )
				{
					aiEnt->NPC->desiredYaw = aiEnt->s.angles[1] + Q_irand( -90, 90 );
				}
				if ( !Q_irand( 0, 30 ) )
				{
					aiEnt->NPC->desiredPitch = Q_irand( -20, 20 );
				}
			}
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
	//TEMP hack for Imperial stand anim
	if ( aiEnt->client->NPC_class == CLASS_IMPERIAL || aiEnt->client->NPC_class == CLASS_IMPWORKER )
	{//hack
		if ( aiEnt->client->pers.cmd.forwardmove || aiEnt->client->pers.cmd.rightmove || aiEnt->client->pers.cmd.upmove )
		{//moving

			if( (aiEnt->client->ps.torsoTimer <= 0) || (aiEnt->client->ps.torsoAnim == BOTH_STAND4) )
			{
				if ( (aiEnt->client->pers.cmd.buttons&BUTTON_WALKING) && !(aiEnt->NPC->scriptFlags&SCF_RUNNING) )
				{//not running, only set upper anim
					//  No longer overrides scripted anims
					NPC_SetAnim( aiEnt, SETANIM_TORSO, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					aiEnt->client->ps.torsoTimer = 200;
				}
			}
		}
		else
		{//standing still, set both torso and legs anim
			//  No longer overrides scripted anims
			if( ( aiEnt->client->ps.torsoTimer <= 0 || (aiEnt->client->ps.torsoAnim == BOTH_STAND4) ) &&
				( aiEnt->client->ps.legsTimer <= 0  || (aiEnt->client->ps.legsAnim == BOTH_STAND4) ) )
			{
				NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				aiEnt->client->ps.torsoTimer = aiEnt->client->ps.legsTimer = 200;
			}
		}
		//FIXME: this is a disgusting hack that is supposed to make the Imperials start with their weapon holstered- need a better way
		if ( aiEnt->client->ps.weapon != WP_NONE )
		{
			ChangeWeapon( aiEnt, WP_NONE );
			aiEnt->client->ps.weapon = WP_NONE;
			aiEnt->client->ps.weaponstate = WEAPON_READY;
			/*
			if ( NPC->weaponModel[0] > 0 )
			{
				trap->G2API_RemoveGhoul2Model( NPC->ghoul2, NPC->weaponModel[0] );
				NPC->weaponModel[0] = -1;
			}
			*/
			//rwwFIXMEFIXME: Do this?
		}
	}
}

/*
-------------------------
NPC_BSST_Idle
-------------------------
*/
/*
void NPC_BSST_Idle( gentity_t *aiEnt )
{
	int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue );

	//There is an event to look at
	if ( alertEvent >= 0 )
	{
		NPC_ST_InvestigateEvent( alertEvent, qfalse );
		NPC_UpdateAngles( aiEnt,qtrue, qtrue );
		return;
	}

	TIMER_Set( NPC, "roamTime", 2000 + Q_irand( 1000, 2000 ) );

	NPC_UpdateAngles( aiEnt,qtrue, qtrue );
}
*/
/*
-------------------------
ST_CheckMoveState
-------------------------
*/

static void ST_CheckMoveState(gentity_t *aiEnt)
{

#ifndef __NO_ICARUS__
	if ( trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) )
	{//moving toward a goal that a script is waiting on, so don't stop for anything!
		move = qtrue;
	}
	//See if we're a scout
	else 
#endif //__NO_ICARUS__
	if ( aiEnt->NPC->squadState == SQUAD_SCOUT )
	{
		//If we're supposed to stay put, then stand there and fire
		if ( TIMER_Done( aiEnt, "stick" ) == qfalse )
		{
			move = qfalse;
			return;
		}

		//Otherwise, if we can see our target, just shoot
		if ( enemyLOS )
		{
			if ( enemyCS )
			{
				//if we're going after our enemy, we can stop now
				if ( aiEnt->NPC->goalEntity == aiEnt->enemy )
				{
					AI_GroupUpdateSquadstates( aiEnt->NPC->group, aiEnt, SQUAD_STAND_AND_SHOOT );
					move = qfalse;
					return;
				}
			}
		}
		else
		{
			//Move to find our target
			faceEnemy = qfalse;
		}

		/*
		if ( TIMER_Done( NPC, "scoutTime" ) )
		{//we can't scout to him, someone else give it a try
			AI_GroupUpdateSquadstates( NPCInfo->group, NPC, SQUAD_STAND_AND_SHOOT );
			TIMER_Set( NPC, "roamTime", Q_irand( 1000, 2000 ) );
			move = qfalse;
			return;
		}
		*/

		//ucmd.buttons |= BUTTON_CAREFUL;
	}
	//See if we're running away
	else if ( aiEnt->NPC->squadState == SQUAD_RETREAT )
	{
		if ( aiEnt->NPC->goalEntity )
		{
			faceEnemy = qfalse;
		}
		else
		{//um, lost our goal?  Just stand and shoot, then
			aiEnt->NPC->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	//see if we're heading to some other combatPoint
	else if ( aiEnt->NPC->squadState == SQUAD_TRANSITION )
	{
		//ucmd.buttons |= BUTTON_CAREFUL;
		if ( !aiEnt->NPC->goalEntity )
		{//um, lost our goal?  Just stand and shoot, then
			aiEnt->NPC->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	//see if we're at point, duck and fire
	else if ( aiEnt->NPC->squadState == SQUAD_POINT )
	{
		if ( TIMER_Done( aiEnt, "stick" ) )
		{
			AI_GroupUpdateSquadstates( aiEnt->NPC->group, aiEnt, SQUAD_STAND_AND_SHOOT );
			return;
		}

		move = qfalse;
		return;
	}
	//see if we're just standing around
	else if ( aiEnt->NPC->squadState == SQUAD_STAND_AND_SHOOT )
	{//from this squadState we can transition to others?
		move = qfalse;
		return;
	}
	//see if we're hiding
	else if ( aiEnt->NPC->squadState == SQUAD_COVER )
	{
		//Should we duck?
		move = qfalse;
		return;
	}
	//see if we're just standing around
	else if ( aiEnt->NPC->squadState == SQUAD_IDLE )
	{
		if ( !aiEnt->NPC->goalEntity )
		{
			move = qfalse;
			return;
		}
	}
	//??
	else
	{//invalid squadState!
	}

	//See if we're moving towards a goal, not the enemy
	if ( ( aiEnt->NPC->goalEntity != aiEnt->enemy ) && ( aiEnt->NPC->goalEntity != NULL ) )
	{
		//Did we make it?
		if ( NAV_HitNavGoal( aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, aiEnt->NPC->goalEntity->r.currentOrigin, 16, FlyingCreature( aiEnt ) ) 
#ifndef __NO_ICARUS__
			|| ( !trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) && aiEnt->NPC->squadState == SQUAD_SCOUT && enemyLOS && enemyDist <= 10000 ) 
#endif //__NO_ICARUS__
			)
		{//either hit our navgoal or our navgoal was not a crucial (scripted) one (maybe a combat point) and we're scouting and found our enemy
			int	newSquadState = SQUAD_STAND_AND_SHOOT;
			//we got where we wanted to go, set timers based on why we were running
			switch ( aiEnt->NPC->squadState )
			{
			case SQUAD_RETREAT://was running away
				//done fleeing, obviously
				TIMER_Set( aiEnt, "duck", (aiEnt->client->pers.maxHealth - aiEnt->health) * 100 );
				TIMER_Set( aiEnt, "hideTime", Q_irand( 3000, 7000 ) );
				TIMER_Set( aiEnt, "flee", -level.time );
				newSquadState = SQUAD_COVER;
				break;
			case SQUAD_TRANSITION://was heading for a combat point
				TIMER_Set( aiEnt, "hideTime", Q_irand( 2000, 4000 ) );
				break;
			case SQUAD_SCOUT://was running after player
				break;
			default:
				break;
			}
			AI_GroupUpdateSquadstates( aiEnt->NPC->group, aiEnt, newSquadState );
			NPC_ReachedGoal(aiEnt);
			//don't attack right away
			TIMER_Set( aiEnt, "attackDelay", Q_irand( 250, 500 ) );	//FIXME: Slant for difficulty levels
			//don't do something else just yet
			TIMER_Set( aiEnt, "roamTime", Q_irand( 1000, 4000 ) );
			return;
		}

		//keep going, hold of roamTimer until we get there
		TIMER_Set( aiEnt, "roamTime", Q_irand( 4000, 8000 ) );
	}
}

void ST_ResolveBlockedShot(gentity_t *aiEnt, int hit )
{
	int	stuckTime;

	//figure out how long we intend to stand here, max
	if ( TIMER_Get( aiEnt, "roamTime" ) > TIMER_Get( aiEnt, "stick" ) )
	{
		stuckTime = TIMER_Get( aiEnt, "roamTime" )-level.time;
	}
	else
	{
		stuckTime = TIMER_Get( aiEnt, "stick" )-level.time;
	}

	if ( TIMER_Done( aiEnt, "duck" ) )
	{//we're not ducking
		if ( AI_GroupContainsEntNum( aiEnt->NPC->group, hit ) )
		{
			gentity_t *member = &g_entities[hit];
			if ( TIMER_Done( member, "duck" ) )
			{//they aren't ducking
				if ( TIMER_Done( member, "stand" ) )
				{//they're not being forced to stand
					//tell them to duck at least as long as I'm not moving
					TIMER_Set( member, "duck", stuckTime );
					return;
				}
			}
		}
	}
	else
	{//maybe we should stand
		if ( TIMER_Done( aiEnt, "stand" ) )
		{//stand for as long as we'll be here
			TIMER_Set( aiEnt, "stand", stuckTime );
			return;
		}
	}
	//Hmm, can't resolve this by telling them to duck or telling me to stand
	//We need to move!
	TIMER_Set( aiEnt, "roamTime", -1 );
	TIMER_Set( aiEnt, "stick", -1 );
	TIMER_Set( aiEnt, "duck", -1 );
	TIMER_Set( aiEnt, "attakDelay", Q_irand( 1000, 3000 ) );
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void ST_CheckFireState(gentity_t *aiEnt)
{
	if ( enemyCS )
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

	//See if we should continue to fire on their last position
	//!TIMER_Done( NPC, "stick" ) ||
	if ( !hitAlly //we're not going to hit an ally
		&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
		&& aiEnt->NPC->enemyLastSeenTime > 0 //we've seen the enemy
		&& aiEnt->NPC->group //have a group
		&& (aiEnt->NPC->group->numState[SQUAD_RETREAT]>0||aiEnt->NPC->group->numState[SQUAD_TRANSITION]>0||aiEnt->NPC->group->numState[SQUAD_SCOUT]>0) )//laying down covering fire
	{
		if ( level.time - aiEnt->NPC->enemyLastSeenTime < 10000 &&//we have seem the enemy in the last 10 seconds
			(!aiEnt->NPC->group || level.time - aiEnt->NPC->group->lastSeenEnemyTime < 10000 ))//we are not in a group or the group has seen the enemy in the last 10 seconds
		{
			if ( !Q_irand( 0, 10 ) )
			{
				//Fire on the last known position
				vec3_t	muzzle, dir, angles;
				qboolean tooClose = qfalse;
				qboolean tooFar = qfalse;
				float distThreshold;
				float dist;

				CalcEntitySpot( aiEnt, SPOT_HEAD, muzzle );
				if ( VectorCompare( impactPos, vec3_origin ) )
				{//never checked ShotEntity this frame, so must do a trace...
					trace_t tr;
					//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
					vec3_t	forward, end;
					AngleVectors( aiEnt->client->ps.viewangles, forward, NULL, NULL );
					VectorMA( muzzle, 8192, forward, end );
					trap->Trace( &tr, muzzle, vec3_origin, vec3_origin, end, aiEnt->s.number, MASK_SHOT, qfalse, 0, 0 );
					VectorCopy( tr.endpos, impactPos );
				}

				//see if impact would be too close to me
				distThreshold = 16384/*128*128*/;//default
				switch ( aiEnt->s.weapon )
				{
				case WP_THERMAL:
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					distThreshold = 65536/*256*256*/;
					break;
				case WP_MODULIZED_WEAPON:
					if ( aiEnt->NPC->scriptFlags&SCF_ALT_FIRE )
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				default:
					break;
				}

				dist = DistanceSquared( impactPos, muzzle );

				if ( dist < distThreshold )
				{//impact would be too close to me
					tooClose = qtrue;
				}
				else if ( level.time - aiEnt->NPC->enemyLastSeenTime > 5000 ||
					(aiEnt->NPC->group && level.time - aiEnt->NPC->group->lastSeenEnemyTime > 5000 ))
				{//we've haven't seen them in the last 5 seconds
					//see if it's too far from where he is
					distThreshold = 65536/*256*256*/;//default
					switch ( aiEnt->s.weapon )
					{
					case WP_TRIP_MINE:
					case WP_DET_PACK:
						distThreshold = 262144/*512*512*/;
						break;
					case WP_MODULIZED_WEAPON:
						if ( aiEnt->NPC->scriptFlags&SCF_ALT_FIRE )
						{
							distThreshold = 262144/*512*512*/;
						}
						break;
					default:
						break;
					}
					dist = DistanceSquared( impactPos, aiEnt->NPC->enemyLastSeenLocation );
					if ( dist > distThreshold )
					{//impact would be too far from enemy
						tooFar = qtrue;
					}
				}

				if ( !tooClose && !tooFar )
				{//okay too shoot at last pos
					VectorSubtract( aiEnt->NPC->enemyLastSeenLocation, muzzle, dir );
					VectorNormalize( dir );
					vectoangles( dir, angles );

					aiEnt->NPC->desiredYaw		= angles[YAW];
					aiEnt->NPC->desiredPitch	= angles[PITCH];

					shoot = qtrue;
					faceEnemy = qfalse;
					//AI_GroupUpdateSquadstates( NPCInfo->group, NPC, SQUAD_STAND_AND_SHOOT );
					return;
				}
			}
		}
	}
}

void ST_TrackEnemy( gentity_t *self, vec3_t enemyPos )
{
	gentity_t *aiEnt = self;
	//clear timers
	TIMER_Set( self, "attackDelay", Q_irand( 1000, 2000 ) );
	//TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stick", Q_irand( 500, 1500 ) );
	TIMER_Set( self, "stand", -1 );
	TIMER_Set( self, "scoutTime", TIMER_Get( self, "stick" )-level.time+Q_irand(5000, 10000) );
	//leave my combat point
	NPC_FreeCombatPoint(aiEnt, self->NPC->combatPoint, qfalse );
	//go after his last seen pos
	NPC_SetMoveGoal( self, enemyPos, 16, qfalse, -1, NULL );
}

int ST_ApproachEnemy( gentity_t *self )
{
	gentity_t *aiEnt = self;
	TIMER_Set( self, "attackDelay", Q_irand( 250, 500 ) );
	//TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stick", Q_irand( 1000, 2000 ) );
	TIMER_Set( self, "stand", -1 );
	TIMER_Set( self, "scoutTime", TIMER_Get( self, "stick" )-level.time+Q_irand(5000, 10000) );
	//leave my combat point
	NPC_FreeCombatPoint(aiEnt, self->NPC->combatPoint, qfalse );
	//return the relevant combat point flags
	return (CP_CLEAR|CP_CLOSEST);
}

void ST_HuntEnemy( gentity_t *self )
{
	gentity_t *aiEnt = self;
	//TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );//Disabled this for now, guys who couldn't hunt would never attack
	//TIMER_Set( NPC, "duck", -1 );
	TIMER_Set( self, "stick", Q_irand( 250, 1000 ) );
	TIMER_Set( self, "stand", -1 );
	TIMER_Set( self, "scoutTime", TIMER_Get( self, "stick" )-level.time+Q_irand(5000, 10000) );
	//leave my combat point
	NPC_FreeCombatPoint(aiEnt, aiEnt->NPC->combatPoint, qfalse );
	//go directly after the enemy
	if ( aiEnt->NPC->scriptFlags & SCF_CHASE_ENEMIES )
	{
		self->NPC->goalEntity = aiEnt->enemy;
	}
}

void ST_TransferTimers( gentity_t *self, gentity_t *other )
{
	TIMER_Set( other, "attackDelay", TIMER_Get( self, "attackDelay" )-level.time );
	TIMER_Set( other, "duck", TIMER_Get( self, "duck" )-level.time );
	TIMER_Set( other, "stick", TIMER_Get( self, "stick" )-level.time );
	TIMER_Set( other, "scoutTime", TIMER_Get( self, "scout" )-level.time );
	TIMER_Set( other, "roamTime", TIMER_Get( self, "roamTime" )-level.time );
	TIMER_Set( other, "stand", TIMER_Get( self, "stand" )-level.time );
	TIMER_Set( self, "attackDelay", -1 );
	TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stick", -1 );
	TIMER_Set( self, "scoutTime", -1 );
	TIMER_Set( self, "roamTime", -1 );
	TIMER_Set( self, "stand", -1 );
}

void ST_TransferMoveGoal( gentity_t *self, gentity_t *other )
{
#ifndef __NO_ICARUS__
	if ( trap->ICARUS_TaskIDPending( (sharedEntity_t *)self, TID_MOVE_NAV ) )
	{//can't transfer movegoal when a script we're running is waiting to complete
		return;
	}
#endif //__NO_ICARUS__
	if ( self->NPC->combatPoint != -1 )
	{//I've got a combatPoint I'm going to, give it to him
		self->NPC->lastFailedCombatPoint = other->NPC->combatPoint = self->NPC->combatPoint;
		self->NPC->combatPoint = -1;
	}
	else
	{//I must be going for a goal, give that to him instead
		if ( self->NPC->goalEntity == self->NPC->tempGoal )
		{
			NPC_SetMoveGoal( other, self->NPC->tempGoal->r.currentOrigin, self->NPC->goalRadius, ((self->NPC->tempGoal->flags&FL_NAVGOAL)?qtrue:qfalse), -1, NULL );
		}
		else
		{
			other->NPC->goalEntity = self->NPC->goalEntity;
		}
	}
	//give him my squadstate
	AI_GroupUpdateSquadstates( self->NPC->group, other, self->NPC->squadState );

	//give him my timers and clear mine
	ST_TransferTimers( self, other );

	//now make me stand around for a second or two at least
	AI_GroupUpdateSquadstates( self->NPC->group, self, SQUAD_STAND_AND_SHOOT );
	TIMER_Set( self, "stand", Q_irand( 1000, 3000 ) );
}

int ST_GetCPFlags(gentity_t *aiEnt)
{
	int cpFlags = 0;
	if ( aiEnt && aiEnt->NPC->group )
	{
		if ( aiEnt == aiEnt->NPC->group->commander && aiEnt->client->NPC_class == CLASS_IMPERIAL )
		{//imperials hang back and give orders
			if ( aiEnt->NPC->group->numGroup > 1 && Q_irand( -3, aiEnt->NPC->group->numGroup ) > 1 )
			{//FIXME: make sure he;s giving orders with these lines
				if ( Q_irand( 0, 1 ) )
				{
					ST_Speech( aiEnt, SPEECH_CHASE, 0.5 );
				}
				else
				{
					ST_Speech( aiEnt, SPEECH_YELL, 0.5 );
				}
			}
			cpFlags = (CP_CLEAR|CP_COVER|CP_AVOID|CP_SAFE|CP_RETREAT);
		}
		else if ( aiEnt->NPC->group->morale < 0 )
		{//hide
			cpFlags = (CP_COVER|CP_AVOID|CP_SAFE|CP_RETREAT);
		}
		else if ( aiEnt->NPC->group->morale < aiEnt->NPC->group->numGroup )
		{//morale is low for our size
			int moraleDrop = aiEnt->NPC->group->numGroup - aiEnt->NPC->group->morale;
			if ( moraleDrop < -6 )
			{//flee (no clear shot needed)
				cpFlags = (CP_FLEE|CP_RETREAT|CP_COVER|CP_AVOID|CP_SAFE);
			}
			else if ( moraleDrop < -3 )
			{//retreat (no clear shot needed)
				cpFlags = (CP_RETREAT|CP_COVER|CP_AVOID|CP_SAFE);
			}
			else if ( moraleDrop < 0 )
			{//cover (no clear shot needed)
				cpFlags = (CP_COVER|CP_AVOID|CP_SAFE);
			}
		}
		else
		{
			int moraleBoost = aiEnt->NPC->group->morale - aiEnt->NPC->group->numGroup;
			if ( moraleBoost > 20 )
			{//charge to any one and outflank (no cover needed)
				cpFlags = (CP_CLEAR|CP_FLANK|CP_APPROACH_ENEMY);
			}
			else if ( moraleBoost > 15 )
			{//charge to closest one (no cover needed)
				cpFlags = (CP_CLEAR|CP_CLOSEST|CP_APPROACH_ENEMY);
			}
			else if ( moraleBoost > 10 )
			{//charge closer (no cover needed)
				cpFlags = (CP_CLEAR|CP_APPROACH_ENEMY);
			}
		}
	}
	if ( !cpFlags )
	{
		//at some medium level of morale
		switch( Q_irand( 0, 3 ) )
		{
		case 0://just take the nearest one
			cpFlags = (CP_CLEAR|CP_COVER|CP_NEAREST);
			break;
		case 1://take one closer to the enemy
			cpFlags = (CP_CLEAR|CP_COVER|CP_APPROACH_ENEMY);
			break;
		case 2://take the one closest to the enemy
			cpFlags = (CP_CLEAR|CP_COVER|CP_CLOSEST|CP_APPROACH_ENEMY);
			break;
		case 3://take the one on the other side of the enemy
			cpFlags = (CP_CLEAR|CP_COVER|CP_FLANK|CP_APPROACH_ENEMY);
			break;
		}
	}
	if ( aiEnt && (aiEnt->NPC->scriptFlags&SCF_USE_CP_NEAREST) )
	{
		cpFlags &= ~(CP_FLANK|CP_APPROACH_ENEMY|CP_CLOSEST);
		cpFlags |= CP_NEAREST;
	}
	return cpFlags;
}
/*
-------------------------
ST_Commander

  Make decisions about who should go where, etc.

FIXME: leader (group-decision-making) AI?
FIXME: need alternate routes!
FIXME: more group voice interaction
FIXME: work in pairs?

-------------------------
*/
void ST_Commander(gentity_t *aiEnt)
{
	int		i, j;
	int		cp, cpFlags_org, cpFlags;
	AIGroupInfo_t	*group = aiEnt->NPC->group;
	gentity_t	*member;//, *buddy;
	qboolean	runner = qfalse;
	qboolean	enemyLost = qfalse;
	qboolean	enemyProtected = qfalse;
//	qboolean	scouting = qfalse;
	int			squadState;
	int			curMemberNum, lastMemberNum;
	float		avoidDist;

	group->processed = qtrue;

	if ( group->enemy == NULL || group->enemy->client == NULL )
	{//hmm, no enemy...?!
		return;
	}

	//FIXME: have this group commander check the enemy group (if any) and see if they have
	//		superior numbers.  If they do, fall back rather than advance.  If you have
	//		superior numbers, advance on them.
	//FIXME: find the group commander and have him occasionally give orders when there is speech
	//FIXME: start fleeing when only a couple of you vs. a lightsaber, possibly give up if the only one left

	if ( group->lastSeenEnemyTime < level.time - 180000 )
	{//dissolve the group
		ST_Speech( aiEnt, SPEECH_LOST, 0.0f );
		group->enemy->waypoint = NAV_FindClosestWaypointForEnt( group->enemy, WAYPOINT_NONE );
		for ( i = 0; i < group->numGroup; i++ )
		{
			member = &g_entities[group->member[i].number];

#ifndef __NO_ICARUS__
			if ( trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) )
			{//running somewhere that a script requires us to go, don't break from that
				continue;
			}
#endif //__NO_ICARUS__
			if ( !(aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )
			{//not allowed to move on my own
				continue;
			}
			//Lost enemy for three minutes?  go into search mode?
			G_ClearEnemy( aiEnt );
			aiEnt->waypoint = NAV_FindClosestWaypointForEnt( aiEnt, group->enemy->waypoint );
			if ( aiEnt->waypoint == WAYPOINT_NONE )
			{
				aiEnt->NPC->behaviorState = BS_DEFAULT;//BS_PATROL;
			}
			else if ( group->enemy->waypoint == WAYPOINT_NONE || (trap->Nav_GetPathCost( aiEnt->waypoint, group->enemy->waypoint ) >= Q3_INFINITE) )
			{
				NPC_BSSearchStart(aiEnt, aiEnt->waypoint, BS_SEARCH );
			}
			else
			{
				NPC_BSSearchStart(aiEnt, group->enemy->waypoint, BS_SEARCH );
			}
		}
		group->enemy = NULL;
		return;
	}


	//See if anyone in our group is not alerted and alert them
	/*
	for ( i = 0; i < group->numGroup; i++ )
	{
		member = &g_entities[group->member[i].number];
		if ( !member->enemy )
		{//he's not mad, so get him mad
			//Have his buddy tell him to get mad
			if ( group->member[i].closestBuddy != ENTITYNUM_NONE )
			{
				buddy = &g_entities[group->member[i].closestBuddy];
				if ( buddy->enemy == group->enemy )
				{
					ST_Speech( NPC, SPEECH_CHARGE, 0.7f );
				}
			}
			G_SetEnemy( member, group->enemy );
		}
	}
	*/
	//Okay, everyone is mad

	//see if anyone is running
	if ( group->numState[SQUAD_SCOUT] > 0 ||
		group->numState[SQUAD_TRANSITION] > 0 ||
		group->numState[SQUAD_RETREAT] > 0 )
	{//someone is running
		runner = qtrue;
	}

	if ( /*!runner &&*/ group->lastSeenEnemyTime > level.time - 32000 && group->lastSeenEnemyTime < level.time - 30000 )
	{//no-one has seen the enemy for 30 seconds// and no-one is running after him
		if ( group->commander && !Q_irand( 0, 1 ) )
		{
			ST_Speech( group->commander, SPEECH_ESCAPING, 0.0f );
		}
		else
		{
			ST_Speech( aiEnt, SPEECH_ESCAPING, 0.0f );
		}
		//don't say this again
		aiEnt->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}

	if ( group->lastSeenEnemyTime < level.time - 10000 )
	{//no-one has seen the enemy for at least 10 seconds!  Should send a scout
		enemyLost = qtrue;
	}

	if ( group->lastClearShotTime < level.time - 5000 )
	{//no-one has had a clear shot for 5 seconds!
		enemyProtected = qtrue;
	}

	//Go through the list:

	//Everyone should try to get to a combat point if possible
	if ( d_asynchronousGroupAI.integer )
	{//do one member a turn
		group->activeMemberNum++;
		if ( group->activeMemberNum >= group->numGroup )
		{
			group->activeMemberNum = 0;
		}
		curMemberNum = group->activeMemberNum;
		lastMemberNum = curMemberNum + 1;
	}
	else
	{
		curMemberNum = 0;
		lastMemberNum = group->numGroup;
	}
	for ( i = curMemberNum; i < lastMemberNum; i++ )
	{
		//reset combat point flags
		cp = -1;
		cpFlags = 0;
		squadState = SQUAD_IDLE;
		avoidDist = 0;
	//	scouting = qfalse;

		//get the next guy
		member = &g_entities[group->member[i].number];
		if ( !member->enemy )
		{//don't include guys that aren't angry
			continue;
		}

		if ( !TIMER_Done( aiEnt, "flee" ) )
		{//running away
			continue;
		}

#ifndef __NO_ICARUS__
		if ( trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) )
		{//running somewhere that a script requires us to go
			continue;
		}
#endif //__NO_ICARUS__

		if ( aiEnt->s.weapon == WP_NONE
			&& aiEnt->NPC->goalEntity
			&& aiEnt->NPC->goalEntity == aiEnt->NPC->tempGoal
			&& aiEnt->NPC->goalEntity->enemy
			&& aiEnt->NPC->goalEntity->enemy->s.eType == ET_ITEM )
		{//running to pick up a gun, don't do other logic
			continue;
		}

		//see if this member should start running (only if have no officer... FIXME: should always run from AEL_DANGER_GREAT?)
		if ( !group->commander || group->commander->NPC->rank < RANK_ENSIGN )
		{
			if ( NPC_CheckForDanger(aiEnt, NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
			{//going to run
				ST_Speech( aiEnt, SPEECH_COVER, 0 );
				continue;
			}
		}

		if ( !(aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )
		{//not allowed to do combat-movement
			continue;
		}

		//check the local state
		if ( aiEnt->NPC->squadState != SQUAD_RETREAT )
		{//not already retreating
			if ( aiEnt->client->ps.weapon == WP_NONE )
			{//weaponless, should be hiding
				if ( aiEnt->NPC->goalEntity == NULL || aiEnt->NPC->goalEntity->enemy == NULL || aiEnt->NPC->goalEntity->enemy->s.eType != ET_ITEM )
				{//not running after a pickup
					if ( TIMER_Done( aiEnt, "hideTime" ) || (DistanceSquared( group->enemy->r.currentOrigin, aiEnt->r.currentOrigin ) < 65536 && NPC_ClearLOS4(aiEnt, aiEnt->enemy )) )
					{//done hiding or enemy near and can see us
						//er, start another flee I guess?
						NPC_StartFlee(aiEnt, aiEnt->enemy, aiEnt->enemy->r.currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
					}//else, just hang here
				}
				continue;
			}
			if ( TIMER_Done( aiEnt, "roamTime" ) && TIMER_Done( aiEnt, "hideTime" ) && aiEnt->health > 10 && !trap->InPVS( group->enemy->r.currentOrigin, aiEnt->r.currentOrigin ) )
			{//cant even see enemy
				//better go after him
				cpFlags |= (CP_CLEAR|CP_COVER);
			}
			else if ( aiEnt->NPC->localState == LSTATE_UNDERFIRE )
			{//we've been shot
				switch( group->enemy->client->ps.weapon )
				{
				case WP_SABER:
					if ( DistanceSquared( group->enemy->r.currentOrigin, aiEnt->r.currentOrigin ) < 65536 )//256 squared
					{
						cpFlags |= (CP_AVOID_ENEMY|CP_COVER|CP_AVOID|CP_RETREAT);
						if ( !group->commander || group->commander->NPC->rank  < RANK_ENSIGN )
						{
							squadState = SQUAD_RETREAT;
						}
						avoidDist = 256;
					}
					break;
				default:
				case WP_MODULIZED_WEAPON:
					cpFlags |= (CP_COVER);
					break;
				}
				if ( aiEnt->health <= 10 )
				{
					if ( !group->commander || group->commander->NPC->rank < RANK_ENSIGN )
					{
						cpFlags |= (CP_FLEE|CP_AVOID|CP_RETREAT);
						squadState = SQUAD_RETREAT;
					}
				}
			}
			else
			{//not hit, see if there are other reasons we should run
				if ( trap->InPVS( aiEnt->r.currentOrigin, group->enemy->r.currentOrigin ) )
				{//in the same room as enemy
					/*if ( aiEnt->client->ps.weapon == WP_ROCKET_LAUNCHER &&
						DistanceSquared( group->enemy->r.currentOrigin, aiEnt->r.currentOrigin ) < MIN_ROCKET_DIST_SQUARED &&
						aiEnt->NPC->squadState != SQUAD_TRANSITION )
					{//too close for me to fire my weapon and I'm not already on the move
						cpFlags |= (CP_AVOID_ENEMY|CP_CLEAR|CP_AVOID);
						avoidDist = 256;
					}
					else*/
					{
						switch( group->enemy->client->ps.weapon )
						{
						case WP_SABER:
							//if ( group->enemy->client->ps.SaberLength() > 0 )
							if (!group->enemy->client->ps.saberHolstered)
							{
								if ( DistanceSquared( group->enemy->r.currentOrigin, aiEnt->r.currentOrigin ) < 65536 )
								{
									if ( TIMER_Done( aiEnt, "hideTime" ) )
									{
										if ( aiEnt->NPC->squadState != SQUAD_TRANSITION )
										{//not already moving: FIXME: we need to see if where we're going is good now?
											cpFlags |= (CP_AVOID_ENEMY|CP_CLEAR|CP_AVOID);
											avoidDist = 256;
										}
									}
								}
							}
						default:
							break;
						}
					}
				}
			}
		}

		if ( !cpFlags )
		{//okay, we have no new enemy-driven reason to run... let's use tactics now
			if ( runner && aiEnt->NPC->combatPoint != -1 )
			{//someone is running and we have a combat point already
				if ( aiEnt->NPC->squadState != SQUAD_SCOUT &&
					aiEnt->NPC->squadState != SQUAD_TRANSITION &&
					aiEnt->NPC->squadState != SQUAD_RETREAT )
				{//it's not us
					if ( TIMER_Done( aiEnt, "verifyCP" ) && DistanceSquared( aiEnt->r.currentOrigin, level.combatPoints[aiEnt->NPC->combatPoint].origin ) > 64*64 )
					{//1 - 3 seconds have passed since you chose a CP, see if you're there since, for some reason, you've stopped running...
						//uh, WTF, we're not on our combat point?
						//er, try again, I guess?
						cp = aiEnt->NPC->combatPoint;
						cpFlags |= ST_GetCPFlags(aiEnt);
					}
					else
					{//cover them
						//stop ducking
						TIMER_Set( aiEnt, "duck", -1 );
						//start shooting
						TIMER_Set( aiEnt, "attackDelay", -1 );
						//AI should take care of the rest - fire at enemy
					}
				}
				else
				{//we're running
					//see if we're blocked
					if ( aiEnt->NPC->aiFlags & NPCAI_BLOCKED )
					{//dammit, something is in our way
						//see if it's one of ours
						for ( j = 0; j < group->numGroup; j++ )
						{
							if ( group->member[j].number == aiEnt->NPC->blockingEntNum )
							{//we're being blocked by one of our own, pass our goal onto them and I'll stand still
								ST_TransferMoveGoal( aiEnt, &g_entities[group->member[j].number] );
								break;
							}
						}
					}
					//we don't need to do anything else
					continue;
				}
			}
			else
			{//okay no-one is running, use some tactics
				if ( aiEnt->NPC->combatPoint != -1 )
				{//we have a combat point we're supposed to be running to
					if ( aiEnt->NPC->squadState != SQUAD_SCOUT &&
						aiEnt->NPC->squadState != SQUAD_TRANSITION &&
						aiEnt->NPC->squadState != SQUAD_RETREAT )
					{//but we're not running
						if ( TIMER_Done( aiEnt, "verifyCP" ) )
						{//1 - 3 seconds have passed since you chose a CP, see if you're there since, for some reason, you've stopped running...
							if ( DistanceSquared( aiEnt->r.currentOrigin, level.combatPoints[aiEnt->NPC->combatPoint].origin ) > 64*64 )
							{//uh, WTF, we're not on our combat point?
								//er, try again, I guess?
								cp = aiEnt->NPC->combatPoint;
								cpFlags |= ST_GetCPFlags(aiEnt);
							}
						}
					}
				}
				if ( enemyLost )
				{//if no-one has seen the enemy for a while, send a scout
					//ask where he went
					if ( group->numState[SQUAD_SCOUT] <= 0 )
					{
					//	scouting = qtrue;
						NPC_ST_StoreMovementSpeech(aiEnt, SPEECH_CHASE, 0.0f );
					}
					//Since no-one else has done this, I should be the closest one, so go after him...
					ST_TrackEnemy( aiEnt, group->enemyLastSeenPos );
					//set me into scout mode
					AI_GroupUpdateSquadstates( group, aiEnt, SQUAD_SCOUT );
					//we're not using a cp, so we need to set runner to true right here
					runner = qtrue;
				}
				else if ( enemyProtected )
				{//if no-one has a clear shot at the enemy, someone should go after him
					//FIXME: if I'm in an area where no safe combat points have a clear shot at me, they don't come after me... they should anyway, though after some extra hesitation.
					//ALSO: seem to give up when behind an area portal?
					//since no-one else here has done this, I should be the closest one
					if ( TIMER_Done( aiEnt, "roamTime" ) && !Q_irand( 0, group->numGroup) )
					{//only do this if we're ready to move again and we feel like it
						cpFlags |= ST_ApproachEnemy( aiEnt );
						//set me into scout mode
						AI_GroupUpdateSquadstates( group, aiEnt, SQUAD_SCOUT );
					}
				}
				else
				{//group can see and has been shooting at the enemy
					//see if we should do something fancy?

					{//we're ready to move
						if ( aiEnt->NPC->combatPoint == -1 )
						{//we're not on a combat point
							if ( 1 )//!Q_irand( 0, 2 ) )
							{//we should go for a combat point
								cpFlags |= ST_GetCPFlags(aiEnt);
							}
							else
							{
								TIMER_Set( aiEnt, "stick", Q_irand( 2000, 4000 ) );
								TIMER_Set( aiEnt, "roamTime", Q_irand( 1000, 3000 ) );
							}
						}
						else if ( TIMER_Done( aiEnt, "roamTime" ) )
						{//we are already on a combat point
							if ( i == 0 )
							{//we're the closest
								if ( (group->morale-group->numGroup>0) && !Q_irand( 0, 4 ) )
								{//try to outflank him
									cpFlags |= (CP_CLEAR|CP_COVER|CP_FLANK|CP_APPROACH_ENEMY);
								}
								else if ( (group->morale-group->numGroup<0) )
								{//better move!
									cpFlags |= ST_GetCPFlags(aiEnt);
								}
								else
								{//If we're point, then get down
									TIMER_Set( aiEnt, "roamTime", Q_irand( 2000, 5000 ) );
									TIMER_Set( aiEnt, "stick", Q_irand( 2000, 5000 ) );
									//FIXME: what if we can't shoot from a ducked pos?
									TIMER_Set( aiEnt, "duck", Q_irand( 3000, 4000 ) );
									AI_GroupUpdateSquadstates( group, aiEnt, SQUAD_POINT );
								}
							}
							else if ( i == group->numGroup - 1 )
							{//farthest from the enemy
								if ( (group->morale-group->numGroup<0) )
								{//low morale, just hang here
									TIMER_Set( aiEnt, "roamTime", Q_irand( 2000, 5000 ) );
									TIMER_Set( aiEnt, "stick", Q_irand( 2000, 5000 ) );
								}
								else if ( (group->morale-group->numGroup>0) )
								{//try to move in on the enemy
									cpFlags |= ST_ApproachEnemy( aiEnt );
									//set me into scout mode
									AI_GroupUpdateSquadstates( group, aiEnt, SQUAD_SCOUT );
								}
								else
								{//use normal decision making process
									cpFlags |= ST_GetCPFlags(aiEnt);
								}
							}
							else
							{//someone in-between
								if ( (group->morale-group->numGroup<0) || !Q_irand( 0, 4 ) )
								{//do something
									cpFlags |= ST_GetCPFlags(aiEnt);
								}
								else
								{
									TIMER_Set( aiEnt, "stick", Q_irand( 2000, 4000 ) );
									TIMER_Set( aiEnt, "roamTime", Q_irand( 2000, 4000 ) );
								}
							}
						}
					}
					if ( !cpFlags )
					{//still not moving
						//see if we should say something?
						/*
						if ( NPC->attackDebounceTime < level.time - 2000 )
						{//we, personally, haven't shot for 2 seconds
							//maybe yell at the enemy?
							ST_Speech( NPC, SPEECH_CHARGE, 0.9f );
						}
						*/

						//see if we should do other fun stuff
						//toy with ducking
						if ( TIMER_Done( aiEnt, "duck" ) )
						{//not ducking
							if ( TIMER_Done( aiEnt, "stand" ) )
							{//don't have to keep standing
								if ( aiEnt->NPC->combatPoint == -1 || (level.combatPoints[aiEnt->NPC->combatPoint].flags&CPF_DUCK) )
								{//okay to duck here
									if ( !Q_irand( 0, 3 ) )
									{
										TIMER_Set( aiEnt, "duck", Q_irand( 1000, 3000 ) );
									}
								}
							}
						}
						//FIXME: what about CPF_LEAN?
					}
				}
			}
		}

		//clear the local state
		aiEnt->NPC->localState = LSTATE_NONE;

		if ( aiEnt->NPC->scriptFlags&SCF_USE_CP_NEAREST )
		{
			cpFlags &= ~(CP_FLANK|CP_APPROACH_ENEMY|CP_CLOSEST);
			cpFlags |= CP_NEAREST;
		}
		//Assign combat points
		if ( cpFlags )
		{//we want to run to a combat point
			/*
			if ( NPCInfo->combatPoint != -1 )
			{//if we're on a combat point, we obviously don't want the one we're closest to
				cpFlags |= CP_AVOID;
			}
			*/

			if ( group->enemy->client->ps.weapon == WP_SABER && /*group->enemy->client->ps.SaberLength() > 0*/!group->enemy->client->ps.saberHolstered )
			{//we obviously want to avoid the enemy if he has a saber
				cpFlags |= CP_AVOID_ENEMY;
				avoidDist = 256;
			}

			//remember what we *wanted* to do...
			cpFlags_org = cpFlags;

			//now get a combat point
			if ( cp == -1 )
			{//may have had sone set above
				cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, group->enemy->r.currentOrigin, cpFlags|CP_HAS_ROUTE, avoidDist, aiEnt->NPC->lastFailedCombatPoint );
			}
			while ( cp == -1 && cpFlags != CP_ANY )
			{//start "OR"ing out certain flags to see if we can find *any* point
				if ( cpFlags & CP_INVESTIGATE )
				{//don't need to investigate
					cpFlags &= ~CP_INVESTIGATE;
				}
				else if ( cpFlags & CP_SQUAD )
				{//don't need to stick to squads
					cpFlags &= ~CP_SQUAD;
				}
				else if ( cpFlags & CP_DUCK )
				{//don't need to duck
					cpFlags &= ~CP_DUCK;
				}
				else if ( cpFlags & CP_NEAREST )
				{//don't need closest one to me
					cpFlags &= ~CP_NEAREST;
				}
				else if ( cpFlags & CP_FLANK )
				{//don't need to flank enemy
					cpFlags &= ~CP_FLANK;
				}
				else if ( cpFlags & CP_SAFE )
				{//don't need one that hasn't been shot at recently
					cpFlags &= ~CP_SAFE;
				}
				else if ( cpFlags & CP_CLOSEST )
				{//don't need to get closest to enemy
					cpFlags &= ~CP_CLOSEST;
					//but let's try to approach at least
					cpFlags |= CP_APPROACH_ENEMY;
				}
				else if ( cpFlags & CP_APPROACH_ENEMY )
				{//don't need to approach enemy
					cpFlags &= ~CP_APPROACH_ENEMY;
				}
				else if ( cpFlags & CP_COVER )
				{//don't need cover
					cpFlags &= ~CP_COVER;
					//but let's pick one that makes us duck
					cpFlags |= CP_DUCK;
				}
				else if ( cpFlags & CP_CLEAR )
				{//don't need a clear shot to enemy
					cpFlags &= ~CP_CLEAR;
				}
				else if ( cpFlags & CP_AVOID_ENEMY )
				{//don't need to avoid enemy
					cpFlags &= ~CP_AVOID_ENEMY;
				}
				else if ( cpFlags & CP_RETREAT )
				{//don't need to retreat
					cpFlags &= ~CP_RETREAT;
				}
				else if ( cpFlags &CP_FLEE )
				{//don't need to flee
					cpFlags &= ~CP_FLEE;
					//but at least avoid enemy and pick one that gives cover
					cpFlags |= (CP_COVER|CP_AVOID_ENEMY);
				}
				else if ( cpFlags & CP_AVOID )
				{//okay, even pick one right by me
					cpFlags &= ~CP_AVOID;
				}
				else
				{
					cpFlags = CP_ANY;
				}
				//now try again
				cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, group->enemy->r.currentOrigin, cpFlags|CP_HAS_ROUTE, avoidDist, -1 );
			}
			//see if we got a valid one
			if ( cp != -1 )
			{//found a combat point
				//let others know that someone is now running
				runner = qtrue;
				//don't change course again until we get to where we're going
				TIMER_Set( aiEnt, "roamTime", Q3_INFINITE );
				TIMER_Set( aiEnt, "verifyCP", Q_irand( 1000, 3000 ) );//don't make sure you're in your CP for 1 - 3 seconds
				NPC_SetCombatPoint(aiEnt, cp );
				NPC_SetMoveGoal( aiEnt, level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
				//okay, try a move right now to see if we can even get there

				//if ( ST_Move() )
				{//we actually can get to it, so okay to say you're going there.
					//FIXME: Hmm... any way we can store this move info so we don't have to do it again
					//		when our turn to think comes up?

					//set us up so others know we're on the move
					if ( squadState != SQUAD_IDLE )
					{
						AI_GroupUpdateSquadstates( group, aiEnt, squadState );
					}
					else if ( cpFlags&CP_FLEE )
					{//outright running for your life
						AI_GroupUpdateSquadstates( group, aiEnt, SQUAD_RETREAT );
					}
					else
					{//any other kind of transition between combat points
						AI_GroupUpdateSquadstates( group, aiEnt, SQUAD_TRANSITION );
					}

					//unless we're trying to flee, walk slowly
					if ( !(cpFlags_org&CP_FLEE) )
					{
						//ucmd.buttons |= BUTTON_CAREFUL;
					}

					/*
					if ( scouting )
					{//successfully chasing enemy
						ST_Speech( NPC, SPEECH_CHASE, 0.0f );
						//don't say this again
						//group->speechDebounceTime = level.time + 5000;
					}
					//flanking:
					else */if ( cpFlags & CP_FLANK )
					{
						if ( group->numGroup > 1 )
						{
							NPC_ST_StoreMovementSpeech(aiEnt, SPEECH_OUTFLANK, -1 );
						}
					}
					else
					{//okay, let's cheat
						if ( group->numGroup > 1 )
						{
							float	dot = 1.0f;
							if ( !Q_irand( 0, 3 ) )
							{//25% of the time, see if we're flanking the enemy
								vec3_t	eDir2Me, eDir2CP;

								VectorSubtract( aiEnt->r.currentOrigin, group->enemy->r.currentOrigin, eDir2Me );
								VectorNormalize( eDir2Me );

								VectorSubtract( level.combatPoints[aiEnt->NPC->combatPoint].origin, group->enemy->r.currentOrigin, eDir2CP );
								VectorNormalize( eDir2CP );

								dot = DotProduct( eDir2Me, eDir2CP );
							}

							if ( dot < 0.4 )
							{//flanking!
								NPC_ST_StoreMovementSpeech( aiEnt, SPEECH_OUTFLANK, -1 );
							}
							else if ( !Q_irand( 0, 10 ) )
							{//regular movement
								NPC_ST_StoreMovementSpeech( aiEnt, SPEECH_YELL, 0.2f );//was SPEECH_COVER
							}
						}
					}
					/*
					else if ( cpFlags & CP_CLOSEST || cpFlags & CP_APPROACH_ENEMY )
					{
						if ( group->numGroup > 1 )
						{
							NPC_ST_StoreMovementSpeech( aiEnt, SPEECH_CHASE, 0.4f );
						}
					}
					*/
				}//else: nothing, a failed move should clear the combatPoint and you can try again next frame
			}
			else if ( aiEnt->NPC->squadState == SQUAD_SCOUT )
			{//we couldn't find a combatPoint by the player, so just go after him directly
				ST_HuntEnemy( aiEnt );
				//set me into scout mode
				AI_GroupUpdateSquadstates( group, aiEnt, SQUAD_SCOUT );
				//AI should take care of rest
			}
		}
	}

	return;
}

/*
-------------------------
NPC_BSST_Attack
-------------------------
*/

void NPC_BSST_Attack(gentity_t *aiEnt)
{
	vec3_t	enemyDir, shootDir;
	float dot;

	//Don't do anything if we're hurt
	if ( aiEnt->painDebounceTime > level.time )
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	// UQ1: Added evasion...
	NPC_CheckEvasion(aiEnt);

	//NPC_CheckEnemy( qtrue, qfalse );
	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )//!NPC->enemy )//
	{
		aiEnt->enemy = NULL;
		if( aiEnt->client->playerTeam == NPCTEAM_PLAYER )
		{
			NPC_BSPatrol(aiEnt);
		}
		else
		{
			NPC_BSST_Patrol(aiEnt);//FIXME: or patrol?
		}
		return;
	}

	//FIXME: put some sort of delay into the guys depending on how they saw you...?

	//Get our group info
	if ( TIMER_Done( aiEnt, "interrogating" ) )
	{
		AI_GetGroup( aiEnt );//, 45, 512, NPC->enemy );
	}
	else
	{
		//FIXME: when done interrogating, I should send out a team alert!
	}

	if ( aiEnt->NPC->group )
	{//I belong to a squad of guys - we should *always* have a group
		if ( !aiEnt->NPC->group->processed )
		{//I'm the first ent in my group, I'll make the command decisions
#if	AI_TIMERS
			int	startTime = GetTime(0);
#endif//	AI_TIMERS
			ST_Commander(aiEnt);
#if	AI_TIMERS
			int commTime = GetTime ( startTime );
			if ( commTime > 20 )
			{
				trap->Printf( S_COLOR_RED"ERROR: Commander time: %d\n", commTime );
			}
			else if ( commTime > 10 )
			{
				trap->Printf( S_COLOR_YELLOW"WARNING: Commander time: %d\n", commTime );
			}
			else if ( commTime > 2 )
			{
				trap->Printf( S_COLOR_GREEN"Commander time: %d\n", commTime );
			}
#endif//	AI_TIMERS
		}
	}
	else if ( TIMER_Done( aiEnt, "flee" ) && NPC_CheckForDanger(aiEnt, NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
	{//not already fleeing, and going to run
		ST_Speech( aiEnt, SPEECH_COVER, 0 );
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	if ( !aiEnt->enemy )
	{//WTF?  somehow we lost our enemy?
		NPC_BSST_Patrol(aiEnt);//FIXME: or patrol?
		return;
	}

	enemyLOS = enemyCS = enemyInFOV = qfalse;
	move = qtrue;
	faceEnemy = qfalse;
	shoot = qfalse;
	hitAlly = qfalse;
	VectorClear( impactPos );
	enemyDist = DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );

	VectorSubtract( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, enemyDir );
	VectorNormalize( enemyDir );
	AngleVectors( aiEnt->client->ps.viewangles, shootDir, NULL, NULL );
	dot = DotProduct( enemyDir, shootDir );
	if ( dot > 0.5f ||( enemyDist * (1.0f-dot)) < 10000 )
	{//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	/*if ( enemyDist < MIN_ROCKET_DIST_SQUARED )//128
	{//enemy within 128
		if ( (aiEnt->client->ps.weapon == WP_CLOSE_RANGE_WEAPONS) &&
			(aiEnt->NPC->scriptFlags & SCF_ALT_FIRE) )
		{//shooting an explosive, but enemy too close, switch to primary fire
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
			//FIXME: we can never go back to alt-fire this way since, after this, we don't know if we were initially supposed to use alt-fire or not...
		}
	}
	else*/ if ( enemyDist > 65536 )//256 squared
	{
		if (WeaponIsSniperCharge(aiEnt->client->ps.weapon))
		{//sniping... should be assumed
			if ( !(aiEnt->NPC->scriptFlags&SCF_ALT_FIRE) )
			{//use primary fire
				aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;
				//reset fire-timing variables
				NPC_ChangeWeapon(aiEnt, aiEnt->client->ps.weapon );
				NPC_UpdateAngles( aiEnt,qtrue, qtrue );
				return;
			}
		}
	}

	//can we see our target?
	if ( NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
	{
		AI_GroupUpdateEnemyLastSeen( aiEnt->NPC->group, aiEnt->enemy->r.currentOrigin );
		aiEnt->NPC->enemyLastSeenTime = level.time;
		enemyLOS = qtrue;

		if ( aiEnt->client->ps.weapon == WP_NONE )
		{
			enemyCS = qfalse;//not true, but should stop us from firing
			NPC_AimAdjust( aiEnt, -1 );//adjust aim worse longer we have no weapon
		}
		else
		{//can we shoot our target?
			if ( enemyInFOV )
			{//if enemy is FOV, go ahead and check for shooting
				int hit = NPC_ShotEntity(aiEnt, aiEnt->enemy, impactPos );
				gentity_t *hitEnt = &g_entities[hit];

				if ( hit == aiEnt->enemy->s.number
					|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == aiEnt->client->enemyTeam )
					|| ( hitEnt && hitEnt->takedamage && ((hitEnt->r.svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||aiEnt->s.weapon == WP_EMPLACED_GUN) ) )
				{//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
					AI_GroupUpdateClearShotTime( aiEnt->NPC->group );
					enemyCS = qtrue;
					NPC_AimAdjust( aiEnt, 2 );//adjust aim better longer we have clear shot at enemy
					VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
				}
				else
				{//Hmm, have to get around this bastard
					NPC_AimAdjust( aiEnt, 1 );//adjust aim better longer we can see enemy
					ST_ResolveBlockedShot(aiEnt, hit );
					if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == aiEnt->client->playerTeam )
					{//would hit an ally, don't fire!!!
						hitAlly = qtrue;
					}
					else
					{//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
					}
				}
			}
			else
			{
				enemyCS = qfalse;//not true, but should stop us from firing
			}
		}
	}
	else if ( trap->InPVS( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin ) )
	{
		aiEnt->NPC->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
		NPC_AimAdjust( aiEnt, -1 );//adjust aim worse longer we cannot see enemy
	}

	if ( aiEnt->client->ps.weapon == WP_NONE )
	{
		faceEnemy = qfalse;
		shoot = qfalse;
	}
	else
	{
		if ( enemyLOS )
		{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
			faceEnemy = qtrue;
		}
		if ( enemyCS )
		{
			shoot = qtrue;
		}
	}

	//Check for movement to take care of
	ST_CheckMoveState(aiEnt);

	//See if we should override shooting decision with any special considerations
	ST_CheckFireState(aiEnt);

	if ( faceEnemy )
	{//face the enemy
		NPC_FaceEnemy(aiEnt, qtrue );
	}

	if ( !(aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )
	{//not supposed to chase my enemies
		if ( aiEnt->NPC->goalEntity == aiEnt->enemy )
		{//goal is my entity, so don't move
			move = qfalse;
		}
	}

	/*if ( aiEnt->client->ps.weaponTime > 0 && aiEnt->s.weapon == WP_ROCKET_LAUNCHER )
	{
		move = qfalse;
	}*/

	if ( move )
	{//move toward goal
		if ( aiEnt->NPC->goalEntity )//&& ( NPCInfo->goalEntity != NPC->enemy || enemyDist > 10000 ) )//100 squared
		{
			move = ST_Move(aiEnt);
		}
		else
		{
			move = qfalse;
		}
	}

	if ( !move )
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

	if ( !TIMER_Done( aiEnt, "flee" ) )
	{//running away
		faceEnemy = qfalse;
	}

	//FIXME: check scf_face_move_dir here?

	if ( !faceEnemy )
	{//we want to face in the dir we're running
		if ( !move )
		{//if we haven't moved, we should look in the direction we last looked?
			VectorCopy( aiEnt->client->ps.viewangles, aiEnt->NPC->lastPathAngles );
		}
		aiEnt->NPC->desiredYaw = aiEnt->NPC->lastPathAngles[YAW];
		aiEnt->NPC->desiredPitch = 0;
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		if ( move )
		{//don't run away and shoot
			shoot = qfalse;
		}
	}

	if ( aiEnt->NPC->scriptFlags & SCF_DONT_FIRE )
	{
		shoot = qfalse;
	}

	if ( aiEnt->enemy && aiEnt->enemy->enemy )
	{
		if ( aiEnt->enemy->s.weapon == WP_SABER && aiEnt->enemy->enemy->s.weapon == WP_SABER )
		{//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot = qfalse;
		}
	}
	//FIXME: don't shoot right away!
	if ( aiEnt->client->ps.weaponTime > 0 )
	{
		/*if ( aiEnt->s.weapon == WP_ROCKET_LAUNCHER )
		{
			if ( !enemyLOS || !enemyCS )
			{//cancel it
				aiEnt->client->ps.weaponTime = 0;
			}
			else
			{//delay our next attempt
				TIMER_Set( aiEnt, "attackDelay", Q_irand( 3000, 5000 ) );
			}
		}*/
	}
	else if ( shoot )
	{//try to shoot if it's time
		if ( TIMER_Done( aiEnt, "attackDelay" ) )
		{
			if( !(aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
			{
				WeaponThink(aiEnt, qtrue );
			}
			/*if ( aiEnt->s.weapon == WP_ROCKET_LAUNCHER
				&& (aiEnt->client->pers.cmd.buttons&BUTTON_ATTACK)
				&& !move
				&& g_npcspskill.integer > 1
				&& !Q_irand( 0, 3 ) )
			{//every now and then, shoot a homing rocket
				aiEnt->client->pers.cmd.buttons &= ~BUTTON_ATTACK;
				aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
				aiEnt->client->ps.weaponTime = Q_irand( 1000, 2500 );
			}*/
		}
	}
}

void NPC_BSST_Default(gentity_t *aiEnt)
{
	if( aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink(aiEnt, qtrue );
	}

	if( !aiEnt->enemy )
	{//don't have an enemy, look for one
		NPC_BSST_Patrol(aiEnt);
	}
	else //if ( NPC->enemy )
	{//have an enemy
		if (NPC_CanUseAdvancedFighting(aiEnt))
		{
			NPC_BSJedi_Default(aiEnt);
		}
		else
		{
			NPC_CheckGetNewWeapon(aiEnt);
			NPC_BSST_Attack(aiEnt);
		}
	}
}
