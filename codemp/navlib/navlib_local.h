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

#ifndef __BOT_LOCAL_H
#define __BOT_LOCAL_H

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourPathCorridor.h"
#include "DetourCommon.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"

#include "Compiler.h"
#include "navlib_api.h"
#include "navlib_api.h"
#include "nav.h"
#include "navlib_convert.h"

#include <stddef.h>


const int MAX_NAV_DATA = 16;
const int MAX_BOT_PATH = 512;
const int MAX_PATH_LOOKAHEAD = 5;
const int MAX_CORNERS = 5;
const int MAX_ROUTE_PLANS = 2;
const int MAX_ROUTE_CACHE = 20;
const int ROUTE_CACHE_TIME = 200;

struct dtRouteResult
{
	dtPolyRef startRef;
	dtPolyRef endRef;
	int       time;
	dtStatus  status;
	bool      invalid;
};

struct NavData_t
{
	dtTileCache      *cache;
	dtNavMesh        *mesh;
	dtNavMeshQuery   *query;
	dtQueryFilter    filter;
	MeshProcess      process;
	char             name[ 64 ];
};

struct Bot_t
{
	NavData_t         *nav;
	dtPathCorridor    corridor;
	int               clientNum;
	bool              needReplan;
	float             cornerVerts[ MAX_CORNERS * 3 ];
	unsigned char     cornerFlags[ MAX_CORNERS ];
	dtPolyRef         cornerPolys[ MAX_CORNERS ];
	int               numCorners;
	bool              offMesh;
	rVec              offMeshStart;
	rVec              offMeshEnd;
	dtPolyRef         offMeshPoly;
	dtRouteResult     routeResults[ MAX_ROUTE_CACHE ];
};

extern int numNavData;
extern NavData_t BotNavData[ MAX_NAV_DATA ];
extern Bot_t agents[MAX_GENTITIES];

void NavEditInit();
void NavEditShutdown();
void BotSaveOffMeshConnections( NavData_t *nav );

void         BotCalcSteerDir( Bot_t *bot, rVec &dir );
void         FindWaypoints( Bot_t *bot, float *corners, unsigned char *cornerFlags, dtPolyRef *cornerPolys, int *numCorners, int maxCorners );
bool         PointInPolyExtents( Bot_t *bot, dtPolyRef ref, rVec point, rVec extents );
bool         PointInPoly( Bot_t *bot, dtPolyRef ref, rVec point );
bool         BotFindNearestPoly( Bot_t *bot, rVec coord, dtPolyRef *nearestPoly, rVec &nearPoint );
bool         FindRoute( Bot_t *bot, rVec s, botRouteTargetInternal target, bool allowPartial );
#endif
