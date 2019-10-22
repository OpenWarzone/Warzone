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
uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_HeightMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4		u_Color;

uniform vec3		u_ViewOrigin;

uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_PrimaryLightColor;

uniform int			u_lightCount;
uniform vec3		u_lightPositions2[MAX_DEFERRED_LIGHTS];
uniform vec3		u_lightColors[MAX_DEFERRED_LIGHTS];
uniform float		u_lightDistances[MAX_DEFERRED_LIGHTS];

uniform vec4		u_Mins;					// MAP_MINS[0], MAP_MINS[1], MAP_MINS[2], 0.0
uniform vec4		u_Maxs;					// MAP_MAXS[0], MAP_MAXS[1], MAP_MAXS[2], 0.0

uniform vec4		u_Local0; // normals, outputBuffers

out vec4			out_Glow;
out vec4			out_Position;
out vec4			out_Normal;
#ifdef __USE_REAL_NORMALMAPS__
out vec4			out_NormalDetail;
#endif //__USE_REAL_NORMALMAPS__


varying vec2		var_Tex1;
varying vec3		var_Position;
//varying vec3		var_Normal;
//varying vec4		var_Color;

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

vec2 GetMapTC(vec3 pos)
{
	vec2 mapSize = u_Maxs.xy - u_Mins.xy;
	return (pos.xy - u_Mins.xy) / mapSize;
}

float GetHeightMap(vec3 pos)
{
	if (pos.x < u_Mins.x || pos.y < u_Mins.y || pos.z < u_Mins.z) return 65536.0;
	if (pos.x > u_Maxs.x || pos.y > u_Maxs.y || pos.z > u_Maxs.z) return 65536.0;

	float h = textureLod(u_HeightMap, GetMapTC(pos), 1.0).r;
	h = mix(u_Mins.z, u_Maxs.z, h);
	return h + 64.0; // raise it up to correct loss of precision. it's only weather anyway...
}

void main()
{
	float hMap = GetHeightMap(var_Position.xyz);

	if (var_Position.z >= hMap)
	{// Above heightmap, so draw the particle...
		vec4 color = texture2D(u_DiffuseMap, var_Tex1);

		if (color.a > 0.0)
		{
			/*{// Add sunlight effect to particles in the direction of of the sun... Hmm ok forget this, it works but doesnt account for sun below horizon and i dont want to do a shadow lookup.
				vec3 v3 = normalize(u_PrimaryLightOrigin.xyz - var_Position.xyz);
				vec3 v2 = normalize(var_Position.xyz - u_ViewOrigin.xyz);
				float l = dot(v3, v2);
				//l = max(3.0-(l*l * .02), 0.0);
				l = max(pow(l, 80.0), 0.0);
				color.rgb += l * u_PrimaryLightColor;
			}*/

			for (int t = 0; t < u_lightCount; t++)
			{// Add emissive light effect to particles in the direction of of the sun...
				float dist = u_lightDistances[t];//distance(u_ViewOrigin.xyz, u_lightPositions2[t].xyz);
				vec3 v3 = normalize(u_lightPositions2[t].xyz - var_Position.xyz);
				vec3 v2 = normalize(var_Position.xyz - u_ViewOrigin.xyz);
				float l = max(dot(v3, v2), 0.0);
				l = max(pow(l, (dist*0.25)), 0.0) * (8.0 / (dist*0.005));
				color.rgb += l * u_lightColors[t];
			}
		}

		gl_FragColor = color;
		gl_FragColor.a = clamp(gl_FragColor.a * 3.0, 0.0, 1.0);
	}
	else
	{// Culled by heightmap...
		gl_FragColor = vec4(0.0);
		out_Glow = vec4(0.0);
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
		out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
		return;
	}

	out_Glow = vec4(0.0);

	if (u_Local0.a > 0.0)
	{
		out_Position = vec4(var_Position.xyz, MATERIAL_GREENLEAVES + 1.0);
		out_Normal = vec4(vec3(EncodeNormal(/*var_Normal.xyz*/u_Local0.xyz), 0.0), 1.0);
#ifdef __USE_REAL_NORMALMAPS__
		out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
	}
	else
	{
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
		out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
	}
}
