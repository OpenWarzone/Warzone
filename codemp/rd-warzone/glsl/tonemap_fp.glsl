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
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_LevelsMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4						u_Color;
uniform vec4						u_Local0; // tonemapMethod, sphericPower, 0.0, 0.0

uniform vec2						u_AutoExposureMinMax;
uniform vec3						u_ToneMinAvgMaxLinear;

varying vec2						var_TexCoords;

const vec3  LUMINANCE_VECTOR =   vec3(0.2125, 0.7154, 0.0721); //vec3(0.299, 0.587, 0.114);

vec3 FilmicTonemap(vec3 x)
{
	const float SS  = 0.22; // Shoulder Strength
	const float LS  = 0.30; // Linear Strength
	const float LA  = 0.10; // Linear Angle
	const float TS  = 0.20; // Toe Strength
	const float TAN = 0.01; // Toe Angle Numerator
	const float TAD = 0.30; // Toe Angle Denominator
	
	vec3 SSxx = SS * x * x;
	vec3 LSx = LS * x;
	vec3 LALSx = LSx * LA;
	
	return ((SSxx + LALSx + TS * TAN) / (SSxx + LSx + TS * TAD)) - TAN / TAD;

	//return ((x*(SS*x+LA*LS)+TS*TAN)/(x*(SS*x+LS)+TS*TAD)) - TAN/TAD;
}

vec3 HaarmPeterDuikerFilmicToneMapping(in vec3 x)
{
	x = max( vec3(0.0), x - 0.004 );
	return pow( abs( ( x * ( 6.2 * x + 0.5 ) ) / ( x * ( 6.2 * x + 1.7 ) + 0.06 ) ), vec3(2.2) );
}

vec3 CustomToneMapping(in vec3 x)
{
	const float A = 0.665f;
	const float B = 0.09f;
	const float C = 0.004f;
	const float D = 0.445f;
	const float E = 0.26f;
	const float F = 0.025f;
	const float G = 0.16f;//0.145f;
	const float H = 1.1844f;//1.15f;

    // gamma space or not?
	return (((x*(A*x+B)+C)/(x*(D*x+E)+F))-G) / H;
}

vec3 ColorFilmicToneMapping(in vec3 x)
{
	// Filmic tone mapping
	const vec3 A = vec3(0.55f, 0.50f, 0.45f);	// Shoulder strength
	const vec3 B = vec3(0.30f, 0.27f, 0.22f);	// Linear strength
	const vec3 C = vec3(0.10f, 0.10f, 0.10f);	// Linear angle
	const vec3 D = vec3(0.10f, 0.07f, 0.03f);	// Toe strength
	const vec3 E = vec3(0.01f, 0.01f, 0.01f);	// Toe Numerator
	const vec3 F = vec3(0.30f, 0.30f, 0.30f);	// Toe Denominator
	const vec3 W = vec3(2.80f, 2.90f, 3.10f);	// Linear White Point Value
	const vec3 F_linearWhite = ((W*(A*W+C*B)+D*E)/(W*(A*W+B)+D*F))-(E/F);
	vec3 F_linearColor = ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-(E/F);

    // gamma space or not?
	//return pow(clamp(F_linearColor * 1.25 / F_linearWhite, 0.0, 1.0), vec3(1.25));
	return clamp(F_linearColor * 1.25 / F_linearWhite, 0.0, 1.0);
}

#define sphericalAmount u_Local0.g //[0.0:2.0] //-Amount of spherical tonemapping applied...sort of

vec3 SphericalPass( vec3 color )
{
	vec3 signedColor = clamp(color.rgb, 0.0, 1.0) * 2.0 - 1.0;
	vec3 sphericalColor = sqrt(vec3(1.0) - signedColor.rgb * signedColor.rgb);
	sphericalColor = sphericalColor * 0.5 + 0.5;
	sphericalColor *= color.rgb;
	color.rgb += sphericalColor.rgb * sphericalAmount;
	color.rgb *= 0.95;
	return color;
}

void main()
{
	vec4 color = texture2D(u_DiffuseMap, var_TexCoords) * u_Color;
	vec3 minAvgMax = texture2D(u_LevelsMap, var_TexCoords).rgb;
	vec3 logMinAvgMaxLum = clamp(minAvgMax * 20.0 - 10.0, -u_AutoExposureMinMax.y, -u_AutoExposureMinMax.x);
		
	float avgLum = exp2(logMinAvgMaxLum.y);
	//float maxLum = exp2(logMinAvgMaxLum.z);

	color.rgb *= u_ToneMinAvgMaxLinear.y / avgLum;
	color.rgb = max(vec3(0.0), color.rgb - vec3(u_ToneMinAvgMaxLinear.x));

	if (u_Local0.r == 1)
	{
		color.rgb = SphericalPass(color.rgb);
	}
	else if (u_Local0.r == 2)
	{
		color.rgb = ColorFilmicToneMapping(color.rgb);
	}
	else if (u_Local0.r == 3)
	{// Watch dogs method...
		color.rgb = HaarmPeterDuikerFilmicToneMapping(color.rgb);
	}
	else if (u_Local0.r == 4)
	{
		color.rgb = CustomToneMapping(color.rgb);
	}
	else
	{
		vec3 fWhite = 1.0 / FilmicTonemap(vec3(u_ToneMinAvgMaxLinear.z - u_ToneMinAvgMaxLinear.x));
		color.rgb = FilmicTonemap(color.rgb) * fWhite;
	}
	
	gl_FragColor = clamp(color, 0.0, 1.0);
}
