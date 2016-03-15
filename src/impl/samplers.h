#ifndef __INCLUDE_GUARD_EA5235C2_ADC9_40B5_9859_473C44497D3A
#define __INCLUDE_GUARD_EA5235C2_ADC9_40B5_9859_473C44497D3A
#ifdef _MSC_VER
	#pragma once
#endif


#include "../rt/renderer.h"

//The default sampler which samples a pixel with a ray through it's center
struct DefaultSampler : public Sampler
{
	virtual void getSamples(uint _x, uint _y, std::vector<Sample> &_result)
	{
		Sample s;
		s.position = float2(0.5f, 0.5f);
		s.weight = 1.f;
		_result.push_back(s);
	}

};


struct RegularSampler : public Sampler
{
	uint samplesX, samplesY;

	virtual void getSamples(uint _x, uint _y, std::vector<Sample> &_result)
	{
		for(uint x = 0; x < samplesX; x++)
			for(uint y = 0; y < samplesY; y++)
			{
				float2 pos = float2((float)(x), (float)(y));
				pos = (pos + float2(0.5, 0.5)) / float2((float)samplesX, (float)samplesY);
				Sample s;
				s.position = pos;
				s.weight = 1.f / (float)(samplesX * samplesY);
				_result.push_back(s);
			}
	}
};

struct RandomSampler : public Sampler
{
	uint sampleCount;
	virtual void getSamples(uint _x, uint _y, std::vector<Sample> &_result)
	{
		for(uint i = 0; i < sampleCount; i++)
		{
			Sample s;
			s.position.x = ((float)rand()) / (float)RAND_MAX;
			s.position.y = ((float)rand()) / (float)RAND_MAX;
			s.weight = 1.f / (float)sampleCount;
			_result.push_back(s);
		}
	}
};

struct StratifiedSampler : public Sampler
{
	uint samplesX, samplesY;
	virtual void getSamples(uint _x, uint _y, std::vector<Sample> &_result)
	{
		for(uint x = 0; x < samplesX; x++)
			for(uint y = 0; y < samplesY; y++)
			{
				float2 offset = float2(((float)rand()) / (float)RAND_MAX, ((float)rand()) / (float)RAND_MAX);

				float2 pos = float2((float)(x), (float)(y));
				pos = (pos + offset) / float2((float)samplesX, (float)samplesY);

				Sample s;
				s.position = pos;
				s.weight = 1.f / (float)(samplesX * samplesY);

				_result.push_back(s);
			}
	}
};


class HaltonSampleGenerator : public Sampler
{
	float inverseRadical(uint _num, uint _radix)
	{
		float ret = 0;
		float curRadix = (float)_radix;

		while(_num != 0)
		{
			ret += (float)(_num % _radix) / curRadix;
			_num /= _radix;
			curRadix *= (float)_radix;
		}

		return ret;

	}

public:

	size_t sampleCount;

	virtual void getSamples(uint _x, uint _y, std::vector<Sample> &_result)
	{
		static _THREAD_LOCAL uint cur = 0;

		for(size_t i = 0; i < sampleCount; i++)
		{
			Sample s;
			s.position.x = inverseRadical(cur, 2);
			s.position.y = inverseRadical(cur++, 3);
			s.weight = 1 / (float)sampleCount;
			_result.push_back(s);
		}
	}
};

#endif //__INCLUDE_GUARD_EA5235C2_ADC9_40B5_9859_473C44497D3A
