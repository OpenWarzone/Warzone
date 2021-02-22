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
#include "tr_local.h"

// tr_shader.c -- this file deals with the parsing and definition of shaders

//#define __DEBUG_SHADER_LOADING__

static char *s_shaderText;

extern qboolean DISABLE_MERGED_GLOWS;

#define GLSL_BLEND_ALPHA			0
#define GLSL_BLEND_INVALPHA			1
#define GLSL_BLEND_DST_ALPHA		2
#define GLSL_BLEND_INV_DST_ALPHA	3
#define GLSL_BLEND_GLOWCOLOR		4
#define GLSL_BLEND_INV_GLOWCOLOR	5
#define GLSL_BLEND_DSTCOLOR			6
#define GLSL_BLEND_INV_DSTCOLOR		7

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define RETAIL_ROCKET_WEDGE_SHADER_HASH (1217042)
#define FILE_HASH_SIZE		1024
static	shader_t*		hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH		2048
static char **shaderTextHashTable[MAX_SHADERTEXT_HASH];

const int lightmapsNone[MAXLIGHTMAPS] =
{
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE
};

const int lightmaps2d[MAXLIGHTMAPS] =
{
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D
};

const int lightmapsVertex[MAXLIGHTMAPS] =
{
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX
};

const int lightmapsFullBright[MAXLIGHTMAPS] =
{
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE
};

const byte stylesDefault[MAXLIGHTMAPS] =
{
	LS_NORMAL,
	LS_LSNONE,
	LS_LSNONE,
	LS_LSNONE
};

qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndexes, const byte *styles );

void KillTheShaderHashTable(void)
{
	memset(shaderTextHashTable, 0, sizeof(shaderTextHashTable));
}

qboolean ShaderHashTableExists(void)
{
	if (shaderTextHashTable[0])
	{
		return qtrue;
	}
	return qfalse;
}

static void ClearGlobalShader(void)
{
	int	i;

	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
		//stages[i].mGLFogColorOverride = GLFOGOVERRIDE_NONE;

		// default normal/specular
		VectorSet4(stages[i].normalScale, 0.0f, 0.0f, 0.0f, 0.0f);
		VectorSet4(stages[i].specularScale, 0.0f, 0.0f, 0.0f, 0.0f); // UQ1: if not set, will fall back to material type defaults instead of cvars...
	}

	shader.contentFlags = CONTENTS_SOLID | CONTENTS_OPAQUE;
}

static uint32_t generateHashValueForText(const char *string, size_t length)
{
	int i = 0;
	uint32_t hash = 0;

	while (length--)
	{
		hash += string[i] * (i + 119);
		i++;
	}

	return (hash ^ (hash >> 10) ^ (hash >> 20));
}



/*
================
return a hash value for the filename
================
*/
#ifdef __GNUCC__
  #warning TODO: check if long is ok here
#endif
static long generateHashValue( const char *fname, const int size ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size-1);
	return hash;
}

void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset) {
	char		strippedName[MAX_IMAGE_PATH];
	int			hash;
	shader_t	*sh, *sh2;
	qhandle_t	h;

	sh = R_FindShaderByName( shaderName );
	if (sh == NULL || sh == tr.defaultShader) {
		h = RE_RegisterShaderLightMap (shaderName, lightmapsNone, stylesDefault);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader) {
		ri->Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if (sh2 == NULL || sh2 == tr.defaultShader) {
		h = RE_RegisterShaderLightMap (newShaderName, lightmapsNone, stylesDefault);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader) {
		ri->Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName, FILE_HASH_SIZE);
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		if (Q_stricmp(sh->name, strippedName) == 0) {
			if (sh != sh2) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if (timeOffset) {
		sh2->timeOffset = atof(timeOffset);
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( const char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri->Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri->Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri->Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "GT0" ) )
	{
		return GLS_ATEST_GT_0;
	}
	else if ( !Q_stricmp( funcname, "LT128" ) )
	{
		return GLS_ATEST_LT_128;
	}
	else if ( !Q_stricmp( funcname, "GE128" ) )
	{
		return GLS_ATEST_GE_128;
	}
	else if ( !Q_stricmp( funcname, "GE192" ) )
	{
		return GLS_ATEST_GE_192;
	}

	ri->Printf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_SRCBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ONE;
		}

		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ZERO;
		}

		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	ri->Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_DSTBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ONE;
		}

		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		if ( r_ignoreDstAlpha->integer )
		{
			return GLS_DSTBLEND_ZERO;
		}

		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	ri->Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "sin" ) )
	{
		return GF_SIN;
	}
	else if ( !Q_stricmp( funcname, "square" ) )
	{
		return GF_SQUARE;
	}
	else if ( !Q_stricmp( funcname, "triangle" ) )
	{
		return GF_TRIANGLE;
	}
	else if ( !Q_stricmp( funcname, "sawtooth" ) )
	{
		return GF_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "inversesawtooth" ) )
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "noise" ) )
	{
		return GF_NOISE;
	}
	else if ( !Q_stricmp( funcname, "random" ) )
	{
		return GF_RAND;
	}

	ri->Printf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( const char **text, waveForm_t *wave )
{
	char *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( const char *_text, shaderStage_t *stage )
{
	const char *token;
	const char **text = &_text;
	texModInfo_t *tmi;

	if ( stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		ri->Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt( text, qfalse );

	//
	// turb
	//
	if ( !Q_stricmp( token, "turb" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !Q_stricmp( token, "scale" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = atof( token );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !Q_stricmp( token, "scroll" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = atof( token );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !Q_stricmp( token, "stretch" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if ( !Q_stricmp( token, "transform" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !Q_stricmp( token, "rotate" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = atof( token );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !Q_stricmp( token, "entityTranslate" ) )
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		ri->Printf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
	}
}

static animMapType_t AnimMapType( const char *token )
{
	if ( !Q_stricmp( token, "clampanimMap" ) ) { return ANIMMAP_CLAMP; }
	else if ( !Q_stricmp( token, "oneshotanimMap" ) ) { return ANIMMAP_ONESHOT; }
	else { return ANIMMAP_NORMAL; }
}

static const char *animMapNames[] = {
	"animMap",
	"clapanimMap",
	"oneshotanimMap"
};

/*
/////===== Part of the VERTIGON system =====/////
===================
ParseSurfaceSprites
===================
*/
// surfaceSprites <type> <width> <height> <density> <fadedist>
//
// NOTE:  This parsing function used to be 12 pages long and very complex.  The new version of surfacesprites
// utilizes optional parameters parsed in ParseSurfaceSpriteOptional.
static bool ParseSurfaceSprites(const char *buffer, shaderStage_t *stage)
{
	const char *token;
	const char **text = &buffer;
	surfaceSpriteType_t sstype = SURFSPRITE_NONE;

	// spritetype
	token = COM_ParseExt(text, qfalse);

	if (token[0] == '\0')
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
			shader.name);
		return false;
	}

	if (!Q_stricmp(token, "vertical"))
	{
		sstype = SURFSPRITE_VERTICAL;
	}
	else if (!Q_stricmp(token, "oriented"))
	{
		sstype = SURFSPRITE_ORIENTED;
	}
	else if (!Q_stricmp(token, "effect"))
	{
		sstype = SURFSPRITE_EFFECT;
	}
	else if (!Q_stricmp(token, "flattened"))
	{
		sstype = SURFSPRITE_FLATTENED;
	}
	else
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: invalid type in shader '%s'\n",
			shader.name);

		return false;
	}

	// width
	token = COM_ParseExt(text, qfalse);
	if (token[0] == '\0')
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
			shader.name);
		return false;
	}

	float width = atof(token);
	if (width <= 0.0f)
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: invalid width in shader '%s'\n",
			shader.name);
		return false;
	}

	// height
	token = COM_ParseExt(text, qfalse);
	if (token[0] == '\0')
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
			shader.name);
		return false;
	}

	float height = atof(token);
	if (height <= 0.0f)
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: invalid height in shader '%s'\n",
			shader.name);
		return false;
	}

	// density
	token = COM_ParseExt(text, qfalse);
	if (token[0] == '\0')
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
			shader.name);
		return false;
	}

	float density = atof(token);
	if (density <= 0.0f)
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: invalid density in shader '%s'\n",
			shader.name);
		return false;
	}

	// fadedist
	token = COM_ParseExt(text, qfalse);
	if (token[0] == '\0')
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n",
			shader.name);
		return false;
	}

	float fadedist = atof(token);
	if (fadedist < 32.0f)
	{
		ri->Printf(PRINT_ALL,
			S_COLOR_YELLOW "WARNING: invalid fadedist (%.2f < 32) in shader '%s'\n",
			fadedist, shader.name);
		return false;
	}

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t *)ri->Hunk_Alloc(sizeof(surfaceSprite_t), h_low);
	}

	// These are all set by the command lines.
	stage->ss->type = sstype;
	stage->ss->width = width;
	stage->ss->height = height;
	stage->ss->density = density;
	stage->ss->fadeDist = fadedist;

	// These are defaults that can be overwritten.
	stage->ss->fadeMax = fadedist * 1.33f;
	stage->ss->fadeScale = 0.0f;
	stage->ss->wind = 0.0f;
	stage->ss->windIdle = 0.0f;
	stage->ss->variance[0] = 0.0f;
	stage->ss->variance[1] = 0.0f;
	stage->ss->facing = SURFSPRITE_FACING_NORMAL;

	// A vertical parameter that needs a default regardless
	stage->ss->vertSkew = 0.0f;

	// These are effect parameters that need defaults nonetheless.
	stage->ss->fxDuration = 1000;		// 1 second
	stage->ss->fxGrow[0] = 0.0f;
	stage->ss->fxGrow[1] = 0.0f;
	stage->ss->fxAlphaStart = 1.0f;
	stage->ss->fxAlphaEnd = 0.0f;

	return true;
}




// Parses the following keywords in a shader stage:
//
// 		ssFademax <fademax>
// 		ssFadescale <fadescale>
// 		ssVariance <varwidth> <varheight>
// 		ssHangdown
// 		ssAnyangle
// 		ssFaceup
// 		ssWind <wind>
// 		ssWindIdle <windidle>
// 		ssVertSkew <skew>
// 		ssFXDuration <duration>
// 		ssFXGrow <growwidth> <growheight>
// 		ssFXAlphaRange <alphastart> <startend>
// 		ssFXWeather
static bool ParseSurfaceSpritesOptional(
		const char *param,
		const char *buffer,
		shaderStage_t *stage
)
{
	const char *token;
	const char **text = &buffer;
	float value;

	if (!stage->ss)
	{
		stage->ss = (surfaceSprite_t *)ri->Hunk_Alloc(sizeof(surfaceSprite_t), h_low );
	}

	// TODO: Tidy this up some how. There's a lot of repeated code

	//
	// fademax
	//
	if (!Q_stricmp(param, "ssFademax"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fademax in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value <= stage->ss->fadeDist)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fademax (%.2f <= fadeDist(%.2f)) in shader '%s'\n", value, stage->ss->fadeDist, shader.name );
			return false;
		}
		stage->ss->fadeMax=value;
		return true;
	}

	//
	// fadescale
	//
	if (!Q_stricmp(param, "ssFadescale"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fadescale in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		stage->ss->fadeScale=value;
		return true;
	}

	//
	// variance
	//
	if (!Q_stricmp(param, "ssVariance"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite variance width in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance width in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->variance[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite variance height in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite variance height in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->variance[1]=value;
		return true;
	}

	//
	// hangdown
	//
	if (!Q_stricmp(param, "ssHangdown"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Hangdown facing overrides previous facing in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->facing=SURFSPRITE_FACING_DOWN;
		return true;
	}

	//
	// anyangle
	//
	if (!Q_stricmp(param, "ssAnyangle"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Anyangle facing overrides previous facing in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->facing=SURFSPRITE_FACING_ANY;
		return true;
	}

	//
	// faceup
	//
	if (!Q_stricmp(param, "ssFaceup"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: Faceup facing overrides previous facing in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->facing=SURFSPRITE_FACING_UP;
		return true;
	}

	//
	// wind
	//
	if (!Q_stricmp(param, "ssWind"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite wind in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite wind in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->wind=value;
		if (stage->ss->windIdle <= 0)
		{	// Also override the windidle, it usually is the same as wind
			stage->ss->windIdle = value;
		}
		return true;
	}

	//
	// windidle
	//
	if (!Q_stricmp(param, "ssWindidle"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite windidle in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite windidle in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->windIdle=value;
		return true;
	}

	//
	// vertskew
	//
	if (!Q_stricmp(param, "ssVertskew"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite vertskew in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite vertskew in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->vertSkew=value;
		return true;
	}

	//
	// fxduration
	//
	if (!Q_stricmp(param, "ssFXDuration"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite duration in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value <= 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite duration in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxDuration=value;
		return true;
	}

	//
	// fxgrow
	//
	if (!Q_stricmp(param, "ssFXGrow"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite grow width in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow width in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxGrow[0]=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite grow height in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite grow height in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxGrow[1]=value;
		return true;
	}

	//
	// fxalpharange
	//
	if (!Q_stricmp(param, "ssFXAlphaRange"))
	{
		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha start in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxAlphaStart=value;

		token = COM_ParseExt( text, qfalse);
		if (token[0]==0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: missing surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return false;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid surfacesprite fxalpha end in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->fxAlphaEnd=value;
		return true;
	}

	//
	// fxweather
	//
	if (!Q_stricmp(param, "ssFXWeather"))
	{
		if (stage->ss->type != SURFSPRITE_EFFECT)
		{
			ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: weather applied to non-effect surfacesprite in shader '%s'\n", shader.name );
			return false;
		}
		stage->ss->type = SURFSPRITE_WEATHERFX;
		return true;
	}

	//
	// invalid ss command.
	//
	ri->Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: invalid optional surfacesprite param '%s' in shader '%s'\n", param, shader.name );
	return false;
}

char *StringContains(char *str1, char *str2, int casesensitive)
{
	int len, i, j;

	len = strlen(str1) - strlen(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (toupper(str1[j]) != toupper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
} //end of the function StringContains

qboolean ForceGlow2 ( char *shader )
{
	if (!shader) return qfalse;

	if (StringContains(shader, "bespin/bench", 0) || StringContains(shader, "bespin/chair", 0))
	{
		return qfalse;
	}

	// UQ1: Testing - Force glow to obvious glow components...
	// Note that this is absolutely a complete HACK... But the only other option is to remake every other map ever made for JKA...
	// Worst case, we end up with a little extra glow - oh dear! the horror!!! :)
	if (StringContains(shader, "glw", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "glow", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "street_light", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "sconce", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "pulse", 0) && !StringContains(shader, "/pulsecannon", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "pls", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "mp/s_flat", 0))
	{
		return qtrue;
	}
	/*else if (StringContains(shader, "blend", 0))
	{
		return qtrue;
	}*/
	else if (StringContains(shader, "onoffr", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "onoffg", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "neon", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "flare", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "comp_panel", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "d_switch", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "doom_display", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "door1_red", 0)) // doomgiver
	{
		return qtrue;
	}
	else if (StringContains(shader, "mapd", 0)) // doomgiver - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "screen0", 0)) // doomgiver - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "_energy", 0)) // doomgiver - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "_display", 0)) // h_evil - may cause issues?
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_off", 0)) // for h_evil/switch_off
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_on", 0)) // for h_evil/switch_on
	{
		return qtrue;
	}
	else if (StringContains(shader, "_screen_", 0)) // for hoth
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_lift", 0)) // for hoth
	{
		return qtrue;
	}
	else if (StringContains(shader, "lava", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "_grn", 0))
	{
		return qtrue;
	}
	/*else if (StringContains(shader, "_red", 0))
	{
		return qtrue;
	}*/
	else if (StringContains(shader, "static_field3", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "power222", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "ggoo", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "blink", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "_blb", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "imp_mine/tanklight", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "cache_panel_anim", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_on", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_off", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_open", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "switch_locked", 0) && !StringContains(shader, "s_switch_locked", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "door_switch_", 0)) // for desert/door_switch_red.png - testing loose check in case of other switch's - may cause bad results
	{
		return qtrue;
	}
	else if (StringContains(shader, "impdetention/doortrim01", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "holotarget", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "eleswitcha", 0)) // ns_streets
	{
		return qtrue;
	}
	else if (StringContains(shader, "eleswitchb", 0)) // ns_streets
	{
		return qtrue;
	}
	else if (StringContains(shader, "_crystal", 0)) // rift
	{
		return qtrue;
	}
	else if (StringContains(shader, "rocky_ruins/screen", 0)) // rocky_ruins
	{
		return qtrue;
	}
	else if (StringContains(shader, "rooftop/screen", 0)) // rooftop
	{
		return qtrue;
	}
	else if (StringContains(shader, "vjun/screen", 0)) // vjun
	{
		return qtrue;
	}
	else if (StringContains(shader, "white", 0) && !StringContains(shader, "flower", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "symbol", 0) && StringContains(shader, "fx", 0)) // rift
	{
		return qtrue;
	}
	else if (StringContains(shader, "deathconlight", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "impgarrison/light_panel_01", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "winlite", 0)) // ns_streets
	{
		return qtrue;
	}
	else if (StringContains(shader, "reclame", 0)) // SJC
	{
		return qtrue;
	}
	else if (StringContains(shader, "flash", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "blob", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "shot", 0) && !StringContains(shader, "acp_arraygun", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "fire", 0) && !StringContains(shader, "campfire", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "flame", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "_blend", 0) && StringContains(shader, "light", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "_blend", 0) && StringContains(shader, "strip", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "_blend", 0) && StringContains(shader, "step", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "blue", 0) && StringContains(shader, "light", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "red", 0) && StringContains(shader, "light", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "green", 0) && StringContains(shader, "light", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "yellow", 0) && StringContains(shader, "light", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "mp/s_flat", 0))
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "cmuzzle", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "ftail", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "shot", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "lightcone", 0)) // GFX - hmm ???
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "spikeb", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "stunpass", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "solidwhite", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "wookie1", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "plasma", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "plume", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "blueline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "redline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "embers", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "burst", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "blob", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "burn", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "caustic", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "bolt", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "exp0", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "lightning", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "mine", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "redring", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "rline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "spark", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "sun", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/", 0) && StringContains(shader, "whiteline", 0)) // GFX
	{
		return qtrue;
	}
	else if (StringContains(shader, "gfx/exp/", 0)) // GFX
	{
		return qtrue;
	}

	return qfalse;
}

qboolean ForceGlow(char *shader)
{
	if (!shader) return qfalse;

	qboolean glow = ForceGlow2(shader);

	if (glow && StringContains(shader, "lightmap", 0))
	{
		return qfalse;
	}

	return glow;
}

static void ComputeShaderGlowColors( shaderStage_t *pStage )
{
	colorGen_t rgbGen = pStage->rgbGen;
	alphaGen_t alphaGen = pStage->alphaGen;

	pStage->glowColorFound = qfalse;

	pStage->glowColor[0] =
   	pStage->glowColor[1] =
   	pStage->glowColor[2] =
   	pStage->glowColor[3] = 1.0f;

	if (!pStage->glow) return;

	switch ( rgbGen )
	{
		case CGEN_CONST:
			pStage->glowColor[0] = pStage->constantColor[0] / 255.0f;
			pStage->glowColor[1] = pStage->constantColor[1] / 255.0f;
			pStage->glowColor[2] = pStage->constantColor[2] / 255.0f;
			pStage->glowColor[3] = pStage->constantColor[3] / 255.0f;
			//ri->Printf(PRINT_ALL, "Glow color found for shader %s - %f %f %f %f.\n", shader.name, pStage->glowColor[0], pStage->glowColor[1], pStage->glowColor[2], pStage->glowColor[3]);
			pStage->glowColorFound = qtrue;
			return;
			break;
		case CGEN_LIGHTMAPSTYLE:
			VectorScale4(styleColors[pStage->lightmapStyle], 1.0f / 255.0f, pStage->glowColor);
			//ri->Printf(PRINT_ALL, "Glow color found (style) for shader %s - %f %f %f %f.\n", shader.name, pStage->glowColor[0], pStage->glowColor[1], pStage->glowColor[2], pStage->glowColor[3]);
			pStage->glowColorFound = qtrue;
			return;
			break;
		default:
			break;
	}

	//if (pStage->bundle[TB_DIFFUSEMAP].image && pStage->bundle[TB_DIFFUSEMAP].image[0] == tr.whiteImage)
	{// Testing - Assume white... Might add shader keyword for glow light colors at some point maybe...
		pStage->glowColor[0] = 1.0;
		pStage->glowColor[1] = 1.0;
		pStage->glowColor[2] = 1.0;
		pStage->glowColor[3] = 1.0;
		pStage->glowColorFound = qtrue;
		//ri->Printf(PRINT_ALL, "Glow color set for whiteimage shader %s - %f %f %f %f.\n", shader.name, pStage->glowColor[0], pStage->glowColor[1], pStage->glowColor[2], pStage->glowColor[3]);
		return;
	}
}

/*
===================
ParseStage
===================
*/
static qboolean ParseStage( shaderStage_t *stage, const char **text )
{
	char *token;
	unsigned depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;
	stage->useSkyImage = false;

	/* WZ Particle EFX */
	stage->particleColor[0] = 1.0;
	stage->particleColor[1] = 1.0;
	stage->particleColor[2] = 0.0;

	/* WZ FireFly EFX */
	stage->fireFlyCount = 30;
	stage->fireFlyColor[0] = 0.94;
	stage->fireFlyColor[1] = 0.94;
	stage->fireFlyColor[2] = 0.14;

	/* WZ Portal EFX */
	stage->portalColor1[0] = 0.125;
	stage->portalColor1[1] = 0.291;
	stage->portalColor1[2] = 0.923;

	stage->portalColor2[0] = 0.925;
	stage->portalColor2[1] = 0.791;
	stage->portalColor2[2] = 0.323;

	stage->portalImageColor[0] = 1.0;
	stage->portalImageColor[1] = 1.0;
	stage->portalImageColor[2] = 1.0;
	stage->portalImageAlpha = 1.0;

	stage->glowStrength = 1.0;
	stage->glowVibrancy = 0.0;
	stage->glowNoMerge = qfalse;

	stage->glowMultiplierRGBA[0] = 1.0;
	stage->glowMultiplierRGBA[1] = 1.0;
	stage->glowMultiplierRGBA[2] = 1.0;
	stage->glowMultiplierRGBA[3] = 1.0;

	stage->emissiveRadiusScale = 1.0;
	stage->emissiveColorScale = 1.5;

	stage->colorMod[0] = 0.0;
	stage->colorMod[1] = 0.0;
	stage->colorMod[2] = 0.0;

	stage->envmapStrength = 0.0;

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri->Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		if ( token[0] == '}' )
		{
			break;
		}
		//
		// map <name>
		//
		else if ( !Q_stricmp( token, "map" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[0].image[0] = tr.whiteImage;

				// UQ1: Testing - Force glow to obvious glow components...
				if (ForceGlow(stage->bundle[0].image[0]->imgName) || stage->glow)
				{
					//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[0]->imgName);
					stage->glow = qtrue;
					
					if (stage->emissiveRadiusScale <= 0.0)
						stage->emissiveRadiusScale = 1.0;

					if (stage->emissiveColorScale <= 0.0)
						stage->emissiveColorScale = 1.5;

					stage->glowStrength = 1.0;// 0.5;
				}
				//UQ1: END - Testing - Force glow to obvious glow components...
				continue;
			}
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps ) {
#ifndef FINAL_BUILD
					ri->Printf (PRINT_ALL, S_COLOR_RED "Lightmap requested but none avilable for shader '%s'\n", shader.name);
#endif
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
				}

				// UQ1: Testing - Force glow to obvious glow components...
				/*if (ForceGlow(stage->bundle[0].image[0]->imgName))
				{
					//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[0]->imgName);
					stage->glow = qtrue;

					if (stage->emissiveRadiusScale <= 0.0)
						stage->emissiveRadiusScale = 1.0;

					if (stage->emissiveColorScale <= 0.0)
						stage->emissiveColorScale = 1.5;
				}*/
				//UQ1: END - Testing - Force glow to obvious glow components...

				stage->noScreenMap = qtrue;

				continue;
			}
			else if (!Q_stricmp(token, "$skyimage"))
			{
				stage->useSkyImage = true;
				continue;
			}
			else if ( !Q_stricmp( token, "$deluxemap" ) )
			{
				if (!tr.worldDeluxeMapping)
				{
					ri->Printf( PRINT_WARNING, "WARNING: shader '%s' wants a deluxe map in a map compiled without them\n", shader.name );
					return qfalse;
				}

				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
				}

				// UQ1: Testing - Force glow to obvious glow components...
				/*if (ForceGlow(stage->bundle[0].image[0]->imgName))
				{
					//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[0]->imgName);
					stage->glow = qtrue;

					if (stage->emissiveRadiusScale <= 0.0)
						stage->emissiveRadiusScale = 1.0;

					if (stage->emissiveColorScale <= 0.0)
						stage->emissiveColorScale = 1.5;
				}*/

				stage->noScreenMap = qtrue;

				//UQ1: END - Testing - Force glow to obvious glow components...
				continue;
			}
			else if (!Q_stricmp(token, "envmap") || !Q_stricmp(token, "$envmap"))
			{
				stage->bundle[0].image[0] = tr.envmapImage;
				continue;
			}
			else if (!Q_stricmp(token, "envmap_spec") || !Q_stricmp(token, "$envmap_spec"))
			{
				stage->bundle[0].image[0] = tr.envmapSpecImage;
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				int flags = IMGFLAG_NONE;

				if (!shader.noMipMaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				if (shader.noTC)
					flags |= IMGFLAG_NO_COMPRESSION;

				if (stage->type == ST_NORMALMAP || stage->type == ST_NORMALPARALLAXMAP)
				{
					type = IMGTYPE_NORMAL;
					flags |= IMGFLAG_NOLIGHTSCALE;

					if (stage->type == ST_NORMALPARALLAXMAP)
						type = IMGTYPE_NORMALHEIGHT;

					stage->noScreenMap = qtrue;
				}
				else if (stage->type == ST_SPECULARMAP)
				{
					type = IMGTYPE_SPECULAR;
					flags |= IMGFLAG_NOLIGHTSCALE;

					stage->noScreenMap = qtrue;
				}
				else
				{
					//if (r_genNormalMaps->integer)
						flags |= IMGFLAG_GENNORMALMAP;

					if (r_srgb->integer)
						flags |= IMGFLAG_SRGB;
				}

				if (/*ForceGlow(token) ||*/ stage->glow)
				{
					flags |= IMGFLAG_GLOW;
				}

#ifdef __DEFERRED_IMAGE_LOADING__
				stage->bundle[0].image[0] = R_DeferImageLoad(token, type, flags);
#else //!__DEFERRED_IMAGE_LOADING__
				stage->bundle[0].image[0] = R_FindImageFile(token, type, flags);
#endif //__DEFERRED_IMAGE_LOADING__

				if (!DISABLE_MERGED_GLOWS)
				{
					char imgname[256] = { 0 };
					char stippedName[256] = { 0 };
					COM_StripExtension(token, stippedName, sizeof(stippedName));
					sprintf(imgname, "%s_g", stippedName);
					
#ifdef __DEFERRED_IMAGE_LOADING__
					stage->bundle[TB_GLOWMAP].image[0] = R_DeferImageLoad(imgname, type, flags | IMGFLAG_GLOW);
#else //!__DEFERRED_IMAGE_LOADING__
					stage->bundle[TB_GLOWMAP].image[0] = R_FindImageFile(imgname, type, (flags & IMGFLAG_GLOW) ? flags : (flags | IMGFLAG_GLOW));
#endif //__DEFERRED_IMAGE_LOADING__
				}

				if (stage->bundle[TB_GLOWMAP].image[0]
					&& stage->bundle[TB_GLOWMAP].image[0] != tr.defaultImage)
				{// We found a mergable glow map...
					stage->glowMapped = qtrue;
					stage->glow = qtrue;
					stage->glowColorFound = qtrue;
					VectorCopy4(stage->bundle[TB_GLOWMAP].image[0]->lightColor, stage->glowColor);
					stage->glowBlend = 0;
					stage->glslShaderIndex |= LIGHTDEF_USE_GLOW_BUFFER;

					if (stage->emissiveRadiusScale <= 0.0)
						stage->emissiveRadiusScale = 1.0;

					if (stage->emissiveColorScale <= 0.0)
						stage->emissiveColorScale = 1.5;

					//ri->Printf(PRINT_WARNING, "Shader [%s] diffuseMap [%s] found a _g glow texture [%s].\n", shader.name, stage->bundle[0].image[0]->imgName, stage->bundle[TB_GLOWMAP].image[0]->imgName);
				}
				else
				{
					stage->bundle[TB_GLOWMAP].image[0] = NULL;
				}

				// UQ1: Testing - Force glow to obvious glow components...
				if (flags & IMGFLAG_GLOW)
				{
					//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[0]->imgName);
					stage->glow = qtrue;

					if (stage->emissiveRadiusScale <= 0.0)
						stage->emissiveRadiusScale = 1.0;

					if (stage->emissiveColorScale <= 0.0)
						stage->emissiveColorScale = 1.5;
				}
				//UQ1: END - Testing - Force glow to obvious glow components...

				if (StringContains(token, "street_light", 0))
				{// bespin light hack...
					shader.glowStrength = 1.0;// 0.35357;// 0.275;
					stage->emissiveRadiusScale = 16.0;
				}
				else if (StringContains(token, "sconce", 0))
				{// bespin light hack...
					shader.glowStrength = 1.0;// 0.575;
					stage->emissiveRadiusScale = 3.0;
				}

				if ( !stage->bundle[0].image[0] )
				{
					ri->Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !Q_stricmp( token, "clampmap" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			int flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if (shader.noTC)
				flags |= IMGFLAG_NO_COMPRESSION;

			if (stage->type == ST_NORMALMAP || stage->type == ST_NORMALPARALLAXMAP)
			{
				type = IMGTYPE_NORMAL;
				flags |= IMGFLAG_NOLIGHTSCALE;

				if (stage->type == ST_NORMALPARALLAXMAP)
					type = IMGTYPE_NORMALHEIGHT;
			}
			else
			{
				//if (r_genNormalMaps->integer)
					flags |= IMGFLAG_GENNORMALMAP;

				if (r_srgb->integer)
					flags |= IMGFLAG_SRGB;
			}


			if (/*ForceGlow(token) ||*/ stage->glow)
			{
				flags |= IMGFLAG_GLOW;
			}

#ifdef __DEFERRED_IMAGE_LOADING__
			stage->bundle[0].image[0] = R_DeferImageLoad(token, type, flags);
#else //!__DEFERRED_IMAGE_LOADING__
			stage->bundle[0].image[0] = R_FindImageFile( token, type, flags );
#endif //__DEFERRED_IMAGE_LOADING__

			if (!DISABLE_MERGED_GLOWS)
			{
				char imgname[256] = { 0 };
				char stippedName[256] = { 0 };
				COM_StripExtension(token, stippedName, sizeof(stippedName));
				sprintf(imgname, "%s_g", stippedName);

#ifdef __DEFERRED_IMAGE_LOADING__
				stage->bundle[TB_GLOWMAP].image[0] = R_DeferImageLoad(imgname, type, flags | IMGFLAG_GLOW);
#else //!__DEFERRED_IMAGE_LOADING__
				stage->bundle[TB_GLOWMAP].image[0] = R_FindImageFile(imgname, type, (flags & IMGFLAG_GLOW) ? flags : (flags | IMGFLAG_GLOW));
#endif //__DEFERRED_IMAGE_LOADING__
			}

			if (stage->bundle[TB_GLOWMAP].image[0]
				&& stage->bundle[TB_GLOWMAP].image[0] != tr.defaultImage)
			{// We found a mergable glow map...
				stage->glowMapped = qtrue;
				stage->glow = qtrue;
				stage->glowColorFound = qtrue;
				VectorCopy4(stage->bundle[TB_GLOWMAP].image[0]->lightColor, stage->glowColor);
				stage->glowBlend = 0;
				stage->glslShaderIndex |= LIGHTDEF_USE_GLOW_BUFFER;

				if (stage->emissiveRadiusScale <= 0.0)
					stage->emissiveRadiusScale = 1.0;

				if (stage->emissiveColorScale <= 0.0)
					stage->emissiveColorScale = 1.5;

				//ri->Printf(PRINT_WARNING, "Shader [%s] diffuseMap [%s] found a _g glow texture [%s].\n", shader.name, stage->bundle[0].image[0]->imgName, stage->bundle[TB_GLOWMAP].image[0]->imgName);
			}
			else
			{
				stage->bundle[TB_GLOWMAP].image[0] = NULL;
			}

			// UQ1: Testing - Force glow to obvious glow components...
			if (flags & IMGFLAG_GLOW)
			{
				//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[0]->imgName);
				stage->glow = qtrue;

				if (stage->emissiveRadiusScale <= 0.0)
					stage->emissiveRadiusScale = 1.0;

				if (stage->emissiveColorScale <= 0.0)
					stage->emissiveColorScale = 1.5;
			}
			//UQ1: END - Testing - Force glow to obvious glow components...

			if ( !stage->bundle[0].image[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) || !Q_stricmp( token, "clampanimMap" ) || !Q_stricmp( token, "oneshotanimMap" ) )
		{
			animMapType_t type = AnimMapType( token );
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for '%s' keyword in shader '%s'\n", animMapNames[type], shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );
			stage->bundle[0].oneShotAnimMap = (qboolean)(type == ANIMMAP_ONESHOT);

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					int flags = type == ANIMMAP_CLAMP ? IMGFLAG_CLAMPTOEDGE : IMGFLAG_NONE;

					if (!shader.noMipMaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					if (r_srgb->integer)
						flags |= IMGFLAG_SRGB;

					if (shader.noTC)
						flags |= IMGFLAG_NO_COMPRESSION;

#ifdef __DEFERRED_IMAGE_LOADING__
					stage->bundle[0].image[num] = R_DeferImageLoad(token, IMGTYPE_COLORALPHA, flags);
#else //!__DEFERRED_IMAGE_LOADING__
					stage->bundle[0].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );
#endif //__DEFERRED_IMAGE_LOADING__

					// UQ1: Testing - Force glow to obvious glow components...
					/*if (ForceGlow(stage->bundle[0].image[num]->imgName))
					{
						//ri->Printf (PRINT_WARNING, "%s forcably marked as a glow shader.\n", stage->bundle[0].image[num]->imgName);
						stage->glow = qtrue;

						if (stage->emissiveRadiusScale <= 0.0)
							stage->emissiveRadiusScale = 1.0;

						if (stage->emissiveColorScale <= 0.0)
							stage->emissiveColorScale = 1.5;
					}*/
					//UQ1: END - Testing - Force glow to obvious glow components...

					if ( !stage->bundle[0].image[num] )
					{
						ri->Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		}
		else if ( !Q_stricmp( token, "videoMap" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri->CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1) {
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		else if (!Q_stricmp(token, "overlayMap") || !Q_stricmp(token, "overlaymap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for 'overlayMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			imgType_t type = IMGTYPE_OVERLAY;
			int flags = IMGFLAG_NONE;

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if (shader.noTC)
				flags |= IMGFLAG_NO_COMPRESSION;

#ifdef __DEFERRED_IMAGE_LOADING__
			stage->bundle[TB_OVERLAYMAP].image[0] = R_DeferImageLoad(token, type, flags);
#else //!__DEFERRED_IMAGE_LOADING__
			stage->bundle[TB_OVERLAYMAP].image[0] = R_FindImageFile(token, type, flags);
#endif //__DEFERRED_IMAGE_LOADING__

			if (!stage->bundle[TB_OVERLAYMAP].image[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find overlayMap image '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}
		else if (!Q_stricmp(token, "envMap") || !Q_stricmp(token, "envmap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for 'envMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			else if (!Q_stricmp(token, "$skyimage"))
			{
				stage->envmapUseSkyImage = true;
				continue;
			}

			imgType_t type = IMGTYPE_OVERLAY;
			int flags = IMGFLAG_NONE;

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if (shader.noTC)
				flags |= IMGFLAG_NO_COMPRESSION;

#ifdef __DEFERRED_IMAGE_LOADING__
			stage->bundle[TB_ENVMAP].image[0] = R_DeferImageLoad(token, type, flags);
#else //!__DEFERRED_IMAGE_LOADING__
			stage->bundle[TB_ENVMAP].image[0] = R_FindImageFile(token, type, flags);
#endif //__DEFERRED_IMAGE_LOADING__

			if (!stage->bundle[TB_ENVMAP].image[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find envMap image '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}
		//
		// alphafunc <func>
		//
		else if ( !Q_stricmp( token, "alphaFunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( token );
			shader.hasAlpha = qtrue;
			continue;
		}
		//
		// depthFunc <func>
		//
		else if ( !Q_stricmp( token, "depthfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "lequal" ) )
			{
				depthFuncBits = 0;
			}
			else if ( !Q_stricmp( token, "equal" ) )
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else if ( !Q_stricmp( token, "disable" ) )
			{
				depthFuncBits = GLS_DEPTHTEST_DISABLE;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
				continue;
			}

			continue;
		}
		//
		// detail
		//
		else if ( !Q_stricmp( token, "detail" ) )
		{
			stage->isDetail = qtrue;
			continue;
		}
		//
		// Don't output this stage to position and normal maps...
		//
		else if (!Q_stricmp(token, "noScreenMap"))
		{
			stage->noScreenMap = qtrue;
			continue;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !Q_stricmp( token, "blendfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if ( !Q_stricmp( token, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !Q_stricmp( token, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					ri->Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit )
			{
				depthMaskBits = 0;
			}

			continue;
		}
		//
		// stage <type>
		//
		else if(!Q_stricmp(token, "stage"))
		{
			token = COM_ParseExt(text, qfalse);
			if(token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameters for stage in shader '%s'\n", shader.name);
				continue;
			}

			if(!Q_stricmp(token, "diffuseMap"))
			{
				stage->type = ST_DIFFUSEMAP;
			}
			else if(!Q_stricmp(token, "normalMap") || !Q_stricmp(token, "bumpMap"))
			{
				stage->type = ST_NORMALMAP;
				VectorSet4(stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
			}
			else if(!Q_stricmp(token, "normalParallaxMap") || !Q_stricmp(token, "bumpParallaxMap"))
			{
				stage->type = ST_NORMALMAP;
				VectorSet4(stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
			}
			else if(!Q_stricmp(token, "specularMap"))
			{
				stage->type = ST_SPECULARMAP;
				//VectorSet4(stage->specularScale, r_baseSpecular->integer, r_baseSpecular->integer, r_baseSpecular->integer, 1.0f);
				VectorSet4(stage->specularScale, 1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
			{
				ri->Printf(PRINT_WARNING, "WARNING: unknown stage parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}

			continue;
		}
		else if (!Q_stricmp(token, "cubeMapScale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for cubeMapScale in shader '%s'\n", shader.name );
				continue;
			}
			stage->cubeMapScale = atof( token );
			continue;
		}
		//
		// specularReflectance <value>
		//
		else if (!Q_stricmp(token, "specularreflectance"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specular reflectance in shader '%s'\n", shader.name );
				continue;
			}
			stage->specularScale[0] =
			stage->specularScale[1] =
			stage->specularScale[2] = Com_Clamp( 0.0f, 1.0f, atof( token ) );
			continue;
		}
		//
		// specularExponent <value>
		//
		else if (!Q_stricmp(token, "specularexponent"))
		{
			float exponent;

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specular exponent in shader '%s'\n", shader.name );
				continue;
			}
			exponent = atof( token );

			// Change shininess to gloss
			// FIXME: assumes max exponent of 8192 and min of 1, must change here if altered in lightall_fp.glsl
			exponent = CLAMP(exponent, 1.0, 8192.0);
			stage->specularScale[3] = log(exponent) / log(8192.0);
			continue;
		}
		//
		// gloss <value>
		//
		else if ( !Q_stricmp( token, "gloss" ) )
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for gloss in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[3] = atof( token );
			continue;
		}
		//
		// parallaxDepth <value>
		//
		else if (!Q_stricmp(token, "parallaxdepth"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for parallaxDepth in shader '%s'\n", shader.name );
				continue;
			}

			stage->normalScale[3] = atof( token );
			continue;
		}
		//
		// normalScale <xy>
		// or normalScale <x> <y>
		// or normalScale <x> <y> <height>
		//
		else if (!Q_stricmp(token, "normalscale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for normalScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->normalScale[0] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// one value, applies to X/Y
				stage->normalScale[1] = stage->normalScale[0];
				continue;
			}

			stage->normalScale[1] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// two values, no height
				continue;
			}

			stage->normalScale[3] = atof( token );
			continue;
		}
		//
		// specularScale <rgb> <gloss>
		// or specularScale <r> <g> <b>
		// or specularScale <r> <g> <b> <gloss>
		//
		else if (!Q_stricmp(token, "specularscale"))
		{
			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[0] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameter for specularScale in shader '%s'\n", shader.name );
				continue;
			}

			stage->specularScale[1] = atof( token );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// two values, rgb then gloss
				stage->specularScale[3] = stage->specularScale[1];
				stage->specularScale[1] =
				stage->specularScale[2] = stage->specularScale[0];
				continue;
			}

			stage->specularScale[2] = Com_Clamp( 0.0f, 1.0f, atof( token ) );

			token = COM_ParseExt(text, qfalse);
			if ( token[0] == 0 )
			{
				// three values, rgb
				continue;
			}

			stage->specularScale[3] = atof( token );
			continue;
		}
		//
		// rgbGen
		//
		else if ( !Q_stricmp( token, "rgbGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				vec3_t	color;

				ParseVector( text, 3, color );
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "identityLighting" ) )
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertex" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if ( !Q_stricmp( token, "vertexLit" ) )
			{
				stage->rgbGen = CGEN_VERTEX_LIT;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertexLit" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX_LIT;
			}
			else if ( !Q_stricmp( token, "lightingDiffuse" ) )
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if ( !Q_stricmp( token, "lightingDiffuseEntity" ) )
			{
				if (shader.lightmapIndex[0] != LIGHTMAP_NONE)
				{
					Com_Printf( S_COLOR_RED "ERROR: rgbGen lightingDiffuseEntity used on a misc_model! in shader '%s'\n", shader.name );
				}
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else if (!Q_stricmp(token, "warzoneLighting"))
			{
				stage->rgbGen = CGEN_LIGHTING_WARZONE;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}

			continue;
		}
		//
		// alphaGen
		//
		else if ( !Q_stricmp( token, "alphaGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				token = COM_ParseExt( text, qfalse );
				stage->constantColor[3] = 255 * atof( token );
				stage->alphaGen = AGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if ( !Q_stricmp( token, "lightingSpecular" ) )
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if ( !Q_stricmp( token, "dot" ) )
			{
				//stage->alphaGen = AGEN_DOT;
			}
			else if ( !Q_stricmp( token, "oneMinusDot" ) )
			{
				//stage->alphaGen = AGEN_ONE_MINUS_DOT;
			}
			else if ( !Q_stricmp( token, "portal" ) )
			{
				stage->alphaGen = AGEN_PORTAL;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					shader.portalRange = 256;
					ri->Printf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				}
				else
				{
					shader.portalRange = atof( token );
				}
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}

			continue;
		}
		//
		// tcGen <function>
		//
		else if ( !Q_stricmp(token, "texgen") || !Q_stricmp( token, "tcGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "environment" ) )
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if ( !Q_stricmp( token, "lightmap" ) )
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if ( !Q_stricmp( token, "vector" ) )
			{
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}

			continue;
		}
		//
		// tcMod <type> <...>
		//
		else if ( !Q_stricmp( token, "tcMod" ) )
		{
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// depthmask
		//
		else if ( !Q_stricmp( token, "depthwrite" ) )
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		}
		else if (!Q_stricmp(token, "colorMod") || !Q_stricmp(token, "colorModifier"))
		{
			ParseVector(text, 3, stage->colorMod);
			SkipRestOfLine(text);
			continue;
		}
		// If this stage has glow...	GLOWXXX
		else if ( Q_stricmp( token, "glow" ) == 0 )
		{
			stage->glow = qtrue;
			
			if (stage->emissiveRadiusScale <= 0.0) 
				stage->emissiveRadiusScale = 1.0;

			if (stage->emissiveColorScale <= 0.0)
				stage->emissiveColorScale = 1.5;

			stage->emissiveHeightScale = 0.0;
			stage->emissiveConeAngle = 0.0;

			continue;
		}
		else if (!Q_stricmp(token, "glowStrength"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'glowStrength' keyword in shader '%s'\n", shader.name);
				stage->glowStrength = 1.0;
				continue;
			}
			stage->glowStrength = atof(token);
			continue;
		}
		else if (!Q_stricmp(token, "glowVibrancy"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'glowVibrancy' keyword in shader '%s'\n", shader.name);
				stage->glowVibrancy = 0.0;
				continue;
			}
			stage->glowVibrancy = atof(token);
			continue;
		}
		else if (Q_stricmp(token, "glowMultiplierRGBA") == 0)
		{
			vec4_t	color;
			color[0] = 1.0;
			color[1] = 1.0;
			color[2] = 1.0;
			color[3] = 1.0;

			ParseVector(text, 4, color);

			stage->glowMultiplierRGBA[0] = color[0];
			stage->glowMultiplierRGBA[1] = color[1];
			stage->glowMultiplierRGBA[2] = color[2];
			stage->glowMultiplierRGBA[3] = color[3];
			continue;
		}
		else if (!Q_stricmp(token, "glowNoMerge"))
		{
			stage->glowNoMerge = qtrue;
			continue;
		}
		else if (!Q_stricmp(token, "isFoliage"))
		{
			stage->isFoliage = qtrue;
			stage->isFoliageChecked = qtrue;
			continue;
		}
		else if (!Q_stricmp(token, "envmapStrength") || !Q_stricmp(token, "envmapstrength"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'envmapStrength' keyword in shader '%s'\n", shader.name);
				stage->envmapStrength = 0.0;
				continue;
			}
			stage->envmapStrength = atof(token);
			continue;
		}
		else if (Q_stricmp(token, "particleColor") == 0)
		{
			vec3_t	color;

			ParseVector(text, 3, color);

			stage->particleColor[0] = color[0];
			stage->particleColor[1] = color[1];
			stage->particleColor[2] = color[2];
			continue;
		}
		else if (Q_stricmp(token, "fireFlyCount") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for fireFlyCount exponent in shader '%s'\n", shader.name);
				stage->fireFlyCount = 30;
				continue;
			}

			stage->fireFlyCount = atoi(token);
			continue;
		}
		else if (Q_stricmp(token, "fireFlyColor") == 0)
		{
			vec3_t	color;

			ParseVector(text, 3, color);

			stage->fireFlyColor[0] = color[0];
			stage->fireFlyColor[1] = color[1];
			stage->fireFlyColor[2] = color[2];
			continue;
		}
		else if (Q_stricmp(token, "portalColor1") == 0)
		{
			vec3_t	color;

			ParseVector(text, 3, color);

			stage->portalColor1[0] = color[0];
			stage->portalColor1[1] = color[1];
			stage->portalColor1[2] = color[2];
			continue;
		}
		else if (Q_stricmp(token, "portalColor2") == 0)
		{
			vec3_t	color;

			ParseVector(text, 3, color);

			stage->portalColor2[0] = color[0];
			stage->portalColor2[1] = color[1];
			stage->portalColor2[2] = color[2];
			continue;
		}
		else if (Q_stricmp(token, "portalImageColor") == 0)
		{
			vec3_t	color;

			ParseVector(text, 3, color);

			stage->portalImageColor[0] = color[0];
			stage->portalImageColor[1] = color[1];
			stage->portalImageColor[2] = color[2];
			continue;
		}
		else if (Q_stricmp(token, "portalImageAlpha") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for portalImageAlpha exponent in shader '%s'\n", shader.name);
				stage->portalImageAlpha = 1.0;
				continue;
			}

			stage->portalImageAlpha = atof(token);
			continue;
		}
		else if (Q_stricmp(token, "emissiveRadiusScale") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for emissiveRadiusScale exponent in shader '%s'\n", shader.name);
				stage->emissiveRadiusScale = 1.0;
				continue;
			}
			
			stage->emissiveRadiusScale = atof(token);
			//ri->Printf(PRINT_WARNING, "WARNING: emissiveRadiusScale exponent in shader '%s' is %f\n", shader.name, stage->emissiveRadiusScale);
			continue;
		}
		else if (Q_stricmp(token, "emissiveColorScale") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for emissiveColorScale exponent in shader '%s'\n", shader.name);
				stage->emissiveColorScale = 1.5;
				continue;
			}

			stage->emissiveColorScale = atof(token);
			//ri->Printf(PRINT_WARNING, "WARNING: emissiveColorScale exponent in shader '%s' is %f\n", shader.name, stage->emissiveColorScale);
			continue;
		}
		else if (Q_stricmp(token, "emissiveHeightScale") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for emissiveHeightScale exponent in shader '%s'\n", shader.name);
				stage->emissiveHeightScale = 0.0;
				continue;
			}

			stage->emissiveHeightScale = atof(token);
			continue;
		}
		else if (Q_stricmp(token, "emissiveConeAngle") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for emissiveConeAngle exponent in shader '%s'\n", shader.name);
				stage->emissiveConeAngle = 0.0;
				continue;
			}

			stage->emissiveConeAngle = atof(token);
			continue;
		}
		//
		// surfaceSprites <type> ...
		//
		else if ( !Q_stricmp( token, "surfaceSprites" ) )
		{
			// Mark this stage as a surface sprite so we can skip it for now
			stage->isSurfaceSprite = qtrue;
#if defined(__XYC_SURFACE_SPRITES__)
			char buffer[1024] = { 0 };

			while (1)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
					break;
				Q_strcat(buffer, sizeof(buffer), token);
				Q_strcat(buffer, sizeof(buffer), " ");
			}

			bool hasSS = (stage->ss != nullptr);
			if (ParseSurfaceSprites(buffer, stage) && !hasSS)
			{
				++shader.numSurfaceSpriteStages;
			}
#elif defined(__JKA_SURFACE_SPRITES__)
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
				//ri->Printf(PRINT_WARNING, "SS_DEBUG: shader: 5s. buffer: %s.\n", shader.name, buffer);
			}

			ParseSurfaceSprites( buffer, stage );
#else //!__XYC_SURFACE_SPRITES__
			SkipRestOfLine(text);
#endif //__XYC_SURFACE_SPRITES__
			continue;
		}
		//
		// ssFademax <fademax>
		// ssFadescale <fadescale>
		// ssVariance <varwidth> <varheight>
		// ssHangdown
		// ssAnyangle
		// ssFaceup
		// ssWind <wind>
		// ssWindIdle <windidle>
		// ssDuration <duration>
		// ssGrow <growwidth> <growheight>
		// ssWeather
		//
		else if (!Q_stricmpn(token, "ss", 2))	// <--- NOTE ONLY COMPARING FIRST TWO LETTERS
		{
#ifdef __XYC_SURFACE_SPRITES__
			stage->isSurfaceSprite = qtrue;

			char buffer[1024] = {};
			char param[128] = {};
			Q_strncpyz(param, token, sizeof(param));

			while (1)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == '\0')
					break;
				Q_strcat(buffer, sizeof(buffer), token);
				Q_strcat(buffer, sizeof(buffer), " ");
			}

			bool hasSS = (stage->ss != nullptr);
			if (ParseSurfaceSpritesOptional(param, buffer, stage) && !hasSS)
			{
				++shader.numSurfaceSpriteStages;
			}
#else //!__XYC_SURFACE_SPRITES__
			//SkipRestOfLine( text );
			char buffer[1024] = "";
			char param[128];
			strcpy(param,token);

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
				//ri->Printf(PRINT_WARNING, "SS_DEBUG2: shader: 5s. buffer: %s.\n", shader.name, buffer);
			}

			ParseSurfaceSpritesOptional( param, buffer, stage );
			stage->isSurfaceSprite = qtrue;
#endif //__XYC_SURFACE_SPRITES__
			continue;
		}
		else
		{
			ri->Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 || blendSrcBits == GLS_SRCBLEND_ONE || blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	if ((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) && (blendDstBits == GLS_DSTBLEND_ZERO))
	{// UQ1: Override... GLSL handles src alpha...
		blendSrcBits = GLS_SRCBLEND_ONE;
	}

	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ZERO ) )
	{
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == AGEN_IDENTITY ) {
		if ( stage->rgbGen == CGEN_IDENTITY || stage->rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	if (stage->glow)
	{
		ComputeShaderGlowColors( stage );
	}

	//
	// compute state bits
	//
	stage->stateBits =	depthMaskBits |
						blendSrcBits | 
						blendDstBits |
						atestBits |
						depthFuncBits;

	return qtrue;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform( const char **text ) {
	char	*token;
	deformStage_t	*ds;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri->Printf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri->Printf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !Q_stricmp( token, "projectionShadow" ) ) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if ( !Q_stricmp( token, "autosprite" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if ( !Q_stricmp( token, "autosprite2" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if ( !Q_stricmpn( token, "text", 4 ) ) {
		int		n;

		n = token[4] - '0';
		if ( n < 0 || n > 7 ) {
			n = 0;
		}
		ds->deformation = (deform_t)(DEFORM_TEXT0 + n);
		return;
	}

	if ( !Q_stricmp( token, "bulge" ) )	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( atof( token ) != 0 )
		{
			ds->deformationSpread = 1.0f / atof( token );
		}
		else
		{
			ds->deformationSpread = 100.0f;
			ri->Printf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) ) {
		int		i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri->Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri->Printf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}

int skyImageNum = -1;
byte *skyImagesData[6] = { NULL };

extern qboolean		PROCEDURAL_SKY_ENABLED;
extern image_t *R_CreateCubemapFromImageDatas(const char *name, byte **pic, int width, int height, imgType_t type, int flags, int internalFormat);

image_t *R_UploadSkyCube(const char *name, int width, int height)
{
	int i = 0;
	bool reused[6] = { false };

	for (i = 0; i < 6; i++)
	{
		if (!skyImagesData[i])
		{
			/*if (i == 4 && skyImagesData[5])
			{
				skyImagesData[i] = skyImagesData[5];
				reused[i] = true;
			}
			else if (i == 5 && skyImagesData[4])
			{
				skyImagesData[i] = skyImagesData[4];
				reused[i] = true;
			}
			else*/ if (skyImagesData[i - 1])
			{
				skyImagesData[i] = skyImagesData[i - 1];
				reused[i] = true;
			}
		}
	}

	byte *finalOrderImages[6];
	finalOrderImages[0] = skyImagesData[0];
	finalOrderImages[1] = skyImagesData[1];
	finalOrderImages[2] = skyImagesData[4];
	finalOrderImages[3] = skyImagesData[5];
	finalOrderImages[4] = skyImagesData[2];
	finalOrderImages[5] = skyImagesData[3];

	image_t *cubeImage = R_CreateCubemapFromImageDatas(name, finalOrderImages, width, height, IMGTYPE_COLORALPHA, /*IMGFLAG_NO_COMPRESSION |*/ IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, 0);

	for (i = 0; i < 6; i++)
	{
		if (skyImagesData[i] && !reused[i])
		{
			Z_Free(skyImagesData[i]);
			skyImagesData[i] = NULL;
		}
	}

	skyImageNum = -1;

	return cubeImage;
}

extern qboolean PROCEDURAL_SKY_ENABLED;

static void DefaultDaySkyParms(void) {
	static const char	*suf[6] = { "rt", "lf", "bk", "ft", "up", "dn" };
	char		pathname[MAX_IMAGE_PATH];
	int			i;
	int imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

	if (shader.noTC)
		imgFlags |= IMGFLAG_NO_COMPRESSION;

	if (r_srgb->integer)
		imgFlags |= IMGFLAG_SRGB;

	// outerbox
	for (i = 0; i < 6; i++) {
		Com_sprintf(pathname, sizeof(pathname), "%s_%s", "textures/skies/scarif", suf[i]);

		skyImageNum = i;
		shader.sky.outerbox[i] = R_FindImageFile((char *)pathname, IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE);

		if (shader.sky.outerbox[i] == NULL || shader.sky.outerbox[i] == tr.defaultImage) {
			/*if (i)
			{
			shader.sky.outerboxnight[i] = shader.sky.outerboxnight[i - 1];	//not found, so let's use the previous image
			}
			else*/
			{
				// Meh, find any working one...
				for (int s = 0; s < 6; s++)
				{
					if (shader.sky.outerbox[s] == NULL || shader.sky.outerbox[s] == tr.defaultImage) continue;
					shader.sky.outerbox[i] = shader.sky.outerbox[s];
					break;
				}
				//shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

#ifndef __GENERATED_SKY_CUBES__
	//ri->Printf(PRINT_WARNING, "DEBUG: DefaultSkyCube.\n");
	tr.skyCubeMap = R_UploadSkyCube("*skyCube", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
#else
	if (!PROCEDURAL_SKY_ENABLED)
	{
		//ri->Printf(PRINT_WARNING, "DEBUG: DefaultSkyCube.\n");
		tr.skyCubeMap = R_UploadSkyCube("*skyCube", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
	}
#endif //__GENERATED_SKY_CUBES__

	skyImageNum = -1;
}

static void DefaultNightSkyParms(void) {
	static const char	*suf[6] = { "rt", "lf", "bk", "ft", "up", "dn" };
	char		pathname[MAX_IMAGE_PATH];
	int			i;
	int imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

	if (shader.noTC)
		imgFlags |= IMGFLAG_NO_COMPRESSION;

	if (r_srgb->integer)
		imgFlags |= IMGFLAG_SRGB;

	// outerbox
	for (i = 0; i<6; i++) {
		Com_sprintf(pathname, sizeof(pathname), "%s_%s", "textures/skies/defaultNightSky", suf[i]);

		skyImageNum = i;
		shader.sky.outerboxnight[i] = R_FindImageFile((char *)pathname, IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE);

		if (shader.sky.outerboxnight[i] == NULL || shader.sky.outerboxnight[i] == tr.defaultImage) {
			/*if (i)
			{
				shader.sky.outerboxnight[i] = shader.sky.outerboxnight[i - 1];	//not found, so let's use the previous image
			}
			else*/
			{
				// Meh, find any working one...
				for (int s = 0; s < 6; s++)
				{
					if (shader.sky.outerboxnight[s] == NULL || shader.sky.outerboxnight[s] == tr.defaultImage) continue;
					shader.sky.outerboxnight[i] = shader.sky.outerboxnight[s];
					break;
				}
				//shader.sky.outerboxnight[i] = tr.defaultImage;
			}
		}
	}

#ifndef __GENERATED_SKY_CUBES__
	//ri->Printf(PRINT_WARNING, "DEBUG: DefaultSkyCubeNight.\n");
	tr.skyCubeMapNight = R_UploadSkyCube("*skyCubeNight", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
#else
	if (!PROCEDURAL_SKY_ENABLED)
	{
		tr.skyCubeMapNight = R_UploadSkyCube("*skyCubeNight", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
	}
#endif //__GENERATED_SKY_CUBES__
	skyImageNum = -1;
}

/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/

extern void R_AttachFBOTexture2D(int target, int texId, int index);

static void ParseSkyParms( const char **text ) {
	char				*token;
	static const char	*suf[6] = {"rt", "lf", "bk", "ft", "up", "dn"};
	char		pathname[MAX_IMAGE_PATH];
	int			i;
	int imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

	/*if (PROCEDURAL_SKY_ENABLED)
	{
		// UQ1: Set skies to default...
		DefaultDaySkyParms();
		DefaultNightSkyParms();
		return;
	}*/

	if (shader.noTC)
		imgFlags |= IMGFLAG_NO_COMPRESSION;

	if (r_srgb->integer)
		imgFlags |= IMGFLAG_SRGB;

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s", token, suf[i] );

			skyImageNum = i;
			shader.sky.outerbox[i] = R_FindImageFile( ( char * )pathname, IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE );

			if ( shader.sky.outerbox[i] == NULL || shader.sky.outerbox[i] == tr.defaultImage) {
				/*if (i)
				{
					shader.sky.outerbox[i] = shader.sky.outerbox[i - 1];	//not found, so let's use the previous image
				}
				else*/
				{
					// Meh, find any working one...
					for (int s = 0; s < 6; s++)
					{
						if (shader.sky.outerbox[s] == NULL || shader.sky.outerbox[s] == tr.defaultImage) continue;
						shader.sky.outerbox[i] = shader.sky.outerbox[s];
						break;
					}
					//shader.sky.outerbox[i] = tr.defaultImage;
				}
			}
		}

#ifndef __GENERATED_SKY_CUBES__
		//ri->Printf(PRINT_WARNING, "DEBUG: SkyCubeDay.\n");
		tr.skyCubeMap = R_UploadSkyCube("*skyCubeDay", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
#else
		if (!PROCEDURAL_SKY_ENABLED)
		{
			tr.skyCubeMap = R_UploadSkyCube("*skyCubeDay", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
		}
#endif //__GENERATED_SKY_CUBES__
		skyImageNum = -1;

		qboolean newSky = qfalse;

		for (i = 0; i<6; i++) {
			// Also light any <texturename>_night_up, etc if found as the night box... Will check "nightSkyParms" next, then default night sky will be used if nothing found...
			Com_sprintf(pathname, sizeof(pathname), "%s_night_%s", token, suf[i]);

			skyImageNum = i;
			image_t *newImage = R_FindImageFile((char *)pathname, IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE);

			if (newImage != NULL && newImage != tr.defaultImage)
			{
				shader.sky.outerboxnight[i] = newImage;
				newSky = qtrue;
			}
		}

		if (newSky)
		{
#ifndef __GENERATED_SKY_CUBES__
			//ri->Printf(PRINT_WARNING, "DEBUG: SkyCubeNight.\n");
			tr.skyCubeMapNight = R_UploadSkyCube("*skyCubeNight", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
#else
			if (!PROCEDURAL_SKY_ENABLED)
			{
				//ri->Printf(PRINT_WARNING, "DEBUG: SkyCubeNight.\n");
				tr.skyCubeMapNight = R_UploadSkyCube("*skyCubeNight", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
			}
#endif //__GENERATED_SKY_CUBES__
			skyImageNum = -1;
		}
		else
		{
			for (i = 0; i < 6; i++)
			{
				if (skyImagesData[i])
				{
					Z_Free(skyImagesData[i]);
					skyImagesData[i] = NULL;
				}
			}

			skyImageNum = -1;

			// UQ1: Set night sky to default...
			DefaultNightSkyParms();
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_WARNING, "WARNING: 'skyParms' missing cloudheight in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );

	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "-" ) ) {
		ri->Printf( PRINT_WARNING, "WARNING: in shader '%s' 'skyParms', innerbox is not supported!", shader.name );
	}

	shader.isSky = qtrue;
}

static void ParseNightSkyParms(const char **text) {
	char				*token;
	static const char	*suf[6] = { "rt", "lf", "bk", "ft", "up", "dn" };
	char		pathname[MAX_IMAGE_PATH];
	int			i;
	int imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

	/*if (PROCEDURAL_SKY_ENABLED)
	{
		// UQ1: Set skies to default...
		//DefaultDaySkyParms();
		//DefaultNightSkyParms();
		return;
	}*/

	if (shader.noTC)
		imgFlags |= IMGFLAG_NO_COMPRESSION;

	if (r_srgb->integer)
		imgFlags |= IMGFLAG_SRGB;

	// outerbox
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0) {
		ri->Printf(PRINT_WARNING, "WARNING: 'nightSkyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-")) {
		for (i = 0; i<6; i++) {
			Com_sprintf(pathname, sizeof(pathname), "%s_%s", token, suf[i]);
			
			skyImageNum = i;
			image_t *newImage = R_FindImageFile((char *)pathname, IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE);

			if (newImage)
			{// If found, use this, otherwise use _night_ image or default sky...
				shader.sky.outerboxnight[i] = newImage;

				if (shader.sky.outerboxnight[i] == NULL || shader.sky.outerboxnight[i] == tr.defaultImage) {
					/*if (i)
					{
						shader.sky.outerboxnight[i] = shader.sky.outerboxnight[i - 1];	//not found, so let's use the previous image
					}
					else*/
					{
						//shader.sky.outerboxnight[i] = tr.defaultImage;
						// Meh, find any working one...
						for (int s = 0; s < 6; s++)
						{
							if (shader.sky.outerboxnight[s] == NULL || shader.sky.outerboxnight[s] == tr.defaultImage) continue;
							shader.sky.outerboxnight[i] = shader.sky.outerboxnight[s];
							break;
						}
					}
				}
			}
		}

#ifndef __GENERATED_SKY_CUBES__
		//ri->Printf(PRINT_WARNING, "DEBUG: SkyCubeNightParse.\n");
		tr.skyCubeMapNight = R_UploadSkyCube("*skyCubeNight", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
#else
		if (!PROCEDURAL_SKY_ENABLED)
		{
			//ri->Printf(PRINT_WARNING, "DEBUG: SkyCubeNightParse.\n");
			tr.skyCubeMapNight = R_UploadSkyCube("*skyCubeNight", SKY_CUBE_SIZE, SKY_CUBE_SIZE);
		}
#endif //__GENERATED_SKY_CUBES__
		skyImageNum = -1;
	}

	// cloudheight
	token = COM_ParseExt(text, qfalse);

#if 0 // UQ1: Use same as day for now... Globals...
	if (token[0] == 0) {
		ri->Printf(PRINT_WARNING, "WARNING: 'nightSkyParms' missing cloudheight in shader '%s'\n", shader.name);
		return;
	}

	shader.sky.cloudHeight = atof(token);
	if (!shader.sky.cloudHeight) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords(shader.sky.cloudHeight);
#endif

	// innerbox
	token = COM_ParseExt(text, qfalse);

#if 0 // UQ1: Doesnt matter...
	if (strcmp(token, "-")) {
		ri->Printf(PRINT_WARNING, "WARNING: in shader '%s' 'nightSkyParms', innerbox is not supported!", shader.name);
	}
#endif

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( const char **text ) {
	char	*token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri->Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( !Q_stricmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !Q_stricmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !Q_stricmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	} else if ( !Q_stricmp( token, "decal" ) ) {
		shader.sort = SS_DECAL;
	} else if ( !Q_stricmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !Q_stricmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !Q_stricmp( token, "additive" ) ) {
		shader.sort = SS_BLEND1;
	} else if ( !Q_stricmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !Q_stricmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else if ( !Q_stricmp( token, "inside" ) ) {
		shader.sort = SS_INSIDE;
	} else if ( !Q_stricmp( token, "mid_inside" ) ) {
		shader.sort = SS_MID_INSIDE;
	} else if ( !Q_stricmp( token, "middle" ) ) {
		shader.sort = SS_MIDDLE;
	} else if ( !Q_stricmp( token, "mid_outside" ) ) {
		shader.sort = SS_MID_OUTSIDE;
	} else if ( !Q_stricmp( token, "outside" ) ) {
		shader.sort = SS_OUTSIDE;
	}
	else {
		shader.sort = atof( token );
	}
}

/*
=================
ParseMaterial
=================
*/
const char *materialNames[MATERIAL_LAST] =
{
	MATERIALS
};

void ParseMaterial( const char **text )
{
	char	*token;
	int		i;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: missing material in shader '%s'\n", shader.name );
		return;
	}
	for(i = 0; i < MATERIAL_LAST; i++)
	{
		if ( !Q_stricmp( token, materialNames[i] ) )
		{
			//shader.surfaceFlags |= i;
			shader.materialType = i;
			break;
		}
	}
}


// this table is also present in q3map

typedef struct infoParm_s {
	const char	*name;
	uint32_t	clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] = {
	// Game content Flags
	{ "nonsolid",		~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_NONE },		// special hack to clear solid flag
	{ "nonopaque",		~CONTENTS_OPAQUE,					SURF_NONE,			CONTENTS_NONE },		// special hack to clear opaque flag
	{ "lava",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_LAVA },		// very damaging
	{ "slime",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_SLIME },		// mildly damaging
	{ "water",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_WATER },		//
	{ "fog",			~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_FOG},			// carves surfaces entering
	{ "shotclip",		~CONTENTS_SOLID,					SURF_NONE,			CONTENTS_SHOTCLIP },	// block shots, but not people
	{ "playerclip",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_PLAYERCLIP },	// block only the player
	{ "monsterclip",	~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_MONSTERCLIP },	//
	{ "botclip",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_BOTCLIP },		// for bots
	{ "trigger",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TRIGGER },		//
	{ "nodrop",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{ "terrain",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_TERRAIN },		// use special terrain collsion
	{ "ladder",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_LADDER },		// climb up in it like water
	{ "abseil",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_ABSEIL },		// can abseil down this brush
	{ "outside",		~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_OUTSIDE },		// volume is considered to be in the outside (i.e. not indoors)
	{ "inside",			~(CONTENTS_SOLID|CONTENTS_OPAQUE),	SURF_NONE,			CONTENTS_INSIDE },		// volume is considered to be inside (i.e. indoors)

	{ "detail",			CONTENTS_ALL,						SURF_NONE,			CONTENTS_DETAIL },		// don't include in structural bsp
	{ "trans",			CONTENTS_ALL,						SURF_NONE,			CONTENTS_TRANSLUCENT },	// surface has an alpha component

	/* Game surface flags */
	{ "sky",			CONTENTS_ALL,						SURF_SKY,			CONTENTS_NONE },		// emit light from an environment map
	{ "slick",			CONTENTS_ALL,						SURF_SLICK,			CONTENTS_NONE },		//

	{ "nodamage",		CONTENTS_ALL,						SURF_NODAMAGE,		CONTENTS_NONE },		//
	{ "noimpact",		CONTENTS_ALL,						SURF_NOIMPACT,		CONTENTS_NONE },		// don't make impact explosions or marks
	{ "nomarks",		CONTENTS_ALL,						SURF_NOMARKS,		CONTENTS_NONE },		// don't make impact marks, but still explode
	{ "nodraw",			CONTENTS_ALL,						SURF_NODRAW,		CONTENTS_NONE },		// don't generate a drawsurface (or a lightmap)
	{ "nosteps",		CONTENTS_ALL,						SURF_NOSTEPS,		CONTENTS_NONE },		//
	{ "nodlight",		CONTENTS_ALL,						SURF_NODLIGHT,		CONTENTS_NONE },		// don't ever add dynamic lights
	{ "metalsteps",		CONTENTS_ALL,						SURF_METALSTEPS,	CONTENTS_NONE },		//
	{ "nomiscents",		CONTENTS_ALL,						SURF_NOMISCENTS,	CONTENTS_NONE },		// No misc ents on this surface
	{ "forcefield",		CONTENTS_ALL,						SURF_FORCEFIELD,	CONTENTS_NONE },		//
	{ "forcesight",		CONTENTS_ALL,						SURF_FORCESIGHT,	CONTENTS_NONE },		// only visible with force sight
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( const char **text ) {
	char	*token;
	int		numInfoParms = ARRAY_LEN( infoParms );
	int		i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !Q_stricmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
			shader.contentFlags &= infoParms[i].clearSolid;
			break;
		}
	}
}

/*
======================================================================================================================================
                                            Rend2 - Backward Compatibility - Material Types.
======================================================================================================================================

This does the job of backward compatibility well enough...
Sucky method, and I know it, but I don't have the time or patience to manually edit every shader in the game manually...
This is good enough!

TODO: At some point, add external overrides file...

*/

qboolean HaveSurfaceType( int materialType)
{
	switch( materialType )
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
	case MATERIAL_SAND:				// 8			// sandy beach
	case MATERIAL_CARPET:			// 27			// lush carpet
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
	case MATERIAL_ROCK:				// 23			//
	case MATERIAL_TILES:			// 26			// tiled floor
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
	case MATERIAL_POLISHEDWOOD:
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
	case MATERIAL_CANVAS:			// 22			// tent material
	case MATERIAL_MARBLE:			// 12			// marble floors
	case MATERIAL_SNOW:				// 14			// freshly laid snow
	case MATERIAL_MUD:				// 17			// wet soil
	case MATERIAL_DIRT:				// 7			// hard mud
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
	case MATERIAL_PLASTIC:			// 25			//
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
	case MATERIAL_ARMOR:			// 30			// body armor
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
	case MATERIAL_GLASS:			// 10			//
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
	case MATERIAL_PUDDLE:
	case MATERIAL_LAVA:
	case MATERIAL_TREEBARK:
	case MATERIAL_STONE:
	case MATERIAL_EFX:
	case MATERIAL_BLASTERBOLT:
	case MATERIAL_FIRE:
	case MATERIAL_SMOKE:
	case MATERIAL_MAGIC_PARTICLES:
	case MATERIAL_MAGIC_PARTICLES_TREE:
	case MATERIAL_FIREFLIES:
	case MATERIAL_PORTAL:
	case MATERIAL_MENU_BACKGROUND:
	case MATERIAL_SKYSCRAPER:
	case MATERIAL_DISTORTEDGLASS:
	case MATERIAL_DISTORTEDPUSH:
	case MATERIAL_DISTORTEDPULL:
	case MATERIAL_CLOAK:
	case MATERIAL_FORCEFIELD:
	case MATERIAL_PROCEDURALFOLIAGE:
	case MATERIAL_BIRD:
		return qtrue;
		break;
	default:
		break;
	}

	return qfalse;
}

void DebugSurfaceTypeSelection( const char *name, int materialType)
{
	if (!r_materialDebug->integer)	return; // disable debugging for now

	if (StringContainsWord(name, "gfx/") || StringContainsWord(name, "sprites/") || StringContainsWord(name, "powerups/"))
		return; // Ignore all these to reduce spam for now...

	if (StringContainsWord(name, "models/weapon"))
		return; // Ignore all these to reduce spam for now...

	//if (StringContainsWord(name, "models/player"))
	//	return; // Ignore all these to reduce spam for now...

	switch( materialType )
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_WATER.\n", name);
		break;
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SHORTGRASS.\n", name);
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_LONGGRASS.\n", name);
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SAND.\n", name);
		break;
	case MATERIAL_CARPET:			// 27			// lush carpet
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_CARPET.\n", name);
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_GRAVEL.\n", name);
		break;
	case MATERIAL_ROCK:				// 23			//
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_ROCK.\n", name);
		break;
	case MATERIAL_STONE:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_STONE.\n", name);
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_TILES.\n", name);
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SOLIDWOOD.\n", name);
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_HOLLOWWOOD.\n", name);
		break;
	case MATERIAL_POLISHEDWOOD:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_POLISHEDWOOD.\n", name);
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SOLIDMETAL.\n", name);
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_HOLLOWMETAL.\n", name);
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_DRYLEAVES.\n", name);
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_GREENLEAVES.\n", name);
		break;
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_FABRIC.\n", name);
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_CANVAS.\n", name);
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_MARBLE.\n", name);
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SNOW.\n", name);
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_MUD.\n", name);
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_DIRT.\n", name);
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_CONCRETE.\n", name);
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_FLESH.\n", name);
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_RUBBER.\n", name);
		break;
	case MATERIAL_PLASTIC:			// 25			//
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_PLASTIC.\n", name);
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_PLASTER.\n", name);
		break;
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SHATTERGLASS.\n", name);
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_ARMOR.\n", name);
		break;
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_ICE.\n", name);
		break;
	case MATERIAL_GLASS:			// 10			//
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_GLASS.\n", name);
		break;
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_BPGLASS.\n", name);
		break;
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_COMPUTER.\n", name);
		break;
	case MATERIAL_PUDDLE:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_PUDDLE.\n", name);
		break;
	case MATERIAL_LAVA:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_LAVA.\n", name);
		break;
	case MATERIAL_TREEBARK:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_TREEBARK.\n", name);
		break;
	case MATERIAL_EFX:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_EFX.\n", name);
		break;
	case MATERIAL_BLASTERBOLT:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_BLASTERBOLT.\n", name);
		break;
	case MATERIAL_FIRE:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_FIRE.\n", name);
		break;
	case MATERIAL_SMOKE:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SMOKE.\n", name);
		break;
	case MATERIAL_MAGIC_PARTICLES:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_MAGIC_PARTICLES.\n", name);
		break;
	case MATERIAL_MAGIC_PARTICLES_TREE:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_MAGIC_PARTICLES_TREE.\n", name);
		break;
	case MATERIAL_FIREFLIES:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_FIREFLIES.\n", name);
		break;
	case MATERIAL_PORTAL:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_PORTAL.\n", name);
		break;
	case MATERIAL_MENU_BACKGROUND:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_MENU_BACKGROUND.\n", name);
		break;
	case MATERIAL_SKYSCRAPER:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_SKYSCRAPER.\n", name);
		break;
	case MATERIAL_DISTORTEDGLASS:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_DISTORTEDGLASS.\n", name);
		break;
	case MATERIAL_DISTORTEDPUSH:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_DISTORTEDPUSH.\n", name);
		break;
	case MATERIAL_DISTORTEDPULL:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_DISTORTEDPULL.\n", name);
		break;
	case MATERIAL_CLOAK:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_CLOAK.\n", name);
		break;
	case MATERIAL_FORCEFIELD:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_FORCEFIELD.\n", name);
		break;
	case MATERIAL_PROCEDURALFOLIAGE:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_PROCEDURALFOLIAGE.\n", name);
		break;
	case MATERIAL_BIRD:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_BIRD.\n", name);
		break;
	default:
		ri->Printf(PRINT_WARNING, "Surface %s was set to MATERIAL_NONE.\n", name);
		break;
	}
}

qboolean StringsContainWord ( const char *heystack, const char *heystack2,  char *needle )
{
	if (StringContainsWord(heystack, needle)) return qtrue;
	if (StringContainsWord(heystack2, needle)) return qtrue;
	return qfalse;
}

qboolean IsKnownShinyMap2 ( const char *heystack )
{
	if (StringContainsWord(heystack, "/players/")) return qfalse;
	if (StringContainsWord(heystack, "/bespin/")) return qtrue;
	if (StringContainsWord(heystack, "/cloudcity/")) return qtrue;
	if (StringContainsWord(heystack, "/byss/")) return qtrue;
	if (StringContainsWord(heystack, "/cairn/")) return qtrue;
	if (StringContainsWord(heystack, "/doomgiver/")) return qtrue;
	if (StringContainsWord(heystack, "/factory/")) return qtrue;
	if (StringContainsWord(heystack, "/hoth/")) return qtrue;
	if (StringContainsWord(heystack, "/impdetention/")) return qtrue;
	if (StringContainsWord(heystack, "/imperial/")) return qtrue;
	if (StringContainsWord(heystack, "/impgarrison/")) return qtrue;
	if (StringContainsWord(heystack, "/kejim/")) return qtrue;
	if (StringContainsWord(heystack, "/nar_hideout/")) return qtrue;
	if (StringContainsWord(heystack, "/nar_streets/")) return qtrue;
	if (StringContainsWord(heystack, "/narshaddaa/")) return qtrue;
	if (StringContainsWord(heystack, "/rail/")) return qtrue;
	if (StringContainsWord(heystack, "/rooftop/")) return qtrue;
	if (StringContainsWord(heystack, "/taspir/")) return qtrue; // lots of metal... will try this
	if (StringContainsWord(heystack, "/vjun/")) return qtrue;
	if (StringContainsWord(heystack, "/wedge/")) return qtrue;

	// MB2 Maps...
	if (StringContainsWord(heystack, "/epiii_boc/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_cc/")) return qtrue;
	if (StringContainsWord(heystack, "/bespinsp/")) return qtrue;
	if (StringContainsWord(heystack, "/imm_cc/")) return qtrue;
	if (StringContainsWord(heystack, "/com_tower/")) return qtrue;
	if (StringContainsWord(heystack, "/imperial_tram/")) return qtrue;
	if (StringContainsWord(heystack, "/corellia/")) return qtrue;
	if (StringContainsWord(heystack, "/mb2_outlander/")) return qtrue;
	if (StringContainsWord(heystack, "/falcon_sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/second-deathstar/")) return qtrue;
	if (StringContainsWord(heystack, "/thedeathstar/")) return qtrue;
	if (StringContainsWord(heystack, "/casa_del_paria/")) return qtrue;
	if (StringContainsWord(heystack, "/evil3_")) return qtrue;
	if (StringContainsWord(heystack, "/hanger/")) return qtrue;
	if (StringContainsWord(heystack, "/naboo/")) return qtrue;
	if (StringContainsWord(heystack, "/shinfl/")) return qtrue;
	if (StringContainsWord(heystack, "/hangar/")) return qtrue;
	if (StringContainsWord(heystack, "/lab/")) return qtrue;
	if (StringContainsWord(heystack, "/mainhall/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/mb2_kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Mustafar/")) return qtrue; // hmm... maybe???
	if (StringContainsWord(heystack, "/mygeeto1a/")) return qtrue;
	if (StringContainsWord(heystack, "/mygeeto1c/")) return qtrue;
	if (StringContainsWord(heystack, "/ddee_hangarc/")) return qtrue;
	if (StringContainsWord(heystack, "/droidee/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_detention/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_leviathan/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_reactor/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_battle_over/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_palp/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_starship/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_utapua/")) return qtrue;
	if (StringContainsWord(heystack, "/mace_dark/")) return qtrue;
	if (StringContainsWord(heystack, "/mace_hanger/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TantIV/")) return qtrue;
	if (StringContainsWord(heystack, "/ship/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Exec/")) return qtrue;
	if (StringContainsWord(heystack, "/tantive/")) return qtrue;
	if (StringContainsWord(heystack, "/tantive1/")) return qtrue;
	if (StringContainsWord(heystack, "/MMT/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Hangar/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TFed/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TFedTOO/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TradeFed/")) return qtrue;

	// Misc Maps...
	if (StringContainsWord(heystack, "/atlantica/")) return qtrue;
	if (StringContainsWord(heystack, "/Carida/")) return qtrue;
	if (StringContainsWord(heystack, "/bunker/")) return qtrue;
	if (StringContainsWord(heystack, "/DF/")) return qtrue;
	if (StringContainsWord(heystack, "/bespinnew/")) return qtrue;
	if (StringContainsWord(heystack, "/cloudcity/")) return qtrue;
	if (StringContainsWord(heystack, "/coruscantsjc/")) return qtrue;
	if (StringContainsWord(heystack, "/ffawedge/")) return qtrue;
	if (StringContainsWord(heystack, "/jenshotel/")) return qtrue;
	if (StringContainsWord(heystack, "/CoruscantStreets/")) return qtrue;
	if (StringContainsWord(heystack, "/ctf_fighterbays/")) return qtrue;
	if (StringContainsWord(heystack, "/e3sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/mustafar_sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/fearis/")) return qtrue;
	if (StringContainsWord(heystack, "/kotor_dantooine/")) return qtrue;
	if (StringContainsWord(heystack, "/kotor_ebon_hawk/")) return qtrue;
	if (StringContainsWord(heystack, "/deltaphantom/")) return qtrue;
	if (StringContainsWord(heystack, "/pass_me_around/")) return qtrue;
	if (StringContainsWord(heystack, "/AMegaCity/")) return qtrue;
	if (StringContainsWord(heystack, "/mantell")) return qtrue;
	if (StringContainsWord(heystack, "/rcruiser/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_padme/")) return qtrue;
	if (StringContainsWord(heystack, "/Anaboo/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_imort/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_mygeeto/")) return qtrue;
	if (StringContainsWord(heystack, "/ACrimeHutt/")) return qtrue;
	if (StringContainsWord(heystack, "/ASenateBase/")) return qtrue;

	// Warzone
	if (StringContainsWord(heystack, "/impfact/")) return qtrue;
	//if (StringContainsWord(heystack, "jh3")) return qtrue;

	return qfalse;
}

extern qboolean GENERIC_MATERIALS_PREFER_SHINY;

qboolean IsKnownShinyMap ( const char *heystack )
{
	if (GENERIC_MATERIALS_PREFER_SHINY)
	{// MapInfo override for this map. Mapper said we should use shiny options.
		return qtrue;
	}

	if (IsKnownShinyMap2( heystack ))
	{
		if (r_materialDebug->integer >= 2)
			ri->Printf(PRINT_WARNING, "Surface %s is known shiny.\n", heystack);

		return qtrue;
	}

	return qfalse;
}

extern char currentMapName[128];

qboolean IsNonDetectionMap(void)
{
	//if (StringContainsWord(currentMapName, "jh3-te")) return qtrue;

	return qfalse;
}

int DetectMaterialType ( const char *name )
{
	if (IsNonDetectionMap()) return MATERIAL_NONE;

	/*switch (sh->materialType)
	{
	case MATERIAL_DISTORTEDGLASS:
		break;
	case MATERIAL_DISTORTEDPUSH:
		break;
	case MATERIAL_DISTORTEDPULL:
		break;
	case MATERIAL_CLOAK:
		break;
	case MATERIAL_FORCEFIELD:
		break;
	default:
		if (isEfxShader && sh->materialType != MATERIAL_MENU_BACKGROUND) sh->materialType = MATERIAL_EFX;
		if (sh->materialType == MATERIAL_NONE) sh->materialType = material;
		break;
	}*/

	if (StringContainsWord(name, "gfx/effects/sabers/saberTrail"))
		return MATERIAL_EFX;

	if (StringContainsWord(name, "models/warzone/birds/"))
		return MATERIAL_BIRD; // always a bird...

	if (StringContainsWord(name, "gfx/effects/force_push"))
		return MATERIAL_DISTORTEDPUSH;
	if (StringContainsWord(name, "gfx/effects/forcePush"))
		return MATERIAL_DISTORTEDPUSH;
	if (StringContainsWord(name, "gfx/water"))
		return MATERIAL_NONE;
	if (StringContainsWord(name, "gfx/atmospheric"))
		return MATERIAL_NONE;
	if (StringContainsWord(name, "warzone/plant") || StringContainsWord(name, "warzone/bushes"))
		return MATERIAL_GREENLEAVES;
	else if ((StringContainsWord(name, "yavin/tree2b") || StringContainsWord(name, "yavin/tree05") || StringContainsWord(name, "yavin/tree06"))
		&& !(StringContainsWord(name, "yavin/tree05_vines") || StringContainsWord(name, "yavin/tree06b")))
		return MATERIAL_TREEBARK;
	else if ((StringContainsWord(name, "yavin/tree08") || StringContainsWord(name, "yavin/tree09"))
		&& !(StringContainsWord(name, "yavin/tree08b") || StringContainsWord(name, "yavin/tree09_vines") || StringContainsWord(name, "yavin/tree09a") || StringContainsWord(name, "yavin/tree09b") || StringContainsWord(name, "yavin/tree09d")))
		return MATERIAL_TREEBARK;

	//
	// Special cases - where we are pretty sure we want lots of specular and reflection...
	//
	else if (StringContainsWord(name, "jetpack"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet") || StringContainsWord(name, "feather"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "/ships/") || StringContainsWord(name, "engine") || StringContainsWord(name, "mp/flag") || StringContainsWord(name, "/atat/"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "wing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "ship_"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (!StringContainsWord(name, "trainer") && StringContainsWord(name, "train"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "water") && !StringContainsWord(name, "splash") && !StringContainsWord(name, "drip") && !StringContainsWord(name, "ripple") && !StringContainsWord(name, "bubble") && !StringContainsWord(name, "woosh") && !StringContainsWord(name, "underwater") && !StringContainsWord(name, "bottom"))
	{
		return MATERIAL_WATER;
		shader.isWater = qtrue;
	}
	else if (StringContainsWord(name, "grass") || StringContainsWord(name, "foliage") || StringContainsWord(name, "yavin/ground") || StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "volcano/terrain") || StringContainsWord(name, "bay/terrain") || StringContainsWord(name, "towers/terrain") || StringContainsWord(name, "yavinassault/terrain"))
		return MATERIAL_SHORTGRASS;
	else if (StringContainsWord(name, "vj4")) // special case for vjun rock...
		return MATERIAL_ROCK;

	//
	// Player model stuff overrides
	//
	else if (StringContainsWord(name, "players") && StringContainsWord(name, "eye") || StringContainsWord(name, "goggles"))
		return MATERIAL_GLASS;
	else if (StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
		return MATERIAL_HOLLOWMETAL;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "bespin"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "players") && !StringContainsWord(name, "glass") && (StringContainsWord(name, "sithsoldier") || StringContainsWord(name, "sith_assassin") || StringContainsWord(name, "r2d2") || StringContainsWord(name, "protocol") || StringContainsWord(name, "r5d2") || StringContainsWord(name, "c3po") || StringContainsWord(name, "hk4") || StringContainsWord(name, "hk5") || StringContainsWord(name, "droid") || StringContainsWord(name, "shadowtrooper")))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "players") && StringContainsWord(name, "shadowtrooper"))
		return MATERIAL_SOLIDMETAL; // dunno about this one.. looks good as armor...
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "mandalore") || StringContainsWord(name, "mandalorian") || StringContainsWord(name, "sith_warrior/body")))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hood") || StringContainsWord(name, "robe") || StringContainsWord(name, "cloth") || StringContainsWord(name, "pants") || StringContainsWord(name, "sith_warrior/bandon_body")))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hair") || StringContainsWord(name, "chewbacca"))) // use carpet
		return MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "flesh") || StringContainsWord(name, "body") || StringContainsWord(name, "leg") || StringContainsWord(name, "hand") || StringContainsWord(name, "head") || StringContainsWord(name, "hips") || StringContainsWord(name, "torso") || StringContainsWord(name, "tentacles") || StringContainsWord(name, "face") || StringContainsWord(name, "arms") || StringContainsWord(name, "sith_warrior/head") || StringContainsWord(name, "sith_warrior/bandon_head")))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "arm") || StringContainsWord(name, "foot") || StringContainsWord(name, "neck")))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "skirt") || StringContainsWord(name, "boots") || StringContainsWord(name, "accesories") || StringContainsWord(name, "accessories") || StringContainsWord(name, "vest") || StringContainsWord(name, "holster") || StringContainsWord(name, "cap") || StringContainsWord(name, "collar")))
		return MATERIAL_FABRIC;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "_cc"))
		return MATERIAL_MARBLE;
	//
	// If player model material not found above, use defaults...
	//

	//
	// Stuff we can be pretty sure of...
	//
	else if (StringContainsWord(name, "concrete"))
		return MATERIAL_CONCRETE;
	else if (!StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && StringContainsWord(name, "models/weapon") && StringContainsWord(name, "saber") && StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
		return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
	else if (!StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && (StringContainsWord(name, "models/weapon") || StringContainsWord(name, "scope") || StringContainsWord(name, "blaster") || StringContainsWord(name, "pistol") || StringContainsWord(name, "thermal") || StringContainsWord(name, "bowcaster") || StringContainsWord(name, "cannon") || StringContainsWord(name, "saber") || StringContainsWord(name, "rifle") || StringContainsWord(name, "rocket")))
		return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
	else if (StringContainsWord(name, "metal") || StringContainsWord(name, "pipe") || StringContainsWord(name, "shaft") || StringContainsWord(name, "jetpack") || StringContainsWord(name, "antenna") || StringContainsWord(name, "xwing") || StringContainsWord(name, "tie_") || StringContainsWord(name, "raven") || StringContainsWord(name, "falcon") || StringContainsWord(name, "engine") || StringContainsWord(name, "elevator") || StringContainsWord(name, "evaporator") || StringContainsWord(name, "airpur") || StringContainsWord(name, "gonk") || StringContainsWord(name, "droid") || StringContainsWord(name, "cart") || StringContainsWord(name, "vent") || StringContainsWord(name, "tank") || StringContainsWord(name, "transformer") || StringContainsWord(name, "generator") || StringContainsWord(name, "grate") || StringContainsWord(name, "rack") || StringContainsWord(name, "mech") || StringContainsWord(name, "turbolift") || StringContainsWord(name, "tube") || StringContainsWord(name, "coil") || StringContainsWord(name, "vader_trim") || StringContainsWord(name, "newfloor_vjun") || StringContainsWord(name, "bay_beam"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "armor") || StringContainsWord(name, "armour"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "textures/byss/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "isd") && !StringContainsWord(name, "power") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "byss_switch"))
		return MATERIAL_SOLIDMETAL; // special for byss shiny
	else if (StringContainsWord(name, "textures/vjun/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "light") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "_env") && !StringContainsWord(name, "switch_off") && !StringContainsWord(name, "switch_on") && !StringContainsWord(name, "screen") && !StringContainsWord(name, "blend") && !StringContainsWord(name, "o_ground") && !StringContainsWord(name, "_onoffg") && !StringContainsWord(name, "_onoffr") && !StringContainsWord(name, "console"))
		return MATERIAL_SOLIDMETAL; // special for vjun shiny
	else if (StringContainsWord(name, "sand"))
		return MATERIAL_SAND;
	else if (StringContainsWord(name, "gravel"))
		return MATERIAL_GRAVEL;
	else if ((StringContainsWord(name, "dirt") || StringContainsWord(name, "ground")) && !StringContainsWord(name, "menus/main_background"))
		return MATERIAL_DIRT;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "stucco"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "rift") && StringContainsWord(name, "piller"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "stucco") || StringContainsWord(name, "piller") || StringContainsWord(name, "sith_jp"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "marbl") || StringContainsWord(name, "teeth"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "snow"))
		return MATERIAL_SNOW;
	else if (StringContainsWord(name, "canvas"))
		return MATERIAL_CANVAS;
	else if (StringContainsWord(name, "rock"))
		return MATERIAL_ROCK;
	else if (StringContainsWord(name, "rubber"))
		return MATERIAL_RUBBER;
	else if (StringContainsWord(name, "carpet"))
		return MATERIAL_CARPET;
	else if (StringContainsWord(name, "plaster"))
		return MATERIAL_PLASTER;
	else if (StringContainsWord(name, "computer") || StringContainsWord(name, "console") || StringContainsWord(name, "button") || StringContainsWord(name, "terminal") || StringContainsWord(name, "switch") || StringContainsWord(name, "panel") || StringContainsWord(name, "control"))
		return MATERIAL_COMPUTER;
	else if (StringContainsWord(name, "fabric"))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "tree") || StringContainsWord(name, "leaf") || StringContainsWord(name, "leaves") || StringContainsWord(name, "fern") || StringContainsWord(name, "vine"))
		return MATERIAL_GREENLEAVES;
	else if (StringContainsWord(name, "bamboo"))
		return MATERIAL_TREEBARK;
	else if (StringContainsWord(name, "wood") && !StringContainsWord(name, "street"))
		return MATERIAL_SOLIDWOOD;
	else if (StringContainsWord(name, "mud"))
		return MATERIAL_MUD;
	else if (StringContainsWord(name, "ice"))
		return MATERIAL_ICE;
	else if ((StringContainsWord(name, "grass") || StringContainsWord(name, "foliage")) && (StringContainsWord(name, "long") || StringContainsWord(name, "tall") || StringContainsWord(name, "thick")))
		return MATERIAL_LONGGRASS;
	else if (StringContainsWord(name, "grass") || StringContainsWord(name, "foliage"))
		return MATERIAL_SHORTGRASS;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "floor"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "floor"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "textures/mp/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "light") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "_env") && !StringContainsWord(name, "underside") && !StringContainsWord(name, "blend") && !StringContainsWord(name, "t_pit") && !StringContainsWord(name, "desert") && !StringContainsWord(name, "cliff"))
		return MATERIAL_SOLIDMETAL; // special for mp shiny
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "frame"))
		return MATERIAL_SOLIDMETAL;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "wall"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "wall") || StringContainsWord(name, "underside"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "door"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "door"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "ground"))
		return MATERIAL_TILES; // dunno about this one
	else if (StringContainsWord(name, "ground"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "desert"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && (StringContainsWord(name, "tile") || StringContainsWord(name, "lift")))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "tile") || StringContainsWord(name, "lift"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "glass") || StringContainsWord(name, "light") || StringContainsWord(name, "screen") || StringContainsWord(name, "lamp") || StringContainsWord(name, "crystal"))
		return MATERIAL_GLASS;
	else if (StringContainsWord(name, "flag"))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "column") || StringContainsWord(name, "stone") || StringContainsWord(name, "statue"))
		return MATERIAL_MARBLE;
	// Extra backup - backup stuff. Used when nothing better found...
	else if (StringContainsWord(name, "red") || StringContainsWord(name, "blue") || StringContainsWord(name, "yellow") || StringContainsWord(name, "white") || StringContainsWord(name, "monitor"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "yavin") && (StringContainsWord(name, "trim") || StringContainsWord(name, "step") || StringContainsWord(name, "pad")))
		return MATERIAL_STONE;
	else if (!StringContainsWord(name, "players") && (StringContainsWord(name, "coruscant") || StringContainsWord(name, "/rooftop/") || StringContainsWord(name, "/nar_") || StringContainsWord(name, "/imperial/")))
		return MATERIAL_TILES;
	else if (!StringContainsWord(name, "players") && (StringContainsWord(name, "deathstar") || StringContainsWord(name, "imperial") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "destroyer")))
		return MATERIAL_TILES;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "dantooine"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "outside"))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "out") && (StringContainsWord(name, "trim") || StringContainsWord(name, "step") || StringContainsWord(name, "pad")))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "out") && (StringContainsWord(name, "frame") || StringContainsWord(name, "wall") || StringContainsWord(name, "round") || StringContainsWord(name, "crate") || StringContainsWord(name, "trim") || StringContainsWord(name, "support") || StringContainsWord(name, "step") || StringContainsWord(name, "pad") || StringContainsWord(name, "weapon") || StringContainsWord(name, "gun")))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "frame") || StringContainsWord(name, "wall") || StringContainsWord(name, "round") || StringContainsWord(name, "crate") || StringContainsWord(name, "trim") || StringContainsWord(name, "support") || StringContainsWord(name, "step") || StringContainsWord(name, "pad") || StringContainsWord(name, "weapon") || StringContainsWord(name, "gun"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "yavin"))
		return MATERIAL_STONE; // On yavin maps, assume rock for anything else...
	else if (StringContainsWord(name, "black") || StringContainsWord(name, "boon") || StringContainsWord(name, "items") || StringContainsWord(name, "shield"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "refract") || StringContainsWord(name, "reflect"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "map_objects") || StringContainsWord(name, "key"))
		return MATERIAL_SOLIDMETAL; // hmmm, maybe... testing...
	else if (StringContainsWord(name, "rodian"))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players")) // Fall back to flesh on anything not caught above...
		return MATERIAL_FLESH;
	else if (IsKnownShinyMap(name)) // Chances are it's shiny...
		return MATERIAL_TILES;
	else
	{
		if (!StringContainsWord(name, "gfx/")
			&& !StringContainsWord(name, "hud")
			&& !StringContainsWord(name, "fire")
			&& !StringContainsWord(name, "force")
			&& !StringContainsWord(name, "explo")
			&& !StringContainsWord(name, "cursor")
			&& !(StringContainsWord(name, "sky") && !StringContainsWord(name, "skyscraper"))
			&& !StringContainsWord(name, "powerup")
			&& !StringContainsWord(name, "slider")
			&& !StringContainsWord(name, "mp/dark_")) // Dont bother reporting gfx/ or hud items...
			if (r_materialDebug->integer)
				ri->Printf(PRINT_WARNING, "Could not work out a default surface type for shader %s. It will fallback to default parallax and specular.\n", name);

		return MATERIAL_CONCRETE;// MATERIAL_NONE;
	}
}

void AssignMaterialType ( const char *name, const char *text )
{
	//ri->Printf(PRINT_WARNING, "Check material type for %s.\n", name);

	if (r_disableGfxDirEnhancement->integer
		&& (StringContainsWord(name, "gfx/"))) return;

	if (StringContainsWord(name, "gfx/2d")
		|| StringContainsWord(name, "gfx/console")
		|| StringContainsWord(name, "gfx/colors")
		|| StringContainsWord(name, "gfx/digits")
		|| StringContainsWord(name, "gfx/hud")
		|| StringContainsWord(name, "gfx/jkg")
		|| StringContainsWord(name, "gfx/menu")) return;

	if (!HaveSurfaceType(shader.materialType))
	{
		int material = DetectMaterialType( name );

		if (material)
			shader.materialType = material;
		//else
		//	shader.materialType = MATERIAL_CARPET; // Fallback to a non-shiny default...
	}
	else
	{
		if (StringContainsWord(name, "gfx/water"))
			shader.materialType = MATERIAL_NONE;
		else if (StringContainsWord(name, "gfx/atmospheric"))
			shader.materialType = MATERIAL_NONE;
		else if (StringContainsWord(name, "warzone/plant") || StringContainsWord(name, "warzone/bushes"))
			shader.materialType = MATERIAL_GREENLEAVES;
		else if ((StringContainsWord(name, "yavin/tree2b") || StringContainsWord(name, "yavin/tree05") || StringContainsWord(name, "yavin/tree06"))
				&& !(StringContainsWord(name, "yavin/tree05_vines") || StringContainsWord(name, "yavin/tree06b")))
			shader.materialType = MATERIAL_TREEBARK;
		else if ((StringContainsWord(name, "yavin/tree08") || StringContainsWord(name, "yavin/tree09"))
				&& !(StringContainsWord(name, "yavin/tree08b") || StringContainsWord(name, "yavin/tree09_vines") || StringContainsWord(name, "yavin/tree09a") || StringContainsWord(name, "yavin/tree09b") || StringContainsWord(name, "yavin/tree09d")))
			shader.materialType = MATERIAL_TREEBARK;

		//
		// Special cases - where we are pretty sure we want lots of specular and reflection... Override!
		//
		else if (StringContainsWord(name, "vj4")) // special case for vjun rock...
			shader.materialType = MATERIAL_ROCK;
		else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet"))
			shader.materialType = MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "/ships/") || StringContainsWord(name, "engine") || StringContainsWord(name, "mp/flag"))
			shader.materialType = MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "wing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_"))
			shader.materialType = MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon"))
			shader.materialType = MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate"))
			shader.materialType = MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train"))
			shader.materialType = MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
			shader.materialType = MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "ship_"))
			shader.materialType = MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "models/weapon") && StringContainsWord(name, "saber") && !StringContainsWord(name, "glow") && StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
			shader.materialType = MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
		else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
			shader.materialType = MATERIAL_ARMOR;
		else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
			shader.materialType = MATERIAL_ARMOR;
		else if (StringContainsWord(name, "grass") || (StringContainsWord(name, "foliage") && !StringContainsWord(name, "billboard")) || StringContainsWord(name, "yavin/ground") || StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "yavinassault/terrain"))
			shader.materialType = MATERIAL_SHORTGRASS;

		//
		// Player model stuff overrides
		//
		else if (StringContainsWord(name, "players") && StringContainsWord(name, "eye"))
			shader.materialType = MATERIAL_GLASS;
		else if (StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
			shader.materialType = MATERIAL_HOLLOWMETAL;
		else if (!StringContainsWord(name, "players") && StringContainsWord(name, "bespin"))
			shader.materialType = MATERIAL_MARBLE;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "sithsoldier") || StringContainsWord(name, "r2d2") || StringContainsWord(name, "protocol") || StringContainsWord(name, "r5d2") || StringContainsWord(name, "c3po")))
			shader.materialType = MATERIAL_SOLIDMETAL;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hood") || StringContainsWord(name, "robe") || StringContainsWord(name, "cloth") || StringContainsWord(name, "pants")))
			shader.materialType = MATERIAL_FABRIC;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hair") || StringContainsWord(name, "chewbacca"))) // use carpet
			shader.materialType = MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "flesh") || StringContainsWord(name, "body") || StringContainsWord(name, "leg") || StringContainsWord(name, "hand") || StringContainsWord(name, "head") || StringContainsWord(name, "hips") || StringContainsWord(name, "torso") || StringContainsWord(name, "tentacles") || StringContainsWord(name, "face") || StringContainsWord(name, "arms")))
			shader.materialType = MATERIAL_FLESH;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "arm") || StringContainsWord(name, "foot") || StringContainsWord(name, "neck")))
			shader.materialType = MATERIAL_FLESH;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "skirt") || StringContainsWord(name, "boots") || StringContainsWord(name, "accesories") || StringContainsWord(name, "accessories") || StringContainsWord(name, "vest") || StringContainsWord(name, "holster") || StringContainsWord(name, "cap") || StringContainsWord(name, "collar")))
			shader.materialType = MATERIAL_FABRIC;
		else if (!StringContainsWord(name, "players") && StringContainsWord(name, "_cc"))
			shader.materialType = MATERIAL_MARBLE;
		//
		// If player model material not found above, use defaults...
		//
	}

	if (StringContainsWord(name, "gfx/water"))
		shader.materialType = MATERIAL_NONE;
	else if (StringContainsWord(name, "gfx/atmospheric"))
		shader.materialType = MATERIAL_NONE;
	else if (StringContainsWord(name, "common/water") && !StringContainsWord(name, "splash") && !StringContainsWord(name, "drip") && !StringContainsWord(name, "ripple") && !StringContainsWord(name, "bubble") && !StringContainsWord(name, "woosh") && !StringContainsWord(name, "underwater") && !StringContainsWord(name, "bottom"))
	{
		shader.materialType = MATERIAL_WATER;
		shader.isWater = qtrue;
	}
	else if (StringContainsWord(name, "vj4"))
	{// special case for vjun rock...
		shader.materialType = MATERIAL_ROCK;
	}
	else if (shader.hasAlpha &&
		!StringContainsWord(name, "billboard") &&
		(StringContainsWord(name, "grass") || StringContainsWord(name, "foliage") || StringContainsWord(name, "yavin/ground")
		|| StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "yavinassault/terrain")
		|| StringContainsWord(name, "tree") || StringContainsWord(name, "plant") || StringContainsWord(name, "bush")
		|| StringContainsWord(name, "shrub") || StringContainsWord(name, "leaf") || StringContainsWord(name, "leaves")
		|| StringContainsWord(name, "branch") || StringContainsWord(name, "flower") || StringContainsWord(name, "weed")
		|| StringContainsWord(name, "warzone/plant") || StringContainsWord(name, "warzone/bushes")))
	{// Always greenleaves... No parallax...
#ifdef FIXME_TREE_BARK_PARALLAX
		if (StringContainsWord(name, "bark") || StringContainsWord(name, "trunk") || StringContainsWord(name, "giant_tree") || StringContainsWord(name, "vine01"))
			shader.materialType = MATERIAL_TREEBARK;
		else
#endif
			shader.materialType = MATERIAL_GREENLEAVES;

	}
	else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "trooper") || StringContainsWord(name, "medpack"))
		if (!(shader.materialType == MATERIAL_PLASTIC)) shader.materialType = MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "grass") || (StringContainsWord(name, "foliage") && !StringContainsWord(name, "billboard")) || StringContainsWord(name, "yavin/ground")
		|| StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "yavinassault/terrain"))
		if (!(shader.materialType == MATERIAL_SHORTGRASS)) shader.materialType = MATERIAL_SHORTGRASS;

	DebugSurfaceTypeSelection(name, shader.materialType);
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader( const char *name, const char **text )
{
	char *token;
	const char *begin = *text;
	int s;

	// First, init custom cubemap and specular override settings... -1.0 means not specified.
	shader.customCubeMapScale = -1.0;
	shader.customSpecularScale = -1.0;

	// Also init base glow strength to 1.0.
	shader.glowStrength = 1.0;
	shader.glowVibrancy = 0.0;

	// Also init tesselation stuff...
	shader.tesselation = qfalse;
	shader.tesselationLevel = 0.0;
	shader.tesselationAlpha = 0.0;

	// Also init emissive strengths...
	shader.emissiveRadiusScale = 1.0;
	shader.emissiveColorScale = 1.0;

	s = 0;

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' )
	{
		ri->Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );

		if ( !token[0] )
		{
			ri->Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		// end of shader definition
		if ( token[0] == '}' )
		{
			break;
		}

		// stage definition
		else if ( token[0] == '{' )
		{
			if ( s >= MAX_SHADER_STAGES ) {
				ri->Printf( PRINT_WARNING, "WARNING: too many stages in shader %s\n", shader.name );
				return qfalse;
			}

			if ( !ParseStage( &stages[s], text ) )
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;

			continue;
		}
		else if (Q_stricmp(token, "warzoneShader") == 0 || Q_stricmp(token, "warzoneEnabled") == 0 || Q_stricmp(token, "warzoneSupported") == 0)
		{// Just skip past these markers, handled in FindShader()...
			shader.warzoneEnabled = qtrue;
			SkipRestOfLine(text);
			continue;
		}
		// If this shader is indoors
		else if (Q_stricmp(token, "indoor") == 0 || Q_stricmp(token, "inside") == 0)
		{
			shader.isIndoor = qtrue;
			SkipRestOfLine(text);
			continue;
		}
		else if (Q_stricmp(token, "vines") == 0 || Q_stricmp(token, "addVines") == 0)
		{
			shader.isVines = qtrue;
			SkipRestOfLine(text);
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// material deprecated as of 11 Jan 01
		// material undeprecated as of 7 May 01 - q3map_material deprecated
		else if ( !Q_stricmp( token, "material" ) || !Q_stricmp( token, "q3map_material" ) )
		{
			ParseMaterial( text );
			continue;
		}
		// sun parms
		else if ( !Q_stricmp( token, "sun" ) || !Q_stricmp( token, "q3map_sun" ) || !Q_stricmp( token, "q3map_sunExt" ) || !Q_stricmp( token, "q3gl2_sun" ) ) {
			float	a, b;
			qboolean isGL2Sun = qfalse;

			if (!Q_stricmp( token, "q3gl2_sun" ) && r_sunlightMode->integer >= 2 )
			{
				isGL2Sun = qtrue;
				tr.sunShadows = qtrue;
			}

			token = COM_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );

			VectorNormalize( tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt( text, qfalse );
			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );

			if (isGL2Sun)
			{
				token = COM_ParseExt( text, qfalse );
				tr.mapLightScale = atof(token);

				token = COM_ParseExt( text, qfalse );
				tr.sunShadowScale = atof(token);

				if (tr.sunShadowScale < 0.0f)
				{
					ri->Printf (PRINT_WARNING, "WARNING: q3gl2_sun's 'shadow scale' value must be between 0 and 1. Clamping to 0.0.\n");
					tr.sunShadowScale = 0.0f;
				}
				else if (tr.sunShadowScale > 1.0f)
				{
					ri->Printf (PRINT_WARNING, "WARNING: q3gl2_sun's 'shadow scale' value must be between 0 and 1. Clamping to 1.0.\n");
					tr.sunShadowScale = 1.0f;
				}
			}

			SkipRestOfLine( text );
			continue;
		}
		// tonemap parms
		else if ( !Q_stricmp( token, "q3gl2_tonemap" ) ) {
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.toneMinAvgMaxLevel[2] = atof( token );

			token = COM_ParseExt( text, qfalse );
			tr.autoExposureMinMax[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.autoExposureMinMax[1] = atof( token );

			SkipRestOfLine( text );
			continue;
		}
		else if (!Q_stricmp(token, "warzoneVextexSplat"))
		{
			shader.warzoneVextexSplat = qtrue;
			SkipRestOfLine(text);
			continue;
		}
		// q3map_surfacelight deprecated as of 16 Jul 01
		else if ( !Q_stricmp( token, "surfacelight" ) || !Q_stricmp( token, "q3map_surfacelight" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "lightColor" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "deformvertexes" ) || !Q_stricmp( token, "deform" ) ) {
			ParseDeform( text );
			continue;
		}
		else if ( !Q_stricmp( token, "tesssize" ) ) {
			SkipRestOfLine( text );
			continue;
		}
		else if (Q_stricmp(token, "tesselation") == 0 || Q_stricmp(token, "tesselate") == 0)
		{
			shader.tesselation = qtrue;
			continue;
		}
		else if (Q_stricmp(token, "tesselationLevel") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for tesselationLevel exponent in shader '%s'\n", shader.name);
				shader.tesselationLevel = 1.0;
				continue;
			}

			shader.tesselationLevel = atof(token);
			continue;
		}
		else if (Q_stricmp(token, "tesselationAlpha") == 0)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parameter for tesselationLevel exponent in shader '%s'\n", shader.name);
				shader.tesselationAlpha = 1.0;
				continue;
			}

			shader.tesselationAlpha = atof(token);
			continue;
		}
		else if ( !Q_stricmp( token, "clampTime" ) ) {
			token = COM_ParseExt( text, qfalse );
			if (token[0]) {
				shader.clampTime = atof(token);
			}
			continue;
		}
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmaps" ) )
		{
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "noglfog" ) )
		{
			//shader.fogPass = FP_NONE;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "noTC" ) )
		{
			shader.noTC = qtrue;
			continue;
		}
		//
		// Detail maps are TC specific...
		//
		else if (!Q_stricmp(token, "detailMapFromTC"))
		{
			shader.detailMapFromTC = qtrue;
			continue;
		}
		//
		// Detail maps are TC specific...
		//
		else if (!Q_stricmp(token, "detailMapFromWorld"))
		{
			shader.detailMapFromWorld = qtrue;
			continue;
		}
		else if (!Q_stricmp(token, "glowStrength"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'glowStrength' keyword in shader '%s'\n", shader.name);
				shader.glowStrength = 1.0;
				continue;
			}
			shader.glowStrength = atof(token);
			continue;
		}
		else if (!Q_stricmp(token, "glowVibrancy"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'glowVibrancy' keyword in shader '%s'\n", shader.name);
				shader.glowVibrancy = 0.0;
				continue;
			}
			shader.glowVibrancy = atof(token);
			continue;
		}
		else if (!Q_stricmp(token, "emissiveRadiusScale"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'emissiveRadiusScale' keyword in shader '%s'\n", shader.name);
				shader.emissiveRadiusScale = 1.0;
				continue;
			}
			shader.emissiveRadiusScale = atof(token);
			continue;
		}
		else if (!Q_stricmp(token, "emissiveColorScale"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'emissiveColorScale' keyword in shader '%s'\n", shader.name);
				shader.emissiveColorScale = 1.0;
				continue;
			}
			shader.emissiveColorScale = atof(token);
			continue;
		}
		//
		// Don't like the warzone material default for the reflectiveness? Use this to override material default for this whole shader...
		//
		else if (!Q_stricmp(token, "customCubeMapScale"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'customCubeMapScale' keyword in shader '%s'\n", shader.name);
				shader.customCubeMapScale = -1.0;
				continue;
			}
			shader.customCubeMapScale = atof(token);
			continue;
		}
		//
		// Don't like the warzone material default for the shinyness? Use this to override material default for this whole shader...
		//
		else if (!Q_stricmp(token, "customSpecularScale"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				ri->Printf(PRINT_WARNING, "WARNING: missing parm for 'customSpecularScale' keyword in shader '%s'\n", shader.name);
				shader.customSpecularScale = -1.0;
				continue;
			}
			shader.customSpecularScale = atof(token);
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) )
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) )
		{
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !Q_stricmp(token, "portal") )
		{
			shader.sort = SS_PORTAL;
			shader.isPortal = qtrue;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) )
		{
			ParseSkyParms( text );
			continue;
		}
		else if (!Q_stricmp(token, "nightskyparms"))
		{
			ParseNightSkyParms(text);
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if ( !Q_stricmp(token, "light") )
		{
			COM_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull") )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri->Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) )
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				ri->Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) )
		{
			ParseSort( text );
			continue;
		}
		else
		{
			ri->Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// UQ1: If we do not have any material type for this shader, try to guess as a backup (for parallax and specular settings)...
	//

	if (!shader.isSky)
		AssignMaterialType(name, *text);

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG ) ) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	// The basejka rocket lock wedge shader uses the incorrect blending mode.
	// It only worked because the shader state was not being set, and relied
	// on previous state to be multiplied by alpha. Since fixing RB_RotatePic,
	// the shader needs to be fixed here to render correctly.
	//
	// We match against the retail version of gfx/2d/wedge by calculating the
	// hash value of the shader text, and comparing it against a precalculated
	// value.
	uint32_t shaderHash = generateHashValueForText(begin, *text - begin);
	if (shaderHash == RETAIL_ROCKET_WEDGE_SHADER_HASH &&
		 Q_stricmp(shader.name, "gfx/2d/wedge") == 0)
	{
		stages[0].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
		stages[0].stateBits |= GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}

	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void )
{
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	//
	// see if this should go into the sky path
	//
	if ( shader.isSky )
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		return;
	}
}

/*
===================
ComputeVertexAttribs

Check which vertex attributes we only need, so we
don't need to submit/copy all of them.
===================
*/
static void ComputeVertexAttribs(void)
{
	int i, stage;

	// dlights always need ATTR_NORMAL
	shader.vertexAttribs = ATTR_POSITION | ATTR_NORMAL;

	// portals always need normals, for SurfIsOffscreen()
	if (shader.isPortal)
	{
		shader.vertexAttribs |= ATTR_NORMAL;
	}

	if (shader.defaultShader)
	{
		shader.vertexAttribs |= ATTR_TEXCOORD0;
		return;
	}

	if(shader.numDeforms)
	{
		for ( i = 0; i < shader.numDeforms; i++)
		{
			deformStage_t  *ds = &shader.deforms[i];

			switch (ds->deformation)
			{
				case DEFORM_BULGE:
					shader.vertexAttribs |= ATTR_NORMAL | ATTR_TEXCOORD0;
					break;

				case DEFORM_AUTOSPRITE:
					shader.vertexAttribs |= ATTR_NORMAL | ATTR_COLOR;
					break;

				case DEFORM_WAVE:
				case DEFORM_NORMALS:
				case DEFORM_TEXT0:
				case DEFORM_TEXT1:
				case DEFORM_TEXT2:
				case DEFORM_TEXT3:
				case DEFORM_TEXT4:
				case DEFORM_TEXT5:
				case DEFORM_TEXT6:
				case DEFORM_TEXT7:
					shader.vertexAttribs |= ATTR_NORMAL;
					break;

				default:
				case DEFORM_NONE:
				case DEFORM_MOVE:
				case DEFORM_PROJECTION_SHADOW:
				case DEFORM_AUTOSPRITE2:
					break;
			}
		}
	}

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active )
		{
			break;
		}

		shader.vertexAttribs |= ATTR_NORMAL;

		for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
		{
			if ( pStage->bundle[i].image[0] == 0 )
			{
				continue;
			}

			switch(pStage->bundle[i].tcGen)
			{
				case TCGEN_TEXTURE:
					shader.vertexAttribs |= ATTR_TEXCOORD0;
					break;
				case TCGEN_LIGHTMAP:
				case TCGEN_LIGHTMAP1:
				case TCGEN_LIGHTMAP2:
				case TCGEN_LIGHTMAP3:
					shader.vertexAttribs |= ATTR_TEXCOORD1;
					break;
				case TCGEN_ENVIRONMENT_MAPPED:
					shader.vertexAttribs |= ATTR_NORMAL;
					break;

				default:
					break;
			}
		}

		switch(pStage->rgbGen)
		{
			case CGEN_EXACT_VERTEX:
			case CGEN_VERTEX:
			case CGEN_EXACT_VERTEX_LIT:
			case CGEN_VERTEX_LIT:
			case CGEN_ONE_MINUS_VERTEX:
				shader.vertexAttribs |= ATTR_COLOR;
				break;

			case CGEN_LIGHTING_DIFFUSE:
			case CGEN_LIGHTING_DIFFUSE_ENTITY:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			case CGEN_LIGHTING_WARZONE:
				shader.vertexAttribs |= ATTR_COLOR;
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			default:
				break;
		}

		switch(pStage->alphaGen)
		{
			case AGEN_LIGHTING_SPECULAR:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			case AGEN_VERTEX:
			case AGEN_ONE_MINUS_VERTEX:
				shader.vertexAttribs |= ATTR_COLOR;
				break;

			default:
				break;
		}
	}
}

void StripCrap( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '_'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot-in+1 ? destsize : dot-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}

qboolean R_TextureFileExists(char *name)
{
	if (!name || !name[0] || name[0] == '\0' || strlen(name) <= 1) return qfalse;

	char texName[MAX_IMAGE_PATH] = { 0 };
	COM_StripExtension(name, texName, MAX_IMAGE_PATH);
	sprintf(texName, "%s.jpg", name);

	//ri->Printf(PRINT_WARNING, "trying: %s.\n", name);
	
	if (ri->FS_FileExists(texName))
	{
		//ri->Printf(PRINT_WARNING, "found: %s.\n", texName);
		return qtrue;
	}

	memset(&texName, 0, sizeof(char) * MAX_IMAGE_PATH);
	COM_StripExtension(name, texName, MAX_IMAGE_PATH);
	sprintf(texName, "%s.tga", name);

	if (ri->FS_FileExists(texName))
	{
		//ri->Printf(PRINT_WARNING, "found: %s.\n", texName);
		return qtrue;
	}

	memset(&texName, 0, sizeof(char) * MAX_IMAGE_PATH);
	COM_StripExtension(name, texName, MAX_IMAGE_PATH);
	sprintf(texName, "%s.png", name);

	if (ri->FS_FileExists(texName))
	{
		//ri->Printf(PRINT_WARNING, "found: %s.\n", texName);
		return qtrue;
	}

	return qfalse;
}

static void CollapseStagesToLightall(shaderStage_t *diffuse, shaderStage_t *normal, shaderStage_t *specular, shaderStage_t *lightmap, qboolean parallax, qboolean tcgen)
{
	int defs = 0;
	qboolean checkNormals = qtrue;
	qboolean checkSplats = (r_splatMapping->integer /*&& !r_lowVram->integer*/) ? qtrue : qfalse;

	if (shader.isPortal || shader.isSky || diffuse->glow || r_normalMapping->integer <= 0)
	{
		checkNormals = qfalse;
	}

	if (diffuse->bundle[TB_DIFFUSEMAP].image[0] 
		&& (StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "gfx/")
			|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "menu")))
	{// Ignore splat checks on menu and gfx dirs...
		checkSplats = qfalse;
		checkNormals = qfalse;
	}

	//ri->Printf(PRINT_ALL, "shader %s has diffuse %s", shader.name, diffuse->bundle[0].image[0]->imgName);

#ifdef __DEFERRED_IMAGE_LOADING__
	if (r_normalMapping->integer >= 2)
	{// Guess we will need to load these, for the rest to generate... *sigh*
		if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
			&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->deferredLoad)
		{// Load the actual image file...
			diffuse->bundle[TB_DIFFUSEMAP].image[0] = R_LoadDeferredImage(diffuse->bundle[TB_DIFFUSEMAP].image[0]);
		}
	}
#endif //__DEFERRED_IMAGE_LOADING__

	// reuse diffuse, mark others inactive
	diffuse->type = ST_GLSL;

	if (!diffuse->isFoliageChecked)
	{// Skip the string checks...
		switch( shader.materialType )
		{// Switch to avoid doing string checks on everything else...
		case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
			if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& (StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "foliage/") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "foliages/")))
			{
				diffuse->isFoliage = true;
			}
			else if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& (StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone/plant/") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone/plants/")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone/bushes/")))
			{
				diffuse->isFoliage = true;
			}
			else if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& !StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "trunk")
				&& !StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "bark")
				&& !StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "giant_tree")
				&& (StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/tree") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "trees")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/grass")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/tree_leaves")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/tree1")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "yavin/vine")))
			{
				diffuse->isFoliage = true;
			}
			else if (diffuse->bundle[TB_DIFFUSEMAP].image[0]
				&& StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone/deadtree")
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "warzone\\deadtree"))
			{
				if (!(StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "bark") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "trunk") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "giant_tree") 
					|| StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "vine01")))
					diffuse->isFoliage = true;
			}
			break;
		default:
			diffuse->isFoliage = false;
			break;
		}

		diffuse->isFoliageChecked = qtrue;
	}

	if (tr.numLightmaps > 0)
	{
		if (lightmap)
		{
			//ri->Printf(PRINT_ALL, ", lightmap");
			diffuse->bundle[TB_LIGHTMAP] = lightmap->bundle[0];
			defs |= LIGHTDEF_USE_LIGHTMAP;
		}
		else
		{
			diffuse->bundle[TB_LIGHTMAP] = diffuse->bundle[TB_DIFFUSEMAP];
			diffuse->bundle[TB_LIGHTMAP].image[0] = tr.whiteImage;
			defs |= LIGHTDEF_USE_LIGHTMAP;
		}
	}

	if (r_deluxeMapping->integer && tr.worldDeluxeMapping && lightmap)
	{
		//ri->Printf(PRINT_ALL, ", deluxemap");
		diffuse->bundle[TB_DELUXEMAP] = lightmap->bundle[0];
		diffuse->bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex[0]];
	}

	// UQ1: Can't we do all this in one stage ffs???
	if (r_normalMapping->integer >= 2 && r_normalMappingReal->integer && checkNormals)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		diffuse->hasNormalMap = false;

		if (diffuse->bundle[TB_NORMALMAP].image[0] && normal->bundle[TB_NORMALMAP].image[0] != tr.whiteImage)
		{
			if (diffuse->normalScale[0] == 0 && diffuse->normalScale[1] == 0 && diffuse->normalScale[2] == 0)
				VectorSet4(diffuse->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
			diffuse->hasNormalMap = true;
		}
		else if (normal && normal->bundle[0].image[0] && normal->bundle[0].image[0] != tr.whiteImage)
		{
			//ri->Printf(PRINT_ALL, ", normalmap %s", normal->bundle[0].image[0]->imgName);
			diffuse->bundle[TB_NORMALMAP] = normal->bundle[0];

			VectorCopy4(normal->normalScale, diffuse->normalScale);
			diffuse->hasNormalMap = true;
		}
		else if (r_normalMappingReal->integer)
		{
			char normalName[MAX_IMAGE_PATH];
			image_t *normalImg = NULL;
			int normalFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB)) | IMGFLAG_NOLIGHTSCALE;

			COM_StripExtension( diffuseImg->imgName, normalName, sizeof( normalName ) );
			Q_strcat( normalName, sizeof( normalName ), "_n" );

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(normalName) || R_TIL_TextureFileExists(normalName))
			{
				normalImg = R_DeferImageLoad(normalName, IMGTYPE_NORMAL, normalFlags);
			}
			else
			{
				normalImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(normalName) || R_TIL_TextureFileExists(normalName))
			{
				normalImg = R_FindImageFile(normalName, IMGTYPE_NORMAL, normalFlags);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (normalImg)
			{
				diffuse->bundle[TB_NORMALMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_NORMALMAP].numImageAnimations = 0;
				diffuse->bundle[TB_NORMALMAP].image[0] = normalImg;

				VectorSet4(diffuse->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);

				diffuse->hasNormalMap = true;
			}
#ifndef __DEFERRED_IMAGE_LOADING__
			else
			{// Generate one...
				if (!shader.isPortal
					&& !shader.isSky
					&& !diffuse->glow
					&& (!diffuse->bundle[TB_NORMALMAP].image[0] || diffuse->bundle[TB_NORMALMAP].image[0] == tr.whiteImage)
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0]
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '*'
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '$'
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '_'
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName[0] != '!'
					&& !(diffuse->bundle[TB_DIFFUSEMAP].image[0]->flags & IMGFLAG_CUBEMAP)
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_NORMAL
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_SPECULAR
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_OVERLAY
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_STEEPMAP
					&& diffuse->bundle[TB_DIFFUSEMAP].image[0]->type != IMGTYPE_WATER_EDGE_MAP

					// gfx dirs can be exempted I guess...
					&& !(r_disableGfxDirEnhancement->integer && StringContainsWord(diffuse->bundle[TB_DIFFUSEMAP].image[0]->imgName, "gfx/")))
				{
					normalImg = R_CreateNormalMapGLSL( normalName, NULL, diffuse->bundle[TB_DIFFUSEMAP].image[0]->width, diffuse->bundle[TB_DIFFUSEMAP].image[0]->height, diffuse->bundle[TB_DIFFUSEMAP].image[0]->flags, diffuse->bundle[TB_DIFFUSEMAP].image[0] );

					if (normalImg)
					{
						diffuse->bundle[TB_NORMALMAP] = diffuse->bundle[0];
						diffuse->bundle[TB_NORMALMAP].numImageAnimations = 0;
						diffuse->bundle[TB_NORMALMAP].image[0] = normalImg;

						if (diffuse->bundle[TB_NORMALMAP].image[0] && diffuse->bundle[TB_NORMALMAP].image[0] != tr.whiteImage) 
							diffuse->hasNormalMap = true;
						else
							diffuse->hasNormalMap = false;

						if (diffuse->normalScale[0] == 0 && diffuse->normalScale[1] == 0 && diffuse->normalScale[2] == 0)
							VectorSet4(diffuse->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value);
					}
				}
				else
				{
					diffuse->hasNormalMap = false;
				}
			}
#endif //__DEFERRED_IMAGE_LOADING__
		}
	}

	if (r_specularMapping->integer && checkNormals)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		if (diffuse->bundle[TB_SPECULARMAP].image[0])
		{// Got one...
			diffuse->bundle[TB_SPECULARMAP] = specular->bundle[0];
			if (specular) VectorCopy4(specular->specularScale, diffuse->specularScale);
			diffuse->hasSpecularMap = true;
		}
		else
		{// Check if we can load one...
			char specularName[MAX_IMAGE_PATH];
			char specularName2[MAX_IMAGE_PATH];
			image_t *specularImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB)) | IMGFLAG_NOLIGHTSCALE;

			COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
			StripCrap( specularName, specularName2, sizeof(specularName));
			Q_strcat( specularName2, sizeof( specularName2 ), "_s" );

			diffuse->hasSpecularMap = false;

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_DeferImageLoad(specularName2, IMGTYPE_SPECULAR, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				COM_StripExtension(diffuseImg->imgName, specularName, sizeof(specularName));
				StripCrap(specularName, specularName2, sizeof(specularName));
				Q_strcat(specularName2, sizeof(specularName2), "_spec");

				if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
				{
					specularImg = R_DeferImageLoad(specularName2, IMGTYPE_SPECULAR, specularFlags | IMGFLAG_MIPMAP);
				}
				else
				{
					specularImg = NULL;
				}
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_FindImageFile(specularName2, IMGTYPE_SPECULAR, specularFlags | IMGFLAG_MIPMAP);
			}
			
			if (!specularImg)
			{
				COM_StripExtension(diffuseImg->imgName, specularName, sizeof(specularName));
				StripCrap(specularName, specularName2, sizeof(specularName));
				Q_strcat(specularName2, sizeof(specularName2), "_spec");

				if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
				{
					specularImg = R_FindImageFile(specularName2, IMGTYPE_SPECULAR, specularFlags | IMGFLAG_MIPMAP);
				}
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (specularImg)
			{
				////if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded specular map %s.\n", specularName2);
				diffuse->bundle[TB_SPECULARMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_SPECULARMAP].numImageAnimations = 0;
				diffuse->bundle[TB_SPECULARMAP].image[0] = specularImg;
				//if (!specular) specular = diffuse;
				if (specular) VectorCopy4(specular->specularScale, diffuse->specularScale);
				diffuse->hasSpecularMap = true;
			}
		}
	}

	if (r_splatMapping->integer && checkSplats)
	{
		image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

		//
		// SteepMap
		//
		{// Check if we can load one...
			char specularName[MAX_IMAGE_PATH];
			char specularName2[MAX_IMAGE_PATH];
			image_t *specularImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension(diffuseImg->imgName, specularName, sizeof(specularName));
			StripCrap(specularName, specularName2, sizeof(specularName));
			Q_strcat(specularName2, sizeof(specularName2), "_steep");

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_DeferImageLoad(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				specularImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_FindImageFile(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (specularImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded steep map %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_STEEPMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_STEEPMAP].numImageAnimations = 0;
				diffuse->bundle[TB_STEEPMAP].image[0] = specularImg;
			}
		}

		//
		// SteepMap1
		//
		{// Check if we can load one...
			char specularName[MAX_IMAGE_PATH];
			char specularName2[MAX_IMAGE_PATH];
			image_t *specularImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension(diffuseImg->imgName, specularName, sizeof(specularName));
			StripCrap(specularName, specularName2, sizeof(specularName));
			Q_strcat(specularName2, sizeof(specularName2), "_steep1");

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_DeferImageLoad(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				specularImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_FindImageFile(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (specularImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded steep1 map %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_STEEPMAP1] = diffuse->bundle[0];
				diffuse->bundle[TB_STEEPMAP1].numImageAnimations = 0;
				diffuse->bundle[TB_STEEPMAP1].image[0] = specularImg;
			}
		}

		//
		// SteepMap2
		//
		{// Check if we can load one...
			char specularName[MAX_IMAGE_PATH];
			char specularName2[MAX_IMAGE_PATH];
			image_t *specularImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension(diffuseImg->imgName, specularName, sizeof(specularName));
			StripCrap(specularName, specularName2, sizeof(specularName));
			Q_strcat(specularName2, sizeof(specularName2), "_steep2");

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_DeferImageLoad(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				specularImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_FindImageFile(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (specularImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded steep2 map %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_STEEPMAP2] = diffuse->bundle[0];
				diffuse->bundle[TB_STEEPMAP2].numImageAnimations = 0;
				diffuse->bundle[TB_STEEPMAP2].image[0] = specularImg;
			}
		}

		//
		// SteepMap3
		//
		{// Check if we can load one...
			char specularName[MAX_IMAGE_PATH];
			char specularName2[MAX_IMAGE_PATH];
			image_t *specularImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension(diffuseImg->imgName, specularName, sizeof(specularName));
			StripCrap(specularName, specularName2, sizeof(specularName));
			Q_strcat(specularName2, sizeof(specularName2), "_steep3");

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_DeferImageLoad(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				specularImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_FindImageFile(specularName2, IMGTYPE_STEEPMAP, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (specularImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded steep3 map %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_STEEPMAP3] = diffuse->bundle[0];
				diffuse->bundle[TB_STEEPMAP3].numImageAnimations = 0;
				diffuse->bundle[TB_STEEPMAP3].image[0] = specularImg;
			}
		}

		//
		// waterEdgeMap
		//
		{// Check if we can load one...
			char specularName[MAX_IMAGE_PATH];
			char specularName2[MAX_IMAGE_PATH];
			image_t *specularImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, specularName, sizeof( specularName ) );
			StripCrap( specularName, specularName2, sizeof(specularName));
			Q_strcat( specularName2, sizeof( specularName2 ), "_riverbed" );

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_DeferImageLoad(specularName2, IMGTYPE_WATER_EDGE_MAP, specularFlags);
			}
			else
			{
				specularImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_FindImageFile(specularName2, IMGTYPE_WATER_EDGE_MAP, specularFlags | IMGFLAG_MIPMAP);
			}

			if (!specularImg)
			{
				memset(specularName, 0, sizeof(char)*MAX_IMAGE_PATH);
				COM_StripExtension(diffuseImg->imgName, specularName2, sizeof(specularName2));
				StripCrap(specularName, specularName2, sizeof(specularName2));
				Q_strcat(specularName2, sizeof(specularName2), "_waterEdge");

				specularImg = R_FindImageFile(specularName2, IMGTYPE_WATER_EDGE_MAP, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (specularImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded steep map2 %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_WATER_EDGE_MAP] = diffuse->bundle[0];
				diffuse->bundle[TB_WATER_EDGE_MAP].numImageAnimations = 0;
				diffuse->bundle[TB_WATER_EDGE_MAP].image[0] = specularImg;
			}
		}

		//
		// RoofMap
		//
		{// Check if we can load one...
			char specularName[MAX_IMAGE_PATH];
			char specularName2[MAX_IMAGE_PATH];
			image_t *specularImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension(diffuseImg->imgName, specularName, sizeof(specularName));
			StripCrap(specularName, specularName2, sizeof(specularName));
			Q_strcat(specularName2, sizeof(specularName2), "_roof");

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_DeferImageLoad(specularName2, IMGTYPE_ROOFMAP, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				specularImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(specularName2) || R_TIL_TextureFileExists(specularName2))
			{
				specularImg = R_FindImageFile(specularName2, IMGTYPE_ROOFMAP, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (specularImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded roof map %s [%i x %i].\n", specularName2, specularImg->width, specularImg->height);
				diffuse->bundle[TB_ROOFMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_ROOFMAP].numImageAnimations = 0;
				diffuse->bundle[TB_ROOFMAP].image[0] = specularImg;
			}
		}
	
		//
		// SplatControl
		//
		{
			// Splat Control Map - We will allow each shader to have it's own map-wide spatter control image...
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_IMAGE_PATH];
			char splatName2[MAX_IMAGE_PATH];
			image_t *splatImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) | IMGFLAG_NOLIGHTSCALE;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat" );

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(specularName2))
			{
				splatImg = R_DeferImageLoad(splatName2, IMGTYPE_SPLATCONTROLMAP, specularFlags);
			}
			else
			{
				splatImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATCONTROLMAP, specularFlags);
			}

			if (!splatImg)
			{
				memset(splatName, 0, sizeof(char)*MAX_IMAGE_PATH);
				COM_StripExtension(diffuseImg->imgName, splatName, sizeof(splatName));
				StripCrap(splatName, splatName2, sizeof(splatName));
				Q_strcat(splatName2, sizeof(splatName2), "_splatControl");

				if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
				{
					splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATCONTROLMAP, specularFlags);
				}
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (splatImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat control map %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATCONTROLMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATCONTROLMAP].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATCONTROLMAP].image[0] = splatImg;
			}
			else
			{
				diffuse->bundle[TB_SPLATCONTROLMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATCONTROLMAP].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATCONTROLMAP].image[0] = NULL;
			}
		}

		//
		// SplatMap1
		//
		{
			// Splat Map #1
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_IMAGE_PATH];
			char splatName2[MAX_IMAGE_PATH];
			image_t *splatImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat1" );

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_DeferImageLoad(splatName2, IMGTYPE_SPLATMAP1, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				splatImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATMAP1, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (splatImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat map1 %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATMAP1] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP1].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP1].image[0] = splatImg;
			}
			else
			{
				diffuse->bundle[TB_SPLATMAP1] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP1].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP1].image[0] = NULL;
				diffuse->bundle[TB_SPLATMAP1].image[1] = NULL;
			}
		}

		//
		// SplatMap2
		//
		{
			// Splat Map #2
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_IMAGE_PATH];
			char splatName2[MAX_IMAGE_PATH];
			image_t *splatImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat2" );

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_DeferImageLoad(splatName2, IMGTYPE_SPLATMAP2, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				splatImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATMAP2, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (splatImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat map2 %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATMAP2] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP2].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP2].image[0] = splatImg;
			}
			else
			{
				diffuse->bundle[TB_SPLATMAP2] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP2].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP2].image[0] = NULL;
				diffuse->bundle[TB_SPLATMAP2].image[1] = NULL;
			}
		}

		//
		// SplatMap3
		//
		{
			// Splat Map #3
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_IMAGE_PATH];
			char splatName2[MAX_IMAGE_PATH];
			image_t *splatImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension( diffuseImg->imgName, splatName, sizeof( splatName ) );
			StripCrap( splatName, splatName2, sizeof(splatName));
			Q_strcat( splatName2, sizeof( splatName2 ), "_splat3" );

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_DeferImageLoad(splatName2, IMGTYPE_SPLATMAP3, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				splatImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_FindImageFile(splatName2, IMGTYPE_SPLATMAP3, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (splatImg)
			{
				if (r_debugSplatMaps->integer) ri->Printf(PRINT_WARNING, "+++++++++++++++ Loaded splat map3 %s [%i x %i].\n", splatName2, splatImg->width, splatImg->height);
				diffuse->bundle[TB_SPLATMAP3] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP3].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP3].image[0] = splatImg;
			}
			else
			{
				diffuse->bundle[TB_SPLATMAP3] = diffuse->bundle[0];
				diffuse->bundle[TB_SPLATMAP3].numImageAnimations = 0;
				diffuse->bundle[TB_SPLATMAP3].image[0] = NULL;
				diffuse->bundle[TB_SPLATMAP3].image[1] = NULL;
			}
		}

#if 0
		//
		// DetailMap
		//
		{
			// Detail Map
			image_t *diffuseImg = diffuse->bundle[TB_DIFFUSEMAP].image[0];

			char splatName[MAX_IMAGE_PATH];
			char splatName2[MAX_IMAGE_PATH];
			image_t *splatImg = NULL;
			int specularFlags = (diffuseImg->flags & ~(IMGFLAG_GENNORMALMAP | IMGFLAG_SRGB | IMGFLAG_CLAMPTOEDGE)) /*| IMGFLAG_NOLIGHTSCALE*/;

			COM_StripExtension(diffuseImg->imgName, splatName, sizeof(splatName));
			StripCrap(splatName, splatName2, sizeof(splatName));
			Q_strcat(splatName2, sizeof(splatName2), "_detail");

#ifdef __DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_DeferImageLoad(splatName2, IMGTYPE_DETAILMAP, specularFlags | IMGFLAG_MIPMAP);
			}
			else
			{
				splatImg = NULL;
			}
#else //!__DEFERRED_IMAGE_LOADING__
			if (R_TextureFileExists(splatName2) || R_TIL_TextureFileExists(splatName2))
			{
				splatImg = R_FindImageFile(splatName2, IMGTYPE_DETAILMAP, specularFlags | IMGFLAG_MIPMAP);
			}
#endif //__DEFERRED_IMAGE_LOADING__

			if (splatImg)
			{
				diffuse->bundle[TB_DETAILMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_DETAILMAP].numImageAnimations = 0;
				diffuse->bundle[TB_DETAILMAP].image[0] = splatImg;
			}
			else
			{
				diffuse->bundle[TB_DETAILMAP] = diffuse->bundle[0];
				diffuse->bundle[TB_DETAILMAP].numImageAnimations = 0;
				diffuse->bundle[TB_DETAILMAP].image[0] = NULL;
			}
		}
#endif
	}

	/*
	if (tcgen || diffuse->bundle[0].numTexMods)
	{
		defs |= LIGHTDEF_USE_TCGEN_AND_TCMOD;
	}
	*/

	if (diffuse->glow)
		defs |= LIGHTDEF_USE_GLOW_BUFFER;

	//ri->Printf(PRINT_ALL, ".\n");


	diffuse->glslShaderIndex = defs;
}

static int CollapseStagesToGLSL(void)
{
	int i, j, numStages;
	qboolean skip = qfalse;
	qboolean hasNormalMap = qfalse;
	qboolean hasSpecularMap = qfalse;

#ifdef __DEVELOPER_MODE__
	//ri->Printf (PRINT_DEVELOPER, "Collapsing stages for shader '%s'\n", shader.name);
#endif //__DEVELOPER_MODE__

#define EXPERIMENTAL_MERGE_STUFF
	
	// skip shaders with deforms
#ifndef EXPERIMENTAL_MERGE_STUFF
	if (shader.numDeforms != 0)
	{
		skip = qtrue;
#ifdef __DEVELOPER_MODE__
		//ri->Printf (PRINT_DEVELOPER, "> Shader has vertex deformations. Aborting stage collapsing\n");
#endif //__DEVELOPER_MODE__
	}
#endif //EXPERIMENTAL_MERGE_STUFF

#ifdef __DEVELOPER_MODE__
	//ri->Printf (PRINT_DEVELOPER, "> Original shader stage order:\n");
#endif //__DEVELOPER_MODE__

/*
	for ( int i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		shaderStage_t *stage = &stages[i];

		if ( !stage->active )
		{
			continue;
		}

#ifdef __DEVELOPER_MODE__
		ri->Printf (PRINT_DEVELOPER, "-> %s\n", stage->bundle[0].image[0]->imgName);
#endif //__DEVELOPER_MODE__
	}
*/

	if (!skip)
	{
		// if 2+ stages and first stage is lightmap, switch them
		// this makes it easier for the later bits to process
		if (tr.numLightmaps > 0 &&
			stages[0].active &&
			stages[0].bundle[0].tcGen >= TCGEN_LIGHTMAP &&
			stages[0].bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
			stages[1].active)
		{
			int blendBits = stages[1].stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

			if (blendBits == (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
				|| blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR))
			{
				int stateBits0 = stages[0].stateBits;
				int stateBits1 = stages[1].stateBits;
				shaderStage_t swapStage;

				swapStage = stages[0];
				stages[0] = stages[1];
				stages[1] = swapStage;

				stages[0].stateBits = stateBits0;
				stages[1].stateBits = stateBits1;

#ifdef __DEVELOPER_MODE__
				//ri->Printf (PRINT_DEVELOPER, "> Swapped first and second stage.\n");
				//ri->Printf (PRINT_DEVELOPER, "-> First stage is now: %s\n", stages[0].bundle[0].image[0]->imgName);
				//ri->Printf (PRINT_DEVELOPER, "-> Second stage is now: %s\n", stages[1].bundle[0].image[0]->imgName);
#endif //__DEVELOPER_MODE__
			}
		}
	}

	if (!skip)
	{
		// scan for shaders that aren't supported
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (tr.numLightmaps <= 0)
			{// If no world data, remove any lightmaps from the shader...
				if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
					pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
				{
					pStage->active = qfalse;
					continue;
				}
			}


			if (pStage->adjustColorsForFog 
#ifdef EXPERIMENTAL_MERGE_STUFF
				&& !pStage->glow
#endif //EXPERIMENTAL_MERGE_STUFF
				)
			{
				skip = qtrue;
				break;
			}
			
//#ifndef EXPERIMENTAL_MERGE_STUFF
			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
				pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			{
				int blendBits = pStage->stateBits & ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

				if (blendBits != (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
					&& blendBits != (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR))
				{
					skip = qtrue;
					break;
				}
			}
//#endif //EXPERIMENTAL_MERGE_STUFF

#ifndef __EXTRA_PRETTY__
			switch(pStage->bundle[0].tcGen)
			{
				case TCGEN_TEXTURE:
				case TCGEN_LIGHTMAP:
				case TCGEN_LIGHTMAP1:
				case TCGEN_LIGHTMAP2:
				case TCGEN_LIGHTMAP3:
				case TCGEN_ENVIRONMENT_MAPPED:
				case TCGEN_VECTOR:
					break;
				default:
					skip = qtrue;
					break;
			}
#endif //__EXTRA_PRETTY__

#ifndef EXPERIMENTAL_MERGE_STUFF
			switch(pStage->alphaGen)
			{
#ifndef __EXTRA_PRETTY__
				case AGEN_LIGHTING_SPECULAR:
#endif //__EXTRA_PRETTY__
				case AGEN_PORTAL:
					skip = qtrue;
					break;
				default:
					break;
			}
#endif //EXPERIMENTAL_MERGE_STUFF
		}
	}

	if (!skip)
	{
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{// Add any glows we see that were not marked as glow...
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->glow)
				continue;

			if (pStage->bundle[TB_DIFFUSEMAP].image[0] && pStage->bundle[TB_DIFFUSEMAP].image[0] != tr.defaultImage)
			{
				if (ForceGlow(pStage->bundle[TB_DIFFUSEMAP].image[0]->imgName))
				{
					pStage->glow = qtrue;
					pStage->glowStrength = 1.0;// 0.17142857142857142857142857142857;// 0.15;// 1.0;
					if (shader.glowStrength <= 0.0) shader.glowStrength = 1.0;// max(pStage->glowStrength, shader.glowStrength);
					shader.hasGlow = qtrue;
				}
			}
		}
	}

#ifdef __MERGE_GLOW_STAGES__
	if (!skip)
	{
		int diffuseStage = -1;
		int glowStage = -1;
		int numActiveStages = 0;

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{// Find a standard non-blended (or one/one will do) diffuse stage that we can merge into...
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			numActiveStages++;
		}

		if (numActiveStages > 1)
		{
			int glowBlend = 0;

			for (i = 0; i < MAX_SHADER_STAGES; i++)
			{// Find a standard non-blended (or one/one will do) diffuse stage that we can merge into...
				shaderStage_t *pStage = &stages[i];

				if (!pStage->active)
					continue;
				
				if (!pStage->glow && !pStage->glowMapped && pStage->bundle[TB_DIFFUSEMAP].image[0])
				{
#if 1
					int blendBits = pStage->stateBits & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

					glowBlend = blendBits;

					if (blendBits == 0)
					{
						diffuseStage = i;
						break;
					}

					if (blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_ONE))
					{
						diffuseStage = i;
						break;
					}

					if (blendBits == (GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE))
					{
						diffuseStage = i;
						break;
					}

					
					if (blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_SRC_ALPHA))
					{
						diffuseStage = i;
						break;
					}

					if (blendBits == (GLS_DSTBLEND_ONE | GLS_SRCBLEND_SRC_ALPHA))
					{
						diffuseStage = i;
						break;
					}

					if (blendBits == (GLS_DSTBLEND_DST_ALPHA | GLS_SRCBLEND_ONE))
					{
						diffuseStage = i;
						break;
					}

					if (blendBits == (GLS_DSTBLEND_DST_ALPHA | GLS_SRCBLEND_SRC_ALPHA))
					{
						diffuseStage = i;
						break;
					}
					

					/*
					if (blendBits == (GLS_DSTBLEND_ONE_MINUS_SRC_COLOR | GLS_SRCBLEND_ONE))
					{
						diffuseStage = i;
						break;
					}

					if (blendBits == (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_DST_COLOR))
					{
						diffuseStage = i;
						break;
					}
					*/
#else
					diffuseStage = i;
					break;
#endif
				}
			}

			//if (diffuseStage != -1)
			{// We found a diffuse stage to merge glow into, check if there is one to merge in...
				for (i = 0; i < MAX_SHADER_STAGES; i++)
				{
					shaderStage_t *pStage = &stages[i];

					if (!pStage->active)
						continue;

					if (pStage->glowNoMerge)
						continue;

					if (pStage->glow && pStage->bundle[TB_DIFFUSEMAP].image[0])
					{
						int blendBits = pStage->stateBits & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

						if (blendBits == 0)
						{
							glowStage = i;
							break;
						}

						if (blendBits == glowBlend)
						{
							glowStage = i;
							break;
						}

						if (blendBits == (GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE))
						{
							glowStage = i;
							break;
						}

						if (blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_ONE))
						{
							glowStage = i;
							break;
						}
						
						if (blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_SRC_ALPHA))
						{
							glowStage = i;
							break;
						}

						if (blendBits == (GLS_DSTBLEND_ONE | GLS_SRCBLEND_SRC_ALPHA))
						{
							glowStage = i;
							break;
						}

						if (blendBits == (GLS_DSTBLEND_DST_ALPHA | GLS_SRCBLEND_ONE))
						{
							glowStage = i;
							break;
						}

						if (blendBits == (GLS_DSTBLEND_DST_ALPHA | GLS_SRCBLEND_SRC_ALPHA))
						{
							glowStage = i;
							break;
						}
						
						//glowStage = i;
						//break;
					}
				}

				if (diffuseStage == -1 && glowStage != -1)
				{
					if (r_debugGlowMerge->integer) ri->Printf(PRINT_ALL, "^1PERFORMANCE WARNING^5: Shader ^7%s^5 has depricated diffuse stage for merged glows. Update your shader for Warzone, or use a <originalTextureName>_g texture for the glow!\n", shader.name);
				}
				else if (glowStage != -1)
				{// We can save a draw call here, so merge the glow stage into the diffuse stage...
					shaderStage_t *dStage = &stages[diffuseStage];
					shaderStage_t *gStage = &stages[glowStage];
					dStage->bundle[TB_GLOWMAP].image[0] = gStage->bundle[TB_DIFFUSEMAP].image[0];
					dStage->glowMapped = qtrue;
					dStage->glow = qtrue;
					dStage->glowColorFound = gStage->glowColorFound;
					
					VectorCopy4(gStage->glowColor, dStage->glowColor);
					
					dStage->glslShaderIndex |= LIGHTDEF_USE_GLOW_BUFFER;

					if (gStage->emissiveRadiusScale <= 0.0)
						dStage->emissiveRadiusScale = 1.0;
					else
						dStage->emissiveRadiusScale = gStage->emissiveRadiusScale;

					if (gStage->emissiveColorScale <= 0.0)
						dStage->emissiveColorScale = 1.5;
					else
						dStage->emissiveColorScale = gStage->emissiveColorScale;

					//dStage->glowBlend = glowBlend;
					if (glowBlend & GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA)
						dStage->glowBlend = GLSL_BLEND_INVALPHA;
					else if (glowBlend & GLS_SRCBLEND_DST_ALPHA)
						dStage->glowBlend = GLSL_BLEND_DST_ALPHA;
					else if (glowBlend & GLS_SRCBLEND_ONE_MINUS_DST_ALPHA)
						dStage->glowBlend = GLSL_BLEND_INV_DST_ALPHA;
					else if (glowBlend & GLS_DSTBLEND_SRC_COLOR)
						dStage->glowBlend = GLSL_BLEND_GLOWCOLOR;
					else if (glowBlend & GLS_DSTBLEND_ONE_MINUS_SRC_COLOR)
						dStage->glowBlend = GLSL_BLEND_INV_GLOWCOLOR;
					else if (glowBlend & GLS_SRCBLEND_DST_COLOR)
						dStage->glowBlend = GLSL_BLEND_DSTCOLOR;
					else if (glowBlend & GLS_SRCBLEND_ONE_MINUS_DST_COLOR)
						dStage->glowBlend = GLSL_BLEND_INV_DSTCOLOR;
					else
						dStage->glowBlend = GLSL_BLEND_GLOWCOLOR;

					gStage->active = qfalse;

					if (r_debugGlowMerge->integer) ri->Printf(PRINT_ALL, "^3PERFORMANCE ENHANCEMENT^5: Successfully merged glow stage ^7%i^5 into a diffuse stage ^7%i^5 for shader ^7%s^5.\n", glowStage, diffuseStage, shader.name);
				}
			}
		}
	}
#endif //__MERGE_GLOW_STAGES__

	if (!skip)
	{
		shaderStage_t *lightmaps[MAX_SHADER_STAGES] = { NULL };

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			{
				lightmaps[i] = pStage;
			}
		}

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];
			shaderStage_t *diffuse, *normal, *specular, *lightmap;
			qboolean parallax, tcgen;

			if (!pStage->active)
				continue;

			// skip normal and specular maps
			if (pStage->type != ST_COLORMAP)
				continue;

			// skip lightmaps
			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
				continue;

			diffuse  = pStage;
			normal   = NULL;
			parallax = qfalse;
			specular = NULL;
			lightmap = NULL;

			// we have a diffuse map, find matching normal, specular, and lightmap
			for (j = i + 1; j < MAX_SHADER_STAGES; j++)
			{
				shaderStage_t *pStage2 = &stages[j];

				if (!pStage2->active)
					continue;

				if (pStage2->glow)
					continue;

				switch(pStage2->type)
				{
					case ST_NORMALMAP:
						if (!normal)
						{
							hasNormalMap = qtrue;
							normal = pStage2;
						}
						break;

					case ST_NORMALPARALLAXMAP:
						if (!normal)
						{
							hasNormalMap = qtrue;
							normal = pStage2;
							parallax = qtrue;
						}
						break;

					case ST_SPECULARMAP:
						if (!specular)
						{
							hasSpecularMap = qtrue;
							specular = pStage2;
						}
						break;

					case ST_COLORMAP:
						if (pStage2->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
							pStage2->bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
							pStage2->rgbGen != CGEN_EXACT_VERTEX)
						{
#ifdef __DEVELOPER_MODE__
							//ri->Printf (PRINT_DEVELOPER, "> Setting lightmap for %s to %s\n", pStage->bundle[0].image[0]->imgName, pStage2->bundle[0].image[0]->imgName);
#endif //__DEVELOPER_MODE__
							lightmap = pStage2;
							lightmaps[j] = NULL;
						}
						break;

					default:
						break;
				}
			}

			tcgen = qfalse;
			if (diffuse->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED
			    || (diffuse->bundle[0].tcGen >= TCGEN_LIGHTMAP && diffuse->bundle[0].tcGen <= TCGEN_LIGHTMAP3)
			    || diffuse->bundle[0].tcGen == TCGEN_VECTOR)
			{
				tcgen = qtrue;
			}

			CollapseStagesToLightall(diffuse, normal, specular, lightmap, parallax, tcgen);
		}

		// deactivate lightmap stages
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->bundle[0].tcGen >= TCGEN_LIGHTMAP &&
				pStage->bundle[0].tcGen <= TCGEN_LIGHTMAP3 &&
				lightmaps[i] == NULL)
			{
				pStage->active = qfalse;
			}
		}
	}

	shader.hasGlow = qfalse;

	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{// Find a standard non-blended (or one/one will do) diffuse stage that we can merge into...
		shaderStage_t *pStage = &stages[i];

		if (!pStage->active)
			continue;

		if (pStage->glow || pStage->glowMapped)
		{
			shader.hasGlow = qtrue;
			break;
		}
	}

	// deactivate normal and specular stages
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		shaderStage_t *pStage = &stages[i];

		if (!pStage->active)
			continue;

		if (pStage->type == ST_NORMALMAP)
		{
			hasNormalMap = qfalse;
			pStage->active = qfalse;
		}

		if (pStage->type == ST_NORMALPARALLAXMAP)
		{
			hasNormalMap = qfalse;
			pStage->active = qfalse;
		}

		if (pStage->type == ST_SPECULARMAP)
		{
			hasSpecularMap = qfalse;
			pStage->active = qfalse;
		}
	}

	//
	//
	//

	// remove inactive stages
	numStages = 0;
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		if (!stages[i].active)
			continue;

		if (stages[i].bundle[0].isLightmap)
			stages[i].noScreenMap = qtrue;

		if ((stages[i].stateBits & GLS_DEPTHTEST_DISABLE) || (stages[i].stateBits & GLS_DEPTHFUNC_EQUAL))
			stages[i].noScreenMap = qtrue;
		
		//if (stages[i].bundle[0].tcGen != TCGEN_IDENTITY && stages[i].bundle[0].tcGen != TCGEN_TEXTURE)
		//	stages[i].noScreenMap = qtrue;

		if (i == numStages)
		{
			numStages++;
			continue;
		}

		stages[numStages] = stages[i];
		stages[i].active = qfalse;
		numStages++;
	}

	// convert any remaining lightmap stages to a lighting pass with a white texture
	// only do this with r_sunlightMode non-zero, as it's only for correct shadows.
	if (r_sunlightMode->integer /*&& shader.numDeforms == 0*/)
	{
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog)
				continue;

			if (pStage->bundle[TB_DIFFUSEMAP].tcGen >= TCGEN_LIGHTMAP && pStage->bundle[TB_DIFFUSEMAP].tcGen <= TCGEN_LIGHTMAP3)
			{
				if (hasNormalMap) pStage->hasNormalMap = true;

				pStage->glslShaderIndex = LIGHTDEF_USE_LIGHTMAP;
				pStage->bundle[TB_LIGHTMAP] = pStage->bundle[TB_DIFFUSEMAP];
				pStage->bundle[TB_DIFFUSEMAP].image[0] = tr.whiteImage;
				pStage->bundle[TB_DIFFUSEMAP].isLightmap = qfalse;
				pStage->bundle[TB_DIFFUSEMAP].tcGen = TCGEN_TEXTURE;
			}
		}
	}

	// convert any remaining lightingdiffuse stages to a lighting pass
	//if (shader.numDeforms == 0)
	{
		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			if (pStage->adjustColorsForFog
#ifdef EXPERIMENTAL_MERGE_STUFF
				&& !pStage->glow
#endif //EXPERIMENTAL_MERGE_STUFF
				)
				continue;

			if (pStage->rgbGen == CGEN_LIGHTING_DIFFUSE ||
				pStage->rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
			{
				if (hasNormalMap) 
					pStage->hasNormalMap = true;
			}
		}
	}

#ifdef __DEVELOPER_MODE__
	//ri->Printf (PRINT_DEVELOPER, "> New shader stage order:\n");
#endif //__DEVELOPER_MODE__

	for ( int i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		shaderStage_t *stage = &stages[i];

		if ( !stage->active )
		{
			continue;
		}

		if (hasNormalMap)
		{
			stage->hasNormalMap = true;
		}

		if (hasSpecularMap)
		{
			stage->hasSpecularMap = true;
		}

#ifdef __DEVELOPER_MODE__
		//ri->Printf (PRINT_DEVELOPER, "-> %s\n", stage->bundle[0].image[0]->imgName);
#endif //__DEVELOPER_MODE__
	}

#if 1
	if (r_debugShaderStages->integer && numStages >= 1 && !tr.world)
	{
		ri->Printf(PRINT_WARNING, "Shader %s has %i stages.\n", shader.name, numStages);

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			shaderStage_t *pStage = &stages[i];

			if (!pStage->active)
				continue;

			char glowMapped[256] = { 0 };
				
			if (pStage->stateBits & GLS_SRCBLEND_ZERO) strcat(glowMapped, " GLS_SRCBLEND_ZERO");
			else if (pStage->stateBits & GLS_SRCBLEND_ONE) strcat(glowMapped, " GLS_SRCBLEND_ONE");
			else if (pStage->stateBits & GLS_SRCBLEND_DST_COLOR) strcat(glowMapped, " GLS_SRCBLEND_DST_COLOR");
			else if (pStage->stateBits & GLS_SRCBLEND_ONE_MINUS_DST_COLOR) strcat(glowMapped, " GLS_SRCBLEND_ONE_MINUS_DST_COLOR");
			else if (pStage->stateBits & GLS_SRCBLEND_SRC_ALPHA) strcat(glowMapped, " GLS_SRCBLEND_SRC_ALPHA");
			else if (pStage->stateBits & GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA) strcat(glowMapped, " GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA");
			else if (pStage->stateBits & GLS_SRCBLEND_DST_ALPHA) strcat(glowMapped, " GLS_SRCBLEND_DST_ALPHA");
			else if (pStage->stateBits & GLS_SRCBLEND_ONE_MINUS_DST_ALPHA) strcat(glowMapped, " GLS_SRCBLEND_ONE_MINUS_DST_ALPHA");
			else if (pStage->stateBits & GLS_SRCBLEND_ALPHA_SATURATE) strcat(glowMapped, " GLS_SRCBLEND_ALPHA_SATURATE");
			else strcat(glowMapped, " GLS_SRCBLEND_UNKNOWN");
			
			if (pStage->stateBits & GLS_DSTBLEND_ZERO) strcat(glowMapped, " GLS_DSTBLEND_ZERO");
			else if (pStage->stateBits & GLS_DSTBLEND_ONE) strcat(glowMapped, " GLS_DSTBLEND_ONE");
			else if (pStage->stateBits & GLS_DSTBLEND_SRC_COLOR) strcat(glowMapped, " GLS_DSTBLEND_SRC_COLOR");
			else if (pStage->stateBits & GLS_DSTBLEND_ONE_MINUS_SRC_COLOR) strcat(glowMapped, " GLS_DSTBLEND_ONE_MINUS_SRC_COLOR");
			else if (pStage->stateBits & GLS_DSTBLEND_SRC_ALPHA) strcat(glowMapped, " GLS_DSTBLEND_SRC_ALPHA");
			else if (pStage->stateBits & GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA) strcat(glowMapped, " GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA");
			else if (pStage->stateBits & GLS_DSTBLEND_DST_ALPHA) strcat(glowMapped, " GLS_DSTBLEND_DST_ALPHA");
			else if (pStage->stateBits & GLS_DSTBLEND_ONE_MINUS_DST_ALPHA) strcat(glowMapped, " GLS_DSTBLEND_ONE_MINUS_DST_ALPHA");
			else strcat(glowMapped, " GLS_DSTBLEND_UNKNOWN");
				
			if (pStage->glowMapped && pStage->isDetail)
				strcat(glowMapped, " (detail - glowMap merged)");
			else if (pStage->glow && pStage->isDetail)
				strcat(glowMapped, " (detail - glowMap unmerged)");
			else if (pStage->glowMapped)
				strcat(glowMapped, " (glowMap merged)");
			else if (pStage->glow)
				strcat(glowMapped, " (glowMap unmerged)");
			else if (pStage->isDetail)
				strcat(glowMapped, " (detail)");

			if (pStage->type == ST_DIFFUSEMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is DiffuseMap%s.\n", i, glowMapped);
			}
			else if (pStage->type == ST_NORMALMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is NormalMap%s.\n", i, glowMapped);
			}
			else if (pStage->type == ST_NORMALPARALLAXMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is NormalParallaxMap%s.\n", i, glowMapped);
			}
			else if (pStage->type == ST_SPECULARMAP)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is SpecularMap%s.\n", i, glowMapped);
			}
			else if (pStage->type == ST_GLSL)
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is GLSL%s.\n", i, glowMapped);
			}
			else
			{
				ri->Printf(PRINT_WARNING, "     Stage %i is %i%s.\n", i, pStage->type, glowMapped);
			}
		}
	}
#endif

	shader.maxStage = 0;
	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		if (!stages[i].active)
			continue;

		shader.maxStage = i;
	}

	// Record number of active stages for sorting...
	shader.numStages = numStages;

	return numStages;
}

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
extern bool gServerSkinHack;
static void FixRenderCommandList( int newShader ) {
	if( !gServerSkinHack ) {
		renderCommandList_t	*cmdList = &backEndData->commands;

		if( cmdList ) {
			const void *curCmd = cmdList->cmds;

			while ( 1 ) {
				curCmd = PADP(curCmd, sizeof(void *));

				switch ( *(const int *)curCmd ) {
				case RC_SET_COLOR:
					{
					const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;
					curCmd = (const void *)(sc_cmd + 1);
					break;
					}
				case RC_STRETCH_PIC:
					{
					const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;
					curCmd = (const void *)(sp_cmd + 1);
					break;
					}
				case RC_ROTATE_PIC:
				case RC_ROTATE_PIC2:
					{
						const rotatePicCommand_t *sp_cmd = (const rotatePicCommand_t *)curCmd;
						curCmd = (const void *)(sp_cmd + 1);
						break;
					}
				case RC_DRAW_SURFS:
					{
					int i;
					drawSurf_t	*drawSurf;
					shader_t	*shader;
					int64_t		fogNum;
					int64_t		entityNum;
					//int64_t		dlightMap;
					int64_t		sortedIndex;
					int64_t		postRender;
					const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

					for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
						R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &postRender );
						sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
						if( sortedIndex >= newShader ) {
							sortedIndex++;
							drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | (entityNum << QSORT_REFENTITYNUM_SHIFT) /*| (fogNum << QSORT_FOGNUM_SHIFT)*/ | (postRender << QSORT_POSTRENDER_SHIFT);// | dlightMap;
						}
					}
					curCmd = (const void *)(ds_cmd + 1);
					break;
					}
				case RC_DRAW_BUFFER:
					{
					const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;
					curCmd = (const void *)(db_cmd + 1);
					break;
					}
				case RC_SWAP_BUFFERS:
					{
					const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;
					curCmd = (const void *)(sb_cmd + 1);
					break;
					}
				case RC_DRAW_OCCLUSION:
					{
					const drawOcclusionCommand_t *do_cmd = (const drawOcclusionCommand_t *)curCmd;
					curCmd = (const void *)(do_cmd + 1);
					break;
					}
				case RC_END_OF_LIST:
				default:
					return;
				}
			}
		}
	}
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void ) {
	int		i;
	float	sort;
	shader_t	*newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i+1] = tr.sortedShaders[i];
		tr.sortedShaders[i+1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList( i+1 );

	newShader->sortedIndex = i+1;
	tr.sortedShaders[i+1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t	*newShader;
	int			i, b;
	int			size, hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri->Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = (shader_t *)ri->Hunk_Alloc( sizeof( shader_t ), h_low );

	*newShader = shader;

	if ( shader.sort <= SS_SEE_THROUGH ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			break;
		}
		newShader->stages[i] = (shaderStage_t *)ri->Hunk_Alloc( sizeof( stages[i] ), h_low );
		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			newShader->stages[i]->bundle[b].texMods = (texModInfo_t *)ri->Hunk_Alloc( size, h_low );
			Com_Memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void ) {
	int		stage;
	shaderStage_t	*bestStage;
	int		bestImageRank;
	int		rank;

	// if we aren't opaque, just use the first pass
	if ( shader.sort == SS_OPAQUE ) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
			shaderStage_t *pStage = &stages[stage];

			if ( !pStage->active ) {
				break;
			}
			rank = 0;

			if ( pStage->bundle[0].isLightmap ) {
				rank -= 100;
			}
			if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE ) {
				rank -= 5;
			}
			if ( pStage->bundle[0].numTexMods ) {
				rank -= 5;
			}
			if ( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING ) {
				rank -= 3;
			}

			if ( rank > bestImageRank  ) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		} else {
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	} else {
		// don't use a lightmap (tesla coils)
		if ( stages[0].bundle[0].isLightmap ) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if ( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		Com_Memset( pStage, 0, sizeof( *pStage ) );
	}
}

int FindFirstLightmapStage ( const shaderStage_t *stages, int numStages )
{
	for ( int i = 0; i < numStages; i++ )
	{
		const shaderStage_t *stage = &stages[i];
		if ( stage->active && stage->bundle[0].isLightmap )
		{
			return i;
		}
	}

	return numStages;
}

int GetNumStylesInShader ( const shader_t *shader )
{
	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		if ( shader->styles[i] >= LS_UNUSED )
		{
			return i - 1;
		}
	}

	return MAXLIGHTMAPS - 1;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader(void) {
	int stage;
	qboolean hasLightmapStage = qfalse;

#ifdef __DEBUG_SHADER_LOADING__
	ri->Printf(PRINT_ALL, "FinishShader: %s.\n", shader.name);
#endif //__DEBUG_SHADER_LOADING__

	//
	// set sky stuff appropriate
	//
	if (shader.isSky) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if (shader.polygonOffset && !shader.sort) {
		shader.sort = SS_DECAL;
	}

	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{// TESTING: Replace anything with IDENTITY rgbGen with new warzone lighting...
		shaderStage_t *pStage = &stages[stage];

		if (!pStage->active) {
			continue;
		}

		if (pStage->rgbGen == CGEN_IDENTITY || pStage->rgbGen == CGEN_IDENTITY_LIGHTING)
		{
			pStage->rgbGen = CGEN_LIGHTING_WARZONE;
		}
	}


	if (shader.glowStrength == 1.0
		&& (StringContainsWord(shader.name, "models/players") || StringContainsWord(shader.name, "models/weapons") || StringContainsWord(shader.name, "models/wzweapons")))
	{// If this shader has glows, but still has the default glow strength, amp up the brightness because these are small glow objects...
		if (StringContainsWord(shader.name, "players/hk"))
		{// Hacky override for follower hk droids eyes...
			shader.glowStrength = 96.0;// 48.0;
		}
		else
		{
			for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
			{
				shaderStage_t *pStage = &stages[stage];

				if (!pStage->active) {
					break;
				}

				if (pStage->glow || pStage->glowMapped)
				{
					shader.glowStrength = 4.0;// 2.0;
					break;
				}
			}
		}
	}
	else if (StringContainsWord(shader.name, "gfx/flames/particle_fire"))
	{// For fires, override the default glow strength...
		shader.glowStrength = 0.00375;// 0.015;
	}
	else if (shader.glowStrength == 1.0
		&& StringContainsWord(shader.name, "neon"))
	{// For fires, override the default glow strength...
		shader.glowStrength = 0.2;
	}
	else if (shader.glowStrength == 1.0
		&& StringContainsWord(shader.name, "rooftop/screen"))
	{// For fires, override the default glow strength...
		shader.glowStrength = 0.2;
	}

#if 0
	int firstLightmapStage;
	shaderStage_t *lmStage;

	firstLightmapStage = FindFirstLightmapStage(stages, MAX_SHADER_STAGES);
	lmStage = &stages[firstLightmapStage];

	if (firstLightmapStage != MAX_SHADER_STAGES)
	{
		if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX)
		{
			if (firstLightmapStage == 0)
			{
				/*// Shift all stages above it down 1.
				memmove (lmStage,
					lmStage + 1,
					sizeof (shaderStage_t) * (MAX_SHADER_STAGES - firstLightmapStage - 1));
				memset (stages + MAX_SHADER_STAGES - 1, 0, sizeof (shaderStage_t));

				// Set state bits back to default on the over-written stage.
				 lmStage->stateBits = GLS_DEFAULT;*/
				ri->Printf(PRINT_ALL, "Shader '%s' has first stage as lightmap by vertex.\n", shader.name);
			}

			/*lmStage->rgbGen = CGEN_EXACT_VERTEX_LIT;
			lmStage->alphaGen = AGEN_SKIP;

			firstLightmapStage = MAX_SHADER_STAGES;*/
		}
	}

	if (firstLightmapStage != MAX_SHADER_STAGES)
	{
		int numStyles = GetNumStylesInShader(&shader);

		ri->Printf(PRINT_ALL, "Shader '%s' has %d stages with light styles.\n", shader.name, numStyles);
		/*if ( numStyles > 0 )
		{
			// Move back all stages, after the first lightmap stage, by 'numStyles' elements.
			memmove (lmStage + numStyles,
				lmStage + 1,
				sizeof (shaderStage_t) * (MAX_SHADER_STAGES - firstLightmapStage - numStyles - 1));

			// Insert new shader stages after first lightmap stage
			for ( int i = 1; i <= numStyles; i++ )
			{
				shaderStage_t *stage = lmStage + i;

				// Duplicate first lightmap stage into this stage.
				*stage = *lmStage;

				if ( shader.lightmapIndex[i] == LIGHTMAP_BY_VERTEX )
				{
					stage->bundle[0].image[0] = tr.whiteImage;
				}
				else if ( shader.lightmapIndex[i] < 0 )
				{
					Com_Error (ERR_DROP, "FinishShader: light style with no lightmap or vertex style in shader %s.\n", shader.name);
				}
				else
				{
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[i]];
					stage->bundle[0].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP + i);
				}

				stage->rgbGen = CGEN_LIGHTMAPSTYLE;
				stage->stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
				stage->stateBits |= GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			}
		}

		// Set all the light styles for the lightmap stages.
		for ( int i = 0; i <= numStyles; i++ )
		{
			lmStage[i].lightmapStyle = shader.styles[i];
		}*/
	}
#else
	int lmStage;
	for (lmStage = 0; lmStage < MAX_SHADER_STAGES; lmStage++)
	{
		shaderStage_t *pStage = &stages[lmStage];
		if (pStage->active && pStage->bundle[0].isLightmap)
		{
			break;
		}
	}

	if (lmStage < MAX_SHADER_STAGES)
	{
		if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX)
		{
			if (lmStage == 0)	//< MAX_SHADER_STAGES-1)
			{//copy the rest down over the lightmap slot
				memmove(&stages[lmStage], &stages[lmStage + 1], sizeof(shaderStage_t) * (MAX_SHADER_STAGES - lmStage - 1));
				memset(&stages[MAX_SHADER_STAGES - 1], 0, sizeof(shaderStage_t));
				//change blending on the moved down stage
				stages[lmStage].stateBits = GLS_DEFAULT;
			}
			//change anything that was moved down (or the *white if LM is first) to use vertex color
			stages[lmStage].rgbGen = CGEN_EXACT_VERTEX;
			stages[lmStage].alphaGen = AGEN_SKIP;
			lmStage = MAX_SHADER_STAGES;	//skip the style checking below
		}
	}

	if (lmStage < MAX_SHADER_STAGES)// && !r_fullbright->value)
	{
		int	numStyles;
		int	i;

		for (numStyles = 0; numStyles < MAXLIGHTMAPS; numStyles++)
		{
			if (shader.styles[numStyles] >= LS_UNUSED)
			{
				break;
			}
		}
		numStyles--;
		if (numStyles > 0)
		{
			for (i = MAX_SHADER_STAGES - 1; i > lmStage + numStyles; i--)
			{
				stages[i] = stages[i - numStyles];
			}

			for (i = 0; i < numStyles; i++)
			{
				stages[lmStage + i + 1] = stages[lmStage];
				if (shader.lightmapIndex[i + 1] == LIGHTMAP_BY_VERTEX)
				{
					stages[lmStage + i + 1].bundle[0].image[0] = tr.whiteImage;
				}
				else if (shader.lightmapIndex[i + 1] < 0)
				{
					Com_Error(ERR_DROP, "FinishShader: light style with no light map or vertex color for shader %s", shader.name);
				}
				else
				{
					stages[lmStage + i + 1].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[i + 1]];
					stages[lmStage + i + 1].bundle[0].tcGen = (texCoordGen_t)(TCGEN_LIGHTMAP + i + 1);
				}
				stages[lmStage + i + 1].rgbGen = CGEN_LIGHTMAPSTYLE;
				stages[lmStage + i + 1].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
				stages[lmStage + i + 1].stateBits |= GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			}
		}

		for (i = 0; i <= numStyles; i++)
		{
			stages[lmStage + i].lightmapStyle = shader.styles[i];
		}
	}
#endif

	//
	// set appropriate stage information
	//
	for (stage = 0; stage < MAX_SHADER_STAGES; ) {
		shaderStage_t *pStage = &stages[stage];

		if (!pStage->active) {
			break;
		}

		// check for a missing texture
			/*if ( !pStage->bundle[0].image[0] ) {
				ri->Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
				pStage->active = qfalse;
				stage++;
				continue;
			}*/

			// check for a missing texture
		switch (pStage->type)
		{
			//case ST_LIGHTMAP:
			//	// skip
			//	break;

		case ST_COLORMAP: // + ST_DIFFUSEMAP
		default:
		{
			if (!pStage->bundle[0].image[0])
			{
				if (!pStage->useSkyImage)
				{
					ri->Printf(PRINT_WARNING, "Shader %s has a colormap/diffusemap stage with no image\n", shader.name);
				}

				pStage->bundle[0].image[0] = tr.defaultImage;

				/*if (!pStage->useSkyImage)
				{
					pStage->active = qfalse;
				}*/
			}
			break;
		}

		case ST_NORMALMAP:
		{
			if (!pStage->bundle[0].image[0])
			{
				ri->Printf(PRINT_WARNING, "Shader %s has a normalmap stage with no image\n", shader.name);
				pStage->bundle[0].image[0] = tr.whiteImage;
				pStage->active = qfalse;
				stage++;
				continue;
			}
			break;
		}

		case ST_SPECULARMAP:
		{
			if (!pStage->bundle[0].image[0])
			{
				ri->Printf(PRINT_WARNING, "Shader %s has a specularmap stage with no image\n", shader.name);
				pStage->bundle[0].image[0] = tr.blackImage; // should be blackImage
				pStage->active = qfalse;
				stage++;
				continue;
			}
			break;
		}

		case ST_NORMALPARALLAXMAP:
		{
			if (!pStage->bundle[0].image[0])
			{
				ri->Printf(PRINT_WARNING, "Shader %s has a normalparallaxmap stage with no image\n", shader.name);
				pStage->active = qfalse;
				stage++;
				continue;
			}
			break;
		}
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if (pStage->isDetail && !r_detailTextures->integer)
		{
			int index;

			for (index = stage + 1; index < MAX_SHADER_STAGES; index++)
			{
				if (!stages[index].active)
					break;
			}

			if (index < MAX_SHADER_STAGES)
				memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage));
			else
			{
				if (stage + 1 < MAX_SHADER_STAGES)
					memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage - 1));

				Com_Memset(&stages[index - 1], 0, sizeof(*stages));
			}

			continue;
		}

		//
		// default texture coordinate generation
		//
		if (pStage->bundle[0].isLightmap) {
			if (pStage->bundle[0].tcGen == TCGEN_BAD) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		}
		else {
			if (pStage->bundle[0].tcGen == TCGEN_BAD) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}

		//
		// determine sort order and fog color adjustment
		//
		if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) &&
			(stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if (((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE)) ||
				((blendSrcBits == GLS_SRCBLEND_ZERO) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			}
			else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if (!shader.sort) {
				// see through item, like a grill or grate
				if (pStage->stateBits & GLS_DEPTHMASK_TRUE) {
					shader.sort = SS_SEE_THROUGH;
				}
				else {
					if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE))
					{
						// GL_ONE GL_ONE needs to come a bit later
						shader.sort = SS_BLEND1;
					}
					else
					{
						shader.sort = SS_BLEND0;
					}
				}
			}
		}

		stage++;
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if (!shader.sort) {
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if (stage > 1 && (r_vertexLight->integer && !r_uiFullScreen->integer)) {
		VertexLightingCollapse();
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	stage = CollapseStagesToGLSL();

	if ((shader.lightmapIndex[0] || shader.lightmapIndex[1] || shader.lightmapIndex[2] || shader.lightmapIndex[3]) && !hasLightmapStage)
	{
#ifdef __DEVELOPER_MODE__
		ri->Printf(PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name);
#endif //__DEVELOPER_MODE__
		// Don't set this, it will just add duplicate shaders to the hash
		//shader.lightmapIndex = LIGHTMAP_NONE;
	}


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0 && !shader.isSky)
		shader.sort = SS_FOG;

	if (shader.contentFlags & CONTENTS_LAVA)
	{// Override missing basejka material type.. lava.
		shader.materialType = MATERIAL_LAVA;
	}

	if (!Q_stricmp(shader.name, "textures/system/nodraw_solid")
		|| StringContainsWord(shader.name, "nodraw_solid")
		|| StringContainsWord(shader.name, "collision")
		|| StringContainsWord(shader.name, "noshader"))
	{// Make sure these are marked to not draw...
		shader.surfaceFlags |= SURF_NODRAW;
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	// determine which vertex attributes this shader needs
	ComputeVertexAttribs();

	if (!strcmp(shader.name, "cursor"))
	{// Mark the cursor so nuklear can override it's image...
		shader.isCursor = qtrue;
	}

	if (StringContainsWord(shader.name, "models/warzone/ships/"))
	{// TODO: Add shader keyword.
		shader.nocull = qtrue;
	}

#ifdef __DEBUG_SHADER_LOADING__
	ri->Printf(PRINT_ALL, "FinishShader: %s FINISHED!\n", shader.name);
#endif //__DEBUG_SHADER_LOADING__

	return GeneratePermanentShader();
}

//========================================================================================

qboolean SkipBracedSection_Depth (const char **program, int depth);

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static const char *FindShaderInShaderText( const char *shadername ) {

	char *token;
	const char *p = NULL;

	int i, hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if(shaderTextHashTable[hash])
	{
		for (i = 0; shaderTextHashTable[hash][i]; i++)
		{
			p = shaderTextHashTable[hash][i];
			token = COM_ParseExt(&p, qtrue);

			if(!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;

	if ( !p ) {
		return NULL;
	}

	// look for label
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, shadername ) ) {
			return p;
		}
		else {
			// skip the definition
			SkipBracedSection_Depth( &p, 0 );
		}
	}

	return NULL;
}


/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char		strippedName[MAX_IMAGE_PATH];
	int			hash;
	shader_t	*sh;

	if ( (name==NULL) || (name[0] == 0) ) {
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0) {
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

static qboolean IsShader ( const shader_t *sh, const char *name, const int *lightmapIndexes, const byte *styles )
{
	if ( Q_stricmp (sh->name, name) != 0 )
	{
		return qfalse;
	}

	if ( sh->defaultShader )
	{
		return qtrue;
	}

	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		if ( sh->lightmapIndex[i] != lightmapIndexes[i] )
		{
			return qfalse;
		}

		if ( sh->styles[i] != styles[i] )
		{
			return qfalse;
		}
	}

	return qtrue;
}


/*
======================================================================================================================================
                                                   Rend2 - Compatibile Generic Shaders.
======================================================================================================================================

This creates generic shaders for anything that has none to support rend2 stuff...

*/

//#define __USE_SKYMAP_STAGES__

#ifdef __SHADER_GENERATOR__
char uniqueGenericGlow[] = "{\n"\
"map %s\n"\
"blendFunc GL_ONE GL_ONE\n"\
"glow\n"\
"noScreenMap\n"\
"}\n";

char uniqueGenericLightmap[] = "{\n"\
"map $lightmap\n"\
"blendfunc GL_DST_COLOR GL_ZERO\n"\
"rgbGen lightingDiffuse\n"\
"//rgbGen warzoneLighting\n"\
"depthFunc equal\n"\
"noScreenMap\n"\
"}\n";

#ifdef __USE_SKYMAP_STAGES__
char uniqueGenericSkyMap[] = "{\n"\
"map $skyimage\n"\
"blendFunc GL_DST_COLOR GL_SRC_COLOR\n"\
"detail\n"\
"alphaGen const %f\n"\
"tcGen environment\n"\
"rgbGen Vertex\n"\
"noScreenMap\n"\
"}\n";
#endif //__USE_SKYMAP_STAGES__

char uniqueGenericFoliageShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_alphashadow\n"\
"q3map_material	GreenLeaves\n"\
"surfaceparm	nonsolid\n"\
"entityMergable\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen identity\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericFoliageBillboardShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_alphashadow\n"\
"q3map_material	DryLeaves\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"entityMergable\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen identity\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericFoliageTreeBarkShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	treebark\n"\
"glowStrength 1.5\n"\
"entityMergable\n"\
"//tesselation\n"\
"//tesselationLevel 3.0\n"\
"//tesselationAlpha 1.0\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"depthWrite\n"\
"//rgbGen identity\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"//tcMod scale 2.5 2.5\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericFoliageLeafsShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_alphashadow\n"\
"q3map_material	GreenLeaves\n"\
"surfaceparm	nonsolid\n"\
"entityMergable\n"\
"cull	twosided\n"\
"//tesselation\n"\
"//tesselationLevel 3.0\n"\
"//tesselationAlpha 1.0\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen identity\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"}\n"\
"}\n"\
"";

char uniqueGenericRockShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	rock\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"glowStrength 1.5\n"\
"entityMergable\n"\
"{\n"\
"map %s\n"\
"%s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen identity\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"}\n"\
"}\n"\
"";

#ifdef __USE_SKYMAP_STAGES__
char uniqueGenericPlayerShader[] = "{\n"\
"qer_editorimage	%s\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"glowStrength 1.5\n"\
"entityMergable\n"\
"tesselation\n"\
"tesselationLevel 3.0\n"\
"tesselationAlpha 1.0\n"\
"{\n"\
"map %s\n"\
"//blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen lightingDiffuse\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericArmorShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	armor\n"\
"surfaceparm trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"//entityMergable\n"\
"glowStrength 16.0\n"\
"cull	twosided\n"\
"tesselation\n"\
"tesselationLevel 3.0\n"\
"tesselationAlpha 1.0\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen lightingDiffuse\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericMetalShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	hollowmetal\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"//blendfunc GL_SRC_ALPHA GL_ZERO\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"//rgbGen entity\n"\
"//rgbGen lightingDiffuse\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"}\n"\
"%s"\
"%s"\
"}\n"\
"";

char uniqueGenericWeaponShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	solidmetal\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"glowStrength 16.0\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"//rgbGen lightingDiffuse\n"\
"rgbGen identityLighting\n"\
"//rgbGen warzoneLighting\n"\
"depthWrite\n"\
"//rgbGen entity\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericShader[] = "{\n"\
"qer_editorimage	%s\n"\
"//entityMergable\n"\
"{\n"\
"map %s\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"%s"\
"}\n"\
"";
#elif defined(__TESSELLATE_PLAYERS__)
char uniqueGenericPlayerShader[] = "{\n"\
"qer_editorimage	%s\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"glowStrength 1.5\n"\
"entityMergable\n"\
"tesselation\n"\
"tesselationLevel 3.0\n"\
"tesselationAlpha 1.0\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.25\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericArmorShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	armor\n"\
"surfaceparm trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"//entityMergable\n"\
"glowStrength 16.0\n"\
"cull	twosided\n"\
"tesselation\n"\
"tesselationLevel 3.0\n"\
"tesselationAlpha 1.0\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.25\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericMetalShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	hollowmetal\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.3\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"%s"\
"}\n"\
"";

char uniqueGenericWeaponShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	solidmetal\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"glowStrength 16.0\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.3\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"rgbGen identityLighting\n"\
"depthWrite\n"\
"//rgbGen entity\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericShader[] = "{\n"\
"qer_editorimage	%s\n"\
"//entityMergable\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.1\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"%s"\
"}\n"\
"";
#else //!__USE_SKYMAP_STAGES__
char uniqueGenericPlayerShader[] = "{\n"\
"qer_editorimage	%s\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"glowStrength 1.5\n"\
"entityMergable\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.25\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericArmorShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	armor\n"\
"surfaceparm trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"//entityMergable\n"\
"glowStrength 16.0\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.25\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericMetalShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	hollowmetal\n"\
"surfaceparm	trans\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.3\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"%s"\
"}\n"\
"";

char uniqueGenericWeaponShader[] = "{\n"\
"qer_editorimage	%s\n"\
"q3map_material	solidmetal\n"\
"surfaceparm	noimpact\n"\
"surfaceparm	nomarks\n"\
"glowStrength 16.0\n"\
"cull	twosided\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.3\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"rgbGen identityLighting\n"\
"depthWrite\n"\
"//rgbGen entity\n"\
"}\n"\
"%s"\
"}\n"\
"";

char uniqueGenericShader[] = "{\n"\
"qer_editorimage	%s\n"\
"//entityMergable\n"\
"{\n"\
"map %s\n"\
"envmap $skyimage\n"\
"envmapStrength 0.1\n"\
"blendfunc GL_ONE GL_ZERO\n"\
"alphaFunc GE128\n"\
"depthWrite\n"\
"rgbGen identityLighting\n"\
"}\n"\
"%s"\
"%s"\
"}\n"\
"";
#endif //__USE_SKYMAP_STAGES__

qboolean R_AllowGenericShader ( const char *name, const char *text )
{
	if (IsNonDetectionMap()) return qfalse;

	//if (StringContainsWord(name, "JH3-TE"))
	//	return qtrue;

	if (StringContainsWord(name, "icon"))
		return qfalse;

	if (StringContainsWord(name, "gfx/"))
		return qfalse;

	if (StringContainsWord(name, "menu/"))
		return qfalse;

	if (StringContainsWord(name, "ui/"))
		return qfalse;

	if (StringContainsWord(name, "gfx/water"))
		return qfalse;
	else if (StringContainsWord(name, "gfx/atmospheric"))
		return qfalse;
	else if ((StringContainsWord(name, "yavin/tree2b") || StringContainsWord(name, "yavin/tree05") || StringContainsWord(name, "yavin/tree06"))
		&& !(StringContainsWord(name, "yavin/tree05_vines") || StringContainsWord(name, "yavin/tree06b")))
		return qtrue;
	else if ((StringContainsWord(name, "yavin/tree08") || StringContainsWord(name, "yavin/tree09"))
		&& !(StringContainsWord(name, "yavin/tree08b") || StringContainsWord(name, "yavin/tree09_vines") || StringContainsWord(name, "yavin/tree09a") || StringContainsWord(name, "yavin/tree09b") || StringContainsWord(name, "yavin/tree09d")))
		return qtrue;

	if (StringContainsWord(name, "raindrop")
		|| StringContainsWord(name, "gfx/effects/bubble")
		|| StringContainsWord(name, "gfx/water/screen_ripple")
		|| StringContainsWord(name, "gfx/water/alpha_bubbles")
		|| StringContainsWord(name, "gfx/water/overlay_bubbles"))
		return qfalse;
	else if (StringContainsWord(name, "vjun/vj4") || StringContainsWord(name, "vjun\\vj4"))
		return qtrue;
	else if (StringContainsWord(name, "warzone/foliage") || StringContainsWord(name, "warzone\\foliage"))
		return qtrue;
	else if (StringContainsWord(name, "warzone/tree") || StringContainsWord(name, "warzone\\tree"))
	{
		if (text && !StringContainsWord(text, "q3map_material"))
			return qtrue;
	}
	else if (StringContainsWord(name, "warzone/test_trees") || StringContainsWord(name, "warzone\\test_trees"))
	{
		if (text && !StringContainsWord(text, "q3map_material"))
			return qtrue;
	}
	else if ( StringContainsWord(name, "warzone/billboard") || StringContainsWord(name, "warzone\\billboard"))
		return qtrue;
	else if (text && (StringsContainWord(name, text, "gfx")))
		return qfalse;
	else if (text && (StringsContainWord(name, text, "glow") || StringContainsWord(name, "icon")))
		return qfalse;
	else if (!text && (StringContainsWord(name, "glow") || StringContainsWord(name, "icon")))
		return qfalse;
	else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet"))
		return qtrue;
	else if (StringContainsWord(name, "mp/flag") || StringContainsWord(name, "/atat/") || StringContainsWord(name, "xwing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_") || StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon") || StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate") || StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train") || StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
		return qtrue;
	else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
		return qtrue;
	else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
		return qtrue;

	if (!strncmp(name, "textures/", 9) || !strncmp(name, "models/", 7))
		return qtrue;

	return qfalse;
}

#endif //__SHADER_GENERATOR__

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/

//#include <mutex>
//std::mutex findshader_lock;

const char *R_ReplaceShader(const char *name)
{
	//extern char				MAP_REPLACE_SHADERS_ORIGINAL[16][MAX_QPATH];
	//extern char				MAP_REPLACE_SHADERS_NEW[16][MAX_QPATH];

	for (int i = 0; i < 16; i++)
	{
		if (MAP_REPLACE_SHADERS_ORIGINAL[i][0] == 0)
			continue;

		if (strlen(name) <= 0)
			continue;

		if (!strcmp(name, MAP_REPLACE_SHADERS_ORIGINAL[i]))
		{
			return MAP_REPLACE_SHADERS_NEW[i];
		}
	}

	return name;
}

shader_t *R_FindShader( const char *shaderName, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage ) {
	char		strippedName[MAX_IMAGE_PATH];
	int			hash, flags;
	const char	*shaderText;
	image_t		*image;
	shader_t	*sh;
#ifdef __SHADER_GENERATOR__
	char		myShader[1024] = {0};
#endif //__SHADER_GENERATOR__

	const char *name = R_ReplaceShader(shaderName);


	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}


	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndexes[0] >= 0 && lightmapIndexes[0] >= tr.numLightmaps ) {
		lightmapIndexes = lightmapsVertex;
	} else if ( lightmapIndexes[0] < LIGHTMAP_2D ) {
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		ri->Printf( PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndexes[0]  );
		lightmapIndexes = lightmapsVertex;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, strippedName, lightmapIndexes, styles)) {
			// match found
			return sh;
		}
	}

#ifdef __DEBUG_SHADER_LOADING__
	ri->Printf(PRINT_ALL, "Loading new shader %s.\n", name);
#endif //__DEBUG_SHADER_LOADING__

	//findshader_lock.lock();

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	Com_Memcpy(shader.lightmapIndex, lightmapIndexes, sizeof(shader.lightmapIndex));
	Com_Memcpy(shader.styles, styles, sizeof(shader.styles));

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = NULL;
	shaderText = FindShaderInShaderText(strippedName);

	qboolean isEfxShader = qfalse;

	if (StringContainsWord(strippedName, "gfx/") || StringContainsWord(strippedName, "gfx_base/"))
	{
		isEfxShader = qtrue;
	}
	else if (StringContainsWord(strippedName, "magicParticles") || StringContainsWord(strippedName, "fireflies"))
	{
		isEfxShader = qtrue;
	}

#ifdef __SHADER_GENERATOR__
	qboolean		forceShaderFileUsage = qfalse;
	qboolean		haveImage = qfalse;
	int				material = DetectMaterialType(name);
	qboolean		allowGeneric = R_AllowGenericShader(name, shaderText);

	if (StringContainsWord(strippedName, "gfx/") || StringContainsWord(strippedName, "gfx_base/"))
	{
		forceShaderFileUsage = qtrue;
	}
	else if (StringContainsWord(strippedName, "magicParticles") || StringContainsWord(strippedName, "fireflies"))
	{
		forceShaderFileUsage = qtrue;
	}

	if (!Q_stricmp(strippedName, "textures/system")
		|| StringContainsWord(strippedName, "nodraw_solid")
		|| StringContainsWord(strippedName, "collision")
		|| StringContainsWord(shader.name, "noshader")
		/*|| StringContainsWord(strippedName, "textures/common")*/)
	{
		forceShaderFileUsage = qtrue;
	}

	if (allowGeneric || !shaderText)
	{
		flags = IMGFLAG_NONE;

		if (r_srgb->integer)
			flags |= IMGFLAG_SRGB;

		if (mipRawImage)
		{
			flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

			//if (r_genNormalMaps->integer)
			flags |= IMGFLAG_GENNORMALMAP;
		}
		else
		{
			flags |= IMGFLAG_CLAMPTOEDGE;
		}

#ifdef __DEFERRED_IMAGE_LOADING__
		image = R_DeferImageLoad(strippedName, IMGTYPE_COLORALPHA, flags);
#else //!__DEFERRED_IMAGE_LOADING__
		image = R_FindImageFile(strippedName, IMGTYPE_COLORALPHA, flags);
#endif //__DEFERRED_IMAGE_LOADING__

		if ((image && image != tr.defaultImage) || R_TIL_TextureFileExists(strippedName))
		{
			haveImage = qtrue;
		}
	}

	if (shaderText)
	{// See if it is warzone enabled by parsing it, if so, use it, otherwise try warzone generic shaders, etc...
		/*if (StringContains((char *)name, "gfx/effects/sabers/saberTrail", 0)) {
			ri->Printf(PRINT_ALL, "*PARSE SHADER* %s\n", name);
			ri->Printf(PRINT_ALL, "%s\n", shaderText);
		}*/

		if (ParseShader(name, &shaderText))
		{
			if (shader.warzoneEnabled || !allowGeneric || forceShaderFileUsage)
			{// It's a proper warzone shader, use it...
				sh = FinishShader();

				switch (sh->materialType)
				{
				case MATERIAL_DISTORTEDGLASS:
					break;
				case MATERIAL_DISTORTEDPUSH:
					break;
				case MATERIAL_DISTORTEDPULL:
					break;
				case MATERIAL_CLOAK:
					break;
				case MATERIAL_FORCEFIELD:
					break;
				default:
					if (isEfxShader && sh->materialType != MATERIAL_MENU_BACKGROUND) sh->materialType = MATERIAL_EFX;
					if (sh->materialType == MATERIAL_NONE) sh->materialType = material;
					break;
				}

				if (r_genericShaderDebug->integer && allowGeneric && !StringContainsWord(name, "models/player") && !StringContainsWord(name, "models/weapon")) // skip this spam for now...
				{
					ri->Printf(PRINT_WARNING, "AGS SKIPPED because %s shader is a warzoneEnabled shader.\n", strippedName);
				}

				if (r_printShaders->integer || r_genericShaderDebug->integer) {
					ri->Printf(PRINT_ALL, "*NON-AGS warzoneEnabled/forcedUsage SHADER* %s\n", name);

					/*if (StringContains((char *)name, "gfx/effects/sabers/saberTrail", 0)) {
						ri->Printf(PRINT_ALL, "%s\n", shaderText);
					}*/
				}

				//findshader_lock.unlock();
				return sh;
			}
			else if (!haveImage)
			{// Todo, store diffuse tex name from this shader and try below?
				sh = FinishShader();

				switch (sh->materialType)
				{
				case MATERIAL_DISTORTEDGLASS:
					break;
				case MATERIAL_DISTORTEDPUSH:
					break;
				case MATERIAL_DISTORTEDPULL:
					break;
				case MATERIAL_CLOAK:
					break;
				case MATERIAL_FORCEFIELD:
					break;
				default:
					if (isEfxShader && sh->materialType != MATERIAL_MENU_BACKGROUND) sh->materialType = MATERIAL_EFX;
					if (sh->materialType == MATERIAL_NONE) sh->materialType = material;
					break;
				}

				if (r_genericShaderDebug->integer && allowGeneric && !StringContainsWord(name, "models/player") && !StringContainsWord(name, "models/weapon")) // skip this spam for now...
				{
					ri->Printf(PRINT_WARNING, "AGS SKIPPED because %s texture does not exist.\n", strippedName);
				}

				if (r_printShaders->integer || r_genericShaderDebug->integer) {
					ri->Printf(PRINT_ALL, "*NON-AGS No-Texture SHADER* %s\n", name);

					/*if (r_printShaders->integer >= 2 || r_genericShaderDebug->integer >= 2) {
						ri->Printf(PRINT_ALL, "%s\n", shaderText);
					}*/
				}

				//findshader_lock.unlock();
				return sh;
			}
			else if (shader.hasGlow)
			{
				sh = FinishShader();

				switch (sh->materialType)
				{
				case MATERIAL_DISTORTEDGLASS:
					break;
				case MATERIAL_DISTORTEDPUSH:
					break;
				case MATERIAL_DISTORTEDPULL:
					break;
				case MATERIAL_CLOAK:
					break;
				case MATERIAL_FORCEFIELD:
					break;
				default:
					if (isEfxShader && sh->materialType != MATERIAL_MENU_BACKGROUND) sh->materialType = MATERIAL_EFX;
					if (sh->materialType == MATERIAL_NONE) sh->materialType = material;
					break;
				}

				if (r_genericShaderDebug->integer && allowGeneric && !StringContainsWord(name, "models/player") && !StringContainsWord(name, "models/weapon")) // skip this spam for now...
				{
					ri->Printf(PRINT_WARNING, "AGS SKIPPED because %s original had a glow stage.\n", strippedName);
				}

				if (r_printShaders->integer || r_genericShaderDebug->integer) {
					ri->Printf(PRINT_ALL, "*NON-AGS GLOW SHADER* %s\n", name);

					/*if (r_printShaders->integer >= 2 || r_genericShaderDebug->integer >= 2) {
						ri->Printf(PRINT_ALL, "%s\n", shaderText);
					}*/
				}

				//findshader_lock.unlock();
				return sh;
			}
			else
			{// Not a warzone shader, and is allowed to use AGS, so clear it and let warzone try it's stuff...
				if (r_genericShaderDebug->integer > 2)
				{
					ri->Printf(PRINT_WARNING, "AGS is attempting to create shader for %s.\n", name);
				}

				// clear the global shader
				ClearGlobalShader();
				Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
				Com_Memcpy(shader.lightmapIndex, lightmapIndexes, sizeof(shader.lightmapIndex));
				Com_Memcpy(shader.styles, styles, sizeof(shader.styles));
			}
		}
		else
		{
			if (r_genericShaderDebug->integer && allowGeneric && !StringContainsWord(name, "models/player") && !StringContainsWord(name, "models/weapon")) // skip this spam for now...
			{
				ri->Printf(PRINT_WARNING, "AGS failed to pass shader %s.\n", name);

				/*if (r_genericShaderDebug->integer > 2)
				{
					ri->Printf(PRINT_ALL, "*AGS FAILED SHADER* %s\n", name);
					ri->Printf(PRINT_ALL, "%s\n", shaderText);
				}*/
			}

			// clear the global shader
			ClearGlobalShader();
			Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
			Com_Memcpy(shader.lightmapIndex, lightmapIndexes, sizeof(shader.lightmapIndex));
			Com_Memcpy(shader.styles, styles, sizeof(shader.styles));
		}
	}

	if (haveImage && (allowGeneric || (!shaderText && !StringContainsWord(strippedName, "icon") && (!strncmp(strippedName, "textures/", 9) || !strncmp(strippedName, "models/", 7)))))
	{// Use the generic shader construction system...
		extern qboolean MAP_LIGHTMAP_DISABLED;

		char lightMapText[512] = { 0 };
		if (!MAP_LIGHTMAP_DISABLED)
		{// Enable the lightmap section...
			strcpy(lightMapText, uniqueGenericLightmap);
		}

		char shaderCustomMap[512] = { 0 };
		int material = DetectMaterialType(name);

		shader.defaultShader = qfalse;

		// Check if this texture has a _glow component...
		char glowName[MAX_IMAGE_PATH] = { 0 };
		char glowName2[MAX_IMAGE_PATH] = { 0 };
		char glowName3[MAX_IMAGE_PATH] = { 0 };
		char glowName4[MAX_IMAGE_PATH] = { 0 };

		// Do we have a glow?
		sprintf(glowName, "%s_glow", strippedName);
		sprintf(glowName2, "%s_glw", strippedName);
		sprintf(glowName3, "%sglow", strippedName);
		sprintf(glowName4, "%sglw", strippedName);

		if (R_TextureFileExists(glowName) || R_TIL_TextureFileExists(glowName))
		{
			sprintf(shaderCustomMap, uniqueGenericGlow, glowName);
		}
		else if (R_TextureFileExists(glowName2) || R_TIL_TextureFileExists(glowName2))
		{
			sprintf(shaderCustomMap, uniqueGenericGlow, glowName2);
		}
		else if (R_TextureFileExists(glowName3) || R_TIL_TextureFileExists(glowName3))
		{
			sprintf(shaderCustomMap, uniqueGenericGlow, glowName3);
		}
		else if (R_TextureFileExists(glowName4) || R_TIL_TextureFileExists(glowName4))
		{
			sprintf(shaderCustomMap, uniqueGenericGlow, glowName4);
		}

#ifdef __USE_SKYMAP_STAGES__
		//if (shaderCustomMap[0] == 0)
		{// No glow? Add sky reflection map...
			vec4_t settings;
			RB_PBR_DefaultsForMaterial(settings, material);
			float reflectionStrength = settings[1] * settings[2];

			if (reflectionStrength > 0.0)
			{// If the reflectiveness of this material is high enough, add sky reflection...
				char shaderCustomAdditionMap[256] = { 0 };
				reflectionStrength = reflectionStrength * 0.75 + 0.25;
				sprintf(shaderCustomAdditionMap, uniqueGenericSkyMap, reflectionStrength);
				sprintf(shaderCustomMap, "%s%s", shaderCustomMap, shaderCustomAdditionMap);
			}
		}
#endif //__USE_SKYMAP_STAGES__

		// Generate the shader...
		if (StringContainsWord(strippedName, "warzone/billboard") || StringContainsWord(strippedName, "warzone\\billboard"))
		{
			sprintf(myShader, uniqueGenericFoliageBillboardShader, strippedName, strippedName);
		}
		else if ((StringContainsWord(name, "yavin/tree2b") || StringContainsWord(name, "yavin/tree05") || StringContainsWord(name, "yavin/tree06"))
			&& !(StringContainsWord(name, "yavin/tree05_vines") || StringContainsWord(name, "yavin/tree06b")))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if ((StringContainsWord(name, "yavin/tree08") || StringContainsWord(name, "yavin/tree09"))
			&& !(StringContainsWord(name, "yavin/tree08b") || StringContainsWord(name, "yavin/tree09_vines") || StringContainsWord(name, "yavin/tree09a") || StringContainsWord(name, "yavin/tree09b") || StringContainsWord(name, "yavin/tree09d")))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "warzone/foliage") || StringContainsWord(strippedName, "warzone\\foliage") || StringContainsWord(name, "warzone/plant") || StringContainsWord(name, "warzone/bushes"))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "yavin/grass") || StringContainsWord(strippedName, "yavin\\grass")
			|| StringContainsWord(strippedName, "yavin/tree_leaves") || StringContainsWord(strippedName, "yavin\\tree_leaves")
			|| StringContainsWord(strippedName, "yavin/tree1") || StringContainsWord(strippedName, "yavin\\tree1")
			|| StringContainsWord(strippedName, "yavin/vine") || StringContainsWord(strippedName, "yavin\\vine"))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "warzone/plants") || StringContainsWord(strippedName, "warzone\\plants") || StringContainsWord(strippedName, "warzone\\bushes"))
		{
			sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
		}
		else if (StringContainsWord(strippedName, "warzone/tree") || StringContainsWord(strippedName, "warzone\\tree")
			|| StringContainsWord(strippedName, "warzone/deadtree") || StringContainsWord(strippedName, "warzone\\deadtree"))
		{
			if (StringContainsWord(strippedName, "bark")
				|| StringContainsWord(strippedName, "trunk")
				|| StringContainsWord(strippedName, "giant_tree")
				|| StringContainsWord(strippedName, "vine01")
				|| StringContainsWord(strippedName, "/palm0"))
			{
				sprintf(myShader, uniqueGenericFoliageTreeBarkShader, strippedName, strippedName, "");
			}
			else
			{
				//sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
				sprintf(myShader, uniqueGenericFoliageLeafsShader, strippedName, strippedName, "");
			}
		}
		else if (StringContainsWord(strippedName, "warzone/test_trees") || StringContainsWord(strippedName, "warzone\\test_trees"))
		{
			if (StringContainsWord(strippedName, "bark") || StringContainsWord(strippedName, "trunk"))
			{
				sprintf(myShader, uniqueGenericFoliageTreeBarkShader, strippedName, strippedName, "");
			}
			else
			{
				//sprintf(myShader, uniqueGenericFoliageShader, strippedName, strippedName);
				sprintf(myShader, uniqueGenericFoliageLeafsShader, strippedName, strippedName, "");
			}
		}
		else if (StringContainsWord(name, "models/weapon"))
		{
			sprintf(myShader, uniqueGenericWeaponShader, strippedName, strippedName, shaderCustomMap, strippedName);
		}
		else if (material == MATERIAL_ARMOR)
		{
			sprintf(myShader, uniqueGenericArmorShader, strippedName, strippedName, shaderCustomMap, strippedName);
		}
		else if (StringContainsWord(strippedName, "players/hk"))
		{
			sprintf(myShader, uniqueGenericMetalShader, strippedName, strippedName, shaderCustomMap, lightMapText);
		}
		else if (StringContainsWord(strippedName, "player"))
		{
			sprintf(myShader, uniqueGenericPlayerShader, strippedName, strippedName, shaderCustomMap, strippedName);
		}
		else if (StringContainsWord(strippedName, "weapon") || StringContainsWord(strippedName, "/atat/") || material == MATERIAL_SOLIDMETAL || material == MATERIAL_HOLLOWMETAL)
		{
			sprintf(myShader, uniqueGenericMetalShader, strippedName, strippedName, shaderCustomMap, lightMapText);
		}
		else if (material == MATERIAL_ROCK || material == MATERIAL_STONE || StringContainsWord(name, "warzone/rocks"))
		{
			sprintf(myShader, uniqueGenericRockShader, strippedName, strippedName, shaderCustomMap, strippedName);
		}
		else if (StringContainsWord(name, "vjun/vj4"))
		{
			if (StringContainsWord(name, "vjun/vj4_b"))
			{// pff
				char realName[128];
				sprintf(realName, "models/map_objects/vjun/vj4");
				sprintf(myShader, uniqueGenericRockShader, strippedName, realName, shaderCustomMap, realName);
			}
			else
			{
				sprintf(myShader, uniqueGenericRockShader, strippedName, strippedName, shaderCustomMap, strippedName);
			}
		}
		else
		{
			sprintf(myShader, uniqueGenericShader, strippedName, strippedName, shaderCustomMap, lightMapText);
		}

		//
		// attempt to define shader from an explicit parameter file
		//
		const char *shaderText2 = myShader;

		if (shaderText2)
		{
			if (ParseShader(name, &shaderText2))
			{
				if (r_genericShaderDebug->integer && allowGeneric && !StringContainsWord(name, "models/player") && !StringContainsWord(name, "models/weapon")) // skip this spam for now...
				{
					ri->Printf(PRINT_WARNING, "AGS generated for %s. Detected material %s.\n", strippedName, materialNames[material]);

					if (r_genericShaderDebug->integer > 2)
					{
						ri->Printf(PRINT_ALL, "\n*AGS SHADER* %s\n", strippedName);
						ri->Printf(PRINT_ALL, "%s\n", myShader);
					}
				}

				sh = FinishShader();

				if (isEfxShader && sh->materialType != MATERIAL_MENU_BACKGROUND) sh->materialType = MATERIAL_EFX;
				if (sh->materialType == MATERIAL_NONE) sh->materialType = material;

				//findshader_lock.unlock();
				return sh;
			}
			else
			{
				ri->Printf(PRINT_WARNING, "AGS generation failed for %s. Detected material %s. Falling back to JKA system. Generated source follows.\n", strippedName, materialNames[material]);
				ri->Printf(PRINT_WARNING, "*AGS FAILED SHADER* %s\n", strippedName);
				ri->Printf(PRINT_WARNING, "%s\n", myShader);
			}
		}

		// clear the global shader
		ClearGlobalShader();
		Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
		Com_Memcpy(shader.lightmapIndex, lightmapIndexes, sizeof(shader.lightmapIndex));
		Com_Memcpy(shader.styles, styles, sizeof(shader.styles));
	}
	else
	{
		if (r_genericShaderDebug->integer > 1 && allowGeneric && !StringContainsWord(name, "models/player") && !StringContainsWord(name, "models/weapon")) // skip this spam for now...
		{
			if (!haveImage)
				ri->Printf(PRINT_WARNING, "AGS failed because %s texture does not exist.\n", strippedName);
			else if (!allowGeneric)
				ri->Printf(PRINT_WARNING, "AGS failed because %s is not allowed to use generic shader.\n", strippedName);
			else if (!shaderText && !StringContainsWord(strippedName, "icon") && (!strncmp(strippedName, "textures/", 9) || !strncmp(strippedName, "models/", 7)))
				ri->Printf(PRINT_WARNING, "AGS failed because %s is an icon or not textures/ or models/.\n", strippedName);
			else
				ri->Printf(PRINT_WARNING, "AGS failed because %s is, umm, hmmm ... rend2 hasn't been laid in too long and is grumpy...\n", strippedName);
		}
	}
#endif //__SHADER_GENERATOR__

	//
	// Use original shader...
	//

	shader.defaultShader = qfalse;

	shaderText = NULL;
	shaderText = FindShaderInShaderText(strippedName);

	if (shaderText)
	{
		// clear the global shader
		ClearGlobalShader();
		Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
		Com_Memcpy(shader.lightmapIndex, lightmapIndexes, sizeof(shader.lightmapIndex));
		Com_Memcpy(shader.styles, styles, sizeof(shader.styles));

		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if (r_printShaders->integer || r_genericShaderDebug->integer) {
			ri->Printf(PRINT_ALL, "*NON-AGS SHADER* %s\n", name);

			if (r_printShaders->integer >= 2 || r_genericShaderDebug->integer >= 2) {
				ri->Printf(PRINT_ALL, "%s\n", shaderText);
			}
		}

		if (!ParseShader(name, &shaderText))
		{
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}
		else
		{
			sh = FinishShader();

			switch (sh->materialType)
			{
			case MATERIAL_DISTORTEDGLASS:
				break;
			case MATERIAL_DISTORTEDPUSH:
				break;
			case MATERIAL_DISTORTEDPULL:
				break;
			case MATERIAL_CLOAK:
				break;
			case MATERIAL_FORCEFIELD:
				break;
			default:
				if (isEfxShader && sh->materialType != MATERIAL_MENU_BACKGROUND) sh->materialType = MATERIAL_EFX;
				break;
			}

			if (sh->materialType == MATERIAL_NONE)
			{
				int material = DetectMaterialType(name);
				sh->materialType = material;
			}

			//findshader_lock.unlock();
			return sh;
		}
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	//

	flags = IMGFLAG_NONE;

	if (r_srgb->integer)
		flags |= IMGFLAG_SRGB;

	if (mipRawImage)
	{
		flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

		//if (r_genNormalMaps->integer)
		flags |= IMGFLAG_GENNORMALMAP;
	}
	else
	{
		flags |= IMGFLAG_CLAMPTOEDGE;
	}

#ifdef __DEFERRED_IMAGE_LOADING__
	image = R_DeferImageLoad(name, IMGTYPE_COLORALPHA, flags);
#else //!__DEFERRED_IMAGE_LOADING__
	image = R_FindImageFile(name, IMGTYPE_COLORALPHA, flags);
#endif //__DEFERRED_IMAGE_LOADING__

	if (!image) {
#ifdef __DEVELOPER_MODE__
		ri->Printf(PRINT_DEVELOPER, "Couldn't find image file for shader %s\n", name);
#endif //__DEVELOPER_MODE__
		shader.defaultShader = qtrue;
		//findshader_lock.unlock();
		return FinishShader();
	}

	//
	// create the default shading commands
	//
	if (shader.lightmapIndex[0] == LIGHTMAP_NONE) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_2D) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			GLS_SRCBLEND_SRC_ALPHA |
			GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}
	else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	if (isEfxShader && shader.materialType != MATERIAL_MENU_BACKGROUND) shader.materialType = MATERIAL_EFX;
#ifndef __SHADER_GENERATOR__
	int material = DetectMaterialType(name);
#endif //__SHADER_GENERATOR__
	if (shader.materialType == MATERIAL_NONE) shader.materialType = material;

	//findshader_lock.unlock();
	return FinishShader();
}

shader_t *R_ReloadShader(const char *name) {
	char		strippedName[MAX_IMAGE_PATH];
	int			hash;
	shader_t	*sh;

	if ((name == NULL) || (name[0] == 0)) {
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0) {


			int i, hash;

			hash = generateHashValue(name, MAX_SHADERTEXT_HASH);

			if (shaderTextHashTable[hash])
			{
				for (i = 0; shaderTextHashTable[hash][i]; i++)
				{
					char *p = shaderTextHashTable[hash][i];
					char *token = COM_ParseExt((const char **)&p, qtrue);

					if (!Q_stricmp(token, name))
					{
						memset(p, 0, sizeof(char) * strlen(p));
						break;
					}
				}
			}



			shader_t *next = sh->next;
			int index = sh->index;
			int sortedIndex = sh->sortedIndex;

			// match found. Blank it...
			memset(sh, 0, sizeof(shader_t));
			// Load the shader again...
			shader_t *sh2 = R_FindShader(name, lightmapsNone, stylesDefault, qtrue);
			// Copy the new one over the old...
			memcpy(sh, sh2, sizeof(shader_t));
			// Give it the original's next value.
			sh->next = next;
			sh->index = index;
			sh->sortedIndex = sortedIndex;
			// Blank the original copy of the new one, so it can be reused by something else...
			memset(sh2, 0, sizeof(shader_t));
			// Return the new copy.
			return sh;
		}
	}

	// Just load it...
	return R_FindShader(name, lightmapsNone, stylesDefault, qtrue);
}

shader_t *R_FindServerShader( const char *name, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage )
{
	char		strippedName[MAX_IMAGE_PATH];
	int			hash;
	shader_t	*sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	COM_StripExtension( name, strippedName, sizeof( strippedName ) );

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, name, lightmapIndexes, styles) ) {
			// match found
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));

	shader.defaultShader = qtrue;
	return FinishShader();
}

qhandle_t RE_RegisterShaderFromImage(const char *name, const int *lightmapIndexes, const byte *styles, image_t *image, qboolean mipRawImage) {
	int			hash;
	shader_t	*sh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// probably not necessary since this function
	// only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	// but better safe than sorry.
	if ( lightmapIndexes[0] >= tr.numLightmaps ) {
		lightmapIndexes = lightmapsFullBright;
	}

	//
	// see if the shader is already loaded
	//

	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( IsShader (sh, name, lightmapIndexes, styles) ) {
			// match found
			return sh->index;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	Com_Memcpy (shader.lightmapIndex, lightmapIndexes, sizeof (shader.lightmapIndex));

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex[0] == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, const int *lightmapIndexes, const byte *styles ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_IMAGE_PATH) {
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_IMAGE_PATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndexes, styles, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_IMAGE_PATH) {
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_IMAGE_PATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmaps2d, stylesDefault, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_IMAGE_PATH) {
		ri->Printf( PRINT_ALL, "Shader name exceeds MAX_IMAGE_PATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmaps2d, stylesDefault, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}

//added for ui -rww
const char *RE_ShaderNameFromIndex(int index)
{
	assert(index >= 0 && index < tr.numShaders && tr.shaders[index]);
	return tr.shaders[index]->name;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
	  ri->Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri->Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void	R_ShaderList_f (void) {
	int			i;
	int			count;
	shader_t	*shader;

	ri->Printf (PRINT_ALL, "^5-----------------------\n");

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri->Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri->Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if (shader->lightmapIndex[0] || shader->lightmapIndex[1] || shader->lightmapIndex[2] || shader->lightmapIndex[3] ) {
			ri->Printf (PRINT_ALL, "L ");
		} else {
			ri->Printf (PRINT_ALL, "  ");
		}

		if ( shader->explicitlyDefined ) {
			ri->Printf( PRINT_ALL, "E " );
		} else {
			ri->Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri->Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri->Printf( PRINT_ALL, "sky " );
		} else {
			ri->Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri->Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name);
		} else {
			ri->Printf (PRINT_ALL,  ": %s\n", shader->name);
		}
		count++;
	}
	ri->Printf (PRINT_ALL, "%i total shaders\n", count);
	ri->Printf (PRINT_ALL, "^5------------------\n");
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define	MAX_SHADER_FILES	4096
static void ScanAndLoadShaderFiles( void )
{
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	const char *p;
	int numShaderFiles;
	int i;
	char *oldp, *token, *hashMem, *textEnd;
	int shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;
	char shaderName[MAX_IMAGE_PATH];
	int shaderLine;

	long sum = 0, summand;
	// scan for shader files
	shaderFiles = ri->FS_ListFiles( "shaders", ".shader", &numShaderFiles );

	if ( !shaderFiles || !numShaderFiles )
	{
		ri->Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaderFiles > MAX_SHADER_FILES ) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ )
	{
		char filename[MAX_IMAGE_PATH];

		// look for a .mtr file first
		{
			char *ext;
			Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles[i] );
			if ( (ext = strrchr(filename, '.')) )
			{
				strcpy(ext, ".mtr");
			}

			fileHandle_t f;
			if ( /*ri->FS_ReadFile( filename, NULL ) <= 0*/ri->FS_FOpenFileRead(filename, &f, qtrue) ) // why read the whole file here ffs???
			{
				ri->FS_FCloseFile(f);
				Com_sprintf( filename, sizeof( filename ), "shaders/%s", shaderFiles[i] );
			}

			//ri->Printf(PRINT_WARNING, "%i: %s.\n", i, filename);
		}

#ifdef __DEVELOPER_MODE__
		ri->Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
#endif //__DEVELOPER_MODE__
		summand = ri->FS_ReadFile( filename, (void **)&buffers[i] );

		if ( !buffers[i] )
			ri->Error( ERR_DROP, "Couldn't load %s", filename );

		// Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders.
		p = buffers[i];
		COM_BeginParseSession(filename);
		while(1)
		{
			token = COM_ParseExt(&p, qtrue);

			if(!*token)
				break;

			Q_strncpyz(shaderName, token, sizeof(shaderName));
			shaderLine = COM_GetCurrentParseLine();

			token = COM_ParseExt(&p, qtrue);
			if(token[0] != '{' || token[1] != '\0')
			{
				ri->Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing opening brace",
							filename, shaderName, shaderLine);
				if (token[0])
				{
					ri->Printf(PRINT_WARNING, " (found \"%s\" on line %d)", token, COM_GetCurrentParseLine());
				}
				ri->Printf(PRINT_WARNING, ".\n");
				ri->FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			if(!SkipBracedSection_Depth(&p, 1))
			{
				ri->Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing closing brace.\n",
							filename, shaderName, shaderLine);
				ri->FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}
		}


		if (buffers[i])
			sum += summand;
	}

	// build single large buffer
	s_shaderText = (char *)ri->Hunk_Alloc( sum + numShaderFiles*2, h_low );
	s_shaderText[ 0 ] = '\0';
	textEnd = s_shaderText;

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaderFiles - 1; i >= 0 ; i-- )
	{
		if ( !buffers[i] )
			continue;

		strcat( textEnd, buffers[i] );
		strcat( textEnd, "\n" );
		textEnd += strlen( textEnd );
		ri->FS_FreeFile( buffers[i] );
	}

	COM_Compress( s_shaderText );

	// free up memory
	ri->FS_FreeFileList( shaderFiles );

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		SkipBracedSection(&p);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char *)ri->Hunk_Alloc( size * sizeof(char *), h_low );

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (char **) hashMem;
		hashMem = ((char *) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		oldp = (char *)p;
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		SkipBracedSection(&p);
	}

	return;

}

#ifdef __XYC_SURFACE_SPRITES__
shader_t *R_CreateShaderFromTextureBundle(
	const char *name,
	const textureBundle_t *bundle,
	uint32_t stateBits)
{
	shader_t *result = R_FindShaderByName(name);

	if (!result) result = tr.defaultShader;

	if (result == tr.defaultShader)
	{
		Com_Memset(&shader, 0, sizeof(shader));
		Com_Memset(&stages, 0, sizeof(stages));

		Q_strncpyz(shader.name, name, sizeof(shader.name));

		stages[0].active = qtrue;
		stages[0].bundle[0] = *bundle;
		stages[0].stateBits = stateBits;
		result = FinishShader();
	}
	return result;
}
#endif //__XYC_SURFACE_SPRITES__

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	Com_Memcpy (shader.lightmapIndex, lightmapsNone, sizeof (shader.lightmapIndex));
	for ( int i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz(shader.name, "<stencil shadow>", sizeof(shader.name));
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();

	// distortion shader is just a marker
	Q_strncpyz( shader.name, "internal_distortion", sizeof( shader.name ) );
	shader.sort = SS_BLEND0;
	shader.defaultShader = qfalse;
	tr.distortionShader = FinishShader();
	shader.defaultShader = qtrue;
}

static void CreateExternalShaders( void ) {
	tr.projectionShadowShader = R_FindShader( "projectionShadow", lightmapsNone, stylesDefault, qtrue );
	tr.flareShader = R_FindShader( "flareShader", lightmapsNone, stylesDefault, qtrue );

	// Hack to make fogging work correctly on flares. Fog colors are calculated
	// in tr_flare.c already.
	if(!tr.flareShader->defaultShader)
	{
		int index;

		for(index = 0; index < tr.flareShader->numUnfoggedPasses; index++)
		{
			tr.flareShader->stages[index]->adjustColorsForFog = ACFF_NONE;
			tr.flareShader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
		}
	}

	tr.sunShader = R_FindShader( "sun", lightmapsNone, stylesDefault, qtrue );
	tr.sunShader->isSky = qtrue;
	tr.sunFlareShader = R_FindShader( "gfx/2d/sunflare", lightmapsNone, stylesDefault, qtrue);

	// HACK: if sunflare is missing, make one using the flare image or dlight image
	if (tr.sunFlareShader->defaultShader)
	{
		image_t *image;

		if (!tr.flareShader->defaultShader && tr.flareShader->stages[0] && tr.flareShader->stages[0]->bundle[0].image[0])
			image = tr.flareShader->stages[0]->bundle[0].image[0];
		else
			image = tr.dlightImage;

		Com_Memset( &shader, 0, sizeof( shader ) );
		Com_Memset( &stages, 0, sizeof( stages ) );

		Q_strncpyz( shader.name, "gfx/2d/sunflare", sizeof( shader.name ) );

		Com_Memcpy (shader.lightmapIndex, lightmapsNone, sizeof (shader.lightmapIndex));
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].stateBits = GLS_DEFAULT;
		tr.sunFlareShader = FinishShader();
	}

}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( qboolean server ) {
	ri->Printf( PRINT_ALL, "Initializing Shaders\n" );

	Com_Memset(hashTable, 0, sizeof(hashTable));

	if ( !server )
	{
		CreateInternalShaders();

		ScanAndLoadShaderFiles();

		CreateExternalShaders();
	}
}
