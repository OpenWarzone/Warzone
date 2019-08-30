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
uniform vec4	u_Local0; // scan_pixel_size_x, scan_pixel_size_y, scan_width, is_ssgi

varying vec2	var_TexCoords;

vec2 PIXEL_OFFSET = vec2(1.0 / u_Dimensions.x, 1.0 / u_Dimensions.y);

//#define scale u_Local0.xyz
const vec3 scale = vec3(1.0, 0.0, 6.0/*16.0*/);

void main(void)
{
	float NUM_VALUES = 1.0;

	vec3 col0 = texture2D(u_DiffuseMap, var_TexCoords.xy).rgb;
	gl_FragColor.rgb = col0.rgb;

	for (float width = 1.0; width <= scale.z; width += 1.0)
	{
		float dist_mult = clamp(1.0 - clamp(width / scale.z, 0.0, 1.0), 0.1, 1.0);
		vec3 col1 = texture2D(u_DiffuseMap, var_TexCoords.xy + ((scale.xy * width) * PIXEL_OFFSET)).rgb;
		vec3 col2 = texture2D(u_DiffuseMap, var_TexCoords.xy - ((scale.xy * width) * PIXEL_OFFSET)).rgb;
		//vec3 add_color = ((col0 / 2.0) + ((col1 + col2) * (dist_mult * 2.0))) / 4.0;
		vec3 add_color = clamp((col1 + col2) / 2.0, 0.0, 1.0) * dist_mult;

		//vec3 BLUE_SHIFT_MOD = vec3(0.333, 0.333, 3.0);
		//vec3 add_color_blue = clamp(add_color * (BLUE_SHIFT_MOD * (1.0 - clamp(dist_mult*3.5, 0.0, 1.0))), 0.0, 1.0);
		//add_color.rgb += clamp(((add_color + add_color + add_color_blue) * 0.37), 0.0, 1.0);

		gl_FragColor.rgb += add_color;
		NUM_VALUES += 1.0;
	}

	gl_FragColor.rgb /= NUM_VALUES;
	gl_FragColor.a	= 1.0;
}
