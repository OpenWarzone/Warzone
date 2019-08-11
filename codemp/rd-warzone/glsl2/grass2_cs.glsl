layout(vertices = MAX_PATCH_VERTICES) out;
//layout(vertices = 3) out; // (1)

uniform sampler2D							u_SteepMap; // Grass control map...
uniform sampler2D							u_SteepMap1; // Map of another grass...
uniform sampler2D							u_SteepMap2; // Map of another grass...
uniform sampler2D							u_SteepMap3; // Map of another grass...

flat in	int									vertIsSlope[];

uniform vec4								u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4								u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4								u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4								u_Local8; // GRASS_SURFACE_MINIMUM_SIZE, GRASS_DISTANCE_FROM_ROADS, GRASS_HEIGHT, GRASS_SURFACE_SIZE_DIVIDER
uniform vec4								u_Local9; // testvalue0, 1, 2, 3
uniform vec4								u_Local10; // foliageLODdistance, TERRAIN_TESS_OFFSET, GRASS_DENSITY, GRASS_TYPE_UNIFORMALITY
uniform vec4								u_Local11; // GRASS_WIDTH_REPEATS, GRASS_MAX_SLOPE, GRASS_TYPE_UNIFORMALITY_SCALER, 0.0
uniform vec4								u_Local12; // GRASS_SIZE_MULTIPLIER_COMMON, GRASS_SIZE_MULTIPLIER_RARE, GRASS_SIZE_MULTIPLIER_UNDERWATER, 0.0
uniform vec4								u_Local13; // HAVE_GRASS_CONTROL, HAVE_GRASS_CONTROL1, HAVE_GRASS_CONTROL2, HAVE_GRASS_CONTROL3

#define SHADER_MAP_SIZE						u_Local1.r
#define SHADER_SWAY							u_Local1.g
#define SHADER_OVERLAY_SWAY					u_Local1.b
#define SHADER_MATERIAL_TYPE				u_Local1.a

#define SHADER_HAS_STEEPMAP					u_Local2.r
#define SHADER_HAS_WATEREDGEMAP				u_Local2.g
#define SHADER_HAS_NORMALMAP				u_Local2.b
#define SHADER_WATER_LEVEL					u_Local2.a

#define SHADER_HAS_SPLATMAP1				u_Local3.r
#define SHADER_HAS_SPLATMAP2				u_Local3.g
#define SHADER_HAS_SPLATMAP3				u_Local3.b
#define SHADER_HAS_SPLATMAP4				u_Local3.a

#define GRASS_SURFACE_MINIMUM_SIZE			u_Local8.r
#define GRASS_DISTANCE_FROM_ROADS			u_Local8.g
#define GRASS_HEIGHT						u_Local8.b
#define GRASS_SURFACE_SIZE_DIVIDER			u_Local8.a

#define MAX_RANGE							u_Local10.r
#define GRASS_DENSITY						u_Local10.b
#define GRASS_TYPE_UNIFORMALITY				u_Local10.a

#define GRASS_WIDTH_REPEATS					u_Local11.r
#define GRASS_MAX_SLOPE						u_Local11.g
#define GRASS_TYPE_UNIFORMALITY_SCALER		u_Local11.b

#define GRASS_SIZE_MULTIPLIER_COMMON		u_Local12.r
#define GRASS_SIZE_MULTIPLIER_RARE			u_Local12.g
#define GRASS_SIZE_MULTIPLIER_UNDERWATER	u_Local12.b

#define HAVE_GRASS_CONTROL					u_Local13.r
#define HAVE_GRASS_CONTROL1					u_Local13.g
#define HAVE_GRASS_CONTROL2					u_Local13.b
#define HAVE_GRASS_CONTROL3					u_Local13.a

#define MAP_WATER_LEVEL						SHADER_WATER_LEVEL // TODO: Use water map
#define GRASS_TYPE_UNIFORM_WATER			0.66

uniform vec3								u_ViewOrigin;

uniform vec4								u_MapInfo; // MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0
uniform vec4								u_Mins;
uniform vec4								u_Maxs;

//uniform float uTessLevel;
//const float uTessLevel = 7.0;
//float uTessLevel = (u_Local9.r > 1.0) ? u_Local9.r : 1.0;

vec2 GetMapTC(vec3 pos)
{
	vec2 mapSize = u_Maxs.xy - u_Mins.xy;
	return (pos.xy - u_Mins.xy) / mapSize;
}

#define GRASS_ALLOW 0.1

bool CheckGrassMapPosition(vec3 pos)
{
	if (HAVE_GRASS_CONTROL <= 0)
	{
		return true;
	}

	vec2 tc = GetMapTC(pos);
	float grass = textureLod(u_SteepMap, tc, 4.0).r; // Lod 4 to blur the map and find anything close to allowable position...

	if (grass > GRASS_ALLOW)
	{
		return true;
	}

	return false;
}

void main() 
{
	// (2)
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	if (vertIsSlope[0] > 0 || vertIsSlope[1] > 0 || vertIsSlope[2] > 0)
	{
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;// 1.0;
		return;
	}

	//face center------------------------
	vec3 Vert1 = gl_in[0].gl_Position.xyz;
	vec3 Vert2 = gl_in[1].gl_Position.xyz;
	vec3 Vert3 = gl_in[2].gl_Position.xyz;

#ifdef __USE_UNDERWATER_ONLY__
	float waterCheckLevel = MAP_WATER_LEVEL - 128.0;

	if (Vert1.z >= waterCheckLevel && Vert2.z >= waterCheckLevel && Vert3.z >= waterCheckLevel)
	{// Can skip this triangle completely...
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;// 1.0;
		return;
	}
#endif //__USE_UNDERWATER_ONLY__

	vec3 Pos = (Vert1 + Vert2 + Vert3) / 3.0;   //Center of the triangle - copy for later

#ifdef __USE_UNDERWATER_ONLY__
	if (Pos.z >= MAP_WATER_LEVEL)
	{// Do less grasses underwater...
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;// 1.0;
		return;
	}
#endif //__USE_UNDERWATER_ONLY__

	if (!CheckGrassMapPosition(Pos))
	{
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;// 1.0;
		return;
	}

	// UQ1: Checked and distance is faster
	float VertDist = distance(u_ViewOrigin, Pos);

	if (VertDist >= MAX_RANGE + 1024 // Too far from viewer...
		|| (VertDist >= 1024.0 && Pos.z < MAP_WATER_LEVEL && u_ViewOrigin.z >= MAP_WATER_LEVEL) // Underwater and distant and player is not...
		|| (VertDist >= 1024.0 && Pos.z >= MAP_WATER_LEVEL && u_ViewOrigin.z < MAP_WATER_LEVEL)) // Above water and player is below...
	{// Early cull...
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;// 1.0;
		return;
	}

	float falloffStart2 = (MAX_RANGE + 1024) / 1.5;

	if (VertDist >= falloffStart2)
	{
		float falloffEnd = (MAX_RANGE + 1024) - falloffStart2;
		float pDist = clamp((VertDist - falloffStart2) / falloffEnd, 0.0, 1.0);
		float vertDistanceScale2 = 1.0 - pDist; // Scale down to zero size by distance...

		if (vertDistanceScale2 <= 0.05)
		{
			gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;// 1.0;
			return;
		}
	}

	float vSize = (distance(Vert1, Vert2) + distance(Vert1, Vert3) + distance(Vert2, Vert3)) / 3.0;

	if (vSize < GRASS_SURFACE_MINIMUM_SIZE)
	{// Don't even bother...
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;// 1.0;
		return;
	}

	float sizeMult = clamp(float(vSize) / GRASS_SURFACE_SIZE_DIVIDER, 0.0, 1.0); // Scale by vert size, so tiny verts don't get a crapload of grass...

	float uTessLevel = (GRASS_DENSITY * sizeMult > 1.0) ? GRASS_DENSITY * sizeMult : 1.0;

#ifndef __USE_UNDERWATER_ONLY__
	if (Pos.z < MAP_WATER_LEVEL)
	{// Do less grasses underwater...
		uTessLevel = max(float(int(uTessLevel / 4.0)), 1.0);
	}
#endif //__USE_UNDERWATER_ONLY__

	// (3)
	gl_TessLevelOuter[0] = uTessLevel;
	gl_TessLevelOuter[1] = uTessLevel;
	gl_TessLevelOuter[2] = uTessLevel;

	// (4)
	gl_TessLevelInner[0] = uTessLevel;
}
