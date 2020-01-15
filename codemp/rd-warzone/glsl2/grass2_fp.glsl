//#define FAKE_LOD

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

uniform vec4						u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4						u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4						u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4						u_Local8; // passnum, GRASS_DISTANCE_FROM_ROADS, GRASS_HEIGHT, 0.0
uniform vec4						u_Local9; // testvalue0, 1, 2, 3
uniform vec4						u_Local10; // foliageLODdistance, TERRAIN_TESS_OFFSET, 0.0, GRASS_TYPE_UNIFORMALITY
uniform vec4						u_Local11; // GRASS_WIDTH_REPEATS, 0.0, 0.0, 0.0

#define SHADER_MAP_SIZE				u_Local1.r
#define SHADER_SWAY					u_Local1.g
#define SHADER_OVERLAY_SWAY			u_Local1.b
#define SHADER_MATERIAL_TYPE		u_Local1.a

#define SHADER_HAS_STEEPMAP			u_Local2.r
#define SHADER_HAS_WATEREDGEMAP		u_Local2.g
#define SHADER_HAS_NORMALMAP		u_Local2.b
#define SHADER_WATER_LEVEL			u_Local2.a

#define SHADER_HAS_SPLATMAP1		u_Local3.r
#define SHADER_HAS_SPLATMAP2		u_Local3.g
#define SHADER_HAS_SPLATMAP3		u_Local3.b
#define SHADER_HAS_SPLATMAP4		u_Local3.a

#define PASS_NUMBER					u_Local8.r
#define GRASS_DISTANCE_FROM_ROADS	u_Local8.g
#define GRASS_HEIGHT				u_Local8.b

#define MAX_RANGE					u_Local10.r
#define TERRAIN_TESS_OFFSET			u_Local10.g
#define GRASS_TYPE_UNIFORMALITY		u_Local10.a

#define GRASS_WIDTH_REPEATS			u_Local11.r

smooth in vec2		vTexCoord;
smooth in vec3		vVertPosition;
//flat in float		vVertNormal;
smooth in vec2		vVertNormal;
flat in int			iGrassType;

out vec4			out_Glow;
out vec4			out_Normal;
#ifdef __USE_REAL_NORMALMAPS__
out vec4			out_NormalDetail;
#endif //__USE_REAL_NORMALMAPS__
out vec4			out_Position;

const float xdec = 1.0/255.0;
const float ydec = 1.0/65025.0;
const float zdec = 1.0/16581375.0;

//#define __ENCODE_NORMALS_RECONSTRUCT_Z__
#define __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
//#define __ENCODE_NORMALS_CRY_ENGINE__
//#define __ENCODE_NORMALS_EQUAL_AREA_PROJECTION__

#ifdef __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
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
#elif defined(__ENCODE_NORMALS_CRY_ENGINE__)
vec3 DecodeNormal(in vec2 N)
{
	vec2 encoded = N * 4.0 - 2.0;
	float f = dot(encoded, encoded);
	float g = sqrt(1.0 - f * 0.25);
	return vec3(encoded * g, 1.0 - f * 0.5);
}
vec2 EncodeNormal(in vec3 N)
{
	float f = sqrt(8.0 * N.z + 8.0);
	return N.xy / f + 0.5;
}
#elif defined(__ENCODE_NORMALS_EQUAL_AREA_PROJECTION__)
vec2 EncodeNormal(vec3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);
	return n.xy / f + 0.5;
}
vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 n;
	n.xy = fenc*g;
	n.z = 1.0 - f / 2.0;
	return n;
}
#else //__ENCODE_NORMALS_RECONSTRUCT_Z__
vec3 DecodeNormal(in vec2 N)
{
	vec3 norm;
	norm.xy = N * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy));
	return norm;
}
vec2 EncodeNormal(vec3 n)
{
	return vec2(n.xy * 0.5 + 0.5);
}
#endif //__ENCODE_NORMALS_RECONSTRUCT_Z__

vec4 DecodeFloatRGBA( float v ) {
  vec4 enc = vec4(1.0, 255.0, 65025.0, 16581375.0) * v;
  enc = fract(enc);
  enc -= enc.yzww * vec4(xdec,xdec,xdec,0.0);
  return enc;
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

	if (GRASS_WIDTH_REPEATS > 0.0) tc.x *= (GRASS_WIDTH_REPEATS * 2.0);

#if defined(__USE_UNDERWATER__)
	if (IS_DEPTH_PASS > 0.0)
	{
		diffuse = vec4(1.0, 1.0, 1.0, texture(u_WaterEdgeMap, tc).a);
	}
	else
	{
		diffuse = texture(u_WaterEdgeMap, tc);
	}
#else //!defined(__USE_UNDERWATER__)
	if (IS_DEPTH_PASS > 0.0)
	{
#ifdef FAKE_LOD
		if (iGrassType == 3)
		{// Below water grass lod...
			diffuse = vec4(0.0);
		}
		else if (iGrassType == 2)
		{// Above water grass lod...
			diffuse = vec4(0.0);
		}
		else 
#endif //FAKE_LOD
		if (iGrassType == 1)
		{
			diffuse = vec4(1.0, 1.0, 1.0, texture(u_WaterEdgeMap, tc).a);
		}
		else
		{
			diffuse = vec4(1.0, 1.0, 1.0, texture(u_DiffuseMap, tc).a);
		}
	}
	else
	{
#ifdef FAKE_LOD
	#define LOD_COORD_MULT_X u_Local9.r
	#define LOD_COORD_MULT_Y u_Local9.g

		if (iGrassType == 3)
		{// Below water grass lod...
			//tc.x = noise(vVertPosition.xyz*LOD_COORD_MULT) * 0.03 + 0.10;
			//tc.y = noise(vVertPosition.yzx*LOD_COORD_MULT) * 0.03 + 0.10;
			//vec2 p = vec2(vVertPosition.xy)*LOD_COORD_MULT;
			vec3 dir = normalize(u_ViewOrigin - vVertPosition);
			vec2 p = vec2(vVertPosition.x+vVertPosition.y*LOD_COORD_MULT_X, dir.z*LOD_COORD_MULT_Y);
			vec2 paramU = vec2(0.05, 0.125);
			vec2 paramV = vec2(0.05, 0.125);
			tc.x = fract(p.x) * paramU.x + paramU.y;
			tc.y = fract(p.y) * paramV.x + paramV.y;
			diffuse = texture(u_WaterEdgeMap, tc);
		}
		else if (iGrassType == 2)
		{// Above water grass lod...
			//tc.x = noise(vVertPosition.xyz*LOD_COORD_MULT) * 0.03 + 0.10;
			//tc.y = noise(vVertPosition.yzx*LOD_COORD_MULT) * 0.03 + 0.10;
			//vec2 p = vec2(vVertPosition.xy)*LOD_COORD_MULT;
			vec3 dir = normalize(u_ViewOrigin - vVertPosition);
			vec2 p = vec2(vVertPosition.x+vVertPosition.y*LOD_COORD_MULT_X, dir.z*LOD_COORD_MULT_Y);
			vec2 paramU = vec2(0.05, 0.125);
			vec2 paramV = vec2(0.05, 0.125);
			tc.x = fract(p.x) * paramU.x + paramU.y;
			tc.y = fract(p.y) * paramV.x + paramV.y;
			diffuse = texture(u_DiffuseMap, tc);
		}
		else 
#endif //FAKE_LOD
		if (iGrassType == 1)
		{
			diffuse = texture(u_WaterEdgeMap, tc);
		}
		else
		{
			diffuse = texture(u_DiffuseMap, tc);
		}
	}
#endif //defined(__USE_UNDERWATER__)

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
	diffuse.a = clamp(diffuse.a * 1.15, 0.0, 1.0);

#ifdef FAKE_LOD
	if (iGrassType >= 2 && diffuse.a > 0.5)
	{// Lods...
		diffuse.a = 1.0;
	}
	else 
#endif //FAKE_LOD
	if (diffuse.a > 0.5)
	{
		float CULL_RANGE = MAX_RANGE;

		if (vVertPosition.z < SHADER_WATER_LEVEL)
		{
			CULL_RANGE = MAX_RANGE * 1.0/*2.0*/;
		}

		// Screen-door transparancy on distant and extremely close grasses...
		float dist = distance(vVertPosition, u_ViewOrigin);
		float alpha = 1.0 - (dist / CULL_RANGE);
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
	}
	else
	{
		diffuse.a = 0.0;
	}

#ifdef FAKE_LOD
	if (iGrassType >= 2 && diffuse.a > 0.05)
	{// Lods...
		gl_FragColor = diffuse;
		out_Glow = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
		out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
		out_Position = vec4(vVertPosition, MATERIAL_PROCEDURALFOLIAGE+1.0);
	}
	else 
#endif //FAKE_LOD
	if (diffuse.a > 0.05)
	{
		gl_FragColor = diffuse;
		out_Glow = vec4(0.0);
		out_Normal = vec4(vVertNormal.xy, 0.0, 1.0);
#ifdef __USE_REAL_NORMALMAPS__
		out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
		out_Position = vec4(vVertPosition, MATERIAL_PROCEDURALFOLIAGE+1.0);
	}
	else
	{
		gl_FragColor = vec4(0.0);
		out_Glow = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
		out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
		out_Position = vec4(0.0);
	}
}
