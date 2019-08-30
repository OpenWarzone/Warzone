#if defined(USE_BINDLESS_TEXTURES)
layout(std140) uniform u_bindlessTexturesBlock
{
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_LightMap;
uniform sampler2D					u_NormalMap;
uniform sampler2D					u_DeluxeMap;
uniform sampler2D					u_SpecularMap;
uniform sampler2D					u_PositionMap;
uniform sampler2D					u_WaterPositionMap;
uniform sampler2D					u_WaterHeightMap;
uniform sampler2D					u_HeightMap;
uniform sampler2D					u_GlowMap;
uniform sampler2D					u_EnvironmentMap;
uniform sampler2D					u_TextureMap;
uniform sampler2D					u_LevelsMap;
uniform sampler2D					u_CubeMap;
uniform sampler2D					u_SkyCubeMap;
uniform sampler2D					u_SkyCubeMapNight;
uniform sampler2D					u_EmissiveCubeMap;
uniform sampler2D					u_OverlayMap;
uniform sampler2D					u_SteepMap;
uniform sampler2D					u_SteepMap1;
uniform sampler2D					u_SteepMap2;
uniform sampler2D					u_SteepMap3;
uniform sampler2D					u_WaterEdgeMap;
uniform sampler2D					u_SplatControlMap;
uniform sampler2D					u_SplatMap1;
uniform sampler2D					u_SplatMap2;
uniform sampler2D					u_SplatMap3;
uniform sampler2D					u_RoadsControlMap;
uniform sampler2D					u_RoadMap;
uniform sampler2D					u_DetailMap;
uniform sampler2D					u_ScreenImageMap;
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2D					u_ShadowMap;
uniform sampler2D					u_ShadowMap2;
uniform sampler2D					u_ShadowMap3;
uniform sampler2D					u_ShadowMap4;
uniform sampler2D					u_ShadowMap5;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D	u_DiffuseMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2		u_Dimensions;

uniform vec4		u_Local0;

varying vec2		var_TexCoords;


vec2 pixelSize = vec2(1.0) / u_Dimensions;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//paint
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void main (void)
{
#if 0
	vec4 res = texture(u_DiffuseMap, var_TexCoords.xy);
	
	int PaintRadius = 2;
	
	float Intensitycount[10];
	vec3 color[10];

	for (int i = 0; i < 10; i++)
	{
		Intensitycount[i] = 0.0;
		color[i] = vec3(0.0);
	}

	for(int k = -PaintRadius; k < (PaintRadius + 1); k++)
	{
		for(int j = -PaintRadius; j < (PaintRadius + 1); j++)
		{
			vec2 tex;
			tex = var_TexCoords.xy + vec2(pixelSize.x * float(k), pixelSize.y * float(j));
			vec3 c = texture(u_DiffuseMap, tex).rgb;

			int lum = int(dot(c, vec3(0.212, 0.716, 0.072))) * 9;
				
			Intensitycount[0] = ( lum == 0 ) ? Intensitycount[0] + 1.0 : Intensitycount[0];
			Intensitycount[1] = ( lum == 1 ) ? Intensitycount[1] + 1.0 : Intensitycount[1];
			Intensitycount[2] = ( lum == 2 ) ? Intensitycount[2] + 1.0 : Intensitycount[2];
			Intensitycount[3] = ( lum == 3 ) ? Intensitycount[3] + 1.0 : Intensitycount[3];
			Intensitycount[4] = ( lum == 4 ) ? Intensitycount[4] + 1.0 : Intensitycount[4];
			Intensitycount[5] = ( lum == 5 ) ? Intensitycount[5] + 1.0 : Intensitycount[5];
			Intensitycount[6] = ( lum == 6 ) ? Intensitycount[6] + 1.0 : Intensitycount[6];
			Intensitycount[7] = ( lum == 7 ) ? Intensitycount[7] + 1.0 : Intensitycount[7];
			Intensitycount[8] = ( lum == 8 ) ? Intensitycount[8] + 1.0 : Intensitycount[8];
			Intensitycount[9] = ( lum == 9 ) ? Intensitycount[9] + 1.0 : Intensitycount[9];
				
			color[0] = ( lum == 0 ) ? color[0] + c : color[0];
			color[1] = ( lum == 1 ) ? color[1] + c : color[1];
			color[2] = ( lum == 2 ) ? color[2] + c : color[2];
			color[3] = ( lum == 3 ) ? color[3] + c : color[3];
			color[4] = ( lum == 4 ) ? color[4] + c : color[4];
			color[5] = ( lum == 5 ) ? color[5] + c : color[5];
			color[6] = ( lum == 6 ) ? color[6] + c : color[6];
			color[7] = ( lum == 7 ) ? color[7] + c : color[7];
			color[8] = ( lum == 8 ) ? color[8] + c : color[8];
			color[9] = ( lum == 9 ) ? color[9] + c : color[9];
		}
	}

	int Maxint = 0;
	float Maxcount = 0.0;

	for(int i = 0; i < 10; i++)
	{
		if(Intensitycount[i] > Maxcount)
		{
			Maxcount = Intensitycount[i];
			Maxint = i;
		}
	}	

	res.xyz = color[Maxint] / Maxcount;
	res.w = 1.0;

	gl_FragColor = res;
#else
	const int uni_Radius = 2;
	//int uni_Radius = int(u_Local0.r);
	float   lRadiusSquare = float((uni_Radius + 1) * (uni_Radius + 1));
    vec3    lColor[4];
    vec3    lColorSquare[4];

    for (int lIndex = 0; lIndex < 4; lIndex++)
    {
        lColor[lIndex] = vec3(0.0f);
        lColorSquare[lIndex] = vec3(0.0f);
    }

    for (int j = -uni_Radius; j <= 0; j++)
    {
        for (int i = -uni_Radius; i <= 0; i++)
        {
            vec3    lTexColor = texture2D(u_DiffuseMap, var_TexCoords + vec2(i, j) / u_Dimensions).rgb;
            lColor[0] += lTexColor;
            lColorSquare[0] += lTexColor * lTexColor;
        }
    }

    for (int j = -uni_Radius; j <= 0; j++)
    {
        for (int i = 0; i <= uni_Radius; i++)
        {
            vec3    lTexColor = texture2D(u_DiffuseMap, var_TexCoords + vec2(i, j) / u_Dimensions).rgb;
            lColor[1] += lTexColor;
            lColorSquare[1] += lTexColor * lTexColor;
        }
    }

    for (int j = 0; j <= uni_Radius; j++)
    {
        for (int i = 0; i <= uni_Radius; i++)
        {
            vec3    lTexColor = texture2D(u_DiffuseMap, var_TexCoords + vec2(i, j) / u_Dimensions).rgb;
            lColor[2] += lTexColor;
            lColorSquare[2] += lTexColor * lTexColor;
        }
    }

    for (int j = 0; j <= uni_Radius; j++)
    {
        for (int i = -uni_Radius; i <= 0; i++)
        {
            vec3    lTexColor = texture2D(u_DiffuseMap, var_TexCoords + vec2(i, j) / u_Dimensions).rgb;
            lColor[3] += lTexColor;
            lColorSquare[3] += lTexColor * lTexColor;
        }
    }

    float   lMinSigma = 4.71828182846;

    for (int i = 0; i < 4; i++)
    {
        lColor[i] /= lRadiusSquare;
        lColorSquare[i] = abs(lColorSquare[i] / lRadiusSquare - lColor[i] * lColor[i]);
        float   lSigma = lColorSquare[i].r + lColorSquare[i].g + lColorSquare[i].b;
        if (lSigma < lMinSigma)
        {
            lMinSigma = lSigma;
            gl_FragColor = vec4(lColor[i], texture2D(u_DiffuseMap, var_TexCoords).a);
        }
    }
#endif
}
