/*
===========================================================================
Copyright (C) 2011 Andrei Drexler, Richard Allen, James Canete

This file is part of Reaction source code.

Reaction source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Reaction source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Reaction source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef TR_POSTPROCESS_H
#define TR_POSTPROCESS_H

typedef struct FBO_s FBO_t;
typedef struct image_s image_t;

void RB_ToneMap(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox, int autoExposure);
void RB_BokehBlur(FBO_t *src, vec4i_t srcBox, FBO_t *dst, vec4i_t dstBox, float blur);
void RB_SunRays(FBO_t *srcFbo, vec4i_t srcBox, FBO_t *dstFbo, vec4i_t dstBox);
void RB_GaussianBlur(FBO_t *srcFbo, FBO_t *intermediateFbo, FBO_t *dstFbo, float spread);
void RB_HBlur(FBO_t *srcFbo, FBO_t *dstFbo, float strength);
void RB_VBlur(FBO_t *srcFbo, FBO_t *dstFbo, float strength);
void RB_BloomDownscale(image_t *sourceImage, FBO_t *destFBO);
void RB_BloomDownscale2(FBO_t *sourceFBO, FBO_t *destFBO);
void RB_BloomUpscale(FBO_t *sourceFBO, FBO_t *destFBO);

// UQ1: Added...
void RB_LinearizeDepth(void);
void RB_CreateAnamorphicImage(void);
void RB_Bloom(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_DarkExpand(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_Anamorphic(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
//void RB_LensFlare(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_MagicDetail(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_CellShade(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_Paint(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_Anaglyph(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_FXAA(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_TXAA(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_Underwater(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
qboolean RB_GenerateVolumeLightImage(void);
qboolean RB_VolumetricLight(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_WaterPost(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_TransparancyPost(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSAO(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_HBAO(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_ESharpening(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_ESharpening2(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_DofFocusDepth(void);
void RB_DOF(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox, int direction);
void RB_MultiPost(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_FastBlur(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_DistanceBlur(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox, int direction);
void RB_BloomRays(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_ColorCorrection(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_ShowNormals(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_ShowDepth(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_DeferredLighting(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_Procedural(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSDM(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSDM_Generate(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_ScreenSpaceReflections(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_FogPostShader(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_TestShader(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox, int pass_num);
void RB_SSDO(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSS(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SoftShadows(void);

#ifdef __SSRTGI__
void RB_SSRTGI_BufferSetup(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSRTGI_StencilSetup(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSRTGI_RayTrace(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSRTGI_CopyFilter(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
void RB_SSRTGI_Output(FBO_t *hdrFbo, vec4i_t hdrBox, FBO_t *ldrFbo, vec4i_t ldrBox);
#endif //__SSRTGI__

#endif
