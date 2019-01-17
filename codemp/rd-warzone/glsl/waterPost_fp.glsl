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
//const float waveHeight = 6.0;//4.0;
#define waveHeight u_Local10.r
#define waveDensity u_Local10.g

// Colour of the sun
//vec3 sunColor = (u_PrimaryLightColor.rgb + vec3(1.0) + vec3(1.0) + vec3(1.0)) / 4.0; // 1/4 real sun color, 3/4 white...
vec3 sunColor = vec3(1.0);

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


float saturate(in float val) {
	return clamp(val, 0.0, 1.0);
}

vec3 saturate(in vec3 val) {
	return clamp(val, vec3(0.0), vec3(1.0));
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
	
#define heightBlurPasses 4.0
	for (int i = 1; i < int(heightBlurPasses); i++)
	{
		h += textureLod(u_HeightMap, GetMapTC(pos), pow(2.0, float(i))).r;
	}

	h /= heightBlurPasses;
	
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


float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

mat3 compute_tangent_frame(in vec3 N, in vec3 P, in vec2 UV) {
    vec3 dp1 = dFdx(P);
    vec3 dp2 = dFdy(P);
    vec2 duv1 = dFdx(UV);
    vec2 duv2 = dFdy(UV);
    // solve the linear system
    vec3 dp1xdp2 = cross(dp1, dp2);
    mat2x3 inverseM = mat2x3(cross(dp2, dp1xdp2), cross(dp1xdp2, dp1));
    vec3 T = inverseM * vec2(duv1.x, duv2.x);
    vec3 B = inverseM * vec2(duv1.y, duv2.y);
    // construct tangent frame
    float maxLength = max(length(T), length(B));
    T = T / maxLength;
    B = B / maxLength;
    return mat3(T, B, N);
}

// Function calculating fresnel term.
// - normal - normalized normal vector
// - eyeVec - normalized eye vector
float fresnelTerm(vec3 normal, vec3 eyeVec)
{
		float angle = 1.0f - clamp(dot(normal, eyeVec), 0.0, 1.0);
		float fresnel = angle * angle;
		fresnel = fresnel * fresnel;
		fresnel = fresnel * angle;
		return clamp(fresnel * (1.0 - clamp(R0, 0.0, 1.0)) + R0 - refractionStrength, 0.0, 1.0);
}

// Indices of refraction
#define Air 1.0
#define Bubble 1.06

// Air to glass ratio of the indices of refraction (Eta)
#define Eta (Air / Bubble)

// see http://en.wikipedia.org/wiki/Refractive_index Reflectivity
#define R0 (((Air - Bubble) * (Air - Bubble)) / ((Air + Bubble) * (Air + Bubble)))

vec3 GetFresnel( vec3 vView, vec3 vNormal, vec3 vR0, float fGloss )
{
	float NdotV = max( 0.0, dot( vView, vNormal ) );
	return vR0 + (vec3(1.0) - vR0) * pow( 1.0 - NdotV, 5.0 ) * pow( fGloss, 20.0 );
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

vec3 AddReflection(vec2 coord, vec3 positionMap, vec3 waterMapLower, vec3 inColor, float height, vec3 surfacePoint)
{
	if (positionMap.y > waterMapLower.y)
	{
		return inColor;
	}

	float wMapCheck = texture(u_WaterPositionMap, vec2(coord.x, 1.0)).a;
	if (wMapCheck > 0.0)
	{// Top of screen pixel is water, don't check...
		return inColor;
	}

	// Offset the final pixel based on the height of the wave at that point, to create randomization...
	float hoff = height * 2.0 - 1.0;
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

	return mix(inColor.rgb, landColor.rgb, vec3(landColor.a * u_Local1.a));
}
#endif //defined(USE_REFLECTION) && !defined(__LQ_MODE__)


float getdiffuse(vec3 n, vec3 l, float p) {
	return pow(dot(n, l) * 0.4 + 0.6, p);
}

float dosun(vec3 ray, vec3 lightDir) {
	return pow(max(0.0, dot(ray, lightDir)), 528.0);
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

// bteitler:
// p: point on ocean surface to get color for
// n: normal on ocean surface at <p>
// l: light (sun) direction
// eye: ray direction from camera position for this pixel
// dist: distance from camera to point <p> on ocean surface
vec3 getSeaColor(vec3 p, vec3 n, vec3 l, vec3 eye, vec3 dist, vec3 lightColor, vec3 SEA_BASE, vec3 SEA_WATER_COLOR, float height) {  
    // bteitler: Fresnel is an exponential that gets bigger when the angle between ocean
    // surface normal and eye ray is smaller
    float fresnel = 1.0 - max(dot(n,-eye),0.0);
    fresnel = pow(fresnel,3.0) * 0.05;//0.45;
        
    // bteitler: Bounce eye ray off ocean towards sky, and get the color of the sky
    vec3 reflected = getSkyColor(reflect(eye, n))*0.99;

	float DAY_FACTOR_DIFFUSE = pow(1.0 - SHADER_NIGHT_SCALE, 8.0);
	float DAY_FACTOR_SPECULAR = pow(1.0 - SHADER_NIGHT_SCALE, 6.0);
    
    // bteitler: refraction effect based on angle between light surface normal
	float dif = diffuse(n, l, 3.0);
	float dif1 = diffuse(n, l, 1.0);
	float dif2 = diffuse(-n, l, 1.0);
	float d = max(dif, max(dif1, dif2));
    //vec3 refracted = SEA_BASE + pow(d, 60.0) * SEA_WATER_COLOR * 0.27 * lightColor; 
	vec3 refracted = SEA_BASE + pow(d, 60.0) * 0.15 * SEA_WATER_COLOR * lightColor;
	refracted += clamp(pow(1.0 - dif2, 10.0) * 60.0, 0.0, 1.0) * lightColor * DAY_FACTOR_DIFFUSE;
	refracted += clamp(pow(dif1, 80.0) * 2048.0, 0.0, 1.0) * lightColor * DAY_FACTOR_DIFFUSE;
    
    // bteitler: blend the refracted color with the reflected color based on our fresnel term
    vec3 color = mix(refracted, reflected, fresnel);
    
    // bteitler: Apply a distance based attenuation factor which is stronger
    // at peaks
//    float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
//#define SEA_HEIGHT 0.5
//    color += SEA_WATER_COLOR * (p.y - SEA_HEIGHT) * 0.15 * atten;
//    color += SEA_WATER_COLOR * height * atten;
    
    // bteitler: Apply specular highlight
	float spec1 = specular(n, l, eye, 30.0/*90.0*/);
	float spec2 = specular(n, l, eye, 4.0);
	
	// Mad scientist uniqueone waterness-looking craziness, adds general water looks across the board... Completely fake, but looks nice :)
	float wness = clamp(n.y, 0.0, 1.0);
	float wness1 = pow(pow(wness, 80.0) * 0.5, 4.0);
	float wness2 = pow(wness, 8.0) * 0.333;
	float wness3 = pow(wness, 2.0) * 0.2;

	// It's not specular, it's not diffuse, it's "speckles"! :)
	float speckles = clamp(pow(1.0 - clamp(n.y * 2.0 - 1.0, 0.0, 1.0), 8.0) * 2.0, 0.0, 1.0);

	// Use the maximum from above for final specular...
	float spec3 = max(wness1, max(wness2, max(wness3, speckles)));

    color += vec3(max(spec1, max(spec2, spec3))) * 0.5 * lightColor * DAY_FACTOR_SPECULAR;
    
    return color;
}

#define iTime (1.0-u_Time)
#define EULER 2.7182818284590452353602874

// its from here https://github.com/achlubek/venginenative/blob/master/shaders/include/WaterHeight.glsl 
float wave(vec2 uv, vec2 emitter, float speed, float phase, float timeshift) {
	float dst = distance(uv, emitter);
	return pow(EULER, sin(dst * phase - (iTime + timeshift) * speed)) / EULER;
}
vec2 wavedrag(vec2 uv, vec2 emitter) {
	return normalize(uv - emitter);
}

#define DRAG_MULT 4.0

float getwaves(vec2 position) {
	position *= 0.1;
	float iter = 0.0;
	float phase = 1.5;// 6.0;
	float speed = 1.0;// 1.5;// 2.0;
	float weight = 1.0;// 1.0;
	float w = 0.0;
	float ws = 0.0;
	for (int i = 0; i<3; i++)
	{
		vec2 p = vec2(sin(iter), cos(iter)) * 30.0;
		float res = wave(position, p, speed, phase, 0.0);
		float res2 = wave(position, p, speed, phase, 0.006);
		position -= wavedrag(position, p) * (res - res2) * weight * DRAG_MULT;
		w += res * weight;
		iter += 12.0;
		ws += weight;
		weight = mix(weight, 0.0, float(i + 1) / 4.0/*0.2*/);
		phase *= 1.2;
		speed *= 1.02;
	}
	return clamp(w / ws, 0.0, 1.0);
	//return clamp(((w / ws) - 0.075) * 1.075, 0.0, 1.0);
}

mat2 octave_m = mat2(1.6,1.2,-1.2,1.6);

float sea_octave(vec2 uv, float choppy) {
    uv += noise(uv.xyx + uv.xyy);        
    vec2 wv = 1.0-abs(sin(uv));
    vec2 swv = abs(cos(uv));    
    wv = mix(wv,swv,wv);
    return pow(1.0-pow(wv.x * wv.y,0.65),choppy);
}

float getwavesDetail( vec2 p )
{
    float freq = 0.4;
    float amp = 0.1;
    float choppy = 5.0;
    vec2 uv = p.xy; uv.x *= 0.75;
    float sea_time = iTime * 0.8;
    float d, h = 0.0;    
    for(int i = 0; i < 3; i++) {        
    	d = sea_octave((uv+sea_time)*freq,choppy);
    	d += sea_octave((uv-sea_time)*freq,choppy);
        h += d * amp;        
    	uv *= octave_m; freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return h;
}

void ApplyHeightAdjustments(inout float height)
{
	height *= 1.05;
	height = pow(height, 1.5);
	
	//height = (height - u_Local0.r) * u_Local0.g;
	
	//height = height * u_Local0.r + u_Local0.g;
	//if (u_Local0.b > 0.0) height = clamp(height, 0.0, 1.0);
}

void GetHeightAndNormal(in vec2 pos, in float e, in float depth, inout float height, inout float chopheight, inout vec3 waveNormal, inout vec3 lightingNormal, in vec3 eyeVecNorm, in float timer, in float level) {
#if !defined(__LQ_MODE__) && defined(REAL_WAVES)
#define waveDetailFactor 0.333 //0.5

	if (USE_OCEAN > 0.0)
	{
		height = getwaves(pos.xy) * depth;

		ApplyHeightAdjustments(height);

		// Create a basic "big waves" normal vector...
		vec2 ex = vec2(e, 0.0) * 32.0;//16.0;

		float height2 = getwaves(pos.xy + ex.xy) * depth;
		float height3 = getwaves(pos.xy + ex.yx) * depth;

		ApplyHeightAdjustments(height2);

		ApplyHeightAdjustments(height3);

		vec3 a = vec3(depth, height * 64.0/*0.001*/, depth);
		vec3 b = vec3(depth - ex.x, height2 * 64.0/*0.001*/, depth);
		vec3 c = vec3(depth, height3 * 64.0/*0.001*/, depth + ex.x);
		waveNormal = cross(normalize(a - b), normalize(a - c));

		// Now make a lighting normal vector, with some "small waves" bumpiness...
		waveNormal = normalize(waveNormal);

		ex = vec2(e, 0.0) * 32.0;

		float dheight = getwavesDetail(pos.xy * waveDetailFactor) * depth;
		float dheight2 = getwavesDetail((pos.xy * waveDetailFactor) + ex.xy) * depth;
		float dheight3 = getwavesDetail((pos.xy * waveDetailFactor) + ex.yx) * depth;

		vec3 da = vec3(depth, dheight * 4.0, depth);
		vec3 db = vec3(depth - ex.x, dheight2 * 4.0, depth);
		vec3 dc = vec3(depth, dheight2 * 4.0, depth + ex.x);
		vec3 dnormal = cross(normalize(da - db), normalize(da - dc));
		dnormal = normalize(dnormal);

		lightingNormal = normalize(mix(waveNormal, dnormal, 0.6));

		//height *= u_Local0.g;//1.4;
		chopheight = max(height, dheight) * 0.95;//* 0.25;
	}
	else
#endif //!defined(__LQ_MODE__) && defined(REAL_WAVES)
	{
		vec2 ex = vec2(e, 0) * 64.0;

		height = getwavesDetail(pos.xy * waveDetailFactor) * depth;
		float height2 = getwavesDetail((pos.xy * waveDetailFactor) + ex.xy) * depth;
		float height3 = getwavesDetail((pos.xy * waveDetailFactor) + ex.yx) * depth;

		/*vec3 da = vec3(pos.x, height, pos.y);
		waveNormal = normalize(cross(normalize(da - vec3(pos.x - e, height2, pos.y)), normalize(da - vec3(pos.x, height3, pos.y + e))));
		waveNormal.xz *= 256.0;
		waveNormal = normalize(waveNormal);
		lightingNormal = waveNormal;*/

		vec3 da = vec3(depth, height * 0.05, depth);
		vec3 db = vec3(depth - e, height2 * 0.05, depth);
		vec3 dc = vec3(depth, height2 * 0.05, depth + e);
		waveNormal = cross(normalize(da - db), normalize(da - dc));
		waveNormal = normalize(waveNormal);
		lightingNormal = waveNormal;

		height = 0.0;
		chopheight = height * 1.125;
	}

	height = length(height);
	chopheight = length(chopheight);
}

void ScanHeightMap(in vec4 waterMapLower, inout vec3 surfacePoint, inout vec3 eyeVecNorm, inout float level, inout vec2 texCoord, inout float t, out vec3 waveNormal, inout vec3 lightingNormal, in float timer, inout float height, inout float chopheight, in bool IS_UNDERWATER)
{
#ifndef USE_RAYTRACE
	//
	// No raytracing enabled, just grab this pixel's height...
	//
	GetHeightAndNormal(surfacePoint.xz*0.03, 0.004, 1.0, height, chopheight, waveNormal, lightingNormal, eyeVecNorm, timer, level);

	surfacePoint.xyz = waterMapLower.xyz;
#else //USE_RAYTRACE
	//
	// Scan for the closest big wave that intersects this ray...
	//

	float dist = distance(surfacePoint.xyz, ViewOrigin.xyz);
	vec3 n = -eyeVecNorm.xyz;

	const int nSteps = 256;
    float dt = dist / float(nSteps);
    t = 0.0;

	vec3 pos = ViewOrigin + n.xyz * t;
	float maxHeight = level + waveHeight;
	vec3 highest = surfacePoint.xyz;

	for (int i = 0; i < nSteps; i++)
	{
		if (pos.y <= maxHeight)
		{
			float h = getwaves(pos.xz*0.03);
			//ApplyHeightAdjustments(h);

			if (!IS_UNDERWATER && level + (h * waveHeight) >= pos.y)
			{
				highest = pos;
				break;
			}
			else if (IS_UNDERWATER && level + (h * waveHeight) <= pos.y)
			{
				highest = pos;
				break;
			}
		}

		t += dt;

		pos = ViewOrigin + n.xyz * t;
	}

	// Now grab either the closest big wave we found above, or the end position if we found no intersections...
	GetHeightAndNormal(highest.xz*0.03, 0.004, 1.0, height, chopheight, waveNormal, lightingNormal, eyeVecNorm, timer, level);

	surfacePoint.xyz = highest.xyz;
	//surfacePoint.y = waterMapLower.y;
#endif //USE_RAYTRACE
}

float WaterFallNearby ( vec2 uv, out float hit )
{
	hit = 0.0;
	float numTests = 0.0;

	for (float x = 1.0; x <= 16.0; x += 1.0)
	{
		numTests += 1.0;

		vec2 coord = uv + vec2(x * pw, 0.0);
		if (uv.x >= 0.0 && uv.x <= 1.0)
		{
			float isWater = texture(u_WaterPositionMap, coord).a;

			if (isWater >= 2.0)
			{
				hit = isWater;
				break;
			}
		}

		coord = uv - vec2(x * pw, 0.0);
		if (uv.x >= 0.0 && uv.x <= 1.0)
		{
			float isWater = texture(u_WaterPositionMap, coord).a;

			if (isWater >= 2.0)
			{
				hit = isWater;
				break;
			}
		}
	}

	return numTests / 16.0;
}

vec4 WaterFall(vec3 color, vec3 color2, vec3 waterMapUpper, vec3 position, float timer, float slope, float wfEdgeFactor, float wfHitType)
{
#ifdef EXPERIMENTAL_WATERFALL
	//return vec4(0.5 + (0.5 * (1.0 - wfEdgeFactor)), 0.0, 0.0, 1.0);

	vec3 n = normalize(position);
	n.y += systemtimer * 0.00008;
	float rand = SmoothNoise( n * u_Local7.r );

	for (float x = 2.0; x <= u_Local7.g; x *= 2.0)
	{
		rand *= SmoothNoise( n * (u_Local7.r * x) );
	}

	rand = clamp(pow(rand, u_Local7.b), 0.0, 1.0);

	rand -= 0.05;

	rand = clamp(pow(rand, u_Local7.a), 0.0, 1.0);

	vec2 texCoord = var_TexCoords.xy;

	//float hitType = 0.0;
	//float edgeFactor = WaterFallScanEdge( texCoord, hitType );
	float edgeFactor = smoothstep(0.0, 1.0, 1.0 - wfEdgeFactor);

	texCoord.x += sin(timer * 0.002 + 3.0 * abs(position.y)) * refractionScale;

	vec3 refraction = textureLod(u_DiffuseMap, texCoord, 0.0).rgb;
	refraction = mix(waterColorShallow.rgb, refraction.rgb, clamp(slope, 0.0, 1.0)/*0.3*/);

	vec3 wfColor = mix(refraction.rgb, vec3(1.0), vec3(rand));
	
	return vec4(mix(color, wfColor, vec3(edgeFactor)), 1.0);
#else //!EXPERIMENTAL_WATERFALL
	vec3 eyeVecNorm = normalize(ViewOrigin - waterMapUpper.xyz);
	vec3 pixelDir = eyeVecNorm;
	vec3 normal = normalize(pixelDir + (color.rgb * 0.5 - 0.25));
	vec3 specular = vec3(0.0);
	vec3 lightDir = normalize(ViewOrigin.xyz - u_PrimaryLightOrigin.xzy);
	float lambertian2 = dot(lightDir.xyz, normal);
	float spec2 = 0.0;
	float fresnel = clamp(1.0 - dot(normal, -eyeVecNorm), 0.0, 1.0);
	fresnel = pow(fresnel, 3.0) * 0.65;

	vec2 texCoord = var_TexCoords.xy;
	texCoord.x += sin(timer * 0.002 + 3.0 * abs(position.y)) * refractionScale;

	vec3 refraction = textureLod(u_DiffuseMap, texCoord, 0.0).rgb;
	refraction = mix(waterColorShallow.rgb, refraction.rgb, 0.3);

	float fTime = u_Time * 2.0;

	vec3 foam = (GetFoamMap(vec3(waterMapUpper.x * 0.03, (waterMapUpper.y * 0.03) + fTime, 0.5)).rgb + GetFoamMap(vec3(waterMapUpper.z * 0.03, (waterMapUpper.y * 0.03) + fTime, 0.5)).rgb) * 0.5;

	color = mix(color + (foam * sunColor), refraction, fresnel * 0.8);

	if (lambertian2 > 0.0)
	{// this is blinn phong
		vec3 mirrorEye = (2.0 * dot(eyeVecNorm, normal) * normal - eyeVecNorm);
		vec3 halfDir2 = normalize(lightDir.xyz + mirrorEye);
		float specAngle = max(dot(halfDir2, normal), 0.0);
		spec2 = pow(specAngle, 16.0);
		specular = vec3(clamp(1.0 - fresnel, 0.4, 1.0)) * (vec3(spec2 * shininess)) * sunColor * specularScale * 25.0;
	}

	color = clamp(color + specular, 0.0, 1.0);

	//color += blinn_phong(waterMapUpper.xyz, color.rgb, normal, eyeVecNorm, lightDir, u_PrimaryLightColor.rgb, u_PrimaryLightColor.rgb, 1.0, u_PrimaryLightOrigin.xyz);

#if defined(USE_REFLECTION) && !defined(__LQ_MODE__)
	color = AddReflection(texCoord, position.xyz, waterMapUpper.xyz, color, 0.0, position.xyz);
#endif //defined(USE_REFLECTION) && !defined(__LQ_MODE__)

	return vec4(color, 1.0);
#endif //EXPERIMENTAL_WATERFALL
}

#ifdef USE_DETAILED_UNDERWATER
vec3 DoUnderwater(vec3 position, bool isSky)
{
	vec3 color = mix(waterColorShallow.rgb, waterColorDeep.rgb, /*isSky ? 1.0 :*/ clamp((MAP_WATER_LEVEL - position.y) / 2048.0, 0.0, 1.0));
	return color;
}
#endif //USE_DETAILED_UNDERWATER

float GodRay(vec2 uv, float scale, float threshold, float speed, float angle){
	float value = pow(sin((uv.x + uv.y * angle + iTime * speed) * scale), 6.0);
    value += float(threshold < value);
    return clamp(value, 0.0, 1.0); 
}

float GodRays( in vec2 uv )
{
    float light = GodRay(uv, 22.0,0.5,-0.003,0.0) * 0.3;
    light += GodRay(uv, 47.0, 0.99, 0.02, 0.0) * 0.1;
    light += GodRay(uv, 25.0, 0.9, -0.01, 0.0) * 0.2;
    light += GodRay(uv, 52.0, 0.4, 0.0001, 0.0) * 0.1;
    light += GodRay(uv, 49.0, 0.4, 0.0003, 0.0) * 0.1;
    light += GodRay(uv, 57.0, 0.4, -0.0001, 0.0) * 0.1;
    light += GodRay(uv, 200.0,0.8, -0.0001, 0.0) * 0.05;
    light -= pow((1.0 - uv.y) * 0.7, 0.8);
    light = max(light, 0.0);
	return light;
}

void main ( void )
{
	vec4 waterMapUpper = waterMapUpperAtCoord(var_TexCoords);
	vec4 waterMapLower = waterMapUpper;
	vec4 origWaterMapUpper = waterMapUpper;
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

	float timer = systemtimer * (waveHeight / 16.0);

	vec3 color2;
	vec2 uv = var_TexCoords;

#if 0 // Hmm deferred lighting etc that run after this cause the above water geometry shadows to still show, etc, so, meh...
	if (IS_UNDERWATER && waterMapLower.a > 0.0)
	{// When underwater, and looking through the surface, distort the above ground view...
		GetUnderwaterUV(uv);
	}
#endif
	
	color2 = textureLod(u_DiffuseMap, uv, 0.0).rgb;

	vec3 color = color2;

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
#ifdef USE_DETAILED_UNDERWATER
		if (IS_UNDERWATER)
		{
			//if (position.x < u_Mins.x || position.y < u_Mins.y || position.z < u_Mins.z) isSky = true;
			//if (position.x > u_Maxs.x || position.y > u_Maxs.y || position.z > u_Maxs.z) isSky = true;

			float pDist = distance(position.xyz, ViewOrigin.xyz);
			float wDist = waterMapUpper.a < 1.0 ? 999999.9 : distance(waterMapLower.xyz, ViewOrigin.xyz);
			float cDist = min(pDist, wDist);

			float power = isSky ? 1.0 : clamp(cDist / 2048.0, 0.0, 1.0) * 0.75 + 0.25;
			color.rgb = mix(color.rgb, DoUnderwater(position.xyz, isSky), power);
			color2.rgb = color.rgb;
		
			/*if (power >= 1.0)
			{
				gl_FragColor = vec4(color.rgb, 1.0);
				return;
			}*/
		}
#endif //USE_DETAILED_UNDERWATER

#if 0
		if (IS_UNDERWATER)
		{
			vec3 dir = normalize(ViewOrigin - position);
			dir = dir * 0.5 + 0.5;
			//float gr = GodRays( vec2((dir.x + dir.z) / 2.0, dir.y) ) * u_Local7.r;
			float gr = GodRays( vec2(dir.x, dir.y) ) * u_Local7.r;
			color.rgb += gr * u_PrimaryLightColor;
			color2.rgb += gr * u_PrimaryLightColor;
		}
#endif

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

#ifdef EXPERIMENTAL_WATERFALL
	float hitType = 0.0;
	float edgeFactor = 0.0;
	
	if (waterMapLower.a < 1024.0)
	{
		edgeFactor = WaterFallNearby( var_TexCoords, hitType );
	}

	if ((waterMapLower.a >= 1024.0 || hitType >= 1024.0) /*&& positionMap.a-1.0 < 1024.0*/)
	{// Low horizontal normal, this is a waterfall...
		if (waterMapLower.a >= 1024.0 /*&& positionMap.a-1.0 < 1024.0*/)
		{// Actual waterfall pixel...
			gl_FragColor = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, waterMapLower.a - 2.0, 0.0, waterMapLower.a);
			return;
		}
		else if (waterMapUpper.a <= 0.0)
		{// near a waterfall, but this is not water.
			gl_FragColor = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, hitType - 2.0, edgeFactor, hitType);
			return;
		}
		else
		{// Blend with water...
			color = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, waterMapLower.a - 2.0, edgeFactor, hitType).rgb;
			color2 = color;
		}
	}
#else //!EXPERIMENTAL_WATERFALL
	if (waterMapLower.a >= 1024.0 /*&& positionMap.a-1.0 < 1024.0*/)
	{// Actual waterfall pixel...
		gl_FragColor = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, waterMapLower.a - 2.0, 0.0, waterMapLower.a);
		return;
	}
#endif //EXPERIMENTAL_WATERFALL

	if (waterMapUpper.a <= 0.0)
	{// Should be safe to skip everything else.
		gl_FragColor = vec4(color2, 1.0);
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

	float waterLevel = waterMapLower.y;
	float level = waterMapLower.y;
	float depth = 0.0;

	if (pixelIsInWaterRange || pixelIsUnderWater || position.y <= level + waveHeight)
	{
		//vec2 wind = normalize(waterMapLower.xzy).xy; // Waves head toward center of map. Should suit current WZ maps...

		// Find intersection with water surface
		vec3 eyeVecNorm = normalize(ViewOrigin - waterMapLower.xyz);
		//vec3 eyeVecNorm = normalize(var_ViewDir.xzy);
		
		float t = 0.0;
		vec3 surfacePoint = waterMapLower.xyz;

		vec2 texCoord = vec2(0.0);

		float height = 0.0;
		float chopheight = 0.0;
		vec3 normal;
		vec3 lightingNormal;
		ScanHeightMap(waterMapLower, surfacePoint, eyeVecNorm, level, texCoord, t, normal, lightingNormal, timer, height, chopheight, IS_UNDERWATER);

		float depth2 = 0.0;
		float depthN = 0.0;
				
		if (pixelIsUnderWater)
		{// When looking up at the water plane, invert the heightmaps...
			height = 1.0 - height;
			chopheight = 1.0 - chopheight;

			level = level + (height * waveHeight);
			surfacePoint.xyz += lightingNormal * height * waveHeight;
			lightingNormal.y *= -1.0;
			lightingNormal = normalize(lightingNormal);

			depth = length(surfacePoint - position) * underWaterClarity;
			depth2 = position.y - surfacePoint.y;
			depthN = depth * fadeSpeed;

#if 0 // i think it looks better with the extra transparancy...
			if (position.y <= level)
			{// This pixel is below the water line... Fast path...
				vec3 waterCol = clamp(length(sunColor) / vec3(sunScale), 0.0, 1.0);
				waterCol = waterCol * mix(waterColorShallow, waterColorDeep, clamp(depth2 / extinction, 0.0, 1.0));

				color2 = color2 - color2 * clamp(depth2 / extinction, 0.0, 1.0);
				color = mix(color2, waterCol, clamp(depthN / visibility, 0.0, 1.0));

				gl_FragColor = vec4(color, 1.0);
				return;
			}
#endif
		}
		else
		{
			level = level + (height * waveHeight);
			
			//surfacePoint.xyz += vec3(0.0, 1.0, 0.0)/*lightingNormal*/ * height * waveHeight;
			//surfacePoint.y = level;
			
			//surfacePoint.xyz = waterMapLower.xyz;
			//surfacePoint.y += height * waveHeight;

			depth = length(position - surfacePoint) * waterClarity;
			depth2 = surfacePoint.y - position.y;
			depthN = depth * fadeSpeed;
		}

		eyeVecNorm = normalize(ViewOrigin - surfacePoint);

		if (pixelIsUnderWater)
		{
			if (position.y < level)
			{
				gl_FragColor = vec4(color2, 1.0);
				return;
			}

			if (depth2 < 0.0 /*&& waterMapLower.y > surfacePoint.y*/)
			{// Waves against shoreline. Pixel is below waterLevel + waveHeight...
				gl_FragColor = vec4(color2, 1.0);
				return;
			}
		}
		else
		{
			if (position.y > level)
			{
				gl_FragColor = vec4(color2, 1.0);
				return;
			}

			if (depth2 < 0.0 && waterMapLower.y < surfacePoint.y)
			{// Waves against shoreline. Pixel is above waterLevel + waveHeight... (but ignore anything marked as actual water - eg: not a shoreline)
				gl_FragColor = vec4(color2, 1.0);
				return;
			}
		}

		//depth = length(position - surfacePoint);
		//depth2 = surfacePoint.y - position.y;
		//depthN = depth * fadeSpeed;


		texCoord = uv;//var_TexCoords.xy;
		texCoord.x += sin(timer * 0.002 + 3.0 * abs(position.y)) * (refractionScale * min(depth2, 1.0));
		vec3 refraction = textureLod(u_DiffuseMap, texCoord, 0.0).rgb;
		

		float waterCol = clamp(length(sunColor) / sunScale, 0.0, 1.0);
		
		vec3 refraction1 = mix(refraction, waterColorShallow * vec3(waterCol), clamp(vec3(depthN) / vec3(visibility), 0.0, 1.0));
		refraction = mix(refraction1, waterColorDeep * vec3(waterCol), clamp((vec3(depth2) / vec3(extinction)), 0.0, 1.0));

		vec4 foam = vec4(0.0);
		float causicStrength = 1.0; // Scale back causics where there's foam, and over distance...

		float pixDist = distance(surfacePoint.xyz, ViewOrigin.xyz);
		causicStrength *= 1.0 - clamp(pixDist / 1024.0, 0.0, 1.0) * (1.0 - clamp(depth2 / extinction.y, 0.0, 1.0));

#if !defined(__LQ_MODE__)
		if (USE_OCEAN > 0.0)
		{
			if (depth2 < foamExistence.x)
			{
				foam = GetFoamMap(origWaterMapUpper.xzy);
			}
			else if (depth2 < foamExistence.y)
			{
				foam = mix(GetFoamMap(origWaterMapUpper.xzy), vec4(0.0), (depth2 - foamExistence.x) / (foamExistence.y - foamExistence.x));
			}

			if (waveHeight - foamExistence.z > 0.0001)
			{
				foam += GetFoamMap(origWaterMapUpper.xzy + foamExistence.z) * clamp((level - (waterLevel + foamExistence.z)) / (waveHeight - foamExistence.z), 0.0, 1.0);
			}
		}
#endif //!defined(__LQ_MODE__)
		
		vec3 finalSunColor = u_PrimaryLightColor;

		if (SUN_VISIBILITY > 0.0)
		{
			vec3 sunsetSun = vec3(1.0, 0.8, 0.625);
			finalSunColor = mix(u_PrimaryLightColor, sunsetSun, SHADER_NIGHT_SCALE);
		}

		float refractionFresnel = pow(fresnelTerm(lightingNormal, eyeVecNorm), 0.5);
		vec3 refractedDeepColor = mix(refraction, waterColorDeep, refractionFresnel * 0.001);

		// bteitler:
		// p: point on ocean surface to get color for
		// n: normal on ocean surface at <p>
		// l: light (sun) direction
		// eye: ray direction from camera position for this pixel
		// dist: distance from camera to point <p> on ocean surface
		color = getSeaColor(surfacePoint, lightingNormal, normalize(ViewOrigin.xyz - u_PrimaryLightOrigin.xzy), eyeVecNorm, surfacePoint - ViewOrigin.xyz, finalSunColor, refractedDeepColor.rgb/*waterColorDeep.rgb*/, waterColorShallow.rgb, height);

#if defined(USE_REFLECTION) && !defined(__LQ_MODE__)
		if (!pixelIsUnderWater && u_Local1.g >= 2.0)
		{
			color = AddReflection(var_TexCoords, position, vec3(surfacePoint.x/*waterMapLower3.x*/, level, surfacePoint.z/*waterMapLower3.z*/), color.rgb, height, surfacePoint.xyz);
		}
#endif //defined(USE_REFLECTION) && !defined(__LQ_MODE__)

		vec3 caustic = color * GetCausicMap(vec3(positionMap.x, positionMap.z, (positionMap.z + level) / 16.0) / 3.0);
		causicStrength *= 1.0 - clamp(length(foam.rgb * 3.0), 0.0, 1.0); // deduct roam strength from causics...
		causicStrength *= 1.0 - clamp(height * 4.0, 0.0, 1.0); // deduct height from causics...
		color = clamp(color + (caustic * causicStrength * 0.5), 0.0, 1.0);

#if !defined(__LQ_MODE__)
		if (USE_OCEAN > 0.0)
		{
			color += foam.rgb * u_PrimaryLightColor;
		}
#endif //!defined(__LQ_MODE__)

		color = mix(refraction, color, clamp(depth * shoreHardness, 0.0, 1.0));
		color = mix(color, color2, 1.0 - clamp(waterClarity2 * depth, 0.8, 1.0));


		// add white caps...
		float whiteCaps = pow(clamp(chopheight, 0.0, 1.0), 16.0);
		if (whiteCaps > 0.0)
		{
			float wcMap = SmoothNoise( surfacePoint.xzy*0.0005 );
			color.rgb = color.rgb + (wcMap * whiteCaps);
		}

		if (USE_OCEAN > 0.0)
		{// add breaking waves
#if 1
			float aboveSeabed = 0.0;

			if (HAVE_HEIGHTMAP > 0.0)
			{
				float hMap = GetHeightMap(surfacePoint.xzy);
				aboveSeabed = (1.0 - clamp(hMap / 512.0, 0.0, 1.0));
			}
			else
			{
				aboveSeabed = (1.0 - clamp(depth2 / 512.0, 0.0, 1.0));
			}

			float breakingWave = pow(clamp(clamp(height - 0.333, 0.0, 1.0) + clamp((height * chopheight) - 0.25, 0.0, 1.0), 0.0, 1.0), 8.0) * aboveSeabed * max(lightingNormal.x * 0.5 + 0.5, lightingNormal.z * 0.5 + 0.5);
			if (breakingWave > 0.0)
			{
				float breakingMap = max(SmoothNoise( surfacePoint.xzy*0.005 ), SmoothNoise( surfacePoint.xzy*0.01 ));
				color.rgb = color.rgb + (breakingMap * pow(breakingWave, 0.15) * pow(aboveSeabed, clamp(1.0 - aboveSeabed, 0.1, 1.0)));

				float breakingWhiteCaps = pow(clamp((breakingWave * 0.25) + chopheight * 0.75, 0.0, 1.0), 4.0);
				if (breakingWhiteCaps > 0.0)
				{
					float wcMap = SmoothNoise( surfacePoint.xzy*0.0005 );
					color.rgb = color.rgb + (wcMap * breakingWhiteCaps);
				}
			}
#else
			float seaFloor = position.y;

			if (HAVE_HEIGHTMAP > 0.0)
			{
				seaFloor = GetHeightMap(waterMapLower.xzy/*surfacePoint.xzy*/);
			}
			
			float belowWaterHt = max(waterMapLower.y - seaFloor, 0.0) * u_Local0.r;
			float waveHt = height * waveHeight;

			if (waveHt >= belowWaterHt)
			{// Breaking wave...
				float breakingFoamFactor = pow(belowWaterHt / waveHt, u_Local0.g);
				float wcMap = SmoothNoise( surfacePoint.xzy*0.0005 );
				color.rgb = color.rgb + (wcMap * breakingFoamFactor);
			}
#endif
		}

		if (pixelIsUnderWater)
		{
			if (position.y < level)
				color = color2;
		}
		else
		{
			if (position.y > level)
				color = color2;
		}

#ifdef __DEBUG__
		if (u_Local0.r == 1.0)
		{
			gl_FragColor = vec4(normal.rbg * 0.5 + 0.5, 1.0);
			return;
		}
		else if (u_Local0.r == 2.0)
		{
			gl_FragColor = vec4(normal.rbg, 1.0);
			return;
		}
		else if (u_Local0.r == 3.0)
		{
			gl_FragColor = vec4(lightingNormal.rbg * 0.5 + 0.5, 1.0);
			return;
		}
		else if (u_Local0.r == 4.0)
		{
			gl_FragColor = vec4(lightingNormal.rbg, 1.0);
			return;
		}
		else if (u_Local0.r == 5.0)
		{
			gl_FragColor = vec4(fresnel, fresnel, fresnel, 1.0);
			return;
		}
		/*else if (u_Local0.r == 6.0)
		{
			gl_FragColor = vec4(fresnel2.xyz, 1.0);
			return;
		}*/
		else if (u_Local0.r == 6.0)
		{
			gl_FragColor = vec4(height, height, height, 1.0);
			return;
		}
#endif //__DEBUG__
	}

	gl_FragColor = vec4(color, 1.0);
}
