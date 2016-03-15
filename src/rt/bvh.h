#ifndef __INCLUDE_GUARD_8D5E74D9_FBD2_4B91_88E1_716ECFC377C4
#define __INCLUDE_GUARD_8D5E74D9_FBD2_4B91_88E1_716ECFC377C4
#ifdef _MSC_VER
	#pragma once
#endif

#include "../core/memory.h"
#include "basic_definitions.h"
#include <algorithm>


namespace bvh_build_internal
{
	struct BuildStateStruct
	{
		size_t segmentStart, segmentEnd;
		size_t nodeIndex;
		BBox centroidBBox;
		BBox objectBBox; // we need object bbox of parent node to solve SAH equation
	};

	struct CentroidWithID
	{
		Point centroid;
		size_t origIndex;
		float leftArea; // stores area to left side of this centroid
		float rightArea; // stores area to right side of this centroid (including itself)
	};
}

// helper to get comparators for sorting CentroidWithID's on a given axis
class CentroidSortFunction
{
private:
    int axis;
public:
    CentroidSortFunction(int _axis) : axis(_axis) {}

    bool operator()(const bvh_build_internal::CentroidWithID &_centroid1, const bvh_build_internal::CentroidWithID &_centroid2) const
    {
        return _centroid1.centroid[axis] < _centroid2.centroid[axis];
    }
};

// bas structure for bounding volume hierarchies
struct BVHStruct
{
protected:

    // a bvh node
	struct Node
	{
	    // bit to determine if the node is a leaf node
		enum {LEAF_FLAG_BIT = sizeof(size_t) * 8 - 1};

        // bbox of primitives in this node
		BBox bbox;

		// pointer to children (internal node)
		// pointer to first primitive (leaf node)
		size_t dataIndex;

        // checks, if node is a leaf
		bool isLeaf() const
		{
			const size_t NODE_TYPE_MASK = ((size_t)1 << LEAF_FLAG_BIT);
			return (dataIndex & NODE_TYPE_MASK) != 0;
		}

        // returns pointer to left child (internal node)
        // or pointer to first primitive (leaf node)
		size_t getLeftChildOrLeaf() const
		{
			const size_t INDEX_MASK = ((size_t)1 << LEAF_FLAG_BIT) - 1;
			return dataIndex & INDEX_MASK;
		}
	};

    // all nodes
	std::vector<Node> m_nodes;
	// pointer to primitives in leafs
	std::vector<Primitive*> m_leafData;

public:
	struct IntersectionReturn
	{
		Primitive *primitive;
		Primitive::IntRet ret;
	};

    virtual void build(const std::vector<Primitive*> &_objects) = 0;

	//Intersects a ray with the BVH.
	virtual IntersectionReturn intersect(const Ray &_ray, float _previousBestDistance) const = 0;

	virtual BBox getSceneBBox() const { return BBox::empty(); };

	// sorts a vector of centroids on a given axis
    virtual void sortOnAxis(std::vector<bvh_build_internal::CentroidWithID> &_centroids, int axis)
    {
        std::sort(_centroids.begin(), _centroids.end(), CentroidSortFunction(axis));
    }
};

//A bounding volume hierarchy
class BVH : public BVHStruct
{
public:

	//Builds the hierarchy over a set of bounded primitives
	virtual void build(const std::vector<Primitive*> &_objects);

	//Intersects a ray with the BVH.
	virtual IntersectionReturn intersect(const Ray &_ray, float _previousBestDistance) const;

	virtual BBox getSceneBBox() const
	{
	    // return BBox of root node
	    return m_nodes[0].bbox;
    };
};

// a bounding volume hierarchy using Surface Area Heuristic to determine split point
class BVHSAH : public BVH
{
public:
    // overwrite build from default SAH
	virtual void build(const std::vector<Primitive*> &_objects);
};


#endif //__INCLUDE_GUARD_8D5E74D9_FBD2_4B91_88E1_716ECFC377C4
