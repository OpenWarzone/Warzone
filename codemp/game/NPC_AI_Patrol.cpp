#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

extern int gWPNum;
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

extern int DOM_GetNearestWP(vec3_t org, int badwp);
extern int NPC_GetNextNode(gentity_t *NPC);
extern qboolean UQ1_UcmdMoveForDir ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest );
//extern qboolean NPC_CoverpointVisible ( gentity_t *NPC, int coverWP );
extern int DOM_GetRandomCloseWP(vec3_t org, int badwp, int unused);
extern int DOM_GetNearestVisibleWP(vec3_t org, int ignore, int badwp);
extern int DOM_GetNearestVisibleWP_Goal(vec3_t org, int ignore, int badwp);
extern int DOM_GetNearestVisibleWP_NOBOX(vec3_t org, int ignore, int badwp);
extern gentity_t *NPC_PickEnemyExt( qboolean checkAlerts );
extern qboolean NPC_MoveDirClear( int forwardmove, int rightmove, qboolean reset );
extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir, vec3_t dest );

qboolean DOM_NPC_ClearPathToSpot( gentity_t *NPC, vec3_t dest, int impactEntNum );
extern int DOM_GetRandomCloseVisibleWP(gentity_t *ent, vec3_t org, int ignoreEnt, int badwp);

extern qboolean NPC_DoLiftPathing(gentity_t *NPC);
extern void NPC_NewWaypointJump(gentity_t *aiEnt);
extern qboolean Warzone_SpawnpointNearMoverEntityLocation(vec3_t org);

qboolean WAYPOINT_PARTOL_BAD_LIST_INITIALIZED = qfalse;
qboolean WAYPOINT_PARTOL_BAD_LIST[MAX_WPARRAY_SIZE] = { qfalse };

#define WPFLAG_WATER				0x40000000 //water point

void NPC_Patrol_MakeBadList(void)
{
	if (WAYPOINT_PARTOL_BAD_LIST_INITIALIZED) return;

	for (int i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i]->flags & WPFLAG_WATER)
		{
			WAYPOINT_PARTOL_BAD_LIST[i] = qtrue;
			continue;
		}

		if (Warzone_SpawnpointNearMoverEntityLocation(gWPArray[i]->origin))
		{
			WAYPOINT_PARTOL_BAD_LIST[i] = qtrue;
			continue;
		}
	}

	WAYPOINT_PARTOL_BAD_LIST_INITIALIZED = qtrue;
}

#define MAX_BEST_PATROL_WAYPOINT_LIST 1024

extern int villageWaypointsNum;
extern int villageWaypoints[MAX_WPARRAY_SIZE];

extern int wildernessWaypointsNum;
extern int wildernessWaypoints[MAX_WPARRAY_SIZE];

int NPC_FindPatrolGoal(gentity_t *NPC)
{
	int bestWaypointsNum = 0;
	int bestWaypoints[MAX_BEST_PATROL_WAYPOINT_LIST] = { -1 };

	NPC_Patrol_MakeBadList(); // Init if it hasn't been checked yet...

	if (villageWaypointsNum > 0)
	{// Have village/town/city waypoints...
		if (NPC_IsCivilian(NPC) || NPC_IsVendor(NPC))
		{// Civilians stay in the village/town/city...
			for (int i = 0; i < villageWaypointsNum && bestWaypointsNum < MAX_BEST_PATROL_WAYPOINT_LIST; i++)
			{
				if (WAYPOINT_PARTOL_BAD_LIST[villageWaypoints[i]]) continue;

				float spawnDist = Distance(gWPArray[villageWaypoints[i]]->origin, NPC->spawn_pos);
				float spawnDistHeight = DistanceVertical(gWPArray[villageWaypoints[i]]->origin, NPC->spawn_pos);

				if (spawnDist < 2048.0 && spawnDist > 1024.0)
				{// If this spot is close to me, but not too close, then maybe add it to the best list...
					if (spawnDistHeight < spawnDist * 0.15)
					{// This point is at a height close to the original spawn position... Looks good...
						bestWaypoints[bestWaypointsNum] = villageWaypoints[i];
						bestWaypointsNum++;
					}
				}
			}

			if (bestWaypointsNum <= 0)
			{// Failed to find any, try a second method, allowing more options...
				for (int i = 0; i < villageWaypointsNum && bestWaypointsNum < MAX_BEST_PATROL_WAYPOINT_LIST; i++)
				{
					if (WAYPOINT_PARTOL_BAD_LIST[villageWaypoints[i]]) continue;

					float spawnDist = Distance(gWPArray[villageWaypoints[i]]->origin, NPC->spawn_pos);
					float spawnDistHeight = DistanceVertical(gWPArray[villageWaypoints[i]]->origin, NPC->spawn_pos);

					if (spawnDist < 1024.0 && spawnDist > 256.0)
					{// If this spot is close to me, but not too close, then maybe add it to the best list...
						if (spawnDistHeight < spawnDist * 0.15)
						{// This point is at a height close to the original spawn position... Looks good...
							bestWaypoints[bestWaypointsNum] = villageWaypoints[i];
							bestWaypointsNum++;
						}
					}
				}
			}
		}
		else
		{// Normal NPCs stay out of the village/town/city... Mostly...
			for (int i = 0; i < wildernessWaypointsNum && bestWaypointsNum < MAX_BEST_PATROL_WAYPOINT_LIST; i++)
			{
				if (WAYPOINT_PARTOL_BAD_LIST[wildernessWaypoints[i]]) continue;

				float spawnDist = Distance(gWPArray[wildernessWaypoints[i]]->origin, NPC->spawn_pos);
				float spawnDistHeight = DistanceVertical(gWPArray[wildernessWaypoints[i]]->origin, NPC->spawn_pos);

				if (spawnDist < 2048.0 && spawnDist > 1024.0)
				{// If this spot is close to me, but not too close, then maybe add it to the best list...
					if (spawnDistHeight < spawnDist * 0.15)
					{// This point is at a height close to the original spawn position... Looks good...
						bestWaypoints[bestWaypointsNum] = wildernessWaypoints[i];
						bestWaypointsNum++;
					}
				}
			}

			if (bestWaypointsNum <= 0)
			{// Failed to find any, try a second method, allowing more options...
				for (int i = 0; i < wildernessWaypointsNum && bestWaypointsNum < MAX_BEST_PATROL_WAYPOINT_LIST; i++)
				{
					if (WAYPOINT_PARTOL_BAD_LIST[wildernessWaypoints[i]]) continue;

					float spawnDist = Distance(gWPArray[wildernessWaypoints[i]]->origin, NPC->spawn_pos);
					float spawnDistHeight = DistanceVertical(gWPArray[wildernessWaypoints[i]]->origin, NPC->spawn_pos);

					if (spawnDist < 1024.0 && spawnDist > 256.0)
					{// If this spot is close to me, but not too close, then maybe add it to the best list...
						if (spawnDistHeight < spawnDist * 0.15)
						{// This point is at a height close to the original spawn position... Looks good...
							bestWaypoints[bestWaypointsNum] = wildernessWaypoints[i];
							bestWaypointsNum++;
						}
					}
				}
			}
		}
	}
	else
	{// No village/town/city waypoints...
		for (int i = 0; i < gWPNum && bestWaypointsNum < MAX_BEST_PATROL_WAYPOINT_LIST; i++)
		{
			if (WAYPOINT_PARTOL_BAD_LIST[i]) continue;

			float spawnDist = Distance(gWPArray[i]->origin, NPC->spawn_pos);
			float spawnDistHeight = DistanceVertical(gWPArray[i]->origin, NPC->spawn_pos);

			if (spawnDist < 2048.0 && spawnDist > 1024.0)
			{// If this spot is close to me, but not too close, then maybe add it to the best list...
				if (spawnDistHeight < spawnDist * 0.15)
				{// This point is at a height close to the original spawn position... Looks good...
					bestWaypoints[bestWaypointsNum] = i;
					bestWaypointsNum++;
				}
			}
		}

		if (bestWaypointsNum <= 0)
		{// Failed to find any, try a second method, allowing more options...
			for (int i = 0; i < gWPNum && bestWaypointsNum < MAX_BEST_PATROL_WAYPOINT_LIST; i++)
			{
				if (WAYPOINT_PARTOL_BAD_LIST[i]) continue;

				float spawnDist = Distance(gWPArray[i]->origin, NPC->spawn_pos);
				float spawnDistHeight = DistanceVertical(gWPArray[i]->origin, NPC->spawn_pos);

				if (spawnDist < 1024.0 && spawnDist > 256.0)
				{// If this spot is close to me, but not too close, then maybe add it to the best list...
					if (spawnDistHeight < spawnDist * 0.15)
					{// This point is at a height close to the original spawn position... Looks good...
						bestWaypoints[bestWaypointsNum] = i;
						bestWaypointsNum++;
					}
				}
			}
		}
	}

	if (bestWaypointsNum <= 0)
	{// Failed...
		return -1;
	}

	int selected = bestWaypoints[irand_big(0, bestWaypointsNum - 1)];

	//trap->Print("NPC %s selected patrol point %i, which is %i distance from spawnpoint. spawn %i %i %i. wp %i %i %i.\n", NPC->client->pers.netname, selected, (int)Distance(NPC->spawn_pos, gWPArray[selected]->origin), (int)NPC->spawn_pos[0], (int)NPC->spawn_pos[1], (int)NPC->spawn_pos[2], (int)gWPArray[selected]->origin[0], (int)gWPArray[selected]->origin[1], (int)gWPArray[selected]->origin[2]);
	return bestWaypoints[irand_big(0, bestWaypointsNum - 1)];
}

#ifdef __USE_NAVLIB__
int NPC_FindPatrolGoalNavLib(gentity_t *NPC)
{
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
			FindRandomNavmeshPointInRadius(NPC->s.number, gWPArray[waypoint]->origin, NPC->client->navigation.goal.origin, 99999999.9);
		}
		//trap->Print("[%s] newGoal: %f %f %f.\n", NPC->client->pers.netname, NPC->client->navigation.goal.origin[0], NPC->client->navigation.goal.origin[1], NPC->client->navigation.goal.origin[2]);
		return 1;
	}
	else
	{
#pragma omp critical
		{
			FindRandomNavmeshSpawnpoint(NPC, NPC->client->navigation.goal.origin);
		}
		//trap->Print("[%s] newGoal: %f %f %f.\n", NPC->client->pers.netname, NPC->client->navigation.goal.origin[0], NPC->client->navigation.goal.origin[1], NPC->client->navigation.goal.origin[2]);
		return 1;
	}
#else
	if (!G_NavmeshIsLoaded())
	{
		return 0;
	}

#pragma omp critical
	{
		FindRandomNavmeshPatrolPoint(NPC->s.number, NPC->client->navigation.goal.origin);
	}
/*#pragma omp critical
	{
		FindRandomNavmeshPointInRadius(NPC->s.number, NPC->r.currentOrigin, NPC->client->navigation.goal.origin, 2048.0);
	}*/

	if (VectorLength(NPC->client->navigation.goal.origin) == 0 || Distance(NPC->r.currentOrigin, NPC->client->navigation.goal.origin) == 0)
	{
		//trap->Print("[%s] failed to find a patrol goal from: %f %f %f.\n", NPC->client->pers.netname, NPC->r.currentOrigin[0], NPC->r.currentOrigin[1], NPC->r.currentOrigin[2]);
		return -1;
	}

	//trap->Print("[%s] newGoal: %f %f %f found for point %f %f %f.\n", NPC->client->pers.netname, NPC->client->navigation.goal.origin[0], NPC->client->navigation.goal.origin[1], NPC->client->navigation.goal.origin[2], NPC->r.currentOrigin[0], NPC->r.currentOrigin[1], NPC->r.currentOrigin[2]);

	return 1;
#endif
}
#endif //__USE_NAVLIB__

void NPC_SetNewPatrolGoalAndPath(gentity_t *aiEnt)
{
	if (aiEnt->next_pathfind_time > level.time)
	{
		return;
	}

	aiEnt->next_pathfind_time = level.time + 10000 + irand(0, 1000);

#ifdef __USE_NAVLIB__
	if (NPC_FindPatrolGoalNavLib(aiEnt))
	{
#pragma omp critical
		{
			aiEnt->client->navigation.goal.haveGoal = (qboolean)NavlibFindRouteToTarget(aiEnt, aiEnt->client->navigation.goal, qtrue);
		}

		//if (aiEnt->client->navigation.goal.haveGoal)
		//	trap->Print("%s found a patrol route.\n", aiEnt->client->pers.netname);
		//else
		//	trap->Print("%s failed to find a patrol route.\n", aiEnt->client->pers.netname);
	}
	return;
#endif //__USE_NAVLIB__

	gentity_t	*NPC = aiEnt;

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
		if (NPC_IsHumanoid(NPC))
			NPC_PickRandomIdleAnimantion(NPC);
		return; // wait for next route creation...
	}

	if (!NPC_FindNewWaypoint(aiEnt))
	{
		//trap->Print("Unable to find waypoint.\n");
		//player_die(NPC, NPC, NPC, 99999, MOD_CRUSH);
		return; // wait before trying to get a new waypoint...
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
		NPC->longTermGoal = NPC_FindPatrolGoal(NPC);
	}

	if (NPC->longTermGoal >= 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, sizeof(int)*MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, (qboolean)(irand(0, 5) <= 0));

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
}

qboolean NPC_PatrolArea(gentity_t *aiEnt)
{
	gentity_t	*NPC = aiEnt;
	usercmd_t	*ucmd = &aiEnt->client->pers.cmd;
	float		wpDist = 0.0;
	qboolean	onMover1 = qfalse;
	qboolean	onMover2 = qfalse;

	aiEnt->NPC->combatMove = /*aiEnt->NPC->vehicleAI ? qfalse :*/ qtrue;

	if (!NPC_HaveValidEnemy(aiEnt))
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
	
	/*if (NPC->noWaypointTime > level.time)
	{
		if (NPC_IsHumanoid(NPC))
			NPC_PickRandomIdleAnimantion(NPC);

		return qfalse;
	}*/

#ifdef __USE_NAVLIB__
	if (G_NavmeshIsLoaded())
	{
		NavlibSetNavMesh(NPC->s.number, 0);

		qboolean walk = qtrue;

		if (DistanceHorizontal(NPC->r.currentOrigin, NPC->npc_previous_pos) > 3)
		{
			NPC->last_move_time = level.time;
			VectorCopy(NPC->r.currentOrigin, NPC->npc_previous_pos);
		}

		if (!NPC->client->navigation.goal.haveGoal)
		{
			NPC->client->navigation.goal.haveGoal = qfalse;
			VectorClear(NPC->client->navigation.goal.origin);
			NPC->client->navigation.goal.ent = NULL;
			NPC_SetNewPatrolGoalAndPath(NPC);

			if (NPC->client->navigation.goal.haveGoal)
			{
				NPC->last_move_time = level.time;
			}
			else
			{
				ucmd->forwardmove = 0;
				ucmd->rightmove = 0;
				ucmd->upmove = 0;

				NPC->client->navigation.goal.haveGoal = qfalse;
				VectorClear(NPC->client->navigation.goal.origin);
				NPC->client->navigation.goal.ent = NULL;

				if (NPC_IsHumanoid(NPC))
					NPC_PickRandomIdleAnimantion(NPC);

				//if (aiEnt->NPC->vehicleAI) Com_Printf("DEBUG VEHAI: waiting2\n");

				return qtrue; // next think...
			}
		}

		qboolean goalInRange = qfalse;

		if (NPC->client->navigation.goal.haveGoal)
		{
			float goalHitDistance = max(128.0, NavlibGetGoalRadius(NPC));

			if (NPC->client->navigation.goal.ent)
			{
				float goalDist = DistanceHorizontal(NPC->client->navigation.goal.ent->r.currentOrigin, NPC->r.currentOrigin);
				goalInRange = (goalDist <= goalHitDistance) ? qtrue : qfalse;
			}
			else
			{
				float goalDist = DistanceHorizontal(NPC->client->navigation.goal.origin, NPC->r.currentOrigin);
				goalInRange = (goalDist <= goalHitDistance) ? qtrue : qfalse;
			}
		}

		if (NPC->client->navigation.goal.haveGoal && !goalInRange)
		{
			if (NPC->last_move_time < level.time - 2000)
			{
				NPC->client->navigation.goal.haveGoal = qfalse;
				VectorClear(NPC->client->navigation.goal.origin);
				NPC->client->navigation.goal.ent = NULL;
				
				ucmd->forwardmove = 0;
				ucmd->rightmove = 0;
				ucmd->upmove = 0;

				if (NPC_IsHumanoid(NPC))
					NPC_PickRandomIdleAnimantion(NPC);

				return qtrue;
			}

#pragma omp critical
			{
				NavlibMoveToGoal(NPC);
			}

			NPC_FacePosition(NPC, NPC->client->navigation.nav.lookPos, qfalse);
			VectorSubtract(NPC->client->navigation.nav.lookPos, NPC->r.currentOrigin, NPC->movedir);

#ifndef __USE_NAVLIB_INTERNAL_MOVEMENT__
			if (DistanceHorizontal(NPC->r.currentOrigin, NPC->client->navigation.goal.origin) < 256)
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
				//trap->Print("DEBUG: bot_strafe_jump_timer\n");

				if (NPC->s.eType == ET_PLAYER)
				{
					trap->EA_Jump(NPC->s.number);
				}

				//if (aiEnt->NPC->vehicleAI) Com_Printf("DEBUG VEHAI: strafeJump\n");
				return qtrue;
			}
			else if (NPC->bot_strafe_left_timer > level.time)
			{
				ucmd->rightmove = -127;
				trap->EA_MoveLeft(NPC->s.number);
				//if (aiEnt->NPC->vehicleAI) Com_Printf("DEBUG VEHAI: strafeLeft\n");
				return qtrue;
			}
			else if (NPC->bot_strafe_right_timer > level.time)
			{
				ucmd->rightmove = 127;
				trap->EA_MoveRight(NPC->s.number);
				//if (aiEnt->NPC->vehicleAI) Com_Printf("DEBUG VEHAI: strafeRight\n");
				return qtrue;
			}
#endif //__USE_NAVLIB_INTERNAL_MOVEMENT__

			return qfalse;
		}
		else
		{// Need a new goal...
			NPC_SetNewPatrolGoalAndPath(NPC);

			if (NPC->client->navigation.goal.haveGoal)
			{
				NPC->last_move_time = level.time;
			}

			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;

			if (NPC_IsHumanoid(NPC))
				NPC_PickRandomIdleAnimantion(NPC);

			//if (aiEnt->NPC->vehicleAI) Com_Printf("DEBUG VEHAI: newgoal %s\n", NPC->client->navigation.goal.haveGoal ? "TRUE" : "FALSE");

			return qtrue;
		}

		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;

		if (NPC_IsHumanoid(NPC))
			NPC_PickRandomIdleAnimantion(NPC);

		return qfalse;
	}
#endif //__USE_NAVLIB__

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum
		|| NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum
		|| NPC->wpSeenTime < level.time - 5000
		|| NPC->wpTravelTime < level.time
		|| NPC->last_move_time < level.time - 15000)
	{// We hit a problem in route, or don't have one yet.. Find a new goal and path...
#ifdef ___AI_PATHING_DEBUG___
		if (NPC->wpSeenTime < level.time - 5000) trap->Print("PATHING DEBUG: %i wpSeenTime.\n", NPC->s.number);
		if (NPC->wpTravelTime < level.time) trap->Print("PATHING DEBUG: %i wpTravelTime.\n", NPC->s.number);
		if (NPC->last_move_time < level.time - 5000) trap->Print("PATHING DEBUG: %i last_move_time.\n", NPC->s.number);
		if ((NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum) && (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum)) trap->Print("PATHING DEBUG: %i wpCurrent & longTermGoal.\n", NPC->s.number);
		else if (NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum) trap->Print("PATHING DEBUG: %i longTermGoal.\n", NPC->s.number);
		else if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum) trap->Print("PATHING DEBUG: %i wpCurrent.\n", NPC->s.number);
#endif //___AI_PATHING_DEBUG___

		NPC_RoutingIncreaseCost(NPC->wpLast, NPC->wpCurrent);

		NPC_ClearPathData(NPC);
		NPC_SetNewPatrolGoalAndPath(aiEnt);
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
		
		if (NPC_IsHumanoid(NPC))
			NPC_PickRandomIdleAnimantion(NPC);
		return qtrue; // next think...
	}

	if (Distance(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 48
		|| (NPC->s.groundEntityNum != ENTITYNUM_NONE && DistanceHorizontal(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 48))
	{// We're at our goal! Find a new goal...
#ifdef ___AI_PATHING_DEBUG___
		trap->Print("PATHING DEBUG: HIT GOAL!\n");
#endif //___AI_PATHING_DEBUG___
		NPC_ClearPathData(NPC);
		NPC->noWaypointTime = level.time + 10000; // Idle at least 10 seconds at this point before finding a new patrol position...
		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;

		if (NPC_IsHumanoid(NPC))
			NPC_PickRandomIdleAnimantion(NPC);
		return qtrue; // next think...
	}

	if (DistanceHorizontal(NPC->r.currentOrigin, NPC->npc_previous_pos) > 3)
	{
		NPC->last_move_time = level.time;
		VectorCopy(NPC->r.currentOrigin, NPC->npc_previous_pos);
	}

	if (NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum)
	{
		vec3_t upOrg, upOrg2;

		VectorCopy(NPC->r.currentOrigin, upOrg);
		upOrg[2] += 18;

		VectorCopy(gWPArray[NPC->wpCurrent]->origin, upOrg2);
		upOrg2[2] += 18;

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
			
			if (NPC_IsHumanoid(NPC))
				NPC_PickRandomIdleAnimantion(NPC);
			return qtrue; // next think...
		}

		NPC->wpTravelTime = level.time + 15000;
		NPC->wpSeenTime = level.time;
	}

	NPC_FacePosition(aiEnt, gWPArray[NPC->wpCurrent]->origin, qfalse);
	NPC->s.angles[PITCH] = NPC->client->ps.viewangles[PITCH] = 0; // Init view PITCH angle so we always look forward, not down or up...
	VectorSubtract(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir);

	if (NPC_DoLiftPathing(NPC))
	{
		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		
		if (NPC_IsHumanoid(NPC))
			NPC_PickRandomIdleAnimantion(NPC);
		return qtrue;
	}

	if (NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum && VectorLength(NPC->client->ps.velocity) < 8 && NPC_RoutingJumpWaypoint(NPC->wpLast, NPC->wpCurrent))
	{// We need to jump to get to this waypoint...
		if (NPC_Jump(NPC, gWPArray[NPC->wpCurrent]->origin))
		{
			//trap->Print("NPC JUMP DEBUG: NPC_FollowRoutes\n");
			VectorCopy(NPC->movedir, NPC->client->ps.moveDir);
			return qtrue;
		}
	}
	else if (NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum && VectorLength(NPC->client->ps.velocity) < 8)
	{// If this is a new waypoint, we may need to jump to it...
		NPC_NewWaypointJump(aiEnt);
	}

	if (NPC_IsCivilian(NPC))
	{
		if (NPC->npc_cower_runaway)
		{// A civilian running away from combat...
			if (!UQ1_UcmdMoveForDir(NPC, &aiEnt->client->pers.cmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin))
			{
				if (NPC_IsHumanoid(NPC))
				{
					if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
						NPC_PickRandomIdleAnimantion(NPC);
					else
						NPC_SelectMoveRunAwayAnimation(NPC);
				}

				return qtrue;
			}

			if (NPC_IsHumanoid(NPC))
			{
				if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
					NPC_PickRandomIdleAnimantion(NPC);
				else
					NPC_SelectMoveRunAwayAnimation(NPC);
			}

			VectorCopy(NPC->movedir, NPC->client->ps.moveDir);

			return qtrue;
		}
		else if (NPC_IsCivilianHumanoid(NPC))
		{// Civilian humanoid... Force walk/run anims...
			if (NPC_PointIsMoverLocation(gWPArray[NPC->wpCurrent]->origin))
			{// When nearby a mover, run!
				if (!UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin))
				{
					if (NPC_IsHumanoid(NPC))
					{
						if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
							NPC_PickRandomIdleAnimantion(NPC);
						else
							NPC_SelectMoveAnimation(aiEnt, qfalse);
					}

					return qtrue;
				}

				if (NPC_IsHumanoid(NPC))
				{
					if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
						NPC_PickRandomIdleAnimantion(NPC);
					else
						NPC_SelectMoveAnimation(aiEnt, qtrue); // UQ1: Always set civilian walk animation...
				}
			}
			else if (!UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, qtrue, gWPArray[NPC->wpCurrent]->origin))
			{
				if (NPC_IsHumanoid(NPC))
				{
					if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
						NPC_PickRandomIdleAnimantion(NPC);
					else
						NPC_SelectMoveAnimation(aiEnt, qtrue);
				}

				return qtrue;
			}

			if (NPC_IsHumanoid(NPC))
			{
				if (aiEnt->client->pers.cmd.forwardmove == 0 && aiEnt->client->pers.cmd.rightmove == 0 && aiEnt->client->pers.cmd.upmove == 0)
					NPC_PickRandomIdleAnimantion(NPC);
				else
					NPC_SelectMoveAnimation(aiEnt, qtrue);
			}
		}
		else
		{// Civilian non-humanoid... let bg_ set anim...
			if (NPC_PointIsMoverLocation(gWPArray[NPC->wpCurrent]->origin))
			{// When nearby a mover, run!
				if (!UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin))
				{
					return qtrue;
				}
			}
			else if (!UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, qtrue, gWPArray[NPC->wpCurrent]->origin))
			{
				return qtrue;
			}

			return qtrue;
		}
	}
	else if (g_gametype.integer == GT_WARZONE || (NPC->r.svFlags & SVF_BOT))
	{
		if (!UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, qtrue, gWPArray[NPC->wpCurrent]->origin))
		{
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;
			
			if (NPC_IsHumanoid(NPC))
				NPC_PickRandomIdleAnimantion(NPC);
			return qtrue;
		}
	}
	else
	{
		if (!UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, qtrue, gWPArray[NPC->wpCurrent]->origin))
		{
			if (NPC->bot_strafe_jump_timer > level.time)
			{
				// Switch to running whenever jumping... Check collision when we failed the first move...
				UQ1_UcmdMoveForDir(NPC, ucmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin);

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
			// Switch to running whenever jumping... No need to check collision...
			UQ1_UcmdMoveForDir_NoAvoidance(NPC, ucmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin);

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
			
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;
			
			if (NPC_IsHumanoid(NPC))
				NPC_PickRandomIdleAnimantion(NPC);
			return qtrue;
		}

		if (NPC->last_move_time < level.time - 2000)
		{
			// Switch to running whenever jumping... No need to check collision...
			UQ1_UcmdMoveForDir_NoAvoidance(NPC, ucmd, NPC->movedir, qfalse, gWPArray[NPC->wpCurrent]->origin);

			ucmd->upmove = 127;

			if (NPC->s.eType == ET_PLAYER)
			{
				trap->EA_Jump(NPC->s.number);
			}
		}
	}

	VectorCopy(NPC->movedir, NPC->client->ps.moveDir);

	return qtrue;
}
