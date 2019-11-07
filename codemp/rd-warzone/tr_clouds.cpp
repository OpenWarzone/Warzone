#include "tr_local.h"

extern qboolean		PROCEDURAL_CLOUDS_LAYER;
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

int		DYNAMIC_WEATHER_NEXT_CHANGE = 0;
int		DYNAMIC_WEATHER_CURRENT_WEATHER = 0;
float	DYNAMIC_WEATHER_CLOUDCOVER = PROCEDURAL_CLOUDS_CLOUDCOVER;
float	DYNAMIC_WEATHER_CLOUDSCALE = 1.0;// PROCEDURAL_CLOUDS_CLOUDSCALE;

float	DYNAMIC_WEATHER_PUDDLE_STRENGTH = 0.0;

int		DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN_TIME = 0;
float	DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN = PROCEDURAL_CLOUDS_CLOUDCOVER;
float	DYNAMIC_WEATHER_WANTED_CLOUDCOVER = PROCEDURAL_CLOUDS_CLOUDCOVER;

typedef enum
{
	DWEATHER_CLEAREST,
	DWEATHER_VERY_CLEAR,
	DWEATHER_CLEAR,
	DWEATHER_CLOUDY,
	DWEATHER_VERY_CLOUDY,
	DWEATHER_LIGHT_RAIN,
	DWEATHER_RAIN,
	DWEATHER_HEAVY_RAIN,
	DWEATHER_RAIN_STORM,
} dynamicWeatherTypes_t;

extern void RE_WorldEffectCommand_REAL(const char *command, qboolean noHelp);
extern void RB_SetupGlobalWeatherZone(void);

void DynamicWeather_UpdateWeatherSystems(void)
{// Set the DYNAMIC_WEATHER_CURRENT_WEATHER value based on cloudcover setting, and also add JKA weather fx as required... 
	float cc = Q_clamp(0.0, DYNAMIC_WEATHER_CLOUDCOVER*0.3, 0.3);

	//ri->Printf(PRINT_ALL, "DYNAMIC_WEATHER_CLOUDCOVER: %f. cc: %f.\n", DYNAMIC_WEATHER_CLOUDCOVER, cc);

	if (StringContainsWord(CURRENT_WEATHER_OPTION, "snow"))
	{
		if (cc >= 0.275)
		{// Snow storm...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_RAIN_STORM)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				RE_WorldEffectCommand_REAL("gustingwind", qtrue);
				RE_WorldEffectCommand_REAL("snow", qtrue);
				RE_WorldEffectCommand_REAL("snow", qtrue);
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_RAIN_STORM;
			}
		}
		else if (cc >= 0.225)
		{// Heavy snow...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_HEAVY_RAIN)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				RE_WorldEffectCommand_REAL("snow", qtrue);
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_HEAVY_RAIN;
			}
		}
		else if (cc >= 0.175)
		{// Normal snow...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_RAIN)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_RAIN;
			}
		}
		else if (cc >= 0.125)
		{// Light snow...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_LIGHT_RAIN)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_LIGHT_RAIN;
			}
		}
		else if (cc >= 0.1)
		{// Cloudy...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_VERY_CLOUDY)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_VERY_CLOUDY;
			}
		}
		else if (cc >= 0.075)
		{// Cloudy...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_CLOUDY)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_CLOUDY;
			}
		}
		else if (cc >= 0.05)
		{// Clear...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_CLEAR)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_CLEAR;
			}
		}
		else if (cc >= 0.025)
		{// Very clear...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_VERY_CLEAR)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_VERY_CLEAR;
			}
		}
		else
		{// Clearest...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER > DWEATHER_CLEAREST)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_CLEAREST;
			}
		}
	}
	else
	{
		if (cc >= 0.275)
		{// Stormy rain...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_RAIN_STORM)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				RE_WorldEffectCommand_REAL("rain", qtrue);
				RE_WorldEffectCommand_REAL("rain", qtrue);
				RE_WorldEffectCommand_REAL("rain", qtrue);
				RE_WorldEffectCommand_REAL("rain", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 0.4;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_RAIN_STORM;
			}
		}
		else if (cc >= 0.225)
		{// Heavy rain...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_HEAVY_RAIN)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				RE_WorldEffectCommand_REAL("rain", qtrue);
				RE_WorldEffectCommand_REAL("rain", qtrue);
				RE_WorldEffectCommand_REAL("lightrain", qtrue);
				RE_WorldEffectCommand_REAL("lightrain", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 0.5;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_HEAVY_RAIN;
			}
		}
		else if (cc >= 0.175)
		{// Normal rain...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_RAIN)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				RE_WorldEffectCommand_REAL("rain", qtrue);
				RE_WorldEffectCommand_REAL("lightrain", qtrue);
				RE_WorldEffectCommand_REAL("lightrain", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 0.65;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_RAIN;
			}
		}
		else if (cc >= 0.125)
		{// Light rain...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_LIGHT_RAIN)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				RE_WorldEffectCommand_REAL("lightrain", qtrue);
				RE_WorldEffectCommand_REAL("lightrain", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 0.75;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_LIGHT_RAIN;
			}
		}
		else if (cc >= 0.1)
		{// Cloudy...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_VERY_CLOUDY)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_VERY_CLOUDY;
			}
		}
		else if (cc >= 0.075)
		{// Cloudy...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_CLOUDY)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_CLOUDY;
			}
		}
		else if (cc >= 0.05)
		{// Clear...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_CLEAR)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_CLEAR;
			}
		}
		else if (cc >= 0.025)
		{// Very clear...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER != DWEATHER_VERY_CLEAR)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				//DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_VERY_CLEAR;
			}
		}
		else
		{// Clearest...
			if (DYNAMIC_WEATHER_CURRENT_WEATHER > DWEATHER_CLEAREST)
			{
				RE_WorldEffectCommand_REAL("clear", qtrue);
				DYNAMIC_WEATHER_CLOUDSCALE = 1.0;
				DYNAMIC_WEATHER_CURRENT_WEATHER = DWEATHER_CLEAREST;
			}
		}
	}
}

void DynamicWeather_Update(void)
{
	if (PROCEDURAL_CLOUDS_DYNAMIC)
	{// TODO: Move this to server, so all clients share the same weather...
		if (DYNAMIC_WEATHER_NEXT_CHANGE <= backEnd.refdef.time)
		{
			DYNAMIC_WEATHER_NEXT_CHANGE = backEnd.refdef.time + 60000;

#define MAX_DWEATHER_JUMP 2
			int minWeatherChoice = Q_clampi(DWEATHER_CLEAREST, DYNAMIC_WEATHER_CURRENT_WEATHER - MAX_DWEATHER_JUMP, DWEATHER_RAIN_STORM);
			int maxWeatherChoice = Q_clampi(DWEATHER_CLEAREST, DYNAMIC_WEATHER_CURRENT_WEATHER + MAX_DWEATHER_JUMP, DWEATHER_RAIN_STORM);
			int nextWeatherChoice = irand(minWeatherChoice, maxWeatherChoice);

			// Set up the blend to the selected choice...
			DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN = DYNAMIC_WEATHER_CLOUDCOVER;
			DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN_TIME = backEnd.refdef.time;
			float wantedCC = 0.0;

			switch (nextWeatherChoice)
			{
				case DWEATHER_CLEAREST:
				default:
				{
					wantedCC = 0.0 + (random()*0.024);
					break;
				}
				case DWEATHER_VERY_CLEAR:
				{
					wantedCC = 0.025 + (random()*0.024);
					break;
				}
				case DWEATHER_CLEAR:
				{
					wantedCC = 0.05 + (random()*0.024);
					break;
				}
				case DWEATHER_CLOUDY:
				{
					wantedCC = 0.075 + (random()*0.024);
					break;
				}
				case DWEATHER_VERY_CLOUDY:
				{
					wantedCC = 0.1 + (random()*0.024);
					break;
				}
				case DWEATHER_LIGHT_RAIN:
				{
					wantedCC = 0.125 + (random()*0.05);
					break;
				}
				case DWEATHER_RAIN:
				{
					wantedCC = 0.175 + (random()*0.05);
					break;
				}
				case DWEATHER_HEAVY_RAIN:
				{
					wantedCC = 0.225 + (random()*0.05);
					break;
				}
				case DWEATHER_RAIN_STORM:
				{
					wantedCC = 0.275 + (random()*0.025);
					break;
				}
			}

			DYNAMIC_WEATHER_WANTED_CLOUDCOVER = Q_clamp(0.0, wantedCC/0.3, 1.0); // converted to 0.0-1.0 range...

			//ri->Printf(PRINT_ALL, "DYNAMIC_WEATHER: Blending to new weather %i (cc: %f cc01: %f).\n", nextWeatherChoice, wantedCC, DYNAMIC_WEATHER_WANTED_CLOUDCOVER);
		}
		else
		{// Blend to our wanted value over time...
			float diff = DYNAMIC_WEATHER_WANTED_CLOUDCOVER - DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN;
			float elapsedTime = (float)(backEnd.refdef.time - DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN_TIME);
			float finalTime = (float)(DYNAMIC_WEATHER_NEXT_CHANGE - DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN_TIME);
			float percentTime = Q_clamp(0.0, elapsedTime / finalTime, 1.0);
			float addCC = diff * percentTime;
			DYNAMIC_WEATHER_CLOUDCOVER = DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN + addCC;

			//ri->Printf(PRINT_ALL, "DYNAMIC_WEATHER: Blending to %f from %f. Current %f.\n", DYNAMIC_WEATHER_WANTED_CLOUDCOVER, DYNAMIC_WEATHER_WANTED_CLOUDCOVER_BEGIN, DYNAMIC_WEATHER_CLOUDCOVER);
		}

		// Update the JKA weather system...
		DynamicWeather_UpdateWeatherSystems();
	}
	else
	{
		DYNAMIC_WEATHER_CLOUDCOVER = PROCEDURAL_CLOUDS_CLOUDCOVER;
		DYNAMIC_WEATHER_CLOUDSCALE = PROCEDURAL_CLOUDS_CLOUDSCALE;
	}
}

void CLOUD_LAYER_Render(void)
{
	if (!PROCEDURAL_CLOUDS_ENABLED || !PROCEDURAL_CLOUDS_LAYER) return;

	extern void SetViewportAndScissor(void);

	CLOUD_LAYER_InitCloudLayer(); // Re-init if map water level has changed...

	//ri->Printf(PRINT_WARNING, "Drawing clouds at %f.\n", MAP_INFO_MAXS[2] - 256.0);

	GLSL_BindProgram(&tr.cloudsShader);

	
	//FBO_Bind(tr.renderNoDepthFbo);
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

	extern float		DYNAMIC_WEATHER_CLOUDCOVER;
	extern float		DYNAMIC_WEATHER_CLOUDSCALE;

	vec4_t vector;
	VectorSet4(vector, PROCEDURAL_CLOUDS_LAYER ? 1.0 : 0.0, DYNAMIC_WEATHER_CLOUDSCALE, PROCEDURAL_CLOUDS_SPEED, PROCEDURAL_CLOUDS_DARK);
	GLSL_SetUniformVec4(&tr.cloudsShader, UNIFORM_LOCAL2, vector);

	VectorSet4(vector, PROCEDURAL_CLOUDS_LIGHT, DYNAMIC_WEATHER_CLOUDCOVER, PROCEDURAL_CLOUDS_CLOUDALPHA, PROCEDURAL_CLOUDS_SKYTINT);
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
	//GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS | GLS_ATEST_GT_0);

	//qglDepthMask(GL_FALSE);
	qglDepthMask(GL_TRUE);

	R_BindVBO(gCloudLayerVboID);

	GLSL_VertexAttribsState(ATTR_INSTANCES_POSITION | ATTR_INSTANCES_TEXCOORD);

	qglBindVertexArray(gCloudLayerVaoID);

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gCloudLayerIndexID);

	qglDrawElements(GL_TRIANGLES, gCloudLayerDrawNumber, GL_UNSIGNED_INT, 0);

	qglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	R_BindNullVBO();
}
