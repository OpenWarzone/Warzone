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
uniform sampler2DShadow				u_ShadowMap;
uniform sampler2DShadow				u_ShadowMap2;
uniform sampler2DShadow				u_ShadowMap3;
uniform sampler2DShadow				u_ShadowMap4;
uniform sampler2DShadow				u_ShadowMap5;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_NormalMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2		u_Dimensions;

uniform vec4		u_Local1; // floatTime, isVlight (0 is bloomrays), r_testshaderValue1, r_testshaderValue2

varying vec2		var_ScreenTex;

// Enable output Debugging...
//#define DEBUG

// Shall we blur the result?
//#define BLUR_WIDTH 1.0
#define BLUR_WIDTH 2.0

// Shall we pixelize randomly the output? -- Sucks!
#define RANDOMIZE_PIXELS

//#define APPLY_CONTRAST

#ifdef RANDOMIZE_PIXELS
float rand(vec2 co) {
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453) * 0.25 + 0.75;
}
#endif

void main()
{
	//vec3 diffuseColor = texture(u_DiffuseMap, var_ScreenTex).rgb;
	vec3 diffuseColor = texture(u_DiffuseMap, u_Local1.g > 0.0 ? clamp(vec2(var_ScreenTex.x, 1.0-var_ScreenTex.y), 0.0, 1.0) : var_ScreenTex).rgb;
	
#if defined(BLUR_WIDTH)
	vec2 offset = vec2(1.0) / u_Dimensions;

	vec4 pixelLight = texture(u_NormalMap, var_ScreenTex);
	vec3 volumeLight = pixelLight.rgb;
	
	float numSamples = 1.0;

	for (float x = -BLUR_WIDTH; x <= BLUR_WIDTH; x += 1.0)
	{
		for (float y = -BLUR_WIDTH; y <= BLUR_WIDTH; y += 1.0)
		{
			vec2 coord = var_ScreenTex + (offset * vec2(x*length(x), y*length(y)));
			coord.y = 1.0 - coord.y;
			if (coord.x >= 0.0 && coord.x <= 1.0 && coord.y >= 0.0 && coord.y <= 1.0)
			{
				volumeLight += texture(u_NormalMap, coord).rgb;
				numSamples += 1.0;
			}
		}
	}

	volumeLight /= numSamples;

#else
	vec3 volumeLight = texture(u_NormalMap, var_ScreenTex).rgb;
#endif

#ifdef APPLY_CONTRAST
#define const_1 ( 64.0 / 255.0)
#define const_2 (255.0 / 255.0)
	volumeLight.rgb = clamp((clamp(volumeLight.rgb - const_1, 0.0, 1.0)) * const_2, 0.0, 1.0);
#endif

if (u_Local1.g <= 0.0)
{// Bloom rays get amplified...
	volumeLight.rgb *= 3.0;
}

#ifdef DEBUG
	gl_FragColor.rgb = volumeLight;
	gl_FragColor.a = 1.0;
	return;
#endif

#ifdef RANDOMIZE_PIXELS
	volumeLight *= rand(var_ScreenTex * length(volumeLight.rgb) * u_Local1.r);
#endif

	gl_FragColor = vec4(diffuseColor + volumeLight, 1.0);
}
