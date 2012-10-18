#ifndef __INCLUDE_GUARD_3862487A_DF63_478D_99C2_652B7C66442E
#define __INCLUDE_GUARD_3862487A_DF63_478D_99C2_652B7C66442E
#ifdef _MSC_VER
	#pragma once
#endif

#include "../rt/basic_definitions.h"
#include "../rt/bvh.h"
#include "../rt/kdtree.h"
#include "shading_basics.h"
#include <typeinfo>

//A group of geometry. Since it is a primitive, it can also
//	contain other groups, thus creating a hierarchy.
class GeometryGroup : public Primitive
{
	//Intersection info which encapsulates the intersection info
	//	of a contained primitive, toghether with a reference to the primitive
	struct GGHitPoint : public RefCntBase
	{
		Primitive::IntRet intRet;
		Primitive* innerPrimitive;
	};

	//A BVH over the bounded primitives
	// we need a pointer here to be able to instantiate different BVH's
	BVHStruct* m_bvh;

	//The list of not bounded primitives
	std::vector<Primitive *> m_nonIdxPrimitives;

public:
	std::vector<Primitive *> primitives;
	bool indexCreated;

    // type determines bvh used
    // 0 - default BVH
    // 1 - SAH BVH
    // 2 - SAH KD Tree
    GeometryGroup(int type)
    {
        indexCreated = false;

        if (type == 0) {
            m_bvh = new BVH();
            std::cout << "Using acceleration structure: Default BVH." << std::endl;
        }
        if (type == 1) {
            m_bvh = new BVHSAH();
            std::cout << "Using acceleration structure: SAH BVH." << std::endl;
        }
        if (type == 2) {
            m_bvh = new KDTree();
            std::cout << "Using acceleration structure: SAH KD-tree." << std::endl;
        }
        if (type > 2 || type < 0) {
            m_bvh = new BVH();
            std::cout << "Invalid type. Using acceleration structure: Default BVH." << std::endl;
        }

        //std::cout << "BVH is: " <<  typeid(m_bvh).name() << std::endl;
    }

    // empty constructor, uses default BVH
    GeometryGroup()
    {
        GeometryGroup(0);
    }

    // kill the bvh
    ~GeometryGroup()
    {
        delete m_bvh;
    }

	virtual SmartPtr<Shader> getShader(IntRet _intData) const;
	virtual IntRet intersect(const Ray& _ray, float _previousBestDistance ) const;
	virtual BBox getBBox() const;

	//Rebuilds the BVH and updated m_nonIdxPrimitives
	void rebuildIndex();
};

#endif //__INCLUDE_GUARD_3862487A_DF63_478D_99C2_652B7C66442E
