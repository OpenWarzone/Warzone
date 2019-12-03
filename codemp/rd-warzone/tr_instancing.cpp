#include "tr_local.h"

#ifdef __INSTANCED_MODELS__

#include <time.h>
#include <stdlib.h>
#include <math.h>

#include "VectorUtils3.h"

typedef struct modelInstanceData_s
{
	mdvModel_t		*mModel = { NULL };
	vec3_t			mOrigin = { 0 };
	//matrix_t		mMatrix = { 0 };
	vec3_t			mAngles = { 0 };
	vec3_t			mScale = { 1 };
	//trRefEntity_t	*mEntity = { NULL };
} modelInstanceData_t;

typedef struct modelInstances_s
{
	int					instanceModelTypes = 0;
	mdvModel_t			*mModels[MAX_INSTANCED_MODEL_TYPES] = { NULL };
	int					instanceModelCounts[MAX_INSTANCED_MODEL_TYPES];
	modelInstanceData_t	instanceModelInfos[MAX_INSTANCED_MODEL_TYPES][MAX_INSTANCED_MODEL_INSTANCES];
} modelInstances_t;

modelInstances_t		mInstances;

void R_AddInstancedModelToList(mdvModel_t *model, vec3_t origin, vec3_t angles, matrix_t model_matrix, trRefEntity_t *ent)
{
	qboolean	FOUND = qfalse;
	int			modelID = 0;

	for (modelID = 0; modelID < mInstances.instanceModelTypes && modelID < MAX_INSTANCED_MODEL_TYPES; modelID++)
	{
		if (mInstances.mModels[modelID] == NULL)
		{
			break;
		}

		if (mInstances.mModels[modelID] == model)
		{
			if (mInstances.instanceModelCounts[modelID] + 1 < MAX_INSTANCED_MODEL_INSTANCES)
			{
				FOUND = qtrue;
				break;
			}
		}
	}

	if (!FOUND)
	{
		if (mInstances.instanceModelTypes + 1 >= MAX_INSTANCED_MODEL_TYPES) return; // Uh oh...

		mInstances.instanceModelTypes++;
		mInstances.mModels[modelID] = model;
	}

	if (mInstances.instanceModelCounts[modelID] + 1 < MAX_INSTANCED_MODEL_INSTANCES)
	{
		mInstances.instanceModelInfos[modelID][mInstances.instanceModelCounts[modelID]].mModel = model; // pointer to the original model, so we can look up it's info in sorts...

		VectorCopy(origin, mInstances.instanceModelInfos[modelID][mInstances.instanceModelCounts[modelID]].mOrigin);
		VectorCopy(ent->e.modelScale, mInstances.instanceModelInfos[modelID][mInstances.instanceModelCounts[modelID]].mScale);
		//mEntities[modelID][mInstances.instanceModelCounts[modelID]] = ent;
#if 0
		VectorCopy(angles, mAngles[modelID][mInstances.instanceModelCounts[modelID]]);

		//Matrix16Copy(model_matrix, mMatrixes[modelID][mInstances.instanceModelCounts[modelID]]);
		//Matrix16Copy(glState.modelviewProjection, mMatrixes[modelID][mInstances.instanceModelCounts[modelID]]);
		//Matrix16Multiply(glState.modelviewProjection, glState.modelview/*model_matrix*/, mMatrixes[modelID][mInstances.instanceModelCounts[modelID]]);

		// set up the transformation matrix

		//R_RotateForEntity(ent, &tr.viewParms, &tr.ori);
		//Matrix16Multiply(tr.viewParms.projectionMatrix/*glState.projection*/, tr.ori.modelViewMatrix, mMatrixes[modelID][mInstances.instanceModelCounts[modelID]]);
		//Matrix16Copy(glState.modelviewProjection, mMatrixes[modelID][mInstances.instanceModelCounts[modelID]]);

		backEnd.currentEntity = ent;
		backEnd.refdef.floatTime = backEnd.currentEntity->e.shaderTime;

		// set up the transformation matrix
		R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori);
		Matrix16Copy(backEnd.ori.modelViewMatrix, glState.modelview);
		Matrix16Multiply(glState.projection, glState.modelview, mMatrixes[modelID][mInstances.instanceModelCounts[modelID]]);

		//ForceCrash();

		//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix, 1);
		//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);
		//Matrix16Multiply(glState.modelviewProjection, backEnd.ori.modelMatrix, mMatrixes[modelID][mInstances.instanceModelCounts[modelID]]);
#endif

		mInstances.instanceModelCounts[modelID]++;
	}
}

static int R_DistanceSortinstances(const void *a, const void *b)
{
	modelInstanceData_t	 *m1, *m2;

	m1 = (modelInstanceData_t *)a;
	m2 = (modelInstanceData_t *)b;


	// Distance sort...
	float dist1 = Distance(m1->mOrigin, tr.refdef.vieworg);
	float dist2 = Distance(m2->mOrigin, tr.refdef.vieworg);

	if (r_testvalue1->integer)
	{
		if (dist1 > dist2)
			return -1;

		else if (dist1 < dist2)
			return 1;
	}
	else
	{
		if (dist1 < dist2)
			return -1;

		else if (dist1 > dist2)
			return 1;
	}



	// Tree sort... Do trees first, because they have most chance of occluding...
	if (m1->mModel->isTree == 0)
	{
		shader_t *shader = tr.shaders[m1->mModel->vboSurfaces[0].mdvSurface->shaderIndexes[0]];

		if (StringContainsWord(shader->name, "tree"))
		{
			m1->mModel->isTree = 1;
		}
		else
		{
			m1->mModel->isTree = -1;
		}
	}

	if (m2->mModel->isTree == 0)
	{
		shader_t *shader = tr.shaders[m2->mModel->vboSurfaces[0].mdvSurface->shaderIndexes[0]];

		if (StringContainsWord(shader->name, "tree"))
		{
			m2->mModel->isTree = 1;
		}
		else
		{
			m2->mModel->isTree = -1;
		}
	}

	if (m1->mModel->isTree > m2->mModel->isTree)
		return -1;

	else if (m1->mModel->isTree < m2->mModel->isTree)
		return 1;
	


	// Num surfaces sort... Less surfaces first, hopefully occlude some extra pixels on lower state change models...
	if (m1->mModel->numSurfaces < m2->mModel->numSurfaces)
		return -1;

	else if (m1->mModel->numSurfaces > m2->mModel->numSurfaces)
		return 1;

	return 0;
}

void R_AddInstancedModelsToScene(void)
{
	if (mInstances.instanceModelTypes <= 0)
	{
		return;
	}

	//ForceCrash();

	GLSL_BindProgram(&tr.instanceShader);

	FBO_Bind(tr.renderFbo);

	SetViewportAndScissor();
	//GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);
	//GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);
	GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);

	GLSL_SetUniformMatrix16(&tr.instanceShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);
	GLSL_SetUniformVec3(&tr.instanceShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);
	GLSL_SetUniformFloat(&tr.instanceShader, UNIFORM_TIME, backEnd.refdef.floatTime);

	//GLSL_SetUniformInt(&tr.instanceShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	//GL_BindToTMU(tr.whiteImage, TB_DIFFUSEMAP);

	//GL_Cull(CT_TWO_SIDED);
	//GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS | GLS_ATEST_GT_0);

#define __INSTANCING_USE_UNIFORMS__

	/*if (r_testvalue0->integer)
	{// Sort by distances first...
		for (int modelID = 0; modelID < mInstances.instanceModelTypes && modelID < MAX_INSTANCED_MODEL_TYPES; modelID++)
		{
			if (mInstances.instanceModelCounts[modelID] > 0)
			{
				qsort(&mInstances.instanceModelInfos[modelID], mInstances.instanceModelCounts[modelID], sizeof(modelInstanceData_t), R_DistanceSortinstances);
			}
		}
	}*/

	// Draw them for this scene...
	for (int modelID = 0; modelID < mInstances.instanceModelTypes && modelID < MAX_INSTANCED_MODEL_TYPES; modelID++)
	{
		if (mInstances.instanceModelCounts[modelID] > 0)
		{
			mdvModel_t *m = mInstances.mModels[modelID];
			GLuint count = mInstances.instanceModelCounts[modelID];

			if (r_instancing->integer >= 2)
			{
				ri->Printf(PRINT_WARNING, "ModelID %i. Count %i.\n", modelID, count);
			}

#ifndef __INSTANCING_USE_UNIFORMS__
			if (m->vao == NULL)
			{
				ri->Printf(PRINT_WARNING, "Warning warning, fuckup in drawmodelinstanced - Model has no VAO!\n");
				continue;
			}

			qglBindVertexArray(m->vao);	// Select VAO
			qglEnableVertexAttribArray(m->vao);

			R_BindVBO(m->vboSurfaces->vbo);
			R_BindIBO(m->vboSurfaces->ibo);

			qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_buffer);
			qglBufferData(GL_ARRAY_BUFFER, count * sizeof(vec3_t), mOrigins[modelID], GL_STREAM_DRAW);
			qglEnableVertexAttribArray(ATTR_INDEX_INSTANCES_POSITION);
			qglVertexAttribPointer(ATTR_INDEX_INSTANCES_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
			qglVertexAttribDivisor(ATTR_INDEX_INSTANCES_POSITION, 1);

			qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_mvp);
			qglBufferData(GL_ARRAY_BUFFER, count * sizeof(matrix_t), mMatrixes[modelID], GL_STREAM_DRAW);
			qglEnableVertexAttribArray(ATTR_INDEX_INSTANCES_MVP);
			qglVertexAttribPointer(ATTR_INDEX_INSTANCES_MVP, 16, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0)/*BUFFER_OFFSET(m->ofs_instancesMVP)*/);
			qglVertexAttribDivisor(ATTR_INDEX_INSTANCES_MVP, 1);

			//ForceCrash();

			qglDrawElementsInstanced(GL_TRIANGLES, m->vboSurfaces->numIndexes, GL_INDEX_TYPE, 0, count);

			qglDisableVertexAttribArray(ATTR_INDEX_INSTANCES_POSITION);
			qglDisableVertexAttribArray(ATTR_INDEX_INSTANCES_MVP);

			qglDisableVertexAttribArray(m->vao);
#else //__INSTANCING_USE_UNIFORMS__
			for (int j = 0; j < m->numVBOSurfaces; j++)
			{
				R_BindVBO(m->vboSurfaces[j].vbo);
				R_BindIBO(m->vboSurfaces[j].ibo);

				shader_t *shader = tr.shaders[m->vboSurfaces[j].mdvSurface->shaderIndexes[0]];

				for (int stage = 0; stage <= shader->maxStage && stage < MAX_SHADER_STAGES; stage++)
				{
					shaderStage_t *pStage = shader->stages[stage];

					if (!pStage)
					{// How does this happen???
						break;
					}

					if (!pStage->active)
					{// Shouldn't this be here, just in case???
						continue;
					}

					uint32_t stateBits = pStage->stateBits;

					if (pStage->isFoliage)
					{
						stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GE_128;
						pStage->stateBits = stateBits;

						if (!shader->hasAlpha && !shader->hasGlow && !pStage->rgbGen && !pStage->alphaGen && shader->cullType != CT_BACK_SIDED)
						{
							GL_Cull(CT_FRONT_SIDED);
						}
						else if (!(backEnd.depthFill || (backEnd.viewParms.flags & VPF_SHADOWPASS)) && (stateBits & GLS_ATEST_BITS))
						{
							GL_Cull(CT_TWO_SIDED);
						}
					}
					else if (!shader->hasAlpha && !shader->hasGlow && !pStage->rgbGen && !pStage->alphaGen && shader->cullType != CT_BACK_SIDED)
					{
						GL_Cull(CT_FRONT_SIDED);
					}

					GLSL_VertexAttribsState(ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD0);
					GLSL_VertexAttribPointers(ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD0);

					if ((backEnd.depthFill || (backEnd.viewParms.flags & VPF_SHADOWPASS)) && !(stateBits & GLS_ATEST_BITS))
					{
						GLSL_SetUniformInt(&tr.instanceShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
						GL_BindToTMU(tr.whiteImage, TB_DIFFUSEMAP);
					}
					else
					{
						GLSL_SetUniformInt(&tr.instanceShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
						GL_BindToTMU(pStage->bundle[TB_DIFFUSEMAP].image[0], TB_DIFFUSEMAP);
					}

					vec3_t	mOrigins[MAX_INSTANCED_MODEL_INSTANCES] = { 0 };
					vec3_t	mScales[MAX_INSTANCED_MODEL_INSTANCES] = { 0 };

					for (int c = 0; c < mInstances.instanceModelCounts[modelID]; c++)
					{
						VectorCopy(mInstances.instanceModelInfos[modelID][c].mOrigin, mOrigins[c]);
						VectorCopy(mInstances.instanceModelInfos[modelID][c].mScale, mScales[c]);
					}

					GLSL_SetUniformVec3xX(&tr.instanceShader, UNIFORM_INSTANCE_POSITIONS, mOrigins, count);
					GLSL_SetUniformVec3xX(&tr.instanceShader, UNIFORM_INSTANCE_SCALES, mScales, count);
					//GLSL_SetUniformMatrix16(&tr.instanceShader, UNIFORM_INSTANCE_MATRIXES, (const float *)mInstances.instanceModelInfos[modelID].mMatrixes, count);

					vec4_t l0;
					VectorSet4(l0, shader->materialType, 0.0, 0.0, 0.0);
					GLSL_SetUniformVec4(&tr.instanceShader, UNIFORM_SETTINGS0, l0);
					//ForceCrash();

					//UpdateTexCoords(pStage);

					GL_State(stateBits);

					qglDrawElementsInstanced(GL_TRIANGLES, m->vboSurfaces[j].numIndexes, GL_INDEX_TYPE, 0, count);
				}
			}
#endif //__INSTANCING_USE_UNIFORMS__
		}
	}

	// Clear the buffer ready for next scene...
	for (int modelID = 0; modelID < mInstances.instanceModelTypes && modelID < MAX_INSTANCED_MODEL_TYPES; modelID++)
	{
		mInstances.instanceModelCounts[modelID] = 0;
		mInstances.mModels[modelID] = NULL;
	}

	mInstances.instanceModelTypes = 0;

	R_BindNullVBO();
}
#endif //__INSTANCED_MODELS__

#ifdef __LODMODEL_INSTANCING__
#define MAX_INSTANCED_LODMODEL_TYPES 64

typedef struct lodModelInstanceData_s
{
	//vec3_t			**mOrigins = NULL;
	//vec3_t			**mAngles = NULL;
	//vec3_t			**mScales = NULL;
	vec3_t			mOrigins[65536] = { NULL };
	vec3_t			mAngles[65536] = { NULL };
	vec3_t			mScales[65536] = { NULL };
} lodModelInstanceData_t;

typedef struct lodModelInstances_s
{
	int							instanceModelTypes = 0;
	mdvModel_t					*mModels[MAX_INSTANCED_LODMODEL_TYPES] = { NULL };
	int							counts[MAX_INSTANCED_LODMODEL_TYPES];
	lodModelInstanceData_t		infos[MAX_INSTANCED_LODMODEL_TYPES];
} lodModelInstances_t;

lodModelInstances_t				mLodModelInstances;

int R_HaveLodModelInstanceForModel(mdvModel_t *model)
{
	for (int i = 0; i < mLodModelInstances.instanceModelTypes; i++)
	{
		if (mLodModelInstances.mModels[i] == model)
			return i;
	}

	return -1;
}

void R_AddLodModelInstance(int id, vec3_t origin, vec3_t angles, vec3_t scale)
{
	//mLodModelInstances.infos[id].mOrigins[mLodModelInstances.counts[id]] = (vec3_t*)Hunk_Alloc(sizeof(vec3_t), h_low);
	//mLodModelInstances.infos[id].mAngles[mLodModelInstances.counts[id]] = (vec3_t*)Hunk_Alloc(sizeof(vec3_t), h_low);
	//mLodModelInstances.infos[id].mScales[mLodModelInstances.counts[id]] = (vec3_t*)Hunk_Alloc(sizeof(vec3_t), h_low);

	//VectorCopy(origin, *mLodModelInstances.infos[id].mOrigins[mLodModelInstances.counts[id]]);
	//VectorCopy(angles, *mLodModelInstances.infos[id].mAngles[mLodModelInstances.counts[id]]);
	//VectorCopy(scale, *mLodModelInstances.infos[id].mScales[mLodModelInstances.counts[id]]);

	VectorCopy(origin, mLodModelInstances.infos[id].mOrigins[mLodModelInstances.counts[id]]);
	VectorCopy(angles, mLodModelInstances.infos[id].mAngles[mLodModelInstances.counts[id]]);
	VectorCopy(scale, mLodModelInstances.infos[id].mScales[mLodModelInstances.counts[id]]);

	mLodModelInstances.counts[id]++;
}

void R_SetupLodModelArray(void)
{
	if (tr.lodModelsCount > 0)
	{
		if (mLodModelInstances.instanceModelTypes <= 0)
		{
			for (int i = 0; i < tr.lodModelsCount; i++)
			{
				lodModel_t *lodModel = &tr.lodModels[i];

				lodModel->qhandle = RE_RegisterModel(lodModel->modelName);
				lodModel->model = R_GetModelByHandle(lodModel->qhandle);

				model_t *model = lodModel->model;

				if (model->type == MOD_MESH) {
					mdvModel_t	*header = model->data.mdv[0];

					int lodModelInstance = R_HaveLodModelInstanceForModel(header);

					if (lodModelInstance >= 0)
					{
						R_AddLodModelInstance(lodModelInstance, lodModel->origin, lodModel->angles, lodModel->scale);
					}
					else
					{
						ri->Printf(PRINT_WARNING, "New lodmodel %s.\n", lodModel->modelName);

						mLodModelInstances.mModels[mLodModelInstances.instanceModelTypes] = header;
						
						int id = mLodModelInstances.instanceModelTypes;
						//mLodModelInstances.infos[id].mOrigins = (vec3_t**)Hunk_Alloc(sizeof(vec3_t*), h_low);
						//mLodModelInstances.infos[id].mAngles = (vec3_t**)Hunk_Alloc(sizeof(vec3_t*), h_low);
						//mLodModelInstances.infos[id].mScales = (vec3_t**)Hunk_Alloc(sizeof(vec3_t*), h_low);
						
						R_AddLodModelInstance(mLodModelInstances.instanceModelTypes, lodModel->origin, lodModel->angles, lodModel->scale);
						mLodModelInstances.instanceModelTypes++;
					}
				}
			}

			ri->Printf(PRINT_WARNING, "There are now %i vao model types\n", mLodModelInstances.instanceModelTypes);
		}
	}
}

void R_AddInstancedLodModelsToScene(void)
{
	if (tr.lodModelsCount <= 0)
	{
		return;
	}

	R_SetupLodModelArray();

	GLSL_BindProgram(&tr.instanceVAOShader);

	FBO_Bind(tr.renderFbo);

	SetViewportAndScissor();
	//GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);
	//GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);
	GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);

	GLSL_SetUniformMatrix16(&tr.instanceVAOShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);
	GLSL_SetUniformVec3(&tr.instanceVAOShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);
	GLSL_SetUniformFloat(&tr.instanceVAOShader, UNIFORM_TIME, backEnd.refdef.floatTime);

	// Draw them for this scene...
	for (int modelID = 0; modelID < mLodModelInstances.instanceModelTypes && modelID < mLodModelInstances.counts[modelID] && modelID < MAX_INSTANCED_LODMODEL_TYPES; modelID++)
	{
		
		mdvModel_t *m = mLodModelInstances.mModels[modelID];
		GLuint count = mLodModelInstances.counts[modelID];

		if (r_instancing->integer >= 2)
		{
			ri->Printf(PRINT_WARNING, "ModelID %i. Count %i.\n", modelID, count);
		}

		if (m->vao == NULL)
		{
			ri->Printf(PRINT_WARNING, "Warning warning, fuckup in drawmodelinstanced - Model has no VAO!\n");
			continue;
		}

		qglBindVertexArray(m->vao);	// Select VAO
		qglEnableVertexAttribArray(m->vao);

		for (int j = 0; j < m->numVBOSurfaces; j++)
		{
			R_BindVBO(m->vboSurfaces[j].vbo);
			R_BindIBO(m->vboSurfaces[j].ibo);

			shader_t *shader = tr.shaders[m->vboSurfaces[j].mdvSurface->shaderIndexes[0]];

			for (int stage = 0; stage <= shader->maxStage && stage < MAX_SHADER_STAGES; stage++)
			{
				shaderStage_t *pStage = shader->stages[stage];

				if (!pStage)
				{// How does this happen???
					break;
				}

				if (!pStage->active)
				{// Shouldn't this be here, just in case???
					continue;
				}

				uint32_t stateBits = pStage->stateBits;

				if (pStage->isFoliage)
				{
					stateBits = GLS_DEPTHMASK_TRUE | GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_ATEST_GE_128;
					pStage->stateBits = stateBits;

					if (!shader->hasAlpha && !shader->hasGlow && !pStage->rgbGen && !pStage->alphaGen && shader->cullType != CT_BACK_SIDED)
					{
						GL_Cull(CT_FRONT_SIDED);
					}
					else if (!(backEnd.depthFill || (backEnd.viewParms.flags & VPF_SHADOWPASS)) && (stateBits & GLS_ATEST_BITS))
					{
						GL_Cull(CT_TWO_SIDED);
					}
				}
				else if (!shader->hasAlpha && !shader->hasGlow && !pStage->rgbGen && !pStage->alphaGen && shader->cullType != CT_BACK_SIDED)
				{
					GL_Cull(CT_FRONT_SIDED);
				}

				GLSL_VertexAttribsState(ATTR_INSTANCES_POSITION | ATTR_NORMAL | ATTR_TEXCOORD0);
				GLSL_VertexAttribPointers(ATTR_INSTANCES_POSITION | ATTR_NORMAL | ATTR_TEXCOORD0);

				if ((backEnd.depthFill || (backEnd.viewParms.flags & VPF_SHADOWPASS)) && !(stateBits & GLS_ATEST_BITS))
				{
					GLSL_SetUniformInt(&tr.instanceVAOShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
					GL_BindToTMU(tr.whiteImage, TB_DIFFUSEMAP);
				}
				else
				{
					GLSL_SetUniformInt(&tr.instanceVAOShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
					GL_BindToTMU(pStage->bundle[TB_DIFFUSEMAP].image[0], TB_DIFFUSEMAP);
				}

				qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceVAOShader.instances_buffer);
				qglBufferData(GL_ARRAY_BUFFER, count * sizeof(vec3_t), mLodModelInstances.infos[modelID].mOrigins, GL_DYNAMIC_DRAW/*GL_STREAM_DRAW*/);
				//qglBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(vec3_t), mLodModelInstances.infos[modelID].mOrigins);
				qglEnableVertexAttribArray(ATTR_INDEX_INSTANCES_POSITION);
				qglVertexAttribPointer(ATTR_INDEX_INSTANCES_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), BUFFER_OFFSET(m->vboSurfaces[j].vbo->ofs_instancesPosition));
				qglVertexAttribDivisor(ATTR_INDEX_INSTANCES_POSITION, 1);

				/*qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceVAOShader.instances_mvp);
				qglBufferData(GL_ARRAY_BUFFER, count * sizeof(matrix_t), mMatrixes[modelID], GL_STREAM_DRAW);
				qglEnableVertexAttribArray(ATTR_INDEX_INSTANCES_MVP);
				qglVertexAttribPointer(ATTR_INDEX_INSTANCES_MVP, 16, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
				qglVertexAttribDivisor(ATTR_INDEX_INSTANCES_MVP, 1);*/

				vec4_t l0;
				VectorSet4(l0, shader->materialType, 0.0, 0.0, 0.0);
				GLSL_SetUniformVec4(&tr.instanceVAOShader, UNIFORM_SETTINGS0, l0);
				//ForceCrash();

				//UpdateTexCoords(pStage);

				GL_State(stateBits);

				qglDrawElementsInstanced(GL_TRIANGLES, m->vboSurfaces[j].numIndexes, GL_INDEX_TYPE, 0, count);
			}
		}

		qglDisableVertexAttribArray(ATTR_INDEX_INSTANCES_POSITION);
		qglDisableVertexAttribArray(ATTR_INDEX_INSTANCES_MVP);

		qglDisableVertexAttribArray(m->vao);
	}

	R_BindNullVBO();
}
#endif //__LODMODEL_INSTANCING__