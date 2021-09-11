// tr_QuickSprite.cpp: implementation of the CQuickSpriteSystem class.
//
//////////////////////////////////////////////////////////////////////

#include "tr_quicksprite.h"

#ifdef __JKA_SURFACE_SPRITES__

extern void R_BindAnimatedImageToTMU( textureBundle_t *bundle, int tmu );


//////////////////////////////////////////////////////////////////////
// Singleton System
//////////////////////////////////////////////////////////////////////
CQuickSpriteSystem SQuickSprite;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuickSpriteSystem::CQuickSpriteSystem()
{
	int i;

	for (i=0; i<SHADER_MAX_VERTEXES; i+=4)
	{
		// Bottom right
		mTextureCoords[i+0][0] = 1.0;
		mTextureCoords[i+0][1] = 1.0;
		// Top right
		mTextureCoords[i+1][0] = 1.0;
		mTextureCoords[i+1][1] = 0.0;
		// Top left
		mTextureCoords[i+2][0] = 0.0;
		mTextureCoords[i+2][1] = 0.0;
		// Bottom left
		mTextureCoords[i+3][0] = 0.0;
		mTextureCoords[i+3][1] = 1.0;
	}
}

CQuickSpriteSystem::~CQuickSpriteSystem()
{

}


void CQuickSpriteSystem::Flush(void)
{
	if (mNextVert == 0)
	{
		return;
	}

	R_DrawElementsVBOIndirectCheckFinish(&tess);

	/*
	if (mUseFog && r_drawfog->integer == 2 &&
		mFogIndex == tr.world->globalFog)
	{ //enable hardware fog when we draw this thing if applicable -rww
		fog_t *fog = tr.world->fogs + mFogIndex;

		qglFogf(GL_FOG_MODE, GL_EXP2);
		qglFogf(GL_FOG_DENSITY, logtestExp2 / fog->parms.depthForOpaque);
		qglFogfv(GL_FOG_COLOR, fog->parms.color);
		qglEnable(GL_FOG);
	}
	*/
	//this should not be needed, since I just wait to disable fog for the surface til after surface sprites are done
	//ri->Printf(PRINT_WARNING, "Drawing ss.\n");

	//
	// render the main pass
	//
	//qglLoadIdentity ();

	//GL_State(mGLStateBits);

	extern void SetViewportAndScissor(void);
	SetViewportAndScissor();
	GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);
	GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);

	//GL_State((mTexBundle->image[0]->hasAlpha) ? (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA) : (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE));
	GL_State(mGLStateBits);
	GL_Cull(CT_TWO_SIDED);
	
	GLSL_BindProgram(&tr.textureColorShader);
	GL_Bind(mTexBundle->image[0]);
	//R_BindAnimatedImageToTMU(mTexBundle, TB_DIFFUSEMAP);

	GLSL_SetUniformMatrix16(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

#ifdef __TEXTURECOLOR_SHADER_BINDLESS__
	if (tr.textureColorShader.isBindless)
	{
		GLSL_SetBindlessTexture(&tr.textureColorShader, UNIFORM_DIFFUSEMAP, &tr.whiteImage, 0);
		GLSL_BindlessUpdate(&tr.textureColorShader);
	}
#endif //__TEXTURECOLOR_SHADER_BINDLESS__

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	tess.minIndex = 0;

	for (int i = 0; i < mNextVert; i += 4)
	{
		if (tess.numVertexes + 4 >= SHADER_MAX_VERTEXES || tess.numIndexes + 6 >= SHADER_MAX_INDEXES)
		{// Would go over the limit, render current queue and continue...
			RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0);
			GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0);
			R_DrawElementsVBO(&tess, tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);

			tess.numIndexes = 0;
			tess.numVertexes = 0;
			tess.firstIndex = 0;
			tess.minIndex = 0;
			tess.maxIndex = 0;
		}

		int ndx = tess.numVertexes;
		int idx = tess.numIndexes;

		// triangle indexes for a simple quad
		tess.indexes[idx + 0] = ndx + 0;
		tess.indexes[idx + 1] = ndx + 1;
		tess.indexes[idx + 2] = ndx + 2;
		tess.indexes[idx + 3] = ndx + 0;
		tess.indexes[idx + 4] = ndx + 2;
		tess.indexes[idx + 5] = ndx + 3;
		/*tess.indexes[idx + 0] = ndx + 0;
		tess.indexes[idx + 1] = ndx + 1;
		tess.indexes[idx + 2] = ndx + 3;
		tess.indexes[idx + 3] = ndx + 3;
		tess.indexes[idx + 4] = ndx + 1;
		tess.indexes[idx + 5] = ndx + 2;*/

		VectorCopy2(mTextureCoords[i + 0], tess.texCoords[ndx + 0][0]);
		VectorCopy2(mTextureCoords[i + 0], tess.texCoords[ndx + 0][1]);

		VectorCopy2(mTextureCoords[i + 1], tess.texCoords[ndx + 1][0]);
		VectorCopy2(mTextureCoords[i + 1], tess.texCoords[ndx + 1][1]);

		VectorCopy2(mTextureCoords[i + 2], tess.texCoords[ndx + 2][0]);
		VectorCopy2(mTextureCoords[i + 2], tess.texCoords[ndx + 2][1]);

		VectorCopy2(mTextureCoords[i + 3], tess.texCoords[ndx + 3][0]);
		VectorCopy2(mTextureCoords[i + 3], tess.texCoords[ndx + 3][1]);

		//tess.vertexColors[ndx] = mColors[i];

		VectorCopy4(mVerts[i + 0], tess.xyz[ndx + 0]);
		tess.normal[ndx + 0] = R_TessXYZtoPackedNormals(tess.xyz[ndx + 0]);

		VectorCopy4(mVerts[i + 1], tess.xyz[ndx + 1]);
		tess.normal[ndx + 1] = R_TessXYZtoPackedNormals(tess.xyz[ndx + 1]);

		VectorCopy4(mVerts[i + 2], tess.xyz[ndx + 2]);
		tess.normal[ndx + 2] = R_TessXYZtoPackedNormals(tess.xyz[ndx + 2]);

		VectorCopy4(mVerts[i + 3], tess.xyz[ndx + 3]);
		tess.normal[ndx + 3] = R_TessXYZtoPackedNormals(tess.xyz[ndx + 3]);

		tess.maxIndex = ndx + 3;

		tess.numVertexes += 4;
		tess.numIndexes += 6;
	}

	RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0);
	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0);
	R_DrawElementsVBO(&tess, tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);


	//
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
	tess.minIndex = 0;
	tess.maxIndex = 0;

	//backEnd.pc.c_vertexes += mNextVert;
	//backEnd.pc.c_indexes += mNextVert;
	//backEnd.pc.c_totalIndexes += mNextVert;

	//only for software fog pass (global soft/volumetric) -rww
#if 0
	if (mUseFog && (r_drawfog->integer != 2 || mFogIndex != tr.world->globalFog))
	{
		fog_t *fog = tr.world->fogs + mFogIndex;

		//
		// render the fog pass
		//
		GL_Bind( tr.fogImage );
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

		//
		// set arrays and lock
		//
		qglTexCoordPointer( 2, GL_FLOAT, 0, mFogTextureCoords);
//		qglEnableClientState( GL_TEXTURE_COORD_ARRAY);	// Done above

		qglDisableClientState( GL_COLOR_ARRAY );
		qglColor4ubv((GLubyte *)&fog->colorInt);

//		qglVertexPointer (3, GL_FLOAT, 16, mVerts);	// Done above

		qglDrawArrays(GL_QUADS, 0, mNextVert);

		// Second pass from fog
		backEnd.pc.c_totalIndexes += mNextVert;
	}
#endif

	//
	// unlock arrays
	//
	//if (qglUnlockArraysEXT)
	//{
	//	qglUnlockArraysEXT();
	//	GLimp_LogComment( "glUnlockArraysEXT\n" );
	//}

	mNextVert=0;
}


void CQuickSpriteSystem::StartGroup(textureBundle_t *bundle, uint32_t glbits, int fogIndex )
{
	mNextVert = 0;

	mTexBundle = bundle;
	mGLStateBits = glbits;
	if (fogIndex != -1)
	{
		mUseFog = qtrue;
		mFogIndex = fogIndex;
	}
	else
	{
		mUseFog = qfalse;
	}

	//qglDisable(GL_CULL_FACE);
}


void CQuickSpriteSystem::EndGroup(void)
{
	Flush();

	//qglColor4ub(255,255,255,255);
	//qglEnable(GL_CULL_FACE);
}




void CQuickSpriteSystem::Add(float *pointdata, color4ub_t color, vec2_t fog)
{
	float *curcoord;
	float *curfogtexcoord;
	uint32_t *curcolor;

	if (mNextVert>SHADER_MAX_VERTEXES-4)
	{
		Flush();
	}

	curcoord = mVerts[mNextVert];
	// This is 16*sizeof(float) because, pointdata comes from a float[16]
	memcpy(curcoord, pointdata, 16*sizeof(float));

	// Set up color
	curcolor = &mColors[mNextVert];
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;

	if (fog)
	{
		curfogtexcoord = &mFogTextureCoords[mNextVert][0];
		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		mUseFog=qtrue;
	}
	else
	{
		mUseFog=qfalse;
	}

	mNextVert+=4;
}

#endif //__JKA_SURFACE_SPRITES__
