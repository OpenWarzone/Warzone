
#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

//extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void WP_DeactivateSaber( gentity_t *self, qboolean clearLength );
extern int PM_AnimLength( int index, animNumber_t anim );

qboolean NPC_CheckPlayerTeamStealth(gentity_t *aiEnt);

static qboolean enemyLOS;
static qboolean enemyCS;
static qboolean faceEnemy;
static qboolean move;
static qboolean shoot;
static float	enemyDist;

/*
-------------------------
ST_Move
-------------------------
*/

static qboolean SaberDroid_Move(gentity_t *aiEnt)
{//movement while in attack move.
	qboolean	moved;
	aiEnt->NPC->combatMove = qtrue;//always move straight toward our goal
	UpdateGoal(aiEnt);
	if ( !aiEnt->NPC->goalEntity )
	{
		aiEnt->NPC->goalEntity = aiEnt->enemy;
	}
	aiEnt->NPC->goalRadius = 64.0f;

	moved = NPC_MoveToGoal(aiEnt, qtrue );
//	navInfo_t	info;
	
	//Get the move info
//	NAV_GetLastMove( info );

	//FIXME: if we bump into another one of our guys and can't get around him, just stop!
	//If we hit our target, then stop and fire!
//	if ( info.flags & NIF_COLLISION ) 
//	{
//		if ( info.blocker == aiEnt->enemy )
//		{
//			SaberDroid_HoldPosition();
//		}
//	}

	//If our move failed, then reset
	/*
	if ( moved == qfalse )
	{//couldn't get to enemy
		//just hang here
		SaberDroid_HoldPosition();
	}
	*/

	return moved;
}

/*
-------------------------
NPC_BSSaberDroid_Patrol
-------------------------
*/

void NPC_BSSaberDroid_Patrol(gentity_t *aiEnt)
{//FIXME: pick up on bodies of dead buddies?
	if ( aiEnt->NPC->confusionTime < level.time )
	{//not confused by mindtrick
		//Look for any enemies
		if ( aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			if ( NPC_CheckPlayerTeamStealth(aiEnt) )
			{//found an enemy
				//aiEnt->NPC->behaviorState = BS_HUNT_AND_KILL;//should be automatic now
				//NPC_AngerSound();
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
		}

		if ( !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
		{//alert reaction behavior.
			//Is there danger nearby
			//[CoOp]
			int alertEvent = NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_SUSPICIOUS );
			//int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_SUSPICIOUS );
			//[/CoOp]
			//There is an event to look at
			if ( alertEvent >= 0 )//&& level.alertEvents[alertEvent].ID != aiEnt->NPC->lastAlertID )
			{
				//aiEnt->NPC->lastAlertID = level.alertEvents[alertEvent].ID;
				if ( level.alertEvents[alertEvent].level >= AEL_DISCOVERED )
				{
					if ( level.alertEvents[alertEvent].owner && 
						level.alertEvents[alertEvent].owner->client && 
						level.alertEvents[alertEvent].owner->health >= 0 &&
						level.alertEvents[alertEvent].owner->client->playerTeam == aiEnt->client->enemyTeam )
					{//an enemy
						G_SetEnemy( aiEnt, level.alertEvents[alertEvent].owner );
						//aiEnt->NPC->enemyLastSeenTime = level.time;
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
	else if ( !aiEnt->client->ps.weaponTime
		&& TIMER_Done( aiEnt, "attackDelay" )
		&& TIMER_Done( aiEnt, "inactiveDelay" ) )
	{//we want to turn off our saber if we need to.
		if ( !aiEnt->client->ps.saberHolstered )
		{//saber is on.
			WP_DeactivateSaber( aiEnt, qfalse );
			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_TURNOFF, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}


/*  I beleive this is used by the saber code for calculating the saber stuff with 
	//saber droids.  Impliment?
int SaberDroid_PowerLevelForSaberAnim( gentity_t *self )
{//determine the strength of the saber attack based on the saber animation.
	switch ( self->client->ps.legsAnim )
	{
	case BOTH_A2_TR_BL:
		if ( self->client->ps.torsoTimer <= 200 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( PM_AnimLength( self->localAnimIndex, (animNumber_t)self->client->ps.legsAnim ) - self->client->ps.torsoTimer < 200 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_2;
		break;
	case BOTH_A1_BL_TR:
		if ( self->client->ps.torsoTimer <= 300 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( PM_AnimLength( self->localAnimIndex, (animNumber_t)self->client->ps.legsAnim ) - self->client->ps.torsoTimer < 200 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_1;
		break;
	case BOTH_A1__L__R:
		if ( self->client->ps.torsoTimer <= 250 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( PM_AnimLength( self->localAnimIndex, (animNumber_t)self->client->ps.legsAnim ) - self->client->ps.torsoTimer < 150 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_1;
		break;
	case BOTH_A3__L__R:
		if ( self->client->ps.torsoTimer <= 200 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if ( PM_AnimLength( self->localAnimIndex, (animNumber_t)self->client->ps.legsAnim ) - self->client->ps.torsoTimer < 300 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	}
	return FORCE_LEVEL_0;
}
*/

/*
-------------------------
NPC_BSSaberDroid_Attack
-------------------------
*/

void NPC_SaberDroid_PickAttack(gentity_t *aiEnt)
{
	int attackAnim = Q_irand( 0, 3 );
	switch ( attackAnim )
	{
	case 0:
	default:
		attackAnim = BOTH_A2_TR_BL;
		aiEnt->client->ps.saberMove = LS_A_TR2BL;
		aiEnt->client->ps.fd.saberAnimLevel = SS_MEDIUM;
		break;
	case 1:
		attackAnim = BOTH_A1_BL_TR;
		aiEnt->client->ps.saberMove = LS_A_BL2TR;
		aiEnt->client->ps.fd.saberAnimLevel = SS_FAST;
		break;
	case 2:
		attackAnim = BOTH_A1__L__R;
		aiEnt->client->ps.saberMove = LS_A_L2R;
		aiEnt->client->ps.fd.saberAnimLevel = SS_FAST;
		break;
	case 3:
		attackAnim = BOTH_A3__L__R;
		aiEnt->client->ps.saberMove = LS_A_L2R;
		aiEnt->client->ps.fd.saberAnimLevel = SS_STRONG;
		break;
	}
	aiEnt->client->ps.saberBlocking = saberMoveData[aiEnt->client->ps.saberMove].blocking;
	/* RAFIXME - since this is saber trail code, I think this needs to be ported to 
	cgame.
	if ( saberMoveData[aiEnt->client->ps.saberMove].trailLength > 0 )
	{
		aiEnt->client->ps.SaberActivateTrail( saberMoveData[aiEnt->client->ps.saberMove].trailLength ); // saber trail lasts for 75ms...feel free to change this if you want it longer or shorter
	}
	else
	{
		aiEnt->client->ps.SaberDeactivateTrail( 0 );
	}
	*/

	NPC_SetAnim( aiEnt, SETANIM_BOTH, attackAnim, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
	aiEnt->client->ps.torsoAnim = aiEnt->client->ps.legsAnim;//need to do this because we have no anim split but saber code checks torsoAnim
	aiEnt->client->ps.weaponTime = aiEnt->client->ps.torsoTimer = aiEnt->client->ps.legsTimer;
	aiEnt->client->ps.weaponstate = WEAPON_FIRING;
}

void NPC_BSSaberDroid_Attack(gentity_t *aiEnt)
{//attack behavior
	//Don't do anything if we're hurt
	if ( aiEnt->painDebounceTime > level.time )
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return;
	}

	//NPC_CheckEnemy( qtrue, qfalse );
	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(aiEnt, qfalse) == qfalse )//!aiEnt->enemy )//
	{
		aiEnt->enemy = NULL;
		NPC_BSSaberDroid_Patrol(aiEnt);//FIXME: or patrol?
		return;
	}

	if ( !aiEnt->enemy )
	{//WTF?  somehow we lost our enemy?
		NPC_BSSaberDroid_Patrol(aiEnt);//FIXME: or patrol?
		return;
	}

	enemyLOS = enemyCS = qfalse;
	move = qtrue;
	faceEnemy = qfalse;
	shoot = qfalse;
	enemyDist = DistanceSquared( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin );

	//can we see our target?
	if ( NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
	{
		aiEnt->NPC->enemyLastSeenTime = level.time;
		enemyLOS = qtrue;

		if ( enemyDist <= 4096 && InFOV3( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->client->ps.viewangles, 90, 45 ) )//within 64 & infront
		{
			VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
			enemyCS = qtrue;
		}
	}
	/*
	else if ( gi.inPVS( aiEnt->enemy->currentOrigin, aiEnt->currentOrigin ) )
	{
		aiEnt->NPC->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
	}
	*/

	if ( enemyLOS )
	{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy = qtrue;
	}

	if ( !TIMER_Done( aiEnt, "taunting" ) )
	{
		move = qfalse;
	}
	else if ( enemyCS )
	{
		shoot = qtrue;
		if ( enemyDist < (aiEnt->r.maxs[0]+aiEnt->enemy->r.maxs[0]+32)*(aiEnt->r.maxs[0]+aiEnt->enemy->r.maxs[0]+32) )
		{//close enough
			move = qfalse;
		}
	}//this should make him chase enemy when out of range...?

	if ( aiEnt->client->ps.legsTimer 
		&& aiEnt->client->ps.legsAnim != BOTH_A3__L__R )//this one is a running attack
	{//in the middle of a held, stationary anim, can't move
		move = qfalse;
	}

	if ( move )
	{//move toward goal
		move = SaberDroid_Move(aiEnt);
		if ( move )
		{//if we had to chase him, be sure to attack as soon as possible
			TIMER_Set( aiEnt, "attackDelay", aiEnt->client->ps.weaponTime );
		}
	}

	if ( !faceEnemy )
	{//we want to face in the dir we're running
		if ( move )
		{//don't run away and shoot
			aiEnt->NPC->desiredYaw = aiEnt->NPC->lastPathAngles[YAW];
			aiEnt->NPC->desiredPitch = 0;
			shoot = qfalse;
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
	}
	else// if ( faceEnemy )
	{//face the enemy
		NPC_FaceEnemy(aiEnt, qtrue);
	}

	if ( aiEnt->NPC->scriptFlags&SCF_DONT_FIRE )
	{
		shoot = qfalse;
	}
	
	//FIXME: need predicted blocking?
	//FIXME: don't shoot right away!
	if ( shoot )
	{//try to shoot if it's time
		if ( TIMER_Done( aiEnt, "attackDelay" ) )
		{	
			if( !(aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
			{//attack!
				NPC_SaberDroid_PickAttack(aiEnt);
				//set attac delay for next attack.
				if ( aiEnt->NPC->rank > RANK_CREWMAN )
				{
					TIMER_Set( aiEnt, "attackDelay", aiEnt->client->ps.weaponTime+Q_irand(0, 1000) );
				}
				else
				{
					TIMER_Set( aiEnt, "attackDelay", aiEnt->client->ps.weaponTime+Q_irand( 0, 1000 )+(Q_irand( 0, 2 )*500) );
				}
			}
		}
	}
}


extern void WP_ActivateSaber( gentity_t *self );
void NPC_BSSD_Default(gentity_t *aiEnt)
{
	if( !aiEnt->enemy )
	{//don't have an enemy, look for one
		NPC_BSSaberDroid_Patrol(aiEnt);
	}
	else//if ( aiEnt->enemy )
	{//have an enemy
		if ( aiEnt->client->ps.saberHolstered == 2 )
		{//turn saber on
			WP_ActivateSaber( aiEnt );
			//aiEnt->client->ps.SaberActivate();
			if ( aiEnt->client->ps.legsAnim == BOTH_TURNOFF
				|| aiEnt->client->ps.legsAnim == BOTH_STAND1 )
			{
				NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_TURNON, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
		}

		NPC_BSSaberDroid_Attack(aiEnt);
		TIMER_Set( aiEnt, "inactiveDelay", Q_irand( 2000, 4000 ) );
	}
	if ( !aiEnt->client->ps.weaponTime )
	{//we're not attacking.
		aiEnt->client->ps.saberMove = LS_READY;
		aiEnt->client->ps.saberBlocking = saberMoveData[LS_READY].blocking;
		//RAFIXME - since this is saber trail code, I think this needs to be ported to cgame.
		//aiEnt->client->ps.SaberDeactivateTrail( 0 );
		aiEnt->client->ps.fd.saberAnimLevel = SS_MEDIUM;
		aiEnt->client->ps.weaponstate = WEAPON_READY;
	}
}
//[/CoOp]
//[/SPPortComplete]
