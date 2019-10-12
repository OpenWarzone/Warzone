/*
MD3 and/or BSP to OBJ converter
Written by Leszek Godlewski <github@inequation.org>
The code in this file is placed in the public domain.
*/

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
	#define strcasecmp	_stricmp
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _USE_MATH_DEFINES
	#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <assert.h>

#include "wzModelConverter.h"

// Create an instance of the Importer class
Assimp::Importer assImpImporter;

// Create an instance of the Importer class
Assimp::Exporter assImpExporter;

std::string AssImp_getBasePath(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(0, pos + 1);
}

std::string AssImp_getTextureName(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(pos + 1, std::string::npos);
}

#if defined(WIN32) || defined(WIN64)
bool textcolorprotect = true;
/*doesn't let textcolor be the same as backgroung color if true*/

inline void setcolor(concol textcolor, concol backcolor);
inline void setcolor(int textcolor, int backcolor);
int textcolor();/*returns current text color*/
int backcolor();/*returns current background color*/

#define std_con_out GetStdHandle(STD_OUTPUT_HANDLE)

//-----------------------------------------------------------------------------

int textcolor()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(std_con_out, &csbi);
	int a = csbi.wAttributes;
	return a % 16;
}

int backcolor()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(std_con_out, &csbi);
	int a = csbi.wAttributes;
	return (a / 16) % 16;
}

inline void setcolor(concol textcol, concol backcol)
{
	setcolor(int(textcol), int(backcol));
}

inline void setcolor(int textcol, int backcol)
{
	if (textcolorprotect)
	{
		if ((textcol % 16) == (backcol % 16))textcol++;
	}
	textcol %= 16; backcol %= 16;
	unsigned short wAttributes = ((unsigned)backcol << 4) | (unsigned)textcol;
	SetConsoleTextAttribute(std_con_out, wAttributes);
}

void Q_ColorPrint(char *text)
{
	setcolor(concol_grey, concol_black); // init console color on each new string...

	int len = strlen(text);

	for (int c = 0; c < len; c++)
	{// This could probably be optimized by dumping more than one char at a time... but this will do...
		if (c == len - 1)
		{
			// Final character, just print it...
			printf("%c", text[c]);
			break;
		}
		else {
			// Check for color changes...
			char read[2] = { 0 };
			sprintf(read, "%c%c", text[c], text[c + 1]);

			if (Q_IsColorStringExt(read))
			{// Color swap character...
			 // Set the new console color...
				if (!strcmp(read, S_COLOR_BLACK))
					setcolor(concol_dark_white, concol_black); // never allow true black...
				else if (!strcmp(read, S_COLOR_RED))
					setcolor(concol_dark_red, concol_black); // no equivalent console color for orange, so using red for orange, and dark_red for red
				else if (!strcmp(read, S_COLOR_GREEN))
					setcolor(concol_green, concol_black);
				else if (!strcmp(read, S_COLOR_YELLOW))
					setcolor(concol_yellow, concol_black);
				else if (!strcmp(read, S_COLOR_BLUE))
					setcolor(concol_blue, concol_black);
				else if (!strcmp(read, S_COLOR_CYAN))
					setcolor(concol_cyan, concol_black);
				else if (!strcmp(read, S_COLOR_MAGENTA))
					setcolor(concol_magenta, concol_black);
				else if (!strcmp(read, S_COLOR_WHITE))
					setcolor(concol_white, concol_black);
				else if (!strcmp(read, S_COLOR_ORANGE))
					setcolor(concol_red, concol_black); // no equivalent console color, so using red for orange
				else if (!strcmp(read, S_COLOR_GREY))
					setcolor(concol_grey, concol_black);
				else // Should never happen...
					setcolor(concol_grey, concol_black);

				c++; // skip to after the final color macro character and continue...
				continue;
			}
			else if (!strcmp(read, "\n")) {
				// Newline char here, just dump the 2 chars at once...
				printf("\n");
				c++;
				continue;
			}

			printf("%c", text[c]);
		}
	}

	setcolor(concol_grey, concol_black); // init console color at end of each new string too...
}

int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int retval;

	retval = _vsnprintf(str, size, format, ap);

	if (retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return size;
	}

	return retval;
}

#define	MAXPRINTMSG	4096

void __cdecl Q_Printf(const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean opening_qconsole = qfalse;

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	Q_ColorPrint(msg);
}
#else //!defined(WIN32) || defined(WIN64)
inline void setcolor(concol textcol, concol backcol)
{

}

inline void setcolor(int textcol, int backcol)
{

}

#define Q_Printf printf
#endif //defined(WIN32) || defined(WIN64)

#ifndef BYTE_ORDER
	#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
		#define LITTLE_ENDIAN	1
		#define BIG_ENDIAN		2
	#endif

	// if not provided by system headers, you should #define byte order here
	#define BYTE_ORDER			LITTLE_ENDIAN
#endif // BYTE ORDER

#if BYTE_ORDER == BIG_ENDIAN
	#define little_short(x)		((((x) << 8) & 0xFF00) | (((x) >> 8) & 0x00FF))
	#define little_long(x)		((((x) << 24) & 0xFF000000)			\
									| (((x) << 8) & 0x00FF0000)		\
									| (((x) >> 8) & 0x0000FF00)		\
									| (((x) >> 24) & 0x000000FF))
#else
	#define little_short
	#define little_long
#endif // BIG_ENDIAN

float normalize_vector(const vec3_t in, vec3_t out)
{
	float length, inv_length;

	length = VectorLength(in);
	if (length != 0.0f)
	{
		inv_length = 1.0f / length;
		VectorScale(in, inv_length, out);
	}
	else
	{
		VectorClear(out);
	}

	return length;
}

void cross_product(const vec3_t a, const vec3_t b, vec3_t out)
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

const char *get_bsp_surface_type(mapSurfaceType_t t)
{
	switch(t)
	{
		case MST_BAD:			return "MST_BAD";
		case MST_PLANAR:		return "MST_PLANAR";
		case MST_PATCH:			return "MST_PATCH";
		case MST_TRIANGLE_SOUP:	return "MST_TRIANGLE_SOUP";
		case MST_FLARE:			return "MST_FLARE";
		case MST_FOLIAGE:		return "MST_FOLIAGE";
		default:				return "(unknown)";
	}
}

int convert_bsp_to_obj(const char *in_name, FILE *in, char *out_name)
{
	dheader_t *bsp;
	dmodel_t *model;
	dsurface_t *surf;
	dshader_t *shader;
	drawVert_t *vert;
	int *tri;
	int model_index, surf_index, surf_index_actual, vert_index, vert_index_cum, tri_index;
	int count, vert_count, tri_count;
	unsigned char *buf;
	char *out_name_buf, *p;
	size_t out_name_buf_len;
	char format_buf[20];
	FILE *out = NULL;
	// TODO: Promote these to command-line switches.
	const int split_models = 0;
	const int skip_planar = 0;
	const int skip_tris = 0;
	const int skip_patches = 0;		// WIP
	const int skip_collision = 1;	// TODO

	// load the file contents into a buffer
	fseek(in, 0, SEEK_END);
	count = ftell(in);
	fseek(in, 0, SEEK_SET);
	buf = (unsigned char*)malloc(count);
	if (!buf)
	{
		Q_Printf("^1ERROR^5: Memory allocation failed\n");
		return 11;
	}
	if (fread(buf, 1, count, in) != (size_t)count)
	{
		Q_Printf("^1ERROR^5: Failed to read file (%d bytes) into buffer\n", count);
		return 12;
	}

	// allocate a string buffer large enough to hold the filename extended by
	// the maximum model index
	// max length of model index
	model_index = 1 + (int)floorf(log10f(MAX_MAP_MODELS));
	// max length of surface index
	surf_index = 1 + (int)floorf(log10f(10240));
	out_name_buf_len = strlen(out_name) + 1 + model_index + 1 + surf_index + 4 + 1;
	out_name_buf = (char*)malloc(out_name_buf_len);
	
	if (split_models)
	{
		// MSVC is retarded and disallows just #defining snprintf.
#if _MSC_VER
		_snprintf(format_buf, sizeof(format_buf), "%%s_%%0%dd_%%0%dd.obj\x00", model_index, surf_index);
#else
		snprintf(format_buf, sizeof(format_buf), "%%s_%%0%dd_%%0%dd.obj\x00", model_index, surf_index);
#endif
	}
	else
	{
#if _MSC_VER
		_snprintf(format_buf, sizeof(format_buf), "%%s_%%0%dd.obj\x00", model_index);
#else
		snprintf(format_buf, sizeof(format_buf), "%%s_%%0%dd.obj\x00", model_index);
#endif
	}

	// find and cut the extension off
	if ((p = strrchr(out_name, '.')) != NULL)
		*p = 0;

	// BSP sanity checking
	bsp = (dheader_t *)buf;

	if (little_long(bsp->ident != BSP_IDENT))
	{
		Q_Printf("^1ERROR^5: Not a valid BSP file\n");
		return 13;
	}

	if (little_long(bsp->version != BSP_VERSION))
	{
		Q_Printf("^1ERROR^5: Unsupported BSP version\n");
		return 14;
	}

	// iterate over all the models
	for (model_index = 0, model = (dmodel_t *)(buf
		+ little_long(bsp->lumps[LUMP_MODELS].fileofs));
		model_index < little_long(bsp->lumps[LUMP_MODELS].filelen)
		/ (int)sizeof(dmodel_t);
		++model_index, ++model)
	{

		if (little_long(model->numSurfaces) < 1
			&& (skip_collision || little_long(model->numBrushes) < 1))
		{
			continue;
		}

		// count exportable surfaces
		for (surf_index = 0, count = 0, surf = (dsurface_t *)(buf
			+ little_long(bsp->lumps[LUMP_SURFACES].fileofs)
			+ little_long(model->firstSurface) * sizeof(dsurface_t));
			surf_index < little_long(model->numSurfaces);
			++surf_index, ++surf)
		{
			if ((skip_planar && little_long(surf->surfaceType) == MST_PLANAR)
				|| (skip_tris && little_long(surf->surfaceType) == MST_TRIANGLE_SOUP)
				|| (skip_patches && little_long(surf->surfaceType) == MST_PATCH))
			{
				continue;
			}
			else if (little_long(surf->surfaceType) != MST_PLANAR
				&& little_long(surf->surfaceType) != MST_TRIANGLE_SOUP
				&& little_long(surf->surfaceType) != MST_PATCH)
			{
				static char warned[1 << (sizeof(char) * 8)] = { 0 };
				if (surf->surfaceType > sizeof(warned) || !warned[surf->surfaceType])
				{
					warned[surf->surfaceType] = 1;
					Q_Printf("^3WARNING^5: cannot handle ^3%s^5 surfaces yet, skipping\n",
						get_bsp_surface_type((mapSurfaceType_t)little_long(surf->surfaceType)));
				}
				continue;
			}
			++count;
		}

		// apparently there's nothing to export, get rid of the file
		if (count == 0)
		{
			if (out)
			{
				fclose(out);
			}

			//Q_Printf("\t^1No exportable surfaces, skipping\n");
			continue;
		}

		Q_Printf("^5Processing model #^3%d^5: ^7%d^5 exportable surfaces\n", model_index, count);

		if (!split_models)
		{
			// Start the output.
			snprintf(out_name_buf, out_name_buf_len, format_buf, out_name, model_index);
			out = fopen(out_name_buf, "w");

			// Begin OBJ data.
			fprintf(out, "# generated by wzModelConverter from %s model #%d\n", in_name, model_index);

			vert_index_cum = 0;
		}
		surf_index_actual = 0;

		// iterate over all the BSP drawable surfaces
		for (surf_index = 0, surf = (dsurface_t *)(buf
			+ little_long(bsp->lumps[LUMP_SURFACES].fileofs)
			+ little_long(model->firstSurface) * sizeof(dsurface_t));
			surf_index < little_long(model->numSurfaces);
			++surf_index, ++surf)
		{
			if ((skip_planar && little_long(surf->surfaceType) == MST_PLANAR)
				|| (skip_tris && little_long(surf->surfaceType) == MST_TRIANGLE_SOUP)
				|| (skip_patches && little_long(surf->surfaceType) == MST_PATCH))
			{
				continue;
			}
			else if (little_long(surf->surfaceType) != MST_PLANAR
				&& little_long(surf->surfaceType) != MST_TRIANGLE_SOUP
				&& little_long(surf->surfaceType) != MST_PATCH)
			{
				continue;
			}

			shader = (dshader_t *)(buf
				+ little_long(bsp->lumps[LUMP_SHADERS].fileofs)
				+ little_long(surf->shaderNum) * sizeof(dshader_t));

			// Skip non-drawable surfaces (i.e. collision).
			if (skip_collision && (little_long(shader->surfaceFlags) & SURF_NODRAW))
			{
				continue;
			}

			++surf_index_actual;

			vert = (drawVert_t *)(buf
				+ little_long(bsp->lumps[LUMP_DRAWVERTS].fileofs)
				+ little_long(surf->firstVert) * sizeof(drawVert_t));
			vert_count = little_long(surf->numVerts);

			tri = (int *)(buf
				+ little_long(bsp->lumps[LUMP_DRAWINDEXES].fileofs)
				+ little_long(surf->firstIndex) * sizeof(int));
			tri_count = little_long(surf->numIndexes) / 3;

			// Tesselate patches.
			if (little_long(surf->surfaceType) == MST_PATCH)
			{
				srfGridMesh_t *grid;
				int width_table[MAX_GRID_SIZE], height_table[MAX_GRID_SIZE], lod_width, lod_height;
				int index, row, column;

				const int subdivisions = 20;	// TODO: Variable LOD. ET values are 4, 12 and 20 for high, medium & low, respectively. Promote to commandline switches or to LOD meshes.
				const float lod_error = 0.f;// 1.0f / 100000.f;	// TODO: Variable LOD. Promote to commandline switches.

				// TODO: Remove dependency on this GPL-ed code so that all of this project stays in the public domain.
				// For the time being, call WolfET's subdivision code to get actual tesselated geometry.
				grid = R_SubdividePatchToGrid(little_long(surf->patchWidth), little_long(surf->patchHeight), vert, subdivisions);
				
				width_table[0] = 0;
				lod_width = 1;
				for (index = 1; index < grid->width - 1; ++index)
				{
					if (grid->widthLodError[index] <= lod_error)
					{
						width_table[lod_width++] = index;
					}
				}
				width_table[lod_width++] = grid->width - 1;
				assert(lod_width <= MAX_GRID_SIZE);

				height_table[0] = 0;
				lod_height = 1;
				for (index = 1; index < grid->height - 1; ++index)
				{
					if (grid->heightLodError[index] <= lod_error)
					{
						height_table[lod_height++] = index;
					}
				}
				height_table[lod_height++] = grid->height - 1;
				assert(lod_height <= MAX_GRID_SIZE);

				// We've generated new geometry, so create buffers to hold it for the code later on to read.
				vert_count = lod_height * lod_width;
				vert = (drawVert_t*)malloc(sizeof(*vert) * vert_count);
				tri_count = (lod_height - 1) * (lod_width - 1) * 2;
				tri = (int*)malloc(sizeof(*tri) * tri_count * 3);

				for (row = 0; row < lod_height; ++row)
				{
					for (column = 0; column < lod_width; ++column)
					{
						index = row * lod_width + column;
						assert(index < vert_count);
						vert_index = height_table[row] * grid->width + width_table[column];
						vert[index] = grid->verts[vert_index];

						if (row < lod_height - 1 && column < lod_width - 1)
						{
							index = (row * (lod_width - 1) + column) * 6;
							assert(index + 5 < tri_count * 3);
							vert_index = row * lod_width + column;

							tri[index + 0] = vert_index;
							tri[index + 1] = vert_index + lod_width;
							tri[index + 2] = vert_index + 1;

							tri[index + 3] = vert_index + 1;
							tri[index + 4] = vert_index + lod_width;
							tri[index + 5] = vert_index + lod_width + 1;
						}
					}
				}
				assert(((row - 2) * (lod_width - 1) + (column - 2)) * 6 + 6 == tri_count * 3);
			}

			if (split_models)
			{
				// Start the surface output.
				snprintf(out_name_buf, out_name_buf_len, format_buf, out_name, model_index, surf_index);
				out = fopen(out_name_buf, "w");

				// Begin OBJ data.
				fprintf(out, "# generated by wzModelConverter from %s model #%d surface #%d\n", in_name, model_index, surf_index);

				vert_index_cum = 0;
			}

			Q_Printf("\t^5Processing surface #^3%d^5: type ^7%s^5, ^7%d^5 vertices, "
				"%d indices\n",
				surf_index, get_bsp_surface_type((mapSurfaceType_t)little_long(surf->surfaceType)),
				vert_count, tri_count * 3);

			// start a group
			fprintf(out,
				"\n"
				"# surface %d/%d (#%d, %s)\n"
				"usemtl %s\n"
				"g surf%d\n"
				"o surf%d\n"
				"\n",
				surf_index_actual, count, surf_index, get_bsp_surface_type((mapSurfaceType_t)little_long(surf->surfaceType)), shader->shader, surf_index, surf_index);

			// Output the vertex list.
			for (vert_index = 0; vert_index < vert_count; ++vert_index, ++vert)
			{
				fprintf(out,
					"v %f %f %f\n",
					(float)vert->xyz[0],
					(float)vert->xyz[1],
					(float)vert->xyz[2]);
			}

			fprintf(out, "\n");

			// output the texture vertex list
			vert = (drawVert_t *)(buf
				+ little_long(bsp->lumps[LUMP_DRAWVERTS].fileofs)
				+ little_long(surf->firstVert) * sizeof(drawVert_t));
			for (vert_index = 0; vert_index < vert_count; ++vert_index, ++vert)
			{
				fprintf(out, "vt %f %f\n", vert->st[0], 1.f - vert->st[1]);
			}

			fprintf(out, "\n");

			// output the normals
			vert = (drawVert_t *)(buf
				+ little_long(bsp->lumps[LUMP_DRAWVERTS].fileofs)
				+ little_long(surf->firstVert) * sizeof(drawVert_t));
			for (vert_index = 0; vert_index < vert_count; ++vert_index, ++vert)
			{
				fprintf(out,
					"vn %f %f %f\n",
					vert->normal[0], vert->normal[1], vert->normal[2]);
			}

			fprintf(out,
				"\n"
				"s 1\n");

			// output the triangle list
			for (tri_index = 0; tri_index < tri_count; ++tri_index, tri += 3)
			{
				fprintf(out,
					"f %d/%d/%d %d/%d/%d %d/%d/%d\n",
					1 + little_long(tri[2]) + vert_index_cum,
					1 + little_long(tri[2]) + vert_index_cum,
					1 + little_long(tri[2]) + vert_index_cum,
					1 + little_long(tri[1]) + vert_index_cum,
					1 + little_long(tri[1]) + vert_index_cum,
					1 + little_long(tri[1]) + vert_index_cum,
					1 + little_long(tri[0]) + vert_index_cum,
					1 + little_long(tri[0]) + vert_index_cum,
					1 + little_long(tri[0]) + vert_index_cum);
			}

			// Keep track of cumulative vertex index so that multiple surfaces in the same file may coexist.
			vert_index_cum += vert_count;
			
			if (split_models)
			{
				fclose(out);
			}
		}

		if (!split_models)
		{
			fclose(out);
		}
	}

	free(out_name_buf);

	return 0;
}

#if 0
int convert_md3_to_obj(const char *in_name, FILE *in, FILE *out, int frame)
{
	md3Header_t *md3;
	md3Surface_t *surf;
	md3XyzNormal_t *vert;
	md3Triangle_t *tri;
	md3St_t *st;
	int i, j;
	unsigned char *buf;

	// load the file contents into a buffer
	fseek(in, 0, SEEK_END);
	i = ftell(in);
	fseek(in, 0, SEEK_SET);
	buf = (unsigned char*)malloc(i);
	if (!buf)
	{
		Q_Printf("^1ERROR^5: Memory allocation failed\n");
		return 11;
	}
	if (fread(buf, 1, i, in) != (size_t)i)
	{
		Q_Printf("^1ERROR^5: Failed to read file (%d bytes) into buffer\n", i);
		return 12;
	}

	// MD3 sanity checking
	md3 = (md3Header_t *)buf;

	if (little_long(md3->ident != MD3_IDENT))
	{
		Q_Printf("^1ERROR^5: Not a valid MD3 file\n");
		return 6;
	}

	if (little_long(md3->version > MD3_VERSION))
	{
		Q_Printf("^1ERROR^5: Unsupported MD3 version\n");
		return 7;
	}

	if (little_long(md3->numFrames) < 1)
	{
		Q_Printf("^1ERROR^5: MD3 has no frames\n");
		return 8;
	}

	if (little_long(md3->numFrames) <= frame)
	{
		Q_Printf("^1ERROR^5: Cannot extract frame #%d from a model that has %d frames\n",
			frame, little_long(md3->numFrames));
		return 9;
	}

	if (little_long(md3->numSurfaces) < 1)
	{
		Q_Printf("^1ERROR^5: MD3 has no surfaces\n");
		return 10;
	}

	Q_Printf("^3MD3 stats^5:\n"
		"^3%d^5 surfaces\n"
		"^3%d^5 tags\n"
		"^3%d^5 frames\n",
		little_long(md3->numSurfaces), little_long(md3->numTags),
		little_long(md3->numFrames));

	// begin OBJ data
	fprintf(out,
		"# generated by wzModelConverter from %s\n", in_name);

	// geometry - iterate over all the MD3 surfaces
	for (i = 0, surf = (md3Surface_t *)(buf + little_long(md3->ofsSurfaces));
		i < little_long(md3->numSurfaces);
		++i, surf += little_long(surf->ofsEnd))
	{
		Q_Printf("^5Processing surface #^3%d^5, \"^5%s^5\": ^7%d^5 vertices, ^7%d^5 triangles\n",
			i, surf->name, little_long(surf->numVerts),
			little_long(surf->numTriangles));

		// start a group
		fprintf(out,
			"\n"
			"# surface #%d\n"
			"g %s\n"
			"o %s\n"
			"\n",
				i, surf->name, surf->name);

		// output the vertex list
		vert = (md3XyzNormal_t *)(((unsigned char *)surf)
			+ little_long(surf->ofsXyzNormals)
			+ frame * little_long(surf->numVerts));
		for (j = 0; j < little_long(surf->numVerts); ++j, ++vert)
		{
			fprintf(out,
				"v %f %f %f\n",
					(float)vert->xyz[0] * MD3_XYZ_SCALE,
					(float)vert->xyz[2] * MD3_XYZ_SCALE,
					(float)vert->xyz[1] * MD3_XYZ_SCALE);
		}

		fprintf(out, "\n");

		// output the texture vertex list
		st = (md3St_t *)(((unsigned char *)surf)
			+ little_long(surf->ofsSt));
		for (j = 0; j < little_long(surf->numVerts); ++j, ++st)
		{
			fprintf(out,
				"vt %f %f\n", st->st[0], 1.f - st->st[1]);
		}

		fprintf(out, "\n");

		// output the normals
		vert = (md3XyzNormal_t *)(((unsigned char *)surf)
			+ little_long(surf->ofsXyzNormals)
			+ frame * little_long(surf->numVerts));
		for (j = 0; j < little_long(surf->numVerts); ++j, ++vert)
		{
			float lat, lng;
			lat = (vert->normal >> 8)	/ 255.f * (float)M_PI * 2.f;
			lng = (vert->normal & 0xFF) / 255.f * (float)M_PI * 2.f;
			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )
			// swap Y with Z for Blender
			fprintf(out,
				"vn %f %f %f\n",
					cos(lat) * sin(lng), cos(lng), sin(lat) * sin(lng));
		}

		fprintf(out,
			"\n"
			"s 1\n");

		// output the triangle list
		tri = (md3Triangle_t *)(((unsigned char *)surf)
			+ little_long(surf->ofsTriangles));
		for (j = 0; j < little_long(surf->numTriangles); ++j, ++tri)
		{
			fprintf(out,
				"f %d/%d/%d %d/%d/%d %d/%d/%d\n",
					1 + little_long(tri->indexes[0]),
					1 + little_long(tri->indexes[0]),
					1 + little_long(tri->indexes[0]),
					1 + little_long(tri->indexes[1]),
					1 + little_long(tri->indexes[1]),
					1 + little_long(tri->indexes[1]),
					1 + little_long(tri->indexes[2]),
					1 + little_long(tri->indexes[2]),
					1 + little_long(tri->indexes[2]));
		}
	}

	return 0;
}
#endif

void COM_StripExtension(const char *in, char *out, int destsize)
{
	const char *dot = strrchr(in, '.'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot - in + 1 ? destsize : dot - in + 1);

	if (in == out && destsize > 1)
		out[destsize - 1] = '\0';
	else
		strncpy(out, in, destsize);
}

qboolean StringContainsWord(const char *haystack, const char *needle)
{
	if (!*needle)
	{
		return qfalse;
	}
	for (; *haystack; ++haystack)
	{
		if (toupper(*haystack) == toupper(*needle))
		{
			/*
			* Matched starting char -- loop through remaining chars.
			*/
			const char *h, *n;
			for (h = haystack, n = needle; *h && *n; ++h, ++n)
			{
				if (toupper(*h) != toupper(*n))
				{
					break;
				}
			}
			if (!*n) /* matched all of 'needle' to null termination */
			{
				return qtrue; /* return the start of the match */
			}
		}
	}
	return qfalse;
}

qboolean FS_FileExists(const char *testpath)
{
	struct stat buffer;
	return (qboolean)(stat(testpath, &buffer) == 0);
}

char texName[512] = { 0 }; // not thread safe, but we are not threading anywhere here...

char *FS_TextureFileExists(const char *name)
{
	if (!name || !name[0] || name[0] == '\0' || strlen(name) < 1) return NULL;

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.png", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.tga", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.jpg", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.dds", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.gif", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.bmp", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.ico", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	return NULL;
}

//#define __INTERNAL_OBJ_EXPORTER__

#ifdef __INTERNAL_OBJ_EXPORTER__
int convert_assimp_to_obj(const char *in_name, FILE *in, FILE *out, int frame, const char *ext, const char *out_file, const char *out_ext)
#else //!__INTERNAL_OBJ_EXPORTER__
int convert_assimp_to_obj(const char *in_name, FILE *in, /*FILE *out,*/ int frame, const char *ext, const char *out_file, const char *out_ext)
#endif //__INTERNAL_OBJ_EXPORTER__
{
	size_t bufSize = 0;
	unsigned char *buf;

	// load the file contents into a buffer
	fseek(in, 0, SEEK_END);
	bufSize = ftell(in);
	fseek(in, 0, SEEK_SET);
	buf = (unsigned char*)malloc(bufSize);
	if (!buf)
	{
		Q_Printf("^1ERROR^5: Memory allocation failed\n");
		return 11;
	}
	if (fread(buf, 1, bufSize, in) != bufSize)
	{
		Q_Printf("^1ERROR^5: Failed to read file (%d bytes) into buffer\n", (int)bufSize);
		return 12;
	}

#if 0
#define aiProcessPreset_Settings (
	aiProcessPreset_TargetRealtime_Quality |
	aiProcess_FlipWindingOrder |
    aiProcess_FixInfacingNormals |
    aiProcess_FindInstances |
    aiProcess_ValidateDataStructure |
	aiProcess_OptimizeMeshes |
    0 )
#elif 0
#define aiProcessPreset_Settings (aiProcess_ValidateDataStructure | aiProcess_FindInvalidData | aiProcess_Triangulate | aiProcess_ValidateDataStructure | aiProcess_GenUVCoords | 0)
#elif 0
#define aiProcessPreset_Settings (aiProcess_Triangulate | 0)
#else
	//aiProcess_MakeLeftHanded
	//aiProcess_ConvertToLeftHanded
	//aiProcess_OptimizeGraph              |  \

#define aiProcessPreset_Settings (\
	aiProcess_ImproveCacheLocality	|\
	aiProcess_JoinIdenticalVertices	|\
	aiProcess_GenNormals	|\
	aiProcess_ValidateDataStructure	|\
	aiProcess_FindInvalidData	|\
	aiProcess_FindDegenerates	|\
	aiProcess_GenUVCoords	|\
	aiProcess_TransformUVCoords	|\
	aiProcess_Triangulate	|\
	aiProcess_SortByPType	|\
	aiProcess_FindInstances                  |\
    aiProcess_ValidateDataStructure          |\
	aiProcess_OptimizeMeshes                 |\
	aiProcess_GenSmoothNormals              |  \
    0 )
#endif

	const aiScene* scene = assImpImporter.ReadFileFromMemory(buf, bufSize, aiProcessPreset_Settings, ext);

	if (!scene)
	{
		Q_Printf("^1ERROR^5: An import error occurred: ^7%s.^5\n", assImpImporter.GetErrorString());
		return 6;
	}
	else if (scene->mNumMeshes < 1)
	{
		Q_Printf("An import error occurred: Model has no surfaces\n");
		return 10;
	}
	else if (strlen(assImpImporter.GetErrorString()) > 0)
	{
		Q_Printf("^3WARNING^5: An import warning occurred: ^7%s.^5\n\n", assImpImporter.GetErrorString());
	}
	else
	{
		Q_Printf("^5Model ^3%s^5 was imported without issue.\n\n", in_name);
	}

	std::string basePath = AssImp_getBasePath(in_name);

	Q_Printf("^3Model stats^5:\n");
	Q_Printf(" ^3%d^5 surfaces.\n", little_long(scene->mNumMeshes));

#ifdef __INTERNAL_OBJ_EXPORTER__
	if (!strcmp(out_ext, "obj"))
	{
		char mtlPath[512] = { 0 };
		sprintf(mtlPath, "%s%s", out_file, "mtl");

		// begin OBJ data
		fprintf(out, "# generated by wzModelConverter from %s\n", in_name);

		fprintf(out, "mtllib %s\n\n", mtlPath);

		char surfaceNames[1024][256] = { 0 };
		char surfaceTextures[1024][256] = { 0 };

		// geometry - iterate over all the MD3 surfaces
		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
		{
			aiString	shaderPath;
			qboolean	foundName = qfalse;
			aiMesh		*surf = scene->mMeshes[i];

			scene->mMaterials[surf->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &shaderPath);

			if (surf->mName.length > 0 && FS_FileExists(surf->mName.C_Str()))
			{// Original file+path exists... Use it...
				shaderPath = surf->mName;
				foundName = qtrue;
			}

			if (shaderPath.length > 0 && FS_FileExists(shaderPath.C_Str()))
			{// Original file+path exists... Use it...
				foundName = qtrue;
			}

			if (surf->mName.length > 0 && FS_TextureFileExists(surf->mName.C_Str()))
			{// Original file+path exists... Use it...
				char *nExt = FS_TextureFileExists(surf->mName.C_Str());

				if (nExt && nExt[0])
				{
					shaderPath = nExt;
					foundName = qtrue;
				}
				foundName = qtrue;
			}

			if (shaderPath.length > 0 && FS_TextureFileExists(shaderPath.C_Str()))
			{// Original file+path exists... Use it...
				char *nExt = FS_TextureFileExists(shaderPath.C_Str());

				if (nExt && nExt[0])
				{
					shaderPath = nExt;
					foundName = qtrue;
				}
				foundName = qtrue;
			}

			if (!foundName)
			{// Grab the filename from the original path.
				std::string textureName = AssImp_getTextureName(shaderPath.C_Str());

				//Q_Printf("DEBUG: shaderPath is %s.\n", shaderPath.C_Str());

				// Free the shaderPath so that we can replace it with what we find...
				shaderPath.Clear();

				//Q_Printf("DEBUG: textureName is %s.\n", textureName.c_str());

				if (textureName.length() > 0 && !foundName)
				{// Check if the file exists in the current directory...
					if (FS_FileExists(textureName.c_str()))
					{
						shaderPath = textureName.c_str();
						foundName = qtrue;
					}
					//else
					//{
					//	Q_Printf("DEBUG: %s does not exist.\n", textureName.c_str());
					//}
				}

				if (textureName.length() > 0 && !foundName)
				{// Check if filename.ext exists in the current directory...
					char *nExt = FS_TextureFileExists(textureName.c_str());

					if (nExt && nExt[0])
					{
						shaderPath = nExt;
						foundName = qtrue;
					}
					//else
					//{
					//	Q_Printf("DEBUG: %s does not exist.\n", nExt);
					//}
				}

				if (textureName.length() > 0 && !foundName)
				{// Search for the file... Make final dir/filename.ext
					char out[256] = { 0 };
					COM_StripExtension(textureName.c_str(), out, sizeof(out));
					textureName = out;

					char shaderRealPath[256] = { 0 };
					sprintf(shaderRealPath, "%s%s", basePath.c_str(), textureName.c_str());

					char *nExt = FS_TextureFileExists(shaderRealPath);

					if (nExt && nExt[0])
					{
						shaderPath = nExt;
						foundName = qtrue;
					}
					//else
					//{
					//	Q_Printf("DEBUG: %s does not exist.\n", nExt);
					//}
				}

				if (shaderPath.length == 0)
				{// We failed... Set name do "unknown".
					shaderPath = "unknown";
				}
			}

			surf->mName.Set(shaderPath.C_Str());

			Q_Printf(" ^5+ ^3%s^5. ^7%d^5 faces. ^7%d^5 verts.\n", surf->mName.C_Str(), surf->mNumFaces, surf->mNumVertices);

			char surfaceName[256] = { 0 };
			COM_StripExtension(shaderPath.C_Str(), surfaceName, sizeof(surfaceName));
			surfaceName[strlen(surfaceName)-1] = '\0';

			// start a group
			fprintf(out,
				"\n"
				"# surface #%d\n"
				"usemtl %s\n"
				//"g %s\n"
				//"o %s\n"
				"\n",
				i, surfaceName/*, surfaceName, surfaceName*/);

			strcpy(surfaceNames[i], surfaceName);
			strcpy(surfaceTextures[i], shaderPath.C_Str());

			// output the vertex list
			for (unsigned int j = 0; j < surf->mNumVertices; ++j)
			{
				aiVector3D *vert = &surf->mVertices[j];

				fprintf(out,
					"v %f %f %f\n",
					(float)vert->x,
					(float)vert->z,
					(float)vert->y);
			}

			fprintf(out, "\n");

			// output the texture vertex list
			for (unsigned int j = 0; j < surf->mNumVertices; j++)
			{
				if (surf->mNormals != NULL && surf->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
				{
					fprintf(out, "vt %f %f\n", surf->mTextureCoords[0][j].x, 1.0 - surf->mTextureCoords[0][j].y);
				}
				else
				{
					fprintf(out, "vt %f %f\n", 0.0, 1.0);
				}
			}

			fprintf(out, "\n");

			// output the normals
			for (unsigned int j = 0; j < surf->mNumVertices; ++j)
			{
				aiVector3D *norm = &surf->mNormals[j];

				fprintf(out,
					"vn %f %f %f\n",
					(float)norm->x,
					(float)norm->z,
					(float)norm->y);
			}

			fprintf(out,
				"\n"
				"s 1\n");

			// output the triangle list
			for (unsigned int j = 0; j < surf->mNumFaces; j++)
			{// Assuming triangles for now... AssImp is currently set to convert everything to triangles anyway...
				int tri[3];
				
				tri[0] = surf->mFaces[j].mIndices[0];
				tri[1] = surf->mFaces[j].mIndices[1];
				tri[2] = surf->mFaces[j].mIndices[2];

#if 1
				fprintf(out,
					"f %d/%d/%d %d/%d/%d %d/%d/%d\n",
					1 + (tri[0]),
					1 + (tri[0]),
					1 + (tri[0]),
					1 + (tri[1]),
					1 + (tri[1]),
					1 + (tri[1]),
					1 + (tri[2]),
					1 + (tri[2]),
					1 + (tri[2]));
#else
				fprintf(out,
					"f %d %d %d\n",
					1 + tri[0],
					1 + tri[1],
					1 + tri[2]);
#endif
			}
		}

		/*
		newmtl venus
			Ns 47.0000
			Ni 1.5000
			d 1.0000
			Tr 0.0000
			Tf 1.0000 1.0000 1.0000 
			illum 2
			Ka 0.7569 0.7569 0.7569
			Kd 0.7569 0.7569 0.7569
			Ks 0.3510 0.3510 0.3510
			Ke 0.0000 0.0000 0.0000
			map_Ka models\warzone\naboo\stone.jpg
			map_Kd models\warzone\naboo\stone.jpg
		*/

		// dump a mtl file as well...
		FILE *mtlFile = fopen(mtlPath, "w");
		if (!mtlFile)
		{
			Q_Printf("^1ERROR^5: Failed to open file ^7%s^5\n", mtlPath);
			return 4;
		}

		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
		{
			fprintf(mtlFile,
				"\n"
				"newmtl %s\n"
				//"   Ns 47.0000\n"
				//"   Ni 1.5000\n"
				//"   d 1.0000\n"
				//"   Tr 0.0000\n"
				//"   Tf 1.0000 1.0000 1.0000\n"
				//"   illum 2\n"
				//"   Ka 0.7569 0.7569 0.7569\n"
				//"   Kd 0.7569 0.7569 0.7569\n"
				//"   Ks 0.3510 0.3510 0.3510\n"
				//"   Ke 0.0000 0.0000 0.0000\n"
				"   map_Ka %s\n"
				"   map_Kd %s\n"
				, surfaceNames[i], surfaceTextures[i], surfaceTextures[i]);
		}

		fclose(mtlFile);
	}
	else
#endif
	{
		char surfaceNames[1024][256] = { 0 };
		char surfaceTextures[1024][256] = { 0 };

		char mtlPath[512] = { 0 };
		sprintf(mtlPath, "%s%s", out_file, "mtl");

		// geometry - iterate over all the MD3 surfaces
		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
		{
			aiString	shaderPath;
			qboolean	foundName = qfalse;
			aiMesh		*surf = scene->mMeshes[i];

			scene->mMaterials[surf->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &shaderPath);

			if (surf->mName.length > 0 && FS_FileExists(surf->mName.C_Str()))
			{// Original file+path exists... Use it...
				shaderPath = surf->mName;
				foundName = qtrue;
			}

			if (shaderPath.length > 0 && FS_FileExists(shaderPath.C_Str()))
			{// Original file+path exists... Use it...
				foundName = qtrue;
			}

			if (surf->mName.length > 0 && FS_TextureFileExists(surf->mName.C_Str()))
			{// Original file+path exists... Use it...
				char *nExt = FS_TextureFileExists(surf->mName.C_Str());

				if (nExt && nExt[0])
				{
					shaderPath = nExt;
					foundName = qtrue;
				}
				foundName = qtrue;
			}

			if (shaderPath.length > 0 && FS_TextureFileExists(shaderPath.C_Str()))
			{// Original file+path exists... Use it...
				char *nExt = FS_TextureFileExists(shaderPath.C_Str());

				if (nExt && nExt[0])
				{
					shaderPath = nExt;
					foundName = qtrue;
				}
				foundName = qtrue;
			}

			if (!foundName)
			{// Grab the filename from the original path.
				std::string textureName = AssImp_getTextureName(shaderPath.C_Str());

				//Q_Printf("DEBUG: shaderPath is %s.\n", shaderPath.C_Str());

				// Free the shaderPath so that we can replace it with what we find...
				shaderPath.Clear();

				//Q_Printf("DEBUG: textureName is %s.\n", textureName.c_str());

				if (textureName.length() > 0 && !foundName)
				{// Check if the file exists in the current directory...
					if (FS_FileExists(textureName.c_str()))
					{
						shaderPath = textureName.c_str();
						foundName = qtrue;
					}
					//else
					//{
					//	Q_Printf("DEBUG: %s does not exist.\n", textureName.c_str());
					//}
				}

				if (textureName.length() > 0 && !foundName)
				{// Check if filename.ext exists in the current directory...
					char *nExt = FS_TextureFileExists(textureName.c_str());

					if (nExt && nExt[0])
					{
						shaderPath = nExt;
						foundName = qtrue;
					}
					//else
					//{
					//	Q_Printf("DEBUG: %s does not exist.\n", nExt);
					//}
				}

				if (textureName.length() > 0 && !foundName)
				{// Search for the file... Make final dir/filename.ext
					char out[256] = { 0 };
					COM_StripExtension(textureName.c_str(), out, sizeof(out));
					textureName = out;

					char shaderRealPath[256] = { 0 };
					sprintf(shaderRealPath, "%s%s", basePath.c_str(), textureName.c_str());

					char *nExt = FS_TextureFileExists(shaderRealPath);

					if (nExt && nExt[0])
					{
						shaderPath = nExt;
						foundName = qtrue;
					}
					//else
					//{
					//	Q_Printf("DEBUG: %s does not exist.\n", nExt);
					//}
				}

				if (shaderPath.length == 0)
				{// We failed... Set name do "unknown".
					shaderPath = "unknown";
				}
			}

			char surfaceName[256] = { 0 };
			COM_StripExtension(shaderPath.C_Str(), surfaceName, sizeof(surfaceName));
			surfaceName[strlen(surfaceName) - 1] = '\0';
			strcpy(surfaceNames[i], surfaceName);
			strcpy(surfaceTextures[i], shaderPath.C_Str());

			surf->mName.Set(surfaceName/*shaderPath.C_Str()*/);
			
			// ------
			aiMaterial* pcHelper = new aiMaterial();

			const int iMode = (int)aiShadingMode_Gouraud;
			pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

			// Add a small ambient color value - Quake 3 seems to have one
			aiColor3D clr;
			clr.b = clr.g = clr.r = 0.05f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_AMBIENT);

			clr.b = clr.g = clr.r = 1.0f;
			pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_DIFFUSE);
			pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_SPECULAR);

			// use surface name + skin_name as material name
			aiString name;
			name.Set(surfaceName);
			pcHelper->AddProperty(&name, AI_MATKEY_NAME);

			aiString szString;
			szString.Set(shaderPath.C_Str());
			pcHelper->AddProperty(&szString, AI_MATKEY_TEXTURE_DIFFUSE(0));
			scene->mMaterials[surf->mMaterialIndex] = (aiMaterial*)pcHelper;

			// ------

			Q_Printf(" ^5+ ^3%s^5. ^7%d^5 faces. ^7%d^5 verts.\n", surf->mName.C_Str(), surf->mNumFaces, surf->mNumVertices);
		}

		printf("\n");

		char outPath[512] = { 0 };
		char outExt[512] = { 0 };

		if (!strcmp(out_ext, "collada"))
		{
			sprintf(outExt, "collada");
			sprintf(outPath, "%s%s", out_file, "dae");
		}
		else if (!strcmp(out_ext, "dae"))
		{
			sprintf(outExt, "collada");
			sprintf(outPath, "%s%s", out_file, "dae");
		}
		else if (!strcmp(out_ext, "obj"))
		{
			// dump a mtl file as well...
			FILE *mtlFile = fopen(mtlPath, "w");
			if (!mtlFile)
			{
				Q_Printf("^1ERROR^5: Failed to open file ^7%s^5 for write.\n", mtlPath);
				return 4;
			}

			for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
			{
				fprintf(mtlFile,
					"\n"
					"newmtl %s\n"
					//"   Ns 47.0000\n"
					//"   Ni 1.5000\n"
					//"   d 1.0000\n"
					//"   Tr 0.0000\n"
					//"   Tf 1.0000 1.0000 1.0000\n"
					//"   illum 2\n"
					//"   Ka 0.7569 0.7569 0.7569\n"
					//"   Kd 0.7569 0.7569 0.7569\n"
					//"   Ks 0.3510 0.3510 0.3510\n"
					//"   Ke 0.0000 0.0000 0.0000\n"
					"   map_Ka %s\n"
					"   map_Kd %s\n"
					, surfaceNames[i], surfaceTextures[i], surfaceTextures[i]);
			}

			fclose(mtlFile);

			sprintf(outExt, "%s", out_ext);
			sprintf(outPath, "%s%s", out_file, outExt);
		}
		else
		{
			sprintf(outExt, "%s", out_ext);
			sprintf(outPath, "%s%s", out_file, outExt);
		}

		qboolean flipHandedness = qfalse; // noesis wants this, my other program doesnt... WTF??? Milkshape also doesn't want this, noesis has something wrong it seems...
		assImpExporter.Export(scene, outExt, outPath, flipHandedness ? aiProcess_MakeLeftHanded : 0, 0);

		if (strlen(assImpExporter.GetErrorString()) > 0)
		{// Output any errors...
			Q_Printf("^1ERROR^5: An export error occurred: ^3%s.^5\n", assImpExporter.GetErrorString());
		}
		else
		{
			Q_Printf("^7%s^5 was ^3successfully^5 exported.\n", outPath);
		}
	}

	return 0;
}

#ifdef __NIF_IMPORT_TEST__
//#define __DEBUG_NIF__

#pragma comment(lib, "../../lib/niflib_dll.lib")

#include "nif_include/niflib.h"
#include "nif_include/obj/NiGeometry.h"
#include "nif_include/obj/NiGeometryData.h"
#include "nif_include/obj/NiNode.h"
#include "nif_include/obj/NiTriShape.h"
#include "nif_include/obj/NiTriShapeData.h"
#include "nif_include/Ref.h"
#include "nif_include/RefObject.h"
#include "nif_include/obj/NiExtraData.h"
#include "nif_include/obj/NiTimeController.h"
#include "nif_include/obj/NiTexturingProperty.h"
#include "nif_include/obj/BSShaderTextureSet.h"
#include "nif_include/obj/BSLightingShaderProperty.h"
#include "nif_include/obj/NiSourceTexture.h"
#include "nif_include/obj/NiMaterialProperty.h"
#include "nif_include/obj/NiVertexColorProperty.h"
#include "nif_include/obj/NiTriStripsData.h"
#include "nif_include/obj/NiTriStrips.h"
#include "nif_include/obj/NiBinaryExtraData.h"
#include "nif_include/obj/bhkCollisionObject.h"
#include "nif_include/obj/bhkTransformShape.h"
#include "nif_include/obj/bhkListShape.h"
#include "nif_include/obj/bhkPackedNiTriStripsShape.h"
#include "nif_include/obj/hkPackedNiTriStripsData.h"
#include "nif_include/obj/bhkMoppBvTreeShape.h"
#include "nif_include/obj/bhkRigidBody.h"

void VectorRotateNIF(vec3_t vIn, vec3_t vRotation, vec3_t out)
{
	vec3_t vWork, va;
	int nIndex[3][2];
	int i;

	VectorCopy(vIn, va);
	VectorCopy(va, vWork);
	nIndex[0][0] = 1; nIndex[0][1] = 2;
	nIndex[1][0] = 2; nIndex[1][1] = 0;
	nIndex[2][0] = 0; nIndex[2][1] = 1;
	for (i = 0; i < 3; i++)
	{
		if (vRotation[i] != 0)
		{
			float dAngle = vRotation[i] * PI / 180.0f;
			float c = (vec_t)cos(dAngle);
			float s = (vec_t)sin(dAngle);
			vWork[nIndex[i][0]] = va[nIndex[i][0]] * c - va[nIndex[i][1]] * s;
			vWork[nIndex[i][1]] = va[nIndex[i][0]] * s + va[nIndex[i][1]] * c;
		}
		VectorCopy(vWork, va);
	}
	VectorCopy(vWork, out);
}

void VectorRotateOriginNIF(vec3_t vIn, vec3_t vRotation, vec3_t vOrigin, vec3_t out)
{
	vec3_t vTemp, vTemp2;

	VectorSubtract(vIn, vOrigin, vTemp);
	VectorRotateNIF(vTemp, vRotation, vTemp2);
	VectorAdd(vTemp2, vOrigin, out);
}

void R_NifRotate(float *pos)
{
	vec3_t vRotation;
	vec3_t vec3_origin;

	vec3_origin[0] = 0;
	vec3_origin[1] = 0;
	vec3_origin[2] = 0;

	vRotation[0] = 0;
	vRotation[1] = 0;
	vRotation[2] = 90;

	VectorRotateOriginNIF(pos, vRotation, vec3_origin, pos);
}

int convert_nif_to_obj(const char *in_name, FILE *in, /*FILE *out,*/ int frame, const char *ext, const char *out_file, const char *out_ext)
{
	size_t bufSize = 0;
	unsigned char *buf;

	// load the file contents into a buffer
	fseek(in, 0, SEEK_END);
	bufSize = ftell(in);
	fseek(in, 0, SEEK_SET);
	buf = (unsigned char*)malloc(bufSize);
	if (!buf)
	{
		Q_Printf("^1ERROR^5: Memory allocation failed\n");
		return 11;
	}
	if (fread(buf, 1, bufSize, in) != bufSize)
	{
		Q_Printf("^1ERROR^5: Failed to read file (%d bytes) into buffer\n", (int)bufSize);
		return 12;
	}


#define NIF_MODEL_SCALE 0.5

	std::string basePath = AssImp_getBasePath(in_name);

	char modName[128] = { { 0 } };
	strcpy(modName, in_name);

	try {
		Niflib::NiObjectRef root = Niflib::ReadNifTree(modName);

		if (!root)
		{
			Q_Printf("R_LoadNIF: %s could not load. Error: ReadNifTree failed.\n", modName);
			return 12;
		}

		Niflib::NiNodeRef node = Niflib::DynamicCast<Niflib::NiNode>(root);

		if (node == NULL)
		{
			Q_Printf("R_LoadNIF: %s could not load. Error: DynamicCast<NiNode>(root) failed.\n", modName);
			return 12;
		}

		std::vector<Niflib::Ref<Niflib::NiAVObject>> children = node->GetChildren();

		if (children.size() <= 0)
		{
			Q_Printf("R_LoadNIF: %s could not load. Error: children.size() is none.\n", modName);
			return 12;
		}

		// Calculate the bounds/radius info... And count geometry type surfs...
		int numGeomSurfs = 0;
		vec3_t bounds[2];
		ClearBounds(bounds[0], bounds[1]);

		int z = 0;

		for (std::vector<Niflib::Ref<Niflib::NiAVObject>>::iterator it = children.begin(); it != children.end(); ++it, z++)
		{
			Niflib::NiAVObject *NiAVO = (*it);

			if (!NiAVO->IsDerivedType(Niflib::NiGeometry::TYPE))
			{
				//Q_Printf("child %i (%s) is not NiGeometry.\n", z, NiAVO->GetName().length() ? NiAVO->GetName().c_str() : "UNKNOWN");
				continue;
			}

			Niflib::NiGeometry *NiGEOM = Niflib::DynamicCast<Niflib::NiGeometry>(NiAVO);

			bool isNiTriShape = false;
			bool isNiTriShapeData = false;
			bool isNiTriStrips = false;
			bool isNiTriStripsData = false;

			if (!NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
			{
				//Q_Printf("child %i (%s) is not NiTriShape or NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
				continue;
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE))
			{
				isNiTriShape = true;
				//Q_Printf("child %i (%s) is NiTriShape.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE))
			{
				isNiTriShapeData = true;
				//Q_Printf("child %i (%s) is NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE))
			{
				isNiTriStrips = true;
				//Q_Printf("child %i (%s) is NiTriStrips.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			if (NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
			{
				isNiTriStripsData = true;
				//Q_Printf("child %i (%s) is NiTriStripsData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
			}

			bool hasTEXTURE = false;

			for (short index(0); index < 2; ++index)
			{
				Niflib::BSLightingShaderProperty *shader = Niflib::DynamicCast<Niflib::BSLightingShaderProperty>(NiGEOM->GetBSProperty(index));

				if (shader != NULL)
				{
					std::vector<std::string> textures = shader->GetTextureSet()->GetTextures();

					if (!textures.size()) continue;

					for (std::vector<std::string>::iterator ti = textures.begin(); ti != textures.end(); ++ti)
					{
						std::string texture = (*ti);

						if (texture.length())
						{
							//Q_Printf("child %i (%s) index %i has %s texture.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN", index, texture.c_str());
							hasTEXTURE = true;
							break;
						}
					}
				}
			}

			if (!hasTEXTURE)
			{
				continue;
			}

			Niflib::Ref<Niflib::NiGeometryData> niGeomData = Niflib::DynamicCast<Niflib::NiGeometryData>(NiGEOM->GetData());


			std::vector<Niflib::Vector3> verts;
			vector<Niflib::Triangle> triangles;
			std::vector<int> idxs = niGeomData->GetVertexIndices();

			if (isNiTriShape || isNiTriShapeData)
			{
				Niflib::NiTriShapeDataRef data = Niflib::DynamicCast<Niflib::NiTriShapeData>(niGeomData);
				verts = data->GetVertices();
				triangles = data->GetTriangles();
			}
			else if (isNiTriStrips || isNiTriStripsData)
			{
				Niflib::NiTriStripsDataRef data = Niflib::DynamicCast<Niflib::NiTriStripsData>(niGeomData);
				verts = data->GetVertices();
				triangles = data->GetTriangles();
			}
			else
			{
				verts = niGeomData->GetVertices();
				std::vector<int> idxs = niGeomData->GetVertexIndices();
			}

			uint32_t numVERTS = 0;

			for (std::vector<Niflib::Vector3>::iterator vi = verts.begin(); vi != verts.end(); vi++)
			{
				Niflib::Vector3 vt = (*vi);

				vec3_t vert;
				vert[0] = vt.x * NIF_MODEL_SCALE;
				vert[1] = vt.y * NIF_MODEL_SCALE;
				vert[2] = vt.z * NIF_MODEL_SCALE;

				R_NifRotate(vert);

				AddPointToBounds(vert, bounds[0], bounds[1]);
				numVERTS++;
			}

			uint32_t numIDX = 0;

			if (isNiTriShape || isNiTriShapeData || isNiTriStrips || isNiTriStripsData)
			{
				for (std::vector<Niflib::Triangle>::iterator tr = triangles.begin(); tr != triangles.end(); tr++)
				{
					numIDX++;
				}
			}
			else
			{
				for (std::vector<int>::iterator id = idxs.begin(); id != idxs.end(); id++)
				{
					numIDX++;
				}
			}

			//Q_Printf("child %i (%s) has %u verts and %u indexes.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN", numVERTS, numIDX);

			if (numVERTS > 0 && numIDX > 0)
			{
				numGeomSurfs++;
			}
		}

		if (numGeomSurfs <= 0)
		{
#ifdef __DEBUG_NIF__
			ri->Printf(PRINT_ALL, "R_LoadNIF: Model %s has no geometry.\n", modName);
#endif //__DEBUG_NIF__
			return 12;
		}

		//
		//
		//
		//
		//

		Q_Printf("^3Model stats^5:\n");
		Q_Printf(" ^3%d^5 surfaces.\n", little_long(numGeomSurfs));


		{
			char surfaceNames[1024][256] = { 0 };
			char surfaceTextures[1024][256] = { 0 };

			char mtlPath[512] = { 0 };
			sprintf(mtlPath, "%s%s", out_file, "mtl");

			// geometry - iterate over all the MD3 surfaces
			for (std::vector<Niflib::Ref<Niflib::NiAVObject>>::iterator it = children.begin(); it != children.end(); ++it)
			{
				Niflib::NiAVObject *NiAVO = (*it);

				if (!NiAVO->IsDerivedType(Niflib::NiGeometry::TYPE))
				{
					continue;
				}

				Niflib::NiGeometry *NiGEOM = Niflib::DynamicCast<Niflib::NiGeometry>(NiAVO);

				bool isNiTriShape = false;
				bool isNiTriShapeData = false;
				bool isNiTriStrips = false;
				bool isNiTriStripsData = false;
				bool hasNormals = false;
				bool hasUVs = false;

				if (!NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE) && !NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
				{
					//ri->Printf(PRINT_WARNING, "child %i (%s) is not NiTriShape or NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
					continue;
				}

				if (NiGEOM->IsDerivedType(Niflib::NiTriShape::TYPE))
				{
					isNiTriShape = true;
					//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriShape.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
				}

				if (NiGEOM->IsDerivedType(Niflib::NiTriShapeData::TYPE))
				{
					isNiTriShapeData = true;
					//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriShapeData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
				}

				if (NiGEOM->IsDerivedType(Niflib::NiTriStrips::TYPE))
				{
					isNiTriStrips = true;
					//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriStrips.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
				}

				if (NiGEOM->IsDerivedType(Niflib::NiTriStripsData::TYPE))
				{
					isNiTriStripsData = true;
					//ri->Printf(PRINT_WARNING, "child %i (%s) is NiTriStripsData.\n", z, NiGEOM->GetName().length() ? NiGEOM->GetName().c_str() : "UNKNOWN");
				}

				Niflib::Ref<Niflib::NiGeometryData> niGeomData = Niflib::DynamicCast<Niflib::NiGeometryData>(NiGEOM->GetData());




			}

			printf("\n");

			char outPath[512] = { 0 };
			char outExt[512] = { 0 };

			if (!strcmp(out_ext, "collada"))
			{
				sprintf(outExt, "collada");
				sprintf(outPath, "%s%s", out_file, "dae");
			}
			else if (!strcmp(out_ext, "dae"))
			{
				sprintf(outExt, "collada");
				sprintf(outPath, "%s%s", out_file, "dae");
			}
			else if (!strcmp(out_ext, "obj"))
			{
				// dump a mtl file as well...
				FILE *mtlFile = fopen(mtlPath, "w");
				if (!mtlFile)
				{
					Q_Printf("^1ERROR^5: Failed to open file ^7%s^5 for write.\n", mtlPath);
					return 4;
				}

				for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
				{
					fprintf(mtlFile,
						"\n"
						"newmtl %s\n"
						//"   Ns 47.0000\n"
						//"   Ni 1.5000\n"
						//"   d 1.0000\n"
						//"   Tr 0.0000\n"
						//"   Tf 1.0000 1.0000 1.0000\n"
						//"   illum 2\n"
						//"   Ka 0.7569 0.7569 0.7569\n"
						//"   Kd 0.7569 0.7569 0.7569\n"
						//"   Ks 0.3510 0.3510 0.3510\n"
						//"   Ke 0.0000 0.0000 0.0000\n"
						"   map_Ka %s\n"
						"   map_Kd %s\n"
						, surfaceNames[i], surfaceTextures[i], surfaceTextures[i]);
				}

				fclose(mtlFile);

				sprintf(outExt, "%s", out_ext);
				sprintf(outPath, "%s%s", out_file, outExt);
			}
			else
			{
				sprintf(outExt, "%s", out_ext);
				sprintf(outPath, "%s%s", out_file, outExt);
			}

			qboolean flipHandedness = qfalse; // noesis wants this, my other program doesnt... WTF??? Milkshape also doesn't want this, noesis has something wrong it seems...
			assImpExporter.Export(scene, outExt, outPath, flipHandedness ? aiProcess_MakeLeftHanded : 0, 0);

			if (strlen(assImpExporter.GetErrorString()) > 0)
			{// Output any errors...
				Q_Printf("^1ERROR^5: An export error occurred: ^3%s.^5\n", assImpExporter.GetErrorString());
			}
			else
			{
				Q_Printf("^7%s^5 was ^3successfully^5 exported.\n", outPath);
			}
		}
	}

	return 0;
}
#endif //__NIF_IMPORT_TEST__

int main(int argc, char *argv[])
{
	FILE *infile;
	char *in_ext, *out_ext;
	int retcode;

	if (argc < 3)
	{
		//Q_Printf("^3Usage^5: ^7%s ^5<^7infile^5> <^7outfile^5> [frame number]\n", argv[0]);
		Q_Printf("^3Usage^5: ^7%s ^5<^7infile^5> <^7outfile^5>\n", argv[0]);

		Q_Printf("\n");
		Q_Printf("^3Available map input formats:\n");
		Q_Printf("   ^3bsp^5 - Quake 3 based BSP formats\n");

		Q_Printf("\n");
		Q_Printf("^3Available model input formats:\n");
		for (unsigned int i = 0; i < assImpImporter.GetImporterCount(); i++)
		{
			const aiImporterDesc *desc = assImpImporter.GetImporterInfo(i);
			Q_Printf("   ^3%s^5 - %s\n", desc->mFileExtensions, desc->mName);

			if (!strcmp(desc->mFileExtensions, "collada"))
			{
				Q_Printf("   ^3%s^5 - %s\n", "dae", desc->mName);
			}
		}

		Q_Printf("\n");
		Q_Printf("^3Available map output formats:\n");
		Q_Printf("   ^3obj^5 - Wavefront OBJ format\n");

		Q_Printf("\n");
		Q_Printf("^3Available model output formats:\n");

		for (unsigned int i = 0; i < assImpExporter.GetExportFormatCount(); i++)
		{
			// Hiding these ones, since we only have file ext to select from...
			if (i == 4) continue; // Wavefront OBJ format without material file
			if (i == 6) continue; // Stereolithography (binary)
			if (i == 8) continue; // Stanford Polygon Library (binary)

			Q_Printf("   ^3%s^5 - %s\n", assImpExporter.GetExportFormatDescription(i)->fileExtension, assImpExporter.GetExportFormatDescription(i)->description);
		}

		return 1;
	}

	in_ext = strrchr(argv[1], '.');
	if (in_ext == NULL)
	{
		Q_Printf("^1ERROR^5: File ^7%s^5 appears to have no extension\n", argv[1]);
		return 2;
	}
	++in_ext;

	out_ext = strrchr(argv[2], '.');
	if (out_ext != NULL)
	{
		++out_ext;
	}

	if (!(infile = fopen(argv[1], "rb")))
	{
		Q_Printf("^1ERROR^5: Failed to open file ^7%s^5\n", argv[1]);
		return 3;
	}

	/*if (!strcasecmp(in_ext, "md3"))
	{
		FILE *outfile = fopen(argv[2], "w");
		if (!outfile)
		{
			Q_Printf("Failed to open file %s\n", argv[2]);
			fclose(infile);
			return 4;
		}
		retcode = convert_md3_to_obj(argv[1], infile, outfile, argc > 3 ? atoi(argv[3]) : 0);
		fclose(outfile);
	}
	else*/ if (!strcasecmp(in_ext, "bsp"))
	{
		retcode = convert_bsp_to_obj(argv[1], infile, argv[2]);
	}
	else if (strlen(in_ext))
	{
#ifdef __INTERNAL_OBJ_EXPORTER__
		FILE *outfile = fopen(argv[2], "w");
		
		if (!outfile)
		{
			Q_Printf("^1ERROR^5: Failed to open file ^7%s^5\n", argv[2]);
			fclose(infile);
			return 4;
		}

		char out_file[512] = { 0 };
		COM_StripExtension(argv[2], out_file, sizeof(out_file));
		retcode = convert_assimp_to_obj(argv[1], infile, outfile, argc > 3 ? atoi(argv[3]) : 0, in_ext, out_file, out_ext);
		fclose(outfile);
#else //!__INTERNAL_OBJ_EXPORTER__
		char out_file[512] = { 0 };
		COM_StripExtension(argv[2], out_file, sizeof(out_file));
		retcode = convert_assimp_to_obj(argv[1], infile, argc > 3 ? atoi(argv[3]) : 0, in_ext, out_file, out_ext);
#endif //__INTERNAL_OBJ_EXPORTER__
	}
	else
	{
		Q_Printf("^1ERROR^5: Unknown extension ^3%s^5 in file ^7%s^5\n", in_ext, argv[1]);
		retcode = 5;
	}

	fclose(infile);

	return retcode;
}
