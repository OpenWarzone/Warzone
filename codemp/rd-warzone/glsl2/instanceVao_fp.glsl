uniform sampler2D	u_DiffuseMap;

uniform vec4		u_Settings0;		// materialType, 0.0, 0.0, 0.0

uniform float		u_Time;

varying vec2		var_Tex1;
varying vec3		var_VertPos;
varying vec3		var_Normal;

out vec4			out_Glow;
out vec4			out_Position;
out vec4			out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4			out_NormalDetail;
#endif //USE_REAL_NORMALMAPS

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

const float							fBranchHardiness = 0.001;
const float							fBranchSize = 128.0;
const float							fWindStrength = 12.0;
const vec3							vWindDirection = normalize(vec3(1.0, 1.0, 0.0));

vec2 GetSway()
{
	// Wind calculation stuff...
	float fWindPower = 0.5f + sin(var_VertPos.x / fBranchSize + var_VertPos.z / fBranchSize + u_Time*(1.2f + fWindStrength / fBranchSize));

	if (fWindPower < 0.0f)
		fWindPower = fWindPower*0.2f;
	else
		fWindPower = fWindPower*0.3f;

	fWindPower *= fWindStrength;

	return vWindDirection.xy*fWindPower*fBranchHardiness;
}

void main()
{
	vec2 tc = var_Tex1;

	if (u_Settings0.r == MATERIAL_GREENLEAVES)
	{
		tc += GetSway();
	}

	gl_FragColor = texture2D(u_DiffuseMap, tc);
	
	out_Glow = vec4(0.0);

	if (gl_FragColor.a >= 0.001)
	{
		out_Position = vec4(var_VertPos.xyz, u_Settings0.r + 1.0);
		out_Normal = vec4(vec3(EncodeNormal(var_Normal.xyz), 0.0), 1.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
	}
	else
	{
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
	}
}
