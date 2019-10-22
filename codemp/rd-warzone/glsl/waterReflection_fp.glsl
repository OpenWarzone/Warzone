#define FIX_WATER_DEPTH_ISSUES		// Use basic depth value for sky hits...

/*
heightMap – height-map used for waves generation as described in the section “Modifying existing geometry”
backBufferMap – current contents of the back buffer
positionMap – texture storing scene position vectors (material type is alpha)
waterPositionMap – texture storing scene water position vectors (alpha is 1.0 when water is at this pixel. 0.0 if not).
normalMap – texture storing normal vectors for normal mapping as described in the section “The computation of normal vectors”
foamMap – texture containing foam – in my case it is a photo of foam converted to greyscale
*/

uniform mat4		u_ModelViewProjectionMatrix;

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
uniform sampler2D	u_DiffuseMap;			// backBufferMap
uniform sampler2D	u_PositionMap;

uniform sampler2D	u_OverlayMap;			// foamMap 1
uniform sampler2D	u_SplatMap1;			// foamMap 2
uniform sampler2D	u_SplatMap2;			// foamMap 3
uniform sampler2D	u_SplatMap3;			// foamMap 4

uniform sampler2D	u_DetailMap;			// causics map

uniform sampler2D	u_HeightMap;			// map height map

uniform sampler2D	u_WaterPositionMap;

uniform samplerCube	u_SkyCubeMap;
uniform samplerCube	u_SkyCubeMapNight;
#endif //defined(USE_BINDLESS_TEXTURES)


uniform vec4		u_Mins;					// MAP_MINS[0], MAP_MINS[1], MAP_MINS[2], 0.0
uniform vec4		u_Maxs;					// MAP_MAXS[0], MAP_MAXS[1], MAP_MAXS[2], 0.0
uniform vec4		u_MapInfo;				// MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], SUN_VISIBLE

uniform vec4		u_Local0;				// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec4		u_Local1;				// MAP_WATER_LEVEL, USE_GLSL_REFLECTION, IS_UNDERWATER, WATER_REFLECTIVENESS
uniform vec4		u_Local10;				// waveHeight, waveDensity, USE_OCEAN, WATER_UNDERWATER_CLARITY

uniform vec2		u_Dimensions;
uniform vec4		u_ViewInfo;				// zmin, zmax, zmax / zmin

varying vec2		var_TexCoords;

// Position of the camera
uniform vec3		u_ViewOrigin;
#define ViewOrigin	u_ViewOrigin.xzy

#define				MAP_WATER_LEVEL		u_Local1.r
#define				waveHeight u_Local10.r

vec4 positionMapAtCoord ( vec2 coord )
{
	return texture(u_PositionMap, coord).xzyw;
}

vec4 waterMapLowerAtCoord ( vec2 coord )
{
	vec4 wmap = texture(u_WaterPositionMap, coord).xzyw;

	if (u_Local1.b <= 0.0) 
		wmap.y = max(wmap.y, MAP_WATER_LEVEL);

	wmap.y -= waveHeight;
	return wmap;
}

vec4 waterMapUpperAtCoord ( vec2 coord )
{
	vec4 wmap = texture(u_WaterPositionMap, coord).xzyw;

	if (u_Local1.b <= 0.0) 
		wmap.y = max(wmap.y, MAP_WATER_LEVEL);

	//wmap.y += waveHeight;
	return wmap;
}

float pw = (1.0/u_Dimensions.x);
float ph = (1.0/u_Dimensions.y);

vec4 AddReflection(vec2 coord, vec3 positionMap, vec3 waterMapLower)
{
	if (positionMap.y > waterMapLower.y)
	{
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	float wMapCheck = texture(u_WaterPositionMap, vec2(coord.x, 1.0)).a;
	if (wMapCheck > 0.0)
	{// Top of screen pixel is water, don't check...
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	// Quick scan for pixel that is not water...
	float QLAND_Y = 0.0;
	float topY = min(coord.y + 0.6, 1.0); // Don'y scan further then this, waste of time as it will be bleneded out...

	const float scanSpeed = 4.0; // How many pixels to scan by on the 1st rough pass...
	
	for (float y = coord.y; y <= topY; y += ph * scanSpeed)
	{
		float isWater = texture(u_WaterPositionMap, vec2(coord.x, y)).a;

		if (isWater <= 0.0)
		{
			QLAND_Y = y;
			break;
		}
	}

	if (QLAND_Y <= 0.0 || QLAND_Y >= 1.0)
	{// Found no non-water surfaces...
		return vec4(0.0, 0.0, 0.0, 1.0);
	}
	
	QLAND_Y -= ph * scanSpeed;
	
	// Full scan from within scanSpeed px for the real 1st pixel...
	float upPos = coord.y;
	float LAND_Y = 0.0;
	float topY2 = QLAND_Y + (ph * scanSpeed);

	for (float y = QLAND_Y; y <= topY2; y += ph)
	{
		float isWater = texture(u_WaterPositionMap, vec2(coord.x, y)).a;

		if (isWater <= 0.0)
		{
			LAND_Y = y;
			break;
		}
	}

	if (LAND_Y <= 0.0 || LAND_Y >= 1.0)
	{// Found no non-water surfaces...
		LAND_Y = QLAND_Y;
	}

	upPos = clamp(coord.y + ((LAND_Y - coord.y) * 2.0), 0.0, 1.0);

	if (upPos > 1.0 || upPos < 0.0)
	{// Not on screen...
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	vec2 finalPosition = clamp(vec2(coord.x, upPos), 0.0, 1.0);

	vec4 pMap = positionMapAtCoord(finalPosition);
	pMap.a -= 1.0;
	float pixelDistance = distance(waterMapLower.xyz, ViewOrigin.xyz);

	if (!(pMap.a == MATERIAL_SKY || pMap.a == MATERIAL_SUN) && distance(pMap.xyz, ViewOrigin) <= pixelDistance)
	{// This position is closer than the original pixel... but not sky
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	vec3 landColor = textureLod(u_DiffuseMap, finalPosition, 0.0).rgb;
	return vec4(landColor.rgb, 1.0 + (1.0 - clamp(pow(upPos, 4.0), 0.0, 1.0)));
}

void main ( void )
{
	if (u_Local1.b > 0.0) 
	{
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec4 waterMapUpper = waterMapUpperAtCoord(var_TexCoords);
	vec4 waterMapLower = waterMapUpper;
	waterMapLower.y -= waveHeight;
	bool pixelIsInWaterRange = false;

	vec4 positionMap = positionMapAtCoord(var_TexCoords);
	vec3 position = positionMap.xyz;
	bool isSky = (positionMap.a-1.0 == 1024.0 || positionMap.a-1.0 == 1025.0) ? true : false;
	vec2 uv = var_TexCoords;

	if (waterMapLower.a > 0.0)
	{
		position.xz = waterMapLower.xz; // test

#if defined(FIX_WATER_DEPTH_ISSUES)
		if (isSky)
		{
			position.xyz = waterMapLower.xyz;
			position.y -= 1024.0;
		}
#endif //defined(FIX_WATER_DEPTH_ISSUES)
	}
	else
	{
#if defined(FIX_WATER_DEPTH_ISSUES)
		if (isSky)
		{
			position.y -= 1024.0;
		}
#endif //defined(FIX_WATER_DEPTH_ISSUES)
	}

	if (waterMapLower.a >= 2.0)
	{// Actual waterfall pixel...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
	else if (waterMapUpper.a <= 0.0)
	{// Should be safe to skip everything else...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
	else if (waterMapLower.y > ViewOrigin.y)
	{// Underwater, no reflections...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
	else if (waterMapUpper.y >= position.y || position.y <= waterMapLower.y + waveHeight)
	{// Need reflections here...
		//vec4 waterMapMiddle = (waterMapUpper + waterMapLower) / 2.0;
		gl_FragColor = AddReflection(var_TexCoords, position.xyz, waterMapUpper.xyz);
		return;
	}
	else
	{// Not within water range...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
