#define REAL_WAVES					// You probably always want this turned on.
//#define USE_REFLECTION				// Enable reflections on water. Define moved to renderer code.
#define FIX_WATER_DEPTH_ISSUES		// Use basic depth value for sky hits...
//#define EXPERIMENTAL_WATERFALL	// Experimental waterfalls...
//#define __DEBUG__
//#define USE_LIGHTING				// Use lighting in this shader? trying to handle the lighting in deferredlight now instead.
//#define USE_DETAILED_UNDERWATER		// Experimenting...

/*
heightMap – height-map used for waves generation as described in the section “Modifying existing geometry”
backBufferMap – current contents of the back buffer
positionMap – texture storing scene position vectors (material type is alpha)
waterPositionMap – texture storing scene water position vectors (alpha is 1.0 when water is at this pixel. 0.0 if not).
normalMap – texture storing normal vectors for normal mapping as described in the section “The computation of normal vectors”
foamMap – texture containing foam – in my case it is a photo of foam converted to greyscale
*/

uniform mat4		u_ModelViewProjectionMatrix;

uniform sampler2D	u_DiffuseMap;			// backBufferMap
uniform sampler2D	u_PositionMap;

uniform sampler2D	u_OverlayMap;			// foamMap 1
uniform sampler2D	u_SplatMap1;			// foamMap 2
uniform sampler2D	u_SplatMap2;			// foamMap 3
uniform sampler2D	u_SplatMap3;			// foamMap 4

uniform sampler2D	u_DetailMap;			// causics map

uniform sampler2D	u_HeightMap;			// map height map

uniform sampler2D	u_WaterPositionMap;

uniform sampler2D	u_EmissiveCubeMap;		// water reflection image... (not really a cube, just reusing using the uniform)

uniform samplerCube	u_SkyCubeMap;
uniform samplerCube	u_SkyCubeMapNight;

uniform vec4		u_Mins;					// MAP_MINS[0], MAP_MINS[1], MAP_MINS[2], 0.0
uniform vec4		u_Maxs;					// MAP_MAXS[0], MAP_MAXS[1], MAP_MAXS[2], 0.0
uniform vec4		u_MapInfo;				// MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], SUN_VISIBILITY

uniform vec4		u_Local0;				// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec4		u_Local1;				// MAP_WATER_LEVEL, USE_GLSL_REFLECTION, IS_UNDERWATER, WATER_REFLECTIVENESS
uniform vec4		u_Local2;				// WATER_COLOR_SHALLOW_R, WATER_COLOR_SHALLOW_G, WATER_COLOR_SHALLOW_B, WATER_CLARITY
uniform vec4		u_Local3;				// WATER_COLOR_DEEP_R, WATER_COLOR_DEEP_G, WATER_COLOR_DEEP_B, HAVE_HEIGHTMAP
uniform vec4		u_Local4;				// SHADER_NIGHT_SCALE, WATER_EXTINCTION1, WATER_EXTINCTION2, WATER_EXTINCTION3
uniform vec4		u_Local7;				// testshadervalue1, etc
uniform vec4		u_Local8;				// testshadervalue5, etc
uniform vec4		u_Local9;				// SUN_VISIBILITY, SHADER_NIGHT_SCALE, 0.0, 0.0
uniform vec4		u_Local10;				// waveHeight, waveDensity, USE_OCEAN, WATER_UNDERWATER_CLARITY

uniform vec2		u_Dimensions;
uniform vec4		u_ViewInfo;				// zmin, zmax, zmax / zmin

varying vec2		var_TexCoords;
varying vec3		var_ViewDir;

uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_PrimaryLightColor;

#define				MAP_WATER_LEVEL		u_Local1.r
#define				HAVE_HEIGHTMAP		u_Local3.a
#define				SHADER_NIGHT_SCALE	u_Local4.r
#define				SUN_VISIBILITY			u_Local9.r

/*uniform int			u_lightCount;
uniform vec3		u_lightPositions2[MAX_DEFERRED_LIGHTS];
uniform float		u_lightDistances[MAX_DEFERRED_LIGHTS];
uniform float		u_lightHeightScales[MAX_DEFERRED_LIGHTS];
uniform vec3		u_lightColors[MAX_DEFERRED_LIGHTS];*/

// Position of the camera
uniform vec3		u_ViewOrigin;
#define ViewOrigin	u_ViewOrigin.xzy

// Timer
uniform float		u_Time;
#define systemtimer		(u_Time * 5000.0)


// Over-all water clearness...
//const float waterClarity2 = 0.001;
const float waterClarity2 = 0.03;

#define waterClarity u_Local2.a
#define underWaterClarity u_Local10.a

// How fast will colours fade out. You can also think about this
// values as how clear water is. Therefore use smaller values (eg. 0.05f)
// to have crystal clear water and bigger to achieve "muddy" water.
const float fadeSpeed = 0.15;

// Normals scaling factor
const float normalScale = 1.0;

// R0 is a constant related to the index of refraction (IOR).
// It should be computed on the CPU and passed to the shader.
const float R0 = 0.5;

// Maximum waves amplitude
#define waveHeight (u_Local10.r * 0.5)
#define waveDensity u_Local10.g

// The smaller this value is, the more soft the transition between
// shore and water. If you want hard edges use very big value.
// Default is 1.0f.
const float shoreHardness = 0.2;//1.0;

// This value modifies current fresnel term. If you want to weaken
// reflections use bigger value. If you want to empasize them use
// value smaller then 0. Default is 0.0.
const float refractionStrength = 0.0;
//float refractionStrength = -0.3;

// Modifies 4 sampled normals. Increase first values to have more
// smaller "waves" or last to have more bigger "waves"
const vec4 normalModifier = vec4(1.0, 2.0, 4.0, 8.0);

// Describes at what depth foam starts to fade out and
// at what it is completely invisible. The third value is at
// what height foam for waves appear (+ waterLevel).
#define foamExistence vec3(8.0, 50.0, waveHeight)

const float sunScale = 3.0;

const float shininess = 0.7;
const float specularScale = 0.07;


// Colour of the water surface
//const vec3 waterColorShallow = vec3(0.0078, 0.5176, 0.7);
vec3 waterColorShallow = u_Local2.rgb;

// Colour of the water depth
//const vec3 waterColorDeep = vec3(0.0059, 0.1276, 0.18);
vec3 waterColorDeep = u_Local3.rgb;

//const vec3 extinction = vec3(35.0, 480.0, 8192.0);
vec3 extinction = u_Local4.gba;

// Water transparency along eye vector.
//const float visibility = 320.0;
const float visibility = 32.0;

// Increase this value to have more smaller waves.
const vec2 scale = vec2(0.002, 0.002);
const float refractionScale = 0.005;

#define USE_OCEAN u_Local10.b

vec2 GetMapTC(vec3 pos)
{
	vec2 mapSize = u_Maxs.xy - u_Mins.xy;
	return (pos.xy - u_Mins.xy) / mapSize;
}

float GetHeightMap(vec3 pos)
{
	if (pos.x < u_Mins.x || pos.y < u_Mins.y || pos.z < u_Mins.z) return 65536.0;
	if (pos.x > u_Maxs.x || pos.y > u_Maxs.y || pos.z > u_Maxs.z) return 65536.0;

	float h = textureLod(u_HeightMap, GetMapTC(pos), 1.0).r;
	
//#define heightBlurPasses 4.0
//	for (int i = 1; i < int(heightBlurPasses); i++)
//	{
//		h += textureLod(u_HeightMap, GetMapTC(pos), pow(2.0, float(i))).r;
//	}
//	h /= heightBlurPasses;
	
	h = mix(u_Mins.z, u_Maxs.z, h);
	return pos.z - h;
}

vec3 GetCausicMap(vec3 m_vertPos)
{
	vec2 coord = (m_vertPos.xy * m_vertPos.z * 0.000025) + ((u_Time * 0.1) * normalize(m_vertPos).xy * 0.5);
	return texture(u_DetailMap, coord).rgb;
}

vec4 GetWhiteCapMap(vec3 m_vertPos)
{
	vec2 coord = (m_vertPos.xy * 0.005) + ((u_Time * 0.1) * normalize(m_vertPos).xy * 0.5);
	return texture(u_SplatMap3, coord);
}

vec4 GetBreakingMap(vec3 m_vertPos)
{
	vec2 coord = (m_vertPos.xy * 0.005) + ((u_Time * 3.0/*0.1*/) * normalize(m_vertPos).xy * 0.5);
	return texture(u_SplatMap1, coord);
}

vec4 GetFoamMap(vec3 m_vertPos)
{
	vec2 coord = (m_vertPos.xy * 0.005) + ((u_Time * 0.1) * normalize(m_vertPos).xy * 0.5);

	vec2 xy = GetMapTC(m_vertPos);
	float x = xy.x;
	float y = xy.y;

	vec4 tex0 = texture(u_OverlayMap, coord);
	vec4 tex1 = texture(u_SplatMap1, coord);
	vec4 tex2 = texture(u_SplatMap2, coord);
	vec4 tex3 = texture(u_SplatMap3, coord);

	vec4 splatColor1 = mix(tex0, tex1, x);
	vec4 splatColor2 = mix(tex2, tex3, y);

	vec4 splatColor = mix(splatColor1, splatColor2, (x+y)*0.5);
	return splatColor;
}

vec4 positionMapAtCoord ( vec2 coord )
{
	return texture(u_PositionMap, coord).xzyw;
}

vec4 waterMapLowerAtCoord ( vec2 coord )
{
	vec4 wmap = texture(u_WaterPositionMap, coord).xzyw;

	if (u_Local1.b <= 0.0) 
		wmap.y = max(wmap.y, MAP_WATER_LEVEL);

	wmap.y -= waveHeight;
	return wmap;
}

vec4 waterMapUpperAtCoord ( vec2 coord )
{
	vec4 wmap = texture(u_WaterPositionMap, coord).xzyw;

	if (u_Local1.b <= 0.0) 
		wmap.y = max(wmap.y, MAP_WATER_LEVEL);

	//wmap.y += waveHeight;
	return wmap;
}

float hash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float noise(in vec3 o) 
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
    float f;
    f  = 0.5000*noise( p ); p = m*p*2.02;
    f += 0.2500*noise( p ); 
	
    return f * (1.0 / (0.5000 + 0.2500));
}

float pw = (1.0/u_Dimensions.x);
float ph = (1.0/u_Dimensions.y);

#if defined(USE_REFLECTION) && !defined(__LQ_MODE__)

vec3 Vibrancy ( vec3 origcolor, float vibrancyStrength )
{
	vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);  				//Calculate luma with these values
	float	max_color = max(origcolor.r, max(origcolor.g,origcolor.b)); 	//Find the strongest color
	float	min_color = min(origcolor.r, min(origcolor.g,origcolor.b)); 	//Find the weakest color
	float	color_saturation = max_color - min_color; 						//Saturation is the difference between min and max
	float	luma = dot(lumCoeff, origcolor.rgb); 							//Calculate luma (grey)
	return mix(vec3(luma), origcolor.rgb, (1.0 + (vibrancyStrength * (1.0 - (sign(vibrancyStrength) * color_saturation))))); 	//Extrapolate between luma and original by 1 + (1-saturation) - current
}

vec3 AddReflection(vec2 coord, vec3 positionMap, vec3 surfacePoint, vec3 inColor, float height, float dt)
{
	/*if (positionMap.y > surfacePoint.y)
	{
		return inColor;
	}*/

	float wMapCheck = texture(u_WaterPositionMap, vec2(coord.x, 1.0)).a;
	if (wMapCheck > 0.0)
	{// Top of screen pixel is water, don't check...
		return inColor;
	}

	// Offset the final pixel based on the height of the wave at that point, to create randomization...
	float hoff = height * 4.0 - 2.0;//* 2.0 - 1.0;
	float offset = hoff * (0.3 * waveHeight * pw);

	vec2 finalPosition = clamp(vec2(coord.x + offset, coord.y), 0.0, 1.0);

	vec4 landColor = vec4(0.0);

	float blurPixels = 0.0;
	for (float x = -2.0; x < 2.0; x += 1.0)
	{
		for (float y = -2.0; y < 2.0; y += 1.0)
		{
			vec2 offset2 = vec2(x * pw * 4.0, y * ph * 4.0);
			vec4 thisColor = textureLod(u_EmissiveCubeMap, finalPosition + offset2, 0.0);
			thisColor.a -= 1.0;

			if (thisColor.a > 0.0)
			{
				landColor += thisColor;
				blurPixels += 1.0;
			}
		}
	}

	if (blurPixels <= 0.0)
	{
		return inColor.rgb;
	}

	landColor /= blurPixels;

	landColor.rgb = Vibrancy( landColor.rgb, 1.25 );

	return mix(inColor.rgb, landColor.rgb, vec3(landColor.a * u_Local1.a * dt));
}
#endif //defined(USE_REFLECTION) && !defined(__LQ_MODE__)

// PI is a mathematical constant relating the ratio of a circle's circumference (distance around
// the edge) to its diameter (distance between two points opposite on the edge).  
// Change pi at your own peril, with your own apologies to God.
const float PI	 	= 3.14159265358;

// Can you explain these epsilons to a wide graphics audience?  YOUR comment could go here.
const float EPSILON	= 1e-3;
#define  EPSILON_NRM	(4.0/*0.5*/ / u_Dimensions.x)

// Constant indicaing the number of steps taken while marching the light ray.  
const int NUM_STEPS = 6;

//Constants relating to the iteration of the heightmap for the wave, another part of the rendering
//process.
const int ITER_GEOMETRY = 2;
const int ITER_FRAGMENT =5;

// Constants that represent physical characteristics of the sea, can and should be changed and 
//  played with
const float SEA_HEIGHT = 0.5;
const float SEA_CHOPPY = 3.0;
const float SEA_SPEED = 1.9;
const float SEA_FREQ = 0.24;
//const vec3 SEA_BASE = vec3(0.11,0.19,0.22);
//const vec3 SEA_WATER_COLOR = vec3(0.55,0.9,0.7);
#define SEA_TIME (u_Time * SEA_SPEED)

vec3 SEA_BASE = u_Local3.rgb;
vec3 SEA_WATER_COLOR = u_Local2.rgb;

/*
vec3 waterColorShallow = u_Local2.rgb;
// Colour of the water depth
vec3 waterColorDeep = u_Local3.rgb;
*/

//Matrix to permute the water surface into a complex, realistic form
mat2 octave_m = mat2(1.7,1.2,-1.2,1.4);

//CaliCoastReplay :  These HSV/RGB translation functions are
//from http://gamedev.stackexchange.com/questions/59797/glsl-shader-change-hue-saturation-brightness
//This one converts red-green-blue color to hue-saturation-value color
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

//CaliCoastReplay :  These HSV/RGB translation functions are
//from http://gamedev.stackexchange.com/questions/59797/glsl-shader-change-hue-saturation-brightness
//This one converts hue-saturation-value color to red-green-blue color
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// bteitler: A 2D hash function for use in noise generation that returns range [0 .. 1].  You could
// use any hash function of choice, just needs to deterministic and return
// between 0 and 1, and also behave randomly.  Googling "GLSL hash function" returns almost exactly 
// this function: http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
// Performance is a real consideration of hash functions since ray-marching is already so heavy.
float hash( vec2 p ) {
    float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*83758.5453123);
	//return fract(sin(h)*4378.5453);
}

// bteitler: A 2D psuedo-random wave / terrain function.  This is actually a poor name in my opinion,
// since its the "hash" function that is really the noise, and this function is smoothly interpolating
// between noisy points to create a continuous surface.
float noise( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );	

    // bteitler: This is equivalent to the "smoothstep" interpolation function.
    // This is a smooth wave function with input between 0 and 1
    // (since it is taking the fractional part of <p>) and gives an output
    // between 0 and 1 that behaves and looks like a wave.  This is far from obvious, but we can graph it to see
    // Wolfram link: http://www.wolframalpha.com/input/?i=plot+x*x*%283.0-2.0*x%29+from+x%3D0+to+1
    // This is used to interpolate between random points.  Any smooth wave function that ramps up from 0 and
    // and hit 1.0 over the domain 0 to 1 would work.  For instance, sin(f * PI / 2.0) gives similar visuals.
    // This function is nice however because it does not require an expensive sine calculation.
    vec2 u = f*f*(3.0-2.0*f);

    // bteitler: This very confusing looking mish-mash is simply pulling deterministic random values (between 0 and 1)
    // for 4 corners of the grid square that <p> is inside, and doing 2D interpolation using the <u> function
    // (remember it looks like a nice wave!) 
    // The grid square has points defined at integer boundaries.  For example, if <p> is (4.3, 2.1), we will 
    // evaluate at points (4, 2), (5, 2), (4, 3), (5, 3), and then interpolate x using u(.3) and y using u(.1).
    return -1.0+2.0*mix( 
                mix( hash( i + vec2(0.0,0.0) ), 
                     hash( i + vec2(1.0,0.0) ), 
                        u.x),
                mix( hash( i + vec2(0.0,1.0) ), 
                     hash( i + vec2(1.0,1.0) ), 
                        u.x), 
                u.y);
}

// bteitler: diffuse lighting calculation - could be tweaked to taste
// lighting
float diffuse(vec3 n,vec3 l,float p) {
    return pow(dot(n,l) * 0.4 + 0.6,p);
}

// bteitler: specular lighting calculation - could be tweaked taste
float specular(vec3 n,vec3 l,vec3 e,float s) {    
    float nrm = (s + 8.0) / (3.1415 * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

// bteitler: Generate a smooth sky gradient color based on ray direction's Y value
// sky
vec3 getSkyColor(vec3 e) {
    e.y = max(e.y,0.0);
    vec3 ret;
    ret.x = pow(1.0-e.y,2.0);
    ret.y = 1.0-e.y;
    ret.z = 0.6+(1.0-e.y)*0.4;
    return ret;
}

// sea
// bteitler: TLDR is that this passes a low frequency random terrain through a 2D symmetric wave function that looks like this:
// http://www.wolframalpha.com/input/?i=%7B1-%7B%7B%7BAbs%5BCos%5B0.16x%5D%5D+%2B+Abs%5BCos%5B0.16x%5D%5D+%28%281.+-+Abs%5BSin%5B0.16x%5D%5D%29+-+Abs%5BCos%5B0.16x%5D%5D%29%7D+*+%7BAbs%5BCos%5B0.16y%5D%5D+%2B+Abs%5BCos%5B0.16y%5D%5D+%28%281.+-+Abs%5BSin%5B0.16y%5D%5D%29+-+Abs%5BCos%5B0.16y%5D%5D%29%7D%7D%5E0.65%7D%7D%5E4+from+-20+to+20
// The <choppy> parameter affects the wave shape.
float sea_octave(vec2 uv, float choppy) {
    // bteitler: Add the smoothed 2D terrain / wave function to the input coordinates
    // which are going to be our X and Z world coordinates.  It may be unclear why we are doing this.
    // This value is about to be passed through a wave function.  So we have a smoothed psuedo random height
    // field being added to our (X, Z) coordinates, and then fed through yet another wav function below.
    uv += noise(uv);
    // Note that you could simply return noise(uv) here and it would take on the characteristics of our 
    // noise interpolation function u and would be a reasonable heightmap for terrain.  
    // However, that isn't the shape we want in the end for an ocean with waves, so it will be fed through
    // a more wave like function.  Note that although both x and y channels of <uv> have the same value added, there is a 
    // symmetry break because <uv>.x and <uv>.y will typically be different values.

    // bteitler: This is a wave function with pointy peaks and curved troughs:
    // http://www.wolframalpha.com/input/?i=1-abs%28cos%28x%29%29%3B
    vec2 wv = 1.0-abs(sin(uv)); 

    // bteitler: This is a wave function with curved peaks and pointy troughs:
    // http://www.wolframalpha.com/input/?i=abs%28cos%28x%29%29%3B
    vec2 swv = abs(cos(uv));  
  
    // bteitler: Blending both wave functions gets us a new, cooler wave function (output between 0 and 1):
    // http://www.wolframalpha.com/input/?i=abs%28cos%28x%29%29+%2B+abs%28cos%28x%29%29+*+%28%281.0-abs%28sin%28x%29%29%29+-+abs%28cos%28x%29%29%29
    wv = mix(wv,swv,wv);

    // bteitler: Finally, compose both of the wave functions for X and Y channels into a final 
    // 1D height value, shaping it a bit along the way.  First, there is the composition (multiplication) of
    // the wave functions: wv.x * wv.y.  Wolfram will give us a cute 2D height graph for this!:
    // http://www.wolframalpha.com/input/?i=%7BAbs%5BCos%5Bx%5D%5D+%2B+Abs%5BCos%5Bx%5D%5D+%28%281.+-+Abs%5BSin%5Bx%5D%5D%29+-+Abs%5BCos%5Bx%5D%5D%29%7D+*+%7BAbs%5BCos%5By%5D%5D+%2B+Abs%5BCos%5By%5D%5D+%28%281.+-+Abs%5BSin%5By%5D%5D%29+-+Abs%5BCos%5By%5D%5D%29%7D
    // Next, we reshape the 2D wave function by exponentiation: (wv.x * wv.y)^0.65.  This slightly rounds the base of the wave:
    // http://www.wolframalpha.com/input/?i=%7B%7BAbs%5BCos%5Bx%5D%5D+%2B+Abs%5BCos%5Bx%5D%5D+%28%281.+-+Abs%5BSin%5Bx%5D%5D%29+-+Abs%5BCos%5Bx%5D%5D%29%7D+*+%7BAbs%5BCos%5By%5D%5D+%2B+Abs%5BCos%5By%5D%5D+%28%281.+-+Abs%5BSin%5By%5D%5D%29+-+Abs%5BCos%5By%5D%5D%29%7D%7D%5E0.65
    // one last final transform (with choppy = 4) results in this which resembles a recognizable ocean wave shape in 2D:
    // http://www.wolframalpha.com/input/?i=%7B1-%7B%7B%7BAbs%5BCos%5Bx%5D%5D+%2B+Abs%5BCos%5Bx%5D%5D+%28%281.+-+Abs%5BSin%5Bx%5D%5D%29+-+Abs%5BCos%5Bx%5D%5D%29%7D+*+%7BAbs%5BCos%5By%5D%5D+%2B+Abs%5BCos%5By%5D%5D+%28%281.+-+Abs%5BSin%5By%5D%5D%29+-+Abs%5BCos%5By%5D%5D%29%7D%7D%5E0.65%7D%7D%5E4
    // Note that this function is called with a specific frequency multiplier which will stretch out the wave.  Here is the graph
    // with the base frequency used by map and map_detailed (0.16):
    // http://www.wolframalpha.com/input/?i=%7B1-%7B%7B%7BAbs%5BCos%5B0.16x%5D%5D+%2B+Abs%5BCos%5B0.16x%5D%5D+%28%281.+-+Abs%5BSin%5B0.16x%5D%5D%29+-+Abs%5BCos%5B0.16x%5D%5D%29%7D+*+%7BAbs%5BCos%5B0.16y%5D%5D+%2B+Abs%5BCos%5B0.16y%5D%5D+%28%281.+-+Abs%5BSin%5B0.16y%5D%5D%29+-+Abs%5BCos%5B0.16y%5D%5D%29%7D%7D%5E0.65%7D%7D%5E4+from+-20+to+20
    return pow(1.0-pow(wv.x * wv.y,0.65),choppy);
}

// bteitler: Compute the distance along Y axis of a point to the surface of the ocean
// using a low(er) resolution ocean height composition function (less iterations).
float map(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz;// uv.x *= 0.75;
    
    // bteitler: Compose our wave noise generation ("sea_octave") with different frequencies
    // and offsets to achieve a final height map that looks like an ocean.  Likely lots
    // of black magic / trial and error here to get it to look right.  Each sea_octave has this shape:
    // http://www.wolframalpha.com/input/?i=%7B1-%7B%7B%7BAbs%5BCos%5B0.16x%5D%5D+%2B+Abs%5BCos%5B0.16x%5D%5D+%28%281.+-+Abs%5BSin%5B0.16x%5D%5D%29+-+Abs%5BCos%5B0.16x%5D%5D%29%7D+*+%7BAbs%5BCos%5B0.16y%5D%5D+%2B+Abs%5BCos%5B0.16y%5D%5D+%28%281.+-+Abs%5BSin%5B0.16y%5D%5D%29+-+Abs%5BCos%5B0.16y%5D%5D%29%7D%7D%5E0.65%7D%7D%5E4+from+-20+to+20
    // which should give you an idea of what is going.  You don't need to graph this function because it
    // appears to your left :)
    float d, h = 0.0;    
    for(int i = 0; i < ITER_GEOMETRY; i++) {
        // bteitler: start out with our 2D symmetric wave at the current frequency
    	d = sea_octave((uv+SEA_TIME)*freq,choppy);
        // bteitler: stack wave ontop of itself at an offset that varies over time for more height and wave pattern variance
    	//d += sea_octave((uv-SEA_TIME)*freq,choppy);

        h += d * amp; // bteitler: Bump our height by the current wave function
        
        // bteitler: "Twist" our domain input into a different space based on a permutation matrix
        // The scales of the matrix values affect the frequency of the wave at this iteration, but more importantly
        // it is responsible for the realistic assymetry since the domain is shiftly differently.
        // This is likely the most important parameter for wave topology.
    	uv *=  octave_m;
        
        freq *= 1.9; // bteitler: Exponentially increase frequency every iteration (on top of our permutation)
        amp *= 0.22; // bteitler: Lower the amplitude every frequency, since we are adding finer and finer detail
        // bteitler: finally, adjust the choppy parameter which will effect our base 2D sea_octave shape a bit.  This makes
        // the "waves within waves" have different looking shapes, not just frequency and offset
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

// bteitler: Compute the distance along Y axis of a point to the surface of the ocean
// using a high(er) resolution ocean height composition function (more iterations).
float map_detailed(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz;// uv.x *= 0.75;
    
    // bteitler: Compose our wave noise generation ("sea_octave") with different frequencies
    // and offsets to achieve a final height map that looks like an ocean.  Likely lots
    // of black magic / trial and error here to get it to look right.  Each sea_octave has this shape:
    // http://www.wolframalpha.com/input/?i=%7B1-%7B%7B%7BAbs%5BCos%5B0.16x%5D%5D+%2B+Abs%5BCos%5B0.16x%5D%5D+%28%281.+-+Abs%5BSin%5B0.16x%5D%5D%29+-+Abs%5BCos%5B0.16x%5D%5D%29%7D+*+%7BAbs%5BCos%5B0.16y%5D%5D+%2B+Abs%5BCos%5B0.16y%5D%5D+%28%281.+-+Abs%5BSin%5B0.16y%5D%5D%29+-+Abs%5BCos%5B0.16y%5D%5D%29%7D%7D%5E0.65%7D%7D%5E4+from+-20+to+20
    // which should give you an idea of what is going.  You don't need to graph this function because it
    // appears to your left :)
    float d, h = 0.0;    
    for(int i = 0; i < ITER_FRAGMENT; i++) {
        // bteitler: start out with our 2D symmetric wave at the current frequency
    	d = sea_octave((uv+SEA_TIME)*freq,choppy);
        // bteitler: stack wave ontop of itself at an offset that varies over time for more height and wave pattern variance
    	d += sea_octave((uv-SEA_TIME)*freq,choppy);
        
        h += d * amp; // bteitler: Bump our height by the current wave function
        
        // bteitler: "Twist" our domain input into a different space based on a permutation matrix
        // The scales of the matrix values affect the frequency of the wave at this iteration, but more importantly
        // it is responsible for the realistic assymetry since the domain is shiftly differently.
        // This is likely the most important parameter for wave topology.
    	uv *= octave_m/1.2;
        
        freq *= 1.9; // bteitler: Exponentially increase frequency every iteration (on top of our permutation)
        amp *= 0.22; // bteitler: Lower the amplitude every frequency, since we are adding finer and finer detail
        // bteitler: finally, adjust the choppy parameter which will effect our base 2D sea_octave shape a bit.  This makes
        // the "waves within waves" have different looking shapes, not just frequency and offset
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

// bteitler:
// p: point on ocean surface to get color for
// n: normal on ocean surface at <p>
// l: light (sun) direction
// eye: ray direction from camera position for this pixel
// dist: distance from camera to point <p> on ocean surface
vec3 getSeaColor(vec3 p, vec3 n, vec3 l, vec3 eye, vec3 dist, vec3 lightColor, float refrStr, vec3 refr) {  
    // bteitler: Fresnel is an exponential that gets bigger when the angle between ocean
    // surface normal and eye ray is smaller
    float fresnel = 1.0 - max(dot(n,-eye),0.0);
    fresnel = pow(fresnel,3.0) * 0.45;
        
    // bteitler: Bounce eye ray off ocean towards sky, and get the color of the sky
    vec3 reflected = mix(getSkyColor(reflect(eye, n))*0.99, refr, refrStr);    
    
    // bteitler: refraction effect based on angle between light surface normal
	vec3 refCol = mix(SEA_BASE, refr, refrStr*waterClarity);
	vec3 swCol = mix(SEA_WATER_COLOR, refr, refrStr*waterClarity);
    vec3 refracted = refCol + diffuse(n, l, 80.0) * swCol * 0.27 * lightColor; 
    
    // bteitler: blend the refracted color with the reflected color based on our fresnel term
    vec3 color = mix(refracted, reflected, fresnel);
    
    // bteitler: Apply a distance based attenuation factor which is stronger
    // at peaks
    float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
    color += SEA_WATER_COLOR * clamp(p.y - SEA_HEIGHT, 0.0, 1.0) * 0.15 * atten;
    
    // bteitler: Apply specular highlight
    color += vec3(specular(n, l, eye, 90.0)) * 0.5 * lightColor;
    
    return color;
}

// bteitler: Estimate the normal at a point <p> on the ocean surface using a slight more detailed
// ocean mapping function (using more noise octaves).
// Takes an argument <eps> (stands for epsilon) which is the resolution to use
// for the gradient.  See here for more info on gradients: https://en.wikipedia.org/wiki/Gradient
// tracing
vec3 getNormal(vec3 p, float eps) {
    // bteitler: Approximate gradient.  An exact gradient would need the "map" / "map_detailed" functions
    // to return x, y, and z, but it only computes height relative to surface along Y axis.  I'm assuming
    // for simplicity and / or optimization reasons we approximate the gradient by the change in ocean
    // height for all axis.
    vec3 n;
    n.y = map_detailed(p); // bteitler: Detailed height relative to surface, temporarily here to save a variable?
    n.x = map_detailed(vec3(p.x+eps,p.y,p.z)) - n.y; // bteitler approximate X gradient as change in height along X axis delta
    n.z = map_detailed(vec3(p.x,p.y,p.z+eps)) - n.y; // bteitler approximate Z gradient as change in height along Z axis delta
    // bteitler: Taking advantage of the fact that we know we won't have really steep waves, we expect
    // the Y normal component to be fairly large always.  Sacrifices yet more accurately to avoid some calculation.
    n.y = eps; 
    return normalize(n);

    // bteitler: A more naive and easy to understand version could look like this and
    // produces almost the same visuals and is a little more expensive.
    // vec3 n;
    // float h = map_detailed(p);
    // n.y = map_detailed(vec3(p.x,p.y+eps,p.z)) - h;
    // n.x = map_detailed(vec3(p.x+eps,p.y,p.z)) - h;
    // n.z = map_detailed(vec3(p.x,p.y,p.z+eps)) - h;
    // return normalize(n);
}

// bteitler: Find out where a ray intersects the current ocean
float heightMapTracing(vec3 ori, vec3 dir, out vec3 p) {  
    float tm = 0.0;
    float tx = 524288.0; // bteitler: a really far distance, this could likely be tweaked a bit as desired

    // bteitler: At a really far away distance along the ray, what is it's height relative
    // to the ocean in ONLY the Y direction?
    float hx = map(ori + dir * tx);
    
    // bteitler: A positive height relative to the ocean surface (in Y direction) at a really far distance means
    // this pixel is pure sky.  Quit early and return the far distance constant.
    if(hx > 0.0) return tx;   

    // bteitler: hm starts out as the height of the camera position relative to ocean.
    float hm = map(ori + dir * tm); 
   
    // bteitler: This is the main ray marching logic.  This is probably the single most confusing part of the shader
    // since height mapping is not an exact distance field (tells you distance to surface if you drop a line down to ocean
    // surface in the Y direction, but there could have been a peak at a very close point along the x and z 
    // directions that is closer).  Therefore, it would be possible/easy to overshoot the surface using the raw height field
    // as the march distance.  The author uses a trick to compensate for this.
    float tmid = 0.0;
    for(int i = 0; i < NUM_STEPS; i++) { // bteitler: Constant number of ray marches per ray that hits the water
        // bteitler: Move forward along ray in such a way that has the following properties:
        // 1. If our current height relative to ocean is higher, move forward more
        // 2. If the height relative to ocean floor very far along the ray is much lower
        //    below the ocean surface, move forward less
        // Idea behind 1. is that if we are far above the ocean floor we can risk jumping
        // forward more without shooting under ocean, because the ocean is mostly level.
        // The idea behind 2. is that if extruding the ray goes farther under the ocean, then 
        // you are looking more orthgonal to ocean surface (as opposed to looking towards horizon), and therefore
        // movement along the ray gets closer to ocean faster, so we need to move forward less to reduce risk
        // of overshooting.
        tmid = mix(tm,tx, hm/(hm-hx));
        p = ori + dir * tmid; 
                  
    	float hmid = map(p); // bteitler: Re-evaluate height relative to ocean surface in Y axis

        if(hmid < 0.0) { // bteitler: We went through the ocean surface if we are negative relative to surface now
            // bteitler: So instead of actually marching forward to cross the surface, we instead
            // assign our really far distance and height to be where we just evaluated that crossed the surface.
            // Next iteration will attempt to go forward more and is less likely to cross the boundary.
            // A naive implementation might have returned <tmid> immediately here, which
            // results in a much poorer / somewhat indeterministic quality rendering.
            tx = tmid;
            hx = hmid;
        } else {
            // Haven't hit surface yet, easy case, just march forward
            tm = tmid;
            hm = hmid;
        }
    }

    // bteitler: Return the distance, which should be really close to the height map without going under the ocean
    return tmid;
}

void Water( inout vec4 fragColor, vec4 positionMap, vec4 waterMap, vec4 waterMapUpper, bool pixelIsUnderWater, float wHeight ) 
{
	vec3 inColor = fragColor.rgb;

	vec3 ori = u_ViewOrigin.xzy;
	ori.y -= wHeight;
	
	//if (length(ori.y - u_ViewOrigin.z) < waveHeight*2.0)
	/*if (distance(waterMap.y, ori.y) < waveHeight*2.0)
	{// Correct low angles...
		if (pixelIsUnderWater)
			ori.y -= waveHeight*2.0;
		else
			ori.y += waveHeight*2.0;
	}*/

	vec3 dir = normalize(waterMap.xyz - ori);

	// bteitler: direction of the infinitely far away directional light.  Changing this will change
    // the sunlight direction.
    vec3 light = normalize(u_PrimaryLightOrigin.xzy - u_ViewOrigin.xzy);

	vec3 lightColor = u_PrimaryLightColor;
	
	if (SHADER_NIGHT_SCALE > 0.0)
	{// Adjust the sun color at sunrise/sunset...
		vec3 sunsetSun = vec3(1.0, 0.8, 0.625);
		lightColor = mix(lightColor, sunsetSun, SHADER_NIGHT_SCALE);
	}

	ori /= waveHeight;

	float mapLevel;
	float adjustedWMUHeight;

	if (pixelIsUnderWater)
	{
		dir *= -1.0;
		light *= -1.0;
		mapLevel = positionMap.y / (waveHeight * 1.25/*2.0*/);
		adjustedWMUHeight = waterMapUpper.y / (waveHeight * 2.0);
	}
	else
	{
		mapLevel = positionMap.y / (waveHeight * -1.25/*-2.0*/);
		adjustedWMUHeight = waterMapUpper.y / (waveHeight * -2.0);
	}

    // tracing

    // bteitler: ray-march to the ocean surface (which can be thought of as a randomly generated height map)
    // and store in p
    vec3 p;
    heightMapTracing(ori, dir, p);

	if (p.y > mapLevel)
	{
		return;
	}

	//float height = map(p);
	
	float height = length((p.y - adjustedWMUHeight));

    vec3 dist = p - ori; // bteitler: distance vector to ocean surface for this pixel's ray
	//vec3 dist2 = dist * u_Local0.g;
	vec3 dist2 = normalize(dist);

    // bteitler: Calculate the normal on the ocean surface where we intersected (p), using
    // different "resolution" (in a sense) based on how far away the ray traveled.  Normals close to
    // the camera should be calculated with high resolution, and normals far from the camera should be calculated with low resolution
    // The reason to do this is that specular effects (or non linear normal based lighting effects) become fairly random at
    // far distances and low resolutions and can cause unpleasant shimmering during motion.
    vec3 n = getNormal(p, 
             dot(dist2,dist2)   // bteitler: Think of this as inverse resolution, so far distances get bigger at an expnential rate
                * EPSILON_NRM // bteitler: Just a resolution constant.. could easily be tweaked to artistic content
           );

	vec3 finalPos = p.xyz;

	if (pixelIsUnderWater)
	{
		finalPos = p.xyz * waveHeight;
	}
	else
	{
		finalPos = p.xyz * waveHeight;
	}

	float hMap;

	/*if (HAVE_HEIGHTMAP > 0.0)
	{
		hMap = GetHeightMap(waterMap.xzy);
		//hMap = GetHeightMap(finalPos.xzy);
	}
	else*/
	{
		hMap = positionMap.y;
	}

	//float depth2 = length(mapLevel - hMap) * 1.25;
	//float depth = length(length(positionMap.xyz - finalPos) / (waveHeight * -1.25));
	//float depthN = length(depth * fadeSpeed);

	float depth = length(positionMap.xyz - finalPos) * waterClarity * 10000000.0;
	float depth2 = length(finalPos.y - positionMap.y) * 1000.0;
	float depthN = length(depth * fadeSpeed);
	

	float timer = systemtimer * (waveHeight / 16.0);
	vec2 texCoord = var_TexCoords.xy;
	texCoord.x += sin(timer * 0.002 + 3.0 * abs(positionMap.y)) * (refractionScale * min(depth2, 1.0));
	vec3 refraction = textureLod(u_DiffuseMap, texCoord, 0.0).rgb;

    // CaliCoastReplay:  Get the sky and sea colors
	//vec3 skyColor = getSkyColor(dir);
	float refStr = (1.0 - clamp((depthN * pow(1.0-waterClarity, 5.0)) / visibility, 0.0, 1.0));
    vec3 seaColor = getSeaColor(p,n,light,dir,dist,lightColor,refStr*0.5,refraction);
	if (pixelIsUnderWater) refStr = 0.5;
	seaColor = mix(seaColor, refraction, refStr);
    
    //Sea/sky preprocessing
    
    //CaliCoastReplay:  A distance falloff for the sea color.   Drastically darkens the sea, 
    //this will be reversed later based on day/night.
    seaColor /= sqrt(sqrt(length(dist))) ;
    
    
    //CaliCoastReplay:  Day/night mode
	vec3 seaColorDay = seaColor;
	//vec3 skyColorDay = skyColor;
	vec3 seaColorNight = seaColor;
	//vec3 skyColorNight = skyColor;
    
        
	//Brighten the sea up again, but not too bright at night
    seaColorNight *= seaColorNight * 8.5;
        
    //Turn down the sky 
    //skyColorNight /= 1.69;
    
    //Brighten the sea up again - bright and beautiful blue at day
    seaColorDay *= sqrt(sqrt(seaColorDay)) * 4.0;
    //skyColorDay *= 1.05;
    //skyColorDay -= 0.03;
    

	float skyDayNightFactor = clamp(clamp(SHADER_NIGHT_SCALE - 0.75, 0.0, 1.0) * 4.0, 0.0, 1.0);

	seaColor = mix(seaColorDay, seaColorNight, skyDayNightFactor);
	//skyColor = mix(skyColorDay, skyColorNight, skyDayNightFactor);
    
    //CaliCoastReplay:  A slight "constrasting" for the sky to match the more contrasted ocean
    //skyColor *= skyColor;
    
    
    //CaliCoastReplay:  A rather hacky manipulation of the high-value regions in the image that seems
    //to add a subtle charm and "sheen" and foamy effect to high value regions through subtle darkening,
    //but it is hacky, and not physically modeled at all.  
    vec3 seaHsv = rgb2hsv(seaColor);
    if (seaHsv.z > .75 && length(dist) < 50.0)
       seaHsv.z -= (0.9 - seaHsv.z) * 1.3;
    seaColor = hsv2rgb(seaHsv);
    
    // bteitler: Mix (linear interpolate) a color calculated for the sky (based solely on ray direction) and a sea color 
    // which contains a realistic lighting model.  This is basically doing a fog calculation: weighing more the sky color
    // in the distance in an exponential manner.
    
    vec3 color = seaColor;
        
    // Postprocessing
    
    // bteitler: Apply an overall image brightness factor as the final color for this pixel.  Can be
    // tweaked artistically.
    fragColor = vec4(pow(color,vec3(0.75)), 1.0);
    
    // CaliCoastReplay:  Adjust hue, saturation, and value adjustment for an even more processed look
    // hsv.x is hue, hsv.y is saturation, and hsv.z is value
    vec3 hsv = rgb2hsv(fragColor.xyz);    
    //CaliCoastReplay: Increase saturation slightly
    hsv.y += 0.131;
    //CaliCoastReplay:
    //A pseudo-multiplicative adjustment of value, increasing intensity near 1 and decreasing it near
    //0 to achieve a more contrasted, real-world look
    hsv.z *= sqrt(hsv.z) * 1.1; 
    
	//skyDayNightFactor

	vec3 dayHsv = hsv;
	vec3 nightHsv = hsv;

	//
	// Night hsv...
	//

    ///CaliCoastReplay:
    //Slight value adjustment at night to turn down global intensity
    nightHsv.z -= 0.045;
    nightHsv*=0.8;
    nightHsv.x += 0.12 + nightHsv.z/100.0;
    //Highly increased saturation at night op, oddly.  Nights appear to be very colorful
    //within their ranges.
    nightHsv.y *= 2.87;
    
	//
	// Day hsv...
	//

    //CaliCoastReplay:
    //Add green tinge to the high range
    //Turn down intensity in day in a different way     
    dayHsv.z *= 0.9;
        
    //CaliCoastReplay:  Hue alteration 
    //dayHsv.x -= dayHsv.z/10.0;
    //dayHsv.x += 0.02 + dayHsv.z/50.0;

    //Final brightening
    dayHsv.z *= 1.01;
    //This really "cinemafies" it for the day -
    //puts the saturation on a squared, highly magnified footing.
    //Worth looking into more as to exactly why.
    // hsv.y *= 5.10 * hsv.y * sqrt(hsv.y);
    dayHsv.y += 0.07;
    

	hsv = mix(dayHsv, nightHsv, skyDayNightFactor);

    
    //CaliCoastReplay:    
    //Replace the final color with the adjusted, translated HSV values
    fragColor.xyz = clamp(hsv2rgb(hsv), 0.0, 1.0);


	vec3 refraction1 = mix(refraction, fragColor.rgb, clamp(depthN / visibility, 0.0, 1.0));
	fragColor.rgb = mix(refraction1, fragColor.rgb, clamp((vec3(depth2) / vec3(extinction)), 0.0, 1.0));


#if defined(USE_REFLECTION) && !defined(__LQ_MODE__)
	float dt = pow(clamp(dot(reflect(-dir, n), -dir), 0.0, 1.0), 4.0);

	if (!pixelIsUnderWater && u_Local1.g >= 2.0 && dt > 0.0)
	{
		fragColor.rgb = AddReflection(var_TexCoords, finalPos, waterMap.xyz, fragColor.rgb, height, dt);
	}
#endif //defined(USE_REFLECTION) && !defined(__LQ_MODE__)

	/*
	bool foamAdded = false;
	bool whitecapAdded = false;
	vec4 foam = vec4(0.0);
	float foamLength = 0.0;
	float foamPower = clamp(pow(1.0 - height, 1.25), 0.0, 1.0);
	foamPower *= depthN / 48.0;

	if (depth2 < foamExistence.x)
	{
		foam = GetFoamMap(waterMap.xzy);
		foamAdded = true;
	}
	else if (depth2 < foamExistence.y)
	{
		foam = mix(GetFoamMap(waterMap.xzy), vec4(0.0), (depth2 - foamExistence.x) / (foamExistence.y - foamExistence.x));
		foamAdded = true;
	}

	if (waveHeight - foamExistence.z > 0.0001)
	{
		float fPow = clamp((finalPos.y - (waterMapUpper.y + foamExistence.z)) / (waveHeight - foamExistence.z), 0.0, 1.0);
		foam += GetFoamMap(waterMap.xzy + foamExistence.z) * fPow;
		if (fPow > 0.0)
		{
			whitecapAdded = true;
		}
	}

	if (foam.a <= 0.0) foam = vec4(0.0);

	if (foamAdded || whitecapAdded)
	{
		fragColor.rgb = clamp(fragColor.rgb + (foam.rgb * foamPower), 0.0, 1.0);
	}
	*/
}

void main ( void )
{
	vec4 waterMapUpper = waterMapUpperAtCoord(var_TexCoords);
	vec4 waterMapLower = waterMapUpper;
	waterMapLower.y -= waveHeight;
	bool IS_UNDERWATER = false;
	bool pixelIsInWaterRange = false;
	bool pixelIsUnderWater = false;

	if (u_Local1.b > 0.0) 
	{
		IS_UNDERWATER = true;
	}

	vec4 positionMap = positionMapAtCoord(var_TexCoords);
	vec3 position = positionMap.xyz;

	bool isSky = (positionMap.a-1.0 == 1024.0 || positionMap.a-1.0 == 1025.0) ? true : false;

	vec3 color = textureLod(u_DiffuseMap, var_TexCoords, 0.0).rgb;

	if (waterMapLower.a > 0.0)
	{
		position.xz = waterMapLower.xz; // test

#if defined(FIX_WATER_DEPTH_ISSUES)
		if (isSky)
		{
			if (IS_UNDERWATER)
			{
				position.xyz = waterMapLower.xyz;
				position.y += 1024.0;
			}
			else
			{
				position.xyz = waterMapLower.xyz;
				position.y -= 1024.0;
			}
		}
#endif //defined(FIX_WATER_DEPTH_ISSUES)
	}
	else
	{
#if defined(FIX_WATER_DEPTH_ISSUES)
		if (isSky)
		{
			if (IS_UNDERWATER)
			{
				position.y += 1024.0;
			}
			else
			{
				position.y -= 1024.0;
			}
		}
#endif //defined(FIX_WATER_DEPTH_ISSUES)
	}

	if (waterMapUpper.a <= 0.0)
	{// Should be safe to skip everything else.
		gl_FragColor = vec4(color, 1.0);
		return;
	}
	else if (IS_UNDERWATER || waterMapLower.y > ViewOrigin.y)
	{
		pixelIsUnderWater = true;
	}
	else if (waterMapUpper.y >= position.y)
	{
		pixelIsInWaterRange = true;
	}

	if (pixelIsInWaterRange || pixelIsUnderWater || position.y <= waterMapLower.y + waveHeight)
	{
		float wHeight = waterMapLower.y;
		waterMapLower.y -= wHeight;
		waterMapUpper.y -= wHeight;
		position.y -= wHeight;
		positionMap.y -= wHeight;
		position.y -= waveHeight * 0.75; // Move the water level to half way up the possible wave height... So height 0.75 is at the original plane...

		vec4 fragColor = vec4(color.rgb, 1.0);
		Water( fragColor, vec4(position.xyz, positionMap.a), waterMapLower, waterMapUpper, pixelIsUnderWater, wHeight );
		color.rgb = fragColor.rgb;
	}

	gl_FragColor = vec4(color, 1.0);
}
