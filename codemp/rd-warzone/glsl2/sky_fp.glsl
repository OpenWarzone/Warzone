#define __HIGH_PASS_SHARPEN__
#define __CLOUDS__
#define __BACKGROUND_HILLS__

#define SCREEN_MAPS_ALPHA_THRESHOLD 0.666

uniform sampler2D										u_DiffuseMap;
uniform sampler2D										u_OverlayMap; // Night sky image... When doing sky...
uniform sampler2D										u_SplatMap1; // auroraImage[0]
uniform sampler2D										u_SplatMap2; // auroraImage[1]

uniform int												u_MoonCount; // moons total count
uniform sampler2D										u_MoonMaps[8]; // moon textures
uniform vec4											u_MoonInfos[8]; // MOON_ENABLED, MOON_ROTATION_OFFSET_X, MOON_ROTATION_OFFSET_Y, MOON_SIZE
uniform vec2											u_MoonInfos2[8]; // MOON_BRIGHTNESS, MOON_TEXTURE_SCALE

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
uniform vec4											u_Local2; // PROCEDURAL_CLOUDS_ENABLED, PROCEDURAL_CLOUDS_CLOUDSCALE, PROCEDURAL_CLOUDS_SPEED, PROCEDURAL_CLOUDS_DARK
uniform vec4											u_Local3; // PROCEDURAL_CLOUDS_LIGHT, PROCEDURAL_CLOUDS_CLOUDCOVER, PROCEDURAL_CLOUDS_CLOUDALPHA, PROCEDURAL_CLOUDS_SKYTINT
uniform vec4											u_Local4; // PROCEDURAL_SKY_NIGHT_HDR_MIN, PROCEDURAL_SKY_NIGHT_HDR_MAX, PROCEDURAL_SKY_PLANETARY_ROTATION, PROCEDURAL_SKY_NEBULA_FACTOR
uniform vec4											u_Local5; // dayNightEnabled, nightScale, skyDirection, auroraEnabled -- Sky draws only!
uniform vec4											u_Local6; // PROCEDURAL_SKY_DAY_COLOR
uniform vec4											u_Local7; // PROCEDURAL_SKY_NIGHT_COLOR
uniform vec4											u_Local8; // AURORA_COLOR
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
#define CLOUDS_SPEED									u_Local2.b
#define CLOUDS_DARK										u_Local2.a

#define CLOUDS_LIGHT									u_Local3.r
#define CLOUDS_CLOUDCOVER								u_Local3.g
#define CLOUDS_CLOUDALPHA								u_Local3.b
#define CLOUDS_SKYTINT									u_Local3.a

#define PROCEDURAL_SKY_NIGHT_HDR_MIN					u_Local4.r
#define PROCEDURAL_SKY_NIGHT_HDR_MAX					u_Local4.g
#define PROCEDURAL_SKY_PLANETARY_ROTATION				u_Local4.b
#define PROCEDURAL_SKY_NEBULA_FACTOR				u_Local4.a

#define SHADER_DAY_NIGHT_ENABLED						u_Local5.r
#define SHADER_NIGHT_SCALE								u_Local5.g
#define SHADER_SKY_DIRECTION							u_Local5.b
#define SHADER_AURORA_ENABLED							u_Local5.a

#define PROCEDURAL_SKY_DAY_COLOR						u_Local6
#define PROCEDURAL_SKY_NIGHT_COLOR						u_Local7

#define AURORA_COLOR									u_Local8

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

float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float hash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float pnoise(in vec3 o) 
{
	vec3 p = floor(o);
	vec3 fr = fract(o);
		
	float n = p.x + p.y*57.0 + p.z * 1009.0;

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

float SmoothNoise( vec3 p )
{
#if 0
    float f;
    f  = 0.5000*pnoise( p ); p = m*p*2.02;
    f += 0.2500*pnoise( p ); 
	
    return f * (1.0 / (0.5000 + 0.2500));
#else
	return pnoise(p);
#endif
}

const mat2 mc = mat2(1.6, 1.2, -1.2, 1.6);

vec2 hash(vec2 p) {
	p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise(in vec2 p) {
	const float K1 = 0.366025404; // (sqrt(3)-1)/2;
	const float K2 = 0.211324865; // (3-sqrt(3))/6;
	vec2 i = floor(p + (p.x + p.y)*K1);
	vec2 a = p - i + (i.x + i.y)*K2;
	vec2 o = (a.x>a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0); //vec2 of = 0.5 + 0.5*vec2(sign(a.x-a.y), sign(a.y-a.x));
	vec2 b = a - o + K2;
	vec2 c = a - 1.0 + 2.0*K2;
	vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
	vec3 n = h*h*h*h*vec3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
	return dot(n, vec3(70.0));
}

float fbm(vec2 n) {
	float total = 0.0, amplitude = 0.1;
	for (int i = 0; i < 7; i++) {
		total += noise(n) * amplitude;
		n = mc * n;
		amplitude *= 0.4;
	}
	return total;
}

vec3 extra_cheap_atmosphere(vec3 raydir, vec3 skyViewDir2, vec3 sunDir, inout vec3 sunColorMod) {
	vec3 sundir = sunDir;
	sundir.y = abs(sundir.y);
	float sunDirLength = pow(clamp(length(sundir.y), 0.0, 1.0), 2.25);
	float rayDirLength = pow(clamp(length(raydir.y), 0.0, 1.0), 0.85);
	float special_trick = 1.0 / (rayDirLength * 1.0 + 0.2);
	float special_trick2 = 1.0 / (length(raydir.y) * 3.0 + 1.0);
	
	vec3 skyColor = PROCEDURAL_SKY_DAY_COLOR.rgb;
	vec3 bluesky = skyColor;
	vec3 bluesky2 = max(bluesky, bluesky - skyColor * 0.0896 * (special_trick + -6.0 * sunDirLength * sunDirLength));
	
	float dotSun = dot(sundir, raydir);
	float raysundt = pow(abs(dotSun), 2.0);
	bluesky2 *= special_trick * (0.24 + raysundt * 0.24);

	bluesky = clamp(bluesky, 0.0, 1.0);
	bluesky2 = clamp(bluesky2, 0.0, 1.0);

	// sunset
	float sunsetIntensity = 0.9;
	float sundt = pow(max(0.0, dotSun), 8.0);
	float my = clamp(1.25-length(raydir.y*1.25), 0.0, 1.0);
	my *= pow(clamp(3.0-distance(raydir.y, sundir.y), 0.0, 1.0), 1.25);
	float mymie = pow(clamp(clamp(sundt, 0.1, 1.0) * (special_trick2 * 0.05 + 0.95), 0.0, sunsetIntensity), 0.75) * 0.9 * my;
	vec3 suncolor = vec3(1.0, 0.5, 0.0);
	vec3 color = (bluesky * 0.333 + bluesky2 * 0.333) + (mymie * suncolor);
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
vec3 Clouds(in vec2 fragCoord, vec3 skycolour)
{
	//vec3 skycolour1 = skycolour;
	//vec3 skycolour2 = skycolour;
	vec4 fragColor = vec4(0.0);
	vec2 p = fragCoord.xy;// / u_Dimensions.xy;
	vec2 uv = p;
	float time = u_Time * CLOUDS_SPEED;
	float q = fbm(uv * CLOUDS_CLOUDSCALE * 0.5);

	//ridged noise shape
	float r = 0.0;
	uv *= CLOUDS_CLOUDSCALE;
	uv -= q - time;
	float weight = 0.8;
	for (int i = 0; i<8; i++) {
		r += abs(weight*noise(uv));
		uv = mc*uv + time;
		weight *= 0.7;
	}

	//noise shape
	float f = 0.0;
	uv = p;
	uv *= CLOUDS_CLOUDSCALE;
	uv -= q - time;
	weight = 0.7;
	for (int i = 0; i<8; i++) {
		f += weight*noise(uv);
		uv = mc*uv + time;
		weight *= 0.6;
	}

	f *= r + f;

	//noise colour
	float c = 0.0;
	time = u_Time * CLOUDS_SPEED * 2.0;
	uv = p;
	uv *= CLOUDS_CLOUDSCALE*2.0;
	uv -= q - time;
	weight = 0.4;
	for (int i = 0; i<7; i++) {
		c += weight*noise(uv);
		uv = mc*uv + time;
		weight *= 0.6;
	}

	//noise ridge colour
	float c1 = 0.0;
	time = u_Time * CLOUDS_SPEED * 3.0;
	uv = p;
	uv *= CLOUDS_CLOUDSCALE*3.0;
	uv -= q - time;
	weight = 0.4;
	for (int i = 0; i<7; i++) {
		c1 += abs(weight*noise(uv));
		uv = mc*uv + time;
		weight *= 0.6;
	}

	c += c1;

	//vec3 skycolour = mix(skycolour2, skycolour1, p.y);
	vec3 cloudcolour = vec3(1.1, 1.1, 0.9) * clamp((CLOUDS_DARK + CLOUDS_LIGHT*c), 0.0, 1.0);

	f = CLOUDS_CLOUDCOVER + CLOUDS_CLOUDALPHA*f*r;

	vec3 result = mix(skycolour, clamp(CLOUDS_SKYTINT * skycolour + cloudcolour, 0.0, 1.0), clamp(f + c, 0.0, 1.0));

	return result;
}
#endif //__CLOUDS__

vec3 reachForTheNebulas(in vec3 from, in vec3 dir, float level, float power) 
{
    vec3 color = vec3(0.0);
    float nebula = pow(SmoothNoise(dir+vec3(PROCEDURAL_SKY_NEBULA_SEED)), 12.0 / (1.0 - clamp(PROCEDURAL_SKY_NEBULA_FACTOR, 0.0, 0.999)));
    
    if (nebula > 0.0)
    {
    	vec3 pos = (dir.xyz + dir.xzy + dir.zyx) / 3.0;
    	vec3 randc = vec3(SmoothNoise( dir.xyz*10.0*level));
		color = nebula * randc;
    }

	return pow(color*2.25, vec3(power));
}

vec3 reachForTheStars(in vec3 from, in vec3 dir, float power) 
{
	float star = pow(SmoothNoise(dir*320.0), 48.0 - clamp(PROCEDURAL_SKY_STAR_DENSITY, 0.0, 16.0));
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
	vec3 color1=clamp(reachForTheNebulas(from, dir, 1.0, 0.5) * 1.5, 0.0, 1.0) * vec3(0.0, 0.0, 1.0);
    vec3 color2=clamp(reachForTheNebulas(from, dir, 2.0, 0.5) * 1.5, 0.0, 1.0) * vec3(0.0, 1.0, 1.0);
	
    vec3 color3=clamp(reachForTheNebulas(from, -dir, 2.0, 0.5) * 0.9, 0.0, 1.0) * vec3(1.0, 0.0, 0.0);
    vec3 color4=clamp(reachForTheNebulas(from, -dir, 3.0, 0.5) * 0.7, 0.0, 1.0) * vec3(1.0, 1.0, 0.0);
    
    vec3 color5=clamp(reachForTheNebulas(from, dir.yxz+dir.yzx, 1.5, 0.9) * 0.9, 0.0, 1.0) * vec3(0.0, 1.0, 0.0);
    vec3 color6=clamp(reachForTheNebulas(from, dir.yxz+dir.yzx, 2.5, 0.7) * 0.7, 0.0, 1.0) * vec3(0.25, 0.75, 0.0);

	// Small stars...
	vec3 colorStars = clamp(reachForTheStars(from, dir, 0.9), 0.0, 1.0);

	// Add them all together...
	color = color1 + color2 + color3 + color4 + color5 + color6 + colorStars;

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
    f += 0.5000*SmoothNoise( p ); p = m3*p*1.22*PROCEDURAL_BACKGROUND_HILLS_SEED;
    f += 0.2500*pnoise( p ); p = m3*p*1.53*PROCEDURAL_BACKGROUND_HILLS_SEED;
    f += 0.1250*pnoise( p ); p = m3*p*4.01*PROCEDURAL_BACKGROUND_HILLS_SEED;
    f += 0.0625*pnoise( p );
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

				color *= AURORA_COLOR.rgb;

				gl_FragColor.rgb = mix(gl_FragColor.rgb, gl_FragColor.rgb + color, auroraPower * str);
			}
		}

#ifdef __CLOUDS__
		if (CLOUDS_ENABLED > 0.0 && SHADER_SKY_DIRECTION != 5.0)
		{// Procedural clouds are enabled...
			vec3 pViewDir = normalize(var_Position.xyz);

			vec3 cloudColor = Clouds(pViewDir.xy * 0.5 + 0.5, gl_FragColor.rgb);

			if (SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0)
			{// Adjust cloud color at night...
				float nMult = clamp(1.25 - SHADER_NIGHT_SCALE, 0.0, 1.0);
				cloudColor *= nMult;
			}

			gl_FragColor.rgb = mix(gl_FragColor.rgb, cloudColor, clamp(pow(pViewDir.z, 2.5), 0.0, 1.0));
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

	out_Position = vec4(var_Position.rgb, 1025.0);
	out_Normal = vec4(EncodeNormal(var_Normal.rgb), 0.0, 1.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
}
