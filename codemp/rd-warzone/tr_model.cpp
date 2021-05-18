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
// tr_models.c -- model loading and caching

#include "tr_local.h"
#include <qcommon/sstring.h>

#define	LL(x) x=LittleLong(x)


static qboolean R_LoadMD3(model_t *mod, int lod, void *buffer, const char *modName);
static qboolean R_LoadMDR(model_t *mod, void *buffer, int filesize, const char *name );

/*
====================
R_RegisterMD3
====================
*/
qhandle_t R_RegisterMD3(const char *name, model_t *mod)
{
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded = qfalse;
	int			numLoaded;
	char filename[MAX_QPATH], namebuf[MAX_QPATH+20];
	char *fext, defex[] = "md3";

	numLoaded = 0;

	strcpy(filename, name);

	fext = strchr(filename, '.');
	if(!fext)
		fext = defex;
	else
	{
		*fext = '\0';
		fext++;
	}

	for (lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod--)
	{
		if(lod)
			Com_sprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		else
			Com_sprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);

		qboolean bAlreadyCached = qfalse;
		if( !CModelCache->LoadFile( namebuf, (void**)&buf, &bAlreadyCached ) )
			continue;

		ident = *(unsigned *)buf;
		if( !bAlreadyCached )
			ident = LittleLong(ident);
		
		switch(ident)
		{
			case MD3_IDENT:
				loaded = R_LoadMD3(mod, lod, buf, namebuf);
				break;
			case MDXA_IDENT:
				loaded = R_LoadMDXA(mod, buf, namebuf, bAlreadyCached);
				break;
			case MDXM_IDENT:
				loaded = R_LoadMDXM(mod, buf, name, bAlreadyCached);
				break;
			default:
				ri->Printf(PRINT_WARNING, "R_RegisterMD3: unknown ident for %s\n", name);
				break;
		}

		if(loaded)
		{
			mod->numLods++;
			numLoaded++;
		}
		else
			break;
	}

	if(numLoaded)
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for(lod--; lod >= 0; lod--)
		{
			mod->numLods++;
			mod->data.mdv[lod] = mod->data.mdv[lod + 1];
		}

		return mod->index;
	}

#ifdef _DEBUG
	ri->Printf(PRINT_WARNING,"R_RegisterMD3: couldn't load %s\n", name);
#endif

	mod->type = MOD_BAD;
	return 0;
}


#define __REGEN_MODEL_NORMALS__

#ifdef __REGEN_MODEL_NORMALS__
void GenerateNormalsForMesh(mdvSurface_t *cv)
{
	for (int i = 0; i < cv->numIndexes; i += 3)
	{
		int tri[3];
		tri[0] = cv->indexes[i];
		tri[1] = cv->indexes[i + 1];
		tri[2] = cv->indexes[i + 2];

		if (tri[0] >= cv->numVerts)
		{
			//ri->Printf(PRINT_WARNING, "Shitty BSP index data - Bad index in face surface (vert) %i >=  (maxVerts) %i.", tri[0], cv->numVerts);
			return;
		}
		if (tri[1] >= cv->numVerts)
		{
			//ri->Printf(PRINT_WARNING, "Shitty BSP index data - Bad index in face surface (vert) %i >=  (maxVerts) %i.", tri[1], cv->numVerts);
			return;
		}
		if (tri[2] >= cv->numVerts)
		{
			//ri->Printf(PRINT_WARNING, "Shitty BSP index data - Bad index in face surface (vert) %i >=  (maxVerts) %i.", tri[2], cv->numVerts);
			return;
		}

		float* a = (float *)cv->verts[tri[0]].xyz;
		float* b = (float *)cv->verts[tri[1]].xyz;
		float* c = (float *)cv->verts[tri[2]].xyz;
		vec3_t ba, ca;
		VectorSubtract(b, a, ba);
		VectorSubtract(c, a, ca);

		vec3_t normal;
		CrossProduct(ca, ba, normal);
		VectorNormalize(normal);

		//ri->Printf(PRINT_WARNING, "OLD: %f %f %f. NEW: %f %f %f.\n", cv->verts[tri[0]].normal[0], cv->verts[tri[0]].normal[1], cv->verts[tri[0]].normal[2], normal[0], normal[1], normal[2]);

#pragma omp critical
		{
			VectorCopy(normal, cv->verts[tri[0]].normal);
			VectorCopy(normal, cv->verts[tri[1]].normal);
			VectorCopy(normal, cv->verts[tri[2]].normal);
		}
	}
}

extern bool ValidForSmoothing(vec3_t v1, vec3_t n1, vec3_t v2, vec3_t n2);

void GenerateSmoothNormalsForSurface(mdvSurface_t *cv)
{
	int numVerts = cv->numVerts;
	int numIndexes = cv->numIndexes;

	mdvVertex_t *verts = cv->verts;

#define __MULTIPASS_SMOOTHING__ // Looks better, but takes a lot longer...

#ifdef __MULTIPASS_SMOOTHING__
	for (int z = 0; z < 3; z++)
#endif //__MULTIPASS_SMOOTHING__
	{
		//#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < numVerts; i++)
		{
			//ri->Printf(PRINT_WARNING, "%i of %i.\n", i, numVerts);

			qboolean updated = qfalse;
			vec3_t finalNormal;

			VectorCopy(verts[i].normal, finalNormal);

			for (int j = 0; j < numVerts; j++)
			{
				if (j == i) continue;

				vec3_t n2;

				//if (Distance(verts[i].position, verts[j].position) > 0.0) continue;
				if (!VectorCompare(verts[i].xyz, verts[j].xyz)) continue;

				if (ValidForSmoothing(verts[i].xyz, verts[i].normal, verts[j].xyz, verts[j].normal))
				{
					VectorAdd(finalNormal, n2, finalNormal);
#ifndef __MULTIPASS_SMOOTHING__
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
#endif //__MULTIPASS_SMOOTHING__

					updated = qtrue;
				}
			}

			if (updated)
			{
				VectorNormalize(finalNormal);

#pragma omp critical (__SET_SMOOTH_NORMAL__)
				{
					VectorCopy(finalNormal, verts[i].normal);
				}
			}
		}
	}
}

void GenerateEnhancedNormalsForSurface(mdvSurface_t *cv)
{
	GenerateNormalsForMesh(cv);
	GenerateSmoothNormalsForSurface(cv);
}
#endif //__REGEN_MODEL_NORMALS__

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

// Create an instance of the Importer class
Assimp::Importer assImpImporter;

std::string AssImp_getBasePath(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(0, pos + 1);
}

std::string AssImp_getTextureName(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(pos + 1, std::string::npos);
}

char modelTexName[MAX_IMAGE_PATH] = { 0 }; // not thread safe, but we are not threading anywhere here...

char *FS_TextureFileExists(const char *name)
{
	if (!name || !name[0] || name[0] == '\0' || strlen(name) <= 1) return NULL;

	char strippedTexName[MAX_IMAGE_PATH] = { 0 };
	memset(&strippedTexName, 0, sizeof(char) * MAX_IMAGE_PATH);
	COM_StripExtension(name, strippedTexName, MAX_IMAGE_PATH);

	sprintf(modelTexName, "%s.png", strippedTexName);

	if (ri->FS_FileExists(modelTexName))
	{
		return modelTexName;
	}

	sprintf(modelTexName, "%s.tga", strippedTexName);

	if (ri->FS_FileExists(modelTexName))
	{
		return modelTexName;
	}

	sprintf(modelTexName, "%s.jpg", strippedTexName);

	if (ri->FS_FileExists(modelTexName))
	{
		return modelTexName;
	}

	sprintf(modelTexName, "%s.dds", strippedTexName);

	if (ri->FS_FileExists(modelTexName))
	{
		return modelTexName;
	}

	sprintf(modelTexName, "%s.gif", strippedTexName);

	if (ri->FS_FileExists(modelTexName))
	{
		return modelTexName;
	}

	sprintf(modelTexName, "%s.bmp", strippedTexName);

	if (ri->FS_FileExists(modelTexName))
	{
		return modelTexName;
	}

	sprintf(modelTexName, "%s.ico", strippedTexName);

	if (ri->FS_FileExists(modelTexName))
	{
		return modelTexName;
	}

	return NULL;
}

std::string R_FindAndAdjustShaderNames(std::string modelName, std::string surfaceName, std::string shaderPath)
{// See if we can find any missing textures, by also looking for them in the model's directory, when they are not found... Return original path if nothing is found...
	qboolean	foundName = qfalse;

	if (shaderPath.length() > 0 && ri->FS_FileExists(shaderPath.c_str()))
	{// Original file+path exists... Use it...
		foundName = qtrue;
	}

	if (!foundName && shaderPath.length() > 0 && FS_TextureFileExists(shaderPath.c_str()))
	{// Original file+path exists... Use it...
		char *nExt = FS_TextureFileExists(shaderPath.c_str());
		shaderPath = nExt;
		foundName = qtrue;
	}

	// Try to find the file in the original model's directory...
	if (!foundName && shaderPath.length() > 0)
	{
		std::string basePath = AssImp_getBasePath(modelName);

		char out[MAX_IMAGE_PATH] = { 0 };
		COM_StripExtension(shaderPath.c_str(), out, sizeof(out));
		std::string textureName = out;

		char shaderRealPath[MAX_IMAGE_PATH] = { 0 };
		sprintf(shaderRealPath, "%s%s", basePath.c_str(), textureName.c_str());

		char *nExt = FS_TextureFileExists(shaderRealPath);

		if (nExt && nExt[0])
		{
			shaderPath = shaderRealPath;
			foundName = qtrue;
		}
	}

	// Try looking for the surface name as a texture, if we were provided a name...
	if (!foundName && surfaceName.length() > 0)
	{
		if (!foundName && surfaceName.length() > 0 && FS_TextureFileExists(surfaceName.c_str()))
		{// See if we can see a file at the surfaceName location (trying all possible extensions)...
			char *nExt = FS_TextureFileExists(surfaceName.c_str());

			if (nExt && nExt[0])
			{
				shaderPath = surfaceName; // we don't actually need/want the extension.
				foundName = qtrue;
			}
			foundName = qtrue;
		}

		if (!foundName && surfaceName.length() > 0)
		{// Try to find the file in the original model path...
			std::string basePath = AssImp_getBasePath(modelName);

			char out[MAX_IMAGE_PATH] = { 0 };
			COM_StripExtension(surfaceName.c_str(), out, sizeof(out));
			std::string textureName = out;

			char shaderRealPath[MAX_IMAGE_PATH] = { 0 };
			sprintf(shaderRealPath, "%s%s", basePath.c_str(), textureName.c_str());

			char *nExt = FS_TextureFileExists(shaderRealPath);

			if (nExt && nExt[0])
			{
				shaderPath = shaderRealPath; // we don't actually need/want the extension.
				foundName = qtrue;
			}
		}
	}

	return shaderPath;
}

//#define __MODEL_MESH_MERGE__

static qboolean R_LoadAssImp(model_t * mod, int lod, void *buffer, const char *modName, int size, const char *ext)
{
	int					i, j;

	mdvModel_t			*mdvModel;
	mdvFrame_t			*frame;
	mdvSurface_t		*surf;
	int					*shaderIndex;
	glIndex_t			*tri;
	mdvVertex_t			*v;
	mdvSt_t				*st;

	std::string basePath = AssImp_getBasePath(modName);

#ifdef __DEBUG_ASSIMP__
	ri->Printf(PRINT_ERROR, "R_LoadAssImp: Loading model %s. Extension: %s. bufferSize: %i. basePath: %s.\n", modName, ext, size, basePath.c_str());
#endif //__DEBUG_ASSIMP__

#define ASSIMP_MODEL_SCALE 64.0

	//assImpImporter.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
	//assImpImporter.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, (ai_real)45.0f);

#if 0
#define aiProcessPreset_TargetRealtime_MaxQuality_Fix ( \
	aiProcessPreset_TargetRealtime_Quality	|  \
	aiProcess_FlipWindingOrder			|  \
    aiProcess_FixInfacingNormals			|  \
    aiProcess_FindInstances                  |  \
    aiProcess_ValidateDataStructure          |  \
	aiProcess_OptimizeMeshes                 |  \
    0 )

	//aiProcess_OptimizeGraph                  |  \

#elif 0
#define aiProcessPreset_TargetRealtime_MaxQuality_Fix ( \
	aiProcess_OptimizeMeshes				|  \
    aiProcess_JoinIdenticalVertices         |  \
    aiProcess_ImproveCacheLocality          |  \
    aiProcess_RemoveRedundantMaterials      |  \
    aiProcess_Triangulate                   |  \
    aiProcess_GenUVCoords                   |  \
    aiProcess_SortByPType                   |  \
    aiProcess_FindDegenerates               |  \
    aiProcess_FindInvalidData               |  \
	aiProcess_FlipWindingOrder				|  \
	aiProcess_RemoveRedundantMaterials		|  \
    aiProcess_ValidateDataStructure			|  \
	0 )
#elif 0
#define aiProcessPreset_TargetRealtime_MaxQuality_Fix ( \
	aiProcess_OptimizeMeshes				|  \
	aiProcess_GenSmoothNormals              |  \
    aiProcess_JoinIdenticalVertices         |  \
    aiProcess_ImproveCacheLocality          |  \
    aiProcess_RemoveRedundantMaterials      |  \
    aiProcess_Triangulate                   |  \
    aiProcess_GenUVCoords                   |  \
    aiProcess_SortByPType                   |  \
    aiProcess_FindDegenerates               |  \
    aiProcess_FindInvalidData               |  \
	aiProcess_FlipWindingOrder				|  \
    aiProcess_ValidateDataStructure			|  \
	0 )

	//    aiProcess_SplitLargeMeshes              |  \
	//    aiProcess_FindInstances					|  \

	//   aiProcess_GenNormals

	// >#AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE
#else
#define aiProcessPreset_TargetRealtime_MaxQuality_Fix ( \
    aiProcess_ImproveCacheLocality          |  \
    aiProcess_FindInvalidData               |  \
    aiProcess_ValidateDataStructure			|  \
    aiProcess_Triangulate                   |  \
	aiProcess_ForceGenNormals               |  \
	aiProcess_GenSmoothNormals              |  \
	aiProcess_OptimizeMeshes                |  \
	aiProcess_OptimizeGraph                 |  \
	aiProcess_RemoveRedundantMaterials      |  \
	aiProcess_JoinIdenticalVertices         |  \
	aiProcess_FlipWindingOrder              |  \
	aiProcess_GenUVCoords                   |  \
	0 )



#define __INVERT_ASSIMP_NORMALS__
#endif

	const aiScene* scene = assImpImporter.ReadFileFromMemory(buffer, size, aiProcessPreset_TargetRealtime_MaxQuality_Fix, ext);
	
	if (!scene)
	{
		ri->Printf(PRINT_ERROR, "R_LoadAssImp: %s could not load. Error: %s\n", modName, assImpImporter.GetErrorString());
		return qfalse;
	}

#ifdef __EXPERIMENTAL_ASSIMP_GLM_CONVERSIONS__
	if (scene->HasAnimations() || StringContainsWord(modName, "players/"))
	{// If it has bones or is in players/ dir, dump it into the GLM converter...
		/*if (StringContainsWord(modName, "players/"))
		{
			extern qboolean R_LoadMDXM_Assimp(model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached, const aiScene* scene, int size);
			qboolean AlreadyCached = qfalse;
			return R_LoadMDXM_Assimp(mod, buffer, modName, AlreadyCached, scene, size);
		}*/

		// Check for bones...
		qboolean hasBones = qfalse;

		for (int j = 0; j < scene->mNumMeshes; j++)
		{
			if (scene->mMeshes[j]->HasBones())
			{
				hasBones = qtrue;
				break;
			}
		}

		if (hasBones)
		{
			extern qboolean R_LoadMDXM_Assimp(model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached, const aiScene* scene, int size);
			qboolean AlreadyCached = qfalse;
			return R_LoadMDXM_Assimp(mod, buffer, modName, AlreadyCached, scene, size);
		}
	}
#endif //__EXPERIMENTAL_ASSIMP_GLM_CONVERSIONS__

	mod->type = MOD_MESH;
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	mdvModel = mod->data.mdv[lod] = (mdvModel_t *)CModelCache->Allocate(size, buffer, modName, &bAlreadyFound, TAG_MODEL_MD3);

	if (bAlreadyFound)
	{
		CModelCache->AllocateShaders(modName);
	}

	// Calculate the bounds/radius info...
	vec3_t bounds[2];
	ClearBounds(bounds[0], bounds[1]);

	for (i = 0; i < scene->mNumMeshes; i++)
	{
		for (j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
		{
			vec3_t origin;
			VectorSet(origin, scene->mMeshes[i]->mVertices[j].x * MD3_XYZ_SCALE * ASSIMP_MODEL_SCALE, scene->mMeshes[i]->mVertices[j].y * MD3_XYZ_SCALE * ASSIMP_MODEL_SCALE, scene->mMeshes[i]->mVertices[j].z * MD3_XYZ_SCALE * ASSIMP_MODEL_SCALE);
			AddPointToBounds(origin, bounds[0], bounds[1]);
		}
	}

#ifdef __DEBUG_ASSIMP__
	ri->Printf(PRINT_ALL, "Model %s. Bounds %f %f %f x %f %f %f.\n", modName, bounds[0][0], bounds[0][1], bounds[0][2], bounds[1][0], bounds[1][1], bounds[1][2]);
#endif //__DEBUG_ASSIMP__

	// swap all the frames - Not supported for now, just using a single empty frame with basic data...
	mdvModel->numFrames = 1;
	mdvModel->frames = frame = (mdvFrame_t *)ri->Hunk_Alloc(sizeof(*frame) * mdvModel->numFrames, h_low);

	vec3_t localOrigin;
	VectorSet(localOrigin, 0.0, 0.0, 0.0);
	float radius = Distance(bounds[0], bounds[1]) / 2.0;

	for (i = 0; i < mdvModel->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(radius);
		for (j = 0; j < 3; j++)
		{
			frame->bounds[0][j] = LittleFloat(bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(localOrigin[j]);
		}
	}

	// swap all the tags - Disabled for now... Not sure if I need tags on non-md3 models...
	mdvModel->numTags = 0;
	/*mdvModel->tags = tag = (mdvTag_t *)ri->Hunk_Alloc(sizeof(*tag), h_low);

	md3Tag = (md3Tag_t *)ri->Hunk_Alloc(sizeof(md3Tag_t), h_low);

	for (i = 0; i < md3Model->numTags * md3Model->numFrames; i++, tag++, md3Tag++)
	{
		for (j = 0; j < 3; j++)
		{
			tag->origin[j] = LittleFloat(md3Tag->origin[j]);
			tag->axis[0][j] = LittleFloat(md3Tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(md3Tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(md3Tag->axis[2][j]);
		}
	}


	mdvModel->tagNames = tagName = (mdvTagName_t *)ri->Hunk_Alloc(sizeof(*tagName) * (md3Model->numTags), h_low);

	md3Tag = (md3Tag_t *)((byte *)md3Model + md3Model->ofsTags);
	for (i = 0; i < md3Model->numTags; i++, tagName++, md3Tag++)
	{
		Q_strncpyz(tagName->name, md3Tag->name, sizeof(tagName->name));
	}*/

#ifdef __MODEL_MESH_MERGE__
	int numTextureNames = scene->mNumMeshes;
	char textureNames[16][512];
	int totalIndexes = 0;
	int totalVerts = 0;
	int indexesStart[16];

	memset(textureNames, 0, sizeof(textureNames));
	memset(indexesStart, 0, sizeof(indexesStart));

	/*if (numTextureNames > 16)
	{
		ri->Printf(PRINT_WARNING, "**************************************** model %s has too many textures (%i) to make an alias.\n", modName, numTextureNames);
		return qfalse;
	}
	else
	{
		ri->Printf(PRINT_WARNING, "**************************************** model %s adding %i textures to alias.\n", modName, numTextureNames);
	}*/

	// Find all the texture names, so we can generate a texture alias map.
	for (i = 0; i < scene->mNumMeshes; i++)
	{
		shader_t		*sh = NULL;
		aiString		shaderPath;	// filename
		aiMesh			*aiSurf = scene->mMeshes[i];

		scene->mMaterials[aiSurf->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &shaderPath);

		std::string textureName = AssImp_getTextureName(shaderPath.C_Str());

		std::string finalPath = R_FindAndAdjustShaderNames(modName, shaderPath.C_Str(), textureName);

		sh = R_FindShader(finalPath.c_str(), lightmapsNone, stylesDefault, qtrue);

		if (sh == NULL || sh == tr.defaultShader)
		{
			//ri->Printf(PRINT_ALL, "mesh %i. shader %s. numFaces %i. numVerts %i. totalIndexes %i. totalVerts %i. No name.\n", i, textureNames[i], aiSurf->mNumFaces, aiSurf->mNumVertices, totalIndexes, totalVerts);
		}
		else
		{
			strcpy(textureNames[i], finalPath.c_str());

			indexesStart[i] = totalIndexes;
			totalIndexes += aiSurf->mNumFaces;
			totalVerts += aiSurf->mNumVertices;
			//ri->Printf(PRINT_ALL, "mesh %i. shader %s. numFaces %i. numVerts %i. totalIndexes %i. totalVerts %i.\n", i, textureNames[i], aiSurf->mNumFaces, aiSurf->mNumVertices, totalIndexes, totalVerts);
		}
	}

	image_t		*textureAliasMap = R_BakeTextures(textureNames, numTextureNames, modName, IMGTYPE_COLORALPHA, IMGFLAG_NONE, qfalse);
	shader_t	*textureAliasMapShader = R_FindShader(modName, lightmapsNone, stylesDefault, qtrue);

	// swap all the surfaces
	mdvModel->numSurfaces = 1;
	mdvModel->surfaces = surf = (mdvSurface_t *)ri->Hunk_Alloc(sizeof(*surf), h_low);

	// change to surface identifier
	surf->surfaceType = SF_MDV;

	// give pointer to model for Tess_SurfaceMDX
	surf->model = mdvModel;

	// copy surface name
	Q_strncpyz(surf->name, modName, sizeof(surf->name) < 64 ? sizeof(surf->name) : 64);

	// lowercase the surface name so skin compares are faster
	Q_strlwr(surf->name);

	// strip off a trailing _1 or _2
	// this is a crutch for q3data being a mess
	j = strlen(surf->name);
	if (j > 2 && surf->name[j - 2] == '_')
	{
		surf->name[j - 2] = 0;
	}

	// register the shaders
	surf->numShaderIndexes = 1;
	surf->shaderIndexes = shaderIndex = (int *)ri->Hunk_Alloc(sizeof(*shaderIndex), h_low);

	*shaderIndex = textureAliasMapShader->index;
	
	surf->numIndexes = totalIndexes * 3;
	surf->indexes = tri = (glIndex_t *)ri->Hunk_Alloc(sizeof(*tri) * totalIndexes * 3, h_low);

	surf->numVerts = totalVerts;
	surf->verts = v = (mdvVertex_t *)ri->Hunk_Alloc(sizeof(*v) * totalVerts * mdvModel->numFrames, h_low);

	surf->st = st = (mdvSt_t *)ri->Hunk_Alloc(sizeof(*st) * totalVerts * mdvModel->numFrames, h_low);

	for (i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh			*aiSurf = scene->mMeshes[i];

		if (!textureNames[i] || textureNames[i][0] == 0 || strlen(textureNames[i]) <= 0)
		{
			continue;
		}

		// swap all the Xyz, Normals, st
		for (j = 0; j < aiSurf->mNumVertices; j++)
		{
			aiVector3D xyz = aiSurf->mVertices[j];

			v->xyz[0] = LittleShort(xyz.x);
			v->xyz[1] = LittleShort(xyz.y);
			v->xyz[2] = LittleShort(xyz.z);

			aiVector3D norm = aiSurf->mNormals[j];

			v->normal[0] = norm.x;
			v->normal[1] = norm.y;
			v->normal[2] = norm.z;

			extern void R_GetBakedOffset(int textureNum, int numTextures, vec2_t *finalOffsetStart, vec2_t *finalOffsetEnd);

			if (aiSurf->mNormals != NULL && aiSurf->HasTextureCoords(0))
			{
				st->st[0] = LittleFloat(aiSurf->mTextureCoords[0][j].x);
				st->st[1] = LittleFloat(1 - aiSurf->mTextureCoords[0][j].y);
			}
			else
			{
				st->st[0] = 0.0;
				st->st[1] = 1.0;
			}

			vec2_t finalOffsetStart, finalOffsetEnd;
			R_GetBakedOffset(i, scene->mNumMeshes, &finalOffsetStart, &finalOffsetEnd);
			st->st[0] = (st->st[0] * 0.25) + finalOffsetStart[0];
			st->st[1] = (st->st[1] * 0.25) + finalOffsetStart[1];

			v++;
			st++;
		}

		// swap all the triangles
		for (j = 0; j < aiSurf->mNumFaces; j++)
		{// Assuming triangles for now... AssImp is currently set to convert everything to triangles anyway...
			tri[0] = LittleLong(aiSurf->mFaces[j].mIndices[0]) + indexesStart[i];
			tri[1] = LittleLong(aiSurf->mFaces[j].mIndices[1]) + indexesStart[i];
			tri[2] = LittleLong(aiSurf->mFaces[j].mIndices[2]) + indexesStart[i];
			tri += 3;
		}

		// find the next surface
		surf++;
	}

	{
		srfVBOMDVMesh_t *vboSurf;

		mdvModel->numVBOSurfaces = mdvModel->numSurfaces;
		vboSurf = mdvModel->vboSurfaces = (srfVBOMDVMesh_t *)ri->Hunk_Alloc(sizeof(*mdvModel->vboSurfaces) * mdvModel->numSurfaces, h_low);

		surf = mdvModel->surfaces;

		// calc tangent spaces
		{
			for (j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++)
			{
				VectorClear(v->tangent);
				VectorClear(v->bitangent);
			}

			for (f = 0; f < mdvModel->numFrames; f++)
			{
				for (j = 0, tri = surf->indexes; j < surf->numIndexes; j += 3, tri += 3)
				{
					vec3_t sdir, tdir;
					const float *v0, *v1, *v2, *t0, *t1, *t2;
					glIndex_t index0, index1, index2;

					index0 = surf->numVerts * f + tri[0];
					index1 = surf->numVerts * f + tri[1];
					index2 = surf->numVerts * f + tri[2];

					v0 = surf->verts[index0].xyz;
					v1 = surf->verts[index1].xyz;
					v2 = surf->verts[index2].xyz;

					t0 = surf->st[tri[0]].st;
					t1 = surf->st[tri[1]].st;
					t2 = surf->st[tri[2]].st;

					R_CalcTexDirs(sdir, tdir, v0, v1, v2, t0, t1, t2);

					VectorAdd(sdir, surf->verts[index0].tangent, surf->verts[index0].tangent);
					VectorAdd(sdir, surf->verts[index1].tangent, surf->verts[index1].tangent);
					VectorAdd(sdir, surf->verts[index2].tangent, surf->verts[index2].tangent);
					VectorAdd(tdir, surf->verts[index0].bitangent, surf->verts[index0].bitangent);
					VectorAdd(tdir, surf->verts[index1].bitangent, surf->verts[index1].bitangent);
					VectorAdd(tdir, surf->verts[index2].bitangent, surf->verts[index2].bitangent);
				}
			}

			for (j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++)
			{
				vec3_t sdir, tdir;

				VectorCopy(v->tangent, sdir);
				VectorCopy(v->bitangent, tdir);

				VectorNormalize(sdir);
				VectorNormalize(tdir);

				R_CalcTbnFromNormalAndTexDirs(v->tangent, v->bitangent, v->normal, sdir, tdir);
			}
		}
		
		vec3_t *verts;
		vec2_t *texcoords;
		uint32_t *normals;

		byte *data;
		int dataSize;

		int ofs_xyz, ofs_normal, ofs_st;

		dataSize = 0;

		ofs_xyz = dataSize;
		dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*verts);

		ofs_normal = dataSize;
		dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*normals);

		ofs_st = dataSize;
		dataSize += surf->numVerts * sizeof(*texcoords);

		data = (byte *)Z_Malloc(dataSize, TAG_MODEL_MD3, qtrue);

		verts = (vec3_t *)(data + ofs_xyz);
		normals = (uint32_t *)(data + ofs_normal);
		texcoords = (vec2_t *)(data + ofs_st);

		v = surf->verts;
		for (j = 0; j < surf->numVerts * mdvModel->numFrames; j++, v++)
		{
			vec3_t nxt;
			vec4_t tangent;

			VectorCopy(v->xyz, verts[j]);

			normals[j] = R_VboPackNormal(v->normal);
			CrossProduct(v->normal, v->tangent, nxt);
			VectorCopy(v->tangent, tangent);
			tangent[3] = (DotProduct(nxt, v->bitangent) < 0.0f) ? -1.0f : 1.0f;
		}

		st = surf->st;
		for (j = 0; j < surf->numVerts; j++, st++) {
			texcoords[j][0] = st->st[0];
			texcoords[j][1] = st->st[1];
		}

		vboSurf->surfaceType = SF_VBO_MDVMESH;
		vboSurf->mdvModel = mdvModel;
		vboSurf->mdvSurface = surf;
		vboSurf->numIndexes = surf->numIndexes;
		vboSurf->numVerts = surf->numVerts;

		vboSurf->minIndex = 0;
		vboSurf->maxIndex = surf->numVerts;

		vboSurf->vbo = R_CreateVBO(data, dataSize, VBO_USAGE_DYNAMIC);

		vboSurf->vbo->ofs_xyz = ofs_xyz;
		vboSurf->vbo->ofs_normal = ofs_normal;
		vboSurf->vbo->ofs_st = ofs_st;

		vboSurf->vbo->stride_xyz = sizeof(*verts);
		vboSurf->vbo->stride_normal = sizeof(*normals);
		vboSurf->vbo->stride_st = sizeof(*st);

		vboSurf->vbo->size_xyz = sizeof(*verts) * surf->numVerts;
		vboSurf->vbo->size_normal = sizeof(*normals) * surf->numVerts;

		Z_Free(data);

		vboSurf->ibo = R_CreateIBO((byte *)surf->indexes, sizeof(glIndex_t) * surf->numIndexes, VBO_USAGE_STATIC);
	}
#else //!__MODEL_MESH_MERGE__
	// swap all the surfaces
	mdvModel->numSurfaces = scene->mNumMeshes;
	mdvModel->surfaces = surf = (mdvSurface_t *)ri->Hunk_Alloc(sizeof(*surf) * mdvModel->numSurfaces, h_low);

	int numRemoved = 0;

	for (i = 0; i < scene->mNumMeshes; i++)
	{
		shader_t		*sh = NULL;
		aiString		shaderPath;	// filename
		aiMesh			*aiSurf = scene->mMeshes[i];

		// change to surface identifier
		surf->surfaceType = SF_MDV;

		scene->mMaterials[aiSurf->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &shaderPath);

		std::string textureName = AssImp_getTextureName(shaderPath.C_Str());

		std::string finalPath = R_FindAndAdjustShaderNames(modName, shaderPath.C_Str(), textureName);
		//ri->Printf(PRINT_WARNING, "shader path: %s.\n", finalPath.c_str());
		sh = R_FindShader(finalPath.c_str(), lightmapsNone, stylesDefault, qtrue);

		if (sh == NULL || sh == tr.defaultShader)
		{
#ifdef __DEBUG_ASSIMP__
			ri->Printf(PRINT_ALL, "Model: %s. Mesh: %s (%i). Missing texture: %s.\n", modName, aiSurf->mName.length ? aiSurf->mName.C_Str() : "NULL", i, finalPath.length() ? finalPath.c_str() : "NULL");
#endif //__DEBUG_ASSIMP__
			sh = tr.defaultShader;
		}

		if (!sh 
			|| sh == tr.defaultShader 
			|| (sh->surfaceFlags & SURF_NODRAW)
			|| (aiSurf->mName.data && aiSurf->mName.data[0] && aiSurf->mName.length && StringContainsWord(aiSurf->mName.C_Str(), "collision"))
			|| (aiSurf->mName.data && aiSurf->mName.data[0] && aiSurf->mName.length && StringContainsWord(aiSurf->mName.C_Str(), "nodraw"))
			|| (aiSurf->mName.data && aiSurf->mName.data[0] && aiSurf->mName.length && StringContainsWord(aiSurf->mName.C_Str(), "noshader")))
		{// A collision surface, set it to nodraw... TODO: Generate planes?!?!?!?
			numRemoved++;
			continue;
		}

		// give pointer to model for Tess_SurfaceMDX
		surf->model = mdvModel;

		// copy surface name
		Q_strncpyz(surf->name, aiSurf->mName.C_Str(), sizeof(surf->name));

		// lowercase the surface name so skin compares are faster
		Q_strlwr(surf->name);

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen(surf->name);
		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// register the shaders
		surf->numShaderIndexes = 1;
		surf->shaderIndexes = shaderIndex = (int *)ri->Hunk_Alloc(sizeof(*shaderIndex), h_low);

		if (sh->defaultShader)
		{
			*shaderIndex = 0;
		}
		else
		{
			*shaderIndex = sh->index;
		}

		// swap all the triangles
		surf->numIndexes = aiSurf->mNumFaces * 3;
		surf->indexes = tri = (glIndex_t *)ri->Hunk_AllocateTempMemory(sizeof(*tri) * aiSurf->mNumFaces * 3);

		for (j = 0; j < aiSurf->mNumFaces; j++, tri += 3)
		{// Assuming triangles for now... AssImp is currently set to convert everything to triangles anyway...
			tri[0] = LittleLong(aiSurf->mFaces[j].mIndices[0]);
			tri[1] = LittleLong(aiSurf->mFaces[j].mIndices[1]);
			tri[2] = LittleLong(aiSurf->mFaces[j].mIndices[2]);
		}

		// swap all the XyzNormals
		surf->numVerts = aiSurf->mNumVertices;
		surf->verts = v = (mdvVertex_t *)ri->Hunk_AllocateTempMemory(sizeof(*v) * aiSurf->mNumVertices);

		for (j = 0; j < aiSurf->mNumVertices; j++, v++)
		{
			aiVector3D xyz = aiSurf->mVertices[j];

			v->xyz[0] = LittleShort(xyz.x);
			v->xyz[1] = LittleShort(xyz.y);
			v->xyz[2] = LittleShort(xyz.z);

			aiVector3D norm = aiSurf->mNormals[j];

#ifdef __INVERT_ASSIMP_NORMALS__
			v->normal[0] = -norm.x;
			v->normal[1] = -norm.y;
			v->normal[2] = -norm.z;
#else //!__INVERT_ASSIMP_NORMALS__
			v->normal[0] = norm.x;
			v->normal[1] = norm.y;
			v->normal[2] = norm.z;
#endif //__INVERT_ASSIMP_NORMALS__
		}

		// swap all the ST
		surf->st = st = (mdvSt_t *)ri->Hunk_Alloc(sizeof(*st) * aiSurf->mNumVertices, h_low);
		
		for (j = 0; j < aiSurf->mNumVertices; j++, st++)
		{
			if (aiSurf->mNormals != NULL && aiSurf->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
			{
				st->st[0] = LittleFloat(aiSurf->mTextureCoords[0][j].x);
				st->st[1] = LittleFloat(1 - aiSurf->mTextureCoords[0][j].y);
			}
			else
			{
				st->st[0] = 0.0;
				st->st[1] = 1.0;
			}
		}

#if 0
		// calc tangent spaces
		{
			for (j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++)
			{
				VectorClear(v->tangent);
				VectorClear(v->bitangent);
			}

			for (f = 0; f < mdvModel->numFrames; f++)
			{
				for (j = 0, tri = surf->indexes; j < surf->numIndexes; j += 3, tri += 3)
				{
					vec3_t sdir, tdir;
					const float *v0, *v1, *v2, *t0, *t1, *t2;
					glIndex_t index0, index1, index2;

					index0 = surf->numVerts * f + tri[0];
					index1 = surf->numVerts * f + tri[1];
					index2 = surf->numVerts * f + tri[2];

					v0 = surf->verts[index0].xyz;
					v1 = surf->verts[index1].xyz;
					v2 = surf->verts[index2].xyz;

					t0 = surf->st[tri[0]].st;
					t1 = surf->st[tri[1]].st;
					t2 = surf->st[tri[2]].st;

					R_CalcTexDirs(sdir, tdir, v0, v1, v2, t0, t1, t2);

					VectorAdd(sdir, surf->verts[index0].tangent, surf->verts[index0].tangent);
					VectorAdd(sdir, surf->verts[index1].tangent, surf->verts[index1].tangent);
					VectorAdd(sdir, surf->verts[index2].tangent, surf->verts[index2].tangent);
					VectorAdd(tdir, surf->verts[index0].bitangent, surf->verts[index0].bitangent);
					VectorAdd(tdir, surf->verts[index1].bitangent, surf->verts[index1].bitangent);
					VectorAdd(tdir, surf->verts[index2].bitangent, surf->verts[index2].bitangent);
				}
			}

			for (j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++)
			{
				vec3_t sdir, tdir;

				VectorCopy(v->tangent, sdir);
				VectorCopy(v->bitangent, tdir);

				VectorNormalize(sdir);
				VectorNormalize(tdir);

				R_CalcTbnFromNormalAndTexDirs(v->tangent, v->bitangent, v->normal, sdir, tdir);
			}
		}
#endif

		// find the next surface
		surf++;
	}

	// Removed some (most likely collision) surfaces... Makse sure when drawing, that we ignore them...
	mdvModel->numSurfaces = scene->mNumMeshes - numRemoved;

	{
		srfVBOMDVMesh_t *vboSurf;

		mdvModel->numVBOSurfaces = mdvModel->numSurfaces;
		mdvModel->vboSurfaces = (srfVBOMDVMesh_t *)ri->Hunk_AllocateTempMemory(sizeof(*mdvModel->vboSurfaces) * mdvModel->numSurfaces);

		vboSurf = mdvModel->vboSurfaces;
		surf = mdvModel->surfaces;

		/*
		#ifdef __INSTANCED_MODELS__
		GLSL_BindProgram(&tr.instanceShader);
		mdvModel->vao = NULL;
		qglGenVertexArrays(1, &mdvModel->vao);
		qglBindVertexArray(mdvModel->vao);
		#endif //__INSTANCED_MODELS__
		*/

#ifdef __LODMODEL_INSTANCING__
		GLSL_BindProgram(&tr.instanceVAOShader);
		mdvModel->vao = NULL;
		qglGenVertexArrays(1, &mdvModel->vao);
		qglBindVertexArray(mdvModel->vao);
#endif //__LODMODEL_INSTANCING__

		for (i = 0; i < mdvModel->numSurfaces; i++, vboSurf++, surf++)
		{
			vec3_t *verts;
			vec2_t *texcoords;
			uint32_t *normals;

#ifdef __REGEN_MODEL_NORMALS__
			shader_t *shader = tr.shaders[surf->shaderIndexes[0]];
			if (shader && shader->recalculateNormals)
			{
				GenerateEnhancedNormalsForSurface(surf);
			}
#endif //__REGEN_MODEL_NORMALS__

			byte *data;
			int dataSize;

			int ofs_xyz, ofs_normal, ofs_st;

			dataSize = 0;

			ofs_xyz = dataSize;
			dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*verts);

			ofs_normal = dataSize;
			dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*normals);

			ofs_st = dataSize;
			dataSize += surf->numVerts * sizeof(*texcoords);

			data = (byte *)Z_Malloc(dataSize, TAG_MODEL_MD3, qtrue);

			verts = (vec3_t *)(data + ofs_xyz);
			normals = (uint32_t *)(data + ofs_normal);
			texcoords = (vec2_t *)(data + ofs_st);

			v = surf->verts;
			for (j = 0; j < surf->numVerts * mdvModel->numFrames; j++, v++)
			{
				vec3_t nxt;
				vec4_t tangent;

				VectorCopy(v->xyz, verts[j]);

				normals[j] = R_VboPackNormal(v->normal);
				CrossProduct(v->normal, v->tangent, nxt);
				VectorCopy(v->tangent, tangent);
				tangent[3] = (DotProduct(nxt, v->bitangent) < 0.0f) ? -1.0f : 1.0f;
			}

			st = surf->st;
			for (j = 0; j < surf->numVerts; j++, st++) {
				texcoords[j][0] = st->st[0];
				texcoords[j][1] = st->st[1];
			}

#ifdef __MESH_OPTIMIZATION__
			R_OptimizeMesh((uint32_t *)&surf->numVerts, (uint32_t *)&surf->numIndexes, surf->indexes, verts);
#endif //__MESH_OPTIMIZATION__


			vboSurf->surfaceType = SF_VBO_MDVMESH;
			vboSurf->mdvModel = mdvModel;
			vboSurf->mdvSurface = surf;
			vboSurf->numIndexes = surf->numIndexes;
			vboSurf->numVerts = surf->numVerts;

			vboSurf->minIndex = 0;
			vboSurf->maxIndex = surf->numVerts;

			/*#ifdef __INSTANCED_MODELS__
			if (mdvModel->numFrames <= 1)
			vboSurf->vbo = R_CreateVBO(data, dataSize, VBO_USAGE_STATIC);
			else
			#endif //__INSTANCED_MODELS__*/
			vboSurf->vbo = R_CreateVBO(data, dataSize, VBO_USAGE_DYNAMIC);

			vboSurf->vbo->ofs_xyz = ofs_xyz;
			vboSurf->vbo->ofs_normal = ofs_normal;
			vboSurf->vbo->ofs_st = ofs_st;

			vboSurf->vbo->stride_xyz = sizeof(*verts);
			vboSurf->vbo->stride_normal = sizeof(*normals);
			vboSurf->vbo->stride_st = sizeof(*st);

			vboSurf->vbo->size_xyz = sizeof(*verts) * surf->numVerts;
			vboSurf->vbo->size_normal = sizeof(*normals) * surf->numVerts;

			Z_Free(data);

			vboSurf->ibo = R_CreateIBO((byte *)surf->indexes, sizeof(glIndex_t) * surf->numIndexes, VBO_USAGE_STATIC);

			ri->Hunk_FreeTempMemory(surf->indexes);
			ri->Hunk_FreeTempMemory(surf->verts);
		}

		/*
		#ifdef __INSTANCED_MODELS__
		qglGenBuffers(1, &tr.instanceShader.instances_buffer);
		qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_buffer);
		qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_POSITION, tr.instanceShader.instances_buffer);
		qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t), NULL, GL_STREAM_DRAW);

		qglGenBuffers(1, &tr.instanceShader.instances_mvp);
		qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_mvp);
		qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_MVP, tr.instanceShader.instances_mvp);
		qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(matrix_t), NULL, GL_STREAM_DRAW);

		mdvModel->ofs_instancesPosition = 0;
		mdvModel->ofs_instancesMVP = MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t);

		qglBindVertexArray(0);
		GLSL_BindProgram(NULL);
		#endif //__INSTANCED_MODELS__
		*/

#ifdef __LODMODEL_INSTANCING__
		qglGenBuffers(1, &tr.instanceVAOShader.instances_buffer);
		qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceVAOShader.instances_buffer);
		qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_POSITION, tr.instanceVAOShader.instances_buffer);
		//qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t), NULL, GL_STREAM_DRAW);

		//qglGenBuffers(1, &tr.instanceShader.instances_mvp);
		//qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_mvp);
		//qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_MVP, tr.instanceShader.instances_mvp);
		//qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(matrix_t), NULL, GL_STREAM_DRAW);

		mdvModel->ofs_instancesPosition = 0;
		//mdvModel->ofs_instancesMVP = MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t);

		qglBindVertexArray(0);
		GLSL_BindProgram(NULL);
#endif //__LODMODEL_INSTANCING__
	}
#endif //__MODEL_MESH_MERGE__

	// No longer need the scene data...
	assImpImporter.FreeScene();

	return qtrue;
}

qhandle_t R_RegisterAssImp(const char *name, model_t *mod)
{
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded = qfalse;
	int			numLoaded;
	char filename[MAX_QPATH], namebuf[MAX_QPATH + 20];
	char *fext, defex[] = "obj";

	numLoaded = 0;

	strcpy(filename, name);

	fext = strchr(filename, '.');

	if (!fext)
	{
		fext = defex;
	}
	else
	{
		*fext = '\0';
		fext++;
	}

	for (lod = MD3_MAX_LODS - 1; lod >= 0; lod--)
	{
		if (lod)
			Com_sprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		else
			Com_sprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);

		qboolean bAlreadyCached = qfalse;

		int size = CModelCache->LoadFile(namebuf, (void**)&buf, &bAlreadyCached);

		if (!size)
		{
#ifdef __DEBUG_ASSIMP__
			if (!lod)
			{// Only output debug info when missing non-lod versions...
				ri->Printf(PRINT_WARNING, "R_RegisterAssImp: Model %s has zero size. Doesn't exist???\n", namebuf);
			}
#endif //__DEBUG_ASSIMP__

			continue;
		}

		ident = LittleLong(MD3_IDENT);

		loaded = R_LoadAssImp(mod, lod, buf, namebuf, size, fext);

#ifdef __EXPERIMENTAL_ASSIMP_GLM_CONVERSIONS__
		if (loaded && mod->type == MOD_MDXM)
		{// If we loaded a bone model, skip the lod crap...
			numLoaded++;

#ifdef __DEBUG_ASSIMP__
			ri->Printf(PRINT_WARNING, "R_RegisterAssImp: loaded assimp boned model %s. modIndex %i. numLods %i. numLoaded %i. numSurfaces %i.\n", name, mod->index, mod->numLods, numLoaded, mod->data.glm->vboModels[0].numVBOMeshes);
#endif //__DEBUG_ASSIMP__

			return mod->index;
		}
#endif //__EXPERIMENTAL_ASSIMP_GLM_CONVERSIONS__

		if (loaded)
		{
			mod->numLods++;
			numLoaded++;
		}
		else
		{
#ifdef __DEBUG_ASSIMP__
			ri->Printf(PRINT_WARNING, "R_RegisterAssImp: Model %s does not exist?\n", namebuf);
#endif //__DEBUG_ASSIMP__

			if (!strcmp(fext, "fbx"))
			{
				ri->Printf(PRINT_WARNING, "R_RegisterAssImp: FBX model %s failed to load. Probably an unsupported FBX format.\n", namebuf);
			}

			break;
		}
	}

	if (numLoaded)
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for (lod--; lod >= 0; lod--)
		{
			mod->numLods++;
			mod->data.mdv[lod] = mod->data.mdv[lod + 1];
		}
		
#ifdef __DEBUG_ASSIMP__
		ri->Printf(PRINT_WARNING, "R_RegisterAssImp: loaded assimp model %s. modIndex %i. numLods %i. numLoaded %i. numSurfaces %i.\n", name, mod->index, mod->numLods, numLoaded, (mod->data.mdv && mod->data.mdv[0]) ? mod->data.mdv[0]->numSurfaces : mod->data.glm->vboModels[0].numVBOMeshes);
#endif //__DEBUG_ASSIMP__

		return mod->index;
	}

#ifdef _DEBUG
	ri->Printf(PRINT_WARNING, "R_RegisterAssImp: couldn't load %s\n", name);
#endif

	mod->type = MOD_BAD;
	return 0;
}

#ifdef __NIF_IMPORT_TEST__
//#define __DEBUG_NIF__

#pragma comment(lib, "../../../lib/niflib_dll.lib")

#include "nif_include/niflib.h"
#include "nif_include/obj/NiGeometry.h"
#include "nif_include/obj/NiGeometryData.h"
#include "nif_include/obj/NiNode.h"
#include "nif_include/obj/NiTriShape.h"
#include "nif_include/obj/NiTriShapeData.h"
#include "nif_include/nifqhull.h"
#include "nif_include/Ref.h"
#include "nif_include/RefObject.h"
#include "nif_include/obj/NiExtraData.h"
#include "nif_include/obj/NiTimeController.h"
#include "nif_include/obj/NiTexturingProperty.h"
#include "nif_include/obj/BSShaderTextureSet.h"
#include "nif_include/obj/BSLightingShaderProperty.h"
#include "nif_include/obj/NiSourceTexture.h"
#include "nif_include/obj/NiMaterialProperty.h"
#include "nif_include/obj/NiVertexColorProperty.h"
#include "nif_include/obj/NiTriStripsData.h"
#include "nif_include/obj/NiTriStrips.h"
#include "nif_include/obj/NiBinaryExtraData.h"
#include "nif_include/obj/bhkCollisionObject.h"
#include "nif_include/obj/bhkTransformShape.h"
#include "nif_include/obj/bhkListShape.h"
#include "nif_include/obj/bhkPackedNiTriStripsShape.h"
#include "nif_include/obj/hkPackedNiTriStripsData.h"
#include "nif_include/obj/bhkMoppBvTreeShape.h"
#include "nif_include/obj/bhkRigidBody.h"

void VectorRotateNIF(vec3_t vIn, vec3_t vRotation, vec3_t out)
{
	vec3_t vWork, va;
	int nIndex[3][2];
	int i;

	VectorCopy(vIn, va);
	VectorCopy(va, vWork);
	nIndex[0][0] = 1; nIndex[0][1] = 2;
	nIndex[1][0] = 2; nIndex[1][1] = 0;
	nIndex[2][0] = 0; nIndex[2][1] = 1;
	for (i = 0; i < 3; i++)
	{
		if (vRotation[i] != 0)
		{
			float dAngle = vRotation[i] * PI / 180.0f;
			float c = (vec_t)cos(dAngle);
			float s = (vec_t)sin(dAngle);
			vWork[nIndex[i][0]] = va[nIndex[i][0]] * c - va[nIndex[i][1]] * s;
			vWork[nIndex[i][1]] = va[nIndex[i][0]] * s + va[nIndex[i][1]] * c;
		}
		VectorCopy(vWork, va);
	}
	VectorCopy(vWork, out);
}

void VectorRotateOriginNIF(vec3_t vIn, vec3_t vRotation, vec3_t vOrigin, vec3_t out)
{
	vec3_t vTemp, vTemp2;

	VectorSubtract(vIn, vOrigin, vTemp);
	VectorRotateNIF(vTemp, vRotation, vTemp2);
	VectorAdd(vTemp2, vOrigin, out);
}

void R_NifRotate(float *pos)
{
	vec3_t vRotation;

	vRotation[0] = 0;
	vRotation[1] = 0;
	vRotation[2] = 90;

	VectorRotateOriginNIF(pos, vRotation, vec3_origin, pos);
}

static qboolean R_LoadNIF(model_t * mod, int lod, void *buffer, const char *modName, int size, const char *ext)
{
	int					i, j;

	mdvModel_t			*mdvModel;
	mdvFrame_t			*frame;
	mdvSurface_t		*surf;
	int					*shaderIndex;
	glIndex_t			*tri;
	mdvVertex_t			*v;
	mdvSt_t				*st;

	std::string basePath = AssImp_getBasePath(modName);

#ifdef __DEBUG_NIF__
	ri->Printf(PRINT_ERROR, "R_LoadNIF: Loading model %s. Extension: %s. bufferSize: %i. basePath: %s.\n", modName, ext, size, basePath.c_str());
#endif //__DEBUG_NIF__

#define NIF_MODEL_SCALE 0.5

	std::string fullPath = va("warzone/%s", modName);

	try {
		Niflib::NiObjectRef root = Niflib::ReadNifTree(fullPath.c_str());

		if (!root)
		{
			ri->Printf(PRINT_ERROR, "R_LoadNIF: %s could not load. Error: ReadNifTree failed.\n", modName);
			return qfalse;
		}

		Niflib::NiNodeRef node = Niflib::DynamicCast<Niflib::NiNode>(root);

		if (node == NULL)
		{
			ri->Printf(PRINT_ERROR, "R_LoadNIF: %s could not load. Error: DynamicCast<NiNode>(root) failed.\n", modName);
			return qfalse;
		}

		std::vector<Niflib::Ref<Niflib::NiAVObject>> children = node->GetChildren();

		if (children.size() <= 0)
		{
			ri->Printf(PRINT_ERROR, "R_LoadNIF: %s could not load. Error: children.size() is none.\n", modName);
			return qfalse;
		}

		// Calculate the bounds/radius info... And count geometry type surfs...
		int numGeomSurfs = 0;
		vec3_t bounds[2];
		ClearBounds(bounds[0], bounds[1]);

		int z = 0;

		for (std::vector<Niflib::Ref<Niflib::NiAVObject>>::iterator it = children.begin(); it != children.end(); ++it, z++)
		{
			Niflib::NiAVObject *NiAVO = (*it);

			if (!NiAVO->IsDerivedType(Niflib::NiGeometry::TYPE))
			{
				//ri->Printf(PRINT_WARNING, "child %i (%s) is not NiGeometry.\n", z, NiAVO->GetName().length() ? NiAVO->GetName().c_str() : "UNKNOWN");
				continue;
			}

			Niflib::NiGeometry *NiGEOM = Niflib::DynamicCast<Niflib::NiGeometry>(NiAVO);

			bool isNiTriShape = false;
			bool isNiTriShapeData = false;
			bool isNiTriStrips = false;
			bool isNiTriStripsData = false;

			if (!NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
			{
				//ri->Printf(PRINT_WARNING, "child %i (%s) is not NiTriShape or NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
				continue;
			}
			
			if (NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE))
			{
				isNiTriShape = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriShape.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}
			
			if (NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE))
			{
				isNiTriShapeData = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}
			
			if (NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE))
			{
				isNiTriStrips = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriStrips.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}
			
			if (NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
			{
				isNiTriStripsData = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriStripsData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			bool hasTEXTURE = false;
			
			for (short index(0); index < 2; ++index)
			{
				Niflib::BSLightingShaderProperty *shader = Niflib::DynamicCast<Niflib::BSLightingShaderProperty>(NiGEOM->GetBSProperty(index));

				if (shader != NULL)
				{
					std::vector<std::string> textures = shader->GetTextureSet()->GetTextures();
					
					if (!textures.size()) continue;

					for (std::vector<std::string>::iterator ti = textures.begin(); ti != textures.end(); ++ti)
					{
						std::string texture = (*ti);

						if (texture.length())
						{
							//ri->Printf(PRINT_WARNING, "child %i (%s) index %i has %s texture.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN", index, texture.c_str());
							hasTEXTURE = true;
							break;
						}
					}
				}
			}

			if (!hasTEXTURE)
			{
				continue;
			}
			
			Niflib::Ref<Niflib::NiGeometryData> niGeomData = Niflib::DynamicCast<Niflib::NiGeometryData>(NiGEOM->GetData());


			std::vector<Niflib::Vector3> verts;
			vector<Niflib::Triangle> triangles;
			std::vector<int> idxs = niGeomData->GetVertexIndices();

			if (isNiTriShape || isNiTriShapeData)
			{
				Niflib::NiTriShapeDataRef data = Niflib::DynamicCast<Niflib::NiTriShapeData>(niGeomData);
				verts = data->GetVertices();
				triangles = data->GetTriangles();
			}
			else if (isNiTriStrips || isNiTriStripsData)
			{
				Niflib::NiTriStripsDataRef data = Niflib::DynamicCast<Niflib::NiTriStripsData>(niGeomData);
				verts = data->GetVertices();
				triangles = data->GetTriangles();
			}
			else
			{
				verts = niGeomData->GetVertices();
				std::vector<int> idxs = niGeomData->GetVertexIndices();
			}

			uint32_t numVERTS = 0;

			for (std::vector<Niflib::Vector3>::iterator vi = verts.begin(); vi != verts.end(); vi++)
			{
				Niflib::Vector3 vt = (*vi);

				vec3_t vert;
				vert[0] = vt.x * NIF_MODEL_SCALE;
				vert[1] = vt.y * NIF_MODEL_SCALE;
				vert[2] = vt.z * NIF_MODEL_SCALE;

				R_NifRotate(vert);

				AddPointToBounds(vert, bounds[0], bounds[1]);
				numVERTS++;
			}

			uint32_t numIDX = 0;
			
			if (isNiTriShape || isNiTriShapeData || isNiTriStrips || isNiTriStripsData)
			{
				for (std::vector<Niflib::Triangle>::iterator tr = triangles.begin(); tr != triangles.end(); tr++)
				{
					numIDX++;
				}
			}
			else
			{
				for (std::vector<int>::iterator id = idxs.begin(); id != idxs.end(); id++)
				{
					numIDX++;
				}
			}

			//ri->Printf(PRINT_WARNING, "child %i (%s) has %u verts and %u indexes.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN", numVERTS, numIDX);

			if (numVERTS > 0 && numIDX > 0)
			{
				numGeomSurfs++;
			}
		}

		if (numGeomSurfs <= 0)
		{
#ifdef __DEBUG_NIF__
			ri->Printf(PRINT_ALL, "R_LoadNIF: Model %s has no geometry.\n", modName);
#endif //__DEBUG_NIF__
			return qfalse;
		}

#ifdef __DEBUG_NIF__
		ri->Printf(PRINT_ALL, "R_LoadNIF: Model %s. Surfaces: %i. Bounds: %f %f %f x %f %f %f.\n", modName, numGeomSurfs, bounds[0][0], bounds[0][1], bounds[0][2], bounds[1][0], bounds[1][1], bounds[1][2]);
#endif //__DEBUG_NIF__

		mod->type = MOD_MESH;
		mod->dataSize += size;

		qboolean bAlreadyFound = qfalse;
		mdvModel = mod->data.mdv[lod] = (mdvModel_t *)CModelCache->Allocate(size, buffer, modName, &bAlreadyFound, TAG_MODEL_MD3);

		if (bAlreadyFound)
		{
			CModelCache->AllocateShaders(modName);
		}

		// swap all the frames - Not supported for now, just using a single empty frame with basic data...
		mdvModel->numFrames = 1;
		mdvModel->frames = frame = (mdvFrame_t *)ri->Hunk_Alloc(sizeof(*frame) * mdvModel->numFrames, h_low);

		vec3_t localOrigin;
		VectorSet(localOrigin, 0.0, 0.0, 0.0);
		float radius = Distance(bounds[0], bounds[1]) / 2.0;

		for (i = 0; i < mdvModel->numFrames; i++, frame++)
		{
			frame->radius = LittleFloat(radius);
			for (j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(localOrigin[j]);
			}
		}

		// swap all the tags - Disabled for now... Not sure if I need tags on non-md3 models...
		mdvModel->numTags = 0;

		// swap all the surfaces
		mdvModel->numSurfaces = numGeomSurfs;
		mdvModel->surfaces = surf = (mdvSurface_t *)ri->Hunk_Alloc(sizeof(*surf) * mdvModel->numSurfaces, h_low);


		int numRemoved = 0;

		for (std::vector<Niflib::Ref<Niflib::NiAVObject>>::iterator it = children.begin(); it != children.end(); ++it)
		{
			Niflib::NiAVObject *NiAVO = (*it);

			if (!NiAVO->IsDerivedType(Niflib::NiGeometry::TYPE))
			{
				continue;
			}

			Niflib::NiGeometry *NiGEOM = Niflib::DynamicCast<Niflib::NiGeometry>(NiAVO);

			bool isNiTriShape = false;
			bool isNiTriShapeData = false;
			bool isNiTriStrips = false;
			bool isNiTriStripsData = false;
			bool hasNormals = false;
			bool hasUVs = false;

			if (!NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
			{
				//ri->Printf(PRINT_WARNING, "child %i (%s) is not NiTriShape or NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
				continue;
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE))
			{
				isNiTriShape = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriShape.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE))
			{
				isNiTriShapeData = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE))
			{
				isNiTriStrips = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriStrips.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
			{
				isNiTriStripsData = true;
				//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriStripsData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			Niflib::Ref<Niflib::NiGeometryData> niGeomData = Niflib::DynamicCast<Niflib::NiGeometryData>(NiGEOM->GetData());

			uint32_t numIDX = 0;

			std::vector<Niflib::Vector3> verts;
			vector<Niflib::Triangle> triangles;
			std::vector<Niflib::Vector3> norms;
			std::vector<Niflib::TexCoord> tcs;
			std::vector<int> idxs = niGeomData->GetVertexIndices();

			if (isNiTriShape || isNiTriShapeData)
			{
				Niflib::NiTriShapeDataRef data = Niflib::DynamicCast<Niflib::NiTriShapeData>(niGeomData);
				verts = data->GetVertices();
				triangles = data->GetTriangles();

				if (data->GetHasNormals())
				{
					norms = data->GetNormals();
					hasNormals = true;
				}

				if (data->GetUVSetCount())
				{
					tcs = data->GetUVSet(0);
					hasUVs = true;
				}
			}
			else if (isNiTriStrips || isNiTriStripsData)
			{
				Niflib::NiTriStripsDataRef data = Niflib::DynamicCast<Niflib::NiTriStripsData>(niGeomData);
				verts = data->GetVertices();
				triangles = data->GetTriangles();
				
				if (data->GetHasNormals())
				{
					norms = data->GetNormals();
					hasNormals = true;
				}

				if (data->GetUVSetCount())
				{
					tcs = data->GetUVSet(0);
					hasUVs = true;
				}
			}
			else
			{
				verts = niGeomData->GetVertices();
				idxs = niGeomData->GetVertexIndices();

				if (niGeomData->GetHasNormals())
				{
					norms = niGeomData->GetNormals();
					hasNormals = true;
				}

				if (niGeomData->GetUVSetCount())
				{
					tcs = niGeomData->GetUVSet(0);
					hasUVs = true;
				}
			}

			if (!hasNormals)
			{
				if (niGeomData->GetHasNormals())
				{
					norms = niGeomData->GetNormals();
					hasNormals = true;
				}
			}

			/*if (hasNormals)
			{
				ri->Printf(PRINT_WARNING, "child %i (%s) has normals.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}*/

			if (!hasUVs)
			{
				if (niGeomData->GetUVSetCount())
				{
					tcs = niGeomData->GetUVSet(0);
					hasUVs = true;
				}
			}

			/*if (hasUVs)
			{
				ri->Printf(PRINT_WARNING, "child %i (%s) has UVs.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}*/

			if (isNiTriShape || isNiTriShapeData || isNiTriStrips || isNiTriStripsData)
			{
				for (std::vector<Niflib::Triangle>::iterator i = triangles.begin(); i != triangles.end(); i++)
				{
					numIDX++;
				}
			}
			else
			{
				for (std::vector<int>::iterator i = idxs.begin(); i != idxs.end(); i++)
				{
					numIDX++;
				}
			}

			uint32_t numVERTS = 0;

			for (std::vector<Niflib::Vector3>::iterator vi = verts.begin(); vi != verts.end(); ++vi)
			{
				numVERTS++;
			}

			if (numIDX <= 0 || numVERTS <= 0)
			{
				continue;
			}

			shader_t		*sh = NULL;
			aiString		shaderPath;	// filename

			// change to surface identifier
			surf->surfaceType = SF_MDV;


			std::string surfaceName = NiGEOM->GetName();
			std::string shaderName;

			for (short index(0); index < 2; ++index)
			{
				Niflib::BSLightingShaderProperty *shader = Niflib::DynamicCast<Niflib::BSLightingShaderProperty>(NiGEOM->GetBSProperty(index));

				if (shader != NULL)
				{
					std::vector<std::string> textures = shader->GetTextureSet()->GetTextures();

					if (!textures.size()) continue;

					for (std::vector<std::string>::iterator ti = textures.begin(); ti != textures.end(); ++ti)
					{
						std::string texture = (*ti);

						if (texture.length())
						{
							shaderName = texture.c_str();
							shaderPath = texture.c_str();
							break;
						}
					}
				}
			}

#ifdef __CONVEX_HULL_CALCULATION__
			//
			// Compute convex hull for mesh verts, this may be really useful for stuff, could be called on any mesh, not just NIF...
			//

			std::vector<Niflib::Triangle> convexHull = Niflib::NifQHull::compute_convex_hull(verts);

			//
			//
			//
#endif //__CONVEX_HULL_CALCULATION__

			std::string textureName = AssImp_getTextureName(shaderName.c_str());

			if (!textureName.length()) textureName = shaderName;

			std::string finalPath = R_FindAndAdjustShaderNames(modName, shaderPath.C_Str(), textureName);

#ifdef __DEBUG_NIF__
			ri->Printf(PRINT_ALL, "R_LoadNIF: Model: %s. Mesh: %s (%i). Using texture: %s.\n", modName, surfaceName.length() ? surfaceName.c_str() : "NULL", i, finalPath.length() ? finalPath.c_str() : "NULL");
#endif //__DEBUG_NIF__

			sh = R_FindShader(finalPath.c_str(), lightmapsNone, stylesDefault, qtrue);

			if (sh == NULL || sh == tr.defaultShader)
			{
#ifdef __DEBUG_NIF__
				ri->Printf(PRINT_ALL, "R_LoadNIF: Model: %s. Mesh: %s (%i). Missing texture: %s.\n", modName, shaderName.length() ? shaderName.c_str() : "NULL", i, finalPath.length() ? finalPath.c_str() : "NULL");
#endif //__DEBUG_NIF__
				sh = tr.defaultShader;
			}

			if (!sh
				|| sh == tr.defaultShader
				|| (sh->surfaceFlags & SURF_NODRAW)
				|| (surfaceName.length() && StringContainsWord(surfaceName.c_str(), "collision"))
				|| (surfaceName.length() && StringContainsWord(surfaceName.c_str(), "nodraw"))
				|| (surfaceName.length() && StringContainsWord(surfaceName.c_str(), "noshader")))
			{// A collision surface, set it to nodraw... TODO: Generate planes?!?!?!?
#ifdef __DEBUG_NIF__
				ri->Printf(PRINT_ALL, "R_LoadNIF: Model: %s. Mesh: %s (%i). Missing texture (or is collision surface): %s.\n", modName, shaderName.length() ? shaderName.c_str() : "NULL", i, finalPath.length() ? finalPath.c_str() : "NULL");
#endif //__DEBUG_NIF__
				numRemoved++;
				continue;
			}

			// give pointer to model for Tess_SurfaceMDX
			surf->model = mdvModel;

			// copy surface name
			Q_strncpyz(surf->name, surfaceName.c_str(), sizeof(surf->name));

			// lowercase the surface name so skin compares are faster
			Q_strlwr(surf->name);

			// strip off a trailing _1 or _2
			// this is a crutch for q3data being a mess
			j = strlen(surf->name);
			if (j > 2 && surf->name[j - 2] == '_')
			{
				surf->name[j - 2] = 0;
			}

			// register the shaders
			surf->numShaderIndexes = 1;
			surf->shaderIndexes = shaderIndex = (int *)ri->Hunk_Alloc(sizeof(*shaderIndex), h_low);

			if (sh->defaultShader)
			{
				*shaderIndex = 0;
			}
			else
			{
				*shaderIndex = sh->index;
			}


			// swap all the triangles
			surf->numIndexes = numIDX * 3;
			surf->indexes = tri = (glIndex_t *)ri->Hunk_AllocateTempMemory(sizeof(*tri) * numIDX * 3);

			if (isNiTriShape || isNiTriShapeData || isNiTriStrips || isNiTriStripsData)
			{
				for (std::vector<Niflib::Triangle>::iterator ti = triangles.begin(); ti != triangles.end(); ti++)
				{
					Niflib::Triangle tr = (*ti);
					tri[0] = LittleLong(tr[2]);
					tri[1] = LittleLong(tr[1]);
					tri[2] = LittleLong(tr[0]);
					tri+=3;
				}
			}
			else
			{
				int tr = 0;

				for (std::vector<int>::iterator ti = idxs.begin(); ti != idxs.end(); ti++)
				{
					surf->indexes[tr] = LittleLong((*ti));
					tr++;
				}
			}

/*#ifdef __DEBUG_NIF__
			for (int zz = 0; zz < numIDX * 3; zz++)
				ri->Printf(PRINT_ALL, "Index %i: %i.\n", zz, surf->indexes[zz]);
#endif //__DEBUG_NIF__*/

			// swap all the XyzNormals
			surf->numVerts = numVERTS;
			surf->verts = v = (mdvVertex_t *)ri->Hunk_AllocateTempMemory(sizeof(*v) * numVERTS);

			for (std::vector<Niflib::Vector3>::iterator vi = verts.begin(); vi != verts.end(); vi++)
			{
				Niflib::Vector3 vt = (*vi);
				v->xyz[0] = LittleShort(vt.x * NIF_MODEL_SCALE);
				v->xyz[1] = LittleShort(vt.y * NIF_MODEL_SCALE);
				v->xyz[2] = LittleShort(vt.z * NIF_MODEL_SCALE);

				R_NifRotate(v->xyz);

				v++;
			}

/*#ifdef __DEBUG_NIF__
			for (int zz = 0; zz < numVERTS; zz++)
				ri->Printf(PRINT_ALL, "Vert %i: %f %f %f.\n", zz, surf->verts[zz].xyz[0], surf->verts[zz].xyz[1], surf->verts[zz].xyz[2]);
#endif //__DEBUG_NIF__*/

			if (hasNormals)
			{
				v = surf->verts;

				for (std::vector<Niflib::Vector3>::iterator ni = norms.begin(); ni != norms.end(); ni++)
				{
					Niflib::Vector3 n = (*ni);
					v->normal[0] = n.x;
					v->normal[1] = n.y;
					v->normal[2] = n.z;
					v++;
				}
			}

/*#ifdef __DEBUG_NIF__
			for (int zz = 0; zz < numVERTS; zz++)
				ri->Printf(PRINT_ALL, "Normal %i: %f %f %f.\n", zz, surf->verts[zz].normal[0], surf->verts[zz].xyz[1], surf->verts[zz].normal[2]);
#endif //__DEBUG_NIF__*/

			// swap all the ST
			surf->st = st = (mdvSt_t *)ri->Hunk_Alloc(sizeof(*st) * numVERTS, h_low);

			if (hasUVs)
			{
				for (std::vector<Niflib::TexCoord>::iterator ti = tcs.begin(); ti != tcs.end(); ++ti)
				{
					Niflib::TexCoord tc = (*ti);
					st->st[0] = LittleFloat(tc.u);
					//st->st[1] = LittleFloat(1 - tc.v);
					st->st[1] = LittleFloat(tc.v);
					st++;
				}
			}
			else
			{// Assume all 0,1
				for (j = 0; j < numVERTS; j++)
				{
					st->st[0] = 0.0;
					st->st[1] = 1.0;
					st++;
				}
			}

/*#ifdef __DEBUG_NIF__
			for (int zz = 0; zz < numVERTS; zz++)
				ri->Printf(PRINT_ALL, "UV %i: %f %f.\n", zz, surf->st[zz].st[0], surf->st[zz].st[1]);
#endif //__DEBUG_NIF__*/

			// find the next surface
			surf++;
		}

		// Removed some (most likely collision) surfaces... Makse sure when drawing, that we ignore them...
		mdvModel->numSurfaces = mdvModel->numSurfaces - numRemoved;

		{
			srfVBOMDVMesh_t *vboSurf;

			mdvModel->numVBOSurfaces = mdvModel->numSurfaces;
			mdvModel->vboSurfaces = (srfVBOMDVMesh_t *)ri->Hunk_AllocateTempMemory(sizeof(*mdvModel->vboSurfaces) * mdvModel->numSurfaces);

			vboSurf = mdvModel->vboSurfaces;
			surf = mdvModel->surfaces;

#ifdef __LODMODEL_INSTANCING__
			GLSL_BindProgram(&tr.instanceVAOShader);
			mdvModel->vao = NULL;
			qglGenVertexArrays(1, &mdvModel->vao);
			qglBindVertexArray(mdvModel->vao);
#endif //__LODMODEL_INSTANCING__

			for (i = 0; i < mdvModel->numSurfaces; i++, vboSurf++, surf++)
			{
				vec3_t *verts;
				vec2_t *texcoords;
				uint32_t *normals;

				byte *data;
				int dataSize;

				int ofs_xyz, ofs_normal, ofs_st;

#ifdef __REGEN_MODEL_NORMALS__
				shader_t *shader = tr.shaders[surf->shaderIndexes[0]];
				if (shader && shader->recalculateNormals)
				{
					GenerateEnhancedNormalsForSurface(surf);
				}
#endif //__REGEN_MODEL_NORMALS__

				dataSize = 0;

				ofs_xyz = dataSize;
				dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*verts);

				ofs_normal = dataSize;
				dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*normals);

				ofs_st = dataSize;
				dataSize += surf->numVerts * sizeof(*texcoords);

				data = (byte *)Z_Malloc(dataSize, TAG_MODEL_MD3, qtrue);

				verts = (vec3_t *)(data + ofs_xyz);
				normals = (uint32_t *)(data + ofs_normal);
				texcoords = (vec2_t *)(data + ofs_st);

				v = surf->verts;
				for (j = 0; j < surf->numVerts * mdvModel->numFrames; j++, v++)
				{
					vec3_t nxt;
					vec4_t tangent;

					VectorCopy(v->xyz, verts[j]);

					normals[j] = R_VboPackNormal(v->normal);
					CrossProduct(v->normal, v->tangent, nxt);
					VectorCopy(v->tangent, tangent);
					tangent[3] = (DotProduct(nxt, v->bitangent) < 0.0f) ? -1.0f : 1.0f;
				}

				st = surf->st;
				for (j = 0; j < surf->numVerts; j++, st++) {
					texcoords[j][0] = st->st[0];
					texcoords[j][1] = st->st[1];
				}

#ifdef __MESH_OPTIMIZATION__
				R_OptimizeMesh((uint32_t *)&surf->numVerts, (uint32_t *)&surf->numIndexes, surf->indexes, verts);
#endif //__MESH_OPTIMIZATION__

				vboSurf->surfaceType = SF_VBO_MDVMESH;
				vboSurf->mdvModel = mdvModel;
				vboSurf->mdvSurface = surf;
				vboSurf->numIndexes = surf->numIndexes;
				vboSurf->numVerts = surf->numVerts;

				vboSurf->minIndex = 0;
				vboSurf->maxIndex = surf->numVerts;

				vboSurf->vbo = R_CreateVBO(data, dataSize, VBO_USAGE_DYNAMIC);

				vboSurf->vbo->ofs_xyz = ofs_xyz;
				vboSurf->vbo->ofs_normal = ofs_normal;
				vboSurf->vbo->ofs_st = ofs_st;

				vboSurf->vbo->stride_xyz = sizeof(*verts);
				vboSurf->vbo->stride_normal = sizeof(*normals);
				vboSurf->vbo->stride_st = sizeof(*st);

				vboSurf->vbo->size_xyz = sizeof(*verts) * surf->numVerts;
				vboSurf->vbo->size_normal = sizeof(*normals) * surf->numVerts;

				Z_Free(data);

				vboSurf->ibo = R_CreateIBO((byte *)surf->indexes, sizeof(glIndex_t) * surf->numIndexes, VBO_USAGE_STATIC);

				ri->Hunk_FreeTempMemory(surf->indexes);
				ri->Hunk_FreeTempMemory(surf->verts);
			}


#ifdef __LODMODEL_INSTANCING__
			qglGenBuffers(1, &tr.instanceVAOShader.instances_buffer);
			qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceVAOShader.instances_buffer);
			qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_POSITION, tr.instanceVAOShader.instances_buffer);

			mdvModel->ofs_instancesPosition = 0;

			qglBindVertexArray(0);
			GLSL_BindProgram(NULL);
#endif //__LODMODEL_INSTANCING__
		}
	}
	catch (exception & e) {
		ri->Printf(PRINT_WARNING, "NIF Error: %s.\n", e.what());
		return qfalse;
	}
	catch (...) {
		ri->Printf(PRINT_WARNING, "NIF Error: Unknown Exception.\n");
		return qfalse;
	}

	return qtrue;
}

#ifdef __NIF_GLM_IMPORT_TEST__
qboolean R_LoadPlayerNIF(model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached)
{
	char stripped[128] = { { 0 } };
	COM_StripExtension(mod_name, stripped, strlen(mod_name));

	return R_LoadNIF(mod, 0, buffer, stripped, 0, "nif");
}
#endif //__NIF_GLM_IMPORT_TEST__

qhandle_t R_RegisterNIF(const char *name, model_t *mod)
{
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded = qfalse;
	int			numLoaded;
	char filename[MAX_QPATH], namebuf[MAX_QPATH + 20];
	char *fext, defex[] = "nif";

	numLoaded = 0;

	strcpy(filename, name);

	fext = strchr(filename, '.');

	if (!fext)
	{
		fext = defex;
	}
	else
	{
		*fext = '\0';
		fext++;
	}

	for (lod = MD3_MAX_LODS - 1; lod >= 0; lod--)
	{
		if (lod)
			Com_sprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		else
			Com_sprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);

		qboolean bAlreadyCached = qfalse;

		int size = CModelCache->LoadFile(namebuf, (void**)&buf, &bAlreadyCached);

		if (!size)
		{
#ifdef __DEBUG_NIF__
			//if (!lod)
			//{// Only output debug info when missing non-lod versions...
			//	ri->Printf(PRINT_WARNING, "R_RegisterNIF: Model %s has zero size. Doesn't exist???\n", namebuf);
			//}
#endif //__DEBUG_NIF__

			continue;
		}

		ident = LittleLong(MD3_IDENT);

		loaded = R_LoadNIF(mod, lod, buf, namebuf, size, fext);

#ifdef __EXPERIMENTAL_ASSIMP_GLM_CONVERSIONS__
		if (loaded && mod->type == MOD_MDXM)
		{// If we loaded a bone model, skip the lod crap...
			numLoaded++;

#ifdef __DEBUG_NIF__
			ri->Printf(PRINT_WARNING, "R_RegisterNIF: loaded assimp boned model %s. modIndex %i. numLods %i. numLoaded %i. numSurfaces %i.\n", name, mod->index, mod->numLods, numLoaded, mod->data.glm->vboModels[0].numVBOMeshes);
#endif //__DEBUG_NIF__

			return mod->index;
		}
#endif //__EXPERIMENTAL_ASSIMP_GLM_CONVERSIONS__

		if (loaded)
		{
			mod->numLods++;
			numLoaded++;
		}
		else
		{
#ifdef __DEBUG_NIF__
			//ri->Printf(PRINT_WARNING, "R_RegisterNIF: Model %s does not exist?\n", namebuf);
#endif //__DEBUG_NIF__
			break;
		}
	}

	if (numLoaded)
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for (lod--; lod >= 0; lod--)
		{
			mod->numLods++;
			mod->data.mdv[lod] = mod->data.mdv[lod + 1];
		}

#ifdef __DEBUG_NIF__
		ri->Printf(PRINT_WARNING, "R_RegisterNIF: loaded assimp model %s. modIndex %i. numLods %i. numLoaded %i. numSurfaces %i.\n", name, mod->index, mod->numLods, numLoaded, (mod->data.mdv && mod->data.mdv[0]) ? mod->data.mdv[0]->numSurfaces : mod->data.glm->vboModels[0].numVBOMeshes);
#endif //__DEBUG_NIF__

		return mod->index;
	}

#ifdef _DEBUG
	ri->Printf(PRINT_WARNING, "R_RegisterNIF: couldn't load %s\n", name);
#endif

	mod->type = MOD_BAD;
	return 0;
}
#endif //__NIF_IMPORT_TEST__

/*
====================
R_RegisterMDR
====================
*/
qhandle_t R_RegisterMDR(const char *name, model_t *mod)
{
	union {
		unsigned *u;
		void *v;
	} buf;
	int	ident;
	qboolean loaded = qfalse;
	int filesize;

	filesize = ri->FS_ReadFile(name, (void **) &buf.v);
	if(!buf.u)
	{
		mod->type = MOD_BAD;
		return 0;
	}
	
	ident = LittleLong(*(unsigned *)buf.u);
	if(ident == MDR_IDENT)
		loaded = R_LoadMDR(mod, buf.u, filesize, name);

	ri->FS_FreeFile (buf.v);
	
	if(!loaded)
	{
		ri->Printf(PRINT_WARNING,"R_RegisterMDR: couldn't load mdr file %s\n", name);
		mod->type = MOD_BAD;
		return 0;
	}
	
	return mod->index;
}

/*
====================
R_RegisterIQM
====================
*/
qhandle_t R_RegisterIQM(const char *name, model_t *mod)
{
	union {
		unsigned *u;
		void *v;
	} buf;
	qboolean loaded = qfalse;
	int filesize;

	filesize = ri->FS_ReadFile(name, (void **) &buf.v);
	if(!buf.u)
	{
		mod->type = MOD_BAD;
		return 0;
	}
	
	loaded = R_LoadIQM(mod, buf.u, filesize, name);

	ri->FS_FreeFile (buf.v);
	
	if(!loaded)
	{
		ri->Printf(PRINT_WARNING,"R_RegisterIQM: couldn't load iqm file %s\n", name);
		mod->type = MOD_BAD;
		return 0;
	}
	
	return mod->index;
}


typedef struct
{
	char *ext;
	qhandle_t (*ModelLoader)( const char *, model_t * );
} modelExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple models of different formats available
static modelExtToLoaderMap_t modelLoaders[ ] =
{
	{ "iqm", R_RegisterIQM },
	{ "mdr", R_RegisterMDR },
	{ "md3", R_RegisterMD3 },
	/* 
	Ghoul 2 Insert Start
	*/
	{ "glm", R_RegisterMD3 },
	{ "gla", R_RegisterMD3 },
	/*
	Ghoul 2 Insert End
	*/
#ifdef __NIF_IMPORT_TEST__
	{ "nif", R_RegisterNIF },
#endif //__NIF_IMPORT_TEST__
	{ "3d", R_RegisterAssImp },
	{ "3ds", R_RegisterAssImp },
	{ "3mf", R_RegisterAssImp },
	{ "ac", R_RegisterAssImp },
	{ "ac3d", R_RegisterAssImp },
	{ "acc", R_RegisterAssImp },
	{ "amj", R_RegisterAssImp },
	{ "ase", R_RegisterAssImp },
	{ "ask", R_RegisterAssImp },
	{ "b3d", R_RegisterAssImp },
	{ "blend", R_RegisterAssImp },
	{ "bvh", R_RegisterAssImp },
	{ "cms", R_RegisterAssImp },
	{ "cob", R_RegisterAssImp },
	{ "collada", R_RegisterAssImp },
	{ "dae", R_RegisterAssImp },
	{ "dxf", R_RegisterAssImp },
	{ "enff", R_RegisterAssImp },
	{ "fbx", R_RegisterAssImp },
	{ "gltf", R_RegisterAssImp },
	{ "hmb", R_RegisterAssImp },
	{ "ifc-step", R_RegisterAssImp },
	{ "irr", R_RegisterAssImp },
	{ "irrmesh", R_RegisterAssImp },
	{ "lwo", R_RegisterAssImp },
	{ "lws", R_RegisterAssImp },
	{ "lxo", R_RegisterAssImp },
	{ "md2", R_RegisterAssImp },
	{ "md3", R_RegisterAssImp },
	{ "md5", R_RegisterAssImp },
	{ "mdc", R_RegisterAssImp },
	{ "mdl", R_RegisterAssImp },
	{ "mesh", R_RegisterAssImp },
	{ "mot", R_RegisterAssImp },
	{ "ms3d", R_RegisterAssImp },
	{ "ndo", R_RegisterAssImp },
	{ "nff", R_RegisterAssImp },
	{ "obj", R_RegisterAssImp },
	{ "off", R_RegisterAssImp },
	{ "ogex", R_RegisterAssImp },
	{ "ply", R_RegisterAssImp },
	{ "pmx", R_RegisterAssImp },
	{ "prj", R_RegisterAssImp },
	{ "q3o", R_RegisterAssImp },
	{ "q3s", R_RegisterAssImp },
	{ "raw", R_RegisterAssImp },
	{ "scn", R_RegisterAssImp },
	{ "sib", R_RegisterAssImp },
	{ "smd", R_RegisterAssImp },
	{ "stp", R_RegisterAssImp },
	{ "stl", R_RegisterAssImp },
	{ "ter", R_RegisterAssImp },
	{ "uc", R_RegisterAssImp },
	{ "vta", R_RegisterAssImp },
	{ "x", R_RegisterAssImp },
	{ "x3d", R_RegisterAssImp },
	{ "xgl", R_RegisterAssImp },
	{ "xml", R_RegisterAssImp },
	{ "zgl", R_RegisterAssImp },
};

static int numModelLoaders = ARRAY_LEN(modelLoaders);

/*
** R_GetModelByHandle
*/
model_t	*R_GetModelByHandle( qhandle_t index ) {
	model_t		*mod;

	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	mod = tr.models[index];

	return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t *R_AllocModel( void ) {
	model_t		*mod;

	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

	mod = (model_t *)ri->Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), h_low );
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t RE_RegisterModel( const char *name ) {
	model_t		*mod;
	qhandle_t	hModel;
	qboolean	orgNameFailed = qfalse;
	int			orgLoader = -1;
	int			i;
	char		localName[ MAX_QPATH ];
	const char	*ext;
	char		altName[ MAX_QPATH ];

	if ( !name || !name[0] ) {
		ri->Printf( PRINT_ALL, "RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		ri->Printf( PRINT_ALL, "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	// search the currently loaded models
	if( ( hModel = CModelCache->SearchLoaded( name ) ) != -1 )
		return hModel;

	if ( name[0] == '*' )
	{
		if ( strcmp (name, "*default.gla") != 0 )
		{
			return 0;
		}
	}

	if( name[0] == '#' )
	{
		// TODO: BSP models
		return 0;
	}

	// allocate a new model_t
	if ( ( mod = R_AllocModel() ) == NULL ) {
		ri->Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

	R_IssuePendingRenderCommands();

	mod->type = MOD_BAD;
	mod->numLods = 0;

	//
	// load the files
	//
	Q_strncpyz( localName, name, MAX_QPATH );

	ext = COM_GetExtension( localName );

	if( *ext )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numModelLoaders; i++ )
		{
			if( !Q_stricmp( ext, modelLoaders[ i ].ext ) )
			{
/*
#ifdef __DEBUG_ASSIMP__
				ri->Printf(PRINT_ALL, "Found supporting model loader for extenstion %s (%s).\n", ext, modelLoaders[i].ext);
#endif //__DEBUG_ASSIMP__
*/

				// Load
				hModel = modelLoaders[ i ].ModelLoader( localName, mod );
				break;
			}
		}

		// A loader was found
		if( i < numModelLoaders )
		{
			if( !hModel )
			{
/*
#ifdef __DEBUG_ASSIMP__
				ri->Printf(PRINT_WARNING, "RE_RegisterModel: Failed to load model filename %s (extension %s). File does not exist? Trying all possible extensions.\n", name, ext[0] ? ext : "NULL");
#endif //__DEBUG_ASSIMP__
*/

				// Loader failed, most likely because the file isn't there;
				// try again without the extension
				orgNameFailed = qtrue;
				orgLoader = i;
				COM_StripExtension( name, localName, MAX_QPATH );
			}
			else
			{
				// Something loaded
				CModelCache->InsertLoaded( name, hModel );
				return mod->index;
			}
		}
	}
/*
#ifdef __DEBUG_ASSIMP__
	else
	{
		ri->Printf(PRINT_WARNING, "RE_RegisterModel: Extension not found in model filename %s. Trying all possible extensions.\n", name);
	}
#endif //__DEBUG_ASSIMP__
*/

	// Try and find a suitable match using all
	// the model formats supported
	for( i = 0; i < numModelLoaders; i++ )
	{
		if (i == orgLoader)
			continue;

		Com_sprintf( altName, sizeof (altName), "%s.%s", localName, modelLoaders[ i ].ext );

		// Load
		hModel = modelLoaders[ i ].ModelLoader( altName, mod );

		if( hModel )
		{
#ifdef __DEVELOPER_MODE__
			if( orgNameFailed )
			{
				ri->Printf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n",
						name, altName );
			}
#endif //__DEVELOPER_MODE__

			break;
		}
	}

	if (!hModel)
	{// Try one more time on the original filename, this time using assimp's built-in format detecion...
		hModel = R_RegisterAssImp(name, mod);
	}

	CModelCache->InsertLoaded( name, hModel );
	return hModel;
}

//rww - Please forgive me for all of the below. Feel free to destroy it and replace it with something better.
//You obviously can't touch anything relating to shaders or ri-> functions here in case a dedicated
//server is running, which is the entire point of having these seperate functions. If anything major
//is changed in the non-server-only versions of these functions it would be wise to incorporate it
//here as well.

/*
=================
R_LoadMDXA_Server - load a Ghoul 2 animation file
=================
*/
qboolean R_LoadMDXA_Server( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {

	mdxaHeader_t		*pinmodel, *mdxa;
	int					version;
	int					size;

 	pinmodel = (mdxaHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//	
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}
	
	if (version != MDXA_VERSION) {
		return qfalse;
	}

	mod->type		= MOD_MDXA;
	mod->dataSize  += size;

	qboolean bAlreadyFound = qfalse;
	mdxa = (mdxaHeader_t*)CModelCache->Allocate( size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLA );
	mod->data.gla = mdxa;

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the 
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri->FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxa == buffer );
//		memcpy( mdxa, buffer, size );	// and don't do this now, since it's the same thing

		LL(mdxa->ident);
		LL(mdxa->version);
		LL(mdxa->numFrames);
		LL(mdxa->numBones);
		LL(mdxa->ofsFrames);
		LL(mdxa->ofsEnd);
	}

 	if ( mdxa->numFrames < 1 ) {
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue;	// All done, stop here, do not LittleLong() etc. Do not pass go...
	}

	return qtrue;
}

/*
=================
R_LoadMDXM_Server - load a Ghoul 2 Mesh file
=================
*/
qboolean R_LoadMDXM_Server( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {
	int					i,l, j;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*lod;
	mdxmSurface_t		*surf;
	int					version;
	int					size;
	//shader_t			*sh;
	mdxmSurfHierarchy_t	*surfInfo;
    
	pinmodel= (mdxmHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//	
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}

	if (version != MDXM_VERSION) {
		return qfalse;
	}

	mod->type	   = MOD_MDXM;
	mod->dataSize += size;	
	
	qboolean bAlreadyFound = qfalse;
	mdxm = (mdxmHeader_t*)CModelCache->Allocate( size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLM );
	mod->data.glm = (mdxmData_t *)ri->Hunk_Alloc (sizeof (mdxmData_t), h_low);
	mod->data.glm->header = mdxm;

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the 
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri->FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxm == buffer );
//		memcpy( mdxm, buffer, size );	// and don't do this now, since it's the same thing

		LL(mdxm->ident);
		LL(mdxm->version);
		LL(mdxm->numLODs);
		LL(mdxm->ofsLODs);
		LL(mdxm->numSurfaces);
		LL(mdxm->ofsSurfHierarchy);
		LL(mdxm->ofsEnd);
	}
		
	// first up, go load in the animation file we need that has the skeletal animation info for this model
	mdxm->animIndex = RE_RegisterServerModel(va ("%s.gla",mdxm->animName));
	if (!mdxm->animIndex) 
	{
		return qfalse;
	}

	mod->numLods = mdxm->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not LittleLong(), do not pass Go...
	}

	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mdxm + mdxm->ofsSurfHierarchy);
 	for ( i = 0 ; i < mdxm->numSurfaces ; i++) 
	{
		LL(surfInfo->numChildren);
		LL(surfInfo->parentIndex);

		// do all the children indexs
		for (j=0; j<surfInfo->numChildren; j++)
		{
			LL(surfInfo->childIndexes[j]);
		}

		// We will not be using shaders on the server.
		//sh = 0;
		// insert it in the surface list
		
		surfInfo->shaderIndex = 0;

		CModelCache->StoreShaderRequest(mod_name, &surfInfo->shader[0], &surfInfo->shaderIndex);

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (intptr_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
  	}
	
	// swap all the LOD's	(we need to do the middle part of this even for intel, because of shader reg and err-check)
	lod = (mdxmLOD_t *) ( (byte *)mdxm + mdxm->ofsLODs );
	for ( l = 0 ; l < mdxm->numLODs ; l++)
	{
		int	triCount = 0;

		LL(lod->ofsEnd);
		// swap all the surfaces
		surf = (mdxmSurface_t *) ( (byte *)lod + sizeof (mdxmLOD_t) + (mdxm->numSurfaces * sizeof(mdxmLODSurfOffset_t)) );
		for ( i = 0 ; i < mdxm->numSurfaces ; i++) 
		{
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);
			LL(surf->ofsHeader);
			LL(surf->numBoneReferences);
			LL(surf->ofsBoneReferences);
//			LL(surf->maxVertBoneWeights);

			triCount += surf->numTriangles;
										
			if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
				return qfalse;
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				return qfalse;
			}
		
			// change to surface identifier
			surf->ident = SF_MDX;

			// register the shaders

			// find the next surface
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = (mdxmLOD_t *)( (byte *)lod + lod->ofsEnd );
	}

	return qtrue;
}

/*
====================
R_RegisterMDX_Server
====================
*/
qhandle_t R_RegisterMDX_Server(const char *name, model_t *mod)
{
	unsigned	*buf;
	int			lod;
	int			ident;
	qboolean	loaded = qfalse;
	int			numLoaded;
	char filename[MAX_QPATH], namebuf[MAX_QPATH+20];
	char *fext, defex[] = "md3";

	numLoaded = 0;

	strcpy(filename, name);

	fext = strchr(filename, '.');
	if(!fext)
		fext = defex;
	else
	{
		*fext = '\0';
		fext++;
	}

	for (lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod--)
	{
		if(lod)
			Com_sprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		else
			Com_sprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);

		qboolean bAlreadyCached = qfalse;
		if( !CModelCache->LoadFile( namebuf, (void**)&buf, &bAlreadyCached ) )
			continue;

		ident = *(unsigned *)buf;
		if( !bAlreadyCached )
			ident = LittleLong(ident);
		
		switch(ident)
		{
			case MDXA_IDENT:
				loaded = R_LoadMDXA_Server(mod, buf, namebuf, bAlreadyCached);
				break;
			case MDXM_IDENT:
				loaded = R_LoadMDXM_Server(mod, buf, namebuf, bAlreadyCached);
				break;
			default:
				//ri->Printf(PRINT_WARNING, "R_RegisterMDX_Server: unknown ident for %s\n", name);
				break;
		}

		if(loaded)
		{
			mod->numLods++;
			numLoaded++;
		}
		else
			break;
	}

	if(numLoaded)
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for(lod--; lod >= 0; lod--)
		{
			mod->numLods++;
			mod->data.mdv[lod] = mod->data.mdv[lod + 1];
		}

		return mod->index;
	}

/*#ifdef _DEBUG
	ri->Printf(PRINT_WARNING,"R_RegisterMDX_Server: couldn't load %s\n", name);
#endif*/

	mod->type = MOD_BAD;
	return 0;
}

// Note that the ordering indicates the order of preference used
// when there are multiple models of different formats available
static modelExtToLoaderMap_t serverModelLoaders[ ] =
{
	/* 
	Ghoul 2 Insert Start
	*/
	{ "glm", R_RegisterMDX_Server },
	{ "gla", R_RegisterMDX_Server },
	/*
	Ghoul 2 Insert End
	*/
};

static int numServerModelLoaders = ARRAY_LEN(serverModelLoaders);

qhandle_t RE_RegisterServerModel( const char *name ) {
	model_t		*mod;
	qhandle_t	hModel;
	int			i;
	char		localName[ MAX_QPATH ];
	const char	*ext;

	if (!r_noServerGhoul2)
	{ //keep it from choking when it gets to these checks in the g2 code. Registering all r_ cvars for the server would be a Bad Thing though.
		r_noServerGhoul2 = ri->Cvar_Get( "r_noserverghoul2", "0", 0);
	}

	if ( !name || !name[0] ) {
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		return 0;
	}

	// search the currently loaded models
	if( ( hModel = CModelCache->SearchLoaded( name ) ) != -1 )
		return hModel;

	if ( name[0] == '*' )
	{
		if ( strcmp (name, "*default.gla") != 0 )
		{
			return 0;
		}
	}

	// allocate a new model_t
	if ( ( mod = R_AllocModel() ) == NULL ) {
		ri->Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

	R_IssuePendingRenderCommands();

	mod->type = MOD_BAD;
	mod->numLods = 0;

	//
	// load the files
	//
	Q_strncpyz( localName, name, MAX_QPATH );

	ext = COM_GetExtension( localName );

	if( *ext )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numServerModelLoaders; i++ )
		{
			if( !Q_stricmp( ext, serverModelLoaders[ i ].ext ) )
			{
				// Load
				hModel = serverModelLoaders[ i ].ModelLoader( localName, mod );
				break;
			}
		}

		// A loader was found
		if( i < numServerModelLoaders )
		{
			if( hModel )
			{
				// Something loaded
				CModelCache->InsertLoaded( name, hModel );
				return mod->index;
			}
		}
	}

	CModelCache->InsertLoaded( name, hModel );
	return hModel;
}

/*
=================
R_LoadMD3
=================
*/

#ifdef __INSTANCED_MODELS__
#include "tr_instancing.h"
#endif //__INSTANCED_MODELS__

static qboolean R_LoadMD3(model_t * mod, int lod, void *buffer, const char *modName)
{
	int             f, i, j;

	md3Header_t    *md3Model;
	md3Frame_t     *md3Frame;
	md3Surface_t   *md3Surf;
	md3Shader_t    *md3Shader;
	md3Triangle_t  *md3Tri;
	md3St_t        *md3st;
	md3XyzNormal_t *md3xyz;
	md3Tag_t       *md3Tag;

	mdvModel_t     *mdvModel;
	mdvFrame_t     *frame;
	mdvSurface_t   *surf;//, *surface;
	int            *shaderIndex;
	glIndex_t	   *tri;
	mdvVertex_t    *v;
	mdvSt_t        *st;
	mdvTag_t       *tag;
	mdvTagName_t   *tagName;

	int             version;
	int             size;

	std::string basePath = AssImp_getBasePath(modName);

	md3Model = (md3Header_t *) buffer;

	version = LittleLong(md3Model->version);
	if(version != MD3_VERSION)
	{
		ri->Printf(PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n", modName, version, MD3_VERSION);
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LittleLong(md3Model->ofsEnd);
	mod->dataSize += size;
	//mdvModel = mod->mdv[lod] = (mdvModel_t *)ri->Hunk_Alloc(sizeof(mdvModel_t), h_low);
	qboolean bAlreadyFound = qfalse;
	mdvModel = mod->data.mdv[lod] = (mdvModel_t *)CModelCache->Allocate( size, buffer, modName, &bAlreadyFound, TAG_MODEL_MD3 );

//  Com_Memcpy(mod->md3[lod], buffer, LittleLong(md3Model->ofsEnd));
	if( !bAlreadyFound )
	{	// HACK
		LL(md3Model->ident);
		LL(md3Model->version);
		LL(md3Model->numFrames);
		LL(md3Model->numTags);
		LL(md3Model->numSurfaces);
		LL(md3Model->ofsFrames);
		LL(md3Model->ofsTags);
		LL(md3Model->ofsSurfaces);
		LL(md3Model->ofsEnd);
	}
	else
	{
		CModelCache->AllocateShaders( modName );
	}

	if(md3Model->numFrames < 1)
	{
		ri->Printf(PRINT_WARNING, "R_LoadMD3: %s has no frames\n", modName);
		return qfalse;
	}

	// swap all the frames
	mdvModel->numFrames = md3Model->numFrames;
	mdvModel->frames = frame = (mdvFrame_t *)ri->Hunk_Alloc(sizeof(*frame) * md3Model->numFrames, h_low);

	md3Frame = (md3Frame_t *) ((byte *) md3Model + md3Model->ofsFrames);
	for(i = 0; i < md3Model->numFrames; i++, frame++, md3Frame++)
	{
		frame->radius = LittleFloat(md3Frame->radius);
		for(j = 0; j < 3; j++)
		{
			frame->bounds[0][j] = LittleFloat(md3Frame->bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(md3Frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(md3Frame->localOrigin[j]);
		}
	}

	// swap all the tags
	mdvModel->numTags = md3Model->numTags;
	mdvModel->tags = tag = (mdvTag_t *)ri->Hunk_Alloc(sizeof(*tag) * (md3Model->numTags * md3Model->numFrames), h_low);

	md3Tag = (md3Tag_t *) ((byte *) md3Model + md3Model->ofsTags);
	for(i = 0; i < md3Model->numTags * md3Model->numFrames; i++, tag++, md3Tag++)
	{
		for(j = 0; j < 3; j++)
		{
			tag->origin[j] = LittleFloat(md3Tag->origin[j]);
			tag->axis[0][j] = LittleFloat(md3Tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(md3Tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(md3Tag->axis[2][j]);
		}
	}


	mdvModel->tagNames = tagName = (mdvTagName_t *)ri->Hunk_Alloc(sizeof(*tagName) * (md3Model->numTags), h_low);

	md3Tag = (md3Tag_t *) ((byte *) md3Model + md3Model->ofsTags);
	for(i = 0; i < md3Model->numTags; i++, tagName++, md3Tag++)
	{
		Q_strncpyz(tagName->name, md3Tag->name, sizeof(tagName->name));
	}

	// swap all the surfaces
	mdvModel->numSurfaces = md3Model->numSurfaces;
	mdvModel->surfaces = surf = (mdvSurface_t *)ri->Hunk_Alloc(sizeof(*surf) * md3Model->numSurfaces, h_low);

	md3Surf = (md3Surface_t *) ((byte *) md3Model + md3Model->ofsSurfaces);
	for(i = 0; i < md3Model->numSurfaces; i++)
	{
		LL(md3Surf->ident);
		LL(md3Surf->flags);
		LL(md3Surf->numFrames);
		LL(md3Surf->numShaders);
		LL(md3Surf->numTriangles);
		LL(md3Surf->ofsTriangles);
		LL(md3Surf->numVerts);
		LL(md3Surf->ofsShaders);
		LL(md3Surf->ofsSt);
		LL(md3Surf->ofsXyzNormals);
		LL(md3Surf->ofsEnd);

#if 0
		if(md3Surf->numVerts >= SHADER_MAX_VERTEXES)
		{
			ri->Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i verts on %s (%i).\n",
				modName, SHADER_MAX_VERTEXES - 1, md3Surf->name[0] ? md3Surf->name : "a surface",
				md3Surf->numVerts );
			return qfalse;
		}
		if(md3Surf->numTriangles * 3 >= SHADER_MAX_INDEXES)
		{
			ri->Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i triangles on %s (%i).\n",
				modName, ( SHADER_MAX_INDEXES / 3 ) - 1, md3Surf->name[0] ? md3Surf->name : "a surface",
				md3Surf->numTriangles );
			return qfalse;
		}
#endif

		// change to surface identifier
		surf->surfaceType = SF_MDV;

		// give pointer to model for Tess_SurfaceMDX
		surf->model = mdvModel;

		// copy surface name
		Q_strncpyz(surf->name, md3Surf->name, sizeof(surf->name));

		// lowercase the surface name so skin compares are faster
		Q_strlwr(surf->name);

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen(surf->name);
		if(j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// register the shaders
		surf->numShaderIndexes = md3Surf->numShaders;
		surf->shaderIndexes = shaderIndex = (int *)ri->Hunk_Alloc(sizeof(*shaderIndex) * md3Surf->numShaders, h_low);

		md3Shader = (md3Shader_t *) ((byte *) md3Surf + md3Surf->ofsShaders);
		for(j = 0; j < md3Surf->numShaders; j++, shaderIndex++, md3Shader++)
		{
			shader_t       *sh;

#if 1 // Search for missing textures...
			std::string textureName = md3Shader->name;
			
			std::string finalPath = R_FindAndAdjustShaderNames(modName, md3Surf->name, textureName);
			sh = R_FindShader(finalPath.c_str(), lightmapsNone, stylesDefault, qtrue);

			if (sh == NULL || sh == tr.defaultShader)
			{
#ifdef __DEBUG_ASSIMP__
				ri->Printf(PRINT_ALL, "Model: %s. Mesh: %s (%i). Missing texture: %s.\n", modName, (md3Surf->name && md3Surf->name[0]) ? md3Surf->name : "NULL", i, finalPath.length() ? finalPath.c_str() : "NULL");
#endif //__DEBUG_ASSIMP__
				sh = tr.defaultShader;
			}
#else
			sh = R_FindShader(md3Shader->name, lightmapsNone, stylesDefault, qtrue);
#endif

			if(sh->defaultShader)
			{
				*shaderIndex = 0;
			}
			else
			{
				*shaderIndex = sh->index;
			}
		}

		// swap all the triangles
		surf->numIndexes = md3Surf->numTriangles * 3;
		surf->indexes = tri = (glIndex_t *)ri->Hunk_AllocateTempMemory(sizeof(*tri) * 3 * md3Surf->numTriangles);

		md3Tri = (md3Triangle_t *) ((byte *) md3Surf + md3Surf->ofsTriangles);
		for(j = 0; j < md3Surf->numTriangles; j++, tri += 3, md3Tri++)
		{
			tri[0] = LittleLong(md3Tri->indexes[0]);
			tri[1] = LittleLong(md3Tri->indexes[1]);
			tri[2] = LittleLong(md3Tri->indexes[2]);
		}

		// swap all the XyzNormals
		surf->numVerts = md3Surf->numVerts;
		surf->verts = v = (mdvVertex_t *)ri->Hunk_AllocateTempMemory(sizeof(*v) * (md3Surf->numVerts * md3Surf->numFrames));

		md3xyz = (md3XyzNormal_t *) ((byte *) md3Surf + md3Surf->ofsXyzNormals);
		for(j = 0; j < md3Surf->numVerts * md3Surf->numFrames; j++, md3xyz++, v++)
		{
			unsigned lat, lng;
			unsigned short normal;

			v->xyz[0] = LittleShort(md3xyz->xyz[0]) * MD3_XYZ_SCALE;
			v->xyz[1] = LittleShort(md3xyz->xyz[1]) * MD3_XYZ_SCALE;
			v->xyz[2] = LittleShort(md3xyz->xyz[2]) * MD3_XYZ_SCALE;

			normal = LittleShort(md3xyz->normal);

			lat = ( normal >> 8 ) & 0xff;
			lng = ( normal & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			v->normal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			v->normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			v->normal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}

		// swap all the ST
		surf->st = st = (mdvSt_t *)ri->Hunk_Alloc(sizeof(*st) * md3Surf->numVerts, h_low);

		md3st = (md3St_t *) ((byte *) md3Surf + md3Surf->ofsSt);
		for(j = 0; j < md3Surf->numVerts; j++, md3st++, st++)
		{
			st->st[0] = LittleFloat(md3st->st[0]);
			st->st[1] = LittleFloat(md3st->st[1]);
		}

		// calc tangent spaces
		{
			for(j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++)
			{
				VectorClear(v->tangent);
				VectorClear(v->bitangent);
			}

			for(f = 0; f < mdvModel->numFrames; f++)
			{
				for(j = 0, tri = surf->indexes; j < surf->numIndexes; j += 3, tri += 3)
				{
					vec3_t sdir, tdir;
					const float *v0, *v1, *v2, *t0, *t1, *t2;
					glIndex_t index0, index1, index2;

					index0 = surf->numVerts * f + tri[0];
					index1 = surf->numVerts * f + tri[1];
					index2 = surf->numVerts * f + tri[2];

					v0 = surf->verts[index0].xyz;
					v1 = surf->verts[index1].xyz;
					v2 = surf->verts[index2].xyz;

					t0 = surf->st[tri[0]].st;
					t1 = surf->st[tri[1]].st;
					t2 = surf->st[tri[2]].st;

					R_CalcTexDirs(sdir, tdir, v0, v1, v2, t0, t1, t2);

					VectorAdd(sdir, surf->verts[index0].tangent,   surf->verts[index0].tangent);
					VectorAdd(sdir, surf->verts[index1].tangent,   surf->verts[index1].tangent);
					VectorAdd(sdir, surf->verts[index2].tangent,   surf->verts[index2].tangent);
					VectorAdd(tdir, surf->verts[index0].bitangent, surf->verts[index0].bitangent);
					VectorAdd(tdir, surf->verts[index1].bitangent, surf->verts[index1].bitangent);
					VectorAdd(tdir, surf->verts[index2].bitangent, surf->verts[index2].bitangent);
				}
			}

			for(j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++)
			{
				vec3_t sdir, tdir;

				VectorCopy(v->tangent,   sdir);
				VectorCopy(v->bitangent, tdir);

				VectorNormalize(sdir);
				VectorNormalize(tdir);

				R_CalcTbnFromNormalAndTexDirs(v->tangent, v->bitangent, v->normal, sdir, tdir);
			}
		}

		// find the next surface
		md3Surf = (md3Surface_t *) ((byte *) md3Surf + md3Surf->ofsEnd);
		surf++;
	}

	{
		srfVBOMDVMesh_t *vboSurf;

		mdvModel->numVBOSurfaces = mdvModel->numSurfaces;
		mdvModel->vboSurfaces = (srfVBOMDVMesh_t *)ri->Hunk_Alloc(sizeof(*mdvModel->vboSurfaces) * mdvModel->numSurfaces, h_low);

		vboSurf = mdvModel->vboSurfaces;
		surf = mdvModel->surfaces;

		/*
#ifdef __INSTANCED_MODELS__
		GLSL_BindProgram(&tr.instanceShader);
		mdvModel->vao = NULL;
		qglGenVertexArrays(1, &mdvModel->vao);
		qglBindVertexArray(mdvModel->vao);
#endif //__INSTANCED_MODELS__
		*/

#ifdef __LODMODEL_INSTANCING__
		GLSL_BindProgram(&tr.instanceVAOShader);
		mdvModel->vao = NULL;
		qglGenVertexArrays(1, &mdvModel->vao);
		qglBindVertexArray(mdvModel->vao);
#endif //__INSTANCED_MODELS__

		for (i = 0; i < mdvModel->numSurfaces; i++, vboSurf++, surf++)
		{
			vec3_t *verts;
			vec2_t *texcoords;
			uint32_t *normals;

			byte *data;
			int dataSize;

			int ofs_xyz, ofs_normal, ofs_st;

#ifdef __REGEN_MODEL_NORMALS__
			shader_t *shader = tr.shaders[surf->shaderIndexes[0]];
			if (shader && shader->recalculateNormals)
			{
				GenerateEnhancedNormalsForSurface(surf);
			}
#endif //__REGEN_MODEL_NORMALS__

			dataSize = 0;

			ofs_xyz = dataSize;
			dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*verts);

			ofs_normal = dataSize;
			dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*normals);

			ofs_st = dataSize;
			dataSize += surf->numVerts * sizeof(*texcoords);

			data = (byte *)Z_Malloc(dataSize, TAG_MODEL_MD3, qtrue);

			verts =      (vec3_t *)(data + ofs_xyz);
			normals =    (uint32_t *)(data + ofs_normal);
			texcoords =  (vec2_t *)(data + ofs_st);
		
			v = surf->verts;
			for ( j = 0; j < surf->numVerts * mdvModel->numFrames ; j++, v++ )
			{
				vec3_t nxt;
				vec4_t tangent;

				VectorCopy(v->xyz,       verts[j]);

				normals[j] = R_VboPackNormal(v->normal);
				CrossProduct(v->normal, v->tangent, nxt);
				VectorCopy(v->tangent, tangent);
				tangent[3] = (DotProduct(nxt, v->bitangent) < 0.0f) ? -1.0f : 1.0f;
			}

			st = surf->st;
			for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
				texcoords[j][0] = st->st[0];
				texcoords[j][1] = st->st[1];
			}

#ifdef __MESH_OPTIMIZATION__
			R_OptimizeMesh((uint32_t *)&surf->numVerts, (uint32_t *)&surf->numIndexes, surf->indexes, (vec3_t *)(data + ofs_xyz));
#endif //__MESH_OPTIMIZATION__

			vboSurf->surfaceType = SF_VBO_MDVMESH;
			vboSurf->mdvModel = mdvModel;
			vboSurf->mdvSurface = surf;
			vboSurf->numIndexes = surf->numIndexes;
			vboSurf->numVerts = surf->numVerts;
			
			vboSurf->minIndex = 0;
			vboSurf->maxIndex = surf->numVerts;

/*#ifdef __INSTANCED_MODELS__
			if (mdvModel->numFrames <= 1)
				vboSurf->vbo = R_CreateVBO(data, dataSize, VBO_USAGE_STATIC);
			else
#endif //__INSTANCED_MODELS__*/
			vboSurf->vbo = R_CreateVBO(data, dataSize, VBO_USAGE_DYNAMIC);

			vboSurf->vbo->ofs_xyz       = ofs_xyz;
			vboSurf->vbo->ofs_normal    = ofs_normal;
			vboSurf->vbo->ofs_st        = ofs_st;

			vboSurf->vbo->stride_xyz       = sizeof(*verts);
			vboSurf->vbo->stride_normal    = sizeof(*normals);
			vboSurf->vbo->stride_st        = sizeof(*st);

			vboSurf->vbo->size_xyz    = sizeof(*verts) * surf->numVerts;
			vboSurf->vbo->size_normal = sizeof(*normals) * surf->numVerts;

			Z_Free(data);

			vboSurf->ibo = R_CreateIBO ((byte *)surf->indexes, sizeof (glIndex_t) * surf->numIndexes, VBO_USAGE_STATIC);

			ri->Hunk_FreeTempMemory(surf->indexes);
			ri->Hunk_FreeTempMemory(surf->verts);
		}

		/*
#ifdef __INSTANCED_MODELS__
		qglGenBuffers(1, &tr.instanceShader.instances_buffer);
		qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_buffer);
		qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_POSITION, tr.instanceShader.instances_buffer);
		qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t), NULL, GL_STREAM_DRAW);

		qglGenBuffers(1, &tr.instanceShader.instances_mvp);
		qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_mvp);
		qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_MVP, tr.instanceShader.instances_mvp);
		qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(matrix_t), NULL, GL_STREAM_DRAW);

		mdvModel->ofs_instancesPosition = 0;
		mdvModel->ofs_instancesMVP = MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t);

		qglBindVertexArray(0);
		GLSL_BindProgram(NULL);
#endif //__INSTANCED_MODELS__
		*/

#ifdef __LODMODEL_INSTANCING__
		qglGenBuffers(1, &tr.instanceVAOShader.instances_buffer);
		qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceVAOShader.instances_buffer);
		qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_POSITION, tr.instanceVAOShader.instances_buffer);
		//qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t), NULL, GL_STREAM_DRAW);

		//qglGenBuffers(1, &tr.instanceShader.instances_mvp);
		//qglBindBuffer(GL_ARRAY_BUFFER, tr.instanceShader.instances_mvp);
		//qglBindBufferBase(GL_ARRAY_BUFFER, ATTR_INDEX_INSTANCES_MVP, tr.instanceShader.instances_mvp);
		//qglBufferData(GL_ARRAY_BUFFER, MAX_INSTANCED_MODEL_INSTANCES * sizeof(matrix_t), NULL, GL_STREAM_DRAW);

		mdvModel->ofs_instancesPosition = 0;
		//mdvModel->ofs_instancesMVP = MAX_INSTANCED_MODEL_INSTANCES * sizeof(vec3_t);

		qglBindVertexArray(0);
		GLSL_BindProgram(NULL);
#endif //__LODMODEL_INSTANCING__
	}

	return qtrue;
}



/*
=================
R_LoadMDR
=================
*/
static qboolean R_LoadMDR( model_t *mod, void *buffer, int filesize, const char *mod_name ) 
{
	int					i, j, k, l;
	mdrHeader_t			*pinmodel, *mdr;
	mdrFrame_t			*frame;
	mdrLOD_t			*lod, *curlod;
	mdrSurface_t			*surf, *cursurf;
	mdrTriangle_t			*tri, *curtri;
	mdrVertex_t			*v, *curv;
	mdrWeight_t			*weight, *curweight;
	mdrTag_t			*tag, *curtag;
	int					size;
	shader_t			*sh;

	pinmodel = (mdrHeader_t *)buffer;

	pinmodel->version = LittleLong(pinmodel->version);
	if (pinmodel->version != MDR_VERSION) 
	{
		ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has wrong version (%i should be %i)\n", mod_name, pinmodel->version, MDR_VERSION);
		return qfalse;
	}

	size = LittleLong(pinmodel->ofsEnd);
	
	if(size > filesize)
	{
		ri->Printf(PRINT_WARNING, "R_LoadMDR: Header of %s is broken. Wrong filesize declared!\n", mod_name);
		return qfalse;
	}
	
	mod->type = MOD_MDR;

	LL(pinmodel->numFrames);
	LL(pinmodel->numBones);
	LL(pinmodel->ofsFrames);

	// This is a model that uses some type of compressed Bones. We don't want to uncompress every bone for each rendered frame
	// over and over again, we'll uncompress it in this function already, so we must adjust the size of the target mdr.
	if(pinmodel->ofsFrames < 0)
	{
		// mdrFrame_t is larger than mdrCompFrame_t:
		size += pinmodel->numFrames * sizeof(frame->name);
		// now add enough space for the uncompressed bones.
		size += pinmodel->numFrames * pinmodel->numBones * ((sizeof(mdrBone_t) - sizeof(mdrCompBone_t)));
	}
	
	// simple bounds check
	if(pinmodel->numBones < 0 ||
		sizeof(*mdr) + pinmodel->numFrames * (sizeof(*frame) + (pinmodel->numBones - 1) * sizeof(*frame->bones)) > size)
	{
		ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
		return qfalse;
	}

	mod->dataSize += size;
	mod->data.mdr = mdr = (mdrHeader_t*)ri->Hunk_Alloc( size, h_low );

	// Copy all the values over from the file and fix endian issues in the process, if necessary.
	
	mdr->ident = LittleLong(pinmodel->ident);
	mdr->version = pinmodel->version;	// Don't need to swap byte order on this one, we already did above.
	Q_strncpyz(mdr->name, pinmodel->name, sizeof(mdr->name));
	mdr->numFrames = pinmodel->numFrames;
	mdr->numBones = pinmodel->numBones;
	mdr->numLODs = LittleLong(pinmodel->numLODs);
	mdr->numTags = LittleLong(pinmodel->numTags);
	// We don't care about the other offset values, we'll generate them ourselves while loading.

	mod->numLods = mdr->numLODs;

	if ( mdr->numFrames < 1 ) 
	{
		ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has no frames\n", mod_name);
		return qfalse;
	}

	/* The first frame will be put into the first free space after the header */
	frame = (mdrFrame_t *)(mdr + 1);
	mdr->ofsFrames = (int)((byte *) frame - (byte *) mdr);
		
	if (pinmodel->ofsFrames < 0)
	{
		mdrCompFrame_t *cframe;
				
		// compressed model...				
		cframe = (mdrCompFrame_t *)((byte *) pinmodel - pinmodel->ofsFrames);
		
		for(i = 0; i < mdr->numFrames; i++)
		{
			for(j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(cframe->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(cframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(cframe->localOrigin[j]);
			}

			frame->radius = LittleFloat(cframe->radius);
			frame->name[0] = '\0';	// No name supplied in the compressed version.
			
			for(j = 0; j < mdr->numBones; j++)
			{
				for(k = 0; k < (sizeof(cframe->bones[j].Comp) / 2); k++)
				{
					// Do swapping for the uncompressing functions. They seem to use shorts
					// values only, so I assume this will work. Never tested it on other
					// platforms, though.
					
					((unsigned short *)(cframe->bones[j].Comp))[k] =
						LittleShort( ((unsigned short *)(cframe->bones[j].Comp))[k] );
				}
				
				/* Now do the actual uncompressing */
				MC_UnCompress(frame->bones[j].matrix, cframe->bones[j].Comp);
			}
			
			// Next Frame...
			cframe = (mdrCompFrame_t *) &cframe->bones[j];
			frame = (mdrFrame_t *) &frame->bones[j];
		}
	}
	else
	{
		mdrFrame_t *curframe;
		
		// uncompressed model...
		//
    
		curframe = (mdrFrame_t *)((byte *) pinmodel + pinmodel->ofsFrames);
		
		// swap all the frames
		for ( i = 0 ; i < mdr->numFrames ; i++) 
		{
			for(j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(curframe->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(curframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(curframe->localOrigin[j]);
			}
			
			frame->radius = LittleFloat(curframe->radius);
			Q_strncpyz(frame->name, curframe->name, sizeof(frame->name));
			
			for (j = 0; j < (int) (mdr->numBones * sizeof(mdrBone_t) / 4); j++) 
			{
				((float *)frame->bones)[j] = LittleFloat( ((float *)curframe->bones)[j] );
			}
			
			curframe = (mdrFrame_t *) &curframe->bones[mdr->numBones];
			frame = (mdrFrame_t *) &frame->bones[mdr->numBones];
		}
	}
	
	// frame should now point to the first free address after all frames.
	lod = (mdrLOD_t *) frame;
	mdr->ofsLODs = (int) ((byte *) lod - (byte *)mdr);
	
	curlod = (mdrLOD_t *)((byte *) pinmodel + LittleLong(pinmodel->ofsLODs));
		
	// swap all the LOD's
	for ( l = 0 ; l < mdr->numLODs ; l++)
	{
		// simple bounds check
		if((byte *) (lod + 1) > (byte *) mdr + size)
		{
			ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
			return qfalse;
		}

		lod->numSurfaces = LittleLong(curlod->numSurfaces);
		
		// swap all the surfaces
		surf = (mdrSurface_t *) (lod + 1);
		lod->ofsSurfaces = (int)((byte *) surf - (byte *) lod);
		cursurf = (mdrSurface_t *) ((byte *)curlod + LittleLong(curlod->ofsSurfaces));
		
		for ( i = 0 ; i < lod->numSurfaces ; i++)
		{
			// simple bounds check
			if((byte *) (surf + 1) > (byte *) mdr + size)
			{
				ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
				return qfalse;
			}

			// first do some copying stuff
			
			surf->ident = SF_MDR;
			Q_strncpyz(surf->name, cursurf->name, sizeof(surf->name));
			Q_strncpyz(surf->shader, cursurf->shader, sizeof(surf->shader));
			
			surf->ofsHeader = (byte *) mdr - (byte *) surf;
			
			surf->numVerts = LittleLong(cursurf->numVerts);
			surf->numTriangles = LittleLong(cursurf->numTriangles);
			// numBoneReferences and BoneReferences generally seem to be unused
			
#if 0
			// now do the checks that may fail.
			if ( surf->numVerts >= SHADER_MAX_VERTEXES ) 
			{
				ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i verts on %s (%i).\n",
					  mod_name, SHADER_MAX_VERTEXES-1, surf->name[0] ? surf->name : "a surface",
					  surf->numVerts );
				return qfalse;
			}
			if ( surf->numTriangles*3 >= SHADER_MAX_INDEXES ) 
			{
				ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has more than %i triangles on %s (%i).\n",
					  mod_name, ( SHADER_MAX_INDEXES / 3 ) - 1, surf->name[0] ? surf->name : "a surface",
					  surf->numTriangles );
				return qfalse;
			}
#endif

			// lowercase the surface name so skin compares are faster
			Q_strlwr( surf->name );

			// register the shaders
#if 1 // Search for missing textures...
			std::string textureName = surf->shader;

			std::string finalPath = R_FindAndAdjustShaderNames(mod_name, surf->name, textureName);
			sh = R_FindShader(finalPath.c_str(), lightmapsNone, stylesDefault, qtrue);

			if ((sh && (sh == tr.defaultShader || (sh->surfaceFlags & SURF_NODRAW)))
				|| StringContainsWord(sh->name, "collision")
				|| StringContainsWord(sh->name, "nodraw")
				|| StringContainsWord(sh->name, "noshader"))
			{// A collision surface, set it to nodraw... TODO: Generate planes?!?!?!?
				continue;
			}

			if (sh == NULL || sh == tr.defaultShader)
			{
#ifdef __DEBUG_ASSIMP__
				ri->Printf(PRINT_ALL, "Model: %s. Mesh: %s (%i). Missing texture: %s.\n", mod_name, (surf->name && surf->name[0]) ? surf->name : "NULL", i, finalPath.length() ? finalPath.c_str() : "NULL");
#endif //__DEBUG_ASSIMP__
				sh = tr.defaultShader;
			}
#else
			sh = R_FindShader(surf->shader, lightmapsNone, stylesDefault, qtrue);
#endif

			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
			
			// now copy the vertexes.
			v = (mdrVertex_t *) (surf + 1);
			surf->ofsVerts = (int)((byte *) v - (byte *) surf);
			curv = (mdrVertex_t *) ((byte *)cursurf + LittleLong(cursurf->ofsVerts));
			
			for(j = 0; j < surf->numVerts; j++)
			{
				LL(curv->numWeights);
			
				// simple bounds check
				if(curv->numWeights < 0 || (byte *) (v + 1) + (curv->numWeights - 1) * sizeof(*weight) > (byte *) mdr + size)
				{
					ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
					return qfalse;
				}

				v->normal[0] = LittleFloat(curv->normal[0]);
				v->normal[1] = LittleFloat(curv->normal[1]);
				v->normal[2] = LittleFloat(curv->normal[2]);
				
				v->texCoords[0] = LittleFloat(curv->texCoords[0]);
				v->texCoords[1] = LittleFloat(curv->texCoords[1]);
				
				v->numWeights = curv->numWeights;
				weight = &v->weights[0];
				curweight = &curv->weights[0];
				
				// Now copy all the weights
				for(k = 0; k < v->numWeights; k++)
				{
					weight->boneIndex = LittleLong(curweight->boneIndex);
					weight->boneWeight = LittleFloat(curweight->boneWeight);
					
					weight->offset[0] = LittleFloat(curweight->offset[0]);
					weight->offset[1] = LittleFloat(curweight->offset[1]);
					weight->offset[2] = LittleFloat(curweight->offset[2]);
					
					weight++;
					curweight++;
				}
				
				v = (mdrVertex_t *) weight;
				curv = (mdrVertex_t *) curweight;
			}
						
			// we know the offset to the triangles now:
			tri = (mdrTriangle_t *) v;
			surf->ofsTriangles = (int)((byte *) tri - (byte *) surf);
			curtri = (mdrTriangle_t *)((byte *) cursurf + LittleLong(cursurf->ofsTriangles));
			
			// simple bounds check
			if(surf->numTriangles < 0 || (byte *) (tri + surf->numTriangles) > (byte *) mdr + size)
			{
				ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
				return qfalse;
			}

			for(j = 0; j < surf->numTriangles; j++)
			{
				tri->indexes[0] = LittleLong(curtri->indexes[0]);
				tri->indexes[1] = LittleLong(curtri->indexes[1]);
				tri->indexes[2] = LittleLong(curtri->indexes[2]);
				
				tri++;
				curtri++;
			}
			
			// tri now points to the end of the surface.
			surf->ofsEnd = (byte *) tri - (byte *) surf;
			surf = (mdrSurface_t *) tri;

			// find the next surface.
			cursurf = (mdrSurface_t *) ((byte *) cursurf + LittleLong(cursurf->ofsEnd));
		}

		// surf points to the next lod now.
		lod->ofsEnd = (int)((byte *) surf - (byte *) lod);
		lod = (mdrLOD_t *) surf;

		// find the next LOD.
		curlod = (mdrLOD_t *)((byte *) curlod + LittleLong(curlod->ofsEnd));
	}
	
	// lod points to the first tag now, so update the offset too.
	tag = (mdrTag_t *) lod;
	mdr->ofsTags = (int)((byte *) tag - (byte *) mdr);
	curtag = (mdrTag_t *) ((byte *)pinmodel + LittleLong(pinmodel->ofsTags));

	// simple bounds check
	if(mdr->numTags < 0 || (byte *) (tag + mdr->numTags) > (byte *) mdr + size)
	{
		ri->Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
		return qfalse;
	}
	
	for (i = 0 ; i < mdr->numTags ; i++)
	{
		tag->boneIndex = LittleLong(curtag->boneIndex);
		Q_strncpyz(tag->name, curtag->name, sizeof(tag->name));
		
		tag++;
		curtag++;
	}
	
	// And finally we know the real offset to the end.
	mdr->ofsEnd = (int)((byte *) tag - (byte *) mdr);

	// phew! we're done.
	
	return qtrue;
}



//=============================================================================

/*
** RE_BeginRegistration
*/
void RE_BeginRegistration( glconfig_t *glconfigOut ) {

	R_Init();

	*glconfigOut = glConfig;

	R_IssuePendingRenderCommands();

	tr.visIndex = 0;
	memset(tr.visClusters, -2, sizeof(tr.visClusters));	// force markleafs to regenerate

	R_ClearFlares();
	RE_ClearScene();

	tr.registered = qtrue;

	// NOTE: this sucks, for some reason the first stretch pic is never drawn
	// without this we'd see a white flash on a level load because the very
	// first time the level shot would not be drawn
//	RE_StretchPic(0, 0, 0, 0, 0, 0, 1, 1, 0);
}

//=============================================================================

void R_SVModelInit()
{
	R_ModelInit();
}

/*
===============
R_ModelInit
===============
*/
void R_ModelInit( void ) {
	model_t		*mod;

	// leave a space for NULL model
	tr.numModels = 0;

	CModelCache->DeleteAll();

	mod = R_AllocModel();
	mod->type = MOD_BAD;
}

extern void KillTheShaderHashTable(void);
void RE_HunkClearCrap(void)
{ //get your dirty sticky assets off me, you damn dirty hunk!
	KillTheShaderHashTable();
	tr.numModels = 0;
	CModelCache->DeleteAll();
	tr.numShaders = 0;
	tr.numSkins = 0;
}


/*
================
R_Modellist_f
================
*/
void R_Modellist_f( void ) {
	int		i, j;
	model_t	*mod;
	int		total;
	int		lods;

	total = 0;
	for ( i = 1 ; i < tr.numModels; i++ ) {
		mod = tr.models[i];
		lods = 1;
		for ( j = 1 ; j < MD3_MAX_LODS ; j++ ) {
			if ( mod->data.mdv[j] && mod->data.mdv[j] != mod->data.mdv[j-1] ) {
				lods++;
			}
		}
		ri->Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		total += mod->dataSize;
	}
	ri->Printf( PRINT_ALL, "%8i : Total models\n", total );

#if	0		// not working right with new hunk
	if ( tr.world ) {
		ri->Printf( PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name );
	}
#endif
}


//=============================================================================


/*
================
R_GetTag
================
*/
static mdvTag_t *R_GetTag( mdvModel_t *mod, int frame, const char *_tagName ) {
	int             i;
	mdvTag_t       *tag;
	mdvTagName_t   *tagName;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = mod->tags + frame * mod->numTags;
	tagName = mod->tagNames;
	for(i = 0; i < mod->numTags; i++, tag++, tagName++)
	{
		if(!strcmp(tagName->name, _tagName))
		{
			return tag;
		}
	}

	return NULL;
}

void R_GetAnimTag( mdrHeader_t *mod, int framenum, const char *tagName, mdvTag_t * dest)
{
	int				i, j, k;
	int				frameSize;
	mdrFrame_t		*frame;
	mdrTag_t		*tag;

	if ( framenum >= mod->numFrames ) 
	{
		// it is possible to have a bad frame while changing models, so don't error
		framenum = mod->numFrames - 1;
	}

	tag = (mdrTag_t *)((byte *)mod + mod->ofsTags);
	for ( i = 0 ; i < mod->numTags ; i++, tag++ )
	{
		if ( !strcmp( tag->name, tagName ) )
		{
			// uncompressed model...
			//
			frameSize = (intptr_t)( &((mdrFrame_t *)0)->bones[ mod->numBones ] );
			frame = (mdrFrame_t *)((byte *)mod + mod->ofsFrames + framenum * frameSize );

			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					dest->axis[j][k]=frame->bones[tag->boneIndex].matrix[k][j];
			}

			dest->origin[0]=frame->bones[tag->boneIndex].matrix[0][3];
			dest->origin[1]=frame->bones[tag->boneIndex].matrix[1][3];
			dest->origin[2]=frame->bones[tag->boneIndex].matrix[2][3];				

			return;
		}
	}

	AxisClear( dest->axis );
	VectorClear( dest->origin );
}

/*
================
R_LerpTag
================
*/
int R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName ) {
	mdvTag_t	*start, *end;
	mdvTag_t	start_space, end_space;
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	model = R_GetModelByHandle( handle );
	if ( !model->data.mdv[0] )
	{
		if(model->type == MOD_MDR)
		{
			start = &start_space;
			end = &end_space;
			R_GetAnimTag((mdrHeader_t *) model->data.mdr, startFrame, tagName, start);
			R_GetAnimTag((mdrHeader_t *) model->data.mdr, endFrame, tagName, end);
		}
		else if( model->type == MOD_IQM ) {
			return R_IQMLerpTag( tag, (iqmData_t *)model->data.iqm,
					startFrame, endFrame,
					frac, tagName );
		} else {

			AxisClear( tag->axis );
			VectorClear( tag->origin );
			return qfalse;

		}
	}
	else
	{
		start = R_GetTag( model->data.mdv[0], startFrame, tagName );
		end = R_GetTag( model->data.mdv[0], endFrame, tagName );
		if ( !start || !end ) {
			AxisClear( tag->axis );
			VectorClear( tag->origin );
			return qfalse;
		}
	}
	
	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
	return qtrue;
}


/*
====================
R_ModelBounds
====================
*/
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	model_t		*model;

	model = R_GetModelByHandle( handle );

	if(model->type == MOD_BRUSH) {
		VectorCopy( model->data.bmodel->bounds[0], mins );
		VectorCopy( model->data.bmodel->bounds[1], maxs );
		
		return;
	} else if (model->type == MOD_MESH) {
		mdvModel_t	*header;
		mdvFrame_t	*frame;

		header = model->data.mdv[0];
		frame = header->frames;

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		
		return;
	} else if (model->type == MOD_MDR) {
		mdrHeader_t	*header;
		mdrFrame_t	*frame;

		header = model->data.mdr;
		frame = (mdrFrame_t *) ((byte *)header + header->ofsFrames);

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		
		return;
	} else if(model->type == MOD_IQM) {
		iqmData_t *iqmData;
		
		iqmData = model->data.iqm;

		if(iqmData->bounds)
		{
			VectorCopy(iqmData->bounds, mins);
			VectorCopy(iqmData->bounds + 3, maxs);
			return;
		}
	}

	VectorClear( mins );
	VectorClear( maxs );
}
