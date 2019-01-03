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

#include "DetourDebugDraw.h"
#include "DebugDraw.h"
#include "navlib_debug.h"
#include "navlib_local.h"

class DebugDrawQuake : public duDebugDraw
{
	BotDebugInterface_t *re;
public:
	void init(BotDebugInterface_t *in);
	void depthMask(bool state);
	void texture(bool) {};
	void begin(duDebugDrawPrimitives prim, float size = 1.0f);
	void vertex(const float* pos, unsigned int color);
	void vertex(const float x, const float y, const float z, unsigned int color);
	void vertex(const float* pos, unsigned int color, const float* uv);
	void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
	void end();
};

void BotDrawNavEdit( NavData_t *nav, DebugDrawQuake *dd );
