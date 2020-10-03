#include "tr_local.h"

extern qboolean		ENABLE_OCCLUSION_CULLING;
extern float		OCCLUSION_CULLING_TOLERANCE;
extern float		OCCLUSION_CULLING_TOLERANCE_FOLIAGE;

extern int R_BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p);

void ClosestPointInBoundingBox(const float *mins, const float *maxs, const float *inPoint, float *outPoint)
{
	outPoint[0] = inPoint[0];
	outPoint[1] = inPoint[1];
	outPoint[2] = inPoint[2];

	if (inPoint[0] < mins[0])
		outPoint[0] = mins[0];
	else if (inPoint[0] > maxs[0])
		outPoint[0] = maxs[0];

	if (inPoint[1] < mins[1])
		outPoint[1] = mins[1];
	else if (inPoint[1] > maxs[1])
		outPoint[1] = maxs[1];

	if (inPoint[2] < mins[2])
		outPoint[2] = mins[2];
	else if (inPoint[2] > maxs[2])
		outPoint[2] = maxs[2];
}

int occlusionFrame = 0;
int numOccluded = 0;
int numNotOccluded = 0;

// Number of occlusions is still limited to <= tr.distanceCull... So usually we won't use them all...
//#define NUM_OCCLUSION_RANGES 36
//const float occlusionRanges[] =
//	{ 1024.0, 1536.0, 2048.0, 2560.0, 3072.0, 3584.0, 4096.0, 4608.0, 5120.0, 5632.0, 6144.0, 6656.0, 7168.0, 8192.0, 9216.0, 10240.0, 12288.0, 14336.0, 16384.0, 18432.0, 20480.0, 24576.0, 28672.0, 32768.0, 36864.0, 40960.0, 49152.0, 53248.0, 57344.0, 61440.0, 65536.0, 98304.0, 131072.0, 196608.0, 262144.0, 524288.0 };

#define NUM_OCCLUSION_RANGES 18
const float occlusionRanges[] =
	{ 2048.0, 4096.0, 8192.0, 16384.0, 24576.0, 32768.0, 36864.0, 40960.0, 49152.0, 53248.0, 57344.0, 61440.0, 65536.0, 98304.0, 131072.0, 196608.0, 262144.0, 524288.0 };

//#define NUM_OCCLUSION_RANGES 11
//const float occlusionRanges[] =
//{ 2048.0, 4096.0, 8192.0, 16384.0, 24576.0, 32768.0, 49152.0, 65536.0, 98304.0, 131072.0, 524288.0 };

#define MAX_QUERIES 256

float RB_GetNextOcclusionRange(float currentRange)
{
	for (int i = 0; i < NUM_OCCLUSION_RANGES; i++)
	{
		float thisRange = occlusionRanges[i];

		if (thisRange > currentRange)
		{
			return thisRange;
		}
	}

	return currentRange;
}

int nextOcclusionCheck = -1;

GLuint	occlusionCheck[MAX_QUERIES];
int		occlusionRangeId[MAX_QUERIES];

int numOcclusionQueries = 0;

void RB_CheckOcclusions(void)
{
	if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer)
	{
		int numComplete = 0;

		float zfar = 0;
		int rangeId = 0;
		int numPassed = 0;

		float zfarFoliage = 0;
		int rangeIdFoliage = 0;
		int numPassedFoliage = 0;

		// Total pixels of this screen...
		float screenPixels = float(glConfig.vidWidth * glConfig.vidHeight) * r_superSampleMultiplier->value;

		for (int i = 0; i < numOcclusionQueries; i++)
		{
			/* Occlusion culling check */
			GLuint result;
			qglGetQueryObjectuiv(occlusionCheck[i], GL_QUERY_RESULT_AVAILABLE, &result);

			if (result)
			{
				numComplete++;

				//ri->Printf(PRINT_WARNING, "Occlusion check %i finished in time!\n", i);

				qglGetQueryObjectuiv(occlusionCheck[i], GL_QUERY_RESULT, &result);

				// OCCLUSION_CULLING_TOLERANCE should be between 0.0 and 0.001.. Lower is more accurate...
				float pixelTolerance = Q_min(OCCLUSION_CULLING_TOLERANCE, 0.1) * screenPixels;

				if (result <= 0)
				{// Absolutely occlusion culled... Also handles non-tolerance setting...
					continue;
				}
				
				if (OCCLUSION_CULLING_TOLERANCE > 0.0 && result <= pixelTolerance)
				{// Occlusion culled...
					continue;
				}

				// Enough pixels are visible on this test, if further away then the previous tests, set new zfar...
				int thisRangeId = occlusionRangeId[i];
				float rangeDistance = occlusionRanges[thisRangeId];

				if (rangeDistance > zfar)
				{// Seems this is further away then the previous occlusion tests zfar, use this instead...
					rangeId = thisRangeId;
					zfar = rangeDistance;
				}

				numPassed++;

				if (OCCLUSION_CULLING_TOLERANCE_FOLIAGE > 0.0)
				{
					// Also do a check specific to foliages, maybe we can skip some heavy alpha draws...
					float pixelToleranceFoliage = Q_min(OCCLUSION_CULLING_TOLERANCE_FOLIAGE, 0.3) * screenPixels;

					if (result <= pixelToleranceFoliage)
					{// Occlusion culled...
						continue;
					}
					
					// Enough pixels are visible on this test, if further away then the previous tests, set new zfar...
					if (rangeDistance > zfarFoliage)
					{// Seems this is further away then the previous occlusion tests zfar, use this instead...
						rangeIdFoliage = thisRangeId;
						zfarFoliage = rangeDistance;
					}

					numPassedFoliage++;
				}
			}
			else
			{
				return; // reuse old zfar until completion?
			}
			/* Occlusion culling check */
		}

		if (zfar < tr.distanceCull)
		{// Seems we found a max zfar we can use...
			int maxRangeId = NUM_OCCLUSION_RANGES - 1;

			if (zfar == 0.0 && numPassed == 0)
			{// If none passed then we should assume minimum zfar...
				zfar = occlusionRanges[1];
			}
			else if (zfar == 0.0)
			{// If none passed then we assume max range... This should never be possible, but just in case...
				zfar = tr.distanceCull;
			}
			else
			{// We got a value to use, move it forward 1 range level...
				if (rangeId >= maxRangeId)
				{// If we are at furthest range, use the max range instead...
					zfar = tr.distanceCull;
				}
				else
				{
					zfar = occlusionRanges[rangeId + 1];
				}
			}

			if (zfar > tr.distanceCull)
			{
				zfar = tr.distanceCull;
			}
		}

		if (OCCLUSION_CULLING_TOLERANCE_FOLIAGE > 0.0)
		{
			// And check the foliage one as well...
			if (zfarFoliage < tr.distanceCull)
			{// Seems we found a max zfar we can use...
				int maxRangeId = NUM_OCCLUSION_RANGES - 1;

				if (zfarFoliage == 0.0 && numPassedFoliage == 0)
				{// If none passed then we should assume minimum zfar...
					zfarFoliage = occlusionRanges[1];
				}
				else if (zfarFoliage == 0.0)
				{// If none passed then we assume max range... This should never be possible, but just in case...
					zfarFoliage = tr.distanceCull;
				}
				else
				{// We got a value to use, move it forward 1 range level...
					if (rangeIdFoliage >= maxRangeId)
					{// If we are at furthest range, use the max range instead...
						zfarFoliage = tr.distanceCull;
					}
					else
					{
						zfarFoliage = occlusionRanges[rangeIdFoliage + 1];
					}
				}

				if (zfarFoliage > tr.distanceCull)
				{
					zfarFoliage = tr.distanceCull;
				}
			}
		}
		else
		{
			tr.occlusionZfarFoliage = tr.occlusionZfar;
		}

		if (tr.occlusionZfar != zfar)
		{// Just update when it changes...
			tr.occlusionZfar = max(zfar, OCCLUSION_CULLING_MIN_DISTANCE);

			if (r_occlusionDebug->integer >= 1)
				ri->Printf(PRINT_WARNING, "zFar found at %f.\n", tr.occlusionZfar);
		}

		if (tr.occlusionZfarFoliage != zfarFoliage)
		{// Just update when it changes...
			tr.occlusionZfarFoliage = max(zfarFoliage, OCCLUSION_CULLING_MIN_DISTANCE);

			if (r_occlusionDebug->integer >= 1)
				ri->Printf(PRINT_WARNING, "zFar (foliage) found at %f.\n", tr.occlusionZfarFoliage);
		}

		nextOcclusionCheck = 1; // Since we finished a query, allow next query to begin straight away...
	}
	else
	{
		tr.occlusionZfar = tr.distanceCull;
		tr.occlusionZfarFoliage = tr.distanceCull;
	}
}

void RB_MoveSky(void)
{
	//
	// Adjust sky pixels to be at camera depth, and write back to depth buffer... Then sky is ignored by the occlusion checks :)
	//

	vec4i_t dstBox;

	dstBox[0] = backEnd.viewParms.viewportX;
	dstBox[1] = backEnd.viewParms.viewportY;
	dstBox[2] = backEnd.viewParms.viewportWidth;
	dstBox[3] = backEnd.viewParms.viewportHeight;

	// Copy the depth map to genericFboImage...
	FBO_BlitFromTexture(tr.renderDepthImage, dstBox, NULL, tr.genericDepthFbo, dstBox, NULL, colorWhite, 0);
	
	FBO_Bind(tr.depthAdjustFbo);
	qglEnable(GL_DEPTH_TEST);
	qglDepthMask(GL_TRUE);
	//qglDepthFunc(GL_ALWAYS);

	shaderProgram_t *shader = &tr.depthAdjustShader;

	GLSL_BindProgram(shader);

	if (shader->isBindless)
	{
		GLSL_SetBindlessTexture(shader, UNIFORM_SCREENDEPTHMAP, &tr.genericDepthImage, 0);
		GLSL_BindlessUpdate(shader);
	}
	else
	{
		GLSL_SetUniformInt(shader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GL_BindToTMU(tr.genericDepthImage, TB_LIGHTMAP);
	}

	GLSL_SetUniformMatrix16(shader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);

	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK_TRUE);
	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_Cull(CT_TWO_SIDED);
	GL_SetDepthRange(0, 1);

	vec2_t texCoords[4];

	VectorSet2(texCoords[0], 0.0f, 0.0f);
	VectorSet2(texCoords[1], 1.0f, 0.0f);
	VectorSet2(texCoords[2], 1.0f, 1.0f);
	VectorSet2(texCoords[3], 0.0f, 1.0f);

	vec4_t quadVerts[4];

	VectorSet4(quadVerts[0], -1, 1, 0, 1);
	VectorSet4(quadVerts[1], 1, 1, 0, 1);
	VectorSet4(quadVerts[2], 1, -1, 0, 1);
	VectorSet4(quadVerts[3], -1, -1, 0, 1);

	RB_InstantQuad2(quadVerts, texCoords);

#ifdef __VR_SEPARATE_EYE_RENDER__
	if (vr_stereoEnabled->integer)
	{
		if (backEnd.stereoFrame == STEREO_LEFT)
		{
			FBO_Bind(tr.renderLeftVRFbo);
		}
		else if (backEnd.stereoFrame == STEREO_RIGHT)
		{
			FBO_Bind(tr.renderRightVRFbo);
		}
		else
		{
			FBO_Bind(tr.renderFbo);
		}
	}
#else //!__VR_SEPARATE_EYE_RENDER__
	FBO_Bind(tr.renderFbo);
#endif //__VR_SEPARATE_EYE_RENDER__
}

void RB_OcclusionCulling(void)
{
	//
	// Render a bunch of occlusion quads, starting a little in front of the viewer, into the distance, covering the whole screen. This gives us zfar to use...
	//

	if (!r_nocull->integer)
	{
		if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer)
		{
			// finish any 2D drawing if needed
			if (tess.numIndexes) {
				RB_EndSurface();
			}

			qboolean first = qtrue;

			if (nextOcclusionCheck == 0)
			{// Initialize...
				tr.occlusionZfar = tr.distanceCull;
			}
			else
			{
				first = qfalse;
			}

			if (backEnd.refdef.time < nextOcclusionCheck)
			{
				return;
			}

			nextOcclusionCheck = backEnd.refdef.time + 100; // Max of 100ms between occlusion checks?

			numOcclusionQueries = 0;

			RB_MoveSky();

			occlusionFrame++;

			GL_Bind(tr.whiteImage);

#ifdef __VR_SEPARATE_EYE_RENDER__
			if (vr_stereoEnabled->integer)
			{
				if (backEnd.stereoFrame == STEREO_LEFT)
				{
					FBO_Bind(tr.renderLeftVRFbo);
				}
				else if (backEnd.stereoFrame == STEREO_RIGHT)
				{
					FBO_Bind(tr.renderRightVRFbo);
				}
				else
				{
					FBO_Bind(tr.renderFbo);
				}
			}
#else //!__VR_SEPARATE_EYE_RENDER__
			FBO_Bind(tr.renderFbo);
#endif //__VR_SEPARATE_EYE_RENDER__

			RB_UpdateVBOs(ATTR_POSITION);
			GLSL_VertexAttribsState(ATTR_POSITION);
			GLSL_BindProgram(&tr.occlusionShader);

			GLSL_SetUniformMatrix16(&tr.occlusionShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);
			GLSL_SetUniformVec4(&tr.occlusionShader, UNIFORM_COLOR, colorWhite);

			/*if (tr.occlusionShader.isBindless)
			{
				GLSL_SetBindlessTexture(&tr.occlusionShader, UNIFORM_SCREENDEPTHMAP, &tr.renderDepthImage, 0);
				GLSL_BindlessUpdate(&tr.occlusionShader);
			}
			else
			{
				GLSL_SetUniformInt(&tr.occlusionShader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
				GL_BindToTMU(tr.renderDepthImage, TB_LIGHTMAP);
			}*/

			GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS);

			// Don't draw into color or depth
			//GL_State(0);
			//qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

			//GL_State(GLS_DEFAULT);

#define __DEBUG_OCCLUSION__

#ifdef __DEBUG_OCCLUSION__
			if (r_occlusionDebug->integer >= 3)
			{
				qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			}
			else
			{
				qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			}
#else //!__DEBUG_OCCLUSION__
			qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
#endif //__DEBUG_OCCLUSION__

			qglDepthMask(GL_FALSE);

			GL_Cull(CT_TWO_SIDED);
			//GL_SetDepthRange(0, 0);
			GL_SetDepthRange(0, 1);


			vec3_t mOrigin, mCameraForward, mCameraLeft, mCameraDown;
	
			VectorCopy(vec3_origin, mOrigin);

			// Using renderer axis value...
			extern void TR_AxisToAngles(const vec3_t axis[3], vec3_t angles);
			vec3_t mAngles;
			TR_AxisToAngles(backEnd.viewParms.ori.axis, mAngles);
			AngleVectors(mAngles, mCameraForward, mCameraLeft, mCameraDown);

			//ri->Printf(PRINT_WARNING, "ViewOrigin %.4f %.4f %.4f. ViewAngles %.4f %.4f %.4f.\n", mOrigin[0], mOrigin[1], mOrigin[2], viewangles[0], viewangles[1], viewangles[2]);

			tess.numIndexes = 0;
			tess.firstIndex = 0;
			tess.numVertexes = 0;
			tess.minIndex = 0;
			tess.maxIndex = 0;
			
			for (int z = occlusionRanges[0], range = 0; z <= tr.distanceCull * 1.75 && range < NUM_OCCLUSION_RANGES; z = occlusionRanges[range], range++)
			{
				if (z < OCCLUSION_CULLING_MIN_DISTANCE)
				{// No point checking this one, mapinfo specified minimum range as higher...
					continue;
				}

				occlusionRangeId[numOcclusionQueries] = range;

#if 1
				vec2_t texCoords[4];

				VectorSet2(texCoords[0], 0.0f, 0.0f);
				VectorSet2(texCoords[1], 1.0f, 0.0f);
				VectorSet2(texCoords[2], 1.0f, 1.0f);
				VectorSet2(texCoords[3], 0.0f, 1.0f);

				vec3_t mPosition, mLeftPositionDown, mLeftPositionUp, mRightPositionDown, mRightPositionUp;
				
				VectorMA(mOrigin, first ? z : z - (occlusionRanges[0] / 2.0), mCameraForward, mPosition);
				
//#define quadSize tr.distanceCull
//#define quadSize 65536.0
//#define quadSize z
#define quadSize (z * 2.0)

				VectorMA(mPosition, -quadSize, mCameraLeft, mLeftPositionDown);
				VectorMA(mLeftPositionDown, -quadSize, mCameraDown, mLeftPositionDown);

				VectorMA(mPosition, quadSize, mCameraLeft, mRightPositionDown);
				VectorMA(mRightPositionDown, -quadSize, mCameraDown, mRightPositionDown);

				VectorMA(mPosition, quadSize, mCameraLeft, mRightPositionUp);
				VectorMA(mRightPositionUp, quadSize, mCameraDown, mRightPositionUp);

				VectorMA(mPosition, -quadSize, mCameraLeft, mLeftPositionUp);
				VectorMA(mLeftPositionUp, quadSize, mCameraDown, mLeftPositionUp);

				

				vec4_t quadVerts[4];
				VectorSet4(quadVerts[0], mLeftPositionDown[0], mLeftPositionDown[1], mLeftPositionDown[2], 1.0);
				VectorSet4(quadVerts[1], mRightPositionDown[0], mRightPositionDown[1], mRightPositionDown[2], 1.0);
				VectorSet4(quadVerts[2], mRightPositionUp[0], mRightPositionUp[1], mRightPositionUp[2], 1.0);
				VectorSet4(quadVerts[3], mLeftPositionUp[0], mLeftPositionUp[1], mLeftPositionUp[2], 1.0);
#else
				vec4_t quadVerts[4];
				vec2_t texCoords[4];
				vec4_t box;

				box[0] = backEnd.viewParms.viewportX      * tr.volumetricFbo->width / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
				box[1] = backEnd.viewParms.viewportY      * tr.volumetricFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
				box[2] = backEnd.viewParms.viewportWidth  * tr.volumetricFbo->width / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
				box[3] = backEnd.viewParms.viewportHeight * tr.volumetricFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

				qglViewport(box[0], box[1], box[2], box[3]);
				qglScissor(box[0], box[1], box[2], box[3]);

				box[0] = backEnd.viewParms.viewportX / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
				box[1] = backEnd.viewParms.viewportY / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
				box[2] = box[0] + backEnd.viewParms.viewportWidth / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
				box[3] = box[1] + backEnd.viewParms.viewportHeight / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

				texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
				texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
				texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
				texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

				box[0] = -1.0f;
				box[1] = -1.0f;
				box[2] = 1.0f;
				box[3] = 1.0f;

				VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
				VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
				VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
				VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

				float depth = Q_clamp(0.0, z / (tr.distanceCull * 1.75), 1.0);
				GL_SetDepthRange(depth, depth);
#endif

#ifdef __DEBUG_OCCLUSION__
				if (r_occlusionDebug->integer >= 3)
				{
					vec4_t debugColor;
					VectorSet4(debugColor, 0.0, 0.0, 1.0, (z / tr.distanceCull) + 0.1);
					GLSL_SetUniformVec4(&tr.occlusionShader, UNIFORM_COLOR, debugColor);
				}
#endif //__DEBUG_OCCLUSION__

				/* Test the occlusion for this quad */
				qglGenQueries(1, &occlusionCheck[numOcclusionQueries]);

				if (OCCLUSION_CULLING_TOLERANCE > 0.0)
					qglBeginQuery(GL_SAMPLES_PASSED, occlusionCheck[numOcclusionQueries]);
				else
					qglBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, occlusionCheck[numOcclusionQueries]); // This is supposebly a little faster??? I don't see it though...

				RB_InstantQuad2(quadVerts, texCoords);

				if (OCCLUSION_CULLING_TOLERANCE > 0.0)
					qglEndQuery(GL_SAMPLES_PASSED);
				else
					qglEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE); // This is supposebly a little faster??? I don't see it though...

				numOcclusionQueries++;
			}


			tess.numIndexes = 0;
			tess.firstIndex = 0;
			tess.numVertexes = 0;
			tess.minIndex = 0;
			tess.maxIndex = 0;

			//GL_SetDepthRange( 0, 1 );
			qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			qglDepthMask(GL_TRUE);
			GL_State(GLS_DEFAULT);
			GL_Cull(CT_FRONT_SIDED);
			R_BindNullVBO();
			R_BindNullIBO();

			//FBO_Bind(glState.previousFBO);

			if (r_occlusionDebug->integer == 2)
			{
				ri->Printf(PRINT_WARNING, "%i occlusion queries performed this frame (%i).\n", numOcclusionQueries, occlusionFrame);
			}

			if (r_occlusion->integer == 2)
			{
				qglFlush();
			}
			else if (r_occlusion->integer == 3)
			{
				qglFinish();
			}
		}

		if (r_occlusionDebug->integer >= 3)
		{
			vec4i_t dstBox;
#ifdef __DEBUG_OCCLUSION__
			VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
			FBO_BlitFromTexture(tr.renderImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);

			VectorSet4(dstBox, 512, glConfig.vidHeight - 256, 256, 256);
			FBO_BlitFromTexture(tr.genericDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);

			VectorSet4(dstBox, 768, glConfig.vidHeight - 256, 256, 256);
			FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
#else //!__DEBUG_OCCLUSION__
			VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
			FBO_BlitFromTexture(tr.dummyImage/*tr.genericDepthImage*/, NULL, NULL, NULL, dstBox, NULL, NULL, 0);

			VectorSet4(dstBox, 768, glConfig.vidHeight - 256, 256, 256);
			FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
#endif //__DEBUG_OCCLUSION__
		}
	}
}

void FBO_BlitFromDepthTexture(struct image_s *src, FBO_t *dst, struct shaderProgram_s *shaderProgram)
{
	vec4i_t dstBox, srcBox;
	vec4_t color;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	vec2_t invTexRes;
	FBO_t *oldFbo = glState.currentFBO;
	matrix_t projection;
	int width, height;

	if (!src)
		return;

	VectorSet4(srcBox, 0, 0, src->width, src->height);
	VectorSet4(dstBox, 0, glConfig.vidHeight * r_superSampleMultiplier->value, glConfig.vidWidth * r_superSampleMultiplier->value, 0);
	VectorCopy4(colorWhite, color);

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
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}

	qglViewport(0, 0, width, height);
	qglScissor(0, 0, width, height);

	Matrix16Ortho(0, width, height, 0, 0, 1, projection);

	qglDisable(GL_CULL_FACE);

	VectorSet4(quadVerts[0], dstBox[0], dstBox[1], 0, 1);
	VectorSet4(quadVerts[1], dstBox[2], dstBox[1], 0, 1);
	VectorSet4(quadVerts[2], dstBox[2], dstBox[3], 0, 1);
	VectorSet4(quadVerts[3], dstBox[0], dstBox[3], 0, 1);

	texCoords[0][0] = srcBox[0] / (float)src->width; texCoords[0][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[1][0] = srcBox[2] / (float)src->width; texCoords[1][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[2][0] = srcBox[2] / (float)src->width; texCoords[2][1] = 1.0f - srcBox[3] / (float)src->height;
	texCoords[3][0] = srcBox[0] / (float)src->width; texCoords[3][1] = 1.0f - srcBox[3] / (float)src->height;

	invTexRes[0] = 1.0f / width;
	invTexRes[1] = 1.0f / height;

	GL_State(0);

	GLSL_BindProgram(shaderProgram);

#ifdef __TEXTURECOLOR_SHADER_BINDLESS__
	if (shaderProgram->isBindless)
	{
		GLSL_SetBindlessTexture(shaderProgram, UNIFORM_DIFFUSEMAP, &src, 0);
		GLSL_BindlessUpdate(shaderProgram);

		GLSL_SetBindlessTexture(shaderProgram, UNIFORM_SCREENDEPTHMAP, &tr.renderDepthImage, 0);
		GLSL_BindlessUpdate(shaderProgram);
	}
	else
	{
		GLSL_SetUniformInt(shaderProgram, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GL_BindToTMU(src, TB_DIFFUSEMAP);

		GLSL_SetUniformInt(shaderProgram, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
		GL_BindToTMU(tr.renderDepthImage, TB_LIGHTMAP);
	}
#else //!__TEXTURECOLOR_SHADER_BINDLESS__
	GL_BindToTMU(src, TB_DIFFUSEMAP);
#endif //__TEXTURECOLOR_SHADER_BINDLESS__

	GLSL_SetUniformMatrix16(shaderProgram, UNIFORM_MODELVIEWPROJECTIONMATRIX, projection, 1);
	GLSL_SetUniformVec4(shaderProgram, UNIFORM_COLOR, color);
	GLSL_SetUniformVec2(shaderProgram, UNIFORM_INVTEXRES, invTexRes);

	{
		vec4_t loc;
		VectorSet4(loc, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value);
		GLSL_SetUniformVec4(shaderProgram, UNIFORM_LOCAL0, loc);
	}

	RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

	FBO_Bind(oldFbo);
}

void RB_zFarCullingEndFrame(void)
{
	if (r_zFarOcclusion->integer)
	{
		FBO_BlitFromDepthTexture(tr.zfarNullImage, tr.zfarDepthFbo, &tr.zFarDepthShader);
	}
}

void RB_zFarCullingBeginFrame(void)
{//tr.zfarDepthImage
	if (r_zFarOcclusion->integer)
	{
		// Copy the precious max depth to renderDepthFbo...
		//FBO_BlitFromTexture(tr.zfarDepthImage, NULL, NULL, tr.renderDepthFbo, NULL, NULL, colorWhite, 0);

		shaderProgram_t *shader = &tr.zFarCopyShader;

		FBO_Bind(tr.renderDepthFbo);

		qglEnable(GL_DEPTH_TEST);
		qglDepthMask(GL_TRUE);
		//qglDepthFunc(GL_ALWAYS);

		GLSL_BindProgram(shader);

		if (shader->isBindless)
		{
			GLSL_SetBindlessTexture(shader, UNIFORM_SCREENDEPTHMAP, &tr.zfarDepthImage, 0);
			GLSL_BindlessUpdate(shader);
		}
		else
		{
			GLSL_SetUniformInt(shader, UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);
			GL_BindToTMU(tr.zfarDepthImage, TB_LIGHTMAP);
		}

		{
			vec4_t loc;
			VectorSet4(loc, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value);
			GLSL_SetUniformVec4(shader, UNIFORM_LOCAL0, loc);
		}

		GLSL_SetUniformMatrix16(shader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);

		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK_TRUE);
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		GL_Cull(CT_TWO_SIDED);
		GL_SetDepthRange(0, 1);

		vec2_t texCoords[4];

		VectorSet2(texCoords[0], 0.0f, 0.0f);
		VectorSet2(texCoords[1], 1.0f, 0.0f);
		VectorSet2(texCoords[2], 1.0f, 1.0f);
		VectorSet2(texCoords[3], 0.0f, 1.0f);

		vec4_t quadVerts[4];

		VectorSet4(quadVerts[0], -1, 1, 0, 1);
		VectorSet4(quadVerts[1], 1, 1, 0, 1);
		VectorSet4(quadVerts[2], 1, -1, 0, 1);
		VectorSet4(quadVerts[3], -1, -1, 0, 1);

		RB_InstantQuad2(quadVerts, texCoords);

#ifdef __VR_SEPARATE_EYE_RENDER__
		if (vr_stereoEnabled->integer)
		{
			if (backEnd.stereoFrame == STEREO_LEFT)
			{
				FBO_Bind(tr.renderLeftVRFbo);
			}
			else if (backEnd.stereoFrame == STEREO_RIGHT)
			{
				FBO_Bind(tr.renderRightVRFbo);
			}
			else
			{
				FBO_Bind(tr.renderFbo);
			}
		}
#else //!__VR_SEPARATE_EYE_RENDER__
		FBO_Bind(tr.renderFbo);
#endif //__VR_SEPARATE_EYE_RENDER__
	}
}
