#ifndef __RAY_H_INCLUDED_CA4668D0_F683_4D3D_B7EB_63AC9E7F7633
#define __RAY_H_INCLUDED_CA4668D0_F683_4D3D_B7EB_63AC9E7F7633
#ifdef _MSC_VER
	#pragma once
#endif

#include "algebra.h"

//A ray class
struct Ray
{
	Point o; //origin
	Vector d; //direction
	//float lastRefractionIndex; // refraction index of last material
	float curRefractionIndex; // current refraction index (defaul t= air at sea level)
	//boolean inObject;

	Ray()
	{
        //astRefractionIndex = 1.00029f;
        curRefractionIndex = 1.00029f;
        //inObject = false;
	}
	Ray(const Point &_o, const Vector &_d)
		: o(_o), d(_d)
    {
        //lastRefractionIndex = 1.00029f;
		curRefractionIndex = 1.00029f;
        //inObject = false;
	}

	Point getPoint(float _distance)
	{
		return o + _distance * d;
	}
};

class ShadowRay : public Ray
{
public:
    int hitCounts;
    Point lightSource;

    ShadowRay()
    {
        hitCounts = 0;
    }
};




#endif //__RAY_H_INCLUDED_CA4668D0_F683_4D3D_B7EB_63AC9E7F7633
