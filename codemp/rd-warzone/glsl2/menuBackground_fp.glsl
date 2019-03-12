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


//#define __BOKEH__
//#define __MOUNTAINS__
//#define __RED_PLANET__
#define __GREEN_PLANET__
//#define __TIE_FIGHTERS__
//#define __HYPERSPACE__
//#define __HYPERSPACE2__


#ifdef __BOKEH__
// A camera. Has a position and a direction. 
struct Camera {
    vec3 pos;
    vec3 dir;
};
    
// A ray. Has origin + direction.
struct Ray {
    vec3 origin;
    vec3 dir;
};
    
// A disk. Has position, size, colour.
struct Disk {
    vec3 pos;
    float radius;
    vec3 col;
};
        
vec4 intersectDisk(in Ray ray, in Disk disk, in float focalPoint) {
	// Move ray to Z plane of disk
	ray.origin += ray.dir * disk.pos.z;

	// Find distance from ray to disk (only xy needs considering since they have equal Z)
	float dist = length(ray.origin.xy - disk.pos.xy);

	// blur depends on distance from focal point
	float blurRadius = abs(focalPoint - disk.pos.z) * 0.1;

	// Calculate alpha component, using blur radius and disk radius
	float alpha = 1. - smoothstep(max(0., disk.radius - blurRadius), disk.radius + blurRadius, dist);

	// Limit to 50% opacity
	alpha *= 0.3;

	// Pre-multiply by alpha and return
	return vec4(disk.col * alpha, alpha);
}

// Normalised random number, borrowed from Hornet's noise distributions: https://www.shadertoy.com/view/4ssXRX
float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

void getBackground( out vec4 fragColor, in vec2 fragCoord )
{
    // We'll need a camera. And some perspective.
    
	// Get some coords for the camera angle from the frag coords. Convert to -1..1 range.
    vec2 uv = fragCoord.xy / iResolution.xy;
    uv = uv * 2. - 1.;
    
    // Aspect correction so we don't get oval bokeh
    uv.y *= iResolution.y/iResolution.x;
    
    // Make a camera at 0,0,0 pointing forwards
    Camera cam = Camera(vec3(0, 0, 0.), vec3(0, 0, 1));
                        
    // Find the ray direction. Simple in this case.
    Ray ray = Ray(cam.pos, normalize(cam.dir + vec3(uv, 0)));
    
    // Cast the ray into the scene, intersect it with bokeh disks.
    // I'm using a float since the loop is simple and it avoids a cast (costly on some platforms)
    const float diskCount = 100.;
    
    // Set the focal point
    float focalPoint = 2.0;
    
    // Create an empty colour
    vec4 col = vec4(0.);
    
    float time = iTime * 0.1;
    
    for (float i=0.0; i<diskCount; i++) {
        // random disk position
        vec3 diskPos = vec3(
            sin(i*(nrand(vec2(i-3., i + 1.)) + 1.) + time),
            sin(i*(nrand(vec2(i-2., i + 2.)) + 2.) + time * 0.9), 
            sin(i*(nrand(vec2(i-1., i + 3.)) + 2.) + time * 0.9) * 5. + 5.5
            );
        
        // Scale x+y by z so it fills the space a bit more nicely
        diskPos.xy *= diskPos.z*0.7;
        
        // random disk colour
        vec3 diskCol = vec3(
            sin(i) * 0.25 + 0.75,
            sin(i + 4.) * 0.25 + 0.55,
            sin(i + 8.) * 0.25 + 0.65
        );
        
        // random disk size
        float diskSize = nrand(vec2(i)) * 0.2 + 0.1;
        
        // create the disk
        Disk disk = Disk(diskPos, diskSize, diskCol);
        
        // Intersect the disk
        vec4 result = intersectDisk(ray, disk, focalPoint);
        
        // Add the colour in
       col += result;
    }
    
	fragColor = vec4(col.rgb, 1.0);
}
#endif //__BOKEH__

#ifdef __MOUNTAINS__
// Mountains. By David Hoskins - 2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// https://www.shadertoy.com/view/4slGD4
// A ray-marched version of my terrain renderer which uses
// streaming texture normals for speed:-
// http://www.youtube.com/watch?v=qzkBnCBpQAM

// It uses binary subdivision to accurately find the height map.
// Lots of thanks to Inigo and his noise functions!

// Video of my OpenGL version that 
// http://www.youtube.com/watch?v=qzkBnCBpQAM

// Stereo version code thanks to Croqueteer :)
//#define STEREO 

float treeLine = 0.0;
float treeCol = 0.0;


vec3 sunLight  = normalize( vec3(  0.4, 0.4,  0.48 ) );
vec3 sunColour = vec3(1.0, .9, .83);
float specular = 0.0;
vec3 cameraPos;
float ambient;
vec2 add = vec2(1.0, 0.0);
#define HASHSCALE1 .1031
#define HASHSCALE3 vec3(.1031, .1030, .0973)
#define HASHSCALE4 vec4(1031, .1030, .0973, .1099)

// This peturbs the fractal positions for each iteration down...
// Helps make nice twisted landscapes...
const mat2 rotate2D = mat2(1.3623, 1.7531, -1.7131, 1.4623);

// Alternative rotation:-
// const mat2 rotate2D = mat2(1.2323, 1.999231, -1.999231, 1.22);


//  1 out, 2 in...
float Hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}
vec2 Hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}

float Noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    
    float res = mix(mix( Hash12(p),          Hash12(p + add.xy),f.x),
                    mix( Hash12(p + add.yx), Hash12(p + add.xx),f.x),f.y);
    return res;
}

vec2 Noise2( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y * 57.0;
   vec2 res = mix(mix( Hash22(p),          Hash22(p + add.xy),f.x),
                  mix( Hash22(p + add.yx), Hash22(p + add.xx),f.x),f.y);
    return res;
}

//--------------------------------------------------------------------------
float Trees(vec2 p)
{
	
 	//return (texture(iChannel1,0.04*p).x * treeLine);
    return Noise(p*13.0)*treeLine;
}


//--------------------------------------------------------------------------
// Low def version for ray-marching through the height field...
// Thanks to IQ for all the noise stuff...

float Terrain( in vec2 p)
{
	vec2 pos = p*0.05;
	float w = (Noise(pos*.25)*0.75+.15);
	w = 66.0 * w * w;
	vec2 dxy = vec2(0.0, 0.0);
	float f = .0;
	for (int i = 0; i < 5; i++)
	{
		f += w * Noise(pos);
		w = -w * 0.4;	//...Flip negative and positive for variation
		pos = rotate2D * pos;
	}
	float ff = Noise(pos*.002);
	
	f += pow(abs(ff), 5.0)*275.-5.0;
	return f;
}

//--------------------------------------------------------------------------
// Map to lower resolution for height field mapping for Scene function...
float Map(in vec3 p)
{
	float h = Terrain(p.xz);
		

	float ff = Noise(p.xz*.3) + Noise(p.xz*3.3)*.5;
	treeLine = smoothstep(ff, .0+ff*2.0, h) * smoothstep(1.0+ff*3.0, .4+ff, h) ;
	treeCol = Trees(p.xz);
	h += treeCol;
	
    return p.y - h;
}

//--------------------------------------------------------------------------
// High def version only used for grabbing normal information.
float Terrain2( in vec2 p)
{
	// There's some real magic numbers in here! 
	// The Noise calls add large mountain ranges for more variation over distances...
	vec2 pos = p*0.05;
	float w = (Noise(pos*.25)*0.75+.15);
	w = 66.0 * w * w;
	vec2 dxy = vec2(0.0, 0.0);
	float f = .0;
	for (int i = 0; i < 5; i++)
	{
		f += w * Noise(pos);
		w =  - w * 0.4;	//...Flip negative and positive for varition	   
		pos = rotate2D * pos;
	}
	float ff = Noise(pos*.002);
	f += pow(abs(ff), 5.0)*275.-5.0;
	

	treeCol = Trees(p);
	f += treeCol;
	if (treeCol > 0.0) return f;

	
	// That's the last of the low resolution, now go down further for the Normal data...
	for (int i = 0; i < 6; i++)
	{
		f += w * Noise(pos);
		w =  - w * 0.4;
		pos = rotate2D * pos;
	}
	
	
	return f;
}

//--------------------------------------------------------------------------
float FractalNoise(in vec2 xy)
{
	float w = .7;
	float f = 0.0;

	for (int i = 0; i < 4; i++)
	{
		f += Noise(xy) * w;
		w *= 0.5;
		xy *= 2.7;
	}
	return f;
}

//--------------------------------------------------------------------------
// Simply Perlin clouds that fade to the horizon...
// 200 units above the ground...
vec3 GetClouds(in vec3 sky, in vec3 rd)
{
	if (rd.y < 0.01) return sky;
	float v = (200.0-cameraPos.y)/rd.y;
	rd.xz *= v;
	rd.xz += cameraPos.xz;
	rd.xz *= .010;
	float f = (FractalNoise(rd.xz) -.55) * 5.0;
	// Uses the ray's y component for horizon fade of fixed colour clouds...
	sky = mix(sky, vec3(.55, .55, .52), clamp(f*rd.y-.1, 0.0, 1.0));

	return sky;
}



//--------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
vec3 GetSky(in vec3 rd)
{
	float sunAmount = max( dot( rd, sunLight), 0.0 );
	float v = pow(1.0-max(rd.y,0.0),5.)*.5;
	vec3  sky = vec3(v*sunColour.x*0.4+0.18, v*sunColour.y*0.4+0.22, v*sunColour.z*0.4+.4);
	// Wide glare effect...
	sky = sky + sunColour * pow(sunAmount, 6.5)*.32;
	// Actual sun...
	sky = sky+ sunColour * min(pow(sunAmount, 1150.0), .3)*.65;
	return sky;
}

//--------------------------------------------------------------------------
// Merge mountains into the sky background for correct disappearance...
vec3 ApplyFog( in vec3  rgb, in float dis, in vec3 dir)
{
	float fogAmount = exp(-dis* 0.00005);
	return mix(GetSky(dir), rgb, fogAmount );
}

//--------------------------------------------------------------------------
// Calculate sun light...
void DoLighting(inout vec3 mat, in vec3 pos, in vec3 normal, in vec3 eyeDir, in float dis)
{
	float h = dot(sunLight,normal);
	float c = max(h, 0.0)+ambient;
	mat = mat * sunColour * c ;
	// Specular...
	if (h > 0.0)
	{
		vec3 R = reflect(sunLight, normal);
		float specAmount = pow( max(dot(R, normalize(eyeDir)), 0.0), 3.0)*specular;
		mat = mix(mat, sunColour, specAmount);
	}
}

//--------------------------------------------------------------------------
// Hack the height, position, and normal data to create the coloured landscape
vec3 TerrainColour(vec3 pos, vec3 normal, float dis)
{
	vec3 mat;
	specular = .0;
	ambient = .1;
	vec3 dir = normalize(pos-cameraPos);
	
	vec3 matPos = pos * 2.0;// ... I had change scale halfway though, this lazy multiply allow me to keep the graphic scales I had

	float disSqrd = dis * dis;// Squaring it gives better distance scales.

	float f = clamp(Noise(matPos.xz*.05), 0.0,1.0);//*10.8;
	f += Noise(matPos.xz*.1+normal.yz*1.08)*.85;
	f *= .55;
	vec3 m = mix(vec3(.63*f+.2, .7*f+.1, .7*f+.1), vec3(f*.43+.1, f*.3+.2, f*.35+.1), f*.65);
	mat = m*vec3(f*m.x+.36, f*m.y+.30, f*m.z+.28);
	// Should have used smoothstep to add colours, but left it using 'if' for sanity...
	if (normal.y < .5)
	{
		float v = normal.y;
		float c = (.5-normal.y) * 4.0;
		c = clamp(c*c, 0.1, 1.0);
		f = Noise(vec2(matPos.x*.09, matPos.z*.095+matPos.yy*0.15));
		f += Noise(vec2(matPos.x*2.233, matPos.z*2.23))*0.5;
		mat = mix(mat, vec3(.4*f), c);
		specular+=.1;
	}

	// Grass. Use the normal to decide when to plonk grass down...
	if (matPos.y < 45.35 && normal.y > .65)
	{

		m = vec3(Noise(matPos.xz*.023)*.5+.15, Noise(matPos.xz*.03)*.6+.25, 0.0);
		m *= (normal.y- 0.65)*.6;
		mat = mix(mat, m, clamp((normal.y-.65)*1.3 * (45.35-matPos.y)*0.1, 0.0, 1.0));
	}

	if (treeCol > 0.0)
	{
		mat = vec3(.02+Noise(matPos.xz*5.0)*.03, .05, .0);
		normal = normalize(normal+vec3(Noise(matPos.xz*33.0)*1.0-.5, .0, Noise(matPos.xz*33.0)*1.0-.5));
		specular = .0;
	}
	
	// Snow topped mountains...
	if (matPos.y > 80.0 && normal.y > .42)
	{
		float snow = clamp((matPos.y - 80.0 - Noise(matPos.xz * .1)*28.0) * 0.035, 0.0, 1.0);
		mat = mix(mat, vec3(.7,.7,.8), snow);
		specular += snow;
		ambient+=snow *.3;
	}
	// Beach effect...
	if (matPos.y < 1.45)
	{
		if (normal.y > .4)
		{
			f = Noise(matPos.xz * .084)*1.5;
			f = clamp((1.45-f-matPos.y) * 1.34, 0.0, .67);
			float t = (normal.y-.4);
			t = (t*t);
			mat = mix(mat, vec3(.09+t, .07+t, .03+t), f);
		}
		// Cheap under water darkening...it's wet after all...
		if (matPos.y < 0.0)
		{
			mat *= .2;
		}
	}

	DoLighting(mat, pos, normal,dir, disSqrd);
	
	// Do the water...
	if (matPos.y < 0.0)
	{
		// Pull back along the ray direction to get water surface point at y = 0.0 ...
		float time = (iTime)*.03;
		vec3 watPos = matPos;
		watPos += -dir * (watPos.y/dir.y);
		// Make some dodgy waves...
		float tx = cos(watPos.x*.052) *4.5;
		float tz = sin(watPos.z*.072) *4.5;
		vec2 co = Noise2(vec2(watPos.x*4.7+1.3+tz, watPos.z*4.69+time*35.0-tx));
		co += Noise2(vec2(watPos.z*8.6+time*13.0-tx, watPos.x*8.712+tz))*.4;
		vec3 nor = normalize(vec3(co.x, 20.0, co.y));
		nor = normalize(reflect(dir, nor));//normalize((-2.0*(dot(dir, nor))*nor)+dir);
		// Mix it in at depth transparancy to give beach cues..
        tx = watPos.y-matPos.y;
		mat = mix(mat, GetClouds(GetSky(nor)*vec3(.3,.3,.5), nor)*.1+vec3(.0,.02,.03), clamp((tx)*.4, .6, 1.));
		// Add some extra water glint...
        mat += vec3(.1)*clamp(1.-pow(tx+.5, 3.)*texture(u_DiffuseMap, watPos.xz*.1, -2.).x, 0.,1.0);
		float sunAmount = max( dot(nor, sunLight), 0.0 );
		mat = mat + sunColour * pow(sunAmount, 228.5)*.6;
        vec3 temp = (watPos-cameraPos*2.)*.5;
        disSqrd = dot(temp, temp);
	}
	mat = ApplyFog(mat, disSqrd, dir);
	return mat;
}

//--------------------------------------------------------------------------
float BinarySubdivision(in vec3 rO, in vec3 rD, vec2 t)
{
	// Home in on the surface by dividing by two and split...
    float halfwayT;
  
    for (int i = 0; i < 5; i++)
    {

        halfwayT = dot(t, vec2(.5));
        float d = Map(rO + halfwayT*rD); 
         t = mix(vec2(t.x, halfwayT), vec2(halfwayT, t.y), step(0.5, d));

    }
	return halfwayT;
}

//--------------------------------------------------------------------------
bool Scene(in vec3 rO, in vec3 rD, out float resT, in vec2 fragCoord )
{
    float t = 1. + Hash12(fragCoord.xy)*5.;
	float oldT = 0.0;
	float delta = 0.0;
	bool fin = false;
	bool res = false;
	vec2 distances;
	for( int j=0; j< 150; j++ )
	{
		if (fin || t > 240.0) break;
		vec3 p = rO + t*rD;
		//if (t > 240.0 || p.y > 195.0) break;
		float h = Map(p); // ...Get this positions height mapping.
		// Are we inside, and close enough to fudge a hit?...
		if( h < 0.5)
		{
			fin = true;
			distances = vec2(oldT, t);
			break;
		}
		// Delta ray advance - a fudge between the height returned
		// and the distance already travelled.
		// It's a really fiddly compromise between speed and accuracy
		// Too large a step and the tops of ridges get missed.
		delta = max(0.01, 0.3*h) + (t*0.0065);
		oldT = t;
		t += delta;
	}
	if (fin) resT = BinarySubdivision(rO, rD, distances);

	return fin;
}

//--------------------------------------------------------------------------
vec3 CameraPath( float t )
{
	float m = 1.0;//+(iMouse.x/iResolution.x)*300.0;
	t = (iTime*1.5+m+657.0)*.006 + t;
    vec2 p = 476.0*vec2( sin(3.5*t), cos(1.5*t) );
	return vec3(35.0-p.x, 0.6, 4108.0+p.y);
}

//--------------------------------------------------------------------------
// Some would say, most of the magic is done in post! :D
vec3 PostEffects(vec3 rgb, vec2 uv)
{
	//#define CONTRAST 1.1
	//#define SATURATION 1.12
	//#define BRIGHTNESS 1.3
	//rgb = pow(abs(rgb), vec3(0.45));
	//rgb = mix(vec3(.5), mix(vec3(dot(vec3(.2125, .7154, .0721), rgb*BRIGHTNESS)), rgb*BRIGHTNESS, SATURATION), CONTRAST);
	rgb = (1.0 - exp(-rgb * 6.0)) * 1.0024;
	//rgb = clamp(rgb+hash12(fragCoord.xy*rgb.r)*0.1, 0.0, 1.0);
	return rgb;
}

//--------------------------------------------------------------------------
void getBackground( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 xy = -1.0 + 2.0*fragCoord.xy / iResolution.xy;
	vec2 uv = xy * vec2(iResolution.x/iResolution.y,1.0);
	vec3 camTar;

	#ifdef STEREO
	float isCyan = mod(fragCoord.x + mod(fragCoord.y,2.0),2.0);
	#endif

	// Use several forward heights, of decreasing influence with distance from the camera.
	float h = 0.0;
	float f = 1.0;
	for (int i = 0; i < 7; i++)
	{
		h += Terrain(CameraPath((.6-f)*.008).xz) * f;
		f -= .1;
	}
	cameraPos.xz = CameraPath(0.0).xz;
	camTar.xyz	 = CameraPath(.005).xyz;
	camTar.y = cameraPos.y = max((h*.25)+3.5, 1.5+sin(iTime*5.)*.5);
	
	float roll = 0.15*sin(iTime*.2);
	vec3 cw = normalize(camTar-cameraPos);
	vec3 cp = vec3(sin(roll), cos(roll),0.0);
	vec3 cu = normalize(cross(cw,cp));
	vec3 cv = normalize(cross(cu,cw));
	vec3 rd = normalize( uv.x*cu + uv.y*cv + 1.5*cw );

	#ifdef STEREO
	cameraPos += .45*cu*isCyan; // move camera to the right - the rd vector is still good
	#endif

	vec3 col;
	float distance;
	if( !Scene(cameraPos,rd, distance, fragCoord) )
	{
		// Missed scene, now just get the sky value...
		col = GetSky(rd);
		col = GetClouds(col, rd);
	}
	else
	{
		// Get world coordinate of landscape...
		vec3 pos = cameraPos + distance * rd;
		// Get normal from sampling the high definition height map
		// Use the distance to sample larger gaps to help stop aliasing...
		float p = min(.3, .0005+.00005 * distance*distance);
		vec3 nor  	= vec3(0.0,		    Terrain2(pos.xz), 0.0);
		vec3 v2		= nor-vec3(p,		Terrain2(pos.xz+vec2(p,0.0)), 0.0);
		vec3 v3		= nor-vec3(0.0,		Terrain2(pos.xz+vec2(0.0,-p)), -p);
		nor = cross(v2, v3);
		nor = normalize(nor);

		// Get the colour using all available data...
		col = TerrainColour(pos, nor, distance);
	}

	col = PostEffects(col, uv);
	
	#ifdef STEREO	
	col *= vec3( isCyan, 1.0-isCyan, 1.0-isCyan );	
	#endif
	
	fragColor=vec4(col,1.0);
}
#endif //__MOUNTAINS__

#ifdef __RED_PLANET__
//Sirenian Dawn by nimitz (twitter: @stormoid)

#define ITR 30
#define FAR 400.
#define time iTime

#define HASHSCALE1 .1031

const vec3 lgt = vec3(-.523, .41, -.747);
mat2 m2 = mat2( 0.80,  0.60, -0.60,  0.80 );

float noise(vec2 pos)
{
	vec2 p = pos * 128.0;
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//form iq, see: http://www.iquilezles.org/www/articles/morenoise/morenoise.htm
vec3 noised( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    vec2 u = f*f*(3.0-2.0*f);
	float a = noise((p+vec2(0.5,0.5))/256.0).x;
	float b = noise((p+vec2(1.5,0.5))/256.0).x;
	float c = noise((p+vec2(0.5,1.5))/256.0).x;
	float d = noise((p+vec2(1.5,1.5))/256.0).x;
	return vec3(a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y,
				6.0*f*(1.0-f)*(vec2(b-a,c-a)+(a-b-c+d)*u.yx));
}

float terrain( in vec2 p)
{
    float rz = 0.;
    float z = 1.;
	vec2  d = vec2(0.0);
    float scl = 2.95;
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
    vec2 e = vec2(-1., 1.)*0.0005*pow(ds,1.);
	return normalize(e.yxx*map(p + e.yxx) + e.xxy*map(p + e.xxy) + 
					 e.xyx*map(p + e.xyx) + e.yyy*map(p + e.yyy) );   
}

float fbm(in vec2 p)
{	
	float z=.5;
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

//Based on: http://www.iquilezles.org/www/articles/fog/fog.htm
vec3 fog(vec3 ro, vec3 rd, vec3 col, float ds)
{
    vec3 pos = ro + rd*ds;
    float mx = (fbm(pos.zx*0.1-time*0.05)-0.5)*.02;
    
    const float b= 1.;
    float den = 0.3*exp(-ro.y*b)*(1.0-exp( -ds*rd.y*b ))/rd.y;
    float sdt = max(dot(rd, lgt), 0.);
    vec3  fogColor  = mix(vec3(0.5,0.2,0.15)*1.2, vec3(1.1,0.6,0.45)*1.3, pow(sdt,2.0)+mx*0.5);
    return mix( col, fogColor, clamp(den + mx,0.,1.) );
}

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
    col += pow(hori, 200.)*vec3(1.0, 0.7,  0.5)*3.;
    col += pow(hori, 25.)* vec3(1.0, 0.5,  0.25)*.3;
    col += pow(hori, 7.)* vec3(1.0, 0.4, 0.25)*.8;
    
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
        c += c2*(mix(vec3(1.0,0.49,0.1),vec3(0.75,0.9,1.),rn.y)*0.25+0.75);
        p *= 1.4;
    }
    return c*c*.7;
}

void getBackground( out vec4 fragColor, in vec2 fragCoord )
{	
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
    
    vec3 bg = max(stars(rd, 0.666), stars(rd, 1.0)) * (1.0-clamp(dot(scatt, vec3(1.3)),0.,1.));
    vec3 col = bg;
    
    vec3 pos = ro+rz*rd;
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
        brdf += bac*vec3(0.15,0.05,0.04);
        brdf += 2.3*dif*vec3(.9,0.4,0.25);
        col = vec3(0.25,0.25,0.3);
        float crv = curv(pos, 2.)*1.;
        float crv2 = curv(pos, .4)*2.5;
        
        col += clamp(crv*0.9,-1.,1.)*vec3(0.25,.6,.5);
        col = col*brdf + col*spe*.1 +.1*fre*col;
        col *= crv*1.+1.;
        col *= crv2*1.+1.;
    }
	
    col = fog(ro, rd, col, rz);
    col = mix(col,bg,smoothstep(FAR-150., FAR, rz));
    col += scatt;
    
    col = pow( col, vec3(0.93,1.0,1.0) );
    col = mix(col, smoothstep(0.,1.,col), 0.2);
    col *= pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1)*0.9+0.1;
    
	fragColor = vec4(col, 1.0);
}
#endif //__RED_PLANET__

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
