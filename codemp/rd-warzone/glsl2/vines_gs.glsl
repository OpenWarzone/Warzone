#define THREE_WAY_GRASS_CLUMPS // 3 way probably gives better coverage, at extra cost... otherwise 2 way X shape... 

#define MAX_FOLIAGES					85

layout(triangles) in;
//layout(triangles, invocations = 8) in;
layout(triangle_strip, max_vertices = MAX_FOLIAGES) out;


uniform mat4								u_ModelViewProjectionMatrix;

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
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D							u_RoadsControlMap;
uniform sampler2D							u_HeightMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4								u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4								u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4								u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4								u_Local8; // GRASS_SURFACE_MINIMUM_SIZE, GRASS_DISTANCE_FROM_ROADS, GRASS_HEIGHT, GRASS_SURFACE_SIZE_DIVIDER
uniform vec4								u_Local9; // testvalue0, 1, 2, 3
uniform vec4								u_Local10; // foliageLODdistance, TERRAIN_TESS_OFFSET, 0.0, GRASS_TYPE_UNIFORMALITY
uniform vec4								u_Local11; // GRASS_WIDTH_REPEATS, GRASS_MAX_SLOPE, GRASS_TYPE_UNIFORMALITY_SCALER, 0.0
uniform vec4								u_Local12; // GRASS_SIZE_MULTIPLIER_COMMON, GRASS_SIZE_MULTIPLIER_RARE, GRASS_SIZE_MULTIPLIER_UNDERWATER, GRASS_LOD_START_RANGE

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

#define GRASS_SIZE_MULTIPLIER_COMMON		u_Local12.r
#define GRASS_SIZE_MULTIPLIER_RARE			u_Local12.g
#define GRASS_SIZE_MULTIPLIER_UNDERWATER	u_Local12.b
#define GRASS_LOD_START_RANGE				u_Local12.a

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


const vec3							vWindDirection = normalize(vec3(1.0, 1.0, 0.0));


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

void main()
{
	iGrassType = 0;

	//
	//
	//

	//face center------------------------
	vec3 Vert1 = gl_in[0].gl_Position.xyz;
	vec3 Vert2 = gl_in[1].gl_Position.xyz;
	vec3 Vert3 = gl_in[2].gl_Position.xyz;

	vec3 vGrassFieldPos = (Vert1 + Vert2 + Vert3) / 3.0;   //Center of the triangle - copy for later
														   //-----------------------------------

	vLocalSeed = vGrassFieldPos;

	float VertDist2 = distance(u_ViewOrigin, vGrassFieldPos);

	if (VertDist2 >= MAX_RANGE)
	{// Too far from viewer... Cull...
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

	// Find the topmost 2 verts... We will draw the vines below these...
	vec3 topVerts[2];
	
	if (Vert1.z >= Vert2.z && Vert1.z >= Vert3.z)
	{
		topVerts[0] = Vert1;

		if (Vert2.z >= Vert3.z)
		{
			topVerts[1] = Vert2;
		}
		else
		{
			topVerts[1] = Vert3;
		}
	}
	else if (Vert2.z >= Vert1.z && Vert2.z >= Vert3.z)
	{
		topVerts[0] = Vert2;

		if (Vert1.z >= Vert3.z)
		{
			topVerts[1] = Vert1;
		}
		else
		{
			topVerts[1] = Vert3;
		}
	}
	else
	{
		topVerts[0] = Vert3;

		if (Vert1.z >= Vert2.z)
		{
			topVerts[1] = Vert1;
		}
		else
		{
			topVerts[1] = Vert2;
		}
	}

	float topVertsDistance = distance(topVerts[0], topVerts[1]);


	vec2 tcOffsetBegin;
	vec2 tcOffsetEnd;

	if (topVerts[0].z < MAP_WATER_LEVEL && topVerts[1].z < MAP_WATER_LEVEL)
	{
		return;
	}
	else
	{
		iGrassType = randomInt(0, 2);

		vLocalSeed = round(vGrassFieldPos * GRASS_TYPE_UNIFORMALITY_SCALER) + 1.0;

		if (randZeroOne() > GRASS_TYPE_UNIFORMALITY)
		{// Randomize...
			iGrassType = randomInt(3, 15);
		}

		tcOffsetBegin = BakedOffsetsBegin[iGrassType];
		tcOffsetEnd = tcOffsetBegin + 0.25;

		// Final value set to 0 == an above water grass...
		iGrassType = 0;
	}

	
	float fWindPower = inWindPower[gl_InvocationID];

	{// Draw either 2 or 3 copies at each position at different angles...
		vec3 P = vGrassFieldPos.xyz;

		vec3 va = topVerts[0];
		vec3 vb = topVerts[1];
		vec3 vc = topVerts[0] - vec3(0.0, 0.0, topVertsDistance * GRASS_HEIGHT);
		vec3 vd = topVerts[1] - vec3(0.0, 0.0, topVertsDistance * GRASS_HEIGHT);

		//vec3 baseNorm = normalize(cross(normalize(va - P), normalize(vb - P)));
		vec3 baseNorm = normalize(cross(normalize(vb - va), normalize(vc - va)));
		//vec3 I = normalize(P.xyz - u_ViewOrigin.xyz);
		vec3 I = normalize(vec3(normalize(P.xyz - u_ViewOrigin.xyz).xy, 0.0));
		vec3 Nf = normalize(faceforward(baseNorm, I, baseNorm));

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
