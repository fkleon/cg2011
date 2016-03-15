#ifndef __INCLUDE_GUARD_0FCFB065_7895_41C4_97EF_626328169DD9
#define __INCLUDE_GUARD_0FCFB065_7895_41C4_97EF_626328169DD9
#ifdef _MSC_VER
	#pragma once
#endif

#include "defs.h"

//A generic state object
//Keys are types
//To use:
//	Define your own struct, which you will use as a key
//	Inside the struct, define the type of the value by typdefing t_data
//For example:
//	struct InRefractiveObjectKey { typedef bool t_data; };
//	...
//	o.state.value<InRefractiveObjectKey>() = false;
//	...
//	if(o.state.value<InRefractiveObjectKey>())
//	{
//		...
//	}
//Can be used for example to store the state of a ray in an
//	integrator, during recursive traversal in combination
//	with (for example) Shader::getIndirectRadiance.
//Use this to expose some state in an efficient way to the shaders 
//	(from the integrator). If you want to keep some state in the integrator
//	which should not be visible to the shaders, simply use
//	member variables in the class.
//Good examples are: depth of ray, contribution of this ray 
//	to the final radiance of the ray path, and data which
//	can help you do refractions (speed of light in the current
//	media for example).
//Note: Be carefull with multiple threads! Each thread takes some state
//	space which is later on not freed. If you create and destroy threads
//	frequently or if you create too many threads, you might run out of
//	state space!
class StateObject
{
	enum {_MAX_KEYS = 16384};

	void* m_stateData[_MAX_KEYS];

	static long getNextTypeIndex()
	{
		static long gFirstFreeIndex = 0;
		long ret;
#pragma omp critical
		{
			ret = gFirstFreeIndex++;
		}

		return ret;
	}
public:

	StateObject()
	{
		memset(m_stateData, 0, sizeof(m_stateData));
	}
		
	template <class KEY>
	typename KEY::t_data& value()
	{
		static _THREAD_LOCAL long gTypeIndex = -1;
		if(gTypeIndex == -1)
			gTypeIndex = getNextTypeIndex();

		void *ret = m_stateData[gTypeIndex];
		if(ret == NULL)
		{
			ret = new typename KEY::t_data;
			_ASSERT(gTypeIndex < _MAX_KEYS);
			m_stateData[gTypeIndex] = ret;
		}

		return *(typename KEY::t_data *)ret;
	}

};


#endif //__INCLUDE_GUARD_0FCFB065_7895_41C4_97EF_626328169DD9
