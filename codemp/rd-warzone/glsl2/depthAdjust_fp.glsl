uniform sampler2D	u_ScreenDepthMap;
//uniform sampler2D	u_PositionMap;

varying vec2		var_Tex;

void main()
{
	float depth = texture(u_ScreenDepthMap, var_Tex).r;
	
	if (depth >= 1.0)
	{
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else
	{
		discard;
	}
}
