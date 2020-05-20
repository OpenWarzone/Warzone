// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_bot.c

#include "g_local.h"
#include "b_local.h"
#include "ai_dominance_main.h"

#define __NPC_MINPLAYERS__
#define __ALWAYS_TWO_TRAVELLINGVENDORS
#define __WAYPOINTS_PRECHECKED__
//#define __ALL_JEDI_NPCS__ // UQ1: For testing jedi/padawan stuff...
//#define __NPC_SPAWN_DEBUGGING__ // UQ1: For displaying what NPCs spawn in chat log for debugging...


#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	1500

#define BOT_SPAWN_QUEUE_DEPTH	16

extern int villageWaypointsNum;
extern int villageWaypoints[MAX_WPARRAY_SIZE];

static struct botSpawnQueue_s {
	int		clientNum;
	int		spawnTime;
} botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

vmCvar_t bot_minplayers;

float trap_Cvar_VariableValue( const char *var_name ) {
	char buf[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer(var_name, buf, sizeof(buf));
	return atof(buf);
}

/*
===============
G_ParseInfos
===============
*/
int G_ParseInfos( char *buf, int max, char *infos[] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	COM_BeginParseSession ("G_ParseInfos");
	while ( 1 ) {
		token = COM_Parse( (const char **)(&buf) );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( (const char **)(&buf), qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( (const char **)(&buf), qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = (char *) G_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1, "G_ParseInfos");
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

/*
===============
G_LoadArenasFromFile
===============
*/
void G_LoadArenasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "file not found: %s\n", filename );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap->Print( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT );
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	level.arenas.num += G_ParseInfos( buf, MAX_ARENAS - level.arenas.num, &level.arenas.infos[level.arenas.num] );
}

int G_GetMapTypeBits(char *type)
{
	int typeBits = 0;

	if( *type ) {
		if( strstr( type, "ffa" ) ) {
			typeBits |= (1 << GT_FFA);
			typeBits |= (1 << GT_TEAM);
			typeBits |= (1 << GT_JEDIMASTER);
		}
		if( strstr( type, "holocron" ) ) {
			typeBits |= (1 << GT_HOLOCRON);
		}
		if( strstr( type, "jedimaster" ) ) {
			typeBits |= (1 << GT_JEDIMASTER);
		}
		if( strstr( type, "duel" ) ) {
			typeBits |= (1 << GT_DUEL);
			typeBits |= (1 << GT_POWERDUEL);
		}
		if( strstr( type, "powerduel" ) ) {
			typeBits |= (1 << GT_DUEL);
			typeBits |= (1 << GT_POWERDUEL);
		}
		if( strstr( type, "siege" ) ) {
			typeBits |= (1 << GT_SIEGE);
		}
		if( strstr( type, "ctf" ) ) {
			typeBits |= (1 << GT_CTF);
			typeBits |= (1 << GT_CTY);
			typeBits |= (1 << GT_WARZONE);
		}
		if( strstr( type, "cty" ) ) {
			typeBits |= (1 << GT_CTY);
		}
		if( strstr( type, "coop" ) ) { // UQ1: We support these in nearly all gametypes now :)
			typeBits |= (1 << GT_SINGLE_PLAYER);
			typeBits |= (1 << GT_FFA);
			typeBits |= (1 << GT_TEAM);
			typeBits |= (1 << GT_CTF);
			typeBits |= (1 << GT_WARZONE);
		}
		if( strstr( type, "instance" ) ) {
			typeBits |= (1 << GT_INSTANCE);
		}
		if( strstr( type, "warzone" ) ) {
			typeBits |= (1 << GT_WARZONE);
		}
	} else {
		typeBits |= (1 << GT_FFA);
		typeBits |= (1 << GT_JEDIMASTER);
	}

	// UQ1: All maps work with these gametypes now...
	if (!(typeBits & (1 << GT_FFA))) typeBits |= (1 << GT_FFA);
	if (!(typeBits & (1 << GT_TEAM))) typeBits |= (1 << GT_TEAM);
	if (!(typeBits & (1 << GT_CTF))) typeBits |= (1 << GT_CTF);
	if (!(typeBits & (1 << GT_WARZONE))) typeBits |= (1 << GT_WARZONE);

	return typeBits;
}

qboolean G_DoesMapSupportGametype(const char *mapname, int gametype)
{
	int			typeBits = 0;
	int			thisLevel = -1;
	int			n = 0;
	char		*type = NULL;

	if (!level.arenas.infos[0])
	{
		return qfalse;
	}

	if (!mapname || !mapname[0])
	{
		return qfalse;
	}

	for( n = 0; n < level.arenas.num; n++ )
	{
		type = Info_ValueForKey( level.arenas.infos[n], "map" );

		if (Q_stricmp(mapname, type) == 0)
		{
			thisLevel = n;
			break;
		}
	}

	if (thisLevel == -1)
	{
		return qfalse;
	}

	type = Info_ValueForKey(level.arenas.infos[thisLevel], "type");

	typeBits = G_GetMapTypeBits(type);
	if (typeBits & (1 << gametype))
	{ //the map in question supports the gametype in question, so..
		return qtrue;
	}

	return qfalse;
}

//rww - auto-obtain nextmap. I could've sworn Q3 had something like this, but I guess not.
const char *G_RefreshNextMap(int gametype, qboolean forced)
{
	int			typeBits = 0;
	int			thisLevel = 0;
	int			desiredMap = 0;
	int			n = 0;
	char		*type = NULL;
	qboolean	loopingUp = qfalse;
	vmCvar_t	mapname;

	if (!g_autoMapCycle.integer && !forced)
	{
		return NULL;
	}

	if (!level.arenas.infos[0])
	{
		return NULL;
	}

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	for( n = 0; n < level.arenas.num; n++ )
	{
		type = Info_ValueForKey( level.arenas.infos[n], "map" );

		if (Q_stricmp(mapname.string, type) == 0)
		{
			thisLevel = n;
			break;
		}
	}

	desiredMap = thisLevel;

	n = thisLevel+1;
	while (n != thisLevel)
	{ //now cycle through the arena list and find the next map that matches the gametype we're in
		if (!level.arenas.infos[n] || n >= level.arenas.num)
		{
			if (loopingUp)
			{ //this shouldn't happen, but if it does we have a null entry break in the arena file
			  //if this is the case just break out of the loop instead of sticking in an infinite loop
				break;
			}
			n = 0;
			loopingUp = qtrue;
		}

		type = Info_ValueForKey(level.arenas.infos[n], "type");

		typeBits = G_GetMapTypeBits(type);
		if (typeBits & (1 << gametype))
		{
			desiredMap = n;
			break;
		}

		n++;
	}

	if (desiredMap == thisLevel)
	{ //If this is the only level for this game mode or we just can't find a map for this game mode, then nextmap
	  //will always restart.
		trap->Cvar_Set( "nextmap", "map_restart 0");
	}
	else
	{ //otherwise we have a valid nextmap to cycle to, so use it.
		type = Info_ValueForKey( level.arenas.infos[desiredMap], "map" );
		trap->Cvar_Set( "nextmap", va("map %s", type));
	}

	return Info_ValueForKey( level.arenas.infos[desiredMap], "map" );
}

/*
===============
G_LoadArenas
===============
*/

#define MAX_MAPS 256
#define MAPSBUFSIZE (MAX_MAPS * 64)

void G_LoadArenas( void ) {
#if 0
	int			numdirs;
	char		filename[MAX_QPATH];
	char		dirlist[1024];
	char*		dirptr;
	int			i, n;
	int			dirlen;

	level.arenas.num = 0;

	// get all arenas from .arena files
	numdirs = trap->FS_GetFileList("scripts", ".arena", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		Q_strncpyz( filename, "scripts/", sizeof( filename ) );
		strcat(filename, dirptr);
		G_LoadArenasFromFile(filename);
	}
//	trap->Print( "%i arenas parsed\n", level.arenas.num );

	for( n = 0; n < level.arenas.num; n++ ) {
		Info_SetValueForKey( level.arenas.infos[n], "num", va( "%i", n ) );
	}

	G_RefreshNextMap(level.gametype, qfalse);

#else

	int			numFiles;
	char		filelist[MAPSBUFSIZE];
	char		filename[MAX_QPATH];
	char*		fileptr;
	int			i, n;
	int			len;

	level.arenas.num = 0;

	// get all arenas from .arena files
	numFiles = trap->FS_GetFileList("scripts", ".arena", filelist, ARRAY_LEN(filelist) );

	fileptr  = filelist;
	i = 0;

	if (numFiles > MAX_MAPS)
		numFiles = MAX_MAPS;

	for(; i < numFiles; i++) {
		len = strlen(fileptr);
		Com_sprintf(filename, sizeof(filename), "scripts/%s", fileptr);
		G_LoadArenasFromFile(filename);
		fileptr += len + 1;
	}
//	trap->Print( "%i arenas parsed\n", level.arenas.num );

	for( n = 0; n < level.arenas.num; n++ ) {
		Info_SetValueForKey( level.arenas.infos[n], "num", va( "%i", n ) );
	}

	G_RefreshNextMap(level.gametype, qfalse);
#endif

}

/*
===============
G_GetArenaInfoByNumber
===============
*/
const char *G_GetArenaInfoByMap( const char *map ) {
	int			n;

	for( n = 0; n < level.arenas.num; n++ ) {
		if( Q_stricmp( Info_ValueForKey( level.arenas.infos[n], "map" ), map ) == 0 ) {
			return level.arenas.infos[n];
		}
	}

	return NULL;
}

#if 0
/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin ) {
	char	model[MAX_QPATH];
	char	*skin;

	Q_strncpyz( model, modelAndSkin, sizeof(model) );
	skin = Q_strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = model;
	}

	if( Q_stricmp( skin, "default" ) == 0 ) {
		skin = model;
	}

	trap->SendConsoleCommand( EXEC_APPEND, va( "play sound/player/announce/%s.wav\n", skin ) );
}
#endif

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( int team ) {
#ifndef __MMO__
	int		i, n, num;
	float	skill;
	char	*value, netname[36], *teamstr;
	gclient_t	*cl;

	num = 0;
	for ( n = 0; n < level.bots.num ; n++ ) {
		value = Info_ValueForKey( level.bots.infos[n], "name" );
		//
		for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
				continue;
			}
			if (level.gametype == GT_SIEGE)
			{
				if ( team >= 0 && cl->sess.siegeDesiredTeam != team ) {
					continue;
				}
			}
			else
			{
				if ( team >= 0 && cl->sess.sessionTeam != team ) {
					continue;
				}
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if (i >= sv_maxclients.integer) {
			num++;
		}
	}
	num = random() * num;
	for ( n = 0; n < level.bots.num ; n++ ) {
		value = Info_ValueForKey( level.bots.infos[n], "name" );
		//
		for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
				continue;
			}
			if (level.gametype == GT_SIEGE)
			{
				if ( team >= 0 && cl->sess.siegeDesiredTeam != team ) {
					continue;
				}
			}
			else
			{
				if ( team >= 0 && cl->sess.sessionTeam != team ) {
					continue;
				}
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if (i >= sv_maxclients.integer) {
			num--;
			if (num <= 0) {
				skill = trap->Cvar_VariableIntegerValue( "g_npcspskill" );
				if (team == FACTION_EMPIRE) teamstr = "red";
				else if (team == FACTION_REBEL) teamstr = "blue";
				else teamstr = "";
				Q_strncpyz(netname, value, sizeof(netname));
				Q_CleanStr(netname);
				trap->SendConsoleCommand( EXEC_INSERT, va("addbot \"%s\" %.2f %s %i\n", netname, skill, teamstr, 0) );
				return;
			}
		}
	}
#else //__MMO__
	int		num = irand(0, level.bots.num-1);
	float	skill;
	char	*value, netname[36], *teamstr;

	value = Info_ValueForKey( level.bots.infos[num], "name" );

	skill = trap->Cvar_VariableIntegerValue( "g_npcspskill" );
	if (team == FACTION_EMPIRE) teamstr = "red";
	else if (team == FACTION_REBEL) teamstr = "blue";
	else teamstr = "";
	Q_strncpyz(netname, value, sizeof(netname));
	Q_CleanStr(netname);
	trap->SendConsoleCommand( EXEC_INSERT, va("addbot \"%s\" %.2f %s %i\n", netname, skill, teamstr, 0) );
#endif //__MMO__
}

/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team ) {
	int i;
	gclient_t	*cl;

	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) )
			continue;

		if ( cl->sess.sessionTeam == FACTION_SPECTATOR && cl->sess.spectatorState == SPECTATOR_FOLLOW )
			continue;

		if ( level.gametype == GT_SIEGE && team >= 0 && cl->sess.siegeDesiredTeam != team )
			continue;
		else if ( team >= 0 && cl->sess.sessionTeam != team )
			continue;

		trap->SendConsoleCommand( EXEC_INSERT, va("clientkick %d\n", i) );
		return qtrue;
	}
	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
int G_CountHumanPlayers( int team ) {
	int i, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< sv_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CountBotPlayers
===============
*/
int G_CountBotPlayers( int team ) {
	int i, n, num;

	num = 0;

	for ( i=0 ; i< sv_maxclients.integer ; i++ ) 
	{
		gclient_t *cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if (level.gametype == GT_SIEGE)
		{
			if ( team >= 0 && cl->sess.siegeDesiredTeam != team ) {
				continue;
			}
		}
		else
		{
			if ( team >= 0 && cl->sess.sessionTeam != team ) {
				continue;
			}
		}

		num++;
	}

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}

		num++;
	}
	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void ) {
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if (level.gametype == GT_SIEGE)
	{
		return;
	}

	if (level.intermissiontime) return;
#ifdef __MMO__
	// UQ1: only check once each 100 milliseconds
	if (checkminimumplayers_time > level.time - 100) {
#else //!__MMO__
	//only check once each 10 seconds
	if (checkminimumplayers_time > level.time - 10000) {
#endif //__MMO__
		return;
	}
	checkminimumplayers_time = level.time;
	trap->Cvar_Update(&bot_minplayers);
	minplayers = bot_minplayers.integer;
	if (minplayers <= 0) return;

	if (minplayers > sv_maxclients.integer)
	{
		minplayers = sv_maxclients.integer;
	}

	humanplayers = G_CountHumanPlayers( -1 );
	botplayers = G_CountBotPlayers(	-1 );

	if ((humanplayers+botplayers) < minplayers)
	{
		G_AddRandomBot(-1);
	}
	else if ((humanplayers+botplayers) > minplayers && botplayers)
	{
		// try to remove spectators first
		if (!G_RemoveRandomBot(FACTION_SPECTATOR))
		{
			// just remove the bot that is playing
			G_RemoveRandomBot(-1);
		}
	}

	/*
	if (level.gametype >= GT_TEAM) {
		int humanplayers2, botplayers2;
		if (minplayers >= sv_maxclients.integer / 2) {
			minplayers = (sv_maxclients.integer / 2) -1;
		}

		humanplayers = G_CountHumanPlayers( FACTION_EMPIRE );
		botplayers = G_CountBotPlayers(	FACTION_EMPIRE );
		humanplayers2 = G_CountHumanPlayers( FACTION_REBEL );
		botplayers2 = G_CountBotPlayers( FACTION_REBEL );
		//
		if ((humanplayers+botplayers+humanplayers2+botplayers) < minplayers)
		{
			if ((humanplayers+botplayers) < (humanplayers2+botplayers2))
			{
				G_AddRandomBot( FACTION_EMPIRE );
			}
			else
			{
				G_AddRandomBot( FACTION_REBEL );
			}
		}
		else if ((humanplayers+botplayers+humanplayers2+botplayers) > minplayers && botplayers)
		{
			if ((humanplayers+botplayers) < (humanplayers2+botplayers2))
			{
				G_RemoveRandomBot( FACTION_REBEL );
			}
			else
			{
				G_RemoveRandomBot( FACTION_EMPIRE );
			}
		}
	}
	else if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) {
		if (minplayers >= sv_maxclients.integer) {
			minplayers = sv_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( -1 );
		botplayers = G_CountBotPlayers( -1 );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( FACTION_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot( FACTION_SPECTATOR )) {
				// just remove the bot that is playing
				G_RemoveRandomBot( -1 );
			}
		}
	}
	else if (level.gametype == GT_FFA) {
		if (minplayers >= sv_maxclients.integer) {
			minplayers = sv_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( FACTION_FREE );
		botplayers = G_CountBotPlayers( FACTION_FREE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( FACTION_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( FACTION_FREE );
		}
	}
	else if (level.gametype == GT_HOLOCRON || level.gametype == GT_JEDIMASTER) {
		if (minplayers >= sv_maxclients.integer) {
			minplayers = sv_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( FACTION_FREE );
		botplayers = G_CountBotPlayers( FACTION_FREE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( FACTION_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( FACTION_FREE );
		}
	}
	*/
}

vmCvar_t npc_followers;
vmCvar_t npc_pathing;

#ifdef __NPC_MINPLAYERS__

vmCvar_t npc_wptonav;
vmCvar_t npc_imperials;
vmCvar_t npc_rebels;
vmCvar_t npc_mandalorians;
vmCvar_t npc_mercs;
vmCvar_t npc_pirates;
vmCvar_t npc_wildlife;
vmCvar_t npc_civilians;
vmCvar_t npc_vendors;

extern int gWPNum;
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

extern void SP_NPC_spawner( gentity_t *self);

int checkminimumnpcs_time = 0;

// Recorded in g_mover.c
extern vec3_t		MOVER_LIST[1024];
extern vec3_t		MOVER_LIST_TOP[1024];
extern int			MOVER_LIST_NUM;

qboolean Warzone_PointNearMoverEntityLocation( vec3_t org )
{// Never spawn near a mover location...
	int i = 0;
	qboolean found = qfalse;

	for (i = 0; i < MOVER_LIST_NUM; i++)
	{
		if (found) continue;

		if (DistanceHorizontal(org, MOVER_LIST[i]) >= 128.0) continue;

		found = qtrue;
	}

	return found;
}

qboolean Warzone_SpawnpointNearMoverEntityLocation( vec3_t org )
{// Never spawn near a mover location...
	int i = 0;
	qboolean found = qfalse;

	for (i = 0; i < MOVER_LIST_NUM; i++)
	{
		if (found) continue;

		if (DistanceHorizontal(org, MOVER_LIST[i]) >= 256.0) continue;

		found = qtrue;
	}

	return found;
}

qboolean Warzone_CheckBelowWaypoint( int wp )
{
	trace_t tr;
	vec3_t org, org2;

	VectorCopy(gWPArray[wp]->origin, org);
	VectorCopy(gWPArray[wp]->origin, org2);
	org2[2] = -65536.0f;//org[2] - 256;

	trap->Trace( &tr, org, NULL, NULL, org2, -1, MASK_PLAYERSOLID|CONTENTS_TRIGGER, 0, 0, 0);
	
	if ( tr.startsolid )
	{
		//trap->Print("Waypoint %i is in solid.\n", wp);
		return qfalse;
	}

	if ( tr.allsolid )
	{
		//trap->Print("Waypoint %i is in solid.\n", wp);
		return qfalse;
	}

	if ( tr.fraction == 1 )
	{
		//trap->Print("Waypoint %i is too high above ground.\n", wp);
		return qfalse;
	}

	if ( tr.contents & CONTENTS_LAVA )
	{
		//trap->Print("Waypoint %i is in lava.\n", wp);
		return qfalse;
	}
	
	if ( tr.contents & CONTENTS_SLIME )
	{
		//trap->Print("Waypoint %i is in slime.\n", wp);
		return qfalse;
	}

	if ( tr.contents & CONTENTS_TRIGGER )
	{
		//trap->Print("Waypoint %i is in trigger.\n", wp);
		return qfalse;
	}

	return qtrue;
}

extern gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles, team_t team );
extern int DOM_GetBestWaypoint(vec3_t org, int ignore, int badwp);
extern int DOM_FindIdealPathtoWP(bot_state_t *bs, int from, int to, int badwp2, int *pathlist);
qboolean Warzone_CheckRoutingFrom( int wp )
{
	gentity_t *spot = NULL;

	if (gWPArray[wp]->wpIsBadChecked)
	{
		if (gWPArray[wp]->wpIsBad)
			return qfalse; // This spot has already been checked and is bad...
		else
			return qtrue; // This spot has already been checked and is good...
	}

	// Check for routing to a spawnpoint from a (spawn) waypoint...
	spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");

	if (!spot) trap->Print("WAYPOINT REACHABILITY CHECK: Failed to find a spawnpoint!\n");

	if (spot->wpCurrent <= 0) // Should only need to do this part once...
	{
		spot->wpCurrent = DOM_GetBestWaypoint(spot->s.origin, -1, -1);

		if (!spot->wpCurrent) trap->Print("WAYPOINT REACHABILITY CHECK: Failed to find a waypoint for spawnpoint!\n");
	}

	spot->longTermGoal = wp;

	memset(spot->pathlist, -1, MAX_WPARRAY_SIZE);
	
	spot->pathsize = ASTAR_FindPathFast(spot->wpCurrent, spot->longTermGoal, spot->pathlist, qfalse);

	gWPArray[wp]->wpIsBadChecked = true;

	if (spot->pathsize > 0)
	{
		//trap->Print("Routing was %i. Spot is at %f %f %f.\n", spot->pathsize, spot->s.origin[0], spot->s.origin[1], spot->s.origin[2]);
		gWPArray[wp]->wpIsBad = false;
		return qtrue; // Found a route... This waypoint looks good to spawn NPCs at!
	}

	gWPArray[wp]->wpIsBad = true;
	return qfalse;
}

extern int G_SelectVillageSpawnpoint(void);
extern int G_SelectWildernessSpawnpoint(void);

void G_CheckVendorNPCs( void )
{
#if 0
	//
	// We always should have some vendors on maps...
	//
	int		botplayers = 0;
	int		minplayers = 0;
	int		i = 0;

	trap->Cvar_Update(&npc_vendors);
	minplayers = npc_vendors.integer;

	if (minplayers <= 0) minplayers = 2; // We should always have at least 2 vendors on any map...

	if (minplayers > 8)
	{
		minplayers = 8;
	}

	for (i = level.maxclients; i < MAX_GENTITIES; i++)
	{
		gentity_t *npc = &g_entities[i];

		if (!npc) continue;
		if (npc->s.eType != ET_NPC) continue;

		if (NPC_IsVendor(npc))
			botplayers++;
	}
#ifdef __ALWAYS_TWO_TRAVELLINGVENDORS
	if (botplayers < minplayers)
	{
		gentity_t	*npc = NULL;
		int			waypoint = G_SelectVillageSpawnpoint();
		int			random = irand(0,36);
		int			tries = 0;

		while (gWPArray[waypoint]->inuse == false || gWPArray[waypoint]->wpIsBad == true || Warzone_SpawnpointNearMoverEntityLocation(gWPArray[waypoint]->origin)
#ifndef __WAYPOINTS_PRECHECKED__
			|| !Warzone_CheckBelowWaypoint(waypoint) || !Warzone_CheckRoutingFrom( waypoint )
#endif //__WAYPOINTS_PRECHECKED__
			)
		{
			gWPArray[waypoint]->inuse = false; // set it bad!

			if (tries > 10)
			{
				return; // Try again on next check...
			}

			// Find a new one... This is probably a bad waypoint...
			waypoint = G_SelectVillageSpawnpoint();
			tries++;
		}

		npc = G_Spawn();

		// UQ1: Always spawn travelling vendor NPCs...
		npc->NPC_type = "vendor_travelling";
		//npc->s.NPC_class = CLASS_TRAVELLING_VENDOR;

		VectorCopy(gWPArray[waypoint]->origin, npc->s.origin);
		npc->s.origin[2]+=32; // Drop down...

		npc->s.angles[PITCH] = 0;
		npc->s.angles[YAW] = irand(0,359);
		npc->s.angles[ROLL] = 0;

#ifdef __NPC_SPAWN_DEBUGGING__
		trap->Print(va("[%i/%i] Spawning (travelling vendor NPC) %s at waypoint %i.\n", botplayers+1, minplayers, npc->NPC_type, waypoint));
#endif //__NPC_SPAWN_DEBUGGING__

		npc->s.eFlags |= EF_RADAROBJECT;
		SP_NPC_spawner( npc );
	}
#endif // __ALWAYS_TWO_TRAVELLINGVENDORS
#endif //0
}

extern qboolean			WATER_ENABLED;
extern float			MAP_WATER_LEVEL;

extern qboolean			TOWN_FORCEFIELD_ENABLED;
extern vec3_t			TOWN_FORCEFIELD_ORIGIN;
extern vec3_t			TOWN_FORCEFIELD_RADIUS_3D;
extern float			TOWN_FORCEFIELD_RADIUS;

void G_CheckCivilianNPCs( void )
{
	int		botplayers = 0;
	int		minplayers = 0;
	int		i = 0;

	if (!TOWN_FORCEFIELD_ENABLED)
	{// No town to put civilians in...
		return;
	}

	trap->Cvar_Update(&npc_civilians);
	minplayers = npc_civilians.integer;

	if (minplayers <= 0) return;

	if (minplayers > 512)
	{
		minplayers = 512;
	}

	for (i = level.maxclients; i < MAX_GENTITIES; i++)
	{
		gentity_t *npc = &g_entities[i];

		if (!npc) continue;
		if (npc->s.eType != ET_NPC) continue;
		if (!(npc->client->NPC_class == CLASS_CIVILIAN
			|| npc->client->NPC_class == CLASS_CIVILIAN_R2D2
			|| npc->client->NPC_class == CLASS_CIVILIAN_R5D2
			|| npc->client->NPC_class == CLASS_CIVILIAN_PROTOCOL
			|| npc->client->NPC_class == CLASS_CIVILIAN_WEEQUAY)) continue;

		botplayers++;
	}

	if (botplayers < minplayers)
	{
		gentity_t	*npc = NULL;
		vec3_t		spawnOrg;
		int			random = irand(0,36);
		int			tries = 0;


#ifdef __USE_NAVLIB__
		if (G_NavmeshIsLoaded())
		{// Find a navmesh spawnpoint in the town...
			extern void G_FindSky(vec3_t org);
			extern void G_FindGround(vec3_t org);

			VectorSet(spawnOrg, 0.0, 0.0, 0.0);

			vec3_t point, offDir;
			offDir[0] = random() * 2.0 - 1.0;
			offDir[1] = random() * 2.0 - 1.0;
			offDir[2] = 0.0;
			VectorNormalize(offDir);
			VectorCopy(TOWN_FORCEFIELD_ORIGIN, point);
			VectorMA(point, (TOWN_FORCEFIELD_RADIUS * 0.5) - 1024.0, offDir, point);
			bool found = FindRandomNavmeshPointInRadius(-1, point, spawnOrg, 512.0);

			G_FindSky(spawnOrg);
			if (VectorLength(spawnOrg) == 0) found = false;

			if (found)
			{
				G_FindGround(spawnOrg);
				if (VectorLength(spawnOrg) == 0) found = false;
			}

			while ((!found || spawnOrg[2] <= MAP_WATER_LEVEL) && tries < 3)
			{
				offDir[0] = random() * 2.0 - 1.0;
				offDir[1] = random() * 2.0 - 1.0;
				offDir[2] = 0.0;
				VectorNormalize(offDir);
				VectorCopy(TOWN_FORCEFIELD_ORIGIN, point);
				VectorMA(point, (TOWN_FORCEFIELD_RADIUS * 0.5) - 1024.0, offDir, point);
				found = FindRandomNavmeshPointInRadius(-1, point, spawnOrg, 512.0);

				G_FindSky(spawnOrg);
				if (VectorLength(spawnOrg) == 0) found = false;

				if (found)
				{
					G_FindGround(spawnOrg);
					if (VectorLength(spawnOrg) == 0) found = false;
				}

				tries++;
			}

			if (!found)
			{// Try again next check...
				//Com_Printf("Town spawnpoint failed.\n");
				return;
			}

			//Com_Printf("Town spawnpoint found at %f %f %f.\n", spawnOrg[0], spawnOrg[1], spawnOrg[2]);
		}
		else
#endif //__USE_NAVLIB__
		{
			int			waypoint = G_SelectVillageSpawnpoint();

			while (gWPArray[waypoint]->inuse == false || gWPArray[waypoint]->wpIsBad == true || Warzone_SpawnpointNearMoverEntityLocation(gWPArray[waypoint]->origin)
#ifndef __WAYPOINTS_PRECHECKED__
				|| !Warzone_CheckBelowWaypoint(waypoint) || !Warzone_CheckRoutingFrom(waypoint)
#endif //__WAYPOINTS_PRECHECKED__
				)
			{
				gWPArray[waypoint]->inuse = false; // set it bad!

				if (tries > 10)
				{
					return; // Try again on next check...
				}

				// Find a new one... This is probably a bad waypoint...
				waypoint = G_SelectVillageSpawnpoint();

				tries++;
			}

			VectorCopy(gWPArray[waypoint]->origin, spawnOrg);
		}

		npc = G_Spawn();

		switch (random)
		{
		case 0:
			npc->NPC_type = "civilian_bartender";
			break;
		case 1:
			npc->NPC_type = "civilian_bartender";
			break;
		case 2:
			npc->NPC_type = "civilian_bartender";
			break;
		case 3:
			npc->NPC_type = "civilian_bartender";
			break;
		case 4:
			npc->NPC_type = "civilian_human_merc";
			break;
		case 5:
			npc->NPC_type = "civilian_human_merc";
			break;
		case 6:
			npc->NPC_type = "civilian_human_merc2";
			break;
		case 7:
			npc->NPC_type = "civilian_human_merc2";
			break;
		case 8:
			npc->NPC_type = "civilian_merchant";
			break;
		case 9:
			npc->NPC_type = "civilian_merchant";
			break;
		case 10:
			npc->NPC_type = "civilian_merchant";
			break;
		case 11:
			npc->NPC_type = "civilian_merchant";
			break;
		case 12:
			npc->NPC_type = "civilian_protocol";
			break;
		case 13:
			//npc->NPC_type = "civilian_protocol";
			//break;
		case 14:
			//npc->NPC_type = "civilian_protocol";
			//break;
		case 15:
			npc->NPC_type = "civilian_r2d2";
			break;
		case 16:
			npc->NPC_type = "civilian_r2d2";
			break;
		case 17:
			//npc->NPC_type = "civilian_r2d2";
			//break;
		case 18:
			npc->NPC_type = "civilian_rodian";
			break;
		case 19:
			npc->NPC_type = "civilian_rodian";
			break;
		case 20:
			npc->NPC_type = "civilian_rodian2";
			break;
		case 21:
			npc->NPC_type = "civilian_rodian2";
			break;
		case 22:
			npc->NPC_type = "civilian_trandoshan";
			break;
		case 23:
			npc->NPC_type = "civilian_trandoshan";
			break;
		case 24:
			npc->NPC_type = "civilian_trandoshan";
			break;
		case 25:
			npc->NPC_type = "civilian_trandoshan";
			break;
		case 26:
			npc->NPC_type = "civilian_ugnaught";
			break;
		case 27:
			npc->NPC_type = "civilian_ugnaught";
			break;
		case 28:
			npc->NPC_type = "civilian_ugnaught2";
			break;
		case 29:
			npc->NPC_type = "civilian_ugnaught2";
			break;
		case 30:
			npc->NPC_type = "civilian_weequay";
			break;
		case 31:
			npc->NPC_type = "civilian_weequay2";
			break;
		case 32:
			npc->NPC_type = "civilian_weequay3";
			break;
		case 33:
			npc->NPC_type = "civilian_weequay4";
			break;
		case 34:
			npc->NPC_type = "civilian_r5d2";
			break;
		case 35:
			npc->NPC_type = "civilian_r5d2";
			break;
		default:
			npc->NPC_type = "civilian_r5d2";
			break;
		}

		VectorCopy(spawnOrg, npc->s.origin);
		npc->s.origin[2]+=32; // Drop down...

		npc->s.angles[PITCH] = 0;
		npc->s.angles[YAW] = irand(0,359);
		npc->s.angles[ROLL] = 0;

#ifdef __NPC_SPAWN_DEBUGGING__
		trap->Print(va("[%i/%i] Spawning (civilian NPC) %s at waypoint %i.\n", botplayers+1, minplayers, npc->NPC_type, waypoint));
#endif //__NPC_SPAWN_DEBUGGING__

		SP_NPC_spawner( npc );
	}
}

/*
===============
G_CheckMinimumNpcs
===============
*/

extern qboolean			EVENTS_ENABLED;
extern vec3_t			NPC_SPAWNPOINT;
extern int				NPC_SPAWNFLAG;

extern qboolean WAYPOINT_PARTOL_BAD_LIST[MAX_WPARRAY_SIZE];

extern void NPC_Patrol_MakeBadList(void);
extern void NPC_PrintNumActiveNPCs(void);

qboolean	SPAWN_FACTIONS_CHECKED = qfalse;
qboolean	HAVE_MANDALORIANS = qfalse;
qboolean	HAVE_MERCS = qfalse;
qboolean	HAVE_PIRATES = qfalse;
qboolean	HAVE_WILDLIFE = qfalse;

int next_npc_max_warning_time = 0;
int next_npc_stats_time = 0;

// Sharing this info with the eventsystem spawner system, so we can skip events that we don't have enough slots to fill...
int			num_imperial_npcs = 0, num_rebel_npcs = 0, num_mandalorian_npcs = 0, num_merc_npcs = 0, num_pirate_npcs = 0, num_wildlife_npcs = 0;

void G_CheckWildlifeSpawns(void)
{
	//
	// Spawn the WILDLIFE team...
	//
	if (HAVE_WILDLIFE)
	{
		vmCvar_t	mapname;
		trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

		if (num_wildlife_npcs + 4 < npc_wildlife.integer)
		{
			spawnGroup_t group;
			int			random = irand(0, 22);
			int			tries = 0;
			vec3_t		position;
			int			SPAWN_TEAM = FACTION_WILDLIFE;

#ifdef __ALL_JEDI_NPCS__
			group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_BOSS);
			if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_BOSS);
#else //!__ALL_JEDI_NPCS__
			if (random >= 16)
			{
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_SPAM);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_SPAM);
			}
			else if (random >= 11)
			{
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_COMMON);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_COMMON);
			}
			else if (random >= 7)
			{
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_OFFICER);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_OFFICER);
			}
			else if (random >= 4)
			{// Officers/Specials...
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_ELITE);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_ELITE);
			}
			else if (random >= 1)
			{// Jedi...
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_BOSS);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_BOSS);
			}
			else
			{
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_SPAM);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_SPAM);
			}
#endif //__ALL_JEDI_NPCS__

			// Wildlife always spawn at random waypoints (for now)...
			FindRandomNavmeshSpawnpoint(NULL, position);
			position[2] += 32; // Drop down...

			if (VectorLength(position) != 32)
			{
				SP_NPC_Spawner_Group(group, position, SPAWN_TEAM, -1);
			}
		}
	}
}
void G_CheckMinimumNpcs(void) 
{
	vmCvar_t	mapname;
	int			min_imperials, min_rebels, min_mandalorians, min_mercs, min_pirates, min_wildlife;
	int			i;
	static int	checkminimumplayers_time;
	spawnGroup_t group;

	if (g_gametype.integer == GT_INSTANCE)
	{
		return;
	}

#ifndef __USE_NAVLIB_SPAWNPOINTS__
	if (gWPNum <= 0)
	{
		return;
	}
#else
	if (!G_NavmeshIsLoaded())
	{
		return;
	}
#endif //__USE_NAVLIB_SPAWNPOINTS__

	if (g_gametype.integer == GT_SIEGE)
	{
		return;
	}

	if (level.intermissiontime) return;

	// only check once each 1 seconds - NPCs can spawn faster then bots... If i use a 1ms extra on these, it should offset their thinks
	// to spread them out and reduce the occurances of npcs all thinking on the same frames all the time...
	//if (level.numConnectedClients > 0 && checkminimumnpcs_time > level.time - 1001) 
	if (/*level.numConnectedClients > 0 &&*/ checkminimumnpcs_time > level.time - 101)
	{
		return;
	}

	/*if (level.numConnectedClients <= 0 && checkminimumnpcs_time > level.time - 51) 
	{
		return;
	}*/

	num_imperial_npcs = 0;
	num_rebel_npcs = 0;
	num_mandalorian_npcs = 0;
	num_merc_npcs = 0;
	num_pirate_npcs = 0;
	num_wildlife_npcs = 0;

#ifndef __USE_NAVLIB_SPAWNPOINTS__
	NPC_Patrol_MakeBadList();
#endif //__USE_NAVLIB_SPAWNPOINTS__

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

	checkminimumnpcs_time = level.time;
	trap->Cvar_Update(&npc_imperials);
	trap->Cvar_Update(&npc_rebels);
	trap->Cvar_Update(&npc_mandalorians);
	trap->Cvar_Update(&npc_mercs);
	trap->Cvar_Update(&npc_pirates);
	trap->Cvar_Update(&npc_wildlife);
	trap->Cvar_Update(&npc_pathing);
	trap->Cvar_Update(&npc_followers);
	min_imperials = npc_imperials.integer;
	min_rebels = npc_rebels.integer;
	min_mandalorians = npc_mandalorians.integer;
	min_mercs = npc_mercs.integer;
	min_pirates = npc_pirates.integer;
	min_wildlife = npc_wildlife.integer;

	// Add vendors...
	G_CheckVendorNPCs();

	// Add civilians...
	G_CheckCivilianNPCs();

	if (min_imperials <= 0 && min_rebels <= 0 && min_mandalorians <= 0 && min_mercs <= 0 && min_pirates <= 0 && min_wildlife <= 0) return;

	if (min_imperials + min_rebels + min_mandalorians + min_mercs + min_pirates + min_wildlife > ENTITYNUM_MAX_NORMAL - (MAX_CLIENTS * 4))
	{
		min_imperials = 0;
		min_rebels = 0;
		min_mandalorians = 0;
		min_mercs = 0;
		min_pirates = 0;
		min_wildlife = 0;

		if (next_npc_max_warning_time <= level.time)
		{
			trap->Print("Too many NPCs to spawn! Check the value of cvars - min_imperials, min_rebels, min_mandalorians, min_mercs, min_pirates, min_wildlife!\n");
			next_npc_max_warning_time = level.time + 10000;
		}
		return;
	}

	for (i = level.maxclients; i < MAX_GENTITIES; i++)
	{
		gentity_t *npc = &g_entities[i];

		if (!(npc
			&& npc->inuse
			&& npc->r.linked
			&& npc->s.eType == ET_NPC
			/*&& NPC_IsAlive(npc, npc)*/)) // we still want to wait for the bodies to disappear before the next wave...
		{
			continue;
		}

		if (NPC_IsCivilian(npc)) continue;
		if (NPC_IsVendor(npc)) continue;

		if (npc->client->sess.sessionTeam == FACTION_EMPIRE)
			num_imperial_npcs++;
		else if (npc->client->sess.sessionTeam == FACTION_REBEL)
			num_rebel_npcs++;
		else if (npc->client->sess.sessionTeam == FACTION_MANDALORIAN)
			num_mandalorian_npcs++;
		else if (npc->client->sess.sessionTeam == FACTION_MERC)
			num_merc_npcs++;
		else if (npc->client->sess.sessionTeam == FACTION_PIRATES)
			num_pirate_npcs++;
		else if (npc->client->sess.sessionTeam == FACTION_WILDLIFE)
			num_wildlife_npcs++;
	}

	// First handle any wildlife spawning... since these guys don't use events...
	G_CheckWildlifeSpawns();

	if (((g_consoleNPCStats.integer && dedicated.integer) || g_consoleNPCStats.integer == 2)
		&& next_npc_stats_time <= level.time)
	{
		// g_consoleNPCStats 0 disables npc stats display in console.
		// g_consoleNPCStats 1 shows them only on dedicated console.
		// g_consoleNPCStats 2 shows them for local client as well as dedicated consoles.
		trap->Print("^5Cvars are ^7%i^5 imperials, ^7%i^5 rebels, ^7%i^5 mandalorians, ^7%i^5 mercs, ^7%i^5 pirates, and ^7%i^5 wildlife NPCs to spawn.\n", min_imperials, min_rebels, min_mandalorians, min_mercs, min_pirates, min_wildlife);
		trap->Print("^5There are ^7%i^5 imperials, ^7%i^5 rebels, ^7%i^5 mandalorians, ^7%i^5 mercs, ^7%i^5 pirates, and ^7%i^5 wildlife NPCs spawned.\n", num_imperial_npcs, num_rebel_npcs, num_mandalorian_npcs, num_merc_npcs, num_pirate_npcs, num_wildlife_npcs);
		NPC_PrintNumActiveNPCs();
		G_PrintEventAreaInfo();
		next_npc_stats_time = level.time + 10000;
	}

	// Find the event that needs new spawns the most...
	int						EVENT_COUNT = G_GetEventsCount();
	int						EVENT_AREA_ID = -1;
	team_t					EVENT_SPAWN_FACTION = FACTION_WILDLIFE;
	spawnGroupRarity_t		EVENT_SPAWN_QUALITY = RARITY_SPAM;
	int						EVENT_SPAWN_SPAWNCOUNT = 0;
	int						EVENT_SPAWN_WAVECOUNT = 4;
	int						EVENT_MAX_SPAWNS = 0;
	qboolean				skipSpawning = qfalse;

	//Com_Printf("EVENT_COUNT %i.\n", EVENT_COUNT);

	if (EVENT_COUNT <= 0)
	{// Wait until event areas have been all set up...
		return;
	}

	if (!SPAWN_FACTIONS_CHECKED)
	{
		group = GetSpawnGroup(va("%s_mandalorians", mapname.string), RARITY_SPAM);
		if (group.npcCount) HAVE_MANDALORIANS = qtrue;

		group = GetSpawnGroup(va("%s_mercenaries", mapname.string), RARITY_SPAM);
		if (group.npcCount) HAVE_MERCS = qtrue;

		group = GetSpawnGroup(va("%s_pirates", mapname.string), RARITY_SPAM);
		if (group.npcCount) HAVE_PIRATES = qtrue;

		group = GetSpawnGroup(va("%s_wildlife", mapname.string), RARITY_SPAM);
		if (group.npcCount) HAVE_WILDLIFE = qtrue;

		SPAWN_FACTIONS_CHECKED = qtrue;
	}

	qboolean isEventSpawn = qfalse;
	qboolean spawnEmpire = qfalse;
	qboolean spawnRebel = qfalse;
	qboolean spawnMandalorian = qfalse;
	qboolean spawnMerc = qfalse;
	qboolean spawnPirate = qfalse;
	qboolean spawnWildlife = qtrue; // Is always allowed...

	// Update event area spawn waves...
	G_UpdateSpawnAreaWaves();

	// Update event area spawn counts before spawning anything new...
	G_CountEventAreaSpawns();

	EVENT_AREA_ID = G_GetEventMostNeedingSpawns();

	if (EVENT_AREA_ID >= 0)
	{
		EVENT_SPAWN_FACTION = G_GetFactionForEvent(EVENT_AREA_ID);
		EVENT_SPAWN_QUALITY = G_GetRarityForEventWave(EVENT_AREA_ID);
		EVENT_SPAWN_SPAWNCOUNT = G_SpawnCountForEvent(EVENT_AREA_ID);
		EVENT_SPAWN_WAVECOUNT = G_WaveSpawnCountForEventWave(EVENT_AREA_ID);
		EVENT_MAX_SPAWNS = G_MaxSpawnsInEvent(EVENT_AREA_ID);

		//trap->Print("G_GetEventMostNeedingSpawns: %i. Faction: %s. Count: %i / %i (%i).\n", EVENT_AREA_ID, TeamName(EVENT_SPAWN_FACTION), EVENT_SPAWN_SPAWNCOUNT, EVENT_SPAWN_WAVECOUNT, EVENT_MAX_SPAWNS);

		if (EVENT_SPAWN_SPAWNCOUNT + 4 > EVENT_SPAWN_WAVECOUNT)
		{// Event is already full...
			return;
		}
		else
		{
			//Com_Printf("EVENT_SPAWN_SPAWNCOUNT %i. EVENT_SPAWN_WAVECOUNT %i.\n", EVENT_SPAWN_SPAWNCOUNT, EVENT_SPAWN_WAVECOUNT);

			switch (EVENT_SPAWN_FACTION)
			{
			case FACTION_EMPIRE:
				spawnEmpire = qtrue;
				isEventSpawn = qtrue;
				break;
			case FACTION_REBEL:
				spawnRebel = qtrue;
				isEventSpawn = qtrue;
				break;
			case FACTION_MANDALORIAN:
				spawnMandalorian = qtrue;
				isEventSpawn = qtrue;
				break;
			case FACTION_MERC:
				spawnMerc = qtrue;
				isEventSpawn = qtrue;
				break;
			case FACTION_PIRATES:
				spawnPirate = qtrue;
				isEventSpawn = qtrue;
				break;
			case FACTION_WILDLIFE:
			case FACTION_FREE:
			case FACTION_SPECTATOR:
				return;
				break;
			}
		}

		//
		// Spawn the IMPERIAL team...
		//
		if (spawnEmpire)
		{
			vec3_t		position;
			int			SPAWN_TEAM = FACTION_EMPIRE;

#ifdef __ALL_JEDI_NPCS__
			group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_BOSS);
			if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_BOSS);
#else //!__ALL_JEDI_NPCS__
			group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
			if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
#endif //__ALL_JEDI_NPCS__

			if (npc_pathing.integer && g_gametype.integer >= GT_TEAM)
			{// New War Zone Instances (not JKG style)... Get CTF spawnpoints...
				gentity_t spawnPoint;
				spawnPoint = *SelectCTFSpawnPoint((team_t)SPAWN_TEAM, 0, position, position, qtrue);
				VectorCopy(spawnPoint.s.origin, position);
				position[2] += 32; // Drop down...
			}
			else
			{// Imps...
				FindRandomEventSpawnpoint(EVENT_AREA_ID, position);
				//trap->Print("imp spawnpoint %f %f %f. num_imperial_npcs %i. min_imperials %i. EVENT_SPAWN_SPAWNCOUNT %i. EVENT_SPAWN_WAVECOUNT %i.\n", position[0], position[1], position[2], num_imperial_npcs, min_imperials, EVENT_SPAWN_SPAWNCOUNT, EVENT_SPAWN_WAVECOUNT);
				position[2] += 32; // Drop down...
			}

			if (VectorLength(position) != 32)
			{
				SP_NPC_Spawner_Group(group, position, SPAWN_TEAM, EVENT_AREA_ID);
			}
		}

		//
		// Spawn the REBEL team...
		//
		if (spawnRebel)
		{
			vec3_t		position;
			int			SPAWN_TEAM = FACTION_REBEL;

#ifdef __ALL_JEDI_NPCS__
			group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_BOSS);
			if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_BOSS);
#else //!__ALL_JEDI_NPCS__
			group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
			if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
#endif //__ALL_JEDI_NPCS__

			if (npc_pathing.integer && g_gametype.integer >= GT_TEAM)
			{// New War Zone Instances (not JKG style)... Get CTF spawnpoints...
				gentity_t spawnPoint;
				spawnPoint = *SelectCTFSpawnPoint((team_t)SPAWN_TEAM, 0, position, position, qtrue);
				VectorCopy(spawnPoint.s.origin, position);
				position[2] += 32; // Drop down...
			}
			else
			{// Rebels...
				FindRandomEventSpawnpoint(EVENT_AREA_ID, position);
				//trap->Print("reb spawnpoint %f %f %f\n", position[0], position[1], position[2]);
				position[2] += 32; // Drop down...
			}

			if (VectorLength(position) != 32)
			{
				SP_NPC_Spawner_Group(group, position, SPAWN_TEAM, EVENT_AREA_ID);
			}
		}

		//
		// Spawn the MANDALORIAN team...
		//
		if (HAVE_MANDALORIANS)
		{
			if (spawnMandalorian)
			{
				vec3_t		position;
				int			SPAWN_TEAM = FACTION_MANDALORIAN;

#ifdef __ALL_JEDI_NPCS__
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_BOSS);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_BOSS);
#else //!__ALL_JEDI_NPCS__
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
#endif //__ALL_JEDI_NPCS__

				FindRandomEventSpawnpoint(EVENT_AREA_ID, position);
				position[2] += 32; // Drop down...

				if (VectorLength(position) != 32)
				{
					SP_NPC_Spawner_Group(group, position, SPAWN_TEAM, EVENT_AREA_ID);
				}
			}
		}

		//
		// Spawn the MERC team...
		//
		if (HAVE_MERCS)
		{
			if (spawnMerc)
			{
				vec3_t		position;
				int			SPAWN_TEAM = FACTION_MERC;

#ifdef __ALL_JEDI_NPCS__
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_BOSS);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_BOSS);
#else //!__ALL_JEDI_NPCS__
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
#endif //__ALL_JEDI_NPCS__

				FindRandomEventSpawnpoint(EVENT_AREA_ID, position);
				position[2] += 32; // Drop down...

				if (VectorLength(position) != 32)
				{
					SP_NPC_Spawner_Group(group, position, SPAWN_TEAM, EVENT_AREA_ID);
				}
			}
		}

		//
		// Spawn the PIRATES team...
		//
		if (HAVE_PIRATES)
		{
			if (spawnPirate)
			{
				vec3_t		position;
				int			SPAWN_TEAM = FACTION_PIRATES;

#ifdef __ALL_JEDI_NPCS__
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), RARITY_BOSS);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), RARITY_BOSS);
#else //!__ALL_JEDI_NPCS__
				group = GetSpawnGroup(va("%s_%s", mapname.string, factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
				if (!group.npcCount) group = GetSpawnGroup(va("default_%s", factionNames[SPAWN_TEAM]), EVENT_SPAWN_QUALITY);
#endif //__ALL_JEDI_NPCS__

				FindRandomEventSpawnpoint(EVENT_AREA_ID, position);
				position[2] += 32; // Drop down...

				if (VectorLength(position) != 32)
				{
					SP_NPC_Spawner_Group(group, position, SPAWN_TEAM, EVENT_AREA_ID);
				}
			}
		}
	}
}
#endif //__NPC_MINPLAYERS__

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void ) {
	int		n;

#ifdef __NPC_MINPLAYERS__
	G_CheckMinimumNpcs();
#endif //__NPC_MINPLAYERS__

	G_CheckMinimumPlayers();

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum, qfalse );
		botSpawnQueue[n].spawnTime = 0;

		/*
		if( level.gametype == GT_SINGLE_PLAYER || level.gametype == GT_INSTANCE || level.gametype == GT_WARZONE ) {
			trap->GetUserinfo( botSpawnQueue[n].clientNum, userinfo, sizeof(userinfo) );
			PlayerIntroSound( Info_ValueForKey (userinfo, "model") );
		}
		*/
	}
}

/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	trap->Print( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum, qfalse );
}

/*
===============
G_RemoveQueuedBotBegin

Called on client disconnect to make sure the delayed spawn
doesn't happen on a freed index
===============
*/
void G_RemoveQueuedBotBegin( int clientNum ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( botSpawnQueue[n].clientNum == clientNum ) {
			botSpawnQueue[n].spawnTime = 0;
			return;
		}
	}
}

/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t	settings;
	char			userinfo[MAX_INFO_STRING];

	trap->GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	Q_strncpyz( settings.personalityfile, Info_ValueForKey( userinfo, "personality" ), sizeof(settings.personalityfile) );
	settings.skill = atof( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof(settings.team) );

	if (!BotAISetupClient( clientNum, &settings, restart )) {
		trap->DropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	return qtrue;
}

/*
===============
G_AddBot
===============
*/
static void G_AddBot( const char *name, float skill, const char *team, int delay, char *altname) {
	gentity_t		*bot = NULL;
	int				clientNum, preTeam = FACTION_FREE;
	char			userinfo[MAX_INFO_STRING] = {0},
					*botinfo = NULL, *key = NULL, *s = NULL, *botname = NULL, *model = NULL;

	// have the server allocate a client slot
	clientNum = trap->BotAllocateClient();
	if ( clientNum == -1 ) {
//		trap->Print( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
//		trap->Print( S_COLOR_RED "Start server with more 'open' slots.\n" );
		trap->SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "UNABLE_TO_ADD_BOT")));
		return;
	}

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName( name );
	if ( !botinfo ) {
		trap->BotFreeClient( clientNum );
		trap->Print( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if( !botname[0] )
		botname = Info_ValueForKey( botinfo, "name" );
	// check for an alternative name
	if ( altname && altname[0] )
		botname = altname;

	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "ip", "localhost" );
	Info_SetValueForKey( userinfo, "skill", va("%.2f", skill) );

		 if ( skill >= 1 && skill < 2 )		Info_SetValueForKey( userinfo, "handicap", "50" );
	else if ( skill >= 2 && skill < 3 )		Info_SetValueForKey( userinfo, "handicap", "70" );
	else if ( skill >= 3 && skill < 4 )		Info_SetValueForKey( userinfo, "handicap", "90" );
	else									Info_SetValueForKey( userinfo, "handicap", "100" );

	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model )	model = DEFAULT_MODEL"/default";
	Info_SetValueForKey( userinfo, key, model );

	key = "sex";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = Info_ValueForKey( botinfo, "gender" );
	if ( !*s )	s = "male";
	Info_SetValueForKey( userinfo, key, s );

	key = "color1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "4";
	Info_SetValueForKey( userinfo, key, s );

	key = "color2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "4";
	Info_SetValueForKey( userinfo, key, s );

	key = "saber1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = DEFAULT_SABER;
	Info_SetValueForKey( userinfo, key, s );

	key = "saber2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "none";
	Info_SetValueForKey( userinfo, key, s );

	key = "forcepowers";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = DEFAULT_FORCEPOWERS;
	Info_SetValueForKey( userinfo, key, s );

	key = "cg_predictItems";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "1";
	Info_SetValueForKey( userinfo, key, s );

	key = "char_color_red";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "255";
	Info_SetValueForKey( userinfo, key, s );

	key = "char_color_green";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "255";
	Info_SetValueForKey( userinfo, key, s );

	key = "char_color_blue";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "255";
	Info_SetValueForKey( userinfo, key, s );

	key = "teamtask";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "0";
	Info_SetValueForKey( userinfo, key, s );

	key = "personality";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s )	s = "botfiles/default.jkb";
	Info_SetValueForKey( userinfo, key, s );

	// initialize the bot settings
	if ( !team || !*team ) {
		if ( level.gametype >= GT_TEAM ) {
			if ( PickTeam( clientNum ) == FACTION_EMPIRE)
				team = "red";
			else
				team = "blue";
		}
		else
			team = "red";
	}
	Info_SetValueForKey( userinfo, "team", team );

	bot = &g_entities[ clientNum ];
//	bot->r.svFlags |= SVF_BOT;
//	bot->inuse = qtrue;

	// register the userinfo
	trap->SetUserinfo( clientNum, userinfo );

	if ( level.gametype >= GT_TEAM )
	{
		if ( team && !Q_stricmp( team, "red" ) )
			bot->client->sess.sessionTeam = FACTION_EMPIRE;
		else if ( team && !Q_stricmp( team, "blue" ) )
			bot->client->sess.sessionTeam = FACTION_REBEL;
		else
			bot->client->sess.sessionTeam = PickTeam( -1 );
	}

	if ( level.gametype == GT_SIEGE )
	{
		bot->client->sess.siegeDesiredTeam = bot->client->sess.sessionTeam;
		bot->client->sess.sessionTeam = FACTION_SPECTATOR;
	}

	preTeam = bot->client->sess.sessionTeam;

	bot->NPC = NULL;

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) )
		return;

	if ( bot->client->sess.sessionTeam != preTeam )
	{
		trap->GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

		if ( bot->client->sess.sessionTeam == FACTION_SPECTATOR )
			bot->client->sess.sessionTeam = (team_t)preTeam;

		if ( bot->client->sess.sessionTeam == FACTION_EMPIRE )
			team = "Red";
		else
		{
			if ( level.gametype == GT_SIEGE )
				team = (bot->client->sess.sessionTeam == FACTION_REBEL) ? "Blue" : "s";
			else
				team = "Blue";
		}

		Info_SetValueForKey( userinfo, "team", team );

		trap->SetUserinfo( clientNum, userinfo );

		bot->client->ps.persistant[ PERS_TEAM ] = bot->client->sess.sessionTeam;

		G_ReadSessionData( bot->client );
		if ( !ClientUserinfoChanged( clientNum ) )
			return;
	}

	if (level.gametype == GT_DUEL ||
		level.gametype == GT_POWERDUEL)
	{
		int loners = 0;
		int doubles = 0;

		bot->client->sess.duelTeam = 0;
		G_PowerDuelCount(&loners, &doubles, qtrue);

		if (!doubles || loners > (doubles/2))
		{
            bot->client->sess.duelTeam = DUELTEAM_DOUBLE;
		}
		else
		{
            bot->client->sess.duelTeam = DUELTEAM_LONE;
		}

		bot->client->sess.sessionTeam = FACTION_SPECTATOR;
		SetTeam(bot, "s");
	}
	else
	{
		if( delay == 0 ) {
			ClientBegin( clientNum, qfalse );
			return;
		}

		AddBotToSpawnQueue( clientNum, delay );
	}
}

/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	float			skill;
	int				delay;
	char			name[MAX_TOKEN_CHARS];
	char			altname[MAX_TOKEN_CHARS];
	char			string[MAX_TOKEN_CHARS];
	char			team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap->Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap->Print( "Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n" );
		return;
	}

	// skill
	trap->Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	}
	else {
		skill = atof( string );
	}

	// team
	trap->Argv( 3, team, sizeof( team ) );

	// delay
	trap->Argv( 4, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	}
	else {
		delay = atoi( string );
	}

	// alternative name
	trap->Argv( 5, altname, sizeof( altname ) );

	G_AddBot( name, skill, team, delay, altname );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		trap->Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap->SendServerCommand( -1, "loaddefered\n" );	// FIXME: spelled wrong, but not changing for demo
	}
}

/*
===============
Svcmd_BotList_f
===============
*/
void Svcmd_BotList_f( void ) {
	int i;
	char name[MAX_NETNAME];
	char funname[MAX_NETNAME];
	char model[MAX_QPATH];
	char personality[MAX_QPATH];

	trap->Print("name             model            personality              funname\n");
	for (i = 0; i < level.bots.num; i++) {
		Q_strncpyz(name, Info_ValueForKey( level.bots.infos[i], "name" ), sizeof( name ));
		if ( !*name ) {
			Q_strncpyz(name, "Padawan", sizeof( name ));
		}
		Q_strncpyz(funname, Info_ValueForKey( level.bots.infos[i], "funname"), sizeof( funname ));
		if ( !*funname ) {
			funname[0] = '\0';
		}
		Q_strncpyz(model, Info_ValueForKey( level.bots.infos[i], "model" ), sizeof( model ));
		if ( !*model ) {
			Q_strncpyz(model, DEFAULT_MODEL"/default", sizeof( model ));
		}
		Q_strncpyz(personality, Info_ValueForKey( level.bots.infos[i], "personality"), sizeof( personality ));
		if (!*personality ) {
			Q_strncpyz(personality, "botfiles/kyle.jkb", sizeof( personality ));
		}
		trap->Print("%-16s %-16s %-20s %-20s\n", name, model, COM_SkipPath(personality), funname);
	}
}

#if 0
/*
===============
G_SpawnBots
===============
*/
static void G_SpawnBots( char *botList, int baseDelay ) {
	char		*bot;
	char		*p;
	float		skill;
	int			delay;
	char		bots[MAX_INFO_VALUE];

	skill = trap->Cvar_VariableIntegerValue( "g_npcspskill" );
	if( skill < 1 ) {
		trap->Cvar_Set( "g_npcspskill", "1" );
		skill = 1;
	}
	else if ( skill > 5 ) {
		trap->Cvar_Set( "g_npcspskill", "5" );
		skill = 5;
	}

	Q_strncpyz( bots, botList, sizeof(bots) );
	p = &bots[0];
	delay = baseDelay;
	while( *p ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
		trap->SendConsoleCommand( EXEC_INSERT, va("addbot \"%s\" %f free %i\n", bot, skill, delay) );

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}
#endif

/*
===============
G_LoadBotsFromFile
===============
*/
static void G_LoadBotsFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( S_COLOR_RED "file not found: %s\n", filename );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap->Print( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_BOTS_TEXT );
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	level.bots.num += G_ParseInfos( buf, MAX_BOTS - level.bots.num, &level.bots.infos[level.bots.num] );
}

/*
===============
G_LoadBots
===============
*/
static void G_LoadBots( void ) {
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	if ( !trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	level.bots.num = 0;

	trap->Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM );
	if( *botsFile.string ) {
		G_LoadBotsFromFile(botsFile.string);
	}
	else {
		//G_LoadBotsFromFile("scripts/bots.txt");
		G_LoadBotsFromFile("botfiles/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap->FS_GetFileList("scripts", ".bot", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadBotsFromFile(filename);
	}
//	trap->Print( "%i bots parsed\n", level.bots.num );
}

/*
===============
G_GetBotInfoByNumber
===============
*/
char *G_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= level.bots.num ) {
		trap->Print( S_COLOR_RED "Invalid bot number: %i\n", num );
		return NULL;
	}
	return level.bots.infos[num];
}

/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < level.bots.num ; n++ ) {
		value = Info_ValueForKey( level.bots.infos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return level.bots.infos[n];
		}
	}

	return NULL;
}

//rww - pd
void LoadPath_ThisLevel(void);
//end rww

extern void G_LoadVillageData(void);

/*
===============
G_InitBots
===============
*/
void G_InitBots( void ) {
	G_LoadBots();
	G_LoadArenas();

	trap->Cvar_Register(&bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO);

#ifdef __NPC_MINPLAYERS__
	trap->Cvar_Register(&npc_imperials, "npc_imperials", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_rebels, "npc_rebels", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_mandalorians, "npc_mandalorians", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_mercs, "npc_mercs", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_pirates, "npc_pirates", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_wildlife, "npc_wildlife", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_civilians, "npc_civilians", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_vendors, "npc_vendors", "0", CVAR_ARCHIVE);
#endif //__NPC_MINPLAYERS__
	trap->Cvar_Register(&npc_pathing, "npc_pathing", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_wptonav, "npc_wptonav", "0", CVAR_ARCHIVE);
	trap->Cvar_Register(&npc_followers, "npc_followers", "0", CVAR_ARCHIVE);

	// Load village/town/city location info...
	G_LoadVillageData();

	//rww - new bot route stuff
	LoadPath_ThisLevel();
	//end rww
}
