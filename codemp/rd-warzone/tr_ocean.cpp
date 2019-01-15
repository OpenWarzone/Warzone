#include "tr_local.h"

#ifdef __OCEAN__
extern qboolean	WATER_ENABLED;
extern qboolean WATER_FARPLANE_ENABLED;
extern float	MAP_INFO_MAXSIZE;
extern float	MAP_WATER_LEVEL;

qboolean WATER_INITIALIZED = qfalse;
qboolean WATER_FAST_INITIALIZED = qfalse;

// Variables

GLuint gDrawNumber = 0; // How many indices to draw 

GLuint gVaoID = 0;			 // ID for vertex array object
//GLuint gVboID = 0;			 // ID for vertex array object
VBO_t *gVboID = NULL;			 // ID for vertex array object
GLuint gIndexID = 0;		 // ID for vertex array object

GLuint gFastDrawNumber = 0; // How many indices to draw 

GLuint gFastVaoID = 0;			 // ID for vertex array object
//GLuint gFastVboID = 0;			 // ID for vertex array object
VBO_t *gFastVboID = NULL;			 // ID for vertex array object
GLuint gFastIndexID = 0;		 // ID for vertex array object

#include "tr_matrix.h"

#define GLS_ALPHA				(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)

void OCEAN_InitOceanFast()
{
	if (!WATER_ENABLED || !WATER_FARPLANE_ENABLED) return;
	if (!(MAP_WATER_LEVEL < 131000.0 && MAP_WATER_LEVEL > -131000.0)) return;
	if (gFastVaoID != 0) return;

	if (!WATER_FAST_INITIALIZED && WATER_ENABLED && r_glslWater->integer >= 1)
	{// Water is ready to be set up for this map...
		WATER_FAST_INITIALIZED = qtrue;

		// constants
		static const int QUAD_GRID_SIZE = 1;// 10;
		static const int NR_VERTICES = (QUAD_GRID_SIZE + 1)*(QUAD_GRID_SIZE + 1);
		static const int NR_TRIANGLES = 2 * QUAD_GRID_SIZE*QUAD_GRID_SIZE;
		static const int NR_INDICES = 3 * NR_TRIANGLES;

		gFastDrawNumber = NR_INDICES;

		//Generate grid positions
		const float scale = 524288.0;// MAP_INFO_MAXSIZE;// 131072.0;// 500.0f;
		const float delta = 2.0f / QUAD_GRID_SIZE;

		vec3* vertices = new vec3[NR_VERTICES];
		vec2* texcoords = new vec2[NR_VERTICES];
		GLuint* indices = new GLuint[3 * NR_TRIANGLES];

		for (int y = 0; y <= QUAD_GRID_SIZE; y++) {
			for (int x = 0; x <= QUAD_GRID_SIZE; x++) {
				int vertexPosition = y*(QUAD_GRID_SIZE + 1) + x;
				vertices[vertexPosition].x = (x*delta - 1.0) * scale;

				//vertices[vertexPosition].y = 0;
				//vertices[vertexPosition].z = (y*delta - 1.0) * scale;
				vertices[vertexPosition].y = (y*delta - 1.0) * scale;
				vertices[vertexPosition].z = MAP_WATER_LEVEL;// 0;

				texcoords[vertexPosition].x = x*delta;
				texcoords[vertexPosition].y = y*delta;
			}
		}

		// Generate indices into vertex list
		for (int y = 0; y < QUAD_GRID_SIZE; y++) {
			for (int x = 0; x < QUAD_GRID_SIZE; x++) {
				int indexPosition = y*QUAD_GRID_SIZE + x;
				// tri 0
				indices[6 * indexPosition] = y    *(QUAD_GRID_SIZE + 1) + x;    //bl  
				indices[6 * indexPosition + 1] = (y + 1)*(QUAD_GRID_SIZE + 1) + x + 1;//tr
				indices[6 * indexPosition + 2] = y    *(QUAD_GRID_SIZE + 1) + x + 1;//br
																					// tri 1
				indices[6 * indexPosition + 3] = y    *(QUAD_GRID_SIZE + 1) + x;    //bl
				indices[6 * indexPosition + 4] = (y + 1)*(QUAD_GRID_SIZE + 1) + x;    //tl
				indices[6 * indexPosition + 5] = (y + 1)*(QUAD_GRID_SIZE + 1) + x + 1;//tr
			}
		}

		uint32_t numVerts = NR_VERTICES;
		uint32_t numIndexes = 3 * NR_TRIANGLES;
		R_OptimizeMesh(&numVerts, &numIndexes, (uint32_t *)indices, NULL);

		// Create a vertex array object
		qglGenVertexArrays(1, &gFastVaoID);
		qglBindVertexArray(gFastVaoID);

		gFastVboID = R_CreateVBO((byte *)vertices, NR_VERTICES * sizeof(vertices[0]) + NR_VERTICES * sizeof(texcoords[0]), VBO_USAGE_STATIC);
		R_BindVBO(gFastVboID);

		// Set the buffer pointers
		qglBufferSubData(GL_ARRAY_BUFFER, 0, NR_VERTICES * sizeof(vertices[0]), vertices);
		qglBufferSubData(GL_ARRAY_BUFFER, NR_VERTICES * sizeof(vertices[0]), NR_VERTICES * sizeof(texcoords[0]), texcoords);

		// Bind the index buffer
		qglGenBuffers(1, &gFastIndexID);
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gFastIndexID);
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * NR_TRIANGLES * sizeof(indices[0]), indices, GL_STATIC_DRAW);


		// Initalize shaders
		GLSL_BindProgram(&tr.waterForwardFastShader);

		// Initialize the vertex position attribute from the vertex shader
		GLuint pos = ATTR_INDEX_INSTANCES_POSITION;// qglGetAttribLocation(tr.waterForwardShader.program, "attr_InstancesPosition");
		qglEnableVertexAttribArray(pos);
		qglVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

		GLuint tex = ATTR_INDEX_INSTANCES_TEXCOORD;// qglGetAttribLocation(tr.waterForwardShader.program, "attr_InstancesTexCoord");
		qglEnableVertexAttribArray(tex);
		qglVertexAttribPointer(tex, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(NR_VERTICES * sizeof(vertices[0])));

		delete[] vertices;
		delete[] texcoords;
		delete[] indices;

		R_BindNullVBO();
		GLSL_BindProgram(NULL);
	}
}

void OCEAN_Render(void)
{
	if (!WATER_ENABLED || !WATER_FARPLANE_ENABLED) return;
	if (!(MAP_WATER_LEVEL < 131000.0 && MAP_WATER_LEVEL > -131000.0)) return;

	if (WATER_ENABLED && WATER_FARPLANE_ENABLED && r_glslWater->integer >= 1)
	{
		extern void SetViewportAndScissor(void);

		OCEAN_InitOceanFast(); // Re-init if map water level has changed...

		GLSL_BindProgram(&tr.waterForwardFastShader);

		FBO_Bind(tr.renderWaterFbo);

		viewParms_t parms = tr.viewParms;

		parms.zFar = 524288.0;

		{
			parms.visBounds[0][0] = -parms.zFar;
			parms.visBounds[0][1] = -parms.zFar;

			parms.visBounds[1][0] = parms.zFar;
			parms.visBounds[1][1] = parms.zFar;
		}


		uint32_t origState = glState.glStateBits;

		extern void R_SetupProjectionZ(viewParms_t *dest);

		R_SetupProjectionZ(&parms);
		GL_SetProjectionMatrix(parms.projectionMatrix);
		GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);

		GLSL_SetUniformMatrix16(&tr.waterForwardFastShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

		GLSL_SetUniformVec3(&tr.waterForwardFastShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);
		GLSL_SetUniformFloat(&tr.waterForwardFastShader, UNIFORM_TIME, backEnd.refdef.floatTime);

		GL_Cull(CT_TWO_SIDED);
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS);

		qglDepthMask(GL_FALSE);

		vec4_t l9;
		VectorSet4(l9, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, MAP_WATER_LEVEL);
		GLSL_SetUniformVec4(&tr.waterForwardFastShader, UNIFORM_LOCAL9, l9);

		vec4_t l10;
		VectorSet4(l10, 0.0, 0.0, 0.0, 0.0);
		GLSL_SetUniformVec4(&tr.waterForwardFastShader, UNIFORM_LOCAL10, l10);

		R_BindVBO(gFastVboID);

		GLSL_VertexAttribsState(ATTR_INSTANCES_POSITION | ATTR_INSTANCES_TEXCOORD);

		qglBindVertexArray(gFastVaoID);
		
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gFastIndexID);

		qglDrawElements(GL_TRIANGLES, gFastDrawNumber, GL_UNSIGNED_INT, 0);

		R_BindNullVBO();
	}
}
#endif //__OCEAN__
