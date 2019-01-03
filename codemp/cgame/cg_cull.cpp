#include "cg_local.h"

//
// UQ1: New distance and FOV checking on common stuff to make sure we never render stuff we don't have to...
//

extern qboolean CG_InFOV( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );

qboolean CullVisible ( vec3_t org1, vec3_t org2, int ignore )
{
	trace_t tr;
	vec3_t orgA, orgB;

	VectorCopy(org1, orgA);
	orgA[2]+=16;
	VectorCopy(org2, orgB);
	orgB[2]+=16;

	CG_Trace( &tr, orgA, NULL, NULL, orgB, ignore, CONTENTS_OPAQUE );

	if ( tr.fraction == 1 )
	{// Completely visible!
		return ( qtrue );
	}
	else if (tr.fraction > 0.8 || Distance(tr.endpos, org2) < 96)//256)
	{// Close enough!
		return ( qtrue );
	}

	VectorCopy(org1, orgA);
	orgA[2]+=8;
	VectorCopy(org2, orgB);

	CG_Trace( &tr, orgA, NULL, NULL, orgB, ignore, CONTENTS_OPAQUE );

	if ( tr.fraction == 1 )
	{// Completely visible!
		return ( qtrue );
	}
	else if (tr.fraction > 0.8 || Distance(tr.endpos, org2) < 96)//256)
	{// Close enough!
		return ( qtrue );
	}

	return ( qfalse );
}

qboolean ShouldCull ( vec3_t org, qboolean check_angles )
{
	if (cg_cull.integer)
	{
		float dist = Distance(cg.refdef.vieworg, org);

		if (dist > 3072.0) return qtrue; // TOO FAR! CULLED!
		if (check_angles && dist > 256 && !CG_InFOV( org, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x*1.2, cg.refdef.fov_y*1.2)) return qtrue; // NOT ON SCREEN! CULLED!
		//if (/*dist > 256 &&*/ !CullVisible(cg.refdef.vieworg, org, cg.clientNum)) return qtrue; // NOT VISIBLE TO US! CULLED!
	}

	return qfalse;
}

void AddRefEntityToScene ( refEntity_t *ent )
{
	//if (!ent->ignoreCull && ShouldCull(ent->origin, qtrue)) return;

	trap->R_AddRefEntityToScene( ent );
}

void PlayEffectID ( int id, vec3_t org, vec3_t fwd, int vol, int rad, qboolean isPortal )
{
	if (!isPortal && ShouldCull(org, qtrue)) return;

	trap->FX_PlayEffectID( id, org, fwd, vol, rad, isPortal );
}

void AddLightToScene ( const vec3_t org, float intensity, float r, float g, float b )
{
	if (ShouldCull((float *)org, qfalse)) return;

	trap->R_AddLightToScene(org, intensity, r, g, b);
}
