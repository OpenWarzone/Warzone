#define SCREEN_MAPS_ALPHA_THRESHOLD 0.666
#define SCREEN_MAPS_LEAFS_THRESHOLD 0.001
//#define SCREEN_MAPS_LEAFS_THRESHOLD 0.9

uniform sampler2D					u_DiffuseMap;

uniform vec4						u_Settings0; // useTC, useDeform, useRGBA, isTextureClamped
uniform vec4						u_Settings1; // useVertexAnim, useSkeletalAnim, blendMode, is2D
uniform vec4						u_Settings2; // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
uniform vec4						u_Settings3; // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL, 0=DetailMapNormal 1=detailMapFromTC 2=detailMapFromWorld, 0.0
uniform vec4						u_Settings4; // MAP_LIGHTMAP_MULTIPLIER, MAP_LIGHTMAP_ENHANCEMENT, hasAlphaTestBits, 0.0

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

#define HAS_ALPHA_BITS				u_Settings4.b


uniform vec4						u_Local1; // TERRAIN_TESSELLATION_OFFSET, sway, overlaySway, materialType
uniform vec4						u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, WATER_LEVEL
uniform vec4						u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4						u_Local4; // stageNum, glowStrength, r_showsplat, 0.0
uniform vec4						u_Local5; // dayNightEnabled, nightScale, skyDirection, auroraEnabled -- Sky draws only!
uniform vec4						u_Local9; // testvalue0, 1, 2, 3

#define TERRAIN_TESSELLATION_OFFSET	u_Local1.r
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

#define SHADER_STAGE_NUM			u_Local4.r
#define SHADER_GLOW_STRENGTH		u_Local4.g
#define SHADER_SHOW_SPLAT			u_Local4.b

#define SHADER_DAY_NIGHT_ENABLED	u_Local5.r
#define SHADER_NIGHT_SCALE			u_Local5.g
#define SHADER_SKY_DIRECTION		u_Local5.b
#define SHADER_AURORA_ENABLED		u_Local5.a


uniform vec2						u_Dimensions;
uniform vec3						u_ViewOrigin;
uniform float						u_Time;
uniform float						u_zFar;


varying vec3						var_VertPos;
varying vec2						var_TexCoords;
varying vec4						var_Color;

const float							fBranchHardiness = 0.001;
const float							fBranchSize = 128.0;
const float							fWindStrength = 12.0;
const vec3							vWindDirection = normalize(vec3(1.0, 1.0, 0.0));

vec2 GetSway ()
{
	// Wind calculation stuff...
	float fWindPower = 0.5f + sin(var_VertPos.x / fBranchSize + var_VertPos.z / fBranchSize + u_Time*(1.2f + fWindStrength / fBranchSize/*20.0f*/));

	if (fWindPower < 0.0f)
		fWindPower = fWindPower*0.2f;
	else
		fWindPower = fWindPower*0.3f;

	fWindPower *= fWindStrength;

	return vWindDirection.xy*fWindPower*fBranchHardiness;
}

void main()
{
	if (USE_TRIPLANAR > 0.0 || USE_REGIONS > 0.0 || HAS_ALPHA_BITS <= 0.0)
	{// Can skip nearly everything... These are always going to be solid color...
		gl_FragColor = vec4(1.0);
	}
	else
	{
		vec2 texCoords = var_TexCoords;

		if (SHADER_SWAY > 0.0)
		{// Sway...
			texCoords += vec2(GetSway());
		}

		// We don't even need the colors, just the alphas...
		gl_FragColor = vec4(1.0, 1.0, 1.0, texture(u_DiffuseMap, texCoords).a * var_Color.a);


		float alphaThreshold = (SHADER_MATERIAL_TYPE == MATERIAL_GREENLEAVES) ? SCREEN_MAPS_LEAFS_THRESHOLD : SCREEN_MAPS_ALPHA_THRESHOLD;

		bool isDetail = false;

		if (USE_ISDETAIL >= 1.0)
		{
			isDetail = true;
		}
		else if (gl_FragColor.a >= alphaThreshold || SHADER_MATERIAL_TYPE == 1024.0 || SHADER_MATERIAL_TYPE == 1025.0 || USE_IS2D > 0.0)
		{

		}
		else
		{
			isDetail = true;
		}

		if (isDetail)
		{
			gl_FragColor.a = 0.0;
		}
		else if (USE_BLEND > 0.0)
		{// Emulate RGB blending... Fuck I hate this crap...
			if (USE_BLEND == 2.0)
			{
				gl_FragColor.a = 0.0;
			}
			else if (USE_BLEND == 4.0)
			{
				gl_FragColor.a = 0.0;
			}
			else
			{
				gl_FragColor.a = 1.0;
			}
		}
	}

	if (USE_EMISSIVE_BLACK > 0.0)
	{
		gl_FragColor.rgb = vec3(0.0);
	}

	if (gl_FragColor.a >= 0.99)
	{// Allow for rounding errors... Don't let them stop pixel culling...
		gl_FragColor.a = 1.0;
	}
}
