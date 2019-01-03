// =======================================================================================================================================
//
//
//
//                       UNIQUEONE'S ALL NEW ULTRA UBER AWESOME ULTIMATE SUPER MAGICAL ASTAR PATHFINDER
//
//                        (actually not as fast as the old one, but provides much, much better routes)
//
//                         --------------------------------------------------------------------------
//
//                                                      TODO LIST:
//									   Multithread this baby to another/other core(s).
//                                 Work out some way to optimize? - My head hurts already!
//                    Waypoint Super Highways? - Save the long waypoint lists for fast access by other AI?
//           Buy a new mouse! - This one's buttons unclick at the wrong moments all the time - it's driving me insane!
//
//
// =======================================================================================================================================

// Disable stupid warnings...
#pragma warning( disable : 4710 )

#include "ai_dominance_main.h"

#ifdef __DOMINANCE_AI__

#define MAX_NODELINKS       32
#define NODE_INVALID -1

extern int FRAME_TIME;
extern int gWPNum;
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

qboolean PATHING_IGNORE_FRAME_TIME = qfalse;

int			*openlist;												//add 1 because it's a binary heap, and they don't use 0 - 1 is the first used index
float		*gcost;
int			*fcost;
char		*list;														//0 is neither, 1 is open, 2 is closed - char because it's the smallest data type
int			*parent;

qboolean PATHFINDING_MEMORY_ALLOCATED = qfalse;

void AllocatePathFindingMemory()
{
	if (PATHFINDING_MEMORY_ALLOCATED) return;

	openlist = (int *)G_Alloc(sizeof(int)*(MAX_WPARRAY_SIZE), "AllocatePathFindingMemory (openlist)");
	gcost = (float *)G_Alloc(sizeof(float)*(MAX_WPARRAY_SIZE), "AllocatePathFindingMemory (gcost)");
	fcost = (int *)G_Alloc(sizeof(int)*(MAX_WPARRAY_SIZE), "AllocatePathFindingMemory (fcost)");
	list = (char *)G_Alloc(sizeof(char)*(MAX_WPARRAY_SIZE), "AllocatePathFindingMemory (list)");
	parent = (int *)G_Alloc(sizeof(int)*(MAX_WPARRAY_SIZE), "AllocatePathFindingMemory (parent)");

	PATHFINDING_MEMORY_ALLOCATED = qtrue;
}

qboolean LinkCanReachMe ( int wp_from, int wp_to )
{
	int i = 0;

	for (i = 0; i < gWPArray[wp_to]->neighbornum; i ++)
	{
		if (gWPArray[wp_to]->neighbors[i].num == wp_from)
			return qtrue;
	}

	return qfalse;
}

qboolean ASTAR_COSTS_DONE = qfalse;

float ASTAR_CostMultipler(int fromNode, int neigborNum)
{
	float costMult = 1.0;

	// UQ1: Prefer flat...
	float height = gWPArray[fromNode]->origin[2] - gWPArray[gWPArray[fromNode]->neighbors[neigborNum].num]->origin[2];
	if (height < 0) height *= -1.0f;
	height += 1.0;
	costMult *= height;

	if (gWPArray[fromNode]->neighbors[neigborNum].forceJumpTo > 0)
	{// Don't jump unless absolutely necessary!
		costMult *= 8.0;
	}

	if (gWPArray[fromNode]->flags & WPFLAG_WATER)
	{// Get out of water ASAP!
		costMult *= 16.0;
	}

	if (gWPArray[fromNode]->flags & WPFLAG_ROAD)
	{// Roads...
		if (gWPArray[gWPArray[fromNode]->neighbors[neigborNum].num]->flags & WPFLAG_ROAD)
		{// All road... Roads are awesome for travel, much lower cost. ;)
			costMult *= 0.0;
		}
		else
		{// From road to non-road, lower cost, but not as good as full road...
			costMult *= 0.2;
		}
	}
	else if (gWPArray[gWPArray[fromNode]->neighbors[neigborNum].num]->flags & WPFLAG_ROAD)
	{// From non-road to road, lower cost, but not as good as full road...
		costMult *= 0.2;
	}

	if (!LinkCanReachMe(fromNode, gWPArray[fromNode]->neighbors[neigborNum].num) || !LinkCanReachMe(gWPArray[fromNode]->neighbors[neigborNum].num, fromNode))
	{// One way links are bad... Make them cost much more...
		costMult *= 16.0;
	}

	return costMult;
}

void ASTAR_InitWaypointCosts ( void )
{
	int i;

	if (ASTAR_COSTS_DONE || gWPNum <= 0) return;

	{
		// Init the waypoint link costs...
		for (i = 0; i < gWPNum; i++)
		{
			int j;

			for (j = 0; j < gWPArray[i]->neighbornum; j++)
			{
				float ht = 0, hd = 0;

				gWPArray[i]->neighbors[j].cost = Distance(gWPArray[i]->origin, gWPArray[gWPArray[i]->neighbors[j].num]->origin);
				gWPArray[i]->neighbors[j].cost *= ASTAR_CostMultipler(i, j);
			}
		}
	}

	ASTAR_COSTS_DONE = qtrue;
}

int ASTAR_GetFCost(int to, int neighborNode, int parentNum, int neighborNum, float *gcost)
{
	float	gc = 0;
	float	hc = 0;

#if 0
	if (gcost[neighborNode] == -1)
	{
		if (parentNum != -1)
		{
			gc = gcost[parentNum];
#if 0
			float cost = Distance(gWPArray[parentNum]->origin, gWPArray[neighborNode]->origin);
#else
			float cost = gWPArray[parentNum]->neighbors[neighborNum].cost;
#endif

			gc += cost;
		}

		gcost[neighborNode] = gc;
	}
	else
	{
		gc = gcost[neighborNode];
	}
#else
	gc = gcost[neighborNode];
#endif
	hc = Distance(gWPArray[to]->origin, gWPArray[neighborNode]->origin);

	//return (int)((gc*0.1) + (hc*0.1));
	return (int)(gc + hc);
}

//#define __ALT_PATH_METHOD_1__ // This version creates a list of random avoid points...
#define __ALT_PATH_METHOD_2__ // This version adjusts costs randomly... Can be mixed with 1 and 3...
#define __ALT_PATH_METHOD_3__ // This version creates a single large avoid point...

int ASTAR_FindPathFast(int from, int to, int *pathlist, qboolean altPath)
{
	//all the data we have to hold...since we can't do dynamic allocation, has to be MAX_WPARRAY_SIZE
	//we can probably lower this later - eg, the open list should never have more than at most a few dozen items on it
	int			badwp = -1;
	int			numOpen = 0;
	int			atNode, temp, newnode = -1;
	qboolean	found = qfalse;
	int			count = -1;
	float		gc;
	int			i, j, u, v, m;
	gentity_t	*bot = NULL;
#ifdef __ALT_PATH_METHOD_1__
	int			IGNORE_AREAS_NUM = 0;
	int			IGNORE_AREAS[16];
#endif //__ALT_PATH_METHOD_1__
#ifdef __ALT_PATH_METHOD_3__
	int			IGNORE_AREA = 0;
	int			IGNORE_RANGE = 2048;
#endif //__ALT_PATH_METHOD_3__
	//int			debug_max_threads = 0;

	if (gWPNum <= 0) return -1;

	if (!PATHING_IGNORE_FRAME_TIME && trap->Milliseconds() - FRAME_TIME > 300)
	{// Never path on an already long frame time...
		return -1;
	}

	if ((from == NODE_INVALID) || (to == NODE_INVALID) || (from >= gWPNum) || (to >= gWPNum) || (from == to))
	{
		//trap->Print("Bad from or to node.\n");
		return (-1);
	}

#pragma omp critical (__ASTAR_MUTEX__)
	{
		// Check if memory needs to be allocated...
		AllocatePathFindingMemory();

		// Init waypoint link costs if needed...
		ASTAR_InitWaypointCosts();

		memset(openlist, 0, (sizeof(int)* (gWPNum + 1)));
		memset(gcost, 0, (sizeof(float)* gWPNum));
		memset(fcost, 0, (sizeof(int)* gWPNum));
		memset(list, 0, (sizeof(char)* gWPNum));
		memset(parent, 0, (sizeof(int)* gWPNum));

		for (i = 0; i < gWPNum; i++)
		{
			gcost[i] = Distance(gWPArray[i]->origin, gWPArray[to]->origin);

			if (gWPArray[i]->flags & WPFLAG_WATER)
			{// Get out of water ASAP!
				gcost[i] *= 1.1;
			}

			if (gWPArray[i]->flags & WPFLAG_ROAD)
			{// Roads...
				gcost[i] *= 0.9;
			}
		}

		openlist[gWPNum + 1] = 0;

		openlist[1] = from;																	//add the starting node to the open list
		numOpen++;
		gcost[from] = 0;																	//its f and g costs are obviously 0
		fcost[from] = 0;

#ifdef __ALT_PATH_METHOD_1__
		if (altPath)
		{// Mark some locations as bad to alter the path...
			for (i = irand(0, gWPNum*0.2); i < gWPNum; i += irand(1 + (gWPNum*0.1), gWPNum*0.3))
			{
				if (Distance(gWPArray[i]->origin, gWPArray[from]->origin) > 384/*256*/
					&& Distance(gWPArray[i]->origin, gWPArray[to]->origin) > 384/*256*/)
				{
					IGNORE_AREAS[IGNORE_AREAS_NUM] = i;
					IGNORE_AREAS_NUM++;
				}

				//if (IGNORE_AREAS_NUM >= 16) break;
				if (IGNORE_AREAS_NUM >= 8) break;
			}
		}
#endif //__ALT_PATH_METHOD_1__

#ifdef __ALT_PATH_METHOD_3__
		if (altPath)
		{// Mark a location as bad to alter the path...
			int tries = 0;
			int divider = 1;

			while (1)
			{
				int choice = irand_big(0, gWPNum - 1);

				if (tries > 5)
				{// Reduce range check so that we never hit an endless loop...
					divider++;
					tries = 0;
				}

				if (Distance(gWPArray[choice]->origin, gWPArray[from]->origin) <= IGNORE_RANGE / divider
					|| Distance(gWPArray[choice]->origin, gWPArray[to]->origin) <= IGNORE_RANGE / divider)
				{// Make sure this is not too close to my, or my target's location...
					//trap->Print("Range %i. Divider %i. Tries %i.\n", IGNORE_RANGE / divider, divider, tries);
					tries++;
					continue;
				}

				IGNORE_AREA = choice;
				IGNORE_RANGE /= divider;
				break;
			}
		}
#endif //__ALT_PATH_METHOD_3__

		while (1)
		{
			if (numOpen != 0)																//if there are still items in the open list
			{
				//pop the top item off of the list
				atNode = openlist[1];
				list[atNode] = 2;															//put the node on the closed list so we don't check it again
				numOpen--;
				openlist[1] = openlist[numOpen + 1];										//move the last item in the list to the top position
				v = 1;

				//this while loop reorders the list so that the new lowest fcost is at the top again
				while (1)
				{
					u = v;
					if ((2 * u + 1) < numOpen)											//if both children exist
					{
						if (fcost[openlist[u]] >= fcost[openlist[2 * u]])
						{
							v = 2 * u;
						}

						if (fcost[openlist[v]] >= fcost[openlist[2 * u + 1]])
						{
							v = 2 * u + 1;
						}
					}
					else
					{
						if ((2 * u) < numOpen)											//if only one child exists
						{
							if (fcost[openlist[u]] >= fcost[openlist[2 * u]])
							{
								v = 2 * u;
							}
						}
					}

					if (u != v)															//if they're out of order, swap this item with its parent
					{
						temp = openlist[u];
						openlist[u] = openlist[v];
						openlist[v] = temp;
					}
					else
					{
						break;
					}
				}

				for (i = 0; i < gWPArray[atNode]->neighbornum && i < MAX_NODELINKS; i++)								//loop through all the links for this node
				{
					newnode = gWPArray[atNode]->neighbors[i].num;

					if (newnode >= gWPNum)
						continue;

					if (newnode < 0)
						continue;

					if (list[newnode] == 2)
					{																		//if this node is on the closed list, skip it
						continue;
					}

#ifdef __ALT_PATH_METHOD_1__
					if (altPath)
					{// Doing an alt path. Check this waypoint is not too close to a marked random location...
						int			badWP = 0;
						qboolean	bad = qfalse;

						for (badWP = 0; badWP < IGNORE_AREAS_NUM; badWP++)
						{
							if (Distance(gWPArray[i]->origin, gWPArray[IGNORE_AREAS[bad]]->origin) <= 384/*256*/)
							{// Too close to bad location...
								bad = qtrue;
								break;
							}
						}

						if (bad)
						{// This wp is too close to a marked bad location. Ignore it...
							continue;
						}
					}
#endif //__ALT_PATH_METHOD_1__

#ifdef __ALT_PATH_METHOD_3__
					if (altPath)
					{// Doing an alt path. Check this waypoint is not too close to a marked random location...
						if (Distance(gWPArray[i]->origin, gWPArray[IGNORE_AREA]->origin) <= IGNORE_RANGE)
						{// Too close to bad location...
							continue;
						}
					}
#endif //__ALT_PATH_METHOD_3__

					if (list[newnode] != 1)												//if this node is not already on the open list
					{
						openlist[++numOpen] = newnode;										//add the new node to the open list
						list[newnode] = 1;
						parent[newnode] = atNode;											//record the node's parent

						if (newnode == to)
						{																	//if we've found the goal, don't keep computing paths!
							break;															//this will break the 'for' and go all the way to 'if (list[to] == 1)'
						}

#ifdef __ALT_PATH_METHOD_2__
						if (altPath)
						{// Let's try simply adding random multiplier to costs...
							if (gWPArray[atNode]->neighbors[i].forceJumpTo) // But still always hate jumping...
								fcost[newnode] = ASTAR_GetFCost(to, newnode, parent[newnode], i, gcost) * irand(3, 5);	//store it's f cost value
							else
								fcost[newnode] = ASTAR_GetFCost(to, newnode, parent[newnode], i, gcost) * irand(1, 3);	//store it's f cost value
						}
						else
#endif //__ALT_PATH_METHOD_2__
							fcost[newnode] = ASTAR_GetFCost(to, newnode, parent[newnode], i, gcost);	//store it's f cost value

						/*if (fcost[newnode] <= 0 && newnode != from)
						{
							trap->Print("ASTAR WARNING: Missing fcost for node %i. This should not happen!\n", newnode);
						}*/

						//this loop re-orders the heap so that the lowest fcost is at the top
						m = numOpen;

						while (m != 1)													//while this item isn't at the top of the heap already
						{
							if (fcost[openlist[m]] <= fcost[openlist[m / 2]])				//if it has a lower fcost than its parent
							{
								temp = openlist[m / 2];
								openlist[m / 2] = openlist[m];
								openlist[m] = temp;											//swap them
								m /= 2;
							}
							else
							{
								break;
							}
						}
					}
					else										//if this node is already on the open list
					{
						gc = gcost[atNode];

#if 1
						float linkCost = gWPArray[atNode]->neighbors[i].cost;
#else
						if (gWPArray[atNode]->neighbors[i].cost > 0)
						{// UQ1: Already have a cost value, skip the calculations!
							linkCost = gWPArray[atNode]->neighbors[i].cost;
						}
						else
						{
							vec3_t	vec;

							VectorSubtract(gWPArray[newnode]->origin, gWPArray[atNode]->origin, vec);
							linkCost = VectorLength(vec);				//calculate what the gcost would be if we reached this node along the current path
							gWPArray[atNode]->neighbors[i].cost = VectorLength(vec);

							if (gWPArray[atNode]->neighbors[i].forceJumpTo > 0)
								gWPArray[atNode]->neighbors[i].cost *= 5.0;

							//trap->Print("ASTAR WARNING: Missing cost for node %i neighbour %i. This should not happen!\n", atNode, i);
						}
#endif

#ifdef __ALT_PATH_METHOD_2__
						if (altPath)
						{// Let's try simply adding random multiplier to costs...
							if (gWPArray[atNode]->neighbors[i].forceJumpTo) // But still always hate jumping...
								linkCost *= irand(4, 16);	//store it's f cost value
							else
								linkCost *= irand(1, 8);	//store it's f cost value
						}
#endif //__ALT_PATH_METHOD_2__

						gc += linkCost;

						if (gc < gcost[newnode])				//if the new gcost is less (ie, this path is shorter than what we had before)
						{
							parent[newnode] = atNode;			//set the new parent for this node
							gcost[newnode] = gc;				//and the new g cost

							for (j = 1; j < numOpen; j++)		//loop through all the items on the open list
							{
								if (openlist[j] == newnode)	//find this node in the list
								{
									//calculate the new fcost and store it
#ifdef __ALT_PATH_METHOD_2__
									if (altPath)
									{// Let's try simply adding random multiplier to costs...
										if (gWPArray[atNode]->neighbors[i].forceJumpTo) // But still always hate jumping...
											fcost[newnode] = ASTAR_GetFCost(to, newnode, parent[newnode], i, gcost) * irand(4, 8);	//store it's f cost value
										else
											fcost[newnode] = ASTAR_GetFCost(to, newnode, parent[newnode], i, gcost) * irand(1, 3);	//store it's f cost value
									}
									else
#endif //__ALT_PATH_METHOD_2__
										fcost[newnode] = ASTAR_GetFCost(to, newnode, parent[newnode], i, gcost);

									//reorder the list again, with the lowest fcost item on top
									m = j;

									while (m != 1)
									{
										if (fcost[openlist[m]] < fcost[openlist[m / 2]])	//if the item has a lower fcost than it's parent
										{
											temp = openlist[m / 2];
											openlist[m / 2] = openlist[m];
											openlist[m] = temp;								//swap them
											m /= 2;
										}
										else
										{
											break;
										}
									}
									break;													//exit the 'for' loop because we already changed this node
								}															//if
							}																//for
						}											//if (gc < gcost[newnode])
					}												//if (list[newnode] != 1) --> else
				}													//for (loop through links)
			}														//if (numOpen != 0)
			else
			{
				found = qfalse;										//there is no path between these nodes
				break;
			}

			if (list[to] == 1)									//if the destination node is on the open list, we're done
			{
				found = qtrue;
				break;
			}
		}															//while (1)
	}

	if (found == qtrue)							//if we found a path, and are trying to store the pathlist...
	{
		count = 0;
		temp = to;												//start at the end point

		while (temp != from)									//travel along the path (backwards) until we reach the starting point
		{
			if (count + 1 >= MAX_WPARRAY_SIZE)
			{
				trap->Print("ASTAR WARNING: pathlist count > MAX_WPARRAY_SIZE.\n");
				return -1; // UQ1: Added to stop crash if path is too long for the memory allocation...
			}

			pathlist[count++] = temp;							//add the node to the pathlist and increment the count
			temp = parent[temp];								//move to the parent of this node to continue the path
		}

		pathlist[count++] = from;								//add the beginning node to the end of the pathlist

		// Debug output the pathlist...
		/*trap->Print("Pathsize is [%i]. Path [", count);
		for (i = 0; i < count; i++)
		{
			trap->Print(" %i", pathlist[i]);
		}
		trap->Print(" ]\n");*/

		return (count);
	}

	//trap->Print("Failed to find path.\n");
	return (-1);											//return the number of nodes in the path, -1 if not found
}

extern int BG_GetTime();
extern int DOM_GetBestWaypoint(vec3_t org, int ignore, int badwp);

void AIMod_TimeMapPaths()
{
	int			startTime = trap->Milliseconds();
	/*short*/ int	pathlist[MAX_WPARRAY_SIZE];
	int			pathsize;
	gentity_t	*ent = NULL;
	int			i, j;
	int			current_wp, longTermGoal;
	int			NUM_PATHS = 0;
	int			PATH_DISTANCES[MAX_GENTITIES];
	int			TOTAL_DISTANCE = 0;
	int			AVERAGE_DISTANCE = 0;

	ent = G_Find(ent, FOFS(classname), "info_player_deathmatch");

	if (!ent)
		trap->Print("No spawnpoint found!\n");

	current_wp = DOM_GetBestWaypoint(ent->r.currentOrigin, -1, -1);

	if (!current_wp)
		trap->Print("No waypoint found!\n");

	trap->Print("Finding bot objectives at node number %i (%f %f %f).\n",
		current_wp, gWPArray[current_wp]->origin[0], gWPArray[current_wp]->origin[1],
		gWPArray[current_wp]->origin[2]);

	PATHING_IGNORE_FRAME_TIME = qtrue;

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		gentity_t	*goal = &g_entities[i];

		if (!goal || !goal->inuse) continue;

		if (!goal->classname
			|| !goal->classname[0]
			|| !Q_stricmp(goal->classname, "freed")
			|| !Q_stricmp(goal->classname, "noclass"))
			continue;

		if (i == ent->s.number) continue;

		longTermGoal = DOM_GetBestWaypoint(goal->s.origin, -1, -1);

		pathsize = ASTAR_FindPathFast(current_wp, longTermGoal, pathlist, qfalse);

		if (pathsize > 0)
		{
			int j;

			PATH_DISTANCES[NUM_PATHS] = 0;

			for (j = 0; j < pathsize - 1; j++)
			{
				PATH_DISTANCES[NUM_PATHS] += Distance(gWPArray[pathlist[j]]->origin, gWPArray[pathlist[j + 1]]->origin);
			}

			NUM_PATHS++;

			trap->Print("Objective %i (%s) pathsize is %i.\n", i, goal->classname, pathsize);
		}
	}

	for (j = 0; j < NUM_PATHS; j++)
	{
		TOTAL_DISTANCE += PATH_DISTANCES[j];
	}

	AVERAGE_DISTANCE = TOTAL_DISTANCE / NUM_PATHS;

	trap->Print("Completed %i paths in %i seconds. Average path distance is %i\n", NUM_PATHS, (int)((int)(trap->Milliseconds() - startTime) / 1000), AVERAGE_DISTANCE);

	PATHING_IGNORE_FRAME_TIME = qfalse;
}

#endif //__DOMINANCE_AI__
