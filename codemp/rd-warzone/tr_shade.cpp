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
// tr_shade.c

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

qboolean CLOSE_LIGHTS_UPDATE = qtrue;

extern qboolean WATER_ENABLED;

color4ub_t	styleColors[MAX_LIGHT_STYLES];

extern void RB_DrawSurfaceSprites( shaderStage_t *stage, shaderCommands_t *input);

extern qboolean RB_CheckOcclusion(matrix_t MVP, shaderCommands_t *input);

extern void GL_BindImageIDToTMU(int imageID, int tmu);

extern vec3_t		SUN_COLOR_MAIN;
extern vec3_t		SUN_COLOR_SECONDARY;
extern vec3_t		SUN_COLOR_TERTIARY;
extern vec3_t		MAP_AMBIENT_COLOR;
extern vec3_t		MAP_AMBIENT_COLOR_NIGHT;
extern int			MAP_LIGHTMAP_ENHANCEMENT;
extern float		MAP_LIGHTMAP_MULTIPLIER;
extern qboolean		MAP_COLOR_SWITCH_RG;
extern qboolean		MAP_COLOR_SWITCH_RB;
extern qboolean		MAP_COLOR_SWITCH_GB;

extern qboolean		ENABLE_CHRISTMAS_EFFECT;

extern float		DYNAMIC_WEATHER_PUDDLE_STRENGTH;

extern qboolean		GRASS_ENABLED;
extern qboolean		GRASS_UNDERWATER_ONLY;
extern qboolean		GRASS_RARE_PATCHES_ONLY;
extern int			GRASS_WIDTH_REPEATS;
extern int			GRASS_DENSITY;
extern float		GRASS_HEIGHT;
extern int			GRASS_DISTANCE;
extern float		GRASS_MAX_SLOPE;
extern float		GRASS_TYPE_UNIFORMALITY;
extern float		GRASS_TYPE_UNIFORMALITY_SCALER;
extern float		GRASS_DISTANCE_FROM_ROADS;
extern float		GRASS_SURFACE_MINIMUM_SIZE;
extern float		GRASS_SURFACE_SIZE_DIVIDER;
extern float		GRASS_SIZE_MULTIPLIER_COMMON;
extern float		GRASS_SIZE_MULTIPLIER_RARE;
extern float		GRASS_SIZE_MULTIPLIER_UNDERWATER;
extern float		GRASS_LOD_START_RANGE;

extern qboolean		FOLIAGE_ENABLED;
extern int			FOLIAGE_DENSITY;
extern float		FOLIAGE_HEIGHT;
extern int			FOLIAGE_DISTANCE;
extern float		FOLIAGE_MAX_SLOPE;
extern float		FOLIAGE_SURFACE_MINIMUM_SIZE;
extern float		FOLIAGE_SURFACE_SIZE_DIVIDER;
extern float		FOLIAGE_LOD_START_RANGE;
extern float		FOLIAGE_TYPE_UNIFORMALITY;
extern float		FOLIAGE_TYPE_UNIFORMALITY_SCALER;
extern float		FOLIAGE_DISTANCE_FROM_ROADS;

extern qboolean		VINES_ENABLED;
extern int			VINES_WIDTH_REPEATS;
extern int			VINES_DENSITY;
extern float		VINES_HEIGHT;
extern int			VINES_DISTANCE;
extern float		VINES_MIN_SLOPE;
extern float		VINES_TYPE_UNIFORMALITY;
extern float		VINES_TYPE_UNIFORMALITY_SCALER;
extern float		VINES_SURFACE_MINIMUM_SIZE;
extern float		VINES_SURFACE_SIZE_DIVIDER;

extern float		WATER_WAVE_HEIGHT;

extern qboolean		TERRAIN_TESSELLATION_ENABLED;
extern float		TERRAIN_TESSELLATION_LEVEL;
extern float		TERRAIN_TESSELLATION_OFFSET;
extern float		TERRAIN_TESSELLATION_MIN_SIZE;

#ifdef __DRAW_INDIRECT__
void R_CreateIndirectBuffers(void)
{
	if (tr.indirectArraysBuffer == 0)
	{
		qglGenBuffers(1, &tr.indirectArraysBuffer);
		qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, tr.indirectArraysBuffer);
		qglBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), NULL, /*GL_STATIC_DRAW*/GL_DYNAMIC_DRAW);
		//qglMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawArraysIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		//qglUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
	}

	if (tr.indirectElementsBuffer == 0)
	{
		qglGenBuffers(1, &tr.indirectElementsBuffer);
		qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, tr.indirectElementsBuffer);
		qglBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), NULL, /*GL_STATIC_DRAW*/GL_DYNAMIC_DRAW);
		//qglMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		//qglUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
	}

	if (tr.indirectMultiElementsBuffer == 0)
	{
		qglGenBuffers(1, &tr.indirectMultiElementsBuffer);
		qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, tr.indirectMultiElementsBuffer);
		qglBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * MAX_MULTIDRAW_PRIMITIVES, NULL, /*GL_STATIC_DRAW*/GL_DYNAMIC_DRAW);
		//qglMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand) * MAX_MULTIDRAW_PRIMITIVES, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		//qglUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
	}
}

void R_DrawElementsVBOIndirect(int numIndexes, glIndex_t firstIndex, glIndex_t minIndex, glIndex_t maxIndex, glIndex_t numVerts, qboolean tesselation)
{
	R_CreateIndirectBuffers();

	if (r_drawIndirect->integer == 1)
	{
		tr.drawElementsIndirectCommand[0].count = numIndexes;       //1 triangle = 3 vertices
		tr.drawElementsIndirectCommand[0].instanceCount = 1;     //Draw 1 instance
		tr.drawElementsIndirectCommand[0].firstIndex = firstIndex;        //Draw from index 0 for this instance
		tr.drawElementsIndirectCommand[0].baseVertex = 0; //Starting from baseVert
		tr.drawElementsIndirectCommand[0].baseInstance = 0;      //gl_InstanceID

		// feed the draw command data to the gpu via the IndirectBuffer
		qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, tr.indirectElementsBuffer);
		qglBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), &tr.drawElementsIndirectCommand);
	}
	else
	{
		qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, tr.indirectElementsBuffer);
		//qglBindBufferRange(GL_DRAW_INDIRECT_BUFFER, index, tr.indirectElementsBuffer, 0, sizeof(DrawElementsIndirectCommand));
		DrawElementsIndirectCommand *cmd = (DrawElementsIndirectCommand *)qglMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		cmd[0].count = numIndexes;
		cmd[0].instanceCount = 1;     //Draw 1 instance
		cmd[0].firstIndex = firstIndex;        //Draw from index 0 for this instance
		cmd[0].baseVertex = 0; //Starting from baseVert
		cmd[0].baseInstance = 0;      //gl_InstanceID
		qglUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
	}

	if (tesselation)
	{
		qglPatchParameteri(GL_PATCH_VERTICES, 3);
		qglDrawElementsIndirect(GL_PATCHES, GL_INDEX_TYPE, (const GLvoid **)0);
	}
	else
	{
		qglDrawElementsIndirect(GL_TRIANGLES, GL_INDEX_TYPE, (const GLvoid **)0);
	}

	qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void R_DrawMultiElementsVBOIndirect(int multiDrawPrimitives, glIndex_t *multiDrawMinIndex, glIndex_t *multiDrawMaxIndex,
	GLsizei *multiDrawNumIndexes, glIndex_t **multiDrawFirstIndex, glIndex_t numVerts, qboolean tesselation)
{
	R_CreateIndirectBuffers();

	if (r_drawIndirect->integer == 1)
	{
		for (int i = 0; i < multiDrawPrimitives; i++)
		{
			tr.drawMultiElementsCommand[i].count = multiDrawNumIndexes[i];       //1 triangle = 3 vertices
			tr.drawMultiElementsCommand[i].instanceCount = 1;     //Draw 1 instance
			tr.drawMultiElementsCommand[i].firstIndex = tess.multiDrawFirstIndexIndirect[i];        //Draw from index 0 for this instance
			tr.drawMultiElementsCommand[i].baseVertex = 0; //Starting from baseVert
			tr.drawMultiElementsCommand[i].baseInstance = 0;      //gl_InstanceID
		}

		// feed the draw command data to the gpu via the IndirectBuffer
		qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, tr.indirectMultiElementsBuffer);
		qglBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsIndirectCommand) * multiDrawPrimitives, &tr.drawMultiElementsCommand);
	}
	else
	{
		qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, tr.indirectMultiElementsBuffer);
		DrawElementsIndirectCommand *cmd = (DrawElementsIndirectCommand *)qglMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, multiDrawPrimitives * sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

		for (int i = 0; i < multiDrawPrimitives; i++)
		{
			cmd[i].count = multiDrawNumIndexes[i];
			cmd[i].instanceCount = 1;     //Draw 1 instance
			cmd[i].firstIndex = tess.multiDrawFirstIndexIndirect[i];        //Draw from index 0 for this instance
			cmd[i].baseVertex = 0; //Starting from baseVert
			cmd[i].baseInstance = 0;      //gl_InstanceID
		}

		qglUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
	}

	if (tesselation)
	{
		qglPatchParameteri(GL_PATCH_VERTICES, 3);
		qglMultiDrawElementsIndirect(GL_PATCHES, GL_INDEX_TYPE, (const GLvoid **)0, multiDrawPrimitives, 0);
	}
	else
	{
		qglMultiDrawElementsIndirect(GL_TRIANGLES, GL_INDEX_TYPE, (const GLvoid **)0, multiDrawPrimitives, 0);
	}

	qglBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}
#endif //__DRAW_INDIRECT__

/*
==================
R_DrawElements

==================
*/

void R_DrawElementsVBO( int numIndexes, glIndex_t firstIndex, glIndex_t minIndex, glIndex_t maxIndex, glIndex_t numVerts, qboolean tesselation )
{
	if (minIndex == 0 && maxIndex == 0 /*&& numIndexes == 6*/)
	{// Fix... Something did not set the corrext maxIndex...
		maxIndex = numIndexes / 2;
	}

#ifdef __DRAW_INDIRECT__
	if (r_drawIndirect->integer)
	{// Currently slower, without any extra batching...
		R_DrawElementsVBOIndirect(numIndexes, firstIndex, minIndex, maxIndex, numVerts, tesselation);
		return;
	}
#endif //!__DRAW_INDIRECT__
	
	if (tesselation)
	{
		//GLint MaxPatchVertices = 0;
		//qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
		//printf("Max supported patch vertices %d\n", MaxPatchVertices);
		//if (r_testvalue0->integer)
			//qglPatchParameteri(GL_PATCH_VERTICES, MaxPatchVertices >= 16 ? 16 : 3);
		//else
			qglPatchParameteri(GL_PATCH_VERTICES, 3);

		qglDrawRangeElements(GL_PATCHES, minIndex, maxIndex, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(firstIndex * sizeof(glIndex_t)));
	}
	else
	{
		qglDrawRangeElements(GL_TRIANGLES, minIndex, maxIndex, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(firstIndex * sizeof(glIndex_t)));
	}
}

void R_DrawMultiElementsVBO( int multiDrawPrimitives, glIndex_t *multiDrawMinIndex, glIndex_t *multiDrawMaxIndex,
	GLsizei *multiDrawNumIndexes, glIndex_t **multiDrawFirstIndex, glIndex_t numVerts, qboolean tesselation)
{
#ifdef __DRAW_INDIRECT__
	if (r_drawIndirect->integer)
	{// Currently slower, without any extra batching...
		R_DrawMultiElementsVBOIndirect(multiDrawPrimitives, multiDrawMinIndex, multiDrawMaxIndex, multiDrawNumIndexes, multiDrawFirstIndex, numVerts, tesselation);
	}
#endif //__DRAW_INDIRECT__
	
	if (tesselation)
	{
		//GLint MaxPatchVertices = 0;
		//qglGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
		//if (r_testvalue0->integer)
		//	qglPatchParameteri(GL_PATCH_VERTICES, MaxPatchVertices >= 16 ? 16 : 3);
		//else
			qglPatchParameteri(GL_PATCH_VERTICES, 3);

		qglMultiDrawElements(GL_PATCHES, multiDrawNumIndexes, GL_INDEX_TYPE, (const GLvoid **)multiDrawFirstIndex, multiDrawPrimitives);
	}
	else
	{
		qglMultiDrawElements(GL_TRIANGLES, multiDrawNumIndexes, GL_INDEX_TYPE, (const GLvoid **)multiDrawFirstIndex, multiDrawPrimitives);
		//qglMultiDrawElementsIndirect(GL_TRIANGLES, GL_INDEX_TYPE, (const GLvoid **)multiDrawFirstIndex, *multiDrawNumIndexes, 0);
	}
}

/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t	tess;


/*
=================
R_BindAnimatedImageToTMU

=================
*/
void R_BindAnimatedImageToTMU( textureBundle_t *bundle, int tmu ) {
	int		index;

	if ( bundle->isVideoMap ) {
		int oldtmu = glState.currenttmu;
		GL_SelectTexture(tmu);
		ri->CIN_RunCinematic(bundle->videoMapHandle);
		ri->CIN_UploadCinematic(bundle->videoMapHandle);
		GL_SelectTexture(oldtmu);
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_BindToTMU( bundle->image[0], tmu);
		return;
	}

	if (backEnd.currentEntity->e.renderfx & RF_SETANIMINDEX )
	{
		index = backEnd.currentEntity->e.skinNum;
	}
	else
	{
		// it is necessary to do this messy calc to make sure animations line up
		// exactly with waveforms of the same frequency
		index = Q_ftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
		index >>= FUNCTABLE_SIZE2;

		if ( index < 0 ) {
			index = 0;	// may happen with shader time offsets
		}
	}

	if ( bundle->oneShotAnimMap )
	{
		if ( index >= bundle->numImageAnimations )
		{
			// stick on last frame
			index = bundle->numImageAnimations - 1;
		}
	}
	else
	{
		// loop
		index %= bundle->numImageAnimations;
	}

	GL_BindToTMU( bundle->image[ index ], tmu );
}


/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris (shaderCommands_t *input) {
	GL_Bind( tr.whiteImage );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	GL_SetDepthRange( 0, 0 );

	{
		shaderProgram_t *sp = &tr.textureColorShader;
		//vec4_t color;

		GLSL_VertexAttribsState(ATTR_POSITION);
		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		GLSL_SetUniformVec4(sp, UNIFORM_COLOR, /*colorWhite*/colorYellow);

		if (tr.textureColorShader.isBindless)
		{
			GLSL_SetBindlessTexture(&tr.textureColorShader, UNIFORM_DIFFUSEMAP, &tr.whiteImage, 0);
			GLSL_BindlessUpdate(&tr.textureColorShader);
		}

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, qfalse);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, qfalse);
		}
	}

	GL_SetDepthRange( 0, 1 );
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals (shaderCommands_t *input) {
	//FIXME: implement this
}


/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/

#if 0
#ifdef __PLAYER_BASED_CUBEMAPS__
extern int			currentPlayerCubemap;
extern vec4_t		currentPlayerCubemapVec;
extern float		currentPlayerCubemapDistance;
#endif //__PLAYER_BASED_CUBEMAPS__
#endif

void RB_EndSurfaceReal(void);

void RB_BeginSurface( shader_t *shader, int fogNum, int cubemapIndex ) {
	if (tess.numIndexes > 0 || tess.numVertexes > 0)
	{// End any old draws we may not have written...
		RB_EndSurface();
	}

	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.firstIndex = 0;
	tess.numVertexes = 0;
	tess.multiDrawPrimitives = 0;
	tess.shader = state;
#ifdef __Q3_FOG__
	tess.fogNum = fogNum;
#else //!__Q3_FOG__
	tess.fogNum = 0;
#endif //__Q3_FOG__

#if 0
#ifdef __PLAYER_BASED_CUBEMAPS__
	tess.cubemapIndex = currentPlayerCubemap;
#else //!__PLAYER_BASED_CUBEMAPS__
	tess.cubemapIndex = cubemapIndex;
#endif //__PLAYER_BASED_CUBEMAPS__
#endif

	//tess.dlightBits = 0;		// will be OR'd in by surface functions
#ifdef __PSHADOWS__
	tess.pshadowBits = 0;       // will be OR'd in by surface functions
#endif
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
	tess.useInternalVBO = qtrue;

	if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
		tess.cubemapIndex = cubemapIndex = 0;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}

	if (backEnd.viewParms.flags & VPF_SHADOWMAP)
	{
		tess.currentStageIteratorFunc = RB_StageIteratorGeneric;
	}
}

/*
** RB_EndSurface
*/
void RB_EndSurfaceReal(void) {
	shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0 || input->numVertexes == 0) {
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES - 1] != 0) {
		ri->Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}
	if (input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0) {
		ri->Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	if (tess.shader == tr.shadowShader) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if (r_debugSort->integer && r_debugSort->integer < tess.shader->sort) {
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if (r_showtris->integer) {
		DrawTris(input);
	}

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
	tess.multiDrawPrimitives = 0;

	glState.vertexAnimation = qfalse;

	GLimp_LogComment("----------\n");
}

void RB_EndSurface(void) {
	RB_EndSurfaceReal();
}


extern float EvalWaveForm( const waveForm_t *wf );
extern float EvalWaveFormClamped( const waveForm_t *wf );


static void ComputeTexMods( shaderStage_t *pStage, int bundleNum, float *outMatrix, float *outOffTurb, float *outScale)
{
	int tm;
	float matrix[6], currentmatrix[6];
	textureBundle_t *bundle = &pStage->bundle[bundleNum];

	outScale[0] = outScale[1] = 1.0;

	matrix[0] = 1.0f; matrix[2] = 0.0f; matrix[4] = 0.0f;
	matrix[1] = 0.0f; matrix[3] = 1.0f; matrix[5] = 0.0f;

	currentmatrix[0] = 1.0f; currentmatrix[2] = 0.0f; currentmatrix[4] = 0.0f;
	currentmatrix[1] = 0.0f; currentmatrix[3] = 1.0f; currentmatrix[5] = 0.0f;

	outMatrix[0] = 1.0f; outMatrix[2] = 0.0f;
	outMatrix[1] = 0.0f; outMatrix[3] = 1.0f;

	outOffTurb[0] = 0.0f; outOffTurb[1] = 0.0f; outOffTurb[2] = 0.0f; outOffTurb[3] = 0.0f;

	for (tm = 0; tm < bundle->numTexMods; tm++) {
		switch (bundle->texMods[tm].type)
		{

		case TMOD_NONE:
			tm = TR_MAX_TEXMODS;		// break out of for loop
			break;

		case TMOD_TURBULENT:
			RB_CalcTurbulentFactors(&bundle->texMods[tm].wave, &outOffTurb[2], &outOffTurb[3]);
			break;

		case TMOD_ENTITY_TRANSLATE:
			RB_CalcScrollTexMatrix(backEnd.currentEntity->e.shaderTexCoord, matrix);
			break;

		case TMOD_SCROLL:
			RB_CalcScrollTexMatrix(bundle->texMods[tm].scroll,
				matrix);
			break;

		case TMOD_SCALE:
			//RB_CalcScaleTexMatrix(bundle->texMods[tm].scale,
			//	matrix);
			outScale[0] = bundle->texMods[tm].scale[0];
			outScale[1] = bundle->texMods[tm].scale[1];
			break;

		case TMOD_STRETCH:
			RB_CalcStretchTexMatrix(&bundle->texMods[tm].wave,
				matrix);
			break;

		case TMOD_TRANSFORM:
			RB_CalcTransformTexMatrix(&bundle->texMods[tm],
				matrix);
			break;

		case TMOD_ROTATE:
			RB_CalcRotateTexMatrix(bundle->texMods[tm].rotateSpeed,
				matrix);
			break;

		default:
			ri->Error(ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'", bundle->texMods[tm].type, tess.shader->name);
			break;
		}

		switch (bundle->texMods[tm].type)
		{
		case TMOD_NONE:
		case TMOD_TURBULENT:
		default:
			break;

		case TMOD_SCALE:
			break;

		case TMOD_ENTITY_TRANSLATE:
		case TMOD_SCROLL:
		case TMOD_STRETCH:
		case TMOD_TRANSFORM:
		case TMOD_ROTATE:
			outMatrix[0] = matrix[0] * currentmatrix[0] + matrix[2] * currentmatrix[1];
			outMatrix[1] = matrix[1] * currentmatrix[0] + matrix[3] * currentmatrix[1];

			outMatrix[2] = matrix[0] * currentmatrix[2] + matrix[2] * currentmatrix[3];
			outMatrix[3] = matrix[1] * currentmatrix[2] + matrix[3] * currentmatrix[3];

			outOffTurb[0] = matrix[0] * currentmatrix[4] + matrix[2] * currentmatrix[5] + matrix[4];
			outOffTurb[1] = matrix[1] * currentmatrix[4] + matrix[3] * currentmatrix[5] + matrix[5];

			currentmatrix[0] = outMatrix[0];
			currentmatrix[1] = outMatrix[1];
			currentmatrix[2] = outMatrix[2];
			currentmatrix[3] = outMatrix[3];
			currentmatrix[4] = outOffTurb[0];
			currentmatrix[5] = outOffTurb[1];
			break;
		}
	}
}


static void ComputeDeformValues(int *deformGen, float *deformParams)
{
	// u_DeformGen
	*deformGen = DGEN_NONE;
	if(!ShaderRequiresCPUDeforms(tess.shader))
	{
		deformStage_t  *ds;

		// only support the first one
		ds = &tess.shader->deforms[0];

		switch (ds->deformation)
		{
			case DEFORM_WAVE:
				*deformGen = ds->deformationWave.func;

				deformParams[0] = ds->deformationWave.base;
				deformParams[1] = ds->deformationWave.amplitude;
				deformParams[2] = ds->deformationWave.phase;
				deformParams[3] = ds->deformationWave.frequency;
				deformParams[4] = ds->deformationSpread;
				deformParams[5] = 0;
				deformParams[6] = 0;
				break;

			case DEFORM_BULGE:
				*deformGen = DGEN_BULGE;

				deformParams[0] = 0;
				deformParams[1] = ds->bulgeHeight; // amplitude
				deformParams[2] = ds->bulgeWidth;  // phase
				deformParams[3] = ds->bulgeSpeed;  // frequency
				deformParams[4] = 0;
				deformParams[5] = 0;
				deformParams[6] = 0;
				break;

			/*case DEFORM_MOVE:
				*deformGen = DGEN_MOVE;
				*waveFunc = ds->deformationWave.func;

				deformParams[0] = ds->deformationWave.base;
				deformParams[1] = ds->deformationWave.amplitude;
				deformParams[2] = ds->deformationWave.phase;
				deformParams[3] = ds->deformationWave.frequency;
				deformParams[4] = ds->moveVector[0];
				deformParams[5] = ds->moveVector[1];
				deformParams[6] = ds->moveVector[2];

				break;*/

			case DEFORM_PROJECTION_SHADOW:
				*deformGen = DGEN_PROJECTION_SHADOW;

				deformParams[0] = backEnd.ori.axis[0][2];
				deformParams[1] = backEnd.ori.axis[1][2];
				deformParams[2] = backEnd.ori.axis[2][2];
				deformParams[3] = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;
				deformParams[4] = backEnd.currentEntity->modelLightDir[0];
				deformParams[5] = backEnd.currentEntity->modelLightDir[1];
				deformParams[6] = backEnd.currentEntity->modelLightDir[2];
				break;

			default:
				break;
		}
	}
}

static void ComputeShaderColors( shaderStage_t *pStage, vec4_t baseColor, vec4_t vertColor, int blend, colorGen_t *forceRGBGen, alphaGen_t *forceAlphaGen )
{
	colorGen_t rgbGen = pStage->rgbGen;
	alphaGen_t alphaGen = pStage->alphaGen;

	baseColor[0] =
   	baseColor[1] =
   	baseColor[2] =
   	baseColor[3] = 1.0f;

   	vertColor[0] =
   	vertColor[1] =
   	vertColor[2] =
   	vertColor[3] = 0.0f;

	if ( forceRGBGen != NULL && *forceRGBGen != CGEN_BAD )
	{
		rgbGen = *forceRGBGen;
	}

	if ( forceAlphaGen != NULL && *forceAlphaGen != AGEN_IDENTITY )
	{
		alphaGen = *forceAlphaGen;
	}

	switch ( rgbGen )
	{
		case CGEN_IDENTITY_LIGHTING:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = tr.identityLight;
			break;
		case CGEN_EXACT_VERTEX:
		case CGEN_EXACT_VERTEX_LIT:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 1.0f;
			break;
		case CGEN_CONST:
			baseColor[0] = pStage->constantColor[0] / 255.0f;
			baseColor[1] = pStage->constantColor[1] / 255.0f;
			baseColor[2] = pStage->constantColor[2] / 255.0f;
			baseColor[3] = pStage->constantColor[3] / 255.0f;
			break;
		case CGEN_VERTEX:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = tr.identityLight;
			vertColor[3] = 1.0f;
			break;
		case CGEN_VERTEX_LIT:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = tr.identityLight;
			break;
		case CGEN_ONE_MINUS_VERTEX:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = tr.identityLight;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = -tr.identityLight;
			break;
		case CGEN_FOG:
			{
				if (!r_fog->integer)
					break;

				fog_t		*fog;

				fog = tr.world->fogs + tess.fogNum;

				baseColor[0] = ((unsigned char *)(&fog->colorInt))[0] / 255.0f;
				baseColor[1] = ((unsigned char *)(&fog->colorInt))[1] / 255.0f;
				baseColor[2] = ((unsigned char *)(&fog->colorInt))[2] / 255.0f;
				baseColor[3] = ((unsigned char *)(&fog->colorInt))[3] / 255.0f;
			}
			break;
		case CGEN_WAVEFORM:
			baseColor[0] =
			baseColor[1] =
			baseColor[2] = RB_CalcWaveColorSingle( &pStage->rgbWave );
			break;
		case CGEN_ENTITY:
		case CGEN_LIGHTING_DIFFUSE_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;

				if ( alphaGen == AGEN_IDENTITY &&
					backEnd.currentEntity->e.shaderRGBA[3] == 255 )
				{
					alphaGen = AGEN_SKIP;
				}
			}
			break;
		case CGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			break;
		case CGEN_LIGHTMAPSTYLE:
			VectorScale4 (styleColors[pStage->lightmapStyle], 1.0f / 255.0f, baseColor);
			break;
		case CGEN_LIGHTING_WARZONE:
			baseColor[0] =
				baseColor[1] =
				baseColor[2] =
				baseColor[3] = 0.0f;

			vertColor[0] =
				vertColor[1] =
				vertColor[2] =
				vertColor[3] = mix(tr.identityLight, 1.0f, 0.5);
			break;
		case CGEN_IDENTITY:
		case CGEN_LIGHTING_DIFFUSE:
		case CGEN_BAD:
			break;
	}

	//
	// alphaGen
	//
	switch ( alphaGen )
	{
		case AGEN_SKIP:
			break;
		case AGEN_CONST:
			if ( rgbGen != CGEN_CONST ) {
				baseColor[3] = pStage->constantColor[3] / 255.0f;
				vertColor[3] = 0.0f;
			}
			break;
		case AGEN_WAVEFORM:
			baseColor[3] = RB_CalcWaveAlphaSingle( &pStage->alphaWave );
			vertColor[3] = 0.0f;
			break;
		case AGEN_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_VERTEX:
			if ( rgbGen != CGEN_VERTEX ) {
				baseColor[3] = 0.0f;
				vertColor[3] = 1.0f;
			}
			break;
		case AGEN_ONE_MINUS_VERTEX:
			baseColor[3] = 1.0f;
			vertColor[3] = -1.0f;
			break;
		case AGEN_IDENTITY:
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_PORTAL:
			// Done entirely in vertex program
			baseColor[3] = 1.0f;
			vertColor[3] = 0.0f;
			break;
	}

	if ( forceAlphaGen != NULL )
	{
		*forceAlphaGen = alphaGen;
	}

	if ( forceRGBGen != NULL )
	{
		*forceRGBGen = rgbGen;
	}

	// multiply color by overbrightbits if this isn't a blend
	if (tr.overbrightBits
	 && !((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR)
	 && !((blend & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR)
	 && !((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR)
	 && !((blend & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))
	{
		float scale = 1 << tr.overbrightBits;

		baseColor[0] *= scale;
		baseColor[1] *= scale;
		baseColor[2] *= scale;
		vertColor[0] *= scale;
		vertColor[1] *= scale;
		vertColor[2] *= scale;
	}

	// FIXME: find some way to implement this.
#if 0
	// if in greyscale rendering mode turn all color values into greyscale.
	if(r_greyscale->integer)
	{
		int scale;

		for(i = 0; i < tess.numVertexes; i++)
		{
			scale = (tess.svars.colors[i][0] + tess.svars.colors[i][1] + tess.svars.colors[i][2]) / 3;
			tess.svars.colors[i][0] = tess.svars.colors[i][1] = tess.svars.colors[i][2] = scale;
		}
	}
#endif
}


static void ComputeFogValues(vec4_t fogDistanceVector, vec4_t fogDepthVector, float *eyeT)
{
	return; // UQ1: Disabled Q3 fog...

	// from RB_CalcFogTexCoords()
	fog_t  *fog;
	vec3_t  local;

	if (!tess.fogNum)
		return;

	if (!r_fog->integer)
		return;

	fog = tr.world->fogs + tess.fogNum;

	VectorSubtract( backEnd.ori.origin, backEnd.viewParms.ori.origin, local );
	fogDistanceVector[0] = -backEnd.ori.modelViewMatrix[2];
	fogDistanceVector[1] = -backEnd.ori.modelViewMatrix[6];
	fogDistanceVector[2] = -backEnd.ori.modelViewMatrix[10];
	fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.ori.axis[0] );

	// scale the fog vectors based on the fog's thickness
	VectorScale4(fogDistanceVector, fog->tcScale, fogDistanceVector);

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface ) {
		fogDepthVector[0] = fog->surface[0] * backEnd.ori.axis[0][0] +
			fog->surface[1] * backEnd.ori.axis[0][1] + fog->surface[2] * backEnd.ori.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.ori.axis[1][0] +
			fog->surface[1] * backEnd.ori.axis[1][1] + fog->surface[2] * backEnd.ori.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.ori.axis[2][0] +
			fog->surface[1] * backEnd.ori.axis[2][1] + fog->surface[2] * backEnd.ori.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.ori.origin, fog->surface );

		*eyeT = DotProduct( backEnd.ori.viewOrigin, fogDepthVector ) + fogDepthVector[3];
	} else {
		*eyeT = 1;	// non-surface fog always has eye inside
	}
}


static void ComputeFogColorMask( shaderStage_t *pStage, vec4_t fogColorMask )
{
	if (r_fog->integer)
	{
		switch(pStage->adjustColorsForFog)
		{
		case ACFF_MODULATE_RGB:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] = 1.0f;
			fogColorMask[3] = 0.0f;
			break;
		case ACFF_MODULATE_ALPHA:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] = 0.0f;
			fogColorMask[3] = 1.0f;
			break;
		case ACFF_MODULATE_RGBA:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] =
				fogColorMask[3] = 1.0f;
			break;
		default:
			fogColorMask[0] =
				fogColorMask[1] =
				fogColorMask[2] =
				fogColorMask[3] = 0.0f;
			break;
		}
	}
	else
	{
		fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] =
			fogColorMask[3] = 0.0f;
	}
}

static void ProjectPshadowVBOGLSL( void ) {
#ifdef __PSHADOWS__
	//
	// TODO: Move into deferredlight glsl...
	//

	shaderCommands_t *input = &tess;

	if ( !backEnd.refdef.num_pshadows ) {
		return;
	}

	if (backEnd.depthFill
		|| (tr.viewParms.flags & VPF_CUBEMAP)
		|| (tr.viewParms.flags & VPF_DEPTHSHADOW)
		|| (tr.viewParms.flags & VPF_SHADOWPASS)
		|| (tr.viewParms.flags & VPF_EMISSIVEMAP)
		|| (tr.viewParms.flags & VPF_SKYCUBEDAY)
		|| (tr.viewParms.flags & VPF_SKYCUBENIGHT))
	{
		return;
	}

	if (backEnd.currentEntity != &tr.worldEntity)
	{// Only add shadows to world surfaces, skip all models and crap for speed...
		return;
	}

	if (backEnd.renderPass != RENDERPASS_PSHADOWS)
	{//Not drawing shadows in this pass, skip...
		return;
	}

	if (!(!backEnd.viewIsOutdoors || !SHADOWS_ENABLED || RB_NightScale() == 1.0))
	{// Also skip during day, when outdoors...
		return;
	}

	shaderProgram_t *sp = NULL;

	int useTesselation = 0;

#ifdef __PSHADOW_TESSELLATION__
	float tessInner = 0.0;
	float tessOuter = 0.0;
	float tessAlpha = 0.0;

	if (r_tessellation->integer)
	{
#ifdef __TERRAIN_TESSELATION__
		if (TERRAIN_TESSELLATION_ENABLED
			&& r_terrainTessellation->integer
			&& r_terrainTessellationMax->value >= 2.0
			&& (tess.shader->isGrass || RB_ShouldUseTerrainTessellation(tess.shader->materialType)))
		{// Always add tesselation to ground surfaces...
			tess.shader->tesselation = qfalse;

			if (TERRAIN_TESSELLATION_3D_ENABLED)
			{
				useTesselation = 3;
			}
			else
			{
				useTesselation = 2;
			}

			tessInner = max(min(r_terrainTessellationMax->value, TERRAIN_TESSELLATION_LEVEL), 2.0);
			tessOuter = tessInner;
			tessAlpha = TERRAIN_TESSELLATION_OFFSET;
		}
		else
#endif //__TERRAIN_TESSELATION__
		if (input->shader->tesselation
			&& input->shader->tesselationLevel > 1.0
			&& input->shader->tesselationAlpha != 0.0)
		{
			useTesselation = 1;
		}
	}

	if (useTesselation)
	{
		sp = &tr.pshadowShader[useTesselation];
		
		GLSL_BindProgram(sp);

		vec4_t l10;
		VectorSet4(l10, tessAlpha, tessInner, tessOuter, TERRAIN_TESSELLATION_MIN_SIZE);
		GLSL_SetUniformVec4(sp, UNIFORM_TESSELATION_INFO, l10);

		if (useTesselation == 3)
		{
			GLSL_SetUniformVec4(sp, UNIFORM_TESSELATION_3D_INFO, TERRAIN_TESSELLATION_3D_OFFSET);
		}

#ifdef __TERRAIN_TESSELATION__
		float TERRAIN_TESS_OFFSET = 0.0;

		// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
		if (TERRAIN_TESSELLATION_ENABLED
			&& r_tessellation->integer
			&& r_terrainTessellation->integer
			&& r_terrainTessellationMax->value >= 2.0)
		{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
			TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

			if (useTesselation == 3)
			{
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
			}

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADMAP, &tr.roadsMapImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADMAP, &tr.blackImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}
		}

		GLSL_SetUniformFloat(sp, UNIFORM_ZFAR, (ENABLE_OCCLUSION_CULLING && r_occlusion->integer) ? tr.occlusionZfar : backEnd.viewParms.zFar);

		vec4_t l3;
		VectorSet4(l3, 0.0, 0.0, 0.0, (tr.roadsMapImage != tr.blackImage) ? 1.0 : 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, l3);

		vec4_t l8;
		VectorSet4(l8, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);

		{
			vec4_t l12;
			VectorSet4(l12, TERRAIN_TESS_OFFSET, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
		}

		{
			extern vec3_t		MAP_INFO_MINS;
			extern vec3_t		MAP_INFO_MAXS;
			extern vec3_t		MAP_INFO_SIZE;

			vec4_t loc;
			VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

			VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

			VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
		}
#endif //__TERRAIN_TESSELATION__
	}
	else
	{
		sp = &tr.pshadowShader[0];
		GLSL_BindProgram(sp);
	}
#else //!__PSHADOW_TESSELLATION__
	sp = &tr.pshadowShader[0];
	GLSL_BindProgram(sp);

	if (r_tessellation->integer)
	{
#ifdef __TERRAIN_TESSELATION__
		if (TERRAIN_TESSELLATION_ENABLED
			&& r_terrainTessellation->integer
			&& r_terrainTessellationMax->value >= 2.0
			&& (/*r_foliage->integer && GRASS_ENABLED &&*/ (input->shader->isGrass || RB_ShouldUseTerrainTessellation(input->shader->materialType))))
		{// Always add tesselation to ground surfaces...
			useTesselation = 2;
		}
		else
#endif //__TERRAIN_TESSELATION__
		if (input->shader->tesselation
			&& input->shader->tesselationLevel > 1.0
			&& input->shader->tesselationAlpha != 0.0)
		{
			useTesselation = 1;
		}
	}
#endif //__PSHADOW_TESSELLATION__

	GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

	GL_Cull(CT_FRONT_SIDED);

	// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
	// where they aren't rendered
#ifndef __PSHADOW_TESSELLATION__
	if (useTesselation)
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHTEST_DISABLE);
	else
#else //__PSHADOW_TESSELLATION__
	if (useTesselation)
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS);
	else
#endif //__PSHADOW_TESSELLATION__
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA /*| GLS_DEPTHFUNC_LESS*/ | GLS_DEPTHFUNC_EQUAL);
	
	for ( int l = 0 ; l < backEnd.refdef.num_pshadows ; l++ ) 
	{
		pshadow_t	*ps;
		vec4_t vector;

		if ( !( tess.pshadowBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this shadow
		}

		ps = &backEnd.refdef.pshadows[l];

		if (ps->invLightPower >= 1.0)
		{
			continue;
		}

		VectorCopy(ps->lightOrigin, vector);
		vector[3] = 1.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vector);

		//VectorScale(ps->lightViewAxis[0], 1.0f / ps->viewRadius, vector);
		//GLSL_SetUniformVec3(sp, UNIFORM_LIGHTFORWARD, vector);

		VectorScale(ps->lightViewAxis[1], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, UNIFORM_LIGHTRIGHT, vector);

		VectorScale(ps->lightViewAxis[2], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, UNIFORM_LIGHTUP, vector);

		GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, ps->lightRadius);

		GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

		vec4_t l0;
		VectorSet4(l0, tr.pshadowMaps[l]->width, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL0, l0);

		vector[0] = ps->realLightOrigin[0];
		vector[1] = ps->realLightOrigin[1];
		vector[2] = ps->realLightOrigin[2];
		vector[3] = useTesselation ? TERRAIN_TESSELLATION_OFFSET + 1.0 : 0.0;
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, vector);

		vector[0] = ps->entityOrigins[0][0];
		vector[1] = ps->entityOrigins[0][1];
		vector[2] = ps->entityOrigins[0][2];
		vector[3] = ps->invLightPower;
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL2, vector);


		if (sp->isBindless)
		{
			GLSL_SetBindlessTexture(sp, UNIFORM_SHADOWMAP, &tr.pshadowMaps[l], 0);
			GLSL_BindlessUpdate(sp);
		}
		else
		{
			GLSL_SetUniformInt(sp, UNIFORM_SHADOWMAP, TB_DIFFUSEMAP);
			GL_BindToTMU(tr.pshadowMaps[l], TB_DIFFUSEMAP);
		}

		//
		// draw
		//
#ifdef __PSHADOW_TESSELLATION__
		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, useTesselation > 0 ? qtrue : qfalse);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, useTesselation > 0 ? qtrue : qfalse);
		}
#else //!__PSHADOW_TESSELLATION__
		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, qfalse);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, qfalse);
		}
#endif //__PSHADOW_TESSELLATION__

		backEnd.pc.c_totalIndexes += tess.numIndexes;
		//backEnd.pc.c_dlightIndexes += tess.numIndexes;
	}
#endif
}

static unsigned int RB_CalcShaderVertexAttribs( const shader_t *shader )
{
	unsigned int vertexAttribs = shader->vertexAttribs;

	if(glState.vertexAnimation)
	{
		//vertexAttribs &= ~ATTR_COLOR;
		vertexAttribs |= ATTR_POSITION2;
		if (vertexAttribs & ATTR_NORMAL)
		{
			vertexAttribs |= ATTR_NORMAL2;
		}
	}

	if (glState.skeletalAnimation)
	{
		vertexAttribs |= ATTR_BONE_WEIGHTS;
		vertexAttribs |= ATTR_BONE_INDEXES;
	}

	return vertexAttribs;
}

static void UpdateTexCoords ( const shaderStage_t *stage )
{
	uint32_t updateAttribs = 0;
	if ( stage->bundle[0].image[0] != NULL )
	{
		switch (stage->bundle[0].tcGen)
		{
			case TCGEN_LIGHTMAP:
			case TCGEN_LIGHTMAP1:
			case TCGEN_LIGHTMAP2:
			case TCGEN_LIGHTMAP3:
			{
				int newLightmapIndex = stage->bundle[0].tcGen - TCGEN_LIGHTMAP + 1;
				if (newLightmapIndex != glState.vertexAttribsTexCoordOffset[0])
				{
					glState.vertexAttribsTexCoordOffset[0] = newLightmapIndex;
					updateAttribs |= ATTR_TEXCOORD0;
				}

				break;
			}

			case TCGEN_TEXTURE:
				if (glState.vertexAttribsTexCoordOffset[0] != 0)
				{
					glState.vertexAttribsTexCoordOffset[0] = 0;
					updateAttribs |= ATTR_TEXCOORD0;
				}
				break;

			default:
				break;
		}
	}

	if ( stage->bundle[TB_LIGHTMAP].image[0] != NULL )
	{
		switch (stage->bundle[TB_LIGHTMAP].tcGen)
		{
			case TCGEN_LIGHTMAP:
			case TCGEN_LIGHTMAP1:
			case TCGEN_LIGHTMAP2:
			case TCGEN_LIGHTMAP3:
			{
				int newLightmapIndex = stage->bundle[TB_LIGHTMAP].tcGen - TCGEN_LIGHTMAP + 1;
				if (newLightmapIndex != glState.vertexAttribsTexCoordOffset[1])
				{
					glState.vertexAttribsTexCoordOffset[1] = newLightmapIndex;
					updateAttribs |= ATTR_TEXCOORD1;
				}

				break;
			}

			case TCGEN_TEXTURE:
				if (glState.vertexAttribsTexCoordOffset[1] != 0)
				{
					glState.vertexAttribsTexCoordOffset[1] = 0;
					updateAttribs |= ATTR_TEXCOORD1;
				}
				break;

			default:
				break;
		}
	}

	if ( updateAttribs != 0 )
	{
		GLSL_UpdateTexCoordVertexAttribPointers (updateAttribs);
	}
}


int			overlaySwayTime = 0;
qboolean	overlaySwayDown = qfalse;
float		overlaySway = 0.0;

void RB_AdvanceOverlaySway ( void )
{
#if 0 // now done using time
	if (overlaySwayTime > backEnd.refdef.time)
		return;

	if (overlaySwayDown)
	{
		overlaySway -= 0.00016;

		if (overlaySway < 0.0)
		{
			overlaySway += 0.00032;
			overlaySwayDown = qfalse;
		}
	}
	else
	{
		overlaySway += 0.00016;

		if (overlaySway > 0.0016)
		{
			overlaySway -= 0.00032;
			overlaySwayDown = qtrue;
		}
	}

	overlaySwayTime = backEnd.refdef.time + 50;
#endif
}

void RB_PBR_DefaultsForMaterial(float *settings, int MATERIAL_TYPE)
{
	float materialType = (float)MATERIAL_TYPE;
	float specularScale = 0.0;
	float cubemapScale = 0.9;
	float parallaxScale = 0.0;

	switch (MATERIAL_TYPE)
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		specularScale = 1.0;
		cubemapScale = 0.7;
		parallaxScale = 1.5;
		break;
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		specularScale = 0.75;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		specularScale = 0.75;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		specularScale = 0.65;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_CARPET:			// 27			// lush carpet
		specularScale = 0.25;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		specularScale = 0.30;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_ROCK:				// 23			//
	case MATERIAL_STONE:
		specularScale = 0.22;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		specularScale = 0.56;
		cubemapScale = 0.15;
		parallaxScale = 1.5;
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
	case MATERIAL_TREEBARK:
		specularScale = 0.05;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		specularScale = 0.025;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_POLISHEDWOOD:
		specularScale = 0.56;
		cubemapScale = 0.15;
		parallaxScale = 1.5;
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		specularScale = 0.98;
		cubemapScale = 0.98;
		parallaxScale = 1.5;
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- UQ1: Used for weapons to force lower parallax and high reflection...
		specularScale = 1.0;
		cubemapScale = 1.0;
		materialType = (float)MATERIAL_HOLLOWMETAL;
		parallaxScale = 1.5;
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		specularScale = 0.35;
		cubemapScale = 0.0;
		parallaxScale = 0.0;
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		specularScale = 0.95;
		cubemapScale = 0.0;
		parallaxScale = 0.0; // GreenLeaves should NEVER be parallaxed.. It's used for surfaces with an alpha channel and parallax screws it up...
		break;
	case MATERIAL_BIRD:
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		specularScale = 0.45;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		specularScale = 0.35;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		specularScale = 0.65;
		cubemapScale = 0.26;
		parallaxScale = 1.5;
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		specularScale = 0.75;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		specularScale = 0.25;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		specularScale = 0.15;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		specularScale = 0.375;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		specularScale = 0.25;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		specularScale = 0.25;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_PLASTIC:			// 25			//
		specularScale = 0.58;
		cubemapScale = 0.24;
		parallaxScale = 1.5;
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		specularScale = 0.3;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
		specularScale = 0.93;
		cubemapScale = 0.37;
		parallaxScale = 1.0;
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		specularScale = 0.5;
		cubemapScale = 0.46;
		parallaxScale = 1.5;
		break;
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		specularScale = 0.75;
		cubemapScale = 0.37;
		parallaxScale = 2.0;
		break;
	case MATERIAL_GLASS:			// 10			//
		specularScale = 0.95;
		cubemapScale = 0.27;
		parallaxScale = 1.0;
		break;
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
		specularScale = 0.93;
		cubemapScale = 0.23;
		parallaxScale = 1.0;
		break;
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		specularScale = 0.92;
		cubemapScale = 0.42;
		parallaxScale = 1.5;
		break;
		break;
	case MATERIAL_PUDDLE:
		specularScale = 1.0;
		cubemapScale = 0.7;
		parallaxScale = 1.5;
		break;
	case MATERIAL_LAVA:
		specularScale = 0.1;
		cubemapScale = 0.0;
		parallaxScale = 1.5;
		break;
	case MATERIAL_EFX:
	case MATERIAL_BLASTERBOLT:
	case MATERIAL_FIRE:
	case MATERIAL_SMOKE:
	case MATERIAL_MAGIC_PARTICLES:
	case MATERIAL_MAGIC_PARTICLES_TREE:
	case MATERIAL_FIREFLIES:
	case MATERIAL_PORTAL:
	case MATERIAL_MENU_BACKGROUND:
		specularScale = 0.0;
		cubemapScale = 0.0;
		parallaxScale = 0.0;
		break;
	case MATERIAL_SKYSCRAPER:
		specularScale = 0.5;
		cubemapScale = 0.46;
		parallaxScale = 1.5;
		break;
	default:
		specularScale = 0.15;
		cubemapScale = 0.0;
		parallaxScale = 1.0;
		break;
	}

	settings[0] = materialType;
	settings[1] = specularScale;
	settings[2] = cubemapScale;
	settings[3] = parallaxScale;
}

extern float		LEAF_ALPHA_MULTIPLIER;

extern float		MAP_GLOW_MULTIPLIER;
extern float		MAP_GLOW_MULTIPLIER_NIGHT;
extern float		MAP_WATER_LEVEL;
extern float		MAP_INFO_MAXSIZE;
extern vec3_t		MAP_INFO_MINS;
extern vec3_t		MAP_INFO_MAXS;
extern vec3_t		MAP_INFO_SIZE;

void RB_SetMaterialBasedProperties(shaderProgram_t *sp, shaderStage_t *pStage, int stageNum, int IS_DEPTH_PASS)
{
	float	materialType = 0.0;
	float	hasOverlay = 0.0;
	float	doSway = 0.0;
	float	hasSplatMap1 = 0;
	float	hasSplatMap2 = 0;
	float	hasSplatMap3 = 0;
	float	hasSteepMap = 0;
	float	hasSteepMap1 = 0;
	float	hasSteepMap2 = 0;
	float	hasSteepMap3 = 0;
	float	hasWaterEdgeMap = 0;
	float	hasRoadMap = 0;
	float	hasNormalMap = 0;
	float	hasEnvMap = 0;

	qboolean isSky = tess.shader->isSky;

	//vec4_t materialSettings;
	//RB_PBR_DefaultsForMaterial(materialSettings, tess.shader->materialType);

	if (pStage->isWater && r_glslWater->integer && WATER_ENABLED)
	{
		materialType = (float)MATERIAL_WATER;
	}
	else if (tess.shader == tr.sunShader)
	{// SPECIAL MATERIAL TYPE FOR SUN
		materialType = 1025.0;
	}
	else if (isSky)
	{// SPECIAL MATERIAL TYPE FOR SKY
		materialType = 1024.0;
	}
	else
	{
		materialType = tess.shader->materialType;// materialSettings[0];
	}

	if (!IS_DEPTH_PASS)
	{
		if (r_normalMapping->integer >= 2
			&& pStage->bundle[TB_NORMALMAP].image[0])
		{
			hasNormalMap = 1.0;
		}

		if (pStage->bundle[TB_OVERLAYMAP].image[0])
		{
			hasOverlay = 1.0;
		}

		if (pStage->bundle[TB_ENVMAP].image[0])
		{
			hasEnvMap = Q_clamp(0.0, pStage->envmapStrength, 1.0);
		}

		if (pStage->bundle[TB_STEEPMAP].image[0])
		{
			hasSteepMap = 1.0;
		}

		if (pStage->bundle[TB_STEEPMAP1].image[0])
		{
			hasSteepMap1 = 1.0;
		}

		if (pStage->bundle[TB_STEEPMAP2].image[0])
		{
			hasSteepMap2 = 1.0;
		}

		if (pStage->bundle[TB_STEEPMAP3].image[0])
		{
			hasSteepMap3 = 1.0;
		}

		if (pStage->bundle[TB_WATER_EDGE_MAP].image[0])
		{
			hasWaterEdgeMap = 1.0;
		}

		if (pStage->bundle[TB_SPLATMAP1].image[0])
		{
			hasSplatMap1 = 1.0;
		}

		if (pStage->bundle[TB_SPLATMAP2].image[0])
		{
			hasSplatMap2 = 1.0;
		}

		if (pStage->bundle[TB_SPLATMAP3].image[0])
		{
			hasSplatMap3 = 1.0;
		}

		if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
		{// Seems we have roads for this map...
			hasRoadMap = 1.0;
		}

		if (pStage->bundle[TB_ROOFMAP].image[0])
		{
			hasSteepMap1 = 1.0;
		}

		if (pStage->isFoliage)
		{
			doSway = 0.7;
		}

		vec4_t	local1;
		VectorSet4(local1, MAP_INFO_MAXSIZE, doSway, overlaySway, materialType);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, local1);

		vec4_t local2;
		VectorSet4(local2, hasWaterEdgeMap, hasNormalMap, WATER_WAVE_HEIGHT, MAP_WATER_LEVEL);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL2, local2);

		vec4_t local3;
		VectorSet4(local3, hasSplatMap1, hasSplatMap2, hasSplatMap3, hasRoadMap);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, local3);

		vec4_t local4;
		float dayNightGlowFactor = mix(MAP_GLOW_MULTIPLIER, MAP_GLOW_MULTIPLIER_NIGHT, RB_NightScale());
		float glowPower = (backEnd.currentEntity == &tr.worldEntity) ? r_glowStrength->value * tess.shader->glowStrength * dayNightGlowFactor * pStage->glowStrength : r_glowStrength->value * tess.shader->glowStrength * pStage->glowStrength;
		VectorSet4(local4, /*(float)stageNum*/SPLATMAP_CONTROL_SCALE, glowPower, r_showsplat->value, max(tess.shader->glowVibrancy, pStage->glowVibrancy) * r_glowVibrancy->value);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL4, local4);

		vec4_t local5;
		VectorSet4(local5, hasOverlay, hasEnvMap, 0.0, LEAF_ALPHA_MULTIPLIER);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL5, local5); // dayNightEnabled, nightScale, skyDirection, LEAF_ALPHA_MULTIPLIER

		vec4_t local13;
		VectorSet4(local13, hasSteepMap, hasSteepMap1, hasSteepMap2, hasSteepMap3);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL13, local13);
	}
	else
	{// Don't waste time on unneeded stuff... Absolute minimum shader complexity...
		if (pStage->isFoliage)
		{
			doSway = 0.7;
		}

		vec4_t	local1;
		VectorSet4(local1, (IS_DEPTH_PASS == 2) ? (TERRAIN_TESSELLATION_OFFSET + 1.0) : 0.0, doSway, overlaySway, materialType);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, local1);

		vec4_t vector;
		VectorSet4(vector, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL2, vector); // hasWaterEdgeMap, hasNormalMap, 0.0, MAP_WATER_LEVEL
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, vector); // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasRoadMap
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL4, vector); // stageNum, glowStrength, r_showsplat, glowVibrancy
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL13, vector); // hasSteepMap, hasSteepMap1, hasSteepMap2, hasSteepMap3
		
		vec4_t local5;
		VectorSet4(local5, 0.0, 0.0, 0.0, LEAF_ALPHA_MULTIPLIER);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL5, local5); // dayNightEnabled, nightScale, skyDirection, LEAF_ALPHA_MULTIPLIER
	}

	GLSL_SetUniformFloat(sp, UNIFORM_TIME, backEnd.refdef.floatTime);
}

void RB_SetStageImageDimensions(shaderProgram_t *sp, shaderStage_t *pStage)
{
	vec2_t dimensions = { 0.0 };

	if (pStage->bundle[TB_DIFFUSEMAP].image[0])
	{
		dimensions[0] = pStage->bundle[TB_DIFFUSEMAP].image[0]->width;
		dimensions[1] = pStage->bundle[TB_DIFFUSEMAP].image[0]->height;
	}

	GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
}

int			NUM_CLOSE_LIGHTS = 0;
int			CLOSEST_LIGHTS[MAX_DEFERRED_LIGHTS] = {0};
vec3_t		CLOSEST_LIGHTS_POSITIONS[MAX_DEFERRED_LIGHTS] = {0};
vec2_t		CLOSEST_LIGHTS_SCREEN_POSITIONS[MAX_DEFERRED_LIGHTS];
float		CLOSEST_LIGHTS_DISTANCES[MAX_DEFERRED_LIGHTS] = {0};
float		CLOSEST_LIGHTS_HEIGHTSCALES[MAX_DEFERRED_LIGHTS] = { 0 };
float		CLOSEST_LIGHTS_CONEANGLES[MAX_DEFERRED_LIGHTS] = { 0 };
vec3_t		CLOSEST_LIGHTS_CONEDIRECTIONS[MAX_DEFERRED_LIGHTS] = { 0 };
vec3_t		CLOSEST_LIGHTS_COLORS[MAX_DEFERRED_LIGHTS] = {0};

extern void WorldCoordToScreenCoord(vec3_t origin, float *x, float *y);
extern qboolean Volumetric_Visible(vec3_t from, vec3_t to, qboolean isSun);
extern void Volumetric_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, const int passEntityNum, const int contentmask);
void RB_UpdateCloseLights ( void )
{
	if (!CLOSE_LIGHTS_UPDATE) return; // Already done for this frame...

	NUM_CLOSE_LIGHTS = 0;

	//int MAX_CLOSE_LIGHTS = MAX_DEFERRED_LIGHTS;
	int MAX_CLOSE_LIGHTS = r_lowVram->integer ? min(r_maxDeferredLights->integer, 8.0) : min(r_maxDeferredLights->integer, MAX_DEFERRED_LIGHTS);

	for ( int l = 0 ; l < backEnd.refdef.num_dlights ; l++ )
	{
		dlight_t	*dl = &backEnd.refdef.dlights[l];

		if (dl->color[0] < 0.0 && dl->color[1] < 0.0 && dl->color[2] < 0.0)
		{// Surface glow light... But has no color assigned...
			continue;
		}

		float distance = Distance(backEnd.refdef.vieworg, dl->origin);

		if (distance > MAX_DEFERRED_LIGHT_RANGE) continue; // Don't even check at this range. Traces are costly!

		if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer)
		{// Check occlusion zfar distance as well...
			if (distance > Q_min(tr.occlusionZfar * 1.75, tr.occlusionOriginalZfar))
			{
				continue;
			}
		}

		if (NUM_CLOSE_LIGHTS < MAX_CLOSE_LIGHTS)
		{// Have free light slots for a new light...
#ifdef __LIGHT_OCCLUSION__
			float x, y;
			WorldCoordToScreenCoord(dl->origin, &x, &y);
#endif //__LIGHT_OCCLUSION__

			CLOSEST_LIGHTS[NUM_CLOSE_LIGHTS] = l;
			VectorCopy(dl->origin, CLOSEST_LIGHTS_POSITIONS[NUM_CLOSE_LIGHTS]);
			CLOSEST_LIGHTS_DISTANCES[NUM_CLOSE_LIGHTS] = dl->radius;
			CLOSEST_LIGHTS_HEIGHTSCALES[NUM_CLOSE_LIGHTS] = dl->heightScale;
			CLOSEST_LIGHTS_COLORS[NUM_CLOSE_LIGHTS][0] = dl->color[0];
			CLOSEST_LIGHTS_COLORS[NUM_CLOSE_LIGHTS][1] = dl->color[1];
			CLOSEST_LIGHTS_COLORS[NUM_CLOSE_LIGHTS][2] = dl->color[2];
#ifdef __LIGHT_OCCLUSION__
			CLOSEST_LIGHTS_SCREEN_POSITIONS[NUM_CLOSE_LIGHTS][0] = x;
			CLOSEST_LIGHTS_SCREEN_POSITIONS[NUM_CLOSE_LIGHTS][1] = y;
#endif //__LIGHT_OCCLUSION__
			CLOSEST_LIGHTS_CONEANGLES[NUM_CLOSE_LIGHTS] = dl->coneAngle;
			VectorCopy(dl->coneDirection, CLOSEST_LIGHTS_CONEDIRECTIONS[NUM_CLOSE_LIGHTS]);
			NUM_CLOSE_LIGHTS++;
			continue;
		}
		else
		{// See if this is closer then one of our other lights...
			int		farthest_light = 0;
			float	farthest_distance = 0.0;

			for (int i = 0; i < NUM_CLOSE_LIGHTS; i++)
			{// Find the most distance light in our current list to replace, if this new option is closer...
				dlight_t	*thisLight = &backEnd.refdef.dlights[CLOSEST_LIGHTS[i]];
				float		dist = Distance(thisLight->origin, backEnd.refdef.vieworg);

				if (dist > farthest_distance)
				{// This one is further!
					farthest_light = i;
					farthest_distance = dist;
					//break;
				}
			}

			if (distance < farthest_distance)
			{// This light is closer. Replace this one in our array of closest lights...
#ifdef __LIGHT_OCCLUSION__
				float x, y;
				WorldCoordToScreenCoord(dl->origin, &x, &y);
#endif //__LIGHT_OCCLUSION__

				CLOSEST_LIGHTS[farthest_light] = l;
				VectorCopy(dl->origin, CLOSEST_LIGHTS_POSITIONS[farthest_light]);
				CLOSEST_LIGHTS_DISTANCES[farthest_light] = dl->radius;
				CLOSEST_LIGHTS_HEIGHTSCALES[farthest_light] = dl->heightScale;
				CLOSEST_LIGHTS_COLORS[farthest_light][0] = dl->color[0];
				CLOSEST_LIGHTS_COLORS[farthest_light][1] = dl->color[1];
				CLOSEST_LIGHTS_COLORS[farthest_light][2] = dl->color[2];
#ifdef __LIGHT_OCCLUSION__
				CLOSEST_LIGHTS_SCREEN_POSITIONS[farthest_light][0] = x;
				CLOSEST_LIGHTS_SCREEN_POSITIONS[farthest_light][1] = y;
#endif //__LIGHT_OCCLUSION__
				CLOSEST_LIGHTS_CONEANGLES[farthest_light] = dl->coneAngle;
				VectorCopy(dl->coneDirection, CLOSEST_LIGHTS_CONEDIRECTIONS[farthest_light]);
			}
		}
	}

	for (int i = 0; i < NUM_CLOSE_LIGHTS; i++)
	{
		if (CLOSEST_LIGHTS_DISTANCES[i] < 0.0)
		{// Remove volume light markers...
			CLOSEST_LIGHTS_DISTANCES[i] = -CLOSEST_LIGHTS_DISTANCES[i];
		}
	}
	int NUM_LIGHTS = r_lowVram->integer ? min(NUM_CLOSE_LIGHTS, min(r_maxDeferredLights->integer, 8.0)) : min(NUM_CLOSE_LIGHTS, min(r_maxDeferredLights->integer, MAX_DEFERRED_LIGHTS));

	//if (NUM_CLOSE_LIGHTS >= min(r_maxDeferredLights->integer, MAX_DEFERRED_LIGHTS))
	{// Sort them by radius...
		for (int i = 0; i < NUM_CLOSE_LIGHTS; i++)
		{
			for (int j = 0; j < NUM_CLOSE_LIGHTS; j++)
			{
				if (i == j) continue;

				if (CLOSEST_LIGHTS_DISTANCES[j] > CLOSEST_LIGHTS_DISTANCES[i])
				{
					int lightNum = CLOSEST_LIGHTS[i];
					CLOSEST_LIGHTS[i] = CLOSEST_LIGHTS[j];
					CLOSEST_LIGHTS[j] = lightNum;

					vec3_t lightOrg;
					VectorCopy(CLOSEST_LIGHTS_POSITIONS[i], lightOrg);
					VectorCopy(CLOSEST_LIGHTS_POSITIONS[j], CLOSEST_LIGHTS_POSITIONS[i]);
					VectorCopy(lightOrg, CLOSEST_LIGHTS_POSITIONS[j]);

					float lightDist = CLOSEST_LIGHTS_DISTANCES[i];
					CLOSEST_LIGHTS_DISTANCES[i] = CLOSEST_LIGHTS_DISTANCES[j];
					CLOSEST_LIGHTS_DISTANCES[j] = lightDist;

					float lightHS = CLOSEST_LIGHTS_HEIGHTSCALES[i];
					CLOSEST_LIGHTS_HEIGHTSCALES[j] = CLOSEST_LIGHTS_HEIGHTSCALES[i];
					CLOSEST_LIGHTS_HEIGHTSCALES[i] = lightHS;

					vec3_t lightColor;
					VectorCopy(CLOSEST_LIGHTS_COLORS[i], lightColor);
					VectorCopy(CLOSEST_LIGHTS_COLORS[j], CLOSEST_LIGHTS_COLORS[i]);
					VectorCopy(lightColor, CLOSEST_LIGHTS_COLORS[j]);

					vec3_t lightDirection;
					VectorCopy(CLOSEST_LIGHTS_CONEDIRECTIONS[i], lightDirection);
					VectorCopy(CLOSEST_LIGHTS_CONEDIRECTIONS[j], CLOSEST_LIGHTS_CONEDIRECTIONS[i]);
					VectorCopy(lightDirection, CLOSEST_LIGHTS_CONEDIRECTIONS[j]);
				}
			}
		}
	}

#ifdef __CALCULATE_LIGHTDIR_FROM_LIGHT_AVERAGES__
	//
	// Make an average light color and direction... Default to sun direction, offset by local lighting...
	//
	backEnd.viewParms.emissiveLightDirection[3] = 0.0;
	VectorCopy(backEnd.refdef.sunDir, backEnd.viewParms.emissiveLightDirection);
	
	vec3_t out;
	float dist = 4096.0;
	VectorMA(backEnd.refdef.vieworg, dist, backEnd.refdef.sunDir, out);
	VectorCopy(out, backEnd.viewParms.emissiveLightOrigin);

	VectorCopy(backEnd.refdef.sunCol, backEnd.viewParms.emissiveLightColor);

	// Sun counts as 2x power, so we bias toward it...
	VectorScale(backEnd.viewParms.emissiveLightDirection, 2.0, backEnd.viewParms.emissiveLightDirection);
	VectorScale(backEnd.viewParms.emissiveLightOrigin, 2.0, backEnd.viewParms.emissiveLightOrigin);
	VectorScale(backEnd.viewParms.emissiveLightColor, 2.0, backEnd.viewParms.emissiveLightColor);

	float nightScale = RB_NightScale();

	if (nightScale > 0.0)
	{
		float nightOffset = mix(1.0, -1.0, nightScale);
		VectorScale(backEnd.viewParms.emissiveLightDirection, nightOffset, backEnd.viewParms.emissiveLightDirection);
		VectorScale(backEnd.viewParms.emissiveLightOrigin, nightOffset, backEnd.viewParms.emissiveLightOrigin);
	}

	for (int i = 0; i < NUM_CLOSE_LIGHTS; i++)
	{// emissives...
		vec3_t dir;
		VectorSubtract(backEnd.refdef.vieworg, CLOSEST_LIGHTS_POSITIONS[i], dir);
		VectorNormalizeFast(dir);

		VectorAdd(backEnd.viewParms.emissiveLightDirection, dir, backEnd.viewParms.emissiveLightDirection);
		VectorAdd(backEnd.viewParms.emissiveLightOrigin, CLOSEST_LIGHTS_POSITIONS[i], backEnd.viewParms.emissiveLightOrigin);
		VectorAdd(backEnd.viewParms.emissiveLightColor, CLOSEST_LIGHTS_COLORS[i], backEnd.viewParms.emissiveLightColor);
	}

	float numLightsAveragedWeighted = 2.0 + NUM_CLOSE_LIGHTS;
	backEnd.viewParms.emissiveLightDirection[0] /= numLightsAveragedWeighted;
	backEnd.viewParms.emissiveLightDirection[1] /= numLightsAveragedWeighted;
	backEnd.viewParms.emissiveLightDirection[2] /= numLightsAveragedWeighted;

	backEnd.viewParms.emissiveLightOrigin[0] /= numLightsAveragedWeighted;
	backEnd.viewParms.emissiveLightOrigin[1] /= numLightsAveragedWeighted;
	backEnd.viewParms.emissiveLightOrigin[2] /= numLightsAveragedWeighted;

	backEnd.viewParms.emissiveLightColor[0] /= numLightsAveragedWeighted;
	backEnd.viewParms.emissiveLightColor[1] /= numLightsAveragedWeighted;
	backEnd.viewParms.emissiveLightColor[2] /= numLightsAveragedWeighted;

	// Bias the vibrancy of the average light color to be mostly white...
	mix(backEnd.viewParms.emissiveLightColor[0], 1.0, 0.8);
	mix(backEnd.viewParms.emissiveLightColor[1], 1.0, 0.8);
	mix(backEnd.viewParms.emissiveLightColor[2], 1.0, 0.8);
#else //!__CALCULATE_LIGHTDIR_FROM_LIGHT_AVERAGES__
	//
	// Use sun like rend2 does...
	//
	VectorCopy(backEnd.refdef.sunCol, backEnd.viewParms.emissiveLightColor);
	VectorCopy(backEnd.refdef.sunDir, backEnd.viewParms.emissiveLightDirection);

	float dist = 4096.0;
	VectorMA(backEnd.refdef.vieworg, dist, backEnd.refdef.sunDir, backEnd.viewParms.emissiveLightOrigin);
#endif //__CALCULATE_LIGHTDIR_FROM_LIGHT_AVERAGES__

	CLOSE_LIGHTS_UPDATE = qfalse;
}

float waveTime = 0.5;
float waveFreq = 0.1;

extern void GLSL_AttachTextures( void );
extern void GLSL_AttachGenericTextures( void );
extern void GLSL_AttachGlowTextures( void );
extern void GLSL_AttachWaterTextures( void );
//extern void GLSL_AttachWaterTextures2( void );

extern qboolean ALLOW_GL_400;

static void RB_IterateStagesGeneric( shaderCommands_t *input )
{
	if (r_cullNoDraws->integer && (tess.shader->surfaceFlags & SURF_NODRAW))
	{
		return;
	}

	if (!ALLOW_PROCEDURALS_ON_MODELS 
		&& backEnd.currentEntity != &tr.worldEntity 
		&& backEnd.currentEntity != &backEnd.entity2D 
		&& backEnd.renderPass != RENDERPASS_NONE)
	{// Rendering of procedurals on model surfaces is disabled by default, but ALLOW_PROCEDURALS_ON_MODELS=1 in the [FIXES] section of mapinfo can be used to override this.
		return;
	}

	vec4_t	fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
	float	eyeT = 0;
	int		deformGen;
	float	deformParams[7];

	int useTesselation = 0;
	qboolean isWorld = qfalse;
	qboolean isWater = qfalse;
	qboolean isFakeGrass = qfalse;
	qboolean isGrassPatches = qfalse;
	qboolean isGrass = qfalse;
	qboolean isGrass2 = qfalse;
	qboolean isGrass3 = qfalse;
	qboolean isGrass4 = qfalse;
	qboolean isVines = qfalse;
	qboolean isGroundFoliage = qfalse;
	qboolean isMist = qfalse;
	qboolean isGlass = qfalse;
	qboolean isPush = qfalse;
	qboolean isPull = qfalse;
	qboolean isCloak = qfalse;
	qboolean isEmissiveBlack = qfalse;

	float tessInner = 0.0;
	float tessOuter = 0.0;
	float tessAlpha = 0.0;

	int IS_DEPTH_PASS = 0;

	if ((tess.shader->isIndoor) && (tr.viewParms.flags & VPF_SHADOWPASS))
	{// Don't draw stuff marked as inside to sun shadow map...
		return;
	}

	ComputeDeformValues(&deformGen, deformParams);

	ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT);

#ifdef __CHEAP_VERTS__
	if (/*backEnd.currentEntity == &tr.worldEntity &&*/ !tess.useInternalVBO)
	{// World VBO's no longer contain attr_color to save memory, so we need to let the GLSL shaders know about it.
		isWorld = qtrue;
	}
#endif //__CHEAP_VERTS__

	//
	// UQ1: I think we only need to do all these once, not per stage... Waste of FPS!
	//

	if (backEnd.currentEntity == &backEnd.entity2D)
	{

	}
	else
#if 1
	if (backEnd.depthFill)
	{
		IS_DEPTH_PASS = 1;

#if 0
		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS_PATCHES_ENABLED
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& (RB_ShouldUseGeometryGrassPatches(tess.shader->materialType)))
		{
			if (r_foliageShadows->integer > 1 || GRASS_PATCHES_HEIGHT * GRASS_PATCHES_SIZE_MULTIPLIER_COMMON >= 20)
			{// Only when foliageShadows >= 2 will we do them on grass... (unless the grass is fairly large)
				isGrassPatches = qtrue;
			}
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS_ENABLED
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& (tess.shader->isGrass || RB_ShouldUseGeometryGrass(tess.shader->materialType)))
		{
			if (r_foliageShadows->integer > 1 || GRASS_HEIGHT * GRASS_SIZE_MULTIPLIER_COMMON >= 20)
			{// Only when foliageShadows >= 2 will we do them on grass... (unless the grass is fairly large)
				isGrass = qtrue;
			}

			tess.shader->isGrass = qtrue; // Cache to speed up future checks...
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS2_ENABLED
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryGrass2(tess.shader->materialType))
		{
			if (r_foliageShadows->integer > 1 || GRASS2_HEIGHT * GRASS2_SIZE_MULTIPLIER_COMMON >= 20)
			{// Only when foliageShadows >= 2 will we do them on grass... (unless the grass is fairly large)
				isGrass2 = qtrue;
			}
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS3_ENABLED
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryGrass3(tess.shader->materialType))
		{
			if (r_foliageShadows->integer > 1 || GRASS3_HEIGHT * GRASS3_SIZE_MULTIPLIER_COMMON >= 20)
			{// Only when foliageShadows >= 2 will we do them on grass... (unless the grass is fairly large)
				isGrass3 = qtrue;
			}
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS4_ENABLED
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryGrass4(tess.shader->materialType))
		{
			if (r_foliageShadows->integer > 1 || GRASS4_HEIGHT * GRASS4_SIZE_MULTIPLIER_COMMON >= 20)
			{// Only when foliageShadows >= 2 will we do them on grass... (unless the grass is fairly large)
				isGrass4 = qtrue;
			}
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& FOLIAGE_ENABLED
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& (tess.shader->isGroundFoliage || RB_ShouldUseGeometryFoliage(tess.shader->materialType)))
		{
			if (r_foliageShadows->integer > 1)
			{// Only when foliageShadows >= 2 will we do shadows on groundcover...
				isGroundFoliage = qtrue;
			}

			tess.shader->isGroundFoliage = qtrue; // Cache to speed up future checks...
		}
#endif

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& VINES_ENABLED
			//&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryVines(tess.shader->materialType))
		{
			isVines = qtrue;
		}
	}
	else if ((tr.viewParms.flags & VPF_CUBEMAP)
		|| (tr.viewParms.flags & VPF_DEPTHSHADOW)
		|| (tr.viewParms.flags & VPF_SHADOWPASS)
		|| (tr.viewParms.flags & VPF_EMISSIVEMAP)
		|| (tr.viewParms.flags & VPF_SKYCUBEDAY)
		|| (tr.viewParms.flags & VPF_SKYCUBENIGHT))
	{
#if 0
		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS_PATCHES_ENABLED
			&& ((tr.viewParms.flags & VPF_DEPTHSHADOW) || (tr.viewParms.flags & VPF_SHADOWPASS))
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& (RB_ShouldUseGeometryGrassPatches(tess.shader->materialType)))
		{
			isGrassPatches = qtrue;
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS_ENABLED
			&& ((tr.viewParms.flags & VPF_DEPTHSHADOW) || (tr.viewParms.flags & VPF_SHADOWPASS))
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& (tess.shader->isGrass || RB_ShouldUseGeometryGrass(tess.shader->materialType)))
		{
			isGrass = qtrue;
			tess.shader->isGrass = qtrue; // Cache to speed up future checks...
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS2_ENABLED
			&& ((tr.viewParms.flags & VPF_DEPTHSHADOW) || (tr.viewParms.flags & VPF_SHADOWPASS))
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryGrass2(tess.shader->materialType))
		{
			isGrass2 = qtrue;
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS3_ENABLED
			&& ((tr.viewParms.flags & VPF_DEPTHSHADOW) || (tr.viewParms.flags & VPF_SHADOWPASS))
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryGrass3(tess.shader->materialType))
		{
			isGrass3 = qtrue;
		}

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& GRASS4_ENABLED
			&& ((tr.viewParms.flags & VPF_DEPTHSHADOW) || (tr.viewParms.flags & VPF_SHADOWPASS))
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryGrass4(tess.shader->materialType))
		{
			isGrass4 = qtrue;
		}
		
		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& FOLIAGE_ENABLED
			&& ((tr.viewParms.flags & VPF_DEPTHSHADOW) || (tr.viewParms.flags & VPF_SHADOWPASS))
			&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& (tess.shader->isGroundFoliage || RB_ShouldUseGeometryFoliage(tess.shader->materialType)))
		{
			isGroundFoliage = qtrue;
			tess.shader->isGroundFoliage = qtrue; // Cache to speed up future checks...
		}
#endif

		if (r_foliage->integer
			&& r_foliageShadows->integer
			&& VINES_ENABLED
			&& ((tr.viewParms.flags & VPF_DEPTHSHADOW) || (tr.viewParms.flags & VPF_SHADOWPASS))
			//&& (glState.currentFBO == tr.sunShadowFbo[0] || glState.currentFBO == tr.sunShadowFbo[1])
			&& RB_ShouldUseGeometryVines(tess.shader->materialType))
		{
			isVines = qtrue;
		}
	}
	else
#endif
	{
		if (r_foliage->integer
			&& GRASS_PATCHES_ENABLED
			&& (RB_ShouldUseGeometryGrassPatches(tess.shader->materialType)))
		{
			isGrassPatches = qtrue;
		}

		if (r_foliage->integer
			&& GRASS_ENABLED
			&& (tess.shader->isGrass || RB_ShouldUseGeometryGrass(tess.shader->materialType)))
		{
			isGrass = qtrue;
			isFakeGrass = qtrue;
			tess.shader->isGrass = qtrue; // Cache to speed up future checks...
		}

		if (r_foliage->integer
			&& GRASS2_ENABLED
			&& RB_ShouldUseGeometryGrass2(tess.shader->materialType))
		{
			isGrass2 = qtrue;
		}

		if (r_foliage->integer
			&& GRASS3_ENABLED
			&& RB_ShouldUseGeometryGrass3(tess.shader->materialType))
		{
			isGrass3 = qtrue;
		}

		if (r_foliage->integer
			&& GRASS4_ENABLED
			&& RB_ShouldUseGeometryGrass4(tess.shader->materialType))
		{
			isGrass4 = qtrue;
		}
		
		if (r_foliage->integer
			&& FOLIAGE_ENABLED
			&& (tess.shader->isGroundFoliage || RB_ShouldUseGeometryFoliage(tess.shader->materialType)))
		{
			isGroundFoliage = qtrue;
			tess.shader->isGroundFoliage = qtrue; // Cache to speed up future checks...
		}

		if (r_foliage->integer
			&& VINES_ENABLED
			&& RB_ShouldUseGeometryVines(tess.shader->materialType))
		{
			isVines = qtrue;
		}

		if (MIST_ENABLED 
			&& MIST_TEXTURE != NULL 
			&& RB_ShouldUseGeometryMist(tess.shader->materialType))
		{
			isMist = qtrue;
		}
	}

	if (backEnd.currentEntity == &backEnd.entity2D && backEnd.renderPass == RENDERPASS_POSTPROCESS)
	{

	}
	else if ((tr.viewParms.flags & VPF_DEPTHSHADOW)
		&& backEnd.renderPass != RENDERPASS_NONE
		&& backEnd.renderPass != RENDERPASS_VINES
		&& backEnd.renderPass != RENDERPASS_POSTPROCESS)
	{
		return;
	}
	else if (!isGrassPatches && backEnd.renderPass == RENDERPASS_GRASS_PATCHES)
	{
		return;
	}
	else if (!isGrass && backEnd.renderPass == RENDERPASS_GRASS)
	{
		return;
	}
	else if (!isGrass2 && backEnd.renderPass == RENDERPASS_GRASS2)
	{
		return;
	}
	else if (!isGrass3 && backEnd.renderPass == RENDERPASS_GRASS3)
	{
		return;
	}
	else if (!isGrass4 && backEnd.renderPass == RENDERPASS_GRASS4)
	{
		return;
	}
	else if (!isGroundFoliage && backEnd.renderPass == RENDERPASS_GROUNDFOLIAGE)
	{
		return;
	}
	else if (!isVines && backEnd.renderPass == RENDERPASS_VINES)
	{
		return;
	}
	else if (!isMist && backEnd.renderPass == RENDERPASS_MIST)
	{
		return;
	}
	else if (backEnd.renderPass == RENDERPASS_PSHADOWS)
	{
		return;
	}

	if (r_tessellation->integer)
	{
#ifdef __TERRAIN_TESSELATION__
		if (TERRAIN_TESSELLATION_ENABLED
			&& !(tr.viewParms.flags & VPF_DEPTHSHADOW)
			&& r_terrainTessellation->integer
			&& r_terrainTessellationMax->value >= 2.0
			&& (tess.shader->isGrass || RB_ShouldUseTerrainTessellation(tess.shader->materialType)))
		{// Always add tesselation to ground surfaces...
			tess.shader->tesselation = qfalse;

			if (IS_DEPTH_PASS && tr.roadsMapImage != tr.blackImage) // FIXME: Need to remove this, but roads are being wierd with differences between depth and splat draws...
			{// When doing depth pass, we simply lower the terrain by the max tessellation amount, reduces pixel culling, but is good enough and faster then tesselating the depth pass.
				IS_DEPTH_PASS = 2;
			}
			else
			{
				float TERRAIN_TESS_OFFSET = 0;

				if (TERRAIN_TESSELLATION_3D_ENABLED)
				{
					useTesselation = 3;
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
				else
				{
					useTesselation = 2;
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;
				}

				tessInner = max(min(r_terrainTessellationMax->value, TERRAIN_TESSELLATION_LEVEL), 2.0);
				tessOuter = tessInner;
				tessAlpha = TERRAIN_TESS_OFFSET;
			}
		}
		else
#endif //__TERRAIN_TESSELATION__
		if (!(tr.viewParms.flags & VPF_CUBEMAP)
			&& !(tr.viewParms.flags & VPF_DEPTHSHADOW)
			&& !(tr.viewParms.flags & VPF_SHADOWPASS)
			&& !(tr.viewParms.flags & VPF_EMISSIVEMAP)
			&& !(tr.viewParms.flags & VPF_SKYCUBEDAY)
			&& !(tr.viewParms.flags & VPF_SKYCUBENIGHT))
		{
			/*
			if (tess.shader->tesselation
				&& tess.shader->tesselationLevel > 1.0
				&& tess.shader->tesselationAlpha != 0.0)
			{
				useTesselation = 1;
				tessInner = tess.shader->tesselationLevel;
				tessOuter = tessInner;
				tessAlpha = tess.shader->tesselationAlpha;
			}
			*/

			useTesselation = 0;
		}
	}

	qboolean usingDeforms = ShaderRequiresCPUDeforms(tess.shader);
	qboolean didNonDetail = qfalse;

	for ( int stage = 0; stage <= input->shader->maxStage && stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = input->xstages[stage];
		shaderProgram_t *sp = NULL, *sp2 = NULL, *sp3 = NULL;
		vec4_t texMatrix;
		vec4_t texOffTurb;
		int stateBits;
		colorGen_t forceRGBGen = CGEN_BAD;
		alphaGen_t forceAlphaGen = AGEN_IDENTITY;
		qboolean lightMapsDisabled = qfalse;
		qboolean isEmissiveBlackStage = qfalse;

		if ( !pStage )
		{// How does this happen???
			break;
		}

		if ( !pStage->active )
		{// Shouldn't this be here, just in case???
			continue;
		}

		if (backEnd.viewParms.flags & VPF_EMISSIVEMAP)
		{
			if (!pStage->glow) 
			{
				// Skip...
				//continue;
				isEmissiveBlackStage = qtrue;
			}
		}

		int index = pStage->glslShaderIndex;

		if ( pStage->isSurfaceSprite )
		{
#if !defined(__JKA_SURFACE_SPRITES__) && !defined(__XYC_SURFACE_SPRITES__)
			if (!r_surfaceSprites->integer)
#endif //__JKA_SURFACE_SPRITES__
			{
				continue;
			}
		}

#ifdef __DEFERRED_IMAGE_LOADING__
		if (pStage->bundle[TB_DIFFUSEMAP].image[0]
			&& pStage->bundle[TB_DIFFUSEMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_DIFFUSEMAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_DIFFUSEMAP].image[0]);
		}

		if (pStage->bundle[TB_NORMALMAP].image[0]
			&& pStage->bundle[TB_NORMALMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_NORMALMAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_NORMALMAP].image[0]);
		}

		if (pStage->bundle[TB_SPECULARMAP].image[0]
			&& pStage->bundle[TB_SPECULARMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_SPECULARMAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_SPECULARMAP].image[0]);
		}

		if (pStage->bundle[TB_OVERLAYMAP].image[0]
			&& pStage->bundle[TB_OVERLAYMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_OVERLAYMAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_OVERLAYMAP].image[0]);
		}

		if (pStage->bundle[TB_STEEPMAP].image[0]
			&& pStage->bundle[TB_STEEPMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_STEEPMAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_STEEPMAP].image[0]);
		}

		if (pStage->bundle[TB_STEEPMAP1].image[0]
			&& pStage->bundle[TB_STEEPMAP1].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_STEEPMAP1].image[0] = R_LoadDeferredImage(pStage->bundle[TB_STEEPMAP1].image[0]);
		}

		if (pStage->bundle[TB_STEEPMAP2].image[0]
			&& pStage->bundle[TB_STEEPMAP2].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_STEEPMAP2].image[0] = R_LoadDeferredImage(pStage->bundle[TB_STEEPMAP2].image[0]);
		}

		if (pStage->bundle[TB_STEEPMAP3].image[0]
			&& pStage->bundle[TB_STEEPMAP3].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_STEEPMAP3].image[0] = R_LoadDeferredImage(pStage->bundle[TB_STEEPMAP3].image[0]);
		}

		if (pStage->bundle[TB_WATER_EDGE_MAP].image[0]
			&& pStage->bundle[TB_WATER_EDGE_MAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_WATER_EDGE_MAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_WATER_EDGE_MAP].image[0]);
		}

		if (pStage->bundle[TB_SPLATCONTROLMAP].image[0]
			&& pStage->bundle[TB_SPLATCONTROLMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_SPLATCONTROLMAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_SPLATCONTROLMAP].image[0]);
		}

		if (pStage->bundle[TB_SPLATMAP1].image[0]
			&& pStage->bundle[TB_SPLATMAP1].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_SPLATMAP1].image[0] = R_LoadDeferredImage(pStage->bundle[TB_SPLATMAP1].image[0]);
		}

		if (pStage->bundle[TB_SPLATMAP2].image[0]
			&& pStage->bundle[TB_SPLATMAP2].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_SPLATMAP2].image[0] = R_LoadDeferredImage(pStage->bundle[TB_SPLATMAP2].image[0]);
		}

		if (pStage->bundle[TB_SPLATMAP3].image[0]
			&& pStage->bundle[TB_SPLATMAP3].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_SPLATMAP3].image[0] = R_LoadDeferredImage(pStage->bundle[TB_SPLATMAP3].image[0]);
		}

		if (pStage->bundle[TB_DETAILMAP].image[0]
			&& pStage->bundle[TB_DETAILMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			pStage->bundle[TB_DETAILMAP].image[0] = R_LoadDeferredImage(pStage->bundle[TB_DETAILMAP].image[0]);
		}
#endif //__DEFERRED_IMAGE_LOADING__

		stateBits = pStage->stateBits;

		if (backEnd.currentEntity)
		{
			assert(backEnd.currentEntity->e.renderfx >= 0);

			if (backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE1)
			{
				// we want to be able to rip a hole in the thing being disintegrated, and by doing the depth-testing it avoids some kinds of artefacts, but will probably introduce others?
				stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_192;
			}

			if (backEnd.currentEntity->e.renderfx & RF_RGB_TINT)
			{//want to use RGBGen from ent
				forceRGBGen = CGEN_ENTITY;
			}

			if (backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA)
			{
				stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				if (backEnd.currentEntity->e.renderfx & RF_ALPHA_DEPTH)
				{ //depth write, so faces through the model will be stomped over by nearer ones. this works because
				  //we draw RF_FORCE_ENT_ALPHA stuff after everything else, including standard alpha surfs.
					stateBits |= GLS_DEPTHMASK_TRUE;
				}
			}
		}
		else
		{// UQ: - FPS TESTING - This may cause issues, we will need to keep an eye on things...
			//if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL) && !(tr.viewParms.flags & VPF_SHADOWPASS))
			//	stateBits |= GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_EQUAL;

			//stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO;
			//stateBits = GLS_DEFAULT;
		}

		qboolean is2D = (backEnd.currentEntity == &backEnd.entity2D || (pStage->stateBits & GLS_DEPTHTEST_DISABLE)) ? qtrue : qfalse;
		float useTC = 0.0;
		float useDeform = 0.0;
		float useRGBA = 0.0;
		float useFog = 0.0;

		float useVertexAnim = 0.0;
		float useSkeletalAnim = 0.0;
		
		qboolean forceDetail = qfalse;

		if (backEnd.currentEntity)
		{
			assert(backEnd.currentEntity->e.renderfx >= 0);

			if ((backEnd.currentEntity->e.renderfx & RF_NODEPTH)
				|| (backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA)
				|| (backEnd.currentEntity->e.renderfx & RF_ALPHA_DEPTH)
				|| (backEnd.currentEntity->e.renderfx & RF_FORCEPOST))
				forceDetail = qtrue;
		}

#define __USE_DETAIL_CHECKING__			// Check and treat stages found to be random details (lightmap stages, 2d, etc) differently...
#define __USE_DETAIL_DEPTH_SKIP__		// Skip drawing detail crap at all in shadow and depth prepasses - they should never be needed...
#define __LIGHTMAP_IS_DETAIL__			// Lightmap stages are considered detail...

		if (pStage->isWater && r_glslWater->integer && WATER_ENABLED && MAP_WATER_LEVEL < 131000.0 && MAP_WATER_LEVEL > -131000.0)
		{
			if (IS_DEPTH_PASS == 1)
			{
				break;
			}
			else if (stage <= 0)
			{
				sp = &tr.waterPostForwardShader;
				pStage->glslShaderGroup = &tr.waterPostForwardShader;
				isWater = qtrue;
			}
			else
			{// Only do one stage on GLSL water...
				break;
			}

			GLSL_BindProgram(sp);
		}
		else if ((tess.shader->materialType) == MATERIAL_DISTORTEDGLASS)
		{
			if (IS_DEPTH_PASS == 1)
			{
				break;
			}
			else if (stage <= 0)
			{
				sp = &tr.waterPostForwardShader;
				pStage->glslShaderGroup = &tr.waterPostForwardShader;
				isGlass = qtrue;
			}
			else
			{// Only do one stage on GLSL water...
				break;
			}

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_DISTORTEDPUSH)
		{
			if (IS_DEPTH_PASS == 1)
			{
				break;
			}
			else if (stage <= 0)
			{
				sp = &tr.waterPostForwardShader;
				pStage->glslShaderGroup = &tr.waterPostForwardShader;
				isPush = qtrue;
				//ri->Printf(PRINT_ALL, "Doing force push shader.\n");
			}
			else
			{// Only do one stage on GLSL water...
				break;
			}

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_DISTORTEDPULL)
		{
			if (IS_DEPTH_PASS == 1)
			{
				break;
			}
			else if (stage <= 0)
			{
				sp = &tr.waterPostForwardShader;
				pStage->glslShaderGroup = &tr.waterPostForwardShader;
				isPull = qtrue;
			}
			else
			{// Only do one stage on GLSL water...
				break;
			}

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_CLOAK)
		{
			if (IS_DEPTH_PASS == 1)
			{
				break;
			}
			else if (stage <= 0)
			{
				sp = &tr.waterPostForwardShader;
				pStage->glslShaderGroup = &tr.waterPostForwardShader;
				isCloak = qtrue;
			}
			else
			{// Only do one stage on GLSL water...
				break;
			}

			GLSL_BindProgram(sp);
		}
		/*else if (r_proceduralSun->integer && tess.shader == tr.sunShader)
		{// Special case for procedural sun...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.sunPassShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);
		}*/
		else if (tess.shader->materialType == MATERIAL_MENU_BACKGROUND)
		{
			// Special case for procedural menu background...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.menuBackgroundShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);

			//ri->Printf(PRINT_WARNING, "using procedural menu background.\n");
		}
		else if (tess.shader->materialType == MATERIAL_FIRE)
		{// Special case for procedural fire...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.fireShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_SMOKE)
		{// Special case for procedural smoke...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.smokeShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_MAGIC_PARTICLES)
		{// Special case for procedural fire...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.magicParticlesShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_MAGIC_PARTICLES_TREE)
		{// Special case for procedural fire...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.magicParticlesTreeShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_FIREFLIES)
		{// Special case for procedural fire...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.magicParticlesFireFlyShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);
		}
		else if (tess.shader->materialType == MATERIAL_PORTAL)
		{// Special case for procedural area transition portals...
			if (IS_DEPTH_PASS) return;
			if (stage > 0) return;

			sp = &tr.portalShader;
			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			isGrass = qfalse;
			isGroundFoliage = qfalse;

			GLSL_BindProgram(sp);
		}
		else if (isGrassPatches && backEnd.renderPass == RENDERPASS_GRASS_PATCHES)
		{// Special extra pass stuff for grass...
			if (stage > 0) return;

			sp = &tr.grassPatchesShader;

			GLSL_BindProgram(sp);
		}
		else if (isGrass && backEnd.renderPass == RENDERPASS_GRASS)
		{// Special extra pass stuff for grass...
			if (stage > 0) return;

			if (GRASS_UNDERWATER_ONLY)
				sp = &tr.grassShader[0];
			else
				sp = &tr.grassShader[1];

			GLSL_BindProgram(sp);
		}
		else if (isGrass2 && backEnd.renderPass == RENDERPASS_GRASS2)
		{// Special extra pass stuff for grass...
			if (stage > 0) return;

			if (GRASS_UNDERWATER_ONLY)
				sp = &tr.grassShader[0];
			else
				sp = &tr.grassShader[1];

			GLSL_BindProgram(sp);
		}
		else if (isGrass3 && backEnd.renderPass == RENDERPASS_GRASS3)
		{// Special extra pass stuff for grass...
			if (stage > 0) return;

			if (GRASS_UNDERWATER_ONLY)
				sp = &tr.grassShader[0];
			else
				sp = &tr.grassShader[1];

			GLSL_BindProgram(sp);
		}
		else if (isGrass4 && backEnd.renderPass == RENDERPASS_GRASS4)
		{// Special extra pass stuff for grass...
			if (stage > 0) return;

			if (GRASS_UNDERWATER_ONLY)
				sp = &tr.grassShader[0];
			else
				sp = &tr.grassShader[1];

			GLSL_BindProgram(sp);
		}
		else if (isGroundFoliage && backEnd.renderPass == RENDERPASS_GROUNDFOLIAGE)
		{
			if (stage > 0) return;

			sp = &tr.foliageShader;
			GLSL_BindProgram(sp);
		}
		else if (isVines && backEnd.renderPass == RENDERPASS_VINES)
		{
			if (stage > 0) return;

			sp = &tr.vinesShader;
			GLSL_BindProgram(sp);
		}
		else if (isMist && backEnd.renderPass == RENDERPASS_MIST)
		{
			if (stage > 0) return;

			sp = &tr.mistShader;
			GLSL_BindProgram(sp);
		}
		else if ((IS_DEPTH_PASS || (tr.viewParms.flags & VPF_CUBEMAP))
			&& !((tr.viewParms.flags & VPF_EMISSIVEMAP) && (pStage->glow || pStage->glowMapped)))
		{
			lightMapsDisabled = qtrue;
			if (IS_DEPTH_PASS && !is2D && stage > 0) return;

#ifdef __USE_DETAIL_CHECKING__
#ifdef __USE_DETAIL_DEPTH_SKIP__
			if (pStage->bundle[0].isLightmap)
			{
				continue;
			}

			if (stage > 0 && didNonDetail && !pStage->glow)
			{
				continue;
			}

			if (pStage->isDetail && !pStage->glow)
			{// Don't waste the time...
				continue;
			}

			if (pStage->noScreenMap)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (pStage->bundle[0].isLightmap)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (forceDetail)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (pStage->type != ST_COLORMAP && pStage->type != ST_GLSL && !pStage->glow)
			{// Don't output these to position and normal map...
				index |= LIGHTDEF_IS_DETAIL;
			}

			if ((!pStage->bundle[TB_DIFFUSEMAP].image[0] || pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_COLORALPHA) && !pStage->glow)
			{// Don't output these to position and normal map...
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (backEnd.currentEntity == &backEnd.entity2D)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (pStage->alphaGen == AGEN_LIGHTING_SPECULAR)
			{// Skip specular crap completely in depth prepass etc...
				continue;
			}

			if (pStage->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED)
			{// Skip envmap crap completely in depth prepass etc...
				continue;
			}

			if (!(index & LIGHTDEF_IS_DETAIL) && !pStage->glow)
			{
				didNonDetail = qtrue;
			}
			else
			{
				continue;
			}
#endif //__USE_DETAIL_DEPTH_SKIP__
#endif /__USE_DETAIL_CHECKING__

			if (glState.vertexAnimation)
			{
				useVertexAnim = 1.0;
			}

			if (glState.skeletalAnimation)
			{
				useSkeletalAnim = 1.0;
			}

			if (pStage->bundle[0].tcGen != TCGEN_TEXTURE || pStage->bundle[0].numTexMods)
			{
				useTC = 1.0;
			}

			if (tess.shader->numDeforms || usingDeforms)
			{
				useDeform = 1.0;
			}

			if (tr.viewParms.flags & VPF_DEPTHSHADOW)
			{// No tess on shadow maps...
				sp = &tr.depthPassShader[0];
				backEnd.pc.c_depthPassDraws++;
			}
			else if (useTesselation == 1)
			{
				sp = &tr.depthPassShader[1];
				backEnd.pc.c_lightallDraws++;
			}
			else if (useTesselation == 2)
			{
				sp = &tr.depthPassShader[2];
				backEnd.pc.c_lightallDraws++;
			}
			else if (useTesselation == 3)
			{
				sp = &tr.depthPassShader[3];
				backEnd.pc.c_lightallDraws++;
			}
			else
			{
				sp = &tr.depthPassShader[0];
				backEnd.pc.c_depthPassDraws++;
			}

			GLSL_BindProgram(sp);
		}
		else
		{
			if (!(tr.world || tr.numLightmaps <= 0) && (index & LIGHTDEF_USE_LIGHTMAP))
			{// Bsp has no lightmap data, disable lightmaps in any shaders that would try to use one...
				if (pStage->bundle[0].isLightmap)
				{
					backEnd.pc.c_lightMapsSkipped++;
					continue;
				}

				index &= ~LIGHTDEF_USE_LIGHTMAP;
				lightMapsDisabled = qtrue;
			}

			if (r_lightmap->integer < 0 && (pStage->bundle[0].isLightmap || pStage->bundle[TB_LIGHTMAP].isLightmap))
			{
				if (pStage->bundle[0].isLightmap)
				{
					backEnd.pc.c_lightMapsSkipped++;
					continue;
				}

				if (index & LIGHTDEF_USE_LIGHTMAP)
				{
					index &= ~LIGHTDEF_USE_LIGHTMAP;
				}

				lightMapsDisabled = qtrue;
			}
			
			if (glState.vertexAnimation)
			{
				useVertexAnim = 1.0;
			}

			if (glState.skeletalAnimation)
			{
				useSkeletalAnim = 1.0;
			}

			if (r_splatMapping->integer
				&& (tess.shader->materialType == MATERIAL_SKYSCRAPER)
				&& pStage->bundle[TB_STEEPMAP].image[0]
				&& !pStage->bundle[TB_WATER_EDGE_MAP].image[0]
				&& !pStage->bundle[TB_SPLATMAP1].image[0]
				&& !pStage->bundle[TB_SPLATMAP2].image[0]
				&& !pStage->bundle[TB_SPLATMAP3].image[0])
			{// Procedural city sky scraper texture, use triplanar, not regions...
				index |= LIGHTDEF_USE_TRIPLANAR;
			}
#ifdef __USE_REGIONS__
			else if (r_splatMapping->integer
				&& (tess.shader->materialType == MATERIAL_ROCK || tess.shader->materialType == MATERIAL_STONE)
				&& (pStage->bundle[TB_STEEPMAP].image[0]
					|| pStage->bundle[TB_STEEPMAP1].image[0]
					|| pStage->bundle[TB_STEEPMAP2].image[0]
					|| pStage->bundle[TB_STEEPMAP3].image[0]
					|| pStage->bundle[TB_WATER_EDGE_MAP].image[0]
					|| pStage->bundle[TB_SPLATMAP1].image[0]
					|| pStage->bundle[TB_SPLATMAP2].image[0]
					|| pStage->bundle[TB_SPLATMAP3].image[0]))
			{
				index |= LIGHTDEF_USE_REGIONS;
			}
#endif //__USE_REGIONS__
			else if (r_splatMapping->integer
				&& (pStage->bundle[TB_STEEPMAP].image[0]
					|| pStage->bundle[TB_STEEPMAP1].image[0]
					|| pStage->bundle[TB_STEEPMAP2].image[0]
					|| pStage->bundle[TB_STEEPMAP3].image[0]
					|| pStage->bundle[TB_WATER_EDGE_MAP].image[0]
					|| pStage->bundle[TB_SPLATMAP1].image[0]
					|| pStage->bundle[TB_SPLATMAP2].image[0]
					|| pStage->bundle[TB_SPLATMAP3].image[0]))
			{
				index |= LIGHTDEF_USE_TRIPLANAR;
			}
			else if (pStage->bundle[TB_ROOFMAP].image[0])
			{
				index |= LIGHTDEF_USE_TRIPLANAR;
			}

			switch (pStage->rgbGen)
			{
			case CGEN_LIGHTING_DIFFUSE:
				useRGBA = 1.0;
				break;
			case CGEN_LIGHTING_WARZONE:
				useRGBA = 1.0;
				break;
			default:
				break;
			}

			switch (pStage->alphaGen)
			{
			case AGEN_LIGHTING_SPECULAR:
			case AGEN_PORTAL:
				useRGBA = 1.0;
				break;
			default:
				break;
			}

			if (pStage->bundle[0].tcGen != TCGEN_TEXTURE || pStage->bundle[0].numTexMods)
			{
				useTC = 1.0;
			}

			if (tess.shader->numDeforms || usingDeforms)
			{
				useDeform = 1.0;
			}

			if (input->fogNum)
			{
				useFog = 1.0;
			}
			
#ifdef __USE_DETAIL_CHECKING__
			if (stage > 0 && didNonDetail)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (forceDetail)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (pStage->isDetail)
			{// Don't output these to position and normal map...
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (pStage->noScreenMap)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (pStage->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

#ifdef __LIGHTMAP_IS_DETAIL__
			if (pStage->bundle[0].isLightmap)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}
#endif //__LIGHTMAP_IS_DETAIL__

			if (pStage->type != ST_COLORMAP && pStage->type != ST_GLSL)
			{// Don't output these to position and normal map...
				index |= LIGHTDEF_IS_DETAIL;
			}

			//GLS_DEPTHTEST_DISABLE
			if (backEnd.currentEntity == &backEnd.entity2D)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}

			if (!pStage->bundle[TB_DIFFUSEMAP].image[0]
				|| pStage->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_COLORALPHA)
			{// Don't output these to position and normal map...
				index |= LIGHTDEF_IS_DETAIL;
			}

#ifdef __LIGHTMAP_IS_DETAIL__
			if (!(pStage->type == ST_COLORMAP || pStage->type == ST_GLSL)
				&& pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP
				&& pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			{
				index |= LIGHTDEF_IS_DETAIL;
			}
#endif //__LIGHTMAP_IS_DETAIL__

			if (!(index & LIGHTDEF_IS_DETAIL))
			{
				didNonDetail = qtrue;
			}
#endif //__USE_DETAIL_CHECKING__
			
			if (r_splatMapping->integer 
				//&& !r_lowVram->integer
				&& ((index & LIGHTDEF_USE_REGIONS) || (index & LIGHTDEF_USE_TRIPLANAR)))
			{
				if (useTesselation == 1)
				{
					sp = &tr.lightAllSplatShader[1];
				}
				else if (useTesselation == 2)
				{
					sp = &tr.lightAllSplatShader[2];
				}
				else if (useTesselation == 3)
				{
					sp = &tr.lightAllSplatShader[3];
				}
				else
				{
					sp = &tr.lightAllSplatShader[0];
				}
			}
			else
			{
				if (tess.shader->materialType == MATERIAL_LAVA)
					sp = &tr.lightAllShader[2];
				if (useTesselation)
					sp = &tr.lightAllShader[1];
				else
					sp = &tr.lightAllShader[0];
			}

			backEnd.pc.c_lightallDraws++;

			GLSL_BindProgram(sp);
		}

		float nightScale = RB_NightScale();

		if (backEnd.renderPass == RENDERPASS_NONE || is2D)
		{
			{// Set up basic shader settings... This way we can avoid the bind bloat of dumb shader #ifdefs...
				vec4_t vec;

				if (!IS_DEPTH_PASS)
				{
					if (r_debugMapAmbientR->value + r_debugMapAmbientG->value + r_debugMapAmbientB->value > 0.0)
					{
						VectorSet4(vec, r_debugMapAmbientR->value, r_debugMapAmbientG->value, r_debugMapAmbientB->value, 0.0);
					}
					else
					{
						vec[0] = mix(MAP_AMBIENT_COLOR[0], MAP_AMBIENT_COLOR_NIGHT[0], nightScale);
						vec[1] = mix(MAP_AMBIENT_COLOR[1], MAP_AMBIENT_COLOR_NIGHT[1], nightScale);
						vec[2] = mix(MAP_AMBIENT_COLOR[2], MAP_AMBIENT_COLOR_NIGHT[2], nightScale);
						vec[4] = 0.0;
						//VectorSet4(vec, MAP_AMBIENT_COLOR[0], MAP_AMBIENT_COLOR[1], MAP_AMBIENT_COLOR[2], 0.0);
					}

					GLSL_SetUniformVec4(sp, UNIFORM_MAP_AMBIENT, vec);
				}

				VectorSet4(vec,
					useTC,
					useDeform,
					useRGBA,
					(pStage->bundle[TB_DIFFUSEMAP].image[0] && (pStage->bundle[TB_DIFFUSEMAP].image[0]->flags & IMGFLAG_CLAMPTOEDGE)) ? 1.0 : 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS0, vec);

				float blendMethod = 0.0;
				float blendMethod2 = useSkeletalAnim;

				if (sp == &tr.lightAllSplatShader[0] || sp == &tr.lightAllSplatShader[1] || sp == &tr.lightAllSplatShader[2] || sp == &tr.lightAllSplatShader[3])
				{
					blendMethod2 = SPLATMAP_SCALE_WATEREDGE1;
					blendMethod = SPLATMAP_SCALE_WATEREDGE2;
				}
				else
				{
#if 1
					/*if (stateBits & GLS_DSTBLEND_ONE_MINUS_SRC_COLOR)
					{
						blendMethod = 1.0;

						stateBits &= ~GLS_SRCBLEND_BITS;
						stateBits &= ~GLS_DSTBLEND_BITS;
						stateBits |= GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE;

					}
					else*/ if (stateBits & GLS_DSTBLEND_SRC_COLOR)
					{
						blendMethod = 2.0;

						stateBits &= ~GLS_SRCBLEND_BITS;
						stateBits &= ~GLS_DSTBLEND_BITS;
						stateBits |= GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE;
					}
					/*else if (stateBits & GLS_DSTBLEND_ONE)
					{
						blendMethod = 3.0;

						stateBits &= ~GLS_SRCBLEND_BITS;
						stateBits &= ~GLS_DSTBLEND_BITS;
						stateBits |= GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE;
					}*/
					else if (/*(stateBits & GLS_SRCBLEND_ONE) &&*/ (stateBits & GLS_DSTBLEND_ONE))
					{
						blendMethod = 4.0;
					}
#endif
				}

				VectorSet4(vec,
					useVertexAnim,
					blendMethod2 /*useSkeletalAnim*/,
					blendMethod,
					is2D ? 1.0 : 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);

				float useGlow = 0.0;
				float useLightMap = 0.0;
				float isRenderCube = 0.0;

				if (!IS_DEPTH_PASS)
				{
					if (pStage->glow || (index & LIGHTDEF_USE_GLOW_BUFFER)) useGlow = 1.0;
					if (pStage->glowMapped) useGlow = 2.0;
					if ((index & LIGHTDEF_USE_LIGHTMAP) && !lightMapsDisabled) useLightMap = 1.0;
					if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo) isRenderCube = 1.0;
				}

				VectorSet4(vec,
					useLightMap,
					useGlow,
					isRenderCube,
					(index & LIGHTDEF_USE_TRIPLANAR) ? (input->shader->warzoneVextexSplat) ? 2.0 : 1.0 : 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS2, vec);

				VectorSet4(vec,
					(index & LIGHTDEF_USE_REGIONS) ? 1.0 : 0.0,
					(index & LIGHTDEF_IS_DETAIL) ? 1.0 : 0.0,
					(backEnd.viewParms.flags & VPF_EMISSIVEMAP) ? 1.0 : 0.0,
					pStage->glowBlend);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS3, vec);

#ifdef __USE_REGIONS__
				if (index & LIGHTDEF_USE_REGIONS)
				{
					VectorSet4(vec,
						MAP_LIGHTMAP_MULTIPLIER * 0.0075,
						MAP_LIGHTMAP_ENHANCEMENT,
						ROCK_SPLATMAP_SCALE_STEEP,
						DYNAMIC_WEATHER_PUDDLE_STRENGTH);
					GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS4, vec);
				}
				else
#endif //__USE_REGIONS__
					if (index & LIGHTDEF_USE_TRIPLANAR)
					{
						VectorSet4(vec,
							MAP_LIGHTMAP_MULTIPLIER * 0.0075,
							MAP_LIGHTMAP_ENHANCEMENT,
							STANDARD_SPLATMAP_SCALE_STEEP,
							DYNAMIC_WEATHER_PUDDLE_STRENGTH);
						GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS4, vec);
					}
					else
					{
						VectorSet4(vec,
							MAP_LIGHTMAP_MULTIPLIER * 0.0075,
							MAP_LIGHTMAP_ENHANCEMENT,
							(tess.shader->hasAlphaTestBits || tess.shader->materialType == MATERIAL_GREENLEAVES) ? 1.0 : 0.0, // TODO: MATERIAL_GREENLEAVES because something isnt right with models...
							DYNAMIC_WEATHER_PUDDLE_STRENGTH);
						GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS4, vec);
					}

				
				//
				// lightAllSplat shaders need splatmap scales and not christmas light info.
				//
#ifdef __USE_REGIONS__
				if (index & LIGHTDEF_USE_REGIONS)
				{
					VectorSet4(vec,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_RG) ? 1.0 : 0.0,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_RB) ? 1.0 : 0.0,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_GB) ? 1.0 : 0.0,
						ROCK_SPLATMAP_SCALE);
					GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
				}
				else
#endif //__USE_REGIONS__
				if (index & LIGHTDEF_USE_TRIPLANAR)
				{
					VectorSet4(vec,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_RG) ? 1.0 : 0.0,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_RB) ? 1.0 : 0.0,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_GB) ? 1.0 : 0.0,
						STANDARD_SPLATMAP_SCALE);
					GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
				}
				else
				{
					VectorSet4(vec,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_RG) ? 1.0 : 0.0,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_RB) ? 1.0 : 0.0,
						(backEnd.currentEntity == &tr.worldEntity && MAP_COLOR_SWITCH_GB) ? 1.0 : 0.0,
						(ENABLE_CHRISTMAS_EFFECT && tess.shader->materialType == MATERIAL_GREENLEAVES) ? 1.0 : 0.0);
					GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
				}

				if (backEnd.currentEntity && backEnd.currentEntity->e.ignoreCull)
					GLSL_SetUniformFloat(sp, UNIFORM_ZFAR, 999999.9);
				else
					GLSL_SetUniformFloat(sp, UNIFORM_ZFAR, (ENABLE_OCCLUSION_CULLING && r_occlusion->integer) ? tr.occlusionZfar : backEnd.viewParms.zFar);

#ifdef __CHEAP_VERTS__
				GLSL_SetUniformInt(sp, UNIFORM_WORLD, isWorld ? 1 : 0);
#endif //__CHEAP_VERTS__
			}

			if (pStage->glow)
			{
				if (r_glowMultiplierR->value != 1.0 || r_glowMultiplierG->value != 1.0 || r_glowMultiplierB->value != 1.0 || r_glowMultiplierA->value != 1.0)
				{
					vec4_t glowMultiplierRGBA;
					VectorSet4(glowMultiplierRGBA, r_glowMultiplierR->value, r_glowMultiplierG->value, r_glowMultiplierB->value, r_glowMultiplierA->value);
					GLSL_SetUniformVec4(sp, UNIFORM_GLOWMULTIPLIER, glowMultiplierRGBA);
				}
				else
				{
					GLSL_SetUniformVec4(sp, UNIFORM_GLOWMULTIPLIER, pStage->glowMultiplierRGBA);
				}
			}

			// UQ1: Used by both generic and lightall...
			RB_SetStageImageDimensions(sp, pStage);
			RB_SetMaterialBasedProperties(sp, pStage, stage, IS_DEPTH_PASS);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);
			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

			GLSL_SetUniformVec3(sp, UNIFORM_LOCALVIEWORIGIN, backEnd.ori.viewOrigin);
			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg/*backEnd.viewParms.ori.origin*/);

			if (glState.vertexAnimation)
			{
				GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);
			}

			if (glState.skeletalAnimation)
			{
				GLSL_SetUniformMatrix16(sp, UNIFORM_BONE_MATRICES, (const float *)glState.boneMatrices, glState.numBones);
			}

			GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);
			if (deformGen != DGEN_NONE)
			{
				GLSL_SetUniformFloat7(sp, UNIFORM_DEFORMPARAMS, deformParams);
				GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			}

			GLSL_SetUniformInt(sp, UNIFORM_TCGEN0, pStage->bundle[0].tcGen);
			if (pStage->bundle[0].tcGen == TCGEN_VECTOR)
			{
				vec3_t vec;

				VectorCopy(pStage->bundle[0].tcGenVectors[0], vec);
				GLSL_SetUniformVec3(sp, UNIFORM_TCGEN0VECTOR0, vec);
				VectorCopy(pStage->bundle[0].tcGenVectors[1], vec);
				GLSL_SetUniformVec3(sp, UNIFORM_TCGEN0VECTOR1, vec);
			}


			vec2_t scale;
			ComputeTexMods(pStage, TB_DIFFUSEMAP, texMatrix, texOffTurb, scale);
			GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, texMatrix);
			GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, texOffTurb);
			GLSL_SetUniformVec2(sp, UNIFORM_TEXTURESCALE, scale);
		}
		
		if (pStage->isFoliage)
		{// Override shader with a faster one...
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GE_128;
			pStage->stateBits = stateBits;
		}
		else if (r_proceduralSun->integer && tess.shader == tr.sunShader)
		{
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_ATEST_GT_0;
			pStage->stateBits = stateBits;
		}
		else if (tess.shader->materialType == MATERIAL_MENU_BACKGROUND)
		{// Special case for procedural main menu background...
			//stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			//pStage->stateBits = stateBits;
		}
		else if (tess.shader->materialType == MATERIAL_FIRE)
		{// Special case for procedural fire...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			pStage->stateBits = stateBits;
		}
		else if (tess.shader->materialType == MATERIAL_SMOKE)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			pStage->stateBits = stateBits;
		}
		else if (tess.shader->materialType == MATERIAL_MAGIC_PARTICLES)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			pStage->stateBits = stateBits;
		}
		else if (tess.shader->materialType == MATERIAL_MAGIC_PARTICLES_TREE)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			pStage->stateBits = stateBits;
		}
		else if (tess.shader->materialType == MATERIAL_FIREFLIES)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			pStage->stateBits = stateBits;
		}
		else if (tess.shader->materialType == MATERIAL_PORTAL)
		{// Special case for procedural portals...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			pStage->stateBits = stateBits;
		}

		//
		// Texture bindings...
		//

		if (isGrassPatches && backEnd.renderPass == RENDERPASS_GRASS_PATCHES)
		{// Special extra pass stuff for grass...
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

#ifdef __HUMANOIDS_BEND_GRASS__ // Bend grass for all close player/NPCs...
			GLSL_SetUniformInt(sp, UNIFORM_HUMANOIDORIGINSNUM, backEnd.humanoidOriginsNum);
			GLSL_SetUniformVec3xX(sp, UNIFORM_HUMANOIDORIGINS, backEnd.humanoidOrigins, backEnd.humanoidOriginsNum);
#endif //__HUMANOIDS_BEND_GRASS__

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.grassPatchesAliasImage, 0);
			}
			else
			{
				GL_BindToTMU(tr.grassPatchesAliasImage, TB_DIFFUSEMAP);
			}

			{
				vec2_t dimensions;
				dimensions[0] = tr.grassPatchesAliasImage->width;
				dimensions[1] = tr.grassPatchesAliasImage->height;
				GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			vec4_t l10;
			VectorSet4(l10, GRASS_PATCHES_DISTANCE, TERRAIN_TESS_OFFSET, GRASS_PATCHES_DENSITY, GRASS_PATCHES_TYPE_UNIFORMALITY);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, GRASS_PATCHES_WIDTH_REPEATS, GRASS_PATCHES_MAX_SLOPE, GRASS_PATCHES_TYPE_UNIFORMALITY_SCALER, GRASS_PATCHES_RARE_PATCHES_ONLY ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.blackImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					IS_DEPTH_PASS ? 1.0 : 0.0,
					0.0,
					0.0,
					LEAF_ALPHA_MULTIPLIER);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					MAP_COLOR_SWITCH_RG ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_RB ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_GB ? 1.0 : 0.0,
					0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isGrass && backEnd.renderPass == RENDERPASS_GRASS)
		{// Special extra pass stuff for grass...
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

#ifdef __HUMANOIDS_BEND_GRASS__ // Bend grass for all close player/NPCs...
			GLSL_SetUniformInt(sp, UNIFORM_HUMANOIDORIGINSNUM, backEnd.humanoidOriginsNum);
			GLSL_SetUniformVec3xX(sp, UNIFORM_HUMANOIDORIGINS, backEnd.humanoidOrigins, backEnd.humanoidOriginsNum);
#endif //__HUMANOIDS_BEND_GRASS__

			if (GRASS_UNDERWATER_ONLY)
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[0], 0);
				}
				else
				{
					GL_BindToTMU(tr.seaGrassAliasImage[0], UNIFORM_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.seaGrassAliasImage[0]->width;
					dimensions[1] = tr.seaGrassAliasImage[0]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}
			else
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.grassAliasImage[0], 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[0], 0);
				}
				else
				{
					GL_BindToTMU(tr.grassAliasImage[0], TB_DIFFUSEMAP);
					GL_BindToTMU(tr.seaGrassAliasImage[0], TB_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.grassAliasImage[0]->width;
					dimensions[1] = tr.grassAliasImage[0]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;
				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			vec4_t l10;
			VectorSet4(l10, GRASS_DISTANCE, TERRAIN_TESS_OFFSET, GRASS_DENSITY, GRASS_TYPE_UNIFORMALITY);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, GRASS_WIDTH_REPEATS, GRASS_MAX_SLOPE, GRASS_TYPE_UNIFORMALITY_SCALER, GRASS_RARE_PATCHES_ONLY ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					IS_DEPTH_PASS ? 1.0 : 0.0,
					0.0,
					0.0,
					LEAF_ALPHA_MULTIPLIER);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					MAP_COLOR_SWITCH_RG ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_RB ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_GB ? 1.0 : 0.0,
					0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isGrass2 && backEnd.renderPass == RENDERPASS_GRASS2)
		{// Special extra pass stuff for grass...
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

#ifdef __HUMANOIDS_BEND_GRASS__ // Bend grass for all close player/NPCs...
			GLSL_SetUniformInt(sp, UNIFORM_HUMANOIDORIGINSNUM, backEnd.humanoidOriginsNum);
			GLSL_SetUniformVec3xX(sp, UNIFORM_HUMANOIDORIGINS, backEnd.humanoidOrigins, backEnd.humanoidOriginsNum);
#endif //__HUMANOIDS_BEND_GRASS__

			if (GRASS_UNDERWATER_ONLY)
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[1], 0);
				}
				else
				{
					GL_BindToTMU(tr.seaGrassAliasImage[1], TB_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.seaGrassAliasImage[1]->width;
					dimensions[1] = tr.seaGrassAliasImage[1]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}
			else
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.grassAliasImage[1], 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[1], 0);
				}
				else
				{
					GL_BindToTMU(tr.grassAliasImage[1], UNIFORM_DIFFUSEMAP);
					GL_BindToTMU(tr.seaGrassAliasImage[1], UNIFORM_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.grassAliasImage[1]->width;
					dimensions[1] = tr.grassAliasImage[1]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			vec4_t l10;
			VectorSet4(l10, GRASS2_DISTANCE, TERRAIN_TESS_OFFSET, GRASS2_DENSITY, GRASS2_TYPE_UNIFORMALITY);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, GRASS2_WIDTH_REPEATS, GRASS2_MAX_SLOPE, GRASS2_TYPE_UNIFORMALITY_SCALER, GRASS2_RARE_PATCHES_ONLY ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					IS_DEPTH_PASS ? 1.0 : 0.0,
					0.0,
					0.0,
					LEAF_ALPHA_MULTIPLIER);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					MAP_COLOR_SWITCH_RG ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_RB ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_GB ? 1.0 : 0.0,
					0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isGrass3 && backEnd.renderPass == RENDERPASS_GRASS3)
		{// Special extra pass stuff for grass...
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

#ifdef __HUMANOIDS_BEND_GRASS__ // Bend grass for all close player/NPCs...
			GLSL_SetUniformInt(sp, UNIFORM_HUMANOIDORIGINSNUM, backEnd.humanoidOriginsNum);
			GLSL_SetUniformVec3xX(sp, UNIFORM_HUMANOIDORIGINS, backEnd.humanoidOrigins, backEnd.humanoidOriginsNum);
#endif //__HUMANOIDS_BEND_GRASS__

			if (GRASS_UNDERWATER_ONLY)
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[2], 0);
				}
				else
				{
					GL_BindToTMU(tr.seaGrassAliasImage[2], TB_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.seaGrassAliasImage[2]->width;
					dimensions[1] = tr.seaGrassAliasImage[2]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}
			else
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.grassAliasImage[2], 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[2], 0);
				}
				else
				{
					GL_BindToTMU(tr.grassAliasImage[2], TB_DIFFUSEMAP);
					GL_BindToTMU(tr.seaGrassAliasImage[2], TB_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.grassAliasImage[2]->width;
					dimensions[1] = tr.grassAliasImage[2]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;
				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			vec4_t l10;
			VectorSet4(l10, GRASS3_DISTANCE, TERRAIN_TESS_OFFSET, GRASS3_DENSITY, GRASS3_TYPE_UNIFORMALITY);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, GRASS3_WIDTH_REPEATS, GRASS3_MAX_SLOPE, GRASS3_TYPE_UNIFORMALITY_SCALER, GRASS3_RARE_PATCHES_ONLY ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					IS_DEPTH_PASS ? 1.0 : 0.0,
					0.0,
					0.0,
					LEAF_ALPHA_MULTIPLIER);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					MAP_COLOR_SWITCH_RG ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_RB ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_GB ? 1.0 : 0.0,
					0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isGrass4 && backEnd.renderPass == RENDERPASS_GRASS4)
		{// Special extra pass stuff for grass...
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

#ifdef __HUMANOIDS_BEND_GRASS__ // Bend grass for all close player/NPCs...
			GLSL_SetUniformInt(sp, UNIFORM_HUMANOIDORIGINSNUM, backEnd.humanoidOriginsNum);
			GLSL_SetUniformVec3xX(sp, UNIFORM_HUMANOIDORIGINS, backEnd.humanoidOrigins, backEnd.humanoidOriginsNum);
#endif //__HUMANOIDS_BEND_GRASS__

			if (GRASS_UNDERWATER_ONLY)
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[3], 0);
				}
				else
				{
					GL_BindToTMU(tr.seaGrassAliasImage[3], TB_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.seaGrassAliasImage[3]->width;
					dimensions[1] = tr.seaGrassAliasImage[3]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}
			else
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.grassAliasImage[3], 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &tr.seaGrassAliasImage[3], 0);
				}
				else
				{
					GL_BindToTMU(tr.grassAliasImage[3], TB_DIFFUSEMAP);
					GL_BindToTMU(tr.seaGrassAliasImage[3], TB_WATER_EDGE_MAP);
				}

				{
					vec2_t dimensions;
					dimensions[0] = tr.grassAliasImage[3]->width;
					dimensions[1] = tr.grassAliasImage[3]->height;
					GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
				}
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;
				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			vec4_t l10;
			VectorSet4(l10, GRASS4_DISTANCE, TERRAIN_TESS_OFFSET, GRASS4_DENSITY, GRASS4_TYPE_UNIFORMALITY);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, GRASS4_WIDTH_REPEATS, GRASS4_MAX_SLOPE, GRASS4_TYPE_UNIFORMALITY_SCALER, GRASS4_RARE_PATCHES_ONLY ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					IS_DEPTH_PASS ? 1.0 : 0.0,
					0.0,
					0.0,
					LEAF_ALPHA_MULTIPLIER);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					MAP_COLOR_SWITCH_RG ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_RB ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_GB ? 1.0 : 0.0,
					0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isGroundFoliage && backEnd.renderPass == RENDERPASS_GROUNDFOLIAGE)
		{
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

//#ifdef __HUMANOIDS_BEND_GRASS__ // Bend grass for all close player/NPCs...
//			GLSL_SetUniformInt(sp, UNIFORM_HUMANOIDORIGINSNUM, backEnd.humanoidOriginsNum);
//			GLSL_SetUniformVec3xX(sp, UNIFORM_HUMANOIDORIGINS, backEnd.humanoidOrigins, backEnd.humanoidOriginsNum);
//#endif //__HUMANOIDS_BEND_GRASS__

			{
				vec2_t dimensions;
				dimensions[0] = tr.foliageAliasImage->width;
				dimensions[1] = tr.foliageAliasImage->height;
				GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
			}

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.foliageAliasImage, 0);
			}
			else
			{
				GL_BindToTMU(tr.foliageAliasImage, TB_DIFFUSEMAP);
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}

			vec4_t l10;
			VectorSet4(l10, FOLIAGE_DISTANCE, TERRAIN_TESS_OFFSET, FOLIAGE_DENSITY, FOLIAGE_TYPE_UNIFORMALITY);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, FOLIAGE_LOD_START_RANGE, FOLIAGE_MAX_SLOPE, FOLIAGE_TYPE_UNIFORMALITY_SCALER, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					IS_DEPTH_PASS ? 1.0 : 0.0,
					0.0,
					0.0,
					LEAF_ALPHA_MULTIPLIER);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					MAP_COLOR_SWITCH_RG ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_RB ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_GB ? 1.0 : 0.0,
					0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isVines && backEnd.renderPass == RENDERPASS_VINES)
		{
			stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;
			//stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ZERO/*GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA*/ | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

			{
				vec2_t dimensions;
				dimensions[0] = tr.vinesAliasImage->width;
				dimensions[1] = tr.vinesAliasImage->height*VINES_HEIGHT;
				GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
			}
			

//#ifdef __HUMANOIDS_BEND_GRASS__ // Bend grass for all close player/NPCs...
//			GLSL_SetUniformInt(sp, UNIFORM_HUMANOIDORIGINSNUM, backEnd.humanoidOriginsNum);
//			GLSL_SetUniformVec3xX(sp, UNIFORM_HUMANOIDORIGINS, backEnd.humanoidOrigins, backEnd.humanoidOriginsNum);
//#endif //__HUMANOIDS_BEND_GRASS__

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.vinesAliasImage, 0);
			}
			else
			{
				GL_BindToTMU(tr.vinesAliasImage, TB_DIFFUSEMAP);
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			vec4_t l10;
			VectorSet4(l10, VINES_DISTANCE, TERRAIN_TESS_OFFSET, VINES_DENSITY, VINES_TYPE_UNIFORMALITY);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, VINES_WIDTH_REPEATS, VINES_MIN_SLOPE, VINES_TYPE_UNIFORMALITY_SCALER, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			{
				vec4_t vec;
				VectorSet4(vec,
					IS_DEPTH_PASS ? 1.0 : 0.0,
					0.0,
					0.0,
					LEAF_ALPHA_MULTIPLIER);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vec);
			}

			{
				vec4_t vec;
				VectorSet4(vec,
					MAP_COLOR_SWITCH_RG ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_RB ? 1.0 : 0.0,
					MAP_COLOR_SWITCH_GB ? 1.0 : 0.0,
					ENABLE_CHRISTMAS_EFFECT ? 1.0 : 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS5, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isMist && backEnd.renderPass == RENDERPASS_MIST)
		{
			//stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;// | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformVec3(sp, UNIFORM_PLAYERORIGIN, backEnd.localPlayerOrigin);

			{
				vec2_t dimensions;
				dimensions[0] = MIST_TEXTURE->width;
				dimensions[1] = MIST_TEXTURE->height*VINES_HEIGHT;
				GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, dimensions);
			}

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &MIST_TEXTURE, 0);
			}
			else
			{
				GL_BindToTMU(MIST_TEXTURE, TB_DIFFUSEMAP);
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the mist down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}
			}

			vec4_t l10;
			VectorSet4(l10, MIST_DISTANCE, TERRAIN_TESS_OFFSET, MIST_DENSITY, MIST_MAX_SLOPE);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, l10);

			vec4_t l11;
			VectorSet4(l11, MIST_ALPHA, MIST_SPEED_X, MIST_SPEED_Y, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, l11);

			GL_Cull(CT_TWO_SIDED);
		}
		else if (IS_DEPTH_PASS || sp == &tr.depthPassShader[0] || sp == &tr.depthPassShader[1] || sp == &tr.depthPassShader[2] || sp == &tr.depthPassShader[3])
		{
			vec4_t baseColor = { 1.0 };
			vec4_t vertColor = { 1.0 };

			if (pStage->rgbGen || pStage->alphaGen)
			{
				ComputeShaderColors(pStage, baseColor, vertColor, stateBits, &forceRGBGen, &forceAlphaGen);

				if ((backEnd.refdef.colorScale != 1.0f) && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
				{
					// use VectorScale to only scale first three values, not alpha
					VectorScale(baseColor, backEnd.refdef.colorScale, baseColor);
					VectorScale(vertColor, backEnd.refdef.colorScale, vertColor);
				}

				if (backEnd.currentEntity != NULL &&
					(backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA))
				{
					vertColor[3] = backEnd.currentEntity->e.shaderRGBA[3] / 255.0f;
				}
			}

			/*if (isEmissiveBlack || isEmissiveBlackStage)
			{// Force black when drawing emissive cubes, and this is not a glow...
				VectorSet4(baseColor, 0.0, 0.0, 0.0, 1.0);
				//VectorSet4(vertColor, 0.0, 0.0, 0.0, 1.0);

				pStage->rgbGen = CGEN_CONST;
			}*/

			GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, baseColor);
			GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, vertColor);

			if (pStage->alphaGen == AGEN_PORTAL)
			{
				GLSL_SetUniformFloat(sp, UNIFORM_PORTALRANGE, tess.shader->portalRange);
			}

			GLSL_SetUniformInt(sp, UNIFORM_COLORGEN, forceRGBGen);
			GLSL_SetUniformInt(sp, UNIFORM_ALPHAGEN, forceAlphaGen);

			if (r_sunlightMode->integer && (r_sunlightSpecular->integer || (backEnd.viewParms.flags & VPF_USESUNLIGHT)))
			{
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
				//GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.refdef.sunCol);
				//GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.refdef.sunDir);
				GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.viewParms.emissiveLightOrigin);
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.viewParms.emissiveLightColor);
			}
		}
		else
		{
			vec4_t baseColor = { 1.0 };
			vec4_t vertColor = { 1.0 };

			GLSL_SetUniformVec3(sp, UNIFORM_COLORMOD, pStage->colorMod);

			if (pStage->rgbGen || pStage->alphaGen)
			{
				ComputeShaderColors(pStage, baseColor, vertColor, stateBits, &forceRGBGen, &forceAlphaGen);

				if ((backEnd.refdef.colorScale != 1.0f) && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
				{
					// use VectorScale to only scale first three values, not alpha
					VectorScale(baseColor, backEnd.refdef.colorScale, baseColor);
					VectorScale(vertColor, backEnd.refdef.colorScale, vertColor);
				}

				if (backEnd.currentEntity != NULL &&
					(backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA))
				{
					vertColor[3] = backEnd.currentEntity->e.shaderRGBA[3] / 255.0f;
				}
			}

			/*if (isEmissiveBlack || isEmissiveBlackStage)
			{// Force black when drawing emissive cubes, and this is not a glow...
				VectorSet4(baseColor, 0.0, 0.0, 0.0, 1.0);
				//VectorSet4(vertColor, 0.0, 0.0, 0.0, 1.0);

				pStage->rgbGen = CGEN_CONST;
			}*/

			GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, baseColor);
			GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, vertColor);

			if (pStage->rgbGen == CGEN_LIGHTING_DIFFUSE || pStage->rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
			{
				vec4_t vec;

				VectorScale(backEnd.currentEntity->ambientLight, 1.0f / 255.0f, vec);
				GLSL_SetUniformVec3(sp, UNIFORM_AMBIENTLIGHT, vec);

				VectorScale(backEnd.currentEntity->directedLight, 1.0f / 255.0f, vec);
				GLSL_SetUniformVec3(sp, UNIFORM_DIRECTEDLIGHT, vec);

				VectorCopy(backEnd.currentEntity->lightDir, vec);
				vec[3] = 0.0f;
				GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vec);
				GLSL_SetUniformVec3(sp, UNIFORM_MODELLIGHTDIR, backEnd.currentEntity->modelLightDir);
				GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, 0.0f);
			}
			else if (pStage->rgbGen == CGEN_LIGHTING_WARZONE)
			{
				/*GLSL_SetUniformVec3(sp, UNIFORM_AMBIENTLIGHT, backEnd.refdef.sunAmbCol);
				GLSL_SetUniformVec3(sp, UNIFORM_DIRECTEDLIGHT, backEnd.refdef.sunCol);

				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.refdef.sunCol);
				GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.refdef.sunDir);*/
				
				//GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.viewParms.emissiveLightDirection);
				//GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.viewParms.emissiveLightColor);
			}

			if (pStage->alphaGen == AGEN_PORTAL)
			{
				GLSL_SetUniformFloat(sp, UNIFORM_PORTALRANGE, tess.shader->portalRange);
			}

			GLSL_SetUniformInt(sp, UNIFORM_COLORGEN, forceRGBGen);
			GLSL_SetUniformInt(sp, UNIFORM_ALPHAGEN, forceAlphaGen);

			if (r_splatMapping->integer
				&& (sp == &tr.lightAllSplatShader[0] || sp == &tr.lightAllSplatShader[1] || sp == &tr.lightAllSplatShader[2] || sp == &tr.lightAllSplatShader[3]))
			{
#ifdef __USE_DETAIL_MAPS__
				if (sp->isBindless)
				{
					if (pStage->bundle[TB_DETAILMAP].image[0])
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_DETAILMAP, &pStage->bundle[TB_DETAILMAP].image[0], 0);
					}
					else
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_DETAILMAP, &pStage->bundle[TB_DETAILMAP].image[0], 0);
					}
				}
				else
				{
					if (pStage->bundle[TB_DETAILMAP].image[0])
					{
						GL_BindToTMU(pStage->bundle[TB_DETAILMAP].image[0], TB_DETAILMAP);
					}
					else
					{
						GL_BindToTMU(tr.defaultDetail, TB_DETAILMAP);
					}
				}
#endif //__USE_DETAIL_MAPS__

				/*if (pStage->glowMapped && pStage->bundle[TB_GLOWMAP].image[0])
				{
					if (sp->isBindless)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_GLOWMAP, &pStage->bundle[TB_GLOWMAP].image[0], 0);
					}
					else
					{
						GL_BindToTMU(pStage->bundle[TB_GLOWMAP].image[0], TB_GLOWMAP);
					}
				}*/

				if ((index & LIGHTDEF_USE_REGIONS) || (index & LIGHTDEF_USE_TRIPLANAR))
				{
					if (pStage->bundle[TB_SPLATCONTROLMAP].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_SPLATCONTROLMAP, &pStage->bundle[TB_SPLATCONTROLMAP].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_SPLATCONTROLMAP].image[0], TB_SPLATCONTROLMAP);
						}
					}
					else
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_SPLATCONTROLMAP, &tr.defaultSplatControlImage, 0);
						}
						else
						{
							GL_BindToTMU(tr.defaultSplatControlImage, TB_SPLATCONTROLMAP); // really need to make a blured (possibly also considering heightmap) version of this...
						}
					}

					if (pStage->bundle[TB_ROOFMAP].image[0])
					{// DiffuseMap becomes the steep texture...
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_DIFFUSEMAP].image[0], TB_STEEPMAP1);
						}
					}
					else if (pStage->bundle[TB_STEEPMAP].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP, &pStage->bundle[TB_STEEPMAP].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_STEEPMAP].image[0], TB_STEEPMAP);
						}
					}

					if (pStage->bundle[TB_STEEPMAP1].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP1, &pStage->bundle[TB_STEEPMAP1].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_STEEPMAP1].image[0], TB_STEEPMAP1);
						}
					}

					if (pStage->bundle[TB_STEEPMAP2].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP2, &pStage->bundle[TB_STEEPMAP2].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_STEEPMAP2].image[0], TB_STEEPMAP2);
						}
					}

					if (pStage->bundle[TB_STEEPMAP3].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP3, &pStage->bundle[TB_STEEPMAP3].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_STEEPMAP3].image[0], TB_STEEPMAP3);
						}
					}

					if (pStage->bundle[TB_WATER_EDGE_MAP].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_WATER_EDGE_MAP, &pStage->bundle[TB_WATER_EDGE_MAP].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_WATER_EDGE_MAP].image[0], TB_WATER_EDGE_MAP);
						}
					}

					if (pStage->bundle[TB_SPLATMAP1].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_SPLATMAP1, &pStage->bundle[TB_SPLATMAP1].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_SPLATMAP1].image[0], TB_SPLATMAP1);
						}
					}

					if (pStage->bundle[TB_SPLATMAP2].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_SPLATMAP2, &pStage->bundle[TB_SPLATMAP2].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_SPLATMAP2].image[0], TB_SPLATMAP2);
						}
					}

					if (pStage->bundle[TB_SPLATMAP3].image[0])
					{
						if (sp->isBindless)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_SPLATMAP3, &pStage->bundle[TB_SPLATMAP3].image[0], 0);
						}
						else
						{
							GL_BindToTMU(pStage->bundle[TB_SPLATMAP3].image[0], TB_SPLATMAP3);
						}
					}
				}
			}

			//
			// Single draw overlaymap system...
			//
			if (pStage->bundle[TB_OVERLAYMAP].image[0])
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_OVERLAYMAP, &pStage->bundle[TB_OVERLAYMAP].image[0], 0);
				}
				else
				{
					R_BindAnimatedImageToTMU(&pStage->bundle[TB_OVERLAYMAP], TB_OVERLAYMAP);
				}
			}

			//
			// Single draw envmap system...
			//
			if (pStage->envmapUseSkyImage && tr.skyImageShader)
			{// This stage wants sky up image for it's diffuse... TODO: sky cubemap...
#ifdef __DAY_NIGHT__
				if (!DAY_NIGHT_CYCLE_ENABLED || nightScale < 1.0)
				{
					if (sp->isBindless)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_ENVMAP, &tr.skyImageShader->sky.outerbox[4], 0);
					}
					else
					{
						GL_BindToTMU(tr.skyImageShader->sky.outerbox[4], TB_ENVMAP); // Sky up...
					}
				}
				else
				{
					if (sp->isBindless)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_ENVMAP, &tr.skyImageShader->sky.outerboxnight[4], 0);
					}
					else
					{
						GL_BindToTMU(tr.skyImageShader->sky.outerboxnight[4], TB_ENVMAP); // Night sky up...
					}
				}
#else
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ENVMAP, &tr.skyImageShader->sky.outerbox[4], 0);
				}
				else
				{
					GL_BindToTMU(tr.skyImageShader->sky.outerbox[4], TB_ENVMAP); // Sky up...
				}
#endif
			}
			else if (pStage->bundle[TB_ENVMAP].image[0])
			{
				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ENVMAP, &pStage->bundle[TB_ENVMAP].image[0], 0);
				}
				else
				{
					R_BindAnimatedImageToTMU(&pStage->bundle[TB_ENVMAP], TB_ENVMAP);
				}
			}

#ifdef __SCREEN_SPACE_GLASS_REFLECTIONS__
			if (r_txaa->integer)
			{// Reflections only work if we have a previous screen to use...
				GLSL_SetUniformInt(sp, UNIFORM_DELUXEMAP, TB_DELUXEMAP);
				GL_BindToTMU(tr.txaaPreviousImage, TB_DELUXEMAP);
			}
			else
			{
				GLSL_SetUniformInt(sp, UNIFORM_DELUXEMAP, TB_DELUXEMAP);
				GL_BindToTMU(tr.blackImage, TB_DELUXEMAP);
			}
#endif //__SCREEN_SPACE_GLASS_REFLECTIONS__

			if (r_sunlightMode->integer && (r_sunlightSpecular->integer || (backEnd.viewParms.flags & VPF_USESUNLIGHT)))
			{
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
				//GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR,   backEnd.refdef.sunCol);
				//GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN,  backEnd.refdef.sunDir);
				GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.viewParms.emissiveLightOrigin);
				GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.viewParms.emissiveLightColor);
			}

			if (sp->isBindless)
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADMAP, &tr.roadImage, 0);
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.blackImage, 0);
				}
			}
			else
			{
				if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
				{
					GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
					GL_BindToTMU(tr.roadImage, TB_ROADMAP);
				}
				else
				{
					GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
				}
			}

			vec4_t loc;
			VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

			VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

			VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
		}

		//
		// do multitexture
		//
		if (backEnd.renderPass == RENDERPASS_NONE || is2D)
		{
			if (sp->isBindless)
			{
				if (sp == &tr.depthPassShader[0] || sp == &tr.depthPassShader[1] || sp == &tr.depthPassShader[2] || sp == &tr.depthPassShader[3])
				{
					if (!(pStage->stateBits & GLS_ATEST_BITS))
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.whiteImage, 0);
					/*else if (tess.shader->isCursor && backEnd.ui_MouseCursor > 0)
						GL_BindImageIDToTMU(backEnd.ui_MouseCursor, TB_COLORMAP);*/ // override with nuklear's selected icon for drag/drop stuff... ****** FIXME - BINDLESS ID ******
					else if (pStage->bundle[TB_COLORMAP].image[0] != 0)
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_COLORMAP].image[0], 0);
				}
				else if ((tr.viewParms.flags & VPF_SHADOWPASS))
				{
					if (pStage->bundle[TB_DIFFUSEMAP].image[0])
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);
					else if (!(pStage->stateBits & GLS_ATEST_BITS))
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.whiteImage, 0);
				}
				else if (IS_DEPTH_PASS || backEnd.depthFill)
				{
					if (!(pStage->stateBits & GLS_ATEST_BITS))
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.whiteImage, 0);
					else if (pStage->bundle[TB_COLORMAP].image[0] != 0)
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_COLORMAP].image[0], 0);
				}
				else if (tess.shader->materialType == MATERIAL_LAVA)
				{// Don't need any textures... Procedural...
				 
				}
				else if (pStage->useSkyImage && tr.skyImageShader)
				{// This stage wants sky up image for it's diffuse... TODO: sky cubemap...
#ifdef __DAY_NIGHT__
					if (!DAY_NIGHT_CYCLE_ENABLED || nightScale < 1.0)
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.skyImageShader->sky.outerbox[4], 0);
					else
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.skyImageShader->sky.outerboxnight[4], 0);
#else
					GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &tr.skyImageShader->sky.outerbox[4], 0);
#endif
				}
				else if (sp == &tr.lightAllShader[0] || sp == &tr.lightAllSplatShader[0]
					|| sp == &tr.lightAllShader[1] || sp == &tr.lightAllSplatShader[1]
					|| sp == &tr.lightAllShader[2] || sp == &tr.lightAllSplatShader[2]
					|| sp == &tr.lightAllSplatShader[3])
				{
					int i;

					if ((r_lightmap->integer == 1 || r_lightmap->integer == 2) && pStage->bundle[TB_LIGHTMAP].image[0])
					{
						for (i = 0; i <= UNIFORM_SHADOWMAP5; i++)
						{
							if (i == UNIFORM_LIGHTMAP)
								GLSL_SetBindlessTexture(sp, UNIFORM_LIGHTMAP, &pStage->bundle[TB_LIGHTMAP].image[0], 0);
							else
								GLSL_SetBindlessTexture(sp, i, &tr.whiteImage, 0);
						}
					}
					else if (r_lightmap->integer == 3 && pStage->bundle[TB_DELUXEMAP].image[0])
					{
						for (i = 0; i <= UNIFORM_SHADOWMAP5; i++)
						{
							if (i == UNIFORM_LIGHTMAP)
								GLSL_SetBindlessTexture(sp, UNIFORM_LIGHTMAP, &pStage->bundle[TB_DELUXEMAP].image[0], 0);
							else
								GLSL_SetBindlessTexture(sp, i, &tr.whiteImage, 0);
						}
					}
					else
					{
						/*if (tess.shader->isCursor && backEnd.ui_MouseCursor > 0)
							GL_BindImageIDToTMU(backEnd.ui_MouseCursor, TB_DIFFUSEMAP); // override with nuklear's selected icon for drag/drop stuff... ****** FIXME - BINDLESS ID ******
						else*/ if (pStage->bundle[TB_ROOFMAP].image[0]) // Roofmap becomes the triplanar flat surface texture...
							GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_ROOFMAP].image[0], 0);
						else if (pStage->bundle[TB_DIFFUSEMAP].image[0])
							GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);

						if (!lightMapsDisabled)
						{
							if (pStage->bundle[TB_LIGHTMAP].image[0])
								GLSL_SetBindlessTexture(sp, UNIFORM_LIGHTMAP, &pStage->bundle[TB_LIGHTMAP].image[0], 0);
							else
								GLSL_SetBindlessTexture(sp, UNIFORM_LIGHTMAP, &tr.whiteImage, 0);
						}

						// bind textures that are sampled and used in the glsl shader, and
						// bind whiteImage to textures that are sampled but zeroed in the glsl shader
						//
						// alternatives:
						//  - use the last bound texture
						//     -> costs more to sample a higher res texture then throw out the result
						//  - disable texture sampling in glsl shader with #ifdefs, as before
						//     -> increases the number of shaders that must be compiled
						//
						if (r_normalMappingReal->integer)
						{
							if (sp == &tr.lightAllShader[0] || sp == &tr.lightAllShader[1] || sp == &tr.lightAllShader[2])
							{
								if (pStage->bundle[TB_NORMALMAP].image[0])
								{
									GLSL_SetBindlessTexture(sp, UNIFORM_NORMALMAP, &pStage->bundle[TB_NORMALMAP].image[0], 0);
								}
								else if (r_normalMapping->integer >= 2)
								{
									GLSL_SetBindlessTexture(sp, UNIFORM_NORMALMAP, &tr.whiteImage, 0);
								}
							}
						}

						if (pStage->glowMapped && pStage->bundle[TB_GLOWMAP].image[0])
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_GLOWMAP, &pStage->bundle[TB_GLOWMAP].image[0], 0);
						}
					}
				}
				else if (pStage->bundle[TB_LIGHTMAP].image[0] != 0)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);

					if (!lightMapsDisabled)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_LIGHTMAP, &pStage->bundle[TB_LIGHTMAP].image[0], 0);
					}
				}
				else
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);
				}
			}
			else
			{
				if (sp == &tr.depthPassShader[0] || sp == &tr.depthPassShader[1] || sp == &tr.depthPassShader[2] || sp == &tr.depthPassShader[3])
				{
					if (!(pStage->stateBits & GLS_ATEST_BITS))
						GL_BindToTMU(tr.whiteImage, 0);
					else if (tess.shader->isCursor && backEnd.ui_MouseCursor > 0)
						GL_BindImageIDToTMU(backEnd.ui_MouseCursor, TB_COLORMAP); // override with nuklear's selected icon for drag/drop stuff...
					else if (pStage->bundle[TB_COLORMAP].image[0] != 0)
						R_BindAnimatedImageToTMU(&pStage->bundle[TB_COLORMAP], TB_COLORMAP);
				}
				else if ((tr.viewParms.flags & VPF_SHADOWPASS))
				{
					if (pStage->bundle[TB_DIFFUSEMAP].image[0])
						R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);
					else if (!(pStage->stateBits & GLS_ATEST_BITS))
						GL_BindToTMU(tr.whiteImage, 0);
				}
				else if (IS_DEPTH_PASS || backEnd.depthFill)
				{
					if (!(pStage->stateBits & GLS_ATEST_BITS))
						GL_BindToTMU(tr.whiteImage, 0);
					else if (pStage->bundle[TB_COLORMAP].image[0] != 0)
						R_BindAnimatedImageToTMU(&pStage->bundle[TB_COLORMAP], TB_COLORMAP);
				}
				else if (tess.shader->materialType == MATERIAL_LAVA)
				{// Don't need any textures... Procedural...
					//GL_BindToTMU(tr.random2KImage[0], TB_DIFFUSEMAP);
				}
				else if (pStage->useSkyImage && tr.skyImageShader)
				{// This stage wants sky up image for it's diffuse... TODO: sky cubemap...
#ifdef __DAY_NIGHT__
					if (!DAY_NIGHT_CYCLE_ENABLED || nightScale < 1.0)
						GL_BindToTMU(tr.skyImageShader->sky.outerbox[4], TB_COLORMAP); // Sky up...
					else
						GL_BindToTMU(tr.skyImageShader->sky.outerboxnight[4], TB_COLORMAP); // Night sky up...
#else
					GL_BindToTMU(tr.skyImageShader->sky.outerbox[4], TB_COLORMAP); // Sky up...
#endif
				}
				else if (sp == &tr.lightAllShader[0] || sp == &tr.lightAllSplatShader[0]
					|| sp == &tr.lightAllShader[1] || sp == &tr.lightAllSplatShader[1]
					|| sp == &tr.lightAllShader[2] || sp == &tr.lightAllSplatShader[2]
					|| sp == &tr.lightAllSplatShader[3])
				{
					int i;

					if ((r_lightmap->integer == 1 || r_lightmap->integer == 2) && pStage->bundle[TB_LIGHTMAP].image[0])
					{
						for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
						{
							if (i == TB_LIGHTMAP)
								R_BindAnimatedImageToTMU(&pStage->bundle[TB_LIGHTMAP], i);
							else
								GL_BindToTMU(tr.whiteImage, i);
						}
					}
					else if (r_lightmap->integer == 3 && pStage->bundle[TB_DELUXEMAP].image[0])
					{
						for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
						{
							if (i == TB_LIGHTMAP)
								R_BindAnimatedImageToTMU(&pStage->bundle[TB_DELUXEMAP], i);
							else
								GL_BindToTMU(tr.whiteImage, i);
						}
					}
					else
					{
						if (tess.shader->isCursor && backEnd.ui_MouseCursor > 0)
							GL_BindImageIDToTMU(backEnd.ui_MouseCursor, TB_DIFFUSEMAP); // override with nuklear's selected icon for drag/drop stuff...
						else if (pStage->bundle[TB_ROOFMAP].image[0]) // Roofmap becomes the triplanar flat surface texture...
							R_BindAnimatedImageToTMU(&pStage->bundle[TB_ROOFMAP], TB_DIFFUSEMAP);
						else if (pStage->bundle[TB_DIFFUSEMAP].image[0])
							R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);

						if (!lightMapsDisabled)
						{
							if (pStage->bundle[TB_LIGHTMAP].image[0])
								R_BindAnimatedImageToTMU(&pStage->bundle[TB_LIGHTMAP], TB_LIGHTMAP);
							else
								GL_BindToTMU(tr.whiteImage, TB_LIGHTMAP);
						}

						// bind textures that are sampled and used in the glsl shader, and
						// bind whiteImage to textures that are sampled but zeroed in the glsl shader
						//
						// alternatives:
						//  - use the last bound texture
						//     -> costs more to sample a higher res texture then throw out the result
						//  - disable texture sampling in glsl shader with #ifdefs, as before
						//     -> increases the number of shaders that must be compiled
						//
						if (r_normalMappingReal->integer)
						{
							if (sp == &tr.lightAllShader[0] || sp == &tr.lightAllShader[1] || sp == &tr.lightAllShader[2])
							{
								if (pStage->bundle[TB_NORMALMAP].image[0])
								{
									R_BindAnimatedImageToTMU(&pStage->bundle[TB_NORMALMAP], TB_NORMALMAP);
								}
								else if (r_normalMapping->integer >= 2)
								{
									GL_BindToTMU(tr.whiteImage, TB_NORMALMAP);
								}
							}
						}

						if (pStage->glowMapped && pStage->bundle[TB_GLOWMAP].image[0])
						{
							GL_BindToTMU(pStage->bundle[TB_GLOWMAP].image[0], TB_GLOWMAP);
						}
					}
				}
				else if (pStage->bundle[TB_LIGHTMAP].image[0] != 0)
				{
					R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], 0);

					if (!lightMapsDisabled)
					{
						R_BindAnimatedImageToTMU(&pStage->bundle[TB_LIGHTMAP], 1);
					}
				}
				else
				{
					//
					// set state
					//
					R_BindAnimatedImageToTMU(&pStage->bundle[0], 0);
				}
			}
		}

		if (!tess.shader->hasAlpha && !tess.shader->hasGlow && !pStage->rgbGen && !pStage->alphaGen && tess.shader->cullType != CT_BACK_SIDED)
		{
			GL_Cull(CT_FRONT_SIDED);
		}

		if (r_proceduralSun->integer && tess.shader == tr.sunShader)
		{// Procedural sun...
			//stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GE_128;
			//stateBits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_ATEST_GT_0;
			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*2.0);

			vec4_t loc;
			VectorSet4(loc, SUN_COLOR_MAIN[0], SUN_COLOR_MAIN[1], SUN_COLOR_MAIN[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7, loc);

			VectorSet4(loc, SUN_COLOR_SECONDARY[0], SUN_COLOR_SECONDARY[1], SUN_COLOR_SECONDARY[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, loc);

			VectorSet4(loc, SUN_COLOR_TERTIARY[0], SUN_COLOR_TERTIARY[1], SUN_COLOR_TERTIARY[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL9, loc);

			GL_Cull(CT_TWO_SIDED);
		}
		else if (tess.shader->materialType == MATERIAL_MENU_BACKGROUND)
		{// Special case for procedural main menu background...
			//stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			vec2_t ss;
			ss[0] = glConfig.vidWidth * r_superSampleMultiplier->value;
			ss[1] = glConfig.vidHeight * r_superSampleMultiplier->value;
			GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, ss);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*10.0);
			GL_Cull(CT_TWO_SIDED);
		}
		else if (tess.shader->materialType == MATERIAL_FIRE)
		{// Special case for procedural fire...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;

			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*10.0);
			GL_Cull(CT_TWO_SIDED);
		}
		else if (tess.shader->materialType == MATERIAL_SMOKE)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*10.0);
			GL_Cull(CT_TWO_SIDED);
		}
		else if (tess.shader->materialType == MATERIAL_MAGIC_PARTICLES)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*2.0);

			{
				vec4_t l6;
				VectorSet4(l6, pStage->particleColor[0], pStage->particleColor[1], pStage->particleColor[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL6, l6);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (tess.shader->materialType == MATERIAL_MAGIC_PARTICLES_TREE)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*2.0);

			{
				vec4_t l6;
				VectorSet4(l6, pStage->particleColor[0], pStage->particleColor[1], pStage->particleColor[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL6, l6);
			}

			{
				vec4_t l7;
				VectorSet4(l7, pStage->fireFlyColor[0], pStage->fireFlyColor[1], pStage->fireFlyColor[2], pStage->fireFlyCount);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7, l7);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (tess.shader->materialType == MATERIAL_FIREFLIES)
		{// Special case for procedural smoke...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*2.0);

			{
				vec4_t l7;
				VectorSet4(l7, pStage->fireFlyColor[0], pStage->fireFlyColor[1], pStage->fireFlyColor[2], pStage->fireFlyCount);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7, l7);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (tess.shader->materialType == MATERIAL_PORTAL)
		{// Special case for procedural portals...
			stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_ATEST_GT_0;
			RB_SetMaterialBasedProperties(sp, pStage, stage, qfalse);

			GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime*2.0);

			{
				vec4_t vec;
				VectorSet4(vec, pStage->portalColor1[0], pStage->portalColor1[1], pStage->portalColor1[2], 1.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL6, vec);

				VectorSet4(vec, pStage->portalColor2[0], pStage->portalColor2[1], pStage->portalColor2[2], 1.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7, vec);

				VectorSet4(vec, pStage->portalImageColor[0], pStage->portalImageColor[1], pStage->portalImageColor[2], pStage->portalImageAlpha);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, vec);
			}

			GL_Cull(CT_TWO_SIDED);
		}

		if (useTesselation)
		{
			vec4_t l10;
			VectorSet4(l10, tessAlpha, tessInner, tessOuter, TERRAIN_TESSELLATION_MIN_SIZE);
			GLSL_SetUniformVec4(sp, UNIFORM_TESSELATION_INFO, l10);

			if (useTesselation == 3)
			{
				GLSL_SetUniformVec4(sp, UNIFORM_TESSELATION_3D_INFO, TERRAIN_TESSELLATION_3D_OFFSET);
			}

			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}


				if (sp->isBindless)
				{
					if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
					}
					else
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.blackImage, 0);
					}
				}
				else
				{
					if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
					{
						GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
					}
					else
					{
						GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
					}
				}
			}

			{
				vec4_t l12;
				VectorSet4(l12, TERRAIN_TESS_OFFSET, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
			}

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}
		}
		else if (useVertexAnim || useSkeletalAnim)
		{
			float TERRAIN_TESS_OFFSET = 0.0;

			// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_tessellation->integer
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0)
			{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
				TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

				if (useTesselation == 3)
				{
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
				}

				if (sp->isBindless)
				{
					if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
					}
					else
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.blackImage, 0);
					}
				}
				else
				{
					if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
					{
						GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
					}
					else
					{
						GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
					}
				}
			}

			{
				vec4_t l12;
				VectorSet4(l12, TERRAIN_TESS_OFFSET, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
			}

			{
				vec4_t loc;
				VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

				VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

				VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
			}
		}
		else
		{
			vec4_t l12;
			VectorSet4(l12, 0.0, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
		}

		vec4_t l9;
		VectorSet4(l9, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value, r_testshaderValue4->value);
		GLSL_SetUniformVec4(sp, UNIFORM_LOCAL9, l9);

		if (isWater && r_glslWater->integer && WATER_ENABLED && MAP_WATER_LEVEL < 131000.0 && MAP_WATER_LEVEL > -131000.0)
		{// Attach dummy water output textures...
			if (glState.currentFBO == tr.renderFbo)
			{// Only attach textures when doing a render pass...
				stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO /*| GLS_ATEST_GT_0*/;
				tess.shader->cullType = CT_TWO_SIDED; // Always...
				FBO_Bind(tr.renderWaterFbo);

				vec4_t passInfo;
				VectorSet4(passInfo, 0.0, WATER_WAVE_HEIGHT, 0.0 /*material*/, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, passInfo);
			}
			else
			{
				break;
			}
		}
		else if (isGlass || isPush || isPull || isCloak)
		{// Attach dummy transparancy output textures...
			backEnd.pc.c_transparancyDraws++;

			if (glState.currentFBO == tr.renderFbo)
			{// Only attach textures when doing a render pass...
				if (isPush)
				{
					if (sp->isBindless)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);
					}
					else
					{
						GLSL_SetUniformInt(sp, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
						R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);
					}
					stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;
				}
				else if (isPull)
				{
					if (sp->isBindless)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);
					}
					else
					{
						GLSL_SetUniformInt(sp, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
						R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);
					}
					stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;
				}
				else if (isCloak)
				{
					if (sp->isBindless)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_DIFFUSEMAP, &pStage->bundle[TB_DIFFUSEMAP].image[0], 0);
					}
					else
					{
						GLSL_SetUniformInt(sp, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
						R_BindAnimatedImageToTMU(&pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);
					}
					stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;
				}
				else // distorted glass
				{
					stateBits = GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GT_0;
				}
				tess.shader->cullType = CT_TWO_SIDED; // Always...
				FBO_Bind(tr.renderTransparancyFbo);

				float material = 0.0;

				if (isGlass)
				{
					material = 2.0;
				}
				else if (isPush)
				{
					material = 3.0;
				}
				else if (isPull)
				{
					material = 4.0;
				}
				else if (isCloak)
				{
					material = 5.0;
				}

				vec4_t passInfo;
				VectorSet4(passInfo, 0.0, WATER_WAVE_HEIGHT, material, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, passInfo);
			}
			else
			{
				break;
			}
		}
		else
		{
			if (glState.currentFBO == tr.renderGlowFbo || glState.currentFBO == tr.renderDetailFbo || glState.currentFBO == tr.renderWaterFbo || glState.currentFBO == tr.renderTransparancyFbo)
			{// Only attach textures when doing a render pass...
				FBO_Bind(tr.renderFbo);
			}
		}

		if (isGrassPatches && backEnd.renderPass == RENDERPASS_GRASS_PATCHES)
		{
			{
				vec4_t l8;
				VectorSet4(l8, GRASS_PATCHES_SURFACE_MINIMUM_SIZE, GRASS_PATCHES_DISTANCE_FROM_ROADS, GRASS_PATCHES_HEIGHT, GRASS_PATCHES_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			{
				vec4_t l12;
				VectorSet4(l12, GRASS_PATCHES_SIZE_MULTIPLIER_COMMON, GRASS_PATCHES_SIZE_MULTIPLIER_RARE, GRASS_PATCHES_CLUMP_LAYERS, GRASS_PATCHES_LOD_START_RANGE);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
			}

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP, &tr.whiteImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP1, (GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS2_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP2, (GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS3_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP3, (GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS4_CONTROL_TEXTURE : &tr.blackImage, 0);
			}
			else
			{
				GL_BindToTMU(tr.whiteImage, TB_STEEPMAP);
				GL_BindToTMU((GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? GRASS2_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP1);
				GL_BindToTMU((GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? GRASS3_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP2);
				GL_BindToTMU((GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? GRASS4_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP3);
			}

			vec4_t l13;
			VectorSet4(l13,
				0.0,
				(GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL13, l13);
		}
		else if (isGrass && backEnd.renderPass == RENDERPASS_GRASS)
		{
			{
				vec4_t l8;
				VectorSet4(l8, GRASS_SURFACE_MINIMUM_SIZE, GRASS_DISTANCE_FROM_ROADS, GRASS_HEIGHT, GRASS_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			{
				vec4_t l12;
				VectorSet4(l12, GRASS_SIZE_MULTIPLIER_COMMON, GRASS_SIZE_MULTIPLIER_RARE, GRASS_SIZE_MULTIPLIER_UNDERWATER, GRASS_LOD_START_RANGE);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
			}

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP, (GRASS_ENABLED && GRASS_CONTROL_TEXTURE) ? &GRASS_CONTROL_TEXTURE : &tr.whiteImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP1, (GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS2_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP2, (GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS3_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP3, (GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS4_CONTROL_TEXTURE : &tr.blackImage, 0);
			}
			else
			{
				GL_BindToTMU((GRASS_ENABLED && GRASS_CONTROL_TEXTURE) ? GRASS_CONTROL_TEXTURE : tr.whiteImage, TB_STEEPMAP);
				GL_BindToTMU((GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? GRASS2_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP1);
				GL_BindToTMU((GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? GRASS3_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP2);
				GL_BindToTMU((GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? GRASS4_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP3);
			}

			vec4_t l13;
			VectorSet4(l13,
				(GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL13, l13);
		}
		else if (isGrass2 && backEnd.renderPass == RENDERPASS_GRASS2)
		{
			{
				vec4_t l8;
				VectorSet4(l8, GRASS2_SURFACE_MINIMUM_SIZE, GRASS2_DISTANCE_FROM_ROADS, GRASS2_HEIGHT, GRASS2_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			{
				vec4_t l12;
				VectorSet4(l12, GRASS2_SIZE_MULTIPLIER_COMMON, GRASS2_SIZE_MULTIPLIER_RARE, GRASS2_SIZE_MULTIPLIER_UNDERWATER, GRASS2_LOD_START_RANGE);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);

				if (sp->isBindless)
				{
					GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP, (GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE) ? &GRASS2_CONTROL_TEXTURE : &tr.whiteImage, 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP1, (GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS_CONTROL_TEXTURE : &tr.blackImage, 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP2, (GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS3_CONTROL_TEXTURE : &tr.blackImage, 0);
					GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP3, (GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS4_CONTROL_TEXTURE : &tr.blackImage, 0);
				}
				else
				{
					GL_BindToTMU((GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE) ? GRASS2_CONTROL_TEXTURE : tr.whiteImage, TB_STEEPMAP);
					GL_BindToTMU((GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? GRASS_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP1);
					GL_BindToTMU((GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? GRASS3_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP2);
					GL_BindToTMU((GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? GRASS4_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP3);
				}

				vec4_t l13;
				VectorSet4(l13,
					(GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
					(GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
					(GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
					(GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL13, l13);
			}
		}
		else if (isGrass3 && backEnd.renderPass == RENDERPASS_GRASS3)
		{
			{
				vec4_t l8;
				VectorSet4(l8, GRASS3_SURFACE_MINIMUM_SIZE, GRASS3_DISTANCE_FROM_ROADS, GRASS3_HEIGHT, GRASS3_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			{
				vec4_t l12;
				VectorSet4(l12, GRASS3_SIZE_MULTIPLIER_COMMON, GRASS3_SIZE_MULTIPLIER_RARE, GRASS3_SIZE_MULTIPLIER_UNDERWATER, GRASS3_LOD_START_RANGE);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
			}

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP, (GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE) ? &GRASS3_CONTROL_TEXTURE : &tr.whiteImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP1, (GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP2, (GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS2_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP3, (GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS4_CONTROL_TEXTURE : &tr.blackImage, 0);
			}
			else
			{
				GL_BindToTMU((GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE) ? GRASS3_CONTROL_TEXTURE : tr.whiteImage, TB_STEEPMAP);
				GL_BindToTMU((GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? GRASS_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP1);
				GL_BindToTMU((GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? GRASS2_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP2);
				GL_BindToTMU((GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? GRASS4_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP3);
			}

			vec4_t l13;
			VectorSet4(l13,
				(GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL13, l13);
		}
		else if (isGrass4 && backEnd.renderPass == RENDERPASS_GRASS4)
		{
			{
				vec4_t l8;
				VectorSet4(l8, GRASS4_SURFACE_MINIMUM_SIZE, GRASS4_DISTANCE_FROM_ROADS, GRASS4_HEIGHT, GRASS4_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			{
				vec4_t l12;
				VectorSet4(l12, GRASS4_SIZE_MULTIPLIER_COMMON, GRASS4_SIZE_MULTIPLIER_RARE, GRASS4_SIZE_MULTIPLIER_UNDERWATER, GRASS4_LOD_START_RANGE);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
			}

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP, (GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE) ? &GRASS4_CONTROL_TEXTURE : &tr.whiteImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP1, (GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP2, (GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS2_CONTROL_TEXTURE : &tr.blackImage, 0);
				GLSL_SetBindlessTexture(sp, UNIFORM_STEEPMAP3, (GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? &GRASS3_CONTROL_TEXTURE : &tr.blackImage, 0);
			}
			else
			{
				GL_BindToTMU((GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE) ? GRASS4_CONTROL_TEXTURE : tr.whiteImage, TB_STEEPMAP);
				GL_BindToTMU((GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? GRASS_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP1);
				GL_BindToTMU((GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? GRASS2_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP2);
				GL_BindToTMU((GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? GRASS3_CONTROL_TEXTURE : tr.blackImage, TB_STEEPMAP3);
			}

			vec4_t l13;
			VectorSet4(l13,
				(GRASS4_ENABLED && GRASS4_CONTROL_TEXTURE && GRASS4_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS_ENABLED && GRASS_CONTROL_TEXTURE && GRASS_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS2_ENABLED && GRASS2_CONTROL_TEXTURE && GRASS2_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0,
				(GRASS3_ENABLED && GRASS3_CONTROL_TEXTURE && GRASS3_CONTROL_TEXTURE != tr.whiteImage) ? 1.0 : 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL13, l13);
		}
		else if (isGroundFoliage && backEnd.renderPass == RENDERPASS_GROUNDFOLIAGE)
		{
			{
				vec4_t l8;
				VectorSet4(l8, FOLIAGE_SURFACE_MINIMUM_SIZE, FOLIAGE_DISTANCE_FROM_ROADS, FOLIAGE_HEIGHT, FOLIAGE_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			{
				vec4_t l12;
				VectorSet4(l12, 0.0, 0.0, 0.0, FOLIAGE_LOD_START_RANGE);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
			}
		}
		else if (isVines && backEnd.renderPass == RENDERPASS_VINES)
		{
			{
				vec4_t l8;
				VectorSet4(l8, VINES_SURFACE_MINIMUM_SIZE, 0.0, VINES_HEIGHT, VINES_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isMist && backEnd.renderPass == RENDERPASS_MIST)
		{
			{
				vec4_t l8;
				VectorSet4(l8, MIST_SURFACE_MINIMUM_SIZE, MIST_LOD_START_RANGE, MIST_HEIGHT, MIST_SURFACE_SIZE_DIVIDER);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
			}

			GL_Cull(CT_TWO_SIDED);
		}
		else if (isGrassPatches && useTesselation >= 2)
		{
			vec4_t l8;
			VectorSet4(l8, GRASS_PATCHES_DISTANCE_FROM_ROADS, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
		}
		else if (isGrass && useTesselation >= 2)
		{
			vec4_t l8;
			VectorSet4(l8, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
		}
		else if (isGrass2 && useTesselation >= 2)
		{
			vec4_t l8;
			VectorSet4(l8, GRASS2_DISTANCE_FROM_ROADS, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
		}
		else if (isGrass3 && useTesselation >= 2)
		{
			vec4_t l8;
			VectorSet4(l8, GRASS3_DISTANCE_FROM_ROADS, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
		}
		else if (isGrass4 && useTesselation >= 2)
		{
			vec4_t l8;
			VectorSet4(l8, GRASS4_DISTANCE_FROM_ROADS, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);
		}

		if (isFakeGrass && backEnd.renderPass == RENDERPASS_NONE && !IS_DEPTH_PASS)
		{// Send shader the avg grass color for above and below ground grasses, so we can draw fake distant grass...
			vec4_t grassColor, seaGrassColor;
			VectorClear4(grassColor);
			VectorClear4(seaGrassColor);

			if (isFakeGrass && tr.grassAliasImage[0])
			{
				VectorCopy4(tr.grassAliasImage[0]->averageColor, grassColor);
				grassColor[3] = 1.0;
			}

			if (isFakeGrass && tr.seaGrassAliasImage[0])
			{
				VectorCopy4(tr.seaGrassAliasImage[0]->averageColor, seaGrassColor);
				seaGrassColor[3] = 1.0;
			}

			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL14, grassColor);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL15, seaGrassColor);

			vec4_t vec;
			VectorSet4(vec, isFakeGrass ? 1.0f : 0.0f, GRASS_DISTANCE, GRASS_MAX_SLOPE, FAKE_GRASS_MINALPHA);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL16, vec);

			VectorSet4(vec, FAKE_GRASS_SCALE, FAKE_GRASS_SCALE_UNDERWATER, FAKE_GRASS_COLORMULT, FAKE_GRASS_COLORMULT_UNDERWATER);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL17, vec);

			VectorSet4(vec, FAKE_GRASS_MINALPHA_UNDERWATER, GRASS_HEIGHT*GRASS_SIZE_MULTIPLIER_UNDERWATER, FAKE_GRASS_ENABLED, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL18, vec);

			if (sp->isBindless)
			{
				GLSL_SetBindlessTexture(sp, UNIFORM_TEXTUREMAP, &tr.random2KImage[0], 0);
			}
			else
			{
				GLSL_SetUniformInt(sp, UNIFORM_TEXTUREMAP, TB_SPECULARMAP);
				GL_BindToTMU(tr.random2KImage[0], TB_SPECULARMAP);
			}

			/*if (isFakeGrass)
			{
				ri->Printf(PRINT_WARNING, "isFakeGrass: color %f %f %f %f. seaColor %f %f %f %f. dist: %f.\n"
					, grassColor[0], grassColor[1], grassColor[2], grassColor[3]
					, seaGrassColor[0], seaGrassColor[1], seaGrassColor[2], seaGrassColor[3]
					, GRASS_DISTANCE);
			}*/
		}
		else
		{
			vec4_t zero;
			VectorClear(zero);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL14, zero);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL15, zero);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL16, zero);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL17, zero);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL18, zero);
		}

		/*if ((tr.viewParms.flags & VPF_DEPTHSHADOW) && nightScale > 0.0)
		{// Shadow draws are always 2 sided, to cull sun from behind the object.
			//GL_Cull(CT_TWO_SIDED);
			if (r_testvalue0->integer > 1)
				GL_Cull(CT_TWO_SIDED);
			else if (r_testvalue0->integer)
				GL_Cull(CT_FRONT_SIDED);
			else
				GL_Cull(CT_BACK_SIDED);
		}*/

		if (tess.shader->materialType == MATERIAL_BIRD && backEnd.currentEntity != NULL && backEnd.currentEntity != &tr.worldEntity)
		{// Flying birds, send center origin for animating wings...
			vec4_t l20;
			VectorSet4(l20, backEnd.currentEntity->e.origin[0], backEnd.currentEntity->e.origin[1], backEnd.currentEntity->e.origin[2], 0.0);
			//ri->Printf(PRINT_WARNING, "bird org: %f %f %f.\n", backEnd.currentEntity->e.origin[0], backEnd.currentEntity->e.origin[1], backEnd.currentEntity->e.origin[2]);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL20, l20);
			
			GL_Cull(CT_TWO_SIDED);
		}
		else
		{
			vec4_t l20;
			VectorSet4(l20, 0.0, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL20, l20);
		}

		if (!is2D && (pStage->glow || pStage->glowMapped))
		{
			backEnd.pc.c_glowDraws++;
		}

		UpdateTexCoords(pStage);

		GL_State(stateBits);

		//
		// draw
		//

		if (sp->isBindless)
		{
			GLSL_BindlessUpdate(sp);
		}

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, (useTesselation || sp->tesselation) ? qtrue : qfalse);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, (useTesselation || sp->tesselation) ? qtrue : qfalse);
		}


		if (glState.currentFBO == tr.renderGlowFbo || glState.currentFBO == tr.renderDetailFbo || glState.currentFBO == tr.renderWaterFbo || glState.currentFBO == tr.renderTransparancyFbo)
		{// Change back to standard render FBO...
			FBO_Bind(tr.renderFbo);
		}

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap ) )
		{
			break;
		}

		if (IS_DEPTH_PASS)
			break;
	}
}

void RB_ExternalIterateStagesGeneric( shaderCommands_t *input )
{
	RB_IterateStagesGeneric( input );
}


static void RB_RenderShadowmap(shaderCommands_t *input)
{
	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
		&& r_sunlightMode->integer >= 2
		&& tr.screenShadowFbo
		&& (backEnd.viewParms.flags & VPF_USESUNLIGHT)
		&& ((backEnd.viewParms.flags & VPF_DEPTHSHADOW) || (backEnd.viewParms.flags & VPF_SHADOWPASS))
		&& SHADOWS_ENABLED
		&& RB_NightScale() < 1.0 // Can ignore rendering shadows at night...
		&& (r_deferredLighting->integer || r_fastLighting->integer))
	{
		int deformGen;
		float deformParams[7];

		ComputeDeformValues(&deformGen, deformParams);

		int useTesselation = 0;

#if 0
		float tessInner = 0.0;
		float tessOuter = 0.0;
		float tessAlpha = 0.0;

		if (r_tessellation->integer)
		{
#ifdef __TERRAIN_TESSELATION__
			if (TERRAIN_TESSELLATION_ENABLED
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0
				&& (tess.shader->isGrass || RB_ShouldUseTerrainTessellation(tess.shader->materialType)))
			{// Always add tesselation to ground surfaces...
				tess.shader->tesselation = qfalse;

				if (TERRAIN_TESSELLATION_3D_ENABLED)
				{
					useTesselation = 3;
				}
				else
				{
					useTesselation = 2;
				}

				tessInner = max(min(r_terrainTessellationMax->value, TERRAIN_TESSELLATION_LEVEL), 2.0);
				tessOuter = tessInner;
				tessAlpha = TERRAIN_TESSELLATION_OFFSET;
			}
			else
#endif //__TERRAIN_TESSELATION__
				if (input->shader->tesselation
					&& input->shader->tesselationLevel > 1.0
					&& input->shader->tesselationAlpha != 0.0)
				{
					useTesselation = 1;
				}
		}
#endif

		{
			shaderProgram_t *sp = &tr.shadowFillShader[useTesselation];

			vec4_t vector;

			GLSL_BindProgram(sp);

			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);

			GLSL_SetUniformFloat(sp, UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

			GLSL_SetUniformInt(sp, UNIFORM_DEFORMGEN, deformGen);
			if (deformGen != DGEN_NONE)
			{
				GLSL_SetUniformFloat7(sp, UNIFORM_DEFORMPARAMS, deformParams);
				GLSL_SetUniformFloat(sp, UNIFORM_TIME, tess.shaderTime);
			}

			VectorCopy(backEnd.viewParms.ori.origin, vector);
			vector[3] = 1.0f;
			GLSL_SetUniformVec4(sp, UNIFORM_LIGHTORIGIN, vector);
			GLSL_SetUniformFloat(sp, UNIFORM_LIGHTRADIUS, backEnd.viewParms.zFar);

#if 0
			if (useTesselation)
			{
				vector[0] = 0;
				vector[1] = 0;
				vector[2] = 0;
				vector[3] = useTesselation ? TERRAIN_TESSELLATION_OFFSET + 1.0 : 0.0;
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, vector);

				vec4_t l10;
				VectorSet4(l10, tessAlpha, tessInner, tessOuter, TERRAIN_TESSELLATION_MIN_SIZE);
				GLSL_SetUniformVec4(sp, UNIFORM_TESSELATION_INFO, l10);

				if (useTesselation == 3)
				{
					GLSL_SetUniformVec4(sp, UNIFORM_TESSELATION_3D_INFO, TERRAIN_TESSELLATION_3D_OFFSET);
				}

#ifdef __TERRAIN_TESSELATION__
				float TERRAIN_TESS_OFFSET = 0.0;

				// Check if this is grass on a tessellated terrain, if so, we want to lower the verts in the vert shader by the maximum possible tessellation height...
				if (TERRAIN_TESSELLATION_ENABLED
					&& r_tessellation->integer
					&& r_terrainTessellation->integer
					&& r_terrainTessellationMax->value >= 2.0)
				{// When tessellating terrain, we need to drop the grasses down lower to allow for the offset...
					TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_OFFSET;

					if (useTesselation == 3)
					{
						TERRAIN_TESS_OFFSET = TERRAIN_TESSELLATION_3D_OFFSET[2];
					}

					if (sp->isBindless)
					{
						if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.roadsMapImage, 0);
						}
						else
						{
							GLSL_SetBindlessTexture(sp, UNIFORM_ROADSCONTROLMAP, &tr.blackImage, 0);
						}

						GLSL_BindlessUpdate(sp);
					}
					else
					{
						if (tr.roadsMapImage && tr.roadsMapImage != tr.blackImage)
						{
							GL_BindToTMU(tr.roadsMapImage, TB_ROADSCONTROLMAP);
						}
						else
						{
							GL_BindToTMU(tr.blackImage, TB_ROADSCONTROLMAP);
						}
					}
				}

				GLSL_SetUniformFloat(sp, UNIFORM_ZFAR, (ENABLE_OCCLUSION_CULLING && r_occlusion->integer) ? tr.occlusionZfar : backEnd.viewParms.zFar);

				vec4_t l3;
				VectorSet4(l3, 0.0, 0.0, 0.0, (tr.roadsMapImage != tr.blackImage) ? 1.0 : 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, l3);

				vec4_t l8;
				VectorSet4(l8, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0, 0.0);
				GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, l8);

				if (useTesselation == 3)
				{
					GLSL_SetUniformVec4(sp, UNIFORM_TESSELATION_3D_INFO, TERRAIN_TESSELLATION_3D_OFFSET);
				}

				{
					vec4_t l12;
					VectorSet4(l12, TERRAIN_TESS_OFFSET, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0);
					GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, l12);
				}

				{
					extern vec3_t		MAP_INFO_MINS;
					extern vec3_t		MAP_INFO_MAXS;
					extern vec3_t		MAP_INFO_SIZE;

					vec4_t loc;
					VectorSet4(loc, MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], 0.0);
					GLSL_SetUniformVec4(sp, UNIFORM_MINS, loc);

					VectorSet4(loc, MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2], 0.0);
					GLSL_SetUniformVec4(sp, UNIFORM_MAXS, loc);

					VectorSet4(loc, MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0);
					GLSL_SetUniformVec4(sp, UNIFORM_MAPINFO, loc);
				}
#endif //__TERRAIN_TESSELATION__
			}
#endif

			GL_State(0);

			//
			// do multitexture
			//
			//if ( pStage->glslShaderGroup )
			{
				//
				// draw
				//

				if (input->multiDrawPrimitives)
				{
					R_DrawMultiElementsVBO(input->multiDrawPrimitives, input->multiDrawMinIndex, input->multiDrawMaxIndex, input->multiDrawNumIndexes, input->multiDrawFirstIndex, input->numVertexes, useTesselation > 0 ? qtrue : qfalse);
				}
				else
				{
					R_DrawElementsVBO(input->numIndexes, input->firstIndex, input->minIndex, input->maxIndex, input->numVertexes, useTesselation > 0 ? qtrue : qfalse);
				}
			}
		}
	}
}


/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	shaderCommands_t *input;
	unsigned int vertexAttribs = 0;

	input = &tess;

	if (!input->numVertexes || !input->numIndexes)
	{
		return;
	}

	if (tess.useInternalVBO)
	{
		RB_DeformTessGeometry();
	}

	vertexAttribs = RB_CalcShaderVertexAttribs( input->shader );

	if (tess.useInternalVBO)
	{
		RB_UpdateVBOs(vertexAttribs);
	}
	else
	{
		backEnd.pc.c_staticVboDraws++;
	}

	//
	// log this call
	//
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	if ((backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		//GL_Cull( CT_TWO_SIDED );

		if (input->shader->cullType == CT_TWO_SIDED)
			GL_Cull( CT_TWO_SIDED );
		else if (input->shader->cullType == CT_FRONT_SIDED)
			GL_Cull( CT_BACK_SIDED );
		else
			GL_Cull( CT_FRONT_SIDED );
	}
	else
		GL_Cull( input->shader->cullType );

	// set polygon offset if necessary
	if ( input->shader->polygonOffset )
	{
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	//
	// Set vertex attribs and pointers
	//
#ifdef __CHEAP_VERTS__
	if (/*backEnd.currentEntity == &tr.worldEntity &&*/ !tess.useInternalVBO && (vertexAttribs & ATTR_COLOR))
	{// World VBO's no longer contain attr_color to save memory, so ATTR_COLOR is unneeded.
		vertexAttribs &= ~ATTR_COLOR;
	}
#endif //__CHEAP_VERTS__
	GLSL_VertexAttribsState(vertexAttribs);

	//
	// UQ1: Set up any special shaders needed for this surface/contents type...
	//

	if ((tess.shader->isWater && r_glslWater->integer && WATER_ENABLED)
		|| (tess.shader->contentFlags & CONTENTS_WATER)
		/*|| (tess.shader->contentFlags & CONTENTS_LAVA)*/
		|| (tess.shader->materialType) == MATERIAL_WATER)
	{
		if (input && input->xstages[0] && input->xstages[0]->isWater == 0 && r_glslWater->integer && WATER_ENABLED) // In case it is already set, no need looping more then once on the same shader...
		{
			int isWater = 1;

			if (tess.shader->contentFlags & CONTENTS_LAVA)
				isWater = 2;

			for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ )
			{
				if (input->xstages[stage])
				{
					input->xstages[stage]->isWater = isWater;
				}
			}
		}
	}

	//
	// render depth if in depthfill mode
	//
	if (backEnd.depthFill)
	{
		RB_IterateStagesGeneric(input);

		//
		// reset polygon offset
		//
		if ( input->shader->polygonOffset )
		{
			qglDisable( GL_POLYGON_OFFSET_FILL );
		}

		return;
	}

	//
	// render shadowmap if in shadowmap mode
	//
	if (backEnd.viewParms.flags & VPF_SHADOWMAP) // VPF_DEPTHSHADOW ???
	{
		if ( input->shader->sort == SS_OPAQUE )
		{
			RB_RenderShadowmap( input );
		}
		//
		// reset polygon offset
		//
		if ( input->shader->polygonOffset )
		{
			qglDisable( GL_POLYGON_OFFSET_FILL );
		}

		return;
	}

	//
	//
	// call shader function
	//
	RB_IterateStagesGeneric( input );

#ifdef __PSHADOWS__
	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && r_shadows->integer == 2)
	{
		ProjectPshadowVBOGLSL();
	}
#endif

	// Now check for surfacesprites.
#ifdef __JKA_SURFACE_SPRITES__
	int SSCOUNT = 0;
	int SSDRAWNCOUNT = 0;
	if (r_surfaceSprites->integer)
	{
		//for ( int stage = 1; stage < MAX_SHADER_STAGES/*tess.shader->numUnfoggedPasses*/; stage++ )
		for ( int stage = 0; stage < MAX_SHADER_STAGES/*tess.shader->numUnfoggedPasses*/; stage++ )
		{
			if (input->xstages[stage] && input->xstages[stage]->active)
			{
				SSCOUNT++;

				if (input->xstages[stage]->ss && input->xstages[stage]->ss->surfaceSpriteType)
				{	// Draw the surfacesprite
					RB_DrawSurfaceSprites(input->xstages[stage], input);
					SSDRAWNCOUNT++;
				}
			}
		}
	}
	if (r_surfaceSprites->integer >= 5)
		ri->Printf(PRINT_WARNING, "SSCOUNT: %i. SSDRAWNCOUNT: %i.\n", SSCOUNT, SSDRAWNCOUNT);
#endif //__JKA_SURFACE_SPRITES__

	//
	// reset polygon offset
	//
	if ( input->shader->polygonOffset )
	{
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
}

