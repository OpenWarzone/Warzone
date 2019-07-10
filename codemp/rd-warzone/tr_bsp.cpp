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
// tr_map.c

#include "tr_local.h"


extern char currentMapName[128];

extern void R_LoadMapInfo ( void );

#if defined(__DEFERRED_IMAGE_LOADING__) && !defined(__DEFERRED_MAP_IMAGE_LOADING__)
extern qboolean DEFERRED_IMAGES_FORCE_LOAD;
#endif //defined(__DEFERRED_IMAGE_LOADING__) && !defined(__DEFERRED_MAP_IMAGE_LOADING__)

struct packedVertex_t
{
	vec3_t position;
	uint32_t normal;
	//uint32_t tangent;
	vec2_t texcoords[1 + MAXLIGHTMAPS];
#ifdef __VBO_PACK_COLOR__
	uint32_t colors[MAXLIGHTMAPS];
#elif defined(__VBO_HALF_FLOAT_COLOR__)
	hvec4_t colors[MAXLIGHTMAPS];
#else //!__VBO_PACK_COLOR__
	vec4_t colors[MAXLIGHTMAPS];
#endif //__VBO_PACK_COLOR__
};

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

world_t				*s_worldData;
static	byte		*fileBase;

int			c_subdivisions;
int			c_gridVerts;

//===============================================================================

static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
static	void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int		shift, r, g, b;

	// shift the color data based on overbright range
	shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}


/*
===============
R_ColorShiftLightingFloats

===============
*/
static void R_ColorShiftLightingFloats(float in[4], float out[4], float scale )
{
	float r, g, b;

	scale *= pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits);

	r = in[0] * scale;
	g = in[1] * scale;
	b = in[2] * scale;

	if ( r > 1.0f || g > 1.0f || b > 1.0f )
	{
		float high = max (max (r, g), b);

		r /= high;
		g /= high;
		b /= high;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}


// Modified from http://graphicrants.blogspot.jp/2009/04/rgbm-color-encoding.html
void ColorToRGBM(const vec3_t color, unsigned char rgbm[4])
{
	vec3_t          sample;
	float			maxComponent;

	VectorCopy(color, sample);

	maxComponent = MAX(sample[0], sample[1]);
	maxComponent = MAX(maxComponent, sample[2]);
	maxComponent = CLAMP(maxComponent, 1.0f/255.0f, 1.0f);

	rgbm[3] = (unsigned char) ceil(maxComponent * 255.0f);
	maxComponent = 255.0f / rgbm[3];

	VectorScale(sample, maxComponent, sample);

	rgbm[0] = (unsigned char) (sample[0] * 255);
	rgbm[1] = (unsigned char) (sample[1] * 255);
	rgbm[2] = (unsigned char) (sample[2] * 255);
}

void ColorToRGBA16F(const vec3_t color, unsigned short rgba16f[4])
{
	rgba16f[0] = FloatToHalf(color[0]);
	rgba16f[1] = FloatToHalf(color[1]);
	rgba16f[2] = FloatToHalf(color[2]);
	rgba16f[3] = FloatToHalf(1.0f);
}


/*
===============
R_LoadLightmaps

===============
*/
#define	DEFAULT_LIGHTMAP_SIZE	128
#define MAX_LIGHTMAP_PAGES 2

extern qboolean MAP_LIGHTMAP_DISABLED;

static	void R_LoadLightmaps( lump_t *l, lump_t *surfs ) {
	byte		*buf, *buf_p;
	dsurface_t  *surf;
	int			len;
	byte		*image;
	int			i, j, numLightmaps, textureInternalFormat = 0;
	float maxIntensity = 0;
	double sumIntensity = 0;

	len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	if (MAP_LIGHTMAP_DISABLED)
	{
		tr.numLightmaps = 0;
		return;
	}

	// we are about to upload textures
	R_IssuePendingRenderCommands();

	tr.lightmapSize = DEFAULT_LIGHTMAP_SIZE;
	numLightmaps = len / (tr.lightmapSize * tr.lightmapSize * 3);

	// check for deluxe mapping
	if (numLightmaps <= 1)
	{
		tr.worldDeluxeMapping = qfalse;
	}
	else
	{
		tr.worldDeluxeMapping = qtrue;

		// Check that none of the deluxe maps are referenced by any of the map surfaces.
		for( i = 0, surf = (dsurface_t *)(fileBase + surfs->fileofs);
			tr.worldDeluxeMapping && i < surfs->filelen / sizeof(dsurface_t);
			i++, surf++ ) {
			for ( int j = 0; j < MAXLIGHTMAPS; j++ )
			{
				int lightmapNum = LittleLong( surf->lightmapNum[j] );

				if ( lightmapNum >= 0 && (lightmapNum & 1) != 0 ) {
					tr.worldDeluxeMapping = qfalse;
					break;
				}
			}
		}
	}

	image = (byte *)Z_Malloc(tr.lightmapSize * tr.lightmapSize * 4 * 2, TAG_BSP); // don't want to use TAG_BSP :< but it's the only one that fits

	if (tr.worldDeluxeMapping)
		numLightmaps >>= 1;

	if (r_mergeLightmaps->integer && numLightmaps >= 1024 )
	{
		// FIXME: fat light maps don't support more than 1024 light maps
		ri->Printf(PRINT_WARNING, "WARNING: number of lightmaps > 1024\n");
		numLightmaps = 1024;
	}

	// use fat lightmaps of an appropriate size
	if (r_mergeLightmaps->integer)
	{
		tr.fatLightmapSize = 512;
		tr.fatLightmapStep = tr.fatLightmapSize / tr.lightmapSize;

		// at most MAX_LIGHTMAP_PAGES
		while (tr.fatLightmapStep * tr.fatLightmapStep * MAX_LIGHTMAP_PAGES < numLightmaps && tr.fatLightmapSize != glConfig.maxTextureSize )
		{
			tr.fatLightmapSize <<= 1;
			tr.fatLightmapStep = tr.fatLightmapSize / tr.lightmapSize;
		}

		tr.numLightmaps = numLightmaps / (tr.fatLightmapStep * tr.fatLightmapStep);

		if (numLightmaps % (tr.fatLightmapStep * tr.fatLightmapStep) != 0)
			tr.numLightmaps++;
	}
	else
	{
		tr.numLightmaps = numLightmaps;
	}

	tr.lightmaps = (image_t **)ri->Hunk_Alloc( tr.numLightmaps * sizeof(image_t *), h_low );

	if (tr.worldDeluxeMapping)
	{
		tr.deluxemaps = (image_t **)ri->Hunk_Alloc( tr.numLightmaps * sizeof(image_t *), h_low );
	}

	if (glRefConfig.floatLightmap)
		textureInternalFormat = GL_RGBA16F;
	else
		textureInternalFormat = GL_RGBA8;

	if (r_mergeLightmaps->integer)
	{

		for (i = 0; i < tr.numLightmaps; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("_fatlightmap%d", i), NULL, tr.fatLightmapSize, tr.fatLightmapSize, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, textureInternalFormat );

			if (tr.worldDeluxeMapping)
			{
				tr.deluxemaps[i] = R_CreateImage(va("_fatdeluxemap%d", i), NULL, tr.fatLightmapSize, tr.fatLightmapSize, IMGTYPE_DELUXE, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
			}
		}
	}

	for(i = 0; i < numLightmaps; i++)
	{
		int xoff = 0, yoff = 0;
		int lightmapnum = i;
		// expand the 24 bit on-disk to 32 bit

		if (r_mergeLightmaps->integer)
		{
			int lightmaponpage = i % (tr.fatLightmapStep * tr.fatLightmapStep);
			xoff = (lightmaponpage % tr.fatLightmapStep) * tr.lightmapSize;
			yoff = (lightmaponpage / tr.fatLightmapStep) * tr.lightmapSize;

			lightmapnum /= (tr.fatLightmapStep * tr.fatLightmapStep);
		}

		// if (tr.worldLightmapping)
		{
			char filename[MAX_QPATH];
			byte *hdrLightmap = NULL;
			int size = 0;

			// look for hdr lightmaps
			if (r_hdr->integer)
			{
				Com_sprintf( filename, sizeof( filename ), "maps/%s/lm_%04d.hdr", s_worldData->baseName, i * (tr.worldDeluxeMapping ? 2 : 1) );
				//ri->Printf(PRINT_ALL, "looking for %s\n", filename);

				size = ri->FS_ReadFile(filename, (void **)&hdrLightmap);
			}

			if (hdrLightmap)
			{
				byte *p = hdrLightmap;
				//ri->Printf(PRINT_ALL, "found!\n");
				
				/* FIXME: don't just skip over this header and actually parse it */
				while (size && !(*p == '\n' && *(p+1) == '\n'))
				{
					size--;
					p++;
				}

				if (!size)
					ri->Error(ERR_DROP, "Bad header for %s!", filename);

				size -= 2;
				p += 2;
				
				while (size && !(*p == '\n'))
				{
					size--;
					p++;
				}

				size--;
				p++;

				buf_p = (byte *)p;

#if 0 // HDRFILE_RGBE
				if (size != tr.lightmapSize * tr.lightmapSize * 4)
					ri->Error(ERR_DROP, "Bad size for %s (%i)!", filename, size);
#else // HDRFILE_FLOAT
				if (size != tr.lightmapSize * tr.lightmapSize * 12)
					ri->Error(ERR_DROP, "Bad size for %s (%i)!", filename, size);
#endif
			}
			else
			{
				if (tr.worldDeluxeMapping)
					buf_p = buf + (i * 2) * tr.lightmapSize * tr.lightmapSize * 3;
				else
					buf_p = buf + i * tr.lightmapSize * tr.lightmapSize * 3;
			}


			for ( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ ) 
			{
				if (hdrLightmap)
				{
					vec4_t color;

#if 0 // HDRFILE_RGBE
					float exponent = exp2(buf_p[j*4+3] - 128);

					color[0] = buf_p[j*4+0] * exponent;
					color[1] = buf_p[j*4+1] * exponent;
					color[2] = buf_p[j*4+2] * exponent;
#else // HDRFILE_FLOAT
					memcpy(color, &buf_p[j*12], 12);

					color[0] = LittleFloat(color[0]);
					color[1] = LittleFloat(color[1]);
					color[2] = LittleFloat(color[2]);
#endif
					color[3] = 1.0f;

					R_ColorShiftLightingFloats(color, color, 1.0f/255.0f);

					if (glRefConfig.floatLightmap)
						ColorToRGBA16F(color, (unsigned short *)(&image[j*8]));
					else
						ColorToRGBM(color, &image[j*4]);
				}
				else if (glRefConfig.floatLightmap)
				{
					vec4_t color;

					//hack: convert LDR lightmap to HDR one
					color[0] = MAX(buf_p[j*3+0], 0.499f);
					color[1] = MAX(buf_p[j*3+1], 0.499f);
					color[2] = MAX(buf_p[j*3+2], 0.499f);

					// if under an arbitrary value (say 12) grey it out
					// this prevents weird splotches in dimly lit areas
					if (color[0] + color[1] + color[2] < 12.0f)
					{
						float avg = (color[0] + color[1] + color[2]) * 0.3333f;
						color[0] = avg;
						color[1] = avg;
						color[2] = avg;
					}
					color[3] = 1.0f;

					R_ColorShiftLightingFloats(color, color, 1.0f/255.0f);

					ColorToRGBA16F(color, (unsigned short *)(&image[j*8]));
				}
				else
				{
					if ( r_lightmap->integer == 2 )
					{	// color code by intensity as development tool	(FIXME: check range)
						float r = buf_p[j*3+0];
						float g = buf_p[j*3+1];
						float b = buf_p[j*3+2];
						float intensity;
						float out[3] = {0.0, 0.0, 0.0};

						intensity = 0.33f * r + 0.685f * g + 0.063f * b;

						if ( intensity > 255 )
							intensity = 1.0f;
						else
							intensity /= 255.0f;

						if ( intensity > maxIntensity )
							maxIntensity = intensity;

						HSVtoRGB( intensity, 1.00, 0.50, out );

						image[j*4+0] = out[0] * 255;
						image[j*4+1] = out[1] * 255;
						image[j*4+2] = out[2] * 255;
						image[j*4+3] = 255;

						sumIntensity += intensity;
					}
					else
					{
						R_ColorShiftLightingBytes( &buf_p[j*3], &image[j*4] );
						image[j*4+3] = 255;
					}
				}
			}

			if (r_mergeLightmaps->integer)
				R_UpdateSubImage(tr.lightmaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize);
			else
				tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, textureInternalFormat );

			if (hdrLightmap)
				ri->FS_FreeFile(hdrLightmap);
		}

		if (tr.worldDeluxeMapping)
		{
			buf_p = buf + (i * 2 + 1) * tr.lightmapSize * tr.lightmapSize * 3;


			for ( j = 0 ; j < tr.lightmapSize * tr.lightmapSize; j++ ) {
				image[j*4+0] = buf_p[j*3+0];
				image[j*4+1] = buf_p[j*3+1];
				image[j*4+2] = buf_p[j*3+2];

				// make 0,0,0 into 127,127,127
				if ((image[j*4+0] == 0) && (image[j*4+1] == 0) && (image[j*4+2] == 0))
				{
					image[j*4+0] =
					image[j*4+1] =
					image[j*4+2] = 127;
				}

				image[j*4+3] = 255;
			}

			if (r_mergeLightmaps->integer)
			{
				R_UpdateSubImage(tr.deluxemaps[lightmapnum], image, xoff, yoff, tr.lightmapSize, tr.lightmapSize );
			}
			else
			{
				tr.deluxemaps[i] = R_CreateImage(va("*deluxemap%d", i), image, tr.lightmapSize, tr.lightmapSize, IMGTYPE_DELUXE, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, 0 );
			}
		}
	}

	if ( r_lightmap->integer == 2 )	{
		ri->Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}

	Z_Free(image);
}


static float FatPackU(float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	if(tr.fatLightmapSize > 0)
	{
		int             x;

		lightmapnum %= (tr.fatLightmapStep * tr.fatLightmapStep);

		x = lightmapnum % tr.fatLightmapStep;

		return (input / ((float)tr.fatLightmapStep)) + ((1.0 / ((float)tr.fatLightmapStep)) * (float)x);
	}

	return input;
}

static float FatPackV(float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	if(tr.fatLightmapSize > 0)
	{
		int             y;

		lightmapnum %= (tr.fatLightmapStep * tr.fatLightmapStep);

		y = lightmapnum / tr.fatLightmapStep;

		return (input / ((float)tr.fatLightmapStep)) + ((1.0 / ((float)tr.fatLightmapStep)) * (float)y);
	}

	return input;
}


static int FatLightmap(int lightmapnum)
{
	if (lightmapnum < 0)
		return lightmapnum;

	if (tr.worldDeluxeMapping)
		lightmapnum >>= 1;

	if (tr.fatLightmapSize > 0)
	{
		return lightmapnum / (tr.fatLightmapStep * tr.fatLightmapStep);
	}
	
	return lightmapnum;
}

/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void		RE_SetWorldVisData( const byte *vis ) {
	tr.externalVisData = vis;
}


/*
=================
R_LoadVisibility
=================
*/
static	void R_LoadVisibility( lump_t *l, bool isMainWorld) {
#if 1
	int		len;
	byte	*buf;

	len = (s_worldData->numClusters + 63) & ~63;
	s_worldData->novis = (byte *)ri->Hunk_Alloc(len, h_low);
	Com_Memset(s_worldData->novis, 0xff, len);

	len = l->filelen;

	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData->numClusters = LittleLong( ((int *)buf)[0] );
	s_worldData->clusterBytes = LittleLong( ((int *)buf)[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData && !isMainWorld) {
		s_worldData->vis = (byte *)tr.externalVisData;
	} else {
		byte	*dest;

		dest = (byte *)ri->Hunk_Alloc( len - 8, h_low );
		Com_Memcpy( dest, buf + 8, len - 8 );
		s_worldData->vis = dest;
	}
#else
	s_worldData->vis = (byte *)ri->CM_GetVisibilityData();
	s_worldData->numClusters = (int)ri->CM_GetVisibilityDataClusterCount();
	s_worldData->clusterBytes = (int)ri->CM_GetVisibilityDataClusterBytesCount();
#endif
}

//===============================================================================


/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum, const int *lightmapNums, const byte *lightmapStyles, const byte *vertexStyles ) {
	shader_t	*shader;
	dshader_t	*dsh;
	const byte	*styles = lightmapStyles;

	int _shaderNum = LittleLong( shaderNum );
	if ( _shaderNum < 0 || _shaderNum >= s_worldData->numShaders ) {
		ri->Error( ERR_DROP, "ShaderForShaderNum: bad num %i", _shaderNum );
	}
	dsh = &s_worldData->shaders[ _shaderNum ];

	if ( lightmapNums[0] == LIGHTMAP_BY_VERTEX ) {
		styles = vertexStyles;
	}

	if ( r_vertexLight->integer ) {
		lightmapNums = lightmapsVertex;
		styles = vertexStyles;
	}

	if ( r_fullbright->integer ) {
		lightmapNums = lightmapsFullBright;
	}

#if defined(__DEFERRED_IMAGE_LOADING__) && !defined(__DEFERRED_MAP_IMAGE_LOADING__)
	DEFERRED_IMAGES_FORCE_LOAD = qtrue;
#endif //defined(__DEFERRED_IMAGE_LOADING__) && !defined(__DEFERRED_MAP_IMAGE_LOADING__)
	shader = R_FindShader( dsh->shader, lightmapNums, styles, qtrue );
#if defined(__DEFERRED_IMAGE_LOADING__) && !defined(__DEFERRED_MAP_IMAGE_LOADING__)
	DEFERRED_IMAGES_FORCE_LOAD = qfalse;
#endif //defined(__DEFERRED_IMAGE_LOADING__) && !defined(__DEFERRED_MAP_IMAGE_LOADING__)

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

#ifdef __REGENERATE_BSP_NORMALS__
// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

extern qboolean ENABLE_REGEN_SMOOTH_NORMALS;

#define MAX_SMOOTH_ERROR 0.8//1.0

bool ValidForSmoothing(vec3_t v1, vec3_t n1, vec3_t v2, vec3_t n2)
{
	if (Distance(n1, n2) < MAX_SMOOTH_ERROR)
	{
		return true;
	}

	return false;
}

void GenerateNormalsForBSPSurface(srfBspSurface_t *cv)
{
	if (!ENABLE_REGEN_SMOOTH_NORMALS)
	{// Not enabled for this map, skip...
		return;
	}

	//ri->Printf(PRINT_WARNING, "Regenerating BSP surface normals.\n");

//#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < cv->numIndexes; i += 3)
	{
		int tri[3];
		tri[0] = cv->indexes[i];
		tri[1] = cv->indexes[i + 1];
		tri[2] = cv->indexes[i + 2];

		if (tri[0] >= cv->numVerts)
		{
			//ri->Printf(PRINT_WARNING, "Shitty BSP index data - Bad index in face surface (vert) %i >=  (maxVerts) %i.", tri[0], cv->numVerts);
			return;
		}
		if (tri[1] >= cv->numVerts)
		{
			//ri->Printf(PRINT_WARNING, "Shitty BSP index data - Bad index in face surface (vert) %i >=  (maxVerts) %i.", tri[1], cv->numVerts);
			return;
		}
		if (tri[2] >= cv->numVerts)
		{
			//ri->Printf(PRINT_WARNING, "Shitty BSP index data - Bad index in face surface (vert) %i >=  (maxVerts) %i.", tri[2], cv->numVerts);
			return;
		}

		float* a = (float *)cv->verts[tri[0]].xyz;
		float* b = (float *)cv->verts[tri[1]].xyz;
		float* c = (float *)cv->verts[tri[2]].xyz;
		vec3_t ba, ca;
		VectorSubtract(b, a, ba);
		VectorSubtract(c, a, ca);

		vec3_t normal;
		CrossProduct(ca, ba, normal);
		VectorNormalize(normal);

		//ri->Printf(PRINT_WARNING, "OLD: %f %f %f. NEW: %f %f %f.\n", cv->verts[tri[0]].normal[0], cv->verts[tri[0]].normal[1], cv->verts[tri[0]].normal[2], normal[0], normal[1], normal[2]);

#pragma omp critical
		{
			VectorCopy(normal, cv->verts[tri[0]].normal);
			VectorCopy(normal, cv->verts[tri[1]].normal);
			VectorCopy(normal, cv->verts[tri[2]].normal);
		}
	}
}

void GenerateSmoothNormalsForSurface(srfBspSurface_t *cv)
{
	int numVerts = cv->numVerts;
	int numIndexes = cv->numIndexes;

	srfVert_t *verts = cv->verts;
	
#define __MULTIPASS_SMOOTHING__ // Looks better, but takes a lot longer...

#ifdef __MULTIPASS_SMOOTHING__
	for (int z = 0; z < 3; z++)
#endif //__MULTIPASS_SMOOTHING__
	{
#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < numVerts; i++)
		{
			//ri->Printf(PRINT_WARNING, "%i of %i.\n", i, numVerts);

			qboolean updated = qfalse;
			vec3_t finalNormal;

			VectorCopy(verts[i].normal, finalNormal);

			for (int j = 0; j < numVerts; j++)
			{
				if (j == i) continue;

				vec3_t n2;

				//if (Distance(verts[i].position, verts[j].position) > 0.0) continue;
				if (!VectorCompare(verts[i].xyz, verts[j].xyz)) continue;

				if (ValidForSmoothing(verts[i].xyz, verts[i].normal, verts[j].xyz, verts[j].normal))
				{
					VectorAdd(finalNormal, n2, finalNormal);
#ifndef __MULTIPASS_SMOOTHING__
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
#endif //__MULTIPASS_SMOOTHING__

					updated = qtrue;
				}
			}

			if (updated)
			{
				VectorNormalize(finalNormal);

#pragma omp critical (__SET_SMOOTH_NORMAL__)
				{
					VectorCopy(finalNormal, verts[i].normal);
				}
			}
		}
	}
}

void GenerateSmoothNormalsForPacked(packedVertex_t *verts, int numVerts)
{
	if (!ENABLE_REGEN_SMOOTH_NORMALS)
	{// Not enabled for this map, skip...
		return;
	}

#define __MULTIPASS_SMOOTHING__ // Looks better, but takes a lot longer...

#ifdef __MULTIPASS_SMOOTHING__
	for (int z = 0; z < 3; z++)
#endif //__MULTIPASS_SMOOTHING__
	{
#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < numVerts; i++)
		{
			//ri->Printf(PRINT_WARNING, "%i of %i.\n", i, numVerts);

			qboolean updated = qfalse;
			vec3_t n1, finalNormal;

			R_VboUnpackNormal(n1, verts[i].normal);

			VectorCopy(n1, finalNormal);

			for (int j = 0; j < numVerts; j++)
			{
				if (j == i) continue;

				vec3_t n2;

				//if (Distance(verts[i].position, verts[j].position) > 0.0) continue;
				if (!VectorCompare(verts[i].position, verts[j].position)) continue;

				R_VboUnpackNormal(n2, verts[j].normal);

				if (ValidForSmoothing(verts[i].position, n1, verts[j].position, n2))
				{
					VectorAdd(finalNormal, n2, finalNormal);
#ifndef __MULTIPASS_SMOOTHING__
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
					VectorAdd(finalNormal, n2, finalNormal);
#endif //__MULTIPASS_SMOOTHING__

					updated = qtrue;
				}
			}

			if (updated)
			{
				VectorNormalize(finalNormal);

#pragma omp critical (__SET_SMOOTH_NORMAL__)
				{
					verts[i].normal = R_VboPackNormal(finalNormal);
				}
			}
		}
	}
}
#endif //__REGENERATE_BSP_NORMALS__

/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf, int *indexes  ) {
	int			i, j;
	srfBspSurface_t	*cv;
	glIndex_t  *tri;
	int			numVerts, numIndexes, badTriangles;
	int realLightmapNum[MAXLIGHTMAPS];

	for ( j = 0; j < MAXLIGHTMAPS; j++ )
	{
		realLightmapNum[j] = FatLightmap (LittleLong (ds->lightmapNum[j]));
	}

	// get fog volume
#ifdef __Q3_FOG__
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
#endif //__Q3_FOG__

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, realLightmapNum, ds->lightmapStyles, ds->vertexStyles);
	if (!surf->shader)
	{
		surf->shader = tr.defaultShader;
	}

	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	if (numVerts > MAX_FACE_POINTS) {
		ri->Printf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numVerts);
		numVerts = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	numIndexes = LittleLong(ds->numIndexes);

	//cv = ri->Hunk_Alloc(sizeof(*cv), h_low);
	cv = (srfBspSurface_t *)surf->data;
	cv->surfaceType = SF_FACE;

#ifdef __FREE_WORLD_DATA__
	cv->numIndexes = numIndexes;
	cv->indexes = (glIndex_t *)malloc(numIndexes * sizeof(cv->indexes[0]));

	cv->numVerts = numVerts;
	cv->verts = (srfVert_t *)malloc(numVerts * sizeof(cv->verts[0]));
#else //!__FREE_WORLD_DATA__
	cv->numIndexes = numIndexes;
	cv->indexes = (glIndex_t *)ri->Hunk_AllocateTempMemory(numIndexes * sizeof(cv->indexes[0]));

	cv->sharedMemoryPointer = NULL;
	cv->numVerts = numVerts;
	cv->verts = (srfVert_t *)ri->Hunk_AllocateTempMemory(numVerts * sizeof(cv->verts[0]));
#endif //__FREE_WORLD_DATA__

	// copy vertexes
	surf->cullinfo.type = CULLINFO_PLANE | CULLINFO_BOX;
	ClearBounds(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
	verts += LittleLong(ds->firstVert);

	for(i = 0; i < numVerts; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		AddPointToBounds(cv->verts[i].xyz, surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);

		for(j = 0; j < 2; j++)
		{
			cv->verts[i].st[j] = LittleFloat(verts[i].st[j]);
		}

		for ( j = 0; j < MAXLIGHTMAPS; j++ )
		{
			cv->verts[i].lightmap[j][0] = FatPackU(LittleFloat(verts[i].lightmap[j][0]), realLightmapNum[j]);
			cv->verts[i].lightmap[j][1] = FatPackV(LittleFloat(verts[i].lightmap[j][1]), realLightmapNum[j]);

			if (hdrVertColors)
			{
				color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
				color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
				color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
			}
			else
			{
				//hack: convert LDR vertex colors to HDR
				if (r_hdr->integer)
				{
					color[0] = MAX(verts[i].color[j][0], 0.499f);
					color[1] = MAX(verts[i].color[j][1], 0.499f);
					color[2] = MAX(verts[i].color[j][2], 0.499f);
				}
				else
				{
					color[0] = verts[i].color[j][0];
					color[1] = verts[i].color[j][1];
					color[2] = verts[i].color[j][2];
				}

			}
			color[3] = verts[i].color[j][3] / 255.0f;

#ifndef __CHEAP_VERTS__
			R_ColorShiftLightingFloats( color, cv->verts[i].vertexColors[j], 1.0f / 255.0f );
#endif //__CHEAP_VERTS__
		}
	}

	// copy triangles
	badTriangles = 0;
	indexes += LittleLong(ds->firstIndex);

	for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
	{
		for(j = 0; j < 3; j++)
		{
			tri[j] = LittleLong(indexes[i + j]);

			if(tri[j] >= numVerts)
			{
				ri->Error(ERR_DROP, "Bad index in face surface");
			}
		}

		if ((tri[0] == tri[1]) || (tri[1] == tri[2]) || (tri[0] == tri[2]))
		{
			tri -= 3;
			badTriangles++;
		}
	}

	if (badTriangles)
	{
		ri->Printf(PRINT_WARNING, "Face has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles);
		cv->numIndexes -= badTriangles * 3;
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->cullPlane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
	cv->cullPlane.dist = DotProduct( cv->verts[0].xyz, cv->cullPlane.normal );
	SetPlaneSignbits( &cv->cullPlane );
	cv->cullPlane.type = PlaneTypeForNormal( cv->cullPlane.normal );
	surf->cullinfo.plane = cv->cullPlane;

#ifdef __REGENERATE_BSP_NORMALS__
	GenerateNormalsForBSPSurface(cv);
#endif //__REGENERATE_BSP_NORMALS__

	//R_OptimizeMesh((uint32_t *)&cv->numVerts, (uint32_t *)&cv->numIndexes, cv->indexes, NULL);

	surf->data = (surfaceType_t *)cv;

	// Calculate tangent spaces
	{
		srfVert_t      *dv[3];

		for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
		{
			dv[0] = &cv->verts[tri[0]];
			dv[1] = &cv->verts[tri[1]];
			dv[2] = &cv->verts[tri[2]];

			R_CalcTangentVectors(dv);
		}
	}
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh ( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf ) {
	srfBspSurface_t	*grid;
	int				i, j;
	int				width, height, numPoints;
	srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE];
	vec3_t			bounds[2];
	vec3_t			tmpVec;
	static surfaceType_t	skipData = SF_SKIP;
	int realLightmapNum[MAXLIGHTMAPS];

	for ( j = 0; j < MAXLIGHTMAPS; j++ )
	{
		realLightmapNum[j] = FatLightmap (LittleLong (ds->lightmapNum[j]));
	}

#ifdef __Q3_FOG__
	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
#endif //__Q3_FOG__

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, realLightmapNum, ds->lightmapStyles, ds->vertexStyles );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData->shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	if(width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE)
		ri->Error(ERR_DROP, "ParseMesh: bad size");

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;

	for(i = 0; i < numPoints; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			points[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			points[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		for(j = 0; j < 2; j++)
		{
			points[i].st[j] = LittleFloat(verts[i].st[j]);
		}

		for ( j = 0; j < MAXLIGHTMAPS; j++ )
		{
			points[i].lightmap[j][0] = FatPackU(LittleFloat(verts[i].lightmap[j][0]), realLightmapNum[j]);
			points[i].lightmap[j][1] = FatPackV(LittleFloat(verts[i].lightmap[j][1]), realLightmapNum[j]);

			if (hdrVertColors)
			{
				color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
				color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
				color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
			}
			else
			{
				//hack: convert LDR vertex colors to HDR
				if (r_hdr->integer)
				{
					color[0] = MAX(verts[i].color[j][0], 0.499f);
					color[1] = MAX(verts[i].color[j][1], 0.499f);
					color[2] = MAX(verts[i].color[j][2], 0.499f);
				}
				else
				{
					color[0] = verts[i].color[j][0];
					color[1] = verts[i].color[j][1];
					color[2] = verts[i].color[j][2];
				}
			}
			color[3] = verts[i].color[j][3] / 255.0f;

#ifndef __CHEAP_VERTS__
			R_ColorShiftLightingFloats( color, points[i].vertexColors[j], 1.0f / 255.0f );
#endif //__CHEAP_VERTS__
		}
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
		bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
	}
	VectorAdd( bounds[0], bounds[1], bounds[1] );
	VectorScale( bounds[1], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );

	//R_OptimizeMesh((uint32_t *)&grid->numVerts, (uint32_t *)&grid->numIndexes, grid->indexes, NULL);

#ifdef __REGENERATE_BSP_NORMALS__
	GenerateNormalsForBSPSurface(grid);
#endif //__REGENERATE_BSP_NORMALS__

	grid->sharedMemoryPointer = NULL;
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, float *hdrVertColors, msurface_t *surf, int *indexes, int surfID ) {
	srfBspSurface_t	*cv;
	glIndex_t		*tri;
	uint32_t		i, j;
	uint32_t		numVerts, numIndexes, badTriangles;

#ifdef __Q3_FOG__
	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
#endif //__Q3_FOG__

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapsVertex, ds->lightmapStyles, ds->vertexStyles );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	numIndexes = LittleLong(ds->numIndexes);

	//cv = (srfBspSurface_t *)ri->Hunk_Alloc(sizeof(*cv), h_low);
	cv = (srfBspSurface_t *)surf->data;
	cv->surfaceType = SF_TRIANGLES;

#ifdef __FREE_WORLD_DATA__
	cv->numIndexes = numIndexes;
	cv->indexes = (glIndex_t *)malloc(numIndexes * sizeof(cv->indexes[0]));

	cv->numVerts = numVerts;
	cv->verts = (srfVert_t *)malloc(numVerts * sizeof(cv->verts[0]));
#else //__FREE_WORLD_DATA__
	cv->numIndexes = numIndexes;
	cv->indexes = (glIndex_t *)ri->Hunk_AllocateTempMemory(numIndexes * sizeof(cv->indexes[0]));

	/*#ifdef __BSP_USE_SHARED_MEMORY__
		cv->numVerts = numVerts;

		ri->Printf(PRINT_WARNING, "Surf %i is %.2f MB.\n", surfID, (numVerts * sizeof(cv->verts[0])) / 1024.0 / 1024.0);

		//if (numVerts * sizeof(cv->verts[0]) >= 5 * 1024 * 1024)
		{
			cv->sharedMemoryPointer = OpenSharedMemory(va("BSPSurface%i", surfID), va("BSPSurface%iMutex", surfID), numVerts * sizeof(cv->verts[0]));

			if (cv->sharedMemoryPointer)
			{
				cv->verts = (srfVert_t *)cv->sharedMemoryPointer->hFileView;
			}
			else
			{
				ri->Printf(PRINT_WARNING, "Surf %i ran out of shared memory, falling back to normal memory for %.2f MB.\n", surfID, (numVerts * sizeof(cv->verts[0])) / 1024.0 / 1024.0);
				cv->sharedMemoryPointer = NULL;
				cv->verts = (srfVert_t *)ri->Hunk_AllocateTempMemory(numVerts * sizeof(cv->verts[0]));
			}
		}
		else
		{
			cv->sharedMemoryPointer = NULL;
			cv->numVerts = numVerts;
			cv->verts = (srfVert_t *)ri->Hunk_AllocateTempMemory(numVerts * sizeof(cv->verts[0]));
		}
	#else //!__BSP_USE_SHARED_MEMORY__*/
		cv->numVerts = numVerts;
		cv->verts = (srfVert_t *)ri->Hunk_AllocateTempMemory(numVerts * sizeof(cv->verts[0]));
	//#endif //__BSP_USE_SHARED_MEMORY__
#endif //__FREE_WORLD_DATA__

	surf->data = (surfaceType_t *) cv;

	// copy vertexes
	surf->cullinfo.type = CULLINFO_BOX;
	ClearBounds(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);
	verts += LittleLong(ds->firstVert);

	for(i = 0; i < numVerts; i++)
	{
		vec4_t color;

		for(j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		/*
		switch ( LittleLong( ds->surfaceType ) ) {
			case MST_FOLIAGE:
				ri->Printf(PRINT_WARNING, "Loaded foliage surface at %f %f %f (%f %f %f) org: %f %f %f - %f %f %f.\n", cv->verts[i].xyz[0], cv->verts[i].xyz[1], cv->verts[i].xyz[2], verts[i].xyz[0], verts[i].xyz[1], verts[i].xyz[2]
				, cv->verts->xyz[0], cv->verts->xyz[1], cv->verts->xyz[2], cv->cullOrigin[0], cv->cullOrigin[1], cv->cullOrigin[2]);
				break;
			default:
				break;
		}
		*/
		AddPointToBounds( cv->verts[i].xyz, surf->cullinfo.bounds[0], surf->cullinfo.bounds[1] );

		for(j = 0; j < 2; j++)
		{
			cv->verts[i].st[j] = LittleFloat(verts[i].st[j]);
		}

		for ( j = 0; j < MAXLIGHTMAPS; j++ )
		{
			cv->verts[i].lightmap[j][0] = LittleFloat(verts[i].lightmap[j][0]);
			cv->verts[i].lightmap[j][1] = LittleFloat(verts[i].lightmap[j][1]);

			if (hdrVertColors)
			{
				color[0] = hdrVertColors[(ds->firstVert + i) * 3    ];
				color[1] = hdrVertColors[(ds->firstVert + i) * 3 + 1];
				color[2] = hdrVertColors[(ds->firstVert + i) * 3 + 2];
			}
			else
			{
				//hack: convert LDR vertex colors to HDR
				if (r_hdr->integer)
				{
					color[0] = MAX(verts[i].color[j][0], 0.499f);
					color[1] = MAX(verts[i].color[j][1], 0.499f);
					color[2] = MAX(verts[i].color[j][2], 0.499f);
				}
				else
				{
					color[0] = verts[i].color[j][0];
					color[1] = verts[i].color[j][1];
					color[2] = verts[i].color[j][2];
				}
			}
			color[3] = verts[i].color[j][3] / 255.0f;

#ifndef __CHEAP_VERTS__
			R_ColorShiftLightingFloats( color, cv->verts[i].vertexColors[j], 1.0f / 255.0f );
#endif //__CHEAP_VERTS__
		}
	}

	// copy triangles
	badTriangles = 0;
	indexes += LittleLong(ds->firstIndex);
	for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
	{
		for(j = 0; j < 3; j++)
		{
			tri[j] = LittleLong(indexes[i + j]);

			if(tri[j] >= numVerts)
			{
				ri->Error(ERR_DROP, "Bad index in face surface");
			}
		}

		/*
		// UQ1: Umm yeah. don't do this. it could be intentional (line).
		if ((tri[0] == tri[1]) || (tri[1] == tri[2]) || (tri[0] == tri[2]))
		{
			tri -= 3;
			badTriangles++;
		}
		*/
	}

	if (badTriangles)
	{
		ri->Printf(PRINT_WARNING, "Trisurf has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles);
		cv->numIndexes -= badTriangles * 3;
		numIndexes = cv->numIndexes; // UQ1: Need to also do this, since we use it just below.
	}

	//R_OptimizeMesh((uint32_t *)&cv->numVerts, (uint32_t *)&cv->numIndexes, cv->indexes, NULL);

#ifdef __REGENERATE_BSP_NORMALS__
	GenerateNormalsForBSPSurface(cv);
#endif //__REGENERATE_BSP_NORMALS__

	// Calculate tangent spaces
	{
		srfVert_t      *dv[3];

		for(i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
		{
			dv[0] = &cv->verts[tri[0]];
			dv[1] = &cv->verts[tri[1]];
			dv[2] = &cv->verts[tri[2]];
			
			//ri->Printf(PRINT_WARNING, "Loaded foliage surface at %f %f %f, %f %f %f, %f %f %f.\n", dv[0]->xyz[0], dv[0]->xyz[1], dv[0]->xyz[2], dv[1]->xyz[0], dv[1]->xyz[1], dv[1]->xyz[2], dv[2]->xyz[0], dv[2]->xyz[1], dv[2]->xyz[2]);

			R_CalcTangentVectors(dv);
		}
	}
}

#ifdef __VBO_LODMODELS__
static void ParseLodModelTriSurf(lodModel_t *lodModel, mdvModel_t *header, int surfid, msurface_t *surf) {
	srfBspSurface_t		*cv;
	glIndex_t			*tri;
	int					i, j;
	int					numVerts, numIndexes, badTriangles;

	mdvSurface_t *ds = &header->surfaces[surfid];
	
	int foundShader = -1;

	shader_t *modelShader = tr.shaders[ds->shaderIndexes[0]];

	for (i = 0; i < s_worldData->numShaders; i++)
	{
		if (!strcmp(s_worldData->shaders[i].shader, modelShader->name))
		{
			foundShader = i;
			break;
		}
	}

	if (foundShader < 0)
	{
		//s_worldData->shaders = (dshader_t *)realloc(s_worldData->shaders, sizeof(dshader_t)*(s_worldData->numShaders+1)); // hmm damn hunk_alloc crap...
		dshader_t *oldShaders = s_worldData->shaders;
		s_worldData->shaders = NULL;
		s_worldData->shaders = (dshader_t *)malloc(sizeof(dshader_t)*(s_worldData->numShaders + 1));
		memcpy(s_worldData->shaders, oldShaders, sizeof(dshader_t) * s_worldData->numShaders);
		strcpy(s_worldData->shaders[s_worldData->numShaders].shader, modelShader->name);
		s_worldData->shaders[s_worldData->numShaders].contentFlags = modelShader->contentFlags;
		s_worldData->shaders[s_worldData->numShaders].surfaceFlags = modelShader->surfaceFlags;
		foundShader = s_worldData->numShaders;
		s_worldData->numShaders++;
	}

	// get shader
	//surf->shader = ShaderForShaderNum(foundShader, lightmapsVertex, (const byte*)lightmapsNone, stylesDefault);
	surf->shader = tr.shaders[ds->shaderIndexes[0]];

	if (!surf->shader)
	{
		ri->Printf(PRINT_WARNING, "Lodmodel %s had bad shader on surface %i.\n", lodModel->modelName, surfid);
		//surf->shader->name
		surf->shader = tr.defaultShader;
	}

	if (r_singleShader->integer && !surf->shader->isSky) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	numIndexes = LittleLong(ds->numIndexes);

	cv = (srfBspSurface_t *)surf->data;
	cv->surfaceType = SF_TRIANGLES;

	cv->numIndexes = numIndexes;
	cv->indexes = (glIndex_t *)ri->Hunk_AllocateTempMemory(numIndexes * sizeof(cv->indexes[0]));

	cv->numVerts = numVerts;
	cv->verts = (srfVert_t *)ri->Hunk_AllocateTempMemory(numVerts * sizeof(cv->verts[0]));

	surf->data = (surfaceType_t *)cv;

	// copy vertexes
	surf->cullinfo.type = CULLINFO_BOX;
	ClearBounds(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);

	for (i = 0; i < numVerts; i++)
	{
		vec4_t color;

		for (j = 0; j < 3; j++)
		{
			float xyz = (lodModel->modelScale[j] * ds->verts[i].xyz[j]) + lodModel->origin[j];

			cv->verts[i].xyz[j] = LittleFloat(xyz);
			cv->verts[i].normal[j] = LittleFloat(ds->verts[i].normal[j]);
		}

		AddPointToBounds(cv->verts[i].xyz, surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]);

		for (j = 0; j < 2; j++)
		{
			cv->verts[i].st[j] = LittleFloat(ds->st[i].st[j]);
		}

		for (j = 0; j < MAXLIGHTMAPS; j++)
		{
			cv->verts[i].lightmap[j][0] = 0;
			cv->verts[i].lightmap[j][1] = 0;

			color[0] = 1.0;
			color[1] = 1.0;
			color[2] = 1.0;
			color[3] = 1.0;

			R_ColorShiftLightingFloats(color, cv->verts[i].vertexColors[j], 1.0f / 255.0f);
		}
	}

	// copy triangles
	badTriangles = 0;
	for (i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
	{
		for (j = 0; j < 3; j++)
		{
			tri[j] = LittleLong(ds->indexes[i + j]);

			if (tri[j] >= numVerts)
			{
				ri->Error(ERR_DROP, "Bad index in face surface");
			}
		}

		if ((tri[0] == tri[1]) || (tri[1] == tri[2]) || (tri[0] == tri[2]))
		{
			tri -= 3;
			badTriangles++;
		}
	}

	if (badTriangles)
	{
		ri->Printf(PRINT_WARNING, "Trisurf has bad triangles, originally shader %s %d tris %d verts, now %d tris\n", surf->shader->name, numIndexes / 3, numVerts, numIndexes / 3 - badTriangles);
		cv->numIndexes -= badTriangles * 3;
	}

	/*// Calculate tangent spaces
	{
		srfVert_t      *dv[3];

		for (i = 0, tri = cv->indexes; i < numIndexes; i += 3, tri += 3)
		{
			dv[0] = &cv->verts[tri[0]];
			dv[1] = &cv->verts[tri[1]];
			dv[2] = &cv->verts[tri[2]];

			R_CalcTangentVectors(dv);
		}
	}*/
}
#endif //__VBO_LODMODELS__

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfFlare_t		*flare;
	int				i;

#ifdef __Q3_FOG__
	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;
#endif //__Q3_FOG__

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapsVertex, ds->lightmapStyles, ds->vertexStyles );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	//flare = ri->Hunk_Alloc( sizeof( *flare ), h_low );
	flare = (srfFlare_t *)surf->data;
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
		flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
		flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints(srfBspSurface_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->width-1; i++) {
		for (j = i + 1; j < grid->width-1; j++) {
			if ( fabs(grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints(srfBspSurface_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->height-1; i++) {
		for (j = i + 1; j < grid->height-1; j++) {
			if ( fabs(grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfBspSurface_t *grid1 ) {
	int j, k, l, m, n, offset1, offset2, touch;
	srfBspSurface_t *grid2;

	for ( j = start; j < s_worldData->numsurfaces; j++ ) {
		//
		grid2 = (srfBspSurface_t *) s_worldData->surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		touch = qfalse;
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = (grid1->height-1) * grid1->width;
			else offset1 = 0;
			if (R_MergedWidthPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->width-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = grid1->width-1;
			else offset1 = 0;
			if (R_MergedHeightPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->height-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		if (touch) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r ( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void ) {
	int i;
	srfBspSurface_t *grid1;


	for ( i = 0; i < s_worldData->numsurfaces; i++ ) {
		//
		grid1 = (srfBspSurface_t *) s_worldData->surfaces[i].data;
		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID )
			continue;
		//
		if ( grid1->lodFixed )
			continue;
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1);
	}
}


/*
===============
R_StitchPatches
===============
*/
int R_StitchPatches( int grid1num, int grid2num ) {
	float *v1, *v2;
	srfBspSurface_t *grid1, *grid2;
	int k, l, m, n, offset1, offset2, row, column;

	grid1 = (srfBspSurface_t *) s_worldData->surfaces[grid1num].data;
	grid2 = (srfBspSurface_t *) s_worldData->surfaces[grid2num].data;
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->width-2; k += 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->height-2; k += 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = grid1->width-1; k > 1; k -= 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					if (!grid2)
						break;
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = grid1->height-1; k > 1; k -= 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri->Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData->surfaces[grid2num].data = (surfaceType_t *) grid2;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num ) {
	int j, numstitches;
	srfBspSurface_t *grid1, *grid2;

	numstitches = 0;
	grid1 = (srfBspSurface_t *) s_worldData->surfaces[grid1num].data;

	for ( j = 0; j < s_worldData->numsurfaces; j++ ) {
		//
		grid2 = (srfBspSurface_t *) s_worldData->surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		while (R_StitchPatches(grid1num, j))
		{
			numstitches++;
		}
	}
	return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches( void ) {
	int i, stitched, numstitches;

	numstitches = 0;
	do
	{
		stitched = qfalse;

//#pragma omp parallel for schedule(dynamic)
		for ( i = 0; i < s_worldData->numsurfaces; i++ ) {
			//
			srfBspSurface_t *grid1 = (srfBspSurface_t *) s_worldData->surfaces[i].data;
			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID )
				continue;
			//
			if ( grid1->lodStitched )
				continue;
			//
			grid1->lodStitched = qtrue;
			stitched = qtrue;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while (stitched);
	ri->Printf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk(void) {
	int i;

	for ( i = 0; i < s_worldData->numsurfaces; i++ ) {
		int size;

		//
		srfBspSurface_t *grid = (srfBspSurface_t *) s_worldData->surfaces[i].data;
		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID )
			continue;
		//
		size = sizeof(*grid);

#ifdef __FREE_WORLD_DATA__
		srfBspSurface_t *hunkgrid = (srfBspSurface_t *)malloc(size);
		Com_Memcpy(hunkgrid, grid, size);

		hunkgrid->widthLodError = (float *)malloc(grid->width * 4);
		Com_Memcpy(hunkgrid->widthLodError, grid->widthLodError, grid->width * 4);

		hunkgrid->heightLodError = (float *)malloc(grid->height * 4);
		Com_Memcpy(hunkgrid->heightLodError, grid->heightLodError, grid->height * 4);

		hunkgrid->numIndexes = grid->numIndexes;
		hunkgrid->indexes = (glIndex_t *)malloc(grid->numIndexes * sizeof(glIndex_t));
		Com_Memcpy(hunkgrid->indexes, grid->indexes, grid->numIndexes * sizeof(glIndex_t));

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts = (srfVert_t *)malloc(grid->numVerts * sizeof(srfVert_t));
		Com_Memcpy(hunkgrid->verts, grid->verts, grid->numVerts * sizeof(srfVert_t));
#else //!__FREE_WORLD_DATA__
		srfBspSurface_t *hunkgrid = (srfBspSurface_t *)ri->Hunk_Alloc(size, h_low);
		Com_Memcpy(hunkgrid, grid, size);

		hunkgrid->widthLodError = (float *)ri->Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = (float *)ri->Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( hunkgrid->heightLodError, grid->heightLodError, grid->height * 4 );

		hunkgrid->numIndexes = grid->numIndexes;
		hunkgrid->indexes = (glIndex_t *)ri->Hunk_Alloc(grid->numIndexes * sizeof(glIndex_t), h_low);
		Com_Memcpy(hunkgrid->indexes, grid->indexes, grid->numIndexes * sizeof(glIndex_t));

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts = (srfVert_t *)ri->Hunk_Alloc(grid->numVerts * sizeof(srfVert_t), h_low);
		Com_Memcpy(hunkgrid->verts, grid->verts, grid->numVerts * sizeof(srfVert_t));

		hunkgrid->sharedMemoryPointer = NULL;
#endif //__FREE_WORLD_DATA__

		R_FreeSurfaceGridMesh( grid );

		s_worldData->surfaces[i].data = (surfaceType_t *) hunkgrid;
	}
}


/*
=================
BSPSurfaceCompare
compare function for qsort()
=================
*/
static int BSPSurfaceCompare(const void *a, const void *b)
{
	//
	// Sort by all the things... Minimize shader and state changes...
	//
	msurface_t   *aa, *bb;

	aa = *(msurface_t **) a;
	bb = *(msurface_t **) b;

	//
	// First fill in the shader's hasAlphaTestBits and hasSplatMaps values...
	//

	// Set up a value to skip stage checks later... does this even run per frame? i shoud actually check probably...
	if (aa->shader->hasAlphaTestBits == 0)
	{
		for (int stage = 0; stage <= aa->shader->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = aa->shader->stages[stage];

			if (!pStage)
			{// How does this happen???
				continue;
			}

			if (!pStage->active)
			{// Shouldn't this be here, just in case???
				continue;
			}

			if (pStage->stateBits & GLS_ATEST_BITS)
			{
				aa->shader->hasAlphaTestBits = max(aa->shader->hasAlphaTestBits, 1);
			}

			if (pStage->alphaGen)
			{
				aa->shader->hasAlphaTestBits = max(aa->shader->hasAlphaTestBits, 2);
			}

			if (aa->shader->hasAlpha)
			{
				aa->shader->hasAlphaTestBits = max(aa->shader->hasAlphaTestBits, 3);
			}
		}

		if (aa->shader->hasAlphaTestBits == 0)
		{
			aa->shader->hasAlphaTestBits = -1;
		}
	}

	if (bb->shader->hasAlphaTestBits == 0)
	{
		for (int stage = 0; stage <= bb->shader->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = bb->shader->stages[stage];

			if (!pStage)
			{// How does this happen???
				continue;
			}

			if (!pStage->active)
			{// Shouldn't this be here, just in case???
				continue;
			}

			if (pStage->stateBits & GLS_ATEST_BITS)
			{
				bb->shader->hasAlphaTestBits = max(bb->shader->hasAlphaTestBits, 1);
				break;
			}

			if (pStage->alphaGen)
			{
				bb->shader->hasAlphaTestBits = max(bb->shader->hasAlphaTestBits, 2);
			}

			if (bb->shader->hasAlpha)
			{
				bb->shader->hasAlphaTestBits = max(bb->shader->hasAlphaTestBits, 3);
			}
		}

		if (bb->shader->hasAlphaTestBits == 0)
		{
			bb->shader->hasAlphaTestBits = -1;
		}
	}

	if (aa->shader->hasSplatMaps == 0)
	{
		for (int stage = 0; stage <= aa->shader->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = aa->shader->stages[stage];

			if (!pStage)
			{// How does this happen???
				continue;
			}

			if (!pStage->active)
			{// Shouldn't this be here, just in case???
				continue;
			}

			if (pStage->bundle[TB_STEEPMAP].image[0]
				|| pStage->bundle[TB_WATER_EDGE_MAP].image[0]
				|| pStage->bundle[TB_SPLATMAP1].image[0]
				|| pStage->bundle[TB_SPLATMAP2].image[0]
				|| pStage->bundle[TB_SPLATMAP3].image[0]
				|| pStage->bundle[TB_ROOFMAP].image[0])
			{
				aa->shader->hasSplatMaps = 1;
				break;
			}
		}

		if (aa->shader->hasSplatMaps == 0)
		{
			aa->shader->hasSplatMaps = -1;
		}
	}

	if (bb->shader->hasSplatMaps == 0)
	{
		for (int stage = 0; stage <= bb->shader->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = bb->shader->stages[stage];

			if (!pStage)
			{// How does this happen???
				continue;
			}

			if (!pStage->active)
			{// Shouldn't this be here, just in case???
				continue;
			}

			if (pStage->bundle[TB_STEEPMAP].image[0]
				|| pStage->bundle[TB_WATER_EDGE_MAP].image[0]
				|| pStage->bundle[TB_SPLATMAP1].image[0]
				|| pStage->bundle[TB_SPLATMAP2].image[0]
				|| pStage->bundle[TB_SPLATMAP3].image[0]
				|| pStage->bundle[TB_ROOFMAP].image[0])
			{
				bb->shader->hasSplatMaps = 1;
				break;
			}
		}

		if (bb->shader->hasSplatMaps == 0)
		{
			bb->shader->hasSplatMaps = -1;
		}
	}

	//
	// The actual sorting...
	//

#ifdef __SPLATMAP_SORTING__
	// Splat maps are always solid with no alpha, and nearly always terrain or large objects, so do them first, they should block a lot of pixels...
	// Non-alpha stage shaders should draw first... Hopefully allow for faster pixel depth culling...
	if (aa->shader->hasSplatMaps > bb->shader->hasSplatMaps)
		return -1;
	else if (aa->shader->hasSplatMaps < bb->shader->hasSplatMaps)
		return 1;
#endif //__SPLATMAP_SORTING__

#ifdef __GRASS_SORTING__
	// Sort by has geometry grass addition required...
	if ((aa->shader->materialType == MATERIAL_SHORTGRASS || aa->shader->materialType == MATERIAL_LONGGRASS) > (bb->shader->materialType == MATERIAL_SHORTGRASS || bb->shader->materialType == MATERIAL_LONGGRASS))
		return -1;
	else if ((aa->shader->materialType == MATERIAL_SHORTGRASS || aa->shader->materialType == MATERIAL_LONGGRASS) < (bb->shader->materialType == MATERIAL_SHORTGRASS || bb->shader->materialType == MATERIAL_LONGGRASS))
		return 1;
#endif //__GRASS_SORTING__


#ifdef __TESS_SORTING__
#ifdef __TERRAIN_TESSELATION__
	extern qboolean TERRAIN_TESSELLATION_ENABLED;
	extern qboolean GRASS_ENABLED;
	extern qboolean RB_ShouldUseGeometryGrass(int materialType);

	qboolean isTerrainTessEnabled = (TERRAIN_TESSELLATION_ENABLED && r_terrainTessellation->integer && r_terrainTessellationMax->value >= 2.0 && (r_foliage->integer && GRASS_ENABLED)) ? qtrue : qfalse;
	qboolean aaIsTessTerrain = (aa->shader->isGrass || RB_ShouldUseGeometryGrass(aa->shader->materialType)) ? qtrue : qfalse;
	qboolean bbIsTessTerrain = (aa->shader->isGrass || RB_ShouldUseGeometryGrass(aa->shader->materialType)) ? qtrue : qfalse;

	if (isTerrainTessEnabled && (aaIsTessTerrain || bbIsTessTerrain))
	{// Always add tesselation to ground surfaces...
		if (aaIsTessTerrain > bbIsTessTerrain)
			return -1;

		else if (aaIsTessTerrain < bbIsTessTerrain)
			return 1;
	}
	else
#endif //__TERRAIN_TESSELATION__
		if (aa->shader->tesselation)
		{// Check tesselation settings... Draw faster tesselation surfs first...
			if (aa->shader->tesselationLevel < bb->shader->tesselationLevel)
				return -1;

			else if (aa->shader->tesselationLevel > bb->shader->tesselationLevel)
				return 1;

			if (aa->shader->tesselationAlpha < bb->shader->tesselationAlpha)
				return -1;

			else if (aa->shader->tesselationAlpha > bb->shader->tesselationAlpha)
				return 1;
		}
#endif //__TESS_SORTING__


#ifdef __WATER_SORTING__
	// Set up a value to skip stage checks later... does this even run per frame? i shoud actually check probably...
	if (aa->shader->isWater < bb->shader->isWater)
		return -1;
	else if (aa->shader->isWater > bb->shader->isWater)
		return 1;
#endif //__WATER_SORTING__


#ifdef __USE_VBO_AREAS__
	if (aa->vboArea < bb->vboArea)
		return -1;
	else if (aa->vboArea > bb->vboArea)
		return 1;
#endif //__USE_VBO_AREAS__


#ifdef __FX_SORTING__
	if (qboolean(aa->shader == tr.sunShader) < qboolean(bb->shader == tr.sunShader))
		return -1;
	else if (qboolean(aa->shader == tr.sunShader) > qboolean(bb->shader == tr.sunShader))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_MENU_BACKGROUND) < qboolean(bb->shader->materialType == MATERIAL_MENU_BACKGROUND))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_MENU_BACKGROUND) > qboolean(bb->shader->materialType == MATERIAL_MENU_BACKGROUND))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_FIRE) < qboolean(bb->shader->materialType == MATERIAL_FIRE))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_FIRE) > qboolean(bb->shader->materialType == MATERIAL_FIRE))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_SMOKE) < qboolean(bb->shader->materialType == MATERIAL_SMOKE))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_SMOKE) > qboolean(bb->shader->materialType == MATERIAL_SMOKE))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_LAVA) < qboolean(bb->shader->materialType == MATERIAL_LAVA))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_LAVA) > qboolean(bb->shader->materialType == MATERIAL_LAVA))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_MAGIC_PARTICLES) < qboolean(bb->shader->materialType == MATERIAL_MAGIC_PARTICLES))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_MAGIC_PARTICLES) > qboolean(bb->shader->materialType == MATERIAL_MAGIC_PARTICLES))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_MAGIC_PARTICLES_TREE) < qboolean(bb->shader->materialType == MATERIAL_MAGIC_PARTICLES_TREE))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_MAGIC_PARTICLES_TREE) > qboolean(bb->shader->materialType == MATERIAL_MAGIC_PARTICLES_TREE))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_FIREFLIES) < qboolean(bb->shader->materialType == MATERIAL_FIREFLIES))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_FIREFLIES) > qboolean(bb->shader->materialType == MATERIAL_FIREFLIES))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_PORTAL) < qboolean(bb->shader->materialType == MATERIAL_PORTAL))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_PORTAL) > qboolean(bb->shader->materialType == MATERIAL_PORTAL))
		return 1;

	if (qboolean(aa->shader->materialType == MATERIAL_LAVA) < qboolean(bb->shader->materialType == MATERIAL_LAVA))
		return -1;
	else if (qboolean(aa->shader->materialType == MATERIAL_LAVA) > qboolean(bb->shader->materialType == MATERIAL_LAVA))
		return 1;
#endif //__FX_SORTING__


#ifdef __GLOW_SORTING__
	// Non-glow stage shaders should draw first... Hopefully allow for faster pixel depth culling...
	if (aa->shader->hasGlow < bb->shader->hasGlow)
		return -1;
	else if (aa->shader->hasGlow > bb->shader->hasGlow)
		return 1;
#endif //__GLOW_SORTING__


#ifdef __ALPHA_SORTING__
	// Non-alpha stage shaders should draw first... Hopefully allow for faster pixel depth culling...
	if (aa->shader->hasAlphaTestBits < bb->shader->hasAlphaTestBits)
		return -1;
	else if (aa->shader->hasAlphaTestBits > bb->shader->hasAlphaTestBits)
		return 1;
#endif //__ALPHA_SORTING__


#ifdef __NUMSTAGES_SORTING__
	// Fewer stage shaders should draw first... Hopefully allow for faster pixel depth culling...
	if (aa->shader->numStages < bb->shader->numStages)
		return -1;
	else if (aa->shader->numStages > bb->shader->numStages)
		return 1;
#endif //__NUMSTAGES_SORTING__

	
#ifdef __MERGED_SORTING__
	if (aa->isMerged > bb->isMerged)
		return -1;
	else if (aa->isMerged < bb->isMerged)
		return 1;
#endif //__MERGED_SORTING__

#ifdef __DEPTHDRAW_SORTING__
	if (aa->depthDrawOnlyFoliage < bb->depthDrawOnlyFoliage)
		return -1;
	else if (aa->depthDrawOnlyFoliage > bb->depthDrawOnlyFoliage)
		return 1;

	if (aa->depthDrawOnly < bb->depthDrawOnly)
		return -1;
	else if (aa->depthDrawOnly > bb->depthDrawOnly)
		return 1;
#endif //__DEPTHDRAW_SORTING__


	// sort by actual shader
	if (aa->shader < bb->shader)
		return -1;
	else if (aa->shader > bb->shader)
		return 1;

	// sort by shader sortIndex
	if (aa->shader->sortedIndex < bb->shader->sortedIndex)
		return -1;
	else if (aa->shader->sortedIndex > bb->shader->sortedIndex)
		return 1;


#ifdef __MATERIAL_SORTING__
	{// Material types.. To minimize changing between geom (grass) versions and non-grass shaders...
		if (aa->shader->materialType < bb->shader->materialType)
			return -1;
		else if (aa->shader->materialType > bb->shader->materialType)
			return 1;
	}
#endif //__MATERIAL_SORTING__
	
#ifdef __INDOOR_OUTDOOR_CULLING__
#ifdef __INDOOR_SORTING__
	if (backEnd.viewIsOutdoors)
	{
		if (aa->shader->isIndoor < bb->shader->isIndoor)
			return -1;
		else if (aa->shader->isIndoor > bb->shader->isIndoor)
			return 1;
	}
	else
	{
		if (aa->shader->isIndoor > bb->shader->isIndoor)
			return -1;
		else if (aa->shader->isIndoor < bb->shader->isIndoor)
			return 1;
	}
#endif //__INDOOR_SORTING__
#endif //__INDOOR_OUTDOOR_CULLING__

#ifdef __Q3_FOG__
	// by fogIndex
	if(aa->fogIndex < bb->fogIndex)
		return -1;
	else if(aa->fogIndex > bb->fogIndex)
		return 1;
#endif //__Q3_FOG__


#ifndef __PLAYER_BASED_CUBEMAPS__
	// by cubemapIndex
	if(aa->cubemapIndex < bb->cubemapIndex)
		return -1;
	else if(aa->cubemapIndex > bb->cubemapIndex)
		return 1;
#endif //__PLAYER_BASED_CUBEMAPS__

	return 0;
}


static void CopyVert(const srfVert_t * in, srfVert_t * out)
{
	int             j;

	for(j = 0; j < 3; j++)
	{
		out->xyz[j]       = in->xyz[j];
#ifndef __CHEAP_VERTS__
		out->tangent[j]   = in->tangent[j];
		//out->bitangent[j] = in->bitangent[j];
#endif //__CHEAP_VERTS__
		out->normal[j]    = in->normal[j];
#ifndef __CHEAP_VERTS__
		out->lightdir[j]  = in->lightdir[j];
#endif //__CHEAP_VERTS__
	}

#ifndef __CHEAP_VERTS__
	out->tangent[3] = in->tangent[3];
#endif //__CHEAP_VERTS__

	for(j = 0; j < 2; j++)
	{
		out->st[j] = in->st[j];
		Com_Memcpy (out->lightmap[j], in->lightmap[j], sizeof (out->lightmap[0]));
	}

#ifndef __CHEAP_VERTS__
	for(j = 0; j < 4; j++)
	{
		Com_Memcpy (out->vertexColors[j], in->vertexColors[j], sizeof (out->vertexColors[0]));
	}
#endif //__CHEAP_VERTS__
}

#ifdef __USE_VBO_AREAS__
#define NUM_MAP_AREAS 4//16//256//64//16//9
#define NUM_MAP_SECTIONS sqrt(NUM_MAP_AREAS)

struct mapArea_t
{
	vec3_t		mins;
	vec3_t		maxs;
	vec3_t		center;
	qboolean	visible = qfalse;
};

struct mapAreas_t
{
	int numAreas = 0;
	mapArea_t areas[(NUM_MAP_AREAS*2)];
};

mapAreas_t MAP_AREAS;

void Setup_VBO_Areas(void)
{
	int		areaNum = 0;
	vec3_t	mapMins, mapMaxs;
	
	VectorCopy(tr.world->nodes[0].mins, mapMins);
	VectorCopy(tr.world->nodes[0].maxs, mapMaxs);

	vec3_t mapSize, modifier;
	VectorSubtract(tr.world->nodes[0].maxs, tr.world->nodes[0].mins, mapSize);
	modifier[0] = mapSize[0] / NUM_MAP_SECTIONS;
	modifier[1] = mapSize[1] / NUM_MAP_SECTIONS;
	modifier[2] = mapSize[2] / 2;

	for (int x = 0; x < NUM_MAP_SECTIONS; x++)
	{
		for (int y = 0; y < NUM_MAP_SECTIONS; y++)
		{
			for (int z = 0; z < 2; z++)
			{
				vec3_t mins, maxs;
				VectorCopy(tr.world->nodes[0].mins, mins);
				mins[0] += (modifier[0] * x);
				mins[1] += (modifier[1] * y);
				mins[2] += (modifier[2] * z);

				// Extend the edges outside the map a little, allow for precision errors, etc...
				if (x == 0)
				{
					mins[0] -= 512.0;
				}
				else if (y == 0)
				{
					mins[1] -= 512.0;
				}
				else if (z == 0)
				{
					mins[2] -= 512.0;
				}

				VectorCopy(tr.world->nodes[0].mins, maxs);
				maxs[0] += (modifier[0] * (x + 1));
				maxs[1] += (modifier[1] * (y + 1));
				maxs[2] += (modifier[2] * (z + 1));

				// Extend the edges outside the map a little, allow for precision errors, etc...
				if (x == NUM_MAP_SECTIONS - 1)
				{
					maxs[0] += 512.0;
				}
				else if (y == NUM_MAP_SECTIONS - 1)
				{
					maxs[1] += 512.0;
				}
				else if (z == 1)
				{
					maxs[2] += 512.0;
				}

				VectorCopy(mins, MAP_AREAS.areas[MAP_AREAS.numAreas].mins);
				VectorCopy(maxs, MAP_AREAS.areas[MAP_AREAS.numAreas].maxs);
				MAP_AREAS.areas[MAP_AREAS.numAreas].center[0] = (mins[0] + maxs[0]) * 0.5;
				MAP_AREAS.areas[MAP_AREAS.numAreas].center[1] = (mins[1] + maxs[1]) * 0.5;
				MAP_AREAS.areas[MAP_AREAS.numAreas].center[2] = (mins[2] + maxs[2]) * 0.5;
				MAP_AREAS.numAreas++;
			}
		}
	}

	/*for (int i = 0; i < MAP_AREAS.numAreas; i++)
	{
		ri->Printf(PRINT_WARNING, "Area %i. mins %i %i %i. maxs %i %i %i.\n"
			, i
			, (int)MAP_AREAS.areas[i].mins[0], (int)MAP_AREAS.areas[i].mins[1], (int)MAP_AREAS.areas[i].mins[2]
			, (int)MAP_AREAS.areas[i].maxs[0], (int)MAP_AREAS.areas[i].maxs[1], (int)MAP_AREAS.areas[i].maxs[2]);
	}*/
}

qboolean R_PointInXYBounds(vec3_t point, vec3_t mins, vec3_t maxs)
{
	int i;

	for (i = 0; i < 2/*3*/; i++)
	{
		if (point[i] < mins[i])
		{
			return qfalse;
		}
		if (point[i] > maxs[i])
		{
			return qfalse;
		}
	}

	return qtrue;
}

int GetVBOArea(vec3_t origin)
{
	for (int i = 0; i < MAP_AREAS.numAreas; i++)
	{
		if (R_PointInXYBounds(origin, MAP_AREAS.areas[i].mins, MAP_AREAS.areas[i].maxs))
		{
			return i;
		}
	}

	return MAP_AREAS.numAreas / 2; // should never happen, just give it the center of map, should it happen.
}

extern int R_BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p);

qboolean R_AreaInFOV(vec3_t spot, vec3_t from)
{
	vec3_t	deltaVector, angles, deltaAngles;
	vec3_t	fromAnglesCopy;
	vec3_t	fromAngles;
	int hFOV = r_testvalue0->integer;// 180;
	int vFOV = r_testvalue0->integer;//180;

	extern void TR_AxisToAngles(const vec3_t axis[3], vec3_t angles);
	TR_AxisToAngles(tr.refdef.viewaxis, fromAngles);

	VectorSubtract(spot, from, deltaVector);
	vectoangles(deltaVector, angles);
	VectorCopy(fromAngles, fromAnglesCopy);

	deltaAngles[PITCH] = AngleDelta(fromAnglesCopy[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAnglesCopy[YAW], angles[YAW]);

	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	return qfalse;
}


qboolean VBOAreaVisible(int areanum)
{
	if (areanum == -1) return qtrue;

	mapArea_t *area = &MAP_AREAS.areas[areanum];

	int r;
	int planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;

	if (planeBits & 1) {
		r = R_BoxOnPlaneSide(area->mins, area->maxs, &tr.viewParms.frustum[0]);
		if (r == 2) {
			return qfalse;				// culled
		}
		if (r == 1) {
			planeBits &= ~1;			// all descendants will also be in front
		}
	}

#if 0
	if (planeBits & 2) {
		r = R_BoxOnPlaneSide(area->mins, area->maxs, &tr.viewParms.frustum[1]);
		if (r == 2) {
			return qfalse;				// culled
		}
		if (r == 1) {
			planeBits &= ~2;			// all descendants will also be in front
		}
	}

	if (planeBits & 4) {
		r = R_BoxOnPlaneSide(area->mins, area->maxs, &tr.viewParms.frustum[2]);
		if (r == 2) {
			return qfalse;				// culled
		}
		if (r == 1) {
			planeBits &= ~4;			// all descendants will also be in front
		}
	}

	if (planeBits & 8) {
		r = R_BoxOnPlaneSide(area->mins, area->maxs, &tr.viewParms.frustum[3]);
		if (r == 2) {
			return qfalse;				// culled
		}
		if (r == 1) {
			planeBits &= ~8;			// all descendants will also be in front
		}
	}

	if (planeBits & 16) {
		r = R_BoxOnPlaneSide(area->mins, area->maxs, &tr.viewParms.frustum[4]);
		if (r == 2) {
			return qfalse;				// culled
		}
		if (r == 1) {
			planeBits &= ~16;			// all descendants will also be in front
		}
	}
#endif

	if (Distance(area->center, tr.refdef.vieworg) > tr.occlusionZfar * 2.0)
	{// Too far away...
		return qfalse;
	}

	return qtrue;
}

qboolean R_PointInBounds(vec3_t point, vec3_t mins, vec3_t maxs)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (point[i] < mins[i])
		{
			return qfalse;
		}
		if (point[i] > maxs[i])
		{
			return qfalse;
		}
	}

	return qtrue;
}

void SetVBOVisibleAreas(void)
{
	int numVisible = 0;
	int numInVisible = 0;

	for (int i = 0; i < MAP_AREAS.numAreas; i++)
	{
		if (r_occlusion->integer)
		{
			if (R_PointInBounds(tr.refdef.vieworg, MAP_AREAS.areas[i].mins, MAP_AREAS.areas[i].maxs))
			{// We are inside this area, always visible...
				MAP_AREAS.areas[i].visible = qtrue;
				numVisible++;
			}
			else if (!VBOAreaVisible(i))
			{// Not in view frustrum...
				MAP_AREAS.areas[i].visible = qfalse;
				numInVisible++;
			}
			else
			{// Visible...
				MAP_AREAS.areas[i].visible = qtrue;
				numVisible++;
			}
		}
		else
		{
			MAP_AREAS.areas[i].visible = qtrue;
			numVisible++;
		}
	}

	if (r_areaVisDebug->integer)
	{
		ri->Printf(PRINT_ALL, "v: %i. i: %i.\n", numVisible, numInVisible);
	}
}

qboolean GetVBOAreaVisible(int area)
{
	if (area >= 0)
	{
		return MAP_AREAS.areas[area].visible;
	}
	else
	{// Sky etc...
		return qtrue;
	}
}
#endif //__USE_VBO_AREAS__

/*
===============
R_CreateWorldVBOs
===============
*/
static void R_CreateWorldVBOs(void)
{
	int             i, j, k;

	int             numVerts;
	packedVertex_t  *verts;

	int             numIndexes;
	glIndex_t      *indexes;

    int             numSortedSurfaces, numSurfaces;
	msurface_t   *surface, **firstSurf, **lastSurf, **currSurf;
	msurface_t  **surfacesSorted;

	VBO_t *vbo;
	IBO_t *ibo;

	//int maxVboSize = 16 * 64 * 1024 * 1024;
	//int maxIboSize = 4 * 64 * 1024 * 1024;

	int maxVboSize = 16 * 1024;
	int maxIboSize = 4 * 1024;

	int             startTime, endTime;

	startTime = ri->Milliseconds();

	// count surfaces
	numSortedSurfaces = 0;

#ifdef __USE_VBO_AREAS__
	Setup_VBO_Areas();
#endif //__USE_VBO_AREAS__
	
	int s;

	for(s = 0, surface = &s_worldData->surfaces[0]; surface < &s_worldData->surfaces[s_worldData->numsurfaces]; surface++, s++)
	{
		srfBspSurface_t *bspSurf = (srfBspSurface_t *)surface->data;
		shader_t *shader = surface->shader;

#ifdef __USE_VBO_AREAS__
		bspSurf->vboArea = -1;
		surface->vboArea = -1;

		vec3_t center;
		VectorCopy(surface->cullinfo.bounds[0], bspSurf->cullBounds[0]);
		VectorCopy(surface->cullinfo.bounds[1], bspSurf->cullBounds[1]);
		VectorAdd(bspSurf->cullBounds[0], bspSurf->cullBounds[1], center);
		VectorScale(center, 0.5f, center);
		VectorCopy(center, bspSurf->cullOrigin);
#endif //__USE_VBO_AREAS__

		if (*surface->data == SF_BAD)
		{
			ri->Printf(PRINT_WARNING, "World surface %i is SF_BAD.\n", s);
			continue;
		}

		// check for this now so we can use srfBspSurface_t* universally in the rest of the function
		if (!(*surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES))
		{
			//ri->Printf(PRINT_WARNING, "World surface %i is not SF_FACE or SF_GRID or SF_TRIANGLES.\n", s);
			continue;
		}

		if (shader->isPortal)
		{
			//ri->Printf(PRINT_WARNING, "World surface %i is PORTAL.\n", s);
			continue;
		}

		if (shader->isSky)
		{
			//ri->Printf(PRINT_WARNING, "World surface %i is SKY.\n", s);
			continue;
		}

		if (ShaderRequiresCPUDeforms(shader))
		{
			//ri->Printf(PRINT_WARNING, "World surface %i is CPU Deformed.\n", s);
			continue;
		}

		if (!bspSurf->numIndexes || !bspSurf->numVerts)
		{
			ri->Printf(PRINT_WARNING, "World surface %i has either no indexes (%i) or no verts (%i).\n", s, bspSurf->numIndexes, bspSurf->numVerts);
			continue;
		}

		numSortedSurfaces++;
	}

	// presort surfaces
	surfacesSorted = (msurface_t **)Z_Malloc(numSortedSurfaces * sizeof(*surfacesSorted), TAG_BSP);

	j = 0;
	for(surface = &s_worldData->surfaces[0]; surface < &s_worldData->surfaces[s_worldData->numsurfaces]; surface++)
	{
		srfBspSurface_t *bspSurf;
		shader_t *shader = surface->shader;

		if (*surface->data == SF_BAD)
			continue;

		if (shader->isPortal || shader->isSky || ShaderRequiresCPUDeforms(shader))
		{
			continue;
		}

		// check for this now so we can use srfBspSurface_t* universally in the rest of the function
		if (!(*surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES))
		{
			continue;
		}

		bspSurf = (srfBspSurface_t *) surface->data;

		if (!bspSurf->numIndexes || !bspSurf->numVerts)
			continue;

#ifdef __USE_VBO_AREAS__
		bspSurf->vboArea = GetVBOArea(bspSurf->cullOrigin);
		surface->vboArea = bspSurf->vboArea;

		if (r_areaVisDebug->integer)
		{
			ri->Printf(PRINT_ALL, "%.4f %.4f %.4f is in area %i. mins: %.4f %.4f %.4f. maxs: %.4f %.4f %.4f.\n", bspSurf->cullOrigin[0], bspSurf->cullOrigin[1], bspSurf->cullOrigin[2], bspSurf->vboArea, MAP_AREAS.areas[bspSurf->vboArea].mins[0], MAP_AREAS.areas[bspSurf->vboArea].mins[1], MAP_AREAS.areas[bspSurf->vboArea].mins[2], MAP_AREAS.areas[bspSurf->vboArea].maxs[0], MAP_AREAS.areas[bspSurf->vboArea].maxs[1], MAP_AREAS.areas[bspSurf->vboArea].maxs[2]);
		}
#endif //__USE_VBO_AREAS__

		surfacesSorted[j++] = surface;
	}

	qsort(surfacesSorted, numSortedSurfaces, sizeof(*surfacesSorted), BSPSurfaceCompare);

	/*
	// Debug surface sorting...
	for (currSurf = surfacesSorted, k = 0; currSurf < &surfacesSorted[numSortedSurfaces]; currSurf++, k++)
	{
		srfBspSurface_t *bspSurf = (srfBspSurface_t *)(*currSurf)->data;
		shader_t *shader = (*currSurf)->shader;
		ri->Printf(PRINT_WARNING, "QSORT DEBUG: %i - %s.\n", k, shader->name);
	}
	*/

#ifdef __USE_VBO_AREAS__
	k = 0;

	for (int a = 0; a < MAP_AREAS.numAreas; a++)
	{
#else
	k = 0;
#endif //__USE_VBO_AREAS__
		for (firstSurf = lastSurf = surfacesSorted; firstSurf < &surfacesSorted[numSortedSurfaces]; firstSurf = lastSurf)
		{
			int currVboSize, currIboSize;

			// Find range of surfaces to merge by:
			// - Collecting a number of surfaces which fit under maxVboSize/maxIboSize, or
			// - All the surfaces with a single shader which go over maxVboSize/maxIboSize
			currVboSize = currIboSize = 0;
			while (currVboSize < maxVboSize && currIboSize < maxIboSize && lastSurf < &surfacesSorted[numSortedSurfaces])
			{
				int addVboSize, addIboSize, currShaderIndex;

				addVboSize = addIboSize = 0;
				currShaderIndex = (*lastSurf)->shader->sortedIndex;

				for (currSurf = lastSurf; currSurf < &surfacesSorted[numSortedSurfaces] && (*currSurf)->shader->sortedIndex == currShaderIndex; currSurf++)
				{
					srfBspSurface_t *bspSurf = (srfBspSurface_t *)(*currSurf)->data;

#ifdef __USE_VBO_AREAS__
					if (bspSurf->vboArea != a)
					{// Not in the current VBO area, skip and it will be added to the right area later...
						continue;
					}
#endif //__USE_VBO_AREAS__

					addVboSize += bspSurf->numVerts * sizeof(srfVert_t);
					addIboSize += bspSurf->numIndexes * sizeof(glIndex_t);
				}

				if ((currVboSize != 0 && addVboSize + currVboSize > maxVboSize)
					|| (currIboSize != 0 && addIboSize + currIboSize > maxIboSize))
					break;

				lastSurf = currSurf;

				currVboSize += addVboSize;
				currIboSize += addIboSize;
			}

			// count verts/indexes/surfaces
			numVerts = 0;
			numIndexes = 0;
			numSurfaces = 0;
			for (currSurf = firstSurf; currSurf < lastSurf; currSurf++)
			{
				srfBspSurface_t *bspSurf = (srfBspSurface_t *)(*currSurf)->data;

#ifdef __USE_VBO_AREAS__
				if (bspSurf->vboArea != a)
				{// Not in the current VBO area, skip and it will be added to the right area later...
					continue;
				}
#endif //__USE_VBO_AREAS__

				numVerts += bspSurf->numVerts;
				numIndexes += bspSurf->numIndexes;
				numSurfaces++;
			}

#ifdef __USE_VBO_AREAS__
			if (numVerts == 0 && numIndexes == 0)
			{// Nothing to add...
				continue;
			}
#endif //__USE_VBO_AREAS__

			ri->Printf(PRINT_ALL, "...calculating world VBO %i ( %i verts %i tris )\n", k, numVerts, numIndexes / 3);

			// create arrays
#ifdef _WIN32
			hSharedMemory *vshared = OpenSharedMemory("BSPverts", "BSPvertsMutex", numVerts * sizeof(packedVertex_t));
			hSharedMemory *ishared = OpenSharedMemory("BSPindexes", "BSPindexesMutex", numIndexes * sizeof(glIndex_t));

			void *vdata = (void *)vshared->hFileView;
			void *idata = (void *)ishared->hFileView;

			verts = (packedVertex_t *)vshared->hFileView;
			indexes = (glIndex_t *)ishared->hFileView;
#else //!
			verts = (packedVertex_t *)ri->Hunk_AllocateTempMemory(numVerts * sizeof(packedVertex_t));
			indexes = (glIndex_t *)ri->Hunk_AllocateTempMemory(numIndexes * sizeof(glIndex_t));
#endif //_WIN32

			// set up indices and copy vertices
			numVerts = 0;
			numIndexes = 0;
			for (currSurf = firstSurf; currSurf < lastSurf; currSurf++)
			{
				srfBspSurface_t *bspSurf = (srfBspSurface_t *)(*currSurf)->data;
				glIndex_t *surfIndex;

#ifdef __USE_VBO_AREAS__
				if (bspSurf->vboArea != a)
				{// Not in the current VBO area, skip and it will be added to the right area later...
					continue;
				}
#endif //__USE_VBO_AREAS__

				bspSurf->firstIndex = numIndexes;
				bspSurf->minIndex = numVerts + bspSurf->indexes[0];
				bspSurf->maxIndex = numVerts + bspSurf->indexes[0];

				for (i = 0, surfIndex = bspSurf->indexes; i < bspSurf->numIndexes; i++, surfIndex++)
				{
					indexes[numIndexes++] = numVerts + *surfIndex;
					bspSurf->minIndex = MIN(bspSurf->minIndex, numVerts + *surfIndex);
					bspSurf->maxIndex = MAX(bspSurf->maxIndex, numVerts + *surfIndex);
				}

				bspSurf->firstVert = numVerts;


				for (i = 0; i < bspSurf->numVerts; i++)
				{
					packedVertex_t& vert = verts[numVerts++];

					VectorCopy(bspSurf->verts[i].xyz, vert.position);

					vert.normal = R_VboPackNormal(bspSurf->verts[i].normal);
					//vert.tangent = R_VboPackTangent(bspSurf->verts[i].tangent);
					VectorCopy2(bspSurf->verts[i].st, vert.texcoords[0]);

					for (int j = 0; j < MAXLIGHTMAPS; j++)
					{
						VectorCopy2(bspSurf->verts[i].lightmap[j], vert.texcoords[1 + j]);
					}

#ifndef __CHEAP_VERTS__
					for (int j = 0; j < MAXLIGHTMAPS; j++)
					{
#ifdef __VBO_PACK_COLOR__
						vert.colors[j] = R_VboPackColor(bspSurf->verts[i].vertexColors[j]);
#elif defined(__VBO_HALF_FLOAT_COLOR__)
						floattofp16(vert.colors[j], bspSurf->verts[i].vertexColors[j], 4);

						/*vec4_t fc;
						fp16tofloat(fc, vert.colors[j], 4);
						ri->Printf(PRINT_ALL, "Original %f %f %f %f. Final %f %f %f %f.\n",
							bspSurf->verts[i].vertexColors[j][0], bspSurf->verts[i].vertexColors[j][1], bspSurf->verts[i].vertexColors[j][2], bspSurf->verts[i].vertexColors[j][3],
							fc[0], fc[1], fc[2], fc[3]);*/
#else //!__VBO_PACK_COLOR__
						VectorCopy4(bspSurf->verts[i].vertexColors[j], vert.colors[j]);
						//vert.colors[j] = R_VboPackTangent(bspSurf->verts[i].vertexColors[j]);
#endif //__VBO_PACK_COLOR__
					}
#endif //__CHEAP_VERTS__

					//vert.lightDirection = R_VboPackNormal(bspSurf->verts[i].lightdir);
				}

				//bspSurf->isWorldVBO = qtrue;

				
				// It's in a VBO/IBO now, free the crap...
				if (bspSurf->sharedMemoryPointer)
				{
					CloseSharedMemory(bspSurf->sharedMemoryPointer);
					bspSurf->sharedMemoryPointer = NULL;
					ri->Hunk_FreeTempMemory(bspSurf->indexes);
				}
				else
				{
					ri->Hunk_FreeTempMemory(bspSurf->verts);
					ri->Hunk_FreeTempMemory(bspSurf->indexes);
					//ri->Printf(PRINT_WARNING, "Surf %i (%s) freed.\n", i, (*currSurf)->shader->name);
				}
			}

#ifdef __REGENERATE_BSP_NORMALS__
			GenerateSmoothNormalsForPacked(verts, numVerts);
#endif //__REGENERATE_BSP_NORMALS__

#ifdef __USE_VBO_AREAS__
			if (numVerts == 0 && numIndexes == 0)
			{// Nothing to add...
				continue;
			}
#endif //__USE_VBO_AREAS__

			vbo = R_CreateVBO((byte *)verts, sizeof(packedVertex_t) * numVerts, VBO_USAGE_STATIC);
			ibo = R_CreateIBO((byte *)indexes, numIndexes * sizeof(glIndex_t), VBO_USAGE_STATIC);

			// Setup the offsets and strides
			vbo->ofs_xyz = offsetof(packedVertex_t, position);
			vbo->ofs_normal = offsetof(packedVertex_t, normal);
			vbo->ofs_st = offsetof(packedVertex_t, texcoords);
			vbo->ofs_vertexcolor = offsetof(packedVertex_t, colors);

			const size_t packedVertexSize = sizeof(packedVertex_t);
			vbo->stride_xyz = packedVertexSize;
			vbo->stride_normal = packedVertexSize;
			vbo->stride_st = packedVertexSize;
			vbo->stride_vertexcolor = packedVertexSize;

			// point bsp surfaces to VBO
			for (currSurf = firstSurf; currSurf < lastSurf; currSurf++)
			{
				srfBspSurface_t *bspSurf = (srfBspSurface_t *)(*currSurf)->data;

#ifdef __USE_VBO_AREAS__
				if (bspSurf->vboArea != a)
				{// Not in the current VBO area, skip and it will be added to the right area later...
					continue;
				}
#endif //__USE_VBO_AREAS__

				bspSurf->vbo = vbo;
				bspSurf->ibo = ibo;

#ifdef __USE_VBO_AREAS__
				(*currSurf)->vbo = vbo;
#endif //__USE_VBO_AREAS__
			}

#ifdef _WIN32
			CloseSharedMemory(vshared);
			CloseSharedMemory(ishared);
#else //!_WIN32
			ri->Hunk_FreeTempMemory(indexes);
			ri->Hunk_FreeTempMemory(verts);
#endif //_WIN32

			k++;
		}
#ifdef __USE_VBO_AREAS__
	}
#endif //__USE_VBO_AREAS__

	Z_Free(surfacesSorted);

	endTime = ri->Milliseconds();
	ri->Printf(PRINT_ALL, "world VBOs calculation time = %5.2f seconds\n", (endTime - startTime) / 1000.0);
}

/*
===============
R_LoadSurfaces
===============
*/

bool IsSurfaceShaderValid(mdvSurface_t *surf)
{
	if (surf->shaderIndexes[0] > tr.numShaders)
		return false;

	shader_t *shader = tr.shaders[surf->shaderIndexes[0]];

	if (shader && !(shader->surfaceFlags & SURF_NODRAW) && shader != tr.defaultShader)
	{
		if (strlen(shader->name) > 0
			&& strcmp(shader->name, "collision")
			&& strcmp(shader->name, "textures/system/nodraw_solid"))
			return true;
	}

	return false;
}

int GetGoodSurfaceCount(mdvModel_t *header)
{
	int count = 0;

	for (int i = 0; i < header->numSurfaces; i++)
	{
		mdvSurface_t *surf = &header->surfaces[i];

		if (surf && surf->numIndexes > 0 && surf->numVerts > 0)
		{
			if (IsSurfaceShaderValid(surf))
				count++;
		}
	}

	return count;
}
static	void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump ) {
	dsurface_t	*in;
	msurface_t	*out;
	drawVert_t	*dv;
	int			*indexes;
	int			count;
	int			numFaces, numMeshes, numTriSurfs, numFlares;
	int			i;
	float *hdrVertColors = NULL;

#ifdef __VBO_LODMODELS__
	ri->Printf(PRINT_WARNING, "##################################################\n");
	ri->Printf(PRINT_WARNING, "There are %i lod models.\n", tr.lodModelsCount);

	int lodModelSurfaceCount = 0;

	for (i = 0; i < tr.lodModelsCount; i++)
	{// Load any needed lod models... And gather surface count...
		lodModel_t *lodModel = &tr.lodModels[i];
		lodModel->qhandle = RE_RegisterModel(lodModel->modelName);
		lodModel->model = R_GetModelByHandle(lodModel->qhandle);
		
		model_t *model = lodModel->model;

		if (model->type == MOD_MESH) {
			mdvModel_t	*header;
			header = model->data.mdv[0];

			

			//lodModelSurfaceCount += header->numSurfaces;
			lodModelSurfaceCount += GetGoodSurfaceCount(header);
		}
		/*else if (model->type == MOD_MDR) {
			mdrHeader_t	*header;
			mdrFrame_t	*frame;

			header = model->data.mdr;
		}
		else if (model->type == MOD_IQM) {
			iqmData_t *iqmData;

			iqmData = model->data.iqm;
		}*/
	}

	ri->Printf(PRINT_WARNING, "Found %i lod model surfaces.\n", lodModelSurfaceCount);
	ri->Printf(PRINT_WARNING, "##################################################\n");
#else //!__VBO_LODMODELS__
	int lodModelSurfaceCount = 0;
#endif //__VBO_LODMODELS__

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	DEBUG_StartTimer("R_LoadSurfacesAlloc", qfalse);

	if (surfs->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);
	count = surfs->filelen / sizeof(*in);

	dv = (drawVert_t *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);

	indexes = (int *)(fileBase + indexLump->fileofs);
	if ( indexLump->filelen % sizeof(*indexes))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);

	out = (msurface_t *)ri->Hunk_Alloc ( (count + lodModelSurfaceCount) * sizeof(*out), h_low );

	s_worldData->surfaces = out;
	s_worldData->numsurfaces = (count + lodModelSurfaceCount);

	s_worldData->surfacesViewCount = (int *)ri->Hunk_Alloc ( (count + lodModelSurfaceCount) * sizeof(*s_worldData->surfacesViewCount), h_low );
	//s_worldData->surfacesDlightBits = (int *)ri->Hunk_Alloc ( (count + lodModelSurfaceCount) * sizeof(*s_worldData->surfacesDlightBits), h_low );
#ifdef __PSHADOWS__
	s_worldData->surfacesPshadowBits = (int *)ri->Hunk_Alloc ( (count + lodModelSurfaceCount) * sizeof(*s_worldData->surfacesPshadowBits), h_low );
#endif

	// load hdr vertex colors
	if (r_hdr->integer)
	{
		char filename[MAX_QPATH];
		int size;

		Com_sprintf( filename, sizeof( filename ), "maps/%s/vertlight.raw", s_worldData->baseName);
		//ri->Printf(PRINT_ALL, "looking for %s\n", filename);

		size = ri->FS_ReadFile(filename, (void **)&hdrVertColors);

		if (hdrVertColors)
		{
			//ri->Printf(PRINT_ALL, "Found!\n");
			if (size != sizeof(float) * 3 * (verts->filelen / sizeof(*dv)))
				ri->Error(ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, (int)((sizeof(float)) * 3 * (verts->filelen / sizeof(*dv))));
		}
	}


	// Two passes, allocate surfaces first, then load them full of data
	// This ensures surfaces are close together to reduce L2 cache misses when using VBOs,
	// which don't actually use the verts and indexes
	in = (dsurface_t *)(fileBase + surfs->fileofs);
	out = s_worldData->surfaces;
	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
			case MST_PATCH:
				// FIXME: do this
				break;
			case MST_FOLIAGE:
			case MST_TRIANGLE_SOUP:
				out->data = (surfaceType_t *)ri->Hunk_Alloc( sizeof(srfBspSurface_t), h_low);
				break;
			case MST_PLANAR:
				out->data = (surfaceType_t *)ri->Hunk_Alloc( sizeof(srfBspSurface_t), h_low);
				break;
			case MST_FLARE:
				out->data = (surfaceType_t *)ri->Hunk_Alloc( sizeof(srfFlare_t), h_low);
				break;
			default:
				break;
		}
	}

#ifdef __VBO_LODMODELS__
	if (lodModelSurfaceCount > 0)
	{// UQ1: Alloc out->data for extra lodmodel surfaces...
		for (i = count; i < count + lodModelSurfaceCount; i++, out++) {
			out->data = (surfaceType_t *)ri->Hunk_Alloc(sizeof(srfBspSurface_t), h_low);
		}
	}
#endif //__VBO_LODMODELS__

	DEBUG_EndTimer(qfalse);

	in = (dsurface_t *)(fileBase + surfs->fileofs);
	out = s_worldData->surfaces;

	DEBUG_StartTimer("R_LoadSurfacesParse", qfalse);

	for ( i = 0 ; i < count ; i++) 
	{
		dsurface_t *thisIn = in + i;
		msurface_t *thisOut = out + i;
		switch ( LittleLong(thisIn->surfaceType ) ) {
		case MST_PATCH:
			ParseMesh (thisIn, dv, hdrVertColors, thisOut);
			{
				srfBspSurface_t *surface = (srfBspSurface_t *)thisOut->data;

				thisOut->cullinfo.type = CULLINFO_BOX | CULLINFO_SPHERE;
				VectorCopy(surface->cullBounds[0], thisOut->cullinfo.bounds[0]);
				VectorCopy(surface->cullBounds[1], thisOut->cullinfo.bounds[1]);
				VectorCopy(surface->cullOrigin, thisOut->cullinfo.localOrigin);
				thisOut->cullinfo.radius = surface->cullRadius;
			}
			numMeshes++;
			break;
		case MST_FOLIAGE:
		case MST_TRIANGLE_SOUP:
			
			if (!thisIn->numIndexes)
			{
				ri->Printf(PRINT_WARNING, "Triangle soup surface %i has %u indexes in bsp data.\n", i, thisIn->numIndexes);
			}

			ParseTriSurf(thisIn, dv, hdrVertColors, thisOut, indexes, i );
			numTriSurfs++;
			break;
		case MST_PLANAR:
			if (!thisIn->numIndexes)
			{
				ri->Printf(PRINT_WARNING, "MST_PLANAR - surface %i has %u indexes in bsp data.\n", i, thisIn->numIndexes);
			}

			ParseFace(thisIn, dv, hdrVertColors, thisOut, indexes );
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare(thisIn, dv, thisOut, indexes );
			{
				thisOut->cullinfo.type = CULLINFO_NONE;
			}
			numFlares++;
			break;
		default:
			ri->Error( ERR_DROP, "Bad surfaceType" );
		}
	}

#ifdef __VBO_LODMODELS__
	// UQ1: Add extra lodmodel surfaces...
	out = s_worldData->surfaces;
	int numSurfsAdded = 0;
	int j;
	for (i = count, j = 0; i < count + lodModelSurfaceCount && j < tr.lodModelsCount; i++, j++) {
		msurface_t *thisOut = out + i;
		lodModel_t *lodModel = &tr.lodModels[j];

		model_t *model = lodModel->model;

		if (model->type == MOD_MESH) {
			mdvModel_t	*header = model->data.mdv[0];

			for (int k = 0; k < header->numSurfaces; k++)
			{
				mdvSurface_t *surf = &header->surfaces[k];

				if (IsSurfaceShaderValid(surf) && surf->numIndexes > 0 && surf->numVerts > 0)
				{
					ParseLodModelTriSurf(lodModel, header, k, thisOut);

					shader_t *shader = tr.shaders[surf->shaderIndexes[0]];
					numSurfsAdded++;
					ri->Printf(PRINT_WARNING, "Added lodmodel surface %i of %i. %i verts. %i indexes. shader: %s.\n", numSurfsAdded, lodModelSurfaceCount, surf->numIndexes, surf->numVerts, shader->name);

					numTriSurfs++;
				}
			}
		}
	}
#endif //__VBO_LODMODELS__

	DEBUG_EndTimer(qfalse);

	if (hdrVertColors)
	{
		ri->FS_FreeFile(hdrVertColors);
	}

#ifdef PATCH_STITCHING
	DEBUG_StartTimer("R_LoadSurfacesStitch", qfalse);
	R_StitchAllPatches();
	DEBUG_EndTimer(qfalse);
#endif

	DEBUG_StartTimer("R_LoadSurfacesFixVertexLodError", qfalse);
	R_FixSharedVertexLodError();
	DEBUG_EndTimer(qfalse);

#ifdef PATCH_STITCHING
	DEBUG_StartTimer("R_LoadSurfacesMoveToHunk", qfalse);
	R_MovePatchSurfacesToHunk();
	DEBUG_EndTimer(qfalse);
#endif

	ri->Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", 
		numFaces, numMeshes, numTriSurfs, numFlares );
}



/*
=================
R_LoadSubmodels
=================
*/
static	void R_LoadSubmodels( lump_t *l ) {
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);
	count = l->filelen / sizeof(*in);

	s_worldData->numBModels = count;
	s_worldData->bmodels = out = (bmodel_t *)ri->Hunk_Alloc( count * sizeof(*out), h_low );

	for ( i=0 ; i<count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );			// this should never happen
		if ( model == NULL ) {
			ri->Error(ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed");
		}

		model->type = MOD_BRUSH;
		model->data.bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for (j=0 ; j<3 ; j++) {
			out->bounds[0][j] = LittleFloat (in->mins[j]);
			out->bounds[1][j] = LittleFloat (in->maxs[j]);
		}

		CModelCache->InsertLoaded (model->name, model->index);

		out->firstSurface = LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );

		if(i == 0)
		{
			// Add this for limiting VBO surface creation
			s_worldData->numWorldSurfaces = out->numSurfaces;
		}

#ifdef __REGENERATE_BSP_NORMALS__
		/*
		for (int s = 0; s < out->numSurfaces; s++)
		{
			msurface_t *surf = s_worldData->surfaces + (out->firstSurface + s);
			srfBspSurface_t *bspSurf = (srfBspSurface_t*)surf->data;
			GenerateNormalsForBSPSurface(bspSurf);
			GenerateSmoothNormalsForSurface(bspSurf);
		}
		*/
#endif //__REGENERATE_BSP_NORMALS__
	}
}



//==================================================================

/*
=================
R_SetParent
=================
*/
static	void R_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	R_SetParent (node->children[0], node);
	R_SetParent (node->children[1], node);
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static	void R_LoadNodesAndLeafs (lump_t *nodeLump, lump_t *leafLump) {
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*out;
	int			numNodes, numLeafs;

	in = (dnode_t *)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(dnode_t) ||
		leafLump->filelen % sizeof(dleaf_t) ) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);
	}
	numNodes = nodeLump->filelen / sizeof(dnode_t);
	numLeafs = leafLump->filelen / sizeof(dleaf_t);

	out = (mnode_t *)ri->Hunk_Alloc ( (numNodes + numLeafs) * sizeof(*out), h_low);	

	s_worldData->nodes = out;
	s_worldData->numnodes = numNodes + numLeafs;
	s_worldData->numDecisionNodes = numNodes;

	// load nodes
	for ( i=0 ; i<numNodes; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (in->mins[j]);
			out->maxs[j] = LittleLong (in->maxs[j]);
		}
	
		p = LittleLong(in->planeNum);
		out->plane = s_worldData->planes + p;

		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = s_worldData->nodes + p;
			else
				out->children[j] = s_worldData->nodes + numNodes + (-1 - p);
		}
	}
	
	// load leafs
	inLeaf = (dleaf_t *)(fileBase + leafLump->fileofs);
	for ( i=0 ; i<numLeafs ; i++, inLeaf++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (inLeaf->mins[j]);
			out->maxs[j] = LittleLong (inLeaf->maxs[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if ( out->cluster >= s_worldData->numClusters ) {
			s_worldData->numClusters = out->cluster + 1;
		}

		out->firstmarksurface = LittleLong(inLeaf->firstLeafSurface);
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}	

	// chain decendants
	R_SetParent (s_worldData->nodes, NULL);
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static	void R_LoadShaders( lump_t *l ) {
#if 1 // Renderer seems to dislike sharing these.
	int		i, count;
	dshader_t	*in, *out;
	
	in = (dshader_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);
	count = l->filelen / sizeof(*in);
	out = (dshader_t *)ri->Hunk_Alloc ( count*sizeof(*out), h_low );

	s_worldData->shaders = out;
	s_worldData->numShaders = count;

	Com_Memcpy( out, in, count*sizeof(*out) );

	for ( i=0 ; i<count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
		//out[i].materialType = LittleLong( out[i].materialType );
	}
#else
	s_worldData->shaders = (dshader_t *)ri->CM_GetShaderData();
	s_worldData->numShaders = (int)ri->CM_GetShaderDataCount();
#endif
}


/*
=================
R_LoadMarksurfaces
=================
*/
static	void R_LoadMarksurfaces (lump_t *l, bool isMainWorld)
{
	if (!isMainWorld)
	{
		int		i, j, count;
		int		*in;
		int     *out;

		in = (int *)(fileBase + l->fileofs);
		if (l->filelen % sizeof(*in))
			ri->Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData->name);
		count = l->filelen / sizeof(*in);
		out = (int *)ri->Hunk_Alloc(count * sizeof(*out), h_low);

		s_worldData->marksurfaces = out;
		s_worldData->nummarksurfaces = count;

		for (i = 0; i < count; i++)
		{
			j = LittleLong(in[i]);
			out[i] = j;
		}
	}
	else
	{
		s_worldData->marksurfaces = (int *)ri->CM_GetLeafSurfacesData();
		s_worldData->nummarksurfaces = (int)ri->CM_GetLeafSurfacesDataCount();
	}
}


/*
=================
R_LoadPlanes
=================
*/
static	void R_LoadPlanes( lump_t *l, bool isMainWorld) {
	if (!isMainWorld)
	{
		int			i, j;
		cplane_t	*out;
		dplane_t 	*in;
		int			count;
		int			bits;

		in = (dplane_t *)(fileBase + l->fileofs);
		if (l->filelen % sizeof(*in))
			ri->Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData->name);
		count = l->filelen / sizeof(*in);
		out = (cplane_t *)ri->Hunk_Alloc(count * 2 * sizeof(*out), h_low);

		s_worldData->planes = out;
		s_worldData->numplanes = count;

		for (i = 0; i < count; i++, in++, out++) {
			bits = 0;
			for (j = 0; j < 3; j++) {
				out->normal[j] = LittleFloat(in->normal[j]);
				if (out->normal[j] < 0) {
					bits |= 1 << j;
				}
			}

			out->dist = LittleFloat(in->dist);
			out->type = PlaneTypeForNormal(out->normal);
			out->signbits = bits;
		}
	}
	else
	{
		s_worldData->planes = (cplane_t *)ri->CM_GetPlanesData();
		s_worldData->numplanes = (int)ri->CM_GetPlanesDataCount();
	}
}

/*
=================
R_LoadFogs

=================
*/
static	void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump ) {
	int			i;
	fog_t		*out;
	dfog_t		*fogs;
	dbrush_t 	*brushes, *brush;
	dbrushside_t	*sides;
	int			count, brushesCount, sidesCount;
	int			sideNum;
	int			planeNum;
	shader_t	*shader;
	float		d;
	int			firstSide;

	fogs = (dfog_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);
	}
	count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	s_worldData->numfogs = count + 1;
	s_worldData->fogs = (fog_t *)ri->Hunk_Alloc ( s_worldData->numfogs*sizeof(*out), h_low);
	out = s_worldData->fogs + 1;

	if ( !count ) {
		return;
	}

	brushes = (dbrush_t *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);
	}
	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (dbrushside_t *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides)) {
		ri->Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData->name);
	}
	sidesCount = sidesLump->filelen / sizeof(*sides);

	for ( i=0 ; i<count ; i++, fogs++) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		if ( out->originalBrushNumber == -1 )
		{
			out->bounds[0][0] = out->bounds[0][1] = out->bounds[0][2] = MIN_WORLD_COORD;
			out->bounds[1][0] = out->bounds[1][1] = out->bounds[1][2] = MAX_WORLD_COORD;
			firstSide = -1;
		}
		else
		{
			if ( (unsigned)out->originalBrushNumber >= brushesCount ) {
				ri->Error( ERR_DROP, "fog brushNumber out of range" );
			}
			brush = brushes + out->originalBrushNumber;

			firstSide = LittleLong( brush->firstSide );

				if ( (unsigned)firstSide > sidesCount - 6 ) {
				ri->Error( ERR_DROP, "fog brush sideNumber out of range" );
			}

			// brushes are always sorted with the axial sides first
			sideNum = firstSide + 0;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][0] = -s_worldData->planes[ planeNum ].dist;

			sideNum = firstSide + 1;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][0] = s_worldData->planes[ planeNum ].dist;

			sideNum = firstSide + 2;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][1] = -s_worldData->planes[ planeNum ].dist;

			sideNum = firstSide + 3;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][1] = s_worldData->planes[ planeNum ].dist;

			sideNum = firstSide + 4;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[0][2] = -s_worldData->planes[ planeNum ].dist;

			sideNum = firstSide + 5;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[1][2] = s_worldData->planes[ planeNum ].dist;
		}

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, lightmapsNone, stylesDefault, qtrue );

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4 ( shader->fogParms.color[0] * tr.identityLight, 
			                          shader->fogParms.color[1] * tr.identityLight, 
			                          shader->fogParms.color[2] * tr.identityLight, 1.0 );

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		out->hasSurface = qtrue;
		if ( sideNum != -1 ) {
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData->planes[ planeNum ].normal, out->surface );
			out->surface[3] = -s_worldData->planes[ planeNum ].dist;
		}

		out++;
	}

}


/*
================
R_LoadLightGrid

================
*/
void R_LoadLightGrid( lump_t *l ) {
	int		i;
	vec3_t	maxs;
	world_t	*w;
	float	*wMins, *wMaxs;

	w = s_worldData;

	w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

	wMins = w->bmodels[0].bounds[0];
	wMaxs = w->bmodels[0].bounds[1];

	for ( i = 0 ; i < 3 ; i++ ) {
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
		maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i])/w->lightGridSize[i] + 1;
	}

	int numGridDataElements = l->filelen / sizeof(*w->lightGridData);

	w->lightGridData = (mgrid_t *)ri->Hunk_Alloc( l->filelen, h_low );
	Com_Memcpy( w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen );

	// deal with overbright bits

	for ( i = 0 ; i < numGridDataElements ; i++ ) 
	{
		for(int j = 0; j < MAXLIGHTMAPS; j++)
		{
			R_ColorShiftLightingBytes(
				w->lightGridData[i].ambientLight[j],
				w->lightGridData[i].ambientLight[j]);
			R_ColorShiftLightingBytes(
				w->lightGridData[i].directLight[j],
				w->lightGridData[i].directLight[j]);
		}
	}

	// load hdr lightgrid
	if (r_hdr->integer)
	{
		char filename[MAX_QPATH];
		float *hdrLightGrid;
		int size;

		Com_sprintf( filename, sizeof( filename ), "maps/%s/lightgrid.raw", s_worldData->baseName);
		//ri->Printf(PRINT_ALL, "looking for %s\n", filename);

		size = ri->FS_ReadFile(filename, (void **)&hdrLightGrid);

		if (hdrLightGrid && size)
		{
			float lightScale = pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits);

			//ri->Printf(PRINT_ALL, "found!\n");

			if (size != sizeof(float) * 6 * numGridDataElements)
			{
				ri->Error(ERR_DROP, "Bad size for %s (%i, expected %i)!", filename, size, (int)(sizeof(float)) * 6 * numGridDataElements);
			}

			w->hdrLightGrid = (float *)ri->Hunk_Alloc(size, h_low);

			for (i = 0; i < numGridDataElements ; i++)
			{
				w->hdrLightGrid[i * 6    ] = hdrLightGrid[i * 6    ] * lightScale;
				w->hdrLightGrid[i * 6 + 1] = hdrLightGrid[i * 6 + 1] * lightScale;
				w->hdrLightGrid[i * 6 + 2] = hdrLightGrid[i * 6 + 2] * lightScale;
				w->hdrLightGrid[i * 6 + 3] = hdrLightGrid[i * 6 + 3] * lightScale;
				w->hdrLightGrid[i * 6 + 4] = hdrLightGrid[i * 6 + 4] * lightScale;
				w->hdrLightGrid[i * 6 + 5] = hdrLightGrid[i * 6 + 5] * lightScale;
			}
		}

		if (hdrLightGrid && size)
			ri->FS_FreeFile(hdrLightGrid);
	}
}

/*
================
R_LoadLightGridArray

================
*/
void R_LoadLightGridArray( lump_t *l ) {
	world_t	*w;

	w = s_worldData;

	w->numGridArrayElements = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if ( (unsigned)l->filelen != w->numGridArrayElements * sizeof(*w->lightGridArray) ) {
		Com_Printf (S_COLOR_YELLOW  "WARNING: light grid array mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridArray = (unsigned short *)Hunk_Alloc( l->filelen, h_low );
	memcpy( w->lightGridArray, (void *)(fileBase + l->fileofs), l->filelen );
}

/*
================
R_LoadEntities
================
*/
extern int MAP_MAX_VIS_RANGE;

qboolean R_SpawnString(const char *key, const char *defaultString, char **out) {
	int i;

	//if (!tr.spawning) {
	//	*out = (char*)defaultString;
	//	// trap->Error( ERR_DROP, "CG_SpawnString() called while not spawning" );
	//}

	for (i = 0; i < tr.numSpawnVars; i++) {
		if (!Q_stricmp(key, tr.spawnVars[i][0])) {
			*out = tr.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char*)defaultString;
	return qfalse;
}
qboolean R_SpawnFloat(const char *key, const char *defaultString, float *out) {
	char *s;
	qboolean present;

	present = R_SpawnString(key, defaultString, &s);
	*out = atof(s);
	return present;
}
qboolean R_SpawnInt(const char *key, const char *defaultString, int *out) {
	char *s;
	qboolean present;

	present = R_SpawnString(key, defaultString, &s);
	*out = atoi(s);
	return present;
}
qboolean R_SpawnBoolean(const char *key, const char *defaultString, qboolean *out) {
	char *s;
	qboolean present;

	present = R_SpawnString(key, defaultString, &s);
	if (!Q_stricmp(s, "qfalse") || !Q_stricmp(s, "false") || !Q_stricmp(s, "no") || !Q_stricmp(s, "0")) {
		*out = qfalse;
	}
	else if (!Q_stricmp(s, "qtrue") || !Q_stricmp(s, "true") || !Q_stricmp(s, "yes") || !Q_stricmp(s, "1")) {
		*out = qtrue;
	}
	else {
		*out = qfalse;
	}

	return present;
}
qboolean R_SpawnVector(const char *key, const char *defaultString, float *out) {
	char *s;
	qboolean present;

	present = R_SpawnString(key, defaultString, &s);
	sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
	return present;
}

void SP_misc_lodmodel(void) {
	char*			model = NULL;
	char*			overrideShader = NULL;
	float			angle;
	float			scale;

	R_SpawnString("model", "", &model);

	if (!model || !model[0]) {
		ri->Error(ERR_DROP, "misc_lodmodel with no model.");
	}

	lodModel_t		*staticmodel = &tr.lodModels[tr.lodModelsCount];
	tr.lodModelsCount++;

	strcpy(staticmodel->modelName, model);

	R_SpawnVector("origin", "0 0 0", staticmodel->origin);
	R_SpawnFloat("zoffset", "0", &staticmodel->zoffset);

	if (!R_SpawnVector("angles", "0 0 0", staticmodel->angles)) {
		if (R_SpawnFloat("angle", "0", &angle)) {
			staticmodel->angles[YAW] = angle;
		}
	}

	if (!R_SpawnVector("modelscale_vec", "1 1 1", staticmodel->scale)) {
		if (R_SpawnFloat("modelscale", "1", &scale)) {
			VectorSet(staticmodel->scale, scale, scale, scale);
		}
	}

	R_SpawnString("_overrideShader", "", (char **)&staticmodel->overrideShader);

	AnglesToAxis(staticmodel->angles, staticmodel->axes);
	for (int i = 0; i < 3; i++) {
		VectorScale(staticmodel->axes[i], staticmodel->scale[i], staticmodel->axes[i]);
	}
}

char *R_AddSpawnVarToken(const char *string) {
	int l;
	char *dest;

	l = strlen(string);
	if (tr.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS) {
		ri->Error(ERR_DROP, "CG_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS");
	}

	dest = tr.spawnVarChars + tr.numSpawnVarChars;
	memcpy(dest, string, l + 1);

	tr.numSpawnVarChars += l + 1;

	return dest;
}

typedef struct spawn_s {
	const char	*name;
	void(*spawn)(void);
} spawn_t;

/* This array MUST be sorted correctly by alphabetical name field */
/* for conformity, use lower-case names too */
spawn_t spawns[] = {
	{ "misc_lodmodel",			SP_misc_lodmodel },
};

static int spawncmp(const void *a, const void *b) {
	return Q_stricmp((const char *)a, ((spawn_t*)b)->name);
}

void R_ParseEntityFromSpawnVars(void) {
	spawn_t *s;
	char *classname;

	if (R_SpawnString("classname", "", &classname)) {
		s = (spawn_t *)bsearch(classname, spawns, ARRAY_LEN(spawns), sizeof(spawn_t), spawncmp);
		if (s)
			s->spawn();
	}
}

void SP_worldspawn(void) {
	char *s;

	R_SpawnString("classname", "", &s);
	if (Q_stricmp(s, "worldspawn")) {
		ri->Error(ERR_DROP, "SP_worldspawn: The first entity isn't 'worldspawn'");
	}
}

qboolean CG_ParseSpawnVars2(void) {
	char keyname[MAX_TOKEN_CHARS];
	char token[MAX_TOKEN_CHARS];

	tr.numSpawnVars = 0;
	tr.numSpawnVarChars = 0;

	// parse the opening brace
	if (!R_GetEntityToken(token, sizeof(token))) {
		// end of spawn string
		return qfalse;
	}

	if (token[0] != '{') {
		ri->Error(ERR_DROP, "R_ParseSpawnVars: found %s when expecting {", token);
	}

	// go through all the key / value pairs
	while (1) {
		// parse key
		if (!R_GetEntityToken(keyname, sizeof(keyname))) {
			ri->Error(ERR_DROP, "R_ParseSpawnVars: EOF without closing brace");
		}

		if (keyname[0] == '}') {
			break;
		}

		// parse value
		if (!R_GetEntityToken(token, sizeof(token))) {
			ri->Error(ERR_DROP, "R_ParseSpawnVars: EOF without closing brace");
		}

		if (token[0] == '}') {
			ri->Error(ERR_DROP, "R_ParseSpawnVars: closing brace without data");
		}

		if (tr.numSpawnVars == MAX_SPAWN_VARS) {
			ri->Error(ERR_DROP, "R_ParseSpawnVars: MAX_SPAWN_VARS");
		}

		tr.spawnVars[tr.numSpawnVars][0] = R_AddSpawnVarToken(keyname);
		tr.spawnVars[tr.numSpawnVars][1] = R_AddSpawnVarToken(token);
		tr.numSpawnVars++;
	}

	return qtrue;
}

void R_ParseEntitiesFromString(void) {
	// make sure it is reset
	R_GetEntityToken(NULL, -1);

	tr.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if (!CG_ParseSpawnVars2()) {
		ri->Error(ERR_DROP, "ParseEntities: no entities");
	}

	SP_worldspawn();

	// parse ents
	while (CG_ParseSpawnVars2()) {
		R_ParseEntityFromSpawnVars();
	}
}

void R_LoadEntities( lump_t *l ) {
	const char *p;
	char *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t	*w;

	w = s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	tr.distanceCull = MAP_MAX_VIS_RANGE ? MAP_MAX_VIS_RANGE : 6000;//DEFAULT_DISTANCE_CULL;

	p = (char *)(fileBase + l->fileofs);

	// store for reference by the cgame
	w->entityString = (char *)ri->Hunk_Alloc( l->filelen + 1, h_low );
	strcpy( w->entityString, p );
	w->entityParsePoint = w->entityString;

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {	
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(keyname, token, sizeof(keyname));

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(value, token, sizeof(value));

		// check for remapping of shaders for vertex lighting
		s = "vertexremapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri->Printf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			if (r_vertexLight->integer) {
				R_RemapShader(value, s, "0");
			}
			continue;
		}
		// check for remapping of shaders
		s = "remapshader";
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri->Printf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			R_RemapShader(value, s, "0");
			continue;
		}
 		if (!Q_stricmp(keyname, "distanceCull")) {
			sscanf(value, "%f", &tr.distanceCull );
			continue;
		}
		// check for a different grid size
		if (!Q_stricmp(keyname, "gridsize")) {
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
			continue;
		}

		// check for auto exposure
		if (!Q_stricmp(keyname, "autoExposureMinMax")) {
			sscanf(value, "%f %f", &tr.autoExposureMinMax[0], &tr.autoExposureMinMax[1]);
			continue;
		}
	}

	//
	// UQ1: parsing other entities so we can grab lodmodels, and maybe other stuff later to VBO shit...
	//
	R_ParseEntitiesFromString();
}

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size ) {
	char	*s;

	if (size == -1)
	{ //force reset
		s_worldData->entityParsePoint = s_worldData->entityString;
		return qtrue;
	}

	s = COM_Parse( (const char **)&s_worldData->entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !s_worldData->entityParsePoint && !s[0] ) {
		s_worldData->entityParsePoint = s_worldData->entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}

#ifndef MAX_SPAWN_VARS
#define MAX_SPAWN_VARS 64
#endif

// derived from G_ParseSpawnVars() in g_spawn.c
static qboolean R_ParseSpawnVars( char *spawnVarChars, int maxSpawnVarChars, int *numSpawnVars, char *spawnVars[MAX_SPAWN_VARS][2] )
{
	char    keyname[MAX_TOKEN_CHARS];
	char	com_token[MAX_TOKEN_CHARS];
	int		numSpawnVarChars = 0;

	*numSpawnVars = 0;

	// parse the opening brace
	if ( !R_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		ri->Printf( PRINT_ALL, "R_ParseSpawnVars: found %s when expecting {\n",com_token );
		return qfalse;
	}

	// go through all the key / value pairs
	while ( 1 ) {  
		int keyLength, tokenLength;

		// parse key
		if ( !R_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
			return qfalse;
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value  
		if ( !R_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: EOF without closing brace\n" );
			return qfalse;
		}

		if ( com_token[0] == '}' ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: closing brace without data\n" );
			return qfalse;
		}

		if ( *numSpawnVars == MAX_SPAWN_VARS ) {
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VARS\n" );
			return qfalse;
		}

		keyLength = strlen(keyname) + 1;
		tokenLength = strlen(com_token) + 1;

		if (numSpawnVarChars + keyLength + tokenLength > maxSpawnVarChars)
		{
			ri->Printf( PRINT_ALL, "R_ParseSpawnVars: MAX_SPAWN_VAR_CHARS\n" );
			return qfalse;
		}

		strcpy(spawnVarChars + numSpawnVarChars, keyname);
		spawnVars[ *numSpawnVars ][0] = spawnVarChars + numSpawnVarChars;
		numSpawnVarChars += keyLength;

		strcpy(spawnVarChars + numSpawnVarChars, com_token);
		spawnVars[ *numSpawnVars ][1] = spawnVarChars + numSpawnVarChars;
		numSpawnVarChars += tokenLength;

		(*numSpawnVars)++;
	}

	return qtrue;
}

#ifndef __REALTIME_CUBEMAP__
static void R_LoadCubemapEntities(const char *cubemapEntityName)
{
	char spawnVarChars[2048];
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2];
	int numCubemaps = 0;

	// count cubemaps
	numCubemaps = 0;
	while(R_ParseSpawnVars(spawnVarChars, sizeof(spawnVarChars), &numSpawnVars, spawnVars))
	{
		int i;

		for (i = 0; i < numSpawnVars; i++)
		{
			if (!Q_stricmp(spawnVars[i][0], "classname") && !Q_stricmp(spawnVars[i][1], cubemapEntityName))
				numCubemaps++;
		}
	}

	if (!numCubemaps)
		return;

	tr.numCubemaps = numCubemaps;
	tr.cubemapOrigins = (vec3_t *)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemapOrigins), h_low);
	tr.cubemapRadius = (float *)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.cubemapRadius), h_low);
	tr.cubemapEnabled = (bool *)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.cubemapEnabled), h_low);
	tr.cubemapRendered = (bool *)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.cubemapRendered), h_low);
	tr.cubemaps = (image_t **)ri->Hunk_Alloc( tr.numCubemaps * sizeof(*tr.cubemaps), h_low);

#ifdef __EMISSIVE_CUBE_IBL__
	if (r_emissiveCubes->integer)
		tr.emissivemaps = (image_t **)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.emissivemaps), h_low);
#endif //__EMISSIVE_CUBE_IBL__
	
	numCubemaps = 0;
	while(R_ParseSpawnVars(spawnVarChars, sizeof(spawnVarChars), &numSpawnVars, spawnVars))
	{
		int i;
		qboolean isCubemap = qfalse;
		qboolean positionSet = qfalse;
		qboolean radiusSet = qfalse;
		vec3_t origin;
		float radius = 1024.0;

		for (i = 0; i < numSpawnVars; i++)
		{
			if (!Q_stricmp(spawnVars[i][0], "classname") && !Q_stricmp(spawnVars[i][1], cubemapEntityName))
				isCubemap = qtrue;

			if (!Q_stricmp(spawnVars[i][0], "origin"))
			{
				sscanf(spawnVars[i][1], "%f %f %f", &origin[0], &origin[1], &origin[2]);
				positionSet = qtrue;
			}

			if (!Q_stricmp(spawnVars[i][0], "radius"))
			{
				sscanf(spawnVars[i][1], "%f", &radius);
				radiusSet = qtrue;
			}
		}

		if (isCubemap && positionSet)
		{
			//ri.Printf(PRINT_ALL, "cubemap at %f %f %f\n", origin[0], origin[1], origin[2]);
			VectorCopy(origin, tr.cubemapOrigins[numCubemaps]);

			if (radiusSet)
				tr.cubemapRadius[numCubemaps] = radius;
			else
				tr.cubemapRadius[numCubemaps] = 2048.0; //1024.0;

			tr.cubemapEnabled[numCubemaps] = true;
			tr.cubemapRendered[numCubemaps] = false;
			numCubemaps++;
		}
	}
}
#endif //__REALTIME_CUBEMAP__

qboolean R_MaterialUsesCubemap ( int materialType)
{
	switch(materialType)
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		return qtrue;
		break;
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		return qfalse;
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		return qfalse;
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		return qfalse;
		break;
	case MATERIAL_CARPET:			// 27			// lush carpet
		return qfalse;
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		return qfalse;
		break;
	case MATERIAL_ROCK:				// 23			//
	case MATERIAL_STONE:
		return qfalse;
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		return qtrue;
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
		return qfalse;
		break;
	case MATERIAL_TREEBARK:
		return qfalse;
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		return qfalse;
		break;
	case MATERIAL_POLISHEDWOOD:
		return qtrue;
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		return qtrue;
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- UQ1: Used for weapons to force lower parallax and high reflection...
		return qtrue;
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		return qfalse;
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		return qfalse;
		break;
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		return qfalse;
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		return qfalse;
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		return qtrue;
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		return qfalse;
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		return qfalse;
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		return qfalse;
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		return qfalse;
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		return qfalse;
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		return qfalse;
		break;
	case MATERIAL_PLASTIC:			// 25			//
		return qtrue;
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		return qfalse;
		break;
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
		return qtrue;
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		return qtrue;
		break;
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		return qtrue;
		break;
	case MATERIAL_GLASS:			// 10			//
	case MATERIAL_DISTORTEDGLASS:
	case MATERIAL_DISTORTEDPUSH:
	case MATERIAL_DISTORTEDPULL:
	case MATERIAL_CLOAK:
		return qtrue;
		break;
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
		return qtrue;
		break;
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		return qtrue;
		break;
	case MATERIAL_PUDDLE:
		return qtrue;
		break;
	case MATERIAL_LAVA:
	case MATERIAL_EFX:
	case MATERIAL_BLASTERBOLT:
	case MATERIAL_FIRE:
	case MATERIAL_SMOKE:
	case MATERIAL_MAGIC_PARTICLES:
	case MATERIAL_MAGIC_PARTICLES_TREE:
	case MATERIAL_FIREFLIES:
	case MATERIAL_PORTAL:
	case MATERIAL_MENU_BACKGROUND:
		return qfalse;
		break;
	case MATERIAL_SKYSCRAPER:
		return qtrue;
		break;
	case MATERIAL_PROCEDURALFOLIAGE:
		return qfalse;
		break;
	default:
		return qfalse;
		break;
	}

	return qfalse;
}

qboolean IgnoreCubemapsOnMap( void )
{// Maps with known really bad FPS... Let's just forget rendering cubemaps here...
#if 0
	if (StringContainsWord(currentMapName, "jkg_mos_eisley"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin_forest"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin_small"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin7"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}

	if (StringContainsWord(currentMapName, "yavin8"))
	{// Ignore this map... We know we don't need shiny here...
		return qtrue;
	}
#endif

	return qfalse;
}

float		MAP_WATER_LEVEL = 131072.0;
float		MAP_WATER_LEVEL2 = 131072.0;

#define		MAX_GLOW_LOCATIONS 65536
int			NUM_MAP_GLOW_LOCATIONS = 0;
vec3_t		MAP_GLOW_LOCATIONS[MAX_GLOW_LOCATIONS] = { 0 };
vec4_t		MAP_GLOW_COLORS[MAX_GLOW_LOCATIONS] = { 0 };
float		MAP_GLOW_RADIUSES[MAX_GLOW_LOCATIONS] = { 0 };
float		MAP_GLOW_HEIGHTSCALES[MAX_GLOW_LOCATIONS] = { 0 };
float		MAP_GLOW_CONEANGLE[MAX_GLOW_LOCATIONS] = { 0 };
vec3_t		MAP_GLOW_CONEDIRECTION[MAX_GLOW_LOCATIONS] = { 0 };
qboolean	MAP_GLOW_COLORS_AVILABLE[MAX_GLOW_LOCATIONS] = { qfalse };

extern void R_WorldToLocal (const vec3_t world, vec3_t local);
extern void R_LocalPointToWorld (const vec3_t local, vec3_t world);

float mix(float x, float y, float a)
{
	return (1 - a)*x + a*y;
}

float sign(float x)
{
	if (x < 0.0) return -1.0;
	if (x > 0.0) return 1.0;
	return 0.0;
}

void R_AddLightVibrancy(float *color, float vibrancy)
{
	vec3_t	lumCoeff = { 0.212656f, 0.715158f, 0.072186f };  						//Calculate luma with these values
	float	max_color = max(color[0], max(color[1], color[2])); 					//Find the strongest color
	float	min_color = min(color[0], min(color[1], color[2])); 					//Find the weakest color
	float	color_saturation = max_color - min_color; 								//Saturation is the difference between min and max
	float	luma = DotProduct(lumCoeff, color); 									//Calculate luma (grey)
																					//Extrapolate between luma and original by 1 + (1-saturation) - current
	color[0] = mix(luma, color[0], (1.0 + (vibrancy * (1.0 - (sign(vibrancy) * color_saturation)))));
	color[1] = mix(luma, color[1], (1.0 + (vibrancy * (1.0 - (sign(vibrancy) * color_saturation)))));
	color[2] = mix(luma, color[2], (1.0 + (vibrancy * (1.0 - (sign(vibrancy) * color_saturation)))));
}

int R_CloseLightNear(vec3_t pos, float distance)
{
	for (int i = 0; i < NUM_MAP_GLOW_LOCATIONS; i++)
	{
		if (MAP_GLOW_CONEANGLE[i] != 0.0) continue;

		if (Distance(MAP_GLOW_LOCATIONS[i], pos) < distance)
		{
			return i;
		}
	}

	return -1;
}

int R_CloseLightOfColorNear(vec3_t pos, float distance, vec4_t color, float colorTolerance)
{
	for (int i = 0; i < NUM_MAP_GLOW_LOCATIONS; i++)
	{
		if (MAP_GLOW_CONEANGLE[i] != 0.0) continue;

		if (Distance(MAP_GLOW_LOCATIONS[i], pos) < distance)
		{
			if (Distance(MAP_GLOW_COLORS[i], color) <= colorTolerance) // forget alpha...
			{
				return i;
			}
		}
	}

	return -1;
}

qboolean CONTENTS_INSIDE_OUTSIDE_FOUND = qfalse;

#ifdef __INDOOR_OUTDOOR_CULLING__
extern int ENABLE_INDOOR_OUTDOOR_SYSTEM;
qboolean INDOOR_BRUSH_FOUND = qfalse;
#endif //__INDOOR_OUTDOOR_CULLING__

void R_CenterOfBounds(vec3_t mins, vec3_t maxs, vec3_t *center)
{
	for (int i = 0; i < 3; i++)
	{
		*center[i] = (mins[i] + maxs[i]) / 2.0;
	}
}

static void R_SetupMapGlowsAndWaterPlane( world_t *world )
{
	r_debugEmissiveLights = ri->Cvar_Get("r_debugEmissiveLights", "0", CVAR_ARCHIVE);

	qboolean setupWaterLevel = qfalse;

	world_t	*w = world;

	if (world == tr.worldSolid)
	{
		CONTENTS_INSIDE_OUTSIDE_FOUND = qfalse;
		NUM_MAP_GLOW_LOCATIONS = 0;
	}

#ifdef __INDOOR_OUTDOOR_CULLING__
	if (world == tr.worldSolid)
	{
		INDOOR_BRUSH_FOUND = qfalse;
	}

	int numIndoorSeen = 0;
	int indoorSeen = 0;
	int outdoorSeen = 0;
#endif //__INDOOR_OUTDOOR_CULLING__

	if (!(MAP_WATER_LEVEL < 131000.0 && MAP_WATER_LEVEL > -131000.0))
	{
		MAP_WATER_LEVEL = 131072.0;
		setupWaterLevel = qtrue;
	}

//#pragma omp parallel for schedule(dynamic) ordered
	for (int i = 0; i < w->numsurfaces; i++)
	{// Get a count of how many we need... Add them to temp list if not too close to another...
		msurface_t *surf =	&w->surfaces[i];
		vec3_t				surfOrigin;
		qboolean			bad = qfalse;
		float				radius = 0.0;
		float				emissiveRadiusScale = 0.0;
		float				emissiveColorScale = 0.0;
		float				emissiveHeightScale = 0.0;
		float				emissiveConeAngle = 0.0;
		vec3_t				emissiveConeDirection = { 0.0 };
		emissiveConeDirection[0] = 0.0;
		emissiveConeDirection[1] = 0.0;
		emissiveConeDirection[2] = 0.0;

		if (!surf || !surf->shader) continue;

		qboolean	hasGlow = qfalse;
		vec4_t		glowColor = { 0 };

		if (surf->shader)
		{
			if ((surf->shader->contentFlags & CONTENTS_INSIDE) || (surf->shader->contentFlags & CONTENTS_OUTSIDE))
			{
				CONTENTS_INSIDE_OUTSIDE_FOUND = qtrue;

#ifdef __INDOOR_OUTDOOR_CULLING__
				if (surf->shader->contentFlags & CONTENTS_INSIDE)
				{
					indoorSeen++;
				}

				if (surf->shader->contentFlags & CONTENTS_OUTSIDE)
				{
					outdoorSeen++;
				}
#endif //__INDOOR_OUTDOOR_CULLING__
			}

#ifdef __INDOOR_OUTDOOR_CULLING__
			if (surf->shader->isIndoor)
			{
				INDOOR_BRUSH_FOUND = qtrue;
				numIndoorSeen++;
			}
#endif //__INDOOR_OUTDOOR_CULLING__

			srfBspSurface_t *bspSurf = (srfBspSurface_t *)surf->data;

			if (!(*surf->data == SF_FACE || *surf->data == SF_GRID || *surf->data == SF_TRIANGLES))
			{// Only handle bsp surfaces...
				continue;
				//bspSurf = NULL;
			}

			for ( int stage = 0; stage < MAX_SHADER_STAGES; stage++ )
			{
				qboolean isBuilding = ((surf->shader->materialType) == MATERIAL_SKYSCRAPER && surf->shader->stages[stage] && surf->shader->stages[stage]->bundle[TB_STEEPMAP].image[0]) ? qtrue : qfalse;
				
				if (surf->shader->stages[stage] && (surf->shader->stages[stage]->glow || surf->shader->stages[stage]->glowMapped || isBuilding))
				{
					hasGlow = qtrue;

					if (isBuilding)
						VectorCopy4(surf->shader->stages[stage]->bundle[TB_STEEPMAP].image[0]->lightColor, glowColor);
					else if (surf->shader->stages[stage]->glowMapped)
						VectorCopy4(surf->shader->stages[stage]->bundle[TB_GLOWMAP].image[0]->lightColor, glowColor);
					else
						VectorCopy4(surf->shader->stages[stage]->bundle[0].image[0]->lightColor, glowColor);

					emissiveRadiusScale = surf->shader->stages[stage]->emissiveRadiusScale * surf->shader->emissiveRadiusScale;
					emissiveColorScale = surf->shader->stages[stage]->emissiveColorScale * surf->shader->emissiveColorScale;
					emissiveHeightScale = surf->shader->stages[stage]->emissiveHeightScale;

					if (surf->cullinfo.type & CULLINFO_SPHERE)
					{
						radius = surf->cullinfo.radius;
					}
					else if (surf->cullinfo.type & CULLINFO_BOX)
					{
						radius = Distance(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]) / 2.0;
					}

					if (bspSurf)
					{
						if (/*surf->shader->stages[stage]->emissiveConeAngle > 0.0 &&*/ radius >= 32.0)
						{
							emissiveConeAngle = surf->shader->stages[stage]->emissiveConeAngle;
							//VectorCopy(bspSurf->verts[0].normal, emissiveConeDirection);
							emissiveConeDirection[0] = bspSurf->verts[0].normal[0] * 2.0 - 1.0;
							emissiveConeDirection[1] = bspSurf->verts[0].normal[1] * 2.0 - 1.0;
							emissiveConeDirection[2] = bspSurf->verts[0].normal[2] * 2.0 - 1.0;
						}
						else if (surf->shader->stages[stage]->emissiveConeAngle > 0.0)
						{
							emissiveConeAngle = surf->shader->stages[stage]->emissiveConeAngle;
							emissiveConeDirection[0] = bspSurf->verts[0].normal[0] * 2.0 - 1.0;
							emissiveConeDirection[1] = bspSurf->verts[0].normal[1] * 2.0 - 1.0;
							emissiveConeDirection[2] = bspSurf->verts[0].normal[2] * 2.0 - 1.0;
						}
					}

					radius = 0;

					//ri->Printf(PRINT_WARNING, "%s color is %f %f %f.\n", surf->shader->stages[stage]->bundle[0].image[0]->imgName, glowColor[0], glowColor[1], glowColor[2]);
					break;
				}
			}

#define EMISSIVE_MERGE_RADIUS 128.0//64.0

			if (hasGlow && bspSurf && surf->shader->name && surf->shader->name[0] != 0 && !StringContainsWord(surf->shader->name, "warzone/trees"))
			{// Handle individual verts, since we can. So we don't loose individual lights with merging... Tree glows can be merged though...
				hasGlow = qfalse;

				VectorScale(glowColor, emissiveColorScale, glowColor);

				radius = 512.0;// 128.0;// 64.0;// Distance(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]) / 2.0;

			
				if (radius > 0)
				{
#define TEMP_LIGHTS_MAX 4096//1024
					int		TEMP_LIGHTS_NUM = 0;
					vec3_t	TEMP_LIGHTS_BASE_POSITION[TEMP_LIGHTS_MAX];
					vec3_t	TEMP_LIGHTS_MINS[TEMP_LIGHTS_MAX];
					vec3_t	TEMP_LIGHTS_MAXS[TEMP_LIGHTS_MAX];
					
					for (int z = 0; z < TEMP_LIGHTS_MAX; z++)
					{
						VectorClear(TEMP_LIGHTS_MINS[z]);
						VectorClear(TEMP_LIGHTS_MAXS[z]);
					}

					for (int v = 0; v < bspSurf->numVerts; v++)
					{// Create temp lights list, with any connected positions...
						srfVert_t *vert = &bspSurf->verts[v];

						if (!vert) continue;

						int CLOSE_TO_TEMP_ID = -1;

						for (int l = 0; l < TEMP_LIGHTS_NUM; l++)
						{
							//vec3_t center;
							//R_CenterOfBounds(TEMP_LIGHTS_MINS[l], TEMP_LIGHTS_MAXS[l], &center);

							if (Distance(/*center*/TEMP_LIGHTS_BASE_POSITION[l], vert->xyz) <= 448.0)
							{
								CLOSE_TO_TEMP_ID = l;
								break;
							}
						}

						if (CLOSE_TO_TEMP_ID >= 0)
						{// Add to current list for this light...
							AddPointToBounds(vert->xyz, TEMP_LIGHTS_MINS[CLOSE_TO_TEMP_ID], TEMP_LIGHTS_MAXS[CLOSE_TO_TEMP_ID]);
						}
						else
						{// Not close to another light currently in the list, so make a new light...
							if (TEMP_LIGHTS_NUM + 1 >= TEMP_LIGHTS_MAX)
							{
								ri->Printf(PRINT_WARNING, "Hit max temp lights...\n");
							}
							else
							{
								VectorCopy(vert->xyz, TEMP_LIGHTS_BASE_POSITION[TEMP_LIGHTS_NUM]);
								VectorCopy(vert->xyz, TEMP_LIGHTS_MINS[TEMP_LIGHTS_NUM]);
								VectorCopy(vert->xyz, TEMP_LIGHTS_MAXS[TEMP_LIGHTS_NUM]);
								TEMP_LIGHTS_NUM++;
							}
						}
					}

					for (int l = 0; l < TEMP_LIGHTS_NUM; l++)
					{// Add the new light...
						// Now all the positions have been merged into mins/maxs. Grab the central point and generate a radius...
						vec3_t surfOrigin;
						surfOrigin[0] = (TEMP_LIGHTS_MINS[l][0] + TEMP_LIGHTS_MAXS[l][0]) / 2.0;
						surfOrigin[1] = (TEMP_LIGHTS_MINS[l][1] + TEMP_LIGHTS_MAXS[l][1]) / 2.0;
						surfOrigin[2] = (TEMP_LIGHTS_MINS[l][2] + TEMP_LIGHTS_MAXS[l][2]) / 2.0;

						radius = Distance(TEMP_LIGHTS_MINS[l], TEMP_LIGHTS_MAXS[l]);

						// Now we have a central point and radius, make a new emissive light there...
						if (NUM_MAP_GLOW_LOCATIONS < MAX_GLOW_LOCATIONS)
						{
							radius = Q_clamp(128.0/*64.0*/, radius, 192.0/*128.0*/);

#pragma omp critical (__MAP_GLOW_ADD__)
							{
								VectorCopy(surfOrigin, MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS]);
								VectorCopy4(glowColor, MAP_GLOW_COLORS[NUM_MAP_GLOW_LOCATIONS]);
								MAP_GLOW_RADIUSES[NUM_MAP_GLOW_LOCATIONS] = radius * emissiveRadiusScale * 2.25;
								MAP_GLOW_HEIGHTSCALES[NUM_MAP_GLOW_LOCATIONS] = emissiveHeightScale;

								MAP_GLOW_CONEANGLE[NUM_MAP_GLOW_LOCATIONS] = emissiveConeAngle;
								VectorCopy(emissiveConeDirection, MAP_GLOW_CONEDIRECTION[NUM_MAP_GLOW_LOCATIONS]);

								MAP_GLOW_COLORS_AVILABLE[NUM_MAP_GLOW_LOCATIONS] = qtrue;

								if (r_debugEmissiveLights->integer)
								{
									ri->Printf(PRINT_WARNING, "(Vert) Light %i (at %i %i %i) radius %f. emissiveColorScale %f. emissiveRadiusScale %f. color %f %f %f. coneAngle %f. coneDirection %f %f %f.\n"
										, NUM_MAP_GLOW_LOCATIONS
										, (int)MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS][0], (int)MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS][1], (int)MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS][2]
										, MAP_GLOW_RADIUSES[NUM_MAP_GLOW_LOCATIONS]
										, emissiveColorScale
										, emissiveRadiusScale
										, glowColor[0], glowColor[1], glowColor[2]
										, emissiveConeAngle
										, emissiveConeDirection[0], emissiveConeDirection[1], emissiveConeDirection[2]);
								}

								NUM_MAP_GLOW_LOCATIONS++;
							}
						}
					}
				}

				radius = 0.0;
			}
		}

		if (surf->cullinfo.type & CULLINFO_SPHERE)
		{
			VectorCopy(surf->cullinfo.localOrigin, surfOrigin);
			radius = surf->cullinfo.radius;
		}
		else if (surf->cullinfo.type & CULLINFO_BOX)
		{
			surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
			surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
			surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;
			radius = Distance(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1]) / 2.0;
		}
		else
		{
			continue;
		}

		if (setupWaterLevel && (surf->shader->materialType) == MATERIAL_WATER)
		{// While doing this, also find lowest water height, so that we can cull underwater grass drawing...
			if (surfOrigin[2] < MAP_WATER_LEVEL)
			{
#pragma omp critical (__MAP_WATER_LEVEL__)
				{
					MAP_WATER_LEVEL2 = MAP_WATER_LEVEL;
					MAP_WATER_LEVEL = surfOrigin[2];
				}
			}
		}

		if (hasGlow)
		{
			VectorScale(glowColor, emissiveColorScale, glowColor);

#ifdef __ALLOW_MAP_GLOWS_MERGE__

			int sameColorTooCloseID = R_CloseLightOfColorNear(surfOrigin, 16.0, glowColor, 99999.0);
			int sameColorGlowNearID = R_CloseLightOfColorNear(surfOrigin, EMISSIVE_MERGE_RADIUS, glowColor, 1.0);

			if (emissiveConeAngle == 0.0 && sameColorTooCloseID >= 0)
			{// Don't add this duplicate light at all... Just mix the colors... In case theres 2 overlayed textures of diff colors, etc...
				MAP_GLOW_COLORS[sameColorTooCloseID][0] = (MAP_GLOW_COLORS[sameColorTooCloseID][0] + glowColor[0]) / 2.0;
				MAP_GLOW_COLORS[sameColorTooCloseID][1] = (MAP_GLOW_COLORS[sameColorTooCloseID][1] + glowColor[1]) / 2.0;
				MAP_GLOW_COLORS[sameColorTooCloseID][2] = (MAP_GLOW_COLORS[sameColorTooCloseID][2] + glowColor[2]) / 2.0;

				if (r_debugEmissiveLights->integer)
				{
					ri->Printf(PRINT_WARNING, "Light %i (at %i %i %i) was mixed colors with another really close light (at %i %i %i). new color: %f %f %f.\n"
						, sameColorTooCloseID
						, (int)MAP_GLOW_LOCATIONS[sameColorTooCloseID][0], (int)MAP_GLOW_LOCATIONS[sameColorTooCloseID][1], (int)MAP_GLOW_LOCATIONS[sameColorTooCloseID][2]
						, (int)surfOrigin[0], (int)surfOrigin[1], (int)surfOrigin[2]
						, MAP_GLOW_COLORS[sameColorTooCloseID][0], MAP_GLOW_COLORS[sameColorTooCloseID][1], MAP_GLOW_COLORS[sameColorTooCloseID][2]);
				}
			}
			else if (emissiveConeAngle == 0.0 && sameColorGlowNearID >= 0)
			{// Already the same color light nearby... Merge...
				// Add extra radius to the original one, instead of adding a new light...
				float distFromOther = Distance(MAP_GLOW_LOCATIONS[sameColorGlowNearID], surfOrigin);
				float radiusMult = Q_clamp(0.0, distFromOther / EMISSIVE_MERGE_RADIUS, 1.0);
				MAP_GLOW_RADIUSES[sameColorGlowNearID] += MAP_GLOW_RADIUSES[sameColorGlowNearID] * radiusMult * 0.5;
				
				// Also move the light's position to the center of the 2 positions...
				vec3_t originalOrigin;

				if (r_debugEmissiveLights->integer)
				{
					VectorCopy(MAP_GLOW_LOCATIONS[sameColorGlowNearID], originalOrigin);
				}

				MAP_GLOW_LOCATIONS[sameColorGlowNearID][0] = surfOrigin[0] + MAP_GLOW_LOCATIONS[sameColorGlowNearID][0] / 2.0;
				MAP_GLOW_LOCATIONS[sameColorGlowNearID][1] = surfOrigin[1] + MAP_GLOW_LOCATIONS[sameColorGlowNearID][1] / 2.0;
				MAP_GLOW_LOCATIONS[sameColorGlowNearID][2] = surfOrigin[2] + MAP_GLOW_LOCATIONS[sameColorGlowNearID][2] / 2.0;

				MAP_GLOW_COLORS[sameColorGlowNearID][0] = (MAP_GLOW_COLORS[sameColorGlowNearID][0] + glowColor[0]) / 2.0;
				MAP_GLOW_COLORS[sameColorGlowNearID][1] = (MAP_GLOW_COLORS[sameColorGlowNearID][1] + glowColor[1]) / 2.0;
				MAP_GLOW_COLORS[sameColorGlowNearID][2] = (MAP_GLOW_COLORS[sameColorGlowNearID][2] + glowColor[2]) / 2.0;

				if (r_debugEmissiveLights->integer)
				{
					ri->Printf(PRINT_WARNING, "Light %i (at %i %i %i) was merged with another close light (at %i %i %i). new origin: %i %i %i. new radius %f. light color: %f %f %f.\n"
						, sameColorGlowNearID
						, (int)originalOrigin[0], (int)originalOrigin[1], (int)originalOrigin[2]
						, (int)surfOrigin[0], (int)surfOrigin[1], (int)surfOrigin[2]
						, (int)MAP_GLOW_LOCATIONS[sameColorGlowNearID][0], (int)MAP_GLOW_LOCATIONS[sameColorGlowNearID][1], (int)MAP_GLOW_LOCATIONS[sameColorGlowNearID][2]
						, MAP_GLOW_RADIUSES[sameColorGlowNearID]
						, MAP_GLOW_COLORS[sameColorGlowNearID][0], MAP_GLOW_COLORS[sameColorGlowNearID][1], MAP_GLOW_COLORS[sameColorGlowNearID][2]);
				}
			}
			else
#endif //__ALLOW_MAP_GLOWS_MERGE__
			if (NUM_MAP_GLOW_LOCATIONS < MAX_GLOW_LOCATIONS)
			{
				radius = Q_clamp(64.0, radius, 128.0);
				
				#pragma omp critical (__MAP_GLOW_ADD__)
				{
					VectorCopy(surfOrigin, MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS]);
					VectorCopy4(glowColor, MAP_GLOW_COLORS[NUM_MAP_GLOW_LOCATIONS]);
					MAP_GLOW_RADIUSES[NUM_MAP_GLOW_LOCATIONS] = radius * emissiveRadiusScale * 2.25;
					MAP_GLOW_HEIGHTSCALES[NUM_MAP_GLOW_LOCATIONS] = emissiveHeightScale;
					
					MAP_GLOW_CONEANGLE[NUM_MAP_GLOW_LOCATIONS] = emissiveConeAngle;
					VectorCopy(emissiveConeDirection, MAP_GLOW_CONEDIRECTION[NUM_MAP_GLOW_LOCATIONS]);
					
					MAP_GLOW_COLORS_AVILABLE[NUM_MAP_GLOW_LOCATIONS] = qtrue;

					if (r_debugEmissiveLights->integer)
					{
						ri->Printf(PRINT_WARNING, "Light %i (at %i %i %i) radius %f. emissiveColorScale %f. emissiveRadiusScale %f. color %f %f %f. coneAngle %f. coneDirection %f %f %f.\n"
							, NUM_MAP_GLOW_LOCATIONS
							, (int)MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS][0], (int)MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS][1], (int)MAP_GLOW_LOCATIONS[NUM_MAP_GLOW_LOCATIONS][2]
							, MAP_GLOW_RADIUSES[NUM_MAP_GLOW_LOCATIONS]
							, emissiveColorScale
							, emissiveRadiusScale
							, glowColor[0], glowColor[1], glowColor[2]
							, emissiveConeAngle
							, emissiveConeDirection[0], emissiveConeDirection[1], emissiveConeDirection[2]);
					}

					NUM_MAP_GLOW_LOCATIONS++;
				}
			}
		}
	}

#ifdef __INDOOR_OUTDOOR_CULLING__
	if (ENABLE_INDOOR_OUTDOOR_SYSTEM > 1)
		ri->Printf(PRINT_WARNING, "Found %i wzindoor, %i jka indoor, and %i jka outdoor surfaces.\n", numIndoorSeen, indoorSeen, outdoorSeen);

	if (ENABLE_INDOOR_OUTDOOR_SYSTEM && CONTENTS_INSIDE_OUTSIDE_FOUND)
	{// Mark any old school indoor/outdoor stuff as best we can...
		INDOOR_BRUSH_FOUND = qtrue;

		for (int i = 0; i < w->numsurfaces; i++)
		{// Get a count of how many we need... Add them to temp list if not too close to another...
			msurface_t *surf = &w->surfaces[i];
			
			if (surf->shader)
			{
				if (surf->shader->contentFlags & CONTENTS_INSIDE)
				{
					surf->shader->isIndoor = qtrue;
				}
				else if (surf->shader->contentFlags & CONTENTS_OUTSIDE)
				{
					surf->shader->isIndoor = qfalse;
				}
				else if (indoorSeen || outdoorSeen)
				{
					if (indoorSeen)
					{// We have seen indoors, and this is not marked, so assume outdoor.
						surf->shader->isIndoor = qfalse;
					}
					else if (outdoorSeen)
					{// We have seen outdoors, and this is not marked, so assume indoor.
						surf->shader->isIndoor = qtrue;
					}
				}
				else
				{// Nothing was marked, assume outdoor on everything... Trace???
					surf->shader->isIndoor = qfalse;
				}
			}
		}
	}
#endif //__INDOOR_OUTDOOR_CULLING__

	if (MAP_WATER_LEVEL2 < 131000.0 && MAP_WATER_LEVEL2 > -131000.0)
	{// If we have a secondary water level, use it instead, it should be the top of the water, not the bottom plane.
		MAP_WATER_LEVEL = MAP_WATER_LEVEL2;
	}

	if (!(MAP_WATER_LEVEL < 131000.0 && MAP_WATER_LEVEL > -131000.0))
	{// No water plane was found, set to map mins...
		MAP_WATER_LEVEL = -131072.0;
	}

	// Add vibrancy to all the emissive lighting, and normalize to 0-1 values...
	for (int i = 0; i < NUM_MAP_GLOW_LOCATIONS; i++)
	{
		// Add vibrancy...
		//R_AddLightVibrancy(MAP_GLOW_COLORS[i], 0.1);
		R_AddLightVibrancy(MAP_GLOW_COLORS[i], 0.5);

		VectorNormalize(MAP_GLOW_COLORS[i]);
		MAP_GLOW_COLORS[i][0] = Q_clamp(0.0, MAP_GLOW_COLORS[i][0], 1.0);
		MAP_GLOW_COLORS[i][1] = Q_clamp(0.0, MAP_GLOW_COLORS[i][1], 1.0);
		MAP_GLOW_COLORS[i][2] = Q_clamp(0.0, MAP_GLOW_COLORS[i][2], 1.0);

		VectorNormalize(MAP_GLOW_COLORS[i]);
		MAP_GLOW_COLORS[i][0] = Q_clamp(0.0, MAP_GLOW_COLORS[i][0], 1.0);
		MAP_GLOW_COLORS[i][1] = Q_clamp(0.0, MAP_GLOW_COLORS[i][1], 1.0);
		MAP_GLOW_COLORS[i][2] = Q_clamp(0.0, MAP_GLOW_COLORS[i][2], 1.0);
	}

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Selected %i surfaces for glow lights.\n", "LIGHTING", NUM_MAP_GLOW_LOCATIONS);
}

#ifndef __REALTIME_CUBEMAP__
static void R_SetupCubemapPoints( void )
{
	if (IgnoreCubemapsOnMap()) return;

#if 0
	//
	// How about we simply generate a grid based on map bounds?
	//

	int numCubemaps = 0;

	extern vec3_t	MAP_INFO_MINS;
	extern vec3_t	MAP_INFO_MAXS;
	extern vec3_t	MAP_INFO_SIZE;

	world_t	*w;
	vec3_t	*cubeOrgs;
	int		numcubeOrgs = 0;

	cubeOrgs = (vec3_t *)malloc(sizeof(vec3_t) * 65568);

	if (!cubeOrgs)
	{
		ri->Error(ERR_DROP, "Failed to allocate cubeOrg memory.\n");
	}

	w = &s_worldData;

	vec3_t scatter;
	scatter[0] = (MAP_INFO_SIZE[0] / 9.0);
	scatter[1] = (MAP_INFO_SIZE[1] / 9.0);
	scatter[2] = (MAP_INFO_SIZE[2] / 9.0);

	for (float x = MAP_INFO_MINS[0] + (scatter[0] * 2.0); x <= MAP_INFO_MAXS[0] - (scatter[0] * 2.0); x += scatter[0])
	{
		for (float y = MAP_INFO_MINS[1] + (scatter[1] * 2.0); y <= MAP_INFO_MAXS[1] - (scatter[1] * 2.0); y += scatter[1])
		{
			for (float z = MAP_INFO_MINS[2] + (scatter[2]); z <= MAP_INFO_MAXS[2] - (scatter[2]); z += scatter[2])
			{
				vec3_t origin;
				VectorSet(origin, x, y, z);

				//ri->Printf(PRINT_WARNING, "Cube %i at %f %f %f.\n", numcubeOrgs, origin[0], origin[1], origin[2]);
				VectorCopy(origin, cubeOrgs[numcubeOrgs]);
				numcubeOrgs++;
			}
		}
	}

	tr.numCubemaps = numcubeOrgs;
	tr.cubemapEnabled = (bool *)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.cubemapEnabled), h_low);
	tr.cubemapOrigins = (vec3_t *)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.cubemapOrigins), h_low);
	tr.cubemapRadius = (float *)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.cubemapRadius), h_low);
	tr.cubemaps = (image_t **)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.cubemaps), h_low);

#ifdef __EMISSIVE_CUBE_IBL__
	if (r_emissiveCubes->integer)
		tr.emissivemaps = (image_t **)ri->Hunk_Alloc(tr.numCubemaps * sizeof(*tr.emissivemaps), h_low);
#endif //__EMISSIVE_CUBE_IBL__

	numCubemaps = 0;

	for (int i = 0; i < numcubeOrgs; i++)
	{// Copy to real list...
		VectorCopy(cubeOrgs[i], tr.cubemapOrigins[numCubemaps]);
		tr.cubemapRadius[numCubemaps] = 2048.0;
		tr.cubemapEnabled[numCubemaps] = true;
		numCubemaps++;
	}

	free(cubeOrgs);
#else
#define MAX_CUBEMAPS 1024

	fileHandle_t	f;

	tr.numCubemaps = 0;

	if (!tr.cubemapsAllocated)
	{// Only allocate once... Allow for reloading the cubemap locations to edit in realtime...
		tr.cubemapEnabled = (bool *)ri->Hunk_Alloc(MAX_CUBEMAPS * sizeof(*tr.cubemapEnabled), h_low);
		tr.cubemapRendered = (bool *)ri->Hunk_Alloc(MAX_CUBEMAPS * sizeof(*tr.cubemapRendered), h_low);
		tr.cubemapOrigins = (vec3_t *)ri->Hunk_Alloc(MAX_CUBEMAPS * sizeof(*tr.cubemapOrigins), h_low);
		tr.cubemapRadius = (float *)ri->Hunk_Alloc(MAX_CUBEMAPS * sizeof(*tr.cubemapRadius), h_low);
		tr.cubemaps = (image_t **)ri->Hunk_Alloc(MAX_CUBEMAPS * sizeof(*tr.cubemaps), h_low);

#ifdef __EMISSIVE_CUBE_IBL__
		if (r_emissiveCubes->integer)
			tr.emissivemaps = (image_t **)ri->Hunk_Alloc(MAX_CUBEMAPS * sizeof(*tr.emissivemaps), h_low);
#endif //__EMISSIVE_CUBE_IBL__

		tr.cubemapsAllocated = true;
	}

	ri->FS_FOpenFileRead(va("maps/%s.cubemaps", currentMapName), &f, qfalse);
	
	if (!f)
		ri->FS_FOpenFileRead(va("maps/mp/%s.cubemaps", currentMapName), &f, qfalse);

	if (!f)
		ri->FS_FOpenFileRead(va("warzone/maps/%s.cubemaps", currentMapName), &f, qfalse);

	if (!f)
		ri->FS_FOpenFileRead(va("warzone/maps/mp/%s.cubemaps", currentMapName), &f, qfalse);

	if (f)
	{// We have an actual cubemap list... Use it's locations...
		ri->FS_Read(&tr.numCubemaps, sizeof(int), f);

		// Check hard limit...
		if (tr.numCubemaps > MAX_CUBEMAPS) tr.numCubemaps = MAX_CUBEMAPS;

		for (int i = 0; i < tr.numCubemaps; i++)
		{
			ri->FS_Read(&tr.cubemapEnabled[i], sizeof(bool), f);
			ri->FS_Read(&tr.cubemapOrigins[i], sizeof(vec3_t), f);
			tr.cubemapRadius[i] = 2048.0;
		}

		ri->FS_FCloseFile(f);
	}
	else
	{
		ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: There is no maps/mapname.cubemaps file. Reverting to old method.\n", "CUBE-MAPPING");
	}

	if (tr.numCubemaps <= 0)
	{// Didn't find anything? Fall back to old system...
	 //
	 // How about we simply generate a grid based on map bounds?
	 //

		int numCubemaps = 0;

		extern vec3_t	MAP_INFO_MINS;
		extern vec3_t	MAP_INFO_MAXS;
		extern vec3_t	MAP_INFO_SIZE;

		world_t	*w;

		w = s_worldData;

		vec3_t scatter;
		scatter[0] = (MAP_INFO_SIZE[0] / 9.0);
		scatter[1] = (MAP_INFO_SIZE[1] / 9.0);
		scatter[2] = (MAP_INFO_SIZE[2] / 9.0);

		for (float x = MAP_INFO_MINS[0] + (scatter[0] * 2.0); x <= MAP_INFO_MAXS[0] - (scatter[0] * 2.0); x += scatter[0])
		{
			for (float y = MAP_INFO_MINS[1] + (scatter[1] * 2.0); y <= MAP_INFO_MAXS[1] - (scatter[1] * 2.0); y += scatter[1])
			{
				for (float z = MAP_INFO_MINS[2] + (scatter[2]); z <= MAP_INFO_MAXS[2] - (scatter[2]); z += scatter[2])
				{
					VectorSet(tr.cubemapOrigins[tr.numCubemaps], x, y, z);
					tr.cubemapRadius[tr.numCubemaps] = 2048.0;
					tr.cubemapEnabled[tr.numCubemaps] = true;
					tr.numCubemaps++;
				}
			}
		}
	}
#endif

	ri->Printf(PRINT_WARNING, "^1*** ^3%s^5: Selected %i positions for cubemaps.\n", "CUBE-MAPPING", tr.numCubemaps);
}

static void R_AssignCubemapsToWorldSurfaces(void)
{
	world_t	*w;
	int i;

	w = s_worldData;

	for (i = 0; i < w->numsurfaces; i++)
	{
		msurface_t *surf = &w->surfaces[i];
		vec3_t surfOrigin;
		
		if (!R_MaterialUsesCubemap( surf->shader->materialType) && surf->shader->customCubeMapScale <= 0.0)
		{
			surf->cubemapIndex = 0;
		}

		if (surf->cullinfo.type & CULLINFO_SPHERE)
		{
			VectorCopy(surf->cullinfo.localOrigin, surfOrigin);
		}
		else if (surf->cullinfo.type & CULLINFO_BOX)
		{
			surfOrigin[0] = (surf->cullinfo.bounds[0][0] + surf->cullinfo.bounds[1][0]) * 0.5f;
			surfOrigin[1] = (surf->cullinfo.bounds[0][1] + surf->cullinfo.bounds[1][1]) * 0.5f;
			surfOrigin[2] = (surf->cullinfo.bounds[0][2] + surf->cullinfo.bounds[1][2]) * 0.5f;
		}
		else
		{
			//ri.Printf(PRINT_ALL, "surface %d has no cubemap\n", i);
			continue;
		}

		surf->cubemapIndex = R_CubemapForPoint(surfOrigin);
		//ri.Printf(PRINT_ALL, "surface %d has cubemap %d\n", i, surf->cubemapIndex);
	}
}


static void R_RenderAllCubemaps(void)
{
	int i, j;
	GLenum cubemapFormat = GL_RGBA8;

	if ( r_hdr->integer )
	{
		cubemapFormat = GL_RGBA16F;
	}

	int vramScaleDiv = 1;

	if (r_lowVram->integer >= 2)
	{// 1GB vram cards...
		vramScaleDiv = 4;
	}
	else if (r_lowVram->integer >= 1)
	{// 2GB vram cards...
		vramScaleDiv = 2;
	}

	byte	*gData = NULL;

#ifdef __EMISSIVE_CUBE_IBL__
	if (r_emissiveCubes->integer)
	{
		if (cubemapFormat == GL_RGBA8)
		{
			gData = (byte *)malloc((r_cubeMapSize->integer / vramScaleDiv) * (r_cubeMapSize->integer / vramScaleDiv) * 4 * sizeof(byte));
			memset(gData, 0, (r_cubeMapSize->integer / vramScaleDiv) * (r_cubeMapSize->integer / vramScaleDiv) * 4 * sizeof(byte));
		}
		else
		{
			gData = (byte *)malloc((r_cubeMapSize->integer / vramScaleDiv) * (r_cubeMapSize->integer / vramScaleDiv) * 8 * sizeof(byte));
			memset(gData, 0, (r_cubeMapSize->integer / vramScaleDiv) * (r_cubeMapSize->integer / vramScaleDiv) * 8 * sizeof(byte));
		}
	}
#endif //__EMISSIVE_CUBE_IBL__

	for (i = 0; i < tr.numCubemaps; i++)
	{
		if (!tr.cubemapEnabled[i]) continue;
		if (tr.cubemapRendered[i]) continue;

		tr.cubemaps[i] = R_CreateImage (va ("*cubeMap%d", i), NULL, r_cubeMapSize->integer / vramScaleDiv, r_cubeMapSize->integer / vramScaleDiv, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat);
		
#ifdef __EMISSIVE_CUBE_IBL__
		if (r_emissiveCubes->integer)
		{// Create black emissive cube, ready to be rendered over...
			tr.emissivemaps[i] = R_CreateImage(va("*emissiveMap%d", i), gData, r_cubeMapSize->integer / vramScaleDiv, r_cubeMapSize->integer / vramScaleDiv, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat);
		}
#endif //__EMISSIVE_CUBE_IBL__
	}

	for (i = 0; i < tr.numCubemaps; i++)
	{
		if (!tr.cubemapEnabled[i]) continue;
		if (tr.cubemapRendered[i]) continue;

		for (j = 0; j < 6; j++)
		{
			RE_ClearScene();
			R_RenderCubemapSide(i, j, qfalse);
			R_IssuePendingRenderCommands();
			R_InitNextFrame();

#ifdef __EMISSIVE_CUBE_IBL__
			if (r_emissiveCubes->integer)
			{
				RE_ClearScene();
				R_RenderEmissiveMapSide(i, j, qfalse);
				R_IssuePendingRenderCommands();
				R_InitNextFrame();
			}
#endif //__EMISSIVE_CUBE_IBL__
		}

		tr.cubemapRendered[i] = true;
	}

#ifdef __EMISSIVE_CUBE_IBL__
	if (r_emissiveCubes->integer)
	{
		free(gData);
	}
#endif //__EMISSIVE_CUBE_IBL__
}
#endif //__REALTIME_CUBEMAP__

extern qboolean CLOSE_LIGHTS_UPDATE;

#ifdef __GENERATED_SKY_CUBES__
void R_RenderSkyCubeSide(int cubemapSide, qboolean subscene, int flag /* VPF_SKYCUBEDAY, VPF_SKYCUBENIGHT*/)
{
	refdef_t refdef;
	viewParms_t	parms;
	float oldColorScale = tr.refdef.colorScale;

	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = 0;
	VectorSet(refdef.vieworg, 0.0, 0.0, 0.0);

	switch (cubemapSide)
	{
	case 0:
		// -X
		VectorSet(refdef.viewaxis[0], -1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, -1);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 1:
		// +X
		VectorSet(refdef.viewaxis[0], 1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, 1);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 2:
		// -Y
		VectorSet(refdef.viewaxis[0], 0, -1, 0);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, -1);
		break;
	case 3:
		// +Y
		VectorSet(refdef.viewaxis[0], 0, 1, 0);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, 1);
		break;
	case 4:
		// -Z
		VectorSet(refdef.viewaxis[0], 0, 0, -1);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	case 5:
		// +Z
		VectorSet(refdef.viewaxis[0], 0, 0, 1);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 1, 0);
		break;
	}

	refdef.fov_x = 90;
	refdef.fov_y = 90;

	refdef.x = 0;
	refdef.y = 0;
	refdef.width = tr.skyCubeMap->width;
	refdef.height = tr.skyCubeMap->height;

	refdef.time = 0;

	if (!subscene)
	{
		RE_BeginScene(&refdef);
	}

	tr.refdef.colorScale = 1.0f;

	Com_Memset(&parms, 0, sizeof(parms));

	parms.viewportX = 0;
	parms.viewportY = 0;
	parms.viewportWidth = tr.skyCubeMap->width;
	parms.viewportHeight = tr.skyCubeMap->height;
	parms.isPortal = qfalse;
	parms.isMirror = qtrue;
	parms.flags = VPF_NOVIEWMODEL | VPF_NOCUBEMAPS | VPF_NOPOSTPROCESS | VPF_CUBEMAP | flag;

	parms.fovX = 90;
	parms.fovY = 90;

	VectorCopy(refdef.vieworg, parms.ori.origin);
	VectorCopy(refdef.viewaxis[0], parms.ori.axis[0]);
	VectorCopy(refdef.viewaxis[1], parms.ori.axis[1]);
	VectorCopy(refdef.viewaxis[2], parms.ori.axis[2]);

	VectorCopy(refdef.vieworg, parms.pvsOrigin);

	// FIXME: sun shadows aren't rendered correctly in cubemaps
	// fix involves changing r_FBufScale to fit smaller cubemap image size, or rendering cubemap to framebuffer first
	if (0) //(r_depthPrepass->value && ((r_forceSun->integer) || tr.sunShadows))
	{
		parms.flags |= VPF_USESUNLIGHT;
	}

	CLOSE_LIGHTS_UPDATE = qtrue;

	parms.targetFbo = tr.renderSkyFbo;
	parms.targetFboLayer = cubemapSide;
	parms.targetFboCubemapIndex = -1;

	R_RenderView(&parms);

	if (subscene)
	{
		tr.refdef.colorScale = oldColorScale;
	}
	else
	{
		RE_EndScene();
	}
}

void R_GenerateSkyCubes(void)
{
	GLenum cubemapFormat = GL_RGBA8;

	if (r_hdr->integer)
	{
		cubemapFormat = GL_RGBA16F;
	}

	int vramScaleDiv = 1;

	if (r_lowVram->integer >= 2)
	{// 1GB vram cards...
		vramScaleDiv = 4;
	}
	else if (r_lowVram->integer >= 1)
	{// 2GB vram cards...
		vramScaleDiv = 2;
	}

	tr.skyCubeMap = R_CreateImage("*skyCubeDay", NULL, 2048 / vramScaleDiv, 2048 / vramScaleDiv, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat);
	tr.skyCubeMapNight = R_CreateImage("*skyCubeNight", NULL, 2048 / vramScaleDiv, 2048 / vramScaleDiv, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP | IMGFLAG_CUBEMAP, cubemapFormat);

	for (int j = 0; j < 6; j++)
	{
		RE_ClearScene();
		R_RenderSkyCubeSide(j, qfalse, VPF_SKYCUBEDAY);
		R_IssuePendingRenderCommands();
		R_InitNextFrame();
	}

	for (int j = 0; j < 6; j++)
	{
		RE_ClearScene();
		R_RenderSkyCubeSide(j, qfalse, VPF_SKYCUBENIGHT);
		R_IssuePendingRenderCommands();
		R_InitNextFrame();
	}
}
#endif //__GENERATED_SKY_CUBES__

/*
=================
R_MergeLeafSurfaces

Merges surfaces that share a common leaf
=================
*/

void R_MergeLeafSurfaces(void)
{
	int i, j, k;
	int numWorldSurfaces;
	int mergedSurfIndex;
	int numMergedSurfaces;
	int numUnmergedSurfaces;
	int numIboIndexes;
	int startTime, endTime;
	msurface_t *mergedSurf;

	startTime = ri->Milliseconds();

	numWorldSurfaces = s_worldData->numWorldSurfaces;

	// use viewcount to keep track of mergers
	for (i = 0; i < numWorldSurfaces; i++)
	{
		s_worldData->surfacesViewCount[i] = -1;
	}

//#define __FULL_WORLD_MERGE__

#ifdef __FULL_WORLD_MERGE__
	// Merge the same shaders of the whole freaking map...
	for (i = 0; i < numWorldSurfaces; i++)
	{
		msurface_t *surf1 = s_worldData->surfaces + i;
		shader_t *shader1 = surf1->shader;

		if (s_worldData->surfacesViewCount[i] != -1)
			continue;

		if (shader1->isSky)
			continue;

		if (shader1->isPortal)
			continue;

		qboolean deforms = qfalse;

		if (ShaderRequiresCPUDeforms(shader1))
			deforms = qtrue;
		
		for (j = 0; j < numWorldSurfaces; j++)
		{
			if (j == i) continue;

			msurface_t *surf2 = s_worldData->surfaces + j;
			shader_t *shader2 = surf2->shader;

			if (s_worldData->surfacesViewCount[j] != -1)
				continue;

			if (shader2->isSky)
				continue;

			if (shader2->isPortal)
				continue;

			if (shader1 != shader2 && deforms && ShaderRequiresCPUDeforms(shader2))
				continue;

			//if (shader1 == shader2)
			{
				s_worldData->surfacesViewCount[j] = i;
			}
		}
	}
#else //!__FULL_WORLD_MERGE__
	// mark matching surfaces
	for (i = 0; i < s_worldData->numnodes - s_worldData->numDecisionNodes; i++)
	{
		mnode_t *leaf = s_worldData->nodes + s_worldData->numDecisionNodes + i;

		for (j = 0; j < leaf->nummarksurfaces; j++)
		{
			int surfNum1 = *(s_worldData->marksurfaces + leaf->firstmarksurface + j);

			if (s_worldData->surfacesViewCount[surfNum1] != -1)
				continue;

			msurface_t *surf1 = s_worldData->surfaces + surfNum1;
			shader_t *shader1 = surf1->shader;

			if (shader1->isSky)
				continue;

			if (shader1->isPortal)
				continue;

			qboolean deforms = qfalse;

			if (ShaderRequiresCPUDeforms(shader1))
				//continue;
				deforms = qtrue;

			s_worldData->surfacesViewCount[surfNum1] = surfNum1;

#ifdef __USE_VBO_AREAS__
			int area1 = surf1->vboArea;
#endif //__USE_VBO_AREAS__

			for (k = j + 1; k < leaf->nummarksurfaces; k++)
			{
				int surfNum2 = *(s_worldData->marksurfaces + leaf->firstmarksurface + k);

				if (s_worldData->surfacesViewCount[surfNum2] != -1)
					continue;

				msurface_t *surf2 = s_worldData->surfaces + surfNum2;
				shader_t *shader2 = surf2->shader;

#ifdef __USE_VBO_AREAS__
				int area2 = surf2->vboArea;
				if (area1 != area2)
					continue;
#endif //__USE_VBO_AREAS__

				if (shader1 != shader2 && deforms && ShaderRequiresCPUDeforms(shader2))
					continue;

				s_worldData->surfacesViewCount[surfNum2] = surfNum1;
			}
		}
	}
#endif //__FULL_WORLD_MERGE__

	// don't add surfaces that don't merge to any others to the merged list
	for (i = 0; i < numWorldSurfaces; i++)
	{
		qboolean merges = qfalse;

		if (s_worldData->surfacesViewCount[i] != i)
			continue;

		for (j = 0; j < numWorldSurfaces; j++)
		{
			if (j == i)
				continue;

			if (s_worldData->surfacesViewCount[j] == i)
			{
				merges = qtrue;
				break;
			}
		}

		if (!merges)
			s_worldData->surfacesViewCount[i] = -1;
	}

	// count merged/unmerged surfaces
	numMergedSurfaces = 0;
	numUnmergedSurfaces = 0;

	for (i = 0; i < numWorldSurfaces; i++)
	{
		if (s_worldData->surfacesViewCount[i] != -1)//== i)
		{
			numMergedSurfaces++;
		}
		else if (s_worldData->surfacesViewCount[i] == -1)
		{
			numUnmergedSurfaces++;
		}
	}

	ri->Printf(PRINT_ALL, "Pre-processed %d surfaces into %d merged, %d unmerged\n", numWorldSurfaces, numMergedSurfaces, numUnmergedSurfaces);

	// Allocate merged surfaces
	s_worldData->mergedSurfaces = (msurface_t *)ri->Hunk_Alloc(sizeof(*s_worldData->mergedSurfaces) * numMergedSurfaces, h_low);
	s_worldData->mergedSurfacesViewCount = (int *)ri->Hunk_Alloc(sizeof(*s_worldData->mergedSurfacesViewCount) * numMergedSurfaces, h_low);
	//s_worldData->mergedSurfacesDlightBits = (int *)ri->Hunk_Alloc(sizeof(*s_worldData->mergedSurfacesDlightBits) * numMergedSurfaces, h_low);
#ifdef __PSHADOWS__
	s_worldData->mergedSurfacesPshadowBits = (int *)ri->Hunk_Alloc(sizeof(*s_worldData->mergedSurfacesPshadowBits) * numMergedSurfaces, h_low);
#endif
	s_worldData->numMergedSurfaces = numMergedSurfaces;
	
	// view surfaces are like mark surfaces, except negative ones represent merged surfaces
	// -1 represents 0, -2 represents 1, and so on
	s_worldData->viewSurfaces = (int *)ri->Hunk_Alloc(sizeof(*s_worldData->viewSurfaces) * s_worldData->nummarksurfaces, h_low);

	// copy view surfaces into mark surfaces
	for (i = 0; i < s_worldData->nummarksurfaces; i++)
	{
		s_worldData->viewSurfaces[i] = s_worldData->marksurfaces[i];
	}

	// need to be synched here
	R_IssuePendingRenderCommands();

	// actually merge surfaces
	numIboIndexes = 0;
	mergedSurfIndex = 0;
	mergedSurf = s_worldData->mergedSurfaces;

	for (i = 0; i < numWorldSurfaces; i++)
	{
		msurface_t *surf1;
		VBO_t *vbo;
		IBO_t *ibo;
		glIndex_t *iboIndexes, *outIboIndexes;

		vec3_t bounds[2];

		int numSurfsToMerge;
		int numIndexes;
		int numVerts;
		int firstIndex;

		srfBspSurface_t *vboSurf;

		if (s_worldData->surfacesViewCount[i] != i)
			continue;

		surf1 = s_worldData->surfaces + i;

		// retrieve vbo
		vbo = ((srfBspSurface_t *)(surf1->data))->vbo;

		// count verts, indexes, and surfaces
		numSurfsToMerge = 0;
		numIndexes = 0;
		numVerts = 0;

		for (j = i; j < numWorldSurfaces; j++)
		{
			msurface_t *surf2;
			srfBspSurface_t *bspSurf;

			if (s_worldData->surfacesViewCount[j] != i)
				continue;

			surf2 = s_worldData->surfaces + j;

			bspSurf = (srfBspSurface_t *) surf2->data;
			numIndexes += bspSurf->numIndexes;
			numVerts += bspSurf->numVerts;
			numSurfsToMerge++;
		}

		if (numVerts == 0 || numIndexes == 0 || numSurfsToMerge < 2)
		{
			continue;
		}

		// create ibo
		ibo = tr.ibos[tr.numIBOs++] = (IBO_t*)ri->Hunk_Alloc(sizeof(*ibo), h_low);
		memset(ibo, 0, sizeof(*ibo));
		numIboIndexes = 0;

		// allocate indexes
		iboIndexes = outIboIndexes = (glIndex_t*)Z_Malloc(numIndexes * sizeof(*outIboIndexes), TAG_BSP);

		// Merge surfaces (indexes) and calculate bounds
		ClearBounds(bounds[0], bounds[1]);
		firstIndex = numIboIndexes;
		for (j = i; j < numWorldSurfaces; j++)
		{
			msurface_t *surf2;
			srfBspSurface_t *bspSurf;

			if (s_worldData->surfacesViewCount[j] != i)
				continue;

			surf2 = s_worldData->surfaces + j;

			AddPointToBounds(surf2->cullinfo.bounds[0], bounds[0], bounds[1]);
			AddPointToBounds(surf2->cullinfo.bounds[1], bounds[0], bounds[1]);

			bspSurf = (srfBspSurface_t *) surf2->data;


			// Mark it...
			surf2->isMerged = qtrue;

			for (k = 0; k < bspSurf->numIndexes; k++)
			{
				*outIboIndexes++ = bspSurf->indexes[k] + bspSurf->firstVert;
				numIboIndexes++;
			}

			break;
		}

		vboSurf = (srfBspSurface_t *)ri->Hunk_Alloc(sizeof(*vboSurf), h_low);
		memset(vboSurf, 0, sizeof(*vboSurf));

		vboSurf->surfaceType = SF_VBO_MESH;

		vboSurf->vbo = vbo;
		vboSurf->ibo = ibo;

		vboSurf->numIndexes = numIndexes;
		vboSurf->numVerts = numVerts;
		vboSurf->firstIndex = firstIndex;

		vboSurf->minIndex = *(iboIndexes + firstIndex);
		vboSurf->maxIndex = *(iboIndexes + firstIndex);

		for (j = 0; j < numIndexes; j++)
		{
			vboSurf->minIndex = MIN(vboSurf->minIndex, *(iboIndexes + firstIndex + j));
			vboSurf->maxIndex = MAX(vboSurf->maxIndex, *(iboIndexes + firstIndex + j));
		}

		VectorCopy(bounds[0], vboSurf->cullBounds[0]);
		VectorCopy(bounds[1], vboSurf->cullBounds[1]);

		VectorCopy(bounds[0], mergedSurf->cullinfo.bounds[0]);
		VectorCopy(bounds[1], mergedSurf->cullinfo.bounds[1]);

		mergedSurf->cullinfo.type = CULLINFO_BOX;
		mergedSurf->data          = (surfaceType_t *)vboSurf;
#ifdef __Q3_FOG__
		mergedSurf->fogIndex      = surf1->fogIndex;
#endif //__Q3_FOG__
		mergedSurf->cubemapIndex  = surf1->cubemapIndex;
		mergedSurf->shader        = surf1->shader;
		
		// finish up the ibo
		qglGenBuffers(1, &ibo->indexesVBO);

		R_BindIBO(ibo);
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, numIboIndexes * sizeof(*iboIndexes), iboIndexes, ibo->iboUsage);
		R_BindNullIBO();

		GL_CheckErrors();

   		Z_Free(iboIndexes);

		// redirect view surfaces to this surf
		for (j = 0; j < numWorldSurfaces; j++)
		{
			if (s_worldData->surfacesViewCount[j] != i)
				continue;

			for (k = 0; k < s_worldData->nummarksurfaces; k++)
			{
				int *mark = s_worldData->marksurfaces + k;
				int *view = s_worldData->viewSurfaces + k;

				if (*mark == j)
					*view = -(mergedSurfIndex + 1);
			}
		}

		mergedSurfIndex++;
		mergedSurf++;
	}

	endTime = ri->Milliseconds();

	ri->Printf(PRINT_ALL, "Processed %d surfaces into %d merged, %d unmerged in %5.2f seconds\n", 
		numWorldSurfaces, numMergedSurfaces, numUnmergedSurfaces, (endTime - startTime) / 1000.0f);

	// reset viewcounts
	for (i = 0; i < numWorldSurfaces; i++)
	{
		s_worldData->surfacesViewCount[i] = -1;
	}
}


static void R_CalcVertexLightDirs( void )
{
#ifndef __CHEAP_VERTS__
	int i, k;

	for(k = 0; k < s_worldData->numsurfaces /* s_worldData->numWorldSurfaces */; k++)
	{
		msurface_t *surface = &s_worldData->surfaces[k];
		srfBspSurface_t *bspSurf = (srfBspSurface_t *) surface->data;

		switch(bspSurf->surfaceType)
		{
			case SF_FACE:
			case SF_GRID:
			case SF_TRIANGLES:

				for(i = 0; i < bspSurf->numVerts; i++)
					R_LightDirForPoint( bspSurf->verts[i].xyz, bspSurf->verts[i].lightdir, bspSurf->verts[i].normal, s_worldData );

				break;

			default:
				break;
		}
	}
#endif //__CHEAP_VERTS__
}

#ifdef __XYC_SURFACE_SPRITES__
struct sprite_t
{
	vec3_t position;
	vec3_t normal;
};

static uint32_t UpdateHash(const char *text, uint32_t hash)
{
	for (int i = 0; text[i]; ++i)
	{
		char letter = tolower((unsigned char)text[i]);
		if (letter == '.') break;				// don't include extension
		if (letter == '\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names

		hash += (uint32_t)(letter)*(i + 119);
	}

	return (hash ^ (hash >> 10) ^ (hash >> 20));
}

static void R_GenerateSurfaceSprites(
	const srfBspSurface_t *bspSurf,
	const shader_t *shader,
	const shaderStage_t *stage,
	srfSprites_t *out)
{
	const surfaceSprite_t *surfaceSprite = stage->ss;
	const textureBundle_t *bundle = &stage->bundle[0];

	const float density = surfaceSprite->density;
	const srfVert_t *verts = bspSurf->verts;
	const glIndex_t *indexes = bspSurf->indexes;

	std::vector<sprite_t> sprites;
	sprites.reserve(10000);
	for (int i = 0, numIndexes = bspSurf->numIndexes; i < numIndexes; i += 3)
	{
		const srfVert_t *v0 = verts + indexes[i + 0];
		const srfVert_t *v1 = verts + indexes[i + 1];
		const srfVert_t *v2 = verts + indexes[i + 2];

		vec3_t p0, p1, p2;
		VectorCopy(v0->xyz, p0);
		VectorCopy(v1->xyz, p1);
		VectorCopy(v2->xyz, p2);

		vec3_t n0, n1, n2;
		VectorCopy(v0->normal, n0);
		VectorCopy(v1->normal, n1);
		VectorCopy(v2->normal, n2);

		const vec2_t p01 = { p1[0] - p0[0], p1[1] - p0[1] };
		const vec2_t p02 = { p2[0] - p0[0], p2[1] - p0[1] };

		const float zarea = std::fabs(p02[0] * p01[1] - p02[1] * p01[0]);
		if (zarea <= 1.0)
		{
			// Triangle's area is too small to consider.
			continue;
		}

		// Generate the positions of the surface sprites.
		//
		// Pick random points inside of each triangle, using barycentric
		// coordinates.
		const float step = density * Q_rsqrt(zarea);
		for (float a = 0.0f; a < 1.0f; a += step)
		{
			for (float b = 0.0f, bend = (1.0f - a); b < bend; b += step)
			{
				float x = flrand(0.0f, 1.0f)*step + a;
				float y = flrand(0.0f, 1.0f)*step + b;
				float z = 1.0f - x - y;

				// Ensure we're inside the triangle bounds.
				if (x > 1.0f) continue;
				if ((x + y) > 1.0f) continue;

				// Calculate position inside triangle.
				// pos = (((p0*x) + p1*y) + p2*z)
				sprite_t sprite = {};
				VectorMA(sprite.position, x, p0, sprite.position);
				VectorMA(sprite.position, y, p1, sprite.position);
				VectorMA(sprite.position, z, p2, sprite.position);

				// x*x + y*y = 1.0
				// => y*y = 1.0 - x*x
				// => y = -/+sqrt(1.0 - x*x)
				float nx = flrand(-1.0f, 1.0f);
				float ny = std::sqrt(1.0f - nx*nx);
				ny *= irand(0, 1) ? -1 : 1;

				VectorSet(sprite.normal, nx, ny, 0.0f);

				// We have 4 copies for each corner of the quad
				sprites.push_back(sprite);
			}
		}
	}

	uint32_t hash = 0;
	for (int i = 0; bundle->image[i]; ++i)
	{
		hash = UpdateHash(bundle->image[i]->imgName, hash);
	}

	uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

	out->surfaceType = SF_SPRITES;
	out->sprite = surfaceSprite;
	out->numSprites = sprites.size();
	out->vbo = R_CreateVBO((byte *)sprites.data(), sizeof(sprite_t) * sprites.size(), VBO_USAGE_STATIC);

	out->ibo = R_CreateIBO((byte *)indices, sizeof(indices), VBO_USAGE_STATIC);

	// FIXME: Need a better way to handle this.
	out->shader = R_CreateShaderFromTextureBundle(va("*ss_%08x\n", hash), bundle, stage->stateBits);
	out->shader->cullType = shader->cullType;
	out->shader->stages[0]->glslShaderGroup = &tr.surfaceSpriteShader;
	//out->shader->stages[0]->alphaTestCmp = stage->alphaTestCmp;

	out->numAttributes = 2;
	out->attributes = (vertexAttribute_t *)ri->Hunk_Alloc(sizeof(vertexAttribute_t) * out->numAttributes, h_low);
	//out->attributes = (vertexAttribute_t *)malloc(sizeof(vertexAttribute_t) * out->numAttributes);
	//out->attributes = (vertexAttribute_t *)ri->Z_Malloc(sizeof(vertexAttribute_t) * out->numAttributes, TAG_GENERAL, qfalse, 4);

	out->attributes[0].vbo = out->vbo;
	out->attributes[0].index = ATTR_INDEX_POSITION;
	out->attributes[0].numComponents = 3;
	out->attributes[0].integerAttribute = qfalse;
	out->attributes[0].type = GL_FLOAT;
	out->attributes[0].normalize = GL_FALSE;
	out->attributes[0].stride = sizeof(sprite_t);
	out->attributes[0].offset = offsetof(sprite_t, position);
	out->attributes[0].stepRate = 1;

	out->attributes[1].vbo = out->vbo;
	out->attributes[1].index = ATTR_INDEX_NORMAL;
	out->attributes[1].numComponents = 3;
	out->attributes[1].integerAttribute = qfalse;
	out->attributes[1].type = GL_FLOAT;
	out->attributes[1].normalize = GL_FALSE;
	out->attributes[1].stride = sizeof(sprite_t);
	out->attributes[1].offset = offsetof(sprite_t, normal);
	out->attributes[1].stepRate = 1;
}

static void R_GenerateSurfaceSprites(const world_t *world)
{
	msurface_t *surfaces = world->surfaces;
	for (int i = 0, numSurfaces = world->numsurfaces; i < numSurfaces; ++i)
	{
		msurface_t *surf = surfaces + i;
		const srfBspSurface_t *bspSurf = (srfBspSurface_t *)surf->data;
		switch (bspSurf->surfaceType)
		{
		case SF_FACE:
		case SF_GRID:
		case SF_TRIANGLES:
		{
			const shader_t *shader = surf->shader;
			if (!shader->numSurfaceSpriteStages)
			{
				continue;
			}

			surf->numSurfaceSprites = shader->numSurfaceSpriteStages;
			surf->surfaceSprites = (srfSprites_t *)ri->Hunk_Alloc(sizeof(srfSprites_t) * surf->numSurfaceSprites, h_low);
			//surf->surfaceSprites = (srfSprites_t *)malloc(sizeof(srfSprites_t) * surf->numSurfaceSprites);
			//surf->surfaceSprites = (srfSprites_t *)ri->Z_Malloc(sizeof(srfSprites_t) * surf->numSurfaceSprites, TAG_GENERAL, qfalse, 4);

			int surfaceSpriteNum = 0;
			for (int j = 0, numStages = shader->numUnfoggedPasses; j < numStages; ++j)
			{
				const shaderStage_t *stage = shader->stages[j];
				if (!stage)
				{
					break;
				}

				if (!stage->ss || stage->ss->type == SURFSPRITE_NONE)
				{
					continue;
				}

				srfSprites_t *sprite = surf->surfaceSprites + surfaceSpriteNum;
				R_GenerateSurfaceSprites(bspSurf, shader, stage, sprite);
				++surfaceSpriteNum;
			}
			break;
		}

		default:
			break;
		}
	}
}
#endif //__XYC_SURFACE_SPRITES__


void StripMaps( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '_'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot-in+1 ? destsize : dot-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}


static void R_FreeExtraWorldData(void)
{
#ifdef __FREE_WORLD_DATA__
	ri->Printf(PRINT_WARNING, "Freeing extra world data.\n");

	for (int k = 0; k < s_worldData->numsurfaces; k++)
	{
		msurface_t *surface = &s_worldData->surfaces[k];
		srfBspSurface_t *bspSurf = (srfBspSurface_t *)surface->data;
		
		if (bspSurf->vbo && bspSurf->ibo)
		{
			switch (bspSurf->surfaceType)
			{
			case SF_FACE:
			case SF_GRID:
			case SF_TRIANGLES:
				free(bspSurf->verts);
				free(bspSurf->indexes);
				bspSurf->verts = NULL;
				bspSurf->indexes = NULL;
				break;
			default:
				break;
			}
		}
	}
#endif //__FREE_WORLD_DATA__
}


void RE_SendLodmodelPointers(int *count, void *data) {
	
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
#include <algorithm>
#include <string>

extern void MAPPING_LoadMapInfo(void);

void RE_LoadWorldMapData( const char *name, bool isMainWorld ) {
	int			i;
	dheader_t	*header;
	union {
		byte *b;
		void *v;
	} buffer;
	byte		*startMarker;

	if (isMainWorld)
	{
		if (tr.worldMapLoaded) {
			ri->Error(ERR_DROP, "ERROR: attempted to redundantly load world map");
		}

		MAP_WATER_LEVEL = 131072.0;

		{// Set currentMapName string...
			string str = name;
			string str2 = "maps/";

			// Find the word in the string
			std::string::size_type pos = str.find(str2);

			// If we find the word we replace it.
			if (pos != std::string::npos)
			{
				str.replace(pos, str2.size(), "");
			}

			COM_StripExtension(str.c_str(), currentMapName, sizeof(currentMapName));
		}

		// Load mapinfo settings...
		DEBUG_StartTimer("MAPPING_LoadMapInfo", qfalse);
		MAPPING_LoadMapInfo();
		DEBUG_EndTimer(qfalse);

		// set default map light scale
		tr.mapLightScale = 1.0f;
		tr.sunShadowScale = 0.5f;

		// set default sun direction to be used if it isn't
		// overridden by a shader
		tr.sunDirection[0] = 0.45f;
		tr.sunDirection[1] = 0.3f;
		tr.sunDirection[2] = 0.9f;

		VectorNormalize(tr.sunDirection);

		// set default autoexposure settings
		tr.autoExposureMinMax[0] = -2.0f;
		tr.autoExposureMinMax[1] = 2.0f;

		// set default tone mapping settings
		tr.toneMinAvgMaxLevel[0] = -8.0f;
		tr.toneMinAvgMaxLevel[1] = -2.0f;
		tr.toneMinAvgMaxLevel[2] = 0.0f;
	}

	// load it
#ifdef __BSP_USE_SHARED_MEMORY__
	buffer.v = NULL;

	fileHandle_t h;
	hSharedMemory *shared = NULL;
	const int iBSPLen = ri->FS_FOpenFileRead(name, &h, qfalse);

	if (h && iBSPLen > 0)
	{
		shared = OpenSharedMemory("SharedBSP", "SharedBSPMutex", iBSPLen);

		buffer.v = (void *)shared->hFileView;

		ri->FS_Read(buffer.v, iBSPLen, h);
		ri->FS_FCloseFile(h);
	}
#else //!__BSP_USE_SHARED_MEMORY__
    uint32_t iBSPLen = ri->FS_ReadFile( name, &buffer.v );
#endif //__BSP_USE_SHARED_MEMORY__

	if (isMainWorld)
	{
		ri->Printf(PRINT_WARNING, "RE_LoadWorldMapData: Loading main world data file %s.\n", name);

		if (!buffer.b) {
			ri->Error(ERR_DROP, "RE_LoadWorldMapData: %s not found", name);
		}
	}
	else
	{
		ri->Printf(PRINT_WARNING, "RE_LoadWorldMapData: Loading extra world data file %s.\n", name);

		if (!buffer.b) {
			ri->Printf(PRINT_WARNING, "RE_LoadWorldMapData: Extra world data file %s not found.\n", name);
			return;
		}
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	s_worldData = (world_t *)ri->Hunk_Alloc(sizeof(world_t), h_low);

	Com_Memset( s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData->name, name, sizeof( s_worldData->name ) );

	Q_strncpyz( s_worldData->baseName, COM_SkipPath( s_worldData->name ), sizeof( s_worldData->name ) );
	COM_StripExtension(s_worldData->baseName, s_worldData->baseName, sizeof(s_worldData->baseName));

	startMarker = (byte *)ri->Hunk_Alloc(0, h_low);
	c_gridVerts = 0;

	header = (dheader_t *)buffer.b;
	fileBase = (byte *)header;

	i = LittleLong (header->version);
	if ( i != BSP_VERSION ) {
		ri->Error (ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", 
			name, i, BSP_VERSION);
	}

	// swap all the lumps
	for (i=0 ; i<sizeof(dheader_t)/4 ; i++) {
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);
	}

	// load into heap
	if (isMainWorld)
	{// I think we only need these for main world...
		DEBUG_StartTimer("R_LoadEntities", qfalse);
		R_LoadEntities(&header->lumps[LUMP_ENTITIES]);
		DEBUG_EndTimer(qfalse);
	}

	DEBUG_StartTimer("R_LoadShaders", qfalse);
	R_LoadShaders( &header->lumps[LUMP_SHADERS] );
	DEBUG_EndTimer(qfalse);
	
	if (isMainWorld)
	{// I think we only need these for main world...
		DEBUG_StartTimer("R_LoadLightmaps", qfalse);
		R_LoadLightmaps(&header->lumps[LUMP_LIGHTMAPS], &header->lumps[LUMP_SURFACES]);
		DEBUG_EndTimer(qfalse);
	}
	
	DEBUG_StartTimer("R_LoadPlanes", qfalse);
	R_LoadPlanes(&header->lumps[LUMP_PLANES], isMainWorld);
	DEBUG_EndTimer(qfalse);

	if (isMainWorld)
	{// I think we only need these for main world...
		DEBUG_StartTimer("R_LoadFogs", qfalse);
		R_LoadFogs(&header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES]);
		DEBUG_EndTimer(qfalse);
	}
	
	//DEBUG_StartTimer("R_LoadSurfaces", qfalse);
	R_LoadSurfaces( &header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES] );
	//DEBUG_EndTimer(qfalse);
	
	DEBUG_StartTimer("R_LoadMarksurfaces", qfalse);
	R_LoadMarksurfaces (&header->lumps[LUMP_LEAFSURFACES], isMainWorld);
	DEBUG_EndTimer(qfalse);
	
	DEBUG_StartTimer("R_LoadNodesAndLeafs", qfalse);
	R_LoadNodesAndLeafs (&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);
	DEBUG_EndTimer(qfalse);
	
	DEBUG_StartTimer("R_LoadSubmodels", qfalse);
	R_LoadSubmodels (&header->lumps[LUMP_MODELS]);
	DEBUG_EndTimer(qfalse);
	
	DEBUG_StartTimer("R_LoadVisibility", qfalse);
	R_LoadVisibility( &header->lumps[LUMP_VISIBILITY], isMainWorld);
	DEBUG_EndTimer(qfalse);
	
	DEBUG_StartTimer("R_LoadLightGrid", qfalse);
	R_LoadLightGrid( &header->lumps[LUMP_LIGHTGRID] );
	DEBUG_EndTimer(qfalse);
	
	DEBUG_StartTimer("R_LoadLightGridArray", qfalse);
	R_LoadLightGridArray( &header->lumps[LUMP_LIGHTARRAY] );
	DEBUG_EndTimer(qfalse);

#ifdef __BSP_USE_SHARED_MEMORY__
	CloseSharedMemory(shared);
#else //!__BSP_USE_SHARED_MEMORY__
	// Free the buffer, ASAP, ffs...
	ri->FS_FreeFile(buffer.v);
	Com_Printf("R_LoadMap: BSP memory image (%.2f MB) was freed.\n", float(iBSPLen) / 1024.0 / 1024.0);
#endif //__BSP_USE_SHARED_MEMORY__
	
#ifdef __XYC_SURFACE_SPRITES__
	R_GenerateSurfaceSprites(&s_worldData);
#endif //__XYC_SURFACE_SPRITES__

	// determine vertex light directions
	DEBUG_StartTimer("R_CalcVertexLightDirs", qfalse);
	R_CalcVertexLightDirs();
	DEBUG_EndTimer(qfalse);

	s_worldData->dataSize = (byte *)ri->Hunk_Alloc(0, h_low) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = s_worldData;

	// Set up water plane and glow postions...
	DEBUG_StartTimer("R_SetupMapGlowsAndWaterPlane", qfalse);
	R_SetupMapGlowsAndWaterPlane(tr.world);
	DEBUG_EndTimer(qfalse);

	// create static VBOS from the world
	DEBUG_StartTimer("R_CreateWorldVBOs", qfalse);
	R_CreateWorldVBOs();
	DEBUG_EndTimer(qfalse);

	if (r_mergeLeafSurfaces->integer)
	{
		DEBUG_StartTimer("R_MergeLeafSurfaces", qfalse);
		R_MergeLeafSurfaces();
		DEBUG_EndTimer(qfalse);
	}

	// make sure the VBO glState entries are safe
	R_BindNullVBO();
	R_BindNullIBO();

	//ri->FS_FreeFile(buffer.v);

	R_FreeExtraWorldData();
}

void RT_LoadWorldMapExtras(void)
{
	DEBUG_StartTimer("R_LoadMapInfo", qfalse);
	R_LoadMapInfo();
	DEBUG_EndTimer(qfalse);

	// Set up water plane and glow postions...
	//DEBUG_StartTimer("R_SetupMapGlowsAndWaterPlane", qfalse);
	//R_SetupMapGlowsAndWaterPlane(tr.worldSolid);

	//if (tr.worldNonSolid)
	//	R_SetupMapGlowsAndWaterPlane(tr.worldNonSolid);
	//DEBUG_EndTimer(qfalse);

#ifndef __REALTIME_CUBEMAP__
	// load cubemaps
	if (r_cubeMapping->integer >= 1 && !r_lowVram->integer)
	{
		DEBUG_StartTimer("R_LoadCubemapEntities", qfalse);
		R_LoadCubemapEntities("misc_cubemap");
		DEBUG_EndTimer(qfalse);

		if (!tr.numCubemaps)
		{
			DEBUG_StartTimer("R_SetupCubemapPoints", qfalse);
			// use deathmatch spawn points as cubemaps
			//R_LoadCubemapEntities("info_player_deathmatch");
			// UQ1: Warzone can do better!
			R_SetupCubemapPoints(); // NOTE: Also sets up water plane and glow postions at the same time... Can skip R_SetupMapGlowsAndWaterPlane()
			DEBUG_EndTimer(qfalse);
		}

#ifndef __PLAYER_BASED_CUBEMAPS__
		if (tr.numCubemaps)
		{
			DEBUG_StartTimer("R_AssignCubemapsToWorldSurfaces", qfalse);
			R_AssignCubemapsToWorldSurfaces();
			DEBUG_EndTimer(qfalse);
		}
#endif //__PLAYER_BASED_CUBEMAPS__
	}
#endif //__REALTIME_CUBEMAP__

#ifndef __REALTIME_CUBEMAP__
	// Render all cubemaps
	if (r_cubeMapping->integer >= 1 && tr.numCubemaps && !r_lowVram->integer)
	{
		DEBUG_StartTimer("R_RenderAllCubemaps", qfalse);
		R_RenderAllCubemaps();
		DEBUG_EndTimer(qfalse);
	}
#endif //__REALTIME_CUBEMAP__

#ifdef __GENERATED_SKY_CUBES__
	extern qboolean PROCEDURAL_SKY_ENABLED;

	if (PROCEDURAL_SKY_ENABLED)
	{// Generate the day/night sky cubemaps...
		R_GenerateSkyCubes();
	}
#endif //__GENERATED_SKY_CUBES__

	/*if (tr.world != NULL)
	{
		for (int i = 0; i < tr.world->numWorldSurfaces; i++)
		{
			msurface_t *surf = tr.world->surfaces + i;

			//if (*surf->data == SF_FACE || *surf->data == SF_TRIANGLES)
			if (surf->isMerged)
			{
				const srfBspSurface_t *bspSurf = (srfBspSurface_t *)surf->data;

				if (bspSurf->isWorldVBO)
				{// Shouldn't need this spam any more...
					ri->Hunk_FreeTempMemory(bspSurf->verts);
					ri->Hunk_FreeTempMemory(bspSurf->indexes);
				}
			}
		}
	}*/
}

void RE_LoadWorldMap(const char *name) {
	tr.worldLoaded = qfalse;
	tr.worldMapLoaded = qfalse;

	if (tr.worldSolid != NULL)
	{
		//free(tr.worldSolid);
		tr.worldSolid = NULL;
	}

	if (tr.worldNonSolid != NULL)
	{
		//free(tr.worldNonSolid);
		tr.worldNonSolid = NULL;
	}

	tr.world = NULL;

	// Load any non-solid extra bsp.
	char strippedName[512] = { 0 };
	char loadName[512] = { 0 };
	COM_StripExtension(name, strippedName, strlen(name));
	sprintf(loadName, "%s_nonsolid.bsp", strippedName);
	RE_LoadWorldMapData(loadName, false);
	tr.worldNonSolid = tr.world;
	tr.world = NULL;

	/*if (tr.worldNonSolid != NULL)
	{
		for (int i = 0; i < tr.worldNonSolid->numWorldSurfaces; i++)
		{
			msurface_t *surf = tr.worldNonSolid->surfaces + i;

			//if (*surf->data == SF_FACE || *surf->data == SF_TRIANGLES)
			if (surf->isMerged)
			{
				const srfBspSurface_t *bspSurf = (srfBspSurface_t *)surf->data;

				//if (bspSurf->isWorldVBO)
				{// Shouldn't need this spam any more...
					ri->Hunk_FreeTempMemory(bspSurf->verts);
					ri->Hunk_FreeTempMemory(bspSurf->indexes);
					ri->Printf(PRINT_WARNING, "Surf %i (%s) freed.\n", i, surf->shader->name);
				}
			}
		}
	}*/

	// Load the base map.
	RE_LoadWorldMapData(name, true);
	tr.worldSolid = tr.world;

	// Set the main pointer to the base map.
	tr.world = tr.worldSolid;

	// Mark the world as completely loaded.
	tr.worldLoaded = qtrue;
	tr.worldMapLoaded = qtrue;

	RT_LoadWorldMapExtras();
}
