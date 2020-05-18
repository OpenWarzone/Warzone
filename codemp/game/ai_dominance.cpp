#include "g_local.h"
#include "g_public.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"		//bot lib interface
#include "botlib/be_aas.h"
#include "botlib/be_ea.h"
#include "botlib/be_ai_char.h"
#include "botlib/be_ai_chat.h"
#include "botlib/be_ai_gen.h"
#include "botlib/be_ai_goal.h"
#include "botlib/be_ai_move.h"
#include "botlib/be_ai_weap.h"
//
#include "ai_dominance_main.h"
#include "w_saber.h"
//
#include "chars.h"
#include "inv.h"

#include "b_local.h"


//Local Variables
extern npcStatic_t NPCS;


/*
#define BOT_CTF_DEBUG	1
*/

#ifndef __DOMINANCE_AI__
#define BOT_THINK_TIME	0
#else //__DOMINANCE_AI__
#define BOT_THINK_TIME	100

extern void NPC_Think ( gentity_t *self);
#endif //__DOMINANCE_AI__

//bot states
bot_state_t	*botstates[MAX_GENTITIES];
//number of bots
int numbots;
//floating point time
float floattime;
//time to do a regular update
float regularupdate_time;
//

//for siege:
extern int rebel_attackers;
extern int imperial_attackers;

boteventtracker_t gBotEventTracker[MAX_GENTITIES];

//rww - new bot cvars..
vmCvar_t bot_forcepowers;
vmCvar_t bot_forgimmick;
vmCvar_t bot_honorableduelacceptance;
vmCvar_t bot_pvstype;
vmCvar_t bot_normgpath;
#ifndef FINAL_BUILD
vmCvar_t bot_getinthecarrr;
#endif

#ifdef _DEBUG
vmCvar_t bot_nogoals;
vmCvar_t bot_debugmessages;
#endif

vmCvar_t bot_attachments;
vmCvar_t bot_camp;

vmCvar_t bot_wp_info;
vmCvar_t bot_wp_edit;
vmCvar_t bot_wp_clearweight;
vmCvar_t bot_wp_distconnect;
vmCvar_t bot_wp_visconnect;
//end rww

wpobject_t *flagRed;
wpobject_t *oFlagRed;
wpobject_t *flagBlue;
wpobject_t *oFlagBlue;

gentity_t *eFlagRed;
gentity_t *droppedRedFlag;
gentity_t *eFlagBlue;
gentity_t *droppedBlueFlag;

//extern gentity_t *NPC;
//Local Variables
extern npcStatic_t NPCS;

//externs
extern bot_state_t	*botstates[MAX_GENTITIES];
//Local Variables
//extern gentity_t		*NPC;
//extern gNPC_t			*NPCInfo;
//extern gclient_t		*client;
//extern usercmd_t		ucmd;
extern visibility_t		enemyVisibility;

extern gNPC_t *New_NPC_t(int entNum);
extern qboolean NPC_ParseParms(const char *NPCName, gentity_t *NPC);
extern void NPC_DefaultScriptFlags(gentity_t *ent);
extern void NPC_Think(gentity_t *self);
extern void GLua_NPCEV_OnThink(gentity_t *self);
extern qboolean NPC_UpdateAngles(qboolean doPitch, qboolean doYaw);
extern void NPC_Begin(gentity_t *ent);
extern void NPC_Precache ( gentity_t *spawner );
extern char *G_ValidateUserinfo( const char *userinfo );
extern qboolean DOM_NPC_ClearPathToSpot( gentity_t *NPC, vec3_t dest, int impactEntNum );

extern void Load_NPC_Names ( void );
extern void SelectNPCNameFromList( gentity_t *NPC );
extern char *Get_NPC_Name ( int NAME_ID );

extern vmCvar_t bot_pvstype;

//#define __FAKE_NPC_LIGHTNING_SPAM__

//movement overrides
void Bot_SetForcedMovement(int bot, int forward, int right, int up)
{
	bot_state_t *bs;

	bs = botstates[bot];

	if (!bs)
	{ //not a bot
		return;
	}

	if (forward != -1)
	{
		if (bs->forceMove_Forward)
		{
			bs->forceMove_Forward = 0;
		}
		else
		{
			bs->forceMove_Forward = forward;
		}
	}
	if (right != -1)
	{
		if (bs->forceMove_Right)
		{
			bs->forceMove_Right = 0;
		}
		else
		{
			bs->forceMove_Right = right;
		}
	}
	if (up != -1)
	{
		if (bs->forceMove_Up)
		{
			bs->forceMove_Up = 0;
		}
		else
		{
			bs->forceMove_Up = up;
		}
	}
}

//check if said angles are within our fov
int InFieldOfVision(vec3_t viewangles, float fov, vec3_t angles)
{
	int i;
	float diff, angle;

	for (i = 0; i < 2; i++)
	{
		angle = AngleMod(viewangles[i]);
		angles[i] = AngleMod(angles[i]);
		diff = angles[i] - angle;
		if (angles[i] > angle)
		{
			if (diff > 180.0)
			{
				diff -= 360.0;
			}
		}
		else
		{
			if (diff < -180.0)
			{
				diff += 360.0;
			}
		}
		if (diff > 0)
		{
			if (diff > fov * 0.5)
			{
				return 0;
			}
		}
		else
		{
			if (diff < -fov * 0.5)
			{
				return 0;
			}
		}
	}
	return 1;
}

//perform pvs check based on rmg or not
qboolean BotPVSCheck( const vec3_t p1, const vec3_t p2 )
{
	if (RMG.integer && bot_pvstype.integer)
	{
		vec3_t subPoint;
		VectorSubtract(p1, p2, subPoint);

		if (VectorLength(subPoint) > 5000)
		{
			return qfalse;
		}
		return qtrue;
	}

	return trap->InPVS(p1, p2);
}

#if 0
// UQ1: Added for fast random nearby waypoints...
int DOM_GetRandomCloseWP(vec3_t org, int badwp, int unused)
{
	int		i;
	float	bestdist;
	float	flLen;
	vec3_t	a;
	int		GOOD_LIST[MAX_WPARRAY_SIZE];
	int		NUM_GOOD = 0;

	i = 0;
	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//We're not doing traces!
		bestdist = 256.0f;

	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (flLen < bestdist)
			{
				GOOD_LIST[NUM_GOOD] = i;
				NUM_GOOD++;
			}
		}

		i++;
	}

	if (NUM_GOOD <= 0)
	{// Try further...
		bestdist = 512.0f;

		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
			{
				VectorSubtract(org, gWPArray[i]->origin, a);
				flLen = VectorLength(a);

				if (flLen < bestdist)
				{
					GOOD_LIST[NUM_GOOD] = i;
					NUM_GOOD++;
				}
			}

			i++;
		}
	}

	if (NUM_GOOD <= 0)
	{// Try further... last chance...
		bestdist = 768.0f;

		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
			{
				VectorSubtract(org, gWPArray[i]->origin, a);
				flLen = VectorLength(a);

				if (flLen < bestdist)
				{
					GOOD_LIST[NUM_GOOD] = i;
					NUM_GOOD++;
				}
			}

			i++;
		}
	}

	if (NUM_GOOD <= 0)
		return -1;

	return GOOD_LIST[irand(0, NUM_GOOD - 1)];
}

int DOM_GetRandomCloseVisibleWP(gentity_t *ent, vec3_t org, int ignoreEnt, int badwp)
{
	int		i;
	float	bestdist;
	float	flLen;
	vec3_t	a;
	int		GOOD_LIST[MAX_WPARRAY_SIZE];
	int		NUM_GOOD = 0;

	i = 0;
	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//We're not doing traces!
		bestdist = 128.0f;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (flLen < bestdist && DOM_NPC_ClearPathToSpot( ent, gWPArray[i]->origin, ent->s.number ))
			{
				GOOD_LIST[NUM_GOOD] = i;
				NUM_GOOD++;
			}
		}

		i++;
	}

	if (NUM_GOOD <= 0)
	{// Try further...
		bestdist = 256.0f;

		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
			{
				VectorSubtract(org, gWPArray[i]->origin, a);
				flLen = VectorLength(a);

				if (flLen < bestdist && DOM_NPC_ClearPathToSpot( ent, gWPArray[i]->origin, ent->s.number ))
				{
					GOOD_LIST[NUM_GOOD] = i;
					NUM_GOOD++;
				}
			}

			i++;
		}
	}

	if (NUM_GOOD <= 0)
	{// Try further... last chance...
		bestdist = 512.0f;

		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
			{
				VectorSubtract(org, gWPArray[i]->origin, a);
				flLen = VectorLength(a);

				if (flLen < bestdist && DOM_NPC_ClearPathToSpot( ent, gWPArray[i]->origin, ent->s.number ))
				{
					GOOD_LIST[NUM_GOOD] = i;
					NUM_GOOD++;
				}
			}

			i++;
		}
	}

	if (NUM_GOOD <= 0)
		return -1;

	return GOOD_LIST[irand(0, NUM_GOOD - 1)];
}
#endif

//just like GetNearestVisibleWP except without visiblity checks
int DOM_GetNearestWP(vec3_t org, int badwp)
{
	int i;
	float bestdist;
	int bestindex;

	i = 0;
	/*if (RMG.integer)
	{
		bestdist = 300;
	}
	else*/
	{
		//We're not doing traces!
		bestdist = 2048;// 999999;
	}
	bestindex = -1;

	for (i = 0; i < gWPNum; i++)
	{
		if (/*gWPArray[i] &&*/ gWPArray[i]->inuse && i != badwp)
		{
			float flLen = Distance(org, gWPArray[i]->origin);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| (gWPArray[i]->flags & WPFLAG_NOMOVEFUNC))
			{//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen += 500;
			}

			if (flLen < bestdist)
			{
				bestdist = flLen;
				bestindex = i;
			}
		}
	}

	return bestindex;
}

//just like GetNearestVisibleWP except without visiblity checks
int DOM_GetNearWP(vec3_t org, int badwp)
{
	int i;
	float bestdist;
	int bestindex;

	i = 0;
	/*if (RMG.integer)
	{
		bestdist = 300;
	}
	else*/
	{
		//We're not doing traces!
		bestdist = 999999;
	}
	bestindex = -1;

//#pragma omp parallel for ordered
	for (i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			float flLen = Distance(org, gWPArray[i]->origin);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| (gWPArray[i]->flags & WPFLAG_NOMOVEFUNC))
			{//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen = flLen + 500;
			}

			if (flLen < bestdist && flLen >= 64)
			{
//#pragma omp critical
				{
//					if (flLen < bestdist)
					{// Check it is still better, because of the critical section lock...
						bestdist = flLen;
						bestindex = i;
					}
				}
			}
		}
	}

	return bestindex;
}

//get the index to the nearest visible waypoint in the global trail
//just like GetNearestVisibleWP except with a bad waypoint input
int DOM_GetNearestVisibleWP(vec3_t org, int ignore, int badwp)
{
	int i;
	float bestdist;
	int bestindex;
	vec3_t a, mins, maxs;

	i = 0;
	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//bestdist = 800;//99999;
		bestdist = 128;//99999;
		//don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

#pragma omp parallel for ordered
	for (i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			float flLen;

			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| (gWPArray[i]->flags & WPFLAG_NOMOVEFUNC)
				|| (gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPUSH)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPULL))
			{//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen = +500;
			}

			if (flLen < bestdist && OrgVisibleBox(org, mins, maxs, gWPArray[i]->origin, ignore))
			{
#pragma omp critical
				{
					if (flLen < bestdist)
					{// Check it is still better, because of the critical section lock...
						bestdist = flLen;
						bestindex = i;
					}
				}
			}
		}
	}

	return bestindex;
}

int DOM_GetNearestVisibleWP_NOBOX(vec3_t org, int ignore, int badwp)
{
	int i;
	float bestdist;
	int bestindex;
	vec3_t a, mins, maxs;

	i = 0;
	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//bestdist = 800;//99999;
		bestdist = 128;//99999;
		//don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

#pragma omp parallel for ordered
	for (i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			float flLen;
			vec3_t wpOrg, fromOrg;

			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| (gWPArray[i]->flags & WPFLAG_NOMOVEFUNC)
				|| (gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPUSH)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPULL))
			{//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen = +500;
			}

			VectorCopy(gWPArray[i]->origin, wpOrg);
			wpOrg[2] += 32;
			VectorCopy(org, fromOrg);
			fromOrg[2] += 18;

			if (flLen < bestdist && OrgVisible(fromOrg, wpOrg, ignore))
			{
#pragma omp critical
				{
					if (flLen < bestdist)
					{// Check it is still better, because of the critical section lock...
						bestdist = flLen;
						bestindex = i;
					}
				}
			}
		}
	}

	return bestindex;
}

int DOM_GetBestWaypoint(vec3_t org, int ignore, int badwp)
{// UQ1: Find the best waypoint for an origin...
	int wp = DOM_GetNearestVisibleWP(org, ignore, badwp);

	if (wp == -1)
	{// UQ1: Can't find a visible wp with a box trace.. fall back to a non-box trace and hope avoidance works...
		wp = DOM_GetNearestVisibleWP_NOBOX(org, ignore, badwp);

		if (wp == -1)
		{// UQ1: No visible waypoints at all...
			wp = DOM_GetNearestWP(org, badwp);

			if (wp == -1)
			{// UQ1: No waypoints
				return -1;
			}
		}
	}

	return wp;
}

void DOM_SetFakeNPCName(gentity_t *ent)
{// UQ1: Find their name type to send to an id to the client for names...
	if (ent->s.NPC_NAME_ID > 0) return;

	// Load names on first check...
	Load_NPC_Names();

	switch( ent->s.NPC_class )
	{
	case CLASS_STORMTROOPER_ADVANCED:
	case CLASS_STORMTROOPER_ATST_PILOT:
	case CLASS_STORMTROOPER_ATAT_PILOT:
		ent->s.NPC_NAME_ID = irand(100, 999);
		strcpy(ent->client->pers.netname, va("TA-%i", ent->s.NPC_NAME_ID));
		break;
	case CLASS_STORMTROOPER:
		ent->s.NPC_NAME_ID = irand(100, 999);
		strcpy(ent->client->pers.netname, va("TK-%i", ent->s.NPC_NAME_ID));
		break;
	case CLASS_SWAMPTROOPER:
		ent->s.NPC_NAME_ID = irand(100, 999);
		strcpy(ent->client->pers.netname, va("TS-%i", ent->s.NPC_NAME_ID));
		break;
	case CLASS_IMPWORKER:
		ent->s.NPC_NAME_ID = irand(100, 999);
		strcpy(ent->client->pers.netname, va("IW-%i", ent->s.NPC_NAME_ID));
		break;
	case CLASS_SHADOWTROOPER:
		ent->s.NPC_NAME_ID = irand(100, 999);
		strcpy(ent->client->pers.netname, va("ST-%i", ent->s.NPC_NAME_ID));
		break;
	default:
		SelectNPCNameFromList(ent);
		strcpy(ent->client->pers.netname, Get_NPC_Name(ent->s.NPC_NAME_ID));
		break;
	}

	{
		char *s, userinfo[MAX_INFO_STRING];

		trap->GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );

		// check for malformed or illegal info strings
		s = G_ValidateUserinfo( userinfo );
		if ( s && *s ) {
			return;
		}

		Info_SetValueForKey( userinfo, "name", ent->client->pers.netname );

		//trap->Print("NPC %i given name %s.\n", ent->s.number, ent->client->pers.netname);

		ClientUserinfoChanged( ent->s.number );
	}
}

void DOM_InitFakeNPC(gentity_t *bot)
{
	int i = 0;
	team_t orig_team = bot->client->sess.sessionTeam;

	bot->NPC = New_NPC_t(bot->s.number);

	//Assign the pointer for bg entity access
	bot->playerState = &bot->client->ps;

	//bot->NPC_type = G_NewString("reborn");
	bot->NPC_type = Q_strlwr( G_NewString(bot->client->pers.netname) );
	
	// Convert the spaces in the bot name to _ to match npc names...
	for (i = 0; i < strlen(bot->NPC_type); i++)
	{
		if (bot->NPC_type[i] == ' ') 
			bot->NPC_type[i] = '_';
	}

	NPC_Precache(bot);// uq1: test

	//set origin
	bot->s.pos.trType = TR_INTERPOLATE;
	bot->s.pos.trTime = level.time;
	VectorCopy( bot->r.currentOrigin, bot->s.pos.trBase );
	VectorClear( bot->s.pos.trDelta );
	bot->s.pos.trDuration = 0;
	//set angles
	bot->s.apos.trType = TR_INTERPOLATE;
	bot->s.apos.trTime = level.time;
	//VectorCopy( newent->r.currentOrigin, newent->s.apos.trBase );
	//Why was the origin being used as angles? Typo I'm assuming -rww
	VectorCopy( bot->s.angles, bot->s.apos.trBase );

	VectorClear( bot->s.apos.trDelta );
	bot->s.apos.trDuration = 0;

	bot->NPC->combatPoint = -1;

	if (!(bot->s.eFlags & EF_FAKE_NPC_BOT))
		bot->s.eFlags |= EF_FAKE_NPC_BOT;
	if (!(bot->client->ps.eFlags & EF_FAKE_NPC_BOT))
		bot->client->ps.eFlags |= EF_FAKE_NPC_BOT;

	//bot->flags |= FL_NO_KNOCKBACK;//don't fall off ledges

	bot->client->ps.weapon = WP_SABER;//init for later check in NPC_Begin

	NPC_DefaultScriptFlags(bot);

	NPC_ParseParms(bot->NPC_type, bot);

	NPC_Begin(bot);
	bot->s.eType = ET_PLAYER; // Replace ET_NPC

	// UQ1: Mark every NPC's spawn position. For patrolling that spot and stuff...
	VectorCopy(bot->r.currentOrigin, bot->spawn_pos);
	VectorCopy(bot->r.currentOrigin, bot->spawn_pos);

	// Init patrol range...
	if (bot->patrol_range <= 0) bot->patrol_range = 512.0f;

	// Init waypoints...
	bot->wpCurrent = -1;
	bot->wpNext = -1;
	bot->wpLast = -1;
	bot->longTermGoal = -1;

	// Init enemy...
	bot->enemy = NULL;

	bot->client->playerTeam = NPCTEAM_ENEMY;

	if (!(bot->s.eFlags & EF_FAKE_NPC_BOT))
		bot->s.eFlags |= EF_FAKE_NPC_BOT;
	if (!(bot->client->ps.eFlags & EF_FAKE_NPC_BOT))
		bot->client->ps.eFlags |= EF_FAKE_NPC_BOT;

#ifdef __FAKE_NPC_LIGHTNING_SPAM__
	bot->client->ps.fd.forcePowersKnown |= (1 << FP_LIGHTNING);
	bot->client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_3;
#endif //__FAKE_NPC_LIGHTNING_SPAM__

	bot->client->ps.fd.forcePowersKnown |= (1 << FP_DRAIN);
	bot->client->ps.fd.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_3;

	
	if ( g_gametype.integer >= GT_TEAM )
	{
		bot->client->sess.sessionTeam = orig_team;

		if (orig_team == FACTION_EMPIRE)
		{
			bot->client->playerTeam = NPCTEAM_ENEMY;
			bot->s.teamowner = NPCTEAM_ENEMY;
		}
		else
		{
			bot->client->playerTeam = NPCTEAM_PLAYER;
			bot->s.teamowner = (int)NPCTEAM_PLAYER;
		}
	}
}

extern void BotChangeViewAngles(bot_state_t *bs, float thinktime);

//action flags
#define ACTION_ATTACK			0x0000001
#define ACTION_USE				0x0000002
#define ACTION_RESPAWN			0x0000008
#define ACTION_JUMP				0x0000010
#define ACTION_MOVEUP			0x0000020
#define ACTION_CROUCH			0x0000080
#define ACTION_MOVEDOWN			0x0000100
#define ACTION_MOVEFORWARD		0x0000200
#define ACTION_MOVEBACK			0x0000800
#define ACTION_MOVELEFT			0x0001000
#define ACTION_MOVERIGHT		0x0002000
#define ACTION_DELAYEDJUMP		0x0008000
#define ACTION_TALK				0x0010000
#define ACTION_GESTURE			0x0020000
#define ACTION_WALK				0x0080000
#define ACTION_FORCEPOWER		0x0100000
#define ACTION_ALT_ATTACK		0x0200000
/*
#define ACTION_AFFIRMATIVE		0x0100000
#define ACTION_NEGATIVE			0x0200000
#define ACTION_GETFLAG			0x0800000
#define ACTION_GUARDBASE		0x1000000
#define ACTION_PATROL			0x2000000
#define ACTION_FOLLOWME			0x8000000
*/

qboolean DOM_FakeNPC_Parse_UCMD (bot_state_t *bs, gentity_t *bot)
{
	qboolean acted = qfalse;
	gentity_t *aiEnt = bot;
	
	if (bs)
	{
		// Set angles... Convert to ideal view angles then run the bot code...
		VectorSet(bs->ideal_viewangles, SHORT2ANGLE(bot->client->pers.cmd.angles[PITCH] + bot->client->ps.delta_angles[PITCH]), SHORT2ANGLE(bot->client->pers.cmd.angles[YAW] + bot->client->ps.delta_angles[YAW]), SHORT2ANGLE(bot->client->pers.cmd.angles[ROLL] + bot->client->ps.delta_angles[ROLL]));
		VectorCopy(bs->ideal_viewangles, bs->viewangles);
		VectorCopy(bs->ideal_viewangles, bot->client->ps.viewangles);
		//VectorCopy(bs->ideal_viewangles, bot->r.currentAngles);
		//VectorCopy(bs->ideal_viewangles, bot->s.angles);
		trap->EA_View(bs->client, bs->ideal_viewangles);
	}

	/*
	if (bot->NPC->goalEntity)
	{
		vec3_t dir;
		VectorSubtract(bot->NPC->goalEntity->r.currentOrigin, bot->r.currentOrigin, dir);
		VectorCopy(dir, bot->client->ps.moveDir);
	}

	if (bot->client->pers.cmd.buttons & BUTTON_WALKING)
	{
		trap->EA_Action(bot->s.number, ACTION_WALK);
		trap->EA_Move(bot->s.number, bot->client->ps.moveDir, 5000.0);
	}
	else
	{
		trap->EA_Move(bot->s.number, bot->client->ps.moveDir, 5000.0);
	}
	*/

	if (bot->enemy && Distance(bot->r.currentOrigin, bot->enemy->r.currentOrigin) > 64)
	{
		//vec3_t dir;

		bot->NPC->goalEntity = bot->enemy;
		
		/*
		VectorSubtract(bot->NPC->goalEntity->r.currentOrigin, bot->r.currentOrigin, dir);
		VectorCopy(dir, bot->client->ps.moveDir);

		if (bot->client->pers.cmd.buttons & BUTTON_WALKING)
		{
			trap->EA_Action(bot->s.number, ACTION_WALK);
			trap->EA_Move(bot->s.number, bot->client->ps.moveDir, 5000.0);
			acted = qtrue;
		}
		else
		{
			trap->EA_Move(bot->s.number, bot->client->ps.moveDir, 5000.0);
			acted = qtrue;
		}
		*/

		NPC_MoveToGoal(aiEnt, qtrue);
		acted = qtrue;
	}

	/*
	if (bot->client->pers.cmd.upmove > 0)
	{
		trap->EA_Jump(bot->s.number);
		acted = qtrue;
	}
	
	if (bot->client->pers.cmd.upmove < 0)
	{
		trap->EA_Crouch(bot->s.number);
		acted = qtrue;
	}
	
	if (bot->client->pers.cmd.rightmove > 0)
	{
		trap->EA_MoveRight(bot->s.number);
		acted = qtrue;
	}

	if (bot->client->pers.cmd.rightmove < 0)
	{
		trap->EA_MoveLeft(bot->s.number);
		acted = qtrue;
	}

	if (bot->client->pers.cmd.forwardmove > 0)
	{
		trap->EA_MoveForward(bot->s.number);
		acted = qtrue;
	}

	if (bot->client->pers.cmd.forwardmove < 0)
	{
		trap->EA_MoveBack(bot->s.number);
		acted = qtrue;
	}
	*/

	if (bot->client->pers.cmd.buttons & BUTTON_ATTACK)
	{
		/*
		if (bot->client->ps.weapon == WP_SABER && bot->client->pers.cmd.rightmove == 0)
		{
			trap->EA_MoveLeft(bs->client); // UQ1: Also move left for a better animation then the plain one...

			if (!(aiEnt->client->pers.cmd.buttons & BUTTON_WALKING))
				trap->EA_Action(bs->client, ACTION_WALK);
		}
		*/

		trap->EA_Attack(bot->s.number);
		acted = qtrue;
	}

	if (bot->client->pers.cmd.buttons & BUTTON_ALT_ATTACK)
	{
		trap->EA_Alt_Attack(bot->s.number);
		acted = qtrue;
	}

	if (bot->client->pers.cmd.buttons & BUTTON_USE)
	{
		trap->EA_Use(bot->s.number);
		acted = qtrue;
	}

	/*
	if (bot->client->pers.cmd.buttons & BUTTON_WALKING)
	{
		trap->EA_Action(bot->s.number, ACTION_WALK);
		acted = qtrue;
	}
	*/

	return acted;
}

vec3_t oldMoveDir;
extern void ClientThink_real( gentity_t *ent );
extern void NPC_ApplyRoff (void);
extern void NPC_Think ( gentity_t *self);

// UQ1: Now lets see if bots can share NPC AI....
void DOM_StandardBotAI(bot_state_t *bs, float thinktime)
{
	gentity_t *aiEnt = &g_entities[bs->client];

	if (!(aiEnt->s.eFlags & EF_FAKE_NPC_BOT))
		aiEnt->s.eFlags |= EF_FAKE_NPC_BOT;
	if (!(aiEnt->client->ps.eFlags & EF_FAKE_NPC_BOT))
		aiEnt->client->ps.eFlags |= EF_FAKE_NPC_BOT;

	if (!aiEnt->NPC)
		DOM_InitFakeNPC(aiEnt);

	DOM_SetFakeNPCName(aiEnt); // Make sure they have a name...

	memset(&aiEnt->client->pers.cmd, 0, sizeof(aiEnt->client->pers.cmd));

	if (aiEnt->health < 1 || aiEnt->client->ps.pm_type == PM_DEAD)
	{
		//RACC - Try to respawn if you're done talking.
		if (rand() % 10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap->EA_Attack(bs->client);

			aiEnt->enemy = aiEnt->NPC->goalEntity = NULL; // Clear enemy???
		}

		return;
	}

	VectorCopy(oldMoveDir, aiEnt->client->ps.moveDir);
	//or use client->pers.lastCommand?

	aiEnt->NPC->last_ucmd.serverTime = level.time - 50;

	NPC_Think(aiEnt);
	DOM_FakeNPC_Parse_UCMD(bs, aiEnt);

#ifndef __NO_ICARUS__
	trap->ICARUS_MaintainTaskManager(aiEnt->s.number);
#endif //__NO_ICARUS__
	VectorCopy(aiEnt->r.currentOrigin, aiEnt->client->ps.origin);

	if (aiEnt->client->ps.pm_flags & PMF_DUCKED && aiEnt->r.maxs[2] > aiEnt->client->ps.crouchheight)
	{
		aiEnt->r.maxs[2] = aiEnt->client->ps.crouchheight;
		aiEnt->r.maxs[1] = 8;
		aiEnt->r.maxs[0] = 8;
		aiEnt->r.mins[1] = -8;
		aiEnt->r.mins[0] = -8;
		trap->LinkEntity((sharedEntity_t *)aiEnt);
	}
	else if (!(aiEnt->client->ps.pm_flags & PMF_DUCKED) && (aiEnt->r.maxs[2] < aiEnt->client->ps.standheight || aiEnt->r.maxs[1] > 10))
	{
		aiEnt->r.maxs[2] = aiEnt->client->ps.standheight;
		aiEnt->r.maxs[1] = 10;
		aiEnt->r.maxs[0] = 10;
		aiEnt->r.mins[1] = -10;
		aiEnt->r.mins[0] = -10;
		trap->LinkEntity((sharedEntity_t *)aiEnt);
	}

	//trap->Print(S_COLOR_RED "Bot [%s] is using NPC AI.\n", aiEnt->client->pers.netname);
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState( int clientNum, playerState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[clientNum];
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->client ) {
		return qfalse;
	}

	memcpy( state, &ent->client->ps, sizeof(playerState_t) );
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState( int entityNum, entityState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[entityNum];
	memset( state, 0, sizeof(entityState_t) );
	if (!ent->inuse) return qfalse;
	if (!ent->r.linked) return qfalse;
	if (ent->r.svFlags & SVF_NOCLIENT) return qfalse;
	memcpy( state, &ent->s, sizeof(entityState_t) );
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
int BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state ) {
	int		entNum;

	entNum = trap->BotGetSnapshotEntity( clientNum, sequence );
	if ( entNum == -1 ) {
		memset(state, 0, sizeof(entityState_t));
		return -1;
	}

	BotAI_GetEntityState( entNum, state );

	return sequence + 1;
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo(int entnum, aas_entityinfo_t *info) {
	if (entnum < 0) return;
	trap->AAS_EntityInfo(entnum, info);
}

/*
==============
NumBots
==============
*/
int NumBots(void) {
	return numbots;
}

/*
==============
AngleDifference
==============
*/
float AngleDifference(float ang1, float ang2) {
	float diff;

	diff = ang1 - ang2;
	if (ang1 > ang2) {
		if (diff > 180.0) diff -= 360.0;
	}
	else {
		if (diff < -180.0) diff += 360.0;
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle(float angle, float ideal_angle, float speed) {
	float move;

	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if (angle == ideal_angle) return angle;
	move = ideal_angle - angle;
	if (ideal_angle > angle) {
		if (move > 180.0) move -= 360.0;
	}
	else {
		if (move < -180.0) move += 360.0;
	}
	if (move > 0) {
		if (move > speed) move = speed;
	}
	else {
		if (move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles(bot_state_t *bs, float thinktime) {
	float diff, factor, maxchange, anglespeed, disired_speed;
	int i;

	if (bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		if (bs->settings.skill <= 1)
		{
			factor = (bs->skills.turnspeed_combat*0.4f)*bs->settings.skill;
		}
		else if (bs->settings.skill <= 2)
		{
			factor = (bs->skills.turnspeed_combat*0.6f)*bs->settings.skill;
		}
		else if (bs->settings.skill <= 3)
		{
			factor = (bs->skills.turnspeed_combat*0.8f)*bs->settings.skill;
		}
		else
		{
			factor = bs->skills.turnspeed_combat*bs->settings.skill;
		}
	}
	else
	{
		factor = bs->skills.turnspeed;
	}

	if (factor > 1)
	{
		factor = 1;
	}
	if (factor < 0.001)
	{
		factor = 0.001f;
	}

	maxchange = bs->skills.maxturn;

	//if (maxchange < 240) maxchange = 240;
	maxchange *= thinktime;
	for (i = 0; i < 2; i++) {
		bs->viewangles[i] = AngleMod(bs->viewangles[i]);
		bs->ideal_viewangles[i] = AngleMod(bs->ideal_viewangles[i]);
		diff = AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]);
		disired_speed = diff * factor;
		bs->viewanglespeed[i] += (bs->viewanglespeed[i] - disired_speed);
		if (bs->viewanglespeed[i] > 180) bs->viewanglespeed[i] = maxchange;
		if (bs->viewanglespeed[i] < -180) bs->viewanglespeed[i] = -maxchange;
		anglespeed = bs->viewanglespeed[i];
		if (anglespeed > maxchange) anglespeed = maxchange;
		if (anglespeed < -maxchange) anglespeed = -maxchange;
		bs->viewangles[i] += anglespeed;
		bs->viewangles[i] = AngleMod(bs->viewangles[i]);
		bs->viewanglespeed[i] *= 0.45 * (1 - factor);
	}
	if (bs->viewangles[PITCH] > 180) bs->viewangles[PITCH] -= 360;
	trap->EA_View(bs->client, bs->viewangles);
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand(bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3], int time, int useTime) {
	vec3_t angles, forward, right;
	short temp;
	int j;
	float f, r, u, m;

	//clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ALT_ATTACK) ucmd->buttons |= BUTTON_ALT_ATTACK;
//	if (bi->actionflags & ACTION_TALK) ucmd->buttons |= BUTTON_TALK;
	if (bi->actionflags & ACTION_GESTURE) ucmd->buttons |= BUTTON_GESTURE;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK) ucmd->buttons |= BUTTON_WALKING;

	if (bi->actionflags & ACTION_FORCEPOWER) ucmd->buttons |= BUTTON_FORCEPOWER;

	if (useTime < level.time && Q_irand(1, 10) < 5)
	{ //for now just hit use randomly in case there's something useable around
		ucmd->buttons |= BUTTON_USE;
	}
#if 0
// Here's an interesting bit.  The bots in TA used buttons to do additional gestures.
// I ripped them out because I didn't want too many buttons given the fact that I was already adding some for JK2.
// We can always add some back in if we want though.
	if (bi->actionflags & ACTION_AFFIRMATIVE) ucmd->buttons |= BUTTON_AFFIRMATIVE;
	if (bi->actionflags & ACTION_NEGATIVE) ucmd->buttons |= BUTTON_NEGATIVE;
	if (bi->actionflags & ACTION_GETFLAG) ucmd->buttons |= BUTTON_GETFLAG;
	if (bi->actionflags & ACTION_GUARDBASE) ucmd->buttons |= BUTTON_GUARDBASE;
	if (bi->actionflags & ACTION_PATROL) ucmd->buttons |= BUTTON_PATROL;
	if (bi->actionflags & ACTION_FOLLOWME) ucmd->buttons |= BUTTON_FOLLOWME;
#endif //0

	if (bi->weapon == WP_NONE)
	{
#ifdef _DEBUG
//		Com_Printf("WARNING: Bot tried to use WP_NONE!\n");
#endif
		bi->weapon = WP_MODULIZED_WEAPON;
	}

	//
	ucmd->weapon = bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);
	ucmd->angles[ROLL] = ANGLE2SHORT(bi->viewangles[ROLL]);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		temp = ucmd->angles[j] - delta_angles[j];
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if (bi->dir[2]) angles[PITCH] = bi->viewangles[PITCH];
	else angles[PITCH] = 0;
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);
	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	//set the view independent movement
	f = DotProduct(forward, bi->dir);
	r = DotProduct(right, bi->dir);
	u = fabs(forward[2]) * bi->dir[2];
	m = fabs(f);

	if (fabs(r) > m) {
		m = fabs(r);
	}

	if (fabs(u) > m) {
		m = fabs(u);
	}

	if (m > 0) {
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = f;
	ucmd->rightmove = r;
	ucmd->upmove = u;
	//normal keyboard movement
	if (bi->actionflags & ACTION_MOVEFORWARD) ucmd->forwardmove = 127;
	if (bi->actionflags & ACTION_MOVEBACK) ucmd->forwardmove = -127;
	if (bi->actionflags & ACTION_MOVELEFT) ucmd->rightmove = -127;
	if (bi->actionflags & ACTION_MOVERIGHT) ucmd->rightmove = 127;
	//jump/moveup
	if (bi->actionflags & ACTION_JUMP) ucmd->upmove = 127;
	//crouch/movedown
	if (bi->actionflags & ACTION_CROUCH) ucmd->upmove = -127;
}

/*
==============
BotUpdateInput
==============
*/
void BotUpdateInput(bot_state_t *bs, int time, int elapsed_time) {
	bot_input_t bi;
	int j;

	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//change the bot view angles
	BotChangeViewAngles(bs, (float) elapsed_time / 1000);
	//retrieve the bot input
	trap->EA_GetInput(bs->client, (float) time / 1000, &bi);
	//respawn hack
	if (bi.actionflags & ACTION_RESPAWN) {
		if (bs->lastucmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN|ACTION_ATTACK);
	}
	//convert the bot input to a usercmd
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time, bs->noUseTime);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

/*
==============
BotAIRegularUpdate
==============
*/
void BotAIRegularUpdate(void) {
	if (regularupdate_time < FloatTime()) {
		trap->BotUpdateEntityItems();
		regularupdate_time = FloatTime() + 0.3;
	}
}

/*
==============
RemoveColorEscapeSequences
==============
*/
void RemoveColorEscapeSequences( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (Q_IsColorStringExt(&text[i])) {
			i++;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}


/*
==============
BotAI
==============
*/

int BotAI(int client, float thinktime) {
	bot_state_t *bs;
	char buf[1024], *args;
	int j;
	gentity_t *bot = &g_entities[client];
#ifdef _DEBUG
	int start = 0;
	int end = 0;
#endif

	trap->EA_ResetInput(client);
	//
	bs = botstates[client];
	if (!bs || !bs->inuse) {
		trap->Print("BotAI: client %d is not setup\n", client);
		return qfalse;
	}

	//retrieve the current client state
	BotAI_GetClientState(client, &bs->cur_ps);

	//retrieve any waiting server commands
	while (trap->BotGetServerCommand(client, buf, sizeof(buf))) {
		//have buf point to the command and args to the command arguments
		args = strchr(buf, ' ');
		if (!args) continue;
		*args++ = '\0';

		//remove color espace sequences from the arguments
		RemoveColorEscapeSequences(args);

		if (!Q_stricmp(buf, "cp "))
		{ /*CenterPrintf*/
		}
		else if (!Q_stricmp(buf, "cs"))
		{ /*ConfigStringModified*/
		}
		else if (!Q_stricmp(buf, "scores"))
		{ /*FIXME: parse scores?*/
		}
		else if (!Q_stricmp(buf, "clientLevelShot"))
		{ /*ignore*/
		}
	}
	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//increase the local time of the bot
	bs->ltime += thinktime;
	//
	bs->thinktime = thinktime;
	//origin of the bot
	VectorCopy(bs->cur_ps.origin, bs->origin);
	//eye coordinates of the bot
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;
	//get the area the bot is in

#ifdef _DEBUG
	start = trap->Milliseconds();
#endif

	gentity_t *aiEnt = bot;

	DOM_StandardBotAI(bs, thinktime); // UQ1: Uses Dominance NPC AI...

#ifdef _DEBUG
	end = trap->Milliseconds();

	trap->Cvar_Update(&bot_debugmessages);

	if (bot_debugmessages.integer)
	{
		Com_Printf("Single AI frametime: %i\n", (end - start));
	}
#endif

	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//everything was ok
	return qtrue;
}

/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink(void) {
	int i, botnum;

	botnum = 0;

	for( i = 0; i < MAX_GENTITIES; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = BOT_THINK_TIME * botnum / numbots;
		botnum++;
	}
}

int PlayersInGame(void)
{
	int i = 0;
	gentity_t *ent;
	int pl = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED)
		{
			pl++;
		}

		i++;
	}

	return pl;
}

/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart) {
	bot_state_t *bs;

	if (!botstates[client]) botstates[client] = (bot_state_t *) G_Alloc(sizeof(bot_state_t), "BotAISetupClient");
																			  //rww - G_Alloc bad! B_Alloc good.

	memset(botstates[client], 0, sizeof(bot_state_t));

	bs = botstates[client];

	if (bs && bs->inuse) {
		trap->Print("BotAISetupClient: client %d already setup\n", client);
		return qfalse;
	}

	memcpy(&bs->settings, settings, sizeof(bot_settings_t));

	bs->client = client; //need to know the client number before doing personality stuff

	//initialize weapon weight defaults..
	bs->botWeaponWeights[WP_NONE] = 0;
	bs->botWeaponWeights[WP_SABER] = 10;
	bs->botWeaponWeights[WP_THERMAL] = 15;
	bs->botWeaponWeights[WP_TRIP_MINE] = 0;
	bs->botWeaponWeights[WP_DET_PACK] = 0;
	bs->botWeaponWeights[WP_MELEE] = 1;

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		bs->botWeaponWeights[WP_SABER] = 13;
	}

	//allocate a goal state
	bs->gs = trap->BotAllocGoalState(client);

	//allocate a weapon state
	bs->ws = trap->BotAllocWeaponState();

	bs->inuse = qtrue;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap->BotAllocMoveState();
	trap->BotResetMoveState(bs->ms);
	numbots++;

	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();

	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient(int client, qboolean restart) {
	bot_state_t *bs;

	bs = botstates[client];
	if (!bs || !bs->inuse) {
		//trap->Print("BotAIShutdownClient: client %d already shutdown\n", client);
		return qfalse;
	}

	trap->BotFreeMoveState(bs->ms);
	//free the goal state`
	trap->BotFreeGoalState(bs->gs);
	//free the weapon weights
	trap->BotFreeWeaponState(bs->ws);
	//
	//clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return qtrue;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void BotResetState(bot_state_t *bs) {
	int client, entitynum, inuse;
	int movestate, goalstate, weaponstate;
	bot_settings_t settings;
	playerState_t ps;							//current player state
	float entergame_time;

	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	movestate = bs->ms;
	goalstate = bs->gs;
	weaponstate = bs->ws;
	entergame_time = bs->entergame_time;
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->entergame_time = entergame_time;
	//reset several states
	if (bs->ms) trap->BotResetMoveState(bs->ms);
	if (bs->gs) trap->BotResetGoalState(bs->gs);
	if (bs->ws) trap->BotResetWeaponState(bs->ws);
	if (bs->gs) trap->BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap->BotResetAvoidReach(bs->ms);
}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap( int restart ) {
	int			i;

	for (i = 0; i < MAX_GENTITIES; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotResetState( botstates[i] );
			botstates[i]->setupcount = 4;
		}
	}

	return qtrue;
}

//rww - bot ai

//standard visibility check
int OrgVisible(vec3_t org1, vec3_t org2, int ignore)
{
	trace_t tr;

	trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0 );

	if (tr.fraction == 1)
	{
		return 1;
	}

	return 0;
}

//special waypoint visibility check
int WPOrgVisible(gentity_t *bot, vec3_t org1, vec3_t org2, int ignore)
{
	trace_t tr;
	gentity_t *ownent;

	trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.fraction != 1 && tr.entityNum != ENTITYNUM_NONE && g_entities[tr.entityNum].s.eType == ET_SPECIAL)
		{
			if (g_entities[tr.entityNum].parent && g_entities[tr.entityNum].parent->client)
			{
				ownent = g_entities[tr.entityNum].parent;

				if (OnSameTeam(bot, ownent) || bot->s.number == ownent->s.number)
				{
					return 1;
				}
			}
			return 2;
		}

		return 1;
	}

	return 0;
}

//visibility check with hull trace
int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore)
{
	trace_t tr;

	if (RMG.integer)
	{
		trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);
	}
	else
	{
		trap->Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);
	}

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}

	return 0;
}


//check for new events
void UpdateEventTracker(void)
{
	int i;

	i = 0;

	while (i < MAX_GENTITIES)
	{
		if (i >= MAX_CLIENTS)
		{
			gentity_t *ent = &g_entities[i];

			if (ent && ent->inuse && ent->client)
			{
				gBotEventTracker[i].eventSequence = ent->client->ps.eventSequence;
				gBotEventTracker[i].events[0] = ent->client->ps.events[0];
				gBotEventTracker[i].events[1] = ent->client->ps.events[1];
				gBotEventTracker[i].eventTime = level.time + 0.5;
			}
			else
			{
				gBotEventTracker[i].eventSequence = 0;
				gBotEventTracker[i].events[0] = 0;
				gBotEventTracker[i].events[1] = 0;
				gBotEventTracker[i].eventTime = 0;
			}
		}
		else if (gBotEventTracker[i].eventSequence != level.clients[i].ps.eventSequence)
		{ //updated event
			gBotEventTracker[i].eventSequence = level.clients[i].ps.eventSequence;
			gBotEventTracker[i].events[0] = level.clients[i].ps.events[0];
			gBotEventTracker[i].events[1] = level.clients[i].ps.events[1];
			gBotEventTracker[i].eventTime = level.time + 0.5;
		}

		i++;
	}
}

int gUpdateVars = 0;

/*
==================
BotAIStartFrame
==================
*/
int BotAIStartFrame(int time) {
	int i;
	int elapsed_time, thinktime;
	static int local_time;
//	static int botlib_residual;
	static int lastbotthink_time;

	if (gUpdateVars < level.time)
	{
		trap->Cvar_Update(&bot_pvstype);
		trap->Cvar_Update(&bot_camp);
		trap->Cvar_Update(&bot_attachments);
		trap->Cvar_Update(&bot_forgimmick);
		trap->Cvar_Update(&bot_honorableduelacceptance);
#ifndef FINAL_BUILD
		trap->Cvar_Update(&bot_getinthecarrr);
#endif
		gUpdateVars = level.time + 1000;
	}

	G_CheckBotSpawn();

#ifdef __BUGGY_MEM_MANAGEMENT__
	//rww - addl bot frame functions
	if (gBotEdit)
	{
		trap->Cvar_Update(&bot_wp_info);
		BotWaypointRender();
	}
#endif //__BUGGY_MEM_MANAGEMENT__

	UpdateEventTracker();
	//end rww

	//cap the bot think time
	//if the bot think time changed we should reschedule the bots
	if (BOT_THINK_TIME != lastbotthink_time) {
		lastbotthink_time = BOT_THINK_TIME;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	if (elapsed_time > BOT_THINK_TIME) thinktime = elapsed_time;
	else thinktime = BOT_THINK_TIME;

	// execute scheduled bot AI
	for( i = 0; i < MAX_GENTITIES; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//
		botstates[i]->botthink_residual += elapsed_time;
		//
		if ( botstates[i]->botthink_residual >= thinktime ) {
			botstates[i]->botthink_residual -= thinktime;

			if (g_entities[i].client->pers.connected == CON_CONNECTED) {
				BotAI(i, (float) thinktime / 1000);
			}
		}
	}

	// execute bot user commands every frame
	for( i = 0; i < MAX_GENTITIES; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap->BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
	}

	return qtrue;
}

/*
==============
BotAISetup
==============
*/
int BotAISetup( int restart ) {
	//rww - new bot cvars..
	trap->Cvar_Register(&bot_forcepowers, "bot_forcepowers", "1", CVAR_CHEAT);
	trap->Cvar_Register(&bot_forgimmick, "bot_forgimmick", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_honorableduelacceptance, "bot_honorableduelacceptance", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_pvstype, "bot_pvstype", "1", CVAR_CHEAT);
#ifndef FINAL_BUILD
	trap->Cvar_Register(&bot_getinthecarrr, "bot_getinthecarrr", "0", 0);
#endif

#ifdef _DEBUG
	trap->Cvar_Register(&bot_nogoals, "bot_nogoals", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_debugmessages, "bot_debugmessages", "0", CVAR_CHEAT);
#endif

	trap->Cvar_Register(&bot_attachments, "bot_attachments", "1", 0);
	trap->Cvar_Register(&bot_camp, "bot_camp", "1", 0);

	trap->Cvar_Register(&bot_wp_info, "bot_wp_info", "1", 0);
	trap->Cvar_Register(&bot_wp_edit, "bot_wp_edit", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_wp_clearweight, "bot_wp_clearweight", "1", 0);
	trap->Cvar_Register(&bot_wp_distconnect, "bot_wp_distconnect", "1", 0);
	trap->Cvar_Register(&bot_wp_visconnect, "bot_wp_visconnect", "1", 0);

	trap->Cvar_Update(&bot_forcepowers);
	//end rww

#ifdef __USE_NAVLIB__
	G_NavlibNavInit();

	if (G_NavmeshIsLoaded())
	{
		extern qboolean			WATER_ENABLED;
		extern float			MAP_WATER_LEVEL;

		if (WATER_ENABLED)
		{// Block water usage by disabling everything below mapInfo's water height...
			vec3_t origin, mins, maxs;
			origin[0] = origin[1] = 0.0;
			origin[2] = MAP_WATER_LEVEL;

			mins[0] = mins[1] = mins[2] = -999999.9;

			maxs[0] = maxs[1] = 999999.9;
			maxs[2] = 0.0;

			Com_Printf("Blocking water at height %f. mins %f %f %f. maxs %f %f %f.\n", MAP_WATER_LEVEL, mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);
			G_NavlibDisableArea(origin, mins, maxs);
		}
	}
#endif //__USE_NAVLIB__

	//if the game is restarted for a tournament
	if (restart) {
		return qtrue;
	}

	//initialize the bot states
	memset( botstates, 0, sizeof(botstates) );

	if (trap->BotLibSetup() != 0)
	{
		//trap->Print("BOTLIB SETUP FAILED!?!?!?\n");
		return qfalse; //wtf?!
	}

#ifdef __AAS_AI_TESTING__
	{
		vmCvar_t	mapname;
		int loaded = 0;
		trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		loaded = trap->BotLibLoadMap(mapname.string);
		//trap->Print("BOTLIB LOAD MAP: %i.\n", loaded);

		if (trap->BotLibStartFrame(FloatTime()) == 1)
		{
			trap->Print("BOTLIB STARTFRAME FAILED!?!?!?\n");
		}
	}
#endif //__AAS_AI_TESTING__
	
	trap->Print("^5-------- ^7BotAISetup Completed^5 ---------\n");

	return qtrue;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown( int restart ) {

	int i;

#ifdef __USE_NAVLIB__
	G_NavlibNavCleanup();
#endif //__USE_NAVLIB__

	//if the game is restarted for a tournament
	if ( restart ) {
		//shutdown all the bots in the botlib
		for (i = 0; i < MAX_GENTITIES; i++) {
			if (botstates[i] && botstates[i]->inuse) {
				BotAIShutdownClient(botstates[i]->client, (qboolean)restart);
			}
		}
		//don't shutdown the bot library
	}
	else {
		trap->BotLibShutdown();
	}

	return qtrue;
}
