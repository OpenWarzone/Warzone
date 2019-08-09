attribute vec3		attr_Position;
attribute vec3		attr_Normal;

flat out int		vertIsSlope;

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

#define M_PI				3.14159265358979323846

float normalToSlope(in vec3 normal) {
#if 1
	float pitch = 1.0 - (normal.z * 0.5 + 0.5);
	return pitch * 180.0;
#else
	float	forward;
	float	pitch;

	if (normal.g == 0.0 && normal.r == 0.0) {
		if (normal.b > 0.0) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		forward = sqrt(normal.r*normal.r + normal.g*normal.g);
		pitch = (atan(normal.b, forward) * 180 / M_PI);
		if (pitch < 0.0) {
			pitch += 360;
		}
	}

	pitch = -pitch;

	if (pitch > 180)
		pitch -= 360;

	if (pitch < -180)
		pitch += 360;

	pitch += 90.0f;

	return length(pitch);
#endif
}

bool SlopeTooGreat(vec3 normal)
{
#if 1
	float pitch = normalToSlope( normal.xyz );
	
	if (pitch < 0.0) pitch = -pitch;

	if (pitch > MIST_MAX_SLOPE)
	{
		return true; // This slope is too steep for grass...
	}
#else
	if (normal.z <= 0.73 && normal.z >= -0.73)
	{
		return true;
	}
#endif

	return false;
}

void main()
{
	vertIsSlope = 0;

	if (SlopeTooGreat(attr_Normal.xyz * 2.0 - 1.0))
		vertIsSlope = 1;

	gl_Position = vec4(attr_Position.xyz, 1.0);
	//gl_Position.z += TERRAIN_TESS_OFFSET;
}
