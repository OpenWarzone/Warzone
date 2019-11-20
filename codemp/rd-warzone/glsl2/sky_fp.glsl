#define __HIGH_PASS_SHARPEN__
#define __CLOUDS__
//#define __CLOUDS2__
#define __LIGHTNING__
#define __BACKGROUND_HILLS__
//#define __TEST_SKY__
#define __AURORA2__
#define __RAINBOWS__

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
uniform sampler2D										u_DiffuseMap;
uniform sampler2D										u_OverlayMap; // Night sky image... When doing sky...
uniform sampler2D										u_SplatMap1; // auroraImage[0]
uniform sampler2D										u_SplatMap2; // auroraImage[1]
uniform sampler2D										u_SplatMap3; // smoothNoiseImage
uniform sampler2D										u_RoadMap; // random2KImage
uniform sampler2D										u_MoonMaps[4]; // moon textures
uniform sampler3D										u_VolumeMap; // volumetricRandom
#endif //defined(USE_BINDLESS_TEXTURES)

uniform int												u_MoonCount; // moons total count
uniform vec4											u_MoonInfos[4]; // MOON_ENABLED, MOON_ROTATION_OFFSET_X, MOON_ROTATION_OFFSET_Y, MOON_SIZE
uniform vec2											u_MoonInfos2[4]; // MOON_BRIGHTNESS, MOON_TEXTURE_SCALE

uniform vec4											u_Settings0; // useTC, useDeform, useRGBA, isTextureClamped
uniform vec4											u_Settings1; // useVertexAnim, useSkeletalAnim, blendMode, is2D
uniform vec4											u_Settings2; // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
uniform vec4											u_Settings3; // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL, 0=DetailMapNormal 1=detailMapFromTC 2=detailMapFromWorld, 0.0

#define USE_TC											u_Settings0.r
#define USE_DEFORM										u_Settings0.g
#define USE_RGBA										u_Settings0.b
#define USE_TEXTURECLAMP								u_Settings0.a

#define USE_VERTEX_ANIM									u_Settings1.r
#define USE_SKELETAL_ANIM								u_Settings1.g
#define USE_BLEND										u_Settings1.b
#define USE_IS2D										u_Settings1.a

#define USE_LIGHTMAP									u_Settings2.r
#define USE_GLOW_BUFFER									u_Settings2.g
#define USE_CUBEMAP										u_Settings2.b
#define USE_TRIPLANAR									u_Settings2.a

#define USE_REGIONS										u_Settings3.r
#define USE_ISDETAIL									u_Settings3.g
#define USE_DETAIL_COORD								u_Settings3.b

uniform vec4											u_Local1; // PROCEDURAL_SKY_ENABLED, DAY_NIGHT_24H_TIME/24.0, PROCEDURAL_SKY_STAR_DENSITY, PROCEDURAL_SKY_NEBULA_SEED
uniform vec4											u_Local2; // PROCEDURAL_CLOUDS_ENABLED, PROCEDURAL_CLOUDS_CLOUDSCALE, PROCEDURAL_CLOUDS_CLOUDCOVER, 0.0 /*UNUSED*/
uniform vec4											u_Local3; // PROCEDURAL_SKY_SUNSET_COLOR_R, PROCEDURAL_SKY_SUNSET_COLOR_G, PROCEDURAL_SKY_SUNSET_COLOR_B, PROCEDURAL_SKY_SUNSET_STRENGTH
uniform vec4											u_Local4; // PROCEDURAL_SKY_NIGHT_HDR_MIN, PROCEDURAL_SKY_NIGHT_HDR_MAX, PROCEDURAL_SKY_PLANETARY_ROTATION, PROCEDURAL_SKY_NEBULA_FACTOR
uniform vec4											u_Local5; // dayNightEnabled, nightScale, skyDirection, auroraEnabled -- Sky draws only!
uniform vec4											u_Local6; // PROCEDURAL_SKY_DAY_COLOR, 0.0 /*UNUSED*/
uniform vec4											u_Local7; // PROCEDURAL_SKY_NIGHT_COLOR
uniform vec4											u_Local8; // AURORA_COLOR_R, AURORA_COLOR_G, AURORA_COLOR_B, 0.0 /*UNUSED*/
uniform vec4											u_Local9; // testvalue0, 1, 2, 3
uniform vec4											u_Local10; // PROCEDURAL_BACKGROUND_HILLS_ENABLED, PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS, PROCEDURAL_BACKGROUND_HILLS_UPDOWN, PROCEDURAL_BACKGROUND_HILLS_SEED
uniform vec4											u_Local11; // PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR
uniform vec4											u_Local12; // PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2
uniform vec4											u_Local13; // AURORA_STRENGTH1, AURORA_STRENGTH2, DYNAMIC_WEATHER_PUDDLE_STRENGTH, 0.0

#define PROCEDURAL_SKY_ENABLED							u_Local1.r
#define DAY_NIGHT_24H_TIME								u_Local1.g
#define PROCEDURAL_SKY_STAR_DENSITY						u_Local1.b
#define PROCEDURAL_SKY_NEBULA_SEED						u_Local1.a

#define CLOUDS_ENABLED									u_Local2.r
#define CLOUDS_CLOUDSCALE								u_Local2.g
#define CLOUDS_CLOUDCOVER								u_Local2.b
//#define CLOUDS_DARK									u_Local2.a

#define PROCEDURAL_SKY_SUNSET_COLOR						u_Local3.rgb
#define PROCEDURAL_SKY_SUNSET_STRENGTH					u_Local3.a

#define PROCEDURAL_SKY_NIGHT_HDR_MIN					u_Local4.r
#define PROCEDURAL_SKY_NIGHT_HDR_MAX					u_Local4.g
#define PROCEDURAL_SKY_PLANETARY_ROTATION				u_Local4.b
#define PROCEDURAL_SKY_NEBULA_FACTOR					u_Local4.a

#define SHADER_DAY_NIGHT_ENABLED						u_Local5.r
#define SHADER_NIGHT_SCALE								u_Local5.g
#define SHADER_SKY_DIRECTION							u_Local5.b
#define SHADER_AURORA_ENABLED							u_Local5.a

#define PROCEDURAL_SKY_DAY_COLOR						u_Local6.rgb
#define PROCEDURAL_SKY_NIGHT_COLOR						u_Local7.rgba

#define AURORA_COLOR									u_Local8.rgb

#define PROCEDURAL_BACKGROUND_HILLS_ENABLED 			u_Local10.r
#define PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS			u_Local10.g
#define PROCEDURAL_BACKGROUND_HILLS_UPDOWN				u_Local10.b
#define PROCEDURAL_BACKGROUND_HILLS_SEED				u_Local10.a

#define PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR		u_Local11.rgb

#define PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2	u_Local12.rgb

#define AURORA_STRENGTH1								u_Local13.r
#define AURORA_STRENGTH2								u_Local13.g
#define DYNAMIC_WEATHER_PUDDLE_STRENGTH					u_Local13.b


uniform vec2						u_Dimensions;
uniform vec3						u_ViewOrigin;
uniform vec4						u_PrimaryLightOrigin;
uniform vec3						u_PrimaryLightColor;
uniform float						u_Time;


varying vec2						var_TexCoords;
varying vec3						var_Position;
varying vec3						var_Normal;
varying vec4						var_Color;

out vec4							out_Glow;
out vec4							out_Position;
out vec4							out_Normal;
#ifdef __USE_REAL_NORMALMAPS__
out vec4							out_NormalDetail;
#endif //__USE_REAL_NORMALMAPS__

//#define __ENCODE_NORMALS_RECONSTRUCT_Z__
#define __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
//#define __ENCODE_NORMALS_CRY_ENGINE__
//#define __ENCODE_NORMALS_EQUAL_AREA_PROJECTION__

#ifdef __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
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
#elif defined(__ENCODE_NORMALS_CRY_ENGINE__)
vec3 DecodeNormal(in vec2 N)
{
	vec2 encoded = N * 4.0 - 2.0;
	float f = dot(encoded, encoded);
	float g = sqrt(1.0 - f * 0.25);
	return vec3(encoded * g, 1.0 - f * 0.5);
}
vec2 EncodeNormal(in vec3 N)
{
	float f = sqrt(8.0 * N.z + 8.0);
	return N.xy / f + 0.5;
}
#elif defined(__ENCODE_NORMALS_EQUAL_AREA_PROJECTION__)
vec2 EncodeNormal(vec3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);
	return n.xy / f + 0.5;
}
vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 n;
	n.xy = fenc*g;
	n.z = 1.0 - f / 2.0;
	return n;
}
#else //__ENCODE_NORMALS_RECONSTRUCT_Z__
vec3 DecodeNormal(in vec2 N)
{
	vec3 norm;
	norm.xy = N * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy));
	return norm;
}
vec2 EncodeNormal(vec3 n)
{
	return vec2(n.xy * 0.5 + 0.5);
}
#endif //__ENCODE_NORMALS_RECONSTRUCT_Z__


#define MOD2 vec2(.16632,.17369)
#define MOD3 vec3(.16532,.17369,.15787)

//--------------------------------------------------------------------------
float Hash( float p )
{
	vec2 p2 = fract(vec2(p) * MOD2);
	p2 += dot(p2.yx, p2.xy+19.19);
	return fract(p2.x * p2.y);
}

float Hash( vec3 p )
{
	p  = fract(p * MOD3);
	p += dot(p.xyz, p.yzx + 19.19);
	return fract(p.x * p.y * p.z);
}

//--------------------------------------------------------------------------

float pNoise( in vec2 x )
{
	vec2 p = floor(x);
	vec2 f = fract(x);
	f = f*f*(3.0-2.0*f);
	float n = p.x + p.y*57.0;
	float res = mix(mix( Hash(n+  0.0), Hash(n+  1.0),f.x),
					mix( Hash(n+ 57.0), Hash(n+ 58.0),f.x),f.y);
	return res;
}

float pNoise(in vec3 p)
{
	vec3 i = floor(p);
	vec3 f = fract(p); 
	f *= f * (3.0-2.0*f);

	return mix(
		mix(mix(Hash(i + vec3(0.,0.,0.)), Hash(i + vec3(1.,0.,0.)),f.x),
			mix(Hash(i + vec3(0.,1.,0.)), Hash(i + vec3(1.,1.,0.)),f.x),
			f.y),
		mix(mix(Hash(i + vec3(0.,0.,1.)), Hash(i + vec3(1.,0.,1.)),f.x),
			mix(Hash(i + vec3(0.,1.,1.)), Hash(i + vec3(1.,1.,1.)),f.x),
			f.y),
		f.z);
}

float tNoise(in vec2 p)
{
	return textureLod(u_RoadMap, p, 0.0).r;
}

float tNoise(in vec3 p)
{
	return textureLod(u_RoadMap, (p.xz+p.y)*0.0005, 0.0).r;
}

float cNoise(in vec3 p)
{
	return texture(u_VolumeMap, p, 0.0).r * mix(0.75, 2.0, pow(1.0 - CLOUDS_CLOUDCOVER, 2.0));// 0.75 (max cloud cover) -> 2.0 (clear skies)
}

const mat3 mcl = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 ) * 2.0;// * 1.7;

//--------------------------------------------------------------------------
float FBM( vec3 p )
{
	p *= CLOUDS_CLOUDSCALE;

	float f;
	
#if 1
	p *= 0.00000125;
	f = 0.5000 * cNoise(p); p = mcl*p;
	f += 0.2500 * cNoise(p); p = mcl*p;
	f += 0.1250 * cNoise(p); p = mcl*p;
	f += 0.0625   * cNoise(p); p = mcl*p;
	f += 0.03125  * cNoise(p); p = mcl*p;
	f += 0.015625 * cNoise(p);
#else
	p *= .0005;
	f = 0.5000 * cNoise(p); p = mcl*p;
	f += 0.2500 * cNoise(p); p = mcl*p;
	f += 0.1250 * cNoise(p); p = mcl*p;
	f += 0.0625 * cNoise(p);
#endif

	return f;
}

//--------------------------------------------------------------------------
/*float FBMSH( vec3 p )
{
	p *= .0005;
	p *= CLOUDS_CLOUDSCALE;

	float f;
	
	f = 0.5000 * pNoise(p); p = mcl*p;
	f += 0.2500 * pNoise(p); p = mcl*p;
	f += 0.1250 * pNoise(p); p = mcl*p;
	f += 0.0625 * pNoise(p);
	return f;
}*/

vec3 extra_cheap_atmosphere(vec3 raydir, vec3 skyViewDir2, vec3 sunDir, inout vec3 sunColorMod) {
	vec3 sundir = sunDir;
	sundir.y = abs(sundir.y);
	float sunDirLength = pow(clamp(length(sundir.y), 0.0, 1.0), 2.25);
	float rayDirLength = pow(clamp(length(raydir.y), 0.0, 1.0), 0.85);
	float special_trick = 1.0 / (rayDirLength * 1.0 + 0.2);
	float special_trick2 = 1.0 / (length(raydir.y) * 3.0 + 1.0);
	
	vec3 bluesky = PROCEDURAL_SKY_DAY_COLOR;
	vec3 bluesky2 = max(bluesky, bluesky - PROCEDURAL_SKY_DAY_COLOR * 0.0896 * (special_trick + -6.0 * sunDirLength * sunDirLength));
	
	float dotSun = dot(sundir, raydir);
	float raysundt = pow(abs(dotSun), 2.0);
	bluesky2 *= special_trick * (0.24 + raysundt * 0.24);

	bluesky = clamp(bluesky, 0.0, 1.0);
	bluesky2 = clamp(bluesky2, 0.0, 1.0);

	// sunset
	float sunsetIntensity = 0.9;
	float sundt = smoothstep(0.0, 1.0, pow(clamp(max(0.0, dotSun), 0.0, 1.0), 3.0));
	float sunsetPhase = 1.0 - clamp(pow(sunDirLength, 0.3), 0.0, 1.0);
	float my = clamp(0.75-length(raydir.y*0.75), 0.0, 1.0) * sunsetPhase;
	float mymie = pow(clamp(clamp(sundt, 0.1, 1.0) * (special_trick2 * 0.05 + 0.95), 0.0, sunsetIntensity), 0.75) * sunsetPhase * my;
	vec3 color = mix((bluesky * 0.333 + bluesky2 * 0.333), PROCEDURAL_SKY_SUNSET_COLOR * PROCEDURAL_SKY_SUNSET_STRENGTH, mymie);
	sunColorMod = clamp(color, 0.0, 1.0);
	return color * 0.5;
}

#if defined(__HIGH_PASS_SHARPEN__)
vec3 Enhance(in sampler2D tex, in vec2 uv, vec3 color, float level)
{
	vec3 blur = textureLod(tex, uv, level).xyz;
	vec3 col = ((color - blur)*0.5 + 0.5) * 1.0;
	col *= ((color - blur)*0.25 + 0.25) * 8.0;
	col = mix(color, col * color, 1.0);
	return col;
}
#endif //defined(__HIGH_PASS_SHARPEN__)


#ifdef __CLOUDS__

//#define RAY_TRACE_STEPS 55

#if defined(CLOUD_QUALITY4)
	#define RAY_TRACE_STEPS 8
	#define CLOUD_LOWER 2800.0
	#define CLOUD_UPPER 26800.0
#elif defined(CLOUD_QUALITY3)
	#define RAY_TRACE_STEPS 6
	#define CLOUD_LOWER 2800.0
	#define CLOUD_UPPER 18800.0
#elif defined(CLOUD_QUALITY2)
	#define RAY_TRACE_STEPS 4
	#define CLOUD_LOWER 2800.0
	#define CLOUD_UPPER 10800.0
#else //if defined(CLOUD_QUALITY1)
	#define RAY_TRACE_STEPS 2
	#define CLOUD_LOWER 2800.0
	#define CLOUD_UPPER 6800.0
#endif

vec3 sunLight  = normalize( u_PrimaryLightOrigin.xzy - u_ViewOrigin.xzy );
vec3 sunColour = u_PrimaryLightColor.rgb;

float gTime;
float cloudy = 0.0;
float cloudShadeFactor = 0.6;
float flash = 0.0;


/* -------------------*/
#define haze  0.01 * (CLOUDS_CLOUDCOVER*20.)
//#define rainmulti 5.0 // makes clouds thicker
//#define rainy (10.0 -rainmulti)
//#define t u_Time
//#define fov tan(radians(60.0))
//#define S(x, y, z) smoothstep(x, y, z)
//#define cameraheight 5e1 //50.

const float R0 = 6360e3; //planet radius //6360e3 actual 6371km
const float Ra = 6380e3; //atmosphere radius //6380e3 troposphere 8 to 14.5km
const float I = 10.; //sun light power, 10.0 is normal
const float SI = 5.; //sun intensity for sun
const float g = 0.45; //light concentration .76 //.45 //.6  .45 is normaL
const float g2 = g * g;

//const float ts= (cameraheight / 2.5e5);

const float s = 0.999; //light concentration for sun
#if SOFT_SUN
const float s2 = s;
#else
const float s2 = s * s;
#endif

const float Hr = 8e3; //Rayleigh scattering top //8e3
const float Hm = 1.2e3; //Mie scattering top //1.3e3

vec3 bM = vec3(21e-6); //normal mie // vec3(21e-6)
//vec3 bM = vec3(50e-6); //high mie

//Rayleigh scattering (sky color, atmospheric up to 8km)
vec3 bR = vec3(5.8e-6, 13.5e-6, 33.1e-6); //normal earth
//vec3 bR = vec3(5.8e-6, 33.1e-6, 13.5e-6); //purple
//vec3 bR = vec3( 63.5e-6, 13.1e-6, 50.8e-6 ); //green
//vec3 bR = vec3( 13.5e-6, 23.1e-6, 115.8e-6 ); //yellow
//vec3 bR = vec3( 5.5e-6, 15.1e-6, 355.8e-6 ); //yeellow
//vec3 bR = vec3(3.5e-6, 333.1e-6, 235.8e-6 ); //red-purple

vec3 C = vec3(0., -R0, 0.); //planet center
vec3 Ds = normalize(u_PrimaryLightOrigin.xzy);//normalize(vec3(0., 0., -1.)); //sun direction?
/* -------------------*/


//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
float MapSH(vec3 p)
{
	float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-cloudShadeFactor);
	h *= smoothstep(CLOUD_UPPER+100., CLOUD_UPPER, p.y);
	return h;
}

float Map(vec3 p)
{
	float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-cloudShadeFactor);
	return h;
}


//--------------------------------------------------------------------------
float GetLighting(vec3 p, vec3 s)
{
    float l = MapSH(p)-MapSH(p+s*200.0);
    return clamp(-l, 0.1, 0.4) * 1.25;
}

//#define EXPERIMENTAL_CLOUD_COLOR
//#define EXPERIMENTAL_CLOUD_COLOR2

//--------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
vec4 GetSky(in vec3 pos,in vec3 rd, out vec2 outPos)
{
	//float sunAmount = max( dot( rd, sunLight), 0.0 );

	//float n = texture(u_VolumeMap, pos+rd * u_Local9.r, 0.0).r * mix(0.75, 2.0, pow(1.0 - CLOUDS_CLOUDCOVER, 2.0));// 0.75 (max cloud cover) -> 2.0 (clear skies)
	//return vec4(n);
	//return vec4(texture(u_VolumeMap, pos+rd * u_Local9.r, 0.0).rrr, 1.0);
	
	// Find the start and end of the cloud layer...
	float beg = ((CLOUD_LOWER-pos.y) / rd.y);
	float end = ((CLOUD_UPPER-pos.y) / rd.y);
	
	// Start position...
	vec3 p = vec3(pos.x + rd.x * beg, 0.0, pos.z + rd.z * beg);
	outPos = p.xz;
    beg +=  Hash(p)*150.0;

	// Trace clouds through that layer...
	float d = 0.0;
	vec3 posAdd = rd * ((end-beg) / float(RAY_TRACE_STEPS));

	vec2 shade;
	vec2 shadeSum = vec2(0.0);
	shade.x = 1.0;

#ifdef EXPERIMENTAL_CLOUD_COLOR
	float bz = 0.0;
#endif //EXPERIMENTAL_CLOUD_COLOR
	
	// I think this is as small as the loop can be
	// for a reasonable cloud density illusion.
	for (int i = 0; i < RAY_TRACE_STEPS; i++)
	{
		if (shadeSum.y >= 1.0) break;

		float h = clamp(Map(p)*2.0*(2.0 / float(RAY_TRACE_STEPS)), 0.0, 1.0);
		shade.y = max(h, 0.0);
		shade.x = GetLighting(p, sunLight);
		shadeSum += shade * (1.0 - shadeSum.y);

#ifdef EXPERIMENTAL_CLOUD_COLOR
		if (i == 0) bz = shade.y;
#endif //EXPERIMENTAL_CLOUD_COLOR

		p += posAdd;
	}

	float final = shadeSum.x;
	final += flash * (shadeSum.y+final+.2) * .5;

#ifdef EXPERIMENTAL_CLOUD_COLOR
	vec4 col = vec4(final, final, final, shadeSum.y);
	col = clamp(col, 0.0, 1.0);

	/*
	float sun = clamp(dot(sunLight, rd), 0.0, 1.0 );
	float s2p = distance(p + posAdd, sunLight * 100.0);
	float tot = shadeSum.y;
	//col = mix(col, vec3(0.5, 0.5, 0.55) * 0.2, pow(bz, 1.5)); // darkens (back-shadows)
	//tot = smoothstep(-7.5, -0.0, 1.0 - tot);
	
	//vec3 sccol = mix(vec3(0.11, 0.1, 0.2), vec3(0.2, 0.0, 0.1), smoothstep(0.0, 900.0, s2p));
	vec3 sccol = mix(vec3(0.11, 0.1, 0.2), vec3(0.2, 0.0, 0.1), smoothstep(0.0, 1.0, s2p));

	col.rgb = mix(col.rgb, sccol, 1.0 - tot) * 1.6; // darkens (nightish)
	
	//vec3 sncol = mix(vec3(1.4, 0.3, 0.0), vec3(1.5, 0.65, 0.0), smoothstep(0.0, 1200.0, s2p));
	vec3 sncol = mix(vec3(1.4, 0.3, 0.0), vec3(1.5, 0.65, 0.0), smoothstep(0.0, 1.0, s2p));

	float sd = pow(sun, 10.0) + 0.7;
	//col.rgb += sncol * bz * bz * bz * tot * tot * tot * sd; // (sunlight coloring)
	col.rgb = mix(col.rgb, sncol, bz * bz * bz * tot * tot * tot * sd);
	*/

	float sunDot = dot(sunLight, rd);
	
	if (sunDot >= 0.0)
	{// Looking towards sun dir...
		float dirPwr = clamp(pow(length(sunDot), 14.95), 0.0, 1.0);
		float alphaPwr = 1.0 - col.a;
		float sunPwr = alphaPwr * dirPwr;
	}
	else
	{// Looking away from sun dir...

	}

	return col;
#else //!EXPERIMENTAL_CLOUD_COLOR
	return clamp(vec4(final, final, final, shadeSum.y), 0.0, 1.0);
#endif //EXPERIMENTAL_CLOUD_COLOR
}

//--------------------------------------------------------------------------
vec4 Clouds(float colorMult)
{
	vec3 cameraPos = vec3(0.0);
	vec3 dir = normalize(u_ViewOrigin.xzy - var_Position.xzy*1025.0);
	float alphaMult = clamp(-dir.y, 0.0, 0.75);

	if (alphaMult <= 0.0)
	{// No point in doing all this math if it's not going to draw anything...
		return vec4(0.0);
	}

	gTime = u_Time*.5 + 75.5;
	cloudy = clamp(CLOUDS_CLOUDCOVER*0.3, 0.0, 0.3);
	cloudShadeFactor = 0.5+(cloudy*0.333);
    float lightning = 0.0;
    
    
	if (cloudy >= 0.275)
    {
        float f = mod(gTime+1.5, 2.5);
        if (f < .8)
        {
            f = smoothstep(.8, .0, f)* 1.5;
        	lightning = mod(-gTime*(1.5-Hash(gTime*.3)*.002), 1.0) * f;
        }
    }
    
    //flash = clamp(vec3(1., 1.0, 1.2) * lightning, 0.0, 1.0);
	flash = clamp(lightning, 0.0, 1.0);
	
	
	vec4 origCol;
	vec2 pos;
	vec4 col = GetSky(cameraPos, dir, pos);
#ifdef EXPERIMENTAL_CLOUD_COLOR2
	origCol = col;

	vec3 sunPixel = normalize(mix(dir, sunLight, 0.001));
	vec2 pos2;
	vec4 sunPixelCol = GetSky(cameraPos, sunPixel, pos2);
	col.rgb = mix(col.rgb, sunPixelCol.rgb, 0.5);
#endif //EXPERIMENTAL_CLOUD_COLOR2

	col.rgb = clamp(col.rgb * (128.0/*64.0*/ * colorMult), 0.0, 1.0);

	float l = exp(-length(pos) * .00002);
	col.rgb = mix(vec3(.6-cloudy*1.2)+flash*.3, col.rgb, max(l, .2));

	// Stretch RGB upwards... 
	col.rgb = pow(col.rgb, vec3(.7));

#ifdef EXPERIMENTAL_CLOUD_COLOR2
	float nFac = length(normalize(u_PrimaryLightOrigin.xzy).y);

	// Add sunset coloring...
	if (nFac > 0.0 && nFac < 0.5 && sunPixelCol.a < origCol.a)
	{//	This direction is less dense, it's most likely toward the sun direction...
		float nScale = 1.0 - (nFac / 0.5);

		float sunPixelDiff = origCol.a - sunPixelCol.a;
		float sunPwr = clamp(pow(sunPixelDiff, 0.0001), 0.0, 1.0);
		sunPwr *= clamp(pow(nScale, 0.25), 0.0, 1.0);
		vec3 sunColor = vec3(1.0, 0.8, 0.325);
		col.rgb = mix(col.rgb, sunColor, sunPwr);
	}
#endif //EXPERIMENTAL_CLOUD_COLOR2
	
	col = clamp(col, 0.0, 1.0);

	float alpha = clamp(pow(col.a, 1.5), 0.0, 1.0);
	alpha *= alphaMult;
	return vec4(col.rgb, alpha);
}
#endif //__CLOUDS__


#ifdef __CLOUDS2__
int junk = int(u_Time * 1000.0);
#define NO_UNROLL(X) (X + min(0, junk))

vec3 sunPos  = normalize( u_PrimaryLightOrigin.xzy - u_ViewOrigin.xzy );
vec3 sunColor = u_PrimaryLightColor.rgb;

float GetCloudHeightBelow(vec3 p)
{
	vec3 p2 = (p * 0.00015) + vec3(-u_Time * 0.0013, 0.0, -u_Time * 0.00165);

	float i = -0.6 + (textureLod(u_VolumeMap, p2, 0.50).r * 1.5);
	p2 *= 1.52;
	i += (-1.0 + 2.0 * textureLod(u_VolumeMap, p2, 0.40).g) * 0.5;
	p2 *= 2.53;
	i += (-1.0 + 2.0 * textureLod(u_VolumeMap, p2, 0.30).b) * 0.25;
	p2 *= 2.51;
	i -= (-1.0 + 2.0 * textureLod(u_VolumeMap, p2, 0.20).r) * 0.12;
	return i-0.1;
}

vec4 TraceCloudsBelow( vec3 origin, vec3 direction, vec3 skyColor, int steps)
{ 
	vec4 cloudCol=vec4(vec3(1.0, 0.53, 0.37)*1.3, 0.0);

	float density = 0.0, dist = 0.0;

	vec3 rayPos;
	float precis; 
	float td = 0.0;
	float densAdd = 0.0;
	float shadowDensity;
	float add = 1.0;
	int i = 0;
	float t = (550.0 - origin.y) / direction.y;
	
	for ( int ii=i; i<NO_UNROLL(steps); i++ )
	{
		rayPos = origin+direction*t;
		density = GetCloudHeightBelow(rayPos);
	
		if (density>0.01)
		{
			densAdd = 0.12*density*max(0.,add);
			shadowDensity = GetCloudHeightBelow(rayPos+(sunPos*90.));
	
			cloudCol.rgb += densAdd * max(0.0, (density - shadowDensity)) * sunColor;
			cloudCol.rgb -= 1.4 * shadowDensity * densAdd; 
			cloudCol.a += (1.0 - cloudCol.a) * densAdd;
			add-=densAdd;
			if (add < 0.0 || cloudCol.a > 0.99) break;
		}
		
		t += 2.0;
	}

	// mix clouds color with sky color
	cloudCol.rgb = mix(cloudCol.rgb, vec3(0.97), smoothstep(100.0, 4960.0, t));
	cloudCol.a = mix(cloudCol.a, 0.0, smoothstep(0.0, 4860.0, t));
	
	return cloudCol;
}

vec4 Clouds(float colorMult)
{
	vec3 cameraPos = vec3(0.0);
	vec3 dir = normalize(u_ViewOrigin.xzy - var_Position.xzy*1025.0);
	float alphaMult = clamp(-dir.y, 0.0, 0.75);

	if (alphaMult <= 0.0)
	{// No point in doing all this math if it's not going to draw anything...
		return vec4(0.0);
	}

	vec4 color = vec4(0.0);

	//if (rayDir.y > 0.0)
	//{
		vec4 cloudColor = TraceCloudsBelow( cameraPos, dir, vec3(0.0), 90);

		// make clouds slightly light near the sun
		float sunVisibility = pow(max(0.0, dot(sunPos, dir)), 2.0) * 0.25;
		color.rgb = mix(color.rgb, max(vec3(0.0), mix(cloudColor.rgb, cloudColor.rgb, 0.6) + sunVisibility), cloudColor.a);
		//color.rgb +=  CalculateRainbow(rayDir, rayOrigin, screenSpace);
		color.a *= alphaMult;
	//}

	return color;
}
#endif //__CLOUDS2__


#ifdef __LIGHTNING__

#define pi 3.1415926535897932384626433832795

vec2 rotate(vec2 p, float a)
{
	return vec2(p.x * cos(a) - p.y * sin(a), p.x * sin(a) + p.y * cos(a));
}

float hash1(float p)
{
	return fract(sin(p * 172.435) * 29572.683) - 0.5;
}

float hash2(vec2 p)
{
	vec2 r = (456.789 * sin(789.123 * p.xy));
	return fract(r.x * r.y * (1.0 + p.x));
}

float ns(float p)
{
	float fr = fract(p);
	float fl = floor(p);
	return mix(hash1(fl), hash1(fl + 1.0), fr);
}

float fbm(float p)
{
	return (ns(p) * 0.4 + ns(p * 2.0 - 10.0) * 0.125 + ns(p * 8.0 + 10.0) * 0.025);
}

float fbmd(float p)
{
	float h = 0.01;
	return atan(fbm(p + h) - fbm(p - h), h);
}

float arcsmp(float x, float seed)
{
	return fbm(x * 3.0 + seed * 1111.111) * (1.0 - exp(-x * 5.0));
}

float arc(vec2 p, float seed, float len)
{
	p *= len;
	float v = abs(p.y - arcsmp(p.x, seed));
	v += exp((2.0 - p.x) * -4.0);
	v = exp(v * -60.0) + exp(v * -10.0) * 0.6;
	//v += exp(p.x * -2.0);
	v *= smoothstep(0.0, 0.05, p.x);
	return v;
}

float arcc(vec2 p, float sd)
{
	float v = 0.0;
	float rnd = fract(sd);
	float sp = 0.0;
	v += arc(p, sd, 1.0);
	for(int i = 0; i < 4; i ++)
	{
		sp = rnd + 0.01;
		vec2 mrk = vec2(sp, arcsmp(sp, sd));
		v += arc(rotate(p - mrk, fbmd(sp)), mrk.x, mrk.x * 0.4 + 1.5);
		rnd = fract(sin(rnd * 195.2837) * 1720.938);
	}
	return v;
}

vec4 GetLightning( in vec3 position )
{
	float rnd2 = u_Time*hash1(float(int(u_Time*0.75)));
	vec3 ro = u_ViewOrigin.xzy;
	vec3 rd = normalize(u_ViewOrigin.xzy - position.xzy) * mix(16.0, 32.0, clamp(rnd2, 0.0, 1.0));
    
    vec3 col;
    
    vec4 rnd = vec4(0.1, 0.2, 0.3, 0.4);
    float arcv = 0.0, arclight = 0.0;
    
    {
        float v = 0.0;
        rnd = fract(sin(rnd * 1.111111) * 298729.258972);
        float ts = rnd.z * 4.0 * 1.61803398875 + 1.0;
        float arcfl = floor(u_Time / ts + rnd.y) * ts;
        float arcfr = fract(u_Time / ts + rnd.y) * ts;
        
        float arcseed = floor(u_Time * 17.0 + rnd.y);
        vec2 uv = rd.xy;
		//uv.y = 1.0 - uv.y;
		uv.x += rnd2 * 0.125;//0.02;
		//uv.y -= 1.5;
		uv.y += 1.5;
        v = arcc(uv.yx, arcseed*0.0033333);

		float arcdur = rnd.x * 0.2 + 0.05;
        float arcint = smoothstep(0.1 + arcdur, arcdur, arcfr);
        v *= arcint;
        arcv += v;

		//float arcz = ro.z + rnd.x + 6.0;//ro.z + 1.0 + rnd.x * 6.0;
        //arclight += exp(abs(arcz - position.z) * -0.3) * fract(sin(arcseed) * 198721.6231) * arcint;
    }
    
    vec3 arccol = vec3(0.9, 0.7, 0.7);
    //col += arclight * arccol * 0.5;
    col = mix(col, arccol, clamp(arcv, 0.0, 1.0));
    col = pow(col, vec3(1.0, 0.8, 0.5) * 1.5) * 1.5;
    col = pow(col, vec3(1.0 / 2.2));

	float alpha = max(col.r, max(col.g, col.b));
	return vec4(col, alpha);
}
#endif //__LIGHTNING__

vec3 reachForTheNebulas(in vec3 from, in vec3 dir, float level, float power) 
{
	vec3 color = vec3(0.0);
	float nebula = pow(pNoise(dir+vec3(PROCEDURAL_SKY_NEBULA_SEED)), 12.0 / (1.0 - clamp(PROCEDURAL_SKY_NEBULA_FACTOR, 0.0, 0.999)));

	if (nebula > 0.0)
	{
		vec3 randc = vec3(nebula*10.0);//vec3(pNoise(dir.xyz*10.0*level));
		color = nebula * randc;
	}

	return pow(color*2.25, vec3(power));
}

vec3 reachForTheStars(in vec3 from, in vec3 dir, float power) 
{
	float star = pow(pNoise(dir*320.0), 48.0 - clamp(PROCEDURAL_SKY_STAR_DENSITY, 0.0, 16.0));
	return pow(vec3(star*2.25), vec3(power));
}

void GetStars(out vec4 fragColor, in vec3 position)
{
	vec3 color = vec3(0.0);
	vec3 from = vec3(0.0);
	vec3 dir = normalize(position);
	vec3 origdir = normalize(position);

	// Adjust for planetary rotation...
	float dnt = DAY_NIGHT_24H_TIME * 2.0 - 1.0;
	if (dnt <= 0.0) dnt = 1.0 + (1.0 - length(dnt));
	dir.xy += dnt * PROCEDURAL_SKY_PLANETARY_ROTATION;

#if 0
	// Nebulae...
	vec3 color1=clamp(reachForTheNebulas(from, -dir.xyz, 64.0, 0.5), 0.0, 1.0) * vec3(0.0, 0.0, 1.0);
	//vec3 color2=clamp(reachForTheNebulas(from, -dir.zxy, 64.0, 0.7), 0.0, 1.0) * vec3(0.0, 1.0, 0.0);
	vec3 color3=clamp(reachForTheNebulas(from, -dir.yxz, 64.0, 0.7), 0.0, 1.0) * vec3(1.0, 0.0, 0.0);
#endif

	// Small stars...
	float twinkle = clamp(cos(length(dir.xz*1000.0+u_Time)*u_Time*0.001) * 2.5, 0.75, 3.0);
	vec3 colorStars = clamp(reachForTheStars(from, dir, 0.9), 0.0, 1.0) * twinkle;

#if 0
	// Add them all together...
	color = color1 /*+ color2*/ + color3 + colorStars;
#else
	color = colorStars;
#endif

	color = clamp(color, 0.0, 1.0);
	color = pow(color, vec3(1.2));

	fragColor = vec4(color, 1.0);
}

void GetSun(out vec4 fragColor, in vec3 position)
{
	vec3 from = vec3(0.0);
	vec3 dir = normalize(position);

	vec3 sunPos = normalize(u_PrimaryLightOrigin.xyz);

	float sunSize = 1.0 - (0.0015 * 1.0);
	float sun = dot(dir, sunPos);

	if (sun > sunSize) {
		vec3 lightColor = u_PrimaryLightColor.rgb;

		if (SHADER_NIGHT_SCALE > 0.0)
		{
			lightColor = mix(lightColor, vec3(1.0, 0.8, 0.625), SHADER_NIGHT_SCALE);
		}

		fragColor = vec4(lightColor.rgb, 1.0);
		return;
	}

	fragColor = vec4(0.0);
}

// 2D rotation function
mat2 rot2D(float a) {
	return mat2(cos(a),sin(a),-sin(a),cos(a));	
}

void GetPlanets(out vec4 fragColor, in vec3 position)
{
	vec3 from = vec3(0.0);
	vec3 dir = normalize(position);

	for (int i = 0; i < u_MoonCount; i++)
	{
		//if (u_MoonInfos[i].r <= 0.0) continue;

		vec3 sunPos = normalize(u_PrimaryLightOrigin.xyz);
		vec3 planetPos = -normalize(u_PrimaryLightOrigin.xyz);

		float planetSize = 1.0 - (0.0015 * u_MoonInfos[i].a);
		float planetTexScale = 6.0 * u_MoonInfos2[i].g;
		float planetTexBright = 2.0 * u_MoonInfos2[i].r;
		float planetTexBright2 = 64.0;

		// Adjust for planetary rotation...
		mat2 planetRot1 = rot2D(u_MoonInfos[i].b);
		mat2 planetRot2 = rot2D(u_MoonInfos[i].g);
		planetPos.yz *= planetRot1;
		planetPos.xy *= planetRot2;
		planetPos = normalize(planetPos);

		float planet = dot(dir, planetPos);

		if (planet > planetSize) {
			float ldot = clamp(dot(dir, sunPos), 0.0, 1.0);
			float lglow = clamp(pow(max(0.0, dot(dir - planetPos, sunPos)), 3.0) * 768.0, 0.015, 1.0);

			//vec3 planetshade = texture(u_MoonMaps[i], dir.xy * planetTexScale).rgb * planetTexBright;
			vec3 planetshade = texture(u_MoonMaps[i], (dir.xy - planetPos.xy) * planetTexScale).rgb * planetTexBright;
			vec3 color = planetshade * 0.7 * lglow;
			color += max(0.0, 0.007 - abs(planet - planetSize)) * lglow * planetTexBright2;
			fragColor = vec4(clamp(color, 0.0, 1.0), 0.8/*0.825*/);
			return; // Since this planet drew a pixel, don't check any more planets, this is the closest one (first in the list)...
		}
	}

	fragColor = vec4(0.0);
}

#ifdef __BACKGROUND_HILLS__
#define EPSILON 0.1

#define bghtime (u_Time+285.)

const mat3 m3 = mat3( 0.00,  0.80,  0.60,
                     -0.80,  0.36, -0.48,
                     -0.60, -0.48,  0.64 );

float hillsFbm( in vec3 p ) {
	float f = 0.0;
	f += 0.5000*tNoise( p ); p = m3*p*1.22*PROCEDURAL_BACKGROUND_HILLS_SEED;
	f += 0.2500*tNoise( p ); p = m3*p*1.53*PROCEDURAL_BACKGROUND_HILLS_SEED;
	f += 0.1250*tNoise( p ); p = m3*p*4.01*PROCEDURAL_BACKGROUND_HILLS_SEED;
	f += 0.0625*tNoise( p );
	return f/0.9375;
}

// intersection functions

bool intersectPlane(const in vec3 ro, const in vec3 rd, const in float height, inout float dist) {	
	if (rd.y==0.0) {
		return false;
	}
		
	float d = -(ro.y - height)/rd.y;
	d = min(100000.0, d);
	if( d > 0. && d < dist ) {
		dist = d;
		return true;
    } else {
		return false;
	}
}

// light direction

vec3 lig = normalize(u_PrimaryLightOrigin.xzy);

// terrain functions
float terrainMap( const in vec3 p ) 
{
    float dist = pow(length(p) / 64.0, 0.2);
    return (((hillsFbm( (/*p.xzz*/p.xyz*0.5+16.0)*0.00346 ) * 1.5 - PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS)*250.0*dist)+(dist*8.0)) - PROCEDURAL_BACKGROUND_HILLS_UPDOWN;
}

vec3 splatblend(vec3 color1, float a1, vec3 color2, float a2)
{
	float depth = 0.2;
	float ma = max(a1, a2) - depth;
	float b1 = max(a1 - ma, 0);
	float b2 = max(a2 - ma, 0);
	return ((color1.rgb * b1) + (color2.rgb * b2)) / (b1 + b2);
}

vec4 raymarchTerrain( const in vec3 ro, const in vec3 rd, const in vec3 bgc, const in float startdist, in float dist ) {
	float t = startdist;

    // raymarch	
	vec4 sum = vec4( 0.0 );
	vec3 col = vec3(0.0);//bgc;
	float alpha = 0.0;
	bool hit = false;
	
	for( int i=0; i<8; i++ ) {
		if( hit ) break;
		
		t += float(i*64);// + t/100.;
		vec3 pos = ro + t*rd;
		
		if( pos.y < terrainMap(pos) ) {
			hit = true;
		}		
	}
	if( hit ) 
	{
		// binary search for hit		
		float dt = 4.+t/400.;
		t -= dt;
		
		vec3 pos = ro + t*rd;	
		t += (0.5 - step( pos.y , terrainMap(pos) )) * dt;		
		for( int j=0; j<2; j++ ) {
			pos = ro + t*rd;
			dt *= 0.5;
			t += (0.5 - step( pos.y , terrainMap(pos) )) * dt;
		}
		pos = ro + t*rd;
		
#if 1
		vec3 dx = vec3( 100.*EPSILON, 0., 0. );
		vec3 dz = vec3( 0., 0., 100.*EPSILON );
		
		
		vec3 normal = vec3( 0., 0., 0. );
		normal.x = (terrainMap(pos + dx) - terrainMap(pos-dx) ) / (200. * EPSILON);
		normal.z = (terrainMap(pos + dz) - terrainMap(pos-dz) ) / (200. * EPSILON);
		normal.y = 1.;
		normal = normalize( normal );		

		//col = vec3(0.2) + 0.7*texture( iChannel2, pos.xz * 0.01 ).xyz * vec3(1.,.9,0.6);
		
		float veg = 0.3*hillsFbm(pos*0.2)+normal.y;
					
		
		if( veg > 0.75 ) {
			col = vec3( PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR )*(0.5+0.5*hillsFbm(pos*0.5))*0.6;
		} else 
		if( veg > 0.66 ) {
			col = col*0.6+vec3( PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2 )*(0.5+0.5*hillsFbm(pos*0.25))*0.3;
		}
		col *= vec3(0.5, 0.52, 0.65)*vec3(1.,.9,0.8);

		vec3 brdf = col;
		
		float diff = clamp( dot( normal, -lig ), 0., 1.);
		
		col = brdf*diff*vec3(1.0,.6,0.1);
		col += brdf*clamp( dot( normal, lig ), 0., 1.)*vec3(0.8,.6,0.5)*0.8;
		col += brdf*clamp( dot( normal, vec3(0.,1.,0.) ), 0., 1.)*vec3(0.8,.8,1.)*0.2;
#else
		vec3 normal = vec3( 0., 0., 0. );
		
		if (u_Local9.b > 1.0)
		{
			float eps = u_Local9.r;
			normal.x = terrainMap(vec3(pos.x+eps, pos.y, pos.z));
			normal.y = terrainMap(vec3(pos.x, pos.y, pos.z));
			normal.z = terrainMap(vec3(pos.x, pos.y, pos.z+eps));
			normal = normalize(normal);
		}
		else if (u_Local9.b > 0.0)
		{
			vec3 dx = vec3( u_Local9.r, 0., 0. );
			vec3 dz = vec3( 0., 0., u_Local9.r );

			normal.x = (terrainMap(pos + dx) - terrainMap(pos-dx)) * u_Local9.g;
			normal.z = (terrainMap(pos + dz) - terrainMap(pos-dz)) * u_Local9.g;
			normal.y = 1.;
			normal = normalize( normal );
		}
		else
		{
			vec3 dx = vec3( u_Local9.r, 0., 0. );
			vec3 dz = vec3( 0., 0., u_Local9.r );

			normal.x = terrainMap(pos + dx) * u_Local9.g;
			normal.z = terrainMap(pos + dz) * u_Local9.g;
			normal.y = 1.;
			normal = normalize(normal);
		}

		if (u_Local9.a > 1.0)
		{
			return vec4(normal.xzy * 0.5 + 0.5, 1.0);
		}
		else if (u_Local9.a > 0.0)
		{
			return vec4(normal.xyz * 0.5 + 0.5, 1.0);
		}

		float snow = clamp(dot(normalize(normal.xyz), vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
		snow = clamp(pow(snow, 0.4), 0.0, 1.0);

		col = PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR;

		if (snow > 0.0)
		{
			col = splatblend(col, 1.0 - snow, PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2, snow);
		}

		vec3 lightColor = u_PrimaryLightColor.rgb;

		if (SHADER_NIGHT_SCALE > 0.0)
		{
			lightColor = mix(lightColor, vec3(1.0, 0.8, 0.625), SHADER_NIGHT_SCALE);
		}

		float diff = clamp( dot( normal, -lig ), 0., 1.);
		col += diff*lightColor;
#endif

		
		dist = t;
		t -= pos.y*3.5;
		alpha = clamp(exp(-0.0000005*t*t), 0.0, 0.3);
		col = mix( bgc, col, alpha );

		return vec4(col, 1.0);
	}

	//return vec4(col, alpha >= 0.1 ? 1.0 : 0.0);
	return vec4(0.0);
}

#ifdef __AURORA2__
//AURORA STUFF
mat2 mm2(in float a){
	float c = cos(a);
	float s = sin(a);
	return mat2(c,s,-s,c);
}

mat2 m2 = mat2(0.95534, 0.29552, -0.29552, 0.95534);

float tri(in float x){
	return clamp(abs(fract(x)-.5),0.01,0.49);
}

vec2 tri2(in vec2 p){
	return vec2(tri(p.x)+tri(p.y),tri(p.y+tri(p.x)));
}

float triNoise2d(in vec2 p, float spd)
{
	float z=1.8;
	float z2=2.5;
	float rz = 0.;
	p *= mm2(p.x*0.06);
	vec2 bp = p;
	for (float i=0.; i<5.; i++ )
	{
		vec2 dg = tri2(bp*1.85)*.75;
		dg *= mm2(u_Time*4.0*spd);
		p -= dg/z2;

		bp *= 1.3;
		z2 *= 1.45;
		z *= .42;
		p *= 1.21 + (rz-1.0)*.02;
		
		rz += tri(p.x+tri(p.y))*z;
		p*= -m2;
	}
	return clamp(1./pow(rz*29., 1.3),0.,.55);
}

float hash21(in vec2 n){ return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453); }

vec4 aurora(vec3 ro, vec3 rd)
{
	vec4 col = vec4(0);
	vec4 avgCol = vec4(0);
	ro *= 1e-5;
	float mt = 10.;
	for(float i=0.;i<5.;i++)
	{
		float of = 0.006*hash21(gl_FragCoord.xy)*smoothstep(0.,15., i*mt);
		float pt = ((.8+pow((i*mt),1.2)*.001)-rd.y)/(rd.y*2.+0.4);
		pt -= of;
		vec3 bpos = (ro) + pt*rd;
		vec2 p = bpos.zx;
		float rzt = triNoise2d(p, 0.1);
		vec4 col2 = vec4(0,0,0, rzt);
		col2.rgb = (sin(1.-vec3(2.15,-.5, 1.2)+(i*mt)*0.053)*(0.5*mt))*rzt;
		avgCol =  mix(avgCol, col2, .5);
		col += avgCol*exp2((-i*mt)*0.04 - 2.5)*smoothstep(0.,5., i*mt);
	}

	col *= (clamp(rd.y*15.+.4,0.,1.2));
	return clamp(col*2.8, 0.0, 1.0);
}
//END AURORA STUFF

vec4 GetAurora2(in vec2 fragCoord)
{
	vec3 from = vec3(0.0);
	vec3 dir = normalize(var_Position.xzy);

	float auroraPower = 0.0;

	if (SHADER_AURORA_ENABLED >= 2.0)
	{// Day enabled aurora - always full strength...
		auroraPower = 1.0;
	}
	else
	{
		auroraPower = SHADER_NIGHT_SCALE;
	}

	vec4 aur = aurora(from, dir);
	aur.a *= auroraPower;

	return aur;
}
#endif //__AURORA2__

#define TAU 6.2831853071

vec4 GetAurora(in vec2 fragCoord)
{
	float autime = u_Time * 0.5;

	if (SHADER_SKY_DIRECTION == 2.0 || SHADER_SKY_DIRECTION == 3.0)
	{// Forward or back sky textures, invert the X axis to make the aura seamless...
		fragCoord.x = 1.0 - fragCoord.x;
	}

	float auroraPower;
	if (SHADER_AURORA_ENABLED >= 2.0)
	{// Day enabled aurora - always full strength...
		auroraPower = 1.0;
	}
	else
	{
		auroraPower = SHADER_NIGHT_SCALE;
	}

	vec2 uv = fragCoord.xy;
	// Move aurora up a bit above horizon...
	//uv *= 1.15;//0.8;
	//uv += 0.1;//0.2;
	uv.y *= 1.15;//0.8;
	uv.y += 0.1;//0.2;
	uv = clamp(uv, 0.0, 1.0);

	float o = texture(u_SplatMap1, uv * 0.25 + vec2(0.0, autime * 0.025)).r;
	float d = (texture(u_SplatMap2, uv * 0.25 - vec2(0.0, autime * 0.02 + o * 0.02)).r * 2.0 - 1.0);
	float v = uv.y + d * 0.1;
	v = 1.0 - abs(v * 2.0 - 1.0);
	v = pow(v, 2.0 + sin((autime * 0.2 + d * 0.25) * TAU) * 0.5);
	v = clamp(v, 0.0, 1.0);
	vec3 color = vec3(0.0);
	float x = (1.0 - uv.x * 0.75);
	float y = 1.0 - abs(uv.y * 2.0 - 1.0);
	x = clamp(x, 0.0, 1.0);
	y = clamp(y, 0.0, 1.0);
	color += vec3(x * 0.5, y, x) * v;
	vec2 seed = fragCoord.xy;
	vec2 r;
	r.x = fract(sin((seed.x * 12.9898) + (seed.y * 78.2330)) * 43758.5453);
	r.y = fract(sin((seed.x * 53.7842) + (seed.y * 47.5134)) * 43758.5453);
	float s = mix(r.x, (sin((autime * 2.5 + 60.0) * r.y) * 0.5 + 0.5) * ((r.y * r.y) * (r.y * r.y)), 0.04);
	color += clamp(pow(s, 70.0) * (1.0 - v), 0.0, 1.0);
	float str = max(color.r, max(color.g, color.b));
	color *= 0.7;
	color *= AURORA_COLOR;

	return vec4(color.rgb, auroraPower*str);
}

void GetBackgroundHills( inout vec4 fragColor, in vec2 fragCoord, vec3 ro, vec3 rd ) {
	fragColor = raymarchTerrain( ro, rd, fragColor.rgb, 1200.0, 1200.0 );
}
#endif //__BACKGROUND_HILLS__

#ifdef __TEST_SKY__

#define ORIG_CLOUD 0
#define ENABLE_RAIN 0 //enable rain drops on screen
#define SIMPLE_SUN 0
#define NICE_HACK_SUN 0
#define SOFT_SUN 1
#define cloudy  0.5 //0.0 clear sky
#define haze  0.01 * (cloudy*20.)
#define rainmulti 5.0 // makes clouds thicker
#define rainy (10.0 -rainmulti)
#define t u_Time
#define fov tan(radians(60.0))
#define S(x, y, z) smoothstep(x, y, z)
#define cameraheight 5e1 //50.
#define mincloudheight 5e3 //5e3
#define maxcloudheight 8e3 //8e3
#define xaxiscloud t*5e2 //t*5e2 +t left -t right *speed
#define yaxiscloud 0. //0.
#define zaxiscloud t*6e2 //t*6e2 +t away from horizon -t towards horizon *speed
#define cloudnoise 2e-4 //2e-4

//#define cloud2


//Performance
const int steps = 16; //16 is fast, 128 or 256 is extreme high
const int stepss = 16; //16 is fast, 16 or 32 is high 

//Environment
const float R0 = 6360e3; //planet radius //6360e3 actual 6371km
const float Ra = 6380e3; //atmosphere radius //6380e3 troposphere 8 to 14.5km
const float I = 10.; //sun light power, 10.0 is normal
const float SI = 5.; //sun intensity for sun
const float g = 0.45; //light concentration .76 //.45 //.6  .45 is normaL
const float g2 = g * g;

const float ts= (cameraheight / 2.5e5);

const float s = 0.999; //light concentration for sun
#if SOFT_SUN
const float s2 = s;
#else
const float s2 = s * s;
#endif
const float Hr = 8e3; //Rayleigh scattering top //8e3
const float Hm = 1.2e3; //Mie scattering top //1.3e3

vec3 bM = vec3(21e-6); //normal mie // vec3(21e-6)
//vec3 bM = vec3(50e-6); //high mie

//Rayleigh scattering (sky color, atmospheric up to 8km)
vec3 bR = vec3(5.8e-6, 13.5e-6, 33.1e-6); //normal earth
//vec3 bR = vec3(5.8e-6, 33.1e-6, 13.5e-6); //purple
//vec3 bR = vec3( 63.5e-6, 13.1e-6, 50.8e-6 ); //green
//vec3 bR = vec3( 13.5e-6, 23.1e-6, 115.8e-6 ); //yellow
//vec3 bR = vec3( 5.5e-6, 15.1e-6, 355.8e-6 ); //yeellow
//vec3 bR = vec3(3.5e-6, 333.1e-6, 235.8e-6 ); //red-purple

vec3 C = vec3(0., -R0, 0.); //planet center
vec3 Ds = normalize(u_PrimaryLightOrigin.xzy);//normalize(vec3(0., 0., -1.)); //sun direction?

float cloudyhigh = 0.05; //if cloud2 defined

#if ORIG_CLOUD
float cloudnear = 1.0; //9e3 12e3  //do not render too close clouds on the zenith
float cloudfar = 1e3; //15e3 17e3
#else
float cloudnear = 1.0; //15e3 17e3
float cloudfar = 70e3; //160e3  //do not render too close clouds on the horizon 160km should be max for cumulus
#endif




//AURORA STUFF
mat2 mm2(in float a){
    float c = cos(a);
    float s = sin(a);
    return mat2(c,s,-s,c);
}

mat2 m2 = mat2(0.95534, 0.29552, -0.29552, 0.95534);

float tri(in float x){
    return clamp(abs(fract(x)-.5),0.01,0.49);
}

vec2 tri2(in vec2 p){
    return vec2(tri(p.x)+tri(p.y),tri(p.y+tri(p.x)));
}

float triNoise2d(in vec2 p, float spd)
{
    float z=1.8;
    float z2=2.5;
	float rz = 0.;
    p *= mm2(p.x*0.06);
    vec2 bp = p;
	for (float i=0.; i<5.; i++ )
	{
        vec2 dg = tri2(bp*1.85)*.75;
        dg *= mm2(t*spd);
        p -= dg/z2;

        bp *= 1.3;
        z2 *= 1.45;
        z *= .42;
		p *= 1.21 + (rz-1.0)*.02;
        
        rz += tri(p.x+tri(p.y))*z;
        p*= -m2;
	}
    return clamp(1./pow(rz*29., 1.3),0.,.55);
}


float hash21(in vec2 n){ return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453); }
vec4 aurora(vec3 ro, vec3 rd)
{
    vec4 col = vec4(0);
    vec4 avgCol = vec4(0);
    ro *= 1e-5;
    float mt = 10.;
    for(float i=0.;i<5.;i++)
    {
        float of = 0.006*hash21(gl_FragCoord.xy)*smoothstep(0.,15., i*mt);
        float pt = ((.8+pow((i*mt),1.2)*.001)-rd.y)/(rd.y*2.+0.4);
        pt -= of;
    	vec3 bpos = (ro) + pt*rd;
        vec2 p = bpos.zx;
        //vec2 p = rd.zx;
        float rzt = triNoise2d(p, 0.1);
        vec4 col2 = vec4(0,0,0, rzt);
        col2.rgb = (sin(1.-vec3(2.15,-.5, 1.2)+(i*mt)*0.053)*(0.5*mt))*rzt;
        avgCol =  mix(avgCol, col2, .5);
        col += avgCol*exp2((-i*mt)*0.04 - 2.5)*smoothstep(0.,5., i*mt);

    }

    col *= (clamp(rd.y*15.+.4,0.,1.2));
    return col*2.8;
}

//END AURORA STUFF

float noise(in vec2 v) { 
    //return textureLod(u_RoadMap, (v+.5)/256., 0.).r;
	return textureLod(u_RoadMap, (v+.5)/2048.0, 0.).r; 
}

// by iq
float Noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);

	
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	//vec2 rg = texture( u_RoadMap, (uv+ 0.5)/256.0, -100.0).yx;
	vec2 rg = texture( u_RoadMap, (uv+ 0.5)/2048.0, -100.0).yx;
	

	return mix( rg.x, rg.y, f.z );
}


float fnoise( vec3 p, in float t )
{
	/*if (u_Local9.r <= 0.0)
	{
		p *= 0.25;
		float f;

		f = 0.5000 * Noise(p); p = p * 3.02; p.y -= t*.1; //t*.05 speed cloud changes
		f += 0.2500 * Noise(p); p = p * 3.03; p.y += t*.06;
		f += 0.1250 * Noise(p); p = p * 3.01;
		f += 0.0625   * Noise(p); p =  p * 3.03;
		f += 0.03125  * Noise(p); p =  p * 3.02;
		f += 0.015625 * Noise(p);

		return f;
	}*/

	return texture(u_VolumeMap, (p * 0.1) /*- t*0.1*/).r;
}

float cloud(vec3 p, in float t ) {
	float cld = fnoise(p*cloudnoise,t) + cloudy*0.1 ;
	cld = smoothstep(.4+.04, .6+.04, cld);
	cld *= cld * (5.0*rainmulti);
	return cld+haze;
}

void densities(in vec3 pos, out float rayleigh, out float mie) {
	float h = length(pos - C) - R0;
	rayleigh =  exp(-h/Hr);
	vec3 d = pos;
    d.y = 0.0;
    float dist = length(d);
    
	float cld = 0.;
	if (mincloudheight < h && h < maxcloudheight) {
		//cld = cloud(pos+vec3(t*1e3,0., t*1e3),t)*cloudy;
        cld = cloud(pos+vec3(xaxiscloud,yaxiscloud, zaxiscloud),t)*cloudy; //direction and speed the cloud movers
		cld *= sin(3.1415*(h-mincloudheight)/mincloudheight) * cloudy;
	}
	#ifdef cloud2
        float cld2 = 0.;
        if (12e3 < h && h < 15.5e3) {
            cld2 = fnoise(pos*3e-4,t)*cloud(pos*32.0+vec3(27612.3, 0.,-t*15e3), t);
            cld2 *= sin(3.1413*(h-12e3)/12e3) * cloudyhigh;
            cld2 = clamp(cld2,0.0,1.0);
        }
    
    #endif

    #if ORIG_CLOUD
    if (dist<cloudfar) {
        float factor = clamp(1.0-((cloudfar - dist)/(cloudfar-cloudnear)),0.0,1.0);
        cld *= factor;
    }
    #else

    if (dist>cloudfar) {

        float factor = clamp(1.0-((dist - cloudfar)/(cloudfar-cloudnear)),0.0,1.0);
        cld *= factor;
    }
    #endif

	mie = exp(-h/Hm) + cld + haze;
	#ifdef cloud2
		mie += cld2;
	#endif
    
}



float escape(in vec3 p, in vec3 d, in float R) {
	vec3 v = p - C;
	float b = dot(v, d);
	float c = dot(v, v) - R*R;
	float det2 = b * b - c;
	if (det2 < 0.) return -1.;
	float det = sqrt(det2);
	float t1 = -b - det, t2 = -b + det;
	return (t1 >= 0.) ? t1 : t2;
}

// this can be explained: http://www.scratchapixel.com/lessons/3d-advanced-lessons/simulating-the-colors-of-the-sky/atmospheric-scattering/
void scatter(vec3 o, vec3 d, out vec3 col, out vec3 scat, in float t) {
    
	float L = escape(o, d, Ra);	
	float mu = dot(d, Ds);
	float opmu2 = 1. + mu*mu;
	float phaseR = .0596831 * opmu2;
	float phaseM = .1193662 * (1. - g2) * opmu2 / ((2. + g2) * pow(1. + g2 - 2.*g*mu, 1.5));
    float phaseS = .1193662 * (1. - s2) * opmu2 / ((2. + s2) * pow(1. + s2 - 2.*s*mu, 1.5));
	
	float depthR = 0., depthM = 0.;
	vec3 R = vec3(0.), M = vec3(0.);
	
	float dl = L / float(steps);
	for (int i = 0; i < steps; ++i) {
		float l = float(i) * dl;
		vec3 p = (o + d * l);

		float dR, dM;
		densities(p, dR, dM);
		dR *= dl; dM *= dl;
		depthR += dR;
		depthM += dM;

		float Ls = escape(p, Ds, Ra);
		if (Ls > 0.) {
			float dls = Ls / float(stepss);
			float depthRs = 0., depthMs = 0.;
			for (int j = 0; j < stepss; ++j) {
				float ls = float(j) * dls;
				vec3 ps = ( p + Ds * ls );
				float dRs, dMs;
				densities(ps, dRs, dMs);
				depthRs += dRs * dls;
				depthMs += dMs * dls;
			}

			vec3 A = exp(-(bR * (depthRs + depthR) + bM * (depthMs + depthM)));
			R += (A * dR);
			M += A * dM ;
		} else {
		}
	}

	//col = (I) * (R * bR * phaseR + M * bM * (phaseM ));
    col = (I) *(M * bM * (phaseM )); // Mie scattering
    #if NICE_HACK_SUN
    col += (SI) *(M * bM *phaseS); //Sun
    #endif
    col += (I) *(R * bR * phaseR); //Rayleigh scattering
    scat = 0.1 *(bM*depthM);
    //scat = 0.0 + clamp(depthM*5e-7,0.,1.); 
}


vec3 hash33(vec3 p)
{
	p = fract(p * vec3(443.8975,397.2973, 491.1871));
	p += dot(p.zxy, p.yxz+19.27);
	return fract(vec3(p.x * p.y, p.z*p.x, p.y*p.z));
}

vec3 stars(in vec3 p)
{
	vec3 c = vec3(0.);
	//float res = iResolution.x*2.5;
	float res = 4096.0*2.5;

	for (float i=0.;i<4.;i++)
	{
		vec3 q = fract(p*(.15*res))-0.5;
		vec3 id = floor(p*(.15*res));
		vec2 rn = hash33(id).xy;
		float c2 = 1.-smoothstep(0.,.6,length(q));
		c2 *= step(rn.x,.0005+i*i*0.001);
		c += c2*(mix(vec3(1.0,0.49,0.1),vec3(0.75,0.9,1.),rn.y)*0.1+0.9);
		p *= 1.3;
	}
	return c*c*.8;
}

//RAIN STUFF
vec3 N31(float p) {
	//  3 out, 1 in... DAVE HOSKINS
   vec3 p3 = fract(vec3(p) * vec3(.1031,.11369,.13787));
   p3 += dot(p3, p3.yzx + 19.19);
   return fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

float SawTooth(float t) {
	return cos(t+cos(t))+sin(2.*t)*.2+sin(4.*t)*.02;
}

float DeltaSawTooth(float t) {
	return 0.4*cos(2.*t)+0.08*cos(4.*t) - (1.-sin(t))*sin(t+cos(t));
}  

vec2 GetDrops(vec2 uv, float seed, float m) {
	
	float t2 = t+m;
	vec2 o = vec2(0.);

	#ifndef DROP_DEBUG
	uv.y += t2*.05;
	#endif
	
	uv *= vec2(10., 2.5)*2.;
	vec2 id = floor(uv);
	vec3 n = N31(id.x + (id.y+seed)*546.3524);
	vec2 bd = fract(uv);
	
	vec2 uv2 = bd;
	
	bd -= 0.5;
	
	bd.y*=4.;

	bd.x += (n.x-.5)*rainy;
	
	t2 += n.z * 6.28;
	float slide = SawTooth(t2);
	
	float ts = 1.5;
	vec2 trailPos = vec2(bd.x*ts, (fract(bd.y*ts*2.-t2*2.)-.5)*.5);
	
	bd.y += slide*2.;								// make drops slide down
	
	#ifdef HIGH_QUALITY
	float dropShape = bd.x*bd.x;
	dropShape *= DeltaSawTooth(t);
	bd.y += dropShape;								// change shape of drop when it is falling
	#endif
	
	float d = length(bd);							// distance to main drop
	
	float trailMask = S(-.2, .2, bd.y);				// mask out drops that are below the main
	trailMask *= bd.y;								// fade dropsize
	float td = length(trailPos*max(.5, trailMask));	// distance to trail drops
	
	float mainDrop = S(.2, .1, d);
	float dropTrail = S(.1, .02, td);
	
	dropTrail *= trailMask;
	o = mix(bd*mainDrop, trailPos, dropTrail);		// mix main drop and drop trail
	
	#ifdef DROP_DEBUG
	if(uv2.x<.02 || uv2.y<.01) o = vec2(1.);
	#endif
	
	return o;
}
//END RAIN STUFF

vec4 GetTestSky( in vec2 fragCoord ) 
{
	vec4 fragColor = vec4(0.0);

	vec3 O = /*vec3(0.0);*/vec3(0., cameraheight, 0.);
	vec3 D = normalize(var_Position.xzy);

	vec3 color = vec3(0.);
	vec3 scat = vec3(0.);

	//float scat = 0.;
	float att = 1.;
	float staratt = 1.;
	float scatatt = 1.;
	vec3 star = vec3(0.);
	vec4 aur = vec4(0.);

	float fade = smoothstep(0.,0.01,abs(D.y))*0.5+0.9;

	//float dnt = DAY_NIGHT_24H_TIME;
	vec2 uvMouse = Ds.xy;
	//vec2 uvMouse = vec2(0.0, dnt);
	
	staratt = 1. - min(1.0, (uvMouse.y*2.0));
	scatatt = 1. - min(1.0, (uvMouse.y*2.2));

	//float dnt = DAY_NIGHT_24H_TIME * 2.0 - 1.0;
	//staratt = dnt;
	//scatatt = dnt * 2.2;

	if (D.y < -ts) {
		float L = - O.y / D.y;
		O = O + D * L;
		D.y = -D.y;
		D = normalize(D+vec3(0,.003*sin(t+6.2831*noise(O.xz+vec2(0.,-t*1e3))),0.));
		att = .6;
		star = stars(D);
		
		if (uvMouse.y < 0.5)
			aur = smoothstep(0.0,2.5,aurora(O,D));
	}
	else {
		float L1 =  O.y / D.y;
		vec3 O1 = O + D * L1;

		vec3 D1 = vec3(1.);
		D1 = normalize(D+vec3(1.,0.0009*sin(t+6.2831*noise(O1.xz+vec2(0.,t*0.8))),0.));
		star = stars(D1);
		
		if (uvMouse.y < 0.5)
			aur = smoothstep(0.,1.5,aurora(O,D))*fade;
	}

	star *= att;
	star *= staratt;

	scatter(O, D, color, scat, t);
	color *= att;
	scat *=  att;
	scat *= scatatt;

	
	color += scat;
	color += star;
	//color=color*(1.-(aur.a)*scatatt) + (aur.rgb*scatatt);
	color += aur.rgb*scatatt;

	
	
	#if ENABLE_RAIN
	vec2 drops = vec2(0.);
	if (rainmulti > 1.0){
		drops = GetDrops(uv/2.0, 1., 1.);
		color += drops.x+drops.y;
	}
	#endif

	//float env = pow( smoothstep(.5, iResolution.x / iResolution.y, length(uv*0.8)), 0.0);
	fragColor = vec4(pow(color, vec3(1.0/2.2)), 1.); //gamma correct

	return fragColor;
}
#endif //__TEST_SKY__

#ifdef __RAINBOWS__
#define colorStep 0.004
#define gradStep 0.0022

// create rain bow opposite direction to the sun
vec4 Rainbow(vec3 rayDir, vec3 sunPos)
{
	if (SHADER_NIGHT_SCALE >= 1.0 || DYNAMIC_WEATHER_PUDDLE_STRENGTH <= 0.0)
	{
		return vec4(0.0);
	}

	float nightFade = clamp(pow(1.0 - SHADER_NIGHT_SCALE, 0.5), 0.0, 1.0);
	float wetnessFade = clamp(pow(DYNAMIC_WEATHER_PUDDLE_STRENGTH, 0.25), 0.0, 1.0);
	float combinedFade = min(nightFade, wetnessFade);

	//float visibility = pow(max(0.0, dot(vec3(sunPos.x * -1.0, 0.0, sunPos.z * -1.0), rayDir)), 20.0);
	float visibility = pow(max(0.0, dot(vec3(sunPos.x, 0.0, sunPos.z), rayDir)), 15.0);

	if (visibility <= 0.0 || combinedFade <= 0.0)
	{
		return vec4(0.0);
	}

	// rainbow colors based on center distance
	float colorPos = 0.05;
	vec4 color = mix(vec4(0.0), vec4(1.0, 0, 0, 1.0), smoothstep(colorPos, colorPos + gradStep, visibility));
	colorPos += colorStep;
	color = mix(color, vec4(1.0, 0.5, 0.0, 1.0), smoothstep(colorPos, colorPos + gradStep, visibility));
	colorPos += colorStep;
	color = mix(color, vec4(1.0, 1.0, 0.0, 1.0), smoothstep(colorPos, colorPos + gradStep, visibility));
	colorPos += colorStep;
	color = mix(color, vec4(0.0, 1.0, 0.0, 1.0), smoothstep(colorPos, colorPos + gradStep, visibility));
	colorPos += colorStep;
	color = mix(color, vec4(0.0, 0.2, 1.0, 1.0), smoothstep(colorPos, colorPos + gradStep, visibility));
	colorPos += colorStep;
	color = mix(color, vec4(0.0, 0.0, 0.9, 1.0), smoothstep(colorPos, colorPos + gradStep, visibility));
	colorPos += colorStep;
	color = mix(color, vec4(0.3, 0.0, 1.0, 1.0), smoothstep(colorPos, colorPos + gradStep, visibility));
	colorPos += colorStep;
	color = mix(color, vec4(0.0), smoothstep(colorPos, colorPos + gradStep, visibility));

	// tone rainbow colors to transparent the closer to rayDir = 0.0 we get
	return color*visibility*3.5/*4.5*/*mix(0.0, 1.0, smoothstep(0.3, 0.0, length(0.3 - rayDir.y)))*combinedFade;
}
#endif //__RAINBOWS__


void main()
{
	vec4 terrainColor = vec4(0.0);
	vec3 nightGlow = vec3(0.0);
	vec3 sunColorMod = vec3(1.0);
	vec4 sun = vec4(0.0);
	vec4 clouds = vec4(0.0);
	vec4 pCol = vec4(0.0);
	vec3 position = var_Position.xzy;
	vec3 lightPosition = u_PrimaryLightOrigin.xzy;
	vec3 skyViewDir = normalize(position);
	vec3 skyViewDir2 = normalize(u_ViewOrigin.xzy - position);
	vec3 skySunDir = normalize(lightPosition);
#if defined(__CLOUDS__) || defined(__CLOUDS2__)
	float cloudiness = clamp(CLOUDS_CLOUDCOVER*0.3, 0.0, 0.3);
#endif //defined(__CLOUDS__) || defined(__CLOUDS2__)

	if (USE_TRIPLANAR > 0.0 || USE_REGIONS > 0.0)
	{// Can skip nearly everything... These are always going to be solid color...
		gl_FragColor = vec4(1.0);
	}
#ifdef __TEST_SKY__
	else if (true)
	{
		gl_FragColor = GetTestSky( var_TexCoords );
	}
#endif //__TEST_SKY__
	else
	{
		vec3 nightDiffuse = vec3(0.0);
		vec2 texCoords = var_TexCoords;

		if (PROCEDURAL_SKY_ENABLED <= 0.0)
		{
			gl_FragColor = texture(u_DiffuseMap, texCoords);
#ifdef __HIGH_PASS_SHARPEN__
			gl_FragColor.rgb = Enhance(u_DiffuseMap, texCoords, gl_FragColor.rgb, 1.0);
#endif //__HIGH_PASS_SHARPEN__
		}
		else
		{
			vec3 atmos = extra_cheap_atmosphere(skyViewDir, skyViewDir2, skySunDir, sunColorMod);

#ifdef __BACKGROUND_HILLS__
			if (SHADER_SKY_DIRECTION == 5.0 && SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE >= 1.0)
			{// At night, just do a black lower sky side...
				terrainColor = vec4(0.0, 0.0, 0.0, 1.0);
			}
			else if (SHADER_SKY_DIRECTION != 4.0 && SHADER_SKY_DIRECTION != 5.0)
			{
				terrainColor.rgb = mix(atmos, vec3(0.1), clamp(SHADER_NIGHT_SCALE * 2.0, 0.0, 1.0));
				terrainColor.a = 0.0;
				GetBackgroundHills( terrainColor, texCoords, vec3(0.0), skyViewDir );
			}
#endif //__BACKGROUND_HILLS__

			if (terrainColor.a != 1.0 && u_MoonCount > 0.0)
			{// In the day, we still want to draw planets... Only if this is not background terrain...
				GetPlanets(pCol, var_Position);

				if (pCol.a > 0.0)
				{// Planet here, blend this behind the atmosphere...
					atmos = mix(atmos, clamp(pCol.rgb, 0.0, 1.0), 0.3);
				}
			}

			gl_FragColor.rgb = clamp(atmos, 0.0, 1.0);
			gl_FragColor.a = 1.0;
		}

#if (defined(__CLOUDS__) || defined(__CLOUDS2__)) && !defined(CLOUD_QUALITY0)
		if (terrainColor.a != 1.0 && CLOUDS_ENABLED > 0.0 && SHADER_SKY_DIRECTION != 5.0)
		{// Procedural clouds are enabled...
			float nMult = 1.0;
			float cdMult = 1.0;

			if (cloudiness >= 0.175)
			{// Darken thick clouds...
				cdMult = clamp(1.5 - ((cloudiness - 0.175) / 0.125), 0.0, 1.0);
			}
			
			if (SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0)
			{// Adjust cloud color at night...
				nMult = clamp(1.5 - SHADER_NIGHT_SCALE, 0.0, 1.0);
			}

			nMult = min(cdMult, nMult);
			clouds = Clouds(nMult);
		}
#endif //(defined(__CLOUDS__) || defined(__CLOUDS2__)) && !defined(CLOUD_QUALITY0)

		if (pCol.a <= 0.0 && terrainColor.a != 1.0)
		{
			GetSun(sun, var_Position);

			if (sun.a > 0.0)
			{
				gl_FragColor = vec4(sun.rgb * sunColorMod, sun.a*(1.0-clouds.a));
			}
		}

#ifdef __RAINBOWS__
		vec4 rainbow = Rainbow(skyViewDir, vec3(0.9, 1.0, 0.3));
		gl_FragColor.rgb = mix(gl_FragColor.rgb, gl_FragColor.rgb + rainbow.rgb, rainbow.a);
#endif //__RAINBOWS__

#define night_const_1 (PROCEDURAL_SKY_NIGHT_HDR_MIN / 255.0)
#define night_const_2 (255.0 / PROCEDURAL_SKY_NIGHT_HDR_MAX)

		if (terrainColor.a != 1.0)
		{// This is sky, and aurora is enabled...
			if (SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0)
			{// Day/Night cycle is enabled, and some night sky contribution is required...
				float atmosMix = 0.8;

				if (pCol.a > 0.0)
				{// Planet here, draw this instead of stars...
					nightDiffuse = clamp(pCol.rgb, 0.0, 1.0);
					atmosMix = pCol.a;
				}
				else if (PROCEDURAL_SKY_ENABLED > 0.0)
				{
					vec4 nCol;
					GetStars(nCol, var_Position);
					nightDiffuse = clamp(nCol.rgb, 0.0, 1.0);
					nightDiffuse *= PROCEDURAL_SKY_NIGHT_COLOR.rgb;
					nightDiffuse = clamp((clamp(nightDiffuse - night_const_1, 0.0, 1.0)) * night_const_2, 0.0, 1.0);
				}
				else
				{
					nightDiffuse = texture(u_OverlayMap, texCoords).rgb;
#ifdef __HIGH_PASS_SHARPEN__
					nightDiffuse.rgb = Enhance(u_OverlayMap, texCoords, nightDiffuse.rgb, 1.0);
#endif //__HIGH_PASS_SHARPEN__
				}

				{// Mis in night atmosphere color...
					vec3 position = var_Position.xzy;
					vec3 lightPosition = u_PrimaryLightOrigin.xzy;

					vec3 skyViewDir = normalize(position);
					vec3 skyViewDir2 = normalize(u_ViewOrigin.xzy - var_Position.xzy);
					vec3 skySunDir = normalize(lightPosition);
					vec3 atmos = extra_cheap_atmosphere(skyViewDir, skyViewDir2, -skySunDir, sunColorMod);
					atmos = atmos * 1.75; // boost it at night a bit...
					nightDiffuse = mix(atmos, nightDiffuse, atmosMix);
				}

				gl_FragColor.rgb = mix(gl_FragColor.rgb, nightDiffuse, SHADER_NIGHT_SCALE); // Mix in night sky with original sky from day -> night...
				nightGlow = nightDiffuse * SHADER_NIGHT_SCALE;
			}

#ifdef __AURORA2__
			if (SHADER_SKY_DIRECTION != 5.0																/* Not down sky textures */
				&& SHADER_AURORA_ENABLED > 0.0																							/* Auroras Enabled */
				&& ((SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0) /* Night Aurora */ || SHADER_AURORA_ENABLED >= 2.0		/* Forced day Aurora */))
			{// Aurora is enabled, and this is not up/down sky textures, add a sexy aurora effect :)
				vec4 aucolor = vec4(0.0);
				
				if (AURORA_STRENGTH1 > 0.0)
				{
					aucolor = GetAurora2(texCoords) * AURORA_STRENGTH1;
				}
				
				if (SHADER_SKY_DIRECTION != 4.0 && AURORA_STRENGTH2 > 0.0)
				{
					vec4 aucolor2 = GetAurora(texCoords) * AURORA_STRENGTH2;
					aucolor = clamp(aucolor + aucolor2, 0.0, 1.0);
				}

				gl_FragColor.rgb = mix(gl_FragColor.rgb, gl_FragColor.rgb + aucolor.rgb, aucolor.a);
			}
#else //!__AURORA2__
			if (SHADER_SKY_DIRECTION != 4.0 && SHADER_SKY_DIRECTION != 5.0																/* Not up/down sky textures */
				&& SHADER_AURORA_ENABLED > 0.0																							/* Auroras Enabled */
				&& ((SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0) /* Night Aurora */ || SHADER_AURORA_ENABLED >= 2.0		/* Forced day Aurora */))
			{// Aurora is enabled, and this is not up/down sky textures, add a sexy aurora effect :)
				vec4 aucolor = GetAurora(texCoords);
				gl_FragColor.rgb = mix(gl_FragColor.rgb, gl_FragColor.rgb + aucolor.rgb, aucolor.a);
			}
#endif //__AURORA2__
		}

#if defined(__CLOUDS__) || defined(__CLOUDS2__)
		if (clouds.a > 0.0 && terrainColor.a != 1.0 && CLOUDS_ENABLED > 0.0 && SHADER_SKY_DIRECTION != 5.0)
		{// Procedural clouds are enabled...
			float cloudMix = clamp(pow(clouds.a, mix(0.75, 0.5, SHADER_NIGHT_SCALE)), 0.0, 1.0);
			gl_FragColor.rgb = mix(gl_FragColor.rgb, clouds.rgb, cloudMix);
			nightGlow *= 1.0 - cloudMix;

#ifdef __LIGHTNING__
			if (cloudiness >= 0.275)
			{// Distant lightning strikes...
				vec4 lightning = GetLightning(var_Position.xyz*1025.0);
				gl_FragColor.rgb += lightning.rgb * lightning.a * 0.5;
				sun = max(sun, lightning * 0.5);
			}
#endif //__LIGHTNING__
		}
#endif //defined(__CLOUDS__) || defined(__CLOUDS2__)

#ifdef __BACKGROUND_HILLS__
		if (PROCEDURAL_BACKGROUND_HILLS_ENABLED > 0.0 && SHADER_SKY_DIRECTION != 4.0 && SHADER_SKY_DIRECTION != 5.0)
		{// Only on horizontal sides.
			gl_FragColor.rgb = mix(gl_FragColor.rgb, terrainColor.rgb, terrainColor.a > 0.0 ? 1.0 : 0.0);
		}
#endif //__BACKGROUND_HILLS__

		// Tonemap.
		gl_FragColor.rgb = pow( gl_FragColor.rgb, vec3(0.7) );
	
		// Contrast and saturation.
		gl_FragColor.rgb = gl_FragColor.rgb*gl_FragColor.rgb*(3.0-2.0*gl_FragColor.rgb);
		gl_FragColor.rgb = mix( gl_FragColor.rgb, vec3(dot(gl_FragColor.rgb,vec3(0.33))), -0.5 );
	}

	gl_FragColor.a = 1.0; // just force it.
	
	if (terrainColor.a != 1.0 && SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.7)
	{// Add night sky to glow map...
		out_Glow = vec4(nightGlow, gl_FragColor.a);

		if (PROCEDURAL_SKY_ENABLED > 0.0)
		{
			out_Glow *= vec4(1.0, 1.0, 1.0, 8.0);
			out_Glow.a *= PROCEDURAL_SKY_NIGHT_COLOR.a;
		}
		
		// Scale by closeness to actual night...
		float mult = (SHADER_NIGHT_SCALE - 0.7) * 3.333;
		out_Glow *= mult;

		out_Glow = max(out_Glow, sun); // reusing sun for lightning flashes as well...

		// And enhance contrast...
		out_Glow.rgb *= out_Glow.rgb;

		// And reduce over-all brightness because it's sky and not a close light...
		out_Glow.rgb *= 0.5;
	}
	else
	{
		if (terrainColor.a <= 0.0 && sun.a > 0.0)
			out_Glow = sun*0.3*(1.0-clouds.a);
		else
			out_Glow = vec4(0.0);
	}

	out_Position = vec4(normalize(var_Position.xyz) * 524288.0, 1025.0);
	out_Normal = vec4(EncodeNormal(-skyViewDir.rbg/*var_Normal.rgb*/), 0.0, 1.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
}
