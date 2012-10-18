#ifndef PHOTONINTEGRATOR_H_
#define PHOTONINTEGRATOR_H_

#include "../rt/basic_definitions.h"
#include "random.h"
#include <cmath>
#include "../stdafx.h"
#include "../rt/geometry_group.h"
#include "../impl/phong_shaders.h"
#include "integrator.h"
#include "../rt/myphotonmap.h"

#define PI 3.1415926f
#define PHOTON_AMP 1200
#define PHOTON_NUM 25000000
#define PHOTON_SAMPLES 200
#define CAUSTIC_SAMPLES 100
#define PHOTON_DISTANCE 4.f
#define CAUSTIC_DISTANCE 10.f

/*
struct DepthStateKey
{
	typedef int t_data;
};
*/

class PhotonMap_Integrator : public Integrator
{
public:
	enum {_MAX_BOUNCES = 7};

	GeometryGroup *scene;
	std::vector<PointLightSource> lightSources;
	float4 ambientLight;
    float curRefractionIndex;
	int totalNumberOfPhotons;

	PhotonMap_Integrator()
	{
		totalNumberOfPhotons = PHOTON_NUM;
		photonMap = createPhotonMap(totalNumberOfPhotons);
		causticMap = createPhotonMap(totalNumberOfPhotons);
		state.value<DepthStateKey>() = 0;
	}

	virtual float getCurrentRefractionIndex()
    {
        return curRefractionIndex;
    }

	virtual void start_photonmapping()
	{
	    Random::init((unsigned)42);
        const clock_t begin_time = clock(); // for building time measurement for debug output

        for(int i=0;i<256;i++)
        {
            double angle = double(i)*(1.0/256.0)*M_PI;
            cosTheta[i] = cos(angle);
            sinTheta[i] = sin(angle);
            cosPhi[i] = cos(2.0*angle);
            sinPhi[i] = sin(2.0*angle);
        }

        //std::cout << "Arrr,fire photons" << std::endl;
        shootPhotons();

        std::cout << "Arrr, " << totalNumberOfPhotons << " photons be fired!" << std::endl;
        std::cout << "The lot of 'em photons " << balancedPhotonMap->stored_photons << " and in the brigg as well " << balancedCausticMap->stored_photons << " caustic landlubbers!" << std::endl;
        std::cout << "Arrdventure took " << float(clock()-begin_time)/CLOCKS_PER_SEC << " shots 'o rum!" << std::endl;
        //abort();
    }

	//get radiance is the same as in integrator plus considering irradiance
	virtual float4 getRadiance(const Ray &_ray)
	{
		state.value<DepthStateKey>()++;

		float4 col = float4::rep(0); // storing the color
		//float4 shadow = float4::rep(1.0f); // determines the light dimming

		if(state.value<DepthStateKey>() < _MAX_BOUNCES)
		{
		    //std::cout << "integrator cur: " << _ray.curRefractionIndex << std::endl;
		    curRefractionIndex = _ray.curRefractionIndex;

			Primitive::IntRet ret = scene->intersect(_ray, FLT_MAX);
			if(ret.distance < FLT_MAX && ret.distance >= Primitive::INTEPS())
			{
				SmartPtr<DefaultPhongShader> shader = scene->getShader(ret);
				if(shader.data() != NULL)
				{
					col += shader->getAmbientCoefficient() * ambientLight;

					Point hp = _ray.o + ret.distance * _ray.d;

					for(std::vector<PointLightSource>::const_iterator it = lightSources.begin(); it != lightSources.end(); it++)
					{
						/*shadow = visibleLS(intPt, it->position);
						if(shadow[0] > 0 && shadow[1] > 0 && shadow[2] > 0 && shadow[3] > 0)
						//if (true)
						{
							Vector lightD = it->position - intPt;
							float4 refl = shader->getReflectance(-_ray.d, lightD);
							float dist = lightD.len();
							float fallOff = it->falloff.x / (dist * dist) + it->falloff.y / dist + it->falloff.z;
							col += shadow * refl * float4::rep(fallOff) * it->intensity;
						}
						*/
						if(visibleLS(hp,it->position))
						{
							Vector lightD = it->position - hp;
							float4 refl = shader->getReflectance(-_ray.d, lightD);
							float dist = lightD.len();
							float fallOff = it->falloff.x / (dist * dist) + it->falloff.y / dist + it->falloff.z;
							col +=  refl * float4::rep(fallOff) * it->intensity;
						}

					}
                    col += shader->getIndirectRadiance(-_ray.d, this);

                    // photon irradiance
					float *irradiance = new float[3];
					float pos[3] = {hp.x,hp.y,hp.z};
					float norm[3] = {shader->getNormal().x, shader->getNormal().y,shader->getNormal().z};
					//std::cout << "irradiance1 :" << irradiance[0] << std::endl;
					irradianceEstimate(balancedPhotonMap,irradiance,pos,norm, PHOTON_DISTANCE, PHOTON_SAMPLES);

					float4 newIrrad = float4(irradiance[0],irradiance[1],irradiance[2],0.f);
					//std::cout << "irradiance2 :" << irradiance[0] << std::endl;
					col+=newIrrad;

					// caustic irradiance
                    float *causticIrradiance = new float[3];
					irradianceEstimate(balancedCausticMap,causticIrradiance,pos,norm, CAUSTIC_DISTANCE, CAUSTIC_SAMPLES);

					float4 newCausticIrrad = float4(causticIrradiance[0],causticIrradiance[1],causticIrradiance[2],0.f);
					col+=newCausticIrrad;
				}
			}
		}

		state.value<DepthStateKey>()--;

		return col;
	}

	virtual float4 getShadow(ShadowRay &_sr) {
		return float4::rep(0.f);
	}
private:
	//get a random direction via lookup table
    Vector getMyRandDirection()
	{
		float rand1 = Random::getRandomFloat(0,255);
		float rand2 = Random::getRandomFloat(0,255);

		int test1 = rand1;
		int test2 = rand2;
		Vector myVec;

		myVec.x = sinTheta[test1]*cosPhi[test2];
		myVec.y = sinTheta[test1]*sinPhi[test2];
		myVec.z = cosTheta[test1];

		return myVec;
	}


	float cosTheta[256];
	float sinTheta[256];
	float cosPhi[256];
	float sinPhi[256];

	PhotonMap *photonMap, *causticMap;
	BalancedPhotonMap *balancedPhotonMap, *balancedCausticMap;


	//this function shoots the photons into the scene
	//the photons are equali distributet, but random
	void shootPhotons()
	{
		int photonsLeftToShoot = totalNumberOfPhotons;
		int photonsPerLightsource = totalNumberOfPhotons / lightSources.size();

		for(int j =0; j< lightSources.size();j++)
		{
			photonsLeftToShoot -= photonsPerLightsource;
			int numberOfPhotons = photonsLeftToShoot < photonsPerLightsource ? photonsLeftToShoot + photonsPerLightsource : photonsPerLightsource;
			float4 powerOfSinglePhoton = float4::rep(PHOTON_AMP)* lightSources[j].intensity/float4::rep(numberOfPhotons);

			Point pos = lightSources[j].position;
			for (int n=0 ; n< numberOfPhotons; n++)
			{
				Ray ray;
				ray.o = pos;
				ray.d = getMyRandDirection();
				//std::cout << "arre we be shooting at the ship over at: (" << ray.d.x << "," << ray.d.y << ";" <<  ray.d.z << ")" << std::endl;
				//std::cout<< "Arrrr fire cannons" << std::endl;
				traceAPhoton(ray,1,powerOfSinglePhoton,0);
			}
		}

		balancedPhotonMap = balancePhotonMap(photonMap);
		balancedCausticMap = balancePhotonMap(causticMap);
	}

	void traceAPhoton(Ray ray, float weight, float4 power, int bounces)
	{
	    traceAPhoton(ray,weight,power,bounces,false);
	}

	void traceAPhoton(Ray ray, float weight, float4 power, int bounces, bool caustic)
	{
		state.value<DepthStateKey>()++;


		if(weight >0.2 && state.value<DepthStateKey>() < 5)
		{
			Primitive::IntRet ret = scene->intersect(ray, FLT_MAX);
			if(ret.distance < FLT_MAX && ret.distance >= Primitive::INTEPS())
			{
				Point hp = ray.o + ret.distance * ray.d;

				SmartPtr<Shader> shader = scene->getShader(ret);
				if(shader.data() != NULL)
				{
					float4 diffuseCoeff = float4::rep(0.0);
					float4 specularCoeff = float4::rep(0.0);
					float refractionProbability = 0;
					float reflectionProbability =0;
					float diffuseReflectionProbability=0;
					float specularExp =0;
					//std::cout << "saved photon at: (" << hp.x << "," << hp.y << "," << hp.z << ")" << std::endl;


					//std::cout << "power" << power[0] << std::endl;
					float pow[3] = {power[0],power[1],power[2]};
					float pos[3] = {hp.x,hp.y,hp.z};
					float dir[3] = {ray.d.x, ray.d.y, ray.d.z};

					float roulettNumber = Random::getRandomFloat(0,1);

					//std::cout << "wohooooo" << std::endl;
					//if(bounces > 0) std::cout << "rekursion jihaaa" << std::endl;

                    // refractive shader
					if(shader->isTransparent())
					{
						SmartPtr<RefractivePhongShader> refrShader = shader;
						refrShader->getCoeff(diffuseCoeff,specularCoeff,specularExp);

						Vector out = ~(-ray.d);
						Ray refl, refr;
						refrShader->getPhotonInformation(out, reflectionProbability, refractionProbability, refl, refr);

						//refractionProbability = 0.0;//shader->getRefractionProbability();
						//reflectionProbability = 0.0;//shader->getReflectionProbability();

						if(roulettNumber < reflectionProbability)
						{
							power *= (specularCoeff/float4::rep(reflectionProbability));
							traceAPhoton(refl,weight*reflectionProbability*0.9f,power,bounces+1);
						} else if(roulettNumber < refractionProbability)
						{
							power *=  (diffuseCoeff/ float4::rep(refractionProbability));
							traceAPhoton(refr,weight * refractionProbability *0.9f, power,bounces+1,true);
						}else
						{
						    state.value<DepthStateKey>()--;
							return;
						}

					}

                    // reflective shader
					if(shader->isReflective())
					{
						SmartPtr<MirrorPhongShader> reflShader = shader;
						reflShader->getCoeff(diffuseCoeff,specularCoeff,specularExp);

                        Vector out = ~(-ray.d);
						Ray refl;
                        reflShader->getPhotonInformation(out, reflectionProbability, refl);

						if(roulettNumber < reflectionProbability)
						{
							power *= (specularCoeff/float4::rep(reflectionProbability));
							traceAPhoton(refl,weight*reflectionProbability*0.9f,power,bounces+1);
						}else
						{
						    state.value<DepthStateKey>()--;
							return;
						}
					}


					SmartPtr<DefaultPhongShader> phongShader = shader;
					phongShader->getCoeff(diffuseCoeff,specularCoeff,specularExp);

					//std::cout<< "roulette Number: " << roulettNumber << std::endl;
					if(roulettNumber < diffuseCoeff.x)
					{
						Vector randomDirection = getMyRandDirection();


						if (randomDirection * phongShader->getNormal() < 0)
						{
							randomDirection = -randomDirection;
						}
						Ray r = Ray();
						r.o = hp;
						r.d = randomDirection;

						power *= diffuseCoeff;

						if(bounces >0) {
						    if (caustic)
                                storePhoton(causticMap, pow, pos, dir);
                            else
                                storePhoton(photonMap, pow, pos, dir);
						}

						traceAPhoton(r,weight*diffuseCoeff.x * 0.9,power,bounces +1);
					}




				}else
				{
					//std::cout << "shader data was null" << std::endl;
				}

			}
			//std::cout << "darrrrnn cannons missed" << std::endl;
		}else{
				//std::cout << "ohhhh tooo weak they be: recursion:"<< state.value<DepthStateKey>() << std::endl;
			}

		state.value<DepthStateKey>()--;
	}




	Vector getRandDirectionOverHemisphere()
	{
		float rand1 = Random::getRandomFloat(0,1);
		float rand2 = Random::getRandomFloat(0,1);
		const float radius = sqrt(1.0f - rand1 * rand1);
		const float phi = 2 * PI * rand2;
		return Vector(cos(phi) * radius, sin(phi) * radius, rand2);
	}

	float getMaxOutOfThree(float x, float y, float z)
	{
		float ret = 0.0f;
		x>y ? x>z ? ret = x : ret = z : y>z ? ret = y : ret = z;
		return ret;
	}

	float getMinOutOfThree(float x, float y, float z)
	{
		float ret = 0.0f;
		x<y ? x<z ? ret = x : ret = z : y<z ? ret = y : ret = z;
		return ret;
	}


	bool visibleLS(const Point& _pt, const Point& _pls)
	{
		Ray r; r.o = _pt; r.d = _pls - _pt;
		Primitive::IntRet ret = scene->intersect(r, 1.1f);
		return ret.distance <= Primitive::INTEPS() || ret.distance >= 1 - Primitive::INTEPS();
	}


};
#endif /* PHOTONINTEGRATOR_H_ */
