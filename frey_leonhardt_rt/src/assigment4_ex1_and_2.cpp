#include "stdafx.h"

#include "core/image.h"
#include "rt/basic_definitions.h"
#include "rt/geometry_group.h"

#include "impl/lwobject.h"
#include "impl/phong_shaders.h"
#include "impl/basic_primitives.h"
#include "impl/perspective_camera.h"
#include "impl/integrator.h"
#include "rt/renderer.h"
#include "impl/samplers.h"

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

void assigment4_1_and_2()
{
	Image img(400, 300);
	img.addRef();

	//Set up the scene
	GeometryGroup scene;
	LWObject cow;
	cow.read("models/cow.obj", true);
	//cow.read("models/3spheretest.obj", true);
	cow.addReferencesToScene(scene.primitives);

    DefaultPhongShader defSh2;
    defSh2.diffuseCoef = float4(0.2f, 0, 0, 0);
    defSh2.ambientCoef = defSh2.diffuseCoef;
    defSh2.specularCoef = float4::rep(0.6f);
    defSh2.specularExponent = 50000.f;
    defSh2.addRef();

	RefractivePhongShader shader;
    shader.refractionIndex = 1.6f;
    shader.diffuseCoef = float4(0.2f, 0.2f, 0, 0);
	shader.ambientCoef = shader.diffuseCoef;
	shader.specularCoef = float4::rep(0.8f);
	shader.specularExponent = 10000.f;
	shader.transparency = float4::rep(0.7f);
	shader.addRef();

	TexturedBumpPhongShader sh4;
	sh4.diffuseCoef = float4(0.2f, 0.2f, 0, 0);
	sh4.ambientCoef = sh4.diffuseCoef;
	sh4.specularCoef = float4::rep(0.8f);
	sh4.specularExponent = 10000.f;
	sh4.reflCoef = 0.4f;
	sh4.addRef();

	//cow.materials[cow.materialMap["Floor"]].shader = &sh4;
    //cube.materials[cube.materialMap["Material"]].shader = &shader;

    BumpMirrorPhongShader sh77;
    sh77.diffuseCoef = float4(0.2f, 0.2f, 0, 0);
    sh77.ambientCoef = sh77.diffuseCoef;
    sh77.specularCoef = float4::rep(0.8f);
    sh77.specularExponent = 10000.f;
    sh77.reflCoef = 0.4f;
    sh77.addRef();

    //cow.materials[cow.materialMap["Floor"]].shader = &sh77;
    cow.materials[cow.materialMap["Cow_White"]].shader = &shader;


 	//Enable bi-linear filtering on the walls
	((TexturedPhongShader*)cow.materials[cow.materialMap["Stones"]].shader.data())->diffTexture->filterMode = Texture::TFM_Bilinear;
 	((TexturedPhongShader*)cow.materials[cow.materialMap["Stones"]].shader.data())->ambientTexture->filterMode = Texture::TFM_Bilinear;
 	((TexturedBumpPhongShader*)cow.materials[cow.materialMap["Stones"]].shader.data())->bumpTexture->filterMode = Texture::TFM_Bilinear;


	//Set up the cameras
	PerspectiveCamera cam1(Point(-9.398149f, -6.266083f, 5.348377f), Point(-6.324413f, -2.961229f, 4.203216f), Vector(0, 0, 1), 30,
		std::make_pair(img.width(), img.height()));

	PerspectiveCamera cam2(Point(2.699700f, 6.437226f, 0.878297f), Point(4.337114f, 8.457443f,- 0.019007f), Vector(0, 0, 1), 30,
		std::make_pair(img.width(), img.height()));

    PerspectiveLensCamera cam3(Point(-9.398149f, -6.266083f, 5.348377f), Point(-6.324413f, -2.961229f, 4.203216f), Vector(0, 0, 1), 30,
		std::make_pair(img.width(), img.height()),0.9f,0.2f,1.f,4,true);

	cam1.addRef();
	cam2.addRef();
	cam3.addRef();

    // index scene
    scene.rebuildIndex();

	//Set up the integrator
	IntegratorImpl integrator;
	integrator.addRef();
	integrator.scene = &scene;
	PointLightSource pls;

	pls.falloff = float4(0, 0, 1, 0);

	pls.intensity  = float4::rep(0.9f);
	pls.position = Point(-2.473637f, 3.119330f, 9.571486f);
	integrator.lightSources.push_back(pls);

	integrator.ambientLight = float4::rep(0.1f);

	DefaultSampler samp;
	samp.addRef();

	HaltonSampleGenerator haltonSamp;
	haltonSamp.sampleCount = 4;

	//Render
	Renderer r;
	r.integrator = &integrator;
	r.target = &img;
	r.sampler = &samp;

	r.camera = &cam1;
	r.render();
	img.writePNG("rc1.png");

	//For seeing the difference in texture filtering
	//r.camera = &cam2;
	//r.render();
	//img.writePNG("result_cam2.png");

	// new lens camera
    //r.camera = &cam3;
	//r.render();
	//img.writePNG("result_cam3.png");
}

