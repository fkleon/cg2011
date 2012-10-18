#ifndef __INCLUDE_GUARD_EA166321_FE56_436D_9533_087DA72CB708
#define __INCLUDE_GUARD_EA166321_FE56_436D_9533_087DA72CB708
#ifdef _MSC_VER
	#pragma once
#endif


#include "../rt/basic_definitions.h"

//The base interface of a shader to an integrator. The integrator only understands
//	and queries the functions defined in this class. All functions work in the context
//	of the current intersection and are immutable (always return the same value).
struct Shader : RefCntBase
{
	//Returns the reflectance of the surface at the point for which the shader
	//	is invoked when illuminating from a point light source. The cosine term to the
	//	light source needs to be calculated here as well.
	//Return float4::rep(0.f) if the shader does not reflectance (an ambient only shader
	//	for example)
	virtual float4 getReflectance(const Vector &_outDir, const Vector &_inDir) const { return float4::rep(0.f);}

	//The ambient coefficient of the material at the intersection point
	//Return float4::rep(0.f) if the shader does not support ambient lighting
	virtual float4 getAmbientCoefficient() const { return float4::rep(0.f);}

	//The radiance that does not come from direct illumination (reflected ray
	//	from a mirror for ex.)
	//Return float4::rep(0.f) if the shader does not support this functionality
	virtual float4 getIndirectRadiance(const Vector &_out, Integrator *_integrator) const { return float4::rep(0.f);}

    // returns if the shader is transparent
    virtual bool isTransparent() const { return false; };

    // returns if the shader is reflective
    virtual bool isReflective() const { return false; };

    // returns a transparency term for shadow ray testing, each component between 0 and 1.
    //Return float4(rep(0.f) if material is not transparent at all
	virtual float4 getTransparency(ShadowRay &_in, Integrator *_integrator) const {
            /*
            std::cout << "hit something in-transparent. hits:" << _in.hitCounts <<
            " origin:" << _in.o[0] <<","<< _in.o[1] <<","<< _in.o[2] <<" direction: "<<
            _in.d[0] << _in.d[] << _in.dstd::endl;
            */
            return float4::rep(0.f);
    }
};

//A class that defines the interface between a shader and a primitive. Used for primitive
//	independent shaders.
struct PluggableShader : public Shader
{
	//Creates a copy of the shader. Cloning is necessary to guarantee
	//	that shaders remain immutable in recursive or multi-threaded
	//	integration. The integrator usually keeps a copy of the shader, which should
	//	not get modified by subsequent calls to setPosition() for other
	//	intersections.
	virtual SmartPtr<PluggableShader> clone() const = 0;

	//Sets the surface point which is shaded
	virtual void setPosition(const Point& _point) {};
	//Sets the normal to the surface point which is shaded
	virtual void setNormal(const Vector& _normal) {};

	//Sets the texture coordinates for the intersection
	virtual void setTextureCoord(const float2& _texCoord) {};

    //Sets the vertices of the face of the shader
    //needed for bump mapping
	virtual void setVertices(const Point& _v1, const Point& _v2, const Point& _v3) {};

    //Sets the tex coordinates of the vertices
    // needed for bump mapping
	virtual void setTexels(float2 _tex1, float2 _tex2, float2 _tex3) {};
};

//A helper macro to implement default cloning
#define _IMPLEMENT_CLONE(_NAME)                                                \
	virtual SmartPtr<PluggableShader> clone() const                            \
	{                                                                          \
		SmartPtr<_NAME> ret = new _NAME;                                       \
		*ret = *this;                                                          \
		                                                                       \
		return ret;                                                            \
	}                                                                          \


#endif //__INCLUDE_GUARD_EA166321_FE56_436D_9533_087DA72CB708
