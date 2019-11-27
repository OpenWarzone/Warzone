#define __PER_PIXEL_NORMAL__
//#define __USING_GEOM_SHADER__

invariant gl_Position;

attribute vec3	attr_InstancesPosition;
attribute vec2	attr_InstancesTexCoord;

uniform mat4	u_ModelViewProjectionMatrix;

uniform vec4	u_Local0; // MAP_WATER_LEVEL, 0, 0, 0
uniform vec4	u_Local9;
uniform vec4	u_Local10;

uniform float	u_Time;

uniform vec3	u_ViewOrigin;

#define numWaves 10

#define WAVE_AMPLITUDE u_Local9.r//1.0//0.01
#define WAVE_LENGTH u_Local9.g//1.0//0.01
#define WAVE_SPEED u_Local9.b//1.0//0.01
#define DAMPING 0.1
#define STEEPNESS 1.0

#define TIME (u_Time * WAVE_SPEED)
#define MAP_WATER_LEVEL u_Local0.r

struct Wave {
  float freq;  // 2*PI / wavelength
  float amp;   // amplitude
  float phase; // speed * 2*PI / wavelength
  vec2 dir;
};

out vec3 FragPos;
out vec3 Normal;

// calculate the gerstner wave
vec3 getGerstnerHeight(Wave w, vec2 pos, float time)
{
	float Q = STEEPNESS/(w.freq*w.amp*numWaves);
	//Q = 0.1f;
	vec3 gerstner = vec3(0.0, 0.0, 0.0);
	gerstner.x = Q*w.amp*w.dir.x*cos( dot(w.freq*w.dir, pos) + w.phase * time);
	gerstner.z = Q*w.amp*w.dir.y*cos( dot(w.freq*w.dir, pos) + w.phase * time);
	gerstner.y = w.amp*sin( dot( w.freq*w.dir, pos) + w.phase * time);

	return gerstner;
}

#ifndef __PER_PIXEL_NORMAL__
vec3 computePartialBinormal(Wave w, vec3 P, float time)
{
	vec3 B = vec3(0.0, 0.0, 0.0);
	vec2 p = vec2(P.x, P.z);
	float inner = w.freq * dot(w.dir, p) + w.phase * time;
	float WA = w.freq * w.amp;
	float Q = STEEPNESS/(w.freq*w.amp*numWaves);
	//Q = 0.1f;
	B.x = Q * pow(w.dir.x, 2) * WA * sin(inner);
	B.z = Q * w.dir.x * w.dir.y * WA * sin(inner);
	B.y = w.dir.x * WA * cos(inner);

	return B;
}

vec3 computePartialTangent(Wave w, vec3 P, float time)
{
	vec3 T = vec3(0.0, 0.0, 0.0);
	vec2 p = vec2(P.x, P.z);
	float inner = w.freq * dot(w.dir, p) + w.phase * time;
	float WA = w.freq * w.amp;
	float Q = STEEPNESS/(w.freq*w.amp*numWaves);
	//Q = 0.1f;
	T.x = Q * w.dir.x * w.dir.y * WA * sin(inner);
	T.z = Q * pow(w.dir.y, 2) * WA * sin(inner);
	T.y = w.dir.y * WA * cos(inner);

	return T;
}

vec3 computePartialGerstnerNormal(Wave w, vec3 P, float time)
{
	vec3 N = vec3(0.0, 0.0, 0.0);
	vec2 p = vec2(P.x, P.z);
	float inner = w.freq*dot(w.dir, p) + w.phase * time;
	float WA = w.freq * w.amp;
	float Q = STEEPNESS/(w.freq*w.amp*numWaves);
	//Q = 0.1f;
	N.x = w.dir.x * WA * cos(inner);
	N.z = w.dir.y * WA * cos(inner);
	N.y = Q * WA * sin(inner);
		
	return N;
}
#endif //__PER_PIXEL_NORMAL__

void main()
{
	vec3 position = attr_InstancesPosition.xyz;
	position.y = MAP_WATER_LEVEL;

	vec2 p = vec2(position.x, position.z);

	float d = 0.0f;
	vec3 gerstnerTot = vec3(0.0, 0.0, 0.0);

	Wave W[10] = Wave[]
	(
		Wave( 0.050, 0.20, 1.5, vec2(-5.5, 2.0) ),
		Wave( 0.120, 0.55, 1.3, vec2(-0.7, 0.7) ),
		Wave( 0.2020, 0.01, 0.5, vec2(-1, 0) ),
		Wave( 0.120, 1.15, 1.60, vec2(1.0, 0.20) ),
		Wave( 0.020, 10.25, 1.32, vec2(1.2, 0.10) ),	
		Wave( 0.020, 0.056, 1.53, vec2(1.4, 0.50) ),	
		Wave( 0.120, -0.35, 3.60, vec2(0.50, 2.20) ),	
		Wave( 0.120, -0.05, 0.60, vec2(2.50, -2.20) ),	
		Wave( 0.050, -0.42, 1.60, vec2(-4.0, -1.20) ),	
		Wave( 0.50, 0.01, 2.330, vec2(-2.0, -1.60) )
	);

	// compute the position
	for(int i = 0; i < numWaves; i++)
	{
		W[i].freq *= WAVE_LENGTH;
		W[i].amp *= WAVE_AMPLITUDE;
		W[i].phase *= u_Local9.a;
		W[i].dir *= u_Local10.a;
		gerstnerTot += getGerstnerHeight(W[i], p, TIME); 
	}

	//gerstnerTot/=4;
	position.x += gerstnerTot.x;
	position.z += gerstnerTot.z;
	position.y += gerstnerTot.y;

#ifndef __PER_PIXEL_NORMAL__
#if 0
	vec3 T = vec3(0.0, 0.0, 0.0);
	for(int i = 0; i < numWaves; i++)
	{
		T += computePartialTangent(W[i], position/*gerstnerTot*/, TIME);
	}
	T.x *= -1.0;
	T.z = 1.0-T.z;
			
	vec3 B = vec3(0.0, 0.0, 0.0);
	for(int i = 0; i < numWaves; i++)
	{
		B += computePartialBinormal(W[i], position/*gerstnerTot*/, TIME);
	}
	B.x = 1.0 - B.x;
	B.z *= -1.0;

	vec3 N = cross(T, B);
#else			
	vec3 N = vec3(0.0, 0.0, 0.0);
	for(int i = 0; i < numWaves; i++)
	{
		N += computePartialGerstnerNormal(W[i], position/*gerstnerTot*/, TIME);
	}
	N.x *= -1.0;
	N.z *= -1.0;
	N.y = 1.0 - N.y;
#endif

	Normal = normalize(N).xzy;
	//Normal.y *= -1.0;
#endif //__PER_PIXEL_NORMAL__

	FragPos = position.xzy;

#ifdef __USING_GEOM_SHADER__
	gl_Position = vec4(position.xzy, 1.0);
#else //!__USING_GEOM_SHADER__
	gl_Position = u_ModelViewProjectionMatrix * vec4(position.xzy, 1.0);
#endif //__USING_GEOM_SHADER__
}
