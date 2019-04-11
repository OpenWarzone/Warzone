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

#ifndef __BOT_VEC_H
#define __BOT_VEC_H

#include "navlib_api.h"

extern float navmeshScale;
extern float navmeshScaleInv;

//coordinate conversion
static inline void quake2recast( float vec[ 3 ] )
{
	float temp = vec[1];
	vec[1] = vec[2];
	vec[2] = temp;

	vec[0] *= navmeshScale;
	vec[1] *= navmeshScale;
	vec[2] *= navmeshScale;
}

static inline void recast2quake( float vec[ 3 ] )
{
	float temp = vec[1];
	vec[1] = vec[2];
	vec[2] = temp;

	vec[0] *= navmeshScaleInv;
	vec[1] *= navmeshScaleInv;
	vec[2] *= navmeshScaleInv;
}

class rVec;
class qVec
{
	float v[ 3 ];

public:
	qVec( ) { }
	qVec( float x, float y, float z );
	qVec( const float vec[ 3 ] );
	qVec( rVec vec );

	inline float &operator[]( int i )
	{
		return v[ i ];
	}

	inline operator const float*() const
	{
		return v;
	}

	inline operator float*()
	{
		return v;
	}
};

class rVec
{
	float v[ 3 ];
public:
	rVec() { }
	rVec( float x, float y, float z );
	rVec( const float vec[ 3 ] );
	rVec( qVec vec );

	inline float &operator[]( int i )
	{
		return v[ i ];
	}
	
	inline operator const float*() const
	{
		return v;
	}

	inline operator float*()
	{
		return v;
	}
};

class rBounds
{
public:
	rVec mins, maxs;

	rBounds()
	{
		clear();
	}

	rBounds( const rBounds &b )
	{
		mins = b.mins;
		maxs = b.maxs;
	}

	rBounds( qVec min, qVec max );
	rBounds( const float min[ 3 ], const float max[ 3 ] );

	void addPoint( rVec p );
	void clear();
};

struct botRouteTargetInternal
{
	navlibRouteTargetType_t type;
	rVec pos;
	rVec polyExtents;

	botRouteTargetInternal() { }
	botRouteTargetInternal( const navlibRouteTarget_t &target );
};

#endif