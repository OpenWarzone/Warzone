/*
 *  mat4.h
 *  asrTracer
 *
 *  Created by Petrik Clarberg on 2006-02-22.
 *  Copyright 2006 Lund University. All rights reserved.
 *
 */

#ifndef mat4_H
#define mat4_H

#include <iostream>
#include <stdexcept>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265359f
#endif

class vec2;
class vec3;
class vec4;
class mat4;

/**
 * Class representing a 4x4 transformation mat4.
 */
class mat4
{
public:
	/// Default constructor. The mat4 is initialized to identity.
	mat4() { identity(); }
		
	/// Constructor initializing all elements to the given value.
	explicit mat4(float s) { for(int i=0; i<16; i++) e[i]=s; }

    /// Constructor initializing matrix with four row vectors
    mat4( const vec4& a, const vec4& b, const vec4& c, const vec4& d );
	
	/// Assignment from a scalar value.
	mat4& operator=(float s) { for(int i=0; i<16; i++) e[i]=s; return *this; }
	
	/// Returns a copy of the element at position row,column (i,j).
	/// @remark The range of the parameters (i,j) is not checked.
	float operator()(int i, int j) const { return e[4*i+j]; }
	
	/// Returns a reference to the element at position row,column (i,j).
	/// @remark The range of the parameters (i,j) is not checked.
	float& operator()(int i, int j) { return e[4*i+j]; }
    
    float& operator[](int index) {
        return e[index];
    }

    const float& operator[](int index) const {
        return e[index];
    }
	
	/// Clear all elements to zero.
	mat4& zero() { for(int i=0; i<16; i++) e[i]=0.0f; return *this; }
	
	/// Set mat4 to identity.
	mat4& identity()
	{
		e[0]  = 1.0f;	e[1]  = 0.0f;	e[2]  = 0.0f;	e[3]  = 0.0f;
		e[4]  = 0.0f;	e[5]  = 1.0f;	e[6]  = 0.0f;	e[7]  = 0.0f;
		e[8]  = 0.0f;	e[9]  = 0.0f;	e[10] = 1.0f;	e[11] = 0.0f;
		e[12] = 0.0f;	e[13] = 0.0f;	e[14] = 0.0f;	e[15] = 1.0f;			
		return *this;
	}
	
	/// Setup scaling mat4.
	mat4& scale(float sx, float sy, float sz) { identity(); e[0]=sx; e[5]=sy; e[10]=sz; return *this; }
	
	/// Setup translation mat4.
	mat4& translation(float tx, float ty, float tz) { identity(); e[3]=tx; e[7]=ty; e[11]=tz; return *this; }
	
	/// Setup rotation mat4 around the x-axis, where the rotation angle is given in degrees.
	mat4& rotX(float rx);

	/// Setup rotation mat4 around the y-axis, where the rotation angle is given in degrees.
	mat4& rotY(float ry);

	/// Setup rotation mat4 around the z-axis, where the rotation angle is given in degrees.
	mat4& rotZ(float rz);
	
	/// Addition of two matrices.
	mat4 operator+(const mat4& m) const { mat4 r; for(int i=0;i<16;i++) r.e[i]=e[i]+m.e[i]; return r; }
			
	/// Addition of two matrices in place.
	mat4& operator+=(const mat4& m) { for(int i=0;i<16;i++) e[i]+=m.e[i]; return *this; }

	/// Subtraction of two matrices.
	mat4 operator-(const mat4& m) const { mat4 r; for(int i=0;i<16;i++) r.e[i]=e[i]-m.e[i]; return r; }
	
	/// Subtraction of two matrices in place.
	mat4& operator-=(const mat4& m) { for(int i=0;i<16;i++) e[i]-=m.e[i]; return *this; }
	
	/// Multiplication of sclar by mat4.
	friend mat4 operator*(float s, const mat4& m) { mat4 r; for(int i=0;i<16;i++) r.e[i]=s*m.e[i]; return r; }
	
	/// Multiplication of mat4 by scalar.
	mat4 operator*(float s) const { mat4 r; for(int i=0;i<16;i++) r.e[i]=s*e[i]; return r; }
	
	/// Multiplication by scalar in place.
	mat4& operator*=(float s) { for(int i=0;i<16;i++) e[i]*=s; return *this; }
	
	/// mat4/vec4 multiplication.
	vec4 operator*(const vec4& p) const;

	/// mat4 multiplication.
	mat4 operator*(const mat4& m) const
	{
		mat4 r(0);
		for(int i=0; i<4; i++)
			for(int j=0; j<4; j++)
				for(int k=0; k<4; k++)
					r(i,j) += (*this)(i,k) * m(k,j);
		return r;
	}
	
	/// mat4 multiplication in place.
	mat4& operator*=(const mat4& m)
	{
		mat4 r(0);
		for(int i=0; i<4; i++)
			for(int j=0; j<4; j++)
				for(int k=0; k<4; k++)
					r(i,j) += (*this)(i,k) * m(k,j);
		return (*this = r);
	}

	/// Compute the determinant of the mat4.
	float determinant() const;
	
	/// Compute the inverse of the mat4.
	mat4 inverse() const;

	/// Compute the transpose of the mat4.
	mat4 transpose() const;
    
    float* getFloatArray() {return e;}
    
	/// Write mat4 elements to an output stream.
	friend std::ostream& operator<<(std::ostream& os, const mat4& A);
			
protected:
	float e[16];			///< The mat4 elements.

	friend class vec3;
	friend class point3;
};

/**
 * Class representing a 2D vector.
 */
class vec2
{
public:
	float x;		///< First component.
	float y;		///< Second component.
    
public:
	/// Default constructor. Initializes vector to (0,0).
	vec2() : x(0.0f), y(0.0f) { }
    
	/// Constructor initializing vector to (x,y).
	vec2(const vec2 &v) : x(v.x), y(v.y) { }
    
	/// Constructor initializing all elements to the given value.
	explicit vec2(float s) : x(s), y(s) { }
    
	/// Constructor initializing vector to (x,y).
	vec2(float _x, float _y) : x(_x), y(_y) { }
	
	/// Returns a copy of the element at position (i).
	/// @remark The range of the parameter is not checked.
	float operator()(int i) const { return (&x)[i]; }
	
	/// Returns a reference to the element at position (i).
	/// @remark The range of the parameter is not checked.
	float& operator()(int i) { return (&x)[i]; }
	
	/// Addition of two vectors.
	vec2 operator+(const vec2& v) const { return vec2(x+v.x, y+v.y); }
    
	/// Addition of two vectors in place.
	vec2& operator+=(const vec2& v) { x+=v.x; y+=v.y; return *this; }
	
	/// Negation of vector.
	vec2 operator-() const { return vec2(-x,-y); }
	
	/// Subtraction of two vectors.
	vec2 operator-(const vec2& v) const { return vec2(x-v.x, y-v.y); }
    
	/// Subtraction of two vectors in place.
	vec2& operator-=(const vec2& v) { x-=v.x; y-=v.y; return *this; }
	
	/// Subtraction of scalar.
	vec2 operator-(const float& s) const { return vec2(x-s, y-s); }
    
	/// Subtraction of scalar in place.
	vec2& operator-=(const float& s) { x-=s; y-=s; return *this; }
    
	/// Multiplication of scalar by vector.
	friend vec2 operator*(float s, const vec2& v) { return vec2(s*v.x,s*v.y); }
	
	/// Multiplication of vector by scalar.
	vec2 operator*(float s) const { return vec2(s*x,s*y); }
	
	/// Multiplication by scalar in place.
	vec2& operator*=(float s) { x*=s; y*=s; return *this; }
    
	/// Division of vector by scalar.
	vec2 operator/(float s) const { s=1.0f/s; return vec2(s*x,s*y); }
    
	/// Division by scalar in place.
	vec2& operator/=(float s) { s=1.0f/s; x*=s; y*=s; return *this; }
	
	/// Division of scalar by vector.
	friend vec2 operator/(float s, const vec2& v) { return vec2(s/v.x,s/v.y); }
    	
	/// Returns length of vector.
	float length() const { return std::sqrt(x*x+y*y); }
    
	/// Returns square length of vector.
	float length2() const { return x*x+y*y; }
    
	/// Normalize vector to unit length.
	vec2& normalize() { float l=1.0f/length(); x*=l; y*=l; return *this; }
	
	/// Write elements to an output stream.
	friend std::ostream& operator<<(std::ostream& os, const vec2& A);
};

/**
 * Class representing a 3D vector.
 */
class vec3
{
public:
	float x;		///< First component.
	float y;		///< Second component.
	float z;		///< Third component.
		
public:
	/// Default constructor. Initializes vector to (0,0,0).
	vec3() : x(0.0f), y(0.0f), z(0.0f) { }

	/// Constructor initializing vector to (x,y,0).
	vec3(vec2 &v);

	/// Constructor initializing all elements to the given value.
	explicit vec3(float s) : x(s), y(s), z(s) { }

	/// Constructor initializing vector to (x,y,z).
	vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) { }
	
	/// Returns a copy of the element at position (i).
	/// @remark The range of the parameter is not checked.
	float operator()(int i) const { return (&x)[i]; }
	
	/// Returns a reference to the element at position (i).
	/// @remark The range of the parameter is not checked.
	float& operator()(int i) { return (&x)[i]; }
	
	/// Addition of two vectors.
	vec3 operator+(const vec3& v) const { return vec3(x+v.x, y+v.y, z+v.z); }

	/// Addition of two vectors in place.
	vec3& operator+=(const vec3& v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
	
	/// Negation of vector.
	vec3 operator-() const { return vec3(-x,-y,-z); }
	
	/// Subtraction of two vectors.
	vec3 operator-(const vec3& v) const { return vec3(x-v.x, y-v.y, z-v.z); }

	/// Subtraction of two vectors in place.
	vec3& operator-=(const vec3& v) { x-=v.x; y-=v.y; z-=v.z; return *this; }
	
	/// Multiplication of scalar by vector.
	friend vec3 operator*(float s, const vec3& v) { return vec3(s*v.x,s*v.y,s*v.z); }
	
	/// Multiplication of vector by scalar.
	vec3 operator*(float s) const { return vec3(s*x,s*y,s*z); }
	
	/// Multiplication by scalar in place.
	vec3& operator*=(float s) { x*=s; y*=s; z*=s; return *this; }

	/// Division of vector by scalar.
	vec3 operator/(float s) const { s=1.0f/s; return vec3(s*x,s*y,s*z); }

	/// Division by scalar in place.
	vec3& operator/=(float s) { s=1.0f/s; x*=s; y*=s; z*=s; return *this; }
	
	/// Division of scalar by vector.
	friend vec3 operator/(float s, const vec3& v) { return vec3(s/v.x,s/v.y,s/v.z); }
			
	/// Returns length of vector.
	float length() const { return std::sqrt(x*x+y*y+z*z); }

	/// Returns square length of vector.
	float length2() const { return x*x+y*y+z*z; }

	/// Normalize vector to unit length.
	vec3& normalize() { float l=1.0f/length(); x*=l; y*=l; z*=l; return *this; }
	
	/// Write elements to an output stream.
	friend std::ostream& operator<<(std::ostream& os, const vec3& A);
};

// Non class vec3 functions
inline
vec3 cross(const vec3& a, const vec3& b )
{
    return vec3( a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x );
}

inline
float dot(const vec3& a, const vec3& b)
{
    return a.x*b.x + a.y*b.y+a.z*b.z;
}

// ++++++++++++++++++
/**
 * Class representing a 4D vector.
 */
class vec4
{
public:
	float x;		///< First component.
	float y;		///< Second component.
	float z;		///< Third component.
    float w;		///< Fourth component.
    
public:
	/// Default constructor. Initializes vector to (0,0,0,0).
	vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
        
	/// Constructor initializing all elements to the given value.
	explicit vec4(float s) : x(s), y(s), z(s), w(s) { }
    
	/// Constructor initializing vector to (x,y,z,w).
	vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) { }
    
    /// Constructor initializing vector from a vec3 and a float w.
	vec4(const vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) { }

    	
	/// Returns a copy of the element at position (i).
	/// @remark The range of the parameter is not checked.
	float operator()(int i) const { return (&x)[i]; }
	
	/// Returns a reference to the element at position (i).
	/// @remark The range of the parameter is not checked.
	float& operator()(int i) { return (&x)[i]; }
	
	/// Addition of two vectors.
	vec4 operator+(const vec4& v) const { return vec4(x+v.x, y+v.y, z+v.z, w+v.w); }
    
	/// Addition of two vectors in place.
	vec4& operator+=(const vec4& v) { x+=v.x; y+=v.y; z+=v.z; w+=v.w; return *this; }
	
	/// Negation of vector.
	vec4 operator-() const { return vec4(-x,-y,-z,-w); }
	
	/// Subtraction of two vectors.
	vec4 operator-(const vec4& v) const { return vec4(x-v.x, y-v.y, z-v.z, w-v.w); }
    
	/// Subtraction of two vectors in place.
	vec4& operator-=(const vec4& v) { x-=v.x; y-=v.y; z-=v.z; w-=v.w; return *this; }
	
	/// Multiplication of scalar by vector.
	friend vec4 operator*(float s, const vec4& v) { return vec4(s*v.x,s*v.y,s*v.z, s*v.w); }
	
	/// Multiplication of vector by scalar.
	vec4 operator*(float s) const { return vec4(s*x,s*y,s*z,s*w); }
	
	/// Multiplication by scalar in place.
	vec4& operator*=(float s) { x*=s; y*=s; z*=s; w*=s; return *this; }
    
	/// Division of vector by scalar.
	vec4 operator/(float s) const { s=1.0f/s; return vec4(s*x,s*y,s*z, s*w); }
    
	/// Division by scalar in place.
	vec4& operator/=(float s) { s=1.0f/s; x*=s; y*=s; z*=s; w*=s; return *this; }
	
	/// Division of scalar by vector.
	friend vec4 operator/(float s, const vec4& v) { return vec4(s/v.x,s/v.y,s/v.z, s/v.w); }
        			
	/// Returns length of vector.
	float length() const { return std::sqrt(x*x+y*y+z*z+w*w); }
    
	/// Returns square length of vector.
	float length2() const { return x*x+y*y+z*z+w*w; }
    
	/// Normalize vector to unit length.
	vec4& normalize() { float l=1.0f/length(); x*=l; y*=l; z*=l; w*=l; return *this; }
    
    vec3 xyz() {return vec3(x,y,z);}
	
	/// Write elements to an output stream.
	friend std::ostream& operator<<(std::ostream& os, const vec4& A);
};

// Non class vec4 functions
/*
float dot(const vec4& a, const vec4& b)
{
    return a.x*b.x + a.y*b.y+a.z*b.z+a.w*b.w;
}
 */

// Non-class matrix function

// OpenGL LookAt function
inline
mat4 LookAt(const vec3& eye, const vec3& at, const vec3& up )
{
    vec3 w = eye - at;
    w.normalize();
    vec3 u = cross(up,w);
    u.normalize();
    vec3 v = cross(w,u);
    v.normalize();
    vec4 t = vec4(0.0, 0.0, 0.0, 1.0);
    mat4 c = mat4(vec4(u,0), vec4(v,0), vec4(w,0), t);
    mat4 T;
    T.translation(-eye.x, -eye.y, -eye.z);
    return c * T;
}

inline
mat4 Perspective(const float fovy, const float aspect,
                 const float znear, const float zfar)
{
    float top   = (float)tan(fovy*M_PI /360.0) * znear;
    float right = top * aspect;
    
    mat4 c(0);
    c(0,0) = znear/right;
    c(1,1) = znear/top;
    c(2,2) = -(zfar + znear)/(zfar - znear);
    c(2,3) = -2.0f*zfar*znear/(zfar - znear);
    c(3,2) = -1.0f;
    return c;
}


#endif
