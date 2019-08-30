#define REAL_WAVES					// You probably always want this turned on.
//#define USE_REFLECTION				// Enable reflections on water. Define moved to renderer code.
#define FIX_WATER_DEPTH_ISSUES		// Use basic depth value for sky hits...
//#define EXPERIMENTAL_WATERFALL	// Experimental waterfalls...
#define EXPERIMENTAL_WATERFALL2	// Experimental waterfalls...
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

uniform mat4								u_ModelViewProjectionMatrix;

#if defined(USE_BINDLESS_TEXTURES)
layout(std140) uniform u_bindlessTexturesBlock
{
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_LightMap;
uniform sampler2D					u_NormalMap;
uniform sampler2D					u_DeluxeMap;
uniform sampler2D					u_SpecularMap;
uniform sampler2D					u_PositionMap;
uniform sampler2D					u_WaterPositionMap;
uniform sampler2D					u_WaterHeightMap;
uniform sampler2D					u_HeightMap;
uniform sampler2D					u_GlowMap;
uniform sampler2D					u_EnvironmentMap;
uniform sampler2D					u_TextureMap;
uniform sampler2D					u_LevelsMap;
uniform sampler2D					u_CubeMap;
uniform sampler2D					u_SkyCubeMap;
uniform sampler2D					u_SkyCubeMapNight;
uniform sampler2D					u_EmissiveCubeMap;
uniform sampler2D					u_OverlayMap;
uniform sampler2D					u_SteepMap;
uniform sampler2D					u_SteepMap1;
uniform sampler2D					u_SteepMap2;
uniform sampler2D					u_SteepMap3;
uniform sampler2D					u_WaterEdgeMap;
uniform sampler2D					u_SplatControlMap;
uniform sampler2D					u_SplatMap1;
uniform sampler2D					u_SplatMap2;
uniform sampler2D					u_SplatMap3;
uniform sampler2D					u_RoadsControlMap;
uniform sampler2D					u_RoadMap;
uniform sampler2D					u_DetailMap;
uniform sampler2D					u_ScreenImageMap;
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2D					u_ShadowMap;
uniform sampler2D					u_ShadowMap2;
uniform sampler2D					u_ShadowMap3;
uniform sampler2D					u_ShadowMap4;
uniform sampler2D					u_ShadowMap5;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D							u_DiffuseMap;			// backBufferMap
uniform sampler2D							u_PositionMap;

uniform sampler2D							u_OverlayMap;			// foamMap 1
uniform sampler2D							u_SplatMap1;			// foamMap 2
uniform sampler2D							u_SplatMap2;			// foamMap 3
uniform sampler2D							u_SplatMap3;			// foamMap 4

uniform sampler2D							u_DetailMap;			// causics map

uniform sampler2D							u_HeightMap;			// map height map

uniform sampler2D							u_WaterPositionMap;

uniform sampler2D							u_EmissiveCubeMap;		// water reflection image... (not really a cube, just reusing using the uniform)

uniform samplerCube							u_SkyCubeMap;
uniform samplerCube							u_SkyCubeMapNight;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4								u_Mins;					// MAP_MINS[0], MAP_MINS[1], MAP_MINS[2], 0.0
uniform vec4								u_Maxs;					// MAP_MAXS[0], MAP_MAXS[1], MAP_MAXS[2], 0.0
uniform vec4								u_MapInfo;				// MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], SUN_VISIBILITY

uniform vec4								u_Local0;				// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec4								u_Local1;				// MAP_WATER_LEVEL, USE_GLSL_REFLECTION, IS_UNDERWATER, WATER_REFLECTIVENESS
uniform vec4								u_Local2;				// WATER_COLOR_SHALLOW_R, WATER_COLOR_SHALLOW_G, WATER_COLOR_SHALLOW_B, WATER_CLARITY
uniform vec4								u_Local3;				// WATER_COLOR_DEEP_R, WATER_COLOR_DEEP_G, WATER_COLOR_DEEP_B, HAVE_HEIGHTMAP
uniform vec4								u_Local4;				// SHADER_NIGHT_SCALE, WATER_EXTINCTION1, WATER_EXTINCTION2, WATER_EXTINCTION3
uniform vec4								u_Local7;				// testshadervalue1, etc
uniform vec4								u_Local8;				// testshadervalue5, etc
uniform vec4								u_Local9;				// SUN_VISIBILITY, SHADER_NIGHT_SCALE, 0.0, 0.0
uniform vec4								u_Local10;				// waveHeight, waveDensity, USE_OCEAN, WATER_UNDERWATER_CLARITY
uniform vec4								u_Local11;				// CONTRAST, SATURATION, BRIGHTNESS, 0.0

uniform vec2								u_Dimensions;
uniform vec4								u_ViewInfo;				// zmin, zmax, zmax / zmin

varying vec2								var_TexCoords;
varying vec3								var_ViewDir;

uniform vec4								u_PrimaryLightOrigin;
uniform vec3								u_PrimaryLightColor;

#define										MAP_WATER_LEVEL		u_Local1.r
#define										HAVE_HEIGHTMAP		u_Local3.a
#define										SHADER_NIGHT_SCALE	u_Local4.r
#define										SUN_VISIBILITY			u_Local9.r

/*uniform int								u_lightCount;
uniform vec3								u_lightPositions2[MAX_DEFERRED_LIGHTS];
uniform float								u_lightDistances[MAX_DEFERRED_LIGHTS];
uniform float								u_lightHeightScales[MAX_DEFERRED_LIGHTS];
uniform vec3								u_lightColors[MAX_DEFERRED_LIGHTS];*/

#define CONTRAST_STRENGTH					u_Local11.r
#define SATURATION_STRENGTH					u_Local11.g
#define BRIGHTNESS_STRENGTH					u_Local11.b

// Position of the camera
uniform vec3								u_ViewOrigin;
#define ViewOrigin							u_ViewOrigin.xzy

// Timer
uniform float								u_Time;
#define systemtimer							(u_Time * 5000.0)


// Over-all water clearness...
const float waterClarity2 =					0.03;

#define waterClarity						u_Local2.a
#define underWaterClarity					u_Local10.a

// How fast will colours fade out. You can also think about this
// values as how clear water is. Therefore use smaller values (eg. 0.05f)
// to have crystal clear water and bigger to achieve "muddy" water.
const float fadeSpeed =						0.15;

// Normals scaling factor
const float normalScale =					1.0;

// R0 is a constant related to the index of refraction (IOR).
// It should be computed on the CPU and passed to the shader.
const float R0 =							0.5;

// Maximum waves amplitude
#define waveHeight							u_Local10.r
#define waveDensity							u_Local10.g

// Colour of the sun
//vec3 sunColor = (u_PrimaryLightColor.rgb + vec3(1.0) + vec3(1.0) + vec3(1.0)) / 4.0; // 1/4 real sun color, 3/4 white...
vec3 sunColor =								vec3(1.0);

// The smaller this value is, the more soft the transition between
// shore and water. If you want hard edges use very big value.
// Default is 1.0f.
const float shoreHardness =					0.2;

// This value modifies current fresnel term. If you want to weaken
// reflections use bigger value. If you want to empasize them use
// value smaller then 0. Default is 0.0.
const float refractionStrength =			0.0;

// Modifies 4 sampled normals. Increase first values to have more
// smaller "waves" or last to have more bigger "waves"
const vec4 normalModifier =					vec4(1.0, 2.0, 4.0, 8.0);

// Describes at what depth foam starts to fade out and
// at what it is completely invisible. The third value is at
// what height foam for waves appear (+ waterLevel).
#define foamExistence						vec3(8.0, 50.0, waveHeight)

const float sunScale =						3.0;

const float shininess =						0.7;
const float specularScale =					0.07;


// Colour of the water surface
vec3 waterColorShallow =					u_Local2.rgb;

// Colour of the water depth
vec3 waterColorDeep =						u_Local3.rgb;

vec3 extinction =							u_Local4.gba;

// Water transparency along eye vector.
const float visibility =					32.0;

// Increase this value to have more smaller waves.
const vec2 scale =							vec2(0.002, 0.002);
const float refractionScale =				0.005;

#define USE_OCEAN							u_Local10.b


// For all settings: 1.0 = 100% 0.5=50% 1.5 = 150%
vec3 ContrastSaturationBrightness(vec3 color, float con, float sat, float brt)
{
	// Increase or decrease theese values to adjust r, g and b color channels seperately
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5;
	const float AvgLumB = 0.5;
	
	const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
	
	vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
	vec3 brtColor = color * brt;
	vec3 intensity = vec3(dot(brtColor, LumCoeff));
	vec3 satColor = mix(intensity, brtColor, sat);
	vec3 conColor = mix(AvgLumin, satColor, con);
	return conColor;
}

float hash( vec2 p ) {
    float h = dot(p,vec2(127.1,311.7));	
	//return fract(sin(h)*83758.5453123);
	return fract(sin(h)*4378.5453);
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

const mat2 m = mat2( 0.00,  0.80,
                    -0.80,  0.36);

float SmoothNoise( vec2 p )
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
    fresnel = pow(fresnel,3.0) * 0.45;

	// bteitler: refraction effect based on angle between light surface normal
	float d = diffuse(-n, l, 1.0);
    //vec3 refr = SEA_BASE + pow(d, 60.0) * SEA_WATER_COLOR * 0.27 * lightColor; 
	vec3 refr = SEA_BASE + pow(d, 60.0) * 0.15 * SEA_WATER_COLOR * lightColor;
        
	float refrStr = height;

    // bteitler: Bounce eye ray off ocean towards sky, and get the color of the sky
    vec3 reflected = mix(getSkyColor(reflect(eye, n))*0.6/*0.99*/, refr, refrStr);    
    
    // bteitler: refraction effect based on angle between light surface normal
	vec3 refCol = mix(SEA_BASE, refr, refrStr*waterClarity);
	vec3 swCol = mix(SEA_WATER_COLOR, refr, refrStr*waterClarity);
    vec3 refracted = refCol + diffuse(n, l, 80.0) * swCol * 0.27 * lightColor; 
    
    // bteitler: blend the refracted color with the reflected color based on our fresnel term
    vec3 color = mix(refracted, reflected, fresnel);
    
    // bteitler: Apply a distance based attenuation factor which is stronger
    // at peaks
    //float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
    //color += SEA_WATER_COLOR * height * 0.15 * atten;
    
    // bteitler: Apply specular highlight
    color += vec3(specular(n, l, eye, 90.0)) * 0.5 * lightColor;

	color += clamp(diffuse(n, -l, 40.0) * 0.333, 0.0, 1.0) * lightColor;

	float dt = clamp(dot(n, -eye), 0.0, 1.0);
	color += vec3(clamp(dt * 1.25, 0.0, 1.0)) * lightColor;

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
		res = mix(res, noise(position+res), 0.25);
		
		//float res2 = wave(position, p, speed, phase, 0.006);
		float res2 = res + noise(position)*0.15;

		position -= wavedrag(position, p) * (res - res2) * weight * DRAG_MULT;
		
		//w += res * weight;
		w += pow(res * weight, 2.0) * 1.5;

		iter += 12.0;
		ws += weight;
		weight = mix(weight, 0.0, float(i + 1) / 3.0/*4.0*/);
		phase *= 1.2;
		speed *= 1.02;
	}
	return clamp(w / ws, 0.0, 1.0);
	//return clamp(((w / ws) - 0.075) * 1.075, 0.0, 1.0);
}

mat2 octave_m = mat2(1.6,1.2,-1.2,1.6);

float sea_octave(vec2 uv, float choppy) {
    uv += noise(uv.xy);
    vec2 wv = 1.0-abs(sin(uv));
    vec2 swv = abs(cos(uv));
    wv = mix(wv,swv,wv);
    return pow(1.0-pow(wv.x * wv.y,0.65),choppy);
}

float getwavesDetail( vec2 p )
{
    float freq = 0.434;//0.4;
    float amp = 0.961;//0.1;
    float choppy = 3.0;//5.0;
    vec2 uv = p.xy; uv.x *= 0.75;
    float sea_time = iTime * 0.8;
    float d, h = 0.0;    
    for(int i = 0; i < 3; i++) {        
		//d = sea_octave((uv+sea_time)*freq,choppy);
		//d += sea_octave((uv-sea_time)*freq,choppy);
		d = sea_octave((uv+sea_time)*freq,choppy) * 1.5;
		h += d * amp;        
		uv *= octave_m; freq *= 1.9; amp *= 0.22;
		choppy = mix(choppy,1.0,0.2);
	}
	return h;
}

float ApplyHeightAdjustments(in float height)
{
	float h = height * 1.05;
	h = pow(h, 1.5);
	return h;
}

#define waveDetailFactor 0.85//0.5//0.333

vec3 getNormalOcean(in vec2 p, in float eps, out float height) {
	vec3 n;
	vec2 p1 = p.xy;
	vec2 p2 = vec2(p.x+eps, p.y);
	vec2 p3 = vec2(p.x, p.y+eps);

    n.y = (ApplyHeightAdjustments(getwaves(p1)) + getwavesDetail(p1 * waveDetailFactor) * 0.125);
	height = n.y;
    n.x = (ApplyHeightAdjustments(getwaves(p2)) + getwavesDetail(p2 * waveDetailFactor) * 0.125) - n.y;
    n.z = (ApplyHeightAdjustments(getwaves(p3)) + getwavesDetail(p3 * waveDetailFactor) * 0.125) - n.y;
    n.y = eps*0.25; 
    return normalize(n);
}

vec3 getNormalDetailed(in vec2 p, in float eps, out float height) {
	vec3 n;
	vec2 p1 = p.xy;
	vec2 p2 = vec2(p.x+eps, p.y);
	vec2 p3 = vec2(p.x, p.y+eps);

    n.y = getwavesDetail(p1 * waveDetailFactor);
	height = n.y;
    n.x = getwavesDetail(p2 * waveDetailFactor) - n.y;
    n.z = getwavesDetail(p3 * waveDetailFactor) - n.y;
    n.y = eps*0.25; 
    return normalize(n);
}

void GetHeightAndNormal(in vec2 pos, in float e, in float depth, inout float height, inout float chopheight, inout vec3 waveNormal, inout vec3 lightingNormal, in vec3 eyeVecNorm, in float timer, in float level) {
#if !defined(__LQ_MODE__) && defined(REAL_WAVES)

	if (USE_OCEAN > 0.0)
	{
		waveNormal = getNormalOcean(pos.xy, (4.0 / u_Dimensions.x), height);
		lightingNormal = waveNormal;
		chopheight = height;
	}
	else
#endif //!defined(__LQ_MODE__) && defined(REAL_WAVES)
	{
		waveNormal = getNormalDetailed(pos.xy, (4.0 / u_Dimensions.x), height);
		lightingNormal = waveNormal;
		chopheight = height;
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
#elif defined(__LQ_MODE__) || !defined(REAL_WAVES)
	//
	// No big waves, so we don't need to trace, just grab this pixel's height...
	//
	GetHeightAndNormal(surfacePoint.xz*0.03, 0.004, 1.0, height, chopheight, waveNormal, lightingNormal, eyeVecNorm, timer, level);

	//surfacePoint.xyz = waterMapLower.xyz;
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
#ifdef EXPERIMENTAL_WATERFALL2
	float wf = clamp(pow(SmoothNoise((waterMapUpper.xz * vec2(0.1, 0.03)) + vec2(0.0, u_Time*4.0)), 1.5), 0.0, 1.0);
	//return vec4(wf, wf, wf, wf);
	wf = clamp(wf * 2.0, 0.0, 1.0) * 0.666;
	float wf2 = SmoothNoise(waterMapUpper.xz * vec2(0.3, 0.1) + vec2(0.0, u_Time*4.0));
	wf = (wf + wf2) / 2.0;
	vec3 wfc = mix(color, vec3(wf), wf);
	return vec4(wfc, 1.0);
#elif defined(EXPERIMENTAL_WATERFALL)
	//return vec4(0.5 + (0.5 * (1.0 - wfEdgeFactor)), 0.0, 0.0, 1.0);

	vec3 n = normalize(position);
	n.y += systemtimer * 0.00008;
	float rand = SmoothNoise( n.xy * u_Local7.r );

	for (float x = 2.0; x <= u_Local7.g; x *= 2.0)
	{
		rand *= SmoothNoise( n.xy * (u_Local7.r * x) );
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
			gl_FragColor = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, waterMapLower.a - 1024.0, 0.0, waterMapLower.a);
			return;
		}
		else if (waterMapUpper.a <= 0.0)
		{// near a waterfall, but this is not water.
			gl_FragColor = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, hitType - 1024.0, edgeFactor, hitType);
			return;
		}
		else
		{// Blend with water...
			color = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, waterMapLower.a - 1024.0, edgeFactor, hitType).rgb;
			color2 = color;
		}
	}
#else //!EXPERIMENTAL_WATERFALL
	if (waterMapLower.a >= 1024.0)
	{// Actual waterfall pixel...
		gl_FragColor = WaterFall(color.rgb, color2.rgb, waterMapUpper.xyz, position.xyz, timer, waterMapLower.a - 1024.0, 0.0, waterMapLower.a);
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
			
			surfacePoint.xyz = waterMapLower.xyz;
			surfacePoint.y += height * waveHeight;

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

		if (!(CONTRAST_STRENGTH == 1.0 && SATURATION_STRENGTH == 1.0 && BRIGHTNESS_STRENGTH == 1.0))
		{// C/S/B enabled...
			color.rgb = ContrastSaturationBrightness(color.rgb, CONTRAST_STRENGTH, SATURATION_STRENGTH, BRIGHTNESS_STRENGTH);
		}

#if defined(USE_REFLECTION) && !defined(__LQ_MODE__)
		if (!pixelIsUnderWater && u_Local1.g >= 2.0)
		{
			color = AddReflection(var_TexCoords, position, vec3(surfacePoint.x/*waterMapLower3.x*/, level, surfacePoint.z/*waterMapLower3.z*/), color.rgb, height, surfacePoint.xyz);
		}
#endif //defined(USE_REFLECTION) && !defined(__LQ_MODE__)

		/*
		// UQ1: Disabled because i cant be bothered fixing issues with it on the skybox right now...
		vec3 caustic = color * GetCausicMap(vec3(positionMap.x, positionMap.z, (positionMap.z + level) / 16.0) / 3.0);
		causicStrength *= 1.0 - clamp(length(foam.rgb * 3.0), 0.0, 1.0); // deduct roam strength from causics...
		causicStrength *= 1.0 - clamp(height * 4.0, 0.0, 1.0); // deduct height from causics...
		color = clamp(color + (caustic * causicStrength * 0.5), 0.0, 1.0);
		*/

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
		whiteCaps = clamp(whiteCaps, 0.0, 1.0);

		if (whiteCaps > 0.0)
		{
			float wcMap = SmoothNoise( surfacePoint.xz*0.0005 );
			wcMap = clamp(wcMap, 0.0, 1.0);

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
			breakingWave = clamp(breakingWave, 0.0, 1.0);

			if (breakingWave > 0.0)
			{
				float breakingMap = max(SmoothNoise( surfacePoint.xz*0.005 ), SmoothNoise( surfacePoint.xz*0.01 ));
				breakingMap = clamp(breakingMap, 0.0, 1.0);

				color.rgb = color.rgb + (breakingMap * pow(breakingWave, 0.15) * pow(aboveSeabed, clamp(1.0 - aboveSeabed, 0.1, 1.0)));

				float breakingWhiteCaps = pow(clamp((breakingWave * 0.25) + chopheight * 0.75, 0.0, 1.0), 4.0);
				breakingWhiteCaps = clamp(breakingWhiteCaps, 0.0, 1.0);

				if (breakingWhiteCaps > 0.0)
				{
					float wcMap = SmoothNoise( surfacePoint.xz*0.0005 );
					wcMap = clamp(wcMap, 0.0, 1.0);

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
				float wcMap = SmoothNoise( surfacePoint.xz*0.0005 );
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
