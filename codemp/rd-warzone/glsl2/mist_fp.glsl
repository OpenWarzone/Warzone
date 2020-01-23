#if defined(USE_BINDLESS_TEXTURES)
layout(std140) uniform u_bindlessTexturesBlock
{
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_LightMap;
uniform sampler2D					u_NormalMap;
uniform sampler2D					u_DeluxeMap;
uniform sampler2D					u_SpecularMap;
uniform sampler2D					u_PositionMap;
uniform sampler2D					u_WaterPositionMap;
uniform sampler2D					u_WaterHeightMap;
uniform sampler2D					u_HeightMap;
uniform sampler2D					u_GlowMap;
uniform sampler2D					u_EnvironmentMap;
uniform sampler2D					u_TextureMap;
uniform sampler2D					u_LevelsMap;
uniform sampler2D					u_CubeMap;
uniform sampler2D					u_SkyCubeMap;
uniform sampler2D					u_SkyCubeMapNight;
uniform sampler2D					u_EmissiveCubeMap;
uniform sampler2D					u_OverlayMap;
uniform sampler2D					u_SteepMap;
uniform sampler2D					u_SteepMap1;
uniform sampler2D					u_SteepMap2;
uniform sampler2D					u_SteepMap3;
uniform sampler2D					u_WaterEdgeMap;
uniform sampler2D					u_SplatControlMap;
uniform sampler2D					u_SplatMap1;
uniform sampler2D					u_SplatMap2;
uniform sampler2D					u_SplatMap3;
uniform sampler2D					u_RoadsControlMap;
uniform sampler2D					u_RoadMap;
uniform sampler2D					u_DetailMap;
uniform sampler2D					u_ScreenImageMap;
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2D					u_ShadowMap;
uniform sampler2D					u_ShadowMap2;
uniform sampler2D					u_ShadowMap3;
uniform sampler2D					u_ShadowMap4;
uniform sampler2D					u_ShadowMap5;
uniform sampler3D					u_VolumeMap;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D							u_DiffuseMap;	// Tiling cloud/mist texture...
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec3								u_ViewOrigin;
uniform vec2								u_Dimensions;

uniform vec4								u_Settings1; // IS_DEPTH_PASS, 0.0, 0.0, 0.0
uniform vec4								u_Settings5; // MAP_COLOR_SWITCH_RG, MAP_COLOR_SWITCH_RB, MAP_COLOR_SWITCH_GB, 0.0

#define IS_DEPTH_PASS						u_Settings1.r
#define LEAF_ALPHA_MULTIPLIER				u_Settings1.a

#define MAP_COLOR_SWITCH_RG					u_Settings5.r
#define MAP_COLOR_SWITCH_RB					u_Settings5.g
#define MAP_COLOR_SWITCH_GB					u_Settings5.b

uniform vec4								u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4								u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4								u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4								u_Local8; // MIST_SURFACE_MINIMUM_SIZE, MIST_DISTANCE_FROM_ROADS, MIST_HEIGHT, MIST_SURFACE_SIZE_DIVIDER
uniform vec4								u_Local9; // testvalue0, 1, 2, 3
uniform vec4								u_Local10; // foliageLODdistance, TERRAIN_TESS_OFFSET, MIST_DENSITY, MIST_MAX_SLOPE
uniform vec4								u_Local11; // MIST_ALPHA, MIST_SPEED_X, MIST_SPEED_Y, 0.0

#define SHADER_MAP_SIZE						u_Local1.r
#define SHADER_SWAY							u_Local1.g
#define SHADER_OVERLAY_SWAY					u_Local1.b
#define SHADER_MATERIAL_TYPE				u_Local1.a

#define SHADER_WATER_LEVEL					u_Local2.a

#define SHADER_HAS_SPLATMAP1				u_Local3.r
#define SHADER_HAS_SPLATMAP2				u_Local3.g
#define SHADER_HAS_SPLATMAP3				u_Local3.b
#define SHADER_HAS_SPLATMAP4				u_Local3.a

#define MIST_SURFACE_MINIMUM_SIZE			u_Local8.r
#define MIST_DISTANCE_FROM_ROADS			u_Local8.g
#define MIST_HEIGHT							u_Local8.b
#define MIST_SURFACE_SIZE_DIVIDER			u_Local8.a

#define MAX_RANGE							u_Local10.r
#define TERRAIN_TESS_OFFSET					u_Local10.g
#define MIST_DENSITY						u_Local10.b
#define MIST_MAX_SLOPE						u_Local10.a

#define MIST_ALPHA							u_Local11.r
#define MIST_SPEED							u_Local11.gb

#define MAP_WATER_LEVEL						SHADER_WATER_LEVEL // TODO: Use water map

uniform float								u_Time;

smooth in vec2		vTexCoord;
smooth in vec3		vVertPosition;
//smooth in vec2		vVertNormal;
flat in vec3		vBasePosition;

out vec4			out_Glow;
out vec4			out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4			out_NormalDetail;
#endif //USE_REAL_NORMALMAPS
out vec4			out_Position;

vec4 GetMap( in sampler2D tex, float scale)
{
	/*
	vec4 xaxis = texture(tex, (vVertPosition.yz * scale) + vec2(u_Time * u_Local9.rg));
	vec4 yaxis = texture(tex, (vVertPosition.xz * scale) + vec2(u_Time * u_Local9.rg));
	vec4 zaxis = texture(tex, (vVertPosition.xy * scale) + vec2(u_Time * u_Local9.rg));

	return (xaxis + yaxis + zaxis) / 3.0;
	*/

	//vec2 tc = (vVertPosition.xy * scale) + (vVertPosition.z * scale) + vec2(u_Time * u_Local9.rg);
	float xy = (vVertPosition.x + vVertPosition.y) * 0.5;
	float z = (vVertPosition.z + xy) * 0.5;
	vec2 tc = vec2(xy, z) * scale;
	tc += vec2(0.02 * u_Time, -0.02 * u_Time) * MIST_SPEED;

	tc.x += vBasePosition.z;
	tc.y += (vBasePosition.x+vBasePosition.y);

	return texture(tex, tc);
}

void main() 
{
	vec4 diffuse = GetMap(u_DiffuseMap, 0.02);

	float vm = 1.0 - clamp(distance(vBasePosition.xy, vVertPosition.xy) / MIST_HEIGHT, 0.0, 1.0);
	float hm = 1.0 - clamp(distance(vBasePosition.z, vVertPosition.z) / (MIST_HEIGHT /* *0.75*/), 0.0, 1.0);
	float m = min(vm, hm);
	float d = 1.0 - clamp(distance(u_ViewOrigin, vVertPosition) / MAX_RANGE, 0.0, 1.0);
	m = min(m, d);

	//diffuse.a = 1.0 - clamp(distance(vBasePosition + vec3(0.0, 0.0, MIST_HEIGHT*0.5), vVertPosition) / (MIST_HEIGHT * u_Local9.a), 0.0, 1.0);
	diffuse.a *= m;
	diffuse.a *= MIST_ALPHA;

	gl_FragColor = diffuse;

	out_Glow = vec4(0.0);
	//out_Normal = vec4(vVertNormal.xy, 0.0, 1.0);
	out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
	out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
	//out_Position = vec4(vVertPosition, MATERIAL_PROCEDURALFOLIAGE+1.0);
	out_Position = vec4(0.0);
}
