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
#include "glext.h"

backEndData_t	*backEndData;
backEndState_t	backEnd;

extern qboolean WATER_ENABLED;

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

void RB_SwapFBOs ( FBO_t **currentFbo, FBO_t **currentOutFbo)
{
	/*if (*currentFbo == tr.renderFbo)
	{// Skip initial blit by starting from render FBO and changing it to generic after 1st use...
		*currentFbo = tr.genericFbo3;
	}*/

	FBO_t *temp = *currentFbo;
	*currentFbo = *currentOutFbo;
	*currentOutFbo = temp;

	FBO_Bind(*currentFbo);
}

/*
** GL_Bind
*/
void GL_Bind( image_t *image ) {
	int texnum;

	if ( !image ) {
		ri->Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage ) {		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum ) 
	{
		if ( image ) {
			image->frameUsed = tr.frameCount;
		}
		glState.currenttextures[glState.currenttmu] = texnum;
		if (image && image->flags & IMGFLAG_CUBEMAP)
			qglBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
		else
			qglBindTexture( GL_TEXTURE_2D, texnum );
	}
}

/*
** GL_SelectTexture
*/
void GL_SelectTexture( int unit )
{
	if ( glState.currenttmu == unit )
	{
		return;
	}

	if (!(unit >= 0 && unit <= 31))
		ri->Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );

	qglActiveTextureARB( GL_TEXTURE0_ARB + unit );

	glState.currenttmu = unit;
}

/*
** GL_BindToTMU
*/
void GL_BindToTMU( image_t *image, int tmu )
{
	int		texnum;
	int     oldtmu = glState.currenttmu;

	if (!image)
		texnum = 0;
	else
		texnum = image->texnum;

	if ( glState.currenttextures[tmu] != texnum ) {
		GL_SelectTexture( tmu );
		if (image)
			image->frameUsed = tr.frameCount;
		glState.currenttextures[tmu] = texnum;

		if (image && (image->flags & IMGFLAG_CUBEMAP))
			qglBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
		else
			qglBindTexture( GL_TEXTURE_2D, texnum );
		GL_SelectTexture( oldtmu );
	}
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	glState.faceCulling = cullType;

	if ( backEnd.projection2D )
	{
		return;
	}

	if ( cullType == CT_TWO_SIDED ) 
	{
		qglDisable( GL_CULL_FACE );
	} 
	else 
	{
		qboolean cullFront;
		qglEnable( GL_CULL_FACE );

		cullFront = (qboolean)(cullType == CT_FRONT_SIDED);
		if ( backEnd.viewParms.isMirror )
		{
			cullFront = (qboolean)(!cullFront);
		}

		if ( backEnd.currentEntity && backEnd.currentEntity->mirrored )
		{
			cullFront = (qboolean)(!cullFront);
		}

		qglCullFace( cullFront ? GL_FRONT : GL_BACK );
	}
}

/*
** GL_TexEnv
*/
void GL_TexEnv( int env )
{
	if ( env == glState.texEnv[glState.currenttmu] )
	{
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch ( env )
	{
	case GL_MODULATE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	case GL_REPLACE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		break;
	case GL_DECAL:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		break;
	case GL_ADD:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
		break;
	default:
		ri->Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env );
		break;
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( uint32_t stateBits )
{
	uint32_t diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_BITS )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else if ( stateBits & GLS_DEPTHFUNC_GREATER)
		{
			qglDepthFunc( GL_GREATER );
		}
		else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
		{
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				ri->Error( ERR_DROP, "GL_State: invalid src blend state bits" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				ri->Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
				break;
			}

			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		}
		else
		{
			qglDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			qglDepthMask( GL_TRUE );
		}
		else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			qglDisable( GL_DEPTH_TEST );
		}
		else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS )
	{
		switch ( stateBits & GLS_ATEST_BITS )
		{
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_GT_0:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GREATER, 0.0f );
			break;
		case GLS_ATEST_LT_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5f );
			break;
		case GLS_ATEST_GE_128:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5f );
			break;
		case GLS_ATEST_GE_192:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.75f );
			break;
		default:
			assert( 0 );
			break;
		}
	}

	glState.glStateBits = stateBits;
}

/*
matrix_t *InitTranslationTransform(float x, float y, float z)
{
	matrix_t m[16];
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = x;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = y;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = z;
    m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f;
	return m;
}


matrix_t *InitCameraTransform(const vec3_t Target, const vec3_t Up)
{
	matrix_t m[16];
    vec3_t N;
	VectorCopy(Target, N);
    VectorNormalize(N);
    vec3_t U;
	VectorCopy(Up, U);
    VectorNormalize(U);
	CrossProduct(U, N, U);
    vec3_t V;
	CrossProduct(N, U, V);

    m[0][0] = U[0];   m[0][1] = U[1];   m[0][2] = U[3];   m[0][3] = 0.0f;
    m[1][0] = V[0];   m[1][1] = V[1];   m[1][2] = V[3];   m[1][3] = 0.0f;
    m[2][0] = N[0];   m[2][1] = N[1];   m[2][2] = N[3];   m[2][3] = 0.0f;
    m[3][0] = 0.0f;  m[3][1] = 0.0f;  m[3][2] = 0.0f;  m[3][3] = 1.0f;
	return m;
}
*/

void myInverseMatrix (float m[16], float src[16])
{

  float m11, m12, m13, m14, m21, m22, m23, m24; // minors of src matrix
  float m31, m32, m33, m34, m41, m42, m43, m44; // minors of src matrix
  float determinent;

  if (m != NULL && src != NULL) {
    // Finding minors of src matrix
    m11 = src[5] * (src[10] * src[15] - src[11] * src[14])
        - src[9] * (src[6] * src[15] - src[7] * src[14])
        + src[13] * (src[6] * src[11] - src[7] * src[10]);
    m12 = src[1] * (src[10] * src[15] - src[11] * src[14])
        - src[9] * (src[2] * src[15] - src[3] * src[14])
        + src[13] * (src[2] * src[11] - src[3] * src[10]);
    m13 = src[1] * (src[6] * src[15] - src[7] * src[14])
        - src[5] * (src[2] * src[15] - src[3] * src[14])
        + src[13] * (src[2] * src[7] - src[3] * src[6]);
    m14 = src[1] * (src[6] * src[11] - src[7] * src[10])
        - src[5] * (src[2] * src[11] - src[3] * src[10])
        + src[9] * (src[2] * src[7] - src[3] * src[6]);
    m21 = src[4] * (src[10] * src[15] - src[11] * src[14])
        - src[8] * (src[6] * src[15] - src[7] * src[14])
        + src[12] * (src[6] * src[11] - src[7] * src[10]);
    m22 = src[0] * (src[10] * src[15] - src[11] * src[14])
        - src[8] * (src[2] * src[15] - src[3] * src[14])
        + src[12] * (src[2] * src[11] - src[3] * src[10]);
    m23 = src[0] * (src[6] * src[15] - src[7] * src[14])
        - src[4] * (src[2] * src[15] - src[3] * src[14])
        + src[12] * (src[2] * src[7] - src[3] * src[6]);
    m24 = src[0] * (src[6] * src[11] - src[7] * src[10])
        - src[4] * (src[2] * src[11] - src[3] * src[10])
        + src[8] * (src[2] * src[7] - src[3] * src[6]);
    m31 = src[4] * (src[9] * src[15] - src[11] * src[13])
        - src[8] * (src[5] * src[15] - src[7] * src[13])
        + src[12] * (src[5] * src[11] - src[7] * src[9]);
    m32 = src[0] * (src[9] * src[15] - src[11] * src[13])
        - src[8] * (src[1] * src[15] - src[3] * src[13])
        + src[12] * (src[1] * src[11] - src[3] * src[9]);
    m33 = src[0] * (src[5] * src[15] - src[7] * src[13])
        - src[4] * (src[1] * src[15] - src[3] * src[13])
        + src[12] * (src[1] * src[7] - src[3] * src[5]);
    m34 = src[0] * (src[5] * src[11] - src[7] * src[9])
        - src[4] * (src[1] * src[11] - src[3] * src[9])
        + src[8] * (src[1] * src[7] - src[3] * src[5]);
    m41 = src[4] * (src[9] * src[14] - src[10] * src[13])
        - src[8] * (src[5] * src[14] - src[6] * src[13])
        + src[12] * (src[5] * src[10] - src[6] * src[9]);
    m42 = src[0] * (src[9] * src[14] - src[10] * src[13])
        - src[8] * (src[1] * src[14] - src[2] * src[13])
        + src[12] * (src[1] * src[10] - src[2] * src[9]);
    m43 = src[0] * (src[5] * src[14] - src[6] * src[13])
        - src[4] * (src[1] * src[14] - src[2] * src[13])
        + src[12] * (src[1] * src[6] - src[2] * src[5]);
    m44 = src[0] * (src[5] * src[10] - src[6] * src[9])
        - src[4] * (src[1] * src[10] - src[2] * src[9])
        + src[8] * (src[1] * src[6] - src[2] * src[5]);

    // calculate the determinent
    determinent = src[0] * m11 - src[4] * m12 + src[8] * m13 - src[12] * m14;
    if (determinent != 0) {
      m[0] = m11 / determinent;
      m[1] = -m12 / determinent;
      m[2] = m13 / determinent;
      m[3] = -m14 / determinent;
      m[4] = -m21 / determinent;
      m[5] = m22 / determinent;
      m[6] = -m23 / determinent;
      m[7] = m24 / determinent;
      m[8] = m31 / determinent;
      m[9] = -m32 / determinent;
      m[10] = m33 / determinent;
      m[11] = -m34 / determinent;
      m[12] = -m41 / determinent;
      m[13] = m42 / determinent;
      m[14] = -m43 / determinent;
      m[15] = m44 / determinent;
    } else {
      fprintf (stderr, "myInverseMatrix() error: no inverse matrix "
          "exists.\n");
    }
  } else {
    fprintf (stderr, "myInverseMatrix() error: matrix pointer is null.\n");
  }
}

void SWAP(float *A, float *B, float *c)
{
	c = A;
	A = B;
	B = c;
}

void RealTransposeMatrix(const float m[16], float invOut[16])
{
	float t;

	memcpy(invOut, m, sizeof(float)*16);

	SWAP(&invOut[1],&invOut[4],&t);
	SWAP(&invOut[2],&invOut[8],&t);
	SWAP(&invOut[6],&invOut[9],&t);
	SWAP(&invOut[3],&invOut[12],&t);
	SWAP(&invOut[7],&invOut[13],&t);
	SWAP(&invOut[11],&invOut[14],&t);
}

bool RealInvertMatrix(const float m[16], float invOut[16])
{
    float inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

void GL_SetProjectionMatrix(matrix_t matrix)
{
	//if (memcmp(matrix, glState.projection, sizeof(float) * 16) == 0) return;

	Matrix16Copy(matrix, glState.projection);
	Matrix16Multiply(glState.projection, glState.modelview, glState.modelviewProjection);
}

void GL_SetModelviewMatrix(matrix_t matrix)
{
	//if (memcmp(matrix, glState.modelview, sizeof(float) * 16) == 0) return;

	Matrix16Copy(matrix, glState.modelview);
	Matrix16Multiply(glState.projection, glState.modelview, glState.modelviewProjection);
}


/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}


void SetViewportAndScissor( void ) {
	GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
extern qboolean SUN_VISIBLE;
extern float MAP_WATER_LEVEL;

void RB_ClearRenderBuffers ( void )
{
	//if (tr.renderFbo && backEnd.viewParms.targetFbo == tr.renderFbo)
	if (!backEnd.depthFill 
		&& !(backEnd.viewParms.flags & VPF_SHADOWPASS) 
		&& !(backEnd.viewParms.flags & VPF_SHADOWMAP)
		&& !(tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		&& !(tr.renderSkyFbo != NULL && backEnd.viewParms.targetFbo == tr.renderSkyFbo))
	{
		if (r_glslWater->integer && WATER_ENABLED && MAP_WATER_LEVEL < 131000.0 && MAP_WATER_LEVEL > -131000.0)
		{
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.waterFbo);
			qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			qglClear(GL_COLOR_BUFFER_BIT);

			FBO_Bind(oldFbo);
			qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		}

		{// Also clear the transparancy map...
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.transparancyFbo);
			qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			qglClear(GL_COLOR_BUFFER_BIT);

			FBO_Bind(oldFbo);
			qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		}

		{// Also clear the render pshadow map...
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.renderPshadowsFbo);
			qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			qglClear(GL_COLOR_BUFFER_BIT);

			FBO_Bind(oldFbo);
			qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		}
	}
}

extern void TR_AxisToAngles(const vec3_t axis[3], vec3_t angles);

void RB_BeginDrawingView (void) {
	int clearBits = 0;

	// sync with gl if needed
#ifdef __USE_QGL_FINISH__
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}
#else if __USE_QGL_FLUSH__
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFlush();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}
#endif //__USE_QGL_FINISH__

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	// FIXME: HUGE HACK: render to the screen fbo if we've already postprocessed the frame and aren't drawing more world
	// drawing more world check is in case of double renders, such as skyportals
	if (backEnd.viewParms.targetFbo == NULL)
	{
		if (!tr.renderFbo || (backEnd.framePostProcessed && (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)))
		{
			FBO_Bind(NULL);
		}
		else
		{
			FBO_Bind(tr.renderFbo);
		}
	}
	else
	{
		FBO_Bind(backEnd.viewParms.targetFbo);

		// FIXME: hack for cubemap testing
		if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		{
			//qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, backEnd.viewParms.targetFbo->colorImage[0]->texnum, 0);
#ifndef __REALTIME_CUBEMAP__
			if (backEnd.viewParms.flags & VPF_EMISSIVEMAP)
				qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, tr.emissivemaps[backEnd.viewParms.targetFboCubemapIndex]->texnum, 0);
			else
				qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex]->texnum, 0);
#else //__REALTIME_CUBEMAP__
			qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, tr.realtimeCubemap->texnum, 0);
#endif //__REALTIME_CUBEMAP__
		}
		else if (tr.renderSkyFbo != NULL && backEnd.viewParms.targetFbo == tr.renderSkyFbo)
		{
			if (backEnd.viewParms.flags & VPF_SKYCUBENIGHT)
				qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, tr.skyCubeMapNight->texnum, 0);
			else
				qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + backEnd.viewParms.targetFboLayer, tr.skyCubeMap->texnum, 0);
		}
	}

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );
	
	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_clear->integer )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
	}

	if ( r_measureOverdraw->integer )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
#ifdef _DEBUG
		qglClearColor( 0.8f, 0.7f, 0.4f, 1.0f );	// FIXME: get color of sky
#else
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
#endif
	}

	RB_ClearRenderBuffers();

	// clear to white for shadow maps
	if (backEnd.viewParms.flags & VPF_SHADOWMAP 
		&& ((tr.sunShadowFbo[0] && backEnd.viewParms.targetFbo == tr.sunShadowFbo[0])
			|| (tr.sunShadowFbo[1] && backEnd.viewParms.targetFbo == tr.sunShadowFbo[1])
			|| (tr.sunShadowFbo[2] && backEnd.viewParms.targetFbo == tr.sunShadowFbo[2])
			|| (tr.sunShadowFbo[3] && backEnd.viewParms.targetFbo == tr.sunShadowFbo[3])
			|| (tr.sunShadowFbo[4] && backEnd.viewParms.targetFbo == tr.sunShadowFbo[4])))
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
	}
	
	// clear to black for cube maps
	if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	if (tr.renderSkyFbo != NULL && backEnd.viewParms.targetFbo == tr.renderSkyFbo)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	}

	qglClear( clearBits );

	if (backEnd.viewParms.targetFbo == NULL)
	{
		// Clear the glow target
		float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
		qglClearBufferfv (GL_COLOR, 1, black);
	}

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;		// force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;
	SUN_VISIBLE = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
#if 0
		float	plane[4];
		double	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.ori.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.ori.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.ori.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.ori.origin) - plane[3];
#endif
		GL_SetModelviewMatrix( s_flipMatrix );
	}

	// UQ1: Set refdef.viewangles... Hopefully this place is good enough to do it?!?!?!?
	//TR_AxisToAngles(tr.refdef.viewaxis, tr.refdef.viewangles);
}

#define	MAC_EVENT_PUMP_MSEC		5


/*
==================
RB_RenderDrawSurfList
==================
*/

#ifdef __PLAYER_BASED_CUBEMAPS__
int			currentPlayerCubemap = 0;
vec4_t		currentPlayerCubemapVec = { 0.0 };
float		currentPlayerCubemapDistance = 0;
#endif //__PLAYER_BASED_CUBEMAPS__

#if 0
int sSortFunc (const void * a, const void * b)
{
	drawSurf_t		*drawSurf1 = (drawSurf_t *)a;
	drawSurf_t		*drawSurf2 = (drawSurf_t *)b;

	uint64_t sort1 = drawSurf1->sort;
	uint64_t sort2 = drawSurf2->sort;

	shader_t *shader1 = tr.sortedShaders[(sort1 >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
	uint64_t entityNum1 = (sort1 >> QSORT_REFENTITYNUM_SHIFT) & REFENTITYNUM_MASK;
	uint64_t postRender1 = (sort1 >> QSORT_POSTRENDER_SHIFT) & 1;

	shader_t *shader2 = tr.sortedShaders[(sort2 >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
	uint64_t entityNum2 = (sort2 >> QSORT_REFENTITYNUM_SHIFT) & REFENTITYNUM_MASK;
	uint64_t postRender2 = (sort2 >> QSORT_POSTRENDER_SHIFT) & 1;

	sort1 = shader1->index + (65536 + entityNum1) + (131072 + postRender1);
	sort2 = shader2->index + (65536 + entityNum2) + (131072 + postRender2);

	if (sort1 < sort2)
		return -1;

	if (sort1 > sort2)
		return 1;

	return 0;
}
#endif

extern qboolean DISABLE_LIFTS_AND_PORTALS_MERGE;

#ifdef __REALTIME_SURFACE_SORTING__
/*
=================
RealtimeSurfaceCompare
compare function for qsort()
=================
*/
static int RealtimeSurfaceCompare(const void *a, const void *b)
{
	drawSurf_t	 *dsa, *dsb;

	dsa = (drawSurf_t *)a;
	dsb = (drawSurf_t *)b;
	
	shader_t *shadera = tr.sortedShaders[(dsa->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
	shader_t *shaderb = tr.sortedShaders[(dsb->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
	
#ifdef __REALTIME_DISTANCE_SORTING__
	switch (*dsa->surface)
	{
	case SF_FACE:
	case SF_GRID:
	case SF_VBO_MESH:
	case SF_TRIANGLES:
	//case SF_POLY:
		{
			srfBspSurface_t   *aa, *bb;

			aa = (srfBspSurface_t *)dsa->surface;
			bb = (srfBspSurface_t *)dsa->surface;

			float dista = Distance(aa->cullOrigin, tr.refdef.vieworg);
			float distb = Distance(bb->cullOrigin, tr.refdef.vieworg);

			if (r_testvalue2->integer)
			{
				dista = Distance(aa->cullOrigin, backEnd.refdef.vieworg);
				distb = Distance(bb->cullOrigin, backEnd.refdef.vieworg);
			}

			// Sort closest to furthest... Hopefully allow for faster pixel depth culling...
			if (dista < distb)
				return -1;

			else if (dista > distb)
				return 1;
		}
		break;
	default:
		{
			int64_t entityNumA = (dsa->sort >> QSORT_REFENTITYNUM_SHIFT) & REFENTITYNUM_MASK;
			int64_t entityNumB = (dsb->sort >> QSORT_REFENTITYNUM_SHIFT) & REFENTITYNUM_MASK;

			if (entityNumA < entityNumB)
				return -1;

			else if (entityNumA > entityNumB)
				return 1;

			trRefEntity_t *entA = &backEnd.refdef.entities[entityNumA];
			trRefEntity_t *entB = &backEnd.refdef.entities[entityNumB];

			float dista = Distance(entA->e.origin, tr.refdef.vieworg);
			float distb = Distance(entB->e.origin, tr.refdef.vieworg);

			if (r_testvalue2->integer)
			{
				dista = Distance(entA->e.origin, backEnd.refdef.vieworg);
				distb = Distance(entB->e.origin, backEnd.refdef.vieworg);
			}

			// Sort closest to furthest... Hopefully allow for faster pixel depth culling...
			if (dista < distb)
				return -1;

			else if (dista > distb)
				return 1;
		}
		break;
	}
#endif //__REALTIME_DISTANCE_SORTING__


#ifdef __REALTIME_FX_SORTING__
	if (qboolean(shadera == tr.sunShader) < qboolean(shaderb == tr.sunShader))
		return -1;
	else if (qboolean(shadera == tr.sunShader) > qboolean(shaderb == tr.sunShader))
		return 1;

	if (qboolean(shadera == tr.moonShader) < qboolean(shaderb == tr.moonShader))
		return -1;
	else if (qboolean(shadera == tr.moonShader) > qboolean(shaderb == tr.moonShader))
		return 1;

	if (qboolean(shadera->materialType == MATERIAL_FIRE) < qboolean(shaderb->materialType == MATERIAL_FIRE))
		return -1;
	else if (qboolean(shadera->materialType == MATERIAL_FIRE) > qboolean(shaderb->materialType == MATERIAL_FIRE))
		return 1;

	if (qboolean(shadera->materialType == MATERIAL_SMOKE) < qboolean(shaderb->materialType == MATERIAL_SMOKE))
		return -1;
	else if (qboolean(shadera->materialType == MATERIAL_SMOKE) > qboolean(shaderb->materialType == MATERIAL_SMOKE))
		return 1;

	if (qboolean(shadera->materialType == MATERIAL_MAGIC_PARTICLES) < qboolean(shaderb->materialType == MATERIAL_MAGIC_PARTICLES))
		return -1;
	else if (qboolean(shadera->materialType == MATERIAL_MAGIC_PARTICLES) > qboolean(shaderb->materialType == MATERIAL_MAGIC_PARTICLES))
		return 1;

	if (qboolean(shadera->materialType == MATERIAL_MAGIC_PARTICLES_TREE) < qboolean(shaderb->materialType == MATERIAL_MAGIC_PARTICLES_TREE))
		return -1;
	else if (qboolean(shadera->materialType == MATERIAL_MAGIC_PARTICLES_TREE) > qboolean(shaderb->materialType == MATERIAL_MAGIC_PARTICLES_TREE))
		return 1;

	if (qboolean(shadera->materialType == MATERIAL_FIREFLIES) < qboolean(shaderb->materialType == MATERIAL_FIREFLIES))
		return -1;
	else if (qboolean(shadera->materialType == MATERIAL_FIREFLIES) > qboolean(shaderb->materialType == MATERIAL_FIREFLIES))
		return 1;

	if (qboolean(shadera->materialType == MATERIAL_PORTAL) < qboolean(shaderb->materialType == MATERIAL_PORTAL))
		return -1;
	else if (qboolean(shadera->materialType == MATERIAL_PORTAL) > qboolean(shaderb->materialType == MATERIAL_PORTAL))
		return 1;
#endif //__REALTIME_FX_SORTING__


#ifdef __REALTIME_WATER_SORTING__
	// Set up a value to skip stage checks later... does this even run per frame? i shoud actually check probably...
	if (shadera->isWater < shaderb->isWater)
		return -1;

	else if (shadera->isWater > shaderb->isWater)
		return 1;
#endif //__REALTIME_WATER_SORTING__


#ifdef __REALTIME_ALPHA_SORTING__
	// Set up a value to skip stage checks later... does this even run per frame? i shoud actually check probably...
	if (shadera->hasAlphaTestBits == 0)
	{
		for (int stage = 0; stage <= shadera->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = shadera->stages[stage];

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
				shadera->hasAlphaTestBits = max(shadera->hasAlphaTestBits, 1);
			}

			if (pStage->alphaGen)
			{
				shadera->hasAlphaTestBits = max(shadera->hasAlphaTestBits, 2);
			}

			if (shadera->hasAlpha)
			{
				shadera->hasAlphaTestBits = max(shadera->hasAlphaTestBits, 3);
			}
		}

		if (shadera->hasAlphaTestBits == 0)
		{
			shadera->hasAlphaTestBits = -1;
		}
	}

	if (shaderb->hasAlphaTestBits == 0)
	{
		for (int stage = 0; stage <= shaderb->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = shaderb->stages[stage];

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
				shaderb->hasAlphaTestBits = max(shaderb->hasAlphaTestBits, 1);
				break;
			}

			if (pStage->alphaGen)
			{
				shaderb->hasAlphaTestBits = max(shaderb->hasAlphaTestBits, 2);
			}

			if (shaderb->hasAlpha)
			{
				shaderb->hasAlphaTestBits = max(shaderb->hasAlphaTestBits, 3);
			}
		}

		if (shaderb->hasAlphaTestBits == 0)
		{
			shaderb->hasAlphaTestBits = -1;
		}
	}

	// Non-alpha stage shaders should draw first... Hopefully allow for faster pixel depth culling...
	if (shadera->hasAlphaTestBits < shaderb->hasAlphaTestBits)
		return -1;

	else if (shadera->hasAlphaTestBits > shaderb->hasAlphaTestBits)
		return 1;
#endif //__REALTIME_ALPHA_SORTING__

#ifdef __REALTIME_SPLATMAP_SORTING__
	// Set up a value to skip stage checks later... does this even run per frame? i shoud actually check probably...
	if (shadera->hasSplatMaps == 0)
	{
		for (int stage = 0; stage <= shadera->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = shadera->stages[stage];

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
				shadera->hasSplatMaps = 1;
			}
			else
			{
				shadera->hasSplatMaps = -1;
			}
		}
	}

	if (shaderb->hasSplatMaps == 0)
	{
		for (int stage = 0; stage <= shaderb->maxStage && stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = shaderb->stages[stage];

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
				shaderb->hasSplatMaps = 1;
			}
			else
			{
				shaderb->hasSplatMaps = -1;
			}
		}

		// Non-alpha stage shaders should draw first... Hopefully allow for faster pixel depth culling...
		if (shadera->hasSplatMaps < shaderb->hasSplatMaps)
			return -1;

		else if (shadera->hasSplatMaps > shaderb->hasSplatMaps)
			return 1;
	}
#endif //__REALTIME_SPLATMAP_SORTING__

#ifdef __REALTIME_SORTINDEX_SORTING__
	if (r_testvalue1->integer)
	{
		// sort by shader
		if (shadera->sortedIndex < shaderb->sortedIndex)
			return -1;

		else if (shadera->sortedIndex > shaderb->sortedIndex)
			return 1;
	}
#endif //__REALTIME_SORTINDEX_SORTING__

#ifdef __REALTIME_NUMSTAGES_SORTING__
	// Fewer stage shaders should draw first... Hopefully allow for faster pixel depth culling...
	if (shadera->numStages < shaderb->numStages)
		return -1;

	else if (shadera->numStages > shaderb->numStages)
		return 1;
#endif //__REALTIME_NUMSTAGES_SORTING__



#ifdef __REALTIME_GLOW_SORTING__
	// Non-glow stage shaders should draw first... Hopefully allow for faster pixel depth culling...
	if (shadera->hasGlow < shaderb->hasGlow)
		return -1;

	else if (shadera->hasGlow > shaderb->hasGlow)
		return 1;
#endif //__REALTIME_GLOW_SORTING__


#ifdef __REALTIME_TESS_SORTING__
	if (shadera->tesselation)
	{// Check tesselation settings... Draw faster tesselation surfs first...
		if (shadera->tesselationLevel < shaderb->tesselationLevel)
			return -1;

		else if (shadera->tesselationLevel > shaderb->tesselationLevel)
			return 1;

		if (shadera->tesselationAlpha < shaderb->tesselationAlpha)
			return -1;

		else if (shadera->tesselationAlpha > shaderb->tesselationAlpha)
			return 1;
	}
#endif //__REALTIME_TESS_SORTING__

#ifdef __REALTIME_MATERIAL_SORTING__
	{// Material types.. To minimize changing between geom (grass) versions and non-grass shaders...
		if (shadera->materialType < shaderb->materialType)
			return -1;

		else if (shadera->materialType > shaderb->materialType)
			return 1;
	}
#endif //__REALTIME_MATERIAL_SORTING__

	return 0;
}

#define MAX_LIST_SHADERS 1048576
int					listShadersNum = 0;
shader_t			*listShaders[MAX_LIST_SHADERS] = { NULL };
int64_t				listEntityNums[MAX_LIST_SHADERS] = { 0 };
int64_t				listPostProcesses[MAX_LIST_SHADERS] = { 0 };
#endif //__REALTIME_SURFACE_SORTING__

void RB_RenderDrawSurfList(drawSurf_t *drawSurfs, int numDrawSurfs, qboolean inQuery) {
	int				i, max_threads_used = 0;
	int				j = 0;
	float			originalTime;
	FBO_t*			fbo = NULL;
	shader_t		*oldShader = NULL;
	int64_t			oldEntityNum = -1;
	int64_t			oldPostRender = 0;
	int             oldCubemapIndex = -1;
	int				oldDepthRange = 0;
	uint64_t		oldSort = (uint64_t)-1;
	qboolean		CUBEMAPPING = qfalse;
	float			depth[2];

	if (r_cubeMapping->integer >= 1 && !r_lowVram->integer)
	{
		CUBEMAPPING = qtrue;
	}

	if (((backEnd.refdef.rdflags & RDF_BLUR)
		|| (tr.viewParms.flags & VPF_SHADOWPASS)
		|| backEnd.depthFill
		|| (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		|| (tr.renderSkyFbo != NULL && backEnd.viewParms.targetFbo == tr.renderSkyFbo)))
	{
		CUBEMAPPING = qfalse;
	}

	// draw everything
	backEnd.currentEntity = &tr.worldEntity;

	depth[0] = 0.f;
	depth[1] = 1.f;

	backEnd.pc.c_surfaces += numDrawSurfs;

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	fbo = glState.currentFBO;

#ifdef __PLAYER_BASED_CUBEMAPS__
	currentPlayerCubemap = 0;

#ifndef __REALTIME_CUBEMAP__
	if (!tr.cubemapOrigins)
#endif //!__REALTIME_CUBEMAP__
	{
		currentPlayerCubemap = 0;

		currentPlayerCubemapVec[0] = 0;
		currentPlayerCubemapVec[1] = 0;
		currentPlayerCubemapVec[2] = 0;
		currentPlayerCubemapVec[3] = 1.0;

		currentPlayerCubemapDistance = 0.0;
	}
#ifndef __REALTIME_CUBEMAP__
	else if (CUBEMAPPING)
	{
		currentPlayerCubemap = R_CubemapForPoint(tr.refdef.vieworg);

		currentPlayerCubemapVec[0] = tr.cubemapOrigins[currentPlayerCubemap - 1][0] - tr.refdef.vieworg[0];
		currentPlayerCubemapVec[1] = tr.cubemapOrigins[currentPlayerCubemap - 1][1] - tr.refdef.vieworg[1];
		currentPlayerCubemapVec[2] = tr.cubemapOrigins[currentPlayerCubemap - 1][2] - tr.refdef.vieworg[2];
		currentPlayerCubemapVec[3] = 1.0f;

		currentPlayerCubemapDistance = Distance(tr.refdef.vieworg, tr.cubemapOrigins[currentPlayerCubemap - 1]);
	}
#endif /__REALTIME_CUBEMAP__
#endif //__PLAYER_BASED_CUBEMAPS__

//#define __DEBUG_MERGE__

#ifdef __DEBUG_MERGE__
	int numShaderChanges = 0;
	int numShaderDraws = 0;
#endif //__DEBUG_MERGE__

#ifdef __REALTIME_SURFACE_SORTING__
	if (r_testvalue0->integer)
	{
		qsort(drawSurfs, numDrawSurfs, sizeof(*drawSurfs), RealtimeSurfaceCompare);
	}
#endif //__REALTIME_SURFACE_SORTING__

	FBO_t *originalFBO = glState.currentFBO;

#ifdef __RENDER_PASSES__
	for (int type = RENDERPASS_NONE; type < RENDERPASS_MAX; type++)
	{
		// draw everything
		backEnd.currentEntity = &tr.worldEntity;

		oldShader = NULL;
		oldEntityNum = -1;
		oldPostRender = 0;
		oldCubemapIndex = -1;
		oldDepthRange = 0;
		oldSort = (uint64_t)-1;

		backEnd.renderPass = (renderPasses_t)type;

		if (backEnd.renderPass == RENDERPASS_PSHADOWS && !(!backEnd.viewIsOutdoors || !SHADOWS_ENABLED || RB_NightScale() == 1.0))
		{// No shadows in the day, when outdoors...
			continue;
		}
		else if (backEnd.renderPass == RENDERPASS_PSHADOWS)
		{
			FBO_Bind(tr.renderPshadowsFbo);
		}
		else if (glState.currentFBO == tr.renderPshadowsFbo)
		{// Switch back to original FBO after drawing PSHADOW pass...
			FBO_Bind(originalFBO);
		}

		// First draw world normally...
		for (i = 0; i < numDrawSurfs; ++i)
		{
			int64_t			zero = 0;
			drawSurf_t		*drawSurf = NULL;
			drawSurf_t		*oldDrawSurf = NULL;
			shader_t		*shader = NULL;
			int64_t			entityNum = -1;
			int64_t			postRender = -1;
			int             cubemapIndex, newCubemapIndex;
			int				depthRange;

			//if ((backEnd.depthFill || (tr.viewParms.flags & VPF_DEPTHSHADOW)) && shader && shader->sort != SS_OPAQUE && shader->sort != SS_SEE_THROUGH) continue; // UQ1: No point thinking any more on this one...

			drawSurf = &drawSurfs[i];

			if (!drawSurf || !drawSurf->surface || *drawSurf->surface <= SF_BAD || *drawSurf->surface >= SF_NUM_SURFACE_TYPES || *drawSurf->surface <= SF_SKIP) continue;

			shader_t *thisShader = tr.sortedShaders[(drawSurf->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
			int64_t thisEntityNum = (drawSurf->sort >> QSORT_REFENTITYNUM_SHIFT) & REFENTITYNUM_MASK;

			if (thisShader->surfaceFlags & SURF_NODRAW)
			{// Skip nodraws completely...
				continue;
			}

			if (thisShader == tr.defaultShader)
			{// Don't draw this either...
				continue;
			}

#ifdef __ZFAR_CULLING_ON_SURFACES__
			if (r_occlusion->integer)
			{
				if (!backEnd.depthFill
					&& drawSurf->depthDrawOnly
					&& !backEnd.projection2D
					&& !thisShader->isSky
					&& !thisShader->isWater)
				{// Surface is marked as only for depth draws, skip it unless its sky or water...
					continue;
				}
			}
#endif //__ZFAR_CULLING_ON_SURFACES__

			qboolean doDraw = qtrue;

			if (backEnd.renderPass != RENDERPASS_NONE)
			{// Skip any surfs that are not of this renderPass...
				extern qboolean RB_ShouldUseGeometryGrass(int materialType);

				qboolean isGrass = qfalse;
				qboolean isVines = qfalse;
				qboolean isGroundFoliage = qfalse;

				if (thisShader->isGrass || thisShader->isGroundFoliage || RB_ShouldUseGeometryGrass(thisShader->materialType))
				{
					isGrass = qtrue;
					isGroundFoliage = qtrue;
				}
				else if (thisShader->isVines || (thisShader->materialType == MATERIAL_TREEBARK || thisShader->materialType == MATERIAL_ROCK))
				{
					isVines = qtrue;
				}

				if (isGrass && backEnd.renderPass == RENDERPASS_GRASS)
				{
					doDraw = qtrue;
				}
				else if (isGroundFoliage && backEnd.renderPass == RENDERPASS_GROUNDFOLIAGE)
				{
					doDraw = qtrue;
				}
				else if (isVines && backEnd.renderPass == RENDERPASS_VINES)
				{
					doDraw = qtrue;
				}
				else if (thisEntityNum == REFENTITYNUM_WORLD && backEnd.renderPass == RENDERPASS_PSHADOWS)
				{
					doDraw = qtrue;
				}
				else
				{
					//continue;
					doDraw = qfalse;
				}
			}

#ifdef __PLAYER_BASED_CUBEMAPS__
#ifdef __REALTIME_CUBEMAP__
			newCubemapIndex = 0;
#else //!__REALTIME_CUBEMAP__
			newCubemapIndex = 0;// currentPlayerCubemap;
#endif //__REALTIME_CUBEMAP__
#else //!__PLAYER_BASED_CUBEMAPS__
			if (!CUBEMAPPING)
			{
				newCubemapIndex = 0;
			}
			else
			{
				if (r_cubeMapping->integer >= 1 && !r_lowVram->integer)
				{
					newCubemapIndex = drawSurf->cubemapIndex;
				}
				else
				{
					newCubemapIndex = 0;
				}

				if (newCubemapIndex > 0)
				{// Let's see if we can swap with a close cubemap and merge them...

					if (Distance(tr.refdef.vieworg, tr.cubemapOrigins[newCubemapIndex - 1]) > r_cubemapCullRange->value)
					{// Too far away to care about cubemaps... Allow merge...
						newCubemapIndex = 0;
					}
				}
			}
#endif //__PLAYER_BASED_CUBEMAPS__

			qboolean isWaterMerge = qfalse;

			if (shader != NULL
				&& shader->isWater
				&& thisShader->isWater)
			{
				isWaterMerge = qtrue;
			}

			qboolean isDepthMerge = qfalse;

			if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
			{// In depth and shadow passes, let's merge all the non-alpha draws, being a simple solid texture and all...
				if (shader != NULL
					&& !shader->hasAlpha
					&& !thisShader->hasAlpha)
				{
					isDepthMerge = qtrue;
				}
			}

			/*if (*drawSurf->surface != SF_VBO_MDVMESH && oldDrawSurf != NULL && *oldDrawSurf->surface == SF_VBO_MDVMESH)
			{
			RB_EndSurface();
			}

			oldDrawSurf = drawSurf;*/

			if (doDraw
				&& (isWaterMerge || isDepthMerge || drawSurf->sort == oldSort)
#if !defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__) && !defined(__REALTIME_CUBEMAP__)
				&& (!CUBEMAPPING || newCubemapIndex == oldCubemapIndex)
#endif //!defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__)
				)
			{// fast path, same as previous sort
				rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
				numShaderDraws++;
#endif //__DEBUG_MERGE__
				continue;
			}

			oldSort = drawSurf->sort;
			R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &zero, &postRender);

			cubemapIndex = newCubemapIndex;

			qboolean dontMerge = qfalse;

			if (DISABLE_LIFTS_AND_PORTALS_MERGE)
			{
				// UQ1: We can't merge movers and portals, but we can merge pretty much everything else...
				trRefEntity_t *ent = NULL;
				trRefEntity_t *oldent = NULL;

				if (entityNum >= 0) oldent = &backEnd.refdef.entities[entityNum];
				if (oldEntityNum >= 0) oldent = &backEnd.refdef.entities[oldEntityNum];

				if (ent && (ent->e.noMerge || (ent->e.renderfx & RF_SETANIMINDEX)))
				{// Either a mover, or a portal... Don't allow merges...
					dontMerge = qtrue;
				}
				else if (oldent && (oldent->e.noMerge || (oldent->e.renderfx & RF_SETANIMINDEX)))
				{// Either a mover, or a portal... Don't allow merges...
					dontMerge = qtrue;
				}
			}

			//
			// change the tess parameters if needed
			// a "entityMergable" shader is a shader that can have surfaces from seperate
			// entities merged into a single batch, like smoke and blood puff sprites
			if (shader != NULL
				&& (shader != oldShader
					|| postRender != oldPostRender
#if !defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__) && !defined(__REALTIME_CUBEMAP__)
					|| (CUBEMAPPING && cubemapIndex != oldCubemapIndex)
#endif //!defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__)
					|| (entityNum != oldEntityNum && !shader->entityMergable && dontMerge)))
			{
				if (oldShader != NULL)
				{
					RB_EndSurface();
				}

				RB_BeginSurface(shader, 0, cubemapIndex);

				backEnd.pc.c_surfBatches++;
				oldShader = shader;
				oldPostRender = postRender;
				oldCubemapIndex = cubemapIndex;
#ifdef __DEBUG_MERGE__
				numShaderChanges++;
#endif //__DEBUG_MERGE__
			}

			//
			// change the modelview matrix if needed
			//
			if (entityNum != oldEntityNum)
			{
				qboolean sunflare = qfalse;
				depthRange = 0;

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				if (entityNum != REFENTITYNUM_WORLD)
				{
					backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
					backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

					// set up the transformation matrix
					R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori);

					if (backEnd.currentEntity->needDlights)
					{// set up the dynamic lighting if needed
						R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
					}

					if (backEnd.currentEntity->e.renderfx & RF_NODEPTH)
					{// No depth at all, very rare but some things for seeing through walls
						depthRange = 2;
					}
					else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
					{// hack the depth range to prevent view model from poking into walls
						depthRange = 1;
					}
				}
				else {
					backEnd.currentEntity = &tr.worldEntity;
					backEnd.refdef.floatTime = originalTime;
					backEnd.ori = backEnd.viewParms.world;
					// we have to reset the shaderTime as well otherwise image animations on
					// the world (like water) continue with the wrong frame
					R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
				}

				GL_SetModelviewMatrix(backEnd.ori.modelViewMatrix);

				//
				// change depthrange. Also change projection matrix so first person weapon does not look like coming
				// out of the screen.
				//
				if (oldDepthRange != depthRange)
				{
					switch (depthRange) {
					default:
					case 0:
						if (!sunflare)
							qglDepthRange(0.0f, 1.0f);

						depth[0] = 0;
						depth[1] = 1;
						break;

					case 1:
						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.3f);

						break;

					case 2:
						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.0f);

						break;
					}

					oldDepthRange = depthRange;
				}

				oldEntityNum = entityNum;
			}

			// add the triangles for this surface
			if (doDraw) rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
			numShaderDraws++;
#endif //__DEBUG_MERGE__
		}

		// draw the contents of the last shader batch
		if (oldShader != NULL) {
			RB_EndSurface();
		}

		// go back to the world modelview matrix
		GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);

		// Restore depth range for subsequent rendering
		qglDepthRange(0.0f, 1.0f);
	}
#else
	{
		// First draw world normally...
		for (i = 0; i < numDrawSurfs; ++i)
		{
			int64_t			zero = 0;
			drawSurf_t		*drawSurf = NULL;
			drawSurf_t		*oldDrawSurf = NULL;
			shader_t		*shader = NULL;
			int64_t			entityNum = -1;
			int64_t			postRender = -1;
			int             cubemapIndex, newCubemapIndex;
			int				depthRange;

			//if ((backEnd.depthFill || (tr.viewParms.flags & VPF_DEPTHSHADOW)) && shader && shader->sort != SS_OPAQUE && shader->sort != SS_SEE_THROUGH) continue; // UQ1: No point thinking any more on this one...

			drawSurf = &drawSurfs[i];

			if (!drawSurf || !drawSurf->surface || *drawSurf->surface <= SF_BAD || *drawSurf->surface >= SF_NUM_SURFACE_TYPES || *drawSurf->surface <= SF_SKIP) continue;

			shader_t *thisShader = tr.sortedShaders[(drawSurf->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];

			if (thisShader->surfaceFlags & SURF_NODRAW)
			{// Skip nodraws completely...
				continue;
			}

			if (thisShader == tr.defaultShader)
			{// Don't draw this either...
				continue;
			}

#ifdef __ZFAR_CULLING_ON_SURFACES__
			if (r_occlusion->integer)
			{
				if (!backEnd.depthFill
					&& drawSurf->depthDrawOnly
					&& !backEnd.projection2D
					&& !thisShader->isSky
					&& !thisShader->isWater)
				{// Surface is marked as only for depth draws, skip it unless its sky or water...
					continue;
				}
			}
#endif //__ZFAR_CULLING_ON_SURFACES__

#ifdef __PLAYER_BASED_CUBEMAPS__
#ifdef __REALTIME_CUBEMAP__
			newCubemapIndex = 0;
#else //!__REALTIME_CUBEMAP__
			newCubemapIndex = 0;// currentPlayerCubemap;
#endif //__REALTIME_CUBEMAP__
#else //!__PLAYER_BASED_CUBEMAPS__
			if (!CUBEMAPPING)
			{
				newCubemapIndex = 0;
			}
			else
			{
				if (r_cubeMapping->integer >= 1 && !r_lowVram->integer)
				{
					newCubemapIndex = drawSurf->cubemapIndex;
				}
				else
				{
					newCubemapIndex = 0;
				}

				if (newCubemapIndex > 0)
				{// Let's see if we can swap with a close cubemap and merge them...

					if (Distance(tr.refdef.vieworg, tr.cubemapOrigins[newCubemapIndex - 1]) > r_cubemapCullRange->value)
					{// Too far away to care about cubemaps... Allow merge...
						newCubemapIndex = 0;
					}
				}
			}
#endif //__PLAYER_BASED_CUBEMAPS__

			qboolean isWaterMerge = qfalse;

			if (shader != NULL
				&& shader->isWater
				&& thisShader->isWater)
			{
				isWaterMerge = qtrue;
			}

			qboolean isDepthMerge = qfalse;

			if (backEnd.depthFill || (tr.viewParms.flags & VPF_SHADOWPASS))
			{// In depth and shadow passes, let's merge all the non-alpha draws, being a simple solid texture and all...
				if (shader != NULL
					&& !shader->hasAlpha
					&& !thisShader->hasAlpha)
				{
					isDepthMerge = qtrue;
				}
			}

			/*if (*drawSurf->surface != SF_VBO_MDVMESH && oldDrawSurf != NULL && *oldDrawSurf->surface == SF_VBO_MDVMESH)
			{
				RB_EndSurface();
			}

			oldDrawSurf = drawSurf;*/

			if ((isWaterMerge || isDepthMerge || drawSurf->sort == oldSort)
#if !defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__) && !defined(__REALTIME_CUBEMAP__)
				&& (!CUBEMAPPING || newCubemapIndex == oldCubemapIndex)
#endif //!defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__)
				)
			{// fast path, same as previous sort
				rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
				numShaderDraws++;
#endif //__DEBUG_MERGE__
				continue;
			}

			oldSort = drawSurf->sort;
			R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &zero, &postRender);

			cubemapIndex = newCubemapIndex;

			qboolean dontMerge = qfalse;

			if (DISABLE_LIFTS_AND_PORTALS_MERGE)
			{
				// UQ1: We can't merge movers and portals, but we can merge pretty much everything else...
				trRefEntity_t *ent = NULL;
				trRefEntity_t *oldent = NULL;

				if (entityNum >= 0) oldent = &backEnd.refdef.entities[entityNum];
				if (oldEntityNum >= 0) oldent = &backEnd.refdef.entities[oldEntityNum];

				if (ent && (ent->e.noMerge || (ent->e.renderfx & RF_SETANIMINDEX)))
				{// Either a mover, or a portal... Don't allow merges...
					dontMerge = qtrue;
				}
				else if (oldent && (oldent->e.noMerge || (oldent->e.renderfx & RF_SETANIMINDEX)))
				{// Either a mover, or a portal... Don't allow merges...
					dontMerge = qtrue;
				}
			}

			//
			// change the tess parameters if needed
			// a "entityMergable" shader is a shader that can have surfaces from seperate
			// entities merged into a single batch, like smoke and blood puff sprites
			if (shader != NULL
				&& (shader != oldShader
					|| postRender != oldPostRender
#if !defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__) && !defined(__REALTIME_CUBEMAP__)
					|| (CUBEMAPPING && cubemapIndex != oldCubemapIndex)
#endif //!defined(__LAZY_CUBEMAP__) && !defined(__PLAYER_BASED_CUBEMAPS__)
					|| (entityNum != oldEntityNum && !shader->entityMergable && dontMerge)))
			{
				if (oldShader != NULL)
				{
					RB_EndSurface();
				}

				RB_BeginSurface(shader, 0, cubemapIndex);

				backEnd.pc.c_surfBatches++;
				oldShader = shader;
				oldPostRender = postRender;
				oldCubemapIndex = cubemapIndex;
#ifdef __DEBUG_MERGE__
				numShaderChanges++;
#endif //__DEBUG_MERGE__
			}

			//
			// change the modelview matrix if needed
			//
			if (entityNum != oldEntityNum)
			{
				qboolean sunflare = qfalse;
				depthRange = 0;

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				if (entityNum != REFENTITYNUM_WORLD)
				{
					backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
					backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

					// set up the transformation matrix
					R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.ori);

					if (backEnd.currentEntity->needDlights)
					{// set up the dynamic lighting if needed
						R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
					}

					if (backEnd.currentEntity->e.renderfx & RF_NODEPTH)
					{// No depth at all, very rare but some things for seeing through walls
						depthRange = 2;
					}
					else if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
					{// hack the depth range to prevent view model from poking into walls
						depthRange = 1;
					}
				}
				else {
					backEnd.currentEntity = &tr.worldEntity;
					backEnd.refdef.floatTime = originalTime;
					backEnd.ori = backEnd.viewParms.world;
					// we have to reset the shaderTime as well otherwise image animations on
					// the world (like water) continue with the wrong frame
					R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori);
				}

				GL_SetModelviewMatrix(backEnd.ori.modelViewMatrix);

				//
				// change depthrange. Also change projection matrix so first person weapon does not look like coming
				// out of the screen.
				//
				if (oldDepthRange != depthRange)
				{
					switch (depthRange) {
					default:
					case 0:
						if (!sunflare)
							qglDepthRange(0.0f, 1.0f);

						depth[0] = 0;
						depth[1] = 1;
						break;

					case 1:
						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.3f);

						break;

					case 2:
						if (!oldDepthRange)
							qglDepthRange(0.0f, 0.0f);

						break;
					}

					oldDepthRange = depthRange;
				}

				oldEntityNum = entityNum;
			}

			// add the triangles for this surface
			rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
#ifdef __DEBUG_MERGE__
			numShaderDraws++;
#endif //__DEBUG_MERGE__
		}
	}
#endif

#ifdef __DEBUG_MERGE__
	ri->Printf(PRINT_ALL, "%i total surface draws. %i shader changes.\n", numShaderDraws, numShaderChanges);
#endif //__DEBUG_MERGE__

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	if (inQuery) {
		qglEndQuery(GL_SAMPLES_PASSED);
	}

	FBO_Bind(fbo);

	// go back to the world modelview matrix

	GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);

	// Restore depth range for subsequent rendering
	qglDepthRange(0.0f, 1.0f);
}


/*
============================================================================

RENDER BACK END FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void	RB_SetGL2D (void) {
	matrix_t matrix;
	int width, height;

	if (backEnd.projection2D && backEnd.last2DFBO == glState.currentFBO)
		return;

	backEnd.projection2D = qtrue;
	backEnd.last2DFBO = glState.currentFBO;

	if (glState.currentFBO)
	{
		width = glState.currentFBO->width;
		height = glState.currentFBO->height;
	}
	else
	{
		width = glConfig.vidWidth * r_superSampleMultiplier->value;
		height = glConfig.vidHeight * r_superSampleMultiplier->value;
	}

	// set 2D virtual screen size
	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Matrix16Ortho(0, 640, 480, 0, 0, 1, matrix);
	GL_SetProjectionMatrix(matrix);
	Matrix16Identity(matrix);
	GL_SetModelviewMatrix(matrix);

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri->Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;

	// reset color scaling
	backEnd.refdef.colorScale = 1.0f;
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	if ( !tr.registered ) {
		return;
	}
	R_IssuePendingRenderCommands();

	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definately want to sync every frame for the cinematics
#ifdef __USE_QGL_FINISH__
	qglFinish();
#else if __USE_QGL_FLUSH__
	if (r_finish->integer) qglFlush();
#endif //__USE_QGL_FINISH__

	start = 0;
	if ( r_speeds->integer ) {
		start = ri->Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri->Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	RE_UploadCinematic (cols, rows, data, client, dirty);

	if ( r_speeds->integer ) {
		end = ri->Milliseconds();
		ri->Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	VectorSet4(quadVerts[0], x,     y,     0.0f, 1.0f);
	VectorSet4(quadVerts[1], x + w, y,     0.0f, 1.0f);
	VectorSet4(quadVerts[2], x + w, y + h, 0.0f, 1.0f);
	VectorSet4(quadVerts[3], x,     y + h, 0.0f, 1.0f);

	VectorSet2(texCoords[0], 0.5f / cols,          0.5f / rows);
	VectorSet2(texCoords[1], (cols - 0.5f) / cols, 0.5f / rows);
	VectorSet2(texCoords[2], (cols - 0.5f) / cols, (rows - 0.5f) / rows);
	VectorSet2(texCoords[3], 0.5f / cols,          (rows - 0.5f) / rows);

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMatrix16(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}

void RE_UploadCinematic (int cols, int rows, const byte *data, int client, qboolean dirty) {

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}


/*
=============
RB_SetColor

=============
*/
const void	*RB_SetColor( const void *data ) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)(cmd + 1);
}

/*
=============
RB_StretchPic
=============
*/
const void *RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int		numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[numVerts]);
		VectorCopy4(color, tess.vertexColors[numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[numVerts + 3]);
	}


	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.normal[numVerts] = R_TessXYZtoPackedNormals(tess.xyz[numVerts]);

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.normal[numVerts + 1] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 1]);

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.normal[numVerts + 2] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 2]);

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.normal[numVerts + 3] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 3]);

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawRotatePic
=============
*/
const void *RB_RotatePic ( const void *data ) 
{
	const rotatePicCommand_t	*cmd;
	shader_t *shader;

	cmd = (const rotatePicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	float angle = DEG2RAD(cmd->a);
	float s = sinf(angle);
	float c = cosf(angle);

	matrix3_t m = {
		{ c, s, 0.0f },
		{ -s, c, 0.0f },
		{ cmd->x + cmd->w, cmd->y, 1.0f }
	};

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[numVerts]);
		VectorCopy4(color, tess.vertexColors[numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[numVerts + 3]);
	}

	tess.xyz[numVerts][0] = m[0][0] * (-cmd->w) + m[2][0];
	tess.xyz[numVerts][1] = m[0][1] * (-cmd->w) + m[2][1];
	tess.xyz[numVerts][2] = 0;

	tess.normal[numVerts] = R_TessXYZtoPackedNormals(tess.xyz[numVerts]);

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = m[2][0];
	tess.xyz[numVerts + 1][1] = m[2][1];
	tess.xyz[numVerts + 1][2] = 0;

	tess.normal[numVerts + 1] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 1]);

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = m[1][0] * (cmd->h) + m[2][0];
	tess.xyz[numVerts + 2][1] = m[1][1] * (cmd->h) + m[2][1];
	tess.xyz[numVerts + 2][2] = 0;

	tess.normal[numVerts + 2] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 2]);

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = m[0][0] * (-cmd->w) + m[1][0] * (cmd->h) + m[2][0];
	tess.xyz[numVerts + 3][1] = m[0][1] * (-cmd->w) + m[1][1] * (cmd->h) + m[2][1];
	tess.xyz[numVerts + 3][2] = 0;

	tess.normal[numVerts + 3] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 3]);

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawRotatePic2
=============
*/
const void *RB_RotatePic2 ( const void *data ) 
{
	const rotatePicCommand_t	*cmd;
	shader_t *shader;

	cmd = (const rotatePicCommand_t *)data;

	// FIXME: HUGE hack
	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	float angle = DEG2RAD(cmd->a);
	float s = sinf(angle);
	float c = cosf(angle);

	matrix3_t m = {
		{ c, s, 0.0f },
		{ -s, c, 0.0f },
		{ cmd->x, cmd->y, 1.0f }
	};

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[numVerts]);
		VectorCopy4(color, tess.vertexColors[numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[numVerts + 3]);
	}

	tess.xyz[numVerts][0] = m[0][0] * (-cmd->w * 0.5f) + m[1][0] * (-cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts][1] = m[0][1] * (-cmd->w * 0.5f) + m[1][1] * (-cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts][2] = 0;

	tess.normal[numVerts] = R_TessXYZtoPackedNormals(tess.xyz[numVerts]);

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = m[0][0] * (cmd->w * 0.5f) + m[1][0] * (-cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts + 1][1] = m[0][1] * (cmd->w * 0.5f) + m[1][1] * (-cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts + 1][2] = 0;

	tess.normal[numVerts + 1] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 1]);

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = m[0][0] * (cmd->w * 0.5f) + m[1][0] * (cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts + 2][1] = m[0][1] * (cmd->w * 0.5f) + m[1][1] * (cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts + 2][2] = 0;

	tess.normal[numVerts + 2] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 2]);

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = m[0][0] * (-cmd->w * 0.5f) + m[1][0] * (cmd->h * 0.5f) + m[2][0];
	tess.xyz[numVerts + 3][1] = m[0][1] * (-cmd->w * 0.5f) + m[1][1] * (cmd->h * 0.5f) + m[2][1];
	tess.xyz[numVerts + 3][2] = 0;

	tess.normal[numVerts + 3] = R_TessXYZtoPackedNormals(tess.xyz[numVerts + 3]);

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawSurfs

=============
*/

extern qboolean DISABLE_DEPTH_PREPASS;

const void	*RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	if (backEnd.viewParms.flags & VPF_DEPTHCLAMP)
	{
		qglEnable(GL_DEPTH_CLAMP);
	}

	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) 
		&& ((r_depthPrepass->integer && !DISABLE_DEPTH_PREPASS) || backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		//FBO_t *oldFbo = glState.currentFBO;

		if (r_occlusion->integer)
		{// Override occlusion for depth prepass and shadow pass...
			//tr.viewParms.zFar = tr.occlusionOriginalZfar;
			extern float RB_GetNextOcclusionRange(float currentRange);
			tr.viewParms.zFar = RB_GetNextOcclusionRange(tr.occlusionZfar);
		}

		backEnd.depthFill = qtrue;
		qboolean FBO_SWITCHED = qfalse;
		if (glState.currentFBO == tr.renderFbo)
		{// Skip outputting to deferred textures while doing depth prepass, by using a depth prepass FBO without any attached textures.
			FBO_Bind(tr.renderDepthFbo);
			FBO_SWITCHED = qtrue;
		}
		qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs, qfalse );
		qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		if (FBO_SWITCHED)
		{// Switch back to original FBO (renderFbo).
			FBO_Bind(tr.renderFbo);
		}
		backEnd.depthFill = qfalse;

		if (r_occlusion->integer)
		{// Set occlusion zFar again, now that depth prepass is completed...
			tr.viewParms.zFar = tr.occlusionZfar;
		}

#if 0
		if (tr.msaaResolveFbo)
		{
			// If we're using multisampling, resolve the depth first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
		else 
#endif
		if (tr.renderFbo == NULL)
		{
			// If we're rendering directly to the screen, copy the depth to a texture
			GL_BindToTMU(tr.renderDepthImage, 0);
			//if (r_hdr->integer)
			//	qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, 0);
			//else
				qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, 0);
		}
	}

	if (backEnd.viewParms.flags & VPF_DEPTHCLAMP)
	{
		qglDisable(GL_DEPTH_CLAMP);
	}

	if (backEnd.viewParms.flags & VPF_DEPTHSHADOW)
	{
		return (const void *)(cmd + 1);
	}

	if (!(backEnd.viewParms.flags & VPF_SHADOWPASS) && !(backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs, qfalse );

		if (r_drawSun->integer)
		{
			//RB_DrawSun(0.1, tr.sunShader);
			if (RB_NightScale() > 0.0)
				RB_DrawMoon(0.05, tr.sunFlareShader); // UQ1: Now in the skybox shader... Drawing it invisible because rend2 occlusion goes nuts at night without it... ???
		}

		//if (r_drawSunRays->integer)
		{
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.sunRaysFbo);
			
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			qglClear( GL_COLOR_BUFFER_BIT );

			tr.sunFlareQueryActive[tr.sunFlareQueryIndex] = qtrue;
			qglBeginQuery(GL_SAMPLES_PASSED, tr.sunFlareQuery[tr.sunFlareQueryIndex]);

			RB_DrawSun(0.3, tr.sunFlareShader);

			qglEndQuery(GL_SAMPLES_PASSED);

			FBO_Bind(oldFbo);
		}

		// darken down any stencil shadows
		RB_ShadowFinish();		

#ifdef __Q3_FLARES__
		// add light flares on lights that aren't obscured
		RB_RenderFlares();
#endif //__Q3_FLARES__
	}

	if (tr.renderCubeFbo != NULL && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		ALLOW_NULL_FBO_BIND = qtrue;
		FBO_Bind(NULL);
		GL_SelectTexture(TB_CUBEMAP);
#ifndef __REALTIME_CUBEMAP__
		if (backEnd.viewParms.flags & VPF_EMISSIVEMAP)
			GL_BindToTMU(tr.emissivemaps[backEnd.viewParms.targetFboCubemapIndex], TB_CUBEMAP);
		else
			GL_BindToTMU(tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex], TB_CUBEMAP);
#else //__REALTIME_CUBEMAP__
		GL_BindToTMU(tr.realtimeCubemap, TB_CUBEMAP);
#endif //__REALTIME_CUBEMAP__
		qglGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		GL_SelectTexture(0);
		ALLOW_NULL_FBO_BIND = qfalse;
	}
	else if (tr.renderSkyFbo != NULL && backEnd.viewParms.targetFbo == tr.renderSkyFbo)
	{
		ALLOW_NULL_FBO_BIND = qtrue;
		FBO_Bind(NULL);
		GL_SelectTexture(TB_CUBEMAP);
		
		if (backEnd.viewParms.flags & VPF_SKYCUBENIGHT)
			GL_BindToTMU(tr.skyCubeMapNight, TB_CUBEMAP);
		else
			GL_BindToTMU(tr.skyCubeMap, TB_CUBEMAP);

		qglGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		GL_SelectTexture(0);
		ALLOW_NULL_FBO_BIND = qfalse;
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer

=============
*/
const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	FBO_Bind(NULL);

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)(cmd + 1);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	qglClear( GL_COLOR_BUFFER_BIT );

#ifdef __USE_QGL_FINISH__
	qglFinish();
#else if __USE_QGL_FLUSH__
	if (r_finish->integer) qglFlush();
#endif //__USE_QGL_FINISH__

	start = ri->Milliseconds();

	for ( i=0 ; i<tr.numImages ; i++ ) {
		image = tr.images[i];

		w = (glConfig.vidWidth * r_superSampleMultiplier->value) / 20;
		h = (glConfig.vidHeight * r_superSampleMultiplier->value) / 15;
		x = (float)(i % 20) * (float)w;
		y = (float)((float)i / 20) * (float)h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		{
			vec4_t quadVerts[4];

			GL_Bind(image);

			VectorSet4(quadVerts[0], x, y, 0, 1);
			VectorSet4(quadVerts[1], x + w, y, 0, 1);
			VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
			VectorSet4(quadVerts[3], x, y + h, 0, 1);

			RB_InstantQuad(quadVerts);
		}
	}

#ifdef __USE_QGL_FINISH__
	qglFinish();
#else if __USE_QGL_FLUSH__
	qglFlush();
#endif //__USE_QGL_FINISH__

	end = ri->Milliseconds();
	ri->Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

}

/*
=============
RB_ColorMask

=============
*/
const void *RB_ColorMask(const void *data)
{
	const colorMaskCommand_t *cmd = (colorMaskCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// reverse color mask, so 0 0 0 0 is the default
	backEnd.colorMask[0] = (qboolean)(!cmd->rgba[0]);
	backEnd.colorMask[1] = (qboolean)(!cmd->rgba[1]);
	backEnd.colorMask[2] = (qboolean)(!cmd->rgba[2]);
	backEnd.colorMask[3] = (qboolean)(!cmd->rgba[3]);

	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
	
	return (const void *)(cmd + 1);
}

/*
=============
RB_ClearDepth

=============
*/
const void *RB_ClearDepth(const void *data)
{
	const clearDepthCommand_t *cmd = (clearDepthCommand_t *)data;
	
	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// texture swapping test
	if (r_showImages->integer)
		RB_ShowImages();

	if (!tr.renderFbo || backEnd.framePostProcessed)
	{
		FBO_Bind(NULL);
	}
	else
	{
		FBO_Bind(tr.renderFbo);
	}

	qglClear(GL_DEPTH_BUFFER_BIT);

#if 0
	// if we're doing MSAA, clear the depth texture for the resolve buffer
	if (tr.msaaResolveFbo)
	{
		FBO_Bind(tr.msaaResolveFbo);
		qglClear(GL_DEPTH_BUFFER_BIT);
	}
#endif
	
	return (const void *)(cmd + 1);
}


/*
=============
RB_SwapBuffers

=============
*/
const void	*RB_SwapBuffers( const void *data ) {
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	ResetGhoul2RenderableSurfaceHeap();

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = (unsigned char *)ri->Hunk_AllocateTempMemory( (glConfig.vidWidth * r_superSampleMultiplier->value) * (glConfig.vidHeight * r_superSampleMultiplier->value) );
		qglReadPixels( 0, 0, glConfig.vidWidth * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < (glConfig.vidWidth * r_superSampleMultiplier->value) * (glConfig.vidHeight * r_superSampleMultiplier->value); i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri->Hunk_FreeTempMemory( stencilReadback );
	}

	if (!backEnd.framePostProcessed)
	{
#if 0
		if (tr.msaaResolveFbo && r_hdr->integer)
		{
			// Resolving an RGB16F MSAA FBO to the screen messes with the brightness, so resolve to an RGB16F FBO first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			FBO_FastBlit(tr.msaaResolveFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else 
#endif
		if (tr.renderFbo)
		{
			FBO_FastBlit(tr.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
	}

	void RE_RenderImGui();
	RE_RenderImGui();

#ifdef __USE_QGL_FINISH__
	if ( !glState.finishCalled ) {
		qglFinish();
	}
#else if __USE_QGL_FLUSH__
	if ( r_finish->integer && !glState.finishCalled ) {
		qglFlush();
	}
#endif //__USE_QGL_FINISH__

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	GLimp_EndFrame();

	backEnd.framePostProcessed = qfalse;
	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
}

#ifdef __JKA_WEATHER__
extern void RB_RenderWorldEffects(void);

#ifdef __INSTANCED_MODELS__
void R_AddInstancedModelsToScene(void);
#endif //__INSTANCED_MODELS__

const void	*RB_WorldEffects( const void *data )
{
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	// Always flush the tess buffer
	if ( tess.numIndexes )
	{
		RB_EndSurface();
	}

#ifdef __INSTANCED_MODELS__
	if (tr.world
		&& !(tr.viewParms.flags & VPF_NOPOSTPROCESS)
		&& !(tr.refdef.rdflags & RDF_NOWORLDMODEL)
		&& !(backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)
		&& !(tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		&& !(tr.renderSkyFbo && backEnd.viewParms.targetFbo == tr.renderSkyFbo))
	{
		matrix_t previousModelViewMarix, previousProjectionMatrix;

		Matrix16Copy(glState.modelview, previousModelViewMarix);
		Matrix16Copy(glState.projection, previousProjectionMatrix);

		FBO_t *previousFBO = glState.currentFBO;
		float previousZfar = tr.viewParms.zFar;
		uint32_t previousState = glState.glStateBits;
		int previousCull = glState.faceCulling;

		R_AddInstancedModelsToScene();

		if (tess.shader)
		{
			RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);
		}

		FBO_Bind(previousFBO);
		GL_SetProjectionMatrix(previousProjectionMatrix);
		GL_SetModelviewMatrix(previousModelViewMarix);
		tr.viewParms.zFar = previousZfar;
		GL_State(previousState);
		GL_Cull(previousCull);
	}
#endif //__INSTANCED_MODELS__

	qboolean waterEnabled = qfalse;

#ifdef __OCEAN__
	extern qboolean WATER_FARPLANE_ENABLED;
	if (r_glslWater->integer && WATER_ENABLED && WATER_FARPLANE_ENABLED)
	{
		waterEnabled = qtrue;
	}
#endif //__OCEAN__

	extern qboolean RB_WeatherEnabled(void);
	extern qboolean PROCEDURAL_CLOUDS_ENABLED;
	extern qboolean PROCEDURAL_CLOUDS_LAYER;

	qboolean doProceduralClouds = (qboolean)(PROCEDURAL_CLOUDS_ENABLED && PROCEDURAL_CLOUDS_LAYER);

	if (!tr.world
		|| (tr.viewParms.flags & VPF_NOPOSTPROCESS)
		|| (tr.refdef.rdflags & RDF_NOWORLDMODEL)
		|| (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)
		|| ((backEnd.viewParms.flags & VPF_SHADOWPASS) /*&& !doProceduralClouds*/)
		|| ((backEnd.viewParms.flags & VPF_DEPTHSHADOW) /*&& !doProceduralClouds*/)
		|| (backEnd.depthFill /*&& !doProceduralClouds*/)
		|| (tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		|| (tr.renderSkyFbo && backEnd.viewParms.targetFbo == tr.renderSkyFbo)
		|| (!RB_WeatherEnabled() && !waterEnabled && !doProceduralClouds))
	{
		// do nothing
		return (const void *)(cmd + 1);
	}

	matrix_t previousModelViewMarix, previousProjectionMatrix;

	Matrix16Copy(glState.modelview, previousModelViewMarix);
	Matrix16Copy(glState.projection, previousProjectionMatrix);

	FBO_t *previousFBO = glState.currentFBO;
	float previousZfar = tr.viewParms.zFar;
	uint32_t previousState = glState.glStateBits;
	int previousCull = glState.faceCulling;

	if (doProceduralClouds)
	{
		void CLOUD_LAYER_Render();
		CLOUD_LAYER_Render();
	}

	if (RB_WeatherEnabled())
	{
		RB_RenderWorldEffects();
	}

#ifdef __OCEAN__
	if (waterEnabled)
	{
		extern void OCEAN_Render(void);
		OCEAN_Render();
	}
#endif //__OCEAN__

	if (tess.shader)
	{
		RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);
	}

	FBO_Bind(previousFBO);
	GL_SetProjectionMatrix(previousProjectionMatrix);
	GL_SetModelviewMatrix(previousModelViewMarix);
	tr.viewParms.zFar = previousZfar;
	GL_State(previousState);
	GL_Cull(previousCull);

	return (const void *)(cmd + 1);
}
#endif //__JKA_WEATHER__

/*
=============
RB_CapShadowMap

=============
*/
const void *RB_CapShadowMap(const void *data)
{
	const capShadowmapCommand_t *cmd = (const capShadowmapCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (cmd->map != -1)
	{
		GL_SelectTexture(0);
		if (cmd->cubeSide != -1)
		{
			if (tr.shadowCubemaps[cmd->map] != NULL)
			{
				GL_Bind(tr.shadowCubemaps[cmd->map]);
				qglCopyTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + cmd->cubeSide, 0, 0, 0, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE );
			}
		}
		else
		{
			if (tr.pshadowMaps[cmd->map] != NULL)
			{
				GL_Bind(tr.pshadowMaps[cmd->map]);
				qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE );
			}
		}
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_PostProcess

=============
*/
qboolean ALLOW_NULL_FBO_BIND = qfalse;

extern qboolean ENABLE_DISPLACEMENT_MAPPING;
extern qboolean FOG_POST_ENABLED;
extern qboolean AO_DIRECTIONAL;
extern int LATE_LIGHTING_ENABLED;

const void *RB_PostProcess(const void *data)
{
	const postProcessCommand_t *cmd = (const postProcessCommand_t *)data;
	FBO_t *srcFbo;
	vec4i_t srcBox, dstBox;
	qboolean autoExposure;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	GL_SetDefaultState();

	tess.numVertexes = tess.numVertexes = 0;

	if (!r_postProcess->integer 
		|| (tr.viewParms.flags & VPF_NOPOSTPROCESS)
		|| (backEnd.viewParms.flags & VPF_SHADOWPASS)
		|| (backEnd.viewParms.flags & VPF_DEPTHSHADOW)
		|| backEnd.depthFill
		|| (tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
		|| (tr.renderSkyFbo && backEnd.viewParms.targetFbo == tr.renderSkyFbo))
	{
		// do nothing
		return (const void *)(cmd + 1);
	}

	if (cmd)
	{
		backEnd.refdef = cmd->refdef;
		backEnd.viewParms = cmd->viewParms;
	}

	srcFbo = tr.renderFbo;

#if 0
	if (tr.msaaResolveFbo)
	{
		// Resolve the MSAA before anything else
		// Can't resolve just part of the MSAA FBO, so multiple views will suffer a performance hit here
		FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		srcFbo = tr.msaaResolveFbo;

		if ( r_dynamicGlow->integer || r_anamorphic->integer )
		{
			FBO_FastBlitIndexed(tr.renderFbo, tr.msaaResolveFbo, 1, 1, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
	}
#endif

	dstBox[0] = backEnd.viewParms.viewportX;
	dstBox[1] = backEnd.viewParms.viewportY;
	dstBox[2] = backEnd.viewParms.viewportWidth;
	dstBox[3] = backEnd.viewParms.viewportHeight;

	// Pre-linearize all possibly needed depth maps, in a single pass...
	if (!r_lowVram->integer)
	{
		DEBUG_StartTimer("Linearize", qtrue);
		RB_LinearizeDepth();
		DEBUG_EndTimer(qtrue);
	}

	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
		&& r_sunlightMode->integer >= 2
		&& tr.screenShadowFbo
		&& (backEnd.viewParms.flags & VPF_USESUNLIGHT)
		&& !(backEnd.viewParms.flags & VPF_DEPTHSHADOW)
		&& !(backEnd.viewParms.flags & VPF_SHADOWPASS)
		&& SHADOWS_ENABLED
		&& RB_NightScale() < 1.0 // Can ignore rendering shadows at night...
		&& r_deferredLighting->integer)
	{
		FBO_t *oldFbo = glState.currentFBO;

		DEBUG_StartTimer("Shadow Combine", qtrue);

		vec4_t quadVerts[4];
		vec2_t texCoords[4];
		vec4_t box;

		FBO_Bind(tr.screenShadowFbo);

		box[0] = backEnd.viewParms.viewportX      * tr.screenShadowFbo->width / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
		box[1] = backEnd.viewParms.viewportY      * tr.screenShadowFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
		box[2] = backEnd.viewParms.viewportWidth  * tr.screenShadowFbo->width / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
		box[3] = backEnd.viewParms.viewportHeight * tr.screenShadowFbo->height / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

		qglViewport(box[0], box[1], box[2], box[3]);
		qglScissor(box[0], box[1], box[2], box[3]);

		box[0] = backEnd.viewParms.viewportX / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
		box[1] = backEnd.viewParms.viewportY / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);
		box[2] = box[0] + backEnd.viewParms.viewportWidth / ((float)glConfig.vidWidth * r_superSampleMultiplier->value);
		box[3] = box[1] + backEnd.viewParms.viewportHeight / ((float)glConfig.vidHeight * r_superSampleMultiplier->value);

		texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
		texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
		texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
		texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

		box[0] = -1.0f;
		box[1] = -1.0f;
		box[2] = 1.0f;
		box[3] = 1.0f;

		VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
		VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
		VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
		VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

		GL_State(GLS_DEPTHTEST_DISABLE);

		GLSL_BindProgram(&tr.shadowmaskShader);

		GL_BindToTMU(tr.linearDepthImageZfar, TB_COLORMAP);

		GL_BindToTMU(tr.sunShadowDepthImage[0], TB_SHADOWMAP);
		GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP, backEnd.refdef.sunShadowMvp[0]);

		GL_BindToTMU(tr.sunShadowDepthImage[1], TB_SHADOWMAP2);
		GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP2, backEnd.refdef.sunShadowMvp[1]);

		GL_BindToTMU(tr.sunShadowDepthImage[2], TB_SHADOWMAP3);
		GLSL_SetUniformMatrix16(&tr.shadowmaskShader, UNIFORM_SHADOWMVP3, backEnd.refdef.sunShadowMvp[2]);

		GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);

		extern qboolean SHADOWS_FULL_SOLID;
		vec4_t vec;
		VectorSet4(vec, r_shadowSamples->value, r_shadowMapSize->value, SHADOWS_FULL_SOLID, 0.0);
		GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_SETTINGS0, vec);

		VectorSet4(vec, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value, r_testshaderValue4->value);
		GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_SETTINGS1, vec);

		{
			vec4_t viewInfo;

			float zmax = r_occlusion->integer ? tr.occlusionOriginalZfar : backEnd.viewParms.zFar;

			float zmax2 = backEnd.viewParms.zFar;
			float ymax2 = zmax2 * tan(backEnd.viewParms.fovY * M_PI / 360.0f);
			float xmax2 = zmax2 * tan(backEnd.viewParms.fovX * M_PI / 360.0f);

			float zmin = r_znear->value;

			vec3_t viewBasis[3];
			VectorScale(backEnd.refdef.viewaxis[0], zmax2, viewBasis[0]);
			VectorScale(backEnd.refdef.viewaxis[1], xmax2, viewBasis[1]);
			VectorScale(backEnd.refdef.viewaxis[2], ymax2, viewBasis[2]);

			GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWFORWARD, viewBasis[0]);
			GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWLEFT, viewBasis[1]);
			GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWUP, viewBasis[2]);

			VectorSet4(viewInfo, zmax / zmin, zmax, /*r_hdr->integer ? 32.0 :*/ 24.0, zmin);

			GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_VIEWINFO, viewInfo);
		}


		RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

		DEBUG_EndTimer(qtrue);

		if (r_shadowBlur->integer)
		{// When not at night, don't bother to blur shadows...
			DEBUG_StartTimer("Shadow Blur", qtrue);
			RB_FastBlur(tr.screenShadowFbo, NULL, tr.screenShadowBlurFbo, NULL);
			DEBUG_EndTimer(qtrue);
		}

		// reset viewport and scissor
		FBO_Bind(oldFbo);
		SetViewportAndScissor();
	}

	//
	// UQ1: Added...
	//
	qboolean SCREEN_BLUR = qfalse;
	qboolean SCREEN_BLUR_MENU = glConfig.menuIsOpen;

	if (backEnd.refdef.rdflags & RDF_BLUR)
	{// Skip most of the fancy stuff when doing a blured screen...
		SCREEN_BLUR = qtrue;
	}

	/*if (!SCREEN_BLUR && r_dynamiclight->integer && r_volumeLight->integer)
	{
		if (!r_lowVram->integer)
		{
			DEBUG_StartTimer("Volumetric Light Generate", qtrue);
			RB_GenerateVolumeLightImage();
			DEBUG_EndTimer(qtrue);
		}
	}*/

	DEBUG_StartTimer("Dynamic Glow", qtrue);

	if (!(backEnd.refdef.rdflags & RDF_BLUR)
		&& !(backEnd.viewParms.flags & VPF_SHADOWPASS)
		&& !(backEnd.viewParms.flags & VPF_DEPTHSHADOW)
		&& !backEnd.depthFill
		&& (r_dynamicGlow->integer || r_anamorphic->integer || r_bloom->integer))
	{
		RB_BloomDownscale(tr.glowImage, tr.glowFboScaled[0]);
		int numPasses = Com_Clampi(1, ARRAY_LEN(tr.glowFboScaled), r_dynamicGlowPasses->integer);
		for ( int i = 1; i < numPasses; i++ )
			RB_BloomDownscale2(tr.glowFboScaled[i - 1], tr.glowFboScaled[i]);
 
		for ( int i = numPasses - 2; i >= 0; i-- )
			RB_BloomUpscale(tr.glowFboScaled[i + 1], tr.glowFboScaled[i]);
	}

	DEBUG_EndTimer(qtrue);

	srcBox[0] = backEnd.viewParms.viewportX;
	srcBox[1] = backEnd.viewParms.viewportY;
	srcBox[2] = backEnd.viewParms.viewportWidth;
	srcBox[3] = backEnd.viewParms.viewportHeight;

	if (srcFbo)
	{
		//ALLOW_NULL_FBO_BIND = qfalse;

		DEBUG_StartTimer("Initial blit", qtrue);
		FBO_FastBlit(tr.renderFbo, NULL, tr.genericFbo3, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		DEBUG_EndTimer(qtrue);

		FBO_t *currentFbo = tr.genericFbo3;
		FBO_t *currentOutFbo = tr.genericFbo;

		if (!SCREEN_BLUR && ENABLE_DISPLACEMENT_MAPPING && r_ssdm->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("SSDM Generate", qtrue);
				RB_SSDM_Generate(currentFbo, srcBox, currentOutFbo, dstBox);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_ao->integer >= 2.0)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("SSAO Generate", qtrue);
				RB_SSAO(currentFbo, srcBox, currentOutFbo, dstBox);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (r_cartoon->integer >= 2.0)
		{
			DEBUG_StartTimer("Cell Shade", qtrue);
			RB_CellShade(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (r_cartoon->integer >= 3.0)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Paint", qtrue);
				RB_Paint(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		/*if (!SCREEN_BLUR && r_ssdo->integer && AO_DIRECTIONAL)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("SSDO", qtrue);

				RB_SSDO(currentFbo, srcBox, currentOutFbo, dstBox);

				if (r_ssdo->integer == 3)
					RB_SwapFBOs(&currentFbo, &currentOutFbo);

				DEBUG_EndTimer(qtrue);
			}
		}*/

		/*if (!SCREEN_BLUR && r_sss->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("SSS", qtrue);

				RB_SSS(currentFbo, srcBox, currentOutFbo, dstBox);

				//if (r_sss->integer == 3)
				RB_SwapFBOs(&currentFbo, &currentOutFbo);

				DEBUG_EndTimer(qtrue);
			}
		}*/

		if (!SCREEN_BLUR && r_anamorphic->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Create Anamorphic", qtrue);
				RB_CreateAnamorphicImage();
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_deferredLighting->integer && !LATE_LIGHTING_ENABLED)
		{
			DEBUG_StartTimer("Deferred Lighting", qtrue);
			RB_DeferredLighting(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		/*if (!SCREEN_BLUR && (r_ssr->value > 0.0 || r_sse->value > 0.0))
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("SSR", qtrue);
				RB_ScreenSpaceReflections(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}*/

		/*if (r_underwater->integer && (backEnd.refdef.rdflags & RDF_UNDERWATER))
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Underwater", qtrue);
				RB_Underwater(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs( &currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}*/

		if (!SCREEN_BLUR && r_magicdetail->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Magic Detail", qtrue);
				RB_MagicDetail(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (SCREEN_BLUR && r_screenBlurSlow->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Screen Blur Slow", qtrue);

				// Blur some times
				float	spread = 1.0f;
				int		numPasses = 8;

				for (int i = 0; i < numPasses; i++)
				{
					RB_GaussianBlur(currentFbo, tr.genericFbo2, currentOutFbo, spread);
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
					spread += 0.6f * 0.25f;
				}

				DEBUG_EndTimer(qtrue);
			}
		}

		if (SCREEN_BLUR && r_screenBlurFast->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Screen Blur Fast", qtrue);
				RB_FastBlur(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		/*if (!SCREEN_BLUR && r_hbao->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("HBAO", qtrue);
				RB_HBAO(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}*/

		if (!SCREEN_BLUR && ENABLE_DISPLACEMENT_MAPPING && r_ssdm->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("SSDM", qtrue);
				RB_SSDM(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_glslWater->integer && WATER_ENABLED)
		{
			DEBUG_StartTimer("Water Post", qtrue);
			RB_WaterPost(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && FOG_POST_ENABLED && r_fogPost->integer && !LATE_LIGHTING_ENABLED)
		{
			DEBUG_StartTimer("Fog Post", qtrue);
			RB_FogPostShader(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_multipost->integer)
		{
			DEBUG_StartTimer("Multi Post", qtrue);
			RB_MultiPost(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs( &currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_dof->integer)
		{
			if (!r_lowVram->integer)
			{
				RB_DofFocusDepth();

				DEBUG_StartTimer("DOF", qtrue);
				RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 2);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 3);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 0);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				RB_DOF(currentFbo, srcBox, currentOutFbo, dstBox, 1);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

#if 0
		if (!SCREEN_BLUR && r_lensflare->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Lens Flare", qtrue);
				RB_LensFlare(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}
#endif

		if (!SCREEN_BLUR && r_testshader->integer)
		{
			DEBUG_StartTimer("Test Shader", qtrue);
			RB_TestShader(currentFbo, srcBox, currentOutFbo, dstBox, 0);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (r_colorCorrection->integer)
		{
			DEBUG_StartTimer("Color Correction", qtrue);
			RB_ColorCorrection(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_deferredLighting->integer && LATE_LIGHTING_ENABLED == 1)
		{
			DEBUG_StartTimer("Deferred Lighting", qtrue);
			RB_DeferredLighting(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}
		
		if (!SCREEN_BLUR && FOG_POST_ENABLED && r_fogPost->integer && LATE_LIGHTING_ENABLED)
		{
			DEBUG_StartTimer("Fog Post", qtrue);
			RB_FogPostShader(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_deferredLighting->integer && LATE_LIGHTING_ENABLED >= 2)
		{
			DEBUG_StartTimer("Deferred Lighting", qtrue);
			RB_DeferredLighting(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_esharpening->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("eSharpen", qtrue);
				RB_ESharpening(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		/*if (!SCREEN_BLUR && r_esharpening2->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("eSharpen2", qtrue);
				RB_ESharpening2(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}*/

		if (!SCREEN_BLUR && r_darkexpand->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Dark Expand", qtrue);
				for (int pass = 0; pass < 2; pass++)
				{
					RB_DarkExpand(currentFbo, srcBox, currentOutFbo, dstBox);
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
				}
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_distanceBlur->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Distance Blur", qtrue);
				if (r_distanceBlur->integer >= 2)
				{// New HQ matso blur versions...
					RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 2);
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
					RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 3);
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
					RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 0);
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
					RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, 1);
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
				}
				else
				{
					RB_DistanceBlur(currentFbo, srcBox, currentOutFbo, dstBox, -1);
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
				}
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_dynamicGlow->integer)
		{
			DEBUG_StartTimer("Dynamic Glow Draw", qtrue);

			// Composite the glow/bloom texture
			int blendFunc = 0;
			vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };

			if (r_dynamicGlow->integer == 2)
			{
				// Debug output
				blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO;
			}
			else if (r_dynamicGlowSoft->integer)
			{
				blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
				color[0] = color[1] = color[2] = r_dynamicGlowIntensity->value;
			}
			else
			{
				blendFunc = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
				color[0] = color[1] = color[2] = r_dynamicGlowIntensity->value;
			}

			FBO_BlitFromTexture(tr.glowFboScaled[0]->colorImage[0], srcBox, NULL, currentFbo, NULL, NULL, color, blendFunc);

			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && (r_bloom->integer == 1 && !r_lowVram->integer))
		{
			DEBUG_StartTimer("Bloom", qtrue);
			RB_Bloom(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_anamorphic->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Anamorphic", qtrue);
				RB_Anamorphic(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_dynamiclight->integer && r_volumeLight->integer)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Volumetric Light Generate", qtrue);
				RB_GenerateVolumeLightImage();
				DEBUG_EndTimer(qtrue);

				DEBUG_StartTimer("Volumetric Light Combine", qtrue);
				if (RB_VolumetricLight(currentFbo, srcBox, currentOutFbo, dstBox))
					RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_bloom->integer >= 2)
		{
			if (!r_lowVram->integer)
			{
				DEBUG_StartTimer("Bloom Rays", qtrue);
				RB_BloomRays(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR)
		{
			DEBUG_StartTimer("Transparancy Post", qtrue);
			RB_TransparancyPost(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_fxaa->integer)
		{
			for (int pass = 0; pass < r_fxaa->integer; pass++)
			{
				DEBUG_StartTimer("FXAA", qtrue);
				RB_FXAA(currentFbo, srcBox, currentOutFbo, dstBox);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				DEBUG_EndTimer(qtrue);
			}
		}

		if (!SCREEN_BLUR && r_txaa->integer)
		{
			DEBUG_StartTimer("TXAA", qtrue);
			RB_TXAA(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_showdepth->integer)
		{
			DEBUG_StartTimer("Show Depth", qtrue);
			RB_ShowDepth(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (!SCREEN_BLUR && r_shownormals->integer)
		{
			DEBUG_StartTimer("Show Normals", qtrue);
			RB_ShowNormals(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		if (r_trueAnaglyph->integer)
		{
			DEBUG_StartTimer("Anaglyph", qtrue);
			RB_Anaglyph(currentFbo, srcBox, currentOutFbo, dstBox);
			RB_SwapFBOs(&currentFbo, &currentOutFbo);
			DEBUG_EndTimer(qtrue);
		}

		extern qboolean menuOpen;
		if (menuOpen)
		{
			FBO_BlitFromTexture(tr.renderGUIImage, srcBox, NULL, currentFbo, dstBox, NULL, NULL, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}

#if 0
		if (SCREEN_BLUR_MENU)
		{
			// Blur some times
			float	spread = 1.0f;
			int		numPasses = 8;

			DEBUG_StartTimer("Menu Blur", qtrue);
			for (int i = 0; i < numPasses; i++)
			{
				RB_GaussianBlur(currentFbo, tr.genericFbo2, currentOutFbo, spread);
				RB_SwapFBOs(&currentFbo, &currentOutFbo);
				spread += 0.6f * 0.25f;
			}
			DEBUG_EndTimer(qtrue);
		}
#endif

		ALLOW_NULL_FBO_BIND = qtrue;

		DEBUG_StartTimer("Final Blit", qtrue);
		FBO_FastBlit(currentFbo, NULL, srcFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		DEBUG_EndTimer(qtrue);

		DEBUG_StartTimer("OcclusionCulling", qtrue);
		RB_OcclusionCulling();
		DEBUG_EndTimer(qtrue);

		//FBO_Bind(srcFbo);

		//
		// End UQ1 Added...
		//

		DEBUG_StartTimer("HDR", qtrue);
		if (r_hdr->integer && (r_toneMap->integer || r_forceToneMap->integer))
		{
			autoExposure = (qboolean)(r_autoExposure->integer || r_forceAutoExposure->integer);
			RB_ToneMap(srcFbo, srcBox, NULL, dstBox, autoExposure);
		}
		else if (r_cameraExposure->value == 0.0f)
		{
			FBO_FastBlit(srcFbo, srcBox, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else
		{
			vec4_t color;

			color[0] =
			color[1] =
			color[2] = pow(2, r_cameraExposure->value); //exp2(r_cameraExposure->value);
			color[3] = 1.0f;

			FBO_Blit(srcFbo, srcBox, NULL, NULL, dstBox, NULL, color, 0);
		}

		DEBUG_EndTimer(qtrue);
	}

	//if (r_drawSunRays->integer)
	//	RB_SunRays(NULL, srcBox, NULL, dstBox);

	//if (backEnd.refdef.blurFactor > 0.0)
	//	RB_BokehBlur(NULL, srcBox, NULL, dstBox, backEnd.refdef.blurFactor);

#ifdef ___WARZONE_AWESOMIUM___
	if (srcFbo)
	{
		//DrawAwesomium( "https://www.youtube.com/watch?v=Nzq9epS2b1A", srcFbo );
		//DrawAwesomium( "http://www.google.com.au", srcFbo );
		//DrawAwesomium( "file://warzone/interface/Google.html", srcFbo );
		//DrawAwesomium( "data:text/html,<h1>Hello World</h1>", srcFbo );
	}
#endif //___WARZONE_AWESOMIUM___

	if (0 && r_sunlightMode->integer && SHADOWS_ENABLED)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 0, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.screenShadowImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.dofFocusDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.sunRaysImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (tr.refdef.num_dlights && r_shadows->integer == 3)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.shadowCubemaps[/* 0 */r_testvalue0->integer], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderPshadowsImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderFbo->colorImage[3], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0 && r_shadows->integer == 2)
	{
		ivec4_t dstBox;
		VectorSet4(dstBox, 512 + 0, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 128, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 256, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 384, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[3], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

#if 0
	if (r_cubeMapping->integer >= 1 && tr.numCubemaps && !r_lowVram->integer)
	{
		vec4i_t dstBox;
		int cubemapIndex = R_CubemapForPoint( backEnd.viewParms.ori.origin );

		if (cubemapIndex)
		{
			VectorSet4(dstBox, 0, glConfig.vidHeight - 256, 256, 256);
			//FBO_BlitFromTexture(tr.renderCubeImage, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
			FBO_BlitFromTexture(tr.realtimeCubemap/*tr.cubemaps[cubemapIndex - 1]*/, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
		}
	}
#endif

#if 0
	//if (backEnd.viewParms.flags & VPF_SKYCUBEDAY)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 0, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.skyCubeMap, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
	}

	//if (backEnd.viewParms.flags & VPF_SKYCUBENIGHT)
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.skyCubeMapNight, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
	}
#endif

#if 0
	{
		vec4i_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.waterReflectionRenderImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}
#endif

	if (r_superSampleMultiplier->value > 1.0)
	{
		VectorSet4(srcBox, 0, 0, glConfig.vidHeight * r_superSampleMultiplier->value, glConfig.vidHeight * r_superSampleMultiplier->value);
		VectorSet4(dstBox, glConfig.vidHeight * (r_superSampleMultiplier->value - 1.0), glConfig.vidHeight * (r_superSampleMultiplier->value - 1.0), glConfig.vidHeight, glConfig.vidHeight);
		FBO_FastBlit(srcFbo, srcBox, tr.genericFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		FBO_FastBlit(tr.genericFbo, dstBox, srcFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	backEnd.framePostProcessed = qtrue;

	return (const void *)(cmd + 1);
}

const void *RB_AwesomiumFrame(const void *data) {
	const awesomiumFrameCommand_t *cmd = (const awesomiumFrameCommand_t *)data;

	R_UpdateSubImage(tr.awesomiumuiImage, cmd->buffer, cmd->x, cmd->y, cmd->width, cmd->height);
	//Free buffer
	free(cmd->buffer);
	//
	FBO_BlitFromTexture(tr.awesomiumuiImage, NULL, NULL, tr.renderFbo, NULL, NULL, NULL, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO); // GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO

	return (const void *)(cmd + 1);

}

/*
====================
RB_ExecuteRenderCommands
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {
	int		t1, t2;

	t1 = ri->Milliseconds ();

#ifdef __PERFORMANCE_DEBUG_STARTUP__
	while ( 1 ) 
	{
		data = PADP(data, sizeof(void *));

		switch ( *(const int *)data ) 
		{
		case RC_SET_COLOR:
			DEBUG_StartTimer("RB_SetColor", qtrue);
			data = RB_SetColor( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_STRETCH_PIC:
			DEBUG_StartTimer("RB_StretchPic", qtrue);
			data = RB_StretchPic( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_ROTATE_PIC:
			DEBUG_StartTimer("RB_RotatePic", qtrue);
			data = RB_RotatePic( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_ROTATE_PIC2:
			DEBUG_StartTimer("RB_RotatePic2", qtrue);
			data = RB_RotatePic2( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_DRAW_SURFS:
			DEBUG_StartTimer("RB_DrawSurfs", qtrue);
			data = RB_DrawSurfs( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_DRAW_BUFFER:
			DEBUG_StartTimer("RB_DrawBuffer", qtrue);
			data = RB_DrawBuffer( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_SWAP_BUFFERS:
			// dont time this, since imgui itself is rendered in here, which confuses DEBUG_EndTimer a bit too much currently
			//DEBUG_StartTimer("RB_SwapBuffers", qtrue);
			data = RB_SwapBuffers( data );
			//DEBUG_EndTimer(qtrue);
			break;
		case RC_SCREENSHOT:
			DEBUG_StartTimer("RB_TakeScreenshotCmd", qtrue);
			data = RB_TakeScreenshotCmd( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_VIDEOFRAME:
			DEBUG_StartTimer("RB_TakeVideoFrameCmd", qtrue);
			data = RB_TakeVideoFrameCmd( data );
			DEBUG_EndTimer(qtrue);
			break;
		case RC_COLORMASK:
			DEBUG_StartTimer("RB_ColorMask", qtrue);
			data = RB_ColorMask(data);
			DEBUG_EndTimer(qtrue);
			break;
		case RC_CLEARDEPTH:
			DEBUG_StartTimer("RB_ClearDepth", qtrue);
			data = RB_ClearDepth(data);
			DEBUG_EndTimer(qtrue);
			break;
		case RC_CAPSHADOWMAP:
			DEBUG_StartTimer("RB_CapShadowMap", qtrue);
			data = RB_CapShadowMap(data);
			DEBUG_EndTimer(qtrue);
			break;
#ifdef __JKA_WEATHER__
		case RC_WORLD_EFFECTS:
			DEBUG_StartTimer("RB_WorldEffects", qtrue);
			data = RB_WorldEffects(data);
			DEBUG_EndTimer(qtrue);
			break;
#endif //__JKA_WEATHER__
		case RC_POSTPROCESS:
			//DEBUG_StartTimer("RB_PostProcess", qtrue); // this won't work. we time stuff inside here...
			data = RB_PostProcess(data);
			//DEBUG_EndTimer(qtrue);
			break;
		case RC_DRAWAWESOMIUMFRAME:
			DEBUG_StartTimer("RB_AwesomiumFrame", qtrue);
			data = RB_AwesomiumFrame(data);
			DEBUG_EndTimer(qtrue);
			break;
		case RC_END_OF_LIST:
		default:
			// finish any 2D drawing if needed
			if(tess.numIndexes) {
				DEBUG_StartTimer("RB_EndSurface", qtrue);
				RB_EndSurface();
				DEBUG_EndTimer(qtrue);
			}

			// stop rendering
			t2 = ri->Milliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
#else //!__PERFORMANCE_DEBUG_STARTUP__
	while (1) 
	{
		data = PADP(data, sizeof(void *));

		switch (*(const int *)data) 
		{
		case RC_SET_COLOR:
			data = RB_SetColor(data);
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic(data);
			break;
		case RC_ROTATE_PIC:
			data = RB_RotatePic(data);
			break;
		case RC_ROTATE_PIC2:
			data = RB_RotatePic2(data);
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs(data);
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer(data);
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd(data);
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd(data);
			break;
		case RC_COLORMASK:
			data = RB_ColorMask(data);
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth(data);
			break;
		case RC_CAPSHADOWMAP:
			data = RB_CapShadowMap(data);
			break;
	#ifdef __JKA_WEATHER__
		case RC_WORLD_EFFECTS:
			data = RB_WorldEffects(data);
			break;
	#endif //__JKA_WEATHER__
		case RC_POSTPROCESS:
			data = RB_PostProcess(data);
			break;
		case RC_DRAWAWESOMIUMFRAME:
			data = RB_AwesomiumFrame(data);
			break;
		case RC_END_OF_LIST:
		default:
			// finish any 2D drawing if needed
			if (tess.numIndexes) {
				RB_EndSurface();
			}

			// stop rendering
			t2 = ri->Milliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
#endif //__PERFORMANCE_DEBUG_STARTUP__
}
