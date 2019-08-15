#include "g_local.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"
#include "ai_dominance_main.h"

// Disable stupid warnings...
#pragma warning( disable : 4996 )

int gWPNum = 0;
wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

//
// Village/Town/City Stuff...
//
int villageWaypointsNum = 0;
int villageWaypoints[MAX_WPARRAY_SIZE] = { -1 };

int wildernessWaypointsNum = 0;
int wildernessWaypoints[MAX_WPARRAY_SIZE] = { -1 };

int		MAP_NUM_VILLAGES = 0;
float	MAP_VILLAGE_RADIUSES[4] = { 0.0 };
vec3_t	MAP_VILLAGE_ORIGINS[4] = { 0.0 };
float	MAP_VILLAGE_BUFFER = 1024.0;

void G_LoadVillageData(void)
{
	vmCvar_t	mapname;
	float		radius = 0.0;

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_ROM | CVAR_SERVERINFO);

	radius = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "cityRadius", "0.0"));

	if (radius > 0.0)
	{
		MAP_VILLAGE_RADIUSES[0] = radius;
		MAP_VILLAGE_ORIGINS[0][0] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "cityLocationX", "0.0"));
		MAP_VILLAGE_ORIGINS[0][1] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "cityLocationY", "0.0"));
		MAP_VILLAGE_ORIGINS[0][2] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "cityLocationZ", "0.0"));
		MAP_NUM_VILLAGES++;
	}
	
	radius = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city2Radius", "0.0"));

	if (radius > 0.0)
	{
		MAP_VILLAGE_RADIUSES[MAP_NUM_VILLAGES] = radius;
		MAP_VILLAGE_ORIGINS[MAP_NUM_VILLAGES][0] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city2LocationX", "0.0"));
		MAP_VILLAGE_ORIGINS[MAP_NUM_VILLAGES][1] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city2LocationY", "0.0"));
		MAP_VILLAGE_ORIGINS[MAP_NUM_VILLAGES][2] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city2LocationZ", "0.0"));
		MAP_NUM_VILLAGES++;
	}

	radius = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city3Radius", "0.0"));

	if (radius > 0.0)
	{
		MAP_VILLAGE_RADIUSES[MAP_NUM_VILLAGES] = radius;
		MAP_VILLAGE_ORIGINS[MAP_NUM_VILLAGES][0] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city3LocationX", "0.0"));
		MAP_VILLAGE_ORIGINS[MAP_NUM_VILLAGES][1] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city3LocationY", "0.0"));
		MAP_VILLAGE_ORIGINS[MAP_NUM_VILLAGES][2] = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "city3LocationZ", "0.0"));
		MAP_NUM_VILLAGES++;
	}

	MAP_VILLAGE_BUFFER = atof(IniRead(va("maps/%s.climate", mapname.string), "CITY", "cityBuffer", "1024.0"));
}

qboolean G_PointIsInVillage (vec3_t point, qboolean addBuffer)
{
	for (int i = 0; i < MAP_NUM_VILLAGES; i++)
	{
		float maxDist = MAP_VILLAGE_RADIUSES[i];

		if (addBuffer) maxDist += MAP_VILLAGE_BUFFER;

		if (Distance(point, MAP_VILLAGE_ORIGINS[i]) <= maxDist)
			return qtrue;
	}

	return qfalse;
}

int G_SelectVillageSpawnpoint(void)
{
	int			waypoint;

	if (villageWaypointsNum > 0)
	{// Find a village waypoint...
		waypoint = irand_big(0, villageWaypointsNum - 1);
		waypoint = villageWaypoints[waypoint];
	}
	else
	{// No villages/towns/cities, select any wp...
		waypoint = irand_big(0, gWPNum - 1);
	}

	return waypoint;
}

int G_SelectWildernessSpawnpoint(void)
{
	int			waypoint;

	if (wildernessWaypointsNum > 0)
	{// Find a wilderness waypoint...
		waypoint = irand_big(0, wildernessWaypointsNum - 1);
		waypoint = wildernessWaypoints[waypoint];
	}
	else
	{// No villages/towns/cities, select any wp...
		waypoint = irand_big(0, gWPNum - 1);
	}

	return waypoint;
}

//
// Waypointing stuff...
//

void G_TestLine(vec3_t start, vec3_t end, int color, int time)
{
	gentity_t *te;

	te = G_TempEntity( start, EV_TESTLINE );
	VectorCopy(start, te->s.origin);
	VectorCopy(end, te->s.origin2);
	te->s.time2 = time;
	te->s.weapon = color;
	te->r.svFlags |= SVF_BROADCAST;
}

//
// UQ1: Autowaypoint nodes to JKA waypoint conversion...
//

//extern qboolean AIMOD_LoadCoverPoints ( void );

#define		MOD_DIRECTORY "Warzone"
#define		BOT_MOD_NAME	"aimod"
float		NOD_VERSION = 1.1f;

void CreateNewWP_FromAWPNode(int index, vec3_t origin, int flags, int weight, int associated_entity, float disttonext, int forceJumpTo, int num_links, int *links, int *link_flags)
{
	int i;

	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		return;
	}

	if (!gWPArray[gWPNum])
	{
		gWPArray[gWPNum] = (wpobject_t *)G_Alloc(sizeof(wpobject_t), "CreateNewWP_FromAWPNode");
	}

	if (!gWPArray[gWPNum])
	{
		trap->Print(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
	}

	//trap->Print("DEBUG: Adding waypoint (%i) %i at %f %f %f with %i links.\n", gWPNum, index, origin[0], origin[1], origin[2], num_links);

	gWPArray[gWPNum]->flags = flags;
	//gWPArray[gWPNum]->weight = weight;
	//gWPArray[gWPNum]->associated_entity = associated_entity;
	//gWPArray[gWPNum]->disttonext = disttonext;
	//gWPArray[gWPNum]->forceJumpTo = forceJumpTo;
	gWPArray[gWPNum]->index = index;
	gWPArray[gWPNum]->inuse = true;
	VectorCopy(origin, gWPArray[gWPNum]->origin);
	gWPArray[gWPNum]->neighbornum = num_links;
	
	i = num_links;

	while (i >= 0)
	{
		gWPArray[gWPNum]->neighbors[i].num = links[i];
		
#define	NODE_JUMP					1024

		if (link_flags[i] & NODE_JUMP)
			gWPArray[gWPNum]->neighbors[i].forceJumpTo = 1;
		else
			gWPArray[gWPNum]->neighbors[i].forceJumpTo = 0;
		
		i--;
	}

	if (gWPArray[gWPNum]->flags & WPFLAG_RED_FLAG)
	{
		flagRed = gWPArray[gWPNum];
		oFlagRed = flagRed;
	}
	else if (gWPArray[gWPNum]->flags & WPFLAG_BLUE_FLAG)
	{
		flagBlue = gWPArray[gWPNum];
		oFlagBlue = flagBlue;
	}

	//trap->Print("Added WP %i at %f %f %f. It has %i links.\n", gWPNum, gWPArray[gWPNum]->origin[0], gWPArray[gWPNum]->origin[1], 
	//	gWPArray[gWPNum]->origin[2], gWPArray[gWPNum]->neighbornum);

	if (G_PointIsInVillage(gWPArray[gWPNum]->origin, qfalse))
	{// Mark any waypoints within the village/town/city area... Ignore the buffer so that villagers stay within the shield...
		gWPArray[gWPNum]->wpIsInVillage = true;

		villageWaypoints[villageWaypointsNum] = gWPNum;
		villageWaypointsNum++;
	}
	else
	{
		gWPArray[gWPNum]->wpIsInVillage = false;

		if (!G_PointIsInVillage(gWPArray[gWPNum]->origin, qtrue))
		{// Only add the waypoints not in the village outer buffer range to the wilderness list...
			wildernessWaypoints[wildernessWaypointsNum] = gWPNum;
			wildernessWaypointsNum++;
		}
	}

	gWPNum++;
}

// UQ1: These are AWP node flag types...
#define NODE_MOVE					0       // Move Node
#define NODE_OBJECTIVE				1
#define NODE_TARGET					2
#define NODE_LAND_VEHICLE			4
#define NODE_FASTHOP				8
#define NODE_COVER					16
#define NODE_WATER					32
#define NODE_LADDER					64      // Ladder Node
#define	NODE_ROAD					128
#define	NODE_DYNAMITE				256
#define	NODE_BUILD					512
#define	NODE_JUMP					1024
#define	NODE_DUCK					2048
#define	NODE_ICE					4096	// Node is located on ice (slick)...
#define NODE_ALLY_UNREACHABLE		8192
#define NODE_AXIS_UNREACHABLE		16384
#define	NODE_AXIS_DELIVER			32768	//place axis should deliver stolen documents/objective
#define	NODE_ALLY_DELIVER			65536	//place allies should deliver stolen documents/objective

int Convert_AWP_Flags ( int flags )
{
	int out_flags = 0;

/*
#define WPFLAG_JUMP					0x00000010 //jump when we hit this
#define WPFLAG_DUCK					0x00000020 //duck while moving around here
#define WPFLAG_NOVIS				0x00000400 //go here for a bit even with no visibility
#define WPFLAG_SNIPEORCAMPSTAND		0x00000800 //a good position to snipe or camp - stand
#define WPFLAG_WAITFORFUNC			0x00001000 //wait for a func brushent under this point before moving here
#define WPFLAG_SNIPEORCAMP			0x00002000 //a good position to snipe or camp - crouch
#define WPFLAG_ONEWAY_FWD			0x00004000 //can only go forward on the trial from here (e.g. went over a ledge)
#define WPFLAG_ONEWAY_BACK			0x00008000 //can only go backward on the trail from here
#define WPFLAG_GOALPOINT			0x00010000 //make it a goal to get here.. goal points will be decided by setting "weight" values
#define WPFLAG_RED_FLAG				0x00020000 //red flag
#define WPFLAG_BLUE_FLAG			0x00040000 //blue flag
#define WPFLAG_SIEGE_REBELOBJ		0x00080000 //rebel siege objective
#define WPFLAG_SIEGE_IMPERIALOBJ	0x00100000 //imperial siege objective
#define WPFLAG_NOMOVEFUNC			0x00200000 //don't move over if a func is under
#define WPFLAG_CALCULATED			0x00400000 //don't calculate it again
#define WPFLAG_NEVERONEWAY			0x00800000 //never flag it as one-way
//[TABBot]
#define WPFLAG_DESTROY_FUNCBREAK	0x01000000 //destroy all the func_breakables in the area
												//before moving to this waypoint
#define WPFLAG_REDONLY				0x02000000 //only bots on the red team will be able to
												//use this waypoint
#define WPFLAG_BLUEONLY				0x04000000 //only bots on the blue team will be able to
												//use this waypoint
#define WPFLAG_FORCEPUSH			0x08000000 //force push all the active func_doors in the
												//area before moving to this waypoint.
#define WPFLAG_FORCEPULL			0x10000000 //force pull all the active func_doors in the
												//area before moving to this waypoint.			
												#define WPFLAG_COVER				0x20000000 //cover point

												#define WPFLAG_WATER				0x40000000 //water point

												#define WPFLAG_ROAD					0x80000000 //road
//[/TABBot]
*/

	if (flags & NODE_FASTHOP)
		out_flags |= WPFLAG_JUMP;

	if (flags & NODE_WATER)
		out_flags |= WPFLAG_WATER;

	if (flags & NODE_JUMP)
		out_flags |= WPFLAG_JUMP;

	if (flags & NODE_DUCK)
		out_flags |= WPFLAG_DUCK;

	if (flags & NODE_AXIS_DELIVER)
		out_flags |= WPFLAG_SIEGE_IMPERIALOBJ;

	if (flags & NODE_ALLY_DELIVER)
		out_flags |= WPFLAG_SIEGE_REBELOBJ;

	if (flags & NODE_ROAD)
		out_flags |= WPFLAG_ROAD;

	//if (flags & NODE_ALLY_UNREACHABLE)
	//	out_flags |= WPFLAG_REDONLY;

	//if (flags & NODE_AXIS_UNREACHABLE)
	//	out_flags |= WPFLAG_BLUEONLY;

	return out_flags;
}

/* */
qboolean
AIMOD_NODES_LoadNodes2 ( void )
{
	FILE			*f;
	int				i, j;
	//char			filename[600];
	vmCvar_t		mapname, fs_homepath, fs_game;
	char			name[] = BOT_MOD_NAME;
	short int		objNum[3] = { 0, 0, 0 },
	objFlags, numLinks;
	int				flags;
	vec3_t			vec;
	short int		fl2;
	int				target;
	char			nm[64] = "";
	float			version;
	char			map[64] = "";
	char			mp[64] = "";
	/*short*/ int		numberNodes;
	short int		temp, fix_aas_nodes;

	i = 0;

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	trap->Cvar_Register( &fs_homepath, "fs_homepath", "", CVAR_SERVERINFO | CVAR_ROM );
	trap->Cvar_Register( &fs_game, "fs_game", "", CVAR_SERVERINFO | CVAR_ROM );
	
	f = fopen( va("%s/%s/nodes/%s.bwp", fs_homepath.string, fs_game.string, mapname.string), "rb" );

	if ( !f )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", mapname.string );
		trap->Print( "^1*** ^3       ^5  You need to make bot routes for this map.\n" );
		trap->Print( "^1*** ^3       ^5  Bots will move randomly for this map.\n" );
		return qfalse;
	}

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_ROM | CVAR_SERVERINFO );	//get the map name
	strcpy( mp, mapname.string );

	fread( &nm, strlen( name) + 1, 1, f);

	fread( &version, sizeof(float), 1, f);
	if ( version != NOD_VERSION && version != 1.0f)
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", mapname.string );
		trap->Print( "^1*** ^3       ^5  Old node file detected.\n" );
		fclose(f);
		return qfalse;
	}

	fread( &map, strlen( mp) + 1, 1, f);
	
	if ( Q_stricmp( map, mp) != 0 )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", mapname.string );
		trap->Print( "^1*** ^3       ^5  Node file is not for this map!\n" );
		fclose(f);
		return qfalse;
	}

	if (version == NOD_VERSION)
	{
		fread( &numberNodes, sizeof(/*short*/ int), 1, f);
	}
	else
	{
		fread( &temp, sizeof(short int), 1, f);
		numberNodes = temp;
	}

	for ( i = 0; i < numberNodes; i++ )					//loop through all the nodes
	{
		int links[32];
		int link_flags[32];
		int new_flags = 0;

		for (j = 0; j < 32; j++)
		{
			links[j] = -1;
			link_flags[j] = -1;
		}

		//read in all the node info stored in the file
		fread( &vec, sizeof(vec3_t), 1, f);
		fread( &flags, sizeof(int), 1, f);
		fread( objNum, (sizeof(short int) * 3), 1, f);
		fread( &objFlags, sizeof(short int), 1, f);
		fread( &numLinks, sizeof(short int), 1, f);

		//Load_AddNode( vec, flags, objNum, objFlags );	//add the node

		//loop through all of the links and read the data
		for ( j = 0; j < numLinks; j++ )
		{
			if (version == NOD_VERSION)
			{
				fread( &target, sizeof(/*short*/ int), 1, f);
			}
			else
			{
				fread( &temp, sizeof(short int), 1, f);
				target = temp;
			}

			fread( &fl2, sizeof(short int), 1, f);
			//ConnectNodes( i, target, fl2 );				//add any links
			links[j] = target;
			link_flags[j] = fl2;
		}

		new_flags = Convert_AWP_Flags(flags);
		CreateNewWP_FromAWPNode(i, vec, new_flags, 1/*weight*/, -1/*associated_entity*/, 64.0f/*disttonext*/, -1, numLinks, links, link_flags);
	}

	fread( &fix_aas_nodes, sizeof(short int), 1, f);

	fclose(f);
	trap->Print( "^1*** ^3%s^5: Successfully loaded ^7%i^5 waypoints from advanced waypoint file ^7nodes/%s.bwp^5.\n", "NAVIGATION",
			  numberNodes, mapname.string );

	//nodes_loaded = qtrue;

	//AIMOD_LoadCoverPoints();
	return qtrue;
}

/* */
qboolean
AIMOD_NODES_LoadNodes ( void )
{
	fileHandle_t	f;
	int				i, j;
	char			filename[60];
	vmCvar_t		mapname;
	short int		objNum[3] = { 0, 0, 0 },
	objFlags, numLinks;
	int				flags;
	vec3_t			vec;
	short int		fl2;
	int				target;
	char			name[] = BOT_MOD_NAME;
	char			nm[64] = "";
	float			version;
	char			map[64] = "";
	char			mp[64] = "";
	/*short*/ int		numberNodes;
	short int		temp, fix_aas_nodes;

	gWPNum = 0;

	i = 0;
	strcpy( filename, "nodes/" );

	////////////////////
	trap->Cvar_VariableStringBuffer( "g_scriptName", filename, sizeof(filename) );
	if ( filename[0] != '\0' )
	{
		trap->Cvar_Register( &mapname, "g_scriptName", "", CVAR_ROM );
	}
	else
	{
		trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	}

	Q_strcat( filename, sizeof(filename), mapname.string );

	///////////////////
	//open the node file for reading, return false on error
	trap->FS_Open( va( "nodes/%s.bwp", filename), &f, FS_READ );
	if ( !f )
	{
		trap->FS_Close( f );
		return AIMOD_NODES_LoadNodes2();
	}

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_ROM | CVAR_SERVERINFO );	//get the map name
	strcpy( mp, mapname.string );
	trap->FS_Read( &nm, strlen( name) + 1, f );									//read in a string the size of the mod name (+1 is because all strings end in hex '00')
	trap->FS_Read( &version, sizeof(float), f );			//read and make sure the version is the same

	if ( version != NOD_VERSION && version != 1.0f )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", filename );
		trap->Print( "^1*** ^3       ^5  Old node file detected.\n" );
		trap->FS_Close( f );
		return qfalse;
	}

	trap->FS_Read( &map, strlen( mp) + 1, f );			//make sure the file is for the current map
	
	if ( Q_stricmp( map, mp) != 0 )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", filename );
		trap->Print( "^1*** ^3       ^5  Node file is not for this map!\n" );
		trap->FS_Close( f );
		return qfalse;
	}

	if (version == NOD_VERSION)
	{
		trap->FS_Read( &numberNodes, sizeof(/*short*/ int), f ); //read in the number of nodes in the map
	}
	else
	{
		trap->FS_Read( &temp, sizeof(short int), f ); //read in the number of nodes in the map
		numberNodes = temp;
	}

	for ( i = 0; i < numberNodes; i++ )					//loop through all the nodes
	{
		int links[32];
		int link_flags[32];
		int new_flags = 0;

		for (j = 0; j < 32; j++)
		{
			links[j] = -1;
			link_flags[j] = -1;
		}

		//read in all the node info stored in the file
		trap->FS_Read( &vec, sizeof(vec3_t), f );
		trap->FS_Read( &flags, sizeof(int), f );
		trap->FS_Read( objNum, sizeof(short int) * 3, f );
		trap->FS_Read( &objFlags, sizeof(short int), f );
		trap->FS_Read( &numLinks, sizeof(short int), f );

		//Load_AddNode( vec, flags, objNum, objFlags );	//add the node

		//loop through all of the links and read the data
		for ( j = 0; j < numLinks; j++ )
		{
			if (version == NOD_VERSION)
			{
				trap->FS_Read( &target, sizeof(/*short*/ int), f );
			}
			else
			{
				trap->FS_Read( &temp, sizeof(short int), f );
				target = temp;
			}

			trap->FS_Read( &fl2, sizeof(short int), f );
			//ConnectNodes( i, target, fl2 );				//add any links
			links[j] = target;
			link_flags[j] = fl2;
		}

		// Set node objective flags..
		//AIMOD_NODES_SetObjectiveFlags( i );
		new_flags = Convert_AWP_Flags(flags);
		CreateNewWP_FromAWPNode(i, vec, new_flags, 1/*weight*/, -1/*associated_entity*/, 64.0f/*disttonext*/, -1, numLinks, links, link_flags);
	}

	trap->FS_Read( &fix_aas_nodes, sizeof(short int), f );
	trap->FS_Close( f );							//close the file
	trap->Print( "^1*** ^3%s^5: Successfully loaded ^7%i^5 waypoints from advanced waypoint file ^7nodes/%s.bwp^5.\n", "NAVIGATION",
			  numberNodes, filename );
	//nodes_loaded = qtrue;

	//AIMOD_LoadCoverPoints();
	return qtrue;
}

extern qboolean Warzone_CheckRoutingFrom( int wp );
extern qboolean Warzone_CheckBelowWaypoint( int wp );
extern qboolean PATHING_IGNORE_FRAME_TIME;

void Warzone_WaypointCheck ( void )
{
/*
	//
	// Checks auto-waypointed files waypoints for a path to a spawnpoint... Disables any waypoints with no valid path... Also checks for bad surfaces...
	//

	int i = 0;
	int NUM_GOOD = 0;
	int NUM_BAD = 0;

	PATHING_IGNORE_FRAME_TIME = qtrue;

	for (i = 0; i < gWPNum; i++)
	{
		qboolean wpOK = qtrue;

		gWPArray[i]->wpIsBadChecked = false;
		gWPArray[i]->wpIsBad = false;
		gWPArray[i]->inuse = true;

		wpOK = Warzone_CheckRoutingFrom( i );
		if (wpOK) wpOK = Warzone_CheckBelowWaypoint( i );

		if (wpOK)
		{
			// Found a route to spawnpoint. Mark it as good...
			gWPArray[i]->inuse = true;
			gWPArray[i]->wpIsBadChecked = true;
			gWPArray[i]->wpIsBad = false;
			NUM_GOOD++;
		}
		else
		{
			// No route to spawnpoint. Mark it as bad...
			gWPArray[i]->inuse = false;
			gWPArray[i]->wpIsBadChecked = true;
			gWPArray[i]->wpIsBad = true;
			NUM_BAD++;
		}
	}

	PATHING_IGNORE_FRAME_TIME = qfalse;

	trap->Print( "^1*** ^3WAYPOINT REACHABILITY CHECK^5: Total %i waypoints. %i waypoints marked GOOD and %i waypoints marked BAD.\n", 
			  gWPNum, NUM_GOOD, NUM_BAD );
*/

	trap->Print("^1*** ^3%s^5: Map has ^7%i^5 village waypoints (in ^7%i^5 villages) and ^7%i^5 wilderness waypoints.\n", "NAVIGATION", villageWaypointsNum, MAP_NUM_VILLAGES, wildernessWaypointsNum);
}

int num_nav_waypoints = 0;
int nav_waypoints[MAX_WPARRAY_SIZE];

extern vmCvar_t npc_wptonav;

int LoadPathData(const char *filename)
{
	if (AIMOD_NODES_LoadNodes()) 
	{
		trap->Cvar_Register( &npc_wptonav, "npc_wptonav", "0", CVAR_ARCHIVE );

		if (npc_wptonav.integer)
		{
			if (gWPNum > 8000 && npc_wptonav.integer < 2)
			{
				trap->Print("* Have too many waypoints to add them to the nav system. Set npc_wptonav to 2 to force addition.\n");
			}
			else if (trap->Nav_GetNumNodes() < gWPNum)
			{// This nav file does not have all our extra waypoints... Add them now...
				int i = 0, j = 0, k = 0, original_count = 0, new_count = 0;

				original_count = trap->Nav_GetNumNodes();

				Com_Printf("^1*** ^3NAVIGATION^5: Navigation system had ^7%i^5 nodes. Adding warzone nodes.\n", original_count);

				for (i = 0; i < gWPNum; i++)
				{
					nav_waypoints[i] = trap->Nav_AddRawPoint(gWPArray[i]->origin, 0, 64);
					num_nav_waypoints++;
				}

				new_count = trap->Nav_GetNumNodes();

				// Now hard link them as like our normal waypoint array...
				trap->Print("* Navigation system linking %i new nodes.\n", new_count - original_count);

				for (j = 0; j < gWPNum; j++)
				{
					for (k = 0; k < gWPArray[j]->neighbornum; k++)
					{
						trap->Nav_HardConnect(nav_waypoints[j], nav_waypoints[gWPArray[j]->neighbors[k].num]);
					}
				}

				trap->Nav_CalculatePaths(qtrue);
				Com_Printf("^1*** ^3NAVIGATION^5: Navigation system now has ^7%i^5 nodes.\n", new_count);
			}
		}

		Warzone_WaypointCheck();
		Com_Printf("^1*** ^3NAVIGATION^5: Navigation system update completed.\n");
		return 1; // UQ1: Load/Convert Auto-Waypoint Nodes... (Now default)
	}

	Com_Printf("^1*** ^3NAVIGATION^5: Navigation system update completed.\n");
	return 0;
}

void LoadPath_ThisLevel(void)
{
	vmCvar_t	mapname;

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	LoadPathData(mapname.string);
}

gentity_t *GetClosestSpawn(gentity_t *ent)
{
	gentity_t	*spawn;
	gentity_t	*closestSpawn = NULL;
	float		closestDist = -1;
	int			i = MAX_CLIENTS;

	spawn = NULL;

	while (i < level.num_entities)
	{
		spawn = &g_entities[i];

		if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch")) )
		{
			float checkDist;
			vec3_t vSub;

			VectorSubtract(ent->client->ps.origin, spawn->r.currentOrigin, vSub);
			checkDist = VectorLength(vSub);

			if (closestDist == -1 || checkDist < closestDist)
			{
				closestSpawn = spawn;
				closestDist = checkDist;
			}
		}

		i++;
	}

	return closestSpawn;
}

gentity_t *GetNextSpawnInIndex(gentity_t *currentSpawn)
{
	gentity_t	*spawn;
	gentity_t	*nextSpawn = NULL;
	int			i = currentSpawn->s.number+1;

	spawn = NULL;

	while (i < level.num_entities)
	{
		spawn = &g_entities[i];

		if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch")) )
		{
			nextSpawn = spawn;
			break;
		}

		i++;
	}

	if (!nextSpawn)
	{ //loop back around to 0
		i = MAX_CLIENTS;

		while (i < level.num_entities)
		{
			spawn = &g_entities[i];

			if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch")) )
			{
				nextSpawn = spawn;
				break;
			}

			i++;
		}
	}

	return nextSpawn;
}
