/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#ifndef TR_LOCAL_H
#define TR_LOCAL_H

// -----------------------------------------------------------------------------------------------------------------------------
//                                                Warzone Debugging Defines
// -----------------------------------------------------------------------------------------------------------------------------

#define __DEBUG_BINDS__

#ifdef __DEBUG_BINDS__
#define __DEBUG_FBO_BINDS__
#define __DEBUG_GLSL_BINDS__
#endif //__DEBUG_BINDS__

#define __PERFORMANCE_DEBUG__

#ifdef __PERFORMANCE_DEBUG__
//#define __PERFORMANCE_DEBUG_TEXT__
//#define __PERFORMANCE_DEBUG_STARTUP__
#endif //__PERFORMANCE_DEBUG__

// -----------------------------------------------------------------------------------------------------------------------------
//                                               Warzone Basic Renderer Defines
// -----------------------------------------------------------------------------------------------------------------------------
#define __CHEAP_VERTS__							// FIXME!!! - Remove or reduce attributes sizes to lower memory usage.
//#define __VBO_PACK_COLOR__					// Try to pack vert colors into a single float - BROKEN!
//#define __VBO_HALF_FLOAT_COLOR__				// Try to pack vert colors into half floats - BROKEN!
#define __HALF_FLOAT__							// Enable half float conversion code...
#define __DRAW_INDIRECT__						// Use indirect draw calls...
//#define __GLSL_OPTIMIZER__						// Enable GLSL optimization...
//#define __DEBUG_SHADER_LOAD__					// Enable extra GLSL shader load debugging info...
//#define __USE_GLSL_SHADER_CACHE__				// Enable GLSL shader binary caching...
//#define __FREE_WORLD_DATA__					// Free verts and indexes for everything with a VBO+IBO to save ram... Would need to disable saber marks to do this though...
//#define __VBO_LODMODELS__						// Experimental lodmodel > VBO crap... Broken...
//#define __LODMODEL_INSTANCING__				// Experimental lodmodel > instancing crap... Broken...
//#define __DLIGHT_BMODEL__						// Bmodel dlights. They are handled by deferred shader like everything else now...

//#define __TWO_SIDED_TERRAIN__					// Draw splat mapped terrain 2 sided, as a hack fix for inverted triangles exported by programs...

//#define __INVERSE_DEPTH_BUFFERS__				// Inverted depth buffer...

//#define __RENDER_HEIGHTMAP__

//#define __USE_QGL_FINISH__					// For testing...
#define __USE_QGL_FLUSH__						// Use this one...

#define __TINY_IMAGE_LOADER__					// Use TIL Image library when JKA fails to load an image (dds support, etc)

//#define __G2_LODS_ENABLED__						// No real speed boost with them, should we disable?
//#define __RENDERER_FOLIAGE__					// A port of the cgame foliage system in the renderer, doesn't work, only just started porting the basic code...
#define __SHADER_GENERATOR__					// Generates warzone compatible shaders on load... Tries to convert old JKA shaders to better WZ ones... Hacky but works...
#define __EXTRA_PRETTY__						// Makes things look better by not disabling some stuff on shaders...
#define __PSHADOWS__							// Player shadows...
#define __DAY_NIGHT__							// Day/Night Cycle system...
//#define __Q3_FLARES__							// Gonna just let the volumetrics handle flaring lights/glows....
//#define __INSTANCED_MODELS__					// experimenting with model instancing for foliage...
//#define __RENDERER_GROUND_FOLIAGE__			// in-progress port of cgame foliage system to renderer...
#define __JKA_WEATHER__							// Testing JKA weather reimplementation...
//#define __JKA_SURFACE_SPRITES__				// Testing JKA surface sprites reimplementation...
//#define __XYC_SURFACE_SPRITES__				// Testing port of Xycaleth's surface sprite implementation...
//#define __REALTIME_CUBEMAP__					// Render cubemap in realtime and use it... Slow as a wet week...
//#define __GLOW_LIGHT_PVS__					// Check if lights are in PVS. Probably not worth the FPS it costs...
//#define __USE_DETAIL_MAPS__					// Enabled detail map system... Disabling for now for more FPS...
//#define __GEOMETRY_SHADER_ALLOW_INVOCATIONS__ // Enable geometry shader invocations support. Slower because you cant set the invocations max in realtime...
//#define __EMISSIVE_CUBE_IBL__					// Experimental IBL using glowmap cubes...
//#define __MESH_OPTIMIZATION__					// Optimize all model's when loading. Disabled for now because it slows load time a bit.
#define __TEXTURECOLOR_SHADER_BINDLESS__		// Do the texturecolor shader bindless...

#define __ZFAR_CULLING_ON_SURFACES__			// Experimental zfar culling separation of depth prepass and render surfaces...
//#define __ZFAR_CULLING_ON_LEAFS__				// Do zfar culling on leaf bounds, not on per surface origins...

//#define __CRC_IMAGE_HASHING__					// Use image CRC hashing, to find and reuse already loaded identical images instead of loading more than one copy... Seems its not worth the extra time it takes to hash the images...
//#define __DEFERRED_IMAGE_LOADING__			// deferred loading of shader images... save vram and speed up map load - at the expense of some ingame stutter?!?!?
//#define __DEFERRED_MAP_IMAGE_LOADING__		// also load map images deferred...

//#define ___WARZONE_AWESOMIUM___				// Enables the old awesomium test code...

//#define __LIGHT_OCCLUSION__						// Check occlusion on lights...

#define __MERGE_GLOW_STAGES__					// Tries to merge glow stages into the diffuse stage of shaders to reduce draw calls.

#define __ALLOW_MAP_GLOWS_MERGE__				// Allow merging of (map glow) emissive lights...

#define __ENTITY_LIGHTING_DLIGHT_GLOW__			// Add some dlight glow to entity lighting...
#define __ENTITY_LIGHTING_MAP_EMISSIVE_LIGHT__			// Add some emissive glow to entity lighting...

//#define __DEPTH_PREPASS_STUFF__

#define __REGENERATE_BSP_NORMALS__				// Re-generate smooth normals for all of the map.

//#define __USE_VBO_AREAS__						// Break up map VBO's into areas.

#define __OCEAN__								// System to add water outside of map boundaries...

//#define __RENDERER_THREADING__				// Just testing...

//#define __EXPERIMENTAL_AUTO_BONE_WEIGHTING__
//#define __EXPERIMENTAL_ASSIMP_GLM_CONVERSIONS__
//#define __DEBUG_ASSIMP__						// Show debug info for loading models through assimp library...

//#define __EXPERIMETNAL_CHARACTER_EDITOR__		// Testing stuff...

//#define __HEIGHTMAP_TERRAIN_TEST__			// Testing stuff...

#define __INDOOR_SHADOWS__						// Testing stuff...

//#define __CALCULATE_LIGHTDIR_FROM_LIGHT_AVERAGES__

#define __CULL_BY_RANGE_AND_SIZE__				// Experimental culling of stuff at a range it wouldn't be big enough to see anyway...

#define __USE_REGIONS__

#define __USE_MAP_EMMISSIVE_BLOCK__				// Do map emissive lights as a SSBO block of it's own...


#define	__PROCEDURALS_IN_DEFERRED_SHADER__		// Merge procedural draws into deferred light shader...

#define __TERRAIN_TESSELATION__
#define __PSHADOW_TESSELLATION__				// Tessellate pshadow draws so that they work correctly on tessellated terrain...

#define __HUMANOIDS_BEND_GRASS__
#ifdef __HUMANOIDS_BEND_GRASS__
	#define MAX_GRASSBEND_HUMANOIDS 4
#endif //__HUMANOIDS_BEND_GRASS__

//#define __USE_WATER_MAPS__					// Testing, using water maps instead of procedural random generation...
//#define __WATER_SPEC_AO_IMAGES__				// Use specular and AO maps for water (TODO, maybe)...


#define __NIF_IMPORT_TEST__						// Loading of skyrim .nif models...
//#define __NIF_GLM_IMPORT_TEST__					// Testing loading of skyrim nif models as player models...

//#define __CONVEX_HULL_CALCULATION__			// FOR FUTURE REFERENCE: Compute convex hull for verts using qhull, this may be really useful for stuff in the future, 
												// could be called on any mesh not just NIFs (or even BSP data for lods or collisions)...


//
// Post Process Effects...
//
//#define __SSDO__								// Screen Space Directional Occlusion...

//
// Optimization stuff...
//
//#define __SORT_POLYS__							// Sorts polys by shader so when they draw, they get merged...
//#define __MERGE_DEPTHPASS_DRAWS__				// Merges non-alpha draws in depth prepass by using defaultshader for them...
#define __USE_DEPTHDRAWONLY__

#define __FX_SORTING__
#define __WATER_SORTING__
#define __ALPHA_SORTING__
#define __GRASS_SORTING__
#define __SPLATMAP_SORTING__
//#define __MERGED_SORTING__
//#define __DEPTHDRAW_SORTING__
#define __NUMSTAGES_SORTING__
#define __GLOW_SORTING__
#define __TESS_SORTING__
#define __MATERIAL_SORTING__
//#define __INDOOR_SORTING__

#define __INDOOR_OUTDOOR_CULLING__

//#define __REALTIME_SURFACE_SORTING__			// Meh, on big maps with lots of models, sorting is much slower then not sorting...
#ifdef __REALTIME_SURFACE_SORTING__
	#define __REALTIME_DISTANCE_SORTING__
	//#define __REALTIME_FX_SORTING__
	//#define __REALTIME_WATER_SORTING__
	//#define __REALTIME_ALPHA_SORTING__
	#define __REALTIME_SPLATMAP_SORTING__
	#define __REALTIME_SORTINDEX_SORTING__
	//#define __REALTIME_NUMSTAGES_SORTING__
	//#define __REALTIME_GLOW_SORTING__
	//#define __REALTIME_TESS_SORTING__
	//#define __REALTIME_MATERIAL_SORTING__
#endif /__REALTIME_SURFACE_SORTING__

//#define __REALTIME_SURFACE_SORTING2__

#ifdef _WIN32
	#define __BSP_USE_SHARED_MEMORY__
#endif //_WIN32

#define MAX_GLM_BONEREFS 20 //10 //16 //20 //80

// -----------------------------------------------------------------------------------------------------------------------------
//                                                Warzone Surface Merging Defines
// -----------------------------------------------------------------------------------------------------------------------------

// Merge the whole damn lot and use player's closest cubemap...
#define __PLAYER_BASED_CUBEMAPS__

// Allow all surfaces to merge with different cubemaps... with our range based checks as well, should be good enough... Not needed with __PLAYER_BASED_CUBEMAPS__
//#define __LAZY_CUBEMAP__

#define __GENERATED_SKY_CUBES__					// Generate sky cubemaps from sky render instead of using map's skybox textures...
//#define __REALTIME_GENERATED_SKY_CUBES__		// Generate sky cubes in realtime... BROKEN!

// -----------------------------------------------------------------------------------------------------------------------------
//                                               Warzone Cubemap and Lights Defines
// -----------------------------------------------------------------------------------------------------------------------------

#define DISTANCE_BETWEEN_CUBEMAPS	1024

#define	MAX_EMISSIVE_LIGHTS			65536
#define	MAX_DEFERRED_LIGHTS			128//64
#define MAX_DEFERRED_LIGHT_RANGE	8192.0

#define MAX_IMAGE_PATH				256

// -----------------------------------------------------------------------------------------------------------------------------
//                                                   Warzone Instancing Defines
// -----------------------------------------------------------------------------------------------------------------------------

#define MAX_INSTANCED_MODEL_TYPES		512
#define MAX_INSTANCED_MODEL_INSTANCES	384//4096

// -----------------------------------------------------------------------------------------------------------------------------

#define SKY_CUBE_SIZE 2048

#ifdef __DEBUG_BINDS__
extern int SCENE_FRAME_NUMBER;
#endif //__DEBUG_BINDS__

#ifdef __DEBUG_FBO_BINDS__
extern int FBO_BINDS_COUNT;
#endif //__DEBUG_FBO_BINDS__

#ifdef __DEBUG_GLSL_BINDS__
extern int GLSL_BINDS_COUNT;
#endif //__DEBUG_GLSL_BINDS__

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../rd-common/tr_public.h"
#include "../rd-common/tr_common.h"
#include "tr_extratypes.h"
#include "tr_extramath.h"
#include "tr_fbo.h"
#include "tr_postprocess.h"
#include "iqm.h"
#include "qgl.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include "../qcommon/inifile.h"
#include "tr_mapping.h"
#include "../game/bg_inventory.h"

#ifdef _WIN32
#include "win32\win_local.h"
#include "qcommon\sstring.h"
#include "../SharedMemory/sharedMemory.h"
#endif

#ifdef __VR__
#include "tr_openvr.h"
#endif //__VR__

#define MAX_GLSL_LENGTH 170000

extern qboolean ALLOW_NULL_FBO_BIND;

#define GL_INDEX_TYPE		GL_UNSIGNED_INT
typedef unsigned int glIndex_t;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

extern qboolean SKIP_CULL_FRAME;
extern qboolean SKIP_CULL_FRAME_DONE;

#ifdef ___WARZONE_AWESOMIUM___
extern int DrawAwesomium( char *URL, FBO_t *srcFbo );
#endif //___WARZONE_AWESOMIUM___

#ifdef __DAY_NIGHT__
extern qboolean DAY_NIGHT_CYCLE_ENABLED;
extern float RB_NightScale(void);
#endif //__DAY_NIGHT__

#define ForceCrash() { refdef_t *blah = NULL; blah->time = 1; }

extern qboolean SHADOWS_ENABLED;

#define MAX_EMISSIVE_LIGHTS 65536
extern int			MAP_EMISSIVE_LIGHT_COUNT;
extern vec3_t		MAP_EMISSIVE_LIGHT_LOCATIONS[MAX_EMISSIVE_LIGHTS];
extern vec4_t		MAP_EMISSIVE_LIGHT_COLORS[MAX_EMISSIVE_LIGHTS];
extern qboolean		MAP_EMISSIVE_LIGHT_COLORS_AVILABLE[MAX_EMISSIVE_LIGHTS];
extern float		MAP_EMISSIVE_LIGHT_RADIUSES[MAX_EMISSIVE_LIGHTS];
extern float		MAP_EMISSIVE_LIGHT_HEIGHTSCALES[MAX_EMISSIVE_LIGHTS];
extern float		MAP_EMISSIVE_LIGHT_CONEANGLE[MAX_EMISSIVE_LIGHTS];
extern vec3_t		MAP_EMISSIVE_LIGHT_CONEDIRECTION[MAX_EMISSIVE_LIGHTS];

#define				MAX_WORLD_GLOW_DLIGHT_RANGE MAX_DEFERRED_LIGHT_RANGE
#define				MAX_WORLD_GLOW_DLIGHTS (MAX_DEFERRED_LIGHTS - 1)
extern int			CLOSE_TOTAL;
extern int			CLOSE_LIST[MAX_WORLD_GLOW_DLIGHTS];
extern float		CLOSE_DIST[MAX_WORLD_GLOW_DLIGHTS];
extern vec3_t		CLOSE_POS[MAX_WORLD_GLOW_DLIGHTS];
extern float		CLOSE_RADIUS[MAX_WORLD_GLOW_DLIGHTS];
extern float		CLOSE_HEIGHTSCALES[MAX_WORLD_GLOW_DLIGHTS];

// 14 bits
// can't be increased without changing bit packing for drawsurfs
// see QSORT_SHADERNUM_SHIFT
#define SHADERNUM_BITS	14
#define MAX_SHADERS		(1<<SHADERNUM_BITS)

#define	MAX_FBOS      128//64

#define MAX_VISCOUNTS 5
#define MAX_VBOS      4096
#define MAX_IBOS      4096

#define MAX_CALC_PSHADOWS    8//16//64
#define MAX_DRAWN_PSHADOWS    8//16 // do not increase past 32, because bit flags are used on surfaces
#define PSHADOW_MAP_SIZE      256//512
#define CUBE_MAP_MIPS      7
#define CUBE_MAP_SIZE      (1 << CUBE_MAP_MIPS)

/*
=====================================================

Renderer-side Cvars
In Q3, these are defined in tr_common.h, which isn't very logical really
In JA, we define these in the tr_local.h, which is much more logical

=====================================================
*/

extern cvar_t	*r_drawSort;

extern cvar_t	*r_mipMapTextures;
extern cvar_t	*r_compressedTextures;

extern cvar_t	*r_cullNoDraws;
extern cvar_t	*r_cullLights;

extern cvar_t	*r_drawIndirect;

extern cvar_t	*r_superSampleMultiplier;

extern cvar_t	*r_instanceCloudReductionCulling;

extern cvar_t	*r_tessellation;
extern cvar_t	*r_terrainTessellation;
extern cvar_t	*r_terrainTessellationMax;

extern cvar_t	*r_cartoon;

extern cvar_t	*r_skydome;

extern cvar_t	*r_fur;

extern cvar_t	*r_foliageQuality;

extern cvar_t	*r_foliage;
extern cvar_t	*r_foliagePasses;
extern cvar_t	*r_foliageDistance;
extern cvar_t	*r_foliageDensity;
extern cvar_t	*r_foliageShadows;

extern cvar_t	*r_rotatex;
extern cvar_t	*r_rotatey;
extern cvar_t	*r_rotatez;

extern cvar_t	*r_materialDebug;
extern cvar_t	*r_genericShaderDebug;

extern cvar_t	*r_weather;
extern cvar_t	*r_surfaceSprites;
extern cvar_t	*r_surfaceWeather;

extern cvar_t	*r_windSpeed;
extern cvar_t	*r_windAngle;
extern cvar_t	*r_windGust;
extern cvar_t	*r_windDampFactor;
extern cvar_t	*r_windPointForce;
extern cvar_t	*r_windPointX;
extern cvar_t	*r_windPointY;

extern cvar_t	*r_flareSize;
extern cvar_t	*r_flareFade;
extern cvar_t	*r_flareCoeff;

extern cvar_t	*r_verbose;
extern cvar_t	*r_ignore;

extern cvar_t	*r_detailTextures;

extern cvar_t	*r_maxDeferredLights;

extern cvar_t	*r_znear;
extern cvar_t	*r_zproj;

extern cvar_t	*r_skipBackEnd;

extern cvar_t	*r_greyscale;

extern cvar_t	*r_ignorehwgamma;
extern cvar_t	*r_measureOverdraw;

extern cvar_t	*r_inGameVideo;
extern cvar_t	*r_fastsky;
extern cvar_t	*r_drawSun;
extern cvar_t	*r_dynamiclight;
extern cvar_t	*r_volumeLight;

extern cvar_t	*r_lodbias;
extern cvar_t	*r_lodscale;
extern cvar_t	*r_autolodscalevalue;

extern cvar_t	*r_norefresh;
extern cvar_t	*r_drawentities;
extern cvar_t	*r_drawworld;
extern cvar_t	*r_speeds;
extern cvar_t	*r_fullbright;
extern cvar_t	*r_novis;
extern cvar_t	*r_nocull;
extern cvar_t	*r_facePlaneCull;
extern cvar_t	*r_showcluster;
extern cvar_t	*r_nocurves;

extern cvar_t	*r_allowExtensions;

extern cvar_t	*r_ext_compressed_textures;
extern cvar_t	*r_ext_multitexture;
extern cvar_t	*r_ext_compiled_vertex_array;
extern cvar_t	*r_ext_texture_env_add;
extern cvar_t	*r_ext_texture_filter_anisotropic;

extern cvar_t  *r_occlusion;
extern cvar_t  *r_occlusionDebug;
extern cvar_t  *r_areaVisDebug;

extern cvar_t  *r_zFarOcclusion;

#ifdef __VR__
extern cvar_t  *vr_ovrdetected;
extern cvar_t  *vr_fovOffset;
extern cvar_t  *vr_lenseOffset;
extern cvar_t  *vr_viewOffsetX;
extern cvar_t  *vr_viewOffsetY;
extern cvar_t  *vr_interpupillaryDistance;
extern cvar_t  *vr_eyeToScreenDistance;
extern cvar_t  *vr_stereoEnabled;
extern cvar_t  *vr_stereoSeparation;
extern cvar_t  *vr_warpShader;
#endif //__VR__

extern cvar_t  *r_ext_draw_range_elements;
extern cvar_t  *r_ext_multi_draw_arrays;
extern cvar_t  *r_ext_texture_float;
extern cvar_t  *r_arb_half_float_pixel;
extern cvar_t  *r_ext_framebuffer_multisample;
extern cvar_t  *r_arb_seamless_cube_map;

extern cvar_t  *r_lazyFrustum;
extern cvar_t  *r_cacheVisibleSurfaces;

extern cvar_t  *r_mergeMultidraws;
extern cvar_t  *r_mergeLeafSurfaces;

extern cvar_t  *r_hdr;
extern cvar_t  *r_postProcess;

extern cvar_t  *r_toneMap;
extern cvar_t  *r_forceToneMap;
extern cvar_t  *r_forceToneMapMin;
extern cvar_t  *r_forceToneMapAvg;
extern cvar_t  *r_forceToneMapMax;

extern cvar_t  *r_forceAutoExposure;
extern cvar_t  *r_forceAutoExposureMin;
extern cvar_t  *r_forceAutoExposureMax;

extern cvar_t  *r_srgb;

extern cvar_t  *r_depthPrepass;

//extern cvar_t  *r_sss;

#ifdef __SSDO__
extern cvar_t  *r_ssdo;
extern cvar_t  *r_ssdoBaseRadius;
extern cvar_t  *r_ssdoMaxOcclusionDist;
#endif //__SSDO__

extern cvar_t  *r_normalMapping;
extern cvar_t  *r_normalMappingReal;
extern cvar_t  *r_normalMapQuality;
extern cvar_t  *r_specularMapping;
extern cvar_t  *r_deluxeMapping;
extern cvar_t  *r_normalAmbient;
extern cvar_t  *r_mergeLightmaps;
extern cvar_t  *r_dlightMode;
extern cvar_t  *r_pshadowDist;
extern cvar_t  *r_imageUpsample;
extern cvar_t  *r_imageUpsampleMaxSize;
extern cvar_t  *r_imageDownsampleMaxSize;
extern cvar_t  *r_imageUpsampleType;
extern cvar_t  *r_genNormalMaps;
extern cvar_t  *r_dlightShadows;
extern cvar_t  *r_forceSun;
extern cvar_t  *r_forceSunMapLightScale;
extern cvar_t  *r_forceSunLightScale;
extern cvar_t  *r_forceSunAmbientScale;
extern cvar_t  *r_proceduralSun;
extern cvar_t  *r_proceduralSunScale;
extern cvar_t  *r_glowStrength;
extern cvar_t  *r_glowVibrancy;
extern cvar_t  *r_glowMultiplierR;
extern cvar_t  *r_glowMultiplierG;
extern cvar_t  *r_glowMultiplierB;
extern cvar_t  *r_glowMultiplierA;
extern cvar_t  *r_sunlightMode;
extern cvar_t  *r_sunlightSpecular;
extern cvar_t  *r_drawSunRays;
extern cvar_t  *r_fastShadows;
extern cvar_t  *r_shadowSamples;
extern cvar_t  *r_shadowContrast;
extern cvar_t  *r_shadowFilter;
extern cvar_t  *r_shadowMapSize;
extern cvar_t  *r_shadowCascadeZNear;
extern cvar_t  *r_shadowCascadeZFar;
extern cvar_t  *r_shadowCascadeZBias;
extern cvar_t  *r_ignoreDstAlpha;

extern cvar_t	*r_ignoreGLErrors;
extern cvar_t	*r_logFile;

extern cvar_t	*r_stencilbits;
extern cvar_t	*r_depthbits;
extern cvar_t	*r_colorbits;
extern cvar_t	*r_texturebits;
extern cvar_t	*r_ext_multisample;

extern cvar_t	*r_instancing;

extern cvar_t	*r_drawBuffer;
extern cvar_t	*r_lightmap;
extern cvar_t	*r_vertexLight;
extern cvar_t	*r_uiFullScreen;
extern cvar_t	*r_shadows;
extern cvar_t	*r_flares;
extern cvar_t	*r_mode;
extern cvar_t	*r_nobind;
extern cvar_t	*r_singleShader;
extern cvar_t	*r_roundImagesDown;
extern cvar_t	*r_colorMipLevels;
extern cvar_t	*r_picmip;
extern cvar_t	*r_showtris;
extern cvar_t	*r_showsky;
extern cvar_t	*r_shownormals;
extern cvar_t	*r_showdepth;
extern cvar_t	*r_showsplat;
extern cvar_t	*r_finish;
extern cvar_t	*r_clear;
extern cvar_t	*r_swapInterval;
extern cvar_t	*r_markcount;
extern cvar_t	*r_textureMode;
extern cvar_t	*r_offsetFactor;
extern cvar_t	*r_offsetUnits;
extern cvar_t	*r_gamma;
extern cvar_t	*r_intensity;
extern cvar_t	*r_lockpvs;
extern cvar_t	*r_noportals;
extern cvar_t	*r_portalOnly;

extern cvar_t	*r_subdivisions;
extern cvar_t	*r_lodCurveError;

extern cvar_t	*r_fullscreen;
extern cvar_t  *r_noborder;

extern cvar_t	*r_customwidth;
extern cvar_t	*r_customheight;
extern cvar_t	*r_customPixelAspect;

extern cvar_t	*r_overBrightBits;
extern cvar_t	*r_mapOverBrightBits;

extern cvar_t	*r_debugSurface;
extern cvar_t	*r_simpleMipMaps;

extern cvar_t	*r_showImages;

extern cvar_t	*r_ambientScale;
extern cvar_t	*r_directedScale;
extern cvar_t	*r_debugLight;
extern cvar_t	*r_debugSort;
extern cvar_t	*r_printShaders;
extern cvar_t	*r_saveFontData;

extern cvar_t	*r_marksOnTriangleMeshes;

extern cvar_t	*r_aviMotionJpegQuality;
extern cvar_t	*r_screenshotJpegQuality;

extern cvar_t	*r_maxpolys;
extern int		max_polys;
extern cvar_t	*r_maxpolyverts;
extern int		max_polyverts;


//
// UQ1: Added...
//
extern cvar_t	*r_perf;
extern cvar_t	*r_glslOptimize;
extern cvar_t	*r_useLowP;
extern cvar_t	*r_lowQualityMode;
extern cvar_t	*r_lowVram;
extern cvar_t	*r_useStaticVBO;
extern cvar_t	*r_volumeLightHQ;
extern cvar_t	*r_debugBinds;
extern cvar_t	*r_debugShaderStages;
extern cvar_t	*r_debugImageCrcHashing;
extern cvar_t	*r_debugMapInfo;
extern cvar_t	*r_debugGlowMerge;
extern cvar_t	*r_debugSplatMaps;
extern cvar_t	*r_debugGLSLOptimizer;
extern cvar_t	*r_splatMapping;
extern cvar_t	*r_parallaxScale;
extern cvar_t	*r_blinnPhong;
extern cvar_t	*r_ao;
extern cvar_t	*r_env;
extern cvar_t	*r_debugEmissiveLights;
extern cvar_t	*r_debugDrawEmissiveLights;
extern cvar_t	*r_debugEmissiveRadiusScale;
extern cvar_t	*r_debugEmissiveColorScale;
extern cvar_t	*r_skynum;
extern cvar_t	*r_volumeLightStrength;
extern cvar_t	*r_fog;
extern cvar_t	*r_multithread;
#ifdef __EMISSIVE_CUBE_IBL__
extern cvar_t	*r_emissiveCubes;
#endif //__EMISSIVE_CUBE_IBL__
extern cvar_t	*r_testvar;
extern cvar_t	*r_steepParallaxEyeX;
extern cvar_t	*r_steepParallaxEyeY;
extern cvar_t	*r_steepParallaxEyeZ;
extern cvar_t	*r_bloom;
extern cvar_t	*r_bloomPasses;
extern cvar_t	*r_bloomDarkenPower;
extern cvar_t	*r_bloomScale;
extern cvar_t	*r_lensflare;
extern cvar_t	*r_anamorphic;
extern cvar_t	*r_anamorphicStrength;
extern cvar_t	*r_bloomRaysSamples;
extern cvar_t	*r_bloomRaysDecay;
extern cvar_t	*r_bloomRaysWeight;
extern cvar_t	*r_bloomRaysDensity;
extern cvar_t	*r_bloomRaysStrength;
extern cvar_t	*r_darkexpand;
extern cvar_t	*r_truehdr;
extern cvar_t  *r_magicdetail;
extern cvar_t  *r_magicdetailStrength;
extern cvar_t  *r_magicdetailMix;
extern cvar_t  *r_dof;
extern cvar_t  *r_debugMapAmbientR;
extern cvar_t  *r_debugMapAmbientG;
extern cvar_t  *r_debugMapAmbientB;
extern cvar_t  *gui_useMenu;
extern cvar_t  *gui_windowTransparancy;
extern cvar_t  *gui_tooltipSize;
extern cvar_t  *gui_tooltipCentered;
extern cvar_t  *r_testvalue0;
extern cvar_t  *r_testvalue1;
extern cvar_t  *r_testvalue2;
extern cvar_t  *r_testvalue3;
extern cvar_t	*r_bindlessTextures;
extern cvar_t  *r_esharpening;
//extern cvar_t  *r_esharpening2;
extern cvar_t  *r_fxaa;
extern cvar_t  *r_fxaaScanMod;
extern cvar_t  *r_txaa;
extern cvar_t  *r_cloudQuality;
extern cvar_t  *r_cloudshadows;
extern cvar_t  *r_underwater;
extern cvar_t  *r_multipost;
extern cvar_t  *r_screenBlurSlow;
extern cvar_t  *r_screenBlurFast;
//extern cvar_t  *r_hbao;
extern cvar_t  *r_fastLighting;
extern cvar_t  *r_deferredLighting;
extern cvar_t  *r_ssdm;
//extern cvar_t  *r_ssr;
//extern cvar_t  *r_ssrStrength;
//extern cvar_t  *r_sse;
//extern cvar_t  *r_sseStrength;
extern cvar_t  *r_steepParallax;
extern cvar_t  *r_trueAnaglyph;
extern cvar_t  *r_trueAnaglyphSeparation;
extern cvar_t  *r_trueAnaglyphRed;
extern cvar_t  *r_trueAnaglyphGreen;
extern cvar_t  *r_trueAnaglyphBlue;
extern cvar_t  *r_trueAnaglyphPower;
extern cvar_t  *r_trueAnaglyphMinDistance;
extern cvar_t  *r_trueAnaglyphMaxDistance;
extern cvar_t  *r_trueAnaglyphParallax;
extern cvar_t  *r_distanceBlur;
extern cvar_t  *r_fogPost;
extern cvar_t  *r_dayNightCycleSpeed;
extern cvar_t  *r_testshader;
extern cvar_t  *r_testshaderValue1;
extern cvar_t  *r_testshaderValue2;
extern cvar_t  *r_testshaderValue3;
extern cvar_t  *r_testshaderValue4;
extern cvar_t  *r_testshaderValue5;
extern cvar_t  *r_testshaderValue6;
extern cvar_t  *r_testshaderValue7;
extern cvar_t  *r_testshaderValue8;
extern cvar_t  *r_testshaderValue9;

//
// UQ1: End Added...
//



/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
extern cvar_t	*r_noPrecacheGLA;
#endif

extern cvar_t	*r_noServerGhoul2;
extern cvar_t	*r_Ghoul2AnimSmooth;
extern cvar_t	*r_Ghoul2UnSqashAfterSmooth;
//extern cvar_t	*r_Ghoul2UnSqash;
//extern cvar_t	*r_Ghoul2TimeBase=0; from single player
//extern cvar_t	*r_Ghoul2NoLerp;
//extern cvar_t	*r_Ghoul2NoBlend;
//extern cvar_t	*r_Ghoul2BlendMultiplier=0;

extern cvar_t	*broadsword;
extern cvar_t	*broadsword_kickbones;
extern cvar_t	*broadsword_kickorigin;
extern cvar_t	*broadsword_playflop;
extern cvar_t	*broadsword_dontstopanim;
extern cvar_t	*broadsword_waitforshot;
extern cvar_t	*broadsword_smallbbox;
extern cvar_t	*broadsword_extra1;
extern cvar_t	*broadsword_extra2;

extern cvar_t	*broadsword_effcorr;
extern cvar_t	*broadsword_ragtobase;
extern cvar_t	*broadsword_dircap;

/*
Ghoul2 Insert End
*/

/*
End Cvars
*/

typedef enum
{
	IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
	IMGTYPE_NORMAL,
	IMGTYPE_SPECULAR,
	IMGTYPE_NORMALHEIGHT,
	IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
	IMGTYPE_OVERLAY,
	IMGTYPE_STEEPMAP,
	IMGTYPE_WATER_EDGE_MAP,
	IMGTYPE_SPLATCONTROLMAP,
	IMGTYPE_SPLATMAP1,
	IMGTYPE_SPLATMAP2,
	IMGTYPE_SPLATMAP3,
	IMGTYPE_ROADSCONTROLMAP,
	IMGTYPE_ROADMAP,
	IMGTYPE_DETAILMAP,
	IMGTYPE_ROOFMAP,
	IMGTYPE_SHADOW,
} imgType_t;

typedef enum
{
	IMGFLAG_NONE           = 0x0000,
	IMGFLAG_MIPMAP         = 0x0001,
	IMGFLAG_PICMIP         = 0x0002,
	IMGFLAG_CUBEMAP        = 0x0004,
	IMGFLAG_NO_COMPRESSION = 0x0010,
	IMGFLAG_NOLIGHTSCALE   = 0x0020,
	IMGFLAG_CLAMPTOEDGE    = 0x0040,
	IMGFLAG_SRGB           = 0x0080,
	IMGFLAG_GENNORMALMAP   = 0x0100,
	IMGFLAG_MUTABLE        = 0x0200,
	IMGFLAG_GLOW           = 0x0400,
	IMGFLAG_3D_VOLUMETRIC  = 0x0800,
	IMGFLAG_FBM            = 0x1000,
} imgFlags_t;

typedef enum
{
	ANIMMAP_NORMAL,
	ANIMMAP_CLAMP,
	ANIMMAP_ONESHOT
} animMapType_t;

typedef struct image_s {
	char		imgName[MAX_IMAGE_PATH];		// game path, including extension
	int			width, height;				// source image
	int			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint		texnum;						// gl texture binding
	GLuint64	bindlessHandle;				// bindless texture handle

	int			frameUsed;			// for texture usage in frame statistics

	int			internalFormat;
	int			TMU;				// only needed for voodoo2

	imgType_t   type;
	int			flags;

	vec4_t		lightColor;
	vec4_t		averageColor;

	bool		hasAlpha;

	bool		generatedNormalMap;

	uint32_t	crcHash;

#ifdef __DEFERRED_IMAGE_LOADING__
	bool		deferredLoad = qfalse;
	imgType_t	deferredLoadType;
	int			deferredLoadFlags;
#endif //__DEFERRED_IMAGE_LOADING__

	int			current_filter_min;
	int			current_filter_max;
	float		current_filter_anisotropic;

	struct image_s*	next;
} image_t;

typedef struct bindlessTexturesBlock_s
{
	GLuint64 					u_DiffuseMap;
	GLuint64 					u_LightMap;
	GLuint64 					u_NormalMap;
	GLuint64 					u_DeluxeMap;
	GLuint64 					u_SpecularMap;
	GLuint64 					u_PositionMap;
	GLuint64 					u_WaterPositionMap;
	GLuint64 					u_WaterHeightMap;
	GLuint64 					u_HeightMap;
	GLuint64 					u_GlowMap;
	GLuint64 					u_EnvironmentMap;
	GLuint64 					u_TextureMap;
	GLuint64 					u_LevelsMap;
	GLuint64 					u_CubeMap;
	GLuint64 					u_SkyCubeMap;
	GLuint64 					u_SkyCubeMapNight;
	GLuint64 					u_EmissiveCubeMap;
	GLuint64 					u_OverlayMap;
	GLuint64 					u_SteepMap;
	GLuint64 					u_SteepMap1;
	GLuint64 					u_SteepMap2;
	GLuint64 					u_SteepMap3;
	GLuint64 					u_WaterEdgeMap;
	GLuint64 					u_SplatControlMap;
	GLuint64 					u_SplatMap1;
	GLuint64 					u_SplatMap2;
	GLuint64 					u_SplatMap3;
	GLuint64 					u_RoadsControlMap;
	GLuint64 					u_RoadMap;
	GLuint64 					u_DetailMap;
	GLuint64 					u_ScreenImageMap;
	GLuint64 					u_ScreenDepthMap;
	GLuint64 					u_ShadowMap;
	GLuint64 					u_ShadowMap2;
	GLuint64 					u_ShadowMap3;
	GLuint64 					u_ShadowMap4;
	GLuint64 					u_ShadowMap5;
	GLuint64					u_VolumeMap;
	GLuint64 					u_MoonMaps[4];
} bindlessTexturesBlock_t;

struct Lights_t
{
	vec4_t										u_lightPositions2;
	vec4_t										u_lightColors;
	vec4_t										u_coneDirection;
};

struct LightBlock_t
{
	Lights_t lights[MAX_DEFERRED_LIGHTS];
};

#ifdef __USE_MAP_EMMISSIVE_BLOCK__
#define MAX_CONCURRENT_EMISSIVE_DRAW_LIGHTS 96//128//64

struct EmissiveLightBlock_t
{
	Lights_t lights[MAX_CONCURRENT_EMISSIVE_DRAW_LIGHTS/*MAX_EMISSIVE_LIGHTS*/];
};
#endif //__USE_MAP_EMMISSIVE_BLOCK__

typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;
	float	coneAngle;
	vec3_t	coneDirection;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost tho when the lightmap is dark

	qboolean isGlowBased;
	float heightScale;
} dlight_t;

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct trRefEntity_s {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
	qboolean	mirrored;		// mirrored matrix, needs reversed culling
	vec3_t		lightDir;		// normalized direction towards light, in world space
	vec3_t      modelLightDir;  // normalized direction towards light, in model space
	vec3_t		ambientLight;	// color normalized to 0-255
	int			ambientLightInt;	// 32 bit rgba packed
	vec3_t		directedLight;
} trRefEntity_t;


typedef struct {
	vec3_t		origin;			// in world coordinates
	vec3_t		axis[3];		// orientation in world
	vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
	float		modelViewMatrix[16];
	float		modelMatrix[16];
} orientationr_t;

typedef enum
{
	VBO_USAGE_STATIC,
	VBO_USAGE_DYNAMIC,
	VBO_USAGE_STREAM
} vboUsage_t;

struct VAO_t
{
	GLuint vboID;
	GLuint vaoID;
	qboolean vboIndependent = qfalse;	// marks if this VBO should be deleted at the same time as the VAO (eg: was set up by the VAO code).
};

typedef struct VBO_s
{
	uint32_t        vertexesVBO;
	int             vertexesSize;	// amount of memory data allocated for all vertices in bytes
	int				vboUsage;
	uint32_t        ofs_xyz;
	uint32_t        ofs_normal;
	uint32_t        ofs_st;
	uint32_t        ofs_vertexcolor;
	uint32_t		ofs_boneweights;
	uint32_t		ofs_boneindexes;
	uint32_t		ofs_instancesPosition;
	uint32_t		ofs_instancesMVP;
	uint32_t		ofs_instancesTC;
	
	uint32_t        stride_xyz;
	uint32_t        stride_normal;
	uint32_t        stride_st;
	uint32_t        stride_vertexcolor;
	uint32_t		stride_boneweights;
	uint32_t		stride_boneindexes;
	uint32_t		stride_instancesMVP;
	uint32_t		stride_instancesPosition;
	uint32_t		stride_instancesTC;

	uint32_t        size_xyz;
	uint32_t        size_normal;

	qboolean		occluded;
	int				nextOcclusionCheckTime;
	qboolean		lastOcclusionCheckResult;
} VBO_t;

typedef struct IBO_s
{
	uint32_t        indexesVBO;
	int             indexesSize;	// amount of memory data allocated for all triangles in bytes
//  uint32_t        ofsIndexes;
	int				iboUsage;
} IBO_t;

//===============================================================================

typedef enum {
	SS_BAD,
	SS_PORTAL,			// mirrors, portals, viewscreens
	SS_ENVIRONMENT,		// sky box
	SS_OPAQUE,			// opaque

	SS_DECAL,			// scorch marks, etc.
	SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
						// in addition to alpha test
	SS_BANNER,

	SS_INSIDE,			// inside body parts (i.e. heart)
	SS_MID_INSIDE,
	SS_MIDDLE,
	SS_MID_OUTSIDE,
	SS_OUTSIDE,			// outside body parts (i.e. ribs)

	SS_FOG,

	SS_UNDERWATER,		// for items that should be drawn in front of the water plane

	SS_BLEND0,			// regular transparency and filters
	SS_BLEND1,			// generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST			// blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE,
	GF_RAND

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
} deform_t;

// deformVertexes types that can be handled by the GPU
typedef enum
{
	// do not edit: same as genFunc_t

	DGEN_NONE,
	DGEN_WAVE_SIN,
	DGEN_WAVE_SQUARE,
	DGEN_WAVE_TRIANGLE,
	DGEN_WAVE_SAWTOOTH,
	DGEN_WAVE_INVERSE_SAWTOOTH,
	DGEN_WAVE_NOISE,
	DGEN_PROJECTION_SHADOW,

	// do not edit until this line

	DGEN_BULGE,
	DGEN_MOVE
} deformGen_t;

typedef enum {
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST
} alphaGen_t;

typedef enum {
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_EXACT_VERTEX_LIT,	// like CGEN_EXACT_VERTEX but takes a light direction from the lightgrid
	CGEN_VERTEX_LIT,		// like CGEN_VERTEX but takes a light direction from the lightgrid
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_LIGHTING_DIFFUSE_ENTITY, // diffuse lighting * entity
	CGEN_LIGHTING_WARZONE,
	CGEN_FOG,				// standard fog
	CGEN_CONST,				// fixed color
	CGEN_LIGHTMAPSTYLE,		// lightmap style
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_LIGHTMAP1,
	TCGEN_LIGHTMAP2,
	TCGEN_LIGHTMAP3,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

typedef struct {
	genFunc_t	func;

	float base;
	float amplitude;
	float phase;
	float frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
} texMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct {
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;
} deformStage_t;


typedef struct {
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;

} texModInfo_t;

#define	MAX_IMAGE_ANIMATIONS	(32)

typedef struct {
	image_t			*image[MAX_IMAGE_ANIMATIONS];
	int				numImageAnimations;
	float			imageAnimationSpeed;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	int				numTexMods;
	texModInfo_t	*texMods;

	int				videoMapHandle;
	qboolean		isLightmap;
	qboolean		oneShotAnimMap;
	qboolean		isVideoMap;
} textureBundle_t;

enum
{
	TB_DIFFUSEMAP		= 0,
	TB_LIGHTMAP			= 1,
	TB_LEVELSMAP		= 1,
	TB_DIFFUSEMAP2		= 1,
	TB_NORMALMAP		= 2,
	TB_DELUXEMAP		= 3,
	TB_SHADOWMAP		= 3,
	TB_SPECULARMAP		= 4,
	TB_POSITIONMAP		= 5,
	TB_WATERPOSITIONMAP	= 6,
	TB_WATERHEIGHTMAP	= 7,
	TB_HEIGHTMAP		= 8,
	TB_GLOWMAP			= 9,
	TB_CUBEMAP			= 10,
	TB_SKYCUBEMAP		= 11,
	TB_SKYCUBEMAPNIGHT	= 12,
	TB_EMISSIVECUBE		= 13,
	TB_OVERLAYMAP		= 14,
	TB_WATER_EDGE_MAP	= 15,
	TB_SPLATCONTROLMAP	= 16,
	TB_SPLATMAP1		= 17,
	TB_SPLATMAP2		= 18,
	TB_SPLATMAP3		= 19,
	TB_ROADSCONTROLMAP	= 20,
	TB_ROADMAP			= 21,
	TB_DETAILMAP		= 22,
	TB_ROOFMAP			= 23,
	TB_STEEPMAP			= 24,				// Note: Shared with moonmap1 sampler uniform. Shaders cannot use both!
	TB_MOONMAP1			= TB_STEEPMAP,		// Note: Shared with steepmap sampler uniform. Shaders cannot use both!
	TB_STEEPMAP1		= 25,				// Note: Shared with moonmap2 sampler uniform. Shaders cannot use both!
	TB_MOONMAP2			= TB_STEEPMAP1,		// Note: Shared with steepmap1 sampler uniform. Shaders cannot use both!
	TB_STEEPMAP2		= 26,				// Note: Shared with moonmap3 sampler uniform. Shaders cannot use both!
	TB_MOONMAP3			= TB_STEEPMAP2,		// Note: Shared with steepmap2 sampler uniform. Shaders cannot use both!
	TB_STEEPMAP3		= 27,				// Note: Shared with moonmap4 sampler uniform. Shaders cannot use both!
	TB_MOONMAP4			= TB_STEEPMAP3,		// Note: Shared with steepmap3 sampler uniform. Shaders cannot use both!
	TB_ENVMAP			= 28,
	NUM_TEXTURE_BUNDLES = 29 // limit is 31 (32 max)
};

typedef enum
{
	// material shader stage types
	ST_COLORMAP = 0,			// vanilla Q3A style shader treatening
	ST_DIFFUSEMAP = 0,          // treat color and diffusemap the same
	ST_NORMALMAP,
	ST_NORMALPARALLAXMAP,
	ST_SPECULARMAP,
	ST_GLSL
} stageType_t;

#define SURFSPRITE_FACING_NORMAL	0
#define SURFSPRITE_FACING_UP		1
#define SURFSPRITE_FACING_DOWN		2
#define SURFSPRITE_FACING_ANY		3

struct SurfaceSpriteBlock
{
	float width;
	float height;
	float fadeStartDistance;
	float fadeEndDistance;
	float fadeScale;
	float widthVariance;
	float heightVariance;
};

enum surfaceSpriteType_t
{
	SURFSPRITE_NONE,
	SURFSPRITE_VERTICAL,
	SURFSPRITE_ORIENTED,
	SURFSPRITE_EFFECT,
	SURFSPRITE_WEATHERFX,
	SURFSPRITE_FLATTENED,
};

typedef struct surfaceSprite_s
{
	surfaceSpriteType_t type;

	int				surfaceSpriteType;
	float			width, height, density, wind, windIdle, fadeDist, fadeMax, fadeScale;
	float			fxAlphaStart, fxAlphaEnd, fxDuration, vertSkew;
	vec2_t			variance, fxGrow;
	int				facing;		// Hangdown on vertical sprites, faceup on others.
} surfaceSprite_t;

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader() in tr_bsp.c
#define LIGHTMAP_2D         -4	// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3	// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

typedef struct {
	qboolean		active;
	qboolean		isDetail;
	qboolean		noScreenMap;
	int				isWater;
	bool			hasSpecularMap;
	bool			hasNormalMap;
	
	qboolean		glowMapped;
	int				glowBlend;
	qboolean		glow;
	bool			glowColorFound;
	vec4_t			glowColor;

	qboolean		indoor;
	
	textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];

	waveForm_t		rgbWave;
	colorGen_t		rgbGen;

	waveForm_t		alphaWave;
	alphaGen_t		alphaGen;

	byte			constantColor[4];			// for CGEN_CONST and AGEN_CONST

	uint32_t		stateBits;					// GLS_xxxx mask

	acff_t			adjustColorsForFog;

	int				lightmapStyle;

	stageType_t     type;
	struct shaderProgram_s *glslShaderGroup;
	int glslShaderIndex;

	vec4_t			normalScale;
	vec4_t			specularScale;

	float			cubeMapScale;

	qboolean		isSurfaceSprite;

	surfaceSprite_t	*ss;

	float			emissiveRadiusScale;
	float			emissiveColorScale;
	float			emissiveHeightScale;
	float			emissiveConeAngle;

	float			glowStrength;
	float			glowVibrancy;
	qboolean		glowNoMerge;

	vec4_t			glowMultiplierRGBA;

	vec3_t			particleColor;

	vec3_t			portalColor1;
	vec3_t			portalColor2;
	vec3_t			portalImageColor;
	float			portalImageAlpha;

	int				fireFlyCount;
	vec3_t			fireFlyColor;

	vec3_t			colorMod;

	bool			isFoliage;
	bool			isFoliageChecked;

	bool			useSkyImage;
	bool			envmapUseSkyImage;

	float			envmapStrength;
} shaderStage_t;

struct shaderCommands_s;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

typedef enum {
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct {
	float		cloudHeight;
	image_t		*outerbox[6];
	image_t		*outerboxnight[6];
} skyParms_t;

typedef struct {
	vec3_t	color;
	float	depthForOpaque;
} fogParms_t;


typedef struct shader_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			lightmapIndex[MAXLIGHTMAPS];	// for a shader to match, both name and all lightmapIndex must match
	byte		styles[MAXLIGHTMAPS];

	int			index;					// this shader == tr.shaders[index]
	int64_t		sortedIndex;			// this shader == tr.sortedShaders[sortedIndex]

	float		sort;					// lower numbered shaders draw before higher numbered

	qboolean	defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	qboolean	explicitlyDefined;		// found in a .shader file

	int			surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
	int			contentFlags;

	qboolean	isCursor;

	qboolean	warzoneEnabled;

	qboolean	entityMergable;			// merge across entites optimizable (smoke, blood)

	qboolean	isWater;
	qboolean	hasAlpha;
	qboolean	hasGlow;
	qboolean	isIndoor;

	int			hasAlphaTestBits;		// 0 - uninitialized. -1 no atest bits. 1 has atest bits... 2 has alphaGen... 3 has alpha in texture...
	int			hasSplatMaps;			// 0 - uninitialized. -1 no splatmaps. 1 has splatmaps...

	qboolean	detailMapFromTC;		// 1:1 match to diffuse coordinates... (good for guns/models/etc for adding detail)
	qboolean	detailMapFromWorld;		// From world... Using map coords like splatmaps... (good for splat mapping, etc for varying terrain shading)

	float		customCubeMapScale;		// to override material default for this whole shader...
	float		customSpecularScale;	// to override material default for this whole shader...

	qboolean	isSky;
	skyParms_t	sky;
	fogParms_t	fogParms;

	float		portalRange;			// distance to fog out at
	qboolean	isPortal;

	int			materialType;

	qboolean	isGrass;
	qboolean	isGroundFoliage;
	qboolean	isVines;
	qboolean	isFur;

	float		glowStrength;
	float		glowVibrancy;
	float		emissiveRadiusScale;
	float		emissiveColorScale;

	cullType_t	cullType;				// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	qboolean	polygonOffset;			// set for decals and other items that must be offset 
	qboolean	noMipMaps;				// for console fonts, 2D elements, etc.
	qboolean	noPicMip;				// for images that must always be full resolution
	qboolean	noTC;					// for images that don't want to be texture compressed (eg skies)

	fogPass_t	fogPass;				// draw a blended pass, possibly with depth test equals

	int         vertexAttribs;          // not all shaders will need all data to be gathered

	int			numDeforms;
	deformStage_t	deforms[MAX_SHADER_DEFORMS];

	qboolean		tesselation;
	float			tesselationLevel;
	float			tesselationAlpha;

#ifdef __XYC_SURFACE_SPRITES__
	int			numSurfaceSpriteStages;
#endif //__XYC_SURFACE_SPRITES__

	int			numUnfoggedPasses;
	shaderStage_t	*stages[MAX_SHADER_STAGES];		

	void		(*optimalStageIteratorFunc)( void );

  float clampTime;                                  // time this shader is clamped to
  float timeOffset;                                 // current time offset for this shader

  struct shader_s *remappedShader;                  // current shader this one is remapped too

  qboolean mergeDrawn;

  qboolean warzoneVextexSplat;

  qboolean nocull;

  int maxStage;

  int numStages;

	struct	shader_s	*next;
} shader_t;

static QINLINE qboolean ShaderRequiresCPUDeforms(const shader_t * shader)
{
	if(shader->numDeforms)
	{
		const deformStage_t *ds = &shader->deforms[0];

		if (shader->numDeforms > 1)
			return qtrue;

		switch (ds->deformation)
		{
			case DEFORM_WAVE:
			case DEFORM_BULGE:
				return qfalse;

			default:
				return qtrue;
		}
	}

	return qfalse;
}

enum
{
	ATTR_INDEX_POSITION,
	ATTR_INDEX_TEXCOORD0,
	ATTR_INDEX_TEXCOORD1,
	ATTR_INDEX_NORMAL,
	ATTR_INDEX_COLOR,
	ATTR_INDEX_BONE_INDEXES,
	ATTR_INDEX_BONE_WEIGHTS,

	// GPU vertex animations
	ATTR_INDEX_POSITION2,
	ATTR_INDEX_NORMAL2,

	// Instancing
	ATTR_INDEX_INSTANCES_TEXCOORD,
	ATTR_INDEX_INSTANCES_POSITION,
	ATTR_INDEX_INSTANCES_MVP,
};

enum
{
	ATTR_POSITION = 0x0001,
	ATTR_TEXCOORD0 = 0x0002,
	ATTR_TEXCOORD1 = 0x0004,
	ATTR_NORMAL = 0x0008,
	ATTR_COLOR = 0x0010,
	ATTR_BONE_INDEXES = 0x0020,
	ATTR_BONE_WEIGHTS = 0x0040,

	// for .md3 interpolation
	ATTR_POSITION2 = 0x0080,
	ATTR_NORMAL2 = 0x0100,

	ATTR_INSTANCES_TEXCOORD = 0x0200,
	ATTR_INSTANCES_POSITION = 0x0400,
	ATTR_INSTANCES_MVP = 0x0800,

	ATTR_DEFAULT = ATTR_POSITION,
	ATTR_BITS = ATTR_POSITION |
	ATTR_TEXCOORD0 |
	ATTR_TEXCOORD1 |
	ATTR_NORMAL |
	ATTR_COLOR |
	ATTR_BONE_INDEXES |
	ATTR_BONE_WEIGHTS |
	ATTR_POSITION2 |
	ATTR_NORMAL2 |
	ATTR_INSTANCES_TEXCOORD |
	ATTR_INSTANCES_POSITION |
	ATTR_INSTANCES_MVP
};

enum
{
	GLS_SRCBLEND_ZERO					= (1 << 0),
	GLS_SRCBLEND_ONE					= (1 << 1),
	GLS_SRCBLEND_DST_COLOR				= (1 << 2),
	GLS_SRCBLEND_ONE_MINUS_DST_COLOR	= (1 << 3),
	GLS_SRCBLEND_SRC_ALPHA				= (1 << 4),
	GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA	= (1 << 5),
	GLS_SRCBLEND_DST_ALPHA				= (1 << 6),
	GLS_SRCBLEND_ONE_MINUS_DST_ALPHA	= (1 << 7),
	GLS_SRCBLEND_ALPHA_SATURATE			= (1 << 8),

	GLS_SRCBLEND_BITS					= GLS_SRCBLEND_ZERO
											| GLS_SRCBLEND_ONE
											| GLS_SRCBLEND_DST_COLOR
											| GLS_SRCBLEND_ONE_MINUS_DST_COLOR
											| GLS_SRCBLEND_SRC_ALPHA
											| GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA
											| GLS_SRCBLEND_DST_ALPHA
											| GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
											| GLS_SRCBLEND_ALPHA_SATURATE,

	GLS_DSTBLEND_ZERO					= (1 << 9),
	GLS_DSTBLEND_ONE					= (1 << 10),
	GLS_DSTBLEND_SRC_COLOR				= (1 << 11),
	GLS_DSTBLEND_ONE_MINUS_SRC_COLOR	= (1 << 12),
	GLS_DSTBLEND_SRC_ALPHA				= (1 << 13),
	GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA	= (1 << 14),
	GLS_DSTBLEND_DST_ALPHA				= (1 << 15),
	GLS_DSTBLEND_ONE_MINUS_DST_ALPHA	= (1 << 16),

	GLS_DSTBLEND_BITS					= GLS_DSTBLEND_ZERO
											| GLS_DSTBLEND_ONE
											| GLS_DSTBLEND_SRC_COLOR
											| GLS_DSTBLEND_ONE_MINUS_SRC_COLOR
											| GLS_DSTBLEND_SRC_ALPHA
											| GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
											| GLS_DSTBLEND_DST_ALPHA
											| GLS_DSTBLEND_ONE_MINUS_DST_ALPHA,

	GLS_DEPTHMASK_TRUE					= (1 << 17),

	GLS_POLYMODE_LINE					= (1 << 18),

	GLS_DEPTHTEST_DISABLE				= (1 << 19),

	GLS_DEPTHFUNC_LESS					= (1 << 20),
	GLS_DEPTHFUNC_EQUAL					= (1 << 21),
	GLS_DEPTHFUNC_GREATER				= (1 << 22),

	GLS_DEPTHFUNC_BITS					= GLS_DEPTHFUNC_LESS
											| GLS_DEPTHFUNC_EQUAL
											| GLS_DEPTHFUNC_GREATER,

	GLS_ATEST_GT_0						= (1 << 23),
	GLS_ATEST_LT_128					= (1 << 24),
	GLS_ATEST_GE_128					= (1 << 25),
	GLS_ATEST_GE_192					= (1 << 26),

	GLS_ATEST_BITS						= GLS_ATEST_GT_0
											| GLS_ATEST_LT_128
											| GLS_ATEST_GE_128
											| GLS_ATEST_GE_192,

	GLS_REDMASK_FALSE					= (1 << 27),
	GLS_GREENMASK_FALSE					= (1 << 28),
	GLS_BLUEMASK_FALSE					= (1 << 29),
	GLS_ALPHAMASK_FALSE					= (1 << 30),

	GLS_COLORMASK_BITS					= GLS_REDMASK_FALSE
											| GLS_GREENMASK_FALSE
											| GLS_BLUEMASK_FALSE
											| GLS_ALPHAMASK_FALSE,

	GLS_STENCILTEST_ENABLE				= (1 << 31),

	GLS_DEFAULT							= GLS_DEPTHMASK_TRUE
};

enum
{
	ATEST_NONE,
	ATEST_LT,
	ATEST_GT,
	ATEST_GE
};

enum
{
	LIGHTDEF_USE_LIGHTMAP = 0x0001,
	LIGHTDEF_USE_GLOW_BUFFER = 0x0002,
	LIGHTDEF_USE_CUBEMAP = 0x0004,
	LIGHTDEF_USE_TRIPLANAR = 0x0008,
	LIGHTDEF_USE_REGIONS = 0x0010,
	LIGHTDEF_IS_DETAIL = 0x0020,
	
	LIGHTDEF_COUNT = 1+(LIGHTDEF_USE_LIGHTMAP | LIGHTDEF_USE_GLOW_BUFFER | LIGHTDEF_USE_CUBEMAP | LIGHTDEF_USE_TRIPLANAR | LIGHTDEF_USE_REGIONS | LIGHTDEF_IS_DETAIL)
};


enum
{
	GLSL_INT,
	GLSL_FLOAT,
	GLSL_FLOAT5,
	GLSL_FLOAT7,
	GLSL_VEC2,
	GLSL_VEC3,
	GLSL_VEC4,
	GLSL_MAT16
};

typedef enum
{
	UNIFORM_DIFFUSEMAP = 0,
	UNIFORM_LIGHTMAP,
	UNIFORM_NORMALMAP,
	UNIFORM_DELUXEMAP,
	UNIFORM_SPECULARMAP,
	UNIFORM_POSITIONMAP,
	UNIFORM_WATERPOSITIONMAP,
	UNIFORM_WATERHEIGHTMAP,
	UNIFORM_HEIGHTMAP,
	UNIFORM_GLOWMAP,
	UNIFORM_ENVMAP,

	UNIFORM_TEXTUREMAP,
	UNIFORM_LEVELSMAP,
	UNIFORM_CUBEMAP,
	UNIFORM_SKYCUBEMAP,
	UNIFORM_SKYCUBEMAPNIGHT,
	UNIFORM_EMISSIVECUBE,
	UNIFORM_OVERLAYMAP,
	UNIFORM_STEEPMAP,
	UNIFORM_STEEPMAP1,
	UNIFORM_STEEPMAP2,
	UNIFORM_STEEPMAP3,
	UNIFORM_WATER_EDGE_MAP,
	UNIFORM_SPLATCONTROLMAP,
	UNIFORM_SPLATMAP1,
	UNIFORM_SPLATMAP2,
	UNIFORM_SPLATMAP3,
	UNIFORM_ROADSCONTROLMAP,
	UNIFORM_ROADMAP,
	UNIFORM_DETAILMAP,

	UNIFORM_SCREENIMAGEMAP,
	UNIFORM_SCREENDEPTHMAP,

	UNIFORM_SHADOWMAP,
	UNIFORM_SHADOWMAP2,
	UNIFORM_SHADOWMAP3,
	UNIFORM_SHADOWMAP4,
	UNIFORM_SHADOWMAP5,

	UNIFORM_VOLUMEMAP,

	UNIFORM_MOONMAPS,

	UNIFORM_SHADOWMVP,
	UNIFORM_SHADOWMVP2,
	UNIFORM_SHADOWMVP3,
	UNIFORM_SHADOWMVP4,
	UNIFORM_SHADOWMVP5,

	UNIFORM_ENABLETEXTURES,

	UNIFORM_DIFFUSETEXMATRIX,
	UNIFORM_DIFFUSETEXOFFTURB,

	UNIFORM_TCGEN0,
	UNIFORM_TCGEN0VECTOR0,
	UNIFORM_TCGEN0VECTOR1,

	UNIFORM_TEXTURESCALE,

	UNIFORM_DEFORMGEN,
	UNIFORM_DEFORMPARAMS,

	UNIFORM_COLORGEN,
	UNIFORM_ALPHAGEN,
	UNIFORM_COLOR,
	UNIFORM_BASECOLOR,
	UNIFORM_VERTCOLOR,
	UNIFORM_COLORMOD,

	UNIFORM_DLIGHTINFO,
	UNIFORM_LIGHTFORWARD,
	UNIFORM_LIGHTUP,
	UNIFORM_LIGHTRIGHT,
	UNIFORM_LIGHTORIGIN,
	UNIFORM_LIGHTCOLOR,

	UNIFORM_MODELLIGHTDIR,
	UNIFORM_LIGHTRADIUS,
	UNIFORM_AMBIENTLIGHT,
	UNIFORM_DIRECTEDLIGHT,

	UNIFORM_PORTALRANGE,

	UNIFORM_FOGDISTANCE,
	UNIFORM_FOGDEPTH,
	UNIFORM_FOGEYET,
	UNIFORM_FOGCOLORMASK,

	UNIFORM_MODELMATRIX,
	UNIFORM_VIEWPROJECTIONMATRIX,
	UNIFORM_MODELVIEWPROJECTIONMATRIX,
	UNIFORM_INVPROJECTIONMATRIX,
	UNIFORM_INVEYEPROJECTIONMATRIX,
	UNIFORM_INVMODELVIEWMATRIX,
	UNIFORM_PROJECTIONMATRIX,
	UNIFORM_MODELVIEWMATRIX,
	UNIFORM_VIEWMATRIX,
	UNIFORM_INVVIEWMATRIX,
	UNIFORM_NORMALMATRIX,

	UNIFORM_TIME,
	UNIFORM_VERTEXLERP,
	UNIFORM_NORMALSCALE,
	UNIFORM_SPECULARSCALE,

	UNIFORM_VIEWINFO, // znear, zfar, width/2, height/2
	UNIFORM_VIEWORIGIN,
	UNIFORM_LOCALVIEWORIGIN,
	UNIFORM_VIEWFORWARD,
	UNIFORM_VIEWLEFT,
	UNIFORM_VIEWUP,

	UNIFORM_PLAYERORIGIN,
#ifdef __HUMANOIDS_BEND_GRASS__
	UNIFORM_HUMANOIDORIGINSNUM,
	UNIFORM_HUMANOIDORIGINS,
#endif //__HUMANOIDS_BEND_GRASS__

	UNIFORM_INSTANCE_POSITIONS,
	UNIFORM_INSTANCE_SCALES,
	UNIFORM_INSTANCE_MATRIXES,

	UNIFORM_INVTEXRES,
	UNIFORM_AUTOEXPOSUREMINMAX,
	UNIFORM_TONEMINAVGMAXLINEAR,

	UNIFORM_PRIMARYLIGHTORIGIN,
	UNIFORM_PRIMARYLIGHTCOLOR,
	UNIFORM_PRIMARYLIGHTAMBIENT,
	UNIFORM_PRIMARYLIGHTRADIUS,

	UNIFORM_CUBEMAPINFO,
	UNIFORM_CUBEMAPSTRENGTH,

	UNIFORM_BONE_MATRICES,
#ifdef __EXPERIMETNAL_CHARACTER_EDITOR__
	UNIFORM_BONE_SCALES,
#endif //__EXPERIMETNAL_CHARACTER_EDITOR__

	// UQ1: Added...
	UNIFORM_MINS,
	UNIFORM_MAXS,
	UNIFORM_MAPINFO,

	UNIFORM_SHADOWZFAR,

	UNIFORM_ALPHATEST,

	UNIFORM_DIMENSIONS,
	UNIFORM_MAP_AMBIENT,
	UNIFORM_ZFAR,
	UNIFORM_WORLD,

	UNIFORM_SETTINGS0,
	UNIFORM_SETTINGS1,
	UNIFORM_SETTINGS2,
	UNIFORM_SETTINGS3,
	UNIFORM_SETTINGS4,
	UNIFORM_SETTINGS5,
	UNIFORM_SETTINGS6,
	UNIFORM_LOCAL0,
	UNIFORM_LOCAL1,
	UNIFORM_LOCAL2,
	UNIFORM_LOCAL3,
	UNIFORM_LOCAL4,
	UNIFORM_LOCAL5,
	UNIFORM_LOCAL6,
	UNIFORM_LOCAL7,
	UNIFORM_LOCAL8,
	UNIFORM_LOCAL9,
	UNIFORM_LOCAL10,
	UNIFORM_LOCAL11,
	UNIFORM_LOCAL12,
	UNIFORM_LOCAL13,
	UNIFORM_LOCAL14,
	UNIFORM_LOCAL15,
	UNIFORM_LOCAL16,
	UNIFORM_LOCAL17,
	UNIFORM_LOCAL18,
	UNIFORM_LOCAL19,
	UNIFORM_LOCAL20,
	UNIFORM_LOCAL21,
	UNIFORM_LOCAL22,
	UNIFORM_LOCAL23,
	UNIFORM_LOCAL24,

	UNIFORM_MOON_COUNT,
	UNIFORM_MOON_INFOS,
	UNIFORM_MOON_INFOS2,

	UNIFORM_MATERIAL_SPECULARS,
	UNIFORM_MATERIAL_REFLECTIVENESS,

	UNIFORM_TESSELATION_INFO,
	UNIFORM_TESSELATION_3D_INFO,

	UNIFORM_LIGHTCOUNT,
	UNIFORM_EMISSIVELIGHTCOUNT,
	UNIFORM_LIGHTPOSITIONS2,
	UNIFORM_LIGHTPOSITIONS,
	UNIFORM_LIGHTDISTANCES,
	UNIFORM_LIGHTHEIGHTSCALES,
	UNIFORM_LIGHT_CONEANGLES,
	UNIFORM_LIGHT_CONEDIRECTIONS,
	UNIFORM_LIGHT_MAX,
	UNIFORM_LIGHT_MAX_DISTANCE,
	UNIFORM_LIGHTCOLORS,
	UNIFORM_VLIGHTPOSITIONS2,
	UNIFORM_VLIGHTPOSITIONS,
	UNIFORM_VLIGHTDISTANCES,
	UNIFORM_VLIGHTCOLORS,

	UNIFORM_GLOWMULTIPLIER,

	UNIFORM_SAMPLES,
//#ifdef __SSDO__
//	UNIFORM_SSDO_KERNEL,
//#endif //__SSDO__

	UNIFORM_COUNT
} uniform_t;

// shaderProgram_t represents a pair of one
// GLSL vertex and one GLSL fragment shader
typedef struct shaderProgram_s
{
	std::string					name;

	GLuint						program;
	GLuint						vertexShader;
	GLuint						fragmentShader;
	GLuint						tessControlShader;
	GLuint						tessEvaluationShader;
	GLuint						geometryShader;
	uint32_t					attribs;	// vertex array attributes

	// uniform parameters
	int							numUniforms;
	GLint						*uniforms;
	short						*uniformBufferOffsets;
	char						*uniformBuffer;

	qboolean					tesselation;
	qboolean					geometry;

	GLuint						instances_mvp = 0;
	GLuint						instances_buffer = 0;

	std::string					vertexText;
	std::string					fragText;
	std::string					tessControlText;
	std::string					tessEvaluationText;
	std::string					geometryText;

	// keep the glsl source code around so we can live edit it
	char						vertexTextChar[MAX_GLSL_LENGTH];
	char						fragTextChar[MAX_GLSL_LENGTH];
	int							usageCount;

	qboolean					isBindless;

	GLuint						bindLessBindingPoint = 0;
	bindlessTexturesBlock_t		bindlessBlock;
	bindlessTexturesBlock_t		bindlessBlockPrevious;
	GLuint						bindlessBlockUBO = 0;

#ifdef __USE_GLSL_SHADER_CACHE__
	qboolean					binaryLoaded = qfalse;
#endif //__USE_GLSL_SHADER_CACHE__

	GLuint						LightsBindingPoint = 0;
	LightBlock_t				LightsBlock;
	LightBlock_t				LightsBlockPrevious;
	GLuint						LightsBlockSSBO = 0;

#ifdef __USE_MAP_EMMISSIVE_BLOCK__
	GLuint						EmissiveLightsBindingPoint = 0;
	GLuint						EmissiveLightsBlockSSBO = 0;
#endif //__USE_MAP_EMMISSIVE_BLOCK__
} shaderProgram_t;

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix
	vec3_t		viewangles;

	stereoFrame_t	stereoFrame;

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];
	qboolean	areamaskModified;	// qtrue if areamask changed since last scene

	float		floatTime;			// tr.refdef.time / 1000.0

	float		blurFactor;

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int64_t		num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	struct srfPoly_s	*polys;

	int			numDrawSurfs;
	struct drawSurf_s	*drawSurfs;

	int			numDepthDrawSurfs;
	struct drawSurf_s	*depthDrawSurfs;

	//unsigned int dlightMask;
#ifdef __PSHADOWS__
	int         num_pshadows;
	struct pshadow_s *pshadows;
#endif

	float       sunShadowMvp[5][16];
	float       sunShadowCascadeZfar[5];
	float       sunDir[4];
	float       sunCol[4];
	float       sunAmbCol[4];
	float       colorScale;

#ifdef __REALTIME_CUBEMAP__
	vec3_t		realtimeCubemapOrigin;
	qboolean	realtimeCubemapRendered = qfalse;
#endif //__REALTIME_CUBEMAP__

	float       autoExposureMinMax[2];
	float       toneMinAvgMaxLinear[3];
} trRefdef_t;


//=================================================================================


typedef struct {
	int			originalBrushNumber;
	vec3_t		bounds[2];

	unsigned	colorInt;				// in packed byte format
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	qboolean	hasSurface;
	float		surface[4];
} fog_t;

typedef enum {
	VPF_NONE            = 0x00,
	VPF_NOVIEWMODEL     = 0x01,
	VPF_SHADOWMAP       = 0x02,
	VPF_DEPTHSHADOW     = 0x04,
	VPF_DEPTHCLAMP      = 0x08,
	VPF_ORTHOGRAPHIC    = 0x10,
	VPF_USESUNLIGHT     = 0x20,
	VPF_FARPLANEFRUSTUM = 0x40,
	VPF_NOCUBEMAPS      = 0x80,
	VPF_NOPOSTPROCESS	= 0x100,
	VPF_SHADOWPASS		= 0x200,
	VPF_CUBEMAP			= 0x400,
	VPF_EMISSIVEMAP		= 0x800,
	VPF_SKYCUBEDAY		= 0x1000,
	VPF_SKYCUBENIGHT	= 0x2000,
	VPF_RENDERCUBE		= 0x4000,
} viewParmFlags_t;

typedef struct {
	orientationr_t	ori;
	orientationr_t	world;
	vec3_t		pvsOrigin;			// may be different than or.origin for portals
	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
	int flags;
	vec3_t		realLightOrigin;	// may be different than normal light origin for pshadows
	int			frameSceneNum;		// copied from tr.frameSceneNum
	int			frameCount;			// copied from tr.frameCount
	cplane_t	portalPlane;		// clip anything behind this if mirroring
	vec4_t		emissiveLightDirection;
	vec4_t		emissiveLightOrigin;
	vec3_t		emissiveLightColor;
	int			viewportX, viewportY, viewportWidth, viewportHeight;
	FBO_t		*targetFbo;
	int         targetFboLayer;
	int         targetFboCubemapIndex;
	image_t		*targetCubemapImage;
	float		fovX, fovY;
	float		projectionMatrix[16];
	cplane_t	frustum[5];
	vec3_t		visBounds[2];
	float		zFar;
	float       zNear;
	float		maxEntityRange;
	stereoFrame_t	stereoFrame;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/
typedef byte color4ub_t[4];

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum {
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MDV,
	SF_MDR,
	SF_IQM,
	SF_MDX,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,
	SF_VBO_MESH,
	SF_VBO_MDVMESH,
#ifdef __XYC_SURFACE_SPRITES__
	SF_SPRITES,
#endif //__XYC_SURFACE_SPRITES__

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct drawSurf_s {
	uint64_t			sort;			// bit combination for fast compares
	int                 cubemapIndex;
	qboolean			depthDrawOnly;
	surfaceType_t		*surface;		// any of surface*_t
} drawSurf_t;

#define	MAX_FACE_POINTS		64

#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s {
	surfaceType_t	surfaceType;
	qhandle_t		hShader;
#ifdef __Q3_FOG__
	int64_t			fogIndex;
#endif //__Q3_FOG__
	int				numVerts;
	polyVert_t		*verts;
} srfPoly_t;

typedef struct srfDisplayList_s {
	surfaceType_t	surfaceType;
	int				listNum;
} srfDisplayList_t;


typedef struct srfFlare_s {
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
} srfFlare_t;

#ifdef __XYC_SURFACE_SPRITES__
struct vertexAttribute_t
{
	VBO_t *vbo;
	int index;
	int numComponents;
	GLboolean integerAttribute;
	GLenum type;
	GLboolean normalize;
	int stride;
	int offset;
	int stepRate;
};

struct srfSprites_t
{
	surfaceType_t surfaceType;

	shader_t *shader;
	const surfaceSprite_t *sprite;
	int numSprites;
	VBO_t *vbo;
	IBO_t *ibo;

	int numAttributes;
	vertexAttribute_t *attributes;
};
#endif //__XYC_SURFACE_SPRITES__

#ifndef __CHEAP_VERTS__
typedef struct
{
	vec3_t          xyz;
	vec2_t          st;
	vec2_t          lightmap[MAXLIGHTMAPS];
	vec3_t          normal;
	//vec4_t          tangent;
	vec3_t          lightdir;
	vec4_t			vertexColors[MAXLIGHTMAPS];

#if DEBUG_OPTIMIZEVERTICES
	unsigned int    id;
#endif
} srfVert_t;
#else //__CHEAP_VERTS__
typedef struct
{
	vec3_t          xyz;
	vec2_t          st;
	vec2_t          lightmap[MAXLIGHTMAPS];
	vec3_t          normal;

#if DEBUG_OPTIMIZEVERTICES
	unsigned int    id;
#endif
} srfVert_t;
#endif //__CHEAP_VERTS__

// srfBspSurface_t covers SF_GRID, SF_TRIANGLES, SF_POLY, and SF_VBO_MESH
typedef struct srfBspSurface_s
{
	surfaceType_t   surfaceType;

	// dynamic lighting information
	//int				dlightBits;
#ifdef __PSHADOWS__
	int             pshadowBits;
#endif

	// culling information
	vec3_t			cullBounds[2];
	vec3_t			cullOrigin;
	float			cullRadius;
	cplane_t        cullPlane;

	// indexes
	uint32_t       numIndexes;
	glIndex_t      *indexes;

	// vertexes
	uint32_t       numVerts;
	srfVert_t      *verts;

	// BSP VBO offsets
	uint32_t        firstVert;
	uint32_t        firstIndex;
	glIndex_t       minIndex;
	glIndex_t       maxIndex;

	// static render data
	VBO_t          *vbo;
	IBO_t          *ibo;
	
	// SF_GRID specific variables after here

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;

#ifdef __USE_VBO_AREAS__
	int				vboArea = -1;
#endif //__USE_VBO_AREAS__

	qboolean		isWorldVBO = qfalse;
	hSharedMemory	*sharedMemoryPointer = NULL;
} srfBspSurface_t;

// inter-quake-model
typedef struct {
	int		num_vertexes;
	int		num_triangles;
	int		num_frames;
	int		num_surfaces;
	int		num_joints;
	int		num_poses;
	struct srfIQModel_s	*surfaces;

	float		*positions;
	float		*texcoords;
	float		*normals;
	float		*tangents;
	byte		*blendIndexes;
	union {
		float	*f;
		byte	*b;
	} blendWeights;
	byte		*colors;
	int		*triangles;

	// depending upon the exporter, blend indices and weights might be int/float
	// as opposed to the recommended byte/byte, for example Noesis exports
	// int/float whereas the official IQM tool exports byte/byte
	byte blendWeightsType; // IQM_UBYTE or IQM_FLOAT

	int		*jointParents;
	float		*jointMats;
	float		*poseMats;
	float		*bounds;
	char		*names;
} iqmData_t;

// inter-quake-model surface
typedef struct srfIQModel_s {
	surfaceType_t	surfaceType;
	char		name[MAX_QPATH];
	shader_t	*shader;
	iqmData_t	*data;
	int		first_vertex, num_vertexes;
	int		first_triangle, num_triangles;
} srfIQModel_t;

typedef struct srfVBOMDVMesh_s
{
	surfaceType_t   surfaceType;

	struct mdvModel_s *mdvModel;
	struct mdvSurface_s *mdvSurface;

	// backEnd stats
	int             numIndexes;
	int             numVerts;
	glIndex_t       minIndex;
	glIndex_t       maxIndex;

	// static render data
	VBO_t          *vbo;
	IBO_t          *ibo;
} srfVBOMDVMesh_t;

extern	void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

/*
==============================================================================

SHADOWS

==============================================================================
*/

typedef struct pshadow_s
{
	float sort;
	
	int    numEntities;
	int    entityNums[8];
	vec3_t entityOrigins[8];
	float  entityRadiuses[8];

	float viewRadius;
	vec3_t viewOrigin;

	vec3_t lightViewAxis[3];
	vec3_t lightOrigin;
	vec3_t realLightOrigin;
	float invLightPower;
	float  lightRadius;
	cplane_t cullPlane;
} pshadow_t;


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

#define CULLINFO_NONE   0
#define CULLINFO_BOX    1
#define CULLINFO_SPHERE 2
#define CULLINFO_PLANE  4

typedef struct cullinfo_s {
	int             type;
	vec3_t          bounds[2];
	vec3_t			localOrigin;
	float			radius;
	cplane_t        plane;

	qboolean		centerOriginInitialized;
	vec3_t			centerOrigin;
	float			currentDistance;
	int				centerDistanceTime;
} cullinfo_t;

typedef struct msurface_s {
	//int					viewCount;		// if == tr.viewCount, already added
	struct shader_s		*shader;
#ifdef __Q3_FOG__
	int64_t				fogIndex;
#endif //__Q3_FOG__
	int                 cubemapIndex;
	cullinfo_t          cullinfo;
	
	qboolean			depthDrawOnly;
	qboolean			depthDrawOnlyFoliage;

#ifdef __XYC_SURFACE_SPRITES__
	int					numSurfaceSprites;
	srfSprites_t		*surfaceSprites;
#endif //__XYC_SURFACE_SPRITES__

#ifdef __USE_VBO_AREAS__
	int					vboArea = -1;
	VBO_t				*vbo = NULL;
#endif //__USE_VBO_AREAS__

	qboolean			isMerged;

	surfaceType_t		*data;			// any of srf*_t
} msurface_t;


#define	CONTENTS_NODE		-1
typedef struct mnode_s {
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int             visCounts[MAX_VISCOUNTS];	// node needs to be traversed if current
	vec3_t			mins, maxs, centerOrigin;	// for bounding box culling
	qboolean		centerOriginInitialized;
	struct mnode_s	*parent;

	// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	// leaf specific
	int			cluster;
	int			area;

	int         firstmarksurface;
	int			nummarksurfaces;

	// Occlusion culling...
	qboolean    occluded;
	int			nextOcclusionCheckTime;
	qboolean	lastOcclusionCheckResult;

	GLuint		occlusionCache;
} mnode_t;

typedef struct {
	vec3_t		bounds[2];		// for culling
	int	        firstSurface;
	int			numSurfaces;
} bmodel_t;

typedef struct 
{
	byte		ambientLight[MAXLIGHTMAPS][3];
	byte		directLight[MAXLIGHTMAPS][3];
	byte		styles[MAXLIGHTMAPS];
	byte		latLong[2];
//	byte		pad[2];								// to align to a cache line
} mgrid_t;

typedef struct {
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	dshader_t	*shaders;

	int			numBModels;
	bmodel_t	*bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mnode_t		*nodes;

	int         numWorldSurfaces;

	int			numsurfaces;
	msurface_t	*surfaces;
	int         *surfacesViewCount;
	//int         *surfacesDlightBits;
#ifdef __PSHADOWS__
	int			*surfacesPshadowBits;
#endif

	int			numMergedSurfaces;
	msurface_t	*mergedSurfaces;
	int         *mergedSurfacesViewCount;
	//int         *mergedSurfacesDlightBits;
#ifdef __PSHADOWS__
	int			*mergedSurfacesPshadowBits;
#endif

	int			nummarksurfaces;
	int         *marksurfaces;
	int         *viewSurfaces;

	int			numfogs;
	fog_t		*fogs;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	float		*hdrLightGrid;
	int			lightGridOffsets[8];

	vec3_t		lightGridStep;

	mgrid_t		*lightGridData;
	word		*lightGridArray;
	int			numGridArrayElements;



	int			numClusters;
	int			clusterBytes;
	const byte	*vis;			// may be passed in by CM_LoadMap to save space
	byte		*novis;			// clusterBytes of 0xff (everything is visible)

	char		*entityString;
	char		*entityParsePoint;
} world_t;


/*
==============================================================================
MDV MODELS - meta format for vertex animation models like .md2, .md3, .mdc
==============================================================================
*/
typedef struct
{
	float           bounds[2][3];
	float           localOrigin[3];
	float           radius;
} mdvFrame_t;

typedef struct
{
	float           origin[3];
	float           axis[3][3];
} mdvTag_t;

typedef struct
{
	char            name[MAX_QPATH];	// tag name
} mdvTagName_t;

typedef struct
{
	vec3_t          xyz;
	vec3_t          normal;
	vec3_t          tangent;
	vec3_t          bitangent;
} mdvVertex_t;

typedef struct
{
	float           st[2];
} mdvSt_t;

typedef struct mdvSurface_s
{
	surfaceType_t   surfaceType;

	char            name[MAX_QPATH];	// polyset name

	int             numShaderIndexes;
	int				*shaderIndexes;

	int             numVerts;
	mdvVertex_t    *verts;
	mdvSt_t        *st;

	int             numIndexes;
	glIndex_t      *indexes;

	struct mdvModel_s *model;
} mdvSurface_t;

typedef struct mdvModel_s
{
	int             numFrames;
	mdvFrame_t     *frames;

	int             numTags;
	mdvTag_t       *tags;
	mdvTagName_t   *tagNames;

	int             numSurfaces;
	mdvSurface_t   *surfaces;

	int             numVBOSurfaces;
	srfVBOMDVMesh_t  *vboSurfaces;

	int             numSkins;

	GLuint			vao; // VAO

	uint32_t		ofs_instancesPosition;
	uint32_t		ofs_instancesMVP;
	uint32_t		ofs_instancesTC;

	int				isTree;					// 0 is unchecked. -1 is not tree. 1 is tree.
} mdvModel_t;


//======================================================================

typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MDR,
	MOD_IQM,
/*
Ghoul2 Insert Start
*/
   	MOD_MDXM,
	MOD_MDXA
/*
Ghoul2 Insert End
*/
} modtype_t;

typedef struct mdxmVBOMesh_s
{
	surfaceType_t surfaceType;

	int indexOffset;
	int minIndex;
	int maxIndex;
	int numIndexes;
	int numVertexes;

	VBO_t *vbo;
	IBO_t *ibo;
} mdxmVBOMesh_t;

typedef struct mdxmVBOModel_s
{
	int numVBOMeshes;
	mdxmVBOMesh_t *vboMeshes;

	VBO_t *vbo;
	IBO_t *ibo;
} mdxmVBOModel_t;

typedef struct mdxmData_s
{
	mdxmHeader_t *header;

	// int numLODs; // available in header->numLODs
	mdxmVBOModel_t *vboModels;
} mdxmData_t;

typedef struct model_s {
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;		// model = tr.models[model->index]

	int			dataSize;	// just for listing purposes
	union
	{
		bmodel_t		*bmodel;			// type == MOD_BRUSH
		mdvModel_t		*mdv[MD3_MAX_LODS];	// type == MOD_MESH
		mdrHeader_t		*mdr;				// type == MOD_MDR
		iqmData_t		*iqm;				// type == MOD_IQM
		mdxmData_t		*glm;				// type == MOD_MDXM
		mdxaHeader_t	*gla;				// type == MOD_MDXA
	} data;

	int			 numLods;
} model_t;


#define	MAX_MOD_KNOWN	1024

void		R_ModelInit (void);

model_t		*R_GetModelByHandle( qhandle_t hModel );
int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName );
void		R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void		R_Modellist_f (void);


//====================================================

//#define	MAX_DRAWIMAGES			2048
//#define	MAX_DRAWIMAGES			4096
//#define	MAX_DRAWIMAGES			8192
#define	MAX_DRAWIMAGES			32768
#define	MAX_SKINS				1024


#define	MAX_DRAWSURFS			262144//0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

0 - 1	: dlightmap index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
2 - 6	: fog index
11 - 20	: entity index
21 - 31	: sorted shader index

	TTimo - 1.32
0-1   : dlightmap index
2-6   : fog index
7-16  : entity index
17-30 : sorted shader index

    SmileTheory - for pshadows
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
1     : pshadow flag
0     : dlight flag
*/
#define	QSORT_FOGNUM_SHIFT	1
#define	QSORT_REFENTITYNUM_SHIFT	6
#define	QSORT_SHADERNUM_SHIFT	(QSORT_REFENTITYNUM_SHIFT+REFENTITYNUM_BITS)
#if (QSORT_SHADERNUM_SHIFT+SHADERNUM_BITS) > 64//32
	#error "Need to update sorting, too many bits."
#endif
#define QSORT_POSTRENDER_SHIFT     (QSORT_SHADERNUM_SHIFT + SHADERNUM_BITS)
#if QSORT_POSTRENDER_SHIFT >= 64//32
	#error "Sort field needs to be expanded"
#endif

extern	int			gl_filter_min, gl_filter_max;

/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
} frontEndCounters_t;

#define	FOG_TABLE_SIZE		256
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)


// the renderer front end should never modify glstate_t
typedef struct glstate_s {
	int			currenttextures[NUM_TEXTURE_BUNDLES];
	int			currenttmu;
	qboolean	finishCalled;
	int			texEnv[2];
	int			faceCulling;
	uint32_t	glStateBits;
	uint32_t		vertexAttribsState;
	uint32_t		vertexAttribPointersSet;
	uint32_t        vertexAttribsNewFrame;
	uint32_t        vertexAttribsOldFrame;
	float           vertexAttribsInterpolation;
	int				vertexAttribsTexCoordOffset[2];
	qboolean        vertexAnimation;
	qboolean		skeletalAnimation;
	matrix_t       *boneMatrices;
	int				numBones;
	shaderProgram_t *currentProgram;
	FBO_t          *currentFBO;
	VBO_t          *currentVBO;
	IBO_t          *currentIBO;
	FBO_t          *previousFBO;
	matrix_t        modelview;
	matrix_t        projection;
	matrix_t		modelviewProjection;
	matrix_t		invProjection;
	matrix_t		viewTrans;
	matrix_t		invEyeProjection;

#ifdef __OPENGL_SHARED_CONTEXTS__
	HDC		hDC;
	HGLRC   sharedGLRC[8];	// handle to shared GL rendering contexts
#endif //__OPENGL_SHARED_CONTEXTS__
} glstate_t;

typedef enum {
	MI_NONE,
	MI_NVX,
	MI_ATI
} memInfo_t;

typedef enum {
	TCR_NONE = 0x0000,
	TCR_LATC = 0x0001,
	TCR_BPTC = 0x0002,
} textureCompressionRef_t;

typedef enum {
	IHV_UNKNOWN,

	IHV_NVIDIA,
	IHV_AMD,
	IHV_INTEL
} gpuIhv_t;

// We can't change glConfig_t without breaking DLL/vms compatibility, so
// store extensions we have here.
typedef struct {
	int glslMajorVersion;
	int glslMinorVersion;

	gpuIhv_t hardwareVendor;

	memInfo_t   memInfo;

	int maxRenderbufferSize;
	int maxColorAttachments;

	int textureCompression;

	qboolean immutableTextures;

	qboolean floatLightmap;

	qboolean bindlessTextures;
} glRefConfig_t;


typedef struct {
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	int     c_surfBatches;
	float	c_overDraw;
	
	int		c_vboVertexBuffers;
	int		c_vboIndexBuffers;
	int		c_vboVertexes;
	int		c_vboIndexes;

	int     c_staticVboDraws;
	int     c_dynamicVboDraws;
	int		c_dynamicVboTotalSize;

	int     c_multidraws;
	int     c_multidrawsMerged;

	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		c_flareAdds;
	int		c_flareTests;
	int		c_flareRenders;

	int     c_glslShaderBinds;
	int     c_depthPassBinds;
	int     c_skyBinds;
	int     c_lightallBinds;
	int     c_depthPassDraws;
	int     c_skyDraws;
	int     c_lightallDraws;
	int     c_fogDraws;
	int     c_lightMapsSkipped;
	int     c_tinySkipped;
	int     c_transparancyDraws;
	int     c_glowDraws;

	int		msec;			// total msec for backend run
} backEndCounters_t;

typedef enum {
	RENDERPASS_NONE,
	RENDERPASS_GEOMETRY = RENDERPASS_NONE,
	RENDERPASS_PSHADOWS,
	RENDERPASS_GRASS_PATCHES,
	RENDERPASS_GRASS,
	RENDERPASS_GRASS2,
	RENDERPASS_GRASS3,
	RENDERPASS_GRASS4,
	RENDERPASS_GROUNDFOLIAGE,
	RENDERPASS_VINES,
	RENDERPASS_MIST,
	RENDERPASS_SKY,
	RENDERPASS_MAX,
	// 2D and stuff is after max...
	RENDERPASS_POSTPROCESS
} renderPasses_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct {
	trRefdef_t			refdef;
	viewParms_t			viewParms;
	orientationr_t		ori;
	backEndCounters_t	pc;
	qboolean			isHyperspace;
	trRefEntity_t		*currentEntity;
	qboolean			skyRenderedThisView;	// flag for drawing sun

	void				*ui_MouseCursor;
	bool				ui_MouseCursorTrashAvailable;

	renderPasses_t		renderPass;

	qboolean			projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	byte				color2D[4];
	qboolean			vertexes2D;		// shader needs to be finished
	trRefEntity_t		entity2D;	// currentEntity will point at this when doing 2D rendering

	FBO_t				*last2DFBO;
	qboolean			colorMask[4];
	qboolean			framePostProcessed;
	qboolean			depthFill;

	qboolean				localPlayerValid;
	vec3_t					localPlayerOrigin;
	int						localPlayerGameEntityNum;
	trRefEntity_t			*localPlayerEntity;
	int						localPlayerEntityNum;
	int						humanoidOriginsNum;
	vec3_t					humanoidOrigins[MAX_GRASSBEND_HUMANOIDS];

#ifdef __INDOOR_OUTDOOR_CULLING__
	qboolean				viewIsOutdoors = qtrue;
	int						viewIsOutdoorsCheckTime = 0;
	int						viewIsOutdoorsCulledCount = 0;
	int						viewIsOutdoorsNotCulledCount = 0;
	vec3_t					viewIsOutdoorsHitPosition;
#endif //__INDOOR_OUTDOOR_CULLING__

#ifdef __VR_SEPARATE_EYE_RENDER__
	stereoFrame_t			stereoFrame;
#endif //__VR_SEPARATE_EYE_RENDER__
} backEndState_t;

typedef struct lodModel_s {
	char			modelName[MAX_QPATH];
	char			overrideShader[MAX_QPATH];
	vec3_t			modelScale;
	vec3_t			origin;
	vec3_t			angles;
	vec3_t			scale;
	matrix3_t		axes;
	float			zoffset;
	qhandle_t		qhandle;
	model_t			*model;
} lodModel_t;

#ifdef __DRAW_INDIRECT__
typedef struct {
	unsigned int  count;
	unsigned int  instanceCount;
	unsigned int  firstVert;
	unsigned int  baseInstance;
} DrawArraysIndirectCommand;

typedef struct {
	unsigned int  count;
	unsigned int  instanceCount;
	unsigned int  firstIndex;
	unsigned int  baseVertex;
	unsigned int  baseInstance;
} DrawElementsIndirectCommand;
#endif //__DRAW_INDIRECT__

//#define MAX_MULTIDRAW_PRIMITIVES	16384
#define MAX_MULTIDRAW_PRIMITIVES	65536//1048576//65536

/*
** trGlobals_t 
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct trGlobals_s {
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	int						visIndex;
	int						visClusters[MAX_VISCOUNTS];
	int						visCounts[MAX_VISCOUNTS];	// incremented every time a new vis cluster is entered

	int						frameCount;		// incremented every frame
	int						sceneCount;		// incremented every scene
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int						frameSceneNum;	// zeroed at RE_BeginFrame

	qboolean				changedFrustum;

	qboolean				worldMapLoaded;
	qboolean				worldDeluxeMapping;
	vec2_t                  autoExposureMinMax;
	vec3_t                  toneMinAvgMaxLevel;

	world_t					*world = NULL;
	world_t					*worldSolid = NULL;
	world_t					*worldNonSolid = NULL;

	const byte				*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*defaultImage;
	image_t					*scratchImage[32];
	image_t					*fogImage;
	image_t					*dlightImage;	// inverse-quare highlight for projective adding
	image_t					*flareImage;
	image_t					*whiteImage;			// full of 0xff
	image_t					*blackImage;			// full of 0x00
	image_t					*blackCube;				// cubemap full of 0x00
	image_t					*greyImage;				// full of 0x77
	image_t					*greyCube;				// cubemap full of 0x77
	image_t					*identityLightImage;	// full of tr.identityLightByte
	image_t					*envmapImage;			// generic envmap
	image_t					*envmapSpecImage;		// generic envmap_spec

	//image_t					*previousRenderImage;
	image_t					*randomImage;
	image_t					*random2KImage[2];
	image_t					*randomVolumetricImage[4]; // 32x32x32, 64x64x64, 128x128x128, 256x256x256
	image_t					*randomFbmVolumetricImage[4]; // 32x32x32, 64x64x64, 128x128x128, 256x256x256
	image_t					*smoothNoiseImage;
	image_t					*defaultSplatControlImage;
	image_t					*defaultGrassMapImage;
	image_t					*defaultDetail;
	
	image_t					*waterFoamImage[4];
	image_t					*waterHeightImage[2];
	image_t					*waterNormalImage[2];
#ifdef __WATER_SPEC_AO_IMAGES__
	image_t					*waterSpecularImage[2];
	image_t					*waterAOImage[2];
#endif //__WATER_SPEC_AO_IMAGES__
	image_t					*waterCausicsImage;

	image_t					*mapImage;
	image_t					*heightMapImage;
	image_t					*waterHeightMapImage;
	image_t					*foliageMapImage;
	image_t					*grassImage[16];
	image_t					*grassPatchesAliasImage;
	image_t					*grassAliasImage[4];
	image_t					*seaGrassImage[4];
	image_t					*seaGrassAliasImage[4];
	image_t					*grassMaskImage[10];
	image_t					*vinesImage[16];
	image_t					*vinesAliasImage;
	image_t					*seaVinesImage[4];
	image_t					*seaVinesAliasImage;
	image_t					*foliageAliasImage;
	image_t					*roadsMapImage;
	image_t					*tessellationMapImage;
	image_t					*roadImage;
	image_t					*moonImage[8];
	image_t					*auroraImage[2];
	image_t					*shinyImage;

	image_t					*forcefieldImage;
	image_t					*renderForcefieldDepthImage;

	image_t					*groundFoliageImage[4];

	image_t                 *shadowCubemaps[MAX_DLIGHTS];
	

	image_t					*renderImage;
#ifdef __VR__
	image_t					*renderLeftVRImage;
	image_t					*renderLeftVRDepthImage;
	image_t					*renderRightVRImage;
	image_t					*renderRightVRDepthImage;
#endif //__VR__
	image_t					*renderGUIImage;
	image_t					*glowImage;
	image_t					*renderNormalImage;
	image_t					*renderNormalDetailedImage;
	image_t					*renderTransparancyNormalImage;
	image_t					*renderPositionMapImage;
	image_t					*waterPositionMapImage;
	image_t					*transparancyMapImage;
	image_t					*renderPshadowsImage;
#if 0
	image_t					*glowImageScaled[4];
#else
	image_t					*glowImageScaled[6];
#endif
	image_t					*sunRaysImage;
	image_t					*renderDepthImage;
#ifdef __RENDER_HEIGHTMAP__
	image_t					*HeightmapImage;
#endif //__RENDER_HEIGHTMAP__
	image_t					*genericDepthImage;
	image_t					*zfarDepthImage;
	image_t					*zfarNullImage;
	image_t					*waterDepthImage;
	image_t					*pshadowMaps[MAX_DRAWN_PSHADOWS];
	image_t					*textureScratchImage[2];
	image_t                 *quarterImage[2];
	image_t					*calcLevelsImage;
	image_t					*targetLevelsImage;
	image_t					*fixedLevelsImage;
	image_t					*sunShadowDepthImage[5];
	image_t					*sunSoftShadowImage[5];
	image_t                 *screenShadowImage;
	image_t                 *screenShadowBlurTempImage;
	image_t                 *screenShadowBlurImage;


	image_t                 *linearDepthImage512;
	image_t                 *linearDepthImage2048;
	image_t                 *linearDepthImage4096;
	image_t                 *linearDepthImage8192;
	image_t                 *linearDepthImageZfar;

	image_t					*dofFocusDepthScratchImage;
	image_t					*dofFocusDepthImage;

	image_t                 *renderCubeImage;
	image_t                 *renderSkyImage;
	
	image_t					*textureDepthImage;

	image_t					*awesomiumuiImage;

#ifdef __SSDO__
	image_t					*ssdoNoiseImage;
#endif //__SSDO__

	FBO_t					*renderFbo;
	FBO_t					*renderDepthFbo;

#ifdef __VR__
	FBO_t					*renderLeftVRFbo;
	FBO_t					*renderRightVRFbo;
#endif __VR__

#ifdef __RENDER_HEIGHTMAP__
	FBO_t					*renderHeightmapFbo;
#endif //__RENDER_HEIGHTMAP__
	FBO_t					*renderGlowFbo;
	FBO_t					*renderDetailFbo;
	FBO_t					*renderWaterFbo;
	FBO_t					*renderTransparancyFbo;
	FBO_t					*renderPshadowsFbo;
	FBO_t					*renderNoDepthFbo;
	FBO_t					*renderGUIFbo;
	FBO_t					*waterFbo;
	FBO_t					*transparancyFbo;
#if 0
	FBO_t					*glowFboScaled[4];
#else
	FBO_t					*glowFboScaled[6];
#endif
#if 0
	FBO_t					*msaaResolveFbo;
#endif
	FBO_t					*sunRaysFbo;
	FBO_t					*depthFbo;
	FBO_t					*pshadowFbos[MAX_DRAWN_PSHADOWS];
	FBO_t					*textureScratchFbo[2];
	FBO_t                   *quarterFbo[2];
	FBO_t					*calcLevelsFbo;
	FBO_t					*targetLevelsFbo;
	FBO_t					*sunShadowFbo[5];
	FBO_t					*sunSoftShadowFbo[5];
	FBO_t					*screenShadowFbo;
	FBO_t					*screenShadowBlurTempFbo;
	FBO_t					*screenShadowBlurFbo;
	FBO_t                   *renderCubeFbo;
	FBO_t                   *renderSkyFbo;
	FBO_t					*awesomiumuiFbo;

	shader_t				*defaultShader;
	shader_t				*shadowShader;
	shader_t				*distortionShader;
	shader_t				*projectionShadowShader;
	shader_t				*purpleShader;
	shader_t				*skyDepthShader;

	shader_t				*flareShader;
	shader_t				*sunShader;
	shader_t				*sunFlareShader;
	shader_t				*skyImageShader;

	int						numLightmaps;
	int						lightmapSize;
	image_t					**lightmaps;
	image_t					**deluxemaps;

	image_t					*skyCubeMap;
	image_t					*skyCubeMapNight;

	int                     fatLightmapSize;
	int		                fatLightmapStep;

	qboolean				worldLoaded;

#ifndef __REALTIME_CUBEMAP__
	bool					cubemapsAllocated;
	int                     numCubemaps;
	bool					*cubemapEnabled;
	vec3_t                  *cubemapOrigins;
	float					*cubemapRadius;
	bool					*cubemapRendered;
	image_t                 **cubemaps;
	image_t                 **emissivemaps;
#else //__REALTIME_CUBEMAP__
	image_t                 *realtimeCubemap;
#endif //__REALTIME_CUBEMAP__

	trRefEntity_t			*currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int64_t					currentEntityNum;
	int64_t					shiftedEntityNum;	// currentEntityNum << QSORT_REFENTITYNUM_SHIFT
	model_t					*currentModel;

	//
	// GPU shader programs
	//
	shaderProgram_t whiteShader;
	shaderProgram_t textureColorShader;
	shaderProgram_t instanceShader;
	shaderProgram_t instanceVAOShader;
	shaderProgram_t weatherShader;
	shaderProgram_t occlusionShader;
	shaderProgram_t depthAdjustShader;
	shaderProgram_t lightAllShader[3];
	shaderProgram_t lightAllSplatShader[4];
	shaderProgram_t skyShader[5];
	shaderProgram_t depthPassShader[4];
	shaderProgram_t forcefieldShader;
	//shaderProgram_t sunPassShader;
	//shaderProgram_t moonPassShader;
	//shaderProgram_t planetPassShader;
	shaderProgram_t fireShader;
	shaderProgram_t smokeShader;
	shaderProgram_t magicParticlesShader;
	shaderProgram_t magicParticlesTreeShader;
	shaderProgram_t magicParticlesFireFlyShader;
	shaderProgram_t portalShader;
	shaderProgram_t menuBackgroundShader;
	shaderProgram_t shadowFillShader[4];
	shaderProgram_t pshadowShader[4];
	//shaderProgram_t down4xShader;
	shaderProgram_t bokehShader;
	shaderProgram_t tonemapShader;
	shaderProgram_t calclevels4xShader[2];
	shaderProgram_t shadowmaskShader;
	shaderProgram_t ssaoShader[3];
	//shaderProgram_t depthBlurShader[2];
	shaderProgram_t testcubeShader;
	shaderProgram_t gaussianBlurShader[2];
	shaderProgram_t glowCompositeShader;
	shaderProgram_t dglowDownsample;
	shaderProgram_t dglowUpsample;

	//
	// UQ1: Added shaders...
	//

	shaderProgram_t linearizeDepthShader;
	shaderProgram_t surfaceSpriteShader;
	//shaderProgram_t sssShader;
	//shaderProgram_t sssBlurShader;
	
#ifdef __SSDO__
	shaderProgram_t ssdoShader;
	shaderProgram_t ssdoBlurShader;
#endif //__SSDO__

	shaderProgram_t generateNormalMapShader;
	shaderProgram_t darkexpandShader;
	shaderProgram_t magicdetailShader;
	shaderProgram_t cellShadeShader;
	shaderProgram_t paintShader;
	shaderProgram_t dofShader[3];
	shaderProgram_t anaglyphShader;
	shaderProgram_t skyDomeShader;
	shaderProgram_t waterForwardShader;
	shaderProgram_t waterForwardFastShader;
	shaderProgram_t waterPostForwardShader;
	shaderProgram_t waterPostShader[6];
	shaderProgram_t waterReflectionShader;
	shaderProgram_t transparancyPostShader;
	shaderProgram_t cloudsShader;
	shaderProgram_t foliageShader;
	shaderProgram_t grassPatchesShader;
	shaderProgram_t grassShader[2];
	shaderProgram_t vinesShader;
	shaderProgram_t mistShader;
	//shaderProgram_t hbaoShader;
	//shaderProgram_t hbao2Shader;
	//shaderProgram_t hbaoCombineShader;
	shaderProgram_t esharpeningShader;
	//shaderProgram_t esharpening2Shader;
	shaderProgram_t fxaaShader;
	shaderProgram_t txaaShader;
	shaderProgram_t underwaterShader;
	shaderProgram_t bloomDarkenShader;
	shaderProgram_t bloomBlurShader;
	shaderProgram_t bloomCombineShader;
	//shaderProgram_t lensflareShader;
	shaderProgram_t multipostShader;
	shaderProgram_t anamorphicDarkenShader;
	shaderProgram_t anamorphicBlurShader;
	shaderProgram_t anamorphicCombineShader;
	shaderProgram_t volumeLightShader[3];
	shaderProgram_t volumeLightInvertedShader[3];
	shaderProgram_t volumeLightCombineShader;
	shaderProgram_t softShadowsShader;
	shaderProgram_t shadowBlurShader;
	shaderProgram_t bloomRaysShader;
	shaderProgram_t distanceBlurShader[4];
	shaderProgram_t dofFocusDepthShader;
	shaderProgram_t fogPostShader;
	shaderProgram_t colorCorrectionShader;
	shaderProgram_t showNormalsShader;
	shaderProgram_t showDepthShader;
	shaderProgram_t fastLightingShader;
	shaderProgram_t deferredLightingShader[3];
	shaderProgram_t proceduralShader;
	shaderProgram_t ssdmShader;
	shaderProgram_t ssdmGenerateShader[2];
	//shaderProgram_t ssrShader;
	//shaderProgram_t ssrCombineShader;
	shaderProgram_t testshaderShader;
	shaderProgram_t zFarDepthShader;
	shaderProgram_t zFarCopyShader;
	

	image_t        *anamorphicRenderFBOImage;
	image_t        *bloomRenderFBOImage[3];
	image_t        *volumetricFBOImage;
	image_t        *bloomRaysFBOImage;
	image_t        *waterReflectionRenderImage;
	image_t        *genericFBOImage;
	image_t        *genericFBO2Image;
	image_t        *genericFBO3Image;

	image_t        *dummyImage;
	image_t        *dummyImage2;
	image_t        *dummyImage3;
	image_t        *dummyImage4;

	image_t        *txaaPreviousImage;

	FBO_t          *anamorphicRenderFBO;
	FBO_t          *bloomRenderFBO[3];
	FBO_t		   *volumetricFbo;
	FBO_t		   *bloomRaysFbo;
	FBO_t          *waterReflectionRenderFBO;
	FBO_t          *txaaPreviousFBO;
	FBO_t		   *genericFbo;
	FBO_t		   *genericFbo2;
	FBO_t		   *genericFbo3;
	FBO_t		   *NormalMapDestinationFBO;
	
	FBO_t		   *linearizeDepthFbo;
	FBO_t			*dofFocusDepthFbo;
	
	FBO_t			*depthAdjustFbo;
	FBO_t			*genericDepthFbo;
	FBO_t			*zfarDepthFbo;

	FBO_t		   *ssaoFbo;
	image_t        *ssaoImage;

#ifdef __SSDO__
	FBO_t		   *ssdoFbo1;
	FBO_t		   *ssdoFbo2;
	image_t        *ssdoImage1;
	image_t        *ssdoImage2;
	image_t        *ssdoIlluminationImage;
#endif //__SSDO__
	
	//FBO_t		   *sssFbo1;
	//FBO_t		   *sssFbo2;
	//image_t        *sssImage1;
	//image_t        *sssImage2;

	FBO_t		   *ssdmFbo;
	image_t        *ssdmImage;


	//
	// UQ1: End Added shaders...
	//

	// -----------------------------------------

	viewParms_t				viewParms;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t			ori;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	float                   mapLightScale;
	float                   sunShadowScale;

	qboolean                sunShadows;
	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;
	vec3_t					moonDirection;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

	int						numImages;
	image_t					*images[MAX_DRAWIMAGES];

	int						numFBOs;
	FBO_t					*fbos[MAX_FBOS];

	int						numVBOs;
	VBO_t					*vbos[MAX_VBOS];

	int						numIBOs;
	IBO_t					*ibos[MAX_IBOS];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];

	GLuint					sunFlareQuery[2];
	int						sunFlareQueryIndex;
	qboolean				sunFlareQueryActive[2];

	GLuint					sunFlareVQuery[2];
	int						sunFlareVQueryIndex;
	qboolean				sunFlareVQueryActive[2];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
	float					fogTable[FOG_TABLE_SIZE];

	float					rangedFog;
	float					distanceCull, distanceCullSquared; //rwwRMG - added
	float					occlusionZfar;
	float					occlusionZfarFoliage;
	float					occlusionOriginalZfar;

#ifdef _WIN32
	WinVars_t *wv;
#endif

	// Specific to Jedi Academy
	int						numBSPModels;
	int						currentLevel;

#define	MAX_SPAWN_VARS			64
#define	MAX_SPAWN_VARS_CHARS	4096

	int	numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

#define				MAX_LODMODEL_MODELS 524288
	int						lodModelsCount = 0;
	lodModel_t				lodModels[MAX_LODMODEL_MODELS];

#ifdef __DRAW_INDIRECT__
	GLuint indirectArraysBuffer = 0;
	DrawArraysIndirectCommand drawArraysIndirectCommand[1];

	GLuint indirectElementsBuffer = 0;
	DrawElementsIndirectCommand drawElementsIndirectCommand[1];

	GLuint indirectMultiElementsBuffer = 0;
	DrawElementsIndirectCommand drawMultiElementsCommand[MAX_MULTIDRAW_PRIMITIVES];
#endif //__DRAW_INDIRECT__
} trGlobals_t;

struct glconfigExt_t
{
	glconfig_t *glConfig;

	const char *originalExtensionString;
};

extern backEndState_t	backEnd;
extern trGlobals_t	tr;
extern glstate_t	glState;		// outside of TR since it shouldn't be cleared during ref re-init
extern glRefConfig_t glRefConfig;

//
// cvars
//
extern cvar_t	*r_flareSize;
extern cvar_t	*r_flareFade;
// coefficient for the flare intensity falloff function.
#define FLARE_STDCOEFF "150"
extern cvar_t	*r_flareCoeff;

extern cvar_t	*r_railWidth;
extern cvar_t	*r_railCoreWidth;
extern cvar_t	*r_railSegmentLength;

extern cvar_t	*r_ignore;				// used for debugging anything
extern cvar_t	*r_verbose;				// used for verbose debug spew

extern cvar_t	*r_znear;				// near Z clip plane
extern cvar_t	*r_zproj;				// z distance of projection plane

extern cvar_t	*r_measureOverdraw;		// enables stencil buffer overdraw measurement

extern cvar_t	*r_lodbias;				// push/pull LOD transitions
extern cvar_t	*r_lodscale;

extern cvar_t	*r_inGameVideo;				// controls whether in game video should be draw
extern cvar_t	*r_fastsky;				// controls whether sky should be cleared or drawn
extern cvar_t	*r_drawSun;				// controls drawing of sun quad
extern cvar_t	*r_dynamiclight;		// dynamic lights enabled/disabled
extern cvar_t	*r_volumeLight;

extern	cvar_t	*r_norefresh;			// bypasses the ref rendering
extern	cvar_t	*r_drawentities;		// disable/enable entity rendering
extern	cvar_t	*r_drawworld;			// disable/enable world rendering
extern	cvar_t	*r_speeds;				// various levels of information display
extern  cvar_t	*r_detailTextures;		// enables/disables detail texturing stages
extern	cvar_t	*r_novis;				// disable/enable usage of PVS
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern	cvar_t	*r_nocurves;
extern	cvar_t	*r_showcluster;

extern cvar_t	*r_gamma;

extern  cvar_t  *r_occlusion;
extern cvar_t  *r_occlusionDebug;
extern cvar_t  *r_areaVisDebug;

extern cvar_t  *r_zFarOcclusion;

extern  cvar_t  *r_ext_draw_range_elements;
extern  cvar_t  *r_ext_multi_draw_arrays;
extern  cvar_t  *r_ext_framebuffer_object;
extern  cvar_t  *r_ext_texture_float;
extern  cvar_t  *r_arb_half_float_pixel;
extern  cvar_t  *r_ext_framebuffer_multisample;
extern  cvar_t  *r_arb_seamless_cube_map;
extern  cvar_t  *r_arb_vertex_type_2_10_10_10_rev;

extern cvar_t  *r_lazyFrustum;
extern cvar_t  *r_cacheVisibleSurfaces;

extern	cvar_t	*r_nobind;						// turns off binding to appropriate textures
extern	cvar_t	*r_singleShader;				// make most world faces use default shader
extern	cvar_t	*r_roundImagesDown;
extern	cvar_t	*r_colorMipLevels;				// development aid to see texture mip usage
extern	cvar_t	*r_picmip;						// controls picmip values
extern	cvar_t	*r_finish;
extern	cvar_t	*r_textureMode;
extern	cvar_t	*r_offsetFactor;
extern	cvar_t	*r_offsetUnits;

extern	cvar_t	*r_fullbright;					// avoid lightmap pass
extern	cvar_t	*r_lightmap;					// render lightmaps only
extern	cvar_t	*r_vertexLight;					// vertex lighting mode for better performance
extern	cvar_t	*r_uiFullScreen;				// ui is running fullscreen

extern	cvar_t	*r_logFile;						// number of frames to emit GL logs
extern	cvar_t	*r_showtris;					// enables wireframe rendering of the world
extern	cvar_t	*r_showsky;						// forces sky in front of all surfaces
extern	cvar_t	*r_shownormals;					// draws wireframe normals
extern	cvar_t	*r_showdepth;					// draws linear depth
extern cvar_t	*r_showsplat;
extern	cvar_t	*r_clear;						// force screen clear every frame

extern	cvar_t	*r_shadows;						// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern	cvar_t	*r_flares;						// light flares

extern	cvar_t	*r_intensity;

extern	cvar_t	*r_lockpvs;
extern	cvar_t	*r_noportals;
extern	cvar_t	*r_portalOnly;

extern	cvar_t	*r_subdivisions;
extern	cvar_t	*r_lodCurveError;
extern	cvar_t	*r_skipBackEnd;

extern  cvar_t  *r_mergeMultidraws;
extern  cvar_t  *r_mergeLeafSurfaces;

extern  cvar_t  *r_hdr;
extern  cvar_t  *r_floatLightmap;
extern  cvar_t  *r_postProcess;

extern  cvar_t  *r_toneMap;
extern  cvar_t  *r_forceToneMap;
extern  cvar_t  *r_forceToneMapMin;
extern  cvar_t  *r_forceToneMapAvg;
extern  cvar_t  *r_forceToneMapMax;

extern  cvar_t  *r_forceAutoExposure;
extern  cvar_t  *r_forceAutoExposureMin;
extern  cvar_t  *r_forceAutoExposureMax;

extern  cvar_t  *r_srgb;

extern  cvar_t  *r_depthPrepass;

#ifdef __SSDO__
extern cvar_t  *r_ssdo;
extern cvar_t  *r_ssdoBaseRadius;
extern cvar_t  *r_ssdoMaxOcclusionDist;
#endif //__SSDO__

extern  cvar_t  *r_normalMapping;
extern cvar_t  *r_normalMapQuality;
extern  cvar_t  *r_specularMapping;
extern  cvar_t  *r_deluxeMapping;
extern  cvar_t  *r_cubeMapping;
extern	cvar_t  *r_cubeMapSize;
extern	cvar_t  *r_cubemapStrength;
extern  cvar_t  *r_deluxeSpecular;
extern  cvar_t  *r_specularIsMetallic;
extern  cvar_t  *r_baseNormalX;
extern  cvar_t  *r_baseNormalY;
extern  cvar_t  *r_baseParallax;
extern  cvar_t  *r_baseSpecular;
extern  cvar_t  *r_baseGloss;
extern  cvar_t  *r_dlightMode;
extern  cvar_t  *r_pshadowDist;
extern  cvar_t  *r_recalcMD3Normals;
extern  cvar_t  *r_mergeLightmaps;
extern  cvar_t  *r_imageUpsample;
extern  cvar_t  *r_imageUpsampleMaxSize;
extern cvar_t  *r_imageDownsampleMaxSize;
extern  cvar_t  *r_imageUpsampleType;
extern  cvar_t  *r_genNormalMaps;
extern	cvar_t  *r_dlightShadows;
extern  cvar_t  *r_forceSun;
extern  cvar_t  *r_forceSunMapLightScale;
extern  cvar_t  *r_forceSunLightScale;
extern  cvar_t  *r_forceSunAmbientScale;
extern cvar_t  *r_proceduralSun;
extern cvar_t  *r_proceduralSunScale;
extern cvar_t  *r_glowStrength;
extern cvar_t  *r_glowVibrancy;
extern cvar_t  *r_glowMultiplierR;
extern cvar_t  *r_glowMultiplierG;
extern cvar_t  *r_glowMultiplierB;
extern cvar_t  *r_glowMultiplierA;
extern  cvar_t  *r_sunlightMode;
extern cvar_t  *r_sunlightSpecular;
extern  cvar_t  *r_drawSunRays;
extern cvar_t  *r_shadowSamples;
extern cvar_t  *r_shadowContrast;
extern  cvar_t  *r_shadowFilter;
extern  cvar_t  *r_shadowMapSize;
extern  cvar_t  *r_shadowCascadeZNear;
extern  cvar_t  *r_shadowCascadeZFar;
extern  cvar_t  *r_shadowCascadeZBias;

extern	cvar_t	*r_greyscale;

extern	cvar_t	*r_ignoreGLErrors;

extern	cvar_t	*r_overBrightBits;
extern	cvar_t	*r_mapOverBrightBits;

extern	cvar_t	*r_debugSurface;
extern	cvar_t	*r_simpleMipMaps;

extern	cvar_t	*r_showImages;
extern	cvar_t	*r_debugSort;

extern	cvar_t	*r_printShaders;

extern cvar_t	*r_marksOnTriangleMeshes;

extern cvar_t	*r_dynamicGlow;
extern cvar_t	*r_dynamicGlowPasses;
extern cvar_t	*r_dynamicGlowIntensity;
extern cvar_t	*r_dynamicGlowSoft;


//
// UQ1: Added...
//
extern cvar_t	*r_perf;
extern cvar_t	*r_glslOptimize;
extern cvar_t	*r_useLowP;
extern cvar_t	*r_lowQualityMode;
extern cvar_t	*r_lowVram;
extern cvar_t	*r_useStaticVBO;
extern cvar_t	*r_volumeLightHQ;
extern cvar_t	*r_debugBinds;
extern cvar_t	*r_debugShaderStages;
extern cvar_t	*r_debugImageCrcHashing;
extern cvar_t	*r_debugMapInfo;
extern cvar_t	*r_debugGlowMerge;
extern cvar_t	*r_debugSplatMaps;
extern cvar_t	*r_debugGLSLOptimizer;
extern cvar_t	*r_splatMapping;
extern cvar_t	*r_shadowMaxDepthError;
extern cvar_t	*r_shadowSolidityValue;
extern cvar_t	*r_disableGfxDirEnhancement;
extern cvar_t	*r_skyLightContribution;
extern cvar_t	*r_cubemapCullRange;
extern cvar_t	*r_glslWater;
extern cvar_t	*r_grassLength;
extern cvar_t	*r_grassWaveSpeed;
extern cvar_t	*r_grassWaveSize;
extern cvar_t	*r_multithread;
#ifdef __EMISSIVE_CUBE_IBL__
extern cvar_t	*r_emissiveCubes;
#endif //__EMISSIVE_CUBE_IBL__
extern cvar_t	*r_testvar;
extern cvar_t	*r_steepParallaxEyeX;
extern cvar_t	*r_steepParallaxEyeY;
extern cvar_t	*r_steepParallaxEyeZ;
extern cvar_t	*r_bloom;
extern cvar_t	*r_bloomPasses;
extern cvar_t	*r_bloomDarkenPower;
extern cvar_t	*r_bloomScale;
extern cvar_t	*r_lensflare;
extern cvar_t	*r_anamorphic;
extern cvar_t	*r_anamorphicStrength;
extern cvar_t	*r_bloomRaysSamples;
extern cvar_t	*r_bloomRaysDecay;
extern cvar_t	*r_bloomRaysWeight;
extern cvar_t	*r_bloomRaysDensity;
extern cvar_t	*r_bloomRaysStrength;
extern cvar_t	*r_darkexpand;
extern cvar_t	*r_truehdr;
extern cvar_t  *r_magicdetail;
extern cvar_t  *r_magicdetailStrength;
extern cvar_t  *r_magicdetailMix;
extern cvar_t  *r_dof;
extern cvar_t  *r_fOff1X;
extern cvar_t  *r_fOff1Y;
extern cvar_t  *r_fOff2X;
extern cvar_t  *r_fOff2Y;
extern cvar_t  *r_fOff3X;
extern cvar_t  *r_fOff3Y;
extern cvar_t  *r_debugMapAmbientR;
extern cvar_t  *r_debugMapAmbientG;
extern cvar_t  *r_debugMapAmbientB;
extern cvar_t  *gui_useMenu;
extern cvar_t  *gui_windowTransparancy;
extern cvar_t  *gui_tooltipSize;
extern cvar_t  *gui_tooltipCentered;
extern cvar_t  *r_testvalue0;
extern cvar_t  *r_testvalue1;
extern cvar_t  *r_testvalue2;
extern cvar_t  *r_testvalue3;
extern cvar_t	*r_bindlessTextures;
extern cvar_t  *r_esharpening;
//extern cvar_t  *r_esharpening2;
extern cvar_t  *r_fxaa;
extern cvar_t  *r_fxaaScanMod;
extern cvar_t  *r_txaa;
extern cvar_t  *r_cloudQuality;
extern cvar_t  *r_cloudshadows;
extern cvar_t  *r_underwater;
extern cvar_t  *r_multipost;
extern cvar_t	*r_steepParallax;
extern cvar_t  *r_trueAnaglyph;
extern cvar_t  *r_trueAnaglyphSeparation;
extern cvar_t  *r_trueAnaglyphRed;
extern cvar_t  *r_trueAnaglyphGreen;
extern cvar_t  *r_trueAnaglyphBlue;
extern cvar_t  *r_trueAnaglyphMinDistance;
extern cvar_t  *r_trueAnaglyphMaxDistance;
extern cvar_t  *r_trueAnaglyphParallax;
extern cvar_t  *r_dayNightCycleSpeed;
extern cvar_t  *r_testshader;
extern cvar_t  *r_testshaderValue1;
extern cvar_t  *r_testshaderValue2;
extern cvar_t  *r_testshaderValue3;
extern cvar_t  *r_testshaderValue4;
extern cvar_t  *r_testshaderValue5;
extern cvar_t  *r_testshaderValue6;
extern cvar_t  *r_testshaderValue7;
extern cvar_t  *r_testshaderValue8;
extern cvar_t  *r_testshaderValue9;

//
// UQ1: End Added...
//

//====================================================================

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );
void R_RenderPshadowMaps(const refdef_t *fd);
void R_RenderSunShadowMaps(const refdef_t *fd, int level, vec4_t sundir, float lightHeight, vec3_t lightOrg);
void R_RenderCubemapSide( int cubemapIndex, int cubemapSide, qboolean subscene );
void R_RenderEmissiveMapSide(int cubemapIndex, int cubemapSide, qboolean subscene);
void R_AddMD3Surfaces( trRefEntity_t *e, model_t *currentModel, int entityNum, int64_t shiftedEntityNum);
void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );

void R_DecomposeSort(const uint64_t sort, int64_t *entityNum, shader_t **shader, int64_t *fogNum, int64_t *postRender);

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int64_t fogIndex, int64_t dlightMap, int64_t postRender, int cubemap, qboolean depthDrawOnly);
void R_AddDrawSurfThreaded(surfaceType_t *surface, shader_t *shader, int64_t fogIndex, int64_t dlightMap, int64_t postRender, int cubemap, qboolean depthDrawOnly, int64_t shiftedEntityNum);
bool R_IsPostRenderEntity(int refEntityNum, const trRefEntity_t *refEntity);

void R_CalcTexDirs(vec3_t sdir, vec3_t tdir, const vec3_t v1, const vec3_t v2,
					const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3);
void R_CalcTbnFromNormalAndTexDirs(vec3_t tangent, vec3_t bitangent, vec3_t normal, vec3_t sdir, vec3_t tdir);
qboolean R_CalcTangentVectors(srfVert_t * dv[3]);

#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes
void R_LocalNormalToWorld (const vec3_t local, vec3_t world);
void R_LocalPointToWorld (const vec3_t local, vec3_t world);
int R_CullBox (vec3_t bounds[2]);
int R_CullBoxMinsMaxs(vec3_t mins, vec3_t maxs);
int R_CullLocalBox (vec3_t bounds[2]);
int R_CullPointAndRadiusEx( const vec3_t origin, float radius, const cplane_t* frustum, int numPlanes );
int R_CullPointAndRadius( const vec3_t origin, float radius );
int R_CullLocalPointAndRadius( const vec3_t origin, float radius );

void R_SetupProjection(viewParms_t *dest, float zProj, float zFar, qboolean computeFrustum);
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *ori );

/*
** GL wrapper/helper functions
*/
void	GL_Bind( image_t *image );
void	GL_BindToTMU( image_t *image, int tmu );
void	GL_SetDefaultState (void);
void	GL_SelectTexture( int unit );
void	GL_TextureMode( const char *string );
void	GL_CheckErrs( char *file, int line );
#define GL_CheckErrors(...) GL_CheckErrs(__FILE__, __LINE__)
void	GL_State( uint32_t stateVector );
void    GL_SetProjectionMatrix(matrix_t matrix);
void    GL_SetModelviewMatrix(matrix_t matrix);
void	GL_TexEnv( int env );
void	GL_Cull( int cullType );
void	GLSL_SetBindlessTexture(shaderProgram_t *program, int uniformNum, image_t **images, int arrayID);
void	GLSL_SetBindlessTextureHandle(shaderProgram_t *program, int uniformNum, uint64_t bindlessHandle, int arrayID);

#define LERP( a, b, w ) ( ( a ) * ( 1.0f - ( w ) ) + ( b ) * ( w ) )
#define LUMA( red, green, blue ) ( 0.2126f * ( red ) + 0.7152f * ( green ) + 0.0722f * ( blue ) )

extern glconfig_t  glConfig;
extern glconfigExt_t	glConfigExt;

typedef _skinSurface_t skinSurface_t;

void	RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void	RE_UploadCinematic (int cols, int rows, const byte *data, int client, qboolean dirty);
void	RE_SetRangedFog ( float range );

void	RE_DrawAwesomiumFrame(int x, int y, int w, int h, unsigned char *buffer);
void	RE_MenuOpenFrame(qboolean menuIsOpen);
void	RE_SendInputEvents(qboolean clientKeyStatus[MAX_KEYS], vec2_t clientMouseStatus, qboolean menuOpen);

void		RE_BeginFrame( stereoFrame_t stereoFrame );
void		RE_BeginRegistration( glconfig_t *glconfig );
void		RE_LoadWorldMap( const char *mapname );
void		RE_SendLodmodelPointers(int *count, void *data);
void		RE_SetWorldVisData( const byte *vis );
qhandle_t	RE_RegisterServerModel( const char *name );
qhandle_t	RE_RegisterModel( const char *name );
qhandle_t	RE_RegisterServerSkin( const char *name );
qhandle_t	RE_RegisterSkin( const char *name );
void		RE_Shutdown( qboolean destroyWindow );

qboolean	R_GetEntityToken( char *buffer, int size );

model_t		*R_AllocModel( void );

void    	R_Init( void );
void		R_UpdateSubImage( image_t *image, byte *pic, int x, int y, int width, int height );

void		R_SetColorMappings( void );
void		R_GammaCorrect( byte *buffer, int bufSize );

void	R_ImageList_f( void );
void	R_ImageHogList_f(void);
void	R_SkinList_f( void );
void	R_FontList_f( void );
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516
const void *RB_TakeScreenshotCmd( const void *data );
void	R_ScreenShotTGA_f( void );
void	R_ScreenShotPNG_f( void );
void	R_ScreenShotJPEG_f( void );

void	R_InitFogTable( void );
float	R_FogFactor( float s, float t );
void	R_InitImages( void );
void	R_DeleteTextures( void );
int		R_SumOfUsedImages( void );
void	R_InitSkins( void );
skin_t	*R_GetSkinByHandle( qhandle_t hSkin );

int R_ComputeLOD( trRefEntity_t *ent, model_t *currentModel);

const void *RB_TakeVideoFrameCmd( const void *data );
void RE_HunkClearCrap(void);

//
// tr_shader.c
//
extern const int lightmapsNone[MAXLIGHTMAPS];
extern const int lightmaps2d[MAXLIGHTMAPS];
extern const int lightmapsVertex[MAXLIGHTMAPS];
extern const int lightmapsFullBright[MAXLIGHTMAPS];
extern const byte stylesDefault[MAXLIGHTMAPS];

shader_t	*R_FindShader( const char *name, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage );
shader_t	*R_GetShaderByHandle( qhandle_t hShader );
shader_t	*R_GetShaderByState( int index, long *cycleTime );
shader_t *R_FindShaderByName( const char *name );
void		R_InitShaders( qboolean server );
void		R_ShaderList_f( void );
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_Init( void );
void		GLimp_Shutdown( void );
void		GLimp_EndFrame( void );
void		GLimp_LogComment( char *comment );
void		GLimp_InitExtraExtensions( void );
void		GLimp_Minimize( void );
void		GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] );

/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/

typedef struct stageVars
{
	color4ub_t	colors[SHADER_MAX_VERTEXES];
	vec2_t		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

struct shaderCommands_s 
{
	glIndex_t	indexes[SHADER_MAX_INDEXES] QALIGN(16);
	vec4_t		xyz[SHADER_MAX_VERTEXES] QALIGN(16);
	uint32_t	normal[SHADER_MAX_VERTEXES] QALIGN(16);
	vec2_t		texCoords[SHADER_MAX_VERTEXES][2] QALIGN(16);
#ifdef __VBO_PACK_COLOR__
	float		vertexColors[SHADER_MAX_VERTEXES] QALIGN(16);
#else //!__VBO_PACK_COLOR__
	vec4_t		vertexColors[SHADER_MAX_VERTEXES] QALIGN(16);
#endif //__VBO_PACK_COLOR__
	uint32_t    lightdir[SHADER_MAX_VERTEXES] QALIGN(16);
	//int			vertexDlightBits[SHADER_MAX_VERTEXES] QALIGN(16);

	VBO_t       *vbo;
	IBO_t       *ibo;

	qboolean    useInternalVBO;

	stageVars_t	svars QALIGN(16);

	//color4ub_t	constantColor255[SHADER_MAX_VERTEXES] QALIGN(16);

	shader_t	*shader;
	float		shaderTime;
	int			fogNum;
	int         cubemapIndex;

	//int			dlightBits;	// or together of all vertexDlightBits
#ifdef __PSHADOWS__
	int         pshadowBits;
#endif

	int			firstIndex;
	int			numIndexes;
	int			numVertexes;
	glIndex_t   minIndex;
	glIndex_t   maxIndex;

	int         multiDrawPrimitives;
	GLsizei     multiDrawNumIndexes[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t  *multiDrawFirstIndex[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t  *multiDrawLastIndex[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t	multiDrawFirstIndexIndirect[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t   multiDrawMinIndex[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t   multiDrawMaxIndex[MAX_MULTIDRAW_PRIMITIVES];

	// info extracted from current shader
	int			numPasses;
	void		(*currentStageIteratorFunc)( void );
	shaderStage_t	**xstages;

	qboolean	SSInitializedWind;

	// JA specific
	bool		fading;
};

#ifdef _WIN32
	typedef __declspec(align(16)) shaderCommands_s	shaderCommands_t;
#else
	typedef struct shaderCommands_s  shaderCommands_t;
#endif
extern	shaderCommands_t	tess;
extern	color4ub_t	styleColors[MAX_LIGHT_STYLES];

void RB_BeginSurface(shader_t *shader, int fogNum, int cubemapIndex );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}

void R_DrawElementsVBO( int numIndexes, glIndex_t firstIndex, glIndex_t minIndex, glIndex_t maxIndex, glIndex_t numVerts, qboolean tesselation );
void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );
void RB_StageIteratorVertexLitTexture( void );
void RB_StageIteratorLightmappedMultitexture( void );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, float color[4] );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, float color[4], float s1, float t1, float s2, float t2 );
void RB_InstantQuad( vec4_t quadVerts[4] );
//void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4], vec4_t color, shaderProgram_t *sp, vec2_t invTexRes);
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4]);
void RB_InstantTri2(vec4_t triVerts[3], vec2_t texCoords[3]);

void RB_ShowImages( void );


/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e, model_t *currentModel, int entityNum, int64_t shiftedEntityNum);
void R_AddWorldSurfaces( void );
qboolean R_inPVS( const vec3_t p1, const vec3_t p2, byte *mask );


/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares( void );

void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal );
void RB_AddDlightFlares( void );
void RB_RenderFlares (void);

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( bmodel_t *bmodel, trRefEntity_t *ent, model_t *currentModel, int entityNum, int64_t shiftedEntityNum);
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *ori );
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
int R_LightDirForPoint( vec3_t point, vec3_t lightDir, vec3_t normal, world_t *world );
int R_CubemapForPoint( vec3_t point );


/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd( void );
void RB_ShadowFinish( void );
void RB_ProjectionShadowDeform( void );

/*
============================================================

SKIES

============================================================
*/

void R_BuildCloudData( shaderCommands_t *shader );
void R_InitSkyTexCoords( float cloudLayerHeight );
void R_DrawSkyBox( shaderCommands_t *shader );
void RB_DrawSun( float scale, shader_t *shader );
void RB_DrawMoon(float scale, shader_t *shader);
void RB_ClipSkyPolygons( shaderCommands_t *shader );

/*
============================================================

CURVE TESSELATION

============================================================
*/

#define PATCH_STITCHING

srfBspSurface_t *R_SubdividePatchToGrid( int width, int height,
								srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] );
srfBspSurface_t *R_GridInsertColumn( srfBspSurface_t *grid, int column, int row, vec3_t point, float loderror );
srfBspSurface_t *R_GridInsertRow( srfBspSurface_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfBspSurface_t *grid );

/*
============================================================

MARKERS, POLYGON PROJECTION ON WORLD POLYGONS

============================================================
*/

int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );


/*
============================================================

VERTEX BUFFER OBJECTS

============================================================
*/

uint32_t R_VboPackTangent(vec4_t v);
uint32_t R_VboPackNormal(vec3_t v);
void R_VboUnpackTangent(vec4_t v, uint32_t b);
void R_VboUnpackNormal(vec3_t v, uint32_t b);

#ifdef __VBO_PACK_COLOR__
float R_VboPackColor(vec4_t v);
float *R_VboUnpackColor(float v);
#endif //__VBO_PACK_COLOR__

#ifdef __HALF_FLOAT__
static unsigned half2float(unsigned short h);
unsigned short float2half(unsigned f);
void floattofp16(unsigned char *dst, float *src, unsigned nelem);
void fp16tofloat(float *dst, unsigned char *src, unsigned nelem);

typedef unsigned char	hvec2_t[2], hvec3_t[3], hvec4_t[4], hvec5_t[5];
#endif //__HALF_FLOAT__

VBO_t          *R_CreateVBO(byte * vertexes, int vertexesSize, vboUsage_t usage);

IBO_t          *R_CreateIBO(byte * indexes, int indexesSize, vboUsage_t usage);

void            R_BindVBO(VBO_t * vbo);
void            R_BindNullVBO(void);

void            R_BindIBO(IBO_t * ibo);
void            R_BindNullIBO(void);

void            R_InitVBOs(void);
void            R_ShutdownVBOs(void);
void            R_VBOList_f(void);

void            RB_UpdateVBOs(unsigned int attribBits);


VAO_t			R_CreateVAO(float* vertices, int size, VBO_t *vbo);
void			R_PrintVAO(VAO_t vao, shaderProgram_t *sp, int size, matrix_t mvp, matrix_t model);
void			R_FreeVAO(VAO_t vao);

/*
============================================================

GLSL

============================================================
*/

int GLSL_BeginLoadGPUShaders(void);
void GLSL_EndLoadGPUShaders( int startTime );
void GLSL_ShutdownGPUShaders(void);
void GLSL_VertexAttribsState(uint32_t stateBits);
void GLSL_UpdateTexCoordVertexAttribPointers ( uint32_t attribBits );
void GLSL_VertexAttribPointers(uint32_t attribBits);
void GLSL_BindProgram(shaderProgram_t * program);

void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value);
void GLSL_SetUniformIntxX(shaderProgram_t *program, int uniformNum, const int *elements, int numElements);
void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value);
void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v);
void GLSL_SetUniformFloat7(shaderProgram_t *program, int uniformNum, const float *v);
void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v);
void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v);
void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v);
void GLSL_SetUniformVec4xX(shaderProgram_t *program, int uniformNum, const vec4_t *elements, int numElements);
void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const float *matrix, int numElements = 1);
void GLSL_SetUniformVec2x16(shaderProgram_t *program, int uniformNum, const vec2_t *elements, int numElements);
void GLSL_SetUniformVec2xX(shaderProgram_t *program, int uniformNum, const vec2_t *elements, int numElements);
void GLSL_SetUniformVec3xX(shaderProgram_t *program, int uniformNum, const vec3_t *elements, int numElements);
void GLSL_SetUniformFloatxX(shaderProgram_t *program, int uniformNum, const float *elements, int numElements);
void GLSL_SetUniformVec3x64(shaderProgram_t *program, int uniformNum, const vec3_t *elements, int numElements);
void GLSL_SetUniformFloatx64(shaderProgram_t *program, int uniformNum, const float *elements, int numElements);

/*
============================================================

SCENE GENERATION

============================================================
*/

void R_InitNextFrame( void );

void RE_ClearScene( void );
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddMiniRefEntityToScene( const miniRefEntity_t *miniRefEnt );
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_BeginScene( const refdef_t *fd );
void RE_RenderScene( const refdef_t *fd );
void RE_EndScene( void );

/*
=============================================================

UNCOMPRESSING BONES

=============================================================
*/

#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

void MC_UnCompress(float mat[3][4],const unsigned char * comp);

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

void R_MDRAddAnimSurfaces( trRefEntity_t *ent, model_t *currentModel, int entityNum, int64_t shiftedEntityNum);
void RB_MDRSurfaceAnim( mdrSurface_t *surface );
qboolean R_LoadIQM (model_t *mod, void *buffer, int filesize, const char *name );
void R_AddIQMSurfaces( trRefEntity_t *ent, model_t *currentModel, int entityNum, int64_t shiftedEntityNum);
void RB_IQMSurfaceAnim( surfaceType_t *surface );
int R_IQMLerpTag( orientation_t *tag, iqmData_t *data,
                  int startFrame, int endFrame,
                  float frac, const char *tagName );

/*
Ghoul2 Insert Start
*/
#ifdef _MSC_VER
#pragma warning (disable: 4512)	//default assignment operator could not be gened
#endif
class CRenderableSurface
{
public:
#ifdef _G2_GORE
	int				ident;
#else
	const int		ident;			// ident of this surface - required so the materials renderer knows what sort of surface this refers to 
#endif
	CBoneCache 		*boneCache;
	mdxmVBOMesh_t	*vboMesh;
	mdxmSurface_t	*surfaceData;	// pointer to surface data loaded into file - only used by client renderer DO NOT USE IN GAME SIDE - if there is a vid restart this will be out of wack on the game
#ifdef _G2_GORE
	float			*alternateTex;		// alternate texture coordinates.
	void			*goreChain;

	float			scale;
	float			fade;
	float			impactTime; // this is a number between 0 and 1 that dictates the progression of the bullet impact
#endif

#ifdef _G2_GORE
	CRenderableSurface& operator= ( const CRenderableSurface& src )
	{
		ident	 = src.ident;
		boneCache = src.boneCache;
		surfaceData = src.surfaceData;
		alternateTex = src.alternateTex;
		goreChain = src.goreChain;
		vboMesh = src.vboMesh;

		return *this;
	}
#endif

CRenderableSurface():	
	ident(SF_MDX),
	boneCache(0),
#ifdef _G2_GORE
	surfaceData(0),
	alternateTex(0),
	goreChain(0)
#else
	surfaceData(0)
#endif
	{}

#ifdef _G2_GORE
	void Init()
	{
		ident = SF_MDX;
		boneCache=0;
		surfaceData=0;
		alternateTex=0;
		goreChain=0;
		vboMesh = NULL;
	}
#endif
};

void R_AddGhoulSurfaces( trRefEntity_t *ent, model_t *currentModel, int entityNum, int64_t shiftedEntityNum);
void RB_SurfaceGhoul( CRenderableSurface *surface );
/*
Ghoul2 Insert End
*/

/*
=============================================================
=============================================================
*/
void	R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst );
void	R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );

void	RB_DeformTessGeometry( void );

void	RB_CalcFogTexCoords( float *dstTexCoords );

void	RB_CalcScaleTexMatrix( const float scale[2], float *matrix );
void	RB_CalcScrollTexMatrix( const float scrollSpeed[2], float *matrix );
void	RB_CalcRotateTexMatrix( float degsPerSecond, float *matrix );
void	RB_CalcTurbulentFactors( const waveForm_t *wf, float *amplitude, float *now );
void	RB_CalcTransformTexMatrix( const texModInfo_t *tmi, float *matrix  );
void	RB_CalcStretchTexMatrix( const waveForm_t *wf, float *matrix );

void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
float	RB_CalcWaveAlphaSingle( const waveForm_t *wf );
float	RB_CalcWaveColorSingle( const waveForm_t *wf );

/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/

void RB_ExecuteRenderCommands( const void *data );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define	MAX_RENDER_COMMANDS	0x40000

typedef struct renderCommandList_s {
	byte	cmds[MAX_RENDER_COMMANDS];
	int		used;
} renderCommandList_t;

typedef struct setColorCommand_s {
	int		commandId;
	float	color[4];
} setColorCommand_t;

typedef struct drawBufferCommand_s {
	int		commandId;
	int		buffer;
} drawBufferCommand_t;

typedef struct subImageCommand_s {
	int		commandId;
	image_t	*image;
	int		width;
	int		height;
	void	*data;
} subImageCommand_t;

typedef struct swapBuffersCommand_s {
	int		commandId;
} swapBuffersCommand_t;

typedef struct endFrameCommand_s {
	int		commandId;
	int		buffer;
} endFrameCommand_t;

typedef struct stretchPicCommand_s {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
} stretchPicCommand_t;

typedef struct rotatePicCommand_s {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
	float	a;
} rotatePicCommand_t;

typedef struct drawSurfsCommand_s {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;

typedef enum {
	SSF_JPEG,
	SSF_TGA,
	SSF_PNG
} screenshotFormat_t;

typedef struct screenShotCommand_s {
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	screenshotFormat_t format;
} screenshotCommand_t;

typedef struct videoFrameCommand_s {
	int						commandId;
	int						width;
	int						height;
	byte					*captureBuffer;
	byte					*encodeBuffer;
	qboolean			motionJpeg;
} videoFrameCommand_t;

typedef struct colorMaskCommand_s {
	int commandId;

	GLboolean rgba[4];
} colorMaskCommand_t;

typedef struct clearDepthCommand_s {
	int commandId;
} clearDepthCommand_t;

typedef struct capShadowmapCommand_s {
	int commandId;
	int map;
	int cubeSide;
} capShadowmapCommand_t;

typedef struct postProcessCommand_s {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
} postProcessCommand_t;

typedef struct {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
} drawOcclusionCommand_t;

typedef struct {
	int	commandId;
	int x, y, width, height;
	unsigned char *buffer;
} awesomiumFrameCommand_t;

typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_ROTATE_PIC,
	RC_ROTATE_PIC2,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_DRAW_OCCLUSION,
	RC_SCREENSHOT,
	RC_VIDEOFRAME,
	RC_COLORMASK,
	RC_CLEARDEPTH,
	RC_CAPSHADOWMAP,
	RC_WORLD_EFFECTS,
	RC_POSTPROCESS,
	RC_DRAWAWESOMIUMFRAME
} renderCommand_t;


// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
//#ifdef __MMO__
//#define	MAX_POLYS		2400
//#define	MAX_POLYVERTS	12000
//#else //!__MMO__
#define	MAX_POLYS		600
#define	MAX_POLYVERTS	3000
//#endif //__MMO__

// all of the information needed by the back end must be
// contained in a backEndData_t.
typedef struct backEndData_s {
	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	drawSurf_t	depthDrawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_REFENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
#ifdef __PSHADOWS__
	pshadow_t pshadows[MAX_CALC_PSHADOWS];
#endif
	renderCommandList_t	commands;
} backEndData_t;

extern	int		max_polys;
extern	int		max_polyverts;

extern	backEndData_t	*backEndData;


void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_IssuePendingRenderCommands( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );
void R_AddCapShadowmapCmd( int dlight, int cubeSide );
void R_AddPostProcessCmd (void);

void RE_SetColor( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_RotatePic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a, qhandle_t hShader );
void RE_RotatePic2 ( float x, float y, float w, float h, float s1, float t1, float s2, float t2,float a, qhandle_t hShader );
void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );
void RE_TakeVideoFrame( int width, int height,
		byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg );

/*
Ghoul2 Insert Start
*/
// tr_ghoul2.cpp
void		Multiply_3x4Matrix(mdxaBone_t *out, mdxaBone_t *in2, mdxaBone_t *in);
extern qboolean R_LoadMDXM (model_t *mod, void *buffer, const char *name, qboolean &bAlreadyCached );
extern qboolean R_LoadMDXA (model_t *mod, void *buffer, const char *name, qboolean &bAlreadyCached );
bool LoadTGAPalletteImage ( const char *name, byte **pic, int *width, int *height);
void		RE_InsertModelIntoHash(const char *name, model_t *mod);
void ResetGhoul2RenderableSurfaceHeap();
/*
Ghoul2 Insert End
*/

void R_InitDecals( void );
void RE_ClearDecals( void );
void RE_AddDecalToScene ( qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary );
void R_AddDecals( void );

image_t	*R_FindImageFile( const char *name, imgType_t type, int flags );
image_t	*R_BakeTextures(char names[16][512], int numNames, const char *outputName, imgType_t type, int flags, qboolean isGrass);
void GL_SetupBindlessTexture(image_t *image);

#ifdef __DEFERRED_IMAGE_LOADING__
image_t	*R_DeferImageLoad(const char *name, imgType_t type, int flags);
image_t *R_LoadDeferredImage(image_t *deferredImage);
#endif //__DEFERRED_IMAGE_LOADING__

qhandle_t RE_RegisterShader( const char *name );
qhandle_t RE_RegisterShaderNoMip( const char *name );
const char		*RE_ShaderNameFromIndex(int index);
image_t *R_CreateImage( const char *name, byte *pic, int width, int height, imgType_t type, int flags, int internalFormat );

float ProjectRadius( float r, vec3_t location );
void RE_RegisterModels_StoreShaderRequest(const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke);
qboolean ShaderHashTableExists(void);
extern void R_ImageLoader_Init(void);

// tr_cache.cpp

/*
 * FileHash_t
 * This stores the loaded file information that we need on retrieval
 */
typedef struct FileHash_s
{
	char			fileName[MAX_QPATH];
	qhandle_t		handle;
} FileHash_t;

/*
 * CacheType_e
 * Cache information specific to each file (FIXME: do we need?)
 */
enum CacheType_e
{
	CACHE_NONE,
	CACHE_IMAGE,
	CACHE_MODEL
};

/* and shaderCache_t is needed for the model cache manager */
typedef std::pair<int,int> shaderCacheEntry_t;
typedef std::vector<shaderCacheEntry_t> shaderCache_t;

/*
 * CachedFile_t
 * The actual data stored in the cache
 */
typedef struct CachedFile_s
{
	void			*pDiskImage;			// pointer to data loaded from disk
	char			fileName[MAX_QPATH];	// filename
	int				iLevelLastUsedOn;		// level we last used this on
	int				iPAKChecksum;			// -1 = not from PAK
	int				iAllocSize;				//

	CacheType_e		eCacheType;				// determine which member of the uCache we're going to use
	shaderCache_t	shaderCache;

	CachedFile_s()
	{
		pDiskImage = NULL;
		iLevelLastUsedOn = 0;
		iPAKChecksum = -1;
		eCacheType = CACHE_NONE;
		fileName[0] = '\0';
		iAllocSize = 0;
	}
} CachedFile_t;

/* assetCache_t and loadedMap_t are two definitions that are needed for the manager */
typedef std::map<std::string, CachedFile_t> assetCache_t;
typedef std::unordered_map<std::string, FileHash_t> loadedMap_t;

/* The actual manager itself, which is used in the model and image loading routines. */
class CCacheManager
{
public:
	virtual ~CCacheManager() {}
	
	qhandle_t			SearchLoaded( const char *fileName );
	void				InsertLoaded( const char *fileName, qhandle_t handle );

	int					LoadFile( const char *pFileName, void **ppFileBuffer, qboolean *pbAlreadyCached );
	void				*Allocate( int iSize, void *pvDiskBuffer, const char *psModelFileName, qboolean *bAlreadyFound, memtag_t eTag );
	void				DeleteAll( void );
	void				DumpNonPure( void );

	virtual qboolean	LevelLoadEnd( qboolean bDeleteEverythingNotUsedInThisLevel ) = 0;

protected:
	loadedMap_t loaded;
	assetCache_t cache;
};

class CImageCacheManager : public CCacheManager
{
public:
	qboolean			LevelLoadEnd( qboolean bDeleteEverythingNotUsedInThisLevel );

	void				DeleteLightMaps( void );
};

class CModelCacheManager : public CCacheManager
{
public:
	qboolean			LevelLoadEnd( qboolean bDeleteEverythingNotUsedInThisLevel );
	void				StoreShaderRequest( const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke );
	void				AllocateShaders( const char *psFileName );
};

extern CImageCacheManager *CImgCache;
extern CModelCacheManager *CModelCache;

qboolean C_Models_LevelLoadEnd( qboolean bDeleteEverythingNotUsedInThisLevel );
qboolean C_Images_LevelLoadEnd( void );
void C_LevelLoadBegin(const char *psMapName, ForceReload_e eForceReload);
int C_GetLevel( void );
void C_LevelLoadEnd( void );

void RB_SurfaceGhoul( CRenderableSurface *surf );

image_t *R_CreateNormalMapGLSL ( const char *name, byte *pic, int width, int height, int flags, image_t	*srcImage );

qboolean R_TextureFileExists(char *name);

uint32_t R_TessXYZtoPackedNormals(vec3_t xyz);

#ifdef __XYC_SURFACE_SPRITES__
shader_t *R_CreateShaderFromTextureBundle(const char *name, const textureBundle_t *bundle, uint32_t stateBits);
#endif //__XYC_SURFACE_SPRITES__

extern float mix(float x, float y, float a);

void RB_PBR_DefaultsForMaterial(float *settings, int MATERIAL_TYPE);

char *R_TIL_TextureFileExists(const char *name);

shader_t *R_ReloadShader(const char *name);

/*
============================================================

OCCLUSION QUERY

============================================================
*/

qboolean RB_CheckOcclusion(mnode_t *node);
void RB_OcclusionCulling(void);
void RB_CheckOcclusions(void);


/*
============================================================

zFar Culling System

============================================================
*/

void RB_zFarCullingEndFrame(void);
void RB_zFarCullingBeginFrame(void);


/*
tr_debug.cpp
*/

int getMilliCount();
int getMilliSpan(int nTimeStart);
void DEBUG_StartTimer(char *name, qboolean usePerfCvar);
void DEBUG_EndTimer(qboolean usePerfCvar);

/*
tr_optimize.cpp
*/
void R_OptimizeMesh(uint32_t *numVerts, uint32_t *numIndexes, uint32_t *indexes, vec3_t *verts);


#ifdef __USE_VBO_AREAS__
int GetVBOArea(vec3_t origin);
#endif //__USE_VBO_AREAS__


void GL_SetDepthRange(GLclampd zNear, GLclampd zFar);
void GL_SetDepthFunc(GLenum func);


/*
tr_nuklear_gui.cpp
*/
int GUI_GetMouseCursor(void);
uint64_t GUI_GetMouseCursorBindless(void);

#include "tr_glsl.h"

#endif //TR_LOCAL_H
