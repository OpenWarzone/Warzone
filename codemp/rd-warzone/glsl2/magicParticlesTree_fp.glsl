uniform sampler2D u_DiffuseMap;

uniform vec2	u_Dimensions;

uniform vec4	u_Local6; // PARTICLE_COLOR
uniform vec4	u_Local7; // FIREFLY_COLOR
uniform vec4	u_Local8; // 
uniform vec4	u_Local9; // tests

uniform float	u_Time;

varying vec3	var_Normal;
varying vec3	var_vertPos;
varying vec2    var_TexCoords;

#define PARTICLE_COLOR	u_Local6.rgb//vec3(1.0, 1.0, 0.0)
#define FIREFLY_COLOR	u_Local7.rgb//vec3(.94, .94, .14)
#define NUM_FIREFLIES	u_Local7.a//30 //20

out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4 out_NormalDetail;
#endif //USE_REAL_NORMALMAPS


#define m_Normal		var_Normal
#define m_TexCoords		var_TexCoords
#define m_vertPos		var_vertPos
#define m_ViewDir		var_ViewDir


#define iTime (u_Time * 0.4)

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
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

	vec3 x1 = x0 - i1 + C.xxx;
	vec3 x2 = x0 - i2 + C.yyy;
	vec3 x3 = x0 - D.yyy;

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

float ball(vec2 p, float fx, float fy, float ax, float ay)
{
	float t = (iTime * 0.5) + (length(var_vertPos.xyz) * 0.01);
	vec2 r = vec2(p.x + cos(t * fx) * ax, p.y + sin(t * fy) * ay);
	r *= 6.0;
	return clamp((0.05 / length(r)) - 0.1, 0.0, 1.0);
}

void getBalls(out vec4 fragColor, in vec2 fragCoord) {
	vec2 q = fragCoord.xy;
	vec2 p = -3.0 + 6.0 * q;
	p.x *= u_Dimensions.x / u_Dimensions.y;

	float col = 0.0;
	float a = 1.;
	float b = 2.;
	float c = 0.2;
	float d = 0.8;

	for (int i = 0; i < int(NUM_FIREFLIES); i++)
	{
		col += ball(p, a, b, c, d);
		a += 0.01;
		b = tan(a * 0.5);
		c += 0.1;
		d = tan(c * 0.5);
	}

	col = pow(clamp(col - 0.5, 0.0, 1.0), 0.5);

	vec3 finalColor = vec3(col * FIREFLY_COLOR) * 3.0;

	fragColor = vec4(clamp(finalColor, 0.0, 3.0), clamp(length(finalColor), 0.0, 1.4));
}

void getParticles( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy;
    
    float fadeLR = .5 - abs(uv.x - .5) ;
    float fadeTB = 1.-uv.y;
    vec3 pos = vec3( uv * vec2( 3. , 1.) - vec2( 0. , iTime * .03 ) , iTime * .01 );
   
    float n = fadeLR * fadeTB * smoothstep(.6, 1. ,snoise( pos * 60. )) * 10.;

	fragColor =  vec4( clamp(PARTICLE_COLOR * n * 1.1, 0.0, 1.5), (n > 1.5) ? pow(clamp(n, 0.0, 1.0), 4.0) : 0.0 );
}

void main()
{
	vec4 particleColor;
	vec4 ballsColor;
	getParticles(particleColor, vec2(1.0)-m_TexCoords );
	getBalls(ballsColor, vec2(1.0) - m_TexCoords);

	gl_FragColor = mix(particleColor, ballsColor, clamp(ballsColor.a, 0.0, 1.0));

	out_Glow = vec4(gl_FragColor) * 1.5;
	out_Glow.a = mix(particleColor.a * 1.5, ballsColor.a * 4.0, clamp(ballsColor.a, 0.0, 1.0));
	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
	out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
}
