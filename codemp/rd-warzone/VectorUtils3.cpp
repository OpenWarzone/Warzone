// VectorUtils
// Vector and matrix manipulation library for OpenGL, a package of the most essential functions.
// Includes:
// - Basic vector operations: Add, subtract, scale, dot product, cross product.
// - Basic matrix operations: Multiply matrix to matrix, matric to vector, transpose.
// - Creation of transformation matrixces: Translation, scaling, rotation.
// - A few more special operations: Orthonormalizaton of a matrix, CrossMatrix,
// - Replacements of some GLU functions: lookAt, frustum, perspective.
// - Inverse and inverse transpose for creating normal matrices.
// Supports both C and C++. The C interface makes it accessible from most languages if desired.

// A note on completeness:
// All operations may not be 100% symmetrical over all types, and some GLSL types are
// missing (like vec2). These will be added if proven important. There is already
// some calls of minor importance (mat3 * mat3, mat3 * vec3) included only for
// symmetry. I don't want the code to grow a lot for such reasons, I want it to be
// compact and to the point.

// Current open design questions:
// Naming conventions: Lower case or capitalized? (Frustum/frustum)
// Prefix for function calls? (The cost would be more typing and making the code harder to read.)
// Add vector operations for vec4? Other *essential* symmetry issues?
// Names for functions when supporting both vec3 and vec4, mat3 and mat4? (vec3Add, vec4Add?)



// History:

// VectorUtils is a small (but growing) math unit by Ingemar Ragnemalm.
// It originated as "geom3d" by Andrew Meggs, but that unit is no more
// than inspiration today. The original VectorUtils(1) was based on it,
// while VectorUtils2 was a rewrite primarily to get rid of the over-use
// of arrays in the original.

// New version 120201:
// Defaults to matrices "by the book". Can also be configured to the flipped
// column-wise matrices that old OpenGL required (and we all hated).
// This is freshly implemented, limited testing, so there can be bugs.
// But it seems to work just fine on my tests with translation, rotations
// and matrix multiplications.

// 1208??: Added lookAt, perspective, frustum
// Also made Transpose do what it should. TransposeRotation is the old function.
// 120913: Fixed perspective. Never trust other's code...
// 120925: Transposing in CrossMatrix
// 121119: Fixed one more glitch in perspective.

// 130227 First draft to a version 3.
// C++ operators if accessed from C++
// Vectors by value when possible. Return values on the stack.
// (Why was this not the case in VectorUtils2? Beause I tried to stay compatible
// with an old C compiler. Older C code generally prefers all non-scalar data by
// reference. But I'd like to move away from that now.)
// Types conform with GLSL as much as possible (vec3, mat4)
// Added some operations for mat3 and vec4, but most of them are more for
// completeness than usefulness; I find vec3's and mat4's to be what I use
// most of the time.
// Also added InvertMat3 and InversTranspose to support creation of normal matrices.
// Marked some calls for removal: CopyVector, TransposeRotation, CopyMatrix.
// 130308: Added InvertMat4 and some more vec3/vec4 operators (+= etc)
// 130922: Fixed a vital bug in CrossMatrix.
// 130924: Fixed a bug in mat3tomat4.
// 131014: Added TransposeMat3 (although I doubt its importance)
// 140213: Corrected mat3tomat4. (Were did the correction in 130924 go?)

// You may use VectorUtils as you please. A reference to the origin is appreciated
// but if you grab some snippets from it without reference... no problem.


#include "VectorUtils3.h"

#ifdef __INSTANCED_MODELS__

char transposed = 0;

// Should be obsolete // Will probably be removed
//	void CopyVector(Point3D *v, Point3D *dest)
//	{
//		dest->x = v->x;
//		dest->y = v->y;
//		dest->z = v->z;
//	}

// Obsolete?
	vec3 SetVector(GLfloat x, GLfloat y, GLfloat z)
	{
		vec3 v;

		v.x = x;
		v.y = y;
		v.z = z;
		return v;
	}

	// vec3 operations
	// vec4 operations can easily be added but I havn't seen much need for them.
	// Some are defined as C++ operators though.

	vec3 VectorSub(vec3 a, vec3 b)
	{
		vec3 result;

		result.x = a.x - b.x;
		result.y = a.y - b.y;
		result.z = a.z - b.z;
		return result;
	}

	vec3 VectorAdd(vec3 a, vec3 b)
	{
		vec3 result;

		result.x = a.x + b.x;
		result.y = a.y + b.y;
		result.z = a.z + b.z;
		return result;
	}

	vec3 CrossProduct(vec3 a, vec3 b)
	{
		vec3 result;

		result.x = a.y*b.z - a.z*b.y;
		result.y = a.z*b.x - a.x*b.z;
		result.z = a.x*b.y - a.y*b.x;

		return result;
	}

	GLfloat DotProduct(vec3 a, vec3 b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	vec3 ScalarMult(vec3 a, GLfloat s)
	{
		vec3 result;

		result.x = a.x * s;
		result.y = a.y * s;
		result.z = a.z * s;

		return result;
	}

	GLfloat Norm(vec3 a)
	{
		GLfloat result;

		result = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
		return result;
	}

	vec3 Normalize(vec3 a)
	{
		GLfloat norm;
		vec3 result;

		norm = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
		result.x = a.x / norm;
		result.y = a.y / norm;
		result.z = a.z / norm;
		return result;
	}

	vec3 CalcNormalVector(vec3 a, vec3 b, vec3 c)
	{
		vec3 n;

		n = CrossProduct(VectorSub(a, b), VectorSub(a, c));
		n = ScalarMult(n, 1/Norm(n));

		return n;
	}

// Splits v into vn (parallell to n) and vp (perpendicular). Does not demand n to be normalized.
	void SplitVector(vec3 v, vec3 n, vec3 *vn, vec3 *vp)
	{
		GLfloat nlen;
		GLfloat nlen2;

		nlen = DotProduct(v, n);
		nlen2 = n.x*n.x+n.y*n.y+n.z*n.z; // Squared length
		if (nlen2 == 0)
		{
			*vp = v;
			*vn = SetVector(0, 0, 0);
		}
		else
		{
			*vn = ScalarMult(n, nlen/nlen2);
			*vp = VectorSub(v, *vn);
		}
	}

// Matrix operations primarily on 4x4 matrixes!
// Row-wise by default but can be configured to column-wise (see SetTransposed)

	mat4 IdentityMatrix()
	{
		mat4 m;
		int i;

		for (i = 0; i <= 15; i++)
			m.m[i] = 0;
		for (i = 0; i <= 3; i++)
			m.m[i * 5] = 1; // 0,5,10,15
		return m;
	}

	mat4 Rx(GLfloat a)
	{
		mat4 m;
		m = IdentityMatrix();
		m.m[5] = cos(a);
		if (transposed)
			m.m[9] = -sin(a);
		else
			m.m[9] = sin(a);
		m.m[6] = -m.m[9]; //sin(a);
		m.m[10] = m.m[5]; //cos(a);
		return m;
	}

	mat4 Ry(GLfloat a)
	{
		mat4 m;
		m = IdentityMatrix();
		m.m[0] = cos(a);
		if (transposed)
			m.m[8] = -sin(a);
		else
			m.m[8] = sin(a);
		m.m[2] = -m.m[8]; //sin(a);
		m.m[10] = m.m[0]; //cos(a);
		return m;
	}

	mat4 Rz(GLfloat a)
	{
		mat4 m;
		m = IdentityMatrix();
		m.m[0] = cos(a);
		if (transposed)
			m.m[4] = -sin(a);
		else
			m.m[4] = sin(a);
		m.m[1] = -m.m[4]; //sin(a);
		m.m[5] = m.m[0]; //cos(a);
		return m;
	}

	mat4 T(GLfloat tx, GLfloat ty, GLfloat tz)
	{
		mat4 m;
		m = IdentityMatrix();
		if (transposed)
		{
			m.m[12] = tx;
			m.m[13] = ty;
			m.m[14] = tz;
		}
		else
		{
			m.m[3] = tx;
			m.m[7] = ty;
			m.m[11] = tz;
		}
		return m;
	}

	mat4 S(GLfloat sx, GLfloat sy, GLfloat sz)
	{
		mat4 m;
		m = IdentityMatrix();
		m.m[0] = sx;
		m.m[5] = sy;
		m.m[10] = sz;
		return m;
	}

	mat4 Mult(mat4 a, mat4 b) // m = a * b
	{
		mat4 m;

		int x, y;
		for (x = 0; x <= 3; x++)
			for (y = 0; y <= 3; y++)
				if (transposed)
					m.m[x*4 + y] =	a.m[y+4*0] * b.m[0+4*x] +
								a.m[y+4*1] * b.m[1+4*x] +
								a.m[y+4*2] * b.m[2+4*x] +
								a.m[y+4*3] * b.m[3+4*x];
				else
					m.m[y*4 + x] =	a.m[y*4+0] * b.m[0*4+x] +
								a.m[y*4+1] * b.m[1*4+x] +
								a.m[y*4+2] * b.m[2*4+x] +
								a.m[y*4+3] * b.m[3*4+x];

		return m;
	}

	// Ej testad!
	mat3 MultMat3(mat3 a, mat3 b) // m = a * b
	{
		mat3 m;

		int x, y;
		for (x = 0; x <= 2; x++)
			for (y = 0; y <= 2; y++)
				if (transposed)
					m.m[x*3 + y] =	a.m[y+3*0] * b.m[0+3*x] +
								a.m[y+3*1] * b.m[1+3*x] +
								a.m[y+3*2] * b.m[2+3*x];
				else
					m.m[y*3 + x] =	a.m[y*3+0] * b.m[0*3+x] +
								a.m[y*3+1] * b.m[1*3+x] +
								a.m[y*3+2] * b.m[2*3+x];

		return m;
	}

	// mat4 * vec3
	// The missing homogenous coordinate is implicitly set to 1.
	vec3 MultVec3(mat4 a, vec3 b) // result = a * b
	{
		vec3 r;

		if (!transposed)
		{
			r.x = a.m[0]*b.x + a.m[1]*b.y + a.m[2]*b.z + a.m[3];
			r.y = a.m[4]*b.x + a.m[5]*b.y + a.m[6]*b.z + a.m[7];
			r.z = a.m[8]*b.x + a.m[9]*b.y + a.m[10]*b.z + a.m[11];
		}
		else
		{
			r.x = a.m[0]*b.x + a.m[4]*b.y + a.m[8]*b.z + a.m[12];
			r.y = a.m[1]*b.x + a.m[5]*b.y + a.m[9]*b.z + a.m[13];
			r.z = a.m[2]*b.x + a.m[6]*b.y + a.m[10]*b.z + a.m[14];
		}

		return r;
	}

	// mat3 * vec3
	vec3 MultMat3Vec3(mat3 a, vec3 b) // result = a * b
	{
		vec3 r;

		if (!transposed)
		{
			r.x = a.m[0]*b.x + a.m[1]*b.y + a.m[2]*b.z;
			r.y = a.m[3]*b.x + a.m[4]*b.y + a.m[5]*b.z;
			r.z = a.m[6]*b.x + a.m[7]*b.y + a.m[8]*b.z;
		}
		else
		{
			r.x = a.m[0]*b.x + a.m[3]*b.y + a.m[6]*b.z;
			r.y = a.m[1]*b.x + a.m[4]*b.y + a.m[7]*b.z;
			r.z = a.m[2]*b.x + a.m[5]*b.y + a.m[8]*b.z;
		}

		return r;
	}

	// mat4 * vec4
	vec4 MultVec4(mat4 a, vec4 b) // result = a * b
	{
		vec4 r;

		if (!transposed)
		{
			r.x = a.m[0]*b.x + a.m[1]*b.y + a.m[2]*b.z + a.m[3]*b.w;
			r.y = a.m[4]*b.x + a.m[5]*b.y + a.m[6]*b.z + a.m[7]*b.w;
			r.z = a.m[8]*b.x + a.m[9]*b.y + a.m[10]*b.z + a.m[11]*b.w;
			r.w = a.m[12]*b.x + a.m[13]*b.y + a.m[14]*b.z + a.m[15]*b.w;
		}
		else
		{
			r.x = a.m[0]*b.x + a.m[4]*b.y + a.m[8]*b.z + a.m[12]*b.w;
			r.y = a.m[1]*b.x + a.m[5]*b.y + a.m[9]*b.z + a.m[13]*b.w;
			r.z = a.m[2]*b.x + a.m[6]*b.y + a.m[10]*b.z + a.m[14]*b.w;
			r.w = a.m[3]*b.x + a.m[7]*b.y + a.m[11]*b.z + a.m[15]*b.w;
		}

		return r;
	}


// Unnecessary
// Will probably be removed
//	void CopyMatrix(GLfloat *src, GLfloat *dest)
//	{
//		int i;
//		for (i = 0; i <= 15; i++)
//			dest[i] = src[i];
//	}


// Added for lab 3 (TSBK03)

	// Orthonormalization of Matrix4D. Assumes rotation only, translation/projection ignored
	void OrthoNormalizeMatrix(mat4 *R)
	{
		vec3 x, y, z;

		if (transposed)
		{
			x = SetVector(R->m[0], R->m[1], R->m[2]);
			y = SetVector(R->m[4], R->m[5], R->m[6]);
//		SetVector(R[8], R[9], R[10], &z);
			// Kryssa fram ur varandra
			// Normera
			z = CrossProduct(x, y);
			z = Normalize(z);
			x = Normalize(x);
			y = CrossProduct(z, x);
			R->m[0] = x.x;
			R->m[1] = x.y;
			R->m[2] = x.z;
			R->m[4] = y.x;
			R->m[5] = y.y;
			R->m[6] = y.z;
			R->m[8] = z.x;
			R->m[9] = z.y;
			R->m[10] = z.z;

			R->m[3] = 0.0;
			R->m[7] = 0.0;
			R->m[11] = 0.0;
			R->m[12] = 0.0;
			R->m[13] = 0.0;
			R->m[14] = 0.0;
			R->m[15] = 1.0;
		}
		else
		{
		// NOT TESTED
			x = SetVector(R->m[0], R->m[4], R->m[8]);
			y = SetVector(R->m[1], R->m[5], R->m[9]);
//		SetVector(R[2], R[6], R[10], &z);
			// Kryssa fram ur varandra
			// Normera
			z = CrossProduct(x, y);
			z = Normalize(z);
			x = Normalize(x);
			y = CrossProduct(z, x);
			R->m[0] = x.x;
			R->m[4] = x.y;
			R->m[8] = x.z;
			R->m[1] = y.x;
			R->m[5] = y.y;
			R->m[9] = y.z;
			R->m[2] = z.x;
			R->m[6] = z.y;
			R->m[10] = z.z;

			R->m[3] = 0.0;
			R->m[7] = 0.0;
			R->m[11] = 0.0;
			R->m[12] = 0.0;
			R->m[13] = 0.0;
			R->m[14] = 0.0;
			R->m[15] = 1.0;
		}
	}


// Commented out since I plan to remove it if I can't see a good reason to keep it.
//	// Only transposes rotation part.
//	mat4 TransposeRotation(mat4 m)
//	{
//		mat4 a;
//
//		a.m[0] = m.m[0]; a.m[4] = m.m[1]; a.m[8] = m.m[2];      a.m[12] = m.m[12];
//		a.m[1] = m.m[4]; a.m[5] = m.m[5]; a.m[9] = m.m[6];      a.m[13] = m.m[13];
//		a.m[2] = m.m[8]; a.m[6] = m.m[9]; a.m[10] = m.m[10];    a.m[14] = m.m[14];
//		a.m[3] = m.m[3]; a.m[7] = m.m[7]; a.m[11] = m.m[11];    a.m[15] = m.m[15];
//
//		return a;
//	}

	// Complete transpose!
	mat4 Transpose(mat4 m)
	{
		mat4 a;

		a.m[0] = m.m[0]; a.m[4] = m.m[1]; a.m[8] = m.m[2];      a.m[12] = m.m[3];
		a.m[1] = m.m[4]; a.m[5] = m.m[5]; a.m[9] = m.m[6];      a.m[13] = m.m[7];
		a.m[2] = m.m[8]; a.m[6] = m.m[9]; a.m[10] = m.m[10];    a.m[14] = m.m[11];
		a.m[3] = m.m[12]; a.m[7] = m.m[13]; a.m[11] = m.m[14];    a.m[15] = m.m[15];

		return a;
	}

	// Complete transpose!
	mat3 TransposeMat3(mat3 m)
	{
		mat3 a;

		a.m[0] = m.m[0]; a.m[3] = m.m[1]; a.m[6] = m.m[2];
		a.m[1] = m.m[3]; a.m[4] = m.m[4]; a.m[7] = m.m[5];
		a.m[2] = m.m[6]; a.m[5] = m.m[7]; a.m[8] = m.m[8];

		return a;
	}

// Rotation around arbitrary axis (rotation only)
mat4 ArbRotate(vec3 axis, GLfloat fi)
{
	vec3 x, y, z;
	mat4 R, Rt, Raxel, m;

// Check if parallel to Z
	if (axis.x < 0.0000001) // Below some small value
	if (axis.x > -0.0000001)
	if (axis.y < 0.0000001)
	if (axis.y > -0.0000001)
	{
		if (axis.z > 0)
		{
			m = Rz(fi);
			return m;
		}
		else
		{
			m = Rz(-fi);
			return m;
		}
	}

	x = Normalize(axis);
	z = SetVector(0,0,1); // Temp z
	y = Normalize(CrossProduct(z, x)); // y' = z^ x x'
	z = CrossProduct(x, y); // z' = x x y

	if (transposed)
	{
		R.m[0] = x.x; R.m[4] = x.y; R.m[8] = x.z;  R.m[12] = 0.0;
		R.m[1] = y.x; R.m[5] = y.y; R.m[9] = y.z;  R.m[13] = 0.0;
		R.m[2] = z.x; R.m[6] = z.y; R.m[10] = z.z;  R.m[14] = 0.0;

		R.m[3] = 0.0; R.m[7] = 0.0; R.m[11] = 0.0;  R.m[15] = 1.0;
	}
	else
	{
		R.m[0] = x.x; R.m[1] = x.y; R.m[2] = x.z;  R.m[3] = 0.0;
		R.m[4] = y.x; R.m[5] = y.y; R.m[6] = y.z;  R.m[7] = 0.0;
		R.m[8] = z.x; R.m[9] = z.y; R.m[10] = z.z;  R.m[11] = 0.0;

		R.m[12] = 0.0; R.m[13] = 0.0; R.m[14] = 0.0;  R.m[15] = 1.0;
	}

	Rt = Transpose(R); // Transpose = Invert -> felet ej i Transpose, och det är en ortonormal matris

	Raxel = Rx(fi); // Rotate around x axis

	// m := Rt * Rx * R
	m = Mult(Mult(Rt, Raxel), R);

	return m;
}




// Inte testad mycket. Hoppas jag inte vänt på den.
mat4 CrossMatrix(vec3 a) // Skapar matris för kryssprodukt
{
	mat4 m;

	if (transposed)
	{
		m.m[0] =    0; m.m[4] =-a.z; m.m[8] = a.y; m.m[12] = 0.0;
		m.m[1] = a.z; m.m[5] =    0; m.m[9] =-a.x; m.m[13] = 0.0;
		m.m[2] =-a.y; m.m[6] = a.x; m.m[10]=    0; m.m[14] = 0.0;
		m.m[3] =  0.0; m.m[7] =  0.0; m.m[11]=  0.0; m.m[15] = 0.0;
		// OBS! 0.0 i homogena koordinaten. Därmed kan matrisen
		// inte användas generellt, men duger för matrisderivatan.
	}
	else
	{
		m.m[0] =    0; m.m[1] =-a.z; m.m[2] = a.y; m.m[3] = 0.0;
		m.m[4] = a.z; m.m[5] =    0; m.m[6] =-a.x; m.m[7] = 0.0;
		m.m[8] =-a.y; m.m[9] = a.x; m.m[10]=    0; m.m[11] = 0.0;
		m.m[12] =  0.0; m.m[13] =  0.0; m.m[14]=  0.0; m.m[15] = 0.0;
		// OBS! 0.0 i homogena koordinaten. Därmed kan matrisen
		// inte användas generellt, men duger för matrisderivatan.
	}

	return m;
}

// KLART HIT

mat4 MatrixAdd(mat4 a, mat4 b)
{
	mat4 dest;

	int i;
	for (i = 0; i < 16; i++)
		dest.m[i] = a.m[i] + b.m[i];

	return dest;
}


void SetTransposed(char t)
{
	transposed = t;
}


// Build standard matrices

mat4 lookAtv(vec3 p, vec3 l, vec3 v)
{
	vec3 n, u;

	n = Normalize(VectorSub(p, l));
	u = Normalize(CrossProduct(v, n));
	v = CrossProduct(n, u);

	mat4 rot = {{ u.x, u.y, u.z, 0,
                      v.x, v.y, v.z, 0,
                      n.x, n.y, n.z, 0,
                      0,   0,   0,   1 }};
	mat4 trans;
	trans = T(-p.x, -p.y, -p.z);
	return Mult(rot, trans);
}

mat4 lookAt(GLfloat px, GLfloat py, GLfloat pz,
			GLfloat lx, GLfloat ly, GLfloat lz,
			GLfloat vx, GLfloat vy, GLfloat vz)
{
	vec3 p, l, v;

	p = SetVector(px, py, pz);
	l = SetVector(lx, ly, lz);
	v = SetVector(vx, vy, vz);

	return lookAtv(p, l, v);
}

// From http://www.opengl.org/wiki/GluPerspective_code
// Changed names and parameter order to conform with VU style
// Rewritten 120913 because it was all wrong...

// Creates a projection matrix like gluPerspective or glFrustum.
// Upload to your shader as usual.
mat4 perspective(float fovyInDegrees, float aspectRatio,
                      float znear, float zfar)
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * M_PI / 360.0);
    if (aspectRatio < 1.0)
    {
	    ymax = znear * tanf(fovyInDegrees * M_PI / 360.0);
       xmax = ymax * aspectRatio;
    }
    else
    {
	    xmax = znear * tanf(fovyInDegrees * M_PI / 360.0);
       ymax = xmax / aspectRatio;
    }

    return frustum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

mat4 frustum(float left, float right, float bottom, float top,
                  float znear, float zfar)
{
    float temp, temp2, temp3, temp4;
    mat4 matrix;

    temp = 2.0 * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    matrix.m[0] = temp / temp2; // 2*near/(right-left)
    matrix.m[1] = 0.0;
    matrix.m[2] = 0.0;
    matrix.m[3] = 0.0;
    matrix.m[4] = 0.0;
    matrix.m[5] = temp / temp3; // 2*near/(top - bottom)
    matrix.m[6] = 0.0;
    matrix.m[7] = 0.0;
    matrix.m[8] = (right + left) / temp2; // A = r+l / r-l
    matrix.m[9] = (top + bottom) / temp3; // B = t+b / t-b
    matrix.m[10] = (-zfar - znear) / temp4; // C = -(f+n) / f-n
    matrix.m[11] = -1.0;
    matrix.m[12] = 0.0;
    matrix.m[13] = 0.0;
    matrix.m[14] = (-temp * zfar) / temp4; // D = -2fn / f-n
    matrix.m[15] = 0.0;

    if (!transposed)
    	matrix = Transpose(matrix);

    return matrix;
}


// The code below is based on code from:
// http://www.dr-lex.be/random/matrix_inv.html

// Inverts mat3 (row-wise matrix)
// (For a more general inverse, try a gaussian elimination.)
mat3 InvertMat3(mat3 in)
{
	float a11, a12, a13, a21, a22, a23, a31, a32, a33;
	mat3 out;

	// Copying to internal variables both clarify the code and
	// buffers data so the output may be identical to the input!
	a11 = in.m[0];
	a12 = in.m[1];
	a13 = in.m[2];
	a21 = in.m[3];
	a22 = in.m[4];
	a23 = in.m[5];
	a31 = in.m[6];
	a32 = in.m[7];
	a33 = in.m[8];
	float DET = a11*(a33*a22-a32*a23)-a21*(a33*a12-a32*a13)+a31*(a23*a12-a22*a13);
	if (DET != 0)
	{
		out.m[0] = (a33*a22-a32*a23)/DET;
		out.m[1] = -(a33*a12-a32*a13)/DET;
		out.m[2] = (a23*a12-a22*a13)/DET;
		out.m[3] = -(a33*a21-a31*a23)/DET;
		out.m[4] = (a33*a11-a31*a13)/DET;
		out.m[5] = -(a23*a11-a21*a13)/DET;
		out.m[6] = (a32*a21-a31*a22)/DET;
		out.m[7] = -(a32*a11-a31*a12)/DET;
		out.m[8] = (a22*a11-a21*a12)/DET;
	}
	else
	{
		mat3 nanout = {{ NAN, NAN, NAN,
								NAN, NAN, NAN,
								NAN, NAN, NAN}};
		out = nanout;
	}

	return out;
}

// For making a normal matrix from a model-to-view matrix
// Takes a mat4 in, ignores 4th row/column (should just be translations),
// inverts as mat3 (row-wise matrix) and returns the transpose
mat3 InverseTranspose(mat4 in)
{
	float a11, a12, a13, a21, a22, a23, a31, a32, a33;
	mat3 out;

	// Copying to internal variables
	a11 = in.m[0];
	a12 = in.m[1];
	a13 = in.m[2];
	a21 = in.m[4];
	a22 = in.m[5];
	a23 = in.m[6];
	a31 = in.m[8];
	a32 = in.m[9];
	a33 = in.m[10];
	float DET = a11*(a33*a22-a32*a23)-a21*(a33*a12-a32*a13)+a31*(a23*a12-a22*a13);
	if (DET != 0)
	{
		out.m[0] = (a33*a22-a32*a23)/DET;
		out.m[3] = -(a33*a12-a32*a13)/DET;
		out.m[6] = (a23*a12-a22*a13)/DET;
		out.m[1] = -(a33*a21-a31*a23)/DET;
		out.m[4] = (a33*a11-a31*a13)/DET;
		out.m[7] = -(a23*a11-a21*a13)/DET;
		out.m[2] = (a32*a21-a31*a22)/DET;
		out.m[5] = -(a32*a11-a31*a12)/DET;
		out.m[8] = (a22*a11-a21*a12)/DET;
	}
	else
	{
			mat3 nanout = {{ NAN, NAN, NAN,
								NAN, NAN, NAN,
								NAN, NAN, NAN}};
		out = nanout;
	}

	return out;
}


// Simple conversions
mat3 mat4tomat3(mat4 m)
{
	mat3 result;

	result.m[0] = m.m[0];
	result.m[1] = m.m[1];
	result.m[2] = m.m[2];
	result.m[3] = m.m[4];
	result.m[4] = m.m[5];
	result.m[5] = m.m[6];
	result.m[6] = m.m[8];
	result.m[7] = m.m[9];
	result.m[8] = m.m[10];
	return result;
}

mat4 mat3tomat4(mat3 m)
{
	mat4 result;

	result.m[0] = m.m[0];
	result.m[1] = m.m[1];
	result.m[2] = m.m[2];
	result.m[3] = 0;
	result.m[4] = m.m[3];
	result.m[5] = m.m[4];
	result.m[6] = m.m[5];
	result.m[7] = 0;
	result.m[8] = m.m[6];
	result.m[9] = m.m[7];
	result.m[10] = m.m[8];
	result.m[11] = 0;

	result.m[12] = 0;
	result.m[13] = 0;
	result.m[14] = 0;
	result.m[15] = 1;
	return result;
}

vec3 vec4tovec3(vec4 v)
{
	vec3 result;
	result.x = v.x;
	result.y = v.y;
	result.z = v.z;
	return result;
}

vec4 vec3tovec4(vec3 v)
{
	vec4 result;
	result.x = v.x;
	result.y = v.y;
	result.z = v.z;
	result.w = 1;
	return result;
}


// Stol... I mean adapted from glMatrix (WebGL math unit). Almost no
// changes despite changing language! But I just might replace it with
// a gaussian elimination some time.
mat4 InvertMat4(mat4 a)
{
   mat4 b;

   float c=a.m[0],d=a.m[1],e=a.m[2],g=a.m[3],
	f=a.m[4],h=a.m[5],i=a.m[6],j=a.m[7],
	k=a.m[8],l=a.m[9],o=a.m[10],m=a.m[11],
	n=a.m[12],p=a.m[13],r=a.m[14],s=a.m[15],
	A=c*h-d*f,
	B=c*i-e*f,
	t=c*j-g*f,
	u=d*i-e*h,
	v=d*j-g*h,
	w=e*j-g*i,
	x=k*p-l*n,
	y=k*r-o*n,
	z=k*s-m*n,
	C=l*r-o*p,
	D=l*s-m*p,
	E=o*s-m*r,
	q=1/(A*E-B*D+t*C+u*z-v*y+w*x);
	b.m[0]=(h*E-i*D+j*C)*q;
	b.m[1]=(-d*E+e*D-g*C)*q;
	b.m[2]=(p*w-r*v+s*u)*q;
	b.m[3]=(-l*w+o*v-m*u)*q;
	b.m[4]=(-f*E+i*z-j*y)*q;
	b.m[5]=(c*E-e*z+g*y)*q;
	b.m[6]=(-n*w+r*t-s*B)*q;
	b.m[7]=(k*w-o*t+m*B)*q;
	b.m[8]=(f*D-h*z+j*x)*q;
	b.m[9]=(-c*D+d*z-g*x)*q;
	b.m[10]=(n*v-p*t+s*A)*q;
	b.m[11]=(-k*v+l*t-m*A)*q;
	b.m[12]=(-f*C+h*y-i*x)*q;
	b.m[13]=(c*C-d*y+e*x)*q;
	b.m[14]=(-n*u+p*B-r*A)*q;
	b.m[15]=(k*u-l*B+o*A)*q;
	return b;
}

#endif //__INSTANCED_MODELS__
