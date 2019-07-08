#define __CLOUD_SHADOWS__

attribute vec3	attr_Position;
attribute vec2	attr_TexCoord0;

uniform mat4	u_ModelViewProjectionMatrix;

uniform float	u_Time;

uniform vec4	u_PrimaryLightOrigin;

uniform vec4								u_Local8; // NIGHT_SCALE,		CLOUDS_CLOUDCOVER,		CLOUDS_CLOUDSCALE,		CLOUDS_SHADOWS_ENABLED


// CLOUDS
#define NIGHT_SCALE							u_Local8.r
#define CLOUDS_CLOUDCOVER					u_Local8.g
#define CLOUDS_CLOUDSCALE					u_Local8.b
#define CLOUDS_SHADOWS_ENABLED				u_Local8.a


varying vec2   	var_TexCoords;

#ifdef __CLOUD_SHADOWS__

varying float	var_CloudShadow;

#define RAY_TRACE_STEPS 2 //55

//float gTime;
float cloudy = 0.0;
float cloudShadeFactor = 0.6;

#define CLOUD_LOWER 2800.0
#define CLOUD_UPPER 6800.0

#define MOD2 vec2(.16632,.17369)
#define MOD3 vec3(.16532,.17369,.15787)


//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
float Hash( float p )
{
	vec2 p2 = fract(vec2(p) * MOD2);
	p2 += dot(p2.yx, p2.xy+19.19);
	return fract(p2.x * p2.y);
}
float Hash(vec3 p)
{
	p  = fract(p * MOD3);
	p += dot(p.xyz, p.yzx + 19.19);
	return fract(p.x * p.y * p.z);
}

//--------------------------------------------------------------------------

float Noise( in vec2 x )
{
	vec2 p = floor(x);
	vec2 f = fract(x);
	f = f*f*(3.0-2.0*f);
	float n = p.x + p.y*57.0;
	float res = mix(mix( Hash(n+  0.0), Hash(n+  1.0),f.x),
					mix( Hash(n+ 57.0), Hash(n+ 58.0),f.x),f.y);
	return res;
}
float Noise(in vec3 p)
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


const mat3 cm = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 ) * 1.7;
//--------------------------------------------------------------------------
float FBM( vec3 p )
{
	p *= .0005;
	p *= CLOUDS_CLOUDSCALE;

	float f;
	
	f = 0.5000 * Noise(p); p = cm*p; //p.y -= gTime*.2;
	f += 0.2500 * Noise(p); p = cm*p; //p.y += gTime*.06;
	f += 0.1250 * Noise(p); p = cm*p;
	f += 0.0625   * Noise(p); p = cm*p;
	f += 0.03125  * Noise(p); p = cm*p;
	f += 0.015625 * Noise(p);
	return f;
}

//--------------------------------------------------------------------------

float Map(vec3 p)
{
	//float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-.6);
	//float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-pow(cloudy, 0.3));
	float h = -(FBM((p*vec3(0.3, 3.0, 0.3))+(u_Time*128.0))-cloudy-cloudShadeFactor);
	return h;
}

//--------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
float GetCloudAlpha(in vec3 pos,in vec3 rd, out vec2 outPos)
{
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
	float shadeSum = 0.0;
	
	// I think this is as small as the loop can be
	// for a reasonable cloud density illusion.
	for (int i = 0; i < RAY_TRACE_STEPS; i++)
	{
		if (shadeSum >= 1.0) break;

		float h = clamp(Map(p)*2.0, 0.0, 1.0);
		shadeSum += max(h, 0.0) * (1.0 - shadeSum);
		p += add;
	}

	return clamp(shadeSum, 0.0, 1.0);
}

float CloudShadows(void)
{
	cloudy = clamp(CLOUDS_CLOUDCOVER*0.3, 0.0, 0.3);
	cloudShadeFactor = 0.5+(cloudy*0.333);
	
	vec3 cameraPos = vec3(0.0);
    vec3 dir = normalize(-u_PrimaryLightOrigin.xzy);
	dir = normalize(vec3(dir.x, -1.0, dir.z));

	vec2 pos;
	float alpha = GetCloudAlpha(cameraPos, dir, pos);

	return (1.0 - (alpha*0.75));
}
#endif //__CLOUD_SHADOWS__

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;

#ifdef __CLOUD_SHADOWS__
	if (CLOUDS_SHADOWS_ENABLED == 1.0)
	{
		float cShadow = CloudShadows() * 0.5 + 0.5;
		var_CloudShadow = mix(cShadow, 1.0, clamp(NIGHT_SCALE, 0.0, 1.0)); // Dampen out cloud shadows at sunrise/sunset...
	}
#endif //__CLOUD_SHADOWS__
}
