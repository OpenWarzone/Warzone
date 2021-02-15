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
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_ScreenDepthMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2						u_Dimensions;
uniform vec4						u_ViewInfo; // zfar / znear, zfar

uniform vec4						u_Local0; // dofValue, dynamicGlowEnabled, 0, direction
uniform vec4						u_Local1; // testvalues


varying vec2						var_TexCoords;
flat varying float					var_FocalDepth;


vec2 sampleOffset = vec2(1.0/u_Dimensions);

#define PIOVER180 0.017453292

//MATSO DOF
#define fMatsoDOFChromaPow		3.0//2.4		// [0.2 to 3.0] Amount of chromatic abberation color shifting.
#define fMatsoDOFBokehCurve		2.4		// [0.5 to 20.0] Bokeh curve.
#define fMatsoDOFBokehLight		1.512//0.512 	// [0.0 to 2.0] Bokeh brightening factor.
#define fMatsoDOFBokehAngle		10		// [0 to 360] Rotation angle of bokeh shape.

#if defined(FAST_DOF)

#define iMatsoDOFBokehQuality	4		// [1 to 10] Blur quality as control value over tap count.
#define DOF_BLURRADIUS 			4.0//12.0

#elif defined(MEDIUM_DOF)

//#define iMatsoDOFBokehQuality	6		// [1 to 10] Blur quality as control value over tap count.
//#define DOF_BLURRADIUS 			5.0//12.0
#define iMatsoDOFBokehQuality	3		// [1 to 10] Blur quality as control value over tap count.
#define DOF_BLURRADIUS 			10.0

#else //defined(HIGH_DOF)

#define iMatsoDOFBokehQuality	10		// [1 to 10] Blur quality as control value over tap count.
//#define DOF_BLURRADIUS 			6.0
#define DOF_BLURRADIUS 			10.0

#endif //defined(HIGH_DOF)

const vec2 tdirs[4] = vec2[4]( vec2(-0.306, 0.739), vec2(0.306, 0.739), vec2(-0.739, 0.306), vec2(-0.739, -0.306) );

vec3 GetMatsoDOFCA(sampler2D col, vec2 tex, float CoC)
{
	vec3 chroma;
	chroma.r = pow(0.5, fMatsoDOFChromaPow * CoC);
	chroma.g = pow(1.0, fMatsoDOFChromaPow * CoC);
	chroma.b = pow(1.5, fMatsoDOFChromaPow * CoC);

	vec2 tr = ((2.0 * tex - 1.0) * chroma.r) * 0.5 + 0.5;
	vec2 tg = ((2.0 * tex - 1.0) * chroma.g) * 0.5 + 0.5;
	vec2 tb = ((2.0 * tex - 1.0) * chroma.b) * 0.5 + 0.5;
	
	return vec3(texture2D(col, tr).r, texture2D(col, tg).g, texture2D(col, tb).b) * (1.0 - CoC);
}

vec4 GetMatsoDOFBlur(int axis, vec2 coord, sampler2D SamplerHDRX)
{
	vec3 tcol = texture2D(SamplerHDRX, coord.xy).rgb;
	float wValue = 1.0;
	float focalDepth = var_FocalDepth;
	float coordDepth = texture2D(u_ScreenDepthMap, coord.xy).x;
	float depthDiff = (coordDepth - focalDepth);

//#if !defined(FAST_DOF)
//	depthDiff *= u_Local1.r;//3.0;
//#endif //!defined(FAST_DOF)
	
	if (depthDiff < 0.0)
	{// Close to camera pixels, blur much less so player model is not blury...
		depthDiff = clamp(pow(length(depthDiff), 128.0), 0.0, 1.0);
	}
	else
	{
		depthDiff = clamp(pow(depthDiff, 0.75), 0.0, 1.0);
	}

	// Always use a tiny bit of blur.
	depthDiff = max(depthDiff, 0.05);

	vec2 discRadius = (depthDiff * float(DOF_BLURRADIUS)) * sampleOffset.xy / float(iMatsoDOFBokehQuality);
	
	for (int i = -iMatsoDOFBokehQuality; i < iMatsoDOFBokehQuality; i++)
	{
		vec2 taxis =  tdirs[axis];

		taxis.x = cos(fMatsoDOFBokehAngle*PIOVER180)*taxis.x-sin(fMatsoDOFBokehAngle*PIOVER180)*taxis.y;
		taxis.y = sin(fMatsoDOFBokehAngle*PIOVER180)*taxis.x+cos(fMatsoDOFBokehAngle*PIOVER180)*taxis.y;
		
		float fi = float(i);
		vec2 tcoord = coord.xy + (taxis * fi * discRadius).xy;

//		vec3 ct = GetMatsoDOFCA(SamplerHDRX, tcoord.xy, discRadius.x);
		vec3 ct = texture2D(SamplerHDRX, tcoord.xy).rgb;

		// my own pseudo-bokeh weighting
		float b = dot(length(ct),0.333) + length(ct) + 0.1;
		float w = clamp(pow(b, fMatsoDOFBokehCurve) + abs(fi), 0.0, 100.0);

		tcol += ct * w;
		wValue += w;
	}

	tcol = clamp(tcol / wValue, 0.0, 1.0);

	return vec4(tcol, 1.0);
}

void main ()
{
	gl_FragColor = GetMatsoDOFBlur(int(u_Local0.a), var_TexCoords, u_DiffuseMap);
}
