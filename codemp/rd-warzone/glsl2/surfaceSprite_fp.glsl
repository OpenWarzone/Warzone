uniform sampler2D				u_DiffuseMap;

uniform vec3					u_ViewOrigin;

uniform vec4					u_Local0; // width, height, fadeStartDistance, fadeEndDistance
uniform vec4					u_Local1; // fadeScale, widthVariance, heightVariance, facing
uniform vec4					u_Local2; // oriented

#define u_Width					u_Local0.r
#define u_Height				u_Local0.g
#define u_FadeStartDistance		u_Local0.b
#define u_FadeEndDistance		u_Local0.a

#define u_FadeScale				u_Local1.r
#define u_WidthVariance			u_Local1.g
#define u_HeightVariance		u_Local1.b
#define u_Facing				u_Local1.a

#define u_Oriented				u_Local2.r

varying vec2   					var_TexCoords;
varying vec3   					var_vertPos;
varying vec3   					var_Normal;
varying float   				var_Alpha;

#define SURFSPRITE_FACING_NORMAL	0
#define SURFSPRITE_FACING_UP		1
#define SURFSPRITE_FACING_DOWN		2
#define SURFSPRITE_FACING_ANY		3

out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4 out_NormalDetail;
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

void main()
{
	const float alphaTestValue = 0.5;
	gl_FragColor = texture(u_DiffuseMap, var_TexCoords);
	gl_FragColor.a *= var_Alpha*(1.0 - alphaTestValue) + alphaTestValue;

#if defined(ALPHA_TEST)
	if ( out_Color.a < alphaTestValue )
		discard;
#endif

	out_Glow = vec4(0.0);

#define SCREEN_MAPS_ALPHA_THRESHOLD 0.666

	if (gl_FragColor.a > SCREEN_MAPS_ALPHA_THRESHOLD)
	{
		out_Position = vec4(var_vertPos.xyz, MATERIAL_GREENLEAVES+1.0);
		out_Normal = vec4( EncodeNormal(var_Normal.xyz), 0.0, 1.0 );
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
