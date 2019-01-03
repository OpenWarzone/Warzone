/*
MD3 and/or BSP to OBJ converter
Written by Leszek Godlewski <github@inequation.org>
The code in this file, unless otherwise noted, is placed in the public domain.
*/

#pragma once

#include <math.h>
#include <stdlib.h>
#include <minmax.h>

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/Exporter.hpp"	//OO version Header!
#include "assimp/importerdesc.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

#if defined(WIN32) || defined(WIN64)
#include <Windows.h>
#include <wincon.h>

enum concol
{
	concol_black = 0,
	concol_dark_blue = 1,
	concol_dark_green = 2,
	concol_dark_aqua, concol_dark_cyan = 3,
	concol_dark_red = 4,
	concol_dark_purple = 5, concol_dark_pink = 5, concol_dark_magenta = 5,
	concol_dark_yellow = 6,
	concol_dark_white = 7,
	concol_gray = 8, concol_grey = 8,
	concol_blue = 9,
	concol_green = 10,
	concol_aqua = 11, concol_cyan = 11,
	concol_red = 12,
	concol_purple = 13, concol_pink = 13, concol_magenta = 13,
	concol_yellow = 14,
	concol_white = 15
};

#define Q_COLOR_ESCAPE	'^'
#define Q_COLOR_BITS 0xF // was 7

// you MUST have the last bit on here about colour strings being less than 7 or taiwanese strings register as colour!!!!
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE && *((p)+1) <= '9' && *((p)+1) >= '0' )
// Correct version of the above for Q_StripColor
#define Q_IsColorStringExt(p)	((p) && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) >= '0' && *((p)+1) <= '9') // ^[0-9]


#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define COLOR_ORANGE	'8'
#define COLOR_GREY		'9'
#define ColorIndex(c)	( ( (c) - '0' ) & Q_COLOR_BITS )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"
#define S_COLOR_ORANGE	"^8"
#define S_COLOR_GREY	"^9"
#endif

//extern inline void setcolor(concol textcol,concol backcol);
extern inline void setcolor(int textcol, int backcol);
//

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
#define VectorClear(v)				((v)[0] = 0.0f, (v)[1] = 0.0f, (v)[2] = 0.0f)
#define VectorCopy(a, b)			((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
#define VectorAdd(a, b, c)			((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1], (c)[2] = (a)[2] + (b)[2])
#define VectorSubtract(a, b, c)		((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#define VectorScale(a, s, b)		((b)[0] = (a)[0] * (s), (b)[1] = (a)[1] * (s), (b)[2] = (a)[2] * (s))
#define DotProduct(a, b)			((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])
#define CrossProduct(a, b, c)		cross_product(a, b, c)
#define VectorLengthSquared(v)		DotProduct((v), (v))
#define VectorLength(v)				sqrtf(VectorLengthSquared(v))
#define VectorNormalize2(a, b)		normalize_vector(a, b)
#define VectorNormalize(v)			{float length = VectorLength(v); VectorScale(v, length, v);}
#define ClearBounds(a, b)			(VectorClear((a)), VectorClear((b)))
#define AddPointToBounds(p, a, b)	((a)[0] = min((a)[0], (p)[0]), (a)[1] = min((a)[1], (p)[1]), (a)[2] = min((a)[2], (p)[2]), (b)[0] = max((b)[0], (p)[0]), (b)[1] = max((b)[1], (p)[1]), (b)[2] = max((b)[2], (p)[2]))
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];
typedef unsigned char byte;
typedef byte qboolean;
enum
{
	qfalse = 0,
	qtrue = 1
};
#include "qfiles.h"
#include "surfaceflags.h"

#define MAX_GRID_SIZE	129
#define MAX_PATCH_SIZE	32
#define PATCH_STITCHING

extern float normalize_vector(const vec3_t in, vec3_t out);
extern void cross_product(const vec3_t a, const vec3_t b, vec3_t out);

/// BEGIN GPL WOLFENSTEIN: ENEMY TERRITORY CODE
typedef struct cplane_s {
	vec3_t normal;
	float dist;
	byte type;              // for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte signbits;          // signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte pad[2];
} cplane_t;

typedef struct srfGridMesh_s
{
	//surfaceType_t surfaceType;

	// culling information
	vec3_t bounds[ 2 ];
	vec3_t origin;
	float radius;
	cplane_t plane;

	// dynamic lighting information
	//int dlightBits[ SMP_FRAMES ];

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t lodOrigin;
	float lodRadius;
	int lodFixed;
	int lodStitched;

	// vertexes
	int width, height;
	float           *widthLodError;
	float           *heightLodError;
	drawVert_t verts[1];            // variable sized
} srfGridMesh_t;

extern srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
	drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE],
	int r_subdivisions );
/// END GPL WOLFENSTEIN: ENEMY TERRITORY CODE