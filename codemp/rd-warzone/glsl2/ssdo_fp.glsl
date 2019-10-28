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
uniform sampler2D			u_DiffuseMap;			// Screen
uniform sampler2D			u_PositionMap;			// Positions
uniform sampler2D			u_NormalMap;			// Normals
//uniform sampler2D			u_ScreenDepthMap;		// Depth
uniform sampler2D			u_DeluxeMap;			// Noise
#endif //defined(USE_BINDLESS_TEXTURES)

//uniform vec3				u_SsdoKernel[32];
uniform vec4				u_ViewInfo;				// znear, zfar, zfar / znear, fov
uniform vec2				u_Dimensions;
uniform vec3				u_ViewOrigin;
uniform vec4				u_PrimaryLightOrigin;
uniform float				u_Time;

uniform vec4				u_Local0;
uniform vec4				u_Local1;

#define HStep				u_Local0.r
#define VStep				u_Local0.g
#define baseRadius			u_Local0.b				// VALUE="0.279710" MIN="0.000100" MAX="0.000000"
#define maxOcclusionDist	u_Local0.a				// VALUE="0.639419" MIN="0.000100" MAX="0.000000"

varying vec2   				var_TexCoords;

out vec4										out_Glow;
out vec4										out_Normal;

#define znear				u_ViewInfo.r			//camera clipping start
#define zfar				u_ViewInfo.g			//camera clipping end

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



vec2 px = vec2(1.0) / u_Dimensions;


vec2 hash22(vec2 p){
	p  = fract(p * vec2(5.3983, 5.4427));
    p += dot(p.yx, p.xy +  vec2(21.5351, 14.3137));
	return fract(vec2(p.x * p.y * 95.4337, p.x * p.y * 97.597));
}

vec4 dssdo_accumulate(vec2 tex, inout vec3 occlusion, inout vec3 illumination)
{
#define num_samples 16//32

	vec4 pMap  = texture(u_PositionMap, tex);

	if (pMap.a-1.0 == 1024.0 || pMap.a-1.0 == 1025.0)
	{// Skybox... Skip...
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	vec3 center_pos  = pMap.xyz;
	vec3 eye_pos = u_ViewOrigin.xyz;

	float center_depth  = distance(eye_pos, center_pos);

	float radius = baseRadius / center_depth;
	float max_distance_inv = 1.f / maxOcclusionDist;
	float attenuation_angle_threshold = 0.1;

	vec3 center_normal = texture(u_NormalMap, tex).xyz;
	center_normal = -DecodeNormal(center_normal.xy);

	const float fudge_factor_l0 = 2.0;
	const float fudge_factor_l1 = 10.0;

	const vec3 sh2_weight_l1 = vec3(fudge_factor_l1 * 0.48860);
	const vec3 sh2_weight = sh2_weight_l1 / num_samples;

	for( int i=0; i < num_samples; ++i )
	{
		vec2 textureOffset;
		textureOffset = hash22((pMap.xy * pMap.z * 0.000001) + vec2(i) + u_Time);
		textureOffset = textureOffset * 2.0 - 1.0;
		textureOffset = textureOffset * center_depth * px * radius * float(i+1);

		vec2 sample_tex = tex + textureOffset;
		vec4 pMap2 = textureLod(u_PositionMap, vec2(sample_tex), 0.0);
		
		if (pMap2.a-1.0 == 1024.0 || pMap2.a-1.0 == 1025.0)
		{// Skip sky/sun/player hits...
			continue;
		}

		vec3 sample_pos = pMap2.xyz;
		vec3 center_to_sample = sample_pos - center_pos;
		float dist = length(center_to_sample);
		vec3 center_to_sample_normalized = center_to_sample / dist;
		float attenuation = 1.0 - clamp(dist * max_distance_inv, 0.0, 1.0);
		float dp = dot(center_normal, center_to_sample_normalized);

		attenuation = attenuation*attenuation * step(attenuation_angle_threshold, dp);

		occlusion += attenuation * sh2_weight * center_to_sample_normalized;
		illumination += (1.0 - attenuation) * sh2_weight.xyz * texture(u_DiffuseMap, sample_tex).rgb;
	}

	illumination /= float(num_samples) * 2.0;
}

void main() 
{
	vec3 illumination = vec3(0.0);
	vec3 occlusion = vec3(0.0);

	dssdo_accumulate(var_TexCoords, occlusion, illumination).xyz;

	gl_FragColor = vec4(occlusion * 0.5 + 0.5, 1.0);
	out_Glow = vec4(illumination, 1.0);
}

