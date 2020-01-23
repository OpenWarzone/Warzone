uniform vec4		u_Local9; // testvalue1-3, MAP_WATER_LEVEL
uniform vec4		u_Local10;

varying vec3		var_vertPos;
varying vec3		var_Normal;

out vec4 out_Glow;
out vec4 out_Normal;
out vec4 out_Position;
#ifdef USE_REAL_NORMALMAPS
out vec4 out_NormalDetail;
#endif //USE_REAL_NORMALMAPS

#define MAP_WATER_LEVEL u_Local9.a

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

void main()
{
	out_Color = vec4(0.0059, 0.3096, 0.445, 1.0);
	out_Position = vec4(var_vertPos.xyz, 1.0);
	//out_Position = vec4(0.0, 0.0, MAP_WATER_LEVEL, 1.0);
	out_Glow = vec4(0.0);
	out_Normal = vec4(EncodeNormal(var_Normal), length(gl_FragCoord.z), 1.0);
#ifdef USE_REAL_NORMALMAPS
	out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
}
