#include "../qcommon/q_shared.h"
#include "../cgame/cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/surfaceflags.h"
#include "../qcommon/inifile.h"

extern qboolean InFOV(vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV);
extern void AIMod_GetMapBounts(void);

// =======================================================================================================================================
//
//                                                             Foliage Rendering...
//
// =======================================================================================================================================

#define			FOLIAGE_MAX_FOLIAGES 2097152//4194304

// =======================================================================================================================================
//
// These settings below are here for future adjustment and expansion...
//
// TODO: * Load <climateType>.climate file with all grasses/plants/trees md3's and textures lists.
//       * Add climate selection option to the header of <mapname>.foliage
//
// =======================================================================================================================================

qboolean	FOLIAGE_INITIALIZED = qfalse;

char		CURRENT_CLIMATE_OPTION[256] = { 0 };

float		NUM_TREE_TYPES = 9;

float		NUM_PLANT_SHADERS = 0;

//#define		PLANT_SCALE_MULTIPLIER 1.0

#define		MAX_PLANT_SHADERS 100
#define		MAX_PLANT_MODELS 69

float		TREE_SCALE_MULTIPLIER = 1.0;

float		PLANT_SCALE_MULTIPLIER = 1.0;

float	CUSTOM_FOLIAGE_MAX_DISTANCE = 0.0;
char	CustomFoliageModelsList[69][128] = { 0 };
float	CUSTOM_FOLIAGE_SCALES[69] = { 0.0 };

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
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/smalltree02.md3",
	"models/warzone/plants/smalltree03.md3",
	"models/warzone/plants/smalltree04.md3",
	"models/warzone/plants/smalltree05.md3",
	"models/warzone/plants/smalltree06.md3",
};

static const char *GrassyPlantsModelsList[] = {
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

static const char *ForestPlantsModelsList[] = {
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
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/fern01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern05.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/gcplantmix01.md3",
	// Near trees/walls...
	"models/warzone/plants/azaleashrub01.md3",
	"models/warzone/plants/azaleashrub02.md3",
	"models/warzone/plants/buckthornshrub01.md3",
	"models/warzone/plants/buckthornshrub02.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub04.md3",
	"models/warzone/plants/gorseshrub01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern03.md3",
	"models/warzone/plants/fern04.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
};

static const char *GrassOnlyModelsList[] = {
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
	// Near trees/walls, or normal...
	"models/warzone/plants/gcgrass01.md3",
	// Near trees/walls...
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
	"models/warzone/plants/gcgrass01.3ds",
	"models/warzone/plants/gcgrass01.3ds",
	"models/warzone/plants/gcgrass01.3ds",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
	"models/warzone/plants/gcgrass01.md3",
};

static const char *ForestPlants2ModelsList[] = {
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
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass02.md3",
	"models/warzone/plants/forestgrass02.md3",
	"models/warzone/plants/forestgrass02.md3",
	"models/warzone/plants/forestgrass02.md3",
	"models/warzone/plants/forestgrass03.md3",
	"models/warzone/plants/forestgrass03.md3",
	"models/warzone/plants/forestgrass03.md3",
	"models/warzone/plants/forestgrass03.md3",
	"models/warzone/plants/forestgrass04.md3",
	"models/warzone/plants/forestgrass04.md3",
	"models/warzone/plants/forestgrass04.md3",
	"models/warzone/plants/forestgrass04.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/fern01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern05.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/gcplantmix01.md3",
	// Near trees/walls...
	"models/warzone/plants/azaleashrub01.md3",
	"models/warzone/plants/azaleashrub02.md3",
	"models/warzone/plants/buckthornshrub01.md3",
	"models/warzone/plants/buckthornshrub02.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub04.md3",
	"models/warzone/plants/gorseshrub01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern03.md3",
	"models/warzone/plants/fern04.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
};

static const char *FieldGrassModelsList[] = {
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
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/gcplantmix01.md3",
	// Near trees/walls...
	"models/warzone/plants/azaleashrub01.md3",
	"models/warzone/plants/azaleashrub02.md3",
	"models/warzone/plants/buckthornshrub01.md3",
	"models/warzone/plants/buckthornshrub02.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub04.md3",
	"models/warzone/plants/gorseshrub01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern03.md3",
	"models/warzone/plants/fern04.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
};

static const char *FieldGrassShrubsModelsList[] = {
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
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass02.md3",
	"models/warzone/plants/forestgrass03.md3",
	"models/warzone/plants/forestgrass04.md3",
	"models/warzone/plants/forestgrass05.md3",
	"models/warzone/plants/forestgrass06.md3",
	"models/warzone/plants/forestgrass07.md3",
	"models/warzone/plants/forestgrass08.md3",
	"models/warzone/plants/forestgrass09.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/fern01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern05.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/gcplantmix01.md3",
	// Near trees/walls...
	"models/warzone/plants/azaleashrub01.md3",
	"models/warzone/plants/azaleashrub02.md3",
	"models/warzone/plants/buckthornshrub01.md3",
	"models/warzone/plants/buckthornshrub02.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub04.md3",
	"models/warzone/plants/gorseshrub01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern03.md3",
	"models/warzone/plants/fern04.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
};

static const char *MushroomForestModelsList[] = {
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
	"models/warzone/plants/mushroom01.md3",
	"models/warzone/plants/mushroom02.md3",
	"models/warzone/plants/mushroom03.md3",
	"models/warzone/plants/mushroom01.md3",
	"models/warzone/plants/mushroom02.md3",
	"models/warzone/plants/mushroom03.md3",
	"models/warzone/plants/mushroom01.md3",
	"models/warzone/plants/mushroom03.md3",
	"models/warzone/plants/mushroom03.md3",
	"models/warzone/plants/mushroom01.md3",
	"models/warzone/plants/mushroom02.md3",
	"models/warzone/plants/mushroom03.md3",
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass02.md3",
	"models/warzone/plants/forestgrass03.md3",
	"models/warzone/plants/forestgrass04.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/fern01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern05.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/gcplantmix01.md3",
	// Near trees/walls...
	"models/warzone/plants/azaleashrub01.md3",
	"models/warzone/plants/azaleashrub02.md3",
	"models/warzone/plants/buckthornshrub01.md3",
	"models/warzone/plants/buckthornshrub02.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub03.md3",
	"models/warzone/plants/buckthornshrub04.md3",
	"models/warzone/plants/gorseshrub01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern03.md3",
	"models/warzone/plants/fern04.md3",
	"models/warzone/plants/fern05.md3",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/gcplantmix01.md3",
	"models/warzone/plants/gcplantmix02.md3",
	"models/warzone/plants/gcplantmix03.md3",
	"models/warzone/plants/gcplantmix04.md3",
	"models/warzone/plants/gcplantmix05.md3",
	"models/warzone/plants/gcplantmix06.md3",
};

static const char *MushroomForest2ModelsList[] = {
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
	"models/warzone/plants/mushroom01.md3",
	"models/warzone/plants/mushroom02.md3",
	"models/warzone/plants/mushroom03.md3",
	"models/warzone/plants/mushroom01.md3",
	"models/warzone/plants/mushroom02.md3",
	"models/warzone/plants/mushroom03.md3",
	"models/warzone/plants/flowergrass01.md3",
	"models/warzone/plants/flowergrass02.md3",
	"models/warzone/plants/flowergrass03.md3",
	"models/warzone/plants/flowergrass04.md3",
	"models/warzone/plants/flowergrass05.md3",
	"models/warzone/plants/forestgrass01.md3",
	"models/warzone/plants/forestgrass02.md3",
	"models/warzone/plants/forestgrass03.md3",
	"models/warzone/plants/forestgrass04.md3",
	"models/warzone/plants/fern01.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern02.md3",
	"models/warzone/plants/fern05.md3",
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
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/fernplants01.3ds",
	"models/warzone/plants/smalltree02.md3",
	"models/warzone/plants/smalltree03.md3",
	"models/warzone/plants/smalltree04.md3",
	"models/warzone/plants/smalltree05.md3",
	"models/warzone/plants/smalltree06.md3",
};

static const char *FernsModelsList[] = {
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	// Near trees/walls, or normal...
	"models/warzone/plants/newfern01.md3",
	// Near trees/walls...
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern01.md3",
	"models/warzone/plants/newfern05.md3",
	"models/warzone/plants/newfern05.md3",
	"models/warzone/plants/newfern05.md3",
	"models/warzone/plants/newfern05.md3",
	"models/warzone/plants/newfern04.md3",
	"models/warzone/plants/newfern04.md3",
	"models/warzone/plants/newfern04.md3",
	"models/warzone/plants/newfern04.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
	"models/warzone/plants/newfern02.md3",
};

// =======================================================================================================================================
//
// CVAR related defines...
//
// =======================================================================================================================================

#define			__FOLIAGE_DENSITY__ cg_foliageDensity.value

#define		FOLIAGE_AREA_SIZE 					512
int			FOLIAGE_VISIBLE_DISTANCE = -1;
int			FOLIAGE_PLANT_VISIBLE_DISTANCE = -1;
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
vec3_t		*FOLIAGE_POSITIONS = NULL;
vec3_t		*FOLIAGE_NORMALS = NULL;
int			*FOLIAGE_PLANT_SELECTION = NULL;
float		*FOLIAGE_PLANT_ANGLE = NULL;
vec3_t		*FOLIAGE_PLANT_ANGLES = NULL;
matrix3_t	*FOLIAGE_PLANT_AXIS = NULL;
float		*FOLIAGE_PLANT_SCALE = NULL;
int			*FOLIAGE_TREE_SELECTION = NULL;
vec3_t		*FOLIAGE_TREE_ANGLES = NULL;
float		*FOLIAGE_TREE_ANGLE = NULL;
matrix3_t	*FOLIAGE_TREE_AXIS = NULL;
vec3_t		*FOLIAGE_TREE_BILLBOARD_ANGLES = NULL;
matrix3_t	*FOLIAGE_TREE_BILLBOARD_AXIS = NULL;
float		*FOLIAGE_TREE_SCALE = NULL;


int			FOLIAGE_AREAS_COUNT = 0;

typedef int		ivec256_t[FOLIAGE_AREA_MAX_FOLIAGES];

int			*FOLIAGE_AREAS_LIST_COUNT = NULL;
ivec256_t	*FOLIAGE_AREAS_LIST = NULL;
int			*FOLIAGE_AREAS_TREES_LIST_COUNT = NULL;
int			*FOLIAGE_AREAS_TREES_VISCHECK_TIME = NULL;
qboolean	*FOLIAGE_AREAS_TREES_VISCHECK_RESULT = NULL;
ivec256_t	*FOLIAGE_AREAS_TREES_LIST = NULL;
vec3_t		*FOLIAGE_AREAS_MINS = NULL;
vec3_t		*FOLIAGE_AREAS_MAXS = NULL;


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
int			IN_RANGE_TREE_AREAS_LIST_COUNT = 0;
int			*IN_RANGE_AREAS_LIST = NULL;
float		*IN_RANGE_AREAS_DISTANCE = NULL;
int			*IN_RANGE_TREE_AREAS_LIST = NULL;
float		*IN_RANGE_TREE_AREAS_DISTANCE = NULL;

#define FOLIAGE_SOLID_TREES_MAX 4
int			FOLIAGE_SOLID_TREES[FOLIAGE_SOLID_TREES_MAX];
float		FOLIAGE_SOLID_TREES_DIST[FOLIAGE_SOLID_TREES_MAX];

float		OLD_FOLIAGE_DENSITY = 64.0;

qboolean	MAP_HAS_TREES = qfalse;

// =======================================================================================================================================
//
// Roads Map System...
//
// =======================================================================================================================================
#include "../rd-warzone/TinyImageLoader/TinyImageLoader.h"

extern qboolean BG_FileExists(const char *fileName);

char *R_TIL_TextureFileExistsFull(const char *name)
{
	if (!name || !name[0] || name[0] == '\0' || strlen(name) < 1) return NULL;

	char texName[512] = { 0 };
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.png", name);

	if (BG_FileExists(texName))
	{
		return "png";
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.tga", name);

	if (BG_FileExists(texName))
	{
		return "tga";
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.jpg", name);

	if (BG_FileExists(texName))
	{
		return "jpg";
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.dds", name);

	if (BG_FileExists(texName))
	{
		return "dds";
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.gif", name);

	if (BG_FileExists(texName))
	{
		return "gif";
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.bmp", name);

	if (BG_FileExists(texName))
	{
		return "bmp";
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.ico", name);

	if (BG_FileExists(texName))
	{
		return "ico";
	}

	return NULL;
}

#define PixelCopy(a,b) ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define srgb_to_linear(c) (((c) <= 0.04045) ? (c) * (1.0 / 12.92) : pow(((c) + 0.055f)*(1.0/1.055), 2.4))

qboolean RadSampleImage(uint8_t *pixels, int width, int height, float st[2], float color[4])
{
	qboolean texturesRGB = qfalse;
	float	sto[2];
	int		x, y;

	/* clear color first */
	color[0] = color[1] = color[2] = color[3] = 0;

	/* dummy check */
	if (pixels == NULL || width < 1 || height < 1)
		return qfalse;

#if 1
	/* bias st */
	sto[0] = st[0];
	while (sto[0] < 0.0f)
		sto[0] += 1.0f;
	sto[1] = st[1];
	while (sto[1] < 0.0f)
		sto[1] += 1.0f;

	/* get offsets */
	x = ((float)width * sto[0]);// +0.5f;
	x %= width;
	y = ((float)height * sto[1]);// +0.5f;
	y %= height;
#else
	x = st[0];
	x *= width;
	y = st[1];
	y *= height;
#endif

	/* get pixel */
	pixels += (y * width * 4) + (x * 4);
	PixelCopy(pixels, color);
	color[3] = pixels[3];

	return qtrue;
}

qboolean TIL_INITIALIZED = qfalse;

qboolean ROAD_MAP_INITIALIZED = qfalse;
til::Image *ROAD_MAP = NULL;

void FOLAGE_LoadRoadImage(void)
{
	if (!ROAD_MAP && !ROAD_MAP_INITIALIZED)
	{
		ROAD_MAP_INITIALIZED = qtrue;

		char name[512] = { 0 };
		strcpy(name, va("maps/%s_roads", cgs.currentmapname));
		char *ext = R_TIL_TextureFileExistsFull(name);

		/*if (ext)
		{
			trap->Print("Found %s.%s\n", name, ext);
		}
		else
		{
			trap->Print("Not found %s.\n", name);
		}*/

		if (ext)
		{
			if (!TIL_INITIALIZED)
			{
				til::TIL_Init();
				TIL_INITIALIZED = qtrue;
			}

			char fullPath[1024] = { 0 };
			sprintf_s(fullPath, "warzone/%s.%s", name, ext);
			ROAD_MAP = til::TIL_Load(fullPath/*, TIL_FILE_ADDWORKINGDIR*/);
			
			if (ROAD_MAP && ROAD_MAP->GetHeight() > 0 && ROAD_MAP->GetWidth() > 0)
			{
				//trap->Print("TIL: Loaded image %s. Size %i x %i.\n", fullPath, ROAD_MAP->GetWidth(), ROAD_MAP->GetHeight());
			}
			else
			{
				//trap->Print("TIL: Clould not load image %s.\n", fullPath);
				til::TIL_Release(ROAD_MAP);
				ROAD_MAP = NULL;
			}
		}
	}
}

qboolean RoadExistsAtPoint(vec3_t point)
{
	if (!ROAD_MAP)
	{
		FOLAGE_LoadRoadImage();
		
		if (!ROAD_MAP)
		{
			return qfalse;
		}
	}

	if (!cg.mapcoordsValid)
	{
		AIMod_GetMapBounts();

		if (!cg.mapcoordsValid)
		{
			return qfalse;
		}
	}

	float mapSize[2];
	float pixel[2];

	mapSize[0] = cg.mapcoordsMaxs[0] - cg.mapcoordsMins[0];
	mapSize[1] = cg.mapcoordsMaxs[1] - cg.mapcoordsMins[1];
	pixel[0] = (point[0] - cg.mapcoordsMins[0]) / mapSize[0];
	pixel[1] = (point[1] - cg.mapcoordsMins[1]) / mapSize[1];

	vec4_t color;
	RadSampleImage((uint8_t *)ROAD_MAP->GetPixels(), ROAD_MAP->GetWidth(), ROAD_MAP->GetHeight(), pixel, color);
	
	float road = color[2];

	if (road > 0)
	{
		return qtrue;
	}

#if 1
#define scanWidth 3
	// Also scan pixels around this position...
	for (int x = -scanWidth; x <= scanWidth; x++)
	{
		for (int y = -scanWidth; y <= scanWidth; y++)
		{
			if (x == 0 && y == 0) continue; // Already checked this one...

			float pixel2[2];
			pixel2[0] = pixel[0] + (x / (float)ROAD_MAP->GetWidth());
			pixel2[1] = pixel[1] + (y / (float)ROAD_MAP->GetHeight());

			if (pixel2[0] >= 0 && pixel2[0] <= 1.0 && pixel2[1] >= 0 && pixel2[1] <= 1.0)
			{
				vec4_t color;
				RadSampleImage(ROAD_MAP->GetPixels(), ROAD_MAP->GetWidth(), ROAD_MAP->GetHeight(), pixel2, color);
				float road2 = color[0];

				if (road2 > 0)
				{
					return qtrue;
				}
			}
		}
	}
#endif

	return qfalse;
}

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

	trap->FS_Open(va("foliage/%s.foliageAreas", cgs.currentmapname), &f, FS_READ);

	if (!f)
	{
		return qfalse;
	}

	trap->FS_Read(&version, sizeof(int), f);

	if (version != FOLIAGE_AREA_FILE_VERSION)
	{// Old version... Update...
		trap->FS_Close(f);
		return qfalse;
	}

	trap->FS_Read(&numPositions, sizeof(int), f);

	if (numPositions != FOLIAGE_NUM_POSITIONS)
	{// Mismatch... Regenerate...
		trap->FS_Close(f);
		return qfalse;
	}

	trap->FS_Read(&FOLIAGE_AREAS_COUNT, sizeof(int), f);

	for (i = 0; i < FOLIAGE_AREAS_COUNT; i++)
	{
		trap->FS_Read(&FOLIAGE_AREAS_MINS[i], sizeof(vec3_t), f);
		trap->FS_Read(&FOLIAGE_AREAS_MAXS[i], sizeof(vec3_t), f);

		trap->FS_Read(&FOLIAGE_AREAS_LIST_COUNT[i], sizeof(int), f);
		trap->FS_Read(&FOLIAGE_AREAS_LIST[i], sizeof(int)*FOLIAGE_AREAS_LIST_COUNT[i], f);

		if (MAP_HAS_TREES)
		{
			trap->FS_Read(&FOLIAGE_AREAS_TREES_LIST_COUNT[i], sizeof(int), f);
			trap->FS_Read(&FOLIAGE_AREAS_TREES_LIST[i], sizeof(int)*FOLIAGE_AREAS_TREES_LIST_COUNT[i], f);
		}
		else
		{
			int unused;
			int unusedArray[FOLIAGE_AREA_MAX_FOLIAGES];
			trap->FS_Read(&unused, sizeof(int), f);
			trap->FS_Read(&unusedArray, sizeof(int)*unused, f);
		}
	}

	trap->FS_Close(f);

	trap->Print("^1*** ^3%s^5: Successfully loaded %i foliageAreas to foliageArea file ^7foliage/%s.foliageAreas^5.\n", "AUTO-FOLIAGE", FOLIAGE_AREAS_COUNT, cgs.currentmapname);

	return qtrue;
}

void FOLIAGE_SaveFoliageAreas(void)
{
	fileHandle_t	f;
	int				i = 0;

	trap->FS_Open(va("foliage/%s.foliageAreas", cgs.currentmapname), &f, FS_WRITE);

	if (!f)
	{
		trap->Print("^1*** ^3%s^5: Failed to save foliageAreas file ^7foliage/%s.foliageAreas^5 for save.\n", "AUTO-FOLIAGE", cgs.currentmapname);
		return;
	}

	trap->FS_Write(&FOLIAGE_AREA_FILE_VERSION, sizeof(int), f);

	trap->FS_Write(&FOLIAGE_NUM_POSITIONS, sizeof(int), f);

	trap->FS_Write(&FOLIAGE_AREAS_COUNT, sizeof(int), f);

	for (i = 0; i < FOLIAGE_AREAS_COUNT; i++)
	{
		trap->FS_Write(&FOLIAGE_AREAS_MINS[i], sizeof(vec3_t), f);
		trap->FS_Write(&FOLIAGE_AREAS_MAXS[i], sizeof(vec3_t), f);

		trap->FS_Write(&FOLIAGE_AREAS_LIST_COUNT[i], sizeof(int), f);
		trap->FS_Write(&FOLIAGE_AREAS_LIST[i], sizeof(int)*FOLIAGE_AREAS_LIST_COUNT[i], f);

		if (MAP_HAS_TREES)
		{
			trap->FS_Write(&FOLIAGE_AREAS_TREES_LIST_COUNT[i], sizeof(int), f);
			trap->FS_Write(&FOLIAGE_AREAS_TREES_LIST[i], sizeof(int)*FOLIAGE_AREAS_TREES_LIST_COUNT[i], f);
		}
		else
		{
			int unused = 0;
			int unusedArray[FOLIAGE_AREA_MAX_FOLIAGES] = { 0 };
			trap->FS_Write(&unused, sizeof(int), f);
			trap->FS_Write(&unusedArray, sizeof(int)*unused, f);
		}
	}

	trap->FS_Close(f);

	trap->Print("^1*** ^3%s^5: Successfully saved %i foliageAreas to foliageArea file ^7foliage/%s.foliageAreas^5.\n", "AUTO-FOLIAGE", FOLIAGE_AREAS_COUNT, cgs.currentmapname);
}

void FOLIAGE_Setup_Foliage_Areas(void)
{
	int		DENSITY_REMOVED = 0;
	int		ZERO_SCALE_REMOVED = 0;
	int		areaNum = 0, i = 0;
	vec3_t	mins, maxs, mapMins, mapMaxs;

	FOLAGE_LoadRoadImage();

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
		
		if (MAP_HAS_TREES)
		{
			FOLIAGE_AREAS_TREES_LIST_COUNT[areaNum] = 0;
			FOLIAGE_AREAS_TREES_VISCHECK_TIME[areaNum] = 0;
			FOLIAGE_AREAS_TREES_VISCHECK_RESULT[areaNum] = qtrue;
		}

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
						//trap->Print("*** Area %i has more then %i foliages ***\n", areaNum, (int)FOLIAGE_AREA_MAX_FOLIAGES);
						break;
					}

					if (!MAP_HAS_TREES || FOLIAGE_TREE_SELECTION[i] <= 0)
					{// Never remove trees...
						int j = 0;

						if (FOLIAGE_PLANT_SCALE[i] <= 0)
						{// Zero scale plant... Remove...
							ZERO_SCALE_REMOVED++;
							continue;
						}

						for (j = 0; j < FOLIAGE_AREAS_LIST_COUNT[areaNum]; j++)
						{// Let's use a density setting to improve FPS...
							if (DistanceHorizontal(FOLIAGE_POSITIONS[i], FOLIAGE_POSITIONS[FOLIAGE_AREAS_LIST[areaNum][j]]) < __FOLIAGE_DENSITY__ * FOLIAGE_PLANT_SCALE[i])
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
	OLD_FOLIAGE_DENSITY = __FOLIAGE_DENSITY__;

	trap->Print("Generated %i foliage areas. %i used of %i total foliages. %i removed by density setting. %i removed due to zero scale.\n", FOLIAGE_AREAS_COUNT, FOLIAGE_NUM_POSITIONS - DENSITY_REMOVED, FOLIAGE_NUM_POSITIONS, DENSITY_REMOVED, ZERO_SCALE_REMOVED);

	// Save for future use...
	FOLIAGE_SaveFoliageAreas();
}

void FOLIAGE_Check_CVar_Change(void)
{
	if (__FOLIAGE_DENSITY__ != OLD_FOLIAGE_DENSITY)
	{
		FOLIAGE_Setup_Foliage_Areas();
	}
}

qboolean FOLIAGE_Box_In_FOV(vec3_t mins, vec3_t maxs, int areaNum, float minsDist, float maxsDist)
{
	vec3_t mins2, maxs2, edge, edge2;
	trace_t tr;

	if (!cg_foliageAreaFOVCheck.integer) return qtrue;

	VectorSet(mins2, mins[0], mins[1], cg.refdef.vieworg[2]);
	VectorSet(maxs2, maxs[0], maxs[1], cg.refdef.vieworg[2]);
	VectorSet(edge, maxs2[0], mins2[1], maxs2[2]);
	VectorSet(edge2, mins2[0], maxs2[1], maxs2[2]);

	if (InFOV(mins2, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/)
		|| InFOV(maxs2, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/)
		|| InFOV(edge, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/)
		|| InFOV(edge2, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/))
	{
		if (MAP_HAS_TREES && cg_foliageAreaVisCheck.integer
			&& minsDist > 8192.0
			&& maxsDist > 8192.0
			&& FOLIAGE_AREAS_TREES_VISCHECK_TIME[areaNum] + 2000 < trap->Milliseconds())
		{// Also vis check distant trees, at 2048 above... (but allow hits of SURF_SKY in case the roof is low)
			vec3_t visMins, visMaxs, viewPos;
			VectorSet(visMins, mins2[0], mins2[1], mins2[2] + 2048.0);
			VectorSet(visMaxs, maxs2[0], maxs2[1], maxs2[2] + 2048.0);
			VectorSet(viewPos, cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2] + 64.0);

			FOLIAGE_AREAS_TREES_VISCHECK_TIME[areaNum] = trap->Milliseconds();

			CG_Trace(&tr, viewPos, NULL, NULL, visMins, cg.clientNum, MASK_SOLID);

			if (tr.fraction >= 1.0 || (tr.surfaceFlags & SURF_SKY))
			{
				FOLIAGE_AREAS_TREES_VISCHECK_RESULT[areaNum] = qtrue;
				return qtrue;
			}

			CG_Trace(&tr, viewPos, NULL, NULL, visMaxs, cg.clientNum, MASK_SOLID);

			if (tr.fraction >= 1.0 || (tr.surfaceFlags & SURF_SKY))
			{
				FOLIAGE_AREAS_TREES_VISCHECK_RESULT[areaNum] = qtrue;
				return qtrue;
			}

			FOLIAGE_AREAS_TREES_VISCHECK_RESULT[areaNum] = qfalse;
			return qfalse;
		}
		else
		{
			if (MAP_HAS_TREES) FOLIAGE_AREAS_TREES_VISCHECK_RESULT[areaNum] = qtrue;
			return qtrue;
		}
	}

	return qfalse;
}

qboolean FOLIAGE_In_FOV(vec3_t mins, vec3_t maxs)
{
	vec3_t edge, edge2;

	VectorSet(edge, maxs[0], mins[1], maxs[2]);
	VectorSet(edge2, mins[0], maxs[1], maxs[2]);

	// FIXME: Go back to 180 Y axis later, if we can boost the speed of the system somewhere else. Using FOV_Y
	//        causes some issues when looking down at an area (something about InFOV seems to not like going
	//        under 0 and looping back to 360?)
	if (!InFOV(mins, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/)
		&& !InFOV(maxs, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/)
		&& !InFOV(edge, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/)
		&& !InFOV(edge2, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x, cg.refdef.fov_y/*180*/))
		return qfalse;

	return qtrue;
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


void FOLIAGE_Calc_In_Range_Areas(void)
{
	if (FOLIAGE_AREAS_COUNT <= 0)
	{
		IN_RANGE_AREAS_LIST_COUNT = 0;
		IN_RANGE_TREE_AREAS_LIST_COUNT = 0;
		return;
	}

	int i = 0;

	FOLIAGE_VISIBLE_DISTANCE = (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0) ? CUSTOM_FOLIAGE_MAX_DISTANCE : FOLIAGE_AREA_SIZE*cg_foliageGrassRangeMult.value;
	FOLIAGE_TREE_VISIBLE_DISTANCE = (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0) ? CUSTOM_FOLIAGE_MAX_DISTANCE : FOLIAGE_AREA_SIZE*cg_foliageTreeRangeMult.value;

	if (MAP_HAS_TREES)
	{
		for (i = 0; i < FOLIAGE_SOLID_TREES_MAX; i++)
		{
			FOLIAGE_SOLID_TREES[i] = -1;
			FOLIAGE_SOLID_TREES_DIST[i] = 131072.0;
		}

		if (cg_foliageTreeRangeMult.value < 8.0)
		{
			FOLIAGE_TREE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*8.0;
			trap->Cvar_Set("cg_foliageTreeRangeMult", "8.0");
			trap->Print("WARNING: Minimum tree range multiplier is 8.0. Cvar has been changed.\n");
		}
	}

	if (Distance(cg.refdef.vieworg, LAST_ORG) > 128.0 || (cg_foliageAreaFOVCheck.integer && Distance(cg.refdef.viewangles, LAST_ANG) > 0.0))//50.0)
	{// Update in range list...
		int i = 0;

		VectorCopy(cg.refdef.vieworg, LAST_ORG);
		VectorCopy(cg.refdef.viewangles, LAST_ANG);

		// Calculate currently-in-range areas to use...
		IN_RANGE_AREAS_LIST_COUNT = 0;
		IN_RANGE_TREE_AREAS_LIST_COUNT = 0;

		for (i = 0; i < FOLIAGE_AREAS_COUNT; i++)
		{
			float minsDist = DistanceHorizontal(FOLIAGE_AREAS_MINS[i], cg.refdef.vieworg);
			float maxsDist = DistanceHorizontal(FOLIAGE_AREAS_MAXS[i], cg.refdef.vieworg);

			if (minsDist < FOLIAGE_VISIBLE_DISTANCE
				|| maxsDist < FOLIAGE_VISIBLE_DISTANCE)
			{
				qboolean inFOV = qtrue;
				qboolean isClose = qtrue;

				if (cg_foliageAreaFOVCheck.integer)
				{
					if (minsDist > FOLIAGE_AREA_SIZE && maxsDist > FOLIAGE_AREA_SIZE)
					{
						isClose = qfalse;
					}

					if (!isClose)
					{
						inFOV = FOLIAGE_Box_In_FOV(FOLIAGE_AREAS_MINS[i], FOLIAGE_AREAS_MAXS[i], i, minsDist, maxsDist);
					}
				}

				if (isClose || inFOV || (cg_foliageAreaFOVCheck.integer && (minsDist <= FOLIAGE_AREA_SIZE * 2.0 || maxsDist <= FOLIAGE_AREA_SIZE * 2.0)))
				{
					if (MAP_HAS_TREES && !isClose && !inFOV)
					{// Not in our FOV, but close enough that we need the trees. Add to tree list instead, so we can skip grass/plant checking...
						IN_RANGE_TREE_AREAS_LIST[IN_RANGE_TREE_AREAS_LIST_COUNT] = i;

						if (minsDist < maxsDist)
							IN_RANGE_TREE_AREAS_DISTANCE[IN_RANGE_TREE_AREAS_LIST_COUNT] = minsDist;
						else
							IN_RANGE_TREE_AREAS_DISTANCE[IN_RANGE_TREE_AREAS_LIST_COUNT] = maxsDist;

						IN_RANGE_TREE_AREAS_LIST_COUNT++;
					}
					else if (isClose || inFOV)
					{// In our FOV, or really close. We need all plants and trees...
						IN_RANGE_AREAS_LIST[IN_RANGE_AREAS_LIST_COUNT] = i;

						if (minsDist < maxsDist)
							IN_RANGE_AREAS_DISTANCE[IN_RANGE_AREAS_LIST_COUNT] = minsDist;
						else
							IN_RANGE_AREAS_DISTANCE[IN_RANGE_AREAS_LIST_COUNT] = maxsDist;

						IN_RANGE_AREAS_LIST_COUNT++;
					}
				}
			}
			else if (MAP_HAS_TREES
				&& (minsDist < FOLIAGE_TREE_VISIBLE_DISTANCE || maxsDist < FOLIAGE_TREE_VISIBLE_DISTANCE))
			{
				if (FOLIAGE_Box_In_FOV(FOLIAGE_AREAS_MINS[i], FOLIAGE_AREAS_MAXS[i], i, minsDist, maxsDist))
				{
					IN_RANGE_TREE_AREAS_LIST[IN_RANGE_TREE_AREAS_LIST_COUNT] = i;

					if (minsDist < maxsDist)
						IN_RANGE_TREE_AREAS_DISTANCE[IN_RANGE_TREE_AREAS_LIST_COUNT] = minsDist;
					else
						IN_RANGE_TREE_AREAS_DISTANCE[IN_RANGE_TREE_AREAS_LIST_COUNT] = maxsDist;

					IN_RANGE_TREE_AREAS_LIST_COUNT++;
				}
			}
		}

		if (cg_foliageAreaSorting.integer)
		{
			FOLIAGE_VisibleAreaSortGrass();
			FOLIAGE_VisibleAreaSortTrees();

			/*for (int i = 0; i < IN_RANGE_AREAS_LIST_COUNT; i++)
			{
			trap->Print("[%i] dist %f.\n", i, IN_RANGE_AREAS_DISTANCE[i]);
			}*/
		}

		//trap->Print("There are %i foliage areas in range. %i tree areas.\n", IN_RANGE_AREAS_LIST_COUNT, IN_RANGE_TREE_AREAS_LIST_COUNT);
	}
}

// =======================================================================================================================================
//
// Collision detection... Only really needed for AWP on client, but there is a CVAR to turn client side predicion on as well.
//
// =======================================================================================================================================

qboolean FOLIAGE_TreeSolidBlocking_AWP(vec3_t moveOrg)
{
	if (MAP_HAS_TREES)
	{
		int areaNum = 0;
		int areaListPos = 0;
		int	CLOSE_AREA_LIST[8192];
		int	CLOSE_AREA_LIST_COUNT = 0;

		for (areaNum = 0; areaNum < FOLIAGE_AREAS_COUNT; areaNum++)
		{
			float DIST = DistanceHorizontal(FOLIAGE_AREAS_MINS[areaNum], moveOrg);
			float DIST2 = DistanceHorizontal(FOLIAGE_AREAS_MAXS[areaNum], moveOrg);

			if (DIST < FOLIAGE_AREA_SIZE * 2.0 || DIST2 < FOLIAGE_AREA_SIZE * 2.0)
			{
				CLOSE_AREA_LIST[CLOSE_AREA_LIST_COUNT] = areaNum;
				CLOSE_AREA_LIST_COUNT++;
			}
		}

		for (areaListPos = 0; areaListPos < CLOSE_AREA_LIST_COUNT; areaListPos++)
		{
			int treeNum = 0;
			int areaNum = CLOSE_AREA_LIST[areaListPos];

			for (treeNum = 0; treeNum < FOLIAGE_AREAS_TREES_LIST_COUNT[areaNum]; treeNum++)
			{
				int		THIS_TREE_NUM = FOLIAGE_AREAS_TREES_LIST[areaNum][treeNum];
				int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] - 1;
				float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[THIS_TREE_NUM] * TREE_SCALE_MULTIPLIER;
				float	DIST = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], moveOrg);

				TREE_RADIUS += 64.0; // Extra space around the tree for player body to fit as well...

				if (FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] > 0 && DIST <= TREE_RADIUS)
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

qboolean FOLIAGE_TreeSolidBlocking_AWP_Path(vec3_t from, vec3_t to)
{
	if (MAP_HAS_TREES)
	{
		int areaNum = 0;
		int areaListPos = 0;
		int	CLOSE_AREA_LIST[8192];
		int	CLOSE_AREA_LIST_COUNT = 0;
		vec3_t dir, angles, forward;

		float fullDist = DistanceHorizontal(from, to);

		for (areaNum = 0; areaNum < FOLIAGE_AREAS_COUNT; areaNum++)
		{
			float DIST = DistanceHorizontal(FOLIAGE_AREAS_MINS[areaNum], to);
			float DIST2 = DistanceHorizontal(FOLIAGE_AREAS_MAXS[areaNum], to);

			if (DIST < FOLIAGE_AREA_SIZE * 2.0 || DIST2 < FOLIAGE_AREA_SIZE * 2.0)
			{
				CLOSE_AREA_LIST[CLOSE_AREA_LIST_COUNT] = areaNum;
				CLOSE_AREA_LIST_COUNT++;
			}
		}

		VectorSubtract(to, from, dir);
		vectoangles(dir, angles);
		AngleVectors(angles, forward, NULL, NULL);

		for (areaListPos = 0; areaListPos < CLOSE_AREA_LIST_COUNT; areaListPos++)
		{
			int treeNum = 0;
			int test = 8;
			int areaNum = CLOSE_AREA_LIST[areaListPos];

			for (treeNum = 0; treeNum < FOLIAGE_AREAS_TREES_LIST_COUNT[areaNum]; treeNum++)
			{
				int		THIS_TREE_NUM = FOLIAGE_AREAS_TREES_LIST[areaNum][treeNum];
				int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] - 1;
				float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[THIS_TREE_NUM] * TREE_SCALE_MULTIPLIER;
				float	DIST = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], from);

				if (FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] <= 0) continue;
				if (fullDist < DIST) continue;

				TREE_RADIUS += 64.0; // Extra space around the tree for player body to fit as well...

				// Check at positions along this path...
				for (test = 8; test < DIST; test += 8)
				{
					vec3_t pos;
					float DIST2;

					VectorMA(from, test, forward, pos);

					DIST2 = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], pos);

					if (DIST2 <= TREE_RADIUS)
					{
						return qtrue;
					}
				}
			}
		}
	}

	return qfalse;
}

qboolean FOLIAGE_TreeSolidBlocking(vec3_t moveOrg)
{
	if (MAP_HAS_TREES)
	{
		int tree = 0;

		if (cg.predictedPlayerState.pm_type == PM_SPECTATOR) return qfalse;
		if (cgs.clientinfo[cg.clientNum].team == FACTION_SPECTATOR) return qfalse;

		for (tree = 0; tree < FOLIAGE_SOLID_TREES_MAX; tree++)
		{
			if (FOLIAGE_SOLID_TREES[tree] >= 0)
			{
				int		THIS_TREE_NUM = FOLIAGE_SOLID_TREES[tree];
				int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] - 1;
				float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[THIS_TREE_NUM] * TREE_SCALE_MULTIPLIER;
				float	TREE_HEIGHT = FOLIAGE_TREE_SCALE[THIS_TREE_NUM] * 2.5*FOLIAGE_TREE_BILLBOARD_SIZE[FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] - 1] * TREE_SCALE_MULTIPLIER;
				float	DIST = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], moveOrg);
				float	hDist = 0;

				if (cg.renderingThirdPerson)
					hDist = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], cg_entities[cg.clientNum].lerpOrigin);
				else
					hDist = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], cg.refdef.vieworg);

				if (DIST <= TREE_RADIUS
					&& (moveOrg[2] >= FOLIAGE_POSITIONS[THIS_TREE_NUM][2] && moveOrg[2] <= FOLIAGE_POSITIONS[THIS_TREE_NUM][2] + (TREE_HEIGHT*2.0))
					&& DIST < hDist				// Move pos would be closer
					&& hDist > TREE_RADIUS)		// Not already stuck in tree...
				{
					//trap->Print("CLIENT: Blocked by tree %i. Radius %f. Distance %f. Type %i.\n", tree, TREE_RADIUS, DIST, THIS_TREE_TYPE);
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

// =======================================================================================================================================
//
// Actual foliage drawing code...
//
// =======================================================================================================================================

void FOLIAGE_AddFoliageEntityToScene(refEntity_t *ent)
{
	AddRefEntityToScene(ent);
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
		trap->Print("Error:  Bad rotType %i given to ApplyAxisRotation\n", rotType);
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

void FOLIAGE_AddToScreen(int num, int passType) {
	refEntity_t		re;
	float			dist = 0;
	float			distFadeScale = 1.5;
	float			minFoliageScale = cg_foliageMinFoliageScale.value;

	if ((!MAP_HAS_TREES || FOLIAGE_TREE_SELECTION[num] <= 0) && FOLIAGE_PLANT_SCALE[num] < minFoliageScale) return;

#if 0
	if (cg.renderingThirdPerson)
		dist = Distance/*Horizontal*/(FOLIAGE_POSITIONS[num], cg_entities[cg.clientNum].lerpOrigin);
	else
#endif
		dist = Distance/*Horizontal*/(FOLIAGE_POSITIONS[num], cg.refdef.vieworg);

	if (MAP_HAS_TREES)
	{// Cull anything in the area outside of cvar specified radius...
		if (passType == FOLIAGE_PASS_TREE && dist > FOLIAGE_TREE_VISIBLE_DISTANCE) return;
		if (passType != FOLIAGE_PASS_TREE && passType != FOLIAGE_PASS_CLOSETREE && dist > FOLIAGE_PLANT_VISIBLE_DISTANCE && FOLIAGE_TREE_SELECTION[num] <= 0) return;
	}

	memset(&re, 0, sizeof(re));

	//re.renderfx |= RF_FORCE_ENT_ALPHA;
	//re.renderfx |= RF_ALPHA_DEPTH;

	VectorCopy(FOLIAGE_POSITIONS[num], re.origin);

	FOLIAGE_VISIBLE_DISTANCE = (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0) ? CUSTOM_FOLIAGE_MAX_DISTANCE : FOLIAGE_AREA_SIZE*cg_foliageGrassRangeMult.value;
	FOLIAGE_PLANT_VISIBLE_DISTANCE = FOLIAGE_VISIBLE_DISTANCE;//FOLIAGE_AREA_SIZE*cg_foliagePlantRangeMult.value;
	FOLIAGE_TREE_VISIBLE_DISTANCE = (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0) ? CUSTOM_FOLIAGE_MAX_DISTANCE : FOLIAGE_AREA_SIZE*cg_foliageTreeRangeMult.value;

	if (dist <= FOLIAGE_PLANT_VISIBLE_DISTANCE)
	{// Draw grass...
		qboolean skipGrass = qfalse;
		qboolean skipPlant = qfalse;
		float minGrassScale = ((dist / FOLIAGE_PLANT_VISIBLE_DISTANCE) * 0.7) + 0.3;

		if (dist > FOLIAGE_VISIBLE_DISTANCE) skipGrass = qtrue;
		if (FOLIAGE_PLANT_SCALE[num] <= minGrassScale && dist > FOLIAGE_VISIBLE_DISTANCE) skipPlant = qtrue;
		if (FOLIAGE_PLANT_SCALE[num] < minFoliageScale) skipPlant = qtrue;

		if (passType == FOLIAGE_PASS_PLANT && !skipPlant && FOLIAGE_PLANT_SELECTION[num] > 0)
		{// Add plant model as well...
			float distMult = Q_clamp(0.0, 1.0 - Q_clamp(0.0, dist / ((float)FOLIAGE_VISIBLE_DISTANCE * 0.7), 1.0), 1.0);

			if (distMult <= 0.0) return;

			float customScale = (CUSTOM_FOLIAGE_SCALES[FOLIAGE_PLANT_SELECTION[num]] != 0.0) ? CUSTOM_FOLIAGE_SCALES[FOLIAGE_PLANT_SELECTION[num]] : 1.0;

			float PLANT_SCALE_XY = 0.4 * FOLIAGE_PLANT_SCALE[num] * customScale * PLANT_SCALE_MULTIPLIER*distFadeScale;
			float PLANT_SCALE_Z = 0.4 * FOLIAGE_PLANT_SCALE[num] * customScale * PLANT_SCALE_MULTIPLIER*distFadeScale*distMult;

			re.reType = RT_PLANT;//RT_MODEL;

			re.origin[2] += 8.0 + (1.0 - (FOLIAGE_PLANT_SCALE[num] * customScale * distMult));

			re.hModel = FOLIAGE_PLANT_MODELS[FOLIAGE_PLANT_SELECTION[num] - 1];

			VectorSet(re.modelScale, PLANT_SCALE_XY, PLANT_SCALE_XY, PLANT_SCALE_Z);

			VectorCopy(FOLIAGE_PLANT_ANGLES[num], re.angles);
			AxisCopy(FOLIAGE_PLANT_AXIS[num], re.axis);

			ScaleModelAxis(&re);

			FOLIAGE_AddFoliageEntityToScene(&re);
		}
	}

	if (MAP_HAS_TREES)
	{
		if ((passType == FOLIAGE_PASS_CLOSETREE || passType == FOLIAGE_PASS_TREE) && FOLIAGE_TREE_SELECTION[num] > 0 && FOLIAGE_TREE_SELECTION[num] <= NUM_TREE_TYPES)
		{// Add the tree model...
			VectorCopy(FOLIAGE_POSITIONS[num], re.origin);
			re.customShader = 0;
			re.renderfx = 0;

			float customScale = (CUSTOM_FOLIAGE_SCALES[FOLIAGE_TREE_SELECTION[num]] != 0.0) ? CUSTOM_FOLIAGE_SCALES[FOLIAGE_TREE_SELECTION[num]] : 1.0;

			if (dist > FOLIAGE_AREA_SIZE*cg_foliageTreeBillboardRangeMult.value || dist > FOLIAGE_TREE_VISIBLE_DISTANCE)
			{
				re.reType = RT_SPRITE;

				re.radius = FOLIAGE_TREE_SCALE[num] * customScale * 2.5*FOLIAGE_TREE_BILLBOARD_SIZE[FOLIAGE_TREE_SELECTION[num] - 1] * TREE_SCALE_MULTIPLIER;

				re.customShader = FOLIAGE_TREE_BILLBOARD_SHADER[FOLIAGE_TREE_SELECTION[num] - 1];

				re.shaderRGBA[0] = 255;
				re.shaderRGBA[1] = 255;
				re.shaderRGBA[2] = 255;
				re.shaderRGBA[3] = 255;

				re.origin[2] += re.radius;
				re.origin[2] += FOLIAGE_TREE_ZOFFSET[FOLIAGE_TREE_SELECTION[num] - 1];

				VectorCopy(FOLIAGE_TREE_ANGLES[num], re.angles);
				AxisCopy(FOLIAGE_TREE_AXIS[num], re.axis);

				FOLIAGE_AddFoliageEntityToScene(&re);
			}
			else
			{
				float	furthestDist = 0.0;
				int		furthestNum = 0;
				int		tree = 0;

				re.reType = RT_MODEL;
				re.hModel = FOLIAGE_TREE_MODEL[FOLIAGE_TREE_SELECTION[num] - 1];

				VectorSet(re.modelScale, FOLIAGE_TREE_SCALE[num] * customScale * 2.5*TREE_SCALE_MULTIPLIER, FOLIAGE_TREE_SCALE[num] * 2.5*TREE_SCALE_MULTIPLIER, FOLIAGE_TREE_SCALE[num] * 2.5*TREE_SCALE_MULTIPLIER);

				re.origin[2] += FOLIAGE_TREE_ZOFFSET[FOLIAGE_TREE_SELECTION[num] - 1];

				VectorCopy(FOLIAGE_TREE_BILLBOARD_ANGLES[num], re.angles);
				AxisCopy(FOLIAGE_TREE_BILLBOARD_AXIS[num], re.axis);

				ScaleModelAxis(&re);

				FOLIAGE_AddFoliageEntityToScene(&re);

				//
				// Create a list of the closest trees for generating solids...
				//

				for (tree = 0; tree < FOLIAGE_SOLID_TREES_MAX; tree++)
				{
					if (FOLIAGE_SOLID_TREES_DIST[tree] > furthestDist)
					{
						furthestNum = tree;
						furthestDist = FOLIAGE_SOLID_TREES_DIST[tree];
					}
				}


				if (dist < FOLIAGE_SOLID_TREES_DIST[furthestNum])
				{
					//trap->Print("Set solid tree %i at %f %f %f. Dist %f.\n", furthestNum, FOLIAGE_POSITIONS[num][0], FOLIAGE_POSITIONS[num][1], FOLIAGE_POSITIONS[num][2], dist);
					FOLIAGE_SOLID_TREES[furthestNum] = num;
					FOLIAGE_SOLID_TREES_DIST[furthestNum] = dist;
				}
			}
		}
	}
}

extern int BG_SiegeGetPairedValue(char *buf, char *key, char *outbuf);

qboolean FOLIAGE_LoadMapClimateInfo(void)
{
	const char		*climateName = NULL;

	climateName = IniRead(va("foliage/%s.climateInfo", cgs.currentmapname), "CLIMATE", "CLIMATE_TYPE", "");

	memset(CURRENT_CLIMATE_OPTION, 0, sizeof(CURRENT_CLIMATE_OPTION));
	strncpy(CURRENT_CLIMATE_OPTION, climateName, strlen(climateName));

	if (CURRENT_CLIMATE_OPTION[0] == '\0')
	{
		trap->Print("^1*** ^3%s^5: No map climate info file ^7foliage/%s.climateInfo^5. Using default climate option.\n", "AUTO-FOLIAGE", cgs.currentmapname);
		strncpy(CURRENT_CLIMATE_OPTION, "tropical", strlen("tropical"));
		return qfalse;
	}

	trap->Print("^1*** ^3%s^5: Successfully loaded climateInfo file ^7foliage/%s.climateInfo^5. Using ^3%s^5 climate option.\n", "AUTO-FOLIAGE", cgs.currentmapname, CURRENT_CLIMATE_OPTION);

	return qtrue;
}

void FOLIAGE_FreeMemory(void)
{// Free any current memory...
	if (FOLIAGE_POSITIONS) free(FOLIAGE_POSITIONS);
	if (FOLIAGE_NORMALS) free(FOLIAGE_NORMALS);
	if (FOLIAGE_PLANT_SELECTION) free(FOLIAGE_PLANT_SELECTION);
	if (FOLIAGE_PLANT_ANGLE) free(FOLIAGE_PLANT_ANGLE);
	if (FOLIAGE_PLANT_ANGLES) free(FOLIAGE_PLANT_ANGLES);
	if (FOLIAGE_PLANT_AXIS) free(FOLIAGE_PLANT_AXIS);
	if (FOLIAGE_PLANT_SCALE) free(FOLIAGE_PLANT_SCALE);
	if (FOLIAGE_TREE_SELECTION) free(FOLIAGE_TREE_SELECTION);
	if (FOLIAGE_TREE_ANGLE) free(FOLIAGE_TREE_ANGLE);
	if (FOLIAGE_TREE_ANGLES) free(FOLIAGE_TREE_ANGLES);
	if (FOLIAGE_TREE_AXIS) free(FOLIAGE_TREE_AXIS);
	if (FOLIAGE_TREE_BILLBOARD_ANGLES) free(FOLIAGE_TREE_BILLBOARD_ANGLES);
	if (FOLIAGE_TREE_BILLBOARD_AXIS) free(FOLIAGE_TREE_BILLBOARD_AXIS);
	if (FOLIAGE_TREE_SCALE) free(FOLIAGE_TREE_SCALE);

	FOLIAGE_POSITIONS = NULL;
	FOLIAGE_NORMALS = NULL;
	FOLIAGE_PLANT_SELECTION = NULL;
	FOLIAGE_PLANT_ANGLE = NULL;
	FOLIAGE_PLANT_ANGLES = NULL;
	FOLIAGE_PLANT_AXIS = NULL;
	FOLIAGE_PLANT_SCALE = NULL;
	FOLIAGE_TREE_SELECTION = NULL;
	FOLIAGE_TREE_ANGLE = NULL;
	FOLIAGE_TREE_ANGLES = NULL;
	FOLIAGE_TREE_AXIS = NULL;
	FOLIAGE_TREE_BILLBOARD_ANGLES = NULL;
	FOLIAGE_TREE_BILLBOARD_AXIS = NULL;
	FOLIAGE_TREE_SCALE = NULL;

	if (IN_RANGE_AREAS_LIST) free(IN_RANGE_AREAS_LIST);
	if (IN_RANGE_AREAS_DISTANCE) free(IN_RANGE_AREAS_DISTANCE);
	if (IN_RANGE_TREE_AREAS_LIST) free(IN_RANGE_TREE_AREAS_LIST);
	if (IN_RANGE_TREE_AREAS_DISTANCE) free(IN_RANGE_TREE_AREAS_DISTANCE);

	IN_RANGE_AREAS_LIST = NULL;
	IN_RANGE_AREAS_DISTANCE = NULL;
	IN_RANGE_TREE_AREAS_LIST = NULL;
	IN_RANGE_TREE_AREAS_DISTANCE = NULL;

	if (FOLIAGE_AREAS_LIST_COUNT) free(FOLIAGE_AREAS_LIST_COUNT);
	if (FOLIAGE_AREAS_LIST) free(FOLIAGE_AREAS_LIST);
	if (FOLIAGE_AREAS_MINS) free(FOLIAGE_AREAS_MINS);
	if (FOLIAGE_AREAS_MAXS) free(FOLIAGE_AREAS_MAXS);

	FOLIAGE_AREAS_LIST_COUNT = NULL;
	FOLIAGE_AREAS_LIST = NULL;
	FOLIAGE_AREAS_MINS = NULL;
	FOLIAGE_AREAS_MAXS = NULL;

	if (FOLIAGE_AREAS_TREES_LIST_COUNT) free(FOLIAGE_AREAS_TREES_LIST_COUNT);
	if (FOLIAGE_AREAS_TREES_VISCHECK_TIME) free(FOLIAGE_AREAS_TREES_VISCHECK_TIME);
	if (FOLIAGE_AREAS_TREES_VISCHECK_RESULT) free(FOLIAGE_AREAS_TREES_VISCHECK_RESULT);
	if (FOLIAGE_AREAS_TREES_LIST) free(FOLIAGE_AREAS_TREES_LIST);

	FOLIAGE_AREAS_TREES_LIST_COUNT = NULL;
	FOLIAGE_AREAS_TREES_VISCHECK_TIME = NULL;
	FOLIAGE_AREAS_TREES_VISCHECK_RESULT = NULL;
	FOLIAGE_AREAS_TREES_LIST = NULL;

	if (FOLIAGE_TREE_SELECTION) free(FOLIAGE_TREE_SELECTION);
	if (FOLIAGE_TREE_ANGLE) free(FOLIAGE_TREE_ANGLE);
	if (FOLIAGE_TREE_ANGLES) free(FOLIAGE_TREE_ANGLES);
	if (FOLIAGE_TREE_AXIS) free(FOLIAGE_TREE_AXIS);
	if (FOLIAGE_TREE_BILLBOARD_ANGLES) free(FOLIAGE_TREE_BILLBOARD_ANGLES);
	if (FOLIAGE_TREE_BILLBOARD_AXIS) free(FOLIAGE_TREE_BILLBOARD_AXIS);
	if (FOLIAGE_TREE_SCALE) free(FOLIAGE_TREE_SCALE);

	FOLIAGE_TREE_SELECTION = NULL;
	FOLIAGE_TREE_ANGLE = NULL;
	FOLIAGE_TREE_ANGLES = NULL;
	FOLIAGE_TREE_AXIS = NULL;
	FOLIAGE_TREE_BILLBOARD_ANGLES = NULL;
	FOLIAGE_TREE_BILLBOARD_AXIS = NULL;
	FOLIAGE_TREE_SCALE = NULL;

	if (IN_RANGE_AREAS_LIST) free(IN_RANGE_AREAS_LIST);
	if (IN_RANGE_TREE_AREAS_LIST) free(IN_RANGE_TREE_AREAS_LIST);

	IN_RANGE_AREAS_LIST_COUNT = 0;
	IN_RANGE_AREAS_LIST = NULL;
	IN_RANGE_TREE_AREAS_LIST_COUNT = 0;
	IN_RANGE_TREE_AREAS_LIST = NULL;

	FOLIAGE_INITIALIZED = qfalse;
	FOLIAGE_LOADED = qfalse;
	MAP_HAS_TREES = qfalse;
	FOLIAGE_NUM_POSITIONS = 0;
}

qboolean FOLIAGE_LoadFoliagePositions(char *filename)
{
	fileHandle_t	f;
	int				i = 0;
	int				numPositions = 0;
	int				numRemovedPositions = 0;
	int				numRemovedRoadPositions = 0;
	float			minFoliageScale = cg_foliageMinFoliageScale.value;
	int fileCount = 0;
	int foliageCount = 0;

	if (!filename || filename[0] == '0')
		trap->FS_Open(va("foliage/%s.foliage", cgs.currentmapname), &f, FS_READ);
	else
		trap->FS_Open(va("foliage/%s.foliage", filename), &f, FS_READ);

	if (!f)
	{
		return qfalse;
	}

	MAP_HAS_TREES = qfalse;
	FOLIAGE_NUM_POSITIONS = 0;
	FOLIAGE_FreeMemory();

	trap->FS_Read(&fileCount, sizeof(int), f);

	FOLIAGE_POSITIONS = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
	FOLIAGE_NORMALS = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
	FOLIAGE_PLANT_SELECTION = (int *)malloc(fileCount * sizeof(int));
	FOLIAGE_PLANT_ANGLE = (float *)malloc(fileCount * sizeof(float));
	FOLIAGE_PLANT_ANGLES = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
	FOLIAGE_PLANT_AXIS = (matrix3_t *)malloc(fileCount * sizeof(matrix3_t));
	FOLIAGE_PLANT_SCALE = (float *)malloc(fileCount * sizeof(float));
	
	FOLIAGE_TREE_SELECTION = NULL;
	FOLIAGE_TREE_ANGLE = NULL;
	FOLIAGE_TREE_ANGLES = NULL;
	FOLIAGE_TREE_AXIS = NULL;
	FOLIAGE_TREE_BILLBOARD_ANGLES = NULL;
	FOLIAGE_TREE_BILLBOARD_AXIS = NULL;
	FOLIAGE_TREE_SCALE = NULL;

	int usedRam = (fileCount * sizeof(vec3_t)) + (fileCount * sizeof(vec3_t)) + (fileCount * sizeof(int)) + (fileCount * sizeof(float)) + (fileCount * sizeof(float)) + (fileCount * sizeof(int)) + (fileCount * sizeof(float)) + (fileCount * sizeof(float)) + (fileCount * sizeof(matrix3_t)) + (fileCount * sizeof(matrix3_t)) + (fileCount * sizeof(matrix3_t)) + (fileCount * sizeof(vec3_t)) + (fileCount * sizeof(vec3_t)) + (fileCount * sizeof(vec3_t));
	usedRam /= 1024;
	trap->Print("^1*** ^3%s^5: %i KB allocated for initial foliage memory.\n", "FOLIAGE", usedRam);

	for (i = 0; i < fileCount; i++)
	{
		int TREE_SELECTION = 0;
		float TREE_ANGLE = 0;
		float TREE_SCALE = 0;

		trap->FS_Read(&FOLIAGE_POSITIONS[foliageCount], sizeof(vec3_t), f);
		trap->FS_Read(&FOLIAGE_NORMALS[foliageCount], sizeof(vec3_t), f);
		trap->FS_Read(&FOLIAGE_PLANT_SELECTION[foliageCount], sizeof(int), f);
		trap->FS_Read(&FOLIAGE_PLANT_ANGLE[foliageCount], sizeof(float), f);
		trap->FS_Read(&FOLIAGE_PLANT_SCALE[foliageCount], sizeof(float), f);
		trap->FS_Read(&TREE_SELECTION, sizeof(int), f);
		
		trap->FS_Read(&TREE_ANGLE, sizeof(float), f);
		trap->FS_Read(&TREE_SCALE, sizeof(float), f);

		if (!MAP_HAS_TREES && TREE_SELECTION > 0)
		{// Init the trees memory area...
			MAP_HAS_TREES = qtrue;

			FOLIAGE_TREE_SELECTION = (int *)malloc(fileCount * sizeof(int));
			FOLIAGE_TREE_ANGLE = (float *)malloc(fileCount * sizeof(float));
			FOLIAGE_TREE_ANGLES = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
			FOLIAGE_TREE_AXIS = (matrix3_t *)malloc(fileCount * sizeof(matrix3_t));
			FOLIAGE_TREE_BILLBOARD_ANGLES = (vec3_t *)malloc(fileCount * sizeof(vec3_t));
			FOLIAGE_TREE_BILLBOARD_AXIS = (matrix3_t *)malloc(fileCount * sizeof(matrix3_t));
			FOLIAGE_TREE_SCALE = (float *)malloc(fileCount * sizeof(float));
		}

		if (MAP_HAS_TREES && TREE_SELECTION > 0)
		{
			FOLIAGE_TREE_SELECTION[foliageCount] = TREE_SELECTION;
			FOLIAGE_TREE_ANGLE[foliageCount] = TREE_ANGLE;
			FOLIAGE_TREE_SCALE[foliageCount] = TREE_SCALE;
		}

		if (RoadExistsAtPoint(FOLIAGE_POSITIONS[foliageCount]))
		{
			numRemovedRoadPositions++;
			continue;
		}

		if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[foliageCount] > 0)
		{// Only keep positions with trees or plants...
			FOLIAGE_PLANT_SELECTION[foliageCount] = 0;
			foliageCount++;
		}
		else if ((MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[foliageCount] > 0) || FOLIAGE_PLANT_SELECTION[foliageCount] > 0)
		{// Only keep positions with trees or plants...
			foliageCount++;
		}
		else
		{
			numRemovedPositions++;
		}
	}

	FOLIAGE_NUM_POSITIONS = foliageCount;

	trap->FS_Close(f);

	if (FOLIAGE_NUM_POSITIONS > 0)
	{// Re-alloc memory to the amount actually used...
		FOLIAGE_POSITIONS = (vec3_t *)realloc(FOLIAGE_POSITIONS, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_NORMALS = (vec3_t *)realloc(FOLIAGE_NORMALS, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_PLANT_SELECTION = (int *)realloc(FOLIAGE_PLANT_SELECTION, FOLIAGE_NUM_POSITIONS * sizeof(int));
		FOLIAGE_PLANT_ANGLE = (float *)realloc(FOLIAGE_PLANT_ANGLE, FOLIAGE_NUM_POSITIONS * sizeof(float));
		FOLIAGE_PLANT_ANGLES = (vec3_t *)realloc(FOLIAGE_PLANT_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_PLANT_AXIS = (matrix3_t *)realloc(FOLIAGE_PLANT_AXIS, FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t));
		FOLIAGE_PLANT_SCALE = (float *)realloc(FOLIAGE_PLANT_SCALE, FOLIAGE_NUM_POSITIONS * sizeof(float));
		
		if (MAP_HAS_TREES)
		{
			FOLIAGE_TREE_SELECTION = (int *)realloc(FOLIAGE_TREE_SELECTION, FOLIAGE_NUM_POSITIONS * sizeof(int));
			FOLIAGE_TREE_ANGLE = (float *)realloc(FOLIAGE_TREE_ANGLE, FOLIAGE_NUM_POSITIONS * sizeof(float));
			FOLIAGE_TREE_ANGLES = (vec3_t *)realloc(FOLIAGE_TREE_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
			FOLIAGE_TREE_AXIS = (matrix3_t *)realloc(FOLIAGE_TREE_AXIS, FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t));
			FOLIAGE_TREE_BILLBOARD_ANGLES = (vec3_t *)realloc(FOLIAGE_TREE_BILLBOARD_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
			FOLIAGE_TREE_BILLBOARD_AXIS = (matrix3_t *)realloc(FOLIAGE_TREE_BILLBOARD_AXIS, FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t));
			FOLIAGE_TREE_SCALE = (float *)realloc(FOLIAGE_TREE_SCALE, FOLIAGE_NUM_POSITIONS * sizeof(float));
		}

		usedRam = (FOLIAGE_NUM_POSITIONS * sizeof(vec3_t)) + (FOLIAGE_NUM_POSITIONS * sizeof(vec3_t)) + (FOLIAGE_NUM_POSITIONS * sizeof(int)) + (FOLIAGE_NUM_POSITIONS * sizeof(float)) + (FOLIAGE_NUM_POSITIONS * sizeof(float)) + (FOLIAGE_NUM_POSITIONS * sizeof(int)) + (FOLIAGE_NUM_POSITIONS * sizeof(float)) + (FOLIAGE_NUM_POSITIONS * sizeof(float)) + (FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t)) + (FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t)) + (FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t));
		usedRam /= 1024;
		trap->Print("^1*** ^3%s^5: %i KB allocated for final foliage memory.\n", "FOLIAGE", usedRam);
	}
	else
	{
		FOLIAGE_FreeMemory();
		trap->Print("^1*** ^3%s^5: No foliage points in foliage file ^7foliage/%s.foliage^5. Memory freed.\n", "FOLIAGE", cgs.currentmapname);
		return qfalse;
	}

	if (!FOLIAGE_AREAS_LIST_COUNT) FOLIAGE_AREAS_LIST_COUNT = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_LIST) FOLIAGE_AREAS_LIST = (ivec256_t *)malloc(sizeof(ivec256_t) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_MINS) FOLIAGE_AREAS_MINS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_MAXS) FOLIAGE_AREAS_MAXS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);

	if (!IN_RANGE_AREAS_LIST) IN_RANGE_AREAS_LIST = (int *)malloc(sizeof(int) * 8192);
	if (!IN_RANGE_AREAS_DISTANCE) IN_RANGE_AREAS_DISTANCE = (float *)malloc(sizeof(float) * 8192);


	if (!FOLIAGE_AREAS_TREES_LIST_COUNT) FOLIAGE_AREAS_TREES_LIST_COUNT = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_TREES_VISCHECK_TIME) FOLIAGE_AREAS_TREES_VISCHECK_TIME = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_TREES_VISCHECK_RESULT) FOLIAGE_AREAS_TREES_VISCHECK_RESULT = (qboolean *)malloc(sizeof(qboolean) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_TREES_LIST) FOLIAGE_AREAS_TREES_LIST = (ivec256_t *)malloc(sizeof(ivec256_t) * FOLIAGE_AREA_MAX);

	if (!IN_RANGE_TREE_AREAS_LIST) IN_RANGE_TREE_AREAS_LIST = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
	if (!IN_RANGE_TREE_AREAS_DISTANCE) IN_RANGE_TREE_AREAS_DISTANCE = (float *)malloc(sizeof(float) * FOLIAGE_AREA_MAX);

	usedRam = (sizeof(int) * FOLIAGE_AREA_MAX) + (sizeof(int) * FOLIAGE_AREA_MAX) + (sizeof(qboolean) * FOLIAGE_AREA_MAX) + (sizeof(ivec256_t) * FOLIAGE_AREA_MAX) + (sizeof(int) * FOLIAGE_AREA_MAX) + (sizeof(float) * FOLIAGE_AREA_MAX) + (sizeof(matrix3_t) * FOLIAGE_AREA_MAX) + (sizeof(vec3_t) * FOLIAGE_AREA_MAX);
	usedRam /= 1024;
	trap->Print("^1*** ^3%s^5: %i KB allocated for final foliage memory.\n", "FOLIAGE", usedRam);

	for (int i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Set up all axis...
		if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
		{// Pre-set up tree and tree billboard axii...
			FOLIAGE_TREE_ANGLES[i][PITCH] = FOLIAGE_TREE_ANGLES[i][ROLL] = 0.0f;
			FOLIAGE_TREE_ANGLES[i][YAW] = FOLIAGE_TREE_ANGLE[i];
			AnglesToAxis(FOLIAGE_TREE_ANGLES[i], FOLIAGE_TREE_AXIS[i]);

			FOLIAGE_TREE_BILLBOARD_ANGLES[i][PITCH] = FOLIAGE_TREE_BILLBOARD_ANGLES[i][ROLL] = 0.0f;
			FOLIAGE_TREE_BILLBOARD_ANGLES[i][YAW] = 270.0 - FOLIAGE_TREE_ANGLE[i];
			AnglesToAxis(FOLIAGE_TREE_BILLBOARD_ANGLES[i], FOLIAGE_TREE_BILLBOARD_AXIS[i]);
		}

		if (FOLIAGE_PLANT_SELECTION[i] > 0)
		{// Pre-set up plant axis...
			vectoangles(FOLIAGE_NORMALS[i], FOLIAGE_PLANT_ANGLES[i]);
			FOLIAGE_PLANT_ANGLES[i][PITCH] += 90;
			AnglesToAxis(FOLIAGE_PLANT_ANGLES[i], FOLIAGE_PLANT_AXIS[i]);

			FOLIAGE_ApplyAxisRotation(FOLIAGE_PLANT_AXIS[i], YAW, FOLIAGE_PLANT_ANGLE[i]);
		}
	}

	trap->Print("^1*** ^3%s^5: Successfully loaded %i foliage points (%i unused grasses, and %i road removed) from foliage file ^7foliage/%s.foliage^5.\n", "FOLIAGE",
		FOLIAGE_NUM_POSITIONS, numRemovedPositions, numRemovedRoadPositions, cgs.currentmapname);

	if (!filename || filename[0] == '0')
	{// Don't need to waste time on this when doing "copy" function...
		FOLIAGE_Setup_Foliage_Areas();
	}

	return qtrue;
}

qboolean FOLIAGE_SaveFoliagePositions(void)
{
	fileHandle_t	f;
	int				i = 0;
	int				numTrees = 0;
	int				numPlants = 0;
	int				numGrasses = 0;

	trap->FS_Open(va("foliage/%s.foliage", cgs.currentmapname), &f, FS_WRITE);

	if (!f)
	{
		trap->Print("^1*** ^3%s^5: Failed to open foliage file ^7foliage/%s.foliage^5 for save.\n", "AUTO-FOLIAGE", cgs.currentmapname);
		return qfalse;
	}

	MAP_HAS_TREES = qfalse;

	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{
		trace_t tr;
		vec3_t	up, down;
		VectorCopy(FOLIAGE_POSITIONS[i], up);
		up[2] += 128;
		VectorCopy(FOLIAGE_POSITIONS[i], down);
		down[2] -= 128;
		CG_Trace(&tr, up, NULL, NULL, down, -1, MASK_SOLID);
		VectorCopy(tr.plane.normal, FOLIAGE_NORMALS[i]);

		vec3_t slopeangles;
		vectoangles(FOLIAGE_NORMALS[i], slopeangles);

		float pitch = slopeangles[0];

		if (pitch > 180)
			pitch -= 360;

		if (pitch < -180)
			pitch += 360;

		pitch += 90.0f;

#define MAX_FOLIAGE_ALLOW_SLOPE 46.0

		if (pitch > MAX_FOLIAGE_ALLOW_SLOPE || pitch < -MAX_FOLIAGE_ALLOW_SLOPE)
		{// We hit something bad in our trace (most likely a wall or tree), instead use flat...
			VectorSet(FOLIAGE_NORMALS[i], 0, 0, 0);
		}

		if (FOLIAGE_TREE_SELECTION && FOLIAGE_TREE_SELECTION[i] > 0)
		{
			MAP_HAS_TREES = qtrue;
		}
	}

	trap->FS_Write(&FOLIAGE_NUM_POSITIONS, sizeof(int), f);

	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{
		trap->FS_Write(&FOLIAGE_POSITIONS[i], sizeof(vec3_t), f);
		trap->FS_Write(&FOLIAGE_NORMALS[i], sizeof(vec3_t), f);
		trap->FS_Write(&FOLIAGE_PLANT_SELECTION[i], sizeof(int), f);
		trap->FS_Write(&FOLIAGE_PLANT_ANGLE[i], sizeof(float), f);
		trap->FS_Write(&FOLIAGE_PLANT_SCALE[i], sizeof(float), f);

		if (MAP_HAS_TREES)
		{
			trap->FS_Write(&FOLIAGE_TREE_SELECTION[i], sizeof(int), f);
			trap->FS_Write(&FOLIAGE_TREE_ANGLE[i], sizeof(float), f);
			trap->FS_Write(&FOLIAGE_TREE_SCALE[i], sizeof(float), f);
		}
		else
		{
			int zero = 0;
			float zerof = 0.0;
			trap->FS_Write(&zero, sizeof(int), f);
			trap->FS_Write(&zerof, sizeof(float), f);
			trap->FS_Write(&zerof, sizeof(float), f);
		}

		if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
			numTrees++;
		else if (FOLIAGE_PLANT_SELECTION[i] > 0)
			numPlants++;
		else
			numGrasses++;
	}

	trap->FS_Close(f);

	if (FOLIAGE_NUM_POSITIONS <= 0)
	{
		FOLIAGE_FreeMemory();
		trap->Print("^1*** ^3%s^5: No foliage points in foliage file ^7foliage/%s.foliage^5. Memory freed.\n", "AUTO-FOLIAGE", cgs.currentmapname);
		return qfalse;
	}

	if (!FOLIAGE_AREAS_LIST_COUNT) FOLIAGE_AREAS_LIST_COUNT = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_LIST) FOLIAGE_AREAS_LIST = (ivec256_t *)malloc(sizeof(ivec256_t) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_MINS) FOLIAGE_AREAS_MINS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);
	if (!FOLIAGE_AREAS_MAXS) FOLIAGE_AREAS_MAXS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);

	if (!IN_RANGE_AREAS_LIST) IN_RANGE_AREAS_LIST = (int *)malloc(sizeof(int) * 8192);
	if (!IN_RANGE_AREAS_DISTANCE) IN_RANGE_AREAS_DISTANCE = (float *)malloc(sizeof(float) * 8192);

	if (!MAP_HAS_TREES)
	{// Don't waste the ram...
		if (FOLIAGE_AREAS_TREES_LIST_COUNT) free(FOLIAGE_AREAS_TREES_LIST_COUNT);
		if (FOLIAGE_AREAS_TREES_VISCHECK_TIME) free(FOLIAGE_AREAS_TREES_VISCHECK_TIME);
		if (FOLIAGE_AREAS_TREES_VISCHECK_RESULT) free(FOLIAGE_AREAS_TREES_VISCHECK_RESULT);
		if (FOLIAGE_AREAS_TREES_LIST) free(FOLIAGE_AREAS_TREES_LIST);

		FOLIAGE_AREAS_TREES_LIST_COUNT = NULL;
		FOLIAGE_AREAS_TREES_VISCHECK_TIME = NULL;
		FOLIAGE_AREAS_TREES_VISCHECK_RESULT = NULL;
		FOLIAGE_AREAS_TREES_LIST = NULL;

		if (FOLIAGE_TREE_SELECTION) free(FOLIAGE_TREE_SELECTION);
		if (FOLIAGE_TREE_ANGLE) free(FOLIAGE_TREE_ANGLE);
		if (FOLIAGE_TREE_ANGLES) free(FOLIAGE_TREE_ANGLES);
		if (FOLIAGE_TREE_AXIS) free(FOLIAGE_TREE_AXIS);
		if (FOLIAGE_TREE_BILLBOARD_ANGLES) free(FOLIAGE_TREE_BILLBOARD_ANGLES);
		if (FOLIAGE_TREE_BILLBOARD_AXIS) free(FOLIAGE_TREE_BILLBOARD_AXIS);
		if (FOLIAGE_TREE_SCALE) free(FOLIAGE_TREE_SCALE);

		FOLIAGE_TREE_SELECTION = NULL;
		FOLIAGE_TREE_ANGLE = NULL;
		FOLIAGE_TREE_ANGLES = NULL;
		FOLIAGE_TREE_AXIS = NULL;
		FOLIAGE_TREE_BILLBOARD_ANGLES = NULL;
		FOLIAGE_TREE_BILLBOARD_AXIS = NULL;
		FOLIAGE_TREE_SCALE = NULL;
	}
	else
	{// We will need a tree in-range list, dynamically allocate...
		if (!FOLIAGE_AREAS_TREES_LIST_COUNT) FOLIAGE_AREAS_TREES_LIST_COUNT = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_TREES_VISCHECK_TIME) FOLIAGE_AREAS_TREES_VISCHECK_TIME = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_TREES_VISCHECK_RESULT) FOLIAGE_AREAS_TREES_VISCHECK_RESULT = (qboolean *)malloc(sizeof(qboolean) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_TREES_LIST) FOLIAGE_AREAS_TREES_LIST = (ivec256_t *)malloc(sizeof(ivec256_t) * FOLIAGE_AREA_MAX);

		if (!IN_RANGE_TREE_AREAS_LIST) IN_RANGE_TREE_AREAS_LIST = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!IN_RANGE_TREE_AREAS_DISTANCE) IN_RANGE_TREE_AREAS_DISTANCE = (float *)malloc(sizeof(float) * FOLIAGE_AREA_MAX);
	}

	for (int i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Set up all axis...
		if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
		{// Pre-set up tree and tree billboard axii...
			FOLIAGE_TREE_ANGLES[i][PITCH] = FOLIAGE_TREE_ANGLES[i][ROLL] = 0.0f;
			FOLIAGE_TREE_ANGLES[i][YAW] = FOLIAGE_TREE_ANGLE[i];
			AnglesToAxis(FOLIAGE_TREE_ANGLES[i], FOLIAGE_TREE_AXIS[i]);

			FOLIAGE_TREE_BILLBOARD_ANGLES[i][PITCH] = FOLIAGE_TREE_BILLBOARD_ANGLES[i][ROLL] = 0.0f;
			FOLIAGE_TREE_BILLBOARD_ANGLES[i][YAW] = 270.0 - FOLIAGE_TREE_ANGLE[i];
			AnglesToAxis(FOLIAGE_TREE_BILLBOARD_ANGLES[i], FOLIAGE_TREE_BILLBOARD_AXIS[i]);
		}

		if (FOLIAGE_PLANT_SELECTION[i] > 0)
		{// Pre-set up plant axis...
			vectoangles(FOLIAGE_NORMALS[i], FOLIAGE_PLANT_ANGLES[i]);
			FOLIAGE_PLANT_ANGLES[i][PITCH] += 90;
			AnglesToAxis(FOLIAGE_PLANT_ANGLES[i], FOLIAGE_PLANT_AXIS[i]);

			FOLIAGE_ApplyAxisRotation(FOLIAGE_PLANT_AXIS[i], YAW, FOLIAGE_PLANT_ANGLE[i]);
		}
	}

	FOLIAGE_Setup_Foliage_Areas();

	trap->Print("^1*** ^3%s^5: Successfully saved %i foliage points (%i trees, %i plants, %i grasses) to foliage file ^7foliage/%s.foliage^5.\n", "AUTO-FOLIAGE",
		FOLIAGE_NUM_POSITIONS, numTrees, numPlants, numGrasses, cgs.currentmapname);

	// Make it reload the whole lot again, to make sure everything is clean...
	FOLIAGE_FreeMemory();
	FOLIAGE_INITIALIZED = qfalse;
	FOLIAGE_LOADED = qfalse;

	return qtrue;
}

qboolean FOLIAGE_IgnoreFoliageOnMap(void)
{
#if 0
	if (StringContainsWord(cgs.currentmapname, "eisley")
		|| StringContainsWord(cgs.currentmapname, "desert")
		|| StringContainsWord(cgs.currentmapname, "tatooine")
		|| StringContainsWord(cgs.currentmapname, "hoth")
		|| StringContainsWord(cgs.currentmapname, "mp/ctf1")
		|| StringContainsWord(cgs.currentmapname, "mp/ctf2")
		|| StringContainsWord(cgs.currentmapname, "mp/ctf4")
		|| StringContainsWord(cgs.currentmapname, "mp/ctf5")
		|| StringContainsWord(cgs.currentmapname, "mp/ffa1")
		|| StringContainsWord(cgs.currentmapname, "mp/ffa2")
		|| StringContainsWord(cgs.currentmapname, "mp/ffa3")
		|| StringContainsWord(cgs.currentmapname, "mp/ffa4")
		|| StringContainsWord(cgs.currentmapname, "mp/ffa5")
		|| StringContainsWord(cgs.currentmapname, "mp/duel1")
		|| StringContainsWord(cgs.currentmapname, "mp/duel2")
		|| StringContainsWord(cgs.currentmapname, "mp/duel3")
		|| StringContainsWord(cgs.currentmapname, "mp/duel4")
		|| StringContainsWord(cgs.currentmapname, "mp/duel5")
		|| StringContainsWord(cgs.currentmapname, "mp/duel7")
		|| StringContainsWord(cgs.currentmapname, "mp/duel9")
		|| StringContainsWord(cgs.currentmapname, "mp/duel10")
		|| StringContainsWord(cgs.currentmapname, "bespin_streets")
		|| StringContainsWord(cgs.currentmapname, "bespin_platform"))
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

	FOLIAGE_VISIBLE_DISTANCE = (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0) ? CUSTOM_FOLIAGE_MAX_DISTANCE : (FOLIAGE_AREA_SIZE*cg_foliageGrassRangeMult.value);
	FOLIAGE_PLANT_VISIBLE_DISTANCE = (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0) ? CUSTOM_FOLIAGE_MAX_DISTANCE : (FOLIAGE_AREA_SIZE*cg_foliagePlantRangeMult.value);
	FOLIAGE_TREE_VISIBLE_DISTANCE = (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0) ? CUSTOM_FOLIAGE_MAX_DISTANCE : (FOLIAGE_AREA_SIZE*cg_foliageTreeRangeMult.value);

	if (!FOLIAGE_INITIALIZED)
	{// Init/register all foliage models...
		int i = 0;

		RoadExistsAtPoint(vec3_origin); // Just to make sure that road map is pre-loaded...

		//
		// Allow for various foliage sets based on climate or mapInfo settings...
		//
		char FOLIAGE_MODEL_SELECTION[128] = { 0 };

		strcpy(FOLIAGE_MODEL_SELECTION, IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "FOLIAGE", "foliageSet", "default"));

		if (FOLIAGE_MODEL_SELECTION[0] == '\0' || !strcmp(FOLIAGE_MODEL_SELECTION, "default"))
		{// If it returned default value, check also in mapinfo file...
			memset(FOLIAGE_MODEL_SELECTION, 0, sizeof(char) * 128);
			strcpy(FOLIAGE_MODEL_SELECTION, IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", "FOLIAGE_SET", "default"));
		}

		if (!Q_stricmp(FOLIAGE_MODEL_SELECTION, "custom")) 
		{
			MAP_HAS_TREES = qfalse;

			trap->Print("^1*** ^3%s^5: Map grass selection using \"^7custom^5\" foliage set.\n", "FOLIAGE", cgs.currentmapname);

			int		customRealNormalTempModelsAdded = 0;
			int		customRealRareTempModelsAdded = 0;
			char	customNormalFoliageModelsTempList[69][128] = { 0 };
			char	customRareFoliageModelsTempList[69][128] = { 0 };
			float	customNormalFoliageModelsTempScalesList[69] = { 1.0 };
			float	customRareFoliageModelsTempScalesList[69] = { 1.0 };

			CUSTOM_FOLIAGE_MAX_DISTANCE = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", "FOLAIGE_MAX_DISTANCE", "0"));

			if (CUSTOM_FOLIAGE_MAX_DISTANCE != 0.0)
			{
				trap->Print("^1*** ^3%s^5: Foliage system using custom max range of \"^7%.4f^5\".\n", "FOLIAGE", CUSTOM_FOLIAGE_MAX_DISTANCE);
			}

#define		ANY_AREA_MODEL_POSITION		46
#define		RARE_MODELS_START			47

			for (i = 0; i < ANY_AREA_MODEL_POSITION; i++)
			{
				char temp[128] = { 0 };
				strcpy(temp, IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", va("openAreaFoliageModel%i", i), ""));
				float customScale = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", va("openAreaFoliageModelScale%i", i), "1.0"));

				if (temp && temp[0] != 0 && strlen(temp) > 0)
				{// Exists...
					// Add it to temp list, so we can use randoms from it if they don't add a full list...
					strcpy(customNormalFoliageModelsTempList[customRealNormalTempModelsAdded], temp);
					customNormalFoliageModelsTempScalesList[customRealNormalTempModelsAdded] = customScale;
					customRealNormalTempModelsAdded++;
					// And add it to the real list...
					strcpy(customNormalFoliageModelsTempList[i], temp);
					CUSTOM_FOLIAGE_SCALES[i] = customScale;
				}
				else if (customRealNormalTempModelsAdded > 0)
				{// Doesn't exist, so pick one from the previously added list...
					int choice = irand(0, customRealNormalTempModelsAdded - 1);
					strcpy(CustomFoliageModelsList[i], customNormalFoliageModelsTempList[choice]);
					CUSTOM_FOLIAGE_SCALES[i] = customNormalFoliageModelsTempScalesList[choice];
				}
			}

			int upto = 0;
			for (i = RARE_MODELS_START; i < MAX_PLANT_MODELS; i++, upto++)
			{
				char temp[128] = { 0 };
				strcpy(temp, IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", va("rareFoliageModel%i", upto), ""));
				float customScale = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", va("rareFoliageModelScale%i", upto), "1.0"));

				if (temp && temp[0] != 0 && strlen(temp) > 0)
				{// Exists...
				 // Add it to temp list, so we can use randoms from it if they don't add a full list...
					strcpy(customRareFoliageModelsTempList[customRealRareTempModelsAdded], temp);
					customRareFoliageModelsTempScalesList[customRealRareTempModelsAdded] = customScale;
					customRealRareTempModelsAdded++;
					// And add it to the real list...
					strcpy(customRareFoliageModelsTempList[i], temp);
					CUSTOM_FOLIAGE_SCALES[i] = customScale;
				}
				else if (customRealRareTempModelsAdded > 0)
				{// Doesn't exist, so pick one from the previously added list...
					int choice = irand(0, customRealRareTempModelsAdded - 1);
					strcpy(CustomFoliageModelsList[i], customRareFoliageModelsTempList[choice]);
					CUSTOM_FOLIAGE_SCALES[i] = customRareFoliageModelsTempScalesList[choice];
				}
			}

			strcpy(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION], IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", "anyAreaFoliageModel", ""));
			CUSTOM_FOLIAGE_SCALES[ANY_AREA_MODEL_POSITION] = 1.0;

			if (!(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION] && CustomFoliageModelsList[ANY_AREA_MODEL_POSITION][0] != 0 && strlen(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION]) > 0))
			{// Sanity check, fallback to using either a rare or a normal model for the anyArea model, if there was none specified...
				if (customRealRareTempModelsAdded > 0)
				{// Have rares? Use one...
					int choice = irand(0, customRealRareTempModelsAdded - 1);
					strcpy(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION], customRareFoliageModelsTempList[choice]);
					CUSTOM_FOLIAGE_SCALES[i] = customRareFoliageModelsTempScalesList[choice];
				}
				else if (customRealNormalTempModelsAdded > 0)
				{// Have standards? Use one...
					int choice = irand(0, customRealNormalTempModelsAdded - 1);
					strcpy(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION], customNormalFoliageModelsTempList[choice]);
					CUSTOM_FOLIAGE_SCALES[i] = customNormalFoliageModelsTempScalesList[choice];
				}
			}

			// Sanity check, make sure we have filled all possible slots...
			if (customRealRareTempModelsAdded > 0)
			{// Had no Standards? Use Rares...
				for (i = 0; i < ANY_AREA_MODEL_POSITION; i++, upto++)
				{
					if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
					{
						int choice = irand(0, customRealRareTempModelsAdded - 1);
						strcpy(CustomFoliageModelsList[i], customRareFoliageModelsTempList[choice]);
						CUSTOM_FOLIAGE_SCALES[i] = customRareFoliageModelsTempScalesList[choice];
					}
				}
			}
			else if (customRealNormalTempModelsAdded > 0)
			{// Had no Rares? Use Standards...
				for (i = RARE_MODELS_START; i < MAX_PLANT_MODELS; i++, upto++)
				{
					if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
					{
						int choice = irand(0, customRealNormalTempModelsAdded - 1);
						strcpy(CustomFoliageModelsList[i], customNormalFoliageModelsTempList[choice]);
						CUSTOM_FOLIAGE_SCALES[i] = customNormalFoliageModelsTempScalesList[choice];
					}
				}
			}
			else if (!(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION] && CustomFoliageModelsList[ANY_AREA_MODEL_POSITION][0] != 0 && strlen(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION]) > 0))
			{// Had no rares or normal foliages, but we did have an anyArea model, use that for everything...
				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					if (!CustomFoliageModelsList[i] || CustomFoliageModelsList[i][0] == 0 || strlen(CustomFoliageModelsList[i]) <= 0)
					{
						strcpy(CustomFoliageModelsList[i], CustomFoliageModelsList[ANY_AREA_MODEL_POSITION]);
						CUSTOM_FOLIAGE_SCALES[i] = 1.0;
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
					}
				}
			}

			if (!(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION] && CustomFoliageModelsList[ANY_AREA_MODEL_POSITION][0] != 0 && strlen(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION]) > 0))
			{// Sanity check, still nothing for the anyArea model, default to basic grass model...
				strcpy(CustomFoliageModelsList[ANY_AREA_MODEL_POSITION], "models/warzone/plants/gcgrass01.md3");
				CUSTOM_FOLIAGE_SCALES[ANY_AREA_MODEL_POSITION] = 1.0;
			}

			for (i = 0; i < MAX_PLANT_MODELS; i++)
			{
				FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(CustomFoliageModelsList[i]);
			}
		}
		else
		{
			char *foliageSet = NULL;

			if (!strcmp(FOLIAGE_MODEL_SELECTION, "grass"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7grass^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(GrassyPlantsModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "grassonly"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7grassonly^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(GrassOnlyModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "tropical"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7tropical^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(TropicalPlantsModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "forest"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7forest^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(ForestPlantsModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "forest2"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7forest2^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(ForestPlants2ModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "fieldgrass"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7fieldgrass^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(FieldGrassModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "fieldgrassshrubs"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7fieldgrassshrubs^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(FieldGrassShrubsModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "mushroomforest"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7mushroomforest^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(MushroomForestModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "mushroomforest2"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7mushroomforest2^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(MushroomForest2ModelsList[i]);
				}
			}
			else if (!strcmp(FOLIAGE_MODEL_SELECTION, "ferns"))
			{
				trap->Print("^1*** ^3%s^5: Map grass selection using foliageSet option \"^7ferns^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(FernsModelsList[i]);
				}
			}
			else
			{
				trap->Print("^1*** ^3%s^5: No map grass selection found. Using default option \"^7tropical^5\".\n", "FOLIAGE", cgs.currentmapname);

				for (i = 0; i < MAX_PLANT_MODELS; i++)
				{
					FOLIAGE_PLANT_MODELS[i] = trap->R_RegisterModel(TropicalPlantsModelsList[i]);
				}
			}

			if (MAP_HAS_TREES)
			{
				// Read all the tree info from the new .climate ini files...
				TREE_SCALE_MULTIPLIER = atof(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", "treeScaleMultiplier", "1.0"));

				for (i = 0; i < 9; i++)
				{
					FOLIAGE_TREE_MODEL[i] = trap->R_RegisterModel(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", va("treeModel%i", i), ""));

					FOLIAGE_TREE_BILLBOARD_SHADER[i] = trap->R_RegisterShader(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", va("treeBillboardShader%i", i), ""));
					FOLIAGE_TREE_BILLBOARD_SIZE[i] = atof(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", va("treeBillboardSize%i", i), "128.0"));
					FOLIAGE_TREE_RADIUS[i] = atof(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", va("treeRadius%i", i), "24.0"));
					FOLIAGE_TREE_ZOFFSET[i] = atof(IniRead(va("climates/%s.climate", CURRENT_CLIMATE_OPTION), "TREES", va("treeZoffset%i", i), "-4.0"));
				}
			}
		}

		PLANT_SCALE_MULTIPLIER = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "FOLIAGE", "FOLIAGE_SCALE", "1.0"));

		FOLIAGE_INITIALIZED = qtrue;
	}

	if (!FOLIAGE_INITIALIZED || !IN_RANGE_AREAS_LIST) return;

	FOLIAGE_Check_CVar_Change();

	VectorCopy(cg.refdef.vieworg, viewOrg);
	VectorCopy(cg.refdef.viewangles, viewAngles);

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
	else
	{
		for (CURRENT_AREA = 0; CURRENT_AREA < IN_RANGE_AREAS_LIST_COUNT; CURRENT_AREA++)
		{// Draw plants last...
			int spot = 0;
			int CURRENT_AREA_ID = IN_RANGE_AREAS_LIST[CURRENT_AREA];

			for (spot = 0; spot < FOLIAGE_AREAS_LIST_COUNT[CURRENT_AREA_ID]; spot++)
			{
				FOLIAGE_AddToScreen(FOLIAGE_AREAS_LIST[CURRENT_AREA_ID][spot], FOLIAGE_PASS_PLANT);
			}
		}
	}
}

// =======================================================================================================================================
//
//                                                             Foliage Generation...
//
// =======================================================================================================================================

extern float aw_percent_complete;
extern char task_string1[255];
extern char task_string2[255];
extern char task_string3[255];
extern char last_node_added_string[255];
extern clock_t	aw_stage_start_time;

int GENFOLIAGE_ALLOW_MATERIAL = -1;

qboolean MaterialIsValidForGrass(int materialType)
{
	switch (materialType)
	{
	case MATERIAL_SHORTGRASS:		// 5					// manicured lawn
	case MATERIAL_LONGGRASS:		// 6					// long jungle grass
	case MATERIAL_MUD:				// 17					// wet soil
	case MATERIAL_DIRT:				// 7					// hard mud
		return qtrue;
		break;
	default:
		break;
	}

	if (GENFOLIAGE_ALLOW_MATERIAL >= 0 && materialType == GENFOLIAGE_ALLOW_MATERIAL) return qtrue;

	return qfalse;
}

qboolean FOLIAGE_FloorIsGrassAt(vec3_t org)
{
	trace_t tr;
	vec3_t org1, org2;
	float pitch = 0;

	VectorCopy(org, org1);
	VectorCopy(org, org2);
	org2[2] -= 20.0f;

	CG_Trace(&tr, org1, NULL, NULL, org2, -1, CONTENTS_SOLID | CONTENTS_TERRAIN);

	if (tr.startsolid || tr.allsolid)
	{
		return qfalse;
	}

	if (tr.fraction == 1.0)
	{
		return qfalse;
	}

	if (Distance(org, tr.endpos) >= 20.0)
	{
		return qfalse;
	}

	if (MaterialIsValidForGrass((int)(tr.materialType)))
		return qtrue;

	return qfalse;
}


#define GRASS_MODEL_WIDTH cg_foliageModelWidth.value
#define GRASS_SLOPE_MAX_DIFF cg_foliageMaxSlopeChange.value
#define GRASS_HEIGHT_MAX_DIFF cg_foliageMaxHeightChange.value

qboolean FOLIAGE_CheckFoliageAlready(vec3_t pos, float density)
{
	int spot = 0;
	int areaNum = FOLIAGE_AreaNumForOrg(pos);

	for (spot = 0; spot < FOLIAGE_AREAS_LIST_COUNT[areaNum]; spot++)
	{
		int THIS_FOLIAGE = FOLIAGE_AREAS_LIST[areaNum][spot];

		if (Distance/*Horizontal*/(pos, FOLIAGE_POSITIONS[THIS_FOLIAGE]) <= density)
			return qtrue;
	}

	return qfalse;
}

#define MAX_SLOPE 46.0

qboolean FOLIAGE_CheckSlope(vec3_t normal)
{
	float pitch;
	vec3_t slopeangles;
	vectoangles(normal, slopeangles);

	pitch = slopeangles[0];

	if (pitch > 180)
		pitch -= 360;

	if (pitch < -180)
		pitch += 360;

	pitch += 90.0f;

	if (pitch > MAX_SLOPE || pitch < -MAX_SLOPE)
		return qfalse;

	return qtrue;
}

qboolean FOLIAGE_CheckSlopeChange(vec3_t normal, vec3_t normal2)
{
	/*
	vec3_t slopeDiff;

	slopeDiff[0] = normal[0] - normal2[0];
	slopeDiff[1] = normal[1] - normal2[1];
	slopeDiff[2] = normal[2] - normal2[2];

	if (slopeDiff[0] > GRASS_SLOPE_MAX_DIFF) return qfalse;
	if (slopeDiff[0] < -GRASS_SLOPE_MAX_DIFF) return qfalse;

	if (slopeDiff[1] > GRASS_SLOPE_MAX_DIFF) return qfalse;
	if (slopeDiff[1] < -GRASS_SLOPE_MAX_DIFF) return qfalse;

	if (slopeDiff[2] > GRASS_SLOPE_MAX_DIFF) return qfalse;
	if (slopeDiff[2] < -GRASS_SLOPE_MAX_DIFF) return qfalse;
	*/

	float pitch1, pitch2;
	vec3_t slopeAngles1, slopeAngles2;

	vectoangles(normal, slopeAngles1);

	pitch1 = slopeAngles1[0];

	if (pitch1 > 180)
		pitch1 -= 360;

	if (pitch1 < -180)
		pitch1 += 360;

	pitch1 += 90.0f;

	vectoangles(normal2, slopeAngles2);

	pitch2 = slopeAngles2[0];

	if (pitch2 > 180)
		pitch2 -= 360;

	if (pitch2 < -180)
		pitch2 += 360;

	pitch2 += 90.0f;

	if (pitch1 - pitch2 > GRASS_SLOPE_MAX_DIFF || pitch1 - pitch2 < -GRASS_SLOPE_MAX_DIFF)
		return qfalse;

	return qtrue;
}

qboolean FOLIAGE_CheckSlopesAround(vec3_t groundpos, vec3_t slope, float scale)
{
	trace_t		tr;
	vec3_t		pos, down;
	vec3_t		pos2, down2;

	VectorCopy(groundpos, pos);
	pos[2] += 96.0;
	VectorCopy(groundpos, down);
	down[2] -= 96.0;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[0] += (GRASS_MODEL_WIDTH * scale);
	down2[0] += (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[0] -= (GRASS_MODEL_WIDTH * scale);
	down2[0] -= (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[1] += (GRASS_MODEL_WIDTH * scale);
	down2[1] += (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[1] -= (GRASS_MODEL_WIDTH * scale);
	down2[1] -= (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[0] += (GRASS_MODEL_WIDTH * scale);
	pos2[1] += (GRASS_MODEL_WIDTH * scale);
	down2[0] += (GRASS_MODEL_WIDTH * scale);
	down2[1] += (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[0] += (GRASS_MODEL_WIDTH * scale);
	pos2[1] -= (GRASS_MODEL_WIDTH * scale);
	down2[0] += (GRASS_MODEL_WIDTH * scale);
	down2[1] -= (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[0] -= (GRASS_MODEL_WIDTH * scale);
	pos2[1] += (GRASS_MODEL_WIDTH * scale);
	down2[0] -= (GRASS_MODEL_WIDTH * scale);
	down2[1] += (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	VectorCopy(pos, pos2);
	VectorCopy(down, down2);
	pos2[0] -= (GRASS_MODEL_WIDTH * scale);
	pos2[1] -= (GRASS_MODEL_WIDTH * scale);
	down2[0] -= (GRASS_MODEL_WIDTH * scale);
	down2[1] -= (GRASS_MODEL_WIDTH * scale);

	CG_Trace(&tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

	// Slope too different...
	if (tr.fraction == 1.0) return qfalse; // Didn't hit anything... Started trace in floor???
	if (!MaterialIsValidForGrass((int)(tr.materialType))) return qfalse;
	if (!FOLIAGE_CheckSlope(tr.plane.normal)) return qfalse;
	if (!FOLIAGE_CheckSlopeChange(slope, tr.plane.normal)) return qfalse;

	return qtrue;
}

float RoofHeightAbove(vec3_t org)
{
	trace_t tr;
	vec3_t org1, org2;
	//	float height = 0;

	VectorCopy(org, org1);
	org1[2] += 16;

	VectorCopy(org, org2);
	org2[2] = 131072.0f;

	CG_Trace(&tr, org1, NULL, NULL, org2, cg.clientNum, MASK_PLAYERSOLID);

	if (tr.startsolid || tr.allsolid)
	{
		//trap->Print("start or allsolid.\n");
		return -131072.0f;
	}

	if (tr.materialType == MATERIAL_TREEBARK
		|| MaterialIsValidForGrass(tr.materialType)
		|| tr.materialType == MATERIAL_SAND
		|| tr.materialType == MATERIAL_ROCK
		|| (tr.contents & CONTENTS_TRANSLUCENT))
	{// Exception for hitting trees...
		return org[2] + 257.0;
	}

	return tr.endpos[2];
}

qboolean FOLIAGE_IsIndoorLocation(vec3_t origin)
{
	if (RoofHeightAbove(origin) - origin[2] <= 256.0)
	{// This must be indoors...
		return qtrue;
	}

	return qfalse;
}

qboolean FOLIAGE_FxExistsNearPoint(vec3_t origin)
{
	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		centity_t *cent = &cg_entities[i];

		if (!cent) continue;

		if (cent->currentState.eType == ET_FX)
		{
			if (Distance(origin, cent->lerpOrigin) < 128.0)
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

#define SPOT_TYPE_TREE 1
#define SPOT_TYPE_PLANT 2
#define SPOT_TYPE_GRASS 3

void FOLIAGE_GenerateFoliage_Real(float scan_density, int plant_chance, int tree_chance, int num_clearings, float check_density, qboolean ADD_MORE, qboolean MULTILEVEL)
{
	int				i;
	float			startx = -131072, starty = -131072, startz = -131072;
	int				grassSpotCount = 0;
	vec3_t			*grassSpotList;
	vec3_t			*grassNormals;
	float			*grassSpotScale;
	int				*grassSpotType;
	float			map_size, temp;
	vec3_t			mapMins, mapMaxs;
	int				start_time = trap->Milliseconds();
	int				update_timer = 0;
	clock_t			previous_time = 0;
	float			offsetY = 0.0;
	float			yoff;
	int				NUM_FAILS = 0;
	vec3_t			MAP_INFO_SIZE;
	vec3_t			MAP_SCALE;
	vec3_t			CLEARING_SPOTS[FOLIAGE_AREA_MAX / 8] = { 0 };
	int				CLEARING_SPOTS_SIZES[FOLIAGE_AREA_MAX / 8] = { 0 };
	int				CLEARING_SPOTS_NUM = 0;
	//int x;
	int				numTrees = 0;
	int				numPlants = 0;
	int				numGrass = 0;

	if (FOLIAGE_IgnoreFoliageOnMap())
	{// Ignore this map... We know we don't need grass here...
		FOLIAGE_NUM_POSITIONS = 0;
		FOLIAGE_LOADED = qtrue;
		return;
	}

	RoadExistsAtPoint(vec3_origin); // Just to make sure that road map is pre-loaded...

	trap->Print("^1*** ^3%s^5: Generate foliage settings...\n", "AUTO-FOLIAGE");
	trap->Print("^1*** ^3%s^5: scan_density: %f. plant_chance %i. tree_chance %i. num_clearings %i. check_density %f. ADD_MORE %s...\n", "AUTO-FOLIAGE", scan_density, plant_chance, tree_chance, num_clearings, check_density, ADD_MORE ? "true" : "false");
	trap->UpdateScreen();

	trap->Print("^1*** ^3%s^5: Finding map bounds...\n", "AUTO-FOLIAGE");
	trap->UpdateScreen();

	AIMod_GetMapBounts();

	if (!cg.mapcoordsValid)
	{
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Map Coordinates are invalid. Can not use auto-waypointer!\n");
		return;
	}

	if (FOLIAGE_POSITIONS && ADD_MORE)
	{// Dynamically re-alloc the max possible memory...
		FOLIAGE_POSITIONS = (vec3_t *)realloc(FOLIAGE_POSITIONS, FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_NORMALS = (vec3_t *)realloc(FOLIAGE_NORMALS, FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_PLANT_SELECTION = (int *)realloc(FOLIAGE_PLANT_SELECTION, FOLIAGE_MAX_FOLIAGES * sizeof(int));
		FOLIAGE_PLANT_ANGLE = (float *)realloc(FOLIAGE_PLANT_ANGLE, FOLIAGE_MAX_FOLIAGES * sizeof(float));
		FOLIAGE_PLANT_ANGLES = (vec3_t *)realloc(FOLIAGE_PLANT_ANGLES, FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_PLANT_AXIS = (matrix3_t *)realloc(FOLIAGE_PLANT_AXIS, FOLIAGE_MAX_FOLIAGES * sizeof(matrix3_t));
		FOLIAGE_PLANT_SCALE = (float *)realloc(FOLIAGE_PLANT_SCALE, FOLIAGE_MAX_FOLIAGES * sizeof(float));

		if (MAP_HAS_TREES)
		{
			FOLIAGE_TREE_SELECTION = (int *)realloc(FOLIAGE_TREE_SELECTION, FOLIAGE_MAX_FOLIAGES * sizeof(int));
			FOLIAGE_TREE_ANGLE = (float *)realloc(FOLIAGE_TREE_ANGLE, FOLIAGE_MAX_FOLIAGES * sizeof(float));
			FOLIAGE_TREE_ANGLES = (vec3_t *)realloc(FOLIAGE_TREE_ANGLES, FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
			FOLIAGE_TREE_AXIS = (matrix3_t *)realloc(FOLIAGE_TREE_AXIS, FOLIAGE_MAX_FOLIAGES * sizeof(matrix3_t));
			FOLIAGE_TREE_BILLBOARD_ANGLES = (vec3_t *)realloc(FOLIAGE_TREE_BILLBOARD_ANGLES, FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
			FOLIAGE_TREE_BILLBOARD_AXIS = (matrix3_t *)realloc(FOLIAGE_TREE_BILLBOARD_AXIS, FOLIAGE_MAX_FOLIAGES * sizeof(matrix3_t));
			FOLIAGE_TREE_SCALE = (float *)realloc(FOLIAGE_TREE_SCALE, FOLIAGE_MAX_FOLIAGES * sizeof(float));
		}
	}
	else
	{// Not currently allocated, just alloc max possible memory...
		FOLIAGE_FreeMemory();
		FOLIAGE_POSITIONS = (vec3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_NORMALS = (vec3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_PLANT_SELECTION = (int *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(int));
		FOLIAGE_PLANT_ANGLE = (float *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(float));
		FOLIAGE_PLANT_ANGLES = (vec3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_PLANT_AXIS = (matrix3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(matrix3_t));
		FOLIAGE_PLANT_SCALE = (float *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(float));
		FOLIAGE_TREE_SELECTION = (int *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(int));
		FOLIAGE_TREE_ANGLE = (float *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(float));
		FOLIAGE_TREE_ANGLES = (vec3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_TREE_AXIS = (matrix3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(matrix3_t));
		FOLIAGE_TREE_BILLBOARD_ANGLES = (vec3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(vec3_t));
		FOLIAGE_TREE_BILLBOARD_AXIS = (matrix3_t *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(matrix3_t));
		FOLIAGE_TREE_SCALE = (float *)malloc(FOLIAGE_MAX_FOLIAGES * sizeof(float));

		if (!FOLIAGE_AREAS_LIST_COUNT) FOLIAGE_AREAS_LIST_COUNT = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_LIST) FOLIAGE_AREAS_LIST = (ivec256_t *)malloc(sizeof(ivec256_t) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_MINS) FOLIAGE_AREAS_MINS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_MAXS) FOLIAGE_AREAS_MAXS = (vec3_t *)malloc(sizeof(vec3_t) * FOLIAGE_AREA_MAX);

		if (!IN_RANGE_AREAS_LIST) IN_RANGE_AREAS_LIST = (int *)malloc(sizeof(int) * 8192);
		if (!IN_RANGE_AREAS_DISTANCE) IN_RANGE_AREAS_DISTANCE = (float *)malloc(sizeof(float) * 8192);

		if (!FOLIAGE_AREAS_TREES_LIST_COUNT) FOLIAGE_AREAS_TREES_LIST_COUNT = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_TREES_VISCHECK_TIME) FOLIAGE_AREAS_TREES_VISCHECK_TIME = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_TREES_VISCHECK_RESULT) FOLIAGE_AREAS_TREES_VISCHECK_RESULT = (qboolean *)malloc(sizeof(qboolean) * FOLIAGE_AREA_MAX);
		if (!FOLIAGE_AREAS_TREES_LIST) FOLIAGE_AREAS_TREES_LIST = (ivec256_t *)malloc(sizeof(ivec256_t) * FOLIAGE_AREA_MAX);

		if (!IN_RANGE_TREE_AREAS_LIST) IN_RANGE_TREE_AREAS_LIST = (int *)malloc(sizeof(int) * FOLIAGE_AREA_MAX);
		if (!IN_RANGE_TREE_AREAS_DISTANCE) IN_RANGE_TREE_AREAS_DISTANCE = (float *)malloc(sizeof(float) * FOLIAGE_AREA_MAX);

		memset(FOLIAGE_PLANT_SELECTION, 0, FOLIAGE_MAX_FOLIAGES * sizeof(int));
		if (FOLIAGE_TREE_SELECTION) memset(FOLIAGE_TREE_SELECTION, 0, FOLIAGE_MAX_FOLIAGES * sizeof(int));

		FOLIAGE_NUM_POSITIONS = 0;
	}

	i = 0;
	

	grassSpotList = (vec3_t *)malloc((sizeof(vec3_t))*FOLIAGE_MAX_FOLIAGES);
	grassNormals = (vec3_t *)malloc((sizeof(vec3_t))*FOLIAGE_MAX_FOLIAGES);
	grassSpotScale = (float *)malloc((sizeof(float))*FOLIAGE_MAX_FOLIAGES);
	grassSpotType = (int *)malloc((sizeof(int))*FOLIAGE_MAX_FOLIAGES);

	if (!grassSpotList)
	{
		FOLIAGE_FreeMemory();
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Failed to allocate grassSpotList! Low memory!\n");
		return;
	}

	if (!grassNormals)
	{
		FOLIAGE_FreeMemory();
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Failed to allocate grassNormals! Low memory!\n");
		return;
	}

	if (!grassSpotScale)
	{
		FOLIAGE_FreeMemory();
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Failed to allocate grassSpotScale! Low memory!\n");
		return;
	}

	if (!grassSpotType)
	{
		FOLIAGE_FreeMemory();
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Failed to allocate grassSpotType! Low memory!\n");
		return;
	}

	memset(grassSpotList, 0, (sizeof(vec3_t))*FOLIAGE_MAX_FOLIAGES);
	memset(grassNormals, 0, (sizeof(vec3_t))*FOLIAGE_MAX_FOLIAGES);
	memset(grassSpotScale, 0, (sizeof(float))*FOLIAGE_MAX_FOLIAGES);
	memset(grassSpotType, 0, (sizeof(int))*FOLIAGE_MAX_FOLIAGES);

	VectorCopy(cg.mapcoordsMins, mapMins);
	VectorCopy(cg.mapcoordsMaxs, mapMaxs);

	if (mapMaxs[0] < mapMins[0])
	{
		temp = mapMins[0];
		mapMins[0] = mapMaxs[0];
		mapMaxs[0] = temp;
	}

	if (mapMaxs[1] < mapMins[1])
	{
		temp = mapMins[1];
		mapMins[1] = mapMaxs[1];
		mapMaxs[1] = temp;
	}

	if (mapMaxs[2] < mapMins[2])
	{
		temp = mapMins[2];
		mapMins[2] = mapMaxs[2];
		mapMaxs[2] = temp;
	}

	trap->S_Shutup(qtrue);

	startx = mapMaxs[0];
	starty = mapMaxs[1];
	startz = mapMaxs[2];

	map_size = Distance(mapMins, mapMaxs);

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Map bounds are ^3%.2f %.2f %.2f ^5to ^3%.2f %.2f %.2f^5.\n", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]));
	strcpy(task_string1, va("^5Map bounds are ^3%.2f %.2f %.2f ^7to ^3%.2f %.2f %.2f^5.", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]));

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Generating foliage points. This could take a while... (Map size ^3%.2f^5)\n", map_size));
	strcpy(task_string2, va("^5Generating foliage points. This could take a while... (Map size ^3%.2f^5)", map_size));

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Finding foliage points...\n"));
	strcpy(task_string3, va("^5Finding foliage points..."));

	trap->UpdateScreen();

	//
	// Create bulk temporary nodes...
	//

	previous_time = clock();
	aw_stage_start_time = clock();
	aw_percent_complete = 0;

	grassSpotCount = 0;

	// Create the map...
	MAP_INFO_SIZE[0] = mapMaxs[0] - mapMins[0];
	MAP_INFO_SIZE[1] = mapMaxs[1] - mapMins[1];
	MAP_INFO_SIZE[2] = mapMaxs[2] - mapMins[2];

	MAP_SCALE[0] = mapMaxs[0] - mapMins[0];
	MAP_SCALE[1] = mapMaxs[1] - mapMins[1];
	MAP_SCALE[2] = 0;

	yoff = scan_density * 1.25;

	trap->Print("^1*** ^3%s^5: Searching for clearing positions...\n", "AUTO-FOLIAGE");
	trap->UpdateScreen();

	while (CLEARING_SPOTS_NUM < num_clearings && NUM_FAILS < 50)
	{
		int			d = 0;
		vec3_t		spot;
		qboolean	IS_CLEARING = qfalse;

		spot[0] = mapMins[0] + (irand(0, MAP_SCALE[0] / 2) + irand(0, MAP_SCALE[0] / 2));
		spot[1] = mapMins[1] + (irand(0, MAP_SCALE[1] / 2) + irand(0, MAP_SCALE[1] / 2));
		spot[2] = 0;

		for (d = 0; d < CLEARING_SPOTS_NUM; d++)
		{
			if (DistanceHorizontal(CLEARING_SPOTS[d], spot) < CLEARING_SPOTS_SIZES[d])
			{
				IS_CLEARING = qtrue;
				continue;
			}
		}

		if (IS_CLEARING)
		{
			NUM_FAILS++;
			continue;
		}

		// Ok, not in range of another one, add this spot to the list...
		VectorSet(CLEARING_SPOTS[CLEARING_SPOTS_NUM], spot[0], spot[1], 0.0);
		CLEARING_SPOTS_SIZES[CLEARING_SPOTS_NUM] = irand(1024, 2048);
		CLEARING_SPOTS_NUM++;
	}


	trap->Print("^1*** ^3%s^5: Generated %i clearing positions...\n", "AUTO-FOLIAGE", CLEARING_SPOTS_NUM);
	trap->UpdateScreen();


	for (int x = (int)mapMins[0]; x <= (int)mapMaxs[0]; x += scan_density)
	{
		float current;
		float complete;

		if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
		{
			break;
		}

		current = MAP_INFO_SIZE[0] - (mapMaxs[0] - (float)x);
		complete = current / MAP_INFO_SIZE[0];

		aw_percent_complete = (float)(complete * 100.0);

		if (yoff == scan_density * 0.75)
			yoff = scan_density * 1.25;
		else if (yoff == scan_density * 1.25)
			yoff = scan_density;
		else
			yoff = scan_density * 0.75;

#pragma omp parallel for schedule(dynamic)
		for (int y = (int)mapMins[1]; y <= (int)mapMaxs[1]; y += yoff)
		{
			if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
			{
				break;
			}

			qboolean hitInvalid = qfalse;

			for (float z = mapMaxs[2]; z >= mapMins[2]; z -= 48.0)
			{
				trace_t		tr;
				vec3_t		pos, down;
				qboolean	FOUND = qfalse;

				if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
				{
					break;
				}

				if(omp_get_thread_num() == 0)
				{// Draw a nice little progress bar ;)
					if (clock() - previous_time > 500) // update display every 500ms...
					{
						previous_time = clock();
						trap->UpdateScreen();
					}
				}

				VectorSet(pos, x, y, z);
				pos[2] += 8.0;
				VectorCopy(pos, down);
				down[2] = mapMins[2];

				if (RoadExistsAtPoint(pos))
				{// There's a road here...
					if (MULTILEVEL)
					{
						continue;
					}
					else
					{
						break;
					}
				}

				CG_Trace(&tr, pos, NULL, NULL, down, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME);

				if (tr.endpos[2] <= mapMins[2])
				{// Went off map...
					break;
				}

				if (tr.surfaceFlags & SURF_SKY)
				{// Sky...
					continue;
				}

				if (tr.contents & CONTENTS_WATER)
				{// Anything below here is underwater...
					break;
				}

				if (tr.contents & CONTENTS_LAVA)
				{// Anything below here is under lava...
					break;
				}

				if (tr.surfaceFlags & SURF_NODRAW)
				{// don't generate a drawsurface at all
					if (MULTILEVEL)
					{
						hitInvalid = qtrue;
						continue;
					}
					else
					{
						break;
					}
				}

				if (tr.contents & CONTENTS_TRANSLUCENT)
				{// don't generate a drawsurface at all
					//if (MULTILEVEL)
					{
						hitInvalid = qtrue;
						continue;
					}
					/*else
					{
						break;
					}*/
				}

				if (ADD_MORE && check_density > 0 && FOLIAGE_CheckFoliageAlready(tr.endpos, check_density))
				{// Already a foliage here...
					if (MULTILEVEL)
					{
						hitInvalid = qtrue;
						continue;
					}
					else
					{
						break;
					}
				}

				if (MaterialIsValidForGrass((tr.materialType)))
				{
#if 0
					if (hitInvalid)
					{// Scan above us for a roof... So we don't add grass inside of buildings...
						if (FOLIAGE_IsIndoorLocation(tr.endpos))
						{// This must be indoors...
							break;
						}

						{
							vec3_t o;
							VectorSet(o, tr.endpos[0] + 64.0, tr.endpos[1], tr.endpos[2] + 16.0);

							if (FOLIAGE_IsIndoorLocation(o))
							{
								break;
							}

							VectorSet(o, tr.endpos[0] - 64.0, tr.endpos[1], tr.endpos[2] + 16.0);

							if (FOLIAGE_IsIndoorLocation(o))
							{
								break;
							}

							VectorSet(o, tr.endpos[0], tr.endpos[1] + 64.0, tr.endpos[2] + 16.0);

							if (FOLIAGE_IsIndoorLocation(o))
							{
								break;
							}

							VectorSet(o, tr.endpos[0], tr.endpos[1] - 64.0, tr.endpos[2] + 16.0);

							if (FOLIAGE_IsIndoorLocation(o))
							{
								break;
							}
						}
					}
#endif

					if (FOLIAGE_FxExistsNearPoint(tr.endpos))
					{
						if (MULTILEVEL)
						{
							hitInvalid = qtrue;
							continue;
						}
						else
						{
							break;
						}
					}

					qboolean DO_TREE = qfalse;
					qboolean DO_PLANT = qfalse;
					qboolean IS_CLEARING = qfalse;

					if (tree_chance > 0 && num_clearings > 0 && CLEARING_SPOTS_NUM > 0)
					{
						int d = 0;
						int variation = irand(0, 2048); /* Variation around edges */

						for (d = 0; d < CLEARING_SPOTS_NUM; d++)
						{
							if (DistanceHorizontal(CLEARING_SPOTS[d], tr.endpos) <= CLEARING_SPOTS_SIZES[d] + variation)
							{
								IS_CLEARING = qtrue;
								break;
							}
						}
					}

					if (!IS_CLEARING
						&& tree_chance > 0
						&& irand_big(0, tree_chance) >= tree_chance)
					{
						if (RoofHeightAbove(tr.endpos) - tr.endpos[2] > 1024.0)
						{
							DO_TREE = qtrue;
						}
						else if (plant_chance > 0 
							&& irand_big(0, plant_chance) >= plant_chance)
						{
							DO_PLANT = qtrue;
							//trap->Print("Failed tree because of roof height. Adding plant instead.\n");
						}
						else
						{
							//trap->Print("Failed tree because of roof height.\n");
						}
					}
					else if (plant_chance > 0 
						&& irand_big(0, plant_chance) >= plant_chance)
					{
						DO_PLANT = qtrue;
					}
					else if (plant_chance == 1)
					{
						DO_PLANT = qtrue;
					}

					if (!DO_TREE && !DO_PLANT) continue;

#if 0
					// Look around here for a different slope angle... Cull if found...
					for (float scale = 1.00; scale >= 0.05 && scale >= cg_foliageMinFoliageScale.value; scale -= 0.05)
					{
						if (scale >= cg_foliageMinFoliageScale.value && FOLIAGE_CheckSlopesAround(tr.endpos, tr.plane.normal, scale))
						{
							#pragma omp critical (__ADD_TEMP_NODE__)
							{
								VectorCopy(tr.plane.normal, grassNormals[grassSpotCount]);

								if (DO_TREE)
								{
									grassSpotType[grassSpotCount] = SPOT_TYPE_TREE;
									numTrees++;
								}
								else if (DO_PLANT)
								{
									grassSpotType[grassSpotCount] = SPOT_TYPE_PLANT;
									numPlants++;
								}
								else
								{
									grassSpotType[grassSpotCount] = SPOT_TYPE_GRASS;
									numGrass++;
								}

								grassSpotScale[grassSpotCount] = scale;
								VectorSet(grassSpotList[grassSpotCount], tr.endpos[0], tr.endpos[1], tr.endpos[2] + 8);

								sprintf(last_node_added_string, "^5Adding potential foliage point ^3%i ^5at ^7%i %i %i^5. Trees ^7%i^5. Plants ^7%i^5. Grasses ^7%i^5.", grassSpotCount, (int)grassSpotList[grassSpotCount][0], (int)grassSpotList[grassSpotCount][1], (int)grassSpotList[grassSpotCount][2], numTrees, numPlants, numGrass);

								grassSpotCount++;
								FOUND = qtrue;
							}
						}

						if (FOUND) break;
					}
#else
#pragma omp critical (__ADD_TEMP_NODE__)
					{
						VectorCopy(tr.plane.normal, grassNormals[grassSpotCount]);

						if (DO_TREE)
						{
							grassSpotType[grassSpotCount] = SPOT_TYPE_TREE;
							numTrees++;
						}
						else if (DO_PLANT)
						{
							grassSpotType[grassSpotCount] = SPOT_TYPE_PLANT;
							numPlants++;
						}
						else
						{
							grassSpotType[grassSpotCount] = SPOT_TYPE_GRASS;
							numGrass++;
						}

						grassSpotScale[grassSpotCount] = 1.0;
						VectorSet(grassSpotList[grassSpotCount], tr.endpos[0], tr.endpos[1], tr.endpos[2] + 8);

						sprintf(last_node_added_string, "^5Adding potential foliage point ^3%i ^5at ^7%i %i %i^5. Trees ^7%i^5. Plants ^7%i^5. Grasses ^7%i^5.", grassSpotCount, (int)grassSpotList[grassSpotCount][0], (int)grassSpotList[grassSpotCount][1], (int)grassSpotList[grassSpotCount][2], numTrees, numPlants, numGrass);

						grassSpotCount++;
						FOUND = qtrue;
					}
#endif

					if (FOUND) break;
				}
				else
				{
					hitInvalid = qtrue;
				}
			}
		}
	}

	if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
	{
		trap->Print("^1*** ^3%s^5: Too many foliage points detected... Try again with a higher density value...\n", "AUTO-FOLIAGE");
		trap->S_Shutup(qfalse);
		aw_percent_complete = 0.0f;
		trap->UpdateScreen();

		// May as well free the memory as well...
		FOLIAGE_FreeMemory();
		return;
	}

	trap->S_Shutup(qfalse);

	aw_percent_complete = 0.0f;

	if (!ADD_MORE)
	{
		FOLIAGE_NUM_POSITIONS = 0;
	}

	if (grassSpotCount > 0)
	{// Ok, we have spots, copy and set up foliage types/scales/angles, and save to file...
		for (i = 0; i < grassSpotCount; i++)
		{
			qboolean DO_TREE = qfalse;
			qboolean DO_PLANT = qfalse;

			if (MAP_HAS_TREES)
			{
				FOLIAGE_TREE_SELECTION[FOLIAGE_NUM_POSITIONS] = 0;
				FOLIAGE_TREE_ANGLE[FOLIAGE_NUM_POSITIONS] = 0.0f;
				FOLIAGE_TREE_SCALE[FOLIAGE_NUM_POSITIONS] = 0.0f;
			}

			FOLIAGE_PLANT_SELECTION[FOLIAGE_NUM_POSITIONS] = 0;
			FOLIAGE_PLANT_ANGLE[FOLIAGE_NUM_POSITIONS] = irand(0, 180);

			VectorCopy(grassSpotList[i], FOLIAGE_POSITIONS[FOLIAGE_NUM_POSITIONS]);
			FOLIAGE_POSITIONS[FOLIAGE_NUM_POSITIONS][2] -= 18.0;

			VectorCopy(grassNormals[i], FOLIAGE_NORMALS[FOLIAGE_NUM_POSITIONS]);

			if (MAP_HAS_TREES && grassSpotType[i] == SPOT_TYPE_TREE)
			{// Add tree...
				FOLIAGE_TREE_SELECTION[FOLIAGE_NUM_POSITIONS] = irand(1, NUM_TREE_TYPES);
				FOLIAGE_TREE_ANGLE[FOLIAGE_NUM_POSITIONS] = (int)(random() * 180);
				FOLIAGE_TREE_SCALE[FOLIAGE_NUM_POSITIONS] = (float)((float)irand(65, 150) / 100.0);
			}
			else if (grassSpotType[i] == SPOT_TYPE_PLANT)
			{// Add plant...
				FOLIAGE_PLANT_SELECTION[FOLIAGE_NUM_POSITIONS] = irand(1, MAX_PLANT_MODELS);
				FOLIAGE_PLANT_SCALE[FOLIAGE_NUM_POSITIONS] = grassSpotScale[i];
			}
			else
			{
				FOLIAGE_PLANT_SCALE[FOLIAGE_NUM_POSITIONS] = grassSpotScale[i];
			}

			FOLIAGE_NUM_POSITIONS++;
		}

		trap->Print("^1*** ^3%s^5: Successfully generated %i foliage points...\n", "AUTO-FOLIAGE", FOLIAGE_NUM_POSITIONS);

		// Re-alloc our memory back to the actual needed amount, to save ram...
		FOLIAGE_POSITIONS = (vec3_t *)realloc(FOLIAGE_POSITIONS, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_NORMALS = (vec3_t *)realloc(FOLIAGE_NORMALS, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_PLANT_SELECTION = (int *)realloc(FOLIAGE_PLANT_SELECTION, FOLIAGE_NUM_POSITIONS * sizeof(int));
		FOLIAGE_PLANT_ANGLE = (float *)realloc(FOLIAGE_PLANT_ANGLE, FOLIAGE_NUM_POSITIONS * sizeof(float));
		FOLIAGE_PLANT_ANGLES = (vec3_t *)realloc(FOLIAGE_PLANT_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
		FOLIAGE_PLANT_AXIS = (matrix3_t *)realloc(FOLIAGE_PLANT_AXIS, FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t));
		FOLIAGE_PLANT_SCALE = (float *)realloc(FOLIAGE_PLANT_SCALE, FOLIAGE_NUM_POSITIONS * sizeof(float));

		MAP_HAS_TREES = qfalse;

		for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
		{
			if (FOLIAGE_TREE_SELECTION[i] > 0)
			{
				MAP_HAS_TREES = qtrue;
				break;
			}
		}

		if (MAP_HAS_TREES)
		{
			FOLIAGE_TREE_SELECTION = (int *)realloc(FOLIAGE_TREE_SELECTION, FOLIAGE_NUM_POSITIONS * sizeof(int));
			FOLIAGE_TREE_ANGLE = (float *)realloc(FOLIAGE_TREE_ANGLE, FOLIAGE_NUM_POSITIONS * sizeof(float));
			FOLIAGE_TREE_ANGLES = (vec3_t *)realloc(FOLIAGE_TREE_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
			FOLIAGE_TREE_AXIS = (matrix3_t *)realloc(FOLIAGE_TREE_AXIS, FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t));
			FOLIAGE_TREE_BILLBOARD_ANGLES = (vec3_t *)realloc(FOLIAGE_TREE_BILLBOARD_ANGLES, FOLIAGE_NUM_POSITIONS * sizeof(vec3_t));
			FOLIAGE_TREE_BILLBOARD_AXIS = (matrix3_t *)realloc(FOLIAGE_TREE_BILLBOARD_AXIS, FOLIAGE_NUM_POSITIONS * sizeof(matrix3_t));
			FOLIAGE_TREE_SCALE = (float *)realloc(FOLIAGE_TREE_SCALE, FOLIAGE_NUM_POSITIONS * sizeof(float));
		}

		// Save the generated info to a file for next time...
		FOLIAGE_SaveFoliagePositions();
	}
	else
	{
		trap->Print("^1*** ^3%s^5: Did not find any grass points on this map...\n", "AUTO-FOLIAGE");
		FOLIAGE_FreeMemory();
	}

	free(grassSpotList);

	//FOLIAGE_LOADED = qtrue;
}

extern qboolean CPU_CHECKED;
extern int UQ_Get_CPU_Info(void);

void FOLIAGE_FoliageRescale(void)
{
	int			update_timer = 0;
	clock_t		previous_time = 0;
	vec3_t		mapMins, mapMaxs;
	float		temp;
	int			NUM_RESCALED = 0;
	int			i = 0;

	AIMod_GetMapBounts();

	if (!cg.mapcoordsValid)
	{
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Map Coordinates are invalid. Can not use auto-waypointer!\n");
		return;
	}

	VectorCopy(cg.mapcoordsMins, mapMins);
	VectorCopy(cg.mapcoordsMaxs, mapMaxs);

	if (mapMaxs[0] < mapMins[0])
	{
		temp = mapMins[0];
		mapMins[0] = mapMaxs[0];
		mapMaxs[0] = temp;
	}

	if (mapMaxs[1] < mapMins[1])
	{
		temp = mapMins[1];
		mapMins[1] = mapMaxs[1];
		mapMaxs[1] = temp;
	}

	if (mapMaxs[2] < mapMins[2])
	{
		temp = mapMins[2];
		mapMins[2] = mapMaxs[2];
		mapMaxs[2] = temp;
	}

	previous_time = clock();
	aw_stage_start_time = clock();
	aw_percent_complete = 0;

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Map bounds are ^3%.2f %.2f %.2f ^5to ^3%.2f %.2f %.2f^5.\n", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]));
	strcpy(task_string1, va("^5Map bounds are ^3%.2f %.2f %.2f ^7to ^3%.2f %.2f %.2f^5.", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]));
	trap->UpdateScreen();

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Rescaling foliage points. This could take a while...\n"));
	strcpy(task_string2, va("^5Rescaling foliage points. This could take a while..."));
	trap->UpdateScreen();

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Rescaling foliage points...\n"));
	strcpy(task_string3, va("^5Rescaling foliage points..."));
	trap->UpdateScreen();

	trap->UpdateScreen();

	trap->S_Shutup(qtrue);

	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		vec3_t		pos, down;
		trace_t		tr;
		float		scale = 1.00;

		aw_percent_complete = (float)((float)i / (float)FOLIAGE_NUM_POSITIONS) * 100.0;

		if (clock() - previous_time > 500) // update display every 500ms...
		{
			previous_time = clock();
			trap->UpdateScreen();
		}

		VectorCopy(FOLIAGE_POSITIONS[i], pos);
		pos[2] += 128.0;
		VectorCopy(pos, down);
		down[2] = mapMins[2];

		CG_Trace(&tr, pos, NULL, NULL, down, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);

		// Look around here for a different slope angle... Cull if found...
		for (scale = 1.00; scale > FOLIAGE_PLANT_SCALE[i] && scale >= cg_foliageMinFoliageScale.value; scale -= 0.05)
		{
			qboolean	FOUND = qfalse;

			if (FOLIAGE_CheckSlopesAround(tr.endpos, tr.plane.normal, scale))
			{
				FOLIAGE_PLANT_SCALE[i] = scale;

				NUM_RESCALED++;

				sprintf(last_node_added_string, "^5Plant point ^3%i ^5at ^7%f %f %f^5 rescaled.", i, FOLIAGE_POSITIONS[i][0], FOLIAGE_POSITIONS[i][1], FOLIAGE_POSITIONS[i][2]);

				FOUND = qtrue;
			}
			else
			{
				FOLIAGE_PLANT_SCALE[i] = 0;
			}

			if (FOUND) break;
		}
	}

	trap->S_Shutup(qfalse);

	aw_percent_complete = 0.0f;

	trap->Print("^1*** ^3%s^5: Successfully rescaled %i grass points...\n", "AUTO-FOLIAGE", FOLIAGE_NUM_POSITIONS);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

void FOLIAGE_FoliageReplant(int plantPercentage)
{
	int i = 0;
	int NUM_REPLACED = 0;

//#pragma omp parallel for schedule(dynamic)
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		FOLIAGE_PLANT_SELECTION[i] = 0;

		if (!MAP_HAS_TREES || FOLIAGE_TREE_SELECTION[i] <= 0)
		{
			if (irand(0, 100) <= plantPercentage)
			{// Replace...
				FOLIAGE_PLANT_SELECTION[i] = irand(1, MAX_PLANT_MODELS);
				NUM_REPLACED++;
			}
			else
			{
				FOLIAGE_PLANT_SELECTION[i] = 0;
				NUM_REPLACED++;
			}
		}
	}

	trap->Print("^1*** ^3%s^5: Successfully replaced %i trees...\n", "AUTO-FOLIAGE", NUM_REPLACED);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

qboolean FOLIAGE_MaterialIsWallSolid(vec3_t normal, int mateiral)
{
	if (mateiral != MATERIAL_SHORTGRASS
		&& mateiral != MATERIAL_LONGGRASS
		&& mateiral != MATERIAL_GREENLEAVES
		&& mateiral != MATERIAL_SAND
		&& mateiral != MATERIAL_GRAVEL
		&& mateiral != MATERIAL_SNOW
		&& mateiral != MATERIAL_MUD
		&& mateiral != MATERIAL_DIRT
		/*&& mateiral != MATERIAL_NONE*/)
	{// Looks like a tree or wall here.. Yay!
		if (!FOLIAGE_CheckSlope(normal))
		{// Also check it's slope, to make sure it's on a non-walkable angle...
			return qtrue;
		}
	}

	return qfalse;
}

#define TREE_SEARCH_DISTANCE 96.0//72.0//128.0

qboolean FOLIAGE_NearbyWall(vec3_t org)
{
	trace_t tr;
	vec3_t pos, end;

	VectorCopy(org, pos);
	pos[2] += 48.0;
	
	VectorCopy(pos, end);
	end[0] += TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	VectorCopy(pos, end);
	end[1] += TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	VectorCopy(pos, end);
	end[0] -= TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	VectorCopy(pos, end);
	end[1] -= TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	VectorCopy(pos, end);
	end[0] += TREE_SEARCH_DISTANCE;
	end[1] += TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	VectorCopy(pos, end);
	end[0] -= TREE_SEARCH_DISTANCE;
	end[1] += TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	VectorCopy(pos, end);
	end[0] += TREE_SEARCH_DISTANCE;
	end[1] -= TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	VectorCopy(pos, end);
	end[0] -= TREE_SEARCH_DISTANCE;
	end[1] -= TREE_SEARCH_DISTANCE;
	CG_Trace(&tr, pos, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID | CONTENTS_WATER);
	if (tr.fraction < 1.0)
	{
		if (FOLIAGE_MaterialIsWallSolid(tr.plane.normal, tr.materialType))
		{// Looks like a tree or wall here.. Yay!
			return qtrue;
		}
	}

	return qfalse;
}

qboolean FOLIAGE_TreeLocationNearby(vec3_t org)
{
	int i = 0;

	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{
		if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
		{
			if (Distance(FOLIAGE_POSITIONS[i], org) <= (TREE_SEARCH_DISTANCE * 1.5) + (FOLIAGE_TREE_RADIUS[FOLIAGE_TREE_SELECTION[i]-1] * FOLIAGE_TREE_SCALE[i] * TREE_SCALE_MULTIPLIER))
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

qboolean FOLIAGE_AnotherBigPlantNearby(vec3_t org, int upToPosition)
{
	int i = 0;

	for (i = 0; i < upToPosition; i++)
	{
		if (FOLIAGE_PLANT_SELECTION[i] > 0)
		{
			if (Distance(FOLIAGE_POSITIONS[i], org) <= TREE_SEARCH_DISTANCE)
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

void FOLIAGE_FoliageReplantSpecial(int plantPercentage)
{
	int i = 0;
	int NUM_REPLACED = 0;
	int NUM_OBJECT_PLANTS = 0;
	int NUM_PLANTS_TOTAL = 0;

	int previous_time = clock();
	aw_stage_start_time = clock();
	aw_percent_complete = 0;

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Rescaling foliage points. This could take a while...\n"));
	strcpy(task_string1, va("^5Rescaling foliage points. This could take a while..."));
	trap->UpdateScreen();

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Rescaling foliage points...\n"));
	strcpy(task_string2, va("^5Rescaling foliage points..."));
	trap->UpdateScreen();

	strcpy(task_string3, "");

	trap->UpdateScreen();

	trap->S_Shutup(qtrue);

	int numCompleted = 0;

//#pragma omp parallel for schedule(dynamic)
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		FOLIAGE_PLANT_SELECTION[i] = 0;

		numCompleted++;
		aw_percent_complete = (float)((float)numCompleted / (float)FOLIAGE_NUM_POSITIONS) * 100.0;

		if (clock() - previous_time > 500) // update display every 500ms...
		{
			previous_time = clock();
			trap->UpdateScreen();
		}

		if (!MAP_HAS_TREES || FOLIAGE_TREE_SELECTION[i] <= 0)
		{
			NUM_PLANTS_TOTAL++;

			if (FOLIAGE_TreeLocationNearby(FOLIAGE_POSITIONS[i]) || FOLIAGE_NearbyWall(FOLIAGE_POSITIONS[i]))
			{// 1 in 2 (or if no other nearby yet) are fern or tall plant...
				//if (irand(1,2) == 1 || !FOLIAGE_AnotherBigPlantNearby(FOLIAGE_POSITIONS[i], i-1))
				//{
				FOLIAGE_PLANT_SELECTION[i] = irand(MAX_PLANT_MODELS - 23, MAX_PLANT_MODELS);
				NUM_OBJECT_PLANTS++;
				//}
				//else
				//	FOLIAGE_PLANT_SELECTION[i] = irand(1, MAX_PLANT_MODELS - 1);

				sprintf(last_node_added_string, "^3%i ^5near object plants replaced. ^3%i ^5normal replaced. ^3%i ^5total plants.", NUM_OBJECT_PLANTS, NUM_REPLACED, NUM_PLANTS_TOTAL);
			}
			else
			{
				if (irand(0, 100) <= plantPercentage)
				{// Replace...
					FOLIAGE_PLANT_SELECTION[i] = irand(1, MAX_PLANT_MODELS - 22);
					NUM_REPLACED++;

					sprintf(last_node_added_string, "^3%i ^5near object plants replaced. ^3%i ^5normal replaced. ^3%i ^5total plants.", NUM_OBJECT_PLANTS, NUM_REPLACED, NUM_PLANTS_TOTAL);
				}
				else
				{
					FOLIAGE_PLANT_SELECTION[i] = 0;
					NUM_REPLACED++;

					sprintf(last_node_added_string, "^3%i ^5near object plants replaced. ^3%i ^5normal replaced. ^3%i ^5total plants.", NUM_OBJECT_PLANTS, NUM_REPLACED, NUM_PLANTS_TOTAL);
				}
			}
		}
	}

	trap->S_Shutup(qfalse);

	aw_percent_complete = 0.0f;

	trap->Print("^1*** ^3%s^5: Successfully replaced %i plants (%i near objects)...\n", "AUTO-FOLIAGE", NUM_REPLACED, NUM_OBJECT_PLANTS);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

void FOLIAGE_FoliageClearFxRunners(void)
{
	int i = 0;
	int NUM_REMOVED_OBJECTS = 0;
	int NUM_PLANTS_TOTAL = 0;

	int previous_time = clock();
	aw_stage_start_time = clock();
	aw_percent_complete = 0;

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Cleaning foliage points. This could take a while...\n"));
	strcpy(task_string1, va("^5Cleaning foliage points. This could take a while..."));
	trap->UpdateScreen();

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Cleaning foliage points...\n"));
	strcpy(task_string2, va("^5Cleaning foliage points..."));
	trap->UpdateScreen();

	strcpy(task_string3, "");

	trap->UpdateScreen();

	trap->S_Shutup(qtrue);

	int numCompleted = 0;

	//#pragma omp parallel for schedule(dynamic)
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		FOLIAGE_PLANT_SELECTION[i] = 0;

		numCompleted++;
		aw_percent_complete = (float)((float)numCompleted / (float)FOLIAGE_NUM_POSITIONS) * 100.0;

		if (clock() - previous_time > 50) // update display every 50ms...
		{
			previous_time = clock();
			trap->UpdateScreen();
		}

		NUM_PLANTS_TOTAL++;

		qboolean FxAtPoint = FOLIAGE_FxExistsNearPoint(FOLIAGE_POSITIONS[i]);

		if (FxAtPoint)
		{
			NUM_REMOVED_OBJECTS++;

			if (MAP_HAS_TREES) FOLIAGE_TREE_SELECTION[i] = 0;

			FOLIAGE_PLANT_SELECTION[i] = 0;
		}
		else if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
		{// Tree here... Replace...
			FOLIAGE_TREE_SELECTION[i] = irand(1, NUM_TREE_TYPES);
			FOLIAGE_PLANT_SELECTION[i] = 0;
		}
		else
		{
			FOLIAGE_PLANT_SELECTION[i] = irand(1, MAX_PLANT_MODELS);
			if (MAP_HAS_TREES) FOLIAGE_TREE_SELECTION[i] = 0;
		}

		sprintf(last_node_added_string, "^3%i ^5objects removed. ^3%i ^5total plants.", NUM_REMOVED_OBJECTS, NUM_PLANTS_TOTAL);
	}

	trap->S_Shutup(qfalse);

	aw_percent_complete = 0.0f;

	trap->Print("^1*** ^3%s^5: Successfully removed ^3%i ^5objects from a total of ^3%i ^5objects....\n", "AUTO-FOLIAGE", NUM_REMOVED_OBJECTS, NUM_PLANTS_TOTAL);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

void FOLIAGE_FoliageClearRoads(void)
{
	int i = 0;
	int NUM_REMOVED_OBJECTS = 0;
	int NUM_PLANTS_TOTAL = 0;

	int previous_time = clock();
	aw_stage_start_time = clock();
	aw_percent_complete = 0;

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Cleaning foliage points. This could take a while...\n"));
	strcpy(task_string1, va("^5Cleaning foliage points. This could take a while..."));
	trap->UpdateScreen();

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Cleaning foliage points...\n"));
	strcpy(task_string2, va("^5Cleaning foliage points..."));
	trap->UpdateScreen();

	strcpy(task_string3, "");

	trap->UpdateScreen();

	trap->S_Shutup(qtrue);

	int numCompleted = 0;

	//#pragma omp parallel for schedule(dynamic)
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		FOLIAGE_PLANT_SELECTION[i] = 0;

		numCompleted++;
		aw_percent_complete = (float)((float)numCompleted / (float)FOLIAGE_NUM_POSITIONS) * 100.0;

		if (clock() - previous_time > 50) // update display every 50ms...
		{
			previous_time = clock();
			trap->UpdateScreen();
		}

		NUM_PLANTS_TOTAL++;

		qboolean roadAtPoint = RoadExistsAtPoint(FOLIAGE_POSITIONS[i]);

		if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0 && !roadAtPoint)
		{// Tree here... Replace...
			FOLIAGE_TREE_SELECTION[i] = irand(1, NUM_TREE_TYPES);
			FOLIAGE_PLANT_SELECTION[i] = 0;
		}
		else if (!roadAtPoint)
		{
			FOLIAGE_PLANT_SELECTION[i] = irand(1, MAX_PLANT_MODELS);
			if (MAP_HAS_TREES) FOLIAGE_TREE_SELECTION[i] = 0;
		}
		else
		{
			NUM_REMOVED_OBJECTS++;

			if (MAP_HAS_TREES) FOLIAGE_TREE_SELECTION[i] = 0;

			FOLIAGE_PLANT_SELECTION[i] = 0;
		}

		sprintf(last_node_added_string, "^3%i ^5objects removed. ^3%i ^5total plants.", NUM_REMOVED_OBJECTS, NUM_PLANTS_TOTAL);
	}

	trap->S_Shutup(qfalse);

	aw_percent_complete = 0.0f;

	trap->Print("^1*** ^3%s^5: Successfully removed ^3%i ^5objects from a total of ^3%i ^5objects....\n", "AUTO-FOLIAGE", NUM_REMOVED_OBJECTS, NUM_PLANTS_TOTAL);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

void FOLIAGE_FoliageClearBuildings(void)
{
	int i = 0;
	int NUM_REMOVED_OBJECTS = 0;
	int NUM_PLANTS_TOTAL = 0;

	int previous_time = clock();
	aw_stage_start_time = clock();
	aw_percent_complete = 0;

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Cleaning foliage points. This could take a while...\n"));
	strcpy(task_string1, va("^5Cleaning foliage points. This could take a while..."));
	trap->UpdateScreen();

	trap->Print(va("^4*** ^3AUTO-FOLIAGE^4: ^5Cleaning foliage points...\n"));
	strcpy(task_string2, va("^5Cleaning foliage points..."));
	trap->UpdateScreen();

	strcpy(task_string3, "");

	trap->UpdateScreen();

	trap->S_Shutup(qtrue);

	int numCompleted = 0;

	//#pragma omp parallel for schedule(dynamic)
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		FOLIAGE_PLANT_SELECTION[i] = 0;

		numCompleted++;
		aw_percent_complete = (float)((float)numCompleted / (float)FOLIAGE_NUM_POSITIONS) * 100.0;

		if (clock() - previous_time > 50) // update display every 50ms...
		{
			previous_time = clock();
			trap->UpdateScreen();
		}

		NUM_PLANTS_TOTAL++;

		qboolean roofAtPoint = FOLIAGE_IsIndoorLocation(FOLIAGE_POSITIONS[i]);

		if (!roofAtPoint)
		{
			vec3_t o;
			VectorSet(o, FOLIAGE_POSITIONS[i][0] + 64.0, FOLIAGE_POSITIONS[i][1], FOLIAGE_POSITIONS[i][2] + 16.0);

			if (FOLIAGE_IsIndoorLocation(o))
			{
				roofAtPoint = qtrue;
			}

			VectorSet(o, FOLIAGE_POSITIONS[i][0] - 64.0, FOLIAGE_POSITIONS[i][1], FOLIAGE_POSITIONS[i][2] + 16.0);

			if (!roofAtPoint && FOLIAGE_IsIndoorLocation(o))
			{
				roofAtPoint = qtrue;
			}

			VectorSet(o, FOLIAGE_POSITIONS[i][0], FOLIAGE_POSITIONS[i][1] + 64.0, FOLIAGE_POSITIONS[i][2] + 16.0);

			if (!roofAtPoint && FOLIAGE_IsIndoorLocation(o))
			{
				roofAtPoint = qtrue;
			}

			VectorSet(o, FOLIAGE_POSITIONS[i][0], FOLIAGE_POSITIONS[i][1] - 64.0, FOLIAGE_POSITIONS[i][2] + 16.0);

			if (!roofAtPoint && FOLIAGE_IsIndoorLocation(o))
			{
				roofAtPoint = qtrue;
			}
		}

		if (roofAtPoint)
		{
			NUM_REMOVED_OBJECTS++;

			if (MAP_HAS_TREES) FOLIAGE_TREE_SELECTION[i] = 0;

			FOLIAGE_PLANT_SELECTION[i] = 0;
		}
		else if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
		{// Tree here... Replace...
			FOLIAGE_TREE_SELECTION[i] = irand(1, NUM_TREE_TYPES);
			FOLIAGE_PLANT_SELECTION[i] = 0;
		}
		else
		{
			FOLIAGE_PLANT_SELECTION[i] = irand(1, MAX_PLANT_MODELS);
			if (MAP_HAS_TREES) FOLIAGE_TREE_SELECTION[i] = 0;
		}

		sprintf(last_node_added_string, "^3%i ^5objects removed. ^3%i ^5total plants.", NUM_REMOVED_OBJECTS, NUM_PLANTS_TOTAL);
	}

	trap->S_Shutup(qfalse);

	aw_percent_complete = 0.0f;

	trap->Print("^1*** ^3%s^5: Successfully removed ^3%i ^5objects from a total of ^3%i ^5objects....\n", "AUTO-FOLIAGE", NUM_REMOVED_OBJECTS, NUM_PLANTS_TOTAL);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

void FOLIAGE_FoliageRetree(void)
{
	int i = 0;
	int NUM_REPLACED = 0;

	if (!MAP_HAS_TREES) return;

	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		if (FOLIAGE_TREE_SELECTION[i] > 0)
		{// Tree here... Replace...
			FOLIAGE_TREE_SELECTION[i] = irand(1, NUM_TREE_TYPES);
			NUM_REPLACED++;
		}
	}

	trap->Print("^1*** ^3%s^5: Successfully replaced %i trees...\n", "AUTO-FOLIAGE", NUM_REPLACED);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

void FOLIAGE_CopyAndRescale(char *filename, float mapScale, float objectScale)
{
	int i = 0;

	if (!FOLIAGE_LoadFoliagePositions(filename))
	{
		trap->Print("^1*** ^3%s^5: Error: File does not exist or can not be loaded...\n", "AUTO-FOLIAGE");
		return;
	}

#pragma omp parallel for ordered schedule(dynamic)
	for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
	{// Check current list...
		FOLIAGE_POSITIONS[i][2] += 18.0; // undo sinking into surface (so the object position is original surface position)...
		FOLIAGE_POSITIONS[i][0] *= mapScale;
		FOLIAGE_POSITIONS[i][1] *= mapScale;
		FOLIAGE_POSITIONS[i][2] *= mapScale;
		FOLIAGE_POSITIONS[i][2] -= (18.0 * objectScale); // redo sinking into surface...
		
		if (MAP_HAS_TREES && FOLIAGE_TREE_SELECTION[i] > 0)
		{
			FOLIAGE_TREE_SCALE[i] *= objectScale;
		}

		if (FOLIAGE_PLANT_SELECTION[i] > 0)
		{
			FOLIAGE_PLANT_SCALE[i] *= objectScale;
		}
	}

	trap->Print("^1*** ^3%s^5: Successfully copied %i foliages...\n", "AUTO-FOLIAGE", FOLIAGE_NUM_POSITIONS);

	// Save the generated info to a file for next time...
	FOLIAGE_SaveFoliagePositions();
}

void FOLIAGE_GenerateFoliage(void)
{
	char	str[MAX_TOKEN_CHARS];

	// UQ1: Check if we have an SSE CPU.. It can speed up our memory allocation by a lot!
	if (!CPU_CHECKED)
		UQ_Get_CPU_Info();

	GENFOLIAGE_ALLOW_MATERIAL = -1;

	if (trap->Cmd_Argc() < 2)
	{
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Creation sub-commands:\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Usage: ^3/genfoliage <method> <density> <plant_chance> <tree_chance> <num_clearings> <check_density>^5.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"standard\" ^5- Create new foliage map.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"concrete\" ^5- Create new foliage map. Allow concrete material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"sand\" ^5- Create new foliage map. Allow sand material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"gravel\" ^5- Create new foliage map. Allow gravel material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"rock\" ^5- Create new foliage map. Allow rock material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"snow\" ^5- Create new foliage map. Allow snow material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"multilevel\" ^5- Create new foliage map. For multi-level maps.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"multilevelconcrete\" ^5- Create new foliage map. For multi-level maps. Allow concrete material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"multilevelsand\" ^5- Create new foliage map. For multi-level maps. Allow sand material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"multilevelgravel\" ^5- Create new foliage map. For multi-level maps. Allow gravel material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"multilevelrock\" ^5- Create new foliage map. For multi-level maps. Allow rock material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"multilevelsnow\" ^5- Create new foliage map. For multi-level maps. Allow snow material.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"copy <original_mapname> <mapScale> <objectScale>\" ^5- Copy from another map's foliage file. Scales are optional.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: \n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Addition sub-commands:\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Usage: ^3/genfoliage <method> <density> <plant_chance> <tree_chance> <num_clearings> <check_density>^5.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"add\" ^5- Add more to current list of foliages. Allows <check_density> to check for another foliage before adding.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"addconcrete\" ^5- Add more to current list of foliages. Allows <check_density> to check for another foliage before adding.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"addsand\" ^5- Add more to current list of foliages. Allows <check_density> to check for another foliage before adding.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"addrock\" ^5- Add more to current list of foliages. Allows <check_density> to check for another foliage before adding.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"addgravel\" ^5- Add more to current list of foliages. Allows <check_density> to check for another foliage before adding.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"addsnow\" ^5- Add more to current list of foliages. Allows <check_density> to check for another foliage before adding.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: \n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Plant selection sub-commands:\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Usage: ^3/genfoliage <method> <percentage_to_keep>^5.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"replant\" ^5- Reselect all grasses/plants. Keeps percentage specified and removes the extras.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"replantspecial\" ^5- Keeps plants around objects (prefers the larger plants) and removes a percentage of plants in open areas.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Adjustment and cleaning sub-commands:\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^4=================================================================================================================================\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Usage: ^3/genfoliage <method>^5.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"rescale\" ^5- Check and fix scale of current grasses/plants.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"retree\" ^5- Reselect all tree types.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"clearfxrunners\" ^5- Remove all objects near fx_runner entities.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"clearbuildings\" ^5- Remove all objects inside buildings.\n");
		trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3\"clearroads\" ^5- Remove all objects on roads.\n");
		trap->UpdateScreen();
		return;
	}

	trap->Cmd_Argv(1, str, sizeof(str));

	if (StringContainsWord(str, "standard")
		|| StringContainsWord(str, "concrete")
		|| StringContainsWord(str, "sand")
		|| StringContainsWord(str, "rock")
		|| StringContainsWord(str, "gravel")
		|| StringContainsWord(str, "snow")
		|| StringContainsWord(str, "multilevel"))
	{
		qboolean multiLevel = qfalse;

		if (StringContainsWord(str, "concrete")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_CONCRETE;
		if (StringContainsWord(str, "sand")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_SAND;
		if (StringContainsWord(str, "rock")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_ROCK;
		if (StringContainsWord(str, "gravel")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_GRAVEL;
		if (StringContainsWord(str, "snow")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_SNOW;

		if (StringContainsWord(str, "multilevel"))
		{
			multiLevel = qtrue;
		}

		if (trap->Cmd_Argc() >= 2)
		{// Override normal density...
			int dist = 64;
			int plant_chance = 1;
			int tree_chance = 0;
			int num_clearings = 0;

			trap->Cmd_Argv(2, str, sizeof(str));
			dist = atoi(str);

			if (dist <= 4)
			{// Fallback and warning...
				dist = 64;
				trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid density set (%i). Using default (%i)...\n", atoi(str), 64);
			}

			trap->Cmd_Argv(3, str, sizeof(str));
			plant_chance = atoi(str);

			trap->Cmd_Argv(4, str, sizeof(str));
			tree_chance = atoi(str);

			trap->Cmd_Argv(5, str, sizeof(str));
			num_clearings = atoi(str);

			if (num_clearings > FOLIAGE_AREA_MAX / 8)
			{
				trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Error: ^5Invalid num_clearings set (%i). Maximum is (%i)...\n", atoi(str), FOLIAGE_AREA_MAX / 8);
				return;
			}

			FOLIAGE_FreeMemory();
			FOLIAGE_GenerateFoliage_Real((float)dist, plant_chance, tree_chance, num_clearings, 0.0, qfalse, multiLevel);
		}
		else
		{
			FOLIAGE_FreeMemory();
			FOLIAGE_GenerateFoliage_Real(64.0, 1, 0, 0, 0, qfalse, multiLevel);
		}
	}
	else if (!strcmp(str, "add")
		|| !strcmp(str, "addconcrete")
		|| !strcmp(str, "addsand")
		|| !strcmp(str, "addrock")
		|| !strcmp(str, "addgravel")
		|| !strcmp(str, "addsnow"))
	{
		if (!strcmp(str, "addconcrete")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_CONCRETE;
		if (!strcmp(str, "addsand")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_SAND;
		if (!strcmp(str, "addrock")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_ROCK;
		if (!strcmp(str, "addgravel")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_GRAVEL;
		if (!strcmp(str, "addsnow")) GENFOLIAGE_ALLOW_MATERIAL = MATERIAL_SNOW;

		if (trap->Cmd_Argc() >= 2)
		{// Override normal density...
			int dist = 64;
			int plant_chance = 1;
			int tree_chance = 0;
			int num_clearings = 0;
			int	check_density = dist;

			trap->Cmd_Argv(2, str, sizeof(str));
			dist = atoi(str);

			if (dist <= 4)
			{// Fallback and warning...
				dist = 256;
				trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid density set (%i). Using default (%i)...\n", atoi(str), 256);
			}

			check_density = dist;

			trap->Cmd_Argv(3, str, sizeof(str));
			plant_chance = atoi(str);

			trap->Cmd_Argv(4, str, sizeof(str));
			tree_chance = atoi(str);

			trap->Cmd_Argv(5, str, sizeof(str));
			num_clearings = atoi(str);

			trap->Cmd_Argv(6, str, sizeof(str));
			check_density = atoi(str);

			if (num_clearings > FOLIAGE_AREA_MAX / 8)
			{
				trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Error: ^5Invalid num_clearings set (%i). Maximum is (%i)...\n", atoi(str), FOLIAGE_AREA_MAX / 8);
				return;
			}

			FOLIAGE_GenerateFoliage_Real((float)dist, plant_chance, tree_chance, num_clearings, check_density, qtrue, qfalse);
		}
		else
		{
			FOLIAGE_GenerateFoliage_Real(64.0, 1, 0, 0, 0, qtrue, qfalse);
		}
	}
	else if (!strcmp(str, "rescale"))
	{
		FOLIAGE_FoliageRescale();
	}
	else if (!strcmp(str, "copy"))
	{
		float mapScale = 1.0;
		float objectScale = 1.0;
		char name[32] = { 0 };

		if (trap->Cmd_Argc() >= 2)
		{// Override normal scale...
			trap->Cmd_Argv(2, str, sizeof(str));

			if (str[0] == '0')
			{
				trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Error: ^5You need to supply the original map name...\n");
				return;
			}

			strcpy(name, str);

			trap->Cmd_Argv(3, str, sizeof(str));
			mapScale = atoi(str);

			trap->Cmd_Argv(4, str, sizeof(str));
			objectScale = atoi(str);

			if (mapScale <= 0) mapScale = 1;
			if (objectScale <= 0) objectScale = 1;
		}

		FOLIAGE_CopyAndRescale(name, mapScale, objectScale);
	}
	else if (!strcmp(str, "replant"))
	{
		if (trap->Cmd_Argc() >= 2)
		{// Override normal density...
			int plantPercentage = 20;

			trap->Cmd_Argv(2, str, sizeof(str));
			plantPercentage = atoi(str);

			if (plantPercentage <= 0)
			{// Fallback and warning...
				plantPercentage = 20;
				trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid plant percentage set (%i). Using default (%i)...\n", atoi(str), plantPercentage);
			}

			FOLIAGE_FoliageReplant(plantPercentage);
		}
		else
		{
			trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Usage:\n");
			trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3/genfoliage replant <plantPercent>^5. Use plantPercent 0 for default.\n");
		}
	}
	else if (!strcmp(str, "replantspecial"))
	{
		if (trap->Cmd_Argc() >= 2)
		{// Override normal density...
			int plantPercentage = 20;

			trap->Cmd_Argv(2, str, sizeof(str));
			plantPercentage = atoi(str);

			if (plantPercentage <= 0)
			{// Fallback and warning...
				plantPercentage = 20;
				trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid plant percentage set (%i). Using default (%i)...\n", atoi(str), plantPercentage);
			}

			FOLIAGE_FoliageReplantSpecial(plantPercentage);
		}
		else
		{
			trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Usage:\n");
			trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^3/genfoliage replant <plantPercent>^5. Use plantPercent 0 for default.\n");
		}
	}
	else if (!strcmp(str, "clearfxrunners"))
	{
		FOLIAGE_FoliageClearFxRunners();
	}
	else if (!strcmp(str, "clearroads"))
	{
		FOLIAGE_FoliageClearRoads();
	}
	else if (!strcmp(str, "clearbuildings"))
	{
		FOLIAGE_FoliageClearBuildings();
	}
	else if (!strcmp(str, "retree"))
	{
		FOLIAGE_FoliageRetree();
	}
}
