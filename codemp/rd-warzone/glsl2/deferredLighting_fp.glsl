﻿#define _PROCEDURALS_IN_DEFERRED_SHADER_
#define _AMBIENT_OCCLUSION_

#ifndef LQ_MODE
	#define _ENHANCED_AO_
	#define _SCREEN_SPACE_REFLECTIONS_
	#define _CLOUD_SHADOWS_
	//#define _SSDO_

	#ifdef _ENHANCED_AO_
		#define _ENABLE_GI_
	#endif //_ENHANCED_AO_

	#ifdef USE_CUBEMAPS
		#define _CUBEMAPS_
	#endif //USE_CUBEMAPS
#endif //LQ_MODE

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
uniform samplerCube					u_CubeMap;
uniform samplerCube					u_SkyCubeMap;
uniform samplerCube					u_SkyCubeMapNight;
uniform samplerCube					u_EmissiveCubeMap;
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
uniform sampler2D							u_DiffuseMap;		// Screen image
uniform sampler2D							u_NormalMap;		// Flat normals
uniform sampler2D							u_PositionMap;		// positionMap
uniform sampler2D							u_WaterPositionMap;	// random2K image
uniform sampler2D							u_RoadsControlMap;	// Screen Pshadows Map
uniform sampler2D							u_HeightMap;		// height map

#ifdef USE_REAL_NORMALMAPS
uniform sampler2D							u_OverlayMap;		// Real normals. Alpha channel 1.0 means enabled...
#endif //USE_REAL_NORMALMAPS

#ifdef _ENHANCED_AO_
uniform sampler2D							u_SteepMap1;			// ssao image
#endif //_ENHANCED_AO_

#ifdef _SCREEN_SPACE_REFLECTIONS_
uniform sampler2D							u_GlowMap;			// anamorphic
#endif //_SCREEN_SPACE_REFLECTIONS_

#ifdef USE_SHADOWMAP
uniform sampler2D							u_ShadowMap;		// Screen Shadow Map
#endif //USE_SHADOWMAP

#ifndef LQ_MODE
#ifdef _SSDO_
	uniform sampler2D							u_SplatControlMap;	// SSDO Map
	uniform sampler2D							u_SplatMap1;		// SSDO Illumination Map
#endif //_SSDO_
uniform sampler2D							u_WaterEdgeMap;		// tr.shinyImage
uniform samplerCube							u_SkyCubeMap;		// Day sky cubemap
uniform samplerCube							u_SkyCubeMapNight;	// Night sky cubemap
#endif //LQ_MODE

#ifdef _CUBEMAPS_
uniform samplerCube							u_CubeMap;			// Closest cubemap
#endif //_CUBEMAPS_

uniform sampler2D							u_ScreenDepthMap;	// 512 depth map.
uniform sampler2D							u_RoadMap;			// SSDM map.
#endif //defined(USE_BINDLESS_TEXTURES)

uniform int									u_lightCount;
uniform int									u_emissiveLightCount;


#ifdef USE_LIGHTS_SSBO
	struct Lights_t
	{
		vec4										u_lightPositions2;
		vec4										u_lightColors;
		vec4										u_coneDirection;
	};

	layout(std430, binding=2) buffer LightBlock
	{ 
		Lights_t lights[];
	};

	#ifdef _USE_MAP_EMMISSIVE_BLOCK_
		layout(std430, binding=3) buffer EmissiveLightBlock
		{ 
			Lights_t emissiveLights[];
		};
	#endif //_USE_MAP_EMMISSIVE_BLOCK_
#else //!USE_LIGHTS_SSBO
	uniform vec3								u_lightPositions2[MAX_DEFERRED_LIGHTS];
	uniform float								u_lightDistances[MAX_DEFERRED_LIGHTS];
	uniform vec3								u_lightColors[MAX_DEFERRED_LIGHTS];
	uniform float								u_lightMaxDistance;
#endif //USE_LIGHTS_SSBO

uniform mat4								u_ModelViewProjectionMatrix;

uniform vec2								u_Dimensions;

uniform vec4								u_Local1;	// SUN_PHONG_SCALE,				MAP_USE_PALETTE_ON_SKY,			r_ao,							GAMMA_CORRECTION
uniform vec4								u_Local2;	// MAP_REFLECTION_ENABLED,		SHADOWS_ENABLED,				SHADOW_MINBRIGHT,				SHADOW_MAXBRIGHT
uniform vec4								u_Local3;	// r_testShaderValue1,			r_testShaderValue2,				r_testShaderValue3,				r_testShaderValue4
uniform vec4								u_Local4;	// r_debugDrawEmissiveLights,	MAP_EMISSIVE_COLOR_SCALE,		MAP_HDR_MIN,					MAP_HDR_MAX
uniform vec4								u_Local5;	// CONTRAST,					SATURATION,						BRIGHTNESS,						WETNESS
uniform vec4								u_Local6;	// AO_MINBRIGHT,				AO_MULTBRIGHT,					VIBRANCY,						TRUEHDR_ENABLED
uniform vec4								u_Local7;	// cubemapEnabled,				r_cubemapCullRange,				PROCEDURAL_SKY_ENABLED,			r_skyLightContribution
uniform vec4								u_Local8;	// NIGHT_SCALE,					PROCEDURAL_CLOUDS_CLOUDCOVER,	PROCEDURAL_CLOUDS_CLOUDSCALE,	CLOUDS_SHADOWS_ENABLED
uniform vec4								u_Local11;	// DISPLACEMENT_MAPPING_STRENGTH, COLOR_GRADING_ENABLED,	USE_SSDO,						HAVE_HEIGHTMAP
uniform vec4								u_Local12;	// r_testShaderValue5,			r_testShaderValue6,				r_testShaderValue7,				r_testShaderValue8

#ifdef _PROCEDURALS_IN_DEFERRED_SHADER_
uniform vec4								u_Local9;	// MAP_INFO_PLAYABLE_HEIGHT, PROCEDURAL_MOSS_ENABLED, PROCEDURAL_SNOW_ENABLED, PROCEDURAL_SNOW_ROCK_ONLY
uniform vec4								u_Local10;	// PROCEDURAL_SNOW_LUMINOSITY_CURVE, PROCEDURAL_SNOW_BRIGHTNESS, PROCEDURAL_SNOW_HEIGHT_CURVE, PROCEDURAL_SNOW_LOWEST_ELEVATION
#endif //_PROCEDURALS_IN_DEFERRED_SHADER_

uniform vec3								u_ViewOrigin;
uniform vec4								u_PrimaryLightOrigin;
uniform vec3								u_PrimaryLightColor;

uniform float								u_Time;

#ifdef _CUBEMAPS_
uniform vec4								u_CubeMapInfo;
uniform float								u_CubeMapStrength;
#endif //_CUBEMAPS_

uniform float								u_MaterialSpeculars[MATERIAL_LAST];
uniform float								u_MaterialReflectiveness[MATERIAL_LAST];

uniform vec4								u_Mins; // mins, mins, mins, WATER_ENABLED
uniform vec4								u_Maxs;

varying vec2								var_TexCoords;
varying float								var_CloudShadow;


#define SUN_PHONG_SCALE						u_Local1.r
#define MAP_USE_PALETTE_ON_SKY				u_Local1.g
#define AO_TYPE								u_Local1.b
#define GAMMA_CORRECTION					u_Local1.a

#define REFLECTIONS_ENABLED					u_Local2.r
#define SHADOWS_ENABLED						u_Local2.g
#define SHADOW_MINBRIGHT					u_Local2.b
#define SHADOW_MAXBRIGHT					u_Local2.a

#define DEBUG_EMISSIVE_LIGHTS				u_Local4.r
#define PROCEDURAL_SKY_ENABLED				u_Local4.g
#define MAP_HDR_MIN							u_Local4.b
#define MAP_HDR_MAX							u_Local4.a

#define WETNESS								u_Local5.r
#define CONTRAST_STRENGTH					u_Local5.g
#define SATURATION_STRENGTH					u_Local5.b
#define BRIGHTNESS_STRENGTH					u_Local5.a

#define AO_MINBRIGHT						u_Local6.r
#define AO_MULTBRIGHT						u_Local6.g
#define VIBRANCY							u_Local6.b
#define TRUEHDR_ENABLED						u_Local6.a

#define MAP_EMISSIVE_COLOR_SCALE			u_Local7.r
#define CUBEMAP_ENABLED						u_Local7.g
#define CUBEMAP_CULLRANGE					u_Local7.b
#define SKY_LIGHT_CONTRIBUTION				u_Local7.a

// CLOUDS
#define NIGHT_SCALE							u_Local8.r
#define CLOUDS_CLOUDCOVER					u_Local8.g
#define CLOUDS_CLOUDSCALE					u_Local8.b
#define CLOUDS_SHADOWS_ENABLED				u_Local8.a

#ifdef _PROCEDURALS_IN_DEFERRED_SHADER_
#define MAP_INFO_PLAYABLE_HEIGHT			u_Local9.r
#define PROCEDURAL_MOSS_ENABLED				u_Local9.g
#define PROCEDURAL_SNOW_ENABLED				u_Local9.b
#define PROCEDURAL_SNOW_ROCK_ONLY			u_Local9.a

#define PROCEDURAL_SNOW_LUMINOSITY_CURVE	u_Local10.r
#define PROCEDURAL_SNOW_BRIGHTNESS			u_Local10.g
#define PROCEDURAL_SNOW_HEIGHT_CURVE		u_Local10.b
#define PROCEDURAL_SNOW_LOWEST_ELEVATION	u_Local10.a
#endif //_PROCEDURALS_IN_DEFERRED_SHADER_

#define DISPLACEMENT_STRENGTH				u_Local11.r
#define COLOR_GRADING_ENABLED				u_Local11.g
#define USE_SSDO							u_Local11.b
#define HAVE_HEIGHTMAP						u_Local11.a

#define WATER_ENABLED						u_Mins.a

vec2 pixel = vec2(1.0) / u_Dimensions;

#define hdr_const_1 (MAP_HDR_MIN / 255.0)
#define hdr_const_2 (255.0 / MAP_HDR_MAX)

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



vec4 positionMapAtCoord ( vec2 coord )
{
	return textureLod(u_PositionMap, coord, 0.0);
}

vec2 GetMapTC(vec3 pos)
{
	vec2 mapSize = u_Maxs.xy - u_Mins.xy;
	return (pos.xy - u_Mins.xy) / mapSize;
}

vec2 RB_PBR_DefaultsForMaterial(float MATERIAL_TYPE)
{// I probably should use an array of const's instead, but this will do for now...
	vec2 settings = vec2(0.0);

	float specularReflectionScale = 0.0;
	float cubeReflectionScale = 0.0;

	switch (int(MATERIAL_TYPE))
	{
	case MATERIAL_WATER:
		specularReflectionScale = 0.1;
		cubeReflectionScale = 0.775;
		break;
	case MATERIAL_SHORTGRASS:
		specularReflectionScale = 0.0055;
		cubeReflectionScale = 0.5125;
		break;
	case MATERIAL_LONGGRASS:
		specularReflectionScale = 0.0065;
		cubeReflectionScale = 0.5125;
		break;
	case MATERIAL_SAND:
		specularReflectionScale = 0.0055;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_CARPET:
		specularReflectionScale = 0.0015;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_GRAVEL:
		specularReflectionScale = 0.0015;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_ROCK:
	case MATERIAL_STONE:
		specularReflectionScale = 0.002;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_TILES:
		specularReflectionScale = 0.026;
		cubeReflectionScale = 0.3625;
		break;
	case MATERIAL_SOLIDWOOD:
	case MATERIAL_TREEBARK:
		specularReflectionScale = 0.0015;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_HOLLOWWOOD:
		specularReflectionScale = 0.00075;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_POLISHEDWOOD:
		specularReflectionScale = 0.026;
		cubeReflectionScale = 0.5125;
		break;
	case MATERIAL_SOLIDMETAL:
		specularReflectionScale = 0.098;
		cubeReflectionScale = 0.985;
		break;
	case MATERIAL_HOLLOWMETAL:
		specularReflectionScale = 0.098;
		cubeReflectionScale = 0.985;
		break;
	case MATERIAL_DRYLEAVES:
		specularReflectionScale = 0.0026;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_GREENLEAVES:
		specularReflectionScale = 0.0055;
		cubeReflectionScale = 0.5125;
		break;
	case MATERIAL_PROCEDURALFOLIAGE:
		specularReflectionScale = 0.0055;
		cubeReflectionScale = 0.5125;
		break;
	case MATERIAL_BIRD:
	case MATERIAL_FABRIC:
		specularReflectionScale = 0.0055;
		cubeReflectionScale = 0.5125;
		break;
	case MATERIAL_CANVAS:
		specularReflectionScale = 0.0045;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_MARBLE:
		specularReflectionScale = 0.025;
		cubeReflectionScale = 0.595;
		break;
	case MATERIAL_SNOW:
		specularReflectionScale = 0.025;
		cubeReflectionScale = 0.7375;
		break;
	case MATERIAL_MUD:
		specularReflectionScale = 0.003;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_DIRT:
		specularReflectionScale = 0.002;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_CONCRETE:
		specularReflectionScale = 0.00175;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_FLESH:
		specularReflectionScale = 0.0045;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_RUBBER:
		specularReflectionScale = 0.0015;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_PLASTIC:
		specularReflectionScale = 0.028;
		cubeReflectionScale = 0.7375;
		break;
	case MATERIAL_PLASTER:
		specularReflectionScale = 0.0025;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_SHATTERGLASS:
		specularReflectionScale = 0.025;
		cubeReflectionScale = 0.7525;
		break;
	case MATERIAL_ARMOR:
		specularReflectionScale = 0.055;
		cubeReflectionScale = 0.745;
		break;
	case MATERIAL_ICE:
		specularReflectionScale = 0.045;
		cubeReflectionScale = 0.835;
		break;
	case MATERIAL_GLASS:
	case MATERIAL_DISTORTEDGLASS:
	case MATERIAL_DISTORTEDPUSH:
	case MATERIAL_DISTORTEDPULL:
	case MATERIAL_CLOAK:
	case MATERIAL_FORCEFIELD:
		specularReflectionScale = 0.035;
		cubeReflectionScale = 0.775;
		break;
	case MATERIAL_BPGLASS:
		specularReflectionScale = 0.033;
		cubeReflectionScale = 0.775;
		break;
	case MATERIAL_COMPUTER:
		specularReflectionScale = 0.042;
		cubeReflectionScale = 0.76;
		break;
	case MATERIAL_PUDDLE:
		specularReflectionScale = 0.098;
		cubeReflectionScale = 0.3235;
		break;
	case MATERIAL_LAVA:
		specularReflectionScale = 0.002;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_EFX:
	case MATERIAL_BLASTERBOLT:
	case MATERIAL_FIRE:
	case MATERIAL_SMOKE:
	case MATERIAL_MAGIC_PARTICLES:
	case MATERIAL_MAGIC_PARTICLES_TREE:
	case MATERIAL_FIREFLIES:
	case MATERIAL_PORTAL:
	case MATERIAL_MENU_BACKGROUND:
		specularReflectionScale = 0.0;
		cubeReflectionScale = 0.0;
		break;
	case MATERIAL_SKYSCRAPER:
		specularReflectionScale = 0.055;
		cubeReflectionScale = 0.745;
		break;
	default:
		specularReflectionScale = 0.0075;
		cubeReflectionScale = 0.4;
		break;
	}

	if (int(MATERIAL_TYPE) < MATERIAL_LAST)
	{// Check for game specified overrides...
		if (u_MaterialSpeculars[int(MATERIAL_TYPE)] != 0.0)
		{
			specularReflectionScale = u_MaterialSpeculars[int(MATERIAL_TYPE)];
		}

		if (u_MaterialReflectiveness[int(MATERIAL_TYPE)] != 0.0)
		{
			cubeReflectionScale = u_MaterialReflectiveness[int(MATERIAL_TYPE)];
		}
	}

	settings.x = specularReflectionScale;
	settings.y = cubeReflectionScale;

	return settings;
}

#ifdef _USE_PROCEDURAL_NOISE_
float proceduralHash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float proceduralNoise(in vec3 o) 
{
	vec3 p = floor(o);
	vec3 fr = fract(o);
		
	float n = p.x + p.y*57.0 + p.z * 1009.0;

	float a = proceduralHash(n+  0.0);
	float b = proceduralHash(n+  1.0);
	float c = proceduralHash(n+ 57.0);
	float d = proceduralHash(n+ 58.0);
	
	float e = proceduralHash(n+  0.0 + 1009.0);
	float f = proceduralHash(n+  1.0 + 1009.0);
	float g = proceduralHash(n+ 57.0 + 1009.0);
	float h = proceduralHash(n+ 58.0 + 1009.0);
	
	
	vec3 fr2 = fr * fr;
	vec3 fr3 = fr2 * fr;
	
	vec3 t = 3.0 * fr2 - 2.0 * fr3;
	
	float u = t.x;
	float v = t.y;
	float w = t.z;

	// this last bit should be refactored to the same form as the rest :)
	float res1 = a + (b-a)*u +(c-a)*v + (a-b+d-c)*u*v;
	float res2 = e + (f-e)*u +(g-e)*v + (e-f+h-g)*u*v;
	
	float res = res1 * (1.0- w) + res2 * (w);
	
	return res;
}
#else //!_USE_PROCEDURAL_NOISE_
const float rpx = 1.0 / 2048.0;

float proceduralHash( const in float n ) {
	return texture(u_WaterPositionMap, vec2(n, 0.0) * rpx).r;
}

float proceduralNoise(in vec3 o) 
{
	return texture(u_WaterPositionMap, o.xy+o.z * rpx).r;
}
#endif //_USE_PROCEDURAL_NOISE_



#ifdef _PROCEDURALS_IN_DEFERRED_SHADER_

const mat3 proceduralMat = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

float proceduralSmoothNoise( vec3 p )
{
    float f;
    f  = 0.5000*proceduralNoise( p ); p = proceduralMat*p*2.02;
    f += 0.2500*proceduralNoise( p ); 
	
    return clamp(f * 1.3333333333333333333333333333333, 0.0, 1.0);
	//return proceduralNoise(p);
}

vec3 splatblend(vec3 color1, float a1, vec3 color2, float a2)
{
    float depth = 0.2;
	float ma = max(a1, a2) - depth;

    float b1 = max(a1 - ma, 0);
    float b2 = max(a2 - ma, 0);

    return ((color1.rgb * b1) + (color2.rgb * b2)) / (b1 + b2);
}

//
// Procedural texturing variation...
//
void AddProceduralMoss(inout vec4 outColor, in vec4 position)
{
	vec3 usePos = position.xyz;
	float moss = clamp(proceduralNoise( usePos.xyz * 0.00125 ), 0.0, 1.0);

	if (moss > 0.25)
	{
		const vec3 colorLight = vec3(0.0, 0.75, 0.0);
		const vec3 colorDark = vec3(0.0, 0.0, 0.0);
			
		float mossClr = proceduralSmoothNoise(usePos.xyz + moss * 0.25);
		vec3 mossColor = mix(colorDark, colorLight, mossClr*0.25);

		moss = pow((moss - 0.25) * 3.0, 0.35);
		outColor.rgb = splatblend(outColor.rgb, 1.0 - moss*0.65, mossColor, moss*0.25);
	}
}
#endif //_PROCEDURALS_IN_DEFERRED_SHADER_

#if defined(_SCREEN_SPACE_REFLECTIONS_)
#define pw pixel.x
#define ph pixel.y
vec3 AddReflection(vec2 coord, vec4 positionMap, vec3 flatNorm, vec3 inColor, float reflectiveness)
{
	if (reflectiveness <= 0.0)
	{// Top of screen pixel is water, don't check...
		return inColor;
	}

	float pixelDistance = distance(positionMap.xyz, u_ViewOrigin.xyz);

	//const float scanSpeed = 48.0;// 16.0;// 5.0; // How many pixels to scan by on the 1st rough pass...
	const float scanSpeed = 16.0;

	// Quick scan for pixel that is not water...
	float QLAND_Y = 0.0;
	float topY = min(coord.y + 0.6, 1.0); // Don'y scan further then this, waste of time as it will be bleneded out...

	for (float y = coord.y; y <= topY; y += ph * scanSpeed)
	{
		vec3 norm = DecodeNormal(textureLod(u_NormalMap, vec2(coord.x, y), 0.0).xy);
		vec4 pMap = positionMapAtCoord(vec2(coord.x, y));

		float pMapDistance = distance(pMap.xyz, u_ViewOrigin.xyz);

		if (pMap.a > 1.0 && pMap.xyz != vec3(0.0) && distance(pMap.xyz, u_ViewOrigin.xyz) <= pixelDistance)
		{
			continue;
		}

		if (norm.z * 0.5 + 0.5 < 0.75 && distance(norm.xyz, flatNorm.xyz) > 0.0)
		{
			QLAND_Y = y;
			break;
		}
		else if (positionMap.a != pMap.a && pMap.a == 0.0)
		{
			QLAND_Y = y;
			break;
		}
	}

	if (QLAND_Y <= 0.0 || QLAND_Y >= 1.0)
	{// Found no non-water surfaces...
		return inColor;
	}
	
	QLAND_Y -= ph * scanSpeed;
	
	// Full scan from within 5 px for the real 1st pixel...
	float upPos = coord.y;
	float LAND_Y = 0.0;

	for (float y = QLAND_Y; y <= topY && y <= QLAND_Y + (ph * scanSpeed); y += ph * 2.0)
	{
		vec3 norm = DecodeNormal(textureLod(u_NormalMap, vec2(coord.x, y), 0.0).xy);
		vec4 pMap = positionMapAtCoord(vec2(coord.x, y));
		
		float pMapDistance = distance(pMap.xyz, u_ViewOrigin.xyz);

		if (pMap.a > 1.0 && pMap.xyz != vec3(0.0) && distance(pMap.xyz, u_ViewOrigin.xyz) <= pixelDistance)
		{
			continue;
		}

		if (norm.z * 0.5 + 0.5 < 0.75 && distance(norm.xyz, flatNorm.xyz) > 0.0)
		{
			LAND_Y = y;
			break;
		}
		else if (positionMap.a != pMap.a && pMap.a == 0.0)
		{
			LAND_Y = y;
			break;
		}
	}

	if (LAND_Y <= 0.0 || LAND_Y >= 1.0)
	{// Found no non-water surfaces...
		return inColor;
	}

	float d = 1.0 / ((1.0 - (LAND_Y - coord.y)) * 1.75);

	upPos = clamp(coord.y + ((LAND_Y - coord.y) * 2.0 * d), 0.0, 1.0);

	if (upPos > 1.0 || upPos < 0.0)
	{// Not on screen...
		return inColor;
	}

	vec4 pMap = positionMapAtCoord(vec2(coord.x, upPos));

	if (pMap.a > 1.0 && pMap.xyz != vec3(0.0) && distance(pMap.xyz, u_ViewOrigin.xyz) <= pixelDistance)
	{// The reflected pixel is closer then the original, this would be a bad reflection.
		return inColor;
	}

	float heightDiff = clamp(distance(upPos, coord.y) * 3.0, 0.0, 1.0);
	float heightStrength = 1.0 - clamp(pow(heightDiff, 0.025), 0.0, 1.0);

	float strength = 1.0 - clamp(pow(upPos, 4.0), 0.0, 1.0);
	strength *= heightStrength;
	strength = clamp(strength, 0.0, 1.0);

	if (strength <= 0.0)
	{
		return inColor;
	}

	// Offset the final pixel based on the height of the wave at that point, to create randomization...
	float hoff = proceduralHash(upPos+(u_Time*0.0004)) * 2.0 - 1.0;
	float offset = hoff * pw;

	vec3 glowColor = textureLod(u_GlowMap, vec2(coord.x + offset, 1.0-upPos), 0.0).rgb;
	vec3 landColor = textureLod(u_DiffuseMap, vec2(coord.x + offset, upPos), 0.0).rgb;
	return mix(inColor.rgb, inColor.rgb + landColor.rgb + (glowColor.rgb * /*2.0*/4.0 * reflectiveness), clamp(strength * reflectiveness * 4.0, 0.0, 1.0));
}
#endif //defined(_SCREEN_SPACE_REFLECTIONS_)

vec4 normalVector(vec3 color) {
	vec4 normals = vec4(color.rgb, length(color.rgb) / 3.0);
	normals.rgb = vec3(length(normals.r - normals.a), length(normals.g - normals.a), length(normals.b - normals.a));

	// Contrast...
#define normLower ( 32.0 / 255.0 )
#define normUpper (255.0 / 212.0 )
	vec3 N = clamp((clamp(normals.rgb - normLower, 0.0, 1.0)) * normUpper, 0.0, 1.0);

	return vec4(vec3(1.0) - (normalize(pow(N, vec3(4.0))) * 0.5 + 0.5), 1.0 - normals.a);
}

#if defined(_AMBIENT_OCCLUSION_)
float drawObject(in vec3 p){
    p = abs(fract(p)-.5);
    return dot(p, vec3(.5));
}

float cellTile(in vec3 p)
{
    p /= 5.5;

    vec4 v, d; 
    d.x = drawObject(p - vec3(.81, .62, .53));
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.y = drawObject(p - vec3(.39, .2, .11));
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawObject(p - vec3(.62, .24, .06));
    p.xz = vec2(p.z-p.x, p.z + p.x)*.7071;
    d.w = drawObject(p - vec3(.2, .82, .64));

    v.xy = min(d.xz, d.yw), v.z = min(max(d.x, d.y), max(d.z, d.w)), v.w = max(v.x, v.y); 
   
    d.x =  min(v.z, v.w) - min(v.x, v.y);

    return d.x*2.66;
}

float aomap(vec3 p)
{
    float n = (.5-cellTile(p))*1.5;
    return p.y + dot(sin(p/2. + cos(p.yzx/2. + 3.14159/2.)), vec3(.5)) + n;
}

float calculateAO(in vec3 pos, in vec3 nor, in vec2 texCoords)
{
	float sca = 0.00013/*2.0*/, occ = 0.0;

	for( int i=0; i<5; i++ ) {
		float hr = 0.01 + float(i)*0.5/4.0;        
		float dd = aomap(nor * hr + pos);
		occ += (hr - dd)*sca;
		sca *= 0.7;
	}

	return clamp( 1.0 - occ, 0.0, 1.0 );    
}
#endif //defined(_AMBIENT_OCCLUSION_)


vec3 TrueHDR ( vec3 color )
{
	return clamp((clamp(color.rgb - hdr_const_1, 0.0, 1.0)) * hdr_const_2, 0.0, 1.0);
}

vec3 Vibrancy ( vec3 origcolor, float vibrancyStrength )
{
	vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);
	float	max_color = max(origcolor.r, max(origcolor.g,origcolor.b));
	float	min_color = min(origcolor.r, min(origcolor.g,origcolor.b));
	float	color_saturation = max_color - min_color;
	float	luma = dot(lumCoeff, origcolor.rgb);
	return mix(vec3(luma), origcolor.rgb, (1.0 + (vibrancyStrength * (1.0 - (sign(vibrancyStrength) * color_saturation)))));
}


//
// Lighting...
//
float getspecularLight(vec3 surfaceNormal, vec3 lightDirection, vec3 viewDirection, float shininess)
{
	//Calculate Blinn-Phong power
	vec3 H = normalize(viewDirection + lightDirection);
	return clamp(pow(max(0.0, dot(surfaceNormal, H)), shininess), 0.0, 1.0);
}

float wetSpecular(vec3 n,vec3 l,vec3 e,float s) {
    float nrm = (s + 8.0) / (3.1415 * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

float getSpecialSauce(vec3 color)
{
	// Secret sauce diffuse adaption.
	//float lum = clamp(length(clamp(color.rgb, 0.0, 1.0))/3.0, 0.0, 1.0);
	float lum = clamp(max(max(color.r, max(color.g, color.b)) * 0.5, length(color) / 3.0), 0.0, 1.0);
	lum = clamp(clamp(pow(lum, 3.5), 0.0, 1.0) * 512.0, 2.0, 4.0);
	return lum;
}

float orenNayar(in vec3 n, in vec3 v, in vec3 ldir, float specPower)
{
	float r2 = pow(specPower, 2.0);
	float a = 1.0 - 0.5*(r2 / (r2 + 0.57));
	float b = 0.45*(r2 / (r2 + 0.09));

	float nl = dot(n, ldir);
	float nv = dot(n, v);

	float ga = dot(v - n * nv, n - n * nl);

	return max(0.0, nl) * (a + b * max(0.0, ga) * sqrt((1.0 - nv * nv)*(1.0 - nl * nl)) / max(nl, nv));
}

vec3 Lighting(vec4 pos, vec3 color, vec3 bump, vec3 flatNormal, vec3 view, vec3 light, vec3 lightColor, float specPower, vec3 lightPos, float wetness) {
	float ss = getSpecialSauce(color);

	// Ambient light.
	float directAmbience = 0.14;
	float enhancedAmbience = 0.24;

	// Nayar diffuse light.
	float base = orenNayar(bump, view, light, specPower) * ss;

	// Specular light.
	vec3 reflection = normalize(reflect(-view, bump));
	float specular = clamp(pow(clamp(dot(light, reflection), 0.0, 1.0), 15.0), 0.0, 1.0) * 0.25;

	float wetSpec = 0.0;

	if (wetness > 0.0)
	{// Increase specular strength when the ground is wet during/after rain...
		wetSpec = wetSpecular(bump, -light, view, 256.0) * wetness * 0.5;
		wetSpec += wetSpecular(bump, -light, view, 4.0) * wetness * 8.0;
		wetSpec *= 0.5;
	}

	float selfShadow = max(dot(bump, -light), 0.0);

	return (lightColor * directAmbience) + (lightColor * enhancedAmbience * ss) + (base * lightColor) + (lightColor * specular * ss) + (lightColor * wetSpec) * selfShadow;
}

void GetSSBOLighting(bool emissive, float origColorStrength, float lightsReflectionFactor, float specularReflectivePower, vec4 position, vec3 lightingNormal, vec3 flatNormal, vec3 E, float wetness, bool useOcclusion, vec4 occlusion, float PshadowValue, inout vec4 outColor)
{
	float pixelDistance = distance(position.xyz, u_ViewOrigin.xyz);

	if (pixelDistance > MAX_DEFERRED_LIGHT_RANGE)
	{
		return;
	}

	float pixelDistanceMult = 1.0 - (pixelDistance / MAX_DEFERRED_LIGHT_RANGE);

	int lightCount = u_lightCount;
	if (emissive) lightCount = u_emissiveLightCount;
	
	if (lightCount > 0.0 && pixelDistanceMult > 0.0)
	{
		vec3 addedLight = vec3(0.0);
		float maxBright = clamp(max(outColor.r, max(outColor.g, outColor.b)), 0.0, 1.0);
		float power = maxBright * 0.85;
		power = clamp(pow(power, 4.0) + 0.5, 0.0, 1.0);

		if (power > 0.0)
		{
			// No pointers in GLSL, at least on AMD, what a bunch of crap...
			for (int li = 0; li < lightCount; li++)
			{
				// WTB: Pointers...
				Lights_t light;
				
				if (emissive)
				{
					light = emissiveLights[li];
				}
				else
				{
					light = lights[li];
				}

				vec3 lightPos = light.u_lightPositions2.xyz;
				float lightDist = distance(lightPos, position.xyz);

				if (lightDist > light.u_lightPositions2.w)
				{
					continue;
				}

				float lightPlayerDist = distance(lightPos.xyz, u_ViewOrigin.xyz);

				if (lightPlayerDist > MAX_DEFERRED_LIGHT_RANGE)
				{
					continue;
				}

				vec3 lightDir = normalize(lightPos - position.xyz);

				if (light.u_coneDirection.a > 0.0)
				{
					vec3 coneDir = -normalize(light.u_coneDirection.xyz * 2.0 - 1.0);

					float lightToSurfaceAngle = degrees(acos(dot(lightDir, coneDir)));
					
					if (lightToSurfaceAngle > light.u_coneDirection.a)
					{// Outside of this light's cone...
						continue;
					}

					/*if (u_Local3.r == 3.0)
					{
						addedLight.rgb += vec3((1.0 - (lightToSurfaceAngle / light.u_coneDirection.a)) * 0.1);
						continue;
					}*/

					lightDir = coneDir;
				}

				float lightDistMult = 1.0 - clamp((lightPlayerDist / MAX_DEFERRED_LIGHT_RANGE), 0.0, 1.0);
				lightDistMult = pow(lightDistMult, 2.0) * pixelDistanceMult;

				if (lightDistMult > 0.0)
				{
					// Attenuation...
					float lightFade = 1.0 - clamp((lightDist * lightDist) / (light.u_lightPositions2.w * light.u_lightPositions2.w), 0.0, 1.0);
					lightFade = pow(lightFade, 2.0);

					if (emissive)
					{// Fade out the most distant lights to stop "popping"...
						float maxLightsFade = pow(1.0 - (float(li) / float(MAX_CONCURRENT_EMISSIVE_DRAW_LIGHTS)), 1.25);
						lightFade *= maxLightsFade * lightDistMult;
					}

					if (lightFade > 0.0)
					{
						float lightStrength = lightDistMult * lightFade * specularReflectivePower * 0.5;

						if (lightStrength > 0.0)
						{
							if (emissive)
							{// Move the light a little in the ray direction...
								lightPos = lightPos + (lightDir * -4.0);

								if (DEBUG_EMISSIVE_LIGHTS > 0.0)
								{
									float dbg = distance(normalize(u_ViewOrigin.xyz - lightPos.xyz), E);
									if (dbg <= 0.01 && distance(lightPos.xyz, u_ViewOrigin.xyz) <= distance(position.xyz, u_ViewOrigin.xyz))
									{
										float alpha = (1.0 - pow(dbg * (1.0 / 0.01), 3.0));
										outColor.rgb = mix(outColor.rgb, vec3(0.0, 0.0, 1.0) * alpha, alpha);
										continue;
									}
								}
							}

							vec3 lightColor = light.u_lightColors.rgb * power * origColorStrength;
							float lightSpecularPower = mix(0.1, 0.5, clamp(lightsReflectionFactor, 0.0, 1.0)) * clamp(lightStrength * 12.0, 0.0, 1.0);
							float light_occlusion = 1.0;

#ifndef LQ_MODE
							if (useOcclusion)
							{
								light_occlusion = clamp(1.0 - clamp(dot(vec4(lightDir, 1.0), occlusion), 0.0, 1.0) * 0.4 + 0.6, 0.0, 1.0);
							}
#endif //LQ_MODE

							float selfShadow = clamp(pow(clamp(dot(-lightDir.rgb, lightingNormal.rgb), 0.0, 1.0), 8.0) * 0.6 + 0.6, 0.0, 1.0);

							addedLight.rgb = max(addedLight.rgb, Lighting(position, outColor.rgb, lightingNormal, flatNormal, E, lightDir, lightColor, lightSpecularPower, lightPos, wetness) * lightFade * selfShadow * light_occlusion);
						}
					}
				}
			}

			addedLight.rgb *= lightsReflectionFactor; // More grey colors get more colorization from ..
			outColor.rgb = outColor.rgb + max(addedLight * PshadowValue * MAP_EMISSIVE_COLOR_SCALE, vec3(0.0));
		}
	}
}

#ifdef _CLOUD_SHADOWS_

#define RAY_TRACE_STEPS 2

//float gTime;
float cloudy = 0.0;
float cloudShadeFactor = 0.6;

#define CLOUD_LOWER 2800.0
#define CLOUD_UPPER 6800.0

#define MOD2 vec2(.16632,.17369)
#define MOD3 vec3(.16532,.17369,.15787)


//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
float Hash( float p )
{
	vec2 p2 = fract(vec2(p) * MOD2);
	p2 += dot(p2.yx, p2.xy+19.19);
	return fract(p2.x * p2.y);
}
float Hash(vec3 p)
{
	p  = fract(p * MOD3);
	p += dot(p.xyz, p.yzx + 19.19);
	return fract(p.x * p.y * p.z);
}

//--------------------------------------------------------------------------

float Noise( in vec2 x )
{
	vec2 p = floor(x);
	vec2 f = fract(x);
	f = f*f*(3.0-2.0*f);
	float n = p.x + p.y*57.0;
	float res = mix(mix( Hash(n+  0.0), Hash(n+  1.0),f.x),
					mix( Hash(n+ 57.0), Hash(n+ 58.0),f.x),f.y);
	return res;
}
float Noise(in vec3 p)
{
    vec3 i = floor(p);
	vec3 f = fract(p); 
	f *= f * (3.0-2.0*f);

	return mix(
		mix(mix(Hash(i + vec3(0.,0.,0.)), Hash(i + vec3(1.,0.,0.)),f.x),
			mix(Hash(i + vec3(0.,1.,0.)), Hash(i + vec3(1.,1.,0.)),f.x),
			f.y),
		mix(mix(Hash(i + vec3(0.,0.,1.)), Hash(i + vec3(1.,0.,1.)),f.x),
			mix(Hash(i + vec3(0.,1.,1.)), Hash(i + vec3(1.,1.,1.)),f.x),
			f.y),
		f.z);
}


const mat3 cm = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 ) * 1.7;
//--------------------------------------------------------------------------
float FBM( vec3 p )
{
	p *= .0005;
	p *= CLOUDS_CLOUDSCALE;

	float f;

	f = 0.5000 * Noise(p); p = cm*p;
	f += 0.2500 * Noise(p); p = cm*p;
	f += 0.1250 * Noise(p); p = cm*p;
	f += 0.0625   * Noise(p); p = cm*p;

	return f;
}

//--------------------------------------------------------------------------

float Map(vec3 p)
{
	float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-cloudShadeFactor);
	return h;
}

//--------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
float GetCloudAlpha(in vec3 pos,in vec3 rd, out vec2 outPos)
{
	float beg = ((CLOUD_LOWER-pos.y) / rd.y);
	float end = ((CLOUD_UPPER-pos.y) / rd.y);
	
	vec3 p = vec3(pos.x + rd.x * beg, 0.0, pos.z + rd.z * beg);
	outPos = p.xz;
	beg +=  Hash(p)*150.0;

	float d = 0.0;
	vec3 add = rd * ((end-beg) / float(RAY_TRACE_STEPS));
	float shadeSum = 0.0;
	
	for (int i = 0; i < RAY_TRACE_STEPS; i++)
	{
		if (shadeSum >= 1.0) break;

		float h = clamp(Map(p)*2.0, 0.0, 1.0);
		shadeSum += max(h, 0.0) * (1.0 - shadeSum);
		p += add;
	}

	return clamp(shadeSum, 0.0, 1.0);
}

float CloudShadows(vec3 position)
{
	cloudy = clamp(CLOUDS_CLOUDCOVER*0.3, 0.0, 0.3);
	cloudShadeFactor = 0.5+(cloudy*0.333);
	
	vec3 cameraPos = vec3(0.0);
    vec3 dir = normalize(u_PrimaryLightOrigin.xzy - position.xzy*0.25);
	dir = normalize(vec3(dir.x, -1.0, dir.z));

	vec2 pos;
	float alpha = GetCloudAlpha(cameraPos, dir, pos);

	return (1.0 - (alpha*0.75));
}
#endif //_CLOUD_SHADOWS_

// For all settings: 1.0 = 100% 0.5=50% 1.5 = 150%
vec3 ContrastSaturationBrightness(vec3 color, float con, float sat, float brt)
{
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5;
	const float AvgLumB = 0.5;
	
	const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
	
	vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
	vec3 brtColor = color * brt;
	vec3 intensity = vec3(dot(brtColor, LumCoeff));
	vec3 satColor = mix(intensity, brtColor, sat);
	vec3 conColor = mix(AvgLumin, satColor, con);
	return conColor;
}

vec3 ColorGrade( vec3 vColor )
{
	vec3 vHue = vec3(1.0, .7, .2);
	
	vec3 vGamma = 1.0 + vHue * 0.6;
	vec3 vGain = vec3(.9) + vHue * vHue * 8.0;
	
	vColor *= 1.5;
	
	float fMaxLum = 100.0;
	vColor /= fMaxLum;
	vColor = pow( vColor, vGamma );
	vColor *= vGain;
	vColor *= fMaxLum;  
	return vColor;
}

vec3 GetScreenPixel(inout vec2 texCoords)
{
	vec3 dMap = texture(u_RoadMap, texCoords).rgb;
	vec3 color = texture(u_DiffuseMap, texCoords).rgb;

	if (dMap.r <= 0.0 || DISPLACEMENT_STRENGTH == 0.0)
	{
		return color;
	}
	
	float invDepth = clamp((1.0 - texture(u_ScreenDepthMap, texCoords).r) * 2.0 - 1.0, 0.0, 1.0);

	if (invDepth <= 0.0)
	{
		return color;
	}

	float material = texture(u_PositionMap, texCoords).a - 1.0;
	float materialMultiplier = 1.0;

	if (material == MATERIAL_ROCK || material == MATERIAL_STONE || material == MATERIAL_SKYSCRAPER)
	{// Rock gets more displacement...
		materialMultiplier = 3.0;
	}
	else if (material == MATERIAL_TREEBARK)
	{// Rock gets more displacement...
		materialMultiplier = 1.5;
	}

	vec3 norm = vec3(dMap.gb, 0.0) * 2.0 - 1.0;

	vec2 distFromCenter = vec2(length(texCoords.x - 0.5), length(texCoords.y - 0.5));
	float displacementStrengthMod = ((DISPLACEMENT_STRENGTH * materialMultiplier) / 18.0); // Default is 18.0. If using higher displacement, need more screen edge flattening, if less, less flattening.
	float screenEdgeScale = clamp(max(distFromCenter.x, distFromCenter.y) * 2.0, 0.0, 1.0);
	screenEdgeScale = 1.0 - pow(screenEdgeScale, 16.0/displacementStrengthMod);

	float finalModifier = invDepth;

	float offset = -DISPLACEMENT_STRENGTH * materialMultiplier * finalModifier * screenEdgeScale * dMap.r;

	texCoords += norm.xy * pixel * offset;

	vec3 col = vec3(0.0);
	col = texture(u_DiffuseMap, texCoords).rgb;

	color = col;

	float shadow = 1.0 - clamp(dMap.r * 0.5 + 0.5, 0.0, 1.0);
	shadow = 1.0 - (shadow * finalModifier);
	color.rgb *= shadow;

	return color;
}

#if 0 // debug heightmap
float GetHeightMap(vec3 pos)
{
	if (pos.x < u_Mins.x || pos.y < u_Mins.y || pos.z < u_Mins.z) return 65536.0;
	if (pos.x > u_Maxs.x || pos.y > u_Maxs.y || pos.z > u_Maxs.z) return 65536.0;

	float h = textureLod(u_HeightMap, GetMapTC(pos), 1.0).r;
	//h = mix(u_Mins.z, u_Maxs.z, h);
	return h; // raise it up to correct loss of precision. it's only weather anyway...
}
#endif

void main(void)
{
	vec2 texCoords = var_TexCoords;
	vec4 position = positionMapAtCoord(texCoords);

#if 0 // debug heightmap
	gl_FragColor = vec4(vec3(GetHeightMap(position.xyz)), 1.0);
	return;
#endif

	vec4 color = vec4(GetScreenPixel(texCoords), 1.0);
	vec4 outColor = color;

	if (position.a - 1.0 >= MATERIAL_SKY
		|| position.a - 1.0 == MATERIAL_SUN
		|| position.a - 1.0 == MATERIAL_GLASS
		|| position.a - 1.0 == MATERIAL_DISTORTEDGLASS
		|| position.a - 1.0 == MATERIAL_DISTORTEDPUSH
		|| position.a - 1.0 == MATERIAL_DISTORTEDPULL
		|| position.a - 1.0 == MATERIAL_CLOAK
		|| position.a - 1.0 == MATERIAL_FORCEFIELD
		|| position.a - 1.0 == MATERIAL_EFX
		|| position.a - 1.0 == MATERIAL_BLASTERBOLT
		|| position.a - 1.0 == MATERIAL_FIRE
		|| position.a - 1.0 == MATERIAL_SMOKE
		|| position.a - 1.0 == MATERIAL_MAGIC_PARTICLES
		|| position.a - 1.0 == MATERIAL_MAGIC_PARTICLES_TREE
		|| position.a - 1.0 == MATERIAL_FIREFLIES
		|| position.a - 1.0 == MATERIAL_PORTAL
		|| position.a - 1.0 == MATERIAL_MENU_BACKGROUND)
	{// Skybox... Skip...
		if (MAP_USE_PALETTE_ON_SKY > 0.0)
		{
			if (!(CONTRAST_STRENGTH == 1.0 && SATURATION_STRENGTH == 1.0 && BRIGHTNESS_STRENGTH == 1.0))
			{// C/S/B enabled...
				outColor.rgb = ContrastSaturationBrightness(outColor.rgb, CONTRAST_STRENGTH, SATURATION_STRENGTH, BRIGHTNESS_STRENGTH);
			}
		}

		if (TRUEHDR_ENABLED > 0.0)
		{// TrueHDR enabled...
			outColor.rgb = TrueHDR(outColor.rgb);
		}

		if (VIBRANCY > 0.0)
		{// Vibrancy enabled...
			outColor.rgb = Vibrancy(outColor.rgb, VIBRANCY);
		}

		outColor.rgb = clamp(outColor.rgb, 0.0, 1.0);
		
		if (COLOR_GRADING_ENABLED > 0.0)
		{
			outColor.rgb = ColorGrade( outColor.rgb );
		}
		
		gl_FragColor = outColor;

		gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0 / GAMMA_CORRECTION));
		gl_FragColor.a = 1.0;
		return;
	}

	vec2 materialSettings = RB_PBR_DefaultsForMaterial(position.a-1.0);
	bool isPuddle = (position.a - 1.0 == MATERIAL_PUDDLE) ? true : false;

	vec4 norm = textureLod(u_NormalMap, texCoords, 0.0);
	norm.xyz = DecodeNormal(norm.xy);


	vec3 flatNorm = norm.xyz = normalize(norm.xyz);

#if defined(_SCREEN_SPACE_REFLECTIONS_)
	// If doing screen space reflections on floors, we need to know what are floor pixels...
	float ssReflection = norm.z * 0.5 + 0.5;

	// Allow only fairly flat surfaces for reflections (floors), and not roofs (for now)...
	if (ssReflection < 0.8)
	{
		ssReflection = 0.0;
	}
	else
	{
		ssReflection = clamp(ssReflection - 0.8, 0.0, 0.2) * 5.0;
	}

	if (isPuddle)
	{// Puddles always reflect and are considered flat, even if it's just an illusion...
		ssReflection = 1.0;
	}
#endif //defined(_SCREEN_SPACE_REFLECTIONS_)

#ifdef USE_REAL_NORMALMAPS

	// Now add detail offsets to the normal value...
	vec4 normalDetail = textureLod(u_OverlayMap, texCoords, 0.0);

	if (normalDetail.a < 1.0)
	{// If we don't have real normalmap, generate fallback normal offsets for this pixel from luminances...
		normalDetail = normalVector(outColor.rgb);
	}

#else //!USE_REAL_NORMALMAPS

	vec4 normalDetail = normalVector(outColor.rgb);

#endif //USE_REAL_NORMALMAPS

	// Simply offset the normal value based on the detail value... It looks good enough, but true PBR would probably want to use the tangent/bitangent below instead...
	normalDetail.rgb = normalize(clamp(normalDetail.xyz, 0.0, 1.0) * 2.0 - 1.0);

	norm.xyz = normalize(mix(norm.xyz, normalDetail.xyz, 0.25 * (length((norm.xyz * 0.5 + 0.5) - (normalDetail.xyz * 0.5 + 0.5)) / 3.0)));

	vec3 N = norm.xyz;
	vec3 E = normalize(u_ViewOrigin.xyz - position.xyz);
	vec3 sunDir = normalize(u_ViewOrigin.xyz - u_PrimaryLightOrigin.xyz);
	vec3 rayDir = reflect(E, N);
	vec3 cubeRayDir = reflect(E, flatNorm);
	float NE = clamp(length(dot(N, E)), 0.0, 1.0);

	float b = clamp(length(outColor.rgb/3.0), 0.0, 1.0);
	b = clamp(pow(b, 6.0/*64.0*/), 0.0, 1.0);
	vec3 of = clamp(pow(vec3(b*65536.0), -N), 0.0, 1.0) * 0.333;//0.5;//0.25;
	vec3 bump = normalize(N+of);

	vec4 occlusion = vec4(0.0);
	vec4 illumination = vec4(0.0);
	float sun_occlusion = 1.0;
	bool useOcclusion = false;

#ifdef _SSDO_
	if (USE_SSDO > 0.0)
	{
		useOcclusion = true;
		occlusion = texture(u_SplatControlMap, texCoords) * 2.0 - 1.0;
		illumination = texture(u_SplatMap1, texCoords);

		sun_occlusion = 1.0 - clamp(dot(vec4(-sunDir, 1.0), occlusion), 0.0, 1.0);
		sun_occlusion = sun_occlusion * 0.4 + 0.6;

		/*
		if (u_Local3.r == 1.0)
		{
			gl_FragColor = vec4(sun_occlusion, sun_occlusion, sun_occlusion, 1.0);
			return;
		}

		if (u_Local3.r == 2.0)
		{
			gl_FragColor = vec4(illumination.rgb, 1.0);
			return;
		}
		*/

		outColor.rgb += illumination.rgb * 0.25;
	}
#endif //_SSDO_


#ifdef _PROCEDURALS_IN_DEFERRED_SHADER_
	if (PROCEDURAL_MOSS_ENABLED > 0.0 && (position.a - 1.0 == MATERIAL_TREEBARK || position.a - 1.0 == MATERIAL_ROCK))
	{// Add any procedural moss...
		AddProceduralMoss(outColor, position);
	}

	if (PROCEDURAL_SNOW_ENABLED > 0.0 
		&& (PROCEDURAL_SNOW_ROCK_ONLY <= 0.0 || (position.a - 1.0 == MATERIAL_ROCK)))
	{// Add any procedural snow...
		if (position.z >= PROCEDURAL_SNOW_LOWEST_ELEVATION)
		{
			float snowHeightFactor = 1.0;

			if (PROCEDURAL_SNOW_LOWEST_ELEVATION > -999999.0)
			{// Elevation is enabled...
				float elevationRange = MAP_INFO_PLAYABLE_HEIGHT - PROCEDURAL_SNOW_LOWEST_ELEVATION;
				float pixelElevation = position.z - PROCEDURAL_SNOW_LOWEST_ELEVATION;
			
				snowHeightFactor = clamp(pixelElevation / elevationRange, 0.0, 1.0);
				snowHeightFactor = clamp(pow(snowHeightFactor, PROCEDURAL_SNOW_HEIGHT_CURVE), 0.0, 1.0);
			}

			float snow = clamp(dot(normalize(bump.xyz), vec3(0.0, 0.0, 1.0)), 0.0, 1.0);

			if (position.a - 1.0 == MATERIAL_GREENLEAVES || position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
				snow = pow(snow * 0.25 + 0.75, 1.333);
			else
				snow = pow(snow, 0.4);

			snow *= snowHeightFactor;
		
			if (snow > 0.0)
			{
				vec3 snowColor = vec3(PROCEDURAL_SNOW_BRIGHTNESS);
				float snowColorFactor = max(outColor.r, max(outColor.g, outColor.b));
				snowColorFactor = clamp(pow(snowColorFactor, PROCEDURAL_SNOW_LUMINOSITY_CURVE), 0.0, 1.0);
				float snowMix = clamp(snow*snowColorFactor, 0.0, 1.0);
				outColor.rgb = splatblend(outColor.rgb, 1.0 - snowMix, snowColor, snowMix);
			}
		}
	}
#endif //_PROCEDURALS_IN_DEFERRED_SHADER_


	float wetness = 0.0;

	if (WETNESS > 0.0)
	{
		if ((position.a - 1.0 == MATERIAL_PUDDLE)
			|| (position.a - 1.0 == MATERIAL_TREEBARK)
			|| (position.a - 1.0 == MATERIAL_GREENLEAVES)
			|| (position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
			|| (position.a - 1.0 == MATERIAL_SHORTGRASS)
			|| (position.a - 1.0 == MATERIAL_LONGGRASS)
			|| (position.a - 1.0 == MATERIAL_ROCK)
			|| (position.a - 1.0 == MATERIAL_SAND)
			|| (position.a - 1.0 == MATERIAL_DIRT)
			|| (position.a - 1.0 == MATERIAL_MUD))
		{
			wetness = WETNESS;

#if defined(_SCREEN_SPACE_REFLECTIONS_)
			// Allow only fairly flat surfaces for reflections...
			if (norm.z * 0.5 + 0.5 >= 0.8)
			{
				ssReflection = WETNESS;
			}
#endif //defined(_SCREEN_SPACE_REFLECTIONS_)
		}
	}

#define specularReflectivePower		materialSettings.x
#define reflectionPower				materialSettings.y

	// This should give me roughly how close to grey this color is... For light colorization.. Highly colored stuff should get less color added to it...
	float greynessFactor = 1.0 - clamp((length(outColor.r - outColor.g) + length(outColor.r - outColor.b) + length(outColor.g - outColor.b)) / 3.0, 0.0, 1.0);

	// Also check how bright it is, so we can scale the lighting up/down...
	float brightnessFactor = 1.0 - clamp(max(outColor.r, max(outColor.g, outColor.b)), 0.0, 1.0);
	brightnessFactor = 1.0 - clamp(pow(brightnessFactor,  6.0), 0.0, 1.0);

	// It looks better to use slightly different cube and light reflection multipliers... Lights should always add some light, cubes should allow none on some pixels..
	float cubeReflectionFactor = clamp(greynessFactor * brightnessFactor, 0.5, 1.0) * reflectionPower;
	float lightsReflectionFactor = (greynessFactor * brightnessFactor * specularReflectivePower) * 0.5 + 0.5;

#if defined(_SCREEN_SPACE_REFLECTIONS_)
	float ssrReflectivePower = lightsReflectionFactor * reflectionPower * ssReflection;
	if (isPuddle) ssrReflectivePower = WETNESS * 3.0; // 3x - 8x seems about right...
	else if (wetness > 0.0) ssrReflectivePower = ssReflection * 0.333;
	else if (ssrReflectivePower < 0.5) ssrReflectivePower = 0.0; // cull on non-wet stuff, when theres little point...
#endif //defined(_SCREEN_SPACE_REFLECTIONS_)

#if defined(_CUBEMAPS_) && defined(REALTIME_CUBEMAPS)
	if (isPuddle) cubeReflectionFactor = WETNESS * 3.0; // 3x - 8x seems about right...
	else if (wetness > 0.0) cubeReflectionFactor += cubeReflectionFactor*0.333;
#endif //defined(_CUBEMAPS_) && defined(REALTIME_CUBEMAPS)

	vec3 lightingNormal = normalize(bump * vec3(1.0, 1.0, 0.25));
	float origColorStrength = clamp(getSpecialSauce(color.rgb) * 0.25, 0.0, 1.0);


	float finalShadow = 1.0;

#if defined(USE_SHADOWMAP) && !defined(LQ_MODE)
	if (SHADOWS_ENABLED > 0.0 && NIGHT_SCALE < 1.0)
	{
		float shadowValue = texture(u_ShadowMap, texCoords).r;
		float selfShadow = max(dot(flatNorm, -sunDir.rgb), 0.0);

		if (position.a - 1.0 == MATERIAL_GREENLEAVES)
		{
			selfShadow = clamp(selfShadow + 0.5, 0.0, 1.0);
		}
		if (position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
		{
			selfShadow = clamp(selfShadow + 0.5, 0.0, 1.0);
		}

		shadowValue = min(shadowValue, selfShadow);

		shadowValue = pow(shadowValue, 1.5);

#define sm_cont_1 ( 64.0 / 255.0)
#define sm_cont_2 (255.0 / 200.0)
		shadowValue = clamp((clamp(shadowValue - sm_cont_1, 0.0, 1.0)) * sm_cont_2, 0.0, 1.0);
		finalShadow = clamp(shadowValue + SHADOW_MINBRIGHT, SHADOW_MINBRIGHT, SHADOW_MAXBRIGHT);
	}
#elif defined(LQ_MODE)
	if (SHADOWS_ENABLED > 0.0 && NIGHT_SCALE < 1.0)
	{
		float selfShadow = max(dot(flatNorm, -sunDir.rgb), 0.0);

		if (position.a - 1.0 == MATERIAL_GREENLEAVES)
		{
			selfShadow = clamp(selfShadow + 0.5, 0.0, 1.0);
		}
		if (position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
		{
			selfShadow = clamp(selfShadow + 0.5, 0.0, 1.0);
		}

		selfShadow = pow(selfShadow, 1.5);

#define sm_cont_1 ( 64.0 / 255.0)
#define sm_cont_2 (255.0 / 200.0)
		selfShadow = clamp((clamp(selfShadow - sm_cont_1, 0.0, 1.0)) * sm_cont_2, 0.0, 1.0);
		finalShadow = clamp(selfShadow + SHADOW_MINBRIGHT, SHADOW_MINBRIGHT, SHADOW_MAXBRIGHT);
	}
#endif //defined(USE_SHADOWMAP) && !defined(LQ_MODE)

	vec3 specularColor = vec3(0.0);
	vec3 skyColor = vec3(0.0);

#ifndef LQ_MODE
	if (SKY_LIGHT_CONTRIBUTION > 0.0 && (cubeReflectionFactor > 0.0 || reflectionPower > 0.0))
	{// Sky cube light contributions... If enabled...
		vec4 cubeInfo = vec4(0.0, 0.0, 0.0, 1.0);
		cubeInfo.xyz -= u_ViewOrigin.xyz;

		cubeInfo.w = pow(distance(u_ViewOrigin.xyz, vec3(0.0, 0.0, 0.0)), 3.0);

		cubeInfo.xyz *= 1.0 / cubeInfo.w;
		cubeInfo.w = 1.0 / cubeInfo.w;
					
		vec3 parallax = cubeInfo.xyz + cubeInfo.w * E;
		parallax.z *= -1.0;
		
		vec3 reflected = cubeRayDir + parallax;

		if (PROCEDURAL_SKY_ENABLED <= 0.0)
		{
			reflected = vec3(-reflected.y, -reflected.z, -reflected.x); // for old sky cubemap generation based on sky textures
		}

#ifdef _REALTIME_SKYCUBES_
		skyColor = textureLod(u_SkyCubeMap, reflected, 4.0).rgb;
#else //!_REALTIME_SKYCUBES_
		if (NIGHT_SCALE > 0.0 && NIGHT_SCALE < 1.0)
		{// Mix between night and day colors...
			vec3 skyColorDay = textureLod(u_SkyCubeMap, reflected, 4.0).rgb;
			vec3 skyColorNight = textureLod(u_SkyCubeMapNight, reflected, 4.0).rgb;
			skyColor = mix(skyColorDay, skyColorNight, NIGHT_SCALE);
		}
		else if (NIGHT_SCALE >= 1.0)
		{// Night only colors...
			skyColor = textureLod(u_SkyCubeMapNight, reflected, 4.0).rgb;
		}
		else
		{// Day only colors...
			skyColor = textureLod(u_SkyCubeMap, reflected, 4.0).rgb;
		}
#endif //_REALTIME_SKYCUBES_

		skyColor = clamp(ContrastSaturationBrightness(skyColor, 1.0, 2.0, 0.333), 0.0, 1.0);
		skyColor = clamp(Vibrancy( skyColor, 0.4 ), 0.0, 1.0);
	}
#endif //LQ_MODE

	if (specularReflectivePower > 0.0)
	{
#ifndef LQ_MODE
#if defined(_CUBEMAPS_)
#ifdef REALTIME_CUBEMAPS
		if (CUBEMAP_ENABLED > 0.0 && cubeReflectionFactor > 0.0 && NE > 0.0 && u_CubeMapStrength > 0.0)
		{// Cubemaps enabled...
			float curDist = distance(u_ViewOrigin.xyz, position.xyz);
			float cubeFade = 1.0 - clamp(curDist / CUBEMAP_CULLRANGE, 0.0, 1.0);
			cubeFade = pow(cubeFade, 1.5);
			
			if (cubeFade > 0.0)
			{
				vec3 cubeLightColor = textureLod(u_CubeMap, cubeRayDir, /*0.0*/7.0 - (cubeReflectionFactor * 7.0)).rgb;
				outColor.rgb = mix(outColor.rgb, outColor.rgb + cubeLightColor.rgb, clamp(NE * cubeFade * (u_CubeMapStrength * 5.0) * cubeReflectionFactor, 0.0, 1.0));
			}
		}
#else //!REALTIME_CUBEMAPS
		if (CUBEMAP_ENABLED > 0.0 && cubeReflectionFactor > 0.0 && NE > 0.0 && u_CubeMapStrength > 0.0)
		{// Cubemaps enabled...
			vec3 cubeLightColor = vec3(0.0);
			float curDist = distance(u_ViewOrigin.xyz, position.xyz);
			float cubeDist = distance(u_CubeMapInfo.xyz, position.xyz);
			float cubeRadius = min(CUBEMAP_CULLRANGE, u_CubeMapInfo.w);
			float cubeFade = min(1.0 - clamp(curDist / CUBEMAP_CULLRANGE, 0.0, 1.0), 1.0 - clamp(cubeDist / u_CubeMapInfo.w, 0.0, 1.0));
			
			// This used to be done in rend2 code, now done here because I need u_CubeMapInfo.xyz to be cube origin for distance checks above... u_CubeMapInfo.w is now radius.
			vec4 cubeInfo = u_CubeMapInfo;
			cubeInfo.xyz = cubeInfo.xyz - u_ViewOrigin.xyz;
			cubeInfo.w = cubeDist;

			cubeInfo.xyz *= 1.0 / cubeInfo.w;
			cubeInfo.w = 1.0 / cubeInfo.w;

			vec3 parallax = cubeInfo.xyz + cubeInfo.w * E;
			parallax.z *= -1.0;

			if (cubeFade > 0.0)
			{
				cubeLightColor = (textureLod(u_CubeMap, cubeRayDir + parallax, 7.0 - (cubeReflectionFactor * 7.0)).rgb + texture(u_CubeMap, cubeRayDir + parallax).rgb) * 0.5;
				outColor.rgb = mix(outColor.rgb, outColor.rgb + cubeLightColor.rgb, clamp(NE * cubeFade * (u_CubeMapStrength /** 20.0*/) * cubeReflectionFactor, 0.0, 1.0));
			}
		}
#endif //REALTIME_CUBEMAPS
		else
		{
			if (cubeReflectionFactor > 0.0 && NE > 0.0)
			{
				vec2 shinyTC = ((cubeRayDir.xy + cubeRayDir.z) / 2.0) * 0.5 + 0.5;

				vec3 shiny = textureLod(u_WaterEdgeMap, shinyTC, 5.5 - (cubeReflectionFactor * 5.5)).rgb;
				shiny = clamp(ContrastSaturationBrightness(shiny, 1.75, 1.0, 0.333), 0.0, 1.0);
				outColor.rgb = mix(outColor.rgb, outColor.rgb + shiny.rgb, clamp(NE * cubeReflectionFactor * (origColorStrength * 0.75 + 0.25), 0.0, 1.0));
			}
		}
#else //!defined(_CUBEMAPS_)
		if (cubeReflectionFactor > 0.0 && NE > 0.0)
		{
			vec2 shinyTC = ((cubeRayDir.xy + cubeRayDir.z) / 2.0) * 0.5 + 0.5;

			vec3 shiny = textureLod(u_WaterEdgeMap, shinyTC, 5.5 - (cubeReflectionFactor * 5.5)).rgb;
			shiny = clamp(ContrastSaturationBrightness(shiny, 1.75, 1.0, 0.333), 0.0, 1.0);
			outColor.rgb = mix(outColor.rgb, outColor.rgb + shiny.rgb, clamp(NE * cubeReflectionFactor * (origColorStrength * 0.75 + 0.25), 0.0, 1.0));
		}
#endif //defined(_CUBEMAPS_)
#endif //!LQ_MODE
	}

	if (SKY_LIGHT_CONTRIBUTION > 0.0 && cubeReflectionFactor > 0.0)
	{// Sky light contributions...
#ifndef LQ_MODE
		outColor.rgb = mix(outColor.rgb, outColor.rgb + skyColor, clamp(NE * SKY_LIGHT_CONTRIBUTION * cubeReflectionFactor * (origColorStrength * 0.75 + 0.25), 0.0, 1.0));
#endif //LQ_MODE
	}

	float PshadowValue = 1.0;

	if (lightsReflectionFactor > 0.0)
	{
		PshadowValue = 1.0 - texture(u_RoadsControlMap, texCoords).a;
	}

	if (SUN_PHONG_SCALE > 0.0 && lightsReflectionFactor > 0.0)
	{// If r_blinnPhong is <= 0.0 then this is pointless...
		float phongFactor = (SUN_PHONG_SCALE * 12.0);

		if (phongFactor > 0.0 && NIGHT_SCALE < 1.0 && finalShadow > 0.0)
		{// this is blinn phong
			float maxBright = clamp(max(outColor.r, max(outColor.g, outColor.b)), 0.0, 1.0);
			vec3 lightColor = Vibrancy(u_PrimaryLightColor.rgb, 1.0);
			float power = clamp(pow(maxBright * 0.75, 4.0) + 0.333, 0.0, 1.0);
			float lightMult = clamp(specularReflectivePower * power, 0.0, 1.0);

			if (lightMult > 0.0)
			{
				lightColor *= lightMult;
				lightColor *= 1.0 - NIGHT_SCALE; // Day->Night scaling of sunlight...

				lightColor.rgb *= lightsReflectionFactor * phongFactor * origColorStrength * 8.0;

				vec3 addColor = Lighting(position, outColor.rgb, lightingNormal, flatNorm, E, -sunDir, clamp(lightColor, 0.0, 1.0), 1.0, u_PrimaryLightOrigin.xyz, wetness);

				addColor *= sun_occlusion;

				if (position.a - 1.0 == MATERIAL_GREENLEAVES || position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
				{// Light bleeding through tree leaves...
					float ndotl = clamp(dot(lightingNormal, sunDir), 0.0, 1.0);
					float diffuse = pow(ndotl, 2.0) * 1.25;

					addColor += lightColor * diffuse;
				}

				float puddleMult = 1.0;
				if (isPuddle) puddleMult = 0.25;
				outColor.rgb = outColor.rgb + max(addColor * PshadowValue * finalShadow * puddleMult, vec3(0.0));
			}
		}
	}

	if (lightsReflectionFactor > 0.0)
	{
#ifdef USE_LIGHTS_SSBO
		GetSSBOLighting(false, origColorStrength, lightsReflectionFactor, specularReflectivePower, position, lightingNormal, flatNorm, E, wetness, useOcclusion, occlusion, PshadowValue, outColor);

	#ifdef _USE_MAP_EMMISSIVE_BLOCK_
		GetSSBOLighting(true, origColorStrength, lightsReflectionFactor, specularReflectivePower, position, lightingNormal, flatNorm, E, wetness, useOcclusion, occlusion, PshadowValue, outColor);
	#endif //_USE_MAP_EMMISSIVE_BLOCK_
#else //!USE_LIGHTS_SSBO
		if (u_lightCount > 0.0)
		{
			vec3 addedLight = vec3(0.0);

			for (int li = 0; li < u_lightCount; li++)
			{
				vec3 lightPos = u_lightPositions2[li].xyz;
				vec3 lightDir = normalize(lightPos - position.xyz);
				float lightDist = distance(lightPos, position.xyz);

				float lightPlayerDist = distance(lightPos.xyz, u_ViewOrigin.xyz);

				float lightDistMult = 1.0 - clamp((lightPlayerDist / MAX_DEFERRED_LIGHT_RANGE), 0.0, 1.0);
				lightDistMult = pow(lightDistMult, 2.0);

				if (lightDistMult > 0.0)
				{
					// Attenuation...
					float lightFade = 1.0 - clamp((lightDist * lightDist) / (u_lightDistances[li] * u_lightDistances[li]), 0.0, 1.0);
					lightFade = pow(lightFade, 2.0);

					if (lightFade > 0.0)
					{
						float lightStrength = lightDistMult * lightFade * lightsReflectionFactor * 0.5;
						float maxLightsScale = mix(0.01, 1.0, clamp(pow(1.0 - (lightPlayerDist / u_lightMaxDistance), 0.5), 0.0, 1.0));
						lightStrength *= maxLightsScale;

						if (lightStrength > 0.0)
						{
							vec3 lightColor = (u_lightColors[li].rgb / length(u_lightColors[li].rgb)) /** MAP_EMISSIVE_COLOR_SCALE*/ * maxLightsScale;
							float selfShadow = clamp(pow(clamp(dot(-lightDir.rgb, bump.rgb), 0.0, 1.0), 8.0) * 0.6 + 0.6, 0.0, 1.0);

							float lightSpecularPower = mix(0.1, 0.5, clamp(lightsReflectionFactor, 0.0, 1.0)) * clamp(lightStrength * 12.0, 0.0, 1.0);

							addedLight.rgb += Lighting(position, outColor.rgb, bump, flatNorm, E, lightDir, lightColor/*clamp(lightColor, 0.0, 1.0)*/, lightSpecularPower, lightPos, wetness) * lightFade * selfShadow;

							if (position.a - 1.0 == MATERIAL_GREENLEAVES || position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
							{// Light bleeding through tree leaves...
								float ndotl = clamp(dot(bump, -lightDir), 0.0, 1.0);
								float diffuse = pow(ndotl, 2.0) * 4.0;

								addedLight.rgb += lightColor * diffuse * lightFade * selfShadow * lightSpecularPower;
							}
						}
					}
				}
			}

			outColor.rgb = outColor.rgb + max(addedLight * 0.5 * PshadowValue * MAP_EMISSIVE_COLOR_SCALE, vec3(0.0));
		}
#endif //USE_LIGHTS_SSBO
	}

#if defined(_SCREEN_SPACE_REFLECTIONS_)
	if (REFLECTIONS_ENABLED > 0.0 && ssrReflectivePower > 0.0 && position.a - 1.0 != MATERIAL_WATER)
	{
		if (isPuddle)
		{// Just basic water color addition for now with reflections... Maybe raindrops at some point later...
			outColor.rgb += vec3(0.0, 0.1, 0.15);
		}

		outColor.rgb = AddReflection(texCoords, position, flatNorm, outColor.rgb, ssrReflectivePower);
	}
#endif //defined(_SCREEN_SPACE_REFLECTIONS_)

#if defined(_AMBIENT_OCCLUSION_)
	#if defined(_ENHANCED_AO_)
		if (AO_TYPE == 1.0)
	#else //!defined(_ENHANCED_AO_)
		if (AO_TYPE >= 1.0)
	#endif //defined(_ENHANCED_AO_)
		{// Fast AO enabled...
			float ao = calculateAO(sunDir, N * 10000.0, texCoords);
			float selfShadow = clamp(pow(clamp(dot(-sunDir.rgb, bump.rgb), 0.0, 1.0), 8.0), 0.0, 1.0);
			ao = clamp(((ao + selfShadow) / 2.0) * AO_MULTBRIGHT + AO_MINBRIGHT, AO_MINBRIGHT, 1.0);
			outColor.rgb *= ao;
		}
#endif //defined(_AMBIENT_OCCLUSION_)

#if defined(_ENHANCED_AO_)
	if (AO_TYPE >= 2.0)
	{// Better, HQ AO enabled...
		float msao = 0.0;
#ifdef _ENABLE_GI_
		vec3 gi = vec3(0.0);
#endif //_ENABLE_GI_

		if (AO_TYPE >= 3.0)
		{
			int width = int(AO_TYPE-2.0);
			float numSamples = 0.0;
		
			for (int x = -width; x <= width; x+=2)
			{
				for (int y = -width; y <= width; y+=2)
				{
					vec2 coord = texCoords + (vec2(float(x), float(y)) * pixel);
#ifdef _ENABLE_GI_
					vec4 giInfo = textureLod(u_SteepMap1, coord, 0.0);
					vec3 illum = giInfo.rgb;
					msao += giInfo.a-1.0;
					gi += illum;
#else //!_ENABLE_GI_
					msao += textureLod(u_SteepMap1, coord, 0.0).x;
#endif //_ENABLE_GI_
					numSamples += 1.0;
				}
			}

			msao /= numSamples;
#ifdef _ENABLE_GI_
			gi /= numSamples;
#endif //_ENABLE_GI_
		}
		else
		{
#ifdef _ENABLE_GI_
			vec4 giInfo = textureLod(u_SteepMap1, texCoords, 0.0);
			msao = giInfo.a-1.0;
			gi = giInfo.rgb;
#else //!_ENABLE_GI_
			msao = textureLod(u_SteepMap1, texCoords, 0.0).x;
#endif //_ENABLE_GI_
		}

#ifdef _ENABLE_GI_
		float sao = clamp(msao, 0.0, 1.0);

#if defined(_AMBIENT_OCCLUSION_)
		// Fast AO enabled...
		float fao = calculateAO(sunDir, N * 10000.0, texCoords);
		float selfShadow = clamp(pow(clamp(dot(-sunDir.rgb, bump.rgb), 0.0, 1.0), 8.0), 0.0, 1.0);
		fao = (fao + selfShadow) / 2.0;
		sao = min(sao, fao);
#endif //defined(_AMBIENT_OCCLUSION_)

		sao = clamp(sao * AO_MULTBRIGHT + AO_MINBRIGHT, AO_MINBRIGHT, 1.0);
		outColor.rgb *= sao;
		outColor.rgb += gi * sao;
#else //!_ENABLE_GI_
		float sao = clamp(msao, 0.0, 1.0);
		sao = clamp(sao * AO_MULTBRIGHT + AO_MINBRIGHT, AO_MINBRIGHT, 1.0);
		outColor.rgb *= sao;
#endif //_ENABLE_GI_
	}
#endif //defined(_ENHANCED_AO_)

	finalShadow = mix(finalShadow, 1.0, NIGHT_SCALE); // Dampen out shadows at sunrise/sunset...

#ifdef _CLOUD_SHADOWS_
	if (CLOUDS_SHADOWS_ENABLED == 1.0 && NIGHT_SCALE < 1.0)
	{
		finalShadow = min(finalShadow, clamp(var_CloudShadow + 0.1, 0.0, 1.0));
	}
	else if (CLOUDS_SHADOWS_ENABLED >= 2.0 && NIGHT_SCALE < 1.0)
	{
		float cShadow = CloudShadows(position.xyz) * 0.5 + 0.5;
		cShadow = mix(cShadow, 1.0, NIGHT_SCALE); // Dampen out cloud shadows at sunrise/sunset...
		finalShadow = min(finalShadow, clamp(cShadow + 0.1, 0.0, 1.0));
	}
#endif //_CLOUD_SHADOWS_

	finalShadow = min(finalShadow, sun_occlusion);
	outColor.rgb *= finalShadow;


	if (!(CONTRAST_STRENGTH == 1.0 && SATURATION_STRENGTH == 1.0 && BRIGHTNESS_STRENGTH == 1.0))
	{// C/S/B enabled...
		outColor.rgb = ContrastSaturationBrightness(outColor.rgb, CONTRAST_STRENGTH, SATURATION_STRENGTH, BRIGHTNESS_STRENGTH);
	}

	if (TRUEHDR_ENABLED > 0.0)
	{// TrueHDR enabled...
		outColor.rgb = TrueHDR( outColor.rgb );
	}

	if (VIBRANCY > 0.0)
	{// Vibrancy enabled...
		outColor.rgb = Vibrancy( outColor.rgb, VIBRANCY );
	}

	outColor.rgb = clamp(outColor.rgb, 0.0, 1.0);

	if (COLOR_GRADING_ENABLED > 0.0)
	{
		outColor.rgb = ColorGrade( outColor.rgb );
	}

	gl_FragColor = outColor;
	
	gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0 / GAMMA_CORRECTION));
	gl_FragColor.a = 1.0;
}

