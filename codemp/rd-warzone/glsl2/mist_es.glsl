//layout(triangles, equal_spacing) in;
layout(triangles, fractional_odd_spacing, ccw) in; // Does rend2 spew out clockwise or counter-clockwise verts???
//layout(triangles, equal_spacing, ccw) in;

uniform vec4								u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4								u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4								u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4								u_Local8; // MIST_SURFACE_MINIMUM_SIZE, MIST_LOD_START_RANGE, MIST_HEIGHT, MIST_SURFACE_SIZE_DIVIDER
uniform vec4								u_Local9; // testvalue0, 1, 2, 3
uniform vec4								u_Local10; // MIST_DISTANCE, TERRAIN_TESS_OFFSET, MIST_DENSITY, MIST_MAX_SLOPE

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
#define MIST_LOD_START_RANGE				u_Local8.g
#define MIST_HEIGHT							u_Local8.b
#define MIST_SURFACE_SIZE_DIVIDER			u_Local8.a

#define MIST_DISTANCE						u_Local10.r
#define TERRAIN_TESS_OFFSET					u_Local10.g
#define MIST_DENSITY						u_Local10.b
#define MIST_MAX_SLOPE						u_Local10.a

#define MAP_WATER_LEVEL						SHADER_WATER_LEVEL // TODO: Use water map

uniform vec3								u_ViewOrigin;

uniform float								u_Time;


//
// General Settings...
//

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
}
