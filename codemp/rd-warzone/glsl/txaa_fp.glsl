// Temporal AA based on Epic Games' implementation:
// https://de45xmedrsdbp.cloudfront.net/Resources/files/TemporalAA_small-59732822.pdf
// 

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
uniform sampler2D							u_GlowMap;			// Previous Output Image
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2								u_Dimensions;

varying vec2								var_TexCoords;

// YUV-RGB conversion routine from Hyper3D
vec3 encodePalYuv(vec3 rgb)
{
	rgb = pow(rgb, vec3(2.0)); // gamma correction
	return vec3(
		dot(rgb, vec3(0.299, 0.587, 0.114)),
		dot(rgb, vec3(-0.14713, -0.28886, 0.436)),
		dot(rgb, vec3(0.615, -0.51499, -0.10001))
	);
}

vec3 decodePalYuv(vec3 yuv)
{
	vec3 rgb = vec3(
		dot(yuv, vec3(1., 0., 1.13983)),
		dot(yuv, vec3(1., -0.39465, -0.58060)),
		dot(yuv, vec3(1., 2.03211, 0.))
	);
	return pow(rgb, vec3(1.0 / 2.0)); // gamma correction
}


void main( void )
{
	vec2 uv = var_TexCoords.xy;
	vec4 lastColor = texture(u_GlowMap, uv);

	vec3 antialiased = lastColor.xyz;
	float mixRate = min(lastColor.w, 0.5);

	vec2 off = /*1.0*/0.5 / u_Dimensions.xy;
	vec3 in0 = texture(u_DiffuseMap, uv).xyz;

	antialiased = mix(antialiased * antialiased, in0 * in0, mixRate);
	antialiased = sqrt(antialiased);

	vec3 in1 = texture(u_DiffuseMap, uv + vec2(+off.x, 0.0)).xyz;
	vec3 in2 = texture(u_DiffuseMap, uv + vec2(-off.x, 0.0)).xyz;
	vec3 in3 = texture(u_DiffuseMap, uv + vec2(0.0, +off.y)).xyz;
	vec3 in4 = texture(u_DiffuseMap, uv + vec2(0.0, -off.y)).xyz;
	vec3 in5 = texture(u_DiffuseMap, uv + vec2(+off.x, +off.y)).xyz;
	vec3 in6 = texture(u_DiffuseMap, uv + vec2(-off.x, +off.y)).xyz;
	vec3 in7 = texture(u_DiffuseMap, uv + vec2(+off.x, -off.y)).xyz;
	vec3 in8 = texture(u_DiffuseMap, uv + vec2(-off.x, -off.y)).xyz;

	antialiased = encodePalYuv(antialiased);
	in0 = encodePalYuv(in0);
	in1 = encodePalYuv(in1);
	in2 = encodePalYuv(in2);
	in3 = encodePalYuv(in3);
	in4 = encodePalYuv(in4);
	in5 = encodePalYuv(in5);
	in6 = encodePalYuv(in6);
	in7 = encodePalYuv(in7);
	in8 = encodePalYuv(in8);

	vec3 minColor = min(min(min(in0, in1), min(in2, in3)), in4);
	vec3 maxColor = max(max(max(in0, in1), max(in2, in3)), in4);
	minColor = mix(minColor,
	   min(min(min(in5, in6), min(in7, in8)), minColor), 0.5);
	maxColor = mix(maxColor,
	   max(max(max(in5, in6), max(in7, in8)), maxColor), 0.5);

	vec3 preclamping = antialiased;
	antialiased = clamp(antialiased, minColor, maxColor);

	mixRate = 1.0 / (1.0 / mixRate + 1.0);

	vec3 diff = antialiased - preclamping;
	float clampAmount = dot(diff, diff);

	mixRate += clampAmount * 4.0;
	mixRate = clamp(mixRate, 0.05, 0.5);

	antialiased = decodePalYuv(antialiased);

	gl_FragColor = vec4(antialiased, mixRate);
}
