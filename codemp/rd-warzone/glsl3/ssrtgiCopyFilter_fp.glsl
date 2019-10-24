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
uniform sampler2D								u_SteepMap;			// sGITexture
uniform sampler2D								u_SteepMap1;		// sGBufferTexture
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

struct VSOUT
{
	vec4		vpos;
	vec2		texcoord;
	vec3		texcoord2viewADD;
	vec3		texcoord2viewMUL;
	vec4		view2texcoord;
};

struct MRT
{
    vec4 gi;
    vec4 gbuf;
};



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

	MRT o;

	o.gi    = tex2D(u_SteepMap,		v.texcoord.xy);
	o.gbuf  = tex2D(u_SteepMap1,	v.texcoord.xy);

#if SPATIAL_FILTER != 0
	float jitter = dot(floor(ivec2(v.vpos.xy) % 4 + 0.1), vec2(0.0625, 0.25)) + 0.0625;
	vec2 dir; sincos(2.3999632 * 16 * jitter, dir.x, dir.y);

	vec4 gi = o.gi;
	float weightsum = 1.0;

	for(int j = 0; j < 16; j++)
	{
		vec2 sample_texcoord = v.texcoord.xy + dir * PIXEL_SIZE * sqrt(j + jitter);   
		dir.xy = mul(dir.xy, vec2x2(0.76465, -0.64444, 0.64444, 0.76465)); 

		vec4 gi_tap   = tex2Dlod(u_SteepMap,	sample_texcoord, 0);
		vec4 gbuf_tap = tex2Dlod(u_SteepMap1,	sample_texcoord, 0);
		
		vec4 gi_delta     = abs(gi_tap - o.gi);
		vec4 gbuf_delta   = abs(gbuf_tap - o.gbuf);

		float ddepth    = gbuf_delta.w;
		float dnormal   = max(max(gbuf_delta.x, gbuf_delta.y), gbuf_delta.z);
		float dvalue    = dot(gi_delta, gi_delta);

		float w = exp(-ddepth * 750)
				* exp(-dnormal * 50 * saturate(ddepth * 10000))
				* exp(-dvalue * 10);

		gi += gi_tap * w;
		weightsum += w;
	}

	gi /= weightsum;
	o.gi = gi;o.gi    = tex2D(u_SteepMap,      v.texcoord.xy);
#endif

	gl_FragColor = o.gi;
	out_Glow = o.gbuf;
}
