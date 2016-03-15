#ifndef __INCLUDE_GUARD_C2FDE8FF_953C_4D01_9CA6_7F03367AD67B
#define __INCLUDE_GUARD_C2FDE8FF_953C_4D01_9CA6_7F03367AD67B
#ifdef _MSC_VER
	#pragma once
#endif

#include "../core/defs.h"
#include "../core/bbox.h"
#include "../core/memory.h"
#include "../core/state.h"


//This is the basic class for a camera, used to get a primary ray for a pixel
class Camera : public RefCntBase
{
public:
	//Returns the primary ray for pixel _x, _y.
	virtual Ray getPrimaryRay(float _x, float _y) = 0;

	//Returns primary rays of a camera,
	// can be one with a perspective camera
	// or more, for example with a lens camera
    virtual std::vector<Ray> getPrimaryRays(float _x, float _y) = 0;
};

//This is the base class for an integrator. The integrator
//	solves the task of determining how much radiance
//	is traveling along the ray towards _ray.o (oposite to _ray.d)
struct Integrator : public RefCntBase
{
	StateObject state;
	virtual float getCurrentRefractionIndex() = 0;
	virtual float4 getRadiance(const Ray &_ray) = 0;
    virtual float4 getShadow(ShadowRay &_sr) = 0;
};

struct Shader;

//A class for a primitive
class Primitive
{
public:

	//The structure returned from an intersection
	struct IntRet
	{
		//Information to pass from the intersection routine
		//	to the getShader routine. Although you can pass
		//	a pointer to a static structure, this is a bad practice
		//	since it wont be safe in a recursive integrator. Also, heap allocations
		//	and deallocations of objects derived from RefCntBase are very fast
		//	and efficient.
		SmartPtr<RefCntBase> hitInfo;

		//The distance to the intersection
		float distance;

		IntRet() : distance(FLT_MAX){}
	};

	//This function creates a shader for the intersection point
	//	specified by _intData. The shader is "interrogated" by the
	//	integrator to determine the radiance traveling backwards
	//	along the ray. It is than dereferences and automatically
	//	deleted, if its reference count drops to 0.
	//You should create a new shader for each hit point.
	virtual SmartPtr<Shader> getShader(IntRet _intData) const = 0;

	//This function intersects a ray with a primitive. It returns the distance
	//	to the intersection as well as (optionally) a data structure to be passed
	//	to the getShader function. This data structure is than used to create
	//	a shader for the intersection. It can contain for example the intersection
	//	point, the barycentric coordinates for a triangle, etc. Look in impl/basic_primitives.h
	//	and impl/lwobject_primitive.cpp for usage examples.
	virtual IntRet intersect(const Ray& _ray, float _previousBestDistance) const = 0;

	//Returns the bounding box around the primitive, and BBox::empty() if the
	//	primitive is unbounded
	virtual BBox getBBox() const = 0;

	//Intersections are considered "successful", if the distance to the intersection is
	//	bigger than INTEPS() and smaller than FLT_MAX
	//static const float INTEPS() { return 0.0001f;};
	static const float INTEPS() { return 0.0001f;};
};

#endif //__INCLUDE_GUARD_C2FDE8FF_953C_4D01_9CA6_7F03367AD67B
