#include "b_local.h"

extern void G_Knockdown( gentity_t *self );
extern void G_SoundOnEnt(gentity_t *ent, soundChannel_t channel, const char *soundPath);
extern float Q_flrand(float min, float max);
extern void G_GetBoltPosition(gentity_t *self, int boltIndex, vec3_t pos, int modelIndix);
extern void Rancor_DropVictim( gentity_t *self );//wahoo - :p

#define MIN_ATTACK_DIST_SQ	128
#define MIN_MISS_DIST		100
#define MIN_MISS_DIST_SQ	(MIN_MISS_DIST*MIN_MISS_DIST)
#define MAX_MISS_DIST		500
#define MAX_MISS_DIST_SQ	(MAX_MISS_DIST*MAX_MISS_DIST)
#define MIN_SCORE			-37500 //speed of (50*50) - dist of (200*200)

void SandCreature_Precache( void )
{
	int i;
	G_EffectIndex( "env/sand_dive" );
	G_EffectIndex( "env/sand_spray" );
	G_EffectIndex( "env/sand_move" );
	G_EffectIndex( "env/sand_move_breach" );
	//G_EffectIndex( "env/sand_attack_breach" );
	for ( i = 1; i < 4; i++ )
	{
		G_SoundIndex( va( "sound/chars/sand_creature/voice%d.mp3", i ) );
	}
	G_SoundIndex( "sound/chars/sand_creature/slither.wav" );
}

void SandCreature_ClearTimers( gentity_t *aiEnt)
{
	TIMER_Set( aiEnt, "speaking", -level.time );
	TIMER_Set( aiEnt, "breaching", -level.time );
	TIMER_Set( aiEnt, "breachDebounce", -level.time );
	TIMER_Set( aiEnt, "pain", -level.time );
	TIMER_Set( aiEnt, "attacking", -level.time );
	TIMER_Set( aiEnt, "missDebounce", -level.time );
}

/* SP NUAM
void NPC_SandCreature_Die( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	//FIXME: somehow make him solid when he dies?
}
*/

void NPC_SandCreature_Pain( gentity_t *self, gentity_t *attacker, int damage )
//void NPC_SandCreature_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	int counter;
	if ( TIMER_Done( self, "pain" ) )
	{
		//float playerDist;
		vec3_t shakePos;
		//FIXME: effect and sound
		//FIXME: shootable during this anim?
		NPC_SetAnim( self, SETANIM_LEGS, Q_irand(BOTH_ATTACK1,BOTH_ATTACK2), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
		G_AddEvent( self, EV_PAIN, Q_irand( 0, 100 ) );
		TIMER_Set( self, "pain", self->client->ps.legsTimer + Q_irand( 500, 2000 ) );
		//wahoo fix for el camera shake effect
		G_GetBoltPosition( self, self->client->renderInfo.headBolt, shakePos, 0 );
		for(counter = 0; counter < MAX_CLIENTS; counter++)
		{
			float playerDist;
			gentity_t *radiusEnt = &g_entities[counter];
			if(radiusEnt && radiusEnt->client)
			{
				playerDist = Distance( radiusEnt->r.currentOrigin, self->r.currentOrigin );
				if ( playerDist < 256 )
					G_ScreenShake( shakePos, radiusEnt, 2.0f, 1000, qfalse );
			}
		}
	}
	self->enemy = self->NPC->goalEntity = NULL;
}

void SandCreature_MoveEffect(gentity_t *aiEnt)
{
	vec3_t	up = {0,0,1};
	vec3_t shakePos;
	vec3_t	org;
	int i;
	org[0] = aiEnt->r.currentOrigin[0];
	org[1] = aiEnt->r.currentOrigin[1];
	org[2] = aiEnt->r.absmin[2]+2;

	//float playerDist = Distance( /*wahoo change*/aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin );
	//if ( playerDist < 256 )
	//{
		//CGCam_Shake( 0.75f*playerDist/256.0f, 250 );
		G_GetBoltPosition( aiEnt, aiEnt->client->renderInfo.headBolt, shakePos, 0 );
	//}
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		float playerDist;
		gentity_t *radiusEnt = &g_entities[i];
		if(radiusEnt && radiusEnt->client)
		{
			playerDist = Distance( radiusEnt->r.currentOrigin, aiEnt->r.currentOrigin );
			if ( playerDist < 256 )
				G_ScreenShake( shakePos, radiusEnt, 2.0f, 250, qfalse );
		}
	}

	
	if ( level.time-aiEnt->client->ps.legsTimer > 2000 )
	{//first time moving for at least 2 seconds
		//clear speakingtime
		TIMER_Set( aiEnt, "speaking", -level.time );
	}

	if ( TIMER_Done( aiEnt, "breaching" ) 
		&& TIMER_Done( aiEnt, "breachDebounce" )
		&& TIMER_Done( aiEnt, "pain" )
		&& TIMER_Done( aiEnt, "attacking" )
		&& !Q_irand( 0, 10 ) )
	{//Breach!
		//FIXME: only do this while moving forward?
		trace_t	trace;
		//make him solid here so he can be hit/gets blocked on stuff. Check clear first.
		trap->Trace( &trace, aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, aiEnt->r.currentOrigin, aiEnt->s.number, MASK_NPCSOLID, qfalse, 0, 0 );
		if ( !trace.allsolid && !trace.startsolid )
		{
			aiEnt->clipmask = MASK_NPCSOLID;//turn solid for a little bit
			aiEnt->r.contents = CONTENTS_BODY;
			//aiEnt->takedamage = qtrue;//can be shot?

			//FIXME: Breach sound?
			//FIXME: Breach effect?
			NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_WALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
			TIMER_Set( aiEnt, "breaching", aiEnt->client->ps.legsTimer );
			TIMER_Set( aiEnt, "breachDebounce", aiEnt->client->ps.legsTimer+Q_irand( 0, 10000 ) );
		}
	}
	if ( !TIMER_Done( aiEnt, "breaching" ) )
	{//different effect when breaching
		//FIXME: make effect
		/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_move_breach" ), org, up );
	}
	else
	{
		/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_move" ), org, up );
	}
	aiEnt->s.loopSound = G_SoundIndex( "sound/chars/sand_creature/slither.wav" );
}

qboolean SandCreature_CheckAhead(gentity_t *aiEnt, vec3_t end )
{
	float	radius;
	float	dist;
	float	tFrac; 
	trace_t	trace;
	int clipmask = aiEnt->clipmask|CONTENTS_BOTCLIP;

	//make sure our goal isn't underground (else the trace will fail)
	vec3_t	bottom;
	bottom[0] = end[0];
	bottom[1] = end[1];
	bottom[2] = end[2]+aiEnt->r.mins[2];
	trap->Trace( &trace, end, vec3_origin, vec3_origin, bottom, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );
	if ( trace.fraction < 1.0f )
	{//in the ground, raise it up
		end[2] -= aiEnt->r.mins[2]*(1.0f-trace.fraction)-0.125f;
	}

	trap->Trace( &trace, aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, end, aiEnt->s.number, clipmask, qfalse, 0, 0 );

	if ( trace.startsolid&&(trace.contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		clipmask &= ~CONTENTS_BOTCLIP;
		trap->Trace( &trace, aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, end, aiEnt->s.number, clipmask, qfalse, 0, 0 );
	}
	//Do a simple check
	if ( ( trace.allsolid == qfalse ) && ( trace.startsolid == qfalse ) && ( trace.fraction == 1.0f ) )
		return qtrue;

	if ( trace.plane.normal[2] >= .7f )
	{
		return qtrue;
	}

	//This is a work around
	radius = ( aiEnt->r.maxs[0] > aiEnt->r.maxs[1] ) ? aiEnt->r.maxs[0] : aiEnt->r.maxs[1];
	dist = Distance( aiEnt->r.currentOrigin, end );
	tFrac = 1.0f - ( radius / dist );

	if ( trace.fraction >= tFrac )
		return qtrue;

	return qfalse;
}

qboolean SandCreature_Move(gentity_t *aiEnt)
{
	qboolean moved = qfalse;
	//FIXME should ignore doors..?
	vec3_t dest;
	VectorCopy( aiEnt->NPC->goalEntity->r.currentOrigin, dest );
	//Sand Creatures look silly using waypoints when they can go straight to the goal
	if ( SandCreature_CheckAhead(aiEnt, dest ) )
	{//use our temp move straight to goal check
		VectorSubtract( dest, aiEnt->r.currentOrigin, aiEnt->client->ps.moveDir );
		aiEnt->client->ps.speed = VectorNormalize( aiEnt->client->ps.moveDir );
		if ( (aiEnt->client->pers.cmd.buttons&BUTTON_WALKING) && aiEnt->client->ps.speed > aiEnt->NPC->stats.walkSpeed )
		{
			aiEnt->client->ps.speed = aiEnt->NPC->stats.walkSpeed;
		}
		else
		{
			if ( aiEnt->client->ps.speed < aiEnt->NPC->stats.walkSpeed )
			{
				aiEnt->client->ps.speed = aiEnt->NPC->stats.walkSpeed;
			}
			if ( !(aiEnt->client->pers.cmd.buttons&BUTTON_WALKING) && aiEnt->client->ps.speed < aiEnt->NPC->stats.runSpeed )
			{
				aiEnt->client->ps.speed = aiEnt->NPC->stats.runSpeed;
			}
			else if ( aiEnt->client->ps.speed > aiEnt->NPC->stats.runSpeed )
			{
				aiEnt->client->ps.speed = aiEnt->NPC->stats.runSpeed;
			}
		}
		moved = qtrue;
	}
	else
	{
		moved = NPC_MoveToGoal(aiEnt, qtrue );
	}
	if ( moved && aiEnt->radius )
	{
		vec3_t	newPos;
		float curTurfRange, newTurfRange;
		curTurfRange = DistanceHorizontal( aiEnt->r.currentOrigin, aiEnt->s.origin );
		VectorMA( aiEnt->r.currentOrigin, aiEnt->client->ps.speed/100.0f, aiEnt->client->ps.moveDir, newPos );
		newTurfRange = DistanceHorizontal( newPos, aiEnt->s.origin );
		if ( newTurfRange > aiEnt->radius && newTurfRange > curTurfRange )
		{//would leave our range
			//stop
			aiEnt->client->ps.speed = 0.0f;
			VectorClear( aiEnt->client->ps.moveDir );
			aiEnt->client->pers.cmd.forwardmove = aiEnt->client->pers.cmd.rightmove = 0;
			moved = qfalse;
		}
	}
	return (moved);
	//often erroneously returns false ???  something wrong with NAV...?
}

void SandCreature_Attack(gentity_t *aiEnt, qboolean miss )
{
	//float playerDist;
	vec3_t shakePos;
	int i;
	//FIXME: make it able to grab a thermal detonator, take it down, 
	//		then have it explode inside them, killing them 
	//		(or, do damage, making them stick half out of the ground and
	//		screech for a bit, giving you a chance to run for it!)

	//FIXME: effect and sound
	//FIXME: shootable during this anim?
	if ( !aiEnt->enemy->client )
	{
		NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	}
	else
	{
		NPC_SetAnim( aiEnt, SETANIM_LEGS, Q_irand( BOTH_ATTACK1, BOTH_ATTACK2 ), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	}
	//don't do anything else while in this anim
	TIMER_Set( aiEnt, "attacking", aiEnt->client->ps.legsTimer );
	//playerDist = Distance( /*wahoo change player->r.currentOrigin*/aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin );
	//if ( playerDist < 256 )
	//{
		//FIXME: tone this down
		//CGCam_Shake( 0.75f*playerDist/128.0f, aiEnt->client->ps.legsTimer );
		//CGCam_Shake( 1.0f*playerDist/128.0f, self->client->ps.legsTimer );
		//CGCam_Shake( 0.75f*playerDist/256.0f, 250 );
		G_GetBoltPosition( aiEnt, aiEnt->client->renderInfo.headBolt, shakePos, 0 );
		//G_ScreenShake( shakePos, NULL, 1.0f, aiEnt->client->ps.legsTimer, qfalse );

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		float playerDist;
		gentity_t *radiusEnt = &g_entities[i];
		if(radiusEnt && radiusEnt->client)
		{
			playerDist = Distance( radiusEnt->r.currentOrigin, aiEnt->r.currentOrigin );
			if ( playerDist < 256 )
				G_ScreenShake( shakePos, radiusEnt, 1.0f, aiEnt->client->ps.legsTimer, qfalse );
		}
	}


	//} 

	if ( miss )
	{//purposely missed him, chance of knocking him down
		//FIXME: if, during the attack anim, I do end up catching him close to my mouth, then snatch him anyway...
		if ( aiEnt->enemy && aiEnt->enemy->client )
		{
			vec3_t dir2Enemy;
			VectorSubtract( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, dir2Enemy );
			if ( dir2Enemy[2] < 30 )
			{
				dir2Enemy[2] = 30;
			}
			//if ( g_spskill.integer > 0 )
			{
				float enemyDist = VectorNormalize( dir2Enemy );
				//FIXME: tone this down, smaller radius
				if ( enemyDist < 200 && aiEnt->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					float throwStr = ((200-enemyDist)*0.4f)+20;
					if ( throwStr > 45 )
					{
						throwStr = 45;
					}
					G_Throw( aiEnt->enemy, dir2Enemy, throwStr );
					//if ( g_spskill.integer > 1 )
					{//knock them down, too
						if ( aiEnt->enemy->health > 0 
							&& Q_flrand( 50, 150 ) > enemyDist )
						{//knock them down
							G_Knockdown( aiEnt->enemy );
							if ( aiEnt->enemy->s.number < MAX_CLIENTS )
							{//make the player look up at me
								vec3_t vAng;
								vectoangles( dir2Enemy, vAng );
								VectorSet( vAng, AngleNormalize180(vAng[PITCH])*-1, aiEnt->enemy->client->ps.viewangles[YAW], 0 );
								SetClientViewAngle( aiEnt->enemy, vAng );
							}
						}
					}
				}
			}
		}
	}
	else
	{
		aiEnt->enemy->activator = aiEnt; // kind of dumb, but when we are locked to the Rancor, we are owned by it.
		aiEnt->activator = aiEnt->enemy;//remember him
		//this guy isn't going anywhere anymore
		aiEnt->enemy->r.contents = 0;
		aiEnt->enemy->clipmask = 0;

		if ( aiEnt->activator->client )
		{
			//aiEnt->activator->client->ps.SaberDeactivateTrail(0);
			aiEnt->client->ps.eFlags2 |= EF2_GENERIC_NPC_FLAG;//wahoo fix - to get it to stick in the mouth
			aiEnt->activator->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;//EF_HELD_BY_SAND_CREATURE;//wahoo fix
			if ( aiEnt->activator->health > 0 && aiEnt->activator->client )
			{
				G_AddEvent( aiEnt->activator, Q_irand(EV_DEATH1, EV_DEATH3), 0 );
				NPC_SetAnim( aiEnt->activator, SETANIM_LEGS, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				NPC_SetAnim( aiEnt->activator, SETANIM_TORSO, BOTH_FALLDEATH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				TossClientItems( aiEnt );
				if ( aiEnt->activator->NPC )
				{//no more thinking for you
					aiEnt->activator->NPC->nextBStateThink = Q3_INFINITE;
				}
			}
			/*
			if ( !aiEnt->activator->s.number )
			{
				cg.overrides.active |= (CG_OVERRIDE_3RD_PERSON_CDP|CG_OVERRIDE_3RD_PERSON_RNG);
				cg.overrides.thirdPersonCameraDamp = 0;
				cg.overrides.thirdPersonRange = 120;
			}
			*/
		}
		else
		{
			aiEnt->client->ps.eFlags2 |= EF2_GENERIC_NPC_FLAG;//wahoo fix - to get it to stick in the mouth
			aiEnt->activator->s.eFlags2 |= EF2_HELD_BY_MONSTER;//EF_HELD_BY_SAND_CREATURE;//wahoo fix
		}
	}
}

float SandCreature_EntScore(gentity_t *aiEnt, gentity_t *ent )
{
	float moveSpeed, dist;

	if ( ent->client )
	{
		moveSpeed = VectorLengthSquared( ent->client->ps.velocity );
	}
	else
	{
		moveSpeed = VectorLengthSquared( ent->s.pos.trDelta );
	}
	dist = DistanceSquared( aiEnt->r.currentOrigin, ent->r.currentOrigin );
	return (moveSpeed-dist);
}

void SandCreature_SeekEnt( gentity_t *aiEnt, gentity_t *bestEnt, float score )
{
	aiEnt->NPC->enemyLastSeenTime = level.time;
	VectorCopy( bestEnt->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
	NPC_SetMoveGoal( aiEnt, aiEnt->NPC->enemyLastSeenLocation, 0, qfalse, -1, NULL );
	if ( score > MIN_SCORE )
	{
		aiEnt->enemy = bestEnt;
	}
}

void SandCreature_CheckMovingEnts(gentity_t *aiEnt)
{
	gentity_t	*radiusEnts[ 128 ];
	int			numEnts;
	const float	radius = aiEnt->NPC->stats.earshot;
	int			i;
	vec3_t		mins, maxs;
	int bestEnt = -1;
	float bestScore = 0;
	float checkScore;
	int			iradiusEnts[ 128 ];

	for ( i = 0; i < 3; i++ )
	{
		mins[i] = aiEnt->r.currentOrigin[i] - radius;
		maxs[i] = aiEnt->r.currentOrigin[i] + radius;
	}

	numEnts = trap->EntitiesInBox( mins, maxs, /*radiusEnts*/iradiusEnts, 128 );

	for ( i = 0; i < numEnts; i++ )
	{
		radiusEnts[ i ] = &g_entities[iradiusEnts[i]];
			/*continue;//wahoo fix*/

		if ( !radiusEnts[i]->inuse )
		{
			continue;
		}
		
		if ( radiusEnts[i] == aiEnt )
		{//Skip the itself
			continue;
		}
		
		if ( radiusEnts[i]->client == NULL )
		{//must be a client
			if ( radiusEnts[i]->s.eType != ET_MISSILE
				|| radiusEnts[i]->s.weapon != WP_THERMAL )
			{//not a thermal detonator
				continue;
			}
		}
		else
		{
			if ( (radiusEnts[i]->client->ps.eFlags&EF2_HELD_BY_MONSTER) )
			{//can't be one being held
				continue;
			}

			/*if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			{//can't be one being held
				continue;
			}

			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_SAND_CREATURE) )
			{//can't be one being held
				continue;
			}*/

			if ( (radiusEnts[i]->s.eFlags&EF_NODRAW) )
			{//not if invisible
				continue;
			}

			if ( radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_WORLD )
			{//not on the ground
				continue;
			}

			if ( radiusEnts[i]->client->NPC_class == CLASS_SAND_CREATURE )
			{
				continue;
			}
		}

		if ( (radiusEnts[i]->flags&FL_NOTARGET) )
		{
			continue;
		}
		/*
		if ( radiusEnts[i]->client && (radiusEnts[i]->client->NPC_class == CLASS_RANCOR || radiusEnts[i]->client->NPC_class == CLASS_ATST_OLD ) )
		{//can't grab rancors or atst's
			continue;
		}
		*/
		checkScore = SandCreature_EntScore(aiEnt, radiusEnts[i] );
		//FIXME: take mass into account too?  What else?
		if ( checkScore > bestScore )
		{
			bestScore = checkScore;
			bestEnt = i;
		}
	}
	if ( bestEnt != -1 )
	{
		SandCreature_SeekEnt(aiEnt, radiusEnts[bestEnt], bestScore );
	}
}

void SandCreature_SeekAlert(gentity_t *aiEnt, int alertEvent )
{
	alertEvent_t *alert = &level.alertEvents[alertEvent];

	//FIXME: check for higher alert status or closer than last location?
	aiEnt->NPC->enemyLastSeenTime = level.time;
	VectorCopy( alert->position, aiEnt->NPC->enemyLastSeenLocation );
	NPC_SetMoveGoal( aiEnt, aiEnt->NPC->enemyLastSeenLocation, 0, qfalse, -1, NULL );
}

void SandCreature_CheckAlerts(gentity_t *aiEnt)
{
	if ( !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		int alertEvent = NPC_CheckAlertEvents(aiEnt, qfalse, qtrue, aiEnt->NPC->lastAlertID, qfalse, AEL_MINOR );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			//if ( level.alertEvents[alertEvent].ID != aiEnt->NPC->lastAlertID )
			{
				SandCreature_SeekAlert(aiEnt,  alertEvent );
			}
		}
	}
}

float SandCreature_DistSqToGoal(gentity_t *aiEnt, qboolean goalIsEnemy )
{
	float goalDistSq;
	if ( !aiEnt->NPC->goalEntity || goalIsEnemy )
	{
		if ( !aiEnt->enemy )
		{
			return Q3_INFINITE;
		}
		aiEnt->NPC->goalEntity = aiEnt->enemy;
	}

	if ( aiEnt->NPC->goalEntity->client )
	{
		goalDistSq = DistanceSquared( aiEnt->r.currentOrigin, aiEnt->NPC->goalEntity->r.currentOrigin );
	}
	else
	{
		vec3_t gOrg;
		VectorCopy( aiEnt->NPC->goalEntity->r.currentOrigin, gOrg );
		gOrg[2] -= (aiEnt->r.mins[2]-aiEnt->NPC->goalEntity->r.mins[2]);//moves the gOrg up/down to make it's origin seem at the proper height as if it had my mins
		goalDistSq = DistanceSquared( aiEnt->r.currentOrigin, gOrg );
	}
	return goalDistSq;
}

void SandCreature_Chase(gentity_t *aiEnt)
{
	float enemyDistSq = SandCreature_DistSqToGoal(aiEnt, qtrue );

	if ( !aiEnt->enemy->inuse )
	{//freed
		aiEnt->enemy = NULL;
		return;
	}
	
	if ( /*wahoo change  r.svFlags&SVF_LOCKEDENEMY*/(aiEnt->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//always know where he is
		aiEnt->NPC->enemyLastSeenTime = level.time;
	}

	if ( /*wahoo change  r.svFlags&SVF_LOCKEDENEMY*/!(aiEnt->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{
		if ( level.time-aiEnt->NPC->enemyLastSeenTime > 10000 )
		{
			aiEnt->enemy = NULL;
			return;
		}
	}

	if ( aiEnt->enemy->client )
	{
		if ( aiEnt->enemy->client->ps.eFlags&EF2_HELD_BY_MONSTER)
			//|| (aiEnt->enemy->client->ps.eFlags&EF_HELD_BY_RANCOR)
			//|| (aiEnt->enemy->client->ps.eFlags&EF_HELD_BY_WAMPA) )
		{//was picked up by another monster, forget about him
			aiEnt->enemy = NULL;
			//wahoo changeaiEnt->r.svFlags &= ~SVF_LOCKEDENEMY;
			aiEnt->NPC->aiFlags &= ~NPCAI_LOCKEDENEMY;
			return;
		}
	}
	//chase the enemy
	if ( aiEnt->enemy->client 
		&& aiEnt->enemy->client->ps.groundEntityNum != ENTITYNUM_WORLD 
		&& !(/*aiEnt->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/aiEnt->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//off the ground!
		//FIXME: keep moving in the dir we were moving for a little bit...
	}
	else
	{
		float enemyScore = SandCreature_EntScore(aiEnt, aiEnt->enemy );
		if ( enemyScore < MIN_SCORE 
			&& !(/*aiEnt->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/aiEnt->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
		{//too slow or too far away
		}
		else
		{
			float moveSpeed;
			if ( aiEnt->enemy->client )
			{
				moveSpeed = VectorLengthSquared( aiEnt->enemy->client->ps.velocity );
			}
			else
			{
				moveSpeed = VectorLengthSquared( aiEnt->enemy->s.pos.trDelta );
			}
			if ( moveSpeed )
			{//he's still moving, update my goalEntity's origin
				SandCreature_SeekEnt(aiEnt, aiEnt->enemy, 0 );
				aiEnt->NPC->enemyLastSeenTime = level.time;
			}
		}
	}

	if ( (level.time-aiEnt->NPC->enemyLastSeenTime) > 5000 
		&& !(/*aiEnt->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/aiEnt->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//enemy hasn't moved in about 5 seconds, see if there's anything else of interest
		SandCreature_CheckAlerts(aiEnt);
		SandCreature_CheckMovingEnts(aiEnt);
	}



	//FIXME: keeps chasing goalEntity even when it's already reached it...?
	if ( enemyDistSq >= MIN_ATTACK_DIST_SQ//aiEnt->NPC->goalEntity &&
		&& (level.time-aiEnt->NPC->enemyLastSeenTime) <= 3000 )
	{//sensed enemy (or something) less than 3 seconds ago
		aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
		if ( SandCreature_Move(aiEnt) )
		{
			SandCreature_MoveEffect(aiEnt);
		}
	}
	else if ( (level.time-aiEnt->NPC->enemyLastSeenTime) <= 5000 
		&& !(/*aiEnt->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/aiEnt->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//NOTE: this leaves a 2-second dead zone in which they'll just sit there unless their enemy moves
		//If there is an event we might be interested in if we weren't still interested in our enemy
		if ( NPC_CheckAlertEvents(aiEnt, qfalse, qtrue, aiEnt->NPC->lastAlertID, qfalse, AEL_MINOR ) >= 0 )
		{//just stir
			SandCreature_MoveEffect(aiEnt);
		}
	}

	if ( enemyDistSq < MIN_ATTACK_DIST_SQ )
	{
		if ( aiEnt->enemy->client )
		{
			aiEnt->client->ps.viewangles[YAW] = aiEnt->enemy->client->ps.viewangles[YAW];
		}
		if ( TIMER_Done( aiEnt, "breaching" ) )
		{
			//okay to attack
			SandCreature_Attack(aiEnt, qfalse );
		}
	}
	else if ( enemyDistSq < MAX_MISS_DIST_SQ 
		&& enemyDistSq > MIN_MISS_DIST_SQ
		&& aiEnt->enemy->client 
		&& TIMER_Done( aiEnt, "breaching" )
		&& TIMER_Done( aiEnt, "missDebounce" )
		&& !VectorCompare( aiEnt->pos1, aiEnt->r.currentOrigin ) //so we don't come up again in the same spot
		&& !Q_irand( 0, 10 ) )
	{
		if ( !(/*aiEnt->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/aiEnt->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
		{
			//miss them
			SandCreature_Attack(aiEnt, qtrue );
			VectorCopy( aiEnt->r.currentOrigin, aiEnt->pos1 );
			TIMER_Set( aiEnt, "missDebounce", Q_irand( 3000, 10000 ) );
		}
	}
}

void SandCreature_Hunt(gentity_t *aiEnt)
{
	SandCreature_CheckAlerts(aiEnt);
	SandCreature_CheckMovingEnts(aiEnt);
	//If we have somewhere to go, then do that
	//FIXME: keeps chasing goalEntity even when it's already reached it...?
	if ( aiEnt->NPC->goalEntity 
		&& SandCreature_DistSqToGoal(aiEnt, qfalse ) >= MIN_ATTACK_DIST_SQ )
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		if ( SandCreature_Move(aiEnt) )
		{
			SandCreature_MoveEffect(aiEnt);
		}
	}
	else
	{
		NPC_ReachedGoal(aiEnt);
	}
}

void SandCreature_Sleep(gentity_t *aiEnt)
{
	SandCreature_CheckAlerts(aiEnt);
	SandCreature_CheckMovingEnts(aiEnt);
	//FIXME: keeps chasing goalEntity even when it's already reached it!
	if ( aiEnt->NPC->goalEntity 
		&& SandCreature_DistSqToGoal(aiEnt, qfalse ) >= MIN_ATTACK_DIST_SQ )
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		if ( SandCreature_Move(aiEnt) )
		{
			SandCreature_MoveEffect(aiEnt);
		}
	}
	else
	{
		NPC_ReachedGoal(aiEnt);
	}
	/*
	if ( UpdateGoal(aiEnt) )
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		//FIXME: Sand Creatures look silly using waypoints when they can go straight to the goal
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	*/
}

void	SandCreature_PushEnts(gentity_t *aiEnt)
{
  	int			numEnts;
	gentity_t*	radiusEnts[128];
	const float	radius = 70;
	vec3_t		mins, maxs;
	vec3_t		smackDir;
	float		smackDist;
	int			i, entIndex;
	int			iradiusEnts[ 128 ];

	for (i = 0; i < 3; i++ )
	{
		mins[i] = aiEnt->r.currentOrigin[i] - radius;
		maxs[i] = aiEnt->r.currentOrigin[i] + radius;
	}

	numEnts = trap->EntitiesInBox(mins, maxs, /*radiusEnts*/iradiusEnts, 128);
	for (entIndex=0; entIndex<numEnts; entIndex++)
	{
		radiusEnts[ entIndex ] = &g_entities[iradiusEnts[entIndex]];
			/*continue;//wahoo fix*/

		// Only Clients
		//--------------
		if (!radiusEnts[entIndex] || !radiusEnts[entIndex]->client || radiusEnts[entIndex]==aiEnt)
		{
			continue;
		}

		// Do The Vector Distance Test
		//-----------------------------
		VectorSubtract(radiusEnts[entIndex]->r.currentOrigin, aiEnt->r.currentOrigin, smackDir);
		smackDist = VectorNormalize(smackDir);
		if (smackDist<radius)
		{
 			G_Throw(radiusEnts[entIndex], smackDir, 90);
		}
	}
}

void NPC_BSSandCreature_Default( gentity_t *aiEnt)
{
	qboolean visible = qfalse;
	
	//clear it every frame, will be set if we actually move this frame...
	aiEnt->s.loopSound = 0;

	if ( aiEnt->health > 0 && TIMER_Done( aiEnt, "breaching" ) )
	{//go back to non-solid mode
		if ( aiEnt->r.contents )
		{
			aiEnt->r.contents = 0;
		}
		if ( aiEnt->clipmask == MASK_NPCSOLID )
		{
			aiEnt->clipmask = CONTENTS_SOLID|CONTENTS_MONSTERCLIP;
		}
		if ( TIMER_Done( aiEnt, "speaking" ) )
		{
			G_SoundOnEnt( aiEnt, CHAN_VOICE, va( "sound/chars/sand_creature/voice%d.mp3", Q_irand( 1, 3 ) ) );
			TIMER_Set( aiEnt, "speaking", Q_irand( 3000, 10000 ) );
		}
	}
	else
	{//still in breaching anim
		visible = qtrue;
		//FIXME: maybe push things up/away and maybe knock people down when doing this?
		//FIXME: don't turn while breaching?
		//FIXME: move faster while breaching?
		//NOTE: shaking now done whenever he moves
	}

	//FIXME: when in start and end of attack/pain anims, need ground disturbance effect around him
	// NOTENOTE: someone stubbed this code in, so I figured I'd use it.  The timers are all weird, ie, magic numbers that sort of work, 
	//	but maybe I'll try and figure out real values later if I have time.
	if ( aiEnt->client->ps.legsAnim == BOTH_ATTACK1
		|| aiEnt->client->ps.legsAnim == BOTH_ATTACK2 ) 
	{//FIXME: get start and end frame numbers for this effect for each of these anims
		vec3_t	up={0,0,1};
		vec3_t org;
		VectorCopy( aiEnt->r.currentOrigin, org );
		org[2] -= 40; 
		if ( aiEnt->client->ps.legsTimer > 3700 )
		{
//			/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_dive"  ), aiEnt->r.currentOrigin, up );
			/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_spray"  ), org, up );
		}
		else if ( aiEnt->client->ps.legsTimer > 1600 	&& aiEnt->client->ps.legsTimer < 1900 )
		{
			/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_spray"  ), org, up );
		}
		///*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_attack_breach" ), org, up );
	}
	

	if ( !TIMER_Done( aiEnt, "pain" ) )
	{
		visible = qtrue;
	}
	else if ( !TIMER_Done( aiEnt, "attacking" ) )
	{
		visible = qtrue;
	}
	else
	{
		if ( aiEnt->activator )
		{//kill and remove the guy we ate
			//FIXME: want to play ...?  What was I going to say?
			G_Damage(aiEnt->activator, aiEnt, aiEnt, NULL, aiEnt->activator->s.origin, aiEnt->activator->health*2, DAMAGE_NO_PROTECTION|DAMAGE_NO_KNOCKBACK, MOD_MELEE);
			if ( aiEnt->activator->client )
			{//racc - picked up an NPC or client, make them non-visible and then drop them.
				aiEnt->client->ps.eFlags |= EF_NODRAW;//wahoofix - fix so that the person doesn't just jump out of the game entirely
				Rancor_DropVictim(aiEnt);//wahoo - drop the dude after you go back down
			}

			aiEnt->activator = aiEnt->enemy = aiEnt->NPC->goalEntity = NULL;
		}

		if ( aiEnt->enemy )
		{
			SandCreature_Chase(aiEnt);
		}
		else if ( (level.time - aiEnt->NPC->enemyLastSeenTime) < 5000 )//FIXME: should make this able to be variable
		{//we were alerted recently, move towards there and look for footsteps, etc.
			SandCreature_Hunt(aiEnt);
		}
		else
		{//no alerts, sleep and wake up only by alerts
			//FIXME: keeps chasing goalEntity even when it's already reached it!
			SandCreature_Sleep(aiEnt);
		}
	}
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
	if ( !visible )
	{
		aiEnt->client->ps.eFlags |= EF_NODRAW;
		aiEnt->s.eFlags |= EF_NODRAW;
	}
	else
	{
		aiEnt->client->ps.eFlags &= ~EF_NODRAW;
		aiEnt->s.eFlags &= ~EF_NODRAW;

		SandCreature_PushEnts(aiEnt);
	}
}

//FIXME: need pain behavior of sticking up through ground, writhing and screaming
//FIXME: need death anim like pain, but flopping aside and staying above ground...
//[/NPCSandCreature]

