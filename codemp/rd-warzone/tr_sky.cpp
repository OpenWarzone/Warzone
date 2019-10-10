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
// tr_sky.c
#include "tr_local.h"

#define SKY_SUBDIVISIONS		8
#define HALF_SKY_SUBDIVISIONS	(SKY_SUBDIVISIONS/2)

static float s_cloudTexCoords[6][SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1][2];
static float s_cloudTexP[6][SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1];

extern qboolean AURORA_ENABLED;
extern qboolean AURORA_ENABLED_DAY;

/*
===================================================================================

POLYGON TO BOX SIDE PROJECTION

===================================================================================
*/

static vec3_t sky_clip[6] = 
{
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1} 
};

static float	sky_mins[2][6], sky_maxs[2][6];
static float	sky_min, sky_max;

/*
================
AddSkyPolygon
================
*/
static void AddSkyPolygon (int nump, vec3_t vecs) 
{
	int		i,j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float	*vp;
	// s = [0]/[2], t = [1]/[2]
	static int	vec_to_st[6][3] =
	{
		{-2,3,1},
		{2,3,-1},

		{1,3,2},
		{-1,3,-2},

		{-2,-1,3},
		{-2,1,-3}

	//	{-1,2,3},
	//	{1,2,-3}
	};

	// decide which face it maps to
	VectorCopy (vec3_origin, v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
	{
		VectorAdd (vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];
		if (dv < 0.001)
			continue;	// don't divide by zero
		j = vec_to_st[axis][0];
		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j-1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j-1] / dv;

		if (s < sky_mins[0][axis])
			sky_mins[0][axis] = s;
		if (t < sky_mins[1][axis])
			sky_mins[1][axis] = t;
		if (s > sky_maxs[0][axis])
			sky_maxs[0][axis] = s;
		if (t > sky_maxs[1][axis])
			sky_maxs[1][axis] = t;
	}
}

#define	ON_EPSILON		0.1f			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64
/*
================
ClipSkyPolygon
================
*/
static void ClipSkyPolygon (int nump, vec3_t vecs, int stage) 
{
	float	*norm;
	float	*v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		ri->Error (ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6)
	{	// fully clipped, so draw it
		AddSkyPolygon (nump, vecs);
		return;
	}

	front = back = qfalse;
	norm = sky_clip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = qtrue;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = qtrue;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v[j] + d*(v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}

/*
==============
ClearSkyBox
==============
*/
static void ClearSkyBox (void) {
	int		i;

	for (i=0 ; i<6 ; i++) {
		sky_mins[0][i] = sky_mins[1][i] = 9999;
		sky_maxs[0][i] = sky_maxs[1][i] = -9999;
	}
}

/*
================
RB_ClipSkyPolygons
================
*/
void RB_ClipSkyPolygons( shaderCommands_t *input )
{
	vec3_t		p[5];	// need one extra point for clipping
	int			i, j;

	ClearSkyBox();

	for ( i = 0; i < input->numIndexes; i += 3 )
	{
		for (j = 0 ; j < 3 ; j++) 
		{
			VectorSubtract( input->xyz[input->indexes[i+j]],
							backEnd.viewParms.ori.origin, 
							p[j] );
		}
		ClipSkyPolygon( 3, p[0], 0 );
	}
}

/*
===================================================================================

CLOUD VERTEX GENERATION

===================================================================================
*/

/*
** MakeSkyVec
**
** Parms: s, t range from -1 to 1
*/
static void MakeSkyVec( float s, float t, int axis, float outSt[2], vec3_t outXYZ )
{
	// 1 = s, 2 = t, 3 = 2048
	static int	st_to_vec[6][3] =
	{
		{3,-1,2},
		{-3,1,2},

		{1,3,2},
		{-1,-3,2},

		{-2,-1,3},		// 0 degrees yaw, look straight up
		{2,-1,-3}		// look straight down
	};

	vec3_t		b;
	int			j, k;
	float	boxSize;

	boxSize = backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
	b[0] = s*boxSize;
	b[1] = t*boxSize;
	b[2] = boxSize;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
		{
			outXYZ[j] = -b[-k - 1];
		}
		else
		{
			outXYZ[j] = b[k - 1];
		}
	}

	// avoid bilerp seam
	s = (s+1)*0.5;
	t = (t+1)*0.5;
	if (s < sky_min)
	{
		s = sky_min;
	}
	else if (s > sky_max)
	{
		s = sky_max;
	}

	if (t < sky_min)
	{
		t = sky_min;
	}
	else if (t > sky_max)
	{
		t = sky_max;
	}

	t = 1.0 - t;


	if ( outSt )
	{
		outSt[0] = s;
		outSt[1] = t;
	}
}

static vec3_t	s_skyPoints[SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1];
static float	s_skyTexCoords[SKY_SUBDIVISIONS+1][SKY_SUBDIVISIONS+1][2];

qboolean MOON_INFO_CHANGED = qfalse;

static void DrawSkySide( struct image_s *image, struct image_s *nightImage, const int skyDirection, const int mins[2], const int maxs[2] )
{
	if (backEnd.viewParms.flags & VPF_EMISSIVEMAP)
	{
		return;
	}

	tess.numIndexes = 0;
	tess.firstIndex = 0;
	tess.numVertexes = 0;
	tess.minIndex = 0;
	tess.maxIndex = 0;


	int s, t;
	int firstVertex = tess.numVertexes;
	//int firstIndex = tess.numIndexes;
	int minIndex = tess.minIndex;
	int maxIndex = tess.maxIndex;
	vec4_t color;

	//tess.numVertexes = 0;
	//tess.numIndexes = 0;
	tess.firstIndex = tess.numIndexes;

	extern qboolean		PROCEDURAL_SKY_ENABLED;
	extern vec3_t		PROCEDURAL_SKY_DAY_COLOR;
	extern vec4_t		PROCEDURAL_SKY_SUNSET_COLOR;
	extern vec4_t		PROCEDURAL_SKY_NIGHT_COLOR;
	extern float		PROCEDURAL_SKY_NIGHT_HDR_MIN;
	extern float		PROCEDURAL_SKY_NIGHT_HDR_MAX;
	extern int			PROCEDURAL_SKY_STAR_DENSITY;
	extern float		PROCEDURAL_SKY_NEBULA_FACTOR;
	extern float		PROCEDURAL_SKY_NEBULA_SEED;
	extern float		PROCEDURAL_SKY_PLANETARY_ROTATION;

	extern qboolean		PROCEDURAL_BACKGROUND_HILLS_ENABLED;
	extern float		PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS;
	extern float		PROCEDURAL_BACKGROUND_HILLS_UPDOWN;
	extern float		PROCEDURAL_BACKGROUND_HILLS_SEED;
	extern vec3_t		PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR;
	extern vec3_t		PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2;

	extern vec3_t		AURORA_COLOR;

	extern qboolean		PROCEDURAL_CLOUDS_ENABLED;
	extern qboolean		PROCEDURAL_CLOUDS_LAYER;
	extern float		PROCEDURAL_CLOUDS_CLOUDSCALE;
	extern float		PROCEDURAL_CLOUDS_SPEED;
	extern float		PROCEDURAL_CLOUDS_DARK;
	extern float		PROCEDURAL_CLOUDS_LIGHT;
	extern float		PROCEDURAL_CLOUDS_CLOUDCOVER;
	extern float		PROCEDURAL_CLOUDS_CLOUDALPHA;
	extern float		PROCEDURAL_CLOUDS_SKYTINT;

	extern float		DYNAMIC_WEATHER_CLOUDCOVER;
	extern float		DYNAMIC_WEATHER_CLOUDSCALE;
	
	GL_Bind(PROCEDURAL_SKY_ENABLED ? tr.whiteImage : image );
	GL_Cull( CT_TWO_SIDED );

	for ( t = mins[1]+HALF_SKY_SUBDIVISIONS; t <= maxs[1]+HALF_SKY_SUBDIVISIONS; t++ )
	{
		for ( s = mins[0]+HALF_SKY_SUBDIVISIONS; s <= maxs[0]+HALF_SKY_SUBDIVISIONS; s++ )
		{
			tess.xyz[tess.numVertexes][0] = s_skyPoints[t][s][0];
			tess.xyz[tess.numVertexes][1] = s_skyPoints[t][s][1];
			tess.xyz[tess.numVertexes][2] = s_skyPoints[t][s][2];
			tess.xyz[tess.numVertexes][3] = 1.0;

			tess.texCoords[tess.numVertexes][0][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][0][1] = s_skyTexCoords[t][s][1];
			
			tess.normal[tess.numVertexes] = R_TessXYZtoPackedNormals(tess.xyz[tess.numVertexes]);

			tess.numVertexes++;

			if(tess.numVertexes >= SHADER_MAX_VERTEXES)
			{
				ri->Error(ERR_DROP, "SHADER_MAX_VERTEXES hit in DrawSkySideVBO()");
			}
		}
	}

	for ( t = 0; t < maxs[1] - mins[1]; t++ )
	{
		for ( s = 0; s < maxs[0] - mins[0]; s++ )
		{
			if (tess.numIndexes + 6 >= SHADER_MAX_INDEXES)
			{
				ri->Error(ERR_DROP, "SHADER_MAX_INDEXES hit in DrawSkySideVBO()");
			}

			tess.indexes[tess.numIndexes++] =  s +       t      * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] =  s +      (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = (s + 1) +  t      * (maxs[0] - mins[0] + 1) + firstVertex;

			tess.indexes[tess.numIndexes++] = (s + 1) +  t      * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] =  s +      (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
			tess.indexes[tess.numIndexes++] = (s + 1) + (t + 1) * (maxs[0] - mins[0] + 1) + firstVertex;
		}
	}

	tess.minIndex = firstVertex;
	tess.maxIndex = tess.numVertexes;


	// FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
	RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);

	if (backEnd.depthFill)
	{
		shaderProgram_t *sp = &tr.shadowFillShader[0];
		vec4_t vector;

		//RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
		GLSL_BindProgram(sp);

		{// unused...
			VectorSet4(vector, 0.0, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS0, vector); // useTC, useDeform, useRGBA, isTextureClamped
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vector); // useVertexAnim, useSkeletalAnim, blendMode, is2D
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS2, vector); // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS3, vector); // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL, 0=DetailMapNormal 1=detailMapFromTC 2=detailMapFromWorld, 0.0
		}


		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);


		if (backEnd.viewParms.flags & VPF_EMISSIVEMAP)
		{
			color[0] =
				color[1] =
				color[2] = 0.0;
			color[3] = 1.0f;
			GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, color);
		}
		else
		{
			color[0] =
				color[1] =
				color[2] = backEnd.refdef.colorScale;
			color[3] = 1.0f;
			GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, color);
		}

		color[0] =
			color[1] =
			color[2] =
			color[3] = 0.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, color);

		VectorSet4(vector, 1.0, 0.0, 0.0, 1.0);
		GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, vector);

		VectorSet4(vector, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, vector);

		vec2_t scale;
		scale[0] = scale[1] = 1.0;
		GLSL_SetUniformVec2(sp, UNIFORM_TEXTURESCALE, scale);

		GLSL_SetUniformFloat(sp, UNIFORM_TIME, tr.refdef.floatTime);

#ifdef __CHEAP_VERTS__
		GLSL_SetUniformInt(sp, UNIFORM_WORLD, 1);
#endif //__CHEAP_VERTS__
	}
	else
	{
		int quality = max(min(r_cloudQuality->integer, 4), 0);
		shaderProgram_t *sp = &tr.skyShader[quality];
		vec4_t vector;

		//FBO_Bind(tr.renderFbo);

		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);
		//GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK_TRUE | GLS_DEPTHTEST_DISABLE);
		//qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		//qglDepthMask(GL_TRUE);

		//RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
		//GLSL_VertexAttribPointers(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
		GLSL_BindProgram(sp);

		{// used...
			//extern float		MAP_WATER_LEVEL;// = 131072.0;
			extern qboolean		PROCEDURAL_SKY_ENABLED;
			extern float		DAY_NIGHT_24H_TIME;

			float dayNight24 = DAY_NIGHT_24H_TIME / 24.0;

			if (tr.viewParms.flags & VPF_SKYCUBEDAY)
				dayNight24 = 12.0;
			else if (tr.viewParms.flags & VPF_SKYCUBENIGHT)
				dayNight24 = 0.0;

			VectorSet4(vector, PROCEDURAL_SKY_ENABLED ? 1.0 : 0.0, dayNight24, PROCEDURAL_SKY_STAR_DENSITY, PROCEDURAL_SKY_NEBULA_SEED);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL1, vector); // 0.0, 0.0, 0.0, PROCEDURAL_SKY_NEBULA_SEED

			VectorSet4(vector, PROCEDURAL_CLOUDS_ENABLED ? 1.0 : 0.0, DYNAMIC_WEATHER_CLOUDSCALE, DYNAMIC_WEATHER_CLOUDCOVER, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL2, vector);

			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL3, PROCEDURAL_SKY_SUNSET_COLOR);

			VectorSet4(vector, PROCEDURAL_SKY_NIGHT_HDR_MIN, PROCEDURAL_SKY_NIGHT_HDR_MAX, PROCEDURAL_SKY_PLANETARY_ROTATION, Q_clamp(0.0, 1.0 - PROCEDURAL_SKY_NEBULA_FACTOR, 1.0));
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL4, vector);

			float auroraEnabled = 0.0;

			if (AURORA_ENABLED && AURORA_ENABLED_DAY)
				auroraEnabled = 2.0;
			else if (AURORA_ENABLED)
				auroraEnabled = 1.0;

			float nightScale = RB_NightScale();
			
			if (tr.viewParms.flags & VPF_SKYCUBEDAY)
				nightScale = 0.0;
			else if (tr.viewParms.flags & VPF_SKYCUBENIGHT)
				nightScale = 1.0;

			VectorSet4(vector, DAY_NIGHT_CYCLE_ENABLED ? 1.0 : 0.0, nightScale, skyDirection, auroraEnabled);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL5, vector); // dayNightEnabled, nightScale, skyDirection, auroraEnabled

			VectorSet4(vector, PROCEDURAL_SKY_DAY_COLOR[0], PROCEDURAL_SKY_DAY_COLOR[1], PROCEDURAL_SKY_DAY_COLOR[2], 0.0 /* UNUSED */);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL6, vector);
			
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL7, PROCEDURAL_SKY_NIGHT_COLOR);

			VectorSet4(vector, AURORA_COLOR[0], AURORA_COLOR[1], AURORA_COLOR[2], 0.0 /*UNUSED*/);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL8, vector);

			VectorSet4(vector, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL9, vector); // testvalues

			VectorSet4(vector, PROCEDURAL_BACKGROUND_HILLS_ENABLED, PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS, PROCEDURAL_BACKGROUND_HILLS_UPDOWN, PROCEDURAL_BACKGROUND_HILLS_SEED);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL10, vector);

			VectorSet4(vector, PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR[0], PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR[1], PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL11, vector);

			VectorSet4(vector, PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2[0], PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2[1], PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2[2], 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_LOCAL12, vector);


			if (MOON_INFO_CHANGED)
			{
				GLSL_SetUniformInt(sp, UNIFORM_MOON_COUNT, MOON_COUNT);

				vec4_t	moonInfos[4];
				vec2_t	moonInfos2[4];
				int		moonBundles[4] = { TB_MOONMAP1 };

				for (int i = 0; i < MOON_COUNT; i++)
				{
					moonInfos[i][0] = MOON_ENABLED[i] ? 1.0 : 0.0;
					moonInfos[i][1] = MOON_ROTATION_OFFSET_X[i];
					moonInfos[i][2] = MOON_ROTATION_OFFSET_Y[i];
					moonInfos[i][3] = MOON_SIZE[i];

					moonInfos2[i][0] = MOON_BRIGHTNESS[i];
					moonInfos2[i][1] = MOON_TEXTURE_SCALE[i];

					if (!sp->isBindless)
					{
						moonBundles[i] = TB_MOONMAP1 + i;
					}
				}

				GLSL_SetUniformVec4xX(sp, UNIFORM_MOON_INFOS, moonInfos, MOON_COUNT);
				GLSL_SetUniformVec2xX(sp, UNIFORM_MOON_INFOS2, moonInfos2, MOON_COUNT);

				if (sp->isBindless)
				{
					for (int i = 0; i < MOON_COUNT; i++)
					{
						GLSL_SetBindlessTexture(sp, UNIFORM_MOONMAPS, &tr.moonImage[i], i);
					}
				}
				else
				{
					GLSL_SetUniformIntxX(sp, UNIFORM_MOONMAPS, moonBundles, MOON_COUNT);

					for (int i = 0; i < MOON_COUNT; i++)
					{
						GL_BindToTMU(tr.moonImage[i], TB_MOONMAP1 + i);
					}
				}

				MOON_INFO_CHANGED = qfalse;
			}
		}


		{// unused...
			VectorSet4(vector, 0.0, 0.0, 0.0, 0.0);
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS0, vector); // useTC, useDeform, useRGBA, isTextureClamped
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS1, vector); // useVertexAnim, useSkeletalAnim, blendMode, is2D
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS2, vector); // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
			GLSL_SetUniformVec4(sp, UNIFORM_SETTINGS3, vector); // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL, 0=DetailMapNormal 1=detailMapFromTC 2=detailMapFromWorld, 0.0
		}
		

		GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		//GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, backEnd.ori.modelMatrix);


		if (backEnd.viewParms.flags & VPF_EMISSIVEMAP)
		{
			color[0] =
			color[1] =
			color[2] = 0.0;
			color[3] = 1.0f;
			GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, color);
		}
		else
		{
			color[0] =
			color[1] =
			color[2] = backEnd.refdef.colorScale;
			color[3] = 1.0f;
			GLSL_SetUniformVec4(sp, UNIFORM_BASECOLOR, color);
		}

		color[0] = 
		color[1] = 
		color[2] = 
		color[3] = 0.0f;
		GLSL_SetUniformVec4(sp, UNIFORM_VERTCOLOR, color);

		VectorSet4(vector, 1.0, 0.0, 0.0, 1.0);
		GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXMATRIX, vector);

		VectorSet4(vector, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(sp, UNIFORM_DIFFUSETEXOFFTURB, vector);

		vec2_t scale;
		scale[0] = scale[1] = 1.0;
		GLSL_SetUniformVec2(sp, UNIFORM_TEXTURESCALE, scale);

		GLSL_SetUniformFloat(sp, UNIFORM_TIME, tr.refdef.floatTime);
		
		if (sp->isBindless)
		{
			GLSL_SetBindlessTexture(sp, UNIFORM_OVERLAYMAP, &nightImage, 0);
			GLSL_SetBindlessTexture(sp, UNIFORM_SPLATMAP1, &tr.auroraImage[0], 0);
			GLSL_SetBindlessTexture(sp, UNIFORM_SPLATMAP2, &tr.auroraImage[1], 0);
			GLSL_SetBindlessTexture(sp, UNIFORM_SPLATMAP3, &tr.defaultSplatControlImage, 0);
			GLSL_SetBindlessTexture(sp, UNIFORM_ROADMAP, &tr.random2KImage[0], 0);
		}
		else
		{
			GLSL_SetUniformInt(sp, UNIFORM_OVERLAYMAP, TB_OVERLAYMAP);
			GL_BindToTMU(nightImage, TB_OVERLAYMAP);
		
			GLSL_SetUniformInt(sp, UNIFORM_SPLATMAP1, TB_SPLATMAP1);
			GL_BindToTMU(tr.auroraImage[0], TB_SPLATMAP1);

			GLSL_SetUniformInt(sp, UNIFORM_SPLATMAP2, TB_SPLATMAP2);
			GL_BindToTMU(tr.auroraImage[1], TB_SPLATMAP2);

			GLSL_SetUniformInt(sp, UNIFORM_SPLATMAP3, TB_SPLATMAP3);
			GL_BindToTMU(tr.defaultSplatControlImage, TB_SPLATMAP3);

			GLSL_SetUniformInt(sp, UNIFORM_ROADMAP, TB_ROADMAP);
			GL_BindToTMU(tr.random2KImage[0], TB_ROADMAP);
		}

		//if (r_testvalue0->integer)
			GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);
		//else
		//	GLSL_SetUniformVec3(sp, UNIFORM_VIEWORIGIN, backEnd.viewParms.ori.origin);

		//vec3_t out;
		
		if (tr.viewParms.flags & VPF_SKYCUBEDAY)
		{
			vec4_t sunDir;
			VectorSet(sunDir, 0.0, 0.0, 1.0);
			GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.refdef.sunDir);
		}
		else if (tr.viewParms.flags & VPF_SKYCUBENIGHT)
		{
			vec4_t sunDir;
			VectorSet(sunDir, 0.0, 0.0, -1.0);
			GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.refdef.sunDir);
		}
		else
		{
			GLSL_SetUniformVec4(sp, UNIFORM_PRIMARYLIGHTORIGIN, backEnd.refdef.sunDir);
		}

		GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
		GLSL_SetUniformVec3(sp, UNIFORM_PRIMARYLIGHTCOLOR, backEnd.refdef.sunCol);

		vec2_t screensize;
		screensize[0] = nightImage->width;
		screensize[1] = nightImage->height;
		GLSL_SetUniformVec2(sp, UNIFORM_DIMENSIONS, screensize);

#ifdef __CHEAP_VERTS__
		GLSL_SetUniformInt(sp, UNIFORM_WORLD, 1);
#endif //__CHEAP_VERTS__

		if (sp->isBindless)
		{
			GLSL_BindlessUpdate(sp);
		}
	}

	backEnd.pc.c_skyDraws++;

	R_DrawElementsVBO(tess.numIndexes - tess.firstIndex, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);

	tess.numIndexes = tess.firstIndex;
	tess.numVertexes = firstVertex;
	tess.firstIndex = 0;
	tess.minIndex = minIndex;
	tess.maxIndex = maxIndex;
}

static void DrawSkyBox( shader_t *shader )
{
	int		i;

	tr.skyImageShader = shader; // Store sky shader for use with $skyimage - TODO: sky cubemap...

	sky_min = 0;
	sky_max = 1;

	Com_Memset( s_skyTexCoords, 0, sizeof( s_skyTexCoords ) );

	for (i=0 ; i<6 ; i++)
	{
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;

		sky_mins[0][i] = floor( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) ||
			 ( sky_mins[1][i] >= sky_maxs[1][i] ) )
		{
			continue;
		}

		sky_mins_subd[0] = sky_mins[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_mins_subd[1] = sky_mins[1][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[0] = sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[1] = sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS;

		if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS )
			sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

		if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							s_skyTexCoords[t][s], 
							s_skyPoints[t][s] );
			}
		}

		DrawSkySide( shader->sky.outerbox[i],
					shader->sky.outerboxnight[i],
					i,
					sky_mins_subd,
					sky_maxs_subd );
	}
}

static void FillCloudySkySide( const int mins[2], const int maxs[2], qboolean addIndexes )
{
	int s, t;
	int vertexStart = tess.numVertexes;
	int tHeight, sWidth;

	tHeight = maxs[1] - mins[1] + 1;
	sWidth = maxs[0] - mins[0] + 1;

	for ( t = mins[1]+HALF_SKY_SUBDIVISIONS; t <= maxs[1]+HALF_SKY_SUBDIVISIONS; t++ )
	{
		for ( s = mins[0]+HALF_SKY_SUBDIVISIONS; s <= maxs[0]+HALF_SKY_SUBDIVISIONS; s++ )
		{
			VectorAdd( s_skyPoints[t][s], backEnd.viewParms.ori.origin, tess.xyz[tess.numVertexes] );

			tess.normal[tess.numVertexes] = R_TessXYZtoPackedNormals(tess.xyz[tess.numVertexes]);

			tess.texCoords[tess.numVertexes][0][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][0][1] = s_skyTexCoords[t][s][1];

			tess.numVertexes++;

			if ( tess.numVertexes >= SHADER_MAX_VERTEXES )
			{
				ri->Error( ERR_DROP, "SHADER_MAX_VERTEXES hit in FillCloudySkySide()" );
			}
		}
	}

	// only add indexes for one pass, otherwise it would draw multiple times for each pass
	if ( addIndexes ) {
		for ( t = 0; t < tHeight-1; t++ )
		{	
			for ( s = 0; s < sWidth-1; s++ )
			{
				tess.indexes[tess.numIndexes] = vertexStart + s + t * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * ( sWidth );
				tess.numIndexes++;

				tess.indexes[tess.numIndexes] = vertexStart + s + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + ( t + 1 ) * ( sWidth );
				tess.numIndexes++;
				tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * ( sWidth );
				tess.numIndexes++;
			}
		}
	}
}

static void FillCloudBox( const shader_t *shader, int stage )
{
	int i;

	for ( i =0; i < 6; i++ )
	{
		int sky_mins_subd[2], sky_maxs_subd[2];
		int s, t;
		float MIN_T;

		if ( 1 ) // FIXME? shader->sky.fullClouds )
		{
			MIN_T = -HALF_SKY_SUBDIVISIONS;

			// still don't want to draw the bottom, even if fullClouds
			if ( i == 5 )
				continue;
		}
		else
		{
			switch( i )
			{
			case 0:
			case 1:
			case 2:
			case 3:
				MIN_T = -1;
				break;
			case 5:
				// don't draw clouds beneath you
				continue;
			case 4:		// top
			default:
				MIN_T = -HALF_SKY_SUBDIVISIONS;
				break;
			}
		}

		sky_mins[0][i] = floor( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

		if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) ||
			 ( sky_mins[1][i] >= sky_maxs[1][i] ) )
		{
			continue;
		}

		sky_mins_subd[0] = Q_ftol(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_mins_subd[1] = Q_ftol(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[0] = Q_ftol(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[1] = Q_ftol(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS);

		if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_mins_subd[1] < MIN_T )
			sky_mins_subd[1] = MIN_T;
		else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

		if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		if ( sky_maxs_subd[1] < MIN_T )
			sky_maxs_subd[1] = MIN_T;
		else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS ) 
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

		//
		// iterate through the subdivisions
		//
		for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
		{
			for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
			{
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							NULL,
							s_skyPoints[t][s] );

				s_skyTexCoords[t][s][0] = s_cloudTexCoords[i][t][s][0];
				s_skyTexCoords[t][s][1] = s_cloudTexCoords[i][t][s][1];
			}
		}

		// only add indexes for first stage
		FillCloudySkySide( sky_mins_subd, sky_maxs_subd, (qboolean)( stage == 0 ) );
	}
}

/*
** R_BuildCloudData
*/
void R_BuildCloudData( shaderCommands_t *input )
{
	int			i;
	shader_t	*shader;

	shader = input->shader;

	assert( shader->isSky );

	sky_min = 1.0 / 256.0f;		// FIXME: not correct?
	sky_max = 255.0 / 256.0f;

	// set up for drawing
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;

	if ( shader->sky.cloudHeight )
	{
		for ( i = 0; i < MAX_SHADER_STAGES; i++ )
		{
			if ( !tess.xstages[i] ) {
				break;
			}
			FillCloudBox( shader, i );
		}
	}
}

/*
** R_InitSkyTexCoords
** Called when a sky shader is parsed
*/
#define SQR( a ) ((a)*(a))
void R_InitSkyTexCoords( float heightCloud )
{
	int i, s, t;
	float radiusWorld = 4096;
	float p;
	float sRad, tRad;
	vec3_t skyVec;
	vec3_t v;

	// init zfar so MakeSkyVec works even though
	// a world hasn't been bounded
	backEnd.viewParms.zFar = 1024;

	for ( i = 0; i < 6; i++ )
	{
		for ( t = 0; t <= SKY_SUBDIVISIONS; t++ )
		{
			for ( s = 0; s <= SKY_SUBDIVISIONS; s++ )
			{
				// compute vector from view origin to sky side integral point
				MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
							i, 
							NULL,
							skyVec );

				// compute parametric value 'p' that intersects with cloud layer
				p = ( 1.0f / ( 2 * DotProduct( skyVec, skyVec ) ) ) *
					( -2 * skyVec[2] * radiusWorld + 
					   2 * sqrt( SQR( skyVec[2] ) * SQR( radiusWorld ) + 
					             2 * SQR( skyVec[0] ) * radiusWorld * heightCloud +
								 SQR( skyVec[0] ) * SQR( heightCloud ) + 
								 2 * SQR( skyVec[1] ) * radiusWorld * heightCloud +
								 SQR( skyVec[1] ) * SQR( heightCloud ) + 
								 2 * SQR( skyVec[2] ) * radiusWorld * heightCloud +
								 SQR( skyVec[2] ) * SQR( heightCloud ) ) );

				s_cloudTexP[i][t][s] = p;

				// compute intersection point based on p
				VectorScale( skyVec, p, v );
				v[2] += radiusWorld;

				// compute vector from world origin to intersection point 'v'
				VectorNormalize( v );

				sRad = Q_acos( v[0] );
				tRad = Q_acos( v[1] );

				s_cloudTexCoords[i][t][s][0] = sRad;
				s_cloudTexCoords[i][t][s][1] = tRad;
			}
		}
	}
}

//======================================================================================

vec3_t		SUN_POSITION;
vec2_t		SUN_SCREEN_POSITION;
qboolean	SUN_VISIBLE = qfalse;

#ifdef __DAY_NIGHT__
extern float DAY_NIGHT_CURRENT_TIME;
#endif //__DAY_NIGHT__

extern vec3_t VOLUMETRIC_ROOF;

extern qboolean RB_UpdateSunFlareVis(void);
extern qboolean Volumetric_Visible(vec3_t from, vec3_t to, qboolean isSun);
extern void Volumetric_RoofHeight(vec3_t from);
extern void TR_AxisToAngles(const vec3_t axis[3], vec3_t angles);

void WorldCoordToScreenCoord2(vec3_t origin, float *x, float *y, qboolean *goodY)
{
	int	xcenter, ycenter;
	vec3_t	local, transformed;
	vec3_t	vfwd, vright, vup, viewAngles;

	TR_AxisToAngles(backEnd.refdef.viewaxis, viewAngles);

	//NOTE: did it this way because most draw functions expect virtual 640x480 coords
	//	and adjust them for current resolution
	xcenter = glConfig.vidWidth / 2;
	ycenter = glConfig.vidHeight / 2;

	VectorSubtract(origin, backEnd.refdef.vieworg, local);

	AngleVectors(viewAngles, vfwd, vright, vup);

	transformed[0] = DotProduct(local, vright);
	transformed[1] = DotProduct(local, vup);
	transformed[2] = DotProduct(local, vfwd);

	// Make sure Z is not negative.
	/*if(transformed[2] < 0.01)
	{
	transformed[2] *= -1.0;
	}*/


	// Simple convert to screen coords.
	float xzi = xcenter / transformed[2] * (95.0 / backEnd.refdef.fov_x);
	float yzi = ycenter / transformed[2] * (106.0 / backEnd.refdef.fov_y);

	*x = (xcenter + xzi * transformed[0]);
	*y = (ycenter - yzi * transformed[1]);

	*x = (*x / glConfig.vidWidth);
	*y = (*y / glConfig.vidHeight);
	*y = 1.0 - *y;
	/*if (*y >= 0.0 && *y <= 1.0)
	{
		*y = 1.0 - *y;
		*goodY = qtrue;
	}*/
}

void R_SetSunScreenPos(vec3_t sunDirection)
{
	qboolean goodY = qfalse;
	vec3_t pos;
	float dist = 4096.0;

	VectorScale(sunDirection, dist, pos);
	VectorCopy(pos, SUN_POSITION);
	VectorAdd(SUN_POSITION, backEnd.refdef.vieworg, SUN_POSITION);
	WorldCoordToScreenCoord2(SUN_POSITION, &SUN_SCREEN_POSITION[0], &SUN_SCREEN_POSITION[1], &goodY);
/*
	ri->Printf(PRINT_WARNING, "Sun screen pos is %f %f.\n", SUN_SCREEN_POSITION[0], SUN_SCREEN_POSITION[1]);

	if (SUN_SCREEN_POSITION[0] >= 0.0 && SUN_SCREEN_POSITION[0] <= 1.0 && SUN_SCREEN_POSITION[1] >= 0.0 && SUN_SCREEN_POSITION[1] <= 1.0)
	{// Fully on screen...
		return;
	}

	vec3_t sDirInv;
	vec2_t sPosInv;
	VectorCopy(sunDirection, sDirInv);
	sDirInv[0] = -sDirInv[0];
	sDirInv[1] = -sDirInv[1];
	VectorScale(sDirInv, dist, pos);
	VectorAdd(pos, backEnd.refdef.vieworg, pos);
	WorldCoordToScreenCoord2(pos, &sPosInv[0], &sPosInv[1], &goodY);

	ri->Printf(PRINT_WARNING, "Inv sun screen pos is %f %f.\n", sPosInv[0], sPosInv[1]);

	if (sPosInv[0] >= 0.0 && sPosInv[0] <= 1.0 && sPosInv[1] >= 0.0 && sPosInv[1] <= 1.0)
	{// Inv is fully on screen... (todo: need to pass this inv to glsl)
		SUN_SCREEN_POSITION[0] = sPosInv[0];
		SUN_SCREEN_POSITION[1] = sPosInv[1];
		return;
	}

	if (sPosInv[1] >= 0.0 && sPosInv[1] <= 1.0)
	{// if inv y is on screen then keep it...
		SUN_SCREEN_POSITION[1] = sPosInv[1];
	}
	
	if (SUN_SCREEN_POSITION[1] < 0.0)
	{// It's off screen and down?
		ri->Printf(PRINT_WARNING, "It's ???.\n");
	}
	
	if (SUN_SCREEN_POSITION[1] > 1.0)
	{// It's off screen and up
		ri->Printf(PRINT_WARNING, "It's ???2.\n");
	}
	
	if (SUN_SCREEN_POSITION[0] < 0.0)
	{// It's just off screen and to the left
		//SUN_SCREEN_POSITION[0] = 0.0;
		ri->Printf(PRINT_WARNING, "It's off screen to left.\n");
	}
	
	if (SUN_SCREEN_POSITION[0] > 1.0)
	{// It's just off screen and to the right
		//SUN_SCREEN_POSITION[0] = 1.0;
		ri->Printf(PRINT_WARNING, "It's off screen to right.\n");
	}
*/
}

/*
** RB_DrawSun
*/
void RB_DrawSun( float scale, shader_t *shader ) {
	if (tr.world != tr.worldSolid)
	{// Only render the solid world's sky...
		return;
	}

	float		size;
	float		dist;
	vec3_t		origin, vec1, vec2;

	if ( !backEnd.skyRenderedThisView ) {
		return;
	}

	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL))
	{// This fixes some skybox issues... For some wierd reason...
		if (!SKIP_CULL_FRAME_DONE)
		{
			SKIP_CULL_FRAME = qtrue;
			SKIP_CULL_FRAME_DONE = qtrue;
		}
	}

#ifdef __DAY_NIGHT__
	if (DAY_NIGHT_CYCLE_ENABLED)
	{
		if (RB_NightScale() >= 1.0)
		{
			SUN_VISIBLE = qfalse;
			return;
		}
	}
#endif //__DAY_NIGHT__

	{
		// FIXME: this could be a lot cleaner
		matrix_t translation, modelview;
		Matrix16Translation(backEnd.viewParms.ori.origin, translation);
		Matrix16Multiply(backEnd.viewParms.world.modelViewMatrix, translation, modelview);
		GL_SetModelviewMatrix(modelview);
	}

	//if (shader == tr.sunFlareShader)
	{// Now done in sky shader...
		dist = backEnd.viewParms.zFar / 1.75;
		size = dist * scale;

		if (r_proceduralSun->integer)
		{
			size *= r_proceduralSunScale->value;
		}

		VectorScale(tr.sunDirection, dist, origin);

		PerpendicularVector(vec1, tr.sunDirection);
		CrossProduct(tr.sunDirection, vec1, vec2);

		VectorScale(vec1, size, vec1);
		VectorScale(vec2, size, vec2);

		// farthest depth range
		GL_SetDepthRange(1.0, 1.0);

		RB_BeginSurface(shader, 0, 0);

		if (shader != tr.sunFlareShader)
		{
			vec4_t col;
			VectorSet4(col, 0, 0, 0, 0);
			RB_AddQuadStamp(origin, vec1, vec2, col);
		}
		else
		{
			RB_AddQuadStamp(origin, vec1, vec2, tr.refdef.sunAmbCol);
		}

		RB_EndSurface();

		// back to normal depth range
		GL_SetDepthRange(0.0, 1.0);
	}

	if (r_dynamiclight->integer)
	{// Lets have some volumetrics with that!
		const float cutoff = 0.25f;
		float dot = DotProduct(tr.sunDirection, backEnd.viewParms.ori.axis[0]);

		R_SetSunScreenPos(tr.sunDirection);

		if (dot < cutoff)
		{
			SUN_VISIBLE = qfalse;
			return;
		}

		if (!RB_UpdateSunFlareVis())
		{
			SUN_VISIBLE = qfalse;
			return;
		}

		SUN_VISIBLE = qtrue;
	}
}

/*
** RB_DrawMoon
*/
void RB_DrawMoon(float scale, shader_t *shader) {
	if (tr.world != tr.worldSolid)
	{// Only render the solid world's sky...
		return;
	}

#ifdef __DAY_NIGHT__
	float		size;
	float		dist;
	vec3_t		origin, vec1, vec2;

	if (!backEnd.skyRenderedThisView) {
		return;
	}

	if (DAY_NIGHT_CYCLE_ENABLED)
	{
		if (RB_NightScale() <= 0.0)
		{
			return;
		}
	}

	{
		// FIXME: this could be a lot cleaner
		matrix_t translation, modelview;
		Matrix16Translation(backEnd.viewParms.ori.origin, translation);
		Matrix16Multiply(backEnd.viewParms.world.modelViewMatrix, translation, modelview);
		GL_SetModelviewMatrix(modelview);
	}

	dist = backEnd.viewParms.zFar / 1.75;
	size = dist * scale;

	if (r_proceduralSun->integer)
	{
		size *= r_proceduralSunScale->value;
	}

	VectorScale(tr.moonDirection, dist, origin);

	PerpendicularVector(vec1, tr.moonDirection);
	CrossProduct(tr.moonDirection, vec1, vec2);

	VectorScale(vec1, size, vec1);
	VectorScale(vec2, size, vec2);

	// farthest depth range
	GL_SetDepthRange(1.0, 1.0);

	RB_BeginSurface(shader, 0, 0);

	if (shader != tr.sunFlareShader)
	{
		vec4_t col;
		VectorSet4(col, 0, 0, 0, 0);
		RB_AddQuadStamp(origin, vec1, vec2, col);
	}
	else
	{
		RB_AddQuadStamp(origin, vec1, vec2, colorWhite);
	}

	RB_EndSurface();

	// back to normal depth range
	GL_SetDepthRange(0.0, 1.0);
#endif //__DAY_NIGHT__
}

void DrawSkyDome ( shader_t *skyShader )
{
	/*vec4_t color;

	// bloom
	color[0] =
		color[1] =
		color[2] = pow(2, MAP_TONEMAP_CAMERAEXPOSURE);
	color[3] = 1.0f;*/

	GLSL_BindProgram(&tr.skyDomeShader);

	matrix_t invMvp, normalMatrix;

	Matrix16SimpleInverse(glState.modelviewProjection, invMvp);
	Matrix16SimpleInverse(glState.modelview, normalMatrix); // Whats a normal matrix with rend2???? I have no idea!

	GLSL_SetUniformMatrix16(&tr.skyDomeShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformMatrix16(&tr.skyDomeShader, UNIFORM_MODELVIEWMATRIX, glState.modelview);
	GLSL_SetUniformMatrix16(&tr.skyDomeShader, UNIFORM_INVPROJECTIONMATRIX, invMvp);
	GLSL_SetUniformMatrix16(&tr.skyDomeShader, UNIFORM_NORMALMATRIX, normalMatrix);

	GLSL_SetUniformFloat(&tr.skyDomeShader, UNIFORM_TIME, backEnd.refdef.floatTime);
	GLSL_SetUniformVec3(&tr.skyDomeShader, UNIFORM_VIEWORIGIN,  backEnd.refdef.vieworg);

	GLSL_SetUniformVec3(&tr.skyDomeShader, UNIFORM_PRIMARYLIGHTAMBIENT, backEnd.refdef.sunAmbCol);
	GLSL_SetUniformVec3(&tr.skyDomeShader, UNIFORM_PRIMARYLIGHTCOLOR,   backEnd.refdef.sunCol);

	vec3_t out;
	float dist = 4096.0;//backEnd.viewParms.zFar / 1.75;
	VectorMA(backEnd.refdef.vieworg, dist, backEnd.refdef.sunDir, out);
	GLSL_SetUniformVec4(&tr.skyDomeShader, UNIFORM_PRIMARYLIGHTORIGIN, out);

	vec4_t l0;
	VectorSet4(l0, r_testshaderValue1->value, r_testshaderValue2->value, r_testshaderValue3->value, r_testshaderValue4->value);
	GLSL_SetUniformVec4(&tr.skyDomeShader, UNIFORM_LOCAL0, l0);

	vec4_t l1;
	VectorSet4(l1, r_testshaderValue5->value, r_testshaderValue6->value, r_testshaderValue7->value, r_testshaderValue8->value);
	GLSL_SetUniformVec4(&tr.skyDomeShader, UNIFORM_LOCAL1, l1);

#ifdef __OLD_SKYDOME__
	GL_BindToTMU(skyShader->sky.outerbox[0], TB_LEVELSMAP);

	if (skyShader->sky.outerbox[0])
	{
		vec2_t screensize;
		screensize[0] = skyShader->sky.outerbox[0]->width;
		screensize[1] = skyShader->sky.outerbox[0]->height;

		GLSL_SetUniformVec2(&tr.skyDomeShader, UNIFORM_DIMENSIONS, screensize);

		vec4i_t		imageBox;
		imageBox[0] = 0;
		imageBox[1] = 0;
		imageBox[2] = skyShader->sky.outerbox[0]->width;
		imageBox[3] = skyShader->sky.outerbox[0]->height;

		vec4i_t		screenBox;
		screenBox[0] = 0;
		screenBox[1] = 0;
		screenBox[2] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screenBox[3] = glConfig.vidHeight * r_superSampleMultiplier->value;

		FBO_BlitFromTexture(skyShader->sky.outerbox[0], imageBox, NULL, glState.currentFBO, screenBox, &tr.skyDomeShader, NULL, 0);
	}
	else
	{
		vec4i_t		imageBox;
		imageBox[0] = 0;
		imageBox[1] = 0;
		imageBox[2] = tr.whiteImage->width;
		imageBox[3] = tr.whiteImage->height;

		vec4i_t		screenBox;
		screenBox[0] = 0;
		screenBox[1] = 0;
		screenBox[2] = glConfig.vidWidth * r_superSampleMultiplier->value;
		screenBox[3] = glConfig.vidHeight * r_superSampleMultiplier->value;

		FBO_BlitFromTexture(tr.whiteImage, imageBox, NULL, glState.currentFBO, screenBox, &tr.skyDomeShader, NULL, 0);
	}
#else //!__OLD_SKYDOME__

	// FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
	RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);


	image_t *tintImage = R_FindImageFile("textures/skydomes/default_tint", IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE);
	image_t *tint2Image = R_FindImageFile("textures/skydomes/default_tint2", IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE);
	image_t *sunImage = R_FindImageFile("textures/skydomes/default_sun", IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_GLOW);
	image_t *moonImage = R_FindImageFile("textures/skydomes/default_moon", IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_GLOW);
	image_t *clouds1Image = R_FindImageFile("textures/skydomes/default_clouds1", IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE);
	image_t *clouds2Image = R_FindImageFile("textures/skydomes/default_clouds2", IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE);

	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
	GL_BindToTMU(tintImage, TB_DIFFUSEMAP);
	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_STEEPMAP1, TB_STEEPMAP1);
	GL_BindToTMU(tint2Image, TB_STEEPMAP1);
	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_WATER_EDGE_MAP, TB_WATER_EDGE_MAP);
	GL_BindToTMU(sunImage, TB_WATER_EDGE_MAP);
	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_SPLATMAP1, TB_SPLATMAP1);
	GL_BindToTMU(moonImage, TB_SPLATMAP1);
	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_SPLATMAP2, TB_SPLATMAP2);
	GL_BindToTMU(clouds1Image, TB_SPLATMAP2);
	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_SPLATMAP3, TB_SPLATMAP3);
	GL_BindToTMU(clouds2Image, TB_SPLATMAP3);

	GLSL_SetUniformInt(&tr.skyDomeShader, UNIFORM_GLOWMAP, TB_GLOWMAP);
	GL_BindToTMU(tr.random2KImage[0], TB_GLOWMAP);

	vec2_t screensize;
	//screensize[0] = backEnd.viewParms.viewportWidth;
	//screensize[1] = backEnd.viewParms.viewportHeight;
	screensize[0] = tintImage->width;
	screensize[1] = tintImage->height;

	GLSL_SetUniformVec2(&tr.skyDomeShader, UNIFORM_DIMENSIONS, screensize);

	vec4i_t		imageBox;
	imageBox[0] = 0;
	imageBox[1] = 0;
	imageBox[2] = tr.whiteImage->width;
	imageBox[3] = tr.whiteImage->height;

	vec4i_t		screenBox;
	screenBox[0] = backEnd.viewParms.viewportX;
	screenBox[1] = backEnd.viewParms.viewportY;
	screenBox[2] = backEnd.viewParms.viewportWidth;
	screenBox[3] = backEnd.viewParms.viewportHeight;

	FBO_BlitFromTexture(tr.whiteImage, imageBox, NULL, glState.currentFBO, screenBox, &tr.skyDomeShader, colorWhite/*color*/, 0);
#endif //__OLD_SKYDOME__
}

/*
================
RB_StageIteratorSky

All of the visible sky triangles are in tess

Other things could be stuck in here, like birds in the sky, etc
================
*/

image_t *skyImage = NULL;

//#define ___FORCED_SKYDOME___

void RB_StageIteratorSky( void ) {
	if (tr.world != tr.worldSolid)
	{// Only render the solid world's sky...
		return;
	}

#ifndef ___FORCED_SKYDOME___
	if ( r_fastsky->integer ) {
		return;
	}

	/*if (backEnd.viewParms.flags & VPF_EMISSIVEMAP)
	{
		return;
	}*/

	// VOID REMOVE HACK
	//int clearBits = GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
	//qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
	//qglClear( clearBits );

	// r_showsky will let all the sky blocks be drawn in
	// front of everything to allow developers to see how
	// much sky is getting sucked in
	if (r_showsky->integer) {
		GL_SetDepthRange(0.0, 0.0);
	}
	else {
		GL_SetDepthRange(1.0, 1.0);
	}

	if (!tess.shader || !tess.shader->sky.outerbox[0] || tess.shader->sky.outerbox[0] == tr.defaultImage || r_skydome->integer)
	{// UQ1: Set a default image...
		shader_t *scarifSky = R_FindShader("textures/sky/scarif", lightmapsNone, stylesDefault, qtrue);
		tess.shader = scarifSky;

		extern qboolean		PROCEDURAL_SKY_ENABLED;
		extern vec3_t		PROCEDURAL_SKY_DAY_COLOR;
		extern vec4_t		PROCEDURAL_SKY_NIGHT_COLOR;
		extern float		PROCEDURAL_SKY_NIGHT_HDR_MIN;
		extern float		PROCEDURAL_SKY_NIGHT_HDR_MAX;
		extern int			PROCEDURAL_SKY_STAR_DENSITY;
		extern float		PROCEDURAL_SKY_NEBULA_FACTOR;
		extern float		PROCEDURAL_SKY_NEBULA_SEED;
		extern float		PROCEDURAL_SKY_PLANETARY_ROTATION;

		extern qboolean		PROCEDURAL_BACKGROUND_HILLS_ENABLED;
		extern float		PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS;
		extern float		PROCEDURAL_BACKGROUND_HILLS_UPDOWN;
		extern float		PROCEDURAL_BACKGROUND_HILLS_SEED;
		extern vec3_t		PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR;
		extern vec3_t		PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2;

		extern vec3_t		AURORA_COLOR;

		extern qboolean		PROCEDURAL_CLOUDS_ENABLED;
		extern qboolean		PROCEDURAL_CLOUDS_LAYER;
		extern qboolean		PROCEDURAL_CLOUDS_DYNAMIC;
		extern float		PROCEDURAL_CLOUDS_CLOUDSCALE;
		extern float		PROCEDURAL_CLOUDS_SPEED;
		extern float		PROCEDURAL_CLOUDS_DARK;
		extern float		PROCEDURAL_CLOUDS_LIGHT;
		extern float		PROCEDURAL_CLOUDS_CLOUDCOVER;
		extern float		PROCEDURAL_CLOUDS_CLOUDALPHA;
		extern float		PROCEDURAL_CLOUDS_SKYTINT;

		extern float		DYNAMIC_WEATHER_CLOUDCOVER;
		extern float		DYNAMIC_WEATHER_CLOUDSCALE;

		PROCEDURAL_SKY_ENABLED = qtrue;
		VectorSet(PROCEDURAL_SKY_DAY_COLOR, 0.2455, 0.58, 1.0);

		PROCEDURAL_SKY_STAR_DENSITY = 4;
		PROCEDURAL_SKY_NEBULA_FACTOR = 0.25;
		PROCEDURAL_SKY_NEBULA_SEED = 3.5;
		PROCEDURAL_SKY_PLANETARY_ROTATION = 0.9;
		VectorSet4(PROCEDURAL_SKY_NIGHT_COLOR, 1.0, 1.0, 1.0, 1.0);
		PROCEDURAL_SKY_NIGHT_HDR_MIN = 0.0;
		PROCEDURAL_SKY_NIGHT_HDR_MAX = 255.0;

		PROCEDURAL_BACKGROUND_HILLS_ENABLED = qtrue;
		PROCEDURAL_BACKGROUND_HILLS_SMOOTHNESS = 0.24;
		PROCEDURAL_BACKGROUND_HILLS_UPDOWN = 380.0;
		PROCEDURAL_BACKGROUND_HILLS_SEED = 1.0;
		VectorSet(PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR, 0.7, 0.7, 0.1);
		VectorSet(PROCEDURAL_BACKGROUND_HILLS_VEGETAION_COLOR2, 0.4, 0.7, 0.1);


		PROCEDURAL_CLOUDS_ENABLED = qtrue;
		PROCEDURAL_CLOUDS_LAYER = qfalse;
		PROCEDURAL_CLOUDS_DYNAMIC = qtrue;
		PROCEDURAL_CLOUDS_CLOUDSCALE = 1.0;
		PROCEDURAL_CLOUDS_SPEED = 0.003;
		PROCEDURAL_CLOUDS_DARK = 0.5;
		PROCEDURAL_CLOUDS_LIGHT = 0.3;
		PROCEDURAL_CLOUDS_CLOUDCOVER = 1.0;
		PROCEDURAL_CLOUDS_CLOUDALPHA = 5.0;
		PROCEDURAL_CLOUDS_SKYTINT = 0.5;


		extern qboolean	DAY_NIGHT_CYCLE_ENABLED;
		extern float		DAY_NIGHT_CYCLE_SPEED;
		extern float		DAY_NIGHT_START_TIME;
		extern float		SUN_PHONG_SCALE;
		extern float		SUN_VOLUMETRIC_SCALE;
		extern float		SUN_VOLUMETRIC_FALLOFF;
		extern vec3_t		SUN_COLOR_MAIN;
		extern vec3_t		SUN_COLOR_SECONDARY;
		extern vec3_t		SUN_COLOR_TERTIARY;
		extern vec3_t		SUN_COLOR_AMBIENT;

		DAY_NIGHT_CYCLE_ENABLED = qtrue;
		DAY_NIGHT_CYCLE_SPEED = 0.0;
		DAY_NIGHT_START_TIME = 10.32;
		SUN_PHONG_SCALE = 0.5;
		SUN_VOLUMETRIC_SCALE = 1.5;
		SUN_VOLUMETRIC_FALLOFF = 0.5;
		VectorSet(SUN_COLOR_MAIN, 1.0, 0.9, 0.825);
		VectorSet(SUN_COLOR_SECONDARY, 0.5, 0.4, 0.3);
		VectorSet(SUN_COLOR_TERTIARY, 0.3, 0.25, 0.2);
		VectorSet(SUN_COLOR_AMBIENT, 0.1, 0.08, 0.06);



		extern qboolean		MOON_ENABLED[8];
		extern float		MOON_SIZE[8];
		extern float		MOON_BRIGHTNESS[8];
		extern float		MOON_TEXTURE_SCALE[8];
		extern float		MOON_ROTATION_OFFSET_X[8];
		extern float		MOON_ROTATION_OFFSET_Y[8];

		MOON_ENABLED[0] = qtrue;
		MOON_SIZE[0] = 7.5;
		MOON_BRIGHTNESS[0] = 4.0;
		MOON_TEXTURE_SCALE[0] = 0.25;
		MOON_ROTATION_OFFSET_X[0] = 0.0;
		MOON_ROTATION_OFFSET_Y[0] = 0.7;
		tr.moonImage[0] = R_FindImageFile("gfx/moons/greenworld", IMGTYPE_COLORALPHA, IMGFLAG_NONE);
	}

	// go through all the polygons and project them onto
	// the sky box to see which blocks on each side need
	// to be drawn
	if (tess.shader->sky.outerbox[0] && tess.shader->sky.outerbox[0] != tr.defaultImage)
	{
		RB_ClipSkyPolygons(&tess);
	}

	/*if ( !tess.shader->sky.outerbox[0] || tess.shader->sky.outerbox[0] == tr.defaultImage || r_skydome->integer ) 
	{// UQ1: Use skydome...
		matrix_t oldmodelview;

		GL_State( 0 );

		matrix_t trans, product;
		Matrix16Copy(glState.modelview, oldmodelview);
		Matrix16Translation(backEnd.viewParms.ori.origin, trans);
		Matrix16Multiply(glState.modelview, trans, product);
		GL_SetModelviewMatrix(product);

		DrawSkyDome(tess.shader);

		GL_SetModelviewMatrix(oldmodelview);

		// generate the vertexes for all the clouds, which will be drawn
		// by the generic shader routine
		//R_BuildCloudData(&tess);
		//RB_StageIteratorGeneric();

		// back to normal depth range
		GL_SetDepthRange(0.0, 1.0);

		// note that sky was drawn so we will draw a sun later
		backEnd.skyRenderedThisView = qtrue;

		return;
	}
	else*/
	{// draw the outer skybox
		matrix_t oldmodelview;

		skyImage = tess.shader->sky.outerbox[r_skynum->integer];
		
		GL_State(0);

		//qglTranslatef (backEnd.viewParms.ori.origin[0], backEnd.viewParms.ori.origin[1], backEnd.viewParms.ori.origin[2]);

		matrix_t trans, product;
		Matrix16Copy( glState.modelview, oldmodelview );
		Matrix16Translation( backEnd.viewParms.ori.origin, trans );
		Matrix16Multiply( glState.modelview, trans, product );
		GL_SetModelviewMatrix( product );

		DrawSkyBox( tess.shader );

		GL_SetModelviewMatrix( oldmodelview );
	}

#else //___FORCED_SKYDOME___
	DrawSkyDome(tess.shader);
#endif //___FORCED_SKYDOME___

	extern qboolean		PROCEDURAL_SKY_ENABLED;

	if (!PROCEDURAL_SKY_ENABLED)
	{
		// generate the vertexes for all the clouds, which will be drawn
		// by the generic shader routine
		R_BuildCloudData(&tess);
		RB_StageIteratorGeneric();
	}

	// draw the inner skybox


	// back to normal depth range
	GL_SetDepthRange( 0.0, 1.0 );

	// note that sky was drawn so we will draw a sun later
	backEnd.skyRenderedThisView = qtrue;
}





