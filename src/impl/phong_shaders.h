#ifndef __INCLUDE_GUARD_810F2AF5_7E81_4F1E_AA05_992B6D2C0016
#define __INCLUDE_GUARD_810F2AF5_7E81_4F1E_AA05_992B6D2C0016
#ifdef _MSC_VER
	#pragma once
#endif


#include "../rt/shading_basics.h"
#include "perlin.h"

#define DELTA  0.00000001
#define EPSILON   0.01f



struct DefaultAmbientShader : public PluggableShader
{
	float4 ambientCoefficient;

	virtual float4 getAmbientCoefficient() const { return ambientCoefficient; }

	_IMPLEMENT_CLONE(DefaultAmbientShader);

	virtual ~DefaultAmbientShader() {}
};

//A base class for a phong shader.
class PhongShaderBase : public PluggableShader
{
public:

	virtual void getCoeff(float4 &_diffuseCoef, float4 &_specularCoef, float &_specularExponent) const = 0;
	virtual Vector getNormal() const = 0;

	virtual float4 getReflectance(const Vector &_outDir, const Vector &_inDir) const
	{
		float4 Cs, Cd;
		float Ce;

		getCoeff(Cd, Cs, Ce);

		Vector normal = getNormal();
		Vector halfVect = ~(~_inDir + ~_outDir);
		float specCoeff = std::max(halfVect * normal, 0.f);
		specCoeff = exp(log(specCoeff) * Ce);
		float diffCoeff = std::max(normal * ~_inDir, 0.f);

		return float4::rep(diffCoeff) * Cd + float4::rep(specCoeff) * Cs;
	}

	virtual ~PhongShaderBase() {}
};


//The default phong shader
class DefaultPhongShader : public PhongShaderBase
{
protected:
	Vector m_normal; //Stored normalized

public:
	float4 diffuseCoef;
	float4 specularCoef;
	float4 ambientCoef;
	float specularExponent;

	//Get the ambient coefficient for the material
	virtual float4 getAmbientCoefficient() const { return ambientCoef; }
	virtual void getCoeff(float4 &_diffuseCoef, float4 &_specularCoef, float &_specularExponent) const
	{
		_diffuseCoef = diffuseCoef;
		_specularCoef = specularCoef;
		_specularExponent = specularExponent;
	}

	virtual Vector getNormal() const { return m_normal;}
	virtual void setNormal(const Vector& _normal) { m_normal = ~_normal;}

	_IMPLEMENT_CLONE(DefaultPhongShader);

};

//A phong shader that supports texturing
class TexturedPhongShader : public DefaultPhongShader
{
protected:
	float2 m_texCoord;
public:
	SmartPtr<Texture> diffTexture;
	SmartPtr<Texture> ambientTexture;
	SmartPtr<Texture> specTexture;

	virtual void setTextureCoord(const float2& _texCoord) { m_texCoord = _texCoord;}

	virtual float4 getAmbientCoefficient() const
	{
		float4 ret = DefaultPhongShader::getAmbientCoefficient();

		if(ambientTexture.data() != NULL)
			ret = ambientTexture->sample(m_texCoord);

		return ret;
	}

	virtual void getCoeff(float4 &_diffuseCoef, float4 &_specularCoef, float &_specularExponent) const
	{
		DefaultPhongShader::getCoeff(_diffuseCoef, _specularCoef, _specularExponent);

		if(diffTexture.data() != NULL)
			_diffuseCoef = diffTexture->sample(m_texCoord);

		if(specTexture.data() != NULL)
			_specularCoef = specTexture->sample(m_texCoord);
	}


	_IMPLEMENT_CLONE(TexturedPhongShader);
};

// a perfect mirror shader with hardcoded bumps
class BumpMirrorPhongShader : public DefaultPhongShader
{
protected:
	float2 m_texCoord;
	Point m_position;
	Vector m_tang, m_biNorm;
public:
	float reflCoef;

	virtual void setPosition(const Point& _point) { m_position = _point; }

	//Set the tangent to (0, 1, 0)
	virtual void setNormal(const Vector& _normal)
	{
		DefaultPhongShader::setNormal(_normal);
		m_tang = Vector(0, -1, 0);
		m_biNorm = ~_normal % m_tang;
	}

	virtual Vector getNormal() const
	{
		Vector ret = m_normal;

		float d;
		float2 t1 = float2(modf(m_texCoord.x, &d), modf(m_texCoord.y, &d)) - float2(0.5f, 0.5f);

		float dist = t1.x * t1.x  + t1.y * t1.y;
		const float R = 0.25;
		const float DIV = 10.f;
		if(dist < R * R)
		{
			Vector newNorm(t1.x / DIV, t1.y / DIV, sqrtf(R * R - dist / (DIV * DIV)));
			ret = newNorm.x * m_tang + newNorm.y * m_biNorm + newNorm.z * m_normal;
			ret = ~ret;
		}

		return ret;
	}

	virtual float4 getIndirectRadiance(const Vector &_out, Integrator *_integrator) const
	{
		Ray r;
		r.o = m_position;

		Vector n = getNormal();
		Vector v = n * fabs(n * _out);
		r.d = _out + 2 * (v - _out);

		return _integrator->getRadiance(r) * float4::rep(reflCoef);
	}

	virtual void setTextureCoord(const float2& _texCoord) { m_texCoord = _texCoord;}

	_IMPLEMENT_CLONE(BumpMirrorPhongShader);

};

// a perfect mirror shader
class MirrorPhongShader : public DefaultPhongShader
{
protected:
	float2 m_texCoord;
	Point m_position;
public:
	float reflCoef;

	virtual void setPosition(const Point& _point) { m_position = _point; }

	virtual Vector getNormal() const
	{
		return m_normal;
	}

    virtual bool isReflective() const
    {
        return true;
    }

	virtual float4 getIndirectRadiance(const Vector &_out, Integrator *_integrator) const
	{
		Ray r;
		r.o = m_position;

		Vector n = getNormal();
		Vector v = n * fabs(n * _out);
		r.d = _out + 2 * (v - _out);

		return _integrator->getRadiance(r) * float4::rep(reflCoef);
	}

	virtual void getPhotonInformation(const Vector &_out, float &_reflectionProbability, Ray &_reflection)
	{
	    Ray r;
		r.o = m_position;

		Vector n = getNormal();
		Vector v = n * fabs(n * _out);
		r.d = _out + 2 * (v - _out);

		_reflection = r;
		_reflectionProbability = reflCoef;
	}

	virtual void setTextureCoord(const float2& _texCoord) { m_texCoord = _texCoord;}

	_IMPLEMENT_CLONE(MirrorPhongShader);

};


/*
* A bump mapping shader supporting textures
*/
class TexturedBumpPhongShader : public TexturedPhongShader
{
protected:
	Point m_position;
public:
    Point vert0, vert1, vert2; // our vertices to calculate pertubed normal
    float2 tex0, tex1, tex2; // texture coordinates of those vertices

	SmartPtr<Texture> bumpTexture;
    float bumpIntensity;
	float reflCoef;

	virtual void setPosition(const Point& _point)
	{
	    m_position = _point;
    }

	virtual void setNormal(const Vector& _normal)
	{
	    m_normal = ~_normal;
	}

	virtual Vector getNormal() const
    {
        Vector ret = m_normal;
        Vector heightNormal;

        // get normal from height map if supported
        if (bumpTexture.data() == NULL) {
            std::cout << "booh! missing bump texture." << std::endl;
            return ret; // no bmp texture, return interpolated face normal
        }
        else {
            // ask texture for normal
            heightNormal = bumpTexture->sampleBumpTexture(m_texCoord); // magic
        }

        /*
        //std::cout << "heightNormal " << heightNormal[0] << "," << heightNormal[1] << "," << heightNormal[2] << std::endl;
        return ret;
        */

        // now we have to calculate tangent and binormal vectors
        // from the original face plane and tex coordinates
		Vector tang, binorm;

		// differences in texture coordinates
		// we need y-axis and determinant of matrix
		// for calculating the tangent.
		float du1 = tex0.x - tex2.x;
		float du2 = tex1.x - tex2.x;
		float dv1 = tex0.y - tex2.y;
		float dv2 = tex1.y - tex2.y;

		// face basis vectors to calculate orthogonal later
		Vector dp1 = vert0 - vert2;
		Vector dp2 = vert1 - vert2;

		// determinant
		float det = du1 * dv2 - dv1 * du2;
		if (fabsf(det) > 0.000001f) // shouldn't be 0
		{
			float inv_det = 1.0f / det;
			tang = ~(inv_det * ( dv2 * dp1 - dv1 * dp2));
			binorm = m_normal % tang; // binormal can be calculated by cross product of face normal and tangent, noth vectors are normalized already
            //binorm = ~(inv_det * (-du2 * dp1 + du1 * dp2));
		} else
            return ret;

		// get our new normal in object space
		// by rotating it
		ret = ~(m_normal + bumpIntensity * (heightNormal.x * tang + heightNormal.y * binorm));

        return ret;
    }

    virtual void setVertices(const Point& _v1, const Point& _v2, const Point& _v3)
    {
        vert0 = _v1;
        vert1 = _v2;
        vert2 = _v3;
    }

	virtual void setTexels(float2 _tex1, float2 _tex2, float2 _tex3)
	{
        tex0 = _tex1;
        tex1 = _tex2;
        tex2 = _tex3;
	}


	virtual float4 getIndirectRadiance(const Vector &_out, Integrator *_integrator) const
	{
		Ray r;
		r.o = m_position;

		Vector n = getNormal();
		Vector v = n * fabs(n * _out);
		r.d = _out + 2 * (v - _out);

		return _integrator->getRadiance(r) * float4::rep(reflCoef);
	}

	virtual void setTextureCoord(const float2& _texCoord) { m_texCoord = _texCoord;}

	_IMPLEMENT_CLONE(TexturedBumpPhongShader);

};


// refractive & reflective transparency
// using snell's law and fresnel formula
class RefractivePhongShader : public DefaultPhongShader
{
protected:
	float2 m_texCoord;
	Point m_position;

public:
	float4 transparency;
	float refractionIndex;

	virtual void setPosition(const Point& _point) { m_position = _point; }

	virtual Vector getNormal() const
	{
		return m_normal;
	}

    virtual bool isTransparent() const { return true; };

	virtual float4 getIndirectRadiance(const Vector &_out, Integrator *_integrator) const
	{
		Ray refl,refr; // outgoing reflection + refraction rays
        float refractionLast = _integrator->getCurrentRefractionIndex(); // refraction index of last material
        float refractionCur = refractionIndex; // refraction index of current material
        float refractionRatio, cosThetaIn, cosThetaOut;
        Vector n = getNormal(); // current normal
        Vector tangentIn, tangentOut;
        Vector out = ~_out; // normalize!

        // new ray starting positions
		refl.o = m_position;
		refr.o = m_position;

        // set refraction indices
		refl.curRefractionIndex = refractionLast;

        // to simplify things we assume that transmittions only happen between air and one material
        if (refractionLast != refractionCur) {
            //std::cout << "transisting from " << refractionLast << " to " << refractionCur << std::endl;
            refr.curRefractionIndex = refractionCur; // entering material
		}
        else {
            //std::cout << "transisting from " << refractionLast << " to air." << std::endl;
            refr.curRefractionIndex = 1.00029f; // second intersection, reset to air.
            refractionCur = 1.00029f;
            n = -n; // turn around normal, we are leaving!
        }

        // calculate tangent of incoming vector
		cosThetaIn = fabs(n * out); // theta of incoming ray
		Vector normalCompIn = n * cosThetaIn;
		tangentIn = normalCompIn - out; // tangent, length equal to sin of incoming ray

        // direction of reflected ray
		refl.d = out + 2 * tangentIn;

        /*
        * new check is farther down with sinSquare
        *
		if (sqrt(1-cosThetaIn*cosThetaIn) > (refractionCur/refractionLast)) {
            std::cout << "norm len: " << n.len() << "  out len: " << out.len() << std::endl;
		    std::cout << "sin In: " << tangentIn.len() << std::endl;
		    std::cout << "Last refr: " << refractionLast << " Cur refr: " << refractionCur << std::endl;
            return _integrator->getRadiance(refl); // only reflection
		}
		*/

		refractionRatio = refractionLast / refractionCur;

		tangentOut = tangentIn * refractionRatio; // outgoing tangent

        // sinSquare theta t
        float sinSquare = 1.f- (refractionRatio*refractionRatio) * (1-cosThetaIn*cosThetaIn);
        //float sinSquare = (refractionRatio*refractionRatio) * (1-cosThetaIn*cosThetaIn);

		// check total internal reflection
        if (sinSquare < 0) {
        //if (sinSquare > 1) {
            //std::cout << "Last refr: " << refractionLast << " Cur refr: " << refractionCur << std::endl;
            //std::cout << "cosThetaIn: " << cosThetaIn << std::endl;
            //std::cout << "sinSquare: " << sinSquare << std::endl;
            return _integrator->getRadiance(refl); // only reflection!
        }

        Vector normalCompOut = (-sqrtf(sinSquare)) * n;
        //Vector normalCompOut = (-sqrtf(1-sinSquare)) * n;
        cosThetaOut = normalCompOut.len();

        // direction of refracted ray (using Snell's law)
		refr.d = tangentOut + normalCompOut;
        refr.o = refr.o + Primitive::INTEPS() * refr.d;

        // FRESNEL ORSMNSESS
        float rs = (refractionLast * cosThetaIn - refractionCur * cosThetaOut) / (refractionLast * cosThetaIn + refractionCur * cosThetaOut);
        float sPolarized = rs*rs; // s-polarized light

        float rp = (refractionLast * cosThetaOut - refractionCur * cosThetaIn) / (refractionLast * cosThetaOut + refractionCur * cosThetaIn);
        float pPolarized = rp*rp; // p-polarized light

        float np = (sPolarized+pPolarized) * 0.5f; // unpolarized light (average)

		float4 reflectionRadiance = _integrator->getRadiance(refl);
        float4 refractionRadiance = _integrator->getRadiance(refr);

        //std::cout <<  "refr: " << 1.0f-np << " refl: " << np << std::endl;

		if (np < 0.005f) // if reflectance is smaller then 0,5%, ignore the reflection ray.
		{
		    //std::cout << "reflectance too low: " << np << std::endl;
		    return refractionRadiance;
		} else {
            return refractionRadiance * float4::rep(1.0f-np) + float4::rep(np) * reflectionRadiance; // reflectance + refraction = 1.0
		}
	}

    // information for the photon tracing
	virtual void getPhotonInformation(Vector &_out, float &_reflectionProbability, float &_refractionProbability, Ray &_reflection, Ray &_refraction)
	{
	    Ray refl,refr; // outgoing reflection + refraction rays

	    float refractionCur, refractionLast;

        // cehck normal to find out if we are leaving or coming
	    if (getNormal() * (-_out) > 0 ) {
            refractionCur = refractionIndex; // refraction index of current material
            refractionLast = 1.00029f;
	    } else {
            refractionCur = 1.00029f;
            refractionLast = refractionIndex;
	    }

        float refractionRatio, cosThetaIn, cosThetaOut;
        Vector n = getNormal(); // current normal
        Vector tangentIn, tangentOut;
        Vector out = ~_out; // normalize!

        // new ray starting positions
		refl.o = m_position;
		refr.o = m_position;

        // set refraction indices
		refl.curRefractionIndex = refractionLast;

        // to simplify things we assume that transmittions only happen between air and one material
        if (refractionLast != refractionCur) {
            //std::cout << "transisting from " << refractionLast << " to " << refractionCur << std::endl;
            refr.curRefractionIndex = refractionCur; // entering material
		}
        else {
            //std::cout << "transisting from " << refractionLast << " to air." << std::endl;
            refr.curRefractionIndex = 1.00029f; // second intersection, reset to air.
            refractionCur = 1.00029f;
            n = -n; // turn around normal, we are leaving!
        }

        // calculate tangent of incoming vector
		cosThetaIn = fabs(n * out); // theta of incoming ray
		Vector normalCompIn = n * cosThetaIn;
		tangentIn = normalCompIn - out; // tangent, length equal to sin of incoming ray

        // direction of reflected ray
		refl.d = out + 2 * tangentIn;

		refractionRatio = refractionLast / refractionCur;

		tangentOut = tangentIn * refractionRatio; // outgoing tangent

        // sinSquare theta t
        float sinSquare = 1.f- (refractionRatio*refractionRatio) * (1-cosThetaIn*cosThetaIn);
        //float sinSquare = (refractionRatio*refractionRatio) * (1-cosThetaIn*cosThetaIn);

		// check total internal reflection
        if (sinSquare < 0) {
            _refractionProbability = 0;
            _reflectionProbability = 1;
            _reflection = refl;
            _refraction = refr;
            return; // only reflection!
        }

        Vector normalCompOut = (-sqrtf(sinSquare)) * n;
        //Vector normalCompOut = (-sqrtf(1-sinSquare)) * n;
        cosThetaOut = normalCompOut.len();

        // direction of refracted ray (using Snell's law)
		refr.d = tangentOut + normalCompOut;
        refr.o = refr.o + Primitive::INTEPS() * refr.d;

        // FRESNEL ORSMNSESS
        float rs = (refractionLast * cosThetaIn - refractionCur * cosThetaOut) / (refractionLast * cosThetaIn + refractionCur * cosThetaOut);
        float sPolarized = rs*rs; // s-polarized light

        float rp = (refractionLast * cosThetaOut - refractionCur * cosThetaIn) / (refractionLast * cosThetaOut + refractionCur * cosThetaIn);
        float pPolarized = rp*rp; // p-polarized light

        float np = (sPolarized+pPolarized) * 0.5f; // unpolarized light (average)

        _reflectionProbability = np;
        _refractionProbability = 1-np;
        _reflection = refl;
        _refraction = refr;
	}

    virtual float4 getReflectance(const Vector &_outDir, const Vector &_inDir) const
	{
	    // no reflectance (fresnel will handle it)
		return float4(0,0,0,1);
	}

	virtual float4 getTransparency(ShadowRay &_in, Integrator *_integrator) const
	{
	    //std::cout << "setting transp, hits: " << _in.hitCounts << std::endl;
	    return transparency * _integrator->getShadow(_in);
	}


	virtual void setTextureCoord(const float2& _texCoord) { m_texCoord = _texCoord;}

	_IMPLEMENT_CLONE(RefractivePhongShader);

};

class ProceduralRefractiveBumpShader : public RefractivePhongShader
{
protected:
    SmartPtr<ProceduralTexture> proceduralTexture;
    float bumpIntensity;

public:
    ProceduralRefractiveBumpShader() {}

    ProceduralRefractiveBumpShader(SmartPtr<ProceduralTexture> _procTex)
    : proceduralTexture(_procTex)
    {
        std::cout << "Initalized procedural refractive bump shader with texture: " << _procTex->name << std::endl;
        bumpIntensity = proceduralTexture->bumpIntensity;
    }

    virtual Vector getNormal() const
    {
        if (proceduralTexture.data() == NULL) // woops, missing texture!
            return m_normal;
        if (bumpIntensity == 0) // nothing to do here
            return m_normal;

        // probe differences in all three dimensions
        // x-axis of sample is sufficient, because texture will only
        // return grey values.
        float bumpNeighborX1 = proceduralTexture->sampleBumpTexture(Point(m_position.x-EPSILON,m_position.y,m_position.z))[0];
        float bumpNeighborX2 = proceduralTexture->sampleBumpTexture(Point(m_position.x+EPSILON,m_position.y,m_position.z))[0];
        float bumpNeighborY1 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y-EPSILON,m_position.z))[0];
        float bumpNeighborY2 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y+EPSILON,m_position.z))[0];
        float bumpNeighborZ1 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y,m_position.z-EPSILON))[0];
        float bumpNeighborZ2 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y,m_position.z+EPSILON))[0];

        // compute variation on x and y axis
        float du = bumpNeighborX1 - bumpNeighborX2;
        float dv = bumpNeighborY1 - bumpNeighborY2;
        float dg = bumpNeighborZ1 - bumpNeighborZ2;

        Vector dif(du,dv,dg);
        return ~(m_normal-(bumpIntensity*dif));
	}

	_IMPLEMENT_CLONE(ProceduralRefractiveBumpShader);

};


// A basic bump mapping shader for procedural textures.
// Will create very discrete and smooth bumps - for a rougher experience,
// use ProceduralHardBumpShader
class ProceduralBumpShader : public DefaultPhongShader
{
protected:
    SmartPtr<ProceduralTexture> proceduralTexture;
    Point m_position;
    float bumpIntensity;

public:

    ProceduralBumpShader()
    {}

    ProceduralBumpShader(SmartPtr<ProceduralTexture> _procTex)
    : proceduralTexture(_procTex)
    {
        std::cout << "Initalized procedural bump shader with texture: " << _procTex->name << std::endl;
        bumpIntensity = proceduralTexture->bumpIntensity;
    }

    virtual void setPosition(const Point& _point)
    {
        m_position = _point;
    }

    // normal calculation from 3D textures.
    //
    // [References]
    // - http://digitalerr0r.wordpress.com/2011/05/18/xna-shader-programming-tutorial-26-bump-mapping-perlin-noise/
    // - http://www.codermind.com/articles/Raytracer-in-C++-Part-III-Textures.html
    // - http://http.developer.nvidia.com/GPUGems/gpugems_ch05.html
    virtual Vector getNormal() const
    {
        if (proceduralTexture.data() == NULL) // woops, missing texture!
            return m_normal;
        if (bumpIntensity == 0) // nothing to do here
            return m_normal;

        // probe differences in all three dimensions
        // x-axis of sample is sufficient, because texture will only
        // return grey values.
        float bumpNeighborX1 = proceduralTexture->sampleBumpTexture(Point(m_position.x-EPSILON,m_position.y,m_position.z))[0];
        float bumpNeighborX2 = proceduralTexture->sampleBumpTexture(Point(m_position.x+EPSILON,m_position.y,m_position.z))[0];
        float bumpNeighborY1 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y-EPSILON,m_position.z))[0];
        float bumpNeighborY2 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y+EPSILON,m_position.z))[0];
        float bumpNeighborZ1 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y,m_position.z-EPSILON))[0];
        float bumpNeighborZ2 = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y,m_position.z+EPSILON))[0];

        // compute variation on x and y axis
        float du = bumpNeighborX1 - bumpNeighborX2;
        float dv = bumpNeighborY1 - bumpNeighborY2;
        float dg = bumpNeighborZ1 - bumpNeighborZ2;

        /*
        float4 t = float4(1.f,0,0,du);
        float4 b = float4(0,1.f,0,dv);
        float4 a = float4(0,0,1.f,dg);
        */

        Vector dif(du,dv,dg);
        return ~(m_normal-(bumpIntensity*dif));
    }

    virtual void getCoeff(float4 &_diffuseCoef, float4 &_specularCoef, float &_specularExponent) const
    {
        // get default shading
        DefaultPhongShader::getCoeff(_diffuseCoef, _specularCoef, _specularExponent);

        // overwrite diffuse part
        if (proceduralTexture.data() != NULL)
            _diffuseCoef = proceduralTexture->sampleTexture(m_position);
    }

    _IMPLEMENT_CLONE(ProceduralBumpShader);
};

// this shader accentuates differences in the bump map textures
// way more than the default ProceduralHardShader due to a
// different calculation routine.
class ProceduralHardBumpShader : public ProceduralBumpShader
{
public:

    ProceduralHardBumpShader()
    {}

    ProceduralHardBumpShader(SmartPtr<ProceduralTexture> _procTex)
    : ProceduralBumpShader(_procTex) {}

    virtual Vector getNormal() const
    {
        if (proceduralTexture.data() == NULL) // woops, missing texture!
            return m_normal;
        if (bumpIntensity == 0) // nothing to do here
            return m_normal;

        /*
        // get surrounding texture to calculate difference
        // this is "noise" bump mapping.
        noiseCoef[0] = proceduralTexture->sampleBumpTexture(m_position)[0];
        noiseCoef[1] = proceduralTexture->sampleBumpTexture(Point(m_position.y,m_position.z,m_position.x))[0];
        noiseCoef[2] = proceduralTexture->sampleBumpTexture(Point(m_position.z,m_position.x,m_position.y))[0];
        */

        float4 noiseCoef;

        // take samples to approximate the gradient
        // vector of our noise function at (x, y, z)
        noiseCoef[0] = proceduralTexture->sampleBumpTexture(m_position)[0];
        noiseCoef[1] = proceduralTexture->sampleBumpTexture(Point(m_position.x+EPSILON,m_position.y,m_position.z))[0];
        noiseCoef[2] = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y+EPSILON,m_position.z))[0];
        noiseCoef[3] = proceduralTexture->sampleBumpTexture(Point(m_position.x,m_position.y,m_position.z+EPSILON))[0];

        // approc. gradient vector
        Vector gradient;
        gradient[0] = (noiseCoef[1]-noiseCoef[0]) / EPSILON;
        gradient[1] = (noiseCoef[2]-noiseCoef[0]) / EPSILON;
        gradient[2] = (noiseCoef[3]-noiseCoef[0]) / EPSILON;

        float temp = gradient*gradient;

        if (fabsf(temp) > 0.000001f) // shouldn't be 0
		{
            return ~(m_normal-(bumpIntensity*gradient));
		} else {
            return m_normal;
		}
    }

    _IMPLEMENT_CLONE(ProceduralHardBumpShader);
};


/*
// a bump shader which generates bump texture procedurally
class ProceduralRefractionBumpShader : public RefractivePhongShader, public ProceduralBumpShader
{
public:
    //float bumpIntensity;
    float frequency;
    float lift;


    virtual void getCoeff(float4 &_diffuseCoef, float4 &_specularCoef, float &_specularExponent) const
      {  // Function returns generated diffuse coefficient and given specular parameters
        float noiseCoefx =
        float(PerlinNoise::noise(frequency* double(m_position.x),
                                frequency * double(m_position.y),
                                frequency * double(m_position.z)));

       // diffuseCoeff = float4::rep(PerlinNoise::noise(m_position.x, m_position.y, m_position.z)) * lightColor + float4::rep(swirlDistance) * darkColor;

            DefaultPhongShader::getCoeff(_diffuseCoef, _specularCoef, _specularExponent);
        //std::cout << noiseCoefx << std::endl;

            _diffuseCoef = float4::rep(noiseCoefx+lift) ;//float4(noiseCoefx,noiseCoefy,noiseCoefz,1);

      }



	virtual Vector getNormal() const
	{
	    return m_normal;
        Vector noiseCoef;

        // surrounding noise
        noiseCoef[0] = PerlinNoise::noise(  frequency * m_position.x,
                                            frequency * m_position.y,
                                            frequency * m_position.z);

        noiseCoef[1] = PerlinNoise::noise(  frequency * m_position.y,
                                            frequency * m_position.z,
                                            frequency * m_position.x);

        noiseCoef[2] = PerlinNoise::noise(  frequency * m_position.z,
                                            frequency * m_position.x,
                                            frequency * m_position.y);

        // "lift" noise up
        for (int i=0; i<2; i++)
        {
            noiseCoef[i] += lift;
        }

        // calculate pertubed normal
        Vector vNormal = m_normal;
        vNormal[0] = (1.0f - bumpIntensity )
                  * vNormal[0] + bumpIntensity * noiseCoef[0];
        vNormal[1] = (1.0f - bumpIntensity )
                  * vNormal[1] + bumpIntensity * noiseCoef[1];
        vNormal[2] = (1.0f - bumpIntensity )
                  * vNormal[2] + bumpIntensity * noiseCoef[2];

        float temp = vNormal * vNormal;

        //if (temp != 0)
        if (fabsf(temp) > 0.000001f) // shouldn't be 0
		{
            return ~vNormal;
		} else {
            return m_normal;
		}

        //std::cout << "old: " << m_normal[0] << "," << m_normal[1] <<","<< m_normal[2] << " new: " << vNormal[0] << "," << vNormal[1] << "," << vNormal[2] << std::endl;
	}

    _IMPLEMENT_CLONE(ProceduralRefractionBumpShader);
};
*/

#endif //__INCLUDE_GUARD_810F2AF5_7E81_4F1E_AA05_992B6D2C0016
