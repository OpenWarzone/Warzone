#include "snd_local.h"
#include "client.h"

extern vec3_t		s_entityPosition[MAX_GENTITIES];

extern void SV_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule, int traceFlags, int useLod );

qboolean CullVisible ( vec3_t org1, vec3_t org2, int ignore )
{
	trace_t tr;
	vec3_t orgA, orgB;

	VectorCopy(org1, orgA);
	orgA[2]+=16;
	VectorCopy(org2, orgB);
	orgB[2]+=16;

	// UQ1: MASK_SOLID because even glass hits should lower volume...
	SV_Trace( &tr, orgA, NULL, NULL, orgB, ignore, MASK_SOLID/*CONTENTS_OPAQUE*/, qfalse, 0, 0 );

	if ( tr.fraction == 1 )
	{// Completely visible!
		return ( qtrue );
	}
	else if (/*tr.fraction > 0.8 ||*/ Distance(tr.endpos, org2) < 96)//256)
	{// Close enough!
		return ( qtrue );
	}

	VectorCopy(org1, orgA);
	orgA[2]+=8;
	VectorCopy(org2, orgB);

	SV_Trace( &tr, orgA, NULL, NULL, orgB, ignore, MASK_SOLID/*CONTENTS_OPAQUE*/, qfalse, 0, 0 );

	if ( tr.fraction == 1 )
	{// Completely visible!
		return ( qtrue );
	}
	else if (/*tr.fraction > 0.8 ||*/ Distance(tr.endpos, org2) < 96)//256)
	{// Close enough!
		return ( qtrue );
	}

	return ( qfalse );
}

qboolean S_InFOV( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV )
{
	vec3_t	deltaVector, angles, deltaAngles;
	vec3_t	fromAnglesCopy;

	VectorSubtract ( spot, from, deltaVector );
	vectoangles ( deltaVector, angles );
	VectorCopy(fromAngles, fromAnglesCopy);
	
	deltaAngles[PITCH]	= AngleDelta ( fromAnglesCopy[PITCH], angles[PITCH] );
	deltaAngles[YAW]	= AngleDelta ( fromAnglesCopy[YAW], angles[YAW] );

	if ( fabs ( deltaAngles[PITCH] ) <= vFOV && fabs ( deltaAngles[YAW] ) <= hFOV ) 
	{
		return qtrue;
	}

	return qfalse;
}

qboolean S_ShouldCull ( vec3_t org, qboolean check_angles, int entityNum )
{// This checks if the sound is behind a wall or something. When it is, the sound needs to be played at lower volume...
	vec3_t checkOrg;

	if (!s_realism->integer) return qfalse;

	if (!org)
	{
		if (entityNum >= 0)
		{// Use entity position...
			if (!(s_entityPosition[entityNum][0] == 0 && s_entityPosition[entityNum][1] == 0 && s_entityPosition[entityNum][2] == 0))
			{// We have a s_entityPosition. Use it...
				VectorCopy(s_entityPosition[entityNum], checkOrg);
			}
			else
			{// Just try to use entityBaseline...
				VectorCopy(cl.entityBaselines[entityNum].origin, checkOrg);
			}
		}
		else
		{// Local sound...
			return qfalse;
		}
	}
	else
	{// We have an origin, use it...
		VectorCopy(org, checkOrg);
	}

	if (checkOrg[0] == 0 && checkOrg[1] == 0 && checkOrg[2] == 0)
	{// Local sound...
		return qfalse;
	}

	float dist = Distance(cl.snap.ps.origin, checkOrg);

	if (dist > DEFAULT_ENTITY_CULL_RANGE/*3072.0*/) return qtrue; // TOO FAR! CULLED!
	if (s_realism->integer >= 2 && check_angles && !S_InFOV(checkOrg, cl.snap.ps.origin, cl.snap.ps.viewangles, 180.0, 180.0)) return qtrue; // NOT ON SCREEN! CULLED!
	//if (s_realism->integer >= 3 && !CullVisible(cl.snap.ps.origin, checkOrg, cl.snap.ps.clientNum)) return qtrue; // NOT VISIBLE TO US! CULLED!

	return qfalse;
}
