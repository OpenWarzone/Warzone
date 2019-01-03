uniform sampler2D u_TextureMap;
uniform vec2  u_InvTexRes;

varying vec2 var_TexCoords;

void main()
{
	// Based on "Next Generation Post Processing in Call of Duty: Advanced Warfare":
	// http://advances.realtimerendering.com/s2014/index.html
	vec4 color = vec4(0.0);
	color += 0.0625 * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2(-1.0, -1.0));
	color += 0.125  * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2( 0.0, -1.0));
	color += 0.0625 * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2( 1.0, -1.0));
	color += 0.125  * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2(-1.0,  0.0));
	color += 0.25   * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2( 0.0,  0.0));
	color += 0.125  * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2( 1.0,  0.0));
	color += 0.0625 * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2(-1.0,  1.0));
	color += 0.125  * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2( 0.0,  1.0));
	color += 0.0625 * texture(u_TextureMap, var_TexCoords + u_InvTexRes * vec2( 1.0,  1.0));

	gl_FragColor = color;
}
