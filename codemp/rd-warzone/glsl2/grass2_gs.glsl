#define THREE_WAY_GRASS_CLUMPS // 3 way probably gives better coverage, at extra cost... otherwise 2 way X shape... 

#define MAX_FOLIAGES					85

layout(triangles) in;
//layout(triangles, invocations = 8) in;
layout(triangle_strip, max_vertices = MAX_FOLIAGES) out;


uniform mat4								u_ModelViewProjectionMatrix;

uniform sampler2D							u_RoadsControlMap;
uniform sampler2D							u_SteepMap; // Grass control map...
uniform sampler2D							u_SteepMap1; // Map of another grass...
uniform sampler2D							u_SteepMap2; // Map of another grass...
uniform sampler2D							u_SteepMap3; // Map of another grass...
uniform sampler2D							u_HeightMap;

uniform vec4								u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4								u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4								u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4								u_Local8; // GRASS_SURFACE_MINIMUM_SIZE, GRASS_DISTANCE_FROM_ROADS, GRASS_HEIGHT, GRASS_SURFACE_SIZE_DIVIDER
uniform vec4								u_Local9; // testvalue0, 1, 2, 3
uniform vec4								u_Local10; // foliageLODdistance, TERRAIN_TESS_OFFSET, 0.0, GRASS_TYPE_UNIFORMALITY
uniform vec4								u_Local11; // GRASS_WIDTH_REPEATS, GRASS_MAX_SLOPE, GRASS_TYPE_UNIFORMALITY_SCALER, GRASS_RARE_PATCHES_ONLY
uniform vec4								u_Local12; // GRASS_SIZE_MULTIPLIER_COMMON, GRASS_SIZE_MULTIPLIER_RARE, GRASS_SIZE_MULTIPLIER_UNDERWATER, GRASS_LOD_START_RANGE
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
#define TERRAIN_TESS_OFFSET					u_Local10.g
#define GRASS_TYPE_UNIFORMALITY				u_Local10.a

#define GRASS_WIDTH_REPEATS					u_Local11.r
#define GRASS_MAX_SLOPE						u_Local11.g
#define GRASS_TYPE_UNIFORMALITY_SCALER		u_Local11.b
#define GRASS_RARE_PATCHES_ONLY				u_Local11.a

#define GRASS_SIZE_MULTIPLIER_COMMON		u_Local12.r
#define GRASS_SIZE_MULTIPLIER_RARE			u_Local12.g
#define GRASS_SIZE_MULTIPLIER_UNDERWATER	u_Local12.b
#define GRASS_LOD_START_RANGE				u_Local12.a

#define HAVE_GRASS_CONTROL					u_Local13.r
#define HAVE_GRASS_CONTROL1					u_Local13.g
#define HAVE_GRASS_CONTROL2					u_Local13.b
#define HAVE_GRASS_CONTROL3					u_Local13.a

#define MAP_WATER_LEVEL						SHADER_WATER_LEVEL // TODO: Use water map
#define GRASS_TYPE_UNIFORM_WATER			0.66

uniform vec3								u_ViewOrigin;
uniform vec3								u_PlayerOrigin;
#ifdef __HUMANOIDS_BEND_GRASS__
uniform int									u_HumanoidOriginsNum;
uniform vec3								u_HumanoidOrigins[MAX_GRASSBEND_HUMANOIDS];
#endif //__HUMANOIDS_BEND_GRASS__
uniform float								u_Time;

uniform vec4								u_MapInfo; // MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0
uniform vec4								u_Mins;
uniform vec4								u_Maxs;

flat in float inWindPower[];

smooth out vec2								vTexCoord;
smooth out vec3								vVertPosition;
smooth out vec2								vVertNormal;
flat out int								iGrassType;

//#define __ENCODE_NORMALS_RECONSTRUCT_Z__
#define __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
//#define __ENCODE_NORMALS_CRY_ENGINE__
//#define __ENCODE_NORMALS_EQUAL_AREA_PROJECTION__

#ifdef __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
vec2 EncodeNormal(vec3 n)
{
	float scale = 1.7777;
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;
	return enc;
}
vec3 DecodeNormal(vec2 enc)
{
	vec3 enc2 = vec3(enc.xy, 0.0);
	float scale = 1.7777;
	vec3 nn =
		enc2.xyz*vec3(2.0 * scale, 2.0 * scale, 0.0) +
		vec3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	return vec3(g * nn.xy, g - 1.0);
}
#elif defined(__ENCODE_NORMALS_CRY_ENGINE__)
vec3 DecodeNormal(in vec2 N)
{
	vec2 encoded = N * 4.0 - 2.0;
	float f = dot(encoded, encoded);
	float g = sqrt(1.0 - f * 0.25);
	return vec3(encoded * g, 1.0 - f * 0.5);
}
vec2 EncodeNormal(in vec3 N)
{
	float f = sqrt(8.0 * N.z + 8.0);
	return N.xy / f + 0.5;
}
#elif defined(__ENCODE_NORMALS_EQUAL_AREA_PROJECTION__)
vec2 EncodeNormal(vec3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);
	return n.xy / f + 0.5;
}
vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 n;
	n.xy = fenc*g;
	n.z = 1.0 - f / 2.0;
	return n;
}
#else //__ENCODE_NORMALS_RECONSTRUCT_Z__
vec3 DecodeNormal(in vec2 N)
{
	vec3 norm;
	norm.xy = N * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy));
	return norm;
}
vec2 EncodeNormal(vec3 n)
{
	return vec2(n.xy * 0.5 + 0.5);
}
#endif //__ENCODE_NORMALS_RECONSTRUCT_Z__

#if 0
float encode(float4 color)
{
	int rgba = (int(color.x * 255.0) << 24) + (int(color.y * 255.0) << 16) + (int(color.z * 255.0) << 8) + int(color.w * 255.0);
	return intBitsToFloat(rgba);
}

vec4 decode(float value)
{
	int rgba = floatBitsToInt(value);
	float r = float(rgba >> 24) / 255.0;
	float g = float((rgba & 0x00ff0000) >> 16) / 255.0;
	float b = float((rgba & 0x0000ff00) >> 8) / 255.0;
	float a = float(rgba & 0x000000ff) / 255.0;
	return vec4(r, g, b, a);
}
#endif

mat4 rotationMatrix(vec3 axis, float angle) 
{ 
    axis = normalize(axis); 
    float s = sin(angle); 
    float c = cos(angle); 
    float oc = 1.0 - c; 
     
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0, 
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0, 
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0, 
                0.0,                                0.0,                                0.0,                                1.0); 
}


const vec3							vWindDirection = normalize(vec3(1.0, 1.0, 0.0));


const float PIover180 = 3.1415/180.0; 

const vec3 vBaseDir[] = vec3[] (
	vec3(1.0, 0.0, 0.0),
#ifdef THREE_WAY_GRASS_CLUMPS
	vec3(float(cos(45.0*PIover180)), float(sin(45.0*PIover180)), 0.0f),
	vec3(float(cos(-45.0*PIover180)), float(sin(-45.0*PIover180)), 0.0f)
#else //!THREE_WAY_GRASS_CLUMPS
	vec3(float(cos(90.0*PIover180)), float(sin(90.0*PIover180)), 0.0f)
#endif //THREE_WAY_GRASS_CLUMPS
);

vec2 BakedOffsetsBegin[16] = vec2[]
(
	vec2( 0.0, 0.0 ),
	vec2( 0.25, 0.0 ),
	vec2( 0.5, 0.0 ),
	vec2( 0.75, 0.0 ),
	vec2( 0.0, 0.25 ),
	vec2( 0.25, 0.25 ),
	vec2( 0.5, 0.25 ),
	vec2( 0.75, 0.25 ),
	vec2( 0.0, 0.5 ),
	vec2( 0.25, 0.5 ),
	vec2( 0.5, 0.5 ),
	vec2( 0.75, 0.5 ),
	vec2( 0.0, 0.75 ),
	vec2( 0.25, 0.75 ),
	vec2( 0.5, 0.75 ),
	vec2( 0.75, 0.75 )
);


const vec2 roadPx = vec2(1.0 / 2048.0);

vec3 vLocalSeed;

// This function returns random number from zero to one
float randZeroOne()
{
	uint n = floatBitsToUint(vLocalSeed.y * 214013.0 + vLocalSeed.x * 2531011.0 + vLocalSeed.z * 141251.0);
	n = n * (n * n * 15731u + 789221u);
	n = (n >> 9u) | 0x3F800000u;

	float fRes = 2.0 - uintBitsToFloat(n);
	vLocalSeed = vec3(vLocalSeed.x + 147158.0 * fRes, vLocalSeed.y*fRes + 415161.0 * fRes, vLocalSeed.z + 324154.0*fRes);
	return fRes;
}

int randomInt(int min, int max)
{
	float fRandomFloat = randZeroOne();
	return int(float(min) + fRandomFloat*float(max - min));
}

#define HASHSCALE1 .1031

float random(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE1);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise(in vec2 st) {
	vec2 i = floor(st);
	vec2 f = fract(st);

	// Four corners in 2D of a tile
	float a = random(i);
	float b = random(i + vec2(1.0, 0.0));
	float c = random(i + vec2(0.0, 1.0));
	float d = random(i + vec2(1.0, 1.0));

	// Smooth Interpolation

	// Cubic Hermine Curve.  Same as SmoothStep()
	vec2 u = f*f*(3.0 - 2.0*f);
	// u = smoothstep(0.,1.,f);

	// Mix 4 coorners percentages
	return mix(a, b, u.x) +
		(c - a)* u.y * (1.0 - u.x) +
		(d - b) * u.x * u.y;
}

float GetRoadFactor(vec2 pixel)
{
	float roadScale = 1.0;

	if (SHADER_HAS_SPLATMAP4 > 0.0)
	{// Also grab the roads map, if we have one...
		float road = texture(u_RoadsControlMap, pixel).r;

		if (road > GRASS_DISTANCE_FROM_ROADS)
		{
			roadScale = 0.0;
		}
		else if (road > 0.0)
		{
			roadScale = 1.0 - clamp(road / GRASS_DISTANCE_FROM_ROADS, 0.0, 1.0);
		}
		else
		{
			roadScale = 1.0;
		}
	}
	else
	{
		roadScale = 1.0;
	}

	return 1.0 - clamp(roadScale * 0.6 + 0.4, 0.0, 1.0);
}

vec2 GetMapTC(vec3 pos)
{
	vec2 mapSize = u_Maxs.xy - u_Mins.xy;
	return (pos.xy - u_Mins.xy) / mapSize;
}

float LDHeightForPosition(vec3 pos)
{
	return noise(vec2(pos.xy * 0.00875));
}

float OffsetForPosition(vec3 pos)
{
	vec2 pixel = GetMapTC(pos);
	float roadScale = GetRoadFactor(pixel);
	float SmoothRand = LDHeightForPosition(pos);
	float offsetScale = SmoothRand * clamp(1.0 - roadScale, 0.75, 1.0);

	float offset = max(offsetScale, roadScale) - 0.5;
	return offset * TERRAIN_TESS_OFFSET;
}

#define GRASS_CULL 0.5
#define GRASS_ALLOW 0.1

bool CheckAltGrass(vec2 tc)
{
	if (HAVE_GRASS_CONTROL1 > 0.0)
	{
		float nograss = textureLod(u_SteepMap1, tc, 0.0).r; // Lod level to add a gap around other grass maps...

		if (nograss > GRASS_CULL)
		{// Another grass here...
			return true;
		}
	}

	if (HAVE_GRASS_CONTROL2 > 0.0)
	{
		float nograss = textureLod(u_SteepMap2, tc, 0.0).r; // Lod level to add a gap around other grass maps...

		if (nograss > GRASS_CULL)
		{// Another grass here...
			return true;
		}
	}

	if (HAVE_GRASS_CONTROL3 > 0.0)
	{
		float nograss = textureLod(u_SteepMap3, tc, 0.0).r; // Lod level to add a gap around other grass maps...

		if (nograss > GRASS_CULL)
		{// Another grass here...
			return true;
		}
	}

	return false;
}

bool CheckGrassMapPosition(vec3 pos)
{
	vec2 tc = GetMapTC(pos);

	if (HAVE_GRASS_CONTROL <= 0)
	{
		if (CheckAltGrass(tc))
		{
			return false;
		}

		return true;
	}

	float grass = textureLod(u_SteepMap, tc, 0.0).r;

	if (grass > 0.5)
	{
		if (CheckAltGrass(tc))
		{
			return false;
		}

		return true;
	}

	return false;
}

void main()
{
	iGrassType = 0;

	//
	//
	//

	//face center------------------------
	vec3 Vert1 = gl_in[0].gl_Position.xyz + OffsetForPosition(gl_in[0].gl_Position.xyz);
	vec3 Vert2 = gl_in[1].gl_Position.xyz + OffsetForPosition(gl_in[1].gl_Position.xyz);
	vec3 Vert3 = gl_in[2].gl_Position.xyz + OffsetForPosition(gl_in[2].gl_Position.xyz);

	vec3 vGrassFieldPos = (Vert1 + Vert2 + Vert3) / 3.0;   //Center of the triangle - copy for later
														   //-----------------------------------

	if (!CheckGrassMapPosition(vGrassFieldPos))
	{
		return;
	}

	vLocalSeed = vGrassFieldPos;
								// invocations support...
								//	vLocalSeed = Pos*float(gl_InvocationID);

	vec3 control;
	control.r = randZeroOne();
	control.g = randZeroOne();
	control.b = randZeroOne();

	float VertDist2 = distance(u_ViewOrigin, vGrassFieldPos);

	if (VertDist2 >= MAX_RANGE)
	{// Too far from viewer... Cull...
		return;
	}

	float heightAboveWater = vGrassFieldPos.z - MAP_WATER_LEVEL;
	float heightAboveWaterLength = length(heightAboveWater);

	if (heightAboveWaterLength <= 128.0)
	{// Too close to water edge...
		return;
	}

	float vertDistanceScale = 1.0;
	float falloffStart = MAX_RANGE / 1.5;

	if (VertDist2 >= falloffStart)
	{
		float falloffEnd = MAX_RANGE - falloffStart;
		float pDist = clamp((VertDist2 - falloffStart) / falloffEnd, 0.0, 1.0);
		vertDistanceScale = 1.0 - pDist; // Scale down to zero size by distance...

		if (vertDistanceScale <= 0.05)
		{
			return;
		}
	}

	vec4 controlMap;

	controlMap.rgb = control.rgb;
	controlMap.a = 1.0;

	vec2 pixel = GetMapTC(vGrassFieldPos);

	if (SHADER_HAS_SPLATMAP4 > 0.0)
	{// Also grab the roads map, if we have one...
		controlMap.a = 1.0 - GetRoadFactor(pixel);

		//if (controlMap.a == 0.0)
		if (controlMap.a <= 0.7)
		{// Road or road edge...
			return;
		}
	}

	int lodLevel = 0;

	if (VertDist2 >= GRASS_LOD_START_RANGE)
	{
		if (VertDist2 >= GRASS_LOD_START_RANGE * 4.0)
		{
			lodLevel = 3;
		}
		else if (VertDist2 >= GRASS_LOD_START_RANGE * 2.0)
		{
			lodLevel = 2;
		}
		else
		{
			lodLevel = 1;
		}
	}


	float controlMapScale = length(controlMap.rgb) / 3.0;
	controlMapScale *= controlMapScale;
	controlMapScale += 0.1;
	controlMapScale = clamp(controlMapScale, 0.0, 1.0);

	float fSizeRandomness = clamp(randZeroOne() * 0.25 + 0.75, 0.0, 1.0);
	float sizeMult = 1.25;
	float fGrassPatchHeight = 1.0;

	vec2 tcOffsetBegin;
	vec2 tcOffsetEnd;

#if defined(__USE_UNDERWATER_ONLY__)
	iGrassType = 0;

	vLocalSeed = round(vGrassFieldPos * GRASS_TYPE_UNIFORMALITY_SCALER) + 1.0;

	if (randZeroOne() > GRASS_TYPE_UNIFORM_WATER)
	{// Randomize...
		iGrassType = randomInt(0, 3);
	}

	if (heightAboveWaterLength <= 192.0)
	{// When near water edge, reduce the size of the grass...
		sizeMult *= clamp(heightAboveWaterLength / 192.0, 0.0, 1.0) * fSizeRandomness;
	}
	else
	{// Deep underwater plants draw larger...
		sizeMult *= clamp(1.0 + (heightAboveWaterLength / 192.0), 1.0, 16.0);
	}

	sizeMult *= GRASS_SIZE_MULTIPLIER_UNDERWATER;

	tcOffsetBegin = BakedOffsetsBegin[iGrassType];
	tcOffsetEnd = tcOffsetBegin + 0.25;

	iGrassType = 1;
#else //!defined(__USE_UNDERWATER_ONLY__)
	if (heightAboveWater < 0.0)
	{
		iGrassType = 0;

		vLocalSeed = round(vGrassFieldPos * GRASS_TYPE_UNIFORMALITY_SCALER) + 1.0;

		if (randZeroOne() > GRASS_TYPE_UNIFORM_WATER)
		{// Randomize...
			iGrassType = randomInt(0, 3);
		}

		if (heightAboveWaterLength <= 192.0)
		{// When near water edge, reduce the size of the grass...
			sizeMult *= clamp(heightAboveWaterLength / 192.0, 0.0, 1.0) * fSizeRandomness;
		}
		else
		{// Deep underwater plants draw larger...
			sizeMult *= clamp(1.0 + (heightAboveWaterLength / 192.0), 1.0, 16.0);
		}

		sizeMult *= GRASS_SIZE_MULTIPLIER_UNDERWATER;

		tcOffsetBegin = BakedOffsetsBegin[iGrassType];
		tcOffsetEnd = tcOffsetBegin + 0.25;

		// Final value set to 1 == an underwater grass...
		iGrassType = 1;
	}
	else
	{
		iGrassType = randomInt(0, 2);

		vLocalSeed = round(vGrassFieldPos * GRASS_TYPE_UNIFORMALITY_SCALER) + 1.0;

		if (randZeroOne() > GRASS_TYPE_UNIFORMALITY)
		{// Randomize...
			iGrassType = randomInt(3, 15);
		}
		else if (GRASS_RARE_PATCHES_ONLY > 0.0)
		{// If only drawing rare grasses, skip adding anything when it's not a rare...
			return;
		}

		if (heightAboveWaterLength <= 256.0)
		{// When near water edge, reduce the size of the grass...
			sizeMult *= clamp(heightAboveWaterLength / 192.0, 0.0, 1.0) * fSizeRandomness;
		}

		if (iGrassType > 2 && iGrassType < 16)
		{// Rare randomized grasses (3 -> 16 - the plants) are a bit larger then the standard grass...
			//sizeMult *= 2.75;
			sizeMult *= GRASS_SIZE_MULTIPLIER_RARE;
		}
		else
		{
			sizeMult *= GRASS_SIZE_MULTIPLIER_COMMON;
		}

		tcOffsetBegin = BakedOffsetsBegin[iGrassType];
		tcOffsetEnd = tcOffsetBegin + 0.25;

		// Final value set to 0 == an above water grass...
		iGrassType = 0;
	}
#endif //defined(__USE_UNDERWATER_ONLY__)

	fGrassPatchHeight = clamp(controlMapScale * vertDistanceScale * 3.0, 0.0, 1.0);

	if (fGrassPatchHeight <= 0.05)
	{
		return;
	}

	fGrassPatchHeight = max(fGrassPatchHeight, 0.5);

	float fGrassFinalSize = GRASS_HEIGHT * sizeMult * fSizeRandomness * controlMap.a; // controlMap.a is road edges multiplier...

	if (fGrassFinalSize <= GRASS_HEIGHT * 0.05)
	{
		return;
	}

	float fWindPower = inWindPower[gl_InvocationID];
	float randDir = sin(randZeroOne()*0.7f)*0.1f;

#ifdef THREE_WAY_GRASS_CLUMPS
	for (int i = 0; i < 3; i++)
#else //!THREE_WAY_GRASS_CLUMPS
	for (int i = 0; i < 2; i++)
#endif //THREE_WAY_GRASS_CLUMPS
	{// Draw either 2 or 3 copies at each position at different angles...
		if (lodLevel == 3 && i > 0) continue;
		if (lodLevel == 2 && i > 1) continue;
		if (lodLevel == 1 && i > 2) continue;

		vec3 direction = (rotationMatrix(vec3(0, 1, 0), randDir)*vec4(vBaseDir[i], 1.0)).xyz;
		
		if (GRASS_WIDTH_REPEATS > 0.0) direction.xy *= GRASS_WIDTH_REPEATS;

		vec3 P = vGrassFieldPos.xyz;

		vec3 va = P - (direction * fGrassFinalSize);
		vec3 vb = P + (direction * fGrassFinalSize);
		vec3 vc = va + vec3(0.0, 0.0, fGrassFinalSize * fGrassPatchHeight);
		vec3 vd = vb + vec3(0.0, 0.0, fGrassFinalSize * fGrassPatchHeight);

		
		//vec3 baseNorm = normalize(cross(normalize(va - P), normalize(vb - P)));
		vec3 baseNorm = normalize(cross(normalize(vb - va), normalize(vc - va)));
		//vec3 I = normalize(P.xyz - u_ViewOrigin.xyz);
		vec3 I = normalize(vec3(normalize(P.xyz - u_ViewOrigin.xyz).xy, 0.0));
		//vec3 I = normalize(vec3(normalize(P.xyz).xy, 0.0));
		vec3 Nf = normalize(faceforward(baseNorm, I, baseNorm));
		
		
		//vec3 baseNorm = vec3(0.0, 0.0, 1.0);//-normalize(cross(normalize(-(direction * fGrassFinalSize) - (direction * fGrassFinalSize)), normalize(-vec3(0.0, 0.0, fGrassFinalSize * fGrassPatchHeight) - (direction * fGrassFinalSize))));
		//vec3 Nf = baseNorm;

		vVertNormal = EncodeNormal(Nf);

#ifndef __HUMANOIDS_BEND_GRASS__ // Just the player...
		vec3 playerOffset = vec3(0.0);
		float distanceToPlayer = distance(u_PlayerOrigin.xy, P.xy);
		float PLAYER_BEND_CLOSENESS = GRASS_HEIGHT * 1.5;
		if (distanceToPlayer < PLAYER_BEND_CLOSENESS)
		{
			vec3 dirToPlayer = normalize(P - u_PlayerOrigin);
			dirToPlayer.z = 0.0;
			playerOffset = dirToPlayer * (PLAYER_BEND_CLOSENESS - distanceToPlayer);
		}
#else //__HUMANOIDS_BEND_GRASS__ - Any player/NPC...
		int closestHumanoid = -1;
		float closestHumanoidDistance = distance(u_PlayerOrigin.xy, P.xy);

		for (int h = 0; h < u_HumanoidOriginsNum; h++)
		{
			float dist = distance(u_HumanoidOrigins[h].xy, P.xy);

			if (dist < closestHumanoidDistance)
			{
				closestHumanoid = h;
				closestHumanoidDistance = dist;
			}
		}

		vec3 playerOffset = vec3(0.0);
		float PLAYER_BEND_CLOSENESS;
		
		if (closestHumanoid == -1)
			PLAYER_BEND_CLOSENESS = GRASS_HEIGHT * 1.5;
		else
			PLAYER_BEND_CLOSENESS = GRASS_HEIGHT * 1.25;

		if (closestHumanoidDistance < PLAYER_BEND_CLOSENESS)
		{
			vec3 dirToPlayer;
			if (closestHumanoid == -1)
				dirToPlayer = normalize(P - u_PlayerOrigin);
			else
				dirToPlayer = normalize(P - u_HumanoidOrigins[closestHumanoid]);

			dirToPlayer.z = 0.0;
			playerOffset = dirToPlayer * (PLAYER_BEND_CLOSENESS - closestHumanoidDistance);
		}
#endif //__HUMANOIDS_BEND_GRASS__

		vVertPosition = va.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(tcOffsetBegin[0], tcOffsetEnd[1]);
		EmitVertex();

		vVertPosition = vb.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(tcOffsetEnd[0], tcOffsetEnd[1]);
		EmitVertex();

		vVertPosition = vc.xyz + playerOffset + vWindDirection*fWindPower;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(tcOffsetBegin[0], tcOffsetBegin[1]);
		EmitVertex();

		vVertPosition = vd.xyz + playerOffset + vWindDirection*fWindPower;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(tcOffsetEnd[0], tcOffsetBegin[1]);
		EmitVertex();

		EndPrimitive();
	}
}
