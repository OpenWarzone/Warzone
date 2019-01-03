uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_SplatControlMap;

uniform vec4		u_Local10;

// The higher the value, the bigger the contrast between the fur length.
#define FUR_STRENGTH_CONTRAST u_Local10.g //2.0

// The higher the value, the less fur.
#define FUR_STRENGTH_CAP u_Local10.b //0.3

in vec4				v_position;
in vec3				v_normal;
in vec2				v_texCoord;
in vec3				v_PrimaryLightDir;

in float			v_furStrength;

out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;

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
	vec3 normal = normalize(v_normal);

	// Orthogonal fur to light is still illumintated. So shift by one, that only fur targeting away from the light do get darkened. 
	float intensity = clamp(dot(normal, v_PrimaryLightDir) + 1.0, 0.0, 1.0);
	
	float power = texture(u_SplatControlMap, v_texCoord).r;
	float furStrength = clamp(v_furStrength * power * FUR_STRENGTH_CONTRAST - FUR_STRENGTH_CAP, 0.0, 1.0);

	float color = texture(u_DiffuseMap, v_texCoord / u_Local10.a).r;
	gl_FragColor = vec4(vec3(0.0, color, 0.0) * intensity, furStrength > 0.5 ? 1.0 : 0.0);

	out_Position = vec4(v_position.xyz, furStrength > 0.5 ? 1.0 : 0.0);
	out_Normal = vec4(EncodeNormal(v_normal.xyz), 0.0, furStrength > 0.5 ? 1.0 : 0.0);
	out_Glow = vec4(0.0);
}
