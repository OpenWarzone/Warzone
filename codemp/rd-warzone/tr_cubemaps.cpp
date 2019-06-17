#include "tr_local.h"

extern void R_RotateForViewer(viewParms_t *viewParms);
extern void R_SetupProjectionZ(viewParms_t *dest);
extern void R_SortDrawSurfs(drawSurf_t *drawSurfs, int numDrawSurfs);

void R_AddModelSurface(model_t *model)
{
	int lod = 0;

	mdvModel_t *mdv = model->data.mdv[lod];

	//
	// draw all surfaces
	//
	mdvSurface_t *surface = mdv->surfaces;

	for (int i = 0; i < mdv->numSurfaces; i++) 
	{
		shader_t *shader = tr.shaders[surface->shaderIndexes[i]];
		
		srfVBOMDVMesh_t *vboSurface = &mdv->vboSurfaces[i];
		R_AddDrawSurf((surfaceType_t *)vboSurface, shader, 0, qfalse, qfalse, 0, qfalse);

		surface++;
	}
}

void R_RenderCubeView(viewParms_t *parms, model_t *model) {
	int		firstDrawSurf;

	if (parms->viewportWidth <= 0 || parms->viewportHeight <= 0) {
		return;
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer(&tr.viewParms);

	R_SetupProjection(&tr.viewParms, r_zproj->value, tr.viewParms.zFar, qtrue);

	// we know the size of the clipping volume. Now set the rest of the projection matrix.
	R_SetupProjectionZ(&tr.viewParms);

	R_AddModelSurface(model);

	R_SortDrawSurfs(tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf);
}

void R_RenderCubeSide(int cubemapSide, model_t *model, image_t *cubeModelImage)
{
	refdef_t				refdef;
	viewParms_t				parms;
	vec3_t					modelBounds[2];
	float oldColorScale =	tr.refdef.colorScale;

	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = 0;

	// We will need the model's bounds, so that we can render from outside the model, looking toward it...
	VectorCopy(model->data.mdv[0]->frames[0].bounds[0], modelBounds[0]);
	VectorCopy(model->data.mdv[0]->frames[0].bounds[1], modelBounds[1]);

	VectorSet(refdef.vieworg, 0.0, 0.0, 0.0); // need to use modelBounds to calculate the view position for each side... how? I suck at this math stuff... axii give me a headache...

	switch (cubemapSide)
	{// UQ1: I inverted these already, so we render inwards...
	case 0:
		// -X
		VectorSet(refdef.viewaxis[0], 1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, 1);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 1:
		// +X
		VectorSet(refdef.viewaxis[0], -1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, -1);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 2:
		// -Y
		VectorSet(refdef.viewaxis[0], 0, 1, 0);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, 1);
		break;
	case 3:
		// +Y
		VectorSet(refdef.viewaxis[0], 0, -1, 0);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, -1);
		break;
	case 4:
		// -Z
		VectorSet(refdef.viewaxis[0], 0, 0, 1);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 5:
		// +Z
		VectorSet(refdef.viewaxis[0], 0, 0, -1);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	}

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.x = 0;
	refdef.y = 0;
	refdef.width = tr.renderCubeFbo->width;
	refdef.height = tr.renderCubeFbo->height;

	refdef.time = 0;

	RE_BeginScene(&refdef);

	tr.refdef.colorScale = 1.0f;

	Com_Memset(&parms, 0, sizeof(parms));

	parms.viewportX = 0;
	parms.viewportY = 0;
	parms.viewportWidth = tr.renderCubeFbo->width;
	parms.viewportHeight = tr.renderCubeFbo->height;
	parms.isPortal = qfalse;
	parms.isMirror = qtrue;
	parms.flags = VPF_NOVIEWMODEL | VPF_NOCUBEMAPS | VPF_NOPOSTPROCESS | VPF_CUBEMAP | VPF_RENDERCUBE;

	parms.fovX = 90;
	parms.fovY = 90;

	VectorCopy(refdef.vieworg, parms.ori.origin);
	VectorCopy(refdef.viewaxis[0], parms.ori.axis[0]);
	VectorCopy(refdef.viewaxis[1], parms.ori.axis[1]);
	VectorCopy(refdef.viewaxis[2], parms.ori.axis[2]);

	VectorCopy(refdef.vieworg, parms.pvsOrigin);

	parms.targetFbo = tr.renderCubeFbo;
	parms.targetFboLayer = cubemapSide;
	parms.targetFboCubemapIndex = -1;
	parms.targetCubemapImage = cubeModelImage;

	R_RenderCubeView(&parms, model);

	RE_EndScene();
}

//
// Generate a cubemap for a specified (md3 or assimp) model... For UI usage, and maybe later, lods...
//
image_t *R_GenerateCubemapForModel(model_t *model, int cubemapSize)
{
	GLenum cubemapFormat = GL_RGBA8;//GL_RGBA16F;

	ri->Printf(PRINT_WARNING, "Rendering cubemap image for model %s.\n", model->name);

	image_t *cubeModelImage = R_CreateImage("*modelCubemap", NULL, cubemapSize, cubemapSize, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat);
	
	for (int j = 0; j < 6; j++)
	{
		RE_ClearScene();
		R_RenderCubeSide(j, model, cubeModelImage);
		R_IssuePendingRenderCommands();
		R_InitNextFrame();
	}

	return cubeModelImage;
}
