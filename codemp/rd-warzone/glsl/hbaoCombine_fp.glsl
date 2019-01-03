uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;

uniform vec2		u_Dimensions;

varying vec2		var_ScreenTex;

//#define DEBUG
#define BLUR_WIDTH	8.0

void main()
{
#if defined(DEBUG)
	gl_FragColor = vec4(textureLod(u_NormalMap, var_ScreenTex, 0.0).rgb, 1.0);
#else //!defined(DEBUG)
	vec4 color = textureLod(u_DiffuseMap, var_ScreenTex, 0.0);
	/*vec3 hbao = textureLod(u_NormalMap, var_ScreenTex, 0.0).rgb;
	gl_FragColor = vec4(color.rgb * (hbao.rgb * 0.7), color.a);*/

	float NUM_VALUES = 1.0;
	vec2 PIXEL_OFFSET = vec2(1.0 / u_Dimensions);
	vec2 texcoord = var_ScreenTex;
	//texcoord.x += PIXEL_OFFSET.x;

	float hbao0 = textureLod(u_NormalMap, texcoord.xy, 0.0).r;
	float hbao = hbao0;

	for (float width = -BLUR_WIDTH; width <= BLUR_WIDTH; width += 1.0)
	{
		//float dist_mult = clamp(1.0 - (width / BLUR_WIDTH), 0.333, 1.0);
		float hbao1 = textureLod(u_NormalMap, texcoord.xy + (width * PIXEL_OFFSET), 0.0).r;
		float hbao2 = textureLod(u_NormalMap, texcoord.xy - (width * PIXEL_OFFSET), 0.0).r;
		float hbao3 = textureLod(u_NormalMap, texcoord.xy + (width * vec2(PIXEL_OFFSET.x, -PIXEL_OFFSET.y)), 0.0).r;
		float hbao4 = textureLod(u_NormalMap, texcoord.xy + (width * vec2(-PIXEL_OFFSET.x, PIXEL_OFFSET.y)), 0.0).r;
		float hbao5 = textureLod(u_NormalMap, texcoord.xy + (width * vec2(PIXEL_OFFSET.x, 0.0)), 0.0).r;
		float hbao6 = textureLod(u_NormalMap, texcoord.xy + (width * vec2(0.0, PIXEL_OFFSET.y)), 0.0).r;
		float hbao7 = textureLod(u_NormalMap, texcoord.xy + (width * vec2(-PIXEL_OFFSET.x, 0.0)), 0.0).r;
		float hbao8 = textureLod(u_NormalMap, texcoord.xy + (width * vec2(0.0, -PIXEL_OFFSET.y)), 0.0).r;
		float ao = clamp(((/*hbao0 +*/ hbao1 + hbao2 + hbao2 + hbao4 + hbao5 + hbao6 + hbao7 + hbao8) /** dist_mult*/) / 8.0/*9.0*/, 0.333, 1.0);

		hbao += ao;
		NUM_VALUES += 1.0;
	}

	hbao /= NUM_VALUES;
	hbao = clamp(hbao, 0.75, 1.0);

	gl_FragColor = vec4((color.rgb * hbao), color.a);
#endif //defined(DEBUG)
}
