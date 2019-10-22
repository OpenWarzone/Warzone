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
uniform sampler2D					u_RoadsControlMap;
uniform sampler2D					u_HeightMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform mat4						u_ModelViewProjectionMatrix;
uniform mat4						u_ModelMatrix;

uniform vec4						u_Settings0; // useTC, useDeform, useRGBA, isTextureClamped
uniform vec4						u_Settings1; // useVertexAnim, useSkeletalAnim, useFog, is2D
uniform vec4						u_Settings2; // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
uniform vec4						u_Settings3; // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL

#define USE_TC						u_Settings0.r
#define USE_DEFORM					u_Settings0.g
#define USE_RGBA					u_Settings0.b
#define USE_TEXTURECLAMP			u_Settings0.a

#define USE_VERTEX_ANIM				u_Settings1.r
#define USE_SKELETAL_ANIM			u_Settings1.g
#define USE_FOG						u_Settings1.b
#define USE_IS2D					u_Settings1.a

#define USE_LIGHTMAP				u_Settings2.r
#define USE_GLOW_BUFFER				u_Settings2.g
#define USE_CUBEMAP					u_Settings2.b
#define USE_TRIPLANAR				u_Settings2.a

#define USE_REGIONS					u_Settings3.r
#define USE_ISDETAIL				u_Settings3.g

uniform vec4						u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4						u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4						u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4						u_Local4; // stageNum, glowStrength, r_showsplat, glowVibrancy
uniform vec4						u_Local8; // GRASS_DISTANCE_FROM_ROADS
uniform vec4						u_Local9; // testvalue0, 1, 2, 3

#define SHADER_HAS_SPLATMAP4		u_Local3.a
#define GRASS_DISTANCE_FROM_ROADS	u_Local8.r

uniform float	u_Time;

uniform vec4						u_MapInfo; // MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0
uniform vec4						u_Mins;
uniform vec4						u_Maxs;

out precise vec3 WorldPos_FS_in;
out precise vec2 TexCoord_FS_in;
out precise vec3 Normal_FS_in;
out precise vec3 ViewDir_FS_in;
out precise vec4 Color_FS_in;
out precise vec4 PrimaryLightDir_FS_in;
out precise vec2 TexCoord2_FS_in;
out precise vec3 Blending_FS_in;
/*flat*/ out float Slope_FS_in;
/*flat*/ out float GrassSlope_FS_in;
out float TessDepth_FS_in;

#define WorldPos_GS_in WorldPos_FS_in
#define TexCoord_GS_in TexCoord_FS_in
#define Normal_GS_in Normal_FS_in
#define ViewDir_GS_in ViewDir_FS_in
#define Color_GS_in Color_FS_in
#define PrimaryLightDir_GS_in PrimaryLightDir_FS_in
#define TexCoord2_GS_in TexCoord2_FS_in
#define Blending_GS_in Blending_FS_in
#define Slope_GS_in Slope_FS_in
#define GrassSlope_GS_in GrassSlope_FS_in

uniform vec4 u_TesselationInfo;

#define uTessAlpha u_TesselationInfo.r

//layout(quads, fractional_odd_spacing, ccw) in;
layout(triangles, fractional_odd_spacing, ccw) in; // Does rend2 spew out clockwise or counter-clockwise verts???
//layout(triangles, equal_spacing, ccw) in;

uniform vec3			u_ViewOrigin;

uniform int    u_DeformGen;
uniform float  u_DeformParams[7];

in precise vec4 WorldPos_ES_in[];
in precise vec3 iNormal[];
in precise vec2 iTexCoord[];
in precise vec4 Color_ES_in[];
in precise vec4 PrimaryLightDir_ES_in[];
in precise vec2 TexCoord2_ES_in[];
in precise vec3 Blending_ES_in[];
in float Slope_ES_in[];
in float GrassSlope_ES_in[];

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

float GetHeightmap(vec2 pixel)
{
	return texture(u_HeightMap, pixel).r;
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
	return offset * uTessAlpha;
}

void main()
{
	// compute texcoords
	WorldPos_GS_in = gl_TessCoord[2] * WorldPos_ES_in[0].xyz
		+ gl_TessCoord[0] * WorldPos_ES_in[1].xyz
		+ gl_TessCoord[1] * WorldPos_ES_in[2].xyz;
	TexCoord_GS_in = gl_TessCoord[2] * iTexCoord[0]
		+ gl_TessCoord[0] * iTexCoord[1]
		+ gl_TessCoord[1] * iTexCoord[2];
	Color_GS_in = gl_TessCoord[2] * Color_ES_in[0]
		+ gl_TessCoord[0] * Color_ES_in[1]
		+ gl_TessCoord[1] * Color_ES_in[2];
	PrimaryLightDir_GS_in = gl_TessCoord[2] * PrimaryLightDir_ES_in[0]
		+ gl_TessCoord[0] * PrimaryLightDir_ES_in[1]
		+ gl_TessCoord[1] * PrimaryLightDir_ES_in[2];
	TexCoord2_GS_in = gl_TessCoord[2] * TexCoord2_ES_in[0]
		+ gl_TessCoord[0] * TexCoord2_ES_in[1]
		+ gl_TessCoord[1] * TexCoord2_ES_in[2];
	Blending_GS_in = gl_TessCoord[2] * Blending_ES_in[0]
		+ gl_TessCoord[0] * Blending_ES_in[1]
		+ gl_TessCoord[1] * Blending_ES_in[2];
	/*Slope_GS_in = gl_TessCoord[2] * Slope_ES_in[0]
		+ gl_TessCoord[0] * Slope_ES_in[1]
		+ gl_TessCoord[1] * Slope_ES_in[2];*/
	Slope_GS_in = Slope_ES_in[0];
	GrassSlope_GS_in = GrassSlope_ES_in[0];

	TessDepth_FS_in = OffsetForPosition(WorldPos_GS_in);
	WorldPos_GS_in.z += TessDepth_FS_in;

	TessDepth_FS_in /= uTessAlpha; // want this to output between 0 and 1

	// normal
	/*
	vec3 barNormal = gl_TessCoord[2] * iNormal[0]
		+ gl_TessCoord[0] * iNormal[1]
		+ gl_TessCoord[1] * iNormal[2];
		*/

	//Normal_GS_in = uTessAlpha*pnNormal + (1.0 - uTessAlpha)*barNormal;
	Normal_GS_in = gl_TessCoord[2] * iNormal[0]
		+ gl_TessCoord[0] * iNormal[1]
		+ gl_TessCoord[1] * iNormal[2];

	/*
	// Adjust normals based on the changes... This sucks though... it flattens them too much...
	vec3 pos0 = WorldPos_ES_in[0].xyz;
	vec3 pos1 = WorldPos_ES_in[1].xyz;
	vec3 pos2 = WorldPos_ES_in[2].xyz;
	pos0 += OffsetForPosition(pos0);
	pos1 += OffsetForPosition(pos1);
	pos2 += OffsetForPosition(pos2);

	vec3 normal = normalize(cross(pos2 - pos0, pos1 - pos0));

	Normal_GS_in = normalize((Normal_GS_in + normal) / 2.0);
	*/

	// final position and normal
	//vec3 finalPos = (1.0 - uTessAlpha)*barPos + uTessAlpha*pnPos;
	/*vec3 finalPos = gl_TessCoord[2] * WorldPos_ES_in[0].xyz
		+ gl_TessCoord[0] * WorldPos_ES_in[1].xyz
		+ gl_TessCoord[1] * WorldPos_ES_in[2].xyz;*/

	vec3 finalPos = WorldPos_GS_in;

	gl_Position = u_ModelViewProjectionMatrix * vec4(finalPos, 1.0);

	//finalPos = (u_ModelMatrix * vec4(finalPos, 1.0)).xyz;
	//Normal_GS_in = (u_ModelMatrix * vec4(Normal_GS_in, 0.0)).xyz;

	//WorldPos_GS_in = finalPos.xyz;
	ViewDir_GS_in = u_ViewOrigin - finalPos;

	
}
