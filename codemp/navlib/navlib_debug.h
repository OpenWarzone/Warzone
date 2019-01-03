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

#ifndef BOT_DEBUG_H
#define BOT_DEBUG_H

enum class debugDrawMode_t
{
	D_DRAW_POINTS,
	D_DRAW_LINES,
	D_DRAW_TRIS,
	D_DRAW_QUADS
};

struct BotDebugInterface_t
{
	void ( *DebugDrawBegin ) ( debugDrawMode_t mode, float size );
	void ( *DebugDrawDepthMask )( bool state );
	void ( *DebugDrawVertex ) ( const vec3_t pos, unsigned int color,const vec2_t uv );
	void ( *DebugDrawEnd ) ();
};

void     BotDebugDrawMesh(BotDebugInterface_t *in);
#endif
