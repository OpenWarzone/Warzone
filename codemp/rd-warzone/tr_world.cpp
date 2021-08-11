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
#include "tr_occlusion.h"

extern bool TR_WorldToScreen(vec3_t worldCoord, float *x, float *y);
extern void TR_AxisToAngles(const vec3_t axis[3], vec3_t angles);

extern qboolean LODMODEL_MAP;
extern int		GRASS_DISTANCE;

void RB_CullSurfaceOcclusion(msurface_t *surf)
{
#if defined(__USE_DEPTHDRAWONLY__) || defined(__ZFAR_CULLING_ON_SURFACES__) // Still mark them for the sake of sorting to lower priority...
	if (!ENABLE_OCCLUSION_CULLING || !r_occlusion->integer)
	{
		surf->depthDrawOnly = qfalse;
		surf->depthDrawOnlyFoliage = qfalse;
		return;
	}

	if (surf->shader && surf->shader->nocull)
	{
		surf->depthDrawOnly = qfalse;
		surf->depthDrawOnlyFoliage = qfalse;
		return;
	}

	if ((backEnd.currentEntity && backEnd.currentEntity->e.ignoreCull) || (tr.currentEntity && tr.currentEntity->e.ignoreCull))
	{
		surf->depthDrawOnly = qfalse;
		surf->depthDrawOnlyFoliage = qfalse;
		return;
	}

	#ifdef __ZFAR_CULLING_ON_SURFACES__
		if (surf->shader->materialType == MATERIAL_GREENLEAVES || (surf->shader->hasAlphaTestBits && !surf->shader->hasSplatMaps))
		{// Tree leaves and alpha surfaces can be culled easier by occlusion culling...
			if (surf->cullinfo.currentDistance > tr.occlusionZfarFoliage * 1.75)
			{// Out of foliage view range, but we still want it on depth draws...
				surf->depthDrawOnlyFoliage = qtrue;
			}
		}

		if (surf->cullinfo.currentDistance > tr.occlusionZfar * 1.75)
		{// Out of view range, but we still want it on depth draws...
			surf->depthDrawOnly = qtrue;
		}
	#endif //__ZFAR_CULLING_ON_SURFACES__
#else //!(defined(__USE_DEPTHDRAWONLY__) || defined(__ZFAR_CULLING_ON_SURFACES__))
	surf->depthDrawOnly = qfalse;
	surf->depthDrawOnlyFoliage = qfalse;
#endif //defined(__USE_DEPTHDRAWONLY__) || defined(__ZFAR_CULLING_ON_SURFACES__)
}

/*
================
R_CullSurface

Tries to cull surfaces before they are lighted or
added to the sorting list.
================
*/
extern qboolean		TERRAIN_TESSELLATION_ENABLED;
extern float		TERRAIN_TESSELLATION_LEVEL;

float cullInvW = 0.0;
float cullInvH = 0.0;
float cullInv = 0.0;

static qboolean	R_CullSurface(msurface_t *surf, int entityNum) {
#ifdef __ZFAR_CULLING_ON_SURFACES__
	surf->depthDrawOnly = qfalse;
	surf->depthDrawOnlyFoliage = qfalse;
#endif //__ZFAR_CULLING_ON_SURFACES__

	if (r_nocull->integer || SKIP_CULL_FRAME) {
		return qfalse;
	}

	if (surf->shader && surf->shader->nocull)
	{
		return qfalse;
	}

	if (entityNum != REFENTITYNUM_WORLD)
	{
		trRefEntity_t *ent = &backEnd.refdef.entities[entityNum];

		if (ent && ent->e.ignoreCull)
		{
			return qfalse;
		}
	}

	if (r_cullNoDraws->integer && ((surf->shader->surfaceFlags & SURF_NODRAW) || (*surf->data == SF_SKIP)))
	{// Always skip nodraw surfs on all calculations...
		return qtrue;
	}

	if (surf->cullinfo.type == CULLINFO_NONE) {
		return qfalse;
	}

	if (*surf->data == SF_GRID && r_nocurves->integer) {
		return qtrue;
	}

	if (!surf->cullinfo.centerOriginInitialized)
	{
		surf->cullinfo.centerOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
		surf->cullinfo.centerOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
		surf->cullinfo.centerOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;
		surf->cullinfo.centerOriginInitialized = qtrue;
	}

	if (surf->cullinfo.centerDistanceTime < backEnd.refdef.time - 1000)
	{
		surf->cullinfo.currentDistance = Distance(tr.viewParms.ori.origin, surf->cullinfo.centerOrigin);
		surf->cullinfo.centerDistanceTime = backEnd.refdef.time;
	}

#ifdef __CULL_BY_RANGE_AND_SIZE__
	if (surf->shader->materialType == MATERIAL_TREEBARK 
		&& surf->cullinfo.currentDistance > 196608.0 /* 262144 */) // Tree stumps at this range may as well be culled. should rarely be big enough to see anyway
	{
		return qtrue;
	}

	if (cullInvW == 0.0)
	{// Only calculate these once...
		cullInvW = 1.0 / glConfig.vidWidth;
		cullInvH = 1.0 / glConfig.vidHeight;
		cullInv = min(cullInvW, cullInvH) * 4.0; // * 4.0 to cull a little extra than a single pixel, while being not noticable.
	}

	if (surf->cullinfo.type & CULLINFO_SPHERE)
	{
		if (surf->cullinfo.radius / surf->cullinfo.currentDistance < cullInv)
		{// Would be less than a pixel in size, cull it...
			backEnd.pc.c_tinySkipped++;
			return qtrue;
		}
	}

	if (surf->cullinfo.type & CULLINFO_BOX)
	{
		if (surf->cullinfo.radius == 0.0)
		{
			surf->cullinfo.radius = Distance(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
		}

		if (surf->cullinfo.radius / surf->cullinfo.currentDistance < cullInv)
		{// Would be less than a pixel in size, cull it...
			backEnd.pc.c_tinySkipped++;
			return qtrue;
		}
	}
#endif //__CULL_BY_RANGE_AND_SIZE__

#ifdef __INDOOR_OUTDOOR_CULLING__
	extern qboolean INDOOR_BRUSH_FOUND;
	extern int ENABLE_INDOOR_OUTDOOR_SYSTEM;
	extern qboolean R_IndoorOutdoorCull(shader_t *shader);

	if (ENABLE_INDOOR_OUTDOOR_SYSTEM && INDOOR_BRUSH_FOUND)
	{
		//float dist = surf->cullinfo.currentDistance;
		float sdist = Distance(tr.viewParms.ori.origin, surf->cullinfo.centerOrigin);

		if (sdist > 1024 && R_IndoorOutdoorCull(surf->shader))
		{
			backEnd.viewIsOutdoorsCulledCount++;
			return qtrue;
		}
		else
		{
			backEnd.viewIsOutdoorsNotCulledCount++;
		}
	}
#endif //__INDOOR_OUTDOOR_CULLING__

	if ((TERRAIN_TESSELLATION_ENABLED && surf->shader->hasSplatMaps) || surf->shader->isGrass)
	{// When doing tessellation or grass surfs, check if this surf is in tess or grass range. If so, skip culling because we always need to add grasses to it (visible or not).
		float dist = Distance(tr.viewParms.ori.origin, surf->cullinfo.centerOrigin);
		//float dist = surf->cullinfo.currentDistance;

		if (surf->shader->isGrass && dist <= GRASS_DISTANCE)
		{// In grass range, never cull...
			return qfalse;
		}
		else if (surf->shader->hasSplatMaps)
		{
			dist = round(dist);
			float distFactor = 1.0;

			if (dist > 4096.0)
			{// Closer then 4096.0 gets full tess...
				distFactor = dist / 4096.0;
			}

			/*if (TERRAIN_TESSELLATION_ENABLED
				&& r_terrainTessellation->integer
				&& r_terrainTessellationMax->value >= 2.0
				&& (r_foliage->integer && GRASS_ENABLED && (tess.shader->isGrass || RB_ShouldUseGeometryGrass(tess.shader->materialType))))
			*/

			float tessLevel = max(min(r_terrainTessellationMax->value, TERRAIN_TESSELLATION_LEVEL), 2.0);
			if (max(tessLevel / distFactor, 1.0) > 1.0)
			{// In tessellation range, never cull...
				return qfalse;
			}
		}
	}


	if (surf->cullinfo.type & CULLINFO_PLANE)
	{
		if (tr.currentModel && tr.currentModel->type == MOD_BRUSH)
		{// UQ1: Hack!!! Disabled... These have cull issues...
			RB_CullSurfaceOcclusion(surf);
#ifdef __USE_DEPTHDRAWONLY__
			if (surf->depthDrawOnly) return qtrue;
			if (surf->depthDrawOnlyFoliage) return qtrue;
#endif //__USE_DEPTHDRAWONLY__
			return qfalse;
		}

		// Only true for SF_FACE, so treat like its own function
		float			d;
		cullType_t ct;

		if (!r_facePlaneCull->integer) {
			RB_CullSurfaceOcclusion(surf);
#ifdef __USE_DEPTHDRAWONLY__
			if (surf->depthDrawOnlyFoliage) return qtrue;
#endif //__USE_DEPTHDRAWONLY__
			return qfalse;
		}

		ct = surf->shader->cullType;

		if (ct == CT_TWO_SIDED)
		{
			RB_CullSurfaceOcclusion(surf);
#ifdef __USE_DEPTHDRAWONLY__
			if (surf->depthDrawOnlyFoliage) return qtrue;
#endif //__USE_DEPTHDRAWONLY__
			return qfalse;
		}

		// don't cull for depth shadow
		/*
		if ( tr.viewParms.flags & VPF_DEPTHSHADOW )
		{
		return qfalse;
		}
		*/

		// shadowmaps draw back surfaces
		if (tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW))
		{
			if (ct == CT_FRONT_SIDED)
			{
				ct = CT_BACK_SIDED;
			}
			else
			{
				ct = CT_FRONT_SIDED;
			}
		}

		// do proper cull for orthographic projection
		if (tr.viewParms.flags & VPF_ORTHOGRAPHIC) {
			d = DotProduct(tr.viewParms.ori.axis[0], surf->cullinfo.plane.normal);
			if (ct == CT_FRONT_SIDED) {
				if (d > 0)
					return qtrue;
			}
			else {
				if (d < 0)
					return qtrue;
			}

			RB_CullSurfaceOcclusion(surf);
#ifdef __USE_DEPTHDRAWONLY__
			if (surf->depthDrawOnlyFoliage) return qtrue;
#endif //__USE_DEPTHDRAWONLY__
			return qfalse;
		}

		d = DotProduct(tr.ori.viewOrigin, surf->cullinfo.plane.normal);

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here 
		if (ct == CT_FRONT_SIDED) {
			if (d < surf->cullinfo.plane.dist - 8) {
				return qtrue;
			}
		}
		else {
			if (d > surf->cullinfo.plane.dist + 8) {
				return qtrue;
			}
		}

		RB_CullSurfaceOcclusion(surf);
#ifdef __USE_DEPTHDRAWONLY__
		if (surf->depthDrawOnlyFoliage) return qtrue;
#endif //__USE_DEPTHDRAWONLY__
		return qfalse;
	}

	if (surf->cullinfo.type & CULLINFO_SPHERE)
	{
		int 	sphereCull;

		if (entityNum != REFENTITYNUM_WORLD) {
			sphereCull = R_CullLocalPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);
		}
		else {
			sphereCull = R_CullPointAndRadius(surf->cullinfo.localOrigin, surf->cullinfo.radius);
		}

		if (sphereCull == CULL_OUT)
		{
			return qtrue;
		}
	}

	if (surf->cullinfo.type & CULLINFO_BOX)
	{
		int boxCull;

		if (entityNum != REFENTITYNUM_WORLD) {
			boxCull = R_CullLocalBox(surf->cullinfo.bounds);
		}
		else {
			boxCull = R_CullBox(surf->cullinfo.bounds);
		}

		if (boxCull == CULL_OUT)
		{
			return qtrue;
		}
	}

	RB_CullSurfaceOcclusion(surf);
#ifdef __USE_DEPTHDRAWONLY__
	if (surf->depthDrawOnlyFoliage) return qtrue;
#endif //__USE_DEPTHDRAWONLY__
	return qfalse;
}

/*
====================
R_DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/
#if 0
static int R_DlightSurface( msurface_t *surf, int dlightBits ) {
	float       d;
	int         i;
	dlight_t    *dl;

	if ( surf->cullinfo.type & CULLINFO_PLANE )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			d = DotProduct( dl->origin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
			if ( d < -dl->radius || d > dl->radius ) {
				// dlight doesn't reach the plane
				dlightBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_BOX )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if ( dl->origin[0] - dl->radius > surf->cullinfo.bounds[1][0]
				|| dl->origin[0] + dl->radius < surf->cullinfo.bounds[0][0]
				|| dl->origin[1] - dl->radius > surf->cullinfo.bounds[1][1]
				|| dl->origin[1] + dl->radius < surf->cullinfo.bounds[0][1]
				|| dl->origin[2] - dl->radius > surf->cullinfo.bounds[1][2]
				|| dl->origin[2] + dl->radius < surf->cullinfo.bounds[0][2] ) {
				// dlight doesn't reach the bounds
				dlightBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_SPHERE )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if (!SpheresIntersect(dl->origin, dl->radius, surf->cullinfo.localOrigin, surf->cullinfo.radius))
			{
				// dlight doesn't reach the bounds
				dlightBits &= ~( 1 << i );
			}
		}
	}

	switch(*surf->data)
	{
	case SF_FACE:
	case SF_GRID:
	case SF_TRIANGLES:
	case SF_VBO_MESH:
		((srfBspSurface_t *)surf->data)->dlightBits = dlightBits;
		break;

	default:
		dlightBits = 0;
		break;
	}

	if ( dlightBits ) {
		tr.pc.c_dlightSurfaces++;
	} else {
		tr.pc.c_dlightSurfacesCulled++;
	}

	return dlightBits;
}
#endif

/*
====================
R_PshadowSurface

Just like R_DlightSurface, cull any we can
====================
*/
static int R_PshadowSurface(msurface_t *surf, int pshadowBits) {
#ifdef __PSHADOWS__
	float       d;
	int         i;
	pshadow_t    *ps;

	switch (*surf->data)
	{// UQ1: If we are not going to use these others, why do the math?
	case SF_FACE:
	case SF_GRID:
	case SF_TRIANGLES:
	case SF_VBO_MESH:
		break;
	default:
		return 0;
		break;
	}

	if ( surf->cullinfo.type & CULLINFO_PLANE )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			d = DotProduct( ps->lightOrigin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
			if ( d < -ps->lightRadius || d > ps->lightRadius ) {
				// pshadow doesn't reach the plane
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_BOX )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			if ( ps->lightOrigin[0] - ps->lightRadius > surf->cullinfo.bounds[1][0]
				|| ps->lightOrigin[0] + ps->lightRadius < surf->cullinfo.bounds[0][0]
				|| ps->lightOrigin[1] - ps->lightRadius > surf->cullinfo.bounds[1][1]
				|| ps->lightOrigin[1] + ps->lightRadius < surf->cullinfo.bounds[0][1]
				|| ps->lightOrigin[2] - ps->lightRadius > surf->cullinfo.bounds[1][2]
				|| ps->lightOrigin[2] + ps->lightRadius < surf->cullinfo.bounds[0][2]
				|| BoxOnPlaneSide(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1], &ps->cullPlane) == 2 ) {
				// pshadow doesn't reach the bounds
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_SPHERE )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			if (!SpheresIntersect(ps->viewOrigin, ps->viewRadius, surf->cullinfo.localOrigin, surf->cullinfo.radius)
				|| DotProduct( surf->cullinfo.localOrigin, ps->cullPlane.normal ) - ps->cullPlane.dist < -surf->cullinfo.radius)
			{
				// pshadow doesn't reach the bounds
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	switch(*surf->data)
	{
	case SF_FACE:
	case SF_GRID:
	case SF_TRIANGLES:
	case SF_VBO_MESH:
		((srfBspSurface_t *)surf->data)->pshadowBits = pshadowBits;
		break;

	default:
		pshadowBits = 0;
		break;
	}

	if ( pshadowBits ) {
		//tr.pc.c_dlightSurfaces++;
	}

	return pshadowBits;
#else
	return 0;
#endif
}

/*
======================
R_AddWorldSurface
======================
*/
/*static*/ void R_AddWorldSurface(msurface_t *surf, int entityNum, int dlightBits, int pshadowBits, qboolean dontCache) {
	// FIXME: bmodel fog?
	int cubemapIndex = 0;

	if (r_cullNoDraws->integer && (!surf->shader || (surf->shader->surfaceFlags & SURF_NODRAW) || (*surf->data == SF_SKIP)))
	{// How did we even get here?
		return;
	}

	/*if (tr.viewParms.flags & VPF_EMISSIVEMAP) {
		if (!surf->shader->hasGlow) {
			// Can skip all thinking on this one...
			return;
		}
	}*/

	if (!(surf->shader && surf->shader->nocull))
	{
		// try to cull before dlighting or adding
		if (R_CullSurface(surf, entityNum)) {
			return;
		}
	}

	// check for dlighting
	/*if ( dlightBits ) {
		dlightBits = R_DlightSurface( surf, dlightBits );
		dlightBits = ( dlightBits != 0 );
		}*/

#ifdef __PSHADOWS__
	// check for pshadows
	if ( pshadowBits ) {
		pshadowBits = R_PshadowSurface( surf, pshadowBits);
		pshadowBits = ( pshadowBits != 0 );
	}
#endif

	cubemapIndex = 0;

#if 0 // Maybe do this for trees???...
	if (1)
	{// Testing...
		/*
		// stencil shadows can't do personal models unless I polyhedron clip
		if (r_shadows->integer == 2
			&& !(tr.currentEntity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			&& surf->shader->sort == SS_OPAQUE)
		{
			R_AddDrawSurf((surfaceType_t *)surf->data, tr.shadowShader, 0, qfalse, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0, surf->depthDrawOnly);
		}

		// projection shadows work fine with personal models
		if (r_shadows->integer == 3
			&& !(tr.currentEntity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			&& surf->shader->sort == SS_OPAQUE)
		{
			R_AddDrawSurf((surfaceType_t *)surf->data, tr.projectionShadowShader, 0, qfalse, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0, surf->depthDrawOnly);
		}
		*/
	}
#endif

	R_AddDrawSurf(surf->data, surf->shader, 
#ifdef __Q3_FOG__
		surf->fogIndex, 
#else //!__Q3_FOG__
		0,
#endif //__Q3_FOG__
		dlightBits, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), cubemapIndex, (qboolean)(surf->depthDrawOnly || surf->depthDrawOnlyFoliage));

#ifdef __XYC_SURFACE_SPRITES__
	for (int i = 0, numSprites = surf->numSurfaceSprites; i < numSprites; ++i)
	{
		srfSprites_t *sprites = surf->surfaceSprites + i;
		R_AddDrawSurf((surfaceType_t *)sprites, sprites->shader,
#ifdef __Q3_FOG__
			surf->fogIndex,
#else //!__Q3_FOG__
			0,
#endif //__Q3_FOG__
			dlightBits, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), cubemapIndex, surf->depthDrawOnly);
	}
#endif //__XYC_SURFACE_SPRITES__
}

void R_AddWorldSurfaceThreaded(msurface_t *surf, trRefEntity_t *ent, int entityNum, int dlightBits, int pshadowBits, qboolean dontCache, int64_t shiftedEntityNum) {
	// FIXME: bmodel fog?
	int cubemapIndex = 0;

	if (r_cullNoDraws->integer && (!surf->shader || (surf->shader->surfaceFlags & SURF_NODRAW) || (*surf->data == SF_SKIP)))
	{// How did we even get here?
		return;
	}

	/*if (tr.viewParms.flags & VPF_EMISSIVEMAP) {
	if (!surf->shader->hasGlow) {
	// Can skip all thinking on this one...
	return;
	}
	}*/

	if (!(ent && ent->e.ignoreCull) && !(surf->shader && surf->shader->nocull))
	{
		// try to cull before dlighting or adding
		if (R_CullSurface(surf, entityNum)) {
			return;
		}
	}

	// check for dlighting
	/*if ( dlightBits ) {
	dlightBits = R_DlightSurface( surf, dlightBits );
	dlightBits = ( dlightBits != 0 );
	}*/

#ifdef __PSHADOWS__
	// check for pshadows
	if (pshadowBits) {
		pshadowBits = R_PshadowSurface(surf, pshadowBits);
		pshadowBits = (pshadowBits != 0);
	}
#endif

	cubemapIndex = 0;

#if 0 // Maybe do this for trees???...
	if (1)
	{// Testing...
	 /*
	 // stencil shadows can't do personal models unless I polyhedron clip
	 if (r_shadows->integer == 2
	 && !(tr.currentEntity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
	 && surf->shader->sort == SS_OPAQUE)
	 {
	 R_AddDrawSurf((surfaceType_t *)surf->data, tr.shadowShader, 0, qfalse, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0, surf->depthDrawOnly);
	 }

	 // projection shadows work fine with personal models
	 if (r_shadows->integer == 3
	 && !(tr.currentEntity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
	 && surf->shader->sort == SS_OPAQUE)
	 {
	 R_AddDrawSurf((surfaceType_t *)surf->data, tr.projectionShadowShader, 0, qfalse, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0, surf->depthDrawOnly);
	 }
	 */
	}
#endif

	R_AddDrawSurfThreaded(surf->data, surf->shader,
#ifdef __Q3_FOG__
		surf->fogIndex,
#else //!__Q3_FOG__
		0,
#endif //__Q3_FOG__
		dlightBits, R_IsPostRenderEntity(entityNum, ent), cubemapIndex, (qboolean)(surf->depthDrawOnly || surf->depthDrawOnlyFoliage), shiftedEntityNum);
}

/*
=============================================================

BRUSH MODELS

=============================================================
*/

/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces(trRefEntity_t *ent, model_t *currentModel, int entityNum, int64_t shiftedEntityNum) {
	bmodel_t	*bmodel;
	int			clip;
	model_t		*pModel;
	int			i;

	pModel = R_GetModelByHandle(ent->e.hModel);

	bmodel = pModel->data.bmodel;

	clip = R_CullLocalBox(bmodel->bounds);
	if (clip == CULL_OUT) {
		return;
	}

#ifdef __DLIGHT_BMODEL__
	R_SetupEntityLighting(&tr.refdef, ent);
	R_DlightBmodel(bmodel, ent, currentModel, entityNum, shiftedEntityNum);
#endif //__DLIGHT_BMODEL__

	for (i = 0; i < bmodel->numSurfaces; i++) {
		int surf = bmodel->firstSurface + i;

		if (tr.world->surfacesViewCount[surf] != tr.viewCount)
		{
			tr.world->surfacesViewCount[surf] = tr.viewCount;
			R_AddWorldSurfaceThreaded(tr.world->surfaces + surf, ent, entityNum, /*ent->needDlights*/qfalse, 0, qtrue, shiftedEntityNum);
		}
	}
}

void RE_SetRangedFog(float range)
{
	tr.rangedFog = range;
}

/*
=============================================================

WORLD MODEL

=============================================================
*/

int R_BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist[2];
	int		sides, b, i;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	dist[0] = dist[1] = 0;
	if (p->signbits < 8) // >= 8: default case is original code (dist[0]=dist[1]=0)
	{
		for (i = 0; i < 3; i++)
		{
			b = (p->signbits >> i) & 1;
			dist[b] += p->normal[i] * emaxs[i];
			dist[!b] += p->normal[i] * emins[i];
		}
	}

	sides = 0;
	if (dist[0] >= p->dist)
		sides = 1;
	if (dist[1] < p->dist)
		sides |= 2;

	return sides;
}

qboolean R_NodeInFOV(vec3_t spot, vec3_t from)
{
	//return qtrue;

	vec3_t	deltaVector, angles, deltaAngles;
	vec3_t	fromAnglesCopy;
	vec3_t	fromAngles;
	//int hFOV = backEnd.refdef.fov_x * 0.5;
	//int vFOV = backEnd.refdef.fov_y * 0.5;
	int hFOV = 120;
	int vFOV = 120;
	//int hFOV = backEnd.refdef.fov_x;
	//int vFOV = backEnd.refdef.fov_y;
	//int hFOV = 80;
	//int vFOV = 80;
	//int hFOV = tr.refdef.fov_x * 0.5;
	//int vFOV = tr.refdef.fov_y * 0.5;

	TR_AxisToAngles(tr.refdef.viewaxis, fromAngles);

	VectorSubtract(spot, from, deltaVector);
	vectoangles(deltaVector, angles);
	VectorCopy(fromAngles, fromAnglesCopy);

	deltaAngles[PITCH] = AngleDelta(fromAnglesCopy[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAnglesCopy[YAW], angles[YAW]);

	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	return qfalse;
}

/*
================
R_RecursiveWorldNode
================
*/
//int numOcclusionNodesChecked = 0;
//int numOcclusionNodesCulled = 0;

static void R_RecursiveWorldNode(mnode_t *node, int planeBits, int dlightBits, int pshadowBits) {
#if 0
	if (r_testvalue0->integer && R_CullBoxMinsMaxs(node->mins, node->maxs) == CULL_OUT)
	{
		return;
	}
#endif

	do {
		// if the node wasn't marked as potentially visible, exit
		if (node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex])
		{
			return;
		}

		if (node->contents != -1 && !node->nummarksurfaces)
		{
			// don't waste time dealing with this empty leaf
			return;
		}

		int			newDlights[2];
		unsigned int newPShadows[2];

		// if the node wasn't marked as potentially visible, exit
		// pvs is skipped for depth shadows
		if (!(tr.viewParms.flags & VPF_DEPTHSHADOW) && node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex]) {
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if (!r_nocull->integer) {
			if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer && node->occluded && !(tr.viewParms.flags & VPF_DEPTHSHADOW))
			{
				return;
			}
			
#if 0
			if (r_testvalue0->integer && R_CullBoxMinsMaxs(node->mins, node->maxs) == CULL_OUT)
			{
				return;
			}
#endif

#if 0
			int		r;

			if (planeBits & 1) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~1;			// all descendants will also be in front
				}
			}

			if (planeBits & 2) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~2;			// all descendants will also be in front
				}
			}

			if (planeBits & 4) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~4;			// all descendants will also be in front
				}
			}

			if (planeBits & 8) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~8;			// all descendants will also be in front
				}
			}

			if (planeBits & 16) {
				r = R_BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);
				if (r == 2) {
					return;						// culled
				}
				if (r == 1) {
					planeBits &= ~16;			// all descendants will also be in front
				}
			}
#endif

			int i;
			int r;

			for (i = 0; i < 5; i++)
			{
				if (planeBits & (1 << i))
				{
					r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[i]);

					if (r == 2)
					{
						return; // culled
					}

					if (r == 1)
					{
						planeBits &= ~(1 << i);  // all descendants will also be in front
					}
				}
			}
		}

#ifdef __ZFAR_CULLING_ON_LEAFS__
		if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer && node->contents != -1)
		{
			float closestCornerDistance = 9999999.0;

			for (int i = 0; i < 8; i++)
			{
				vec3_t v;

				if (i & 1)
				{
					v[0] = node->mins[0];
				}
				else
				{
					v[0] = node->maxs[0];
				}

				if (i & 2)
				{
					v[1] = node->mins[1];
				}
				else
				{
					v[1] = node->maxs[1];
				}

				if (i & 4)
				{
					v[2] = node->mins[2];
				}
				else
				{
					v[2] = node->maxs[2];
				}

				float distance = Distance(/*tr.viewParms.ori.origin*/tr.refdef.vieworg, v);

				if (distance < closestCornerDistance)
				{
					closestCornerDistance = distance;
				}
			}

			if (closestCornerDistance > tr.occlusionZfar * 2.0 && !tr.currentEntity->e.ignoreCull)// * 1.75)
			{
				return;
			}
		}
#endif //__ZFAR_CULLING_ON_LEAFS__

		if (node->contents != -1) {
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative

		newDlights[0] = 0;
		newDlights[1] = 0;

#ifdef __PSHADOWS__
		newPShadows[0] = 0;
		newPShadows[1] = 0;
		if ( pshadowBits ) {
			int	i;

			for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
				pshadow_t	*shadow;
				float		dist;

				if ( pshadowBits & ( 1 << i ) ) {
					shadow = &tr.refdef.pshadows[i];
					dist = DotProduct( shadow->lightOrigin, node->plane->normal ) - node->plane->dist;

					if ( dist > -shadow->lightRadius ) {
						newPShadows[0] |= ( 1 << i );
					}
					if ( dist < shadow->lightRadius ) {
						newPShadows[1] |= ( 1 << i );
					}
				}
			}
		}
#else
		newPShadows[0] = 0;
		newPShadows[1] = 0;
#endif

		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits, newDlights[0], newPShadows[0]);

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
		pshadowBits = newPShadows[1];
	} while (1);

	{
		// leaf node, so add mark surfaces
		int			c;
		int surf, *view;

		tr.pc.c_leafs++;

		// add to z buffer bounds
		if (node->mins[0] < tr.viewParms.visBounds[0][0]) {
			tr.viewParms.visBounds[0][0] = node->mins[0];
		}
		if (node->mins[1] < tr.viewParms.visBounds[0][1]) {
			tr.viewParms.visBounds[0][1] = node->mins[1];
		}
		if (node->mins[2] < tr.viewParms.visBounds[0][2]) {
			tr.viewParms.visBounds[0][2] = node->mins[2];
		}

		if (node->maxs[0] > tr.viewParms.visBounds[1][0]) {
			tr.viewParms.visBounds[1][0] = node->maxs[0];
		}
		if (node->maxs[1] > tr.viewParms.visBounds[1][1]) {
			tr.viewParms.visBounds[1][1] = node->maxs[1];
		}
		if (node->maxs[2] > tr.viewParms.visBounds[1][2]) {
			tr.viewParms.visBounds[1][2] = node->maxs[2];
		}

		// add merged and unmerged surfaces
		if (tr.world->viewSurfaces && !r_nocurves->integer)
			view = tr.world->viewSurfaces + node->firstmarksurface;
		else
			view = tr.world->marksurfaces + node->firstmarksurface;

		c = node->nummarksurfaces;

		while (c--)
		{
			// just mark it as visible, so we don't jump out of the cache derefencing the surface
			surf = *view;
			if (surf < 0)
			{
				if (tr.world->mergedSurfacesViewCount[-surf - 1] != tr.viewCount)
				{
					tr.world->mergedSurfacesViewCount[-surf - 1] = tr.viewCount;
					//tr.world->mergedSurfacesDlightBits[-surf - 1] = dlightBits;
#ifdef __PSHADOWS__
					tr.world->mergedSurfacesPshadowBits[-surf - 1] = pshadowBits;
#endif
				}
				else
				{
					//tr.world->mergedSurfacesDlightBits[-surf - 1] |= dlightBits;
#ifdef __PSHADOWS__
					tr.world->mergedSurfacesPshadowBits[-surf - 1] |= pshadowBits;
#endif
				}
			}
			else
			{
				if (tr.world->surfacesViewCount[surf] != tr.viewCount)
				{
					tr.world->surfacesViewCount[surf] = tr.viewCount;
					//tr.world->surfacesDlightBits[surf] = dlightBits;
#ifdef __PSHADOWS__
					tr.world->surfacesPshadowBits[surf] = pshadowBits;
#endif
				}
				else
				{
					//tr.world->surfacesDlightBits[surf] |= dlightBits;
#ifdef __PSHADOWS__
					tr.world->surfacesPshadowBits[surf] |= pshadowBits;
#endif
				}
			}
			view++;
		}
	}
}

/*
===============
R_PointInLeaf
===============
*/
static mnode_t *R_PointInLeaf(const vec3_t p) {
	mnode_t		*node;
	float		d;
	cplane_t	*plane;

	if (!tr.world) {
		ri->Error(ERR_DROP, "R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while (1) {
		if (node->contents != -1) {
			break;
		}
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0) {
			node = node->children[0];
		}
		else {
			node = node->children[1];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte *R_ClusterPVS(int cluster) {
	if (r_novis->integer)
	{
		return tr.world->novis;
	}

	if (!tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters) {
		//return NULL;
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
qboolean R_inPVS(const vec3_t p1, const vec3_t p2, byte *mask) {
	if (r_novis->integer)
	{
		return qtrue;
	}

	int		cluster;

	mnode_t *leaf = R_PointInLeaf(p1);
	cluster = leaf->cluster;

	//agh, the damn snapshot mask doesn't work for this
	mask = (byte *)R_ClusterPVS(cluster);

	leaf = R_PointInLeaf(p2);
	cluster = leaf->cluster;

	//if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	if (!(mask[cluster >> 3] & (1 << (cluster & 7))))
		return qfalse;

	return qtrue;
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves(void) {
	const byte	*vis;
	mnode_t	*leaf, *parent;
	int		i;
	int		cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if (r_lockpvs->integer) {
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again
	for (i = 0; i < MAX_VISCOUNTS; i++)
	{
		// if the areamask or r_showcluster was modified, invalidate all visclusters
		// this caused doors to open into undrawn areas
		if (tr.refdef.areamaskModified || r_showcluster->modified)
		{
			tr.visClusters[i] = -2;
		}
		else if (tr.visClusters[i] == cluster)
		{
			if (tr.visClusters[i] != tr.visClusters[tr.visIndex] && r_showcluster->integer)
			{
				ri->Printf(PRINT_ALL, "found cluster:%i  area:%i  index:%i\n", cluster, leaf->area, i);
			}
			tr.visIndex = i;
			return;
		}
	}

	tr.visIndex = (tr.visIndex + 1) % MAX_VISCOUNTS;
	tr.visCounts[tr.visIndex]++;
	tr.visClusters[tr.visIndex] = cluster;

	if (r_showcluster->modified || r_showcluster->integer) {
		r_showcluster->modified = qfalse;
		if (r_showcluster->integer) {
			ri->Printf(PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

	vis = R_ClusterPVS(tr.visClusters[tr.visIndex]);

	for (i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++) {
		cluster = leaf->cluster;
		if (cluster < 0 || cluster >= tr.world->numClusters) {
			continue;
		}

		// check general pvs
		if (vis && !(vis[cluster >> 3] & (1 << (cluster & 7)))) {
			continue;
		}

		// check for door connection
		//if ((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7)))) {
		if (!(vis[cluster >> 3] & (1 << (cluster & 7)))) {
			continue;		// not visible
		}

		parent = leaf;
		do {
			if (parent->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
				break;
			parent->visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			parent = parent->parent;
		} while (parent);
	}
}

qboolean G_BoxInBounds(vec3_t point, vec3_t mins, vec3_t maxs, vec3_t boundsMins, vec3_t boundsMaxs)
{
	vec3_t boxMins;
	vec3_t boxMaxs;

	VectorAdd(point, mins, boxMins);
	VectorAdd(point, maxs, boxMaxs);

	if (boxMaxs[0] > boundsMaxs[0])
		return qfalse;

	if (boxMaxs[1] > boundsMaxs[1])
		return qfalse;

	if (boxMaxs[2] > boundsMaxs[2])
		return qfalse;

	if (boxMins[0] < boundsMins[0])
		return qfalse;

	if (boxMins[1] < boundsMins[1])
		return qfalse;

	if (boxMins[2] < boundsMins[2])
		return qfalse;

	//box is completely contained within bounds
	return qtrue;
}

#ifdef __RENDERER_FOLIAGE__
void R_FoliageQuadStamp(vec4_t quadVerts[4])
{
	vec2_t texCoords[4];

	VectorSet2(texCoords[0], 0.0f, 0.0f);
	VectorSet2(texCoords[1], 1.0f, 0.0f);
	VectorSet2(texCoords[2], 1.0f, 1.0f);
	VectorSet2(texCoords[3], 0.0f, 1.0f);

	GLSL_BindProgram(&tr.textureColorShader);

	GLSL_SetUniformMatrix16(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection, 1);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

#ifdef __TEXTURECOLOR_SHADER_BINDLESS__
	if (tr.textureColorShader.isBindless)
	{
		GLSL_SetBindlessTexture(&tr.textureColorShader, UNIFORM_DIFFUSEMAP, &tr.whiteImage, 0);
		GLSL_BindlessUpdate(&tr.textureColorShader);
	}
#endif //__TEXTURECOLOR_SHADER_BINDLESS__

	RB_InstantQuad2(quadVerts, texCoords);
}

extern int64_t		r_numentities;
extern int			r_firstSceneEntity;

extern void TR_AxisToAngles ( const vec3_t axis[3], vec3_t angles );
extern void R_WorldToLocal (const vec3_t world, vec3_t local);

static void R_FoliageQuad2( vec3_t surfOrigin, cplane_t plane, vec3_t bounds[2], float scale ) {
	vec3_t	left, up;
	float	radius;
	//float	color[4];

	// calculate the xyz locations for the four corners
	radius = 24.0 * scale;

	vec3_t mins, maxs, newOrg, angles, vfwd, vright, vup;

	VectorSet(mins, bounds[0][0], bounds[0][1], bounds[0][2]);
	VectorSet(maxs, bounds[1][0], bounds[1][1], bounds[1][2]);

	if (mins[0] > maxs[0])
	{
		float temp;
		temp = mins[0];
		mins[0] = maxs[0];
		maxs[0] = temp;
	}

	if (mins[1] > maxs[1])
	{
		float temp;
		temp = mins[1];
		mins[1] = maxs[1];
		maxs[1] = temp;
	}

	if (mins[2] > maxs[2])
	{
		float temp;
		temp = mins[2];
		mins[2] = maxs[2];
		maxs[2] = temp;
	}

	float length = maxs[0] - mins[0];
	mins[0] = -(length/2.0);
	maxs[0] = (length/2.0);

	length = maxs[1] - mins[1];
	mins[1] = -(length/2.0);
	maxs[1] = (length/2.0);

	length = maxs[2] - mins[2];
	mins[2] = -(length/2.0);
	maxs[2] = (length/2.0);

	//AngleVectors(plane.normal, vfwd, vright, vup);
	matrix3_t axis;
	VectorCopy( plane.normal, axis[0] );
	PerpendicularVector( axis[1], axis[0] );
	CrossProduct( axis[0], axis[1], axis[2] );

	VectorCopy( axis[1], left );
	VectorCopy( axis[2], up );

	VectorScale( left, radius, left );
	VectorScale( up, radius, up );

	TR_AxisToAngles(axis, angles);
	AngleVectors (angles, vfwd, vright, vup);

	//ri->Printf(PRINT_WARNING, "mins %f %f %f, maxs %f %f %f, plane %f %f %f\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2], plane.normal[0], plane.normal[1], plane.normal[2]);

	bool Ynerf = true;

	for (float offsetX = mins[0]; offsetX < maxs[0]; offsetX += 32.0)
	{
		Ynerf = !Ynerf;

		float ms1 = mins[1];
		if (Ynerf) ms1 += 16.0;

		for (float offsetY = ms1; offsetY < maxs[1]; offsetY += 32.0)
		{
			qhandle_t shader = RE_RegisterShader("models/pop/foliages/sch_weed_a.tga");

			VectorCopy(surfOrigin, newOrg);
			VectorMA( newOrg, offsetX, vright, newOrg );
			VectorMA( newOrg, offsetY, vup, newOrg );

			refEntity_t re;
			memset( &re, 0, sizeof( re ) );
			VectorCopy(newOrg, re.origin);

			re.reType = RT_GRASS;

			re.radius = radius;
			re.customShader = shader;
			re.shaderRGBA[0] = 255;
			re.shaderRGBA[1] = 255;
			re.shaderRGBA[2] = 255;
			re.shaderRGBA[3] = 255;

			//re.origin[2] += re.radius;

			angles[PITCH] = angles[ROLL] = 0.0f;
			angles[YAW] = 0.0f;

			VectorCopy(angles, re.angles);
			AnglesToAxis(angles, re.axis);

			//if (Distance(tr.refdef.vieworg, newOrg) < 64) ri->Printf(PRINT_WARNING, "org %f %f %f\n", newOrg[0], newOrg[1], newOrg[2]);

			RE_AddRefEntityToScene( &re );

			tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
			tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];
		}
	}
}

/*
static void R_FoliageQuad( vec3_t surfOrigin, cplane_t plane, vec3_t bounds[2], float scale ) {
vec3_t	left, up;
float	radius;
float	color[4];

// calculate the xyz locations for the four corners
radius = 128.0;//24.0 * scale;

VectorSet4(color, 1, 1, 1, 1);

vec3_t mins, maxs, newOrg, origin, angles, vfwd, vright, vup;

VectorSet(mins, bounds[0][0], bounds[0][1], bounds[0][2]);
VectorSet(maxs, bounds[1][0], bounds[1][1], bounds[1][2]);

if (mins[0] > maxs[0])
{
float temp;
temp = mins[0];
mins[0] = maxs[0];
maxs[0] = temp;
}

if (mins[1] > maxs[1])
{
float temp;
temp = mins[1];
mins[1] = maxs[1];
maxs[1] = temp;
}

if (mins[2] > maxs[2])
{
float temp;
temp = mins[2];
mins[2] = maxs[2];
maxs[2] = temp;
}

float length = maxs[0] - mins[0];
mins[0] = -(length/2.0);
maxs[0] = (length/2.0);

length = maxs[1] - mins[1];
mins[1] = -(length/2.0);
maxs[1] = (length/2.0);

length = maxs[2] - mins[2];
mins[2] = -(length/2.0);
maxs[2] = (length/2.0);

//AngleVectors(plane.normal, vfwd, vright, vup);
matrix3_t axis;
VectorCopy( plane.normal, axis[0] );
PerpendicularVector( axis[1], axis[0] );
CrossProduct( axis[0], axis[1], axis[2] );

VectorCopy( axis[1], left );
VectorCopy( axis[2], up );

VectorScale( left, radius, left );
VectorScale( up, radius, up );

TR_AxisToAngles(axis, angles);
AngleVectors (angles, vfwd, vright, vup);

//ri->Printf(PRINT_WARNING, "mins %f %f %f, maxs %f %f %f, plane %f %f %f\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2], plane.normal[0], plane.normal[1], plane.normal[2]);

for (float offsetX = mins[0]; offsetX < maxs[0]; offsetX += 64.0)
{
for (float offsetY = mins[1]; offsetY < maxs[1]; offsetY += 64.0)
{
VectorCopy(surfOrigin, newOrg);
newOrg[2] += radius;
VectorMA( newOrg, offsetX, vright, newOrg );
VectorMA( newOrg, offsetY, vup, newOrg );
//if (Distance(tr.refdef.vieworg, newOrg) < 64) ri->Printf(PRINT_WARNING, "org %f %f %f\n", newOrg[0], newOrg[1], newOrg[2]);

R_WorldToLocal(newOrg, origin);
//VectorCopy(newOrg, origin);

RB_AddQuadStamp( origin, left, up, color );

#if 0
vec4_t quadVerts[4];
quadVerts[0][0] = origin[0] + left[0] + up[0];
quadVerts[0][1] = origin[1] + left[1] + up[1];
quadVerts[0][2] = origin[2] + left[2] + up[2];

quadVerts[1][0] = origin[0] - left[0] + up[0];
quadVerts[1][1] = origin[1] - left[1] + up[1];
quadVerts[1][2] = origin[2] - left[2] + up[2];

quadVerts[2][0] = origin[0] - left[0] - up[0];
quadVerts[2][1] = origin[1] - left[1] - up[1];
quadVerts[2][2] = origin[2] - left[2] - up[2];

quadVerts[3][0] = origin[0] + left[0] - up[0];
quadVerts[3][1] = origin[1] + left[1] - up[1];
quadVerts[3][2] = origin[2] + left[2] - up[2];
R_FoliageQuadStamp(quadVerts);
#endif
}
}
}

int			FOLIAGE_NUM_SURFACES = 0;
vec3_t		FOLIAGE_ORIGINS[65536];
vec3_t		FOLIAGE_BOUNDS[65536][2];
cplane_t	FOLIAGE_PLANES[65536];
shader_t	*FOLIAGE_SHADERS[65536];

void R_DrawFoliage (int surfaceNum) {
shader_t	*shader = FOLIAGE_SHADERS[surfaceNum];

if ((shader->materialType ) == MATERIAL_SHORTGRASS)
{
R_FoliageQuad(FOLIAGE_ORIGINS[surfaceNum], FOLIAGE_PLANES[surfaceNum], FOLIAGE_BOUNDS[surfaceNum], 1.0);
}
else if ((shader->materialType ) == MATERIAL_LONGGRASS)
{
R_FoliageQuad(FOLIAGE_ORIGINS[surfaceNum], FOLIAGE_PLANES[surfaceNum], FOLIAGE_BOUNDS[surfaceNum], 2.0);
}
}

void R_DrawAllFoliages ( void )
{
for (int i = 0; i < FOLIAGE_NUM_SURFACES; i++)
{
R_DrawFoliage(i);
}

ri->Printf(PRINT_WARNING, "%i foliage surfaces drawn.\n", FOLIAGE_NUM_SURFACES);
}
*/

void R_AddFoliage (msurface_t *surf) {
	//RE_RegisterShader("models/warzone/foliage/grass01.png");
	if ((surf->shader->materialType ) == MATERIAL_SHORTGRASS || (surf->shader->materialType ) == MATERIAL_LONGGRASS)
	{
		vec3_t		surfOrigin;

		//shader_t	*shader = R_GetShaderByHandle(RE_RegisterShader("models/warzone/foliage/grass01.png"));

		surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
		surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
		surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;

		if (Distance(surfOrigin, tr.refdef.vieworg) <= 2048)
		{
			/*
			VectorCopy(surfOrigin, FOLIAGE_ORIGINS[FOLIAGE_NUM_SURFACES]);
			VectorCopy(surf->cullinfo.bounds[0], FOLIAGE_BOUNDS[FOLIAGE_NUM_SURFACES][0]);
			VectorCopy(surf->cullinfo.bounds[1], FOLIAGE_BOUNDS[FOLIAGE_NUM_SURFACES][1]);
			FOLIAGE_PLANES[FOLIAGE_NUM_SURFACES] = surf->cullinfo.plane;
			FOLIAGE_SHADERS[FOLIAGE_NUM_SURFACES] = shader;
			FOLIAGE_NUM_SURFACES++;
			*/
			R_FoliageQuad2( surfOrigin, surf->cullinfo.plane, surf->cullinfo.bounds, 1.0 );
		}
	}
}
#endif //__RENDERER_FOLIAGE__

//#define __DISTANCE_SORTING__

#ifdef __DISTANCE_SORTING__
static int DistanceSurfaceCompare(const void *a, const void *b)
{
	msurface_t   *aa, *bb;

	aa = (msurface_t *)a;
	bb = (msurface_t *)b;

	if (!aa->cullinfo.centerOriginInitialized)
	{// If this surface's center org has not been set up yet, set it up now...
		aa->cullinfo.centerOrigin[0] = (aa->cullinfo.bounds[0][0] + aa->cullinfo.bounds[1][0]) * 0.5f;
		aa->cullinfo.centerOrigin[1] = (aa->cullinfo.bounds[0][1] + aa->cullinfo.bounds[1][1]) * 0.5f;
		aa->cullinfo.centerOrigin[2] = (aa->cullinfo.bounds[0][2] + aa->cullinfo.bounds[1][2]) * 0.5f;
		aa->cullinfo.centerOriginInitialized = qtrue;
	}

	if (!bb->cullinfo.centerOriginInitialized)
	{// If this surface's center org has not been set up yet, set it up now...
		bb->cullinfo.centerOrigin[0] = (bb->cullinfo.bounds[0][0] + bb->cullinfo.bounds[1][0]) * 0.5f;
		bb->cullinfo.centerOrigin[1] = (bb->cullinfo.bounds[0][1] + bb->cullinfo.bounds[1][1]) * 0.5f;
		bb->cullinfo.centerOrigin[2] = (bb->cullinfo.bounds[0][2] + bb->cullinfo.bounds[1][2]) * 0.5f;
		bb->cullinfo.centerOriginInitialized = qtrue;
	}

	aa->cullinfo.currentDistance = Distance(aa->cullinfo.centerOrigin, backEnd.refdef.vieworg);
	bb->cullinfo.currentDistance = Distance(bb->cullinfo.centerOrigin, backEnd.refdef.vieworg);

	if (aa->cullinfo.currentDistance < bb->cullinfo.currentDistance)
		return -1;

	if (aa->cullinfo.currentDistance > bb->cullinfo.currentDistance)
		return 1;

#if 0
	// shader first
	if (aa->shader->sortedIndex < bb->shader->sortedIndex)
		return -1;

	else if (aa->shader->sortedIndex > bb->shader->sortedIndex)
		return 1;
#endif

#ifdef __Q3_FOG__
	// by fogIndex
	if (aa->fogIndex < bb->fogIndex)
		return -1;

	else if (aa->fogIndex > bb->fogIndex)
		return 1;
#endif //__Q3_FOG__

#ifndef __PLAYER_BASED_CUBEMAPS__
	// by cubemapIndex
	if (aa->cubemapIndex < bb->cubemapIndex)
		return -1;

	else if (aa->cubemapIndex > bb->cubemapIndex)
		return 1;
#endif //__PLAYER_BASED_CUBEMAPS__

	return 0;
}

#endif //__DISTANCE_SORTING__

/*
=============
R_AddWorldSurfaces
=============
*/

#if defined(__SOFTWARE_OCCLUSION__) && defined(__THREADED_OCCLUSION2__)
extern void Occlusion_FinishThread();
#endif //defined(__SOFTWARE_OCCLUSION__) && defined(__THREADED_OCCLUSION2__)

vec3_t PREVIOUS_OCCLUSION_ORG = { -999999 };
vec3_t PREVIOUS_OCCLUSION_ANGLES = { -999999 };

void R_AddWorldSurfaces(void) {
	int planeBits;
#ifdef __PSHADOWS__
	int pshadowBits;//, dlightBits;
#endif
	int changeFrustum = 0;
	int scene;

	if (!r_drawworld->integer) {
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL) {
		return;
	}

	/*if (tr.viewParms.flags & VPF_SKYCUBEDAY) {
		return;
	}

	if (tr.viewParms.flags & VPF_SKYCUBENIGHT) {
		return;
	}*/

	/*
	#ifdef __RENDERER_FOLIAGE__
	FOLIAGE_NUM_SURFACES = 0;
	#endif //__RENDERER_FOLIAGE__
	*/

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	scene = tr.viewParms.isPortal ? 1 : 0;

	// determine which leaves are in the PVS / areamask
	if (!(tr.viewParms.flags & VPF_DEPTHSHADOW))
		R_MarkLeaves();

	if (!backEnd.depthFill)
	{
		// clear out the visible min/max
		ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);
	}

	// perform frustum culling and flag all the potentially visible surfaces
	tr.refdef.num_dlights = min(tr.refdef.num_dlights, MAX_DLIGHTS);
#ifdef __PSHADOWS__
	tr.refdef.num_pshadows = min(tr.refdef.num_pshadows, MAX_DLIGHTS);
#endif

	planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;

	if (tr.viewParms.flags & VPF_DEPTHSHADOW)
	{
		//dlightBits = 0;
#ifdef __PSHADOWS__
		pshadowBits = 0;
#endif
	}
	else if (!(tr.viewParms.flags & VPF_SHADOWMAP))
	{
		//dlightBits = ( 1 << tr.refdef.num_dlights ) - 1;
#ifdef __PSHADOWS__
		pshadowBits = (1 << tr.refdef.num_pshadows) - 1;
#endif
	}
	else
	{
		//dlightBits = ( 1 << tr.refdef.num_dlights ) - 1;
#ifdef __PSHADOWS__
		pshadowBits = 0;
#endif
	}

	if (!(tr.viewParms.flags & VPF_DEPTHSHADOW) && !backEnd.depthFill)
	{
		if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer >= 1)
		{
			RB_CheckOcclusions();
		}

/*#ifdef __USE_VBO_AREAS__
		extern void SetVBOVisibleAreas(void);
		SetVBOVisibleAreas();
#endif //__USE_VBO_AREAS__*/
	}

#ifdef __PSHADOWS__
	R_RecursiveWorldNode(tr.world->nodes, planeBits, 0, pshadowBits);
#else //!__PSHADOWS__
	//numOcclusionNodesChecked = 0;
	//numOcclusionNodesCulled = 0;
	R_RecursiveWorldNode(tr.world->nodes, planeBits, 0, 0);
	/*if (r_testvalue1->integer)
	{
		ri->Printf(PRINT_WARNING, "checked: %i. occluded: %i.\n", numOcclusionNodesChecked, numOcclusionNodesCulled);
	}*/
#endif //__PSHADOWS__

#ifdef __DISTANCE_SORTING__
	// Sort by distance first, then by shader second...
	std::qsort(tr.world->surfaces, tr.world->numWorldSurfaces, sizeof(*tr.world->surfaces), DistanceSurfaceCompare);
	std::qsort(tr.world->mergedSurfaces, tr.world->numMergedSurfaces, sizeof(*tr.world->mergedSurfaces), DistanceSurfaceCompare);
#endif //__DISTANCE_SORTING__

	// now add all the potentially visible surfaces
	// also mask invisible dlights for next frame
	{
		int i;

		//tr.refdef.dlightMask = 0;

#ifdef __RENDERER_THREADING__
#pragma omp parallel for if (r_multithread->integer && tr.world->numWorldSurfaces > 128) num_threads(r_multithread->integer)
#endif
		for (i = 0; i < tr.world->numWorldSurfaces; i++)
		{
			if (tr.world->surfacesViewCount[i] != tr.viewCount)
				continue;

#if 1
			if (!(tr.world->surfaces + i)->shader->isSky)
			{// Skip everything except sky when drawing sky cubes...
				if (tr.viewParms.flags & VPF_SKYCUBEDAY) {
					continue;
				}

				if (tr.viewParms.flags & VPF_SKYCUBENIGHT) {
					continue;
				}
			}
#endif

			if ((tr.world->surfaces + i)->shader->isSky && tr.world != tr.worldSolid)
			{// Skip sky when drawing the transparancy world...
				continue;
			}

			if (!(tr.world->surfaces + i)->isMerged)
			{
#ifdef __PSHADOWS__
				R_AddWorldSurface(tr.world->surfaces + i, tr.currentEntityNum, 0/*tr.world->surfacesDlightBits[i]*/, tr.world->surfacesPshadowBits[i], qtrue);
#else //!__PSHADOWS__
				R_AddWorldSurface(tr.world->surfaces + i, tr.currentEntityNum, 0/*tr.world->surfacesDlightBits[i]*/, 0/*tr.world->surfacesPshadowBits[i]*/, qtrue);
#endif //__PSHADOWS__
				//tr.refdef.dlightMask |= tr.world->surfacesDlightBits[i];

#ifdef __RENDERER_FOLIAGE__
				R_AddFoliage(tr.world->surfaces + i);
#endif //__RENDERER_FOLIAGE__
			}
		}

#ifdef __RENDERER_THREADING__
#pragma omp parallel for if (r_multithread->integer && tr.world->numMergedSurfaces > 128) num_threads(r_multithread->integer)
#endif
		for (i = 0; i < tr.world->numMergedSurfaces; i++)
		{
			if (tr.world->mergedSurfacesViewCount[i] != tr.viewCount)
				continue;

#if 1
			if (!(tr.world->mergedSurfaces + i)->shader->isSky)
			{// Skip everything except sky when drawing sky cubes...
				if (tr.viewParms.flags & VPF_SKYCUBEDAY) {
					continue;
				}

				if (tr.viewParms.flags & VPF_SKYCUBENIGHT) {
					continue;
				}
			}
#endif

#ifdef __PSHADOWS__
			R_AddWorldSurface( tr.world->mergedSurfaces + i, tr.currentEntityNum, 0/*tr.world->mergedSurfacesDlightBits[i]*/, tr.world->mergedSurfacesPshadowBits[i], qtrue );
#else //!__PSHADOWS__
			R_AddWorldSurface(tr.world->mergedSurfaces + i, tr.currentEntityNum, 0/*tr.world->mergedSurfacesDlightBits[i]*/, 0/*tr.world->mergedSurfacesPshadowBits[i]*/, qtrue);
#endif //__PSHADOWS__
			//tr.refdef.dlightMask |= tr.world->mergedSurfacesDlightBits[i];

#ifdef __RENDERER_FOLIAGE__
			R_AddFoliage(tr.world->mergedSurfaces + i);
#endif //__RENDERER_FOLIAGE__
		}

		/*
		#ifdef __RENDERER_FOLIAGE__
		ri->Printf(PRINT_WARNING, "%i foliage surfaces added.\n", FOLIAGE_NUM_SURFACES);
		#endif //__RENDERER_FOLIAGE__
		*/

		//tr.refdef.dlightMask = ~tr.refdef.dlightMask;
	}

}