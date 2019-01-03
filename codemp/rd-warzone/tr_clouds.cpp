#include "tr_local.h"

extern qboolean		PROCEDURAL_CLOUDS_LAYER;
extern qboolean		PROCEDURAL_CLOUDS_ENABLED;
extern qboolean		PROCEDURAL_CLOUDS_LAYER;
extern float		PROCEDURAL_CLOUDS_CLOUDSCALE;
extern float		PROCEDURAL_CLOUDS_SPEED;
extern float		PROCEDURAL_CLOUDS_DARK;
extern float		PROCEDURAL_CLOUDS_LIGHT;
extern float		PROCEDURAL_CLOUDS_CLOUDCOVER;
extern float		PROCEDURAL_CLOUDS_CLOUDALPHA;
extern float		PROCEDURAL_CLOUDS_SKYTINT;
extern vec3_t		MAP_INFO_MAXS;
extern float		MAP_INFO_MAXSIZE;
extern float		MAP_WATER_LEVEL;


#define MAP_CLOUD_LAYER_HEIGHT (MAP_INFO_MAXS[2] - 256.0)

qboolean CLOUD_LAYER_INITIALIZED = qfalse;

// Variables
GLuint gCloudLayerDrawNumber = 0; // How many indices to draw 

GLuint gCloudLayerVaoID = 0;			 // ID for vertex array object
VBO_t *gCloudLayerVboID = NULL;			 // ID for vertex array object
GLuint gCloudLayerIndexID = 0;		 // ID for vertex array object

#include "tr_matrix.h"

#define GLS_ALPHA				(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)

void CLOUD_LAYER_InitCloudLayer()
{
	if (!PROCEDURAL_CLOUDS_ENABLED || !PROCEDURAL_CLOUDS_LAYER) return;
	if (gCloudLayerVaoID != 0) return;

	CLOUD_LAYER_INITIALIZED = qtrue;

	// constants
	static const int QUAD_GRID_SIZE = 1;// 10;
	static const int NR_VERTICES = (QUAD_GRID_SIZE + 1)*(QUAD_GRID_SIZE + 1);
	static const int NR_TRIANGLES = 2 * QUAD_GRID_SIZE*QUAD_GRID_SIZE;
	static const int NR_INDICES = 3 * NR_TRIANGLES;

	gCloudLayerDrawNumber = NR_INDICES;

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
			vertices[vertexPosition].z = MAP_CLOUD_LAYER_HEIGHT;

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
	qglGenVertexArrays(1, &gCloudLayerVaoID);
	qglBindVertexArray(gCloudLayerVaoID);

	gCloudLayerVboID = R_CreateVBO((byte *)vertices, NR_VERTICES * sizeof(vertices[0]) + NR_VERTICES * sizeof(texcoords[0]), VBO_USAGE_STATIC);
	R_BindVBO(gCloudLayerVboID);

	// Set the buffer pointers
	qglBufferSubData(GL_ARRAY_BUFFER, 0, NR_VERTICES * sizeof(vertices[0]), vertices);
	qglBufferSubData(GL_ARRAY_BUFFER, NR_VERTICES * sizeof(vertices[0]), NR_VERTICES * sizeof(texcoords[0]), texcoords);

	// Bind the index buffer
	qglGenBuffers(1, &gCloudLayerIndexID);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gCloudLayerIndexID);
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * NR_TRIANGLES * sizeof(indices[0]), indices, GL_STATIC_DRAW);


	// Initalize shaders
	GLSL_BindProgram(&tr.cloudsShader);

	// Initialize the vertex position attribute from the vertex shader
	GLuint pos = ATTR_INDEX_INSTANCES_POSITION;// qglGetAttribLocation(tr.cloudsShader.program, "attr_InstancesPosition");
	qglEnableVertexAttribArray(pos);
	qglVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	GLuint tex = ATTR_INDEX_INSTANCES_TEXCOORD;// qglGetAttribLocation(tr.cloudsShader.program, "attr_InstancesTexCoord");
	qglEnableVertexAttribArray(tex);
	qglVertexAttribPointer(tex, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(NR_VERTICES * sizeof(vertices[0])));

	delete[] vertices;
	delete[] texcoords;
	delete[] indices;

	R_BindNullVBO();
	GLSL_BindProgram(NULL);

	//ri->Printf(PRINT_WARNING, "Initialized procedural cloud layer.\n");
}

void CLOUD_LAYER_Render(void)
{
	if (!PROCEDURAL_CLOUDS_ENABLED || !PROCEDURAL_CLOUDS_LAYER) return;

	extern void SetViewportAndScissor(void);

	CLOUD_LAYER_InitCloudLayer(); // Re-init if map water level has changed...

	//ri->Printf(PRINT_WARNING, "Drawing clouds at %f.\n", MAP_INFO_MAXS[2] - 256.0);

	GLSL_BindProgram(&tr.cloudsShader);

	//FBO_Bind(tr.renderFbo);

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

	GLSL_SetUniformMatrix16(&tr.cloudsShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

	GLSL_SetUniformVec3(&tr.cloudsShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);
	GLSL_SetUniformFloat(&tr.cloudsShader, UNIFORM_TIME, backEnd.refdef.floatTime);


	vec4_t vector;
	VectorSet4(vector, PROCEDURAL_CLOUDS_LAYER ? 1.0 : 0.0, PROCEDURAL_CLOUDS_CLOUDSCALE, PROCEDURAL_CLOUDS_SPEED, PROCEDURAL_CLOUDS_DARK);
	GLSL_SetUniformVec4(&tr.cloudsShader, UNIFORM_LOCAL2, vector);

	VectorSet4(vector, PROCEDURAL_CLOUDS_LIGHT, PROCEDURAL_CLOUDS_CLOUDCOVER, PROCEDURAL_CLOUDS_CLOUDALPHA, PROCEDURAL_CLOUDS_SKYTINT);
	GLSL_SetUniformVec4(&tr.cloudsShader, UNIFORM_LOCAL3, vector);

	float nightScale = RB_NightScale();

	if (tr.viewParms.flags & VPF_SKYCUBEDAY)
		nightScale = 0.0;
	else if (tr.viewParms.flags & VPF_SKYCUBENIGHT)
		nightScale = 1.0;

	VectorSet4(vector, DAY_NIGHT_CYCLE_ENABLED ? 1.0 : 0.0, DAY_NIGHT_CYCLE_ENABLED ? nightScale : 0.0, MAP_CLOUD_LAYER_HEIGHT, 0.0);
	GLSL_SetUniformVec4(&tr.cloudsShader, UNIFORM_LOCAL5, vector); // dayNightEnabled, nightScale, 0.0, 0.0

	VectorSet4(vector, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value);
	GLSL_SetUniformVec4(&tr.cloudsShader, UNIFORM_LOCAL9, vector); // testvalues

	//GL_Cull(CT_TWO_SIDED);
	//GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS);
	GL_Cull(CT_TWO_SIDED);
	GL_State(GLS_ALPHA | GLS_DEPTHFUNC_LESS | GLS_ATEST_GT_0);

	//qglDepthMask(GL_FALSE);
	qglDepthMask(GL_TRUE);

	R_BindVBO(gCloudLayerVboID);

	GLSL_VertexAttribsState(ATTR_INSTANCES_POSITION | ATTR_INSTANCES_TEXCOORD);

	qglBindVertexArray(gCloudLayerVaoID);

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gCloudLayerIndexID);

	qglDrawElements(GL_TRIANGLES, gCloudLayerDrawNumber, GL_UNSIGNED_INT, 0);

	R_BindNullVBO();
}
