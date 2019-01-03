uniform sampler2D			u_DiffuseMap;			// Screen
uniform sampler2D			u_PositionMap;			// Positions
uniform sampler2D			u_NormalMap;			// Normals
//uniform sampler2D			u_ScreenDepthMap;		// Depth
uniform sampler2D			u_DeluxeMap;			// Noise

uniform vec3				u_SsdoKernel[32];
uniform vec4				u_ViewInfo;				// znear, zfar, zfar / znear, fov
uniform vec2				u_Dimensions;
uniform vec3				u_ViewOrigin;
uniform vec4				u_PrimaryLightOrigin;

uniform vec4				u_Local0;
uniform vec4				u_Local1;

#define HStep				u_Local0.r
#define VStep				u_Local0.g
#define baseRadius			u_Local0.b				// VALUE="0.279710" MIN="0.000100" MAX="0.000000"
#define maxOcclusionDist	u_Local0.a				// VALUE="0.639419" MIN="0.000100" MAX="0.000000"

varying vec2   				var_TexCoords;

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

// 'threshold' is constant , 'value' is smoothly varying
float aastep(float threshold, float value) 
{
	float afwidth = length(vec2(dFdx(value), dFdy(value))) * 0.70710678118654757;
	return smoothstep(threshold-afwidth, threshold+afwidth, value);
}

vec4 dssdo_accumulate(vec2 tex)
{
	vec3 points[32] = vec3[]
	(
		vec3(-0.134, 0.044, -0.825),
		vec3(0.045, -0.431, -0.529),
		vec3(-0.537, 0.195, -0.371),
		vec3(0.525, -0.397, 0.713),
		vec3(0.895, 0.302, 0.139),
		vec3(-0.613, -0.408, -0.141),
		vec3(0.307, 0.822, 0.169),
		vec3(-0.819, 0.037, -0.388),
		vec3(0.376, 0.009, 0.193),
		vec3(-0.006, -0.103, -0.035),
		vec3(0.098, 0.393, 0.019),
		vec3(0.542, -0.218, -0.593),
		vec3(0.526, -0.183, 0.424),
		vec3(-0.529, -0.178, 0.684),
		vec3(0.066, -0.657, -0.570),
		vec3(-0.214, 0.288, 0.188),
		vec3(-0.689, -0.222, -0.192),
		vec3(-0.008, -0.212, -0.721),
		vec3(0.053, -0.863, 0.054),
		vec3(0.639, -0.558, 0.289),
		vec3(-0.255, 0.958, 0.099),
		vec3(-0.488, 0.473, -0.381),
		vec3(-0.592, -0.332, 0.137),
		vec3(0.080, 0.756, -0.494),
		vec3(-0.638, 0.319, 0.686),
		vec3(-0.663, 0.230, -0.634),
		vec3(0.235, -0.547, 0.664),
		vec3(0.164, -0.710, 0.086),
		vec3(-0.009, 0.493, -0.038),
		vec3(-0.322, 0.147, -0.105),
		vec3(-0.554, -0.725, 0.289),
		vec3(0.534, 0.157, -0.250)
	);

#define num_samples 32

	vec2 noise_texture_size = vec2(4,4);
	vec4 pMap  = texture(u_PositionMap, tex);

	if (pMap.a-1.0 == 1024.0 || pMap.a-1.0 == 1025.0 /*|| pMap.a-1.0 == 21 || pMap.a-1.0 == 16 || pMap.a-1.0 == 30 || pMap.a-1.0 == 25*/)
	{// Skybox... Skip...
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	vec3 center_pos  = pMap.xyz;
	vec3 eye_pos = u_ViewOrigin.xyz;

	float center_depth  = distance(eye_pos, center_pos);

	float radius = baseRadius / center_depth;
	float max_distance_inv = 1.f / maxOcclusionDist;
	float attenuation_angle_threshold = 0.1;

	vec3 noise = texture(u_DeluxeMap, tex*u_Dimensions.xy/noise_texture_size).xyz*2-1;

	//radius = min(radius, 0.1);

	vec3 center_normal = texture(u_NormalMap, tex).xyz;
	center_normal = DecodeNormal(center_normal.xy);

	vec4 occlusion_sh2 = vec4(0.0);

	const float fudge_factor_l0 = 2.0;
	const float fudge_factor_l1 = 10.0;

	const float sh2_weight_l0 = fudge_factor_l0 * 0.28209; //0.5*sqrt(1.0/pi);
	const vec3 sh2_weight_l1 = vec3(fudge_factor_l1 * 0.48860); //0.5*sqrt(3.0/pi);

	const vec4 sh2_weight = vec4(sh2_weight_l1, sh2_weight_l0) / num_samples;

//#pragma unroll num_samples
	for( int i=0; i < num_samples; ++i )
	{
	    vec2 textureOffset = reflect( points[ i ].xy, noise.xy ).xy * radius;
		vec2 sample_tex = tex + textureOffset;
		vec4 pMap2 = textureLod(u_PositionMap, vec2(sample_tex), 0.0);
		
		if (pMap2.a-1.0 == 1024.0 || pMap2.a-1.0 == 1025.0 || pMap2.a-1.0 == 21 || pMap2.a-1.0 == 16 || pMap2.a-1.0 == 30 /*|| pMap2.a-1.0 == 25*/)
		{// Skip sky/sun/player hits...
			continue;
		}

		vec3 sample_pos = pMap2.xyz;
		vec3 center_to_sample = sample_pos - center_pos;
		float dist = length(center_to_sample);
		vec3 center_to_sample_normalized = center_to_sample / dist;
		float attenuation = 1.0 - clamp(dist * max_distance_inv, 0.0, 1.0);
		float dp = dot(center_normal, center_to_sample_normalized);

		attenuation = attenuation*attenuation * /*aastep*/step(attenuation_angle_threshold, dp);

		occlusion_sh2 += attenuation * sh2_weight * vec4(center_to_sample_normalized, 1.0);
	}

	return vec4(occlusion_sh2.rgb, 1.0);
}

void main() 
{
	gl_FragColor = dssdo_accumulate(var_TexCoords);
}

