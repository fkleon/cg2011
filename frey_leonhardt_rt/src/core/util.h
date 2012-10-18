#ifndef __UTIL_H_INCLUDED_6DEB3409_AA7C_48E0_AEDC_5A40687E23E6
#define __UTIL_H_INCLUDED_6DEB3409_AA7C_48E0_AEDC_5A40687E23E6
#ifdef _MSC_VER
	#pragma once
#endif

#include "../core/algebra.h"

//This routine intersects a ray with a triangle
//Returns:
//x, y, z <-> barycentric coordinates of intersection
//w <-> distance to intersection
inline float4 intersectTriangle(
	const Point &_p1, const Point &_p2, const Point &_p3,
	const Ray &_ray)
{
	float4 ret = float4::rep(FLT_MAX);

	Vector e1 = _p1 - _p3;
	Vector e2 = _p2 - _p3;

	Vector pvec = _ray.d % e2;
	float det =  e1 * pvec;

	if(fabs(det) > 0.00001)
	{
		Vector tvec = _ray.o - _p3;
		Vector qvec = tvec % e1;

		float u = tvec * pvec / det;
		float v = _ray.d * qvec / det;

		ret.w = 
			(u < -0.00001 || v < -0.00001 || u + v > 1.00002) ? FLT_MAX : e2 * qvec / det;

		ret = float4(u, v, 1 - u - v, ret.w);
	}

	return ret;
}

#endif //__UTIL_H_INCLUDED_6DEB3409_AA7C_48E0_AEDC_5A40687E23E6
