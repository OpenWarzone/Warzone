/*
===========================================================================
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_glsl.c
#include "tr_local.h"

#include "tr_glsl.h"

extern const char *fallbackShader_bokeh_vp;
extern const char *fallbackShader_bokeh_fp;
extern const char *fallbackShader_calclevels4x_vp;
extern const char *fallbackShader_calclevels4x_fp;
extern const char *fallbackShader_depthblur_vp;
extern const char *fallbackShader_depthblur_fp;
extern const char *fallbackShader_down4x_vp;
extern const char *fallbackShader_down4x_fp;
extern const char *fallbackShader_fogpass_vp;
extern const char *fallbackShader_fogpass_fp;
extern const char *fallbackShader_generic_vp;
extern const char *fallbackShader_generic_fp;
extern const char *fallbackShader_lightall_vp;
extern const char *fallbackShader_lightall_gs;
extern const char *fallbackShader_lightall_fp;
extern const char *fallbackShader_lightallSplat_vp;
extern const char *fallbackShader_lightallSplat_gs;
extern const char *fallbackShader_lightallSplat_fp;
extern const char *fallbackShader_sun_vp;
extern const char *fallbackShader_sun_fp;
extern const char *fallbackShader_planet_vp;
extern const char *fallbackShader_planet_fp;
extern const char *fallbackShader_fire_vp;
extern const char *fallbackShader_fire_fp;
extern const char *fallbackShader_menuBackground_vp;
extern const char *fallbackShader_menuBackground_fp;
extern const char *fallbackShader_smoke_vp;
extern const char *fallbackShader_smoke_fp;
extern const char *fallbackShader_magicParticles_vp;
extern const char *fallbackShader_magicParticles_fp;
extern const char *fallbackShader_magicParticlesTree_vp;
extern const char *fallbackShader_magicParticlesTree_fp;
extern const char *fallbackShader_magicParticlesFireFly_vp;
extern const char *fallbackShader_magicParticlesFireFly_fp;
extern const char *fallbackShader_portal_vp;
extern const char *fallbackShader_portal_fp;
extern const char *fallbackShader_depthPass_vp;
extern const char *fallbackShader_depthPass_fp;
extern const char *fallbackShader_sky_vp;
extern const char *fallbackShader_sky_fp;
extern const char *fallbackShader_pshadow_vp;
extern const char *fallbackShader_pshadow_fp;
extern const char *fallbackShader_shadowfill_vp;
extern const char *fallbackShader_shadowfill_fp;
extern const char *fallbackShader_shadowmask_vp;
extern const char *fallbackShader_shadowmask_fp;
extern const char *fallbackShader_ssao_vp;
extern const char *fallbackShader_ssao_fp;
extern const char *fallbackShader_texturecolor_vp;
extern const char *fallbackShader_texturecolor_fp;
extern const char *fallbackShader_tonemap_vp;
extern const char *fallbackShader_tonemap_fp;
extern const char *fallbackShader_gaussian_blur_vp;
extern const char *fallbackShader_gaussian_blur_fp;
extern const char *fallbackShader_dglow_downsample_vp;
extern const char *fallbackShader_dglow_downsample_fp;
extern const char *fallbackShader_dglow_upsample_vp;
extern const char *fallbackShader_dglow_upsample_fp;

// UQ1: Added...
extern const char *fallbackShader_linearizeDepth_vp;
extern const char *fallbackShader_linearizeDepth_fp;
extern const char *fallbackShader_weather_vp;
extern const char *fallbackShader_weather_fp;
extern const char *fallbackShader_surfaceSprite_vp;
extern const char *fallbackShader_surfaceSprite_fp;
//extern const char *fallbackShader_sss_vp;
//extern const char *fallbackShader_sss_fp;
//extern const char *fallbackShader_sssBlur_vp;
//extern const char *fallbackShader_sssBlur_fp;
//extern const char *fallbackShader_ssdo_vp;
//extern const char *fallbackShader_ssdo_fp;
//extern const char *fallbackShader_ssdoBlur_vp;
//extern const char *fallbackShader_ssdoBlur_fp;
extern const char *fallbackShader_instance_vp;
extern const char *fallbackShader_instance_fp;
extern const char *fallbackShader_instanceVao_vp;
extern const char *fallbackShader_instanceVao_fp;
extern const char *fallbackShader_occlusion_vp;
extern const char *fallbackShader_occlusion_fp;
extern const char *fallbackShader_depthAdjust_vp;
extern const char *fallbackShader_depthAdjust_fp;
extern const char *fallbackShader_generateNormalMap_vp;
extern const char *fallbackShader_generateNormalMap_fp;
extern const char *fallbackShader_magicdetail_vp;
extern const char *fallbackShader_magicdetail_fp;
extern const char *fallbackShader_volumelight_vp;
extern const char *fallbackShader_volumelight_fp;
extern const char *fallbackShader_volumelightInverted_vp;
extern const char *fallbackShader_volumelightInverted_fp;
extern const char *fallbackShader_volumelightCombine_vp;
extern const char *fallbackShader_volumelightCombine_fp;
extern const char *fallbackShader_anaglyph_vp;
extern const char *fallbackShader_anaglyph_fp;
extern const char *fallbackShader_skyDome_fp;
extern const char *fallbackShader_skyDome_vp;
extern const char *fallbackShader_waterPost_fp;
extern const char *fallbackShader_waterPost_vp;
extern const char *fallbackShader_waterPost2_fp;
extern const char *fallbackShader_waterPost2_vp;
extern const char *fallbackShader_waterReflection_fp;
extern const char *fallbackShader_waterReflection_vp;
extern const char *fallbackShader_waterPostForward_fp;
extern const char *fallbackShader_waterPostForward_vp;
extern const char *fallbackShader_waterForward_fp;
extern const char *fallbackShader_waterForward_vp;
extern const char *fallbackShader_waterForward_gs;
extern const char *fallbackShader_waterForwardFast_fp;
extern const char *fallbackShader_waterForwardFast_vp;
extern const char *fallbackShader_transparancyPost_fp;
extern const char *fallbackShader_transparancyPost_vp;
extern const char *fallbackShader_clouds_fp;
extern const char *fallbackShader_clouds_vp;
extern const char *fallbackShader_foliage_fp;
extern const char *fallbackShader_foliage_vp;
extern const char *fallbackShader_foliage_cs;
extern const char *fallbackShader_foliage_es;
extern const char *fallbackShader_foliage_gs;
extern const char *fallbackShader_grassPatches_fp;
extern const char *fallbackShader_grassPatches_vp;
extern const char *fallbackShader_grassPatches_cs;
extern const char *fallbackShader_grassPatches_es;
extern const char *fallbackShader_grassPatches_gs;
extern const char *fallbackShader_grass2_fp;
extern const char *fallbackShader_grass2_vp;
extern const char *fallbackShader_grass2_cs;
extern const char *fallbackShader_grass2_es;
extern const char *fallbackShader_grass2_gs;
extern const char *fallbackShader_vines_fp;
extern const char *fallbackShader_vines_vp;
extern const char *fallbackShader_vines_cs;
extern const char *fallbackShader_vines_es;
extern const char *fallbackShader_vines_gs;
extern const char *fallbackShader_mist_fp;
extern const char *fallbackShader_mist_vp;
extern const char *fallbackShader_mist_cs;
extern const char *fallbackShader_mist_es;
extern const char *fallbackShader_mist_gs;
extern const char *fallbackShader_hbao_vp;
extern const char *fallbackShader_hbao_fp;
extern const char *fallbackShader_hbaoCombine_vp;
extern const char *fallbackShader_hbaoCombine_fp;
extern const char *fallbackShader_esharpening_vp;
extern const char *fallbackShader_esharpening_fp;
extern const char *fallbackShader_esharpening2_vp;
extern const char *fallbackShader_esharpening2_fp;
extern const char *fallbackShader_depthOfField_vp;
extern const char *fallbackShader_depthOfField_fp;
extern const char *fallbackShader_depthOfField2_vp;
extern const char *fallbackShader_depthOfField2_fp;
extern const char *fallbackShader_bloom_blur_vp;
extern const char *fallbackShader_bloom_blur_fp;
extern const char *fallbackShader_bloom_combine_vp;
extern const char *fallbackShader_bloom_combine_fp;
extern const char *fallbackShader_anamorphic_blur_vp;
extern const char *fallbackShader_anamorphic_blur_fp;
extern const char *fallbackShader_anamorphic_combine_vp;
extern const char *fallbackShader_anamorphic_combine_fp;
extern const char *fallbackShader_darkexpand_vp;
extern const char *fallbackShader_darkexpand_fp;
extern const char *fallbackShader_lensflare_vp;
extern const char *fallbackShader_lensflare_fp;
extern const char *fallbackShader_multipost_vp;
extern const char *fallbackShader_multipost_fp;
extern const char *fallbackShader_underwater_vp;
extern const char *fallbackShader_underwater_fp;
extern const char *fallbackShader_fxaa_vp;
extern const char *fallbackShader_fxaa_fp;
extern const char *fallbackShader_txaa_vp;
extern const char *fallbackShader_txaa_fp;
extern const char *fallbackShader_fastBlur_vp;
extern const char *fallbackShader_fastBlur_fp;
//extern const char *fallbackShader_shadowBlur_vp;
//extern const char *fallbackShader_shadowBlur_fp;
extern const char *fallbackShader_distanceBlur_vp;
extern const char *fallbackShader_distanceBlur_fp;
extern const char *fallbackShader_dofFocusDepth_vp;
extern const char *fallbackShader_dofFocusDepth_fp;
extern const char *fallbackShader_bloomRays_vp;
extern const char *fallbackShader_bloomRays_fp;
extern const char *fallbackShader_fogPost_vp;
extern const char *fallbackShader_fogPost_fp;
extern const char *fallbackShader_showNormals_vp;
extern const char *fallbackShader_showNormals_fp;
extern const char *fallbackShader_showDepth_vp;
extern const char *fallbackShader_showDepth_fp;
extern const char *fallbackShader_fastLighting_vp;
extern const char *fallbackShader_fastLighting_fp;
extern const char *fallbackShader_deferredLighting_vp;
extern const char *fallbackShader_deferredLighting_fp;
extern const char *fallbackShader_procedural_vp;
extern const char *fallbackShader_procedural_fp;
extern const char *fallbackShader_ssdm_vp;
extern const char *fallbackShader_ssdm_fp;
extern const char *fallbackShader_ssdmGenerate_vp;
extern const char *fallbackShader_ssdmGenerate_fp;
extern const char *fallbackShader_ssr_vp;
extern const char *fallbackShader_ssr_fp;
extern const char *fallbackShader_ssrCombine_vp;
extern const char *fallbackShader_ssrCombine_fp;
extern const char *fallbackShader_colorCorrection_vp;
extern const char *fallbackShader_colorCorrection_fp;
extern const char *fallbackShader_cellShade_vp;
extern const char *fallbackShader_cellShade_fp;
extern const char *fallbackShader_paint_vp;
extern const char *fallbackShader_paint_fp;

extern const char *fallbackShader_testshader_vp;
extern const char *fallbackShader_testshader_fp;

extern const char *fallbackShader_tessellation_cs;
extern const char *fallbackShader_tessellation_es;
extern const char *fallbackShader_tessellation_gs;

extern const char *fallbackShader_tessellationTerrain_cs;
extern const char *fallbackShader_tessellationTerrain_es;
extern const char *fallbackShader_tessellationTerrain_gs;

extern const char *fallbackShader_tessellation3D_cs;
extern const char *fallbackShader_tessellation3D_es;
extern const char *fallbackShader_tessellation3D_gs;

// These must be in the same order as in uniform_t in tr_local.h.
static uniformInfo_t uniformsInfo[] =
{
	{ "u_DiffuseMap", GLSL_INT, 1 },
	{ "u_LightMap", GLSL_INT, 1 },
	{ "u_NormalMap", GLSL_INT, 1 },
	{ "u_DeluxeMap", GLSL_INT, 1 },
	{ "u_SpecularMap", GLSL_INT, 1 },
	{ "u_PositionMap", GLSL_INT, 1 },
	{ "u_WaterPositionMap", GLSL_INT, 1 },
	{ "u_WaterHeightMap", GLSL_INT, 1 },
	{ "u_HeightMap", GLSL_INT, 1 },
	{ "u_GlowMap", GLSL_INT, 1 },
	{ "u_EnvironmentMap", GLSL_INT, 1 },

	{ "u_TextureMap", GLSL_INT, 1 },
	{ "u_LevelsMap", GLSL_INT, 1 },
	{ "u_CubeMap", GLSL_INT, 1 },
	{ "u_SkyCubeMap", GLSL_INT, 1 },
	{ "u_SkyCubeMapNight", GLSL_INT, 1 },
	{ "u_EmissiveCubeMap", GLSL_INT, 1 },
	{ "u_OverlayMap", GLSL_INT, 1 },
	{ "u_SteepMap", GLSL_INT, 1 },
	{ "u_SteepMap1", GLSL_INT, 1 },
	{ "u_SteepMap2", GLSL_INT, 1 },
	{ "u_SteepMap3", GLSL_INT, 1 },
	{ "u_WaterEdgeMap", GLSL_INT, 1 },
	{ "u_SplatControlMap", GLSL_INT, 1 },
	{ "u_SplatMap1", GLSL_INT, 1 },
	{ "u_SplatMap2", GLSL_INT, 1 },
	{ "u_SplatMap3", GLSL_INT, 1 },
	{ "u_RoadsControlMap", GLSL_INT, 1 },
	{ "u_RoadMap", GLSL_INT, 1 },
	{ "u_DetailMap", GLSL_INT, 1 },

	{ "u_ScreenImageMap", GLSL_INT, 1 },
	{ "u_ScreenDepthMap", GLSL_INT, 1 },

	{ "u_ShadowMap", GLSL_INT, 1 },
	{ "u_ShadowMap2", GLSL_INT, 1 },
	{ "u_ShadowMap3", GLSL_INT, 1 },
	{ "u_ShadowMap4", GLSL_INT, 1 },
	{ "u_ShadowMap5", GLSL_INT, 1 },

	{ "u_MoonMaps", GLSL_INT, 4 },

	{ "u_ShadowMvp", GLSL_MAT16, 1 },
	{ "u_ShadowMvp2", GLSL_MAT16, 1 },
	{ "u_ShadowMvp3", GLSL_MAT16, 1 },
	{ "u_ShadowMvp4", GLSL_MAT16, 1 },
	{ "u_ShadowMvp5", GLSL_MAT16, 1 },

	{ "u_EnableTextures", GLSL_VEC4, 1 },
	{ "u_DiffuseTexMatrix", GLSL_VEC4, 1 },
	{ "u_DiffuseTexOffTurb", GLSL_VEC4, 1 },

	{ "u_TCGen0", GLSL_INT, 1 },
	{ "u_TCGen0Vector0", GLSL_VEC3, 1 },
	{ "u_TCGen0Vector1", GLSL_VEC3, 1 },

	{ "u_textureScale", GLSL_VEC2, 1 },

	{ "u_DeformGen", GLSL_INT, 1 },
	{ "u_DeformParams", GLSL_FLOAT7, 1 },

	{ "u_ColorGen", GLSL_INT, 1 },
	{ "u_AlphaGen", GLSL_INT, 1 },
	{ "u_Color", GLSL_VEC4, 1 },
	{ "u_BaseColor", GLSL_VEC4, 1 },
	{ "u_VertColor", GLSL_VEC4, 1 },
	{ "u_ColorMod", GLSL_VEC3, 1 },

	{ "u_DlightInfo", GLSL_VEC4, 1 },
	{ "u_LightForward", GLSL_VEC3, 1 },
	{ "u_LightUp", GLSL_VEC3, 1 },
	{ "u_LightRight", GLSL_VEC3, 1 },
	{ "u_LightOrigin", GLSL_VEC4, 1 },
	{ "u_LightColor", GLSL_VEC4, 1 },

	{ "u_ModelLightDir", GLSL_VEC3, 1 },
	{ "u_LightRadius", GLSL_FLOAT, 1 },
	{ "u_AmbientLight", GLSL_VEC3, 1 },
	{ "u_DirectedLight", GLSL_VEC3, 1 },

	{ "u_PortalRange", GLSL_FLOAT, 1 },

	{ "u_FogDistance", GLSL_VEC4, 1 },
	{ "u_FogDepth", GLSL_VEC4, 1 },
	{ "u_FogEyeT", GLSL_FLOAT, 1 },
	{ "u_FogColorMask", GLSL_VEC4, 1 },

	{ "u_ModelMatrix", GLSL_MAT16, 1 },
	{ "u_ViewProjectionMatrix", GLSL_MAT16, 1 },
	{ "u_ModelViewProjectionMatrix", GLSL_MAT16, 1 },
	{ "u_invProjectionMatrix", GLSL_MAT16, 1 },
	{ "u_invEyeProjectionMatrix", GLSL_MAT16, 1 },
	{ "u_invModelViewMatrix", GLSL_MAT16, 1 },
	{ "u_ProjectionMatrix", GLSL_MAT16, 1 },
	{ "u_ModelViewMatrix", GLSL_MAT16, 1 },
	{ "u_ViewMatrix", GLSL_MAT16, 1 },
	{ "u_invViewMatrix", GLSL_MAT16, 1 },
	{ "u_NormalMatrix", GLSL_MAT16, 1 },

	{ "u_Time", GLSL_FLOAT, 1 },
	{ "u_VertexLerp", GLSL_FLOAT, 1 },
	{ "u_NormalScale", GLSL_VEC4, 1 },
	{ "u_SpecularScale", GLSL_VEC4, 1 },

	{ "u_ViewInfo", GLSL_VEC4, 1 },
	{ "u_ViewOrigin", GLSL_VEC3, 1 },
	{ "u_LocalViewOrigin", GLSL_VEC3, 1 },
	{ "u_ViewForward", GLSL_VEC3, 1 },
	{ "u_ViewLeft", GLSL_VEC3, 1 },
	{ "u_ViewUp", GLSL_VEC3, 1 },
	{ "u_PlayerOrigin", GLSL_VEC3, 1 },
#ifdef __HUMANOIDS_BEND_GRASS__
	{ "u_HumanoidOriginsNum", GLSL_INT, 1 },
	{ "u_HumanoidOrigins", GLSL_VEC3, MAX_GRASSBEND_HUMANOIDS },
#endif //__HUMANOIDS_BEND_GRASS__

	{ "u_InstancePositions", GLSL_VEC3, MAX_INSTANCED_MODEL_INSTANCES },
	{ "u_InstanceScales", GLSL_VEC3, MAX_INSTANCED_MODEL_INSTANCES },
	{ "u_InstanceMatrixes", GLSL_MAT16, MAX_INSTANCED_MODEL_INSTANCES },

	{ "u_InvTexRes", GLSL_VEC2, 1 },
	{ "u_AutoExposureMinMax", GLSL_VEC2, 1 },
	{ "u_ToneMinAvgMaxLinear", GLSL_VEC3, 1 },

	{ "u_PrimaryLightOrigin", GLSL_VEC4, 1 },
	{ "u_PrimaryLightColor", GLSL_VEC3, 1 },
	{ "u_PrimaryLightAmbient", GLSL_VEC3, 1 },
	{ "u_PrimaryLightRadius", GLSL_FLOAT, 1 },

	{ "u_CubeMapInfo", GLSL_VEC4, 1 },
	{ "u_CubeMapStrength", GLSL_FLOAT, 1 },

	{ "u_BoneMatrices", GLSL_MAT16, MAX_GLM_BONEREFS/*20*/ },
#ifdef __EXPERIMETNAL_CHARACTER_EDITOR__
	{ "u_BoneScales", GLSL_FLOAT, 20 },
#endif //__EXPERIMETNAL_CHARACTER_EDITOR__

		// UQ1: Added...
	{ "u_Mins", GLSL_VEC4, 1 },
	{ "u_Maxs", GLSL_VEC4, 1 },
	{ "u_MapInfo", GLSL_VEC4, 1 },

	{ "u_ShadowZfar", GLSL_FLOAT, 5 },

	{ "u_AlphaTestValues", GLSL_VEC2, 1 },

	{ "u_Dimensions", GLSL_VEC2, 1 },
	{ "u_MapAmbient", GLSL_VEC4, 1 },
	{ "u_zFar", GLSL_FLOAT, 1 },
	{ "u_isWorld", GLSL_INT, 1 },

	{ "u_Settings0", GLSL_VEC4, 1 },
	{ "u_Settings1", GLSL_VEC4, 1 },
	{ "u_Settings2", GLSL_VEC4, 1 },
	{ "u_Settings3", GLSL_VEC4, 1 },
	{ "u_Settings4", GLSL_VEC4, 1 },
	{ "u_Settings5", GLSL_VEC4, 1 },
	{ "u_Local0", GLSL_VEC4, 1 },
	{ "u_Local1", GLSL_VEC4, 1 },
	{ "u_Local2", GLSL_VEC4, 1 },
	{ "u_Local3", GLSL_VEC4, 1 },
	{ "u_Local4", GLSL_VEC4, 1 },
	{ "u_Local5", GLSL_VEC4, 1 },
	{ "u_Local6", GLSL_VEC4, 1 },
	{ "u_Local7", GLSL_VEC4, 1 },
	{ "u_Local8", GLSL_VEC4, 1 },
	{ "u_Local9", GLSL_VEC4, 1 },
	{ "u_Local10", GLSL_VEC4, 1 },
	{ "u_Local11", GLSL_VEC4, 1 },
	{ "u_Local12", GLSL_VEC4, 1 },
	{ "u_Local13", GLSL_VEC4, 1 },
	{ "u_Local14", GLSL_VEC4, 1 },
	{ "u_Local15", GLSL_VEC4, 1 },
	{ "u_Local16", GLSL_VEC4, 1 },
	{ "u_Local17", GLSL_VEC4, 1 },
	{ "u_Local18", GLSL_VEC4, 1 },
	{ "u_Local19", GLSL_VEC4, 1 },
	{ "u_Local20", GLSL_VEC4, 1 },

	{ "u_MoonCount", GLSL_INT, 1 },
	{ "u_MoonInfos", GLSL_VEC4, 4 },
	{ "u_MoonInfos2", GLSL_VEC2, 4 },

	{ "u_MaterialSpeculars", GLSL_FLOAT, MATERIAL_LAST },
	{ "u_MaterialReflectiveness", GLSL_FLOAT, MATERIAL_LAST },

	{ "u_TesselationInfo", GLSL_VEC4, 1 },
	{ "u_Tesselation3DInfo", GLSL_VEC4, 1 },

	{ "u_lightCount", GLSL_INT, 1 },
	{ "u_lightPositions2", GLSL_VEC3, MAX_DEFERRED_LIGHTS },
	{ "u_lightPositions", GLSL_VEC2, MAX_DEFERRED_LIGHTS },
	{ "u_lightDistances", GLSL_FLOAT, MAX_DEFERRED_LIGHTS },
	{ "u_lightHeightScales", GLSL_FLOAT, MAX_DEFERRED_LIGHTS },
	{ "u_lightConeAngles", GLSL_FLOAT, MAX_DEFERRED_LIGHTS },
	{ "u_lightConeDirections", GLSL_VEC3, MAX_DEFERRED_LIGHTS },
	{ "u_lightMax", GLSL_INT, 1 },
	{ "u_lightMaxDistance", GLSL_FLOAT, 1 },
	{ "u_lightColors", GLSL_VEC3, MAX_DEFERRED_LIGHTS },
	{ "u_vlightPositions2", GLSL_VEC3, 1 },
	{ "u_vlightPositions", GLSL_VEC2, 1 },
	{ "u_vlightDistances", GLSL_FLOAT, 1 },
	{ "u_vlightColors", GLSL_VEC3, 1 },

	{ "u_GlowMultiplier", GLSL_VEC4, 1 },

	{ "u_Samples", GLSL_INT, 1 },
	//{ "u_SsdoKernel", GLSL_VEC3, 32 },
};

void GLSL_PrintProgramInfoLog(GLuint object, qboolean developerOnly)
{
	char           *msg;
	static char     msgPart[1024];
	int             maxLength = 0;
	int             i;
#ifdef __DEVELOPER_MODE__
	int             printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_ALL;
#else //!__DEVELOPER_MODE__
	int             printLevel = PRINT_ALL;

	if (developerOnly)
	{
		return;
	}
#endif //__DEVELOPER_MODE__

	qglGetProgramiv(object, GL_INFO_LOG_LENGTH, &maxLength);

	if (maxLength <= 0)
	{
		ri->Printf(printLevel, "No compile log.\n");
		return;
	}

	ri->Printf(printLevel, "compile log:\n");

	if (maxLength < 1023)
	{
		qglGetProgramInfoLog(object, maxLength, &maxLength, msgPart);

		msgPart[maxLength + 1] = '\0';

		ri->Printf(printLevel, "%s\n", msgPart);
	}
	else
	{
		msg = (char *)Z_Malloc(maxLength, TAG_SHADERTEXT);

		qglGetProgramInfoLog(object, maxLength, &maxLength, msg);

		for (i = 0; i < maxLength; i += 1024)
		{
			Q_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri->Printf(printLevel, "%s\n", msgPart);
		}

		Z_Free(msg);
	}
}

void GLSL_PrintShaderInfoLog(GLuint object, qboolean developerOnly)
{
	char           *msg;
	static char     msgPart[1024];
	int             maxLength = 0;
	int             i;
#ifdef __DEVELOPER_MODE__
	int             printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_ALL;
#else //!__DEVELOPER_MODE__
	int             printLevel = PRINT_ALL;

	if (developerOnly)
	{
		return;
	}
#endif //__DEVELOPER_MODE__


	qglGetShaderiv(object, GL_INFO_LOG_LENGTH, &maxLength);

	if (maxLength <= 0)
	{
		ri->Printf(printLevel, "No compile log.\n");
		return;
	}

	ri->Printf(printLevel, "compile log:\n");

	if (maxLength < 1023)
	{
		qglGetShaderInfoLog(object, maxLength, &maxLength, msgPart);

		msgPart[maxLength + 1] = '\0';

		ri->Printf(printLevel, "%s\n", msgPart);
	}
	else
	{
		msg = (char *)Z_Malloc(maxLength, TAG_SHADERTEXT);

		qglGetShaderInfoLog(object, maxLength, &maxLength, msg);

		for (i = 0; i < maxLength; i += 1024)
		{
			Q_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri->Printf(printLevel, "%s\n", msgPart);
		}

		Z_Free(msg);
	}
}

void GLSL_PrintShaderSource(GLuint shader)
{
	char           *msg;
	static char     msgPart[1024];
	int             maxLength = 0;
	int             i;

	qglGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &maxLength);

	msg = (char *)Z_Malloc(maxLength, TAG_SHADERTEXT);

	qglGetShaderSource(shader, maxLength, &maxLength, msg);

	for (i = 0; i < maxLength; i += 1024)
	{
		Q_strncpyz(msgPart, msg + i, sizeof(msgPart));
		ri->Printf(PRINT_ALL, "%s\n", msgPart);
	}

	Z_Free(msg);
}

qboolean ALLOW_GL_400 = qfalse;

char *GLSL_GetHighestSupportedVersion(void)
{
	ALLOW_GL_400 = qfalse;

	if (glRefConfig.glslMajorVersion >= 4)
	{
		ALLOW_GL_400 = qtrue;

		if (glRefConfig.glslMajorVersion >= 5)
			return "#version 500 core\n";
		else if (glRefConfig.glslMajorVersion >= 4 && glRefConfig.glslMinorVersion >= 20)
			return "#version 420 core\n";
		else if (glRefConfig.glslMajorVersion >= 4 && glRefConfig.glslMinorVersion >= 10)
			return "#version 410 core\n";
		else
			return "#version 400 core\n";
	}
	else
	{
		// UQ1: Use the highest level of GL that is supported... Check once and record for all future glsl loading...
		if (glRefConfig.glslMajorVersion >= 3 && glRefConfig.glslMinorVersion >= 30)
			return "#version 330\n";
		else if (glRefConfig.glslMajorVersion >= 3 && glRefConfig.glslMinorVersion >= 20)
			return "#version 320\n";
		else if (glRefConfig.glslMajorVersion >= 3 && glRefConfig.glslMinorVersion >= 10)
			return "#version 310\n";
		else if (glRefConfig.glslMajorVersion >= 3)
			return "#version 300\n";
		else if (glRefConfig.glslMajorVersion >= 2 && glRefConfig.glslMinorVersion >= 20)
			return "#version 220\n";
		else if (glRefConfig.glslMajorVersion >= 2 && glRefConfig.glslMinorVersion >= 10)
			return "#version 210\n";
		else if (glRefConfig.glslMajorVersion >= 2)
			return "#version 200\n";
		else
			return "#version 150\n";
	}

	return "#version 150\n";
}

const char glslMaterialsList[] =
"#define MATERIAL_NONE							0\n"\
"#define MATERIAL_SOLIDWOOD						1\n"\
"#define MATERIAL_HOLLOWWOOD					2\n"\
"#define MATERIAL_SOLIDMETAL					3\n"\
"#define MATERIAL_HOLLOWMETAL					4\n"\
"#define MATERIAL_SHORTGRASS					5\n"\
"#define MATERIAL_LONGGRASS						6\n"\
"#define MATERIAL_DIRT							7\n"\
"#define MATERIAL_SAND							8\n"\
"#define MATERIAL_GRAVEL						9\n"\
"#define MATERIAL_GLASS							10\n"\
"#define MATERIAL_CONCRETE						11\n"\
"#define MATERIAL_MARBLE						12\n"\
"#define MATERIAL_WATER							13\n"\
"#define MATERIAL_SNOW							14\n"\
"#define MATERIAL_ICE							15\n"\
"#define MATERIAL_FLESH							16\n"\
"#define MATERIAL_MUD							17\n"\
"#define MATERIAL_BPGLASS						18\n"\
"#define MATERIAL_DRYLEAVES						19\n"\
"#define MATERIAL_GREENLEAVES					20\n"\
"#define MATERIAL_FABRIC						21\n"\
"#define MATERIAL_CANVAS						22\n"\
"#define MATERIAL_ROCK							23\n"\
"#define MATERIAL_RUBBER						24\n"\
"#define MATERIAL_PLASTIC						25\n"\
"#define MATERIAL_TILES							26\n"\
"#define MATERIAL_CARPET						27\n"\
"#define MATERIAL_PLASTER						28\n"\
"#define MATERIAL_SHATTERGLASS					29\n"\
"#define MATERIAL_ARMOR							30\n"\
"#define MATERIAL_COMPUTER						31\n"\
"#define MATERIAL_PUDDLE						32\n"\
"#define MATERIAL_POLISHEDWOOD					33\n"\
"#define MATERIAL_LAVA							34\n"\
"#define MATERIAL_TREEBARK						35\n"\
"#define MATERIAL_STONE							36\n"\
"#define MATERIAL_EFX							37\n"\
"#define MATERIAL_BLASTERBOLT					38\n"\
"#define MATERIAL_FIRE							39\n"\
"#define MATERIAL_SMOKE							40\n"\
"#define MATERIAL_FIREFLIES						41\n"\
"#define MATERIAL_MAGIC_PARTICLES_TREE			42\n"\
"#define MATERIAL_MAGIC_PARTICLES				43\n"\
"#define MATERIAL_PORTAL						44\n"\
"#define MATERIAL_MENU_BACKGROUND				45\n"\
"#define MATERIAL_SKYSCRAPER					46\n"\
"#define MATERIAL_DISTORTEDGLASS				47\n"\
"#define MATERIAL_DISTORTEDPUSH					48\n"\
"#define MATERIAL_DISTORTEDPULL					49\n"\
"#define MATERIAL_CLOAK							50\n"\
"#define MATERIAL_PROCEDURALFOLIAGE				51\n"\
"#define MATERIAL_LAST							52\n"\
"#define MATERIAL_SKY							1024\n"\
"#define MATERIAL_SUN							1025\n"\
"\n";

void GLSL_GetShaderHeader(GLenum shaderType, const GLcharARB *extra, std::string &dest, char *forceVersion, qboolean bindless)
{
	float fbufWidthScale, fbufHeightScale;

	//dest[0] = '\0';

	if (forceVersion)
		dest.append(va("#version %s\n", forceVersion));
	else
		dest.append(va("%s", GLSL_GetHighestSupportedVersion()));

	fbufWidthScale = 1.0f / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
	fbufHeightScale = 1.0f / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

	if (r_useLowP->integer)
	{
		dest.append("precision lowp int;\n");
		dest.append("precision lowp float;\n");
	}

#ifdef __INVERSE_DEPTH_BUFFERS__
	dest.append("#define INVERSE_DEPTH_BUFFERS\n");
#endif //__INVERSE_DEPTH_BUFFERS__

	if (bindless && glRefConfig.bindlessTextures)
	{
		dest.append("#define USE_BINDLESS_TEXTURES\n\n");
		dest.append("#extension GL_ARB_bindless_texture : require\n");
	}

#ifdef __LIGHTS_UBO__
	dest.append("#define USE_LIGHTS_UBO\n\n");
#endif //__LIGHTS_UBO__

	if (shaderType == GL_VERTEX_SHADER)
	{
		dest.append("#define attribute in\n");
		dest.append("#define varying out\n");

#ifdef __VBO_PACK_COLOR__
		dest.append("#define __VBO_PACK_COLOR__\n");
#endif //__VBO_PACK_COLOR__

#ifdef __CHEAP_VERTS__
		dest.append("#define __CHEAP_VERTS__\n");
#endif //__CHEAP_VERTS__

		dest.append(glslMaterialsList);
	}
	else if (shaderType == GL_TESS_CONTROL_SHADER)
	{
		dest.append(va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));
		dest.append(glslMaterialsList);

		if (extra)
		{
			dest.append(extra);
		}

		//GLint MaxPatchVertices = 0;
		//qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
		dest.append(va("#define MAX_PATCH_VERTICES %i\n", 3/*MaxPatchVertices >= 16 ? 16 : 3*/));

		// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
		// so we have to reset the line counting
		dest.append("#line 0\n");
		return;
	}
	else if (shaderType == GL_TESS_EVALUATION_SHADER)
	{
		dest.append(va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));
		dest.append(glslMaterialsList);

		if (extra)
		{
			dest.append(extra);
		}

		dest.append("#ifndef M_PI\n#define M_PI 3.14159265358979323846\n#endif\n");

		//GLint MaxPatchVertices = 0;
		//qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
		dest.append(va("#define MAX_PATCH_VERTICES %i\n", 3/*MaxPatchVertices >= 16 ? 16 : 3*/));

		dest.append(va("#ifndef deformGen_t\n"
				"#define deformGen_t\n"
				"#define DGEN_WAVE_SIN %i\n"
				"#define DGEN_WAVE_SQUARE %i\n"
				"#define DGEN_WAVE_TRIANGLE %i\n"
				"#define DGEN_WAVE_SAWTOOTH %i\n"
				"#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
				"#define DGEN_BULGE %i\n"
				"#define DGEN_MOVE %i\n"
				"#define DGEN_PROJECTION_SHADOW %i\n"
				"#endif\n",
				DGEN_WAVE_SIN,
				DGEN_WAVE_SQUARE,
				DGEN_WAVE_TRIANGLE,
				DGEN_WAVE_SAWTOOTH,
				DGEN_WAVE_INVERSE_SAWTOOTH,
				DGEN_BULGE,
				DGEN_MOVE,
				DGEN_PROJECTION_SHADOW));

		dest.append(va("#ifndef tcGen_t\n"
				"#define tcGen_t\n"
				"#define TCGEN_LIGHTMAP %i\n"
				"#define TCGEN_LIGHTMAP1 %i\n"
				"#define TCGEN_LIGHTMAP2 %i\n"
				"#define TCGEN_LIGHTMAP3 %i\n"
				"#define TCGEN_TEXTURE %i\n"
				"#define TCGEN_ENVIRONMENT_MAPPED %i\n"
				"#define TCGEN_FOG %i\n"
				"#define TCGEN_VECTOR %i\n"
				"#endif\n",
				TCGEN_LIGHTMAP,
				TCGEN_LIGHTMAP1,
				TCGEN_LIGHTMAP2,
				TCGEN_LIGHTMAP3,
				TCGEN_TEXTURE,
				TCGEN_ENVIRONMENT_MAPPED,
				TCGEN_FOG,
				TCGEN_VECTOR));

		// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
		// so we have to reset the line counting
		dest.append("#line 0\n");
		return;
	}
	else if (shaderType == GL_GEOMETRY_SHADER)
	{
		dest.append(va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));
		dest.append(glslMaterialsList);

		if (extra)
		{
			dest.append(extra);
		}

		// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
		// so we have to reset the line counting
		dest.append("#line 0\n");
		return;
	}
	else
	{
		dest.append("#define varying in\n");

		dest.append("out vec4 out_Color;\n");
		dest.append("#define gl_FragColor out_Color\n");
	}

	dest.append("#ifndef M_PI\n#define M_PI 3.14159265358979323846\n#endif\n");

	dest.append(va("#ifndef deformGen_t\n"
		"#define deformGen_t\n"
		"#define DGEN_WAVE_SIN %i\n"
		"#define DGEN_WAVE_SQUARE %i\n"
		"#define DGEN_WAVE_TRIANGLE %i\n"
		"#define DGEN_WAVE_SAWTOOTH %i\n"
		"#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
		"#define DGEN_BULGE %i\n"
		"#define DGEN_MOVE %i\n"
		"#define DGEN_PROJECTION_SHADOW %i\n"
		"#endif\n",
		DGEN_WAVE_SIN,
		DGEN_WAVE_SQUARE,
		DGEN_WAVE_TRIANGLE,
		DGEN_WAVE_SAWTOOTH,
		DGEN_WAVE_INVERSE_SAWTOOTH,
		DGEN_BULGE,
		DGEN_MOVE,
		DGEN_PROJECTION_SHADOW));

	dest.append(va("#ifndef tcGen_t\n"
		"#define tcGen_t\n"
		"#define TCGEN_LIGHTMAP %i\n"
		"#define TCGEN_LIGHTMAP1 %i\n"
		"#define TCGEN_LIGHTMAP2 %i\n"
		"#define TCGEN_LIGHTMAP3 %i\n"
		"#define TCGEN_TEXTURE %i\n"
		"#define TCGEN_ENVIRONMENT_MAPPED %i\n"
		"#define TCGEN_FOG %i\n"
		"#define TCGEN_VECTOR %i\n"
		"#endif\n",
		TCGEN_LIGHTMAP,
		TCGEN_LIGHTMAP1,
		TCGEN_LIGHTMAP2,
		TCGEN_LIGHTMAP3,
		TCGEN_TEXTURE,
		TCGEN_ENVIRONMENT_MAPPED,
		TCGEN_FOG,
		TCGEN_VECTOR));

	dest.append(va("#ifndef colorGen_t\n"
		"#define colorGen_t\n"
		"#define CGEN_LIGHTING_DIFFUSE %i\n"
		"#endif\n",
		CGEN_LIGHTING_DIFFUSE));

	dest.append(va("#ifndef alphaGen_t\n"
		"#define alphaGen_t\n"
		"#define AGEN_LIGHTING_SPECULAR %i\n"
		"#define AGEN_PORTAL %i\n"
		"#endif\n",
		AGEN_LIGHTING_SPECULAR,
		AGEN_PORTAL));

	dest.append(va("#ifndef texenv_t\n"
		"#define texenv_t\n"
		"#define TEXENV_MODULATE %i\n"
		"#define TEXENV_ADD %i\n"
		"#define TEXENV_REPLACE %i\n"
		"#endif\n",
		GL_MODULATE,
		GL_ADD,
		GL_REPLACE));

	dest.append(va("#ifndef colorGenWarzone_t\n"
			"#define colorGenWarzone_t\n"
			"#define CGEN_LIGHTING_WARZONE %i\n"
			"#endif\n",
			CGEN_LIGHTING_WARZONE));

	if (!r_lowVram->integer)
		dest.append("#define __HIGH_MTU_AVAILABLE__\n");

	if (r_lowVram->integer)
		dest.append("#define __LQ_MODE__\n");

	dest.append(va("#define MAX_INSTANCED_MODEL_INSTANCES %i\n", MAX_INSTANCED_MODEL_INSTANCES));

	dest.append(va("#define MAX_DEFERRED_LIGHTS %i\n", MAX_DEFERRED_LIGHTS));
	dest.append(va("#define MAX_DEFERRED_LIGHT_RANGE %f\n", MAX_DEFERRED_LIGHT_RANGE));

	dest.append(va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));
	dest.append(glslMaterialsList);

	if (r_normalMappingReal->integer)
	{
		dest.append("#define __USE_REAL_NORMALMAPS__\n");
	}

	if (extra)
	{
		dest.append(extra);
	}

	// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	dest.append("#line 0\n");
}

int GLSL_EnqueueCompileGPUShader(GLuint program, GLuint *prevShader, const GLchar *buffer, int size, GLenum shaderType)
{
	GLuint     shader;

	shader = qglCreateShader(shaderType);

	qglShaderSource(shader, 1, &buffer, &size);

	// compile shader
	qglCompileShader(shader);

	*prevShader = shader;

	return 1;
}

void GLSL_LinkProgram(GLuint program)
{
	GLint           linked;

	qglLinkProgram(program);

	qglGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLSL_PrintProgramInfoLog(program, qfalse);
		ri->Printf(PRINT_ALL, "\n");
		ri->Error(ERR_DROP, "shaders failed to link");
	}
}

GLint GLSL_LinkProgramSafe(GLuint program) {
	GLint           linked;
	qglLinkProgram(program);
	qglGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLSL_PrintProgramInfoLog(program, qfalse);
		Com_Printf("shaders failed to link\n");
	}
	return linked;
}

void GLSL_ValidateProgram(GLuint program)
{
	GLint           validated;

	qglValidateProgram(program);

	qglGetProgramiv(program, GL_VALIDATE_STATUS, &validated);
	if (!validated)
	{
		GLSL_PrintProgramInfoLog(program, qfalse);
		ri->Printf(PRINT_ALL, "\n");
		ri->Error(ERR_DROP, "shaders failed to validate");
	}
}

void GLSL_ShowProgramUniforms(shaderProgram_t *program)
{
	int             i, count, size;
	GLenum			type;
	char            uniformName[1000];

	// install the executables in the program object as part of current state.
	GLSL_BindProgram(program);

	// check for GL Errors

	// query the number of active uniforms
	qglGetProgramiv(program->program, GL_ACTIVE_UNIFORMS, &count);

	// Loop over each of the active uniforms, and set their value
	for (i = 0; i < count; i++)
	{
		qglGetActiveUniform(program->program, i, sizeof(uniformName), NULL, &size, &type, uniformName);

#ifdef __DEVELOPER_MODE__
		ri->Printf(PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName);
#endif //__DEVELOPER_MODE__
	}

	GLSL_BindProgram(NULL);
}

int GLSL_BeginLoadGPUShader2(shaderProgram_t * program, std::string name, int attribs, std::string vpCode, std::string fpCode, std::string cpCode, std::string epCode, std::string gsCode, qboolean bindless)
{
#ifdef __DEVELOPER_MODE__
	ri->Printf(PRINT_DEVELOPER, "^5------- ^7GPU shader^5 -------\n");
#endif //__DEVELOPER_MODE__

	program->name = name;

	program->program = qglCreateProgram();
	program->attribs = attribs;

	program->tesselation = qfalse;
	program->tessControlShader = NULL;
	program->tessEvaluationShader = NULL;

	program->geometry = qfalse;
	program->geometryShader = NULL;

	program->isBindless = bindless;

	// Refresh the bindless block when compiling shaders as well..
	memset(&program->bindlessBlock, 0, sizeof(program->bindlessBlock));
	memset(&program->bindlessBlockPrevious, 0, sizeof(program->bindlessBlockPrevious));

	program->vertexText = vpCode;
	strcpy(program->vertexTextChar, vpCode.c_str());
	if (!(GLSL_EnqueueCompileGPUShader(program->program, &program->vertexShader, program->vertexText.c_str(), program->vertexText.length(), GL_VERTEX_SHADER)))
	{
		ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER\n", program->name.c_str());
		qglDeleteProgram(program->program);
		return 0;
	}

	program->tessControlText = cpCode;
	if (program->tessControlText.length())
	{
		if (!(GLSL_EnqueueCompileGPUShader(program->program, &program->tessControlShader, program->tessControlText.c_str(), program->tessControlText.length(), GL_TESS_CONTROL_SHADER)))
		{
			ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Unable to load \"%s\" as GL_TESS_CONTROL_SHADER\n", program->name.c_str());
			program->tessControlShader = 0;
			qglDeleteProgram(program->program);
			return 0;
		}
		else
		{
			//ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Load \"%s\" as GL_TESS_CONTROL_SHADER\n", name);
			program->tesselation = qtrue;
		}
	}
	else
	{
		program->tessControlShader = 0;
	}

	program->tessEvaluationText = epCode;
	if (program->tessEvaluationText.length())
	{
		if (!(GLSL_EnqueueCompileGPUShader(program->program, &program->tessEvaluationShader, program->tessEvaluationText.c_str(), program->tessEvaluationText.length(), GL_TESS_EVALUATION_SHADER)))
		{
			ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Unable to load \"%s\" as GL_TESS_EVALUATION_SHADER\n", program->name.c_str());
			program->tessEvaluationShader = 0;
			qglDeleteProgram(program->program);
			return 0;
		}
		else
		{
			//ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Load \"%s\" as GL_TESS_EVALUATION_SHADER\n", name);
			program->tesselation = qtrue;
		}
	}
	else
	{
		program->tessEvaluationShader = 0;
	}

	program->geometryText = gsCode;
	if (program->geometryText.length())
	{
		if (!(GLSL_EnqueueCompileGPUShader(program->program, &program->geometryShader, program->geometryText.c_str(), program->geometryText.length(), GL_GEOMETRY_SHADER)))
		{
			ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Unable to load \"%s\" as GL_GEOMETRY_SHADER\n", program->name.c_str());
			program->geometryShader = 0;
			qglDeleteProgram(program->program);
			return 0;
		}
		else
		{
			//ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Load \"%s\" as GL_GEOMETRY_SHADER\n", name);
			program->geometry = qtrue;
		}
	}
	else
	{
		program->geometryShader = 0;
	}

	program->fragText = fpCode;
	strcpy(program->fragTextChar, fpCode.c_str());
	if (program->fragText.length())
	{
		if (!(GLSL_EnqueueCompileGPUShader(program->program, &program->fragmentShader, program->fragText.c_str(), program->fragText.length(), GL_FRAGMENT_SHADER)))
		{
			ri->Printf(PRINT_ALL, "GLSL_BeginLoadGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER\n", program->name.c_str());
			qglDeleteProgram(program->program);
			return 0;
		}
	}

	return 1;
}

bool GLSL_IsGPUShaderCompiled(GLuint shader)
{
	GLint compiled;

	qglGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLSL_PrintShaderSource(shader);
		GLSL_PrintShaderInfoLog(shader, qfalse);
		ri->Error(ERR_DROP, "Couldn't compile shader");
		return qfalse;
	}

	return qtrue;
}

extern void FBO_AttachTextureImage(image_t *img, int index);
extern void R_AttachFBOTextureDepth(int texId);
extern void SetViewportAndScissor(void);
extern void FBO_SetupDrawBuffers();
extern qboolean R_CheckFBO(const FBO_t * fbo);

void GLSL_AttachTextures(void)
{// Moved here for convenience...
	FBO_AttachTextureImage(tr.renderImage, 0);
	FBO_AttachTextureImage(tr.glowImage, 1);
	FBO_AttachTextureImage(tr.renderNormalImage, 2);
	FBO_AttachTextureImage(tr.renderPositionMapImage, 3);
	if (r_normalMappingReal->integer)
	{
		FBO_AttachTextureImage(tr.renderNormalDetailedImage, 4);
	}
}

void GLSL_AttachGlowTextures(void)
{// Moved here for convenience...
	FBO_AttachTextureImage(tr.renderImage, 0);
	FBO_AttachTextureImage(tr.glowImage, 1);
}

void GLSL_AttachGenericTextures(void)
{// Moved here for convenience...
	FBO_AttachTextureImage(tr.renderImage, 0);
}

void GLSL_AttachRenderDepthTextures(void)
{// Moved here for convenience...
	FBO_AttachTextureImage(tr.renderImage, 0);
}

void GLSL_AttachRenderGUITextures(void)
{// Moved here for convenience...
	FBO_AttachTextureImage(tr.renderGUIImage, 0);
}

void GLSL_AttachWaterTextures(void)
{// To output dummy textures on waters in RB_IterateStagesGeneric...
	FBO_AttachTextureImage(tr.dummyImage, 0); // dummy
	FBO_AttachTextureImage(tr.dummyImage2, 1); // dummy
	FBO_AttachTextureImage(tr.dummyImage3, 2); // dummy
	FBO_AttachTextureImage(tr.waterPositionMapImage, 3); // water positions
	if (r_normalMappingReal->integer)
	{
		FBO_AttachTextureImage(tr.dummyImage4, 4);
	}
}

void GLSL_AttachTransparancyTextures(void)
{// To output dummy textures on waters in RB_IterateStagesGeneric...
	FBO_AttachTextureImage(tr.dummyImage, 0); // dummy
	FBO_AttachTextureImage(tr.dummyImage2, 1); // dummy
	FBO_AttachTextureImage(tr.dummyImage3, 2); // dummy
	FBO_AttachTextureImage(tr.transparancyMapImage, 3); // transparancy info
	if (r_normalMappingReal->integer)
	{
		FBO_AttachTextureImage(tr.dummyImage4, 4);
	}
}

void GLSL_BindAttributeLocations(shaderProgram_t *program, int attribs) {
	if (attribs & ATTR_POSITION      )		qglBindAttribLocation(program->program, ATTR_INDEX_POSITION      , "attr_Position"      );
	if (attribs & ATTR_TEXCOORD0     )		qglBindAttribLocation(program->program, ATTR_INDEX_TEXCOORD0     , "attr_TexCoord0"     );
	if (attribs & ATTR_TEXCOORD1     )		qglBindAttribLocation(program->program, ATTR_INDEX_TEXCOORD1     , "attr_TexCoord1"     );
	if (attribs & ATTR_NORMAL        )		qglBindAttribLocation(program->program, ATTR_INDEX_NORMAL        , "attr_Normal"        );
	if (attribs & ATTR_COLOR         )		qglBindAttribLocation(program->program, ATTR_INDEX_COLOR         , "attr_Color"         );
	if (attribs & ATTR_POSITION2     )		qglBindAttribLocation(program->program, ATTR_INDEX_POSITION2     , "attr_Position2"     );
	if (attribs & ATTR_NORMAL2       )		qglBindAttribLocation(program->program, ATTR_INDEX_NORMAL2       , "attr_Normal2"       );
	if (attribs & ATTR_BONE_INDEXES  )		qglBindAttribLocation(program->program, ATTR_INDEX_BONE_INDEXES  , "attr_BoneIndexes"   );
	if (attribs & ATTR_BONE_WEIGHTS  )		qglBindAttribLocation(program->program, ATTR_INDEX_BONE_WEIGHTS  , "attr_BoneWeights"   );

	if (attribs & ATTR_INSTANCES_TEXCOORD)	qglBindAttribLocation(program->program, ATTR_INDEX_INSTANCES_TEXCOORD, "attr_InstancesTexCoord");
	if (attribs & ATTR_INSTANCES_POSITION)	qglBindAttribLocation(program->program, ATTR_INDEX_INSTANCES_POSITION, "attr_InstancesPosition");
	if (attribs & ATTR_INSTANCES_MVP)		qglBindAttribLocation(program->program, ATTR_INDEX_INSTANCES_MVP, "attr_InstancesMVP");
}

#ifdef __USE_GLSL_SHADER_CACHE__
void GLSL_SaveShaderToCache(shaderProgram_t *program)
{
	if (program->name[0] == '#') return; // Q_strcat/va corruption?

	ri->Printf(PRINT_WARNING, "Saving shader %s binary to cache file %s.\n", program->name, va("glslCache/%s.cache", program->name));

	GLint formats = 0;
	qglGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);

	GLint *binaryFormats = new GLint[formats];
	qglGetIntegerv(GL_PROGRAM_BINARY_FORMATS, binaryFormats);

	GLint len = 0;
	qglGetProgramiv(program->program, GL_PROGRAM_BINARY_LENGTH, &len);

	uint8_t* binary = new uint8_t[len];
	//GLenum *binaryFormats = 0;
	GLsizei finalLength = 0;
	qglGetProgramBinary(program->program, len, &finalLength, (GLenum*)binaryFormats, binary);

	ri->FS_WriteFile(va("glslCache/%s.cache", program->name), binary, finalLength);
	
	delete[] binary;
}

int GLSL_LoadShaderFromCache(shaderProgram_t *program, const char *name)
{
	extern qboolean GLimp_HaveExtension(const char *ext);

	if (!GLimp_HaveExtension("ARB_get_program_binary"))
	{
		return -1;
	}

	//ri->Printf(PRINT_WARNING, "Loading shader %s from cache.\n", name);

	size_t nameBufSize = strlen(name) + 1;
	program->name = (char *)Z_Malloc(nameBufSize, TAG_GENERAL);
	Q_strncpyz(program->name, name, nameBufSize);

	uint8_t *binary = NULL;
	int len = ri->FS_ReadFile(va("glslCache/%s.cache", program->name), (void **)&binary);

	if (len <= 0)
	{
		Z_Free(program->name);
		return -1; // Make sure the code knows it failed, so it can fall back to a recompile...
	}

	GLint formats = 0;
	qglGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);

	GLint *binaryFormats = new GLint[formats];
	qglGetIntegerv(GL_PROGRAM_BINARY_FORMATS, binaryFormats);

	GLuint progId = qglCreateProgram();
	qglProgramBinary(progId, (GLenum)*binaryFormats, binary, len);

	ri->FS_FreeFile(binary);

	GLint success;
	qglGetProgramiv(progId, GL_LINK_STATUS, &success);

	if (!success)
	{// Loading failed...
		//ri->Printf(PRINT_WARNING, "%s no success loading from cache.\n", name);
		Z_Free(program->name);

		/*
		GLSL_PrintProgramInfoLog(progId, qfalse);
		ri->Printf(PRINT_ALL, "\n");
		ri->Error(ERR_DROP, "shaders failed to link");
		*/

		return -1; // Make sure the code knows it failed, so it can fall back to a recompile...
	}

	ri->Printf(PRINT_WARNING, "%s loaded from cache.\n", name);
	program->program = progId;
	program->binaryLoaded = qtrue;
	return (int)progId;
}
#endif //__USE_GLSL_SHADER_CACHE__

bool GLSL_EndLoadGPUShader(shaderProgram_t *program)
{
	uint32_t attribs = program->attribs;

	//ri->Printf(PRINT_WARNING, "Compiling glsl: %s.\n", program->name);

#ifdef __USE_GLSL_SHADER_CACHE__
	extern qboolean GLimp_HaveExtension(const char *ext);

	if (GLimp_HaveExtension("ARB_get_program_binary"))
	{
		if (program->binaryLoaded)
		{
			/*qglBindFragDataLocation(program->program, 0, "out_Color");
			qglBindFragDataLocation(program->program, 1, "out_Glow");
			qglBindFragDataLocation(program->program, 2, "out_Normal");
			qglBindFragDataLocation(program->program, 3, "out_Position");

			if (r_normalMappingReal->integer || program == &tr.linearizeDepthShader)
			{
				qglBindFragDataLocation(program->program, 4, "out_NormalDetail");
			}

			GLSL_BindAttributeLocations(program, attribs);*/

			program->vertexShader = program->fragmentShader = program->tessControlShader = program->tessEvaluationShader = program->geometryShader = 0;

			return true;
		}
	}
#endif //!__USE_GLSL_SHADER_CACHE__

	if (!GLSL_IsGPUShaderCompiled(program->vertexShader))
	{
		return false;
	}

	if (program->tessControlShader && !GLSL_IsGPUShaderCompiled(program->tessControlShader))
	{
		program->tessControlShader = 0;
		return false;
	}

	if (program->tessEvaluationShader && !GLSL_IsGPUShaderCompiled(program->tessEvaluationShader))
	{
		program->tessEvaluationShader = 0;
		return false;
	}

	if (program->geometryShader && !GLSL_IsGPUShaderCompiled(program->geometryShader))
	{
		program->geometryShader = 0;
		return false;
	}

	if (!GLSL_IsGPUShaderCompiled(program->fragmentShader))
	{
		return false;
	}

	qglAttachShader(program->program, program->vertexShader);
	qglAttachShader(program->program, program->fragmentShader);

	if (program->tessControlShader)
		qglAttachShader(program->program, program->tessControlShader);
	if (program->tessEvaluationShader)
		qglAttachShader(program->program, program->tessEvaluationShader);

	if (program->geometryShader)
		qglAttachShader(program->program, program->geometryShader);

	qglBindFragDataLocation(program->program, 0, "out_Color");
	qglBindFragDataLocation(program->program, 1, "out_Glow");
	qglBindFragDataLocation(program->program, 2, "out_Normal");
	qglBindFragDataLocation(program->program, 3, "out_Position");

	if (r_normalMappingReal->integer || program == &tr.linearizeDepthShader)
	{
		qglBindFragDataLocation(program->program, 4, "out_NormalDetail");
	}

	GLSL_BindAttributeLocations(program, attribs);

#ifdef __USE_GLSL_SHADER_CACHE__
	if (GLimp_HaveExtension("ARB_get_program_binary"))
	{
		qglProgramParameteri(program->program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
		GLSL_LinkProgram(program->program);
		GLSL_SaveShaderToCache(program);
	}
	else
	{
		GLSL_LinkProgram(program->program);
	}
#else //__USE_GLSL_SHADER_CACHE__
	GLSL_LinkProgram(program->program);
#endif //__USE_GLSL_SHADER_CACHE__

	// Won't be needing these anymore...
	qglDetachShader(program->program, program->vertexShader);
	qglDetachShader(program->program, program->fragmentShader);

	if (program->tessControlShader)
		qglDetachShader(program->program, program->tessControlShader);
	if (program->tessEvaluationShader)
		qglDetachShader(program->program, program->tessEvaluationShader);

	if (program->geometryShader)
		qglDetachShader(program->program, program->geometryShader);

	qglDeleteShader(program->vertexShader);
	qglDeleteShader(program->fragmentShader);

	if (program->tessControlShader)
		qglDeleteShader(program->tessControlShader);
	if (program->tessEvaluationShader)
		qglDeleteShader(program->tessEvaluationShader);

	if (program->geometryShader)
		qglDeleteShader(program->geometryShader);

	program->vertexShader = program->fragmentShader = program->tessControlShader = program->tessEvaluationShader = program->geometryShader = 0;

	return true;
}

#ifdef __GLSL_OPTIMIZER__
glslopt_ctx *ctx = NULL;

//#define __GLSL_OPTIMIZER_DEBUG__

void GLSL_PrintShaderOptimizationStats(char *shaderName, glslopt_shader *shader)
{
	if (!r_debugGLSLOptimizer->integer) return;

	static const char* kGlslTypeNames[kGlslTypeCount] = {
		"float",
		"int",
		"bool",
		"2d",
		"3d",
		"cube",
		"2dshadow",
		"2darray",
		"other",
	};
	static const char* kGlslPrecNames[kGlslPrecCount] = {
		"high",
		"medium",
		"low",
	};

#ifdef __GLSL_OPTIMIZER_DEBUG__
	ri->Printf(PRINT_WARNING, "\n==============================================================\n");
	ri->Printf(PRINT_WARNING, "Optimization Report - %s\n", shaderName);
	ri->Printf(PRINT_WARNING, "==============================================================\n");

	std::string textHir = glslopt_get_raw_output(shader);
	std::string textOpt = glslopt_get_output(shader);

	// append stats
	int statsAlu, statsTex, statsFlow;
	glslopt_shader_get_stats(shader, &statsAlu, &statsTex, &statsFlow);
	ri->Printf(PRINT_WARNING, "\n// stats: %i alu %i tex %i flow\n", statsAlu, statsTex, statsFlow);

	// append inputs
	const int inputCount = glslopt_shader_get_input_count(shader);
	if (inputCount > 0)
	{
		ri->Printf(PRINT_WARNING, "// inputs: %i\n", inputCount);
	}
	for (int i = 0; i < inputCount; ++i)
	{
		const char* parName;
		glslopt_basic_type parType;
		glslopt_precision parPrec;
		int parVecSize, parMatSize, parArrSize, location;
		glslopt_shader_get_input_desc(shader, i, &parName, &parType, &parPrec, &parVecSize, &parMatSize, &parArrSize, &location);
		if (location >= 0)
			ri->Printf(PRINT_WARNING, "//  #%i: %s (%s %s) %ix%i [%i] loc %i\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize, location);
		else
			ri->Printf(PRINT_WARNING, "//  #%i: %s (%s %s) %ix%i [%i]\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize);
	}
	// append uniforms
	const int uniformCount = glslopt_shader_get_uniform_count(shader);
	const int uniformSize = glslopt_shader_get_uniform_total_size(shader);
	if (uniformCount > 0)
	{
		ri->Printf(PRINT_WARNING, "// uniforms: %i (total size: %i)\n", uniformCount, uniformSize);
	}
	for (int i = 0; i < uniformCount; ++i)
	{
		const char* parName;
		glslopt_basic_type parType;
		glslopt_precision parPrec;
		int parVecSize, parMatSize, parArrSize, location;
		glslopt_shader_get_uniform_desc(shader, i, &parName, &parType, &parPrec, &parVecSize, &parMatSize, &parArrSize, &location);
		if (location >= 0)
			ri->Printf(PRINT_WARNING, "//  #%i: %s (%s %s) %ix%i [%i] loc %i\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize, location);
		else
			ri->Printf(PRINT_WARNING, "//  #%i: %s (%s %s) %ix%i [%i]\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize);
	}
	// append textures
	const int textureCount = glslopt_shader_get_texture_count(shader);
	if (textureCount > 0)
	{
		ri->Printf(PRINT_WARNING, "// textures: %i\n", textureCount);
	}
	for (int i = 0; i < textureCount; ++i)
	{
		const char* parName;
		glslopt_basic_type parType;
		glslopt_precision parPrec;
		int parVecSize, parMatSize, parArrSize, location;
		glslopt_shader_get_texture_desc(shader, i, &parName, &parType, &parPrec, &parVecSize, &parMatSize, &parArrSize, &location);
		if (location >= 0)
			ri->Printf(PRINT_WARNING, "//  #%i: %s (%s %s) %ix%i [%i] loc %i\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize, location);
		else
			ri->Printf(PRINT_WARNING, "//  #%i: %s (%s %s) %ix%i [%i]\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize);
	}
	ri->Printf(PRINT_WARNING, "==============================================================\n");
#else //!__GLSL_OPTIMIZER_DEBUG__
	std::string textHir = glslopt_get_raw_output(shader);
	std::string textOpt = glslopt_get_output(shader);

	// append stats
	int statsAlu, statsTex, statsFlow;
	glslopt_shader_get_stats(shader, &statsAlu, &statsTex, &statsFlow);

	// append inputs
	const int inputCount = glslopt_shader_get_input_count(shader);

	// append uniforms
	const int uniformCount = glslopt_shader_get_uniform_count(shader);
	const int uniformSize = glslopt_shader_get_uniform_total_size(shader);

	// append textures
	const int textureCount = glslopt_shader_get_texture_count(shader);

	ri->Printf(PRINT_WARNING, "^1*** ^3GLSL-OPTIMIZATION^5: GlslOpt ^3%s^5 - ^7%i^5 alu. ^7%i^5 tex. ^7%i^5 flw. ^7%i^5 inp. ^7%i^5 unif (tsize: ^7%i^5). ^7%i^5 texc.\n", shaderName, statsAlu, statsTex, statsFlow, inputCount, uniformCount, uniformSize, textureCount);
#endif //__GLSL_OPTIMIZER_DEBUG__
}
#endif //__GLSL_OPTIMIZER__

int shaders_next_id;
shaderProgram_t *shaders[256];

int GLSL_BeginLoadGPUShader(shaderProgram_t * program, const char *name,
	int attribs, qboolean fragmentShader, qboolean tesselation, qboolean geometry, qboolean isBindless, const GLcharARB *extra, qboolean addHeader,
	char *forceVersion, const char *fallback_vp, const char *fallback_fp, const char *fallback_cp, const char *fallback_ep, const char *fallback_gs)
{
	qboolean bindless = qfalse;

	if (shaders_next_id + 1 < 256)
	{
		shaders[shaders_next_id++] = program; // register the shader, so we can access them all later
	}

	if (isBindless && glRefConfig.bindlessTextures)
	{// Only enable if the extensions required exist on the GPU...
		bindless = qtrue;

		ri->Printf(PRINT_WARNING, "GLSL %s is running bindless.\n", name && name[0] ? name : "unnamed");
	}

#ifdef __USE_GLSL_SHADER_CACHE__
	int progId = GLSL_LoadShaderFromCache(program, name);

	if (progId != -1)
	{// Loaded ok from cache...
		program->attribs = attribs;

		program->tesselation = qfalse;
		program->tessControlShader = NULL;
		program->tessEvaluationShader = NULL;

		program->geometry = qfalse;
		program->geometryShader = NULL;

		program->isBindless = bindless;

		if (tesselation)
		{
			program->tesselation = qtrue;
		}

		if (geometry)
		{
			program->geometry = qtrue;
		}

		return 1;
	}
#endif //__USE_GLSL_SHADER_CACHE__

	std::string vpCode;
	std::string fpCode;
	std::string cpCode;
	std::string epCode;
	std::string gsCode;

#ifdef __DEBUG_SHADER_LOAD__
	ri->Printf(PRINT_WARNING, "Begin GLSL load for %s.\n", name);
#endif //__DEBUG_SHADER_LOAD__

	if (addHeader)
	{
		GLSL_GetShaderHeader(GL_VERTEX_SHADER, extra, vpCode, forceVersion, bindless);
		vpCode += fallback_vp;
	}
	else
	{
		vpCode = fallback_vp;
	}

	//ri->Printf(PRINT_WARNING, "[%s]\n%s\n", name, vpCode.c_str());

	if (tesselation && fallback_cp && fallback_cp[0])
	{
		if (addHeader)
		{
			GLSL_GetShaderHeader(GL_TESS_CONTROL_SHADER, extra, cpCode, forceVersion, bindless);
			cpCode += fallback_cp;
		}
		else
		{
			cpCode = fallback_cp;
		}
	}

	if (tesselation && fallback_ep && fallback_ep[0])
	{
		if (addHeader)
		{
			GLSL_GetShaderHeader(GL_TESS_EVALUATION_SHADER, extra, epCode, forceVersion, bindless);
			epCode += fallback_ep;
		}
		else
		{
			epCode = fallback_ep;
		}
	}

	if (geometry && fallback_gs && fallback_gs[0])
	{
		if (addHeader)
		{
			GLSL_GetShaderHeader(GL_GEOMETRY_SHADER, extra, gsCode, forceVersion, bindless);
			gsCode += fallback_gs;
		}
		else
		{
			gsCode = fallback_gs;
		}
	}

	if (fragmentShader)
	{
		if (addHeader)
		{
			GLSL_GetShaderHeader(GL_FRAGMENT_SHADER, extra, fpCode, forceVersion, bindless);
			fpCode += fallback_fp;
		}
		else
		{
			fpCode = fallback_fp;
		}
	}

#ifdef __GLSL_OPTIMIZER__
	//
	// Shader optimization only supports vert and frag shaders...
	//
	if (r_glslOptimize->integer)
	{
		try {
			if (!StringContainsWord(name, "lightall")
				&& !StringContainsWord(name, "depthPass")
				&& !StringContainsWord(name, "sky")
				&& !StringContainsWord(name, "fxaa")
				&& !StringContainsWord(name, "instances")
				&& !StringContainsWord(name, "depthAdjust"))
			{// The optimizer doesn't like lightAll and depthPass vert shaders...
				if (vpCode.length())
				{
					glslopt_shader *sh = glslopt_optimize(ctx, kGlslOptShaderVertex, vpCode.c_str(), 0);
					if (glslopt_get_status(sh)) {
						const char *newSource = glslopt_get_output(sh);
						vpCode.clear();
						vpCode = newSource;
						GLSL_PrintShaderOptimizationStats(va("%s (vert)", name), sh);
					}
					else {
						const char *errorLog = glslopt_get_log(sh);
						if (r_debugGLSLOptimizer->integer) ri->Printf(PRINT_WARNING, "GLSL optimization failed on vert shader %s.\n\nLOG:\n%s\n", name, errorLog);
					}
					glslopt_shader_delete(sh);
				}
			}

			if (!StringContainsWord(name, "deferredLighting") // The optimizer doesn't like deferredLighting shader...
				&& !StringContainsWord(name, "fxaa") // NV fxaa is not a fan of optimization
				&& !StringContainsWord(name, "sky") // too big for optimizer i thinks...
				&& !StringContainsWord(name, "instances")
				&& !StringContainsWord(name, "depthAdjust"))
			{
				if (fpCode.length())
				{
					glslopt_shader *sh = glslopt_optimize(ctx, kGlslOptShaderFragment, fpCode.c_str(), 0);
					if (glslopt_get_status(sh)) {
						const char *newSource = glslopt_get_output(sh);
#ifdef __GLSL_OPTIMIZER_DEBUG__
						ri->Printf(PRINT_WARNING, "strlen %i\n", strlen(newSource));
						ri->Printf(PRINT_WARNING, "%s\n", newSource);
#endif //__GLSL_OPTIMIZER_DEBUG__
						fpCode.clear();
						fpCode = newSource;
						GLSL_PrintShaderOptimizationStats(va("%s (frag)", name), sh);
					}
					else {
						const char *errorLog = glslopt_get_log(sh);
						if (r_debugGLSLOptimizer->integer) ri->Printf(PRINT_WARNING, "GLSL optimization failed on frag shader %s.\n\nLOG:\n%s\n", name, errorLog);
					}
					glslopt_shader_delete(sh);
				}
			}
		}
		catch (...) {

		}
	}
#endif //__GLSL_OPTIMIZER__
	
	if (tesselation && cpCode.length())
	{
		if (geometry && gsCode.length())
			return GLSL_BeginLoadGPUShader2(program, name, attribs, vpCode, fragmentShader ? fpCode : "", cpCode, epCode, gsCode, bindless);
		else
			return GLSL_BeginLoadGPUShader2(program, name, attribs, vpCode, fragmentShader ? fpCode : "", cpCode, epCode, "", bindless);
	}
	else if (geometry && gsCode.length())
	{
		return GLSL_BeginLoadGPUShader2(program, name, attribs, vpCode, fragmentShader ? fpCode : "", "", "", gsCode.c_str(), bindless);
	}
	else
	{
		return GLSL_BeginLoadGPUShader2(program, name, attribs, vpCode, fragmentShader ? fpCode : "", "", "", "", bindless);
	}
}

void GLSL_InitUniforms(shaderProgram_t *program)
{
	int i, size;

	GLint *uniforms;

	program->uniforms = (GLint *)Z_Malloc(UNIFORM_COUNT * sizeof (*program->uniforms), TAG_GENERAL);
	program->uniformBufferOffsets = (short *)Z_Malloc(UNIFORM_COUNT * sizeof (*program->uniformBufferOffsets), TAG_GENERAL);

	uniforms = program->uniforms;

	size = 0;
	for (i = 0; i < UNIFORM_COUNT; i++)
	{
		uniforms[i] = qglGetUniformLocation(program->program, uniformsInfo[i].name);

		if (uniforms[i] == -1)
			continue;

		program->uniformBufferOffsets[i] = size;

		switch (uniformsInfo[i].type)
		{
		case GLSL_INT:
			size += sizeof(GLint) * uniformsInfo[i].size;
			break;
		case GLSL_FLOAT:
			size += sizeof(GLfloat) * uniformsInfo[i].size;
			break;
		case GLSL_FLOAT5:
			size += sizeof(float) * 5 * uniformsInfo[i].size;
			break;
		case GLSL_FLOAT7:
			size += sizeof(float) * 7 * uniformsInfo[i].size;
			break;
		case GLSL_VEC2:
			size += sizeof(float) * 2 * uniformsInfo[i].size;
			break;
		case GLSL_VEC3:
			size += sizeof(float) * 3 * uniformsInfo[i].size;
			break;
		case GLSL_VEC4:
			size += sizeof(float) * 4 * uniformsInfo[i].size;
			break;
		case GLSL_MAT16:
			size += sizeof(float) * 16 * uniformsInfo[i].size;
			break;
		default:
			break;
		}
	}

	program->uniformBuffer = (char *)Z_Malloc(size, TAG_SHADERTEXT, qtrue);
}

void GLSL_FinishGPUShader(shaderProgram_t *program)
{
#if defined(__GLSL_OPTIMIZER__) && defined(__GLSL_OPTIMIZER_DEBUG__)
	ri->Printf(PRINT_WARNING, "Finish GPU shader: %s.\n", program->name);
#endif //defined(__GLSL_OPTIMIZER__) && defined(__GLSL_OPTIMIZER_DEBUG__)
	GLSL_ValidateProgram(program->program);
	GLSL_ShowProgramUniforms(program);
	GL_CheckErrors();
}

qboolean GLSL_CompareFloatBuffers(const float *buffer1, float *buffer2, int numElements)
{
	for (int i = 0; i < numElements; i++)
	{
		if (buffer1[i] != buffer2[i])
		{
			return qfalse;
		}
	}

	return qtrue;
}

qboolean GLSL_CompareIntBuffers(const int *buffer1, int *buffer2, int numElements)
{
	for (int i = 0; i < numElements; i++)
	{
		if (buffer1[i] != buffer2[i])
		{
			return qfalse;
		}
	}

	return qtrue;
}

//#define __BINDLESS_OFFSETS__

GLuint CURRENT_BINDING_POINT = 1;

void GLSL_BindlessInitialize(shaderProgram_t *program)
{
	if (!program->isBindless)
	{
		return;
	}

	if (program->bindLessBindingPoint <= 0)
	{// Create a new buffer, if we have not yet...
		GLuint index = qglGetUniformBlockIndex(program->program, "u_bindlessTexturesBlock");

		program->bindLessBindingPoint = CURRENT_BINDING_POINT;
		CURRENT_BINDING_POINT++;

		qglUniformBlockBinding(program->program, index, program->bindLessBindingPoint);

		qglGenBuffers(1, &program->bindlessBlockUBO);
		qglBindBuffer(GL_UNIFORM_BUFFER, program->bindlessBlockUBO);
		qglBufferData(GL_UNIFORM_BUFFER, sizeof(program->bindlessBlock), NULL, GL_DYNAMIC_DRAW/*GL_STREAM_DRAW*/);
		qglBindBufferBase(GL_UNIFORM_BUFFER, program->bindLessBindingPoint, program->bindlessBlockUBO);
		qglBindBuffer(GL_UNIFORM_BUFFER, 0);

		qglBindBuffer(GL_UNIFORM_BUFFER, program->bindlessBlockUBO);
		qglBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(program->bindlessBlock), &program->bindlessBlock);
		qglBindBuffer(GL_UNIFORM_BUFFER, 0);

		qglBindBufferRange(GL_UNIFORM_BUFFER, index, program->bindlessBlockUBO, 0, sizeof(program->bindlessBlock));
	}
}

void GLSL_BindlessUpdate(shaderProgram_t *program)
{
	if (!program->isBindless)
	{
		return;
	}

	GLSL_BindlessInitialize(program);

#ifndef __BINDLESS_OFFSETS__
	if (!memcmp(&program->bindlessBlock, &program->bindlessBlockPrevious, sizeof(program->bindlessBlock)))
	{// Has not changed...
		return;
	}

	qglBindBuffer(GL_UNIFORM_BUFFER, program->bindlessBlockUBO);
	qglBufferSubData(GL_UNIFORM_BUFFER, 0 /* TODO: offsets */, sizeof(program->bindlessBlock), &program->bindlessBlock);
	qglBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Update our previous copy buffer so we can skip updates...
	memcpy(&program->bindlessBlockPrevious, &program->bindlessBlock, sizeof(program->bindlessBlock));
#else //!__BINDLESS_OFFSETS__
	bindlessTexturesBlock_t *block = &program->bindlessBlock;
	bindlessTexturesBlock_t *blockPrevious = &program->bindlessBlockPrevious;

	qglBindBuffer(GL_UNIFORM_BUFFER, program->bindlessBlockUBO);

	for (int uniformNum = UNIFORM_DIFFUSEMAP; uniformNum <= UNIFORM_MOONMAPS; uniformNum++)
	{
		size_t offset = 0;

		switch (uniformNum)
		{
		case UNIFORM_DIFFUSEMAP:
			if (block->u_DiffuseMap == blockPrevious->u_DiffuseMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_DiffuseMap);
			break;
		case UNIFORM_LIGHTMAP:
			if (block->u_LightMap == blockPrevious->u_LightMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_LightMap);
			break;
		case UNIFORM_NORMALMAP:
			if (block->u_NormalMap == blockPrevious->u_NormalMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_NormalMap);
			break;
		case UNIFORM_DELUXEMAP:
			if (block->u_DeluxeMap == blockPrevious->u_DeluxeMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_DeluxeMap);
			break;
		case UNIFORM_SPECULARMAP:
			if (block->u_SpecularMap == blockPrevious->u_SpecularMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SpecularMap);
			break;
		case UNIFORM_POSITIONMAP:
			if (block->u_PositionMap == blockPrevious->u_PositionMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_PositionMap);
			break;
		case UNIFORM_WATERPOSITIONMAP:
			if (block->u_WaterPositionMap == blockPrevious->u_WaterPositionMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_WaterPositionMap);
			break;
		case UNIFORM_WATERHEIGHTMAP:
			if (block->u_WaterHeightMap == blockPrevious->u_WaterHeightMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_WaterHeightMap);
			break;
		case UNIFORM_HEIGHTMAP:
			if (block->u_HeightMap == blockPrevious->u_HeightMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_HeightMap);
			break;
		case UNIFORM_GLOWMAP:
			if (block->u_GlowMap == blockPrevious->u_GlowMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_GlowMap);
			break;
		case UNIFORM_ENVMAP:
			if (block->u_EnvironmentMap == blockPrevious->u_EnvironmentMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_EnvironmentMap);
			break;
		case UNIFORM_TEXTUREMAP:
			if (block->u_TextureMap == blockPrevious->u_TextureMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_TextureMap);
			break;
		case UNIFORM_LEVELSMAP:
			if (block->u_LevelsMap == blockPrevious->u_LevelsMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_LevelsMap);
			break;
		case UNIFORM_CUBEMAP:
			if (block->u_CubeMap == blockPrevious->u_CubeMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_CubeMap);
			break;
		case UNIFORM_SKYCUBEMAP:
			if (block->u_SkyCubeMap == blockPrevious->u_SkyCubeMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SkyCubeMap);
			break;
		case UNIFORM_SKYCUBEMAPNIGHT:
			if (block->u_SkyCubeMapNight == blockPrevious->u_SkyCubeMapNight)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SkyCubeMapNight);
			break;
		case UNIFORM_EMISSIVECUBE:
			if (block->u_EmissiveCubeMap == blockPrevious->u_EmissiveCubeMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_EmissiveCubeMap);
			break;
		case UNIFORM_OVERLAYMAP:
			if (block->u_OverlayMap == blockPrevious->u_OverlayMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_OverlayMap);
			break;
		case UNIFORM_STEEPMAP:
			if (block->u_SteepMap == blockPrevious->u_SteepMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SteepMap);
			break;
		case UNIFORM_STEEPMAP1:
			if (block->u_SteepMap1 == blockPrevious->u_SteepMap1)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SteepMap1);
			break;
		case UNIFORM_STEEPMAP2:
			if (block->u_SteepMap2 == blockPrevious->u_SteepMap2)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SteepMap2);
			break;
		case UNIFORM_STEEPMAP3:
			if (block->u_SteepMap3 == blockPrevious->u_SteepMap3)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SteepMap3);
			break;
		case UNIFORM_WATER_EDGE_MAP:
			if (block->u_WaterEdgeMap == blockPrevious->u_WaterEdgeMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_WaterEdgeMap);
			break;
		case UNIFORM_SPLATCONTROLMAP:
			if (block->u_SplatControlMap == blockPrevious->u_SplatControlMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SplatControlMap);
			break;
		case UNIFORM_SPLATMAP1:
			if (block->u_SplatMap1 == blockPrevious->u_SplatMap1)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SplatMap1);
			break;
		case UNIFORM_SPLATMAP2:
			if (block->u_SplatMap2 == blockPrevious->u_SplatMap2)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SplatMap2);
			break;
		case UNIFORM_SPLATMAP3:
			if (block->u_SplatMap3 == blockPrevious->u_SplatMap3)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_SplatMap3);
			break;
		case UNIFORM_ROADSCONTROLMAP:
			if (block->u_RoadsControlMap == blockPrevious->u_RoadsControlMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_RoadsControlMap);
			break;
		case UNIFORM_ROADMAP:
			if (block->u_RoadMap == blockPrevious->u_RoadMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_RoadMap);
			break;
		case UNIFORM_DETAILMAP:
			if (block->u_DetailMap == blockPrevious->u_DetailMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_DetailMap);
			break;
		case UNIFORM_SCREENIMAGEMAP:
			if (block->u_ScreenImageMap == blockPrevious->u_ScreenImageMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_ScreenImageMap);
			break;
		case UNIFORM_SCREENDEPTHMAP:
			if (block->u_ScreenDepthMap == blockPrevious->u_ScreenDepthMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_ScreenDepthMap);
			break;
		case UNIFORM_SHADOWMAP:
			if (block->u_ShadowMap == blockPrevious->u_ShadowMap)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_ShadowMap);
			break;
		case UNIFORM_SHADOWMAP2:
			if (block->u_ShadowMap2 == blockPrevious->u_ShadowMap2)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_ShadowMap2);
			break;
		case UNIFORM_SHADOWMAP3:
			if (block->u_ShadowMap3 == blockPrevious->u_ShadowMap3)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_ShadowMap3);
			break;
		case UNIFORM_SHADOWMAP4:
			if (block->u_ShadowMap4 == blockPrevious->u_ShadowMap4)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_ShadowMap4);
			break;
		case UNIFORM_SHADOWMAP5:
			if (block->u_ShadowMap5 == blockPrevious->u_ShadowMap5)
				continue;

			offset = offsetof(bindlessTexturesBlock_t, u_ShadowMap5);
			break;
		case UNIFORM_MOONMAPS:
			{
				if (block->u_MoonMaps[0] != blockPrevious->u_MoonMaps[0]
					&& block->u_MoonMaps[1] == blockPrevious->u_MoonMaps[1]
					&& block->u_MoonMaps[2] == blockPrevious->u_MoonMaps[2]
					&& block->u_MoonMaps[3] == blockPrevious->u_MoonMaps[3])
					continue;

				offset = offsetof(bindlessTexturesBlock_t, u_MoonMaps);

				qglBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(GLuint64) * 4, &program->bindlessBlock);
				memcpy((&program->bindlessBlockPrevious) + offset, (&program->bindlessBlock) + offset, sizeof(GLuint64) * 4);

				continue;
			}
			break;
		default:
			continue;
		}

		qglBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(GLuint64), &program->bindlessBlock);
		memcpy((&program->bindlessBlockPrevious) + offset, (&program->bindlessBlock) + offset, sizeof(GLuint64));
	}

	qglBindBuffer(GL_UNIFORM_BUFFER, 0);
#endif //__BINDLESS_OFFSETS__
}

void GLSL_SetBindlessTexture(shaderProgram_t *program, int uniformNum, image_t **images, int arrayID)
{
	if (program->isBindless && uniformNum >= UNIFORM_DIFFUSEMAP && uniformNum <= UNIFORM_MOONMAPS)
	{// When using bindless textures, we simply set the appropriate array value...
		if (!images || !images[0])
		{
			ri->Printf(PRINT_WARNING, "GLSL_SetBindlessTexture: Fix ya code doofus, you set a NULL texture for uniform %i.\n", uniformNum);
			return;
		}

		/*shaderProgram_t *origProgram = NULL;

		if (glState.currentProgram != program)
		{
			origProgram = glState.currentProgram;

			GLSL_BindProgram(program);
		}*/

		GLSL_BindlessInitialize(program);

		bindlessTexturesBlock_t *block = &program->bindlessBlock;

		GLuint64 bindlessHandle = images[0]->bindlessHandle;
		
		/*ri->Printf(PRINT_WARNING, "SetBindlessTexture: Program %s uniform %i is binding %s (handle %u).\n"
			, (program && program->name && program->name[0] != 0) ? program->name : "unknown program"
			, uniformNum
			, (images[0] && images[0]->imgName && images[0]->imgName[0] != 0) ? images[0]->imgName : "unknown image"
			, bindlessHandle);*/

		switch (uniformNum)
		{
		case UNIFORM_DIFFUSEMAP:
			block->u_DiffuseMap = bindlessHandle;
			break;
		case UNIFORM_LIGHTMAP:
			block->u_LightMap = bindlessHandle;
			break;
		case UNIFORM_NORMALMAP:
			block->u_NormalMap = bindlessHandle;
			break;
		case UNIFORM_DELUXEMAP:
			block->u_DeluxeMap = bindlessHandle;
			break;
		case UNIFORM_SPECULARMAP:
			block->u_SpecularMap = bindlessHandle;
			break;
		case UNIFORM_POSITIONMAP:
			block->u_PositionMap = bindlessHandle;
			break;
		case UNIFORM_WATERPOSITIONMAP:
			block->u_WaterPositionMap = bindlessHandle;
			break;
		case UNIFORM_WATERHEIGHTMAP:
			block->u_WaterHeightMap = bindlessHandle;
			break;
		case UNIFORM_HEIGHTMAP:
			block->u_HeightMap = bindlessHandle;
			break;
		case UNIFORM_GLOWMAP:
			block->u_GlowMap = bindlessHandle;
			break;
		case UNIFORM_ENVMAP:
			block->u_EnvironmentMap = bindlessHandle;
			break;
		case UNIFORM_TEXTUREMAP:
			block->u_TextureMap = bindlessHandle;
			break;
		case UNIFORM_LEVELSMAP:
			block->u_LevelsMap = bindlessHandle;
			break;
		case UNIFORM_CUBEMAP:
			block->u_CubeMap = bindlessHandle;
			break;
		case UNIFORM_SKYCUBEMAP:
			block->u_SkyCubeMap = bindlessHandle;
			break;
		case UNIFORM_SKYCUBEMAPNIGHT:
			block->u_SkyCubeMapNight = bindlessHandle;
			break;
		case UNIFORM_EMISSIVECUBE:
			block->u_EmissiveCubeMap = bindlessHandle;
			break;
		case UNIFORM_OVERLAYMAP:
			block->u_OverlayMap = bindlessHandle;
			break;
		case UNIFORM_STEEPMAP:
			block->u_SteepMap = bindlessHandle;
			break;
		case UNIFORM_STEEPMAP1:
			block->u_SteepMap1 = bindlessHandle;
			break;
		case UNIFORM_STEEPMAP2:
			block->u_SteepMap2 = bindlessHandle;
			break;
		case UNIFORM_STEEPMAP3:
			block->u_SteepMap3 = bindlessHandle;
			break;
		case UNIFORM_WATER_EDGE_MAP:
			block->u_WaterEdgeMap = bindlessHandle;
			break;
		case UNIFORM_SPLATCONTROLMAP:
			block->u_SplatControlMap = bindlessHandle;
			break;
		case UNIFORM_SPLATMAP1:
			block->u_SplatMap1 = bindlessHandle;
			break;
		case UNIFORM_SPLATMAP2:
			block->u_SplatMap2 = bindlessHandle;
			break;
		case UNIFORM_SPLATMAP3:
			block->u_SplatMap3 = bindlessHandle;
			break;
		case UNIFORM_ROADSCONTROLMAP:
			block->u_RoadsControlMap = bindlessHandle;
			break;
		case UNIFORM_ROADMAP:
			block->u_RoadMap = bindlessHandle;
			break;
		case UNIFORM_DETAILMAP:
			block->u_DetailMap = bindlessHandle;
			break;
		case UNIFORM_SCREENIMAGEMAP:
			block->u_ScreenImageMap = bindlessHandle;
			break;
		case UNIFORM_SCREENDEPTHMAP:
			block->u_ScreenDepthMap = bindlessHandle;
			break;
		case UNIFORM_SHADOWMAP:
			block->u_ShadowMap = bindlessHandle;
			break;
		case UNIFORM_SHADOWMAP2:
			block->u_ShadowMap2 = bindlessHandle;
			break;
		case UNIFORM_SHADOWMAP3:
			block->u_ShadowMap3 = bindlessHandle;
			break;
		case UNIFORM_SHADOWMAP4:
			block->u_ShadowMap4 = bindlessHandle;
			break;
		case UNIFORM_SHADOWMAP5:
			block->u_ShadowMap5 = bindlessHandle;
			break;
		case UNIFORM_MOONMAPS:
			block->u_MoonMaps[arrayID] = bindlessHandle;
			break;
		}

#ifdef __BINDLESS_OFFSETS__
		GLSL_BindlessUpdate(program);
#endif //__BINDLESS_OFFSETS__

		/*if (origProgram)
		{
			GLSL_BindProgram(origProgram);
		}*/
	}
}

void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value)
{
	if (program->isBindless && uniformNum >= UNIFORM_DIFFUSEMAP && uniformNum <= UNIFORM_MOONMAPS)
	{// When using bindless textures, skip this...
		return;
	}

	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	GLint *compare = (GLint *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniformsInfo[uniformNum].type != GLSL_INT)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	qglUniform1i(uniforms[uniformNum], value);
}

void GLSL_SetUniformIntxX(shaderProgram_t *program, int uniformNum, const int *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	GLint *compare = (GLint *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniformsInfo[uniformNum].type != GLSL_INT)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (int *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareIntBuffers((const int *)elements, compare, numElements)) return;

	Com_Memcpy(compare, elements, sizeof(int)* numElements);

	qglUniform1iv(uniforms[uniformNum], numElements, (const GLint *)elements);
}

void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	GLfloat *compare = (GLfloat *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}
	
	*compare = value;

	qglUniform1f(uniforms[uniformNum], value);
}

void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniformsInfo[uniformNum].type != GLSL_VEC2)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (GLSL_CompareFloatBuffers(v, compare, 2)) return;

	compare[0] = v[0];
	compare[1] = v[1];

	qglUniform2f(uniforms[uniformNum], v[0], v[1]);
}

void GLSL_SetUniformVec2x16(shaderProgram_t *program, int uniformNum, const vec2_t *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_VEC2)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec2x16: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)elements, compare, numElements*2)) return;

	Com_Memcpy(compare, elements, sizeof (vec2_t)* numElements);

	qglUniform2fv(uniforms[uniformNum], numElements, (const GLfloat *)elements);
}

void GLSL_SetUniformVec2xX(shaderProgram_t *program, int uniformNum, const vec2_t *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_VEC2)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec2xX: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)elements, compare, numElements * 2)) return;

	Com_Memcpy(compare, elements, sizeof(vec2_t)* numElements);

	qglUniform2fv(uniforms[uniformNum], numElements, (const GLfloat *)elements);
}

void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_VEC3)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	float *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)v, compare, 3)) return;

	VectorCopy(v, compare);

	qglUniform3f(uniforms[uniformNum], v[0], v[1], v[2]);
}

void GLSL_SetUniformVec3xX(shaderProgram_t *program, int uniformNum, const vec3_t *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_VEC3)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec3xX: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)elements, compare, numElements * 3)) return;

	Com_Memcpy(compare, elements, sizeof (vec3_t)* numElements);

	qglUniform3fv(uniforms[uniformNum], numElements, (const GLfloat *)elements);
}

void GLSL_SetUniformVec3x64(shaderProgram_t *program, int uniformNum, const vec3_t *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_VEC3)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec3x64: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)elements, compare, numElements * 3)) return;

	Com_Memcpy(compare, elements, sizeof (vec3_t)* numElements);

	qglUniform3fv(uniforms[uniformNum], numElements, (const GLfloat *)elements);
}

void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_VEC4)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	float *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)v, compare, 4)) return;

	VectorCopy4(v, compare);

	qglUniform4f(uniforms[uniformNum], v[0], v[1], v[2], v[3]);
}

void GLSL_SetUniformVec4xX(shaderProgram_t *program, int uniformNum, const vec4_t *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_VEC4)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformVec4xX: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)elements, compare, numElements * 4)) return;

	Com_Memcpy(compare, elements, sizeof(vec4_t)* numElements);

	qglUniform4fv(uniforms[uniformNum], numElements, (const GLfloat *)elements);
}

void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT5)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformFloat5: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	float *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)v, compare, 5)) return;

	VectorCopy5(v, compare);

	qglUniform1fv(uniforms[uniformNum], 5, v);
}

void GLSL_SetUniformFloat7(shaderProgram_t *program, int uniformNum, const float *v)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT7)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformFloat7: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	float *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)v, compare, 7)) return;

	VectorCopy7(v, compare);

	qglUniform1fv(uniforms[uniformNum], 7, v);
}

void GLSL_SetUniformFloatxX(shaderProgram_t *program, int uniformNum, const float *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformFloatxX: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)elements, compare, numElements)) return;

	Com_Memcpy(compare, elements, sizeof (float)* numElements);

	qglUniform1fv(uniforms[uniformNum], numElements, (const GLfloat *)elements);
}

void GLSL_SetUniformFloatx64(shaderProgram_t *program, int uniformNum, const float *elements, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformFloatx64: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)elements, compare, numElements)) return;

	Com_Memcpy(compare, elements, sizeof (float)* numElements);

	qglUniform1fv(uniforms[uniformNum], numElements, (const GLfloat *)elements);
}

void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const float *matrix, int numElements)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
		return;

	float *compare;

	if (uniformsInfo[uniformNum].type != GLSL_MAT16)
	{
		ri->Printf(PRINT_WARNING, "GLSL_SetUniformMatrix16: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (uniformsInfo[uniformNum].size < numElements)
		return;

	compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (GLSL_CompareFloatBuffers((const float *)matrix, compare, numElements * 16)) return;

	matrix_t currentShaderUniform;
	qglGetUniformfv(program->program, uniforms[uniformNum], currentShaderUniform);

	Com_Memcpy(compare, matrix, sizeof (float)* 16 * numElements);

	qglUniformMatrix4fv(uniforms[uniformNum], numElements, GL_FALSE, matrix);
}

void GLSL_DeleteGPUShader(shaderProgram_t *program)
{
	if (program->program)
	{
		if (program->bindlessBlockUBO)
		{
			qglDeleteBuffers(1, &program->bindlessBlockUBO);
			program->bindlessBlockUBO = 0;
		}

#ifdef __LIGHTS_UBO__
		if (program->LightsBlockUBO)
		{
			qglDeleteBuffers(1, &program->LightsBlockUBO);
			program->LightsBlockUBO = 0;
		}
#endif //__LIGHTS_UBO__

		qglDeleteProgram(program->program);

		program->name.clear();
		Z_Free(program->uniformBuffer);
		Z_Free(program->uniformBufferOffsets);
		Z_Free(program->uniforms);

		Com_Memset(program, 0, sizeof(*program));
	}
}

int GLSL_BeginLoadGPUShaders(void)
{
	int startTime;
	int i;
	char extradefines[1024] = { { 0 } };
	int attribs;

	ri->Printf(PRINT_ALL, "^5------- ^7GLSL_InitGPUShaders^5 -------\n");

	R_IssuePendingRenderCommands();

	startTime = ri->Milliseconds();

#ifdef __GLSL_OPTIMIZER__
	if (r_glslOptimize->integer)
	{
		ctx = glslopt_initialize(kGlslTargetOpenGL);
	}
#endif //__GLSL_OPTIMIZER__


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;

	if (!GLSL_BeginLoadGPUShader(&tr.textureColorShader, "texturecolor", attribs, qtrue, qfalse, qfalse, qtrue, NULL, qtrue, NULL, fallbackShader_texturecolor_vp, fallbackShader_texturecolor_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load texturecolor shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;// | ATTR_COLOR | ATTR_NORMAL;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.weatherShader, "weather", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_weather_vp, fallbackShader_weather_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load weather shader!");
	}

#ifdef __INSTANCED_MODELS__
	attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD0 /*| ATTR_INSTANCES_MVP | ATTR_INSTANCES_POSITION*/;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.instanceShader, "instance", attribs, qtrue, qfalse, qfalse, qfalse, NULL, qtrue, NULL, fallbackShader_instance_vp, fallbackShader_instance_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load instance shader!");
	}
#endif //__INSTANCED_MODELS__

#ifdef __LODMODEL_INSTANCING__
	attribs = ATTR_INSTANCES_POSITION | ATTR_NORMAL | ATTR_TEXCOORD0 /*| ATTR_INSTANCES_MVP | ATTR_INSTANCES_POSITION*/;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.instanceVAOShader, "instanceVao", attribs, qtrue, qfalse, qfalse, qfalse, NULL, qtrue, NULL, fallbackShader_instanceVao_vp, fallbackShader_instanceVao_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load instanceVao shader!");
	}
#endif //__LODMODEL_INSTANCING__

	attribs = ATTR_POSITION;// | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.occlusionShader, "occlusion", attribs, qtrue, qfalse, qfalse, qtrue, NULL, qtrue, NULL, fallbackShader_occlusion_vp, fallbackShader_occlusion_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load occlusion shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.depthAdjustShader, "depthAdjust", attribs, qtrue, qfalse, qfalse, qtrue, NULL, qtrue, NULL, fallbackShader_depthAdjust_vp, fallbackShader_depthAdjust_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load depthAdjust shader!");
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		if (r_deluxeSpecular->value > 0.000001f)
			strcat(extradefines, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

		if (r_hdr->integer && !glRefConfig.floatLightmap)
			strcat(extradefines, "#define RGBM_LIGHTMAP\n");

		strcat(extradefines, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

		if (r_deluxeMapping->integer)
			strcat(extradefines, "#define USE_DELUXEMAP\n");

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.lightAllShader[0], "lightall0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_lightall_vp, fallbackShader_lightall_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load lightall0 shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		if (r_deluxeSpecular->value > 0.000001f)
			strcat(extradefines, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

		if (r_hdr->integer && !glRefConfig.floatLightmap)
			strcat(extradefines, "#define RGBM_LIGHTMAP\n");

		strcat(extradefines, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

		if (r_deluxeMapping->integer)
			strcat(extradefines, "#define USE_DELUXEMAP\n");

		strcat(extradefines, "#define USE_TESSELLATION\n");

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.lightAllShader[1], "lightall1", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_lightall_vp, fallbackShader_lightall_fp, fallbackShader_tessellation_cs, fallbackShader_tessellation_es, NULL/*fallbackShader_tessellation_gs*/))
		{
			ri->Error(ERR_FATAL, "Could not load lightall1 (tess) shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		if (r_deluxeSpecular->value > 0.000001f)
			strcat(extradefines, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

		if (r_hdr->integer && !glRefConfig.floatLightmap)
			strcat(extradefines, "#define RGBM_LIGHTMAP\n");

		strcat(extradefines, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

		if (r_deluxeMapping->integer)
			strcat(extradefines, "#define USE_DELUXEMAP\n");

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		strcat(extradefines, "#define __LAVA__\n");

		if (!GLSL_BeginLoadGPUShader(&tr.lightAllShader[2], "lightall2", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_lightall_vp, fallbackShader_lightall_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load lightall (lava) shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		if (r_deluxeSpecular->value > 0.000001f)
			strcat(extradefines, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

		if (r_hdr->integer && !glRefConfig.floatLightmap)
			strcat(extradefines, "#define RGBM_LIGHTMAP\n");

		strcat(extradefines, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

		if (r_deluxeMapping->integer)
			strcat(extradefines, "#define USE_DELUXEMAP\n");

#ifdef __HEIGHTMAP_TERRAIN_TEST__
		strcat(extradefines, "#define __HEIGHTMAP_TERRAIN_TEST__\n");
#endif //__HEIGHTMAP_TERRAIN_TEST__

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.lightAllSplatShader[0], "lightallSplat0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_lightallSplat_vp, fallbackShader_lightallSplat_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load lightallSplat0 shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		if (r_deluxeSpecular->value > 0.000001f)
			strcat(extradefines, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

		if (r_hdr->integer && !glRefConfig.floatLightmap)
			strcat(extradefines, "#define RGBM_LIGHTMAP\n");

		strcat(extradefines, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

		if (r_deluxeMapping->integer)
			strcat(extradefines, "#define USE_DELUXEMAP\n");

		strcat(extradefines, "#define USE_TESSELLATION\n");

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.lightAllSplatShader[1], "lightallSplat1", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_lightallSplat_vp, fallbackShader_lightallSplat_fp, fallbackShader_tessellation_cs, fallbackShader_tessellation_es, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load lightallSplat1 (tess) shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		if (r_deluxeSpecular->value > 0.000001f)
			strcat(extradefines, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

		if (r_hdr->integer && !glRefConfig.floatLightmap)
			strcat(extradefines, "#define RGBM_LIGHTMAP\n");

		strcat(extradefines, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

		if (r_deluxeMapping->integer)
			strcat(extradefines, "#define USE_DELUXEMAP\n");

		strcat(extradefines, "#define USE_TESSELLATION\n");

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.lightAllSplatShader[2], "lightallSplat2", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_lightallSplat_vp, fallbackShader_lightallSplat_fp, fallbackShader_tessellationTerrain_cs, fallbackShader_tessellationTerrain_es, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load lightallSplat2 (tessTerrain) shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		if (r_deluxeSpecular->value > 0.000001f)
			strcat(extradefines, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

		if (r_hdr->integer && !glRefConfig.floatLightmap)
			strcat(extradefines, "#define RGBM_LIGHTMAP\n");

		strcat(extradefines, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

		if (r_deluxeMapping->integer)
			strcat(extradefines, "#define USE_DELUXEMAP\n");

		strcat(extradefines, "#define USE_TESSELLATION_3D\n");

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.lightAllSplatShader[3], "lightallSplat3", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_lightallSplat_vp, fallbackShader_lightallSplat_fp, fallbackShader_tessellation3D_cs, fallbackShader_tessellation3D_es, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load lightallSplat3 (tess3D) shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.depthPassShader[0], "depthPass0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_depthPass_vp, fallbackShader_depthPass_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load depthPass0 shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		strcat(extradefines, "#define USE_TESSELLATION\n");
		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.depthPassShader[1], "depthPass1", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_depthPass_vp, fallbackShader_depthPass_fp, fallbackShader_tessellation_cs, fallbackShader_tessellation_es, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load depthPass1 shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		strcat(extradefines, "#define USE_TESSELLATION\n");
		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.depthPassShader[2], "depthPass2", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_depthPass_vp, fallbackShader_depthPass_fp, fallbackShader_tessellationTerrain_cs, fallbackShader_tessellationTerrain_es, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load depthPass2 shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;

		extradefines[0] = '\0';

		strcat(extradefines, "#define USE_TESSELLATION_3D\n");
		strcat(extradefines, va("#define MAX_GLM_BONEREFS %i\n", MAX_GLM_BONEREFS));

		if (!GLSL_BeginLoadGPUShader(&tr.depthPassShader[3], "depthPass3", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_depthPass_vp, fallbackShader_depthPass_fp, fallbackShader_tessellation3D_cs, fallbackShader_tessellation3D_es, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load depthPass3 shader!");
		}
	}

	attribs = ATTR_POSITION | ATTR_POSITION2 | ATTR_NORMAL | ATTR_NORMAL2 | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.shadowFillShader[0], "shadowfill0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_shadowfill_vp, fallbackShader_shadowfill_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load shadowfill0 shader!");
	}

	attribs = ATTR_POSITION | ATTR_POSITION2 | ATTR_NORMAL | ATTR_NORMAL2 | ATTR_TEXCOORD0;
	extradefines[0] = '\0';
	strcat(extradefines, "#define USE_TESSELLATION\n");

	if (!GLSL_BeginLoadGPUShader(&tr.shadowFillShader[1], "shadowfill1", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_shadowfill_vp, fallbackShader_shadowfill_fp, fallbackShader_tessellation_cs, fallbackShader_tessellation_es, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load shadowfill1 shader!");
	}

	attribs = ATTR_POSITION | ATTR_POSITION2 | ATTR_NORMAL | ATTR_NORMAL2 | ATTR_TEXCOORD0;
	extradefines[0] = '\0';
	strcat(extradefines, "#define USE_TESSELLATION\n");

	if (!GLSL_BeginLoadGPUShader(&tr.shadowFillShader[2], "shadowfill2", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_shadowfill_vp, fallbackShader_shadowfill_fp, fallbackShader_tessellationTerrain_cs, fallbackShader_tessellationTerrain_es, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load shadowfill2 shader!");
	}

	attribs = ATTR_POSITION | ATTR_POSITION2 | ATTR_NORMAL | ATTR_NORMAL2 | ATTR_TEXCOORD0;
	extradefines[0] = '\0';
	strcat(extradefines, "#define USE_TESSELLATION_3D\n");

	if (!GLSL_BeginLoadGPUShader(&tr.shadowFillShader[3], "shadowfill3", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_shadowfill_vp, fallbackShader_shadowfill_fp, fallbackShader_tessellation3D_cs, fallbackShader_tessellation3D_es, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load shadowfill3 shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.shadowmaskShader, "shadowmask", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_shadowmask_vp, fallbackShader_shadowmask_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load shadowmask shader!");
	}


	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.shadowBlurShader, "shadowBlur", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_shadowBlur_vp, fallbackShader_shadowBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load shadowBlur shader!");
	}
	*/


	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2 | ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
		//attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;

		extradefines[0] = '\0';

		if (!GLSL_BeginLoadGPUShader(&tr.skyShader, "sky", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_sky_vp, fallbackShader_sky_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load sky shader!");
		}
	}

	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.sunPassShader, "sun", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_sun_vp, fallbackShader_sun_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load sun shader!");
	}
	*/

	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.moonPassShader, "moon", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_planet_vp, fallbackShader_planet_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load moon shader!");
	}
	*/

	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.planetPassShader, "planet", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_planet_vp, fallbackShader_planet_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load planet shader!");
	}
	*/

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.fireShader, "fire", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_fire_vp, fallbackShader_fire_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load fire shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.menuBackgroundShader, "menuBackground", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_menuBackground_vp, fallbackShader_menuBackground_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load menuBackground shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.smokeShader, "smoke", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_smoke_vp, fallbackShader_smoke_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load smoke shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.magicParticlesShader, "magicParticles", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_magicParticles_vp, fallbackShader_magicParticles_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load magicParticles shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.magicParticlesTreeShader, "magicParticlesTree", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_magicParticlesTree_vp, fallbackShader_magicParticlesTree_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load magicParticlesTree shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.magicParticlesFireFlyShader, "magicParticlesFireFly", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_magicParticlesFireFly_vp, fallbackShader_magicParticlesFireFly_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load magicParticlesFireFly shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL | ATTR_TEXCOORD1 | ATTR_POSITION2 | ATTR_NORMAL2;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.portalShader, "portal", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_portal_vp, fallbackShader_portal_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load portal shader!");
	}


	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;

		extradefines[0] = '\0';

		if (!GLSL_BeginLoadGPUShader(&tr.foliageShader, "foliage", attribs, qtrue, qtrue, qtrue, qtrue, extradefines, qtrue, NULL, fallbackShader_foliage_vp, fallbackShader_foliage_fp, fallbackShader_foliage_cs, fallbackShader_foliage_es, fallbackShader_foliage_gs))
		{
			ri->Error(ERR_FATAL, "Could not load foliage shader!");
		}
	}


	if (r_foliage->integer)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;

		extradefines[0] = '\0';

#ifdef __HUMANOIDS_BEND_GRASS__
		Q_strcat(extradefines, 1024, "#define __HUMANOIDS_BEND_GRASS__\n");
		Q_strcat(extradefines, 1024, va("#define MAX_GRASSBEND_HUMANOIDS %i\n", MAX_GRASSBEND_HUMANOIDS));
#endif //__HUMANOIDS_BEND_GRASS__

		if (!GLSL_BeginLoadGPUShader(&tr.grassPatchesShader, "grassPatches", attribs, qtrue, qtrue, qtrue, qtrue, extradefines, qtrue, NULL, fallbackShader_grassPatches_vp, fallbackShader_grassPatches_fp, fallbackShader_grassPatches_cs, fallbackShader_grassPatches_es, fallbackShader_grassPatches_gs))
		{
			ri->Error(ERR_FATAL, "Could not load grassPatches shader!");
		}



		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;

		extradefines[0] = '\0';

		Q_strcat(extradefines, 1024, "#define __USE_UNDERWATER_ONLY__\n");
#ifdef __HUMANOIDS_BEND_GRASS__
		Q_strcat(extradefines, 1024, "#define __HUMANOIDS_BEND_GRASS__\n");
		Q_strcat(extradefines, 1024, va("#define MAX_GRASSBEND_HUMANOIDS %i\n", MAX_GRASSBEND_HUMANOIDS));
#endif //__HUMANOIDS_BEND_GRASS__

		if (!GLSL_BeginLoadGPUShader(&tr.grassShader[0], "grass20", attribs, qtrue, qtrue, qtrue, qtrue, extradefines, qtrue, NULL, fallbackShader_grass2_vp, fallbackShader_grass2_fp, fallbackShader_grass2_cs, fallbackShader_grass2_es, fallbackShader_grass2_gs))
		{
			ri->Error(ERR_FATAL, "Could not load grass0 shader!");
		}


		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;

		extradefines[0] = '\0';

#ifdef __HUMANOIDS_BEND_GRASS__
		Q_strcat(extradefines, 1024, "#define __HUMANOIDS_BEND_GRASS__\n");
		Q_strcat(extradefines, 1024, va("#define MAX_GRASSBEND_HUMANOIDS %i\n", MAX_GRASSBEND_HUMANOIDS));
#endif //__HUMANOIDS_BEND_GRASS__

		if (!GLSL_BeginLoadGPUShader(&tr.grassShader[1], "grass21", attribs, qtrue, qtrue, qtrue, qtrue, extradefines, qtrue, NULL, fallbackShader_grass2_vp, fallbackShader_grass2_fp, fallbackShader_grass2_cs, fallbackShader_grass2_es, fallbackShader_grass2_gs))
		{
			ri->Error(ERR_FATAL, "Could not load grass21 shader!");
		}
	}

	if (r_foliage->integer)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;

		extradefines[0] = '\0';

		if (!GLSL_BeginLoadGPUShader(&tr.vinesShader, "vines", attribs, qtrue, qtrue, qtrue, qtrue, extradefines, qtrue, NULL, fallbackShader_vines_vp, fallbackShader_vines_fp, fallbackShader_vines_cs, fallbackShader_vines_es, fallbackShader_vines_gs))
		{
			ri->Error(ERR_FATAL, "Could not load vines shader!");
		}
	}

	//if (r_foliage->integer)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;

		extradefines[0] = '\0';

		if (!GLSL_BeginLoadGPUShader(&tr.mistShader, "mist", attribs, qtrue, qtrue, qtrue, qtrue, extradefines, qtrue, NULL, fallbackShader_mist_vp, fallbackShader_mist_fp, fallbackShader_mist_cs, fallbackShader_mist_es, fallbackShader_mist_gs))
		{
			ri->Error(ERR_FATAL, "Could not load mist shader!");
		}
	}

	
	attribs = ATTR_POSITION | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_PCF\n");

	if (!GLSL_BeginLoadGPUShader(&tr.pshadowShader[0], "pshadow0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_pshadow_vp, fallbackShader_pshadow_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load pshadow0 shader!");
	}

#ifdef __PSHADOW_TESSELLATION__
	attribs = ATTR_POSITION | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_PCF\n#define USE_TESSELLATION\n");

	if (!GLSL_BeginLoadGPUShader(&tr.pshadowShader[1], "pshadow1", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_pshadow_vp, fallbackShader_pshadow_fp, fallbackShader_tessellation_cs, fallbackShader_tessellation_es, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load pshadow1 shader!");
	}

	attribs = ATTR_POSITION | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_PCF\n#define USE_TESSELLATION\n");

	if (!GLSL_BeginLoadGPUShader(&tr.pshadowShader[2], "pshadow2", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_pshadow_vp, fallbackShader_pshadow_fp, fallbackShader_tessellationTerrain_cs, fallbackShader_tessellationTerrain_es, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load pshadow2 shader!");
	}

	attribs = ATTR_POSITION | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_PCF\n#define USE_TESSELLATION_3D\n");

	if (!GLSL_BeginLoadGPUShader(&tr.pshadowShader[3], "pshadow3", attribs, qtrue, qtrue, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_pshadow_vp, fallbackShader_pshadow_fp, fallbackShader_tessellation3D_cs, fallbackShader_tessellation3D_es, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load pshadow3 shader!");
	}
#endif //__PSHADOW_TESSELLATION__


	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.down4xShader, "down4x", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_down4x_vp, fallbackShader_down4x_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load down4x shader!");
	}
	*/

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.bokehShader, "bokeh", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_bokeh_vp, fallbackShader_bokeh_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load bokeh shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.tonemapShader, "tonemap", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_tonemap_vp, fallbackShader_tonemap_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load tonemap shader!");
	}


	for (i = 0; i < 2; i++)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0;
		extradefines[0] = '\0';

		if (!i)
			Q_strcat(extradefines, 1024, "#define FIRST_PASS\n");

		char shaderName[128] = { 0 };
		sprintf(shaderName, "calclevels4x%i", i);

		if (!GLSL_BeginLoadGPUShader(&tr.calclevels4xShader[i], shaderName, attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_calclevels4x_vp, fallbackShader_calclevels4x_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load calclevels4x%i shader!", i);
		}
	}



	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.ssaoShader, "ssao", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_ssao_vp, fallbackShader_ssao_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssao shader!");
	}


	/*attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.ssdoShader, "ssdo", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_ssdo_vp, fallbackShader_ssdo_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssdo shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.ssdoBlurShader, "ssdoBlur", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_ssdoBlur_vp, fallbackShader_ssdoBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssdoBlur shader!");
	}*/

	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.sssShader, "sss", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_sss_vp, fallbackShader_sss_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load sss shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.sssBlurShader, "sssBlur", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_sssBlur_vp, fallbackShader_sssBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load sssBlur shader!");
	}
	*/

#ifdef __XYC_SURFACE_SPRITES__
	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.surfaceSpriteShader, "surfaceSprite", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_surfaceSprite_vp, fallbackShader_surfaceSprite_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load surfaceSprite shader!");
	}
#endif //__XYC_SURFACE_SPRITES__

	/*
	for (i = 0; i < 2; i++)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0;
		extradefines[0] = '\0';

		if (i & 1)
			Q_strcat(extradefines, 1024, "#define USE_VERTICAL_BLUR\n");
		else
			Q_strcat(extradefines, 1024, "#define USE_HORIZONTAL_BLUR\n");

		char shaderName[128] = { 0 };
		sprintf(shaderName, "depthBlur%i", i);
		if (!GLSL_BeginLoadGPUShader(&tr.depthBlurShader[i], shaderName, attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_depthblur_vp, fallbackShader_depthblur_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load depthBlur%i shader!", i);
		}
	}
	*/

#if 0
	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.testcubeShader, "testcube", attribs, qtrue, qfalse, extradefines, qtrue, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load testcube shader!");
	}

	GLSL_InitUniforms(&tr.testcubeShader);

	GLSL_BindProgram(&tr.testcubeShader);
	GLSL_SetUniformInt(&tr.testcubeShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);

	GLSL_FinishGPUShader(&tr.testcubeShader);

	numEtcShaders++;
#endif

	attribs = 0;
	extradefines[0] = '\0';
	Q_strcat(extradefines, 1024, "#define BLUR_X\n");

	if (!GLSL_BeginLoadGPUShader(&tr.gaussianBlurShader[0], "gaussian_blur0", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_gaussian_blur_vp, fallbackShader_gaussian_blur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load gaussian_blur0 (X-direction) shader!");
	}

	attribs = 0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.gaussianBlurShader[1], "gaussian_blur1", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_gaussian_blur_vp, fallbackShader_gaussian_blur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load gaussian_blur1 (Y-direction) shader!");
	}

	attribs = 0;
	extradefines[0] = '\0';
	if (!GLSL_BeginLoadGPUShader(&tr.dglowDownsample, "dglow_downsample", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_dglow_downsample_vp, fallbackShader_dglow_downsample_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load dynamic glow downsample shader!");
	}

	attribs = 0;
	extradefines[0] = '\0';
	if (!GLSL_BeginLoadGPUShader(&tr.dglowUpsample, "dglow_upsample", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_dglow_upsample_vp, fallbackShader_dglow_upsample_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load dynamic glow upsample shader!");
	}

	//
	// UQ1: Added...
	//

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.darkexpandShader, "darkexpand", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_darkexpand_vp, fallbackShader_darkexpand_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load darkexpand shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.multipostShader, "multipost", attribs, qtrue, qfalse, qfalse, qfalse/*qtrue - unused*/, extradefines, qtrue, NULL, fallbackShader_multipost_vp, fallbackShader_multipost_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load multipost shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define DUAL_PASS\n");

	if (!GLSL_BeginLoadGPUShader(&tr.volumeLightShader[0], "volumelight0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_volumelight_vp, fallbackShader_volumelight_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load volumelight0 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define DUAL_PASS\n");
	Q_strcat(extradefines, 1024, "#define MQ_VOLUMETRIC\n");

	if (!GLSL_BeginLoadGPUShader(&tr.volumeLightShader[1], "volumelight1", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_volumelight_vp, fallbackShader_volumelight_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load volumelight1 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define DUAL_PASS\n");
	Q_strcat(extradefines, 1024, "#define HQ_VOLUMETRIC\n");

	if (!GLSL_BeginLoadGPUShader(&tr.volumeLightShader[2], "volumelight2", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_volumelight_vp, fallbackShader_volumelight_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load volumelight2 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.volumeLightCombineShader, "volumelightCombine", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_volumelightCombine_vp, fallbackShader_volumelightCombine_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load volumelightCombine shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.bloomBlurShader, "bloom_blur", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_bloom_blur_vp, fallbackShader_bloom_blur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load bloom_blur shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.bloomCombineShader, "bloom_combine", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_bloom_combine_vp, fallbackShader_bloom_combine_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load bloom_combine shader!");
	}

	/*attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.lensflareShader, "lensflare", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_lensflare_vp, fallbackShader_lensflare_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load lensflare shader!");
	}*/

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.anamorphicBlurShader, "anamorphic_blur", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_anamorphic_blur_vp, fallbackShader_anamorphic_blur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load anamorphic_blur shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.anamorphicCombineShader, "anamorphic_combine", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_anamorphic_combine_vp, fallbackShader_anamorphic_combine_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load anamorphic_combine shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.cellShadeShader, "cellShade", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_cellShade_vp, fallbackShader_cellShade_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load cellShade shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.paintShader, "paint", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_paint_vp, fallbackShader_paint_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load paint shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.magicdetailShader, "magicdetail", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_magicdetail_vp, fallbackShader_magicdetail_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load magicdetail shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.waterPostShader[0], "waterPost0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterPost_vp, fallbackShader_waterPost_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterPost[0] shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_REFLECTION\n");

	if (!GLSL_BeginLoadGPUShader(&tr.waterPostShader[1], "waterPost1", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterPost_vp, fallbackShader_waterPost_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterPost[1] shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_REFLECTION\n");
	Q_strcat(extradefines, 1024, "#define USE_RAYTRACE\n");

	if (!GLSL_BeginLoadGPUShader(&tr.waterPostShader[2], "waterPost2", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterPost_vp, fallbackShader_waterPost_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterPost[2] shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.waterPostShader[3], "waterPost3", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterPost2_vp, fallbackShader_waterPost2_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterPost[3] shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_REFLECTION\n");

	if (!GLSL_BeginLoadGPUShader(&tr.waterPostShader[4], "waterPost4", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterPost2_vp, fallbackShader_waterPost2_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterPost[4] shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_REFLECTION\n");
	Q_strcat(extradefines, 1024, "#define USE_RAYTRACE\n");

	if (!GLSL_BeginLoadGPUShader(&tr.waterPostShader[5], "waterPost5", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterPost2_vp, fallbackShader_waterPost2_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterPost[5] shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.waterReflectionShader, "waterReflection", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterReflection_vp, fallbackShader_waterReflection_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterReflection shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL;
	extradefines[0] = '\0';

	if(!GLSL_BeginLoadGPUShader(&tr.transparancyPostShader, "transparancyPost", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_transparancyPost_vp, fallbackShader_transparancyPost_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load transparancyPost shader!");
	}
	

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.fastBlurShader, "fastBlur", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_fastBlur_vp, fallbackShader_fastBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load fastBlur shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.bloomRaysShader, "bloomRays", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_bloomRays_vp, fallbackShader_bloomRays_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load bloomRays shader!");
	}



	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define OLD_BLUR\n");

	if (!GLSL_BeginLoadGPUShader(&tr.distanceBlurShader[0], "distanceBlur0", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_distanceBlur_vp, fallbackShader_distanceBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load distanceBlur0 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define FAST_BLUR\n");

	if (!GLSL_BeginLoadGPUShader(&tr.distanceBlurShader[1], "distanceBlur1", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_distanceBlur_vp, fallbackShader_distanceBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load distanceBlur1 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define MEDIUM_BLUR\n");

	if (!GLSL_BeginLoadGPUShader(&tr.distanceBlurShader[2], "distanceBlur2", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_distanceBlur_vp, fallbackShader_distanceBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load distanceBlur2 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define HIGH_BLUR\n");

	if (!GLSL_BeginLoadGPUShader(&tr.distanceBlurShader[3], "distanceBlur3", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_distanceBlur_vp, fallbackShader_distanceBlur_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load distanceBlur3 shader!");
	}
	*/



	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.testshaderShader, "testshader", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_testshader_vp, fallbackShader_testshader_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load testshader shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.linearizeDepthShader, "linearizeDepth", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_linearizeDepth_vp, fallbackShader_linearizeDepth_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load linearizeDepth shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.showNormalsShader, "showNormals", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_showNormals_vp, fallbackShader_showNormals_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load showNormals shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.showDepthShader, "showDepth", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_showDepth_vp, fallbackShader_showDepth_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load showDepth shader!");
	}

#ifndef __PROCEDURALS_IN_DEFERRED_SHADER__
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0;
		extradefines[0] = '\0';

		if (!GLSL_BeginLoadGPUShader(&tr.proceduralShader, "procedural", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_procedural_vp, fallbackShader_procedural_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load procedural shader!");
		}
	}
#endif //__PROCEDURALS_IN_DEFERRED_SHADER__

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0;
		extradefines[0] = '\0';

		if (r_sunlightMode->integer >= 2)
		{
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");
		}

		if (!GLSL_BeginLoadGPUShader(&tr.fastLightingShader, "fastLighting", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_fastLighting_vp, fallbackShader_fastLighting_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load fastLighting shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0;
		extradefines[0] = '\0';

		if (r_sunlightMode->integer >= 2)
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");

		if (r_cubeMapping->integer)
		{
			Q_strcat(extradefines, 1024, "#define USE_CUBEMAPS\n");

#ifdef __EMISSIVE_CUBE_IBL__
			Q_strcat(extradefines, 1024, "#define USE_EMISSIVECUBES\n");
#endif //__EMISSIVE_CUBE_IBL__
		}

#ifdef __LIGHT_OCCLUSION__
		Q_strcat(extradefines, 1024, "#define __LIGHT_OCCLUSION__\n");
#endif //__LIGHT_OCCLUSION__

		Q_strcat(extradefines, 1024, "#define __FAST_LIGHTING__\n");

#ifdef __REALTIME_CUBEMAP__
		Q_strcat(extradefines, 1024, "#define REALTIME_CUBEMAPS\n");
#endif //__REALTIME_CUBEMAP__

		if (!GLSL_BeginLoadGPUShader(&tr.deferredLightingShader[0], "deferredLighting0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_deferredLighting_vp, fallbackShader_deferredLighting_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load deferredLighting0 shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0;
		extradefines[0] = '\0';

		if (r_sunlightMode->integer >= 2)
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");

		if (r_cubeMapping->integer)
		{
			Q_strcat(extradefines, 1024, "#define USE_CUBEMAPS\n");

#ifdef __EMISSIVE_CUBE_IBL__
			Q_strcat(extradefines, 1024, "#define USE_EMISSIVECUBES\n");
#endif //__EMISSIVE_CUBE_IBL__
		}

#ifdef __LIGHT_OCCLUSION__
		Q_strcat(extradefines, 1024, "#define __LIGHT_OCCLUSION__\n");
#endif //__LIGHT_OCCLUSION__

		Q_strcat(extradefines, 1024, "#define __NAYAR_LIGHTING__\n");

#ifdef __REALTIME_CUBEMAP__
		Q_strcat(extradefines, 1024, "#define REALTIME_CUBEMAPS\n");
#endif //__REALTIME_CUBEMAP__

		if (!GLSL_BeginLoadGPUShader(&tr.deferredLightingShader[1], "deferredLighting1", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_deferredLighting_vp, fallbackShader_deferredLighting_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load deferredLighting1 shader!");
		}
	}

	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD0;
		extradefines[0] = '\0';

		if (r_sunlightMode->integer >= 2)
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");

		if (r_cubeMapping->integer)
		{
			Q_strcat(extradefines, 1024, "#define USE_CUBEMAPS\n");

#ifdef __EMISSIVE_CUBE_IBL__
			Q_strcat(extradefines, 1024, "#define USE_EMISSIVECUBES\n");
#endif //__EMISSIVE_CUBE_IBL__
		}

#ifdef __LIGHT_OCCLUSION__
		Q_strcat(extradefines, 1024, "#define __LIGHT_OCCLUSION__\n");
#endif //__LIGHT_OCCLUSION__

#ifdef __REALTIME_CUBEMAP__
		Q_strcat(extradefines, 1024, "#define REALTIME_CUBEMAPS\n");
#endif //__REALTIME_CUBEMAP__

		if (!GLSL_BeginLoadGPUShader(&tr.deferredLightingShader[2], "deferredLighting2", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_deferredLighting_vp, fallbackShader_deferredLighting_fp, NULL, NULL, NULL))
		{
			ri->Error(ERR_FATAL, "Could not load deferredLighting2 shader!");
		}
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.ssdmShader, "ssdm", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_ssdm_vp, fallbackShader_ssdm_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssdm shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';
	
	if (!GLSL_BeginLoadGPUShader(&tr.ssdmGenerateShader[0], "ssdmGenerate0", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_ssdmGenerate_vp, fallbackShader_ssdmGenerate_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssdmGenerate0 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define __HQ_PARALLAX__\n");

	if (!GLSL_BeginLoadGPUShader(&tr.ssdmGenerateShader[1], "ssdmGenerate1", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_ssdmGenerate_vp, fallbackShader_ssdmGenerate_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssdmGenerate1 shader!");
	}

	/*
	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.ssrShader, "ssr", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_ssr_vp, fallbackShader_ssr_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssr shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.ssrCombineShader, "ssrCombine", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_ssrCombine_vp, fallbackShader_ssrCombine_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load ssrCombine shader!");
	}
	*/

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.colorCorrectionShader, "colorCorrection", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_colorCorrection_vp, fallbackShader_colorCorrection_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load colorCorrection shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.fogPostShader, "fogPost", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_fogPost_vp, fallbackShader_fogPost_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load fogPost shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.dofFocusDepthShader, "dofFocusDepth", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_dofFocusDepth_vp, fallbackShader_dofFocusDepth_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load dofFocusDepth shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define FAST_DOF\n");

	if (!GLSL_BeginLoadGPUShader(&tr.dofShader[0], "depthOfField0", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_depthOfField2_vp, fallbackShader_depthOfField2_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load depthOfField0 shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define MEDIUM_DOF\n");

	if (!GLSL_BeginLoadGPUShader(&tr.dofShader[1], "depthOfField1", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_depthOfField2_vp, fallbackShader_depthOfField2_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load depthOfField1 shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.dofShader[2], "depthOfField2", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_depthOfField2_vp, fallbackShader_depthOfField2_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load depthOfField2 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.esharpeningShader, "esharpening", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_esharpening_vp, fallbackShader_esharpening_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load esharpening shader!");
	}

	/*attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.esharpening2Shader, "esharpening2", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_esharpening2_vp, fallbackShader_esharpening2_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load esharpening2 shader!");
	}*/


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.fxaaShader, "fxaa", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_fxaa_vp, fallbackShader_fxaa_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load fxaa shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.txaaShader, "txaa", attribs, qtrue, qfalse, qfalse, qtrue/*qfalse*/, extradefines, qtrue, NULL, fallbackShader_txaa_vp, fallbackShader_txaa_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load txaa shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.generateNormalMapShader, "generateNormalMap", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_generateNormalMap_vp, fallbackShader_generateNormalMap_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load generateNormalMap shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.underwaterShader, "underwater", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_underwater_vp, fallbackShader_underwater_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load underwater shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.anaglyphShader, "anaglyph", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_anaglyph_vp, fallbackShader_anaglyph_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load anaglyph shader!");
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL | ATTR_COLOR;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.skyDomeShader, "skyDome", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_skyDome_vp, fallbackShader_skyDome_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load skyDome shader!");
	}




	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_PRIMARY_LIGHT_SPECULAR\n");

	if (!GLSL_BeginLoadGPUShader(&tr.waterPostForwardShader, "waterPostForward", attribs, qtrue, qfalse, qfalse, qtrue, extradefines, qtrue, NULL, fallbackShader_waterPostForward_vp, fallbackShader_waterPostForward_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterPostForward shader!");
	}



#ifdef __OCEAN__
	attribs = ATTR_INSTANCES_POSITION | ATTR_INSTANCES_TEXCOORD;// ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL;
#else //!__OCEAN__
	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL;
#endif //__OCEAN__
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.waterForwardShader, "waterForward", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_waterForward_vp, fallbackShader_waterForward_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterForward shader!");
	}


#ifdef __OCEAN__
	attribs = ATTR_INSTANCES_POSITION | ATTR_INSTANCES_TEXCOORD | ATTR_NORMAL;// ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL;
#else //!__OCEAN__
	attribs = ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_COLOR | ATTR_NORMAL;
#endif //__OCEAN__
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.waterForwardFastShader, "waterForwardFast", attribs, qtrue, qfalse, qfalse, qfalse/*qtrue*/, extradefines, qtrue, NULL, fallbackShader_waterForwardFast_vp, fallbackShader_waterForwardFast_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load waterForwardFast shader!");
	}


	/*attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define FAST_HBAO\n");

	if (!GLSL_BeginLoadGPUShader(&tr.hbaoShader, "hbao", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_hbao_vp, fallbackShader_hbao_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load hbao shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.hbao2Shader, "hbao2", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_hbao_vp, fallbackShader_hbao_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load hbao2 shader!");
	}

	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.hbaoCombineShader, "hbaoCombine", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_hbaoCombine_vp, fallbackShader_hbaoCombine_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load hbaoCombine shader!");
	}*/

	attribs = ATTR_INSTANCES_POSITION | ATTR_INSTANCES_TEXCOORD | ATTR_NORMAL;
	extradefines[0] = '\0';

	if (!GLSL_BeginLoadGPUShader(&tr.cloudsShader, "clouds", attribs, qtrue, qfalse, qfalse, qfalse, extradefines, qtrue, NULL, fallbackShader_clouds_vp, fallbackShader_clouds_fp, NULL, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load clouds shader!");
	}

	//
	// UQ1: End Added...
	//

#ifdef __GLSL_OPTIMIZER__
	if (r_glslOptimize->integer)
	{
		glslopt_cleanup(ctx);
	}
#endif //__GLSL_OPTIMIZER__

	//ri->Error(ERR_DROP, "Oh noes!\n");
	return startTime;
}

void GLSL_EndLoadGPUShaders(int startTime)
{
	int i;
	int numLightShaders = 0, numEtcShaders = 0;

	if (!GLSL_EndLoadGPUShader(&tr.textureColorShader))
	{
		ri->Error(ERR_FATAL, "Could not load texturecolor shader!");
	}

	GLSL_InitUniforms(&tr.textureColorShader);

	GLSL_BindProgram(&tr.textureColorShader);
	GLSL_SetUniformInt(&tr.textureColorShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.textureColorShader);

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.weatherShader))
	{
		ri->Error(ERR_FATAL, "Could not load weather shader!");
	}

	GLSL_InitUniforms(&tr.weatherShader);

	GLSL_BindProgram(&tr.weatherShader);
	GLSL_SetUniformInt(&tr.weatherShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.weatherShader);

	numEtcShaders++;


#ifdef __INSTANCED_MODELS__
	if (!GLSL_EndLoadGPUShader(&tr.instanceShader))
	{
		ri->Error(ERR_FATAL, "Could not load instance shader!");
	}

	GLSL_InitUniforms(&tr.instanceShader);

	GLSL_BindProgram(&tr.instanceShader);
	GLSL_SetUniformInt(&tr.instanceShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.instanceShader);

	numEtcShaders++;
#endif //__INSTANCED_MODELS__

#ifdef __LODMODEL_INSTANCING__
	if (!GLSL_EndLoadGPUShader(&tr.instanceVAOShader))
	{
		ri->Error(ERR_FATAL, "Could not load instanceVao shader!");
	}

	GLSL_InitUniforms(&tr.instanceVAOShader);

	GLSL_BindProgram(&tr.instanceVAOShader);
	GLSL_SetUniformInt(&tr.instanceVAOShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.instanceVAOShader);

	numEtcShaders++;
#endif //__LODMODEL_INSTANCING__


	if (!GLSL_EndLoadGPUShader(&tr.occlusionShader))
	{
		ri->Error(ERR_FATAL, "Could not load occlusion shader!");
	}

	GLSL_InitUniforms(&tr.occlusionShader);

	GLSL_BindProgram(&tr.occlusionShader);

	//GLSL_SetUniformInt(&tr.occlusionShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);
	//GLSL_SetUniformInt(&tr.occlusionShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

	GLSL_FinishGPUShader(&tr.occlusionShader);

	numEtcShaders++;

	
	if (!GLSL_EndLoadGPUShader(&tr.depthAdjustShader))
	{
		ri->Error(ERR_FATAL, "Could not load depthAdjust shader!");
	}

	GLSL_InitUniforms(&tr.depthAdjustShader);

	GLSL_BindProgram(&tr.depthAdjustShader);

	GLSL_SetUniformInt(&tr.depthAdjustShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

	GLSL_FinishGPUShader(&tr.depthAdjustShader);

	numEtcShaders++;



	for (i = 0; i < 4; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.depthPassShader[i]))
		{
			ri->Error(ERR_FATAL, va("Could not load depthPass%i shader!", i));
		}

		GLSL_InitUniforms(&tr.depthPassShader[i]);
		GLSL_BindProgram(&tr.depthPassShader[i]);

		GLSL_SetUniformInt(&tr.depthPassShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.depthPassShader[i], UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.depthPassShader[i]);
#endif

		numLightShaders++;
	}
	

	for (int i = 0; i < 3; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.lightAllShader[i]))
		{
			ri->Error(ERR_FATAL, "Could not load lightall shader!");
		}

		GLSL_InitUniforms(&tr.lightAllShader[i]);
		GLSL_BindProgram(&tr.lightAllShader[i]);

		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_LIGHTMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_DELUXEMAP, TB_DELUXEMAP);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_SPECULARMAP, TB_SPECULARMAP);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_SHADOWMAP, TB_SPLATMAP1);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_OVERLAYMAP, TB_OVERLAYMAP);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_GLOWMAP, TB_GLOWMAP);
		GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_ENVMAP, TB_ENVMAP);
		//GLSL_SetUniformInt(&tr.lightAllShader[i], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.lightAllShader[i]);
#endif

		numLightShaders++;
	}



	for (int i = 0; i < 4; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.lightAllSplatShader[i]))
		{
			ri->Error(ERR_FATAL, "Could not load lightallSplat shader!");
		}

		GLSL_InitUniforms(&tr.lightAllSplatShader[i]);
		GLSL_BindProgram(&tr.lightAllSplatShader[i]);

		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_SPLATMAP1, TB_SPLATMAP1);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_SPLATMAP2, TB_SPLATMAP2);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_SPLATMAP3, TB_SPLATMAP3);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_STEEPMAP, TB_STEEPMAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_STEEPMAP1, TB_STEEPMAP1);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_STEEPMAP2, TB_STEEPMAP2);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_STEEPMAP3, TB_STEEPMAP3);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_WATER_EDGE_MAP, TB_WATER_EDGE_MAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_SPLATCONTROLMAP, TB_SPLATCONTROLMAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_ROADMAP, TB_ROADMAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_LIGHTMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_DETAILMAP, TB_DETAILMAP);
		//GLSL_SetUniformInt(&tr.lightAllSplatShader[i], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.lightAllSplatShader[i]);
#endif

		numLightShaders++;
	}



	if (!GLSL_EndLoadGPUShader(&tr.skyShader))
	{
		ri->Error(ERR_FATAL, "Could not load sky shader!");
	}

	GLSL_InitUniforms(&tr.skyShader);

	GLSL_BindProgram(&tr.skyShader);
	GLSL_SetUniformInt(&tr.skyShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.skyShader, UNIFORM_SPLATMAP1, TB_SPLATMAP1);
	GLSL_SetUniformInt(&tr.skyShader, UNIFORM_SPLATMAP2, TB_SPLATMAP2);
	GLSL_SetUniformInt(&tr.skyShader, UNIFORM_SPLATMAP3, TB_SPLATMAP3);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.skyShader);
#endif

	numLightShaders++;


	/*
	if (!GLSL_EndLoadGPUShader(&tr.sunPassShader))
	{
		ri->Error(ERR_FATAL, "Could not load sun shader!");
	}

	GLSL_InitUniforms(&tr.sunPassShader);

	GLSL_BindProgram(&tr.sunPassShader);
	GLSL_SetUniformInt(&tr.sunPassShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.sunPassShader);
#endif

	numLightShaders++;
	*/


	/*
	if (!GLSL_EndLoadGPUShader(&tr.moonPassShader))
	{
		ri->Error(ERR_FATAL, "Could not load moon shader!");
	}

	GLSL_InitUniforms(&tr.moonPassShader);

	GLSL_BindProgram(&tr.moonPassShader);
	GLSL_SetUniformInt(&tr.moonPassShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.moonPassShader, UNIFORM_SPECULARMAP, TB_SPECULARMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.moonPassShader);
#endif

	numLightShaders++;
	*/


	/*
	if (!GLSL_EndLoadGPUShader(&tr.planetPassShader))
	{
		ri->Error(ERR_FATAL, "Could not load planet shader!");
	}

	GLSL_InitUniforms(&tr.planetPassShader);

	GLSL_BindProgram(&tr.planetPassShader);
	GLSL_SetUniformInt(&tr.planetPassShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.planetPassShader, UNIFORM_SPECULARMAP, TB_SPECULARMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.planetPassShader);
#endif

	numLightShaders++;
	*/



	if (!GLSL_EndLoadGPUShader(&tr.fireShader))
	{
		ri->Error(ERR_FATAL, "Could not load fire shader!");
	}

	GLSL_InitUniforms(&tr.fireShader);

	GLSL_BindProgram(&tr.fireShader);
	GLSL_SetUniformInt(&tr.fireShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.fireShader);
#endif

	numLightShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.menuBackgroundShader))
	{
		ri->Error(ERR_FATAL, "Could not load menuBackground shader!");
	}

	GLSL_InitUniforms(&tr.menuBackgroundShader);

	GLSL_BindProgram(&tr.menuBackgroundShader);
	GLSL_SetUniformInt(&tr.menuBackgroundShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.menuBackgroundShader);
#endif

	numLightShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.smokeShader))
	{
		ri->Error(ERR_FATAL, "Could not load smoke shader!");
	}

	GLSL_InitUniforms(&tr.smokeShader);

	GLSL_BindProgram(&tr.smokeShader);
	GLSL_SetUniformInt(&tr.smokeShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.smokeShader);
#endif

	numLightShaders++;




	if (!GLSL_EndLoadGPUShader(&tr.magicParticlesShader))
	{
		ri->Error(ERR_FATAL, "Could not load magicParticles shader!");
	}

	GLSL_InitUniforms(&tr.magicParticlesShader);

	GLSL_BindProgram(&tr.magicParticlesShader);
	GLSL_SetUniformInt(&tr.magicParticlesShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.magicParticlesShader);
#endif

	numLightShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.magicParticlesTreeShader))
	{
		ri->Error(ERR_FATAL, "Could not load magicParticlesTree shader!");
	}

	GLSL_InitUniforms(&tr.magicParticlesTreeShader);

	GLSL_BindProgram(&tr.magicParticlesTreeShader);
	GLSL_SetUniformInt(&tr.magicParticlesTreeShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.magicParticlesTreeShader);
#endif



	numLightShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.magicParticlesFireFlyShader))
	{
		ri->Error(ERR_FATAL, "Could not load magicParticlesFireFly shader!");
	}

	GLSL_InitUniforms(&tr.magicParticlesFireFlyShader);

	GLSL_BindProgram(&tr.magicParticlesFireFlyShader);
	GLSL_SetUniformInt(&tr.magicParticlesFireFlyShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.magicParticlesFireFlyShader);
#endif

	numLightShaders++;


	if (!GLSL_EndLoadGPUShader(&tr.portalShader))
	{
		ri->Error(ERR_FATAL, "Could not load portal shader!");
	}

	GLSL_InitUniforms(&tr.portalShader);

	GLSL_BindProgram(&tr.portalShader);
	GLSL_SetUniformInt(&tr.portalShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.portalShader);
#endif

	numLightShaders++;





	if (!GLSL_EndLoadGPUShader(&tr.foliageShader))
	{
		ri->Error(ERR_FATAL, "Could not load foliage shader!");
	}

	GLSL_InitUniforms(&tr.foliageShader);

	GLSL_BindProgram(&tr.foliageShader);
	GLSL_SetUniformInt(&tr.foliageShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	
	// Control textures...
	GLSL_SetUniformInt(&tr.foliageShader, UNIFORM_SPLATCONTROLMAP, TB_SPLATCONTROLMAP);
	GLSL_SetUniformInt(&tr.foliageShader, UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);
	GLSL_SetUniformInt(&tr.foliageShader, UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);

	GLSL_SetUniformInt(&tr.foliageShader, UNIFORM_WATER_EDGE_MAP, TB_WATER_EDGE_MAP); // 16 - Sea grass 0...

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.foliageShader);
#endif

	numEtcShaders++;



	if (r_foliage->integer)
	{
		if (!GLSL_EndLoadGPUShader(&tr.grassPatchesShader))
		{
			ri->Error(ERR_FATAL, "Could not load grassPatches shader!");
		}

		GLSL_InitUniforms(&tr.grassPatchesShader);

		GLSL_BindProgram(&tr.grassPatchesShader);

		// Grass/plant textures...
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_WATER_EDGE_MAP, TB_WATER_EDGE_MAP); // 16 - Sea grass 0...

																						   // Control textures...
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_SPLATCONTROLMAP, TB_SPLATCONTROLMAP);
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_STEEPMAP, TB_STEEPMAP);
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_STEEPMAP1, TB_STEEPMAP1);
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_STEEPMAP2, TB_STEEPMAP2);
		GLSL_SetUniformInt(&tr.grassPatchesShader, UNIFORM_STEEPMAP3, TB_STEEPMAP3);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.grassPatchesShader[0]);
#endif

		numEtcShaders++;



		if (!GLSL_EndLoadGPUShader(&tr.grassShader[0]))
		{
			ri->Error(ERR_FATAL, "Could not load grass shader!");
		}

		GLSL_InitUniforms(&tr.grassShader[0]);

		GLSL_BindProgram(&tr.grassShader[0]);
		
		// Grass/plant textures...
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_WATER_EDGE_MAP, TB_WATER_EDGE_MAP); // 16 - Sea grass 0...

		// Control textures...
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_SPLATCONTROLMAP, TB_SPLATCONTROLMAP);
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_STEEPMAP, TB_STEEPMAP);
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_STEEPMAP1, TB_STEEPMAP1);
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_STEEPMAP2, TB_STEEPMAP2);
		GLSL_SetUniformInt(&tr.grassShader[0], UNIFORM_STEEPMAP3, TB_STEEPMAP3);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.grassShader[0]);
#endif

		numEtcShaders++;



		if (!GLSL_EndLoadGPUShader(&tr.grassShader[1]))
		{
			ri->Error(ERR_FATAL, "Could not load grass2 shader!");
		}

		GLSL_InitUniforms(&tr.grassShader[1]);

		GLSL_BindProgram(&tr.grassShader[1]);
		
		// Grass/plant textures...
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP); // 0

		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_WATER_EDGE_MAP, TB_WATER_EDGE_MAP); // 16 - Sea grass 0...

		// Control textures...
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_SPLATCONTROLMAP, TB_SPLATCONTROLMAP);
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_STEEPMAP, TB_STEEPMAP);
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_STEEPMAP1, TB_STEEPMAP1);
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_STEEPMAP2, TB_STEEPMAP2);
		GLSL_SetUniformInt(&tr.grassShader[1], UNIFORM_STEEPMAP3, TB_STEEPMAP3);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.grassShader[1]);
#endif

		numEtcShaders++;


		if (!GLSL_EndLoadGPUShader(&tr.vinesShader))
		{
			ri->Error(ERR_FATAL, "Could not load vines shader!");
		}

		GLSL_InitUniforms(&tr.vinesShader);

		GLSL_BindProgram(&tr.vinesShader);

		// Grass/plant textures...
		GLSL_SetUniformInt(&tr.vinesShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP); // 0
		GLSL_SetUniformInt(&tr.vinesShader, UNIFORM_WATER_EDGE_MAP, TB_WATER_EDGE_MAP); // 16 - Sea grass 0...

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.vinesShader);
#endif

		numEtcShaders++;




		if (!GLSL_EndLoadGPUShader(&tr.mistShader))
		{
			ri->Error(ERR_FATAL, "Could not load mist shader!");
		}

		GLSL_InitUniforms(&tr.mistShader);

		GLSL_BindProgram(&tr.mistShader);

		// Grass/plant textures...
		GLSL_SetUniformInt(&tr.mistShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP); // 0

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.mistShader);
#endif

		numEtcShaders++;
	}

	for (int i = 0; i < 4; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.shadowFillShader[i]))
		{
			ri->Error(ERR_FATAL, va("Could not load shadowfill%i shader!", i));
		}

		GLSL_InitUniforms(&tr.shadowFillShader[i]);
		GLSL_BindProgram(&tr.shadowFillShader[i]);

		GLSL_SetUniformInt(&tr.shadowFillShader[i], UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.shadowFillShader[i]);
#endif

		numEtcShaders++;
	}


	for (int i = 0; i < 4; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.pshadowShader[i]))
		{
			ri->Error(ERR_FATAL, va("Could not load pshadow%i shader!", i));
		}

		GLSL_InitUniforms(&tr.pshadowShader[i]);

		GLSL_BindProgram(&tr.pshadowShader[i]);
		GLSL_SetUniformInt(&tr.pshadowShader[i], UNIFORM_SHADOWMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.pshadowShader[i], UNIFORM_ROADSCONTROLMAP, TB_ROADSCONTROLMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.pshadowShader[i]);
#endif

		numEtcShaders++;
	}


	/*
	if (!GLSL_EndLoadGPUShader(&tr.down4xShader))
	{
		ri->Error(ERR_FATAL, "Could not load down4x shader!");
	}

	GLSL_InitUniforms(&tr.down4xShader);

	GLSL_BindProgram(&tr.down4xShader);
	GLSL_SetUniformInt(&tr.down4xShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.down4xShader);
#endif

	numEtcShaders++;
	*/


	if (!GLSL_EndLoadGPUShader(&tr.bokehShader))
	{
		ri->Error(ERR_FATAL, "Could not load bokeh shader!");
	}

	GLSL_InitUniforms(&tr.bokehShader);

	GLSL_BindProgram(&tr.bokehShader);
	GLSL_SetUniformInt(&tr.bokehShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.bokehShader);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.tonemapShader))
	{
		ri->Error(ERR_FATAL, "Could not load tonemap shader!");
	}

	GLSL_InitUniforms(&tr.tonemapShader);

	GLSL_BindProgram(&tr.tonemapShader);
	GLSL_SetUniformInt(&tr.tonemapShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.tonemapShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.tonemapShader);
#endif

	numEtcShaders++;

	for (i = 0; i < 2; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.calclevels4xShader[i]))
		{
			ri->Error(ERR_FATAL, "Could not load calclevels4x shader!");
		}

		GLSL_InitUniforms(&tr.calclevels4xShader[i]);

		GLSL_BindProgram(&tr.calclevels4xShader[i]);
		GLSL_SetUniformInt(&tr.calclevels4xShader[i], UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.calclevels4xShader[i]);
#endif

		numEtcShaders++;
	}

	if (!GLSL_EndLoadGPUShader(&tr.shadowmaskShader))
	{
		ri->Error(ERR_FATAL, "Could not load shadowmask shader!");
	}

	GLSL_InitUniforms(&tr.shadowmaskShader);

	GLSL_BindProgram(&tr.shadowmaskShader);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SCREENDEPTHMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SHADOWMAP, TB_SPLATMAP1);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SHADOWMAP2, TB_SPLATMAP2);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SHADOWMAP3, TB_SPLATMAP3);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.shadowmaskShader);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.ssaoShader))
	{
		ri->Error(ERR_FATAL, "Could not load ssao shader!");
	}

	GLSL_InitUniforms(&tr.ssaoShader);

	GLSL_BindProgram(&tr.ssaoShader);
	GLSL_SetUniformInt(&tr.ssaoShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssaoShader);
#endif

	numEtcShaders++;



	/*if (!GLSL_EndLoadGPUShader(&tr.ssdoShader))
	{
		ri->Error(ERR_FATAL, "Could not load ssdo shader!");
	}

	GLSL_InitUniforms(&tr.ssdoShader);

	GLSL_BindProgram(&tr.ssdoShader);
	GLSL_SetUniformInt(&tr.ssdoShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssdoShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.ssdoBlurShader))
	{
		ri->Error(ERR_FATAL, "Could not load ssdoBlur shader!");
	}

	GLSL_InitUniforms(&tr.ssdoBlurShader);

	GLSL_BindProgram(&tr.ssdoBlurShader);
	GLSL_SetUniformInt(&tr.ssdoBlurShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssdoBlurShader);
#endif

	numEtcShaders++;
	*/


	/*if (!GLSL_EndLoadGPUShader(&tr.sssShader))
	{
		ri->Error(ERR_FATAL, "Could not load sss shader!");
	}

	GLSL_InitUniforms(&tr.sssShader);

	GLSL_BindProgram(&tr.sssShader);
	GLSL_SetUniformInt(&tr.sssShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.sssShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.sssBlurShader))
	{
		ri->Error(ERR_FATAL, "Could not load sssBlur shader!");
	}

	GLSL_InitUniforms(&tr.sssBlurShader);

	GLSL_BindProgram(&tr.sssBlurShader);
	GLSL_SetUniformInt(&tr.sssBlurShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.sssBlurShader);
#endif

	numEtcShaders++;
	*/


#ifdef __XYC_SURFACE_SPRITES__
	if (!GLSL_EndLoadGPUShader(&tr.surfaceSpriteShader))
	{
		ri->Error(ERR_FATAL, "Could not load surfaceSprite shader!");
	}

	GLSL_InitUniforms(&tr.surfaceSpriteShader);

	GLSL_BindProgram(&tr.surfaceSpriteShader);
	GLSL_SetUniformInt(&tr.surfaceSpriteShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.surfaceSpriteShader);
#endif

	numEtcShaders++;
#endif //__XYC_SURFACE_SPRITES__


	/*
	for (i = 0; i < 2; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.depthBlurShader[i]))
		{
			ri->Error(ERR_FATAL, "Could not load depthBlur shader!");
		}

		GLSL_InitUniforms(&tr.depthBlurShader[i]);

		GLSL_BindProgram(&tr.depthBlurShader[i]);
		GLSL_SetUniformInt(&tr.depthBlurShader[i], UNIFORM_SCREENIMAGEMAP, TB_COLORMAP);
		GLSL_SetUniformInt(&tr.depthBlurShader[i], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.depthBlurShader[i]);
#endif

		numEtcShaders++;
	}
	*/

	for (i = 0; i < 2; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.gaussianBlurShader[i]))
		{
			ri->Error(ERR_FATAL, "Could not load gaussian blur shader!");
		}

		GLSL_InitUniforms(&tr.gaussianBlurShader[i]);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.gaussianBlurShader[i]);
#endif

		numEtcShaders++;
	}

	if (!GLSL_EndLoadGPUShader(&tr.dglowDownsample))
	{
		ri->Error(ERR_FATAL, "Could not load dynamic glow downsample shader!");
	}

	GLSL_InitUniforms(&tr.dglowDownsample);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.dglowDownsample);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.dglowUpsample))
	{
		ri->Error(ERR_FATAL, "Could not load dynamic glow upsample shader!");
	}

	GLSL_InitUniforms(&tr.dglowUpsample);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.dglowUpsample);
#endif

	numEtcShaders++;

#if 0
	attribs = ATTR_POSITION | ATTR_TEXCOORD0;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.testcubeShader, "testcube", attribs, qtrue, extradefines, qtrue, NULL, NULL))
	{
		ri->Error(ERR_FATAL, "Could not load testcube shader!");
	}

	GLSL_InitUniforms(&tr.testcubeShader);

	GLSL_BindProgram(&tr.testcubeShader);
	GLSL_SetUniformInt(&tr.testcubeShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);

	GLSL_FinishGPUShader(&tr.testcubeShader);

	numEtcShaders++;
#endif



	//
	// UQ1: Added...
	//

	if (!GLSL_EndLoadGPUShader(&tr.darkexpandShader))
	{
		ri->Error(ERR_FATAL, "Could not load darkexpand shader!");
	}

	GLSL_InitUniforms(&tr.darkexpandShader);

	GLSL_BindProgram(&tr.darkexpandShader);
	GLSL_SetUniformInt(&tr.darkexpandShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.darkexpandShader, UNIFORM_LEVELSMAP,  TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.darkexpandShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.darkexpandShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}


#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.darkexpandShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.multipostShader))
	{
		ri->Error(ERR_FATAL, "Could not load multipost shader!");
	}

	GLSL_InitUniforms(&tr.multipostShader);
	GLSL_BindProgram(&tr.multipostShader);
	GLSL_SetUniformInt(&tr.multipostShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.multipostShader);
#endif

	numEtcShaders++;

	for (int i = 0; i < 3; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.volumeLightShader[i]))
		{
			ri->Error(ERR_FATAL, "Could not load volumelight shader!");
		}

		GLSL_InitUniforms(&tr.volumeLightShader[i]);
		GLSL_BindProgram(&tr.volumeLightShader[i]);
		//GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		//GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		//GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_GLOWMAP, TB_GLOWMAP);
		//GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_POSITIONMAP, TB_POSITIONMAP);

		GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);
		GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_SHADOWMAP, TB_SPLATMAP1);
		GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_SHADOWMAP2, TB_SPLATMAP2);
		GLSL_SetUniformInt(&tr.volumeLightShader[i], UNIFORM_SHADOWMAP3, TB_SPLATMAP3);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.volumeLightShader[i]);
#endif

		numEtcShaders++;
	}



	if (!GLSL_EndLoadGPUShader(&tr.volumeLightCombineShader))
	{
		ri->Error(ERR_FATAL, "Could not load volumelightCombine shader!");
	}

	GLSL_InitUniforms(&tr.volumeLightCombineShader);
	GLSL_BindProgram(&tr.volumeLightCombineShader);
	GLSL_SetUniformInt(&tr.volumeLightCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.volumeLightCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.volumeLightCombineShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.bloomBlurShader))
	{
		ri->Error(ERR_FATAL, "Could not load bloom_blur shader!");
	}

	GLSL_InitUniforms(&tr.bloomBlurShader);
	GLSL_BindProgram(&tr.bloomBlurShader);
	GLSL_SetUniformInt(&tr.bloomBlurShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.bloomBlurShader);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.bloomCombineShader))
	{
		ri->Error(ERR_FATAL, "Could not load bloom_combine shader!");
	}

	GLSL_InitUniforms(&tr.bloomCombineShader);
	GLSL_BindProgram(&tr.bloomCombineShader);
	GLSL_SetUniformInt(&tr.bloomCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.bloomCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.bloomCombineShader);
#endif

	numEtcShaders++;


	/*if (!GLSL_EndLoadGPUShader(&tr.lensflareShader))
	{
		ri->Error(ERR_FATAL, "Could not load lensflare shader!");
	}

	GLSL_InitUniforms(&tr.lensflareShader);
	GLSL_BindProgram(&tr.lensflareShader);
	GLSL_SetUniformInt(&tr.lensflareShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.lensflareShader);
#endif

	numEtcShaders++;*/



	if (!GLSL_EndLoadGPUShader(&tr.anamorphicBlurShader))
	{
		ri->Error(ERR_FATAL, "Could not load anamorphic_blur shader!");
	}

	GLSL_InitUniforms(&tr.anamorphicBlurShader);
	GLSL_BindProgram(&tr.anamorphicBlurShader);
	GLSL_SetUniformInt(&tr.anamorphicBlurShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.anamorphicBlurShader);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.anamorphicCombineShader))
	{
		ri->Error(ERR_FATAL, "Could not load anamorphic_combine shader!");
	}

	GLSL_InitUniforms(&tr.anamorphicCombineShader);
	GLSL_BindProgram(&tr.anamorphicCombineShader);
	GLSL_SetUniformInt(&tr.anamorphicCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.anamorphicCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.anamorphicCombineShader);
#endif

	numEtcShaders++;





	if (!GLSL_EndLoadGPUShader(&tr.cellShadeShader))
	{
		ri->Error(ERR_FATAL, "Could not load cellShade shader!");
	}

	GLSL_InitUniforms(&tr.cellShadeShader);

	GLSL_BindProgram(&tr.cellShadeShader);

	GLSL_SetUniformInt(&tr.cellShadeShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.cellShadeShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);
	GLSL_SetUniformInt(&tr.cellShadeShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.cellShadeShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.cellShadeShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.cellShadeShader);
#endif

	numEtcShaders++;




	if (!GLSL_EndLoadGPUShader(&tr.paintShader))
	{
		ri->Error(ERR_FATAL, "Could not load paint shader!");
	}

	GLSL_InitUniforms(&tr.paintShader);

	GLSL_BindProgram(&tr.paintShader);

	GLSL_SetUniformInt(&tr.paintShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.paintShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.paintShader);
#endif

	numEtcShaders++;




	if (!GLSL_EndLoadGPUShader(&tr.magicdetailShader))
	{
		ri->Error(ERR_FATAL, "Could not load magicdetail shader!");
	}

	GLSL_InitUniforms(&tr.magicdetailShader);

	GLSL_BindProgram(&tr.magicdetailShader);

	GLSL_SetUniformInt(&tr.magicdetailShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.magicdetailShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.magicdetailShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.magicdetailShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.magicdetailShader);
#endif

	numEtcShaders++;


	for (int i = 0; i < 6; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.waterPostShader[i]))
		{
			ri->Error(ERR_FATAL, va("Could not load waterPost[%i] shader!", i));
		}

		GLSL_InitUniforms(&tr.waterPostShader[i]);

		GLSL_BindProgram(&tr.waterPostShader[i]);

		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_POSITIONMAP, TB_POSITIONMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_OVERLAYMAP, TB_OVERLAYMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_SPLATMAP1, TB_SPLATMAP1);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_SPLATMAP2, TB_SPLATMAP2);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_SPLATMAP3, TB_SPLATMAP3);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_SPLATCONTROLMAP, TB_SPLATCONTROLMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_SKYCUBEMAP, TB_SKYCUBEMAP);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_SKYCUBEMAPNIGHT, TB_SKYCUBEMAPNIGHT);
		GLSL_SetUniformInt(&tr.waterPostShader[i], UNIFORM_EMISSIVECUBE, TB_EMISSIVECUBE);

		{
			vec4_t viewInfo;

			float zmax = backEnd.viewParms.zFar;
			float zmin = r_znear->value;

			VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
			//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

			GLSL_SetUniformVec4(&tr.waterPostShader[i], UNIFORM_VIEWINFO, viewInfo);
		}

		{
			vec2_t screensize;
			screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
			screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

			GLSL_SetUniformVec2(&tr.waterPostShader[i], UNIFORM_DIMENSIONS, screensize);

			//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
		}

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.waterPostShader[i]);
#endif

		numEtcShaders++;
	}

	if (!GLSL_EndLoadGPUShader(&tr.waterReflectionShader))
	{
		ri->Error(ERR_FATAL, "Could not load waterReflection shader!");
	}

	GLSL_InitUniforms(&tr.waterReflectionShader);

	GLSL_BindProgram(&tr.waterReflectionShader);

	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_POSITIONMAP, TB_POSITIONMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_OVERLAYMAP, TB_OVERLAYMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_SPLATMAP1, TB_SPLATMAP1);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_SPLATMAP2, TB_SPLATMAP2);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_SPLATMAP3, TB_SPLATMAP3);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_SPLATCONTROLMAP, TB_SPLATCONTROLMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_SKYCUBEMAP, TB_SKYCUBEMAP);
	GLSL_SetUniformInt(&tr.waterReflectionShader, UNIFORM_SKYCUBEMAPNIGHT, TB_SKYCUBEMAPNIGHT);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.waterReflectionShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.waterReflectionShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.waterReflectionShader);
#endif

	numEtcShaders++;

	

	if (!GLSL_EndLoadGPUShader(&tr.transparancyPostShader))
	{
		ri->Error(ERR_FATAL, "Could not load transparancyPost shader!");
	}

	GLSL_InitUniforms(&tr.transparancyPostShader);

	GLSL_BindProgram(&tr.transparancyPostShader);

	GLSL_SetUniformInt(&tr.transparancyPostShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.transparancyPostShader, UNIFORM_WATERPOSITIONMAP, TB_WATERPOSITIONMAP);
	GLSL_SetUniformInt(&tr.transparancyPostShader, UNIFORM_POSITIONMAP, TB_POSITIONMAP);

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.transparancyPostShader, UNIFORM_DIMENSIONS, screensize);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.transparancyPostShader);
#endif

	numEtcShaders++;


	for (int num = 0; num < 3; num++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.dofShader[num]))
		{
			ri->Error(ERR_FATAL, "Could not load depthOfField shader!");
		}

		GLSL_InitUniforms(&tr.dofShader[num]);

		GLSL_BindProgram(&tr.dofShader[num]);

		GLSL_SetUniformInt(&tr.dofShader[num], UNIFORM_TEXTUREMAP, TB_COLORMAP);
		GLSL_SetUniformInt(&tr.dofShader[num], UNIFORM_LEVELSMAP, TB_LEVELSMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.dofShader[num]);
#endif

		numEtcShaders++;
	}




	if (!GLSL_EndLoadGPUShader(&tr.testshaderShader))
	{
		ri->Error(ERR_FATAL, "Could not load testshader shader!");
	}

	GLSL_InitUniforms(&tr.testshaderShader);

	GLSL_BindProgram(&tr.testshaderShader);

	GLSL_SetUniformInt(&tr.testshaderShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.testshaderShader, UNIFORM_POSITIONMAP, TB_POSITIONMAP);
	GLSL_SetUniformInt(&tr.testshaderShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.testshaderShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.testshaderShader, UNIFORM_DELUXEMAP, TB_DELUXEMAP);
	GLSL_SetUniformInt(&tr.testshaderShader, UNIFORM_GLOWMAP, TB_GLOWMAP);

	GLSL_SetUniformVec3(&tr.testshaderShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);
	GLSL_SetUniformFloat(&tr.testshaderShader, UNIFORM_TIME, backEnd.refdef.floatTime);

	{
		vec4_t loc;
		VectorSet4(loc, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value);
		GLSL_SetUniformVec4(&tr.testshaderShader, UNIFORM_LOCAL0, loc);
	}

	{
		vec4_t loc;
		VectorSet4(loc, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value, r_testshaderValue4->value);
		GLSL_SetUniformVec4(&tr.testshaderShader, UNIFORM_LOCAL1, loc);
	}


	{
		vec4_t loc;
		VectorSet4(loc, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(&tr.testshaderShader, UNIFORM_MAPINFO, loc);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.testshaderShader, UNIFORM_DIMENSIONS, screensize);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.testshaderShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.showNormalsShader))
	{
		ri->Error(ERR_FATAL, "Could not load showNormals shader!");
	}

	GLSL_InitUniforms(&tr.showNormalsShader);

	GLSL_BindProgram(&tr.showNormalsShader);

	GLSL_SetUniformInt(&tr.showNormalsShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.showNormalsShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.showNormalsShader);
#endif

	numEtcShaders++;


	if (!GLSL_EndLoadGPUShader(&tr.showDepthShader))
	{
		ri->Error(ERR_FATAL, "Could not load showDepth shader!");
	}

	GLSL_InitUniforms(&tr.showDepthShader);

	GLSL_BindProgram(&tr.showDepthShader);

	GLSL_SetUniformInt(&tr.showDepthShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.showDepthShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.showDepthShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.linearizeDepthShader))
	{
		ri->Error(ERR_FATAL, "Could not load linearizeDepth shader!");
	}

	GLSL_InitUniforms(&tr.linearizeDepthShader);

	GLSL_BindProgram(&tr.linearizeDepthShader);

	GLSL_SetUniformInt(&tr.linearizeDepthShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.linearizeDepthShader);
#endif

	numEtcShaders++;


#ifndef __PROCEDURALS_IN_DEFERRED_SHADER__
	{
		if (!GLSL_EndLoadGPUShader(&tr.proceduralShader))
		{
			ri->Error(ERR_FATAL, "Could not load procedural!");
		}

		GLSL_InitUniforms(&tr.proceduralShader);

		GLSL_BindProgram(&tr.proceduralShader);

		GLSL_SetUniformInt(&tr.proceduralShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.proceduralShader, UNIFORM_POSITIONMAP, TB_POSITIONMAP);
		GLSL_SetUniformInt(&tr.proceduralShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.proceduralShader, UNIFORM_SHADOWMAP, TB_SHADOWMAP);
		GLSL_SetUniformInt(&tr.proceduralShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.proceduralShader, UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);

		GLSL_SetUniformVec3(&tr.proceduralShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.proceduralShader);
#endif

		numEtcShaders++;
	}
#endif //__PROCEDURALS_IN_DEFERRED_SHADER__

	{
		if (!GLSL_EndLoadGPUShader(&tr.fastLightingShader))
		{
			ri->Error(ERR_FATAL, "Could not load deferredLightingshader0!");
		}

		GLSL_InitUniforms(&tr.fastLightingShader);

		GLSL_BindProgram(&tr.fastLightingShader);

		GLSL_SetUniformInt(&tr.fastLightingShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.fastLightingShader, UNIFORM_POSITIONMAP, TB_POSITIONMAP);
		GLSL_SetUniformInt(&tr.fastLightingShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.fastLightingShader, UNIFORM_SHADOWMAP, TB_SHADOWMAP);
		GLSL_SetUniformInt(&tr.fastLightingShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.fastLightingShader, UNIFORM_CUBEMAP, TB_CUBEMAP);
		GLSL_SetUniformInt(&tr.fastLightingShader, UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.fastLightingShader);
#endif

		numEtcShaders++;
	}

	{
		if (!GLSL_EndLoadGPUShader(&tr.deferredLightingShader[0]))
		{
			ri->Error(ERR_FATAL, "Could not load deferredLightingshader0!");
		}

		GLSL_InitUniforms(&tr.deferredLightingShader[0]);

		GLSL_BindProgram(&tr.deferredLightingShader[0]);

		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_POSITIONMAP, TB_POSITIONMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_SHADOWMAP, TB_SHADOWMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_CUBEMAP, TB_CUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_CUBEMAP, TB_CUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_SKYCUBEMAP, TB_SKYCUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[0], UNIFORM_SKYCUBEMAPNIGHT, TB_SKYCUBEMAPNIGHT);

		GLSL_SetUniformVec3(&tr.deferredLightingShader[0], UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.deferredLightingShader[0]);
#endif

		numEtcShaders++;
	}

	{
		if (!GLSL_EndLoadGPUShader(&tr.deferredLightingShader[1]))
		{
			ri->Error(ERR_FATAL, "Could not load deferredLightingshader1!");
		}

		GLSL_InitUniforms(&tr.deferredLightingShader[1]);

		GLSL_BindProgram(&tr.deferredLightingShader[1]);

		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_POSITIONMAP, TB_POSITIONMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_SHADOWMAP, TB_SHADOWMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_CUBEMAP, TB_CUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_CUBEMAP, TB_CUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_SKYCUBEMAP, TB_SKYCUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[1], UNIFORM_SKYCUBEMAPNIGHT, TB_SKYCUBEMAPNIGHT);

		GLSL_SetUniformVec3(&tr.deferredLightingShader[1], UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.deferredLightingShader[1]);
#endif

		numEtcShaders++;
	}

	{
		if (!GLSL_EndLoadGPUShader(&tr.deferredLightingShader[2]))
		{
			ri->Error(ERR_FATAL, "Could not load deferredLightingshader2!");
		}

		GLSL_InitUniforms(&tr.deferredLightingShader[2]);

		GLSL_BindProgram(&tr.deferredLightingShader[2]);

		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_POSITIONMAP, TB_POSITIONMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_NORMALMAP, TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_SHADOWMAP, TB_SHADOWMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_CUBEMAP, TB_CUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_CUBEMAP, TB_CUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_SKYCUBEMAP, TB_SKYCUBEMAP);
		GLSL_SetUniformInt(&tr.deferredLightingShader[2], UNIFORM_SKYCUBEMAPNIGHT, TB_SKYCUBEMAPNIGHT);

		GLSL_SetUniformVec3(&tr.deferredLightingShader[2], UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.deferredLightingShader[2]);
#endif

		numEtcShaders++;
	}



	if (!GLSL_EndLoadGPUShader(&tr.ssdmShader))
	{
		ri->Error(ERR_FATAL, "Could not load ssdmShader!");
	}

	GLSL_InitUniforms(&tr.ssdmShader);

	GLSL_BindProgram(&tr.ssdmShader);

	GLSL_SetUniformInt(&tr.ssdmShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.ssdmShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.ssdmShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

	GLSL_SetUniformVec3(&tr.ssdmShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssdmShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.ssdmGenerateShader[0]))
	{
		ri->Error(ERR_FATAL, "Could not load ssdmGenerateShader1!");
	}

	GLSL_InitUniforms(&tr.ssdmGenerateShader[0]);

	GLSL_BindProgram(&tr.ssdmGenerateShader[0]);

	GLSL_SetUniformInt(&tr.ssdmGenerateShader[0], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[0], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[0], UNIFORM_POSITIONMAP, TB_POSITIONMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[0], UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[0], UNIFORM_GLOWMAP, TB_GLOWMAP);

	GLSL_SetUniformVec3(&tr.ssdmGenerateShader[0], UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssdmGenerateShader[0]);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.ssdmGenerateShader[1]))
	{
		ri->Error(ERR_FATAL, "Could not load ssdmGenerateShader2!");
	}

	GLSL_InitUniforms(&tr.ssdmGenerateShader[1]);

	GLSL_BindProgram(&tr.ssdmGenerateShader[1]);

	GLSL_SetUniformInt(&tr.ssdmGenerateShader[1], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[1], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[1], UNIFORM_POSITIONMAP, TB_POSITIONMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[1], UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.ssdmGenerateShader[1], UNIFORM_GLOWMAP, TB_GLOWMAP);

	GLSL_SetUniformVec3(&tr.ssdmGenerateShader[1], UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssdmGenerateShader[1]);
#endif

	numEtcShaders++;



	/*if (!GLSL_EndLoadGPUShader(&tr.ssrShader))
	{
		ri->Error(ERR_FATAL, "Could not load ssr!");
	}

	GLSL_InitUniforms(&tr.ssrShader);

	GLSL_BindProgram(&tr.ssrShader);

	GLSL_SetUniformInt(&tr.ssrShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.ssrShader, UNIFORM_GLOWMAP, TB_GLOWMAP);
	GLSL_SetUniformInt(&tr.ssrShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssrShader);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.ssrCombineShader))
	{
		ri->Error(ERR_FATAL, "Could not load ssrCombine shader!");
	}

	GLSL_InitUniforms(&tr.ssrCombineShader);
	GLSL_BindProgram(&tr.ssrCombineShader);
	GLSL_SetUniformInt(&tr.ssrCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.ssrCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.ssrCombineShader);
#endif

	numEtcShaders++;
	*/




	if (!GLSL_EndLoadGPUShader(&tr.colorCorrectionShader))
	{
		ri->Error(ERR_FATAL, "Could not load colorCorrection shader!");
	}

	GLSL_InitUniforms(&tr.colorCorrectionShader);

	GLSL_BindProgram(&tr.colorCorrectionShader);

	GLSL_SetUniformInt(&tr.colorCorrectionShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.colorCorrectionShader, UNIFORM_DELUXEMAP, TB_DELUXEMAP);
	GLSL_SetUniformInt(&tr.colorCorrectionShader, UNIFORM_GLOWMAP, TB_GLOWMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.colorCorrectionShader);
#endif

	numEtcShaders++;


	if (!GLSL_EndLoadGPUShader(&tr.fogPostShader))
	{
		ri->Error(ERR_FATAL, "Could not load fogPost shader!");
	}

	GLSL_InitUniforms(&tr.fogPostShader);

	GLSL_BindProgram(&tr.fogPostShader);

	GLSL_SetUniformInt(&tr.fogPostShader, UNIFORM_HEIGHTMAP, TB_HEIGHTMAP);
	GLSL_SetUniformInt(&tr.fogPostShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.fogPostShader, UNIFORM_POSITIONMAP, TB_POSITIONMAP);
	GLSL_SetUniformInt(&tr.fogPostShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.fogPostShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.fogPostShader, UNIFORM_DIMENSIONS, screensize);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.fogPostShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.fastBlurShader))
	{
		ri->Error(ERR_FATAL, "Could not load fastBlur shader!");
	}

	GLSL_InitUniforms(&tr.fastBlurShader);

	GLSL_BindProgram(&tr.fastBlurShader);

	GLSL_SetUniformInt(&tr.fastBlurShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.fastBlurShader, UNIFORM_SCREENDEPTHMAP, TB_HEIGHTMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.fastBlurShader);
#endif

	numEtcShaders++;


	/*
	if (!GLSL_EndLoadGPUShader(&tr.shadowBlurShader))
	{
		ri->Error(ERR_FATAL, "Could not load shadowBlur shader!");
	}

	GLSL_InitUniforms(&tr.shadowBlurShader);

	GLSL_BindProgram(&tr.shadowBlurShader);

	GLSL_SetUniformInt(&tr.shadowBlurShader, UNIFORM_SHADOWMAP, TB_SHADOWMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.shadowBlurShader);
#endif

	numEtcShaders++;
	*/



	if (!GLSL_EndLoadGPUShader(&tr.bloomRaysShader))
	{
		ri->Error(ERR_FATAL, "Could not load bloomRays shader!");
	}

	GLSL_InitUniforms(&tr.bloomRaysShader);

	GLSL_BindProgram(&tr.bloomRaysShader);

	GLSL_SetUniformInt(&tr.bloomRaysShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.bloomRaysShader, UNIFORM_GLOWMAP, TB_GLOWMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.bloomRaysShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.dofFocusDepthShader))
	{
		ri->Error(ERR_FATAL, "Could not load dofFocusDepth shader!");
	}

	GLSL_InitUniforms(&tr.dofFocusDepthShader);

	GLSL_BindProgram(&tr.dofFocusDepthShader);

	GLSL_SetUniformInt(&tr.dofFocusDepthShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.dofFocusDepthShader);
#endif

	numEtcShaders++;



	/*
	for (i = 0; i < 4; i++)
	{
		if (!GLSL_EndLoadGPUShader(&tr.distanceBlurShader[i]))
		{
			ri->Error(ERR_FATAL, "Could not load distanceBlur%i shader!", i);
		}

		GLSL_InitUniforms(&tr.distanceBlurShader[i]);

		GLSL_BindProgram(&tr.distanceBlurShader[i]);

		GLSL_SetUniformInt(&tr.distanceBlurShader[i], UNIFORM_LEVELSMAP, TB_LEVELSMAP);
		GLSL_SetUniformInt(&tr.distanceBlurShader[i], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.distanceBlurShader[i], UNIFORM_GLOWMAP, TB_GLOWMAP);

		{
			vec2_t screensize;
			screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
			screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

			GLSL_SetUniformVec2(&tr.distanceBlurShader[i], UNIFORM_DIMENSIONS, screensize);
		}

		{
			vec4_t info;

			info[0] = r_distanceBlur->value;
			info[1] = r_dynamicGlow->value;
			info[2] = 0.0;
			info[3] = 0;

			VectorSet4(info, info[0], info[1], info[2], info[3]);

			GLSL_SetUniformVec4(&tr.distanceBlurShader[i], UNIFORM_LOCAL0, info);
		}

		{
			vec4_t loc;
			VectorSet4(loc, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value);
			GLSL_SetUniformVec4(&tr.distanceBlurShader[i], UNIFORM_LOCAL1, loc);
		}

		{
			vec4_t viewInfo;
			float zmax = 2048.0;//3072.0;//backEnd.viewParms.zFar;
			float ymax = zmax * tan(backEnd.viewParms.fovY * M_PI / 360.0f);
			float xmax = zmax * tan(backEnd.viewParms.fovX * M_PI / 360.0f);
			float zmin = r_znear->value;
			VectorSet4(viewInfo, zmin, zmax, zmax / zmin, 0.0);
			GLSL_SetUniformVec4(&tr.distanceBlurShader[i], UNIFORM_VIEWINFO, viewInfo);
		}

#if defined(_DEBUG)
		GLSL_FinishGPUShader(&tr.distanceBlurShader[i]);
#endif

		numEtcShaders++;
	}
	*/


	//esharpeningShader
	if (!GLSL_EndLoadGPUShader(&tr.esharpeningShader))
	{
		ri->Error(ERR_FATAL, "Could not load esharpening shader!");
	}

	GLSL_InitUniforms(&tr.esharpeningShader);

	GLSL_BindProgram(&tr.esharpeningShader);

	GLSL_SetUniformInt(&tr.esharpeningShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.esharpeningShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.esharpeningShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.esharpeningShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.esharpeningShader);
#endif

	numEtcShaders++;


	/*
	//esharpeningShader
	if (!GLSL_EndLoadGPUShader(&tr.esharpening2Shader))
	{
		ri->Error(ERR_FATAL, "Could not load esharpening2 shader!");
	}

	GLSL_InitUniforms(&tr.esharpening2Shader);

	GLSL_BindProgram(&tr.esharpening2Shader);

	GLSL_SetUniformInt(&tr.esharpening2Shader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.esharpening2Shader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.esharpening2Shader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.esharpening2Shader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.esharpening2Shader);
#endif

	numEtcShaders++;
	*/


	
	if (!GLSL_EndLoadGPUShader(&tr.fxaaShader))
	{
		ri->Error(ERR_FATAL, "Could not load fxaa shader!");
	}

	GLSL_InitUniforms(&tr.fxaaShader);

	GLSL_BindProgram(&tr.fxaaShader);

	GLSL_SetUniformInt(&tr.fxaaShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.fxaaShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.fxaaShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.fxaaShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.fxaaShader);
#endif

	numEtcShaders++;
	


	if (!GLSL_EndLoadGPUShader(&tr.txaaShader))
	{
		ri->Error(ERR_FATAL, "Could not load txaa shader!");
	}

	GLSL_InitUniforms(&tr.txaaShader);

	GLSL_BindProgram(&tr.txaaShader);

	GLSL_SetUniformInt(&tr.txaaShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.txaaShader, UNIFORM_GLOWMAP, TB_GLOWMAP);

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.txaaShader, UNIFORM_DIMENSIONS, screensize);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.txaaShader);
#endif

	numEtcShaders++;



	//generateNormalMap
	if (!GLSL_EndLoadGPUShader(&tr.generateNormalMapShader))
	{
		ri->Error(ERR_FATAL, "Could not load generateNormalMap shader!");
	}

	GLSL_InitUniforms(&tr.generateNormalMapShader);

	GLSL_BindProgram(&tr.generateNormalMapShader);

	GLSL_SetUniformInt(&tr.generateNormalMapShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.generateNormalMapShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.generateNormalMapShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.generateNormalMapShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.generateNormalMapShader);
#endif

	numEtcShaders++;

	//underwaterShader
	if (!GLSL_EndLoadGPUShader(&tr.underwaterShader))
	{
		ri->Error(ERR_FATAL, "Could not load underwater shader!");
	}

	GLSL_InitUniforms(&tr.underwaterShader);

	GLSL_BindProgram(&tr.underwaterShader);

	GLSL_SetUniformInt(&tr.underwaterShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.underwaterShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.underwaterShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.underwaterShader);
#endif

	numEtcShaders++;




	if (!GLSL_EndLoadGPUShader(&tr.anaglyphShader))
	{
		ri->Error(ERR_FATAL, "Could not load anaglyph shader!");
	}

	GLSL_InitUniforms(&tr.anaglyphShader);

	GLSL_BindProgram(&tr.anaglyphShader);

	GLSL_SetUniformInt(&tr.anaglyphShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.anaglyphShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.anaglyphShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.anaglyphShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

	{
		vec4_t local0;
		VectorSet4(local0, r_trueAnaglyphSeparation->value, r_trueAnaglyphRed->value, r_trueAnaglyphGreen->value, r_trueAnaglyphBlue->value);
		GLSL_SetUniformVec4(&tr.anaglyphShader, UNIFORM_LOCAL0, local0);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.anaglyphShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.skyDomeShader))
	{
		ri->Error(ERR_FATAL, "Could not load skyDomeShader shader!");
	}

	GLSL_InitUniforms(&tr.skyDomeShader);

	GLSL_BindProgram(&tr.skyDomeShader);

	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_LEVELSMAP, TB_LEVELSMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.skyDomeShader, UNIFORM_VIEWINFO, viewInfo);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.skyDomeShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.waterPostForwardShader))
	{
		ri->Error(ERR_FATAL, "Could not load waterPostForward shader!");
	}

	GLSL_InitUniforms(&tr.waterPostForwardShader);

	GLSL_BindProgram(&tr.waterPostForwardShader);

	GLSL_SetUniformInt(&tr.waterPostForwardShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.waterPostForwardShader, UNIFORM_LIGHTMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.waterPostForwardShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.waterPostForwardShader, UNIFORM_DELUXEMAP,   TB_DELUXEMAP);
	GLSL_SetUniformInt(&tr.waterPostForwardShader, UNIFORM_SPECULARMAP, TB_SPECULARMAP);
	GLSL_SetUniformInt(&tr.waterPostForwardShader, UNIFORM_SHADOWMAP, TB_SHADOWMAP);
	GLSL_SetUniformInt(&tr.waterPostForwardShader, UNIFORM_CUBEMAP, TB_CUBEMAP);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.waterPostForwardShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.waterPostForwardShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.waterPostForwardShader);
#endif

	numEtcShaders++;




	if (!GLSL_EndLoadGPUShader(&tr.waterForwardShader))
	{
		ri->Error(ERR_FATAL, "Could not load waterForward shader!");
	}

	GLSL_InitUniforms(&tr.waterForwardShader);

	GLSL_BindProgram(&tr.waterForwardShader);

	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_LIGHTMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_DELUXEMAP, TB_DELUXEMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_SPECULARMAP, TB_SPECULARMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_SHADOWMAP, TB_SHADOWMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_CUBEMAP, TB_CUBEMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_SKYCUBEMAP, TB_SKYCUBEMAP);
	GLSL_SetUniformInt(&tr.waterForwardShader, UNIFORM_SKYCUBEMAPNIGHT, TB_SKYCUBEMAPNIGHT);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.waterForwardShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.waterForwardShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.waterForwardShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.waterForwardFastShader))
	{
		ri->Error(ERR_FATAL, "Could not load waterForwardFast shader!");
	}

	GLSL_InitUniforms(&tr.waterForwardFastShader);

	GLSL_BindProgram(&tr.waterForwardFastShader);

	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_LIGHTMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_DELUXEMAP, TB_DELUXEMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_SPECULARMAP, TB_SPECULARMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_SHADOWMAP, TB_SHADOWMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_CUBEMAP, TB_CUBEMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_SKYCUBEMAP, TB_SKYCUBEMAP);
	GLSL_SetUniformInt(&tr.waterForwardFastShader, UNIFORM_SKYCUBEMAPNIGHT, TB_SKYCUBEMAPNIGHT);

	{
		vec4_t viewInfo;

		float zmax = backEnd.viewParms.zFar;
		float zmin = r_znear->value;

		VectorSet4(viewInfo, zmax / zmin, zmax, 0.0, 0.0);
		//VectorSet4(viewInfo, zmin, zmax, 0.0, 0.0);

		GLSL_SetUniformVec4(&tr.waterForwardFastShader, UNIFORM_VIEWINFO, viewInfo);
	}

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.waterForwardFastShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.waterForwardFastShader);
#endif

	numEtcShaders++;



	if (!GLSL_EndLoadGPUShader(&tr.cloudsShader))
	{
		ri->Error(ERR_FATAL, "Could not load clouds shader!");
	}

	GLSL_InitUniforms(&tr.cloudsShader);

	GLSL_BindProgram(&tr.cloudsShader);

	{
		vec2_t screensize;
		screensize[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screensize[1] = glConfig.vidHeight * r_superSampleMultiplier->value;

		GLSL_SetUniformVec2(&tr.cloudsShader, UNIFORM_DIMENSIONS, screensize);

		//ri->Printf(PRINT_WARNING, "Sent dimensions %f %f.\n", screensize[0], screensize[1]);
	}

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.cloudsShader);
#endif

	numEtcShaders++;



	/*if (!GLSL_EndLoadGPUShader(&tr.hbaoShader))
	{
		ri->Error(ERR_FATAL, "Could not load hbao shader!");
	}

	GLSL_InitUniforms(&tr.hbaoShader);

	GLSL_BindProgram(&tr.hbaoShader);
	GLSL_SetUniformInt(&tr.hbaoShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.hbaoShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.hbaoShader, UNIFORM_NORMALMAP, TB_NORMALMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.hbaoShader);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.hbao2Shader))
	{
		ri->Error(ERR_FATAL, "Could not load hbao2 shader!");
	}

	GLSL_InitUniforms(&tr.hbao2Shader);

	GLSL_BindProgram(&tr.hbao2Shader);
	GLSL_SetUniformInt(&tr.hbao2Shader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
	GLSL_SetUniformInt(&tr.hbao2Shader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.hbao2Shader, UNIFORM_NORMALMAP, TB_NORMALMAP);

#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.hbao2Shader);
#endif

	numEtcShaders++;

	if (!GLSL_EndLoadGPUShader(&tr.hbaoCombineShader))
	{
		ri->Error(ERR_FATAL, "Could not load hbaoCombine shader!");
	}

	GLSL_InitUniforms(&tr.hbaoCombineShader);

	GLSL_BindProgram(&tr.hbaoCombineShader);
	GLSL_SetUniformInt(&tr.hbaoCombineShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.hbaoCombineShader, UNIFORM_NORMALMAP, TB_NORMALMAP);
	
#if defined(_DEBUG)
	GLSL_FinishGPUShader(&tr.hbaoCombineShader);
#endif

	numEtcShaders++;*/

	//
	// UQ1: End Added...
	//

	GLSL_BindProgram(NULL);

	ri->Printf(PRINT_ALL, "loaded %i GLSL shaders (%i light %i etc) in %5.2f seconds\n",
		numLightShaders + numEtcShaders, numLightShaders,
		numEtcShaders, (ri->Milliseconds() - startTime) / 1000.0);
}

void GLSL_ShutdownGPUShaders(void)
{
	int i;

	ri->Printf(PRINT_ALL, "^5------- ^7GLSL_ShutdownGPUShaders^5 -------\n");

	qglDisableVertexAttribArray(ATTR_INDEX_TEXCOORD0);
	qglDisableVertexAttribArray(ATTR_INDEX_TEXCOORD1);
	qglDisableVertexAttribArray(ATTR_INDEX_POSITION);
	qglDisableVertexAttribArray(ATTR_INDEX_POSITION2);
	qglDisableVertexAttribArray(ATTR_INDEX_NORMAL);
	qglDisableVertexAttribArray(ATTR_INDEX_NORMAL2);
	qglDisableVertexAttribArray(ATTR_INDEX_COLOR);
	qglDisableVertexAttribArray(ATTR_INDEX_BONE_INDEXES);
	qglDisableVertexAttribArray(ATTR_INDEX_BONE_WEIGHTS);
	
	GLSL_BindProgram(NULL);

	GLSL_DeleteGPUShader(&tr.textureColorShader);
	
	GLSL_DeleteGPUShader(&tr.weatherShader);

#ifdef __INSTANCED_MODELS__
	GLSL_DeleteGPUShader(&tr.instanceShader);
#endif //__INSTANCED_MODELS__
#ifdef __LODMODEL_INSTANCING__
	GLSL_DeleteGPUShader(&tr.instanceVAOShader);
#endif //__LODMODEL_INSTANCING__
	GLSL_DeleteGPUShader(&tr.occlusionShader);
	GLSL_DeleteGPUShader(&tr.depthAdjustShader);

	GLSL_DeleteGPUShader(&tr.lightAllShader[0]);
	GLSL_DeleteGPUShader(&tr.lightAllShader[1]);
	GLSL_DeleteGPUShader(&tr.lightAllShader[2]);
	GLSL_DeleteGPUShader(&tr.lightAllSplatShader[0]);
	GLSL_DeleteGPUShader(&tr.lightAllSplatShader[1]);
	GLSL_DeleteGPUShader(&tr.lightAllSplatShader[2]);
	GLSL_DeleteGPUShader(&tr.lightAllSplatShader[3]);
	GLSL_DeleteGPUShader(&tr.skyShader);
	GLSL_DeleteGPUShader(&tr.depthPassShader[0]);
	GLSL_DeleteGPUShader(&tr.depthPassShader[1]);
	GLSL_DeleteGPUShader(&tr.depthPassShader[2]);
	GLSL_DeleteGPUShader(&tr.depthPassShader[3]);

	//GLSL_DeleteGPUShader(&tr.sunPassShader);
	//GLSL_DeleteGPUShader(&tr.moonPassShader);
	//GLSL_DeleteGPUShader(&tr.planetPassShader);

	GLSL_DeleteGPUShader(&tr.fireShader);
	GLSL_DeleteGPUShader(&tr.smokeShader);
	GLSL_DeleteGPUShader(&tr.magicParticlesShader);
	GLSL_DeleteGPUShader(&tr.magicParticlesTreeShader);
	GLSL_DeleteGPUShader(&tr.magicParticlesFireFlyShader);
	GLSL_DeleteGPUShader(&tr.portalShader);
	GLSL_DeleteGPUShader(&tr.menuBackgroundShader);

	GLSL_DeleteGPUShader(&tr.shadowFillShader[0]);
	GLSL_DeleteGPUShader(&tr.shadowFillShader[1]);
	GLSL_DeleteGPUShader(&tr.shadowFillShader[2]);
	GLSL_DeleteGPUShader(&tr.shadowFillShader[3]);

	GLSL_DeleteGPUShader(&tr.pshadowShader[0]);
#ifdef __PSHADOW_TESSELLATION__
	GLSL_DeleteGPUShader(&tr.pshadowShader[1]);
	GLSL_DeleteGPUShader(&tr.pshadowShader[2]);
	GLSL_DeleteGPUShader(&tr.pshadowShader[3]);
#endif //__PSHADOW_TESSELLATION__
	//GLSL_DeleteGPUShader(&tr.down4xShader);
	GLSL_DeleteGPUShader(&tr.bokehShader);
	GLSL_DeleteGPUShader(&tr.tonemapShader);

	for (i = 0; i < 2; i++)
		GLSL_DeleteGPUShader(&tr.calclevels4xShader[i]);

	GLSL_DeleteGPUShader(&tr.shadowmaskShader);

	GLSL_DeleteGPUShader(&tr.ssaoShader);
	//GLSL_DeleteGPUShader(&tr.sssShader);
	//GLSL_DeleteGPUShader(&tr.sssBlurShader);
	//GLSL_DeleteGPUShader(&tr.ssdoShader);
	//GLSL_DeleteGPUShader(&tr.ssdoBlurShader);

#ifdef __XYC_SURFACE_SPRITES__
	GLSL_DeleteGPUShader(&tr.surfaceSpriteShader);
#endif //__XYC_SURFACE_SPRITES__

	/*
	for (i = 0; i < 2; i++)
		GLSL_DeleteGPUShader(&tr.depthBlurShader[i]);
	*/

	// UQ1: Added...
	GLSL_DeleteGPUShader(&tr.linearizeDepthShader);
	GLSL_DeleteGPUShader(&tr.darkexpandShader);
	GLSL_DeleteGPUShader(&tr.magicdetailShader);
	GLSL_DeleteGPUShader(&tr.cellShadeShader);
	GLSL_DeleteGPUShader(&tr.paintShader);
	GLSL_DeleteGPUShader(&tr.esharpeningShader);
	//GLSL_DeleteGPUShader(&tr.esharpening2Shader);
	GLSL_DeleteGPUShader(&tr.anaglyphShader);
	GLSL_DeleteGPUShader(&tr.waterForwardShader);
	GLSL_DeleteGPUShader(&tr.waterForwardFastShader);
	GLSL_DeleteGPUShader(&tr.waterPostForwardShader);
	GLSL_DeleteGPUShader(&tr.waterPostShader[0]);
	GLSL_DeleteGPUShader(&tr.waterPostShader[1]);
	GLSL_DeleteGPUShader(&tr.waterPostShader[2]);
	GLSL_DeleteGPUShader(&tr.waterPostShader[3]);
	GLSL_DeleteGPUShader(&tr.waterPostShader[4]);
	GLSL_DeleteGPUShader(&tr.waterPostShader[5]);
	GLSL_DeleteGPUShader(&tr.waterReflectionShader);
	GLSL_DeleteGPUShader(&tr.transparancyPostShader);
	GLSL_DeleteGPUShader(&tr.cloudsShader);
	GLSL_DeleteGPUShader(&tr.foliageShader);
	GLSL_DeleteGPUShader(&tr.grassPatchesShader);
	if (r_foliage->integer)	GLSL_DeleteGPUShader(&tr.grassShader[0]);
	if (r_foliage->integer)	GLSL_DeleteGPUShader(&tr.grassShader[1]);
	GLSL_DeleteGPUShader(&tr.vinesShader);
	GLSL_DeleteGPUShader(&tr.mistShader);
	//GLSL_DeleteGPUShader(&tr.hbaoShader);
	//GLSL_DeleteGPUShader(&tr.hbao2Shader);
	//GLSL_DeleteGPUShader(&tr.hbaoCombineShader);
	GLSL_DeleteGPUShader(&tr.bloomBlurShader);
	GLSL_DeleteGPUShader(&tr.bloomCombineShader);
	//GLSL_DeleteGPUShader(&tr.lensflareShader);
	GLSL_DeleteGPUShader(&tr.multipostShader);
	GLSL_DeleteGPUShader(&tr.anamorphicBlurShader);
	GLSL_DeleteGPUShader(&tr.anamorphicCombineShader);
	GLSL_DeleteGPUShader(&tr.dofShader[0]);
	GLSL_DeleteGPUShader(&tr.dofShader[1]);
	GLSL_DeleteGPUShader(&tr.dofShader[2]);
	GLSL_DeleteGPUShader(&tr.fxaaShader);
	GLSL_DeleteGPUShader(&tr.txaaShader);
	GLSL_DeleteGPUShader(&tr.underwaterShader);
	GLSL_DeleteGPUShader(&tr.volumeLightShader[0]);
	GLSL_DeleteGPUShader(&tr.volumeLightShader[1]);
	GLSL_DeleteGPUShader(&tr.volumeLightShader[2]);
	GLSL_DeleteGPUShader(&tr.volumeLightCombineShader);
	GLSL_DeleteGPUShader(&tr.fastBlurShader);
	//GLSL_DeleteGPUShader(&tr.shadowBlurShader);
	GLSL_DeleteGPUShader(&tr.bloomRaysShader);
	/*GLSL_DeleteGPUShader(&tr.distanceBlurShader[0]);
	GLSL_DeleteGPUShader(&tr.distanceBlurShader[1]);
	GLSL_DeleteGPUShader(&tr.distanceBlurShader[2]);
	GLSL_DeleteGPUShader(&tr.distanceBlurShader[3]);*/
	GLSL_DeleteGPUShader(&tr.dofFocusDepthShader);
	GLSL_DeleteGPUShader(&tr.fogPostShader);
	GLSL_DeleteGPUShader(&tr.colorCorrectionShader);
	GLSL_DeleteGPUShader(&tr.showNormalsShader);
	GLSL_DeleteGPUShader(&tr.showDepthShader);
	GLSL_DeleteGPUShader(&tr.fastLightingShader);
	GLSL_DeleteGPUShader(&tr.deferredLightingShader[0]);
	GLSL_DeleteGPUShader(&tr.deferredLightingShader[1]);
	GLSL_DeleteGPUShader(&tr.deferredLightingShader[2]);
#ifndef __PROCEDURALS_IN_DEFERRED_SHADER__
	GLSL_DeleteGPUShader(&tr.proceduralShader);
#endif //__PROCEDURALS_IN_DEFERRED_SHADER__
	GLSL_DeleteGPUShader(&tr.ssdmShader);
	GLSL_DeleteGPUShader(&tr.ssdmGenerateShader[0]);
	GLSL_DeleteGPUShader(&tr.ssdmGenerateShader[1]);
	//GLSL_DeleteGPUShader(&tr.ssrShader);
	//GLSL_DeleteGPUShader(&tr.ssrCombineShader);
	GLSL_DeleteGPUShader(&tr.testshaderShader);
	GLSL_DeleteGPUShader(&tr.skyDomeShader);
	GLSL_DeleteGPUShader(&tr.generateNormalMapShader);

	GLSL_BindProgram(NULL);
}


void GLSL_BindProgram(shaderProgram_t * program)
{
	if (program)
		program->usageCount++;
	if (glState.currentProgram != program)
	{
#ifdef __DEBUG_GLSL_BINDS__
		if (r_debugBinds->integer == 1 || r_debugBinds->integer == 3)
		{
			GLSL_BINDS_COUNT++;

			char from[256] = { 0 };
			char to[256] = { 0 };

			if (glState.currentProgram)
				strcpy(from, glState.currentProgram->name.c_str());
			else
				strcpy(from, "NULL");

			if (program)
				strcpy(to, program->name.c_str());
			else
				strcpy(to, "NULL");

			ri->Printf(PRINT_WARNING, "Frame: [%i] GLSL_BindProgram: [%i] [%s] -> [%s].\n", SCENE_FRAME_NUMBER, GLSL_BINDS_COUNT, from, to);
		}
#endif //__DEBUG_GLSL_BINDS__

		if (!program)
		{
			if (r_logFile->integer)
			{
				GLimp_LogComment("--- GL_BindNullProgram ---\n");
			}

			if (glState.currentProgram)
			{
				qglUseProgram(0);
				glState.currentProgram = NULL;
			}
			return;
		}

		if (r_logFile->integer)
		{
			// don't just call LogComment, or we will get a call to va() every frame!
			GLimp_LogComment(va("--- GL_BindProgram( %s ) ---\n", program->name));
		}

		//if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
		//	ri->Printf(PRINT_WARNING, "DEBUG: Binding program %s. Last program was %s\n", program->name, glState.currentProgram ? glState.currentProgram->name : "NULL");

		qglUseProgram(program->program);
		glState.currentProgram = program;
		backEnd.pc.c_glslShaderBinds++;

		if (program == &tr.lightAllShader[0] || program == &tr.lightAllShader[1] || program == &tr.lightAllShader[2])
			backEnd.pc.c_lightallBinds++;
		else if (program == &tr.lightAllSplatShader[0] || program == &tr.lightAllSplatShader[1] || program == &tr.lightAllSplatShader[2] || program == &tr.lightAllSplatShader[3])
			backEnd.pc.c_lightallBinds++;
		else if (program == &tr.depthPassShader[0] || program == &tr.depthPassShader[1] || program == &tr.depthPassShader[2] || program == &tr.depthPassShader[3])
			backEnd.pc.c_depthPassBinds++;
		else if (program == &tr.skyShader)
			backEnd.pc.c_skyBinds++;
	}
}

void GLSL_VertexAttribsState(uint32_t stateBits)
{
	uint32_t		diff;

	GLSL_VertexAttribPointers(stateBits);

	diff = stateBits ^ glState.vertexAttribsState;
	if (!diff)
	{
		return;
	}

	if (diff & ATTR_POSITION)
	{
		if (stateBits & ATTR_POSITION)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_POSITION )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_POSITION);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_POSITION )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_POSITION);
		}
	}

	if (diff & ATTR_TEXCOORD0)
	{
		if (stateBits & ATTR_TEXCOORD0)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_TEXCOORD )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_TEXCOORD0);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_TEXCOORD )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_TEXCOORD0);
		}
	}

	if (diff & ATTR_TEXCOORD1)
	{
		if (stateBits & ATTR_TEXCOORD1)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_LIGHTCOORD )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_TEXCOORD1);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_LIGHTCOORD )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_TEXCOORD1);
		}
	}

	if (diff & ATTR_NORMAL)
	{
		if (stateBits & ATTR_NORMAL)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_NORMAL )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_NORMAL);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_NORMAL )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_NORMAL);
		}
	}

	if (diff & ATTR_COLOR)
	{
		if (stateBits & ATTR_COLOR)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_COLOR )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_COLOR);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_COLOR )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_COLOR);
		}
	}

	if (diff & ATTR_POSITION2)
	{
		if (stateBits & ATTR_POSITION2)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_POSITION2 )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_POSITION2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_POSITION2 )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_POSITION2);
		}
	}

	if (diff & ATTR_NORMAL2)
	{
		if (stateBits & ATTR_NORMAL2)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_NORMAL2 )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_NORMAL2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_NORMAL2 )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_NORMAL2);
		}
	}

	if (diff & ATTR_BONE_INDEXES)
	{
		if (stateBits & ATTR_BONE_INDEXES)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_BONE_INDEXES )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_BONE_INDEXES);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_BONE_INDEXES )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_BONE_INDEXES);
		}
	}

	if (diff & ATTR_BONE_WEIGHTS)
	{
		if (stateBits & ATTR_BONE_WEIGHTS)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_BONE_WEIGHTS);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_BONE_WEIGHTS);
		}
	}

	if (diff & ATTR_INSTANCES_TEXCOORD)
	{
		if (stateBits & ATTR_INSTANCES_TEXCOORD)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_INSTANCES_TEXCOORD )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_INSTANCES_TEXCOORD);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_INSTANCES_TEXCOORD )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_INSTANCES_TEXCOORD);
		}
	}

	if (diff & ATTR_INSTANCES_MVP)
	{
		if (stateBits & ATTR_INSTANCES_MVP)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_INSTANCES_MVP )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_INSTANCES_MVP);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_INSTANCES_MVP )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_INSTANCES_MVP);
		}
	}

	if (diff & ATTR_INSTANCES_POSITION)
	{
		if (stateBits & ATTR_INSTANCES_POSITION)
		{
			GLimp_LogComment("qglEnableVertexAttribArray( ATTR_INDEX_INSTANCES_POSITION )\n");
			qglEnableVertexAttribArray(ATTR_INDEX_INSTANCES_POSITION);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArray( ATTR_INDEX_INSTANCES_POSITION )\n");
			qglDisableVertexAttribArray(ATTR_INDEX_INSTANCES_POSITION);
		}
	}

	glState.vertexAttribsState = stateBits;
}

void GLSL_UpdateTexCoordVertexAttribPointers(uint32_t attribBits)
{
	VBO_t *vbo = glState.currentVBO;

	if (attribBits & ATTR_TEXCOORD0)
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_TEXCOORD )\n");

		qglVertexAttribPointer(ATTR_INDEX_TEXCOORD0, 2, GL_FLOAT, 0, vbo->stride_st, BUFFER_OFFSET(vbo->ofs_st + sizeof (vec2_t)* glState.vertexAttribsTexCoordOffset[0]));
	}

	if (attribBits & ATTR_TEXCOORD1)
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_LIGHTCOORD )\n");

		qglVertexAttribPointer(ATTR_INDEX_TEXCOORD1, 2, GL_FLOAT, 0, vbo->stride_st, BUFFER_OFFSET(vbo->ofs_st + sizeof (vec2_t)* glState.vertexAttribsTexCoordOffset[1]));
	}
}

void GLSL_VertexAttribPointers(uint32_t attribBits)
{
	qboolean animated;
	int newFrame, oldFrame;
	VBO_t *vbo = glState.currentVBO;

	if (!vbo)
	{
		ri->Error(ERR_FATAL, "GL_VertexAttribPointers: no VBO bound");
		return;
	}

	// don't just call LogComment, or we will get a call to va() every frame!
	if (r_logFile->integer)
	{
		GLimp_LogComment("--- GL_VertexAttribPointers() ---\n");
	}

	// position/normal are always set in case of animation
	oldFrame = glState.vertexAttribsOldFrame;
	newFrame = glState.vertexAttribsNewFrame;
	animated = glState.vertexAnimation;

	if ((attribBits & ATTR_POSITION) && (!(glState.vertexAttribPointersSet & ATTR_POSITION) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_POSITION )\n");

		qglVertexAttribPointer(ATTR_INDEX_POSITION, 3, GL_FLOAT, 0, vbo->stride_xyz, BUFFER_OFFSET(vbo->ofs_xyz + newFrame * vbo->size_xyz));
		glState.vertexAttribPointersSet |= ATTR_POSITION;
	}

	if ((attribBits & ATTR_TEXCOORD0) && !(glState.vertexAttribPointersSet & ATTR_TEXCOORD0))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_TEXCOORD )\n");

		qglVertexAttribPointer(ATTR_INDEX_TEXCOORD0, 2, GL_FLOAT, 0, vbo->stride_st, BUFFER_OFFSET(vbo->ofs_st + sizeof (vec2_t)* glState.vertexAttribsTexCoordOffset[0]));
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD0;
	}

	if ((attribBits & ATTR_TEXCOORD1) && !(glState.vertexAttribPointersSet & ATTR_TEXCOORD1))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_LIGHTCOORD )\n");

		qglVertexAttribPointer(ATTR_INDEX_TEXCOORD1, 2, GL_FLOAT, 0, vbo->stride_st, BUFFER_OFFSET(vbo->ofs_st + sizeof (vec2_t)* glState.vertexAttribsTexCoordOffset[1]));
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD1;
	}

	if ((attribBits & ATTR_NORMAL) && (!(glState.vertexAttribPointersSet & ATTR_NORMAL) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_NORMAL )\n");

		qglVertexAttribPointer(ATTR_INDEX_NORMAL, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, vbo->stride_normal, BUFFER_OFFSET(vbo->ofs_normal + newFrame * vbo->size_normal));
		glState.vertexAttribPointersSet |= ATTR_NORMAL;
	}

	if ((attribBits & ATTR_COLOR) && !(glState.vertexAttribPointersSet & ATTR_COLOR))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_COLOR )\n");

#ifdef __VBO_PACK_COLOR__
		qglVertexAttribPointer(ATTR_INDEX_COLOR, 4/*1*/, GL_FLOAT, 0, vbo->stride_vertexcolor, BUFFER_OFFSET(vbo->ofs_vertexcolor));
#elif defined(__VBO_HALF_FLOAT_COLOR__)
		qglVertexAttribPointer(ATTR_INDEX_COLOR, 4, GL_HALF_FLOAT, 0, vbo->stride_vertexcolor, BUFFER_OFFSET(vbo->ofs_vertexcolor));
#else //!__VBO_PACK_COLOR__
		qglVertexAttribPointer(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, vbo->stride_vertexcolor, BUFFER_OFFSET(vbo->ofs_vertexcolor));
#endif //__VBO_PACK_COLOR__
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}

	if ((attribBits & ATTR_POSITION2) && (!(glState.vertexAttribPointersSet & ATTR_POSITION2) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_POSITION2 )\n");

		qglVertexAttribPointer(ATTR_INDEX_POSITION2, 3, GL_FLOAT, 0, vbo->stride_xyz, BUFFER_OFFSET(vbo->ofs_xyz + oldFrame * vbo->size_xyz));
		glState.vertexAttribPointersSet |= ATTR_POSITION2;
	}

	if ((attribBits & ATTR_NORMAL2) && (!(glState.vertexAttribPointersSet & ATTR_NORMAL2) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_NORMAL2 )\n");

		qglVertexAttribPointer(ATTR_INDEX_NORMAL2, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, vbo->stride_normal, BUFFER_OFFSET(vbo->ofs_normal + oldFrame * vbo->size_normal));
		glState.vertexAttribPointersSet |= ATTR_NORMAL2;
	}

	if ((attribBits & ATTR_BONE_INDEXES) && !(glState.vertexAttribPointersSet & ATTR_BONE_INDEXES))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_BONE_INDEXES )\n");

		qglVertexAttribPointer(ATTR_INDEX_BONE_INDEXES, 4, GL_FLOAT, 0, vbo->stride_boneindexes, BUFFER_OFFSET(vbo->ofs_boneindexes));
		glState.vertexAttribPointersSet |= ATTR_BONE_INDEXES;
	}

	if ((attribBits & ATTR_BONE_WEIGHTS) && !(glState.vertexAttribPointersSet & ATTR_BONE_WEIGHTS))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_BONE_WEIGHTS )\n");

		qglVertexAttribPointer(ATTR_INDEX_BONE_WEIGHTS, 4, GL_FLOAT, 0, vbo->stride_boneweights, BUFFER_OFFSET(vbo->ofs_boneweights));
		glState.vertexAttribPointersSet |= ATTR_BONE_WEIGHTS;
	}

	if ((attribBits & ATTR_INSTANCES_MVP) && !(glState.vertexAttribPointersSet & ATTR_INSTANCES_MVP))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_INSTANCES_MVP )\n");

		qglVertexAttribPointer(ATTR_INDEX_INSTANCES_MVP, 16, GL_FLOAT, 0, 0, BUFFER_OFFSET(vbo->ofs_instancesMVP));
		//qglVertexAttribDivisor(ATTR_INDEX_INSTANCES_MVP, 1);
		glState.vertexAttribPointersSet |= ATTR_INSTANCES_MVP;
	}

	if ((attribBits & ATTR_INSTANCES_POSITION) && !(glState.vertexAttribPointersSet & ATTR_INSTANCES_POSITION))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_INSTANCES_POSITION )\n");

		qglVertexAttribPointer(ATTR_INDEX_INSTANCES_POSITION, 3, GL_FLOAT, 0, 0, BUFFER_OFFSET(vbo->ofs_instancesPosition));
		//qglVertexAttribDivisor(ATTR_INDEX_INSTANCES_POSITION, 1);
		glState.vertexAttribPointersSet |= ATTR_INSTANCES_POSITION;
	}

	if ((attribBits & ATTR_INSTANCES_TEXCOORD) && !(glState.vertexAttribPointersSet & ATTR_INSTANCES_TEXCOORD))
	{
		GLimp_LogComment("qglVertexAttribPointer( ATTR_INDEX_INSTANCES_TEXCOORD )\n");

		qglVertexAttribPointer(ATTR_INDEX_INSTANCES_TEXCOORD, 2, GL_FLOAT, 0, 0, BUFFER_OFFSET(vbo->ofs_instancesTC));
		glState.vertexAttribPointersSet |= ATTR_INSTANCES_TEXCOORD;
	}
}

void GL_SetDepthRange(GLclampd zNear, GLclampd zFar)
{
#ifdef __INVERSE_DEPTH_BUFFERS__
	qglDepthRange(1.0f - zNear, 1.0f - zFar);
#else //!__INVERSE_DEPTH_BUFFERS__
	qglDepthRange(zNear, zFar);
#endif //__INVERSE_DEPTH_BUFFERS__
}

void GL_SetDepthFunc(GLenum func)
{
#ifdef __INVERSE_DEPTH_BUFFERS__
	switch (func)
	{
	case GL_GREATER:
		qglDepthFunc(GL_LESS);
		break;
	case GL_GEQUAL:
		qglDepthFunc(GL_LEQUAL);
		break;
	case GL_LEQUAL:
		qglDepthFunc(GL_GEQUAL);
		break;
	case GL_LESS:
		qglDepthFunc(GL_GREATER);
		break;
	default:
		qglDepthFunc(func);
		break;
	}
#else //!__INVERSE_DEPTH_BUFFERS__
	qglDepthFunc(func);
#endif //__INVERSE_DEPTH_BUFFERS__
}