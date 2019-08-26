//layout(triangles, equal_spacing) in;
layout(triangles, fractional_odd_spacing, ccw) in; // Does rend2 spew out clockwise or counter-clockwise verts???
//layout(triangles, equal_spacing, ccw) in;

uniform sampler2D					u_SplatControlMap;
uniform sampler2D					u_RoadsControlMap;

uniform vec4						u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4						u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4						u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4						u_Local8; // passnum, GRASS_DISTANCE_FROM_ROADS, GRASS_HEIGHT, 0.0
uniform vec4						u_Local9; // testvalue0, 1, 2, 3
uniform vec4						u_Local10; // foliageLODdistance, TERRAIN_TESS_OFFSET, 0.0, GRASS_TYPE_UNIFORMALITY
uniform vec4						u_Local11; // GRASS_WIDTH_REPEATS, GRASS_MAX_SLOPE, 0.0, 0.0

#define SHADER_MAP_SIZE				u_Local1.r
#define SHADER_SWAY					u_Local1.g
#define SHADER_OVERLAY_SWAY			u_Local1.b
#define SHADER_MATERIAL_TYPE		u_Local1.a

#define SHADER_HAS_STEEPMAP			u_Local2.r
#define SHADER_HAS_WATEREDGEMAP		u_Local2.g
#define SHADER_HAS_NORMALMAP		u_Local2.b
#define SHADER_WATER_LEVEL			u_Local2.a

#define SHADER_HAS_SPLATMAP1		u_Local3.r
#define SHADER_HAS_SPLATMAP2		u_Local3.g
#define SHADER_HAS_SPLATMAP3		u_Local3.b
#define SHADER_HAS_SPLATMAP4		u_Local3.a

#define PASS_NUMBER					u_Local8.r
#define GRASS_DISTANCE_FROM_ROADS	u_Local8.g
#define GRASS_HEIGHT				u_Local8.b

#define MAX_RANGE					u_Local10.r
#define TERRAIN_TESS_OFFSET			u_Local10.g
#define GRASS_TYPE_UNIFORMALITY		u_Local10.a

#define GRASS_WIDTH_REPEATS			u_Local11.r
#define GRASS_MAX_SLOPE				u_Local11.g

#define MAP_WATER_LEVEL				SHADER_WATER_LEVEL // TODO: Use water map
#define GRASS_TYPE_UNIFORM_WATER	0.66

uniform vec3						u_ViewOrigin;
uniform float						u_Time;

uniform vec4						u_MapInfo; // MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0
uniform vec4						u_Mins;
uniform vec4						u_Maxs;

//
// General Settings...
//

const float							fWindStrength = 12.0;

vec3 lerp3D(vec3 v0, vec3 v1, vec3 v2)
{
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

int iLerp3D(int i0, int i1, int i2)
{
	return int(gl_TessCoord.x * float(i0) + gl_TessCoord.y * float(i1) + gl_TessCoord.z * int(i2));
}

flat out float inWindPower;

void main() 
{
	vec3 pos = lerp3D(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
	gl_Position = vec4(pos, 1.0);

	// Wind calculation stuff...
	inWindPower = 0.5f + sin(pos.x / 30 + pos.z / 30 + u_Time*(1.2f + fWindStrength / 20.0f));

	if (inWindPower < 0.0f)
		inWindPower = inWindPower * 0.2f;
	else
		inWindPower = inWindPower * 0.3f;

	inWindPower *= fWindStrength;
}
