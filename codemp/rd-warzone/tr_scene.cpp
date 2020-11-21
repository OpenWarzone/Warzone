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

int			r_firstSceneDrawSurf;

int			r_numdlights;
int			r_firstSceneDlight;

int64_t		r_numentities;
int			r_firstSceneEntity;

int			r_numpolys;
int			r_firstScenePoly;

int			r_numpolyverts;


extern qboolean CLOSE_LIGHTS_UPDATE;


/*
====================
R_InitNextFrame

====================
*/
void R_InitNextFrame(void) {
	backEndData->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;

	backEnd.localPlayerValid = qfalse;
	backEnd.humanoidOriginsNum = 0;
}


/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene(void) {
	r_firstSceneDlight = r_numdlights;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly = r_numpolys;
}

#ifdef __INDOOR_OUTDOOR_CULLING__
extern int ENABLE_INDOOR_OUTDOOR_SYSTEM;
extern qboolean INDOOR_BRUSH_FOUND;

void Indoors_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, const int passEntityNum, const int contentmask)
{
	results->entityNum = ENTITYNUM_NONE;
	ri->CM_BoxTrace(results, start, end, mins, maxs, 0, contentmask, 0);
	results->entityNum = results->fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
}

void R_CheckIfOutside(void)
{
	if (!tr.worldLoaded) return;

	const float outside_check_max_height = 4096.0;// 524288.0;
//#ifndef __INDOOR_SHADOWS__
//	if (!ENABLE_INDOOR_OUTDOOR_SYSTEM) return;
//	if (!INDOOR_BRUSH_FOUND) return;
//#endif //__INDOOR_SHADOWS__

	/*if (backEnd.viewIsOutdoorsCheckTime > backEnd.refdef.time - 1000)
	{// Wait before next check...
		return;
	}*/

	backEnd.viewIsOutdoorsCheckTime = backEnd.refdef.time;

	// Trace for sky above...
	trace_t trace;
	vec3_t start, end;

	VectorCopy(backEnd.localPlayerOrigin, start);
	start[2] += 16.0;

	VectorCopy(backEnd.localPlayerOrigin, end);
	end[2] += outside_check_max_height;

	Indoors_Trace(&trace, start, NULL, NULL, end, backEnd.localPlayerEntityNum, (CONTENTS_SOLID | CONTENTS_TERRAIN));

	VectorCopy(trace.endpos, backEnd.viewIsOutdoorsHitPosition);

	if ((trace.surfaceFlags & SURF_SKY) || trace.fraction == 1.0)
	{// Sky seen...
		//ri->Printf(PRINT_WARNING, "You are outside.\n");
		backEnd.viewIsOutdoors = qtrue;
		VectorCopy(trace.endpos, backEnd.viewIsOutdoorsHitPosition);
		return;
	}

	// Second attempt...
	VectorCopy(backEnd.localPlayerOrigin, end);
	end[2] += outside_check_max_height;
	end[0] += 512.0;

	Indoors_Trace(&trace, start, NULL, NULL, end, backEnd.localPlayerEntityNum, (CONTENTS_SOLID | CONTENTS_TERRAIN));

	if ((trace.surfaceFlags & SURF_SKY) || trace.fraction == 1.0)
	{// Sky seen...
		//ri->Printf(PRINT_WARNING, "You are outside.\n");
		backEnd.viewIsOutdoors = qtrue;
		VectorCopy(trace.endpos, backEnd.viewIsOutdoorsHitPosition);
		return;
	}

	// Third attempt...
	VectorCopy(backEnd.localPlayerOrigin, end);
	end[2] += outside_check_max_height;
	end[1] += 512.0;

	Indoors_Trace(&trace, start, NULL, NULL, end, backEnd.localPlayerEntityNum, (CONTENTS_SOLID | CONTENTS_TERRAIN));

	if ((trace.surfaceFlags & SURF_SKY) || trace.fraction == 1.0)
	{// Sky seen...
		//ri->Printf(PRINT_WARNING, "You are outside.\n");
		backEnd.viewIsOutdoors = qtrue;
		VectorCopy(trace.endpos, backEnd.viewIsOutdoorsHitPosition);
		return;
	}

	// Fourth attempt...
	VectorCopy(backEnd.localPlayerOrigin, end);
	end[2] += outside_check_max_height;
	end[0] -= 512.0;

	Indoors_Trace(&trace, start, NULL, NULL, end, backEnd.localPlayerEntityNum, (CONTENTS_SOLID | CONTENTS_TERRAIN));

	if ((trace.surfaceFlags & SURF_SKY) || trace.fraction == 1.0)
	{// Sky seen...
		//ri->Printf(PRINT_WARNING, "You are outside.\n");
		backEnd.viewIsOutdoors = qtrue;
		VectorCopy(trace.endpos, backEnd.viewIsOutdoorsHitPosition);
		return;
	}

	// Fifth attempt...
	VectorCopy(backEnd.localPlayerOrigin, end);
	end[2] += outside_check_max_height;
	end[1] -= 512.0;

	Indoors_Trace(&trace, start, NULL, NULL, end, backEnd.localPlayerEntityNum, (CONTENTS_SOLID | CONTENTS_TERRAIN));

	if ((trace.surfaceFlags & SURF_SKY) || trace.fraction == 1.0)
	{// Sky seen...
		//ri->Printf(PRINT_WARNING, "You are outside.\n");
		backEnd.viewIsOutdoors = qtrue;
		VectorCopy(trace.endpos, backEnd.viewIsOutdoorsHitPosition);
		return;
	}

	// Didn't see any sky...
	//ri->Printf(PRINT_WARNING, "You are inside.\n");
	backEnd.viewIsOutdoors = qfalse;
}

qboolean R_IndoorOutdoorCull(shader_t *shader)
{
	if (!ENABLE_INDOOR_OUTDOOR_SYSTEM) return qfalse;
	if (!INDOOR_BRUSH_FOUND) return qfalse;

	if (shader->isPortal) return qfalse;
	if (shader->isSky) return qfalse;
	//if (backEnd.currentEntity == &backEnd.entity2D) return qfalse;
	
	for (int stage = 0; stage <= shader->maxStage && stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = shader->stages[stage];

		if (!pStage) break;
		if (!pStage->active) continue;

		if (pStage->stateBits & GLS_DEPTHTEST_DISABLE)
		{// 2D probably, don't cull...
			return qfalse;
		}
	}

	if (backEnd.viewIsOutdoors && shader->isIndoor)
	{// Is indoor shader and we are outdoors, cull...
		return qtrue;
	}

	if (!backEnd.viewIsOutdoors && !shader->isIndoor)
	{// Is outdoor shader and we are indoors, cull...
		return qtrue;
	}

	return qfalse;
}
#endif //__INDOOR_OUTDOOR_CULLING__

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces(void) {
	int			i;
	shader_t	*sh;
	srfPoly_t	*poly;
	int		fogMask;

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;
	fogMask = !((tr.refdef.rdflags & RDF_NOFOG) == 0);

#ifdef __SORT_POLYS__
	//
	// Since polys are based on world matrix, we should be able to sort them by shader and allow them to merge with previous polys as much as possible...
	//

	// First make a list of shader usage...
	poly = tr.refdef.polys;

	int			numShaders = 0;
	qhandle_t	shaders[65536];

	for (i = 0; i < tr.refdef.numPolys; i++)
	{
		if (poly)
		{
			int found = -1;

			for (int p = 0; p < numShaders; p++)
			{
				if (shaders[numShaders] == poly->hShader)
				{
					found = p;
					break;
				}
			}

			if (found < 0)
			{
				shaders[numShaders] = poly->hShader;
			}
		}

		poly++;
	}

	// Now draw each shader in order, so they can be merged in drawing...
	for (int p = 0; p < numShaders; p++)
	{
		poly = tr.refdef.polys;

		for (i = 0; i < tr.refdef.numPolys; i++)
		{
			if (poly)
			{
				if (poly->hShader == shaders[p])
				{
					surfaceType_t *ply = (surfaceType_t *)poly;

					if (Distance(poly->verts[0].xyz, backEnd.refdef.vieworg) <= backEnd.viewParms.zFar)
					{
						sh = R_GetShaderByHandle(poly->hShader);
						
						R_AddDrawSurf(ply, sh,
#ifdef __Q3_FOG__
							poly->fogIndex & fogMask,
#else //!__Q3_FOG__
							0,
#endif //__Q3_FOG__
							qfalse, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0 /* cubemapIndex */, qfalse);
					}
				}
			}

			poly++;
		}
	}
#else //!__SORT_POLYS__
	poly = tr.refdef.polys;

	for (i = 0; i < tr.refdef.numPolys; i++)
	{
		if (poly)
		{
			if (Distance(poly->verts[0].xyz, backEnd.refdef.vieworg) <= backEnd.viewParms.zFar)
			{
				sh = R_GetShaderByHandle(poly->hShader);
				R_AddDrawSurf((surfaceType_t *)poly, sh,
#ifdef __Q3_FOG__
					poly->fogIndex & fogMask,
#else //!__Q3_FOG__
					0,
#endif //__Q3_FOG__
					qfalse, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0 /* cubemapIndex */, qfalse);
			}
		}

		poly++;
	}
#endif //__SORT_POLYS__
}

//#define __MERGE_POLYS__

#ifdef __MERGE_POLYS__
srfPoly_t *RE_FindPolyForShader(qhandle_t shader)
{// So, let's try to merge all these efx polys together...
	for (int i = 0; i < r_numpolys; i++)
	{
		srfPoly_t *poly = &backEndData->polys[i];
		
		if (poly && poly->hShader == shader)
		{
			//ri->Printf(PRINT_WARNING, "Reused old poly cache for shader %s.\n", tr.shaders[shader]->name);
			return poly;
		}
	}

	//ri->Printf(PRINT_WARNING, "Created new poly cache for shader %s.\n", tr.shaders[shader]->name);

	srfPoly_t *poly = &backEndData->polys[r_numpolys];
	poly->surfaceType = SF_POLY;
	poly->hShader = shader;
	poly->numVerts = 0;
	poly->verts = &backEndData->polyVerts[r_numpolyverts];

	// done.
	r_numpolys++;

	return poly;
}
#endif //__MERGE_POLYS__

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys) {
	srfPoly_t	*poly;
	int			/*i,*/ j;
	int			fogIndex;
	//fog_t		*fog;
	//vec3_t		bounds[2];

	if (!tr.registered) {
		return;
	}

	if (!hShader) {
		// This isn't a useful warning, and an hShader of zero isn't a null shader, it's
		// the default shader.
		//ri->Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		//return;
	}

	for (j = 0; j < numPolys; j++) {
		if (r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys) {
			/*
			NOTE TTimo this was initially a PRINT_WARNING
			but it happens a lot with high fighting scenes and particles
			since we don't plan on changing the const and making for room for those effects
			simply cut this message to developer only
			*/
			ri->Printf(PRINT_WARNING, "WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
			return;
		}

#ifdef __MERGE_POLYS__
		poly = RE_FindPolyForShader(hShader);

		/*for (int z = 0; z < numVerts; z++)
		{
			//poly->verts[poly->numVerts + z] = verts[(numVerts*j)+z];
			Com_Memcpy(&poly->verts[poly->numVerts + z], &verts[(numVerts*j)+z], sizeof(*verts));
		}*/
		Com_Memcpy(&poly->verts[poly->numVerts], &verts[numVerts*j], numVerts * sizeof(*verts));

		poly->numVerts += numVerts;
		
		// done.
		r_numpolyverts += numVerts;
#else //!__MERGE_POLYS__
		poly = &backEndData->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData->polyVerts[r_numpolyverts];

		Com_Memcpy(poly->verts, &verts[numVerts*j], numVerts * sizeof(*verts));

		// done.
		r_numpolys++;
		r_numpolyverts += numVerts;
#endif //__MERGE_POLYS__

#ifndef __Q3_FOG__
		/*
		// find which fog volume the poly is in
		VectorCopy(poly->verts[0].xyz, bounds[0]);
		VectorCopy(poly->verts[0].xyz, bounds[1]);
		for (i = 1; i < poly->numVerts; i++) {
			AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
		}
		*/
		fogIndex = 0;
#else //!__Q3_FOG__
		// if no world is loaded
		if (1) {
			fogIndex = 0;
		} else 
		if (tr.world == NULL) {
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if (tr.world->numfogs == 1) {
			fogIndex = 0;
		}
		else {
			// find which fog volume the poly is in
			VectorCopy(poly->verts[0].xyz, bounds[0]);
			VectorCopy(poly->verts[0].xyz, bounds[1]);
			for (i = 1; i < poly->numVerts; i++) {
				AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
			}
			for (fogIndex = 1; fogIndex < tr.world->numfogs; fogIndex++) {
				fog = &tr.world->fogs[fogIndex];
				if (bounds[1][0] >= fog->bounds[0][0]
					&& bounds[1][1] >= fog->bounds[0][1]
					&& bounds[1][2] >= fog->bounds[0][2]
					&& bounds[0][0] <= fog->bounds[1][0]
					&& bounds[0][1] <= fog->bounds[1][1]
					&& bounds[0][2] <= fog->bounds[1][2]) {
					break;
				}
			}
			if (fogIndex == tr.world->numfogs) {
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
#endif //__Q3_FOG__
	}
}


//=================================================================================

//#define __PVS_CULL__ // UQ1: Testing...

#ifdef __HUMANOIDS_BEND_GRASS__
extern qboolean TR_InFOV(vec3_t spot, vec3_t from);

void RE_AddCloseHumanoidOrigin(const vec3_t origin)
{
	float entityDistance = Distance(origin, backEnd.refdef.vieworg);

	if (entityDistance > 1024.0)
	{// Skip it...
		return;
	}

	/*if (!TR_InFOV((float *)origin, backEnd.refdef.vieworg))
	{// Not on screen...
		return;
	}*/

	if (backEnd.humanoidOriginsNum < MAX_GRASSBEND_HUMANOIDS)
	{// Free space in the list, just add it at the end...
		VectorCopy(origin, backEnd.humanoidOrigins[backEnd.humanoidOriginsNum]);
		backEnd.humanoidOriginsNum++;
		return;
	}

	// List is full, find the furthest option...
	int furthest = -1;
	float furthestDist = 0.0;

	for (int i = 0; i < MAX_GRASSBEND_HUMANOIDS; i++)
	{
		float dist = Distance(backEnd.humanoidOrigins[i], backEnd.refdef.vieworg);
		
		if (dist > furthestDist)
		{
			furthest = i;
			furthestDist = dist;
		}
	}

	if (furthest != -1)
	{// We found a furthest option, check if this new position is closer, if so, replace the old one...
		if (entityDistance < furthestDist)
		{
			VectorCopy(origin, backEnd.humanoidOrigins[furthest]);
		}
	}
}
#endif //__HUMANOIDS_BEND_GRASS__

/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene(const refEntity_t *ent) {
	vec3_t cross;

	if (!tr.registered) {
		return;
	}
	if (r_numentities >= MAX_REFENTITIES) {
#ifdef __DEVELOPER_MODE__
		ri->Printf(PRINT_DEVELOPER, "RE_AddRefEntityToScene: Dropping refEntity, reached MAX_REFENTITIES\n");
#endif //__DEVELOPER_MODE__
		return;
	}

	if (Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) || Q_isnan(ent->origin[2])) {
		static qboolean firstTime = qtrue;
		if (firstTime) {
			firstTime = qfalse;
			ri->Printf(PRINT_WARNING, "RE_AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}
	if ((int)ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE) {
		ri->Error(ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType);
	}

#ifdef __PVS_CULL__
	{
		byte mask;

		if (!ent->ignoreCull && !R_inPVS( tr.refdef.vieworg, ent->origin, &mask )) {
			return;
		}
	}
#endif //__PVS_CULL__

	backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numentities].lightingCalculated = qfalse;

	CrossProduct(ent->axis[0], ent->axis[1], cross);
	backEndData->entities[r_numentities].mirrored = (qboolean)(DotProduct(ent->axis[2], cross) < 0.f);

	if (ent->isLocalPlayer)
	{
		VectorCopy(ent->origin, backEnd.localPlayerOrigin);
		backEnd.localPlayerGameEntityNum = ent->localPlayerGameEntityNum;
		backEnd.localPlayerEntityNum = r_numentities;
		backEnd.localPlayerEntity = &backEndData->entities[r_numentities];
		backEnd.localPlayerValid = qtrue;

		// Send inventory list to GUI as pointers...
		extern void GUI_SetPlayerInventory(uint16_t *inventory, uint16_t *inventoryMod1, uint16_t *inventoryMod2, uint16_t *inventoryMod3, int *inventoryEquipped);
		GUI_SetPlayerInventory(ent->playerInventory, ent->playerInventoryMod1, ent->playerInventoryMod2, ent->playerInventoryMod3, ent->playerEquipped);
	}
#ifdef __HUMANOIDS_BEND_GRASS__
	else if (ent->isHumanoid)
	{// Record this humanoid (player/NPC to origins list)...
		RE_AddCloseHumanoidOrigin(ent->origin);
	}
#endif //__HUMANOIDS_BEND_GRASS__

	r_numentities++;
}

/*
=====================
RE_AddMiniRefEntityToScene

1:1 with how vanilla does it --eez
=====================
*/
void RE_AddMiniRefEntityToScene(const miniRefEntity_t *miniRefEnt) {
	refEntity_t entity;
	if (!tr.registered)
		return;
	if (!miniRefEnt)
		return;

#ifdef __PVS_CULL__
	{
		byte mask;

		if (!R_inPVS( tr.refdef.vieworg, miniRefEnt->origin, &mask )) {
			//PVS_CULL_COUNT++;
			return;
		}
	}
#endif //__PVS_CULL__

	memset(&entity, 0, sizeof(entity));
	memcpy(&entity, miniRefEnt, sizeof(*miniRefEnt));
	RE_AddRefEntityToScene(&entity);
}


/*
=====================
RE_AddDynamicLightToScene

=====================
*/
void RE_AddDynamicLightToScene(const vec3_t org, float intensity, float r, float g, float b, int additive, qboolean isGlowBased, float heightScale, float coneAngle, vec3_t coneDirection) {
	dlight_t	*dl;

	if (!tr.registered) {
		return;
	}
	if (r_numdlights >= MAX_DLIGHTS) {
		return;
	}
	/*if ( intensity <= 0 ) {
		return;
		}*/ // UQ1: negative now means volumetric
	//assert(0);
	dl = &backEndData->dlights[r_numdlights++];
	VectorCopy(org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->additive = additive;
	dl->isGlowBased = isGlowBased;
	dl->heightScale = heightScale;
	dl->coneAngle = coneAngle;
	VectorCopy(coneDirection, dl->coneDirection);
}

extern void R_AddLightVibrancy(float *color, float vibrancy);

/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
	/* UQ1: Additional vibrancy for lighting */
	vec3_t color = { r, g, b };
	
	//R_AddLightVibrancy(color, 0.5);
	//VectorNormalize(color);
	
	//float gMax = max(color[0], max(color[1], color[2]));
	//color[0] /= gMax;
	//color[1] /= gMax;
	//color[2] /= gMax;

	vec3_t cd;
	VectorClear(cd);
	RE_AddDynamicLightToScene(org, intensity, color[0], color[1], color[2], qfalse, qfalse, 0, 0, cd);
}

/*
=====================
RE_AddAdditiveLightToScene

=====================
*/
void RE_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b) {
	/* UQ1: Additional vibrancy for lighting */
	vec3_t color = { r, g, b };
	
	//R_AddLightVibrancy(color, 0.5);
	//VectorNormalize(color);
	
	//float gMax = max(color[0], max(color[1], color[2]));
	//color[0] /= gMax;
	//color[1] /= gMax;
	//color[2] /= gMax;

	vec3_t cd;
	VectorClear(cd);
	RE_AddDynamicLightToScene(org, intensity, color[0], color[1], color[2], qtrue, qfalse, 0, 0, cd);
}

#ifdef __DAY_NIGHT__
int DAY_NIGHT_UPDATE_TIME = 0;

float DAY_NIGHT_SUN_DIRECTION = 0.0;
float DAY_NIGHT_MOON_DIRECTION = 0.0;
float DAY_NIGHT_CURRENT_TIME = 0.0;
float DAY_NIGHT_AMBIENT_SCALE = 0.0;
vec4_t DAY_NIGHT_AMBIENT_COLOR_ORIGINAL;
vec4_t DAY_NIGHT_AMBIENT_COLOR_CURRENT;

float DAY_NIGHT_24H_TIME = 0.0;

float RB_NightScale ( void )
{
	if (!DAY_NIGHT_CYCLE_ENABLED || !tr.worldLoaded)
	{
		return 0.0;
	}

#define SUNRISE_TIME 8.2//r_testvalue0->value
#define SUNSET_TIME 21.8//r_testvalue1->value

	if (DAY_NIGHT_24H_TIME >= SUNRISE_TIME-2.0 && DAY_NIGHT_24H_TIME <= SUNRISE_TIME)
	{// Sunrise...
		return (SUNRISE_TIME - DAY_NIGHT_24H_TIME) / 2.0;
	}

	if (DAY_NIGHT_24H_TIME >= SUNSET_TIME-2.0 && DAY_NIGHT_24H_TIME <= SUNSET_TIME)
	{// Sunset...
		return 1.0 - ((SUNSET_TIME - DAY_NIGHT_24H_TIME) / 2.0);
	}

	if (DAY_NIGHT_24H_TIME >= SUNRISE_TIME-1.5 && DAY_NIGHT_24H_TIME <= SUNSET_TIME-2.0)
	{// Daytime...
		return 0.0;
	}

	// Night time...
	return 1.0;
}

extern float DAY_NIGHT_CYCLE_SPEED;
extern float DAY_NIGHT_START_TIME;

extern qboolean SUN_VISIBLE;
extern vec3_t SUN_POSITION;
extern vec2_t SUN_SCREEN_POSITION;

extern qboolean RB_UpdateSunFlareVis(void);
extern qboolean Volumetric_Visible(vec3_t from, vec3_t to, qboolean isSun);
extern void Volumetric_RoofHeight(vec3_t from);
extern void WorldCoordToScreenCoord(vec3_t origin, float *x, float *y);

void R_ShowTime(void)
{
	ri->Printf(PRINT_ALL, "^5The current day/night ^5Day night cycle time is ^7%.4f^5.\n", DAY_NIGHT_CURRENT_TIME * 24.0);
}

 void R_OpenGLToScreen(const vec4_t v, vec4_t outScreenPos) {

	// Get the matrices and viewport
	float modelView[16];
	float projection[16];
	double viewport[4];
	double depthRange[2];

	/*
	qglGetDoublev(GL_MODELVIEW_MATRIX, modelView);
	qglGetDoublev(GL_PROJECTION_MATRIX, projection);
	qglGetDoublev(GL_VIEWPORT, viewport);
	qglGetDoublev(GL_DEPTH_RANGE, depthRange);
	*/

	{
		// FIXME: this could be a lot cleaner
		//matrix_t translation;
		//Matrix16Translation(backEnd.viewParms.ori.origin, translation);
		//Matrix16Multiply(backEnd.viewParms.world.modelViewMatrix, translation, modelView);

		//Matrix16Copy(matrix, glState.modelview);
		//Matrix16Multiply(glState.projection, glState.modelview, glState.modelviewProjection);

		Matrix16Copy(glState.modelview, modelView);
		Matrix16Copy(glState.projection, projection);
		qglGetDoublev(GL_VIEWPORT, viewport);
		qglGetDoublev(GL_DEPTH_RANGE, depthRange);
	}

	// Compose the matrices into a single row-major transformation
	vec4_t T[4];
	int r, c, i;
	for (r = 0; r < 4; ++r) {
		for (c = 0; c < 4; ++c) {
			T[r][c] = 0;
			for (i = 0; i < 4; ++i) {
				// OpenGL matrices are column major
				T[r][c] += projection[r + i * 4] * modelView[i + c * 4];
			}
		}
	}

	// Transform the vertex
	vec4_t result;
	for (r = 0; r < 4; ++r) {
		result[r] = DotProduct4(T[r], v);
	}

	// Homogeneous divide
	const double rhw = 1 / result[4];

	outScreenPos[0] = (1 + result[0] * rhw) * viewport[2] / 2 + viewport[0];
	outScreenPos[1] = (1 - result[1] * rhw) * viewport[3] / 2 + viewport[1];
	outScreenPos[2] = (result[2] * rhw) * (depthRange[1] - depthRange[0]) + depthRange[0];
	outScreenPos[3] = rhw;
}

void RB_UpdateDayNightCycle()
{
	//
	// Day/Night Cycle - Now using actual server time...
	//
	if (DAY_NIGHT_UPDATE_TIME == 0)
	{// Init base ambient color stuff...
		VectorCopy4(tr.refdef.sunAmbCol, DAY_NIGHT_AMBIENT_COLOR_ORIGINAL);
		DAY_NIGHT_UPDATE_TIME = 1;
	}

	int milli = (double(tr.refdef.time) * 16384.0 * r_dayNightCycleSpeed->value) + (DAY_NIGHT_START_TIME * 1000.0 * 60 * 60); // roughly matches old speed...

	// Hours
	int hr = (milli / (1000 * 60 * 60)) % 24;

	// Minutes
	int min = (milli / (1000 * 60)) % 60;

	// Seconds
	int sec = (milli / 1000) % 60;

	// Convert server time into a 24 hour version float...
	DAY_NIGHT_24H_TIME = (float)hr + ((float)min / 60.0) + ((float)sec / 60.0 / 60.0);
	
	// Convert 24h server time into 0.0 -> 1.0 float intervals.
	DAY_NIGHT_CURRENT_TIME = DAY_NIGHT_24H_TIME / 24.0;

	//
	// End of day/night cycle calculations...
	//


	// We need to match up real world 24 type time to sun direction... Offset...
	float adjustedTime24h = DAY_NIGHT_24H_TIME + 4.0;
	if (adjustedTime24h > 24.0) adjustedTime24h = adjustedTime24h - 24.0;
	float sTime = (adjustedTime24h / 24.0);
	DAY_NIGHT_SUN_DIRECTION = (sTime - 0.5) * 6.283185307179586476925286766559;

	// Moon is directly opposed to sun dir...
	adjustedTime24h = DAY_NIGHT_24H_TIME + 16.0;
	if (adjustedTime24h > 24.0) adjustedTime24h = adjustedTime24h - 24.0;
	sTime = (adjustedTime24h / 24.0);
	DAY_NIGHT_MOON_DIRECTION = (sTime - 0.5) * 6.283185307179586476925286766559;


	vec4_t sunColor;
	VectorCopy4(DAY_NIGHT_AMBIENT_COLOR_ORIGINAL, sunColor);
	sunColor[3] = 1.0;

	VectorCopy4(sunColor, DAY_NIGHT_AMBIENT_COLOR_CURRENT);

	//ri->Printf(PRINT_WARNING, "Day/Night timer is %.4f (%.4f). Sun dir %.4f.\n", DAY_NIGHT_24H_TIME, DAY_NIGHT_CURRENT_TIME, DAY_NIGHT_SUN_DIRECTION);



	VectorCopy4(DAY_NIGHT_AMBIENT_COLOR_CURRENT, tr.refdef.sunAmbCol);
	VectorCopy4(tr.refdef.sunAmbCol, tr.refdef.sunCol);

	{
		float a = 0.3;
		float b = DAY_NIGHT_SUN_DIRECTION;
		tr.sunDirection[0] = cos(a) * cos(b);
		tr.sunDirection[1] = sin(a) * cos(b);
		tr.sunDirection[2] = sin(b);
	}

	{
		float a = 0.3;
		float b = DAY_NIGHT_MOON_DIRECTION;
		tr.moonDirection[0] = cos(a) * cos(b);
		tr.moonDirection[1] = sin(a) * cos(b);
		tr.moonDirection[2] = sin(b);
	}

	//ri->Printf(PRINT_ALL, "sunDir %.4f moonDir %.4f\n", DAY_NIGHT_SUN_DIRECTION, DAY_NIGHT_MOON_DIRECTION);

	//
	// Update sun position info for post process stuff...
	//

	const float cutoff = 0.25f;
	float dot = DotProduct(tr.sunDirection, backEnd.viewParms.ori.axis[0]);

	//float dist;
	//vec4_t pos;// , hpos;
	matrix_t trans, model, mvp;

	Matrix16Translation(backEnd.viewParms.ori.origin, trans);
	Matrix16Multiply(backEnd.viewParms.world.modelMatrix, trans, model);
	Matrix16Multiply(backEnd.viewParms.projectionMatrix, model, mvp);

	extern void R_SetSunScreenPos(vec3_t sunDirection);
	R_SetSunScreenPos(tr.sunDirection);

	if (dot < cutoff)
	{
		SUN_VISIBLE = qfalse;
		return;
	}

	if (!RB_UpdateSunFlareVis())
	{
		SUN_VISIBLE = qfalse;
		return;
	}

	SUN_VISIBLE = qtrue;
}

extern void R_LocalPointToWorld(const vec3_t local, vec3_t world);
extern void R_WorldToLocal(const vec3_t world, vec3_t local);
#else
float RB_NightScale(void)
{
	return 0.0;
}
#endif //__DAY_NIGHT__

extern void TR_AxisToAngles(const vec3_t axis[3], vec3_t angles);

#ifdef __RENDERER_GROUND_FOLIAGE__
extern void FOLIAGE_DrawGrass(void);
#endif //__RENDERER_GROUND_FOLIAGE__

extern void RB_AddGlowShaderLights(void);
extern void RB_UpdateCloseLights();

extern vec3_t		SUN_COLOR_MAIN;
extern vec3_t		SUN_COLOR_SECONDARY;
extern vec3_t		SUN_COLOR_TERTIARY;
extern vec3_t		SUN_COLOR_AMBIENT;

void RE_BeginScene(const refdef_t *fd)
{
#ifdef __PERFORMANCE_DEBUG_STARTUP__
	DEBUG_StartTimer("RE_BeginScene", qtrue);
#endif //__PERFORMANCE_DEBUG_STARTUP__

	Com_Memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

	//ri->Printf(PRINT_WARNING, "New scene.\n");

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy(fd->vieworg, tr.refdef.vieworg);
	VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
	VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
	VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL)) {
		int		areaDiff;
		int		i;

		// compare the area bits
		areaDiff = 0;
		for (i = 0; i < MAX_MAP_AREA_BYTES / 4; i++) {
			areaDiff |= ((int *)tr.refdef.areamask)[i] ^ ((int *)fd->areamask)[i];
			((int *)tr.refdef.areamask)[i] = ((int *)fd->areamask)[i];
		}

		if (areaDiff) {
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}

	tr.refdef.sunDir[3] = 0.0f;
	tr.refdef.sunCol[3] = 1.0f;
	tr.refdef.sunAmbCol[3] = 1.0f;

	VectorNormalize(tr.sunDirection);

	VectorCopy(tr.sunDirection, tr.refdef.sunDir);

	float colorScale = 1.0;

	if (r_forceSun->integer)
		colorScale = (r_forceSun->integer) ? r_forceSunMapLightScale->value : tr.mapLightScale;
	else if (r_sunlightMode->integer >= 2)
		colorScale = 0.75;

	tr.refdef.colorScale = colorScale;

	if (r_sunlightMode->integer >= 1)
	{
		float ambCol = 1.0;

		if (r_forceSun->integer || r_dlightShadows->integer)
			ambCol = (r_forceSun->integer) ? r_forceSunAmbientScale->value : tr.sunShadowScale;
		else if (r_sunlightMode->integer >= 2)
			ambCol = 0.75;

#ifndef __DAY_NIGHT__
		tr.refdef.sunCol[0] =
			tr.refdef.sunCol[1] =
			tr.refdef.sunCol[2] = 1.0f;

		tr.refdef.sunAmbCol[0] =
			tr.refdef.sunAmbCol[1] =
			tr.refdef.sunAmbCol[2] = ambCol;
#else //__DAY_NIGHT__
		if (DAY_NIGHT_CYCLE_ENABLED)
		{
			/*
			tr.refdef.sunAmbCol[0] *= ambCol;
			tr.refdef.sunAmbCol[1] *= ambCol;
			tr.refdef.sunAmbCol[2] *= ambCol;
			*/

			RB_UpdateDayNightCycle();
			VectorNormalize(tr.sunDirection);
			VectorCopy(tr.sunDirection, tr.refdef.sunDir);

			/*
			tr.refdef.sunCol[0] =
				tr.refdef.sunCol[1] =
				tr.refdef.sunCol[2] = 1.0f;

			tr.refdef.sunAmbCol[0] =
				tr.refdef.sunAmbCol[1] =
				tr.refdef.sunAmbCol[2] = ambCol;
			*/

			VectorCopy(SUN_COLOR_MAIN, tr.refdef.sunCol);
			VectorCopy(SUN_COLOR_AMBIENT, tr.refdef.sunAmbCol);
		}
		else
		{
			/*
			tr.refdef.sunCol[0] =
				tr.refdef.sunCol[1] =
				tr.refdef.sunCol[2] = 1.0f;

			tr.refdef.sunAmbCol[0] =
				tr.refdef.sunAmbCol[1] =
				tr.refdef.sunAmbCol[2] = ambCol;
			*/

			VectorCopy(SUN_COLOR_MAIN, tr.refdef.sunCol);
			VectorCopy(SUN_COLOR_AMBIENT, tr.refdef.sunAmbCol);
		}
#endif //__DAY_NIGHT__
	}
	else
	{
		float scale = pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits - 8);
		if (r_sunlightMode->integer/*r_forceSun->integer || r_dlightShadows->integer*/)
		{
			VectorScale(tr.sunLight, scale * r_forceSunLightScale->value, tr.refdef.sunCol);
			VectorScale(tr.sunLight, scale * r_forceSunAmbientScale->value, tr.refdef.sunAmbCol);
		}
		else
		{
			VectorScale(tr.sunLight, scale, tr.refdef.sunCol);
			VectorScale(tr.sunLight, scale * tr.sunShadowScale, tr.refdef.sunAmbCol);
		}
	}

	if (r_forceAutoExposure->integer)
	{
		tr.refdef.autoExposureMinMax[0] = r_forceAutoExposureMin->value;
		tr.refdef.autoExposureMinMax[1] = r_forceAutoExposureMax->value;
	}
	else
	{
		tr.refdef.autoExposureMinMax[0] = tr.autoExposureMinMax[0];
		tr.refdef.autoExposureMinMax[1] = tr.autoExposureMinMax[1];
	}

	if (r_forceToneMap->integer)
	{
		tr.refdef.toneMinAvgMaxLinear[0] = pow(2, r_forceToneMapMin->value);
		tr.refdef.toneMinAvgMaxLinear[1] = pow(2, r_forceToneMapAvg->value);
		tr.refdef.toneMinAvgMaxLinear[2] = pow(2, r_forceToneMapMax->value);
	}
	else
	{
		tr.refdef.toneMinAvgMaxLinear[0] = pow(2, tr.toneMinAvgMaxLevel[0]);
		tr.refdef.toneMinAvgMaxLinear[1] = pow(2, tr.toneMinAvgMaxLevel[1]);
		tr.refdef.toneMinAvgMaxLinear[2] = pow(2, tr.toneMinAvgMaxLevel[2]);
	}

	// Makro - copy exta info if present
	if (fd->rdflags & RDF_EXTRA) {
		const refdefex_t* extra = (const refdefex_t*)(fd + 1);

		tr.refdef.blurFactor = extra->blurFactor;

		if (fd->rdflags & RDF_SUNLIGHT)
		{
			VectorCopy(extra->sunDir, tr.refdef.sunDir);
			VectorCopy(extra->sunCol, tr.refdef.sunCol);
			VectorCopy(extra->sunAmbCol, tr.refdef.sunAmbCol);
		}
	}
	else
	{
		tr.refdef.blurFactor = 0.0f;
	}

	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

	backEnd.refdef.dlights = tr.refdef.dlights;
	backEnd.refdef.num_dlights = tr.refdef.num_dlights;

	RB_AddGlowShaderLights();
	RB_UpdateCloseLights();

#ifdef __INDOOR_OUTDOOR_CULLING__
//#ifndef __INDOOR_SHADOWS__
//	if (ENABLE_INDOOR_OUTDOOR_SYSTEM && INDOOR_BRUSH_FOUND)
//#endif //__INDOOR_SHADOWS__
	{
		if (ENABLE_INDOOR_OUTDOOR_SYSTEM > 1)
			ri->Printf(PRINT_WARNING, "%i inside or outside surfaces were culled last frame. %i were not culled.\n", backEnd.viewIsOutdoorsCulledCount, backEnd.viewIsOutdoorsNotCulledCount);

		backEnd.viewIsOutdoorsCulledCount = 0;
		backEnd.viewIsOutdoorsNotCulledCount = 0;
		R_CheckIfOutside();
	}
#endif //__INDOOR_OUTDOOR_CULLING__

	// Add the decals here because decals add polys and we need to ensure
	// that the polys are added before the the renderer is prepared
	if ( !(tr.refdef.rdflags & RDF_NOWORLDMODEL) )
		R_AddDecals();

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

#ifdef __PSHADOWS__
	tr.refdef.num_pshadows = 0;
	tr.refdef.pshadows = &backEndData->pshadows[0];
#endif

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if ((/*r_dynamiclight->integer == 0 ||*/ r_vertexLight->integer == 1)) {
		tr.refdef.num_dlights = 0;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	// UQ1: Set refdef.viewangles... Hopefully this place is good enough to do it?!?!?!?
	TR_AxisToAngles(tr.refdef.viewaxis, tr.refdef.viewangles);

#ifdef __RENDERER_GROUND_FOLIAGE__
	//if (backEnd.depthFill && !(tr.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		FOLIAGE_DrawGrass();
	}
#endif //__RENDERER_GROUND_FOLIAGE__

#ifdef __PERFORMANCE_DEBUG_STARTUP__
	DEBUG_EndTimer(qtrue);
#endif //__PERFORMANCE_DEBUG_STARTUP__
}

void RE_EndScene()
{
	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;

#ifdef __INVERSE_DEPTH_BUFFERS__
	qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	qglClearDepth(0.0f);
#else //!__INVERSE_DEPTH_BUFFERS__
	qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	qglClearDepth(1.0f);
#endif //__INVERSE_DEPTH_BUFFERS__
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/

int NEXT_SHADOWMAP_UPDATE[6] = { { 0 } };
vec3_t SHADOWMAP_LAST_VIEWANGLES[6] = { { 0 } };
vec3_t SHADOWMAP_LAST_VIEWORIGIN[6] = { { -999999.9f } }; // -999999.9 to force update on first draw...

extern void Volumetric_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, const int passEntityNum, const int contentmask);

qboolean	TRACE_HIT_SKY;
vec3_t		TRACE_ROOF;

void RE_FindRoof(vec3_t from)
{
	TRACE_HIT_SKY = qfalse;

	trace_t trace;
	vec3_t roofh;
	VectorSet(roofh, from[0], from[1], from[2] + 128000);
	Volumetric_Trace(&trace, from, NULL, NULL, roofh, -1, (CONTENTS_SOLID | CONTENTS_TERRAIN));
	VectorSet(TRACE_ROOF, trace.endpos[0], trace.endpos[1], trace.endpos[2] - 8.0);

	if (trace.surfaceFlags & SURF_SKY)
	{
		TRACE_HIT_SKY = qtrue;
	}
}


#ifdef __DEBUG_BINDS__
int SCENE_FRAME_NUMBER = 0;

#ifdef __DEBUG_FBO_BINDS__
int FBO_BINDS_COUNT = 0;
#endif //__DEBUG_FBO_BINDS__

#ifdef __DEBUG_GLSL_BINDS__
int GLSL_BINDS_COUNT = 0;
#endif //__DEBUG_GLSL_BINDS__

void RB_UpdateDebuggingInfo(void)
{
	SCENE_FRAME_NUMBER++;

	if (SCENE_FRAME_NUMBER > 32768) SCENE_FRAME_NUMBER = 0; // For sanity, just loop back to zero...

	if (r_debugBinds->integer)
	{
#ifdef __DEBUG_FBO_BINDS__
		FBO_BINDS_COUNT = 0;
#endif //__DEBUG_FBO_BINDS__

#ifdef __DEBUG_GLSL_BINDS__
		GLSL_BINDS_COUNT = 0;
#endif //__DEBUG_GLSL_BINDS__
	}
}
#endif //__DEBUG_BINDS__

void RE_RenderScene(const refdef_t *fd) {
	viewParms_t		parms;
	int				startTime;

	if (!tr.registered) {
		return;
	}

	// Set main world data pointer...
	tr.world = tr.worldSolid;

#ifdef __PERFORMANCE_DEBUG_STARTUP__
	DEBUG_StartTimer("RE_RenderScene", qtrue);
#endif //__PERFORMANCE_DEBUG_STARTUP__

	GLimp_LogComment("====== RE_RenderScene =====\n");

	if (r_norefresh->integer) {
		return;
	}

	if (!tr.world || fd->rdflags & RDF_NOWORLDMODEL)
	{
		ALLOW_NULL_FBO_BIND = qtrue;
	}
	else
	{
		ALLOW_NULL_FBO_BIND = qfalse;
	}

#ifdef __DEBUG_BINDS__
	RB_UpdateDebuggingInfo();
#endif //__DEBUG_BINDS__

	startTime = ri->Milliseconds();

	if (!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL)) {
		ri->Error(ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	CLOSE_LIGHTS_UPDATE = qtrue;
	RE_BeginScene(fd);

#ifdef __PSHADOWS__
	/* playing with more shadows */
	if(!( fd->rdflags & RDF_NOWORLDMODEL ) && r_shadows->integer == 2)
	{
		R_RenderPshadowMaps(fd);
	}
#endif

	// playing with even more shadows
	if (!(fd->rdflags & RDF_NOWORLDMODEL)
//#ifdef __VR_SEPARATE_EYE_RENDER__
//		&& (backEnd.stereoFrame == STEREO_CENTER || backEnd.stereoFrame == STEREO_LEFT) // Only need to render shadows once for 1st eye drawn...
//#endif //__VR_SEPARATE_EYE_RENDER__
		&& (r_sunlightMode->integer >= 2 || r_forceSun->integer || tr.sunShadows)
		&& !backEnd.depthFill
		&& SHADOWS_ENABLED
		&& RB_NightScale() < 1.0 // Can ignore rendering shadows at night...
		&& (r_deferredLighting->integer || r_fastLighting->integer))
	{
		vec4_t lightDir;

		qboolean FBO_SWITCHED = qfalse;
#ifdef __VR_SEPARATE_EYE_RENDER__
		if (glState.currentFBO == tr.renderFbo || glState.currentFBO == tr.renderLeftVRFbo || glState.currentFBO == tr.renderRightVRFbo)
#else //!__VR_SEPARATE_EYE_RENDER__
		if (glState.currentFBO == tr.renderFbo)
#endif //__VR_SEPARATE_EYE_RENDER__
		{// Skip outputting to deferred textures while doing depth prepass, by using a depth prepass FBO without any attached textures.
			glState.previousFBO = glState.currentFBO;
			FBO_Bind(tr.renderDepthFbo);
#ifdef __VR_SEPARATE_EYE_RENDER__
			qglClearColor(0, 0, 0, 1);
			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif //__VR_SEPARATE_EYE_RENDER__
			FBO_SWITCHED = qtrue;
		}

		/*if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer)
		{// Override occlusion for depth prepass and shadow pass...
			tr.viewParms.zFar = tr.occlusionOriginalZfar;
		}*/

		float lightHeight = 999999.9;
		vec3_t lightOrigin;
		VectorCopy(fd->vieworg, lightOrigin);

		if (!backEnd.viewIsOutdoors)
		{
			lightDir[0] = 0.0;
			lightDir[1] = 0.0;
			//if (r_testvalue0->integer < 1) // UQ1: ancient debug cvar left in, think this one is right...
				lightDir[2] = 1.0;
			//else
			//	lightDir[2] = -1.0;
			lightDir[3] = 0.0;
			
			VectorCopy(backEnd.viewIsOutdoorsHitPosition, lightOrigin);
			//lightOrigin[2] -= r_testvalue1->value;// 8.0;

			lightHeight = lightOrigin[2] - backEnd.localPlayerOrigin[2];
		}
		else
		{
			VectorCopy4(tr.refdef.sunDir, lightDir);
		}

		int nowTime = ri->Milliseconds();

		/* Check for forced shadow updates if viewer changes position/angles */
		qboolean forceUpdate[6] = { { qfalse } };

		for (int level = 0; level < 4; level++)
		{
			if (Distance(tr.refdef.viewangles, SHADOWMAP_LAST_VIEWANGLES[level]) > SHADOW_FORCE_UPDATE_ANGLE_CHANGE)
			{
				forceUpdate[level] = qtrue;
			}
			else if (Distance(tr.refdef.vieworg, SHADOWMAP_LAST_VIEWORIGIN[level]) > 256.0 / (5-level))
			{
				forceUpdate[level] = qtrue;
			}
		}

		// Always update close shadows, so players/npcs moving around get shadows, even if the player's view doesn't change...
		R_RenderSunShadowMaps(fd, 0, lightDir, lightHeight, lightOrigin);

		if (nowTime >= NEXT_SHADOWMAP_UPDATE[0] || forceUpdate[0])
		{// Timed updates for distant shadows, or forced by view change...
			R_RenderSunShadowMaps(fd, 1, lightDir, lightHeight, lightOrigin);

			NEXT_SHADOWMAP_UPDATE[0] = nowTime + 50;

			VectorCopy(tr.refdef.viewangles, SHADOWMAP_LAST_VIEWANGLES[0]);
			VectorCopy(tr.refdef.vieworg, SHADOWMAP_LAST_VIEWORIGIN[0]);
		}

		if (nowTime >= NEXT_SHADOWMAP_UPDATE[1] || forceUpdate[1])
		{// Timed updates for distant shadows, or forced by view change...
			R_RenderSunShadowMaps(fd, 2, lightDir, lightHeight, lightOrigin);

			NEXT_SHADOWMAP_UPDATE[1] = nowTime + 200;

			VectorCopy(tr.refdef.viewangles, SHADOWMAP_LAST_VIEWANGLES[1]);
			VectorCopy(tr.refdef.vieworg, SHADOWMAP_LAST_VIEWORIGIN[1]);
		}

		if (!(r_fastShadows->integer > 1) && (nowTime >= NEXT_SHADOWMAP_UPDATE[2] || forceUpdate[2]))
		{// Timed updates for distant shadows, or forced by view change...
			R_RenderSunShadowMaps(fd, 3, lightDir, lightHeight, lightOrigin);

			NEXT_SHADOWMAP_UPDATE[2] = nowTime + 2000;

			VectorCopy(tr.refdef.viewangles, SHADOWMAP_LAST_VIEWANGLES[2]);
			VectorCopy(tr.refdef.vieworg, SHADOWMAP_LAST_VIEWORIGIN[2]);
		}

		if (!(r_fastShadows->integer) && (nowTime >= NEXT_SHADOWMAP_UPDATE[3] || forceUpdate[3]))
		{// Timed updates for distant shadows, or forced by view change...
			R_RenderSunShadowMaps(fd, 4, lightDir, lightHeight, lightOrigin);

			NEXT_SHADOWMAP_UPDATE[3] = nowTime + 10000;

			VectorCopy(tr.refdef.viewangles, SHADOWMAP_LAST_VIEWANGLES[3]);
			VectorCopy(tr.refdef.vieworg, SHADOWMAP_LAST_VIEWORIGIN[3]);
		}

		/*if (ENABLE_OCCLUSION_CULLING && r_occlusion->integer)
		{// Set occlusion zFar again, now that depth prepass is completed...
			tr.viewParms.zFar = tr.occlusionZfar;
		}*/

		if (FBO_SWITCHED)
		{// Switch back to original FBO (renderFbo).
			FBO_Bind(glState.previousFBO);
		}
	}

	// playing with cube maps
	// this is where dynamic cubemaps would be rendered
#ifndef __REALTIME_CUBEMAP__
	if (0) //(!( fd->rdflags & RDF_NOWORLDMODEL ))
	{
		int i, j;

		for (i = 0; i < tr.numCubemaps; i++)
		{
			for (j = 0; j < 6; j++)
			{
				R_RenderCubemapSide(i, j, qtrue);
			}
		}
	}
#endif //__REALTIME_CUBEMAP__

#ifdef __REALTIME_CUBEMAP__
	if (!(fd->rdflags & RDF_NOWORLDMODEL)
		&& r_cubeMapping->integer 
		&& !r_lowVram->integer 
		&& !backEnd.depthFill
		/*&& Distance(tr.refdef.vieworg, backEnd.refdef.realtimeCubemapOrigin) > 0.0*/ /* FIXME - Skip when unneeded */)
	{
		vec3_t finalPos;
		VectorCopy(tr.refdef.vieworg, finalPos);

		VectorCopy(finalPos, tr.refdef.realtimeCubemapOrigin);
		VectorCopy(finalPos, backEnd.refdef.realtimeCubemapOrigin);

		for (int j = 0; j < 6; j++)
		{
			extern void R_RenderCubemapSideRealtime(vec3_t origin, int cubemapSide, qboolean subscene);
			R_RenderCubemapSideRealtime(finalPos, j, qtrue);
		}

		//tr.refdef.realtimeCubemapRendered = qtrue;
		//backEnd.refdef.realtimeCubemapRendered = qtrue;
	}
	else
	{
		//tr.refdef.realtimeCubemapRendered = qfalse;
		//backEnd.refdef.realtimeCubemapRendered = qfalse;
	}
#endif //__REALTIME_CUBEMAP__

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	Com_Memset(&parms, 0, sizeof(parms));
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	parms.stereoFrame = tr.refdef.stereoFrame;

	VectorCopy(fd->vieworg, parms.ori.origin);
	VectorCopy(fd->viewaxis[0], parms.ori.axis[0]);
	VectorCopy(fd->viewaxis[1], parms.ori.axis[1]);
	VectorCopy(fd->viewaxis[2], parms.ori.axis[2]);

	VectorCopy(fd->vieworg, parms.pvsOrigin);

	if (!(fd->rdflags & RDF_NOWORLDMODEL)
		&& (r_sunlightMode->integer >= 2 || r_forceSun->integer || tr.sunShadows)
		&& !backEnd.depthFill
		&& SHADOWS_ENABLED
		&& RB_NightScale() < 1.0 // Can ignore rendering shadows at night...
		&& (r_deferredLighting->integer || r_fastLighting->integer))
	{
		parms.flags = VPF_USESUNLIGHT;
	}

	parms.maxEntityRange = 512000;

	CLOSE_LIGHTS_UPDATE = qtrue;

	// Render the sold world...
	R_RenderView( &parms );

	if (!tr.world
		|| (tr.viewParms.flags & VPF_NOPOSTPROCESS)
		|| (tr.refdef.rdflags & RDF_NOWORLDMODEL)
		|| (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)
		|| (backEnd.viewParms.flags & VPF_SHADOWPASS)
		|| (backEnd.viewParms.flags & VPF_DEPTHSHADOW)
		|| backEnd.depthFill
		|| (tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		|| (tr.renderSkyFbo && backEnd.viewParms.targetFbo == tr.renderSkyFbo))
	{
		// do nothing
	}
	else
	{
#ifdef __JKA_WEATHER__
		if (r_weather->integer && !(fd->rdflags & RDF_NOWORLDMODEL))
		{
			extern void RE_RenderWorldEffects(void);
			RE_RenderWorldEffects();
		}
#endif //__JKA_WEATHER__

		R_AddPostProcessCmd();
	}

	RE_EndScene();

	SKIP_CULL_FRAME = qfalse;

	tr.frontEndMsec += ri->Milliseconds() - startTime;

#ifdef __PERFORMANCE_DEBUG_STARTUP__
	DEBUG_EndTimer(qtrue);
#endif //__PERFORMANCE_DEBUG_STARTUP__
}
