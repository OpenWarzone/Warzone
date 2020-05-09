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
uniform sampler2D		u_DiffuseMap;
uniform sampler2D		u_ScreenDepthMap;
#endif //defined(USE_BINDLESS_TEXTURES)


uniform vec4			u_ViewInfo; // zmin, zmax, zmax / zmin
uniform float			u_ShadowZfar[5];
uniform vec2			u_Dimensions;
uniform vec4			u_Settings0;

uniform vec4			u_Local0; // r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value

varying vec2			var_TexCoords;

#define BLUR_RADIUS		u_Settings0.r
#define BLUR_PIXELMULT	u_Settings0.g
#define BLUR_DIRECTION	u_Settings0.b

#define px (1.0/u_Dimensions.x)
#define py (1.0/u_Dimensions.y)

void main()
{
	float numValues = 1.0;
	vec2 shadowInfo = texture(u_DiffuseMap, var_TexCoords.xy).xy;
	float color = shadowInfo.x;
	float depth = shadowInfo.y;
	float inColor = color;

	depth = clamp(pow(1.0 - depth, 4.0), 0.0, 1.0);

	for (float x = -BLUR_RADIUS; x <= BLUR_RADIUS; x += BLUR_PIXELMULT)
	{
		for (float y = -BLUR_RADIUS; y <= BLUR_RADIUS; y += BLUR_PIXELMULT)
		{
			vec2 offset = vec2(x * px, y * py) * 2.0 * depth;
			vec2 xy = vec2(var_TexCoords + offset);
			color += texture(u_DiffuseMap, xy).x;
			numValues += 1.0;
		}
	}

	color /= numValues;

#define contrastLower ( -16.0 / 255.0 )
#define contrastUpper ( 255.0 / 295.0 )
	color = clamp((clamp(color - contrastLower, 0.0, 1.0)) * contrastUpper, 0.0, 1.0);

	gl_FragColor = vec4(color, 0.0, 0.0, 1.0);
}
