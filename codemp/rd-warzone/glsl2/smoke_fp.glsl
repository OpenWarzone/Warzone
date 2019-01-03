uniform sampler2D u_DiffuseMap;

uniform vec2	u_Dimensions;

uniform vec4	u_Local6; // SMOKE_COLOR - TODO?
uniform vec4	u_Local7; // FLAME_COLOR_MAIN - TODO?
uniform vec4	u_Local8; // FLAME_COLOR_SECONDARY - TODO?
uniform vec4	u_Local9; // tests

uniform float	u_Time;

varying vec3	var_Normal;
varying vec3	var_vertPos;
varying vec2    var_TexCoords;


out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef __USE_REAL_NORMALMAPS__
out vec4 out_NormalDetail;
#endif //__USE_REAL_NORMALMAPS__


#define m_Normal		var_Normal
#define m_TexCoords		var_TexCoords
#define m_vertPos		var_vertPos
#define m_ViewDir		var_ViewDir


#define shaderTime (u_Time * 0.4)


#if 0
vec3 mod289(vec3 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
	return mod289(((x*34.0) + 1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
{
	const vec2	C = vec2(1.0 / 6.0, 1.0 / 3.0);
	const vec4	D = vec4(0.0, 0.5, 1.0, 2.0);

	// First corner
	vec3 i = floor(v + dot(v, C.yyy));
	vec3 x0 = v - i + dot(i, C.xxx);

	// Other corners
	vec3 g = step(x0.yzx, x0.xyz);
	vec3 l = 1.0 - g;
	vec3 i1 = min(g.xyz, l.zxy);
	vec3 i2 = max(g.xyz, l.zxy);

	//	 x0 = x0 - 0.0 + 0.0 * C.xxx;
	//	 x1 = x0 - i1	+ 1.0 * C.xxx;
	//	 x2 = x0 - i2	+ 2.0 * C.xxx;
	//	 x3 = x0 - 1.0 + 3.0 * C.xxx;
	vec3 x1 = x0 - i1 + C.xxx;
	vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
	vec3 x3 = x0 - D.yyy;			// -1.0+3.0*C.x = -0.5 = -D.y

									// Permutations
	i = mod289(i);
	vec4 p = permute(permute(permute(
		i.z + vec4(0.0, i1.z, i2.z, 1.0))
		+ i.y + vec4(0.0, i1.y, i2.y, 1.0))
		+ i.x + vec4(0.0, i1.x, i2.x, 1.0));

	// Gradients: 7x7 points over a square, mapped onto an octahedron.
	// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
	float n_ = 0.142857142857; // 1.0/7.0
	vec3	ns = n_ * D.wyz - D.xzx;

	vec4 j = p - 49.0 * floor(p * ns.z * ns.z);	//	mod(p,7*7)

	vec4 x_ = floor(j * ns.z);
	vec4 y_ = floor(j - 7.0 * x_);		// mod(j,N)

	vec4 x = x_ *ns.x + ns.yyyy;
	vec4 y = y_ *ns.x + ns.yyyy;
	vec4 h = 1.0 - abs(x) - abs(y);

	vec4 b0 = vec4(x.xy, y.xy);
	vec4 b1 = vec4(x.zw, y.zw);

	//vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
	//vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
	vec4 s0 = floor(b0)*2.0 + 1.0;
	vec4 s1 = floor(b1)*2.0 + 1.0;
	vec4 sh = -step(h, vec4(0.0));

	vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy;
	vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww;

	vec3 p0 = vec3(a0.xy, h.x);
	vec3 p1 = vec3(a0.zw, h.y);
	vec3 p2 = vec3(a1.xy, h.z);
	vec3 p3 = vec3(a1.zw, h.w);

	//Normalise gradients
	//vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
	vec4 norm = inversesqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;

	// Mix final noise value
	vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
	m = m * m;
	return 42.0 * dot(m*m, vec4(dot(p0, x0), dot(p1, x1),
		dot(p2, x2), dot(p3, x3)));
}

//////////////////////////////////////////////////////////////

// PRNG
// From https://www.shadertoy.com/view/4djSRW
float prng(in vec2 seed) {
	seed = fract(seed * vec2(5.3983, 5.4427));
	seed += dot(seed.yx, seed.xy + vec2(21.5351, 14.3137));
	return fract(seed.x * seed.y * 95.4337);
}

//////////////////////////////////////////////////////////////

float PI = 3.1415926535897932384626433832795;

float noiseStack(vec3 pos, int octaves, float falloff) {
	float noise = snoise(vec3(pos));
	float off = 1.0;
	if (octaves>1) {
		pos *= 2.0;
		off *= falloff;
		noise = (1.0 - off)*noise + off*snoise(vec3(pos));
	}
	if (octaves>2) {
		pos *= 2.0;
		off *= falloff;
		noise = (1.0 - off)*noise + off*snoise(vec3(pos));
	}
	if (octaves>3) {
		pos *= 2.0;
		off *= falloff;
		noise = (1.0 - off)*noise + off*snoise(vec3(pos));
	}
	return (1.0 + noise) / 2.0;
}

vec2 noiseStackUV(vec3 pos, int octaves, float falloff, float diff) {
	float displaceA = noiseStack(pos, octaves, falloff);
	float displaceB = noiseStack(pos + vec3(3984.293, 423.21, 5235.19), octaves, falloff);
	return vec2(displaceA, displaceB);
}

void getFlames(out vec4 fragColor, in vec2 fragCoord, out vec4 glow) {
	//fragColor = vec4(0.0, 0.0, fragCoord.y / u_Dimensions.y, 1.0);
	//return;

	float time = u_Time;
	vec2 resolution = u_Dimensions.xy;
	vec2 drag = vec2(0.5);
	vec2 offset = vec2(0.5);
	//
	float xpart = fragCoord.x / resolution.x;
	float ypart = fragCoord.y / resolution.y;
	//
	float clip = resolution.y;//310.0;
	float ypartClip = fragCoord.y / clip;
	float ypartClippedFalloff = clamp(2.0 - ypartClip, 0.0, 1.0);
	float ypartClipped = min(ypartClip, 1.0);
	float ypartClippedn = 1.0 - ypartClipped;
	//
	float xfuel = 1.0 - abs(2.0*xpart - 1.0);//pow(1.0-abs(2.0*xpart-1.0),0.5);
											 //
	float timeSpeed = 0.5;
	float realTime = timeSpeed*time;
	//
	vec2 coordScaled = 0.02*fragCoord - 0.2*vec2(offset.x, 0.0);
	vec3 position = vec3(coordScaled, 0.0) + vec3(1223.0, 6434.0, 8425.0);
	vec3 flow = vec3(4.1*(0.5 - xpart)*pow(ypartClippedn, 4.0), -2.0*xfuel*pow(ypartClippedn, 64.0), 0.0);
	vec3 timing = realTime*vec3(0.0, -1.7, 1.1) + flow;
	//
	vec3 displacePos = vec3(1.0, 0.5, 1.0)*3.4*position + realTime*vec3(0.01, -0.7, 1.3);
	vec3 displace3 = vec3(noiseStackUV(displacePos, 2, 0.4, 0.1), 0.0);
	//
	vec3 noiseCoord = (vec3(2.0, 1.0, 1.0)*position + timing + 0.4*displace3) / 1.0;
	float noise = noiseStack(noiseCoord, 3, 0.4);
	//
	float flames = pow(ypartClipped, 0.5*xfuel)*pow(noise, 0.25*xfuel);
	//
	float f = ypartClippedFalloff*pow(1.0 - flames*flames*flames, 7.0);
	float fff = f*f*f;
	vec3 fire = 1.5*vec3(f, fff, fff*fff);
	fire *= clamp(1.0 - (pow(ypart, 0.2)), 0.0, 1.0);
	fire = clamp(fire, 0.0, 1.0);
	//
	// smoke
	float smokeNoise = 0.75 + snoise(0.4*position + timing*vec3(1.0, 1.0, 0.2)) / 2.0;
	vec3 smoke = vec3(0.53*pow(xfuel, 3.0)*pow(ypart, 0.015)*(smokeNoise + 0.4*(1.0 - noise)));
	smoke.rgb = vec3(clamp((length(smoke.rgb) / 3.0) * 2.0, 0.0, 1.0));
	if (ypart < 0.25) smoke *= ypart / 0.25;
	smoke *= clamp(1.0 - (pow(ypart, 0.8)), 0.0, 1.0);
	//
	// sparks
	float sparkGridSize = 30.0;
	vec2 sparkCoord = fragCoord - vec2(2.0*offset.x, 190.0*realTime);
	sparkCoord -= 30.0*noiseStackUV(0.01*vec3(sparkCoord, 30.0*time), 1, 0.4, 0.1);
	sparkCoord += 100.0*flow.xy;
	if (mod(sparkCoord.y / sparkGridSize, 2.0)<1.0) sparkCoord.x += 0.5*sparkGridSize;
	vec2 sparkGridIndex = vec2(floor(sparkCoord / sparkGridSize));
	float sparkRandom = prng(sparkGridIndex);
	float sparkLife = min(10.0*(1.0 - min((sparkGridIndex.y + (190.0*realTime / sparkGridSize)) / (24.0 - 20.0*sparkRandom), 1.0)), 1.0);
	vec3 sparks = vec3(0.0);
	if (sparkLife>0.0) {
		float sparkSize = xfuel*xfuel*sparkRandom*0.08;
		float sparkRadians = 999.0*sparkRandom*2.0*PI + 2.0*time;
		vec2 sparkCircular = vec2(sin(sparkRadians), cos(sparkRadians));
		vec2 sparkOffset = (0.5 - sparkSize)*sparkGridSize*sparkCircular;
		vec2 sparkModulus = mod(sparkCoord + sparkOffset, sparkGridSize) - 0.5*vec2(sparkGridSize);
		float sparkLength = length(sparkModulus);
		float sparksGray = max(0.0, 1.0 - sparkLength / (sparkSize*sparkGridSize));
		sparks = sparkLife*sparksGray*vec3(1.0, 0.3, 0.0);
		sparks *= 1.0 - ypart;
		sparks = clamp(sparks * 8.0, 0.0, 1.0);
	}
	//
	fragColor = vec4(max(fire, sparks) + smoke, 1.0);
	fragColor.a = clamp(length(fragColor.rgb / 3.0), 0.0, 1.0);
	fragColor.a = clamp(fragColor.a * 4.0, 0.0, 1.0);

	glow.rgba = vec4(max(fire, sparks), 1.0);
	glow.a = clamp(length(glow.rgb / 3.0), 0.0, 1.0);
	glow.a = clamp(glow.a * 4.0, 0.0, 1.0);
}
#else
// Taken from https://www.shadertoy.com/view/4ts3z2
float tri(in float x) { return abs(fract(x) - .5); }
vec3 tri3(in vec3 p) { return vec3(tri(p.z + tri(p.y*1.)), tri(p.z + tri(p.x*1.)), tri(p.y + tri(p.x*1.))); }


// Taken from https://www.shadertoy.com/view/4ts3z2
float triNoise3D(in vec3 p, in float spd)
{
	float z = 1.4;
	float rz = 0.;
	vec3 bp = p;
	for (float i = 0.; i <= 3.; i++)
	{
		vec3 dg = tri3(bp*2.);
		p += (dg + shaderTime*.1*spd);

		bp *= 1.8;
		z *= 1.5;
		p *= 1.2;
		//p.xz*= m2;

		rz += (tri(p.z + tri(p.x + tri(p.y)))) / z;
		bp += 0.14;
	}
	return rz;
}


vec3 hsv(float h, float s, float v)
{
	return mix(vec3(1.0), clamp((abs(fract(
		h + vec3(3.0, 2.0, 1.0) / 3.0) * 6.0 - 3.0) - 1.0), 0.0, 1.0), s) * v;
}

vec2 hash(vec2 p)
{
	p = vec2(dot(p, vec2(127.1, 311.7)),
		dot(p, vec2(269.5, 183.3)));

	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float level = 1.;
float noise(in vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);

	vec2 u = f*f*(3.0 - 2.0*f);
	float t = pow(2., level)* .2*shaderTime;
	mat2 R = mat2(cos(t), -sin(t), sin(t), cos(t));
	if (mod(i.x + i.y, 2.) == 0.) R = -R;

	return 2.*mix(mix(dot(hash(i + vec2(0, 0)), (f - vec2(0, 0))*R),
		dot(hash(i + vec2(1, 0)), -(f - vec2(1, 0))*R), u.x),
		mix(dot(hash(i + vec2(0, 1)), -(f - vec2(0, 1))*R),
			dot(hash(i + vec2(1, 1)), (f - vec2(1, 1))*R), u.x), u.y);
}

float Mnoise(in vec2 uv) {
	//return noise(uv);                      // base turbulence
	//return -1. + 2.* (1.-abs(noise(uv)));  // flame like
	return -1. + 2.* (abs(noise(uv)));     // cloud like
}

float turb(in vec2 uv)
{
	float f = 0.0;

	level = 1.;
	mat2 m = mat2(1.6, 2.2, -1.2, 1.6);
	f = 0.5000*Mnoise(uv); uv = m*uv;
	level++;
	f += 0.2000*Mnoise(uv); uv = m*uv;
	level++;
	f += 0.0850*Mnoise(uv); uv = m*uv;
	level++;
	f += 0.0825*Mnoise(uv); uv = m*uv;
	level++;
	f += 0.0825*Mnoise(uv); uv = m*uv; level++;
	return f / .9375;
}
// -----------------------------------------------

//#define CYLINDERS

void getSmoke(out vec4 fragColor, in vec2 uv)
{
	float dist = (1.0 - distance(0.0, uv.y));

#ifndef CYLINDERS
	dist = pow(dist, 2.0);
	float fadeLR = (0.5 - abs(uv.x - 0.5)) * 2.0;
#else //CYLINDERS
	dist = pow(dist, 4.0);
	float fadeLR = 1.0;
#endif //CYLINDERS

	//
	// Smoke...
	//
	vec2 tuv = uv;

	float line = (0.5 - distance(0.5, tuv.x)) * 2.0;
	float glow = (0.5 - distance(0.5, tuv.y)) * 2.0;

	float sm = smoothstep(0.0, 1.0, line*0.5);
	float sg = smoothstep(0.0, 1.0, glow*0.5);

	tuv.y -= shaderTime*0.03;
	float f = turb(5.*tuv);
	float tout = .5 + .4* f;
	float fout = smoothstep(0.0, 0.05, sm*tout);
	float gout = smoothstep(0.0, 0.1, sg*tout);

#ifndef CYLINDERS
	vec4 smoke = (vec4(fout*tout*0.8 + gout*tout*0.1 + fout*0.6) * (exp(-uv.y*3.0)) * (1.0 - (distance(0.5, uv.x) * 2.0))) * 32.0;
#else //CYLINDERS
	vec4 smoke = (vec4(fout*tout*1.8 + gout*tout*1.5 + fout*0.6) * (exp(-uv.y*3.0)) * (1.0 - dist)) * 32.0;
#endif //CYLINDERS

	//
	// Combine...
	//
	fragColor = smoke;
	fragColor.a = clamp(length(fragColor.rgb / 3.0), 0.0, 1.0);
	fragColor.a = clamp(fragColor.a * 0.03, 0.0, 1.0);
}
#endif

void main()
{
	getSmoke(gl_FragColor, vec2(1.0) - m_TexCoords);
	out_Glow = vec4(0.0);
	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
}
