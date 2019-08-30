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
uniform sampler2D u_DiffuseMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2	u_Dimensions;

varying vec2	var_TexCoords;

#define BLOOM_BLUR_WIDTH 3.0

void main(void)
{
  vec2 PIXEL_OFFSET = vec2(1.0 / u_Dimensions);
  const vec2 X_AXIS = vec2(1.0, 0.0);
  const vec2 Y_AXIS = vec2(0.0, 1.0);

  vec3 color = vec3(0.0, 0.0, 0.0);
  vec3 col0 = texture2D(u_DiffuseMap, var_TexCoords.xy).rgb;

  for (float width = 0.0; width < BLOOM_BLUR_WIDTH; width += 1.0)
  {
	vec3 col1 = texture2D(u_DiffuseMap, var_TexCoords.xy + ((X_AXIS.xy * width) * PIXEL_OFFSET)).rgb;
	vec3 col2 = texture2D(u_DiffuseMap, var_TexCoords.xy - ((X_AXIS.xy * width) * PIXEL_OFFSET)).rgb;
	color.rgb += (col0 / 2.0) + (col1 + col2) / 4.0;
  }

  color.rgb /= BLOOM_BLUR_WIDTH;

  vec3 color2 = vec3(0.0, 0.0, 0.0);

  for (float width = 0.0; width < BLOOM_BLUR_WIDTH; width += 1.0)
  {
	vec3 col1 = texture2D(u_DiffuseMap, var_TexCoords.xy + ((Y_AXIS.xy * width) / u_Dimensions)).rgb;
	vec3 col2 = texture2D(u_DiffuseMap, var_TexCoords.xy - ((Y_AXIS.xy * width) / u_Dimensions)).rgb;
	color2.rgb += (col0 / 2.0) + (col1 + col2) / 4.0;
  }

  color2.rgb /= BLOOM_BLUR_WIDTH;

  gl_FragColor.rgb = (color + color2) / 2.0;
  gl_FragColor.a	= 1.0;
}
