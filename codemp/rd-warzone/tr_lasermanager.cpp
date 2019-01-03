#if 0
/*#include "rendering/lasermanager.h"

#include "util/shader.h"
#include "util/loadtexture.h"

#include "gl/glew.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
using namespace glm;*/

#include "tr_local.h"
#include "tr_lasermanager.h"

#include <iostream>
using namespace std;

/*
Some notes on how the lasers are rendered:

Currently, I use a single glDrawElements(GL_TRIANGLES) call to render all of the lasers owned by a manager
instance. Drawing indexed GL_TRIANGLES is pretty fast, but there may be better ways to do this. Instanced
rendering won't work in this case since we must set vertex positions individually for each laser. If using
GL_TRIANGLE_STRIP is more efficient, then using primitive restart is another possible option.

Laser rendering is dependent on the correct orthographic projection settings; without modifying how the
LaserManager works, changes to the orthographic projection settings can break laser rendering. The size
of the orthographic view should be the same size as the window, with (0, 0) being the window center.

While it's trivially easy to scale the laserbolt as a whole based on its distance from the camera, faking
the perspective distortion is much trickier because this requires trapezoids, and it is well-known that
texturing trapezoids using a square texture is a more difficult problem:
	http://stackoverflow.com/questions/15242507/perspective-correct-texturing-of-trapezoid-in-opengl-es-2-0
However, by adapting solutions from any of the following;
	http://www.xyzw.us/~cass/qcoord/
	http://www.reedbeta.com/blog/quadrilateral-interpolation-part-1/
we can achieve the desired effect and scale the ends of the laserbolts independently based on their distance
from the camera, giving a much more convincing effect.

KNOWN BUGS/ISSUES:

-- The laser won't render if either of the points fall behind the viewer; this should probably be tweaked
   in a future fix for maximum flexibility, but a simple fix to this isn't obvious to me at this time.
*/

LaserManager::LaserManager(int maxLasers)
{
	// set the maximum number of lasers we can render at one time
	this -> maxLasers = maxLasers;

	// set aside some memory to contain the vertex position, vertex colour, and depth data we'll send to the GPU every render step
	vertices = new GLfloat[maxLasers * 16];
	texCoords = new GLfloat[maxLasers * 24];
	colors = new GLfloat[maxLasers * 32];
	depths = new GLfloat[maxLasers * 8];

	// initialize our GPU memory, set up our shader, and load the laserbolt texture
	setupVBOs();
	setupShader();
	loadTextures();
}

LaserManager::~LaserManager()
{
	delete vertices;
	delete texCoords;
	delete colors;
	delete depths;

	delete laserShader;
	delete outlineShader;

	qglDeleteBuffers(5, vbos);
	qglDeleteVertexArrays(1, &vao);
}

void LaserManager::setupVBOs()
{
	const int NUM_TOTAL_INDICES = maxLasers * 18;			// each laser is made up of 6 triangles, each of which uses 3 vertex indices
	const int NUM_INDICES_PER_LASER = 8;					// each laser requires 8 unique indices to be used to form those 6 triangles

	// used to contain our element index information that we send to the GPU
	GLuint *indices = new GLuint[NUM_TOTAL_INDICES];
	GLuint *currentIndex = indices;
	int i;

	// we can assign indices at initialization time since those won't change; only our vertex position and vertex colour data will change
	for(i = 0; i < maxLasers; i ++)
	{
		// top left triangle of top end cap
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 2;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 0;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 1;

		// bottom right triangle of top end cap
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 1;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 3;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 2;

		// top left triangle of body
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 4;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 2;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 3;

		// bottom right triangle of body
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 3;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 5;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 4;

		// top left triangle of bottom end cap
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 6;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 4;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 5;

		// bottom right triangle of bottom end cap
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 5;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 7;
		*currentIndex++ = (NUM_INDICES_PER_LASER * i) + 6;
	}

	// set up our VAO and generate our VBOs
	qglGenVertexArrays(1, &vao);
	qglBindVertexArray(vao);
	qglGenBuffers(5, vbos);

	// vertex positions for our laser
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	qglBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16 * maxLasers, NULL, GL_DYNAMIC_DRAW);
	qglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	qglEnableVertexAttribArray(0);

	// texture coordinates for our laser
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	qglBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 24 * maxLasers, NULL, GL_DYNAMIC_DRAW);
	qglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	qglEnableVertexAttribArray(1);

	// colours for our laser
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
	qglBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 32 * maxLasers, NULL, GL_DYNAMIC_DRAW);
	qglVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	qglEnableVertexAttribArray(2);

	// depths for our laser
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
	qglBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 8 * maxLasers, NULL, GL_DYNAMIC_DRAW);
	qglVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, NULL);
	qglEnableVertexAttribArray(3);

	// these define the element indices we render
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[4]);
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * NUM_TOTAL_INDICES, indices, GL_STATIC_DRAW);

	// free the memory associate with our index data
	delete[] indices;
}

void LaserManager::setupShader()
{
	/*
	laserShader = new Shader("shaders/laser.vert", "shaders/laser.frag");
	laserShader -> bindAttrib("a_Vertex", 0);
	laserShader -> bindAttrib("a_TexCoord", 1);
	laserShader -> bindAttrib("a_Color", 2);
	laserShader -> bindAttrib("a_Depth", 3);
	laserShader -> link();
	laserShader -> bind();
	laserShader -> uniform1i("u_Texture", 0);
	laserShader -> unbind();

	outlineShader = new Shader("shaders/outline.vert", "shaders/outline.frag");
	outlineShader -> bindAttrib("a_Vertex", 0);
	outlineShader -> bindAttrib("a_Depth", 3);
	outlineShader -> link();
	outlineShader -> bind();
	outlineShader -> unbind();
	*/
}

void LaserManager::loadTextures()
{
	//texture = loadTexture("png/laserbolt.png");
}

void LaserManager::prepareView(matrix_t &orthoProjection, matrix_t &orthoView, matrix_t &perspectiveProjection, matrix_t &perspectiveView, vec4_t &viewPort)
{
	// store our viewing data
	Matrix16Copy(orthoProjection, this->orthoProjection);
	Matrix16Copy(orthoView, this->orthoView);
	Matrix16Copy(perspectiveProjection, this->perspectiveProjection);
	Matrix16Copy(perspectiveView, this->perspectiveView);
	VectorCopy4(viewPort, this->viewPort);

	// reset our vertex attribute data pointers
	currentVertex = vertices;
	currentTexCoord = texCoords;
	currentColor = colors;
	currentDepth = depths;
	activeLasers = 0;
}

void LaserManager::setCameraPos(vec3_t cameraPos)
{
	VectorCopy(cameraPos, this->cameraPos);
}

void LaserManager::addLaser(vec3_t p1, vec3_t p2, vec4_t color, float width)
{
	const float DIST_SCALING_FACTOR = 1500.0f;	// sort of hand-tuned value that defines how the laser should scale with distance
	const float EPS = 0.01f;					// distance at which two projected end-point coords are considered the same
	const float HEAD_ON_FALLOFF = 128.0f;		// how slowly we will gradually start to consider the bolt as being viewed head-on

	// project the incoming 3D coordinates onto the screen; this is the key to this technique---we can then overlay a laser
	// image, with ends at each of the 3D coordinates passed in, onto an orthographic view
	vec3_t orthoP1 = project(p1, perspectiveView, perspectiveProjection, viewPort);
	vec3_t orthoP2 = project(p2, perspectiveView, perspectiveProjection, viewPort);

	// supports distance calculations to determine how large to scale the ends of the laser
	float headOnFactor;							// to what degree [0.0 - 1.0] we're seeing the bolt head-on
	float distFactor1, distFactor2;				// distance-scaling factor for each end of the bolt
	float topWidth, bottomWidth;				// computed width of the two end-points based on our distance to each

	// for computing the positions of the laser bolt geometry in orthographic space
	vec2_t screenDir;								// direction of laserbolt in orthographic space
	vec2_t topX, topY;							// horizontal and vertical offsets for the top end segment
	vec2_t bottomX, bottomY;						// horizontal and vertical offsets for the bottom end segment
	int i;

	// make sure both of the laser's points appear on screen; otherwise, we don't render anything
	if(orthoP1[2] < 1.0 || orthoP2[2] < 1.0)
	{
		// cheap distance calculation to determine how large to scale the beam
		distFactor1 = DIST_SCALING_FACTOR / Distance(p1, cameraPos);
		distFactor2 = DIST_SCALING_FACTOR / Distance(p2, cameraPos);

		// prevent lasers viewed perfectly dead-on from having the exact same two end points
		orthoP1[0] += EPS * (fabs(orthoP1[0] - orthoP2[0]) < EPS || fabs(orthoP1[1] - orthoP2[1]) < EPS);

		// negate any of the coordinates that fall outside the screen bounds to bring them back in
		orthoP1 *= -sign(orthoP1[2] - 1.0f);
		orthoP2 *= -sign(orthoP2[2] - 1.0f);

		// determines to what degree [0.0 - 1.0] we're see the bolt head-on
		headOnFactor = pow(abs(dot(normalize(p2 - p1), normalize(cameraPos - ((p1 + p2) / 2.0f)))), HEAD_ON_FALLOFF);

		// scale the rear end (whichever end that is) based on how head-on we view it; if we see it head-on, then it should
		// appear to be as large as the end which is nearest to the viewer
		if(distFactor1 > distFactor2)
			distFactor2 = distFactor2 + (distFactor1 - distFactor2) * headOnFactor;
		else
			distFactor1 = distFactor1 + (distFactor2 - distFactor1) * headOnFactor;

		// this is the width of the top and bottom segments due to perspective distortion
		topWidth = distFactor1 * width;
		bottomWidth = distFactor2 * width;

		// compute the direction of the beam's screen coordinates
		screenDir = normalize(vec2(orthoP1 - orthoP2));

		// now compute the offsets for the top and bottom vertices; these will be used to construct
		// the vertex positions around the laser ends, using orthoP1 and orthoP2 as starting points
		topY = screenDir * topWidth;
		topX = vec2(topY.y, -topY.x);
		bottomY = screenDir * bottomWidth;
		bottomX = vec2(bottomY.y, -bottomY.x);

		// construct the vertices for our laser bolt geometry
		addVertex(orthoP1.x - (topX.x) + topY.x, orthoP1.y - (topX.y) + topY.y);				// top left
		addVertex(orthoP1.x + (topX.x) + topY.x, orthoP1.y + (topX.y) + topY.y);				// top right
		addVertex(orthoP1.x - (topX.x), orthoP1.y - (topX.y));									// top middle left
		addVertex(orthoP1.x + (topX.x), orthoP1.y + (topX.y));									// top middle right
		addVertex(orthoP2.x - (bottomX.x), orthoP2.y - (bottomX.y));							// bottom middle left
		addVertex(orthoP2.x + (bottomX.x), orthoP2.y + (bottomX.y));							// bottom middle right
		addVertex(orthoP2.x - (bottomX.x) - bottomY.x, orthoP2.y - (bottomX.y) - bottomY.y);	// bottom left
		addVertex(orthoP2.x + (bottomX.x) - bottomY.x, orthoP2.y + (bottomX.y) - bottomY.y);	// bottom right

		// regular UV texture coordinates won't do the trick here since we're creating trapezoid shapes
		addTexCoord(0.0, topWidth, topWidth);							// top left
		addTexCoord(topWidth, topWidth, topWidth);						// top right
		addTexCoord(0.0, topWidth * 0.5, topWidth);						// top middle left
		addTexCoord(topWidth, topWidth * 0.5, topWidth);				// top middle right
		addTexCoord(0.0, bottomWidth * 0.5, bottomWidth);				// bottom middle left
		addTexCoord(bottomWidth, bottomWidth * 0.5, bottomWidth);		// bottom middle right
		addTexCoord(0.0, 0.0, bottomWidth);								// bottom left
		addTexCoord(bottomWidth, 0.0, bottomWidth);						// bottom right

		// fill in our color values for this laser; this is pretty inefficient since we're not using instanced
		// rendering---I'll need to implement a better way to store per-laser colours (the upside is that this
		// technique makes it very easy to potentially allow multiple colours per laser)
		for(i = 0; i < 8; i++)
		{
			addColor(color.x, color.y, color.z, color.w);
		}

		// finally, fill in our depths; for now, we're going to make the (slightly inaccurate) assumption that
		// the four vertices on each end of the laser have depth values equal to the projected depths of the
		// laser end points...this is a close enough approximation that it still looks good; if we wanted to
		// improve this, we could compute and project 8 vertices of the laser, instead of just the two endpoints,
		// but this would probably be overkill anyways
		for(i = 0; i < 4; i ++)
			addDepth(orthoP1.z);
		for(i = 0; i < 4; i ++)
			addDepth(orthoP2.z);

		// track one more laser
		activeLasers ++;
	}
}

void LaserManager::addVertex(float x, float y)
{
	*currentVertex++ = x;
	*currentVertex++ = y;
}

void LaserManager::addTexCoord(float s, float t, float q)
{
	*currentTexCoord++ = s;
	*currentTexCoord++ = t;
	*currentTexCoord++ = q;
}

void LaserManager::addColor(float r, float g, float b, float a)
{
	*currentColor++ = r;
	*currentColor++ = g;
	*currentColor++ = b;
	*currentColor++ = a;
}

void LaserManager::addDepth(float depth)
{
	*currentDepth++ = depth;
}

void LaserManager::render(bool showGeometry)
{
	// no transform on the lasers we render
	matrix_t model = matrix_t(1.0);

	// activate our vertex array object
	qglBindVertexArray(vao);

	// pass in our vertex data
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	qglBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * activeLasers * 16, vertices);

	// pass in our texture coordinate data
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
	qglBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * activeLasers * 24, texCoords);

	// pass in our color data
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
	qglBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * activeLasers * 32, colors);

	// pass in our depth data
	qglBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
	qglBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * activeLasers * 8, depths);

	// activate our shader
    laserShader -> bind();
	laserShader -> uniformMatrix4fv("u_Projection", 1, value_ptr(orthoProjection));
	laserShader -> uniformMatrix4fv("u_Model", 1, value_ptr(model));
	laserShader -> uniformMatrix4fv("u_View", 1, value_ptr(orthoView));

	// now render everything we have in one fell swoop
	qglBindTexture(GL_TEXTURE_2D, texture);
	qglDrawElements(GL_TRIANGLES, 18 * activeLasers, GL_UNSIGNED_INT, NULL);

	// do we also show the mesh outline of the lasers we just rendered?
	if(showGeometry)
	{
		// activate our shader
		outlineShader -> bind();
		outlineShader -> uniformMatrix4fv("u_Projection", 1, value_ptr(orthoProjection));
		outlineShader -> uniformMatrix4fv("u_Model", 1, value_ptr(model));
		outlineShader -> uniformMatrix4fv("u_View", 1, value_ptr(orthoView));

		// now render everything we have in one fell swoop
		glBindTexture(GL_TEXTURE_2D, texture);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, 18 * activeLasers, GL_UNSIGNED_INT, NULL);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}
#endif
