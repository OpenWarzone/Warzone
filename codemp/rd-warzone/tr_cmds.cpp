/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"

/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void ) {
	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1) {
		ri->Printf (PRINT_ALL, "%i/%i/%i shaders/batches/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfBatches, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes, 
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3, 
			R_SumOfUsedImages()/(1000000.0f), backEnd.pc.c_overDraw / ((float)(glConfig.vidWidth * r_superSampleMultiplier->value) * (glConfig.vidHeight * r_superSampleMultiplier->value)) ); 
	} else if (r_speeds->integer == 2) {
		ri->Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out, 
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri->Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out, 
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri->Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri->Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	} 
	else if (r_speeds->integer == 5 )
	{
		ri->Printf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		ri->Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n", 
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if (r_speeds->integer == 7 )
	{
		ri->Printf( PRINT_ALL, "VBO draws: static %i dynamic %i (%.2fKB)\nMultidraws: %i merged %i\n",
			backEnd.pc.c_staticVboDraws, backEnd.pc.c_dynamicVboDraws, backEnd.pc.c_dynamicVboTotalSize / (1024.0f),
			backEnd.pc.c_multidraws, backEnd.pc.c_multidrawsMerged );
		ri->Printf( PRINT_ALL, "GLSL binds: total %i. depthPass %i. sky %i. lightAll %i. misc %i.\n",
			backEnd.pc.c_glslShaderBinds, backEnd.pc.c_depthPassBinds, backEnd.pc.c_skyBinds, backEnd.pc.c_lightallBinds, backEnd.pc.c_glslShaderBinds - (backEnd.pc.c_depthPassBinds + backEnd.pc.c_skyBinds + backEnd.pc.c_lightallBinds));
		ri->Printf(PRINT_ALL, "GLSL draws: total %i. depthPass %i. sky %i. lightAll %i.\n",
			backEnd.pc.c_depthPassDraws + backEnd.pc.c_skyDraws + backEnd.pc.c_lightallDraws, backEnd.pc.c_depthPassDraws, backEnd.pc.c_skyDraws, backEnd.pc.c_lightallDraws);
	}
	else if (r_speeds->integer == 8)
	{
		ri->Printf(PRINT_ALL, "Lightmap surfaces skipped: %i\n", backEnd.pc.c_lightMapsSkipped);
	}
	else if (r_speeds->integer == 9)
	{
		ri->Printf(PRINT_ALL, "Tiny surfaces skipped: %i\n", backEnd.pc.c_tinySkipped);
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}


/*
====================
R_IssueRenderCommands
====================
*/
extern qboolean LOADSCREEN_POSTPROCESS;
extern const void *RB_PostProcess(const void *data);

void R_IssueRenderCommands( qboolean runPerformanceCounters ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData->commands;
	assert(cmdList);
	// add an end-of-list command
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if ( runPerformanceCounters ) {
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer ) {
		// let it start on the new batch
		RB_ExecuteRenderCommands( cmdList->cmds );
	}
}


/*
====================
R_IssuePendingRenderCommands

Issue any pending commands and wait for them to complete.
====================
*/
void R_IssuePendingRenderCommands( void ) {
	if ( !tr.registered ) {
		return;
	}
	R_IssueRenderCommands( qfalse );
}

/*
============
R_GetCommandBuffer

make sure there is enough command space
============
*/
void *R_GetCommandBuffer( int bytes ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData->commands;
	bytes = PAD(bytes, sizeof(void *));

	// always leave room for the end of list command
	if ( cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS ) {
		if ( bytes > MAX_RENDER_COMMANDS - 4 ) {
			ri->Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}


/*
=============
R_AddDrawSurfCmd

=============
*/
void	R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	drawSurfsCommand_t	*cmd;

	cmd = (drawSurfsCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}


/*
=============
R_AddCapShadowmapCmd

=============
*/
void	R_AddCapShadowmapCmd( int map, int cubeSide ) {
	capShadowmapCommand_t	*cmd;

	cmd = (capShadowmapCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_CAPSHADOWMAP;

	cmd->map = map;
	cmd->cubeSide = cubeSide;
}


/*
=============
R_PostProcessingCmd

=============
*/
void	R_AddPostProcessCmd( ) {
	postProcessCommand_t	*cmd;

	cmd = (postProcessCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_POSTPROCESS;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void	RE_SetColor( const float *rgba ) {
	setColorCommand_t	*cmd;

  if ( !tr.registered ) {
    return;
  }
	cmd = (setColorCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	if ( !rgba ) {
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}

void RE_AdjustForSuperSample ( float *x, float *y, float *w, float *h )
{
	if ( r_superSampleMultiplier->value <= 1.0) return;

	//*x *= (*x / r_superSampleMultiplier->value);
	*x /= 2.0;
	*y += 240 - (*y / 2.0);
	*w /= 2.0;
	*h *= (240 / (*y * 2.0));
}

/*
=============
RE_RotatePic
=============
*/
void RE_RotatePic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) {
	rotatePicCommand_t	*cmd;

	cmd = (rotatePicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_ROTATE_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	RE_AdjustForSuperSample(&cmd->x, &cmd->y, &cmd->w, &cmd->h);
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
	cmd->a = a;
}

/*
=============
RE_RotatePic2
=============
*/
void RE_RotatePic2 ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) {
	rotatePicCommand_t	*cmd;

	cmd = (rotatePicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_ROTATE_PIC2;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	RE_AdjustForSuperSample(&cmd->x, &cmd->y, &cmd->w, &cmd->h);
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
	cmd->a = a;
}

/*
=============
RE_StretchPic
=============
*/
void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	stretchPicCommand_t	*cmd;

  if (!tr.registered) {
    return;
  }
	cmd = (stretchPicCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	RE_AdjustForSuperSample(&cmd->x, &cmd->y, &cmd->w, &cmd->h);
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

void RE_RenderWorldEffects(void)
{
	drawBufferCommand_t	*cmd;

	cmd = (drawBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_WORLD_EFFECTS;
}

#define MODE_RED_CYAN	1
#define MODE_RED_BLUE	2
#define MODE_RED_GREEN	3
#define MODE_GREEN_MAGENTA 4
#define MODE_MAX	MODE_GREEN_MAGENTA

void R_SetColorMode(GLboolean *rgba, stereoFrame_t stereoFrame, int colormode)
{
	rgba[0] = rgba[1] = rgba[2] = rgba[3] = GL_TRUE;
	
	if(colormode > MODE_MAX)
	{
		colormode -= MODE_MAX;
	}
}


/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
extern void RB_AdvanceOverlaySway(void);

void RE_BeginFrame( stereoFrame_t stereoFrame ) {
	drawBufferCommand_t	*cmd = NULL;
	colorMaskCommand_t *colcmd = NULL;

	if ( !tr.registered ) {
		return;
	}

	extern void NuklearUI_Main(void);
	NuklearUI_Main();

	RB_AdvanceOverlaySway();

	glState.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	//
	// do overdraw measurement
	//
	if ( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			ri->Printf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
			ri->Cvar_Set( "r_measureOverdraw", "0" );
		}
		else
		{
			R_IssuePendingRenderCommands();
			qglEnable( GL_STENCIL_TEST );
			qglStencilMask( ~0U );
			qglClearStencil( 0U );
			qglStencilFunc( GL_ALWAYS, 0U, ~0U );
			qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
		}
		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified ) {
			R_IssuePendingRenderCommands();
			qglDisable( GL_STENCIL_TEST );
		}
		r_measureOverdraw->modified = qfalse;
	}

	//
	// texturemode stuff
	//
	if ( r_textureMode->modified || r_ext_texture_filter_anisotropic->modified ) 
	{
		R_IssuePendingRenderCommands();
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = qfalse;
		r_ext_texture_filter_anisotropic->modified = qfalse;
	}

	//
	// gamma stuff
	//
	if ( r_gamma->modified ) {
		r_gamma->modified = qfalse;

		R_IssuePendingRenderCommands();
		R_SetColorMappings();
	}

	// check for errors
	if ( !r_ignoreGLErrors->integer )
	{
		int	err;

		R_IssuePendingRenderCommands();
		if ((err = qglGetError()) != GL_NO_ERROR)
			ri->Error(ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!", err);
	}

	
	if( !(cmd = (drawBufferCommand_t *)R_GetCommandBuffer(sizeof(*cmd))) )
		return;

	if(cmd)
	{
		cmd->commandId = RC_DRAW_BUFFER;

		if (!Q_stricmp(r_drawBuffer->string, "GL_FRONT"))
			cmd->buffer = (int)GL_FRONT;
		else
			cmd->buffer = (int)GL_BACK;
	}
	
	tr.refdef.stereoFrame = stereoFrame;
}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
#ifdef __VR__
void SetupShaderDistortion(int eye, float VPX, float VPY, float VPW, float VPH)
{
	float  as, x, y, w, h;
	struct OVR_StereoCfg stereoCfg;
	GLuint lenscenter = qglGetUniformLocation(glConfig.oculusProgId, "LensCenter");
	GLuint screencenter = qglGetUniformLocation(glConfig.oculusProgId, "ScreenCenter");
	GLuint uscale = qglGetUniformLocation(glConfig.oculusProgId, "Scale");
	GLuint uscalein = qglGetUniformLocation(glConfig.oculusProgId, "ScaleIn");
	GLuint uhmdwarp = qglGetUniformLocation(glConfig.oculusProgId, "HmdWarpParam");
	GLuint offset = qglGetUniformLocation(glConfig.oculusProgId, "Offset");

	stereoCfg.x = VPX;
	stereoCfg.y = VPY;
	stereoCfg.w = VPW*0.5f;
	stereoCfg.h = VPH;

	as = (VPW*0.5f) / VPH;
	x = VPX / (float)(glConfig.vidWidth);
	y = VPY / (float)(glConfig.vidHeight);
	w = VPW / (float)(glConfig.vidWidth);
	h = VPH / (float)(glConfig.vidHeight);

	if (OVRDetected)
	{
		OVR_StereoConfig(eye, &stereoCfg);
	}
	else
	{
		stereoCfg.distscale = 1.701516f;
		stereoCfg.XCenterOffset = 0.145299f;
		if (eye == 1)
		{
			stereoCfg.XCenterOffset *= -1;
		}
		stereoCfg.K[0] = 1.00f;
		stereoCfg.K[1] = 0.22f;
		stereoCfg.K[2] = 0.24f;
		stereoCfg.K[3] = 0.00f;
	}

	qglUniform2f(lenscenter, x + (w + stereoCfg.XCenterOffset * vr_lenseoffset->value)*0.5f, y + h*0.5f);
	qglUniform2f(screencenter, x + w*0.5f, y + h*0.5f);
	if (eye == 1)
		qglUniform2f(offset, vr_viewofsx->value, vr_viewofsy->value);
	else
		qglUniform2f(offset, -vr_viewofsx->value, vr_viewofsy->value);

	stereoCfg.distscale = 1.0f / stereoCfg.distscale;

	qglUniform2f(uscale, (w / 2) * stereoCfg.distscale, (h / 2) * stereoCfg.distscale * as);
	qglUniform2f(uscalein, (2 / w), (2 / h) / as);
	qglUniform4fv(uhmdwarp, 1, stereoCfg.K);

	if (vr_ipd->value != 0.0)
	{
		HMD.InterpupillaryDistance = vr_ipd->value;
	}

}
#endif //__VR__

void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) {
	swapBuffersCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (swapBuffersCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

#ifdef __VR__
	//*** Rift post processing	
	if (vr_warpingShader->integer)
	{
		float x;
		float y;
		float w;
		float h;
		GLuint texID;
		static int b = 0;

		//qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		//qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		qglBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);

		if (!backEnd.projection2D)
		{
			extern void	RB_SetGL2D(void);
			RB_SetGL2D();
		}

		qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight); // Render on the whole framebuffer, complete from the lower left corner to the upper right

																 // Use our shader
		qglUseProgram(glConfig.oculusProgId);


		qglEnable(GL_TEXTURE_2D);


		{
			float VPX = 0.0f;
			float VPY = 0.0f;
			float VPW = glConfig.vidWidth; // ViewPort Width
			float VPH = glConfig.vidHeight;

			SetupShaderDistortion(0, VPX, VPY, VPW, VPH); // Left Eye
		}

		// Set our "renderedTexture" sampler to user Texture Unit 0
		texID = qglGetUniformLocation(glConfig.oculusProgId, "texid");
		qglUniform1i(texID, 0);


		qglColor3f(tr.identityLight, tr.identityLight, tr.identityLight);

		int     oldtmu = glState.currenttmu;
		int		oldtexture = glState.currenttextures[oldtmu];

		//	if (stereoFrame == STEREO_LEFT)
		{
			GL_SelectTexture(0/*GL_TEXTURE0*/);
			qglBindTexture(GL_TEXTURE_2D, glConfig.oculusRenderTargetLeft);

			x = 0.0f;
			y = 0.0f;
			w = glConfig.vidWidth;
			h = glConfig.vidHeight;


			qglBegin(GL_QUADS);
			qglTexCoord2f(0, 1);
			qglVertex2f(x, y);
			qglTexCoord2f(1, 1);
			qglVertex2f(x + w / 2, y);
			qglTexCoord2f(1, 0);
			qglVertex2f(x + w / 2, y + h);
			qglTexCoord2f(0, 0);
			qglVertex2f(x, y + h);
			qglEnd();
		}
		//else
		{
			{
				float VPX = 0;
				float VPY = 0.0f;
				float VPW = glConfig.vidWidth; // ViewPort Width
				float VPH = glConfig.vidHeight;

				SetupShaderDistortion(1, VPX, VPY, VPW, VPH); // Right Eye
			}

			GL_SelectTexture(0/*GL_TEXTURE0*/);
			qglBindTexture(GL_TEXTURE_2D, glConfig.oculusRenderTargetRight);


			x = glConfig.vidWidth*0.5f;
			y = 0.0f;
			w = glConfig.vidWidth;
			h = glConfig.vidHeight;


			qglBegin(GL_QUADS);
			qglTexCoord2f(0, 1);
			qglVertex2f(x, y);
			qglTexCoord2f(1, 1);
			qglVertex2f(x + w / 2, y);
			qglTexCoord2f(1, 0);
			qglVertex2f(x + w / 2, y + h);
			qglTexCoord2f(0, 0);
			qglVertex2f(x, y + h);
			qglEnd();
		}
		// unbind the GLSL program
		// this means that from here the OpenGL fixed functionality is used
		qglUseProgram(0);


		// UQ: Revert bindings...
		qglBindTexture(GL_TEXTURE_2D, oldtexture);
		glState.currenttextures[oldtmu] = oldtexture;
		GL_SelectTexture(oldtmu);
	}
#endif //__VR__

	R_InitNextFrame();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}

/*
=============
RE_TakeVideoFrame
=============
*/
void RE_TakeVideoFrame( int width, int height,
		byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg )
{
	videoFrameCommand_t	*cmd;

	if( !tr.registered ) {
		return;
	}

	cmd = (videoFrameCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if( !cmd ) {
		return;
	}

	cmd->commandId = RC_VIDEOFRAME;

	cmd->width = width;
	cmd->height = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer = encodeBuffer;
	cmd->motionJpeg = motionJpeg;
}

/*
=============
RE_MenuOpenFrame
=============
*/

void RE_MenuOpenFrame(qboolean menuIsOpen)
{
	glConfig.menuIsOpen = menuIsOpen;
}

/*
=============
RE_DrawAwesomiumFrame
=============
*/
void RE_DrawAwesomiumFrame(int x, int y, int w, int h, unsigned char *buffer){
	awesomiumFrameCommand_t	*cmd;

	if (!tr.registered) {
		return;
	}

	cmd = (awesomiumFrameCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd) {
		return;
	}

	cmd->commandId = RC_DRAWAWESOMIUMFRAME;

	cmd->x = x;
	cmd->y = y;
	cmd->width = w;
	cmd->height = h;
	cmd->buffer = buffer;
}
