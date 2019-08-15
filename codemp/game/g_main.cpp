// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"
#include "g_ICARUScb.h"
#include "g_nav.h"
#include "bg_saga.h"
#include "jkg_damagetypes.h"
#include "ai_dominance_main.h"
#include "b_local.h"
#include <thread>

level_locals_t	level;

int		eventClearTime = 0;
static int navCalcPathTime = 0;
extern int fatalErrors;

int killPlayerTimer = 0;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];
extern int gWPNum;

qboolean gDuelExit = qfalse;

void G_InitGame					( int levelTime, int randomSeed, int restart );
void G_RunFrame					( int levelTime );
void G_ShutdownGame				( int restart );
void CheckExitRules				( void );
void G_ROFF_NotetrackCallback	( gentity_t *cent, const char *notetrack);

extern stringID_table_t setTable[];

qboolean G_ParseSpawnVars( qboolean inSubBSP );
void G_SpawnGEntityFromSpawnVars( qboolean inSubBSP );


qboolean NAV_ClearPathToPoint( gentity_t *self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEntNum );
qboolean NPC_ClearLOS2( gentity_t *ent, const vec3_t end );
int NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask);
qboolean NAV_CheckNodeFailedForEnt( gentity_t *ent, int nodeNum );
qboolean G_EntIsUnlockedDoor( int entityNum );
qboolean G_EntIsDoor( int entityNum );
qboolean G_EntIsBreakable( int entityNum );
qboolean G_EntIsRemovableUsable( int entNum );
void CP_FindCombatPointWaypoints( void );

extern qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=MAX_CLIENTS, e=g_entities+i ; i < level.num_entities ; i++,e++ ) {
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		if (e->r.contents==CONTENTS_TRIGGER)
			continue;//triggers NEVER link up in teams!
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

//	trap->Print ("%i teams with %i entities\n", c, c2);
}

sharedBuffer_t gSharedBuffer;

void WP_SaberLoadParms( void );
void BG_VehicleLoadParms( void );

void G_CacheGametype( void )
{
	// check some things
	if ( g_gametype.string[0] && isalpha( g_gametype.string[0] ) )
	{
		int gt = BG_GetGametypeForString( g_gametype.string );
		if ( gt == -1 )
		{
			trap->Print( "Gametype '%s' unrecognised, defaulting to FFA/Deathmatch\n", g_gametype.string );
			level.gametype = GT_FFA;
		}
		else
			level.gametype = (gametype_t)gt;
	}
	else if ( g_gametype.integer < 0 || level.gametype >= GT_MAX_GAME_TYPE )
	{
		trap->Print( "g_gametype %i is out of range, defaulting to 0\n", level.gametype );
		level.gametype = GT_FFA;
	}
	else
		level.gametype = (gametype_t)atoi( g_gametype.string );

	trap->Cvar_Set( "g_gametype", va( "%i", level.gametype ) );
}

model_scale_list_t model_scale_list[512];

int num_scale_models = -1;
qboolean scale_models_loaded = qfalse;

void Load_Model_Scales( void )
{// Load model scales from external file.
	char *s, *t;
	int len;
	fileHandle_t	f;
	char *buf;
	qboolean alt = qfalse;

	if (scale_models_loaded) return;

	len = trap->FS_Open( "modelscale.cfg\0", &f, FS_READ );

	if ( !f )
	{
		trap->Print("^1*** ^3WARNING^5: No model scale file.\n", "modelscale.cfg");
		return;
	}

	if ( !len )
	{ //empty file
		trap->FS_Close( f );
		return;
	}

	buf = (char *)malloc(len+1);
	memset(buf, 0, len+1);

	if ( buf == NULL )
	{//alloc memory for buffer
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( buf, len, f );
	trap->FS_Close( f );

	Com_Printf("^1*** ^3MODEL-SCALE^5: Loading player model scales database from file ^7%s^5.\n", "modelscale.cfg");

	for (t = s = buf; *t; /* */ ) 
	{
		if (!alt) // Space between first & second options.
			s = strchr(s, ' ' );
		else
			s = strchr(s, '\n');

		if (!s)
			break;

		if (!alt)
		{ // Space between first & second options.
			while (*s == ' ')
				*s++ = 0;
		}
		else
		{	
			while (*s == '\n')
				*s++ = 0;
		}

		if (*t)
		{
			if ( !Q_strncmp( "//", va("%s", t), 2 ) == 0 
				&& strlen(va("%s", t)) > 1)
			{// Not a comment either... Record it in our list...
				if (!alt)
				{// First value...
					strcpy(model_scale_list[num_scale_models].botName, va("%s", t));
					alt = qtrue;
					//G_Printf("^5%s ", model_scale_list[num_scale_models].botName);
				}
				else
				{// The scale value itself...
					model_scale_list[num_scale_models].scale = 100*atof(t);
					num_scale_models++;
					alt = qfalse;
					//G_Printf("Scale %f.\n", atof(t));
				}
			}
		}

		t = s;
	}

	num_scale_models--;

	Com_Printf("^1*** ^3MODEL-SCALE^5: There are ^7%i^5 player model scales in the current database.\n", num_scale_models);

	free(buf);

	//if (num_scale_models > 0)
	scale_models_loaded = qtrue;

	//free(buf);
}

extern void SP_info_player_deathmatch( gentity_t *ent );

/*
qboolean CheckSpawnPosition(vec3_t position)
{
	trace_t		tr;
	vec3_t testPos;
	//vec3_t	mins = {-15, -15, -1};
	//vec3_t	maxs = {15, 15, 48};

	vec3_t	mins = {-96, -96, -1};
	vec3_t	maxs = {96, 96, 64};
	
	VectorCopy(position, testPos);

	testPos[2] += 8.0;

	//trap->Trace( &tr, testPos, NULL, NULL, downPos, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
	trap->Trace( &tr, testPos, mins, maxs, testPos, -1, MASK_PLAYERSOLID, 0, 0, 0 );

	if (tr.startsolid || tr.allsolid)
	{
		return qfalse;
	}

	return qtrue;
}
*/

qboolean WP_CheckInSolid (vec3_t position)
{
	trace_t	trace;
	vec3_t	end, mins, maxs;
	vec3_t pos;

	int contents = CONTENTS_TRIGGER;
	int clipmask = MASK_DEADSOLID;
	
	VectorSet(mins, -15, -15, DEFAULT_MINS_2);
	VectorSet(maxs, 15, 15, DEFAULT_MAXS_2);

	VectorCopy(position, pos);
	pos[2]+=28;
	VectorCopy(pos, end);
	end[2] += mins[2];
	mins[2] = 0;

	trap->Trace(&trace, pos, mins, maxs, end, -1, clipmask, qfalse, 0, 0);
	if(trace.allsolid || trace.startsolid)
	{
		return qtrue;
	}

	if(trace.fraction < 1.0)
	{
		return qtrue;
	}

	return qfalse;
}

extern qboolean Warzone_SpawnpointNearMoverEntityLocation( vec3_t org );

qboolean CheckSpawnPosition( vec3_t position )
{
	trace_t tr;
	vec3_t org, org2;
	
	if (WP_CheckInSolid(position)) return qfalse;

	//return qtrue;

	VectorCopy(position, org);
	VectorCopy(position, org2);
	//org2[2] = -65536.0f;
	org2[2] -= 64.0f;

	trap->Trace( &tr, org, NULL, NULL, org2, -1, MASK_PLAYERSOLID|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_SHOTCLIP|CONTENTS_TRANSLUCENT, 0, qfalse, 0);
	
	/*if ( tr.startsolid )
	{
		trap->Print("Waypoint %i is in solid.\n");
		return qfalse;
	}

	if ( tr.allsolid )
	{
		trap->Print("Waypoint %i is in solid.\n");
		return qfalse;
	}*/

	if ( tr.fraction == 1 )
	{
		//trap->Print("Waypoint %i is too high above ground.\n");
		return qfalse;
	}

	if ( tr.contents & CONTENTS_LAVA )
	{
		//trap->Print("Waypoint %i is in lava.\n");
		return qfalse;
	}
	
	if ( tr.contents & CONTENTS_SLIME )
	{
		//trap->Print("Waypoint %i is in slime.\n");
		return qfalse;
	}

	if ( tr.contents & CONTENTS_TRIGGER )
	{
		//trap->Print("Waypoint %i is in trigger.\n");
		return qfalse;
	}

	if (tr.contents & CONTENTS_WATER)
	{
		//trap->Print("Waypoint %i is in trigger.\n");
		return qfalse;
	}

	if ( (tr.surfaceFlags & SURF_NOMARKS) && (tr.surfaceFlags & SURF_NODRAW) )
	{
		//trap->Print("Waypoint %i is in trigger.\n");
		return qfalse;
	}

	if (Warzone_SpawnpointNearMoverEntityLocation( position ) || Warzone_SpawnpointNearMoverEntityLocation( tr.endpos ))
	{
		//trap->Print("Waypoint %i is too close to mover.\n");
		return qfalse;
	}

	return qtrue;
}

float HeightExageratedDistance ( vec3_t org, vec3_t org2 )
{// Prefer height difference...
	return (Distance(org, org2) * 0.5) + DistanceVertical(org, org2);
}

extern int DOM_GetNearestWP(vec3_t org, int badwp);

qboolean LoadSpawnpointPositions( qboolean IsTeam )
{
	fileHandle_t	f;
	int				i = 0;
	int				NUM_BLUE_POSITIONS = 0;
	int				NUM_RED_POSITIONS = 0;
	vec3_t			BLUE_SPAWNPOINTS[64];
	vec3_t			RED_SPAWNPOINTS[64];

	vmCvar_t	mapname;
	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	
	if (IsTeam)
	{
		vec3_t		blue_angles = { 0 };
		vec3_t		red_angles = { 0 };

		trap->FS_Open( va( "spawnpoints/%s.team_spawnpoints", mapname.string), &f, FS_READ );

		if ( !f )
		{
			return qfalse;
		}

		trap->FS_Read( &NUM_BLUE_POSITIONS, sizeof(int), f );

		for (i = 0; i < NUM_BLUE_POSITIONS; i++)
		{
			trap->FS_Read( &BLUE_SPAWNPOINTS[i], sizeof(vec3_t), f );

			{
				gentity_t *spawnpoint = G_Spawn();
				spawnpoint->classname = "team_CTF_bluespawn";
				VectorCopy(BLUE_SPAWNPOINTS[i], spawnpoint->s.origin);
				VectorCopy(blue_angles, spawnpoint->s.angles);
				spawnpoint->noWaypointTime = 1; // Don't send auto-generated spawnpoints to client...
				SP_info_player_deathmatch( spawnpoint );
			}
		}

		trap->FS_Read( &NUM_RED_POSITIONS, sizeof(int), f );

		for (i = 0; i < NUM_RED_POSITIONS; i++)
		{
			trap->FS_Read( &RED_SPAWNPOINTS[i], sizeof(vec3_t), f );

			{
				gentity_t *spawnpoint = G_Spawn();
				spawnpoint->classname = "team_CTF_redspawn";
				VectorCopy(RED_SPAWNPOINTS[i], spawnpoint->s.origin);
				VectorCopy(red_angles, spawnpoint->s.angles);
				spawnpoint->noWaypointTime = 1; // Don't send auto-generated spawnpoints to client...
				SP_info_player_deathmatch( spawnpoint );
			}
		}

		trap->FS_Close(f);

		trap->Print( "^1*** ^3%s^5: Successfully loaded ^7%i^5 blue and ^7%i^5 red spawn points from spawnpoints file ^7spawnpoints/%s.team_spawnpoints^5.\n", "AUTO-SPAWNPOINTS",
			NUM_BLUE_POSITIONS, NUM_RED_POSITIONS, mapname.string );
	}
	else
	{
		vec3_t		ffa_angles = { 0 };

		trap->FS_Open( va( "spawnpoints/%s.ffa_spawnpoints", mapname.string), &f, FS_READ );

		if ( !f )
		{
			return qfalse;
		}

		trap->FS_Read( &NUM_BLUE_POSITIONS, sizeof(int), f );

		for (i = 0; i < NUM_BLUE_POSITIONS; i++)
		{
			trap->FS_Read( &BLUE_SPAWNPOINTS[i], sizeof(vec3_t), f );

			{
				gentity_t *spawnpoint = G_Spawn();
				spawnpoint->classname = "info_player_deathmatch";
				VectorCopy(BLUE_SPAWNPOINTS[i], spawnpoint->s.origin);
				VectorCopy(ffa_angles, spawnpoint->s.angles);
				spawnpoint->noWaypointTime = 1; // Don't send auto-generated spawnpoints to client...
				SP_info_player_deathmatch( spawnpoint );
			}
		}

		trap->FS_Close(f);

		trap->Print( "^1*** ^3%s^5: Successfully loaded ^7%i^5 spawn points from spawnpoints file ^7spawnpoints/%s.ffa_spawnpoints^5.\n", "AUTO-SPAWNPOINTS",
			NUM_BLUE_POSITIONS, mapname.string );
	}

	return qtrue;
}

qboolean SaveSpawnpointPositions( qboolean IsTeam, int NUM_BLUE_POSITIONS, vec3_t BLUE_SPAWNPOINTS[64], int NUM_RED_POSITIONS, vec3_t RED_SPAWNPOINTS[64] )
{
	fileHandle_t	f;
	int				i = 0;
	
	vmCvar_t	mapname;
	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	if (IsTeam)
	{
		trap->FS_Open( va( "spawnpoints/%s.team_spawnpoints", mapname.string), &f, FS_WRITE );

		if ( !f )
		{
			trap->Print( "^1*** ^3%s^5: Failed to open spawnpoints file ^7spawnpoints/%s.team_spawnpoints^5 for save.\n", "AUTO-SPAWNPOINTS", mapname.string );
			return qfalse;
		}

		trap->FS_Write( &NUM_BLUE_POSITIONS, sizeof(int), f );

		for (i = 0; i < NUM_BLUE_POSITIONS; i++)
		{
			trap->FS_Write( &BLUE_SPAWNPOINTS[i], sizeof(vec3_t), f );
		}

		trap->FS_Write( &NUM_RED_POSITIONS, sizeof(int), f );

		for (i = 0; i < NUM_RED_POSITIONS; i++)
		{
			trap->FS_Write( &RED_SPAWNPOINTS[i], sizeof(vec3_t), f );
		}

		trap->FS_Close(f);

		trap->Print( "^1*** ^3%s^5: Successfully saved ^7%i^5 blue and ^7%i^5 red spawn points to spawnpoints file ^7spawnpoints/%s.team_spawnpoints^5.\n", "AUTO-SPAWNPOINTS",
			NUM_BLUE_POSITIONS, NUM_RED_POSITIONS, mapname.string );

		return qtrue;
	}
	else
	{
		trap->FS_Open( va( "spawnpoints/%s.ffa_spawnpoints", mapname.string), &f, FS_WRITE );

		if ( !f )
		{
			trap->Print( "^1*** ^3%s^5: Failed to open spawnpoints file ^7spawnpoints/%s.ffa_spawnpoints^5 for save.\n", "AUTO-SPAWNPOINTS", mapname.string );
			return qfalse;
		}

		trap->FS_Write( &NUM_BLUE_POSITIONS, sizeof(int), f );

		for (i = 0; i < NUM_BLUE_POSITIONS; i++)
		{
			trap->FS_Write( &BLUE_SPAWNPOINTS[i], sizeof(vec3_t), f );
		}

		trap->FS_Close(f);

		trap->Print( "^1*** ^3%s^5: Successfully saved ^7%i^5 spawn points to spawnpoints file ^7spawnpoints/%s.ffa_spawnpoints^5.\n", "AUTO-SPAWNPOINTS",
			NUM_BLUE_POSITIONS, mapname.string );

		return qtrue;
	}

	return qfalse;	
}

void CreateSpawnpoints( void )
{// UQ1: Create extra spawnpoints based on gametype from waypoint locations...
	if (LoadSpawnpointPositions((qboolean)(g_gametype.integer >= GT_TEAM))) return; // Loaded from file...
	if (gWPNum <= 0) return; // No waypoints to use...

	if (g_gametype.integer >= GT_TEAM)
	{// Create team spawnpoints...
		int			blue_count = 0;
		int			red_count = 0;
		int			tries = 0;
		int			i = 0;
		gentity_t	*bluespot = NULL;
		gentity_t	*redspot = NULL;
		vec3_t		blue_angles = { 0 };
		vec3_t		red_angles = { 0 };
		int			MOST_DISTANT_POINTS[2];
		float		MOST_DISTANT_DISTANCE = 0.0f;
		int			BLUE_BEST_LIST[32] = { -1 };
		int			RED_BEST_LIST[32] = { -1 };
		int			LIST_TOTAL = 0;
		vec3_t		BLUE_SPAWNPOINTS[64] = { 0 };
		vec3_t		RED_SPAWNPOINTS[64] = { 0 };

		// Count the current number of spawnpoints...
		while ((bluespot = G_Find (bluespot, FOFS(classname), "team_CTF_bluespawn")) != NULL) 
		{
			blue_count++;
		}

		if (blue_count >= 8) return; // Don't need any more...

		while ((redspot = G_Find (redspot, FOFS(classname), "team_CTF_redspawn")) != NULL) 
		{
			red_count++;
		}

		if (red_count >= 8) return; // Don't need any more...

		//
		// Ok, we need more spawnpoints...
		//

		bluespot = G_Find (bluespot, FOFS(classname), "team_CTF_bluespawn");
		redspot = G_Find (redspot, FOFS(classname), "team_CTF_redspawn");

		if (red_count > 0 && blue_count > 0 && bluespot && redspot)
		{// Use the map's known spawnpoints for each team as the team start positions...
			//trap->Print("Using real CTF spawn points to generate extra spawn points.\n");
			MOST_DISTANT_POINTS[0] = DOM_GetNearestWP(bluespot->s.origin, -1);
			MOST_DISTANT_POINTS[1] = DOM_GetNearestWP(redspot->s.origin, -1);
			MOST_DISTANT_DISTANCE = HeightExageratedDistance(gWPArray[MOST_DISTANT_POINTS[0]]->origin, gWPArray[MOST_DISTANT_POINTS[1]]->origin);
		}
		else
		{// We have no team spawnpoints, so find the furthest 2 points from eachother on the map as each team's start positions...
			//trap->Print("Using FURTHEST map points to generate extra spawn points.\n");

			MOST_DISTANT_POINTS[0] = -1;
			MOST_DISTANT_POINTS[1] = -1;
			MOST_DISTANT_DISTANCE = 0.0;

#pragma omp parallel for schedule(dynamic)
			for (int n = 0; n < gWPNum; n++)
			{
				if (gWPArray[n]->flags & WPFLAG_WATER)
					continue;

				if (Warzone_SpawnpointNearMoverEntityLocation( gWPArray[n]->origin )) 
					continue;

				for (int o = 0; o < gWPNum; o++)
				{
					float dist;
					if (n == o) continue;

					if (gWPArray[o]->flags & WPFLAG_WATER)
						continue;

					if (Warzone_SpawnpointNearMoverEntityLocation( gWPArray[o]->origin )) 
						continue;

					dist = HeightExageratedDistance(gWPArray[n]->origin, gWPArray[o]->origin);

					if (dist > MOST_DISTANT_DISTANCE || MOST_DISTANT_POINTS[0] == -1 || MOST_DISTANT_POINTS[1] == -1)
					{
#pragma omp critical (__ADD_DIST_POINT__)
						{
							MOST_DISTANT_POINTS[0] = n;
							MOST_DISTANT_POINTS[1] = o;
							MOST_DISTANT_DISTANCE = dist;
						}
					}
				}
			}
		}

		//trap->Print("Furthest spawns are at (%f %f %f) and (%f %f %f). distance %f.\n", gWPArray[MOST_DISTANT_POINTS[0]]->origin[0], gWPArray[MOST_DISTANT_POINTS[0]]->origin[1], gWPArray[MOST_DISTANT_POINTS[0]]->origin[2], gWPArray[MOST_DISTANT_POINTS[1]]->origin[0], gWPArray[MOST_DISTANT_POINTS[1]]->origin[1], gWPArray[MOST_DISTANT_POINTS[1]]->origin[2], MOST_DISTANT_DISTANCE);

		blue_count = 0;
		red_count = 0;
		tries = 0;

		// Find add the best waypoint to the list until we have added 32 of each...
		while (LIST_TOTAL < 32)
		{
			int		BLUE_CLOSEST = -1;
			float	BLUE_CLOSEST_DIST = 0;
			int		RED_CLOSEST = -1;
			float	RED_CLOSEST_DIST = 0;

#pragma omp parallel for schedule(dynamic)
			for (int n = 0; n < gWPNum; n++)
			{
				qboolean alreadyInList = qfalse;
				float bdist, rdist;

				if (MOST_DISTANT_POINTS[0] == n || MOST_DISTANT_POINTS[1] == n)
					continue;

				if (gWPArray[n]->flags & WPFLAG_WATER)
					continue;

				for (int o = 0; o < LIST_TOTAL; o++)
				{
					if (n == BLUE_BEST_LIST[o] || n == RED_BEST_LIST[o])
					{
						alreadyInList = qtrue;
						break;
					}
				}

				if (alreadyInList)
					continue;

				// Not already on the list... See how close it is...
				bdist = HeightExageratedDistance(gWPArray[MOST_DISTANT_POINTS[0]]->origin, gWPArray[n]->origin);
				rdist = HeightExageratedDistance(gWPArray[MOST_DISTANT_POINTS[1]]->origin, gWPArray[n]->origin);

				if (bdist < BLUE_CLOSEST_DIST || BLUE_CLOSEST == -1)
				{// This one is better...
					if (!CheckSpawnPosition(gWPArray[n]->origin))
						continue; // Bad spawnpoint position...

#pragma omp critical (__SET_DIST_POINT_BLUE__)
					{
						BLUE_CLOSEST_DIST = bdist;
						BLUE_CLOSEST = n;
					}
				}
				
				if (rdist < RED_CLOSEST_DIST || RED_CLOSEST == -1)
				{// This one is better...
					if (!CheckSpawnPosition(gWPArray[n]->origin))
						continue; // Bad spawnpoint position...

#pragma omp critical (__SET_DIST_POINT_RED__)
					{
						RED_CLOSEST_DIST = rdist;
						RED_CLOSEST = n;
					}
				}
			}

			// Add the best found ones to each team's list...
			BLUE_BEST_LIST[LIST_TOTAL] = BLUE_CLOSEST;
			RED_BEST_LIST[LIST_TOTAL] = RED_CLOSEST;
			LIST_TOTAL++;
		}

		// We now have lists for each team... Make the spawnpoints...
		for (int n = 0; n < LIST_TOTAL; n++)
		{
			if (BLUE_BEST_LIST[n] != -1)
			{
				gentity_t *spawnpoint = G_Spawn();
				spawnpoint->classname = "team_CTF_bluespawn";
				VectorCopy(gWPArray[BLUE_BEST_LIST[n]]->origin, spawnpoint->s.origin);
				spawnpoint->s.origin[2]+=32.0;
				VectorCopy(blue_angles, spawnpoint->s.angles);
				spawnpoint->noWaypointTime = 1; // Don't send auto-generated spawnpoints to client...
				SP_info_player_deathmatch( spawnpoint );
				//trap->Print("Created blue spawn at %f %f %f.\n", spawnpoint->s.origin[0], spawnpoint->s.origin[1], spawnpoint->s.origin[2]);
				VectorCopy(spawnpoint->s.origin, BLUE_SPAWNPOINTS[n]);
				blue_count++;
			}

			if (RED_BEST_LIST[n] != -1)
			{
				gentity_t *spawnpoint = G_Spawn();
				spawnpoint->classname = "team_CTF_redspawn";
				VectorCopy(gWPArray[RED_BEST_LIST[n]]->origin, spawnpoint->s.origin);
				spawnpoint->s.origin[2]+=32.0;
				VectorCopy(red_angles, spawnpoint->s.angles);
				spawnpoint->noWaypointTime = 1; // Don't send auto-generated spawnpoints to client...
				SP_info_player_deathmatch( spawnpoint );
				VectorCopy(spawnpoint->s.origin, RED_SPAWNPOINTS[n]);
				//trap->Print("Created red spawn at %f %f %f.\n", spawnpoint->s.origin[0], spawnpoint->s.origin[1], spawnpoint->s.origin[2]);
				red_count++;
			}
		}

		trap->Print("^1*** ^3%s^5: Generated %i extra blue spawnpoints and %i extra red spawnpoints.\n", "AUTO-SPAWNPOINTS", blue_count, red_count);

		SaveSpawnpointPositions( qtrue, blue_count, BLUE_SPAWNPOINTS, red_count, RED_SPAWNPOINTS );
	}
	else
	{// Create ffa spawnpoints...
		int			count = 0;
		int			i = 0;
		gentity_t	*spot = NULL;
#if 0
		vec3_t		map_size;
		vec3_t		mins, maxs, red_angles, blue_angles, blue_mins, blue_maxs, red_mins, red_maxs;
#else
		vec3_t		blue_angles;
#endif
		vec3_t		SPAWNPOINTS[64] = { 0 };
		int			RAND_MULT = 1;

#if 0
		VectorSet(mins, 65536, 65536, 65536);
		VectorSet(maxs, -65536, -65536, -65536);
#endif

		// Count the current number of spawnpoints...
		while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) 
		{
			count++;
		}

		if (count >= 32) return; // Don't need any more...

		//
		// Ok, we need more spawnpoints...
		//

		while (1)
		{
			int			j = 0;
			int			choice = irand_big(0, gWPNum-1);
			qboolean	bad = qfalse;
			vec3_t		zeroOrg = { 0 };

			if (gWPArray[choice]->flags & WPFLAG_WATER)
				continue;
			
			// Find 32 that are not close to eachother...
			for (j = 0; j < count; j++)
			{
				if (Distance(SPAWNPOINTS[j], gWPArray[choice]->origin) < 128)
				{
					bad = qtrue;
					break;
				}
			}

			if (bad) continue;

			VectorSubtract(zeroOrg, gWPArray[i]->origin, blue_angles); // Face map center...
			vectoangles(blue_angles, blue_angles);
			blue_angles[2] = 0;

			{
				gentity_t	*spawnpoint = G_Spawn();
				spawnpoint->classname = "info_player_deathmatch";
				VectorCopy(gWPArray[i]->origin, spawnpoint->s.origin);
				spawnpoint->s.origin[2]+=32.0;
				VectorCopy(blue_angles, spawnpoint->s.angles);
				spawnpoint->noWaypointTime = 1; // Don't send auto-generated spawnpoints to client...
				SP_info_player_deathmatch( spawnpoint );
				VectorCopy(spawnpoint->s.origin, SPAWNPOINTS[count]);
				count++;
			}

			if (count >= 32) break;
		}

		trap->Print("^1*** ^3%s^5: Generated %i extra ffa spawnpoints.\n", "AUTO-SPAWNPOINTS", count);

		SaveSpawnpointPositions( qfalse, count, SPAWNPOINTS, 0, (vec3_t*)NULL );
	}
}


qboolean		EVENTS_ENABLED = qfalse;
float			EVENT_BUFFER = 32768.0;
float			EVENT_TRACE_SIZE = 1024.0;

float			MAP_WATER_LEVEL = -999999.9;

qboolean MAPPING_LoadMapInfo(void)
{
	vmCvar_t		mapname;
	const char		*climateName = NULL;
	trap->Cvar_Register(&mapname, "mapname", "", CVAR_ROM | CVAR_SERVERINFO);

	EVENTS_ENABLED = (atoi(IniRead(va("maps/%s.mapInfo", mapname.string), "EVENTS", "ENABLE_EVENTS", "0")) > 0) ? qtrue : qfalse;
	EVENT_BUFFER = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "EVENTS", "EVENT_BUFFER", "32768.0"));
	EVENT_TRACE_SIZE = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "EVENTS", "EVENT_TRACE_SIZE", "1024.0"));

	MAP_WATER_LEVEL = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "WATER", "MAP_WATER_LEVEL", "-999999.9"));

	return qtrue;
}

int IsBelowWaterPlane(vec3_t pos, float viewHeight)
{// Mimics the PM_SetWaterLevel levels for the warzone map water planes...
	float npcUnderwaterHeight = viewHeight - MINS_Z;
	float npcSwimHeight = npcUnderwaterHeight / 2;

	if (pos[2] + MINS_Z + npcUnderwaterHeight <= MAP_WATER_LEVEL)
	{
		return 3;
	}
	else if (pos[2] + MINS_Z + npcSwimHeight <= MAP_WATER_LEVEL)
	{
		return 2;
	}
	else if (pos[2] <= MAP_WATER_LEVEL)
	{
		return 1;
	}

	return 0;
}

#if defined(__USE_NAVLIB__) || defined(__USE_NAVLIB_SPAWNPOINTS__)
void FindRandomNavmeshSpawnpoint(gentity_t *self, vec3_t point)
{
	int tries = 0; // Can't let it hang, if the whole map happens to be underwater....

	NavlibFindRandomPointOnMesh(self, point);

	while (point[2] <= MAP_WATER_LEVEL && tries < 10)
	{
		NavlibFindRandomPointOnMesh(self, point);
		tries++;
	}
}

bool FindRandomNavmeshPointInRadius(int npcEntityNum, const vec3_t origin, vec3_t point, float radius)
{
	int tries = 0; // Can't let it hang, if the whole map happens to be underwater....

	NavlibFindRandomPointInRadius(npcEntityNum, origin, point, radius);

	while (point[2] <= MAP_WATER_LEVEL && tries < 10)
	{
		NavlibFindRandomPointInRadius(npcEntityNum, origin, point, radius);
		tries++;
	}

	if (tries >= 10)
		return false;

	return true;
}

void FindRandomNavmeshPatrolPoint(int npcEntityNum, vec3_t point)
{
	int tries = 0; // Can't let it hang, if the whole map happens to be underwater....

	gentity_t *npc = &g_entities[npcEntityNum];

	if (!npc)
	{// Should never be used this way, but return a completely random point.
		FindRandomNavmeshSpawnpoint(NULL, point);
		return;
	}

	NavlibFindRandomPointInRadius(npcEntityNum, npc->spawn_pos, point, /*256.0*/1024.0);

	while (point[2] <= MAP_WATER_LEVEL && tries < 10)
	{
		NavlibFindRandomPointInRadius(npcEntityNum, npc->spawn_pos, point, /*256.0*/1024.0);
		tries++;
	}
}
#endif //defined(__USE_NAVLIB__) || defined(__USE_NAVLIB_SPAWNPOINTS__)

/*
============
G_InitGame

============
*/
extern void RemoveAllWP(void);
extern void BG_ClearVehicleParseParms(void);
extern void NPC_PrecacheWarzoneNPCs ( void );
extern void FOLIAGE_LoadTrees( void );
extern void JKG_InitDamageSystem(void);

gentity_t *SelectRandomDeathmatchSpawnPoint(qboolean isbot);
void SP_info_jedimaster_start( gentity_t *ent );
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;
	vmCvar_t	mapname;
	vmCvar_t	ckSum;
	char serverinfo[MAX_INFO_STRING] = {0};

	//Init RMG to 0, it will be autoset to 1 if there is terrain on the level.
	trap->Cvar_Set("RMG", "0");
	RMG.integer = 0;

	//Clean up any client-server ghoul2 instance attachments that may still exist exe-side
	trap->G2API_CleanEntAttachments();

	BG_InitAnimsets(); //clear it out

	trap->SV_RegisterSharedMemory( gSharedBuffer.raw );

	//Load external vehicle data
	BG_VehicleLoadParms();

	trap->Print ("^5------- ^3Game Initialization^5 -------\n");
	trap->Print ("^5gamename: ^7%s\n", GAMEVERSION);
	trap->Print ("^5gamedate: ^4%s\n", __DATE__);
	trap->Print ("^5game mod: ^4%s\n", GAME_VERSION);

	srand( randomSeed );

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;

	level.follow1 = level.follow2 = -1;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	level.snd_hack = G_SoundIndex("sound/player/hacking.wav");
	level.snd_medHealed = G_SoundIndex("sound/player/supp_healed.wav");
	level.snd_medSupplied = G_SoundIndex("sound/player/supp_supplied.wav");

	//trap->SP_RegisterServer("mp_svgame");

	if ( g_log.string[0] )
	{
		trap->FS_Open( g_log.string, &level.logFile, g_logSync.integer ? FS_APPEND_SYNC : FS_APPEND );
		if ( level.logFile )
			trap->Print( "Logging to %s\n", g_log.string );
		else
			trap->Print( "WARNING: Couldn't open logfile: %s\n", g_log.string );
	}
	else
		trap->Print( "Not logging game events to disk.\n" );

	trap->GetServerinfo( serverinfo, sizeof( serverinfo ) );
	G_LogPrintf( "^5------------------------------------------------------------\n" );

#ifdef __ENABLE_DEDICATED_CONSOLE_COLORS__
	// Just for better readability on console...
	Q_StripColor(serverinfo);
#endif //__ENABLE_DEDICATED_CONSOLE_COLORS__

	G_LogPrintf( "^7InitGame: ^3%s^7\n", serverinfo );

	if ( g_securityLog.integer )
	{
		if ( g_securityLog.integer == 1 )
			trap->FS_Open( SECURITY_LOG, &level.security.log, FS_APPEND );
		else if ( g_securityLog.integer == 2 )
			trap->FS_Open( SECURITY_LOG, &level.security.log, FS_APPEND_SYNC );

		if ( level.security.log )
			trap->Print( "Logging to " SECURITY_LOG "\n" );
		else
			trap->Print( "WARNING: Couldn't open logfile: " SECURITY_LOG "\n" );
	}
	else
		trap->Print( "Not logging security events to disk.\n" );


	//WP_Grenade_CryoBanDamage();

	//WP_Thermal_FireDamage();

	G_CacheGametype();

	G_InitWorldSession();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = sv_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for ( i=0 ; i<MAX_CLIENTS ; i++ ) {
		g_entities[i].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap->LocateGameData( (sharedEntity_t *)level.gentities, level.num_entities, sizeof( gentity_t ),
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	//Load sabers.cfg data
	WP_SaberLoadParms();

	NPC_InitGame();

	TIMER_Clear();
	//
	//ICARUS INIT START

//	Com_Printf("^5------ ^7ICARUS Initialization^5 ------\n");

#ifndef __NO_ICARUS__
	trap->ICARUS_Init();
#endif //__NO_ICARUS__

//	Com_Printf ("^5-----------------------------------\n");

	//ICARUS INIT END
	//

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	//make sure saber data is loaded before this! (so we can precache the appropriate hilts)
	if (level.gametype == GT_SIEGE) // UQ1: Why were we doing this in non siege gametypes???
		InitSiegeMode();

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	trap->Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

	navCalculatePaths	= (qboolean)( trap->Nav_Load( mapname.string, ckSum.integer ) == qfalse );

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString(qfalse);

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( level.gametype >= GT_TEAM ) {
		G_CheckTeamItems();
	}
	else if ( level.gametype == GT_JEDIMASTER )
	{
		trap->SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );
	}

	if (level.gametype == GT_POWERDUEL)
	{
		trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1|-1") );
	}
	else
	{
		trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1") );
	}
// nmckenzie: DUEL_HEALTH: Default.
	trap->SetConfigstring ( CS_CLIENT_DUELHEALTHS, va("-1|-1|!") );
	trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("-1") );

	SaveRegisteredItems();

	//trap->Print ("^5-----------------------------------\n");

	if( level.gametype == GT_SINGLE_PLAYER || trap->Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}

	if ( trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( );
	} else {
		G_LoadArenas();
	}

	Com_Printf ("^5------- ^7Tree Initialization^5 -------\n");
	FOLIAGE_LoadTrees();
	Com_Printf ("^5-----------------------------------\n");

	MAPPING_LoadMapInfo();

	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
	{
		G_LogPrintf("Duel Tournament Begun: kill limit %d, win limit: %d\n", fraglimit.integer, duel_fraglimit.integer );
	}

	if ( navCalculatePaths )
	{//not loaded - need to calc paths
		navCalcPathTime = level.time + START_TIME_NAV_CALC;//make sure all ents are in and linked
	}
	else
	{//loaded
		//FIXME: if this is from a loadgame, it needs to be sure to write this
		//out whenever you do a savegame since the edges and routes are dynamic...
		//OR: always do a navigator.CheckBlockedEdges() on map startup after nav-load/calc-paths
		//navigator.pathsCalculated = qtrue;//just to be safe?  Does this get saved out?  No... assumed
		trap->Nav_SetPathsCalculated(qtrue);
		//need to do this, because combatpoint waypoints aren't saved out...?
		CP_FindCombatPointWaypoints();
		navCalcPathTime = 0;

		/*
		if ( g_eSavedGameJustLoaded == eNO )
		{//clear all the failed edges unless we just loaded the game (which would include failed edges)
			trap->Nav_ClearAllFailedEdges();
		}
		*/
		//No loading games in MP.
	}

	if (level.gametype == GT_SIEGE)
	{ //just get these configstrings registered now...
		while (i < MAX_CUSTOM_SIEGE_SOUNDS)
		{
			if (!bg_customSiegeSoundNames[i])
			{
				break;
			}
			G_SoundIndex((char *)bg_customSiegeSoundNames[i]);
			i++;
		}
	}

#if 0
	//[Create Dungeon]
	if (g_gametype.integer == GT_SINGLE_PLAYER || g_gametype.integer == GT_INSTANCE)
	{//load Up all the Npc In a Dungeon by where they have been placed ingame and load it up
		Load_Dungeon();
	}
	//[/Create Dungeon]
#endif

	if ( level.gametype == GT_JEDIMASTER ) {
		gentity_t *ent = NULL;
		int i=0;
		for ( i=0, ent=g_entities; i<level.num_entities; i++, ent++ ) {
			if ( ent->isSaberEntity )
				break;
		}

		if ( i == level.num_entities ) {
			// no JM saber found. drop one at one of the player spawnpoints
			gentity_t *spawnpoint = SelectRandomDeathmatchSpawnPoint(qfalse);

			if( !spawnpoint ) {
				trap->Error( ERR_DROP, "Couldn't find an FFA spawnpoint to drop the jedimaster saber at!\n" );
				return;
			}

			ent = G_Spawn();
			G_SetOrigin( ent, spawnpoint->s.origin );
			SP_info_jedimaster_start( ent );
		}
	}

	CreateSpawnpoints();
	Load_Model_Scales();

	Com_Printf("^5--------- ^7Spawn Groups^5 ---------\n");
	NPC_LoadSpawnList( "default_rebels" );
	NPC_LoadSpawnList( "default_empire" );
	NPC_LoadSpawnList( "default_wildlife" );
	NPC_LoadSpawnList( va("%s_rebels", mapname.string) );
	NPC_LoadSpawnList( va("%s_empire", mapname.string) );
	NPC_LoadSpawnList( va("%s_mandalorians", mapname.string) );
	NPC_LoadSpawnList( va("%s_mercenaries", mapname.string) );
	NPC_LoadSpawnList( va("%s_wildlife", mapname.string) );
	Com_Printf("^5--------------------------------\n");

	// Precache the map's used NPCs...
	NPC_PrecacheWarzoneNPCs();

	JKG_InitDamageSystem();

	//trap->Print("MAX_CONFIGSTRINGS is %i.\n", (int)MAX_CONFIGSTRINGS);
}

#ifdef __NPC_DYNAMIC_THREADS__
extern void AI_ThreadsShutdown(void);
#endif //__NPC_DYNAMIC_THREADS__

/*
=================
G_ShutdownGame
=================
*/
extern void FOLIAGE_FreeMemory(void);

void G_ShutdownGame( int restart ) {
	int i = 0;
	gentity_t *ent;

//	trap->Print ("==== ShutdownGame ====\n");

	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

//	Com_Printf("... Gameside GHOUL2 Cleanup\n");
	while (i < MAX_GENTITIES)
	{ //clean up all the ghoul2 instances
		ent = &g_entities[i];

		if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&ent->ghoul2);
			ent->ghoul2 = NULL;
		}
		if (ent->client)
		{
			int j = 0;

			while (j < MAX_SABERS)
			{
				if (ent->client->weaponGhoul2[j] && trap->G2API_HaveWeGhoul2Models(ent->client->weaponGhoul2[j]))
				{
					trap->G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[j]);
				}
				j++;
			}
		}
		i++;
	}

	if (g2SaberInstance && trap->G2API_HaveWeGhoul2Models(g2SaberInstance))
	{
		trap->G2API_CleanGhoul2Models(&g2SaberInstance);
		g2SaberInstance = NULL;
	}
	if (precachedKyle && trap->G2API_HaveWeGhoul2Models(precachedKyle))
	{
		trap->G2API_CleanGhoul2Models(&precachedKyle);
		precachedKyle = NULL;
	}

#ifndef __NO_ICARUS__
//	Com_Printf ("... ICARUS_Shutdown\n");
	trap->ICARUS_Shutdown ();	//Shut ICARUS down
#endif //__NO_ICARUS__

//	Com_Printf ("... Reference Tags Cleared\n");
	TAG_Init();	//Clear the reference tags

	if ( level.logFile ) {
		G_LogPrintf( "^7ShutdownGame:\n^5------------------------------------------------------------\n" );
		trap->FS_Close( level.logFile );
		level.logFile = 0;
	}

	if ( level.security.log )
	{
		G_SecurityLogPrintf( "ShutdownGame\n\n" );
		trap->FS_Close( level.security.log );
		level.security.log = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	trap->ROFF_Clean();

	if ( trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

	G_CleanAllFakeClients(); //get rid of dynamically allocated fake client structs.

#ifdef __NPC_DYNAMIC_THREADS__
	AI_ThreadsShutdown();
#endif //__NPC_DYNAMIC_THREADS__

	FOLIAGE_FreeMemory();
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
//	if ( level.intermissiontime ) {
//		return;
//	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if (!g_allowHighPingDuelist.integer && client->ps.ping >= 999)
		{ //don't add people who are lagging out if cvar is not set to allow it.
			continue;
		}
		if ( client->sess.sessionTeam != FACTION_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum )
			nextInLine = client;
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

/*
=======================
AddTournamentQueue

Add client to end of tournament queue
=======================
*/

void AddTournamentQueue( gclient_t *client )
{
	int index;
	gclient_t *curclient;

	for( index = 0; index < level.maxclients; index++ )
	{
		curclient = &level.clients[index];

		if ( curclient->pers.connected != CON_DISCONNECTED )
		{
			if ( curclient == client )
				curclient->sess.spectatorNum = 0;
			else if ( curclient->sess.sessionTeam == FACTION_SPECTATOR )
				curclient->sess.spectatorNum++;
		}
	}
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

void G_PowerDuelCount(int *loners, int *doubles, qboolean countSpec)
{
	int i = 0;
	gclient_t *cl;

	while (i < MAX_CLIENTS)
	{
		cl = g_entities[i].client;

		if (g_entities[i].inuse && cl && (countSpec || cl->sess.sessionTeam != FACTION_SPECTATOR))
		{
			if (cl->sess.duelTeam == DUELTEAM_LONE)
			{
				(*loners)++;
			}
			else if (cl->sess.duelTeam == DUELTEAM_DOUBLE)
			{
				(*doubles)++;
			}
		}
		i++;
	}
}

qboolean g_duelAssigning = qfalse;
void AddPowerDuelPlayers( void )
{
	int			i;
	int			loners = 0;
	int			doubles = 0;
	int			nonspecLoners = 0;
	int			nonspecDoubles = 0;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 3 )
	{
		return;
	}

	nextInLine = NULL;

	G_PowerDuelCount(&nonspecLoners, &nonspecDoubles, qfalse);
	if (nonspecLoners >= 1 && nonspecDoubles >= 2)
	{ //we have enough people, stop
		return;
	}

	//Could be written faster, but it's not enough to care I suppose.
	G_PowerDuelCount(&loners, &doubles, qtrue);

	if (loners < 1 || doubles < 2)
	{ //don't bother trying to spawn anyone yet if the balance is not even set up between spectators
		return;
	}

	//Count again, with only in-game clients in mind.
	loners = nonspecLoners;
	doubles = nonspecDoubles;
//	G_PowerDuelCount(&loners, &doubles, qfalse);

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != FACTION_SPECTATOR ) {
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_FREE)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
		{
			continue;
		}
		if (client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
		{
			continue;
		}

		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum )
			nextInLine = client;
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );

	//Call recursively until everyone is in
	AddPowerDuelPlayers();
}

qboolean g_dontFrickinCheck = qfalse;

void RemovePowerDuelLosers(void)
{
	int remClients[3];
	int remNum = 0;
	int i = 0;
	gclient_t *cl;

	while (i < MAX_CLIENTS && remNum < 3)
	{
		//cl = &level.clients[level.sortedClients[i]];
		cl = &level.clients[i];

		if (cl->pers.connected == CON_CONNECTED)
		{
			if ((cl->ps.stats[STAT_HEALTH] <= 0 || cl->iAmALoser) &&
				(cl->sess.sessionTeam != FACTION_SPECTATOR || cl->iAmALoser))
			{ //he was dead or he was spectating as a loser
                remClients[remNum] = i;
				remNum++;
			}
		}

		i++;
	}

	if (!remNum)
	{ //Time ran out or something? Oh well, just remove the main guy.
		remClients[remNum] = level.sortedClients[0];
		remNum++;
	}

	i = 0;
	while (i < remNum)
	{ //set them all to spectator
		SetTeam( &g_entities[ remClients[i] ], "s" );
		i++;
	}

	g_dontFrickinCheck = qfalse;

	//recalculate stuff now that we have reset teams.
	CalculateRanks();
}

void RemoveDuelDrawLoser(void)
{
	int clFirst = 0;
	int clSec = 0;
	int clFailure = 0;

	if ( level.clients[ level.sortedClients[0] ].pers.connected != CON_CONNECTED )
	{
		return;
	}
	if ( level.clients[ level.sortedClients[1] ].pers.connected != CON_CONNECTED )
	{
		return;
	}

	clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
	clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];

	if (clFirst > clSec)
	{
		clFailure = 1;
	}
	else if (clSec > clFirst)
	{
		clFailure = 0;
	}
	else
	{
		clFailure = 2;
	}

	if (clFailure != 2)
	{
		SetTeam( &g_entities[ level.sortedClients[clFailure] ], "s" );
	}
	else
	{ //we could be more elegant about this, but oh well.
		SetTeam( &g_entities[ level.sortedClients[1] ], "s" );
	}
}

/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], "s" );
}

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
		level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
		level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
		level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
	{
		int clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
		int clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];
		int clFailure = 0;
		int clSuccess = 0;

		if (clFirst > clSec)
		{
			clFailure = 1;
			clSuccess = 0;
		}
		else if (clSec > clFirst)
		{
			clFailure = 0;
			clSuccess = 1;
		}
		else
		{
			clFailure = 2;
			clSuccess = 2;
		}

		if (clFailure != 2)
		{
			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
		else
		{
			clSuccess = 0;
			clFailure = 1;

			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
	else
	{
		clientNum = level.sortedClients[0];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.wins++;
			ClientUserinfoChanged( clientNum );

			trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );
		}

		clientNum = level.sortedClients[1];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	if (level.gametype == GT_POWERDUEL)
	{
		//sort single duelists first
		if (ca->sess.duelTeam == DUELTEAM_LONE && ca->sess.sessionTeam != FACTION_SPECTATOR)
		{
			return -1;
		}
		if (cb->sess.duelTeam == DUELTEAM_LONE && cb->sess.sessionTeam != FACTION_SPECTATOR)
		{
			return 1;
		}

		//others will be auto-sorted below but above spectators.
	}

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}


	// then spectators
	if ( ca->sess.sessionTeam == FACTION_SPECTATOR && cb->sess.sessionTeam == FACTION_SPECTATOR ) {
		if ( ca->sess.spectatorNum > cb->sess.spectatorNum ) {
			return -1;
		}
		if ( ca->sess.spectatorNum < cb->sess.spectatorNum ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == FACTION_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == FACTION_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

qboolean gQueueScoreMessage = qfalse;
int gQueueScoreMessageTime = 0;

//A new duel started so respawn everyone and make sure their stats are reset
qboolean G_CanResetDuelists(void)
{
	int i;
	gentity_t *ent;

	i = 0;
	while (i < 3)
	{ //precheck to make sure they are all respawnable
		ent = &g_entities[level.sortedClients[i]];

		if (!ent->inuse || !ent->client || ent->health <= 0 ||
			ent->client->sess.sessionTeam == FACTION_SPECTATOR ||
			ent->client->sess.duelTeam <= DUELTEAM_FREE)
		{
			return qfalse;
		}
		i++;
	}

	return qtrue;
}

qboolean g_noPDuelCheck = qfalse;
void G_ResetDuelists(void)
{
	int i;
	gentity_t *ent = NULL;

	i = 0;
	while (i < 3)
	{
		ent = &g_entities[level.sortedClients[i]];

		g_noPDuelCheck = qtrue;
		player_die(ent, ent, ent, 999, MOD_SUICIDE);
		g_noPDuelCheck = qfalse;
		trap->UnlinkEntity ((sharedEntity_t *)ent);
		ClientSpawn(ent);
		i++;
	}
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
//	int		preNumSpec = 0;
	//int		nonSpecIndex = -1;
	gclient_t	*cl;

//	preNumSpec = level.numNonSpectatorClients;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots

	for ( i = 0; i < ARRAY_LEN(level.numteamVotingClients); i++ ) {
		level.numteamVotingClients[i] = 0;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != FACTION_SPECTATOR || level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
			{
				if (level.clients[i].sess.sessionTeam != FACTION_SPECTATOR)
				{
					level.numNonSpectatorClients++;
					//nonSpecIndex = i;
				}

				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED )
				{
					if (level.clients[i].sess.sessionTeam != FACTION_SPECTATOR || level.clients[i].iAmALoser)
					{
						level.numPlayingClients++;
					}
					if ( !(g_entities[i].r.svFlags & SVF_BOT) )
					{
						level.numVotingClients++;
						if ( level.clients[i].sess.sessionTeam == FACTION_EMPIRE )
							level.numteamVotingClients[0]++;
						else if ( level.clients[i].sess.sessionTeam == FACTION_REBEL )
							level.numteamVotingClients[1]++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	if ( !g_warmup.integer || level.gametype == GT_SIEGE )
		level.warmupTime = 0;

	/*
	if (level.numNonSpectatorClients == 2 && preNumSpec < 2 && nonSpecIndex != -1 && level.gametype == GT_DUEL && !level.warmupTime)
	{
		gentity_t *currentWinner = G_GetDuelWinner(&level.clients[nonSpecIndex]);

		if (currentWinner && currentWinner->client)
		{
			trap->SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
			currentWinner->client->pers.netname, G_GetStringEdString("MP_SVGAME", "VERSUS"), level.clients[nonSpecIndex].pers.netname));
		}
	}
	*/
	//NOTE: for now not doing this either. May use later if appropriate.

	qsort( level.sortedClients, level.numConnectedClients,
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( level.gametype >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[FACTION_EMPIRE] == level.teamScores[FACTION_REBEL] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[FACTION_EMPIRE] > level.teamScores[FACTION_REBEL] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else if(i != 0 ){
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( (level.gametype == GT_SINGLE_PLAYER || level.gametype == GT_INSTANCE) && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( level.gametype >= GT_TEAM ) {
		trap->SetConfigstring( CS_SCORES1, va("%i", level.teamScores[FACTION_EMPIRE] ) );
		trap->SetConfigstring( CS_SCORES2, va("%i", level.teamScores[FACTION_REBEL] ) );
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap->SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap->SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap->SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap->SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap->SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap->SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) );
		}

		if (level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL)
		{ //when not in duel, use this configstring to pass the index of the player currently in first place
			if ( level.numConnectedClients >= 1 )
			{
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", level.sortedClients[0] ) );
			}
			else
			{
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission or in multi-frag Duel game mode, send the new info to everyone
	if ( level.intermissiontime || level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		gQueueScoreMessage = qtrue;
		gQueueScoreMessageTime = level.time + 500;
		//SendScoreboardMessageToAllClients();
		//rww - Made this operate on a "queue" system because it was causing large overflows
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
extern void G_LeaveVehicle( gentity_t *ent, qboolean ConCheck );
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	FindIntermissionPoint();
	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	G_LeaveVehicle( ent, qfalse );

	ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
	ent->client->ps.rocketLockTime = 0;

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->client->ps.eFlags2 = 0;
	ent->s.eFlags2 = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.loopIsSoundset = qfalse;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
extern qboolean	gSiegeRoundBegun;
extern qboolean	gSiegeRoundEnded;
extern int	gSiegeRoundWinningTeam;
void FindIntermissionPoint( void ) {
	gentity_t	*ent = NULL;
	gentity_t	*target;
	vec3_t		dir;

	// find the intermission spot
	if ( level.gametype == GT_SIEGE
		&& level.intermissiontime
		&& level.intermissiontime <= level.time
		&& gSiegeRoundEnded )
	{
	   	if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM1)
		{
			ent = G_Find (NULL, FOFS(classname), "info_player_intermission_red");
			if ( ent && ent->target2 )
			{
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
	   	else if (gSiegeRoundWinningTeam == SIEGETEAM_TEAM2)
		{
			ent = G_Find (NULL, FOFS(classname), "info_player_intermission_blue");
			if ( ent && ent->target2 )
			{
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
	}
	if ( !ent )
	{
		ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	}
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( vec3_origin, level.intermission_origin, level.intermission_angle, FACTION_SPECTATOR, qfalse );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}
}

qboolean DuelLimitHit(void);

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournament mode, change the wins / losses
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

		if (level.gametype != GT_POWERDUEL)
		{
			AdjustTournamentScores();
		}
		if (DuelLimitHit())
		{
			gDuelExit = qtrue;
		}
		else
		{
			gDuelExit = qfalse;
		}
	}

	level.intermissiontime = level.time;

	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			if (level.gametype != GT_POWERDUEL ||
				!client->client ||
				client->client->sess.sessionTeam != FACTION_SPECTATOR)
			{ //don't respawn spectators in powerduel or it will mess the line order all up
				ClientRespawn(client);
			}
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}

qboolean DuelLimitHit(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( duel_fraglimit.integer && cl->sess.wins >= duel_fraglimit.integer )
		{
			return qtrue;
		}
	}

	return qfalse;
}

void DuelResetWinsLosses(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		cl->sess.wins = 0;
		cl->sess.losses = 0;
	}
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
extern void SiegeDoTeamAssign(void); //g_saga.c
extern siegePers_t g_siegePersistant; //g_saga.c
void ExitLevel (void) {
	int		i;
	gclient_t *cl;

	// if we are running a tournament map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		if (!DuelLimitHit())
		{
			if ( !level.restarted ) {
				trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				level.restarted = qtrue;
				level.changemap = NULL;
				level.intermissiontime = 0;
			}
			return;
		}

		DuelResetWinsLosses();
	}


	if (level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer &&
		g_siegePersistant.beatingTime)
	{ //restart same map...
		trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	}
	else
	{
		trap->SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}
	level.changemap = NULL;
	level.intermissiontime = 0;

	if (level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer)
	{ //switch out now
		SiegeDoTeamAssign();
	}

	// reset all the scores so we don't enter the intermission again
	level.teamScores[FACTION_EMPIRE] = 0;
	level.teamScores[FACTION_REBEL] = 0;
	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< sv_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024] = {0};
	int			mins, seconds, msec, l;

	msec = level.time - level.startTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds %= 60;
//	msec %= 1000;

	Com_sprintf( string, sizeof( string ), "%i:%02i ", mins, seconds );

	l = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string + l, sizeof( string ) - l, fmt, argptr );
	va_end( argptr );

	if ( dedicated.integer )
		trap->Print( "%s", string + l );

	if ( !level.logFile )
		return;

	trap->FS_Write( string, strlen( string ), level.logFile );
}
/*
=================
G_SecurityLogPrintf

Print to the security logfile with a time stamp if it is open
=================
*/
void QDECL G_SecurityLogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024] = {0};
	time_t		rawtime;
	int			timeLen=0;

	time( &rawtime );
	localtime( &rawtime );
	strftime( string, sizeof( string ), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &rawtime ) );
	timeLen = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string+timeLen, sizeof( string ) - timeLen, fmt, argptr );
	va_end( argptr );

	if ( dedicated.integer )
		trap->Print( "%s", string + timeLen );

	if ( !level.security.log )
		return;

	trap->FS_Write( string, strlen( string ), level.security.log );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;
//	qboolean		won = qtrue;
	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap->SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( level.gametype >= GT_TEAM ) {
		G_LogPrintf( "red:%i  blue:%i\n",
			level.teamScores[FACTION_EMPIRE], level.teamScores[FACTION_REBEL] );
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == FACTION_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		if (level.gametype >= GT_TEAM) {
			G_LogPrintf( "(%s) score: %i  ping: %i  client: [%s] %i \"%s^7\"\n", TeamName(cl->ps.persistant[PERS_TEAM]), cl->ps.persistant[PERS_SCORE], ping, cl->pers.guid, level.sortedClients[i], cl->pers.netname );
		} else {
			G_LogPrintf( "score: %i  ping: %i  client: [%s] %i \"%s^7\"\n", cl->ps.persistant[PERS_SCORE], ping, cl->pers.guid, level.sortedClients[i], cl->pers.netname );
		}
//		if (g_singlePlayer.integer && (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)) {
//			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
//				won = qfalse;
//			}
//		}
	}

	//yeah.. how about not.
	/*
	if (g_singlePlayer.integer) {
		if (level.gametype >= GT_CTF) {
			won = level.teamScores[FACTION_EMPIRE] > level.teamScores[FACTION_REBEL];
		}
		trap->SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
	*/
}

qboolean gDidDuelStuff = qfalse; //gets reset on game reinit

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady;
	int			i;
	gclient_t	*cl;
	int			readyMask;

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	for (i=0 ; i< sv_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}

		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDidDuelStuff &&
		(level.time > level.intermissiontime + 2000) )
	{
		gDidDuelStuff = qtrue;

		if ( g_austrian.integer && level.gametype != GT_POWERDUEL )
		{
			G_LogPrintf("Duel Results:\n");
			//G_LogPrintf("Duel Time: %d\n", level.time );
			G_LogPrintf("winner: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
			G_LogPrintf("loser: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[1]].pers.netname,
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[1]].sess.wins,
				level.clients[level.sortedClients[1]].sess.losses );
		}
		// if we are running a tournament map, kick the loser to spectator status,
		// which will automatically grab the next spectator and restart
		if (!DuelLimitHit())
		{
			if (level.gametype == GT_POWERDUEL)
			{
				RemovePowerDuelLosers();
				AddPowerDuelPlayers();
			}
			else
			{
				if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
					level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
					level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
					level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
				{
					RemoveDuelDrawLoser();
				}
				else
				{
					RemoveTournamentLoser();
				}
				AddTournamentPlayer();
			}

			if ( g_austrian.integer )
			{
				if (level.gametype == GT_POWERDUEL)
				{
					G_LogPrintf("Power Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						level.clients[level.sortedClients[2]].pers.netname,
						level.clients[level.sortedClients[2]].sess.wins,
						level.clients[level.sortedClients[2]].sess.losses,
						fraglimit.integer );
				}
				else
				{
					G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						fraglimit.integer );
				}
			}

			if (level.gametype == GT_POWERDUEL)
			{
				if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
				{
					trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
					trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
				}
			}
			else
			{
				if (level.numPlayingClients >= 2)
				{
					trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
					trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
				}
			}

			return;
		}

		if ( g_austrian.integer && level.gametype != GT_POWERDUEL )
		{
			G_LogPrintf("Duel Tournament Winner: %s wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
		}

		if (level.gametype == GT_POWERDUEL)
		{
			RemovePowerDuelLosers();
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{
				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
		else
		{
			//this means we hit the duel limit so reset the wins/losses
			//but still push the loser to the back of the line, and retain the order for
			//the map change
			if (level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
				level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
				level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
			{
				RemoveDuelDrawLoser();
			}
			else
			{
				RemoveTournamentLoser();
			}

			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDuelExit)
	{ //in duel, we have different behaviour for between-round intermissions
		if ( level.time > level.intermissiontime + 4000 )
		{ //automatically go to next after 4 seconds
			ExitLevel();
			return;
		}

		for (i=0 ; i< sv_maxclients.integer ; i++)
		{ //being in a "ready" state is not necessary here, so clear it for everyone
		  //yes, I also thinking holding this in a ps value uniquely for each player
		  //is bad and wrong, but it wasn't my idea.
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}
			cl->ps.stats[STAT_CLIENTS_READY] = 0;
		}
		return;
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< sv_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	if (d_noIntermissionWait.integer)
	{ //don't care who wants to go, just go.
		ExitLevel();
		return;
	}

	// if nobody wants to go, clear timer
	if ( !ready ) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	if ( level.gametype >= GT_TEAM ) {
		return (qboolean)(level.teamScores[FACTION_EMPIRE] == level.teamScores[FACTION_REBEL]);
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return (qboolean)(a == b);
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
qboolean g_endPDuel = qfalse;
void CheckExitRules( void ) {
 	int			i;
	gclient_t	*cl;
	char *sKillLimit;
	qboolean printLimit = qtrue;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	}

	if (gDoSlowMoDuel)
	{ //don't go to intermission while in slow motion
		return;
	}

	if (gEscaping)
	{
		int numLiveClients = 0;

		for ( i=0; i < MAX_CLIENTS; i++ )
		{
			if (g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0)
			{
				if (g_entities[i].client->sess.sessionTeam != FACTION_SPECTATOR &&
					!(g_entities[i].client->ps.pm_flags & PMF_FOLLOW))
				{
					numLiveClients++;
				}
			}
		}
		if (gEscapeTime < level.time)
		{
			gEscaping = qfalse;
			LogExit( "Escape time ended." );
			return;
		}
		if (!numLiveClients)
		{
			gEscaping = qfalse;
			LogExit( "Everyone failed to escape." );
			return;
		}
	}

	if ( level.intermissionQueued ) {
		//int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		int time = INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	/*
	if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients < 3)
		{
			if (!level.intermissiontime)
			{
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Duel forfeit (1)\n");
				}
				LogExit("Duel forfeit.");
				return;
			}
		}
	}
	*/

	// check for sudden death
	if (level.gametype != GT_SIEGE)
	{
		if ( ScoreIsTied() ) {
			// always wait for sudden death
			if ((level.gametype != GT_DUEL) || !timelimit.value)
			{
				if (level.gametype != GT_POWERDUEL)
				{
					return;
				}
			}
		}
	}

	if (level.gametype != GT_SIEGE)
	{
		if ( timelimit.value > 0.0f && !level.warmupTime ) {
			if ( level.time - level.startTime >= timelimit.value*60000 ) {
//				trap->SendServerCommand( -1, "print \"Timelimit hit.\n\"");
				trap->SendServerCommand( -1, va("print \"%s.\n\"",G_GetStringEdString("MP_SVGAME", "TIMELIMIT_HIT")));
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Timelimit hit (1)\n");
				}
				LogExit( "Timelimit hit." );
				return;
			}
		}
	}

	if (level.gametype == GT_POWERDUEL && level.numPlayingClients >= 3)
	{
		if (g_endPDuel)
		{
			g_endPDuel = qfalse;
			LogExit("Powerduel ended.");
		}

		//yeah, this stuff was completely insane.
		/*
		int duelists[3];
		duelists[0] = level.sortedClients[0];
		duelists[1] = level.sortedClients[1];
		duelists[2] = level.sortedClients[2];

		if (duelists[0] != -1 &&
			duelists[1] != -1 &&
			duelists[2] != -1)
		{
			if (!g_entities[duelists[0]].inuse ||
				!g_entities[duelists[0]].client ||
				g_entities[duelists[0]].client->ps.stats[STAT_HEALTH] <= 0 ||
				g_entities[duelists[0]].client->sess.sessionTeam != FACTION_FREE)
			{ //The lone duelist lost, give the other two wins (if applicable) and him a loss
				if (g_entities[duelists[0]].inuse &&
					g_entities[duelists[0]].client)
				{
					g_entities[duelists[0]].client->sess.losses++;
					ClientUserinfoChanged(duelists[0]);
				}
				if (g_entities[duelists[1]].inuse &&
					g_entities[duelists[1]].client)
				{
					if (g_entities[duelists[1]].client->ps.stats[STAT_HEALTH] > 0 &&
						g_entities[duelists[1]].client->sess.sessionTeam == FACTION_FREE)
					{
						g_entities[duelists[1]].client->sess.wins++;
					}
					else
					{
						g_entities[duelists[1]].client->sess.losses++;
					}
					ClientUserinfoChanged(duelists[1]);
				}
				if (g_entities[duelists[2]].inuse &&
					g_entities[duelists[2]].client)
				{
					if (g_entities[duelists[2]].client->ps.stats[STAT_HEALTH] > 0 &&
						g_entities[duelists[2]].client->sess.sessionTeam == FACTION_FREE)
					{
						g_entities[duelists[2]].client->sess.wins++;
					}
					else
					{
						g_entities[duelists[2]].client->sess.losses++;
					}
					ClientUserinfoChanged(duelists[2]);
				}

				//Will want to parse indecies for two out at some point probably
				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", duelists[1] ) );

				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Coupled duelists won (1)\n");
				}
				LogExit( "Coupled duelists won." );
				gDuelExit = qfalse;
			}
			else if ((!g_entities[duelists[1]].inuse ||
				!g_entities[duelists[1]].client ||
				g_entities[duelists[1]].client->sess.sessionTeam != FACTION_FREE ||
				g_entities[duelists[1]].client->ps.stats[STAT_HEALTH] <= 0) &&
				(!g_entities[duelists[2]].inuse ||
				!g_entities[duelists[2]].client ||
				g_entities[duelists[2]].client->sess.sessionTeam != FACTION_FREE ||
				g_entities[duelists[2]].client->ps.stats[STAT_HEALTH] <= 0))
			{ //the coupled duelists lost, give the lone duelist a win (if applicable) and the couple both losses
				if (g_entities[duelists[1]].inuse &&
					g_entities[duelists[1]].client)
				{
					g_entities[duelists[1]].client->sess.losses++;
					ClientUserinfoChanged(duelists[1]);
				}
				if (g_entities[duelists[2]].inuse &&
					g_entities[duelists[2]].client)
				{
					g_entities[duelists[2]].client->sess.losses++;
					ClientUserinfoChanged(duelists[2]);
				}

				if (g_entities[duelists[0]].inuse &&
					g_entities[duelists[0]].client &&
					g_entities[duelists[0]].client->ps.stats[STAT_HEALTH] > 0 &&
					g_entities[duelists[0]].client->sess.sessionTeam == FACTION_FREE)
				{
					g_entities[duelists[0]].client->sess.wins++;
					ClientUserinfoChanged(duelists[0]);
				}

				trap->SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", duelists[0] ) );

				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Lone duelist won (1)\n");
				}
				LogExit( "Lone duelist won." );
				gDuelExit = qfalse;
			}
		}
		*/
		return;
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		if (fraglimit.integer > 1)
		{
			sKillLimit = "Kill limit hit.";
		}
		else
		{
			sKillLimit = "";
			printLimit = qfalse;
		}
	}
	else
	{
		sKillLimit = "Kill limit hit.";
	}
	if ( level.gametype < GT_SIEGE && fraglimit.integer ) {
		if ( level.teamScores[FACTION_EMPIRE] >= fraglimit.integer ) {
			trap->SendServerCommand( -1, va("print \"Red %s\n\"", G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")) );
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (1)\n");
			}
			LogExit( sKillLimit );
			return;
		}

		if ( level.teamScores[FACTION_REBEL] >= fraglimit.integer ) {
			trap->SendServerCommand( -1, va("print \"Blue %s\n\"", G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")) );
			if (d_powerDuelPrint.integer)
			{
				Com_Printf("POWERDUEL WIN CONDITION: Kill limit (2)\n");
			}
			LogExit( sKillLimit );
			return;
		}

		for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != FACTION_FREE ) {
				continue;
			}

			if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && duel_fraglimit.integer && cl->sess.wins >= duel_fraglimit.integer )
			{
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Duel limit hit (1)\n");
				}
				LogExit( "Duel limit hit." );
				gDuelExit = qtrue;
				trap->SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the win limit.\n\"",
					cl->pers.netname ) );
				return;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= fraglimit.integer ) {
				if (d_powerDuelPrint.integer)
				{
					Com_Printf("POWERDUEL WIN CONDITION: Kill limit (3)\n");
				}
				LogExit( sKillLimit );
				gDuelExit = qfalse;
				if (printLimit)
				{
					trap->SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s.\n\"",
													cl->pers.netname,
													G_GetStringEdString("MP_SVGAME", "HIT_THE_KILL_LIMIT")
													)
											);
				}
				return;
			}
		}
	}

	if ( level.gametype >= GT_CTF && capturelimit.integer ) {

		if ( level.teamScores[FACTION_EMPIRE] >= capturelimit.integer )
		{
			trap->SendServerCommand( -1,  va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")));
			trap->SendServerCommand( -1,  va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
			LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[FACTION_REBEL] >= capturelimit.integer ) {
			trap->SendServerCommand( -1,  va("print \"%s \"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")));
			trap->SendServerCommand( -1,  va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "HIT_CAPTURE_LIMIT")));
			LogExit( "Capturelimit hit." );
			return;
		}
	}
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

void G_RemoveDuelist(int team)
{
	int i = 0;
	gentity_t *ent;
	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client && ent->client->sess.sessionTeam != FACTION_SPECTATOR &&
			ent->client->sess.duelTeam == team)
		{
			SetTeam(ent, "s");
		}
        i++;
	}
}

/*
=============
CheckTournament

Once a frame, check for changes in tournament player state
=============
*/
int g_duelPrintTimer = 0;
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
//	if ( level.numPlayingClients == 0 && (level.gametype != GT_POWERDUEL) ) {
//		return;
//	}

	if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
		{
			trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
		}
	}
	else
	{
		if (level.numPlayingClients >= 2)
		{
			trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
		}
	}

	if ( level.gametype == GT_DUEL )
	{
		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 && !level.intermissiontime && !level.intermissionQueued ) {
			AddTournamentPlayer();

			if (level.numPlayingClients >= 2)
			{
				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
			}
		}

		if (level.numPlayingClients >= 2)
		{
// nmckenzie: DUEL_HEALTH
			if ( g_showDuelHealths.integer >= 1 )
			{
				playerState_t *ps1, *ps2;
				ps1 = &level.clients[level.sortedClients[0]].ps;
				ps2 = &level.clients[level.sortedClients[1]].ps;
				trap->SetConfigstring ( CS_CLIENT_DUELHEALTHS, va("%i|%i|!",
					ps1->stats[STAT_HEALTH], ps2->stats[STAT_HEALTH]));
			}
		}

		//rww - It seems we have decided there will be no warmup in duel.
		//if (!g_warmup.integer)
		{ //don't care about any of this stuff then, just add people and leave me alone
			level.warmupTime = 0;
			return;
		}
#if 0
		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;

				if (level.warmupTime < (level.time + 3000))
				{ //rww - this is an unpleasent hack to keep the level from resetting completely on the client (this happens when two map_restarts are issued rapidly)
					level.warmupTime = level.time + 3000;
				}
				trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap->Cvar_Set( "g_restarted", "1" );
			trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
#endif
	}
	else if (level.gametype == GT_POWERDUEL)
	{
		if (level.numPlayingClients < 2)
		{ //hmm, ok, pull more in.
			g_dontFrickinCheck = qfalse;
		}

		if (level.numPlayingClients > 3)
		{ //umm..yes..lets take care of that then.
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone > 1)
			{
				G_RemoveDuelist(DUELTEAM_LONE);
			}
			else if (dbl > 2)
			{
				G_RemoveDuelist(DUELTEAM_DOUBLE);
			}
		}
		else if (level.numPlayingClients < 3)
		{ //hmm, someone disconnected or something and we need em
			int lone = 0, dbl = 0;

			G_PowerDuelCount(&lone, &dbl, qfalse);
			if (lone < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
			else if (dbl < 1)
			{
				g_dontFrickinCheck = qfalse;
			}
		}

		// pull in a spectator if needed
		if (level.numPlayingClients < 3 && !g_dontFrickinCheck)
		{
			AddPowerDuelPlayers();

			if (level.numPlayingClients >= 3 &&
				G_CanResetDuelists())
			{
				gentity_t *te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
				te->r.svFlags |= SVF_BROADCAST;
				//this is really pretty nasty, but..
				te->s.otherEntityNum = level.sortedClients[0];
				te->s.otherEntityNum2 = level.sortedClients[1];
				te->s.groundEntityNum = level.sortedClients[2];

				trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				G_ResetDuelists();

				g_dontFrickinCheck = qtrue;
			}
			else if (level.numPlayingClients > 0 ||
				level.numConnectedClients > 0)
			{
				if (g_duelPrintTimer < level.time)
				{ //print once every 10 seconds
					int lone = 0, dbl = 0;

					G_PowerDuelCount(&lone, &dbl, qtrue);
					if (lone < 1)
					{
						trap->SendServerCommand( -1, va("cp \"%s\n\"", G_GetStringEdString("MP_SVGAME", "DUELMORESINGLE")) );
					}
					else
					{
						trap->SendServerCommand( -1, va("cp \"%s\n\"", G_GetStringEdString("MP_SVGAME", "DUELMOREPAIRED")) );
					}
					g_duelPrintTimer = level.time + 10000;
				}
			}

			if (level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3)
			{ //pulled in a needed person
				if (G_CanResetDuelists())
				{
					gentity_t *te = G_TempEntity(vec3_origin, EV_GLOBAL_DUEL);
					te->r.svFlags |= SVF_BROADCAST;
					//this is really pretty nasty, but..
					te->s.otherEntityNum = level.sortedClients[0];
					te->s.otherEntityNum2 = level.sortedClients[1];
					te->s.groundEntityNum = level.sortedClients[2];

					trap->SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );

					if ( g_austrian.integer )
					{
						G_LogPrintf("Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
							level.clients[level.sortedClients[0]].pers.netname,
							level.clients[level.sortedClients[0]].sess.wins,
							level.clients[level.sortedClients[0]].sess.losses,
							level.clients[level.sortedClients[1]].pers.netname,
							level.clients[level.sortedClients[1]].sess.wins,
							level.clients[level.sortedClients[1]].sess.losses,
							level.clients[level.sortedClients[2]].pers.netname,
							level.clients[level.sortedClients[2]].sess.wins,
							level.clients[level.sortedClients[2]].sess.losses,
							fraglimit.integer );
					}
					//trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
					//FIXME: This seems to cause problems. But we'd like to reset things whenever a new opponent is set.
				}
			}
		}
		else
		{ //if you have proper num of players then don't try to add again
			g_dontFrickinCheck = qtrue;
		}

		level.warmupTime = 0;
		return;
	}
	else if ( level.warmupTime != 0 ) {
		int		counts[FACTION_NUM_FACTIONS];
		qboolean	notEnough = qfalse;

		if ( level.gametype > GT_TEAM ) {
			counts[FACTION_REBEL] = TeamCount( -1, FACTION_REBEL );
			counts[FACTION_EMPIRE] = TeamCount( -1, FACTION_EMPIRE );

			if (counts[FACTION_EMPIRE] < 1 || counts[FACTION_REBEL] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		/*
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}
		*/

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			if ( g_warmup.integer > 1 ) {
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			} else {
				level.warmupTime = 0;
			}
			trap->SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap->Cvar_Set( "g_restarted", "1" );
			trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}

void G_KickAllBots(void)
{
	int i;
	gclient_t	*cl;

	for ( i=0 ; i< sv_maxclients.integer ; i++ )
	{
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) )
		{
			continue;
		}
		trap->SendConsoleCommand( EXEC_INSERT, va("clientkick %d\n", i) );
	}
}

/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap->SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );

		if (level.votingGametype)
		{
			if (level.gametype != level.votingGametypeTo)
			{ //If we're voting to a different game type, be sure to refresh all the map stuff
				const char *nextMap = G_RefreshNextMap(level.votingGametypeTo, qtrue);

				if (level.votingGametypeTo == GT_SIEGE)
				{ //ok, kick all the bots, cause the aren't supported!
                    G_KickAllBots();
					//just in case, set this to 0 too... I guess...maybe?
					//trap->Cvar_Set("bot_minplayers", "0");
				}

				if (nextMap && nextMap[0])
				{
					trap->SendConsoleCommand( EXEC_APPEND, va("map %s\n", nextMap ) );
				}
			}
			else
			{ //otherwise, just leave the map until a restart
				G_RefreshNextMap(level.votingGametypeTo, qfalse);
			}

			if (g_fraglimitVoteCorrection.integer)
			{ //This means to auto-correct fraglimit when voting to and from duel.
				const int currentGT = level.gametype;
				const int currentFL = fraglimit.integer;
				const int currentTL = timelimit.integer;

				if ((level.votingGametypeTo == GT_DUEL || level.votingGametypeTo == GT_POWERDUEL) && currentGT != GT_DUEL && currentGT != GT_POWERDUEL)
				{
					if (currentFL > 3 || !currentFL)
					{ //if voting to duel, and fraglimit is more than 3 (or unlimited), then set it down to 3
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 3\n");
					}
					if (currentTL)
					{ //if voting to duel, and timelimit is set, make it unlimited
						trap->SendConsoleCommand(EXEC_APPEND, "timelimit 0\n");
					}
				}
				else if ((level.votingGametypeTo != GT_DUEL && level.votingGametypeTo != GT_POWERDUEL) &&
					(currentGT == GT_DUEL || currentGT == GT_POWERDUEL))
				{
					if (currentFL && currentFL < 20)
					{ //if voting from duel, an fraglimit is less than 20, then set it up to 20
						trap->SendConsoleCommand(EXEC_APPEND, "fraglimit 20\n");
					}
				}
			}

			level.votingGametype = qfalse;
			level.votingGametypeTo = 0;
		}
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time-level.voteTime >= VOTE_TIME || level.voteYes + level.voteNo == 0 ) {
		trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"), level.voteStringClean) );
	}
	else {
		if ( level.voteYes > level.numVotingClients/2 ) {
			// execute the command, then remove the vote
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEPASSED"), level.voteStringClean) );
			level.voteExecuteTime = level.time + level.voteExecuteDelay;
		}

		// same behavior as a timeout
		else if ( level.voteNo >= (level.numVotingClients+1)/2 )
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "VOTEFAILED"), level.voteStringClean) );

		else // still waiting for a majority
			return;
	}
	level.voteTime = 0;
	trap->SetConfigstring( CS_VOTE_TIME, "" );
}

/*
==================
PrintTeam
==================
*/
void PrintTeam(int team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap->SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(int team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	PrintTeam(team, va("print \"%s %s\n\"", level.clients[client].pers.netname, G_GetStringEdString("MP_SVGAME", "NEWTEAMLEADER")) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( int team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		if ( i >= level.maxclients ) {
			for ( i = 0 ; i < level.maxclients ; i++ ) {
				if ( level.clients[i].sess.sessionTeam != team )
					continue;
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( int team ) {
	int cs_offset;

	if ( team == FACTION_EMPIRE )
		cs_offset = 0;
	else if ( team == FACTION_REBEL )
		cs_offset = 1;
	else
		return;

	if ( level.teamVoteExecuteTime[cs_offset] && level.teamVoteExecuteTime[cs_offset] < level.time ) {
		level.teamVoteExecuteTime[cs_offset] = 0;
		if ( !Q_strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
			//set the team leader
			SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
		}
		else {
			trap->SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
		}
	}

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}

	if ( level.time-level.teamVoteTime[cs_offset] >= VOTE_TIME || level.teamVoteYes[cs_offset] + level.teamVoteNo[cs_offset] == 0 ) {
		trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED"), level.teamVoteStringClean[cs_offset]) );
	}
	else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEPASSED"), level.teamVoteStringClean[cs_offset]) );
			level.voteExecuteTime = level.time + 3000;
		}

		// same behavior as a timeout
		else if ( level.teamVoteNo[cs_offset] >= (level.numteamVotingClients[cs_offset]+1)/2 )
			trap->SendServerCommand( -1, va("print \"%s (%s)\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEFAILED"), level.teamVoteStringClean[cs_offset]) );

		else // still waiting for a majority
			return;
	}
	level.teamVoteTime[cs_offset] = 0;
	trap->SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );
}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		char password[MAX_INFO_STRING];
		char *c = password;
		lastMod = g_password.modificationCount;

		strcpy( password, g_password.string );
		while( *c )
		{
			if ( *c == '%' )
			{
				*c = '.';
			}
			c++;
		}
		trap->Cvar_Set("g_password", password );

		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap->Cvar_Set( "g_needpass", "1" );
		} else {
			trap->Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent, qboolean calledFromAIThread) {
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		goto runicarus;
	}
	if (thinktime > level.time) {
		goto runicarus;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		//trap->Error( ERR_DROP, "NULL ent->think");
		goto runicarus;
	}
	ent->think (ent);

runicarus:
#ifndef __NO_ICARUS__
	if (ent->inuse)
	{
		trap->ICARUS_MaintainTaskManager(ent->s.number);
	}
#else //__NO_ICARUS__
	if (ent->inuse)
	{

	}
#endif //__NO_ICARUS__
}

int g_LastFrameTime = 0;
int g_TimeSinceLastFrame = 0;

qboolean gDoSlowMoDuel = qfalse;
int gSlowMoDuelTime = 0;

//#define _G_FRAME_PERFANAL

void NAV_CheckCalcPaths( void )
{
	if ( navCalcPathTime && navCalcPathTime < level.time )
	{//first time we've ever loaded this map...
		vmCvar_t	mapname;
		vmCvar_t	ckSum;

		trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap->Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

		//clear all the failed edges
		trap->Nav_ClearAllFailedEdges();

		//Calculate all paths
		NAV_CalculatePaths( mapname.string, ckSum.integer );

		trap->Nav_CalculatePaths(qfalse);

#ifndef FINAL_BUILD
		if ( fatalErrors )
		{
			Com_Printf( S_COLOR_RED"Not saving .nav file due to fatal nav errors\n" );
		}
		else
#endif
		if ( trap->Nav_Save( mapname.string, ckSum.integer ) == qfalse )
		{
			Com_Printf("Unable to save navigations data for map \"%s\" (checksum:%d)\n", mapname.string, ckSum.integer );
		}
		navCalcPathTime = 0;
	}
}

//so shared code can get the local time depending on the side it's executed on
int BG_GetTime(void)
{
	return level.time;
}

void WP_SaberStartMissileBlockCheck(gentity_t *self, usercmd_t *ucmd);

int ACTIVE_ENTS[MAX_GENTITIES];
int ACTIVE_ENTS_NUM = 0;

#ifdef __NPC_DYNAMIC_THREADS__
#include <thread>
#include <mutex>

int ACTIVE_NPCS[MAX_GENTITIES];
int ACTIVE_NPCS_NUM = 0;

std::thread *ai_thread1 = NULL;
std::thread *ai_thread2 = NULL;
std::thread *ai_thread3 = NULL;
std::thread *ai_thread4 = NULL;

std::mutex active_thread_mutex[4];

qboolean ai_shutdown1 = qfalse;
qboolean ai_shutdown2 = qfalse;
qboolean ai_shutdown3 = qfalse;
qboolean ai_shutdown4 = qfalse;

qboolean ai_running1 = qfalse;
qboolean ai_running2 = qfalse;
qboolean ai_running3 = qfalse;
qboolean ai_running4 = qfalse;

int ai_next_think1 = 0;
int ai_next_think2 = 0;
int ai_next_think3 = 0;
int ai_next_think4 = 0;

int AI_THREAD_ENTITIES_START[4] = { -1 };
int AI_THREAD_ENTITIES_END[4] = { -1 };

#define AI_THINK_TIME 50

void AI_ThinkThread1(void)
{
	while (!ai_shutdown1)
	{
		while (ai_next_think1 > level.time)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			if (ai_shutdown1)
				break;
		}

		active_thread_mutex[0].lock();

		if (ai_shutdown1)
			break;

		for (int aEnt = AI_THREAD_ENTITIES_START[0]; aEnt < ACTIVE_NPCS_NUM && aEnt < AI_THREAD_ENTITIES_END[0]; aEnt++)
		{
			try {
				int i = ACTIVE_NPCS[aEnt];

				gentity_t *ent = &g_entities[i];

				if (!(ent && ent->inuse && ent->client && ent->r.linked && ent->s.eType == ET_NPC))
					continue;

				G_RunThink(ent, qtrue);

				// turn off any expired powerups
				for (int j = 0; j < MAX_POWERUPS; j++) {
					if (ent->client && ent->client->ps.powerups[j] < level.time) {
						ent->client->ps.powerups[j] = 0;
					}
				}


				if (ent->client)
				{
					JKG_DoPlayerDamageEffects(ent);
					WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
					WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
					WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
				}
			} catch (int code ) {
				// TODO: Debug dump...
				trap->Print("AI think failed on entity %i.\n", aEnt);
			}
		}

		active_thread_mutex[0].unlock();

		ai_next_think1 = level.time + AI_THINK_TIME;
	}

	trap->Print("Spun down AI thread #1.\n");
	ai_running1 = qfalse;
}

void AI_ThinkThread2(void)
{
	while (!ai_shutdown2)
	{
		while (ai_next_think2 > level.time)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			if (ai_shutdown2)
				break;
		}

		if (ai_shutdown2)
			break;

		active_thread_mutex[1].lock();

		for (int aEnt = AI_THREAD_ENTITIES_START[1]; aEnt < ACTIVE_NPCS_NUM && aEnt < AI_THREAD_ENTITIES_END[1]; aEnt++)
		{
			try {
				int i = ACTIVE_NPCS[aEnt];

				gentity_t *ent = &g_entities[i];

				if (!(ent && ent->inuse && ent->client && ent->r.linked && ent->s.eType == ET_NPC))
					continue;

				G_RunThink(ent, qtrue);

				// turn off any expired powerups
				for (int j = 0; j < MAX_POWERUPS; j++) {
					if (ent->client && ent->client->ps.powerups[j] < level.time) {
						ent->client->ps.powerups[j] = 0;
					}
				}


				if (ent->client)
				{
					JKG_DoPlayerDamageEffects(ent);
					WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
					WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
					WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
				}
			}
			catch (int code) {
				// TODO: Debug dump...
				trap->Print("AI think failed on entity %i.\n", aEnt);
			}
		}

		active_thread_mutex[1].unlock();

		ai_next_think2 = level.time + AI_THINK_TIME;
	}

	trap->Print("Spun down AI thread #2.\n");
	ai_running2 = qfalse;
}

void AI_ThinkThread3(void)
{
	while (!ai_shutdown3)
	{
		while (ai_next_think3 > level.time)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			if (ai_shutdown3)
				break;
		}

		if (ai_shutdown3)
			break;

		active_thread_mutex[2].lock();

		for (int aEnt = AI_THREAD_ENTITIES_START[2]; aEnt < ACTIVE_NPCS_NUM && aEnt < AI_THREAD_ENTITIES_END[2]; aEnt++)
		{
			try {
				int i = ACTIVE_NPCS[aEnt];

				gentity_t *ent = &g_entities[i];

				if (!(ent && ent->inuse && ent->client && ent->r.linked && ent->s.eType == ET_NPC))
					continue;

				G_RunThink(ent, qtrue);

				// turn off any expired powerups
				for (int j = 0; j < MAX_POWERUPS; j++) {
					if (ent->client && ent->client->ps.powerups[j] < level.time) {
						ent->client->ps.powerups[j] = 0;
					}
				}


				if (ent->client)
				{
					JKG_DoPlayerDamageEffects(ent);
					WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
					WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
					WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
				}
			}
			catch (int code) {
				// TODO: Debug dump...
				trap->Print("AI think failed on entity %i.\n", aEnt);
			}
		}

		active_thread_mutex[2].unlock();

		ai_next_think3 = level.time + AI_THINK_TIME;
	}

	trap->Print("Spun down AI thread #3.\n");
	ai_running3 = qfalse;
}

void AI_ThinkThread4(void)
{
	while (!ai_shutdown4)
	{
		while (ai_next_think4 > level.time)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			if (ai_shutdown4)
				break;
		}

		if (ai_shutdown4)
			break;

		active_thread_mutex[3].lock();

		for (int aEnt = AI_THREAD_ENTITIES_START[3]; aEnt < ACTIVE_NPCS_NUM && aEnt < AI_THREAD_ENTITIES_END[3]; aEnt++)
		{
			try {
				int i = ACTIVE_NPCS[aEnt];

				gentity_t *ent = &g_entities[i];

				if (!(ent && ent->inuse && ent->client && ent->r.linked && ent->s.eType == ET_NPC))
					continue;

				G_RunThink(ent, qtrue);

				// turn off any expired powerups
				for (int j = 0; j < MAX_POWERUPS; j++) {
					if (ent->client && ent->client->ps.powerups[j] < level.time) {
						ent->client->ps.powerups[j] = 0;
					}
				}

				if (ent->client)
				{
					JKG_DoPlayerDamageEffects(ent);
					WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
					WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
					WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
				}
			}
			catch (int code) {
				// TODO: Debug dump...
				trap->Print("AI think failed on entity %i.\n", aEnt);
			}
		}

		active_thread_mutex[3].unlock();

		ai_next_think4 = level.time + AI_THINK_TIME;
	}

	trap->Print("Spun down AI thread #4.\n");
	ai_running4 = qfalse;
}

void AI_Theads(void)
{
	if (dedicated.integer)
	{// Only thread when dedicated... Fucking G2API threading unsafe crap...
		if (!ai_running1 && ACTIVE_NPCS_NUM > 0)
		{
			ai_thread1 = new std::thread(AI_ThinkThread1);
			ai_thread1->detach();
			trap->Print("Spun up AI thread #1.\n");
			ai_shutdown1 = qfalse;
			ai_running1 = qtrue;
			ai_next_think1 = level.time;
		}

		/*
		if (ai_running1 && ACTIVE_NPCS_NUM <= 0)
		{
			ai_shutdown1 = qtrue;
			//ai_thread1->join();
		}
		*/

		if (!ai_running2 && ACTIVE_NPCS_NUM > 0)//128)
		{
			ai_thread2 = new std::thread(AI_ThinkThread2);
			ai_thread2->detach();
			trap->Print("Spun up AI thread #2.\n");
			ai_shutdown2 = qfalse;
			ai_running2 = qtrue;
			ai_next_think2 = level.time;
		}

		/*
		if (ai_running2 && ACTIVE_NPCS_NUM <= 128)
		{
			ai_shutdown2 = qtrue;
			//ai_thread2->join();
		}
		*/

		if (!ai_running3 && ACTIVE_NPCS_NUM > 0)//256)
		{
			ai_thread3 = new std::thread(AI_ThinkThread3);
			ai_thread3->detach();
			trap->Print("Spun up AI thread #3.\n");
			ai_shutdown3 = qfalse;
			ai_running3 = qtrue;
			ai_next_think3 = level.time;
		}

		/*
		if (ai_running3 && ACTIVE_NPCS_NUM <= 256)
		{
			ai_shutdown3 = qtrue;
			//ai_thread3->join();
		}
		*/

		if (!ai_running4 && ACTIVE_NPCS_NUM > 0)//384)
		{
			ai_thread4 = new std::thread(AI_ThinkThread4);
			ai_thread4->detach();
			trap->Print("Spun up AI thread #4.\n");
			ai_shutdown4 = qfalse;
			ai_running4 = qtrue;
			ai_next_think4 = level.time;
		}

		/*
		if (ai_running4 && ACTIVE_NPCS_NUM <= 384)
		{
			ai_shutdown4 = qtrue;
			//ai_thread4->join();
		}
		*/
	}
	else
	{
		for (int aEnt = 0; aEnt < ACTIVE_NPCS_NUM; aEnt++)
		{
			int i = ACTIVE_NPCS[aEnt];

			gentity_t *ent = &g_entities[i];

			if (!(ent && ent->inuse && ent->client && ent->r.linked && ent->s.eType == ET_NPC))
				continue;

			G_RunThink(ent, qtrue);

			// turn off any expired powerups
			for (int j = 0; j < MAX_POWERUPS; j++) {
				if (ent->client->ps.powerups[j] < level.time) {
					ent->client->ps.powerups[j] = 0;
				}
			}

			JKG_DoPlayerDamageEffects(ent);
			WP_ForcePowersUpdate(ent, &ent->client->pers.cmd);
			WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
			WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
		}
	}
}

void AI_ThreadsShutdown(void)
{
	if (dedicated.integer)
	{// Only thread when dedicated... Fucking G2API threading unsafe crap...
		ACTIVE_NPCS_NUM = 0;

		ai_shutdown1 = qtrue;
		ai_shutdown2 = qtrue;
		ai_shutdown3 = qtrue;
		ai_shutdown4 = qtrue;

		trap->Print("Waiting for AI threads to shut down...\n");

		while (ai_running1 || ai_running2 || ai_running3 || ai_running4)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		trap->Print("AI threads have shut down...\n");

		ai_thread1 = NULL;
		ai_thread2 = NULL;
		ai_thread3 = NULL;
		ai_thread4 = NULL;
	}
}
#endif //__NPC_DYNAMIC_THREADS__

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/

void JKG_DamagePlayers(void);
void AI_UpdateGroups( void );
void ClearPlayerAlertEvents( void );
void SiegeCheckTimers(void);
extern void Jedi_Decloak( gentity_t *self );
qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );
extern void NPC_PrintNumActiveNPCs(void);

int g_siegeRespawnCheck = 0;
void SetMoverState( gentity_t *ent, moverState_t moverState, int time );
int FRAME_TIME = 0;

void G_RunFrame( int levelTime ) {
#ifdef _G_FRAME_PERFANAL
	int			iTimer_ItemRun = 0;
	int			iTimer_ROFF = 0;
	int			iTimer_ClientEndframe = 0;
	int			iTimer_GameChecks = 0;
	int			iTimer_Queues = 0;
	void		*timer_ItemRun;
	void		*timer_ROFF;
	void		*timer_ClientEndframe;
	void		*timer_GameChecks;
	void		*timer_Queues;
#endif
	FRAME_TIME = trap->Milliseconds();

#ifdef __NPC_DYNAMIC_THREADS__
	//active_thread_mutex[0].lock();
	//active_thread_mutex[1].lock();
	//active_thread_mutex[2].lock();
	//active_thread_mutex[3].lock();

	ACTIVE_NPCS_NUM = 0;
#endif //__NPC_DYNAMIC_THREADS__
	ACTIVE_ENTS_NUM = 0;
	

	if (level.gametype == GT_SIEGE &&
		g_siegeRespawn.integer &&
		g_siegeRespawnCheck < level.time)
	{ //check for a respawn wave
		gentity_t *clEnt;
		for ( int i=0; i < MAX_CLIENTS; i++ )
		{
			clEnt = &g_entities[i];

			if (clEnt->inuse && clEnt->client &&
				clEnt->client->tempSpectate >= level.time &&
				clEnt->client->sess.sessionTeam != FACTION_SPECTATOR)
			{
				ClientRespawn(clEnt);
				clEnt->client->tempSpectate = 0;
			}
		}

		g_siegeRespawnCheck = level.time + g_siegeRespawn.integer * 1000;
	}

	if (gDoSlowMoDuel)
	{
		if (level.restarted)
		{
			char buf[128];
			float tFVal = 0;

			trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

			tFVal = atof(buf);

			trap->Cvar_Set("timescale", "1");
			if (tFVal == 1.0f)
			{
				gDoSlowMoDuel = qfalse;
			}
		}
		else
		{
			float timeDif = (level.time - gSlowMoDuelTime); //difference in time between when the slow motion was initiated and now
			float useDif = 0; //the difference to use when actually setting the timescale

			if (timeDif < 150)
			{
				trap->Cvar_Set("timescale", "0.1f");
			}
			else if (timeDif < 1150)
			{
				useDif = (timeDif/1000); //scale from 0.1 up to 1
				if (useDif < 0.1f)
				{
					useDif = 0.1f;
				}
				if (useDif > 1.0f)
				{
					useDif = 1.0f;
				}
				trap->Cvar_Set("timescale", va("%f", useDif));
			}
			else
			{
				char buf[128];
				float tFVal = 0;

				trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

				tFVal = atof(buf);

				trap->Cvar_Set("timescale", "1");
				if (timeDif > 1500 && tFVal == 1.0f)
				{
					gDoSlowMoDuel = qfalse;
				}
			}
		}
	}

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
#ifdef __NPC_DYNAMIC_THREADS__
		//active_thread_mutex[0].unlock();
		//active_thread_mutex[1].unlock();
		//active_thread_mutex[2].unlock();
		//active_thread_mutex[3].unlock();
#endif //__NPC_DYNAMIC_THREADS__
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	if (g_allowNPC.integer)
	{
		NAV_CheckCalcPaths();
	}

	AI_UpdateGroups();

	if (g_allowNPC.integer)
	{
		if ( d_altRoutes.integer )
		{
			trap->Nav_CheckAllFailedEdges();
		}
		trap->Nav_ClearCheckedNodes();

		/*
		//remember last waypoint, clear current one
		for ( int i = 0; i < level.num_entities ; i++)
		{
			gentity_t *ent = &g_entities[i];

			if ( !ent->inuse )
				continue;

			if ( ent->waypoint != WAYPOINT_NONE
				&& ent->noWaypointTime < level.time )
			{
				ent->lastWaypoint = ent->waypoint;
				ent->waypoint = WAYPOINT_NONE;
			}
			if ( d_altRoutes.integer )
			{
				trap->Nav_CheckFailedNodes( (sharedEntity_t *)ent );
			}
		}
		*/

		//Look to clear out old events
		ClearPlayerAlertEvents();
	}

	g_TimeSinceLastFrame = (level.time - g_LastFrameTime);

	// get any cvar changes
	G_UpdateCvars();

	// Damage players
	JKG_DamagePlayers();


#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ItemRun);
#endif
	//
	// go through all allocated objects
	//

	for (int i = 0; i < level.num_entities; i++)
	{// Lets see how much stuff needs to do a full think...
		gentity_t *ent = &g_entities[i];

		if (!ent->inuse) {
			continue;
		}

		// clear events that are too old
		if (level.time - ent->eventTime > EVENT_VALID_MSEC) {
			if (ent->s.event) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if (ent->client) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if (ent->freeAfterEvent) {
				// tempEntities or dropped items completely go away after their event
				if (ent->s.eFlags & EF_SOUNDTRACKER)
				{ //don't trigger the event again..
					ent->s.event = 0;
					ent->s.eventParm = 0;
					ent->s.eType = 0;
					ent->eventTime = 0;
				}
				else
				{
					G_FreeEntity(ent);
					continue;
				}
			}
			else if (ent->unlinkAfterEvent) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap->UnlinkEntity((sharedEntity_t *)ent);
			}
		}

		// temporary entities don't think
		if (ent->freeAfterEvent) {
			continue;
		}

		if (!ent->r.linked && ent->neverFree) {
			continue;
		}

		ACTIVE_ENTS[ACTIVE_ENTS_NUM] = i;
		ACTIVE_ENTS_NUM++;
		
#ifdef __NPC_DYNAMIC_THREADS__
		if (ent->s.eType == ET_NPC)
		{
			ACTIVE_NPCS[ACTIVE_NPCS_NUM] = i;
			ACTIVE_NPCS_NUM++;
		}
#endif //__NPC_DYNAMIC_THREADS__
	}

#ifdef __NPC_DYNAMIC_THREADS__
	int NUM_ENTS_PER_THREAD = ACTIVE_NPCS_NUM / 4;
	AI_THREAD_ENTITIES_START[0] = 0;
	AI_THREAD_ENTITIES_END[0] = NUM_ENTS_PER_THREAD;
	AI_THREAD_ENTITIES_START[1] = NUM_ENTS_PER_THREAD;
	AI_THREAD_ENTITIES_END[1] = NUM_ENTS_PER_THREAD * 2;
	AI_THREAD_ENTITIES_START[2] = NUM_ENTS_PER_THREAD * 2;
	AI_THREAD_ENTITIES_END[2] = NUM_ENTS_PER_THREAD * 3;
	AI_THREAD_ENTITIES_START[3] = NUM_ENTS_PER_THREAD * 3;
	AI_THREAD_ENTITIES_END[3] = ACTIVE_NPCS_NUM;
	//active_thread_mutex[0].unlock();
	//active_thread_mutex[1].unlock();
	//active_thread_mutex[2].unlock();
	//active_thread_mutex[3].unlock();

	AI_Theads();
#endif //__NPC_DYNAMIC_THREADS__

	for (int aEnt = 0; aEnt < ACTIVE_ENTS_NUM; aEnt++)
	{
		int i = ACTIVE_ENTS[aEnt];

		gentity_t *ent = &g_entities[i];

		if ( !ent->inuse ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
#if 0 //use if body dragging enabled?
			if (ent->s.eType == ET_BODY)
			{ //special case for bodies
				float grav = 3.0f;
				float mass = 0.14f;
				float bounce = 1.15f;

				G_RunExPhys(ent, grav, mass, bounce, qfalse, NULL, 0);
			}
			else
			{
				G_RunItem( ent );
			}
#else
			G_RunItem( ent );
#endif
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if (g_allowNPC.integer)
		{
			//if (ent->s.eType == ET_NPC)
			{
				//remember last waypoint, clear current one
				if (ent->waypoint != WAYPOINT_NONE
					&& ent->noWaypointTime < level.time)
				{
					ent->lastWaypoint = ent->waypoint;
					ent->waypoint = WAYPOINT_NONE;
				}
				if (d_altRoutes.integer)
				{
					trap->Nav_CheckFailedNodes((sharedEntity_t *)ent);
				}
			}
		}

		//fix for self-deactivating areaportals in Siege
		if ( ent->s.eType == ET_MOVER && level.gametype == GT_SIEGE && level.intermissiontime)
		{
			if ( !Q_stricmp("func_door", ent->classname) && ent->moverState != MOVER_POS1 )
			{
				SetMoverState( ent, MOVER_POS1, level.time );
				if ( ent->teammaster == ent || !ent->teammaster )
				{
					trap->AdjustAreaPortalState( (sharedEntity_t *)ent, qfalse );
				}

				//stop the looping sound
				ent->s.loopSound = 0;
				ent->s.loopIsSoundset = qfalse;
			}
			continue;
		}

		if ( ent->client && (ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC) )
		{// UQ1: NPCs can hack/use too!!!
			if (ent->client->isHacking > MAX_CLIENTS)
			{ //hacking checks
				gentity_t *hacked = &g_entities[ent->client->isHacking];
				vec3_t angDif;

				VectorSubtract(ent->client->ps.viewangles, ent->client->hackingAngles, angDif);

				//keep him in the "use" anim
				if (ent->client->ps.torsoAnim != BOTH_CONSOLE1)
				{
					G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_CONSOLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				}
				else
				{
					ent->client->ps.torsoTimer = 500;
				}

				ent->client->ps.weaponTime = ent->client->ps.torsoTimer;

				if (!(ent->client->pers.cmd.buttons & BUTTON_USE) && (ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC))
				{ //have to keep holding use
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!hacked->inuse)
				{ //shouldn't happen, but safety first
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (!G_PointInBounds( ent->client->ps.origin, hacked->r.absmin, hacked->r.absmax ))
				{ //they stepped outside the thing they're hacking, so reset hacking time
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if (VectorLength(angDif) > 10.0f && (ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC))
				{ //must remain facing generally the same angle as when we start (UQ1: But only for players)
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
			}
		}

		if ( i < MAX_CLIENTS )
		{
			G_CheckClientTimeouts ( ent );

			if (ent->client->inSpaceIndex && ent->client->inSpaceIndex != ENTITYNUM_NONE)
			{ //we're in space, check for suffocating and for exiting
                gentity_t *spacetrigger = &g_entities[ent->client->inSpaceIndex];

				if (!spacetrigger->inuse ||
					!G_PointInBounds(ent->client->ps.origin, spacetrigger->r.absmin, spacetrigger->r.absmax))
				{ //no longer in space then I suppose
                    ent->client->inSpaceIndex = 0;
				}
				else
				{ //check for suffocation
                    if (ent->client->inSpaceSuffocation < level.time)
					{ //suffocate!
						if (ent->health > 0 && ent->takedamage)
						{ //if they're still alive..
							G_Damage(ent, spacetrigger, spacetrigger, NULL, ent->client->ps.origin, Q_irand(50, 70), DAMAGE_NO_ARMOR, MOD_SUICIDE);

							if (ent->health > 0)
							{ //did that last one kill them?
								//play the choking sound
								G_EntitySound(ent, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )));

								//make them grasp their throat
								ent->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
								ent->client->ps.forceHandExtendTime = level.time + 2000;
							}
						}

						ent->client->inSpaceSuffocation = level.time + Q_irand(100, 200);
					}
				}
			}

#define JETPACK_DEFUEL_RATE			400 //approx. 20 seconds of idle use from a fully charged fuel amt
#define JETPACK_REFUEL_RATE			300 //seems fair
#define JETPACK_COOLDOWN			5
			if (ent->client->jetPackOn)
			{ //using jetpack, drain fuel
				if (ent->client->jetPackDebReduce < level.time)
				{
					if (ent->client->pers.cmd.forwardmove || ent->client->pers.cmd.upmove || ent->client->pers.cmd.rightmove)
					{ //only use fuel when actually boosting.
						if (ent->client->pers.cmd.upmove > 0)
						{ //take more if they're thrusting
							ent->client->ps.jetpackFuel -= 2;
						}
						else
						{
							ent->client->ps.jetpackFuel--;
						}
					}
					if (ent->client->ps.jetpackFuel <= 0)
					{ //turn it off
						ent->client->ps.jetpackFuel = 0;
						Jetpack_Off(ent);
					}
					ent->client->jetPackDebReduce = level.time + JETPACK_DEFUEL_RATE;
				}
			}
			else if (ent->client->ps.jetpackFuel < 100)
			{ //recharge jetpack
				if (ent->client->jetPackDebRecharge < level.time)
				{
					ent->client->ps.jetpackFuel++;
					ent->client->jetPackDebRecharge = level.time + JETPACK_REFUEL_RATE;
				}
			}

#define CLOAK_DEFUEL_RATE		200 //approx. 20 seconds of idle use from a fully charged fuel amt
#define CLOAK_REFUEL_RATE		150 //seems fair
			if (ent->client->ps.powerups[PW_CLOAKED])
			{ //using cloak, drain battery
				if (ent->client->cloakDebReduce < level.time)
				{
					ent->client->ps.cloakFuel--;

					if (ent->client->ps.cloakFuel <= 0)
					{ //turn it off
						ent->client->ps.cloakFuel = 0;
						Jedi_Decloak(ent);
					}
					ent->client->cloakDebReduce = level.time + CLOAK_DEFUEL_RATE;
				}
			}
			else if (ent->client->ps.cloakFuel < 100)
			{ //recharge cloak
				if (ent->client->cloakDebRecharge < level.time)
				{
					ent->client->ps.cloakFuel++;
					ent->client->cloakDebRecharge = level.time + CLOAK_REFUEL_RATE;
				}
			}

			if (level.gametype == GT_SIEGE &&
				ent->client->siegeClass != -1 &&
				(bgSiegeClasses[ent->client->siegeClass].classflags & (1<<CFL_STATVIEWER)))
			{ //see if it's time to send this guy an update of extended info
				if (ent->client->siegeEDataSend < level.time)
				{
                    G_SiegeClientExData(ent);
					ent->client->siegeEDataSend = level.time + 1000; //once every sec seems ok
				}
			}

			if((!level.intermissiontime)&&!(ent->client->ps.pm_flags&PMF_FOLLOW) && ent->client->sess.sessionTeam != FACTION_SPECTATOR)
			{
				JKG_DoPlayerDamageEffects(ent);
				WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
				WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
				WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
			}

			if (g_allowNPC.integer)
			{
				//This was originally intended to only be done for client 0.
				//Make sure it doesn't slow things down too much with lots of clients in game.
				NAV_FindPlayerWaypoint(i);
			}

#ifndef __NO_ICARUS__
			trap->ICARUS_MaintainTaskManager(ent->s.number);
#endif //__NO_ICARUS__

			G_RunClient( ent );
			continue;
		}
#ifndef __NPC_DYNAMIC_THREADS__
		else if (ent->s.eType == ET_NPC)
		{
			int j;
			// turn off any expired powerups
			for ( j = 0 ; j < MAX_POWERUPS ; j++ ) {
				if ( ent->client->ps.powerups[ j ] < level.time ) {
					ent->client->ps.powerups[ j ] = 0;
				}
			}
		}
#endif //__NPC_DYNAMIC_THREADS__

#ifdef __NPC_DYNAMIC_THREADS__
		if (ent->s.eType != ET_NPC)
		{
			G_RunThink(ent, qfalse);
		}
#else //!__NPC_DYNAMIC_THREADS__
		G_RunThink( ent, qfalse );

		if (ent->s.eType == ET_NPC)
		{
			JKG_DoPlayerDamageEffects(ent);
			WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
			WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
			WP_SaberStartMissileBlockCheck(ent, &ent->client->pers.cmd);
		}
#endif //__NPC_DYNAMIC_THREADS__
	}

	//thinkTime = trap->Milliseconds() - thinkTime;

	//trap->Print("Thinktime was %i ms.\n", thinkTime);

#ifdef _G_FRAME_PERFANAL
	iTimer_ItemRun = trap->PrecisionTimer_End(timer_ItemRun);
#endif

	SiegeCheckTimers();

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ROFF);
#endif
	trap->ROFF_UpdateEntities();
#ifdef _G_FRAME_PERFANAL
	iTimer_ROFF = trap->PrecisionTimer_End(timer_ROFF);
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_ClientEndframe);
#endif
	// perform final fixups on the players
#if 0
	for ( int i=0 ; i < level.maxclients ; i++ ) {
		gentity_t *ent = &g_entities[i];

		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}
#else
	for (int aEnt = 0; aEnt < ACTIVE_ENTS_NUM; aEnt++)
	{
		int i = ACTIVE_ENTS[aEnt];
		gentity_t *ent = &g_entities[i];

		if (ent->inuse && ent->client)
		{
			if (ent->s.eType == ET_PLAYER) 
			{
				ClientEndFrame(ent);
			}
			else if (ent->s.eType == ET_NPC) 
			{
				playerState_t *ps = &ent->client->ps;
				entityState_t *s = &ent->s; 

				//ps->commandTime = level.time - 50;// ((1000 / sv_fps.integer) * 2.0)/*25*/;// 100;

				s->pos.trType = TR_INTERPOLATE;
				//s->pos.trType = TR_LINEAR_STOP;
				
				qboolean FULL_UPDATE = qfalse;

				if (ent->next_full_update <= level.time)
				{
					FULL_UPDATE = qtrue;
				}

				if (FULL_UPDATE || Distance(ent->prev_posTrBase, ps->origin) > 2)
				{
					VectorCopy(ps->origin, s->pos.trBase);
					VectorCopy(s->pos.trBase, ent->prev_posTrBase);
				}
				else
				{
					VectorCopy(ent->prev_posTrBase, s->pos.trBase);
				}

				//if (snap) {
				//	SnapVector(s->pos.trBase);
				//}

				// set the trDelta for flag direction
				if (FULL_UPDATE || Distance(ent->prev_posTrDelta, ps->velocity) > 16)
				{
					VectorCopy(ps->velocity, s->pos.trDelta);
					VectorCopy(s->pos.trDelta, ent->prev_posTrDelta);
				}
				else
				{
					VectorCopy(ent->prev_posTrDelta, s->pos.trDelta);
				}

				s->apos.trType = TR_INTERPOLATE;

				if (FULL_UPDATE || Distance(ent->prev_aposTrBase, ps->viewangles) > 32)
				{
					VectorCopy(ps->viewangles, s->apos.trBase);
					VectorCopy(s->apos.trBase, ent->prev_aposTrBase);
				}
				else
				{
					VectorCopy(ent->prev_aposTrBase, s->apos.trBase);
				}

				//if (snap) {
				//	SnapVector(s->apos.trBase);
				//}

				if (FULL_UPDATE || ent->prev_moveDir - ps->movementDir > 24 || ent->prev_moveDir - ps->movementDir < -24 || ps->movementDir == 0)
				{
					s->angles2[YAW] = ps->movementDir;
					ent->prev_moveDir = ps->movementDir;
				}
				else
				{
					s->angles2[YAW] = ent->prev_moveDir;
				}

				if (FULL_UPDATE || ent->prev_speed - ps->speed > 8 || ent->prev_speed - ps->speed < -8 || ps->speed == 0)
				{
					s->speed = ps->speed;
					ent->prev_speed = ps->speed;
				}
				else
				{
					s->speed = ent->prev_speed;
				}

				if (FULL_UPDATE) ent->next_full_update = level.time + 2000; // Force a full update every 2 seconds...

																			// set maximum extra polation time
				s->pos.trDuration = 1000 / sv_fps.integer;

				s->apos.trType = TR_INTERPOLATE;
			}
		}
	}
#endif
#ifdef _G_FRAME_PERFANAL
	iTimer_ClientEndframe = trap->PrecisionTimer_End(timer_ClientEndframe);
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_GameChecks);
#endif
	// see if it is time to do a tournament restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( FACTION_EMPIRE );
	CheckTeamVote( FACTION_REBEL );

	// for tracking changes
	CheckCvars();

#ifdef _G_FRAME_PERFANAL
	iTimer_GameChecks = trap->PrecisionTimer_End(timer_GameChecks);
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start(&timer_Queues);
#endif
	//At the end of the frame, send out the ghoul2 kill queue, if there is one
	G_SendG2KillQueue();

	if (gQueueScoreMessage)
	{
		if (gQueueScoreMessageTime < level.time)
		{
			SendScoreboardMessageToAllClients();

			gQueueScoreMessageTime = 0;
			gQueueScoreMessage = (qboolean)0;
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_Queues = trap->PrecisionTimer_End(timer_Queues);
#endif



#ifdef _G_FRAME_PERFANAL
	Com_Printf("^5---------------\n^5ItemRun: ^7%i\n^5ROFF: ^7%i\n^5ClientEndframe: ^7%i\n^5GameChecks: ^7%i\n^5Queues: ^7%i\n^5---------------\n",
		iTimer_ItemRun,
		iTimer_ROFF,
		iTimer_ClientEndframe,
		iTimer_GameChecks,
		iTimer_Queues);
#endif

	NPC_PrintNumActiveNPCs();

	g_LastFrameTime = level.time;
}

const char *G_GetStringEdString(char *refSection, char *refName)
{
	/*
	static char text[1024]={0};
	trap->SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
	*/

	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a stringed reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the stringed reference indication and dealt with
	//properly.
	static char text[1024]={0};
	Com_sprintf(text, sizeof(text), "@@@%s", refName);
	return text;
}

static void G_SpawnRMGEntity( void ) {
	if ( G_ParseSpawnVars( qfalse ) )
		G_SpawnGEntityFromSpawnVars( qfalse );
}

static void _G_ROFF_NotetrackCallback( int entID, const char *notetrack ) {
	G_ROFF_NotetrackCallback( &g_entities[entID], notetrack );
}

static int G_ICARUS_PlaySound( void ) {
	T_G_ICARUS_PLAYSOUND *sharedMem = &gSharedBuffer.playSound;
	return Q3_PlaySound( sharedMem->taskID, sharedMem->entID, sharedMem->name, sharedMem->channel );
}
static qboolean G_ICARUS_Set( void ) {
	T_G_ICARUS_SET *sharedMem = &gSharedBuffer.set;
	return Q3_Set( sharedMem->taskID, sharedMem->entID, sharedMem->type_name, sharedMem->data );
}
static void G_ICARUS_Lerp2Pos( void ) {
	T_G_ICARUS_LERP2POS *sharedMem = &gSharedBuffer.lerp2Pos;
	Q3_Lerp2Pos( sharedMem->taskID, sharedMem->entID, sharedMem->origin, sharedMem->nullAngles ? NULL : sharedMem->angles, sharedMem->duration );
}
static void G_ICARUS_Lerp2Origin( void ) {
	T_G_ICARUS_LERP2ORIGIN *sharedMem = &gSharedBuffer.lerp2Origin;
	Q3_Lerp2Origin( sharedMem->taskID, sharedMem->entID, sharedMem->origin, sharedMem->duration );
}
static void G_ICARUS_Lerp2Angles( void ) {
	T_G_ICARUS_LERP2ANGLES *sharedMem = &gSharedBuffer.lerp2Angles;
	Q3_Lerp2Angles( sharedMem->taskID, sharedMem->entID, sharedMem->angles, sharedMem->duration );
}
static int G_ICARUS_GetTag( void ) {
	T_G_ICARUS_GETTAG *sharedMem = &gSharedBuffer.getTag;
	return Q3_GetTag( sharedMem->entID, sharedMem->name, sharedMem->lookup, sharedMem->info );
}
static void G_ICARUS_Lerp2Start( void ) {
	T_G_ICARUS_LERP2START *sharedMem = &gSharedBuffer.lerp2Start;
	Q3_Lerp2Start( sharedMem->entID, sharedMem->taskID, sharedMem->duration );
}
static void G_ICARUS_Lerp2End( void ) {
	T_G_ICARUS_LERP2END *sharedMem = &gSharedBuffer.lerp2End;
	Q3_Lerp2End( sharedMem->entID, sharedMem->taskID, sharedMem->duration );
}
static void G_ICARUS_Use( void ) {
	T_G_ICARUS_USE *sharedMem = &gSharedBuffer.use;
	Q3_Use( sharedMem->entID, sharedMem->target );
}
static void G_ICARUS_Kill( void ) {
	T_G_ICARUS_KILL *sharedMem = &gSharedBuffer.kill;
	Q3_Kill( sharedMem->entID, sharedMem->name );
}
static void G_ICARUS_Remove( void ) {
	T_G_ICARUS_REMOVE *sharedMem = &gSharedBuffer.remove;
	Q3_Remove( sharedMem->entID, sharedMem->name );
}
static void G_ICARUS_Play( void ) {
	T_G_ICARUS_PLAY *sharedMem = &gSharedBuffer.play;
	Q3_Play( sharedMem->taskID, sharedMem->entID, sharedMem->type, sharedMem->name );
}
static int G_ICARUS_GetFloat( void ) {
	T_G_ICARUS_GETFLOAT *sharedMem = &gSharedBuffer.getFloat;
	return Q3_GetFloat( sharedMem->entID, sharedMem->type, sharedMem->name, &sharedMem->value );
}
static int G_ICARUS_GetVector( void ) {
	T_G_ICARUS_GETVECTOR *sharedMem = &gSharedBuffer.getVector;
	return Q3_GetVector( sharedMem->entID, sharedMem->type, sharedMem->name, sharedMem->value );
}
static int G_ICARUS_GetString( void ) {
	T_G_ICARUS_GETSTRING *sharedMem = &gSharedBuffer.getString;
	char *crap = NULL; //I am sorry for this -rww
	char **morecrap = &crap; //and this
	int r = Q3_GetString( sharedMem->entID, sharedMem->type, sharedMem->name, morecrap );

	if ( crap )
		strcpy( sharedMem->value, crap );

	return r;
}
static void G_ICARUS_SoundIndex( void ) {
	T_G_ICARUS_SOUNDINDEX *sharedMem = &gSharedBuffer.soundIndex;
	G_SoundIndex( sharedMem->filename );
}
static int G_ICARUS_GetSetIDForString( void ) {
	T_G_ICARUS_GETSETIDFORSTRING *sharedMem = &gSharedBuffer.getSetIDForString;
	return GetIDForString( setTable, sharedMem->string );
}
static qboolean G_NAV_ClearPathToPoint( int entID, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEnt ) {
	return NAV_ClearPathToPoint( &g_entities[entID], pmins, pmaxs, point, clipmask, okToHitEnt );
}
static qboolean G_NPC_ClearLOS2( int entID, const vec3_t end ) {
	return NPC_ClearLOS2( &g_entities[entID], end );
}
static qboolean	G_NAV_CheckNodeFailedForEnt( int entID, int nodeNum ) {
	return NAV_CheckNodeFailedForEnt( &g_entities[entID], nodeNum );
}

/*
============
GetModuleAPI
============
*/

gameImport_t *trap = NULL;

extern "C"
Q_EXPORT gameExport_t* QDECL GetModuleAPI( int apiVersion, gameImport_t *import )
{
	static gameExport_t ge = {0};

	assert( import );
	trap = import;
	//Com_Printf	= trap->Print;
	//Com_Error	= trap->Error;

	memset( &ge, 0, sizeof( ge ) );

	if ( apiVersion != GAME_API_VERSION ) {
		trap->Print( "Mismatched GAME_API_VERSION: expected %i, got %i\n", GAME_API_VERSION, apiVersion );
		return NULL;
	}

	ge.InitGame							= G_InitGame;
	ge.ShutdownGame						= G_ShutdownGame;
	ge.ClientConnect					= ClientConnect;
	ge.ClientBegin						= ClientBegin;
	ge.ClientUserinfoChanged			= ClientUserinfoChanged;
	ge.ClientDisconnect					= ClientDisconnect;
	ge.ClientCommand					= ClientCommand;
	ge.ClientThink						= ClientThink;
	ge.RunFrame							= G_RunFrame;
	ge.ConsoleCommand					= ConsoleCommand;
	ge.BotAIStartFrame					= BotAIStartFrame;
	ge.ROFF_NotetrackCallback			= _G_ROFF_NotetrackCallback;
	ge.SpawnRMGEntity					= G_SpawnRMGEntity;
	ge.ICARUS_PlaySound					= G_ICARUS_PlaySound;
	ge.ICARUS_Set						= G_ICARUS_Set;
	ge.ICARUS_Lerp2Pos					= G_ICARUS_Lerp2Pos;
	ge.ICARUS_Lerp2Origin				= G_ICARUS_Lerp2Origin;
	ge.ICARUS_Lerp2Angles				= G_ICARUS_Lerp2Angles;
	ge.ICARUS_GetTag					= G_ICARUS_GetTag;
	ge.ICARUS_Lerp2Start				= G_ICARUS_Lerp2Start;
	ge.ICARUS_Lerp2End					= G_ICARUS_Lerp2End;
	ge.ICARUS_Use						= G_ICARUS_Use;
	ge.ICARUS_Kill						= G_ICARUS_Kill;
	ge.ICARUS_Remove					= G_ICARUS_Remove;
	ge.ICARUS_Play						= G_ICARUS_Play;
	ge.ICARUS_GetFloat					= G_ICARUS_GetFloat;
	ge.ICARUS_GetVector					= G_ICARUS_GetVector;
	ge.ICARUS_GetString					= G_ICARUS_GetString;
	ge.ICARUS_SoundIndex				= G_ICARUS_SoundIndex;
	ge.ICARUS_GetSetIDForString			= G_ICARUS_GetSetIDForString;
	ge.NAV_ClearPathToPoint				= G_NAV_ClearPathToPoint;
	ge.NPC_ClearLOS2					= G_NPC_ClearLOS2;
	ge.NAVNEW_ClearPathBetweenPoints	= NAVNEW_ClearPathBetweenPoints;
	ge.NAV_CheckNodeFailedForEnt		= G_NAV_CheckNodeFailedForEnt;
	ge.NAV_EntIsUnlockedDoor			= G_EntIsUnlockedDoor;
	ge.NAV_EntIsDoor					= G_EntIsDoor;
	ge.NAV_EntIsBreakable				= G_EntIsBreakable;
	ge.NAV_EntIsRemovableUsable			= G_EntIsRemovableUsable;
	ge.NAV_FindCombatPointWaypoints		= CP_FindCombatPointWaypoints;
	ge.BG_GetItemIndexByTag				= BG_GetItemIndexByTag;

	return &ge;
}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 )
{
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;

	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;

	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect( arg0, (qboolean)arg1, (qboolean)arg2 );

	case GAME_CLIENT_THINK:
		ClientThink( arg0, NULL );
		return 0;

	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;

	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;

	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0, qtrue );
		return 0;

	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;

	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;

	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();

	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );

	case GAME_ROFF_NOTETRACK_CALLBACK:
		_G_ROFF_NotetrackCallback( arg0, (const char *)arg1 );
		return 0;

	case GAME_SPAWN_RMG_ENTITY:
		G_SpawnRMGEntity();
		return 0;

	case GAME_ICARUS_PLAYSOUND:
		return G_ICARUS_PlaySound();

	case GAME_ICARUS_SET:
		return G_ICARUS_Set();

	case GAME_ICARUS_LERP2POS:
		G_ICARUS_Lerp2Pos();
		return 0;

	case GAME_ICARUS_LERP2ORIGIN:
		G_ICARUS_Lerp2Origin();
		return 0;

	case GAME_ICARUS_LERP2ANGLES:
		G_ICARUS_Lerp2Angles();
		return 0;

	case GAME_ICARUS_GETTAG:
		return G_ICARUS_GetTag();

	case GAME_ICARUS_LERP2START:
		G_ICARUS_Lerp2Start();
		return 0;

	case GAME_ICARUS_LERP2END:
		G_ICARUS_Lerp2End();
		return 0;

	case GAME_ICARUS_USE:
		G_ICARUS_Use();
		return 0;

	case GAME_ICARUS_KILL:
		G_ICARUS_Kill();
		return 0;

	case GAME_ICARUS_REMOVE:
		G_ICARUS_Remove();
		return 0;

	case GAME_ICARUS_PLAY:
		G_ICARUS_Play();
		return 0;

	case GAME_ICARUS_GETFLOAT:
		return G_ICARUS_GetFloat();

	case GAME_ICARUS_GETVECTOR:
		return G_ICARUS_GetVector();

	case GAME_ICARUS_GETSTRING:
		return G_ICARUS_GetString();

	case GAME_ICARUS_SOUNDINDEX:
		G_ICARUS_SoundIndex();
		return 0;

	case GAME_ICARUS_GETSETIDFORSTRING:
		return G_ICARUS_GetSetIDForString();

	case GAME_NAV_CLEARPATHTOPOINT:
		return G_NAV_ClearPathToPoint( arg0, (float *)arg1, (float *)arg2, (float *)arg3, arg4, arg5 );

	case GAME_NAV_CLEARLOS:
		return G_NPC_ClearLOS2( arg0, (const float *)arg1 );

	case GAME_NAV_CLEARPATHBETWEENPOINTS:
		return NAVNEW_ClearPathBetweenPoints((float *)arg0, (float *)arg1, (float *)arg2, (float *)arg3, arg4, arg5);

	case GAME_NAV_CHECKNODEFAILEDFORENT:
		return NAV_CheckNodeFailedForEnt(&g_entities[arg0], arg1);

	case GAME_NAV_ENTISUNLOCKEDDOOR:
		return G_EntIsUnlockedDoor(arg0);

	case GAME_NAV_ENTISDOOR:
		return G_EntIsDoor(arg0);

	case GAME_NAV_ENTISBREAKABLE:
		return G_EntIsBreakable(arg0);

	case GAME_NAV_ENTISREMOVABLEUSABLE:
		return G_EntIsRemovableUsable(arg0);

	case GAME_NAV_FINDCOMBATPOINTWAYPOINTS:
		CP_FindCombatPointWaypoints();
		return 0;

	case GAME_GETITEMINDEXBYTAG:
		return BG_GetItemIndexByTag(arg0, arg1);
	}

	return -1;
}
