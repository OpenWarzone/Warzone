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
uniform sampler3D					u_VolumeMap;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D u_DiffuseMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4	u_ViewInfo; // zfar / znear, zfar
uniform vec2	u_Dimensions;
uniform vec4	u_Local0; // num_passes, 0, 0, 0


varying vec2	var_TexCoords;

#define SHARPEN_ENABLED
//#define HDR_ENABLED
//#define SSAO_ENABLED
//#define LF_ENABLED

vec2 resolution = u_Dimensions;
vec2 vTexCoords = var_TexCoords;

vec2 TEX_OFFSET = vec2(1.0 / float(u_Dimensions.x), 1.0 / float(u_Dimensions.y));

vec2 KERNEL[8] = vec2[](
	vec2(TEX_OFFSET.x, TEX_OFFSET.y),
	vec2(0.0, TEX_OFFSET.y),
	vec2(-TEX_OFFSET.x, TEX_OFFSET.y),
	vec2(-TEX_OFFSET.x, 0.0),
	vec2(-TEX_OFFSET.x, -TEX_OFFSET.y),
	vec2(0.0, -TEX_OFFSET.y),
	vec2(TEX_OFFSET.x, -TEX_OFFSET.y),
	vec2(TEX_OFFSET.x, 0.0));

//==============================================================================
vec3 grayScale(in vec3 col)
{
	float grey = (col.r + col.g + col.b) * 0.333333333; // aka: / 3.0
	return vec3(grey);
}

//==============================================================================
vec3 saturation(in vec3 col, in float factor)
{
	const vec3 lumCoeff = vec3(0.2125, 0.7154, 0.0721);

	vec3 intensity = vec3(dot(col, lumCoeff));
	return mix(intensity, col, factor);
}

//==============================================================================
vec3 gammaCorrection(in float gamma, in vec3 col)
{
	return pow(col, vec3(1.0 / gamma));
}

//==============================================================================
vec3 gammaCorrectionRgb(in vec3 gamma, in vec3 col)
{
	return pow(col, 1.0 / gamma);
}

//==============================================================================
vec3 sharpen(in sampler2D tex, in vec2 texCoords)
{
	const float sharpenFactor = 0.25;

	vec3 col = texture(tex, texCoords).rgb;

	vec3 col2 = texture(tex, texCoords + KERNEL[0]).rgb;
	for(int i = 1; i < 8; i++)
	{
		col2 += texture(tex, texCoords + KERNEL[i]).rgb;
	}

	return col * (8.0 * sharpenFactor + 1.0) - sharpenFactor * col2;
}

//==============================================================================
vec3 erosion(in sampler2D tex, in vec2 texCoords)
{
    vec3 minValue = texture(tex, texCoords).rgb;

    for (int i = 0; i < 8; i++)
    {
        vec3 tmpCol = textureLod(tex, texCoords + KERNEL[i], 0.0).rgb;
        minValue = min(tmpCol, minValue);
    }

    return minValue;
}


//==============================================================================
void main(void)
{
	vec3 fColor;

#if defined(SHARPEN_ENABLED)
	fColor = sharpen(u_DiffuseMap, vTexCoords);
#else
	fColor = texture(u_DiffuseMap, vTexCoords).rgb;
#endif
	//fColor = erosion(u_DiffuseMap, vTexCoords);

#if defined(HDR_ENABLED)
	vec3 hdr = texture(ppsHdrFai, vTexCoords).rgb;
	fColor += hdr;
#endif

#if defined(SSAO_ENABLED)
	float ssao = texture(ppsSsaoFai, vTexCoords).r;
	fColor *= ssao;
#endif

#if defined(LF_ENABLED)
	vec3 lf = texture(ppsLfFai, vTexCoords).rgb;
	fColor += lf;
#endif

	//fColor = BlendHardLight(vec3(0.7, 0.72, 0.4), fColor);
	fColor = gammaCorrectionRgb(vec3(0.9, 0.92, 0.75), fColor);

	gl_FragColor = vec4(fColor, 1.0);
}
