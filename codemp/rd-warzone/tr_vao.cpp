#include "tr_local.h"

VAO_t R_CreateVAO(float* vertices, int size, VBO_t *vbo)
{
	VAO_t vao;

	vao.vaoID = 0;

	if (vbo)
	{
		vao.vboID = vbo->vertexesVBO;
		vao.vboIndependent = qtrue;
	}
	else
	{
		vao.vboID = 0;
		vao.vboIndependent = qfalse;
		qglGenBuffers(1, &vao.vboID);
		qglBindBuffer(GL_ARRAY_BUFFER, vao.vboID);
		qglBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices, GL_STATIC_DRAW);
		//qglBufferSubData(GL_ARRAY_BUFFER, 0, size*sizeof(float), vertices);
		qglBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if (qglIsVertexArray(vao.vaoID) == GL_TRUE)
		qglDeleteVertexArrays(1, &vao.vaoID);

	qglGenVertexArrays(1, &vao.vaoID);
	qglBindVertexArray(vao.vaoID);
	qglEnableVertexAttribArray(0);
	qglBindBuffer(GL_ARRAY_BUFFER, vao.vboID);
	qglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindVertexArray(0);
	return vao;
}

void R_PrintVAO(VAO_t vao, shaderProgram_t *sp, int size, matrix_t mvp, matrix_t model)
{
	GLSL_BindProgram(sp);
	qglBindVertexArray(vao.vaoID);
	GLSL_SetUniformMatrix16(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, mvp, 1);
	GLSL_SetUniformMatrix16(sp, UNIFORM_MODELMATRIX, model);
	qglDrawArrays(GL_TRIANGLES, 0, size);
	qglBindVertexArray(0);
	GLSL_BindProgram(NULL);
}

void R_FreeVAO(VAO_t vao)
{
	if (!vao.vboIndependent)
	{
		qglDeleteBuffers(1, &vao.vboID);
	}

	qglDeleteVertexArrays(1, &vao.vaoID);
}
