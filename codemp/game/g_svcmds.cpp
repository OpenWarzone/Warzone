// Copyright (C) 1999-2000 Id Software, Inc.
//

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

typedef struct ipFilter_s {
	uint32_t mask, compare;
} ipFilter_t;

#define	MAX_IPFILTERS (1024)

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter( char *s, ipFilter_t *f ) {
	char num[128];
	int i, j;
	byteAlias_t b, m;

	b.ui = m.ui = 0u;

	for ( i=0; i<4; i++ ) {
		if ( *s < '0' || *s > '9' ) {
			if ( *s == '*' ) {
				// 'match any'
				// b[i] and m[i] to 0
				s++;
				if ( !*s )
					break;
				s++;
				continue;
			}
			trap->Print( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while ( *s >= '0' && *s <= '9' )
			num[j++] = *s++;

		num[j] = 0;
		b.b[i] = (byte)atoi( num );
		m.b[i] = 0xFF;

		if ( !*s )
			break;

		s++;
	}

	f->mask = m.ui;
	f->compare = b.ui;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans( void ) {
	byteAlias_t b, m;
	int i, j;
	char ip[NET_ADDRSTRMAXLEN], iplist_final[MAX_CVAR_VALUE_STRING];

	*iplist_final = 0;
	for ( i=0; i<numIPFilters; i++ ) {
		if ( ipFilters[i].compare == 0xFFFFFFFFu )
			continue;

		b.ui = ipFilters[i].compare;
		m.ui = ipFilters[i].mask;
		*ip = 0;
		for ( j=0; j<4; j++ ) {
			if ( m.b[j] != 0xFF )
				Q_strcat( ip, sizeof( ip ), "*" );
			else
				Q_strcat( ip, sizeof( ip ), va( "%i", (int)b.c[j] ) );
			Q_strcat( ip, sizeof( ip ), (j<3) ? "." : " " );
		}
		if ( strlen( iplist_final )+strlen( ip ) < MAX_CVAR_VALUE_STRING )
			Q_strcat( iplist_final, sizeof( iplist_final ), ip );
		else {
			Com_Printf( "g_banIPs overflowed at MAX_CVAR_VALUE_STRING\n" );
			break;
		}
	}

	trap->Cvar_Set( "g_banIPs", iplist_final );
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket( char *from ) {
	int i;
	uint32_t in;
	byteAlias_t m;
	char *p;

	i = 0;
	p = from;
	while ( *p && i < 4 ) {
		m.b[i] = 0;
		while ( *p >= '0' && *p <= '9' ) {
			m.b[i] = m.b[i]*10 + (*p - '0');
			p++;
		}
		if ( !*p || *p == ':' )
			break;
		i++, p++;
	}

	in = m.ui;

	for ( i=0; i<numIPFilters; i++ ) {
		if ( (in & ipFilters[i].mask) == ipFilters[i].compare )
			return (qboolean)(g_filterBan.integer != 0);
	}

	return (qboolean)(g_filterBan.integer == 0);
}

/*
=================
AddIP
=================
*/
static void AddIP( char *str ) {
	int i;

	for ( i=0; i<numIPFilters; i++ ) {
		if ( ipFilters[i].compare == 0xFFFFFFFFu )
			break; // free spot
	}
	if ( i == numIPFilters ) {
		if ( numIPFilters == MAX_IPFILTERS ) {
			trap->Print( "IP filter list is full\n" );
			return;
		}
		numIPFilters++;
	}

	if ( !StringToFilter( str, &ipFilters[i] ) )
		ipFilters[i].compare = 0xFFFFFFFFu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans( void ) {
	char *s = NULL, *t = NULL, str[MAX_CVAR_VALUE_STRING] = {0};

	Q_strncpyz( str, g_banIPs.string, sizeof( str ) );

	for ( t=s=g_banIPs.string; *t; t=s ) {
		s = strchr( s, ' ' );
		if ( !s )
			break;

		while ( *s == ' ' )
			*s++ = 0;

		if ( *t )
			AddIP( t );
	}
}

/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Argc() < 2 ) {
		trap->Print("Usage: addip <ip-mask>\n");
		return;
	}

	trap->Argv( 1, str, sizeof( str ) );

	AddIP( str );
}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Argc() < 2 ) {
		trap->Print("Usage: removeip <ip-mask>\n");
		return;
	}

	trap->Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
		return;

	for (i=0 ; i<numIPFilters ; i++) {
		if (ipFilters[i].mask == f.mask	&&
			ipFilters[i].compare == f.compare) {
			ipFilters[i].compare = 0xffffffffu;
			trap->Print ("Removed.\n");

			UpdateIPBans();
			return;
		}
	}

	trap->Print ( "Didn't find %s.\n", str );
}

void Svcmd_ListIP_f (void)
{
	int		i, count = 0;
	byteAlias_t b;

	for(i = 0; i < numIPFilters; i++) {
		if ( ipFilters[i].compare == 0xffffffffu )
			continue;

		b.ui = ipFilters[i].compare;
		trap->Print ("%i.%i.%i.%i\n", b.b[0], b.b[1], b.b[2], b.b[3]);
		count++;
	}
	trap->Print ("%i bans.\n", count);
}

/*
===================
Svcmd_EntityList_f
===================
*/
extern stringID_table_t ClassTable[];

void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities;
	for (e = 0; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		trap->Print("%4i (%4i):", e, check->s.number);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			trap->Print("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			trap->Print("ET_PLAYER           ");
			break;
		case ET_ITEM:
			trap->Print("ET_ITEM             ");
			break;
		case ET_LIGHTSABER:
			trap->Print("ET_LIGHTSABER       ");
			break;
		case ET_MISSILE:
			trap->Print("ET_MISSILE          ");
			break;
		case ET_SPECIAL:
			trap->Print("ET_SPECIAL          ");
			break;
		case ET_HOLOCRON:
			trap->Print("ET_HOLOCRON         ");
			break;
		case ET_MOVER:
			trap->Print("ET_MOVER            ");
			break;
		case ET_BEAM:
			trap->Print("ET_BEAM             ");
			break;
		case ET_PORTAL:
			trap->Print("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			trap->Print("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			trap->Print("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			trap->Print("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			trap->Print("ET_INVISIBLE        ");
			break;
		case ET_NPC:
			trap->Print("ET_NPC              ");
			break;
		case ET_TEAM:
			trap->Print("ET_TEAM             ");
			break;
		case ET_BODY:
			trap->Print("ET_BODY             ");
			break;
		case ET_TERRAIN:
			trap->Print("ET_TERRAIN          ");
			break;
		case ET_FX:
			trap->Print("ET_FX               ");
			break;
		case ET_MOVER_MARKER:
			trap->Print("ET_MOVER_MARKER     ");
			break;
		case ET_SPAWNPOINT:
			trap->Print("ET_SPAWNPOINT       ");
			break;
		case ET_TRIGGER_HURT:
			trap->Print("ET_TRIGGER_HURT     ");
			break;
		case ET_SERVERMODEL:
			trap->Print("ET_SERVERMODEL      ");
			break;
		case ET_NPC_SPAWNER:
			trap->Print("ET_NPC_SPAWNER      ");
			break;
		case ET_FREED:
			trap->Print("ET_FREED            ");
			break;
		default:
			if (!check->freeAfterEvent) // Not a tempEntity... wtf???
				trap->Print("%-3i                ", check->s.eType);
			else
				trap->Print("EVENT: %-3i          ", check->s.eType);
			break;
		}
		
		trap->Print("Linked: %s  ", check->r.linked ? "TRUE " : "FALSE");

		trap->Print("Origin: %-6i %-6i %-6i  ", (int)check->r.currentOrigin[0], (int)check->r.currentOrigin[1], (int)check->r.currentOrigin[2]);

		if (check->client)
			trap->Print("Health: %-5i (max %-5i) psHealth: %-5i (max %-5i) Dead: %s  ", check->s.health, check->s.maxhealth, check->client->ps.stats[STAT_HEALTH], check->client->ps.stats[STAT_MAX_HEALTH], (check->s.eFlags & EF_DEAD) ? "TRUE " : "FALSE");
		else
			trap->Print("Health: %-5i (max %-5i) psHealth: NONE  (max NONE ) Dead: %s  ", check->s.health, check->s.maxhealth, (check->s.eFlags & EF_DEAD) ? "TRUE " : "FALSE");

		if ( check->classname ) 
		{
			if (check->s.eType == ET_NPC)
			{
				trap->Print("%s (%s)", check->classname, ClassTable[check->s.NPC_class].name);
			}
			else
			{
				trap->Print("%s", check->classname);
			}
		}

		trap->Print("\n");
	}
}

qboolean StringIsInteger( const char *s );
/*
===================
ClientForString
===================
*/
gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		cleanInput[MAX_STRING_CHARS];

	// numeric values could be slot numbers
	if ( StringIsInteger( s ) ) {
		idnum = atoi( s );
		if ( idnum >= 0 && idnum < level.maxclients ) {
			cl = &level.clients[idnum];
			if ( cl->pers.connected == CON_CONNECTED ) {
				return cl;
			}
		}
	}

	Q_strncpyz( cleanInput, s, sizeof(cleanInput) );
	Q_StripColor( cleanInput );

	// check for a name match
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname_nocolor, cleanInput ) ) {
			return cl;
		}
	}

	trap->Print( "User %s is not on the server\n", s );
	return NULL;
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	if ( trap->Argc() < 3 ) {
		trap->Print("Usage: forceteam <player> <team>\n");
		return;
	}

	// find the player
	trap->Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap->Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str );
}

char *ConcatArgs( int start );
void Svcmd_Say_f( void ) {
	char *p = NULL;
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT] = {0};

	if ( trap->Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	if ( strlen( p ) >= MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Say_f from -1 (server) has been truncated: %s\n", p );
	}

	Q_strncpyz( text, p, sizeof(text) );
	Q_strstrip( text, "\n\r", "  " );

	//G_LogPrintf( "say: server: %s\n", text );
	trap->SendServerCommand( -1, va("print \"server: %s\n\"", text ) );
}

void DL_MemStats( void )
{
	/*
	#define public_cALLOc    dlcalloc
#define public_fREe      dlfree
#define public_cFREe     dlcfree
#define public_mALLOc    dlmalloc
#define public_mEMALIGn  dlmemalign
#define public_rEALLOc   dlrealloc
#define public_vALLOc    dlvalloc
#define public_pVALLOc   dlpvalloc
#define public_mALLINFo  dlmallinfo
#define public_mALLOPt   dlmallopt
#define public_mTRIm     dlmalloc_trim
#define public_mSTATs    dlmalloc_stats
#define public_mUSABLe   dlmalloc_usable_size
#define public_iCALLOc   dlindependent_calloc
#define public_iCOMALLOc dlindependent_comalloc*/
	//dlmalloc_stats();
}

typedef struct svcmd_s {
	const char	*name;
	void		(*func)(void);
	qboolean	dedicated;
} svcmd_t;

int svcmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((svcmd_t*)b)->name );
}

/*
Svcmd_NPC_Kill_f
*/
extern stringID_table_t TeamTable[];
extern char	*TeamNames[FACTION_NUM_FACTIONS];

void Svcmd_NPC_Kill_f(void)
{
	int			n;
	gentity_t	*player;
	char		name[1024];
	npcteam_t	killTeam = NPCTEAM_FREE;
	qboolean	killNonSF = qfalse;

	trap->Argv(1, name, 1024);

	if (!name[0])
	{
		Com_Printf(S_COLOR_RED"Error, Expected:\n");
		Com_Printf(S_COLOR_RED"NPCkill '[NPC targetname]' - kills NPCs with certain targetname\n");
		Com_Printf(S_COLOR_RED"or\n");
		Com_Printf(S_COLOR_RED"NPCkill 'all' - kills all NPCs\n");
		return;
	}

	for (n = 1; n < ENTITYNUM_MAX_NORMAL; n++)
	{
		player = &g_entities[n];
		if (!player->inuse) {
			continue;
		}
		if (killNonSF)
		{
			if (player)
			{
				if (player->client)
				{
					if (player->client->playerTeam != NPCTEAM_PLAYER)
					{
						Com_Printf(S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname);
						player->health = 0;

						if (player->die && player->client)
						{
							player->die(player, player, player, player->client->pers.maxHealth, MOD_UNKNOWN);
						}
					}
				}
				else if (player->NPC_type && player->classname && player->classname[0] && Q_stricmp("NPC_starfleet", player->classname) != 0)
				{//A spawner, remove it
					Com_Printf(S_COLOR_GREEN"Removing NPC spawner %s with NPC named %s\n", player->NPC_type, player->NPC_targetname);
					G_FreeEntity(player);
					//FIXME: G_UseTargets2(player, player, player->NPC_target & player->target);?
				}
			}
		}
		else if (player && player->NPC && player->client)
		{
			if (killTeam != NPCTEAM_FREE)
			{
				if (player->client->playerTeam == killTeam)
				{
					Com_Printf(S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname);
					player->health = 0;
					player->s.health = player->client->ps.stats[STAT_HEALTH] = 0;
					player->client->ps.eFlags |= EF_DEAD;
					player->s.eFlags |= EF_DEAD;
					if (player->die)
					{
						player->die(player, player, player, player->client->pers.maxHealth, MOD_UNKNOWN);
					}
				}
			}
			else if ((player->targetname && Q_stricmp(name, player->targetname) == 0)
				|| Q_stricmp(name, "all") == 0)
			{
				Com_Printf(S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname);
				player->health = 0;
				player->s.health = player->client->ps.stats[STAT_HEALTH] = 0;
				player->client->ps.eFlags |= EF_DEAD;
				player->s.eFlags |= EF_DEAD;
				if (player->die)
				{
					player->die(player, player, player, 100, MOD_UNKNOWN);
				}
			}
		}
	}
}

void G_CheckFields( void );
void G_CheckSpawns( void );

/* This array MUST be sorted correctly by alphabetical name field */
svcmd_t svcmds[] = {
	{ "addbot",						Svcmd_AddBot_f,						qfalse },
	{ "addip",						Svcmd_AddIP_f,						qfalse },
	{ "botlist",					Svcmd_BotList_f,					qfalse },
	{ "checkfields",				G_CheckFields,						qfalse },
	{ "checkspawns",				G_CheckSpawns,						qfalse },
	//	{ "dlmemstats",					DL_MemStats,						qfalse },
	{ "entitylist",					Svcmd_EntityList_f,					qfalse },
	{ "forceteam",					Svcmd_ForceTeam_f,					qfalse },
	{ "game_memory",				Svcmd_GameMem_f,					qfalse },
	{ "listip",						Svcmd_ListIP_f,						qfalse },
	{ "npckill", 					Svcmd_NPC_Kill_f,					qfalse },
	{ "removeip",					Svcmd_RemoveIP_f,					qfalse },
	{ "say",						Svcmd_Say_f,						qtrue },
	{ "toggleuserinfovalidation",	Svcmd_ToggleUserinfoValidation_f,	qfalse },
};
static const size_t numsvcmds = ARRAY_LEN( svcmds );

/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS] = {0};
	svcmd_t	*command = NULL;

	trap->Argv( 0, cmd, sizeof( cmd ) );

	command = (svcmd_t *)bsearch( cmd, svcmds, numsvcmds, sizeof( svcmds[0] ), svcmdcmp );
	if ( !command )
		return qfalse;

	if ( command->dedicated && !dedicated.integer )
		return qfalse;

	command->func();
	return qtrue;
}

