#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "w_saber.h"

//#define __JEDI_TRACKING__
//#define __EVASION_JUMPING__

extern qboolean NPC_IsCombatPathing(gentity_t *aiEnt);

void JEDI_Debug(gentity_t *aiEnt, char *debugText)
{
	if (aiEnt->s.weapon == WP_SABER) 
	{
		trap->Print(va("JEDI DEBUG: [%s] %s\n", aiEnt->client->pers.netname, debugText));
	}
}

extern int gWPNum;
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

extern vmCvar_t npc_pathing;

#define SABER_ATTACK_RANGE 24

extern void NPC_ClearPathData ( gentity_t *NPC );

extern qboolean BG_SabersOff( playerState_t *ps );

extern qboolean NPC_NeedsHeal ( gentity_t *NPC );
extern int NPC_GetHealthPercent (gentity_t *self, gentity_t *NPC);

extern void CG_DrawAlert( vec3_t origin, float rating );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void ST_Speech( gentity_t *self, int speechType, float failChance );
extern void ForceJump( gentity_t *self, usercmd_t *ucmd );
extern void WP_FireMelee( gentity_t *ent, qboolean alt_fire );
extern qboolean NPC_CombatMoveToGoal( gentity_t *aiEnt, qboolean tryStraight, qboolean retreat );
extern qboolean NPC_CheckFallPositionOK(gentity_t *NPC, vec3_t position);

#define	MAX_VIEW_DIST		2048
#define MAX_VIEW_SPEED		100
#define	JEDI_MAX_LIGHT_INTENSITY 64
#define	JEDI_MIN_LIGHT_THRESHOLD 10
#define	JEDI_MAX_LIGHT_THRESHOLD 50

#define	DISTANCE_SCALE		0.25f
#define	SPEED_SCALE			0.25f
#define	FOV_SCALE			0.5f
#define	LIGHT_SCALE			0.25f

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.3 )

#define	MAX_CHECK_THRESHOLD	1

extern void NPC_ClearLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern qboolean NPC_CheckEnemyStealth( gentity_t *aiEnt);
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

extern gitem_t	*BG_FindItemForAmmo( ammo_t ammo );

extern void ForceThrow( gentity_t *self, qboolean pull );
extern void ForceLightning( gentity_t *self );
extern void ForceHeal( gentity_t *self );
extern void ForceRage( gentity_t *self );
extern void ForceProtect( gentity_t *self );
extern void ForceAbsorb( gentity_t *self );
extern int WP_MissileBlockForBlock( int saberBlock );
extern qboolean G_GetHitLocFromSurfName( gentity_t *ent, const char *surfName, int *hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod );
extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower );
extern qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
extern void WP_DeactivateSaber( gentity_t *self, qboolean clearLength ); //clearLength = qfalse
extern void WP_ActivateSaber( gentity_t *self );

extern qboolean PM_SaberInStart( int move );
extern qboolean BG_SaberInSpecialAttack( int anim );
extern qboolean BG_SaberInAttack( int move );
extern qboolean PM_SaberInBounce( int move );
extern qboolean PM_SaberInParry( int move );
extern qboolean PM_SaberInKnockaway( int move );
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean PM_SaberInDeflect( int move );
extern qboolean BG_SpinningSaberAnim( int anim );
extern qboolean BG_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean BG_InRoll( playerState_t *ps, int anim );
extern qboolean BG_CrouchAnim( int anim );

extern qboolean NPC_SomeoneLookingAtMe(gentity_t *ent);

extern int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd );

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

static void Jedi_Aggression( gentity_t *self, int change );
qboolean Jedi_WaitingAmbush( gentity_t *self );

extern void G_AddPadawanCombatCommentEvent( gentity_t *self, int event, int speakDebounceTime );

extern int bg_parryDebounce[];

int	jediSpeechDebounceTime[FACTION_NUM_FACTIONS];//used to stop several jedi from speaking all at once
//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

qboolean NPC_EnemyAboveMe( gentity_t *NPC )
{
	if (!NPC) return qfalse;
	if (!NPC->enemy || !NPC_IsAlive(NPC, NPC->enemy)) return qfalse;

	if (NPC->enemy->r.currentOrigin[2] > NPC->r.currentOrigin[2] + 64) return qtrue;
	if (NPC->enemy->r.currentOrigin[2] < NPC->r.currentOrigin[2] - 64) return qtrue; // also allow jump down

	return qfalse;
}

qboolean NPC_EnemyAttackingMeWithSaber( gentity_t *NPC )
{
	if (!NPC) return qfalse;
	if (!NPC->enemy || !NPC_IsAlive(NPC, NPC->enemy)) return qfalse;
	if (NPC->s.weapon != WP_SABER || NPC->enemy->s.weapon != WP_SABER) return qfalse;

	if (BG_SaberInAttack(NPC->client->ps.saberMove)) return qtrue;

	return qfalse;
}

void Jedi_CopyAttackCounterInfo(gentity_t *NPC)
{
	// Share attack/counter info with our follower or master...
	if (NPC->parent && NPC_IsAlive(NPC, NPC->parent) && NPC->parent->enemy == NPC->enemy)
	{
		NPC->parent->npc_attack_time = NPC->npc_attack_time;

		if (NPC->parent->npc_attack_time > level.time)
			NPC->parent->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		else
			NPC->parent->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
	}

	if (NPC->padawan && NPC_IsAlive(NPC, NPC->padawan) && NPC->padawan->enemy == NPC->enemy)
	{
		NPC->padawan->npc_attack_time = NPC->npc_attack_time;

		if (NPC->padawan->npc_attack_time > level.time)
			NPC->padawan->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		else
			NPC->padawan->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
	}

	if (NPC->enemy->parent && NPC_IsAlive(NPC, NPC->enemy->parent) && NPC->enemy->parent->enemy == NPC->enemy->enemy)
	{
		NPC->enemy->parent->npc_attack_time = NPC->enemy->npc_attack_time;

		if (NPC->enemy->parent->npc_attack_time > level.time)
			NPC->enemy->parent->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		else
			NPC->enemy->parent->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
	}

	if (NPC->enemy->padawan && NPC_IsAlive(NPC, NPC->enemy->padawan) && NPC->enemy->padawan->enemy == NPC->enemy->enemy)
	{
		NPC->enemy->padawan->npc_attack_time = NPC->enemy->npc_attack_time;

		if (NPC->enemy->padawan->npc_attack_time > level.time)
			NPC->enemy->padawan->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		else
			NPC->enemy->padawan->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
	}
}

qboolean Jedi_AttackOrCounter( gentity_t *NPC )
{
	if (!NPC || !NPC_IsAlive(NPC, NPC)) 
	{
		return qfalse;
	}
	
	if (!NPC->enemy || !NPC_IsAlive(NPC, NPC->enemy))
	{
		NPC->npc_attack_time = 0;
		return qfalse;
	}

	if (NPC->s.weapon != WP_SABER || NPC->enemy->s.weapon != WP_SABER)
	{
		return qfalse;
	}

	//
	// Current attacker always controls all the timers/counters...
	//
	if (NPC->npc_attack_time <= level.time - 1000)
	{// Timer has run out, pick if we should initally attack or defend...
		if (NPC->enemy->s.eType == ET_PLAYER)
		{// When enemy is a player...
			if (irand(0, 1) < 1)
			{
				NPC->npc_attack_time = level.time + 1000;
			}
			else
			{
				NPC->npc_attack_time = level.time;
			}
		}
		else
		{
			if (NPC->enemy->npc_attack_time >= level.time || NPC_EnemyAttackingMeWithSaber(NPC->enemy))
			{// Enemy is already attacking, start by defending...
				NPC->npc_attack_time = level.time;
			}
			else
			{// Enemy is not attacking, start by attacking...
				NPC->npc_attack_time = level.time + 1000;
			}
		}

		Jedi_CopyAttackCounterInfo(NPC);
	}

	//
	// Make sure that we are looking at our enemy, and that they are looking at us (if they are not a player)...
	//

	//NPC_SetLookTarget(NPC, NPC->enemy->s.number, level.time + 100);
	NPC_FaceEntity(NPC, NPC->enemy, qtrue);

	if (NPC->enemy->s.eType != ET_PLAYER)
	{
		//NPC_SetLookTarget(NPC->enemy, NPC->s.number, level.time + 100);
		NPC_FaceEntity(NPC->enemy, NPC, qtrue);
	}

	//
	// Either do attack, or counter action...
	//

	if (NPC->npc_attack_time > level.time)
	{// Attack mode...
		NPC->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		NPC->enemy->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
		return qtrue;
	}
	else
	{// Counter mode...
		NPC->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
		if (NPC->enemy->s.eType != ET_PLAYER) NPC->enemy->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		return qfalse;
	}
}

qboolean NPC_Jedi_EnemyInForceRange ( gentity_t *aiEnt)
{
	if (!aiEnt->enemy || !NPC_IsAlive(aiEnt, aiEnt->enemy)) return qfalse;

	if (Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin) > 256) return qfalse;
	//if (Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin) < 48) return qfalse;

	return qtrue;
}

qboolean NPC_Jedi_EntityInForceRange ( gentity_t *aiEnt, gentity_t *ent )
{
	if (!ent || !NPC_IsAlive(aiEnt, ent)) return qfalse;

	if (Distance(aiEnt->r.currentOrigin, ent->r.currentOrigin) > 256) return qfalse;
	//if (Distance(aiEnt->r.currentOrigin, ent->r.currentOrigin) < 48) return qfalse;

	return qtrue;
}

void NPC_CultistDestroyer_Precache( void )
{//precashe for cultist destroyer
	G_SoundIndex( "sound/movers/objects/green_beam_lp2.wav" );
	G_EffectIndex( "force/destruction_exp" );
}

void NPC_ShadowTrooper_Precache( void )
{
#ifndef __MMO__
	RegisterItem( BG_FindItemForAmmo( AMMO_FORCE ) );
#endif //__MMO__
	G_SoundIndex( "sound/chars/shadowtrooper/cloak.wav" );
	G_SoundIndex( "sound/chars/shadowtrooper/decloak.wav" );
}

void NPC_Rosh_Dark_Precache( void )
{//precashe for Rosh boss
	G_EffectIndex( "force/kothos_recharge.efx" );
	G_EffectIndex( "force/kothos_beam.efx" );
}

void Jedi_ClearTimers( gentity_t *ent )
{
	TIMER_Set( ent, "roamTime", 0 );
	TIMER_Set( ent, "chatter", 0 );
	TIMER_Set( ent, "strafeLeft", 0 );
	TIMER_Set( ent, "strafeRight", 0 );
	TIMER_Set( ent, "noStrafe", 0 );
	TIMER_Set( ent, "walking", 0 );
	TIMER_Set( ent, "taunting", 0 );
	TIMER_Set( ent, "parryTime", 0 );
	TIMER_Set( ent, "parryReCalcTime", 0 );
	TIMER_Set( ent, "forceJumpChasing", 0 );
	TIMER_Set( ent, "jumpChaseDebounce", 0 );
	TIMER_Set( ent, "moveforward", 0 );
	TIMER_Set( ent, "moveback", 0 );
	TIMER_Set( ent, "movenone", 0 );
	TIMER_Set( ent, "moveright", 0 );
	TIMER_Set( ent, "moveleft", 0 );
	TIMER_Set( ent, "movecenter", 0 );
	TIMER_Set( ent, "saberLevelDebounce", 0 );
	TIMER_Set( ent, "noRetreat", 0 );
	TIMER_Set( ent, "holdLightning", 0 );
	TIMER_Set( ent, "gripping", 0 );
	TIMER_Set( ent, "draining", 0 );
	TIMER_Set( ent, "noturn", 0 );
}

void Jedi_PlayBlockedPushSound( gentity_t *self )
{
	if ( self->s.number >= 0 && self->s.number < MAX_CLIENTS )
	{
		if (!NPC_IsJedi(self) && !NPC_IsBountyHunter(self))//NPC_IsStormtrooper(self))
			ST_Speech( self, SPEECH_OUTFLANK, 0.7f );
		else
			G_AddVoiceEvent( self, EV_PUSHFAIL, 3000 );
	}
	else if ( self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time )
	{
		if (!NPC_IsJedi(self) && !NPC_IsBountyHunter(self))//NPC_IsStormtrooper(self))
			ST_Speech( self, SPEECH_OUTFLANK, 0.7f );
		else
			G_AddVoiceEvent( self, EV_PUSHFAIL, 3000 );

		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void Jedi_PlayDeflectSound( gentity_t *self )
{
	if ( self->s.number >= 0 && self->s.number < MAX_CLIENTS )
	{
		if (!NPC_IsJedi(self) && !NPC_IsBountyHunter(self))//NPC_IsStormtrooper(self))
			ST_Speech( self, SPEECH_CHASE, 0.7f );
		else
			G_AddVoiceEvent( self, Q_irand( EV_DEFLECT1, EV_DEFLECT3 ), 3000 );
	}
	else if ( self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time )
	{
		if (!NPC_IsJedi(self) && !NPC_IsBountyHunter(self))//NPC_IsStormtrooper(self))
			ST_Speech( self, SPEECH_CHASE, 0.7f );
		else
			G_AddVoiceEvent( self, Q_irand( EV_DEFLECT1, EV_DEFLECT3 ), 3000 );

		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void NPC_Jedi_PlayConfusionSound( gentity_t *self )
{
	if ( self->health > 0 )
	{
		if ( self->client && ( self->client->NPC_class == CLASS_TAVION || self->client->NPC_class == CLASS_DESANN ) )
		{
			G_AddVoiceEvent( self, Q_irand( EV_CONFUSE1, EV_CONFUSE3 ), 2000 );
		}
		else if ( Q_irand( 0, 1 ) )
		{
			if (!NPC_IsJedi(self) && !NPC_IsBountyHunter(self))//NPC_IsStormtrooper(self))
				ST_Speech( self, SPEECH_ESCAPING, 0 );
			else
				G_AddVoiceEvent( self, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 2000 );
		}
		else
		{
			if (!NPC_IsJedi(self) && !NPC_IsBountyHunter(self))//NPC_IsStormtrooper(self))
				ST_Speech( self, SPEECH_CONFUSED, 0 );
			else
				G_AddVoiceEvent( self, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 2000 );
		}
	}
}

qboolean Jedi_CultistDestroyer( gentity_t *self )
{
	if ( !self || !self->client )
	{
		return qfalse;
	}
	if( self->client->NPC_class == CLASS_REBORN &&
		self->s.weapon == WP_MELEE &&
		!Q_stricmp( "cultist_destroyer", self->NPC_type ) )
	{
		return qtrue;
	}
	return qfalse;
}

void Boba_Precache( void )
{
	G_SoundIndex( "sound/boba/jeton.wav" );
	G_SoundIndex( "sound/boba/jethover.wav" );
	G_SoundIndex( "sound/effects/combustfire.mp3" );
	G_EffectIndex( "boba/jet" );
	G_EffectIndex( "boba/fthrw" );
}

extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );

qboolean Fast_ChangeWeapon( gentity_t *aiEnt, int wp )
{
	if ( aiEnt->s.weapon == wp )
	{
		return qtrue;
	}

	NPC_ChangeWeapon(aiEnt, wp );

	if ( aiEnt->s.weapon != wp )
		return qfalse;

	return qtrue;
}

qboolean Boba_ChangeWeapon( gentity_t *aiEnt, int wp )
{
	if (aiEnt->next_weapon_switch > level.time) return qfalse;

	if ( aiEnt->s.weapon == wp )
	{
		return qtrue;
	}

	NPC_ChangeWeapon(aiEnt, wp );

	if ( aiEnt->s.weapon != wp )
		return qfalse;

	return qtrue;
}

void WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty )
{
	int parts;
	qboolean runningResist = qfalse;

	if ( !self || self->health <= 0 || !self->client || !pusher || !pusher->client )
	{
		return;
	}
	if ( ((self->s.number >= 0 && self->s.number < MAX_CLIENTS) || self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",self->NPC_type) || self->client->NPC_class == CLASS_LUKE)
		&& (VectorLengthSquared( self->client->ps.velocity ) > 10000 || self->client->ps.fd.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3 ) )
	{
		runningResist = qtrue;
	}
	if ( !runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !BG_SpinningSaberAnim( self->client->ps.legsAnim )
		&& !BG_FlippingAnim( self->client->ps.legsAnim )
		&& !PM_RollingAnim( self->client->ps.legsAnim )
		&& !PM_InKnockDown( &self->client->ps )
		&& !BG_CrouchAnim( self->client->ps.legsAnim ))
	{//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{//play resist just in torso
		parts = SETANIM_TORSO;
	}
	NPC_SetAnim( self, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if ( !noPenalty )
	{
		char buf[128];
		float tFVal = 0;

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);

		if ( !runningResist )
		{
			VectorClear( self->client->ps.velocity );
			//still stop them from attacking or moving for a bit, though
			//FIXME: maybe push just a little (like, slide)?
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * tFVal );
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			//self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
			//rwwFIXMEFIXME: Do this?
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * tFVal );
			}
		}
	}
	//play my force push effect on my hand
	self->client->ps.powerups[PW_DISINT_4] = level.time + self->client->ps.torsoTimer + 500;
	self->client->ps.powerups[PW_PULL] = 0;
	Jedi_PlayBlockedPushSound( self );
}

qboolean Boba_StopKnockdown( gentity_t *self, gentity_t *pusher, vec3_t pushDir, qboolean forceKnockdown ) //forceKnockdown = qfalse
{
	vec3_t	pDir, fwd, right, ang;
	float	fDot, rDot;
	int		strafeTime;

	if ( !NPC_IsBountyHunter(self) )
	{
		return qfalse;
	}

	if ( (self->client->ps.eFlags2&EF2_FLYING) )//moveType == MT_FLYSWIM )
	{//can't knock me down when I'm flying
		return qtrue;
	}

	VectorSet(ang, 0, self->r.currentAngles[YAW], 0);
	strafeTime = Q_irand( 1000, 2000 );

	AngleVectors( ang, fwd, right, NULL );
	VectorNormalize2( pushDir, pDir );
	fDot = DotProduct( pDir, fwd );
	rDot = DotProduct( pDir, right );

	if ( Q_irand( 0, 2 ) )
	{//flip or roll with it
		usercmd_t	tempCmd;
		if ( fDot >= 0.4f )
		{
			tempCmd.forwardmove = 127;
			TIMER_Set( self, "moveforward", strafeTime );
		}
		else if ( fDot <= -0.4f )
		{
			tempCmd.forwardmove = -127;
			TIMER_Set( self, "moveback", strafeTime );
		}
		else if ( rDot > 0 )
		{
			tempCmd.rightmove = 127;
			TIMER_Set( self, "strafeRight", strafeTime );
			TIMER_Set( self, "strafeLeft", -1 );
		}
		else
		{
			tempCmd.rightmove = -127;
			TIMER_Set( self, "strafeLeft", strafeTime );
			TIMER_Set( self, "strafeRight", -1 );
		}
		G_AddEvent( self, EV_JUMP, 0 );
		if ( !Q_irand( 0, 1 ) )
		{//flip
			self->client->ps.fd.forceJumpCharge = 280;//FIXME: calc this intelligently?
			ForceJump( self, &tempCmd );
		}
		else
		{//roll
			TIMER_Set( self, "duck", strafeTime );
		}
		self->painDebounceTime = 0;//so we do something
	}
	else if ( !Q_irand( 0, 1 ) && forceKnockdown )
	{//resist
		WP_ResistForcePush( self, pusher, qtrue );
	}
	else
	{//fall down
		return qfalse;
	}

	return qtrue;
}

void Boba_FlyStart( gentity_t *self )
{//switch to seeker AI for a while
#if 0
	if ( TIMER_Done( self, "jetRecharge" ) )
	{
		self->client->ps.gravity = 0;
		if ( self->NPC )
		{
			self->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
		}

		self->client->ps.eFlags2 |= EF2_FLYING;//moveType = MT_FLYSWIM;
		self->client->jetPackTime = level.time + Q_irand( 3000, 10000 );

		//take-off sound
		G_SoundOnEnt( self, CHAN_ITEM, "sound/boba/jeton.wav" );

		//jet loop sound
		self->s.loopSound = G_SoundIndex( "sound/boba/jethover.wav" );

		if ( self->NPC )
		{
			self->count = Q3_INFINITE; // SEEKER shot ammo count
		}
	}
#endif
}

void Boba_FlyStop( gentity_t *self )
{
#if 0
	self->client->ps.gravity = g_gravity.value;

	if ( self->NPC )
	{
		self->NPC->aiFlags &= ~NPCAI_CUSTOM_GRAVITY;
	}

	self->client->ps.eFlags2 &= ~EF2_FLYING;
	self->client->jetPackTime = 0;

	//stop jet loop sound
	self->s.loopSound = 0;

	if ( self->NPC )
	{
		self->count = 0; // SEEKER shot ammo count
		TIMER_Set( self, "jetRecharge", Q_irand( 1000, 5000 ) );
		TIMER_Set( self, "jumpChaseDebounce", Q_irand( 500, 2000 ) );
	}

	self->client->ps.eFlags &= ~EF_JETPACK_ACTIVE;
	self->client->ps.eFlags &= ~EF_JETPACK_FLAMING;
	self->client->ps.eFlags &= ~EF_JETPACK_HOVER;
	self->s.eFlags &= ~EF_JETPACK_ACTIVE;
	self->s.eFlags &= ~EF_JETPACK_FLAMING;
	self->s.eFlags &= ~EF_JETPACK_HOVER;
	self->client->ps.pm_type = PM_NORMAL;
#endif
}

qboolean Boba_Flying( gentity_t *self )
{
#if 0
	return ((qboolean)(self->client->ps.eFlags2&EF2_FLYING));//moveType==MT_FLYSWIM));
#else
	return qfalse;
#endif
}

void Boba_FireFlameThrower( gentity_t *self )
{
	int		damage	= Q_irand( 20, 30 );
	trace_t		tr;
	gentity_t	*traceEnt = NULL;
	mdxaBone_t	boltMatrix;
	vec3_t		start, end, dir, traceMins = {-4, -4, -4}, traceMaxs = {4, 4, 4};

	trap->G2API_GetBoltMatrix( self->ghoul2, 0, self->client->renderInfo.handLBolt,
			&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
			NULL, self->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, start );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );
	//G_PlayEffect( "boba/fthrw", start, dir );
	VectorMA( start, 128, dir, end );

	trap->Trace( &tr, start, traceMins, traceMaxs, end, self->s.number, MASK_SHOT, qfalse, 0, 0 );

	traceEnt = &g_entities[tr.entityNum];
	if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage && traceEnt != self)
	{
		G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_LAVA );
		//rwwFIXMEFIXME: add DAMAGE_NO_HIT_LOC?
	}
}

void Boba_StartFlameThrower( gentity_t *self )
{
	int	flameTime = 4000;//Q_irand( 1000, 3000 );
	mdxaBone_t	boltMatrix;
	vec3_t		org, dir;

	self->client->ps.torsoTimer = flameTime;//+1000;
	if ( self->NPC )
	{
		TIMER_Set( self, "nextAttackDelay", flameTime );
		TIMER_Set( self, "walking", 0 );
	}
	TIMER_Set( self, "flameTime", flameTime );
	/*
	gentity_t *fire = G_Spawn();
	if ( fire != NULL )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir, ang;
		trap->G2API_GetBoltMatrix( NPC->ghoul2, NPC->playerModel, NPC->handRBolt,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, (cg.time?cg.time:level.time),
				NULL, NPC->s.modelScale );

		trap->G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
		trap->G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );
		vectoangles( dir, ang );

		VectorCopy( org, fire->s.origin );
		VectorCopy( ang, fire->s.angles );

		fire->targetname = "bobafire";
		SP_fx_explosion_trail( fire );
		fire->damage = 1;
		fire->radius = 10;
		fire->speed = 200;
		fire->fxID = G_EffectIndex( "boba/fthrw" );//"env/exp_fire_trail" );//"env/small_fire"
		fx_explosion_trail_link( fire );
		fx_explosion_trail_use( fire, NPC, NPC );

	}
	*/
	G_SoundOnEnt( self, CHAN_WEAPON, "sound/effects/combustfire.mp3" );

	trap->G2API_GetBoltMatrix(self->ghoul2, 0, self->client->renderInfo.handRBolt, &boltMatrix, self->r.currentAngles,
		self->r.currentOrigin, level.time, NULL, self->modelScale);

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );

	G_PlayEffectID( G_EffectIndex("boba/fthrw"), org, dir);
}

void Boba_DoFlameThrower( gentity_t *self )
{
	NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if ( TIMER_Done( self, "nextAttackDelay" ) && TIMER_Done( self, "flameTime" ) )
	{
		Boba_StartFlameThrower( self );
	}
	Boba_FireFlameThrower( self );
}

void Boba_FireDecide( gentity_t *aiEnt)
{
	qboolean	enemyLOS = qfalse, enemyCS = qfalse, enemyInFOV = qfalse;
	//qboolean	move = qtrue;
	qboolean	shoot = qfalse, hitAlly = qfalse;
	vec3_t		impactPos, enemyDir, shootDir;
	float		enemyDist, dot;
	qboolean	ENEMY_IS_BREAKABLE = qfalse;

	if ((NPC_IsBountyHunter(aiEnt) || aiEnt->hasJetpack)
		&& aiEnt->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& aiEnt->client->ps.fd.forceJumpZStart
		&& !BG_FlippingAnim( aiEnt->client->ps.legsAnim )
		&& !Q_irand( 0, 4 ) )
	{//take off
		Boba_FlyStart( aiEnt );
	}

	if ( !aiEnt->enemy )
	{
		return;
	}

	ENEMY_IS_BREAKABLE = NPC_EntityIsBreakable(aiEnt, aiEnt->enemy);

	if (!CanShoot (aiEnt->enemy, aiEnt ))
	{// UQ1: Umm, how about we actually check if we can hit them first???
		return;
	}

	if ( !ENEMY_IS_BREAKABLE
		&& Distance(aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin) <= 64 
		&& aiEnt->client->ps.weapon != WP_SABER//!NPC_IsJedi(aiEnt) 
		&& aiEnt->client->NPC_class != CLASS_BOBAFETT // BOBA kicks like a jedi now...
		/*&& !(aiEnt->client->NPC_class == CLASS_BOBAFETT && (!TIMER_Done( aiEnt, "nextAttackDelay" ) || !TIMER_Done( aiEnt, "flameTime" )))*/)
	{// Close range - switch to melee... TODO: Make jedi/sith kick...
		if (aiEnt->next_rifle_butt_time > level.time)
		{// Wait for anim to play out...
			return;
		}
		else
		{// Hit them again...
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_MELEE2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD ); // UQ1: Better anim?????
			WP_FireMelee(aiEnt, qfalse);
			aiEnt->next_rifle_butt_time = level.time + 10000;
			return;
		}
	}
	else if ( ENEMY_IS_BREAKABLE
		&& Distance(aiEnt->enemy->breakableOrigin, aiEnt->r.currentOrigin) <= 64 
		&& aiEnt->client->ps.weapon != WP_SABER//!NPC_IsJedi(aiEnt) 
		&& aiEnt->client->NPC_class != CLASS_BOBAFETT // BOBA kicks like a jedi now...
		/*&& !(aiEnt->client->NPC_class == CLASS_BOBAFETT && (!TIMER_Done( aiEnt, "nextAttackDelay" ) || !TIMER_Done( aiEnt, "flameTime" )))*/)
	{// Close range - switch to melee... TODO: Make jedi/sith kick...
		if (aiEnt->next_rifle_butt_time > level.time)
		{// Wait for anim to play out...
			return;
		}
		else
		{// Hit them again...
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_MELEE2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD ); // UQ1: Better anim?????
			WP_FireMelee(aiEnt, qfalse);
			aiEnt->next_rifle_butt_time = level.time + 10000;
			return;
		}
	}
	else if (aiEnt->client->ps.weapon == WP_MODULIZED_WEAPON)
	{
		if ( aiEnt->health < aiEnt->client->pers.maxHealth*0.5f || ENEMY_IS_BREAKABLE )
		{
			aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;

			aiEnt->NPC->burstMin = 1;
			aiEnt->NPC->burstMean = 3;
			aiEnt->NPC->burstMax = 5;
			aiEnt->NPC->burstSpacing = Q_irand( 300, 750 );//attack debounce
		}
		else
		{
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
		}
	}
	else if (aiEnt->client->ps.weapon == WP_MODULIZED_WEAPON)
	{
		if ( aiEnt->health < aiEnt->client->pers.maxHealth*0.5f || NPC_IsJedi(aiEnt) || aiEnt->s.eType == ET_PLAYER || ENEMY_IS_BREAKABLE )
		{
			aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;

			aiEnt->NPC->burstMin = 3;
			aiEnt->NPC->burstMean = 12;
			aiEnt->NPC->burstMax = 20;
			aiEnt->NPC->burstSpacing = Q_irand( 300, 750 );//attack debounce
		}
		else
		{
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
		}
	}
	else if (aiEnt->client->ps.weapon != WP_SABER && ENEMY_IS_BREAKABLE)
	{// Breakables...
		aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;

		aiEnt->NPC->burstMin = 3;
		aiEnt->NPC->burstMean = 12;
		aiEnt->NPC->burstMax = 20;
		aiEnt->NPC->burstSpacing = Q_irand( 300, 750 );//attack debounce
	}
	else if (aiEnt->client->ps.weapon != WP_SABER)
	{// UQ1: How about I add a default for them??? :)
		if ( aiEnt->health < aiEnt->client->pers.maxHealth*0.5f || NPC_IsJedi(aiEnt) || aiEnt->s.eType == ET_PLAYER || ENEMY_IS_BREAKABLE )
		{
			aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;

			aiEnt->NPC->burstMin = 3;
			aiEnt->NPC->burstMean = 12;
			aiEnt->NPC->burstMax = 20;
			aiEnt->NPC->burstSpacing = Q_irand( 300, 750 );//attack debounce
		}
		else
		{
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
		}
	}
	

	VectorClear( impactPos );

	if (ENEMY_IS_BREAKABLE)
	{
		enemyDist = DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->breakableOrigin );
		VectorSubtract( aiEnt->enemy->breakableOrigin, aiEnt->r.currentOrigin, enemyDir );
	}
	else
	{
		enemyDist = DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin );
		VectorSubtract( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, enemyDir );
	}

	VectorNormalize( enemyDir );
	AngleVectors( aiEnt->client->ps.viewangles, shootDir, NULL, NULL );
	dot = DotProduct( enemyDir, shootDir );

	if ( dot > 0.5f ||( enemyDist * (1.0f-dot)) < 10000 )
	{//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if ( !ENEMY_IS_BREAKABLE
		&& NPC_IsBountyHunter(aiEnt)
		&& (enemyDist < (128*128) && enemyInFOV) || !TIMER_Done( aiEnt, "flameTime" ) )
	{//flamethrower
		Boba_DoFlameThrower( aiEnt );
		enemyCS = qfalse;
		shoot = qfalse;
		aiEnt->NPC->enemyLastSeenTime = level.time;
		aiEnt->client->pers.cmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
	}
	else if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin) >= 512.0)
	{//sniping... should be assumed
		if ( !(aiEnt->NPC->scriptFlags&SCF_ALT_FIRE) )
		{//use primary fire
			aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;
			//reset fire-timing variables
			NPC_ChangeWeapon(aiEnt, aiEnt->client->ps.weapon );
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return;
		}
	}
	else if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin) >= 512.0)
	{//sniper rifle, but too close...
		if ( aiEnt->NPC->scriptFlags&SCF_ALT_FIRE )
		{//use primary fire
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
			//reset fire-timing variables
			NPC_ChangeWeapon(aiEnt, aiEnt->client->ps.weapon );
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return;
		}
	}

	//can we see our target?
	if ( TIMER_Done( aiEnt, "nextAttackDelay" ) && TIMER_Done( aiEnt, "flameTime" ) )
	{
		if ( NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
		{
			aiEnt->NPC->enemyLastSeenTime = level.time;
			enemyLOS = qtrue;

			if ( aiEnt->client->ps.weapon == WP_NONE )
			{
				enemyCS = qfalse;//not true, but should stop us from firing
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
						enemyCS = qtrue;
						//NPC_AimAdjust( aiEnt, 2 );//adjust aim better longer we have clear shot at enemy
						VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
					}
					else
					{//Hmm, have to get around this bastard
						//NPC_AimAdjust( aiEnt, 1 );//adjust aim better longer we can see enemy
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
		else if ( !ENEMY_IS_BREAKABLE && trap->InPVS( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin ) )
		{
			aiEnt->NPC->enemyLastSeenTime = level.time;
			//NPC_AimAdjust( aiEnt, -1 );//adjust aim worse longer we cannot see enemy
		}
		else if (ENEMY_IS_BREAKABLE && trap->InPVS( aiEnt->enemy->breakableOrigin, aiEnt->r.currentOrigin ) )
		{
			aiEnt->NPC->enemyLastSeenTime = level.time;
			//NPC_AimAdjust( aiEnt, -1 );//adjust aim worse longer we cannot see enemy
		}

		if ( aiEnt->client->ps.weapon == WP_NONE )
		{
			shoot = qfalse;
		}
		else
		{
			if ( enemyCS )
			{
				shoot = qtrue;
			}
		}

		if ( !enemyCS )
		{//if have a clear shot, always try
			//See if we should continue to fire on their last position
			//!TIMER_Done( NPC, "stick" ) ||
			if ( !hitAlly //we're not going to hit an ally
				&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
				&& aiEnt->NPC->enemyLastSeenTime > 0 )//we've seen the enemy
			{
				if ( level.time - aiEnt->NPC->enemyLastSeenTime < 10000 )//we have seem the enemy in the last 10 seconds
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
							case WP_THERMAL:
							case WP_TRIP_MINE:
							case WP_DET_PACK:
								distThreshold = 262144/*512*512*/;
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
						}
					}
				}
			}
		}

		//FIXME: don't shoot right away!
		/*if ( aiEnt->client->ps.weaponTime > 0 )
		{
			if ( aiEnt->s.weapon == WP_ROCKET_LAUNCHER )
			{
				if ( !enemyLOS || !enemyCS )
				{//cancel it
					aiEnt->client->ps.weaponTime = 0;
				}
				else
				{//delay our next attempt
					TIMER_Set( aiEnt, "nextAttackDelay", Q_irand( 500, 1000 ) );
				}
			}
		}
		else*/ if ( shoot )
		{//try to shoot if it's time
			if ( TIMER_Done( aiEnt, "nextAttackDelay" ) )
			{
				if( !(aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
				{
					WeaponThink(aiEnt, qtrue );
				}
				/*
				if ( aiEnt->s.weapon == WP_ROCKET_LAUNCHER
					&& (aiEnt->client->pers.cmd.buttons&BUTTON_ATTACK)
					&& !Q_irand( 0, 3 ) )
				{//every now and then, shoot a homing rocket
					aiEnt->client->pers.cmd.buttons &= ~BUTTON_ATTACK;
					aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
					aiEnt->client->ps.weaponTime = Q_irand( 500, 1500 );
				}
				*/
			}
		}
	}
}

void Jedi_Cloak( gentity_t *self )
{
	if ( self )
	{
		self->flags |= FL_NOTARGET;
		if ( self->client )
		{
			if ( !self->client->ps.powerups[PW_CLOAKED] )
			{//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;

				//FIXME: debounce attacks?
				//FIXME: temp sound
				G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/cloak.wav") );
			}
		}
	}
}

void Jedi_Decloak( gentity_t *self )
{
	if ( self )
	{
		self->flags &= ~FL_NOTARGET;
		if ( self->client )
		{
			if ( self->client->ps.powerups[PW_CLOAKED] )
			{//Uncloak
				self->client->ps.powerups[PW_CLOAKED] = 0;

				G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/decloak.wav") );
			}
		}
	}
}

void Jedi_CheckCloak( gentity_t *aiEnt)
{
	if ( aiEnt && aiEnt->client && aiEnt->client->NPC_class == CLASS_SHADOWTROOPER )
	{
		if ( !aiEnt->client->ps.saberHolstered ||
			aiEnt->health <= 0 ||
			aiEnt->client->ps.saberInFlight ||
		//	(NPC->client->ps.eFlags&EF_FORCE_GRIPPED) ||
		//	(NPC->client->ps.eFlags&EF_FORCE_DRAINED) ||
			aiEnt->painDebounceTime > level.time )
		{//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			Jedi_Decloak( aiEnt );
		}
		else if ( aiEnt->health > 0
			&& !aiEnt->client->ps.saberInFlight
		//	&& !(NPC->client->ps.eFlags&EF_FORCE_GRIPPED)
		//	&& !(NPC->client->ps.eFlags&EF_FORCE_DRAINED)
			&& aiEnt->painDebounceTime < level.time )
		{//still alive, have saber in hand, not taking pain and not being gripped
			Jedi_Cloak( aiEnt );
		}
	}
}
/*
==========================================================================================
AGGRESSION
==========================================================================================
*/
static void Jedi_Aggression( gentity_t *self, int change )
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
		if ( self->client->NPC_class == CLASS_DESANN )
		{
			upper_threshold = 20;
			lower_threshold = 5;
		}
		else
		{
			upper_threshold = 10;
			lower_threshold = 3;
		}
	}

	if ( self->NPC->stats.aggression > upper_threshold )
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if ( self->NPC->stats.aggression < lower_threshold )
	{
		self->NPC->stats.aggression = lower_threshold;
	}
	//Com_Printf( "(%d) %s agg %d change: %d\n", level.time, self->NPC_type, self->NPC->stats.aggression, change );
}

static void Jedi_AggressionErosion( gentity_t *aiEnt, int amt )
{
	if ( TIMER_Done( aiEnt, "roamTime" ) )
	{//the longer we're not alerted and have no enemy, the more our aggression goes down
		TIMER_Set( aiEnt, "roamTime", Q_irand( 2000, 5000 ) );
		Jedi_Aggression( aiEnt, amt );
	}

	if ( aiEnt->NPC->stats.aggression < 4 || (aiEnt->NPC->stats.aggression < 6&&aiEnt->client->NPC_class == CLASS_DESANN))
	{//turn off the saber
		WP_DeactivateSaber( aiEnt, qfalse );
	}
}

void NPC_Jedi_RateNewEnemy( gentity_t *self, gentity_t *enemy )
{
	float healthAggression;
	float weaponAggression;
	int newAggression;

	switch( enemy->s.weapon )
	{
	case WP_SABER:
		healthAggression = (float)self->health/200.0f*6.0f;
		weaponAggression = 7;//go after him
		break;
	case WP_MODULIZED_WEAPON:
		if ( DistanceSquared( self->r.currentOrigin, enemy->r.currentOrigin ) < 65536 )//256 squared
		{
			healthAggression = (float)self->health/200.0f*8.0f;
			weaponAggression = 8;//go after him
		}
		else
		{
			healthAggression = 8.0f - ((float)self->health/200.0f*8.0f);
			weaponAggression = 2;//hang back for a second
		}
		break;
	default:
		healthAggression = (float)self->health/200.0f*8.0f;
		weaponAggression = 6;//approach
		break;
	}
	//Average these with current aggression
	newAggression = ceil( (healthAggression + weaponAggression + (float)self->NPC->stats.aggression )/3.0f);
	//Com_Printf( "(%d) new agg %d - new enemy\n", level.time, newAggression );
	Jedi_Aggression( self, newAggression - self->NPC->stats.aggression );

	//don't taunt right away
	TIMER_Set( self, "chatter", Q_irand( 4000, 7000 ) );
}

static void Jedi_Rage( gentity_t *aiEnt)
{
	Jedi_Aggression( aiEnt, 10 - aiEnt->NPC->stats.aggression + Q_irand( -2, 2 ) );
	TIMER_Set( aiEnt, "roamTime", 0 );
	TIMER_Set( aiEnt, "chatter", 0 );
	TIMER_Set( aiEnt, "walking", 0 );
	TIMER_Set( aiEnt, "taunting", 0 );
	TIMER_Set( aiEnt, "jumpChaseDebounce", 0 );
	TIMER_Set( aiEnt, "movenone", 0 );
	TIMER_Set( aiEnt, "movecenter", 0 );
	TIMER_Set( aiEnt, "noturn", 0 );
	ForceRage( aiEnt );
}

void Jedi_RageStop( gentity_t *self )
{
	if ( self->NPC )
	{//calm down and back off
		TIMER_Set( self, "roamTime", 0 );
		Jedi_Aggression( self, Q_irand( -5, 0 ) );
	}
}
/*
==========================================================================================
SPEAKING
==========================================================================================
*/

static qboolean Jedi_BattleTaunt( gentity_t *aiEnt)
{
	if ( TIMER_Done( aiEnt, "chatter" )
		&& !Q_irand( 0, 3 )
		&& aiEnt->NPC->blockedSpeechDebounceTime < level.time
		&& jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time )
	{
		int event = -1;
		if ( aiEnt->client->playerTeam == NPCTEAM_PLAYER
			&& aiEnt->enemy && aiEnt->enemy->client && (aiEnt->enemy->client->NPC_class == CLASS_JEDI || aiEnt->enemy->client->NPC_class == CLASS_PADAWAN || aiEnt->enemy->client->NPC_class == CLASS_HK51) )
		{//a jedi fighting a jedi - training
			if ( (aiEnt->client->NPC_class == CLASS_JEDI || aiEnt->client->NPC_class == CLASS_PADAWAN || aiEnt->client->NPC_class == CLASS_HK51) && aiEnt->NPC->rank == RANK_COMMANDER )
			{//only trainer taunts
				event = EV_TAUNT1;
			}
		}
		else
		{//reborn or a jedi fighting an enemy
			event = Q_irand( EV_TAUNT1, EV_TAUNT3 );
		}
		if ( event != -1 )
		{
			if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))//NPC_IsStormtrooper(aiEnt))
				ST_Speech( aiEnt, SPEECH_OUTFLANK, 0 );
			else
				G_AddVoiceEvent( aiEnt, event, 3000 );

			jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
			TIMER_Set( aiEnt, "chatter", Q_irand( 10000, 15000 ) );

			if ( aiEnt->enemy && aiEnt->enemy->NPC 
				&& aiEnt->enemy->s.weapon == WP_SABER 
				&& aiEnt->enemy->client 
				&& (aiEnt->enemy->client->NPC_class == CLASS_JEDI || aiEnt->enemy->client->NPC_class == CLASS_PADAWAN) )
			{//Have the enemy jedi say something in response when I'm done?
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
==========================================================================================
MOVEMENT
==========================================================================================
*/
static qboolean Jedi_ClearPathToSpot( gentity_t *aiEnt, vec3_t dest, int impactEntNum )
{
	trace_t	trace;
	vec3_t	mins, start, end, dir;
	float	dist, drop, i;

	//Offset the step height
	VectorSet( mins, aiEnt->r.mins[0], aiEnt->r.mins[1], aiEnt->r.mins[2] + STEPSIZE );

	trap->Trace( &trace, aiEnt->r.currentOrigin, mins, aiEnt->r.maxs, dest, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		return qfalse;
	}

	if ( trace.fraction < 1.0f )
	{//hit something
		if ( impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum )
		{//hit what we're going after
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	//otherwise, clear path in a straight line.
	//Now at intervals of my size, go along the trace and trace down STEPSIZE to make sure there is a solid floor.
	VectorSubtract( dest, aiEnt->r.currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dest[2] > aiEnt->r.currentOrigin[2] )
	{//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{//going down or level, check for moderate drops
		drop = 64;
	}
	for ( i = aiEnt->r.maxs[0]*2; i < dist; i += aiEnt->r.maxs[0]*2 )
	{//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA( aiEnt->r.currentOrigin, i, dir, start );
		VectorCopy( start, end );
		end[2] -= drop;
		trap->Trace( &trace, start, mins, aiEnt->r.maxs, end, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );//NPC->r.mins?
		if ( trace.fraction < 1.0f || trace.allsolid || trace.startsolid )
		{//good to go
			continue;
		}
		//no floor here! (or a long drop?)
		return qfalse;
	}
	//we made it!
	return qtrue;
}

qboolean NPC_MoveDirClear( gentity_t *aiEnt, int forwardmove, int rightmove, qboolean reset )
{
	vec3_t	forward, right, testPos, angles, mins;
	trace_t	trace;
	float	fwdDist, rtDist;
	float	bottom_max = -STEPSIZE*4 - 1;

	if ( !forwardmove && !rightmove )
	{//not even moving
		//Com_Printf( "%d skipping walk-cliff check (not moving)\n", level.time );
		return qtrue;
	}

	if ( aiEnt->client->pers.cmd.upmove > 0 || aiEnt->client->ps.fd.forceJumpCharge )
	{//Going to jump
		//Com_Printf( "%d skipping walk-cliff check (going to jump)\n", level.time );
		return qtrue;
	}

	if ( aiEnt->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in the air
		//Com_Printf( "%d skipping walk-cliff check (in air)\n", level.time );
		return qtrue;
	}
	/*
	if ( fabs( AngleDelta( NPC->r.currentAngles[YAW], NPCInfo->desiredYaw ) ) < 5.0 )//!ucmd.angles[YAW] )
	{//Not turning much, don't do this
		//NOTE: Should this not happen only if you're not turning AT ALL?
		//	You could be turning slowly but moving fast, so that would
		//	still let you walk right off a cliff...
		//NOTE: Or maybe it is a good idea to ALWAYS do this, regardless
		//	of whether ot not we're turning?  But why would we be walking
		//  straight into a wall or off	a cliff unless we really wanted to?
		return;
	}
	*/

	//FIXME: to really do this right, we'd have to actually do a pmove to predict where we're
	//going to be... maybe this should be a flag and pmove handles it and sets a flag so AI knows
	//NEXT frame?  Or just incorporate current velocity, runspeed and possibly friction?
	VectorCopy( aiEnt->r.mins, mins );
	mins[2] += STEPSIZE;
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = aiEnt->client->ps.viewangles[YAW];//Add ucmd.angles[YAW]?
	AngleVectors( angles, forward, right, NULL );
	fwdDist = ((float)forwardmove)/2.0f;
	rtDist = ((float)rightmove)/2.0f;
	VectorMA( aiEnt->r.currentOrigin, fwdDist, forward, testPos );
	VectorMA( testPos, rtDist, right, testPos );
	trap->Trace( &trace, aiEnt->r.currentOrigin, mins, aiEnt->r.maxs, testPos, aiEnt->s.number, aiEnt->clipmask|CONTENTS_BOTCLIP, qfalse, 0, 0 );
	if ( trace.allsolid || trace.startsolid )
	{//hmm, trace started inside this brush... how do we decide if we should continue?
		//FIXME: what do we do if we start INSIDE a CONTENTS_BOTCLIP? Try the trace again without that in the clipmask?
		if ( reset )
		{
			trace.fraction = 1.0f;
		}
		VectorCopy( testPos, trace.endpos );
		//return qtrue;
	}
	if ( trace.fraction < 0.6 )
	{//Going to bump into something very close, don't move, just turn
		if ( (aiEnt->enemy && trace.entityNum == aiEnt->enemy->s.number) || (aiEnt->NPC->goalEntity && trace.entityNum == aiEnt->NPC->goalEntity->s.number) )
		{//okay to bump into enemy or goal
			//Com_Printf( "%d bump into enemy/goal okay\n", level.time );
			return qtrue;
		}
		else if ( reset )
		{//actually want to screw with the ucmd
			//Com_Printf( "%d avoiding walk into wall (entnum %d)\n", level.time, trace.entityNum );
			aiEnt->client->pers.cmd.forwardmove = 0;
			aiEnt->client->pers.cmd.rightmove = 0;
			VectorClear( aiEnt->client->ps.moveDir );
		}
		return qfalse;
	}

	if ( aiEnt->NPC->goalEntity )
	{
		qboolean enemy_in_air = qfalse;

		if (aiEnt->NPC->goalEntity->client)
		{
			if (aiEnt->NPC->goalEntity->client->ps.groundEntityNum == ENTITYNUM_NONE)
				enemy_in_air = qtrue;
		}

		if ( aiEnt->NPC->goalEntity->r.currentOrigin[2] < aiEnt->r.currentOrigin[2] )
		{//goal is below me, okay to step off at least that far plus stepheight
			if (!enemy_in_air)
				bottom_max += aiEnt->NPC->goalEntity->r.currentOrigin[2] - aiEnt->r.currentOrigin[2];
		}
	}
	VectorCopy( trace.endpos, testPos );
	testPos[2] += bottom_max;

	trap->Trace( &trace, trace.endpos, mins, aiEnt->r.maxs, testPos, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );

	//FIXME:Should we try to see if we can still get to our goal using the waypoint network from this trace.endpos?
	//OR: just put NPC clip brushes on these edges (still fall through when die)

	if ( trace.allsolid || trace.startsolid )
	{//Not going off a cliff
		//Com_Printf( "%d walk off cliff okay (droptrace in solid)\n", level.time );
		return qtrue;
	}

	if ( trace.fraction < 1.0 )
	{//Not going off a cliff
		//FIXME: what if plane.normal is sloped?  We'll slide off, not land... plus this doesn't account for slide-movement...
		//Com_Printf( "%d walk off cliff okay will hit entnum %d at dropdist of %4.2f\n", level.time, trace.entityNum, (trace.fraction*bottom_max) );
		return qtrue;
	}

	//going to fall at least bottom_max, don't move, just turn... is this bad, though?  What if we want them to drop off?
	if ( reset )
	{//actually want to screw with the ucmd
		//Com_Printf( "%d avoiding walk off cliff\n", level.time );
		aiEnt->client->pers.cmd.forwardmove *= -1.0;//= 0;
		aiEnt->client->pers.cmd.rightmove *= -1.0;//= 0;
		VectorScale( aiEnt->client->ps.moveDir, -1, aiEnt->client->ps.moveDir );
	}
	return qfalse;
}
/*
-------------------------
Jedi_HoldPosition
-------------------------
*/

static void Jedi_HoldPosition( gentity_t *aiEnt)
{
	//NPCInfo->squadState = SQUAD_STAND_AND_SHOOT;
	aiEnt->NPC->goalEntity = NULL;

	/*
	if ( TIMER_Done( NPC, "stand" ) )
	{
		TIMER_Set( NPC, "duck", Q_irand( 2000, 4000 ) );
	}
	*/
}

/*
-------------------------
Jedi_Move
-------------------------
*/

void Jedi_Move( gentity_t *aiEnt, gentity_t *goal, qboolean retreat )
{
	qboolean	moved;
	navInfo_t	info;

	//if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt)) 
	//{
	//	// UQ1: We don't want to use jedi movement for normal gunners... They get too close all the time...
	//	return;
	//}

	aiEnt->NPC->combatMove = qtrue;
	aiEnt->NPC->goalEntity = goal;

	moved = NPC_CombatMoveToGoal(aiEnt, qtrue, retreat );

#if 0
	//FIXME: temp retreat behavior- should really make this toward a safe spot or maybe to outflank enemy
	if ( retreat )
	{//FIXME: should we trace and make sure we can go this way?  Or somehow let NPC_CombatMoveToGoal know we want to retreat and have it handle it?
		aiEnt->client->pers.cmd.forwardmove *= -1;
		aiEnt->client->pers.cmd.rightmove *= -1;
		VectorScale( aiEnt->client->ps.moveDir, -1, aiEnt->client->ps.moveDir );
	}
#endif

	//Get the move info
	NAV_GetLastMove( &info );

	//If we hit our target, then stop and fire!
	if ( ( info.flags & NIF_COLLISION ) && ( info.blocker == aiEnt->enemy ) )
	{
		Jedi_HoldPosition(aiEnt);
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{
		Jedi_HoldPosition(aiEnt);
	}
}

static qboolean Jedi_Hunt( gentity_t *aiEnt)
{
	if (!NPC_IsJedi(aiEnt) && aiEnt->client->ps.weapon != WP_SABER) return qfalse; // UQ1: Non-Jedi do not hunt this way...

	//Com_Printf( "Hunting\n" );
	//if we're at all interested in fighting, go after him
	if ( aiEnt->NPC->stats.aggression > 1 )
	{//approach enemy
		aiEnt->NPC->combatMove = qtrue;
		if ( !(aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )
		{
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return qtrue;
		}
		else
		{
			if ( aiEnt->NPC->goalEntity == NULL )
			{//hunt
				aiEnt->NPC->goalEntity = aiEnt->enemy;
			}
			//Jedi_Move( NPC->enemy, qfalse );
			if ( NPC_CombatMoveToGoal(aiEnt, qfalse, qfalse ) )
			{
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return qtrue;
			}
		}
	}
	return qfalse;
}

static qboolean Jedi_Track( gentity_t *aiEnt)
{
	if (!NPC_IsJedi(aiEnt) && aiEnt->client->ps.weapon != WP_SABER) return qfalse; // UQ1: Non-Jedi do not track this way...

	//if we're at all interested in fighting, go after him
	if ( aiEnt->NPC->stats.aggression > 1 )
	{//approach enemy
		aiEnt->NPC->combatMove = qtrue;
		NPC_SetMoveGoal( aiEnt, aiEnt->NPC->enemyLastSeenLocation, 16, qtrue, 0, aiEnt->enemy );
		if ( NPC_CombatMoveToGoal(aiEnt, qfalse, qfalse ) )
		{
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return qtrue;
		}
	}
	return qfalse;
}

//#define __DEBUG_COMBAT_POSITIONS__

#define COMBAT_POSIITON_SELECTION_RANGE 256.0
#define COMBAT_POSIITON_ALLOW_RANGE 2048.0

qboolean Jedi_AdvanceRetreatGoalInvalid(gentity_t *aiEnt)
{
	if (NPC_IsCombatPathing(aiEnt))
	{
		float distToGoal = Distance(aiEnt->client->navigation.goal.origin, aiEnt->enemy->r.currentOrigin);

		if (VectorLength(aiEnt->client->navigation.goal.origin) == 0 || distToGoal > COMBAT_POSIITON_ALLOW_RANGE)
		{
			return qtrue;
		}

		if (GoalInRange(aiEnt, NavlibGetGoalRadius(aiEnt)))
		{
			return qtrue;
		}

		return qfalse;
	}

	return qtrue;
}

void Jedi_Retreat(gentity_t *aiEnt)
{
	if (!TIMER_Done(aiEnt, "noRetreat"))
	{//don't actually move
		return;
	}
	//FIXME: when retreating, we should probably see if we can retreat
	//in the direction we want.  If not...?  Evade?
	//Com_Printf( "Retreating\n" );

#ifdef __USE_NAVLIB__
	if (!aiEnt->enemy || !NPC_ValidEnemy(aiEnt, aiEnt->enemy))
	{// No enemy, so use original JKA movement...
		aiEnt->NPC->forceWalkTime = level.time + 100;
		Jedi_Move(aiEnt, aiEnt->enemy, qtrue);
	}
	else if (aiEnt->s.weapon == WP_SABER /*&& Jedi_ClearPathToSpot(aiEnt, aiEnt->enemy->r.currentOrigin, aiEnt->enemy->s.number)*/)
	{// Jedi go direct...
		aiEnt->NPC->forceWalkTime = level.time + 100;
		Jedi_Move(aiEnt, aiEnt->enemy, qtrue);
	}
	else if (G_NavmeshIsLoaded())
	{
		if (Jedi_AdvanceRetreatGoalInvalid(aiEnt))
		{
			NavlibSetNavMesh(aiEnt->s.number, 0);

#pragma omp critical
			{
				FindRandomNavmeshPointInRadius(aiEnt->s.number, aiEnt->enemy->r.currentOrigin, aiEnt->client->navigation.goal.origin, COMBAT_POSIITON_SELECTION_RANGE);
			}

			float distToGoal = Distance(aiEnt->client->navigation.goal.origin, aiEnt->enemy->r.currentOrigin);

			if (VectorLength(aiEnt->client->navigation.goal.origin) == 0 || distToGoal > COMBAT_POSIITON_ALLOW_RANGE)
			{
#ifdef __DEBUG_COMBAT_POSITIONS__
				trap->Print("NPC %s failed to find combat retreat goal. goal %f %f %f. enemy: %f %f %f. distToGoal %f.\n", aiEnt->client->pers.netname
					, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
					, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
#endif //__DEBUG_COMBAT_POSITIONS__

				aiEnt->client->navigation.goal.haveGoal = qfalse;
				aiEnt->NPC->forceWalkTime = level.time + 100;
				Jedi_Move(aiEnt, aiEnt->enemy, qtrue);
			}
			else
			{
				aiEnt->NPC->forceWalkTime = 0;

#pragma omp critical
				{
					aiEnt->client->navigation.goal.haveGoal = (qboolean)NavlibFindRouteToTarget(aiEnt, aiEnt->client->navigation.goal, qtrue);
				}

#ifdef __DEBUG_COMBAT_POSITIONS__
				if (aiEnt->client->navigation.goal.haveGoal)
				{
					trap->Print("NPC %s found a combat retreat goal. goal %f %f %f. enemy: %f %f %f. distToGoal %f.\n", aiEnt->client->pers.netname
						, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
						, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
				}
				else
				{
					trap->Print("NPC %s failed to find combat retreat goal path. goal %f %f %f. enemy: %f %f %f. distToGoal %f.\n", aiEnt->client->pers.netname
						, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
						, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
				}
#endif //__DEBUG_COMBAT_POSITIONS__

				if (!aiEnt->client->navigation.goal.haveGoal)
				{// Fallback to JKA movement...
					aiEnt->NPC->forceWalkTime = level.time + 100;
					Jedi_Move(aiEnt, aiEnt->enemy, qtrue);
				}
			}
		}
		else
		{
#ifdef __DEBUG_COMBAT_POSITIONS__
			float distToGoal = Distance(aiEnt->client->navigation.goal.origin, aiEnt->enemy->r.currentOrigin);

			trap->Print("NPC %s is heading to a combat goal. goal %f %f %f. enemy: %f %f %f. distToGoal %f.\n", aiEnt->client->pers.netname
				, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
				, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
#endif //__DEBUG_COMBAT_POSITIONS__
		}
	}
	else
	{
		aiEnt->NPC->forceWalkTime = level.time + 100;
		Jedi_Move(aiEnt, aiEnt->enemy, qtrue);
	}
#else //!__USE_NAVLIB__
	aiEnt->NPC->forceWalkTime = level.time + 100;
	Jedi_Move(aiEnt, aiEnt->enemy, qtrue );
#endif //__USE_NAVLIB__
}

void Jedi_Advance( gentity_t *aiEnt)
{
	if ( !aiEnt->client->ps.saberInFlight )
	{
		//NPC->client->ps.SaberActivate();
		WP_ActivateSaber(aiEnt);
	}
	//Com_Printf( "Advancing\n" );

#ifdef __USE_NAVLIB__
	if (!aiEnt->enemy || !NPC_ValidEnemy(aiEnt, aiEnt->enemy))
	{// No enemy, so use original JKA movement...
		aiEnt->NPC->forceWalkTime = 0;
		Jedi_Move(aiEnt, aiEnt->enemy, qfalse);
	}
	else if (aiEnt->s.weapon == WP_SABER /*&& Jedi_ClearPathToSpot(aiEnt, aiEnt->enemy->r.currentOrigin, aiEnt->enemy->s.number)*/)
	{// Jedi go direct...
		aiEnt->NPC->forceWalkTime = 0;
		Jedi_Move(aiEnt, aiEnt->enemy, qfalse);
	}
	else if (G_NavmeshIsLoaded())
	{
		if (Jedi_AdvanceRetreatGoalInvalid(aiEnt))
		{
			NavlibSetNavMesh(aiEnt->s.number, 0);

#pragma omp critical
			{
				FindRandomNavmeshPointInRadius(aiEnt->s.number, aiEnt->enemy->r.currentOrigin, aiEnt->client->navigation.goal.origin, COMBAT_POSIITON_SELECTION_RANGE);
			}

			float distToGoal = Distance(aiEnt->client->navigation.goal.origin, aiEnt->enemy->r.currentOrigin);

			if (VectorLength(aiEnt->client->navigation.goal.origin) == 0 || distToGoal > COMBAT_POSIITON_ALLOW_RANGE)
			{
#ifdef __DEBUG_COMBAT_POSITIONS__
				trap->Print("NPC %s failed to find combat advance goal. goal %f %f %f. enemy: %f %f %f. distToGoal: %f.\n", aiEnt->client->pers.netname
					, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
					, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
#endif //__DEBUG_COMBAT_POSITIONS__

				aiEnt->client->navigation.goal.haveGoal = qfalse;
				aiEnt->NPC->forceWalkTime = 0;
				Jedi_Move(aiEnt, aiEnt->enemy, qfalse);
			}
			else
			{
				aiEnt->NPC->forceWalkTime = 0;

#pragma omp critical
				{
					aiEnt->client->navigation.goal.haveGoal = (qboolean)NavlibFindRouteToTarget(aiEnt, aiEnt->client->navigation.goal, qtrue);
				}

#ifdef __DEBUG_COMBAT_POSITIONS__
				if (aiEnt->client->navigation.goal.haveGoal)
				{
					trap->Print("NPC %s found a combat advance goal. goal %f %f %f. enemy: %f %f %f. distToGoal %f.\n", aiEnt->client->pers.netname
						, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
						, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
				}
				else
				{
					trap->Print("NPC %s failed to find combat advance goal path. goal %f %f %f. enemy: %f %f %f. distToGoal %f.\n", aiEnt->client->pers.netname
						, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
						, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
				}
#endif //__DEBUG_COMBAT_POSITIONS__

				if (!aiEnt->client->navigation.goal.haveGoal)
				{// Fallback to JKA movement...
					aiEnt->NPC->forceWalkTime = 0;
					Jedi_Move(aiEnt, aiEnt->enemy, qfalse);
				}
			}
		}
		else
		{
#ifdef __DEBUG_COMBAT_POSITIONS__
			float distToGoal = Distance(aiEnt->client->navigation.goal.origin, aiEnt->enemy->r.currentOrigin);

			trap->Print("NPC %s is heading to a combat goal. goal %f %f %f. enemy: %f %f %f. distToGoal %f.\n", aiEnt->client->pers.netname
				, aiEnt->client->navigation.goal.origin[0], aiEnt->client->navigation.goal.origin[1], aiEnt->client->navigation.goal.origin[2]
				, aiEnt->enemy->r.currentOrigin[0], aiEnt->enemy->r.currentOrigin[1], aiEnt->enemy->r.currentOrigin[2], distToGoal);
#endif //__DEBUG_COMBAT_POSITIONS__
		}
	}
	else
	{
		aiEnt->NPC->forceWalkTime = 0;
		Jedi_Move(aiEnt, aiEnt->enemy, qfalse);
	}
#else //!__USE_NAVLIB__
	aiEnt->NPC->forceWalkTime = 0;
	Jedi_Move(aiEnt, aiEnt->enemy, qfalse );
#endif //__USE_NAVLIB__

	//TIMER_Set( NPC, "roamTime", Q_irand( 2000, 4000 ) );
	//TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );
	//TIMER_Set( NPC, "duck", 0 );
}

void Jedi_AdjustSaberAnimLevel( gentity_t *self, int newLevel )
{
	if ( !self || !self->client )
	{
		return;
	}
#ifdef __FUCK_THIS__
	//FIXME: each NPC shold have a unique pattern of behavior for the order in which they
	if ( self->client->NPC_class == CLASS_TAVION )
	{//special attacks
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_5;
		return;
	}
	else if ( self->client->NPC_class == CLASS_DESANN )
	{//special attacks
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_4;
		return;
	}
	if ( self->client->playerTeam == NPCTEAM_ENEMY )
	{
		if ( self->NPC->rank == RANK_CIVILIAN || self->NPC->rank == RANK_LT_JG )
		{//grunt and fencer always uses quick attacks
			self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
			return;
		}
		if ( self->NPC->rank == RANK_CREWMAN
			|| self->NPC->rank == RANK_ENSIGN )
		{//acrobat & force-users always use medium attacks
			self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_2;
			return;
		}
		/*
		if ( self->NPC->rank == RANK_LT )
		{//boss always uses strong attacks
			self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_3;
			return;
		}
		*/
	}
	//use the different attacks, how often they switch and under what circumstances
	if ( newLevel > self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
	{//cap it
		self->client->ps.fd.saberAnimLevel = self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}
	else if ( newLevel < FORCE_LEVEL_1 )
	{
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}
	else
	{//go ahead and set it
		self->client->ps.fd.saberAnimLevel = newLevel;
	}

	if ( d_JediAI.integer )
	{
		switch ( self->client->ps.fd.saberAnimLevel )
		{
		case FORCE_LEVEL_1:
			Com_Printf( S_COLOR_GREEN"%s Saber Attack Set: fast\n", self->NPC_type );
			break;
		case FORCE_LEVEL_2:
			Com_Printf( S_COLOR_YELLOW"%s Saber Attack Set: medium\n", self->NPC_type );
			break;
		case FORCE_LEVEL_3:
			Com_Printf( S_COLOR_RED"%s Saber Attack Set: strong\n", self->NPC_type );
			break;
		}
	}
#endif //__FUCK_THIS__

	if (self->client->NPC_class == CLASS_TAVION)
	{//special attacks
		self->client->ps.fd.saberAnimLevel = SS_TAVION;
		self->client->ps.fd.saberAnimLevelBase = SS_TAVION;
		self->client->ps.fd.saberDrawAnimLevel = SS_TAVION;
		self->client->npcFavoredStance = SS_TAVION;
		return;
	}
	else if (self->client->NPC_class == CLASS_DESANN)
	{//special attacks
		self->client->ps.fd.saberAnimLevel = SS_DESANN;
		self->client->ps.fd.saberAnimLevelBase = SS_DESANN;
		self->client->ps.fd.saberDrawAnimLevel = SS_DESANN;
		self->client->npcFavoredStance = SS_DESANN;
		return;
	}
	else
	{
		//Cmd_SaberAttackCycle_f(self);

		/*
		RANK_CIVILIAN,
		RANK_CREWMAN,
		RANK_ENSIGN,
		RANK_LT_JG,
		RANK_LT,
		RANK_LT_COMM,
		RANK_COMMANDER,
		RANK_CAPTAIN
		*/
		/*if (self->NPC->rank == RANK_CIVILIAN || self->NPC->rank == RANK_LT_JG)
		{//grunt and fencer always uses quick attacks
			self->client->ps.fd.saberAnimLevel = SS_FAST;
			self->client->ps.fd.saberAnimLevelBase = SS_FAST;
			self->client->ps.fd.saberDrawAnimLevel = SS_FAST;
			return;
		}
		if (self->NPC->rank == RANK_CREWMAN || self->NPC->rank == RANK_ENSIGN)
		{//acrobat & force-users always use medium attacks
			self->client->ps.fd.saberAnimLevel = SS_MEDIUM;
			self->client->ps.fd.saberAnimLevelBase = SS_MEDIUM;
			self->client->ps.fd.saberDrawAnimLevel = SS_MEDIUM;
			return;
		}
		if ( self->NPC->rank == RANK_LT )
		{//boss always uses strong attacks
			self->client->ps.fd.saberAnimLevel = SS_STRONG;
			self->client->ps.fd.saberAnimLevelBase = SS_STRONG;
			self->client->ps.fd.saberDrawAnimLevel = SS_STRONG;
			return;
		}
		else
		{
			self->client->ps.fd.saberAnimLevel = SS_CROWD_CONTROL;
			self->client->ps.fd.saberAnimLevelBase = SS_CROWD_CONTROL;
			self->client->ps.fd.saberDrawAnimLevel = SS_CROWD_CONTROL;
			return;
		}*/

		if (self->client->npcFavoredStance >= SS_FAST)
		{// Already selected a favored stance...
			self->client->ps.fd.saberAnimLevel = self->client->npcFavoredStance;
			self->client->ps.fd.saberAnimLevelBase = self->client->npcFavoredStance;
			self->client->ps.fd.saberDrawAnimLevel = self->client->npcFavoredStance;
			return;
		}

		if (self->client->saber[0].model[0] && self->client->saber[1].model[0])
		{// Dual sabers...
			int styleChoice = irand(0, 1);

			switch (styleChoice)
			{
			case 0:
				self->client->ps.fd.saberAnimLevel = SS_DUAL;
				self->client->ps.fd.saberAnimLevelBase = SS_DUAL;
				self->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
				break;
			default:
				self->client->ps.fd.saberAnimLevel = SS_CROWD_CONTROL;
				self->client->ps.fd.saberAnimLevelBase = SS_CROWD_CONTROL;
				self->client->ps.fd.saberDrawAnimLevel = SS_CROWD_CONTROL;
				break;
			}
		}
		else if (self->client->saber[0].numBlades > 1)
		{// Dual blade...
			int styleChoice = irand(0, 1);

			switch (styleChoice)
			{
			case 0:
				self->client->ps.fd.saberAnimLevel = SS_STAFF;
				self->client->ps.fd.saberAnimLevelBase = SS_STAFF;
				self->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
				break;
			default:
				self->client->ps.fd.saberAnimLevel = SS_CROWD_CONTROL;
				self->client->ps.fd.saberAnimLevelBase = SS_CROWD_CONTROL;
				self->client->ps.fd.saberDrawAnimLevel = SS_CROWD_CONTROL;
				break;
			}
		}
		else
		{// Single saber...
			int styleChoice = irand(0, 11);

			switch (styleChoice)
			{
			case 0:
			case 1:
				self->client->ps.fd.saberAnimLevel = SS_FAST;
				self->client->ps.fd.saberAnimLevelBase = SS_FAST;
				self->client->ps.fd.saberDrawAnimLevel = SS_FAST;
				break;
			case 2:
			case 3:
				self->client->ps.fd.saberAnimLevel = SS_MEDIUM;
				self->client->ps.fd.saberAnimLevelBase = SS_MEDIUM;
				self->client->ps.fd.saberDrawAnimLevel = SS_MEDIUM;
				break;
			case 4:
				self->client->ps.fd.saberAnimLevel = SS_STRONG;
				self->client->ps.fd.saberAnimLevelBase = SS_STRONG;
				self->client->ps.fd.saberDrawAnimLevel = SS_STRONG;
				break;
			case 5:
			case 6:
				self->client->ps.fd.saberAnimLevel = SS_DESANN;
				self->client->ps.fd.saberAnimLevelBase = SS_DESANN;
				self->client->ps.fd.saberDrawAnimLevel = SS_DESANN;
				break;
			case 7:
			case 8:
				self->client->ps.fd.saberAnimLevel = SS_TAVION;
				self->client->ps.fd.saberAnimLevelBase = SS_TAVION;
				self->client->ps.fd.saberDrawAnimLevel = SS_TAVION;
				break;
			default:
				self->client->ps.fd.saberAnimLevel = SS_CROWD_CONTROL;
				self->client->ps.fd.saberAnimLevelBase = SS_CROWD_CONTROL;
				self->client->ps.fd.saberDrawAnimLevel = SS_CROWD_CONTROL;
				break;
			}
		}

		// Remember our favored stance...
		self->client->npcFavoredStance = self->client->ps.fd.saberAnimLevel;
		//trap->Print("NPC %s selected stance %i.\n", self->client->pers.netname, self->client->npcFavoredStance);
	}
}

extern void ForceTelepathy(gentity_t *self);

static void Jedi_CheckDecreaseSaberAnimLevel( gentity_t *aiEnt)
{
	if ( !aiEnt->client->ps.weaponTime && !(aiEnt->client->pers.cmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK)) )
	{//not attacking
		if ( TIMER_Done( aiEnt, "saberLevelDebounce" ) && !Q_irand( 0, 10 ) )
		{
			//Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel-1) );//drop
			Jedi_AdjustSaberAnimLevel( aiEnt, Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 ));//random
			TIMER_Set( aiEnt, "saberLevelDebounce", Q_irand( 3000, 10000 ) );
		}
	}
	else
	{
		TIMER_Set( aiEnt, "saberLevelDebounce", Q_irand( 1000, 5000 ) );
	}
}

extern void ForceDrain( gentity_t *self );
static void Jedi_CombatDistance( gentity_t *aiEnt, int enemy_dist )
{//FIXME: for many of these checks, what we really want is horizontal distance to enemy
	if (!aiEnt->enemy) return;

#if 0
	if ( aiEnt->client->ps.fd.forcePowersActive&(1<<FP_GRIP) &&
		aiEnt->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//when gripping, don't move
		return;
	}
	else if ( !TIMER_Done( aiEnt, "gripping" ) )
	{//stopped gripping, clear timers just in case
		TIMER_Set( aiEnt, "gripping", -level.time );
		TIMER_Set( aiEnt, "attackDelay", Q_irand( 0, 1000 ) );
	}

	if ( Jedi_CultistDestroyer( aiEnt ) )
	{
		Jedi_Advance();
		aiEnt->client->ps.speed = aiEnt->NPC->stats.runSpeed;
		aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
	}

	if ( aiEnt->client->ps.fd.forcePowersActive&(1<<FP_DRAIN) &&
		aiEnt->client->ps.fd.forcePowerLevel[FP_DRAIN] >= FORCE_LEVEL_1 )
	{//when draining, don't move
		return;
	}
	else if ( aiEnt->client->ps.fd.forcePowersActive&(1<<FP_GRIP) &&
		aiEnt->client->ps.fd.forcePowerLevel[FP_DRAIN] >= FORCE_LEVEL_1 )
	{//when gripping, don't move
		return;
	}
	else if ( !TIMER_Done( aiEnt, "draining" ) )
	{//stopped draining, clear timers just in case
		TIMER_Set( aiEnt, "draining", -level.time );
		TIMER_Set( aiEnt, "attackDelay", Q_irand( 0, 1000 ) );
	}
	else if ( !TIMER_Done( aiEnt, "gripping" ) )
	{//stopped gripping, clear timers just in case
		TIMER_Set( aiEnt, "gripping", -level.time );
		TIMER_Set( aiEnt, "attackDelay", Q_irand( 0, 1000 ) );
	}

	// UQ1: Special heals/protects/absorbs - mainly for padawans...
	if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0
		&& (aiEnt->s.NPC_class == CLASS_PADAWAN || Q_irand( 0, 1 ))
		&& aiEnt->health < aiEnt->maxHealth * 0.75)
	{
		ForceHeal( aiEnt );
	}
	if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_PROTECT)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_PROTECT)) == 0
		&& (aiEnt->s.NPC_class == CLASS_PADAWAN || Q_irand( 0, 1 ))
		&& Q_irand( 0, 1 ) )
	{
		ForceProtect( aiEnt );
	}
	else if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_ABSORB)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_ABSORB)) == 0
		&& (aiEnt->s.NPC_class == CLASS_PADAWAN || Q_irand( 0, 1 ))
		&& Q_irand( 0, 1 ) )
	{
		ForceAbsorb( aiEnt );
	}

	if ( !aiEnt->client->ps.saberInFlight && TIMER_Done( aiEnt, "taunting" ) )
	{
		if ( enemy_dist > 256 )
		{//we're way out of range
			qboolean usedForce = qfalse;
			if ( aiEnt->NPC->stats.aggression < Q_irand( 0, 20 )
				&& aiEnt->health < aiEnt->client->pers.maxHealth*0.75f
				&& !Q_irand( 0, 2 ) )
			{
				if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_TELEPATHY)) != 0
					&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_TELEPATHY)) == 0
					&& Q_irand( 0, 1 ) )
				{
					ForceTelepathy(aiEnt);
					usedForce = qtrue;
				}
				else if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_PROTECT)) != 0
					&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_PROTECT)) == 0
					&& Q_irand( 0, 1 ) )
				{
					ForceProtect( aiEnt );
					usedForce = qtrue;
				}
				else if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_ABSORB)) != 0
					&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_ABSORB)) == 0
					&& Q_irand( 0, 1 ) )
				{
					ForceAbsorb( aiEnt );
					usedForce = qtrue;
				}
				else if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_RAGE)) != 0
					&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) == 0
					&& Q_irand( 0, 1 ) )
				{
					Jedi_Rage();
					usedForce = qtrue;
				}
				else if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_SPEED)) != 0
					&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_SPEED)) == 0
					&& Q_irand( 0, 1 ) )
				{
					ForceSpeed( aiEnt, 500 );
					usedForce = qtrue;
				}
				else if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_GRIP)) != 0
					&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_GRIP)) == 0
					&& Q_irand( 0, 1 ) )
				{
					ForceGrip( aiEnt );
					TIMER_Set( aiEnt, "gripping", 3000 );
					TIMER_Set( aiEnt, "attackDelay", 3000 );
					usedForce = qtrue;
				}
				else if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_LIGHTNING)) != 0
					&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_LIGHTNING)) == 0
					&& Q_irand( 0, 1 ) )
				{
					ForceLightning( aiEnt );
					usedForce = qtrue;
				}
				//FIXME: what about things like mind tricks and force sight?
			}
			if ( enemy_dist > 384 )
			{//FIXME: check for enemy facing away and/or moving away
				if ( !Q_irand( 0, 10 ) && aiEnt->NPC->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time )
				{
					if ( NPC_ClearLOS4( aiEnt->enemy ) )
					{
						G_AddVoiceEvent( aiEnt, Q_irand( EV_JCHASE1, EV_JCHASE3 ), 10000 );
					}
					jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
				}
			}
		}
		else if ( enemy_dist <= 64
			&& ((aiEnt->NPC->scriptFlags&SCF_DONT_FIRE)||(!Q_stricmp("Yoda",aiEnt->NPC_type)&&!Q_irand(0,10))) )
		{//can't use saber and they're in striking range
			if ( !Q_irand( 0, 5 ) && InFront( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->client->ps.viewangles, 0.2f ) )
			{
				if ( ((aiEnt->NPC->scriptFlags&SCF_DONT_FIRE)||aiEnt->client->pers.maxHealth - aiEnt->health > aiEnt->client->pers.maxHealth*0.25f)//lost over 1/4 of our health or not firing
					&& aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_DRAIN) //know how to drain
					&& WP_ForcePowerAvailable( aiEnt, FP_DRAIN, 20 )//have enough power
					&& !Q_irand( 0, 2 ) )
				{//drain
					TIMER_Set( aiEnt, "draining", 3000 );
					TIMER_Set( aiEnt, "attackDelay", 3000 );
					Jedi_Advance();
					return;
				}
				else
				{
					ForceThrow( aiEnt, qfalse );
				}
			}
			Jedi_Retreat();
		}
		else if ( enemy_dist <= SABER_ATTACK_RANGE
			&& aiEnt->client->pers.maxHealth - aiEnt->health > aiEnt->client->pers.maxHealth*0.25f//lost over 1/4 of our health
			&& aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_DRAIN) //know how to drain
			&& WP_ForcePowerAvailable( aiEnt, FP_DRAIN, 20 )//have enough power
			&& !Q_irand( 0, 10 )
			&& InFront( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, aiEnt->client->ps.viewangles, 0.2f ) )
		{
			TIMER_Set( aiEnt, "draining", 3000 );
			TIMER_Set( aiEnt, "attackDelay", 3000 );
			Jedi_Advance();
			return;
		}
	}
#endif //0

	if ( NPC_IsBountyHunter(aiEnt) || aiEnt->hasJetpack )
	{
		if ( !TIMER_Done( aiEnt, "flameTime" ) )
		{
			if ( enemy_dist > 50 )
			{
				Jedi_Advance(aiEnt);
			}
			else if ( enemy_dist < 38 )
			{
				Jedi_Retreat(aiEnt);
			}
		}
		else if ( enemy_dist < 200 )
		{
			Jedi_Retreat(aiEnt);
		}
		else if ( enemy_dist > 1024 )
		{
			Jedi_Advance(aiEnt);
		}
	}
	else if ( aiEnt->client->ps.saberInFlight
		&& !PM_SaberInBrokenParry( aiEnt->client->ps.saberMove )
		&& aiEnt->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
	{//maintain distance
		if ( enemy_dist < aiEnt->client->ps.saberEntityDist )
		{
			Jedi_Retreat(aiEnt);
		}
		else if ( enemy_dist > aiEnt->client->ps.saberEntityDist && enemy_dist > 100 )
		{
			Jedi_Advance(aiEnt);
		}
		if ( aiEnt->client->ps.weapon == WP_SABER //using saber
			&& aiEnt->client->ps.saberEntityState == SES_LEAVING  //not returning yet
			&& aiEnt->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(aiEnt->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
			&& !(aiEnt->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
		{//hold it out there
			aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
			//FIXME: time limit?
		}
	}
	else if ( !TIMER_Done( aiEnt, "taunting" ) )
	{
		if (aiEnt->enemy && !NPC_ValidEnemy(aiEnt, aiEnt->enemy))
		{

		}
		else if ( enemy_dist <= 64 )
		{//he's getting too close
			aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
			if ( !aiEnt->client->ps.saberInFlight )
			{
				WP_ActivateSaber(aiEnt);
			}
			TIMER_Set( aiEnt, "taunting", -level.time );
		}
		//else if ( NPC->client->ps.torsoAnim == BOTH_GESTURE1 && NPC->client->ps.torsoTimer < 2000 )
		else if (aiEnt->client->ps.forceHandExtend == HANDEXTEND_JEDITAUNT && (aiEnt->client->ps.forceHandExtendTime - level.time) < 200)
		{//we're almost done with our special taunt
			//FIXME: this doesn't always work, for some reason
			if ( !aiEnt->client->ps.saberInFlight )
			{
				WP_ActivateSaber(aiEnt);
			}
		}
	}
	else if ( aiEnt->client->ps.saberEventFlags&SEF_LOCK_WON )
	{//we won a saber lock, press the advantage
		if ( enemy_dist > 0 )
		{//get closer so we can hit!
			Jedi_Advance(aiEnt);
		}
		if ( enemy_dist > 128 )
		{//lost 'em
			aiEnt->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		if ( aiEnt->enemy->painDebounceTime + 2000 < level.time )
		{//the window of opportunity is gone
			aiEnt->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		//don't strafe?
		TIMER_Set( aiEnt, "strafeLeft", -1 );
		TIMER_Set( aiEnt, "strafeRight", -1 );
	}
	else if ( aiEnt->enemy
		&& aiEnt->enemy->client
		&& (aiEnt->enemy->s.weapon == WP_SABER || aiEnt->enemy->s.weapon == WP_MELEE)
		&& aiEnt->enemy->client->ps.saberLockTime > level.time
		&& aiEnt->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		if ( enemy_dist < 64 )
		{//FIXME: maybe just pick another enemy?
			Jedi_Retreat(aiEnt);
		}
	}
	else if ( (aiEnt->s.weapon == WP_SABER || aiEnt->s.weapon == WP_MELEE)
		&& enemy_dist > SABER_ATTACK_RANGE )
	{//we're too damn far!
		Jedi_Advance(aiEnt);
	}
	else if ( (aiEnt->s.weapon == WP_SABER || aiEnt->s.weapon == WP_MELEE)
		&& enemy_dist < SABER_ATTACK_RANGE/3 )
	{//we're too damn close!
		Jedi_Retreat(aiEnt);
	}
	else if ( (aiEnt->s.weapon != WP_SABER && aiEnt->s.weapon != WP_MELEE)
		&& enemy_dist <= 256.0 )
	{//we're too damn close!
		//Jedi_Retreat(aiEnt);

		if (irand(0, 8) == 0 && aiEnt->enemy && aiEnt->enemy->s.weapon == WP_SABER)
		{
			extern qboolean Jedi_EvasionRoll(gentity_t *aiEnt);
			if (!Jedi_EvasionRoll(aiEnt))
			{
				Jedi_Retreat(aiEnt);
			}
		}
		else if (aiEnt->enemy->s.weapon != WP_SABER)
		{
			Jedi_Retreat(aiEnt);
		}
	}
	else if ( enemy_dist > 512.0 )
	{//we're too damn far!
		Jedi_Advance(aiEnt);
	}

#ifdef __FORCE_SPEED__
	//if really really mad, rage!
	if ( aiEnt->NPC->stats.aggression > Q_irand( 5, 15 )
		&& aiEnt->health < aiEnt->client->pers.maxHealth*0.75f
		&& !Q_irand( 0, 2 ) )
	{
		if ( (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_RAGE)) != 0
			&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) == 0 )
		{
			Jedi_Rage(aiEnt);
		}
	}
#endif //__FORCE_SPEED__
}

static qboolean Jedi_Strafe( gentity_t *aiEnt, int strafeTimeMin, int strafeTimeMax, int nextStrafeTimeMin, int nextStrafeTimeMax, qboolean walking )
{
	if( Jedi_CultistDestroyer( aiEnt ) )
	{
		return qfalse;
	}
	if ( aiEnt->client->ps.saberEventFlags&SEF_LOCK_WON && aiEnt->enemy && aiEnt->enemy->painDebounceTime > level.time )
	{//don't strafe if pressing the advantage of winning a saberLock
		return qfalse;
	}
	if ( TIMER_Done( aiEnt, "strafeLeft" ) && TIMER_Done( aiEnt, "strafeRight" ) )
	{
		qboolean strafed = qfalse;
		//TODO: make left/right choice a tactical decision rather than random:
		//		try to keep own back away from walls and ledges,
		//		try to keep enemy's back to a ledge or wall
		//		Maybe try to strafe toward designer-placed "safe spots" or "goals"?
		int	strafeTime = Q_irand( strafeTimeMin, strafeTimeMax );

		if ( Q_irand( 0, 1 ) )
		{
			if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, -127, qfalse ) )
			{
				TIMER_Set( aiEnt, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, 127, qfalse ) )
			{
				TIMER_Set( aiEnt, "strafeRight", strafeTime );
				strafed = qtrue;
			}
		}
		else
		{
			if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, 127, qfalse  ) )
			{
				TIMER_Set( aiEnt, "strafeRight", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, -127, qfalse  ) )
			{
				TIMER_Set( aiEnt, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
		}

		if ( strafed )
		{
			TIMER_Set( aiEnt, "noStrafe", strafeTime + Q_irand( nextStrafeTimeMin, nextStrafeTimeMax ) );
			if ( walking )
			{//should be a slow strafe
				TIMER_Set( aiEnt, "walking", strafeTime );
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
static void Jedi_FaceEntity( gentity_t *self, gentity_t *other, qboolean doPitch )
{
	vec3_t		entPos;
	vec3_t		muzzle;

	//Get the positions
	CalcEntitySpot( other, SPOT_ORIGIN, entPos );

	//Get the positions
	CalcEntitySpot( self, SPOT_HEAD_LEAN, muzzle );//SPOT_HEAD

	//Find the desired angles
	vec3_t	angles;

	GetAnglesForDirection( muzzle, entPos, angles );

	self->NPC->desiredYaw		= AngleNormalize360( angles[YAW] );
	if ( doPitch )
	{
		self->NPC->desiredPitch	= AngleNormalize360( angles[PITCH] );
	}
}
*/

/*
qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc )

Jedi will play a dodge anim, blur, and make the force speed noise.

Right now used to dodge instant-hit weapons.

FIXME: possibly call this for saber melee evasion and/or missile evasion?
FIXME: possibly let player do this too?
*/
//rwwFIXMEFIXME: Going to use qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc ) from
//w_saber.c.. maybe use seperate one for NPCs or add cases to that one?

evasionType_t Jedi_CheckFlipEvasions( gentity_t *self, float rightdot, float zdiff )
{
	if ( self->NPC && (self->NPC->scriptFlags&SCF_NO_ACROBATICS) )
	{
		return EVASION_NONE;
	}
	if ( self->client
		&& (self->client->ps.fd.forceRageRecoveryTime > level.time	|| (self->client->ps.fd.forcePowersActive&(1<<FP_RAGE))) )
	{//no fancy dodges when raging
		return EVASION_NONE;
	}
	//Check for:
	//ARIALS/CARTWHEELS
	//WALL-RUNS
	//WALL-FLIPS
	//FIXME: if facing a wall, do forward wall-walk-backflip
	if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT || self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT )
	{//already running on a wall
		vec3_t right, fwdAngles;
		int		anim = -1;
		float animLength;

		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors( fwdAngles, NULL, right, NULL );

		animLength = BG_AnimLength( self->localAnimIndex, (animNumber_t)self->client->ps.legsAnim );
		if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT && rightdot < 0 )
		{//I'm running on a wall to my left and the attack is on the left
			if ( animLength - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_LEFT_FLIP;
			}
		}
		else if ( self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT && rightdot > 0 )
		{//I'm running on a wall to my right and the attack is on the right
			if ( animLength - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_RIGHT_FLIP;
			}
		}
		if ( anim != -1 )
		{//flip off the wall!
			int parts;
			//FIXME: check the direction we will flip towards for do-not-enter/walls/drops?
			//NOTE: we presume there is still a wall there!
			if ( anim == BOTH_WALL_RUN_LEFT_FLIP )
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA( self->client->ps.velocity, 150, right, self->client->ps.velocity );
			}
			else if ( anim == BOTH_WALL_RUN_RIGHT_FLIP )
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA( self->client->ps.velocity, -150, right, self->client->ps.velocity );
			}
			parts = SETANIM_LEGS;
			if ( !self->client->ps.weaponTime )
			{
				parts = SETANIM_BOTH;
			}
			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
			//rwwFIXMEFIXME: Add these pm flags?
			G_AddEvent( self, EV_JUMP, 0 );
			return EVASION_OTHER;
		}
	}
	else if ( self->client->NPC_class != CLASS_DESANN //desann doesn't do these kind of frilly acrobatics
		&& (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT)
		&& Q_irand( 0, 5 ) > 4
		&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )
		&& !PM_InKnockDown( &self->client->ps )
		&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
	{
		vec3_t fwd, right, traceto, mins, maxs, fwdAngles;
		trace_t	trace;
		int parts, anim;
		float	speed, checkDist;
		qboolean allowCartWheels = qtrue;
		qboolean allowWallFlips = qtrue;

		if ( self->client->ps.weapon == WP_SABER )
		{
			if ( self->client->saber[0].model[0]
				&& (self->client->saber[0].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
			else if ( self->client->saber[1].model[0]
				&& (self->client->saber[1].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
			if ( self->client->saber[0].model[0]
				&& (self->client->saber[0].saberFlags&SFL_NO_WALL_FLIPS) )
			{
				allowWallFlips = qfalse;
			}
			else if ( self->client->saber[1].model[0]
				&& (self->client->saber[1].saberFlags&SFL_NO_WALL_FLIPS) )
			{
				allowWallFlips = qfalse;
			}
		}

		VectorSet(mins, self->r.mins[0],self->r.mins[1],0);
		VectorSet(maxs, self->r.maxs[0],self->r.maxs[1],24);
		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors( fwdAngles, fwd, right, NULL );

		parts = SETANIM_BOTH;

		if ( BG_SaberInAttack( self->client->ps.saberMove )
			|| PM_SaberInStart( self->client->ps.saberMove ) )
		{
			parts = SETANIM_LEGS;
		}
		if ( rightdot >= 0 )
		{
			if ( Q_irand( 0, 1 ) )
			{
				anim = BOTH_ARIAL_LEFT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_LEFT;
			}
			checkDist = -128;
			speed = -200;
		}
		else
		{
			if ( Q_irand( 0, 1 ) )
			{
				anim = BOTH_ARIAL_RIGHT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_RIGHT;
			}
			checkDist = 128;
			speed = 200;
		}

#ifdef __EVASION_JUMPING__
		//trace in the dir that we want to go
		VectorMA( self->r.currentOrigin, checkDist, right, traceto );
		trap->Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, qfalse, 0, 0 );
		
		if ( trace.fraction >= 1.0f && allowCartWheels )
		{//it's clear, let's do it
			//FIXME: check for drops?
			vec3_t jumpRt;

			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.weaponTime = self->client->ps.legsTimer;//don't attack again until this anim is done
			VectorCopy( self->client->ps.viewangles, fwdAngles );
			fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
			//do the flip
			AngleVectors( fwdAngles, NULL, jumpRt, NULL );
			VectorScale( jumpRt, speed, self->client->ps.velocity );
			self->client->ps.fd.forceJumpCharge = 0;//so we don't play the force flip anim
			self->client->ps.velocity[2] = 200;
			self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
			//self->client->ps.pm_flags |= PMF_JUMPING;
			if ( !NPC_IsJedi(self) )
			{
				G_AddEvent( self, EV_JUMP, 0 );
			}
			else
			{
				G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
			}
			//ucmd.upmove = 0;
			return EVASION_CARTWHEEL;
		}
		else if ( !(trace.contents&CONTENTS_BOTCLIP) )
		{//hit a wall, not a do-not-enter brush
			//FIXME: before we check any of these jump-type evasions, we should check for headroom, right?
			//Okay, see if we can flip *off* the wall and go the other way
			vec3_t	idealNormal;
			gentity_t *traceEnt;

			VectorSubtract( self->r.currentOrigin, traceto, idealNormal );
			VectorNormalize( idealNormal );
			traceEnt = &g_entities[trace.entityNum];
			if ( (trace.entityNum<ENTITYNUM_WORLD&&traceEnt&&traceEnt->s.solid!=SOLID_BMODEL) || DotProduct( trace.plane.normal, idealNormal ) > 0.7f )
			{//it's a ent of some sort or it's a wall roughly facing us
				float bestCheckDist = 0;
				//hmm, see if we're moving forward
				if ( DotProduct( self->client->ps.velocity, fwd ) < 200 )
				{//not running forward very fast
					//check to see if it's okay to move the other way
					if ( (trace.fraction*checkDist) <= 32 )
					{//wall on that side is close enough to wall-flip off of or wall-run on
						bestCheckDist = checkDist;
						checkDist *= -1.0f;
						VectorMA( self->r.currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						trap->Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, qfalse, 0, 0 );
						if ( trace.fraction >= 1.0f )
						{//it's clear, let's do it
							if ( allowWallFlips )
							{//okay to do wall-flips with this saber
								//FIXME: check for drops?
								//turn the cartwheel into a wallflip in the other dir
								if ( rightdot > 0 )
								{
									anim = BOTH_WALL_FLIP_LEFT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA( self->client->ps.velocity, 150, right, self->client->ps.velocity );
								}
								else
								{
									anim = BOTH_WALL_FLIP_RIGHT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA( self->client->ps.velocity, -150, right, self->client->ps.velocity );
								}
								self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
								//animate me
								parts = SETANIM_LEGS;
								if ( !self->client->ps.weaponTime )
								{
									parts = SETANIM_BOTH;
								}
								NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
								//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
								if ( !NPC_IsJedi(self) )
								{
									G_AddEvent( self, EV_JUMP, 0 );
								}
								else
								{
									G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
								}
								return EVASION_OTHER;
								}
						}
						else
						{//boxed in on both sides
							if ( DotProduct( self->client->ps.velocity, fwd ) < 0 )
							{//moving backwards
								return EVASION_NONE;
							}
							if ( (trace.fraction*checkDist) <= 32 && (trace.fraction*checkDist) < bestCheckDist )
							{
								bestCheckDist = checkDist;
							}
						}
					}
					else
					{//too far from that wall to flip or run off it, check other side
						checkDist *= -1.0f;
						VectorMA( self->r.currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						trap->Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, qfalse, 0, 0 );
						if ( (trace.fraction*checkDist) <= 32 )
						{//wall on this side is close enough
							bestCheckDist = checkDist;
						}
						else
						{//neither side has a wall within 32
							return EVASION_NONE;
						}
					}
				}
				//Try wall run?
				if ( bestCheckDist )
				{//one of the walls was close enough to wall-run on
					qboolean allowWallRuns = qtrue;
					if ( self->client->ps.weapon == WP_SABER )
					{
						if ( self->client->saber[0].model[0]
							&& (self->client->saber[0].saberFlags&SFL_NO_WALL_RUNS) )
						{
							allowWallRuns = qfalse;
						}
						else if ( self->client->saber[1].model[0]
							&& (self->client->saber[1].saberFlags&SFL_NO_WALL_RUNS) )
						{
							allowWallRuns = qfalse;
						}
					}
					if ( allowWallRuns )
					{//okay to do wallruns with this saber
						//FIXME: check for long enough wall and a drop at the end?
						if ( bestCheckDist > 0 )
						{//it was to the right
							anim = BOTH_WALL_RUN_RIGHT;
						}
						else
						{//it was to the left
							anim = BOTH_WALL_RUN_LEFT;
						}
						self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
						//animate me
						parts = SETANIM_LEGS;
						if ( !self->client->ps.weaponTime )
						{
							parts = SETANIM_BOTH;
						}
						NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
						//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
						if ( !NPC_IsJedi(self) )
						{
							G_AddEvent( self, EV_JUMP, 0 );
						}
						else
						{
							G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
						}
						return EVASION_OTHER;
					}
				}
				//else check for wall in front, do backflip off wall
			}
		}
#endif //__EVASION_JUMPING__
	}

	return EVASION_NONE;
}

int Jedi_ReCalcParryTime( gentity_t *self, evasionType_t evasionType )
{
	if ( !self->client )
	{
		return 0;
	}
	if ( self->s.number >= 0 && self->s.number < MAX_CLIENTS )
	{//player
		return bg_parryDebounce[self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]];
	}
	else if ( self->NPC )
	{
		if ( !g_saberRealisticCombat.integer
			&& ( g_npcspskill.integer == 2 || (g_npcspskill.integer == 1 && self->client->NPC_class == CLASS_TAVION) ) )
		{
			if ( self->client->NPC_class == CLASS_TAVION )
			{
				return 0;
			}
			else
			{
				return Q_irand( 0, 150 );
			}
		}
		else
		{
			int	baseTime;
			if ( evasionType == EVASION_DODGE )
			{
				baseTime = self->client->ps.torsoTimer;
			}
			else if ( evasionType == EVASION_CARTWHEEL )
			{
				baseTime = self->client->ps.torsoTimer;
			}
			else if ( self->client->ps.saberInFlight )
			{
				baseTime = Q_irand( 1, 3 ) * 50;
			}
			else
			{
				if ( g_saberRealisticCombat.integer )
				{
					baseTime = 500;

					switch ( g_npcspskill.integer )
					{
					case 0:
						baseTime = 500;
						break;
					case 1:
						baseTime = 300;
						break;
					case 2:
					default:
						baseTime = 100;
						break;
					}
				}
				else
				{
					baseTime = 150;//500;

					switch ( g_npcspskill.integer )
					{
					case 0:
						baseTime = 200;//500;
						break;
					case 1:
						baseTime = 100;//300;
						break;
					case 2:
					default:
						baseTime = 50;//100;
						break;
					}
				}

				if ( self->client->NPC_class == CLASS_TAVION )
				{//Tavion is faster
					baseTime = ceil(baseTime/2.0f);
				}
				else if ( self->NPC->rank >= RANK_LT_JG )
				{//fencers, bosses, shadowtroopers, luke, desann, et al use the norm
					if ( !Q_irand( 0, 2 ) )
					{//with the occasional fast parry
						baseTime = ceil(baseTime/2.0f);
					}
				}
				else if ( self->NPC->rank == RANK_CIVILIAN )
				{//grunts are slowest
					baseTime = baseTime*Q_irand(1,3);
				}
				else if ( self->NPC->rank == RANK_CREWMAN )
				{//acrobats aren't so bad
					if ( evasionType == EVASION_PARRY
						|| evasionType == EVASION_DUCK_PARRY
						|| evasionType == EVASION_JUMP_PARRY )
					{//slower with parries
						baseTime = baseTime*Q_irand(1,2);
					}
					else
					{//faster with acrobatics
						//baseTime = baseTime;
					}
				}
				else
				{//force users are kinda slow
					baseTime = baseTime*Q_irand(1,2);
				}
				if ( evasionType == EVASION_DUCK || evasionType == EVASION_DUCK_PARRY )
				{
					baseTime += 100;
				}
				else if ( evasionType == EVASION_JUMP || evasionType == EVASION_JUMP_PARRY )
				{
					baseTime += 50;
				}
				else if ( evasionType == EVASION_OTHER )
				{
					baseTime += 100;
				}
				else if ( evasionType == EVASION_FJUMP )
				{
					baseTime += 100;
				}
			}

			return baseTime;
		}
	}
	return 0;
}

qboolean Jedi_QuickReactions( gentity_t *self )
{
	if ( NPC_IsJedi(self) )
	{
		return qtrue;
	}

	return qfalse;
}

qboolean Jedi_SaberBusy( gentity_t *self )
{
	if ( self->client->ps.torsoTimer > 300
	&& ( (BG_SaberInAttack( self->client->ps.saberMove )&&self->client->ps.fd.saberAnimLevel==FORCE_LEVEL_3)
		|| BG_SpinningSaberAnim( self->client->ps.torsoAnim )
		|| BG_SaberInSpecialAttack( self->client->ps.torsoAnim )
		//|| PM_SaberInBounce( self->client->ps.saberMove )
		|| PM_SaberInBrokenParry( self->client->ps.saberMove )
		//|| PM_SaberInDeflect( self->client->ps.saberMove )
		|| BG_FlippingAnim( self->client->ps.torsoAnim )
		|| PM_RollingAnim( self->client->ps.torsoAnim ) ) )
	{//my saber is not in a parrying position
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
Jedi_SaberBlock

Pick proper block anim

FIXME: Based on difficulty level/enemy saber combat skill, make this decision-making more/less effective

NOTE: always blocking projectiles in this func!

-------------------------
*/
extern qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result );
evasionType_t Jedi_SaberBlockGo( gentity_t *self, usercmd_t *cmd, vec3_t pHitloc, vec3_t phitDir, gentity_t *incoming, float dist ) //dist = 0.0f
{
	vec3_t hitloc, hitdir, diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;
	int	  duckChance = 0;
	int	  dodgeAnim = -1;
	qboolean	saberBusy = qfalse, doDodge = qfalse;
	evasionType_t	evasionType = EVASION_NONE;

	//FIXME: if we don't have our saber in hand, pick the force throw option or a jump or strafe!
	//FIXME: reborn don't block enough anymore
	if ( !incoming )
	{
		VectorCopy( pHitloc, hitloc );
		VectorCopy( phitDir, hitdir );
		//FIXME: maybe base this on rank some?  And/or g_npcspskill?
		if ( self->client->ps.saberInFlight )
		{//DOH!  do non-saber evasion!
			saberBusy = qtrue;
		}
		else if ( Jedi_QuickReactions( self ) )
		{//jedi trainer and tavion are much faster at parrying and can do it whenever they like
			//Also, on medium, all level 3 people can parry any time and on hard, all level 2 or 3 people can parry any time
		}
		else
		{
			saberBusy = Jedi_SaberBusy( self );
		}
	}
	else
	{
		if ( incoming->s.weapon == WP_SABER )
		{//flying lightsaber, face it!
			//FIXME: for this to actually work, we'd need to call update angles too?
			//Jedi_FaceEntity( self, incoming, qtrue );
		}
		VectorCopy( incoming->r.currentOrigin, hitloc );
		VectorNormalize2( incoming->s.pos.trDelta, hitdir );
	}
	
	if ( self->client && self->client->ps.weapon != WP_SABER )
	{
		saberBusy = qtrue;
	}

	VectorSubtract( hitloc, self->client->renderInfo.eyePoint, diff );
	diff[2] = 0;
	//VectorNormalize( diff );
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff);// + flrand(-0.10f,0.10f);
	//totalHeight = self->client->renderInfo.eyePoint[2] - self->r.absmin[2];
	zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];// + Q_irand(-6,6);

	//see if we can dodge if need-be
	if ( (dist>16 && (Q_irand( 0, 2 ) || saberBusy))
		|| self->client->ps.saberInFlight
		|| BG_SabersOff( &self->client->ps )
		|| self->client->ps.weapon != WP_SABER )
	{//either it will miss by a bit (and 25% chance) OR our saber is not in-hand OR saber is off
		if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT_JG) )
		{//acrobat or fencer or above
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE &&//on the ground
				!(self->client->ps.pm_flags&PMF_DUCKED)&&cmd->upmove>=0&&TIMER_Done( self, "duck" )//not ducking
				&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )//not rolling
				&& !PM_InKnockDown( &self->client->ps )//not knocked down
				&& ( self->client->ps.saberInFlight ||
					self->client->ps.weapon != WP_SABER/*!NPC_IsJedi(self)*/ ||
					(!BG_SaberInAttack( self->client->ps.saberMove )//not attacking
					&& !PM_SaberInStart( self->client->ps.saberMove )//not starting an attack
					&& !BG_SpinningSaberAnim( self->client->ps.torsoAnim )//not in a saber spin
					&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ))//not in a special attack
					)
				)
			{//need to check all these because it overrides both torso and legs with the dodge
				doDodge = qtrue;
			}
		}
	}
	// Figure out what quadrant the block was in.
	if ( d_JediAI.integer )
	{
		Com_Printf( "(%d) evading attack from height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, hitloc[2]-self->r.absmin[2],zdiff,rightdot);
	}

	//UL = > -1//-6
	//UR = > -6//-9
	//TOP = > +6//+4
	//FIXME: take FP_SABER_DEFENSE into account here somehow?
	if ( zdiff >= -5 )//was 0
	{
		if ( incoming || !saberBusy )
		{
			if ( rightdot > 12
				|| (rightdot > 3 && zdiff < 5)
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, 0.3
			{//coming from right
				if ( doDodge )
				{
					if ( !NPC_IsJedi(self) && !Q_irand( 0, 2 ) )
					{//roll!
						TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
						evasionType = EVASION_DUCK;
					}
					else if ( Q_irand( 0, 1 ) )
					{
						dodgeAnim = BOTH_DODGE_FL;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BL;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					evasionType = EVASION_PARRY;
					if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{
						if ( zdiff > 5 )
						{
							TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
							evasionType = EVASION_DUCK_PARRY;
							if ( d_JediAI.integer )
							{
								Com_Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "UR block\n" );
				}
			}
			else if ( rightdot < -12
				|| (rightdot < -3 && zdiff < 5)
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, -0.3
			{//coming from left
				if ( doDodge )
				{
					if ( !NPC_IsJedi(self) && !Q_irand( 0, 2 ) )
					{//roll!
						TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
						TIMER_Start( self, "strafeRight", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeLeft", 0 );
						evasionType = EVASION_DUCK;
					}
					else if ( Q_irand( 0, 1 ) )
					{
						dodgeAnim = BOTH_DODGE_FR;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BR;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					evasionType = EVASION_PARRY;
					if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{
						if ( zdiff > 5 )
						{
							TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
							evasionType = EVASION_DUCK_PARRY;
							if ( d_JediAI.integer )
							{
								Com_Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "UL block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				evasionType = EVASION_PARRY;
				if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					duckChance = 4;
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "TOP block\n" );
				}
			}
		}
		else
		{
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{
				//duckChance = 2;
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
				}
			}
		}
	}
	//LL = -22//= -18 to -39
	//LR = -23//= -20 to -41
	else if ( zdiff > -22 )//was-15 )
	{
		if ( 1 )//zdiff < -10 )
		{//hmm, pretty low, but not low enough to use the low block, so we need to duck
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{
				//duckChance = 2;
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
				}
			}
			else
			{//in air!  Ducking does no good
			}
		}

		if ( incoming || !saberBusy )
		{
			if ( rightdot > 8 || (rightdot > 3 && zdiff < -11) )//was normalized, 0.2
			{
				if ( doDodge )
				{
					if ( !NPC_IsJedi(self) && !Q_irand( 0, 2 ) )
					{//roll!
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
					}
					else
					{
						dodgeAnim = BOTH_DODGE_L;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					if ( evasionType == EVASION_DUCK )
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-UR block\n" );
				}
			}
			else if ( rightdot < -8 || (rightdot < -3 && zdiff < -11) )//was normalized, -0.2
			{
				if ( doDodge )
				{
					if ( !NPC_IsJedi(self) && !Q_irand( 0, 2 ) )
					{//roll!
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
					}
					else
					{
						dodgeAnim = BOTH_DODGE_R;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					if ( evasionType == EVASION_DUCK )
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-UL block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				if ( evasionType == EVASION_DUCK )
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_PARRY;
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-TOP block\n" );
				}
			}
		}
	}
#ifdef __EVASION_JUMPING__
	else if ( saberBusy || (zdiff < -36 && ( zdiff < -44 || !Q_irand( 0, 2 ) ) ) )//was -30 and -40//2nd one was -46
	{//jump!
		if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{//already in air, duck to pull up legs
			TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
			evasionType = EVASION_DUCK;
			if ( d_JediAI.integer )
			{
				Com_Printf( "legs up\n" );
			}
			if ( incoming || !saberBusy )
			{
				//since the jump may be cleared if not safe, set a lower block too
				if ( rightdot >= 0 )
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
					evasionType = EVASION_DUCK_PARRY;
					if ( d_JediAI.integer )
					{
						Com_Printf( "LR block\n" );
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
					evasionType = EVASION_DUCK_PARRY;
					if ( d_JediAI.integer )
					{
						Com_Printf( "LL block\n" );
					}
				}
			}
		}
		else
		{//gotta jump!
			if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG ) &&
				(!Q_irand( 0, 10 ) || (!Q_irand( 0, 2 ) && (cmd->forwardmove || cmd->rightmove))) )
			{//superjump
				//FIXME: check the jump, if can't, then block
				if ( self->NPC
					&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
					&& self->client->ps.fd.forceRageRecoveryTime < level.time
					&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
					&& !PM_InKnockDown( &self->client->ps ) )
				{
					self->client->ps.fd.forceJumpCharge = 320;//FIXME: calc this intelligently
					evasionType = EVASION_FJUMP;
					if ( d_JediAI.integer )
					{
						Com_Printf( "force jump + " );
					}
				}
			}
			else
			{//normal jump
				//FIXME: check the jump, if can't, then block
				if ( self->NPC
					&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
					&& self->client->ps.fd.forceRageRecoveryTime < level.time
					&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) )
				{
					if ( !NPC_IsJedi(self) && !Q_irand( 0, 1 ) )
					{//roll!
						if ( rightdot > 0 )
						{
							TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
							TIMER_Set( self, "strafeRight", 0 );
							TIMER_Set( self, "walking", 0 );
						}
						else
						{
							TIMER_Start( self, "strafeRight", Q_irand( 500, 1500 ) );
							TIMER_Set( self, "strafeLeft", 0 );
							TIMER_Set( self, "walking", 0 );
						}
					}
					else
					{
						if ( self == aiEnt )
						{
							cmd->upmove = 127;
						}
						else
						{
							self->client->ps.velocity[2] = JUMP_VELOCITY;
						}
					}
					evasionType = EVASION_JUMP;
					if ( d_JediAI.integer )
					{
						Com_Printf( "jump + " );
					}
				}
				if ( self->client->NPC_class == CLASS_TAVION )
				{
					if ( !incoming
						&& self->client->ps.groundEntityNum < ENTITYNUM_NONE
						&& !Q_irand( 0, 2 ) )
					{
						if ( !BG_SaberInAttack( self->client->ps.saberMove )
							&& !PM_SaberInStart( self->client->ps.saberMove )
							&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )
							&& !PM_InKnockDown( &self->client->ps )
							&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
						{//do the butterfly!
							int butterflyAnim;
							if ( Q_irand( 0, 1 ) )
							{
								butterflyAnim = BOTH_BUTTERFLY_LEFT;
							}
							else
							{
								butterflyAnim = BOTH_BUTTERFLY_RIGHT;
							}
							evasionType = EVASION_CARTWHEEL;
							NPC_SetAnim( self, SETANIM_BOTH, butterflyAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							self->client->ps.velocity[2] = 225;
							self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
						//	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
						//	self->client->ps.SaberActivateTrail( 300 );//FIXME: reset this when done!
							//Ah well. No hacking from the server for now.
							if ( !NPC_IsJedi(self) )
							{
								G_AddEvent( self, EV_JUMP, 0 );
							}
							else
							{
								G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jump.wav") );
							}
							cmd->upmove = 0;
							saberBusy = qtrue;
						}
					}
				}
			}
			if ( ((evasionType = Jedi_CheckFlipEvasions( self, rightdot, zdiff ))!=EVASION_NONE) )
			{
				saberBusy = qtrue;
			}
			else if ( incoming || !saberBusy )
			{
				//since the jump may be cleared if not safe, set a lower block too
				if ( rightdot >= 0 )
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
					if ( evasionType == EVASION_JUMP )
					{
						evasionType = EVASION_JUMP_PARRY;
					}
					else if ( evasionType == EVASION_NONE )
					{
						evasionType = EVASION_PARRY;
					}
					if ( d_JediAI.integer )
					{
						Com_Printf( "LR block\n" );
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
					if ( evasionType == EVASION_JUMP )
					{
						evasionType = EVASION_JUMP_PARRY;
					}
					else if ( evasionType == EVASION_NONE )
					{
						evasionType = EVASION_PARRY;
					}
					if ( d_JediAI.integer )
					{
						Com_Printf( "LL block\n" );
					}
				}
			}
		}
	}
#endif //__EVASION_JUMPING__
	else
	{
		if ( incoming || !saberBusy )
		{
			if ( rightdot >= 0 )
			{
				self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
				evasionType = EVASION_PARRY;
				if ( d_JediAI.integer )
				{
					Com_Printf( "LR block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
				evasionType = EVASION_PARRY;
				if ( d_JediAI.integer )
				{
					Com_Printf( "LL block\n" );
				}
			}
#ifdef __EVASION_JUMPING__
			if ( incoming && incoming->s.weapon == WP_SABER )
			{//thrown saber!
				if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG ) &&
					(!Q_irand( 0, 10 ) || (!Q_irand( 0, 2 ) && (cmd->forwardmove || cmd->rightmove))) )
				{//superjump
					//FIXME: check the jump, if can't, then block
					if ( self->NPC
						&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
						&& !PM_InKnockDown( &self->client->ps ) )
					{
						self->client->ps.fd.forceJumpCharge = 320;//FIXME: calc this intelligently
						evasionType = EVASION_FJUMP;
						if ( d_JediAI.integer )
						{
							Com_Printf( "force jump + " );
						}
					}
				}
				else
				{//normal jump
					//FIXME: check the jump, if can't, then block
					if ( self->NPC
						&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)))
					{
						if ( self == aiEnt )
						{
							cmd->upmove = 127;
						}
						else
						{
							self->client->ps.velocity[2] = JUMP_VELOCITY;
						}
						evasionType = EVASION_JUMP_PARRY;
						if ( d_JediAI.integer )
						{
							Com_Printf( "jump + " );
						}
					}
				}
			}
#endif //__EVASION_JUMPING__
		}
	}

	//stop taunting
	TIMER_Set( self, "taunting", 0 );
	//stop gripping
	TIMER_Set( self, "gripping", -level.time );
	WP_ForcePowerStop( self, FP_GRIP );
	//stop draining
	TIMER_Set( self, "draining", -level.time );
	WP_ForcePowerStop( self, FP_DRAIN );

	if ( dodgeAnim != -1 )
	{//dodged
		evasionType = EVASION_DODGE;
		NPC_SetAnim( self, SETANIM_BOTH, dodgeAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		//force them to stop moving in this case
		self->client->ps.pm_time = self->client->ps.torsoTimer;
		//FIXME: maybe make a sound?  Like a grunt?  EV_JUMP?
		self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		//dodged, not block
	}
	else
	{
		if ( duckChance )
		{
			if ( !Q_irand( 0, duckChance ) )
			{
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				if ( evasionType == EVASION_PARRY )
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_DUCK;
				}
				/*
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
				}
				*/
			}
		}

		if ( incoming )
		{
			self->client->ps.saberBlocked = WP_MissileBlockForBlock( self->client->ps.saberBlocked );
		}

	}
	//if ( self->client->ps.saberBlocked != BLOCKED_NONE )
	{
		int parryReCalcTime = Jedi_ReCalcParryTime( self, evasionType );
		if ( self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime )
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}
	return evasionType;
}

extern float ShortestLineSegBewteen2LineSegs( vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1, vec3_t close_pnt2 );
extern int WPDEBUG_SaberColor( saber_colors_t saberColor );
static qboolean Jedi_SaberBlock( gentity_t *aiEnt, int saberNum, int bladeNum ) //saberNum = 0, bladeNum = 0
{
	vec3_t hitloc, saberTipOld, saberTip, top, bottom, axisPoint, saberPoint, dir;//saberBase,
	vec3_t pointDir, baseDir, tipDir, saberHitPoint, saberMins, saberMaxs;
	float	pointDist, baseDirPerc, dist;
	float	bladeLen = 0;
	trace_t	tr;
	evasionType_t	evasionType;

	//FIXME: reborn don't block enough anymore
	/*
	//maybe do this on easy only... or only on grunt-level reborn
	if ( NPC->client->ps.weaponTime )
	{//i'm attacking right now
		return qfalse;
	}
	*/

	if ( !TIMER_Done( aiEnt, "parryReCalcTime" ) )
	{//can't do our own re-think of which parry to use yet
		return qfalse;
	}

	if ( aiEnt->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] > level.time )
	{//can't move the saber to another position yet
		return qfalse;
	}

	/*
	if ( NPCInfo->rank < RANK_LT_JG && Q_irand( 0, (2 - g_npcspskill.integer) ) )
	{//lower rank reborn have a random chance of not doing it at all
		NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 300;
		return qfalse;
	}
	*/

	if ( aiEnt->enemy->health <= 0 || !aiEnt->enemy->client )
	{//don't keep blocking him once he's dead (or if not a client)
		return qfalse;
	}

	if ( Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin) > 64
		|| !(aiEnt->enemy->client->pers.cmd.buttons & BUTTON_ATTACK))
	{// UQ1: They were doing evasion WAY too far away... And when the enemy isn't even attacking...
		return qfalse;
	}

	/*
	//VectorMA( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDir, saberTip );
	//VectorMA( NPC->enemy->client->renderInfo.muzzlePointNext, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDirNext, saberTipNext );
	VectorMA( NPC->enemy->client->renderInfo.muzzlePointOld, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDirOld, saberTipOld );
	VectorMA( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDir, saberTip );

	VectorSubtract( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->renderInfo.muzzlePointOld, dir );//get the dir
	VectorAdd( dir, NPC->enemy->client->renderInfo.muzzlePoint, saberBase );//extrapolate

	VectorSubtract( saberTip, saberTipOld, dir );//get the dir
	VectorAdd( dir, saberTip, saberTipOld );//extrapolate

	VectorCopy( NPC->r.currentOrigin, top );
	top[2] = NPC->r.absmax[2];
	VectorCopy( NPC->r.currentOrigin, bottom );
	bottom[2] = NPC->r.absmin[2];

	float dist = ShortestLineSegBewteen2LineSegs( saberBase, saberTipOld, bottom, top, saberPoint, axisPoint );
	if ( 0 )//dist > NPC->r.maxs[0]*4 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI.integer )
		{
			Com_Printf( "enemy saber dist: %4.2f\n", dist );
		}
		TIMER_Set( NPC, "parryTime", -1 );
		return qfalse;
	}

	//get the actual point of impact
	trace_t	tr;
	trap->Trace( &tr, saberPoint, vec3_origin, vec3_origin, axisPoint, NPC->enemy->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
	if ( tr.allsolid || tr.startsolid )
	{//estimate
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->r.maxs[0]*1.22, dir, hitloc );
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}
	*/
	VectorSet(saberMins,-4,-4,-4);
	VectorSet(saberMaxs,4,4,4);

	VectorMA( aiEnt->enemy->client->saber[saberNum].blade[bladeNum].muzzlePointOld, aiEnt->enemy->client->saber[saberNum].blade[bladeNum].length, aiEnt->enemy->client->saber[saberNum].blade[bladeNum].muzzleDirOld, saberTipOld );
	VectorMA( aiEnt->enemy->client->saber[saberNum].blade[bladeNum].muzzlePoint, aiEnt->enemy->client->saber[saberNum].blade[bladeNum].length, aiEnt->enemy->client->saber[saberNum].blade[bladeNum].muzzleDir, saberTip );
//	VectorCopy(NPC->enemy->client->lastSaberBase_Always, muzzlePoint);
//	VectorMA(muzzlePoint, GAME_SABER_LENGTH, NPC->enemy->client->lastSaberDir_Always, saberTip);
//	VectorCopy(saberTip, saberTipOld);

	VectorCopy( aiEnt->r.currentOrigin, top );
	top[2] = aiEnt->r.absmax[2];
	VectorCopy( aiEnt->r.currentOrigin, bottom );
	bottom[2] = aiEnt->r.absmin[2];

	dist = ShortestLineSegBewteen2LineSegs( aiEnt->enemy->client->renderInfo.muzzlePoint, saberTip, bottom, top, saberPoint, axisPoint );
	if ( dist > aiEnt->r.maxs[0]*5 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI.integer )
		{
			Com_Printf( S_COLOR_RED"enemy saber dist: %4.2f\n", dist );
		}
		/*
		if ( dist < 300 //close
			&& !Jedi_QuickReactions( NPC )//quick reaction people can interrupt themselves
			&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || BG_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
		{//he's swinging at me and close enough to be a threat, don't start an attack right now
			TIMER_Set( NPC, "parryTime", 100 );
		}
		else
		*/
		{
			TIMER_Set( aiEnt, "parryTime", -1 );
		}
		return qfalse;
	}
	if ( d_JediAI.integer )
	{
		Com_Printf( S_COLOR_GREEN"enemy saber dist: %4.2f\n", dist );
	}

	VectorSubtract( saberPoint, aiEnt->enemy->client->renderInfo.muzzlePoint, pointDir );
	pointDist = VectorLength( pointDir );

	bladeLen = aiEnt->enemy->client->saber[saberNum].blade[bladeNum].length;

	if ( bladeLen <= 0 )
	{
		baseDirPerc = 0.5f;
	}
	else
	{
		baseDirPerc = pointDist/bladeLen;
	}
	VectorSubtract( aiEnt->enemy->client->renderInfo.muzzlePoint, aiEnt->enemy->client->renderInfo.muzzlePointOld, baseDir );
	VectorSubtract( saberTip, saberTipOld, tipDir );
	VectorScale( baseDir, baseDirPerc, baseDir );
	VectorMA( baseDir, 1.0f-baseDirPerc, tipDir, dir );
	VectorMA( saberPoint, 200, dir, hitloc );

	//get the actual point of impact
	trap->Trace( &tr, saberPoint, saberMins, saberMaxs, hitloc, aiEnt->enemy->s.number, CONTENTS_BODY, qfalse, 0, 0 );//, G2_RETURNONHIT, 10 );
	if ( tr.allsolid || tr.startsolid || tr.fraction >= 1.0f )
	{//estimate
		vec3_t	dir2Me;
		VectorSubtract( axisPoint, saberPoint, dir2Me );
		dist = VectorNormalize( dir2Me );
		if ( DotProduct( dir, dir2Me ) < 0.2f )
		{//saber is not swinging in my direction
			/*
			if ( dist < 300 //close
				&& !Jedi_QuickReactions( NPC )//quick reaction people can interrupt themselves
				&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || BG_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
			{//he's swinging at me and close enough to be a threat, don't start an attack right now
				TIMER_Set( NPC, "parryTime", 100 );
			}
			else
			*/
			{
				TIMER_Set( aiEnt, "parryTime", -1 );
			}
			return qfalse;
		}
		ShortestLineSegBewteen2LineSegs( saberPoint, hitloc, bottom, top, saberHitPoint, hitloc );
		/*
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->r.maxs[0]*1.22, dir, hitloc );
		*/
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}

	if ( d_JediAI.integer )
	{
		//G_DebugLine( saberPoint, hitloc, FRAMETIME, WPDEBUG_SaberColor( NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].color ), qtrue );
		G_TestLine(saberPoint, hitloc, 0x0000ff, FRAMETIME);
	}

	//FIXME: if saber is off and/or we have force speed and want to be really cocky,
	//		and the swing misses by some amount, we can use the dodges here... :)
	if ( (evasionType=Jedi_SaberBlockGo( aiEnt, &aiEnt->client->pers.cmd, hitloc, dir, NULL, dist )) != EVASION_DODGE )
	{//we did block (not dodge)
		int parryReCalcTime;

		if ( !aiEnt->client->ps.saberInFlight )
		{//make sure saber is on
			WP_ActivateSaber(aiEnt);
		}

		//debounce our parry recalc time
		parryReCalcTime = Jedi_ReCalcParryTime( aiEnt, evasionType );
		TIMER_Set( aiEnt, "parryReCalcTime", Q_irand( 0, parryReCalcTime ) );
		if ( d_JediAI.integer )
		{
			Com_Printf( "Keep parry choice until: %d\n", level.time + parryReCalcTime );
		}

		//determine how long to hold this anim
		if ( TIMER_Done( aiEnt, "parryTime" ) )
		{
			if ( aiEnt->client->NPC_class == CLASS_TAVION )
			{
				TIMER_Set( aiEnt, "parryTime", Q_irand( parryReCalcTime/2, parryReCalcTime*1.5 ) );
			}
			else if ( aiEnt->NPC->rank >= RANK_LT_JG )
			{//fencers and higher hold a parry less
				TIMER_Set( aiEnt, "parryTime", parryReCalcTime );
			}
			else
			{//others hold it longer
				TIMER_Set( aiEnt, "parryTime", Q_irand( 1, 2 )*parryReCalcTime );
			}
		}
	}
	else
	{
		int dodgeTime = aiEnt->client->ps.torsoTimer;
		if ( aiEnt->NPC->rank > RANK_LT_COMM && aiEnt->client->NPC_class != CLASS_DESANN )
		{//higher-level guys can dodge faster
			dodgeTime -= 200;
		}
		TIMER_Set( aiEnt, "parryReCalcTime", dodgeTime );
		TIMER_Set( aiEnt, "parryTime", dodgeTime );
	}
	return qtrue;
}

qboolean Jedi_EvasionRoll(gentity_t *aiEnt)
{
	if (!aiEnt->enemy->client)
	{
		aiEnt->npc_roll_start = qfalse;
		return qfalse;
	}
	else if (aiEnt->enemy->client
		&& aiEnt->enemy->s.weapon == WP_SABER
		&& aiEnt->enemy->client->ps.saberLockTime > level.time)
	{//don't try to block/evade an enemy who is in a saberLock
		aiEnt->npc_roll_start = qfalse;
		return qfalse;
	}
	else if (aiEnt->client->ps.saberEventFlags&SEF_LOCK_WON && aiEnt->enemy->painDebounceTime > level.time)
	{//pressing the advantage of winning a saber lock
		aiEnt->npc_roll_start = qfalse;
		return qfalse;
	}

	if (aiEnt->npc_roll_time >= level.time)
	{// Already in a roll...
		aiEnt->npc_roll_start = qfalse;
		return qfalse;
	}

	// Init...
	aiEnt->npc_roll_direction = EVASION_ROLL_DIR_NONE;
	aiEnt->npc_roll_start = qfalse;

#ifdef __ROLL_EVASION_TRACES__
	qboolean canRollBack = qfalse;
	qboolean canRollLeft = qfalse;
	qboolean canRollRight = qfalse;

	trace_t tr;
	vec3_t fwd, right, up, start, end;
	AngleVectors(aiEnt->r.currentAngles, fwd, right, up);

	VectorSet(start, aiEnt->r.currentOrigin[0], aiEnt->r.currentOrigin[1], aiEnt->r.currentOrigin[2] + 24.0);
	VectorMA(start, -128, fwd, end);
	trap->Trace(&tr, start, NULL, NULL, end, aiEnt->s.number, MASK_NPCSOLID, qfalse, 0, 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(aiEnt, end))
	{// We can roll back...
		canRollBack = qtrue;
	}

	VectorMA(start, -128, right, end);
	trap->Trace(&tr, start, NULL, NULL, end, aiEnt->s.number, MASK_NPCSOLID, qfalse, 0, 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(aiEnt, end))
	{// We can roll back...
		canRollLeft = qtrue;
	}

	VectorMA(start, 128, right, end);
	trap->Trace(&tr, start, NULL, NULL, end, aiEnt->s.number, MASK_NPCSOLID, qfalse, 0, 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(aiEnt, end))
	{// We can roll back...
		canRollRight = qtrue;
	}
#else //!__ROLL_EVASION_TRACES__
	qboolean canRollBack = qtrue;
	qboolean canRollLeft = qtrue;
	qboolean canRollRight = qtrue;
#endif //__ROLL_EVASION_TRACES__

	if (canRollBack && canRollLeft && canRollRight)
	{
		int choice = irand(0, 2);

		switch (choice) {
		case 2:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			//trap->Print("%s chose to roll right. had 3 options.\n", aiEnt->client->pers.netname);
			break;
		case 1:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			//trap->Print("%s chose to roll left. had 3 options.\n", aiEnt->client->pers.netname);
			break;
		default:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			//trap->Print("%s chose to roll back. had 3 options.\n", aiEnt->client->pers.netname);
			break;
		}

		return qtrue;
	}
	else if (canRollBack && canRollLeft)
	{
		int choice = irand(0, 1);

		switch (choice) {
		case 1:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			//trap->Print("%s chose to roll left. had 2 options. back or left.\n", aiEnt->client->pers.netname);
			break;
		default:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			//trap->Print("%s chose to roll back. had 2 options. back or left.\n", aiEnt->client->pers.netname);
			break;
		}

		return qtrue;
	}
	else if (canRollBack && canRollRight)
	{
		int choice = irand(0, 1);

		switch (choice) {
		case 1:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			//trap->Print("%s chose to roll right. had 2 options. back or right.\n", aiEnt->client->pers.netname);
			break;
		default:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			//trap->Print("%s chose to roll back. had 2 options. back or right.\n", aiEnt->client->pers.netname);
			break;
		}

		return qtrue;
	}
	else if (canRollLeft && canRollRight)
	{
		int choice = irand(0, 1);

		switch (choice) {
		case 1:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			//trap->Print("%s chose to roll right. had 2 options. left or right.\n", aiEnt->client->pers.netname);
			break;
		default:
			aiEnt->npc_roll_time = level.time + 5000;
			aiEnt->npc_roll_start = qtrue;
			aiEnt->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			//trap->Print("%s chose to roll left. had 2 options. left or right.\n", aiEnt->client->pers.netname);
			break;
		}

		return qtrue;
	}
	else if (canRollLeft)
	{
		aiEnt->npc_roll_time = level.time + 5000;
		aiEnt->npc_roll_start = qtrue;
		aiEnt->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
		//trap->Print("%s chose to roll left. his only option.\n", aiEnt->client->pers.netname);
		return qtrue;
	}
	else if (canRollRight)
	{
		aiEnt->npc_roll_time = level.time + 5000;
		aiEnt->npc_roll_start = qtrue;
		aiEnt->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
		//trap->Print("%s chose to roll right. his only option.\n", aiEnt->client->pers.netname);
		return qtrue;
	}
	else if (canRollBack)
	{
		aiEnt->npc_roll_time = level.time + 5000;
		aiEnt->npc_roll_start = qtrue;
		aiEnt->npc_roll_direction = EVASION_ROLL_DIR_BACK;
		//trap->Print("%s chose to roll back. his only option.\n", aiEnt->client->pers.netname);
		return qtrue;
	}

	//trap->Print("%s had no roll options.\n", aiEnt->client->pers.netname);
	return qfalse;
}

/*
-------------------------
Jedi_EvasionSaber

defend if other is using saber and attacking me!
-------------------------
*/
void Jedi_EvasionSaber( gentity_t *aiEnt, vec3_t enemy_movedir, float enemy_dist, vec3_t enemy_dir )
{
	vec3_t	dirEnemy2Me;
	int		evasionChance = 30;//only step aside 30% if he's moving at me but not attacking
	qboolean	enemy_attacking = qfalse;
	qboolean	throwing_saber = qfalse;
	qboolean	shooting_lightning = qfalse;

	if ( !aiEnt->enemy->client )
	{
		return;
	}
	else if ( aiEnt->enemy->client
		&& aiEnt->enemy->s.weapon == WP_SABER
		&& aiEnt->enemy->client->ps.saberLockTime > level.time )
	{//don't try to block/evade an enemy who is in a saberLock
		return;
	}
	else if ( aiEnt->client->ps.saberEventFlags&SEF_LOCK_WON && aiEnt->enemy->painDebounceTime > level.time )
	{//pressing the advantage of winning a saber lock
		return;
	}

	if ( aiEnt->enemy->client->ps.saberInFlight && !TIMER_Done( aiEnt, "taunting" ) )
	{//if he's throwing his saber, stop taunting
		TIMER_Set( aiEnt, "taunting", -level.time );
		if ( !aiEnt->client->ps.saberInFlight )
		{
			WP_ActivateSaber(aiEnt);
		}
	}

	if ( TIMER_Done( aiEnt, "parryTime" ) )
	{
		if ( aiEnt->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			aiEnt->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}

	if ( aiEnt->enemy->client->ps.weaponTime && aiEnt->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{
		if ( !aiEnt->client->ps.saberInFlight && Jedi_SaberBlock(aiEnt, 0, 0) )
		{
			return;
		}
	}

	VectorSubtract( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin, dirEnemy2Me );
	VectorNormalize( dirEnemy2Me );

	if ( aiEnt->enemy->client->ps.weaponTime && aiEnt->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{//enemy is attacking
		enemy_attacking = qtrue;
		evasionChance = 90;
	}

	if ( (aiEnt->enemy->client->ps.fd.forcePowersActive&(1<<FP_LIGHTNING) ) )
	{//enemy is shooting lightning
		enemy_attacking = qtrue;
		shooting_lightning = qtrue;
		evasionChance = 50;
	}

	if ( aiEnt->enemy->client->ps.saberInFlight && aiEnt->enemy->client->ps.saberEntityNum != ENTITYNUM_NONE && aiEnt->enemy->client->ps.saberEntityState != SES_RETURNING )
	{//enemy is shooting lightning
		enemy_attacking = qtrue;
		throwing_saber = qtrue;
	}

	//FIXME: this needs to take skill and rank(reborn type) into account much more
	if ( Q_irand( 0, 100 ) < evasionChance )
	{//check to see if he's coming at me
		float facingAmt;
		if ( VectorCompare( enemy_movedir, vec3_origin ) || shooting_lightning || throwing_saber )
		{//he's not moving (or he's using a ranged attack), see if he's facing me
			vec3_t	enemy_fwd;
			AngleVectors( aiEnt->enemy->client->ps.viewangles, enemy_fwd, NULL, NULL );
			facingAmt = DotProduct( enemy_fwd, dirEnemy2Me );
		}
		else
		{//he's moving
			facingAmt = DotProduct( enemy_movedir, dirEnemy2Me );
		}

		if ( flrand( 0.25, 1 ) < facingAmt )
		{//coming at/facing me!
			int whichDefense = 0;
			if ( aiEnt->client->ps.weaponTime || aiEnt->client->ps.saberInFlight || aiEnt->client->ps.weapon != WP_SABER )
			{//I'm attacking or recovering from a parry, can only try to strafe/jump right now
				if ( Q_irand( 0, 10 ) < aiEnt->NPC->stats.aggression )
				{
					return;
				}
				whichDefense = 100;
			}
			else
			{
				if ( shooting_lightning )
				{//check for lightning attack
					//only valid defense is strafe and/or jump
					whichDefense = 100;
				}
				else if ( throwing_saber )
				{//he's thrown his saber!  See if it's coming at me
					float	saberDist;
					vec3_t	saberDir2Me;
					vec3_t	saberMoveDir;
					gentity_t *saber = &g_entities[aiEnt->enemy->client->ps.saberEntityNum];
					VectorSubtract( aiEnt->r.currentOrigin, saber->r.currentOrigin, saberDir2Me );
					saberDist = VectorNormalize( saberDir2Me );
					VectorCopy( saber->s.pos.trDelta, saberMoveDir );
					VectorNormalize( saberMoveDir );
					if ( !Q_irand( 0, 3 ) )
					{
						//Com_Printf( "(%d) raise agg - enemy threw saber\n", level.time );
						Jedi_Aggression( aiEnt, 1 );
					}
					if ( DotProduct( saberMoveDir, saberDir2Me ) > 0.5 )
					{//it's heading towards me
						if ( saberDist < 100 )
						{//it's close
							whichDefense = Q_irand( 3, 6 );
						}
						else if ( saberDist < 200 )
						{//got some time, yet, try pushing
							whichDefense = Q_irand( 0, 8 );
						}
					}
				}
				if ( whichDefense )
				{//already chose one
				}
				else if ( enemy_dist > 80 || !enemy_attacking )
				{//he's pretty far, or not swinging, just strafe
					if ( VectorCompare( enemy_movedir, vec3_origin ) )
					{//if he's not moving, not swinging and far enough away, no evasion necc.
						return;
					}
					if ( Q_irand( 0, 10 ) < aiEnt->NPC->stats.aggression )
					{
						return;
					}
					whichDefense = 100;
				}
				else
				{//he's getting close and swinging at me
					vec3_t	fwd;
					//see if I'm facing him
					AngleVectors( aiEnt->client->ps.viewangles, fwd, NULL, NULL );
					if ( DotProduct( enemy_dir, fwd ) < 0.5 )
					{//I'm not really facing him, best option is to strafe
						whichDefense = Q_irand( 5, 16 );
					}
					else if ( enemy_dist < 56 )
					{//he's very close, maybe we should be more inclined to block or throw
						whichDefense = Q_irand( aiEnt->NPC->stats.aggression, 12 );
					}
					else
					{
						whichDefense = Q_irand( 2, 16 );
					}
				}
			}

			if ( whichDefense >= 4 && whichDefense <= 12 )
			{//would try to block
				if ( aiEnt->client->ps.saberInFlight )
				{//can't, saber in not in hand, so fall back to strafe/jump
					whichDefense = 100;
				}
			}

			switch( whichDefense )
			{
			case 0:
			case 1:
			case 2:
			case 3:
				//use jedi force push?
				//FIXME: try to do this if health low or enemy back to a cliff?
				if ( (aiEnt->NPC->rank == RANK_ENSIGN || aiEnt->NPC->rank > RANK_LT_JG) && TIMER_Done( aiEnt, "parryTime" ) )
				{//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
					ForceThrow( aiEnt, qfalse );
				}
				else if (!Jedi_SaberBlock(aiEnt, 0, 0))
				{
					Jedi_EvasionRoll(aiEnt);
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				//try to parry the blow
				//Com_Printf( "blocking\n" );
				if (!Jedi_SaberBlock(aiEnt, 0, 0))
				{
					Jedi_EvasionRoll(aiEnt);
				}
				break;
			default:
				//Evade!
				//start a strafe left/right if not already
				if ( !Q_irand( 0, 5 ) || !Jedi_Strafe(aiEnt, 300, 1000, 0, 1000, qfalse ) )
				{//certain chance they will pick an alternative evasion
					//if couldn't strafe, try a different kind of evasion...
#ifdef __EVASION_JUMPING__
					if ( shooting_lightning || throwing_saber || enemy_dist < 80 )
					{
						//FIXME: force-jump+forward - jump over the guy!
						if ( shooting_lightning || (!Q_irand( 0, 2 ) && aiEnt->NPC->stats.aggression < 4 && TIMER_Done( aiEnt, "parryTime" ) ) )
						{
							if ( (aiEnt->NPC->rank == RANK_ENSIGN || aiEnt->NPC->rank > RANK_LT_JG) && !shooting_lightning && Q_irand( 0, 2 ) )
							{//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
								ForceThrow( aiEnt, qfalse );
							}
							else if ( (aiEnt->NPC->rank==RANK_CREWMAN||aiEnt->NPC->rank>RANK_LT_JG)
								&& !(aiEnt->NPC->scriptFlags&SCF_NO_ACROBATICS)
								&& aiEnt->client->ps.fd.forceRageRecoveryTime < level.time
								&& !(aiEnt->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
								&& !PM_InKnockDown( &aiEnt->client->ps ) )
							{//FIXME: make this a function call?
								//FIXME: check for clearance, safety of landing spot?
								aiEnt->client->ps.fd.forceJumpCharge = 480;
								//Don't jump again for another 2 to 5 seconds
								TIMER_Set( aiEnt, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
								if ( Q_irand( 0, 2 ) )
								{
									if ( NPC_MoveDirClear( 127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
									{
										aiEnt->client->pers.cmd.forwardmove = 127;
										VectorClear( aiEnt->client->ps.moveDir );
									}
								}
								else
								{
									if ( NPC_MoveDirClear( -127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
									{
										aiEnt->client->pers.cmd.forwardmove = -127;
										VectorClear( aiEnt->client->ps.moveDir );
									}
								}
								//FIXME: if this jump is cleared, we can't block... so pick a random lower block?
								if ( Q_irand( 0, 1 ) )//FIXME: make intelligent
								{
									aiEnt->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
								}
								else
								{
									aiEnt->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
								}
							}
						}
						else if ( enemy_attacking )
						{
							if (!Jedi_SaberBlock(aiEnt, 0, 0))
							{
								Jedi_EvasionRoll(aiEnt);
							}
						}
					}
#else //!__EVASION_JUMPING__
					if (!Jedi_SaberBlock(aiEnt, 0, 0))
					{
						Jedi_EvasionRoll(aiEnt);
					}
#endif //__EVASION_JUMPING__
				}
				else
				{//strafed
					if ( d_JediAI.integer )
					{
						Com_Printf( "def strafe\n" );
					}
#ifdef __EVASION_JUMPING__
					if ( !(aiEnt->NPC->scriptFlags&SCF_NO_ACROBATICS)
						&& aiEnt->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(aiEnt->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
						&& (aiEnt->NPC->rank == RANK_CREWMAN || aiEnt->NPC->rank > RANK_LT_JG )
						&& !PM_InKnockDown( &aiEnt->client->ps )
						&& !Q_irand( 0, 5 ) )
					{//FIXME: make this a function call?
						//FIXME: check for clearance, safety of landing spot?
						if ( !NPC_IsJedi(aiEnt) )
						{
							aiEnt->client->ps.fd.forceJumpCharge = 280;//FIXME: calc this intelligently?
						}
						else
						{
							aiEnt->client->ps.fd.forceJumpCharge = 320;
						}
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( aiEnt, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
					}
#else //!__EVASION_JUMPING__
					if (!Jedi_SaberBlock(aiEnt, 0, 0))
					{
						Jedi_EvasionRoll(aiEnt);
					}
#endif //__EVASION_JUMPING__
				}
				break;
			}

			//turn off slow walking no matter what
			TIMER_Set( aiEnt, "walking", -level.time );
			TIMER_Set( aiEnt, "taunting", -level.time );
		}
	}
}

/*
-------------------------
Jedi_Flee
-------------------------
*/
/*

static qboolean Jedi_Flee( void )
{
	return qfalse;
}
*/

/*
==========================================================================================
INTERNAL AI ROUTINES
==========================================================================================
*/
gentity_t *Jedi_FindEnemyInCone( gentity_t *self, gentity_t *fallback, float minDot )
{
	vec3_t forward, mins, maxs, dir;
	float	dist, bestDist = Q3_INFINITE;
	gentity_t	*enemy = fallback;
	gentity_t	*check = NULL;
	int			entityList[MAX_GENTITIES];
	int			e, numListedEntities;
	trace_t		tr;

	if ( !self->client )
	{
		return enemy;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );

	for ( e = 0 ; e < 3 ; e++ )
	{
		mins[e] = self->r.currentOrigin[e] - 1024;
		maxs[e] = self->r.currentOrigin[e] + 1024;
	}
	numListedEntities = trap->EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		check = &g_entities[entityList[e]];
		if ( check == self )
		{//me
			continue;
		}
		if ( !(check->inuse) )
		{//freed
			continue;
		}
		if ( !check->client )
		{//not a client - FIXME: what about turrets?
			continue;
		}
		if ( check->client->playerTeam != self->client->enemyTeam )
		{//not an enemy - FIXME: what about turrets?
			continue;
		}
		if ( check->health <= 0 )
		{//dead
			continue;
		}
		if ( !NPC_ValidEnemy(self, check) )
		{// civilian
			continue;
		}

		if ( !trap->InPVS( check->r.currentOrigin, self->r.currentOrigin ) )
		{//can't potentially see them
			continue;
		}

		VectorSubtract( check->r.currentOrigin, self->r.currentOrigin, dir );
		dist = VectorNormalize( dir );

		if ( DotProduct( dir, forward ) < minDot )
		{//not in front
			continue;
		}

		//really should have a clear LOS to this thing...
		trap->Trace( &tr, self->r.currentOrigin, vec3_origin, vec3_origin, check->r.currentOrigin, self->s.number, MASK_SHOT, qfalse, 0, 0 );
		if ( tr.fraction < 1.0f && tr.entityNum != check->s.number )
		{//must have clear shot
			continue;
		}

		if ( dist < bestDist )
		{//closer than our last best one
			dist = bestDist;
			enemy = check;
		}
	}
	return enemy;
}

void Jedi_SetEnemyInfo( gentity_t *aiEnt, vec3_t enemy_dest, vec3_t enemy_dir, float *enemy_dist, vec3_t enemy_movedir, float *enemy_movespeed, int prediction )
{
	if ( !aiEnt || !aiEnt->enemy )
	{//no valid enemy
		return;
	}
	if ( !aiEnt->enemy->client )
	{
		VectorClear( enemy_movedir );
		*enemy_movespeed = 0;
		VectorCopy( aiEnt->enemy->r.currentOrigin, enemy_dest );
		enemy_dest[2] += aiEnt->enemy->r.mins[2] + 24;//get it's origin to a height I can work with
		VectorSubtract( enemy_dest, aiEnt->r.currentOrigin, enemy_dir );
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir );// - (NPC->client->ps.saberLengthMax + NPC->r.maxs[0]*1.5 + 16);
	}
	else
	{//see where enemy is headed
		VectorCopy( aiEnt->enemy->client->ps.velocity, enemy_movedir );
		*enemy_movespeed = VectorNormalize( enemy_movedir );
		//figure out where he'll be, say, 3 frames from now
		VectorMA( aiEnt->enemy->r.currentOrigin, *enemy_movespeed * 0.001 * prediction, enemy_movedir, enemy_dest );
		//figure out what dir the enemy's estimated position is from me and how far from the tip of my saber he is
		VectorSubtract( enemy_dest, aiEnt->r.currentOrigin, enemy_dir );//NPC->client->renderInfo.muzzlePoint
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir ) - (aiEnt->client->saber[0].blade[0].lengthMax + aiEnt->r.maxs[0]*1.5 + 16); //just use the blade 0 len I guess
		//FIXME: keep a group of enemies around me and use that info to make decisions...
		//		For instance, if there are multiple enemies, evade more, push them away
		//		and use medium attacks.  If enemies are using blasters, switch to fast.
		//		If one jedi enemy, use strong attacks.  Use grip when fighting one or
		//		two enemies, use lightning spread when fighting multiple enemies, etc.
		//		Also, when kill one, check rest of group instead of walking up to victim.
	}
}

extern float WP_SpeedOfMissileForWeapon( int wp, qboolean alt_fire );
static void Jedi_FaceEnemy( gentity_t *aiEnt, qboolean doPitch )
{
	vec3_t	enemy_eyes, eyes, angles;

	if ( aiEnt == NULL )
		return;

	if ( aiEnt->enemy == NULL )
		return;

	if ( aiEnt->client->ps.fd.forcePowersActive & (1<<FP_GRIP) &&
		aiEnt->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//don't update?
		aiEnt->NPC->desiredPitch = aiEnt->client->ps.viewangles[PITCH];
		aiEnt->NPC->desiredYaw = aiEnt->client->ps.viewangles[YAW];
		return;
	}
	CalcEntitySpot( aiEnt, SPOT_HEAD, eyes );

	CalcEntitySpot( aiEnt->enemy, SPOT_HEAD, enemy_eyes );

	if ( (NPC_IsBountyHunter(aiEnt) || aiEnt->hasJetpack)
		&& TIMER_Done( aiEnt, "flameTime" )
		&& aiEnt->s.weapon != WP_NONE
		&& !(WeaponIsSniperCharge(aiEnt->s.weapon) && Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin) >= 512.0)
		//&& (aiEnt->s.weapon != WP_ROCKET_LAUNCHER||!(aiEnt->NPC->scriptFlags&SCF_ALT_FIRE))
		&& aiEnt->s.weapon != WP_THERMAL
		&& aiEnt->s.weapon != WP_TRIP_MINE
		&& aiEnt->s.weapon != WP_DET_PACK )
	{//boba leads his enemy
		if ( aiEnt->enemy && aiEnt->enemy->client && aiEnt->health < aiEnt->client->pers.maxHealth*0.5f )
		{//lead
			float missileSpeed = WP_SpeedOfMissileForWeapon( aiEnt->s.weapon, ((qboolean)(aiEnt->NPC->scriptFlags&SCF_ALT_FIRE)) );
			if ( missileSpeed )
			{
				float eDist = Distance( eyes, enemy_eyes );
				eDist /= missileSpeed;//How many seconds it will take to get to the enemy
				VectorMA( enemy_eyes, eDist*flrand(0.95f,1.25f), aiEnt->enemy->client->ps.velocity, enemy_eyes );
			}
		}
	}

	//Find the desired angles
	if ( !aiEnt->client->ps.saberInFlight
		&& (aiEnt->client->ps.legsAnim == BOTH_A2_STABBACK1
			|| aiEnt->client->ps.legsAnim == BOTH_CROUCHATTACKBACK1
			|| aiEnt->client->ps.legsAnim == BOTH_ATTACK_BACK)
		)
	{//point *away*
		GetAnglesForDirection( enemy_eyes, eyes, angles );
	}
	else
	{//point towards him
		GetAnglesForDirection( eyes, enemy_eyes, angles );
	}

	aiEnt->NPC->desiredYaw	= AngleNormalize360( angles[YAW] );
	/*
	if ( NPC->client->ps.saberBlocked == BLOCKED_UPPER_LEFT )
	{//temp hack- to make up for poor coverage on left side
		NPCInfo->desiredYaw += 30;
	}
	*/

	if ( doPitch )
	{
		aiEnt->NPC->desiredPitch = AngleNormalize360( angles[PITCH] );
		if ( aiEnt->client->ps.saberInFlight )
		{//tilt down a little
			aiEnt->NPC->desiredPitch += 10;
		}
	}
	//FIXME: else desiredPitch = 0?  Or keep previous?
}

static void Jedi_DebounceDirectionChanges( gentity_t *aiEnt)
{
	//FIXME: check these before making fwd/back & right/left decisions?
	//Time-debounce changes in forward/back dir
	if ( aiEnt->client->pers.cmd.forwardmove > 0 )
	{
		if ( !TIMER_Done( aiEnt, "moveback" ) || !TIMER_Done( aiEnt, "movenone" ) )
		{
			aiEnt->client->pers.cmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( aiEnt->client->pers.cmd.rightmove > 0 )
			{
				if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, 127, qfalse ) )
					aiEnt->client->pers.cmd.rightmove = 127;
			}
			else if ( aiEnt->client->pers.cmd.rightmove < 0 )
			{
				if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, -127, qfalse ) )
					aiEnt->client->pers.cmd.rightmove = -127;
			}
			VectorClear( aiEnt->client->ps.moveDir );
			TIMER_Set( aiEnt, "moveback", -level.time );
			if ( TIMER_Done( aiEnt, "movenone" ) )
			{
				TIMER_Set( aiEnt, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( aiEnt, "moveforward" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( aiEnt, "moveforward", Q_irand( 500, 2000 ) );
		}
	}
	else if ( aiEnt->client->pers.cmd.forwardmove < 0 )
	{
		if ( !TIMER_Done( aiEnt, "moveforward" ) || !TIMER_Done( aiEnt, "movenone" ) )
		{
			aiEnt->client->pers.cmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( aiEnt->client->pers.cmd.rightmove > 0 )
			{
				if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, 127, qfalse ) )
					aiEnt->client->pers.cmd.rightmove = 127;
			}
			else if ( aiEnt->client->pers.cmd.rightmove < 0 )
			{
				if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, -127, qfalse ) )
					aiEnt->client->pers.cmd.rightmove = -127;
			}
			VectorClear( aiEnt->client->ps.moveDir );
			TIMER_Set( aiEnt, "moveforward", -level.time );
			if ( TIMER_Done( aiEnt, "movenone" ) )
			{
				TIMER_Set( aiEnt, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( aiEnt, "moveback" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( aiEnt, "moveback", Q_irand( 250, 1000 ) );
		}
	}
	else if ( !TIMER_Done( aiEnt, "moveforward" ) )
	{//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
		if ( NPC_MoveDirClear(aiEnt, 127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
			aiEnt->client->pers.cmd.forwardmove = 127;
		VectorClear( aiEnt->client->ps.moveDir );
	}
	else if ( !TIMER_Done( aiEnt, "moveback" ) )
	{//NOTE: edge checking should stop me if this is bad...
		if ( NPC_MoveDirClear(aiEnt , -127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
			aiEnt->client->pers.cmd.forwardmove = -127;
		VectorClear( aiEnt->client->ps.moveDir );
	}
	//Time-debounce changes in right/left dir
	if ( aiEnt->client->pers.cmd.rightmove > 0 )
	{
		if ( !TIMER_Done( aiEnt, "moveleft" ) || !TIMER_Done( aiEnt, "movecenter" ) )
		{
			aiEnt->client->pers.cmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( aiEnt->client->pers.cmd.forwardmove > 0 )
			{
				if ( NPC_MoveDirClear(aiEnt, 127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
					aiEnt->client->pers.cmd.forwardmove = 127;
			}
			else if ( aiEnt->client->pers.cmd.forwardmove < 0 )
			{
				if ( NPC_MoveDirClear(aiEnt , -127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
					aiEnt->client->pers.cmd.forwardmove = -127;
			}
			VectorClear( aiEnt->client->ps.moveDir );
			TIMER_Set( aiEnt, "moveleft", -level.time );
			if ( TIMER_Done( aiEnt, "movecenter" ) )
			{
				TIMER_Set( aiEnt, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( aiEnt, "moveright" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( aiEnt, "moveright", Q_irand( 250, 1500 ) );
		}
	}
	else if ( aiEnt->client->pers.cmd.rightmove < 0 )
	{
		if ( !TIMER_Done( aiEnt, "moveright" ) || !TIMER_Done( aiEnt, "movecenter" ) )
		{
			aiEnt->client->pers.cmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( aiEnt->client->pers.cmd.forwardmove > 0 )
			{
				if ( NPC_MoveDirClear(aiEnt, 127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
					aiEnt->client->pers.cmd.forwardmove = 127;
			}
			else if ( aiEnt->client->pers.cmd.forwardmove < 0 )
			{
				if ( NPC_MoveDirClear(aiEnt,  -127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
					aiEnt->client->pers.cmd.forwardmove = -127;
			}
			VectorClear( aiEnt->client->ps.moveDir );
			TIMER_Set( aiEnt, "moveright", -level.time );
			if ( TIMER_Done( aiEnt, "movecenter" ) )
			{
				TIMER_Set( aiEnt, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( aiEnt, "moveleft" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( aiEnt, "moveleft", Q_irand( 250, 1500 ) );
		}
	}
	else if ( !TIMER_Done( aiEnt, "moveright" ) )
	{//NOTE: edge checking should stop me if this is bad...
		if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, 127, qfalse ) )
			aiEnt->client->pers.cmd.rightmove = 127;
		VectorClear( aiEnt->client->ps.moveDir );
	}
	else if ( !TIMER_Done( aiEnt, "moveleft" ) )
	{//NOTE: edge checking should stop me if this is bad...
		if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, -127, qfalse ) )
			aiEnt->client->pers.cmd.rightmove = -127;
		VectorClear( aiEnt->client->ps.moveDir );
	}
}

static void Jedi_TimersApply( gentity_t *aiEnt)
{
	if ( aiEnt->client->pers.cmd.rightmove == 0 )
	{//only if not already strafing
		//FIXME: if enemy behind me and turning to face enemy, don't strafe in that direction, too
		if ( !TIMER_Done( aiEnt, "strafeLeft" ) )
		{
			if ( aiEnt->NPC->desiredYaw > aiEnt->client->ps.viewangles[YAW] + 60 )
			{//we want to turn left, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				//aiEnt->client->pers.cmd.rightmove = -127;
				if (NPC_MoveDirClear(aiEnt, 0, -127, qfalse ))
				{
					aiEnt->bot_strafe_left_timer = level.time + 50;
					VectorClear( aiEnt->client->ps.moveDir );
				}
			}
		}
		else if ( !TIMER_Done( aiEnt, "strafeRight" ) )
		{
			if ( aiEnt->NPC->desiredYaw < aiEnt->client->ps.viewangles[YAW] - 60 )
			{//we want to turn right, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				//aiEnt->client->pers.cmd.rightmove = 127;
				if (NPC_MoveDirClear(aiEnt, 0, 127, qfalse ))
				{
					aiEnt->bot_strafe_right_timer = level.time + 50;
					VectorClear( aiEnt->client->ps.moveDir );
				}
			}
		}
		else
		{
			aiEnt->client->pers.cmd.rightmove = 0;
		}
	}

	Jedi_DebounceDirectionChanges(aiEnt);

	//use careful anim/slower movement if not already moving
	if ( !aiEnt->client->pers.cmd.forwardmove && !TIMER_Done( aiEnt, "walking" ) )
	{
		aiEnt->client->pers.cmd.buttons |= (BUTTON_WALKING);
	}

	if ( !TIMER_Done( aiEnt, "taunting" ) )
	{
		aiEnt->client->pers.cmd.buttons |= (BUTTON_WALKING);
	}

	if ( !TIMER_Done( aiEnt, "gripping" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		aiEnt->client->pers.cmd.buttons |= BUTTON_FORCEGRIP;
	}

	if ( !TIMER_Done( aiEnt, "draining" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		aiEnt->client->pers.cmd.buttons |= BUTTON_FORCE_DRAIN;
	}

	if ( !TIMER_Done( aiEnt, "holdLightning" ) )
	{//hold down the lightning key
		aiEnt->client->pers.cmd.buttons |= BUTTON_FORCE_LIGHTNING;
	}
}

static void Jedi_CombatTimersUpdate( gentity_t *aiEnt, int enemy_dist )
{
	if( Jedi_CultistDestroyer( aiEnt ) )
	{
		Jedi_Aggression( aiEnt, 5 );
		return;
	}
	if ( TIMER_Done( aiEnt, "roamTime" ) )
	{
		TIMER_Set( aiEnt, "roamTime", Q_irand( 2000, 5000 ) );
		//okay, now mess with agression
		if ( aiEnt->client->ps.fd.forcePowersActive&(1<<FP_RAGE) )
		{//raging
			Jedi_Aggression( aiEnt, Q_irand( 0, 3 ) );
		}
		else if ( aiEnt->client->ps.fd.forceRageRecoveryTime > level.time )
		{//recovering
			Jedi_Aggression( aiEnt, Q_irand( 0, -2 ) );
		}
		if ( aiEnt->enemy && aiEnt->enemy->client )
		{
			switch( aiEnt->enemy->client->ps.weapon )
			{
			case WP_SABER:
				//If enemy has a lightsaber, always close in
				if ( BG_SabersOff( &aiEnt->enemy->client->ps ) )
				{//fool!  Standing around unarmed, charge!
					//Com_Printf( "(%d) raise agg - enemy saber off\n", level.time );
					Jedi_Aggression( aiEnt, 2 );
				}
				else
				{
					//Com_Printf( "(%d) raise agg - enemy saber\n", level.time );
					Jedi_Aggression( aiEnt, 1 );
				}
				break;
			default:
				break;
			}
		}
	}

	if ( TIMER_Done( aiEnt, "noStrafe" ) && TIMER_Done( aiEnt, "strafeLeft" ) && TIMER_Done( aiEnt, "strafeRight" ) )
	{
		//FIXME: Maybe more likely to do this if aggression higher?  Or some other stat?
		if ( !Q_irand( 0, 4 ) )
		{//start a strafe
			if ( Jedi_Strafe(aiEnt, 1000, 3000, 0, 4000, qtrue ) )
			{
				if ( d_JediAI.integer )
				{
					Com_Printf( "off strafe\n" );
				}
			}
		}
		else
		{//postpone any strafing for a while
			TIMER_Set( aiEnt, "noStrafe", Q_irand( 1000, 3000 ) );
		}
	}

	if ( aiEnt->client->ps.saberEventFlags )
	{//some kind of saber combat event is still pending
		int newFlags = aiEnt->client->ps.saberEventFlags;
		if ( aiEnt->client->ps.saberEventFlags&SEF_PARRIED )
		{//parried
			TIMER_Set( aiEnt, "parryTime", -1 );
			/*
			if ( NPCInfo->rank >= RANK_LT_JG )
			{
				NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 100;
			}
			else
			{
				NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			}
			*/
			if ( aiEnt->enemy && aiEnt->enemy->client && PM_SaberInKnockaway( aiEnt->enemy->client->ps.saberMove ) )
			{//advance!
				Jedi_Aggression( aiEnt, 1 );//get closer
				Jedi_AdjustSaberAnimLevel( aiEnt, (aiEnt->client->ps.fd.saberAnimLevel-1) );//use a faster attack
			}
			else
			{
				if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we parried\n", level.time );
					Jedi_Aggression( aiEnt, -1 );
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( aiEnt, (aiEnt->client->ps.fd.saberAnimLevel-1) );
				}
			}
			if ( d_JediAI.integer )
			{
				Com_Printf( "(%d) PARRY: agg %d, no parry until %d\n", level.time, aiEnt->NPC->stats.aggression, level.time + 100 );
			}
			newFlags &= ~SEF_PARRIED;
		}
		if ( !aiEnt->client->ps.weaponTime && (aiEnt->client->ps.saberEventFlags&SEF_HITENEMY) )//hit enemy
		{//we hit our enemy last time we swung, drop our aggression
			if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
			{
				//Com_Printf( "(%d) drop agg - we hit enemy\n", level.time );
				Jedi_Aggression( aiEnt, -1 );
				if ( d_JediAI.integer )
				{
					Com_Printf( "(%d) HIT: agg %d\n", level.time, aiEnt->NPC->stats.aggression );
				}
				if ( !Q_irand( 0, 3 )
					&& aiEnt->NPC->blockedSpeechDebounceTime < level.time
					&& jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time
					&& aiEnt->painDebounceTime < level.time - 1000 )
				{
					if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))//NPC_IsStormtrooper(aiEnt))
						ST_Speech( aiEnt, SPEECH_YELL, 0 );
					else
						G_AddVoiceEvent( aiEnt, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 10000 );

					jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
				}
			}
			if ( !Q_irand( 0, 2 ) )
			{
				Jedi_AdjustSaberAnimLevel( aiEnt, (aiEnt->client->ps.fd.saberAnimLevel+1) );
			}
			newFlags &= ~SEF_HITENEMY;
		}
		if ( (aiEnt->client->ps.saberEventFlags&SEF_BLOCKED) )
		{//was blocked whilst attacking
			if ( PM_SaberInBrokenParry( aiEnt->client->ps.saberMove )
				|| aiEnt->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
			{
				//Com_Printf( "(%d) drop agg - we were knock-blocked\n", level.time );
				if ( aiEnt->client->ps.saberInFlight )
				{//lost our saber, too!!!
					Jedi_Aggression( aiEnt, -5 );//really really really should back off!!!
				}
				else
				{
					Jedi_Aggression( aiEnt, -2 );//really should back off!
				}
				Jedi_AdjustSaberAnimLevel( aiEnt, (aiEnt->client->ps.fd.saberAnimLevel+1) );//use a stronger attack
				if ( d_JediAI.integer )
				{
					Com_Printf( "(%d) KNOCK-BLOCKED: agg %d\n", level.time, aiEnt->NPC->stats.aggression );
				}
			}
			else
			{
				if ( !Q_irand( 0, 2 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we were blocked\n", level.time );
					Jedi_Aggression( aiEnt, -1 );
					if ( d_JediAI.integer )
					{
						Com_Printf( "(%d) BLOCKED: agg %d\n", level.time, aiEnt->NPC->stats.aggression );
					}
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( aiEnt, (aiEnt->client->ps.fd.saberAnimLevel+1) );
				}
			}
			newFlags &= ~SEF_BLOCKED;
			//FIXME: based on the type of parry the enemy is doing and my skill,
			//		choose an attack that is likely to get around the parry?
			//		right now that's generic in the saber animation code, auto-picks
			//		a next anim for me, but really should be AI-controlled.
		}
		if ( aiEnt->client->ps.saberEventFlags&SEF_DEFLECTED )
		{//deflected a shot
			newFlags &= ~SEF_DEFLECTED;
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( aiEnt, (aiEnt->client->ps.fd.saberAnimLevel-1) );
			}
		}
		if ( aiEnt->client->ps.saberEventFlags&SEF_HITWALL )
		{//hit a wall
			newFlags &= ~SEF_HITWALL;
		}
		if ( aiEnt->client->ps.saberEventFlags&SEF_HITOBJECT )
		{//hit some other damagable object
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( aiEnt, (aiEnt->client->ps.fd.saberAnimLevel-1) );
			}
			newFlags &= ~SEF_HITOBJECT;
		}
		aiEnt->client->ps.saberEventFlags = newFlags;
	}
}

static void Jedi_CombatIdle( gentity_t *aiEnt, int enemy_dist )
{
	if ( !TIMER_Done( aiEnt, "parryTime" ) )
	{
		return;
	}
	if ( aiEnt->client->ps.saberInFlight )
	{//don't do this idle stuff if throwing saber
		return;
	}
	if ( aiEnt->client->ps.fd.forcePowersActive&(1<<FP_RAGE)
		|| aiEnt->client->ps.fd.forceRageRecoveryTime > level.time )
	{//never taunt while raging or recovering from rage
		return;
	}
	//FIXME: make these distance numbers defines?
	if ( enemy_dist >= 64 )
	{//FIXME: only do this if standing still?
		//based on aggression, flaunt/taunt
		int chance = 20;
		if ( aiEnt->client->NPC_class == CLASS_SHADOWTROOPER )
		{
			chance = 10;
		}
		//FIXME: possibly throw local objects at enemy?
		if ( Q_irand( 2, chance ) < aiEnt->NPC->stats.aggression )
		{
			if ( TIMER_Done( aiEnt, "chatter" ) && aiEnt->client->ps.forceHandExtend == HANDEXTEND_NONE )
			{//FIXME: add more taunt behaviors
				//FIXME: sometimes he turns it off, then turns it right back on again???
				if ( enemy_dist > 200
					&& aiEnt->client->ps.weapon == WP_SABER/*NPC_IsJedi(aiEnt)*/
					&& !aiEnt->client->ps.saberHolstered
					&& !Q_irand( 0, 5 ) )
				{//taunt even more, turn off the saber
					//FIXME: don't do this if health low?
					WP_DeactivateSaber( aiEnt, qfalse );
					//Don't attack for a bit
					aiEnt->NPC->stats.aggression = 3;
					//FIXME: maybe start strafing?
					//debounce this
					if ( aiEnt->client->playerTeam != NPCTEAM_PLAYER && !Q_irand( 0, 1 ))
					{
						//NPC->client->ps.taunting = level.time + 100;
						aiEnt->client->ps.forceHandExtend = HANDEXTEND_JEDITAUNT;
						aiEnt->client->ps.forceHandExtendTime = level.time + 5000;

						TIMER_Set( aiEnt, "chatter", Q_irand( 5000, 10000 ) );
						TIMER_Set( aiEnt, "taunting", 5500 );
					}
					else
					{
						Jedi_BattleTaunt(aiEnt);
						TIMER_Set( aiEnt, "taunting", Q_irand( 5000, 10000 ) );
					}
				}
				else if ( Jedi_BattleTaunt(aiEnt) )
				{//FIXME: pick some anims
				}
			}
		}
	}
}

static qboolean Jedi_AttackDecide( gentity_t *aiEnt, int enemy_dist )
{
	if (!aiEnt->enemy || !NPC_IsAlive(aiEnt, aiEnt->enemy))
	{
		return qfalse;
	}

	// Begin fixed cultist_destroyer AI
	if ( Jedi_CultistDestroyer( aiEnt ) )
	{ // destroyer
		if ( enemy_dist <= 32 )
		{//go boom!
			//float?
			//VectorClear( NPC->client->ps.velocity );
			//NPC->client->ps.gravity = 0;
			//NPC->svFlags |= SVF_CUSTOM_GRAVITY;
			//NPC->client->moveType = MT_FLYSWIM;
			//NPC->flags |= FL_NO_KNOCKBACK;
			aiEnt->flags |= FL_GODMODE;
			aiEnt->takedamage = qfalse;

			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
			aiEnt->client->ps.fd.forcePowersActive |= ( 1 << FP_RAGE );
			aiEnt->painDebounceTime = aiEnt->useDebounceTime = level.time + aiEnt->client->ps.torsoTimer;
			return qtrue;
		}
		return qfalse;
	}

	if ( aiEnt->enemy->client
		&& aiEnt->enemy->s.weapon == WP_SABER
		&& aiEnt->enemy->client->ps.saberLockTime > level.time
		&& aiEnt->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		return qfalse;
	}

	if ( aiEnt->client->ps.saberEventFlags & SEF_LOCK_WON )
	{//we won a saber lock, press the advantage with an attack!
		int	chance = 0;
		if ( aiEnt->client->NPC_class == CLASS_DESANN || aiEnt->client->NPC_class == CLASS_LUKE || !Q_stricmp("Yoda",aiEnt->NPC_type) )
		{//desann and luke
			chance = 20;
		}
		else if ( aiEnt->client->NPC_class == CLASS_TAVION )
		{//tavion
			chance = 10;
		}
		else if ( aiEnt->client->NPC_class == CLASS_REBORN && aiEnt->NPC->rank == RANK_LT_JG )
		{//fencer
			chance = 5;
		}
		else
		{
			chance = aiEnt->NPC->rank;
		}
		if ( Q_irand( 0, 30 ) < chance )
		{//based on skill with some randomness
			aiEnt->client->ps.saberEventFlags &= ~SEF_LOCK_WON;//clear this now that we are using the opportunity
			TIMER_Set( aiEnt, "noRetreat", Q_irand( 500, 2000 ) );
			//FIXME: check enemy_dist?
			aiEnt->client->ps.weaponTime = aiEnt->NPC->shotTime = aiEnt->attackDebounceTime = 0;
			//NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
			WeaponThink(aiEnt, qtrue );
			return qtrue;
		}
	}
	
	if ( aiEnt->s.eFlags & EF_FAKE_NPC_BOT ||
		/*aiEnt->client->NPC_class == CLASS_TAVION ||
		( aiEnt->client->NPC_class == CLASS_REBORN && aiEnt->NPC->rank == RANK_LT_JG ) ||
		( aiEnt->client->NPC_class == CLASS_JEDI && aiEnt->NPC->rank == RANK_COMMANDER ) ||
		( aiEnt->client->NPC_class == CLASS_PADAWAN && aiEnt->NPC->rank == RANK_COMMANDER )*/NPC_IsJedi(aiEnt) )
	{//tavion, fencers, jedi trainer are all good at following up a parry with an attack
		if ( ( PM_SaberInParry( aiEnt->client->ps.saberMove ) || PM_SaberInKnockaway( aiEnt->client->ps.saberMove ) )
			&& aiEnt->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//try to attack straight from a parry
			aiEnt->client->ps.weaponTime = aiEnt->NPC->shotTime = aiEnt->attackDebounceTime = 0;
			//NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
			Jedi_AdjustSaberAnimLevel( aiEnt, FORCE_LEVEL_1 );//try to follow-up with a quick attack
			WeaponThink(aiEnt, qtrue );
			return qtrue;
		}
	}

	//try to hit them if we can
	if ( enemy_dist >= 64 )
	{
		Jedi_Advance(aiEnt);
		
		if ( enemy_dist >= 128 )
			return qfalse;
	}

	if ( !TIMER_Done( aiEnt, "parryTime" ) )
	{
		return qfalse;
	}

	if ( (aiEnt->NPC->scriptFlags & SCF_DONT_FIRE) )
	{//not allowed to attack
		return qfalse;
	}

	if ( !(aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK) && !(aiEnt->client->pers.cmd.buttons & BUTTON_ALT_ATTACK) )
	{//not already attacking
		if (CanShoot (aiEnt->enemy, aiEnt ))
		{// UQ1: Umm, how about we actually check if we can hit them first???
			if (aiEnt->s.weapon == WP_SABER)
			{//Try to attack
				NPC_FaceEnemy(aiEnt, qtrue);

				if (!BG_SaberInAttack(aiEnt->client->ps.saberMove) && !Jedi_SaberBusy( aiEnt ))
				{
					if (NPC_GetHealthPercent( aiEnt, aiEnt ) < 30)
					{// Back away while attacking...
						int rand = irand(0,100);

						//JEDI_Debug(aiEnt, "heal");

						if (!NPC_IsJedi(aiEnt) && aiEnt->client->ps.weapon != WP_SABER)
						{// Jedi handle their own attack/retreats...
							Jedi_Retreat(aiEnt);
						}

						if (rand < 20) 
							aiEnt->client->pers.cmd.rightmove = 64;
						else if (rand < 40) 
							aiEnt->client->pers.cmd.rightmove = -64;

						if (!NPC_IsJedi(aiEnt))
						{// Jedi handle this their own way...
							if (TIMER_Done(aiEnt, "heal")
								&& aiEnt->client->ps.fd.forcePowerLevel[FP_HEAL] > 0
								&& Q_irand(0, 20) < 5)
							{
								//trap->Print("%s is using heal.\n", aiEnt->NPC_type);
								ForceHeal(aiEnt);
								TIMER_Set(aiEnt, "heal", irand(5000, 15000));
								return qtrue;
							}
							else if (TIMER_Done(aiEnt, "drain")
								&& aiEnt->client->ps.fd.forcePowerLevel[FP_DRAIN] > 0
								&& NPC_Jedi_EnemyInForceRange(aiEnt)
								&& Q_irand(0, 20) < 5)
							{
								//trap->Print("%s is using drain.\n", aiEnt->NPC_type);
								NPC_FaceEnemy(aiEnt, qtrue);
								ForceDrain(aiEnt);
								TIMER_Set(aiEnt, "drain", irand(5000, 15000));
								return qtrue;
							}
							else
							{
								WeaponThink(aiEnt, qtrue);
							}
						}
						else
						{
							WeaponThink(aiEnt, qtrue );
						}
					}
					else
					{
						int rand = irand(0,100);

						//JEDI_Debug(aiEnt, "attack");

						Jedi_Advance(aiEnt);

						// Sometimes move left/right while swinging saber to vary attacks...
						if (rand < 20) 
							aiEnt->client->pers.cmd.rightmove = 64;
						else if (rand < 40) 
							aiEnt->client->pers.cmd.rightmove = -64;

						WeaponThink(aiEnt, qtrue );

						if (rand > 95) 
						{// Do a kata occasionally...
							aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
							aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
						}
						else if (rand > 90) 
						{// Do a lunge, etc occasionally...
							aiEnt->client->pers.cmd.upmove = -127;
							aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
						}
					}
				}
			}
			else
			{//Try to attack
				WeaponThink(aiEnt, qtrue );
			}
		}
	}

	//FIXME:  Maybe try to push enemy off a ledge?

	//close enough to step forward

	//FIXME: an attack debounce timer other than the phaser debounce time?
	//		or base it on aggression?

	if ( aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK )
	{//attacking
#if 0
		if ( enemy_dist > 32 /*&& aiEnt->NPC->stats.aggression >= 4*/ )
		{//move forward if we're too far away and we're chasing him
			//ucmd.forwardmove = 127;
			Jedi_Advance(aiEnt);
		}
		else if ( enemy_dist < 0 )
		{//move back if we're too close
			//ucmd.forwardmove = -127;
			Jedi_Retreat(aiEnt);
		}
		
		//FIXME: based on the type of parry/attack the enemy is doing and my skill,
		//		choose an attack that is likely to get around the parry?
		//		right now that's generic in the saber animation code, auto-picks
		//		a next anim for me, but really should be AI-controlled.
		//FIXME: have this interact with/override above strafing code?
		if ( !aiEnt->client->pers.cmd.rightmove )
		{//not already strafing
			if ( !Q_irand( 0, 3 ) )
			{//25% chance of doing this
				vec3_t  right, dir2enemy;

				AngleVectors( aiEnt->r.currentAngles, NULL, right, NULL );
				VectorSubtract( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentAngles, dir2enemy );
				if ( DotProduct( right, dir2enemy ) > 0 )
				{//he's to my right, strafe left
					if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, -127, qfalse ) )
						aiEnt->client->pers.cmd.rightmove = -127;
					VectorClear( aiEnt->client->ps.moveDir );
				}
				else
				{//he's to my left, strafe right
					if ( NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, 127, qfalse ) )
						aiEnt->client->pers.cmd.rightmove = 127;
					VectorClear( aiEnt->client->ps.moveDir );
				}
			}
		}
#endif
		return qtrue;
	}

	//return qfalse;
	return qtrue;
}

#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f

qboolean Jedi_Jump( gentity_t *aiEnt, vec3_t dest, int goalEntNum )
{//FIXME: if land on enemy, knock him down & jump off again
	//
	// First type of jump...
	//

	if (aiEnt->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{// Already in the air... Wait...
		return qfalse;
	}

	if (!NPC_IsJedi(aiEnt) && Distance(dest, aiEnt->r.currentOrigin) > 256.0 && TIMER_Done( aiEnt, "emergencyJump" ))
	{// Too far for a non-jedi to jump...
		return qfalse;
	}

	if (aiEnt->nextJediJumpThink > level.time)
	{// Need to wait...
		return qfalse;
	}

	aiEnt->nextJediJumpThink = level.time + 5000;

	if (goalEntNum != ENTITYNUM_NONE)
	{
		gentity_t *goal = &g_entities[goalEntNum];

		if (!goal) 
		{// Invalid entity. Never jump there...
			return qfalse;
		}

		if (goal->s.eType == ET_NPC || goal->s.eType == ET_PLAYER)
		{
			if (goal->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{// Goal is in mid-air - Never jump there (we not end up where we want and most likely fall)...
				return qfalse;
			}
		}
	}

	if (aiEnt->client->ps.groundEntityNum != ENTITYNUM_NONE || !TIMER_Done( aiEnt, "emergencyJump" ))
	{
		float	targetDist, shotSpeed = 300, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed,
		vec3_t	targetDir, shotVel, failCase;
		trace_t	trace;
		trajectory_t	tr;
		qboolean	blocked;
		int		elapsedTime, timeStep = 500, hitCount = 0, maxHits = 7;
		vec3_t	lastPos, testPos;//, bottom;

		while ( hitCount < maxHits )
		{
			VectorSubtract( dest, aiEnt->r.currentOrigin, targetDir );
			targetDist = VectorNormalize( targetDir );

			VectorScale( targetDir, shotSpeed, shotVel );
			travelTime = targetDist/shotSpeed;
			shotVel[2] += travelTime * 0.5 * aiEnt->client->ps.gravity;

			if ( !hitCount )
			{//save the first one as the worst case scenario
				VectorCopy( shotVel, failCase );
			}

			if ( 1 )//tracePath )
			{//do a rough trace of the path
				blocked = qfalse;

				VectorCopy( aiEnt->r.currentOrigin, tr.trBase );
				VectorCopy( shotVel, tr.trDelta );
				tr.trType = TR_GRAVITY;
				tr.trTime = level.time;
				travelTime *= 1000.0f;
				VectorCopy( aiEnt->r.currentOrigin, lastPos );

				//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
				for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
				{
					if ( (float)elapsedTime > travelTime )
					{//cap it
						elapsedTime = floor( travelTime );
					}
					BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
					if ( testPos[2] < lastPos[2] )
					{//going down, ignore botclip
						trap->Trace( &trace, lastPos, aiEnt->r.mins, aiEnt->r.maxs, testPos, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );
					}
					else
					{//going up, check for botclip
						trap->Trace( &trace, lastPos, aiEnt->r.mins, aiEnt->r.maxs, testPos, aiEnt->s.number, aiEnt->clipmask|CONTENTS_BOTCLIP, qfalse, 0, 0 );
					}

					if ( trace.allsolid || trace.startsolid )
					{
						blocked = qtrue;
						break;
					}
					
					if ( trace.fraction < 1.0f )
					{//hit something
						/*if ( goalEntNum != ENTITYNUM_NONE && trace.entityNum == goalEntNum && NPC_CheckFallPositionOK(aiEnt, trace.endpos) )
						{//hit the enemy, that's perfect!
							//Hmm, don't want to land on him, though...
							break;
						}
						else*/ 
						if ( !TIMER_Done( aiEnt, "emergencyJump" ) )
						{// Accept anything solid if we are falling to our death!
							break;
						}
						else if ( goalEntNum != ENTITYNUM_NONE 
							&& (trace.entityNum == ENTITYNUM_NONE || trace.entityNum == ENTITYNUM_WORLD) 
							&& Distance( trace.endpos, dest ) <= 32/*128*//*96*/ 
							&& NPC_CheckFallPositionOK(aiEnt, trace.endpos) )
						{//hit the spot, that's perfect!
							break;
						}
						else if ( goalEntNum == ENTITYNUM_NONE 
							&& Distance( trace.endpos, dest ) <= 32/*128*//*96*/ 
							&& NPC_CheckFallPositionOK(aiEnt, trace.endpos) )
						{//hit the spot, that's perfect!
							break;
						}
						else
						{
							if ( trace.contents & CONTENTS_BOTCLIP )
							{//hit a do-not-enter brush
								blocked = qtrue;
								break;
							}
							if ( trace.plane.normal[2] > 0.7 && Distance( trace.endpos, dest ) < 32/*64*/ )//hit within 64 of desired location, should be okay
							{//close enough!
								break;
							}
							else
							{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
								impactDist = DistanceSquared( trace.endpos, dest );
								if ( impactDist < bestImpactDist )
								{
									bestImpactDist = impactDist;
									VectorCopy( shotVel, failCase );
								}
								blocked = qtrue;
								break;
							}
						}
					}

					if ( elapsedTime == floor( travelTime ) )
					{//reached end, all clear
						if ( trace.fraction >= 1.0f )
						{//hmm, make sure we'll land on the ground...
							/*
							//FIXME: do we care how far below ourselves or our dest we'll land?
							VectorCopy( trace.endpos, bottom );
							//bottom[2] -= 128;
							bottom[2] -= 64; // UQ1: Try less fall...
							trap->Trace( &trace, trace.endpos, aiEnt->r.mins, aiEnt->r.maxs, bottom, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );
							if ( trace.fraction >= 1.0f )
							{//would fall too far
								blocked = qtrue;
							}
							*/

							if (!NPC_CheckFallPositionOK(aiEnt, trace.endpos))
							{//would fall too far
								blocked = qtrue;
							}
						}
						break;
					}
					else
					{
						//all clear, try next slice
						VectorCopy( testPos, lastPos );
					}
				}

				if ( blocked )
				{//hit something, adjust speed (which will change arc)
					hitCount++;
					shotSpeed = 300 + ((hitCount-2) * 100);//from 100 to 900 (skipping 300)
					if ( hitCount >= 2 )
					{//skip 300 since that was the first value we tested
						shotSpeed += 100;
					}
				}
				else
				{//made it!
					break;
				}
			}
			else
			{//no need to check the path, go with first calc
				break;
			}
		}

		/*
		if ( hitCount >= maxHits )
		{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
			//NOTE: or try failcase?
			VectorCopy( failCase, aiEnt->client->ps.velocity );
		}
		VectorCopy( shotVel, aiEnt->client->ps.velocity );
		*/

		if ( hitCount < maxHits )
		{//NOTE: all good...
			VectorCopy( shotVel, aiEnt->client->ps.velocity );
			return qtrue;
		}
	}

	//
	// Try a second type of jump...
	//

	if (aiEnt->client->ps.groundEntityNum != ENTITYNUM_NONE || !TIMER_Done( aiEnt, "emergencyJump" ))
	{// Try the other type of jump in an emergency...
		float APEX = 512.0;

		if (!NPC_IsJedi(aiEnt) && TIMER_Done( aiEnt, "emergencyJump" ))
		{// Not a jedi and not an emergency...
			APEX = 256.0;
		}

		while (APEX >= APEX_HEIGHT)
		{//a more complicated jump
			vec3_t		dir, p1, p2, apex;
			float		time, height, forward, z, xy, dist, apexHeight;

			//float P_WIDTH = (sqrt(APEX)+sqrt(APEX));

			if ( aiEnt->r.currentOrigin[2] > dest[2] )//NPCInfo->goalEntity->r.currentOrigin
			{
				VectorCopy( aiEnt->r.currentOrigin, p1 );
				VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
			}
			else if ( aiEnt->r.currentOrigin[2] < dest[2] )//NPCInfo->goalEntity->r.currentOrigin
			{
				VectorCopy( dest, p1 );//NPCInfo->goalEntity->r.currentOrigin
				VectorCopy( aiEnt->r.currentOrigin, p2 );
			}
			else
			{
				VectorCopy( aiEnt->r.currentOrigin, p1 );
				VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
			}

			//z = xy*xy
			VectorSubtract( p2, p1, dir );
			dir[2] = 0;

			//Get xy and z diffs
			xy = VectorNormalize( dir );
			z = p1[2] - p2[2];

			apexHeight = APEX/*APEX_HEIGHT*//2;

			//Determine most desirable apex height
			//FIXME: length of xy will change curve of parabola, need to account for this
			//somewhere... PARA_WIDTH
			/*
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

			z = (sqrt(apexHeight + z) - sqrt(apexHeight));

			//assert(z >= 0);
			if (z <= 0)
			{
				//return qfalse;
				APEX -= 16;
				continue;
			}

			//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5/xy*100.0f);

			xy -= z;
			xy *= 0.5;

			//assert(xy > 0);
			if (xy <= 0) 
			{
				//return qfalse;
				APEX -= 16;
				continue;
			}

			VectorMA( p1, xy, dir, apex );
			apex[2] += apexHeight;

			VectorCopy(apex, aiEnt->pos1);

			//Now we have the apex, aim for it
			height = apex[2] - aiEnt->r.currentOrigin[2];
			time = sqrt( height / ( .5 * aiEnt->client->ps.gravity ) );//was 0.5, but didn't work well for very long jumps

			if ( time )
			{
				VectorSubtract ( apex, aiEnt->r.currentOrigin, aiEnt->client->ps.velocity );
				aiEnt->client->ps.velocity[2] = 0;
				dist = VectorNormalize( aiEnt->client->ps.velocity );

				forward = dist / time * 1.25;//er... probably bad, but...
				VectorScale( aiEnt->client->ps.velocity, forward, aiEnt->client->ps.velocity );

				//FIXME:  Uh.... should we trace/EvaluateTrajectory this to make sure we have clearance and we land where we want?
				aiEnt->client->ps.velocity[2] = time * aiEnt->client->ps.gravity;

				//Com_Printf("Jump Velocity: %4.2f, %4.2f, %4.2f\n", NPC->client->ps.velocity[0], NPC->client->ps.velocity[1], NPC->client->ps.velocity[2] );

				return qtrue;
			}

			// Failed, try next APEX height...
			APEX -= 16;
		}
	}

	//
	// Try a third type of jump...
	//

/*
	if (!TIMER_Done( aiEnt, "emergencyJump" ))
	{// Try the other type of jump in an emergency...
		//if (aiEnt->client->ps.groundEntityNum != ENTITYNUM_NONE)
		if ( dest[2] - aiEnt->r.currentOrigin[2] < 64 && DistanceHorizontal( aiEnt->r.currentOrigin, dest ) > 256 )
		{//a pretty horizontal jump, easy to fake:
			vec3_t enemy_diff;
			float enemy_z_diff, enemy_xy_diff;

			VectorSubtract( dest, aiEnt->r.currentOrigin, enemy_diff );
			enemy_z_diff = enemy_diff[2];
			enemy_diff[2] = 0;
			enemy_xy_diff = VectorNormalize( enemy_diff );

			VectorScale( enemy_diff, enemy_xy_diff*0.8, aiEnt->client->ps.velocity );

			if ( enemy_z_diff < 64 )
			{
				aiEnt->client->ps.velocity[2] = enemy_xy_diff;
			}
			else
			{
				aiEnt->client->ps.velocity[2] = enemy_z_diff*2+enemy_xy_diff/2;
			}

			return qtrue;
		}
	}
*/

	return qfalse;
}

static qboolean Jedi_TryJump( gentity_t *aiEnt, gentity_t *goal )
{//FIXME: never does a simple short, regular jump...
	//FIXME: I need to be on ground too!
	qboolean jumped = qfalse;

	if (NPC_EnemyAboveMe(aiEnt) && NPC_Jump( aiEnt, goal->r.currentOrigin ))
	{
		return qtrue;
	}

	if (aiEnt->s.weapon != WP_SABER)
		return qfalse;

	if ( (aiEnt->NPC->scriptFlags&SCF_NO_ACROBATICS) )
	{
		return qfalse;
	}
	if ( TIMER_Done( aiEnt, "jumpChaseDebounce" ) )
	{
		if ( (!goal->client || goal->client->ps.groundEntityNum != ENTITYNUM_NONE) )
		{
			if ( !PM_InKnockDown( &aiEnt->client->ps ) && !BG_InRoll( &aiEnt->client->ps, aiEnt->client->ps.legsAnim ) )
			{//enemy is on terra firma
				vec3_t goal_diff;
				float goal_z_diff;
				float goal_xy_dist;
				VectorSubtract( goal->r.currentOrigin, aiEnt->r.currentOrigin, goal_diff );
				goal_z_diff = goal_diff[2];
				goal_diff[2] = 0;
				goal_xy_dist = VectorNormalize( goal_diff );
				if ( goal_xy_dist < 550 && goal_z_diff > -400/*was -256*/ )//for now, jedi don't take falling damage && (NPC->health > 20 || goal_z_diff > 0 ) && (NPC->health >= 100 || goal_z_diff > -128 ))//closer than @512
				{
					qboolean debounce = qfalse;
					if ( aiEnt->health < 150 && ((aiEnt->health < 30 && goal_z_diff < 0) || goal_z_diff < -128 ) )
					{//don't jump, just walk off... doesn't help with ledges, though
						debounce = qtrue;
					}
					else if ( goal_z_diff < 32 && goal_xy_dist < 200 )
					{//what is their ideal jump height?
						aiEnt->client->pers.cmd.upmove = 127;
						debounce = qtrue;
					}
					else
					{
						/*
						//NO!  All Jedi can jump-navigate now...
						if ( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG )
						{//can't do acrobatics
							return qfalse;
						}
						*/
						if ( goal_z_diff > 0 || goal_xy_dist > 128 )
						{//Fake a force-jump
							//Screw it, just do my own calc & throw
							vec3_t dest;
							VectorCopy( goal->r.currentOrigin, dest );
							if ( goal == aiEnt->enemy )
							{
								int	sideTry = 0;
								while( sideTry < 10 )
								{//FIXME: make it so it doesn't try the same spot again?
									trace_t	trace;
									vec3_t	bottom;

									if ( Q_irand( 0, 1 ) )
									{
										dest[0] += aiEnt->enemy->r.maxs[0]*1.25;
									}
									else
									{
										dest[0] += aiEnt->enemy->r.mins[0]*1.25;
									}
									if ( Q_irand( 0, 1 ) )
									{
										dest[1] += aiEnt->enemy->r.maxs[1]*1.25;
									}
									else
									{
										dest[1] += aiEnt->enemy->r.mins[1]*1.25;
									}
									VectorCopy( dest, bottom );
									bottom[2] -= 128;
									trap->Trace( &trace, dest, aiEnt->r.mins, aiEnt->r.maxs, bottom, goal->s.number, aiEnt->clipmask, qfalse, 0, 0 );
									if ( trace.fraction < 1.0f )
									{//hit floor, okay to land here
										break;
									}
									sideTry++;
								}
								if ( sideTry >= 10 )
								{//screw it, just jump right at him?
									VectorCopy( goal->r.currentOrigin, dest );
								}
							}
							if ( Jedi_Jump(aiEnt, dest, goal->s.number ) )
							{
								//Com_Printf( "(%d) pre-checked force jump\n", level.time );

								//FIXME: store the dir we;re going in in case something gets in the way of the jump?
								//? = vectoyaw( NPC->client->ps.velocity );
								/*
								if ( NPC->client->ps.velocity[2] < 320 )
								{
									NPC->client->ps.velocity[2] = 320;
								}
								else
								*/
								{//FIXME: make this a function call
									int jumpAnim;
									//FIXME: this should be more intelligent, like the normal force jump anim logic
									if ( !NPC_IsJedi(aiEnt)
										||( aiEnt->NPC->rank != RANK_CREWMAN && aiEnt->NPC->rank <= RANK_LT_JG ) )
									{//can't do acrobatics
										jumpAnim = BOTH_FORCEJUMP1;
									}
									else
									{
										jumpAnim = BOTH_FLIP_F;
									}
									NPC_SetAnim( aiEnt, SETANIM_BOTH, jumpAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								}

								aiEnt->client->ps.fd.forceJumpZStart = aiEnt->r.currentOrigin[2];
								//NPC->client->ps.pm_flags |= PMF_JUMPING;

								aiEnt->client->ps.weaponTime = aiEnt->client->ps.torsoTimer;
								aiEnt->client->ps.fd.forcePowersActive |= ( 1 << FP_LEVITATION );

								if ( NPC_IsBountyHunter(aiEnt) || aiEnt->hasJetpack)
								{
									G_SoundOnEnt( aiEnt, CHAN_ITEM, "sound/boba/jeton.wav" );
									aiEnt->client->jetPackTime = level.time + Q_irand( 1000, 3000 );
								}
								else
								{
									G_SoundOnEnt( aiEnt, CHAN_BODY, "sound/weapons/force/jump.wav" );
								}

								TIMER_Set( aiEnt, "forceJumpChasing", Q_irand( 2000, 3000 ) );
								debounce = qtrue;
								jumped = qtrue;
							}
						}
					}
					if ( debounce )
					{
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( aiEnt, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
						if ( NPC_MoveDirClear(aiEnt, 127, aiEnt->client->pers.cmd.rightmove, qfalse ) )
							aiEnt->client->pers.cmd.forwardmove = 127;
						VectorClear( aiEnt->client->ps.moveDir );
						TIMER_Set( aiEnt, "duck", -level.time ); // UQ1: WTF?? -level.time????
						return qtrue;
					}
				}
			}
		}
	}

	if (jumped) return qtrue;

	return qfalse;
}

static qboolean Jedi_Jumping( gentity_t *aiEnt, gentity_t *goal )
{
	if ( !TIMER_Done( aiEnt, "forceJumpChasing" ) && goal )
	{//force-jumping at the enemy
//		if ( !(NPC->client->ps.pm_flags & PMF_JUMPING )//forceJumpZStart )
//			&& !(NPC->client->ps.pm_flags&PMF_TRIGGER_PUSHED))
		if (aiEnt->client->ps.groundEntityNum != ENTITYNUM_NONE) //rwwFIXMEFIXME: Not sure if this is gonna work, use the PM flags ideally.
		{//landed
			TIMER_Set( aiEnt, "forceJumpChasing", 0 );
		}
		else
		{
			NPC_FaceEntity(aiEnt, goal, qtrue );
			return qtrue;
		}
	}
	return qfalse;
}

extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir, vec3_t dest );

static void Jedi_CheckEnemyMovement( gentity_t *aiEnt, float enemy_dist )
{
	if ( !aiEnt->enemy || !aiEnt->enemy->client )
	{
		return;
	}

	if ( aiEnt->client->NPC_class != CLASS_TAVION
		&& aiEnt->client->NPC_class != CLASS_DESANN
		&& aiEnt->client->NPC_class != CLASS_LUKE
		&& Q_stricmp("Yoda",aiEnt->NPC_type) )
	{
		if ( aiEnt->enemy->enemy && aiEnt->enemy->enemy == aiEnt )
		{//enemy is mad at *me*
			if ( aiEnt->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1 ||
				aiEnt->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN )
			{//enemy is flipping over me
				if ( Q_irand( 0, aiEnt->NPC->rank ) < RANK_LT )
				{//be nice and stand still for him...
					aiEnt->client->pers.cmd.forwardmove = aiEnt->client->pers.cmd.rightmove = aiEnt->client->pers.cmd.upmove = 0;
					VectorClear( aiEnt->client->ps.moveDir );
					aiEnt->client->ps.fd.forceJumpCharge = 0;
					TIMER_Set( aiEnt, "strafeLeft", -1 );
					TIMER_Set( aiEnt, "strafeRight", -1 );
					TIMER_Set( aiEnt, "noStrafe", Q_irand( 500, 1000 ) );
					TIMER_Set( aiEnt, "movenone", Q_irand( 500, 1000 ) );
					TIMER_Set( aiEnt, "movecenter", Q_irand( 500, 1000 ) );
				}
			}
			else if ( aiEnt->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_BACK1
				|| aiEnt->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_RIGHT
				|| aiEnt->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_LEFT
				|| aiEnt->enemy->client->ps.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
				|| aiEnt->enemy->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP )
			{//he's flipping off a wall
				if ( aiEnt->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE )
				{//still in air
					if ( enemy_dist < 256 )
					{//close
						if ( Q_irand( 0, aiEnt->NPC->rank ) < RANK_LT )
						{//be nice and stand still for him...
							vec3_t enemyFwd, dest, dir;

							/*
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "noturn", Q_irand( 200, 500 ) );
							*/
							//stop current movement
							aiEnt->client->pers.cmd.forwardmove = aiEnt->client->pers.cmd.rightmove = aiEnt->client->pers.cmd.upmove = 0;
							VectorClear( aiEnt->client->ps.moveDir );
							aiEnt->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( aiEnt, "strafeLeft", -1 );
							TIMER_Set( aiEnt, "strafeRight", -1 );
							TIMER_Set( aiEnt, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( aiEnt, "noturn", Q_irand( 250, 500 )*(3-g_npcspskill.integer) );

							VectorCopy( aiEnt->enemy->client->ps.velocity, enemyFwd );
							VectorNormalize( enemyFwd );
							VectorMA( aiEnt->enemy->r.currentOrigin, -64, enemyFwd, dest );
							VectorSubtract( dest, aiEnt->r.currentOrigin, dir );
							if ( VectorNormalize( dir ) > 32 )
							{
								G_UcmdMoveForDir( aiEnt, &aiEnt->client->pers.cmd, dir, dest );
							}
							else
							{
								TIMER_Set( aiEnt, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( aiEnt, "movecenter", Q_irand( 500, 1000 ) );
							}
						}
					}
				}
			}
			else if ( aiEnt->enemy->client->ps.legsAnim == BOTH_A2_STABBACK1 )
			{//he's stabbing backwards
				if ( enemy_dist < 256 && enemy_dist > 64 )
				{//close
					if ( !InFront( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin, aiEnt->enemy->r.currentAngles, 0.0f ) )
					{//behind him
						if ( !Q_irand( 0, aiEnt->NPC->rank ) )
						{//be nice and stand still for him...
							vec3_t enemyFwd, dest, dir;

							//stop current movement
							aiEnt->client->pers.cmd.forwardmove = aiEnt->client->pers.cmd.rightmove = aiEnt->client->pers.cmd.upmove = 0;
							VectorClear( aiEnt->client->ps.moveDir );
							aiEnt->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( aiEnt, "strafeLeft", -1 );
							TIMER_Set( aiEnt, "strafeRight", -1 );
							TIMER_Set( aiEnt, "noStrafe", Q_irand( 500, 1000 ) );

							AngleVectors( aiEnt->enemy->r.currentAngles, enemyFwd, NULL, NULL );
							VectorMA( aiEnt->enemy->r.currentOrigin, -32, enemyFwd, dest );
							VectorSubtract( dest, aiEnt->r.currentOrigin, dir );
							if ( VectorNormalize( dir ) > 64 )
							{
								G_UcmdMoveForDir( aiEnt, &aiEnt->client->pers.cmd, dir, dest );
							}
							else
							{
								TIMER_Set( aiEnt, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( aiEnt, "movecenter", Q_irand( 500, 1000 ) );
							}
						}
					}
				}
			}
		}
	}
	//FIXME: also:
	//		If enemy doing wall flip, keep running forward
	//		If enemy doing back-attack and we're behind him keep running forward toward his back, don't strafe
}

static void Jedi_CheckJumps( gentity_t *aiEnt)
{
	vec3_t	jumpVel;
	trace_t	trace;
	trajectory_t	tr;
	vec3_t	lastPos, testPos, bottom;
	int		elapsedTime;

	if ( (aiEnt->NPC->scriptFlags&SCF_NO_ACROBATICS) )
	{
		aiEnt->client->ps.fd.forceJumpCharge = 0;
		aiEnt->client->pers.cmd.upmove = 0;
		return;
	}
	//FIXME: should probably check this before AI decides that best move is to jump?  Otherwise, they may end up just standing there and looking dumb
	//FIXME: all this work and he still jumps off ledges... *sigh*... need CONTENTS_BOTCLIP do-not-enter brushes...?
	VectorClear(jumpVel);

	if ( aiEnt->client->ps.fd.forceJumpCharge )
	{
		//Com_Printf( "(%d) force jump\n", level.time );
		WP_GetVelocityForForceJump( aiEnt, jumpVel, &aiEnt->client->pers.cmd );
	}
	else if ( aiEnt->client->pers.cmd.upmove > 0 )
	{
		//Com_Printf( "(%d) regular jump\n", level.time );
		VectorCopy( aiEnt->client->ps.velocity, jumpVel );
		jumpVel[2] = JUMP_VELOCITY;
	}
	else
	{
		return;
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( !jumpVel[0] && !jumpVel[1] )//FIXME: && !ucmd.forwardmove && !ucmd.rightmove?
	{//we assume a jump straight up is safe
		//Com_Printf( "(%d) jump straight up is safe\n", level.time );
		return;
	}
	//Now predict where this is going
	//in steps, keep evaluating the trajectory until the new z pos is <= than current z pos, trace down from there

	VectorCopy( aiEnt->r.currentOrigin, tr.trBase );
	VectorCopy( jumpVel, tr.trDelta );
	tr.trType = TR_GRAVITY;
	tr.trTime = level.time;
	VectorCopy( aiEnt->r.currentOrigin, lastPos );

	VectorClear(trace.endpos); //shut the compiler up

	//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
	for ( elapsedTime = 500; elapsedTime <= 4000; elapsedTime += 500 )
	{
		BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
		//FIXME: account for PM_AirMove if ucmd.forwardmove and/or ucmd.rightmove is non-zero...
		if ( testPos[2] < lastPos[2] )
		{//going down, don't check for BOTCLIP
			trap->Trace( &trace, lastPos, aiEnt->r.mins, aiEnt->r.maxs, testPos, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );//FIXME: include CONTENTS_BOTCLIP?
		}
		else
		{//going up, check for BOTCLIP
			trap->Trace( &trace, lastPos, aiEnt->r.mins, aiEnt->r.maxs, testPos, aiEnt->s.number, aiEnt->clipmask|CONTENTS_BOTCLIP, qfalse, 0, 0 );
		}
		if ( trace.allsolid || trace.startsolid )
		{//WTF?
			//FIXME: what do we do when we start INSIDE the CONTENTS_BOTCLIP?  Do the trace again without that clipmask?
			goto jump_unsafe;
		}
		if ( trace.fraction < 1.0f )
		{//hit something
			if ( trace.contents & CONTENTS_BOTCLIP )
			{//hit a do-not-enter brush
				goto jump_unsafe;
			}
			//FIXME: trace through func_glass?
			break;
		}
		VectorCopy( testPos, lastPos );
	}
	//okay, reached end of jump, now trace down from here for a floor
	VectorCopy( trace.endpos, bottom );
	if ( bottom[2] > aiEnt->r.currentOrigin[2] )
	{//only care about dist down from current height or lower
		bottom[2] = aiEnt->r.currentOrigin[2];
	}
	else if ( aiEnt->r.currentOrigin[2] - bottom[2] > 400 )
	{//whoa, long drop, don't do it!
		//probably no floor at end of jump, so don't jump
		goto jump_unsafe;
	}
	bottom[2] -= 128;
	trap->Trace( &trace, trace.endpos, aiEnt->r.mins, aiEnt->r.maxs, bottom, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0 );
	if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0f )
	{//hit ground!
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{//landed on an ent
			gentity_t *groundEnt = &g_entities[trace.entityNum];
			if ( groundEnt->r.svFlags&SVF_GLASS_BRUSH )
			{//don't land on breakable glass!
				goto jump_unsafe;
			}
		}
		//Com_Printf( "(%d) jump is safe\n", level.time );
		return;
	}
jump_unsafe:
	//probably no floor at end of jump, so don't jump
	//Com_Printf( "(%d) unsafe jump cleared\n", level.time );
	aiEnt->client->ps.fd.forceJumpCharge = 0;
	aiEnt->client->pers.cmd.upmove = 0;
}

extern void ST_TrackEnemy( gentity_t *self, vec3_t enemyPos );
extern qboolean NPC_FollowEnemyRoute(gentity_t *aiEnt);

static void Jedi_Combat( gentity_t *aiEnt)
{
	vec3_t		enemy_dir, enemy_movedir, enemy_dest;
	float		enemy_dist, enemy_movespeed;
	qboolean	enemy_lost = qfalse;
	qboolean	haveEnemy = qfalse;
	qboolean	enemyJumping = qfalse;

	if (aiEnt->enemy && NPC_IsAlive(aiEnt, aiEnt->enemy))
	{
		haveEnemy = qtrue;

		if (aiEnt->enemy->s.groundEntityNum == ENTITYNUM_NONE)
		{
			enemyJumping = qtrue;
		}
	}

	if (!haveEnemy)
	{
		return;
	}

	//See where enemy will be 300 ms from now
	Jedi_SetEnemyInfo(aiEnt, enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );

	if (enemy_dist > 256)
	{
		if (NPC_EnemyAboveMe(aiEnt) && !enemyJumping && NPC_Jump( aiEnt, enemy_dest ))
		{
			//JEDI_Debug(aiEnt, "Jump.");
			return;
		}
	}

	if ( Jedi_Jumping(aiEnt, aiEnt->enemy ) )
	{//I'm in the middle of a jump, so just see if I should attack
		Jedi_AttackDecide(aiEnt, enemy_dist );
		//JEDI_Debug(aiEnt, "In Jump.");
		return;
	}

	//If we can't get straight at him
#ifndef __USE_NAVLIB__
	if ( enemy_dist > 256 && !Jedi_ClearPathToSpot(aiEnt, enemy_dest, aiEnt->enemy->s.number ))
	{//hunt him down
		//Com_Printf( "No Clear Path\n" );
		if ( (NPC_ClearLOS4(aiEnt, aiEnt->enemy )||aiEnt->NPC->enemyLastSeenTime>level.time-500) && NPC_FaceEnemy(aiEnt, qtrue ) )//( NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG ) &&
		{
			if ( !enemyJumping && Jedi_TryJump(aiEnt, aiEnt->enemy ) )
			{//FIXME: what about jumping to his enemyLastSeenLocation?
				//trap->Print("JUMP!\n");
				aiEnt->longTermGoal = -1;
				//JEDI_Debug(aiEnt, "Jump 2.");
				return;
			}
		}

		//Check for evasion
		if ( TIMER_Done( aiEnt, "parryTime" ) )
		{//finished parrying
			if ( aiEnt->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
				aiEnt->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
			{//wasn't blocked myself
				aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
			}
		}

		//
		// UQ1: Screw that raven code, let's use waypointing to hunt them down...
		//

		aiEnt->NPC->goalEntity = aiEnt->enemy;

		if (NPC_FollowEnemyRoute(aiEnt))
		{
			//trap->Print("ROUTE!\n");

			if ( enemy_dist < 512 && !Q_irand( 0, 10 ) && aiEnt->NPC->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time && !NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
			{
				if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))
					ST_Speech( aiEnt, SPEECH_LOST, 0 );
				else
					G_AddVoiceEvent( aiEnt, Q_irand( EV_JLOST1, EV_JLOST3 ), 10000 );

				jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
			}

			//JEDI_Debug(aiEnt, "Pathing.");
			return;
		}
		else
		{
			//trap->Print("FAILED ROUTE!\n");

			NPC_ClearPathData(aiEnt);

			if (Jedi_Track(aiEnt) || Jedi_Hunt(aiEnt))
			{
				//JEDI_Debug(aiEnt, "Backup Pathing.");
				return;
			}
		}
	}
#endif //__USE_NAVLIB__


	//else, we can see him or we can't track him at all

	//every few seconds, decide if we should we advance or retreat?
	Jedi_CombatTimersUpdate(aiEnt, enemy_dist );

	//We call this even if lost enemy to keep him moving and to update the taunting behavior
	//maintain a distance from enemy appropriate for our aggression level
	Jedi_CombatDistance(aiEnt, enemy_dist );

	if (!(aiEnt->enemy && NPC_IsAlive(aiEnt, aiEnt->enemy))) return;

	if ( !enemy_lost )
	{
		//Update our seen enemy position
		if ( !aiEnt->enemy->client || ( aiEnt->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE && aiEnt->client->ps.groundEntityNum != ENTITYNUM_NONE ) )
		{
			VectorCopy( aiEnt->enemy->r.currentOrigin, aiEnt->NPC->enemyLastSeenLocation );
		}

		aiEnt->NPC->enemyLastSeenTime = level.time;
	}

	//Turn to face the enemy
	if ( TIMER_Done( aiEnt, "noturn" ) )
	{
		Jedi_FaceEnemy(aiEnt, qtrue );
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	//Check for evasion
	if ( TIMER_Done( aiEnt, "parryTime" ) )
	{//finished parrying
		if ( aiEnt->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			aiEnt->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}

	// UQ1: Replaced by a more intensive check... This is used by most NPCs now...
	NPC_CheckEvasion(aiEnt);

	//apply strafing/walking timers, etc.
	Jedi_TimersApply(aiEnt);

	if ( aiEnt->client->ps.weapon != WP_SABER )
	{
		Boba_FireDecide(aiEnt);
	}
	else if ( !aiEnt->client->ps.saberInFlight )
	{//not throwing saber or using force grip
		qboolean attacked = qfalse;

		if ( aiEnt->enemy 
			&& NPC_IsAlive(aiEnt, aiEnt->enemy)
			&& Distance(aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin) <= 64 
			&& (aiEnt->client->ps.weapon == WP_SABER || aiEnt->client->NPC_class == CLASS_BOBAFETT)
			&& !(aiEnt->client->NPC_class == CLASS_BOBAFETT && (!TIMER_Done( aiEnt, "nextAttackDelay" ) || !TIMER_Done( aiEnt, "flameTime" )))
			&& aiEnt->next_kick_time <= level.time
			&& irand(0,100) > 75)
		{// Close range - switch to melee... KICK!
			//G_Printf("Debug: NPC kicking...\n");
			aiEnt->NPC->scriptFlags &= ~SCF_ALT_FIRE;
			NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_A7_KICK_F, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD ); // UQ1: Better anim?????
			WP_FireMelee( aiEnt, qfalse );

			if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))
				ST_Speech( aiEnt, SPEECH_OUTFLANK, 0 );
			else
				G_AddVoiceEvent( aiEnt, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 10000 );

			aiEnt->next_rifle_butt_time = level.time + 10000;
			aiEnt->next_kick_time = level.time + 15000;

			if (aiEnt->enemy && NPC_IsAlive(aiEnt, aiEnt->enemy) && irand(0, 100) <= 25)
			{// 25% of the time, knock them over...
				int desiredAnim = 0;

				vec3_t	smackDir;
				VectorSubtract(aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, smackDir);
				smackDir[2] += 30;
				VectorNormalize(smackDir);

				//hurt them
				G_Damage(aiEnt->enemy, aiEnt, aiEnt, smackDir, aiEnt->r.currentOrigin, (g_npcspskill.integer + 1)*Q_irand(5, 10), DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_CRUSH);

				//throw them
				G_Throw(aiEnt->enemy, smackDir, 64);

				switch (irand(0, 16))
				{
				case 1: desiredAnim = BOTH_DEATH12; break;
				case 2: desiredAnim = BOTH_DEATH14; break;
				case 3: desiredAnim = BOTH_DEATH16; break;
				case 4: desiredAnim = BOTH_DEATH22; break;
				case 5: desiredAnim = BOTH_DEATH23; break;
				case 6: desiredAnim = BOTH_DEATH24; break;
				case 7: desiredAnim = BOTH_DEATH25; break;
				case 8: desiredAnim = BOTH_DEATH4; break;
				case 9: desiredAnim = BOTH_DEATH5; break;
				case 10: desiredAnim = BOTH_DEATH8; break;
				case 11: desiredAnim = BOTH_DEATHBACKWARD1; break;
				case 12: desiredAnim = BOTH_DEATHBACKWARD2; break;
				case 13: desiredAnim = BOTH_DEATHFORWARD3; break;
				case 14: desiredAnim = BOTH_KNOCKDOWN1; break;
				case 15: desiredAnim = BOTH_KNOCKDOWN2; break;
				case 16: desiredAnim = BOTH_KNOCKDOWN3; break;
				default: desiredAnim = BOTH_KNOCKDOWN1; break;
				}

				//make them backflip
				NPC_SetAnim(aiEnt->enemy, SETANIM_BOTH, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD); // this will be the bad anim. not sure what to replace it with
			}

			attacked = qtrue;
		}

		if (!attacked)
		{
			attacked = Jedi_AttackDecide(aiEnt, enemy_dist );
		}

		//if (attacked) JEDI_Debug(aiEnt, "ATTACKED!!!");

		//see if we can attack
		if ( !attacked )
		{//we're not attacking, decide what else to do
			Jedi_CombatIdle(aiEnt, enemy_dist );
			//FIXME: lower aggression when actually strike offensively?  Or just when do damage?
			//Jedi_Advance();
		}
		else
		{//we are attacking
			//stop taunting
			TIMER_Set( aiEnt, "taunting", -level.time );
		}
	}

	//Check for certain enemy special moves
	Jedi_CheckEnemyMovement(aiEnt, enemy_dist );

	//Make sure that we don't jump off ledges over long drops
	Jedi_CheckJumps(aiEnt);

#ifndef __USE_NAVLIB__
	//Just make sure we don't strafe into walls or off cliffs
	if ( !NPC_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, aiEnt->client->pers.cmd.rightmove, qtrue ) )
	{//uh-oh, we are going to fall or hit something
		navInfo_t	info;
		//Get the move info
		NAV_GetLastMove( &info );
		if ( !(info.flags & NIF_MACRO_NAV) )
		{//micro-navigation told us to step off a ledge, try using macronav for now
			NPC_CombatMoveToGoal(aiEnt, qfalse, qfalse );
		}
		//reset the timers.
		TIMER_Set( aiEnt, "strafeLeft", 0 );
		TIMER_Set( aiEnt, "strafeRight", 0 );
	}
#endif //__USE_NAVLIB__
}

/*
==========================================================================================
EXTERNALLY CALLED BEHAVIOR STATES
==========================================================================================
*/

/*
-------------------------
NPC_Jedi_Pain
-------------------------
*/

void NPC_Jedi_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	gentity_t *other = attacker;
	vec3_t point;

	VectorCopy(gPainPoint, point);

	//FIXME: base the actual aggression add/subtract on health?
	//FIXME: don't do this more than once per frame?
	//FIXME: when take pain, stop force gripping....?
	if ( other->s.weapon == WP_SABER )
	{//back off
		TIMER_Set( self, "parryTime", -1 );
		if ( self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",self->NPC_type) )
		{//less for Desann
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_npcspskill.integer)*50;
		}
		else if ( self->NPC->rank >= RANK_LT_JG )
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_npcspskill.integer)*100;//300
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_npcspskill.integer)*200;//500
		}
		if ( !Q_irand( 0, 3 ) )
		{//ouch... maybe switch up which saber power level we're using
			Jedi_AdjustSaberAnimLevel( self, Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 ) );
		}
		if ( !Q_irand( 0, 1 ) )//damage > 20 || self->health < 40 ||
		{
			//Com_Printf( "(%d) drop agg - hit by saber\n", level.time );
			Jedi_Aggression( self, -1 );
		}
		if ( d_JediAI.integer )
		{
			Com_Printf( "(%d) PAIN: agg %d, no parry until %d\n", level.time, self->NPC->stats.aggression, level.time+500 );
		}
		//for testing only
		// Figure out what quadrant the hit was in.
		if ( d_JediAI.integer )
		{
			vec3_t	diff, fwdangles, right;
			float rightdot, zdiff;

			VectorSubtract( point, self->client->renderInfo.eyePoint, diff );
			diff[2] = 0;
			fwdangles[1] = self->client->ps.viewangles[1];
			AngleVectors( fwdangles, NULL, right, NULL );
			rightdot = DotProduct(right, diff);
			zdiff = point[2] - self->client->renderInfo.eyePoint[2];

			Com_Printf( "(%d) saber hit at height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, point[2]-self->r.absmin[2],zdiff,rightdot);
		}
	}
	else
	{//attack
		//Com_Printf( "(%d) raise agg - hit by ranged\n", level.time );
		Jedi_Aggression( self, 1 );
	}

	self->NPC->enemyCheckDebounceTime = 0;

	WP_ForcePowerStop( self, FP_GRIP );

	//NPC_Pain( self, inflictor, other, point, damage, mod );
	NPC_Pain(self, attacker, damage);

	if ( !damage && self->health > 0 )
	{//FIXME: better way to know I was pushed
		if (!NPC_IsJedi(self) && !NPC_IsBountyHunter(self))//NPC_IsStormtrooper(self))
			ST_Speech( self, SPEECH_PUSHED, 0.7f );
		else
			G_AddVoiceEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 10000 );
	}

	//drop me from the ceiling if I'm on it
	if ( Jedi_WaitingAmbush( self ) )
	{
		self->client->noclip = qfalse;
	}
	if ( self->client->ps.legsAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_LEGS, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	if ( self->client->ps.torsoAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
}

qboolean Jedi_CheckDanger( gentity_t *aiEnt)
{
	int alertEvent = NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qfalse, AEL_MINOR );

	if ( level.alertEvents[alertEvent].level >= AEL_DANGER )
	{//run away!
		if ( !level.alertEvents[alertEvent].owner
			|| !level.alertEvents[alertEvent].owner->client
			|| (level.alertEvents[alertEvent].owner!=aiEnt&&level.alertEvents[alertEvent].owner->client->playerTeam!=aiEnt->client->playerTeam) )
		{//no owner
			return qfalse;
		}
		G_SetEnemy( aiEnt, level.alertEvents[alertEvent].owner );
		aiEnt->NPC->enemyLastSeenTime = level.time;
		TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_CheckAmbushPlayer( gentity_t *aiEnt)
{
	int i = 0;
	gentity_t *player;
	float target_dist;
	float zDiff;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		player = &g_entities[i];

		if (player == aiEnt) continue;

		if ( !player || !player->client )
		{
			continue;
		}

		if ( !NPC_ValidEnemy(aiEnt, player ) )
		{
			continue;
		}

//		if ( NPC->client->ps.powerups[PW_CLOAKED] || g_crosshairEntNum != NPC->s.number )
		if (aiEnt->client->ps.powerups[PW_CLOAKED] || !NPC_SomeoneLookingAtMe(aiEnt)) //rwwFIXMEFIXME: Need to pay attention to who is under crosshair for each player or something.
		{//if I'm not cloaked and the player's crosshair is on me, I will wake up, otherwise do this stuff down here...
			if ( !trap->InPVS( player->r.currentOrigin, aiEnt->r.currentOrigin ) )
			{//must be in same room
				continue;
			}
			else
			{
				if ( !aiEnt->client->ps.powerups[PW_CLOAKED] )
				{
					NPC_SetLookTarget( aiEnt, 0, 0 );
				}
			}
			zDiff = aiEnt->r.currentOrigin[2]-player->r.currentOrigin[2];
			if ( zDiff <= 0 || zDiff > 512 )
			{//never ambush if they're above me or way way below me
				continue;
			}

			//If the target is this close, then wake up regardless
			if ( (target_dist = DistanceHorizontalSquared( player->r.currentOrigin, aiEnt->r.currentOrigin )) > 4096 )
			{//closer than 64 - always ambush
				if ( target_dist > 147456 )
				{//> 384, not close enough to ambush
					continue;
				}
				//Check FOV first
				if ( aiEnt->client->ps.powerups[PW_CLOAKED] )
				{
					if ( InFOV( player, aiEnt, 30, 90 ) == qfalse )
					{
						continue;
					}
				}
				else
				{
					if ( InFOV( player, aiEnt, 45, 90 ) == qfalse )
					{
						continue;
					}
				}
			}

			if ( !NPC_ClearLOS4(aiEnt, player ) )
			{
				continue;
			}
		}

		//Got him, return true;
		G_SetEnemy( aiEnt, player );
		aiEnt->NPC->enemyLastSeenTime = level.time;
		TIMER_Set( aiEnt, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}

	//Didn't get anyone.
	return qfalse;
}

void Jedi_Ambush( gentity_t *self )
{
	self->client->noclip = qfalse;
//	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
	NPC_SetAnim( self, SETANIM_BOTH, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	self->client->ps.weaponTime = self->client->ps.torsoTimer; //NPC->client->ps.torsoTimer; //what the?
	if ( self->client->ps.weapon == WP_SABER/*NPC_IsJedi(self)*/ )
	{
		WP_ActivateSaber(self);
	}
	Jedi_Decloak( self );
	G_AddVoiceEvent( self, Q_irand( EV_ANGER1, EV_ANGER3 ), 1000 );
}

qboolean Jedi_WaitingAmbush( gentity_t *self )
{
	if ( (self->spawnflags&JSF_AMBUSH) && self->client->noclip )
	{
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Jedi_Patrol
-------------------------
*/

static void Jedi_Patrol( gentity_t *aiEnt)
{
	aiEnt->client->ps.saberBlocked = BLOCKED_NONE;

	if ( Jedi_WaitingAmbush( aiEnt ) )
	{//hiding on the ceiling
		NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_CEILING_CLING, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		if ( aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{//look for enemies
			if ( Jedi_CheckAmbushPlayer(aiEnt) || Jedi_CheckDanger(aiEnt) )
			{//found him!
				Jedi_Ambush( aiEnt );
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
		}
	}
	else if ( aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES )//NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{//look for enemies
		gentity_t *best_enemy = NULL;
		float	best_enemy_dist = Q3_INFINITE;
		int i;
		for ( i = 0; i < ENTITYNUM_WORLD; i++ )
		{
			gentity_t *enemy = &g_entities[i];
			float	enemy_dist;
			if ( enemy && enemy->client && NPC_ValidEnemy(aiEnt, enemy ) && enemy->client->playerTeam == aiEnt->client->enemyTeam )
			{
				if ( trap->InPVS( aiEnt->r.currentOrigin, enemy->r.currentOrigin ) )
				{//we could potentially see him
					enemy_dist = DistanceSquared( aiEnt->r.currentOrigin, enemy->r.currentOrigin );
					if ( enemy->s.eType == ET_PLAYER || enemy_dist < best_enemy_dist )
					{
						//if the enemy is close enough, or threw his saber, take him as the enemy
						//FIXME: what if he throws a thermal detonator?
						if ( enemy_dist < (220*220) || ( aiEnt->NPC->investigateCount>= 3 && !aiEnt->client->ps.saberHolstered ) )
						{
							G_SetEnemy( aiEnt, enemy );
							//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
							aiEnt->NPC->stats.aggression = 3;
							break;
						}
						else if ( enemy->client->ps.saberInFlight && !enemy->client->ps.saberHolstered )
						{//threw his saber, see if it's heading toward me and close enough to consider a threat
							float	saberDist;
							vec3_t	saberDir2Me;
							vec3_t	saberMoveDir;
							gentity_t *saber = &g_entities[enemy->client->ps.saberEntityNum];
							VectorSubtract( aiEnt->r.currentOrigin, saber->r.currentOrigin, saberDir2Me );
							saberDist = VectorNormalize( saberDir2Me );
							VectorCopy( saber->s.pos.trDelta, saberMoveDir );
							VectorNormalize( saberMoveDir );
							if ( DotProduct( saberMoveDir, saberDir2Me ) > 0.5 )
							{//it's heading towards me
								if ( saberDist < 200 )
								{//incoming!
									G_SetEnemy( aiEnt, enemy );
									//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
									aiEnt->NPC->stats.aggression = 3;
									break;
								}
							}
						}
						best_enemy_dist = enemy_dist;
						best_enemy = enemy;
					}
				}
			}
		}
		if ( !aiEnt->enemy )
		{//still not mad
			if ( !best_enemy )
			{
				//Com_Printf( "(%d) drop agg - no enemy (patrol)\n", level.time );
				Jedi_AggressionErosion(aiEnt , -1);
				//FIXME: what about alerts?  But not if ignore alerts
			}
			else
			{//have one to consider
				if ( NPC_ClearLOS4(aiEnt, best_enemy ) )
				{//we have a clear (of architecture) LOS to him
					if ( best_enemy->s.number )
					{//just attack
						G_SetEnemy( aiEnt, best_enemy );
						aiEnt->NPC->stats.aggression = 3;
					}
					else if ( NPC_IsBountyHunter(aiEnt) || aiEnt->hasJetpack )
					{//the player, toy with him
						//get progressively more interested over time
						if ( TIMER_Done( aiEnt, "watchTime" ) )
						{//we want to pick him up in stages
							if ( TIMER_Get( aiEnt, "watchTime" ) == -1 )
							{//this is the first time, we'll ignore him for a couple seconds
								TIMER_Set( aiEnt, "watchTime", Q_irand( 3000, 5000 ) );
								goto finish;
							}
							else
							{//okay, we've ignored him, now start to notice him
								if ( !aiEnt->NPC->investigateCount )
								{
									if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))//NPC_IsStormtrooper(aiEnt))
										ST_Speech( aiEnt, SPEECH_DETECTED, 0 );
									else
										G_AddVoiceEvent( aiEnt, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 10000 );
								}
								aiEnt->NPC->investigateCount++;
								TIMER_Set( aiEnt, "watchTime", Q_irand( 9000, 15000 ) );
							}
						}
						//while we're waiting, do what we need to do
						if ( best_enemy_dist < (440*440) || aiEnt->NPC->investigateCount >= 2 )
						{//stage three: keep facing him
							NPC_FaceEntity(aiEnt, best_enemy, qtrue );
							if ( best_enemy_dist < (330*330) )
							{//stage four: turn on the saber
								if ( !aiEnt->client->ps.saberInFlight )
								{
									WP_ActivateSaber(aiEnt);
								}
							}
						}
						else if ( best_enemy_dist < (550*550) || aiEnt->NPC->investigateCount == 1 )
						{//stage two: stop and face him every now and then
							if ( TIMER_Done( aiEnt, "watchTime" ) )
							{
								NPC_FaceEntity(aiEnt, best_enemy, qtrue );
							}
						}
						else
						{//stage one: look at him.
							NPC_SetLookTarget( aiEnt, best_enemy->s.number, 0 );
						}
					}
				}
				else if ( TIMER_Done( aiEnt, "watchTime" ) )
				{//haven't seen him in a bit, clear the lookTarget
					NPC_ClearLookTarget( aiEnt );
				}
			}
		}
	}
finish:
	//If we have somewhere to go, then do that
	/*
	if ( UpdateGoal() )
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		//Jedi_Move( aiEnt, NPCInfo->goalEntity );
		NPC_CombatMoveToGoal( aiEnt, qtrue );
	}
	*/

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	if ( aiEnt->enemy )
	{//just picked one up
		aiEnt->NPC->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
	}

	if ( UpdateGoal(aiEnt) )
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		//Jedi_Move( aiEnt, aiEnt->NPC->goalEntity, qfalse );
		NPC_CombatMoveToGoal(aiEnt, qtrue, qfalse );
	}
}

qboolean Jedi_CanPullBackSaber( gentity_t *self )
{
	if ( self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN && !TIMER_Done( self, "parryTime" ) )
	{
		return qfalse;
	}

	if ( self->client->NPC_class == CLASS_SHADOWTROOPER
		|| self->client->NPC_class == CLASS_TAVION
		|| self->client->NPC_class == CLASS_LUKE
		|| self->client->NPC_class == CLASS_DESANN
		|| !Q_stricmp( "Yoda", self->NPC_type ) )
	{
		return qtrue;
	}

	if ( self->painDebounceTime > level.time )//|| self->client->ps.weaponTime > 0 )
	{
		return qfalse;
	}

	return qtrue;
}
/*
-------------------------
NPC_BSJedi_FollowLeader
-------------------------
*/
void NPC_BSJedi_FollowLeader( gentity_t *aiEnt)
{
	qboolean haveEnemy = qfalse;
	qboolean enemyJumping = qfalse;
	float enemy_dist;

	if (aiEnt->enemy && NPC_IsAlive(aiEnt, aiEnt->enemy))
		haveEnemy = qtrue;

	if (haveEnemy)
	{
		enemy_dist = Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin);

		if (aiEnt->enemy->s.groundEntityNum == ENTITYNUM_NONE)
			enemyJumping = qtrue;
	}
	else
	{
		enemy_dist = Distance(aiEnt->r.currentOrigin, aiEnt->NPC->goalEntity->r.currentOrigin);
	}

	aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
	if ( !aiEnt->enemy )
	{
		//Com_Printf( "(%d) drop agg - no enemy (follow)\n", level.time );
		Jedi_AggressionErosion(aiEnt , -1);
	}

	//did we drop our saber?  If so, go after it!
	if ( aiEnt->client->ps.saberInFlight )
	{//saber is not in hand
		if ( aiEnt->client->ps.saberEntityNum < ENTITYNUM_NONE && aiEnt->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( g_entities[aiEnt->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			{//fell to the ground, try to pick it up...
				if ( Jedi_CanPullBackSaber( aiEnt ) )
				{
					//FIXME: if it's on the ground and we just pulled it back to us, should we
					//		stand still for a bit to make sure it gets to us...?
					//		otherwise we could end up running away from it while it's on its
					//		way back to us and we could lose it again.
					aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
					aiEnt->NPC->goalEntity = &g_entities[aiEnt->client->ps.saberEntityNum];
					aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
					if ( aiEnt->enemy && aiEnt->enemy->health > 0 )
					{//get our saber back NOW!
						if ( !NPC_CombatMoveToGoal(aiEnt, qtrue, qfalse ) )//Jedi_Move( NPCInfo->goalEntity, qfalse );
						{//can't nav to it, try jumping to it
							NPC_FaceEntity(aiEnt, aiEnt->NPC->goalEntity, qtrue );
							if (!Jedi_TryJump(aiEnt, aiEnt->NPC->goalEntity ))
							{
								Jedi_Move(aiEnt, aiEnt->NPC->goalEntity, qfalse); // UQ1: FALLBACK
								//Jedi_Hunt(); // UQ1: FALLBACK - HUNT...
							}
						}
						NPC_UpdateAngles(aiEnt, qtrue, qtrue );
						return;
					}
				}
			}
		}
	}

	if ( aiEnt->NPC->goalEntity )
	{
		trace_t	trace;

		if ( NPC_EnemyAboveMe(aiEnt) && !enemyJumping && NPC_Jump(aiEnt, aiEnt->NPC->goalEntity->r.currentOrigin) )
		{
			return;
		}

		if ( Jedi_Jumping(aiEnt, aiEnt->NPC->goalEntity ) )
		{//in mid-jump
			return;
		}

		if ( !NAV_CheckAhead( aiEnt, aiEnt->NPC->goalEntity->r.currentOrigin, &trace, ( aiEnt->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
		{//can't get straight to him
			if ( !NPC_ClearLOS4(aiEnt, aiEnt->NPC->goalEntity ) && NPC_FaceEntity(aiEnt, aiEnt->NPC->goalEntity, qtrue ) )
			{//no line of sight
				if (haveEnemy && NPC_FollowEnemyRoute(aiEnt))
				{
					//trap->Print("ROUTE!\n");

					if ( enemy_dist < 384 && !Q_irand( 0, 10 ) && aiEnt->NPC->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time && !NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
					{
						if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))
							ST_Speech( aiEnt, SPEECH_LOST, 0 );
						else
							G_AddVoiceEvent( aiEnt, Q_irand( EV_JLOST1, EV_JLOST3 ), 10000 );

						jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
					}

					return;
				}
				else
				{
					if ( !enemyJumping && Jedi_TryJump(aiEnt, aiEnt->NPC->goalEntity ) )
					{//started a jump
						return;
					}
					else
					{
						Jedi_Move(aiEnt, aiEnt->NPC->goalEntity, qfalse); // UQ1: FALLBACK
						return;
					}
				}
			}
		}

		if ( aiEnt->NPC->aiFlags & NPCAI_BLOCKED )
		{
			if (haveEnemy && NPC_FollowEnemyRoute(aiEnt))
			{
				//trap->Print("ROUTE!\n");

				if ( aiEnt->NPC->goalEntity
					&& Distance(aiEnt->r.currentOrigin, aiEnt->NPC->goalEntity->r.currentOrigin) < 384 
					&& !Q_irand( 0, 10 ) && aiEnt->NPC->blockedSpeechDebounceTime < level.time 
					&& jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time && !NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
				{
					if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))//NPC_IsStormtrooper(aiEnt))
						ST_Speech( aiEnt, SPEECH_GIVEUP, 0 );
					else
						G_AddVoiceEvent( aiEnt, Q_irand( EV_JLOST1, EV_JLOST3 ), 10000 );

					jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
				}
				return;
			}
			else
			{
				//trap->Print("FAILED ROUTE!\n");
				NPC_ClearPathData(aiEnt);

				if (Jedi_Hunt(aiEnt))
					return;

				if (Jedi_Track(aiEnt))
					return;
			}
		}
	}

	if (!aiEnt->client->leader)
	{// No leader... Pathing...
		if (haveEnemy && NPC_FollowEnemyRoute(aiEnt))
		{
			//trap->Print("ROUTE!\n");

			if ( aiEnt->NPC->goalEntity
				&& Distance(aiEnt->r.currentOrigin, aiEnt->NPC->goalEntity->r.currentOrigin) < 384 
				&& !Q_irand( 0, 10 ) && aiEnt->NPC->blockedSpeechDebounceTime < level.time 
				&& jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time && !NPC_ClearLOS4(aiEnt, aiEnt->enemy ) )
			{
				if (!NPC_IsJedi(aiEnt) && !NPC_IsBountyHunter(aiEnt))//NPC_IsStormtrooper(aiEnt))
					ST_Speech( aiEnt, SPEECH_GIVEUP, 0 );
				else
					G_AddVoiceEvent( aiEnt, Q_irand( EV_JLOST1, EV_JLOST3 ), 10000 );

				jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
			}
			return;
		}
		else
		{
			//trap->Print("FAILED ROUTE!\n");
			NPC_ClearPathData(aiEnt);

			if (Jedi_Hunt(aiEnt))
				return;

			if (Jedi_Track(aiEnt))
				return;
		}
	}
	else
	{//try normal movement
		NPC_BSFollowLeader(aiEnt);
	}
}


/*
-------------------------
Jedi_Attack
-------------------------
*/

static void Jedi_Attack( gentity_t *aiEnt)
{
	if (aiEnt->client->ps.weapon == WP_SABER)
	{// UQ1: Testing new AI...
		if (aiEnt->enemy)
		{
			//always face enemy if have one
			aiEnt->NPC->combatMove = qtrue;

			if (Distance(aiEnt->client->ps.origin, aiEnt->enemy->r.currentOrigin) <= 96.0)
			{
				usercmd_t *cmd = &aiEnt->client->pers.cmd;
				cmd->buttons |= BUTTON_ATTACK;
				
				if (aiEnt->NPC->saberAttackDirectionTime < level.time)
				{// Pick a new direction for saber attack move selection....
					aiEnt->NPC->saberAttackDirection = irand(0, 4);
					aiEnt->NPC->saberAttackDirectionTime = level.time + 500;
				}

				switch (aiEnt->NPC->saberAttackDirection)
				{
				default:
				case 0:
					// Just walk move forwards...
					cmd->forwardmove = 48.0;
					break;
				case 1:
					// Forward and right...
					cmd->forwardmove = 48.0;
					cmd->rightmove = 48.0;
					break;
				case 2:
					// Forward and left...
					cmd->forwardmove = 48.0;
					cmd->rightmove = -48.0;
					break;
				case 3:
					// Right...
					cmd->rightmove = 48.0;
					break;
				case 4:
					// Left...
					cmd->rightmove = -48.0;
					break;
				}
			}
			else
			{
				aiEnt->NPC->saberAttackDirectionTime = 0;
			}
		}
		else
		{
			aiEnt->NPC->saberAttackDirectionTime = 0;
			Jedi_Patrol(aiEnt);//was calling Idle... why?
			return;
		}

		return;
	}


	//Don't do anything if we're in a pain anim
	if ( aiEnt->painDebounceTime > level.time )
	{
		if ( Q_irand( 0, 1 ) )
		{
			Jedi_FaceEnemy(aiEnt, qtrue );
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );

		//JEDI_Debug(aiEnt, "painDebounce.");
		return;
	}

	if ( aiEnt->client->ps.saberLockTime > level.time )
	{
		//FIXME: maybe if I'm losing I should try to force-push out of it?  Very rarely, though...
		if ( aiEnt->client->ps.fd.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
			&& aiEnt->client->ps.saberLockTime < level.time + 5000
			&& !Q_irand( 0, 10 ))
		{
			ForceThrow( aiEnt, qfalse );
		}
		//based on my skill, hit attack button every other to every several frames in order to push enemy back
		else
		{
			float chance;

			if ( aiEnt->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",aiEnt->NPC_type) )
			{
				if ( g_npcspskill.integer )
				{
					chance = 4.0f;//he pushes *hard*
				}
				else
				{
					chance = 3.0f;//he pushes *hard*
				}
			}
			else if ( aiEnt->client->NPC_class == CLASS_TAVION )
			{
				chance = 2.0f+g_npcspskill.value;//from 2 to 4
			}
			else
			{//the escalation in difficulty is nice, here, but cap it so it doesn't get *impossible* on hard
				float maxChance	= (float)(RANK_LT)/2.0f+3.0f;//5?
				if ( !g_npcspskill.value )
				{
					chance = (float)(aiEnt->NPC->rank)/2.0f;
				}
				else
				{
					chance = (float)(aiEnt->NPC->rank)/2.0f+1.0f;
				}
				if ( chance > maxChance )
				{
					chance = maxChance;
				}
			}
		//	if ( flrand( -4.0f, chance ) >= 0.0f && !(NPC->client->ps.pm_flags&PMF_ATTACK_HELD) )
		//	{
		//		ucmd.buttons |= BUTTON_ATTACK;
		//	}
			if (aiEnt->enemy && !NPC_ValidEnemy(aiEnt, aiEnt->enemy))
			{
				
			}
			else if ( flrand( -4.0f, chance ) >= 0.0f )
			{
				aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
			}
			//rwwFIXMEFIXME: support for PMF_ATTACK_HELD
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );

		//JEDI_Debug(aiEnt, "SaberLock.");
		return;
	}
	//did we drop our saber?  If so, go after it!
	if ( aiEnt->client->ps.saberInFlight )
	{//saber is not in hand
	//	if ( NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0 )//player is 0
		if (!aiEnt->client->ps.saberEntityNum && aiEnt->client->saberStoredIndex) //this is valid, it's 0 when our saber is gone -rww (mp-specific)
		{//
			//if ( g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			if (1) //no matter
			{//fell to the ground, try to pick it up
			//	if ( Jedi_CanPullBackSaber( NPC ) )
				if (1) //no matter
				{
					aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
					aiEnt->NPC->goalEntity = &g_entities[aiEnt->client->saberStoredIndex];
					aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
					if ( aiEnt->enemy && aiEnt->enemy->health > 0 )
					{//get our saber back NOW!
						Jedi_Move(aiEnt, aiEnt->NPC->goalEntity, qfalse );
						NPC_UpdateAngles(aiEnt, qtrue, qtrue );
						if ( aiEnt->enemy->s.weapon == WP_SABER )
						{//be sure to continue evasion
							vec3_t	enemy_dir, enemy_movedir, enemy_dest;
							float	enemy_dist, enemy_movespeed;
							Jedi_SetEnemyInfo(aiEnt, enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );
							Jedi_EvasionSaber(aiEnt, enemy_movedir, enemy_dist, enemy_dir );
						}

						//JEDI_Debug(aiEnt, "SaberInFlight.");
						return;
					}
				}
			}
		}
	}

	//see if our enemy was killed by us, gloat and turn off saber after cool down.
	//FIXME: don't do this if we have other enemies to fight...?
	if ( aiEnt->enemy )
	{
		if ( aiEnt->enemy->health <= 0 && aiEnt->enemy->enemy == aiEnt && aiEnt->client->playerTeam != NPCTEAM_PLAYER )//good guys don't gloat
		{//my enemy is dead and I killed him
			aiEnt->NPC->enemyCheckDebounceTime = 0;//keep looking for others

			if ( NPC_IsBountyHunter(aiEnt) )
			{
				if ( aiEnt->NPC->walkDebounceTime < level.time && aiEnt->NPC->walkDebounceTime >= 0 )
				{
					TIMER_Set( aiEnt, "gloatTime", 10000 );
					aiEnt->NPC->walkDebounceTime = -1;
				}
				if ( !TIMER_Done( aiEnt, "gloatTime" ) )
				{
					if ( DistanceHorizontalSquared( aiEnt->client->renderInfo.eyePoint, aiEnt->enemy->r.currentOrigin ) > 4096 && (aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{
						aiEnt->NPC->goalEntity = aiEnt->enemy;
						Jedi_Move(aiEnt, aiEnt->enemy, qfalse );
						aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
					}
					else
					{
						TIMER_Set( aiEnt, "gloatTime", 0 );
					}
				}
				else if ( aiEnt->NPC->walkDebounceTime == -1 )
				{
					aiEnt->NPC->walkDebounceTime = -2;

					if (aiEnt->s.NPC_class == CLASS_PADAWAN)
					{// Do any padawan chats...
						G_AddPadawanCombatCommentEvent( aiEnt, EV_PADAWAN_COMBAT_KILL_TALK, 10000+irand(0,15000) );
					}
					else
					{
						G_AddVoiceEvent( aiEnt, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 10000 );
					}

					jediSpeechDebounceTime[aiEnt->client->playerTeam] = level.time + 10000;
					aiEnt->NPC->desiredPitch = 0;
					aiEnt->NPC->goalEntity = NULL;
				}
				Jedi_FaceEnemy(aiEnt, qtrue );
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );
				return;
			}
			else
			{
				if ( !TIMER_Done( aiEnt, "parryTime" ) )
				{
					TIMER_Set( aiEnt, "parryTime", -1 );
					aiEnt->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
				}
				aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
				if ( !aiEnt->client->ps.saberHolstered && aiEnt->client->ps.saberInFlight )
				{//saber is still on (or we're trying to pull it back), count down erosion and keep facing the enemy
					//FIXME: need to stop this from happening over and over again when they're blocking their victim's saber
					//FIXME: turn off saber sooner so we get cool walk anim?
					//Com_Printf( "(%d) drop agg - enemy dead\n", level.time );
					Jedi_AggressionErosion(aiEnt , -3);
					if ( BG_SabersOff( &aiEnt->client->ps ) && !aiEnt->client->ps.saberInFlight )
					{//turned off saber (in hand), gloat
						if (aiEnt->s.NPC_class == CLASS_PADAWAN)
						{// Do any padawan chats...
							G_AddPadawanCombatCommentEvent( aiEnt, EV_PADAWAN_COMBAT_KILL_TALK, 10000+irand(0,15000) );
						}
						else
						{
							G_AddVoiceEvent( aiEnt, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 10000 );
						}

						jediSpeechDebounceTime[aiEnt->client->playerTeam] = level.time + 10000;
						aiEnt->NPC->desiredPitch = 0;
						aiEnt->NPC->goalEntity = NULL;
					}
					TIMER_Set( aiEnt, "gloatTime", 10000 );
				}
				if ( !aiEnt->client->ps.saberHolstered || aiEnt->client->ps.saberInFlight || !TIMER_Done( aiEnt, "gloatTime" ) )
				{//keep walking
					if ( DistanceHorizontalSquared( aiEnt->client->renderInfo.eyePoint, aiEnt->enemy->r.currentOrigin ) > 4096 && (aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{
						aiEnt->NPC->goalEntity = aiEnt->enemy;
						Jedi_Move(aiEnt, aiEnt->enemy, qfalse );
						aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
					}
					else
					{//got there
						if ( aiEnt->health < aiEnt->client->pers.maxHealth
							&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0
							&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0 )
						{
							ForceHeal( aiEnt );
						}
					}
					Jedi_FaceEnemy(aiEnt, qtrue );
					NPC_UpdateAngles(aiEnt, qtrue, qtrue );

					//JEDI_Debug(aiEnt, "Gloat.");
					return;
				}
			}
		}
	}

	//If we don't have an enemy, just idle
	if ( aiEnt->enemy && aiEnt->enemy->s.weapon == WP_TURRET && !Q_stricmp( "PAS", aiEnt->enemy->classname ) )
	{
		if ( aiEnt->enemy->count <= 0 )
		{//it's out of ammo
			if ( aiEnt->enemy->activator && NPC_ValidEnemy(aiEnt, aiEnt->enemy->activator ) )
			{
				gentity_t *turretOwner = aiEnt->enemy->activator;
				G_ClearEnemy( aiEnt );
				G_SetEnemy( aiEnt, turretOwner );
			}
			else
			{
				G_ClearEnemy( aiEnt );
			}
		}
	}

	NPC_CheckEnemy(aiEnt, qtrue, qtrue, qtrue );

	if ( !aiEnt->enemy )
	{
		aiEnt->client->ps.saberBlocked = BLOCKED_NONE;
		if ( aiEnt->NPC->tempBehavior == BS_HUNT_AND_KILL )
		{//lost him, go back to what we were doing before
			aiEnt->NPC->tempBehavior = BS_DEFAULT;
			NPC_UpdateAngles(aiEnt, qtrue, qtrue );
			return;
		}

		Jedi_Patrol(aiEnt);//was calling Idle... why?
		return;
	}

	//always face enemy if have one
	aiEnt->NPC->combatMove = qtrue;

	//Track the player and kill them if possible
	Jedi_Combat(aiEnt);
	
#if 0
	if ( !(aiEnt->NPC->scriptFlags&SCF_CHASE_ENEMIES)
		|| ((aiEnt->client->ps.fd.forcePowersActive&(1<<FP_HEAL))&&aiEnt->client->ps.fd.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_2))
	{//this is really stupid, but okay...
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.rightmove = 0;
		if ( aiEnt->client->pers.cmd.upmove > 0 )
		{
			aiEnt->client->pers.cmd.upmove = 0;
		}
		aiEnt->client->ps.fd.forceJumpCharge = 0;
		VectorClear( aiEnt->client->ps.moveDir );
	}
#endif

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( aiEnt->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//don't push while in air, throws off jumps!
		//FIXME: if we are in the air over a drop near a ledge, should we try to push back towards the ledge?
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.rightmove = 0;
		VectorClear( aiEnt->client->ps.moveDir );
	}

	if ( !TIMER_Done( aiEnt, "duck" ) )
	{
		aiEnt->client->pers.cmd.upmove = -127;
	}

	if ( aiEnt->client->ps.weapon == WP_SABER/*NPC_IsJedi(aiEnt)*/ )
	{
		if ( PM_SaberInBrokenParry( aiEnt->client->ps.saberMove ) || aiEnt->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
		{//just make sure they don't pull their saber to them if they're being blocked
			aiEnt->client->pers.cmd.buttons &= ~BUTTON_ATTACK;
		}
	}

	if( (aiEnt->NPC->scriptFlags&SCF_DONT_FIRE) //not allowed to attack
		|| ((aiEnt->client->ps.fd.forcePowersActive&(1<<FP_HEAL))&&aiEnt->client->ps.fd.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_3)
		|| ((aiEnt->client->ps.saberEventFlags&SEF_INWATER)&&!aiEnt->client->ps.saberInFlight) )//saber in water
	{
		aiEnt->client->pers.cmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
	}

	if ( aiEnt->NPC->scriptFlags&SCF_NO_ACROBATICS )
	{
		aiEnt->client->pers.cmd.upmove = 0;
		aiEnt->client->ps.fd.forceJumpCharge = 0;
	}

	if ( aiEnt->client->ps.weapon == WP_SABER/*NPC_IsJedi(aiEnt)*/ )
	{
		Jedi_CheckDecreaseSaberAnimLevel(aiEnt);
	}

	if ( aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK /*&& aiEnt->client->playerTeam == NPCTEAM_ENEMY*/ )
	{
		if ( Q_irand( 0, aiEnt->client->ps.fd.saberAnimLevel ) > 0
			&& Q_irand( 0, aiEnt->client->pers.maxHealth+10 ) > aiEnt->health
			&& !Q_irand( 0, 3 ))
		{//the more we're hurt and the stronger the attack we're using, the more likely we are to make a anger noise when we swing
			G_AddVoiceEvent( aiEnt, Q_irand( EV_COMBAT1, EV_COMBAT3 ), 1000 );
		}
	}

#ifdef __FORCE_SPEED__
	if ( aiEnt->client->ps.weapon == WP_SABER/*NPC_IsJedi(aiEnt)*/ )
	{
		if ( aiEnt->client->NPC_class == CLASS_TAVION
			|| (g_npcspskill.integer && ( aiEnt->client->NPC_class == CLASS_DESANN || aiEnt->NPC->rank >= Q_irand( RANK_CREWMAN, RANK_CAPTAIN ))))
		{//Tavion will kick in force speed if the player does...
			if ( aiEnt->enemy
				&& aiEnt->enemy->s.number >= 0 && aiEnt->enemy->s.number < MAX_CLIENTS
				&& aiEnt->enemy->client
				&& (aiEnt->enemy->client->ps.fd.forcePowersActive & (1<<FP_SPEED))
				&& !(aiEnt->client->ps.fd.forcePowersActive & (1<<FP_SPEED)) )
			{
				int chance = 0;
				switch ( g_npcspskill.integer )
				{
				case 0:
					chance = 9;
					break;
				case 1:
					chance = 3;
					break;
				case 2:
					chance = 1;
					break;
				}
				if ( !Q_irand( 0, chance ) )
				{
					ForceSpeed( aiEnt, 0 );
				}
			}
		}
	}
#endif //__FORCE_SPEED__
}

extern void WP_Explode( gentity_t *self );
qboolean Jedi_InSpecialMove( gentity_t *aiEnt)
{
	if ( aiEnt->client->ps.torsoAnim == BOTH_KYLE_PA_1
		|| aiEnt->client->ps.torsoAnim == BOTH_KYLE_PA_2
		|| aiEnt->client->ps.torsoAnim == BOTH_KYLE_PA_3
		|| aiEnt->client->ps.torsoAnim == BOTH_PLAYER_PA_1
		|| aiEnt->client->ps.torsoAnim == BOTH_PLAYER_PA_2
		|| aiEnt->client->ps.torsoAnim == BOTH_PLAYER_PA_3
		|| aiEnt->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END
		|| aiEnt->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRABBED )
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return qtrue;
	}

	/*if ( Jedi_InNoAIAnim( NPC ) )
	{//in special anims, don't do force powers or attacks, just face the enemy
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( aiEnt, qtrue );
		}
		else
		{
			NPC_UpdateAngles( aiEnt, qtrue, qtrue );
		}
		return qtrue;
	}*/

	if ( aiEnt->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
		|| aiEnt->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD )
	{
		if ( !TIMER_Done( aiEnt, "draining" ) )
		{//FIXME: what do we do if we ran out of power?  NPC's can't?
			//FIXME: don't keep turning to face enemy or we'll end up spinning around
			aiEnt->client->pers.cmd.buttons |= BUTTON_FORCE_DRAIN;
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return qtrue;
	}

	if ( aiEnt->client->ps.torsoAnim == BOTH_TAVION_SWORDPOWER )
	{
		aiEnt->health += Q_irand( 1, 2 );
		if ( aiEnt->health > aiEnt->client->ps.stats[STAT_MAX_HEALTH] )
		{
			aiEnt->health = aiEnt->client->ps.stats[STAT_MAX_HEALTH];
		}
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		return qtrue;
	}
	/*else if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_HOLD )
	{
		if ( NPC->client->ps.torsoTimer <= 100 )
		{
			NPC->s.loopSound = 0;
			G_StopEffect( G_EffectIndex( "scepter/beam.efx" ), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number );
			NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoTimer;
			NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear( NPC->client->ps.velocity );
			VectorClear( NPC->client->ps.moveDir );
		}
		else
		{
			Tavion_ScepterDamage(aiEnt);
		}
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( aiEnt, qtrue );
		}
		else
		{
			NPC_UpdateAngles( aiEnt, qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_STOP )
	{
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( aiEnt, qtrue );
		}
		else
		{
			NPC_UpdateAngles( aiEnt, qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_TAVION_SCEPTERGROUND )
	{
		if ( NPC->client->ps.torsoTimer <= 1200
			&& !NPC->count )
		{
			Tavion_ScepterSlam(aiEnt);
			NPC->count = 1;
		}
		NPC_UpdateAngles( aiEnt, qtrue, qtrue );
		return qtrue;
	}*/

	if ( Jedi_CultistDestroyer( aiEnt ) )
	{
		if ( !aiEnt->takedamage )
		{//ready to explode
			if ( aiEnt->useDebounceTime <= level.time )
			{
				//this should damage everyone - FIXME: except other destroyers?
				aiEnt->client->playerTeam = NPCTEAM_FREE;//FIXME: will this destroy wampas, tusken & rancors?
				aiEnt->splashDamage = 200;	// rough match to SP
				aiEnt->splashRadius = 512;	// see above
				WP_Explode( aiEnt );
				return qtrue;
			}
			if ( aiEnt->enemy )
			{
				NPC_FaceEnemy(aiEnt, qfalse );
			}
			return qtrue;
		}
	}
	return qfalse;
}

void BountyHunter_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->client->ps.weapon != aiEnt->NPC->originalWeapon)
	{
		Boba_ChangeWeapon(aiEnt, aiEnt->NPC->originalWeapon);
	}
}

void Commando_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->client->ps.weapon != aiEnt->NPC->originalWeapon)
	{
		Boba_ChangeWeapon(aiEnt, aiEnt->NPC->originalWeapon);
	}
}

void Follower_SelectBestWeapon(gentity_t *aiEnt)
{// Stoiss????
	if (aiEnt->client->ps.weapon != aiEnt->NPC->originalWeapon)
	{
		Boba_ChangeWeapon(aiEnt, aiEnt->NPC->originalWeapon);
	}
}

void Gunner_SelectBestWeapon(gentity_t *aiEnt)
{// Stoiss????
	if (aiEnt->client->ps.weapon != aiEnt->NPC->originalWeapon)
	{
		Boba_ChangeWeapon(aiEnt, aiEnt->NPC->originalWeapon);
	}
}

void AdvancedGunner_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->client->ps.weapon != aiEnt->NPC->originalWeapon)
	{
		Boba_ChangeWeapon(aiEnt, aiEnt->NPC->originalWeapon);
	}
}

void Grenader_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->client->ps.weapon != aiEnt->NPC->originalWeapon)
	{
		Boba_ChangeWeapon(aiEnt, aiEnt->NPC->originalWeapon);
	}
}

void Jedi_SelectBestWeapon( gentity_t *aiEnt)
{
	if ( aiEnt->enemy
		&& aiEnt->client->ps.weapon != WP_MODULIZED_WEAPON
		&& Distance( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin ) > 768 )
	{
		Boba_ChangeWeapon(aiEnt, WP_MODULIZED_WEAPON);
		return;
	}
	else if ( aiEnt->client->ps.weapon != WP_SABER )
	{
		Boba_ChangeWeapon(aiEnt, WP_SABER );
		return;
	}

	/*
	if ( NPC_HasGrenades(aiEnt) )
	{
		Grenader_SelectBestWeapon(aiEnt);
		return;
	}
	*/
}

void BOT_SelectBestWeapon( gentity_t *aiEnt)
{
	if ( aiEnt->enemy
		&& aiEnt->client->ps.weapon != WP_MODULIZED_WEAPON
		&& Distance( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin ) > 512 )
	{
		Fast_ChangeWeapon(aiEnt, WP_MODULIZED_WEAPON);
		return;
	}
	else if ( aiEnt->client->ps.weapon != WP_SABER )
	{
		Boba_ChangeWeapon(aiEnt, WP_SABER );
		return;
	}
}

void Default_SelectBestWeapon( gentity_t *aiEnt)
{
	int primaryWeapon = aiEnt->client->ps.primaryWeapon;
	int secondaryWeapon = aiEnt->client->ps.secondaryWeapon;
	int temporaryWeapon = aiEnt->client->ps.temporaryWeapon;

	if (primaryWeapon <= WP_NONE) 
	{
		if (aiEnt->client && aiEnt->client->ps.weapon > WP_NONE)
			primaryWeapon = aiEnt->client->ps.weapon;
		else
			primaryWeapon = aiEnt->s.weapon;
	}

	if ( temporaryWeapon > WP_NONE
		&& aiEnt->enemy
		&& aiEnt->client->ps.weapon != temporaryWeapon 
		&& Distance( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin ) > 800 )
	{
		Boba_ChangeWeapon(aiEnt, temporaryWeapon );
	}
	else if (secondaryWeapon > WP_NONE
		&& aiEnt->enemy
		&& aiEnt->client->ps.weapon != secondaryWeapon
		&& Distance( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin ) > 400 )
	{
		Boba_ChangeWeapon(aiEnt, secondaryWeapon);
	}
	else if (aiEnt->client->ps.weapon != primaryWeapon)
	{
		Boba_ChangeWeapon(aiEnt, primaryWeapon);
	}
}

void NPC_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->next_weapon_switch > level.time) return;

	if ( aiEnt->s.eType == ET_PLAYER )
	{// A BotNPC...
		BOT_SelectBestWeapon(aiEnt);
		return;
	}

	if ( NPC_IsJedi(aiEnt) )
	{
		Jedi_SelectBestWeapon(aiEnt);
		return;
	}

	if (NPC_IsFollowerGunner(aiEnt))
	{// Hmm, stoiss... What guns????
		Follower_SelectBestWeapon(aiEnt);
		return;
	}

	if (NPC_IsGunner(aiEnt))
	{// Hmm, stoiss... What guns????
		Gunner_SelectBestWeapon(aiEnt);
		return;
	}

	if ( NPC_IsBountyHunter(aiEnt) )
	{
		BountyHunter_SelectBestWeapon(aiEnt);
		return;
	}

	if ( NPC_IsCommando(aiEnt) )
	{
		Commando_SelectBestWeapon(aiEnt);
		return;
	}

	if (NPC_IsAdvancedGunner(aiEnt) )
	{
		AdvancedGunner_SelectBestWeapon(aiEnt);
		return;
	}

	/*
	if ( NPC_HasGrenades(aiEnt) )
	{
		Grenader_SelectBestWeapon(aiEnt);
		return;
	}
	*/

	// If none of the above, then select based on NPC file...
	Default_SelectBestWeapon(aiEnt);
}

qboolean Jedi_CheckForce ( gentity_t *aiEnt)
{// UQ1: New code to make better use of force powers...
	//
	// Give them any force powers they might need...
	//
	if (NPC_IsLightJedi(aiEnt))
	{// Jedi...
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_TEAM_HEAL))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_TEAM_HEAL);
			aiEnt->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] = 3;
		}
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_HEAL))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_HEAL);
			aiEnt->client->ps.fd.forcePowerLevel[FP_HEAL] = 3;
		}
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_PROTECT))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_PROTECT);
			aiEnt->client->ps.fd.forcePowerLevel[FP_PROTECT] = 3;
		}
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_ABSORB);
			aiEnt->client->ps.fd.forcePowerLevel[FP_ABSORB] = 3;
		}
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_TELEPATHY))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_TELEPATHY);
			aiEnt->client->ps.fd.forcePowerLevel[FP_TELEPATHY] = 3;
		}
	}
	else if (NPC_IsDarkJedi(aiEnt))
	{// Sith...
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_DRAIN))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_DRAIN);
			aiEnt->client->ps.fd.forcePowerLevel[FP_DRAIN] = 3;
		}
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_LIGHTNING))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_LIGHTNING);
			aiEnt->client->ps.fd.forcePowerLevel[FP_LIGHTNING] = 3;
		}
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_GRIP))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_GRIP);
			aiEnt->client->ps.fd.forcePowerLevel[FP_GRIP] = 3;
		}
		/*
		if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_RAGE))) 
		{
			aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_RAGE);
			aiEnt->client->ps.fd.forcePowerLevel[FP_RAGE] = 3;
		}
		*/
	}
	else
	{// Not a jedi/sith???
		return qfalse;
	}

	if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_PUSH))) 
	{
		aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_PUSH);
		aiEnt->client->ps.fd.forcePowerLevel[FP_PUSH] = 3;
	}

	if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_PULL))) 
	{
		aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_PULL);
		aiEnt->client->ps.fd.forcePowerLevel[FP_PULL] = 3;
	}

	if (!(aiEnt->client->ps.fd.forcePowersKnown & (1 << FP_SABERTHROW))) 
	{
		aiEnt->client->ps.fd.forcePowersKnown |= (1 << FP_SABERTHROW);
		aiEnt->client->ps.fd.forcePowerLevel[FP_SABERTHROW] = 3;
	}

	if (Jedi_SaberBusy( aiEnt ))
	{
		return qfalse;
	}

	// UQ1: Special heals/protects/absorbs - mainly for padawans...
	if ( TIMER_Done( aiEnt, "teamheal" )
		&& aiEnt->padawan
		&& NPC_IsAlive(aiEnt, aiEnt->padawan)
		&& aiEnt->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] > 0
		&& Distance(aiEnt->padawan->r.currentOrigin, aiEnt->r.currentOrigin) < 256
		&& NPC_NeedsHeal( aiEnt->padawan )
		&& Q_irand( 0, 20 ) < 2)
	{// Team heal our padawan???
		NPC_FacePosition(aiEnt, aiEnt->padawan->r.currentOrigin, qtrue);
		ForceTeamHeal( aiEnt );
		TIMER_Set( aiEnt, "teamheal", irand(5000, 15000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "teamheal" )
		&& aiEnt->parent
		&& NPC_IsAlive(aiEnt, aiEnt->parent)
		&& aiEnt->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] > 0
		&& Distance(aiEnt->parent->r.currentOrigin, aiEnt->r.currentOrigin) < 256
		&& NPC_NeedsHeal( aiEnt->parent )
		&& Q_irand( 0, 20 ) < 2)
	{// Team heal our jedi???
		NPC_FacePosition(aiEnt, aiEnt->parent->r.currentOrigin, qtrue);
		ForceTeamHeal( aiEnt );
		TIMER_Set( aiEnt, "teamheal", irand(5000, 15000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "heal" )
		&& aiEnt->client->ps.fd.forcePowerLevel[FP_HEAL] > 0
		&& NPC_NeedsHeal( aiEnt )
		&& Q_irand( 0, 20 ) < 2)
	{
		//trap->Print("%s is using heal.\n", aiEnt->NPC_type);
		ForceHeal( aiEnt );
		TIMER_Set( aiEnt, "heal", irand(5000, 15000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "drain" )
		&& aiEnt->client->ps.fd.forcePowerLevel[FP_DRAIN] > 0
		&& NPC_Jedi_EnemyInForceRange(aiEnt)
		&& NPC_NeedsHeal( aiEnt )
		&& Q_irand( 0, 20 ) < 2)
	{
		//trap->Print("%s is using drain.\n", aiEnt->NPC_type);
		NPC_FaceEnemy(aiEnt, qtrue);
		ForceDrain( aiEnt );
		TIMER_Set( aiEnt, "drain", irand(5000, 15000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "grip" )
		&& aiEnt->client->ps.fd.forcePowerLevel[FP_GRIP] > 0
		&& NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		//trap->Print("%s is using grip.\n", aiEnt->NPC_type);
		NPC_FaceEnemy(aiEnt, qtrue);
		ForceGrip( aiEnt );
		TIMER_Set( aiEnt, "grip", irand(12000, 25000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "lightning" )
		&& aiEnt->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > 0
		&& NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		//trap->Print("%s is using lightning.\n", aiEnt->NPC_type);
		NPC_FaceEnemy(aiEnt, qtrue);
		ForceLightning( aiEnt );
		TIMER_Set( aiEnt, "lightning", irand(12000, 25000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "protect" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_PROTECT)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_PROTECT)) == 0
		&& Q_irand( 0, 20 ) < 2)
	{
		//trap->Print("%s is using protect.\n", aiEnt->NPC_type);
		ForceProtect( aiEnt );
		TIMER_Set( aiEnt, "protect", irand(15000, 30000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "absorb" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_ABSORB)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_ABSORB)) == 0
		&& Q_irand( 0, 20 ) < 2)
	{
		//trap->Print("%s is using absorb.\n", aiEnt->NPC_type);
		ForceAbsorb( aiEnt );
		TIMER_Set( aiEnt, "absorb", irand(15000, 30000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "telepathy" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_TELEPATHY)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_TELEPATHY)) == 0
		&& NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		//trap->Print("%s is using telepathy.\n", aiEnt->NPC_type);
		NPC_FaceEnemy(aiEnt, qtrue);
		ForceTelepathy(aiEnt);
		TIMER_Set( aiEnt, "telepathy", irand(15000, 30000) );
		return qtrue;
	}
	/*
	else if ( TIMER_Done( aiEnt, "rage" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_RAGE)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) == 0
		&& NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		//trap->Print("%s is using rage.\n", aiEnt->NPC_type);
		Jedi_Rage(aiEnt);
		TIMER_Set( aiEnt, "rage", irand(15000, 30000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "speed" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_SPEED)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_SPEED)) == 0
		&& NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		//trap->Print("%s is using speed.\n", aiEnt->NPC_type);
		ForceSpeed( aiEnt, 500 );
		TIMER_Set( aiEnt, "speed", irand(15000, 30000) );
		return qtrue;
	}
	*/
	else if ( TIMER_Done( aiEnt, "push" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_PUSH)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_PUSH)) == 0
		&& NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		//trap->Print("%s is using push.\n", aiEnt->NPC_type);
		NPC_FaceEnemy(aiEnt, qtrue);
		ForceThrow( aiEnt, qfalse );
		TIMER_Set( aiEnt, "push", irand(15000, 30000) );
		return qtrue;
	}
	else if ( TIMER_Done( aiEnt, "pull" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_PULL)) != 0
		&& (aiEnt->client->ps.fd.forcePowersActive&(1<<FP_PULL)) == 0
		&& !NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		//trap->Print("%s is using pull.\n", aiEnt->NPC_type);
		NPC_FaceEnemy(aiEnt, qtrue);
		ForceThrow( aiEnt, qtrue );
		TIMER_Set( aiEnt, "pull", irand(15000, 30000) );
		return qtrue;
	}
	else if (aiEnt->client->ps.weapon == WP_SABER //using saber
		&& TIMER_Done( aiEnt, "saberthrow" )
		&& (aiEnt->client->ps.fd.forcePowersKnown&(1<<FP_SABERTHROW)) != 0
		&& !NPC_Jedi_EnemyInForceRange(aiEnt)
		&& Q_irand( 0, 20 ) < 2 )
	{
		NPC_FaceEnemy(aiEnt, qtrue);

		//trap->Print("%s is using saber throw.\n", aiEnt->NPC_type);
		if (aiEnt->client->ps.saberEntityState == 0  //not thrown yet
			&& aiEnt->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(aiEnt->client->ps.fd.forcePowersActive&(1 << FP_SABERTHROW))
			&& !(aiEnt->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
		{//hold it out there
			aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
			TIMER_Set( aiEnt, "saberthrow", irand(15000, 30000) );
		}
		else if (aiEnt->client->ps.saberEntityState != SES_RETURNING  //not returning yet
			&& aiEnt->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(aiEnt->client->ps.fd.forcePowersActive&(1 << FP_SABERTHROW))
			&& !(aiEnt->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
		{//hold it out there
			aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
		}
		else if (aiEnt->client->ps.saberEntityState == SES_RETURNING  //not returning yet
			&& aiEnt->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(aiEnt->client->ps.fd.forcePowersActive&(1 << FP_SABERTHROW))
			&& !(aiEnt->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
		{//hold it out there
			aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
		}

		return qtrue;
	}

	return qfalse;
}

qboolean NPC_MoveIntoOptimalAttackPosition ( gentity_t *aiEnt)
{
	gentity_t	*NPC = aiEnt;
	float		dist = DistanceHorizontal(NPC->enemy->r.currentOrigin, NPC->r.currentOrigin);
	float		OPTIMAL_MIN_RANGE = 256;
	float		OPTIMAL_MAX_RANGE = 384;
	float		OPTIMAL_RANGE = 300;
	float		OPTIMAL_RANGE_ALLOWANCE = 64;
	qboolean	ON_PLAYER = qfalse;
	qboolean	haveEnemy = qfalse;
	qboolean	enemyJumping = qfalse;
	float		enemy_dist, diff;

	if (aiEnt->enemy && NPC_IsAlive(aiEnt, aiEnt->enemy))
	{
		haveEnemy = qtrue;
	
		enemy_dist = Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin);

		if (aiEnt->enemy->s.groundEntityNum == ENTITYNUM_NONE)
		{
			enemyJumping = qtrue;
		}
	}
	else
	{
		enemy_dist = Distance(aiEnt->r.currentOrigin, aiEnt->NPC->goalEntity->r.currentOrigin);
	}

	if (NPC->s.weapon == WP_SABER)
	{
		OPTIMAL_MIN_RANGE = 20;
		OPTIMAL_MAX_RANGE = 40;
	}

	OPTIMAL_RANGE = OPTIMAL_MIN_RANGE + ((OPTIMAL_MAX_RANGE - OPTIMAL_MIN_RANGE) / 2.0);
	OPTIMAL_RANGE_ALLOWANCE = (OPTIMAL_MAX_RANGE - OPTIMAL_MIN_RANGE);

	diff = dist - OPTIMAL_RANGE;
	if (diff < 0) diff *= -1;

	if (NPC_GetOffPlayer(NPC))
	{// Get off of their head!
		//JEDI_Debug(aiEnt, "optimal position (get off player).");
		return qtrue;
	}
	else if (NPC_MoverCrushCheck(NPC))
	{// There is a mover gonna crush us... Step back...
		//JEDI_Debug(aiEnt, "optimal position (crusher).");
		return qtrue;
	}
	else if (diff <= OPTIMAL_RANGE_ALLOWANCE)
	{// We are at optimal range now...
		return qfalse;
	}
	else if (dist > OPTIMAL_MAX_RANGE)
	{// If clear then move stright there...
		NPC_FacePosition(aiEnt, NPC->enemy->r.currentOrigin, qfalse );

		if ((dist > 256 || (dist > 64 && !Jedi_ClearPathToSpot(aiEnt, aiEnt->enemy->r.currentOrigin, aiEnt->enemy->s.number))) && NPC_FollowEnemyRoute(aiEnt))
		{
			//JEDI_Debug(va("optimal position (too far - routing). Dist %f. Opt %f. Allow %f. Min %f. Max %f.", dist, OPTIMAL_RANGE, OPTIMAL_RANGE_ALLOWANCE, OPTIMAL_MIN_RANGE, OPTIMAL_MAX_RANGE));
			return qtrue;
		}
		else
		{
			Jedi_Advance(aiEnt);
			//JEDI_Debug(va("optimal position (too far - combatmove). Dist %f. Opt %f. Allow %f. Min %f. Max %f.", dist, OPTIMAL_RANGE, OPTIMAL_RANGE_ALLOWANCE, OPTIMAL_MIN_RANGE, OPTIMAL_MAX_RANGE));
			
			if (dist > 64)
				return qtrue;

			return qfalse;
		}
	}
	else if (NPC_IsJedi(aiEnt) || aiEnt->client->ps.weapon == WP_SABER)
	{// Jedi can skip the min optimal check, they handle their own attack/counters...
		return qfalse;
	}
	else if (dist < OPTIMAL_MIN_RANGE)
	{// If clear then move back a bit...
		NPC_FacePosition(aiEnt, NPC->enemy->r.currentOrigin, qfalse );

		Jedi_Retreat(aiEnt);
		//JEDI_Debug(va("optimal position (too close - combatmove). Dist %f. Opt %f. Allow %f. Min %f. Max %f.", dist, OPTIMAL_RANGE, OPTIMAL_RANGE_ALLOWANCE, OPTIMAL_MIN_RANGE, OPTIMAL_MAX_RANGE));
		return qtrue;
	}

	return qfalse;
}

qboolean NPC_JediCheckFall ( gentity_t *aiEnt)
{// UQ1: Jedi can save them selves from falling - with the force!
	gentity_t	*NPC = aiEnt;
	usercmd_t	*ucmd = &aiEnt->client->pers.cmd;

	if (!NPC_IsJedi(NPC))
	{// Only jedi (and jetpackers using their own method) can use the force to save themselves from falling...
		return qfalse;
	}

	if (NPC_IsAlive(aiEnt, NPC->enemy) && NPC->enemy->r.currentOrigin < NPC->r.currentOrigin && NPC->enemy->s.groundEntityNum != ENTITYNUM_NONE)
	{// Our enemy is below us... That is fine...
		return qfalse;
	}
	else if (NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum && gWPArray[NPC->wpCurrent]->origin < NPC->r.currentOrigin)
	{// Our waypoint is below us... That is fine...
		return qfalse;
	}

	if (TIMER_Done( aiEnt, "emergencyJumpTime" ) && NPC_JetpackFallingEmergencyCheck(NPC))
	{// We are falling... Use the force luke!

		//trap->Print("%s is emergency jumping.\n", NPC->NPC_type);

		TIMER_Set( aiEnt, "emergencyJump", 1000 );

		if (NPC->enemy 
			&& NPC_IsAlive(aiEnt, NPC->enemy) 
			&& Jedi_Jump(aiEnt, NPC->enemy->r.currentOrigin, NPC->enemy->s.number))
		{// Use enemy as our target point...
			// Play a sound to make the unbelievable, believable.... lol...
			G_Sound( NPC, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );

			// Max once every 10 seconds...
			TIMER_Set( aiEnt, "emergencyJumpTime", 10000 );

			return qtrue;
		}
		else if (NPC->wpCurrent >= 0 
			&& NPC->wpCurrent < gWPNum
			&& Jedi_Jump(aiEnt, gWPArray[NPC->wpCurrent]->origin, ENTITYNUM_NONE))
		{// Use our current waypoint as our target point...
			// Play a sound to make the unbelievable, believable.... lol...
			G_Sound( NPC, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );

			// Max once every 10 seconds...
			TIMER_Set( aiEnt, "emergencyJumpTime", 10000 );

			return qtrue;
		}
		else if (NPC->wpLast >= 0 
			&& NPC->wpLast < gWPNum
			&& Jedi_Jump(aiEnt, gWPArray[NPC->wpLast]->origin, ENTITYNUM_NONE))
		{// Use our last waypoint as our target point...
			// Play a sound to make the unbelievable, believable.... lol...
			G_Sound( NPC, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );

			// Max once every 10 seconds...
			TIMER_Set( aiEnt, "emergencyJumpTime", 10000 );

			return qtrue;
		}
		else if (NPC->wpNext >= 0 
			&& NPC->wpNext < gWPNum
			&& Jedi_Jump(aiEnt, gWPArray[NPC->wpNext]->origin, ENTITYNUM_NONE))
		{// Use our last waypoint as our target point...
			// Play a sound to make the unbelievable, believable.... lol...
			G_Sound( NPC, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );

			// Max once every 10 seconds...
			TIMER_Set( aiEnt, "emergencyJumpTime", 10000 );

			return qtrue;
		}
		else
		{// We need to find a waypoint...
			NPC->wpCurrent = DOM_GetNearWP(NPC->r.currentOrigin, NPC->wpCurrent);

			if (NPC->wpCurrent >= 0 
				&& NPC->wpCurrent < gWPNum
				&& Jedi_Jump(aiEnt, gWPArray[NPC->wpCurrent]->origin, ENTITYNUM_NONE))
			{
				// Play a sound to make the unbelievable, believable.... lol...
				G_Sound( NPC, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );

				// Max once every 10 seconds...
				TIMER_Set( aiEnt, "emergencyJumpTime", 10000 );
				return qtrue;
			}
		}
	}

	//trap->Print("%s failed emergency jumping.\n", NPC->NPC_type);

	// Uber fail... Die jedi! I could search for a waypoint he can jump to, but what's the point???
	return qfalse;
}

void NPC_CallForHelpChaseSpeech(gentity_t *aiEnt)
{
	if (NPC_IsJedi(aiEnt))
	{
		G_AddVoiceEvent(aiEnt, Q_irand(EV_JCHASE1, EV_JCHASE3), 2000);
	}
	else if (NPC_IsBountyHunter(aiEnt))
	{
		G_AddVoiceEvent(aiEnt, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
	}
	else
	{
		G_AddVoiceEvent(aiEnt, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
	}
}

void NPC_CreateAttackGroup(int faction, vec3_t position, gentity_t *aiEnt, gentity_t *enemy, float range)
{
	int addedCount = 0;

	for (int i = 0; i < MAX_GENTITIES && addedCount < 4; i++)
	{
		gentity_t *ent = &g_entities[i];

		if (!ent)
			continue;

		if (!ent->client)
			continue;

		if (ent == aiEnt)
			continue;

		if (ent->isPadawan)
			continue;

		if (ent->client->sess.sessionTeam != faction)
			continue;

		if (ent->s.eType != ET_NPC)
			continue;

		if (!NPC_IsAlive(ent, ent))
			continue;

		if (Distance(ent->r.currentOrigin, position) > range)
			continue;

		if (npc_pathing.integer == 0 && VectorLength(ent->spawn_pos) != 0 && Distance(ent->spawn_pos, position) > 6000.0)
			continue;

		if (ent->enemy && NPC_ValidEnemy(ent, ent->enemy))
			continue; // Already has an enemy...

		if (!NPC_ValidEnemy(ent, aiEnt->enemy))
			continue;

		// Looks like a valid teammate... Call them over...
		G_SetEnemy(ent, enemy);

		// So the added NPC's don't immediately call for extra help...
		ent->NPC->helpCallTime = level.time + 15000;

		addedCount++;
	}
}

extern qboolean NPC_CanUseAdvancedFighting(gentity_t *aiEnt);

void NPC_CallForHelpSpeech(gentity_t *aiEnt)
{
	if (NPC_IsJedi(aiEnt))
	{
		G_AddVoiceEvent(aiEnt, Q_irand(EV_JDETECTED1, EV_JDETECTED3), 15000 + irand(0, 30000));
	}
	else if (NPC_IsBountyHunter(aiEnt))
	{
		G_AddVoiceEvent(aiEnt, Q_irand(EV_DETECTED1, EV_DETECTED5), 15000 + irand(0, 30000));
	}
	else
	{
		if (irand(0, 1) == 1)
			ST_Speech(aiEnt, SPEECH_DETECTED, 0);
		else if (irand(0, 1) == 1)
			ST_Speech(aiEnt, SPEECH_CHASE, 0);
		else
			ST_Speech(aiEnt, SPEECH_SIGHT, 0);
	}
}

void NPC_CallForHelp(gentity_t *aiEnt)
{
	if (aiEnt->isPadawan)
		return;

	if (NPC_ValidEnemy(aiEnt, aiEnt->enemy))
	{
		if (NPC_IsJedi(aiEnt))
		{// Jedi/Sith call for help based on health...
			if (aiEnt->health < aiEnt->maxHealth * 0.75)
			{// Call for help on low health, from a larger area...
				NPC_CreateAttackGroup(aiEnt->client->sess.sessionTeam, aiEnt->r.currentOrigin, aiEnt, aiEnt->enemy, 1024.0);
				NPC_CallForHelpSpeech(aiEnt);
				aiEnt->NPC->helpCallJediLevel = 1;
			}
			else if (aiEnt->health < aiEnt->maxHealth * 0.5)
			{// Call for help on low health, from a larger area...
				NPC_CreateAttackGroup(aiEnt->client->sess.sessionTeam, aiEnt->r.currentOrigin, aiEnt, aiEnt->enemy, 2048.0);
				NPC_CallForHelpSpeech(aiEnt);
				aiEnt->NPC->helpCallJediLevel = 2;
			}
			else if (aiEnt->health < aiEnt->maxHealth * 0.25)
			{// Call for help on low health, from a larger area...
				NPC_CreateAttackGroup(aiEnt->client->sess.sessionTeam, aiEnt->r.currentOrigin, aiEnt, aiEnt->enemy, 4096.0);
				NPC_CallForHelpSpeech(aiEnt);
				aiEnt->NPC->helpCallJediLevel = 3;
			}

			return;
		}

		if (aiEnt->NPC->helpCallTime <= level.time)
		{// Standard gunners work on a timer...
			if (NPC_CanUseAdvancedFighting(aiEnt))
			{// Stormies, gunners, etc... Call all their friends from the start...
				NPC_CreateAttackGroup(aiEnt->client->sess.sessionTeam, aiEnt->r.currentOrigin, aiEnt, aiEnt->enemy, 2048.0);
				NPC_CallForHelpSpeech(aiEnt);
				aiEnt->NPC->helpCallTime = level.time + 15000;
			}
			else
			{
				NPC_CreateAttackGroup(aiEnt->client->sess.sessionTeam, aiEnt->r.currentOrigin, aiEnt, aiEnt->enemy, 512.0);
				NPC_CallForHelpSpeech(aiEnt);
				aiEnt->NPC->helpCallTime = level.time + 30000;
			}

			return;
		}
	}
}

void NPC_BSJedi_Default( gentity_t *aiEnt)
{
	if ( Jedi_InSpecialMove(aiEnt) )
	{
		//JEDI_Debug(aiEnt, "special move.");
		return;
	}

	Jedi_CheckCloak(aiEnt);

	if (Jedi_CheckForce(aiEnt))
	{// UQ1: Don't move/attack when we just used or are using a force power...
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.rightmove = 0;
		aiEnt->client->pers.cmd.upmove = 0;
		//JEDI_Debug(aiEnt, "CheckForce.");
		return;
	}

	if (NPC_JediCheckFall(aiEnt))
	{// Just get out of danger!
		//JEDI_Debug(aiEnt, "CheckFall.");
		return;
	}

	if (!aiEnt->enemy || !NPC_IsAlive(aiEnt, aiEnt->enemy))
	{
		return;
	}

	NPC_CallForHelp(aiEnt);

	{//have an enemy
		if ( Jedi_WaitingAmbush( aiEnt ) )
		{//we were still waiting to drop down - must have had enemy set on me outside my AI
			Jedi_Ambush( aiEnt );
		}

		if ( Jedi_CultistDestroyer( aiEnt )
			&& !aiEnt->NPC->charmedTime )
		{//destroyer
			//permanent effect
			aiEnt->NPC->charmedTime = Q3_INFINITE;
			aiEnt->client->ps.fd.forcePowersActive |= ( 1 << FP_RAGE );
			aiEnt->client->ps.fd.forcePowerDuration[FP_RAGE] = Q3_INFINITE;
			//NPC->client->ps.eFlags |= EF_FORCE_DRAINED;
			//FIXME: precache me!
			aiEnt->s.loopSound = G_SoundIndex( "sound/movers/objects/green_beam_lp2.wav" );//test/charm.wav" );
		}

		NPC_SelectBestWeapon(aiEnt);

		if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && Distance(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin) >= 512.0)
		{// Using sniper rifle... Use sniper AI...
			//aiEnt->NPC->scriptFlags |= SCF_ALT_FIRE;
			NPC_BSSniper_Default(aiEnt);
			return;
		}
		/*else if ( aiEnt->client->ps.weapon == WP_THERMAL )
		{// Using grenade.. Use grenader AI...
			NPC_BSGrenadier_Default(aiEnt);
			NPC_CheckEvasion(aiEnt);
			return;
		}*/

		if (aiEnt->enemy && NPC_IsAlive(aiEnt, aiEnt->enemy))
		{
			if (aiEnt->s.weapon != WP_SABER || aiEnt->enemy->s.weapon != WP_SABER)
			{// Normal non-jedi NPC or enemy... Use normal system...
				if (NPC_MoveIntoOptimalAttackPosition(aiEnt))
				{// Just move into optimal range...
					//return;
				}

				Jedi_Attack(aiEnt);
			}
			else
			{// Jedi/Sith. Use attack/counter system...
				if (Jedi_AttackOrCounter( aiEnt ))
				{// Attack...
					if (NPC_IsDarkJedi(aiEnt))
					{// Do taunt/anger...
						int TorA = Q_irand(0, 3);

						switch (TorA) {
						case 3:
							G_AddVoiceEvent(aiEnt, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + irand(0, 15000));
							break;
						case 2:
							G_AddVoiceEvent(aiEnt, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + irand(0, 15000));
							break;
						case 1:
							G_AddVoiceEvent(aiEnt, Q_irand(EV_ANGER1, EV_ANGER1), 5000 + irand(0, 15000));
							break;
						default:
							G_AddVoiceEvent(aiEnt, Q_irand(EV_TAUNT1, EV_TAUNT5), 5000 + irand(0, 15000));
							break;
						}
					}
					else if (NPC_IsJedi(aiEnt))
					{// Do taunt...
						int TorA = Q_irand(0, 2);

						switch (TorA) {
						case 2:
							G_AddVoiceEvent(aiEnt, Q_irand(EV_JCHASE1, EV_JCHASE3), 5000 + irand(0, 15000));
							break;
						case 1:
							G_AddVoiceEvent(aiEnt, Q_irand(EV_COMBAT1, EV_COMBAT3), 5000 + irand(0, 15000));
							break;
						default:
							G_AddVoiceEvent(aiEnt, Q_irand(EV_TAUNT1, EV_TAUNT5), 5000 + irand(0, 15000));
						}
					}

					if (NPC_MoveIntoOptimalAttackPosition(aiEnt))
					{// Just move into optimal range...
						return;
					}

					Jedi_Attack(aiEnt);
				}
				else
				{// Counter...
					qboolean dualSabers = qfalse;
					qboolean blockFound = qfalse;
					int saberNum, bladeNum;

					//JEDI_Debug(aiEnt, "counter");

					if (aiEnt->enemy->client->saber[1].model[0]) {
						dualSabers = qtrue;
					}

					float dist = DistanceHorizontal(aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin);

					if (dist < 96)
					{
						if (aiEnt->npc_counter_avoid_time < level.time + 2000)
						{
							if (irand(0, 25) == 25) // randomize moving away...
								aiEnt->npc_counter_avoid_time = level.time + 200;
							else if (dist < 36.0) // too close, always back off...
								aiEnt->npc_counter_avoid_time = level.time + 200;
							else // hold ground...
								aiEnt->npc_counter_avoid_time = level.time;
						}

						if (aiEnt->npc_counter_avoid_time > level.time)
						{// Only move back when not too far away...
							Jedi_Retreat(aiEnt);
						}
					}

					for (saberNum = 0; saberNum < (dualSabers ? MAX_SABERS : 1); saberNum++) 
					{
						if (blockFound) break;

						for (bladeNum = 0; bladeNum < aiEnt->enemy->client->saber[saberNum].numBlades; bladeNum++) 
						{
							if (blockFound) break;

							if (Jedi_SaberBlock(aiEnt, saberNum, bladeNum))
							{
								blockFound = qtrue;
								break;
							}
						}
					}

					if (!blockFound)
					{// Try to heal...
						if ( TIMER_Done( aiEnt, "heal" )
							&& !Jedi_SaberBusy( aiEnt )
							&& aiEnt->client->ps.fd.forcePowerLevel[FP_HEAL] > 0
							&& aiEnt->health > 0
							&& aiEnt->client->ps.weaponTime <= 0
							&& aiEnt->client->ps.fd.forcePower >= 25
							&& aiEnt->client->ps.fd.forcePowerDebounce[FP_HEAL] <= level.time
							&& WP_ForcePowerUsable(aiEnt, FP_HEAL)
							&& aiEnt->health <= aiEnt->client->ps.stats[STAT_MAX_HEALTH] / 3
							&& Q_irand( 0, 2 ) == 2)
						{// Try to heal...
							ForceHeal( aiEnt );
							TIMER_Set( aiEnt, "heal", irand(5000, 10000) );

							if (NPC_IsJedi(aiEnt))
							{// Do deflect taunt...
								G_AddVoiceEvent(aiEnt, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + irand(0, 15000));
							}
						}
						else if ( TIMER_Done( aiEnt, "drain" )
							&& !Jedi_SaberBusy( aiEnt )
							&& aiEnt->client->ps.fd.forcePowerLevel[FP_DRAIN] > 0
							&& aiEnt->health > 0
							&& aiEnt->client->ps.forceHandExtend == HANDEXTEND_NONE
							&& WP_ForcePowerUsable(aiEnt, FP_DRAIN)
							&& aiEnt->client->ps.weaponTime <= 0
							&& aiEnt->client->ps.fd.forcePower >= 25
							&& aiEnt->client->ps.fd.forcePowerDebounce[FP_DRAIN] <= level.time
							&& aiEnt->health <= aiEnt->client->ps.stats[STAT_MAX_HEALTH] / 3
							&& NPC_Jedi_EnemyInForceRange(aiEnt)
							&& Q_irand(0, 2) == 2)
						{// Try to drain them...
							NPC_FaceEnemy(aiEnt, qtrue);
							ForceDrain( aiEnt );
							TIMER_Set( aiEnt, "drain", irand(5000, 10000) );

							if (NPC_IsJedi(aiEnt))
							{// Do deflect taunt...
								G_AddVoiceEvent(aiEnt, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + irand(0, 15000));
							}
						}
						else
						{// Check for an evasion method...
							NPC_CheckEvasion(aiEnt);

							if (NPC_IsJedi(aiEnt))
							{// Do deflect taunt...
								G_AddVoiceEvent(aiEnt, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 5000 + irand(0, 15000));
							}
						}
					}
				}
			}
		}

		if (aiEnt->s.NPC_class == CLASS_PADAWAN)
		{// Do any padawan chats...
			G_AddPadawanCombatCommentEvent( aiEnt, EV_PADAWAN_COMBAT_TALK, 10000+irand(0,15000) );
		}

		if ( TIMER_Done( aiEnt, "duck" ) && aiEnt->client->pers.cmd.upmove < 0) aiEnt->client->pers.cmd.upmove = 0;
	}
}
