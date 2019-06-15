uniform sampler2D	u_TextureMap;
uniform sampler2D	u_ScreenDepthMap;
uniform sampler2D	u_GlowMap;

uniform vec2		u_Dimensions;
uniform vec4		u_ViewInfo; // zfar / znear, zfar

uniform vec4		u_Local0; // dofValue, dynamicGlowEnabled, 0, direction
uniform vec4		u_Local1; // testvalues


varying vec2		var_TexCoords;
flat varying float	var_FocalDepth;


vec2 sampleOffset = vec2(1.0/u_Dimensions);

#define PIOVER180 0.017453292

//MATSO DOF
//#define fMatsoDOFChromaPow		3.0		// [0.2 to 3.0] Amount of chromatic abberation color shifting.
//#define fMatsoDOFBokehCurve		2.4		// [0.5 to 20.0] Bokeh curve.
//#define fMatsoDOFBokehLight		0.512 	// [0.0 to 2.0] Bokeh brightening factor.
//#define fMatsoDOFBokehAngle		10		// [0 to 360] Rotation angle of bokeh shape.
#define fMatsoDOFChromaPow		3.0//2.4		// [0.2 to 3.0] Amount of chromatic abberation color shifting.
#define fMatsoDOFBokehCurve		6.0		// [0.5 to 20.0] Bokeh curve.
#define fMatsoDOFBokehLight		4.0//0.512 	// [0.0 to 2.0] Bokeh brightening factor.
#define fMatsoDOFBokehAngle		10		// [0 to 360] Rotation angle of bokeh shape.

#if defined(FAST_DOF)

#define iMatsoDOFBokehQuality	6		// [1 to 10] Blur quality as control value over tap count.
#define DOF_BLURRADIUS 			3.0

#elif defined(MEDIUM_DOF)

#define iMatsoDOFBokehQuality	6		// [1 to 10] Blur quality as control value over tap count.
#define DOF_BLURRADIUS 			6.0

#else //defined(HIGH_DOF)

#define iMatsoDOFBokehQuality	8		// [1 to 10] Blur quality as control value over tap count.
#define DOF_BLURRADIUS 			10.0

#endif //defined(HIGH_DOF)

const vec2 tdirs[4] = vec2[4]( vec2(-0.306, 0.739), vec2(0.306, 0.739), vec2(-0.739, 0.306), vec2(-0.739, -0.306) );


float GetGlowStrength(vec2 coord)
{
	vec2 coord2 = coord;
	coord2.y = 1.0 - coord2.y;
	vec4 glow = texture2D(u_GlowMap, coord2.xy);
	float glowStrength = clamp(length(glow.rgb) * fMatsoDOFBokehLight, 0.0, 1.0);
	return glowStrength;
}

vec4 GetMatsoDOFCA(sampler2D col, vec2 tex, float CoC)
{
	vec3 chroma;
	chroma.r = pow(0.5, fMatsoDOFChromaPow * CoC);
	chroma.g = pow(1.0, fMatsoDOFChromaPow * CoC);
	chroma.b = pow(1.5, fMatsoDOFChromaPow * CoC);

	vec2 tr = ((2.0 * tex - 1.0) * chroma.r) * 0.5 + 0.5;
	vec2 tg = ((2.0 * tex - 1.0) * chroma.g) * 0.5 + 0.5;
	vec2 tb = ((2.0 * tex - 1.0) * chroma.b) * 0.5 + 0.5;
	
	vec3 color = vec3(texture2D(col, tr).r, texture2D(col, tg).g, texture2D(col, tb).b) * (1.0 - CoC);

	return vec4(color, 1.0);
}

vec4 GetMatsoDOFBlur(int axis, vec2 coord, sampler2D SamplerHDRX)
{
	vec4 tcol = texture2D(SamplerHDRX, coord.xy);
	float focalDepth = var_FocalDepth;
	float coordDepth = texture2D(u_ScreenDepthMap, coord.xy).x;
	float depthDiff = (coordDepth - focalDepth) * 3.0;
	vec2 discRadius = (depthDiff * float(DOF_BLURRADIUS)) * sampleOffset.xy * 0.5 / float(iMatsoDOFBokehQuality);
	float wValue = 1.0;
	
	if (depthDiff < 0.0)
	{// Close to camera pixels, blur much less so player model is not blury...
		discRadius *= 0.003;
	}

	discRadius *= 0.5;

#if !defined(FAST_DOF)
	bool isGlow = false;

	if (u_Local0.g <= 0.0)
	{// No dynamic glow map to use. Apply bokeh as per standard matso DOF...
		wValue = (1.0 + pow(length(tcol.rgb) + 0.1, fMatsoDOFBokehCurve)) * (1.0 - fMatsoDOFBokehLight);	// special recipe from papa Matso ;)
	}
	else
	{// Have dynamic glow to use. Only apply brightening to glow areas...
		float glowStrength = GetGlowStrength(coord);
		isGlow = bool(glowStrength > 0.05);

		if (isGlow)
		{// Since we have a glow map for the screen, only brighten when this pixel is on the glow map... So we don't brighten non-glowing parts of the screen.
			wValue = (1.0 + pow(length(tcol.rgb) + 0.1, fMatsoDOFBokehCurve)) * (1.0 - (fMatsoDOFBokehLight*glowStrength));	// special recipe from papa Matso ;)
		}
	}
#endif

	for (int i = -iMatsoDOFBokehQuality; i < iMatsoDOFBokehQuality; i++)
	{
		vec2 taxis =  tdirs[axis];

		taxis.x = cos(fMatsoDOFBokehAngle*PIOVER180)*taxis.x-sin(fMatsoDOFBokehAngle*PIOVER180)*taxis.y;
		taxis.y = sin(fMatsoDOFBokehAngle*PIOVER180)*taxis.x+cos(fMatsoDOFBokehAngle*PIOVER180)*taxis.y;
		
		float fi = float(i);
		vec2 tcoord = coord.xy + (taxis * fi * discRadius).xy;

		vec4 ct;

#if !defined(FAST_DOF)
		if (isGlow || u_Local0.g <= 0.0)
			ct = GetMatsoDOFCA(SamplerHDRX, tcoord.xy, discRadius.x);
		else
#endif
			ct = texture2D(SamplerHDRX, tcoord.xy);

		// my own pseudo-bokeh weighting
		float b = dot(length(ct.rgb),0.333) + length(ct.rgb) + 0.1;
		float w = pow(b, fMatsoDOFBokehCurve) + abs(fi);

		tcol += ct * w;
		wValue += w;
	}

	tcol /= wValue;

	return vec4(tcol.rgb, 1.0);
}

void main ()
{
	gl_FragColor = GetMatsoDOFBlur(int(u_Local0.a), var_TexCoords, u_TextureMap);
}
