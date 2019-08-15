//
// NPC.cpp - generic functions
//
#include "b_local.h"
#include "anims.h"
#include "say.h"
#include "icarus/Q3_Interface.h"
#include "ai_dominance_main.h"

#define __NPC_STRAFE__

#define WAYPOINT_NONE -1

extern bot_state_t *botstates[MAX_GENTITIES];

extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
extern void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time );
extern void NPC_BSNoClip (gentity_t *aiEnt);
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void NPC_ApplyRoff (gentity_t *aiEnt);
extern void NPC_TempLookTarget ( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern void NPC_CheckPlayerAim (gentity_t *aiEnt);
extern void NPC_CheckAllClear (gentity_t *aiEnt);
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern qboolean NPC_CheckLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void Mark1_dying( gentity_t *self );
extern void NPC_BSCinematic(gentity_t *aiEnt);
extern int GetTime ( int lastTime );
extern void NPC_BSGM_Default(gentity_t *aiEnt);
extern void NPC_CheckCharmed(gentity_t *aiEnt);
extern qboolean Boba_Flying( gentity_t *self );
extern int DOM_GetNearestWP(vec3_t org, int badwp);
extern int DOM_GetNearWP(vec3_t org, int badwp);
extern void Jedi_Move( gentity_t *aiEnt, gentity_t *goal, qboolean retreat );
extern void NPC_EnforceConversationRange ( gentity_t *self );
extern qboolean NPC_CombatMoveToGoal(gentity_t *aiEnt, qboolean tryStraight, qboolean retreat );
qboolean UQ_MoveDirClear( gentity_t *aiEnt, int forwardmove, int rightmove, qboolean reset );
extern qboolean NPC_IsJetpacking ( gentity_t *self );
extern void ST_Speech( gentity_t *self, int speechType, float failChance );
extern void BubbleShield_Update(gentity_t *aiEnt);
extern qboolean NPC_IsCombatPathing(gentity_t *aiEnt);

// Conversations...
extern void NPC_NPCConversation(gentity_t *aiEnt);
extern void NPC_FindConversationPartner(gentity_t *aiEnt);
extern void NPC_StormTrooperConversation(gentity_t *aiEnt);

//Local Variables
extern npcStatic_t NPCS;

void NPC_SetAnim(gentity_t	*ent,int type,int anim,int priority);
void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope );
extern void GM_Dying( gentity_t *self );
extern qboolean G_EntIsBreakable( int entityNum );

extern int eventClearTime;

qboolean NPC_EntityIsBreakable ( gentity_t *self, gentity_t *ent )
{
	if (ent
		&& ent->inuse
		&& ent->takedamage
		&& ent->classname 
		&& ent->classname[0] 
		&& ent->s.eType != ET_INVISIBLE
		&& ent->s.eType != ET_NPC
		&& ent->s.eType != ET_PLAYER
		&& !ent->client
		&& G_EntIsBreakable(ent->s.number)
		&& !EntIsGlass(ent) // UQ1: Ignore glass...
		&& ent->health > 0
		&& !(ent->r.svFlags & SVF_PLAYER_USABLE))
	{
#if 0 // UQ1: Removed restrictions on these...
		if ( (ent->flags & FL_DMG_BY_SABER_ONLY) 
			&& self->s.weapon != WP_SABER )
		{
			return qfalse;
		}

		if ( (ent->flags & FL_DMG_BY_HEAVY_WEAP_ONLY) 
			&& self->s.weapon != WP_SABER
			&& self->s.weapon != WP_THERMAL )
		{// Heavy weapons only... FIXME: Add new weapons???
			return qfalse;
		}
#endif

		return qtrue;
	}

	return qfalse;
}

qboolean NPC_IsAlive (gentity_t *self, gentity_t *NPC )
{
	if (!NPC)
	{
		return qfalse;
	}

	if (self && self->client && NPC_EntityIsBreakable(self, NPC) && NPC->health > 0)
	{
		return qtrue;
	}

	if ( NPC->s.eType == ET_NPC || NPC->s.eType == ET_PLAYER )
	{
		if (NPC->client && NPC->client->ps.pm_type == PM_SPECTATOR)
		{
			return qfalse;
		}

		if (NPC->health <= 0 && (NPC->client && NPC->client->ps.stats[STAT_HEALTH] <= 0))
		{
			return qfalse;
		}
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

int NPC_GetHealthPercent ( gentity_t *self, gentity_t *NPC )
{
	if (!NPC)
	{
		return 0;
	}

	if (self && NPC_EntityIsBreakable(self, NPC))
	{
		return (int)(((float)NPC->health / (float)NPC->maxHealth) * 100);
	}

	if ( NPC->s.eType == ET_NPC || NPC->s.eType == ET_PLAYER )
	{
		if (NPC->client && NPC->client->ps.pm_type == PM_SPECTATOR)
		{
			return 0;
		}

		if (NPC->client)
		{
			int health = NPC->client->ps.stats[STAT_HEALTH];
			int maxhealth = NPC->client->ps.stats[STAT_MAX_HEALTH];

			return (int)(((float)health / (float)maxhealth) * 100);
		}
		else if (NPC->s.eType != ET_PLAYER && NPC->health <= 0 )
		{
			return (int)(((float)NPC->health / (float)NPC->maxHealth) * 100);
		}
	}
	else
	{
		return (int)(((float)NPC->health / (float)NPC->maxHealth) * 100);
	}

	return (int)(((float)NPC->health / (float)NPC->maxHealth) * 100);
}

qboolean NPC_NeedsHeal ( gentity_t *NPC )
{
	if (NPC_GetHealthPercent(NPC, NPC) < 50)
		return qtrue;

	return qfalse;
}

void CorpsePhysics( gentity_t *self )
{
	// run the bot through the server like it was a real client
	memset( &self->client->pers.cmd, 0, sizeof( usercmd_t ) );
	ClientThink( self->s.number, &self->client->pers.cmd);
	//VectorCopy( self->s.origin, self->s.origin2 );
	//rww - don't get why this is happening.

	if ( self->client->NPC_class == CLASS_GALAKMECH )
	{
		GM_Dying( self );
	}
	//FIXME: match my pitch and roll for the slope of my groundPlane
	if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !(self->s.eFlags&EF_DISINTEGRATION) )
	{//on the ground
		//FIXME: check 4 corners
		pitch_roll_for_slope( self, NULL );
	}

	if ( eventClearTime == level.time + ALERT_CLEAR_TIME )
	{//events were just cleared out so add me again
		if ( !(self->client->ps.eFlags&EF_NODRAW) )
		{
			AddSightEvent( self->enemy, self->r.currentOrigin, 384, AEL_DISCOVERED, 0.0f );
		}
	}

	if ( level.time - self->s.time > 3000 )
	{//been dead for 3 seconds
		if ( g_dismember.integer < 11381138 && !g_saberRealisticCombat.integer )
		{//can't be dismembered once dead
			if ( self->client->NPC_class != CLASS_PROTOCOL )
			{
			//	self->client->dismembered = qtrue;
			}
		}
	}

	//if ( level.time - self->s.time > 500 )
	if (self->client->respawnTime < (level.time+500))
	{//don't turn "nonsolid" until about 1 second after actual death

		if (self->client->ps.eFlags & EF_DISINTEGRATION)
		{
			self->r.contents = 0;
		}
		else if ((self->client->NPC_class != CLASS_MARK1) && (self->client->NPC_class != CLASS_INTERROGATOR))	// The Mark1 & Interrogator stays solid.
		{
			self->r.contents = CONTENTS_CORPSE;
			//self->r.maxs[2] = -8;
		}

		if ( self->message )
		{
			self->r.contents |= CONTENTS_TRIGGER;
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/
#define REMOVE_DISTANCE		128
#define REMOVE_DISTANCE_SQR (REMOVE_DISTANCE * REMOVE_DISTANCE)

void NPC_RemoveBody( gentity_t *self )
{
	// Remove NPC name...
	self->s.NPC_NAME_ID = 0;
	memset(self->client->pers.netname, 0, sizeof(char)*36);

	CorpsePhysics( self );

	self->nextthink = level.time + FRAMETIME;

#ifndef __NO_ICARUS__
	if ( self->NPC->nextBStateThink <= level.time )
	{
		trap->ICARUS_MaintainTaskManager(self->s.number);
	}
	self->NPC->nextBStateThink = level.time + FRAMETIME;
#endif //__NO_ICARUS__

	if ( self->message )
	{//I still have a key
		trap->Print("NPC %s not removed because of key.\n", self->NPC_type);
		return;
	}

	// I don't consider this a hack, it's creative coding . . .
	// I agree, very creative... need something like this for ATST and GALAKMECH too!
	if (self->client->NPC_class == CLASS_MARK1)
	{
		Mark1_dying( self );
	}

	// Since these blow up, remove the bounding box.
	if ( self->client->NPC_class == CLASS_REMOTE
		|| self->client->NPC_class == CLASS_SENTRY
		|| self->client->NPC_class == CLASS_PROBE
		|| self->client->NPC_class == CLASS_INTERROGATOR
		|| self->client->NPC_class == CLASS_MARK2 )
	{
#ifndef __NO_ICARUS__
		//if ( !self->taskManager || !self->taskManager->IsRunning() )
		if (!trap->ICARUS_IsRunning(self->s.number))
#endif //__NO_ICARUS__
		{
			if ( !self->activator || !self->activator->client || !(self->activator->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
			{//not being held by a Rancor
				self->s.eType = ET_INVISIBLE;
				
				//G_FreeEntity( self );

				self->think = G_FreeEntity;
				self->nextthink = level.time + FRAMETIME;
			}
			else
				trap->Print("NPC %s not removed because of activator1.\n", self->NPC_type);
		}
		return;
	}

	//FIXME: don't ever inflate back up?
	self->r.maxs[2] = self->client->renderInfo.eyePoint[2] - self->r.currentOrigin[2] + 4;
	if ( self->r.maxs[2] < -8 )
	{
		self->r.maxs[2] = -8;
	}

	/*
	if ( self->client->NPC_class == CLASS_GALAKMECH )
	{//never disappears
		return;
	}
	*/

	if ( self->NPC && self->NPC->timeOfDeath <= level.time )
	{
		self->NPC->timeOfDeath = level.time + 1000;

		//FIXME: there are some conditions - such as heavy combat - in which we want
		//			to remove the bodies... but in other cases it's just weird, like
		//			when they're right behind you in a closed room and when they've been
		//			placed as dead NPCs by a designer...
		//			For now we just assume that a corpse with no enemy was
		//			placed in the map as a corpse
		//if ( self->enemy )
		{
			//if (!trap->ICARUS_IsRunning(self->s.number))
			{
				if ( !self->activator || !self->activator->client || !(self->activator->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
				{//not being held by a Rancor
					if ( self->client && self->client->ps.saberEntityNum > 0 && self->client->ps.saberEntityNum < ENTITYNUM_WORLD )
					{
						gentity_t *saberent = &g_entities[self->client->ps.saberEntityNum];

						if ( saberent )
						{
							saberent->s.eType = ET_INVISIBLE;
							G_FreeEntity( saberent );
						}
					}

#ifndef __NO_ICARUS__
					trap->ICARUS_FreeEnt((sharedEntity_t*)self); // UQ1: This???
#endif //__NO_ICARUS__
					self->s.eType = ET_INVISIBLE;

					//G_FreeEntity( self );

					self->think = G_FreeEntity;
					self->nextthink = level.time + FRAMETIME;
				}
				else
					trap->Print("NPC %s not removed because of activator2.\n", self->NPC_type);
			}
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/

int BodyRemovalPadTime( gentity_t *ent )
{
	int	time;

	if ( !ent || !ent->client )
		return 0;
/*
	switch ( ent->client->playerTeam )
	{
	case NPCTEAM_KLINGON:	// no effect, we just remove them when the player isn't looking
	case NPCTEAM_SCAVENGERS:
	case NPCTEAM_HIROGEN:
	case NPCTEAM_MALON:
	case NPCTEAM_IMPERIAL:
	case NPCTEAM_STARFLEET:
		time = 10000; // 15 secs.
		break;

	case NPCTEAM_BORG:
		time = 2000;
		break;

	case NPCTEAM_STASIS:
		return qtrue;
		break;

	case NPCTEAM_FORGE:
		time = 1000;
		break;

	case NPCTEAM_BOTS:
//		if (!Q_stricmp( ent->NPC_type, "mouse" ))
//		{
			time = 0;
//		}
//		else
//		{
//			time = 10000;
//		}
		break;

	case NPCTEAM_8472:
		time = 2000;
		break;

	default:
		// never go away
		time = Q3_INFINITE;
		break;
	}
*/
	// team no longer indicates species/race, so in this case we'd use NPC_class, but
	switch( ent->client->NPC_class )
	{
	case CLASS_MOUSE:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	//case CLASS_PROTOCOL:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_INTERROGATOR:
		time = 0;
		break;
	default:
		// never go away
	//	time = Q3_INFINITE;
		// for now I'm making default 10000
		time = 10000;
		break;

	}


	return time;
}


/*
----------------------------------------
NPC_RemoveBodyEffect

Effect to be applied when ditching the corpse
----------------------------------------
*/

static void NPC_RemoveBodyEffect(gentity_t *NPC)
{
//	vec3_t		org;
//	gentity_t	*tent;

	if ( !NPC || !NPC->client || (NPC->s.eFlags & EF_NODRAW) )
		return;
/*
	switch(NPC->client->playerTeam)
	{
	case NPCTEAM_STARFLEET:
		//FIXME: Starfleet beam out
		break;

	case NPCTEAM_BOTS:
//		VectorCopy( NPC->r.currentOrigin, org );
//		org[2] -= 16;
//		tent = G_TempEntity( org, EV_BOT_EXPLODE );
//		tent->owner = NPC;

		break;

	default:
		break;
	}
*/


	// team no longer indicates species/race, so in this case we'd use NPC_class, but

	// stub code
	switch(NPC->client->NPC_class)
	{
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	//case CLASS_PROTOCOL:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_INTERROGATOR:
	case CLASS_ATST: // yeah, this is a little weird, but for now I'm listing all droids
	//	VectorCopy( NPC->r.currentOrigin, org );
	//	org[2] -= 16;
	//	tent = G_TempEntity( org, EV_BOT_EXPLODE );
	//	tent->owner = NPC;
		break;
	default:
		break;
	}
}


/*
====================================================================
void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope )

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothinh\g
and returns.
====================================================================
*/

void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope )
{
	vec3_t	slope;
	vec3_t	nvf, ovf, ovr, startspot, endspot, new_angles = { 0, 0, 0 };
	float	pitch, mod, dot;

	//if we don't have a slope, get one
	if( !pass_slope || VectorCompare( vec3_origin, pass_slope ) )
	{
		trace_t trace;

		VectorCopy( forwhom->r.currentOrigin, startspot );
		startspot[2] += forwhom->r.mins[2] + 4;
		VectorCopy( startspot, endspot );
		endspot[2] -= 300;
		trap->Trace( &trace, forwhom->r.currentOrigin, vec3_origin, vec3_origin, endspot, forwhom->s.number, MASK_SOLID, qfalse, 0, 0 );
//		if(trace_fraction>0.05&&forwhom.movetype==MOVETYPE_STEP)
//			forwhom.flags(-)FL_ONGROUND;

		if ( trace.fraction >= 1.0 )
			return;

		if( !( &trace.plane ) )
			return;

		if ( VectorCompare( vec3_origin, trace.plane.normal ) )
			return;

		VectorCopy( trace.plane.normal, slope );
	}
	else
	{
		VectorCopy( pass_slope, slope );
	}


	AngleVectors( forwhom->r.currentAngles, ovf, ovr, NULL );

	vectoangles( slope, new_angles );
	pitch = new_angles[PITCH] + 90;
	new_angles[ROLL] = new_angles[PITCH] = 0;

	AngleVectors( new_angles, nvf, NULL, NULL );

	mod = DotProduct( nvf, ovr );

	if ( mod<0 )
		mod = -1;
	else
		mod = 1;

	dot = DotProduct( nvf, ovf );

	if ( forwhom->client )
	{
		float oldmins2;

		forwhom->client->ps.viewangles[PITCH] = dot * pitch;
		forwhom->client->ps.viewangles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
		oldmins2 = forwhom->r.mins[2];
		forwhom->r.mins[2] = -24 + 12 * fabs(forwhom->client->ps.viewangles[PITCH])/180.0f;
		//FIXME: if it gets bigger, move up
		if ( oldmins2 > forwhom->r.mins[2] )
		{//our mins is now lower, need to move up
			//FIXME: trace?
			forwhom->client->ps.origin[2] += (oldmins2 - forwhom->r.mins[2]);
			forwhom->r.currentOrigin[2] = forwhom->client->ps.origin[2];
			trap->LinkEntity( (sharedEntity_t *)forwhom );
		}
	}
	else
	{
		forwhom->r.currentAngles[PITCH] = dot * pitch;
		forwhom->r.currentAngles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
	}
}


/*
----------------------------------------
NPC_DeadThink
----------------------------------------
*/
void NPC_DeadThink ( gentity_t *NPC )
{
	trace_t	trace;

	// Remove NPC name...
	NPC->s.NPC_NAME_ID = 0;
	memset(NPC->client->pers.netname, 0, sizeof(char)*36);

	//HACKHACKHACKHACKHACK
	//We should really have a seperate G2 bounding box (seperate from the physics bbox) for G2 collisions only
	//FIXME: don't ever inflate back up?
	NPC->r.maxs[2] = NPC->client->renderInfo.eyePoint[2] - NPC->r.currentOrigin[2] + 4;
	if ( NPC->r.maxs[2] < -8 )
	{
		NPC->r.maxs[2] = -8;
	}
	if ( VectorCompare( NPC->client->ps.velocity, vec3_origin ) )
	{//not flying through the air
		if ( NPC->r.mins[0] > -32 )
		{
			NPC->r.mins[0] -= 1;
			trap->Trace (&trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, NPC->r.currentOrigin, NPC->s.number, NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPC->r.mins[0] += 1;
			}
		}
		if ( NPC->r.maxs[0] < 32 )
		{
			NPC->r.maxs[0] += 1;
			trap->Trace (&trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, NPC->r.currentOrigin, NPC->s.number, NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPC->r.maxs[0] -= 1;
			}
		}
		if ( NPC->r.mins[1] > -32 )
		{
			NPC->r.mins[1] -= 1;
			trap->Trace (&trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, NPC->r.currentOrigin, NPC->s.number, NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPC->r.mins[1] += 1;
			}
		}
		if ( NPC->r.maxs[1] < 32 )
		{
			NPC->r.maxs[1] += 1;
			trap->Trace (&trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, NPC->r.currentOrigin, NPC->s.number, NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPC->r.maxs[1] -= 1;
			}
		}
	}
	//HACKHACKHACKHACKHACK

	//death anim done (or were given a specific amount of time to wait before removal), wait the requisite amount of time them remove
	if ( level.time >= NPC->NPC->timeOfDeath + BodyRemovalPadTime( NPC ) )
	{
		if ( NPC->client->ps.eFlags & EF_NODRAW )
		{
			//if (!trap->ICARUS_IsRunning(NPC->s.number))
			{
				if ( NPC->client && NPC->client->ps.saberEntityNum > 0 && NPC->client->ps.saberEntityNum < ENTITYNUM_WORLD )
				{
					gentity_t *saberent = &g_entities[NPC->client->ps.saberEntityNum];

					if ( saberent )
					{
						G_FreeEntity( saberent );
					}
				}

#ifndef __NO_ICARUS__
				trap->ICARUS_FreeEnt((sharedEntity_t*)NPC); // UQ1: This???
#endif //__NO_ICARUS__
				//G_FreeEntity( NPC );

				NPC->think = G_FreeEntity;
				NPC->nextthink = level.time + FRAMETIME;
			}
			//trap->ICARUS_FreeEnt(NPC); // UQ1: This???
		}
		else
		{
			class_t	npc_class;

			// Start the body effect first, then delay 400ms before ditching the corpse
			NPC_RemoveBodyEffect(NPC);

			if ( NPC->client && NPC->client->ps.saberEntityNum > 0 && NPC->client->ps.saberEntityNum < ENTITYNUM_WORLD )
			{
				gentity_t *saberent = &g_entities[NPC->client->ps.saberEntityNum];

				if ( saberent )
				{
					G_FreeEntity( saberent );
				}
			}

			//FIXME: keep it running through physics somehow?
			NPC->think = NPC_RemoveBody;
			NPC->nextthink = level.time + FRAMETIME;

			npc_class = NPC->client->NPC_class;

			// check for droids
			if ( npc_class == CLASS_SEEKER || npc_class == CLASS_REMOTE || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
				npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
				npc_class == CLASS_MARK2 || npc_class == CLASS_SENTRY )//npc_class == CLASS_PROTOCOL ||
			{
				NPC->client->ps.eFlags |= EF_NODRAW;
				NPC->NPC->timeOfDeath = level.time + FRAMETIME * 8;
			}
			else
				NPC->NPC->timeOfDeath = level.time + FRAMETIME * 4;
		}
		return;
	}

	// If the player is on the ground and the resting position contents haven't been set yet...(BounceCount tracks the contents)
	if ( NPC->bounceCount < 0 && NPC->s.groundEntityNum >= 0 )
	{
		// if client is in a nodrop area, make him/her nodraw
		int contents = NPC->bounceCount = trap->PointContents( NPC->r.currentOrigin, -1 );

		if ( ( contents & CONTENTS_NODROP ) )
		{
			NPC->client->ps.eFlags |= EF_NODRAW;
		}
	}

	CorpsePhysics( NPC );
}

extern	qboolean	showBBoxes;
vec3_t NPCDEBUG_RED = {1.0, 0.0, 0.0};
vec3_t NPCDEBUG_GREEN = {0.0, 1.0, 0.0};
vec3_t NPCDEBUG_BLUE = {0.0, 0.0, 1.0};
vec3_t NPCDEBUG_LIGHT_BLUE = {0.3f, 0.7f, 1.0};
extern void G_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
extern void G_Line( vec3_t start, vec3_t end, vec3_t color, float alpha );
extern void G_Cylinder( vec3_t start, vec3_t end, float radius, vec3_t color );

void NPC_ShowDebugInfo (void)
{
	if ( showBBoxes )
	{
		gentity_t	*found = NULL;
		vec3_t		mins, maxs;

		while( (found = G_Find( found, FOFS(classname), "NPC" ) ) != NULL )
		{
			if ( trap->InPVS( found->r.currentOrigin, g_entities[0].r.currentOrigin ) )
			{
				VectorAdd( found->r.currentOrigin, found->r.mins, mins );
				VectorAdd( found->r.currentOrigin, found->r.maxs, maxs );
				G_Cube( mins, maxs, NPCDEBUG_RED, 0.25 );
			}
		}
	}
}

void NPC_ApplyScriptFlags (gentity_t *aiEnt)
{
	if ( aiEnt->NPC->scriptFlags & SCF_CROUCHED )
	{
		if ( aiEnt->NPC->charmedTime > level.time && (aiEnt->client->pers.cmd.forwardmove || aiEnt->client->pers.cmd.rightmove) )
		{//ugh, if charmed and moving, ignore the crouched command
		}
		else
		{
			aiEnt->client->pers.cmd.upmove = -127;
		}
	}

	if(aiEnt->NPC->scriptFlags & SCF_RUNNING)
	{
		aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
	}
	else if(aiEnt->NPC->scriptFlags & SCF_WALKING)
	{
		if ( aiEnt->NPC->charmedTime > level.time && (aiEnt->client->pers.cmd.forwardmove || aiEnt->client->pers.cmd.rightmove) )
		{//ugh, if charmed and moving, ignore the walking command
		}
		else
		{
			aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
		}
	}
/*
	if(NPCInfo->scriptFlags & SCF_CAREFUL)
	{
		ucmd.buttons |= BUTTON_CAREFUL;
	}
*/
	if(aiEnt->NPC->scriptFlags & SCF_LEAN_RIGHT)
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_USE;
		aiEnt->client->pers.cmd.rightmove = 127;
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.upmove = 0;
	}
	else if(aiEnt->NPC->scriptFlags & SCF_LEAN_LEFT)
	{
		aiEnt->client->pers.cmd.buttons |= BUTTON_USE;
		aiEnt->client->pers.cmd.rightmove = -127;
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.upmove = 0;
	}

	if ( (aiEnt->NPC->scriptFlags & SCF_ALT_FIRE) && (aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK) )
	{//Use altfire instead
		aiEnt->client->pers.cmd.buttons |= BUTTON_ALT_ATTACK;
	}
}

void Q3_DebugPrint( int level, const char *format, ... );
void NPC_HandleAIFlags (gentity_t *aiEnt)
{
	//FIXME: make these flags checks a function call like NPC_CheckAIFlagsAndTimers
	if ( aiEnt->NPC->aiFlags & NPCAI_LOST )
	{//Print that you need help!
		//FIXME: shouldn't remove this just yet if cg_draw needs it
		aiEnt->NPC->aiFlags &= ~NPCAI_LOST;

		/*
		if ( showWaypoints )
		{
			Q3_DebugPrint(WL_WARNING, "%s can't navigate to target %s (my wp: %d, goal wp: %d)\n", NPC->targetname, NPCInfo->goalEntity->targetname, NPC->waypoint, NPCInfo->goalEntity->waypoint );
		}
		*/

		if ( aiEnt->NPC->goalEntity && aiEnt->NPC->goalEntity == aiEnt->enemy )
		{//We can't nav to our enemy
			//Drop enemy and see if we should search for him
			NPC_LostEnemyDecideChase(aiEnt);
		}
	}

	//MRJ Request:
	/*
	if ( NPCInfo->aiFlags & NPCAI_GREET_ALLIES && !NPC->enemy )//what if "enemy" is the greetEnt?
	{//If no enemy, look for teammates to greet
		//FIXME: don't say hi to the same guy over and over again.
		if ( NPCInfo->greetingDebounceTime < level.time )
		{//Has been at least 2 seconds since we greeted last
			if ( !NPCInfo->greetEnt )
			{//Find a teammate whom I'm facing and who is facing me and within 128
				NPCInfo->greetEnt = NPC_PickAlly( qtrue, 128, qtrue, qtrue );
			}

			if ( NPCInfo->greetEnt && !Q_irand(0, 5) )
			{//Start greeting someone
				qboolean	greeted = qfalse;

				//TODO:  If have a greetscript, run that instead?

				//FIXME: make them greet back?
				if( !Q_irand( 0, 2 ) )
				{//Play gesture anim (press gesture button?)
					greeted = qtrue;
					NPC_SetAnim( NPC, SETANIM_TORSO, Q_irand( BOTH_GESTURE1, BOTH_GESTURE3 ), SETANIM_FLAG_NORMAL|SETANIM_FLAG_HOLD );
					//NOTE: play full-body gesture if not moving?
				}

				if( !Q_irand( 0, 2 ) )
				{//Play random voice greeting sound
					greeted = qtrue;
					//FIXME: need NPC sound sets

					//G_AddVoiceEvent( NPC, Q_irand(EV_GREET1, EV_GREET3), 2000 );
				}

				if( !Q_irand( 0, 1 ) )
				{//set looktarget to them for a second or two
					greeted = qtrue;
					NPC_TempLookTarget( NPC, NPCInfo->greetEnt->s.number, 1000, 3000 );
				}

				if ( greeted )
				{//Did at least one of the things above
					//Don't greet again for 2 - 4 seconds
					NPCInfo->greetingDebounceTime = level.time + Q_irand( 2000, 4000 );
					NPCInfo->greetEnt = NULL;
				}
			}
		}
	}
	*/
	//been told to play a victory sound after a delay
	if ( aiEnt->NPC->greetingDebounceTime && aiEnt->NPC->greetingDebounceTime < level.time )
	{
		G_AddVoiceEvent( aiEnt, Q_irand(EV_VICTORY1, EV_VICTORY3), Q_irand( 2000, 4000 ) );
		aiEnt->NPC->greetingDebounceTime = 0;
	}

	if ( aiEnt->NPC->ffireCount > 0 )
	{
		if ( aiEnt->NPC->ffireFadeDebounce < level.time )
		{
			aiEnt->NPC->ffireCount--;
			//Com_Printf( "drop: %d < %d\n", NPCInfo->ffireCount, 3+((2-g_npcspskill.integer)*2) );
			aiEnt->NPC->ffireFadeDebounce = level.time + 3000;
		}
	}
	if ( d_patched.integer )
	{//use patch-style navigation
		if ( aiEnt->NPC->consecutiveBlockedMoves > 20 )
		{//been stuck for a while, try again?
			aiEnt->NPC->consecutiveBlockedMoves = 0;
		}
	}
}

void NPC_AvoidWallsAndCliffs (gentity_t *aiEnt)
{
	//...
	if (aiEnt->s.groundEntityNum != ENTITYNUM_NONE
		&& !NPC_IsJetpacking( aiEnt )
		&& !UQ_MoveDirClear(aiEnt, aiEnt->client->pers.cmd.forwardmove, aiEnt->client->pers.cmd.rightmove, qfalse ))
	{// Moving here would fall... Never do it!
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.rightmove = 0;
		aiEnt->client->pers.cmd.upmove = 0;
	}
}

void NPC_CheckAttackScript(gentity_t *aiEnt)
{
	if(!(aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK))
	{
		return;
	}

	G_ActivateBehavior(aiEnt, BSET_ATTACK);
}

float NPC_MaxDistSquaredForWeapon (gentity_t *aiEnt);
void NPC_CheckAttackHold(gentity_t *aiEnt)
{
	vec3_t		vec;

	// If they don't have an enemy they shouldn't hold their attack anim.
	if ( !aiEnt->enemy )
	{
		aiEnt->NPC->attackHoldTime = 0;
		return;
	}

/*	if ( ( NPC->client->ps.weapon == WP_BORG_ASSIMILATOR ) || ( NPC->client->ps.weapon == WP_BORG_DRILL ) )
	{//FIXME: don't keep holding this if can't hit enemy?

		// If they don't have shields ( been disabled) they shouldn't hold their attack anim.
		if ( !(NPC->NPC->aiFlags & NPCAI_SHIELDS) )
		{
			NPCInfo->attackHoldTime = 0;
			return;
		}

		VectorSubtract(NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, vec);
		if( VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon() )
		{
			NPCInfo->attackHoldTime = 0;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, 0);
		}
		else if( NPCInfo->attackHoldTime && NPCInfo->attackHoldTime > level.time )
		{
			ucmd.buttons |= BUTTON_ATTACK;
		}
		else if ( ( NPCInfo->attackHold ) && ( ucmd.buttons & BUTTON_ATTACK ) )
		{
			NPCInfo->attackHoldTime = level.time + NPCInfo->attackHold;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, NPCInfo->attackHold);
		}
		else
		{
			NPCInfo->attackHoldTime = 0;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, 0);
		}
	}
	else*/
	{//everyone else...?  FIXME: need to tie this into AI somehow?
		if (aiEnt->enemy && !NPC_ValidEnemy(aiEnt, aiEnt->enemy))
			return;

		VectorSubtract(aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, vec);
		if( VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon(aiEnt) )
		{
			aiEnt->NPC->attackHoldTime = 0;
		}
		else if( aiEnt->NPC->attackHoldTime && aiEnt->NPC->attackHoldTime > level.time )
		{
			aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
		}
		else if ( ( aiEnt->NPC->attackHold ) && ( aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK ) )
		{
			aiEnt->NPC->attackHoldTime = level.time + aiEnt->NPC->attackHold;
		}
		else
		{
			aiEnt->NPC->attackHoldTime = 0;
		}
	}
}

/*
void NPC_KeepCurrentFacing(void)

Fills in a default ucmd to keep current angles facing
*/
void NPC_KeepCurrentFacing(gentity_t *aiEnt)
{
	if(!aiEnt->client->pers.cmd.angles[YAW])
	{
		aiEnt->client->pers.cmd.angles[YAW] = ANGLE2SHORT( aiEnt->client->ps.viewangles[YAW] ) - aiEnt->client->ps.delta_angles[YAW];
	}

	if(!aiEnt->client->pers.cmd.angles[PITCH])
	{
		aiEnt->client->pers.cmd.angles[PITCH] = ANGLE2SHORT( aiEnt->client->ps.viewangles[PITCH] ) - aiEnt->client->ps.delta_angles[PITCH];
	}
}

qboolean NPC_CanUseAdvancedFighting(gentity_t *aiEnt)
{// UQ1: Evasion/Weapon Switching/etc...
	// Who can evade???
	switch (aiEnt->client->NPC_class)
	{
	//case CLASS_ATST:
	case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:		
	case CLASS_CLAW:
	case CLASS_COMMANDO:
	case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	case CLASS_GRAN:
	//case CLASS_HOWLER:
	case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	case CLASS_JAN:				
	case CLASS_JEDI:
	case CLASS_PADAWAN:
	case CLASS_HK51:
	case CLASS_NATIVE:
	case CLASS_NATIVE_GUNNER:
	case CLASS_KYLE:				
	case CLASS_LANDO:			
	//case CLASS_LIZARD:
	case CLASS_LUKE:				
	case CLASS_MARK1:			// droid
	case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	case CLASS_MONMOTHA:			
	case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	case CLASS_MURJJ:
	case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	case CLASS_REBEL:
	case CLASS_REBORN:
	case CLASS_REELO:
	//case CLASS_REMOTE:
	case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	case CLASS_SHADOWTROOPER:
	case CLASS_STORMTROOPER:
	case CLASS_STORMTROOPER_ADVANCED:
	case CLASS_SWAMP:
	case CLASS_SWAMPTROOPER:
	case CLASS_TAVION:
	case CLASS_TRANDOSHAN:
	case CLASS_UGNAUGHT:
	case CLASS_JAWA:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
		// OK... EVADE AWAY!!!
		break;
	default:
		// NOT OK...
		return qfalse;
		break;
	}

	return qtrue;
}

/*
-------------------------
NPC_BehaviorSet_Charmed
-------------------------
*/

void NPC_BehaviorSet_Charmed( gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_FOLLOW_LEADER://# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader(aiEnt);
		break;
	case BS_REMOVE:
		NPC_BSRemove(aiEnt);
		break;
	case BS_SEARCH:			//# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch(aiEnt);
		break;
	case BS_WANDER:			//# 46: Wander down random waypoint paths
		NPC_BSWander(aiEnt);
		break;
	case BS_FLEE:
		NPC_BSFlee(aiEnt);
		break;
	default:
	case BS_DEFAULT://whatever
		NPC_BSDefault(aiEnt);
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Default
-------------------------
*/

void NPC_BehaviorSet_Default( gentity_t *aiEnt, int bState )
{
	if ( aiEnt->enemy && aiEnt->enemy->inuse && aiEnt->enemy->health > 0)
	{// UQ1: Have an anemy... Check if we should use advanced fighting for this NPC...
		if ( NPC_CanUseAdvancedFighting(aiEnt) )
		{// UQ1: This NPC can use advanced tactics... Use them!!!
			NPC_BSJedi_Default(aiEnt);
			return;
		}
	}

	switch( bState )
	{
	case BS_ADVANCE_FIGHT://head toward captureGoal, shoot anything that gets in the way
		NPC_BSAdvanceFight (aiEnt);
		break;
	case BS_SLEEP://Follow a path, looking for enemies
		NPC_BSSleep (aiEnt);
		break;
	case BS_FOLLOW_LEADER://# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader(aiEnt);
		break;
	case BS_JUMP:			//41: Face navgoal and jump to it.
		NPC_BSJump(aiEnt);
		break;
	case BS_REMOVE:
		NPC_BSRemove(aiEnt);
		break;
	case BS_SEARCH:			//# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch(aiEnt);
		break;
	case BS_NOCLIP:
		NPC_BSNoClip(aiEnt);
		break;
	case BS_WANDER:			//# 46: Wander down random waypoint paths
		NPC_BSWander(aiEnt);
		break;
	case BS_FLEE:
		NPC_BSFlee(aiEnt);
		break;
	case BS_WAIT:
		NPC_BSWait(aiEnt);
		break;
	case BS_CINEMATIC:
		NPC_BSCinematic(aiEnt);
		break;
	default:
	case BS_DEFAULT://whatever
		NPC_BSDefault(aiEnt);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Interrogator
-------------------------
*/
void NPC_BehaviorSet_Interrogator(gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSInterrogator_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

void NPC_BSImperialProbe_Attack( gentity_t *aiEnt);
void NPC_BSImperialProbe_Patrol(gentity_t *aiEnt);
void NPC_BSImperialProbe_Wait(gentity_t *aiEnt);

/*
-------------------------
NPC_BehaviorSet_ImperialProbe
-------------------------
*/
void NPC_BehaviorSet_ImperialProbe(gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSImperialProbe_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}


void NPC_BSSeeker_Default( gentity_t *aiEnt);

/*
-------------------------
NPC_BehaviorSet_Seeker
-------------------------
*/
void NPC_BehaviorSet_Seeker( gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSeeker_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

void NPC_BSRemote_Default( gentity_t *aiEnt);

/*
-------------------------
NPC_BehaviorSet_Remote
-------------------------
*/
void NPC_BehaviorSet_Remote(gentity_t *aiEnt, int bState )
{
	NPC_BSRemote_Default(aiEnt);
}

void NPC_BSSentry_Default(gentity_t *aiEnt);

/*
-------------------------
NPC_BehaviorSet_Sentry
-------------------------
*/
void NPC_BehaviorSet_Sentry(gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSentry_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Grenadier
-------------------------
*/
void NPC_BehaviorSet_Grenadier(gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSGrenadier_Default(aiEnt);
		break;

	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Sniper
-------------------------
*/
void NPC_BehaviorSet_Sniper(gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		//NPC_BSSniper_Default(aiEnt);
		NPC_BSST_Default(aiEnt);
		break;

	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Stormtrooper
-------------------------
*/

void NPC_BehaviorSet_Stormtrooper(gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSST_Default(aiEnt);
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate(aiEnt);
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep(aiEnt);
		break;

	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Jedi
-------------------------
*/

void NPC_BehaviorSet_Jedi(gentity_t *aiEnt, int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSJedi_Default(aiEnt);
		break;

	case BS_FOLLOW_LEADER:
		NPC_BSJedi_FollowLeader(aiEnt);
		break;

	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Droid
-------------------------
*/
void NPC_BehaviorSet_Droid(gentity_t *aiEnt, int bState)
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSDroid_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_SandCreature
-------------------------
*/
extern void NPC_BSSandCreature_Default( gentity_t *aiEnt);
void NPC_BehaviorSet_SandCreature(gentity_t *aiEnt, int bState)
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSandCreature_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark1
-------------------------
*/
void NPC_BehaviorSet_Mark1(gentity_t *aiEnt, int bState)
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSMark1_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark2
-------------------------
*/
void NPC_BehaviorSet_Mark2(gentity_t *aiEnt, int bState)
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSMark2_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_ATST
-------------------------
*/
void NPC_BehaviorSet_ATST(gentity_t *aiEnt, int bState)
{
	/*
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSATST_Default(aiEnt);
		break;
	default:
		if (NPC_HaveValidEnemy(aiEnt))
			NPC_BSATST_Default(aiEnt);
		else
			NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
	*/

	/*
	extern void ATST_Attack(gentity_t *aiEnt);

	if (NPC_HaveValidEnemy(aiEnt))
		ATST_Attack(aiEnt);
	else
		NPC_BehaviorSet_Default(aiEnt, bState);
	*/

	NPC_BehaviorSet_Jedi(aiEnt, bState);
}

/*
-------------------------
NPC_BehaviorSet_MineMonster
-------------------------
*/
void NPC_BehaviorSet_MineMonster(gentity_t *aiEnt, int bState)
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSMineMonster_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Howler
-------------------------
*/
void NPC_BehaviorSet_Howler(gentity_t *aiEnt, int bState)
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSHowler_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Rancor
-------------------------
*/
void NPC_BehaviorSet_Rancor(gentity_t *aiEnt, int bState)
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSRancor_Default(aiEnt);
		break;
	default:
		NPC_BehaviorSet_Default(aiEnt, bState );
		break;
	}
}

void ST_SelectBestWeapon( gentity_t *aiEnt )
{
	if (aiEnt->next_weapon_switch > level.time) return;

	if (!(aiEnt->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (!aiEnt->enemy) return;

	NPC_ChangeWeapon(aiEnt, WP_MODULIZED_WEAPON );
}

void Commando2_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->next_weapon_switch > level.time) return;

	if (!aiEnt->enemy) return;

	if (!(aiEnt->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	/*if (aiEnt->client->ps.weapon != WP_DISRUPTOR 
		&& DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin )>(700*700) )
	{
		NPC_ChangeWeapon(aiEnt, WP_SOME_SNIPER_WEAPON );
	}
	else*/ if (aiEnt->client->ps.weapon != WP_MODULIZED_WEAPON 
		&& DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin )>(300*300) )
	{
		NPC_ChangeWeapon(aiEnt, WP_THERMAL );
	}
	else
	{
		NPC_ChangeWeapon(aiEnt, WP_MODULIZED_WEAPON );
	}
}

void Sniper_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->next_weapon_switch > level.time) return;

	if (!(aiEnt->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (!aiEnt->enemy) return;

	if ( aiEnt->client->ps.weapon != WP_MODULIZED_WEAPON 
		&& DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin )>(300*300) )
	{
		NPC_ChangeWeapon(aiEnt, WP_MODULIZED_WEAPON );
	}
}

void Rocketer_SelectBestWeapon( gentity_t *aiEnt)
{
	if (aiEnt->next_weapon_switch > level.time) return;

	if (!(aiEnt->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (!aiEnt->enemy) return;

	/*if ( aiEnt->client->ps.weapon != WP_ROCKET_LAUNCHER 
		&& DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin )>(600*600) )
	{
		NPC_ChangeWeapon(aiEnt, WP_ROCKET_LAUNCHER );
	}
	else*/ if ( aiEnt->client->ps.weapon != WP_MODULIZED_WEAPON 
		&& DistanceSquared( aiEnt->r.currentOrigin, aiEnt->enemy->r.currentOrigin )>(300*300) )
	{
		NPC_ChangeWeapon(aiEnt, WP_MODULIZED_WEAPON );
	}
}

/*
-------------------------
NPC_RunBehavior
-------------------------
*/
extern void NPC_BSSD_Default(gentity_t *aiEnt);
extern void NPC_BSEmplaced( gentity_t *aiEnt);
extern qboolean NPC_CheckSurrender(gentity_t *aiEnt);
extern void Boba_FlyStop( gentity_t *self );
extern void NPC_BSWampa_Default(gentity_t *aiEnt);
extern qboolean Jedi_CultistDestroyer( gentity_t *self );
void NPC_RunBehavior( gentity_t *aiEnt, int team, int bState )
{
	if (aiEnt->s.NPC_class == CLASS_VEHICLE &&
		aiEnt->m_pVehicle)
	{ //vehicles don't do AI!
		return;
	}

	if ( bState == BS_CINEMATIC )
	{
		NPC_BSCinematic(aiEnt);
	}
	else if ( !TIMER_Done(aiEnt, "DEMP2_StunTime"))
	{//we've been stunned by a demp2 shot.
		NPC_UpdateAngles(aiEnt, qtrue, qtrue);
		return;
	}
	else if ( aiEnt->client->ps.weapon == WP_EMPLACED_GUN )
	{
		NPC_BSEmplaced(aiEnt);
		NPC_CheckCharmed(aiEnt);
		return;
	}
	else if ( aiEnt->client->NPC_class == CLASS_JEDI 
		|| aiEnt->client->NPC_class == CLASS_PADAWAN 
		|| aiEnt->client->NPC_class == CLASS_HK51
		|| aiEnt->client->NPC_class == CLASS_REBORN
		|| aiEnt->client->NPC_class == CLASS_TAVION
		|| aiEnt->client->NPC_class == CLASS_ALORA
		|| aiEnt->client->NPC_class == CLASS_DESANN
		|| aiEnt->client->NPC_class == CLASS_KYLE
		|| aiEnt->client->NPC_class == CLASS_LUKE
		|| aiEnt->client->NPC_class == CLASS_MORGANKATARN
		|| aiEnt->client->NPC_class == CLASS_MONMOTHA
		|| aiEnt->client->NPC_class == CLASS_SHADOWTROOPER
		|| aiEnt->client->NPC_class == CLASS_JAN
		|| (aiEnt->client->ps.eFlags & EF_FAKE_NPC_BOT) /* temporary */ )
	{//jedi
		NPC_BehaviorSet_Jedi(aiEnt, bState );
	}
	else if ( aiEnt->client->NPC_class == CLASS_HOWLER
		|| aiEnt->client->NPC_class == CLASS_REEK
		|| aiEnt->client->NPC_class == CLASS_NEXU
		/*|| aiEnt->client->NPC_class == CLASS_ACKLAY*/)
	{
		NPC_BehaviorSet_Howler(aiEnt, bState );
		return;
	}
	else if ( Jedi_CultistDestroyer( aiEnt ) )
	{
		NPC_BSJedi_Default(aiEnt);
	}
	else if ( aiEnt->client->NPC_class == CLASS_SABER_DROID )
	{//saber droid
		NPC_BSSD_Default(aiEnt);
	}
	else if (aiEnt->client->NPC_class == CLASS_GLIDER)
	{
		NPC_BehaviorSet_Seeker(aiEnt, bState);
	}
	else if ( aiEnt->client->NPC_class == CLASS_WAMPA )
	{//wampa
		NPC_BSWampa_Default(aiEnt);
	}
	else if ( aiEnt->client->NPC_class == CLASS_RANCOR )
	{//rancor
		NPC_BehaviorSet_Rancor(aiEnt, bState );
	}
	else if ( aiEnt->client->NPC_class == CLASS_REMOTE )
	{
		NPC_BehaviorSet_Remote(aiEnt, bState );
	}
	else if ( aiEnt->client->NPC_class == CLASS_SEEKER )
	{
		NPC_BehaviorSet_Seeker(aiEnt, bState );
	}
	else if ( aiEnt->client->NPC_class == CLASS_BOBAFETT || aiEnt->hasJetpack )
	{//bounty hunter
		if ( Boba_Flying( aiEnt ) )
		{
			NPC_BehaviorSet_Seeker(aiEnt, bState);
		}
		else
		{
			NPC_BehaviorSet_Jedi(aiEnt, bState );
		}
	}
	else if ( Jedi_CultistDestroyer( aiEnt ) )
	{
		NPC_BSJedi_Default(aiEnt);
	}
	else if (aiEnt->client->NPC_class == CLASS_ATST)
	{
		NPC_BehaviorSet_ATST(aiEnt, bState);
	}

	/*else if ( aiEnt->NPC->scriptFlags & SCF_FORCED_MARCH )
	{//being forced to march
		NPC_BSDefault(aiEnt);
	}*/
	else
	{
		switch( team )
		{
		// not sure if TEAM_ENEMY is appropriate here, I think I should be using NPC_class to check for behavior - dmv
		case NPCTEAM_ENEMY:
			// special cases for enemy droids
			switch( aiEnt->client->NPC_class)
			{
			case CLASS_ATST:
				NPC_BehaviorSet_ATST(aiEnt, bState );
				return;
			case CLASS_PROBE:
				NPC_BehaviorSet_ImperialProbe(aiEnt, bState);
				return;
			case CLASS_REMOTE:
				NPC_BehaviorSet_Remote(aiEnt, bState );
				return;
			case CLASS_SENTRY:
				NPC_BehaviorSet_Sentry(aiEnt, bState);
				return;
			case CLASS_INTERROGATOR:
				NPC_BehaviorSet_Interrogator(aiEnt, bState );
				return;
			case CLASS_MINEMONSTER:
				NPC_BehaviorSet_MineMonster(aiEnt, bState );
				return;
			case CLASS_HOWLER:
				NPC_BehaviorSet_Howler(aiEnt, bState);
				return;
			case CLASS_REEK:
				//NPC_BehaviorSet_Jedi(aiEnt, bState);
				NPC_BehaviorSet_Howler(aiEnt, bState);
				return;
			case CLASS_NEXU:
				NPC_BehaviorSet_Howler(aiEnt, bState);
				//NPC_BehaviorSet_Rancor(aiEnt, bState);
				return;
			case CLASS_ACKLAY:
				NPC_BehaviorSet_Rancor(aiEnt, bState );
				return;
			case CLASS_MARK1:
				NPC_BehaviorSet_Mark1(aiEnt, bState );
				return;
			case CLASS_MARK2:
				NPC_BehaviorSet_Mark2(aiEnt, bState );
				return;
			case CLASS_GALAKMECH:
				NPC_BSGM_Default(aiEnt);
				return;
			case CLASS_SAND_CREATURE:
				NPC_BehaviorSet_SandCreature(aiEnt, bState );
				return;
			case CLASS_TUSKEN:
				Sniper_SelectBestWeapon(aiEnt);

				if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && (aiEnt->NPC->scriptFlags & SCF_ALT_FIRE))
				{//a sniper
					NPC_BehaviorSet_Sniper(aiEnt, bState );
					return;
				}
				else
				{
					NPC_BehaviorSet_Stormtrooper(aiEnt, bState );
					return;
				}
				return;
			case CLASS_MERC:
				Commando2_SelectBestWeapon(aiEnt);

				if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && (aiEnt->NPC->scriptFlags & SCF_ALT_FIRE))
				{//a sniper
					NPC_BehaviorSet_Sniper(aiEnt, bState );
					return;
				}
				else
				{
					NPC_BehaviorSet_Stormtrooper(aiEnt, bState );
					return;
				}
				return;
			case CLASS_SABOTEUR:
			case CLASS_HAZARD_TROOPER:
			case CLASS_REELO:
			case CLASS_COMMANDO:
				Commando2_SelectBestWeapon(aiEnt);

				if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && (aiEnt->NPC->scriptFlags & SCF_ALT_FIRE))
				{//a sniper
					NPC_BehaviorSet_Sniper(aiEnt, bState );
					return;
				}
				else
				{
					NPC_BehaviorSet_Stormtrooper(aiEnt, bState );
					return;
				}
				return;
			//case CLASS_ASSASSIN_DROID:
			case CLASS_IMPERIAL:
			case CLASS_RODIAN:
			case CLASS_TRANDOSHAN:
				if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && (aiEnt->NPC->scriptFlags & SCF_ALT_FIRE))
				{//a sniper
					NPC_BehaviorSet_Sniper(aiEnt, bState );
					return;
				}
				else
				{
					NPC_BehaviorSet_Stormtrooper(aiEnt, bState );
					return;
				}
				return;
			case CLASS_ASSASSIN_DROID:
				NPC_BSJedi_Default(aiEnt);
				BubbleShield_Update(aiEnt);
				return;
			case CLASS_NOGHRI:
			case CLASS_SABER_DROID:
				//NPC_BehaviorSet_Jedi( aiEnt, bState );
				NPC_BSJedi_Default(aiEnt);
				return;
			case CLASS_STORMTROOPER:
				ST_SelectBestWeapon(aiEnt);
				NPC_BehaviorSet_Stormtrooper(aiEnt, bState );
				return;
			case CLASS_ALORA:
				NPC_BehaviorSet_Jedi(aiEnt, bState );
				return;
			case CLASS_ROCKETTROOPER:
				Rocketer_SelectBestWeapon(aiEnt);
				NPC_BehaviorSet_Stormtrooper(aiEnt, bState );
				return;
			default:
				break;
			}

			/*if ( aiEnt->enemy && aiEnt->s.weapon == WP_NONE && bState != BS_HUNT_AND_KILL && !trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) )
			{//if in battle and have no weapon, run away, fixme: when in BS_HUNT_AND_KILL, they just stand there
				if ( bState != BS_FLEE )
				{
					NPC_StartFlee( aiEnt->enemy, aiEnt->enemy->r.currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
				}
				else
				{
					NPC_BSFlee();
				}
				return;
			}*/
			/*else if ( aiEnt->client->ps.weapon == WP_SABER )
			{//special melee exception
				NPC_BehaviorSet_Default(aiEnt, bState );
				return;
			}*/
			/*else*/ if (WeaponIsSniperCharge(aiEnt->client->ps.weapon) && (aiEnt->NPC->scriptFlags & SCF_ALT_FIRE))
			{//a sniper
				NPC_BehaviorSet_Sniper(aiEnt, bState );
				return;
			}
			else if ( aiEnt->client->ps.weapon == WP_THERMAL )//FIXME: separate AI for melee fighters
			{//a grenadier
				NPC_BehaviorSet_Grenadier(aiEnt, bState );
				return;
			}
			else if ( aiEnt->client->ps.weapon == WP_SABER )
			{//special melee exception
				//NPC_BehaviorSet_Default(aiEnt, bState );
				NPC_BehaviorSet_Jedi(aiEnt, bState );
				return;
			}
			else if ( NPC_CheckSurrender(aiEnt) )
			{
				return;
			}

			NPC_BehaviorSet_Stormtrooper(aiEnt, bState );
			break;

		case NPCTEAM_NEUTRAL:

			// special cases for enemy droids
			if ( aiEnt->client->NPC_class == CLASS_PROTOCOL || aiEnt->client->NPC_class == CLASS_UGNAUGHT ||
				aiEnt->client->NPC_class == CLASS_JAWA)
			{
				NPC_BehaviorSet_Default(aiEnt, bState);
			}
			else if ( aiEnt->client->NPC_class == CLASS_VEHICLE )
			{
				// TODO: Add vehicle behaviors here.
				NPC_UpdateAngles(aiEnt, qtrue, qtrue );//just face our spawn angles for now
			}
			else
			{
				// Just one of the average droids
				NPC_BehaviorSet_Droid(aiEnt, bState );
			}
			break;

		default:
			if ( aiEnt->client->NPC_class == CLASS_SEEKER )
			{
				NPC_BehaviorSet_Seeker(aiEnt, bState);
			}
			else
			{
				if ( aiEnt->NPC->charmedTime > level.time )
				{
					NPC_BehaviorSet_Charmed(aiEnt, bState );
				}
				else
				{
					NPC_BehaviorSet_Default(aiEnt, bState );
				}
				NPC_CheckCharmed(aiEnt);
			}
			break;
		}
	}
}

/*
===============
NPC_ExecuteBState

  MCG

NPC Behavior state thinking

===============
*/
void NPC_ExecuteBState ( gentity_t *self )//, int msec )
{
	bState_t	bState;
	gentity_t	*aiEnt = self;

	if (aiEnt->s.weapon < 0) aiEnt->s.weapon = WP_NONE;

	NPC_HandleAIFlags(aiEnt);

	//FIXME: these next three bits could be a function call, some sort of setup/cleanup func
	//Lookmode must be reset every think cycle
	if(aiEnt->delayScriptTime && aiEnt->delayScriptTime <= level.time)
	{
		G_ActivateBehavior( aiEnt, BSET_DELAYED);
		aiEnt->delayScriptTime = 0;
	}

	//Clear this and let bState set it itself, so it automatically handles changing bStates... but we need a set bState wrapper func
	aiEnt->NPC->combatMove = qfalse;

	//Execute our bState
	if(aiEnt->NPC->tempBehavior)
	{//Overrides normal behavior until cleared
		bState = aiEnt->NPC->tempBehavior;
	}
	else
	{
		if(!aiEnt->NPC->behaviorState)
			aiEnt->NPC->behaviorState = aiEnt->NPC->defaultBehavior;

		bState = aiEnt->NPC->behaviorState;
	}

	//Pick the proper bstate for us and run it
	NPC_RunBehavior( self, self->client->playerTeam, bState );


//	if(bState != BS_POINT_COMBAT && NPCInfo->combatPoint != -1)
//	{
		//level.combatPoints[NPCInfo->combatPoint].occupied = qfalse;
		//NPCInfo->combatPoint = -1;
//	}

	//Here we need to see what the scripted stuff told us to do
//Only process snapshot if independent and in combat mode- this would pick enemies and go after needed items
//	ProcessSnapshot();

//Ignore my needs if I'm under script control- this would set needs for items
//	CheckSelf();

	//Back to normal?  All decisions made?

	//FIXME: don't walk off ledges unless we can get to our goal faster that way, or that's our goal's surface
	//NPCPredict();

	if ( aiEnt->enemy )
	{
		if ( !aiEnt->enemy->inuse )
		{//just in case bState doesn't catch this
			G_ClearEnemy( aiEnt );
		}
	}

	if ( aiEnt->client->ps.saberLockTime && aiEnt->client->ps.saberLockEnemy != ENTITYNUM_NONE )
	{
		NPC_SetLookTarget( aiEnt, aiEnt->client->ps.saberLockEnemy, level.time+1000 );
	}
	else if ( !NPC_CheckLookTarget( aiEnt ) )
	{
		if ( aiEnt->enemy )
		{
			NPC_SetLookTarget( aiEnt, aiEnt->enemy->s.number, 0 );
		}
	}

	if ( aiEnt->enemy )
	{
		if(aiEnt->enemy->flags & FL_DONT_SHOOT)
		{
			aiEnt->client->pers.cmd.buttons &= ~BUTTON_ATTACK;
			aiEnt->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		}
		else if ( aiEnt->client->playerTeam != NPCTEAM_ENEMY && aiEnt->enemy->NPC && (aiEnt->enemy->NPC->surrenderTime > level.time || (aiEnt->enemy->NPC->scriptFlags&SCF_FORCED_MARCH)) )
		{//don't shoot someone who's surrendering if you're a good guy
			aiEnt->client->pers.cmd.buttons &= ~BUTTON_ATTACK;
			aiEnt->client->pers.cmd.buttons &= ~BUTTON_ALT_ATTACK;
		}

		if(aiEnt->client->ps.weaponstate == WEAPON_IDLE)
		{
			aiEnt->client->ps.weaponstate = WEAPON_READY;
		}
	}
	else
	{
		if(aiEnt->client->ps.weaponstate == WEAPON_READY)
		{
			aiEnt->client->ps.weaponstate = WEAPON_IDLE;
		}
	}
}

void NPC_CheckInSolid( gentity_t *aiEnt )
{
	trace_t	trace;
	vec3_t	point;
	VectorCopy(aiEnt->r.currentOrigin, point);
	point[2] -= 0.25;

	trap->Trace(&trace, aiEnt->r.currentOrigin, aiEnt->r.mins, aiEnt->r.maxs, point, aiEnt->s.number, aiEnt->clipmask, qfalse, 0, 0);
	if(!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(aiEnt->r.currentOrigin, aiEnt->NPC->lastClearOrigin);
	}
	else
	{
		if(VectorLengthSquared(aiEnt->NPC->lastClearOrigin))
		{
//			Com_Printf("%s stuck in solid at %s: fixing...\n", NPC->script_targetname, vtos(NPC->r.currentOrigin));
			G_SetOrigin(aiEnt, aiEnt->NPC->lastClearOrigin);
			trap->LinkEntity((sharedEntity_t *)aiEnt);
		}
	}
}

void G_DroidSounds( gentity_t *self )
{
	if ( self->client )
	{//make the noises
		if ( TIMER_Done( self, "patrolNoise" ) && !Q_irand( 0, 20 ) )
		{
			switch( self->client->NPC_class )
			{
			case CLASS_R2D2:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r2d2/misc/r2d2talk0%d.wav",Q_irand(1, 3)) );
				break;
			case CLASS_R5D2:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r5d2/misc/r5talk%d.wav",Q_irand(1, 4)) );
				break;
			case CLASS_PROBE:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d.wav",Q_irand(1, 3)) );
				break;
			case CLASS_MOUSE:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/mouse/misc/mousego%d.wav",Q_irand(1, 3)) );
				break;
			case CLASS_GONK:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/gonk/misc/gonktalk%d.wav",Q_irand(1, 2)) );
				break;
			default:
				break;
			}
			TIMER_Set( self, "patrolNoise", Q_irand( 2000, 4000 ) );
		}
	}
}

qboolean NPC_IsVendor(gentity_t *NPC)
{
	if (!NPC->client) return qfalse;

	switch (NPC->client->NPC_class)
	{
	case CLASS_GENERAL_VENDOR:
	case CLASS_WEAPONS_VENDOR:
	case CLASS_ARMOR_VENDOR:
	case CLASS_SUPPLIES_VENDOR:
	case CLASS_FOOD_VENDOR:
	case CLASS_MEDICAL_VENDOR:
	case CLASS_GAMBLER_VENDOR:
	case CLASS_TRADE_VENDOR:
	case CLASS_ODDITIES_VENDOR:
	case CLASS_DRUG_VENDOR:
	case CLASS_TRAVELLING_VENDOR:
		return qtrue;
		break;
	default:
		break;
	}

	return qfalse;
}

qboolean NPC_IsCivilian(gentity_t *NPC)
{
	if (!NPC->client) return qfalse;

	if (NPC->client->NPC_class == CLASS_CIVILIAN
		|| NPC->client->NPC_class == CLASS_CIVILIAN_R2D2
		|| NPC->client->NPC_class == CLASS_CIVILIAN_R5D2
		|| NPC->client->NPC_class == CLASS_CIVILIAN_PROTOCOL
		|| NPC->client->NPC_class == CLASS_CIVILIAN_WEEQUAY)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean NPC_IsCivilianHumanoid(gentity_t *NPC)
{
	if (!NPC->client) return qfalse;

	if (NPC->client->NPC_class == CLASS_CIVILIAN
		|| NPC->client->NPC_class == CLASS_CIVILIAN_WEEQUAY)
	{
		return qtrue;
	}

	return qfalse;
}

void NPC_PickRandomIdleAnimantionCivilian(gentity_t *NPC)
{
#ifndef __NPCS_USE_BG_ANIMS__
	int randAnim = irand(0,10);

	if (!NPC_IsHumanoid(NPC)) return;

	switch (randAnim)
	{
	case 0:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 1:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND4, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 2:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND6, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 3:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND8, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 4:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 5:
	case 6:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9IDLE1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 7:
	case 8:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 9:
	default:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	}
#endif //__NPCS_USE_BG_ANIMS__
}

void NPC_PickRandomIdleAnimantion(gentity_t *NPC)
{
#ifndef __NPCS_USE_BG_ANIMS__
	int randAnim = 0;

	if (!NPC || !NPC->client) return;

	if (NPC->s.eType != ET_NPC && NPC->s.eType != ET_PLAYER) return;

	if (NPC->enemy) return; // No idle anims when we got an enemy...

	if (!NPC_IsHumanoid(NPC)) return;

	//VectorClear( NPC->client->ps.velocity );
	NPC->client->ps.velocity[0] = 0;
	NPC->client->ps.velocity[1] = 0;
	VectorClear( NPC->client->ps.moveDir );
	VectorClear( NPC->movedir );

	randAnim = irand(0,10);

	switch (NPC->client->ps.legsAnim)
	{
		case BOTH_STAND1:
		case BOTH_STAND2:
		case BOTH_STAND3:
		case BOTH_STAND4:
		case BOTH_STAND5:
		case BOTH_STAND6:
		case BOTH_STAND8:
		case BOTH_STAND9:
		case BOTH_STAND10:
		case BOTH_STAND9IDLE1:
		case BOTH_GUARD_IDLE1:
		case BOTH_GUARD_LOOKAROUND1:
			// Check torso also...
			if (NPC->client->ps.torsoAnim == NPC->client->ps.legsAnim)
				return; // Already running an idle animation...
		break;
	default:
		break;
	}

	if (NPC->client->lookTime > level.time) return; // Wait before next anim...

	NPC->client->lookTime = level.time + irand(5000, 15000);

	if (NPC_IsCivilian(NPC))
	{
		NPC_PickRandomIdleAnimantionCivilian(NPC);
		return;
	}

	switch (randAnim)
	{
	case 0:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND3, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
	case 1:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND4, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
	case 2:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND6, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
	case 3:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND8, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
	case 4:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
	case 5:
	case 6:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9IDLE1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 7:
	case 8:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	case 9:
	default:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
		break;
	}
#endif //__NPCS_USE_BG_ANIMS__
}

qboolean NPC_SelectMoveRunAwayAnimation( gentity_t *aiEnt)
{
#ifndef __NPCS_USE_BG_ANIMS__
	int animChoice = irand(0,1);

	if (!NPC_IsHumanoid(aiEnt)) return qfalse;

	if (aiEnt->client->pers.cmd.forwardmove < 0) 
		NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_RUNBACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	else 
		NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_RUN2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );


	if (aiEnt->npc_cower_runaway_anim == 0)
	{
		if (animChoice == 0) 
			aiEnt->npc_cower_runaway_anim = BOTH_COWER1;
		else 
			aiEnt->npc_cower_runaway_anim = TORSO_SURRENDER_START;
	}

	NPC_SetAnim(aiEnt, SETANIM_TORSO, aiEnt->npc_cower_runaway_anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);

	//trap->Print("%s is running away scared.\n", aiEnt->NPC_type);

	/*
	// UQ1: Stoiss - know any other good anims???
	BOTH_TUSKENTAUNT1
	BOTH_COWER1
	TORSO_SURRENDER_START
	BOTH_WOOKRAGE // What is this anim???
	*/

	aiEnt->client->ps.legsTimer = 200;
	aiEnt->client->ps.torsoTimer = 200;

	return qtrue;
#else //__NPCS_USE_BG_ANIMS__
	return qfalse;
#endif //__NPCS_USE_BG_ANIMS__
}

qboolean NPC_SetCivilianMoveAnim( gentity_t *aiEnt, qboolean walk )
{
#ifndef __NPCS_USE_BG_ANIMS__
	if (NPC_IsCivilianHumanoid(aiEnt))
	{// Set better torso anims when not holding a weapon.
		if (walk)
		{
			if (aiEnt->client->pers.cmd.forwardmove < 0) 
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_WALKBACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			else 
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_WALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else
		{
			if (aiEnt->client->pers.cmd.forwardmove < 0) 
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_RUNBACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			else 
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_RUN2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}

		NPC_SetAnim(aiEnt, SETANIM_TORSO, BOTH_STAND9IDLE1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);

		aiEnt->client->ps.legsTimer = 200;
		aiEnt->client->ps.torsoTimer = 200;

		//trap->Print("Civilian %s NPC anim set. Weapon %i.\n", aiEnt->client->modelname, aiEnt->client->ps.weapon);
		return qtrue;
	}
#endif //__NPCS_USE_BG_ANIMS__

	return qfalse;
}

void NPC_SelectMoveAnimation(gentity_t *aiEnt, qboolean walk)
{
#ifndef __NPCS_USE_BG_ANIMS__
	if (!NPC_IsHumanoid(aiEnt)) return;

	if (aiEnt->client->ps.crouchheight <= 0)
		aiEnt->client->ps.crouchheight = CROUCH_MAXS_2;

	if (aiEnt->client->ps.standheight <= 0)
		aiEnt->client->ps.standheight = DEFAULT_MAXS_2;

	if (NPC_SetCivilianMoveAnim(aiEnt, walk))
	{
		return;
	}

	if ((aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK) || (aiEnt->client->pers.cmd.buttons & BUTTON_ALT_ATTACK)) 
	{
		return;
	}

	if (aiEnt->client->pers.cmd.forwardmove == 0 
		&& aiEnt->client->pers.cmd.rightmove == 0 
		&& aiEnt->client->pers.cmd.upmove == 0
		&& VectorLength(aiEnt->client->ps.velocity) < 4)
	{// Standing still...
		if (aiEnt->client->ps.pm_flags & PMF_DUCKED)
		{
			NPC_SetAnim(aiEnt, SETANIM_LEGS, BOTH_CROUCH1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		}
		else if ( aiEnt->client->ps.eFlags2 & EF2_USE_ALT_ANIM )
		{//holding someone
			NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		}
		else if ( aiEnt->client->ps.eFlags2 & EF2_ALERTED )
		{//have an enemy or have had one since we spawned
			NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_STAND2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		}
		else
		{//just stand there
			//NPC_SetAnim( aiEnt, SETANIM_BOTH, BOTH_STAND1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0 );
			NPC_PickRandomIdleAnimantion(aiEnt);
			//NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		}

		aiEnt->client->ps.torsoTimer = 200;
		aiEnt->client->ps.legsTimer = 200;
	}
	else if (walk)
	{// Use walking anims..
		if (aiEnt->client->pers.cmd.forwardmove < 0)
		{
			if (aiEnt->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim(aiEnt, SETANIM_LEGS, BOTH_CROUCH1WALKBACK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else if (aiEnt->client->ps.weapon == WP_SABER)
			{// Walk with saber...
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_WALKBACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				//NPC_SetAnim( aiEnt, SETANIM_TORSO, BOTH_WALKBACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else
			{// Standard walk anim..
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_WALKBACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				//NPC_SetAnim( aiEnt, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			//trap->Print("Walking Back.\n");
		}
		else
		{
			if (aiEnt->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim(aiEnt, SETANIM_LEGS, BOTH_CROUCH1WALK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else if (aiEnt->client->ps.weapon == WP_SABER)
			{// Walk with saber...
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_WALK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				//NPC_SetAnim( aiEnt, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else
			{// Standard walk anim..
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_WALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				//NPC_SetAnim( aiEnt, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}

			//trap->Print("Walking Forward.\n");
		}

		aiEnt->client->ps.torsoTimer = 200;
		aiEnt->client->ps.legsTimer = 200;
	}
	else if ( aiEnt->client->ps.eFlags2 & EF2_USE_ALT_ANIM )
	{//full on run, on all fours
		if (aiEnt->client->ps.pm_flags & PMF_DUCKED)
		{
			NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_CROUCH1WALK, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], 0);
		}
		else 
		{
			NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_RUN1, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0 );
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], 0);
		}

		aiEnt->client->ps.torsoTimer = 200;
		aiEnt->client->ps.legsTimer = 200;
	}
	else
	{//regular, upright run
		if (aiEnt->client->pers.cmd.forwardmove < 0)
		{
			if (aiEnt->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim(aiEnt, SETANIM_LEGS, BOTH_CROUCH1WALKBACK, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], 0);
			}
			else 
			{
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_RUNBACK2, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0 );
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], 0);
			}

			//trap->Print("Running Back.\n");
		}
		else
		{
			if (aiEnt->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_CROUCH1WALK, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0);
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], 0);
			}
			else 
			{
				NPC_SetAnim( aiEnt, SETANIM_LEGS, BOTH_RUN2, /*SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD*/0 );
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], 0);
			}

			//trap->Print("Running Forward.\n");
		}

		aiEnt->client->ps.torsoTimer = 200;
		aiEnt->client->ps.legsTimer = 200;
	}

	if(aiEnt->attackDebounceTime > level.time)
	{//We just shot but aren't still shooting, so hold the gun up for a while
		if(aiEnt->client->ps.weapon == WP_SABER )
		{//One-handed
			NPC_SetAnim(aiEnt,SETANIM_TORSO,TORSO_WEAPONREADY1,SETANIM_FLAG_NORMAL);
		}
		else
		{//we just shot at someone. Hold weapon in attack anim for now
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponAttackAnim[aiEnt->s.weapon], 0);//Stoiss not sure about this one here.. testing
		}
	}
	else if (!TIMER_Done( aiEnt, "attackDelay" ))
	{
		if(aiEnt->client->ps.weapon == WP_SABER )
		{//One-handed
			NPC_SetAnim(aiEnt,SETANIM_TORSO,TORSO_WEAPONREADY1,SETANIM_FLAG_NORMAL);
		}
		else
		{//we just shot at someone. Hold weapon in attack anim for now
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponAttackAnim[aiEnt->s.weapon], 0);//Stoiss not sure about this one here.. testing
		}
	}
#endif //__NPCS_USE_BG_ANIMS__
}

typedef enum
{// Avoidance methods...
	AVOIDANCE_NONE,
	AVOIDANCE_BLOCKED,
	AVOIDANCE_STRAFE_RIGHT,
	AVOIDANCE_STRAFE_LEFT,
	AVOIDANCE_STRAFE_CROUCH,
	AVOIDANCE_STRAFE_JUMP,
} avoidanceMethods_t;

vec3_t jumpLandPosition;

qboolean NPC_NeedJump( gentity_t *aiEnt )
{
	trace_t		tr;
	vec3_t		org1, org2;
	vec3_t		forward;
	gentity_t	*NPC = aiEnt;

	VectorCopy(NPC->r.currentOrigin, org1);

	AngleVectors( NPC->r.currentAngles, forward, NULL, NULL );

	// Check jump...
	org1[2] += 8;
	forward[PITCH] = forward[ROLL] = 0;
	VectorMA( org1, 64, forward, org2 );

	if (NPC->waterlevel > 0)
	{// Always jump out of water...
		VectorCopy(org2, jumpLandPosition);
		return qtrue;
	}

	trap->Trace( &tr, org1, NULL, NULL, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );

	if (tr.fraction < 1.0f)
	{// Looks like we might need to jump... Check if it would work...
		VectorCopy(NPC->r.currentOrigin, org1);
		org1[2] += 32;
		VectorMA( org1, 64, forward, org2 );
		trap->Trace( &tr, org1, NULL, NULL, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );

		if (tr.fraction >= 0.7f)
		{// Close enough...
			//G_Printf("need jump");
			VectorCopy(org2, jumpLandPosition);
			return qtrue;
		}
	}

	return qfalse;
}

qboolean NPC_ClearPathToJump( gentity_t *NPC, vec3_t dest, int impactEntNum )
{
	trace_t	trace;
	vec3_t	mins, start, end, dir;
	float	dist, drop;
	float	i;

	//Offset the step height
	VectorSet( mins, NPC->r.mins[0], NPC->r.mins[1], NPC->r.mins[2] + STEPSIZE );
	
	trap->Trace( &trace, NPC->r.currentOrigin, mins, NPC->r.maxs, dest, NPC->s.number, NPC->clipmask, 0, 0, 0 );

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
	VectorSubtract( dest, NPC->r.currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dest[2] > NPC->r.currentOrigin[2] )
	{//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{//going down or level, check for moderate drops
		drop = 64;
	}
	for ( i = NPC->r.maxs[0]*2; i < dist; i += NPC->r.maxs[0]*2 )
	{//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA( NPC->r.currentOrigin, i, dir, start );
		VectorCopy( start, end );
		end[2] -= drop;
		trap->Trace( &trace, start, mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask, 0, 0, 0 );//NPC->r.mins?
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

extern qboolean Jedi_Jump(gentity_t *aiEnt, vec3_t dest, int goalEntNum );

#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f

extern qboolean NPC_CheckFallPositionOK(gentity_t *NPC, vec3_t position);

qboolean NPC_SimpleJump( gentity_t *NPC, vec3_t from, vec3_t to )
{
	gentity_t *aiEnt = NPC;

	if (Distance(from, to) < 512 && DistanceHorizontal(NPC->r.currentOrigin, to) > 48)
	{
		float apexHeight = to[2];
		float currentWpDist = DistanceHorizontal(NPC->r.currentOrigin, to);
		float lastWpDist = DistanceHorizontal(NPC->r.currentOrigin, from);
		float distBetweenWp = DistanceHorizontal(to, from);

		VectorSubtract(to, NPC->r.currentOrigin, NPC->movedir);

		// TODO - Visibility Check the path from apex point!

		if (from[2] > apexHeight) apexHeight = from[2];
		apexHeight += distBetweenWp / 2.0;

		if ((lastWpDist * 0.5 < currentWpDist || NPC->r.currentOrigin[2] < to[2]) 
			&& currentWpDist > 48 && apexHeight > NPC->r.currentOrigin[2])
		{
			VectorScale(NPC->movedir, 1.5, NPC->movedir);

			//NPC->client->ps.velocity[0] = NPC->movedir[0];
			//NPC->client->ps.velocity[1] = NPC->movedir[1];
			UQ1_UcmdMoveForDir_NoAvoidance( NPC, &aiEnt->client->pers.cmd, NPC->movedir, qfalse, to );

			aiEnt->client->pers.cmd.upmove = 127.0;
			if (NPC->s.eType == ET_PLAYER) trap->EA_Jump(NPC->s.number);
		}
		else
		{
			VectorScale(NPC->movedir, 1.1, NPC->movedir);

			//NPC->client->ps.velocity[0] = NPC->movedir[0];
			//NPC->client->ps.velocity[1] = NPC->movedir[1];
			UQ1_UcmdMoveForDir_NoAvoidance( NPC, &aiEnt->client->pers.cmd, NPC->movedir, qfalse, to );

			aiEnt->client->pers.cmd.upmove = 0;
		}

		//NPC_FacePosition( NPC, to, qfalse );
		NPC->npc_jumping = qtrue;

		return qtrue;
	}

	NPC->npc_jumping = qfalse;
	return qfalse;
}

qboolean NPC_Jump( gentity_t *NPC, vec3_t dest )
{//FIXME: if land on enemy, knock him down & jump off again
	/*qboolean wouldFall = qfalse;
	qboolean skipSimpleJump = qfalse;

	if (!NPC->npc_jumping)
	{
		vec3_t dir;
		VectorSubtract(dest, NPC->r.currentOrigin, dir);
		wouldFall = NPC_CheckFall(NPC, dir);
		
		if (wouldFall)
		{
			VectorCopy(NPC->r.currentOrigin, NPC->npc_jump_start);
			VectorCopy(dest, NPC->npc_jump_dest);
		}
		else
		{
			skipSimpleJump = qtrue;
		}
	}

	if (!skipSimpleJump && NPC_SimpleJump( NPC, NPC->npc_jump_start, NPC->npc_jump_dest ))
	{
		return qtrue;
	}*/

	return Jedi_Jump( NPC, dest, ENTITYNUM_NONE );
}

//extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern qboolean PM_InKnockDown( playerState_t *ps );

qboolean NPC_TryJump( gentity_t *NPC, vec3_t goal )
{//FIXME: never does a simple short, regular jump...
	gentity_t	*aiEnt = NPC;
	usercmd_t	ucmd = aiEnt->client->pers.cmd;

	if ( TIMER_Done( NPC, "jumpChaseDebounce" ) )
	{
		{
			if ( !PM_InKnockDown( &NPC->client->ps ) && !BG_InRoll( &NPC->client->ps, NPC->client->ps.legsAnim ) )
			{//enemy is on terra firma
				vec3_t goal_diff;
				float goal_z_diff;
				float goal_xy_dist;
				VectorSubtract( goal, NPC->r.currentOrigin, goal_diff );
				goal_z_diff = goal_diff[2];
				goal_diff[2] = 0;
				goal_xy_dist = VectorNormalize( goal_diff );
				if ( goal_xy_dist < 550 && goal_z_diff > -400/*was -256*/ )//for now, jedi don't take falling damage && (NPC->health > 20 || goal_z_diff > 0 ) && (NPC->health >= 100 || goal_z_diff > -128 ))//closer than @512
				{
					qboolean debounce = qfalse;
					if ( NPC->health < 150 && ((NPC->health < 30 && goal_z_diff < 0) || goal_z_diff < -128 ) )
					{//don't jump, just walk off... doesn't help with ledges, though
						debounce = qtrue;
					}
					else if ( goal_z_diff < 32 && goal_xy_dist < 200 )
					{//what is their ideal jump height?
						
						ucmd.upmove = 127;
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
							VectorCopy( goal, dest );
							
							{
								int	sideTry = 0;
								while( sideTry < 10 )
								{//FIXME: make it so it doesn't try the same spot again?
									trace_t	trace;
									vec3_t	bottom;

									VectorCopy( dest, bottom );
									bottom[2] -= 128;
									trap->Trace( &trace, dest, NPC->r.mins, NPC->r.maxs, bottom, NPC->s.number, NPC->clipmask, 0, 0, 0 );
									if ( trace.fraction < 1.0f )
									{//hit floor, okay to land here
										break;
									}
									sideTry++;
								}
								if ( sideTry >= 10 )
								{//screw it, just jump right at him?
									VectorCopy( goal, dest );
								}
							}

							if ( NPC_Jump( NPC, dest ) )
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
									//if ( NPC->client->NPC_class == CLASS_BOBAFETT 
									//	||( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG ) )
									//{//can't do acrobatics
									//	jumpAnim = BOTH_FORCEJUMP1;
									//}
									//else
									//{
										jumpAnim = BOTH_FLIP_F;
									//}
									G_SetAnim( NPC, &ucmd, SETANIM_BOTH, jumpAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
								}

								NPC->client->ps.fd.forceJumpZStart = NPC->r.currentOrigin[2];
								//NPC->client->ps.pm_flags |= PMF_JUMPING;

								NPC->client->ps.weaponTime = NPC->client->ps.torsoTimer;
								NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_LEVITATION );
								
								//if ( NPC->client->NPC_class == CLASS_BOBAFETT )
								//{
								//	G_SoundOnEnt( NPC, CHAN_ITEM, "sound/boba/jeton.wav" );
								//	NPC->client->jetPackTime = level.time + Q_irand( 1000, 15000 + irand(0, 30000) );
								//}
								//else
								//{
								//	G_SoundOnEnt( NPC, CHAN_BODY, "sound/weapons/force/jump.wav" );
								//}

								TIMER_Set( NPC, "forceJumpChasing", Q_irand( 2000, 3000 ) );
								debounce = qtrue;
							}
						}
					}

					if ( debounce )
					{
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
						ucmd.forwardmove = 127;
						VectorClear( NPC->client->ps.moveDir );
						TIMER_Set( NPC, "duck", -level.time );
						return qtrue;
					}
				}
			}
		}
	}

	return qfalse;
}

int NPC_SelectBestAvoidanceMethod ( gentity_t *aiEnt, vec3_t moveDir )
{// Just find the largest visible distance direction...
	trace_t		tr;
	vec3_t		org1, org2, goalPos;//, mins, maxs;
	vec3_t		mins = {-15, -15, DEFAULT_MINS_2};
	vec3_t		maxs = {15, 15, DEFAULT_MAXS_2};
	vec3_t		forward, right, up, angles;
	int			i = 0;
	qboolean	SKIP_RIGHT = qfalse;
	qboolean	SKIP_LEFT = qfalse;
	qboolean	TREE_BLOCK = qfalse;
	gentity_t	*NPC = aiEnt;

	if (NPC->bot_strafe_right_timer > level.time)
		return AVOIDANCE_STRAFE_RIGHT;

	if (NPC->bot_strafe_left_timer > level.time)
		return AVOIDANCE_STRAFE_LEFT;

	if (NPC->bot_strafe_crouch_timer > level.time)
		return AVOIDANCE_STRAFE_CROUCH;

	if (NPC->bot_strafe_jump_timer > level.time)
		return AVOIDANCE_STRAFE_JUMP;
	
	vectoangles(moveDir, angles);
	AngleVectors( angles, forward, right, up );

	//AngleVectors(NPC->client->ps.viewangles, forward, right, up);
	//flaten up/down
	right[2] = 0;

	VectorCopy(NPC->r.currentOrigin, org1);
	org1[2] += 32;

	VectorMA(NPC->r.currentOrigin, 24, forward, org2);
	VectorCopy(org2, goalPos);
	org2[2] += 32;

	if (FOLIAGE_TreeSolidBlocking( NPC, org2 ))
	{// Blocked by tree...
		TREE_BLOCK = qtrue;
	}

	trap->Trace( &tr, org1, mins, maxs, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
	if (!tr.startsolid && !tr.allsolid && tr.fraction >= 0.9f && !TREE_BLOCK)
	{// It is accessable normally...
		return AVOIDANCE_NONE;
	}

	{// Try left 2...
		vec3_t trypos;

		VectorCopy(NPC->r.currentOrigin, trypos);
		VectorMA(trypos, -64, right, trypos);

		VectorCopy(trypos, org1);
		org1[2] += 32;

		VectorCopy(goalPos, org2);
		org2[2] += 32;

		trap->Trace( &tr, org1, mins, maxs, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
		if (!tr.startsolid && !tr.allsolid /*&& tr.fraction >= 0.9f*/ && !FOLIAGE_TreeSolidBlocking( NPC, org2 ))
		{// It is accessable normally...
			return AVOIDANCE_STRAFE_LEFT;
		}
	}

	{// Try right 2...
		vec3_t trypos;

		VectorCopy(NPC->r.currentOrigin, trypos);
		VectorMA(trypos, 64, right, trypos);

		VectorCopy(trypos, org1);
		org1[2] += 32;

		VectorCopy(goalPos, org2);
		org2[2] += 32;

		trap->Trace( &tr, org1, mins, maxs, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
		if (!tr.startsolid && !tr.allsolid /*&& tr.fraction >= 0.9f*/ && !FOLIAGE_TreeSolidBlocking( NPC, org2 ))
		{// It is accessable normally...
			return AVOIDANCE_STRAFE_RIGHT;
		}
	}

	if (!TREE_BLOCK)
	{// Try crouch...
		vec3_t trypos, mins2, maxs2;

		VectorCopy(mins, mins2);
		VectorCopy(maxs, maxs2);
		maxs[2]-=18;

		VectorCopy(NPC->r.currentOrigin, trypos);

		VectorCopy(trypos, org1);
		org1[2] += 32;

		VectorCopy(goalPos, org2);
		org2[2] += 32;

		trap->Trace( &tr, org1, mins2, maxs2, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
		if (!tr.startsolid && !tr.allsolid && tr.fraction >= 0.9f)
		{// It is accessable normally...
			return AVOIDANCE_STRAFE_CROUCH;
		}
	
		// Try jump...
		VectorCopy(NPC->r.currentOrigin, trypos);

		VectorCopy(trypos, org1);
		org1[2] += 32;

		VectorCopy(goalPos, org2);
		org2[2] += 32;

		trap->Trace( &tr, org1, mins, maxs, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
		if (!tr.startsolid && !tr.allsolid && tr.fraction >= 0.9f)
		{// It is accessable normally...
			return AVOIDANCE_STRAFE_JUMP;
		}
	}

	return AVOIDANCE_BLOCKED;
}

qboolean NPC_NPCBlockingPath ( gentity_t *aiEnt, vec3_t moveDir )
{
	//int			i, num;
	//int			touch[MAX_GENTITIES];
	//vec3_t		mins, maxs;
	//vec3_t		range = { 64, 64, 64 };
	int			BEST_METHOD = AVOIDANCE_NONE;
	gentity_t	*NPC = aiEnt;

	if (NPC->bot_strafe_left_timer > level.time) return qtrue;
	if (NPC->bot_strafe_right_timer > level.time) return qtrue;
	if (NPC->bot_strafe_crouch_timer > level.time) return qtrue;
	if (NPC->bot_strafe_jump_timer > level.time) return qtrue;

	NPC->bot_strafe_left_timer = 0;
	NPC->bot_strafe_right_timer = 0;
	NPC->bot_strafe_crouch_timer = 0;
	NPC->bot_strafe_jump_timer = 0;

#if 0
	VectorSubtract( NPC->r.currentOrigin, range, mins );
	VectorAdd( NPC->r.currentOrigin, range, maxs );

	num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i = 0; i < num; i++)
	{
		gentity_t *ent = &g_entities[touch[i]];

		if (!ent) continue;
		if (ent == NPC) continue; // UQ1: OLD JKG Mod was missing this :)
		if (ent->s.eType != ET_PLAYER && ent->s.eType != ET_NPC) continue;
		if (ent == NPC->enemy) continue; // enemy never blocks path...
		if (ent == NPC->padawan) continue; // padawan never blocks path...
		if (ent == NPC->parent) continue; // jedi never blocks path...

		if (InFOV2(ent->r.currentOrigin, NPC, 60, 180))
		{
			NPC->bot_strafe_left_timer = level.time + 100;
			ent->bot_strafe_left_timer = level.time + 100;
			return qtrue;
		}
	}
#endif

	BEST_METHOD = NPC_SelectBestAvoidanceMethod(aiEnt, moveDir);
	
	switch (BEST_METHOD)
	{
	case AVOIDANCE_STRAFE_RIGHT:
		NPC->bot_strafe_right_timer = level.time + 100;
		return qtrue;
		break;
	case AVOIDANCE_STRAFE_LEFT:
		NPC->bot_strafe_left_timer = level.time + 100;
		return qtrue;
		break;
	case AVOIDANCE_STRAFE_CROUCH:
		NPC->bot_strafe_crouch_timer = level.time + 100;
		break;
	case AVOIDANCE_STRAFE_JUMP:
		NPC->bot_strafe_jump_timer = level.time + 100;
		break;
	case AVOIDANCE_BLOCKED:
		NPC->beStillTime = level.time + 500;
		break;
	default:
		break;
	}

	return qfalse;
}

//Adjusts the moveDir to account for strafing
void NPC_AdjustforStrafe (gentity_t *aiEnt, vec3_t moveDir, qboolean walk, float walkSpeed)
{
	gentity_t	*NPC = aiEnt;
	vec3_t right, angles;

	if (NPC->bot_strafe_right_timer < level.time && NPC->bot_strafe_left_timer < level.time)
	{//no strafe
		return;
	}

	//AngleVectors(NPC->client->ps.viewangles, NULL, right, NULL);
	vectoangles(moveDir, angles);
	AngleVectors( angles, NULL, right, NULL );

	//flaten up/down
	right[2] = 0;

	if (NPC->bot_strafe_left_timer >= level.time)
	{//strafing left
		VectorScale(right, -1, right);
	}

	//We assume that moveDir has been normalized before this function.
	VectorAdd(moveDir, right, moveDir);
	VectorNormalize(moveDir);
}

qboolean NPC_CheckFallPositionOK (gentity_t *NPC, vec3_t position)
{
	trace_t		tr;
	vec3_t testPos, downPos;
	vec3_t mins, maxs;
	gentity_t *aiEnt = NPC;

	if (NPC_IsJetpacking(NPC) || !TIMER_Done( aiEnt, "emergencyJump" )) return qtrue; // We can't fall anyway...

	VectorSet(mins, -8, -8, -1);
	VectorSet(maxs, 8, 8, 1);
	
	VectorCopy(position, testPos);
	VectorCopy(position, downPos);

	downPos[2] -= 96.0;

	if (NPC->s.groundEntityNum < ENTITYNUM_MAX_NORMAL)
		downPos[2] -= 192.0;
	
	testPos[2] += 48.0;

	trap->Trace( &tr, testPos, mins, maxs, downPos, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );

	if (tr.entityNum != ENTITYNUM_NONE)
	{
		return qtrue;
	}
	else if (tr.fraction == 1.0f)
	{
		//trap->Print("%s is holding position to not fall!\n", NPC->client->pers.netname);
		return qfalse;
	}

	return qtrue;
}

//#define __NPC_EXTENDED_FALL_CHECKING__

qboolean NPC_CheckFall (gentity_t *NPC, vec3_t dir)
{
	vec3_t forwardPos;
	gentity_t *aiEnt = NPC;

	if (NPC_IsJetpacking(NPC) || !TIMER_Done( aiEnt, "emergencyJump" )) return qfalse;

	VectorMA( NPC->r.currentOrigin, 18, dir, forwardPos );
	forwardPos[2]+=16.0;

	if (!OrgVisible(NPC->r.currentOrigin, forwardPos, NPC->s.number)) 
	{// If we can't see 18 forward, we can't move there at all... Blocked...
		return qtrue;
	}

	if (!NPC_CheckFallPositionOK(NPC, forwardPos))
	{
		return qtrue;
	}

#ifdef __NPC_EXTENDED_FALL_CHECKING__
	VectorMA( NPC->r.currentOrigin, 32, dir, forwardPos );
	forwardPos[2]+=16.0;

	if (OrgVisible(NPC->r.currentOrigin, forwardPos, NPC->s.number) && !NPC_CheckFallPositionOK(NPC, forwardPos))
	{
		return qtrue;
	}

	VectorMA( NPC->r.currentOrigin, 48, dir, forwardPos );
	forwardPos[2]+=16.0;

	if (OrgVisible(NPC->r.currentOrigin, forwardPos, NPC->s.number) && !NPC_CheckFallPositionOK(NPC, forwardPos))
	{
		return qtrue;
	}

	VectorMA( NPC->r.currentOrigin, 64, dir, forwardPos );
	forwardPos[2]+=16.0;

	if (OrgVisible(NPC->r.currentOrigin, forwardPos, NPC->s.number) && !NPC_CheckFallPositionOK(NPC, forwardPos))
	{
		return qtrue;
	}
#endif //__NPC_EXTENDED_FALL_CHECKING__

	return qfalse;
}

int NPC_CheckFallJump (gentity_t *NPC, vec3_t dest, usercmd_t *cmd)
{
	float MAX_JUMP_DISTANCE = 256.0;
	float dist = Distance(dest, NPC->r.currentOrigin);
	gentity_t *aiEnt = NPC;

	if (NPC->client->ps.groundEntityNum == ENTITYNUM_NONE) 
		return 0;

	if (NPC_IsJetpacking(NPC) || !TIMER_Done( aiEnt, "emergencyJump" )) 
		return 0;

	if (!NPC_CheckFallPositionOK(NPC, dest)) 
		return 0;

	if (NPC_IsJedi(NPC)) MAX_JUMP_DISTANCE = 512.0; // Jedi can jump further...

	if (dist <= MAX_JUMP_DISTANCE && NPC_TryJump( NPC, dest ))
	{// Looks like we can jump there... Let's do that instead of failing!
		if (NPC_Jump(NPC, dest))
			return 2; // next think...
	}

	return 0;
}

extern vmCvar_t npc_pathing;

qboolean UQ_MoveDirClear ( gentity_t *aiEnt, int forwardmove, int rightmove, qboolean reset )
{
	vec3_t	forward, right, testPos, angles, mins;
	trace_t	trace;
	float	fwdDist, rtDist;
	float	bottom_max = -STEPSIZE*4 - 1;

	if (!npc_pathing.integer)
	{// Ignore this completely when patroling, for FPS...
		return qtrue;
	}

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
#if 0
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
#endif

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

	if (NPC_IsJetpacking(aiEnt) || !TIMER_Done( aiEnt, "emergencyJump" )) 
	{// Jetpack on, we can't fall...
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

//===========================================================================
// Routine      : UQ1_UcmdMoveForDir
// Description  : Set a valid ucmd move for the current move direction... A working one, unlike raven's joke...
qboolean UQ1_UcmdMoveForDir ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest )
{
	vec3_t		forward, right;
	qboolean	jumping = qfalse;
	float		walkSpeed = 63.0;// 60.5;// 55 * 1.1;//48;//64;//32;//self->NPC->stats.walkSpeed*1.1;
	gentity_t	*aiEnt = self;

	/*
	if (self->s.eType == ET_NPC)
	{
		walkSpeed = self->NPC->stats.walkSpeed*1.1;
	}
	*/

	int waterPlane = 0;

	if (self && self->client)
	{
		waterPlane = IsBelowWaterPlane(self->client->ps.origin, self->client->ps.viewheight);
	}

	if (self->waterlevel > 0 && self->enemy && self->enemy->client && NPC_IsAlive(self, self->enemy))
	{// When we have a valid enemy, always check water level so we don't drown while attacking them...

	}
	else if (waterPlane)
	{
		if (self->client->ps.eFlags & EF_JETPACK)
		{// Fly over :)
			pm->waterlevel = 2;
			pm->watertype = CONTENTS_WATER;
		}
		else
		{
			pm->waterlevel = waterPlane;
			pm->watertype = CONTENTS_WATER;
		}
	}
	else if (gWPNum > 0 && self->wpCurrent >= 0 && self->wpCurrent < gWPNum)
	{
		if (gWPArray[self->wpCurrent]->flags & WPFLAG_WATER)
		{// Use current waypoint info to save CPU time on traces...
			self->watertype = CONTENTS_WATER;
			
			if (self->client->ps.eFlags & EF_JETPACK)
				self->waterlevel = 2;
			else
				self->waterlevel = 1;
		}
	}

	if (self->client
		&& self->client->ps.weapon == WP_SABER
		&& self->enemy
		&& self->enemy->client
		&& self->enemy->client->ps.weapon == WP_SABER
		&& NPC_IsAlive(self, self->enemy)
		&& Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) < 96.0)
	{// Jedi always walk when in combat with saber...
		walk = qtrue;
	}
	else if (self->NPC && self->NPC->forceWalkTime > level.time)
	{// Retreating NPCs walk backwards, don't run... Because thats annoying as hell to saber users...
		walk = qtrue;
	}

	if (walk)
	{
		cmd->buttons |= BUTTON_WALKING;
	}

	if (self->NPC->conversationPartner && self->NPC->conversationPartner->NPC)
	{
		cmd->upmove = 0;
		cmd->rightmove = 0;
		cmd->forwardmove = 0;
		return qfalse;
	}

	AngleVectors(self->client->ps.viewangles/*self->r.currentAngles*/, forward, right, NULL);
	
	cmd->upmove = 0;

	dir[2] = 0;
	VectorNormalize(dir);
	forward[2] = 0;
	VectorNormalize(forward);
	right[2] = 0;
	VectorNormalize(right);

#ifdef __NPC_STRAFE__
	if (self->s.groundEntityNum != ENTITYNUM_NONE)
	{// Do avoidance only when not in mid air...
		if (self->wpCurrent >= 0) 
		{// Check path for avoidance needs...
			NPC_NPCBlockingPath(self, dir);
		}

		if (self->beStillTime > level.time)
		{// Screwed...
			cmd->upmove = 0;
			cmd->rightmove = 0;
			cmd->forwardmove = 0;
			return qfalse;
		}

		if (self->bot_strafe_jump_timer > level.time)
		{
			//trap->Print("DEBUG: self->bot_strafe_jump_timer > level.time\n");
			cmd->upmove = 127.0;
			if (aiEnt->s.eType == ET_PLAYER) trap->EA_Jump(aiEnt->s.number);
			jumping = qtrue;
		}
		else if (self->bot_strafe_crouch_timer > level.time)
		{
			//trap->Print("DEBUG: self->bot_strafe_crouch_timer > level.time\n");
			cmd->upmove = -127.0;
			if (aiEnt->s.eType == ET_PLAYER) trap->EA_Crouch(aiEnt->s.number);
		}

		if (NPC_CheckFall(self, dir))
		{
			int JUMP_RESULT = NPC_CheckFallJump(self, dest, cmd);

			if (JUMP_RESULT == 2)
			{// We can jump there... JKA method...
				//trap->Print("DEBUG: JUMP_RESULT == 2\n");
				return qfalse;
			}
			else if (JUMP_RESULT == 1)
			{// We can jump there... My method...
				//trap->Print("DEBUG: JUMP_RESULT == 1\n");
				cmd->upmove = 127.0;
				if (aiEnt->s.eType == ET_PLAYER) trap->EA_Jump(aiEnt->s.number);
				jumping = qtrue;
			}
			else
			{// Moving here would cause us to fall (or 18 forward is blocked)... Wait!
				cmd->forwardmove = 0;
				cmd->rightmove = 0;
				cmd->upmove = 0;
				return qfalse;
			}
		}

		// If avoidance is needed, adjust our move dir...
		NPC_AdjustforStrafe(aiEnt, dir, walk, walkSpeed);
	}
#endif //__NPC_STRAFE__

	//NPCs cheat and store this directly because converting movement into a ucmd loses precision
	VectorCopy( dir, self->client->ps.moveDir );

	// get direction and non-optimal magnitude
	float speed = walk ? walkSpeed : 127.0f;
	float forwardmove = speed * DotProduct(forward, dir);
	float rightmove = speed * DotProduct(right, dir);

	// find optimal magnitude to make speed as high as possible
	if (Q_fabs(forwardmove) > Q_fabs(rightmove))
	{
		float highestforward = forwardmove < 0 ? -speed : speed;

		float highestright = highestforward * rightmove / forwardmove;

		cmd->forwardmove = ClampChar(highestforward);
		cmd->rightmove = ClampChar(highestright);
	}
	else
	{
		float highestright = rightmove < 0 ? -speed : speed;

		float highestforward = highestright * forwardmove / rightmove;

		cmd->forwardmove = ClampChar(highestforward);
		cmd->rightmove = ClampChar(highestright);
	}

	if (/*trap->PointContents( self->r.currentOrigin, self->s.number ) & CONTENTS_WATER*/self->waterlevel > 0)
	{// Always go to surface...
		cmd->upmove = 127.0;
	}

	if (!jumping 
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !UQ_MoveDirClear(aiEnt, cmd->forwardmove, cmd->rightmove, qfalse ))
	{// Dir not clear, or we would fall!
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		return qfalse;
	}

	if (aiEnt->s.eType == ET_PLAYER)
	{
		vec3_t viewAngles;
		vectoangles(dir, viewAngles);
		trap->EA_View(aiEnt->s.number, viewAngles);

		if (aiEnt->client->pers.cmd.buttons & BUTTON_WALKING)
		{
			trap->EA_Action(aiEnt->s.number, 0x0080000);
			trap->EA_Move(aiEnt->s.number, dir, 100);

			if (self->bot_strafe_jump_timer > level.time)
				trap->EA_Jump(aiEnt->s.number);
			else if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(aiEnt->s.number);
			else if (self->bot_strafe_right_timer > level.time)
				trap->EA_MoveRight(aiEnt->s.number);
		}
		else
		{
			trap->EA_Move(aiEnt->s.number, dir, 200);

			if (self->bot_strafe_jump_timer > level.time)
				trap->EA_Jump(aiEnt->s.number);
			else if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(aiEnt->s.number);
			else if (self->bot_strafe_right_timer > level.time)
				trap->EA_MoveRight(aiEnt->s.number);
		}
	}

	return qtrue;
}

qboolean UQ1_UcmdMoveForDir_NoAvoidance ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest )
{
	vec3_t		forward, right;
	float		walkSpeed = 63.0;// 60.5;// 55 * 1.1;//48;//64;//32;//self->NPC->stats.walkSpeed*1.1;
	gentity_t	*aiEnt = self;

	/*
	if (self->s.eType == ET_NPC)
	{
		walkSpeed = self->NPC->stats.walkSpeed*1.1;
	}
	*/

	int waterPlane = 0;

	if (self && self->client)
	{
		waterPlane = IsBelowWaterPlane(self->client->ps.origin, self->client->ps.viewheight);
	}

	if (self->waterlevel > 0 && self->enemy && self->enemy->client && NPC_IsAlive(self, self->enemy))
	{// When we have a valid enemy, always check water level so we don't drown while attacking them...

	}
	else if (waterPlane)
	{
		if (self->client->ps.eFlags & EF_JETPACK)
		{// Fly over :)
			pm->waterlevel = 2;
			pm->watertype = CONTENTS_WATER;
		}
		else
		{
			pm->waterlevel = waterPlane;
			pm->watertype = CONTENTS_WATER;
		}
	}
	else if (gWPNum > 0 && self->wpCurrent >= 0 && self->wpCurrent < gWPNum)
	{
		if (gWPArray[self->wpCurrent]->flags & WPFLAG_WATER)
		{// Use current waypoint info to save CPU time on traces...
			self->watertype = CONTENTS_WATER;
			
			if (self->client->ps.eFlags & EF_JETPACK)
				self->waterlevel = 2;
			else
				self->waterlevel = 1;
		}
	}

	if (self->client
		&& self->client->ps.weapon == WP_SABER
		&& self->enemy
		&& self->enemy->client
		&& self->enemy->client->ps.weapon == WP_SABER
		&& NPC_IsAlive(self, self->enemy)
		&& Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) < 96.0)
	{// Jedi always walk when in combat with saber...
		walk = qtrue;
	}
	else if (self->NPC && self->NPC->forceWalkTime > level.time)
	{// Retreating NPCs walk backwards, don't run... Because thats annoying as hell to saber users...
		walk = qtrue;
	}

	if (walk)
	{
		cmd->buttons |= BUTTON_WALKING;
	}

	AngleVectors(self->client->ps.viewangles/*self->r.currentAngles*/, forward, right, NULL );
	
	cmd->upmove = 0;

	dir[2] = 0;
	VectorNormalize(dir);
	forward[2] = 0;
	VectorNormalize(forward);
	right[2] = 0;
	VectorNormalize(right);

	if (self->beStillTime > level.time)
	{// Screwed...
		cmd->upmove = 0;
		cmd->rightmove = 0;
		cmd->forwardmove = 0;
		return qfalse;
	}

	//NPCs cheat and store this directly because converting movement into a ucmd loses precision
	VectorCopy( dir, self->client->ps.moveDir );

	/*if (!self->enemy || !NPC_ValidEnemy(aiEnt, self->enemy))
	{// Always face the move position...
		vectoangles(dir, faceAngles);
		AngleVectors( faceAngles, faceDir, NULL, NULL );

		//flaten up/down
		faceDir[2] = 0;
		VectorMA( self->r.currentOrigin, 18, faceDir, facePos);
		NPC_FacePosition(aiEnt, facePos, qfalse );
	}*/

	AngleVectors(self->client->ps.viewangles, forward, right, nullptr);
	forward[2] = 0;
	VectorNormalize(forward);
	right[2] = 0;
	VectorNormalize(right);

	// get direction and non-optimal magnitude
	float speed = walk ? walkSpeed : 127.0f;
	float forwardmove = speed * DotProduct(forward, dir);
	float rightmove = speed * DotProduct(right, dir);

	// find optimal magnitude to make speed as high as possible
	if (Q_fabs(forwardmove) > Q_fabs(rightmove))
	{
		float highestforward = forwardmove < 0 ? -speed : speed;

		float highestright = highestforward * rightmove / forwardmove;

		cmd->forwardmove = ClampChar(highestforward);
		cmd->rightmove = ClampChar(highestright);
	}
	else
	{
		float highestright = rightmove < 0 ? -speed : speed;

		float highestforward = highestright * forwardmove / rightmove;

		cmd->forwardmove = ClampChar(highestforward);
		cmd->rightmove = ClampChar(highestright);
	}

	if (/*trap->PointContents( self->r.currentOrigin, self->s.number ) & CONTENTS_WATER*/self->waterlevel > 0)
	{// Always go to surface...
		cmd->upmove = 127.0;
	}

	if (aiEnt->s.eType == ET_PLAYER)
	{
		vec3_t viewAngles;
		vectoangles(dir, viewAngles);
		trap->EA_View(aiEnt->s.number, viewAngles);

		if (aiEnt->client->pers.cmd.buttons & BUTTON_WALKING)
		{
			trap->EA_Action(aiEnt->s.number, 0x0080000);
			trap->EA_Move(aiEnt->s.number, dir, 100);

			if (self->bot_strafe_jump_timer > level.time)
				trap->EA_Jump(aiEnt->s.number);
			else if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(aiEnt->s.number);
			else if (self->bot_strafe_right_timer > level.time)
				trap->EA_MoveRight(aiEnt->s.number);
		}
		else
		{
			trap->EA_Move(aiEnt->s.number, dir, 200);

			if (self->bot_strafe_jump_timer > level.time)
				trap->EA_Jump(aiEnt->s.number);
			else if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(aiEnt->s.number);
			else if (self->bot_strafe_right_timer > level.time)
				trap->EA_MoveRight(aiEnt->s.number);
		}
	}

	return qtrue;
}

qboolean NPC_OrgVisible(vec3_t org1, vec3_t org2, int ignore)
{
	trace_t tr;
	vec3_t	from, to;

	VectorCopy(org1, from);
	from[2] += 32;
	VectorCopy(org2, to);
	to[2] += 18;

	trap->Trace(&tr, from, NULL, NULL, to, ignore, MASK_SOLID, 0, 0, 0);

	if (tr.fraction == 1)
	{
		return qtrue;
	}

	return qfalse;
}

extern void NPC_SelectBestWeapon(gentity_t *aiEnt);

void NPC_CheckTypeStuff ( gentity_t *aiEnt)
{
	if ( aiEnt->client->NPC_class == CLASS_VEHICLE )
	{

	}
	else
	{
		NPC_SelectBestWeapon(aiEnt);

		if (NPC_IsBountyHunter(aiEnt) || NPC_IsCommando(aiEnt) || NPC_IsAdvancedGunner(aiEnt))
		{
			if (!(aiEnt->client->ps.eFlags & EF_JETPACK))
			{// Commando's get a jetpack...
				//trap->Print("Commando %s given jetpack.\n", aiEnt->NPC_type);
				aiEnt->client->ps.eFlags |= EF_JETPACK;
				aiEnt->hasJetpack = qtrue;
			}
		}
	}
}

extern void Jedi_AdjustSaberAnimLevel(gentity_t *self, int newLevel);
extern void ClientThink_real(gentity_t *ent);

void NPC_GenericFrameCode ( gentity_t *self )
{
	gentity_t *aiEnt = self;

	if (NPC_IsJedi(aiEnt))
	{
		Jedi_AdjustSaberAnimLevel(aiEnt, 0);
	}

	// UQ1: Check any jetpack stuff...
	NPC_CheckFlying(aiEnt);

	if ( aiEnt->client->ps.weaponstate == WEAPON_READY )
	{
		aiEnt->client->ps.weaponstate = WEAPON_IDLE;
	}

#ifndef __NPCS_USE_BG_ANIMS__
	if (NPC_IsHumanoid(aiEnt))
	{
		if (!self->NPC->conversationPartner &&
			(self->client->ps.weapon > WP_SABER || self->client->ps.weapon == WP_NONE) &&
			!(aiEnt->s.torsoAnim == TORSO_WEAPONREADY1 || aiEnt->s.torsoAnim == TORSO_WEAPONREADY3))
		{//we look ready for action, using one of the first 2 weapon, let's rest our weapon on our shoulder
			//NPC_SetAnim(aiEnt, SETANIM_TORSO, TORSO_WEAPONIDLE3, SETANIM_FLAG_NORMAL);
			NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponReadyAnim[aiEnt->s.weapon], 0);
		}

		//if(aiEnt->attackDebounceTime > level.time)
		if (!TIMER_Done(aiEnt, "attackDelay"))
		{//We just shot but aren't still shooting, so hold the gun up for a while
			if (aiEnt->client->ps.weapon == WP_SABER)
			{//One-handed
				//NPC_SetAnim(aiEnt,SETANIM_TORSO,TORSO_WEAPONREADY1,SETANIM_FLAG_NORMAL);
			}
			else
			{//we just shot at someone. Hold weapon in attack anim for now
				NPC_SetAnim(aiEnt, SETANIM_TORSO, WeaponAttackAnim[aiEnt->s.weapon], 0);//Stoiss not sure about this one here.. testing
			}
		}
	}
#endif //__NPCS_USE_BG_ANIMS__

	NPC_CheckAttackHold(aiEnt);
	NPC_ApplyScriptFlags(aiEnt);

#if !defined(__USE_NAVLIB__) && !defined(__USE_NAVLIB_INTERNAL_MOVEMENT__)
	//cliff and wall avoidance
	NPC_AvoidWallsAndCliffs(aiEnt);
#endif //!defined(__USE_NAVLIB__) && !defined(__USE_NAVLIB_INTERNAL_MOVEMENT__)

	// run the bot through the server like it was a real client
	//=== Save the ucmd for the second no-think Pmove ============================
	aiEnt->client->pers.cmd.serverTime = level.time - 50;

	memcpy( &aiEnt->NPC->last_ucmd, &aiEnt->client->pers.cmd, sizeof( usercmd_t ) );

	if ( !aiEnt->NPC->attackHoldTime )
	{
		aiEnt->NPC->last_ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);//so we don't fire twice in one think
	}
	//============================================================================
	NPC_CheckAttackScript(aiEnt);
	NPC_KeepCurrentFacing(aiEnt);

	if ( NPC_IsCivilianHumanoid(aiEnt) && !aiEnt->npc_cower_runaway && !self->NPC->conversationPartner )
	{// Set better torso anims when not holding a weapon.
		NPC_SetAnim(aiEnt, SETANIM_TORSO, BOTH_STAND9IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		aiEnt->client->ps.torsoTimer = 200;
		//trap->Print(va("%s set torso anim.\n", aiEnt->client->modelname));
	}

	//if ( !aiEnt->next_roff_time || aiEnt->next_roff_time < level.time )
	//{//If we were following a roff, we don't do normal pmoves.
		ClientThink( aiEnt->s.number, &aiEnt->client->pers.cmd );
	//}
	//else
	//{
	//	NPC_ApplyRoff(aiEnt);
	//}

	// end of thinking cleanup
	aiEnt->NPC->touchedByPlayer = NULL;

	NPC_CheckPlayerAim(aiEnt);
	NPC_CheckAllClear(aiEnt);
}

/*
===============
NPC_Think

Main NPC AI - called once per frame
===============
*/
extern gentity_t *NPC_PickEnemyExt( gentity_t *aiEnt, qboolean checkAlerts );
extern qboolean NPC_FindEnemy( gentity_t *aiEnt, qboolean checkAlerts );
extern void NPC_ConversationAnimation(gentity_t *NPC);
extern void G_SpeechEvent( gentity_t *self, int event );

extern vmCvar_t npc_pathing;

#ifdef __AAS_AI_TESTING__
#include "botlib/aasfile.h"
extern void BotUpdateInput(bot_state_t *bs, int time, int elapsed_time);
extern bot_moveresult_t BotAttackMove(bot_state_t *bs, int tfl);
#endif //__AAS_AI_TESTING__

//#define __LOW_THINK_AI__

int next_active_npc_print = 0;

void NPC_PrintNumActiveNPCs(void)
{
	if (!dedicated.integer) return;
	if (next_active_npc_print > level.time) return;

	int num_active_npcs = 0;
	int num_inactive_npcs = 0;
	int num_total_npcs = 0;

	for (int i = level.maxclients; i < MAX_GENTITIES; i++)
	{
		gentity_t *NPC = &g_entities[i];

		if (!NPC) continue;
		if (!(NPC && NPC->inuse && NPC->client && NPC->r.linked && NPC->s.eType == ET_NPC)) continue;
		//if (!NPC_IsAlive(NPC, NPC)) continue;

		if (NPC->npc_activate_time > level.time)
		{
			num_active_npcs++;
		}
		else
		{
			num_inactive_npcs++;
		}

		num_total_npcs++;
		continue;
	}

	int percActive = (int)(((float)num_active_npcs / (float)num_total_npcs) * 100.0);
	int percInactive = (int)(((float)num_inactive_npcs / (float)num_total_npcs) * 100.0);

	if (num_total_npcs <= 0)
	{
		percActive = percInactive = 0;
	}

	trap->Print("^7%i ^5(^7%i^5 percent) active and ^7%i^5 (^7%i^5 percent) inactive NPCs.\n", num_active_npcs, percActive, num_inactive_npcs, percInactive);

	next_active_npc_print = level.time + 10000;
}

qboolean NPC_CheckNearbyPlayers(gentity_t *NPC)
{
	if (level.numConnectedClients <= 0)
	{
		return qfalse;
	}

	if (NPC->npc_activate_time > level.time)
	{
		return qtrue;
	}

	if (NPC->npc_activate_check_time > level.time)
	{
		return qfalse;
	}

	if (NPC->isPadawan)
	{// Padawans and followers with a player or an active NPC parent are always enabled...
		if (NPC->parent 
			&& (NPC->parent->s.eType == ET_PLAYER || (NPC->parent->s.eType == ET_NPC && NPC->parent->npc_activate_time >= level.time))
			&& NPC_IsAlive(NPC, NPC->parent))
		{
			NPC->npc_activate_time = level.time + 30000;
			return qtrue;
		}
	}

	for (int i = 0; i < sv_maxclients.integer; i++)
	{
		gclient_t *cl = level.clients + i;
		gentity_t *ent = &g_entities[i];

		if (cl->pers.connected != CON_CONNECTED) 
		{
			continue;
		}
		
		if (Distance(NPC->r.currentOrigin, cl->ps.origin) > 8192.0)
		{
			continue;
		}

		// Looks like he's close enough... Activate the NPC...
		NPC->npc_activate_time = level.time + 30000;
		return qtrue;
	}

	NPC->npc_activate_check_time = level.time + 5000;
	NPC->npc_activate_time = 0;
	return qfalse;
}

qboolean NPC_EscapeWater ( gentity_t *aiEnt )
{
	vec3_t dest;
	VectorCopy(aiEnt->spawn_pos, dest);

	NPC_FacePosition(aiEnt, dest, qfalse);
	VectorSubtract(dest, aiEnt->r.currentOrigin, aiEnt->movedir);

	if (UQ1_UcmdMoveForDir(aiEnt, &aiEnt->client->pers.cmd, aiEnt->movedir, qfalse, dest))
	{// See if we can simply move normally to the exit location...
		if (aiEnt->waterlevel && aiEnt->watertype == CONTENTS_WATER)
			aiEnt->water_escape_time = level.time + 500;

		return qtrue;
	}

	if (NPC_Jump(aiEnt, dest))
	{// See if we can jump to the exit location...
		VectorCopy(aiEnt->movedir, aiEnt->client->ps.moveDir);
		
		if (aiEnt->waterlevel && aiEnt->watertype == CONTENTS_WATER)
			aiEnt->water_escape_time = level.time + 500;

		return qtrue;
	}

	if (UQ1_UcmdMoveForDir_NoAvoidance(aiEnt, &aiEnt->client->pers.cmd, aiEnt->movedir, qfalse, dest))
	{// Force move toward the exit location...
		if (aiEnt->waterlevel && aiEnt->watertype == CONTENTS_WATER)
			aiEnt->water_escape_time = level.time + 500;

		return qtrue;
	}

	return qfalse;
	/*
	aiEnt->client->pers.cmd.forwardmove = 127;
	aiEnt->client->pers.cmd.rightmove = 0;
	aiEnt->client->pers.cmd.upmove = 127;

	if (aiEnt->waterlevel && aiEnt->watertype == CONTENTS_WATER)
		aiEnt->water_escape_time = level.time + 500;

	return qtrue;
	*/
}

#if	AI_TIMERS
extern int AITime;
#endif//	AI_TIMERS
void NPC_Think ( gentity_t *self )//, int msec )
{
	vec3_t	oldMoveDir;
	int i = 0;
	gentity_t *aiEnt = self;

	if (!self->pathlist)
	{
		self->pathlist = (int *)malloc(MAX_WPARRAY_SIZE*sizeof(int));
	}

	self->nextthink = level.time + FRAMETIME;
	//self->nextthink = level.time;

#ifdef __AAS_AI_TESTING__
	if (!botstates[self->s.number])
	{// Set up bot state for this npc if required...
		bot_settings_t	settings;
		memset(&settings, 0, sizeof(bot_settings_t));

		settings.skill = 5;
		if (self->client->sess.sessionTeam == FACTION_EMPIRE)
			strcpy(settings.team, "RED");
		else
			strcpy(settings.team, "BLUE");

		BotAISetupClient(self->s.number, &settings, qfalse);
	}
	botstates[self->s.number]->client = self->s.number;
#endif //__AAS_AI_TESTING__

	/*
	if (!(self->s.eFlags & EF_CLIENTSMOOTH)) {
		self->s.eFlags |= EF_CLIENTSMOOTH;
	}

	if (!(self->client->ps.eFlags & EF_CLIENTSMOOTH)) {
		self->client->ps.eFlags |= EF_CLIENTSMOOTH;
	}
	*/

	memset( &aiEnt->client->pers.cmd, 0, sizeof( aiEnt->client->pers.cmd ) );

	VectorCopy( self->client->ps.moveDir, oldMoveDir );

	// UQ1: Testing with this removed...
	if (self->s.NPC_class != CLASS_VEHICLE)
	{ //YOU ARE BREAKING MY PREDICTION. Bad clear.
		VectorClear( self->client->ps.moveDir );
	}

	if(!self || !self->NPC || !self->client)
	{
		return;
	}

	trap->Cvar_Update(&npc_pathing);

	// dead NPCs have a special think, don't run scripts (for now)
	//FIXME: this breaks deathscripts
	if ( self->health <= 0 )
	{
		self->s.eFlags |= EF_DEAD;
		self->client->ps.pm_type = PM_DEAD;

		// Check flying stuff when dead too...
		NPC_CheckFlying(aiEnt);

		self->health = self->s.health = self->client->ps.stats[STAT_HEALTH] = 0;

		NPC_DeadThink(self);

#ifndef __NO_ICARUS__
		if ( aiEnt->NPC->nextBStateThink <= level.time )
		{
			trap->ICARUS_MaintainTaskManager(self->s.number);
		}
#endif //__NO_ICARUS__

		if (self->client)
			VectorCopy(self->r.currentOrigin, self->client->ps.origin);

		return;
	}

	// see if NPC ai is frozen
	if ( d_npcfreeze.value || (aiEnt->r.svFlags&SVF_ICARUS_FREEZE) )
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue );
		ClientThink(self->s.number, &aiEnt->client->pers.cmd);
		//VectorCopy(self->s.origin, self->s.origin2 );
		VectorCopy(self->r.currentOrigin, self->client->ps.origin);
		return;
	}

	// UQ1: Generic stuff for different NPC types...
	NPC_CheckTypeStuff(aiEnt);

	if ( self->client->NPC_class == CLASS_VEHICLE)
	{
		if (self->client->ps.m_iVehicleNum)
		{//we don't think on our own
			//well, run scripts, though...
#ifndef __NO_ICARUS__
			trap->ICARUS_MaintainTaskManager(self->s.number);
#endif //__NO_ICARUS__
			return;
		}
		else
		{
			VectorClear(self->client->ps.moveDir);
			self->client->pers.cmd.forwardmove = 0;
			self->client->pers.cmd.rightmove = 0;
			self->client->pers.cmd.upmove = 0;
			self->client->pers.cmd.buttons = 0;
			memcpy(&self->m_pVehicle->m_ucmd, &self->client->pers.cmd, sizeof(usercmd_t));
		}
	}
	else if ( aiEnt->s.m_iVehicleNum )
	{//droid in a vehicle?
		G_DroidSounds( self );
	}

	if (aiEnt->s.eType != ET_NPC && aiEnt->s.eType != ET_PLAYER)
	{//Something drastic happened in our script
		return;
	}

	if (!NPC_CheckNearbyPlayers(self))
	{// Don't think...
		if (self->npc_deactivated_forced_clientthink_time > level.time)
		{
			VectorCopy(self->r.currentOrigin, self->client->ps.origin);
			return;
		}
		
		// Do a quick clientthink every 5 sec...
		self->npc_deactivated_forced_clientthink_time = level.time + 5000;
		NPC_UpdateAngles(aiEnt, qtrue, qtrue);
		ClientThink(self->s.number, &self->client->pers.cmd);
		VectorCopy(self->r.currentOrigin, self->client->ps.origin);
		return;
	}

	if ( aiEnt->NPC->nextBStateThink <= level.time
		&& !aiEnt->s.m_iVehicleNum )//NPCs sitting in Vehicles do NOTHING
	{
#if	AI_TIMERS
		int	startTime = GetTime(0);
#endif//	AI_TIMERS

		// UQ1: Think more often!
#ifndef __LOW_THINK_AI__
		aiEnt->NPC->nextBStateThink = level.time + FRAMETIME/2;
#else //__LOW_THINK_AI__
		aiEnt->NPC->nextBStateThink = level.time + FRAMETIME;
#endif //__LOW_THINK_AI__

		memcpy( &aiEnt->client->pers.cmd, &aiEnt->NPC->last_ucmd, sizeof( usercmd_t ) );
		aiEnt->client->pers.cmd.buttons = 0; // init buttons...

		//
		// Escaping water...
		//
#if 0
		if (((self->waterlevel && self->watertype == CONTENTS_WATER) || self->water_escape_time > level.time) && NPC_EscapeWater(self))
		{
			// UQ1: Check any jetpack stuff...
			NPC_CheckFlying(aiEnt);

			if (aiEnt->client->ps.weaponstate == WEAPON_READY)
			{
				aiEnt->client->ps.weaponstate = WEAPON_IDLE;
			}

			NPC_CheckAttackHold(aiEnt);

			// run the bot through the server like it was a real client
			//=== Save the ucmd for the second no-think Pmove ============================
			aiEnt->client->pers.cmd.serverTime = level.time - 50;

			memcpy(&aiEnt->NPC->last_ucmd, &aiEnt->client->pers.cmd, sizeof(usercmd_t));

			if (!aiEnt->NPC->attackHoldTime)
			{
				aiEnt->NPC->last_ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);//so we don't fire twice in one think
			}

			NPC_KeepCurrentFacing(aiEnt);

			if (NPC_IsCivilianHumanoid(aiEnt) && !aiEnt->npc_cower_runaway && !self->NPC->conversationPartner)
			{// Set better torso anims when not holding a weapon.
				NPC_SetAnim(aiEnt, SETANIM_TORSO, BOTH_STAND9IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				aiEnt->client->ps.torsoTimer = 200;
			}

			ClientThink(aiEnt->s.number, &aiEnt->client->pers.cmd);

			// end of thinking cleanup
			aiEnt->NPC->touchedByPlayer = NULL;

			NPC_CheckPlayerAim(aiEnt);
			NPC_CheckAllClear(aiEnt);
			return;
		}
#endif

		//nextthink is set before this so something in here can override it
		if (self->s.NPC_class != CLASS_VEHICLE && !self->m_pVehicle)
		{ //ok, let's not do this at all for vehicles.
			qboolean is_civilian = NPC_IsCivilian(self);
			qboolean is_vendor = NPC_IsVendor(self);
			qboolean is_jedi = NPC_IsJedi(self);
			qboolean is_bot = (qboolean)(self->s.eType == ET_PLAYER);
			qboolean use_pathing = qfalse;

			//if (!is_jedi && self->isPadawan) is_jedi = qtrue;
			if (self->isPadawan) is_jedi = qfalse;

			if (is_civilian || is_vendor)
			{
				use_pathing = qfalse;
			}
			else if (npc_pathing.integer > 0)
			{
				if (is_bot) use_pathing = qtrue;
				if (g_gametype.integer >= GT_TEAM) use_pathing = qtrue;
			}
			else
			{
				if (is_bot) use_pathing = qtrue;
			}

			NPC_DoPadawanStuff(aiEnt); // check any padawan stuff we might need to do...

			/*if (self->enemy 
				&& (!NPC_ValidEnemy(self->enemy) || (!NPC_EntityIsBreakable(self, self->enemy) && Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) > 3192.0)))
			{// If NPC Bot's enemy is invalid (eg: a dead NPC) or too far away, clear it!
				G_ClearEnemy(self);
			}*/

			if (is_civilian) 
			{
				G_ClearEnemy(self);
			}

			if ((!self->enemy || !NPC_IsAlive(self, self->enemy)) && !is_civilian && self->next_enemy_check_time < level.time)
			{
				NPC_FindEnemy(aiEnt, qtrue );

				if (self->enemy && NPC_IsAlive(self, self->enemy))
				{// UQ1: Ummmm sounds please!
					if (NPC_IsJedi(self))
					{
						G_AddVoiceEvent( self, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 15000 + irand(0, 30000) );
					}
					else if (NPC_IsBountyHunter(self))
					{
						G_AddVoiceEvent( self, Q_irand( EV_DETECTED1, EV_DETECTED5 ), 15000 + irand(0, 30000) );
					}
					else
					{
						if (irand(0,1) == 1)
							ST_Speech( self, SPEECH_DETECTED, 0 );
						else if (irand(0,1) == 1)
							ST_Speech( self, SPEECH_CHASE, 0 );
						else
							ST_Speech( self, SPEECH_SIGHT, 0 );
					}

					if (!self->enemy->enemy || (self->enemy->enemy && !NPC_IsAlive(self, self->enemy->enemy)))
					{// Make me their enemy if they have none too...
						self->enemy->enemy = self;
					}
				}

				self->next_enemy_check_time = level.time + irand(2000, 5000);
			}

			/*if ( self->enemy ) 
			{
				NPC_FaceEnemy( qtrue );
			}*/

			if (!self->isPadawan && !self->enemy && !self->NPC->conversationPartner)
			{// Let's look for someone to chat to, shall we???
				NPC_FindConversationPartner(aiEnt);
			}

			if (!self->isPadawan && self->NPC->conversationPartner)
			{
				if (!(aiEnt->client->pers.cmd.buttons & BUTTON_WALKING))
					aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			}

			if (self->padawan && self->padawan_reply_waiting && self->padawan_reply_time - 8000 < level.time && NPC_IsAlive(self, self->padawan))
			{// Look at the padawan when it is talking to us...
				NPC_FacePosition(aiEnt, self->padawan->r.currentOrigin, qfalse );
				self->beStillTime = level.time + 5000;
			}

			if (self->padawan && self->padawan_reply_waiting && self->padawan_reply_time < level.time && NPC_IsAlive(self, self->padawan))
			{// Do any replies to padawan comments...
				self->padawan_reply_waiting = qfalse;
				G_SpeechEvent( self, EV_PADAWAN_IDLE_REPLY );

				//trap->Print("Master %s replying to padawan comment.\n", self->client->pers.netname);

				if (!self->enemy || !NPC_ValidEnemy(aiEnt, self->enemy))
				{// Also look and animate if we have no enemy...
					vec3_t origin, angles;

					// Look at our partner...
					VectorCopy(self->padawan->r.currentOrigin, origin);
					VectorSubtract( origin, self->r.currentOrigin, self->move_vector );
					vectoangles( self->move_vector, angles );
					G_SetAngles(self, angles);
					VectorCopy(angles, self->client->ps.viewangles);
					NPC_FacePosition(aiEnt, origin, qfalse );
					NPC_ConversationAnimation(self);
					self->beStillTime = level.time + 5000;
				}
			}

			if (self->beStillTime > level.time && (!self->enemy || !NPC_ValidEnemy(aiEnt, self->enemy)))
			{// Just idle...
				if (self->padawan && NPC_IsAlive(self, self->padawan))
				{// Look at our padawan...
					vec3_t origin, angles;
					VectorCopy(self->padawan->r.currentOrigin, origin);
					VectorSubtract( origin, self->r.currentOrigin, self->move_vector );
					vectoangles( self->move_vector, angles );
					G_SetAngles(self, angles);
					VectorCopy(angles, self->client->ps.viewangles);
					NPC_FacePosition(aiEnt, origin, qfalse );
				}

				aiEnt->client->pers.cmd.forwardmove = 0;
				aiEnt->client->pers.cmd.rightmove = 0;
				aiEnt->client->pers.cmd.upmove = 0;
				NPC_GenericFrameCode( self );
				return;
			}

			if (!self->enemy)
			{
				//
				// Civilian NPC cowerring...
				//
				if (self->npc_cower_time < level.time)
				{// Just finished cowerring... Stand up again...
					if (aiEnt->NPC->scriptFlags & SCF_CROUCHED)
					{// Makse sure they are no longer crouching/cowering in place...
						aiEnt->NPC->scriptFlags &= ~SCF_CROUCHED;
						aiEnt->client->pers.cmd.upmove = 127;
					}

					// Init the run-away flag...
					self->npc_cower_runaway = qfalse;
				}

				if (self->npc_cower_time > level.time && !self->npc_cower_runaway)
				{// A civilian NPC that is cowering in place...
					if ( TIMER_Done( aiEnt, "flee" ) && TIMER_Done( aiEnt, "panic" ) )
					{// We finished running away, now cower in place...
						aiEnt->client->pers.cmd.forwardmove = 0;
						aiEnt->client->pers.cmd.rightmove = 0;
						aiEnt->client->pers.cmd.upmove = 0;
						aiEnt->client->pers.cmd.buttons = 0;

						if (!(aiEnt->NPC->scriptFlags & SCF_CROUCHED))
						{// Makse sure they are crouching/cowering in place...
							aiEnt->NPC->scriptFlags |= SCF_CROUCHED;

							if (self->s.eType == ET_PLAYER)
								trap->EA_Crouch(self->s.number);
							else
								aiEnt->client->pers.cmd.upmove = -127;

							NPC_FacePosition(aiEnt, self->r.currentOrigin, qtrue ); // Should make them look at the ground I hope...
						}
					}

					NPC_GenericFrameCode( self );
				}
				//
				// Hacking/Using something...
				//
				else if (self->client->isHacking)
				{// Hacking/using something... Look at it and wait...
					gentity_t *HACK_TARGET = &g_entities[self->client->isHacking];

					if (HACK_TARGET) NPC_FaceEntity(aiEnt, HACK_TARGET, qtrue);

					if (!(aiEnt->client->pers.cmd.buttons & BUTTON_USE))
						aiEnt->client->pers.cmd.buttons |= BUTTON_USE;

					NPC_GenericFrameCode( self );
				}
				//
				// Conversations...
				//
				else if (!self->isPadawan && self->NPC->conversationPartner)
				{// Chatting with another NPC... Stay still!
					NPC_EnforceConversationRange(self);
					NPC_FacePosition(aiEnt, self->NPC->conversationPartner->r.currentOrigin, qfalse );
					NPC_NPCConversation(aiEnt);
					NPC_GenericFrameCode( self );
				}
				//
				// Pathfinding...
				//
				else if (NPC_PadawanMove(aiEnt))
				{
					NPC_GenericFrameCode( self );
				}
#ifdef __USE_NAV_PATHING__
				else if (!self->isPadawan && use_pathing && !self->enemy)
				{
					
					//if (!self->waypoint)
					//	self->waypoint = trap->Nav_GetNearestNode((sharedEntity_t *)self, self->waypoint, 0x00000002/*NF_CLEAR_PATH*/, -1/*WAYPOINT_NONE*/);

					//if (!self->longTermGoal)
						//self->longTermGoal = trap->Nav_GetNearestNode((sharedEntity_t *)self, self->waypoint, 0x00000002/*NF_CLEAR_PATH*/, -1/*WAYPOINT_NONE*/);
					//	self->longTermGoal = irand(0, trap->Nav_GetNumNodes()-1);
					
					//self->wpCurrent = trap->Nav_GetBestNode(self->waypoint, self->longTermGoal);
					//if (!self->wpCurrent) self->wpCurrent = trap->Nav_GetBestNodeAltRoute( self->waypoint, self->longTermGoal, &self->pathsize, -1 );

					if (!aiEnt->NPC->goalEntity && self->npc_dumb_route_time < level.time) 
					{
						int i = 0;
						qboolean found = qfalse;

						for (i = 0; i < ENTITYNUM_MAX_NORMAL; i++)
						{
							gentity_t *ent = &g_entities[i];

							if (!ent) continue;
							if (!ent->inuse) continue;
							if (ent->s.eType != ET_PLAYER && ent->s.eType != ET_NPC && ent->s.eType != ET_ITEM) continue;
							if (OnSameTeam(self, ent)) continue;
							
							//if (irand(0, 3) < 1)
							{
								found = qtrue;
								break;
							}
						}

						if (found)
						{
							aiEnt->NPC->goalEntity = &g_entities[i];
							//trap->Print("NPC %s found a goal.\n", self->NPC_type);
						}

						self->npc_dumb_route_time = level.time + 5000;
					}

					if (aiEnt->NPC->goalEntity)
					{
						//trap->Print("NPC %s following path.\n", self->NPC_type);
						NPC_SlideMoveToGoal(aiEnt);
					}
				}
#else //!__USE_NAV_PATHING__

#ifdef __AAS_AI_TESTING__
				else if (!self->isPadawan && use_pathing && !self->enemy)
				{
					if ((!aiEnt->NPC->goalEntity || Distance(aiEnt->NPC->goalEntity->r.currentOrigin, self->r.currentOrigin) < 64) 
						&& self->npc_dumb_route_time < level.time) 
					{
						int i = 0;
						qboolean found = qfalse;

						for (i = 0; i < ENTITYNUM_MAX_NORMAL; i++)
						{
							gentity_t *ent = &g_entities[i];

							if (!ent) continue;
							if (!ent->inuse) continue;
							if (ent->s.eType != ET_PLAYER && ent->s.eType != ET_NPC && ent->s.eType != ET_ITEM) continue;
							if (ent->s.eType == ET_PLAYER && ent->client->sess.sessionTeam == FACTION_SPECTATOR) continue;
							if (!NPC_ValidEnemy(self, ent)) continue;
							
							//if (irand(0, 3) < 1)
							{
								found = qtrue;
								break;
							}
						}

						if (found)
						{
							aiEnt->NPC->goalEntity = &g_entities[i];
							//trap->Print("NPC %s found a goal.\n", self->NPC_type);
						}

						self->npc_dumb_route_time = level.time + 5000;
					}

					if (aiEnt->NPC->goalEntity)
					{
						bot_moveresult_t moveresult;
						bot_state_t *bs = botstates[self->s.number];
						bot_input_t *bi = &bs->bi;

						memcpy(&bs->cur_ps, &aiEnt->client->ps, sizeof(playerState_t));
						
						if ((bs->goal.origin[0] == 0 && bs->goal.origin[1] == 0 && bs->goal.origin[2] == 0)
							|| Distance(self->r.currentOrigin, bs->goal.origin) < 64)
						{// New goal
							VectorCopy(aiEnt->NPC->goalEntity->r.currentOrigin, bs->goal.origin);
							//bs->goal.origin[2] += 64;
							VectorCopy(aiEnt->NPC->goalEntity->r.mins, bs->goal.mins);
							VectorCopy(aiEnt->NPC->goalEntity->r.maxs, bs->goal.maxs);
							//VectorSet(bs->goal.mins, -8, -8, -8);
							//VectorSet(bs->goal.maxs, 8, 8, 8);
							bs->goal.areanum = trap->AAS_PointAreaNum(aiEnt->NPC->goalEntity->r.currentOrigin);
							bs->goal.entitynum = aiEnt->NPC->goalEntity->s.number;
							bs->goal.flags = 0;
							bs->goal.iteminfo = 0;
							bs->goal.number = 0;
							self->longTermGoal = bs->goal.areanum;
						}

						trap->BotUpdateEntityItems();
						//trap->BotCalculatePaths(0);
						
						//trap->EA_ResetInput(self->s.number);
						//trap->EA_GetInput(bs->client, (float)level.time / 1000, bi);
						BotUpdateInput(bs, level.time, 50);
						//trap->Print("NPC %s following path.\n", self->NPC_type);
						
						self->wpCurrent = trap->AAS_PointAreaNum(self->s.origin);

						if (self->wpCurrent >= 0)
						{
							trap->AAS_EnableRoutingArea(self->wpCurrent, qtrue);
							trap->AAS_EnableRoutingArea(self->longTermGoal, qtrue);
							
							//self->longTermGoal = trap->AAS_PointAreaNum(aiEnt->NPC->goalEntity->r.currentOrigin);
							self->pathsize = trap->AAS_PredictRoute(self->pathlist, self->wpCurrent, self->r.currentOrigin, bs->goal.areanum, 0, -1, -1, -1, -1, -1, -1);

							//trap->BotMoveToGoal(&moveresult, bs->ms, &bs->goal, 0);
							moveresult = BotAttackMove(bs, 0);

							//VectorCopy(moveresult.movedir, self->movedir);

							if (!moveresult.blocked && !moveresult.blockentity && !moveresult.failure)
							{
								vec3_t angles, forward, right, up, movePos;

								VectorCopy(self->client->ps.viewangles, bi->viewangles);

								//vectoangles(moveresult.movedir, angles);
								//AngleVectors( moveresult.ideal_viewangles, forward, right, up );

								//VectorMA(self->r.currentOrigin, 256, forward, movePos);

								//NPC_FacePosition(self, moveresult.ideal_viewangles, qfalse);

								//VectorNormalize(moveresult.movedir);

								//self->movedir[0] = moveresult.movedir[1];
								//self->movedir[1] = moveresult.movedir[0];
								//self->movedir[2] = moveresult.movedir[2];

								//trap->BotUserCommand(self->s.number, &aiEnt->client->pers.cmd);

								//UQ1_UcmdMoveForDir( self, &aiEnt->client->pers.cmd, self->movedir, qfalse, movePos );
								//trap->BotMoveInDirection(bs->ms, bi.dir, bi.speed, 0);

								//AngleVectors(bi->viewangles, forward, right, up);
								VectorMA(self->r.currentOrigin, 256, moveresult.movedir/*forward*/, movePos);
								NPC_FacePosition(self, movePos, qfalse);

								trap->EA_GetInput(self->s.number, 50, bi);


								//moveresult.movedir[0] = -moveresult.movedir[0];
								//moveresult.movedir[1] = -moveresult.movedir[1];
								//VectorNormalize(forward);
								VectorCopy(bi->dir, moveresult.movedir);
								VectorCopy(moveresult.movedir, bs->moveDir);
								VectorCopy(moveresult.movedir, self->movedir);
								UQ1_UcmdMoveForDir_NoAvoidance(self, &aiEnt->client->pers.cmd, self->movedir/*forward bi->dir*/, qfalse, movePos);
								//trap->BotUserCommand(self->s.number, &aiEnt->client->pers.cmd);

								trap->Print("move to entity %i. pos %f %f %f. dir %f %f %f. movedir %f %f %f. ucmd %i %i %i.\n"
									, aiEnt->NPC->goalEntity->s.number
									, movePos[0], movePos[1], movePos[2]
									, bi->dir[0], bi->dir[1], bi->dir[2]
									, self->movedir[0], self->movedir[1], self->movedir[2]
									, (int)aiEnt->client->pers.cmd.forwardmove, (int)aiEnt->client->pers.cmd.rightmove, (int)aiEnt->client->pers.cmd.upmove);
							}
						}
					}

					NPC_GenericFrameCode( self );
				}
#else
				else if (!self->isPadawan && use_pathing && NPC_FollowRoutes(aiEnt))
				{
					//trap->Print("NPCBOT DEBUG: NPC is following routes.\n");
					NPC_GenericFrameCode( self );
				}
#endif //__AAS_AI_TESTING__

#endif //__USE_NAV_PATHING__
				//
				// Patroling...
				//
				else if (!self->isPadawan && !use_pathing && NPC_PatrolArea(aiEnt))
				{
					//trap->Print("NPCBOT DEBUG: NPC is patroling.\n");
					NPC_GenericFrameCode( self );
				}
				else
				{
					//trap->Print("NPCBOT DEBUG: NPC is stuck.\n");

					aiEnt->client->pers.cmd.forwardmove = 0;
					aiEnt->client->pers.cmd.rightmove = 0;
					aiEnt->client->pers.cmd.upmove = 0;
					aiEnt->client->pers.cmd.buttons = 0;

					if (NPC_IsHumanoid(aiEnt))
					{
						if (!aiEnt->NPC->conversationPartner)
						{// Not chatting with another NPC... Set idle animation...
							NPC_PickRandomIdleAnimantion(aiEnt);
						}
					}

					NPC_GenericFrameCode( self );
				}
			}
			else
			{
				//trap->Print("NPCBOT DEBUG: NPC is attacking.\n");

				// If we have combat pathing, use it while we do the other stuff...
				if (NPC_IsCombatPathing(aiEnt))
				{
					NPC_FollowRoutes(aiEnt);
				}

				NPC_ExecuteBState( self );

#if 0 // UQ1: Should be handled in behavior now...
				if (self->enemy 
					&& NPC_IsAlive(self, self->enemy)
					&& NPC_IsJedi(self) 
					&& Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) > 64)
				{
					// UQ1: Always force move to any goal they might have...
					aiEnt->NPC->goalEntity = self->enemy;
					
					if (UpdateGoal())
						NPC_MoveToGoal( qtrue );
				}
				else if (self->enemy 
					&& NPC_IsAlive(self, self->enemy)
					&& Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) > 64
					&& NPC_CheckVisibility ( aiEnt->enemy, CHECK_360|CHECK_VISRANGE ) < VIS_FOV)
				{
					// UQ1: Enemy is not in our view, move toward it...
					aiEnt->NPC->goalEntity = self->enemy;
					
					if (UpdateGoal())
						if (!NPC_MoveToGoal( qfalse ))
							NPC_MoveToGoal( qtrue );
				}
#endif //0

				NPC_GenericFrameCode( self );
			}
		}
		else
		{
			//trap->Print("NPCBOT DEBUG: NPC is movetogoal.\n");
			// UQ1: Always force move to any goal they might have...
			//NPC_MoveToGoal( aiEnt, qtrue );
			//if (UpdateGoal(aiEnt))
			//	if (!NPC_MoveToGoal( qfalse ))
			//		NPC_MoveToGoal( qtrue );
		}

#if	AI_TIMERS
		int addTime = GetTime( startTime );
		if ( addTime > 50 )
		{
			Com_Printf( S_COLOR_RED"ERROR: NPC number %d, %s %s at %s, weaponnum: %d, using %d of AI time!!!\n", NPC->s.number, NPC->NPC_type, NPC->targetname, vtos(NPC->r.currentOrigin), NPC->s.weapon, addTime );
		}
		AITime += addTime;
#endif//	AI_TIMERS

		//ClientThink(aiEnt->s.number, &aiEnt->client->pers.cmd);

	}
	else
	{
#ifndef __LOW_THINK_AI__
		VectorCopy( oldMoveDir, self->client->ps.moveDir );

		//or use client->pers.lastCommand?
		aiEnt->NPC->last_ucmd.serverTime = level.time - 50;
		//if ( !aiEnt->next_roff_time || aiEnt->next_roff_time < level.time )
		{//If we were following a roff, we don't do normal pmoves.
			//FIXME: firing angles (no aim offset) or regular angles?
			//if (self->enemy) NPC_UpdateAngles(qtrue, qtrue);
			memcpy( &aiEnt->client->pers.cmd, &aiEnt->NPC->last_ucmd, sizeof( usercmd_t ) );
			NPC_GenericFrameCode(self);
			//ClientThink(aiEnt->s.number, &aiEnt->client->pers.cmd);
		}
		//else
		//{
		//	NPC_ApplyRoff(aiEnt);
		//}
#else //__LOW_THINK_AI__
		//VectorCopy( oldMoveDir, self->client->ps.moveDir );

		NPC_GenericFrameCode( self );
		//NPC_ApplyRoff();
#endif //__LOW_THINK_AI__
	}

#ifndef __NO_ICARUS__
	//must update icarus *every* frame because of certain animation completions in the pmove stuff that can leave a 50ms gap between ICARUS animation commands
	trap->ICARUS_MaintainTaskManager(self->s.number);
#endif //__NO_ICARUS__

	VectorCopy(self->r.currentOrigin, self->client->ps.origin);
}

void NPC_InitAI ( void )
{
	/*
	trap->Cvar_Register(&g_saberRealisticCombat, "g_saberRealisticCombat", "0", CVAR_CHEAT);

	trap->Cvar_Register(&debugNoRoam, "d_noroam", "0", CVAR_CHEAT);
	trap->Cvar_Register(&debugNPCAimingBeam, "d_npcaiming", "0", CVAR_CHEAT);
	trap->Cvar_Register(&debugBreak, "d_break", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_npcai, "d_npcai", "0", CVAR_CHEAT);
	trap->Cvar_Register(&debugNPCFreeze, "d_npcfreeze", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_JediAI, "d_JediAI", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_noGroupAI, "d_noGroupAI", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_asynchronousGroupAI, "d_asynchronousGroupAI", "0", CVAR_CHEAT);

	//0 = never (BORING)
	//1 = kyle only
	//2 = kyle and last enemy jedi
	//3 = kyle and any enemy jedi
	//4 = kyle and last enemy in a group
	//5 = kyle and any enemy
	//6 = also when kyle takes pain or enemy jedi dodges player saber swing or does an acrobatic evasion

	trap->Cvar_Register(&d_slowmodeath, "d_slowmodeath", "0", CVAR_CHEAT);

	trap->Cvar_Register(&d_saberCombat, "d_saberCombat", "0", CVAR_CHEAT);

	trap->Cvar_Register(&g_npcspskill, "g_npcspskill", "0", CVAR_ARCHIVE | CVAR_USERINFO);
	*/
}

/*
==================================
void NPC_InitAnimTable( void )

  Need to initialize this table.
  If someone tried to play an anim
  before table is filled in with
  values, causes tasks that wait for
  anim completion to never finish.
  (frameLerp of 0 * numFrames of 0 = 0)
==================================
*/
/*
void NPC_InitAnimTable( void )
{
	int i;

	for ( i = 0; i < MAX_ANIM_FILES; i++ )
	{
		for ( int j = 0; j < MAX_ANIMATIONS; j++ )
		{
			level.knownAnimFileSets[i].animations[j].firstFrame = 0;
			level.knownAnimFileSets[i].animations[j].frameLerp = 100;
			level.knownAnimFileSets[i].animations[j].initialLerp = 100;
			level.knownAnimFileSets[i].animations[j].numFrames = 0;
		}
	}
}
*/

#ifdef __DOMINANCE_AI__
extern void NPC_ShadowTrooper_Precache( void );
extern void NPC_Gonk_Precache( void );
extern void NPC_Mouse_Precache( void );
extern void NPC_Seeker_Precache( void );
extern void NPC_Remote_Precache( void );
extern void	NPC_R2D2_Precache(void);
extern void	NPC_R5D2_Precache(void);
extern void NPC_Probe_Precache(void);
extern void NPC_Interrogator_Precache(gentity_t *self);
extern void NPC_MineMonster_Precache( void );
extern void NPC_Howler_Precache( void );
extern void NPC_ATST_Precache(void);
extern void NPC_Sentry_Precache(void);
extern void NPC_Mark1_Precache(void);
extern void NPC_Mark2_Precache(void);
extern void NPC_GalakMech_Precache( void );
extern void NPC_GalakMech_Init( gentity_t *ent );
extern void NPC_Protocol_Precache( void );
extern void Boba_Precache( void );
extern void NPC_Wampa_Precache( void );
#endif //__DOMINANCE_AI__

void NPC_InitGame( void )
{
//	globals.NPCs = (gNPC_t *) trap->TagMalloc(game.maxclients * sizeof(game.bots[0]), TAG_GAME);
//	trap->Cvar_Register(&debugNPCName, "d_npc", "0", CVAR_CHEAT);

	NPC_LoadParms();
	NPC_InitAI();
//	NPC_InitAnimTable();
	/*
	ResetTeamCounters();
	for ( int team = NPCTEAM_FREE; team < NPCFACTION_NUM_FACTIONS; team++ )
	{
		teamLastEnemyTime[team] = -10000;
	}
	*/

	
#ifdef __DOMINANCE_AI__
	NPC_ShadowTrooper_Precache();
	NPC_Gonk_Precache();
	NPC_Mouse_Precache();
	NPC_Seeker_Precache();
	NPC_Remote_Precache();
	NPC_R2D2_Precache();
	NPC_R5D2_Precache();
	NPC_Probe_Precache();
	//NPC_Interrogator_Precache(gentity_t *self);
	NPC_MineMonster_Precache();
	NPC_Howler_Precache();
	NPC_ATST_Precache();
	NPC_Sentry_Precache();
	NPC_Mark1_Precache();
	NPC_Mark2_Precache();
	NPC_GalakMech_Precache();
	NPC_Protocol_Precache();
	Boba_Precache();
	NPC_Wampa_Precache();
#endif //__DOMINANCE_AI__
	
}

extern void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern void BG_SetAnimFinal(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags);

void NPC_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask )
{
	trap->Trace(results, start, mins, maxs, end, passEntityNum, contentmask, 0, 0, 0);
}

//#define __NEW__

extern stringID_table_t animTable[MAX_ANIMATIONS + 1];
void NPC_SetAnim(gentity_t *ent, int setAnimParts, int anim, int setAnimFlags)
{	// FIXME : once torsoAnim and legsAnim are in the same structure for NCP and Players
	// rename PM_SETAnimFinal to PM_SetAnim and have both NCP and Players call PM_SetAnim
#ifndef __NEW__
	/*if (ent->s.NPC_class == CLASS_WAMPA)
	{
		trap->Print("WAMPA NPC %i set anim %s.\n", ent->s.number, animTable[anim].name);
	}*/

	G_SetAnim(ent, NULL, setAnimParts, anim, setAnimFlags, 0);
#endif //__NEW__

#ifdef __NEW__
	if(ent && ent->inuse && ent->client)
	{//Players, NPCs
		//if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
		{
			pmove_t pmv;

			memset (&pmv, 0, sizeof(pmv));
			pmv.ps = &ent->client->ps;
			pmv.animations = bgAllAnims[ent->localAnimIndex].anims;
			pmv.cmd = ent->client->pers.cmd;
			pmv.trace = NPC_Trace;
			pmv.pointcontents = trap->PointContents;
			pmv.gametype = level.gametype;

			//don't need to bother with ghoul2 stuff, it's not even used in PM_SetAnim.
			pm = &pmv;

			if (setAnimParts & SETANIM_TORSO)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->client->ps.torsoAnim != anim )
				{
					//PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoTimer, 0 );
					PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoTimer, 0 );
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->client->ps.legsAnim != anim )
				{
					//PM_SetLegsAnimTimer( ent, &ent->client->ps.legsAnimTimer, 0 );
					PM_SetLegsAnimTimer( ent, &ent->client->ps.legsTimer, 0 );
				}
			}
		}

		//PM_SetAnimFinal(&ent->client->ps.torsoAnim,&ent->client->ps.legsAnim,setAnimParts,anim,setAnimFlags,
		//	&ent->client->ps.torsoAnimTimer,&ent->client->ps.legsAnimTimer,ent);

		BG_SetAnimFinal(&ent->client->ps, bgAllAnims[ent->localAnimIndex].anims, setAnimParts, anim, setAnimFlags);
		//ent->client->ps.torsoTimer
	}
	/*else
	{//bodies, etc.
		if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
		{
			if (setAnimParts & SETANIM_TORSO)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->s.torsoAnim != anim )
				{
					PM_SetTorsoAnimTimer( ent, &ent->s.torsoAnimTimer, 0 );
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->s.legsAnim != anim )
				{
					PM_SetLegsAnimTimer( ent, &ent->s.legsAnimTimer, 0 );
				}
			}
		}

		PM_SetAnimFinal(&ent->s.torsoAnim,&ent->s.legsAnim,setAnimParts,anim,setAnimFlags,
			&ent->s.torsoAnimTimer,&ent->s.legsAnimTimer,ent);
	}*/
	else
	{
		G_SetAnim(ent, NULL, setAnimParts, anim, setAnimFlags, 0);
	}
#endif //__NEW__
}
