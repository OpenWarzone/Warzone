/*
 *  mat4.cpp
 *  asrTracer
 *
 *  Created by Petrik Clarberg on 2006-02-22.
 *  Copyright 2006 Lund University. All rights reserved.
 *
 */

#include <iomanip>
#include "tr_matrix.h"

// ------------------------ mat4 functions ------------------------ //

mat4::mat4( const vec4& a, const vec4& b, const vec4& c, const vec4& d )
{
    e[0]  = a.x;	e[1]  = a.y;	e[2]  = a.z;	e[3]  = a.w;
    e[4]  = b.x;	e[5]  = b.y;	e[6]  = b.z;	e[7]  = b.w;
    e[8]  = c.x;	e[9]  = c.y;	e[10] = c.z;	e[11] = c.w;
    e[12] = d.x;	e[13] = d.y;	e[14] = d.z;	e[15] = d.w;
}


/// Write mat4 elements to an output stream.
std::ostream& operator<<(std::ostream& os, const mat4& A)
{
	std::streamsize p = os.precision();
	std::streamsize w = os.width();
	for(int i=0; i<4; i++)
	{
		for(int j=0; j<4; j++) os << std::setprecision(5) << std::setw(10) << A(i,j);
		os << std::endl;
	}
	os.precision(p);
	os.width(w);
	return os;
}

/// mat4/vec4 multiplication. 
vec4 mat4::operator*(const vec4& p) const
{
	vec4 r(0);
	for(int i=0; i<4; i++)
		r(i) = (*this)(i,0)*p.x + (*this)(i,1)*p.y + (*this)(i,2)*p.z + (*this)(i,3)*p.w;
	return r;
}


/// Setup rotation mat4 around the x-axis, where the rotation angle is given in degrees.
mat4& mat4::rotX(float rx)
{
	identity();
	float a = rx*M_PI/180.0f;
	float s = std::sin(a);
	float c = std::cos(a);
	(*this)(1,1) = c;	(*this)(1,2) = -s;
	(*this)(2,1) = s;	(*this)(2,2) =  c;
	return *this;
}

/// Setup rotation mat4 around the y-axis, where the rotation angle is given in degrees.
mat4& mat4::rotY(float ry)
{
	identity();
	float a = ry*M_PI/180.0f;
	float s = std::sin(a);
	float c = std::cos(a);
	(*this)(0,0) =  c;	(*this)(0,2) = s;
	(*this)(2,0) = -s;	(*this)(2,2) = c;
	return *this;
}

/// Setup rotation mat4 around the z-axis, where the rotation angle is given in degrees.
mat4& mat4::rotZ(float rz)
{
	identity();
	float a = rz*M_PI/180.0f;
	float s = std::sin(a);
	float c = std::cos(a);
	(*this)(0,0) = c;	(*this)(0,1) = -s;
	(*this)(1,0) = s;	(*this)(1,1) =  c;
	return *this;
}


/// Compute the determinant of the mat4.
float mat4::determinant() const
{
	const mat4& m = *this;
	return ((m(0,0) * m(1,1) - m(1,0) * m(0,1)) * (m(2,2) * m(3,3) - m(3,2) * m(2,3)) -
			(m(0,0) * m(2,1) - m(2,0) * m(0,1)) * (m(1,2) * m(3,3) - m(3,2) * m(1,3)) +
			(m(0,0) * m(3,1) - m(3,0) * m(0,1)) * (m(1,2) * m(2,3) - m(2,2) * m(1,3)) +
			(m(1,0) * m(2,1) - m(2,0) * m(1,1)) * (m(0,2) * m(3,3) - m(3,2) * m(0,3)) -
			(m(1,0) * m(3,1) - m(3,0) * m(1,1)) * (m(0,2) * m(2,3) - m(2,2) * m(0,3)) +
			(m(2,0) * m(3,1) - m(3,0) * m(2,1)) * (m(0,2) * m(1,3) - m(1,2) * m(0,3)));
}


/// Compute the inverse of the mat4.
mat4 mat4::inverse() const
{
	mat4 result;
	float d = determinant();
	if(d == 0.0f) return result.zero();
	d = 1.0f / d;
	
	const mat4& m = *this;
	result(0,0) = d * (m(1,1) * (m(2,2) * m(3,3) - m(3,2) * m(2,3)) + m(2,1) * (m(3,2) * m(1,3) - m(1,2) * m(3,3)) + m(3,1) * (m(1,2) * m(2,3) - m(2,2) * m(1,3)));
	result(1,0) = d * (m(1,2) * (m(2,0) * m(3,3) - m(3,0) * m(2,3)) + m(2,2) * (m(3,0) * m(1,3) - m(1,0) * m(3,3)) + m(3,2) * (m(1,0) * m(2,3) - m(2,0) * m(1,3)));
	result(2,0) = d * (m(1,3) * (m(2,0) * m(3,1) - m(3,0) * m(2,1)) + m(2,3) * (m(3,0) * m(1,1) - m(1,0) * m(3,1)) + m(3,3) * (m(1,0) * m(2,1) - m(2,0) * m(1,1)));
	result(3,0) = d * (m(1,0) * (m(3,1) * m(2,2) - m(2,1) * m(3,2)) + m(2,0) * (m(1,1) * m(3,2) - m(3,1) * m(1,2)) + m(3,0) * (m(2,1) * m(1,2) - m(1,1) * m(2,2)));
	result(0,1) = d * (m(2,1) * (m(0,2) * m(3,3) - m(3,2) * m(0,3)) + m(3,1) * (m(2,2) * m(0,3) - m(0,2) * m(2,3)) + m(0,1) * (m(3,2) * m(2,3) - m(2,2) * m(3,3)));
	result(1,1) = d * (m(2,2) * (m(0,0) * m(3,3) - m(3,0) * m(0,3)) + m(3,2) * (m(2,0) * m(0,3) - m(0,0) * m(2,3)) + m(0,2) * (m(3,0) * m(2,3) - m(2,0) * m(3,3)));
	result(2,1) = d * (m(2,3) * (m(0,0) * m(3,1) - m(3,0) * m(0,1)) + m(3,3) * (m(2,0) * m(0,1) - m(0,0) * m(2,1)) + m(0,3) * (m(3,0) * m(2,1) - m(2,0) * m(3,1)));
	result(3,1) = d * (m(2,0) * (m(3,1) * m(0,2) - m(0,1) * m(3,2)) + m(3,0) * (m(0,1) * m(2,2) - m(2,1) * m(0,2)) + m(0,0) * (m(2,1) * m(3,2) - m(3,1) * m(2,2)));
	result(0,2) = d * (m(3,1) * (m(0,2) * m(1,3) - m(1,2) * m(0,3)) + m(0,1) * (m(1,2) * m(3,3) - m(3,2) * m(1,3)) + m(1,1) * (m(3,2) * m(0,3) - m(0,2) * m(3,3)));
	result(1,2) = d * (m(3,2) * (m(0,0) * m(1,3) - m(1,0) * m(0,3)) + m(0,2) * (m(1,0) * m(3,3) - m(3,0) * m(1,3)) + m(1,2) * (m(3,0) * m(0,3) - m(0,0) * m(3,3)));
	result(2,2) = d * (m(3,3) * (m(0,0) * m(1,1) - m(1,0) * m(0,1)) + m(0,3) * (m(1,0) * m(3,1) - m(3,0) * m(1,1)) + m(1,3) * (m(3,0) * m(0,1) - m(0,0) * m(3,1)));
	result(3,2) = d * (m(3,0) * (m(1,1) * m(0,2) - m(0,1) * m(1,2)) + m(0,0) * (m(3,1) * m(1,2) - m(1,1) * m(3,2)) + m(1,0) * (m(0,1) * m(3,2) - m(3,1) * m(0,2)));
	result(0,3) = d * (m(0,1) * (m(2,2) * m(1,3) - m(1,2) * m(2,3)) + m(1,1) * (m(0,2) * m(2,3) - m(2,2) * m(0,3)) + m(2,1) * (m(1,2) * m(0,3) - m(0,2) * m(1,3)));
	result(1,3) = d * (m(0,2) * (m(2,0) * m(1,3) - m(1,0) * m(2,3)) + m(1,2) * (m(0,0) * m(2,3) - m(2,0) * m(0,3)) + m(2,2) * (m(1,0) * m(0,3) - m(0,0) * m(1,3)));
	result(2,3) = d * (m(0,3) * (m(2,0) * m(1,1) - m(1,0) * m(2,1)) + m(1,3) * (m(0,0) * m(2,1) - m(2,0) * m(0,1)) + m(2,3) * (m(1,0) * m(0,1) - m(0,0) * m(1,1)));
	result(3,3) = d * (m(0,0) * (m(1,1) * m(2,2) - m(2,1) * m(1,2)) + m(1,0) * (m(2,1) * m(0,2) - m(0,1) * m(2,2)) + m(2,0) * (m(0,1) * m(1,2) - m(1,1) * m(0,2)));
	return result;
}


mat4 mat4::transpose() const
{
	mat4 result;
	const mat4& m = *this;

	result(0,0) = m(0,0);
	result(1,0) = m(0,1);
	result(2,0) = m(0,2);
	result(3,0) = m(0,3);
	result(0,1) = m(1,0);
	result(1,1) = m(1,1);
	result(2,1) = m(1,2);
	result(3,1) = m(1,3);
	result(0,2) = m(2,0);
	result(1,2) = m(2,1);
	result(2,2) = m(2,2);
	result(3,2) = m(2,3);
	result(0,3) = m(3,0);
	result(1,3) = m(3,1);
	result(2,3) = m(3,2);
	result(3,3) = m(3,3);
	return result;
}

// ------------------------ vec3 functions ------------------------ //

/// Write vector elements to an output stream.
std::ostream& operator<<(std::ostream& os, const vec3& A)
{
	std::streamsize p = os.precision();
	std::streamsize w = os.width();
	for(int i=0; i<3; i++) os << std::setprecision(5) << std::setw(10) << A(i);
	os << std::endl;
//	os << A.x << A.y << A.z << std::endl;
	os.precision(p);
	os.width(w);
	return os;
}


// ------------------------ vec4 functions ------------------------ //

/// Write point elements to an output stream.
std::ostream& operator<<(std::ostream& os, const vec4& A)
{
	std::streamsize p = os.precision();
	std::streamsize w = os.width();
	for(int i=0; i<4; i++) os << std::setprecision(5) << std::setw(10) << A(i);
	os << std::endl;
//	os << A.x << A.y << A.z << std::endl;
	os.precision(p);
	os.width(w);
	return os;
}
