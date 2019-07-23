uniform mat4		u_ModelViewProjectionMatrix;

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_ScreenDepthMap;
uniform sampler2D	u_PositionMap;
uniform sampler2D	u_WaterPositionMap;

uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec2		u_Dimensions;

uniform vec4		u_Local0;		// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec4		u_Local2;		// FOG_WORLD_COLOR_R, FOG_WORLD_COLOR_G, FOG_WORLD_COLOR_B, FOG_WORLD_ENABLE
uniform vec4		u_Local3;		// FOG_WORLD_COLOR_SUN_R, FOG_WORLD_COLOR_SUN_G, FOG_WORLD_COLOR_SUN_B, FOG_WORLD_ALPHA
uniform vec4		u_Local4;		// FOG_WORLD_CLOUDINESS, FOG_LAYER, FOG_LAYER_SUN_PENETRATION, FOG_LAYER_ALTITUDE_BOTTOM
uniform vec4		u_Local5;		// FOG_LAYER_COLOR_R, FOG_LAYER_COLOR_G, FOG_LAYER_COLOR_B, FOG_LAYER_ALPHA
uniform vec4		u_Local6;		// FOG_LAYER_INVERT, FOG_WORLD_WIND, FOG_LAYER_CLOUDINESS, FOG_LAYER_WIND
uniform vec4		u_Local7;		// nightScale, FOG_LAYER_ALTITUDE_TOP, FOG_LAYER_ALTITUDE_FADE, WATER_ENABLED
uniform vec4		u_Local8;		// sun color
uniform vec4		u_Local9;		// FOG_LAYER_BBOX
uniform vec4		u_Local10;		// MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], FOG_WORLD_FADE_ALTITUDE
uniform vec4		u_Local11;		// MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], FOG_LINEAR_ENABLE
uniform vec4		u_Local12;		// FOG_LINEAR_COLOR[0], FOG_LINEAR_COLOR[1], FOG_LINEAR_COLOR[2], FOG_LINEAR_ALPHA
uniform vec4		u_MapInfo;		// MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], FOG_LINEAR_RANGE_POW

uniform vec3		u_ViewOrigin;
uniform vec4		u_PrimaryLightOrigin;
uniform float		u_Time;

varying vec2		var_TexCoords;


#define WATER_ENABLED	u_Local7.a

vec4 positionMapAtCoord ( vec2 coord )
{
	vec4 pos = textureLod(u_PositionMap, coord, 0.0);

	/*if (WATER_ENABLED > 0.0)
	{
		bool isSky = (pos.a - 1.0 >= MATERIAL_SKY) ? true : false;

		float isWater = textureLod(u_WaterPositionMap, coord, 0.0).a;

		if (isWater > 0.0 || (isWater > 0.0 && isSky))
		{
			vec3 wMap = textureLod(u_WaterPositionMap, coord, 0.0).xyz;
		
			if ((wMap.z > pos.z || isSky) && u_ViewOrigin.z > wMap.z)
			{
				pos.xyz = wMap.xyz;
			}
		}
	}*/

	return pos;
}

#define			FOG_VOLUMETRIC_QUALITY				2//6//8/*3*///2				// 2 is just fine... Higher looks only slightly better at a much greater FPS cost.

//
// World fog...
//

#define			worldvAA									u_Local10.xzy
#define			worldvBB									u_Local11.xzy

#define			woldFogFadeAltitude							u_Local10.a
#define			worldFogHeight								worldvBB.y

float			worldFogThicknessInv						= 1. / (worldvBB.y - worldvAA.y);

float			worldFogAlpha								= clamp(u_Local3.a, 0.0, 1.0);

#define			worldFogSunColor							u_Local3.rgb
#define			worldFogColor								u_Local2.rgb

#define			worldFogCloudiness							u_Local4.r
#define			worldFogWind								u_Local6.g

float			worldFogWindTime							= u_Time * worldFogWind * worldFogCloudiness * 2000.0;


//
// Layer fog...
//

#define			FOG_LAYER_SUN_PENETRATION			u_Local4.b

#define			FOG_LAYER_ALTITUDE_BOTTOM			u_Local4.a
#define			FOG_LAYER_ALTITUDE_TOP				u_Local7.g
#define			FOG_LAYER_ALTITUDE_FADE				u_Local7.b

#define			FOG_LAYER_BBOX						u_Local9.rgba

#define			FOG_LAYER_COLOR						u_Local5.rgb
#define			FOG_LAYER_ALPHA						u_Local5.a

#define			FOG_LAYER_CLOUDINESS				u_Local6.b
#define			FOG_LAYER_WIND						u_Local6.a

#define			FOG_LAYER_SUN_COLOR					u_Local8.rgb

bool			FOG_LAYER_INVERT					= (u_Local6.r > 0.0) ? true : false;
float			fogBottom							= FOG_LAYER_INVERT ? FOG_LAYER_ALTITUDE_TOP : FOG_LAYER_ALTITUDE_BOTTOM;
float			fogHeight							= FOG_LAYER_INVERT ? FOG_LAYER_ALTITUDE_BOTTOM : FOG_LAYER_ALTITUDE_TOP;
float			fadeAltitude						= FOG_LAYER_ALTITUDE_FADE;
float			fogThicknessInv						= 1. / (fogHeight - fogBottom);

float			fogWindTime							= u_Time * FOG_LAYER_WIND * FOG_LAYER_CLOUDINESS * 2000.0;

bool			fogHasBBox							= (FOG_LAYER_BBOX[0] == 0.0 && FOG_LAYER_BBOX[1] == 0.0 && FOG_LAYER_BBOX[2] == 0.0 && FOG_LAYER_BBOX[3] == 0.0) ? false : true;

vec3			vAA									= vec3( fogHasBBox ? FOG_LAYER_BBOX[0] : -524288.0, fogBottom, fogHasBBox ? FOG_LAYER_BBOX[1] : -524288.0 );
vec3			vBB									= vec3( fogHasBBox ? FOG_LAYER_BBOX[2] :  524288.0, fogHeight, fogHasBBox ? FOG_LAYER_BBOX[3] :  524288.0 );

#define			eyePos								u_ViewOrigin.xzy

vec3			sundir							= -normalize(u_ViewOrigin.xzy - u_PrimaryLightOrigin.xzy);
const float		sunDiffuseStrength					= float(6.0);
const float		sunSpecularExponent					= float(100.0);


const mat3 nm = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 ) * 2.02;

struct Ray {
	vec3 Origin;
	vec3 Dir;
};

struct AABB {
	vec3 Min;
	vec3 Max;
};

bool IntersectBox(in Ray r, in AABB aabb, out float t0, out float t1)
{
	vec3 invR = 1.0 / r.Dir;
	vec3 tbot = invR * (aabb.Min - r.Origin);
	vec3 ttop = invR * (aabb.Max - r.Origin);
	vec3 tmin = min(ttop, tbot);
	vec3 tmax = max(ttop, tbot);
	vec2 t = max(tmin.xx, tmin.yz);
	t0 = max(0.,max(t.x, t.y));
	t  = min(tmax.xx, tmax.yz);
	t1 = min(t.x, t.y);
	//return (t0 <= t1) && (t1 >= 0.);
	return (abs(t0) <= t1);
}

float hash( float n ) {
	return fract(sin(n)*43758.5453);
}

float fnoise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}

float noise(in vec3 x, bool isFullWorldFog)
{
	return fnoise( x / (((isFullWorldFog) ? worldFogCloudiness : FOG_LAYER_CLOUDINESS) * 1000.0) );
}

float MapClouds(in vec3 p, bool isFullWorldFog)
{
	float factor = 1.0 - smoothstep(((isFullWorldFog) ? woldFogFadeAltitude : fadeAltitude), ((isFullWorldFog) ? worldFogHeight : fogHeight), p.y);

	vec3 pos = p;//normalize(p);

	float wind = ((isFullWorldFog) ? worldFogWindTime : fogWindTime);
	vec3 windFactor = vec3(-0.025, 1.0, 1.0) * wind; // x is for slower vertical - because its using 2d > 3d conversion.

	pos += windFactor * 0.07;

	float f = 0.5 * noise( pos, isFullWorldFog );
	pos = m*pos - windFactor * 0.3;
	f += 0.25 * noise( pos, isFullWorldFog );
	pos = m*pos - windFactor * 0.07;
	f += 0.1250 * noise( pos, isFullWorldFog );
	pos = m*pos + windFactor * 0.8;
	f += 0.0625 * noise( pos, isFullWorldFog );

    f = mix(0.0, f, factor);

	return f;
}

vec4 RaymarchClouds(in vec3 start, in vec3 end, bool isFullWorldFog)
{
	float l = length(end - start);
	const float numsteps = FOG_VOLUMETRIC_QUALITY;//20.0;
	//const float tstep = 1.0;
	const float tstep = 1. / numsteps;
	float depth = min(l * ((isFullWorldFog) ? worldFogThicknessInv : fogThicknessInv), 1.5);

	float fogContrib = 0.;
	float sunContrib = 0.;
	float alpha = 0.;

	//for (float t=0.0; t<=1.0; t+=tstep)
	for (float t=tstep/*0.0*/; t<=1.0; t+=tstep)
	//float t = 1.0;
	{
		//if (t == 0.0) continue; // Adds nothing, may as well skip... Why doesnt setting the commented out lines above do the same thing?

		vec3  pos = mix(start, end, t);
		float fog = MapClouds(pos, isFullWorldFog);
		fogContrib += fog;

		vec3  lightPos = sundir * ((isFullWorldFog) ? 1.0 : FOG_LAYER_SUN_PENETRATION) + pos;
		float lightFog = MapClouds(lightPos, isFullWorldFog);
		float sunVisibility = clamp((fog - lightFog), 0.0, 1.0 ) * sunDiffuseStrength;
		sunContrib += sunVisibility;

		float b = smoothstep(1.0, 0.7, abs((t - 0.5) * 2.0));
		alpha += b;
	}

	//fogContrib = pow(fogContrib, u_Local0.r); // Per fog contrasts maybe?

	float	fogAlpha = ((isFullWorldFog) ? worldFogAlpha : FOG_LAYER_ALPHA);
	vec3	fogColor = ((isFullWorldFog) ? worldFogColor : FOG_LAYER_COLOR);
	vec3	fogSunColor = ((isFullWorldFog) ? worldFogSunColor : FOG_LAYER_SUN_COLOR);

	fogContrib *= tstep;
	sunContrib *= tstep;
	alpha      *= tstep * fogAlpha * depth;

	vec3 ndir = (end - start) / l;
	float sun = pow( clamp( dot((FOG_LAYER_INVERT && isFullWorldFog) ? -sundir : sundir, ndir), 0.0, 1.0 ), sunSpecularExponent );
	sunContrib += sun * clamp(1. - fogContrib * alpha, 0.2, 1.) * 1.0;

	vec4 col;
	col.rgb = sunContrib * fogSunColor + fogColor;
	col.a   = fogContrib * clamp(pow(alpha, 0.001), 0.0, fogAlpha);
	return col;
}

//
// Shared...
//
void main ( void )
{
	vec3 col = textureLod(u_DiffuseMap, var_TexCoords, 0.0).rgb;
	vec3 fogColor = col;

	if (/*u_Local7.r < 1.0 &&*/ (u_Local2.a > 0.0 || u_Local4.g > 0.0 || u_Local11.a > 0.0))
	{
		vec4 pMap = positionMapAtCoord( var_TexCoords );
		vec3 viewOrg = u_ViewOrigin.xyz;

		float fogNightColorScale = clamp((1.0 - u_Local7.r) + 0.25, 0.0, 1.0);

		//
		// Linear fog...
		//
		if (u_Local11.a > 0.0)
		{
			float depth = textureLod(u_ScreenDepthMap, var_TexCoords, 0.0).r;
			//depth = clamp(depth * 1.333, 0.0, 1.0);
			depth = clamp(pow(depth, u_MapInfo.a), 0.0, 1.0);
			fogColor = mix(fogColor, clamp(u_Local12.rgb, 0.0, 1.0) * fogNightColorScale, u_Local12.a * depth);
		}

		//
		// World fog...
		//
		if (u_Local2.a > 0.0)
		{
			vec3 worldPos = pMap.xzy;

			// clamp ray in boundary box
			Ray r;
			r.Origin = eyePos;
			r.Dir = worldPos - eyePos;

			AABB box;
			box.Min = worldvAA;
			box.Max = worldvBB;
			float t1, t2;
	
			if (IntersectBox(r, box, t1, t2)) 
			{
				t1 = clamp(t1, 0.0, 1.0);
				t2 = clamp(t2, 0.0, 1.0);
				vec3 startPos = r.Dir * t1 + r.Origin;
				vec3 endPos   = r.Dir * t2 + r.Origin;

				// finally raymarch the volume
				vec4 vFog = RaymarchClouds(startPos, endPos, true);

				// blend with distance to make endless fog have smooth horizon
				//fogColor = mix(fogColor, clamp(fogColor + vFog.rgb, 0.0, 1.0), smoothstep(gl_Fog.end * 10.0, gl_Fog.start, length(worldPos - eyePos)));
				fogColor = mix(fogColor, clamp(fogColor + vFog.rgb, 0.0, 1.0) * fogNightColorScale, vFog.a);
			}
		}

		//
		// Volumetric fog...
		//
		if (u_Local4.g > 0.0)
		{
			vec3 worldPos = pMap.xzy;

			// clamp ray in boundary box
			Ray r;
			r.Origin = eyePos;
			r.Dir = worldPos - eyePos;
			AABB box;
			box.Min = vAA;
			box.Max = vBB;
			float t1, t2;
	
			if (IntersectBox(r, box, t1, t2)) 
			{
				t1 = clamp(t1, 0.0, 1.0);
				t2 = clamp(t2, 0.0, 1.0);
				vec3 startPos = r.Dir * t1 + r.Origin;
				vec3 endPos   = r.Dir * t2 + r.Origin;

				// finally raymarch the volume
				vec4 vFog = RaymarchClouds(startPos, endPos, false);

				// blend with distance to make endless fog have smooth horizon
				//fogColor = mix(fogColor, clamp(fogColor + vFog.rgb, 0.0, 1.0), smoothstep(gl_Fog.end * 10.0, gl_Fog.start, length(worldPos - eyePos)));
				fogColor = mix(fogColor, clamp(fogColor + vFog.rgb, 0.0, 1.0) * fogNightColorScale, vFog.a);
			}
		}

		// Blend out fog as we head more to night time... For now... Sky doesn't like it much at night transition (sun angles, etc)...
		//fogColor.rgb = mix(clamp(fogColor.rgb, 0.0, 1.0), col.rgb, u_Local7.r);
	}

	gl_FragColor = vec4(fogColor, 1.0);
}
