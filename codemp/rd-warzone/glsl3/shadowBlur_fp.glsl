uniform sampler2DShadow	u_ShadowMap;

uniform vec2			u_Dimensions;

uniform vec4			u_Local0; // r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value

varying vec2			var_TexCoords;

#define BLUR_RADIUS 2.0

void main()
{
	float numValues = 0.0;
	float shadow = 0.0;

	for (float x = -BLUR_RADIUS; x <= BLUR_RADIUS; x += BLUR_PIXELMULT)
	{
		for (float y = -BLUR_RADIUS; y <= BLUR_RADIUS; y += BLUR_PIXELMULT)
		{
			vec2 offset = vec2(x, y) * u_Dimensions;
			vec2 xy = vec2(var_TexCoords + offset);
			shadow += textureProj(u_ShadowMap, xy, 0.0).x;
			numValues += 1.0;
		}
	}

	shadow /= numValues;

	gl_FragColor = vec4(shadow, shadow, shadow, 1.0);
}
