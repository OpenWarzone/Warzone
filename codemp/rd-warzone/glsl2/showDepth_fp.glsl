uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_ScreenDepthMap;

uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin

varying vec2		var_TexCoords;

void main(void)
{
	float depth = texture(u_ScreenDepthMap, var_TexCoords).r;
	gl_FragColor = vec4(depth, depth, depth, 1.0);
}