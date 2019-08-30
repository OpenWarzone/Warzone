//#define TEST_PARALLAX

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
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_RoadMap;
uniform sampler2D					u_PositionMap;
uniform sampler2D					u_NormalMap;
uniform sampler2D					u_ScreenDepthMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2					u_Dimensions;

uniform vec4					u_Local1; // DISPLACEMENT_MAPPING_STRENGTH, r_testShaderValue1, r_testShaderValue2, r_testShaderValue3

uniform vec4					u_ViewInfo; // znear, zfar, zfar / znear, fov
uniform vec3					u_ViewOrigin;

varying vec2					var_TexCoords;

#define DISPLACEMENT_STRENGTH	u_Local1.r

vec2 px = vec2(1.0) / u_Dimensions.xy;

//#define __ENCODE_NORMALS_RECONSTRUCT_Z__
#define __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
//#define __ENCODE_NORMALS_CRY_ENGINE__
//#define __ENCODE_NORMALS_EQUAL_AREA_PROJECTION__

#ifdef __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
vec2 EncodeNormal(vec3 n)
{
	float scale = 1.7777;
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;
	return enc;
}
vec3 DecodeNormal(vec2 enc)
{
	vec3 enc2 = vec3(enc.xy, 0.0);
	float scale = 1.7777;
	vec3 nn =
		enc2.xyz*vec3(2.0 * scale, 2.0 * scale, 0.0) +
		vec3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	return vec3(g * nn.xy, g - 1.0);
}
#elif defined(__ENCODE_NORMALS_CRY_ENGINE__)
vec3 DecodeNormal(in vec2 N)
{
	vec2 encoded = N * 4.0 - 2.0;
	float f = dot(encoded, encoded);
	float g = sqrt(1.0 - f * 0.25);
	return vec3(encoded * g, 1.0 - f * 0.5);
}
vec2 EncodeNormal(in vec3 N)
{
	float f = sqrt(8.0 * N.z + 8.0);
	return N.xy / f + 0.5;
}
#elif defined(__ENCODE_NORMALS_EQUAL_AREA_PROJECTION__)
vec2 EncodeNormal(vec3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);
	return n.xy / f + 0.5;
}
vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 n;
	n.xy = fenc*g;
	n.z = 1.0 - f / 2.0;
	return n;
}
#else //__ENCODE_NORMALS_RECONSTRUCT_Z__
vec3 DecodeNormal(in vec2 N)
{
	vec3 norm;
	norm.xy = N * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy));
	return norm;
}
vec2 EncodeNormal(vec3 n)
{
	return vec2(n.xy * 0.5 + 0.5);
}
#endif //__ENCODE_NORMALS_RECONSTRUCT_Z__

void main(void)
{
#ifndef TEST_PARALLAX
	vec3 dMap = texture(u_RoadMap, var_TexCoords).rgb;
	vec4 color = vec4(texture(u_DiffuseMap, var_TexCoords).rgb, 1.0);

	if (dMap.r <= 0.0)
	{
		gl_FragColor = color;
		return;
	}
	
	vec2 texCoords = var_TexCoords;
	float invDepth = clamp((1.0 - texture(u_ScreenDepthMap, texCoords).r) * 2.0 - 1.0, 0.0, 1.0);

	if (invDepth <= 0.0)
	{
		gl_FragColor = color;
		return;
	}

	//invDepth = clamp(length(invDepth * 1.75 - 0.75), 0.0, 1.0);

	float material = texture(u_PositionMap, var_TexCoords).a - 1.0;
	float materialMultiplier = 1.0;

	if (material == MATERIAL_ROCK || material == MATERIAL_STONE || material == MATERIAL_SKYSCRAPER)
	{// Rock gets more displacement...
		materialMultiplier = 3.0;
	}
	else if (material == MATERIAL_TREEBARK)
	{// Rock gets more displacement...
		materialMultiplier = 1.5;
	}
	//const float materialMultiplier = 1.0;

	vec3 norm = vec3(dMap.gb, 0.0) * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy)); // reconstruct Z from X and Y

	vec2 distFromCenter = vec2(length(texCoords.x - 0.5), length(texCoords.y - 0.5));
	float displacementStrengthMod = ((DISPLACEMENT_STRENGTH * materialMultiplier) / 18.0); // Default is 18.0. If using higher displacement, need more screen edge flattening, if less, less flattening.
	float screenEdgeScale = clamp(max(distFromCenter.x, distFromCenter.y) * 2.0, 0.0, 1.0);
	screenEdgeScale = 1.0 - pow(screenEdgeScale, 16.0/displacementStrengthMod);

	float finalModifier = invDepth;

	float offset = -DISPLACEMENT_STRENGTH * materialMultiplier * finalModifier * screenEdgeScale * dMap.r;


	texCoords += norm.xy * px * offset;
	//color = vec4(texture(u_DiffuseMap, texCoords).rgb * shadow, 1.0);

	vec3 col = vec3(0.0);
	float steps = min(DISPLACEMENT_STRENGTH * materialMultiplier, 4.0);
	vec2 dir = (texCoords - var_TexCoords) / steps;
	col = texture(u_DiffuseMap, texCoords).rgb;

	for (float x = steps; x > 0.0; x -= 1.0)
	{
		col += texture(u_DiffuseMap, var_TexCoords + (dir*x)).rgb;
	}
	col /= steps+1.0;
	color = vec4(col, 1.0);

	//float shadow = 1.0 - clamp(dMap.r, 0.0, 1.0);
	//shadow = pow(shadow, 8.0);
	//shadow = 1.0 - (shadow * finalModifier);
	//shadow = clamp(shadow * 0.75 + 0.25, 0.0, 1.0);
	//color.rgb *= shadow;

	gl_FragColor = color;
#else //!TEST_PARALLAX
	vec3 parallax = texture(u_RoadMap, var_TexCoords).rgb;

	if (length(parallax.xy) == 0.0)
	{
		gl_FragColor = vec4(texture(u_DiffuseMap, var_TexCoords).rgb, 1.0);
	}
	else
	{
		gl_FragColor = vec4(texture(u_DiffuseMap, var_TexCoords+(parallax.xy*2.0-1.0)).rgb * parallax.z, 1.0);
	}
#endif //TEST_PARALLAX
}
