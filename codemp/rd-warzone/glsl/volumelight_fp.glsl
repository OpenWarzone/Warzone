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
uniform sampler2DShadow				u_ShadowMap;
uniform sampler2DShadow				u_ShadowMap2;
uniform sampler2DShadow				u_ShadowMap3;
uniform sampler2DShadow				u_ShadowMap4;
uniform sampler2DShadow				u_ShadowMap5;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D					u_DiffuseMap;

uniform sampler2DShadow				u_ShadowMap;
uniform sampler2DShadow				u_ShadowMap2;
uniform sampler2DShadow				u_ShadowMap3;
uniform sampler2DShadow				u_ShadowMap4;
uniform sampler2DShadow				u_ShadowMap5;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4					u_Local0; // r_lensflare, 0.0, r_volumeLightStrength * SUN_VOLUMETRIC_SCALE, SUN_VOLUMETRIC_FALLOFF
uniform vec4					u_Local1; // nightScale, isVolumelightShader, 0.0, 0.0
uniform vec4					u_Local2; // r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value

#define LENS_FLARE_ENABLED		u_Local0.r
#define NIGHT_FACTOR			u_Local1.r

// General options...
#define VOLUMETRIC_STRENGTH		u_Local0.b

#if defined(HQ_VOLUMETRIC)
const int	iVolumetricSamples = 32;
const float	fVolumetricDecay = 0.96875;
#elif defined (MQ_VOLUMETRIC)
const int	iVolumetricSamples = 16;
const float	fVolumetricDecay = 0.9375;
#else //!defined(HQ_VOLUMETRIC) && !defined(MQ_VOLUMETRIC)
const int	iVolumetricSamples = 8;
const float	fVolumetricDecay = 0.875;
#endif //defined(HQ_VOLUMETRIC) && defined(MQ_VOLUMETRIC)

const float	fVolumetricWeight = 0.5;
const float	fVolumetricDensity = 1.0;
const float fVolumetricFalloffRange = 0.4;



uniform vec3				u_vlightColors;

uniform vec2				u_vlightPositions;

uniform mat4				u_ShadowMvp;
uniform mat4				u_ShadowMvp2;
uniform mat4				u_ShadowMvp3;
uniform mat4				u_ShadowMvp4;
uniform mat4				u_ShadowMvp5;

uniform vec4				u_Settings0;			// r_shadowSamples (numBlockerSearchSamples), r_shadowMapSize, r_testshaderValue1->value, r_testshaderValue2->value
uniform vec3				u_ViewOrigin;
uniform vec4				u_PrimaryLightOrigin;
uniform vec3				u_ViewForward;
uniform vec4				u_ViewInfo;				// zfar / znear, zfar, depthBits, znear

varying vec2				var_DepthTex;
varying vec3				var_ViewDir;
varying vec3				var_ViewDir2;

#define						r_shadowMapSize			u_Settings0.g

float offset_lookup(sampler2DShadow shadowmap, vec4 loc, vec2 offset, float scale)
{
	return textureProj(shadowmap, vec4(loc.xy + offset * scale * loc.w, loc.z, loc.w)) > 0.1 ? 1.0 : 0.0;
}

float PCF(const sampler2DShadow shadowmap, const vec4 st, const float dist, float scale)
{
	float mult;
	vec4 sCoord = vec4(st);
	vec2 offset = mod(sCoord.xy, 0.5);
	offset.y += offset.x;  // y ^= x in floating point
	if (offset.y > 1.1) offset.y = 0;
	return offset_lookup(shadowmap, sCoord, offset, 0.0);
}

//////////////////////////////////////////////////////////////////////////
float GetVolumetricShadow(void)
{
	float result = 1.0;
	float fWeight = 0.0;
	float dWeight = 1.0;
	float invSamples = 1.0 / float(iVolumetricSamples);
	float sceneDepth = texture(u_DiffuseMap, var_DepthTex).x * 0.4;

	for (int i = 0; i < iVolumetricSamples; i++)
	{
		float depth = (i / float(iVolumetricSamples)) * sceneDepth;
		vec4 biasPos = vec4(u_ViewOrigin + var_ViewDir * (depth - 0.5 / u_ViewInfo.x), 1.0);
		vec4 shadowpos = u_ShadowMvp * biasPos;

		if (all(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
		{
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			result += PCF(u_ShadowMap, shadowpos, shadowpos.z, 1.0 / r_shadowMapSize) > 0.9999999 ? dWeight : 0.0;
			dWeight -= invSamples;
			fWeight += invSamples;
			continue;
		}

		shadowpos = u_ShadowMvp2 * biasPos;

		if (all(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
		{
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			result += PCF(u_ShadowMap2, shadowpos, shadowpos.z, 1.0 / r_shadowMapSize) > 0.9999999 ? dWeight : 0.0;
			dWeight -= invSamples;
			fWeight += invSamples;
			continue;
		}

		shadowpos = u_ShadowMvp3 * biasPos;

		if (all(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
		{
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			result += PCF(u_ShadowMap3, shadowpos, shadowpos.z, 1.0 / r_shadowMapSize) > 0.9999999 ? dWeight : 0.0;
			dWeight -= invSamples;
			fWeight += invSamples;
			continue;
		}

		shadowpos = u_ShadowMvp4 * biasPos;

		if (all(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
		{
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			result += PCF(u_ShadowMap4, shadowpos, shadowpos.z, 1.0 / r_shadowMapSize) > 0.9999999 ? dWeight : 0.0;
			dWeight -= invSamples;
			fWeight += invSamples;
			continue;
		}

		shadowpos = u_ShadowMvp5 * biasPos;

		if (all(lessThanEqual(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
		{
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			result += PCF(u_ShadowMap5, shadowpos, shadowpos.z, 1.0 / r_shadowMapSize) > 0.9999999 ? dWeight : 0.0;
			dWeight -= invSamples;
			fWeight += invSamples;
			continue;
		}
	}
	
	result /= fWeight;
	return result;
}

#define __LENS_FLARE__

#ifdef __LENS_FLARE__
float flare(in vec3 s, in vec3 ctr, in vec3 sDir)
{
    float c = 0.;
	s = normalize(s);
    float sc = dot(s,-sDir);
    c += .5*smoothstep(.99,1.,sc);
    
    s = normalize(s+.9*ctr);
    sc = dot(s,-sDir);
    c += .3*smoothstep(.9,.91,sc);
    
    s = normalize(s-.6*ctr);
    sc = dot(s,-sDir);
    c += smoothstep(.99,1.,sc);
    
    return c;
}

vec3 lensflare3D(in vec3 ray, in vec3 ctr, in vec3 sDir)
{
    vec3 red = vec3(1.,.6,.3);
    vec3 green = vec3(.3,1.,.6);
    vec3 blue = vec3(.6,.3,1.);
	vec3 col = vec3(0.);
    vec3 ref = reflect(ray,ctr);

    col += red*flare(ref,ctr,sDir);
    col += green*flare(ref-.15*ctr,ctr,sDir);
    col += blue*flare(ref-.3*ctr,ctr,sDir);
    
    ref = reflect(ctr,ray);
    col += red*flare(ref,ctr,sDir);
    col += green*flare(ref+.15*ctr,ctr,sDir);
    col += blue*flare(ref+.3*ctr,ctr,sDir);
    
    float d = dot(ctr,sDir);
	return .4*col*max(0.,d*d*d*d*d);
}
#endif //__LENS_FLARE__

void main()
{
	if (NIGHT_FACTOR >= 1.0)
	{// Night... Skip...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 sunColor = u_vlightColors;

	if (NIGHT_FACTOR > 0.0)
	{// Sunrise/Sunset - Adjust the sun color at sunrise/sunset...
		sunColor = mix(sunColor, vec3(1.0, 0.8, 0.625), NIGHT_FACTOR);
	}

	float shadow = GetVolumetricShadow();

#define lightLower2 ( 256.0 / 255.0 )
#define lightUpper2 ( 255.0 / 16384.0 )
	shadow = clamp((shadow - lightLower2) * lightUpper2, 0.0, 1.0);

	shadow = pow(shadow, 1.333); // a little more smoothness/contrast...

	vec3 totalColor = sunColor * shadow;


	// Extra brightness in the direction of the sun...
	vec3 lDir = normalize(u_PrimaryLightOrigin.xyz - u_ViewOrigin.xyz);
	vec3 vDir = normalize(var_ViewDir);
	float fall = max(dot(lDir, vDir), 0.0);
	fall = pow(fall, 16.0);
	fall *= shadow;
	fall *= 8.0;
	fall += 1.0;
	totalColor *= fall;

	totalColor.rgb *= VOLUMETRIC_STRENGTH;// * 1.5125;

	if (NIGHT_FACTOR > 0.0)
	{// Sunrise/Sunset - Scale down screen color, before adding lighting...
		sunColor = mix(sunColor, vec3(0.0), NIGHT_FACTOR);
	}

#ifdef __LENS_FLARE__
	if (LENS_FLARE_ENABLED > 0.0 && shadow > 0.0)
	{
		totalColor.rgb += lensflare3D(vDir.xzy, normalize(var_ViewDir2).xzy, lDir.xzy) * shadow * 2.0;
	}
#endif //__LENS_FLARE__

	gl_FragColor = vec4(totalColor, 1.0);
}
