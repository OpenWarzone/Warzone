uniform vec4		u_Local9;
uniform vec4		u_Local10;

uniform sampler2D	u_DiffuseMap;

varying vec2		var_TexCoords;
varying vec3		var_vertPos;
varying vec3		var_Normal;
flat varying float	var_IsWater;

out vec4 out_Glow;
out vec4 out_Normal;
out vec4 out_Position;
#ifdef __USE_REAL_NORMALMAPS__
out vec4 out_NormalDetail;
#endif //__USE_REAL_NORMALMAPS__

// Maximum waves amplitude
#define maxAmplitude u_Local10.g

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

void main()
{
	out_Glow = vec4(0.0);
	out_Normal = vec4(EncodeNormal(var_Normal), 0.0, 1.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
	//if (var_IsWater >= 2.0 && var_IsWater <= 5.0)
	//	out_Color = vec4(texture(u_DiffuseMap, var_TexCoords).rgb, 1.0);
	//else
	//	out_Color = vec4(0.0059, 0.3096, 0.445, 1.0);

	if (var_IsWater == 3.0 || var_IsWater == 4.0)
	{
		float dist = (0.5 - distance(var_TexCoords, vec2(0.5))) * 2.0;
		if (dist > 0.0)
		{
			out_Position = vec4(dist, var_TexCoords.x - 0.5, var_TexCoords.y - 0.5, var_IsWater);
			out_Color = vec4(0.0059, 0.3096, 0.445, 1.0);
		}
		else
		{
			out_Position = vec4(0.0);
			out_Color = vec4(0.0);
		}
	}
	else if (var_IsWater >= 2.0 && var_IsWater <= 5.0)
	{
		out_Position = vec4(texture(u_DiffuseMap, var_TexCoords).rgb, var_IsWater);
		out_Color = vec4(0.0059, 0.3096, 0.445, 1.0);
	}
	else
	{
		out_Position = vec4(var_vertPos.xyz, var_IsWater);
		out_Color = vec4(0.0059, 0.3096, 0.445, 1.0);
	}
}
