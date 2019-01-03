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

#include "client/client.h"
#include "DetourDebugDraw.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include "DebugDraw.h"
#pragma GCC diagnostic pop
#include "navlib_local.h"
#include "nav.h"
#include "navlib_debug.h"
#include "navlib_navdraw.h"

void DebugDrawQuake::init(BotDebugInterface_t *ref)
{
	re = ref;
}

void DebugDrawQuake::depthMask(bool state)
{
	re->DebugDrawDepthMask( ( bool ) ( int ) state );
}

void DebugDrawQuake::begin(duDebugDrawPrimitives prim, float s)
{
	re->DebugDrawBegin( ( debugDrawMode_t ) prim, s );
}

void DebugDrawQuake::vertex(const float* pos, unsigned int c)
{
	vertex( pos, c, nullptr );
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color)
{
	vec3_t vert;
	VectorSet( vert, x, y, z );
	recast2quake( vert );
	re->DebugDrawVertex( vert, color, nullptr );
}

void DebugDrawQuake::vertex(const float *pos, unsigned int color, const float* uv)
{
	vec3_t vert;
	VectorCopy( pos, vert );
	recast2quake( vert );
	re->DebugDrawVertex( vert, color, uv );
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	vec3_t vert;
	vec2_t uv;
	uv[0] = u;
	uv[1] = v;
	VectorSet( vert, x, y, z );
	recast2quake( vert );
	re->DebugDrawVertex( vert, color, uv );
}

void DebugDrawQuake::end()
{
	re->DebugDrawEnd();
}
