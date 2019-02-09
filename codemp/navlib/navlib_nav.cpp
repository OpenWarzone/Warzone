/*
===========================================================================
Copyright (C) 2016-2017 Unique One

This file is part of the Warzone navlib source code.

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

#include "navlib_local.h"
#include "server/server.h"

Bot_t agents[MAX_GENTITIES];

extern vec3_t navmeshSize;
extern vec3_t navmeshMins;
extern vec3_t navmeshMaxs;

/*
====================
bot_nav.cpp

All vectors used as inputs and outputs to functions here use the engine's coordinate system
====================
*/

/*
================
ProjectPointOntoVectorBounded
================
*/
void ProjectPointOntoVectorBounded(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj) {
	vec3_t pVec, vec;
	int j;

	VectorSubtract(point, vStart, pVec);
	VectorSubtract(vEnd, vStart, vec);
	VectorNormalize(vec);
	// project onto the directional vector for this segment
	VectorMA(vStart, DotProduct(pVec, vec), vec, vProj);
	// check bounds
	for (j = 0; j < 3; j++)
		if ((vProj[j] > vStart[j] && vProj[j] > vEnd[j]) ||
			(vProj[j] < vStart[j] && vProj[j] < vEnd[j])) {
			break;
		}
	if (j < 3) {
		if (Q_fabs(vProj[j] - vStart[j]) < Q_fabs(vProj[j] - vEnd[j])) {
			VectorCopy(vStart, vProj);
		}
		else {
			VectorCopy(vEnd, vProj);
		}
	}
}


void BotSetPolyFlags( qVec origin, qVec mins, qVec maxs, unsigned short flags )
{
	qVec qExtents;
	qVec realMin, realMax;
	qVec qCenter;

	if ( !numNavData )
	{
		return;
	}

	// support abs min/max by recalculating the origin and min/max
	VectorAdd( mins, origin, realMin );
	VectorAdd( maxs, origin, realMax );
	VectorAdd( realMin, realMax, qCenter );
	VectorScale( qCenter, 0.5, qCenter );
	VectorSubtract( realMax, qCenter, realMax );
	VectorSubtract( realMin, qCenter, realMin );

	// find extents
	for ( int j = 0; j < 3; j++ )
	{
		qExtents[ j ] = max( fabsf( realMin[ j ] ), fabsf( realMax[ j ] ) );
	}

	// convert to recast coordinates
	rVec center = qCenter;
	rVec extents = qExtents;

	// quake2recast conversion makes some components negative
	extents[ 0 ] = fabsf( extents[ 0 ] );
	extents[ 1 ] = fabsf( extents[ 1 ] );
	extents[ 2 ] = fabsf( extents[ 2 ] );

	// setup a filter so our queries include disabled polygons
	dtQueryFilter filter;
	filter.setIncludeFlags( POLYFLAGS_WALK | POLYFLAGS_DISABLED );
	filter.setExcludeFlags( 0 );

	for ( int i = 0; i < numNavData; i++ )
	{
		const int maxPolys = 20;
		int polyCount = 0;
		dtPolyRef polys[ maxPolys ];

		dtNavMeshQuery *query = BotNavData[ i ].query;
		dtNavMesh *mesh = BotNavData[ i ].mesh;

		query->queryPolygons( center, extents, &filter, polys, &polyCount, maxPolys );

		for ( int i = 0; i < polyCount; i++ )
		{
			mesh->setPolyFlags( polys[ i ], flags );
		}
	}
}

void Navlib::NavlibDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_DISABLED );
}

void Navlib::NavlibEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs )
{
	BotSetPolyFlags( origin, mins, maxs, POLYFLAGS_WALK );
}

void Navlib::NavlibSetNavMesh( int npcEntityNum, qhandle_t nav )
{
	if ( nav < 0 || nav >= numNavData )
	{
		trap->Print( "Navigation handle out of bounds\n" );
		return;
	}

	Bot_t *bot = &agents[ npcEntityNum ];

	bot->nav = &BotNavData[ nav ];
	bot->needReplan = true;
}

void GetEntPosition( int num, rVec &pos )
{
	pos = qVec( g_entities[num].r.currentOrigin );
}

void GetEntPosition( int num, qVec &pos )
{
	pos = g_entities[num].r.currentOrigin;
}

bool Navlib::NavlibFindRoute( int npcEntityNum, const navlibRouteTarget_t *target, bool allowPartial )
{
	rVec start;
	Bot_t *bot = &agents[ npcEntityNum ];

	GetEntPosition( npcEntityNum, start );
	bool result = FindRoute( bot, start, *target, allowPartial );
	return static_cast<bool>( result );
}

static bool withinRadiusOfOffMeshConnection( const Bot_t *bot, rVec pos, rVec off, dtPolyRef conPoly )
{
	const dtOffMeshConnection *con = bot->nav->mesh->getOffMeshConnectionByRef( conPoly );
	if ( !con )
	{
		return false;
	}

	if ( dtVdist2DSqr( pos, off ) < con->rad * con->rad )
	{
		return true;
	}

	return false;
}

static bool overOffMeshConnectionStart( const Bot_t *bot, rVec pos )
{
	int corner = bot->numCorners - 1;
	const bool offMeshConnection = ( bot->cornerFlags[ corner ] & DT_STRAIGHTPATH_OFFMESH_CONNECTION ) ? true : false;

	if ( offMeshConnection )
	{
		return withinRadiusOfOffMeshConnection( bot, pos, &bot->cornerVerts[ corner * 3 ], bot->cornerPolys[ corner ] );
	}
	return false;
}

void UpdatePathCorridor( Bot_t *bot, rVec spos, botRouteTargetInternal target )
{
	bot->corridor.movePosition( spos, bot->nav->query, &bot->nav->filter );

	if ( target.type == navlibRouteTargetType_t::BOT_TARGET_DYNAMIC )
	{
		bot->corridor.moveTargetPosition( target.pos, bot->nav->query, &bot->nav->filter );
	}

	if ( !bot->corridor.isValid( MAX_PATH_LOOKAHEAD, bot->nav->query, &bot->nav->filter ) )
	{
		bot->corridor.trimInvalidPath( bot->corridor.getFirstPoly(), spos, bot->nav->query, &bot->nav->filter );
		bot->needReplan = true;
	}

	FindWaypoints( bot, bot->cornerVerts, bot->cornerFlags, bot->cornerPolys, &bot->numCorners, MAX_CORNERS );
}

void Navlib::NavlibUpdateCorridor( int npcEntityNum, const navlibRouteTarget_t *target, navlibNavCmd_t *cmd )
{
	rVec spos;
	rVec epos;
	Bot_t *bot = &agents[ npcEntityNum ];
	botRouteTargetInternal rtarget;

	if ( !cmd || !target )
	{
		return;
	}

	GetEntPosition( npcEntityNum, spos );

	rtarget = *target;
	epos = rtarget.pos;

	UpdatePathCorridor( bot, spos, rtarget );

	if ( !bot->offMesh )
	{
		if ( bot->needReplan )
		{
			if ( FindRoute( bot, spos, rtarget, false ) )
			{
				bot->needReplan = false;
			}
		}

		cmd->havePath = !bot->needReplan;

		if ( overOffMeshConnectionStart( bot, spos ) )
		{
			dtPolyRef refs[ 2 ];
			rVec start;
			rVec end;
			int corner = bot->numCorners - 1;
			dtPolyRef con = bot->cornerPolys[ corner ];

			if ( bot->corridor.moveOverOffmeshConnection( con, refs, start, end, bot->nav->query ) )
			{
				bot->offMesh = true;
				bot->offMeshPoly = con;
				bot->offMeshEnd = end;
				bot->offMeshStart = start;
			}
		}

		dtPolyRef firstPoly = bot->corridor.getFirstPoly();
		dtPolyRef lastPoly = bot->corridor.getLastPoly();

		if ( !PointInPoly( bot, firstPoly, spos ) )
		{
			bot->needReplan = true;
		}

		if ( rtarget.type == navlibRouteTargetType_t::BOT_TARGET_DYNAMIC )
		{
			if ( !PointInPolyExtents( bot, lastPoly, epos, rtarget.polyExtents ) )
			{
				bot->needReplan = true;
			}
		}

		rVec rdir;
		BotCalcSteerDir( bot, rdir );

		VectorCopy( rdir, cmd->dir );
		recast2quake( cmd->dir );

		cmd->directPathToGoal = bot->numCorners <= 1;

		VectorCopy( bot->corridor.getPos(), cmd->pos );
		recast2quake( cmd->pos );

		// if there are no corners, we have reached the goal
		// FIXME: this must be done because of a weird bug where the target is not reachable even if 
		// the path was checked for a partial path beforehand
		if ( bot->numCorners == 0 )
		{
			VectorCopy( cmd->pos, cmd->tpos );
		}
		else
		{
			VectorCopy( bot->corridor.getTarget(), cmd->tpos );

			float height;
			if ( dtStatusSucceed( bot->nav->query->getPolyHeight( bot->corridor.getLastPoly(), cmd->tpos, &height ) ) )
			{
				cmd->tpos[ 1 ] = height;
			}
			recast2quake( cmd->tpos );
		}
	}
	
	if ( bot->offMesh )
	{
		qVec pos, proj;
		qVec start = bot->offMeshStart;
		qVec end = bot->offMeshEnd;
		qVec dir;
		qVec pVec;

		GetEntPosition( npcEntityNum, pos );
		start[ 2 ] = pos[ 2 ];
		end[ 2 ] = pos[ 2 ];

		ProjectPointOntoVectorBounded( pos, start, end, proj );
		
		VectorCopy( proj, cmd->pos );
		cmd->directPathToGoal = false;
		VectorSubtract( end, pos, cmd->dir );
		VectorNormalize( cmd->dir );

		VectorCopy( bot->corridor.getTarget(), cmd->tpos );
		float height;
		if ( dtStatusSucceed( bot->nav->query->getPolyHeight( bot->corridor.getLastPoly(), cmd->tpos, &height ) ) )
		{
			cmd->tpos[ 1 ] = height;
		}
		recast2quake( cmd->tpos );

		cmd->havePath = true;

		if ( withinRadiusOfOffMeshConnection( bot, spos, bot->offMeshEnd, bot->offMeshPoly ) )
		{
			bot->offMesh = false;
		}
	}
}

float frand()
{
	//return ( float ) rand() / ( float ) RAND_MAX;
	return random();
	//return rand() / (RAND_MAX + 1.0);
}

void GetRandomOrgWithinMapBounds(vec3_t &point)
{
	int x = navmeshMins[0] + irand_big(0, navmeshSize[0]);
	int y = navmeshMins[1] + irand_big(0, navmeshSize[1]);
	int z = (navmeshMins[2] + navmeshSize[2]) * 0.5;// navmeshMins[2] + irand_big(0, navmeshSize[2]);
	VectorSet(point, (float)x, (float)y, (float)z);

	//trap->Print("0. mins: %f %f %f.\n", navmeshMins[0], navmeshMins[1], navmeshMins[2]);
	//trap->Print("0. maxs: %f %f %f.\n", navmeshMaxs[0], navmeshMaxs[1], navmeshMaxs[2]);
	//trap->Print("0. size: %f %f %f.\n", navmeshSize[0], navmeshSize[1], navmeshSize[2]);
	//trap->Print("0. xyz: %f %f %f.\n", point[0], point[1], point[2]);
}

void Navlib::NavlibFindRandomPoint( int npcEntityNum, vec3_t point )
{
	if (npcEntityNum < 0)
	{
		vec3_t origin;
		GetRandomOrgWithinMapBounds(origin);
		//VectorClear(origin);

#if 1
		if (Navlib::NavlibFindRandomPointInRadius(npcEntityNum, origin, point, 524288.0f))
		{
			return;
		}

		VectorCopy(origin, point);
		return;
#else
		rVec nearPoint;
		dtPolyRef nearPoly = 0;

		VectorSet(point, 0, 0, 0);

		dtNavMeshQuery* query = dtAllocNavMeshQuery();
		dtQueryFilter filter;
		memset(&filter, 0, sizeof(filter));

		query->init(BotNavData[0].mesh, 2048);

		dtStatus status;

		rVec start(origin);
		//quake2recast(start);
		rVec extents(524288.0f, 524288.0f, 524288.0f);

		filter.setIncludeFlags(POLYFLAGS_WALK | POLYFLAGS_DISABLED);
		filter.setExcludeFlags(POLYFLAGS_SWIM);

		status = query->findNearestPoly(start, extents, &filter, &nearPoly, nearPoint);

		if (dtStatusFailed(status) || nearPoly == 0)
		{
			dtFreeNavMeshQuery(query);
			return; // failed
		}

		dtFreeNavMeshQuery(query);

		VectorCopy(nearPoint, point);
		recast2quake(point);
		return;
#endif
	}
	else
	{
		vec3_t origin;
		qVec curOrigin = g_entities[npcEntityNum].r.currentOrigin;
		VectorCopy(curOrigin, origin);
		quake2recast(origin);

		if (!Navlib::NavlibFindRandomPointInRadius(npcEntityNum, origin, point, 524288.0f))
		{
			VectorCopy(g_entities[npcEntityNum].r.currentOrigin, point);
		}
	}
}

void Navlib::NavlibFindRandomPatrolPoint(int npcEntityNum, vec3_t point)
{
	if (npcEntityNum < 0)
	{
		trap->Print("NavLib Warning: NavlibFindRandomPatrolPoint called with npcEntityNum < 0. Returning zero vector.\n");
		VectorClear(point);
		return;
	}

	vec3_t negMult, origin;
	qVec curOrigin = g_entities[npcEntityNum].r.currentOrigin;
	
	VectorSet(negMult, random() > 0.5f ? 1.0f : -1.0f, random() > 0.5f ? 1.0f : -1.0f, 0.0);
	VectorSet(origin, curOrigin[0] + (1024.0f * random() * negMult[0]), curOrigin[1] + (1024.0f * random() * negMult[1]), curOrigin[2]);
	quake2recast(origin);

	if (!Navlib::NavlibFindRandomPointInRadius(npcEntityNum, origin, point, 1024.0f))
	{
		VectorCopy(g_entities[npcEntityNum].r.currentOrigin, point);
	}
}

bool Navlib::NavlibFindRandomPointInRadius( int npcEntityNum, const vec3_t origin, vec3_t point, float radius )
{
	rVec nearPoint;
	dtPolyRef nearPoly = NULL;

	VectorSet( point, 0, 0, 0 );

	if (npcEntityNum < 0)
	{// No NPC specified, generate a query based at origin...
		//trap->Print("1. in org: %f %f %f.\n", origin[0], origin[1], origin[2]);

		rVec rorigin = qVec(origin);
		//quake2recast(rorigin);

		dtNavMeshQuery* query = dtAllocNavMeshQuery();
		dtQueryFilter filter;
		memset(&filter, 0, sizeof(filter));
		
		query->init(BotNavData[0].mesh, 2048);

		dtStatus status;

		rVec start(origin);
		//quake2recast(start);

		//trap->Print("2. start: %f %f %f.\n", start[0], start[1], start[2]);

		rVec extents(radius, radius, radius);

		filter.setIncludeFlags(POLYFLAGS_WALK | POLYFLAGS_DISABLED);
		filter.setExcludeFlags(POLYFLAGS_SWIM);

		nearPoint[0] = 0;
		nearPoint[1] = 0;
		nearPoint[2] = 0;

		status = query->findNearestPoly(start, extents, &filter, &nearPoly, nearPoint);

		if (dtStatusFailed(status) || nearPoly == NULL)
		{
			dtFreeNavMeshQuery(query);
			return false; // failed
		}

		//trap->Print("3. nearPoint: %f %f %f.\n", nearPoint[0], nearPoint[1], nearPoint[2]);

		dtPolyRef randRef;
		status = query->findRandomPointAroundCircle(nearPoly, rorigin, radius, &filter, frand, &randRef, nearPoint);

		//trap->Print("3. nearPoint2: %f %f %f.\n", nearPoint[0], nearPoint[1], nearPoint[2]);

		if (dtStatusFailed(status))
		{
			dtFreeNavMeshQuery(query);
			return false;
		}

		dtFreeNavMeshQuery(query);
	}
	else
	{
		rVec rorigin = qVec(origin);
		Bot_t *bot = &agents[npcEntityNum];

		if (!BotFindNearestPoly(bot, rorigin, &nearPoly, nearPoint))
		{
			return false;
		}

		dtPolyRef randRef;
		dtStatus status = bot->nav->query->findRandomPointAroundCircle(nearPoly, rorigin, radius, &bot->nav->filter, frand, &randRef, nearPoint);

		if (dtStatusFailed(status))
		{
			return false;
		}
	}

	VectorCopy( nearPoint, point );
	recast2quake( point );
	return true;
}

bool Navlib::NavlibNavTrace(navlibTrace_t *trace, vec3_t start, vec3_t end, int npcEntityNum)
{
	dtPolyRef startRef;
	dtStatus status;
	rVec extents( 75, 96, 75 );
	rVec spos = qVec( start );
	rVec epos = qVec( end );

	memset( trace, 0, sizeof( *trace ) );

	Bot_t *bot = &agents[ npcEntityNum ];

	status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, nullptr );
	if ( dtStatusFailed( status ) || startRef == 0 )
	{
		//try larger extents
		extents[ 1 ] += 500;
		status = bot->nav->query->findNearestPoly( spos, extents, &bot->nav->filter, &startRef, nullptr );
		if ( dtStatusFailed( status ) || startRef == 0 )
		{
			return false;
		}
	}

	status = bot->nav->query->raycast( startRef, spos, epos, &bot->nav->filter, &trace->frac, trace->normal, nullptr, nullptr, 0 );
	if ( dtStatusFailed( status ) )
	{
		return false;
	}

	recast2quake( trace->normal );
	return true;
}

void Navlib::NavlibAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *obstacleHandle )
{
	qVec min = mins;
	qVec max = maxs;
	rBounds box( min, max );

	for ( int i = 0; i < numNavData; i++ )
	{
		dtObstacleRef ref;
		NavData_t *nav = &BotNavData[ i ];

		const dtTileCacheParams *params = nav->cache->getParams();
		float offset = params->walkableRadius;

		rBounds tempBox = box;

		// offset bbox by agent radius like the navigation mesh was originally made
		tempBox.mins[ 0 ] -= offset;
		tempBox.mins[ 2 ] -= offset;

		tempBox.maxs[ 0 ] += offset;
		tempBox.maxs[ 2 ] += offset;
		
		// offset mins down by agent height so obstacles placed on ledges are handled correctly
		tempBox.mins[ 1 ] -= params->walkableHeight;
		
		nav->cache->addBoxObstacle( tempBox.mins, tempBox.maxs, &ref );
		*obstacleHandle = ref;
	}
}

void Navlib::NavlibRemoveObstacle( qhandle_t obstacleHandle )
{
	for ( int i = 0; i < numNavData; i++ )
	{
		NavData_t *nav = &BotNavData[ i ];
		if ( nav->cache->getObstacleCount() <= 0 )
		{
			continue;
		}
		nav->cache->removeObstacle( obstacleHandle );
	}
}

void Navlib::NavlibUpdateObstacles( void )
{
	for ( int i = 0; i < numNavData; i++ )
	{
		NavData_t *nav = &BotNavData[ i ];
		nav->cache->update( 0, nav->mesh );
	}
}
