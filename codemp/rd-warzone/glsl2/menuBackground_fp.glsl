uniform sampler2D		u_DiffuseMap;

uniform vec2			u_Dimensions;

uniform float			u_Time;

uniform vec4			u_Local9;

varying vec3			var_Normal;
varying vec3			var_vertPos;
varying vec2			var_TexCoords;

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


#define iResolution u_Dimensions.xy
#define iTime (u_Time * 0.4)
#define iTimeDelta 0.5


//#define __TIE_FIGHTERS__
//#define __HYPERSPACE__
//#define __HYPERSPACE2__

#define __GREEN_PLANET__
//#define __BESPIN__


#ifdef __GREEN_PLANET__
//Sirenian Dawn by nimitz (twitter: @stormoid)

#define ADD_WATER
//#define ADD_SUN
#define ADD_BEACHES
#define FASTER_RANDOM
#define WATER_LEVEL 2.3

#define ITR 30//90
#define FAR 400.
#define time iTime

#ifdef ADD_SUN
vec3 lgt;
#else //!ADD_SUN
const vec3 lgt = vec3(-.523, .41, -.747);
#endif //ADD_SUN

mat2 m2 = mat2( 0.80,  0.60, -0.60,  0.80 );

#define HASHSCALE1 .1031

float fnoise(vec2 pos)
{
	vec2 p = pos * 32768.0;
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

const mat2 m = mat2( 0.00,  0.80,
                    -0.80,  0.36 );

float noise( vec2 p )
{
    float f;
    f  = 0.5000*fnoise( p ); p = m*p*2.02;
    f += 0.2500*fnoise( p ); 
	
    return f * (1.0 / (0.5000 + 0.2500));
}

//form iq, see: http://www.iquilezles.org/www/articles/morenoise/morenoise.htm
vec3 noised( in vec2 x )
{
#ifndef FASTER_RANDOM
    vec2 p = floor(x);
    vec2 f = fract(x);
    vec2 u = f*f*(3.0-2.0*f);
	float a = noise((p+vec2(0.5,0.5))/256.0).x;
	float b = noise((p+vec2(1.5,0.5))/256.0).x;
	float c = noise((p+vec2(0.5,1.5))/256.0).x;
	float d = noise((p+vec2(1.5,1.5))/256.0).x;
	return vec3(a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y,
				6.0*f*(1.0-f)*(vec2(b-a,c-a)+(a-b-c+d)*u.yx));
#else
	vec2 p = floor(x);
    vec2 f = fract(x);
    vec2 u = f*f*(3.0-2.0*f);
	float a = fnoise((p+vec2(0.5,0.5))/256.0).x;
	float b = fnoise((p+vec2(1.5,0.5))/256.0).x;
	float c = fnoise((p+vec2(0.5,1.5))/256.0).x;
	float d = fnoise((p+vec2(1.5,1.5))/256.0).x;
	return vec3(a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y,
				6.0*f*(1.0-f)*(vec2(b-a,c-a)+(a-b-c+d)*u.yx)) * 0.5 + 0.333;
#endif
}

float terrain( in vec2 p)
{
    float rz = 0.1;
    float z = 1.2;
	vec2  d = vec2(0.1);
    float scl = 3.95;
    float zscl = -.4;
    float zz = 5.;
    for( int i=0; i<5; i++ )
    {
        vec3 n = noised(p);
        d += pow(abs(n.yz),vec2(zz));
        d -= smoothstep(-.5,1.5,n.yz);
        zz -= 1.;
        rz += z*n.x/(dot(d,d)+.85);
        z *= zscl;
        zscl *= .8;
        p = m2*p*scl;
    }
    
    rz /= smoothstep(1.5,-.5,rz)+.75;
    return rz;
}

float map(vec3 p)
{
    return p.y-(terrain(p.zx*0.07))*2.7-1.;
}

/*	The idea is simple, as the ray gets further from the eye, I increase 
	the step size of the raymarching and lower the target precision, 
	this allows for better performance with virtually no loss in visual quality. */
float march(in vec3 ro, in vec3 rd, out float itrc)
{
    float t = 0.;
    float d = map(rd*t+ro);
    float precis = 0.0001;
    for (int i=0;i<=ITR;i++)
    {
        if (abs(d) < precis || t > FAR) break;
        precis = t*0.0001;
        float rl = max(t*0.02,1.);
        t += d*rl;
        d = map(rd*t+ro)*0.7;
        itrc++;
    }

    return t;
}

vec3 rotx(vec3 p, float a){
    float s = sin(a), c = cos(a);
    return vec3(p.x, c*p.y - s*p.z, s*p.y + c*p.z);
}

vec3 roty(vec3 p, float a){
    float s = sin(a), c = cos(a);
    return vec3(c*p.x + s*p.z, p.y, -s*p.x + c*p.z);
}

vec3 rotz(vec3 p, float a){
    float s = sin(a), c = cos(a);
    return vec3(c*p.x - s*p.y, s*p.x + c*p.y, p.z);
}

vec3 normal(in vec3 p, in float ds)
{  
    vec2 e = vec2(-1., 1.)*0.005*pow(ds,1.);
	return normalize(e.yxx*map(p + e.yxx) + e.xxy*map(p + e.xxy) + 
					 e.xyx*map(p + e.xyx) + e.yyy*map(p + e.yyy) );   
}

float fbm(in vec2 p)
{	
	float z= .5;
	float rz = 0.;
	for (float i= 0.;i<3.;i++ )
	{
        rz+= (sin(noise(p)*5.)*0.5+0.5) *z;
		z *= 0.5;
		p = p*2.;
	}
	return rz;
}

float bnoise(in vec2 p){ return fbm(p*3.); }
vec3 bump(in vec3 p, in vec3 n, in float ds)
{
    vec2 e = vec2(0.005*ds,0);
    float n0 = bnoise(p.zx);
    vec3 d = vec3(bnoise(p.zx+e.xy)-n0, 1., bnoise(p.zx+e.yx)-n0)/e.x*0.025;
    d -= n*dot(n,d);
    n = normalize(n-d);
    return n;
}

float curv(in vec3 p, in float w)
{
    vec2 e = vec2(-1., 1.)*w;   
    float t1 = map(p + e.yxx), t2 = map(p + e.xxy);
    float t3 = map(p + e.xyx), t4 = map(p + e.yyy);
    return .15/e.y *(t1 + t2 + t3 + t4 - 4. * map(p));
}

#ifdef ADD_SUN
mat2 rot2D(float a) {
	return mat2(cos(a),sin(a),-sin(a),cos(a));	
}

void updateLightPos()
{
	vec3 sunPos = normalize(vec3(0.2, 0.3, 0.5));
	// Adjust for planetary rotation...
	mat2 sunRot1 = rot2D(0.01*iTime);
	mat2 sunRot2 = rot2D(0.01*iTime);
	sunPos.yz *= sunRot1;
	sunPos.xy *= sunRot2;
	sunPos = normalize(sunPos);
	lgt = sunPos;
}

vec3 sun(vec3 col, vec3 dir)
{
	vec3 sun = col;
	float sundot = clamp(dot(dir,lgt),0.0,1.0);

	// sun
	sun += 0.1*vec3(0.9, 0.3, 0.9)*pow(sundot, 0.5);
	sun += 0.2*vec3(1., 0.7, 0.7)*pow(sundot, 1.);
	sun += 0.95*vec3(1.)*pow(sundot, 256.);

	return sun;
}
#endif //ADD_SUN

//Based on: http://www.iquilezles.org/www/articles/fog/fog.htm
vec3 fog(vec3 ro, vec3 rd, vec3 col, float ds)
{
    vec3 pos = ro + rd*ds;
    float mx = (fbm(pos.zx*0.1-time*0.05)-0.5)*.02;
    
    const float b= 1.;
    float den = 0.1*exp(-ro.y*b)*(1.0-exp( -ds*rd.y*b ))/rd.y;

#ifdef ADD_WATER
	den *= 8.0;

	if (pos.y < WATER_LEVEL)
		den *= pos.y / WATER_LEVEL;
#endif //ADD_WATER

    float sdt = max(dot(rd, lgt), 0.);
    vec3  fogColor  = mix(vec3(0.8,0.8,1)*1.2, vec3(1)*1.3, pow(sdt,2.0)+mx*0.5);
    return mix( col, fogColor, clamp(den + mx,0.,1.) );
}

#ifdef ADD_WATER
vec3 water(vec3 ro, vec3 rd, vec3 col, float ds)
{
    vec3 pos = ro + rd*ds;
	float den = 0.0;
	vec3  wColor = col;

	if (pos.y < WATER_LEVEL) {
		den = 1.0 - clamp(pos.y / WATER_LEVEL, 0.0, 1.0);
		wColor = mix(vec3(0.05, 0.07, 0.12), vec3(0.01, 0.02, 0.04), den);
		den = 1.0;
	}

	return mix( col, wColor, clamp(den,0.,1.) );
}
#endif //ADD_WATER

#ifdef ADD_BEACHES
vec3 beach(vec3 ro, vec3 rd, vec3 col, float ds)
{
    vec3 pos = ro + rd*ds;
	float den = 0.0;
	vec3 sColor = vec3(0.0);

	if (pos.y >= WATER_LEVEL && pos.y < WATER_LEVEL + 0.003) {
		den = 1.0 - clamp((pos.y - WATER_LEVEL) / 0.003, 0.0, 1.0);
		den *= 0.15;
		sColor = mix(vec3(0.4650,0.3850,0.250), vec3(0.87,0.780,0.670), den);
	}

	return mix( col, sColor, den );
}
#endif //ADD_BEACHES

float linstep(in float mn, in float mx, in float x){
	return clamp((x - mn)/(mx - mn), 0., 1.);
}

//Complete hack, but looks good enough :)
vec3 scatter(vec3 ro, vec3 rd)
{   
    float sd= max(dot(lgt, rd)*0.5+0.5,0.);
    float dtp = 13.-(ro + rd*(FAR)).y*3.5;
    float hori = (linstep(-1500., 0.0, dtp) - linstep(11., 500., dtp))*1.;
    hori *= pow(sd,.04);
    
    vec3 col = vec3(0);
    col += pow(hori, 500.)*vec3(0.15, 0.4,  1)*3.;
    col += pow(hori, 200.)*vec3(0.15, 0.4,  1)*3.;
    col += pow(hori, 25.)* vec3(0.4, 0.5,  1)*.3;
    col += pow(hori, 7.)* vec3(0.6, 0.7,  1)*.8;
    
    return col;
}

//From Dave_Hoskins (https://www.shadertoy.com/view/4djSRW)
vec3 hash33(vec3 p)
{
    p = fract(p * vec3(443.8975,397.2973, 491.1871));
    p += dot(p.zxy, p.yxz+19.27);
    return fract(vec3(p.x * p.y, p.z*p.x, p.y*p.z));
}

//Very happy with this star function, cheap and smooth
vec3 stars(in vec3 pos, in float level)
{
	vec3 p = pos * level;//128.0;
    vec3 c = vec3(0.);
    float res = iResolution.x*0.8;
    
	for (float i=0.;i<3.;i++)
    {
        vec3 q = fract(p*(.15*res))-0.5;
        vec3 id = floor(p*(.15*res));
        vec2 rn = hash33(id).xy;
        float c2 = 1.-smoothstep(0.,.6,length(q));
        c2 *= step(rn.x,.0005+i*i*0.001);
        c += c2*(mix(vec3(0.5,0.49,0.1),vec3(0.75,0.9,1.),rn.y)*0.25+0.75);
        p *= 1.4;
    }
    return c*c*.7;
}

void getBackground( out vec4 fragColor, in vec2 fragCoord )
{
#ifdef ADD_SUN
	updateLightPos();
#endif //ADD_SUN

	vec2 q = fragCoord.xy / iResolution.xy;
    vec2 p = q - 0.5;
	p.x*=iResolution.x/iResolution.y;
	vec2 mo = vec2(-0.25, 0.333);//iMouse.xy / iResolution.xy-.5;
    mo = (mo==vec2(-.5))?mo=vec2(-.2,0.3):mo;
    mo.x *= 1.2;
    mo -= vec2(1.2,-0.1);
	mo.x *= iResolution.x/iResolution.y;
    mo.x += sin(time*0.15)*0.2;
	
    vec3 ro = vec3(650., sin(time*0.2)*0.25+10.,-time);
    vec3 eye = normalize(vec3(cos(mo.x),-0.5+mo.y,sin(mo.x)));
    vec3 right = normalize(vec3(cos(mo.x+1.5708),0.,sin(mo.x+1.5708)));
    vec3 up = normalize(cross(right, eye));
	vec3 rd = normalize((p.x*right + p.y*up)*1.05 + eye);
    rd.y += abs(p.x*p.x*0.015);
    rd = normalize(rd);
	
    float count = 0.;
	float rz = march(ro,rd, count);
    
    vec3 scatt = scatter(ro, rd);
    
    vec3 bg = max(stars(rd, 0.666), stars(rd, 1.0))*(1.0-clamp(dot(scatt, vec3(1)),0.,1.));
    vec3 col = bg;
    
    vec3 pos = ro+rz*rd;

#ifdef ADD_SUN
	col = bg = sun(col, rd);
#endif //ADD_SUN

    vec3 nor= normal( pos, rz );
    if ( rz < FAR )
    {
        nor = bump(pos,nor,rz);
        float amb = clamp( 0.5+0.5*nor.y, 0.0, 1.0 );
        float dif = clamp( dot( nor, lgt ), 0.0, 1.0 );
        float bac = clamp( dot( nor, normalize(vec3(-lgt.x,0.0,-lgt.z))), 0.0, 1.0 );
        float spe = pow(clamp( dot( reflect(rd,nor), lgt ), 0.0, 1.0 ),500.);
        float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 2.0 );
        vec3 brdf = 1.*amb*vec3(0.10,0.11,0.12);
        brdf += bac*vec3(0.15,0.05,0.0);
        brdf += 2.3*dif*vec3(0.15,0.05,0.0);
        col = vec3(0.25,0.25,0.3);
        float crv = curv(pos, 2.)*1.;
        float crv2 = curv(pos, .4)*2.5;
        
        col += clamp(crv*0.9,-1.,1.)*vec3(0.25,.6,.5);
        col = col*brdf + col*spe*.1 +.1*fre*col;
        col *= crv*1.+1.;
        col *= crv2*1.+1.;
    }
	
#ifdef ADD_WATER
	col = water(ro, rd, col, rz);
#endif //ADD_WATER
#ifdef ADD_BEACHES
	col = beach(ro, rd, col, rz);
#endif //ADD_BEACHES
    col = fog(ro, rd, col, rz);
    col = mix(col,bg,smoothstep(FAR-150., FAR, rz));
    col += scatt;
    
    col = pow( col, vec3(1,0.9,0.9) );
    col = mix(col, smoothstep(0.,1.,col), 0.2);
    col *= pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1)*0.9+0.1;
    
	fragColor = vec4(col, 1.0);
}
#endif //__GREEN_PLANET__

#ifdef __TIE_FIGHTERS__
//Fancy ties by nimitz (twitter: @stormoid)

/*
	Somewhat complex modelling in a fully procedural shader that runs decently fast.
	I kinda cheated on the wings, the real ones are less hexagonal than this.
	Not doing proper occlusion checking for the lens flares to keep it fast.
*/

#define ITR 100
#define FAR 155.
#define time iTime

const float fov = 1.5;

//Global material id (keeps code cleaner)
float matid = 0.;

//--------------------Utility, Domain folding and Primitives---------------------
float tri(in float x){return abs(fract(x)-.5);}
mat3 rot_x(float a){float sa = sin(a); float ca = cos(a); return mat3(1.,.0,.0,    .0,ca,sa,   .0,-sa,ca);}
mat3 rot_y(float a){float sa = sin(a); float ca = cos(a); return mat3(ca,.0,sa,    .0,1.,.0,   -sa,.0,ca);}
mat3 rot_z(float a){float sa = sin(a); float ca = cos(a); return mat3(ca,sa,.0,    -sa,ca,.0,  .0,.0,1.);}
vec3 rotz(vec3 p, float a){
    float s = sin(a), c = cos(a);
    return vec3(c*p.x - s*p.y, s*p.x + c*p.y, p.z);
}

//From Dave_Hoskins
vec2 hash22(vec2 p){
	p  = fract(p * vec2(5.3983, 5.4427));
    p += dot(p.yx, p.xy +  vec2(21.5351, 14.3137));
	return fract(vec2(p.x * p.y * 95.4337, p.x * p.y * 97.597));
}

vec3 hash33(vec3 p){
	p  = fract(p * vec3(5.3983, 5.4427, 6.9371));
    p += dot(p.yzx, p.xyz  + vec3(21.5351, 14.3137, 15.3219));
	return fract(vec3(p.x * p.z * 95.4337, p.x * p.y * 97.597, p.y * p.z * 93.8365));
}


//2dFoldings, inspired by Gaz/Knighty  see: https://www.shadertoy.com/view/4tX3DS
vec2 foldHex(in vec2 p)
{
    p.xy = abs(p.xy);
    const vec2 pl1 = vec2(-0.5, 0.8657);
    const vec2 pl2 = vec2(-0.8657, 0.5);
    p -= pl1*2.*min(0., dot(p, pl1));
    p -= pl2*2.*min(0., dot(p, pl2));
    return p;
}

vec2 foldOct(in vec2 p)
{
    p.xy = abs(p.xy);
    const vec2 pl1 = vec2(-0.7071, 0.7071);
    const vec2 pl2 = vec2(-0.9237, 0.3827);
    p -= pl1*2.*min(0., dot(p, pl1));
    p -= pl2*2.*min(0., dot(p, pl2));
    
    return p;
}

float sbox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float cyl( vec3 p, vec2 h )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float torus( vec3 p, vec2 t ){
  return length( vec2(length(p.xz)-t.x,p.y) )-t.y;
}

//using floor() in a SDF causes degeneracy.
float smoothfloor(in float x, in float k)
{
    float xk = x+k*0.5;
    return floor(xk-1.) + smoothstep(0.,k,fract(xk));
}

float hexprism(vec3 p, vec2 h){
    vec3 q = abs(p);
    return max(q.z-h.y,max((q.y*0.866025+q.x*0.5),q.x)-h.x);
}

//------------------------------------------------------------------------

vec3 position(in vec3 p)
{
    float dst =7.;
    float id = floor(p.z/dst*.1);
    p.xy += sin(id*10.+time);
    p.z += sin(id*10.+time*0.9)*.5;
    p = rotz(p,sin(time*0.5)*0.5+id*0.1);
    p.z = (abs(p.z)-dst)*sign(p.z);
    return p;
}

float map(vec3 p)
{
    matid= 0.;
	vec3 bp =p; //keep original coords around
   
    float mn = length(bp)-.7; //main ball
    
    //Cockpit
    p.z -=0.8;
    vec3 q = p;
    q.xy *= mat2(0.9239, 0.3827, -0.3827, 0.9239); //pi/8
    q.xy = foldOct(q.xy);
    p.z += length(p.xy)*.46;
    p.xy = foldOct(p.xy);
    float g = sbox(p-vec3(0.32,0.2,0.),vec3(.3,0.3,0.04)); //Cockpit Spokes
   	float mg = min(mn,g);
    if (mn < -g)matid = 2.;
    mn = max(mn,-g);
    float g2 = sbox(q,vec3(.45,0.15,.17)); //Cockpit center
    if (mn < -g2)matid = 2.;
    mn = max(mn,-g2);
    mn = min(mn,torus(bp.yzx+vec3(0,-.545,0),vec2(0.4,0.035))); //Cockpit lip
    mn = max(mn,-torus(bp+vec3(0,-.585,0),vec2(0.41,0.03))); //Hatch
    
    //Engine (Polar coords)
    mn = max(mn,-(bp.z+0.6));
    vec3 pl = bp.xzy;
    pl = vec3(length(pl.xz)-0.33, pl.y, atan(pl.z,pl.x));
    pl.y += .55;
    mn =  min(mn,sbox(pl, vec3(.29+bp.z*0.35,.25,4.)));
    pl.z = fract(pl.z*1.7)-0.5;
    mn = min(mn, sbox(pl + vec3(0.03,0.09,0.), vec3(0.05, .1, .2)));
    
    p = bp;
    p.x = abs(p.x)-1.1; //Main symmetry
    
    mn = min(mn, cyl(p.xzy-vec3(-0.87,.43,-0.48),vec2(.038,0.1))); //Gunports
    
    const float wd = 0.61; //Main width
    const float wg = 1.25; //Wign size
    
    mn = min(mn, cyl(p.yxz,vec2(0.22+smoothfloor((abs(p.x+0.12)-0.15)*4.,0.1)*0.04,0.6))); //Main structure
    vec3 pp = p;
    pp.y *= 0.95;
    vec3 r = p;
    p.y *= 0.65;
    p.z= abs(p.z);
    p.z -= 0.16;
    q = p;
    r.y = abs(r.y)-.5;
    mn = min(mn, sbox(r-vec3(-.3,-0.37,0.),vec3(0.35,.12-smoothfloor(r.x*2.-.4,0.1)*0.1*(-r.x*1.7),0.015-r.x*0.15))); //Side Structure
    mn = min(mn, sbox(r-vec3(-.0,-0.5,0.),vec3(0.6, .038, 0.18+r.x*.5))); //Side Structure
    p.zy = foldHex(p.zy)-0.5;
    pp.zy = foldHex(pp.zy)-0.5;
    mn = min(mn, sbox(p-vec3(wd,wg,0),vec3(0.05,.01,.6))); //wing Outer edge
    q.yz = foldHex(q.yz)-0.5;
    
    
    mn = min(mn, sbox(q-vec3(wd,-0.495-abs(q.x-wd)*.07,0.),vec3(0.16-q.z*0.07,.015-q.z*0.005,wg+.27))); //wing spokes
    mn = min(mn, sbox(q-vec3(wd,-0.5,0.),vec3(0.12-q.z*0.05,.04,wg+.26))); //Spoke supports
    
    mn = min(mn, sbox(pp-vec3(wd,-0.35,0.),vec3(0.12,.35,.5))); //Wing centers
    mn = min(mn, sbox(pp-vec3(wd,-0.35,0.),vec3(0.15+tri(pp.y*pp.z*30.*tri(pp.y*2.5))*0.06,.25,.485))); //Wing centers
    
    float wgn = sbox(p-vec3(wd,0,0),vec3(0.04,wg,1.));//Actual wings (different material)
    if (mn > wgn)matid = 1.;
    mn = min(mn, wgn);
    
    //Engine port
    float ep = hexprism(bp+vec3(0,0,0.6),vec2(.15,0.02));
    if (mn > ep)matid = 2.;
    mn = min(mn, ep);

    
    return mn;
}

float march(in vec3 ro, in vec3 rd)
{
	float precis = 0.001;
    float h=precis*2.0;
    float d = 0.;
    for( int i=0; i<ITR; i++ )
    {
        if( abs(h)<precis || d>FAR ) break;
        d += h;
        float res = map(position(ro+rd*d))*0.93;
        h = res;
    }
	return d;
}

//greeble-ish texture
float tex(in vec3 q)
{
    q.zy = foldOct(q.zy);
    vec2 p = q.zx;
    float id = floor(p.x)+100.*floor(p.y);
    float rz= 1.0;
    for(int i = 0;i<3;i++)
    {
        vec2 h = (hash22(floor(p))-0.5)*.95;
        vec2 q = fract(p)-0.5;
        q += h;
        float d = max(abs(q.x),abs(q.y))+0.1;
        p += 0.5;
        rz += min(rz,smoothstep(0.5,.55,d))*1.;
        p*=1.4;
    }
    rz /= 7.;
    return rz;
}

vec3 wingtex(in vec2 p, in float ds, in float ind)
{
    p.y *= 0.65;
    p.x = abs(p.x)-0.14;
    p = foldHex(p);
    
    //Fighting aliasing with distance and incidence.
    float rz = smoothstep(0.07,.0,tri(p.x*7.5))*15.*(ind)/(ds*ds);
    return vec3(1,.9,.8)*rz*0.7;
}

float mapHD(in vec3 p)
{
    float d= map(p);
    d += tex(p*3.+vec3(4.,0.,0.))*0.03/(length(p)*.3+.9);
    return d;
}

vec3 normal(const in vec3 p)
{  
    vec2 e = vec2(-1., 1.)*0.008;
	return normalize(e.yxx*mapHD(p + e.yxx) + e.xxy*mapHD(p + e.xxy) + 
					 e.xyx*mapHD(p + e.xyx) + e.yyy*mapHD(p + e.yyy) );   
}

//form iq
float getAO( in vec3 pos, in vec3 nor )
{
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01 + 0.13*float(i)/3.;
        vec3 aopos =  nor * hr + pos;
        float dd = map( aopos );
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1. - 3.5*occ, 0.0, 1.0 );    
}

//smooth and cheap 3d starfield
vec3 stars(in vec3 p)
{
    vec3 c = vec3(0.);
    float res = iResolution.x*.85*fov;
    
    //Triangular deformation (used to break sphere intersection pattterns)
    p.x += (tri(p.z*50.)+tri(p.y*50.))*0.006;
    p.y += (tri(p.z*50.)+tri(p.x*50.))*0.006;
    p.z += (tri(p.x*50.)+tri(p.y*50.))*0.006;
    
	for (float i=0.;i<3.;i++)
    {
        vec3 q = fract(p*(.15*res))-0.5;
        vec3 id = floor(p*(.15*res));
        float rn = hash33(id).z;
        float c2 = 1.-smoothstep(-0.2,.4,length(q));
        c2 *= step(rn,0.005+i*0.014);
        c += c2*(mix(vec3(1.0,0.75,0.5),vec3(0.85,0.9,1.),rn*30.)*0.5 + 0.5);
        p *= 1.15;
    }
    return c*c*1.5;
}

vec3 flare(in vec2 p, in vec2 pos, in vec3 lcol, in float sz)
{
	vec2 q = p-pos;
    q *= sz;
	float a = atan(q.x,q.y);
    float r = length(q);
    
    float rz= 0.;
    rz += .07/(length((q)*vec2(7.,200.))); //horiz streaks
    rz += 0.3*(pow(abs(fract(a*.97+.52)-0.5),3.)*(sin(a*30.)*0.15+0.85)*exp2((-r*5.))); //Spokes
	
    vec3 col = vec3(rz)*lcol;   
    col += exp2((1.-length(q))*50.-50.)*lcol*vec3(3.);
    col += exp2((1.-length(q))*20.-20.)*lcol*vec3(1,0.95,0.8)*0.5;    
    return clamp(col,0.,1.);
}


//A weird looking small moon
float slength(in vec2 p){ return max(abs(p.x), abs(p.y)); }
float moontex(in vec3 p)
{
    float r = length(p);
    vec3 q = vec3(r, acos(p.y/r), atan(p.z,p.x));
    q *= 6.5;
    vec3 bq = q;
    q.y = q.y*0.44-0.42;
    vec2 id = floor(q.zy);
    vec2 s = fract(q.zy)-0.5;
    
    float rz = 1.;
    float z = 0.25;
    for(int i=0;i<=3;i++)
    {
        vec2 rn = hash22(id+vec2(i)+0.0019)*.6 + 0.4;
        s -= abs(s)-rn*0.45;
        rz -= smoothstep(0.5,0.45-float(i)*0.1,slength(s*rn*1.3))*z;
        q *= 3.5;
        z *= .85;
        id = floor(q.zy);
    	s = fract(q.zy)-0.5;
    }
    
    rz -= smoothstep(0.035,.03,abs(bq.y-10.15))*.3; //main trench
    return rz;
}

float sphr(in vec3 ro, in vec3 rd, in vec4 sph)
{
	vec3 oc = ro - sph.xyz;
	float b = dot(oc,rd);
	float c = dot(oc,oc) - sph.w*sph.w;
	float h = b*b - c;
	if (h < 0.) return -1.;
	else return -b - sqrt(h);
}

void getBackground( out vec4 fragColor, in vec2 fragCoord )
{	
	vec2 p = fragCoord.xy/iResolution.xy-0.5;
	//p.x*=iResolution.x/iResolution.y;
	vec2 mo = vec2(0.0);//iMouse.xy / iResolution.xy-.5;
    mo = (mo==vec2(-.5))?mo=vec2(-0.15,0.):mo;
	mo.x *= iResolution.x/iResolution.y;
    mo*=4.;
	mo.x += time*0.17+0.1;

    vec3 ro = vec3(0.,0.,17.);
    vec3 rd = normalize(vec3(vec2(-p.x,p.y),-fov));
    float cms = 1.-step(sin((time+0.0001)*0.5),0.);
    mat3 inv_cam = mat3(0);
    
    if (cms < 0.5)
    {
        mat3 cam = rot_x(-mo.y)*rot_y(-mo.x);
        inv_cam = rot_y(-mo.x)*rot_x(mo.y); 
        ro *= cam;rd *= cam;
    }
    else
    {
        float frct = fract(time*0.15915);
        float frct2 = fract(time*0.15915+0.50001);
        float cms = 1.-step(sin((time+0.0001)*0.5),0.);
        ro = vec3(-15.,1.-(step(frct2,0.5))*frct2*40.,140.-frct*280.);
        vec3 ta = vec3(0);
        vec3 fwd = normalize(ta - ro);
        vec3 rgt = normalize(cross(vec3(0., 1., 0.), fwd ));
        vec3 up = normalize(cross(fwd, rgt));
        mat3 cam = mat3(rgt,up,-fwd);
        rd = normalize(vec3(vec2(p.x,p.y),-fov))*cam;
        inv_cam = transpose(cam);
    }
    
	float rz = march(ro,rd);
	
    vec3 lgt = normalize( vec3(.2, 0.35, 0.7) );
    vec3 col = vec3(0.0);
    float sdt = max(dot(rd,lgt),0.); 
    
    vec3 lcol = vec3(1,.85,0.73);
    col += stars(rd);
    
    vec3 fp = (-lgt*inv_cam);
    col += clamp(flare(p,-fp.xy/fp.z*fov, lcol,1.)*fp.z*1.1,0.,1.);
    
    //Another nearby star
    vec3 lcol2 = vec3(0.25,.38,1);
    vec3 lgt2 = normalize(vec3(-0.2,-.1,-0.8));
    fp = (-lgt2*inv_cam);
    col += clamp(flare(p,-fp.xy/fp.z*fov, lcol2,2.)*fp.z*1.1,0.,1.);
    
    //A "moon"
    vec4 sph = vec4(2000,500,-700,1000);
    float mn = sphr(ro,rd,sph);
    
    if (mn > 0.)
    {
        vec3 pos = ro+rd*mn;
        vec3 nor = normalize(pos-sph.xyz);
        vec3 dif = clamp(dot( nor, lgt ), 0., 1.)*0.985*lcol;
        vec3 bac = clamp( dot( nor, lgt2), 0.0, 1.0 )*lcol2;
        col = moontex((pos-sph.xyz))*vec3(0.52,0.54,0.7)*0.3;
        col *= dif + bac*0.01 + 0.005;
    }
    
    
    if ( rz < FAR )
    {
        float mat = matid;
        vec3 pos = ro+rz*rd;
        pos = position(pos);
        vec3 nor= normal(pos);
        float dif = clamp( dot( nor, lgt ), 0.0, 1.0 );
        float bac = clamp( dot( nor, lgt2), 0.0, 1.0 );
        float spe = pow(clamp( dot( reflect(rd,nor), lgt ), 0.0, 1.0 ),7.);
        float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 3.0 );
        vec3 brdf = vec3(0.);
        brdf += bac*mix(lcol2,vec3(1),0.5)*0.06;
        brdf += 1.5*dif*lcol;
        col = vec3(0.54,0.56,0.65)*1.1;
        col *= col;
        if (mat == 1.) 
        {
            brdf *= 0.0;
            spe *= 0.05;
            fre *= 0.05;
            brdf += wingtex(pos.zy,rz, max(dot(-rd,nor),0.)*0.5+0.5)*0.6;
        }
        else if (mat == 2.)
        {
            col = vec3(0);
            spe *= 0.1;
        }
        
        col = col*brdf + spe*.23 +.03*fre;
        col *= getAO(pos,nor);
    }
    
    col = clamp(col, 0.,1.);
    col = pow(clamp(col,0.,1.), vec3(0.416667))*1.055 - 0.055; //sRGB
	
	fragColor = vec4( col, 1.0 );
}
#endif //__TIE_FIGHTERS__

#ifdef __HYPERSPACE__
// 'Warp Speed' by David Hoskins 2013.
// I tried to find gaps and variation in the star cloud for a feeling of structure.
void getBackground( out vec4 fragColor, in vec2 fragCoord )
{
    float time = iTime * 16.0;

    float s = 0.0, v = 0.0;
	vec2 uv = (fragCoord.xy / iResolution.xy) * 2.0 - 1.0;
	float t = time*0.005;
	uv.x = (uv.x * iResolution.x / iResolution.y) + sin(t) * 0.5;
	float si = sin(t + 2.17); // ...Squiffy rotation matrix!
	float co = cos(t);
	uv *= mat2(co, si, -si, co);
	vec3 col = vec3(0.0);
	vec3 init = vec3(0.25, 0.25 + sin(time * 0.001) * 0.4, floor(time) * 0.0008);
	for (int r = 0; r < 100; r++) 
	{
		vec3 p = init + s * vec3(uv, 0.143);
		p.z = mod(p.z, 2.0);
		for (int i=0; i < 10; i++)	p = abs(p * 2.04) / dot(p, p) - 0.75;
		v += length(p * p) * smoothstep(0.0, 0.5, 0.9 - s) * .002;
		// Get a purple and cyan effect by biasing the RGB in different ways...
		col +=  vec3(v * 0.8, 1.1 - s * 0.5, .7 + v * 0.5) * v * 0.013;
		s += .01;
	}
	fragColor = vec4(col, 1.0);
}
#endif //__HYPERSPACE__

#ifdef __HYPERSPACE2__
void getBackground( out vec4 fragColor, in vec2 fragCoord ) 
{
	float time = (iTime+2.4) * 16.0;
	
    float s = 0.0, v = 0.0;
	vec2 uv = (fragCoord.xy / iResolution.xy) * 2.0 - 1.0;
	float t = time*0.005;
	uv.x = (uv.x * iResolution.x / iResolution.y) + sin(t)*.5;
	float si = sin(t+2.17); // ...Squiffy rotation matrix!
	float co = cos(t);
	uv *= mat2(co, si, -si, co);
	vec3 col = vec3(0.0);
	for (int r = 0; r < 100; r++) 
	{
		vec3 p= vec3(0.3, 0.2, floor(time) * 0.0008) + s * vec3(uv, 0.143);
		p.z = mod(p.z,2.0);
		for (int i=0; i < 10; i++) p = abs(p*2.04) / dot(p, p) - 0.75;
		v += length(p*p)*smoothstep(0.0, 0.5, 0.9 - s) * .002;
		col +=  vec3(v * 0.8, 1.1 - s * 0.5, .7 + v * 0.5) * v * 0.013;
		s += .01;
	}	
	
    fragColor = vec4(col, 1.0);
}
#endif //__HYPERSPACE2__

#ifdef __BESPIN__
// Flight over Bespin - yamahabob
// Borrowed most of this code from inigo quilez et al
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define PI 3.141592654

// General functions

mat3 rx90 = mat3(1.0, 0.0, 0.0,
				 0.0, 0.0, 1.0,
				 0.0,-1.0, 0.0 );
mat3 ry90 = mat3(0.0, 0.0,-1.0,
				 0.0, 1.0, 0.0, 
				 1.0, 0.0, 0.0 );
mat3 rz90 = mat3(0.0,-1.0, 0.0,
				 1.0, 0.0, 0.0, 
				 0.0, 0.0, 1.0 );

mat3 rotX( float a )
{
	return mat3(1.0,    0.0,    0.0,
				0.0, cos(a),-sin(a),
				0.0, sin(a), cos(a) );
}

mat3 rotY( float a )
{
	return mat3(cos(a), 0.0, sin(a),
				   0.0, 1.0,    0.0, 
			   -sin(a), 0.0, cos(a) );
}

mat3 rotZ( float a )
{
	return mat3(cos(a),-sin(a), 0.0,
				sin(a), cos(a), 0.0,
				   0.0,    0.0, 1.0 );
}

mat2 rot(float a) {
	return mat2(cos(a),sin(a),-sin(a),cos(a));
}

// -------------------------------------
// Cloud Calculations
// (borrowed from iq)
// -------------------------------------

#define SPEED 5.

float speed;

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

vec3 hash33(vec3 p){
	p  = fract(p * vec3(5.3983, 5.4427, 6.9371));
    p += dot(p.yzx, p.xyz  + vec3(21.5351, 14.3137, 15.3219));
	return fract(vec3(p.x * p.z * 95.4337, p.x * p.y * 97.597, p.y * p.z * 93.8365));
}

float noise( in vec3 x )
{
	/*
	vec3 p = floor(x);
	vec3 f = fract(x);
	//f = f*f*(3.0-2.0*f);
	vec2 uv = (p.xy + vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = texture( iChannel0, 0.00390625*uv ).yx;
	return 1.5*mix( rg.x, rg.y, f.z ) - 0.75;
	*/

	return 1.5*pNoise(x)-0.75;
}

float map5( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*speed;
	float f;
	f  = 0.50000*noise( q ); q = q*2.02;
	f += 0.25000*noise( q ); q = q*2.03;
	f += 0.12500*noise( q ); q = q*2.01;
	f += 0.06250*noise( q ); q = q*2.02;
	f += 0.03125*noise( q );
	return clamp( -p.y - 0.5 + 1.75*f, 0.0, 1.0 );
}

float map4( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*speed;
	float f;
	f  = 0.50000*noise( q ); q = q*2.02;
	f += 0.25000*noise( q ); q = q*2.03;
	f += 0.12500*noise( q ); q = q*2.01;
	f += 0.06250*noise( q );
	return clamp( -p.y - 0.5 + 1.75*f, 0.0, 1.0 );
}
float map3( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*speed;
	float f;
	f  = 0.50000*noise( q ); q = q*2.02;
	f += 0.25000*noise( q ); q = q*2.03;
	f += 0.12500*noise( q );
	return clamp( -p.y - 0.5 + 1.75*f, 0.0, 1.0 );
}
float map2( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*speed;
	float f;
	f  = 0.50000*noise( q ); q = q*2.02;
	f += 0.25000*noise( q );;
	return clamp( -p.y - 0.5 + 1.75*f, 0.0, 1.0 );
}

vec3 sundir = normalize( vec3(-0.5,-0.1,-1.0) );

vec4 integrate( in vec4 sum, in float dif, in float den, in vec3 bgcol, in float t )
{
	// lighting
	vec3 lin = vec3(0.65,0.68,0.7)*1.2 + 0.5*vec3(0.7, 0.5, 0.3)*dif;        
	vec4 col = vec4( mix( 1.15*vec3(1.0,0.95,0.8), vec3(0.65), den ), den );
	col.xyz *= lin;
	col.xyz = mix( col.xyz, bgcol, 1.0-exp(-0.004*t*t) );
	// front to back blending    
	col.a *= 0.4;
	col.rgb *= col.a;
	return sum + col*(1.0-sum.a);
}

#define MARCH(STEPS,MAPLOD) for(int i=0; i<STEPS; i++) { vec3  pos = ro + t*rd; if( pos.y<-3. || pos.y>1.2 || sum.a > 0.99 ) break; float den = MAPLOD( pos ); if( den>0.1 ) { float dif = clamp((den - MAPLOD(pos+0.5*sundir))*2., 0.0, 1.0 ); sum = integrate( sum, dif, den, bgcol, t ); } t += max(0.2,0.03*t); }

vec4 raymarch( in vec3 ro, in vec3 rd, in vec3 bgcol )
{
	vec4 sum = vec4(0.0);

	float t = 0.0;

	MARCH(25,map4);
	MARCH(20,map3);
	MARCH(15,map2);
	MARCH(15,map2);

	return clamp( sum, 0.0, 1.0 );
}

mat3 lookat( vec3 fw, vec3 up ){
	fw=normalize(fw);vec3 rt=normalize(cross(fw,normalize(up)));return mat3(rt,cross(rt,fw),fw);
}

mat3 setCamera( in vec3 fw )
{
	vec3 cw = normalize( fw );
	vec3 cp = vec3( 0.0, 1.0, 0.0 );
	vec3 cu = vec3( -cw.z, 0.0, cw.x );
	vec3 cv = normalize( cross(cu,cw) );
	return mat3( cu, cv, cw );
}

vec3 cloudRender( in vec3 ro, in vec3 rd )
{
	// background sky     
	float sun = clamp( dot(sundir,rd), 0.0, 1.0 );
	vec3 col = 0.9*vec3(0.949,0.757,0.525) - rd.y*0.2*vec3(0.949,0.757,0.525);// + 0.15*0.5;
	col += 0.8*vec3(1.0,.6,0.1)*pow( sun, 20.0 );

	// clouds    
	vec4 res = raymarch( ro, rd, col );
	col = col*(1.0-res.w) + res.xyz;
	
	// sun glare    
	col += 0.1*vec3(0.949,0.757,0.525)*pow( sun, 3.0 );

	return col;
}


// =========== Ship Calculations ===========

float time;
vec3 shippos;
mat3 shipltow;
mat2 shiptilt;

// the flight path

vec3 shippath( float t )
{
	return vec3( 5.*sin( 0.9*t ), 2.5*sin( 0.6*t ), 0. );
}

vec3 shipvel( float t )
{
	return vec3( 5.*0.9*cos( 0.9*t ), 2.5*0.6*cos( 0.6*t ), 15. );
}

vec3 shipacc( float t )
{
	return vec3( -4.*0.9*0.9*sin( 0.9*(t)), -2.5*0.6*0.6*sin( 0.6*t ), 0. );
}


// distance functions for basic shapes

float sdPlane( vec3 p, vec4 n ) { return dot(p,n.xyz) + n.w; }

float sdSphere( vec3 p, float s ) { return length(p)-s; }

float sdBox( vec3 p, vec3 b ) { vec3 d = abs(p) - b; return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0)); }

float sdHexPrism( vec3 p, vec2 h ) { vec3 q = abs(p); return max(q.z-h.y,max((q.x*0.866025+q.y*0.5),q.y)-h.x); }

float sdCapsule( vec3 p, vec3 a, vec3 b, float r ) { vec3 pa = p-a, ba = b-a; return length( pa - ba*clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 )) - r; }

float sdConeSection( in vec3 p, in float h, in float r1, in float r2 )
{
	float d1 = -p.y - h;
	float q = p.y - h;
	float si = 0.5*(r1-r2)/h;
	float d2 = max( sqrt( dot(p.xz,p.xz)*(1.0-si*si)) + q*si - r2, q );
	return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
}

float length8( vec3 d ) { vec3 q = pow( d, vec3(8.) ); return pow( q.x + q.y + q.z, 0.125 ); }

float sdTorus82( vec3 p, vec2 t ) { vec2 q = vec2(length(p.xz)-t.x,p.y); return length8(vec3(q,0.))-t.y; }

float sdCylinder( vec3 p, vec3 c ) { return length(p.xz-c.xy)-c.z; }

float sdCappedCylinder( vec3 p, vec2 h ) { vec2 d = abs(vec2(length(p.xz),p.y)) - h; return min(max(d.x,d.y),0.0) + length(max(d,0.0)); }

vec2 min2( vec2 d1, vec2 d2 ) { return ( d1.x < d2.x ) ? d1 : d2; }

vec2 max2( vec2 d1, vec2 d2 ) { return ( d1.x > d2.x ) ? d1 : d2; }

float smin( float a, float b, float k ) { float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 ); return mix( b, a, h ) - k*h*(1.0-h); }

vec2 smin( vec2 a, vec2 b, float k ) { float h = clamp( 0.5+0.5*(b.x-a.x)/k, 0.0, 1.0 ); return mix( b, a, h ) - k*h*(1.0-h); }


// texture functions

float noise( float s )
{
	//vec2 uv = vec2( s, s );
	//return texture( iChannel0, uv ).x;
	return pNoise(vec2(s));
}

float cubicPulse( float c, float w, float x )
{
	x = abs(x - c);
	if( x>w ) return 0.0;
	x /= w;
	return 1.0 - x*x*(3.0-2.0*x);
}

// plate texture
float texPlates( vec2 uv )
{
	vec2 n = vec2( 10., 4. );
	vec2 sec = floor( uv*n );
	vec2 suv = uv*n - sec;
	
	float s1 = noise( 0.3*( uv.x + sec.y/n.y ) );
	float s2 = 0.3*s1+0.5;
	float s3 = noise( 0.4*sec.y/n.y );
	float luma = 0.5*(1. - cubicPulse( s3, 0.03, suv.x )) + 0.5;
	luma *= mix(1.0, s2, smoothstep(0.0, 1.0, suv.y));
	luma *= 0.5*smoothstep( 0.0, 0.005*n.y, min( suv.y, 1.-suv.y)) + 0.5;
	return clamp(luma, 0., 1.);
}

// circular hull texture
vec3 texHull( vec3 p )
{
	vec2 uv;
	uv.x = atan(p.z, p.x)/PI;
	float d = length(p.xz);
	uv.y = 0.925 - 0.5*d;
	vec3 shipCol1 = 0.9*vec3(0.729, 0.694, 0.627);
	vec3 shipCol2 = 0.4*vec3(0.6, 0.537, 0.447);
	return texPlates( uv ) * mix(shipCol1, shipCol2, cubicPulse( 0., 0.5, uv.x ));
}

// ship world to local transformation

vec3 shipw2l( vec3 p )
{
	p.z = -p.z;
	p = shipltow*(p - shippos);
	p.zy *= shiptilt;
	return p;
}

// ship distance evaluation where p is in world coords

vec2 shipDE(vec3 p)
{
	float d1, d2, d3, d4;
	p = shipw2l( p );
	d1 = sdSphere( p, 3.3 ); // the bounding 'hit' sphere
	if ( d1 > 0.2 ) return vec2( d1, 0. );
	
	// main disk
	vec3 q = p;
	q.y = abs(q.y);
	d1 = sdSphere( q + vec3( 0., 14.78, 0. ), 15. );
	d1 = max( d1, sdSphere( q, 2. ));
	d1 = max( -q.y + 0.06, d1 );
	d2 = max(  q.y - 0.06, sdSphere( q, 1.95 ));
	vec2 vres = vec2( min( d1, d2 ), 1.);
	
	// centre pylon
	vres = smin( vres, vec2( sdCappedCylinder( p, vec2( 0.45, 0.28 )), 2.), 0.1);
	
	// front forks
	q = p;
	q.z = abs(q.z);
	q += vec3(1.45, 0., -1.1);
	float front = sdBox( q, vec3( 1.8, 0.09, 0.8 )); // front
	vec3 norm = normalize(vec3(-1.1, 0.0, 2.3));
	q = p;
	q.z = abs(q.z);
	float plane = sdPlane( q, vec4(-norm, 1.92) );
	d1 = max( -plane, front );
	vres = min2( vres, vec2( d1, 3. ));
	
	// Z crossbar
	d1 = sdHexPrism( p, vec2( 0.26, 2.0 ));
	d2 = sdBox( p, vec3( 0.8, 0.8, 0.6 ));
	vres = min2( vres, vec2( max( -d2, d1 ), 4.));
	
	// X crossbar
	q = p;
	q.y = abs(q.y);
	q = ry90 * rotZ(0.07) * q + vec3( 0., 0.02, -1.5);
	vres = min2( vres, vec2( max( -abs(p.y) + 0.07, sdHexPrism( q, vec2(0.36, 0.9 ))), 5.));

	// cockpit walkway
	vec3 p1 = vec3(-0.8*sin(0.524), 0.1, -0.8*cos(0.524));
	vec3 p2 = vec3(-2.1*sin(0.524), 0.07, -2.1*cos(0.524));
	vec3 p3 = p2 + vec3(-0.2, 0., 0.);
	vres = min2( vres, vec2( sdCapsule( p, p1, p2, 0.18 ), 6.));
	vres = min2( vres, vec2( sdCapsule( p, p2, p3, 0.18 ), 6.1));

	// cockpit
	q = rz90*(p - p3 + vec3(0.2, 0., 0.));
	vres = min2( vres, vec2( sdConeSection( q, 0.15, 0.18, 0.08), 7. ));
	
	// side cylinders
	q = vec3( 0., 3.87, 0. );
	q = mod( rx90 * p, q ) - 0.5*q;
	vres = max2( vres, vec2( -sdTorus82( q, vec2( 0.26, 0.09 )), 8.));

	// exhaust ports
	p1 = vec3(0.75, 0., 0.);
	p2 = vec3(0.45, 0., 0.);
	q = p - p1;
	vres = smin( vres, vec2( sdCappedCylinder( q, vec2( 0.14, 0.255 )), 9.), 0.044);
	q -= p2;
	vres = smin( vres, vec2( sdCappedCylinder( q, vec2( 0.14, 0.225 )), 9.), 0.044);
	q = p;
	q.z = -abs(q.z); // reflect
	q = rotY(0.45)*q - p1;
	vres = smin( vres, vec2( sdCappedCylinder( q, vec2( 0.14, 0.255 )), 9.), 0.044);
	q -= p2;
	vres = smin( vres, vec2( sdCappedCylinder( q, vec2( 0.14, 0.225 )), 9.), 0.044);

	// gun port
	q = p + vec3(0.22, 0., 0.);
	vres = min2( vres, vec2( sdCappedCylinder( q, vec2( 0.14, 0.32 )), 9.));
	
	// gun
	p1 = vec3(-0.22, 0., 0.);
	p2 = p1 + vec3( 0., 0.35, 0.0);
	p3 = vec3(-0.22, 0.33, 0.03);
	vec3 p4 = p3 + vec3(-0.25, 0.04, 0.);
	vec3 p5 = vec3(0., 0.03, 0.);
	q = p;
	q.z = abs(q.z);
	q.y = abs(q.y);
	d1 = sdCapsule( q, p1, p2, 0.06 ); // gun pod
	d1 = min( d1, sdCapsule( q, p3, p4, 0.01 )); // gun 1
	d1 = min( d1, sdCapsule( q, p3 + p5, p4 + p5, 0.01 )); // gun 2
	vres = min2( vres, vec2( d1, 10.));
	
	// upper dish
	p1 = vec3( -1.1, 0.4, 0.83 );
	p2 = p1 - vec3( -0.05, 0.0, 0.0 );
	p3 = p2 - vec3( -0.1, 0.25, 0.0 );
	q = p - p1;
	d1 = sdSphere( q, 0.2 );
	q = q + vec3( 0.75, -0.1, 0.0 );
	d2 = sdSphere( q, 0.8 );
	d3 = sdSphere( q, 0.81 );
	d4 = sdCapsule( p, p2, p3, 0.03 );
	vres = min2( vres, vec2( min( d4, max( d3, max( -d2, d1 ))), 10.));
	
	return vres;
}

// ray marching and rendering

vec2 shipCastRay( in vec3 ro, in vec3 rd )
{
	float tmin = 1.0;
	float tmax = 20.0;
	
	const float precis = 0.001;
	float t = tmin;
	float m = -1.0;
	for( int i=0; i<50; i++ )
	{
		vec2 res = shipDE( ro+rd*t );
		if( res.x<precis || t>tmax ) break;
		t += res.x;
		m = res.y;
	}

	if ( t>tmax ) m=-1.0;
	return vec2( t, m );
}


float softshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
	float res = 1.0;
	float t = mint;
	for( int i=0; i<16; i++ )
	{
		float h = shipDE( ro + rd*t ).x;
		res = min( res, 8.0*h/t );
		t += clamp( h, 0.02, 0.10 );
		if( h<0.001 || t>tmax ) break;
	}
	return clamp( res, 0.0, 1.0 );
}

vec3 calcNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
		shipDE(pos+eps.xyy).x - shipDE(pos-eps.xyy).x,
		shipDE(pos+eps.yxy).x - shipDE(pos-eps.yxy).x,
		shipDE(pos+eps.yyx).x - shipDE(pos-eps.yyx).x );
	return normalize(nor);
}

float calcAO( in vec3 pos, in vec3 nor )
{
	float occ = 0.0;
	float sca = 1.0;
	for( int i=0; i<5; i++ )
	{
		float hr = 0.01 + 0.12*float(i)/4.0;
		vec3 aopos =  nor * hr + pos;
		float dd = shipDE( aopos ).x;
		occ -= (dd-hr)*sca;
		sca *= 0.95;
	}
	return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    
}

vec3 shipRender( in vec3 ro, in vec3 rd, in vec3 col )
{ 
	vec2 res = shipCastRay(ro,rd);
	float t = res.x;
	float m = res.y;
	if( m >-0.5 )
	{
		vec3 pos = ro + t*rd;
		vec3 nor = calcNormal( pos );
		vec3 ref = reflect( rd, nor );
		vec2 uv = vec2(0.);
		float luma = 0.;
		
		vec3 shipCol = 0.9*vec3(0.729, 0.694, 0.627);//vec3(0.5, 0.45, 0.45);

		vec3 q = shipw2l( pos );
		bool isExhaust = false;
		
		if ( abs(q.y) < 0.06 && m < 3.5 )
		{
			if (q.x < 1.6)
				col = mix(vec3(0.1), shipCol, 0.5 * /*texture( iChannel0, 0.2*q.xy ).x*/pNoise(0.2*q.xy));
			else
				isExhaust = true;
		} else if ( m < 1.1 ) // main disk
		{
			col = texHull( q );
		} else if ( m < 2.1 ) // centre pylon
		{
			q = 1.5*(q + vec3(0.22, 0., 0.));
			uv.x = 0.5*(atan(q.z, q.x)/PI);
			uv.y = 1. - length(q.xz);
			luma = texPlates( uv );
			col = luma * shipCol;
		} else if ( m < 3.1 ) // front forks
		{
			uv.x = 0.3*abs(q.z);
			uv.y = 0.25*q.x+0.4;
			luma = texPlates( uv );
			col = luma * shipCol;
		} else if ( m < 4.1 ) // Z crossbar
		{
			uv.x = 0.5*q.x;
			uv.y = -0.8*abs(q.z);
			float luma = texPlates( uv );
			col = luma * shipCol;
		} else if ( m < 5.1 ) // X crossbar
		{
			uv.x = 0.5*q.z;
			uv.y = q.x;
			if (abs(q.z) < 0.12 && abs(q.x) > 0.8 && abs(q.x) < 2.3)
			{
				uv.y *= 0.5;
				col = shipCol * mix( 0.5, 1., 0.5 * /*texture( iChannel0, 0.2*uv ).x*/pNoise(0.2*uv));
			} else
				col = shipCol * texPlates( uv );
		} else if ( m < 6.6 ) // cockpit walkway
		{
			uv = rot(0.524)*q.xz;
			uv.x = 0.5*uv.x;
			uv.y = uv.y;
			luma = texPlates( uv );
			col = luma * shipCol;
		} else if ( m < 7.1 ) // cockpit
		{
			if ( q.y > 0.12 )
				col = vec3(0.03);
			else
				col = shipCol;
		} else if ( m < 8.1 ) // side ports
		{
			col = shipCol;
		} else if ( m < 9.9 ) // exhaust ports and gun port
		{
			col = vec3(0.05);
		} else
		{
			col = shipCol;
		}

		if ( isExhaust )
		{
			// ship exhaust
			float blume = pow( clamp( dot(nor,-rd), 0.0, 1.0), 10. );
			col = clamp( blume + vec3( 0.215, 0.945, 1. ) * (0.5 * cos( 80.*q.z ) + 0.5), 0., 1.);
		} else
		{
			// ship hull lighting        
			float occ = calcAO( pos, nor );
			vec3  lig = sundir;
			float amb = clamp( 0.5+0.5*nor.y, 0.0, 1.0 );
			float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
			float bac = clamp( dot( nor, normalize(vec3(-lig.x,0.0,-lig.z))), 0.0, 1.0 )*clamp( 1.0-pos.y,0.0,1.0);
			float dom = smoothstep( -0.1, 0.1, ref.y );
			float fre = pow( clamp( 1.0+dot(nor,rd),0.0,1.0), 2.0 );
			float spe = pow( clamp( dot( ref, lig ), 0.0, 1.0 ), 16.0 );

			dif *= softshadow( pos, lig, 0.02, 2.5 );
			dom *= softshadow( pos, ref, 0.02, 2.5 );

			vec3 brdf = vec3(0.0);
			brdf += 1.20*dif*vec3(1.00,0.90,0.60);
			brdf += 1.10*spe*vec3(1.00,0.90,0.60)*dif;
			brdf += 0.30*amb*vec3(0.50,0.70,1.00)*occ;
			brdf += 0.40*dom*vec3(0.50,0.70,1.00)*occ;
			brdf += 0.30*bac*vec3(0.25,0.25,0.25)*occ;
			brdf += 0.40*fre*vec3(1.00,1.00,1.00)*occ;
			brdf += 0.02;
			col = clamp(col*brdf, 0.0, 1.0);
			
			// Gamma correction
			col = pow( col, vec3(0.4545) );
		}
	}

	return col;
}

void getBackground( out vec4 fragColor, in vec2 fragCoord ) 
{
	vec2 q = fragCoord.xy/iResolution.xy;
    vec2 p = -1.0+2.0*q;
    p.x *= iResolution.x/iResolution.y;
    //    vec2 md = iMouse.xy/iResolution.xy - 0.5;

    // render clouds and sky
    speed = iTime * SPEED;
    vec3 ro = 4.0*normalize(vec3( 0.0, 0.3, 5.0));
    vec3 ta = vec3(0.0, -1.0, 0.0);
    mat3 ca = setCamera( ta - ro );
    vec3 rd = ca * normalize( vec3(p.xy,1.0) );
    vec3 col = cloudRender( ro, rd );
    // animate
	time = iTime;
    shippos = shippath( time );
    vec3 shipv = shipvel( time );
    vec3 shipa = shipacc( time );
    // camera	
    //ro = rotY(2.*PI*md.x) * rotX(-0.99*PI*md.y) * vec3( 0.0, 0.0, 9.);
    ro = vec3( 0.0, 0.0, 5.*sin( 0.5*time ) + 13.);
    // camera-to-world transformation
    ca = setCamera( ta - ro );
    // ship direction
    shiptilt = rot( 0.4*shipa.x );
    shipltow = ry90 * setCamera( -normalize( shipv ) );
    // ray direction
	rd = ca * normalize( vec3(p.xy, 1.0) );
    // render ship
    col = shipRender( ro, rd, col );
    fragColor = vec4(col, 1.);
}
#endif //__BESPIN__

void main()
{
	getBackground( gl_FragColor, (vec2(1.0)-m_TexCoords) * u_Dimensions.xy );
	out_Glow = vec4(0.0);
	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
}
