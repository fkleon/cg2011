#include "../core/memory.h"
#include "basic_definitions.h"

// a kd-tree implementation
// which uses surface area heuristics to
// determine split point
// Implementation inspired by
//      - Pharr, Humphreys: Physically Based Rendering, 2nd Edition, Chapter 4
// and  - Wald, Havran:     On building fast kd-Trees for Ray Tracing, and on doing that in O(N log N)
class KDTree : public BVHStruct
{
protected:

   // a kd tree node
	struct KDNode : public Node
	{
		int splitPlane, numPrims;
		float splitValue;

        // if a leaf is initialized with no primitives
        // in the corresponding leafDataIndex should be NULL
        void initLeaf(int _numPrims, size_t leafDataIndex, BBox primBBox) {
            const size_t NODE_TYPE_MASK = ((size_t)1 << KDNode::LEAF_FLAG_BIT); // set leaf flag
            dataIndex = leafDataIndex | NODE_TYPE_MASK; // save leaf data
            numPrims = _numPrims;
            bbox = primBBox;
        }

        void initInterior(int axis, float split) {
            splitPlane = axis;
            splitValue = split;
        }

        int getSplitAxis() const {
            return splitPlane;
        }

        int getNumPrims() const {
            return numPrims;
        }

        float getSplitValue() const {
            return splitValue;
        }
	};

	// to evaluate SAH without centroids,
	// we have to sort the primitives by
	// their BBox edge values on the split dimnension
	// AND the position of the triangles themselves,
	// e.g. left, planar or right
	struct SortHelper
	{
	    float compValue; // value of BBox edge
	    int position; // left, planar, right
	};

    // helper to get comparators for sorting SortHelper's on a given axis
    class PrimitiveSortFunction
    {
    public:
        PrimitiveSortFunction() {}

        bool operator()(const SortHelper &_sh1, const SortHelper &_sh2) const
        {
            // first compare values, and if values are equal,
            // compare positions
            return (_sh1.compValue < _sh2.compValue ? true : _sh1.compValue == _sh2.compValue ? _sh1.position < _sh2.position : false);
        }
    };

    // helper structure to build the tree
    struct BuildStateStruct
	{
        std::vector<size_t>* primitives; //the index of the triangles in the volume are stored
		//size_t segmentStart, segmentEnd;
		size_t nodeIndex;
		//BBox centroidBBox;
		BBox objectBBox;    // we need object bbox of parent node to solve SAH equation
		int recursionDepth; // current recursion depth
		int numPrims;       // number of primitives in this node
	};

	// helper structure to traverse the tree
	struct KdToDo
	{
        int nodeIndex;
        float tNear, tFar;
	};

private:

    // triangle intersection cost, BBox traversal cost, empty bonus
    float t_tri, t_aabb, e_bonus;
    // maximum depth, maximum number of primitives in a leaf
    int maxDepth, maxPrims;

    // middle split or SAH?
    bool useSAH;

    // building state, object bboxes and centroids
	BuildStateStruct curState;
    std::vector<BBox> objectBBoxes;
    std::vector<bvh_build_internal::CentroidWithID> centroids;

    // tree nodes and leaf data
    std::vector<KDNode> m_nodes;
	std::vector<Primitive*> m_leafData;

    void init(float _t_tri, float _t_aabb, float _e_bonus, int _maxPrims, int _maxDepth)
    {
        t_tri = _t_tri;
        t_aabb = _t_aabb;
        e_bonus = _e_bonus;
        maxPrims = _maxPrims;
        maxDepth = _maxDepth;
    }

    // this method will evaluate the SAH and find the best split plane
    void findSplitAxis(float &_bestCost, int &_bestAxis, int &_bestEvent, float &_splitValue, std::vector<size_t>* _primitives, const float totalNodeArea) const;
    // SAH evaluation of parameters
    float evaluateSAH(float leftArea, float rightArea, float totalNodeArea, int leftObjCnt, int rightObjCnt) const;

    // sort vector of primitive helpers
    void sortOnAxis(std::vector<SortHelper> _primitives) const
    {
        std::sort(_primitives.begin(), _primitives.end(), PrimitiveSortFunction());
    }

public:

    KDTree() {
        init(10.f, 1.f, .8f, 4, 100);
        useSAH = true;
    }

    KDTree(float _t_tri, float _t_aabb, float _e_bonus, int _maxPrims, int _maxDepth, bool _useSAH)
    {
        init(_t_tri, _t_aabb, _e_bonus, _maxPrims, _maxDepth);
        useSAH = _useSAH;
    }

    virtual void build(const std::vector<Primitive*> &_objects);

	//Intersects a ray with the BVH.
	virtual IntersectionReturn intersect(const Ray &_ray, float _previousBestDistance) const;

	virtual BBox getSceneBBox() const
	{
	    // return BBox of root node
	    return m_nodes[0].bbox;
    };
};

// a balanced kd-tree implementation,
// splitting on the median
// used for storing our photon map
class KDPhotonTree
{
   public:

	//Builds the hierarchy over a set of bounded primitives
	void build(const std::vector<Point*> &_points);

	//Intersects a ray with the BVH.
	float4 lookup(const Point &_point, float _distance) const;
};
