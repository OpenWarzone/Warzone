#include "g_local.h"
#include "g_nav.h"
#include "ai_dominance_main.h"
#include "b_local.h"
#include <thread>

//
// Defines...
//

//#define __USE_ALL_IMPERIAL_SHIPS__					// Enables the victory Star Destroyer. It has too many textures and slows loading and renderring more than the others, so i'm disabling it for now... *FIXED*
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
qboolean				event_areas_disabled[MAX_TEAM_EVENT_AREAS] = { { qfalse } };
team_t					event_areas_current_team[MAX_TEAM_EVENT_AREAS] = { { FACTION_SPECTATOR } };
eventSize_t				event_areas_event_size[MAX_TEAM_EVENT_AREAS] = { { EVENT_SIZE_SMALL } };
int						event_areas_spawn_count[MAX_TEAM_EVENT_AREAS] = { { 0 } };
int						event_areas_spawn_wave[MAX_TEAM_EVENT_AREAS] = { { 0 } };
int						event_areas_spawn_quality[MAX_TEAM_EVENT_AREAS] = { { 0 } };
qboolean				event_areas_wave_filled[MAX_TEAM_EVENT_AREAS] = { { qfalse } };
gentity_t				*event_areas_ship[MAX_TEAM_EVENT_AREAS] = { { NULL } };
int						event_areas_ship_hyperspace_in_time[MAX_TEAM_EVENT_AREAS] = { { 0 } };
int						event_areas_ship_hyperspace_out_time[MAX_TEAM_EVENT_AREAS] = { { 0 } };

char					*miscModelString = "misc_model";

#define					SHIP_HYPERSPACE_TIME	10000
#define					SHIP_HYPERPACE_POW		16.0f

float ship_move_mix(float x, float y, float a)
{
	return x * (1.0 - a) + y * a;
}

void G_EventShipThink(gentity_t *ent)
{
	int area = ent->spawnArea;

	if (event_areas_ship_hyperspace_in_time[area] >= level.time)
	{
		// Mark as hyperspacing...
		ent->s.generic1 = 2;

		int time = event_areas_ship_hyperspace_in_time[area] - (level.time - 50);
		
		vec3_t pos;
		float alpha = Q_clamp(0.0, (float)time / (float)SHIP_HYPERSPACE_TIME, 1.0);
		alpha = 1.0 - pow(alpha, SHIP_HYPERPACE_POW);
		pos[0] = ship_move_mix(ent->movedir[0], ent->move_vector[0], alpha);
		pos[1] = ship_move_mix(ent->movedir[1], ent->move_vector[1], alpha);
		pos[2] = ent->move_vector[2];
		G_SetOrigin(ent, pos);

		//Com_Printf("Hyperspace in at %i %i %i. Target %i %i %i. alpha %f.\n", (int)pos[0], (int)pos[1], (int)pos[2], (int)ent->move_vector[0], (int)ent->move_vector[1], (int)ent->move_vector[2], alpha);

		ent->nextthink = level.time + 5;
	}
	else if (event_areas_ship_hyperspace_out_time[area] >= level.time)
	{
		// Mark as hyperspacing...
		ent->s.generic1 = 2;

		int time = event_areas_ship_hyperspace_out_time[area] - (level.time - 50);

		if (time <= 100)
		{// Just unspawn the ship and wait...
			if (event_areas_ship[area])
			{
				G_FreeEntity(event_areas_ship[area]);
				event_areas_ship[area] = NULL;
				event_areas_ship_hyperspace_out_time[area] = 0;
			}
		}
		else
		{
			vec3_t pos;
			float alpha = (float)time / (float)SHIP_HYPERSPACE_TIME;
			alpha = 1.0 - pow(1.0 - alpha, SHIP_HYPERPACE_POW);
			pos[0] = ship_move_mix(ent->movedir[0], ent->move_vector[0], alpha);
			pos[1] = ship_move_mix(ent->movedir[1], ent->move_vector[1], alpha);
			pos[2] = ent->move_vector[2];
			G_SetOrigin(ent, pos);

			//Com_Printf("Hyperspace out at %i %i %i. Target %i %i %i. alpha %f.\n", (int)pos[0], (int)pos[1], (int)pos[2], (int)ent->move_vector[0], (int)ent->move_vector[1], (int)ent->move_vector[2], alpha);
		}

		ent->nextthink = level.time + 5;
	}
	else
	{
		// Mark as bobbing...
		ent->s.generic1 = 1;

		ent->nextthink = level.time + 1000;
	}
}

void G_EventModelPrecache(void)
{
	if (!EVENTS_ENABLED) return;

	G_ModelIndex("models/warzone/ships/isd/isd.3ds");
	G_ModelIndex("models/warzone/ships/isd2/isd2.3ds");
	G_ModelIndex("models/warzone/ships/knight/knight.3ds");
#ifdef __USE_ALL_IMPERIAL_SHIPS__
	G_ModelIndex("models/warzone/ships/victory/victory.3ds");
	G_ModelIndex("models/warzone/ships/dominator/dominator.3ds"); // zfar issues
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
	if (event_areas_ship[area])
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
			int choice = irand(0, 2);
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
				sprintf(modelName, "models/warzone/ships/knight/knight.3ds");
				loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
				modelScale = 112;
				zOffset = 65536.0;
				event_areas_event_size[area] = EVENT_SIZE_LARGE;
			}
#ifdef __USE_ALL_IMPERIAL_SHIPS__
			else if (choice == 3)
			{
				sprintf(modelName, "models/warzone/ships/dominator/dominator.3ds");
				loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
				modelScale = 64;
				zOffset = 49152.0;
				event_areas_event_size[area] = EVENT_SIZE_MEDIUM;
			}
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
	ent->nextthink = level.time + 50;
	ent->think = G_EventShipThink;

	

	VectorCopy(vec3_origin, ent->s.angles);
	G_SetAngles(ent, ent->s.angles);

	VectorCopy(event_areas[area], ent->move_vector);
	ent->move_vector[2] += zOffset;

	vec3_t startPos, fwd;
	//AngleVectors(ent->s.angles, fwd, NULL, NULL);
	AngleVectors(ent->s.angles, NULL, fwd, NULL); // Because the models are rotated 90 deg lol...
	VectorNormalize(fwd);
	VectorMA(ent->move_vector, 1999999.0, fwd, startPos);
	G_SetOrigin(ent, startPos);
	VectorCopy(startPos, ent->movedir);

	ent->spawnArea = area;

	event_areas_ship_hyperspace_in_time[area] = level.time + SHIP_HYPERSPACE_TIME;

	trap->LinkEntity((sharedEntity_t *)ent);

	event_areas_ship[area] = ent;

	/*
	trap->Print("EVENT: Spawned ship %s for team %s above event area %i (position %f %f %f) at %f %f %f.\n", modelName, TeamName(event_areas_current_team[area]), area
		, event_areas[area][0], event_areas[area][1], event_areas[area][2]
		, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
	*/
}

void G_EventModelThink(gentity_t *ent)
{
	int area = ent->spawnArea;

	if (event_areas_ship_hyperspace_out_time[area] >= level.time)
	{// Event is over, unspawn...
		int time = event_areas_ship_hyperspace_out_time[area] - (level.time - 50);

		if (time <= 200)
		{// Disappear after 15 seconds...
			ent->think = G_FreeEntity;
			ent->nextthink = level.time + 15000;
		}
	}
	else
	{
		ent->nextthink = level.time + 1000;
	}
}

void G_CreateModelsForEventArea(int area)
{
#if 0 // hmm. nothing good to add atm...
	if (event_areas_ship[area])
	{// Already spawned the ship for this area...
		return;
	}

	team_t team = event_areas_current_team[area];

	if (team < FACTION_EMPIRE || team > FACTION_PIRATES)
	{// None...
		return;
	}

	char modelName[128] = { { 0 } };
	int modelFrame = 0;
	int modelScale = 1;
	int loopSound = 0;
	float zOffset = 8192.0;

	sprintf(modelName, "models/warzone/ships/isd/isd.3ds");
	//loopSound = G_SoundIndex("sound/vehicles/shuttle/loop.wav");
	modelScale = 56;
	zOffset = 32768.0;


	int numModels = 0;

	if (event_areas_event_size[area] == EVENT_SIZE_LARGE)
	{
		numModels = 8;
	}
	if (event_areas_event_size[area] == EVENT_SIZE_MEDIUM)
	{
		numModels = 4;
	}
	if (event_areas_event_size[area] == EVENT_SIZE_SMALL)
	{
		numModels = 2;
	}

	for (int i = 0; i < numModels; i++)
	{
		gentity_t *ent = G_Spawn();

		VectorSet(ent->r.mins, -512, -512, -512);
		VectorSet(ent->r.maxs, 512, 512, 512);

		ent->s.eType = ET_SERVERMODEL;
		ent->classname = miscModelString;
		ent->model = modelName;
		ent->s.frame = modelFrame;

		// Mark as not bobbing...
		ent->s.generic1 = 0;

		ent->s.modelindex = G_ModelIndex(ent->model);
		ent->s.iModelScale = modelScale; // NOTE: EF_SERVERMODEL does NOT use / 100.0 on scale.

		// No culling on hoverring event ships...
		//ent->r.svFlags |= SVF_BROADCAST;

		ent->s.eFlags = 0;
		ent->r.contents = CONTENTS_SOLID;
		ent->clipmask = MASK_SOLID;
		ent->s.teamowner = team;
		ent->s.owner = ENTITYNUM_NONE;

		ent->s.loopSound = loopSound;

		// TODO: Come from sky and hover...
		ent->nextthink = level.time + 50;
		ent->think = G_EventModelThink;


		VectorCopy(vec3_origin, ent->s.angles);
		G_SetAngles(ent, ent->s.angles);

		VectorCopy(event_areas[area], ent->move_vector);
		ent->move_vector[2] += zOffset;

		vec3_t startPos, fwd;
		//AngleVectors(ent->s.angles, fwd, NULL, NULL);
		AngleVectors(ent->s.angles, NULL, fwd, NULL); // Because the models are rotated 90 deg lol...
		VectorNormalize(fwd);
		VectorMA(ent->move_vector, 1999999.0, fwd, startPos);
		G_SetOrigin(ent, startPos);
		VectorCopy(startPos, ent->movedir);

		ent->spawnArea = area;

		event_areas_ship_hyperspace_in_time[area] = level.time + SHIP_HYPERSPACE_TIME;

		trap->LinkEntity((sharedEntity_t *)ent);

		event_areas_ship[area] = ent;

		/*
		trap->Print("EVENT: Spawned ship %s for team %s above event area %i (position %f %f %f) at %f %f %f.\n", modelName, TeamName(event_areas_current_team[area]), area
		, event_areas[area][0], event_areas[area][1], event_areas[area][2]
		, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
		*/
	}
#endif
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
		event_areas_disabled[i] = qfalse;
		event_areas_event_size[i] = EVENT_SIZE_SMALL;
		event_areas_spawn_count[i] = 0;
		event_areas_spawn_wave[i] = 0;
		event_areas_spawn_quality[i] = 0;
		event_areas_wave_filled[i] = qfalse;
		event_areas_ship[i] = NULL;
		event_areas_ship_hyperspace_in_time[i] = 0;
		event_areas_ship_hyperspace_out_time[i] = 0;

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

		// Check validity of each of the points, for older files that didn't check for water below...
		qboolean bad = qfalse;

		if (event_areas[i][2] <= MAP_WATER_LEVEL + 32.0)
		{
			bad = qtrue;
		}

		if (!bad)
		{
			if (trap->PointContents(event_areas[i], -1) & CONTENTS_WATER)
			{
				bad = qtrue;
			}
		}

		if (bad)
		{
			trap->Print("^1*** ^3%s^5: Disabled event area ^7%i^5 of old version eventAreas file because it is above water.\n", "EVENTS", i);
			event_areas_disabled[i] = qtrue;
			event_areas_current_team[i] = FACTION_SPECTATOR;
		}
		else
		{
			event_areas_disabled[i] = qfalse;
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
				event_areas_disabled[i] = qfalse;
				event_areas_event_size[i] = EVENT_SIZE_SMALL;
				event_areas_spawn_count[i] = 0;
				event_areas_spawn_wave[i] = 0;
				event_areas_spawn_quality[i] = 0;
				event_areas_wave_filled[i] = qfalse;
				event_areas_ship[i] = NULL;
				event_areas_ship_hyperspace_in_time[i] = 0;
				event_areas_ship_hyperspace_out_time[i] = 0;

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

#pragma omp parallel for schedule(dynamic)
				for (int attempt = 0; attempt < 4096; attempt++)
				//while (num_event_areas < MAX_TEAM_EVENT_AREAS && tries < MAX_TEAM_EVENT_AREA_TRIES)
				{
					if (!(num_event_areas < MAX_TEAM_EVENT_AREAS))
						continue;

					qboolean bad = qfalse;
					vec3_t org;
					FindRandomNavmeshSpawnpoint(NULL, org);

					extern void G_FindSky(vec3_t org);
					G_FindSky(org);
					//trap->Print("sky at %f %f %f.\n", org[0], org[1], org[2]);
					if (VectorLength(org) == 0) 
						bad = qtrue;

					if (!bad)
					{
						extern void G_FindGround(vec3_t org);

						G_FindGround(org);
						//trap->Print("ground at %f %f %f.\n", org[0], org[1], org[2]);
						if (VectorLength(org) == 0) 
							bad = qtrue;
					}

					if (org[2] <= MAP_WATER_LEVEL + 32.0)
					{
						bad = qtrue;
					}
					
					if (!bad)
					{
						for (int i = 0; i < num_event_areas; i++)
						{
							if (Distance(org, event_areas[i]) < DISTANCE_BETWEEN_EVENTS)
							{
								bad = qtrue;
								break;
							}
						}
					}

					if (!bad)
					{
						if (trap->PointContents(org, -1) & CONTENTS_WATER)
						{
							bad = qtrue;
						}
					}

					if (!bad)
					{
						if (!HasSkyVisibility(org))
						{
#pragma omp critical(__SHOW_PROGRESS__)
							{
								trap->Print("EVENT AREA GENERATOR: Attempt %i of %i. %f %f %f failed trace. %i areas found so far.\n", tries, MAX_TEAM_EVENT_AREA_TRIES, org[0], org[1], org[2], num_event_areas);
							}
							continue;
						}
						else
						{
#pragma omp critical(__SHOW_PROGRESS__)
							{
								trap->Print("EVENT AREA GENERATOR: Attempt %i of %i. %f %f %f SUCCEEDED trace. %i areas found so far.\n", tries, MAX_TEAM_EVENT_AREA_TRIES, org[0], org[1], org[2], num_event_areas + 1);
							}
						}

#pragma omp critical(__ADD_EVENT_AREA__)
						{
							VectorCopy(org, event_areas[num_event_areas]);
							num_event_areas++;
						}
					}
					else
					{
#pragma omp critical(__SHOW_PROGRESS__)
						{
							trap->Print("EVENT AREA GENERATOR: Attempt %i of %i. %f %f %f failed position. %i areas found so far.\n", tries, MAX_TEAM_EVENT_AREA_TRIES, org[0], org[1], org[2], num_event_areas);
						}
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

			if (num_event_areas > 0)
			{
				G_SaveEventAreas();
				trap->Print("^1*** ^3%s^5: Generated ^3%i ^5event areas for this map.\n", "EVENTS", num_event_areas);
			}
		}
		else
		{
			trap->Print("^1*** ^3%s^5: Loaded ^3%i ^5event areas for this map.\n", "EVENTS", num_event_areas);
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
	memset(event_areas_disabled, qfalse, sizeof(event_areas_disabled));
	memset(event_areas_current_team, 0, sizeof(event_areas_current_team));
	memset(event_areas_event_size, EVENT_SIZE_SMALL, sizeof(event_areas_event_size));
	memset(event_areas_spawn_count, 0, sizeof(event_areas_spawn_count));
	memset(event_areas_spawn_wave, 0, sizeof(event_areas_spawn_wave));
	memset(event_areas_spawn_quality, 0, sizeof(event_areas_spawn_quality));
	memset(event_areas_wave_filled, qfalse, sizeof(event_areas_wave_filled));
	memset(event_areas_ship, NULL, sizeof(event_areas_ship));
	memset(event_areas_ship_hyperspace_in_time, 0, sizeof(event_areas_ship_hyperspace_in_time));
	memset(event_areas_ship_hyperspace_out_time, 0, sizeof(event_areas_ship_hyperspace_out_time));
	
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

void G_InitEventArea(int eventArea)
{
	event_areas_disabled[eventArea] = qfalse;
	event_areas_event_size[eventArea] = EVENT_SIZE_SMALL;
	event_areas_spawn_count[eventArea] = 0;
	event_areas_spawn_wave[eventArea] = 0;
	event_areas_spawn_quality[eventArea] = 0;
	event_areas_ship_hyperspace_in_time[eventArea] = 0;
	event_areas_wave_filled[eventArea] = qfalse;

	if (event_areas_ship[eventArea])
	{// Tell the ship to warp out, ready for the next event...
		vec3_t fwd;
		//AngleVectors(event_areas_ship[eventArea]->s.angles, fwd, NULL, NULL);
		AngleVectors(event_areas_ship[eventArea]->s.angles, NULL, fwd, NULL); // Because the models are rotated 90 deg lol...
		VectorNormalize(fwd);
		VectorMA(event_areas_ship[eventArea]->move_vector, -1999999.0, fwd, event_areas_ship[eventArea]->movedir);

		event_areas_ship_hyperspace_out_time[eventArea] = level.time + SHIP_HYPERSPACE_TIME;
	}
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
			return 8;// 4;
			break;
		case EVENT_SIZE_MEDIUM:
			return 16;// 8;
			break;
		case EVENT_SIZE_LARGE:
			return 24;// 12;
			break;
		}
		break;
	case 1:
		// More Trash...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 12;// 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 20;// 12;
			break;
		case EVENT_SIZE_LARGE:
			return 32;// 16;
			break;
		}
		break;
	case 2:
		// Common...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 12;// 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 20;// 12;
			break;
		case EVENT_SIZE_LARGE:
			return 32;// 16;
			break;
		}
		break;
	case 3:
		// Commanders...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 12;// 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 20;// 12;
			break;
		case EVENT_SIZE_LARGE:
			return 32;// 16;
			break;
		}
		break;
	case 4:
		// Elites...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 12;// 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 20;// 12;
			break;
		case EVENT_SIZE_LARGE:
			return 32;// 16;
			break;
		}
		break;
	case 5:
		// Boss...
		switch (eventSize)
		{
		case EVENT_SIZE_SMALL:
		default:
			return 12;// 8;
			break;
		case EVENT_SIZE_MEDIUM:
			return 20;// 12;
			break;
		case EVENT_SIZE_LARGE:
			return 32;// 16;
			break;
		}
		break;
	}
}

int G_MaxSpawnsForEventSize(eventSize_t eventSize)
{// NOTE: Must be in groups of 4, as this is how the spawnGroups system is set up... TODO: Varying event sizes...
	int maxSpawns = 4;

	for (int wave = 0; wave < 6; wave++)
	{
		int ms = G_MaxSpawnsPerWave(wave, eventSize);

		if (ms > maxSpawns)
		{
			maxSpawns = ms;
		}
	}

	return maxSpawns;
	//return 32; // Screw it, make sure there are always 32 (the max possible) slots free...
}

int G_MaxSpawnsInEvent(int eventNum)
{
	if (!EVENTS_ENABLED || eventNum < 0 || num_event_areas <= 0)
	{
		return 0;
	}

	return G_MaxSpawnsForEventSize(event_areas_event_size[eventNum]);
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
					G_InitEventArea(i);
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

			trap->Print("^5Area ^7%i^5 (faction ^7%s^5) (wave ^7%i^5) has ^7%i^5/^7%i^5 spawned NPCs.\n", i, TeamName(event_areas_current_team[i]), currentWave, currentCount, maxWaveSpawns);
		}
	}
}

extern vmCvar_t npc_imperials;
extern vmCvar_t npc_rebels;
extern vmCvar_t npc_mandalorians;
extern vmCvar_t npc_mercs;
extern vmCvar_t npc_pirates;
extern vmCvar_t npc_wildlife;

extern int			num_imperial_npcs, num_rebel_npcs, num_mandalorian_npcs, num_merc_npcs, num_pirate_npcs, num_wildlife_npcs;

qboolean G_EnabledFactionEvent(int eventNum)
{
	if (event_areas_disabled[eventNum])
	{
		return qfalse;
	}

	team_t		eventFaction = event_areas_current_team[eventNum];
	int			largestWave = G_MaxSpawnsForEventSize(event_areas_event_size[eventNum]);
	int			currentCount = event_areas_spawn_count[eventNum];

	if (G_WaveSpawnCountForEventWave(eventNum) - currentCount < 4)
	{
		return qfalse;
	}

	switch (eventFaction)
	{
	case FACTION_EMPIRE:
		trap->Cvar_Update(&npc_imperials);
		if (npc_imperials.integer)
		{
			if (currentCount <= 0)
			{
				if (num_imperial_npcs + largestWave > npc_imperials.integer)
				{// Not enough free NPC slots to start this event...
					return qfalse;
				}
			}

			return qtrue;
		}
		break;
	case FACTION_REBEL:
		trap->Cvar_Update(&npc_rebels);
		if (npc_rebels.integer)
		{
			if (currentCount <= 0)
			{
				if (num_rebel_npcs + largestWave > npc_rebels.integer)
				{// Not enough free NPC slots to start this event...
					return qfalse;
				}
			}

			return qtrue;
		}
		break;
	case FACTION_MANDALORIAN:
		trap->Cvar_Update(&npc_mandalorians);
		if (npc_mandalorians.integer)
		{
			if (currentCount <= 0)
			{
				if (num_mandalorian_npcs + largestWave > npc_mandalorians.integer)
				{// Not enough free NPC slots to start this event...
					return qfalse;
				}
			}

			return qtrue;
		}
		break;
	case FACTION_MERC:
		trap->Cvar_Update(&npc_mercs);
		if (npc_mercs.integer)
		{
			if (currentCount <= 0)
			{
				if (num_merc_npcs + largestWave > npc_mercs.integer)
				{// Not enough free NPC slots to start this event...
					return qfalse;
				}
			}

			return qtrue;
		}
		break;
	case FACTION_PIRATES:
		trap->Cvar_Update(&npc_pirates);
		if (npc_pirates.integer)
		{
			if (currentCount <= 0)
			{
				if (num_pirate_npcs + largestWave > npc_pirates.integer)
				{// Not enough free NPC slots to start this event...
					return qfalse;
				}
			}

			return qtrue;
		}
		break;
	case FACTION_WILDLIFE:
		trap->Cvar_Update(&npc_wildlife);
		if (npc_wildlife.integer)
		{
			if (currentCount <= 0)
			{
				if (num_wildlife_npcs + largestWave > npc_wildlife.integer)
				{// Not enough free NPC slots to start this event...
					return qfalse;
				}
			}

			return qtrue;
		}
		break;
	case FACTION_SPECTATOR:
	default:
		return qfalse;// qtrue;
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
			if (event_areas_disabled[i])
				continue;

			G_CountEventAreaSpawns();

			int				currentWave = event_areas_spawn_wave[i];
			int				maxWaveSpawns = G_MaxSpawnsPerWave(currentWave, event_areas_event_size[i]);
			int				largestWave = G_MaxSpawnsForEventSize(event_areas_event_size[i]);
			int				currentCount = event_areas_spawn_count[i];
			qboolean		waitingForWave = event_areas_wave_filled[i];
			qboolean		hyperspaceIn = (event_areas_ship[i] && event_areas_ship_hyperspace_in_time != 0 && event_areas_ship_hyperspace_in_time[i] >= level.time) ? qtrue : qfalse;
			qboolean		hyperspaceOut = (event_areas_ship[i] && event_areas_ship_hyperspace_out_time != 0 && event_areas_ship_hyperspace_out_time[i] >= level.time) ? qtrue : qfalse;

#if 1
			if (waitingForWave || currentCount + 4 > maxWaveSpawns)
			{
				continue;
			}

			if (!G_EnabledFactionEvent(i))
			{
				continue;
			}

			if (hyperspaceIn || hyperspaceOut)
			{
				return -1;
			}

			if (currentWave >= 0)
			{
				//Com_Printf("Most needed %i. has %i of %i spawns.\n", i, currentCount, maxWaveSpawns);
				return i;
			}
#else
			// UQ1: Starts up unfillable events when npc_imperials etc is below the total needed on a map...
			if (!waitingForWave && !hyperspaceIn && !hyperspaceOut && currentWave >= 0 && G_EnabledFactionEvent(i) && currentCount < leastSpawnsCount && currentCount < maxWaveSpawns)
			{
				leastSpawnsEvent = i;
				leastSpawnsCount = event_areas_spawn_count[i];
			}
#endif
		}
	}

	return leastSpawnsEvent;
}

team_t G_GetFactionForEvent(int eventNum)
{
	if (!EVENTS_ENABLED || num_event_areas <= 0 || eventNum < 0 || event_areas_disabled[eventNum])
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

extern void G_FindSky(vec3_t org);
extern void G_FindGround(vec3_t org);

void FindRandomWildlifeSpawnpoint(vec3_t point)
{
	int tries = 0; // Can't let it hang, if the whole map happens to be underwater....

	NavlibFindRandomPointOnMesh(NULL, point);

	bool found = qtrue;

	G_FindSky(point);
	if (VectorLength(point) == 0) found = false;

	if (found)
	{
		G_FindGround(point);
		if (VectorLength(point) == 0) found = false;
	}

	while ((!found || point[2] <= MAP_WATER_LEVEL || WildlifeSpawnpointTooCloseToEvent(point)) && tries < 10)
	{
		found = qtrue;

		NavlibFindRandomPointOnMesh(NULL, point);

		G_FindSky(point);
		if (VectorLength(point) == 0) found = false;

		if (found)
		{
			G_FindGround(point);
			if (VectorLength(point) == 0) found = false;
		}

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
	int bestArea = eventArea;
	VectorCopy(event_areas[bestArea], event_position);

	//trap->Print("EVENT: area %i selected for team %s spawn.\n", bestArea, TeamName(event_areas_current_team[bestArea]));

	// Spawn a ship above this 'in use' event area...
	G_CreateSpawnVesselForEventArea(bestArea);
	G_CreateModelsForEventArea(bestArea);

#if defined(__USE_NAVLIB__) || defined(__USE_NAVLIB_SPAWNPOINTS__)
	if (G_NavmeshIsLoaded())
	{
		VectorSet(point, 0.0, 0.0, 0.0);

#pragma omp critical
		{
			FindRandomNavmeshPointInRadius(-1, event_position, point, 1024.0/*2048.0*/);
		}

		bool found = qtrue;

		G_FindSky(point);
		if (VectorLength(point) == 0) found = false;

		if (found)
		{
			G_FindGround(point);
			if (VectorLength(point) == 0) found = false;
		}

		while ((!found || point[2] <= MAP_WATER_LEVEL) && tries < 10)
		{
			found = qtrue;

#pragma omp critical
			{
				FindRandomNavmeshPointInRadius(-1, event_position, point, 1024.0/*2048.0*/);
			}

			G_FindSky(point);
			if (VectorLength(point) == 0) found = false;

			if (found)
			{
				G_FindGround(point);
				if (VectorLength(point) == 0) found = false;
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

qboolean G_IsOutsideEventArea(gentity_t *NPC)
{
	if (NPC->spawnArea > 0 && Distance(NPC->r.currentOrigin, event_areas[NPC->spawnArea]) > EVENT_BUFFER * 0.5)
	{// It is outside of it's spawn area...
		return qtrue;
	}

	return qfalse;
}
