#include "../qcommon/q_shared.h"

extern qboolean			GENERIC_MATERIALS_PREFER_SHINY;
extern qboolean			DISABLE_DEPTH_PREPASS;
extern qboolean			LODMODEL_MAP;
extern qboolean			DISABLE_MERGED_GLOWS;
extern qboolean			DISABLE_LIFTS_AND_PORTALS_MERGE;
extern qboolean			ENABLE_REGEN_SMOOTH_NORMALS;
extern float			LEAF_ALPHA_MULTIPLIER;
extern int				ENABLE_INDOOR_OUTDOOR_SYSTEM;
extern int				MAP_MAX_VIS_RANGE;

extern qboolean			ENABLE_OCCLUSION_CULLING;
extern float			OCCLUSION_CULLING_TOLERANCE;
extern float			OCCLUSION_CULLING_TOLERANCE_FOLIAGE;
extern int				OCCLUSION_CULLING_MIN_DISTANCE;

extern qboolean			ENABLE_DISPLACEMENT_MAPPING;
extern float			DISPLACEMENT_MAPPING_STRENGTH;
extern qboolean			MAP_REFLECTION_ENABLED;
extern qboolean			ENABLE_CHRISTMAS_EFFECT;

extern float			STANDARD_SPLATMAP_SCALE;
extern float			ROCK_SPLATMAP_SCALE;

extern qboolean			TERRAIN_TESSELLATION_ENABLED;
extern float			TERRAIN_TESSELLATION_LEVEL;
extern float			TERRAIN_TESSELLATION_OFFSET;
extern float			TERRAIN_TESSELLATION_MIN_SIZE;

extern qboolean			DAY_NIGHT_CYCLE_ENABLED;
extern float			DAY_NIGHT_CYCLE_SPEED;
extern float			DAY_NIGHT_START_TIME;
extern float			SUN_PHONG_SCALE;
extern float			SUN_VOLUMETRIC_SCALE;
extern float			SUN_VOLUMETRIC_FALLOFF;
extern vec3_t			SUN_COLOR_MAIN;
extern vec3_t			SUN_COLOR_SECONDARY;
extern vec3_t			SUN_COLOR_TERTIARY;
extern vec3_t			SUN_COLOR_AMBIENT;

extern qboolean			PROCEDURAL_SKY_ENABLED;
extern vec3_t			PROCEDURAL_SKY_DAY_COLOR;
extern vec4_t			PROCEDURAL_SKY_NIGHT_COLOR;
extern vec4_t			PROCEDURAL_SKY_SUNSET_COLOR;
extern float			PROCEDURAL_SKY_NIGHT_HDR_MIN;
extern float			PROCEDURAL_SKY_NIGHT_HDR_MAX;
extern int				PROCEDURAL_SKY_STAR_DENSITY;
extern float			PROCEDURAL_SKY_NEBULA_FACTOR;
extern float			PROCEDURAL_SKY_NEBULA_SEED;
extern float			PROCEDURAL_SKY_PLANETARY_ROTATION;
extern qboolean			PROCEDURAL_BACKGROUND_HILLS_ENABLED;
extern float			PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS;
extern float			PROCEDURAL_BACKGROUND_HILLS_UPDOWN;
extern float			PROCEDURAL_BACKGROUND_HILLS_SEED;
extern vec3_t			PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR;
extern vec3_t			PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2;

extern qboolean			PROCEDURAL_CLOUDS_ENABLED;
extern qboolean			PROCEDURAL_CLOUDS_LAYER;
extern qboolean			PROCEDURAL_CLOUDS_DYNAMIC;
extern float			PROCEDURAL_CLOUDS_CLOUDSCALE;
extern float			PROCEDURAL_CLOUDS_SPEED;
extern float			PROCEDURAL_CLOUDS_DARK;
extern float			PROCEDURAL_CLOUDS_LIGHT;
extern float			PROCEDURAL_CLOUDS_CLOUDCOVER;
extern float			PROCEDURAL_CLOUDS_CLOUDALPHA;
extern float			PROCEDURAL_CLOUDS_SKYTINT;

extern qboolean			PROCEDURAL_MOSS_ENABLED;

extern qboolean			PROCEDURAL_SNOW_ENABLED;
extern qboolean			PROCEDURAL_SNOW_ROCK_ONLY;
extern float			PROCEDURAL_SNOW_LOWEST_ELEVATION;
extern float			PROCEDURAL_SNOW_HEIGHT_CURVE;
extern float			PROCEDURAL_SNOW_LUMINOSITY_CURVE;
extern float			PROCEDURAL_SNOW_BRIGHTNESS;

extern int				MAP_TONEMAP_METHOD;
extern qboolean			MAP_TONEMAP_AUTOEXPOSURE;
extern float			MAP_TONEMAP_SPHERICAL_STRENGTH;
extern int				LATE_LIGHTING_ENABLED;
extern qboolean			MAP_LIGHTMAP_DISABLED;
extern int				MAP_LIGHTMAP_ENHANCEMENT;
extern int				MAP_LIGHTING_METHOD;
extern qboolean			MAP_USE_PALETTE_ON_SKY;
extern float			MAP_LIGHTMAP_MULTIPLIER;
extern vec3_t			MAP_AMBIENT_CSB;
extern vec3_t			MAP_AMBIENT_COLOR;
extern float			MAP_GLOW_MULTIPLIER;
extern vec3_t			MAP_AMBIENT_CSB_NIGHT;
extern vec3_t			MAP_AMBIENT_COLOR_NIGHT;
extern float			MAP_GLOW_MULTIPLIER_NIGHT;
extern float			SKY_LIGHTING_SCALE;
extern qboolean			MAP_COLOR_SWITCH_RG;
extern qboolean			MAP_COLOR_SWITCH_RB;
extern qboolean			MAP_COLOR_SWITCH_GB;
extern float			MAP_EMISSIVE_COLOR_SCALE;
extern float			MAP_EMISSIVE_COLOR_SCALE_NIGHT;
extern float			MAP_EMISSIVE_RADIUS_SCALE;
extern float			MAP_EMISSIVE_RADIUS_SCALE_NIGHT;
extern float			MAP_HDR_MIN;
extern float			MAP_HDR_MAX;

extern qboolean			AURORA_ENABLED;
extern qboolean			AURORA_ENABLED_DAY;
extern vec3_t			AURORA_COLOR;

extern qboolean			AO_ENABLED;
extern qboolean			AO_BLUR;
extern qboolean			AO_DIRECTIONAL;
extern float			AO_MINBRIGHT;
extern float			AO_MULTBRIGHT;

extern qboolean			SHADOWS_ENABLED;
extern qboolean			SHADOWS_FULL_SOLID;
extern int				SHADOW_CASCADE1;
extern int				SHADOW_CASCADE2;
extern int				SHADOW_CASCADE_BIAS1;
extern int				SHADOW_CASCADE_BIAS2;
extern float			SHADOW_Z_ERROR_OFFSET_NEAR;
extern float			SHADOW_Z_ERROR_OFFSET_MID;
extern float			SHADOW_Z_ERROR_OFFSET_FAR;
extern float			SHADOW_MINBRIGHT;
extern float			SHADOW_MAXBRIGHT;
extern float			SHADOW_FORCE_UPDATE_ANGLE_CHANGE;
extern qboolean			SHADOW_SOFT;
extern float			SHADOW_SOFT_WIDTH;
extern float			SHADOW_SOFT_STEP;

extern qboolean			FOG_POST_ENABLED;
extern qboolean			FOG_LINEAR_ENABLE;
extern vec3_t			FOG_LINEAR_COLOR;
extern float			FOG_LINEAR_ALPHA;
extern float			FOG_LINEAR_RANGE_POW;
extern qboolean			FOG_WORLD_ENABLE;
extern vec3_t			FOG_WORLD_COLOR;
extern vec3_t			FOG_WORLD_COLOR_SUN;
extern float			FOG_WORLD_CLOUDINESS;
extern float			FOG_WORLD_WIND;
extern float			FOG_WORLD_ALPHA;
extern float			FOG_WORLD_FADE_ALTITUDE;
extern qboolean			FOG_LAYER_ENABLE;
extern qboolean			FOG_LAYER_INVERT;
extern float			FOG_LAYER_SUN_PENETRATION;
extern float			FOG_LAYER_ALPHA;
extern float			FOG_LAYER_CLOUDINESS;
extern float			FOG_LAYER_WIND;
extern float			FOG_LAYER_ALTITUDE_BOTTOM;
extern float			FOG_LAYER_ALTITUDE_TOP;
extern float			FOG_LAYER_ALTITUDE_FADE;
extern vec3_t			FOG_LAYER_COLOR;
extern vec4_t			FOG_LAYER_BBOX;

extern qboolean			WATER_ENABLED;
extern qboolean			WATER_USE_OCEAN;
extern qboolean			WATER_ALTERNATIVE_METHOD;
extern qboolean			WATER_FARPLANE_ENABLED;
extern float			WATER_REFLECTIVENESS;
extern float			WATER_WAVE_HEIGHT;
extern float			WATER_CLARITY;
extern float			WATER_UNDERWATER_CLARITY;
extern vec3_t			WATER_COLOR_SHALLOW;
extern vec3_t			WATER_COLOR_DEEP;
extern float			WATER_EXTINCTION1;
extern float			WATER_EXTINCTION2;
extern float			WATER_EXTINCTION3;

extern qboolean			GRASS_ENABLED;
extern qboolean			GRASS_UNDERWATER_ONLY;
extern qboolean			GRASS_RARE_PATCHES_ONLY;
extern int				GRASS_WIDTH_REPEATS;
extern int				GRASS_DENSITY;
extern float			GRASS_HEIGHT;
extern int				GRASS_DISTANCE;
extern float			GRASS_MAX_SLOPE;
extern float			GRASS_SURFACE_MINIMUM_SIZE;
extern float			GRASS_SURFACE_SIZE_DIVIDER;
extern float			GRASS_TYPE_UNIFORMALITY;
extern float			GRASS_TYPE_UNIFORMALITY_SCALER;
extern float			GRASS_DISTANCE_FROM_ROADS;
extern float			GRASS_SIZE_MULTIPLIER_COMMON;
extern float			GRASS_SIZE_MULTIPLIER_RARE;
extern float			GRASS_SIZE_MULTIPLIER_UNDERWATER;
extern float			GRASS_LOD_START_RANGE;

extern qboolean			FOLIAGE_ENABLED;
extern int				FOLIAGE_DENSITY;
extern float			FOLIAGE_HEIGHT;
extern int				FOLIAGE_DISTANCE;
extern float			FOLIAGE_MAX_SLOPE;
extern float			FOLIAGE_SURFACE_MINIMUM_SIZE;
extern float			FOLIAGE_SURFACE_SIZE_DIVIDER;
extern float			FOLIAGE_LOD_START_RANGE;
extern float			FOLIAGE_TYPE_UNIFORMALITY;
extern float			FOLIAGE_TYPE_UNIFORMALITY_SCALER;
extern float			FOLIAGE_DISTANCE_FROM_ROADS;

extern qboolean			VINES_ENABLED;
extern qboolean			VINES_UNDERWATER_ONLY;
extern int				VINES_WIDTH_REPEATS;
extern int				VINES_DENSITY;
extern float			VINES_HEIGHT;
extern int				VINES_DISTANCE;
extern float			VINES_MIN_SLOPE;
extern float			VINES_SURFACE_MINIMUM_SIZE;
extern float			VINES_SURFACE_SIZE_DIVIDER;
extern float			VINES_TYPE_UNIFORMALITY;
extern float			VINES_TYPE_UNIFORMALITY_SCALER;

extern int				MOON_COUNT;
extern qboolean			MOON_ENABLED[8];
extern float			MOON_SIZE[8];
extern float			MOON_BRIGHTNESS[8];
extern float			MOON_TEXTURE_SCALE[8];
extern float			MOON_ROTATION_OFFSET_X[8];
extern float			MOON_ROTATION_OFFSET_Y[8];
extern char				ROAD_TEXTURE[256];
extern qboolean			MATERIAL_SPECULAR_CHANGED;
extern float			MATERIAL_SPECULAR_STRENGTHS[MATERIAL_LAST];
extern float			MATERIAL_SPECULAR_REFLECTIVENESS[MATERIAL_LAST];

extern qboolean			JKA_WEATHER_ENABLED;
extern qboolean			WZ_WEATHER_ENABLED;
extern qboolean			WZ_WEATHER_SOUND_ONLY;

extern float			MAP_WATER_LEVEL;

extern char				CURRENT_CLIMATE_OPTION[256];
extern char				CURRENT_WEATHER_OPTION[256];

#define					MAX_FOLIAGE_ALLOWED_MATERIALS 64
extern int				FOLIAGE_ALLOWED_MATERIALS_NUM;
extern int				FOLIAGE_ALLOWED_MATERIALS[MAX_FOLIAGE_ALLOWED_MATERIALS];
