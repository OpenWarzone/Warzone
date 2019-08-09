#define __HIGH_PASS_SHARPEN__
#define __CLOUDS__
#define __LIGHTNING__
#define __BACKGROUND_HILLS__

#define SCREEN_MAPS_ALPHA_THRESHOLD 0.666

uniform sampler2D										u_DiffuseMap;
uniform sampler2D										u_OverlayMap; // Night sky image... When doing sky...
uniform sampler2D										u_SplatMap1; // auroraImage[0]
uniform sampler2D										u_SplatMap2; // auroraImage[1]
uniform sampler2D										u_SplatMap3; // smoothNoiseImage
uniform sampler2D										u_RoadMap; // random2KImage

uniform int												u_MoonCount; // moons total count
uniform sampler2D										u_MoonMaps[4]; // moon textures
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

const mat3 mcl = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 ) * 1.7;

//--------------------------------------------------------------------------
float FBM( vec3 p )
{
	p *= CLOUDS_CLOUDSCALE;

	float f;
	
#if 0
	/* Mix of texture and procedural noises for speed */
	p *= .000675;
	f = 0.5000 * tNoise(p); p = mcl*p;
	f += 0.2500 * tNoise(p); p = mcl*p;
	f += 0.1250 * pNoise(p); p = mcl*p;
	f += 0.0625   * pNoise(p); p = mcl*p;
	f += 0.03125  * tNoise(p); p = mcl*p;
	f += 0.015625 * tNoise(p);
#elif 1
	/* Mix of texture and procedural noises for speed */
	p *= .000675;
	f = 0.5000 * pNoise(p); p = mcl*p;
	f += 0.2500 * pNoise(p); p = mcl*p;
	f += 0.1250 * pNoise(p); p = mcl*p;
	f += 0.0625   * pNoise(p); p = mcl*p;
	f += 0.03125  * pNoise(p); p = mcl*p;
	f += 0.015625 * pNoise(p);
#elif 0
	p *= .0001;
	vec2 r1 = textureLod(u_RoadMap, p.xz, 2.0).rg;
	p = p * mcl;
	vec2 r2 = textureLod(u_RoadMap, p.xz, 1.0).rg;
	p = p * mcl;
	vec2 r3 = textureLod(u_RoadMap, p.xz, 0.0).rg;

	f = 0.5000 * r1.r;
	f += 0.2500 * r1.g;
	f += 0.1250 * r2.r;
	f += 0.0625 * r2.g;
	f += 0.03125 * r3.r;
	f += 0.015625 * r3.g;

	f = pow(f, 1.05);
#else
	p *= .0005;
	f = 0.5000 * pNoise(p); p = mcl*p;
	f += 0.2500 * pNoise(p); p = mcl*p;
	f += 0.1250 * pNoise(p); p = mcl*p;
	f += 0.0625 * pNoise(p);
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
#define RAY_TRACE_STEPS 2 //55

vec3 sunLight  = normalize( u_PrimaryLightOrigin.xzy - u_ViewOrigin.xzy );
vec3 sunColour = u_PrimaryLightColor.rgb;

float gTime;
float cloudy = 0.0;
float cloudShadeFactor = 0.6;
float flash = 0.0;

#define CLOUD_LOWER 2800.0
#define CLOUD_UPPER 6800.0


//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
float MapSH(vec3 p)
{
	//float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-.6);
	//float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-pow(cloudy, 0.3));
	float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-cloudShadeFactor);
	//h *= smoothstep(CLOUD_LOWER, CLOUD_LOWER+100., p.y);
	//h *= smoothstep(CLOUD_LOWER-500., CLOUD_LOWER, p.y);
	h *= smoothstep(CLOUD_UPPER+100., CLOUD_UPPER, p.y);
	return h;
}

float Map(vec3 p)
{
	//float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-.6);
	//float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-pow(cloudy, 0.3));
	float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-cloudShadeFactor);
	return h;
}


//--------------------------------------------------------------------------
float GetLighting(vec3 p, vec3 s)
{
    float l = MapSH(p)-MapSH(p+s*200.0);
    return clamp(-l, 0.1, 0.4) * 1.25;
}

//--------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
vec4 GetSky(in vec3 pos,in vec3 rd, out vec2 outPos)
{
	//float sunAmount = max( dot( rd, sunLight), 0.0 );
	
	// Find the start and end of the cloud layer...
	float beg = ((CLOUD_LOWER-pos.y) / rd.y);
	float end = ((CLOUD_UPPER-pos.y) / rd.y);
	
	// Start position...
	vec3 p = vec3(pos.x + rd.x * beg, 0.0, pos.z + rd.z * beg);
	outPos = p.xz;
    beg +=  Hash(p)*150.0;

	// Trace clouds through that layer...
	float d = 0.0;
	vec3 add = rd * ((end-beg) / float(RAY_TRACE_STEPS));
	vec2 shade;
	vec2 shadeSum = vec2(0.0);
	shade.x = 1.0;
	
	// I think this is as small as the loop can be
	// for a reasonable cloud density illusion.
	for (int i = 0; i < RAY_TRACE_STEPS; i++)
	{
		if (shadeSum.y >= 1.0) break;

		float h = clamp(Map(p)*2.0, 0.0, 1.0);
		shade.y = max(h, 0.0);
        shade.x = GetLighting(p, sunLight);
		shadeSum += shade * (1.0 - shadeSum.y);
		p += add;
	}

	float final = shadeSum.x;
	final += flash * (shadeSum.y+final+.2) * .5;

	return clamp(vec4(final, final, final, shadeSum.y), 0.0, 1.0);
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
	
	
	vec4 col;
	vec2 pos;
	col = GetSky(cameraPos, dir, pos);

	col.rgb = clamp(col.rgb * (64.0 * colorMult), 0.0, 1.0);

	float l = exp(-length(pos) * .00002);
	col.rgb = mix(vec3(.6-cloudy*1.2)+flash*.3, col.rgb, max(l, .2));
	
	// Stretch RGB upwards... 
	col.rgb = pow(col.rgb, vec3(.7));
	
	col = clamp(col, 0.0, 1.0);

	float alpha = col.a;
	alpha *= alphaMult;
	return vec4(col.rgb, alpha);
}
#endif //__CLOUDS__

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
	//p = rotate(p, iTime);
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
	vec3 color = vec3(star);
	return pow(color*2.25, vec3(power));
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

	// Nebulae...
	vec3 color1=clamp(reachForTheNebulas(from, -dir.xyz, 64.0, 0.5), 0.0, 1.0) * vec3(0.0, 0.0, 1.0);
	//vec3 color2=clamp(reachForTheNebulas(from, -dir.zxy, 64.0, 0.7), 0.0, 1.0) * vec3(0.0, 1.0, 0.0);
	vec3 color3=clamp(reachForTheNebulas(from, -dir.yxz, 64.0, 0.7), 0.0, 1.0) * vec3(1.0, 0.0, 0.0);

	// Small stars...
	vec3 colorStars = clamp(reachForTheStars(from, dir, 0.9), 0.0, 1.0);

	// Add them all together...
	color = color1 /*+ color2*/ + color3 + colorStars;

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
			vec3 sunsetSun = vec3(1.0, 0.8, 0.625);
			//vec3 sunsetSun = vec3(u_Local9.r, u_Local9.g, u_Local9.b);
			lightColor = mix(lightColor, sunsetSun, SHADER_NIGHT_SCALE);
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

vec3 lig = normalize(u_PrimaryLightOrigin.xzy/*vec3( 0.3,0.5, 0.6)*/);

// terrain functions
float terrainMap( const in vec3 p ) 
{
    float dist = pow(length(p) / 64.0, 0.2);
    return (((hillsFbm( (p.xzz*0.5+16.0)*0.00346 ) * 1.5 - PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS)*250.0*dist)+(dist*8.0)) - PROCEDURAL_BACKGROUND_HILLS_UPDOWN;
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
		
		dist = t;
		t -= pos.y*3.5;
		alpha = clamp(exp(-0.0000005*t*t), 0.0, 0.3);
		col = mix( bgc, col, alpha );

		return vec4(col, 1.0);
	}

	//return vec4(col, alpha >= 0.1 ? 1.0 : 0.0);
	return vec4(0.0);
}

void GetBackgroundHills( inout vec4 fragColor, in vec2 fragCoord, vec3 ro, vec3 rd ) {
	fragColor = raymarchTerrain( ro, rd, fragColor.rgb, 1200.0, 1200.0 );
}
#endif //__BACKGROUND_HILLS__

void main()
{
	vec4 terrainColor = vec4(0.0);
	vec3 nightGlow = vec3(0.0);
	vec3 sunColorMod = vec3(1.0);
	vec4 sun = vec4(0.0);

	if (USE_TRIPLANAR > 0.0 || USE_REGIONS > 0.0)
	{// Can skip nearly everything... These are always going to be solid color...
		gl_FragColor = vec4(1.0);
	}
	else
	{
		vec3 nightDiffuse = vec3(0.0);
		vec2 texCoords = var_TexCoords;

		if (PROCEDURAL_SKY_ENABLED <= 0.0)
		{
			gl_FragColor = texture(u_DiffuseMap, texCoords);
#ifdef __HIGH_PASS_SHARPEN__
			gl_FragColor.rgb = Enhance(u_DiffuseMap, texCoords, gl_FragColor.rgb, 1.0/*8.0*/);
#endif //__HIGH_PASS_SHARPEN__
		}
		else
		{
			vec3 position = var_Position.xzy;
			vec3 lightPosition = u_PrimaryLightOrigin.xzy;

			vec3 skyViewDir = normalize(position);
			vec3 skyViewDir2 = normalize(u_ViewOrigin.xzy - var_Position.xzy);
			vec3 skySunDir = normalize(lightPosition);
			vec3 atmos = extra_cheap_atmosphere(skyViewDir, skyViewDir2, skySunDir, sunColorMod);

#ifdef __BACKGROUND_HILLS__
			if (SHADER_SKY_DIRECTION != 4.0 && SHADER_SKY_DIRECTION != 5.0)
			{
				terrainColor.rgb = mix(atmos, vec3(0.1), clamp(SHADER_NIGHT_SCALE * 2.0, 0.0, 1.0));
				terrainColor.a = 0.0;
				GetBackgroundHills( terrainColor, texCoords, vec3(0.0), skyViewDir );
			}
#endif //__BACKGROUND_HILLS__

			if (terrainColor.a <= 0.0)
			{// In the day, we still want to draw planets... Only if this is not background terrain...
				vec4 pCol;
				GetPlanets(pCol, var_Position);

				if (pCol.a > 0.0)
				{// Planet here, blend this behind the atmosphere...
					atmos = mix(atmos, pCol.rgb, 0.3/*pCol.a*/);
				}
			}

			gl_FragColor.rgb = clamp(atmos, 0.0, 1.0);
			gl_FragColor.a = 1.0;
		}

		GetSun(sun, var_Position);

		if (sun.a > 0.0)
		{
			gl_FragColor = vec4(sun.rgb * sunColorMod, sun.a);
		}

		if (SHADER_SKY_DIRECTION == 5.0 && SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE >= 1.0)
		{// At night, just do a black lower sky side...
			terrainColor = vec4(0.0, 0.0, 0.0, 1.0);
		}

#define night_const_1 (PROCEDURAL_SKY_NIGHT_HDR_MIN / 255.0)
#define night_const_2 (255.0 / PROCEDURAL_SKY_NIGHT_HDR_MAX)

		if (/*SHADER_MATERIAL_TYPE == 1024.0 &&*/ terrainColor.a != 1.0)
		{// This is sky, and aurora is enabled...
			if (SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0)
			{// Day/Night cycle is enabled, and some night sky contribution is required...
				float atmosMix = 0.8;
				vec4 pCol;
				GetPlanets(pCol, var_Position);

				if (pCol.a > 0.0)
				{// Planet here, draw this instead of stars...
					nightDiffuse = pCol.rgb;
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

				{
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

			if (SHADER_SKY_DIRECTION != 4.0 && SHADER_SKY_DIRECTION != 5.0													/* Not up/down sky textures */
				&& SHADER_AURORA_ENABLED > 0.0																		/* Auroras Enabled */
				&& ((SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0) /* Night Aurora */ || SHADER_AURORA_ENABLED >= 2.0		/* Forced day Aurora */))
			{// Aurora is enabled, and this is not up/down sky textures, add a sexy aurora effect :)
				vec2 fragCoord = texCoords;

				if (SHADER_SKY_DIRECTION == 2.0 || SHADER_SKY_DIRECTION == 3.0)
				{// Forward or back sky textures, invert the X axis to make the aura seamless...
					fragCoord.x = 1.0 - fragCoord.x;
				}

				float auroraPower;

				if (SHADER_AURORA_ENABLED >= 2.0)
					auroraPower = 1.0; // Day enabled aurora - always full strength...
				else
					auroraPower = SHADER_NIGHT_SCALE;

				vec2 uv = fragCoord.xy;
				
				// Move aurora up a bit above horizon...
				uv *= 0.8;
				uv += 0.2;

				uv = clamp(uv, 0.0, 1.0);
   
#define TAU 6.2831853071
#define time u_Time * 0.5


				float o = texture(u_SplatMap1, uv * 0.25 + vec2(0.0, time * 0.025)).r;
				float d = (texture(u_SplatMap2, uv * 0.25 - vec2(0.0, time * 0.02 + o * 0.02)).r * 2.0 - 1.0);
    
				float v = uv.y + d * 0.1;
				v = 1.0 - abs(v * 2.0 - 1.0);
				v = pow(v, 2.0 + sin((time * 0.2 + d * 0.25) * TAU) * 0.5);
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

				float s = mix(r.x, (sin((time * 2.5 + 60.0) * r.y) * 0.5 + 0.5) * ((r.y * r.y) * (r.y * r.y)), 0.04); 
				color += clamp(pow(s, 70.0) * (1.0 - v), 0.0, 1.0);
				float str = max(color.r, max(color.g, color.b));

				color *= 0.7;

				color *= AURORA_COLOR;

				gl_FragColor.rgb = mix(gl_FragColor.rgb, gl_FragColor.rgb + color, auroraPower * str);
			}
		}

#ifdef __CLOUDS__
		if (CLOUDS_ENABLED > 0.0 && SHADER_SKY_DIRECTION != 5.0)
		{// Procedural clouds are enabled...
			float cloudiness = clamp(CLOUDS_CLOUDCOVER*0.3, 0.0, 0.3);
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
			vec4 clouds = Clouds(nMult);

			gl_FragColor.rgb = mix(gl_FragColor.rgb, clouds.rgb, clouds.a);

#ifdef __LIGHTNING__
			if (cloudiness >= 0.275)
			{// Distant lightning strikes...
				vec4 lightning = GetLightning(var_Position.xyz*1025.0);
				gl_FragColor.rgb += lightning.rgb * lightning.a * 0.5;
				sun = max(sun, lightning * 0.5);
			}
#endif //__LIGHTNING__
		}
#endif //__CLOUDS__

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
	

		gl_FragColor.a *= var_Color.a;
	}

	gl_FragColor.a = 1.0; // just force it.
	
	if (/*SHADER_MATERIAL_TYPE == 1024.0 &&*/ SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.7 && terrainColor.a != 1.0)
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
		if (sun.a > 0.0 && terrainColor.a <= 0.0)
			out_Glow = sun;
		else
			out_Glow = vec4(0.0);
	}

	//out_Position = vec4(var_Position.rgb, 1025.0);
	out_Position = vec4(normalize(var_Position.xyz) * /*1048576.0*/524288.0, 1025.0);
	out_Normal = vec4(EncodeNormal(var_Normal.rgb), 0.0, 1.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
}
