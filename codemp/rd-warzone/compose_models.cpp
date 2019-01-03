#include "compose_models.h"

void scaleVertices(mdxmSurface_t *surf, float scalar) {
	mdxmVertex_t *vert = firstVertex(surf);
	for (int vertex_id=0; vertex_id<surf->numVerts; vertex_id++) {
		vert->vertCoords[0] *= scalar;
		vert->vertCoords[1] *= scalar;
		vert->vertCoords[2] *= scalar;
		vert++;
	}
}

const char *toString(surfaceType_t t) {
	switch (t) {
		case SF_BAD:			return "SF_BAD";
		case SF_SKIP:			return "SF_SKIP";
		case SF_FACE:			return "SF_FACE";
		case SF_GRID:			return "SF_GRID";
		case SF_TRIANGLES:		return "SF_TRIANGLES";
		case SF_POLY:			return "SF_POLY";
		case SF_MDV:			return "SF_MDV";
		case SF_MDR:			return "SF_MDR";
		case SF_IQM:			return "SF_IQM";
		case SF_MDX:			return "SF_MDX";
		case SF_FLARE:			return "SF_FLARE";
		case SF_ENTITY:			return "SF_ENTITY";
		case SF_DISPLAY_LIST:	return "SF_DISPLAY_LIST";
		case SF_VBO_MESH:		return "SF_VBO_MESH";
		case SF_VBO_MDVMESH:	return "SF_VBO_MDVMESH";
	}
	return "missing surfaceType_t switch";
}

const char *toString(modtype_t type) {
	switch (type) {
		case MOD_BAD   : return "MOD_BAD";
		case MOD_BRUSH : return "MOD_BRUSH";
		case MOD_MESH  : return "MOD_MESH";
		case MOD_MDR   : return "MOD_MDR";
		case MOD_IQM   : return "MOD_IQM";
		case MOD_MDXM  : return "MOD_MDXM";
		case MOD_MDXA  : return "MOD_MDXA";
	}
	return "missing modtype_t switch";
}

const char *toString(mdxmSurface_t *surf) {

	static char tmp[512];
	snprintf(tmp, sizeof(tmp), "surf ptr=%p type=%s numVerts=%d numTriangles=%d numBoneReferences=%d", 
		surf,
		toString((surfaceType_t)surf->ident),
		surf->numVerts,
		surf->numTriangles,
		surf->numBoneReferences
	);
	return tmp;
}

// the surface has no idea about its name as much ive seen, we only know it in the context of DockMDXM so far
const char *toString(mdxmSurface_t *surf, char *lookupTableName) {

	static char tmp[512];
	snprintf(tmp, sizeof(tmp), "surf name=%s ptr=%p type=%s numVerts=%d numTriangles=%d numBoneReferences=%d",
		lookupTableName,
		surf,
		toString((surfaceType_t)surf->ident),
		surf->numVerts,
		surf->numTriangles,
		surf->numBoneReferences
	);
	return tmp;
}