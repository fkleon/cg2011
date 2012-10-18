#include "stdafx.h"

#include "core/image.h"
#include "rt/basic_definitions.h"
#include "rt/geometry_group.h"
#include "impl/basic_primitives.h"
#include "impl/perspective_camera.h"
#include "impl/samplers.h"
#include "impl/lwobject.h"
#include "impl/phong_shaders.h"
#include "impl/integrator.h"


struct EyeLightIntegrator : public Integrator
{
public:
	Primitive *scene;

	virtual float getCurrentRefractionIndex()
	{
        return 0;
	}

	virtual float4 getRadiance(const Ray &_ray)
	{
		float4 col = float4::rep(0);

		Primitive::IntRet ret = scene->intersect(_ray, FLT_MAX);
		if(ret.distance < FLT_MAX && ret.distance >= Primitive::INTEPS())
		{
			SmartPtr<Shader> shader = scene->getShader(ret);
			if(shader.data() != NULL)
				col += shader->getReflectance(-_ray.d, -_ray.d);
		}

		return col;
	}
};

//A check board shader in 3D, dependent only of the position in space
class CheckBoard3DShader : public PluggableShader
{
	Point m_position;
public:
	//Only the x, y, and z are taken into account
	float4 scale;
	virtual void setPosition(const Point& _point) {m_position = _point;}
	virtual float4 getReflectance(const Vector &_outDir, const Vector &_inDir) const
	{
		float4 t = float4(m_position) * scale;

		int x = (int)floor(t.x), y = (int)floor(t.y), z = (int)floor(t.z);
		x %= 2; y %= 2; z %= 2;

		float col = (float)((6 + x + 0 + z) % 2);
		return float4::rep(col);
	}

	_IMPLEMENT_CLONE(CheckBoard3DShader);

};

void assigment4_ex3()
{
	//Image img(400, 300);
	Image img(800, 600);
	//Image img(1024,768);
	//Image img(2048,2048);
	img.addRef();

	CheckBoard3DShader sh;
	sh.scale = float4::rep(4);
	sh.addRef();

	InfinitePlane p1(Point(0, 0.1f, 0), Vector(0, 1, 0), &sh);

    DefaultPhongShader defSh;
    defSh.diffuseCoef = float4(0, 0.2f, 0, 0);
    defSh.ambientCoef = defSh.diffuseCoef;
    defSh.specularCoef = float4::rep(0.8f);
    defSh.specularExponent = 5000.f;
    defSh.addRef();

    DefaultPhongShader defSh2;
    defSh2.diffuseCoef = float4(0.2f, 0, 0, 0);
    defSh2.ambientCoef = defSh.diffuseCoef;
    defSh2.specularCoef = float4::rep(0.6f);
    defSh2.specularExponent = 50000.f;
    defSh2.addRef();

	RefractivePhongShader shader;
    shader.refractionIndex = 1.6f;
    shader.diffuseCoef = float4(0.3f, 0.3f, 0.3f, 0);
	shader.ambientCoef = shader.diffuseCoef;
	shader.specularCoef = float4::rep(0.2f);
	shader.specularExponent = 1000.f;
	shader.transparency = float4::rep(0.65f);
	shader.addRef();

/*
	ProceduralBumpShader procBump;
    procBump.diffuseCoef = float4(110.f/255,54.f/255,9.f/255,0.f);
    //procBump.diffuseCoef = float4(0.0f, 0.0f, 0.0f, 1.);
    //procBump.diffuseCoef = float4::rep(0);
	procBump.ambientCoef = procBump.diffuseCoef;
	procBump.specularCoef = float4::rep(0.2f);
	procBump.specularExponent = 10000000.f;
	procBump.refractionIndex = 1.3f;
	procBump.transparency = float4::rep(0.65f);
	procBump.bumpIntensity = 0.1f;
	procBump.frequency = 3.0f;
	procBump.addRef();
*/

    float4 kackBraun = float4(181.f/255,58.f/255,4.f/255,0);
    float4 duennschissBraun = float4(119.f/255,15.f/255,0.f/255,0);

    SmartPtr<ProceduralTexture> woodTex = new ProceduralWoodTexture(duennschissBraun, kackBraun, 20.f, 1.0f);
    SmartPtr<ProceduralTexture> marbleTex = new ProceduralMarbleTexture(duennschissBraun, kackBraun, 20.f, 0.5f);
    //tex->frequency = 3.f;b
    //tex->setColor(wood.lightColor, wood.darkColor);
    ProceduralBumpShader procBumpShader(marbleTex);
    procBumpShader.diffuseCoef = float4(110.f/255,54.f/255,9.f/255,0.f);
    procBumpShader.ambientCoef = procBumpShader.diffuseCoef;
    procBumpShader.specularCoef = float4::rep(0.3f);
    procBumpShader.specularExponent = 10000.f;

    ProceduralHardBumpShader procBumpHardShader(marbleTex);
    procBumpHardShader.diffuseCoef = float4(110.f/255,54.f/255,9.f/255,0.f);
    procBumpHardShader.ambientCoef = procBumpHardShader.diffuseCoef;
    procBumpHardShader.specularCoef = float4::rep(0.3f);
    procBumpHardShader.specularExponent = 10000.f;
    // sphere test purpose
    Sphere s1(Point(-2.f,  1.7f,  4), 1   , &shader);
    Sphere s2(Point(1.1,    2.5f,     6), 2.0f, &procBumpShader);
    Sphere s3(Point(-1.,   1.1f, 3), 1,    &defSh2);

    DefaultAmbientShader amb;
    amb.ambientCoefficient = float4(255.f,255.f,0.f,0.f);

    InfiniteLine l1(Point(0, 0.2f, 10.f),Point(-1.f, 1.2f, -11.f), &amb);
    //InfiniteLine l1(Ray(Point(-1.24626f,4.3464f,-4.932f),Vector(-0.0842391f,0.0448591f,0.995435f)), &defSh);

    GeometryGroup scene;

    //scene.primitives.push_back(&l1);

    InfinitePlane p2(Point(0, 0.1f, 0), Vector(0, 0, 1), &defSh);

    scene.primitives.push_back(&p1);
    scene.primitives.push_back(&p2);

    //scene.primitives.push_back(&s1);
    scene.primitives.push_back(&s2);
    scene.primitives.push_back(&s3);

    //Sphere lens(Point(0,  10.5f,  20.f), 2 , &lensShader);
    //scene.primitives.push_back(&lens);

    scene.rebuildIndex();

	PerspectiveCamera cam2(Point(0, 2.5f, 15.f), Vector(0, 0, -1), Vector(0, 1, 0), 60,
		std::make_pair(img.width(), img.height()));
	PerspectiveCamera cam1(Point(0, 15.f, 20.f), Vector(0, -1, 0), Vector(0, 0, -1), 60,
		std::make_pair(img.width(), img.height()));

    // - Camera position
    // - look at point (or forward vector)
    // - up vector
    // - view angle
    // - image resolution
    // Parameters:
    // (1)_focalLength,
	// (2) _lensAperture,
	// (3) _lensDistance
	// (4) _numSamples
	// (5) _adjustFocus (to look at point)
    //PerspectiveLensCamera cam3(Point(0, 2.5f, 15.f), Vector(0, 0, -1), Vector(0, 1, 0), 60,
	//	std::make_pair(img.width(), img.height()),5.6f,0.9f,0.f,0.f);
    PerspectiveLensCamera cam3(Point(0, 2.5f, 12.5f), Point(1.f,2.1f,5.f), Vector(0, 1, 0), 60,
		std::make_pair(img.width(), img.height()),0.93f,0.0f,1.1f,32,true);

    OrthographicCamera cam4(Point(0, 2.5f, 30.f), Point(1.f,1.1f,1.f), Vector(0, 1, 0), 90,
		std::make_pair(img.width(), img.height()));

    PerspectiveCamera cam5(Point(0, 2.5f, 10.f), Point(1.f,1.1f,5.f), Vector(0, 1, 0), 60,
		std::make_pair(img.width(), img.height()));

	cam1.addRef();
	cam3.addRef();
	cam4.addRef();
	cam5.addRef();

	//Set up the integrator
	IntegratorImpl integrator;
	integrator.addRef();
	integrator.scene = &scene;

	PointLightSource pls;
	PointLightSource pls2;

	pls.falloff = float4(0, 0, 1, 0);
	pls.intensity  = float4::rep(1.0f);
	pls.position = Point(-2.473637f, 3.119330f, 15.571486f);

    pls2.falloff = float4(0, 0, 1, 0);
	pls2.intensity  = float4::rep(0.5f);
	pls2.position = Point(-2.473637f, 3.119330f, -8.f);

	integrator.lightSources.push_back(pls);
	//integrator.lightSources.push_back(pls2);

	integrator.ambientLight = float4::rep(0.1f);

	/*
	EyeLightIntegrator integrator;
	integrator.addRef();
	integrator.scene = &p1;
    */

	DefaultSampler sampDef;
	sampDef.addRef();


    /*
	RegularSampler regSamp;
	regSamp.addRef();
	regSamp.samplesX = 4;
	regSamp.samplesY = 4;

	RandomSampler randSamp;
	randSamp.addRef();
	randSamp.sampleCount = 16;

	StratifiedSampler stratSamp;
	stratSamp.addRef();
	stratSamp.samplesX = 4;
	stratSamp.samplesY = 4;
    */
	HaltonSampleGenerator haltonSamp;
	haltonSamp.addRef();
	haltonSamp.sampleCount = 4;


	SmartPtr<Sampler> samplers[] = {&haltonSamp}; //, &sampDef, &regSamp, &randSamp, &stratSamp};

	Renderer r;
	r.camera = &cam3;
	r.integrator = &integrator;
	r.target = &img;

    //PerspectiveLensCamera cam = cam3;
    //float focus = 1.0f;

	for(int s = 0; s < 1; s++)
	{
	    //focus -= 0.01f;
        //cam = PerspectiveLensCamera(Point(0, 2.5f, 15.f), Point(1.f,1.1f,1.f), Vector(0, 1, 0), 60,
		//std::make_pair(img.width(), img.height()),0.93f,0.4f,1.1f,3,true);
		r.sampler = samplers[0];
		r.render();
		std::stringstream ssm;
		ssm << "result_ex3_" << s << ".png" << std::flush;
		img.writePNG(ssm.str());

	}

}

