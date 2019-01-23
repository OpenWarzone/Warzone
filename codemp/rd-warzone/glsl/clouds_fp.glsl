#define USE_EXPERIMENTAL_CLOUDS

uniform vec4						u_Local2; // PROCEDURAL_CLOUDS_ENABLED, PROCEDURAL_CLOUDS_CLOUDSCALE, PROCEDURAL_CLOUDS_SPEED, PROCEDURAL_CLOUDS_DARK
uniform vec4						u_Local3; // PROCEDURAL_CLOUDS_LIGHT, PROCEDURAL_CLOUDS_CLOUDCOVER, PROCEDURAL_CLOUDS_CLOUDALPHA, PROCEDURAL_CLOUDS_SKYTINT
uniform vec4						u_Local5; // dayNightEnabled, nightScale, MAP_CLOUD_LAYER_HEIGHT, 0.0
uniform vec4						u_Local9; // testvalues

#define CLOUDS_ENABLED				u_Local2.r
#define CLOUDS_CLOUDSCALE			u_Local2.g
#define CLOUDS_SPEED				u_Local2.b
#define CLOUDS_DARK					u_Local2.a

#define CLOUDS_LIGHT				u_Local3.r
#define CLOUDS_CLOUDCOVER			u_Local3.g
#define CLOUDS_CLOUDALPHA			u_Local3.b
#define CLOUDS_SKYTINT				u_Local3.a

#define SHADER_DAY_NIGHT_ENABLED	u_Local5.r
#define SHADER_NIGHT_SCALE			u_Local5.g

uniform vec3						u_ViewOrigin;
uniform float						u_Time;

uniform vec4						u_PrimaryLightOrigin;
uniform vec3						u_PrimaryLightColor;


varying vec2						var_TexCoords;
varying vec3						var_vertPos;
varying vec3						var_Normal;


out vec4 out_Glow;
out vec4 out_Normal;
out vec4 out_Position;
#ifdef __USE_REAL_NORMALMAPS__
out vec4 out_NormalDetail;
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


#if defined(USE_EXPERIMENTAL_CLOUDS)

#define RAY_TRACE_STEPS 2 //55

vec3 sunLight  = normalize( u_PrimaryLightOrigin.xzy - u_ViewOrigin.xzy );
vec3 sunColour = u_PrimaryLightColor.rgb;

float gTime;
float cloudy = 0.0;
float cloudShadeFactor = 0.6;
float flash = 0.0;

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


const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 ) * 1.7;
//--------------------------------------------------------------------------
float FBM( vec3 p )
{
	p *= .0005;
	p *= CLOUDS_CLOUDSCALE;

	float f;
	
	f = 0.5000 * Noise(p); p = m*p; //p.y -= gTime*.2;
	f += 0.2500 * Noise(p); p = m*p; //p.y += gTime*.06;
	f += 0.1250 * Noise(p); p = m*p;
	f += 0.0625   * Noise(p); p = m*p;
	f += 0.03125  * Noise(p); p = m*p;
	f += 0.015625 * Noise(p);
	return f;
}
//--------------------------------------------------------------------------
float FBMSH( vec3 p )
{
	p *= .0005;
	p *= CLOUDS_CLOUDSCALE;

	float f;
	
	f = 0.5000 * Noise(p); p = m*p; //p.y -= gTime*.2;
	f += 0.2500 * Noise(p); p = m*p; //p.y += gTime*.06;
	f += 0.1250 * Noise(p); p = m*p;
	f += 0.0625   * Noise(p); p = m*p;
	return f;
}

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
	//shadeSum.x /= 10.0;
	//shadeSum = min(shadeSum, 1.0);
	
	//vec3 clouds = mix(vec3(pow(shadeSum.x, .6)), sunColour, (1.0-shadeSum.y)*.4);
    //vec3 clouds = vec3(shadeSum.x);
	
	//clouds += min((1.0-sqrt(shadeSum.y)) * pow(sunAmount, 4.0), 1.0) * 2.0;
   
    //clouds += vec3(flash) * (shadeSum.y+shadeSum.x+.2) * .5;

	//sky = mix(sky, min(clouds, 1.0), shadeSum.y);
	
	//return clamp(sky, 0.0, 1.0);

	float final = shadeSum.x;
	final += flash * (shadeSum.y+final+.2) * .5;

	return clamp(vec4(final, final, final, shadeSum.y), 0.0, 1.0);
}

//--------------------------------------------------------------------------
void main()
{
	gTime = u_Time*.5 + 75.5;
	cloudy = clamp(CLOUDS_CLOUDCOVER*0.3, 0.0, 0.3);
	cloudShadeFactor = 0.5+(cloudy*0.333);
    float lightning = 0.0;
    
    
	if (cloudy >= 0.285)
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
	
	
	vec3 cameraPos = vec3(0.0);
    vec3 dir = normalize(u_ViewOrigin.xzy - var_vertPos.xzy);

	vec4 col;
	vec2 pos;
	col = GetSky(cameraPos, dir, pos);

	col.rgb = clamp(col.rgb * 64.0, 0.0, 1.0);

	float l = exp(-length(pos) * .00002);
	col.rgb = mix(vec3(.6-cloudy*1.2)+flash*.3, col.rgb, max(l, .2));
	
	// Stretch RGB upwards... 
	col.rgb = pow(col.rgb, vec3(.7));
	
	col = clamp(col, 0.0, 1.0);

	float alpha = col.a;//max(col.r, max(col.g, col.b));
	alpha *= clamp(-dir.y, 0.0, 0.75);
	gl_FragColor = vec4(col.rgb, alpha);

	out_Glow = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
}

#else //!defined(USE_EXPERIMENTAL_CLOUDS)

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

vec4 Clouds(in vec2 fragCoord)
{
	vec2 p = fragCoord.xy;
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

	vec3 cloudcolour = /*vec3(1.1, 1.1, 0.9)*/vec3(1.0, 1.0, 1.0) * clamp((CLOUDS_DARK + CLOUDS_LIGHT*c), 0.0, 1.0);

	f = CLOUDS_CLOUDCOVER + CLOUDS_CLOUDALPHA*f*r;

	//vec3 result = mix(skycolour, clamp(CLOUDS_SKYTINT * skycolour + cloudcolour, 0.0, 1.0), clamp(f + c, 0.0, 1.0));
	vec3 result = cloudcolour;

	return vec4(result.rgb, clamp(f + c, 0.0, 1.0));
}


void main()
{
	vec3 pViewDir = normalize(u_ViewOrigin - var_vertPos.xyz);

	vec4 cloudColor = Clouds(pViewDir.xy * 0.5 + 0.5);
	cloudColor.a *= clamp(pow(-pViewDir.z, 1.75/*u_Local9.b*//*2.5*/), 0.0, 1.0);
	cloudColor.a *= 0.75;//u_Local9.g;

	if (SHADER_DAY_NIGHT_ENABLED > 0.0 && SHADER_NIGHT_SCALE > 0.0)
	{// Adjust cloud color at night...
		float nMult = clamp(1.25 - SHADER_NIGHT_SCALE, 0.0, 1.0);
		cloudColor *= nMult;
	}

	gl_FragColor = cloudColor;
	out_Glow = vec4(0.0);
	//out_Normal = vec4(EncodeNormal(var_Normal), 0.0, 1.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
	//out_Position = vec4(var_vertPos.xyz, SHADER_MATERIAL_TYPE+1.0);

	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
}

#endif //!defined(USE_EXPERIMENTAL_CLOUDS)
