#include "stdafx.h"

#include "core/image.h"
#include "rt/basic_definitions.h"
#include "rt/geometry_group.h"

#include "impl/lwobject.h"
#include "impl/phong_shaders.h"
#include "impl/basic_primitives.h"
#include "impl/perspective_camera.h"
#include "impl/integrator.h"
#include "impl/photonMapping_Integrator.h"
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

void doit()
{
	//Image img(600, 400);
	Image img(1280, 960);

	img.addRef();

	//Set up the scene
	GeometryGroup scene(ACC_STRUCT);

	LWObject cow;
	//cow.read("models/cow.obj", true);
	cow.read("models/untitled.obj", true);
	//cow.read("models/house_obj.obj", true);
	//cow.read("models/3spheretest.obj", true);
	//cow.read("models/TheDragon.obj", true);
	cow.addReferencesToScene(scene.primitives);
/*
    ((TexturedPhongShader*)cow.materials[cow.materialMap["Material.001"]].shader.data())->diffTexture->filterMode = Texture::TFM_Bilinear;
 	((TexturedPhongShader*)cow.materials[cow.materialMap["Material.001"]].shader.data())->ambientTexture->filterMode = Texture::TFM_Bilinear;
 	((TexturedBumpPhongShader*)cow.materials[cow.materialMap["Material.001"]].shader.data())->bumpTexture->filterMode = Texture::TFM_Bilinear;
*/
/*
    SmartPtr<Image> imag;
    imag->readPNG("models/tiles.png");

    SmartPtr<Texture> tiles;
    tiles->image = imag;

    ((TexturedBumpPhongShader*)cow.materials[cow.materialMap["Material.001"]].shader.data())->diffTexture = tiles;
*/
    MirrorPhongShader sh77;
    sh77.diffuseCoef = float4(220.f/255, 193.f/255, 42.f/255, 0);
    sh77.ambientCoef = sh77.diffuseCoef;
    sh77.specularCoef = float4::rep(0.0f);
    sh77.specularExponent = 10000.f;
    sh77.reflCoef = 0.6f;
    sh77.addRef();

    CheckBoard3DShader check;
	check.scale = float4::rep(1);
	check.addRef();

    float4 woodenBrownDark = float4(103.f/255,53.f/255,3.f/255,0);
    float4 woodenBrownLight = float4(167.f/255,86.f/255,7.f/255,0);

    float4 kBraun = float4(181.f/255,58.f/255,4.f/255,0);
    float4 dBraun = float4(119.f/255,15.f/255,0.f/255,0);

    float4 marmorDunkel = float4(0.6,0.6,0.6,0);
    float4 marmorHell = float4(0.4,0.4,0.4,0);

    SmartPtr<ProceduralTexture> woodTex = new ProceduralWoodTexture(woodenBrownDark, woodenBrownLight, 0.1f, 0.8f);
    SmartPtr<ProceduralTexture> marbleTex = new ProceduralMarbleTexture(kBraun,dBraun, 0.8f, 0.8f);
    SmartPtr<ProceduralTexture> planetTex = new ProceduralPlanetTexture();
    SmartPtr<ProceduralTexture> waterTex = new ProceduralWaterTexture(0.5f, 0.9f);
    SmartPtr<ProceduralTexture> floorMarbleTex = new ProceduralMarbleTexture(marmorHell,marmorDunkel, 0.4f, 0.3f);

	RefractivePhongShader shader;
    shader.refractionIndex = 1.6f;
    shader.diffuseCoef = float4(0.2f, 0.2f, 0, 0);
	shader.ambientCoef = shader.diffuseCoef;
	shader.specularCoef = float4::rep(0.8f);
	shader.specularExponent = 10000.f;
	shader.transparency = float4::rep(0.7f);
	shader.addRef();

    cow.materials[cow.materialMap["glass"]].shader = &shader;

	RefractivePhongShader diamondShader;
    diamondShader.refractionIndex = 2.6f;
    diamondShader.diffuseCoef = float4(0.2f, 0.2f, 0, 0);
	diamondShader.ambientCoef = diamondShader.diffuseCoef;
	diamondShader.specularCoef = float4::rep(0.8f);
	diamondShader.specularExponent = 10000.f;
	diamondShader.transparency = float4::rep(0.7f);
	diamondShader.addRef();

    cow.materials[cow.materialMap["diamant"]].shader = &diamondShader;

    ProceduralBumpShader procBumpShader(marbleTex);
    procBumpShader.diffuseCoef = float4(110.f/255,54.f/255,9.f/255,0.f);
    procBumpShader.ambientCoef = procBumpShader.diffuseCoef;
    procBumpShader.specularCoef = float4::rep(0.3f);
    procBumpShader.specularExponent = 10000.f;

    ProceduralBumpShader floor(floorMarbleTex);
    floor.diffuseCoef = float4(0.5,0.5,0.5,0);
    floor.ambientCoef = floor.diffuseCoef;
    floor.specularCoef = float4::rep(0.3f);
    floor.specularExponent = 10000.f;

    ProceduralBumpShader planet(planetTex);
    planet.diffuseCoef = float4(110.f/255,54.f/255,9.f/255,0.f);
    planet.ambientCoef = planet.diffuseCoef;
    planet.specularCoef = float4::rep(0.3f);
    planet.specularExponent = 10000.f;

    ProceduralBumpShader procBumpShader2(woodTex);
    procBumpShader2.diffuseCoef = float4(110.f/255,54.f/255,9.f/255,0.f);
    procBumpShader2.ambientCoef = procBumpShader2.diffuseCoef;
    procBumpShader2.specularCoef = float4::rep(0.3f);
    procBumpShader2.specularExponent = 10000.f;

    ProceduralRefractiveBumpShader water(waterTex);
    water.refractionIndex = 1.4f;
    water.diffuseCoef = float4(0.6,0.6,0.8,0.f);
    water.ambientCoef = water.diffuseCoef;
    water.specularCoef = float4::rep(0.3f);
	water.transparency = float4::rep(0.7f);
    water.specularExponent = 10000.f;
    water.addRef();

    cow.materials[cow.materialMap["water"]].shader = &water;
    cow.materials[cow.materialMap["dragonskin"]].shader = &procBumpShader;
    cow.materials[cow.materialMap["podest"]].shader = &procBumpShader2;
    cow.materials[cow.materialMap["Material.001_stones_diffuse.png"]].shader = &floor;

    DefaultPhongShader defaultPhong;
    defaultPhong.diffuseCoef = float4(0.3f,0.3f,0.3f, 0);
    defaultPhong.ambientCoef = defaultPhong.diffuseCoef;
    defaultPhong.specularCoef = float4::rep(0.0f);
    defaultPhong.specularExponent = 10000.f;
    defaultPhong.addRef();

    //cow.materials[cow.materialMap["Gold"]].shader = &sh77;
    //cow.materials[cow.materialMap["glass"]].shader = &procBumpShader;


	InfinitePlane p2(Point(-5.f, 0.f, 0.f), Vector(1, 0, 0), &defaultPhong);
	InfinitePlane p1(Point(0, -2.f, 0), Vector(0, 1, 0), &defaultPhong);
    //scene.primitives.push_back(&p1);
    //scene.primitives.push_back(&p2);

    // sphere test purpose
    Sphere s2(Point(0.1,    2.0f,     11), 2.0f, &planet);

    Sphere s3(Point(6.1,    1.0f,     10), 1.0f, &shader);
    Sphere s4(Point(1.1,    1.5f,     14.5f), 1.5f, &shader);

    scene.primitives.push_back(&s2);
    scene.primitives.push_back(&s3);
    scene.primitives.push_back(&s4);

    //InfinitePlane plane(Point(0,0.f,0),Vector(0,1,0), &check);

	PerspectiveCamera cam1(Point(-10.398149f, -7.266083f, 6.348377f), Point(-6.324413f, -2.961229f, 4.203216f), Vector(0, 0, 1), 30,
		std::make_pair(img.width(), img.height()));
    //cam1.addRef();

    //PerspectiveLensCamera cam3(Point(-10.398149f, -7.266083f, 6.348377f), Point(-6.324413f, -2.961229f, 4.203216f), Vector(0, 0, 1), 30,
	//	std::make_pair(img.width(), img.height()),0.9f,0.0f,1.f,4,false);

    PerspectiveLensCamera cam4(Point(30.f, 6.f, 0), Point(4,2.f,0), Vector(0, 1, 0), 50,
		std::make_pair(img.width(), img.height()),0.9f,0.3f,1.f,16,true);

    //PerspectiveLensCamera cam1(Point(30.f, 0.f, 0.f), Point(0, 0, 0), Vector(0, 1, 0), 60,
	//	std::make_pair(img.width(), img.height()),0.9f,0.0f,1.f,4,true);
    //cam1.addRef();


	PerspectiveCamera cam2(Point(6.f, 1.f, 0.f), Point(0, -1, 0), Vector(0, 1, 0), 60,
		std::make_pair(img.width(), img.height()));
	//cam2.addRef();


    // index scene
    scene.rebuildIndex();

    //Set up the integrator
	PhotonMap_Integrator integrator;
	//IntegratorImpl integrator;
	integrator.addRef();
	integrator.scene = &scene;

	PointLightSource pls;
	pls.falloff = float4(0, 0, 1, 0);
	pls.intensity  = float4::rep(0.6f);
	pls.position = Point(7.f, 3.f, 0.f);
	integrator.lightSources.push_back(pls);

	PointLightSource pls2;
	pls2.falloff = float4(0, 0, 1, 0);
	pls2.intensity  = float4::rep(0.7f);
	pls2.position = Point(30.f, 6.f, 0.f);
	integrator.lightSources.push_back(pls2);

    PointLightSource pls3;
	pls3.falloff = float4(0, 0, 1, 0);
	pls3.intensity  = float4(0.3f,0.1f,0.1f,0);
	pls3.position = Point(7.f, 3.f, -8.f);
	integrator.lightSources.push_back(pls3);

	integrator.ambientLight = float4::rep(0.1f);

    integrator.start_photonmapping();

	DefaultSampler samp;
	samp.addRef();

	HaltonSampleGenerator halton;
	halton.sampleCount = 4;

	//Render
	Renderer r;
	r.integrator = &integrator;
	r.target = &img;
	r.sampler = &halton;

	r.camera = &cam4;
	r.render(32,23);
	img.writePNG("frey_leonhardt_rc.png");

}
