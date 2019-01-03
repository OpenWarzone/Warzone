uniform sampler2D	u_DiffuseMap;

varying vec2		var_DiffuseTex;
varying vec4		var_Color;
varying vec3		var_Normal;
varying vec3		var_VertPos;

uniform vec4				u_Local8; // stageNum, glowStrength, 0, 0

out vec4 out_Glow;

void main()
{
	gl_FragColor = texture2D(u_DiffuseMap, var_DiffuseTex) * var_Color;
	
	#if defined(USE_GLOW_BUFFER)
#define glow_const_1 ( 23.0 / 255.0)
#define glow_const_2 (255.0 / 229.0)
		gl_FragColor.rgb = clamp((clamp(gl_FragColor.rgb - glow_const_1, 0.0, 1.0)) * glow_const_2, 0.0, 1.0);
		gl_FragColor.rgb *= u_Local8.g;
		out_Glow = gl_FragColor;
	#else
		out_Glow = vec4(0.0);
	#endif
}
