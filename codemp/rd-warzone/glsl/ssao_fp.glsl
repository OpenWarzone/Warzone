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
uniform sampler2D   u_DiffuseMap;		// screen diffuse map
uniform sampler2D   u_ScreenDepthMap;	// depth
uniform sampler2D   u_PositionMap;		// position map
uniform sampler2D   u_NormalMap;		// normal map
uniform sampler2D   u_OverlayMap;		// detailed normal map
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2        u_Dimensions;
uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec3		u_ViewOrigin;
uniform vec4		u_PrimaryLightOrigin;

uniform vec4		u_Local0; // r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value
uniform vec4		u_Local1; // mCameraForward.x, y, z

varying vec2	var_ScreenTex;
varying vec3	var_Position;


#ifdef __USE_REAL_NORMALMAPS__
//#define __USE_DETAIL_NORMALS__ // Not needed. Waste of time...
#endif //__USE_REAL_NORMALMAPS__

#ifdef __USE_DETAIL_NORMALS__
#define __FAST_NORMAL_DETAIL__
#endif //__USE_DETAIL_NORMALS__

#define NUM_OCCLUSION_CHECKS 8
//#define NUM_OCCLUSION_CHECKS 16


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

vec4 normalVector(vec3 color) {
	vec4 normals = vec4(color.rgb, length(color.rgb) / 3.0);
	normals.rgb = vec3(length(normals.r - normals.a), length(normals.g - normals.a), length(normals.b - normals.a));

	// Contrast...
#define normLower ( 32.0 / 255.0 )
#define normUpper (255.0 / 212.0 )
	vec3 N = clamp((clamp(normals.rgb - normLower, 0.0, 1.0)) * normUpper, 0.0, 1.0);

	return vec4(vec3(1.0) - (normalize(pow(N, vec3(4.0))) * 0.5 + 0.5), 1.0 - normals.a);
}

const vec3 unKernel[32] = vec3[]
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

vec3 vLocalSeed;

// This function returns random number from zero to one
float randZeroOne()
{
	uint n = floatBitsToUint(vLocalSeed.y * 214013.0 + vLocalSeed.x * 2531011.0 + vLocalSeed.z * 141251.0);
	n = n * (n * n * 15731u + 789221u);
	n = (n >> 9u) | 0x3F800000u;

	float fRes = 2.0 - uintBitsToFloat(n);
	vLocalSeed = vec3(vLocalSeed.x + 147158.0 * fRes, vLocalSeed.y*fRes + 415161.0 * fRes, vLocalSeed.z + 324154.0*fRes);
	return fRes;
}

float ssao( in vec3 position, in vec2 pixel, in vec3 normal, in float resolution, in float strength, in float minDistance, in float maxDisance )
{
    vec2  uv  = pixel;
    float z   = texture2D( u_ScreenDepthMap, uv ).x;		// read eye linear z

	if (z >= 1.0)
	{// Sky...
		return 1.0;
	}

	vec3 light = normalize(position.xyz - u_PrimaryLightOrigin.xyz);

	vec2  res = vec2(resolution) / u_Dimensions.xy;
	float numOcclusions = 0.0;

	vLocalSeed = position;

	vec3 ref = unKernel[int(randZeroOne() * 32.0)];
	//vec3 ref = vec3(u_Local0.r);

    // accumulate occlusion
    float bl = 0.0;
    for( int i = 0; i < NUM_OCCLUSION_CHECKS; i++ )
    {
		vec3 of = faceforward( reflect( unKernel[/*i*/int(randZeroOne() * 32.0)], ref ), light, normal );
		vec2 thisUV = uv + (res * of.xy);

		if (thisUV.x > 1.0 || thisUV.y > 1.0 || thisUV.x < 0.0 || thisUV.y < 0.0)
		{// Don't sample outside of screen bounds...
			float zd = 0.5;
			bl += clamp(zd*10.0, 0.1, 1.0)*(1.0 - clamp((zd - 1.0) / 5.0, 0.0, 1.0));
			numOcclusions += 1.0;
			continue;
		}

		float sz = texture2D( u_ScreenDepthMap, thisUV).x;
		float zd = (sz-z)*strength;

		if (length(sz - z) < minDistance)
		{
			zd = 0.5;
			bl += clamp(zd*10.0,0.1,1.0)*(1.0-clamp((zd-1.0)/5.0,0.0,1.0));
			numOcclusions += 1.0;
			continue;
		}
		else if (length(sz - z) > maxDisance)
		{
			zd = 0.0;
			bl += clamp(zd*10.0,0.1,1.0)*(1.0-clamp((zd-1.0)/5.0,0.0,1.0));
			numOcclusions += 1.0;
			continue;
		}
		else
		{
			bl += clamp(zd*10.0,0.1,1.0)*(1.0-clamp((zd-1.0)/5.0,0.0,1.0));
			numOcclusions += 1.0;
		}
    }

	float ao = clamp(bl/float(numOcclusions), 0.0, 1.0);
	ao = mix(ao, 1.0, z);

    return ao;
}

void main( void ) 
{
	vec4 position = textureLod(u_PositionMap, var_ScreenTex, 0.0);

	if (position.a-1.0 == MATERIAL_SKY || position.a-1.0 == MATERIAL_SUN || position.a-1.0 == MATERIAL_GLASS || position.a-1.0 == MATERIAL_NONE)
	{// Skybox... Skip...
		gl_FragColor=vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}

	vec4 norm = textureLod(u_NormalMap, var_ScreenTex, 0.0);
	norm.xyz = DecodeNormal(norm.xy);

#ifdef __USE_DETAIL_NORMALS__
	if (u_Local0.r > 0.0)
	{// Use detail normals...
		vec4 normalDetail = textureLod(u_OverlayMap, var_ScreenTex, 0.0);

		if (normalDetail.a < 1.0)
		{// Don't have real normalmap, make normals for this pixel...
#ifdef __FAST_NORMAL_DETAIL__
			normalDetail = normalVector(texture(u_DiffuseMap, var_ScreenTex).rgb);
#else //!__FAST_NORMAL_DETAIL__
			normalDetail = normalVector(var_ScreenTex);
#endif //__FAST_NORMAL_DETAIL__
		}

		normalDetail.rgb = normalize(clamp(normalDetail.xyz, 0.0, 1.0) * 2.0 - 1.0);
		norm.rgb = normalize(mix(norm.xyz, normalDetail.xyz, 0.25 * (length(norm.xyz - normalDetail.xyz) / 3.0)));
	}
#endif //__USE_DETAIL_NORMALS__

	vec3 N = normalize(norm.xyz);

	float msao = ssao( position.xyz, var_ScreenTex, N.xyz, 32.0, 64.0, 0.001, 0.01 );
	float sao = clamp(msao, 0.0, 1.0);
	gl_FragColor=vec4(sao, 0.0, 0.0, 1.0);
}
