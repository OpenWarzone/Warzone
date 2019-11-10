/*
===========================================================================
Copyright (C) 2006 Kirk Barnes
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_fbo.c
#include "tr_local.h"

/*
=============
R_CheckFBO
=============
*/
qboolean R_CheckFBO(const FBO_t * fbo)
{
	int             code;
	int             id;

	qglGetIntegerv(GL_FRAMEBUFFER_BINDING, &id);
	qglBindFramebuffer(GL_FRAMEBUFFER, fbo->frameBuffer);

	code = qglCheckFramebufferStatus(GL_FRAMEBUFFER);

	qglBindFramebuffer(GL_FRAMEBUFFER, id);

	if(code == GL_FRAMEBUFFER_COMPLETE)
	{
		return qtrue;
	}

	// an error occured
	switch (code)
	{
		case GL_FRAMEBUFFER_COMPLETE:
			break;

		case GL_FRAMEBUFFER_UNSUPPORTED:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Unsupported framebuffer format\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_UNDEFINED:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Default framebuffer was checked, but does not exist\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete attachment\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, no attachments attached\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, mismatched multisampling values\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, mismatched layer targets\n",
					  fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer\n", fbo->name);
			break;

		default:
			ri->Printf(PRINT_WARNING, "R_CheckFBO: (%s) unknown error 0x%X\n", fbo->name, code);
			//ri->Error(ERR_FATAL, "R_CheckFBO: (%s) unknown error 0x%X", fbo->name, code);
			//assert(0);
			break;
	}

	return qfalse;
}

/*
============
FBO_Create
============
*/
FBO_t          *FBO_Create(const char *name, int width, int height)
{
	FBO_t          *fbo;

	if(strlen(name) >= MAX_QPATH)
	{
		ri->Error(ERR_DROP, "FBO_Create: \"%s\" is too long", name);
	}

	if(width <= 0 || width > glRefConfig.maxRenderbufferSize)
	{
		ri->Error(ERR_DROP, "FBO_Create: bad width %i", width);
	}

	if(height <= 0 || height > glRefConfig.maxRenderbufferSize)
	{
		ri->Error(ERR_DROP, "FBO_Create: bad height %i", height);
	}

	if(tr.numFBOs == MAX_FBOS)
	{
		ri->Error(ERR_DROP, "FBO_Create: MAX_FBOS hit");
	}

	fbo = tr.fbos[tr.numFBOs] = (FBO_t *)ri->Hunk_Alloc(sizeof(*fbo), h_low);
	Q_strncpyz(fbo->name, name, sizeof(fbo->name));
	fbo->index = tr.numFBOs++;
	fbo->width = width;
	fbo->height = height;

	qglGenFramebuffers(1, &fbo->frameBuffer);

	return fbo;
}

void FBO_CreateBuffer(FBO_t *fbo, int format, int index, int multisample)
{
	uint32_t *pRenderBuffer;
	GLenum attachment;
	qboolean absent;

	switch(format)
	{
		case GL_RGB:
		case GL_RGBA:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_RGB32F:
		case GL_RGBA32F:
			fbo->colorFormat = format;
			pRenderBuffer = &fbo->colorBuffers[index];
			attachment = GL_COLOR_ATTACHMENT0 + index;
			break;

		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
			fbo->depthFormat = format;
			pRenderBuffer = &fbo->depthBuffer;
			attachment = GL_DEPTH_ATTACHMENT;
			break;

		case GL_STENCIL_INDEX:
		case GL_STENCIL_INDEX1:
		case GL_STENCIL_INDEX4:
		case GL_STENCIL_INDEX8:
		case GL_STENCIL_INDEX16:
			fbo->stencilFormat = format;
			pRenderBuffer = &fbo->stencilBuffer;
			attachment = GL_STENCIL_ATTACHMENT;
			break;

		case GL_DEPTH_STENCIL:
		case GL_DEPTH24_STENCIL8:
			fbo->packedDepthStencilFormat = format;
			pRenderBuffer = &fbo->packedDepthStencilBuffer;
			attachment = 0; // special for stencil and depth
			break;

		default:
			ri->Printf(PRINT_WARNING, "FBO_CreateBuffer: invalid format %d\n", format);
			return;
	}

	absent = (qboolean)(*pRenderBuffer == 0);
	if (absent)
		qglGenRenderbuffers(1, pRenderBuffer);

	qglBindRenderbuffer(GL_RENDERBUFFER, *pRenderBuffer);
	if (multisample)
	{
		qglRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, format, fbo->width, fbo->height);
	}
	else
	{
		qglRenderbufferStorage(GL_RENDERBUFFER, format, fbo->width, fbo->height);
	}

	if(absent)
	{
		if (attachment == 0)
		{
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,   GL_RENDERBUFFER, *pRenderBuffer);
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *pRenderBuffer);
		}
		else
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, *pRenderBuffer);
	}
}


/*
=================
R_AttachFBOTexture1D
=================
*/
void R_AttachFBOTexture1D(int texId, int index)
{
	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri->Printf(PRINT_WARNING, "R_AttachFBOTexture1D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_1D, texId, 0);
}

/*
=================
R_AttachFBOTexture2D
=================
*/
void R_AttachFBOTexture2D(int target, int texId, int index)
{
	if(target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
	{
		ri->Printf(PRINT_WARNING, "R_AttachFBOTexture2D: invalid target %i\n", target);
		return;
	}

	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri->Printf(PRINT_WARNING, "R_AttachFBOTexture2D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, texId, 0);
}

/*
=================
R_AttachFBOTexture3D
=================
*/
void R_AttachFBOTexture3D(int texId, int index, int zOffset)
{
	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri->Printf(PRINT_WARNING, "R_AttachFBOTexture3D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_3D, texId, 0, zOffset);
}

/*
=================
R_AttachFBOTextureDepth
=================
*/
void R_AttachFBOTextureDepth(int texId)
{
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

/*
=================
R_AttachFBOTexturePackedDepthStencil
=================
*/
void R_AttachFBOTexturePackedDepthStencil(int texId)
{
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

void FBO_AttachTextureImage(image_t *img, int index)
{
	if (!glState.currentFBO)
	{
		ri->Printf(PRINT_WARNING, "FBO: attempted to attach a texture image with no FBO bound!\n");
		return;
	}

	if (glState.currentFBO)
	{// UQ1: Can we skip this bind?
		if (glState.currentFBO->attachedImages[index] == img->texnum)
			return;

		glState.currentFBO->attachedImages[index] = img->texnum;
	}

	R_AttachFBOTexture2D(GL_TEXTURE_2D, img->texnum, index);
	glState.currentFBO->colorImage[index] = img;
	glState.currentFBO->colorBuffers[index] = img->texnum;
}

void FBO_SetupDrawBuffers()
{
	if (!glState.currentFBO)
	{
		ri->Printf(PRINT_WARNING, "FBO: attempted to attach a texture image with no FBO bound!\n");
		return;
	}

	FBO_t *currentFBO = glState.currentFBO;
	int numBuffers = 0;
	GLenum bufs[8];

	while ( currentFBO->colorBuffers[numBuffers] != 0 )
	{
		numBuffers++;
	}

	if ( numBuffers == 0 )
	{
		qglDrawBuffer (GL_NONE);
	}
	else
	{
		for ( int i = 0; i < numBuffers; i++ )
		{
			bufs[i] = GL_COLOR_ATTACHMENT0 + i;
		}

		qglDrawBuffers (numBuffers, bufs);
	}
}

/*
============
FBO_Bind
============
*/
void FBO_Bind(FBO_t * fbo)
{
	if (glState.currentFBO == fbo)
		return;

	//if (fbo == NULL && !ALLOW_NULL_FBO_BIND && tr.world && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
	//	return;
		
#ifdef __DEBUG_FBO_BINDS__
	if (r_debugBinds->integer == 2 || r_debugBinds->integer == 3)
	{
		FBO_BINDS_COUNT++;

		char from[256] = { 0 };
		char to[256] = { 0 };

		if (glState.currentFBO)
			strcpy(from, glState.currentFBO->name);
		else
			strcpy(from, "NULL");

		if (fbo)
			strcpy(to, fbo->name);
		else
			strcpy(to, "NULL");

		ri->Printf(PRINT_WARNING, "Frame: [%i] FBO_Bind: [%i] [%s] -> [%s].\n", SCENE_FRAME_NUMBER, FBO_BINDS_COUNT, from, to);
	}
#endif //__DEBUG_FBO_BINDS__

	if (r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		if (fbo)
			GLimp_LogComment(va("--- FBO_Bind( %s ) ---\n", fbo->name));
		else
			GLimp_LogComment("--- FBO_Bind ( NULL ) ---\n");
	}

	if (!fbo)
	{
		//if (ALLOW_NULL_FBO_BIND || !tr.world || (backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			qglBindFramebuffer(GL_FRAMEBUFFER, 0);
			//qglBindRenderbuffer(GL_RENDERBUFFER, 0);
			glState.currentFBO = NULL;
		}
		
		return;
	}
		
	qglBindFramebuffer(GL_FRAMEBUFFER, fbo->frameBuffer);

	/*
	   if(fbo->colorBuffers[0])
	   {
	   qglBindRenderbuffer(GL_RENDERBUFFER, fbo->colorBuffers[0]);
	   }
	 */

	/*
	   if(fbo->depthBuffer)
	   {
	   qglBindRenderbuffer(GL_RENDERBUFFER, fbo->depthBuffer);
	   qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->depthBuffer);
	   }
	 */

	glState.currentFBO = fbo;
}

/*
============
FBO_Init
============
*/

extern void GLSL_AttachTextures( void );
extern void GLSL_AttachRenderDepthTextures(void);
extern void GLSL_AttachGlowTextures(void);
extern void GLSL_AttachGenericTextures(void);
extern void GLSL_AttachWaterTextures(void);
extern void GLSL_AttachTransparancyTextures(void);
extern void GLSL_AttachRenderGUITextures(void);

void FBO_Init(void)
{
	int             i;
	// int             width, height, hdrFormat, multisample;
	int             hdrFormat, multisample;

	ri->Printf(PRINT_ALL, "^5------- ^7FBO_Init^5 -------\n");

	tr.numFBOs = 0;

	GL_CheckErrors();

	R_IssuePendingRenderCommands();

/*	if(glRefConfig.textureNonPowerOfTwo)
	{
		width = glConfig.vidWidth * r_superSampleMultiplier->value;
		height = glConfig.vidHeight * r_superSampleMultiplier->value;
	}
	else
	{
		width = NextPowerOfTwo(glConfig.vidWidth * r_superSampleMultiplier->value);
		height = NextPowerOfTwo(glConfig.vidHeight * r_superSampleMultiplier->value);
	} */

	hdrFormat = GL_RGBA8;

	if (r_hdr->integer >= 2)
	{
		hdrFormat = GL_RGB32F;
	}
	else if (r_hdr->integer)
	{
		hdrFormat = GL_RGB16F;
	}

	qglGetIntegerv(GL_MAX_SAMPLES, &multisample);

	if (r_ext_framebuffer_multisample->integer < multisample)
	{
		multisample = r_ext_framebuffer_multisample->integer;
	}

	if (multisample < 2)
		multisample = 0;

	if (multisample != r_ext_framebuffer_multisample->integer)
	{
		ri->Cvar_SetValue("r_ext_framebuffer_multisample", (float)multisample);
	}

	//
	// UQ1's waterPosition FBO...
	//
	{
		tr.waterFbo = FBO_Create("_waterPosition", tr.waterPositionMapImage->width, tr.waterPositionMapImage->height);
		FBO_Bind(tr.waterFbo);
		FBO_AttachTextureImage(tr.waterPositionMapImage, 0);
		R_CheckFBO(tr.waterFbo);
	}

	//
	// UQ1's waterPosition FBO...
	//
	{
		tr.transparancyFbo = FBO_Create("_transparancy", tr.transparancyMapImage->width, tr.transparancyMapImage->height);
		FBO_Bind(tr.transparancyFbo);
		FBO_AttachTextureImage(tr.transparancyMapImage, 0);
		R_CheckFBO(tr.transparancyFbo);
	}

	//
	// UQ1's Generic FBO...
	//
	{
		tr.genericFbo = FBO_Create("_generic", tr.genericFBOImage->width, tr.genericFBOImage->height);
		FBO_Bind(tr.genericFbo);
		FBO_AttachTextureImage(tr.genericFBOImage, 0);
		R_CheckFBO(tr.genericFbo);
	}

	//
	// UQ1's Generic FBO2...
	//
	{
		tr.genericFbo2 = FBO_Create("_generic2", tr.genericFBO2Image->width, tr.genericFBO2Image->height);
		FBO_Bind(tr.genericFbo2);
		FBO_AttachTextureImage(tr.genericFBO2Image, 0);
		R_CheckFBO(tr.genericFbo2);
	}

	//
	// UQ1's Generic FBO3...
	//
	{
		tr.genericFbo3 = FBO_Create("_generic3", tr.genericFBO3Image->width, tr.genericFBO3Image->height);
		FBO_Bind(tr.genericFbo3);
		FBO_AttachTextureImage(tr.genericFBO3Image, 0);
		R_CheckFBO(tr.genericFbo3);
	}

	//
	// UQ1's SSDM FBO...
	//
	{
		tr.ssdmFbo = FBO_Create("_ssdmFbo", tr.ssdmImage->width, tr.ssdmImage->height);
		FBO_Bind(tr.ssdmFbo);
		FBO_AttachTextureImage(tr.ssdmImage, 0);
		R_CheckFBO(tr.ssdmFbo);
	}

	//
	// UQ1's SSAO FBO...
	//
	{
		tr.ssaoFbo = FBO_Create("_ssaoFbo", tr.ssaoImage->width, tr.ssaoImage->height);
		FBO_Bind(tr.ssaoFbo);
		FBO_AttachTextureImage(tr.ssaoImage, 0);
		R_CheckFBO(tr.ssaoFbo);
	}

#ifdef __SSDO__
	//
	// UQ1's SSDO FBO1...
	//
	{
		tr.ssdoFbo1 = FBO_Create("_ssdoFbo1", tr.ssdoImage1->width, tr.ssdoImage1->height);
		FBO_Bind(tr.ssdoFbo1);
		FBO_AttachTextureImage(tr.ssdoImage1, 0);
		FBO_AttachTextureImage(tr.ssdoIlluminationImage, 1);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.ssdoFbo1);
	}

	//
	// UQ1's SSDO FBO2...
	//
	{
		tr.ssdoFbo2 = FBO_Create("_ssdoFbo2", tr.ssdoImage2->width, tr.ssdoImage2->height);
		FBO_Bind(tr.ssdoFbo2);
		FBO_AttachTextureImage(tr.ssdoImage2, 0);
		R_CheckFBO(tr.ssdoFbo2);
	}
#endif //__SSDO__

	//
	// UQ1's SSS FBO1...
	//
	/*
	{
		tr.sssFbo1 = FBO_Create("_sssFbo1", tr.sssImage1->width, tr.sssImage1->height);
		FBO_Bind(tr.sssFbo1);
		FBO_AttachTextureImage(tr.sssImage1, 0);
		R_CheckFBO(tr.sssFbo1);
	}

	//
	// UQ1's SSS FBO2...
	//
	{
		tr.sssFbo2 = FBO_Create("_sssFbo2", tr.sssImage2->width, tr.sssImage2->height);
		FBO_Bind(tr.sssFbo2);
		FBO_AttachTextureImage(tr.sssImage2, 0);
		R_CheckFBO(tr.sssFbo2);
	}
	*/

	//
	// Bloom VBO's...
	//
	{
		tr.bloomRenderFBO[0] = FBO_Create("_bloom0", tr.bloomRenderFBOImage[0]->width, tr.bloomRenderFBOImage[0]->height);
		FBO_Bind(tr.bloomRenderFBO[0]);
		FBO_AttachTextureImage(tr.bloomRenderFBOImage[0], 0);
		R_CheckFBO(tr.bloomRenderFBO[0]);


		tr.bloomRenderFBO[1] = FBO_Create("_bloom1", tr.bloomRenderFBOImage[1]->width, tr.bloomRenderFBOImage[1]->height);
		FBO_Bind(tr.bloomRenderFBO[1]);
		FBO_AttachTextureImage(tr.bloomRenderFBOImage[1], 0);
		R_CheckFBO(tr.bloomRenderFBO[1]);


		tr.bloomRenderFBO[2] = FBO_Create("_bloom2", tr.bloomRenderFBOImage[2]->width, tr.bloomRenderFBOImage[2]->height);
		FBO_Bind(tr.bloomRenderFBO[2]);
		FBO_AttachTextureImage(tr.bloomRenderFBOImage[2], 0);
		R_CheckFBO(tr.bloomRenderFBO[2]);
	}

	//
	// Bloom Area VBO's...
	//
	for (int i = 0; i < 8; i++)
	{
		tr.bloomAreaRenderFBO[i] = FBO_Create(va("_bloomArea%i", i), tr.bloomAreaRenderFBOImage[i]->width, tr.bloomAreaRenderFBOImage[i]->height);
		FBO_Bind(tr.bloomAreaRenderFBO[i]);
		FBO_AttachTextureImage(tr.bloomAreaRenderFBOImage[i], 0);
		R_CheckFBO(tr.bloomAreaRenderFBO[i]);
	}

	{
		tr.bloomAreaRenderFinalFBO = FBO_Create("_bloomAreaFinal", tr.bloomAreaRenderFinalFBOImage->width, tr.bloomAreaRenderFinalFBOImage->height);
		FBO_Bind(tr.bloomAreaRenderFinalFBO);
		FBO_AttachTextureImage(tr.bloomAreaRenderFinalFBOImage, 0);
		R_CheckFBO(tr.bloomAreaRenderFinalFBO);
	}

	//
	// Anamorphic FBO's...
	//
	{
		tr.anamorphicRenderFBO = FBO_Create("_anamorphic", tr.anamorphicRenderFBOImage->width, tr.anamorphicRenderFBOImage->height);
		FBO_Bind(tr.anamorphicRenderFBO);
		FBO_AttachTextureImage(tr.anamorphicRenderFBOImage, 0);
		R_CheckFBO(tr.anamorphicRenderFBO);
	}

	//
	// UQ1's Volumetric FBO...
	//
	{
		tr.volumetricFbo = FBO_Create("_volumetric", tr.volumetricFBOImage->width, tr.volumetricFBOImage->height);
		FBO_Bind(tr.volumetricFbo);
		FBO_AttachTextureImage(tr.volumetricFBOImage, 0);
		R_CheckFBO(tr.volumetricFbo);
	}

	//
	// UQ1's Bloom Rays FBO...
	//
	{
		tr.bloomRaysFbo = FBO_Create("_bloomRays", tr.bloomRaysFBOImage->width, tr.bloomRaysFBOImage->height);
		FBO_Bind(tr.bloomRaysFbo);
		FBO_AttachTextureImage(tr.bloomRaysFBOImage, 0);
		R_CheckFBO(tr.bloomRaysFbo);
	}

	//
	// Water Reflection FBO...
	//
	{
		tr.waterReflectionRenderFBO = FBO_Create("_waterReflection", tr.waterReflectionRenderImage->width, tr.waterReflectionRenderImage->height);
		FBO_Bind(tr.waterReflectionRenderFBO);
		FBO_AttachTextureImage(tr.waterReflectionRenderImage, 0);
		R_CheckFBO(tr.waterReflectionRenderFBO);
	}

	//
	// TXAA FBO...
	//
	{
		tr.txaaPreviousFBO = FBO_Create("_txaa", tr.txaaPreviousImage->width, tr.txaaPreviousImage->height);
		FBO_Bind(tr.txaaPreviousFBO);
		FBO_AttachTextureImage(tr.txaaPreviousImage, 0);
		R_CheckFBO(tr.txaaPreviousFBO);
	}

#ifdef __RENDER_HEIGHTMAP__
	//
	// Heightmap render FBO...
	//
	{
		tr.renderHeightmapFbo = FBO_Create("_renderHeightmap", tr.HeightmapImage->width, tr.HeightmapImage->height);
		FBO_Bind(tr.renderHeightmapFbo);
		
		//qglDrawBuffer(GL_NONE);
		//qglReadBuffer(GL_NONE);
		//R_AttachFBOTextureDepth(tr.HeightmapImage->texnum);

		GLSL_AttachRenderDepthTextures();
		R_AttachFBOTextureDepth(tr.HeightmapImage->texnum);

		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderHeightmapFbo);

		qglClearColor(1, 1, 1, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		qglClearDepth(1.0f);
	}
#endif //__RENDER_HEIGHTMAP__
	
	
#if 0
	// only create a render FBO if we need to resolve MSAA or do HDR
	// otherwise just render straight to the screen (tr.renderFbo = NULL)
	if (multisample)
	{
		tr.renderFbo = FBO_Create("_render", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderFbo);

		FBO_CreateBuffer(tr.renderFbo, hdrFormat, 0, multisample);
		FBO_CreateBuffer(tr.renderFbo, hdrFormat, 1, multisample);
		FBO_CreateBuffer(tr.renderFbo, GL_DEPTH_COMPONENT24, 0, multisample);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.renderFbo);

		tr.msaaResolveFbo = FBO_Create("_msaaResolve", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.msaaResolveFbo);

		//FBO_CreateBuffer(tr.msaaResolveFbo, hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.renderImage, 0);
		FBO_AttachTextureImage(tr.glowImage, 1);
		FBO_AttachTextureImage(tr.renderNormalImage, 2);
		FBO_AttachTextureImage(tr.renderPositionMapImage, 3);
		if (r_normalMappingReal->integer)
		{
			FBO_AttachTextureImage(tr.renderNormalDetailedImage, 4);
		}

		//FBO_CreateBuffer(tr.msaaResolveFbo, GL_DEPTH_COMPONENT24, 0, 0);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.msaaResolveFbo);
	}
	else
#endif
	{
		tr.renderFbo = FBO_Create("_render", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderFbo);
		//FBO_CreateBuffer(tr.renderFbo, hdrFormat, 0, 0);
		GLSL_AttachTextures();
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderFbo);

		tr.renderDepthFbo = FBO_Create("_renderDepth", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderDepthFbo);
		//qglDrawBuffer(GL_NONE);
		//qglReadBuffer(GL_NONE);
		GLSL_AttachRenderDepthTextures();
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderDepthFbo);

		tr.renderNoDepthFbo = FBO_Create("_renderNoDepth", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderNoDepthFbo);
		//qglDrawBuffer(GL_NONE);
		//qglReadBuffer(GL_NONE);
		GLSL_AttachRenderDepthTextures();
		//R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderNoDepthFbo);

		tr.renderGUIFbo = FBO_Create("_renderGUI", tr.renderGUIImage->width, tr.renderGUIImage->height);
		FBO_Bind(tr.renderGUIFbo);
		GLSL_AttachRenderGUITextures();
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderGUIFbo);
	}

	// clear render buffer
	// this fixes the corrupt screen bug with r_hdr 1 on older hardware
	if (tr.renderFbo)
	{
		FBO_Bind(tr.renderFbo);
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		//FBO_Bind(NULL);
	}

	if (tr.waterFbo)
	{
		FBO_Bind(tr.waterFbo);
		qglClearColor( 0, 0, 0, 0 );
		qglClear( GL_COLOR_BUFFER_BIT );
		//FBO_Bind(NULL);
	}

	//FBO_Bind(NULL);

	// glow buffers
	{
		for ( int i = 0; i < ARRAY_LEN(tr.glowImageScaled); i++ )
		{
			tr.glowFboScaled[i] = FBO_Create (va ("*glowScaled%d", i), tr.glowImageScaled[i]->width, tr.glowImageScaled[i]->height);

			FBO_Bind (tr.glowFboScaled[i]);

			FBO_AttachTextureImage (tr.glowImageScaled[i], 0);

			FBO_SetupDrawBuffers();

			R_CheckFBO (tr.glowFboScaled[i]);

			if (tr.glowFboScaled[i])
			{
				FBO_Bind(tr.glowFboScaled[i]);
				qglClearColor(0, 0, 0, 0);
				qglClear(GL_COLOR_BUFFER_BIT);
				//FBO_Bind(NULL);
			}
		}
	}

	//if (r_drawSunRays->integer)
	{
		tr.sunRaysFbo = FBO_Create("_sunRays", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.sunRaysFbo);

		FBO_AttachTextureImage(tr.sunRaysImage, 0);

		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.sunRaysFbo);
	}

	// FIXME: Don't use separate color/depth buffers for a shadow buffer
#if MAX_DRAWN_PSHADOWS > 0
	if (tr.pshadowMaps[0] != NULL)
	{
		for( i = 0; i < MAX_DRAWN_PSHADOWS; i++)
		{
			tr.pshadowFbos[i] = FBO_Create(va("_shadowmap%i", i), tr.pshadowMaps[i]->width, tr.pshadowMaps[i]->height);
			FBO_Bind(tr.pshadowFbos[i]);

			FBO_CreateBuffer(tr.pshadowFbos[i], GL_DEPTH_COMPONENT24, 0, 0);
			R_AttachFBOTextureDepth(tr.pshadowMaps[i]->texnum);

			FBO_SetupDrawBuffers();

			R_CheckFBO(tr.pshadowFbos[i]);
		}
	}
#endif

	if (tr.sunShadowDepthImage[0] != NULL)
	{
		for ( i = 0; i < 5; i++)
		{
			tr.sunShadowFbo[i] = FBO_Create(va("_sunshadowmap%i", i), tr.sunShadowDepthImage[i]->width, tr.sunShadowDepthImage[i]->height);
			FBO_Bind(tr.sunShadowFbo[i]);

			qglDrawBuffer(GL_NONE);
			qglReadBuffer(GL_NONE);

			R_AttachFBOTextureDepth(tr.sunShadowDepthImage[i]->texnum);

			FBO_SetupDrawBuffers();

			R_CheckFBO(tr.sunShadowFbo[i]);
		}

		tr.screenShadowFbo = FBO_Create("_screenshadow", tr.screenShadowImage->width, tr.screenShadowImage->height);
		
		FBO_Bind(tr.screenShadowFbo);
		FBO_AttachTextureImage(tr.screenShadowImage, 0);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.screenShadowFbo);

		tr.screenShadowBlurTempFbo = FBO_Create("_screenshadowBlurTemp", tr.screenShadowImage->width, tr.screenShadowImage->height);
		FBO_Bind(tr.screenShadowBlurTempFbo);
		FBO_AttachTextureImage(tr.screenShadowBlurTempImage, 0);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.screenShadowBlurTempFbo);

		tr.screenShadowBlurFbo = FBO_Create("_screenshadowBlur", tr.screenShadowImage->width, tr.screenShadowImage->height);
		FBO_Bind(tr.screenShadowBlurFbo);
		FBO_AttachTextureImage(tr.screenShadowBlurImage, 0);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.screenShadowBlurFbo);
	}

	for (i = 0; i < 2; i++)
	{
		tr.textureScratchFbo[i] = FBO_Create(va("_texturescratch%d", i), tr.textureScratchImage[i]->width, tr.textureScratchImage[i]->height);
		FBO_Bind(tr.textureScratchFbo[i]);

		//FBO_CreateBuffer(tr.textureScratchFbo[i], GL_RGBA8, 0, 0);
		FBO_AttachTextureImage(tr.textureScratchImage[i], 0);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.textureScratchFbo[i]);
	}

	{
		tr.calcLevelsFbo = FBO_Create("_calclevels", tr.calcLevelsImage->width, tr.calcLevelsImage->height);
		FBO_Bind(tr.calcLevelsFbo);

		//FBO_CreateBuffer(tr.calcLevelsFbo, hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.calcLevelsImage, 0);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.calcLevelsFbo);
	}

	{
		tr.targetLevelsFbo = FBO_Create("_targetlevels", tr.targetLevelsImage->width, tr.targetLevelsImage->height);
		FBO_Bind(tr.targetLevelsFbo);

		//FBO_CreateBuffer(tr.targetLevelsFbo, hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.targetLevelsImage, 0);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.targetLevelsFbo);
	}

	for (i = 0; i < 2; i++)
	{
		tr.quarterFbo[i] = FBO_Create(va("_quarter%d", i), tr.quarterImage[i]->width, tr.quarterImage[i]->height);
		FBO_Bind(tr.quarterFbo[i]);

		//FBO_CreateBuffer(tr.quarterFbo[i], hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.quarterImage[i], 0);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.quarterFbo[i]);
	}

	if (tr.renderCubeImage != NULL)
	{
		tr.renderCubeFbo = FBO_Create("_renderCubeFbo", tr.renderCubeImage->width, tr.renderCubeImage->height);
		FBO_Bind(tr.renderCubeFbo);
		
		//FBO_AttachTextureImage(tr.renderCubeImage, 0);
		R_AttachFBOTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, tr.renderCubeImage->texnum, 0);
		glState.currentFBO->colorImage[0] = tr.renderCubeImage;
		glState.currentFBO->colorBuffers[0] = tr.renderCubeImage->texnum;

		FBO_CreateBuffer(tr.renderCubeFbo, GL_DEPTH_COMPONENT24, 0, 0);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.renderCubeFbo);
	}

	if (tr.renderSkyImage != NULL)
	{
		tr.renderSkyFbo = FBO_Create("_renderSkyFbo", tr.renderSkyImage->width, tr.renderSkyImage->height);
		FBO_Bind(tr.renderSkyFbo);

		//FBO_AttachTextureImage(tr.renderCubeImage, 0);
		R_AttachFBOTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, tr.renderSkyImage->texnum, 0);
		glState.currentFBO->colorImage[0] = tr.renderSkyImage;
		glState.currentFBO->colorBuffers[0] = tr.renderSkyImage->texnum;

		FBO_CreateBuffer(tr.renderSkyFbo, GL_DEPTH_COMPONENT24, 0, 0);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.renderSkyFbo);
	}

	/*
	{
		tr.awesomiumuiFbo = FBO_Create("_awesomiumui", tr.awesomiumuiImage->width, tr.awesomiumuiImage->height);
		FBO_Bind(tr.awesomiumuiFbo);

		FBO_AttachTextureImage(tr.awesomiumuiImage, 0);

		FBO_SetupDrawBuffers();

		R_CheckFBO(tr.awesomiumuiFbo);
	}
	*/


	{
		tr.renderGlowFbo = FBO_Create("_renderGlow", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderGlowFbo);

		//FBO_CreateBuffer(tr.renderFbo, hdrFormat, 0, 0);
		GLSL_AttachGlowTextures();
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderGlowFbo);

		FBO_Bind(tr.renderGlowFbo);
		qglClearColor(1, 0, 0.5, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//FBO_Bind(NULL);


		tr.renderDetailFbo = FBO_Create("_renderDetail", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderDetailFbo);

		//FBO_CreateBuffer(tr.renderDetailFbo, hdrFormat, 0, 0);
		GLSL_AttachGenericTextures();
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderDetailFbo);

		FBO_Bind(tr.renderDetailFbo);
		qglClearColor(1, 0, 0.5, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//FBO_Bind(NULL);


		tr.renderWaterFbo = FBO_Create("_renderWater", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderWaterFbo);

		GLSL_AttachWaterTextures();
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		//R_AttachFBOTextureDepth(tr.waterDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderWaterFbo);

		FBO_Bind(tr.renderWaterFbo);
		qglClearColor(1, 1, 1, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//FBO_Bind(NULL);


		tr.renderTransparancyFbo = FBO_Create("_renderTransparancy", tr.renderForcefieldDepthImage->width, tr.renderForcefieldDepthImage->height);
		FBO_Bind(tr.renderTransparancyFbo);

		GLSL_AttachTransparancyTextures();
		R_AttachFBOTextureDepth(tr.renderForcefieldDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderTransparancyFbo);

		FBO_Bind(tr.renderTransparancyFbo);
		qglClearColor(1, 1, 1, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//FBO_Bind(NULL);


		tr.renderPshadowsFbo = FBO_Create("_renderPshadows", tr.renderPshadowsImage->width, tr.renderPshadowsImage->height);
		FBO_Bind(tr.renderPshadowsFbo);
		FBO_AttachTextureImage(tr.renderPshadowsImage, 0);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.renderPshadowsFbo);
	}

	//
	// UQ1's genericDepth FBO (copy depth image to genericDepthImage)...
	//
	{
		tr.genericDepthFbo = FBO_Create("_genericDepth", tr.genericDepthImage->width, tr.genericDepthImage->height);
		FBO_Bind(tr.genericDepthFbo);
		FBO_AttachTextureImage(tr.genericDepthImage, 0);
		//R_AttachFBOTextureDepth(tr.genericDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.genericDepthFbo);
	}

	//
	// UQ1's depthAdjust FBO (for depth buffer modification)...
	//
	{
		tr.depthAdjustFbo = FBO_Create("_depthAdjust", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.depthAdjustFbo);
		//FBO_AttachTextureImage(tr.renderDepthImage, 0);
		FBO_AttachTextureImage(tr.dummyImage, 0);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.depthAdjustFbo);

		FBO_Bind(tr.depthAdjustFbo);
		qglClearColor(0.0, 0.0, 0.0, 1.0);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	//
	// UQ1's linearizeDepths FBO (for depth buffer pre-linearization)...
	//
	{
		tr.linearizeDepthFbo = FBO_Create("_linearizeDepth", tr.linearDepthImage512->width, tr.linearDepthImage512->height);
		FBO_Bind(tr.linearizeDepthFbo);
		FBO_AttachTextureImage(tr.linearDepthImage512, 0);
		FBO_AttachTextureImage(tr.linearDepthImage2048, 1);
		FBO_AttachTextureImage(tr.linearDepthImage4096, 2);
		FBO_AttachTextureImage(tr.linearDepthImage8192, 3);
		FBO_AttachTextureImage(tr.linearDepthImageZfar, 4);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.linearizeDepthFbo);
	}

	//
	// UQ1's dofFocusDepth FBO...
	//
	{
		tr.dofFocusDepthFbo = FBO_Create("_dofFocusDepth", tr.dofFocusDepthImage->width, tr.dofFocusDepthImage->height);
		FBO_Bind(tr.dofFocusDepthFbo);
		FBO_AttachTextureImage(tr.dofFocusDepthImage, 0);
		FBO_SetupDrawBuffers();
		R_CheckFBO(tr.dofFocusDepthFbo);
	}

	GL_CheckErrors();

	FBO_Bind(NULL);
}

void FBO_Delete(FBO_t *fbo)
{
	int             j;

	FBO_Bind(NULL);

	for(j = 0; j < glRefConfig.maxColorAttachments; j++)
	{
		if(fbo->colorBuffers[j])
			qglDeleteRenderbuffers(1, &fbo->colorBuffers[j]);
	}

	if(fbo->depthBuffer)
		qglDeleteRenderbuffers(1, &fbo->depthBuffer);

	if(fbo->stencilBuffer)
		qglDeleteRenderbuffers(1, &fbo->stencilBuffer);

	if(fbo->frameBuffer)
		qglDeleteFramebuffers(1, &fbo->frameBuffer);
}

/*
============
FBO_Shutdown
============
*/
void FBO_Shutdown(void)
{
	int             i, j;
	FBO_t          *fbo;

	ri->Printf(PRINT_ALL, "^5------- ^7FBO_Shutdown^5 -------\n");

	FBO_Bind(NULL);

	for(i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		for(j = 0; j < glRefConfig.maxColorAttachments; j++)
		{
			if(fbo->colorBuffers[j])
				qglDeleteRenderbuffers(1, &fbo->colorBuffers[j]);
		}

		if(fbo->depthBuffer)
			qglDeleteRenderbuffers(1, &fbo->depthBuffer);

		if(fbo->stencilBuffer)
			qglDeleteRenderbuffers(1, &fbo->stencilBuffer);

		if(fbo->frameBuffer)
			qglDeleteFramebuffers(1, &fbo->frameBuffer);
	}
}

/*
============
R_FBOList_f
============
*/
void R_FBOList_f(void)
{
	int             i;
	FBO_t          *fbo;

	ri->Printf(PRINT_ALL, "^7             size       name\n");
	ri->Printf(PRINT_ALL, "^5----------------------------------------------------------\n");

	for(i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		ri->Printf(PRINT_ALL, "  %4i: %4i %4i %s\n", i, fbo->width, fbo->height, fbo->name);
	}

	ri->Printf(PRINT_ALL, " %i FBOs\n", tr.numFBOs);
}

void FBO_BlitFromTexture(struct image_s *src, vec4i_t inSrcBox, vec2_t inSrcTexScale, FBO_t *dst, vec4i_t inDstBox, struct shaderProgram_s *shaderProgram, vec4_t inColor, int blend)
{
	vec4i_t dstBox, srcBox;
	vec2_t srcTexScale;
	vec4_t color;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	vec2_t invTexRes;
	FBO_t *oldFbo = glState.currentFBO;
	matrix_t projection;
	int width, height;

	if (!src)
		return;

	if (inSrcBox)
	{
		VectorSet4(srcBox, inSrcBox[0], inSrcBox[1], inSrcBox[0] + inSrcBox[2],  inSrcBox[1] + inSrcBox[3]);
	}
	else
	{
		VectorSet4(srcBox, 0, 0, src->width, src->height);
	}

	// framebuffers are 0 bottom, Y up.
	if (inDstBox)
	{
		if (dst)
		{
			dstBox[0] = inDstBox[0];
			dstBox[1] = dst->height - inDstBox[1] - inDstBox[3];
			dstBox[2] = inDstBox[0] + inDstBox[2];
			dstBox[3] = dst->height - inDstBox[1];
		}
		else
		{
			dstBox[0] = inDstBox[0];
			dstBox[1] = glConfig.vidHeight - inDstBox[1] - inDstBox[3];
			dstBox[2] = inDstBox[0] + inDstBox[2];
			dstBox[3] = glConfig.vidHeight - inDstBox[1];
		}
	}
	else if (dst)
	{
		VectorSet4(dstBox, 0, dst->height, dst->width, 0);
	}
	else
	{
		VectorSet4(dstBox, 0, glConfig.vidHeight * r_superSampleMultiplier->value, glConfig.vidWidth * r_superSampleMultiplier->value, 0);
	}

	if (inSrcTexScale)
	{
		VectorCopy2(inSrcTexScale, srcTexScale);
	}
	else
	{
		srcTexScale[0] = srcTexScale[1] = 1.0f;
	}

	if (inColor)
	{
		VectorCopy4(inColor, color);
	}
	else
	{
		VectorCopy4(colorWhite, color);
	}

	if (!shaderProgram)
	{
		shaderProgram = &tr.textureColorShader;
	}

	FBO_Bind(dst);

	if (glState.currentFBO)
	{
		width = glState.currentFBO->width;
		height = glState.currentFBO->height;
	}
	else
	{
		width = glConfig.vidWidth * r_superSampleMultiplier->value;
		height = glConfig.vidHeight * r_superSampleMultiplier->value;
	}

	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Matrix16Ortho(0, width, height, 0, 0, 1, projection);

	qglDisable( GL_CULL_FACE );

	VectorSet4(quadVerts[0], dstBox[0], dstBox[1], 0, 1);
	VectorSet4(quadVerts[1], dstBox[2], dstBox[1], 0, 1);
	VectorSet4(quadVerts[2], dstBox[2], dstBox[3], 0, 1);
	VectorSet4(quadVerts[3], dstBox[0], dstBox[3], 0, 1);

	texCoords[0][0] = srcBox[0] / (float)src->width; texCoords[0][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[1][0] = srcBox[2] / (float)src->width; texCoords[1][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[2][0] = srcBox[2] / (float)src->width; texCoords[2][1] = 1.0f - srcBox[3] / (float)src->height;
	texCoords[3][0] = srcBox[0] / (float)src->width; texCoords[3][1] = 1.0f - srcBox[3] / (float)src->height;

	invTexRes[0] = 1.0f / src->width  * srcTexScale[0];
	invTexRes[1] = 1.0f / src->height * srcTexScale[1];

	GL_State( blend );

	GLSL_BindProgram(shaderProgram);

	if (shaderProgram->isBindless)
	{
		GLSL_SetBindlessTexture(shaderProgram, UNIFORM_DIFFUSEMAP, &src, 0);
		GLSL_BindlessUpdate(shaderProgram);
	}
	else
	{
		GLSL_SetUniformInt(shaderProgram, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GL_BindToTMU(src, TB_DIFFUSEMAP);
	}

	GLSL_SetUniformMatrix16(shaderProgram, UNIFORM_MODELVIEWPROJECTIONMATRIX, projection);
	GLSL_SetUniformVec4(shaderProgram, UNIFORM_COLOR, color);
	GLSL_SetUniformVec2(shaderProgram, UNIFORM_INVTEXRES, invTexRes);
	GLSL_SetUniformVec2(shaderProgram, UNIFORM_AUTOEXPOSUREMINMAX, tr.refdef.autoExposureMinMax);
	GLSL_SetUniformVec3(shaderProgram, UNIFORM_TONEMINAVGMAXLINEAR, tr.refdef.toneMinAvgMaxLinear);

	RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

	FBO_Bind(oldFbo);
}

void FBO_Blit(FBO_t *src, vec4i_t inSrcBox, vec2_t srcTexScale, FBO_t *dst, vec4i_t dstBox, struct shaderProgram_s *shaderProgram, vec4_t color, int blend)
{
	vec4i_t srcBox;

	if (!src)
	{
		ri->Printf(PRINT_WARNING, "Tried to blit from a NULL FBO!\n");
		return;
	}

	// framebuffers are 0 bottom, Y up.
	if (inSrcBox)
	{
		srcBox[0] = inSrcBox[0];
		srcBox[1] = src->height - inSrcBox[1] - inSrcBox[3];
		srcBox[2] = inSrcBox[2];
		srcBox[3] = inSrcBox[3];
	}
	else
	{
		VectorSet4(srcBox, 0, src->height, src->width, -src->height);
	}

	FBO_BlitFromTexture(src->colorImage[0], srcBox, srcTexScale, dst, dstBox, shaderProgram, color, blend | GLS_DEPTHTEST_DISABLE);
}

void FBO_FastBlit(FBO_t *src, vec4i_t srcBox, FBO_t *dst, vec4i_t dstBox, int buffers, int filter)
{
	vec4i_t srcBoxFinal, dstBoxFinal;
	GLuint srcFb, dstFb;

	// get to a neutral state first
	//FBO_Bind(NULL);

	srcFb = src ? src->frameBuffer : 0;
	dstFb = dst ? dst->frameBuffer : 0;

	if (!srcBox)
	{
		if (src)
		{
			VectorSet4(srcBoxFinal, 0, 0, src->width, src->height);
		}
		else
		{
			VectorSet4(srcBoxFinal, 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value);
		}
	}
	else
	{
		VectorSet4(srcBoxFinal, srcBox[0], srcBox[1], srcBox[0] + srcBox[2], srcBox[1] + srcBox[3]);
	}

	if (!dstBox)
	{
		if (dst)
		{
			VectorSet4(dstBoxFinal, 0, 0, dst->width, dst->height);
		}
		else
		{
			VectorSet4(dstBoxFinal, 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value);
		}
	}
	else
	{
		VectorSet4(dstBoxFinal, dstBox[0], dstBox[1], dstBox[0] + dstBox[2], dstBox[1] + dstBox[3]);
	}

	qglBindFramebuffer(GL_READ_FRAMEBUFFER, srcFb);
	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFb);

	qglBlitFramebuffer(srcBoxFinal[0], srcBoxFinal[1], srcBoxFinal[2], srcBoxFinal[3],
	                      dstBoxFinal[0], dstBoxFinal[1], dstBoxFinal[2], dstBoxFinal[3],
						  buffers, filter);

	FBO_Bind(NULL);
}

void FBO_FastBlitIndexed(FBO_t *src, FBO_t *dst, int srcReadBuffer, int dstDrawBuffer, int buffers, int filter)
{
	assert (src != NULL);
	assert (dst != NULL);

	qglBindFramebuffer(GL_READ_FRAMEBUFFER, src->frameBuffer);
	qglReadBuffer (GL_COLOR_ATTACHMENT0 + srcReadBuffer);

	qglBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->frameBuffer);
	qglDrawBuffer (GL_COLOR_ATTACHMENT0 + dstDrawBuffer);

	qglBlitFramebuffer(0, 0, src->width, src->height,
	                      0, 0, dst->width, dst->height,
						  buffers, filter);

	qglReadBuffer (GL_COLOR_ATTACHMENT0);

	glState.currentFBO = dst;
	FBO_SetupDrawBuffers();

	FBO_Bind(NULL);
}
