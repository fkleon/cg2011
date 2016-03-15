#ifndef __INCLUDE_GUARD_8B7841BB_FFCF_42B9_BF24_2CD457380EAB
#define __INCLUDE_GUARD_8B7841BB_FFCF_42B9_BF24_2CD457380EAB
#ifdef _MSC_VER
	#pragma once
#endif

#include "impl/random.h"
#include "core/algebra.h"
#include "rt/basic_definitions.h"
#include <iostream>
// boost random uniform distribution
//#include <boost/random/linear_congruential.hpp>
//#include <boost/random/uniform_01.hpp>
// boost matrices
//#include <boost/numeric/ublas/matrix.hpp>
//#include <boost/numeric/ublas/io.hpp>

#define PI 3.1415926f

//A perspective camera implementation
class PerspectiveCamera : public Camera
{
protected:
	Point m_center;
	Vector m_forward, m_up, m_right;
	Vector m_topLeft;
	Vector m_stepX, m_stepY;

	int resX, resY;

	void init(const Point &_center, const Vector &_forward, const Vector &_up,
		float _vertOpeningAngInGrad, std::pair<uint, uint> _resolution)
	{
		m_center = _center;

		float aspect_ratio = (float)_resolution.first / (float)_resolution.second;
        resX = _resolution.first;
        resY = _resolution.second;

        m_forward = _forward;
		Vector forward_axis = ~_forward;
		Vector right_axis = ~(forward_axis % _up);
		Vector up_axis = -~(right_axis % forward_axis);
		m_up = up_axis;
		//m_forward = forward_axis;
		m_right = right_axis;

		float angleInRad = _vertOpeningAngInGrad * (float)M_PI / 180.f;
		Vector row_vector = 2.f * right_axis * tanf(angleInRad / 2.f) * aspect_ratio;
		Vector col_vector = 2.f * up_axis * tanf(angleInRad / 2.f);

		m_stepX = row_vector / (float)_resolution.first;
		m_stepY = col_vector / (float)_resolution.second;
		m_topLeft = forward_axis - row_vector / 2.f - col_vector / 2.f;
	}

public:
	PerspectiveCamera(const Point &_center, const Vector &_forward, const Vector &_up,
		float _vertOpeningAngInGrad, std::pair<uint, uint> _resolution)
	{
		init(_center, _forward, _up, _vertOpeningAngInGrad, _resolution);
	}

	PerspectiveCamera(const Point &_center, const Point &_lookAt, const Vector &_up,
		float _vertOpeningAngInGrad, std::pair<uint, uint> _resolution)
	{
		init(_center, _lookAt - _center, _up, _vertOpeningAngInGrad, _resolution);
	}

	virtual Ray getPrimaryRay(float _x, float _y)
	{
		Ray ret;
		ret.o = m_center;
		ret.d = m_topLeft + _x * m_stepX + _y * m_stepY; // was *4.f
		return ret;
	}

    virtual std::vector<Ray> getPrimaryRays(float _x, float _y)
	{
	    std::vector<Ray> rays;
		Ray ret = getPrimaryRay(_x, _y);
        rays.push_back(ret);
		return rays;
	}
};

// a simple orthographic camera.
// will send rays from the image plane collision point
// in direction of the forward vector.
// keep in mind to increase the angle if you want to see more!
class OrthographicCamera : public PerspectiveCamera
{
public:
	OrthographicCamera(const Point &_center, const Vector &_forward, const Vector &_up,
		float _vertOpeningAngInGrad, std::pair<uint, uint> _resolution)
		:PerspectiveCamera(_center, _forward, _up, _vertOpeningAngInGrad, _resolution)
	{
	}

	OrthographicCamera(const Point &_center, const Point &_lookAt, const Vector &_up,
		float _vertOpeningAngInGrad, std::pair<uint, uint> _resolution)
		:PerspectiveCamera(_center, _lookAt, _up, _vertOpeningAngInGrad, _resolution)
	{

	}

    virtual Ray getPrimaryRay(float _x, float _y)
	{
		Ray ret;
		ret.o = m_center + m_topLeft + _x * m_stepX + _y * m_stepY;
		ret.d = m_forward;
		return ret;
	}

    virtual std::vector<Ray> getPrimaryRays(float _x, float _y)
	{
	    std::vector<Ray> rays;
		Ray ret = getPrimaryRay(_x, _y);
        rays.push_back(ret);
		return rays;
	}
};

// A perspective camera implementation
// which simulates a lens according to the thin lens model
// To obtain a perfectly sharp image, lensDistance must be equal to focalLength
// Also an aperture of 0 will simulate an ordinary pin-hole camera.
class PerspectiveLensCamera : public PerspectiveCamera
{
    float focalLength;
    float lensAperture;
    float lensDistance;

    Vector m_lensCenter;
    Vector m_imageCenter;
    Point m_lookAt;

    std::vector<Point> samplePoints;

    int samples;


	void init(float _focalLength, float _lensAperture, float _lensDistance, int _samplePoints, bool _focusAtLookAt)
	{
        _ASSERT(_lensDistance >= _focalLength); // or else we will look backwards
        _ASSERT(_lensDistance != _focalLength); // or else we will devide by 0

	    std::cout<< "Initializing PerspectiveLensCamera with focal length: " << _focalLength << ", aperture: " << _lensAperture
	    << " and lens distance: " << _lensDistance << "! Samples: " << _samplePoints << std::endl
	    << "- Forcing focus at lookAt: " << _focusAtLookAt << std::endl;

        // calculate center of lens and center of image plane
		Vector m_imageCenter = m_topLeft + m_stepX*(float)resX*0.5f + m_stepY*(float)resY*0.5f;
        m_lensCenter = m_imageCenter + (~m_imageCenter * _lensDistance);

        /*
        std::cout <<"origin: " << m_center[0] << "," <<m_center[1] << "," <<m_center[2] <<std::endl;
        std::cout <<"origin to image: " << m_imageCenter[0] << "," <<m_imageCenter[1] << "," <<m_imageCenter[2] << " length: " << m_imageCenter.len() << std::endl;
        std::cout <<"origin to lens: " << m_lensCenter[0] << "," <<m_lensCenter[1] << "," <<m_lensCenter[2] << " length: " << m_lensCenter.len() << std::endl;
        */

        Point lensCenter = m_center+m_lensCenter;
        Point imageCenter = m_center+m_imageCenter;

        /*
        std::cout <<"camera middle: " << m_center[0] << "," <<m_center[1] << "," <<m_center[2] <<  std::endl;
        std::cout <<"image plane middle: " << imageCenter[0] << "," <<imageCenter[1] << "," <<imageCenter[2] <<  std::endl;
        std::cout <<"lens middle: " << lensCenter[0] << "," <<lensCenter[1] << "," <<lensCenter[2] << std::endl;
        */

        // set member variables
        if (_focusAtLookAt) {
		    // calculate new focalLength
            float focalDistance = (m_lookAt - lensCenter).len();
            focalLength = (_lensDistance*focalDistance)/(_lensDistance+focalDistance);
            std::cout << "(focus at lookAt point) adjusted focal length from " << _focalLength << " to " << focalLength << std::endl;
		} else {
            focalLength = _focalLength;
		}

		lensAperture = _lensAperture;
		lensDistance = _lensDistance;
		samples = _samplePoints;

		// initialize random numbers
        //boost::minstd_rand intgen;
        //boost::uniform_01<boost::minstd_rand> gen(intgen);

        //genSamples();
	}

public:

	PerspectiveLensCamera(const Point &_center, const Vector &_forward, const Vector &_up,
		float _vertOpeningAngInGrad, std::pair<uint, uint> _resolution, float _focalLength,
		float _lensAperture, float _lensDistance, int _samplePoints)
		: PerspectiveCamera(_center, _forward, _up, _vertOpeningAngInGrad, _resolution)
	{
		init(_focalLength, _lensAperture, _lensDistance, _samplePoints, false);
	}


	PerspectiveLensCamera(const Point &_center, const Point &_lookAt, const Vector &_up,
		float _vertOpeningAngInGrad, std::pair<uint, uint> _resolution, float _focalLength,
		float _lensAperture, float _lensDistance, int _samplePoints, bool _focusAtLookAt)
        : PerspectiveCamera(_center, _lookAt, _up, _vertOpeningAngInGrad, _resolution)

	{
	    m_lookAt = _lookAt;
		init(_focalLength, _lensAperture, _lensDistance, _samplePoints, _focusAtLookAt);
	}

	virtual Ray getPrimaryRay(float _x, float _y)
	{
	    return getPrimaryRays(_x,_y)[0];
	}

	virtual std::vector<Ray> getPrimaryRays(float _x, float _y)
	{
	    std::vector<Ray> rays;

        if (lensAperture == 0) { // focus at infinity, everything sharp (lens radius = 0)
            // only one sample needed.
            samples = 1;
        }

        if (lensDistance == focalLength) { // shouldn't happen
            Ray r = PerspectiveLensCamera::getPrimaryRay(_x,_y);
            rays.push_back(r);
            return rays;
        }

	    // lens simulation will flip the image vertically and horizontally,
	    // therefore invert coordinates here to get a correct image afterwards
	    _x = resX-_x;
	    _y = resY-_y;

        // get "normal" primary ray to determine source point on image plane
	    Ray r = PerspectiveCamera::getPrimaryRay(_x, _y);

	    // source point on image plane
	    Point p = m_center + r.d;

        //std::cout << "lensDistance: " <<lensDistance << " , focalLength: " << focalLength << std::endl;

        // do crazy shit
        float o = (focalLength*lensDistance) /  (lensDistance-focalLength);
        Point lensCenter = m_center+m_lensCenter;
        Vector pc = Vector(lensCenter-p);
        float ratio = (pc.len()) / (lensDistance);

        Point q = lensCenter + o*ratio* (~pc);

        //std::cout <<"P: "<< p[0] << "," <<p[1] << "," <<p[2] <<std::endl;
        //std::cout <<"Q: "<< q[0] << "," <<q[1] << "," <<q[2] <<std::endl;
	    //Point q = Point(0, 1, -10);

        // generate new samples
        genSamples();
        //genSamples(lensAperture, rings, samples, false);

	    // distribution of rays
	    for (int i = 0; i < samples; i++) {
            Ray sampleRay;
            sampleRay.o = samplePoints[i];
            sampleRay.d = ~(q-samplePoints[i]);

            //std::cout <<"Created sample ray #" << i << ": "<< sampleRay.o[0] << "," <<sampleRay.o[1] << "," <<sampleRay.o[2]
            //<< " to: " << sampleRay.d[0] << "," <<sampleRay.d[1] << "," << sampleRay.d[2]  << std::endl;

            rays.push_back(sampleRay);
	    }

		return rays;
	}

private:

    // generates random sample points on the lens and saves
    // them in samplePoints.
	void genSamples()
	{
	    samplePoints.clear();
	    for(int i=0; i<samples; ++i)
        {
            // convert to polar coordinates
            // theta = 2*PI*rand1
            // r = aperture*sqrt(rand2)*0.5
            float rd = Random::getRandomFloat(0,1);
            float theta = 2*PI*rd; // theta angle in interval [0,360)
            float rd2 = Random::getRandomFloat(0,1);

            //std::cout << "heyho lets go " << lensAperture << std::endl;
            float r = 0.5f*lensAperture*sqrt(rd2); // random distance between 0 and radius of lens (0.5f*aperture)

            // convert polar coordinates to carthesian coodinates
            // use up vector as x-coordinate for polar coordinates
            float x = r * cos(theta);
            float y = r * sin(theta);

            //std::cout << "x: " << x << " and y: " << y << std::endl;

            Point samplePoint = m_center + (m_lensCenter + m_right * x + m_up * y);

            //std::cout <<"Created sample point #" << i << ": "<< samplePoint[0] << "," <<samplePoint[1] << "," <<samplePoint[2] <<std::endl;

            samplePoints.push_back(samplePoint);
        }
	}

    // generates sample points on lens
    // based on a n-edged form
    // for a nice lens effect
	void genSamples(float _aperture, int _pointsPerRing, int _numRings, bool _round)
	{
        samplePoints.clear();
        for (int i = 0 ; i < _numRings; i++) {

            float rd = Random::getRandomFloat(0,1);
            float theta = 2*PI*rd; // theta angle in interval [0,360)

            if (!_round && (i == (_numRings-1)))
            {
                theta = .5*(rd - .5); // FIXME: spiel mal mit nem konstanten Faktoren hier vielleicht..
            }

            float r = _aperture / _numRings * (i+1);
            float angle = 360 / _pointsPerRing;
            for(int j=0 ; j<_pointsPerRing ; j++)
            {
                float2 polar = float2(r-Random::getRandomFloat(0,1)*(_aperture / _numRings) * sqrt(Random::getRandomFloat(0,1)), theta + j*angle);
                float x = polar.x * cos(polar.y);
                float y = polar.x * sin(polar.y);
                Point samplePoint = m_center + (m_lensCenter + m_right * x + m_up * y);
                samplePoints.push_back(samplePoint);

            }

            //std::cout << "heyho lets go " << lensAperture << std::endl;
            //float r = 0.5f*lensAperture*sqrt(rd2); // random distance between 0 and radius of lens (0.5f*aperture)
	    }
	}
};




#endif //__INCLUDE_GUARD_8B7841BB_FFCF_42B9_BF24_2CD457380EAB
