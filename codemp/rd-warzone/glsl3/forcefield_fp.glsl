#define SCREEN_MAPS_ALPHA_THRESHOLD 0.666
#define SCREEN_MAPS_LEAFS_THRESHOLD 0.001

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
uniform sampler3D					u_VolumeMap;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D					u_DiffuseMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4						u_Settings0; // useTC, useDeform, useRGBA, isTextureClamped
uniform vec4						u_Settings1; // useVertexAnim, useSkeletalAnim, blendMethod, is2D
uniform vec4						u_Settings2; // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
uniform vec4						u_Settings3; // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL, 0=DetailMapNormal 1=detailMapFromTC 2=detailMapFromWorld, USE_GLOW_BLEND_MODE
uniform vec4						u_Settings4; // MAP_LIGHTMAP_MULTIPLIER, MAP_LIGHTMAP_ENHANCEMENT, 0.0, 0.0
uniform vec4						u_Settings5; // MAP_COLOR_SWITCH_RG, MAP_COLOR_SWITCH_RB, MAP_COLOR_SWITCH_GB, ENABLE_CHRISTMAS_EFFECT

#define USE_TC						u_Settings0.r
#define USE_DEFORM					u_Settings0.g
#define USE_RGBA					u_Settings0.b
#define USE_TEXTURECLAMP			u_Settings0.a

#define USE_VERTEX_ANIM				u_Settings1.r
#define USE_SKELETAL_ANIM			u_Settings1.g
#define USE_BLEND					u_Settings1.b
#define USE_IS2D					u_Settings1.a

#define USE_LIGHTMAP				u_Settings2.r
#define USE_GLOW_BUFFER				u_Settings2.g
#define USE_CUBEMAP					u_Settings2.b
#define USE_TRIPLANAR				u_Settings2.a

#define USE_REGIONS					u_Settings3.r
#define USE_ISDETAIL				u_Settings3.g
#define USE_EMISSIVE_BLACK			u_Settings3.b
#define USE_GLOW_BLEND_MODE			u_Settings3.a

#define MAP_LIGHTMAP_MULTIPLIER		u_Settings4.r
#define MAP_LIGHTMAP_ENHANCEMENT	u_Settings4.g

#define MAP_COLOR_SWITCH_RG			u_Settings5.r
#define MAP_COLOR_SWITCH_RB			u_Settings5.g
#define MAP_COLOR_SWITCH_GB			u_Settings5.b
#define ENABLE_CHRISTMAS_EFFECT		u_Settings5.a


uniform vec4						u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4						u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4						u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4						u_Local4; // stageNum, glowStrength, r_showsplat, glowVibrancy
uniform vec4						u_Local5; // SHADER_HAS_OVERLAY, SHADER_ENVMAP_STRENGTH, 0.0, LEAF_ALPHA_MULTIPLIER
uniform vec4						u_Local6; // TOWN_FORCEFIELD_ALPHAMULT, 0.0, 0.0, 0.0
uniform vec4						u_Local9; // testvalue0, 1, 2, 3

uniform vec2						u_Dimensions;
uniform vec3						u_ViewOrigin;
uniform float						u_Time;

#define SHADER_MATERIAL_TYPE		u_Local1.a

#define TOWN_FORCEFIELD_ALPHAMULT	u_Local6.r

varying vec2						var_TexCoords;
varying vec3						var_Normal;
varying vec3						var_vertPos;
varying vec3						var_ViewDir;



#define m_TexCoords					var_TexCoords
#define m_vertPos					var_vertPos

vec3 m_Normal						= normalize(gl_FrontFacing ? -var_Normal : var_Normal);
vec3 m_ViewDir						= normalize(var_ViewDir);




out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4 out_NormalDetail;
#endif //USE_REAL_NORMALMAPS



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


float _IntersectPower = 0.18;
float _RimStrength = 1.69;
float _DistortStrength = 0.80;
float _DistortTimeFactor = 1.195;//0.195;


void main()
{
	float rim = 1.0 - length(dot(m_Normal, m_ViewDir)) * _RimStrength;
	float glow = max(_IntersectPower, rim);//rim;//max(intersect, rim);

	vec3 offset = m_ViewDir.xyz * 0.125;
	vec2 off;
	off.x = noise(m_Normal * 32.0 - u_Time * _DistortTimeFactor);
	off.y = noise(m_Normal * 32.0 - u_Time * _DistortTimeFactor);
	offset.xy *= off;
	vec4 color = texture(u_DiffuseMap, offset.xy);

	gl_FragColor = color * 0.5 + glow * 0.25;
	gl_FragColor.rgb *= vec3(0.75, 0.75, 1.75);
	gl_FragColor.a = clamp(glow * TOWN_FORCEFIELD_ALPHAMULT * 0.3 + 0.25, 0.0, 1.0);
	
	//
	// Hit FX...
	//
	
#if 0
	vec3 hit3 = vec3(-1.0, -3.0, -0.01);
	vec3 hit2 = vec3(-0.6972498297691345, 1.0605579614639282, -0.27839863300323486);
	vec3 hit = vec3(0.0,0.0,2.0);

	if( dot( normalize(hit), m_Normal ) > 0.99 ) gl_FragColor = vec4(1.0,0.0,0.0,1.0);
	if( dot( normalize(hit2), m_Normal ) > 0.995 ) gl_FragColor = vec4(1.0,0.0,0.0,1.0);

	//float hAngle = dot( normalize(hit3), m_Normal );
	//float angleStart = fract(u_Time * 0.001 * 0.4) * 2.0 - 1.0; //Map 0 to 1 to -1 to 1
	//float thickness = 0.03;
	//if( hAngle <= angleStart && hAngle >= angleStart-thickness ) gl_FragColor += vec4(1.0,0.0,0.0,1.0);
#endif

	out_Glow = vec4(0.0);
	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
	out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
}
