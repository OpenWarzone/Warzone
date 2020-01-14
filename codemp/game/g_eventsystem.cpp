#include "g_local.h"
#include "g_nav.h"
#include "ai_dominance_main.h"
#include "b_local.h"
#include <thread>

//
// Defines...
//

//#define __USE_ALL_IMPERIAL_SHIPS__					// Enables the victory Star Destroyer. It has too many textures and slows loading and renderring more than the others, so i'm disabling it for now...
//#define __USE_ALL_REBEL_SHIPS__						// Use all the smaller rebel event ships? I don't think so for now, just use the calamari cruiser... Faster loading and better looking...

//
//
//
extern qboolean			EVENTS_ENABLED;
extern float			EVENT_BUFFER;
extern float			EVENT_TRACE_SIZE;

extern float			MAP_WATER_LEVEL;

typedef enum {
	EVENT_SIZE_SMALL,
	EVENT_SIZE_MEDIUM,
	EVENT_SIZE_LARGE,
	EVENT_SIZE_MAX
} eventSize_t;


//#define					DISTANCE_BETWEEN_EVENTS (EVENT_BUFFER >= 8192.0) ? EVENT_BUFFER : 32768.0
float					DISTANCE_BETWEEN_EVENTS = EVENT_BUFFER;
#define					MAX_TEAM_EVENT_AREAS 256
#define					MAX_TEAM_EVENT_AREA_TRIES 4096

const float				EVENTS_VERSION = 1.0;

qboolean				event_areas_initialized = qfalse;
int						num_event_areas = 0;
vec3_t					event_areas[MAX_TEAM_EVENT_AREAS] = { { 0.0 } };
team_t					event_areas_current_team[MAX_TEAM_EVENT_AREAS] = { { FACTION_SPECTATOR } };
eventSize_t				event_areas_event_size[MAX_TEAM_EVENT_AREAS] = { { EVENT_SIZE_SMALL } };
int						event_areas_spawn_count[MAX_TEAM_EVENT_AREAS] = { { 0 } };
int						event_areas_spawn_wave[MAX_TEAM_EVENT_AREAS] = { { 0 } };
int						event_areas_spawn_quality[MAX_TEAM_EVENT_AREAS] = { { 0 } };
qboolean				event_areas_wave_filled[MAX_TEAM_EVENT_AREAS] = { { qfalse } };
qboolean				event_areas_has_ship[MAX_TEAM_EVENT_AREAS] = { { qfalse } };

char					*miscModelString = "misc_model";


void G_EventShipThink(gentity_t *ent)
{
	ent->nextthink = level.time + 1000;
}

void G_EventModelPrecache(void)
{
	if (!EVENTS_ENABLED) return;

	G_ModelIndex("models/warzone/ships/isd/isd.3ds");
	G_ModelIndex("models/warzone/ships/isd2/isd2.3ds");
	G_ModelIndex("models/warzone/ships/dominator/dominator.3ds");
	G_ModelIndex("models/warzone/ships/knight/knight.3ds");
#ifdef __USE_ALL_IMPERIAL_SHIPS__
	G_ModelIndex("models/warzone/ships/victory/victory.3ds");
#endif //__USE_ALL_IMPERIAL_SHIPS__
	
	G_ModelIndex("models/warzone/ships/calamari/calamari.3ds");
#ifdef __USE_ALL_REBEL_SHIPS__
	G_ModelIndex("models/warzone/ships/nebulonb2/neb.3ds");
	G_ModelIndex("models/warzone/ships/medfrig/medfrig.md3");
	G_ModelIndex("models/warzone/ships/corvette/corvette.3ds");
	G_ModelIndex("models/warzone/ships/falcon/falcon.md3");
	G_ModelIndex("models/warzone/ships/u-wing/uwing.3ds");
	G_ModelIndex("models/warzone/ships/yt2400/yt2400.3ds");
#endif //__USE_ALL_REBEL_SHIPS__

	G_ModelIndex("models/warzone/ships/bothan/bothan.3ds");

	G_ModelIndex("models/warzone/ships/maleo1/maleo1.3ds");
	
	G_SoundIndex("sound/vehicles/shuttle/loop.wav");
	G_SoundIndex("sound/movers/objects/shuttle_hover_8.wav");
	G_SoundIndex("sound/movers/objects/raven_hover_9.wav");
	G_SoundIndex("sound/movers/objects/raven_hover_4.wav");
	G_SoundIndex("sound/vehicles/x-wing/loop.wav");
	G_SoundIndex("sound/vehicles/ambience/cockpit_steady.wav");


	// Used by cgame fighter escorts, so precache these as well...
	G_ModelIndex("models/map_objects/ships/tie_fighter.md3");
	G_ModelIndex("models/map_objects/ships/x_wing_nogear.md3");
	G_SoundIndex("sound/vehicles/tie/loop.wav");
}

void G_CreateSpawnVesselForEventArea(int area)
{
	if (event_areas_has_ship[area])
	{// Already spawned the ship for this area...
		return;
	}

	team_t team = event_areas_current_team[area];
	char modelName[128] = { { 0 } };
	int modelFrame = 0;
	int modelScale = 1;
	int loopSound = 0;
	float zOffset = 8192.0;

	event_areas_event_size[area] = EVENT_SIZE_SMALL;

	switch (team)
	{
	case FACTION_WILDLIFE:
		{// Wildlife is native and does not spawn from a ship...
			return;
		}
		break;
	case FACTION_EMPIRE:
		{
#ifdef __USE_ALL_IMPERIAL_SHIPS__
			int choice = irand(0, 4);
#else //!__USE_ALL_IMPERIAL_SHIPS__
			int choice = irand(0, 3);
#endif //__USE_ALL_IMPERIAL_SHIPS__

			if (choice == 0)
			{
				sprintf(modelName, "models/warzone/ships/isd/isd.3ds");
				loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
				modelScale = 56;
				zOffset = 32768.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
			else if (choice == 1)
			{
				sprintf(modelName, "models/warzone/ships/isd2/isd2.3ds");
				loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
				modelScale = 56;
				zOffset = 32768.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
			else if (choice == 2)
			{
				sprintf(modelName, "models/warzone/ships/dominator/dominator.3ds");
				loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
				modelScale = 64;
				zOffset = 49152.0;
				event_areas_event_size[area] = EVENT_SIZE_MEDIUM;
			}
			else
#ifdef __USE_ALL_IMPERIAL_SHIPS__
				if (choice == 3)
#endif //__USE_ALL_IMPERIAL_SHIPS__
			{
				sprintf(modelName, "models/warzone/ships/knight/knight.3ds");
				loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
				modelScale = 112;
				zOffset = 65536.0;
				event_areas_event_size[area] = EVENT_SIZE_LARGE;
			}
#ifdef __USE_ALL_IMPERIAL_SHIPS__
			else
			{
				sprintf(modelName, "models/warzone/ships/victory/victory.3ds");
				loopSound = G_SoundIndex("sound/vehicles/ambience/cockpit_steady.wav");
				modelScale = 56;
				zOffset = 32768.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
#endif //__USE_ALL_IMPERIAL_SHIPS__
		}
		break;
	case FACTION_REBEL:
		{
#ifdef __USE_ALL_REBEL_SHIPS__
			int choice = irand(0, 5);
			if (choice == 0)
			{
				sprintf(modelName, "models/warzone/ships/calamari/calamari.3ds");
				loopSound = G_SoundIndex("sound/movers/objects/shuttle_hover_8.wav");
				modelScale = 56;
				zOffset = 32768.0;
				event_areas_event_size[area] = EVENT_SIZE_LARGE;
			}
			else if (choice == 1)
			{
				sprintf(modelName, "models/warzone/ships/nebulonb2/neb.3ds");
				loopSound = G_SoundIndex("sound/movers/objects/raven_hover_9.wav");
				modelFrame = 0;
				modelScale = 12;
				zOffset = 6144.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
			else if (choice == 2)
			{
				sprintf(modelName, "models/warzone/ships/medfrig/medfrig.md3");
				loopSound = G_SoundIndex("sound/movers/objects/raven_hover_9.wav");
				modelFrame = 0;
				modelScale = 12;
				zOffset = 6144.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
			else if (choice == 3)
			{
				sprintf(modelName, "models/warzone/ships/corvette/corvette.3ds");
				loopSound = G_SoundIndex("sound/movers/objects/raven_hover_4.wav");
				modelFrame = 0;
				modelScale = 16;
				zOffset = 4096.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
			else if (choice == 4)
			{
				sprintf(modelName, "models/warzone/ships/falcon/falcon.md3");
				loopSound = G_SoundIndex("sound/vehicles/x-wing/loop.wav");
				modelFrame = 0;
				modelScale = 16;
				zOffset = 4096.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
			else
			{
				sprintf(modelName, "models/warzone/ships/u-wing/uwing.3ds");
				loopSound = G_SoundIndex("sound/vehicles/x-wing/loop.wav");
				modelFrame = 0;
				modelScale = 6;
				zOffset = 2048.0;
				event_areas_event_size[area] = EVENT_SIZE_SMALL;
			}
#else //!__USE_ALL_REBEL_SHIPS__
		sprintf(modelName, "models/warzone/ships/calamari/calamari.3ds");
		loopSound = G_SoundIndex("sound/movers/objects/shuttle_hover_8.wav");
		modelScale = 56;
		zOffset = 32768.0;
		event_areas_event_size[area] = EVENT_SIZE_LARGE;
#endif //__USE_ALL_REBEL_SHIPS__
		}
		break;
	case FACTION_MANDALORIAN:
		{
			sprintf(modelName, "models/warzone/ships/bothan/bothan.3ds");
			loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
			modelScale = 42;
			zOffset = 24576.0;
			event_areas_event_size[area] = EVENT_SIZE_MEDIUM;
		}
		break;
	case FACTION_MERC:
		{
			sprintf(modelName, "models/warzone/ships/maleo1/maleo1.3ds");
			loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
			modelScale = 56;
			zOffset = 32768.0;
			event_areas_event_size[area] = EVENT_SIZE_MEDIUM;
		}
		break;
	case FACTION_PIRATES:
		{
			sprintf(modelName, "models/warzone/ships/maleo1/maleo1.3ds");
			loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
			modelScale = 56;
			zOffset = 32768.0;
			event_areas_event_size[area] = EVENT_SIZE_MEDIUM;
		}
		break;
	default:
		break;
	}

	gentity_t *ent = G_Spawn();
	
	VectorSet(ent->r.mins, -512, -512, -512);
	VectorSet(ent->r.maxs, 512, 512, 512);

	ent->s.eType = ET_SERVERMODEL;
	ent->classname = miscModelString;
	ent->model = modelName;
	ent->s.frame = modelFrame;
	
	// Mark as bobbing...
	ent->s.generic1 = 1;

	ent->s.modelindex = G_ModelIndex(ent->model);
	ent->s.iModelScale = modelScale; // NOTE: EF_SERVERMODEL does NOT use / 100.0 on scale.
	
	// No culling on hoverring event ships...
	ent->r.svFlags |= SVF_BROADCAST;
	
	//ent->s.eFlags |= EF_PERMANENT;// 0;
	ent->s.eFlags = 0;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;
	ent->s.teamowner = team;
	ent->s.owner = ENTITYNUM_NONE;

	//ent->neverFree = qtrue; // hmmm

	ent->s.loopSound = loopSound;

	//TODO: Come from sky and hover...
	ent->nextthink = level.time + 200;
	ent->think = G_EventShipThink;

	VectorCopy(event_areas[area], ent->s.origin);
	ent->s.origin[2] += zOffset;// 2048.0;

	G_SetOrigin(ent, ent->s.origin);
	VectorCopy(vec3_origin, ent->s.angles);
	G_SetAngles(ent, ent->s.angles);

	trap->LinkEntity((sharedEntity_t *)ent);

	event_areas_has_ship[area] = qtrue;

	/*
	trap->Print("EVENT: Spawned ship %s for team %s above event area %i (position %f %f %f) at %f %f %f.\n", modelName, TeamName(event_areas_current_team[area]), area
		, event_areas[area][0], event_areas[area][1], event_areas[area][2]
		, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
	*/
}

void G_SaveEventAreas(void)
{
	fileHandle_t	f;
	int				i = 0;

	vmCvar_t	mapname;
	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

	trap->FS_Open(va("maps/%s.eventAreas", mapname.string), &f, FS_WRITE);

	if (!f)
	{
		trap->Print("^1*** ^3%s^5: Failed to open eventAreas file ^7maps/%s.eventAreas^5 for save.\n", "EVENTS", mapname.string);
		return;
	}

	trap->FS_Write(&EVENTS_VERSION, sizeof(float), f);
	trap->FS_Write(&num_event_areas, sizeof(int), f);
	trap->FS_Write(&event_areas, sizeof(vec3_t) * num_event_areas, f);
	trap->FS_Close(f);
}

qboolean G_LoadEventAreas(void)
{
	fileHandle_t	f;
	int				i = 0;

	vmCvar_t	mapname;
	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

	trap->FS_Open(va("maps/%s.eventAreas", mapname.string), &f, FS_READ);

	if (!f)
	{// Will generate a new one...
		return qfalse;
	}

	float version = 0.0;

	trap->FS_Read(&version, sizeof(float), f);

	if (version != EVENTS_VERSION)
	{// Will generate a new one...
		trap->FS_Close(f);
		return qfalse;
	}

	team_t CURRENT_FACTION = FACTION_EMPIRE;

	for (int i = 0; i < MAX_TEAM_EVENT_AREAS; i++)
	{
		VectorClear(event_areas[i]);
		event_areas_event_size[i] = EVENT_SIZE_SMALL;
		event_areas_spawn_count[i] = 0;
		event_areas_spawn_wave[i] = 0;
		event_areas_spawn_quality[i] = 0;
		event_areas_wave_filled[i] = qfalse;
		event_areas_has_ship[i] = qfalse;

		//trap->Print("EVENTS: area %i is team %s.\n", i, TeamName(CURRENT_FACTION));

		event_areas_current_team[i] = CURRENT_FACTION;
		CURRENT_FACTION = (team_t)((int)CURRENT_FACTION + 1);

		if (CURRENT_FACTION > FACTION_PIRATES)
		{// Back to the start...
			CURRENT_FACTION = FACTION_EMPIRE;
		}
	}

	trap->FS_Read(&num_event_areas, sizeof(int), f);
	trap->FS_Read(&event_areas, sizeof(vec3_t) * num_event_areas, f);
	trap->FS_Close(f);


	//
	// Refresh the event area owners per load, in case I add new factions...
	//
	CURRENT_FACTION = FACTION_EMPIRE;

	for (int i = 0; i < num_event_areas; i++)
	{
		event_areas_current_team[i] = CURRENT_FACTION;
		CURRENT_FACTION = (team_t)((int)CURRENT_FACTION + 1);

		if (CURRENT_FACTION > FACTION_PIRATES)
		{// Back to the start...
			CURRENT_FACTION = FACTION_EMPIRE;
		}
	}

	return qtrue;
}

qboolean HasSkyVisibility(vec3_t pos)
{
	trace_t tr;
	vec3_t start, up;
	VectorSet(start, pos[0], pos[1], pos[2] + 48.0);
	VectorSet(up, start[0], start[1], start[2] + 999999.9);
	trap->Trace(&tr, start, NULL, NULL, up, -1, MASK_SOLID, qfalse, 0, 0);
	
	if (tr.surfaceFlags & SURF_SKY)
	{
		if (EVENT_TRACE_SIZE == 0.0)
		{
			return qtrue;
		}
		else
		{
			vec3_t mins, maxs;
			VectorSet(start, pos[0], pos[1], pos[2] + 384.0);
			VectorSet(up, start[0], start[1], start[2] + 512.0);
			VectorSet(mins, -EVENT_TRACE_SIZE, -EVENT_TRACE_SIZE, -1.0);
			VectorSet(maxs, EVENT_TRACE_SIZE, EVENT_TRACE_SIZE, 1.0);
			trap->Trace(&tr, start, mins, maxs, up, -1, MASK_SOLID, qfalse, 0, 0);

			if (!tr.startsolid && !tr.allsolid)
			{
				return qtrue;
			}
		}
	}
	
	

	return qfalse;
}

void G_SetupEventAreas(void)
{
	if (!EVENTS_ENABLED)
	{
		num_event_areas = 0;

		if (!event_areas_initialized)
		{
			trap->Print("^1*** ^3%s^5: Events are disabled for this map. Enable them in it's mapinfo by adding ^3ENABLE_EVENTS=1^5 to an ^3[EVENTS]^5 section.\n", "EVENTS", num_event_areas);
			event_areas_initialized = qtrue;
		}

		return;
	}

	if (!event_areas_initialized)
	{
		if (!G_LoadEventAreas())
		{
			num_event_areas = 0;

			DISTANCE_BETWEEN_EVENTS = EVENT_BUFFER;

			trap->Print("^1*** ^3%s^5: Generating event areas for this map's first load using a radius of ^3%2f^5 and trace size of ^3%2f^5. This may take a few minutes.\n", "EVENTS", DISTANCE_BETWEEN_EVENTS, EVENT_TRACE_SIZE);
			trap->SendServerCommand(-1, va("cp \"Generating event areas for this map's first load using a radius of ^3%2f^5 and trace size of ^3%2f^5.\nThis may take a few minutes.\n\"", DISTANCE_BETWEEN_EVENTS, EVENT_TRACE_SIZE));

			team_t CURRENT_FACTION = FACTION_EMPIRE;

			for (int i = 0; i < MAX_TEAM_EVENT_AREAS; i++)
			{
				VectorClear(event_areas[i]);
				event_areas_event_size[i] = EVENT_SIZE_SMALL;
				event_areas_spawn_count[i] = 0;
				event_areas_spawn_wave[i] = 0;
				event_areas_spawn_quality[i] = 0;
				event_areas_wave_filled[i] = qfalse;
				event_areas_has_ship[i] = qfalse;

				//trap->Print("EVENTS: area %i is team %s.\n", i, TeamName(CURRENT_FACTION));

				event_areas_current_team[i] = CURRENT_FACTION;
				CURRENT_FACTION = (team_t)((int)CURRENT_FACTION + 1);

				if (CURRENT_FACTION > FACTION_PIRATES)
				{// Back to the start...
					CURRENT_FACTION = FACTION_EMPIRE;
				}
			}

#if defined(__USE_NAVLIB__) || defined(__USE_NAVLIB_SPAWNPOINTS__)
			if (G_NavmeshIsLoaded())
			{
				int tries = 0;

				while (num_event_areas < MAX_TEAM_EVENT_AREAS && tries < MAX_TEAM_EVENT_AREA_TRIES)
				{
					qboolean bad = qfalse;
					vec3_t org;
					FindRandomNavmeshSpawnpoint(NULL, org);

					for (int i = 0; i < num_event_areas; i++)
					{
						if (Distance(org, event_areas[i]) < DISTANCE_BETWEEN_EVENTS)
						{
							bad = qtrue;
							break;
						}
					}

					if (!bad)
					{
						if (!HasSkyVisibility(org))
						{
							trap->Print("EVENT AREA GENERATOR: Attempt %i of %i. %f %f %f failed trace. %i areas found so far.\n", tries, MAX_TEAM_EVENT_AREA_TRIES, org[0], org[1], org[2], num_event_areas);
							continue;
						}
						else
						{
							trap->Print("EVENT AREA GENERATOR: Attempt %i of %i. %f %f %f SUCCEEDED trace. %i areas found so far.\n", tries, MAX_TEAM_EVENT_AREA_TRIES, org[0], org[1], org[2], num_event_areas+1);
						}
#if 0
						if (!IsSpawnpointReachable(org))
						{
							//trap->Print("%f %f %f -> %f %f %f is NOT pathable. padawan is at %f %f %f.\n", org[0], org[1], org[2], gameSpawnPoint[0], gameSpawnPoint[1], gameSpawnPoint[2], testerPadawan->r.currentOrigin[0], testerPadawan->r.currentOrigin[1], testerPadawan->r.currentOrigin[2]);
							continue;
						}
						else
						{
							trap->Print("%f %f %f -> %f %f %f is pathable. padawan is at %f %f %f.\n", org[0], org[1], org[2], gameSpawnPoint[0], gameSpawnPoint[1], gameSpawnPoint[2], testerPadawan->r.currentOrigin[0], testerPadawan->r.currentOrigin[1], testerPadawan->r.currentOrigin[2]);
						}
#endif
						VectorCopy(org, event_areas[num_event_areas]);
						num_event_areas++;
					}

					tries++;
				}
			}
			else if (gWPNum > 0)
#endif //defined(__USE_NAVLIB__) || defined(__USE_NAVLIB_SPAWNPOINTS__)
			{
				int tries = 0;

				while (num_event_areas < MAX_TEAM_EVENT_AREAS && tries < MAX_TEAM_EVENT_AREA_TRIES)
				{
					qboolean bad = qfalse;
					vec3_t org;
					int wp = irand(0, gWPNum - 1);
					VectorCopy(gWPArray[wp]->origin, org);

					for (int i = 0; i < num_event_areas; i++)
					{
						if (Distance(org, event_areas[i]) < DISTANCE_BETWEEN_EVENTS)
						{
							bad = qtrue;
							break;
						}
					}

					if (!bad)
					{
						VectorCopy(org, event_areas[num_event_areas]);
						num_event_areas++;
					}

					tries++;
				}
			}

			G_SaveEventAreas();

			trap->Print("^1*** ^3%s^5: Generated ^3%i ^5event areas for this map.\n", "EVENTS", num_event_areas);
		}
		else
		{
			trap->Print("^1*** ^3%s^5: Loaded ^3%i ^5event areas for this map.\n", "EVENTS", num_event_areas);

			/*for (int i = 0; i < num_event_areas; i++)
			{
				trap->Print("Area %i is at %f %f %f.\n", i, event_areas[i][0], event_areas[i][1], event_areas[i][2]);
			}*/
		}

		event_areas_initialized = qtrue;
	}
}

void G_InitEventAreas(void)
{
	// Init everything...
	num_event_areas = 0;
	event_areas_initialized = qfalse;

	memset(event_areas, 0, sizeof(event_areas));
	memset(event_areas_current_team, 0, sizeof(event_areas_current_team));
	memset(event_areas_event_size, EVENT_SIZE_SMALL, sizeof(event_areas_event_size));
	memset(event_areas_spawn_count, 0, sizeof(event_areas_spawn_count));
	memset(event_areas_spawn_wave, 0, sizeof(event_areas_spawn_wave));
	memset(event_areas_spawn_quality, 0, sizeof(event_areas_spawn_quality));
	memset(event_areas_wave_filled, qfalse, sizeof(event_areas_wave_filled));
	memset(event_areas_has_ship, 0, sizeof(event_areas_has_ship));
	
	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		gentity_t *ent = &g_entities[i];

		if (ent && ent->s.eType == ET_SERVERMODEL)
		{
			G_FreeEntity(ent);
		}
	}

	G_SetupEventAreas();
}

int G_GetEventsCount(void)
{
	if (!EVENTS_ENABLED || !event_areas_initialized)
	{
		if (event_areas_initialized)
		{
			return 0;
		}

		// Special case, so other stuff knows events have not been setup yet...
		return -1;
	}

	return num_event_areas;
}

void G_CountEventAreaSpawns(void)
{
	if (!EVENTS_ENABLED)
	{
		return;
	}

	if (num_event_areas > 0)
	{
		memset(event_areas_spawn_count, 0, sizeof(event_areas_spawn_count));

		for (int i = 0; i < MAX_GENTITIES; i++)
		{
			gentity_t *ent = &g_entities[i];

			if (NPC_IsCivilian(ent)) continue;
			if (NPC_IsVendor(ent)) continue;

			if (ent 
				&& ent->s.eType == ET_NPC 
				&& ent->inuse
				&& ent->r.linked
				//&& NPC_IsAlive(ent, ent) // we still want to wait for the bodies to disappear before the next wave...
				&& ent->spawnArea >= 0 
				&& ent->spawnArea < num_event_areas)
			{
				event_areas_spawn_count[ent->spawnArea]++;
			}
		}
	}
}

int G_SpawnCountForEvent(int eventNum)
{
	if (!EVENTS_ENABLED || eventNum < 0 || num_event_areas <= 0)
	{
		return 0;
	}

	return event_areas_spawn_count[eventNum];
}

int G_MaxSpawnsPerWave(int wave, eventSize_t eventSize)
{// NOTE: Must be in groups of 4, as this is how the spawnGroups system is set up... TODO: Varying event sizes...
	switch (wave)
	{
	case 0:
	default:
		// Trash...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 4;
			break;
		case EVENT_SIZE_MEDIUM:
			return 8;
			break;
		case EVENT_SIZE_LARGE:
			return 12;
			break;
		}
		break;
	case 1:
		// More Trash...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 12;
			break;
		case EVENT_SIZE_LARGE:
			return 16;
			break;
		}
		break;
	case 2:
		// Common...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 12;
			break;
		case EVENT_SIZE_LARGE:
			return 16;
			break;
		}
		break;
	case 3:
		// Commanders...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 12;
			break;
		case EVENT_SIZE_LARGE:
			return 16;
			break;
		}
		break;
	case 4:
		// Elites...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 12;
			break;
		case EVENT_SIZE_LARGE:
			return 16;
			break;
		}
		break;
	case 5:
		// Boss...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 12;
			break;
		case EVENT_SIZE_LARGE:
			return 16;
			break;
		}
		break;
	}
}

int G_WaveSpawnCountForEventWave(int eventNum)
{
	if (!EVENTS_ENABLED || eventNum < 0 || num_event_areas <= 0)
	{
		return 4;
	}

	return G_MaxSpawnsPerWave(event_areas_spawn_wave[eventNum], event_areas_event_size[eventNum]);
}

spawnGroupRarity_t G_GetRarityForEventWave(int eventNum)
{
	if (!EVENTS_ENABLED || eventNum < 0 || num_event_areas <= 0)
	{
		return RARITY_SPAM;
	}

	if (event_areas_spawn_quality[eventNum] > 0)
	{// Special case for end boss fight, spawn 1 boss spawn, and then elites with him...
		return RARITY_ELITE;
	}

	int wave = event_areas_spawn_wave[eventNum];

	switch (wave)
	{
	case 0:
	default:
		// Trash...
		return RARITY_SPAM;
		break;
	case 1:
		// Trash...
		return RARITY_SPAM;
		break;
	case 2:
		// Common...
		return RARITY_COMMON;
		break;
	case 3:
		// Commanders...
		return RARITY_OFFICER;
		break;
	case 4:
		// Elites...
		return RARITY_ELITE;
		break;
	case 5:
		// Boss...
		event_areas_spawn_quality[eventNum]++; // Force elite defenders to spawn after this boss...
		return RARITY_BOSS;
		break;
	}
}

void G_UpdateSpawnAreaWaves(void)
{
	if (!EVENTS_ENABLED)
	{
		return;
	}

	if (num_event_areas > 0)
	{
		for (int i = 0; i < num_event_areas; i++)
		{
			int wave = event_areas_spawn_wave[i];
			int maxWaveSpawns = G_MaxSpawnsPerWave(wave, event_areas_event_size[i]);
			int currentCount = event_areas_spawn_count[i];

			if (currentCount > maxWaveSpawns - 4)
			{// This event is filled up, make sure we wait until the wave completes before spawning NPCs...
				event_areas_wave_filled[i] = qtrue;
			}

			if (currentCount <= 0)
			{// This wave is completed, increment the wave, and allow new spawns...
				if (event_areas_wave_filled[i])
				{
					event_areas_spawn_wave[i]++;
					event_areas_spawn_quality[i] = 0;
					event_areas_wave_filled[i] = qfalse;
				}

				if (event_areas_spawn_wave[i] > 5)
				{// Reset to 0 for now... TODO: Switch to another event and ship hyperspace out/in over time...
					event_areas_spawn_wave[i] = 0;
					event_areas_spawn_quality[i] = 0;
				}
			}
		}
	}
}

void G_PrintEventAreaInfo(void)
{
	if (!EVENTS_ENABLED)
	{
		return;
	}

	for (int i = 0; i < num_event_areas; i++)
	{
		if (event_areas_spawn_count[i] > 0)
		{
			int currentWave = event_areas_spawn_wave[i];
			int maxWaveSpawns = G_MaxSpawnsPerWave(currentWave, event_areas_event_size[i]);
			int currentCount = event_areas_spawn_count[i];

			trap->Print("^5Area ^7%i^5 (wave ^7%i^5) has ^7%i^5/^7%i^5 spawned NPCs.\n", i, currentWave, currentCount, maxWaveSpawns);
		}
	}
}

extern vmCvar_t npc_imperials;
extern vmCvar_t npc_rebels;
extern vmCvar_t npc_mandalorians;
extern vmCvar_t npc_mercs;
extern vmCvar_t npc_pirates;
extern vmCvar_t npc_wildlife;

qboolean G_EnabledFactionEvent(int eventNum)
{
	team_t eventFaction = event_areas_current_team[eventNum];

	switch (eventFaction)
	{
	case FACTION_EMPIRE:
		trap->Cvar_Update(&npc_imperials);
		if (npc_imperials.integer)
			return qtrue;
		break;
	case FACTION_REBEL:
		trap->Cvar_Update(&npc_rebels);
		if (npc_rebels.integer)
			return qtrue;
		break;
	case FACTION_MANDALORIAN:
		trap->Cvar_Update(&npc_mandalorians);
		if (npc_mandalorians.integer)
			return qtrue;
		break;
	case FACTION_MERC:
		trap->Cvar_Update(&npc_mercs);
		if (npc_mercs.integer)
			return qtrue;
		break;
	case FACTION_PIRATES:
		trap->Cvar_Update(&npc_pirates);
		if (npc_pirates.integer)
			return qtrue;
		break;
	case FACTION_WILDLIFE:
		trap->Cvar_Update(&npc_wildlife);
		if (npc_wildlife.integer)
			return qtrue;
		break;
	case FACTION_SPECTATOR:
	default:
		return qtrue;
		break;
	}

	return qfalse;
}

int G_GetEventMostNeedingSpawns(void)
{
	int leastSpawnsEvent = -1;
	int leastSpawnsCount = 9999;

	if (!EVENTS_ENABLED)
	{
		return -1;
	}

	if (num_event_areas > 0)
	{
		for (int i = 0; i < num_event_areas; i++)
		{
			int				currentWave = event_areas_spawn_wave[i];
			int				maxWaveSpawns = G_MaxSpawnsPerWave(currentWave, event_areas_event_size[i]);
			int				currentCount = event_areas_spawn_count[i];
			qboolean		waitingForWave = event_areas_wave_filled[i];

			if (!waitingForWave && currentWave >= 0 && G_EnabledFactionEvent(i) && currentCount < leastSpawnsCount && currentCount < maxWaveSpawns)
			{
				leastSpawnsEvent = i;
				leastSpawnsCount = event_areas_spawn_count[i];
			}
		}
	}

	return leastSpawnsEvent;
}

team_t G_GetFactionForEvent(int eventNum)
{
	if (!EVENTS_ENABLED || num_event_areas <= 0 || eventNum < 0)
	{
		return FACTION_SPECTATOR;
	}

	return event_areas_current_team[eventNum];
}

qboolean WildlifeSpawnpointTooCloseToEvent(vec3_t pos)
{
	float eventBuffer = max(DISTANCE_BETWEEN_EVENTS * 0.5, 8192.0);

	for (int i = 0; i < num_event_areas; i++)
	{
		if (DistanceHorizontal(pos, event_areas[i]) < eventBuffer)
		{
			return qtrue;
		}
	}

	return qfalse;
}

void FindRandomWildlifeSpawnpoint(vec3_t point)
{
	int tries = 0; // Can't let it hang, if the whole map happens to be underwater....

	NavlibFindRandomPointOnMesh(NULL, point);

	while ((point[2] <= MAP_WATER_LEVEL || WildlifeSpawnpointTooCloseToEvent(point)) && tries < 10)
	{
		NavlibFindRandomPointOnMesh(NULL, point);
		tries++;
	}
}

void FindRandomEventSpawnpoint(int eventArea, vec3_t point)
{
	if (!EVENTS_ENABLED)
	{// Events are not enabled for this map...
		FindRandomNavmeshSpawnpoint(NULL, point);
		return;
	}

	if (num_event_areas <= 0)
	{// No event areas? Use navmesh random spawnpoints...
		FindRandomNavmeshSpawnpoint(NULL, point);
		return;
	}

	team_t team = FACTION_WILDLIFE;

	if (eventArea >= 0)
	{
		team = G_GetFactionForEvent(eventArea);
	}

	if (team == FACTION_WILDLIFE)
	{// Spawn outside of event areas so they don't interract with the NPCs in them, and for random enemy encounters heading to/from events...
		FindRandomWildlifeSpawnpoint(point);
		return;
	}

	int tries = 0; // Can't let it hang, if the whole map happens to be underwater....

	vec3_t event_position;

#if 0
	int numPossibleAreas = 0;
	int possibleAreas[MAX_TEAM_EVENT_AREAS] = { { -1 } };

	// Find a list of valid possible event areas for this team...
	for (int i = 0; i < num_event_areas; i++)
	{
		if (event_areas_current_team[i] == team)
		{
			possibleAreas[numPossibleAreas] = i;
			numPossibleAreas++;
		}
	}

	// Select an area at random... (TODO: sort and weight?)
	int bestArea = possibleAreas[irand(0, numPossibleAreas - 1)];
#else
	int bestArea = eventArea;
#endif
	VectorCopy(event_areas[bestArea], event_position);

	//trap->Print("EVENT: area %i selected for team %s spawn.\n", bestArea, TeamName(event_areas_current_team[bestArea]));

	// Spawn a ship above this 'in use' event area...
	G_CreateSpawnVesselForEventArea(bestArea);

#if defined(__USE_NAVLIB__) || defined(__USE_NAVLIB_SPAWNPOINTS__)
	if (G_NavmeshIsLoaded())
	{
		VectorSet(point, 0.0, 0.0, 0.0);

#pragma omp critical
		{
			FindRandomNavmeshPointInRadius(-1, event_position, point, 2048.0);
		}

		while (point[2] <= MAP_WATER_LEVEL && tries < 10)
		{
#pragma omp critical
			{
				FindRandomNavmeshPointInRadius(-1, event_position, point, 2048.0);
			}
			tries++;
		}
	}
	else
#endif //defined(__USE_NAVLIB__) || defined(__USE_NAVLIB_SPAWNPOINTS__)
	{
		if (gWPNum > 0)
		{// meh, this won't work. use old system for now...
			int wp = irand(0, gWPNum - 1);
			VectorCopy(gWPArray[wp]->origin, point);
		}
		else
		{// omg basejka bs...
			gentity_t *spawn = SelectSpawnPoint(vec3_origin, vec3_origin, vec3_origin, team, qfalse);
			VectorCopy(spawn->s.origin, point);
		}
	}
}
