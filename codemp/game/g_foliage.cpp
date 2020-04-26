#include "../qcommon/q_shared.h"
#include "../game/g_local.h"
#include "../game/surfaceflags.h"

extern qboolean InFOV( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );

// =======================================================================================================================================
//
//                                                             Foliage Rendering...
//
// =======================================================================================================================================


#define			FOLIAGE_MAX_FOLIAGES 2097152


char		CURRENT_CLIMATE_OPTION[256] = { 0 };

#define		PLANT_SCALE_MULTIPLIER 1.0

float		TREE_SCALE_MULTIPLIER = 1.0;

qboolean	FOLIAGE_LOADED = qfalse;
int			FOLIAGE_NUM_POSITIONS = 0;
vec3_t		*FOLIAGE_POSITIONS = NULL;
int			*FOLIAGE_TREE_SELECTION = NULL;
float		*FOLIAGE_TREE_SCALE = NULL;

#define		FOLIAGE_AREA_SIZE				512
#define		FOLIAGE_VISIBLE_DISTANCE		(FOLIAGE_AREA_SIZE*2.5)
#define		FOLIAGE_TREE_VISIBLE_DISTANCE	(FOLIAGE_AREA_SIZE*70.0)

#define		FOLIAGE_AREA_MAX				131072
#define		FOLIAGE_AREA_MAX_FOLIAGES		256

typedef int		ivec256_t[FOLIAGE_AREA_MAX_FOLIAGES];

int			FOLIAGE_AREAS_COUNT = 0;
int			*FOLIAGE_AREAS_LIST_COUNT = NULL;
ivec256_t	*FOLIAGE_AREAS_LIST = NULL;
vec3_t		*FOLIAGE_AREAS_MINS = NULL;
vec3_t		*FOLIAGE_AREAS_MAXS = NULL;

float		FOLIAGE_TREE_RADIUS[69] = { 0 };
float		FOLIAGE_TREE_BILLBOARD_SIZE[69] = { 0 };

int IN_RANGE_AREAS_LIST_COUNT = 0;
int IN_RANGE_AREAS_LIST[1024];
int IN_RANGE_TREE_AREAS_LIST_COUNT = 0;
int IN_RANGE_TREE_AREAS_LIST[8192];

qboolean FOLIAGE_In_Bounds( int areaNum, int foliageNum )
{
	if (foliageNum >= FOLIAGE_NUM_POSITIONS) return qfalse;

	if (FOLIAGE_AREAS_MINS[areaNum][0] < FOLIAGE_POSITIONS[foliageNum][0]
	&& FOLIAGE_AREAS_MINS[areaNum][1] < FOLIAGE_POSITIONS[foliageNum][1]
	&& FOLIAGE_AREAS_MAXS[areaNum][0] >= FOLIAGE_POSITIONS[foliageNum][0]
	&& FOLIAGE_AREAS_MAXS[areaNum][1] >= FOLIAGE_POSITIONS[foliageNum][1])
	{
		return qtrue;
	}

	return qfalse;
}

void FOLIAGE_Setup_Foliage_Areas( void )
{
	int		areaNum = 0, i = 0;
	vec3_t	mins, maxs, mapMins, mapMaxs;

	VectorSet(mapMins, 128000, 128000, 0);
	VectorSet(mapMaxs, -128000, -128000, 0);

	// Find map bounds first... Reduce area numbers...
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{
		if (FOLIAGE_POSITIONS[i][0] < mapMins[0])
			mapMins[0] = FOLIAGE_POSITIONS[i][0];

		if (FOLIAGE_POSITIONS[i][0] > mapMaxs[0])
			mapMaxs[0] = FOLIAGE_POSITIONS[i][0];

		if (FOLIAGE_POSITIONS[i][1] < mapMins[1])
			mapMins[1] = FOLIAGE_POSITIONS[i][1];

		if (FOLIAGE_POSITIONS[i][1] > mapMaxs[1])
			mapMaxs[1] = FOLIAGE_POSITIONS[i][1];
	}

	mapMins[0] -= 1024.0;
	mapMins[1] -= 1024.0;
	mapMaxs[0] += 1024.0;
	mapMaxs[1] += 1024.0;

	VectorSet(mins, mapMins[0], mapMins[1], 0);
	VectorSet(maxs, mapMins[0] + FOLIAGE_AREA_SIZE, mapMins[1] + FOLIAGE_AREA_SIZE, 0);

	FOLIAGE_AREAS_COUNT = 0;

	for (areaNum = 0; areaNum < FOLIAGE_AREA_MAX; areaNum++)
	{
		if (mins[1] > mapMaxs[1]) break; // found our last area...

		FOLIAGE_AREAS_LIST_COUNT[areaNum] = 0;

		while (FOLIAGE_AREAS_LIST_COUNT[areaNum] == 0 && mins[1] <= mapMaxs[1])
		{// While loop is so we can skip zero size areas for speed...
			VectorCopy(mins, FOLIAGE_AREAS_MINS[areaNum]);
			VectorCopy(maxs, FOLIAGE_AREAS_MAXS[areaNum]);

			// Assign foliages to the area lists...
			for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
			{
				if (FOLIAGE_In_Bounds(areaNum, i))
				{
					FOLIAGE_AREAS_LIST[areaNum][FOLIAGE_AREAS_LIST_COUNT[areaNum]] = i;
					FOLIAGE_AREAS_LIST_COUNT[areaNum]++;
				}
			}

			mins[0] += FOLIAGE_AREA_SIZE;
			maxs[0] = mins[0] + FOLIAGE_AREA_SIZE;

			if (mins[0] > mapMaxs[0])
			{
				mins[0] = mapMins[0];
				maxs[0] = mapMins[0] + FOLIAGE_AREA_SIZE;

				mins[1] += FOLIAGE_AREA_SIZE;
				maxs[1] = mins[1] + FOLIAGE_AREA_SIZE;
			}
		}
	}

	FOLIAGE_AREAS_COUNT = areaNum;

	trap->Print("^1*** ^3FOLIAGE-COLLISION^5: Generated ^7%i^5 foliage areas. ^7%i^5 total foliages...\n", FOLIAGE_AREAS_COUNT, FOLIAGE_NUM_POSITIONS);
}

int FOLIAGE_AreaNumForOrg( vec3_t moveOrg )
{
	int areaNum = 0;

	for (areaNum = 0; areaNum < FOLIAGE_AREAS_COUNT; areaNum++)
	{
		if (FOLIAGE_AREAS_MINS[areaNum][0] < moveOrg[0]
		&& FOLIAGE_AREAS_MINS[areaNum][1] < moveOrg[1]
		&& FOLIAGE_AREAS_MAXS[areaNum][0] >= moveOrg[0]
		&& FOLIAGE_AREAS_MAXS[areaNum][1] >= moveOrg[1])
		{
			return areaNum;
		}
	}

	return qfalse;
}

void FOLIAGE_SetupClosestAreas( gentity_t *ent, vec3_t moveOrg )
{
	qboolean areaChanged = qfalse;

	if (ent->CURRENT_AREA < 0 
		|| ent->CURRENT_AREA >= FOLIAGE_AREAS_COUNT
		|| !(FOLIAGE_AREAS_MINS[ent->CURRENT_AREA][0] < moveOrg[0] && FOLIAGE_AREAS_MINS[ent->CURRENT_AREA][1] < moveOrg[1] && FOLIAGE_AREAS_MAXS[ent->CURRENT_AREA][0] >= moveOrg[0] && FOLIAGE_AREAS_MAXS[ent->CURRENT_AREA][1] >= moveOrg[1]))
	{// If we have no valid CURRENT_AREA, or have move outside the current one, update...
		ent->CURRENT_AREA = FOLIAGE_AreaNumForOrg( ent->r.currentOrigin );
		areaChanged = qtrue;
	}

	if (areaChanged || DistanceHorizontal(ent->CLOSE_AREA_PREVIOUS_ORG, ent->r.currentOrigin) > 256)
	{// Only update when the entity is moving...
		int areaNum = 0;

		ent->CLOSE_AREA_LIST_COUNT = 0;
		VectorCopy(moveOrg, ent->CLOSE_AREA_PREVIOUS_ORG);

		for (areaNum = 0; areaNum < FOLIAGE_AREAS_COUNT; areaNum++)
		{
			float DIST = DistanceHorizontal(FOLIAGE_AREAS_MINS[areaNum], moveOrg);
			float DIST2 = DistanceHorizontal(FOLIAGE_AREAS_MAXS[areaNum], moveOrg);

			if (DIST < FOLIAGE_AREA_SIZE * 2.0 || DIST2 < FOLIAGE_AREA_SIZE * 2.0)
			{
				ent->CLOSE_AREA_LIST[ent->CLOSE_AREA_LIST_COUNT] = areaNum;
				ent->CLOSE_AREA_LIST_COUNT++;
			}
		}
	}
}

qboolean FOLIAGE_TreeSolidBlocking( gentity_t *ent, vec3_t moveOrg )
{
	int areaListPos = 0;

	if (ent->client && ent->client->sess.sessionTeam == FACTION_SPECTATOR) return qfalse;

	FOLIAGE_SetupClosestAreas( ent, moveOrg );

	for (areaListPos = 0; areaListPos < ent->CLOSE_AREA_LIST_COUNT; areaListPos++)
	{
		int treeNum = 0;
		int areaNum = ent->CLOSE_AREA_LIST[areaListPos];

		for (treeNum = 0; treeNum < FOLIAGE_AREAS_LIST_COUNT[areaNum]; treeNum++)
		{
			int		THIS_TREE_NUM = FOLIAGE_AREAS_LIST[areaNum][treeNum];
			int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[THIS_TREE_NUM]-1;
			float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[THIS_TREE_NUM]*TREE_SCALE_MULTIPLIER;
			float	TREE_HEIGHT = FOLIAGE_TREE_SCALE[THIS_TREE_NUM]*2.5*FOLIAGE_TREE_BILLBOARD_SIZE[FOLIAGE_TREE_SELECTION[THIS_TREE_NUM]-1]*TREE_SCALE_MULTIPLIER;
			float	DIST = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], moveOrg);
			float	hDist =  DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], ent->r.currentOrigin);

			if (DIST <= TREE_RADIUS 
				&& (moveOrg[2] >= FOLIAGE_POSITIONS[THIS_TREE_NUM][2] && moveOrg[2] <= FOLIAGE_POSITIONS[THIS_TREE_NUM][2] + (TREE_HEIGHT*2.0))
				&& DIST < hDist 				// Move pos would be closer
				&& hDist > TREE_RADIUS)			// Not already stuck in tree...
			{
				//trap->Print("SERVER: Blocked by tree %i. Radius %f. Distance %f. Type %i. AreaNum %i\n", THIS_TREE_NUM, TREE_RADIUS, DIST, THIS_TREE_TYPE, areaNum);
				return qtrue;
			}
		}
	}

	return qfalse;
}

extern int BG_SiegeGetPairedValue(char *buf, char *key, char *outbuf);

qboolean FOLIAGE_LoadMapClimateInfo( void )
{
	vmCvar_t		mapname;
	const char		*climateName = NULL;

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_ROM | CVAR_SERVERINFO );

	climateName = IniRead(va("foliage/%s.climateInfo", mapname.string), "CLIMATE", "CLIMATE_TYPE", "");

	memset(CURRENT_CLIMATE_OPTION, 0, sizeof(CURRENT_CLIMATE_OPTION));
	strncpy(CURRENT_CLIMATE_OPTION, climateName, strlen(climateName));

	if (CURRENT_CLIMATE_OPTION[0] == '\0')
	{
		trap->Print( "^1*** ^3%s^5: No map climate info file ^7foliage/%s.climateInfo^5. Using default climate option.\n", "FOLIAGE-COLLISION", mapname.string );
		strncpy(CURRENT_CLIMATE_OPTION, "tropical", strlen("tropical"));
		return qfalse;
	}

	trap->Print( "^1*** ^3%s^5: Successfully loaded climateInfo file ^7foliage/%s.climateInfo^5. Using ^3%s^5 climate option.\n", "FOLIAGE-COLLISION", mapname.string, CURRENT_CLIMATE_OPTION );

	return qtrue;
}

void FOLIAGE_FreeMemory(void)
{
	if (FOLIAGE_POSITIONS)
	{
		free(FOLIAGE_POSITIONS);
		free(FOLIAGE_TREE_SELECTION);
		free(FOLIAGE_TREE_SCALE);

		FOLIAGE_POSITIONS = NULL;
		FOLIAGE_TREE_SELECTION = NULL;
		FOLIAGE_TREE_SCALE = NULL;
	}
}

#define		MAX_PLANT_MODELS 69

qboolean	USING_CUSTOM_FOLIAGE = qfalse;
float		CUSTOM_FOLIAGE_MAX_DISTANCE = 0.0;
char		CustomFoliageModelsList[69][128] = { 0 };
float		CUSTOM_FOLIAGE_SCALES[69] = { 0.0 };
float		CUSTOM_FOLIAGE_COLLISION_RADIUS[69] = { 0.0 };
float		CUSTOM_FOLIAGE_COLLISION_HEIGHT[69] = { 0.0 };

qboolean FOLIAGE_LoadFoliagePositions( void )
{
	fileHandle_t	f;
	int				i = 0;
	int				fileCount = 0;
	int				treeCount = 0;
	vmCvar_t		mapname;

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_ROM | CVAR_SERVERINFO );

	trap->FS_Open( va( "foliage/%s.foliage", mapname.string), &f, FS_READ );

	if ( !f )
	{
		if (FOLIAGE_AREAS_LIST_COUNT) free(FOLIAGE_AREAS_LIST_COUNT);
		if (FOLIAGE_AREAS_LIST) free(FOLIAGE_AREAS_LIST);
		if (FOLIAGE_AREAS_MINS) free(FOLIAGE_AREAS_MINS);
		if (FOLIAGE_AREAS_MAXS) free(FOLIAGE_AREAS_MAXS);
		return qfalse;
	}

	trap->FS_Read( &fileCount, sizeof(int), f );

	FOLIAGE_FreeMemory();
	
	// Alloc max possible needed memory...
	FOLIAGE_POSITIONS = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
	FOLIAGE_TREE_SELECTION = (int *)malloc(fileCount * sizeof(int));
	FOLIAGE_TREE_SCALE = (float *)malloc(fileCount * sizeof(float));

	char FOLIAGE_MODEL_SELECTION[128] = { 0 };

	strcpy(FOLIAGE_MODEL_SELECTION, IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "FOLIAGE", "foliageSet", "default"));

	if (FOLIAGE_MODEL_SELECTION[0] == '\0' || !strcmp(FOLIAGE_MODEL_SELECTION, "default"))
	{// If it returned default value, check also in mapinfo file...
		memset(FOLIAGE_MODEL_SELECTION, 0, sizeof(char) * 128);
		strcpy(FOLIAGE_MODEL_SELECTION, IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", "FOLIAGE_SET", "default"));
	}

	if (!Q_stricmp(FOLIAGE_MODEL_SELECTION, "custom"))
	{
		//USING_CUSTOM = qtrue;

		//
		//
		//

		trap->Print("^1*** ^3%s^5: Map \"^7%s^5\" foliage selection using \"^7custom^5\" foliage set.\n", "FOLIAGE-COLLISION", mapname.string);

		int		customRealNormalTempModelsAdded = 0;
		int		customRealRareTempModelsAdded = 0;
		char	customNormalFoliageModelsTempList[69][128] = { 0 };
		char	customRareFoliageModelsTempList[69][128] = { 0 };
		float	customNormalFoliageModelsTempScalesList[69] = { 1.0 };
		float	customRareFoliageModelsTempScalesList[69] = { 1.0 };
		float	customNormalFoliageModelsTempCollisionRadiusList[69] = { 0.0 };
		float	customRareFoliageModelsTempCollisionRadiusList[69] = { 0.0 };
		float	customNormalFoliageModelsTempCollisionHeightList[69] = { 0.0 };
		float	customRareFoliageModelsTempCollisionHeightList[69] = { 0.0 };

#define		ANY_AREA_MODEL_POSITION		46
#define		RARE_MODELS_START			47

		strcpy(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION], IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", "anyAreaFoliageModel", ""));
		CUSTOM_FOLIAGE_SCALES[ANY_AREA_MODEL_POSITION] = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", "anyAreaFoliageModelScale", "1.0"));
		CUSTOM_FOLIAGE_COLLISION_RADIUS[ANY_AREA_MODEL_POSITION] = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", "anyAreaFoliageModelCollisionRadius", "0.0"));
		CUSTOM_FOLIAGE_COLLISION_HEIGHT[ANY_AREA_MODEL_POSITION] = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", "anyAreaFoliageModelCollisionHeight", va("%f", CUSTOM_FOLIAGE_COLLISION_RADIUS[ANY_AREA_MODEL_POSITION])));

		for (i = 0; i < ANY_AREA_MODEL_POSITION; i++)
		{
			char temp[128] = { 0 };
			strcpy(temp, IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("openAreaFoliageModel%i", i), ""));
			float customScale = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("openAreaFoliageModelScale%i", i), "1.0"));
			float customCollisionRadius = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("openAreaFoliageModelCollisionRadius%i", i), "0.0"));
			float customCollisionHeight = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("openAreaFoliageModelCollisionHeight%i", i), va("%f", customCollisionRadius)));

			if (temp && temp[0] != 0 && strlen(temp) > 0)
			{// Exists...
			 // Add it to temp list, so we can use randoms from it if they don't add a full list...
				strcpy(customNormalFoliageModelsTempList[customRealNormalTempModelsAdded], temp);
				customNormalFoliageModelsTempScalesList[customRealNormalTempModelsAdded] = customScale;
				customNormalFoliageModelsTempCollisionRadiusList[customRealNormalTempModelsAdded] = customCollisionRadius;
				customNormalFoliageModelsTempCollisionHeightList[customRealNormalTempModelsAdded] = customCollisionHeight;
				customRealNormalTempModelsAdded++;
			}
		}

		int upto = 0;
		for (i = RARE_MODELS_START; i < MAX_PLANT_MODELS; i++, upto++)
		{
			char temp[128] = { 0 };
			strcpy(temp, IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("rareFoliageModel%i", upto), ""));
			float customScale = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("rareFoliageModelScale%i", upto), "1.0"));
			float customCollisionRadius = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("rareFoliageModelCollisionRadius%i", upto), "0.0"));
			float customCollisionHeight = atof(IniRead(va("maps/%s.mapInfo", mapname.string), "FOLIAGE", va("rareFoliageModelCollisionHeight%i", upto), va("%f", customCollisionRadius)));

			if (temp && temp[0] != 0 && strlen(temp) > 0)
			{// Exists...
			 // Add it to temp list, so we can use randoms from it if they don't add a full list...
				strcpy(customRareFoliageModelsTempList[customRealRareTempModelsAdded], temp);
				customRareFoliageModelsTempScalesList[customRealRareTempModelsAdded] = customScale;
				customRareFoliageModelsTempCollisionRadiusList[customRealRareTempModelsAdded] = customCollisionRadius;
				customRareFoliageModelsTempCollisionHeightList[customRealRareTempModelsAdded] = customCollisionHeight;
				customRealRareTempModelsAdded++;
			}
		}

		// Fill the any area model, if required...
		if (!(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION] && CustomFoliageModelsList[ANY_AREA_MODEL_POSITION][0] != 0 && strlen(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION]) > 0))
		{// Sanity check, fallback to using either a rare or a normal model for the anyArea model, if there was none specified...
			if (customRealRareTempModelsAdded > 0)
			{// Have rares? Use one...
				strcpy(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION], customRareFoliageModelsTempList[0]);
				CUSTOM_FOLIAGE_SCALES[ANY_AREA_MODEL_POSITION] = customRareFoliageModelsTempScalesList[0];
				CUSTOM_FOLIAGE_COLLISION_RADIUS[ANY_AREA_MODEL_POSITION] = customRareFoliageModelsTempCollisionRadiusList[0];
				CUSTOM_FOLIAGE_COLLISION_HEIGHT[ANY_AREA_MODEL_POSITION] = customRareFoliageModelsTempCollisionHeightList[0];
			}

			if (customRealNormalTempModelsAdded > 0)
			{// Have standards? Use one...
				strcpy(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION], customNormalFoliageModelsTempList[0]);
				CUSTOM_FOLIAGE_SCALES[ANY_AREA_MODEL_POSITION] = customNormalFoliageModelsTempScalesList[0];
				CUSTOM_FOLIAGE_COLLISION_RADIUS[ANY_AREA_MODEL_POSITION] = customNormalFoliageModelsTempCollisionRadiusList[0];
				CUSTOM_FOLIAGE_COLLISION_HEIGHT[ANY_AREA_MODEL_POSITION] = customNormalFoliageModelsTempCollisionHeightList[0];
			}
		}

		//
		// Fill the final list, from the temp lists...
		//
		if (customRealRareTempModelsAdded > 0 && customRealNormalTempModelsAdded > 0)
		{
			// Fill Standards...
			upto = 0;

			for (i = 0; i < ANY_AREA_MODEL_POSITION; i++)
			{
				if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
				{
					if (upto >= customRealRareTempModelsAdded - 1)
					{
						upto = 0;
					}
					else
					{
						upto++;
					}

					strcpy(CustomFoliageModelsList[i], customRareFoliageModelsTempList[upto]);
					CUSTOM_FOLIAGE_SCALES[i] = customRareFoliageModelsTempScalesList[upto];
					CUSTOM_FOLIAGE_COLLISION_RADIUS[i] = customRareFoliageModelsTempCollisionRadiusList[upto];
					CUSTOM_FOLIAGE_COLLISION_HEIGHT[i] = customRareFoliageModelsTempCollisionHeightList[upto];
				}
			}

			// Fill Rares...
			upto = 0;

			for (i = RARE_MODELS_START; i < MAX_PLANT_MODELS; i++)
			{
				if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
				{
					if (upto >= customRealNormalTempModelsAdded - 1)
					{
						upto = 0;
					}
					else
					{
						upto++;
					}

					strcpy(CustomFoliageModelsList[i], customNormalFoliageModelsTempList[upto]);
					CUSTOM_FOLIAGE_SCALES[i] = customNormalFoliageModelsTempScalesList[upto];
					CUSTOM_FOLIAGE_COLLISION_RADIUS[i] = customNormalFoliageModelsTempCollisionRadiusList[upto];
					CUSTOM_FOLIAGE_COLLISION_HEIGHT[i] = customNormalFoliageModelsTempCollisionHeightList[upto];
				}
			}
		}
		else if (customRealNormalTempModelsAdded > 0)
		{// Fill All...
			upto = 0;

			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
				{
					if (upto >= customRealNormalTempModelsAdded - 1)
					{
						upto = 0;
					}
					else
					{
						upto++;
					}

					strcpy(CustomFoliageModelsList[i], customNormalFoliageModelsTempList[upto]);
					CUSTOM_FOLIAGE_SCALES[i] = customNormalFoliageModelsTempScalesList[upto];
					CUSTOM_FOLIAGE_COLLISION_RADIUS[i] = customNormalFoliageModelsTempCollisionRadiusList[upto];
					CUSTOM_FOLIAGE_COLLISION_HEIGHT[i] = customNormalFoliageModelsTempCollisionHeightList[upto];
				}
			}
		}
		else if (customRealRareTempModelsAdded > 0)
		{// Fill All...
			upto = 0;

			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
				{
					if (upto >= customRealRareTempModelsAdded - 1)
					{
						upto = 0;
					}
					else
					{
						upto++;
					}

					strcpy(CustomFoliageModelsList[i], customRareFoliageModelsTempList[upto]);
					CUSTOM_FOLIAGE_SCALES[i] = customRareFoliageModelsTempScalesList[upto];
					CUSTOM_FOLIAGE_COLLISION_RADIUS[i] = customRareFoliageModelsTempCollisionRadiusList[upto];
					CUSTOM_FOLIAGE_COLLISION_HEIGHT[i] = customRareFoliageModelsTempCollisionHeightList[upto];
				}
			}
		}
		else if (CustomFoliageModelsList[ANY_AREA_MODEL_POSITION] && CustomFoliageModelsList[ANY_AREA_MODEL_POSITION][0] != 0 && strlen(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION]) > 0)
		{// Had no rares or normal foliages, but we did have an anyArea model, use that for everything...
			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
				{
					strcpy(CustomFoliageModelsList[i], CustomFoliageModelsList[ANY_AREA_MODEL_POSITION]);
					CUSTOM_FOLIAGE_SCALES[i] = CUSTOM_FOLIAGE_SCALES[ANY_AREA_MODEL_POSITION];
					CUSTOM_FOLIAGE_COLLISION_RADIUS[i] = CUSTOM_FOLIAGE_COLLISION_RADIUS[ANY_AREA_MODEL_POSITION];
					CUSTOM_FOLIAGE_COLLISION_HEIGHT[i] = CUSTOM_FOLIAGE_COLLISION_HEIGHT[ANY_AREA_MODEL_POSITION];
				}
			}
		}
		else
		{// Had absolutely nothing??? Add standard green grasses everywhere...
			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
				{
					strcpy(CustomFoliageModelsList[i], "models/warzone/plants/gcgrass01.md3");
					CUSTOM_FOLIAGE_SCALES[i] = 1.0;
					CUSTOM_FOLIAGE_COLLISION_RADIUS[i] = 0.0;
					CUSTOM_FOLIAGE_COLLISION_HEIGHT[i] = 0.0;
				}
			}
		}

		for (i = 0; i < MAX_PLANT_MODELS; i++)
		{
			//trap->Print("^1*** ^3%s^5: Foliage: %i. Model: %s. Radius: %f.\n", "FOLIAGE-COLLISION", i, CustomFoliageModelsList[i], CUSTOM_FOLIAGE_COLLISION_RADIUS[i]);

			if (CustomFoliageModelsList[i] && CustomFoliageModelsList[i][0] != 0 && strlen(CustomFoliageModelsList[i]) > 0 && CUSTOM_FOLIAGE_COLLISION_RADIUS[i] > 0.0 && CUSTOM_FOLIAGE_COLLISION_HEIGHT[i] > 0.0)
			{// Have at least one foliage set to be solid...
				USING_CUSTOM_FOLIAGE = qtrue;
				//trap->Print("^1*** ^3%s^5: USING_CUSTOM_FOLIAGE.\n", "FOLIAGE-COLLISION");
				break;
			}
		}
	}

	for (i = 0; i < fileCount; i++)
	{
		vec3_t	unneededVec3;
		int		unneededInt;
		float	unneededFloat;

		if (USING_CUSTOM_FOLIAGE)
		{// Plants become trees...
			trap->FS_Read(&FOLIAGE_POSITIONS[treeCount], sizeof(vec3_t), f);
			trap->FS_Read(&unneededVec3, sizeof(vec3_t), f);
			trap->FS_Read(&FOLIAGE_TREE_SELECTION[treeCount], sizeof(int), f); // actually plant selection slot
			trap->FS_Read(&unneededFloat, sizeof(float), f);
			trap->FS_Read(&FOLIAGE_TREE_SCALE[treeCount], sizeof(float), f); // actually plant scale slot
			trap->FS_Read(&unneededInt, sizeof(int), f);
			trap->FS_Read(&unneededFloat, sizeof(float), f);
			trap->FS_Read(&unneededFloat, sizeof(float), f);

			if (FOLIAGE_TREE_SELECTION[treeCount] > 0 && CUSTOM_FOLIAGE_COLLISION_RADIUS[FOLIAGE_TREE_SELECTION[treeCount]-1] > 0.0)
			{// Only keep positions with plants, and a colision radius set for it...
				FOLIAGE_TREE_RADIUS[treeCount] = CUSTOM_FOLIAGE_COLLISION_RADIUS[FOLIAGE_TREE_SELECTION[treeCount]-1];
				FOLIAGE_TREE_BILLBOARD_SIZE[treeCount] = CUSTOM_FOLIAGE_COLLISION_HEIGHT[FOLIAGE_TREE_SELECTION[treeCount]-1];
				FOLIAGE_TREE_SCALE[treeCount] = FOLIAGE_TREE_SCALE[treeCount] * CUSTOM_FOLIAGE_SCALES[FOLIAGE_TREE_SELECTION[treeCount] - 1];
				//trap->Print("^1*** ^3%s^5: Plant ^7%i^5 (type ^7%i^5) has a collision radius of ^7%f^5 (scaled ^7%f^5), a collision height of ^7%f^5 (scaled ^7%f^5), and a scale of ^7%f^5.\n", "FOLIAGE-COLLISION", treeCount, FOLIAGE_TREE_SELECTION[treeCount]-1, FOLIAGE_TREE_RADIUS[treeCount], FOLIAGE_TREE_RADIUS[treeCount]*FOLIAGE_TREE_SCALE[treeCount], FOLIAGE_TREE_BILLBOARD_SIZE[treeCount], FOLIAGE_TREE_BILLBOARD_SIZE[treeCount] * FOLIAGE_TREE_SCALE[treeCount], FOLIAGE_TREE_SCALE[treeCount]);
				treeCount++;
			}
		}
		else
		{
			trap->FS_Read(&FOLIAGE_POSITIONS[treeCount], sizeof(vec3_t), f);
			trap->FS_Read(&unneededVec3, sizeof(vec3_t), f);
			trap->FS_Read(&unneededInt, sizeof(int), f);
			trap->FS_Read(&unneededFloat, sizeof(float), f);
			trap->FS_Read(&unneededFloat, sizeof(float), f);
			trap->FS_Read(&FOLIAGE_TREE_SELECTION[treeCount], sizeof(int), f);
			trap->FS_Read(&unneededFloat, sizeof(float), f);
			trap->FS_Read(&FOLIAGE_TREE_SCALE[treeCount], sizeof(float), f);

			if (FOLIAGE_TREE_SELECTION[treeCount] > 0)
			{// Only keep positions with trees...
				treeCount++;
			}
		}
	}

	FOLIAGE_NUM_POSITIONS = treeCount;

	trap->FS_Close(f);

	if (FOLIAGE_NUM_POSITIONS > 0)
	{// Re-alloc to whats actually used...
		FOLIAGE_POSITIONS = (vec3_t *)realloc(FOLIAGE_POSITIONS, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_TREE_SELECTION = (int *)realloc(FOLIAGE_TREE_SELECTION, FOLIAGE_NUM_POSITIONS * sizeof(int));
		FOLIAGE_TREE_SCALE = (float *)realloc(FOLIAGE_TREE_SCALE, FOLIAGE_NUM_POSITIONS * sizeof(float));

		if (!FOLIAGE_AREAS_LIST_COUNT) FOLIAGE_AREAS_LIST_COUNT = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_LIST) FOLIAGE_AREAS_LIST = (ivec256_t *)malloc(sizeof(ivec256_t) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_MINS) FOLIAGE_AREAS_MINS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_MAXS) FOLIAGE_AREAS_MAXS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);
	}
	else
	{
		FOLIAGE_FreeMemory();

		if (FOLIAGE_AREAS_LIST_COUNT) free(FOLIAGE_AREAS_LIST_COUNT);
		if (FOLIAGE_AREAS_LIST) free(FOLIAGE_AREAS_LIST);
		if (FOLIAGE_AREAS_MINS) free(FOLIAGE_AREAS_MINS);
		if (FOLIAGE_AREAS_MAXS) free(FOLIAGE_AREAS_MAXS);

		FOLIAGE_AREAS_LIST_COUNT = NULL;
		FOLIAGE_AREAS_LIST = NULL;
		FOLIAGE_AREAS_MINS = NULL;
		FOLIAGE_AREAS_MAXS = NULL;

		trap->Print("^1*** ^3%s^5: No tree points in foliage file ^7foliage/%s.foliage^5. Memory freed.\n", "FOLIAGE-COLLISION", mapname.string);
		return qfalse;
	}

	trap->Print("^1*** ^3%s^5: Successfully loaded ^7%i^5 foliage points from foliage file ^7foliage/%s.foliage^5. Found ^7%i^5 collision objects.\n", "FOLIAGE-COLLISION",
		fileCount, mapname.string, FOLIAGE_NUM_POSITIONS);

	FOLIAGE_Setup_Foliage_Areas();

	return qtrue;
}

qboolean FOLIAGE_IgnoreFoliageOnMap( void )
{
	vmCvar_t		mapname;

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_ROM | CVAR_SERVERINFO );

	if (StringContainsWord(mapname.string, "eisley")
		|| StringContainsWord(mapname.string, "desert")
		|| StringContainsWord(mapname.string, "tatooine")
		|| StringContainsWord(mapname.string, "hoth")
		|| StringContainsWord(mapname.string, "mp/ctf1")
		|| StringContainsWord(mapname.string, "mp/ctf2")
		|| StringContainsWord(mapname.string, "mp/ctf4")
		|| StringContainsWord(mapname.string, "mp/ctf5")
		|| StringContainsWord(mapname.string, "mp/ffa1")
		|| StringContainsWord(mapname.string, "mp/ffa2")
		|| StringContainsWord(mapname.string, "mp/ffa3")
		|| StringContainsWord(mapname.string, "mp/ffa4")
		|| StringContainsWord(mapname.string, "mp/ffa5")
		|| StringContainsWord(mapname.string, "mp/duel1")
		|| StringContainsWord(mapname.string, "mp/duel2")
		|| StringContainsWord(mapname.string, "mp/duel3")
		|| StringContainsWord(mapname.string, "mp/duel4")
		|| StringContainsWord(mapname.string, "mp/duel5")
		|| StringContainsWord(mapname.string, "mp/duel7")
		|| StringContainsWord(mapname.string, "mp/duel9")
		|| StringContainsWord(mapname.string, "mp/duel10")
		|| StringContainsWord(mapname.string, "bespin_streets")
		|| StringContainsWord(mapname.string, "bespin_platform"))
	{// Ignore this map... We know we don't need grass here...
		return qtrue;
	}

	return qfalse;
}

void FOLIAGE_LoadTrees( void )
{
	int i = 0;

	trap->Print("^1*** ^3FOLIAGE-COLLISION^5: Loading Trees...\n");

	if (FOLIAGE_IgnoreFoliageOnMap())
	{// Ignore this map... We know we don't need grass here...
		FOLIAGE_NUM_POSITIONS = 0;
		FOLIAGE_LOADED = qtrue;
		trap->Print("^1*** ^3FOLIAGE-COLLISION^5: Trees are ignored on this map.\n");

		if (FOLIAGE_AREAS_LIST_COUNT) free(FOLIAGE_AREAS_LIST_COUNT);
		if (FOLIAGE_AREAS_LIST) free(FOLIAGE_AREAS_LIST);
		if (FOLIAGE_AREAS_MINS) free(FOLIAGE_AREAS_MINS);
		if (FOLIAGE_AREAS_MAXS) free(FOLIAGE_AREAS_MAXS);

		FOLIAGE_AREAS_LIST_COUNT = NULL;
		FOLIAGE_AREAS_LIST = NULL;
		FOLIAGE_AREAS_MINS = NULL;
		FOLIAGE_AREAS_MAXS = NULL;
		return;
	}

	if (!FOLIAGE_LOADED)
	{
		FOLIAGE_LoadMapClimateInfo();
		FOLIAGE_LoadFoliagePositions();
		FOLIAGE_LOADED = qtrue;
	}

	if (FOLIAGE_NUM_POSITIONS <= 0)
	{
		trap->Print("^1*** ^3FOLIAGE-COLLISION^5: No tree positions found for map.\n");

		if (FOLIAGE_AREAS_LIST_COUNT) free(FOLIAGE_AREAS_LIST_COUNT);
		if (FOLIAGE_AREAS_LIST) free(FOLIAGE_AREAS_LIST);
		if (FOLIAGE_AREAS_MINS) free(FOLIAGE_AREAS_MINS);
		if (FOLIAGE_AREAS_MAXS) free(FOLIAGE_AREAS_MAXS);

		FOLIAGE_AREAS_LIST_COUNT = NULL;
		FOLIAGE_AREAS_LIST = NULL;
		FOLIAGE_AREAS_MINS = NULL;
		FOLIAGE_AREAS_MAXS = NULL;
		return;
	}

	// Read all the tree info from the new .climate ini files...
	TREE_SCALE_MULTIPLIER = atof(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", "treeScaleMultiplier", "1.0"));

	if (USING_CUSTOM_FOLIAGE)
	{// Already should be set up...
		
	}
	else
	{
		for (i = 0; i < 9; i++)
		{
			FOLIAGE_TREE_BILLBOARD_SIZE[i] = atof(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", va("treeBillboardSize%i", i), "128.0"));
			FOLIAGE_TREE_RADIUS[i] = atof(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", va("treeRadius%i", i), "24.0"));
		}
	}

	/* // Hmm max of 64... this won't work :(
#ifdef __USE_NAVLIB__
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{
		if (FOLIAGE_TREE_SELECTION[i] > 0)
		{// Only keep positions with trees...
			int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[i] - 1;
			float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[i] * TREE_SCALE_MULTIPLIER;
			vec3_t mins, maxs;
			VectorSet(mins, -(TREE_RADIUS / 0.65), -(TREE_RADIUS / 0.65), 0.0);
			VectorSet(maxs, TREE_RADIUS / 0.65, TREE_RADIUS / 0.65, 512.0);
			NavlibAddObstacle(mins, maxs, (qhandle_t *)(4096 + i));
		}
	}
#endif //__USE_NAVLIB__
	*/

#ifdef __USE_NAVLIB__
	if (G_NavmeshIsLoaded())
	{// Block all the points on the navmesh...
		trap->Print("^1*** ^3FOLIAGE-COLLISION^5: Blocking collision objects on navmesh...\n");

		for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
		{
			if (FOLIAGE_TREE_SELECTION[i] > 0)
			{// Only keep positions with trees...
				vec3_t	mins, maxs;
				int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[i] - 1;
				float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[i] * TREE_SCALE_MULTIPLIER;
				float	TREE_HEIGHT = FOLIAGE_TREE_SCALE[i] * 2.5 * FOLIAGE_TREE_BILLBOARD_SIZE[FOLIAGE_TREE_SELECTION[i] - 1] * TREE_SCALE_MULTIPLIER;
				
				VectorSet(mins, -(TREE_RADIUS * 0.5), -(TREE_RADIUS * 0.5), -32.0);
				VectorSet(maxs, TREE_RADIUS * 0.5, TREE_RADIUS * 0.5, TREE_HEIGHT);

				//Com_Printf("Blocking tree at %f %f %f on navmesh size mins %f %f %f, maxs %f %f %f.\n", FOLIAGE_POSITIONS[i][0], FOLIAGE_POSITIONS[i][1], FOLIAGE_POSITIONS[i][2], mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);
				G_NavlibDisableArea(FOLIAGE_POSITIONS[i], mins, maxs);
			}
		}
	}
#endif //__USE_NAVLIB__
}

