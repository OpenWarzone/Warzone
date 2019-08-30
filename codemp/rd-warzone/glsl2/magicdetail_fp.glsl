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
uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_ScreenDepthMap;
#endif //defined(USE_BINDLESS_TEXTURES)


varying vec2		var_TexCoords;
varying vec2		var_ScreenTex;

uniform vec2		u_Dimensions;
uniform vec4		u_ViewInfo; // zfar / znear, zfar
//uniform mat4		u_invEyeProjectionMatrix;
uniform vec4		u_Local0;

vec2 PixelSize = vec2(1.0f / u_Dimensions.x, 1.0f / u_Dimensions.y);

#define   MAGICDETAIL_STRENGTH u_Local0.x
#define   MAGICDETAIL_MIX u_Local0.y

float GenerateDetail( vec2 fragCoord )
{
	float M =abs(length(texture2D(u_DiffuseMap, fragCoord + vec2(0., 0.)*PixelSize).rgb) / 3.0);
	float L =abs(length(texture2D(u_DiffuseMap, fragCoord + vec2(1.0, 0.)*PixelSize).rgb) / 3.0);
	float R =abs(length(texture2D(u_DiffuseMap, fragCoord + vec2(-1.0, 0.)*PixelSize).rgb) / 3.0);	
	float U =abs(length(texture2D(u_DiffuseMap, fragCoord + vec2(0., 1.0)*PixelSize).rgb) / 3.0);;
	float D =abs(length(texture2D(u_DiffuseMap, fragCoord + vec2(0., -1.0)*PixelSize).rgb) / 3.0);
	float X = ((R-M)+(M-L))*0.5;
	float Y = ((D-M)+(M-U))*0.5;
	
	vec4 N = vec4(normalize(vec3(X, Y, MAGICDETAIL_STRENGTH)), 1.0);

	vec3 col = N.xyz * 0.5 + 0.5;
	return (length(col) / 3.0) * 2.0;
}

float getLinearDepth(sampler2D depthMap, const vec2 tex, const float zFarDivZNear)
{
	return texture2D(depthMap, tex).r;
}

void main()
{
	vec4 inColor = texture2D(u_DiffuseMap, var_TexCoords);
	float inDepth = getLinearDepth(u_ScreenDepthMap, var_TexCoords, u_ViewInfo.x);

	float strength = GenerateDetail(var_TexCoords.xy);
	strength *= (strength * 1.4);
	
	vec4 color = vec4( (inColor.rgb * (strength * (1.0 - inDepth))) + (inColor.rgb * inDepth), 1.0);
	color = clamp(color, 0.0, 1.0);

	gl_FragColor.rgb = mix(inColor.rgb, color.rgb, clamp(MAGICDETAIL_MIX, 0.0, 1.0));
	gl_FragColor.a = 1.0;
}
