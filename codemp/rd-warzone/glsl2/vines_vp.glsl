attribute vec3		attr_Position;
attribute vec3		attr_Normal;

flat out int		vertIsSlope;

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

#define M_PI				3.14159265358979323846

float normalToSlope(in vec3 normal) {
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

	return pitch;
}

bool SlopeTooSmall(vec3 normal)
{
	float pitch = normalToSlope( normal.xyz );
	
	if (pitch < 0.0) pitch = -pitch;

	if (pitch <= u_Local11.g/*90.0*/)
	{
		return true; // This slope not an overhang...
	}

	return false;
}

void main()
{
	vertIsSlope = 0;

	if (SlopeTooSmall(attr_Normal.xyz * 2.0 - 1.0))
		vertIsSlope = 1;

	gl_Position = vec4(attr_Position.xyz, 1.0);
	//gl_Position.z += TERRAIN_TESS_OFFSET;
}
