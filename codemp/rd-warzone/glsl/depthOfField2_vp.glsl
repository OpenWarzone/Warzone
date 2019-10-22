attribute vec3		attr_Position;
attribute vec4		attr_TexCoord0;

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
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2D					u_SpecularMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4						u_Local0; // dofValue, 0, 0, 0
uniform vec4						u_Local1; // testvalues

varying vec2						var_TexCoords;
flat varying float					var_FocalDepth;


float GetFocalDepth()
{
	float depthsum = clamp(texture(u_SpecularMap, vec2(0.5, 0.5)).x + 0.1, 0.0, 1.0) * 0.999;
	//depthsum = pow(clamp(depthsum * 2.25, 0.0, 1.0), 1.15);
	//depthsum = pow(clamp(depthsum * u_Local1.r, 0.0, 1.0), u_Local1.g);
	depthsum = pow(clamp(depthsum * 1.25, 0.0, 1.0), 0.5);
	return depthsum; 
}

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
	var_FocalDepth = GetFocalDepth();
}
