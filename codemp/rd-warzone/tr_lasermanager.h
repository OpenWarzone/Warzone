#if 0
#pragma once

#include "tr_local.h"
//#include "gl/glew.h"
//#include "glm/glm.hpp"

class Shader;

class LaserManager
{
private:
	int maxLasers;						// maximum number of lasers we allow at once; passed as argument to LaserManager constructor
	int activeLasers;					// number of lasers that are actually active at the moment

	GLuint vao;							// OpenGL vertex array object identifier
	GLuint vbos[5];						// OpenGL vertex buffer object identifiers: vertex positions, tex coords, colours, depths, and indices

	GLfloat *vertices;					// dynamically-allocated list of vertex positions for lasers we want to render
	GLfloat *currentVertex;				// used to track insertion of vertex positions in the "vertices" array

	GLfloat *texCoords;					// dynamically-allocated list of texture coordinates for lasers we want to render
	GLfloat *currentTexCoord;			// used to track insertion of texture coordinate positions in the "texCoords" array

	GLfloat *colors;					// dynamically-allocated list of vertex colours for lasers we want to render
	GLfloat *currentColor;				// used to track insertion of vertex colours in the "colors" array

	GLfloat *depths;					// dynamically-allocated list of vertex fragment depths for lasers we want to render
	GLfloat *currentDepth;				// used to track insertion of vertex fragment depths in the "depths" array

	Shader *laserShader;				// shader object we use for rendering the lasers
	Shader *outlineShader;				// shader object we use to see the mesh outline of the laser geometry

	matrix_t orthoProjection;			// defines the orthographic projection we use when rendering the lasers
	matrix_t orthoView;				// defines the orthographic view we use when rendering the lasers
	matrix_t perspectiveProjection;	// defines the perspective projection we project laser coordinates onto ortho coordinates with
	matrix_t perspectiveView;			// defines the perspective view we project laser coordinates onto ortho coordinates with
	vec4_t viewPort;					// defines the edges of the screen viewing area
	vec3_t cameraPos;				// defines the position of the 3D (perspective) camera

	GLuint texture;						// the laserbolt texture that we use

	void setupVBOs();					// setup our VAO and VBOs
	void setupShader();					// load and configure the laser rendering shader
	void loadTextures();				// load the laserbolt texture that we use

	// some simple methods for easily configuring data we want to send to the GPU
	void addVertex(float x, float y);
	void addTexCoord(float u, float v, float q);
	void addColor(float r, float g, float b, float a);
	void addDepth(float depth);

public:
	// called once to instantiate the class and prepare for a maximum number of lasers to render at once
	LaserManager(int maxLasers);

	// called when program does not need to render lasers any more
	~LaserManager();

	// must be called to set the camera position so we know how far away a particular laser bolt is
	void setCameraPos(vec3_t cameraPos);

	// must be called so we can perform a projection from perspective viewing space into ortho viewing space, to render our lasers
	void prepareView(matrix_t &orthoProjection, matrix_t &orthoView, matrix_t &perspectiveProjection, matrix_t &perspectiveView, vec4_t &viewPort);

	// called once per render loop per laser that we want to render
	void addLaser(vec3_t p1, vec3_t p2, vec4_t color, float width);

	// performs batch laser rendering; renders all lasers specified by addLaser() since last render() call;
	// the showGeometry variable controls whether or not we will also render the mesh outline for the lasers too
	void render(bool showGeometry);
};
#endif
