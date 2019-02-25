#include "qcommon/q_shared.h"
#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

//#define ___AI_PATHING_DEBUG___
#define ___PATH_SHORTEN___

extern int gWPNum;
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

extern int DOM_GetNearestWP(vec3_t org, int badwp);
extern int NPC_GetNextNode(gentity_t *NPC);
extern qboolean UQ1_UcmdMoveForDir ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest );
//extern qboolean NPC_CoverpointVisible ( gentity_t *NPC, int coverWP );
extern int DOM_GetRandomCloseWP(vec3_t org, int badwp, int unused);
extern int DOM_GetNearestVisibleWP(vec3_t org, int ignore, int badwp);
extern int DOM_GetNearestVisibleWP_NOBOX(vec3_t org, int ignore, int badwp);
extern gentity_t *NPC_PickEnemyExt( qboolean checkAlerts );
extern void ST_Speech( gentity_t *self, int speechType, float failChance );

extern qboolean NPC_EnemyAboveMe( gentity_t *NPC );

extern vmCvar_t npc_pathing;

// Recorded in g_mover.c
extern vec3_t		MOVER_LIST[1024];
extern vec3_t		MOVER_LIST_TOP[1024];
extern int			MOVER_LIST_NUM;

// UQ1: The maximum distance an NPC is allowed to go to get to the next link (eg: if he is trying to move further, then he is lost/fallen/etc)
float MAX_LINK_DISTANCE = 1024.0;//512.0;

/*///////////////////////////////////////////////////
NPC_GetNextNode
if the bot has reached a node, this function selects the next node
that he will go to, and returns it
right now it's being developed, feel free to experiment
*////////////////////////////////////////////////////

int NPC_GetNextNode(gentity_t *NPC)
{
#ifndef __USE_NAVMESH__
	int node = WAYPOINT_NONE;

	//we should never call this in BOTSTATE_MOVE with no goal
	//setup the goal/path in HandleIdleState
	if (NPC->longTermGoal == WAYPOINT_NONE)
	{
		return WAYPOINT_NONE;
	}

	if (NPC->pathsize <= 0)	//if the bot is at the end of his path, this shouldn't have been called
	{
		//NPC->longTermGoal = WAYPOINT_NONE;	//reset to having no goal
		return WAYPOINT_NONE;
	}

	if (NPC->npc_cower_runaway || NPC->isPadawan)
		NPC_ShortenPath(NPC); // Shorten any path we can, if we are running away from combat...

	if (NPC->pathsize <= 0)	//if the bot is at the end of his path, this shouldn't have been called
	{
		//NPC->longTermGoal = WAYPOINT_NONE;	//reset to having no goal
		return WAYPOINT_NONE;
	}

	node = NPC->pathlist[NPC->pathsize-1];	//pathlist is in reverse order
	
#ifdef ___AI_PATHING_DEBUG___
	//if (node < -1) { gentity_t *N = NULL; N->alt_fire = qtrue; } // force crash for pathlist debugging in relwithdebuginfo mode
	/*if (node < -1) 
	{// Debug output the pathlist...
		int i;
		trap->Print("Pathsize is [%i]. Path [", NPC->pathsize);
		for (i = 0; i < NPC->pathsize; i++)
		{
			trap->Print(" %i", NPC->pathlist[i]);
		}
		trap->Print(" ]\n");
	}*/
#endif //___AI_PATHING_DEBUG___
	
	NPC->pathsize--;	//mark that we've moved another node

	if (NPC->pathsize <= 0)
	{
		if (NPC->wpCurrent < 0)
		{
			node = NPC->wpCurrent = NPC->longTermGoal;

			NPC->wpTravelTime = level.time + 15000;
			NPC->wpSeenTime = level.time;
			NPC->last_move_time = level.time;
		}
		else
		{
			node = NPC->longTermGoal;

			NPC->wpTravelTime = level.time + 15000;
			NPC->wpSeenTime = level.time;
			NPC->last_move_time = level.time;
		}
	}
	return node;
#else //!__USE_NAVMESH__
	return -1;
#endif //__USE_NAVMESH__
}

qboolean NPC_ShortenJump(gentity_t *NPC, int node)
{
#ifndef __USE_NAVMESH__
	float MAX_JUMP_DISTANCE = 192.0;
	float dist = Distance(gWPArray[node]->origin, NPC->r.currentOrigin);
	
	if (NPC_IsJetpacking(NPC)) return qfalse;

	if (NPC_IsJedi(NPC)) MAX_JUMP_DISTANCE = 512.0; // Jedi can jump further...

	if (dist <= MAX_JUMP_DISTANCE
		&& NPC_TryJump( NPC, gWPArray[node]->origin ))
	{// Looks like we can jump there... Let's do that instead of failing!
		//trap->Print("%s is shortening path using jump.\n", NPC->client->pers.netname);
		return qtrue; // next think...
	}
#endif //__USE_NAVMESH__

	return qfalse;
}

void NPC_ShortenPath(gentity_t *NPC)
{
#ifndef __USE_NAVMESH__
#ifdef ___PATH_SHORTEN___
	qboolean	found = qfalse;
	int			position = -1;
	int			x = 0;

	if (NPC->wpCurrent < 0 || NPC->wpCurrent > gWPNum) return;

	for (x = 0; x < gWPArray[NPC->wpCurrent]->neighbornum; x++)
	{
		if (gWPArray[NPC->wpCurrent]->neighbors[x].num == NPC->longTermGoal)
		{// Our current wp links direct to our goal!
			//int shortenedBy = NPC->pathsize;
			//trap->Print("%s found a shorter (direct to goal! - shortened by %i) path.\n", NPC->NPC_type, shortenedBy);
			NPC->pathsize = 0;
			found = qtrue;
			break;
		}

		for (position = 0; position < NPC->pathsize; position++)
		{
			if (gWPArray[NPC->wpCurrent]->neighbors[x].num == NPC->pathlist[position])
			{// The current wp links direct to this node!
				//int shortenedBy = NPC->pathsize - position;
				//trap->Print("%s found a shorter (shortened by %i) path.\n", NPC->NPC_type, shortenedBy);
				NPC->pathsize = position;
				found = qtrue;
				break;
			}

			if (NPC_ShortenJump(NPC, NPC->pathlist[position]))
			{// Can we jump to a position closer to the end goal?
				//int shortenedBy = NPC->pathsize - position;
				NPC->pathsize = position;
				found = qtrue;
				//trap->Print("%s found a shorter path using jump (shortened by %i).\n", NPC->NPC_type, shortenedBy);
				break;
			}
		}

		if (found) break;
	}
#endif //___PATH_SHORTEN___
#endif //__USE_NAVMESH__
}

qboolean NPC_FindNewWaypoint( gentity_t *aiEnt)
{
#ifndef __USE_NAVMESH__
	gentity_t	*NPC = aiEnt;

	// Try to find a visible waypoint first...
	if (NPC->wpCurrent >= 0 
		&& NPC->wpCurrent < gWPNum 
		&& Distance(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) < 128
		&& InFOV2(gWPArray[NPC->wpCurrent]->origin, NPC, 120, 180)
		&& (NPC->wpSeenTime > level.time || OrgVisible(NPC->r.currentOrigin, gWPArray[NPC->wpCurrent]->origin, NPC->s.number)))
	{// Current one looks fine...
		NPC->wpTravelTime = level.time + 15000;
		NPC->wpSeenTime = level.time;
		NPC->last_move_time = level.time;
		return qtrue;
	}

	NPC->wpCurrent = DOM_GetNearestWP(NPC->r.currentOrigin, NPC->wpCurrent);

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		//G_Printf("NPC Waypointing Debug: NPC %i (%s) failed to find a waypoint for itself.", NPC->s.number, NPC->NPC_type);
		return qfalse; // failed... try again after som avoidance code...
	}

	NPC->wpTravelTime = level.time + 15000;
	NPC->wpSeenTime = level.time;
	NPC->last_move_time = level.time;
#endif //__USE_NAVMESH__

	return qtrue; // all good, we have a new waypoint...
}

void NPC_SetEnemyGoal(gentity_t *aiEnt)
{
#ifndef __USE_NAVMESH__
	qboolean IS_COVERPOINT = qfalse;
	int			COVERPOINT_WP = -1;
	int			COVERPOINT_OFC_WP = -1;
	gentity_t	*NPC = aiEnt;

	//if (NPC->wpSeenTime > level.time)
	//	return; // wait for next route creation...

	/*
	if (NPC->wpTravelTime < level.time)
		G_Printf("wp travel time\n");
	else 
		G_Printf("Bad wps (lt: %i) (ps: %i) (wc: %i) (wn: %i)\n", NPC->longTermGoal, NPC->pathsize, NPC->wpCurrent, NPC->wpNext);
	*/

	if (!NPC_FindNewWaypoint(aiEnt))
		return; // wait before trying to get a new waypoint...

	// UQ1: Gunner NPCs find cover...
	if (NPC->client->ps.weapon != WP_SABER)
	{// Should we find a cover point???
		if (NPC->enemy->wpCurrent <= 0 || NPC->enemy->wpCurrent < gWPNum)
		{// Find a new waypoint for them...
			NPC->enemy->wpCurrent = DOM_GetNearestWP(NPC->enemy->r.currentOrigin, NPC->enemy->s.number);
		}

		if (NPC->enemy->wpCurrent > 0 
			&& NPC->enemy->wpCurrent < gWPNum
			&& Distance(gWPArray[NPC->enemy->wpCurrent]->origin, NPC->enemy->r.currentOrigin) <= 256)
		{
			/*int i = 0;

			for (i = 0; i < num_cover_spots; i++)
			{
				qboolean BAD = qfalse;

				if (Distance(NPC->r.currentOrigin, gWPArray[cover_nodes[i]]->origin) <= 2048.0f
					&& Distance(NPC->enemy->r.currentOrigin, gWPArray[cover_nodes[i]]->origin) <= 2048.0f)
				{// Range looks good from both places...
					int thisWP = cover_nodes[i];
					
					// OK, looking good so far... Let's see how the visibility is...
					if (NPC_IsCoverpointFor(thisWP, NPC->enemy))
					{// Looks good for a cover point...
						int j = 0;
						int z = 0;

						for (z = 0; z < MAX_GENTITIES; z++)
						{// Now just check to make sure noone else is using it... 30 stormies behind a barrel anyone???
							gentity_t *ent = &g_entities[z];

							if (!ent) continue;
							if (!ent->inuse) continue;

							if (ent->coverpointGoal == thisWP
								|| ent->wpCurrent == thisWP
								|| ent->wpNext == thisWP)
							{// Meh, he already claimed it!
								BAD = qtrue;
								break;
							}
						}

						// Twas a stormie barrel... *sigh*
						if (BAD) continue;

						// So far, so good... Now check if a link from it can see the enemy.. (to dip in and out of cover to/from)
						for (j = 0; j < gWPArray[thisWP]->neighbornum; j++)
						{
							int lookWP = gWPArray[thisWP]->neighbors[j].num;

							if (!NPC_IsCoverpointFor(lookWP, NPC->enemy))
							{// Yes! Found one!
								COVERPOINT_WP = thisWP;
								COVERPOINT_OFC_WP = lookWP;
								IS_COVERPOINT = qtrue;
								break;
							}
						}

						if (IS_COVERPOINT) break; // We got one!
					}
				}

				if (IS_COVERPOINT) break; // We got one!
			}

			if (IS_COVERPOINT)
			{// WooHoo!!!! We got one! *dance*
				NPC->longTermGoal = NPC->coverpointGoal = COVERPOINT_WP;
				NPC->coverpointOFC = COVERPOINT_OFC_WP;
			}*/

			if (NPC->longTermGoal <= 0)
			{// Fallback...
				NPC->longTermGoal = DOM_GetNearestWP(NPC->enemy->r.currentOrigin, NPC->enemy->s.number);
			}
		}
		else
		{// Just head toward them....
			NPC->longTermGoal = DOM_GetNearestWP(NPC->enemy->r.currentOrigin, NPC->enemy->s.number);
		}
	}
	else
	{
		NPC->longTermGoal = DOM_GetNearestWP(NPC->enemy->r.currentOrigin, NPC->enemy->s.number);
	}

	if (NPC->longTermGoal > 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qfalse);

		//if (NPC->pathsize <= 0) // Use the alternate (older) A* pathfinding code as alternative/fallback...
			//NPC->pathsize = DOM_FindIdealPathtoWP(NULL, NPC->wpCurrent, NPC->longTermGoal, -1, NPC->pathlist);
			//NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qtrue);
		
		if (NPC->pathsize > 0)
		{
			/*
			if (NPC->enemy->s.eType == ET_PLAYER)
			{
				if (IS_COVERPOINT)
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint COVERPOINT path between waypoints %i and %i for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->client->pers.netname);
				else
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint path between waypoints %i and %i for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->client->pers.netname);
			}
			else
			{
				if (IS_COVERPOINT)
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint COVERPOINT path between waypoints %i and %i for enemy %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->classname);
				else
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint path between waypoints %i and %i for enemy %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->classname);
			}
			*/

			NPC->wpLast = NPC->wpCurrent;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from

			//G_Printf("New: wps (lt: %i) (ps: %i) (wc: %i) (wn: %i)\n", NPC->longTermGoal, NPC->pathsize, NPC->wpCurrent, NPC->wpNext);

			if (NPC->client->ps.weapon == WP_SABER)
			{
				G_AddVoiceEvent( NPC, Q_irand( EV_JCHASE1, EV_JCHASE3 ), 15000 + irand(0, 30000) );
			}
			else
			{
				int choice = irand(0,13);

				switch (choice)
				{
				case 0:
					G_AddVoiceEvent( NPC, EV_OUTFLANK1, 15000 + irand(0, 30000) );
					break;
				case 1:
					G_AddVoiceEvent( NPC, EV_OUTFLANK2, 15000 + irand(0, 30000) );
					break;
				case 2:
					G_AddVoiceEvent( NPC, EV_CHASE1, 15000 + irand(0, 30000) );
					break;
				case 3:
					G_AddVoiceEvent( NPC, EV_CHASE2, 15000 + irand(0, 30000) );
					break;
				case 4:
					G_AddVoiceEvent( NPC, EV_CHASE3, 15000 + irand(0, 30000) );
					break;
				case 5:
					G_AddVoiceEvent( NPC, EV_COVER1, 15000 + irand(0, 30000) );
					break;
				case 6:
					G_AddVoiceEvent( NPC, EV_COVER2, 15000 + irand(0, 30000) );
					break;
				case 7:
					G_AddVoiceEvent( NPC, EV_COVER3, 15000 + irand(0, 30000) );
					break;
				case 8:
					G_AddVoiceEvent( NPC, EV_COVER4, 15000 + irand(0, 30000) );
					break;
				case 9:
					G_AddVoiceEvent( NPC, EV_COVER5, 15000 + irand(0, 30000) );
					break;
				case 10:
					G_AddVoiceEvent( NPC, EV_ESCAPING1, 15000 + irand(0, 30000) );
					break;
				case 11:
					G_AddVoiceEvent( NPC, EV_ESCAPING2, 15000 + irand(0, 30000) );
					break;
				case 12:
					G_AddVoiceEvent( NPC, EV_ESCAPING3, 15000 + irand(0, 30000) );
					break;
				default:
					G_AddVoiceEvent( NPC, EV_COVER5, 15000 + irand(0, 30000) );
					break;
				}
			}
		}
		else if (NPC->enemy->s.eType == ET_PLAYER)
		{
			//G_Printf("NPC Waypointing Debug: NPC %i (%s) failed to create a route between waypoints %i and %i for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->client->pers.netname);
			NPC->longTermGoal = NPC->coverpointOFC = NPC->coverpointGoal = -1;
			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 2000;
			NPC->last_move_time = level.time;
			return;
		}
	}
	else
	{
		//if (NPC->enemy->s.eType == ET_PLAYER)
		//	G_Printf("NPC Waypointing Debug: NPC %i (%s) failed to find a waypoint for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->enemy->client->pers.netname);

		NPC->longTermGoal = NPC->coverpointOFC = NPC->coverpointGoal = -1;

		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 2000;
		NPC->last_move_time = level.time;
		return;
	}

	// Delay before next route creation...
	NPC->wpSeenTime = level.time + 2000;
	NPC->last_move_time = level.time;
	// Delay before giving up on this new waypoint/route...
	NPC->wpTravelTime = level.time + 15000;
#endif //__USE_NAVMESH__
}

qboolean NPC_CopyPathFromNearbyNPC(gentity_t *aiEnt)
{
#ifndef __USE_NAVMESH__
	gentity_t	*NPC = aiEnt;
	int i = 0;

	for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
	{
		gentity_t *test = &g_entities[i];

		if (i == NPC->s.number) continue;
		if (!test) continue;
		if (!test->inuse) continue;
		if (test->s.eType != ET_NPC) continue;
		if (test->pathsize <= 0) continue;
		if (test->client->NPC_class != NPC->client->NPC_class) continue; // Only copy from same NPC classes???
		if (Distance(NPC->r.currentOrigin, test->r.currentOrigin) > 128) continue;
		if (test->wpCurrent <= 0) continue;
		if (test->longTermGoal <= 0) continue;
		if (test->npc_dumb_route_time > level.time) continue;
		
		// Don't let them be copied again for 2 seconds...
		test->npc_dumb_route_time = level.time + 2000;

		// Seems we found one!
		memcpy(NPC->pathlist, test->pathlist, sizeof(int)*test->pathsize);
		NPC->pathsize = test->pathsize;
		NPC->wpCurrent = test->wpCurrent;
		NPC->wpNext = test->wpNext;
		NPC->wpLast = test->wpLast;
		NPC->longTermGoal = test->longTermGoal;
		
		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 2000;
		// Delay before giving up on this new waypoint/route...
		NPC->wpTravelTime = level.time + 15000;
		NPC->last_move_time = level.time;
		
		// Don't let me be copied for 5 seconds...
		NPC->npc_dumb_route_time = level.time + 5000;
		
		//G_Printf("NPC Waypointing Debug: NPC %i (%s) copied a %i waypoint path between waypoints %i and %i from %i (%s).", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, test->s.number, test->NPC_type);
		return qtrue;
	}
#endif //__USE_NAVMESH__

	return qfalse;
}

int NPC_FindGoal( gentity_t *NPC )
{
#ifdef __USE_NAVLIB__
#if 0
	if (gWPNum > 0)
	{
		int waypoint = irand_big(0, gWPNum - 1);
		int tries = 0;

		while (gWPArray[waypoint]->inuse == false || gWPArray[waypoint]->wpIsBad == true)
		{
			if (tries > 10) return -1; // Try again next frame...

			waypoint = irand_big(0, gWPNum - 1);
			tries++;
		}

#pragma omp critical
		{
			NavlibFindRandomPointInRadius(NPC->s.number, gWPArray[waypoint]->origin, NPC->client->navigation.goal.origin, 99999999.9);
		}
		//trap->Print("[%s] newGoal: %f %f %f.\n", NPC->client->pers.netname, NPC->client->navigation.goal.origin[0], NPC->client->navigation.goal.origin[1], NPC->client->navigation.goal.origin[2]);
		return 1;
	}
	else
	{
#pragma omp critical
		{
			NavlibFindRandomPointOnMesh(NPC, NPC->client->navigation.goal.origin);
		}
		//trap->Print("[%s] newGoal: %f %f %f.\n", NPC->client->pers.netname, NPC->client->navigation.goal.origin[0], NPC->client->navigation.goal.origin[1], NPC->client->navigation.goal.origin[2]);
		return 1;
	}
#else
	//NavlibFindRandomPatrolPoint(NPC->s.number, NPC->client->navigation.goal.origin);
	NavlibFindRandomPointOnMesh(NPC, NPC->client->navigation.goal.origin);
	if (VectorLength(NPC->client->navigation.goal.origin) == 0)
		return -1;

	return 1;
#endif
#endif //__USE_NAVLIB__

#ifndef __USE_NAVMESH__
	int waypoint = irand_big(0, gWPNum-1);
	int tries = 0;

	while (gWPArray[waypoint]->inuse == false || gWPArray[waypoint]->wpIsBad == true)
	{
		if (tries > 10) return -1; // Try again next frame...

		waypoint = irand_big(0, gWPNum-1);
		tries++;
	}

	return waypoint;
#else //!__USE_NAVMESH__
	return -1;
#endif //__USE_NAVMESH__
}

int NPC_FindTeamGoal( gentity_t *NPC )
{
#ifdef __USE_NAVLIB__
#pragma omp critical
	{
		NavlibFindRandomPointOnMesh(NPC, NPC->client->navigation.goal.origin);
	}
	return 1;
#endif //__USE_NAVLIB__

#ifndef __USE_NAVMESH__
	int waypoint = -1;
	int i;
	
	for ( i = 0; i < MAX_GENTITIES; i++)
	{
		gentity_t *ent = &g_entities[i];

		if (!ent) continue;
		if (ent == NPC) continue;
		if (ent->s.eType != ET_NPC && ent->s.eType != ET_PLAYER) continue;
		if (!ent->client) continue;
		if (!NPC_ValidEnemy(NPC, ent)) continue;

		if (ent->s.eType == ET_PLAYER)
		{
			if (ent->wpCurrent < 0 || ent->wpCurrent >= gWPNum
				|| Distance(ent->r.currentOrigin, gWPArray[ent->wpCurrent]->origin) > 128.0)
			{// Their current waypoint is invalid. Find one for them...
				ent->wpCurrent = DOM_GetNearestWP(ent->r.currentOrigin, ent->wpCurrent);
			}
		}

		if (ent->wpCurrent <= 0 || ent->wpCurrent >= gWPNum) continue;

		// Looks ok...
		waypoint = ent->wpCurrent;
		//strcpy(enemy_name, ent->NPC_type);
		break;
	}

	if (waypoint < 0) 
	{// If nothing found then wander...
		waypoint = NPC_FindGoal( NPC );
		//trap->Print("%s failed to find enemy goal.\n", NPC->NPC_type);
	}
	else
	{
		//trap->Print("%s found enemy (%s) goal.\n", NPC->NPC_type, enemy_name);
	}

	return waypoint;
#else //!__USE_NAVMESH__
	return -1;
#endif //__USE_NAVMESH__
}

extern void NPC_SetNewPadawanGoalAndPath(gentity_t *aiEnt);

void NPC_SetNewGoalAndPath(gentity_t *aiEnt)
{
	if (aiEnt->isPadawan)
	{
		NPC_SetNewPadawanGoalAndPath(aiEnt);
		return;
	}

	if (aiEnt->next_pathfind_time > level.time)
	{
		return;
	}

	aiEnt->next_pathfind_time = level.time + 10000 + irand(0, 1000);

#ifdef __USE_NAVLIB__
	if (NPC_FindGoal(aiEnt))
	{
#pragma omp critical
		{
			aiEnt->client->navigation.goal.haveGoal = (qboolean)NavlibFindRouteToTarget(aiEnt, aiEnt->client->navigation.goal, qtrue);
		}

		//if (aiEnt->client->navigation.goal.haveGoal)
		//	trap->Print("%s found a route.\n", aiEnt->client->pers.netname);
		//else
		//	trap->Print("%s failed to find a route.\n", aiEnt->client->pers.netname);
	}
	return;
#endif //__USE_NAVLIB__

#ifndef __USE_NAVMESH__
	gentity_t	*NPC = aiEnt;
	qboolean	padawanPath = qfalse;

	//if (NPC->client->NPC_class != CLASS_TRAVELLING_VENDOR)
	//	if (NPC_CopyPathFromNearbyNPC()) 
	//		return;

	if (NPC_IsJedi(NPC) && (!(NPC->client->ps.fd.forcePowersKnown & (1 << FP_LEVITATION)) || NPC->client->ps.fd.forcePowerLevel[FP_LEVITATION] < 3))
	{
		// Give all Jedi/Sith NPCs jump 3...
		NPC->client->ps.fd.forcePowersKnown |= (1 << FP_LEVITATION);
		NPC->client->ps.fd.forcePowerLevel[FP_LEVITATION] = 3;
	}
	else if (!(NPC->client->ps.fd.forcePowersKnown & (1 << FP_LEVITATION)) || NPC->client->ps.fd.forcePowerLevel[FP_LEVITATION] < 2) 
	{// Give all NPCs jump 2 just for pathing the map and not getting stuck..
		NPC->client->ps.fd.forcePowersKnown |= (1 << FP_LEVITATION);
		NPC->client->ps.fd.forcePowerLevel[FP_LEVITATION] = 2;
	}

	if (NPC->wpSeenTime > level.time)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		return; // wait for next route creation...
	}

	if (!NPC_FindNewWaypoint(aiEnt))
	{
		//trap->Print("Unable to find waypoint.\n");
		//player_die(NPC, NPC, NPC, 99999, MOD_CRUSH);
		return; // wait before trying to get a new waypoint...
	}

	if (NPC->isPadawan)
	{
		if (!NPC->parent || !NPC_IsAlive(NPC, NPC->parent))
		{
			NPC->parent = NULL;
			return; // wait...
		}

		if (NPC->parent && NPC_IsAlive(NPC, NPC->parent))
		{// Need a new path to our master...
			padawanPath = qtrue;
		}
	}

	//
	// First try preferred goal...
	//

	if (NPC->return_home)
	{// Returning home...
		NPC->longTermGoal = DOM_GetNearestWP(NPC->spawn_pos, NPC->wpCurrent);
	}
	else
	{// Find a new generic goal...
		if (g_gametype.integer >= GT_TEAM)
		{
			trap->Cvar_Update(&npc_pathing);

			if (padawanPath) 
				NPC->longTermGoal = NPC_FindPadawanGoal( NPC );
			else if (npc_pathing.integer == 1 && irand(0,5) == 0) // 1 in 6 will head straight to the enemy... When npc_pathing == 2, all NPCs head to random spots...
				NPC->longTermGoal = NPC_FindTeamGoal( NPC );
			else // 5 out of every 6 will use a totally random spot to spread them out... When npc_pathing == 2, all NPCs head to random spots...
				NPC->longTermGoal = NPC_FindGoal( NPC );
		}
		else
		{
			NPC->longTermGoal = NPC_FindGoal( NPC );
		}
	}

	if (NPC->longTermGoal < 0)
	{
		NPC->longTermGoal = NPC_FindGoal( NPC );
	}

	if (NPC->longTermGoal >= 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, sizeof(int)*MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, (qboolean)(irand(0,5) <= 0));

		if (NPC->pathsize > 0)
		{
#ifdef ___AI_PATHING_DEBUG___
			trap->Print("NPC Waypointing Debug: NPC %i created a %i waypoint path for a random goal between waypoints %i and %i.\n", NPC->s.number, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal);
#endif //___AI_PATHING_DEBUG___
			NPC->wpLast = -1;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from

			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 1000;//30000;
			// Delay before giving up on this new waypoint/route...
			NPC->wpTravelTime = level.time + 15000;
			NPC->last_move_time = level.time;
			return;
		}
	}
#endif //__USE_NAVMESH__
}

#ifdef __USE_NAVLIB__
void NPC_SetPadawanGoalAndPath(gentity_t *aiEnt)
{
	if (aiEnt->next_pathfind_time > level.time)
	{
		return;
	}

	aiEnt->next_pathfind_time = level.time + 10000 + irand(0, 1000);

	if (aiEnt->parent && NPC_IsAlive(aiEnt, aiEnt->parent))
	{// Parent is alive, follow route to him, if he is at range...
		if (Distance(aiEnt->r.currentOrigin, aiEnt->parent->r.currentOrigin) > 128.0)
		{
			aiEnt->client->navigation.goal.ent = aiEnt->parent;

#pragma omp critical
			{
				aiEnt->client->navigation.goal.haveGoal = (qboolean)NavlibFindRouteToTarget(aiEnt, aiEnt->client->navigation.goal, qtrue);
			}

			return;
		}
	}
	else
	{// Parent is dead, find a random route...
		NPC_SetNewGoalAndPath(aiEnt);
		return;
	}

	aiEnt->client->navigation.goal.haveGoal = qfalse;
}
#endif //__USE_NAVLIB__

/*
void NPC_SetNewWarzoneGoalAndPath()
{
	gentity_t	*NPC = aiEnt;

	//if (NPC->client->NPC_class == CLASS_TRAVELLING_VENDOR)
	//{
	//	NPC_SetNewGoalAndPath(); // Use normal waypointing...
	//	return;
	//}

	if (NPC->wpSeenTime > level.time)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		return; // wait for next route creation...
	}

	if (!NPC_FindNewWaypoint())
	{
		return; // wait before trying to get a new waypoint...
	}

	// Find a new warzone goal...
	NPC->longTermGoal = NPC_FindWarzoneGoal( NPC );

	if (NPC->longTermGoal <= 0) // Backup - Find a new generic goal...
		NPC->longTermGoal = NPC_FindGoal( NPC );

	if (NPC->longTermGoal >= 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, sizeof(int)*MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, (qboolean)irand(0,1));

		if (NPC->pathsize > 0)
		{
			//G_Printf("NPC Waypointing Debug: NPC %i created a %i waypoint path for a random goal between waypoints %i and %i.", NPC->s.number, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal);
			NPC->wpLast = -1;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from
		}
		else
		{
			//G_Printf("NPC Waypointing Debug: NPC %i failed to create a route between waypoints %i and %i.", NPC->s.number, NPC->wpCurrent, NPC->longTermGoal);
			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 1000;//30000;
			NPC_PickRandomIdleAnimantion(NPC);
			return;
		}
	}
	else
	{
		//G_Printf("NPC Waypointing Debug: NPC %i failed to find a goal waypoint.", NPC->s.number);

		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 1000;//30000;
		NPC_PickRandomIdleAnimantion(NPC);
		return;
	}

	// Delay before next route creation...
	NPC->wpSeenTime = level.time + 1000;//30000;
	// Delay before giving up on this new waypoint/route...
	//NPC->wpTravelTime = level.time + 15000;
}
*/

qboolean DOM_NPC_ClearPathToSpot( gentity_t *NPC, vec3_t dest, int impactEntNum )
{
	trace_t	trace;
	vec3_t	/*start, end, dir,*/ org, destorg;
//	float	dist, drop;
//	float	i;

	//Offset the step height
	//vec3_t	mins = {-10, -10, -6};
	//vec3_t	maxs = {10, 10, 16};

	VectorCopy(NPC->s.origin, org);
	//org[2]+=STEPSIZE;
	org[2]+=16;

	VectorCopy(dest, destorg);
	//destorg[2]+=STEPSIZE;
	destorg[2]+=16;

	trap->Trace( &trace, org, NULL/*mins*/, NULL/*maxs*/, destorg, NPC->s.number, MASK_PLAYERSOLID/*NPC->clipmask*/, 0, 0, 0 );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		//G_Printf("SOLID!\n");
		return qfalse;
	}

	/*
	if ( trace.fraction < 1.0f )
	{//hit something
		if ( (impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum ))
		{//hit what we're going after
			//G_Printf("OK!\n");
			return qtrue;
		}
		else
		{
			//G_Printf("TRACE FAIL! - NPC %i hit entity %i (%s).\n", NPC->s.number, trace.entityNum, g_entities[trace.entityNum].classname);
			return qfalse;
		}
	}
	*/

	if ( trace.fraction < 1.0f )
		return qfalse;

	/*
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
		trap->Trace( &trace, start, mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask );//NPC->r.mins?
		if ( trace.fraction < 1.0f || trace.allsolid || trace.startsolid )
		{//good to go
			continue;
		}
		G_Printf("FLOOR!\n");
		//no floor here! (or a long drop?)
		return qfalse;
	}*/
	//we made it!
	return qtrue;
}

qboolean NPC_PointIsMoverLocation( vec3_t org )
{// Never spawn near a mover location...
	int i = 0;

	for (i = 0; i < MOVER_LIST_NUM; i++)
	{
		if (DistanceHorizontal(org, MOVER_LIST[i]) >= 128.0) continue;

		return qtrue;
	}

	return qfalse;
}

void NPC_ClearPathData ( gentity_t *NPC )
{
#ifdef __USE_NAVLIB__
	memset(&NPC->client->navigation, 0, sizeof(NPC->client->navigation));
#endif //__USE_NAVLIB__

#ifndef __USE_NAVMESH__
	NPC->longTermGoal = -1;
	NPC->wpCurrent = -1;
	NPC->pathsize = -1;
	NPC->longTermGoal = NPC->coverpointOFC = NPC->coverpointGoal = -1;

	//NPC->wpSeenTime = 0;
#endif //__USE_NAVMESH__
}

qboolean NPC_RoutingJumpWaypoint ( int wpLast, int wpCurrent )
{
#ifndef __USE_NAVMESH__
	int			link = 0;
	qboolean	found = qfalse;

	if (wpLast < 0 || wpLast > gWPNum) return qfalse;

	for (link = 0; link < gWPArray[wpLast]->neighbornum; link++)
	{
		if (gWPArray[wpLast]->neighbors[link].num == wpCurrent) 
		{// found it!
			found = qtrue;
			break;
		}
	}

	if (found && gWPArray[wpLast]->neighbors[link].forceJumpTo > 0)
	{
		return qtrue;
	}
#endif //__USE_NAVMESH__

	return qfalse;
}

qboolean NPC_RoutingIncreaseCost ( int wpLast, int wpCurrent )
{
#ifndef __USE_NAVMESH__
	int			link = 0;
	qboolean	found = qfalse;

	if (wpLast < 0 || wpLast > gWPNum) return qfalse;
	if (wpCurrent < 0 || wpCurrent > gWPNum) return qfalse;

	for (link = 0; link < gWPArray[wpLast]->neighbornum; link++)
	{
		if (gWPArray[wpLast]->neighbors[link].num == wpCurrent) 
		{// found it!
			gWPArray[wpLast]->neighbors[link].cost *= 2;

			if (gWPArray[wpLast]->neighbors[link].cost > 32768)
				gWPArray[wpLast]->neighbors[link].cost = 32768;

			//gWPArray[wpLast]->neighbors[link].forceJumpTo = 1;

			if (gWPArray[wpLast]->neighbors[link].cost < 1) 
				gWPArray[wpLast]->neighbors[link].cost = 2;

			return qtrue;
		}
	}
#endif //__USE_NAVMESH__

	return qfalse;
}

int CheckForFuncAbove(vec3_t org, int ignore)
{
	gentity_t *fent;
	vec3_t under, org2;
	trace_t tr;

	VectorCopy(org, org2);
	org2[2]+=16.0;

	VectorCopy(org, under);

	under[2] += 16550;

	trap->Trace(&tr, org2, NULL, NULL, under, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	fent = &g_entities[tr.entityNum];

	if (!fent)
	{
		return 0;
	}

	if (strstr(fent->classname, "func_"))
	{
		if (tr.allsolid || Distance(tr.endpos, org) < 128) return 0; // Not above us... It's a door!

		return 1; //there's a func brush here
	}

	return 0;
}

//see if there's a func_* ent under the given pos.
//kind of badly done, but this shouldn't happen
//often.
int CheckForFunc(vec3_t org, int ignore)
{
	gentity_t *fent;
	vec3_t under;
	trace_t tr;

	VectorCopy(org, under);

	under[2] -= 64;

	trap->Trace(&tr, org, NULL, NULL, under, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	fent = &g_entities[tr.entityNum];

	if (!fent)
	{
		return 0;
	}

	if (strstr(fent->classname, "func_"))
	{
		return fent->s.number; //there's a func brush here
	}

	return 0;
}

int WaitingForNow(gentity_t *aiEnt, vec3_t goalpos)
{ //checks if the bot is doing something along the lines of waiting for an elevator to raise up
#ifndef __USE_NAVMESH__
	vec3_t		xybot, xywp, a;
#if 0
	vec3_t		goalpos2;
	qboolean	have_goalpos2 = qfalse;
#endif

	if (aiEnt->wpCurrent < 0 || aiEnt->wpCurrent >= gWPNum)
	{
		return 0;
	}

#if 0
	if (aiEnt->wpNext >= 0 && aiEnt->wpNext < gWPNum)
	{
		VectorCopy(gWPArray[aiEnt->wpNext]->origin, goalpos2);
		have_goalpos2 = qtrue;
	}

	if ((int)goalpos[0] != (int)gWPArray[aiEnt->wpCurrent]->origin[0] ||
		(int)goalpos[1] != (int)gWPArray[aiEnt->wpCurrent]->origin[1] ||
		(int)goalpos[2] != (int)gWPArray[aiEnt->wpCurrent]->origin[2])
	{
		return 0;
	}

	if (CheckForFuncAbove(goalpos, aiEnt->s.number) > 0 || (have_goalpos2 && CheckForFuncAbove(goalpos2, aiEnt->s.number) > 0))
	{// Squisher above alert!
		return 1;
	}
#endif

	VectorCopy(aiEnt->r.currentOrigin, xybot);
	VectorCopy(gWPArray[aiEnt->wpCurrent]->origin, xywp);

	xybot[2] = 0;
	xywp[2] = 0;

	VectorSubtract(xybot, xywp, a);

	if (VectorLength(a) < 16)
	{
		if (CheckForFunc(aiEnt->r.currentOrigin, aiEnt->s.number) > 0)
		{
			return 1; //we're probably standing on an elevator and riding up/down. Or at least we hope so.
		}
	}
	else if (VectorLength(a) < 64 && CheckForFunc(aiEnt->r.currentOrigin, aiEnt->s.number))
	{
		aiEnt->useDebounceTime = level.time + 2000;
	}
#endif //__USE_NAVMESH__

	return 0;
}

qboolean NPC_MoverCrushCheck ( gentity_t *NPC )
{
#if 0
	int above = CheckForFuncAbove(NPC->r.currentOrigin, NPC->s.number);

	if (above > 0)
	{
		gentity_t *ABOVE_ENT = &g_entities[above];

		if (!ABOVE_ENT) return qfalse;

		// Looks like there is a mover above us... Step back!
		NPC_FacePosition( NPC, ABOVE_ENT->r.currentOrigin, qfalse );

		aiEnt->NPC->goalEntity = ABOVE_ENT;

		if ( UpdateGoal() )
		{// Retreat until we are off of them...
			NPC_CombatMoveToGoal( qtrue, qtrue );
		}
		else
		{// Fallback...
			NPC->client->pers.cmd.forwardmove = -127.0;
		}

		return qtrue;
	}
#endif //0

	return qfalse;
}

qboolean NPC_GetOffPlayer ( gentity_t *NPC )
{
	if (NPC->client->ps.groundEntityNum < ENTITYNUM_MAX_NORMAL)
	{// We are on some entity...
		gentity_t *on = &g_entities[NPC->client->ps.groundEntityNum];

		if (!on) return qfalse;
		if (on->s.eType != ET_NPC && on->s.eType != ET_PLAYER) return qfalse;
		if (!on->client) return qfalse;

		// Looks like we are on a player or NPC... Get off of them...
		NPC_FacePosition(NPC, on->r.currentOrigin, qfalse );

		NPC->NPC->goalEntity = on;

		if ( UpdateGoal(NPC) )
		{// Retreat until we are off of them...
			NPC_CombatMoveToGoal( NPC, qtrue, qtrue );
		}
		else
		{// Fallback...
			NPC->client->pers.cmd.forwardmove = -127.0;
		}

		return qtrue;
	}

	return qfalse;
}

qboolean NPC_HaveValidEnemy(gentity_t *aiEnt)
{
	gentity_t	*NPC = aiEnt;
	
	if (NPC->enemy)
	{
		if (NPC_IsAlive(NPC, NPC->enemy))
		{
			if (NPC->enemy == NPC->enemy->padawan)
			{
				NPC->enemy = NULL;
				return qfalse;
			}
			else if (NPC->enemy == NPC->enemy->parent)
			{
				NPC->enemy = NULL;
				return qfalse;
			}
			if (OnSameTeam(NPC, NPC->enemy))
			{
				return qfalse;
			}

			return qtrue;
		}
	}
	
	return qfalse;
}

void NPC_NewWaypointJump (gentity_t *aiEnt)
{// Jumping to new waypoint...
#ifndef __USE_NAVMESH__
	vec3_t myOrg, wpOrg;
	qboolean should_jump = qtrue;

	VectorCopy(aiEnt->r.currentOrigin, myOrg);
	myOrg[2]+= 8;

	if (aiEnt->wpCurrent < 0 || aiEnt->wpCurrent >= gWPNum)
	{
		return;
	}

	VectorCopy(gWPArray[aiEnt->wpCurrent]->origin, wpOrg);
	//wpOrg[2]+= 8;

	if (!aiEnt->npc_jumping && aiEnt->wpLast >= 0 && aiEnt->wpLast < gWPNum)
	{// Have a wpLast... We are mid route...
		if (OrgVisible(myOrg, wpOrg, aiEnt->s.number))
			return;
	}
	else if (!aiEnt->npc_jumping && wpOrg[2] <= myOrg[2]+8)
	{// No wpLast, we are on a new route, but this waypoint is walkable...
		if (OrgVisible(myOrg, wpOrg, aiEnt->s.number))
			return; // No need to jump to this...
	}

	if (aiEnt->enemy && NPC_IsAlive(aiEnt, aiEnt->enemy))
	{// Don't jump when we are chasing an enemy, and they are not above me...
		should_jump = NPC_EnemyAboveMe(aiEnt);
	}

	if (should_jump && NPC_Jump( aiEnt, wpOrg ))
	{// Continue the jump...
		//trap->Print("NPC JUMP DEBUG: NPC_NewWaypointJump\n");
		return;
	}
#endif //__USE_NAVMESH__
}

qboolean NPC_DoLiftPathing(gentity_t *NPC)
{
	gentity_t *aiEnt = NPC;
#ifndef __USE_NAVMESH__
	if (NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum)
	{
		qboolean onMover1 = (qboolean)WaitingForNow(aiEnt, gWPArray[NPC->wpCurrent]->origin);
		qboolean onMover2 = NPC_PointIsMoverLocation(gWPArray[NPC->wpCurrent]->origin);

		if ((onMover1 || onMover2) 
			&& DistanceVertical(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) < 128)
		{
			if (DistanceHorizontal(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) < DistanceVertical(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin))
			{// Most likely on an elevator... Allow hitting waypoints all the way up/down...
				while (NPC->wpCurrent >= 0 
					&& NPC->wpCurrent < gWPNum
					&& (onMover1 || onMover2)
					&& DistanceVertical(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) < 128)
				{
					NPC->wpLast = NPC->wpCurrent;
					NPC->wpCurrent = NPC->wpNext;
					NPC->wpNext = NPC_GetNextNode(NPC);
					
					NPC->wpTravelTime = level.time + 15000;
					NPC->wpSeenTime = level.time;

					if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum)
						break;

					onMover1 = (qboolean)WaitingForNow(aiEnt, gWPArray[NPC->wpCurrent]->origin);
					onMover2 = NPC_PointIsMoverLocation(gWPArray[NPC->wpCurrent]->origin);
				}

				if (NPC->wpCurrent >= 0 
					&& NPC->wpCurrent < gWPNum
					&& gWPArray[NPC->wpCurrent]->origin[2] >= NPC->r.currentOrigin[2])
				{// Next waypoint is above us... Jump to it if possible...
					NPC_FacePosition(aiEnt, gWPArray[NPC->wpCurrent]->origin, qfalse);

					if ((NPC_IsJedi(NPC) || NPC_IsBountyHunter(NPC)) 
						&& NPC_Jump(NPC, gWPArray[NPC->wpCurrent]->origin))
					{
						//trap->Print("NPC JUMP DEBUG: NPC_DoLiftPathing\n");
						VectorCopy( NPC->movedir, NPC->client->ps.moveDir );
						return qtrue;
					}
				}

				// Idle...
				aiEnt->client->pers.cmd.forwardmove = 0;
				aiEnt->client->pers.cmd.rightmove = 0;
				aiEnt->client->pers.cmd.upmove = 0;
				NPC_PickRandomIdleAnimantion(NPC);

				return qtrue;
			}
			else 
			{
				if (NPC->wpCurrent >= 0 
					&& NPC->wpCurrent < gWPNum
					&& gWPArray[NPC->wpCurrent]->origin[2] >= NPC->r.currentOrigin[2])
				{// Next waypoint is above us... Jump to it if possible...
					NPC_FacePosition(aiEnt, gWPArray[NPC->wpCurrent]->origin, qfalse);

					if ((NPC_IsJedi(NPC) || NPC_IsBountyHunter(NPC)) 
						&& NPC_Jump(NPC, gWPArray[NPC->wpCurrent]->origin))
					{
						//trap->Print("NPC JUMP DEBUG: NPC_DoLiftPathing\n");
						VectorCopy( NPC->movedir, NPC->client->ps.moveDir );
						return qtrue;
					}
				}

				// If waypoing is below us, we can simply walk off...
				UQ1_UcmdMoveForDir_NoAvoidance( NPC, &aiEnt->client->pers.cmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin );
				return qtrue;
			}
		}
	}
#endif //__USE_NAVMESH__

	return qfalse;
}

qboolean NPC_FollowRoutes(gentity_t *aiEnt)
{// Quick method of following bot routes...
	gentity_t	*NPC = aiEnt;
	usercmd_t	*ucmd = &aiEnt->client->pers.cmd;
	float		wpDist = 0.0;
	qboolean	padawanPath = qfalse;
	qboolean	onMover1 = qfalse;
	qboolean	onMover2 = qfalse;

	aiEnt->NPC->combatMove = qtrue;

	if ( !NPC_HaveValidEnemy(aiEnt) )
	{
		switch (NPC->client->NPC_class)
		{
		case CLASS_CIVILIAN:
		case CLASS_CIVILIAN_R2D2:
		case CLASS_CIVILIAN_R5D2:
		case CLASS_CIVILIAN_PROTOCOL:
		case CLASS_CIVILIAN_WEEQUAY:
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
			NPC->r.contents = 0;
			NPC->clipmask = MASK_NPCSOLID&~CONTENTS_BODY;
			// These guys have no enemies...
			break;
		default:
			break;
		}
	}

	G_ClearEnemy(NPC);

	if (NPC_GetOffPlayer(NPC))
	{// Get off of their head!
		return qtrue;
	}

	if (NPC_MoverCrushCheck(NPC))
	{// There is a mover gonna crush us... Step back...
		return qtrue;
	}

	ucmd->forwardmove = 0;
	ucmd->rightmove = 0;
	ucmd->upmove = 0;

#ifdef __USE_NAVLIB__
	if (G_NavmeshIsLoaded())
	{
		qboolean walk = qfalse;

		if (DistanceHorizontal(NPC->r.currentOrigin, NPC->npc_previous_pos) > 3)
		{
			NPC->last_move_time = level.time;
			VectorCopy(NPC->r.currentOrigin, NPC->npc_previous_pos);
		}

#ifndef __USE_NAVLIB__
		if (NPC->isPadawan)
		{
			if (NPC->nextPadawanWaypointThink < level.time)
			{
				NavlibSetNavMesh(NPC->s.number, 0);
				NPC_SetPadawanGoalAndPath(NPC);

				if ((!NPC->parent || NPC_IsAlive(NPC, NPC->parent))
					&& NPC->client->navigation.goal.haveGoal
					&& NPC->client->navigation.goal.ent != NPC->parent)
				{// We have a route, but it not our parent (he/she is dead), go there using normal code below...

				}
				else if (NPC->client->navigation.goal.haveGoal
					&& NPC->client->navigation.goal.ent == NPC->parent
					&& !GoalInRange(NPC, NavlibGetGoalRadius(NPC)))
				{// Have a goal to our parent... Go there...
					NavlibSetNavMesh(NPC->s.number, 0);
#pragma omp critical
					{
						NavlibMoveToGoal(NPC);
					}
					NPC_FacePosition(NPC, NPC->client->navigation.nav.lookPos, qfalse);
					VectorSubtract(NPC->client->navigation.nav.pos, NPC->r.currentOrigin, NPC->movedir);

#ifndef __USE_NAVLIB_INTERNAL_MOVEMENT__
					if (Distance(NPC->r.currentOrigin, NPC->client->navigation.goal.ent->r.currentOrigin) < 256)
					{
						walk = qtrue;
					}

					if (UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, walk, NPC->client->navigation.nav.pos))
					{
						if (NPC->last_move_time < level.time - 2000)
						{
							ucmd->upmove = 127;

							if (NPC->s.eType == ET_PLAYER)
							{
								trap->EA_Jump(NPC->s.number);
							}
						}

						return qtrue;
					}
					else if (NPC->bot_strafe_jump_timer > level.time)
					{
						ucmd->upmove = 127;

						if (NPC->s.eType == ET_PLAYER)
						{
							trap->EA_Jump(NPC->s.number);
						}
					}
					else if (NPC->bot_strafe_left_timer > level.time)
					{
						ucmd->rightmove = -127;
						trap->EA_MoveLeft(NPC->s.number);
					}
					else if (NPC->bot_strafe_right_timer > level.time)
					{
						ucmd->rightmove = 127;
						trap->EA_MoveRight(NPC->s.number);
					}

					if (NPC->last_move_time < level.time - 4000)
					{
						NPC->client->navigation.goal.haveGoal = qfalse;
						VectorClear(NPC->client->navigation.goal.origin);
						NPC->client->navigation.goal.ent = NULL;
						return qfalse;
					}

					if (NPC->last_move_time < level.time - 2000)
					{
						ucmd->upmove = 127;

						if (NPC->s.eType == ET_PLAYER)
						{
							trap->EA_Jump(NPC->s.number);
						}
					}
#endif //__USE_NAVLIB_INTERNAL_MOVEMENT__
					return qtrue;
				}
				else
				{// Either hit our goal, or we don't have a route to our parent...
					if (NPC->client->navigation.goal.haveGoal
						&& NPC->client->navigation.goal.ent == NPC->parent
						&& GoalInRange(NPC, NavlibGetGoalRadius(NPC)))
					{// Hit our parent goal... Wait...
						NPC->client->navigation.goal.haveGoal = qfalse;
						VectorClear(NPC->client->navigation.goal.origin);
						NPC->client->navigation.goal.ent = NULL;
						//trap->Print("[%s] hit padawan goal!\n", NPC->client->pers.netname);
						return qfalse;
					}

					return qfalse;
				}
			}
		}
#endif //__USE_NAVLIB__

		if (NPC->client->navigation.goal.haveGoal && !GoalInRange(NPC, NavlibGetGoalRadius(NPC)))
		{
			if (NPC->last_move_time < level.time - 4000)
			{
				NPC->client->navigation.goal.haveGoal = qfalse;
				VectorClear(NPC->client->navigation.goal.origin);
				NPC->client->navigation.goal.ent = NULL;
				NPC_SetNewGoalAndPath(NPC);
			}

			NavlibSetNavMesh(NPC->s.number, 0);
#pragma omp critical
			{
				NavlibMoveToGoal(NPC);
			}
			NPC_FacePosition(NPC, NPC->client->navigation.nav.lookPos, qfalse);
			VectorSubtract(NPC->client->navigation.nav.lookPos, NPC->r.currentOrigin, NPC->movedir);
			
			if (NPC->isPadawan || NPC->s.NPC_class == CLASS_PADAWAN || NPC->s.NPC_class == CLASS_HK51)
			{
				//trap->Print("Padawan %s is a padawan.\n", NPC->client->pers.netname);

				if (NPC->parent && NPC_IsAlive(NPC, NPC->parent))
				{
					Padawan_CheckForce(aiEnt);

					//trap->Print("Padawan %s parent is alive.\n", NPC->client->pers.netname);
					//trap->Print("Padawan %s has FP_SPEED level %i.\n", NPC->client->pers.netname, NPC->client->ps.fd.forcePowerLevel[FP_SPEED]);
					//trap->Print("Padawan %s has FP_PROTECT level %i.\n", NPC->client->pers.netname, NPC->client->ps.fd.forcePowerLevel[FP_PROTECT]);

					if (NPC->client->navigation.goal.ent == NPC->parent)
					{// Catch-up stuff for followers, teleport to master or use force speed/protect to get there faster and safer...
						float dist = Distance(NPC->parent->r.currentOrigin, NPC->r.currentOrigin);

						//trap->Print("Padawan %s dist to master is %f.\n", NPC->client->pers.netname, dist);
						//trap->Print("Padawan %s dist to master is %f. NPC->parent->s.groundEntityNum %i. NPC->parent->client->ps.groundEntityNum %i.\n", NPC->client->pers.netname, dist, NPC->parent->s.groundEntityNum, NPC->parent->client->ps.groundEntityNum);

						if (NPC->parent->s.groundEntityNum != ENTITYNUM_NONE
							&& NPC->parent->client->ps.groundEntityNum != ENTITYNUM_NONE
							&& NPC->nextPadawanTeleportThink <= level.time
							&& dist > 4096.0)
						{// Padawan is too far from jedi. Teleport to him... Only if they are not in mid air...
							vec3_t position;
#pragma omp critical
							{
								NavlibFindRandomPointInRadius(-1, NPC->parent->r.currentOrigin, position, 2048.0);
							}

							position[2] += 32.0;

							NPC->nextPadawanTeleportThink = level.time + 5000;

							TeleportNPC(NPC, position, NPC->s.angles);

							NPC_ClearGoal(aiEnt);
							aiEnt->NPC->goalEntity = NULL;
							aiEnt->NPC->tempGoal = NULL;

							ucmd->forwardmove = 0;
							ucmd->rightmove = 0;
							ucmd->upmove = 0;
							NPC_PickRandomIdleAnimantion(NPC);

							//trap->Print("Padawan %s teleported to master.\n", NPC->client->pers.netname);

							return qtrue;
						}
						else if (aiEnt->client->NPC_class == CLASS_PADAWAN)
						{
							extern void WP_ForcePowerStart(gentity_t *self, forcePowers_t forcePower, int overrideAmt);

							if (dist > 1024.0
								//&& TIMER_Done(NPC, "speed")
								&& NPC->client->ps.fd.forcePowerLevel[FP_SPEED] > 0
								&& !(NPC->client->ps.fd.forcePowersActive & (1 << FP_SPEED)))
							{// When the master is a long way away, use force speed to get to him faster and safer...
								NPC->client->ps.forceAllowDeactivateTime = level.time + 1500;

								WP_ForcePowerStart(NPC, FP_SPEED, 0);
								G_Sound(NPC, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav"));
								G_Sound(NPC, TRACK_CHANNEL_2, G_SoundIndex("sound/weapons/force/speedloop.wav"));
								//TIMER_Set(NPC, "speed", irand(15000, 20000));

								//trap->Print("Padawan %s used force speed.\n", NPC->client->pers.netname);
							}
							else if (dist > 512.0
								//&& NPC->health < NPC->maxHealth * 0.5
								//&& TIMER_Done(NPC, "protect")
								&& NPC->client->ps.fd.forcePowerLevel[FP_PROTECT] > 0
								&& !(NPC->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)))
							{// When the master is a fair way away, use force protect to get to him safer...
								NPC->client->ps.forceAllowDeactivateTime = level.time + 1500;

								WP_ForcePowerStart(NPC, FP_PROTECT, 0);
								G_PreDefSound(NPC->client->ps.origin, PDSOUND_PROTECT);
								G_Sound(NPC, TRACK_CHANNEL_3, G_SoundIndex("sound/weapons/force/protectloop.wav"));
								//TIMER_Set(NPC, "protect", irand(15000, 20000));

								//trap->Print("Padawan %s used force protect.\n", NPC->client->pers.netname);
							}
							else if (NPC->client->ps.fd.forcePowerLevel[FP_HEAL] > 0
								&& NPC->health < NPC->maxHealth * 0.75)
							{// Use heal when health is down, if we have it...
								ForceHeal(NPC);
								//trap->Print("Padawan %s used force heal.\n", NPC->client->pers.netname);
							}
						}
						else if (aiEnt->client->NPC_class == CLASS_HK51)
						{
							extern void WP_ForcePowerStart(gentity_t *self, forcePowers_t forcePower, int overrideAmt);

							if (dist > 1024.0
								//&& TIMER_Done(NPC, "speed")
								&& NPC->client->ps.fd.forcePowerLevel[FP_SPEED] > 0
								&& !(NPC->client->ps.fd.forcePowersActive & (1 << FP_SPEED)))
							{// When the master is a long way away, use force speed to get to him faster and safer...
								NPC->client->ps.forceAllowDeactivateTime = level.time + 1500;

								WP_ForcePowerStart(NPC, FP_SPEED, 0);
								G_Sound(NPC, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav"));
								G_Sound(NPC, TRACK_CHANNEL_2, G_SoundIndex("sound/weapons/force/speedloop.wav"));
								//TIMER_Set(NPC, "speed", irand(15000, 20000));
							}
						}
					}
				}
			}

#ifndef __USE_NAVLIB_INTERNAL_MOVEMENT__
			if (Distance(NPC->r.currentOrigin, NPC->client->navigation.goal.origin) < 256)
			{
				walk = qtrue;
			}

			if (UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, walk, NPC->client->navigation.nav.lookPos))
			{
				if (NPC->last_move_time < level.time - 2000 || DistanceVertical(NPC->client->navigation.nav.pos, NPC->r.currentOrigin) > DistanceHorizontal(NPC->client->navigation.nav.pos, NPC->r.currentOrigin) * 0.666)
				{
					ucmd->upmove = 127;

					if (NPC->s.eType == ET_PLAYER)
					{
						trap->EA_Jump(NPC->s.number);
					}
				}

				return qtrue;
			}
			else if (NPC->bot_strafe_jump_timer > level.time)
			{
				ucmd->upmove = 127;

				if (NPC->s.eType == ET_PLAYER)
				{
					trap->EA_Jump(NPC->s.number);
				}
			}
			else if (NPC->bot_strafe_left_timer > level.time)
			{
				ucmd->rightmove = -127;
				trap->EA_MoveLeft(NPC->s.number);
			}
			else if (NPC->bot_strafe_right_timer > level.time)
			{
				ucmd->rightmove = 127;
				trap->EA_MoveRight(NPC->s.number);
			}

			if (NPC->last_move_time < level.time - 2000 || DistanceVertical(NPC->client->navigation.nav.pos, NPC->r.currentOrigin) > DistanceHorizontal(NPC->client->navigation.nav.pos, NPC->r.currentOrigin) * 0.666)
			{
				ucmd->upmove = 127;

				if (NPC->s.eType == ET_PLAYER)
				{
					trap->EA_Jump(NPC->s.number);
				}
			}
#endif //__USE_NAVLIB_INTERNAL_MOVEMENT__
			return qtrue;
		}
		else
		{// Need a new goal...
			NavlibSetNavMesh(NPC->s.number, 0);

			if (NPC->client->navigation.goal.haveGoal && GoalInRange(NPC, NavlibGetGoalRadius(NPC)))
			{
				NPC_ClearGoal(NPC);
				//trap->Print("[%s] hit goal!\n", NPC->client->pers.netname);
			}

			NPC_SetNewGoalAndPath(NPC);

			if (NPC->client->navigation.goal.haveGoal)
			{
#pragma omp critical
				{
					NavlibMoveToGoal(NPC);
				}
				NPC_FacePosition(NPC, NPC->client->navigation.nav.lookPos, qfalse);
				VectorSubtract(NPC->client->navigation.nav.lookPos, NPC->r.currentOrigin, NPC->movedir);

#ifndef __USE_NAVLIB_INTERNAL_MOVEMENT__
				if (Distance(NPC->r.currentOrigin, NPC->client->navigation.goal.origin) < 256)
				{
					walk = qtrue;
				}

				if (UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, walk, NPC->client->navigation.nav.lookPos))
				{
					return qtrue;
				}
				else if (NPC->bot_strafe_jump_timer > level.time)
				{
					ucmd->upmove = 127;

					if (NPC->s.eType == ET_PLAYER)
					{
						trap->EA_Jump(NPC->s.number);
					}
				}
				else if (NPC->bot_strafe_left_timer > level.time)
				{
					ucmd->rightmove = -127;
					trap->EA_MoveLeft(NPC->s.number);
				}
				else if (NPC->bot_strafe_right_timer > level.time)
				{
					ucmd->rightmove = 127;
					trap->EA_MoveRight(NPC->s.number);
				}
#endif //__USE_NAVLIB_INTERNAL_MOVEMENT__
				return qtrue;
			}
			else if (NPC->bot_strafe_jump_timer > level.time)
			{
				ucmd->upmove = 127;

				if (NPC->s.eType == ET_PLAYER)
				{
					trap->EA_Jump(NPC->s.number);
				}
			}
			else if (NPC->bot_strafe_left_timer > level.time)
			{
				ucmd->rightmove = -127;
				trap->EA_MoveLeft(NPC->s.number);
			}
			else if (NPC->bot_strafe_right_timer > level.time)
			{
				ucmd->rightmove = 127;
				trap->EA_MoveRight(NPC->s.number);
			}

			return qtrue;
		}

		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse;
	}
#endif //__USE_NAVLIB__

#ifndef __USE_NAVMESH__
	if (NPC->isPadawan)
	{
		if (NPC->nextPadawanWaypointThink < level.time)
		{
			if (NPC->parent
				&& (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum 
				|| NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum
				|| Distance(NPC->parent->r.currentOrigin, gWPArray[NPC->longTermGoal]->origin) > 128))
			{// Need a new path to our master...
				padawanPath = qtrue;
			}
			else if (NPC->parent 
				&& NPC->parent->wpCurrent >= 0 && NPC->parent->wpCurrent < gWPNum 
				&& Distance(NPC->parent->r.currentOrigin, gWPArray[NPC->parent->wpCurrent]->origin) > 128)
			{// Need a new path to our master...
				padawanPath = qtrue;
			}

			NPC->nextPadawanWaypointThink = level.time + 5000; // only look for a new route every 5 seconds..
		}
	}

	if ( padawanPath
		|| NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum 
		|| NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum 
		|| NPC->wpSeenTime < level.time - 5000
		|| NPC->wpTravelTime < level.time 
		|| NPC->last_move_time < level.time - 15000 )
	{// We hit a problem in route, or don't have one yet.. Find a new goal and path...
#ifdef ___AI_PATHING_DEBUG___
		if (NPC->wpSeenTime < level.time - 5000) trap->Print("PATHING DEBUG: %i wpSeenTime.\n", NPC->s.number);
		if (NPC->wpTravelTime < level.time) trap->Print("PATHING DEBUG: %i wpTravelTime.\n", NPC->s.number);
		if (NPC->last_move_time < level.time - 5000) trap->Print("PATHING DEBUG: %i last_move_time.\n", NPC->s.number);
		if ((NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum) && (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum)) trap->Print("PATHING DEBUG: %i wpCurrent & longTermGoal.\n", NPC->s.number);
		else if (NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum) trap->Print("PATHING DEBUG: %i longTermGoal.\n", NPC->s.number);
		else if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum) trap->Print("PATHING DEBUG: %i wpCurrent.\n", NPC->s.number);
#endif //___AI_PATHING_DEBUG___

		if (!padawanPath)
		{
			NPC_RoutingIncreaseCost( NPC->wpLast, NPC->wpCurrent );
		}

		NPC_ClearPathData(NPC);
		NPC_SetNewGoalAndPath(aiEnt);
	}

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
	{// FIXME: Try to roam out of problems...
#ifdef ___AI_PATHING_DEBUG___
		trap->Print("PATHING DEBUG: NO PATH!\n");
#endif //___AI_PATHING_DEBUG___
		NPC_ClearPathData(NPC);
		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse; // next think...
	}
	
	if (Distance(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 48
		|| (NPC->s.groundEntityNum != ENTITYNUM_NONE && DistanceHorizontal(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 48))
	{// We're at our goal! Find a new goal...
#ifdef ___AI_PATHING_DEBUG___
		trap->Print("PATHING DEBUG: HIT GOAL!\n");
#endif //___AI_PATHING_DEBUG___
		NPC_ClearPathData(NPC);
		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse; // next think...
	}

	if (DistanceHorizontal(NPC->r.currentOrigin, NPC->npc_previous_pos) > 3)
	{
		NPC->last_move_time = level.time;
		VectorCopy(NPC->r.currentOrigin, NPC->npc_previous_pos);
	}

	if ( NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum )
	{
		vec3_t upOrg, upOrg2;

		VectorCopy(NPC->r.currentOrigin, upOrg);
		upOrg[2]+=18;

		VectorCopy(gWPArray[NPC->wpCurrent]->origin, upOrg2);
		upOrg2[2]+=18;

		if (NPC->wpSeenTime <= level.time)
		{
			if (OrgVisible(upOrg, upOrg2, NPC->s.number))
			{
				NPC->wpSeenTime = level.time;
			}
		}

		if (NPC_IsJetpacking(NPC))
		{// Jetpacking... Ignore heights...
			wpDist = DistanceHorizontal(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);
		}
		else if (NPC->s.groundEntityNum != ENTITYNUM_NONE)
		{// On somebody's head or in the air...
			wpDist = DistanceHorizontal(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);
		}
		else
		{// On ground...
			wpDist = DistanceHorizontal(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);//Distance(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);
		}

		//if (wpDist > 512) trap->Print("To far! (%f)\n", wpDist);
	}

	if (wpDist < 48)
	{// At current node.. Pick next in the list...
		//trap->Print("HIT WP %i. Next WP is %i.\n", NPC->wpCurrent, NPC->wpNext);

		NPC->wpLast = NPC->wpCurrent;
		NPC->wpCurrent = NPC->wpNext;
		NPC->wpNext = NPC_GetNextNode(NPC);

		if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
		{// FIXME: Try to roam out of problems...
#ifdef ___AI_PATHING_DEBUG___
			trap->Print("PATHING DEBUG: HIT ROUTE END!\n");
#endif //___AI_PATHING_DEBUG___
			NPC_ClearPathData(NPC);
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;
			NPC_PickRandomIdleAnimantion(NPC);
			return qfalse; // next think...
		}

		NPC->wpTravelTime = level.time + 15000;
		NPC->wpSeenTime = level.time;
	}

	NPC_FacePosition(aiEnt, gWPArray[NPC->wpCurrent]->origin, qfalse );
	NPC->s.angles[PITCH] = NPC->client->ps.viewangles[PITCH] = 0; // Init view PITCH angle so we always look forward, not down or up...
	VectorSubtract( gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir );
	
	if (NPC_DoLiftPathing(NPC))
	{
		return qtrue;
	}

	if (VectorLength(NPC->client->ps.velocity) < 8 && NPC_RoutingJumpWaypoint( NPC->wpLast, NPC->wpCurrent ))
	{// We need to jump to get to this waypoint...
		if (NPC_Jump(NPC, gWPArray[NPC->wpCurrent]->origin))
		{
			//trap->Print("NPC JUMP DEBUG: NPC_FollowRoutes\n");
			VectorCopy( NPC->movedir, NPC->client->ps.moveDir );
			return qtrue;
		}
	}
	else if (VectorLength(NPC->client->ps.velocity) < 8)
	{// If this is a new waypoint, we may need to jump to it...
		NPC_NewWaypointJump(aiEnt);
	}

	if (NPC_IsCivilian(NPC))
	{
		if (NPC->npc_cower_runaway)
		{// A civilian running away from combat...
			if (!UQ1_UcmdMoveForDir( NPC, &aiEnt->client->pers.cmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin )) 
			{
				if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
					NPC_PickRandomIdleAnimantion(NPC);
				else
					NPC_SelectMoveRunAwayAnimation(NPC);

				return qtrue;
			}

			if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
				NPC_PickRandomIdleAnimantion(NPC);
			else
				NPC_SelectMoveRunAwayAnimation(NPC);

			VectorCopy( NPC->movedir, NPC->client->ps.moveDir );

			return qtrue;
		}
		else if (NPC_IsCivilianHumanoid(NPC))
		{// Civilian humanoid... Force walk/run anims...
			if (NPC_PointIsMoverLocation(gWPArray[NPC->wpCurrent]->origin))
			{// When nearby a mover, run!
				if (!UQ1_UcmdMoveForDir( NPC, ucmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin )) 
				{ 
					if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
						NPC_PickRandomIdleAnimantion(NPC);
					else
						NPC_SelectMoveAnimation(aiEnt, qfalse);

					return qtrue; 
				}

				if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
					NPC_PickRandomIdleAnimantion(NPC);
				else
					NPC_SelectMoveAnimation(aiEnt, qtrue); // UQ1: Always set civilian walk animation...
			}
			else if (!UQ1_UcmdMoveForDir( NPC, ucmd, NPC->movedir, qtrue, gWPArray[NPC->wpCurrent]->origin ))
			{
				if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
					NPC_PickRandomIdleAnimantion(NPC);
				else
					NPC_SelectMoveAnimation(aiEnt, qtrue);

				return qtrue;
			}

			if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
				NPC_PickRandomIdleAnimantion(NPC);
			else
				NPC_SelectMoveAnimation(aiEnt, qtrue);
		}
		else
		{// Civilian non-humanoid... let bg_ set anim...
			return qtrue;
		}
	}
	else if (g_gametype.integer == GT_WARZONE || (NPC->r.svFlags & SVF_BOT))
	{
		if (!UQ1_UcmdMoveForDir( NPC, ucmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin ))
		{ 
			return qtrue; 
		}
	}
	else 
	{
		qboolean walk = qtrue;

		if (NPC_HaveValidEnemy(aiEnt)) walk = qfalse;

		if (!UQ1_UcmdMoveForDir( NPC, ucmd, NPC->movedir, walk, gWPArray[NPC->wpCurrent]->origin ))
		{
			if (NPC->bot_strafe_jump_timer > level.time)
			{
				ucmd->upmove = 127;

				if (NPC->s.eType == ET_PLAYER)
				{
					trap->EA_Jump(NPC->s.number);
				}
			}
			else if (NPC->bot_strafe_left_timer > level.time)
			{
				ucmd->rightmove = -127;
				trap->EA_MoveLeft(NPC->s.number);
			}
			else if (NPC->bot_strafe_right_timer > level.time)
			{
				ucmd->rightmove = 127;
				trap->EA_MoveRight(NPC->s.number);
			}

			if (NPC->last_move_time < level.time - 2000)
			{
				ucmd->upmove = 127;

				if (NPC->s.eType == ET_PLAYER)
				{
					trap->EA_Jump(NPC->s.number);
				}
			}
		}
		else if (NPC->bot_strafe_jump_timer > level.time)
		{
			ucmd->upmove = 127;

			if (NPC->s.eType == ET_PLAYER)
			{
				trap->EA_Jump(NPC->s.number);
			}
		}
		else if (NPC->bot_strafe_left_timer > level.time)
		{
			ucmd->rightmove = -127;
			trap->EA_MoveLeft(NPC->s.number);
		}
		else if (NPC->bot_strafe_right_timer > level.time)
		{
			ucmd->rightmove = 127;
			trap->EA_MoveRight(NPC->s.number);
		}

		if (NPC->last_move_time < level.time - 4000)
		{
			NPC_ClearPathData(NPC);
			NPC_SetNewGoalAndPath(aiEnt);
			return qfalse;
		}

		if (NPC->last_move_time < level.time - 2000)
		{
			ucmd->upmove = 127;

			if (NPC->s.eType == ET_PLAYER)
			{
				trap->EA_Jump(NPC->s.number);
			}
		}
	}

	VectorCopy( NPC->movedir, NPC->client->ps.moveDir );
#else //!__USE_NAVMESH__
	Warzone_Nav_UpdateEntity(NPC);
#endif //__USE_NAVMESH__

	return qtrue;
}

void NPC_SetNewEnemyGoalAndPath(gentity_t *aiEnt)
{
#ifndef __USE_NAVMESH__
	gentity_t	*NPC = aiEnt;

	if (NPC->npc_dumb_route_time > level.time)
	{// Try to use JKA routing as a backup until timer runs out...
		if ( UpdateGoal(aiEnt) )
		{
			if (NPC_CombatMoveToGoal(aiEnt, qtrue, qfalse ))
			{// Worked!
				return;
			}
		}

		// Failed... Idle...
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.rightmove = 0;
		aiEnt->client->pers.cmd.upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		return;
	}

	if (NPC->wpSeenTime > level.time)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		return; // wait for next route creation...
	}

	if (!NPC_FindNewWaypoint(aiEnt))
	{
		NPC->npc_dumb_route_time = level.time + 10000;
		return; // wait before trying to get a new waypoint...
	}

	NPC->longTermGoal = DOM_GetNearestWP(aiEnt->enemy->r.currentOrigin, NPC->wpCurrent);

	if (NPC->longTermGoal >= 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, sizeof(int)*MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qfalse);

		if (NPC->pathsize > 0)
		{
			//G_Printf("NPC Waypointing Debug: NPC %i created a %i waypoint path for a random goal between waypoints %i and %i.", NPC->s.number, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal);
			NPC->wpLast = -1;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from
		}
		else
		{
			NPC->npc_dumb_route_time = level.time + 10000;
			//G_Printf("NPC Waypointing Debug: NPC %i failed to create a route between waypoints %i and %i.", NPC->s.number, NPC->wpCurrent, NPC->longTermGoal);
			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 1000;//30000;
			NPC_PickRandomIdleAnimantion(NPC);
			return;
		}
	}
	else
	{
		//G_Printf("NPC Waypointing Debug: NPC %i failed to find a goal waypoint.", NPC->s.number);
		
		//trap->Print("Unable to find goal waypoint.\n");

		NPC->npc_dumb_route_time = level.time + 10000;

		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 10000;//30000;
		NPC_PickRandomIdleAnimantion(NPC);
		return;
	}

	// Delay before next route creation...
	NPC->wpSeenTime = level.time + 1000;//30000;
	// Delay before giving up on this new waypoint/route...
	NPC->wpTravelTime = level.time + 15000;
#endif //__USE_NAVMESH__
}

extern int jediSpeechDebounceTime[FACTION_NUM_FACTIONS];//used to stop several jedi AI from speaking all at once
extern int groupSpeechDebounceTime[FACTION_NUM_FACTIONS];//used to stop several group AI from speaking all at once

qboolean NPC_FollowEnemyRoute(gentity_t *aiEnt)
{// Quick method of following bot routes...
	gentity_t	*NPC = aiEnt;
	usercmd_t	ucmd = aiEnt->client->pers.cmd;
	float		wpDist = 0.0;
	qboolean	onMover1 = qfalse;
	qboolean	onMover2 = qfalse;

	aiEnt->NPC->combatMove = qtrue;

	if ( !NPC_HaveValidEnemy(aiEnt) )
	{
		NPC_ClearGoal(aiEnt);
		return qfalse;
	}

	if (NPC_GetOffPlayer(NPC))
	{// Get off of their head!
		return qfalse;
	}

	if (NPC_MoverCrushCheck(NPC))
	{// There is a mover gonna crush us... Step back...
		return qtrue;
	}

	if ((NPC->client->ps.weapon == WP_SABER || NPC->client->ps.weapon == WP_MELEE)
		&& Distance(NPC->r.currentOrigin, aiEnt->enemy->r.currentOrigin) <= 48)
	{// Close enough already... Don't move...
		//trap->Print("close!\n");
		return qfalse;
	}
	else if ( !(NPC->client->ps.weapon == WP_SABER || NPC->client->ps.weapon == WP_MELEE)
		&& NPC_ClearLOS4(aiEnt, aiEnt->enemy ))
	{// Already visible to shoot... Don't move...
		//trap->Print("close wp!\n");
		return qfalse;
	}

#ifdef __USE_NAVLIB__
	if (G_NavmeshIsLoaded())
	{
		NavlibSetNavMesh(NPC->s.number, 0);

		if (aiEnt->client->navigation.goal.ent != NPC->enemy)
		{
			NPC_ClearGoal(NPC);
			NPC->client->navigation.goal.ent = NPC->enemy;

#pragma omp critical
			{
				NPC->client->navigation.goal.haveGoal = (qboolean)NavlibFindRouteToTarget(NPC, NPC->client->navigation.goal, qtrue);
			}
		}
	}

	return NPC_FollowRoutes(NPC);
#else //!__USE_NAVLIB__

#ifndef __USE_NAVMESH__
	if (DistanceHorizontal(NPC->r.currentOrigin, NPC->npc_previous_pos) > 3)
	{
		NPC->last_move_time = level.time;
		VectorCopy(NPC->r.currentOrigin, NPC->npc_previous_pos);
	}

	if ( NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum )
	{
		vec3_t upOrg, upOrg2;

		VectorCopy(NPC->r.currentOrigin, upOrg);
		upOrg[2]+=18;

		VectorCopy(gWPArray[NPC->wpCurrent]->origin, upOrg2);
		upOrg2[2]+=18;

		if (NPC->wpSeenTime <= level.time)
		{
			if (OrgVisible(upOrg, upOrg2, NPC->s.number))
			{
				NPC->wpSeenTime = level.time;
			}
		}

		if (NPC_IsJetpacking(NPC))
		{// Jetpacking... Ignore heights...
			wpDist = DistanceHorizontal(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);
		}
		else if (NPC->s.groundEntityNum != ENTITYNUM_NONE)
		{// On somebody's head or in the air...
			wpDist = DistanceHorizontal(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);
		}
		else
		{// On ground...
			wpDist = Distance(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);
		}

	}

#if 0
	if ( (NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum && NPC->longTermGoal >= 0 && NPC->longTermGoal < gWPNum && wpDist <= MAX_LINK_DISTANCE)
		&& (NPC->wpSeenTime < level.time - 1000 || NPC->wpTravelTime < level.time || NPC->last_move_time < level.time - 1000) )
	{// Try this for 2 seconds before giving up...
		float MAX_JUMP_DISTANCE = 192.0;
		
		if (NPC_IsJedi(NPC)) MAX_JUMP_DISTANCE = 512.0; // Jedi can jump further...

		if (Distance(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) <= MAX_JUMP_DISTANCE
			&& NPC_TryJump( NPC, gWPArray[NPC->wpCurrent]->origin ))
		{// Looks like we can jump there... Let's do that instead of failing!
			//trap->Print("%s is jumping to waypoint.\n", NPC->client->pers.netname);
			return qtrue; // next think...
		}
	}
#endif

	if ( NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum 
		|| NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum 
		//|| wpDist > MAX_LINK_DISTANCE
		|| NPC->wpSeenTime < level.time - 5000
		|| NPC->wpTravelTime < level.time 
		|| NPC->last_move_time < level.time - 5000 
		|| Distance(gWPArray[NPC->longTermGoal]->origin, NPC->enemy->r.currentOrigin) > 256.0)
	{// We hit a problem in route, or don't have one yet.. Find a new goal and path...

		if (wpDist > MAX_LINK_DISTANCE || NPC->wpTravelTime < level.time )
		{
			NPC_RoutingIncreaseCost( NPC->wpLast, NPC->wpCurrent );
		}

		NPC_ClearPathData(NPC);
		NPC_SetNewEnemyGoalAndPath(aiEnt);
		G_ClearEnemy(NPC); // UQ1: Give up...

		if (NPC_IsJedi(aiEnt))
		{
			if ( !Q_irand( 0, 10 ) && aiEnt->NPC->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[aiEnt->client->playerTeam] < level.time )
			{
				G_AddVoiceEvent( aiEnt, Q_irand( EV_JLOST1, EV_JLOST3 ), 10000 );
				jediSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
			}
		}
		else
		{
			if ( !Q_irand( 0, 10 ) && aiEnt->NPC->blockedSpeechDebounceTime < level.time && groupSpeechDebounceTime[aiEnt->client->playerTeam] < level.time )
			{
				G_AddVoiceEvent( aiEnt, Q_irand( EV_GIVEUP1, EV_GIVEUP4 ), 10000 );
				groupSpeechDebounceTime[aiEnt->client->playerTeam] = aiEnt->NPC->blockedSpeechDebounceTime = level.time + 10000;
			}
		}

		if (!(NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum))
		{
			NPC->wpTravelTime = level.time + 15000;
			NPC->wpSeenTime = level.time;
			NPC->last_move_time = level.time;
		}
	}

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
	{// FIXME: Try to roam out of problems...
		NPC_ClearPathData(NPC);
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		ucmd.upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		//trap->Print("NO WP!\n");
		return qfalse; // next think...
	}

	if (Distance(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 48
		|| (NPC->s.groundEntityNum != ENTITYNUM_NONE && DistanceHorizontal(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 48))
	{// We're at out goal! Find a new goal...
		NPC_ClearPathData(NPC);
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		ucmd.upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		//trap->Print("AT DEST!\n");
		return qfalse; // next think...
	}

	if (NPC_DoLiftPathing(NPC))
	{
		return qtrue;
	}

	if (wpDist < 48)
	{// At current node.. Pick next in the list...
		NPC->wpLast = NPC->wpCurrent;
		NPC->wpCurrent = NPC->wpNext;
		NPC->wpNext = NPC_GetNextNode(NPC);

		if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
		{// FIXME: Try to roam out of problems...
			NPC_ClearPathData(NPC);
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			ucmd.upmove = 0;
			NPC_PickRandomIdleAnimantion(NPC);
			//trap->Print("????\n");
			return qfalse; // next think...
		}

		NPC->wpTravelTime = level.time + 15000;
		NPC->wpSeenTime = level.time;
	}

	{
		vec3_t upOrg, upOrg2;

		VectorCopy(NPC->r.currentOrigin, upOrg);
		upOrg[2]+=18;

		VectorCopy(gWPArray[NPC->wpCurrent]->origin, upOrg2);
		upOrg2[2]+=18;

		if (NPC->wpSeenTime <= level.time)
		{
			if (OrgVisible(upOrg, upOrg2, NPC->s.number))
			{
				NPC->wpSeenTime = level.time;
			}
		}
	}

	//NPC_FacePosition( NPC, gWPArray[NPC->wpCurrent]->origin, qfalse );
	VectorSubtract( gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir );

	if (VectorLength(NPC->client->ps.velocity) < 8 && NPC_RoutingJumpWaypoint( NPC->wpLast, NPC->wpCurrent ))
	{// We need to jump to get to this waypoint...
		if (NPC_EnemyAboveMe(NPC) && NPC_Jump(NPC, gWPArray[NPC->wpCurrent]->origin))
		{
			//trap->Print("NPC JUMP DEBUG: NPC_FollowEnemyRoute\n");
			VectorCopy( NPC->movedir, NPC->client->ps.moveDir );
			return qtrue;
		}
	}
	else if (VectorLength(NPC->client->ps.velocity) < 8)
	{// If this is a new waypoint, we may need to jump to it...
		NPC_NewWaypointJump(aiEnt);
	}

	if (!UQ1_UcmdMoveForDir( NPC, &aiEnt->client->pers.cmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin )) { /*NPC_PickRandomIdleAnimantion(NPC);*/ return qtrue; }
	VectorCopy( NPC->movedir, NPC->client->ps.moveDir );
	//NPC_SelectMoveAnimation(qfalse);
#else //!__USE_NAVMESH__
	Warzone_Nav_UpdateEntity(NPC);
#endif //__USE_NAVMESH__

	return qtrue;
#endif //__USE_NAVLIB__
}
