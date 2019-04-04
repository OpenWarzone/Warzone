/*
===========================================================================
Copyright (C) 2007-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_vbo.c
#include "tr_local.h"

uint32_t R_VboPackTangent(vec4_t v)
{
	return (((uint32_t)(v[3] * 1.5f   + 2.0f  )) << 30)
		    | (((uint32_t)(v[2] * 511.5f + 512.0f)) << 20)
		    | (((uint32_t)(v[1] * 511.5f + 512.0f)) << 10)
		    | (((uint32_t)(v[0] * 511.5f + 512.0f)));
}

uint32_t R_VboPackNormal(vec3_t v)
{
	return (((uint32_t)(v[2] * 511.5f + 512.0f)) << 20)
		    | (((uint32_t)(v[1] * 511.5f + 512.0f)) << 10)
		    | (((uint32_t)(v[0] * 511.5f + 512.0f)));
}

void R_VboUnpackTangent(vec4_t v, uint32_t b)
{
	v[0] = ((b)       & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[1] = ((b >> 10) & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[2] = ((b >> 20) & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[3] = ((b >> 30) & 0x3)   * 1.0f/1.5f   - 1.0f;
}

void R_VboUnpackNormal(vec3_t v, uint32_t b)
{
	v[0] = ((b)       & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[1] = ((b >> 10) & 0x3ff) * 1.0f/511.5f - 1.0f;
	v[2] = ((b >> 20) & 0x3ff) * 1.0f/511.5f - 1.0f;
}

#ifdef __VBO_PACK_COLOR__
uint32_t R_VboPackColor(vec4_t v)
{
	return (((uint32_t)(v[3] * 1.5f)) << 30)
		| (((uint32_t)(v[2] * 511.5f)) << 20)
		| (((uint32_t)(v[1] * 511.5f)) << 10)
		| (((uint32_t)(v[0] * 511.5f)));
}

void R_VboUnpackColor(vec4_t v, uint32_t b)
{
	v[0] = ((b) & 0x3ff) / 511.5f;
	v[1] = ((b >> 10) & 0x3ff) / 511.5f;
	v[2] = ((b >> 20) & 0x3ff) / 511.5f;
	v[3] = ((b >> 30) & 0x3)   / 1.5f;
}

#endif //__VBO_PACK_COLOR__

#ifdef __HALF_FLOAT__
static unsigned half2float(unsigned short h)
{
	unsigned short h_exp, h_sig;
	unsigned f_sgn, f_exp, f_sig;

	h_exp = (h & 0x7c00u);
	f_sgn = ((unsigned)h & 0x8000u) << 16;
	switch (h_exp) {
	case 0x0000u: /* 0 or subnormal */
		h_sig = (h & 0x03ffu);
		/* Signed zero */
		if (h_sig == 0) {
			return f_sgn;
		}
		/* Subnormal */
		h_sig <<= 1;
		while ((h_sig & 0x0400u) == 0) {
			h_sig <<= 1;
			h_exp++;
		}
		f_exp = ((unsigned)(127 - 15 - h_exp)) << 23;
		f_sig = ((unsigned)(h_sig & 0x03ffu)) << 13;
		return f_sgn + f_exp + f_sig;
	case 0x7c00u: /* inf or NaN */
				  /* All-ones exponent and a copy of the significand */
		return f_sgn + 0x7f800000u + (((unsigned)(h & 0x03ffu)) << 13);
	default: /* normalized */
			 /* Just need to adjust the exponent and shift */
		return f_sgn + (((unsigned)(h & 0x7fffu) + 0x1c000u) << 13);
	}
}

unsigned short float2half(unsigned f)
{
	unsigned f_exp, f_sig;
	unsigned short h_sgn, h_exp, h_sig;

	h_sgn = (unsigned short)((f & 0x80000000u) >> 16);
	f_exp = (f & 0x7f800000u);

	/* Exponent overflow/NaN converts to signed inf/NaN */
	if (f_exp >= 0x47800000u) {
		if (f_exp == 0x7f800000u) {
			/* Inf or NaN */
			f_sig = (f & 0x007fffffu);
			if (f_sig != 0) {
				/* NaN - propagate the flag in the significand... */
				unsigned short ret = (unsigned short)(0x7c00u + (f_sig >> 13));
				/* ...but make sure it stays a NaN */
				if (ret == 0x7c00u) {
					ret++;
				}
				return h_sgn + ret;
			}
			else {
				/* signed inf */
				return (unsigned short)(h_sgn + 0x7c00u);
			}
		}
		else {
			/* overflow to signed inf */
#if NPY_HALF_GENERATE_OVERFLOW
			npy_set_floatstatus_overflow();
#endif
			return (unsigned short)(h_sgn + 0x7c00u);
		}
	}

	/* Exponent underflow converts to a subnormal half or signed zero */
	if (f_exp <= 0x38000000u) {
		/*
		* Signed zeros, subnormal floats, and floats with small
		* exponents all convert to signed zero halfs.
		*/
		if (f_exp < 0x33000000u) {
#if NPY_HALF_GENERATE_UNDERFLOW
			/* If f != 0, it underflowed to 0 */
			if ((f & 0x7fffffff) != 0) {
				npy_set_floatstatus_underflow();
			}
#endif
			return h_sgn;
		}
		/* Make the subnormal significand */
		f_exp >>= 23;
		f_sig = (0x00800000u + (f & 0x007fffffu));
#if NPY_HALF_GENERATE_UNDERFLOW
		/* If it's not exactly represented, it underflowed */
		if ((f_sig&(((unsigned)1 << (126 - f_exp)) - 1)) != 0) {
			npy_set_floatstatus_underflow();
		}
#endif
		f_sig >>= (113 - f_exp);
		/* Handle rounding by adding 1 to the bit beyond half precision */
#if NPY_HALF_ROUND_TIES_TO_EVEN
		/*
		* If the last bit in the half significand is 0 (already even), and
		* the remaining bit pattern is 1000...0, then we do not add one
		* to the bit after the half significand.  In all other cases, we do.
		*/
		if ((f_sig & 0x00003fffu) != 0x00001000u) {
			f_sig += 0x00001000u;
		}
#else
		f_sig += 0x00001000u;
#endif
		h_sig = (unsigned short)(f_sig >> 13);
		/*
		* If the rounding causes a bit to spill into h_exp, it will
		* increment h_exp from zero to one and h_sig will be zero.
		* This is the correct result.
		*/
		return (unsigned short)(h_sgn + h_sig);
	}

	/* Regular case with no overflow or underflow */
	h_exp = (unsigned short)((f_exp - 0x38000000u) >> 13);
	/* Handle rounding by adding 1 to the bit beyond half precision */
	f_sig = (f & 0x007fffffu);
#if NPY_HALF_ROUND_TIES_TO_EVEN
	/*
	* If the last bit in the half significand is 0 (already even), and
	* the remaining bit pattern is 1000...0, then we do not add one
	* to the bit after the half significand.  In all other cases, we do.
	*/
	if ((f_sig & 0x00003fffu) != 0x00001000u) {
		f_sig += 0x00001000u;
	}
#else
	f_sig += 0x00001000u;
#endif
	h_sig = (unsigned short)(f_sig >> 13);
	/*
	* If the rounding causes a bit to spill into h_exp, it will
	* increment h_exp by one and h_sig will be zero.  This is the
	* correct result.  h_exp may increment to 15, at greatest, in
	* which case the result overflows to a signed inf.
	*/
#if NPY_HALF_GENERATE_OVERFLOW
	h_sig += h_exp;
	if (h_sig == 0x7c00u) {
		npy_set_floatstatus_overflow();
	}
	return h_sgn + h_sig;
#else
	return h_sgn + h_exp + h_sig;
#endif
}

void floattofp16(unsigned char *dst, float *src, unsigned nelem)
{
	unsigned i;
	unsigned short *_dst = (unsigned short *)dst;
	unsigned *_src = (unsigned *)src;

	for (i = 0; i < nelem; i++)
		_dst[i] = float2half(_src[i]);
}

void fp16tofloat(float *dst, unsigned char *src, unsigned nelem)
{
	unsigned i;
	unsigned *_dst = (unsigned *)dst;
	unsigned short *_src = (unsigned short *)src;

	for (i = 0; i < nelem; i++)
		_dst[i] = half2float(_src[i]);
}
#endif //__HALF_FLOAT__


static GLenum GetGLBufferUsage ( vboUsage_t usage )
{
	switch (usage)
	{
		case VBO_USAGE_STATIC:
			return GL_STATIC_DRAW;

		case VBO_USAGE_DYNAMIC:
			return GL_DYNAMIC_DRAW;// GL_STREAM_DRAW;

		case VBO_USAGE_STREAM:
			return GL_STREAM_DRAW;

		default:
			ri->Error (ERR_FATAL, "bad vboUsage_t given: %i", usage);
			return GL_INVALID_OPERATION;
	}
}

/*
============
R_CreateVBO
============
*/
VBO_t *R_CreateVBO(byte * vertexes, int vertexesSize, vboUsage_t usage)
{
	VBO_t          *vbo;
	int				glUsage = GetGLBufferUsage (usage);

	if ( tr.numVBOs == MAX_VBOS ) {
		ri->Error( ERR_DROP, "R_CreateVBO: MAX_VBOS hit");
	}

	R_IssuePendingRenderCommands();

	vbo = tr.vbos[tr.numVBOs] = (VBO_t *)ri->Hunk_Alloc(sizeof(*vbo), h_low);
	tr.numVBOs++;

	memset(vbo, 0, sizeof(*vbo));

	vbo->vertexesSize = vertexesSize;
	vbo->vboUsage = glUsage;

	qglGenBuffers(1, &vbo->vertexesVBO);

	qglBindBuffer(GL_ARRAY_BUFFER, vbo->vertexesVBO);
	qglBufferData(GL_ARRAY_BUFFER, vertexesSize, vertexes, glUsage);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);

	glState.currentVBO = NULL;

	GL_CheckErrors();

	return vbo;
}

/*
============
R_CreateIBO
============
*/
IBO_t *R_CreateIBO(byte * indexes, int indexesSize, vboUsage_t usage)
{
	IBO_t          *ibo;
	int				glUsage = GetGLBufferUsage (usage);

	if ( tr.numIBOs == MAX_IBOS ) {
		ri->Error( ERR_DROP, "R_CreateIBO: MAX_IBOS hit");
	}

	R_IssuePendingRenderCommands();

	ibo = tr.ibos[tr.numIBOs] = (IBO_t *)ri->Hunk_Alloc(sizeof(*ibo), h_low);
	tr.numIBOs++;

	ibo->indexesSize = indexesSize;
	ibo->iboUsage = glUsage;

	qglGenBuffers(1, &ibo->indexesVBO);

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO);
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexesSize, indexes, glUsage);

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glState.currentIBO = NULL;

	GL_CheckErrors();

	return ibo;
}

/*
============
R_BindVBO
============
*/
void R_BindVBO(VBO_t * vbo)
{
	if(!vbo)
	{
		//R_BindNullVBO();
		ri->Error(ERR_DROP, "R_BindNullVBO: NULL vbo");
		return;
	}

	if(r_logFile->integer)
	{
		GLimp_LogComment("--- R_BindVBO() ---\n");
	}

	if(glState.currentVBO != vbo)
	{
		glState.currentVBO = vbo;
		glState.vertexAttribPointersSet = 0;

		glState.vertexAttribsInterpolation = 0;
		glState.vertexAttribsOldFrame = 0;
		glState.vertexAttribsNewFrame = 0;
		glState.vertexAttribsTexCoordOffset[0] = 0;
		glState.vertexAttribsTexCoordOffset[1] = 1;
		glState.vertexAnimation = qfalse;
		glState.skeletalAnimation = qfalse;

		qglBindBuffer(GL_ARRAY_BUFFER, vbo->vertexesVBO);

		backEnd.pc.c_vboVertexBuffers++;
	}
}

/*
============
R_BindNullVBO
============
*/
void R_BindNullVBO(void)
{
	GLimp_LogComment("--- R_BindNullVBO ---\n");

	if(glState.currentVBO)
	{
		qglBindBuffer(GL_ARRAY_BUFFER, 0);
		glState.currentVBO = NULL;
	}

	GL_CheckErrors();
}

/*
============
R_BindIBO
============
*/
void R_BindIBO(IBO_t * ibo)
{
	if(!ibo)
	{
		//R_BindNullIBO();
		ri->Error(ERR_DROP, "R_BindIBO: NULL ibo");
		return;
	}

	if(r_logFile->integer)
	{
		GLimp_LogComment("--- R_BindIBO() ---\n");
	}

	if(glState.currentIBO != ibo)
	{
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO);

		glState.currentIBO = ibo;

		backEnd.pc.c_vboIndexBuffers++;
	}
}

/*
============
R_BindNullIBO
============
*/
void R_BindNullIBO(void)
{
	GLimp_LogComment("--- R_BindNullIBO ---\n");

	if(glState.currentIBO)
	{
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glState.currentIBO = NULL;
		glState.vertexAttribPointersSet = 0;
	}
}

/*
============
R_InitVBOs
============
*/
void R_InitVBOs(void)
{
	int             dataSize;
	int             offset;

	ri->Printf(PRINT_ALL, "^5------- ^7R_InitVBOs^5 -------\n");

	tr.numVBOs = 0;
	tr.numIBOs = 0;

	dataSize  = sizeof(tess.xyz[0]);
	dataSize += sizeof(tess.normal[0]);
	dataSize += sizeof(tess.vertexColors[0]);
	dataSize += sizeof(tess.texCoords[0][0]) * 2;
	//dataSize += sizeof(tess.lightdir[0]);
	dataSize *= SHADER_MAX_VERTEXES;

	//tess.vbo = R_CreateVBO(NULL, dataSize, VBO_USAGE_DYNAMIC);
	tess.vbo = R_CreateVBO(NULL, dataSize, VBO_USAGE_STATIC);

	offset = 0;

	tess.vbo->ofs_xyz         = offset; offset += sizeof(tess.xyz[0])              * SHADER_MAX_VERTEXES;
	tess.vbo->ofs_normal      = offset; offset += sizeof(tess.normal[0])           * SHADER_MAX_VERTEXES;
	// these next two are actually interleaved
	tess.vbo->ofs_st          = offset; offset += sizeof(tess.texCoords[0][0]) * 2 * SHADER_MAX_VERTEXES;

	tess.vbo->ofs_vertexcolor = offset; offset += sizeof(tess.vertexColors[0])     * SHADER_MAX_VERTEXES;

	tess.vbo->stride_xyz         = sizeof(tess.xyz[0]);
	tess.vbo->stride_normal      = sizeof(tess.normal[0]);
	tess.vbo->stride_vertexcolor = sizeof(tess.vertexColors[0]);
	tess.vbo->stride_st          = sizeof(tess.texCoords[0][0]) * 2;

	dataSize = sizeof(tess.indexes[0]) * SHADER_MAX_INDEXES;

	//tess.ibo = R_CreateIBO(NULL, dataSize, VBO_USAGE_DYNAMIC);
	tess.ibo = R_CreateIBO(NULL, dataSize, VBO_USAGE_STATIC);

	R_BindNullVBO();
	R_BindNullIBO();

	GL_CheckErrors();
}

/*
============
R_ShutdownVBOs
============
*/
void R_ShutdownVBOs(void)
{
	int             i;
	VBO_t          *vbo;
	IBO_t          *ibo;

	ri->Printf(PRINT_ALL, "^5------- ^7R_ShutdownVBOs^5 -------\n");

	R_BindNullVBO();
	R_BindNullIBO();


	for(i = 0; i < tr.numVBOs; i++)
	{
		vbo = tr.vbos[i];

		if(vbo->vertexesVBO)
		{
			qglDeleteBuffers(1, &vbo->vertexesVBO);
		}

		//ri->Free(vbo);
	}

	for(i = 0; i < tr.numIBOs; i++)
	{
		ibo = tr.ibos[i];

		if(ibo->indexesVBO)
		{
			qglDeleteBuffers(1, &ibo->indexesVBO);
		}

		//ri->Free(ibo);
	}

	tr.numVBOs = 0;
	tr.numIBOs = 0;
}

/*
============
R_VBOList_f
============
*/
void R_VBOList_f(void)
{
	int             i;
	VBO_t          *vbo;
	IBO_t          *ibo;
	int             vertexesSize = 0;
	int             indexesSize = 0;

	ri->Printf (PRINT_ALL, "^7 vertex buffers\n");
	ri->Printf (PRINT_ALL, "^5----------------\n\n");

	ri->Printf(PRINT_ALL, "^7 id   size (MB)\n");
	ri->Printf(PRINT_ALL, "^5---------------\n");

	for(i = 0; i < tr.numVBOs; i++)
	{
		vbo = tr.vbos[i];

		ri->Printf(PRINT_ALL, " %4i %4.2f\n", i, vbo->vertexesSize / (1024.0f * 1024.0f));

		vertexesSize += vbo->vertexesSize;
	}

	ri->Printf(PRINT_ALL, " %d total buffers\n", tr.numVBOs);
	ri->Printf(PRINT_ALL, " %.2f MB in total\n\n", vertexesSize / (1024.0f * 1024.0f));


	ri->Printf (PRINT_ALL, "^7 index buffers\n");
	ri->Printf (PRINT_ALL, "^5---------------\n\n");

	ri->Printf(PRINT_ALL, "^7 id   size (MB)\n");
	ri->Printf(PRINT_ALL, "^5---------------\n");

	for(i = 0; i < tr.numIBOs; i++)
	{
		ibo = tr.ibos[i];

		ri->Printf(PRINT_ALL, " %4i %4.2f\n", i, ibo->indexesSize / (1024.0f * 1024.0f));

		indexesSize += ibo->indexesSize;
	}

	ri->Printf(PRINT_ALL, " %d total buffers\n", tr.numIBOs);
	ri->Printf(PRINT_ALL, " %.2f MB in total\n\n", indexesSize / (1024.0f * 1024.0f));
}


/*
==============
RB_UpdateVBOs

Adapted from Tess_UpdateVBOs from xreal

Update the default VBO to replace the client side vertex arrays
==============
*/
void RB_UpdateVBOs(unsigned int attribBits)
{
	GLimp_LogComment("--- RB_UpdateVBOs ---\n");

	backEnd.pc.c_dynamicVboDraws++;

	// update the default VBO
	if(tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES)
	{
		backEnd.pc.c_dynamicVboTotalSize += tess.numVertexes * (tess.vbo->vertexesSize / SHADER_MAX_VERTEXES);

		R_BindVBO(tess.vbo);

		// orphan old buffer so we don't stall on it
		qglBufferData(GL_ARRAY_BUFFER, tess.vbo->vertexesSize, NULL, tess.vbo->vboUsage);

		if(attribBits & ATTR_BITS)
		{
			if(attribBits & ATTR_POSITION)
			{
				//ri->Printf(PRINT_ALL, "offset %d, size %d\n", tess.vbo->ofs_xyz, tess.numVertexes * sizeof(tess.xyz[0]));
				qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_xyz,         tess.numVertexes * sizeof(tess.xyz[0]),              tess.xyz);
			}

			if(attribBits & ATTR_TEXCOORD0 || attribBits & ATTR_TEXCOORD1)
			{
				// these are interleaved, so we update both if either need it
				//ri->Printf(PRINT_ALL, "offset %d, size %d\n", tess.vbo->ofs_st, tess.numVertexes * sizeof(tess.texCoords[0][0]) * 2);
				qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_st,          tess.numVertexes * sizeof(tess.texCoords[0][0]) * 2, tess.texCoords);
			}

			if(attribBits & ATTR_NORMAL)
			{
				//ri->Printf(PRINT_ALL, "offset %d, size %d\n", tess.vbo->ofs_normal, tess.numVertexes * sizeof(tess.normal[0]));
				qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_normal,      tess.numVertexes * sizeof(tess.normal[0]),           tess.normal);
			}

			if(attribBits & ATTR_COLOR)
			{
				//ri->Printf(PRINT_ALL, "offset %d, size %d\n", tess.vbo->ofs_vertexcolor, tess.numVertexes * sizeof(tess.vertexColors[0]));
				qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_vertexcolor, tess.numVertexes * sizeof(tess.vertexColors[0]),     tess.vertexColors);
			}
		}
		else
		{
			qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_xyz,         tess.numVertexes * sizeof(tess.xyz[0]),              tess.xyz);
			qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_st,          tess.numVertexes * sizeof(tess.texCoords[0][0]) * 2, tess.texCoords);
			qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_normal,      tess.numVertexes * sizeof(tess.normal[0]),           tess.normal);
			qglBufferSubData(GL_ARRAY_BUFFER, tess.vbo->ofs_vertexcolor, tess.numVertexes * sizeof(tess.vertexColors[0]),     tess.vertexColors);
		}
	}

	// update the default IBO
	if(tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES)
	{
		R_BindIBO(tess.ibo);

		// orphan old buffer so we don't stall on it
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, tess.ibo->indexesSize, NULL, tess.ibo->iboUsage);

		qglBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, tess.numIndexes * sizeof(tess.indexes[0]), tess.indexes);
	}
}
