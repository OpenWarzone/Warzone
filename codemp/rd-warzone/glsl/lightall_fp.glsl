//#define _CHRISTMAS_LIGHTS_

#define SCREEN_MAPS_ALPHA_THRESHOLD 0.666
#define SCREEN_MAPS_LEAFS_THRESHOLD 0.001
//#define SCREEN_MAPS_LEAFS_THRESHOLD 0.9


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
uniform sampler2D					u_GlowMap;
uniform sampler2D					u_LightMap;
uniform sampler2D					u_NormalMap;
uniform sampler2D					u_DeluxeMap;
uniform sampler2D					u_OverlayMap;
uniform sampler2D					u_EnvironmentMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4						u_MapAmbient; // a basic light/color addition across the whole map...

uniform vec4						u_PrimaryLightOrigin;
uniform vec3						u_PrimaryLightColor;
uniform float						u_PrimaryLightRadius;

uniform vec4						u_Settings0; // useTC, useDeform, useRGBA, isTextureClamped
uniform vec4						u_Settings1; // useVertexAnim, useSkeletalAnim, blendMethod, is2D
uniform vec4						u_Settings2; // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
uniform vec4						u_Settings3; // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL, 0=DetailMapNormal 1=detailMapFromTC 2=detailMapFromWorld, USE_GLOW_BLEND_MODE
uniform vec4						u_Settings4; // MAP_LIGHTMAP_MULTIPLIER, MAP_LIGHTMAP_ENHANCEMENT, 0.0, 0.0
uniform vec4						u_Settings5; // MAP_COLOR_SWITCH_RG, MAP_COLOR_SWITCH_RB, MAP_COLOR_SWITCH_GB, ENABLE_CHRISTMAS_EFFECT
uniform vec4						u_Settings6; // TREE_BRANCH_HARDINESS, TREE_BRANCH_SIZE, TREE_BRANCH_WIND_STRENGTH, 0.0

#define USE_TC						u_Settings0.r
#define USE_DEFORM					u_Settings0.g
#define USE_RGBA					u_Settings0.b
#define USE_TEXTURECLAMP			u_Settings0.a

#define USE_VERTEX_ANIM				u_Settings1.r
#define USE_SKELETAL_ANIM			u_Settings1.g
#define USE_BLEND					u_Settings1.b
#define USE_IS2D					u_Settings1.a

#define USE_LIGHTMAP				u_Settings2.r
#define USE_GLOW_BUFFER				u_Settings2.g
#define USE_CUBEMAP					u_Settings2.b
#define USE_TRIPLANAR				u_Settings2.a

#define USE_REGIONS					u_Settings3.r
#define USE_ISDETAIL				u_Settings3.g
#define USE_EMISSIVE_BLACK			u_Settings3.b
#define USE_GLOW_BLEND_MODE			u_Settings3.a

#define MAP_LIGHTMAP_MULTIPLIER		u_Settings4.r
#define MAP_LIGHTMAP_ENHANCEMENT	u_Settings4.g

#define MAP_COLOR_SWITCH_RG			u_Settings5.r
#define MAP_COLOR_SWITCH_RB			u_Settings5.g
#define MAP_COLOR_SWITCH_GB			u_Settings5.b
#define ENABLE_CHRISTMAS_EFFECT		u_Settings5.a


uniform vec4						u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4						u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4						u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4						u_Local4; // stageNum, glowStrength, r_showsplat, glowVibrancy
uniform vec4						u_Local5; // SHADER_HAS_OVERLAY, SHADER_ENVMAP_STRENGTH, 0.0, LEAF_ALPHA_MULTIPLIER
uniform vec4						u_Local9; // testvalue0, 1, 2, 3

uniform vec2						u_Dimensions;
uniform vec2						u_textureScale;

uniform vec3						u_ViewOrigin;
uniform vec4						u_MapInfo; // MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0
uniform vec4						u_Mins;
uniform vec4						u_Maxs;

uniform vec3						u_ColorMod;
uniform vec4						u_GlowMultiplier;

uniform float						u_Time;

uniform float						u_zFar;

#define MAP_MAX_HEIGHT				u_Maxs.b

#define SHADER_MAP_SIZE				u_Local1.r
#define SHADER_SWAY					u_Local1.g
#define SHADER_OVERLAY_SWAY			u_Local1.b
#define SHADER_MATERIAL_TYPE		u_Local1.a

#define SHADER_HAS_STEEPMAP			u_Local2.r
#define SHADER_HAS_WATEREDGEMAP		u_Local2.g
#define SHADER_HAS_NORMALMAP		u_Local2.b
#define SHADER_WATER_LEVEL			u_Local2.a

#define SHADER_HAS_SPLATMAP1		u_Local3.r
#define SHADER_HAS_SPLATMAP2		u_Local3.g
#define SHADER_HAS_SPLATMAP3		u_Local3.b
#define SHADER_HAS_SPLATMAP4		u_Local3.a

#define SHADER_STAGE_NUM			u_Local4.r
#define SHADER_GLOW_STRENGTH		u_Local4.g
#define SHADER_SHOW_SPLAT			u_Local4.b
#define SHADER_GLOW_VIBRANCY		u_Local4.a

#define SHADER_HAS_OVERLAY			u_Local5.r
#define SHADER_ENVMAP_STRENGTH		u_Local5.g
#define LEAF_ALPHA_MULTIPLIER		u_Local5.a



// TODO: Make mapinfo setting???
//#define LEAF_ALPHA_RANGE_LODS_ON_OTHERS
#define LEAF_ALPHA_RANGE				32768.0
#define LEAF_ALPHA_RANGE_MIN_LOD		1.0
#define LEAF_ALPHA_RANGE_MAX_LOD		16.0



#if defined(USE_TESSELLATION)

in vec3						Normal_FS_in;
in vec2						TexCoord_FS_in;
in vec2						envTC_FS_in;
in vec3						WorldPos_FS_in;
in vec3						ViewDir_FS_in;

in vec4						Color_FS_in;
in vec4						PrimaryLightDir_FS_in;
in vec2						TexCoord2_FS_in;

in vec3						Blending_FS_in;
flat in float				Slope_FS_in;


vec3 m_Normal 				= normalize(gl_FrontFacing ? -Normal_FS_in.xyz : Normal_FS_in.xyz);

#define m_TexCoords			TexCoord_FS_in
#define m_envTC				envTC_FS_in
#define m_vertPos			WorldPos_FS_in
#define m_ViewDir			ViewDir_FS_in

#define var_Color			Color_FS_in
#define	var_PrimaryLightDir PrimaryLightDir_FS_in
#define var_TexCoords2		TexCoord2_FS_in

#define var_Blending		Blending_FS_in
#define var_Slope			Slope_FS_in


#else //!defined(USE_TESSELLATION)

varying vec2				var_TexCoords;
varying vec2				var_TexCoords2;
varying vec2				var_envTC;
varying vec3				var_Normal;

varying vec4				var_Color;

varying vec4				var_PrimaryLightDir;

varying vec3				var_vertPos;

varying vec3				var_ViewDir;


varying vec3				var_Blending;
varying float				var_Slope;


vec3 m_Normal				= normalize(gl_FrontFacing ? -var_Normal : var_Normal);
#define m_TexCoords			var_TexCoords
#define m_envTC				var_envTC
#define m_vertPos			var_vertPos
#define m_ViewDir			var_ViewDir


#endif //defined(USE_TESSELLATION)



out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4 out_NormalDetail;
#endif //USE_REAL_NORMALMAPS


vec2 pxSize = vec2(1.0) / u_Dimensions;

vec2 EncodeNormal(vec3 n)
{
	float scale = 1.7777;
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;
	return enc;
}
vec3 DecodeNormal(vec2 enc)
{
	vec3 enc2 = vec3(enc.xy, 0.0);
	float scale = 1.7777;
	vec3 nn =
		enc2.xyz*vec3(2.0 * scale, 2.0 * scale, 0.0) +
		vec3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	return vec3(g * nn.xy, g - 1.0);
}

#ifdef _CHRISTMAS_LIGHTS_
float hash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float noise(in vec3 o, in float seed) 
{
	vec3 p = floor(o);
	vec3 fr = fract(o);
		
	float n = p.x + p.y*57.0 + p.z * seed;

	float a = hash(n+  0.0);
	float b = hash(n+  1.0);
	float c = hash(n+ 57.0);
	float d = hash(n+ 58.0);
	
	float e = hash(n+  0.0 + 1009.0);
	float f = hash(n+  1.0 + 1009.0);
	float g = hash(n+ 57.0 + 1009.0);
	float h = hash(n+ 58.0 + 1009.0);
	
	
	vec3 fr2 = fr * fr;
	vec3 fr3 = fr2 * fr;
	
	vec3 t = 3.0 * fr2 - 2.0 * fr3;
	
	float u = t.x;
	float v = t.y;
	float w = t.z;

	// this last bit should be refactored to the same form as the rest :)
	float res1 = a + (b-a)*u +(c-a)*v + (a-b+d-c)*u*v;
	float res2 = e + (f-e)*u +(g-e)*v + (e-f+h-g)*u*v;
	
	float res = res1 * (1.0- w) + res2 * (w);
	
	return res;
}

const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

float SmoothNoise( vec3 p, in float seed )
{
#if 0
    float f;
    f  = 0.5000*noise( p, seed ); p = m*p*2.02;
    f += 0.2500*noise( p, seed ); 
	
    return f * (1.0 / (0.5000 + 0.2500));
#else
	return noise(p, seed);
#endif
}
#endif //_CHRISTMAS_LIGHTS_


vec3 Vibrancy ( vec3 origcolor, float vibrancyStrength )
{
	vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);  				//Calculate luma with these values
	float	max_color = max(origcolor.r, max(origcolor.g,origcolor.b)); 	//Find the strongest color
	float	min_color = min(origcolor.r, min(origcolor.g,origcolor.b)); 	//Find the weakest color
	float	color_saturation = max_color - min_color; 						//Saturation is the difference between min and max
	float	luma = dot(lumCoeff, origcolor.rgb); 							//Calculate luma (grey)
	return mix(vec3(luma), origcolor.rgb, (1.0 + (vibrancyStrength * (1.0 - (sign(vibrancyStrength) * color_saturation))))); 	//Extrapolate between luma and original by 1 + (1-saturation) - current
}

float getspecularLight(vec3 n, vec3 l, vec3 e, float s) {
	//float nrm = (s + 8.0) / (3.1415 * 8.0);
	float ndotl = clamp(max(dot(reflect(e, n), l), 0.0), 0.1, 1.0);
	return clamp(pow(ndotl, s), 0.1, 1.0);// * nrm;
}

float getdiffuseLight(vec3 n, vec3 l, float p) {
	float ndotl = clamp(dot(n, l), 0.5, 0.9);
	return pow(ndotl, p);
}

#if defined(_HIGH_PASS_SHARPEN_)
vec3 Enhance(in sampler2D tex, in vec2 uv, vec3 color, float level)
{
	vec3 blur = textureLod(tex, uv, level).xyz;
	vec3 col = ((color - blur)*0.5 + 0.5);
	col *= ((color - blur)*0.25 + 0.25) * 8.0;
	col = col * color;
	return col;
}
#endif //defined(_HIGH_PASS_SHARPEN_)

void main()
{
	float dist = distance(m_vertPos.xyz, u_ViewOrigin.xyz);
	//float leafDistanceAlphaMod = 1.0;

	if (USE_IS2D <= 0.0 && dist > u_zFar && USE_VERTEX_ANIM < 1.0)
	{// Skip it all...
		gl_FragColor = vec4(0.0);
		out_Glow = vec4(0.0);
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
		return;
	}

	bool LIGHTMAP_ENABLED = (USE_LIGHTMAP > 0.0 && USE_GLOW_BUFFER != 1.0 && USE_IS2D <= 0.0) ? true : false;
	vec2 texCoords = m_TexCoords.xy;
	vec3 N = m_Normal.xyz;

	vec4 diffuse = vec4(0.0);

	diffuse = texture(u_DiffuseMap, texCoords);

	#if defined(_HIGH_PASS_SHARPEN_)
		if (USE_IS2D > 0.0 || USE_TEXTURECLAMP > 0.0)
		{
			diffuse.rgb = Enhance(u_DiffuseMap, texCoords, diffuse.rgb, 16.0);
		}
		/*
#ifdef LEAF_ALPHA_RANGE_LODS_ON_OTHERS
		else if (dist > LEAF_ALPHA_RANGE && diffuse.a < 1.0)
#else //!LEAF_ALPHA_RANGE_LODS_ON_OTHERS
		else if (SHADER_MATERIAL_TYPE == MATERIAL_GREENLEAVES && dist > LEAF_ALPHA_RANGE && diffuse.a < 1.0)
#endif //LEAF_ALPHA_RANGE_LODS_ON_OTHERS
		{// Try to fill out distant tree leafs, so they puff out and take up more pixels in the background. To both block vis better, and also make trees look less crappy...
			float lod = mix(LEAF_ALPHA_RANGE_MIN_LOD, LEAF_ALPHA_RANGE_MAX_LOD, clamp((dist-LEAF_ALPHA_RANGE) / u_zFar, 0.0, 1.0));
			vec4 lodColor = textureLod(u_DiffuseMap, texCoords, lod);
			float best = diffuse.a > lodColor.a ? 0.0 : 1.0;
			diffuse = mix(diffuse, lodColor, best);
			leafDistanceAlphaMod = lod * 64.0;
		}
		*/
		else
		{
			diffuse.rgb = Enhance(u_DiffuseMap, texCoords, diffuse.rgb, 8.0 + (gl_FragCoord.z * 8.0));
		}
	#endif //defined(_HIGH_PASS_SHARPEN_)

	// Alter colors by shader's colormod setting...
	diffuse.rgb += diffuse.rgb * u_ColorMod.rgb;


	if (SHADER_HAS_OVERLAY > 0.0)
	{// Blend the overlay...
		vec4 overlay = texture(u_OverlayMap, texCoords);

	/*#if defined(_HIGH_PASS_SHARPEN_)
		if (USE_IS2D > 0.0 || USE_TEXTURECLAMP > 0.0)
		{
			overlay.rgb = Enhance(u_OverlayMap, texCoords, overlay.rgb, 16.0);
		}
		else
		{
			overlay.rgb = Enhance(u_OverlayMap, texCoords, overlay.rgb, 8.0 + (gl_FragCoord.z * 8.0));
		}
	#endif //defined(_HIGH_PASS_SHARPEN_)*/

		// overlay map is always blended by diffuse rgb strength (multiplied by the overlay's alpha to support alphas)...
		vec3 oStr = clamp(diffuse.rgb * overlay.a * 0.5, 0.0, 1.0);
		diffuse.rgb = mix((diffuse.rgb * 0.5), overlay.rgb * oStr, diffuse.rgb * overlay.a);
	}

	if (SHADER_ENVMAP_STRENGTH > 0.0)
	{// Blend the overlay...
		vec4 env = texture(u_EnvironmentMap, m_envTC);

	/*#if defined(_HIGH_PASS_SHARPEN_)
		if (USE_IS2D > 0.0 || USE_TEXTURECLAMP > 0.0)
		{
			env.rgb = Enhance(u_EnvironmentMap, m_envTC, env.rgb, 16.0);
		}
		else
		{
			env.rgb = Enhance(u_EnvironmentMap, m_envTC, env.rgb, 8.0 + (gl_FragCoord.z * 8.0));
		}
	#endif //defined(_HIGH_PASS_SHARPEN_)*/

		diffuse.rgb = mix(diffuse.rgb, env.rgb, SHADER_ENVMAP_STRENGTH * env.a);
	}

	// Set alpha early so that we can cull early...
	gl_FragColor.a = clamp(diffuse.a * var_Color.a, 0.0, 1.0);


#ifdef USE_REAL_NORMALMAPS
	vec4 norm = vec4(0.0);

	if (USE_GLOW_BUFFER != 1.0 && USE_IS2D <= 0.0 && USE_ISDETAIL <= 0.0 && SHADER_HAS_NORMALMAP > 0.0)
	{
		norm = texture(u_NormalMap, texCoords);
		norm.a = 1.0;
	}
#endif //USE_REAL_NORMALMAPS


	vec3 ambientColor = vec3(0.0);
	vec3 lightColor = clamp(var_Color.rgb, 0.0, 1.0);

	if (LIGHTMAP_ENABLED)
	{// TODO: Move to screen space?
		vec4 lightmapColor = textureLod(u_LightMap, var_TexCoords2.st, 0.0);

		#if defined(RGBM_LIGHTMAP)
			lightmapColor.rgb *= lightmapColor.a;
		#endif //defined(RGBM_LIGHTMAP)

#define lm_const_1 ( 56.0 / 255.0)
#define lm_const_2 (255.0 / 200.0)

		if (MAP_LIGHTMAP_ENHANCEMENT <= 0.0)
		{
			float lmBrightMult = clamp(1.0 - (length(lightmapColor.rgb) / 3.0), 0.0, 0.9);

			lmBrightMult = clamp((clamp(lmBrightMult - lm_const_1, 0.0, 1.0)) * lm_const_2, 0.0, 1.0);
			lmBrightMult = lmBrightMult * 0.7;

			lmBrightMult *= MAP_LIGHTMAP_MULTIPLIER;

			lightColor = lightmapColor.rgb * lmBrightMult;

			ambientColor = lightColor;
			float surfNL = clamp(dot(var_PrimaryLightDir.xyz, N.xyz), 0.0, 1.0);
			lightColor /= clamp(max(surfNL, 0.25), 0.0, 1.0);
			ambientColor = clamp(ambientColor - lightColor * surfNL, 0.0, 1.0);
			lightColor *= lightmapColor.rgb;
			lightColor = clamp(lightColor * 133.333, 0.0, 1.0);
		}
		else if (MAP_LIGHTMAP_ENHANCEMENT == 1.0)
		{
			// Old style...
			float lmBrightMult = clamp(1.0 - (length(lightmapColor.rgb) / 3.0), 0.0, 0.9);

			lmBrightMult = clamp((clamp(lmBrightMult - lm_const_1, 0.0, 1.0)) * lm_const_2, 0.0, 1.0);
			lmBrightMult = lmBrightMult * 0.7;

			lmBrightMult *= MAP_LIGHTMAP_MULTIPLIER;

			lightColor = lightmapColor.rgb * lmBrightMult;

			ambientColor = lightColor;
			float surfNL = clamp(dot(var_PrimaryLightDir.xyz, N.xyz), 0.0, 1.0);
			lightColor /= clamp(max(surfNL, 0.25), 0.0, 1.0);
			ambientColor = clamp(ambientColor - lightColor * surfNL, 0.0, 1.0);
			lightColor *= lightmapColor.rgb;
			lightColor = clamp(lightColor * 133.333, 0.0, 1.0);


			// New style...
			vec3 lightColor2 = lightmapColor.rgb * MAP_LIGHTMAP_MULTIPLIER;
			vec3 E = normalize(m_ViewDir);
			vec3 bNorm = normalize(N.xyz + ((diffuse.rgb * 2.0 - 1.0) * -0.25)); // just add some fake bumpiness to it, fast as possible...

			float dif = min(getdiffuseLight(bNorm, var_PrimaryLightDir.xyz, 0.3), 0.15);
			vec3 ambientColor2 = clamp(dif * lightColor, 0.0, 1.0);

			float fre = pow(clamp(dot(bNorm, -E) + 1.0, 0.0, 1.0), 0.3);
			float spec = clamp(getspecularLight(bNorm, -var_PrimaryLightDir.xyz, E, 16.0) * fre, 0.05, 1.0) * 512.0;
			lightColor2 = clamp(spec * lightColor, 0.0, 1.0);

			// Mix them 50/50
			lightColor = (lightColor + lightColor2) * 0.5;
			ambientColor = (ambientColor + ambientColor2) * 0.5;
		}
		else
		{
			lightColor = lightmapColor.rgb * MAP_LIGHTMAP_MULTIPLIER;

			vec3 E = normalize(m_ViewDir);
			vec3 bNorm = normalize(N.xyz + ((diffuse.rgb * 2.0 - 1.0) * -0.25)); // just add some fake bumpiness to it, fast as possible...

			float dif = min(getdiffuseLight(bNorm, var_PrimaryLightDir.xyz, 0.3), 0.15);
			ambientColor = clamp(dif * lightColor, 0.0, 1.0);

			float fre = pow(clamp(dot(bNorm, -E) + 1.0, 0.0, 1.0), 0.3);
			float spec = clamp(getspecularLight(bNorm, -var_PrimaryLightDir.xyz, E, 16.0) * fre, 0.05, 1.0) * 512.0;
			lightColor = clamp(spec * lightColor, 0.0, 1.0);
		}
	}

	gl_FragColor.rgb = diffuse.rgb + ambientColor;


	if (USE_GLOW_BUFFER != 1.0 
		&& USE_IS2D <= 0.0 
		&& USE_VERTEX_ANIM <= 0.0
		&& USE_SKELETAL_ANIM <= 0.0
		&& SHADER_MATERIAL_TYPE != MATERIAL_LAVA
		&& SHADER_MATERIAL_TYPE != MATERIAL_SKY 
		&& SHADER_MATERIAL_TYPE != MATERIAL_SUN 
		&& SHADER_MATERIAL_TYPE != MATERIAL_EFX
		&& SHADER_MATERIAL_TYPE != MATERIAL_BLASTERBOLT
		&& SHADER_MATERIAL_TYPE != MATERIAL_FIRE
		&& SHADER_MATERIAL_TYPE != MATERIAL_SMOKE
		&& SHADER_MATERIAL_TYPE != MATERIAL_MAGIC_PARTICLES
		&& SHADER_MATERIAL_TYPE != MATERIAL_MAGIC_PARTICLES_TREE
		&& SHADER_MATERIAL_TYPE != MATERIAL_FIREFLIES
		&& SHADER_MATERIAL_TYPE != MATERIAL_GLASS
		&& SHADER_MATERIAL_TYPE != MATERIAL_DISTORTEDGLASS
		&& SHADER_MATERIAL_TYPE != MATERIAL_PORTAL
		&& SHADER_MATERIAL_TYPE != MATERIAL_MENU_BACKGROUND)
	{
		gl_FragColor.rgb = gl_FragColor.rgb * u_MapAmbient.rgb;
	}
	
	gl_FragColor.rgb *= clamp(lightColor, 0.0, 1.0);

#ifndef LEAF_ALPHA_RANGE_LODS_ON_OTHERS
	if (SHADER_MATERIAL_TYPE == MATERIAL_GREENLEAVES)
#endif //LEAF_ALPHA_RANGE_LODS_ON_OTHERS
	{// Amp up alphas on tree leafs, etc, so they draw at range instead of being blurred out...
		//gl_FragColor.a = clamp(gl_FragColor.a * LEAF_ALPHA_MULTIPLIER * leafDistanceAlphaMod, 0.0, 1.0);
		gl_FragColor.a = (gl_FragColor.a * LEAF_ALPHA_MULTIPLIER > 0.5) ? 1.0 : 0.0; // UQ1: Trying cutout method instead...
	}

	float alphaThreshold = (SHADER_MATERIAL_TYPE == MATERIAL_GREENLEAVES) ? SCREEN_MAPS_LEAFS_THRESHOLD : SCREEN_MAPS_ALPHA_THRESHOLD;
	bool isDetail = false;

	if (USE_ISDETAIL >= 1.0)
	{
		isDetail = true;
	}
	else if (gl_FragColor.a >= alphaThreshold 
		|| SHADER_MATERIAL_TYPE == 1024.0 
		|| SHADER_MATERIAL_TYPE == 1025.0 
		|| SHADER_MATERIAL_TYPE == MATERIAL_PUDDLE 
		|| SHADER_MATERIAL_TYPE == MATERIAL_EFX 
		|| SHADER_MATERIAL_TYPE == MATERIAL_GLASS
		|| SHADER_MATERIAL_TYPE == MATERIAL_DISTORTEDGLASS
		|| USE_IS2D > 0.0)
	{
		
	}
	else
	{
		isDetail = true;
	}
	
	if (USE_BLEND > 0.0)
	{// Emulate RGB blending... Fuck I hate this crap...
		float colStr = clamp(max(gl_FragColor.r, max(gl_FragColor.g, gl_FragColor.b)), 0.0, 1.0);

		/*if (USE_BLEND == 3.0)
		{
			gl_FragColor.a *= colStr * 2.0;
			gl_FragColor.rgb *= 0.5;
		}
		else*/ if (USE_BLEND == 2.0)
		{
			colStr = clamp(colStr + 0.1, 0.0, 1.0);
			gl_FragColor.a = 1.0 - colStr;
		}
		/*else
		{
			colStr = clamp(colStr - 0.1, 0.0, 1.0);
			gl_FragColor.a = colStr;
		}*/
	}

	/*if (SHADER_MATERIAL_TYPE == MATERIAL_GLASS && USE_IS2D <= 0.0)
	{
		gl_FragColor *= 6.0;
	}
	else if (SHADER_MATERIAL_TYPE == MATERIAL_EFX && USE_IS2D <= 0.0)
	{
		gl_FragColor *= 2.0;
	}*/

	gl_FragColor.a = clamp(gl_FragColor.a, 0.0, 1.0);

	if (gl_FragColor.a >= 0.99) gl_FragColor.a = 1.0; // Allow for rounding errors... Don't let them stop pixel culling...


	if (USE_EMISSIVE_BLACK > 0.0 && USE_GLOW_BUFFER <= 0.0)
	{
		gl_FragColor.rgb = vec3(0.0);
	}

	float useDisplacementMapping = 0.0;

	if (SHADER_MATERIAL_TYPE != MATERIAL_GREENLEAVES
		&& SHADER_MATERIAL_TYPE != MATERIAL_DRYLEAVES
		&& SHADER_MATERIAL_TYPE != MATERIAL_SHATTERGLASS
		&& SHADER_MATERIAL_TYPE != MATERIAL_BPGLASS
		&& SHADER_MATERIAL_TYPE != MATERIAL_ICE
		&& SHADER_MATERIAL_TYPE != MATERIAL_GLASS
		&& SHADER_MATERIAL_TYPE != MATERIAL_DISTORTEDGLASS
		&& SHADER_MATERIAL_TYPE != MATERIAL_EFX
		&& SHADER_MATERIAL_TYPE != MATERIAL_BLASTERBOLT
		&& SHADER_MATERIAL_TYPE != MATERIAL_FIRE
		&& SHADER_MATERIAL_TYPE != MATERIAL_SMOKE
		&& SHADER_MATERIAL_TYPE != MATERIAL_MAGIC_PARTICLES
		&& SHADER_MATERIAL_TYPE != MATERIAL_MAGIC_PARTICLES_TREE
		&& SHADER_MATERIAL_TYPE != MATERIAL_FIREFLIES
		&& SHADER_MATERIAL_TYPE != MATERIAL_PORTAL
		&& SHADER_MATERIAL_TYPE != MATERIAL_MENU_BACKGROUND
		&& SHADER_MATERIAL_TYPE != MATERIAL_SKY
		&& SHADER_MATERIAL_TYPE != MATERIAL_SUN
		&& USE_VERTEX_ANIM <= 0.0
		&& USE_SKELETAL_ANIM <= 0.0)
	{// TODO: Shader setting...
		useDisplacementMapping = 1.0;
	}

	if (MAP_COLOR_SWITCH_RG > 0.0)
	{
		gl_FragColor.rg = gl_FragColor.gr;
	}

	if (MAP_COLOR_SWITCH_RB > 0.0)
	{
		gl_FragColor.rb = gl_FragColor.br;
	}

	if (MAP_COLOR_SWITCH_GB > 0.0)
	{
		gl_FragColor.gb = gl_FragColor.bg;
	}

	#ifdef _CHRISTMAS_LIGHTS_
		if (SHADER_MATERIAL_TYPE == MATERIAL_GREENLEAVES && ENABLE_CHRISTMAS_EFFECT > 0.0 && gl_FragColor.a >= alphaThreshold)
		{
			float mapmult = 0.05;//0.01;
			float f = SmoothNoise(m_vertPos.xyz * mapmult, 1009.0);
			f = pow(f, 32.0);
			vec3 bri = pow(vec3(SmoothNoise(m_vertPos.yzx * mapmult, 1009.0), SmoothNoise(m_vertPos.xzy * mapmult, 1009.0), SmoothNoise(m_vertPos.zyx * mapmult, 1009.0)), vec3(4.0));
			vec4 lights = vec4(0.0);
			lights.rgb = bri*f*32.0;//512.0;
			lights.a = clamp(max(lights.r, max(lights.g, lights.b)), 0.0, 1.0);
			
			gl_FragColor = vec4(clamp(gl_FragColor.rgb + (lights.rgb * lights.a), 0.0, 1.0), gl_FragColor.a);
			out_Glow = vec4(lights);

			out_Position = vec4(m_vertPos.xyz, SHADER_MATERIAL_TYPE+1.0);
			out_Normal = vec4(vec3(EncodeNormal(N.xyz), 0.0), 1.0 );
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = norm;
	#endif //USE_REAL_NORMALMAPS
			return;
		}
	#endif //_CHRISTMAS_LIGHTS_

	vec4 specularGlow = vec4(0.0);

	if (USE_GLOW_BUFFER != 1.0 && USE_IS2D <= 0.0 && MAP_LIGHTMAP_ENHANCEMENT > 2.0)
	{// Add specular to bloom...
		vec3 specLightColor = gl_FragColor.rgb;//u_PrimaryLightColor.rgb * gl_FragColor.rgb;

		vec3 E = normalize(m_ViewDir);
		vec3 bNorm = normalize(N.xyz + ((diffuse.rgb * 2.0 - 1.0) * -0.25)); // just add some fake bumpiness to it, fast as possible...

		float fre = pow(clamp(dot(bNorm, -E) + 1.0, 0.0, 1.0), 0.3);
		float spec = clamp(getspecularLight(bNorm, -var_PrimaryLightDir.xyz, E, 16.0) * fre, 0.05, 1.0);
		specularGlow.rgb = clamp(spec * specLightColor * MAP_LIGHTMAP_MULTIPLIER, 0.0, 1.0);
		specularGlow.a = spec;
	}

#define glow_const_1 ( 23.0 / 255.0)
#define glow_const_2 (255.0 / 229.0)

	if (SHADER_MATERIAL_TYPE == 1024.0 || SHADER_MATERIAL_TYPE == 1025.0)
	{
		out_Glow = vec4(0.0);
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
			out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
	}
	else if (USE_GLOW_BUFFER >= 2.0 && USE_IS2D <= 0.0)
	{// Merged diffuse+glow stage...
		vec4 glowColor = max(texture(u_GlowMap, texCoords), 0.0);

	/*#if defined(_HIGH_PASS_SHARPEN_)
		if (USE_IS2D > 0.0 || USE_TEXTURECLAMP > 0.0)
		{
			glowColor.rgb = Enhance(u_GlowMap, texCoords, glowColor.rgb, 16.0);
		}
		else
		{
			glowColor.rgb = Enhance(u_GlowMap, texCoords, glowColor.rgb, 8.0 + (gl_FragCoord.z * 8.0));
		}
	#endif //defined(_HIGH_PASS_SHARPEN_)*/

		if (SHADER_MATERIAL_TYPE != MATERIAL_GLASS && length(glowColor.rgb) <= 0.0)
			glowColor.a = 0.0;

#define GLSL_BLEND_ALPHA			0
#define GLSL_BLEND_INVALPHA			1
#define GLSL_BLEND_DST_ALPHA		2
#define GLSL_BLEND_INV_DST_ALPHA	3
#define GLSL_BLEND_GLOWCOLOR		4
#define GLSL_BLEND_INV_GLOWCOLOR	5
#define GLSL_BLEND_DSTCOLOR			6
#define GLSL_BLEND_INV_DSTCOLOR		7

		if (USE_GLOW_BLEND_MODE == GLSL_BLEND_INV_DSTCOLOR)
		{
			glowColor.rgb = (glowColor.rgb * glowColor.a) * (vec3(1.0) - clamp(gl_FragColor.rgb, 0.0, 1.0));
		}
		else if (USE_GLOW_BLEND_MODE == GLSL_BLEND_DSTCOLOR)
		{
			glowColor.rgb = (glowColor.rgb * glowColor.a) * clamp(gl_FragColor.rgb, 0.0, 1.0);
		}
		else if (USE_GLOW_BLEND_MODE == GLSL_BLEND_INV_GLOWCOLOR)
		{
			glowColor.rgb = (glowColor.rgb * glowColor.a) * (vec3(1.0) - glowColor.rgb);
		}
		else if (USE_GLOW_BLEND_MODE == GLSL_BLEND_GLOWCOLOR)
		{
			glowColor.rgb = (glowColor.rgb * glowColor.a) * glowColor.rgb;
		}
		else if (USE_GLOW_BLEND_MODE == GLSL_BLEND_INV_DST_ALPHA)
		{
			glowColor.rgb = (glowColor.rgb * glowColor.a) * (1.0 - clamp(gl_FragColor.a, 0.0, 1.0));
		}
		else if (USE_GLOW_BLEND_MODE == GLSL_BLEND_DST_ALPHA)
		{
			glowColor.rgb = (glowColor.rgb * glowColor.a) * clamp(gl_FragColor.a, 0.0, 1.0);
		}
		else if (USE_GLOW_BLEND_MODE == GLSL_BLEND_INVALPHA)
		{
			glowColor.rgb = (glowColor.rgb * (1.0 - clamp(glowColor.a, 0.0, 1.0)));
		}
		else
		{
			glowColor.rgb = (glowColor.rgb * glowColor.a);
		}


		if (SHADER_GLOW_VIBRANCY != 0.0)
		{
			glowColor.rgb = Vibrancy( glowColor.rgb, SHADER_GLOW_VIBRANCY );
		}

		glowColor.rgb = clamp((clamp(glowColor.rgb - glow_const_1, 0.0, 1.0)) * glow_const_2, 0.0, 1.0);
		float glowRamp = ((min(dist, 16384.0) / 16384.0) * 8.0) + 1.0;
		glowColor.rgb *= glowRamp;

		glowColor.rgb *= SHADER_GLOW_STRENGTH;

		if (SHADER_MATERIAL_TYPE != MATERIAL_GLASS && SHADER_MATERIAL_TYPE != MATERIAL_BLASTERBOLT && SHADER_MATERIAL_TYPE != MATERIAL_EFX && length(glowColor.rgb) <= 0.0)
			glowColor.a = 0.0;

		glowColor += specularGlow;

		float glowMax = clamp(length(glowColor.rgb) / 3.0, 0.0, 1.0);
		glowColor.a *= glowMax;
		glowColor.rgb *= glowColor.a;

		glowColor.a = clamp(glowColor.a, 0.0, 1.0);
		out_Glow = glowColor * u_GlowMultiplier;
		
		//out_Glow.rgb = clamp(out_Glow.rgb, 0.0, 2.0); // cap stregnth for the sake of the buffer

		gl_FragColor.rgb = mix(gl_FragColor.rgb, glowColor.rgb, glowColor.a);
		gl_FragColor.a = max(gl_FragColor.a, glowColor.a);

		if (isDetail)
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
		else if (SHADER_MATERIAL_TYPE == MATERIAL_EFX || SHADER_MATERIAL_TYPE == MATERIAL_GLASS)
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
		else if (USE_BLEND != 4.0 && gl_FragColor.a >= alphaThreshold || SHADER_MATERIAL_TYPE == 1024.0 || SHADER_MATERIAL_TYPE == 1025.0 || SHADER_MATERIAL_TYPE == MATERIAL_PUDDLE)
		{
			out_Position = vec4(m_vertPos.xyz, SHADER_MATERIAL_TYPE+1.0);
			out_Normal = vec4(vec3(EncodeNormal(N.xyz), 0.0), 1.0 );
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = norm;
	#endif //USE_REAL_NORMALMAPS
		}
		else
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
	}
	else if (USE_GLOW_BUFFER > 0.0 && USE_IS2D <= 0.0)
	{
		vec4 glowColor = max(gl_FragColor, 0.0);

		if (SHADER_MATERIAL_TYPE != MATERIAL_GLASS && length(glowColor.rgb) <= 0.0)
			glowColor.a = 0.0;

		if (SHADER_GLOW_VIBRANCY != 0.0)
		{
			glowColor.rgb = Vibrancy( glowColor.rgb, SHADER_GLOW_VIBRANCY );
		}

		glowColor.rgb = clamp((clamp(glowColor.rgb - glow_const_1, 0.0, 1.0)) * glow_const_2, 0.0, 1.0);
		float glowRamp = ((min(dist, 16384.0) / 16384.0) * 8.0) + 1.0;
		glowColor.rgb *= glowRamp;

		glowColor.rgb *= SHADER_GLOW_STRENGTH;

		if (SHADER_MATERIAL_TYPE != MATERIAL_GLASS && SHADER_MATERIAL_TYPE != MATERIAL_BLASTERBOLT && SHADER_MATERIAL_TYPE != MATERIAL_EFX && length(glowColor.rgb) <= 0.0)
			glowColor.a = 0.0;

		glowColor += specularGlow;

		glowColor.a = clamp(glowColor.a, 0.0, 1.0);
		out_Glow = glowColor * u_GlowMultiplier;

		//out_Glow.rgb = clamp(out_Glow.rgb, 0.0, 2.0); // cap stregnth for the sake of the buffer

		gl_FragColor.rgb = glowColor.rgb;
		gl_FragColor.a = max(gl_FragColor.a, glowColor.a);

		if (isDetail)
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
		else if (SHADER_MATERIAL_TYPE == MATERIAL_EFX || SHADER_MATERIAL_TYPE == MATERIAL_GLASS)
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
		else if (USE_BLEND != 4.0 && gl_FragColor.a >= alphaThreshold || SHADER_MATERIAL_TYPE == 1024.0 || SHADER_MATERIAL_TYPE == 1025.0 || SHADER_MATERIAL_TYPE == MATERIAL_PUDDLE)
		{
			out_Position = vec4(m_vertPos.xyz, SHADER_MATERIAL_TYPE+1.0);
			out_Normal = vec4(vec3(EncodeNormal(N.xyz), 0.0), 1.0 );
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
		else
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
	}
	else
	{
		out_Glow = specularGlow;//vec4(0.0);

		if (isDetail)
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
		else if (SHADER_MATERIAL_TYPE == MATERIAL_EFX || SHADER_MATERIAL_TYPE == MATERIAL_GLASS)
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
		else if (USE_BLEND != 4.0 && gl_FragColor.a >= alphaThreshold || SHADER_MATERIAL_TYPE == 1024.0 || SHADER_MATERIAL_TYPE == 1025.0 || SHADER_MATERIAL_TYPE == MATERIAL_PUDDLE)
		{
			out_Position = vec4(m_vertPos.xyz, SHADER_MATERIAL_TYPE+1.0);
			out_Normal = vec4(vec3(EncodeNormal(N.xyz), useDisplacementMapping), 1.0 );
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = norm;
	#endif //USE_REAL_NORMALMAPS
		}
		else
		{
			out_Position = vec4(0.0);
			out_Normal = vec4(0.0);
	#ifdef USE_REAL_NORMALMAPS
				out_NormalDetail = vec4(0.0);
	#endif //USE_REAL_NORMALMAPS
		}
	}
}
