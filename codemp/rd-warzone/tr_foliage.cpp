#include "tr_local.h"

#ifdef __RENDERER_GROUND_FOLIAGE__

#include "../qcommon/inifile.h"

#define GAME_VERSION "FOLIAGE"

extern char currentMapName[128];

// =======================================================================================================================================
//
//                                                             Foliage Rendering...
//
// =======================================================================================================================================

#define			FOLIAGE_MAX_FOLIAGES	4194304

#define			FOLIAGE_MAX_RANGE		8.5
#define			FOLIAGE_TREE_MAX_RANGE	64.0
#define			FOLIAGE_BILLBOARD_RANGE 6.5

#define			FOLIAGE_MIN_SCALE		0.3
#define			FOLIAGE_DENSITY			64.0

// =======================================================================================================================================
//
// BEGIN - FOLIAGE OPTIONS
//
// =======================================================================================================================================

// =======================================================================================================================================
//
// END - FOLIAGE OPTIONS
//
// =======================================================================================================================================


// =======================================================================================================================================
//
// These settings below are here for future adjustment and expansion...
//
// TODO: * Load <climateType>.climate file with all grasses/plants/trees md3's and textures lists.
//       * Add climate selection option to the header of <mapname>.foliage
//
// =======================================================================================================================================

qboolean	FOLIAGE_INITIALIZED = qfalse;

char		CURRENT_FOLIAGE_CLIMATE_OPTION[256] = { 0 };

float		NUM_TREE_TYPES = 9;

float		NUM_PLANT_SHADERS = 0;

#define		PLANT_SCALE_MULTIPLIER 1.0

#define		MAX_PLANT_SHADERS 100
#define		MAX_PLANT_MODELS 69

float		TREE_SCALE_MULTIPLIER = 1.0;

#if 1
static const char *TropicalPlantsModelsList[] = {
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant02.md3",
	"models/warzone/plants/groundplant02.md3",
	"models/warzone/plants/groundplant03.md3",
	"models/warzone/plants/groundplant03.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant02.md3",
	"models/warzone/plants/groundplant02.md3",
	"models/warzone/plants/groundplant03.md3",
	"models/warzone/plants/groundplant03.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant01.md3",
	"models/warzone/plants/groundplant02.md3",
	"models/warzone/plants/groundplant02.md3",
	"models/warzone/plants/groundplant03.md3",
	"models/warzone/plants/groundplant03.md3",
	"models/warzone/plants/groundplant04.md3",
	"models/warzone/plants/fern01.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/smalltree01.md3",
	// Near trees/walls...
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern03.md3",
	"models/warzone/plants/fern03.md3",
	"models/warzone/plants/fern04.md3",
	"models/warzone/plants/fern04.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fernplants01.md3",
	"models/warzone/plants/fernplants01.md3",
	"models/warzone/plants/smalltree02.md3",
	"models/warzone/plants/smalltree03.md3",
	"models/warzone/plants/smalltree04.md3",
	"models/warzone/plants/smalltree05.md3",
	"models/warzone/plants/smalltree06.md3",
};
#else // Grassy climate models... TODO: Add INI option when I can be bothered...
static const char *TropicalPlantsModelsList[] = {
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass02.md3",
	"models/warzone/plants/gcgrass02.md3",
	"models/warzone/plants/gcgrass02.md3",
	"models/warzone/plants/gcgrass02.md3",
	"models/warzone/plants/gcgrass02.md3",
	"models/warzone/plants/gcgrass03.md3",
	"models/warzone/plants/gcgrass04.md3",
	"models/warzone/plants/gcgrass05.md3",
	"models/warzone/plants/gcgrass06.md3",
	"models/warzone/plants/gcgrass07.md3",
	"models/warzone/plants/gcgrass08.md3",
	"models/warzone/plants/gcgrass09.md3",
	"models/warzone/plants/gcgrass10.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/gcplantmix01.md3",
	// Near trees/walls...
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
};
#endif

#define SpringPlantsModelsList TropicalPlantsModelsList
#define EndorPlantsModelsList TropicalPlantsModelsList
#define SnowPlantsModelsList TropicalPlantsModelsList


// =======================================================================================================================================
//
// CVAR related defines...
//
// =======================================================================================================================================

#define		FOLIAGE_AREA_SIZE 					512
int			FOLIAGE_VISIBLE_DISTANCE = -1;
int			FOLIAGE_TREE_VISIBLE_DISTANCE = -1;

#define		FOLIAGE_AREA_MAX					131072
#define		FOLIAGE_AREA_MAX_FOLIAGES			256

typedef enum {
	FOLIAGE_PASS_GRASS,
	FOLIAGE_PASS_PLANT,
	FOLIAGE_PASS_CLOSETREE,
	FOLIAGE_PASS_TREE,
} foliagePassTypes_t;

// =======================================================================================================================================
//
// These maybe should have been a stuct, but many separate variables let us use static memory allocations...
//
// =======================================================================================================================================

qboolean	FOLIAGE_LOADED = qfalse;
int			FOLIAGE_NUM_POSITIONS = 0;
#if 0
vec3_t		FOLIAGE_POSITIONS[FOLIAGE_MAX_FOLIAGES];
vec3_t		FOLIAGE_NORMALS[FOLIAGE_MAX_FOLIAGES];
int			FOLIAGE_PLANT_SELECTION[FOLIAGE_MAX_FOLIAGES];
float		FOLIAGE_PLANT_ANGLES[FOLIAGE_MAX_FOLIAGES];
float		FOLIAGE_PLANT_SCALE[FOLIAGE_MAX_FOLIAGES];
int			FOLIAGE_TREE_SELECTION[FOLIAGE_MAX_FOLIAGES];
float		FOLIAGE_TREE_ANGLES[FOLIAGE_MAX_FOLIAGES];
float		FOLIAGE_TREE_SCALE[FOLIAGE_MAX_FOLIAGES];
#else
vec3_t		*FOLIAGE_POSITIONS = NULL;
vec3_t		*FOLIAGE_NORMALS = NULL;
int			*FOLIAGE_PLANT_SELECTION = NULL;
float		*FOLIAGE_PLANT_ANGLES = NULL;
float		*FOLIAGE_PLANT_SCALE = NULL;
int			*FOLIAGE_TREE_SELECTION = NULL;
float		*FOLIAGE_TREE_ANGLES = NULL;
float		*FOLIAGE_TREE_SCALE = NULL;
#endif


int			FOLIAGE_AREAS_COUNT = 0;
int			FOLIAGE_AREAS_LIST_COUNT[FOLIAGE_AREA_MAX];
int			FOLIAGE_AREAS_LIST[FOLIAGE_AREA_MAX][FOLIAGE_AREA_MAX_FOLIAGES];
int			FOLIAGE_AREAS_TREES_LIST_COUNT[FOLIAGE_AREA_MAX];
int			FOLIAGE_AREAS_TREES_VISCHECK_TIME[FOLIAGE_AREA_MAX];
qboolean	FOLIAGE_AREAS_TREES_VISCHECK_RESULT[FOLIAGE_AREA_MAX];
int			FOLIAGE_AREAS_TREES_LIST[FOLIAGE_AREA_MAX][FOLIAGE_AREA_MAX_FOLIAGES];
vec3_t		FOLIAGE_AREAS_MINS[FOLIAGE_AREA_MAX];
vec3_t		FOLIAGE_AREAS_MAXS[FOLIAGE_AREA_MAX];


qhandle_t	FOLIAGE_PLANT_MODEL[5] = { 0 };
qhandle_t	FOLAIGE_GRASS_BILLBOARD_SHADER = 0;
qhandle_t	FOLIAGE_TREE_MODEL[16] = { 0 };
//void		*FOLIAGE_TREE_G2_MODEL[16] = { NULL };
float		FOLIAGE_TREE_RADIUS[16] = { 0 };
float		FOLIAGE_TREE_ZOFFSET[16] = { 0 };
qhandle_t	FOLIAGE_TREE_BILLBOARD_SHADER[16] = { 0 };
float		FOLIAGE_TREE_BILLBOARD_SIZE[16] = { 0 };

qhandle_t	FOLIAGE_PLANT_SHADERS[MAX_PLANT_SHADERS] = { 0 };
qhandle_t	FOLIAGE_PLANT_MODELS[MAX_PLANT_MODELS] = { 0 };

int			IN_RANGE_AREAS_LIST_COUNT = 0;
int			IN_RANGE_AREAS_LIST[8192];
float		IN_RANGE_AREAS_DISTANCE[8192];
int			IN_RANGE_TREE_AREAS_LIST_COUNT = 0;
int			IN_RANGE_TREE_AREAS_LIST[131072/*8192*/];
float		IN_RANGE_TREE_AREAS_DISTANCE[131072/*8192*/];

qboolean	MAP_HAS_TREES = qfalse;

// =======================================================================================================================================
//
// Area System... This allows us to manipulate in realtime which foliages we should use...
//
// =======================================================================================================================================

int FOLIAGE_AreaNumForOrg(vec3_t moveOrg)
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

qboolean FOLIAGE_In_Bounds(int areaNum, int foliageNum)
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

const int FOLIAGE_AREA_FILE_VERSION = 1;

qboolean FOLIAGE_LoadFoliageAreas(void)
{
	fileHandle_t	f;
	int				numPositions = 0;
	int				i = 0;
	int				version = 0;

	ri->FS_FOpenFileRead(va("foliage/%s.foliageAreas", currentMapName), &f, qfalse);

	if (!f)
	{
		return qfalse;
	}

	ri->FS_Read(&version, sizeof(int), f);

	if (version != FOLIAGE_AREA_FILE_VERSION)
	{// Old version... Update...
		ri->FS_FCloseFile(f);
		return qfalse;
	}

	ri->FS_Read(&numPositions, sizeof(int), f);

	if (numPositions != FOLIAGE_NUM_POSITIONS)
	{// Mismatch... Regenerate...
		ri->FS_FCloseFile(f);
		return qfalse;
	}

	ri->FS_Read(&FOLIAGE_AREAS_COUNT, sizeof(int), f);

	for (i = 0; i < FOLIAGE_AREAS_COUNT; i++)
	{
		ri->FS_Read(&FOLIAGE_AREAS_MINS[i], sizeof(vec3_t), f);
		ri->FS_Read(&FOLIAGE_AREAS_MAXS[i], sizeof(vec3_t), f);

		ri->FS_Read(&FOLIAGE_AREAS_LIST_COUNT[i], sizeof(int), f);
		ri->FS_Read(&FOLIAGE_AREAS_LIST[i], sizeof(int)*FOLIAGE_AREAS_LIST_COUNT[i], f);

		ri->FS_Read(&FOLIAGE_AREAS_TREES_LIST_COUNT[i], sizeof(int), f);
		ri->FS_Read(&FOLIAGE_AREAS_TREES_LIST[i], sizeof(int)*FOLIAGE_AREAS_TREES_LIST_COUNT[i], f);
	}

	ri->FS_FCloseFile(f);

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Successfully loaded %i foliageAreas to foliageArea file ^7foliage/%s.foliageAreas^5.\n", GAME_VERSION, FOLIAGE_AREAS_COUNT, currentMapName);

	return qtrue;
}

void FOLIAGE_SaveFoliageAreas(void)
{
	fileHandle_t	f;
	int				i = 0;

	f = ri->FS_FOpenFileWrite(va("foliage/%s.foliageAreas", currentMapName), qfalse);

	if (!f)
	{
		ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Failed to save foliageAreas file ^7foliage/%s.foliageAreas^5 for save.\n", GAME_VERSION, currentMapName);
		return;
	}

	ri->FS_Write(&FOLIAGE_AREA_FILE_VERSION, sizeof(int), f);

	ri->FS_Write(&FOLIAGE_NUM_POSITIONS, sizeof(int), f);

	ri->FS_Write(&FOLIAGE_AREAS_COUNT, sizeof(int), f);

	for (i = 0; i < FOLIAGE_AREAS_COUNT; i++)
	{
		ri->FS_Write(&FOLIAGE_AREAS_MINS[i], sizeof(vec3_t), f);
		ri->FS_Write(&FOLIAGE_AREAS_MAXS[i], sizeof(vec3_t), f);

		ri->FS_Write(&FOLIAGE_AREAS_LIST_COUNT[i], sizeof(int), f);
		ri->FS_Write(&FOLIAGE_AREAS_LIST[i], sizeof(int)*FOLIAGE_AREAS_LIST_COUNT[i], f);

		ri->FS_Write(&FOLIAGE_AREAS_TREES_LIST_COUNT[i], sizeof(int), f);
		ri->FS_Write(&FOLIAGE_AREAS_TREES_LIST[i], sizeof(int)*FOLIAGE_AREAS_TREES_LIST_COUNT[i], f);
	}

	ri->FS_FCloseFile(f);

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Successfully saved %i foliageAreas to foliageArea file ^7foliage/%s.foliageAreas^5.\n", GAME_VERSION, FOLIAGE_AREAS_COUNT, currentMapName);
}

void FOLIAGE_Setup_Foliage_Areas(void)
{
	int		DENSITY_REMOVED = 0;
	int		ZERO_SCALE_REMOVED = 0;
	int		areaNum = 0, i = 0;
	vec3_t	mins, maxs, mapMins, mapMaxs;

	// Try to load previous areas file...
	if (FOLIAGE_LoadFoliageAreas()) return;

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
		FOLIAGE_AREAS_TREES_LIST_COUNT[areaNum] = 0;
		FOLIAGE_AREAS_TREES_VISCHECK_TIME[areaNum] = 0;
		FOLIAGE_AREAS_TREES_VISCHECK_RESULT[areaNum] = qtrue;

		while (FOLIAGE_AREAS_LIST_COUNT[areaNum] == 0 && mins[1] <= mapMaxs[1])
		{// While loop is so we can skip zero size areas for speed...
			VectorCopy(mins, FOLIAGE_AREAS_MINS[areaNum]);
			VectorCopy(maxs, FOLIAGE_AREAS_MAXS[areaNum]);

			// Assign foliages to the area lists...
			for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
			{
				if (FOLIAGE_In_Bounds(areaNum, i))
				{
					qboolean OVER_DENSITY = qfalse;

					if (FOLIAGE_AREAS_LIST_COUNT[areaNum] > FOLIAGE_AREA_MAX_FOLIAGES)
					{
						//ri->Printf(PRINT_WARNING, "*** Area %i has more then %i foliages ***\n", areaNum, (int)FOLIAGE_AREA_MAX_FOLIAGES);
						break;
					}

					if (FOLIAGE_TREE_SELECTION[i] <= 0)
					{// Never remove trees...
						int j = 0;

						if (FOLIAGE_PLANT_SCALE[i] <= 0)
						{// Zero scale plant... Remove...
							ZERO_SCALE_REMOVED++;
							continue;
						}

						for (j = 0; j < FOLIAGE_AREAS_LIST_COUNT[areaNum]; j++)
						{// Let's use a density setting to improve FPS...
							if (DistanceHorizontal(FOLIAGE_POSITIONS[i], FOLIAGE_POSITIONS[FOLIAGE_AREAS_LIST[areaNum][j]]) < FOLIAGE_DENSITY * FOLIAGE_PLANT_SCALE[i])
							{// Adding this would go over density setting...
								OVER_DENSITY = qtrue;
								DENSITY_REMOVED++;
								break;
							}
						}
					}

					if (!OVER_DENSITY)
					{
						FOLIAGE_AREAS_LIST[areaNum][FOLIAGE_AREAS_LIST_COUNT[areaNum]] = i;
						FOLIAGE_AREAS_LIST_COUNT[areaNum]++;

						if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
						{// Also make a trees list for faster tree selection...
							FOLIAGE_AREAS_TREES_LIST[areaNum][FOLIAGE_AREAS_TREES_LIST_COUNT[areaNum]] = i;
							FOLIAGE_AREAS_TREES_LIST_COUNT[areaNum]++;
						}
					}
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

	ri->Printf(PRINT_WARNING, "Generated %i foliage areas. %i used of %i total foliages. %i removed by density setting. %i removed due to zero scale.\n", FOLIAGE_AREAS_COUNT, FOLIAGE_NUM_POSITIONS - DENSITY_REMOVED, FOLIAGE_NUM_POSITIONS, DENSITY_REMOVED, ZERO_SCALE_REMOVED);

	// Save for future use...
	FOLIAGE_SaveFoliageAreas();
}

void FOLIAGE_Check_CVar_Change(void)
{
}

vec3_t		LAST_ORG = { 0 };
vec3_t		LAST_ANG = { 0 };

void FOLIAGE_VisibleAreaSortGrass(void)
{// Sorted furthest to closest...
	int i, j, increment, temp;
	float tempDist;

	increment = 3;

	while (increment > 0)
	{
		for (i = 0; i < IN_RANGE_AREAS_LIST_COUNT; i++)
		{
			temp = IN_RANGE_AREAS_LIST[i];
			tempDist = IN_RANGE_AREAS_DISTANCE[i];

			j = i;

			while ((j >= increment) && (IN_RANGE_AREAS_DISTANCE[j - increment] < tempDist))
			{
				IN_RANGE_AREAS_LIST[j] = IN_RANGE_AREAS_LIST[j - increment];
				IN_RANGE_AREAS_DISTANCE[j] = IN_RANGE_AREAS_DISTANCE[j - increment];
				j = j - increment;
			}

			IN_RANGE_AREAS_LIST[j] = temp;
			IN_RANGE_AREAS_DISTANCE[j] = tempDist;
		}

		if (increment / 2 != 0)
			increment = increment / 2;
		else if (increment == 1)
			increment = 0;
		else
			increment = 1;
	}
}

void FOLIAGE_VisibleAreaSortTrees(void)
{// Sorted closest to furthest...
	if (MAP_HAS_TREES)
	{
		int i, j, increment, temp;
		float tempDist;

		increment = 3;

		while (increment > 0)
		{
			for (i = 0; i < IN_RANGE_TREE_AREAS_LIST_COUNT; i++)
			{
				temp = IN_RANGE_TREE_AREAS_LIST[i];
				tempDist = IN_RANGE_TREE_AREAS_DISTANCE[i];

				j = i;

				while ((j >= increment) && (IN_RANGE_TREE_AREAS_DISTANCE[j - increment] > tempDist))
				{
					IN_RANGE_TREE_AREAS_LIST[j] = IN_RANGE_TREE_AREAS_LIST[j - increment];
					IN_RANGE_TREE_AREAS_DISTANCE[j] = IN_RANGE_TREE_AREAS_DISTANCE[j - increment];
					j = j - increment;
				}

				IN_RANGE_TREE_AREAS_LIST[j] = temp;
				IN_RANGE_TREE_AREAS_DISTANCE[j] = tempDist;
			}

			if (increment / 2 != 0)
				increment = increment / 2;
			else if (increment == 1)
				increment = 0;
			else
				increment = 1;
		}
	}
}

#include <amp.h>
#include <amp_math.h>
using namespace concurrency;
using namespace concurrency::precise_math;

void FOLIAGE_Calc_In_Range_Areas(void)
{
	int i = 0;

	FOLIAGE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*FOLIAGE_MAX_RANGE;
	FOLIAGE_TREE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*FOLIAGE_TREE_MAX_RANGE;

	if (Distance(tr.refdef.vieworg, LAST_ORG) > 128.0)
	{// Update in range list...
		int i = 0;

		VectorCopy(tr.refdef.vieworg, LAST_ORG);
		VectorCopy(tr.refdef.viewangles, LAST_ANG);

		IN_RANGE_AREAS_LIST_COUNT = 0;
		IN_RANGE_TREE_AREAS_LIST_COUNT = 0;

		// Calculate currently-in-range areas to use...
		for (i = 0; i < FOLIAGE_AREAS_COUNT; i++)
		{
			float minsDist = DistanceHorizontal(FOLIAGE_AREAS_MINS[i], tr.refdef.vieworg);
			float maxsDist = DistanceHorizontal(FOLIAGE_AREAS_MAXS[i], tr.refdef.vieworg);

			if (minsDist < FOLIAGE_VISIBLE_DISTANCE
				|| maxsDist < FOLIAGE_VISIBLE_DISTANCE)
			{
				IN_RANGE_AREAS_LIST[IN_RANGE_AREAS_LIST_COUNT] = i;

				if (minsDist < maxsDist)
					IN_RANGE_AREAS_DISTANCE[IN_RANGE_AREAS_LIST_COUNT] = minsDist;
				else
					IN_RANGE_AREAS_DISTANCE[IN_RANGE_AREAS_LIST_COUNT] = maxsDist;

				IN_RANGE_AREAS_LIST_COUNT++;
			}
			else if (MAP_HAS_TREES
				&& (minsDist < FOLIAGE_TREE_VISIBLE_DISTANCE || maxsDist < FOLIAGE_TREE_VISIBLE_DISTANCE))
			{
				IN_RANGE_TREE_AREAS_LIST[IN_RANGE_TREE_AREAS_LIST_COUNT] = i;

				if (minsDist < maxsDist)
					IN_RANGE_TREE_AREAS_DISTANCE[IN_RANGE_TREE_AREAS_LIST_COUNT] = minsDist;
				else
					IN_RANGE_TREE_AREAS_DISTANCE[IN_RANGE_TREE_AREAS_LIST_COUNT] = maxsDist;

				IN_RANGE_TREE_AREAS_LIST_COUNT++;
			}
		}

#if 0
		if (cg_foliageAreaSorting.integer)
		{
			FOLIAGE_VisibleAreaSortGrass();
			FOLIAGE_VisibleAreaSortTrees();

			/*for (int i = 0; i < IN_RANGE_AREAS_LIST_COUNT; i++)
			{
			ri->Printf(PRINT_WARNING, "[%i] dist %f.\n", i, IN_RANGE_AREAS_DISTANCE[i]);
			}*/
		}
#endif

		//ri->Printf(PRINT_WARNING, "There are %i foliage areas in range. %i tree areas.\n", IN_RANGE_AREAS_LIST_COUNT, IN_RANGE_TREE_AREAS_LIST_COUNT);
	}
}


// =======================================================================================================================================
//
// Actual foliage drawing code...
//
// =======================================================================================================================================

void FOLIAGE_AddFoliageEntityToScene(refEntity_t *ent)
{
	RE_AddRefEntityToScene(ent);
}

void FOLIAGE_ApplyAxisRotation(vec3_t axis[3], int rotType, float value)
{//apply matrix rotation to this axis.
 //rotType = type of rotation (PITCH, YAW, ROLL)
 //value = size of rotation in degrees, no action if == 0
	vec3_t result[3];  //The resulting axis
	vec3_t rotation[3];  //rotation matrix
	int i, j; //multiplication counters

	if (value == 0)
	{//no rotation, just return.
		return;
	}

	//init rotation matrix
	switch (rotType)
	{
	case ROLL: //R_X
		rotation[0][0] = 1;
		rotation[0][1] = 0;
		rotation[0][2] = 0;

		rotation[1][0] = 0;
		rotation[1][1] = cos(value / 360 * (2 * M_PI));
		rotation[1][2] = -sin(value / 360 * (2 * M_PI));

		rotation[2][0] = 0;
		rotation[2][1] = sin(value / 360 * (2 * M_PI));
		rotation[2][2] = cos(value / 360 * (2 * M_PI));
		break;

	case PITCH: //R_Y
		rotation[0][0] = cos(value / 360 * (2 * M_PI));
		rotation[0][1] = 0;
		rotation[0][2] = sin(value / 360 * (2 * M_PI));

		rotation[1][0] = 0;
		rotation[1][1] = 1;
		rotation[1][2] = 0;

		rotation[2][0] = -sin(value / 360 * (2 * M_PI));
		rotation[2][1] = 0;
		rotation[2][2] = cos(value / 360 * (2 * M_PI));
		break;

	case YAW: //R_Z
		rotation[0][0] = cos(value / 360 * (2 * M_PI));
		rotation[0][1] = -sin(value / 360 * (2 * M_PI));
		rotation[0][2] = 0;

		rotation[1][0] = sin(value / 360 * (2 * M_PI));
		rotation[1][1] = cos(value / 360 * (2 * M_PI));
		rotation[1][2] = 0;

		rotation[2][0] = 0;
		rotation[2][1] = 0;
		rotation[2][2] = 1;
		break;

	default:
		ri->Printf(PRINT_WARNING, "Error:  Bad rotType %i given to ApplyAxisRotation\n", rotType);
		break;
	};

	//apply rotation
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			result[i][j] = rotation[i][0] * axis[0][j] + rotation[i][1] * axis[1][j]
				+ rotation[i][2] * axis[2][j];
			/* post apply method
			result[i][j] = axis[i][0]*rotation[0][j] + axis[i][1]*rotation[1][j]
			+ axis[i][2]*rotation[2][j];
			*/
		}
	}

	//copy result
	AxisCopy(result, axis);

}

void ScaleModelAxis(refEntity_t	*ent)

{		// scale the model should we need to
	if (ent->modelScale[0] && ent->modelScale[0] != 1.0f)
	{
		VectorScale(ent->axis[0], ent->modelScale[0], ent->axis[0]);
		ent->nonNormalizedAxes = qtrue;
	}
	if (ent->modelScale[1] && ent->modelScale[1] != 1.0f)
	{
		VectorScale(ent->axis[1], ent->modelScale[1], ent->axis[1]);
		ent->nonNormalizedAxes = qtrue;
	}
	if (ent->modelScale[2] && ent->modelScale[2] != 1.0f)
	{
		VectorScale(ent->axis[2], ent->modelScale[2], ent->axis[2]);
		ent->nonNormalizedAxes = qtrue;
	}
}

void FOLIAGE_AddToScreen(int num, int passType) {
	refEntity_t		re;
	vec3_t			angles;
	float			dist = 0;
	float			distFadeScale = 1.5;
	float			minFoliageScale = FOLIAGE_MIN_SCALE;

	if (FOLIAGE_TREE_SELECTION[num] <= 0 && FOLIAGE_PLANT_SCALE[num] < minFoliageScale) return;

#if 0
	if (cg.renderingThirdPerson)
		dist = Distance/*Horizontal*/(FOLIAGE_POSITIONS[num], cg_entities[cg.clientNum].lerpOrigin);
	else
#endif
		dist = Distance/*Horizontal*/(FOLIAGE_POSITIONS[num], tr.refdef.vieworg);

	// Cull anything in the area outside of cvar specified radius...
	if (passType == FOLIAGE_PASS_TREE && dist > FOLIAGE_TREE_VISIBLE_DISTANCE) return;
	if (passType != FOLIAGE_PASS_TREE && passType != FOLIAGE_PASS_CLOSETREE && dist > FOLIAGE_VISIBLE_DISTANCE && FOLIAGE_TREE_SELECTION[num] <= 0) return;

	memset(&re, 0, sizeof(re));

	//re.renderfx |= RF_FORCE_ENT_ALPHA;
	//re.renderfx |= RF_ALPHA_DEPTH;

	VectorCopy(FOLIAGE_POSITIONS[num], re.origin);

	FOLIAGE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*FOLIAGE_MAX_RANGE;
	FOLIAGE_TREE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*FOLIAGE_TREE_MAX_RANGE;

	//#define __GRASS_ONLY__

	if (dist <= FOLIAGE_VISIBLE_DISTANCE)
	{// Draw grass...
		qboolean skipGrass = qfalse;
		qboolean skipPlant = qfalse;
		float minGrassScale = ((dist / FOLIAGE_VISIBLE_DISTANCE) * 0.7) + 0.3;

#ifndef __GRASS_ONLY__
		if (dist > FOLIAGE_VISIBLE_DISTANCE) skipGrass = qtrue;
		if (FOLIAGE_PLANT_SCALE[num] <= minGrassScale && dist > FOLIAGE_VISIBLE_DISTANCE) skipPlant = qtrue;
		if (FOLIAGE_PLANT_SCALE[num] < minFoliageScale) skipPlant = qtrue;

		if (passType == FOLIAGE_PASS_PLANT && !skipPlant && FOLIAGE_PLANT_SELECTION[num] > 0)
		{// Add plant model as well...
			float PLANT_SCALE = 0.4 * FOLIAGE_PLANT_SCALE[num] * PLANT_SCALE_MULTIPLIER*distFadeScale;

			re.reType = RT_PLANT;//RT_MODEL;

			re.origin[2] += 8.0 + (1.0 - FOLIAGE_PLANT_SCALE[num]);

			re.hModel = FOLIAGE_PLANT_MODELS[FOLIAGE_PLANT_SELECTION[num] - 1];

			VectorSet(re.modelScale, PLANT_SCALE, PLANT_SCALE, PLANT_SCALE);

			vectoangles(FOLIAGE_NORMALS[num], angles);
			angles[PITCH] += 90;
			//angles[YAW] = FOLIAGE_PLANT_ANGLES[num];
			//angles[YAW] += 90.0 + FOLIAGE_PLANT_ANGLES[num];

			VectorCopy(angles, re.angles);
			AnglesToAxis(angles, re.axis);

			//FOLIAGE_ApplyAxisRotation(re.axis, PITCH, AngOffset[PITCH]);
			FOLIAGE_ApplyAxisRotation(re.axis, YAW, FOLIAGE_PLANT_ANGLES[num]);
			//FOLIAGE_ApplyAxisRotation(re.axis, ROLL, AngOffset[ROLL]);

			// Add extra rotation so it's different to grass angle...
			//RotateAroundDirection( re.axis, FOLIAGE_PLANT_ANGLES[num] );

			ScaleModelAxis(&re);

			FOLIAGE_AddFoliageEntityToScene(&re);
		}

		if ((passType == FOLIAGE_PASS_GRASS || passType == FOLIAGE_PASS_CLOSETREE) && !skipGrass && dist <= FOLIAGE_VISIBLE_DISTANCE)
#endif //__GRASS_ONLY__
		{
		}
	}

	if ((passType == FOLIAGE_PASS_CLOSETREE || passType == FOLIAGE_PASS_TREE) && FOLIAGE_TREE_SELECTION[num] > 0)
	{// Add the tree model...
		VectorCopy(FOLIAGE_POSITIONS[num], re.origin);
		re.customShader = 0;
		re.renderfx = 0;

		if (dist > FOLIAGE_AREA_SIZE*FOLIAGE_BILLBOARD_RANGE || dist > FOLIAGE_TREE_VISIBLE_DISTANCE)
		{
			re.reType = RT_SPRITE;

			re.radius = FOLIAGE_TREE_SCALE[num] * 2.5*FOLIAGE_TREE_BILLBOARD_SIZE[FOLIAGE_TREE_SELECTION[num] - 1] * TREE_SCALE_MULTIPLIER;

			re.customShader = FOLIAGE_TREE_BILLBOARD_SHADER[FOLIAGE_TREE_SELECTION[num] - 1];

			re.shaderRGBA[0] = 255;
			re.shaderRGBA[1] = 255;
			re.shaderRGBA[2] = 255;
			re.shaderRGBA[3] = 255;

			re.origin[2] += re.radius;
			re.origin[2] += FOLIAGE_TREE_ZOFFSET[FOLIAGE_TREE_SELECTION[num] - 1];

			angles[PITCH] = angles[ROLL] = 0.0f;
			angles[YAW] = FOLIAGE_TREE_ANGLES[num];

			VectorCopy(angles, re.angles);
			AnglesToAxis(angles, re.axis);

			FOLIAGE_AddFoliageEntityToScene(&re);
		}
		else
		{
			float	furthestDist = 0.0;
			int		furthestNum = 0;
			int		tree = 0;

			re.reType = RT_MODEL;
			re.hModel = FOLIAGE_TREE_MODEL[FOLIAGE_TREE_SELECTION[num] - 1];

			/*if (FOLIAGE_TREE_G2_MODEL[FOLIAGE_TREE_SELECTION[num] - 1] != NULL)
			{// G2 Instance of this model...
				re.ghoul2 = FOLIAGE_TREE_G2_MODEL[FOLIAGE_TREE_SELECTION[num] - 1];
			}*/

			VectorSet(re.modelScale, FOLIAGE_TREE_SCALE[num] * 2.5*TREE_SCALE_MULTIPLIER, FOLIAGE_TREE_SCALE[num] * 2.5*TREE_SCALE_MULTIPLIER, FOLIAGE_TREE_SCALE[num] * 2.5*TREE_SCALE_MULTIPLIER);

			re.origin[2] += FOLIAGE_TREE_ZOFFSET[FOLIAGE_TREE_SELECTION[num] - 1];

			angles[PITCH] = angles[ROLL] = 0.0f;
			angles[YAW] = 270.0 - FOLIAGE_TREE_ANGLES[num];

			VectorCopy(angles, re.angles);
			AnglesToAxis(angles, re.axis);

			ScaleModelAxis(&re);

			FOLIAGE_AddFoliageEntityToScene(&re);
		}
	}
}

qboolean FOLIAGE_LoadMapClimateInfo(void)
{
	const char		*climateName = NULL;

	climateName = IniRead(va("foliage/%s.climateInfo", currentMapName), "CLIMATE", "CLIMATE_TYPE", "");

	memset(CURRENT_FOLIAGE_CLIMATE_OPTION, 0, sizeof(CURRENT_FOLIAGE_CLIMATE_OPTION));
	strncpy(CURRENT_FOLIAGE_CLIMATE_OPTION, climateName, strlen(climateName));

	if (CURRENT_FOLIAGE_CLIMATE_OPTION[0] == '\0')
	{
		ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: No map climate info file ^7foliage/%s.climateInfo^5. Using default climate option.\n", GAME_VERSION, currentMapName);
		strncpy(CURRENT_FOLIAGE_CLIMATE_OPTION, "tropical", strlen("tropical"));
		return qfalse;
	}

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Successfully loaded climateInfo file ^7foliage/%s.climateInfo^5. Using ^3%s^5 climate option.\n", GAME_VERSION, currentMapName, CURRENT_FOLIAGE_CLIMATE_OPTION);

	return qtrue;
}

void FOLIAGE_FreeMemory(void)
{
	if (FOLIAGE_POSITIONS)
	{// Free any current memory...
		free(FOLIAGE_POSITIONS);
		free(FOLIAGE_NORMALS);
		free(FOLIAGE_PLANT_SELECTION);
		free(FOLIAGE_PLANT_ANGLES);
		free(FOLIAGE_PLANT_SCALE);
		free(FOLIAGE_TREE_SELECTION);
		free(FOLIAGE_TREE_ANGLES);
		free(FOLIAGE_TREE_SCALE);

		FOLIAGE_POSITIONS = NULL;
		FOLIAGE_NORMALS = NULL;
		FOLIAGE_PLANT_SELECTION = NULL;
		FOLIAGE_PLANT_ANGLES = NULL;
		FOLIAGE_PLANT_SCALE = NULL;
		FOLIAGE_TREE_SELECTION = NULL;
		FOLIAGE_TREE_ANGLES = NULL;
		FOLIAGE_TREE_SCALE = NULL;
	}
}

qboolean FOLIAGE_LoadFoliagePositions(char *filename)
{
	fileHandle_t	f;
	int				i = 0;
	int				numPositions = 0;
	int				numRemovedPositions = 0;
	float			minFoliageScale = FOLIAGE_MIN_SCALE;
	int fileCount = 0;
	int foliageCount = 0;

	if (!filename || filename[0] == '0')
		ri->FS_FOpenFileRead(va("foliage/%s.foliage", currentMapName), &f, qfalse);
	else
		ri->FS_FOpenFileRead(va("foliage/%s.foliage", filename), &f, qfalse);

	if (!f)
	{
		ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: No foliage file ^7foliage/%s.foliage^5.\n", GAME_VERSION, currentMapName);
		return qfalse;
	}

	MAP_HAS_TREES = qfalse;

	FOLIAGE_NUM_POSITIONS = 0;

	FOLIAGE_FreeMemory();

	ri->FS_Read(&fileCount, sizeof(int), f);

	FOLIAGE_POSITIONS = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
	FOLIAGE_NORMALS = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
	FOLIAGE_PLANT_SELECTION = (int *)malloc(fileCount * sizeof(int));
	FOLIAGE_PLANT_ANGLES = (float *)malloc(fileCount * sizeof(float));
	FOLIAGE_PLANT_SCALE = (float *)malloc(fileCount * sizeof(float));
	FOLIAGE_TREE_SELECTION = (int *)malloc(fileCount * sizeof(int));
	FOLIAGE_TREE_ANGLES = (float *)malloc(fileCount * sizeof(float));
	FOLIAGE_TREE_SCALE = (float *)malloc(fileCount * sizeof(float));

	for (i = 0; i < fileCount; i++)
	{
		ri->FS_Read(&FOLIAGE_POSITIONS[foliageCount], sizeof(vec3_t), f);
		ri->FS_Read(&FOLIAGE_NORMALS[foliageCount], sizeof(vec3_t), f);
		ri->FS_Read(&FOLIAGE_PLANT_SELECTION[foliageCount], sizeof(int), f);
		ri->FS_Read(&FOLIAGE_PLANT_ANGLES[foliageCount], sizeof(float), f);
		ri->FS_Read(&FOLIAGE_PLANT_SCALE[foliageCount], sizeof(float), f);
		ri->FS_Read(&FOLIAGE_TREE_SELECTION[foliageCount], sizeof(int), f);
		ri->FS_Read(&FOLIAGE_TREE_ANGLES[foliageCount], sizeof(float), f);
		ri->FS_Read(&FOLIAGE_TREE_SCALE[foliageCount], sizeof(float), f);

		if (FOLIAGE_TREE_SELECTION[foliageCount] > 0)
		{
			MAP_HAS_TREES = qtrue;
		}

		if (FOLIAGE_TREE_SELECTION[foliageCount] > 0)
		{// Only keep positions with trees or plants...
			FOLIAGE_PLANT_SELECTION[foliageCount] = 0;
			foliageCount++;
		}
		else if (FOLIAGE_TREE_SELECTION[foliageCount] > 0 || FOLIAGE_PLANT_SELECTION[foliageCount] > 0)
		{// Only keep positions with trees or plants...
			foliageCount++;
		}
		else
		{
			numRemovedPositions++;
		}
	}

	FOLIAGE_NUM_POSITIONS = foliageCount;

	ri->FS_FCloseFile(f);

	if (FOLIAGE_NUM_POSITIONS > 0)
	{// Re-alloc memory to the amount actually used...
		FOLIAGE_POSITIONS = (vec3_t *)realloc(FOLIAGE_POSITIONS, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_NORMALS = (vec3_t *)realloc(FOLIAGE_NORMALS, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_PLANT_SELECTION = (int *)realloc(FOLIAGE_PLANT_SELECTION, FOLIAGE_NUM_POSITIONS * sizeof(int));
		FOLIAGE_PLANT_ANGLES = (float *)realloc(FOLIAGE_PLANT_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(float));
		FOLIAGE_PLANT_SCALE = (float *)realloc(FOLIAGE_PLANT_SCALE, FOLIAGE_NUM_POSITIONS * sizeof(float));
		FOLIAGE_TREE_SELECTION = (int *)realloc(FOLIAGE_TREE_SELECTION, FOLIAGE_NUM_POSITIONS * sizeof(int));
		FOLIAGE_TREE_ANGLES = (float *)realloc(FOLIAGE_TREE_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(float));
		FOLIAGE_TREE_SCALE = (float *)realloc(FOLIAGE_TREE_SCALE, FOLIAGE_NUM_POSITIONS * sizeof(float));
	}
	else
	{
		FOLIAGE_FreeMemory();
	}

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Successfully loaded %i foliage points (%i unused grasses removed) from foliage file ^7foliage/%s.foliage^5.\n", GAME_VERSION,
		FOLIAGE_NUM_POSITIONS, numRemovedPositions, currentMapName);

	if (!filename || filename[0] == '0')
	{// Don't need to waste time on this when doing "copy" function...
		FOLIAGE_Setup_Foliage_Areas();
	}

	return qtrue;
}

qboolean FOLIAGE_IgnoreFoliageOnMap(void)
{
#if 0
	if (StringContainsWord(currentMapName, "eisley")
		|| StringContainsWord(currentMapName, "desert")
		|| StringContainsWord(currentMapName, "tatooine")
		|| StringContainsWord(currentMapName, "hoth")
		|| StringContainsWord(currentMapName, "mp/ctf1")
		|| StringContainsWord(currentMapName, "mp/ctf2")
		|| StringContainsWord(currentMapName, "mp/ctf4")
		|| StringContainsWord(currentMapName, "mp/ctf5")
		|| StringContainsWord(currentMapName, "mp/ffa1")
		|| StringContainsWord(currentMapName, "mp/ffa2")
		|| StringContainsWord(currentMapName, "mp/ffa3")
		|| StringContainsWord(currentMapName, "mp/ffa4")
		|| StringContainsWord(currentMapName, "mp/ffa5")
		|| StringContainsWord(currentMapName, "mp/duel1")
		|| StringContainsWord(currentMapName, "mp/duel2")
		|| StringContainsWord(currentMapName, "mp/duel3")
		|| StringContainsWord(currentMapName, "mp/duel4")
		|| StringContainsWord(currentMapName, "mp/duel5")
		|| StringContainsWord(currentMapName, "mp/duel7")
		|| StringContainsWord(currentMapName, "mp/duel9")
		|| StringContainsWord(currentMapName, "mp/duel10")
		|| StringContainsWord(currentMapName, "bespin_streets")
		|| StringContainsWord(currentMapName, "bespin_platform"))
	{// Ignore this map... We know we don't need grass here...
		return qtrue;
	}

#endif
	return qfalse;
}

void FOLIAGE_DrawGrass(void)
{
	int spot = 0;
	int CURRENT_AREA = 0;
	vec3_t viewOrg, viewAngles;

	if (FOLIAGE_IgnoreFoliageOnMap())
	{// Ignore this map... We know we don't need grass here...
		FOLIAGE_NUM_POSITIONS = 0;
		FOLIAGE_LOADED = qtrue;
		return;
	}

	if (!FOLIAGE_LOADED)
	{
		FOLIAGE_LoadFoliagePositions(NULL);
		FOLIAGE_LOADED = qtrue;
		FOLIAGE_LoadMapClimateInfo();
	}

	if (FOLIAGE_NUM_POSITIONS <= 0)
	{
		return;
	}

	FOLIAGE_VISIBLE_DISTANCE = (FOLIAGE_AREA_SIZE*FOLIAGE_MAX_RANGE);
	FOLIAGE_TREE_VISIBLE_DISTANCE = (FOLIAGE_AREA_SIZE*FOLIAGE_TREE_MAX_RANGE);

	if (!FOLIAGE_INITIALIZED)
	{// Init/register all foliage models...
		int i = 0;

		if (!strcmp(CURRENT_FOLIAGE_CLIMATE_OPTION, "springpineforest"))
		{
			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				FOLIAGE_PLANT_MODELS[i] = RE_RegisterModel(SpringPlantsModelsList[i]);
			}
		}
		else if (!strcmp(CURRENT_FOLIAGE_CLIMATE_OPTION, "endorredwoodforest"))
		{
			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				FOLIAGE_PLANT_MODELS[i] = RE_RegisterModel(EndorPlantsModelsList[i]);
			}
		}
		else if (!strcmp(CURRENT_FOLIAGE_CLIMATE_OPTION, "snowpineforest"))
		{
			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				FOLIAGE_PLANT_MODELS[i] = RE_RegisterModel(SnowPlantsModelsList[i]);
			}
		}
		else if (!strcmp(CURRENT_FOLIAGE_CLIMATE_OPTION, "tropicalold"))
		{
			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				FOLIAGE_PLANT_MODELS[i] = RE_RegisterModel(TropicalPlantsModelsList[i]);
			}
		}
		else // Default to new tropical...
		{
			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				FOLIAGE_PLANT_MODELS[i] = RE_RegisterModel(TropicalPlantsModelsList[i]);
			}
		}

		// Read all the tree info from the new .climate ini files...
		TREE_SCALE_MULTIPLIER = atof(IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", "treeScaleMultiplier", "1.0"));

		for (i = 0; i < 9; i++)
		{
			FOLIAGE_TREE_MODEL[i] = RE_RegisterModel(IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", va("treeModel%i", i), ""));

			/*if (StringContainsWord(IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", va("treeModel%i", i), ""), ".glm"))
			{// Init G2 instance if this is a GLM...
				trap->G2API_InitGhoul2Model(&FOLIAGE_TREE_G2_MODEL[i], IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", va("treeModel%i", i), ""), 0, 0, 0, 0, 0);
			}*/

			FOLIAGE_TREE_BILLBOARD_SHADER[i] = RE_RegisterShader(IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", va("treeBillboardShader%i", i), ""));
			FOLIAGE_TREE_BILLBOARD_SIZE[i] = atof(IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", va("treeBillboardSize%i", i), "128.0"));
			FOLIAGE_TREE_RADIUS[i] = atof(IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", va("treeRadius%i", i), "24.0"));
			FOLIAGE_TREE_ZOFFSET[i] = atof(IniRead(va("climates/%s.climate", CURRENT_FOLIAGE_CLIMATE_OPTION), "TREES", va("treeZoffset%i", i), "-4.0"));
		}

		FOLIAGE_INITIALIZED = qtrue;
	}

	FOLIAGE_Check_CVar_Change();

	VectorCopy(tr.refdef.vieworg, viewOrg);
	VectorCopy(tr.refdef.viewangles, viewAngles);

	FOLIAGE_Calc_In_Range_Areas();

	if (MAP_HAS_TREES)
	{
		for (CURRENT_AREA = 0; CURRENT_AREA < IN_RANGE_AREAS_LIST_COUNT; CURRENT_AREA++)
		{
			int spot = 0;
			int CURRENT_AREA_ID = IN_RANGE_AREAS_LIST[CURRENT_AREA];

			for (spot = 0; spot < FOLIAGE_AREAS_TREES_LIST_COUNT[CURRENT_AREA_ID]; spot++)
			{// Draw close trees second...
				if (FOLIAGE_TREE_SELECTION[FOLIAGE_AREAS_TREES_LIST[CURRENT_AREA_ID][spot]] > 0)
					FOLIAGE_AddToScreen(FOLIAGE_AREAS_TREES_LIST[CURRENT_AREA_ID][spot], FOLIAGE_PASS_CLOSETREE);
			}
		}

		for (CURRENT_AREA = 0; CURRENT_AREA < IN_RANGE_TREE_AREAS_LIST_COUNT; CURRENT_AREA++)
		{// Draw trees first...
			int spot = 0;
			int CURRENT_AREA_ID = IN_RANGE_TREE_AREAS_LIST[CURRENT_AREA];

			for (spot = 0; spot < FOLIAGE_AREAS_TREES_LIST_COUNT[CURRENT_AREA_ID]; spot++)
			{
				FOLIAGE_AddToScreen(FOLIAGE_AREAS_TREES_LIST[CURRENT_AREA_ID][spot], FOLIAGE_PASS_TREE);
			}
		}
	}

	for (CURRENT_AREA = 0; CURRENT_AREA < IN_RANGE_AREAS_LIST_COUNT; CURRENT_AREA++)
	{// Draw plants last...
		int spot = 0;
		int CURRENT_AREA_ID = IN_RANGE_AREAS_LIST[CURRENT_AREA];

		for (spot = 0; spot < FOLIAGE_AREAS_LIST_COUNT[CURRENT_AREA_ID]; spot++)
		{
			if (FOLIAGE_TREE_SELECTION[FOLIAGE_AREAS_LIST[CURRENT_AREA_ID][spot]] <= 0)
				FOLIAGE_AddToScreen(FOLIAGE_AREAS_LIST[CURRENT_AREA_ID][spot], FOLIAGE_PASS_PLANT);
		}
	}
}

#endif //__RENDERER_GROUND_FOLIAGE__
