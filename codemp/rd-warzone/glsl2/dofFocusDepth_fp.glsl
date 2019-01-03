uniform sampler2D		u_ScreenDepthMap;

uniform vec2			u_Dimensions;

varying vec2			var_TexCoords;

void main ()
{
	float color = 0.0;
	vec2 invDepth = vec2(1.0) / u_Dimensions;

	for (float x = 0.0; x < u_Dimensions.x; x += 1.0)
	{
		for (float y = 0.0; y < u_Dimensions.y; y += 1.0)
		{
			vec2 tc = vec2(x,y) * invDepth;
			color += texture(u_ScreenDepthMap, tc).r;
		}
	}

	float avgDepth = color / (u_Dimensions.x * u_Dimensions.y);
	gl_FragColor = vec4(avgDepth, avgDepth, avgDepth, 1.0);
}
