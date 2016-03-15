#ifndef __INCLUDE_GUARD_D2CAE848_1C1E_4362_8DFE_163B2EA2A97D
#define __INCLUDE_GUARD_D2CAE848_1C1E_4362_8DFE_163B2EA2A97D
#ifdef _MSC_VER
	#pragma once
#endif

#include "../rt/basic_definitions.h"

struct PointLightSource
{
	Point position;
	float4 intensity, falloff;
	//falloff formula: (.x  / dist^2 + .y / dist + .z) * intensity;
};


struct DepthStateKey
{
	typedef int t_data;
};

class IntegratorImpl : public Integrator
{
public:
	enum {_MAX_BOUNCES = 7};

	GeometryGroup *scene;
	std::vector<PointLightSource> lightSources;
	float4 ambientLight;
    float curRefractionIndex;

	IntegratorImpl()
	{
		state.value<DepthStateKey>() = 0;
	}

    virtual float getCurrentRefractionIndex()
    {
        return curRefractionIndex;
    }


	virtual float4 getRadiance(const Ray &_ray)
	{
		state.value<DepthStateKey>()++;

		float4 col = float4::rep(0); // storing the color
		float4 shadow = float4::rep(1.0f); // determines the light dimming

		if(state.value<DepthStateKey>() < _MAX_BOUNCES)
		{
		    //std::cout << "integrator cur: " << _ray.curRefractionIndex << std::endl;
		    curRefractionIndex = _ray.curRefractionIndex;

			Primitive::IntRet ret = scene->intersect(_ray, FLT_MAX);
			if(ret.distance < FLT_MAX && ret.distance >= Primitive::INTEPS())
			{
				SmartPtr<Shader> shader = scene->getShader(ret);
				if(shader.data() != NULL)
				{
					col += shader->getAmbientCoefficient() * ambientLight;

					Point intPt = _ray.o + ret.distance * _ray.d;

					for(std::vector<PointLightSource>::const_iterator it = lightSources.begin(); it != lightSources.end(); it++)
					{
						shadow = visibleLS(intPt, it->position);
						if(shadow[0] > 0 && shadow[1] > 0 && shadow[2] > 0 && shadow[3] > 0)
						//if (true)
						{
							Vector lightD = it->position - intPt;
							float4 refl = shader->getReflectance(-_ray.d, lightD);
							float dist = lightD.len();
							float fallOff = it->falloff.x / (dist * dist) + it->falloff.y / dist + it->falloff.z;
							col += shadow * refl * float4::rep(fallOff) * it->intensity;
						}
					}
                    col += shader->getIndirectRadiance(-_ray.d, this);
				}
			}
		}

		state.value<DepthStateKey>()--;

		return col;
	}

	virtual float4 getShadow(ShadowRay &_sr)
	{
        Primitive::IntRet ret = scene->intersect(_sr, FLT_MAX);
        // check if something gets hit in between lightsource and origin of ray
		if(ret.distance < FLT_MAX && ret.distance < (_sr.lightSource-_sr.o).len() && ret.distance >= Primitive::INTEPS())
        {
			SmartPtr<Shader> shader = scene->getShader(ret);
			if(shader.data() != NULL)
			{
			    _sr.hitCounts++;
			    // send new shadow ray from hitpoint in same direction with slight offset to prevent floating point errors
			    Point intPt = _sr.o + ((ret.distance + Primitive::INTEPS()) * _sr.d);
			    _sr.o = intPt;
			    //_sr.d = ~_sr.d;
                //std::cout << "hi " << _sr.hitCounts << " hi2 " << _sr.o[0] << std::endl;
			    return shader->getTransparency(_sr, this);
			}
		}

        //std::cout << "direct light" << std::endl;
        return float4::rep(1.0f);
	}
private:

	float4 visibleLS(const Point& _pt, const Point& _pls)
	{
        ShadowRay r;
        r.lightSource = _pls;
        r.d = ~(_pls - _pt);
        r.o = _pt + Primitive::INTEPS() * r.d;
        //r.d = ~r.d;

        //return float4::rep(1.0f);
        return getShadow(r);

        /*
        Primitive::IntRet ret = scene->intersect(r, 1.1f);
        if (ret.distance >= 1 - Primitive::INTEPS())
        {
             return float4::rep(1.0f);
        } else {

            return getShadow(r);
        }
        */
        //return ret.distance >= 1 - Primitive::INTEPS();


        //return getShadow(r);



	}

};


#endif //__INCLUDE_GUARD_D2CAE848_1C1E_4362_8DFE_163B2EA2A97D
