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
uniform sampler2D					u_ScreenDepthMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2						u_Dimensions;
uniform vec2						u_InvTexRes;

uniform vec4						u_Local0;

varying vec2						var_TexCoords;

void main ()
{
	// Scans the whole depthmap image from the current frame and writes a single pixel value of the max depth seen in the previous frame...
	float maxDepth = 0.0;

	for (float x = 0.0; x <= 1.0; x += u_InvTexRes.x)
	{
		for (float y = 0.0; y <= 1.0; y += u_InvTexRes.y)
		{
			float depth = texture(u_ScreenDepthMap, vec2(x,y)).r;

			if (depth < 1.0 && depth > maxDepth)
			{// Sky is never added...
				maxDepth = depth;
			}
		}
	}

	// Adds a little (0.01) extra depth to the max depth to let the max range accumulate again over future frames and too add a little buffer...
	maxDepth = clamp(maxDepth + 0.01/*0.000001*/, 0.0, 1.0);
	gl_FragColor = vec4(maxDepth, maxDepth, maxDepth, 1.0);
}
