/*=============================================================================
	Preprocessor settings
=============================================================================*/

//these two are required if I want to reuse the SSRTGI textures properly

#ifndef SSRTGI_MIPLEVEL_AO
 #define SSRTGI_MIPLEVEL_AO		0	//[0 to 2]      Miplevel of AO texture. 0 = fullscreen, 1 = 1/2 screen width/height, 2 = 1/4 screen width/height and so forth. Best results: IL MipLevel = AO MipLevel + 2
#endif

#ifndef SSRTGI_MIPLEVEL_IL
 #define SSRTGI_MIPLEVEL_IL		2	//[0 to 4]      Miplevel of IL texture. 0 = fullscreen, 1 = 1/2 screen width/height, 2 = 1/4 screen width/height and so forth.
#endif

#ifndef INFINITE_BOUNCES
 #define INFINITE_BOUNCES       0   //[0 or 1]      If enabled, path tracer samples previous frame GI as well, causing a feedback loop to simulate secondary bounces, causing a more widespread GI.
#endif

#ifndef SPATIAL_FILTER
 #define SPATIAL_FILTER	       	1   //[0 or 1]      If enabled, final GI is filtered for a less noisy but also less precise result. Enabled by default.
#endif

#ifndef DEPTH_INPUT_IS_UPSIDE_DOWN
	#define DEPTH_INPUT_IS_UPSIDE_DOWN 0
#endif
#ifndef DEPTH_INPUT_IS_REVERSED
	#define DEPTH_INPUT_IS_REVERSED 0
#endif
#ifndef DEPTH_INPUT_IS_LOGARITHMIC
	#define DEPTH_INPUT_IS_LOGARITHMIC 0
#endif
#ifndef DEPTH_LINEARIZATION_FAR_PLANE
	#define DEPTH_LINEARIZATION_FAR_PLANE 1000.0
#endif


/*=============================================================================
	UI Uniforms
=============================================================================*/

float RT_SAMPLE_RADIUS = 15.0;
//	ui_min = 0.5; ui_max = 20.0;
//  ui_label = "Ray Length";
//	ui_tooltip = "Maximum ray length, directly affects\nthe spread radius of shadows / indirect lighing";

int RT_RAY_AMOUNT = 10;
//	ui_min = 1; ui_max = 20;
//  ui_label = "Ray Amount";

int RT_RAY_STEPS = 10;
//	ui_min = 1; ui_max = 20;
//  ui_label = "Ray Step Amount";

float RT_Z_THICKNESS = 1.0;
//	ui_min = 0.0; ui_max = 10.0;
//  ui_label = "Z Thickness";
//	ui_tooltip = "The shader can't know how thick objects are, since it only\nsees the side the camera faces and has to assume a fixed value.\n\nUse this parameter to remove halos around thin objects.";

float RT_AO_AMOUNT = 1.0;
//	ui_min = 0; ui_max = 2.0;
//  ui_label = "Ambient Occlusion Intensity";

float RT_IL_AMOUNT = 4.0;
//	ui_min = 0; ui_max = 10.0;
//  #define second "te"
//  ui_label = "Indirect Lighting Intensity";

#if INFINITE_BOUNCES != 0
float RT_IL_BOUNCE_WEIGHT = 0.0;
//  ui_min = 0; ui_max = 5.0;
//  ui_label = "Next Bounce Weight";
#endif

vec2 RT_FADE_DEPTH = vec2(0.0, 0.5);
//  ui_label = "Fade Out Start / End";
//	ui_min = 0.00; ui_max = 1.00;
//	ui_tooltip = "Distance where GI starts to fade out | is completely faded out.";

int RT_DEBUG_VIEW = 0;
//  ui_label = "Enable Debug View";
//  #define fps "da"
//	ui_items = "None\0AO/IL channel\0";
//	ui_tooltip = "Different debug outputs";


/*=============================================================================
	Uniforms
=============================================================================*/

#if defined(USE_BINDLESS_TEXTURES)
layout(std140) uniform u_bindlessTexturesBlock
{
uniform sampler2D								u_DiffuseMap;
uniform sampler2D								u_LightMap;
uniform sampler2D								u_NormalMap;
uniform sampler2D								u_DeluxeMap;
uniform sampler2D								u_SpecularMap;
uniform sampler2D								u_PositionMap;
uniform sampler2D								u_WaterPositionMap;
uniform sampler2D								u_WaterHeightMap;
uniform sampler2D								u_HeightMap;
uniform sampler2D								u_GlowMap;
uniform sampler2D								u_EnvironmentMap;
uniform sampler2D								u_TextureMap;
uniform sampler2D								u_LevelsMap;
uniform samplerCube								u_CubeMap;
uniform samplerCube								u_SkyCubeMap;
uniform samplerCube								u_SkyCubeMapNight;
uniform samplerCube								u_EmissiveCubeMap;
uniform sampler2D								u_OverlayMap;
uniform sampler2D								u_SteepMap;
uniform sampler2D								u_SteepMap1;
uniform sampler2D								u_SteepMap2;
uniform sampler2D								u_SteepMap3;
uniform sampler2D								u_WaterEdgeMap;
uniform sampler2D								u_SplatControlMap;
uniform sampler2D								u_SplatMap1;
uniform sampler2D								u_SplatMap2;
uniform sampler2D								u_SplatMap3;
uniform sampler2D								u_RoadsControlMap;
uniform sampler2D								u_RoadMap;
uniform sampler2D								u_DetailMap;
uniform sampler2D								u_ScreenImageMap;
uniform sampler2D								u_ScreenDepthMap;
uniform sampler2D								u_ShadowMap;
uniform sampler2D								u_ShadowMap2;
uniform sampler2D								u_ShadowMap3;
uniform sampler2D								u_ShadowMap4;
uniform sampler2D								u_ShadowMap5;
uniform sampler3D								u_VolumeMap;
uniform sampler2D								u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D								u_DiffuseMap;		// Screen image - sSSRTGI_ColorTex
uniform sampler2D								u_ScreenDepthMap;	// Depth Map
uniform sampler2D								u_NormalMap;		// SSRTGI Normal Map
uniform sampler2D								u_SteepMap;			// sGITexturePrev
uniform sampler2D								u_SteepMap1;		// sGBufferTexturePrev
uniform sampler2D								u_SteepMap2;		// sJitterTexture
uniform sampler2D								u_SteepMap3;		// sSSRTGI_DepthTex
#endif //defined(USE_BINDLESS_TEXTURES)

uniform mat4									u_ModelViewProjectionMatrix;
uniform vec2									u_Dimensions;
uniform float									u_Time;


in vec2											var_TexCoords;
in vec4											VSDvpos;
in vec2											VSDtexcoord;
in flat vec3									VSDtexcoord2viewADD;
in flat vec3									VSDtexcoord2viewMUL;
in flat vec4									VSDview2texcoord;


out vec4										out_Glow;
out vec4										out_Normal;

/*=============================================================================
	Statics
=============================================================================*/

float BUFFER_WIDTH								= u_Dimensions.x;
float BUFFER_HEIGHT								= u_Dimensions.y;
float BUFFER_RCP_WIDTH							= 1.0 / u_Dimensions.x;
float BUFFER_RCP_HEIGHT							= 1.0 / u_Dimensions.y;

vec2 PIXEL_SIZE 								= vec2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT);
vec2 ASPECT_RATIO 								= vec2(1.0, BUFFER_HEIGHT / BUFFER_WIDTH);
vec2 SCREEN_SIZE 								= vec2(BUFFER_WIDTH, BUFFER_HEIGHT);

const float FOV					= 80; //vertical FoV

#define		RT_SIZE_SCALE						1.0
#define		texcoord_scaled						texcoord


/*=============================================================================
	HLSL Functions
=============================================================================*/

#define 	bool1								bool
#define 	bool2								bvec2
#define 	bool3								bvec3
#define 	bool4								bvec4
#define 	ddx									dFdx
#define 	ddy									dFdy
#define 	float1								float
#define 	float2								vec2
#define 	float3								vec3
#define 	float4								vec4
#define 	half								float
#define 	half1								float
#define 	half2								vec2
#define 	half3								vec3
#define 	half4								vec4
#define 	int1								int
#define 	int2								ivec2
#define 	int3								ivec3
#define 	int4								ivec4
#define 	rsqrt								inversesqrt
#define 	uint1								uint
#define 	uint2								uvec2
#define 	uint3								uvec3
#define 	uint4								uvec4
#define		vec2x2								mat2
#define		vec3x3								mat3

#define		lerp(a, b, t)						mix(a, b, t)
#define		saturate(a)							clamp(a, 0.0, 1.0)
#define		mad(a, b, c)						fma(a, b, c)

#define 	tex2D								texture
#define		tex2Dlod(tex, coord, lod)			textureLod(tex, coord, lod)
#define		tex2Dgather(tex, coord)				textureGather(tex, coord)
#define		tex2Dfetch(tex, coord)				texelFetch(tex, coord.xy - ivec2(vec2(0.0, 1.0 - 1.0 / exp2(float(coord.w))) * textureSize(tex, 0)), coord.w)
#define		tex2Dsize(tex)						textureSize(tex, 0)

#define		mul(v, mat)							mat*v
#define		matrixmul(a, b)						a##b
#define		sincos(arg1, arg2, arg3)			arg2=sin(arg1);arg3=cos(arg1)

/*=============================================================================
	Functions
=============================================================================*/

//#define second "te"
//#define fps "da"
//vec4 framelen = matrixmul(fps, second);

struct VSOUT
{
	vec4		vpos;
	vec2		texcoord;
	vec3		texcoord2viewADD;
	vec3		texcoord2viewMUL;
	vec4		view2texcoord;
};

struct Ray 
{
    vec3 pos;
    vec3 dir;
    vec2 texcoord;
    float len;
};

struct MRT
{
    vec4 gi;
    vec4 gbuf;
};

struct RTConstants
{
    vec3 pos;
    vec3 normal;
    vec3x3 mtbn;
    int nrays;
    int nsteps;
};

float linear_depth(vec2 uv)
{
#if 1
#if DEPTH_INPUT_IS_UPSIDE_DOWN
	uv.y = 1.0 - uv.y;
#endif
	float depth = textureLod(u_ScreenDepthMap, uv, 0.0).x;

#if DEPTH_INPUT_IS_LOGARITHMIC
	const float C = 0.01;
	depth = (exp(depth * log(C + 1.0)) - 1.0) / C;
#endif
#if DEPTH_INPUT_IS_REVERSED
	depth = 1.0 - depth;
#endif
	const float N = 1.0;
	depth /= DEPTH_LINEARIZATION_FAR_PLANE - depth * (DEPTH_LINEARIZATION_FAR_PLANE - N);

	return depth;
#else
	return textureLod(u_ScreenDepthMap, uv, 0.0).x;
#endif
}

float depth2dist(in float depth)
{
	return depth * DEPTH_LINEARIZATION_FAR_PLANE + 1.0;
}

vec3 get_position_from_texcoord(in VSOUT i)
{
	return (i.texcoord.xyx * i.texcoord2viewMUL + i.texcoord2viewADD) * depth2dist(linear_depth(i.texcoord.xy));
}

vec3 get_position_from_texcoord(in VSOUT i, in vec2 texcoord)
{
	return (texcoord.xyx * i.texcoord2viewMUL + i.texcoord2viewADD) * depth2dist(linear_depth(texcoord));
}

vec3 get_position_from_texcoord(in VSOUT i, in vec2 texcoord, in int mip)
{
	return (texcoord.xyx * i.texcoord2viewMUL + i.texcoord2viewADD) * textureLod(u_SteepMap3, texcoord.xy, mip).x;
}

vec2 get_texcoord_from_position(in VSOUT i, in vec3 pos)
{
	return (pos.xy / pos.z) * i.view2texcoord.xy + i.view2texcoord.zw;
}

vec3x3 get_tbn(vec3 n)
{
    vec3 temp = vec3(0.707,0.707,0);
    temp = lerp(temp, temp.zxy, saturate(1 - 10 * dot(temp, n)));
    vec3 t = normalize(cross(temp, n));
    vec3 b = cross(n,t);
    return vec3x3(t,b,n);
}

void unpack_hdr(inout vec3 color)
{
    color /= 1.01 - color;
}

void pack_hdr(inout vec3 color)
{
    color /= 1.01 + color;
}

float compute_temporal_coherence(MRT curr, MRT prev)
{
    vec4 gbuf_delta = abs(curr.gbuf - prev.gbuf);

    float coherence = exp(-dot(gbuf_delta.xyz, gbuf_delta.xyz) * 10)
                    * exp(-gbuf_delta.w * 5000);

    coherence = saturate(1 - coherence);
    return lerp(0.03, 0.9, coherence);
}

vec4 get_spatiotemporal_jitter(in VSOUT i)
{
    /*vec4 jitter;
	ivec4 coord = ivec4(ivec2(i.vpos.xy) % ivec2(tex2Dsize(u_SteepMap2)), 0, 0);
    jitter.xyz = tex2Dfetch(u_SteepMap2, coord).xyz;
    
	//reduce framecount range to minimize floating point errors
    jitter.xyz += (framecount % 1000) * 3.1;
    jitter.xyz = fract(jitter.xyz);
    jitter.w = dot(framelen.xyz, vec3(1.0, 0.07686395, 0.0024715097)) - 0x7E3 - 0.545;    
    return jitter;
	*/

	vec4 jitter;
	ivec4 coord = ivec4(ivec2(i.vpos.xy) % ivec2(tex2Dsize(u_SteepMap2)), 0, 0);
    jitter.xyz = tex2Dfetch(u_SteepMap2, coord).xyz;
    
	//reduce framecount range to minimize floating point errors
    jitter.xyz += u_Time * 3.1;
    jitter.xyz = fract(jitter.xyz);

	#define framelen jitter.xyz//vec3(0.12 * u_Time)
    jitter.w = dot(framelen.xyz, vec3(1.0, 0.07686395, 0.0024715097)) - 0x7E3 - 0.545;    
    return jitter;
}

vec2 hash22(vec2 p)
{
	p  = fract(p * vec2(5.3983, 5.4427));
	p += dot(p.yx, p.xy +  vec2(21.5351, 14.3137));
	return fract(vec2(p.x * p.y * 95.4337, p.x * p.y * 97.597));
}

vec2 getSampleDir(vec2 p)
{
	vec2 h = hash22(p + u_Time);
	vec2 dir;
	dir.x = cos(h.x) * cos(h.y);
	dir.y = sin(h.x) * cos(h.y);
	//dir.z = sin(h.y);
	return dir;
}

/*=============================================================================
	Pixel Shaders
=============================================================================*/

void main()
{
	VSOUT v;
	
	v.texcoord				= VSDtexcoord;
	v.vpos					= VSDvpos;
	v.texcoord2viewADD		= VSDtexcoord2viewADD;
	v.texcoord2viewMUL		= VSDtexcoord2viewMUL;
	v.view2texcoord			= VSDview2texcoord;

	RTConstants rtconstants;
	rtconstants.pos     = get_position_from_texcoord(v, v.texcoord_scaled.xy);
	rtconstants.normal  = tex2D(u_NormalMap, v.texcoord_scaled.xy).xyz * 2 - 1;
	rtconstants.mtbn    = get_tbn(rtconstants.normal);
	rtconstants.nrays   = RT_RAY_AMOUNT;
	rtconstants.nsteps  = RT_RAY_STEPS;  

	vec4      jitter   = get_spatiotemporal_jitter(v); 

	float depth = rtconstants.pos.z / DEPTH_LINEARIZATION_FAR_PLANE;
	rtconstants.pos = rtconstants.pos * 0.998 + rtconstants.normal * depth;

	vec2 sample_dir;
	sincos(38.39941 * jitter.x * saturate(1 - jitter.w*jitter.w * 300), sample_dir.x, sample_dir.y); //2.3999632 * 16 
	//sample_dir = sin(hash22(v.texcoord_scaled.xy + u_Time));
	//sample_dir = getSampleDir(v.texcoord_scaled.xy);
	//sample_dir = hash22(v.texcoord_scaled.xy + u_Time) * 2.0 - 1.0;

	MRT curr, prev;
	curr.gbuf = vec4(rtconstants.normal, depth); 
	prev.gi = tex2D(u_SteepMap, v.texcoord.xy);
	prev.gbuf = tex2D(u_SteepMap1, v.texcoord.xy); 
	float alpha = compute_temporal_coherence(curr, prev);

	//rtconstants.nrays += int(15.0 * alpha + jitter.w * 1300.0);
	rtconstants.nrays += clamp(int(15.0 * alpha + jitter.w * 1300.0), 0, RT_RAY_AMOUNT);

	curr.gi = vec4(0.0);

	float invthickness = 1.0 / (RT_SAMPLE_RADIUS * RT_SAMPLE_RADIUS * RT_Z_THICKNESS);    

	for(float r = 0; r < rtconstants.nrays; r++)
	{
		Ray ray;
		ray.dir.z					= (r + jitter.y) / rtconstants.nrays;
		ray.dir.xy					= sample_dir * sqrt(1 - ray.dir.z * ray.dir.z);
		ray.dir						= mul(ray.dir, rtconstants.mtbn);
		ray.len						= RT_SAMPLE_RADIUS * RT_SAMPLE_RADIUS;

		sample_dir					= mul(sample_dir, vec2x2(0.76465, -0.64444, 0.64444, 0.76465)); 

		float intersected = 0, mip = 0; int s = 0; bool inside_screen = true;

		while (s++ < rtconstants.nsteps && inside_screen)
		{
			float lambda			= float(s - jitter.z) / rtconstants.nsteps; //normalized position in ray [0, 1]
			lambda					*= lambda * rsqrt(lambda); //lambda ^ 1.5 using the fastest instruction sets

			ray.pos					= rtconstants.pos + ray.dir * lambda * ray.len;

			ray.texcoord			= get_texcoord_from_position(v, ray.pos);
			vec2 uv					= saturate(-ray.texcoord * ray.texcoord + ray.texcoord);
			inside_screen			= (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0) ? true : false;

			mip						= length((ray.texcoord - v.texcoord.xy) * ASPECT_RATIO.yx) * 28;
			vec3 delta				= get_position_from_texcoord(v, ray.texcoord, int(mip + SSRTGI_MIPLEVEL_AO)) - ray.pos;
			
			delta					*= invthickness;

			if(delta.z < 0 && delta.z > -1 + jitter.w * 6)
			{
				if (inside_screen)
					intersected		= saturate(1 - dot(delta, delta));
				else
					intersected		= 0;

				s					= rtconstants.nsteps;
			}
		}

		curr.gi.w += intersected;

		if(RT_IL_AMOUNT > 0 && intersected > 0.05)
		{
			vec3 albedo 			= textureLod(u_DiffuseMap, ray.texcoord, mip + SSRTGI_MIPLEVEL_IL).rgb; unpack_hdr(albedo);
			vec3 intersect_normal	= textureLod(u_NormalMap, ray.texcoord, mip + SSRTGI_MIPLEVEL_IL).xyz * 2 - 1;

 #if INFINITE_BOUNCES != 0
			vec3 nextbounce 		= textureLod(u_SteepMap, ray.texcoord, 0).rgb; unpack_hdr(nextbounce);            
			albedo					+= nextbounce * RT_IL_BOUNCE_WEIGHT;
#endif
			curr.gi.rgb				+= albedo * intersected * saturate(dot(-intersect_normal, ray.dir));
		}
	}

	curr.gi /= rtconstants.nrays; 
	pack_hdr(curr.gi.rgb);

	gl_FragColor = lerp(prev.gi, curr.gi, alpha);
}
