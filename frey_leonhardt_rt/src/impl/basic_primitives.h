#ifndef __PRIMITIVES_H_INCLUDED_29F93A38_6FEC_4D65_82B0_DC9B7CB5848A
#define __PRIMITIVES_H_INCLUDED_29F93A38_6FEC_4D65_82B0_DC9B7CB5848A
#ifdef _MSC_VER
	#pragma once
#endif

#include "core/algebra.h"
#include "core/util.h"
#include "rt/basic_definitions.h"
#include "../rt/shading_basics.h"

#define SMALL_NUM  0.00000001

struct BasicPrimitiveHitPoint : public RefCntBase
{
	Point hit;
};

//An infinite plane
struct InfinitePlane : public Primitive
{
	float4 equation;
	SmartPtr<PluggableShader> shader;

	InfinitePlane() {}
	InfinitePlane(Point _origin, Vector _normal, SmartPtr<PluggableShader> _shader)
		: shader(_shader)
	{
		equation = _normal;
		equation.w = -equation.dot(_origin);
	}

	virtual IntRet intersect(const Ray& _ray, float _previousBestDistance ) const
	{
		IntRet ret;

		float div = float4(_ray.d).dot(equation);
		if(fabs(div) > 0.00001)
		{
			float dist = -float4(_ray.o).dot(equation) / div;

			SmartPtr<BasicPrimitiveHitPoint> hit(new BasicPrimitiveHitPoint);
			hit->hit = _ray.o + _ray.d * dist;
			ret.hitInfo = hit;
			ret.distance = dist;
		}

		return ret;
	}

	virtual SmartPtr<Shader> getShader(IntRet _intData) const
	{
		SmartPtr<PluggableShader> ret = shader->clone();

		SmartPtr<BasicPrimitiveHitPoint> hit = _intData.hitInfo;

		ret->setPosition(hit->hit);
		ret->setNormal(*(Vector*)&equation);

		return ret;
	}

	virtual BBox getBBox() const
	{
		return BBox::empty();
	}
};

//An infinite line
struct InfiniteLine : public Primitive
{
    Point p1, p2;
    Vector p2p1;
	SmartPtr<PluggableShader> shader;

	InfiniteLine() {}
	InfiniteLine(Point _origin, Point _p2, SmartPtr<PluggableShader> _shader)
		: shader(_shader), p1(_origin), p2(p2)
	{
	    p2p1 = ~(p2-p1);
	}

		InfiniteLine(Ray _ray, SmartPtr<PluggableShader> _shader)
		: shader(_shader), p1(_ray.o), p2(_ray.o+_ray.d)
	{
	    p2p1 = ~(p2-p1);;
	}

    // Using distance between lines algorithm found here
    // http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
	virtual IntRet intersect(const Ray& _ray, float _previousBestDistance) const
	{
		IntRet ret;

        Point rp1, rp2;

        rp1 = _ray.o;
        // we are assuming that the ray is an infinite line, too,
		// therefore get any second point on the ray
		rp2 =  _ray.o + _ray.d;

        Vector   u = rp2 - rp1;
        Vector   v = p2p1;
        Vector   w = rp1 - p1;
        float    a = u*u;        // always >= 0
        float    b = u*v;
        float    c = v*v;        // always >= 0
        float    d = u*w;
        float    e = v*w;
        float    D = a*c - b*b;       // always >= 0

        float    sc, tc;

        // compute the line parameters of the two closest points
        if (D < SMALL_NUM) {         // the lines are almost parallel
            sc = 0.0;
            tc = (b>c ? d/b : e/c);   // use the largest denominator
        }
        else {
            sc = (b*e - c*d) / D;
            tc = (a*e - b*d) / D;
        }

        // get the difference of the two closest points
        Vector   dP = w + (sc * u) - (tc * v);  // = L1(sc) - L2(tc)

        float dist = dP.len();   // closest distance

        // if distance is under threshold, count as hit
        if (fabs(dist) < 0.005f ) //Primitive::INTEPS())
        {
            // check if hitpoint is in front of ray and is actually visible
            // (diabled for reflections)
            //if (sc<0)
            //  return ret;
            SmartPtr<BasicPrimitiveHitPoint> hit(new BasicPrimitiveHitPoint);
			hit->hit = _ray.o + sc * u;
			ret.hitInfo = hit;
			ret.distance = (sc * u).len();
        }

        return ret;

        /*
		float div = float4(_ray.d).dot(equation);
		if(fabs(div) > 0.00001)
		{
			float dist = -float4(_ray.o).dot(equation) / div;

			SmartPtr<BasicPrimitiveHitPoint> hit(new BasicPrimitiveHitPoint);
			hit->hit = _ray.o + _ray.d * dist;
			ret.hitInfo = hit;
			ret.distance = dist;
		}

		return ret;
		*/
	}

	virtual SmartPtr<Shader> getShader(IntRet _intData) const
	{
		SmartPtr<PluggableShader> ret = shader->clone();

		SmartPtr<BasicPrimitiveHitPoint> hit = _intData.hitInfo;

		ret->setPosition(hit->hit);

		return ret;
	}

	virtual BBox getBBox() const
	{
		return BBox::empty();
	}
};

//A sphere
struct Sphere : public Primitive
{
	Point center;
	float radius;
	SmartPtr<PluggableShader> shader;

	Sphere() {}
	Sphere(Point _center, float _radius, SmartPtr<PluggableShader> _shader)
		: shader(_shader), center(_center), radius(_radius) {}

	virtual IntRet intersect(const Ray& _ray, float _previousBestDistance ) const
	{
		IntRet ret;

		float A = _ray.d * _ray.d;
		float B = 2 * (_ray.o - center) * _ray.d;
		float C = (_ray.o - center) * (_ray.o - center) - radius * radius;

		float det = B * B - 4 * A * C;

		if(det >= 0)
		{
			float sol1 = (-B + sqrt(det)) / (2 * A);
			float sol2 = (-B - sqrt(det)) / (2 * A);

			if(sol1 > sol2)
				std::swap(sol1, sol2);

			float dist = sol1 > INTEPS() ? sol1 : sol2;

			SmartPtr<BasicPrimitiveHitPoint> hit = new BasicPrimitiveHitPoint;
			hit->hit = _ray.o + _ray.d * dist;
			ret.hitInfo = hit;
			ret.distance = dist;
		}

		return ret;
	}

	virtual SmartPtr<Shader> getShader(IntRet _intData) const
	{

		SmartPtr<PluggableShader> ret = shader->clone();
		SmartPtr<BasicPrimitiveHitPoint> hit = _intData.hitInfo;

		ret->setPosition(hit->hit);
		ret->setNormal(hit->hit - center);

		return ret;
	}

	virtual BBox getBBox() const
	{
		return BBox::empty();
	}
};

//A triangle
struct Triangle : public Primitive
{
	Point p1, p2, p3;
	SmartPtr<PluggableShader> shader;

	Triangle(){}
	Triangle(Point _p1, Point _p2, Point _p3, PluggableShader *_shader)
		: p1(_p1), p2(_p2), p3(_p3), shader(_shader)
	{}


	virtual IntRet intersect(const Ray& _ray, float _previousBestDistance ) const
	{
		IntRet ret;

		float4 intRes = intersectTriangle(p1, p2, p3, _ray);
		ret.distance = intRes.w;

		if(intRes.w != FLT_MAX)
		{

			SmartPtr<BasicPrimitiveHitPoint> hit = new BasicPrimitiveHitPoint;
			hit->hit = _ray.o + _ray.d * intRes.w;

			ret.hitInfo = hit;
		}

		return ret;
	}

	virtual SmartPtr<Shader> getShader(IntRet _intData) const
	{
		SmartPtr<PluggableShader> ret = shader->clone();
		SmartPtr<BasicPrimitiveHitPoint> hit = _intData.hitInfo;

		ret->setPosition(hit->hit);

		Vector e1 = ~(p2 - p1);
		Vector e2 = ~(p3 - p1);
		Vector norm = ~(e1 % e2);
		ret->setNormal(norm);

		return ret; // wasnt here before
	}

	virtual BBox getBBox() const
	{
		return BBox::empty();
	}
};

#endif //__PRIMITIVES_H_INCLUDED_29F93A38_6FEC_4D65_82B0_DC9B7CB5848A
