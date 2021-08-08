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
// tr_main.c -- main control flow for each frame

#include "tr_local.h"

#include <string.h> // memcpy

#include "ghoul2/g2_local.h"

extern qboolean CURRENT_DRAW_DLIGHTS_UPDATE;

trGlobals_t		tr;

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


refimport_t	*ri = NULL;

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t	entitySurface = SF_ENTITY;

qboolean SKIP_CULL_FRAME = qfalse;
qboolean SKIP_CULL_FRAME_DONE = qfalse;

/*
================
R_CompareVert
================
*/
qboolean R_CompareVert(srfVert_t * v1, srfVert_t * v2, qboolean checkST)
{
	int             i;

	for(i = 0; i < 3; i++)
	{
		if(floor(v1->xyz[i] + 0.1) != floor(v2->xyz[i] + 0.1))
		{
			return qfalse;
		}

		if(checkST && ((v1->st[0] != v2->st[0]) || (v1->st[1] != v2->st[1])))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
=============
R_CalcNormalForTriangle
=============
*/
void R_CalcNormalForTriangle(vec3_t normal, const vec3_t v0, const vec3_t v1, const vec3_t v2)
{
	vec3_t          udir, vdir;

	// compute the face normal based on vertex points
	VectorSubtract(v2, v0, udir);
	VectorSubtract(v1, v0, vdir);
	CrossProduct(udir, vdir, normal);

	VectorNormalize(normal);
}

/*
=============
R_CalcTangentsForTriangle
http://members.rogers.com/deseric/tangentspace.htm
=============
*/
void R_CalcTangentsForTriangle(vec3_t tangent, vec3_t bitangent,
							   const vec3_t v0, const vec3_t v1, const vec3_t v2,
							   const vec2_t t0, const vec2_t t1, const vec2_t t2)
{
	int             i;
	vec3_t          planes[3];
	vec3_t          u, v;

	for(i = 0; i < 3; i++)
	{
		VectorSet(u, v1[i] - v0[i], t1[0] - t0[0], t1[1] - t0[1]);
		VectorSet(v, v2[i] - v0[i], t2[0] - t0[0], t2[1] - t0[1]);

		VectorNormalize(u);
		VectorNormalize(v);

		CrossProduct(u, v, planes[i]);
	}

	//So your tangent space will be defined by this :
	//Normal = Normal of the triangle or Tangent X Bitangent (careful with the cross product,
	// you have to make sure the normal points in the right direction)
	//Tangent = ( dp(Fx(s,t)) / ds,  dp(Fy(s,t)) / ds, dp(Fz(s,t)) / ds )   or     ( -Bx/Ax, -By/Ay, - Bz/Az )
	//Bitangent =  ( dp(Fx(s,t)) / dt,  dp(Fy(s,t)) / dt, dp(Fz(s,t)) / dt )  or     ( -Cx/Ax, -Cy/Ay, -Cz/Az )

	// tangent...
	tangent[0] = -planes[0][1] / planes[0][0];
	tangent[1] = -planes[1][1] / planes[1][0];
	tangent[2] = -planes[2][1] / planes[2][0];
	VectorNormalize(tangent);

	// bitangent...
	bitangent[0] = -planes[0][2] / planes[0][0];
	bitangent[1] = -planes[1][2] / planes[1][0];
	bitangent[2] = -planes[2][2] / planes[2][0];
	VectorNormalize(bitangent);
}




/*
=============
R_CalcTangentSpace
=============
*/
void R_CalcTangentSpace(vec3_t tangent, vec3_t bitangent, vec3_t normal,
						const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1, const vec2_t t2)
{
	vec3_t          cp, u, v;
	vec3_t          faceNormal;

	VectorSet(u, v1[0] - v0[0], t1[0] - t0[0], t1[1] - t0[1]);
	VectorSet(v, v2[0] - v0[0], t2[0] - t0[0], t2[1] - t0[1]);

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[0] = -cp[1] / cp[0];
		bitangent[0] = -cp[2] / cp[0];
	}

	u[0] = v1[1] - v0[1];
	v[0] = v2[1] - v0[1];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[1] = -cp[1] / cp[0];
		bitangent[1] = -cp[2] / cp[0];
	}

	u[0] = v1[2] - v0[2];
	v[0] = v2[2] - v0[2];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[2] = -cp[1] / cp[0];
		bitangent[2] = -cp[2] / cp[0];
	}

	VectorNormalize(tangent);
	VectorNormalize(bitangent);

	// compute the face normal based on vertex points
	if ( normal[0] == 0.0f && normal[1] == 0.0f && normal[2] == 0.0f )
	{
		VectorSubtract(v2, v0, u);
		VectorSubtract(v1, v0, v);
		CrossProduct(u, v, faceNormal);
	}
	else
	{
		VectorCopy(normal, faceNormal);
	}

	VectorNormalize(faceNormal);

#if 1
	// Gram-Schmidt orthogonalize
	//tangent[a] = (t - n * Dot(n, t)).Normalize();
	VectorMA(tangent, -DotProduct(faceNormal, tangent), faceNormal, tangent);
	VectorNormalize(tangent);

	// compute the cross product B=NxT
	//CrossProduct(normal, tangent, bitangent);
#else
	// normal, compute the cross product N=TxB
	CrossProduct(tangent, bitangent, normal);
	VectorNormalize(normal);

	if(DotProduct(normal, faceNormal) < 0)
	{
		//VectorInverse(normal);
		//VectorInverse(tangent);
		//VectorInverse(bitangent);

		// compute the cross product T=BxN
		CrossProduct(bitangent, faceNormal, tangent);

		// compute the cross product B=NxT
		//CrossProduct(normal, tangent, bitangent);
	}
#endif

	VectorCopy(faceNormal, normal);
}

void R_CalcTangentSpaceFast(vec3_t tangent, vec3_t bitangent, vec3_t normal,
						const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1, const vec2_t t2)
{
	vec3_t          cp, u, v;
	vec3_t          faceNormal;

	VectorSet(u, v1[0] - v0[0], t1[0] - t0[0], t1[1] - t0[1]);
	VectorSet(v, v2[0] - v0[0], t2[0] - t0[0], t2[1] - t0[1]);

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[0] = -cp[1] / cp[0];
		bitangent[0] = -cp[2] / cp[0];
	}

	u[0] = v1[1] - v0[1];
	v[0] = v2[1] - v0[1];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[1] = -cp[1] / cp[0];
		bitangent[1] = -cp[2] / cp[0];
	}

	u[0] = v1[2] - v0[2];
	v[0] = v2[2] - v0[2];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[2] = -cp[1] / cp[0];
		bitangent[2] = -cp[2] / cp[0];
	}

	VectorNormalizeFast(tangent);
	VectorNormalizeFast(bitangent);

	// compute the face normal based on vertex points
	VectorSubtract(v2, v0, u);
	VectorSubtract(v1, v0, v);
	CrossProduct(u, v, faceNormal);

	VectorNormalizeFast(faceNormal);

#if 0
	// normal, compute the cross product N=TxB
	CrossProduct(tangent, bitangent, normal);
	VectorNormalizeFast(normal);

	if(DotProduct(normal, faceNormal) < 0)
	{
		VectorInverse(normal);
		//VectorInverse(tangent);
		//VectorInverse(bitangent);

		CrossProduct(normal, tangent, bitangent);
	}

	VectorCopy(faceNormal, normal);
#else
	// Gram-Schmidt orthogonalize
		//tangent[a] = (t - n * Dot(n, t)).Normalize();
	VectorMA(tangent, -DotProduct(faceNormal, tangent), faceNormal, tangent);
	VectorNormalizeFast(tangent);
#endif

	VectorCopy(faceNormal, normal);
}

/*
http://www.terathon.com/code/tangent.html
*/
void R_CalcTexDirs(vec3_t sdir, vec3_t tdir, const vec3_t v1, const vec3_t v2,
				   const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3)
{
	float			x1, x2, y1, y2, z1, z2;
	float			s1, s2, t1, t2, r;

	x1 = v2[0] - v1[0];
	x2 = v3[0] - v1[0];
	y1 = v2[1] - v1[1];
	y2 = v3[1] - v1[1];
	z1 = v2[2] - v1[2];
	z2 = v3[2] - v1[2];

	s1 = w2[0] - w1[0];
	s2 = w3[0] - w1[0];
	t1 = w2[1] - w1[1];
	t2 = w3[1] - w1[1];

	r = 1.0f / (s1 * t2 - s2 * t1);

	VectorSet(sdir, (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
	VectorSet(tdir, (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
}

void R_CalcTbnFromNormalAndTexDirs(vec3_t tangent, vec3_t bitangent, vec3_t normal, vec3_t sdir, vec3_t tdir)
{
	vec3_t n_cross_t;
	float n_dot_t, handedness;

	// Gram-Schmidt orthogonalize
	n_dot_t = DotProduct(normal, sdir);
	VectorMA(sdir, -n_dot_t, normal, tangent);
	VectorNormalize(tangent);

	// Calculate handedness
	CrossProduct(normal, sdir, n_cross_t);
	handedness = (DotProduct(n_cross_t, tdir) < 0.0f) ? -1.0f : 1.0f;

	// Calculate bitangent
	CrossProduct(normal, tangent, bitangent);
	VectorScale(bitangent, handedness, bitangent);
}

qboolean R_CalcTangentVectors(srfVert_t * dv[3])
{
#if 0
	int             i;
	float           bb, s, t;
	vec3_t          bary;


	/* calculate barycentric basis for the triangle */
	bb = (dv[1]->st[0] - dv[0]->st[0]) * (dv[2]->st[1] - dv[0]->st[1]) - (dv[2]->st[0] - dv[0]->st[0]) * (dv[1]->st[1] - dv[0]->st[1]);
	if(fabs(bb) < 0.00000001f)
		return qfalse;

#ifndef __CHEAP_VERTS__
	/* do each vertex */
	for(i = 0; i < 3; i++)
	{
		vec3_t bitangent, nxt;

		// calculate s tangent vector
		s = dv[i]->st[0] + 10.0f;
		t = dv[i]->st[1];
		bary[0] = ((dv[1]->st[0] - s) * (dv[2]->st[1] - t) - (dv[2]->st[0] - s) * (dv[1]->st[1] - t)) / bb;
		bary[1] = ((dv[2]->st[0] - s) * (dv[0]->st[1] - t) - (dv[0]->st[0] - s) * (dv[2]->st[1] - t)) / bb;
		bary[2] = ((dv[0]->st[0] - s) * (dv[1]->st[1] - t) - (dv[1]->st[0] - s) * (dv[0]->st[1] - t)) / bb;

		dv[i]->tangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
		dv[i]->tangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
		dv[i]->tangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];

		VectorSubtract(dv[i]->tangent, dv[i]->xyz, dv[i]->tangent);
		VectorNormalize(dv[i]->tangent);

		// calculate t tangent vector
		s = dv[i]->st[0];
		t = dv[i]->st[1] + 10.0f;
		bary[0] = ((dv[1]->st[0] - s) * (dv[2]->st[1] - t) - (dv[2]->st[0] - s) * (dv[1]->st[1] - t)) / bb;
		bary[1] = ((dv[2]->st[0] - s) * (dv[0]->st[1] - t) - (dv[0]->st[0] - s) * (dv[2]->st[1] - t)) / bb;
		bary[2] = ((dv[0]->st[0] - s) * (dv[1]->st[1] - t) - (dv[1]->st[0] - s) * (dv[0]->st[1] - t)) / bb;

		bitangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
		bitangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
		bitangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];

		VectorSubtract(bitangent, dv[i]->xyz, bitangent);
		VectorNormalize(bitangent);

		// store bitangent handedness
		CrossProduct(dv[i]->normal, dv[i]->tangent, nxt);
		dv[i]->tangent[3] = (DotProduct(nxt, bitangent) < 0.0f) ? -1.0f : 1.0f;

		// debug code
		//% Sys_FPrintf( SYS_VRB, "%d S: (%f %f %f) T: (%f %f %f)\n", i,
		//%     stv[ i ][ 0 ], stv[ i ][ 1 ], stv[ i ][ 2 ], ttv[ i ][ 0 ], ttv[ i ][ 1 ], ttv[ i ][ 2 ] );
	}
#endif //__CHEAP_VERTS__
#endif

	return qtrue;
}


/*
=================
R_CullLocalBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLocalBox(vec3_t localBounds[2]) {
#if 0
	int		i, j;
	vec3_t	transformed[8];
	float	dists[8];
	vec3_t	v;
	cplane_t	*frust;
	int			anyBack;
	int			front, back;

	if ( r_nocull->integer || SKIP_CULL_FRAME) {
		return CULL_CLIP;
	}

	// transform into world space
	for (i = 0 ; i < 8 ; i++) {
		v[0] = bounds[i&1][0];
		v[1] = bounds[(i>>1)&1][1];
		v[2] = bounds[(i>>2)&1][2];

		VectorCopy( tr.ori.origin, transformed[i] );
		VectorMA( transformed[i], v[0], tr.ori.axis[0], transformed[i] );
		VectorMA( transformed[i], v[1], tr.ori.axis[1], transformed[i] );
		VectorMA( transformed[i], v[2], tr.ori.axis[2], transformed[i] );
	}

	// check against frustum planes
	anyBack = 0;
	for (i = 0 ; i < 4 ; i++) {
		frust = &tr.viewParms.frustum[i];

		front = back = 0;
		for (j = 0 ; j < 8 ; j++) {
			dists[j] = DotProduct(transformed[j], frust->normal);
			if ( dists[j] > frust->dist ) {
				front = 1;
				if ( back ) {
					break;		// a point is in front
				}
			} else {
				back = 1;
			}
		}
		if ( !front ) {
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if ( !anyBack ) {
		return CULL_IN;		// completely inside frustum
	}

	return CULL_CLIP;		// partially clipped
#else
	int             j;
	vec3_t          transformed;
	vec3_t          v;
	vec3_t          worldBounds[2];

	if (r_nocull->integer || SKIP_CULL_FRAME)
	{
		return CULL_CLIP;
	}

	// transform into world space
	ClearBounds(worldBounds[0], worldBounds[1]);

	for (j = 0; j < 8; j++)
	{
		v[0] = localBounds[j & 1][0];
		v[1] = localBounds[(j >> 1) & 1][1];
		v[2] = localBounds[(j >> 2) & 1][2];

		R_LocalPointToWorld(v, transformed);

		AddPointToBounds(transformed, worldBounds[0], worldBounds[1]);
	}

	return R_CullBox(worldBounds);
#endif
}

/*
=================
R_CullBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullBox(vec3_t worldBounds[2]) {
	int             i;
	cplane_t       *frust;
	qboolean        anyClip;
	int             r, numPlanes;

	numPlanes = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 5 : 4;

	// check against frustum planes
	anyClip = qfalse;
	for (i = 0; i < numPlanes; i++)
	{
		frust = &tr.viewParms.frustum[i];

		r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], frust);

		if (r == 2)
		{
			// completely outside frustum
			return CULL_OUT;
		}
		if (r == 3)
		{
			anyClip = qtrue;
		}
	}

	if (!anyClip)
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
}

int R_CullBoxMinsMaxs(vec3_t mins, vec3_t maxs) {
	int             i;
	cplane_t       *frust;
	qboolean        anyClip;
	int             r, numPlanes;

	numPlanes = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 5 : 4;

	// check against frustum planes
	anyClip = qfalse;
	for (i = 0; i < numPlanes; i++)
	{
		frust = &tr.viewParms.frustum[i];

		r = BoxOnPlaneSide(mins, maxs, frust);

		if (r == 2)
		{
			// completely outside frustum
			return CULL_OUT;
		}
		if (r == 3)
		{
			anyClip = qtrue;
		}
	}

	if (!anyClip)
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
}

/*
** R_CullLocalPointAndRadius
*/
int R_CullLocalPointAndRadius( const vec3_t pt, float radius )
{
	vec3_t transformed;

	R_LocalPointToWorld(pt, transformed);

	return R_CullPointAndRadius(transformed, radius);
}

/*
** R_CullPointAndRadius
*/
int R_CullPointAndRadiusEx( const vec3_t pt, float radius, const cplane_t* frustum, int numPlanes )
{
	int		i;
	float	dist;
	const cplane_t	*frust;
	qboolean mightBeClipped = qfalse;

	if (r_nocull->integer) {
		return CULL_CLIP;
	}

	// check against frustum planes
	for (i = 0; i < numPlanes; i++)
	{
		frust = &frustum[i];

		dist = DotProduct(pt, frust->normal) - frust->dist;
		if (dist < -radius)
		{
			return CULL_OUT;
		}
		else if (dist <= radius)
		{
			mightBeClipped = qtrue;
		}
	}

	if (mightBeClipped)
	{
		return CULL_CLIP;
	}

	return CULL_IN;		// completely inside frustum
}

/*
** R_CullPointAndRadius
*/
int R_CullPointAndRadius( const vec3_t pt, float radius )
{
	return R_CullPointAndRadiusEx(pt, radius, tr.viewParms.frustum, (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 5 : 4);
}

/*
=================
R_LocalNormalToWorld

=================
*/
void R_LocalNormalToWorld (const vec3_t local, vec3_t world) {
	world[0] = local[0] * tr.ori.axis[0][0] + local[1] * tr.ori.axis[1][0] + local[2] * tr.ori.axis[2][0];
	world[1] = local[0] * tr.ori.axis[0][1] + local[1] * tr.ori.axis[1][1] + local[2] * tr.ori.axis[2][1];
	world[2] = local[0] * tr.ori.axis[0][2] + local[1] * tr.ori.axis[1][2] + local[2] * tr.ori.axis[2][2];
}

/*
=================
R_LocalPointToWorld

=================
*/
void R_LocalPointToWorld (const vec3_t local, vec3_t world) {
	world[0] = local[0] * tr.ori.axis[0][0] + local[1] * tr.ori.axis[1][0] + local[2] * tr.ori.axis[2][0] + tr.ori.origin[0];
	world[1] = local[0] * tr.ori.axis[0][1] + local[1] * tr.ori.axis[1][1] + local[2] * tr.ori.axis[2][1] + tr.ori.origin[1];
	world[2] = local[0] * tr.ori.axis[0][2] + local[1] * tr.ori.axis[1][2] + local[2] * tr.ori.axis[2][2] + tr.ori.origin[2];
}

/*
=================
R_WorldToLocal

=================
*/
void R_WorldToLocal (const vec3_t world, vec3_t local) {
	local[0] = DotProduct(world, tr.ori.axis[0]);
	local[1] = DotProduct(world, tr.ori.axis[1]);
	local[2] = DotProduct(world, tr.ori.axis[2]);
}

/*
==========================
R_TransformModelToClip

==========================
*/
void R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst ) {
	int i;

	for ( i = 0 ; i < 4 ; i++ ) {
		eye[i] = 
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];
	}

	for ( i = 0 ; i < 4 ; i++ ) {
		dst[i] = 
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}
}

/*
==========================
R_TransformClipToWindow

==========================
*/
void R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window ) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );

	window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int) ( window[0] + 0.5 );
	window[1] = (int) ( window[1] + 0.5 );
}


/*
==========================
myGlMultMatrix

==========================
*/
void myGlMultMatrix( const float *a, const float *b, float *out ) {
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

/*
=================
R_RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
float	glMatrix[16];

void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms,
					   orientationr_t *ori ) {
	vec3_t	delta;
	float	axisLength;
	
	if ( ent->e.reType != RT_MODEL && ent->e.reType != RT_GRASS && ent->e.reType != RT_PLANT && ent->e.reType != RT_MODEL_INSTANCED) {
		*ori = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, ori->origin );

	VectorCopy( ent->e.axis[0], ori->axis[0] );
	VectorCopy( ent->e.axis[1], ori->axis[1] );
	VectorCopy( ent->e.axis[2], ori->axis[2] );

	glMatrix[0] = ori->axis[0][0];
	glMatrix[4] = ori->axis[1][0];
	glMatrix[8] = ori->axis[2][0];
	glMatrix[12] = ori->origin[0];

	glMatrix[1] = ori->axis[0][1];
	glMatrix[5] = ori->axis[1][1];
	glMatrix[9] = ori->axis[2][1];
	glMatrix[13] = ori->origin[1];

	glMatrix[2] = ori->axis[0][2];
	glMatrix[6] = ori->axis[1][2];
	glMatrix[10] = ori->axis[2][2];
	glMatrix[14] = ori->origin[2];

	glMatrix[3] = 0;
	glMatrix[7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	Matrix16Copy(glMatrix, ori->modelMatrix);
	myGlMultMatrix(glMatrix, viewParms->world.modelViewMatrix, ori->modelViewMatrix);

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->ori.origin, ori->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes ) {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0f / axisLength;
		}
	} else {
		axisLength = 1.0f;
	}

	ori->viewOrigin[0] = DotProduct( delta, ori->axis[0] ) * axisLength;
	ori->viewOrigin[1] = DotProduct( delta, ori->axis[1] ) * axisLength;
	ori->viewOrigin[2] = DotProduct( delta, ori->axis[2] ) * axisLength;
}

/*
=================
R_RotateForViewer

Sets up the modelview matrix for a given viewParm
=================
*/

/*#ifdef __VR_SEPARATE_EYE_RENDER__
void GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye, float zNear, float zFar, float *out)
{
#if 0
	vr::HmdMatrix44_t mat = HMD->GetProjectionMatrix(nEye, zNear, zFar);

	out[0] = mat.m[0][0];
	out[1] = mat.m[1][0];
	out[2] = mat.m[2][0];
	out[3] = mat.m[3][0];
	
	out[4] = mat.m[0][1];
	out[5] = mat.m[1][1];
	out[6] = mat.m[2][1];
	out[7] = mat.m[3][1];

	out[8] = mat.m[0][2];
	out[9] = mat.m[1][2];
	out[10] = mat.m[2][2];
	out[11] = mat.m[3][2];
	
	out[12] = mat.m[0][3];
	out[13] = mat.m[1][3];
	out[14] = mat.m[2][3];
	out[15] = mat.m[3][3];
#else
	vr::HmdMatrix34_t mat = HMD->GetEyeToHeadTransform(nEye);

	out[0] = mat.m[0][0];
	out[1] = mat.m[1][0];
	out[2] = mat.m[2][0];
	out[3] = 0;

	out[3] = mat.m[0][1];
	out[4] = mat.m[1][1];
	out[5] = mat.m[2][1];
	out[6] = 0;

	out[7] = mat.m[0][2];
	out[8] = mat.m[1][2];
	out[9] = mat.m[2][2];
	out[10] = 0;

	out[11] = mat.m[0][3];
	out[12] = mat.m[1][3];
	out[13] = mat.m[2][3];
	out[14] = 1;
#endif
}
#endif //__VR_SEPARATE_EYE_RENDER__*/

/*static*/ void R_RotateForViewer(viewParms_t *viewParms)
{
	float	viewerMatrix[16];
	vec3_t	origin;

	Com_Memset(&tr.ori, 0, sizeof(tr.ori));
	tr.ori.axis[0][0] = 1;
	tr.ori.axis[1][1] = 1;
	tr.ori.axis[2][2] = 1;
	VectorCopy(viewParms->ori.origin, tr.ori.viewOrigin);

	// transform by the camera placement
	VectorCopy(viewParms->ori.origin, origin);

	viewerMatrix[0] = viewParms->ori.axis[0][0];
	viewerMatrix[4] = viewParms->ori.axis[0][1];
	viewerMatrix[8] = viewParms->ori.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = viewParms->ori.axis[1][0];
	viewerMatrix[5] = viewParms->ori.axis[1][1];
	viewerMatrix[9] = viewParms->ori.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = viewParms->ori.axis[2][0];
	viewerMatrix[6] = viewParms->ori.axis[2][1];
	viewerMatrix[10] = viewParms->ori.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;


	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
/*#ifdef __VR_SEPARATE_EYE_RENDER__
#if 0
	float   glviewMatrix[16];
	myGlMultMatrix(viewerMatrix, s_flipMatrix, glviewMatrix);

	float   viewerTranslate[16];

	viewerTranslate[0] = 1.0;
	viewerTranslate[1] = 0.0;
	viewerTranslate[2] = 0.0;
	viewerTranslate[3] = 0.0;

	viewerTranslate[4] = 0.0;
	viewerTranslate[5] = 1.0;
	viewerTranslate[6] = 0.0;
	viewerTranslate[7] = 0.0;

	viewerTranslate[8] = 0.0;
	viewerTranslate[9] = 0.0;
	viewerTranslate[10] = 1.0;
	viewerTranslate[11] = 0.0;

	viewerTranslate[12] = 0.5 * vr_interpupillaryDistance->value;//HMD.InterpupillaryDistance;
	if (viewParms != NULL && viewParms->stereoFrame == STEREO_RIGHT)
	{
		viewerTranslate[12] *= -1.0;
	}
	viewerTranslate[13] = 0.0;
	viewerTranslate[14] = 0.0;
	viewerTranslate[15] = 1.0;


	myGlMultMatrix(viewerTranslate, glviewMatrix, tr.ori.modelViewMatrix);
#else
	float eyeMatrix[16];
	float viewMatrix[16];
	GetHMDMatrixProjectionEye((viewParms->stereoFrame == STEREO_RIGHT) ? vr::Eye_Right : vr::Eye_Left, r_znear->value, backEnd.viewParms.zFar, eyeMatrix);

	myGlMultMatrix(viewerMatrix, eyeMatrix, viewMatrix);

	myGlMultMatrix(viewMatrix, s_flipMatrix, tr.ori.modelViewMatrix);
	Matrix16Identity(tr.ori.modelMatrix);
#endif
#else //!__VR_SEPARATE_EYE_RENDER__*/
	myGlMultMatrix(viewerMatrix, s_flipMatrix, tr.ori.modelViewMatrix);
	Matrix16Identity(tr.ori.modelMatrix);
//#endif //__VR_SEPARATE_EYE_RENDER__

	viewParms->world = tr.ori;

}

/*
** SetFarClip
*/
void R_SetFarClip( void )
{
	float	farthestCornerDistance = 0;
	int		i;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		// override the zfar then
		if ( tr.refdef.rdflags & RDF_AUTOMAP )
			tr.viewParms.zFar = 32768.0f;
		else
			tr.viewParms.zFar = 2048.0f;
		return;
	}

	//
	// set far clipping planes dynamically
	//
	for ( i = 0; i < 8; i++ )
	{
		vec3_t v;
		float distance;

		if ( i & 1 )
		{
			v[0] = tr.viewParms.visBounds[0][0];
		}
		else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if ( i & 2 )
		{
			v[1] = tr.viewParms.visBounds[0][1];
		}
		else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if ( i & 4 )
		{
			v[2] = tr.viewParms.visBounds[0][2];
		}
		else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		distance = DistanceSquared( tr.viewParms.ori.origin, v );

		if ( distance > farthestCornerDistance )
		{
			farthestCornerDistance = distance;
		}
	}
	// Bring in the zFar to the distanceCull distance
	// The sky renders at zFar so need to move it out a little
	// ...and make sure there is a minimum zfar to prevent problems
	tr.viewParms.zFar = Com_Clamp(2048.0f, tr.distanceCull * (1.732), sqrtf( farthestCornerDistance ));
	tr.occlusionOriginalZfar = tr.viewParms.zFar;

	if (!(tr.viewParms.flags & VPF_DEPTHSHADOW) && !backEnd.depthFill && ENABLE_OCCLUSION_CULLING && r_occlusion->integer)
	{
		if (tr.occlusionZfar <= 0.0)
		{// For when the map just loaded and no occlusions have been done yet...
			tr.occlusionZfar = tr.viewParms.zFar;
			tr.occlusionZfarFoliage = tr.viewParms.zFar;
		}

		tr.viewParms.zFar = tr.occlusionZfar;
	}
}

/*
=================
R_SetupFrustum

Set up the culling frustum planes for the current view using the results we got from computing the first two rows of
the projection matrix.
=================
*/
void R_SetupFrustum (viewParms_t *dest, float xmin, float xmax, float ymax, float zProj, float zFar, float stereoSep)
{
	vec3_t ofsorigin;
	float oppleg, adjleg, length;
	int i;
	
	// symmetric case can be simplified
	VectorCopy(dest->ori.origin, ofsorigin);

	length = sqrt(xmax * xmax + zProj * zProj);
	oppleg = xmax / length;
	adjleg = zProj / length;

	VectorScale(dest->ori.axis[0], oppleg, dest->frustum[0].normal);
	VectorMA(dest->frustum[0].normal, adjleg, dest->ori.axis[1], dest->frustum[0].normal);

	VectorScale(dest->ori.axis[0], oppleg, dest->frustum[1].normal);
	VectorMA(dest->frustum[1].normal, -adjleg, dest->ori.axis[1], dest->frustum[1].normal);


	length = sqrt(ymax * ymax + zProj * zProj);
	oppleg = ymax / length;
	adjleg = zProj / length;

	VectorScale(dest->ori.axis[0], oppleg, dest->frustum[2].normal);
	VectorMA(dest->frustum[2].normal, adjleg, dest->ori.axis[2], dest->frustum[2].normal);

	VectorScale(dest->ori.axis[0], oppleg, dest->frustum[3].normal);
	VectorMA(dest->frustum[3].normal, -adjleg, dest->ori.axis[2], dest->frustum[3].normal);
	
	for (i=0 ; i<4 ; i++) {
		dest->frustum[i].type = PLANE_NON_AXIAL;
		dest->frustum[i].dist = DotProduct (ofsorigin, dest->frustum[i].normal);
		SetPlaneSignbits( &dest->frustum[i] );
	}

	if (zFar != 0.0f)
	{
		vec3_t farpoint;

		VectorMA(ofsorigin, zFar, dest->ori.axis[0], farpoint);
		VectorScale(dest->ori.axis[0], -1.0f, dest->frustum[4].normal);

		dest->frustum[4].type = PLANE_NON_AXIAL;
		dest->frustum[4].dist = DotProduct (farpoint, dest->frustum[4].normal);
		SetPlaneSignbits( &dest->frustum[4] );
		dest->flags |= VPF_FARPLANEFRUSTUM;
	}
}

/*
===============
R_SetupProjection
===============
*/
#ifdef __VR_SEPARATE_EYE_RENDER__
void R_SetupProjectionVR(viewParms_t *dest, float zProj, float zFar, qboolean computeFrustum)
{
	//***
	float	xmin, xmax, ymin, ymax;
	float	width, height, stereoSep = vr_stereoSeparation->value;
	float   frameMultiplier = 1;
	float   a = tr.renderLeftVRFbo->width / (2.0*tr.renderLeftVRFbo->height);//(float)HMD.HResolution / (2.0*(float)HMD.VResolution);  //*** Correct Aspect ratio is 2.0 *
	//float   fov = 2.0*atan((float)/*HMD.VScreenSize*/VR->RenderWidth() / (2.0*(float)/*HMD.EyeToScreenDistance*/vr_eyeToScreenDistance->value)) - vr_fovOffset->value;
	float   fov = 2.0*atan((float)VR->DeviceWidth() / (2.0*(float)VR->DeviceHeight())) - vr_fovOffset->value;
	float   zNear = r_znear->value;

	/*
	* offset the view origin of the viewer for stereo rendering
	* by setting the projection matrix appropriately.
	*/

	if (stereoSep != 0)
	{
		if (dest->stereoFrame == STEREO_LEFT)
		{
			frameMultiplier = 2;
			stereoSep = zProj / stereoSep;
		}
		else if (dest->stereoFrame == STEREO_RIGHT)
		{
			frameMultiplier = 2;
			stereoSep = zProj / -stereoSep;
		}
		else
			stereoSep = 0;
	}

	ymax = zProj * tan(dest->fovY * M_PI / 360.0f);
	ymin = -ymax;

	xmax = zProj * tan(dest->fovX * M_PI / 360.0f);
	xmin = -xmax;

	width = (xmax - xmin);
	height = ymax - ymin;

	{
		float projectionMatrix[16];
		float H[16];
		float hMeters;
		float h;
		
		hMeters = ((float)/*HMD.HScreenSize*/VR->RenderHeight() / 4.0) - (vr_interpupillaryDistance->value / 2.0);
		h = 4.0*hMeters / (float)/*HMD.HScreenSize*/VR->RenderHeight();

		projectionMatrix[0] = 1.0 / (a*tan(fov / 2));
		projectionMatrix[1] = 0;
		projectionMatrix[2] = 0;
		projectionMatrix[3] = 0;

		projectionMatrix[4] = 0;
		projectionMatrix[5] = 1.0 / (tan(fov / 2));
		projectionMatrix[6] = 0;
		projectionMatrix[7] = 0;

		projectionMatrix[8] = 0;
		projectionMatrix[9] = 0;
		projectionMatrix[10] = zFar / (zNear - zFar);
		projectionMatrix[11] = -1.0;

		projectionMatrix[12] = 0;
		projectionMatrix[13] = 0;
		projectionMatrix[14] = (zFar*zNear) / (zNear - zFar);
		projectionMatrix[15] = 0;


		H[0] = 1.0;
		H[1] = 0.0;
		H[2] = 0.0;
		H[3] = 0.0;

		H[4] = 0.0;
		H[5] = 1.0;
		H[6] = 0.0;
		H[7] = 0.0;

		H[8] = 0.0;
		H[9] = 0.0;
		H[10] = 1.0;
		H[11] = 0.0;

		H[12] = h;
		if (dest->stereoFrame == STEREO_RIGHT)
			H[12] *= -1.0;
		H[13] = 0.0;
		H[14] = 0.0;
		H[15] = 1.0;

		myGlMultMatrix(H, projectionMatrix, dest->projectionMatrix);
	}

	// Now that we have all the data for the projection matrix we can also setup the view frustum.
	if (computeFrustum)
		R_SetupFrustum(dest, xmin, xmax, ymax, zProj, zFar, stereoSep);
}
#endif //!__VR_SEPARATE_EYE_RENDER__

void R_SetupProjection(viewParms_t *dest, float zProj, float zFar, qboolean computeFrustum)
{
/*#ifdef __VR_SEPARATE_EYE_RENDER__
	if (OVRDetected)
	{
		R_SetupProjectionVR(dest, zProj, zFar, computeFrustum);
		return;
	}
#endif //__VR_SEPARATE_EYE_RENDER__*/

	float	xmin, xmax, ymin, ymax;
	float	width, height, stereoSep = 0;

#ifdef __VR_SEPARATE_EYE_RENDER__
	//

	stereoSep = vr_stereoSeparation->value;

	if (stereoSep != 0)
	{
		if (dest->stereoFrame == STEREO_LEFT)
		{
			//stereoSep = zProj / stereoSep;
		}
		else if (dest->stereoFrame == STEREO_RIGHT)
		{
			//stereoSep = zProj / -stereoSep;
			stereoSep = -stereoSep;
		}
		else
		{
			stereoSep = 0;
		}
	}
#endif //__VR_SEPARATE_EYE_RENDER__

	//

	/*
	* offset the view origin of the viewer for stereo rendering
	* by setting the projection matrix appropriately.
	*/

	ymax = zProj * tan(dest->fovY * M_PI / 360.0f);
	ymin = -ymax;

	xmax = zProj * tan(dest->fovX * M_PI / 360.0f);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

	dest->projectionMatrix[0] = 2 * zProj / width;
	dest->projectionMatrix[4] = 0;
	dest->projectionMatrix[8] = (xmax + xmin + 2 * stereoSep) / width;
	dest->projectionMatrix[12] = 2 * zProj * stereoSep / width;

	dest->projectionMatrix[1] = 0;
	dest->projectionMatrix[5] = 2 * zProj / height;
	dest->projectionMatrix[9] = (ymax + ymin) / height;	// normally 0
	dest->projectionMatrix[13] = 0;

	dest->projectionMatrix[3] = 0;
	dest->projectionMatrix[7] = 0;
	dest->projectionMatrix[11] = -1;
	dest->projectionMatrix[15] = 0;

	// Now that we have all the data for the projection matrix we can also setup the view frustum.
	if (computeFrustum)
		R_SetupFrustum(dest, xmin, xmax, ymax, zProj, zFar, stereoSep);
}

/*
===============
R_SetupProjectionZ

Sets the z-component transformation part in the projection matrix
===============
*/
void R_SetupProjectionZ(viewParms_t *dest)
{
	float zNear, zFar, depth;

#ifdef __INVERSE_DEPTH_BUFFERS__
	zNear = dest->zFar;
	zFar = r_znear->value;

	depth = zNear - zFar;

	dest->projectionMatrix[2] = 0;
	dest->projectionMatrix[6] = 0;
	dest->projectionMatrix[10] = -(zNear + zFar) / depth;
	dest->projectionMatrix[14] = -2 * zNear * zFar / depth;
#else //!__INVERSE_DEPTH_BUFFERS__
	zNear = r_znear->value;
	zFar = dest->zFar;

	depth = zFar - zNear;

	dest->projectionMatrix[2] = 0;
	dest->projectionMatrix[6] = 0;
	dest->projectionMatrix[10] = -(zFar + zNear) / depth;
	dest->projectionMatrix[14] = -2 * zFar * zNear / depth;
#endif //__INVERSE_DEPTH_BUFFERS__

	if (dest->isPortal)
	{
		float	plane[4];
		float	plane2[4];
		vec4_t q, c;

		// transform portal plane into camera space
		plane[0] = dest->portalPlane.normal[0];
		plane[1] = dest->portalPlane.normal[1];
		plane[2] = dest->portalPlane.normal[2];
		plane[3] = dest->portalPlane.dist;

		plane2[0] = -DotProduct(dest->ori.axis[1], plane);
		plane2[1] = DotProduct(dest->ori.axis[2], plane);
		plane2[2] = -DotProduct(dest->ori.axis[0], plane);
		plane2[3] = DotProduct(plane, dest->ori.origin) - plane[3];

		// Lengyel, Eric. "Modifying the Projection Matrix to Perform Oblique Near-plane Clipping".
		// Terathon Software 3D Graphics Library, 2004. http://www.terathon.com/code/oblique.html
		q[0] = (SGN(plane2[0]) + dest->projectionMatrix[8]) / dest->projectionMatrix[0];
		q[1] = (SGN(plane2[1]) + dest->projectionMatrix[9]) / dest->projectionMatrix[5];
		q[2] = -1.0f;
		q[3] = (1.0f + dest->projectionMatrix[10]) / dest->projectionMatrix[14];

		VectorScale4(plane2, 2.0f / DotProduct4(plane2, q), c);

		dest->projectionMatrix[2] = c[0];
		dest->projectionMatrix[6] = c[1];
		dest->projectionMatrix[10] = c[2] + 1.0f;
		dest->projectionMatrix[14] = c[3];

	}
}

/*
===============
R_SetupProjectionOrtho
===============
*/
void R_SetupProjectionOrtho(viewParms_t *dest, vec3_t viewBounds[2])
{
	float xmin, xmax, ymin, ymax, znear, zfar;
	//viewParms_t *dest = &tr.viewParms;
	int i;
	vec3_t pop;

	// Quake3:   Projection:
	//
	//    Z  X   Y  Z
	//    | /    | /
	//    |/     |/
	// Y--+      +--X

	xmin = viewBounds[0][1];
	xmax = viewBounds[1][1];
	ymin = -viewBounds[1][2];
	ymax = -viewBounds[0][2];
#ifdef __INVERSE_DEPTH_BUFFERS__
	znear = viewBounds[1][0];
	zfar = viewBounds[0][0];
#else //!__INVERSE_DEPTH_BUFFERS__
	znear = viewBounds[0][0];
	zfar = viewBounds[1][0];
#endif //__INVERSE_DEPTH_BUFFERS__

	dest->projectionMatrix[0] = 2 / (xmax - xmin);
	dest->projectionMatrix[4] = 0;
	dest->projectionMatrix[8] = 0;
	dest->projectionMatrix[12] = (xmax + xmin) / (xmax - xmin);

	dest->projectionMatrix[1] = 0;
	dest->projectionMatrix[5] = 2 / (ymax - ymin);
	dest->projectionMatrix[9] = 0;
	dest->projectionMatrix[13] = (ymax + ymin) / (ymax - ymin);

	dest->projectionMatrix[2] = 0;
	dest->projectionMatrix[6] = 0;
#ifdef __INVERSE_DEPTH_BUFFERS__
	dest->projectionMatrix[10] = -2 / (znear - zfar);
	dest->projectionMatrix[14] = -(znear + zfar) / (znear - zfar);
#else //!__INVERSE_DEPTH_BUFFERS__
	dest->projectionMatrix[10] = -2 / (zfar - znear);
	dest->projectionMatrix[14] = -(zfar + znear) / (zfar - znear);
#endif //__INVERSE_DEPTH_BUFFERS__

	dest->projectionMatrix[3] = 0;
	dest->projectionMatrix[7] = 0;
	dest->projectionMatrix[11] = 0;
	dest->projectionMatrix[15] = 1;

	VectorScale(dest->ori.axis[1], 1.0f, dest->frustum[0].normal);
	VectorMA(dest->ori.origin, viewBounds[0][1], dest->frustum[0].normal, pop);
	dest->frustum[0].dist = DotProduct(pop, dest->frustum[0].normal);

	VectorScale(dest->ori.axis[1], -1.0f, dest->frustum[1].normal);
	VectorMA(dest->ori.origin, -viewBounds[1][1], dest->frustum[1].normal, pop);
	dest->frustum[1].dist = DotProduct(pop, dest->frustum[1].normal);

	VectorScale(dest->ori.axis[2], 1.0f, dest->frustum[2].normal);
	VectorMA(dest->ori.origin, viewBounds[0][2], dest->frustum[2].normal, pop);
	dest->frustum[2].dist = DotProduct(pop, dest->frustum[2].normal);

	VectorScale(dest->ori.axis[2], -1.0f, dest->frustum[3].normal);
	VectorMA(dest->ori.origin, -viewBounds[1][2], dest->frustum[3].normal, pop);
	dest->frustum[3].dist = DotProduct(pop, dest->frustum[3].normal);

	VectorScale(dest->ori.axis[0], -1.0f, dest->frustum[4].normal);
	VectorMA(dest->ori.origin, -viewBounds[1][0], dest->frustum[4].normal, pop);
	dest->frustum[4].dist = DotProduct(pop, dest->frustum[4].normal);

	for (i = 0; i < 5; i++)
	{
		dest->frustum[i].type = PLANE_NON_AXIAL;
		SetPlaneSignbits(&dest->frustum[i]);
	}

	dest->flags |= VPF_FARPLANEFRUSTUM;
}

/*
=================
R_MirrorPoint
=================
*/
void R_MirrorPoint (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out) {
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	VectorSubtract( in, surface->origin, local );

	VectorClear( transformed );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(local, surface->axis[i]);
		VectorMA( transformed, d, camera->axis[i], transformed );
	}

	VectorAdd( transformed, camera->origin, out );
}

void R_MirrorVector (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out) {
	int		i;
	float	d;

	VectorClear( out );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(in, surface->axis[i]);
		VectorMA( out, d, camera->axis[i], out );
	}
}


/*
=============
R_PlaneForSurface
=============
*/
void R_PlaneForSurface (surfaceType_t *surfType, cplane_t *plane) {
	srfBspSurface_t	*tri;
	srfPoly_t		*poly;
	srfVert_t		*v1, *v2, *v3;
	vec4_t			plane4;

	if (!surfType) {
		Com_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch (*surfType) {
	case SF_FACE:
		*plane = ((srfBspSurface_t *)surfType)->cullPlane;
		return;
	case SF_TRIANGLES:
		tri = (srfBspSurface_t *)surfType;
		v1 = tri->verts + tri->indexes[0];
		v2 = tri->verts + tri->indexes[1];
		v3 = tri->verts + tri->indexes[2];
		PlaneFromPoints( plane4, v1->xyz, v2->xyz, v3->xyz );
		VectorCopy( plane4, plane->normal ); 
		plane->dist = plane4[3];
		return;
	case SF_POLY:
		poly = (srfPoly_t *)surfType;
		PlaneFromPoints( plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz );
		VectorCopy( plane4, plane->normal ); 
		plane->dist = plane4[3];
		return;
	default:
		Com_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;		
		return;
	}
}

/*
=================
R_GetPortalOrientation

entityNum is the entity that the portal surface is a part of, which may
be moving and rotating.

Returns qtrue if it should be mirrored
=================
*/
qboolean R_GetPortalOrientations(drawSurf_t *drawSurf, int64_t entityNum,
							 orientation_t *surface, orientation_t *camera,
							 vec3_t pvsOrigin, qboolean *mirror ) {
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;
	vec3_t		transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.ori );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.ori.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.ori.origin );
	} else {
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[0] );
	PerpendicularVector( surface->axis[1], surface->axis[0] );
	CrossProduct( surface->axis[0], surface->axis[1], surface->axis[2] );

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) {
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] && 
			e->e.oldorigin[1] == e->e.origin[1] && 
			e->e.oldorigin[2] == e->e.origin[2] ) {
			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
			VectorCopy( surface->axis[1], camera->axis[1] );
			VectorCopy( surface->axis[2], camera->axis[2] );

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
		VectorMA( e->e.origin, -d, surface->axis[0], surface->origin );
			
		// now get the camera origin and orientation
		VectorCopy( e->e.oldorigin, camera->origin );
		AxisCopy( e->e.axis, camera->axis );
		VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
		VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );

		// optionally rotate
		if ( e->e.oldframe ) {
			// if a speed is specified
			if ( e->e.frame ) {
				// continuous rotate
				d = (tr.refdef.time/1000.0f) * e->e.frame;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			} else {
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( tr.refdef.time * 0.003f );
				d = e->e.skinNum + d * 4;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			}
		}
		else if ( e->e.skinNum ) {
			d = e->e.skinNum;
			VectorCopy( camera->axis[1], transformed );
			RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
			CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
		}
		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri->Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return qfalse;
}

static qboolean IsMirror(const drawSurf_t *drawSurf, int64_t entityNum)
{
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD ) 
	{
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.ori );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.ori.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.ori.origin );
	} 

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) 
	{
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] && 
			e->e.oldorigin[1] == e->e.origin[1] && 
			e->e.oldorigin[2] == e->e.origin[2] ) 
		{
			return qtrue;
		}

		return qfalse;
	}
	return qfalse;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qboolean SurfIsOffscreen( const drawSurf_t *drawSurf, vec4_t clipDest[128] ) {
	float shortest = 100000000;
	int64_t entityNum;
	int numTriangles;
	shader_t *shader;
	int64_t	fogNum;
	//int		cubemap;
	int64_t postRender;
	vec4_t clip, eye;
	int i;
	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	R_RotateForViewer(&tr.viewParms);

	R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &postRender);
	RB_BeginSurface(shader, fogNum, drawSurf->cubemapIndex);
	rb_surfaceTable[*drawSurf->surface](drawSurf->surface);

	if (tess.numVertexes > 128)
	{
		// Don't bother trying, just assume it's off-screen and make it look bad. Besides, artists
		// shouldn't be using this many vertices on a mirror surface anyway :)
		return qtrue;
	}

	for (i = 0; i < tess.numVertexes; i++)
	{
		int j;
		unsigned int pointFlags = 0;

		R_TransformModelToClip(tess.xyz[i], tr.ori.modelViewMatrix, tr.viewParms.projectionMatrix, eye, clip);
		VectorCopy4(clip, clipDest[i]);

		for (j = 0; j < 3; j++)
		{
			if (clip[j] >= clip[3])
			{
				pointFlags |= (1 << (j * 2));
			}
			else if (clip[j] <= -clip[3])
			{
				pointFlags |= (1 << (j * 2 + 1));
			}
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if (pointAnd)
	{
		return qtrue;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	numTriangles = tess.numIndexes / 3;

	for (i = 0; i < tess.numIndexes; i += 3)
	{
		vec3_t normal, tNormal;

		float len;

		VectorSubtract(tess.xyz[tess.indexes[i]], tr.viewParms.ori.origin, normal);

		len = VectorLengthSquared(normal);			// lose the sqrt
		if (len < shortest)
		{
			shortest = len;
		}

		R_VboUnpackNormal(tNormal, tess.normal[tess.indexes[i]]);

		if (DotProduct(normal, tNormal) >= 0)
		{
			numTriangles--;
		}
	}
	if (!numTriangles)
	{
		return qtrue;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if (IsMirror(drawSurf, entityNum))
	{
		return qfalse;
	}

	if (shortest > (tess.shader->portalRange*tess.shader->portalRange))
	{
		return qtrue;
	}

	return qfalse;
}

/*
========================
R_MirrorViewBySurface

Returns qtrue if another view has been rendered
========================
*/
qboolean R_MirrorViewBySurface(drawSurf_t *drawSurf, int64_t entityNum) {
	vec4_t			clipDest[128];
	viewParms_t		newParms;
	viewParms_t		oldParms;
	orientation_t	surface, camera;

	// don't recursively mirror
	if (tr.viewParms.isPortal) {
#ifdef __DEVELOPER_MODE__
		ri->Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
#endif //__DEVELOPER_MODE__
		return qfalse;
	}

	if ( r_noportals->integer || (r_fastsky->integer == 1) ) {
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf, clipDest ) ) {
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = qtrue;

	newParms.zFar = 0.0f;

	newParms.flags &= ~VPF_FARPLANEFRUSTUM;
	if ( !R_GetPortalOrientations( drawSurf, entityNum, &surface, &camera, 
		newParms.pvsOrigin, &newParms.isMirror ) ) {
		return qfalse;		// bad portal, no portalentity
	}

	if (newParms.isMirror)
		newParms.flags |= VPF_NOVIEWMODEL;

	R_MirrorPoint (oldParms.ori.origin, &surface, &camera, newParms.ori.origin );

	VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );
	
	R_MirrorVector (oldParms.ori.axis[0], &surface, &camera, newParms.ori.axis[0]);
	R_MirrorVector (oldParms.ori.axis[1], &surface, &camera, newParms.ori.axis[1]);
	R_MirrorVector (oldParms.ori.axis[2], &surface, &camera, newParms.ori.axis[2]);

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	CURRENT_DRAW_DLIGHTS_UPDATE = qtrue;
	R_RenderView (&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}

/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_SpriteFogNum( trRefEntity_t *ent ) {
#ifdef __Q3_FOG__
	int				i, j;
	fog_t			*fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}
#endif //__Q3_FOG__

	return 0;
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

/*
===============
R_Radix
===============
*/
static QINLINE void R_Radix( int byte, int size, drawSurf_t *source, drawSurf_t *dest )
{
  int           count[ 256 ] = { 0 };
  int           index[ 256 ];
  int           i;
  unsigned char *sortKey = NULL;
  unsigned char *end = NULL;

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  end = sortKey + ( size * sizeof( drawSurf_t ) );

  for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
    ++count[ *sortKey ];

  index[ 0 ] = 0;

  for( i = 1; i < 256; ++i )
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  for( i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
===============
R_RadixSort

Radix sort with 4 byte size buckets
===============
*/
static void R_RadixSort( drawSurf_t *source, int size )
{
  static drawSurf_t scratch[ MAX_DRAWSURFS ];
#ifdef Q3_LITTLE_ENDIAN
  R_Radix( 0, size, source, scratch );
  R_Radix( 1, size, scratch, source );
  R_Radix( 2, size, source, scratch );
  R_Radix( 3, size, scratch, source );
  R_Radix( 4, size, source, scratch ); // added 4..7 for 64bit sorting
  R_Radix( 5, size, scratch, source );
  R_Radix( 6, size, source, scratch );
  R_Radix( 7, size, scratch, source );
#else
  R_Radix( 7, size, source, scratch );
  R_Radix( 6, size, scratch, source );
  R_Radix( 5, size, source, scratch );
  R_Radix( 4, size, scratch, source );
  R_Radix( 3, size, source, scratch );
  R_Radix( 2, size, scratch, source );
  R_Radix( 1, size, source, scratch );
  R_Radix( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
}

//==========================================================================================

bool R_IsPostRenderEntity ( int refEntityNum, const trRefEntity_t *refEntity )
{
	if ( refEntityNum == REFENTITYNUM_WORLD )
	{
		return false;
	}

	return (refEntity->e.renderfx & RF_DISTORTION) ||
			(refEntity->e.renderfx & RF_FORCEPOST) ||
			(refEntity->e.renderfx & RF_FORCE_ENT_ALPHA);
}

/*
=================
R_AddDrawSurf
=================
*/

#include <mutex>
std::mutex						mAddSurfMutex;

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, 
			int64_t fogIndex, int64_t dlightMap, int64_t postRender,
					int cubemap, qboolean depthDrawOnly) {
	int			index;
	
	if (r_cullNoDraws->integer && (!shader || (shader->surfaceFlags & SURF_NODRAW) || (*surface == SF_SKIP)))
	{// How did we even get here?
		return;
	}

#ifdef __Q3_FOG__
	if (tr.refdef.rdflags & RDF_NOFOG)
	{
		fogIndex = 0;
	}
#else //!__Q3_FOG__
	fogIndex = 0;
#endif //__Q3_FOG__

	if ( (shader->surfaceFlags & SURF_FORCESIGHT) /*&& !(tr.refdef.rdflags & RDF_ForceSightOn)*/ )
	{	//if shader is only seen with ForceSight and we don't have ForceSight on, then don't draw
		return;
	}

#ifdef __RENDERER_THREADING__
	mAddSurfMutex.lock();
#endif

#pragma omp critical (__ADD_DRAW_SURFACE__)
	{
		// instead of checking for overflow, we just mask the index
		// so it wraps around
		index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
		// the sort data is packed into a single 32 bit value so it can be
		// compared quickly during the qsorting process
		tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT)
			| tr.shiftedEntityNum | (fogIndex << QSORT_FOGNUM_SHIFT)
			| (postRender << QSORT_POSTRENDER_SHIFT) | (int64_t)dlightMap;
		tr.refdef.drawSurfs[index].cubemapIndex = cubemap;
		tr.refdef.drawSurfs[index].surface = surface;
#ifdef __ZFAR_CULLING_ON_SURFACES__
		tr.refdef.drawSurfs[index].depthDrawOnly = depthDrawOnly;
#endif //__ZFAR_CULLING_ON_SURFACES__
		tr.refdef.numDrawSurfs++;
	}
	
#ifdef __RENDERER_THREADING__
	mAddSurfMutex.unlock();
#endif
}

#include <mutex>
std::mutex						mAddSurfThreadedMutex;

void R_AddDrawSurfThreaded(surfaceType_t *surface, shader_t *shader,
	int64_t fogIndex, int64_t dlightMap, int64_t postRender,
	int cubemap, qboolean depthDrawOnly, int64_t shiftedEntityNum) {
	int			index;

	if (r_cullNoDraws->integer && (!shader || (shader->surfaceFlags & SURF_NODRAW) || (*surface == SF_SKIP)))
	{// How did we even get here?
		return;
	}

#ifdef __Q3_FOG__
	if (tr.refdef.rdflags & RDF_NOFOG)
	{
		fogIndex = 0;
	}
#else //!__Q3_FOG__
	fogIndex = 0;
#endif //__Q3_FOG__

	if ((shader->surfaceFlags & SURF_FORCESIGHT) /*&& !(tr.refdef.rdflags & RDF_ForceSightOn)*/)
	{	//if shader is only seen with ForceSight and we don't have ForceSight on, then don't draw
		return;
	}

	mAddSurfThreadedMutex.lock();

	// instead of checking for overflow, we just mask the index
	// so it wraps around
	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT)
		| shiftedEntityNum | (fogIndex << QSORT_FOGNUM_SHIFT)
		| (postRender << QSORT_POSTRENDER_SHIFT) | (int64_t)dlightMap;
	tr.refdef.drawSurfs[index].cubemapIndex = cubemap;
	tr.refdef.drawSurfs[index].surface = surface;
#ifdef __ZFAR_CULLING_ON_SURFACES__
	tr.refdef.drawSurfs[index].depthDrawOnly = depthDrawOnly;
#endif //__ZFAR_CULLING_ON_SURFACES__
	tr.refdef.numDrawSurfs++;

	mAddSurfThreadedMutex.unlock();
}

/*
=================
R_DecomposeSort
=================
*/
void R_DecomposeSort(const uint64_t sort, int64_t *entityNum, shader_t **shader,
					int64_t *fogNum, int64_t *postRender) {
	//*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*entityNum = ( sort >> QSORT_REFENTITYNUM_SHIFT ) & REFENTITYNUM_MASK;
	*postRender = (sort >> QSORT_POSTRENDER_SHIFT ) & 1;
	//*dlightMap = sort & 1;

	if (*entityNum != REFENTITYNUM_WORLD && backEnd.refdef.entities)
	{
		trRefEntity_t *thisEnt = &backEnd.refdef.entities[*entityNum];

		if (thisEnt && thisEnt->e.customShader)
		{
			*shader = R_GetShaderByHandle(thisEnt->e.customShader);
		}
	}
}

/*
=================
R_SortDrawSurfs
=================
*/
void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader;
	int64_t			fogNum;
	int64_t			entityNum;
	//int64_t			dlighted;
	int64_t			postRender;
	int				i;

	//ri->Printf(PRINT_ALL, "firstDrawSurf %d numDrawSurfs %d\n", (int)(drawSurfs - tr.refdef.drawSurfs), numDrawSurfs);

	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs >= MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS-1;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	R_RadixSort( drawSurfs, numDrawSurfs );

	// skip pass through drawing if rendering a shadow map
	if (tr.viewParms.flags & (VPF_SHADOWMAP | VPF_DEPTHSHADOW))
	{
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		R_DecomposeSort( (drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &postRender );

		if ( !shader || shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			ri->Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( (drawSurfs+i), entityNum) ) {
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer ) {
				return;
			}
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}

extern void TR_AxisToAngles ( const vec3_t axis[3], vec3_t angles );

extern qboolean LODMODEL_MAP;

static void R_AddEntitySurface (int entityNum)
{
	trRefEntity_t	*ent = &tr.refdef.entities[entityNum];
	shader_t		*shader;
	
	if (!ent) return;

	if (backEnd.refdef.rdflags & RDF_BLUR)
	{
		if (ent && Distance(ent->e.origin, backEnd.refdef.vieworg/*tr.refdef.vieworg*/) > 1024)
		{// Don't draw distant entities in scope blured background view...
			return;
		}
	}

	ent->needDlights = qfalse;

	// preshift the value we are going to OR into the drawsurf sort
	int64_t shiftedEntityNum = entityNum << QSORT_REFENTITYNUM_SHIFT;

	//
	// the weapon model must be handled special --
	// we don't want the hacked weapon position showing in 
	// mirrors, because the true body position will already be drawn
	//
	if ( (ent->e.renderfx & RF_FIRST_PERSON) && (tr.viewParms.flags & VPF_NOVIEWMODEL)) {
		return;
	}

#if 1
	if (!ent->e.ignoreCull)
	{
		if ((tr.viewParms.flags & VPF_SHADOWPASS) || backEnd.depthFill)
		{// Don't draw grass and plants on shadow pass for speed...
			if (!r_foliageShadows->integer)
			{
				switch (ent->e.reType) {
				case RT_GRASS:
				case RT_PLANT:
					//case RT_MODEL_INSTANCED:
					return;
					break;
				default:
					break;
				}
			}


			if (!LODMODEL_MAP && (tr.viewParms.flags & VPF_SHADOWPASS))
			{// Only allow models at range to be drawn on LODMODEL_MAP maps...
				if (ent && Distance(ent->e.origin, tr.refdef.vieworg) > tr.viewParms.maxEntityRange)
					return; // Too far away to bother rendering to shadowmap...
			}
		}
	}
#endif

	// simple generated models, like sprites and beams, are not culled
	switch (ent->e.reType) {
	case RT_PORTALSURFACE:
		break;		// don't draw anything
	case RT_SPRITE:
	case RT_BEAM:
	case RT_ORIENTED_QUAD:
	case RT_ELECTRICITY:
	case RT_LINE:
	case RT_ORIENTEDLINE:
	case RT_CYLINDER:
	case RT_SABER_GLOW:
	{
		// self blood sprites, talk balloons, etc should not be drawn in the primary
		// view.  We can't just do this check for all entities, because md3
		// entities may still want to cast shadows from them
		if ((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
			return;
		}

		//if (R_CullPointAndRadius( ent->e.origin, ent->e.radius ) != CULL_OUT)
		{
			shader = R_GetShaderByHandle(ent->e.customShader);

			R_AddDrawSurfThreaded(&entitySurface, shader, R_SpriteFogNum(ent), 0, R_IsPostRenderEntity(entityNum, ent), 0 /* cubeMap */, qfalse, shiftedEntityNum);
		}
	}
	break;

	case RT_MODEL:
	case RT_GRASS:
	case RT_PLANT:
	case RT_MODEL_INSTANCED:
	{
		// we must set up parts of tr.ori for model culling
		R_RotateForEntity(ent, &tr.viewParms, &tr.ori);

		model_t *currentModel = R_GetModelByHandle(ent->e.hModel);

		//ri->Printf(PRINT_ALL, "%s\n", currentModel->name);

		if (!currentModel) {
			R_AddDrawSurfThreaded(&entitySurface, tr.defaultShader, 0, 0, R_IsPostRenderEntity(entityNum, ent), 0/* cubeMap */, qfalse, shiftedEntityNum);
		}
		else {
			switch (currentModel->type) {
			case MOD_MESH:
				R_AddMD3Surfaces(ent, currentModel, entityNum, shiftedEntityNum);
				break;
			case MOD_MDR:
				R_MDRAddAnimSurfaces(ent, currentModel, entityNum, shiftedEntityNum);
				break;
			case MOD_IQM:
				R_AddIQMSurfaces(ent, currentModel, entityNum, shiftedEntityNum);
				break;
			case MOD_BRUSH:
				R_AddBrushModelSurfaces(ent, currentModel, entityNum, shiftedEntityNum);
				break;
			case MOD_MDXM:
				if (ent->e.ghoul2)
					R_AddGhoulSurfaces(ent, currentModel, entityNum, shiftedEntityNum);
				break;
			case MOD_BAD:		// null model axis
				if ((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
					break;
				}

				if (ent->e.ghoul2 && G2API_HaveWeGhoul2Models(*((CGhoul2Info_v *)ent->e.ghoul2)))
				{
					R_AddGhoulSurfaces(ent, currentModel, entityNum, shiftedEntityNum);
					break;
				}

				R_AddDrawSurfThreaded(&entitySurface, tr.defaultShader, 0, 0, R_IsPostRenderEntity(entityNum, ent), 0 /* cubeMap */, qfalse, shiftedEntityNum);
				break;
			default:
				//ri->Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
				assert(0);
				break;
			}
		}
	}
	break;
	case RT_ENT_CHAIN:
	{
		model_t *currentModel = NULL;
		shader = R_GetShaderByHandle(ent->e.customShader);

		R_AddDrawSurfThreaded(&entitySurface, shader, R_SpriteFogNum(ent), false, R_IsPostRenderEntity(entityNum, ent), 0 /* cubeMap */, qfalse, shiftedEntityNum);
	}
	break;
	default:
	{
		model_t *currentModel = NULL;
		ri->Error(ERR_DROP, "R_AddEntitySurfaces: Bad reType");
	}
	break;
	}
}

/*
=============
R_AddEntitySurfaces
=============
*/
//#ifdef __INSTANCED_MODELS__
//extern void R_AddInstancedModelsToScene(void);
//#endif //__INSTANCED_MODELS__


static int R_SortEntities(const void *a, const void *b)
{
	trRefEntity_t	 *e1, *e2;

	e1 = (trRefEntity_t *)a;
	e2 = (trRefEntity_t *)b;

	if (e1 == &tr.worldEntity || e2 == &tr.worldEntity)
		return 0;

	if (e1->e.reType && e2->e.reType)
	{
		if (e1->e.reType > e2->e.reType)
			return -1;

		else if (e1->e.reType < e2->e.reType)
			return 1;
	}

	if (e1->e.hModel && e2->e.hModel)
	{// Group same models together, save some VBO binds...
		//model_t *model1 = R_GetModelByHandle(e1->e.hModel);
		//model_t *model2 = R_GetModelByHandle(e2->e.hModel);

		if (e1->e.hModel > e2->e.hModel)
			return -1;

		else if (e1->e.hModel < e2->e.hModel)
			return 1;
	}

	if (e1->e.ghoul2 && e2->e.ghoul2)
	{// Group same models together, save some VBO binds...
		if (e1->e.ghoul2 > e2->e.ghoul2)
			return -1;

		else if (e1->e.ghoul2 < e2->e.ghoul2)
			return 1;
	}

	/*if (e1->e.customShader && e2->e.customShader)
	{// Group same skins together, save binds...
		if (e1->e.customShader > e2->e.customShader)
			return -1;

		else if (e1->e.customShader < e2->e.customShader)
			return 1;
	}

	if (e1->e.customSkin && e2->e.customSkin)
	{// Group same skins together, save binds...
		if (e1->e.customSkin > e2->e.customSkin)
			return -1;

		else if (e1->e.customSkin < e2->e.customSkin)
			return 1;
	}

	// Distance sort...
	float dist1 = Distance(e1->e.origin, backEnd.refdef.vieworg);
	float dist2 = Distance(e2->e.origin, backEnd.refdef.vieworg);

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
	*/

	return 0;
}


void R_AddEntitySurfaces (void) {
	int i;

	if ( !r_drawentities->integer ) {
		return;
	}

	//if (r_testvalue0->integer)
	//	qsort(tr.refdef.entities, tr.refdef.num_entities, sizeof(trRefEntity_t), R_SortEntities);

//	bool useThreading = (r_testvalue0->integer && tr.refdef.num_entities > r_testvalue0->integer);

//#pragma omp parallel for if (useThreading) num_threads(r_testvalue0->integer)
	for (i = 0; i < tr.refdef.num_entities; i++)
	{
		R_AddEntitySurface(i);
	}

//#ifdef __INSTANCED_MODELS__
//	R_AddInstancedModelsToScene();
//#endif //__INSTANCED_MODELS__
}

void R_AddIgnoreCullEntitySurfaces(void) {
	int i;

	if (!r_drawentities->integer) {
		return;
	}

	for (i = 0; i < tr.refdef.num_entities; i++)
	{
		trRefEntity_t	*ent = &tr.refdef.entities[i];

		if (ent && ent->e.ignoreCull)
		{
			R_AddEntitySurface(i);
		}
	}
}

/*
====================
R_GenerateDrawSurfs
====================
*/
void R_GenerateDrawSurfs( void ) 
{
	tr.world = tr.worldSolid;

	R_AddWorldSurfaces();

	if (tr.worldNonSolid)
	{// Set extra world data pointer, and render it...
		tr.world = tr.worldNonSolid;
		R_AddWorldSurfaces();

		// Restore the original pointer to solid world...
		tr.world = tr.worldSolid;
	}

	if (!(tr.viewParms.flags & VPF_CUBEMAP))
	{// UQ: Don't fx on cubemaps... or emissivemaps
		R_AddPolygonSurfaces();
	}
	
	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation

	// dynamically compute far clip plane distance
	if (!(tr.viewParms.flags & VPF_SHADOWMAP))
	{
		R_SetFarClip();
	}

	// we know the size of the clipping volume. Now set the rest of the projection matrix.
	R_SetupProjectionZ(&tr.viewParms);

	if (!(tr.viewParms.flags & VPF_CUBEMAP))
	{// UQ: Don't render entities on cubemaps... or emissivemaps
		R_AddEntitySurfaces();
	}
}

/*
================
R_DebugPolygon
================
*/
void R_DebugPolygon( int color, int numPoints, float *points ) {
	// FIXME: implement this
#if 0
	int		i;

	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	qglColor3f( color&1, (color>>1)&1, (color>>2)&1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	GL_SetDepthRange( 0, 0 );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();
	GL_SetDepthRange( 0, 1 );
#endif
}

/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
void R_DebugGraphics( void ) {
	if ( !r_debugSurface->integer ) {
		return;
	}

	R_IssuePendingRenderCommands();

	GL_Bind( tr.whiteImage);
	GL_Cull( CT_FRONT_SIDED );
	ri->CM_DrawDebugSurface( R_DebugPolygon );
}


/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/

void R_RenderView (viewParms_t *parms) {
	int		firstDrawSurf;

	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 ) {
		return;
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer (&tr.viewParms);

	R_SetupProjection(&tr.viewParms, r_zproj->value, tr.viewParms.zFar, qtrue);

	R_GenerateDrawSurfs();

	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

	// draw main system development information (surface outlines, etc)
	R_DebugGraphics();
}

void Light_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, const int passEntityNum, const int contentmask)
{
	results->entityNum = ENTITYNUM_NONE;
	ri->CM_BoxTrace(results, start, end, mins, maxs, 0, contentmask, 0);
	results->entityNum = results->fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
}

#define __PSHADOW_USE_BEST_LIGHT__
#define __PSHADOWS_GLM__
//#define __PSHADOWS_MD3_ETC__
//#define __PSHADOWS_MISC_ENTS__

#ifdef __PSHADOW_USE_BEST_LIGHT__
#if 0 // Use deferredLighting's CURRENT_DRAW_DLIGHTS_IDS list...
extern int					CURRENT_DRAW_DLIGHTS_COUNT;
extern int					CURRENT_DRAW_DLIGHTS_IDS[MAX_DEFERRED_LIGHTS];
extern vec2_t				CURRENT_DRAW_DLIGHTS_SCREEN_POSITIONS[MAX_DEFERRED_LIGHTS];
extern vec3_t				CURRENT_DRAW_DLIGHTS_POSITIONS[MAX_DEFERRED_LIGHTS];
extern float				CURRENT_DRAW_DLIGHTS_RADIUS[MAX_DEFERRED_LIGHTS];
extern float				CURRENT_DRAW_DLIGHTS_HEIGHTSCALES[MAX_DEFERRED_LIGHTS];
extern vec3_t				CURRENT_DRAW_DLIGHTS_COLORS[MAX_DEFERRED_LIGHTS];

#define NUM_LIGHTS					CURRENT_DRAW_DLIGHTS_COUNT
#define LIGHTS_POSITIONS(a)			CURRENT_DRAW_DLIGHTS_POSITIONS[a]
#define LIGHTS_RADIUS(a)			CURRENT_DRAW_DLIGHTS_RADIUS[a]
#elif 1 // Use the JKA dlights list...
#define NUM_LIGHTS					backEnd.refdef.num_dlights
#define LIGHTS_POSITIONS(a)			(backEnd.refdef.dlights[a].origin)
#define LIGHTS_RADIUS(a)			(backEnd.refdef.dlights[a].radius * 3.0/*2.0*//*r_testvalue2->value*/)
#else // Only use static map glow lights...
extern int			CLOSE_TOTAL;
extern int			CLOSE_LIST[MAX_WORLD_GLOW_DLIGHTS];
extern float		CLOSE_DIST[MAX_WORLD_GLOW_DLIGHTS];
extern vec3_t		CLOSE_POS[MAX_WORLD_GLOW_DLIGHTS];
extern float		CLOSE_RADIUS[MAX_WORLD_GLOW_DLIGHTS];
extern float		CLOSE_HEIGHTSCALES[MAX_WORLD_GLOW_DLIGHTS];

extern float		MAP_EMISSIVE_RADIUS_SCALE;

#define NUM_LIGHTS					CLOSE_TOTAL
#define LIGHTS_POSITIONS(a)			CLOSE_POS[a]
#define LIGHTS_RADIUS(a)			CLOSE_RADIUS[a]
#endif
#endif //__PSHADOW_USE_BEST_LIGHT__

#ifdef __USE_MAP_EMMISSIVE_BLOCK__
extern EmissiveLightBlock_t				EmissiveLightsBlock;
extern EmissiveLightBlock_t				EmissiveLightsBlockPrevious;
extern int								emissiveUpdateTime;
extern int								NUM_CURRENT_EMISSIVE_DRAW_LIGHTS;
#endif //__USE_MAP_EMMISSIVE_BLOCK__

void R_RenderPshadowMaps(const refdef_t *fd)
{
#ifdef __PSHADOWS__
	viewParms_t		shadowParms;
	int i;
	float invLightPower = 1.0;

	vec3_t playerOrigin;

	if (backEnd.localPlayerValid)
	{
		VectorCopy(backEnd.localPlayerOrigin, playerOrigin);
		//ri->Printf(PRINT_WARNING, "Local player is at %f %f %f.\n", backEnd.localPlayerOrigin[0], backEnd.localPlayerOrigin[1], backEnd.localPlayerOrigin[2]);
	}
	else
	{
		VectorCopy(backEnd.refdef.vieworg, playerOrigin);
		//ri->Printf(PRINT_WARNING, "No Local player! Using vieworg at %f %f %f.\n", backEnd.localPlayerOrigin[0], backEnd.localPlayerOrigin[1], backEnd.localPlayerOrigin[2]);
	}

#define MAX_PSHADOW_RADIUS 512.0//r_testvalue0->value//1024.0

	vec3_t	bestLightPosition;
	float	bestLightRadius = MAX_PSHADOW_RADIUS;

#ifdef __INDOOR_OUTDOOR_CULLING__
	if (!backEnd.viewIsOutdoors || !SHADOWS_ENABLED || RB_NightScale() == 1.0)
#endif //__INDOOR_OUTDOOR_CULLING__
	{// Only use pshadows inside, when sun shadows are disabled, or at night...
#ifdef __PSHADOW_USE_BEST_LIGHT__
		// Let's try to find a real light source...
		int		dLight = -1;
		float	dLightFactor = 0.0;

		for (int i = 0; i < NUM_LIGHTS; i++)
		{
			float radius = LIGHTS_RADIUS(i);
			float dist = Distance(playerOrigin, LIGHTS_POSITIONS(i));

			if (dist > radius /*|| LIGHTS_POSITIONS(i)[2] < playerOrigin[2]*/)
			{// Not in range of this light...
				continue;
			}

			float factor = 1.0 - Q_clamp(0.0, dist / radius, 1.0);

			if (factor > dLightFactor)
			{
				dLight = i;
				dLightFactor = factor;
			}
		}

		if (dLight == -1)
		{// Nothing in radius found...
#ifdef __USE_MAP_EMMISSIVE_BLOCK__
			// Let's try to find a real light source from emissive lights...
			int		dLight = -1;
			float	dLightFactor = 0.0;

			for (int i = 0; i < NUM_CURRENT_EMISSIVE_DRAW_LIGHTS; i++)
			{
				float radius = EmissiveLightsBlock.lights[i].u_lightPositions2[3] * 3.0;
				float dist = Distance(playerOrigin, EmissiveLightsBlock.lights[i].u_lightPositions2);

				if (dist > radius /*|| LIGHTS_POSITIONS(i)[2] < playerOrigin[2]*/)
				{// Not in range of this light...
					continue;
				}

				float factor = 1.0 - Q_clamp(0.0, dist / radius, 1.0);

				if (factor > dLightFactor)
				{
					dLight = i;
					dLightFactor = factor;
				}
			}

			if (dLight == -1)
			{// Nothing in radius found...
				return;
			}
			else
			{// Seems that we found an emissive light to use...
				invLightPower = Q_clamp(0.0, 0.01*(1.0 - dLightFactor), 1.0);
				if (invLightPower == 1.0) return;
				bestLightPosition[0] = mix(EmissiveLightsBlock.lights[dLight].u_lightPositions2[0], playerOrigin[0], invLightPower);
				bestLightPosition[1] = mix(EmissiveLightsBlock.lights[dLight].u_lightPositions2[1], playerOrigin[1], invLightPower);
				bestLightPosition[2] = playerOrigin[2] + 64.0;
				bestLightRadius = mix(EmissiveLightsBlock.lights[dLight].u_lightPositions2[3] * 3.0, 128.0, invLightPower);
			}
#else //!__USE_MAP_EMMISSIVE_BLOCK__
			return;
#endif //__USE_MAP_EMMISSIVE_BLOCK__
		}
		else
		{
			// Seems that we found a dlight to use...
			invLightPower = Q_clamp(0.0, 0.01*(1.0-dLightFactor), 1.0);
			if (invLightPower == 1.0) return;
			bestLightPosition[0] = mix(LIGHTS_POSITIONS(dLight)[0], playerOrigin[0], invLightPower);
			bestLightPosition[1] = mix(LIGHTS_POSITIONS(dLight)[1], playerOrigin[1], invLightPower);
			bestLightPosition[2] = playerOrigin[2] + 64.0;
			bestLightRadius = mix(LIGHTS_RADIUS(dLight), 128.0, invLightPower);
		}
#else
		/*vec3_t ambientLight, directedLight, lightDir;
		R_LightForPoint(playerOrigin, ambientLight, directedLight, lightDir);

		// sometimes there's no light
		if (DotProduct(lightDir, lightDir) < 0.9f)
		{
			VectorSet(lightDir, 0.0f, 0.0f, 1.0f);
			//return;
		}

		VectorMA(playerOrigin, bestLightRadius, lightDir, bestLightPosition);*/

		/*vec3_t lightDir;
		lightDir[0] = 0.0;
		lightDir[1] = 0.0;
		if (r_testvalue0->integer < 1)
			lightDir[2] = 1.0;
		else
			lightDir[2] = -1.0;
		lightDir[3] = 0.0;*/

		VectorCopy(backEnd.viewIsOutdoorsHitPosition, bestLightPosition);

		bestLightPosition[2] -= r_testvalue1->value;
#endif //__PSHADOW_USE_BEST_LIGHT__
	}
#ifdef __INDOOR_OUTDOOR_CULLING__
	else
	{
		return;
	}
#endif //__INDOOR_OUTDOOR_CULLING__

	// first, make a list of GLM shadows
	for (i = 0; i < tr.refdef.num_entities; i++)
	{
		trRefEntity_t *ent = &tr.refdef.entities[i];

		if (Distance(playerOrigin, ent->e.origin) - ent->e.radius > bestLightRadius)
		{
			continue;
		}

		if (ent->e.reType == RT_MODEL)
		{
			model_t *model = R_GetModelByHandle(ent->e.hModel);
			pshadow_t shadow;
			float radius = 0.0f;
			float scale = 1.0f;
			vec3_t diff;
			int j;

			if (!model)
				continue;

			if (ent->e.nonNormalizedAxes)
			{
				scale = VectorLength(ent->e.axis[0]);
			}

			switch (model->type)
			{
			case MOD_MDXM:
			{
				if (ent->e.ghoul2)
				{
					// scale the radius if needed
					float largestScale = ent->e.modelScale[0];
					if (ent->e.modelScale[1] > largestScale)
						largestScale = ent->e.modelScale[1];
					if (ent->e.modelScale[2] > largestScale)
						largestScale = ent->e.modelScale[2];
					if (!largestScale)
						largestScale = 1;
					
					largestScale *= 1.2;

					radius = ent->e.radius * largestScale;
				}
			}
			break;
			case MOD_BAD:
			{
				if (ent->e.ghoul2 && G2API_HaveWeGhoul2Models(*((CGhoul2Info_v *)ent->e.ghoul2)))
				{
					// scale the radius if needed
					float largestScale = ent->e.modelScale[0];
					if (ent->e.modelScale[1] > largestScale)
						largestScale = ent->e.modelScale[1];
					if (ent->e.modelScale[2] > largestScale)
						largestScale = ent->e.modelScale[2];
					if (!largestScale)
						largestScale = 1;
					
					largestScale *= 1.2;

					radius = ent->e.radius * largestScale;
				}
			}
			break;
			default:
				break;
			}

			if (!radius)
				continue;

			// Cull entities that are behind the viewer by more than lightRadius
			VectorSubtract(ent->e.origin, fd->vieworg, diff);
			if (DotProduct(diff, fd->viewaxis[0]) < -r_pshadowDist->value)
				continue;

			memset(&shadow, 0, sizeof(shadow));

			shadow.numEntities = 1;
			shadow.entityNums[0] = i;
			shadow.viewRadius = radius;
			shadow.lightRadius = r_pshadowDist->value;
			VectorCopy(ent->e.origin, shadow.viewOrigin);
			shadow.sort = DotProduct(diff, diff) / (radius * radius);
			VectorCopy(ent->e.origin, shadow.entityOrigins[0]);
			shadow.entityRadiuses[0] = radius;

			for (j = 0; j < MAX_CALC_PSHADOWS; j++)
			{
				pshadow_t swap;

				if (j + 1 > tr.refdef.num_pshadows)
				{
					tr.refdef.num_pshadows = j + 1;
					tr.refdef.pshadows[j] = shadow;
					break;
				}

				// sort shadows by distance from camera divided by radius
				// FIXME: sort better
				if (tr.refdef.pshadows[j].sort <= shadow.sort)
					continue;

				swap = tr.refdef.pshadows[j];
				tr.refdef.pshadows[j] = shadow;
				shadow = swap;
			}
		}
	}

#ifdef __PSHADOWS_MD3_ETC__
	// now, if there's room, make a list of MD3, etc, shadows
	for (i = tr.refdef.num_pshadows; i < tr.refdef.num_entities; i++)
	{
		trRefEntity_t *ent = &tr.refdef.entities[i];

		if ((ent->e.renderfx & (RF_FIRST_PERSON | RF_NOSHADOW)))
			continue;

		//if((ent->e.renderfx & RF_THIRD_PERSON))
		//continue;

		if (Distance(playerOrigin, ent->e.origin) - ent->e.radius > bestLightRadius)
		{
			continue;
		}

		if (ent->e.reType == RT_MODEL || ent->e.reType == RT_PLANT || ent->e.reType == RT_GRASS || ent->e.reType == RT_MODEL_INSTANCED)
		{
			model_t *model = R_GetModelByHandle(ent->e.hModel);
			pshadow_t shadow;
			float radius = 0.0f;
			float scale = 1.0f;
			vec3_t diff;
			int j;

			if (!model)
				continue;

			if (ent->e.nonNormalizedAxes)
			{
				scale = VectorLength(ent->e.axis[0]);
			}

			switch (model->type)
			{
			case MOD_MESH:
			{
				mdvFrame_t *frame = &model->data.mdv[0]->frames[ent->e.frame];
				radius = frame->radius * scale;
			}
			break;
			case MOD_MDR:
			{
				// FIXME: never actually tested this
				mdrHeader_t *header = model->data.mdr;
				int frameSize = (size_t)(&((mdrFrame_t *)0)->bones[header->numBones]);
				mdrFrame_t *frame = (mdrFrame_t *)((byte *)header + header->ofsFrames + frameSize * ent->e.frame);
				radius = frame->radius;
			}
			break;
			case MOD_IQM:
			{
				// FIXME: never actually tested this
				iqmData_t *data = model->data.iqm;
				vec3_t diag;
				float *framebounds;

				framebounds = data->bounds + 6 * ent->e.frame;
				VectorSubtract(framebounds + 3, framebounds, diag);
				radius = 0.5f * VectorLength(diag);
			}
			break;
			default:
				break;
			}

			if (!radius)
				continue;

			// Cull entities that are behind the viewer by more than lightRadius
			VectorSubtract(ent->e.origin, fd->vieworg, diff);
			if (DotProduct(diff, fd->viewaxis[0]) < -r_pshadowDist->value)
				continue;

			memset(&shadow, 0, sizeof(shadow));

			shadow.numEntities = 1;
			shadow.entityNums[0] = i;
			shadow.viewRadius = radius;
			shadow.lightRadius = r_pshadowDist->value;
			VectorCopy(ent->e.origin, shadow.viewOrigin);
			shadow.sort = DotProduct(diff, diff) / (radius * radius);
			VectorCopy(ent->e.origin, shadow.entityOrigins[0]);
			shadow.entityRadiuses[0] = radius;

			for (j = tr.refdef.num_pshadows/*0*/; j < MAX_CALC_PSHADOWS; j++)
			{
				pshadow_t swap;

				if (j + 1 > tr.refdef.num_pshadows)
				{
					tr.refdef.num_pshadows = j + 1;
					tr.refdef.pshadows[j] = shadow;
					break;
				}

				// sort shadows by distance from camera divided by radius
				// FIXME: sort better
				if (tr.refdef.pshadows[j].sort <= shadow.sort)
					continue;

				swap = tr.refdef.pshadows[j];
				tr.refdef.pshadows[j] = shadow;
				shadow = swap;
			}
		}
	}

#ifdef __PSHADOWS_MISC_ENTS__
	// now, if there's room, make a list of misc entity, shadows
	for (i = 0; i < tr.refdef.num_entities; i++)
	{
		trRefEntity_t *ent = &tr.refdef.entities[i];

		if ((ent->e.renderfx & (RF_FIRST_PERSON | RF_NOSHADOW)))
			continue;

		//if((ent->e.renderfx & RF_THIRD_PERSON))
		//continue;

		if (Distance(playerOrigin, ent->e.origin) - ent->e.radius > bestLightRadius)
		{
			continue;
		}

		if (ent->e.reType == RT_SPRITE
			|| ent->e.reType == RT_BEAM
			|| ent->e.reType == RT_ORIENTED_QUAD
			|| ent->e.reType == RT_ELECTRICITY
			|| ent->e.reType == RT_LINE
			|| ent->e.reType == RT_ORIENTEDLINE
			|| ent->e.reType == RT_CYLINDER
			|| ent->e.reType == RT_SABER_GLOW
			|| ent->e.reType == RT_ENT_CHAIN)
		{
			pshadow_t shadow;
			float radius = 0.0f;
			float scale = 1.0f;
			vec3_t diff;
			int j;

			radius = ent->e.radius * scale;

			if (!radius)
				continue;

			// Cull entities that are behind the viewer by more than lightRadius
			VectorSubtract(ent->e.origin, fd->vieworg, diff);
			if (DotProduct(diff, fd->viewaxis[0]) < -r_pshadowDist->value)
				continue;

			memset(&shadow, 0, sizeof(shadow));

			shadow.numEntities = 1;
			shadow.entityNums[0] = i;
			shadow.viewRadius = radius;
			shadow.lightRadius = r_pshadowDist->value;
			VectorCopy(ent->e.origin, shadow.viewOrigin);
			shadow.sort = DotProduct(diff, diff) / (radius * radius);
			VectorCopy(ent->e.origin, shadow.entityOrigins[0]);
			shadow.entityRadiuses[0] = radius;

			for (j = tr.refdef.num_pshadows; j < MAX_CALC_PSHADOWS; j++)
			{
				pshadow_t swap;

				if (j + 1 > tr.refdef.num_pshadows)
				{
					tr.refdef.num_pshadows = j + 1;
					tr.refdef.pshadows[j] = shadow;
					break;
				}

				// sort shadows by distance from camera divided by radius
				// FIXME: sort better
				if (tr.refdef.pshadows[j].sort <= shadow.sort)
					continue;

				swap = tr.refdef.pshadows[j];
				tr.refdef.pshadows[j] = shadow;
				shadow = swap;
			}
		}
	}
#endif //__PSHADOWS_MISC_ENTS__
#endif //__PSHADOWS_GLM__

#if 0
	// next, merge touching pshadows
	if (0) //for ( i = 0; i < tr.refdef.num_pshadows; i++)
	{
		pshadow_t *ps1 = &tr.refdef.pshadows[i];
		int j;

		for (j = i + 1; j < tr.refdef.num_pshadows; j++)
		{
			pshadow_t *ps2 = &tr.refdef.pshadows[j];
			int k;
			qboolean touch;

			if (ps1->numEntities == 8)
				break;

			touch = qfalse;
			if (SpheresIntersect(ps1->viewOrigin, ps1->viewRadius, ps2->viewOrigin, ps2->viewRadius))
			{
				for (k = 0; k < ps1->numEntities; k++)
				{
					if (SpheresIntersect(ps1->entityOrigins[k], ps1->entityRadiuses[k], ps2->viewOrigin, ps2->viewRadius))
					{
						touch = qtrue;
						break;
					}
				}
			}

			if (touch)
			{
				vec3_t newOrigin;
				float newRadius;

				BoundingSphereOfSpheres(ps1->viewOrigin, ps1->viewRadius, ps2->viewOrigin, ps2->viewRadius, newOrigin, &newRadius);
				VectorCopy(newOrigin, ps1->viewOrigin);
				ps1->viewRadius = newRadius;

				ps1->entityNums[ps1->numEntities] = ps2->entityNums[0];
				VectorCopy(ps2->viewOrigin, ps1->entityOrigins[ps1->numEntities]);
				ps1->entityRadiuses[ps1->numEntities] = ps2->viewRadius;

				ps1->numEntities++;

				for (k = j; k < tr.refdef.num_pshadows - 1; k++)
				{
					tr.refdef.pshadows[k] = tr.refdef.pshadows[k + 1];
				}

				j--;
				tr.refdef.num_pshadows--;
			}
		}
	}
#endif

	// cap number of drawn pshadows
	if (tr.refdef.num_pshadows > MAX_DRAWN_PSHADOWS)
	{
		tr.refdef.num_pshadows = MAX_DRAWN_PSHADOWS;
	}

	// next, fill up the rest of the shadow info
	for (i = 0; i < tr.refdef.num_pshadows; i++)
	{
		pshadow_t *shadow = &tr.refdef.pshadows[i];
		vec3_t up;
		vec3_t lightDir;

		//VectorSet(lightDir, 0.57735f, 0.57735f, 0.57735f);



		//shadow->lightRadius = min(bestLightRadius, MAX_PSHADOW_RADIUS);
		shadow->lightRadius = bestLightRadius;
		VectorCopy(bestLightPosition, shadow->realLightOrigin);
		shadow->invLightPower = invLightPower;

		VectorSubtract(shadow->viewOrigin, bestLightPosition, lightDir);
		VectorNormalize(lightDir);
		VectorMA(shadow->viewOrigin, shadow->viewRadius, lightDir, shadow->lightOrigin);

		// make up a projection, up doesn't matter
		VectorScale(lightDir, -1.0f, shadow->lightViewAxis[0]);
		VectorSet(up, 0, 0, -1);

		//shadow->lightRadius = min(bestLightRadius, MAX_PSHADOW_RADIUS/**0.5*/);




		if (fabs(DotProduct(up, shadow->lightViewAxis[0])) > 0.9f)
		{
			VectorSet(up, -1, 0, 0);
		}

		//if (shadow->lightRadius > 512.0)//256.0)
		//{
		//	shadow->lightRadius = 512.0;// 256.0;
		//}

		CrossProduct(shadow->lightViewAxis[0], up, shadow->lightViewAxis[1]);
		VectorNormalize(shadow->lightViewAxis[1]);
		CrossProduct(shadow->lightViewAxis[0], shadow->lightViewAxis[1], shadow->lightViewAxis[2]);

		VectorCopy(shadow->lightViewAxis[0], shadow->cullPlane.normal);
		shadow->cullPlane.dist = DotProduct(shadow->cullPlane.normal, shadow->lightOrigin);
		shadow->cullPlane.type = PLANE_NON_AXIAL;
		SetPlaneSignbits(&shadow->cullPlane);
	}

	// next, render shadowmaps
	for (i = 0; i < tr.refdef.num_pshadows; i++)
	{
		int firstDrawSurf;
		pshadow_t *shadow = &tr.refdef.pshadows[i];
		int j;

		if (!shadow->numEntities) continue;

		Com_Memset(&shadowParms, 0, sizeof(shadowParms));

		shadowParms.viewportX = 0;
		shadowParms.viewportY = 0;
		shadowParms.viewportWidth = tr.pshadowMaps[0]->width;
		shadowParms.viewportHeight = tr.pshadowMaps[0]->height;
		shadowParms.isPortal = qfalse;
		shadowParms.isMirror = qfalse;

		shadowParms.fovX = 90;
		shadowParms.fovY = 90;

		shadowParms.targetFbo = tr.pshadowFbos[i];

		shadowParms.flags = (viewParmFlags_t)(VPF_DEPTHSHADOW | VPF_NOVIEWMODEL);
		shadowParms.zFar = shadow->lightRadius;

		VectorCopy(shadow->lightOrigin, shadowParms.ori.origin);

		VectorCopy(shadow->realLightOrigin, shadowParms.realLightOrigin);

		VectorCopy(shadow->lightViewAxis[0], shadowParms.ori.axis[0]);
		VectorCopy(shadow->lightViewAxis[1], shadowParms.ori.axis[1]);
		VectorCopy(shadow->lightViewAxis[2], shadowParms.ori.axis[2]);

		{
			tr.viewCount++;

			tr.viewParms = shadowParms;
			tr.viewParms.frameSceneNum = tr.frameSceneNum;
			tr.viewParms.frameCount = tr.frameCount;

			firstDrawSurf = tr.refdef.numDrawSurfs;

			tr.viewCount++;

			// set viewParms.world
			R_RotateForViewer(&tr.viewParms);

			{
				float xmin, xmax, ymin, ymax, znear, zfar;
				viewParms_t *dest = &tr.viewParms;
				vec3_t pop;

				xmin = ymin = -shadow->viewRadius;
				xmax = ymax = shadow->viewRadius;
				znear = 0;
				zfar = shadow->lightRadius;

				dest->projectionMatrix[0] = 2 / (xmax - xmin);
				dest->projectionMatrix[4] = 0;
				dest->projectionMatrix[8] = (xmax + xmin) / (xmax - xmin);
				dest->projectionMatrix[12] = 0;

				dest->projectionMatrix[1] = 0;
				dest->projectionMatrix[5] = 2 / (ymax - ymin);
				dest->projectionMatrix[9] = (ymax + ymin) / (ymax - ymin);	// normally 0
				dest->projectionMatrix[13] = 0;

				dest->projectionMatrix[2] = 0;
				dest->projectionMatrix[6] = 0;
				dest->projectionMatrix[10] = 2 / (zfar - znear);
				dest->projectionMatrix[14] = 0;

				dest->projectionMatrix[3] = 0;
				dest->projectionMatrix[7] = 0;
				dest->projectionMatrix[11] = 0;
				dest->projectionMatrix[15] = 1;

				VectorScale(dest->ori.axis[1], 1.0f, dest->frustum[0].normal);
				VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[0].normal, pop);
				dest->frustum[0].dist = DotProduct(pop, dest->frustum[0].normal);

				VectorScale(dest->ori.axis[1], -1.0f, dest->frustum[1].normal);
				VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[1].normal, pop);
				dest->frustum[1].dist = DotProduct(pop, dest->frustum[1].normal);

				VectorScale(dest->ori.axis[2], 1.0f, dest->frustum[2].normal);
				VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[2].normal, pop);
				dest->frustum[2].dist = DotProduct(pop, dest->frustum[2].normal);

				VectorScale(dest->ori.axis[2], -1.0f, dest->frustum[3].normal);
				VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[3].normal, pop);
				dest->frustum[3].dist = DotProduct(pop, dest->frustum[3].normal);

				VectorScale(dest->ori.axis[0], -1.0f, dest->frustum[4].normal);
				VectorMA(dest->ori.origin, -shadow->lightRadius, dest->frustum[4].normal, pop);
				dest->frustum[4].dist = DotProduct(pop, dest->frustum[4].normal);

				for (j = 0; j < 5; j++)
				{
					dest->frustum[j].type = PLANE_NON_AXIAL;
					SetPlaneSignbits(&dest->frustum[j]);
				}

				dest->flags |= VPF_FARPLANEFRUSTUM;
			}

//#define __PSHADOWS_ADD_WORLD__
#ifdef __PSHADOWS_ADD_WORLD__
			extern void R_AddWorldSurface(msurface_t *surf, int entityNum, int dlightBits, int pshadowBits, qboolean dontCache);

			/*vec3_t lightviewBounds[2];
			//VectorSet(lightviewBounds[0], shadow->realLightOrigin[0] - shadow->lightRadius, shadow->realLightOrigin[1] - shadow->lightRadius, shadow->realLightOrigin[2] - shadow->lightRadius);
			//VectorSet(lightviewBounds[1], shadow->realLightOrigin[0] + shadow->lightRadius, shadow->realLightOrigin[1] + shadow->lightRadius, shadow->realLightOrigin[2] + shadow->lightRadius);
			matrix_t lightViewMatrix;
			vec4_t point, base, lightViewPoint;
			float lx, ly;

			base[3] = 1;
			point[3] = 1;
			lightViewPoint[3] = 1;

			Matrix16View(shadow->lightViewAxis, shadow->lightOrigin, lightViewMatrix);

			ClearBounds(lightviewBounds[0], lightviewBounds[1]);

			float splitZNear = 1.0;

			// add view near plane
			lx = splitZNear * tan(fd->fov_x * M_PI / 360.0f);
			ly = splitZNear * tan(fd->fov_y * M_PI / 360.0f);
			VectorMA(fd->vieworg, splitZNear, fd->viewaxis[0], base);

			VectorMA(base, lx, fd->viewaxis[1], point);
			VectorMA(point, ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

			VectorMA(base, -lx, fd->viewaxis[1], point);
			VectorMA(point, ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

			VectorMA(base, lx, fd->viewaxis[1], point);
			VectorMA(point, -ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

			VectorMA(base, -lx, fd->viewaxis[1], point);
			VectorMA(point, -ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);


			// add view far plane
			float splitZFar = shadow->viewRadius;

			lx = splitZFar * tan(fd->fov_x * M_PI / 360.0f);
			ly = splitZFar * tan(fd->fov_y * M_PI / 360.0f);
			VectorMA(fd->vieworg, splitZFar, fd->viewaxis[0], base);

			VectorMA(base, lx, fd->viewaxis[1], point);
			VectorMA(point, ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

			VectorMA(base, -lx, fd->viewaxis[1], point);
			VectorMA(point, ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

			VectorMA(base, lx, fd->viewaxis[1], point);
			VectorMA(point, -ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

			VectorMA(base, -lx, fd->viewaxis[1], point);
			VectorMA(point, -ly, fd->viewaxis[2], point);
			Matrix16Transform(lightViewMatrix, point, lightViewPoint);
			AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

			R_SetupProjectionOrtho(&tr.viewParms, lightviewBounds);*/

			//float origZfar = tr.viewParms.zFar;
			//tr.viewParms.zFar = shadow->lightRadius * 2.0;
			
			//R_AddWorldSurfaces();
			for (i = 0; i < tr.world->numWorldSurfaces; i++)
			{
				//if (tr.world->surfacesViewCount[i] != tr.viewCount)
				//	continue;

				if (!(tr.world->surfaces + i)->isMerged)
				{
					//R_AddWorldSurface(tr.world->surfaces + i, tr.currentEntityNum, 0, tr.world->surfacesPshadowBits[i], qtrue);

					R_AddDrawSurf((tr.world->surfaces + i)->data, (tr.world->surfaces + i)->shader,
						0,
						0, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0, qfalse);
				}
			}

			for (i = 0; i < tr.world->numMergedSurfaces; i++)
			{
				//if (tr.world->mergedSurfacesViewCount[i] != tr.viewCount)
				//	continue;

				//R_AddWorldSurface(tr.world->mergedSurfaces + i, tr.currentEntityNum, 0, tr.world->mergedSurfacesPshadowBits[i], qtrue);

				R_AddDrawSurf((tr.world->mergedSurfaces + i)->data, (tr.world->mergedSurfaces + i)->shader,
					0,
					0, R_IsPostRenderEntity(tr.currentEntityNum, tr.currentEntity), 0, qfalse);
			}

			//tr.viewParms.zFar = origZfar;
#endif //__PSHADOWS_ADD_WORLD__

			for (j = 0; j < shadow->numEntities && j < 8; j++)
			{
				R_AddEntitySurface(shadow->entityNums[j]);
			}

			R_SortDrawSurfs(tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf);
		}
	}
#endif
}

static float CalcSplit(float n, float f, float i, float m)
{
	return (n * pow(f / n, i / m) + (f - n) * i / m) / 2.0f;
}

extern float	MAP_INFO_MAXSIZE;

void R_RenderSunShadowMaps(const refdef_t *fd, int level, vec4_t sunDir, float lightHeight, vec3_t lightOrg)
{
	viewParms_t		shadowParms;
	vec4_t lightDir;//, lightCol;
	vec3_t lightViewAxis[3];
	vec3_t lightOrigin;
	float splitZNear, splitZFar;
	float viewZNear;// , viewZFar;
	vec3_t lightviewBounds[2];
	qboolean lightViewIndependentOfCameraView = qtrue;// qfalse;

	VectorCopy4(sunDir, lightDir);

	viewZNear = r_znear->value;// r_shadowCascadeZNear->value;
	//viewZFar = SHADOW_ZFAR;// 4096.0;// r_shadowCascadeZFar->value;
	float splitBias = r_shadowCascadeZBias->value;

	switch (level)
	{
	case 0:
	default:
		splitZNear = viewZNear;
		splitZFar = SHADOW_CASCADE1 + SHADOW_CASCADE_BIAS1; // CalcSplit(viewZNear, viewZFar, 1, 3) + splitBias;
		break;
	case 1:
		splitZNear = SHADOW_CASCADE1 - SHADOW_CASCADE_BIAS1; // CalcSplit(viewZNear, viewZFar, 1, 3) + splitBias;
		splitZFar = SHADOW_CASCADE2 + SHADOW_CASCADE_BIAS2; // viewZFar;
		break;
	case 2:
		splitZNear = SHADOW_CASCADE2 - SHADOW_CASCADE_BIAS2; // CalcSplit(viewZNear, viewZFar, 1, 3) + splitBias;
		splitZFar = SHADOW_CASCADE3 + SHADOW_CASCADE_BIAS3; // viewZFar;
		break;
	case 3:
		splitZNear = SHADOW_CASCADE3 - SHADOW_CASCADE_BIAS3; // CalcSplit(viewZNear, viewZFar, 1, 3) + splitBias;
		splitZFar = SHADOW_CASCADE4 + SHADOW_CASCADE_BIAS4; // viewZFar;
		break;
	case 4:
		splitZNear = SHADOW_CASCADE4 - SHADOW_CASCADE_BIAS4; // viewZFar;
		splitZFar = backEnd.viewParms.zFar;// MAP_INFO_MAXSIZE;
		break;
	}

	if (ENABLE_OCCLUSION_CULLING && splitZNear > tr.occlusionZfar)
	{
		return;
	}

	VectorCopy(lightOrg, lightOrigin);
	//VectorCopy(fd->vieworg, lightOrigin);


	// Make up a projection
	VectorScale(lightDir, -1.0f, lightViewAxis[0]);

	if (lightViewIndependentOfCameraView)
	{
		// Use world up as light view up
		VectorSet(lightViewAxis[2], 0, 0, 1);
	}
	else if (level == 0)
	{
		// Level 0 tries to use a diamond texture orientation relative to camera view
		// Use halfway between camera view forward and left for light view up
		VectorAdd(fd->viewaxis[0], fd->viewaxis[1], lightViewAxis[2]);
	}
	else
	{
		// Use camera view up as light view up
		VectorCopy(fd->viewaxis[2], lightViewAxis[2]);
	}

	// Check if too close to parallel to light direction
	if (abs(DotProduct(lightViewAxis[2], lightViewAxis[0])) > 0.9f)
	{
		if (lightViewIndependentOfCameraView)
		{
			// Use world left as light view up
			VectorSet(lightViewAxis[2], 0, 1, 0);
		}
		else if (level == 0)
		{
			// Level 0 tries to use a diamond texture orientation relative to camera view
			// Use halfway between camera view forward and up for light view up
			VectorAdd(fd->viewaxis[0], fd->viewaxis[2], lightViewAxis[2]);
		}
		else
		{
			// Use camera view left as light view up
			VectorCopy(fd->viewaxis[1], lightViewAxis[2]);
		}
	}

	// clean axes
	CrossProduct(lightViewAxis[2], lightViewAxis[0], lightViewAxis[1]);
	VectorNormalize(lightViewAxis[1]);
	CrossProduct(lightViewAxis[0], lightViewAxis[1], lightViewAxis[2]);

	// Create bounds for light projection using slice of view projection
	{
		matrix_t lightViewMatrix;
		vec4_t point, base, lightViewPoint;
		float lx, ly;

		base[3] = 1;
		point[3] = 1;
		lightViewPoint[3] = 1;

		Matrix16View(lightViewAxis, lightOrigin, lightViewMatrix);

		ClearBounds(lightviewBounds[0], lightviewBounds[1]);

		// add view near plane
		lx = splitZNear * tan(fd->fov_x * M_PI / 360.0f);
		ly = splitZNear * tan(fd->fov_y * M_PI / 360.0f);
		VectorMA(fd->vieworg, splitZNear, fd->viewaxis[0], base);

		VectorMA(base,   lx, fd->viewaxis[1], point);
		VectorMA(point,  ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

		VectorMA(base,  -lx, fd->viewaxis[1], point);
		VectorMA(point,  ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

		VectorMA(base,   lx, fd->viewaxis[1], point);
		VectorMA(point, -ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

		VectorMA(base,  -lx, fd->viewaxis[1], point);
		VectorMA(point, -ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);
		

		// add view far plane
		lx = splitZFar * tan(fd->fov_x * M_PI / 360.0f);
		ly = splitZFar * tan(fd->fov_y * M_PI / 360.0f);
		VectorMA(fd->vieworg, splitZFar, fd->viewaxis[0], base);

		VectorMA(base,   lx, fd->viewaxis[1], point);
		VectorMA(point,  ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

		VectorMA(base,  -lx, fd->viewaxis[1], point);
		VectorMA(point,  ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

		VectorMA(base,   lx, fd->viewaxis[1], point);
		VectorMA(point, -ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

		VectorMA(base,  -lx, fd->viewaxis[1], point);
		VectorMA(point, -ly, fd->viewaxis[2], point);
		Matrix16Transform(lightViewMatrix, point, lightViewPoint);
		AddPointToBounds(lightViewPoint, lightviewBounds[0], lightviewBounds[1]);

		

		// Moving the Light in Texel-Sized Increments
		// from http://msdn.microsoft.com/en-us/library/windows/desktop/ee416324%28v=vs.85%29.aspx
		//
		if (lightViewIndependentOfCameraView)
		{
			float cascadeBound, worldUnitsPerTexel, invWorldUnitsPerTexel;

			cascadeBound = MAX(lightviewBounds[1][0] - lightviewBounds[0][0], lightviewBounds[1][1] - lightviewBounds[0][1]);
			cascadeBound = MAX(cascadeBound, lightviewBounds[1][2] - lightviewBounds[0][2]);
			worldUnitsPerTexel = cascadeBound / tr.sunShadowFbo[level]->width;
			invWorldUnitsPerTexel = 1.0f / worldUnitsPerTexel;

			VectorScale(lightviewBounds[0], invWorldUnitsPerTexel, lightviewBounds[0]);
			lightviewBounds[0][0] = floor(lightviewBounds[0][0]);
			lightviewBounds[0][1] = floor(lightviewBounds[0][1]);
			lightviewBounds[0][2] = floor(lightviewBounds[0][2]);
			VectorScale(lightviewBounds[0], worldUnitsPerTexel, lightviewBounds[0]);

			VectorScale(lightviewBounds[1], invWorldUnitsPerTexel, lightviewBounds[1]);
			lightviewBounds[1][0] = floor(lightviewBounds[1][0]);
			lightviewBounds[1][1] = floor(lightviewBounds[1][1]);
			lightviewBounds[1][2] = floor(lightviewBounds[1][2]);
			VectorScale(lightviewBounds[1], worldUnitsPerTexel, lightviewBounds[1]);
		}

		//ri->Printf(PRINT_ALL, "znear %f zfar %f\n", lightviewBounds[0][0], lightviewBounds[1][0]);		
		//ri->Printf(PRINT_ALL, "fovx %f fovy %f xmin %f xmax %f ymin %f ymax %f\n", fd->fov_x, fd->fov_y, xmin, xmax, ymin, ymax);
	}


	{
		int firstDrawSurf;

		Com_Memset( &shadowParms, 0, sizeof( shadowParms ) );

		shadowParms.viewportX = 0;
		shadowParms.viewportY = 0;
		shadowParms.viewportWidth  = tr.sunShadowFbo[level]->width;
		shadowParms.viewportHeight = tr.sunShadowFbo[level]->height;
		shadowParms.isPortal = qfalse;
		shadowParms.isMirror = qfalse;

		shadowParms.fovX = 90;
		shadowParms.fovY = 90;

		shadowParms.targetFbo = tr.sunShadowFbo[level];

		shadowParms.flags = (viewParmFlags_t)( VPF_DEPTHSHADOW | VPF_DEPTHCLAMP | VPF_ORTHOGRAPHIC | VPF_NOVIEWMODEL | VPF_SHADOWPASS );
		shadowParms.zFar = lightviewBounds[1][0];

		if (level <= 0)
			shadowParms.maxEntityRange = 4096;
		else if (level <= 1)
			shadowParms.maxEntityRange = 8192;
		else
			shadowParms.maxEntityRange = 16384;

		VectorCopy(lightOrigin, shadowParms.ori.origin);
		
		VectorCopy(lightViewAxis[0], shadowParms.ori.axis[0]);
		VectorCopy(lightViewAxis[1], shadowParms.ori.axis[1]);
		VectorCopy(lightViewAxis[2], shadowParms.ori.axis[2]);

		VectorCopy(lightOrigin, shadowParms.pvsOrigin );

		{
			tr.viewCount++;

			tr.viewParms = shadowParms;
			tr.viewParms.frameSceneNum = tr.frameSceneNum;
			tr.viewParms.frameCount = tr.frameCount;

			firstDrawSurf = tr.refdef.numDrawSurfs;

			tr.viewCount++;

			float ORIG_RANGE = tr.viewParms.maxEntityRange;

			tr.viewParms.flags |= VPF_SHADOWPASS;
			tr.viewParms.maxEntityRange = shadowParms.maxEntityRange;

			// set viewParms.world
			R_RotateForViewer(&tr.viewParms);

			R_SetupProjectionOrtho(&tr.viewParms, lightviewBounds);

#ifdef __INVERSE_DEPTH_BUFFERS__
			qglClearColor(0, 0, 0, 0);

			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			qglClearDepth(0.0f);
#else //!__INVERSE_DEPTH_BUFFERS__
			qglClearColor(1, 1, 1, 1);

			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			qglClearDepth(1.0f);
#endif //__INVERSE_DEPTH_BUFFERS__

			tr.world = tr.worldSolid;
			R_AddWorldSurfaces();

			if (tr.worldNonSolid)
			{// Set extra world data pointer, and render it...
				tr.world = tr.worldNonSolid;
				R_AddWorldSurfaces();

				// Restore the original pointer to solid world...
				tr.world = tr.worldSolid;
			}

			if (level < 2 || LODMODEL_MAP)
			{
				//R_AddPolygonSurfaces(); // UQ1: Don't really need these on the shadow map...
				R_AddEntitySurfaces();
			}
			else
			{// Still draw any models that are set to ignore culling (event ships, etc)...
				R_AddIgnoreCullEntitySurfaces();
			}

			R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

			tr.viewParms.flags &= ~VPF_SHADOWPASS;
			tr.viewParms.maxEntityRange = ORIG_RANGE;
		}

		Matrix16Multiply(tr.viewParms.projectionMatrix, tr.viewParms.world.modelViewMatrix, tr.refdef.sunShadowMvp[level]);
		tr.refdef.sunShadowCascadeZfar[level] = splitZFar;
	}
}

#ifdef __RENDER_HEIGHTMAP__
void R_RenderHeightMap(void)
{
	//vec3_t mapSize;
	//VectorSubtract(tr.world->bmodels[0].bounds[1], tr.world->bmodels[0].bounds[0], mapSize);
	//mapSize[2] = tr.world->bmodels[0].bounds[1][2];

	//const vec3_t left = { 0.0f,  0.0f, -1.0f };
	//const vec3_t forward = { -1.0f,  0.0f,  0.0f };
	//const vec3_t up = { 0.0f, 1.0f,  0.0f };

	vec3_t viewOrigin;
	//VectorMA(tr.world->bmodels[0].bounds[1], 0.5f, mapSize, viewOrigin);
	VectorSet(viewOrigin, (tr.world->bmodels[0].bounds[0][0] + tr.world->bmodels[0].bounds[1][0]) * 0.5, (tr.world->bmodels[0].bounds[0][1] + tr.world->bmodels[0].bounds[1][1]) * 0.5, tr.world->bmodels[0].bounds[1][2] - 16.0);

	ri->Printf(PRINT_WARNING, "heightmap bounds: %f %f %f x %f %f %f. viewOrigin: %f %f %f.\n"
		, tr.world->bmodels[0].bounds[0][0], tr.world->bmodels[0].bounds[0][1], tr.world->bmodels[0].bounds[0][2]
		, tr.world->bmodels[0].bounds[1][0], tr.world->bmodels[0].bounds[1][1], tr.world->bmodels[0].bounds[1][2]
		, viewOrigin[0], viewOrigin[1], viewOrigin[2]);

	refdef_t refdef;
	viewParms_t	parms;
	float oldColorScale = tr.refdef.colorScale;

	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = 0;
	VectorCopy(viewOrigin, refdef.vieworg);


	//VectorSet(refdef.viewaxis[0], -1, 0, 0);
	//VectorSet(refdef.viewaxis[1], 0, 0, -1);
	//VectorSet(refdef.viewaxis[2], 0, 1, 0);

	VectorSet(refdef.viewaxis[0], 0, 0, -1);
	VectorSet(refdef.viewaxis[1], -1, 0, 0);
	VectorSet(refdef.viewaxis[2], 0, 1, 0);


	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.x = 0;
	refdef.y = 0;
	refdef.width = tr.renderHeightmapFbo->width;
	refdef.height = tr.renderHeightmapFbo->height;

	refdef.time = 0;

	RE_BeginScene(&refdef);

	tr.refdef.colorScale = 1.0f;

#if 0
	Com_Memset(&parms, 0, sizeof(parms));

	parms.viewportX = 0;
	parms.viewportY = 0;
	parms.viewportWidth = tr.renderHeightmapFbo->width;
	parms.viewportHeight = tr.renderHeightmapFbo->height;
	parms.isPortal = qfalse;
	parms.isMirror = qfalse;
	parms.flags = VPF_DEPTHSHADOW | VPF_DEPTHCLAMP | VPF_ORTHOGRAPHIC | VPF_NOVIEWMODEL | VPF_NOPOSTPROCESS | VPF_SHADOWPASS;

	parms.fovX = 90;
	parms.fovY = 90;

	parms.zFar = 9999999.9;

	VectorCopy(refdef.vieworg, parms.ori.origin);
	VectorCopy(refdef.viewaxis[0], parms.ori.axis[0]);
	VectorCopy(refdef.viewaxis[1], parms.ori.axis[1]);
	VectorCopy(refdef.viewaxis[2], parms.ori.axis[2]);

	VectorCopy(refdef.vieworg, parms.pvsOrigin);

	parms.targetFbo = tr.renderHeightmapFbo;

	CURRENT_DRAW_DLIGHTS_UPDATE = qtrue;
	R_RenderView(&parms);
#else
	{
		tr.viewCount++;

		Com_Memset(&parms, 0, sizeof(parms));

		parms.viewportX = 0;
		parms.viewportY = 0;
		parms.viewportWidth = tr.renderHeightmapFbo->width;
		parms.viewportHeight = tr.renderHeightmapFbo->height;
		parms.isPortal = qfalse;
		parms.isMirror = qfalse;
		parms.flags = VPF_DEPTHSHADOW | VPF_DEPTHCLAMP | VPF_ORTHOGRAPHIC | VPF_NOVIEWMODEL | VPF_NOPOSTPROCESS | VPF_SHADOWPASS;

		parms.fovX = 90;
		parms.fovY = 90;

		parms.zFar = 9999999.9;

		VectorCopy(refdef.vieworg, parms.ori.origin);
		VectorCopy(refdef.viewaxis[0], parms.ori.axis[0]);
		VectorCopy(refdef.viewaxis[1], parms.ori.axis[1]);
		VectorCopy(refdef.viewaxis[2], parms.ori.axis[2]);

		VectorCopy(refdef.vieworg, parms.pvsOrigin);

		tr.viewParms = parms;
		tr.viewParms.frameSceneNum = tr.frameSceneNum;
		tr.viewParms.frameCount = tr.frameCount;

		int firstDrawSurf = tr.refdef.numDrawSurfs;

		tr.viewCount++;

		float ORIG_RANGE = tr.viewParms.maxEntityRange;

		tr.viewParms.targetFbo = tr.renderHeightmapFbo;

		tr.viewParms.flags = VPF_DEPTHSHADOW | VPF_DEPTHCLAMP | VPF_ORTHOGRAPHIC | VPF_NOVIEWMODEL | VPF_NOPOSTPROCESS | VPF_SHADOWPASS;
		tr.viewParms.maxEntityRange = 999999.9;

		// set viewParms.world
		R_RotateForViewer(&tr.viewParms);

		R_SetupProjectionOrtho(&tr.viewParms, tr.world->bmodels[0].bounds);

#ifdef __INVERSE_DEPTH_BUFFERS__
		qglClearColor(0, 0, 0, 0);

		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		qglClearDepth(0.0f);
#else //!__INVERSE_DEPTH_BUFFERS__
		qglClearColor(1, 1, 1, 1);

		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		qglClearDepth(1.0f);
#endif //__INVERSE_DEPTH_BUFFERS__

		tr.world = tr.worldSolid;
		R_AddWorldSurfaces();

		if (tr.worldNonSolid)
		{// Set extra world data pointer, and render it...
			tr.world = tr.worldNonSolid;
			R_AddWorldSurfaces();

			// Restore the original pointer to solid world...
			tr.world = tr.worldSolid;
		}

		//R_AddEntitySurfaces();
		//R_AddIgnoreCullEntitySurfaces();

		R_SortDrawSurfs(tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf);

		tr.viewParms.flags &= ~VPF_SHADOWPASS;

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
#endif

	RE_EndScene();
}
#endif //__RENDER_HEIGHTMAP__

#ifndef __REALTIME_CUBEMAP__
void R_RenderCubemapSide( int cubemapIndex, int cubemapSide, qboolean subscene )
{
	refdef_t refdef;
	viewParms_t	parms;
	float oldColorScale = tr.refdef.colorScale;

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = 0;
	VectorCopy(tr.cubemapOrigins[cubemapIndex], refdef.vieworg);

	switch(cubemapSide)
	{
		case 0:
			// -X
			VectorSet( refdef.viewaxis[0], -1,  0,  0);
			VectorSet( refdef.viewaxis[1],  0,  0, -1);
			VectorSet( refdef.viewaxis[2],  0,  1,  0);
			break;
		case 1: 
			// +X
			VectorSet( refdef.viewaxis[0],  1,  0,  0);
			VectorSet( refdef.viewaxis[1],  0,  0,  1);
			VectorSet( refdef.viewaxis[2],  0,  1,  0);
			break;
		case 2: 
			// -Y
			VectorSet( refdef.viewaxis[0],  0, -1,  0);
			VectorSet( refdef.viewaxis[1],  1,  0,  0);
			VectorSet( refdef.viewaxis[2],  0,  0, -1);
			break;
		case 3: 
			// +Y
			VectorSet( refdef.viewaxis[0],  0,  1,  0);
			VectorSet( refdef.viewaxis[1],  1,  0,  0);
			VectorSet( refdef.viewaxis[2],  0,  0,  1);
			break;
		case 4:
			// -Z
			VectorSet( refdef.viewaxis[0],  0,  0, -1);
			VectorSet( refdef.viewaxis[1],  1,  0,  0);
			VectorSet( refdef.viewaxis[2],  0,  1,  0);
			break;
		case 5:
			// +Z
			VectorSet( refdef.viewaxis[0],  0,  0,  1);
			VectorSet( refdef.viewaxis[1], -1,  0,  0);
			VectorSet( refdef.viewaxis[2],  0,  1,  0);
			break;
	}

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.x = 0;
	refdef.y = 0;
	refdef.width = tr.renderCubeFbo->width;
	refdef.height = tr.renderCubeFbo->height;

	refdef.time = 0;

	if (!subscene)
	{
		RE_BeginScene(&refdef);
	}

	tr.refdef.colorScale = 1.0f;

	Com_Memset( &parms, 0, sizeof( parms ) );

	parms.viewportX = 0;
	parms.viewportY = 0;
	parms.viewportWidth = tr.renderCubeFbo->width;
	parms.viewportHeight = tr.renderCubeFbo->height;
	parms.isPortal = qfalse;
	parms.isMirror = qtrue;
	parms.flags =  VPF_NOVIEWMODEL | VPF_NOCUBEMAPS | VPF_NOPOSTPROCESS | VPF_CUBEMAP;

	parms.fovX = 90;
	parms.fovY = 90;

	VectorCopy( refdef.vieworg, parms.ori.origin );
	VectorCopy( refdef.viewaxis[0], parms.ori.axis[0] );
	VectorCopy( refdef.viewaxis[1], parms.ori.axis[1] );
	VectorCopy( refdef.viewaxis[2], parms.ori.axis[2] );

	VectorCopy( refdef.vieworg, parms.pvsOrigin );

	// FIXME: sun shadows aren't rendered correctly in cubemaps
	// fix involves changing r_FBufScale to fit smaller cubemap image size, or rendering cubemap to framebuffer first
	if (0) //(r_depthPrepass->value && ((r_forceSun->integer) || tr.sunShadows))
	{
		parms.flags = VPF_USESUNLIGHT;
	}

	parms.targetFbo = tr.renderCubeFbo;
	parms.targetFboLayer = cubemapSide;
	parms.targetFboCubemapIndex = cubemapIndex;
	
	CURRENT_DRAW_DLIGHTS_UPDATE = qtrue;
	R_RenderView(&parms);

	if (subscene)
	{
		tr.refdef.colorScale = oldColorScale;
	}
	else
	{
		RE_EndScene();
	}
}

#ifdef __EMISSIVE_CUBE_IBL__
void R_RenderEmissiveMapSide(int cubemapIndex, int cubemapSide, qboolean subscene)
{
	refdef_t refdef;
	viewParms_t	parms;
	float oldColorScale = tr.refdef.colorScale;

	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = 0;
	VectorCopy(tr.cubemapOrigins[cubemapIndex], refdef.vieworg);

	switch (cubemapSide)
	{
	case 0:
		// -X
		VectorSet(refdef.viewaxis[0], -1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, -1);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 1:
		// +X
		VectorSet(refdef.viewaxis[0], 1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, 1);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 2:
		// -Y
		VectorSet(refdef.viewaxis[0], 0, -1, 0);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, -1);
		break;
	case 3:
		// +Y
		VectorSet(refdef.viewaxis[0], 0, 1, 0);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, 1);
		break;
	case 4:
		// -Z
		VectorSet(refdef.viewaxis[0], 0, 0, -1);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 5:
		// +Z
		VectorSet(refdef.viewaxis[0], 0, 0, 1);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	}

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.x = 0;
	refdef.y = 0;
	refdef.width = tr.renderCubeFbo->width;
	refdef.height = tr.renderCubeFbo->height;

	refdef.time = 0;

	if (!subscene)
	{
		RE_BeginScene(&refdef);
	}

	tr.refdef.colorScale = 1.0f;

	Com_Memset(&parms, 0, sizeof(parms));

	parms.viewportX = 0;
	parms.viewportY = 0;
	parms.viewportWidth = tr.renderCubeFbo->width;
	parms.viewportHeight = tr.renderCubeFbo->height;
	parms.isPortal = qfalse;
	parms.isMirror = qtrue;
	parms.flags = VPF_NOVIEWMODEL | VPF_NOCUBEMAPS | VPF_NOPOSTPROCESS | VPF_CUBEMAP | VPF_EMISSIVEMAP;

	parms.fovX = 90;
	parms.fovY = 90;

	VectorCopy(refdef.vieworg, parms.ori.origin);
	VectorCopy(refdef.viewaxis[0], parms.ori.axis[0]);
	VectorCopy(refdef.viewaxis[1], parms.ori.axis[1]);
	VectorCopy(refdef.viewaxis[2], parms.ori.axis[2]);

	VectorCopy(refdef.vieworg, parms.pvsOrigin);

	// FIXME: sun shadows aren't rendered correctly in cubemaps
	// fix involves changing r_FBufScale to fit smaller cubemap image size, or rendering cubemap to framebuffer first
	if (0) //(r_depthPrepass->value && ((r_forceSun->integer) || tr.sunShadows))
	{
		parms.flags = VPF_USESUNLIGHT;
	}

	parms.targetFbo = tr.renderCubeFbo;
	parms.targetFboLayer = cubemapSide;
	parms.targetFboCubemapIndex = cubemapIndex;

	CURRENT_DRAW_DLIGHTS_UPDATE = qtrue;
	R_RenderView(&parms);

	if (subscene)
	{
		tr.refdef.colorScale = oldColorScale;
	}
	else
	{
		RE_EndScene();
	}
}
#endif //__EMISSIVE_CUBE_IBL__
#endif //__REALTIME_CUBEMAP__

#ifdef __REALTIME_CUBEMAP__
void R_RenderCubemapSideRealtime(vec3_t origin, int cubemapSide, qboolean subscene)
{
	refdef_t refdef;
	viewParms_t	parms;
	float oldColorScale = tr.refdef.colorScale;

	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = 0;
	VectorCopy(origin, refdef.vieworg);

	switch (cubemapSide)
	{
	case 0:
		// -X
		VectorSet(refdef.viewaxis[0], -1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, -1);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 1:
		// +X
		VectorSet(refdef.viewaxis[0], 1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, 1);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 2:
		// -Y
		VectorSet(refdef.viewaxis[0], 0, -1, 0);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, -1);
		break;
	case 3:
		// +Y
		VectorSet(refdef.viewaxis[0], 0, 1, 0);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, 1);
		break;
	case 4:
		// -Z
		VectorSet(refdef.viewaxis[0], 0, 0, -1);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 5:
		// +Z
		VectorSet(refdef.viewaxis[0], 0, 0, 1);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	}

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.x = 0;
	refdef.y = 0;
	refdef.width = tr.renderCubeFbo->width;
	refdef.height = tr.renderCubeFbo->height;

	refdef.time = 0;

	if (!subscene)
	{
		RE_BeginScene(&refdef);
	}

	tr.refdef.colorScale = 1.0f;

	Com_Memset(&parms, 0, sizeof(parms));

	parms.viewportX = 0;
	parms.viewportY = 0;
	parms.viewportWidth = tr.renderCubeFbo->width;
	parms.viewportHeight = tr.renderCubeFbo->height;
	parms.isPortal = qfalse;
	parms.isMirror = qtrue;
	parms.flags = VPF_NOVIEWMODEL | VPF_NOCUBEMAPS | VPF_NOPOSTPROCESS | VPF_CUBEMAP;

	parms.fovX = 90;
	parms.fovY = 90;

	VectorCopy(refdef.vieworg, parms.ori.origin);
	VectorCopy(refdef.viewaxis[0], parms.ori.axis[0]);
	VectorCopy(refdef.viewaxis[1], parms.ori.axis[1]);
	VectorCopy(refdef.viewaxis[2], parms.ori.axis[2]);

	VectorCopy(refdef.vieworg, parms.pvsOrigin);

	// FIXME: sun shadows aren't rendered correctly in cubemaps
	// fix involves changing r_FBufScale to fit smaller cubemap image size, or rendering cubemap to framebuffer first
	if (0) //(r_depthPrepass->value && ((r_forceSun->integer) || tr.sunShadows))
	{
		parms.flags = VPF_USESUNLIGHT;
	}

	parms.targetFbo = tr.renderCubeFbo;
	parms.targetFboLayer = cubemapSide;
	//parms.targetFboCubemapIndex = cubemapIndex;

	CURRENT_DRAW_DLIGHTS_UPDATE = qtrue;
	R_RenderView(&parms);

	if (subscene)
	{
		tr.refdef.colorScale = oldColorScale;
	}
	else
	{
		RE_EndScene();
	}
}
#endif //__REALTIME_CUBEMAP__
