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
uniform sampler2D					u_DiffuseMap;	// Land grass atlas
uniform sampler2D					u_WaterEdgeMap; // Sea grass atlas
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec3						u_ViewOrigin;
uniform vec2						u_Dimensions;

uniform vec4						u_Settings1; // IS_DEPTH_PASS, 0.0, 0.0, LEAF_ALPHA_MULTIPLIER
uniform vec4						u_Settings5; // MAP_COLOR_SWITCH_RG, MAP_COLOR_SWITCH_RB, MAP_COLOR_SWITCH_GB, 0.0

#define IS_DEPTH_PASS				u_Settings1.r
#define LEAF_ALPHA_MULTIPLIER		u_Settings1.a

#define MAP_COLOR_SWITCH_RG			u_Settings5.r
#define MAP_COLOR_SWITCH_RB			u_Settings5.g
#define MAP_COLOR_SWITCH_GB			u_Settings5.b

uniform vec4								u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4								u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4								u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4								u_Local8; // GRASS_SURFACE_MINIMUM_SIZE, GRASS_DISTANCE_FROM_ROADS, GRASS_HEIGHT, GRASS_SURFACE_SIZE_DIVIDER
uniform vec4								u_Local9; // testvalue0, 1, 2, 3
uniform vec4								u_Local10; // foliageLODdistance, TERRAIN_TESS_OFFSET, GRASS_DENSITY, GRASS_TYPE_UNIFORMALITY
uniform vec4								u_Local11; // GRASS_WIDTH_REPEATS, GRASS_MAX_SLOPE, GRASS_TYPE_UNIFORMALITY_SCALER, 0.0
uniform vec4								u_Local12; // GRASS_SIZE_MULTIPLIER_COMMON, GRASS_SIZE_MULTIPLIER_RARE, GRASS_SIZE_MULTIPLIER_UNDERWATER, 0.0

#define SHADER_MAP_SIZE						u_Local1.r
#define SHADER_SWAY							u_Local1.g
#define SHADER_OVERLAY_SWAY					u_Local1.b
#define SHADER_MATERIAL_TYPE				u_Local1.a

#define SHADER_HAS_STEEPMAP					u_Local2.r
#define SHADER_HAS_WATEREDGEMAP				u_Local2.g
#define SHADER_HAS_NORMALMAP				u_Local2.b
#define SHADER_WATER_LEVEL					u_Local2.a

#define SHADER_HAS_SPLATMAP1				u_Local3.r
#define SHADER_HAS_SPLATMAP2				u_Local3.g
#define SHADER_HAS_SPLATMAP3				u_Local3.b
#define SHADER_HAS_SPLATMAP4				u_Local3.a

#define GRASS_SURFACE_MINIMUM_SIZE			u_Local8.r
#define GRASS_DISTANCE_FROM_ROADS			u_Local8.g
#define GRASS_HEIGHT						u_Local8.b
#define GRASS_SURFACE_SIZE_DIVIDER			u_Local8.a

#define MAX_RANGE							u_Local10.r
#define GRASS_DENSITY						u_Local10.b
#define GRASS_TYPE_UNIFORMALITY				u_Local10.a

#define GRASS_WIDTH_REPEATS					u_Local11.r
#define GRASS_MAX_SLOPE						u_Local11.g
#define GRASS_TYPE_UNIFORMALITY_SCALER		u_Local11.b

#define GRASS_SIZE_MULTIPLIER_COMMON		u_Local12.r
#define GRASS_SIZE_MULTIPLIER_RARE			u_Local12.g
#define GRASS_SIZE_MULTIPLIER_UNDERWATER	u_Local12.b

#define MAP_WATER_LEVEL						SHADER_WATER_LEVEL // TODO: Use water map
#define GRASS_TYPE_UNIFORM_WATER			0.66

smooth in vec2		vTexCoord;
smooth in vec3		vVertPosition;
//flat in float		vVertNormal;
smooth in vec2		vVertNormal;
flat in int			iGrassType;

out vec4			out_Glow;
out vec4			out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4			out_NormalDetail;
#endif //USE_REAL_NORMALMAPS
out vec4			out_Position;

const float xdec = 1.0/255.0;
const float ydec = 1.0/65025.0;
const float zdec = 1.0/16581375.0;

vec2 EncodeNormal(vec3 n)
{
	float scale = 1.7777;
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;
	return enc;
}
vec3 DecodeNormal(vec2 enc)
{
	vec3 enc2 = vec3(enc.xy, 0.0);
	float scale = 1.7777;
	vec3 nn =
		enc2.xyz*vec3(2.0 * scale, 2.0 * scale, 0.0) +
		vec3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	return vec3(g * nn.xy, g - 1.0);
}

vec4 DecodeFloatRGBA( float v ) {
  vec4 enc = vec4(1.0, 255.0, 65025.0, 16581375.0) * v;
  enc = fract(enc);
  enc -= enc.yzww * vec4(xdec,xdec,xdec,0.0);
  return enc;
}

bool isDithered(vec2 pos, float alpha)
{
	pos *= u_Dimensions.xy * 12.0;

	// Define a dither threshold matrix which can
	// be used to define how a 4x4 set of pixels
	// will be dithered
	float DITHER_THRESHOLDS[16] = float[]
	(
		1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
		13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
		4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
		16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
	);

	int index = (int(pos.x) % 4) * 4 + int(pos.y) % 4;
	return (alpha - DITHER_THRESHOLDS[index] < 0) ? true : false;
}

void main() 
{
	vec4 diffuse;

	vec2 tc = vTexCoord;

	if (IS_DEPTH_PASS > 0.0)
	{
		diffuse = vec4(1.0, 1.0, 1.0, texture(u_DiffuseMap, tc).a);
	}
	else 
	{
		diffuse = texture(u_DiffuseMap, tc);
	}

	if (MAP_COLOR_SWITCH_RG > 0.0)
	{
		diffuse.rg = diffuse.gr;
	}

	if (MAP_COLOR_SWITCH_RB > 0.0)
	{
		diffuse.rb = diffuse.br;
	}

	if (MAP_COLOR_SWITCH_GB > 0.0)
	{
		diffuse.gb = diffuse.bg;
	}

	// Amp up alphas on tree leafs, etc, so they draw at range instead of being blurred out...
	diffuse.a = clamp(diffuse.a * 1.5/*LEAF_ALPHA_MULTIPLIER*/, 0.0, 1.0);

	//diffuse.rgba = vec4(1.0);

	if (diffuse.a > 0.5)
	{
#if 1
		// Screen-door transparancy on distant and extremely close foliages...
		float dist = distance(vVertPosition, u_ViewOrigin);
		float alpha = 1.0 - (dist / MAX_RANGE);
		float afar = clamp(alpha * 2.0, 0.0, 1.0);
		float anear = clamp(dist / 48.0, 0.0, 1.0);

		if (isDithered(tc, afar))
		{
			diffuse.a = 0.0;
		}
		else if (isDithered(tc, anear))
		{
			diffuse.a = 0.0;
		}
		else
		{
			diffuse.a = 1.0;
		}
#else
		diffuse.a = 1.0;
#endif
	}
	else
	{
		diffuse.a = 0.0;
	}

	if (diffuse.a > 0.05)
	{
		gl_FragColor = vec4(diffuse.rgb, diffuse.a);
		out_Glow = vec4(0.0);
		out_Normal = vec4(vVertNormal.xy/*EncodeNormal(DecodeNormal(vVertNormal.xy))*/, 0.0, 1.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
		out_Position = vec4(vVertPosition, MATERIAL_PROCEDURALFOLIAGE+1.0);
	}
	else
	{
		gl_FragColor = vec4(0.0);
		out_Glow = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
		out_Position = vec4(0.0);
	}
}
