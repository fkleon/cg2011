#ifndef __INCLUDE_GUARD_6A0F8987_914D_41B8_8E51_53B29CF1A045
#define __INCLUDE_GUARD_6A0F8987_914D_41B8_8E51_53B29CF1A045
#ifdef _MSC_VER
	#pragma once
#endif


#include "../core/image.h"
#include "../impl/perlin.h"
#include <iostream>
#include <algorithm>
#include "../impl/random.h"

//Specifies where the center of the texel is.
//Currently the value 0.5 means that (0.5, 0.5)
//	in normalized texture coordinates will correspond
//	to the center of the the pixel (0,0) in the image
//	This is quite visible with bilinear filtering, since
//	texel (1, 1) will actually be
//	1/4 (pixel(0, 0) + pixel(0, 1) + pixel(1, 0) + pixel(1, 1)),
//	whereas texel(0.5, 0.5) will be = pixel(0, 0) of the imagge
#define _TEXEL_CENTER_OFFS 0.5f

//A texture class
class Texture : public RefCntBase
{

public:
	//What to do if the texture coordinates are outside
	//	of the texture
	enum TextureAddressMode
	{
		TAM_Wrap, //Wrap around 0. Results in a repeated texture
		TAM_Border, //Clamp the coordinate to the border.
		TAM_Repeat	//Results in the border pixels repeated
	};

	//The texture filtering mode. It affects magnifaction only
	enum TextureFilterMode
	{
		TFM_Point,
		TFM_Bilinear
	};

    // determines texture type
	enum TextureType
	{
	    TT_HateMap, // height map, i.e. for bump mapping or displacement mapping
	    TT_NormalMap, // normal map, i.e. for bump/discplacement map
	    TT_Texture // normal texture
	};

	SmartPtr<Image> image;

	TextureAddressMode addressModeX, addressModeY;
	TextureFilterMode filterMode;
	TextureType textureType;

	Texture()
	{
		addressModeX = TAM_Wrap;
		addressModeY = TAM_Wrap;
		filterMode = TFM_Point;
		textureType = TT_Texture;
	}

	//Sample the texture. Coordinates are normalized:
	//	(0, 0) corresponds to pixel (0, 0) in the image and
	//	(1, 1) - to pixel (width - 1, height - 1) of the image,
	//	if doing point sampling
	float4 sample(const float2& _pos) const
	{
		//Denormalize the texture coordinates and offset the center
		//	of the texel
		float2 pos =
			_pos * float2((float)image->width(), (float)image->height())
			+ float2(_TEXEL_CENTER_OFFS, _TEXEL_CENTER_OFFS);

        if (textureType == TT_Texture) {
            return sampleDenormalized(pos);
        } else {
            // not supported here
            return float4::rep(0);
        }
	}

    // samples all four neighbors of the point to
    // create a normal vector out of the variance in height difference
    // taking normalized coordinates
	Vector sampleBumpTexture(const float2& _pos) const {
	    //Denormalize the texture coordinates and offset the center
		//	of the texel
		float2 pos =
			_pos * float2((float)image->width(), (float)image->height())
			+ float2(_TEXEL_CENTER_OFFS, _TEXEL_CENTER_OFFS);

        if (textureType == TT_HateMap) {
                // if we are sampling a bump texture,
                // sample all the neighbors
                float neighbor1x = pos.x-1.f;
                float neighbor2x = pos.x+1.f;
                float neighbor3y = pos.y-1.f;
                float neighbor4y = pos.y+1.f;

                // get correct neighbor coordinates according to texture address mode
                fixAddress(neighbor1x,(float)image->width(),addressModeX);
                fixAddress(neighbor2x,(float)image->width(),addressModeX);
                fixAddress(neighbor3y,(float)image->height(),addressModeY);
                fixAddress(neighbor4y,(float)image->height(),addressModeY);

                // sample the points
                float4 bumpNeighbor1 = sampleDenormalized(float2(neighbor1x,pos.y));
                float4 bumpNeighbor2 = sampleDenormalized(float2(neighbor2x,pos.y));
                float4 bumpNeighbor3 = sampleDenormalized(float2(pos.x,neighbor3y));
                float4 bumpNeighbor4 = sampleDenormalized(float2(pos.x,neighbor4y));

                // compute variation on x and y axis
                float du = bumpNeighbor2.x - bumpNeighbor1.x;
                float dv = bumpNeighbor4.x - bumpNeighbor3.x;

                Vector t = Vector(1.f,0,du);
                Vector b = Vector(0,1.f,dv);

                // normal of height map at this point
                Vector heightNormal =  ~(t%b);

                return heightNormal;
        } else if (textureType == TT_NormalMap) {
            // TODO do funky stuff
            return Vector(0,0,0);
        } else {
            // not supported
            return Vector(0,0,0);
        }
	}

private:
    // taking denormalized coordinates
    float4 sampleDenormalized(float2 pos) const
    {
        if(filterMode == TFM_Point)
			return lookupTexel(pos.x, pos.y);
		else if(filterMode == TFM_Bilinear)
		{
			float x_lo = floor(pos.x);
			float y_lo = floor(pos.y);
			float x_hi = ceil(pos.x);
			float y_hi = ceil(pos.y);

			float4 pix[2][2];
			pix[0][0] = lookupTexel(x_lo, y_lo);
			pix[1][0] = lookupTexel(x_hi, y_lo);
			pix[0][1] = lookupTexel(x_lo, y_hi);
			pix[1][1] = lookupTexel(x_hi, y_hi);

			float4 xhw = float4::rep(pos.x - x_lo);
			float4 yhw = float4::rep(pos.y - y_lo);
			float4 xlw = float4::rep(1 - xhw.x);
			float4 ylw = float4::rep(1 - yhw.x);

			return
				ylw * (xlw * pix[0][0] + xhw * pix[1][0]) +
				yhw * (xlw * pix[0][1] + xhw * pix[1][1]);
		}
		else
			return float4::rep(0.f);
    }


	//Correct the sampling address to be inside the texture
	static void fixAddress(float &_addr, float _max, TextureAddressMode _tam)
	{
		if(_tam == TAM_Wrap) {
			_addr = fmodf(_addr, _max);
			if (_addr < 0) // hack because fmodf sucks
                _addr+=_max;
		}
		else if(_tam == TAM_Border)
			_addr = std::min(std::max(_addr, 0.f), _max);
        else if(_tam == TAM_Repeat)
            _addr = _addr - floorf(_addr/ _max) * _max;
	}

	//Lookup a texel using point sampling. Coordinates are
	//	denormalized and texel center is at (0, 0) of image
	//	pixel's center
	float4 lookupTexel(float _x, float _y) const
	{
		float realX = _x, realY = _y;
		fixAddress(realX, (float)image->width(), addressModeX);
		fixAddress(realY, (float)image->height(), addressModeY);

		uint x = (uint)floor(realX);
		uint y = (uint)floor(realY);

		return (*image)(x, y);
	}

};


// texture using a noise function to generate
// actual texture and bump map
struct ProceduralTexture : public RefCntBase
{
protected:
    float4 color1;
    float4 color2;
public:
    std::string name; // name of the texture
    float bumpIntensity; // should be in interval [0,1], a bump intensity of 0 will disable bump mapping

    ProceduralTexture() : bumpIntensity(0.3f)
    {
        color1 = float4::rep(1.f);
        color2 = float4::rep(0.f);
        name = "DefaultProceduralTexture";
    }

    ProceduralTexture(float4 _color1, float4 _color2, float _bumpIntensity)
    : color1(_color1), color2(_color2), bumpIntensity(_bumpIntensity)
    {
        name = "DefaultProceduralTexture";
    }

    /*
    virtual float4 sampleTexture(const Point& _pos) const { return float4::rep(0.f); }
    virtual float4 sampleBumpTexture(const Point &_pos) const { return float4::rep(0.f); }
    */

    float4 sampleTexture(const Point& _pos) const {
        return getTexel(_pos, color1, color2);
    }

    // returns inverted texture in black/white
	virtual float4 sampleBumpTexture(const Point& _pos) const {
	    float4 black = float4::rep(0);
	    float4 white = float4::rep(1);
        return getTexel(_pos, white, black);
	}

    // set main colors of texture
	virtual void setColor(float4 _color1, float4 _color2) {
        color1 = _color1;
        color2 = _color2;
	}

private:

    virtual float4 getTexel(const Point& _pos, float4 _color1, float4 _color2) const {
        return float4::rep(0.f);
    }
};


// a procedural wood texture
class ProceduralWoodTexture : public ProceduralTexture
{
public:
    float scale;

    ProceduralWoodTexture() : ProceduralTexture()
    {
        scale = 1.f;
        name = "ProceduralWoodDefault";
    }

    ProceduralWoodTexture(float4 _color1, float4 _color2, float _scale, float _bumpIntensity)
    : ProceduralTexture(_color1, _color2, _bumpIntensity), scale(_scale) {
        name = "ProceduralWood";
    }

private:
    float4 getTexel(const Point& _pos, float4 _color1, float4 _color2) const
    {
        /*
        float noise = 0;
	    float k = 2;

	    for (int i = frequency ; i > 0 ; i--)
	    {
	        	    noise += sinf(i*PerlinNoise::noise(_x, _y, _z))/k;
	        	    k*=2;
	    }
	    return float4::rep(noise);
        */

        float fac = .25f;           // factor of turbulence function
        float gap = scale * 10.0f;  // gap between "rings" of wood

        // distribution of noise
        float dist = sqrt(_pos.x * _pos.x + _pos.y * _pos.y) + fac * turbulence(_pos.x,_pos.y,_pos.z);
        dist *= gap;
        dist -= floor(dist); // reduce distance to small interval

        // get some different noise for wood
        float f = 2 * ((1 - dist * dist) / 5 + 0.3 - 0.2 * PerlinNoise::noise(10*dist, 9*dist, 11*dist));

        // texel color will contains both colors of out texture
        float4 texelColor =  float4::rep(f) * _color1 + float4::rep(1 - f) * _color2;

        return texelColor;
    }


    float turbulence(float x, float y, float z) const
    {
        // wooden turbulence
        float t = 0.f;
        int j = 2;
        for (int i = 1; i < 6; i++) {
            t += PerlinNoise::noise(i * x, i * y, i * z) / j;
            j *= 2;
        }
        return t;
    }

};

class ProceduralWaterTexture : public ProceduralTexture
{
public:
   ProceduralWaterTexture() : ProceduralTexture()
    {
        frequency = 0.5f;
        name = "ProceduralWaterDefault";
    }

    ProceduralWaterTexture(float _frequency, float _bumpIntensity) : ProceduralTexture()
    {
        frequency = _frequency;
        bumpIntensity = _bumpIntensity;
        name = "ProceduralWaterDefault";
    }


private:
    float frequency;

    float4 getTexel(const Point& _pos, float4 _color1, float4 _color2) const
    {
        return float4::rep(stripes(PerlinNoise::noise(_pos.x,_pos.y,_pos.z),frequency));
        //return float4::rep((cos(PerlinNoise::noise(_pos.x,_pos.y,_pos.z))));
    }

    // stripe function
    float stripes(float x, float f) const
    {
        float t = .5 + .5 * sinf(f * 2 * M_PI * x);
        return t * t - .5;
    }
};

// a 3d texture for marble
class ProceduralMarbleTexture : public ProceduralTexture
{
public:
    float frequency;
    int width;

    ProceduralMarbleTexture() : ProceduralTexture()
    {
        frequency = 1.6f;
        width = 512;
        name = "ProceduralMarbleDefault";
    }

    ProceduralMarbleTexture(float4 _color1, float4 _color2, float _frequency, float _bumpIntensity)
    : ProceduralTexture(_color1, _color2, _bumpIntensity), frequency(_frequency) {
        name = "ProceduralMarble";
        width = 512;
    }

private:
    float4 getTexel(const Point& _pos, float4 _color1, float4 _color2) const
    {
        /*
        float marb = abs(cos(6*( _pos.x + PerlinNoise::noise(_pos.x,_pos.y,_pos.z)) ));
        if (marb < 0.3)
            return _color2*float4::rep(marb);
        else
            return _color1*float4::rep(marb);
        */

        //.03 * noise(x, y, z, 8); //LUMPY
        float marble = (stripes(_pos.x + 2 * turbulence(_pos.x, _pos.y, _pos.z, 1), frequency));  //MARBLED
        //-.10 * turbulence(x, y, z, 1);                       //CRINKLED
        //std::cout << "posix: " << _pos.x << " marble turb: " << turbulence(_pos.x, _pos.y, _pos.z, 1) << std::endl;
        return _color1*float4::rep(marble) +  _color2*float4::rep(1-marble);
    }

    // stripe function
    float stripes(float x, float f) const
    {
        float t = .5 + .5 * sinf(f * 2 * M_PI * x);
        return t * t - .5;
    }

    // turbulence function
    float turbulence(float x, float y, float z, float f) const
    {
        float t = -.5;
        //int i = 0;
        //for (int i = 0 ; i < 10 ; i++)m
        for ( ; f <= width/12 ; f *= 2) {
            t += abs(PerlinNoise::noise(f*x,f*y,f*z) / f);
            //t += abs(PerlinNoise::noise(++i*x,i*y,i*z) / f);
        }
        return t;
    }

    float abs(float x) const
    {
        return (x < 0 ? x*-1 : x);
    }
};

// a magnificient planet.
class ProceduralPlanetTexture : public ProceduralTexture
{
public:

    ProceduralPlanetTexture() : ProceduralTexture()
    {
        name = "ProceduralPlanetDefault";
        init();
    }

    ProceduralPlanetTexture(float4 _color1, float4 _color2, float _bumpIntensity)
    : ProceduralTexture(_color1, _color2, _bumpIntensity) {
        name = "ProceduralPlanet";
        init();
    }

private:
    std::vector<float4> landColors;

    // initialize land colors
    void init() {
        float4 landColor1(.5f, .39f, .2f,0);
        float4 landColor2(.2f, .3f, 0, 0);
        float4 landColor3(.065f, .22f, 0.04f, 0);
        //float4 landColor4(.6f, .6f, .23f, 0);

        landColors.clear();
        landColors.push_back(landColor1);
        landColors.push_back(landColor2);
        landColors.push_back(landColor3);
        //landColors.push_back(landColor4);
    }

    float4 getTexel(const Point& _pos, float4 _color1, float4 _color2) const
    {
        // some texture parameters
        float lacunarity = 0.95f;
       // float spectral_exp = 0.5f; // not used
        float filtwidth = 2.f;
        float octaves = 7;
        float sea_level = 0;
        float mtn_scale = 1;
        float depth_scale = 1;
        float depth_max = .5f;
        float mottle_scale = 20.f;
        //float mottle_dim = 0.25f; // not used
        float mottle_mag = 0.02f;
        float offset = 0;
        float bump_scale = bumpIntensity;

        // things to work with
        Point p2;
        float purt, chaos;
        float4 ct;

        float bumpy = fBm(_pos, filtwidth, octaves, lacunarity);

        // bump height
        chaos = bumpy + offset;

        // check if we are on land or ocean
        if (chaos > sea_level) {
            // land
            chaos *= mtn_scale;
            float bumpiness = bump_scale * bumpy;
            p2 = _pos + Vector(bumpiness,bumpiness,bumpiness); // * normalize(N);
            purt = fBm(p2, mottle_scale*filtwidth, 6, 2); //, mottle_dim);

            // colorize
            float random = Random::getRandomFloat(0,3);
            int rand = (int)random;
            ct = landColors[rand];

            // mottle
            ct += float4::rep(mottle_mag*purt)*float4(0.5f,0.175f,0.5f,0);
        } else {
            //p2 = _pos;
            // oceans
            ct = float4(.12f, .3f, .6f, 0);
            chaos -= sea_level;
            chaos *= depth_scale;
            chaos = std::max(chaos, -depth_max);
            ct *= float4::rep(1+chaos);
        }

        return ct;
    }

    // stripe function
    float stripes(float x, float f) const
    {
        float t = .5 + .5 * sinf(f * 2 * M_PI * x);
        return t * t - .5;
    }

    // turbulence function
    float turbulence(float x, float y, float z, float f) const
    {
        float t = -.5;
        int width = 512;

        for ( ; f <= width/12 ; f *= 2) {
            t += abs(PerlinNoise::noise(f*x,f*y,f*z) / f);
        }
        return t;
    }

    float abs(float x) const
    {
        return (x < 0 ? x*-1 : x);
    }

    /*
    * Procedural fBm at a given Point.
    *
    * Possible parameters:
    * "H" is the fractal increment parameter
    * "lacunarity" is the gap between successive frequencies
    * "octaves" is the number of frequencies in the fBm
    *
    * Also see "Texturing and Modeling - A Procedural Approach"
    * by Ebert, Musgrave, Peachey, Perlin & Worley
    */
    float fBm(Point point, float H, float lacunarity, float octaves) const
    {
        float value, remainder;//, Noise();
        int i;

        value = 0.0;

        /* inner loop of fractal construction */
        for (i=0; i<octaves; i++) {
            value += PerlinNoise::noise(point.x,point.y,point.z) * pow(lacunarity, -H*i);
            point.x *= lacunarity;
            point.y *= lacunarity;
            point.z *= lacunarity;
        }

        remainder = octaves - (int)octaves;

        if (remainder) /* add in "octaves" remainder */
            /* i and spatial freq. are preset in loop above */
            value += remainder * PerlinNoise::noise(point.x,point.y,point.z) * pow(lacunarity, -H*i);

        return value;
    }

    // box function
    int box(float x)
    {
        return (x<0.5)?0:1;
    }

};

#endif //__INCLUDE_GUARD_6A0F8987_914D_41B8_8E51_53B29CF1A045
