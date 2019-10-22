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
uniform sampler2D					u_DiffuseMap;
#endif //defined(USE_BINDLESS_TEXTURES)


uniform vec4						u_ViewInfo; // zmin, zmax, zmax / zmin

varying vec2						var_TexCoords;

//out vec4 out_Color;
out vec4 out_Glow;
out vec4 out_Normal;
out vec4 out_Position;
out vec4 out_NormalDetail;

#define out_Depth512 out_Color
#define out_Depth2048 out_Normal
#define out_Depth4096 out_Position
#define out_Depth8192 out_Glow
#define out_DepthZfar out_NormalDetail

float linearize(float depth, float zmin, float zmax)
{
	return 1.0 / mix(zmax/zmin, 1.0, depth);
}

void main(void)
{
	float depth = texture(u_DiffuseMap, vec2(var_TexCoords.x, 1.0-var_TexCoords.y)).r;
#ifdef INVERSE_DEPTH_BUFFERS
	depth = 1.0 - depth;
#endif //__INVERSE_DEPTH_BUFFERS__
	float depth512 = linearize(depth, u_ViewInfo.r, 512.0);
	float depth2048 = linearize(depth, u_ViewInfo.r, 2048.0);
	float depth4096 = linearize(depth, u_ViewInfo.r, 4096.0);
	float depth8192 = linearize(depth, u_ViewInfo.r, 8192.0);
	float depthZfar = linearize(depth, u_ViewInfo.r, u_ViewInfo.g);

	out_Depth512 = vec4(depth512, depth512, depth512, 1.0);
	out_Depth2048 = vec4(depth2048, depth2048, depth2048, 1.0);
	out_Depth4096 = vec4(depth4096, depth4096, depth4096, 1.0);
	out_Depth8192 = vec4(depth8192, depth8192, depth8192, 1.0);
	out_DepthZfar = vec4(depthZfar, depthZfar, depthZfar, 1.0);
}
