//NPC_behavior.cpp
/*
FIXME - MCG:
These all need to make use of the snapshots.  Write something that can look for only specific
things in a snapshot or just go through the snapshot every frame and save the info in case
we need it...
*/

#include "b_local.h"
#include "g_nav.h"
#include "icarus/Q3_Interface.h"

extern	qboolean	showBBoxes;
extern vec3_t NPCDEBUG_BLUE;
extern void G_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
extern void NPC_CheckGetNewWeapon( gentity_t *aiEnt);

extern qboolean PM_InKnockDown( playerState_t *ps );

extern void NPC_AimAdjust( gentity_t *aiEnt, int change );
extern qboolean NPC_SomeoneLookingAtMe(gentity_t *ent);
/*
 void NPC_BSAdvanceFight (void)

Advance towards your captureGoal and shoot anyone you can along the way.
*/
void NPC_BSAdvanceFight (gentity_t *aiEnt )
{//FIXME: IMPLEMENT
	//Head to Goal if I can

	//Make sure we're still headed where we want to capture
	if ( aiEnt->NPC->captureGoal )
	{//FIXME: if no captureGoal, what do we do?
		//VectorCopy( NPCInfo->captureGoal->r.currentOrigin, NPCInfo->tempGoal->r.currentOrigin );
		//NPCInfo->goalEntity = NPCInfo->tempGoal;

		NPC_SetMoveGoal( aiEnt, aiEnt->NPC->captureGoal->r.currentOrigin, 16, qtrue, -1, NULL );

//		NAV_ClearLastRoute(NPC);
		aiEnt->NPC->goalTime = level.time + 100000;
	}

//	NPC_BSRun();

	NPC_CheckEnemy(aiEnt, qtrue, qfalse, qtrue);

	//FIXME: Need melee code
	if( aiEnt->enemy )
	{//See if we can shoot him
		vec3_t		delta, forward;
		vec3_t		angleToEnemy;
		vec3_t		hitspot, muzzle, diff, enemy_org, enemy_head;
		float		distanceToEnemy;
		qboolean	attack_ok = qfalse;
		qboolean	dead_on = qfalse;
		float		attack_scale = 1.0;
		float		aim_off;
		float		max_aim_off = 64;

		//Yaw to enemy
		VectorMA(aiEnt->enemy->r.absmin, 0.5, aiEnt->enemy->r.maxs, enemy_org);
		CalcEntitySpot( aiEnt, SPOT_WEAPON, muzzle );

		VectorSubtract (enemy_org, muzzle, delta);
		vectoangles ( delta, angleToEnemy );
		distanceToEnemy = VectorNormalize(delta);

		if(!NPC_EnemyTooFar(aiEnt, aiEnt->enemy, distanceToEnemy*distanceToEnemy, qtrue))
		{
			attack_ok = qtrue;
		}

		if(attack_ok)
		{
			NPC_UpdateShootAngles(aiEnt, angleToEnemy, qfalse, qtrue);

			aiEnt->NPC->enemyLastVisibility = NPCS.enemyVisibility;
			NPCS.enemyVisibility = NPC_CheckVisibility ( aiEnt, aiEnt->enemy, CHECK_FOV);//CHECK_360|//CHECK_PVS|

			if(NPCS.enemyVisibility == VIS_FOV)
			{//He's in our FOV

				attack_ok = qtrue;
				CalcEntitySpot( aiEnt->enemy, SPOT_HEAD, enemy_head);

				if(attack_ok)
				{
					trace_t		tr;
					gentity_t	*traceEnt;
					//are we gonna hit him if we shoot at his center?
					trap->Trace ( &tr, muzzle, NULL, NULL, enemy_org, aiEnt->s.number, MASK_SHOT, qfalse, 0, 0 );
					traceEnt = &g_entities[tr.entityNum];
					if( traceEnt != aiEnt->enemy &&
						(!traceEnt || !traceEnt->client || !aiEnt->client->enemyTeam || aiEnt->client->enemyTeam != traceEnt->client->playerTeam) )
					{//no, so shoot for the head
						attack_scale *= 0.75;
						trap->Trace ( &tr, muzzle, NULL, NULL, enemy_head, aiEnt->s.number, MASK_SHOT, qfalse, 0, 0 );
						traceEnt = &g_entities[tr.entityNum];
					}

					VectorCopy( tr.endpos, hitspot );

					if( traceEnt == aiEnt->enemy || (traceEnt->client && aiEnt->client->enemyTeam && aiEnt->client->enemyTeam == traceEnt->client->playerTeam) )
					{
						dead_on = qtrue;
					}
					else
					{
						attack_scale *= 0.5;
						if(aiEnt->client->playerTeam)
						{
							if(traceEnt && traceEnt->client && traceEnt->client->playerTeam)
							{
								if(aiEnt->client->playerTeam == traceEnt->client->playerTeam)
								{//Don't shoot our own team
									attack_ok = qfalse;
								}
							}
						}
					}
				}

				if( attack_ok )
				{
					//ok, now adjust pitch aim
					VectorSubtract (hitspot, muzzle, delta);
					vectoangles ( delta, angleToEnemy );
					aiEnt->NPC->desiredPitch = angleToEnemy[PITCH];
					NPC_UpdateShootAngles(aiEnt, angleToEnemy, qtrue, qfalse);

					if( !dead_on )
					{//We're not going to hit him directly, try a suppressing fire
						//see if where we're going to shoot is too far from his origin
						AngleVectors (aiEnt->NPC->shootAngles, forward, NULL, NULL);
						VectorMA ( muzzle, distanceToEnemy, forward, hitspot);
						VectorSubtract(hitspot, enemy_org, diff);
						aim_off = VectorLength(diff);
						if(aim_off > random() * max_aim_off)//FIXME: use aim value to allow poor aim?
						{
							attack_scale *= 0.75;
							//see if where we're going to shoot is too far from his head
							VectorSubtract(hitspot, enemy_head, diff);
							aim_off = VectorLength(diff);
							if(aim_off > random() * max_aim_off)
							{
								attack_ok = qfalse;
							}
						}
						attack_scale *= (max_aim_off - aim_off + 1)/max_aim_off;
					}
				}
			}
		}

		if( attack_ok )
		{
			if( NPC_CheckAttack( aiEnt, attack_scale ))
			{//check aggression to decide if we should shoot
				NPCS.enemyVisibility = VIS_SHOOT;
				WeaponThink(aiEnt, qtrue);
			}
		}
//Don't do this- only for when stationary and trying to shoot an enemy
//		else
//			NPC->cantHitEnemyCounter++;
	}
	else
	{//FIXME:
		NPC_UpdateShootAngles(aiEnt, aiEnt->client->ps.viewangles, qtrue, qtrue);
	}

	if(!aiEnt->client->pers.cmd.forwardmove && !aiEnt->client->pers.cmd.rightmove)
	{//We reached our captureGoal
#ifndef __NO_ICARUS__
		if(trap->ICARUS_IsInitialized(aiEnt->s.number))
		{
			trap->ICARUS_TaskIDComplete( (sharedEntity_t *)aiEnt, TID_BSTATE );
		}
#endif //__NO_ICARUS__
	}
}

void Disappear(gentity_t *self)
{
//	ClientDisconnect(self);
	self->s.eFlags |= EF_NODRAW;
	self->think = 0;
	self->nextthink = -1;
}

void MakeOwnerInvis (gentity_t *self);
void BeamOut (gentity_t *self)
{
//	gentity_t *tent = G_Spawn();

/*
	tent->owner = self;
	tent->think = MakeOwnerInvis;
	tent->nextthink = level.time + 1800;
	//G_AddEvent( ent, EV_PLAYER_TELEPORT, 0 );
	tent = G_TempEntity( self->client->pcurrentOrigin, EV_PLAYER_TELEPORT );
*/
	//fixme: doesn't actually go away!
	self->nextthink = level.time + 1500;
	self->think = Disappear;
	self->client->squadname = NULL;
	self->client->playerTeam = (npcteam_t)FACTION_FREE;
	self->s.teamowner = FACTION_FREE;
	//self->r.svFlags |= SVF_BEAMING; //this appears unused in SP as well
}

void NPC_BSCinematic(gentity_t *aiEnt)
{

	if( aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( aiEnt, qtrue );
	}

	if ( UpdateGoal(aiEnt) )
	{//have a goalEntity
		//move toward goal, should also face that goal
		NPC_MoveToGoal( aiEnt, qtrue );
	}

	if ( aiEnt->NPC->watchTarget )
	{//have an entity which we want to keep facing
		//NOTE: this will override any angles set by NPC_MoveToGoal
		vec3_t eyes, viewSpot, viewvec, viewangles;

		CalcEntitySpot( aiEnt, SPOT_HEAD_LEAN, eyes );
		CalcEntitySpot( aiEnt->NPC->watchTarget, SPOT_HEAD_LEAN, viewSpot );

		VectorSubtract( viewSpot, eyes, viewvec );

		vectoangles( viewvec, viewangles );

		aiEnt->NPC->lockedDesiredYaw = aiEnt->NPC->desiredYaw = viewangles[YAW];
		aiEnt->NPC->lockedDesiredPitch = aiEnt->NPC->desiredPitch = viewangles[PITCH];
	}

	NPC_UpdateAngles( aiEnt, qtrue, qtrue );
}

void NPC_BSWait(gentity_t *aiEnt)
{
	NPC_UpdateAngles( aiEnt, qtrue, qtrue );
}


void NPC_BSInvestigate (gentity_t *aiEnt)
{
/*
	//FIXME: maybe allow this to be set as a tempBState in a script?  Just specify the
	//investigateGoal, investigateDebounceTime and investigateCount? (Needs a macro)
	vec3_t		invDir, invAngles, spot;
	gentity_t	*saveGoal;
	//BS_INVESTIGATE would turn toward goal, maybe take a couple steps towards it,
	//look for enemies, then turn away after your investigate counter was down-
	//investigate counter goes up every time you set it...

	if(level.time > NPCInfo->enemyCheckDebounceTime)
	{
		NPCInfo->enemyCheckDebounceTime = level.time + (NPCInfo->stats.vigilance * 1000);
		NPC_CheckEnemy(qtrue, qfalse);
		if(NPC->enemy)
		{//FIXME: do anger script
			NPCInfo->goalEntity = NPC->enemy;
//			NAV_ClearLastRoute(NPC);
			NPCInfo->behaviorState = BS_RUN_AND_SHOOT;
			NPCInfo->tempBehavior = BS_DEFAULT;
			NPC_AngerSound();
			return;
		}
	}

	NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );

	if(NPCInfo->stats.vigilance <= 1.0 && NPCInfo->eventOwner)
	{
		VectorCopy(NPCInfo->eventOwner->r.currentOrigin, NPCInfo->investigateGoal);
	}

	saveGoal = NPCInfo->goalEntity;
	if(	level.time > NPCInfo->walkDebounceTime )
	{
		vec3_t	vec;

		VectorSubtract(NPCInfo->investigateGoal, NPC->r.currentOrigin, vec);
		vec[2] = 0;
		if(VectorLength(vec) > 64)
		{
			if(Q_irand(0, 100) < NPCInfo->investigateCount)
			{//take a full step
				//NPCInfo->walkDebounceTime = level.time + 1400;
				//actually finds length of my BOTH_WALK anim
				NPCInfo->walkDebounceTime = PM_AnimLength( NPC->client->clientInfo.animFileIndex, BOTH_WALK1 );
			}
		}
	}

	if(	level.time < NPCInfo->walkDebounceTime )
	{//walk toward investigateGoal

		/*
		NPCInfo->goalEntity = NPCInfo->tempGoal;
//		NAV_ClearLastRoute(NPC);
		VectorCopy(NPCInfo->investigateGoal, NPCInfo->tempGoal->r.currentOrigin);
		*/

/*		NPC_SetMoveGoal( NPC, NPCInfo->investigateGoal, 16, qtrue );

		NPC_MoveToGoal( qtrue );

		//FIXME: walk2?
		NPC_SetAnim(NPC,SETANIM_LEGS,BOTH_WALK1,SETANIM_FLAG_NORMAL);

		ucmd.buttons |= BUTTON_WALKING;
	}
	else
	{

		NPC_SetAnim(NPC,SETANIM_LEGS,BOTH_STAND1,SETANIM_FLAG_NORMAL);

		if(NPCInfo->hlookCount > 30)
		{
			if(Q_irand(0, 10) > 7)
			{
				NPCInfo->hlookCount = 0;
			}
		}
		else if(NPCInfo->hlookCount < -30)
		{
			if(Q_irand(0, 10) > 7)
			{
				NPCInfo->hlookCount = 0;
			}
		}
		else if(NPCInfo->hlookCount == 0)
		{
			NPCInfo->hlookCount = Q_irand(-1, 1);
		}
		else if(Q_irand(0, 10) > 7)
		{
			if(NPCInfo->hlookCount > 0)
			{
				NPCInfo->hlookCount++;
			}
			else//lookCount < 0
			{
				NPCInfo->hlookCount--;
			}
		}

		if(NPCInfo->vlookCount >= 15)
		{
			if(Q_irand(0, 10) > 7)
			{
				NPCInfo->vlookCount = 0;
			}
		}
		else if(NPCInfo->vlookCount <= -15)
		{
			if(Q_irand(0, 10) > 7)
			{
				NPCInfo->vlookCount = 0;
			}
		}
		else if(NPCInfo->vlookCount == 0)
		{
			NPCInfo->vlookCount = Q_irand(-1, 1);
		}
		else if(Q_irand(0, 10) > 8)
		{
			if(NPCInfo->vlookCount > 0)
			{
				NPCInfo->vlookCount++;
			}
			else//lookCount < 0
			{
				NPCInfo->vlookCount--;
			}
		}

		//turn toward investigateGoal
		CalcEntitySpot( NPC, SPOT_HEAD, spot );
		VectorSubtract(NPCInfo->investigateGoal, spot, invDir);
		VectorNormalize(invDir);
		vectoangles(invDir, invAngles);
		NPCInfo->desiredYaw = AngleNormalize360(invAngles[YAW] + NPCInfo->hlookCount);
		NPCInfo->desiredPitch = AngleNormalize360(invAngles[PITCH] + NPCInfo->hlookCount);
	}

	NPC_UpdateAngles(qtrue, qtrue);

	NPCInfo->goalEntity = saveGoal;
//	NAV_ClearLastRoute(NPC);

	if(level.time > NPCInfo->investigateDebounceTime)
	{
		NPCInfo->tempBehavior = BS_DEFAULT;
	}

	NPC_CheckSoundEvents();
	*/
}

qboolean NPC_CheckInvestigate(gentity_t *aiEnt, int alertEventNum )
{
	gentity_t	*owner = level.alertEvents[alertEventNum].owner;
	int		invAdd = level.alertEvents[alertEventNum].level;
	vec3_t	soundPos;
	float	soundRad = level.alertEvents[alertEventNum].radius;
	float	earshot = aiEnt->NPC->stats.earshot;

	VectorCopy( level.alertEvents[alertEventNum].position, soundPos );

	//NOTE: Trying to preserve previous investigation behavior
	if ( !owner )
	{
		return qfalse;
	}

	if ( owner->s.eType != ET_PLAYER && owner->s.eType != ET_NPC && owner == aiEnt->NPC->goalEntity )
	{
		return qfalse;
	}

	if ( owner->s.eFlags & EF_NODRAW )
	{
		return qfalse;
	}

	if ( owner->flags & FL_NOTARGET )
	{
		return qfalse;
	}

	if ( soundRad < earshot )
	{
		return qfalse;
	}

	//if(!trap->InPVSIgnorePortals(ent->r.currentOrigin, NPC->r.currentOrigin))//should we be able to hear through areaportals?
	if ( !trap->InPVS( soundPos, aiEnt->r.currentOrigin ) )
	{//can hear through doors?
		return qfalse;
	}

	if ( owner->client && owner->client->playerTeam && aiEnt->client->playerTeam && owner->client->playerTeam != aiEnt->client->playerTeam )
	{
		if( (float)aiEnt->NPC->investigateCount >= (aiEnt->NPC->stats.vigilance*200) && owner )
		{//If investigateCount == 10, just take it as enemy and go
			if ( ValidEnemy( aiEnt, owner ) )
			{//FIXME: run angerscript
				G_SetEnemy( aiEnt, owner );
				aiEnt->NPC->goalEntity = aiEnt->enemy;
				aiEnt->NPC->goalRadius = 12;
				aiEnt->NPC->behaviorState = BS_HUNT_AND_KILL;
				return qtrue;
			}
		}
		else
		{
			aiEnt->NPC->investigateCount += invAdd;
		}
		//run awakescript
		G_ActivateBehavior(aiEnt, BSET_AWAKE);

		/*
		if ( Q_irand(0, 10) > 7 )
		{
			NPC_AngerSound();
		}
		*/

		//NPCInfo->hlookCount = NPCInfo->vlookCount = 0;
		aiEnt->NPC->eventOwner = owner;
		VectorCopy( soundPos, aiEnt->NPC->investigateGoal );
		if ( aiEnt->NPC->investigateCount > 20 )
		{
			aiEnt->NPC->investigateDebounceTime = level.time + 10000;
		}
		else
		{
			aiEnt->NPC->investigateDebounceTime = level.time + (aiEnt->NPC->investigateCount*500);
		}
		aiEnt->NPC->tempBehavior = BS_INVESTIGATE;
		return qtrue;
	}

	return qfalse;
}


/*
void NPC_BSSleep( void )
*/
void NPC_BSSleep(gentity_t *aiEnt)
{
	int alertEvent = NPC_CheckAlertEvents( aiEnt, qtrue, qfalse, -1, qfalse, AEL_MINOR );

	//There is an event to look at
	if ( alertEvent >= 0 )
	{
		G_ActivateBehavior(aiEnt, BSET_AWAKE);
		return;
	}

	/*
	if ( level.time > NPCInfo->enemyCheckDebounceTime )
	{
		if ( NPC_CheckSoundEvents() != -1 )
		{//only 1 alert per second per 0.1 of vigilance
			NPCInfo->enemyCheckDebounceTime = level.time + (NPCInfo->stats.vigilance * 10000);
			G_ActivateBehavior(NPC, BSET_AWAKE);
		}
	}
	*/
}

extern qboolean NPC_MoveDirClear(gentity_t *aiEnt, int forwardmove, int rightmove, qboolean reset );
void NPC_BSFollowLeader (gentity_t *aiEnt)
{
	vec3_t		vec;
	float		leaderDist;
	visibility_t	leaderVis;
	int			curAnim;

	if ( !aiEnt->client->leader )
	{//ok, stand guard until we find an enemy
		if( aiEnt->NPC->tempBehavior == BS_HUNT_AND_KILL )
		{
			aiEnt->NPC->tempBehavior = BS_DEFAULT;
		}
		else
		{
			aiEnt->NPC->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard(aiEnt);
		}
		return;
	}

	if ( !aiEnt->enemy  )
	{//no enemy, find one
		NPC_CheckEnemy( aiEnt, (qboolean)(aiEnt->NPC->confusionTime<level.time), qfalse, qtrue );//don't find new enemy if this is tempbehav
		if ( aiEnt->enemy )
		{//just found one
			aiEnt->NPC->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
		}
		else
		{
			if ( !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
			{
				int eventID = NPC_CheckAlertEvents( aiEnt, qtrue, qtrue, -1, qfalse, AEL_MINOR );
				if ( level.alertEvents[eventID].level >= AEL_SUSPICIOUS && (aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
				{
					aiEnt->NPC->lastAlertID = level.alertEvents[eventID].ID;
					if ( !level.alertEvents[eventID].owner ||
						!level.alertEvents[eventID].owner->client ||
						level.alertEvents[eventID].owner->health <= 0 ||
						level.alertEvents[eventID].owner->client->playerTeam != aiEnt->client->enemyTeam )
					{//not an enemy
					}
					else
					{
						//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
						G_SetEnemy( aiEnt, level.alertEvents[eventID].owner );
						aiEnt->NPC->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
						aiEnt->NPC->enemyLastSeenTime = level.time;
						TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 1000 ) );
					}
				}

			}
		}
		if ( !aiEnt->enemy )
		{
			if ( aiEnt->client->leader
				&& aiEnt->client->leader->enemy
				&& aiEnt->client->leader->enemy != aiEnt
				&& ( (aiEnt->client->leader->enemy->client&&aiEnt->client->leader->enemy->client->playerTeam==aiEnt->client->enemyTeam)
					||(/*NPC->client->leader->enemy->r.svFlags&SVF_NONNPC_ENEMY*/0&&aiEnt->client->leader->enemy->alliedTeam==aiEnt->client->enemyTeam) )
				&& aiEnt->client->leader->enemy->health > 0 )
			{ //rwwFIXMEFIXME: use SVF_NONNPC_ENEMY?
				G_SetEnemy( aiEnt, aiEnt->client->leader->enemy );
				aiEnt->NPC->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
				aiEnt->NPC->enemyLastSeenTime = level.time;
			}
		}
	}
	else
	{
		if ( aiEnt->enemy->health <= 0 || (aiEnt->enemy->flags&FL_NOTARGET) )
		{
			G_ClearEnemy( aiEnt );
			if ( aiEnt->NPC->enemyCheckDebounceTime > level.time + 1000 )
			{
				aiEnt->NPC->enemyCheckDebounceTime = level.time + Q_irand( 1000, 2000 );
			}
		}
		else if ( aiEnt->client->ps.weapon && aiEnt->NPC->enemyCheckDebounceTime < level.time )
		{
			NPC_CheckEnemy(aiEnt, (qboolean)(aiEnt->NPC->confusionTime<level.time||aiEnt->NPC->tempBehavior!=BS_FOLLOW_LEADER), qfalse, qtrue );//don't find new enemy if this is tempbehav
		}
	}

	if ( aiEnt->enemy && aiEnt->client->ps.weapon )
	{//If have an enemy, face him and fire
		if ( aiEnt->client->ps.weapon == WP_SABER )//|| NPCInfo->confusionTime>level.time )
		{//lightsaber user or charmed enemy
			if ( aiEnt->NPC->tempBehavior != BS_FOLLOW_LEADER )
			{//not already in a temp bState
				//go after the guy
				aiEnt->NPC->tempBehavior = BS_HUNT_AND_KILL;
				NPC_UpdateAngles(aiEnt, qtrue, qtrue);
				return;
			}
		}

		NPCS.enemyVisibility = NPC_CheckVisibility (aiEnt, aiEnt->enemy, CHECK_FOV|CHECK_SHOOT );//CHECK_360|CHECK_PVS|
		if ( NPCS.enemyVisibility > VIS_PVS )
		{//face
			vec3_t	enemy_org, muzzle, delta, angleToEnemy;

			CalcEntitySpot( aiEnt->enemy, SPOT_HEAD, enemy_org );
			NPC_AimWiggle(aiEnt, enemy_org );

			CalcEntitySpot( aiEnt, SPOT_WEAPON, muzzle );

			VectorSubtract( enemy_org, muzzle, delta);
			vectoangles( delta, angleToEnemy );

			aiEnt->NPC->desiredYaw = angleToEnemy[YAW];
			aiEnt->NPC->desiredPitch = angleToEnemy[PITCH];
			NPC_UpdateFiringAngles(aiEnt, qtrue, qtrue );

			if ( NPCS.enemyVisibility >= VIS_SHOOT )
			{//shoot
				NPC_AimAdjust( aiEnt, 2 );
				if ( NPC_GetHFOVPercentage( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->client->ps.viewangles, aiEnt->NPC->stats.hfov ) > 0.6f
					&& NPC_GetHFOVPercentage( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->client->ps.viewangles, aiEnt->NPC->stats.vfov ) > 0.5f )
				{//actually withing our front cone
					WeaponThink(aiEnt, qtrue );
				}
			}
			else
			{
				NPC_AimAdjust(aiEnt, 1 );
			}

			//NPC_CheckCanAttack(aiEnt, 1.0, qfalse);
		}
		else
		{
			NPC_AimAdjust(aiEnt, -1 );
		}
	}
	else
	{//FIXME: combine with vector calc below
		vec3_t	head, leaderHead, delta, angleToLeader;

		CalcEntitySpot( aiEnt->client->leader, SPOT_HEAD, leaderHead );
		CalcEntitySpot( aiEnt, SPOT_HEAD, head );
		VectorSubtract (leaderHead, head, delta);
		vectoangles ( delta, angleToLeader );
		VectorNormalize(delta);
		aiEnt->NPC->desiredYaw = angleToLeader[YAW];
		aiEnt->NPC->desiredPitch = angleToLeader[PITCH];

		NPC_UpdateAngles(aiEnt, qtrue, qtrue);
	}

	//leader visible?
	leaderVis = NPC_CheckVisibility(aiEnt, aiEnt->client->leader, CHECK_PVS|CHECK_360|CHECK_SHOOT );//			ent->e_UseFunc = useF_NULL;


	//Follow leader, stay within visibility and a certain distance, maintain a distance from.
	curAnim = aiEnt->client->ps.legsAnim;
	if ( curAnim != BOTH_ATTACK1 && curAnim != BOTH_ATTACK2 && curAnim != BOTH_ATTACK3 && curAnim != BOTH_MELEE1 && curAnim != BOTH_MELEE2 )
	{//Don't move toward leader if we're in a full-body attack anim
		//FIXME, use IdealDistance to determine if we need to close distance
		float	followDist = 96.0f;//FIXME:  If there are enmies, make this larger?
		float	backupdist, walkdist, minrundist;
		float	leaderHDist;

		if ( aiEnt->NPC->followDist )
		{
			followDist = aiEnt->NPC->followDist;
		}
		backupdist = followDist/2.0f;
		walkdist = followDist*0.83;
		minrundist = followDist*1.33;

		VectorSubtract(aiEnt->client->leader->r.currentOrigin, aiEnt->r.currentOrigin, vec);
		leaderDist = VectorLength( vec );//FIXME: make this just nav distance?
		//never get within their radius horizontally
		vec[2] = 0;
		leaderHDist = VectorLength( vec );
		if( leaderHDist > backupdist && (leaderVis != VIS_SHOOT || leaderDist > walkdist) )
		{//We should close in?
			aiEnt->NPC->goalEntity = aiEnt->client->leader;

			NPC_SlideMoveToGoal(aiEnt);
			if ( leaderVis == VIS_SHOOT && leaderDist < minrundist )
			{
				aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			}
		}
		else if ( leaderDist < backupdist )
		{//We should back off?
			aiEnt->NPC->goalEntity = aiEnt->client->leader;
			NPC_SlideMoveToGoal(aiEnt);

			//reversing direction
			aiEnt->client->pers.cmd.forwardmove = -aiEnt->client->pers.cmd.forwardmove;
			aiEnt->client->pers.cmd.rightmove   = -aiEnt->client->pers.cmd.rightmove;
			VectorScale( aiEnt->client->ps.moveDir, -1, aiEnt->client->ps.moveDir );
		}//otherwise, stay where we are
		//check for do not enter and stop if there's one there...
		if ( aiEnt->client->pers.cmd.forwardmove || aiEnt->client->pers.cmd.rightmove || VectorCompare( vec3_origin, aiEnt->client->ps.moveDir ) )
		{
			NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, aiEnt->client->pers.cmd.rightmove, qtrue );
		}
	}
}
#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f
void NPC_BSJump (gentity_t *aiEnt)
{
	vec3_t		dir, angles, p1, p2, apex;
	float		time, height, forward, z, xy, dist, yawError, apexHeight;

	if( !aiEnt->NPC->goalEntity )
	{//Should have task completed the navgoal
		return;
	}

	if ( aiEnt->NPC->jumpState != JS_JUMPING && aiEnt->NPC->jumpState != JS_LANDING )
	{
		//Face navgoal
		VectorSubtract(aiEnt->NPC->goalEntity->r.currentOrigin, aiEnt->r.currentOrigin, dir);
		vectoangles(dir, angles);
		aiEnt->NPC->desiredPitch = aiEnt->NPC->lockedDesiredPitch = AngleNormalize360(angles[PITCH]);
		aiEnt->NPC->desiredYaw = aiEnt->NPC->lockedDesiredYaw = AngleNormalize360(angles[YAW]);
	}

	NPC_UpdateAngles ( aiEnt, qtrue, qtrue );
	yawError = AngleDelta ( aiEnt->client->ps.viewangles[YAW], aiEnt->NPC->desiredYaw );
	//We don't really care about pitch here

	switch ( aiEnt->NPC->jumpState )
	{
	case JS_FACING:
		if ( yawError < MIN_ANGLE_ERROR )
		{//Facing it, Start crouching
			NPC_SetAnim(aiEnt, SETANIM_LEGS, BOTH_CROUCH1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			aiEnt->NPC->jumpState = JS_CROUCHING;
		}
		break;
	case JS_CROUCHING:
		if ( aiEnt->client->ps.legsTimer > 0 )
		{//Still playing crouching anim
			return;
		}

		//Create a parabola

		if ( aiEnt->r.currentOrigin[2] > aiEnt->NPC->goalEntity->r.currentOrigin[2] )
		{
			VectorCopy( aiEnt->r.currentOrigin, p1 );
			VectorCopy( aiEnt->NPC->goalEntity->r.currentOrigin, p2 );
		}
		else if ( aiEnt->r.currentOrigin[2] < aiEnt->NPC->goalEntity->r.currentOrigin[2] )
		{
			VectorCopy( aiEnt->NPC->goalEntity->r.currentOrigin, p1 );
			VectorCopy( aiEnt->r.currentOrigin, p2 );
		}
		else
		{
			VectorCopy( aiEnt->r.currentOrigin, p1 );
			VectorCopy( aiEnt->NPC->goalEntity->r.currentOrigin, p2 );
		}

		//z = xy*xy
		VectorSubtract( p2, p1, dir );
		dir[2] = 0;

		//Get xy and z diffs
		xy = VectorNormalize( dir );
		z = p1[2] - p2[2];

		apexHeight = APEX_HEIGHT/2;
		/*
		//Determine most desirable apex height
		apexHeight = (APEX_HEIGHT * PARA_WIDTH/xy) + (APEX_HEIGHT * z/128);
		if ( apexHeight < APEX_HEIGHT * 0.5 )
		{
			apexHeight = APEX_HEIGHT*0.5;
		}
		else if ( apexHeight > APEX_HEIGHT * 2 )
		{
			apexHeight = APEX_HEIGHT*2;
		}
		*/

		//FIXME: length of xy will change curve of parabola, need to account for this
		//somewhere... PARA_WIDTH

		z = (sqrt(apexHeight + z) - sqrt(apexHeight));

		assert(z >= 0);

//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5/xy*100.0f);

		// Don't need to set apex xy if NPC is jumping directly up.
		if ( xy > 0.0f )
		{
			xy -= z;
			xy *= 0.5;

			assert(xy > 0);
		}

		VectorMA (p1, xy, dir, apex);
		apex[2] += apexHeight;

		VectorCopy(apex, aiEnt->pos1);

		//Now we have the apex, aim for it
		height = apex[2] - aiEnt->r.currentOrigin[2];
		time = sqrt( height / ( .5 * aiEnt->client->ps.gravity ) );
		if ( !time )
		{
//			Com_Printf("ERROR no time in jump\n");
			return;
		}

		// set s.origin2 to the push velocity
		VectorSubtract ( apex, aiEnt->r.currentOrigin, aiEnt->client->ps.velocity );
		aiEnt->client->ps.velocity[2] = 0;
		dist = VectorNormalize( aiEnt->client->ps.velocity );

		forward = dist / time;
		VectorScale( aiEnt->client->ps.velocity, forward, aiEnt->client->ps.velocity );

		aiEnt->client->ps.velocity[2] = time * aiEnt->client->ps.gravity;

//		Com_Printf( "%s jumping %s, gravity at %4.0f percent\n", NPC->targetname, vtos(NPC->client->ps.velocity), NPC->client->ps.gravity/8.0f );

		aiEnt->flags |= FL_NO_KNOCKBACK;
		aiEnt->NPC->jumpState = JS_JUMPING;
		//FIXME: jumpsound?
		break;
	case JS_JUMPING:

		if ( showBBoxes )
		{
			VectorAdd(aiEnt->r.mins, aiEnt->pos1, p1);
			VectorAdd(aiEnt->r.maxs, aiEnt->pos1, p2);
			G_Cube( p1, p2, NPCDEBUG_BLUE, 0.5 );
		}

		if ( aiEnt->s.groundEntityNum != ENTITYNUM_NONE)
		{//Landed, start landing anim
			//FIXME: if the
			VectorClear(aiEnt->client->ps.velocity);
			NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_LAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			aiEnt->NPC->jumpState = JS_LANDING;
			//FIXME: landsound?
		}
		else if ( aiEnt->client->ps.legsTimer > 0 )
		{//Still playing jumping anim
			//FIXME: apply jump velocity here, a couple frames after start, not right away
			return;
		}
		else
		{//still in air, but done with jump anim, play inair anim
			NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE);
		}
		break;
	case JS_LANDING:
		if ( aiEnt->client->ps.legsTimer > 0 )
		{//Still playing landing anim
			return;
		}
		else
		{
			aiEnt->NPC->jumpState = JS_WAITING;


			//task complete no matter what...
			NPC_ClearGoal(aiEnt);
			aiEnt->NPC->goalTime = level.time;
			aiEnt->NPC->aiFlags &= ~NPCAI_MOVING;
			aiEnt->client->pers.cmd.forwardmove = 0;
			aiEnt->flags &= ~FL_NO_KNOCKBACK;

#ifndef __NO_ICARUS__
			//Return that the goal was reached
			trap->ICARUS_TaskIDComplete( (sharedEntity_t *)aiEnt, TID_MOVE_NAV );
#endif //__NO_ICARUS__

			//Or should we keep jumping until reached goal?

			/*
			NPCInfo->goalEntity = UpdateGoal();
			if ( !NPCInfo->goalEntity )
			{
				NPC->flags &= ~FL_NO_KNOCKBACK;
				Q3_TaskIDComplete( NPC, TID_MOVE_NAV );
			}
			*/

		}
		break;
	case JS_WAITING:
	default:
		aiEnt->NPC->jumpState = JS_FACING;
		break;
	}
}

void NPC_BSRemove (gentity_t *aiEnt)
{
	NPC_UpdateAngles ( aiEnt, qtrue, qtrue );
	//OJKFIXME: clientnum 0
	if( !trap->InPVS( aiEnt->r.currentOrigin, g_entities[0].r.currentOrigin ) )//FIXME: use cg.vieworg?
	{ //rwwFIXMEFIXME: Care about all clients instead of just 0?
		G_UseTargets2( aiEnt, aiEnt, aiEnt->target3 );
		aiEnt->s.eFlags |= EF_NODRAW;
		aiEnt->s.eType = ET_INVISIBLE;
		aiEnt->r.contents = 0;
		aiEnt->health = 0;
		aiEnt->targetname = NULL;

		//Disappear in half a second
		aiEnt->think = G_FreeEntity;
		aiEnt->nextthink = level.time + FRAMETIME;
	}//FIXME: else allow for out of FOV???
}

void NPC_BSSearch (gentity_t *aiEnt)
{
	NPC_CheckEnemy(aiEnt, qtrue, qfalse, qtrue);
	//Look for enemies, if find one:
	if ( aiEnt->enemy )
	{
		if( aiEnt->NPC->tempBehavior == BS_SEARCH )
		{//if tempbehavior, set tempbehavior to default
			aiEnt->NPC->tempBehavior = BS_DEFAULT;
		}
		else
		{//if bState, change to run and shoot
			aiEnt->NPC->behaviorState = BS_HUNT_AND_KILL;
			NPC_BSRunAndShoot(aiEnt);
		}
		return;
	}

	//FIXME: what if our goalEntity is not NULL and NOT our tempGoal - they must
	//want us to do something else?  If tempBehavior, just default, else set
	//to run and shoot...?

	//FIXME: Reimplement

	if ( !aiEnt->NPC->investigateDebounceTime )
	{//On our way to a tempGoal
		float	minGoalReachedDistSquared = 32*32;
		vec3_t	vec;

		//Keep moving toward our tempGoal
		aiEnt->NPC->goalEntity = aiEnt->NPC->tempGoal;

		VectorSubtract ( aiEnt->NPC->tempGoal->r.currentOrigin, aiEnt->r.currentOrigin, vec);
		if ( vec[2] < 24 )
		{
			vec[2] = 0;
		}

		if ( aiEnt->NPC->tempGoal->waypoint != WAYPOINT_NONE )
		{
			/*
			//FIXME: can't get the radius...
			float	wpRadSq = waypoints[NPCInfo->tempGoal->waypoint].radius * waypoints[NPCInfo->tempGoal->waypoint].radius;
			if ( minGoalReachedDistSquared > wpRadSq )
			{
				minGoalReachedDistSquared = wpRadSq;
			}
			*/

			minGoalReachedDistSquared = 32*32;//12*12;
		}

		if ( VectorLengthSquared( vec ) < minGoalReachedDistSquared )
		{
			//Close enough, just got there
			aiEnt->waypoint = NAV_FindClosestWaypointForEnt( aiEnt, WAYPOINT_NONE );

			if ( ( aiEnt->NPC->homeWp == WAYPOINT_NONE ) || ( aiEnt->waypoint == WAYPOINT_NONE ) )
			{
				//Heading for or at an invalid waypoint, get out of this bState
				if( aiEnt->NPC->tempBehavior == BS_SEARCH )
				{//if tempbehavior, set tempbehavior to default
					aiEnt->NPC->tempBehavior = BS_DEFAULT;
				}
				else
				{//if bState, change to stand guard
					aiEnt->NPC->behaviorState = BS_STAND_GUARD;
					NPC_BSRunAndShoot(aiEnt);
				}
				return;
			}

			if ( aiEnt->waypoint == aiEnt->NPC->homeWp )
			{
				//Just Reached our homeWp, if this is the first time, run your lostenemyscript
				if ( aiEnt->NPC->aiFlags & NPCAI_ENROUTE_TO_HOMEWP )
				{
					aiEnt->NPC->aiFlags &= ~NPCAI_ENROUTE_TO_HOMEWP;
					G_ActivateBehavior( aiEnt, BSET_LOSTENEMY );
				}

			}

			//Com_Printf("Got there.\n");
			//Com_Printf("Looking...");
			if( !Q_irand(0, 1) )
			{
				NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_NORMAL);
			}
			else
			{
				NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_NORMAL);
			}
			aiEnt->NPC->investigateDebounceTime = level.time + Q_irand(3000, 10000);
		}
		else
		{
			NPC_MoveToGoal(aiEnt, qtrue );
		}
	}
	else
	{
		//We're there
		if ( aiEnt->NPC->investigateDebounceTime > level.time )
		{
			//Still waiting around for a bit
			//Turn angles every now and then to look around
			if ( aiEnt->NPC->tempGoal->waypoint != WAYPOINT_NONE )
			{
				if ( !Q_irand( 0, 30 ) )
				{
					int	numEdges = trap->Nav_GetNodeNumEdges( aiEnt->NPC->tempGoal->waypoint );

					if ( numEdges != WAYPOINT_NONE )
					{
						int branchNum = Q_irand( 0, numEdges - 1 );

						vec3_t	branchPos, lookDir;

						int nextWp = trap->Nav_GetNodeEdge( aiEnt->NPC->tempGoal->waypoint, branchNum );
						trap->Nav_GetNodePosition( nextWp, branchPos );

						VectorSubtract( branchPos, aiEnt->NPC->tempGoal->r.currentOrigin, lookDir );
						aiEnt->NPC->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + flrand( -45, 45 ) );
					}

					//pick an angle +-45 degrees off of the dir of a random branch
					//from NPCInfo->tempGoal->waypoint
					//int branch = Q_irand( 0, (waypoints[NPCInfo->tempGoal->waypoint].numNeighbors - 1) );
					//int	nextWp = waypoints[NPCInfo->tempGoal->waypoint].nextWaypoint[branch][NPC->client->moveType];
					//vec3_t	lookDir;

					//VectorSubtract( waypoints[nextWp].origin, NPCInfo->tempGoal->r.currentOrigin, lookDir );
					//Look in that direction +- 45 degrees
					//NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + Q_flrand( -45, 45 ) );
				}
			}
			//Com_Printf(".");
		}
		else
		{//Just finished waiting
			aiEnt->waypoint = NAV_FindClosestWaypointForEnt( aiEnt, WAYPOINT_NONE );

			if ( aiEnt->waypoint == aiEnt->NPC->homeWp )
			{
				int	numEdges = trap->Nav_GetNodeNumEdges( aiEnt->NPC->tempGoal->waypoint );

				if ( numEdges != WAYPOINT_NONE )
				{
					int branchNum = Q_irand( 0, numEdges - 1 );

					int nextWp = trap->Nav_GetNodeEdge( aiEnt->NPC->homeWp, branchNum );
					trap->Nav_GetNodePosition( nextWp, aiEnt->NPC->tempGoal->r.currentOrigin );
					aiEnt->NPC->tempGoal->waypoint = nextWp;
				}

				/*
				//Pick a random branch
				int branch = Q_irand( 0, (waypoints[NPCInfo->homeWp].numNeighbors - 1) );
				int	nextWp = waypoints[NPCInfo->homeWp].nextWaypoint[branch][NPC->client->moveType];

				VectorCopy( waypoints[nextWp].origin, NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = nextWp;
				//Com_Printf("\nHeading for wp %d...\n", waypoints[NPCInfo->homeWp].nextWaypoint[branch][NPC->client->moveType]);
				*/
			}
			else
			{//At a branch, so return home
				trap->Nav_GetNodePosition( aiEnt->NPC->homeWp, aiEnt->NPC->tempGoal->r.currentOrigin );
				aiEnt->NPC->tempGoal->waypoint = aiEnt->NPC->homeWp;
				/*
				VectorCopy( waypoints[NPCInfo->homeWp].origin, NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = NPCInfo->homeWp;
				//Com_Printf("\nHeading for wp %d...\n", NPCInfo->homeWp);
				*/
			}

			aiEnt->NPC->investigateDebounceTime = 0;
			//Start moving toward our tempGoal
			aiEnt->NPC->goalEntity = aiEnt->NPC->tempGoal;
			NPC_MoveToGoal(aiEnt, qtrue );
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

void NPC_PickRandomTempGoal ( gentity_t *aiEnt)
{
	if (!aiEnt->NPC->tempGoal)
	{// Pick a random enemy as a temp goal...
		int i;
		gentity_t	*bestEnt = NULL;
		float		bestDist = 999999.9f;

		for (i = 0; i < MAX_GENTITIES; i++)
		{
			float dist;
			gentity_t *ent = &g_entities[i];

			if (!ent || !ent->inuse)
				continue;

			if ((ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC) && !ValidEnemy(aiEnt, ent))
				continue;

			switch (ent->s.eType)
			{
			case ET_ITEM:
			case ET_HOLOCRON:
			case ET_PUSH_TRIGGER:
			case ET_TELEPORT_TRIGGER:
			case ET_BODY:
				break;
			default:
				continue;
				break;
			}

			dist = Distance(ent->r.currentOrigin, aiEnt->r.currentOrigin);

			if (dist < bestDist)
			{
				if (bestEnt && irand(0,10) >= 10) 
					continue;

				bestDist = dist;
				bestEnt = ent;
			}
		}

		aiEnt->NPC->tempGoal = aiEnt;
	}
}

/*
-------------------------
NPC_BSSearchStart
-------------------------
*/

void NPC_BSSearchStart( gentity_t *aiEnt, int homeWp, bState_t bState )
{
	//FIXME: Reimplement
	if ( homeWp == WAYPOINT_NONE )
	{
		homeWp = NAV_FindClosestWaypointForEnt( aiEnt, WAYPOINT_NONE );
		if( aiEnt->waypoint == WAYPOINT_NONE )
		{
			aiEnt->waypoint = homeWp;
		}
	}

	//NPC_PickRandomTempGoal();

	if (!aiEnt->NPC->tempGoal) return; // Still nowhere to go...

	aiEnt->NPC->homeWp = homeWp;
	aiEnt->NPC->tempBehavior = bState;
	aiEnt->NPC->aiFlags |= NPCAI_ENROUTE_TO_HOMEWP;
	aiEnt->NPC->investigateDebounceTime = 0;
	//trap->Nav_GetNodePosition( homeWp, aiEnt->NPC->tempGoal->r.currentOrigin );
	//aiEnt->NPC->tempGoal->waypoint = homeWp;
	//Com_Printf("\nHeading for wp %d...\n", NPCInfo->homeWp);
	aiEnt->NPC->tempGoal->waypoint = NAV_FindClosestWaypointForEnt( aiEnt->NPC->tempGoal, WAYPOINT_NONE );
}

/*
-------------------------
NPC_BSNoClip

  Use in extreme circumstances only
-------------------------
*/

void NPC_BSNoClip (gentity_t *aiEnt )
{
	if ( UpdateGoal(aiEnt) )
	{
		vec3_t	dir, forward, right, angles, up = {0, 0, 1};
		float	fDot, rDot, uDot;

		VectorSubtract( aiEnt->NPC->goalEntity->r.currentOrigin, aiEnt->r.currentOrigin, dir );

		vectoangles( dir, angles );
		aiEnt->NPC->desiredYaw = angles[YAW];

		AngleVectors( aiEnt->r.currentAngles, forward, right, NULL );

		VectorNormalize( dir );

		fDot = DotProduct(forward, dir) * 127;
		rDot = DotProduct(right, dir) * 127;
		uDot = DotProduct(up, dir) * 127;

		aiEnt->client->pers.cmd.forwardmove = floor(fDot);
		aiEnt->client->pers.cmd.rightmove = floor(rDot);
		aiEnt->client->pers.cmd.upmove = floor(uDot);
	}
	else
	{
		//Cut velocity?
		VectorClear( aiEnt->client->ps.velocity );
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

void NPC_BSWander (gentity_t *aiEnt)
{//FIXME: don't actually go all the way to the next waypoint, just move in fits and jerks...?
	if ( !aiEnt->NPC->investigateDebounceTime )
	{//Starting out
		float	minGoalReachedDistSquared = 64;//32*32;
		vec3_t	vec;

		//Keep moving toward our tempGoal
		aiEnt->NPC->goalEntity = aiEnt->NPC->tempGoal;

		VectorSubtract ( aiEnt->NPC->tempGoal->r.currentOrigin, aiEnt->r.currentOrigin, vec);

		if ( aiEnt->NPC->tempGoal->waypoint != WAYPOINT_NONE )
		{
			minGoalReachedDistSquared = 64;
		}

		if ( VectorLengthSquared( vec ) < minGoalReachedDistSquared )
		{
			//Close enough, just got there
			aiEnt->waypoint = NAV_FindClosestWaypointForEnt( aiEnt, WAYPOINT_NONE );

			if( !Q_irand(0, 1) )
			{
				NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_NORMAL);
			}
			else
			{
				NPC_SetAnim(aiEnt, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_NORMAL);
			}
			//Just got here, so Look around for a while
			aiEnt->NPC->investigateDebounceTime = level.time + Q_irand(3000, 10000);
		}
		else
		{
			//Keep moving toward goal
			NPC_MoveToGoal(aiEnt, qtrue );
		}
	}
	else
	{
		//We're there
		if ( aiEnt->NPC->investigateDebounceTime > level.time )
		{
			//Still waiting around for a bit
			//Turn angles every now and then to look around
			if ( aiEnt->NPC->tempGoal->waypoint != WAYPOINT_NONE )
			{
				if ( !Q_irand( 0, 30 ) )
				{
					int	numEdges = trap->Nav_GetNodeNumEdges( aiEnt->NPC->tempGoal->waypoint );

					if ( numEdges != WAYPOINT_NONE )
					{
						int branchNum = Q_irand( 0, numEdges - 1 );

						vec3_t	branchPos, lookDir;

						int	nextWp = trap->Nav_GetNodeEdge( aiEnt->NPC->tempGoal->waypoint, branchNum );
						trap->Nav_GetNodePosition( nextWp, branchPos );

						VectorSubtract( branchPos, aiEnt->NPC->tempGoal->r.currentOrigin, lookDir );
						aiEnt->NPC->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + flrand( -45, 45 ) );
					}
				}
			}
		}
		else
		{//Just finished waiting
			aiEnt->waypoint = NAV_FindClosestWaypointForEnt( aiEnt, WAYPOINT_NONE );

			if ( aiEnt->waypoint != WAYPOINT_NONE )
			{
				int	numEdges = trap->Nav_GetNodeNumEdges( aiEnt->waypoint );

				if ( numEdges != WAYPOINT_NONE )
				{
					int branchNum = Q_irand( 0, numEdges - 1 );

					int nextWp = trap->Nav_GetNodeEdge( aiEnt->waypoint, branchNum );
					trap->Nav_GetNodePosition( nextWp, aiEnt->NPC->tempGoal->r.currentOrigin );
					aiEnt->NPC->tempGoal->waypoint = nextWp;
				}

				aiEnt->NPC->investigateDebounceTime = 0;
				//Start moving toward our tempGoal
				aiEnt->NPC->goalEntity = aiEnt->NPC->tempGoal;
				NPC_MoveToGoal(aiEnt, qtrue );
			}
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

/*
void NPC_BSFaceLeader (void)
{
	vec3_t	head, leaderHead, delta, angleToLeader;

	if ( !NPC->client->leader )
	{//uh.... okay.
		return;
	}

	CalcEntitySpot( NPC->client->leader, SPOT_HEAD, leaderHead );
	CalcEntitySpot( NPC, SPOT_HEAD, head );
	VectorSubtract( leaderHead, head, delta );
	vectoangles( delta, angleToLeader );
	VectorNormalize( delta );
	NPC->NPC->desiredYaw = angleToLeader[YAW];
	NPC->NPC->desiredPitch = angleToLeader[PITCH];

	NPC_UpdateAngles(qtrue, qtrue);
}
*/
/*
-------------------------
NPC_BSFlee
-------------------------
*/
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void WP_DropWeapon( gentity_t *dropper, vec3_t velocity );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
void NPC_Surrender(gentity_t *aiEnt)
{//FIXME: say "don't shoot!" if we weren't already surrendering
	if ( aiEnt->client->ps.weaponTime || PM_InKnockDown( &aiEnt->client->ps ) )
	{
		return;
	}
	if ( aiEnt->s.weapon != WP_NONE && aiEnt->s.weapon != WP_SABER )
	{
		//WP_DropWeapon( NPC, NULL ); //rwwFIXMEFIXME: Do this (gonna need a system for notifying client of removal)
	}
	if ( aiEnt->NPC->surrenderTime < level.time - 5000 )
	{//haven't surrendered for at least 6 seconds, tell them what you're doing
		//FIXME: need real dialogue EV_SURRENDER
		aiEnt->NPC->blockedSpeechDebounceTime = 0;//make sure we say this
		G_AddVoiceEvent( aiEnt, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 3000 );
	}
//	NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
//	NPC->client->ps.torsoTimer = 1000;
	aiEnt->NPC->surrenderTime = level.time + 1000;//stay surrendered for at least 1 second
	//FIXME: while surrendering, make a big sight/sound alert? Or G_AlertTeam?
}

qboolean NPC_CheckSurrender(gentity_t *aiEnt)
{
	if (
#ifndef __NO_ICARUS__
		!trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) &&
#endif //__NO_ICARUS__
		aiEnt->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !aiEnt->client->ps.weaponTime && !PM_InKnockDown( &aiEnt->client->ps )
		&& aiEnt->enemy && aiEnt->enemy->client && aiEnt->enemy->enemy == aiEnt && aiEnt->enemy->s.weapon != WP_NONE
		&& aiEnt->enemy->health > 20 && aiEnt->enemy->painDebounceTime < level.time - 3000 && aiEnt->enemy->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time - 1000 )
	{//don't surrender if scripted to run somewhere or if we're in the air or if we're busy or if we don't have an enemy or if the enemy is not mad at me or is hurt or not a threat or busy being attacked
		//FIXME: even if not in a group, don't surrender if there are other enemies in the PVS and within a certain range?
		if ( aiEnt->s.weapon != WP_SABER )
		{//jedi and heavy weapons guys never surrender
			//FIXME: rework all this logic into some orderly fashion!!!
			if ( aiEnt->s.weapon != WP_NONE )
			{//they have a weapon so they'd have to drop it to surrender
				//don't give up unless low on health
				if ( aiEnt->health > 25 /*|| NPC->health >= NPC->max_health*/ )
				{ //rwwFIXMEFIXME: Keep max health not a ps state?
					return qfalse;
				}
				//if ( g_crosshairEntNum == NPC->s.number && NPC->painDebounceTime > level.time )
				if (NPC_SomeoneLookingAtMe(aiEnt) && aiEnt->painDebounceTime > level.time)
				{//if he just shot me, always give up
					//fall through
				}
				else
				{//don't give up unless facing enemy and he's very close
					if ( !InFOV( aiEnt->enemy, aiEnt, 60, 30 ) )
					{//I'm not looking at them
						return qfalse;
					}
					else if ( DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin ) < 65536/*256*256*/ )
					{//they're not close
						return qfalse;
					}
					else if ( !trap->InPVS( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin ) )
					{//they're not in the same room
						return qfalse;
					}
				}
			}
			//fixme: this logic keeps making npc's randomly surrender
			/*
			if ( NPCInfo->group && NPCInfo->group->numGroup <= 1 )
			{//I'm alone but I was in a group//FIXME: surrender anyway if just melee or no weap?
				if ( NPC->s.weapon == WP_NONE
					//NPC has a weapon
					|| (NPC->enemy && NPC->enemy->s.number < MAX_CLIENTS)
					|| (NPC->enemy->s.weapon == WP_SABER&&NPC->enemy->client&&!NPC->enemy->client->ps.saberHolstered)
					|| (NPC->enemy->NPC && NPC->enemy->NPC->group && NPC->enemy->NPC->group->numGroup > 2) )
				{//surrender only if have no weapon or fighting a player or jedi or if we are outnumbered at least 3 to 1
					if ( (NPC->enemy && NPC->enemy->s.number < MAX_CLIENTS) )
					{//player is the guy I'm running from
						//if ( g_crosshairEntNum == NPC->s.number )
						if (NPC_SomeoneLookingAtMe(NPC))
						{//give up if player is aiming at me
							NPC_Surrender();
							NPC_UpdateAngles( qtrue, qtrue );
							return qtrue;
						}
						else if ( NPC->enemy->s.weapon == WP_SABER )
						{//player is using saber
							if ( InFOV( NPC, NPC->enemy, 60, 30 ) )
							{//they're looking at me
								if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < 16384 )
								{//they're close
									if ( trap->InPVS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
									{//they're in the same room
										NPC_Surrender();
										NPC_UpdateAngles( qtrue, qtrue );
										return qtrue;
									}
								}
							}
						}
					}
					else if ( NPC->enemy )
					{//???
						//should NPC's surrender to others?
						if ( InFOV( NPC, NPC->enemy, 30, 30 ) )
						{//they're looking at me
							if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < 4096 )
							{//they're close
								if ( trap->InPVS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
								{//they're in the same room
									//FIXME: should player-team NPCs not fire on surrendered NPCs?
									NPC_Surrender();
									NPC_UpdateAngles( qtrue, qtrue );
									return qtrue;
								}
							}
						}
					}
				}
			}
			*/
		}
	}
	return qfalse;
}

void NPC_BSFlee(gentity_t *aiEnt)
{//FIXME: keep checking for danger
	gentity_t *goal;

	if ( TIMER_Done( aiEnt, "flee" ) && aiEnt->NPC->tempBehavior == BS_FLEE )
	{
		aiEnt->NPC->tempBehavior = BS_DEFAULT;
		aiEnt->NPC->squadState = SQUAD_IDLE;
		//FIXME: should we set some timer to make him stay in this spot for a bit,
		//so he doesn't just suddenly turn around and come back at the enemy?
		//OR, just stop running toward goal for last second or so of flee?
	}
	if ( NPC_CheckSurrender(aiEnt) )
	{
		return;
	}
	goal = aiEnt->NPC->goalEntity;
	if ( !goal )
	{
		goal = aiEnt->NPC->lastGoalEntity;
		if ( !goal )
		{//???!!!
			goal = aiEnt->NPC->tempGoal;
		}
	}

	if ( goal )
	{
		qboolean moved;
		qboolean reverseCourse = qtrue;

		//FIXME: if no weapon, find one and run to pick it up?

		//Let's try to find a waypoint that gets me away from this thing
		if ( aiEnt->waypoint == WAYPOINT_NONE )
		{
			aiEnt->waypoint = NAV_GetNearestNode( aiEnt, aiEnt->lastWaypoint );
		}
		if ( aiEnt->waypoint != WAYPOINT_NONE )
		{
			int	numEdges = trap->Nav_GetNodeNumEdges( aiEnt->waypoint );

			if ( numEdges != WAYPOINT_NONE )
			{
				vec3_t	dangerDir;
				int		nextWp;
				int		branchNum;

				VectorSubtract( aiEnt->NPC->investigateGoal, aiEnt->r.currentOrigin, dangerDir );
				VectorNormalize( dangerDir );

				for ( branchNum = 0; branchNum < numEdges; branchNum++ )
				{
					vec3_t	branchPos, runDir;

					nextWp = trap->Nav_GetNodeEdge( aiEnt->waypoint, branchNum );
					trap->Nav_GetNodePosition( nextWp, branchPos );

					VectorSubtract( branchPos, aiEnt->r.currentOrigin, runDir );
					VectorNormalize( runDir );
					if ( DotProduct( runDir, dangerDir ) > flrand( 0, 0.5 ) )
					{//don't run toward danger
						continue;
					}
					//FIXME: don't want to ping-pong back and forth
					NPC_SetMoveGoal( aiEnt, branchPos, 0, qtrue, -1, NULL );
					reverseCourse = qfalse;
					break;
				}
			}
		}

		moved = NPC_MoveToGoal(aiEnt, qfalse );//qtrue? (do try to move straight to (away from) goal)

		if ( aiEnt->s.weapon == WP_NONE && (moved == qfalse || reverseCourse) )
		{//No weapon and no escape route... Just cower?  Need anim.
			NPC_Surrender(aiEnt);
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return;
		}
		//If our move failed, then just run straight away from our goal
		//FIXME: We really shouldn't do this.
		if ( moved == qfalse )
		{
			vec3_t	dir;
			if ( reverseCourse )
			{
				VectorSubtract( aiEnt->r.currentOrigin, goal->r.currentOrigin, dir );
			}
			else
			{
				VectorSubtract( goal->r.currentOrigin, aiEnt->r.currentOrigin, dir );
			}
			aiEnt->NPC->distToGoal = VectorNormalize( dir );
			aiEnt->NPC->desiredYaw = vectoyaw( dir );
			aiEnt->NPC->desiredPitch = 0;
			aiEnt->client->pers.cmd.forwardmove = 127;
		}
		else if ( reverseCourse )
		{
			//ucmd.forwardmove *= -1;
			//ucmd.rightmove *= -1;
			//VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
			aiEnt->NPC->desiredYaw *= -1;
		}
		//FIXME: can stop after a safe distance?
		//ucmd.upmove = 0;
		aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
		//FIXME: what do we do once we've gotten to our goal?
	}
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	NPC_CheckGetNewWeapon(aiEnt);
}

void NPC_StartFlee( gentity_t *aiEnt, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax )
{
	int cp = -1;

#ifndef __NO_ICARUS__
	if ( trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) )
	{//running somewhere that a script requires us to go, don't interrupt that!
		return;
	}
#endif //__NO_ICARUS__

	//if have a fleescript, run that instead
	if ( G_ActivateBehavior( aiEnt, BSET_FLEE ) )
	{
		return;
	}
	//FIXME: play a flee sound?  Appropriate to situation?
	if ( enemy )
	{
		G_SetEnemy( aiEnt, enemy );
	}

	//FIXME: if don't have a weapon, find nearest one we have a route to and run for it?
	if ( dangerLevel > AEL_DANGER || aiEnt->s.weapon == WP_NONE || ((!aiEnt->NPC->group || aiEnt->NPC->group->numGroup <= 1) && aiEnt->health <= 10 ) )
	{//IF either great danger OR I have no weapon OR I'm alone and low on health, THEN try to find a combat point out of PVS
		cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, dangerPoint, CP_COVER|CP_AVOID|CP_HAS_ROUTE|CP_NO_PVS, 128, -1 );
	}
	//FIXME: still happens too often...
	if ( cp == -1 )
	{//okay give up on the no PVS thing
		cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, dangerPoint, CP_COVER|CP_AVOID|CP_HAS_ROUTE, 128, -1 );
		if ( cp == -1 )
		{//okay give up on the avoid
			cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, dangerPoint, CP_COVER|CP_HAS_ROUTE, 128, -1 );
			if ( cp == -1 )
			{//okay give up on the cover
				cp = NPC_FindCombatPoint(aiEnt, aiEnt->r.currentOrigin, aiEnt->r.currentOrigin, dangerPoint, CP_HAS_ROUTE, 128, -1 );
			}
		}
	}

	//see if we got a valid one
	if ( cp != -1 )
	{//found a combat point
		NPC_SetCombatPoint(aiEnt, cp );
		NPC_SetMoveGoal( aiEnt, level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
		aiEnt->NPC->behaviorState = BS_HUNT_AND_KILL;
		aiEnt->NPC->tempBehavior = BS_DEFAULT;
	}
	else
	{//need to just run like hell!
		if ( aiEnt->s.weapon != WP_NONE )
		{
			return;//let's just not flee?
		}
		else
		{
			//FIXME: other evasion AI?  Duck?  Strafe?  Dodge?
			aiEnt->NPC->tempBehavior = BS_FLEE;
			//Run straight away from here... FIXME: really want to find farthest waypoint/navgoal from this pos... maybe based on alert event radius?
			NPC_SetMoveGoal( aiEnt, dangerPoint, 0, qtrue, -1, NULL );
			//store the danger point
			VectorCopy( dangerPoint, aiEnt->NPC->investigateGoal );//FIXME: make a new field for this?
		}
	}
	//FIXME: localize this Timer?
	TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
	//FIXME: is this always applicable?
	aiEnt->NPC->squadState = SQUAD_RETREAT;
	TIMER_Set( aiEnt, "flee", Q_irand( fleeTimeMin, fleeTimeMax ) );
	TIMER_Set( aiEnt, "panic", Q_irand( 1000, 4000 ) );//how long to wait before trying to nav to a dropped weapon

	if (aiEnt->client->NPC_class != CLASS_PROTOCOL)
	{
		TIMER_Set( aiEnt, "duck", 0 );
	}
}

void G_StartFlee( gentity_t *self, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax )
{
	if ( !self->NPC )
	{//player
		return;
	}

	NPC_StartFlee(self, enemy, dangerPoint, dangerLevel, fleeTimeMin, fleeTimeMax );
}

void NPC_BSEmplaced(gentity_t *aiEnt)
{
	qboolean enemyLOS = qfalse;
	qboolean enemyCS = qfalse;
	qboolean faceEnemy = qfalse;
	qboolean shoot = qfalse;
	vec3_t	impactPos;

	//Don't do anything if we're hurt
	if ( aiEnt->painDebounceTime > level.time )
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	if( aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink(aiEnt, qtrue );
	}

	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )
	{
		if ( !Q_irand( 0, 30 ) )
		{
			aiEnt->NPC->desiredYaw = aiEnt->s.angles[1] + Q_irand( -90, 90 );
		}
		if ( !Q_irand( 0, 30 ) )
		{
			aiEnt->NPC->desiredPitch = Q_irand( -20, 20 );
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	if ( NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
	{
		int hit;
		gentity_t *hitEnt;

		enemyLOS = qtrue;

		hit = NPC_ShotEntity(aiEnt, aiEnt->enemy, impactPos );
		hitEnt = &g_entities[hit];

		if ( hit == aiEnt->enemy->s.number || ( hitEnt && hitEnt->takedamage ) )
		{//can hit enemy or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
			enemyCS = qtrue;
			NPC_AimAdjust( aiEnt, 2 );//adjust aim better longer we have clear shot at enemy
			VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
		}
	}
/*
	else if ( trap->InPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
		NPC_AimAdjust( aiEnt, -1 );//adjust aim worse longer we cannot see enemy
	}
*/

	if ( enemyLOS )
	{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy = qtrue;
	}
	if ( enemyCS )
	{
		shoot = qtrue;
	}

	if ( faceEnemy )
	{//face the enemy
		NPC_FaceEnemy(aiEnt, qtrue );
	}
	else
	{//we want to face in the dir we're running
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
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
	if ( shoot )
	{//try to shoot if it's time
		if( !(aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
		{
			WeaponThink(aiEnt, qtrue );
		}
	}
}
