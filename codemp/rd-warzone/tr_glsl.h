#ifndef TR_GLSL
#define TR_GLSL

#ifdef __GLSL_OPTIMIZER__
	#include "glsl_optimizer.h"
	extern glslopt_ctx *ctx;
	void GLSL_PrintShaderOptimizationStats(char *shaderName, glslopt_shader *shader);
#endif

typedef struct uniformInfo_s {
	char *name;
	int type;
	int size;
} uniformInfo_t;

extern uniformInfo_t uniformsInfo[];
extern qboolean ALLOW_GL_400;
extern char		GLSL_MAX_VERSION[64];
extern const char glslMaterialsList[];
extern int shaders_next_id;
extern shaderProgram_t *shaders[256];

void GLSL_PrintProgramInfoLog(GLuint object, qboolean developerOnly);
void GLSL_PrintShaderInfoLog(GLuint object, qboolean developerOnly);
void GLSL_PrintShaderSource(GLuint shader);
char *GLSL_GetHighestSupportedVersion(void);
void GLSL_GetShaderHeader(GLenum shaderType, const GLcharARB *extra, char *dest, int size, char *forceVersion);
int GLSL_EnqueueCompileGPUShader(GLuint program, GLuint *prevShader, const GLchar *buffer, int size, GLenum shaderType);
int GLSL_LoadGPUShaderText(const char *name, const char *fallback, GLenum shaderType, char *dest, int destSize);
void GLSL_LinkProgram(GLuint program);
GLint GLSL_LinkProgramSafe(GLuint program);
void GLSL_ValidateProgram(GLuint program);
void GLSL_ShowProgramUniforms(shaderProgram_t *program);
int GLSL_BeginLoadGPUShader2(shaderProgram_t * program, const char *name, int attribs, const char *vpCode, const char *fpCode, const char *cpCode, const char *epCode, const char *gsCode);
bool GLSL_IsGPUShaderCompiled(GLuint shader);
void FBO_AttachTextureImage(image_t *img, int index);
void R_AttachFBOTextureDepth(int texId);
void SetViewportAndScissor(void);
void FBO_SetupDrawBuffers();
qboolean R_CheckFBO(const FBO_t * fbo);
void GLSL_AttachTextures(void);
void GLSL_AttachGlowTextures(void);
void GLSL_AttachGenericTextures(void);
void GLSL_AttachRenderDepthTextures(void);
void GLSL_AttachRenderGUITextures(void);
void GLSL_AttachWaterTextures(void);
bool GLSL_EndLoadGPUShader(shaderProgram_t *program);
int GLSL_BeginLoadGPUShader(shaderProgram_t * program, const char *name,
	int attribs, qboolean fragmentShader, qboolean tesselation, qboolean geometry, const GLcharARB *extra, qboolean addHeader,
	char *forceVersion, const char *fallback_vp, const char *fallback_fp, const char *fallback_cp, const char *fallback_ep, const char *fallback_gs);
void GLSL_InitUniforms(shaderProgram_t *program);
void GLSL_FinishGPUShader(shaderProgram_t *program);
void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value);
void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value);
void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v);
void GLSL_SetUniformVec2x16(shaderProgram_t *program, int uniformNum, const vec2_t *elements, int numElements);
void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v);
void GLSL_SetUniformVec3xX(shaderProgram_t *program, int uniformNum, const vec3_t *elements, int numElements);
void GLSL_SetUniformVec3x64(shaderProgram_t *program, int uniformNum, const vec3_t *elements, int numElements);
void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v);
void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v);
void GLSL_SetUniformFloatxX(shaderProgram_t *program, int uniformNum, const float *elements, int numElements);
void GLSL_SetUniformFloatx64(shaderProgram_t *program, int uniformNum, const float *elements, int numElements);
void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const float *matrix, int numElements);
void GLSL_DeleteGPUShader(shaderProgram_t *program);
int GLSL_BeginLoadGPUShaders(void);
void GLSL_BindAttributeLocations(shaderProgram_t *program, int attribs);
void GLSL_EndLoadGPUShaders(int startTime);
void GLSL_ShutdownGPUShaders(void);
void GLSL_BindProgram(shaderProgram_t * program);
void GLSL_VertexAttribsState(uint32_t stateBits);
void GLSL_UpdateTexCoordVertexAttribPointers(uint32_t attribBits);
void GLSL_VertexAttribPointers(uint32_t attribBits);

#endif