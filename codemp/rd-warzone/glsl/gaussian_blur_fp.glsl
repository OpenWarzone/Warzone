uniform sampler2D u_TextureMap;

uniform vec4      u_Color;
uniform vec2      u_InvTexRes;
varying vec2      var_TexCoords;

void main()
{
	vec4 color = vec4 (0.0);

	const float weights[] = float[2](0.25, 0.5);

	#if defined(BLUR_X)
		color += texture2D (u_TextureMap, vec2 (-1.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[0];
		color += texture2D (u_TextureMap, vec2 ( 0.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[1];
		color += texture2D (u_TextureMap, vec2 ( 1.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[0];
	#else
		color += texture2D (u_TextureMap, vec2 (0.0, -1.0) * u_InvTexRes + var_TexCoords) * weights[0];
		color += texture2D (u_TextureMap, vec2 (0.0,  0.0) * u_InvTexRes + var_TexCoords) * weights[1];
		color += texture2D (u_TextureMap, vec2 (0.0,  1.0) * u_InvTexRes + var_TexCoords) * weights[0];
	#endif

	gl_FragColor = color * u_Color;
}
