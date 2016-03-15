#include "stdafx.h"
#include "bvh.h"
#include <algorithm>
#include "kdtree.h"

#define _EPS 0.0000001f
#define MIN_DISTANCE 0.000001f

using namespace bvh_build_internal;

/*
* #0.5
*/

void KDTree::build(const std::vector<Primitive*> &_objects)
{
    const clock_t begin_time = clock(); // for building time measurement for debug output
    int numLeafs = 0;
    int numFaces = 0;

    // find a sufficient depth if no depth is given
    // (according to Physically Based Rendering, 2nd Edition)
    if (maxDepth <=0) {
        maxDepth = (8+1.3f*log2(_objects.size()));
        std::cout << "(KD-Tree) max depth: " << maxDepth << std::endl;
        //maxDepth = 90;
    }

    objectBBoxes.resize(_objects.size());  // vector for storing object BBoxes

	// ******* KD-tree ***************
	curState.recursionDepth = 0;                    // we start with recursion level 0
	curState.primitives = new std::vector<size_t>;  // reserve memory for our pointers
	// *******************************

    // initialize current state
	curState.nodeIndex = 0;                // we are root
    curState.objectBBox = BBox::empty();   // start current state with empty object bbox

    // build object BBoxes
	for(size_t i = 0; i < objectBBoxes.size(); i++)
	{
		objectBBoxes[i] = _objects[i]->getBBox();    // put bounding box of object in BBox vector
	    curState.objectBBox.extend(objectBBoxes[i]); // extend scene object box
	    curState.primitives->push_back(i);           // remember original index of primitive
	}

	m_nodes.resize(1); // initialize nodes list

	std::stack<BuildStateStruct> buildStack;

	for(;;) // stop when build stack is empty
	{
	    size_t objCnt = curState.primitives->size();        // number of objects in current segment
		Vector boxDiag = curState.objectBBox.diagonal();    // diagonal of node bounding box

        // build internal node and continue recursion
        // initialize SAH variables
        float bestCost = objCnt * t_tri; // initialized with number of triangles * cost to intersect one
        int bestAxis = -1;
        int bestEvent = -1;
        float splitVal = -1;

        float totalNodeArea = curState.objectBBox.area(); // total area of parent node

        if (useSAH) {
            // use SAH to determine split axis and value
            findSplitAxis(bestCost, bestAxis, bestEvent, splitVal, curState.primitives, totalNodeArea);
        } else {
            // this is a simple middle split with rotating axis
            int axis = curState.recursionDepth % 3;
            int val = curState.objectBBox.getCentroid()[axis];

            splitVal = val;
            bestAxis = axis;
        }

        if (boxDiag[bestAxis] < _EPS)
            bestAxis = -1; // don't split too small nodes

        // debug
        //std::cout << "area: "<< totalNodeArea<< " bestDim: " << bestAxis << ", bestCost: " << bestCost << ", bestEvent: " << bestEvent << ", splitVal: " << splitVal << std::endl;

        // ********* BUILD LEAF **************** //
		if(bestAxis < 0 || objCnt < maxPrims || curState.recursionDepth > maxDepth) // if no optimal split axis was found, create a leaf here
		{
		    // debug
		    //std::cout << "building a leaf node with " << objCnt << " triangles." << std::endl;

            // initialize leaf
			m_nodes[curState.nodeIndex].initLeaf(objCnt, m_leafData.size(), BBox::empty());

			for(size_t i = 0; i < curState.primitives->size(); i++)
			{
			    size_t origIndex = (*curState.primitives)[i];

			    // extend BBox with all objects
				m_nodes[curState.nodeIndex].bbox.extend(objectBBoxes[origIndex]);
				// put object in leaf
				m_leafData.push_back(_objects[origIndex]);
			}

			delete curState.primitives; // free memory

			m_leafData.push_back(NULL); // NULL pointer means leaf end

            // some statistics never hurt
            numFaces+=objCnt;
            numLeafs++;

			if(buildStack.empty())
				break;

			curState = buildStack.top(); // load new state..
			buildStack.pop(); // .. and delete it from stack

			continue;
		}

        // ******************** KD-tree **************
        // If no stop criteria is met, continue recursive tree building
        // *******************************************

        // create child nodes
		BuildStateStruct rightState; // new state for right child

        // initialize BBoxes
        BBox nodeBBox = BBox::empty();
		curState.objectBBox = BBox::empty(); // object bbox left
		rightState.objectBBox = BBox::empty(); // object bbox right

		// initialize primitive pointer vectors
        rightState.primitives = new std::vector<size_t>;
		std::vector<size_t>* leftPrims = new std::vector<size_t>;

        // put primitives where they belong
        for (int cur=0; cur<curState.primitives->size(); cur++)
        {
            size_t origIndex = (*curState.primitives)[cur];


            if (objectBBoxes[origIndex].min[bestAxis] == splitVal && objectBBoxes[origIndex].max[bestAxis] == splitVal)
            {
                // we found a planar triangle!
                leftPrims->push_back(origIndex);
                curState.objectBBox.extend(objectBBoxes[origIndex]);
                curState.numPrims++;
                continue;
            }


            if (objectBBoxes[origIndex].min[bestAxis] <= splitVal)
            {
                // planar and left-handed primitives
                leftPrims->push_back(origIndex);
                curState.objectBBox.extend(objectBBoxes[origIndex]);
				nodeBBox.extend(objectBBoxes[origIndex]);
				curState.numPrims++;
            }

            if (objectBBoxes[origIndex].min[bestAxis] > splitVal)
            {
                // right-handed primitives
				rightState.primitives->push_back(origIndex);
				rightState.objectBBox.extend(objectBBoxes[origIndex]);
				nodeBBox.extend(objectBBoxes[origIndex]);
				rightState.numPrims++;
            }
        }

//        std::cout << "left prims has: " << leftPrims->size() << std::endl;
//        std::cout << "right prims has: " << rightState.primitives->size() << std::endl;

        delete curState.primitives; // free memory of old primitives
        curState.primitives = leftPrims;

        //std::cout << "continue tree building (1). objCnt: " << objCnt << ", left: " << curState.numPrims << ", right:" << rightState.numPrims << std::endl;

        //store parent node stuff

        // set bbox of current node
		m_nodes[curState.nodeIndex].bbox = nodeBBox;
		// set number of node
		m_nodes[curState.nodeIndex].dataIndex = m_nodes.size();


        // ****************** KD-tree **********************************
		// save split value, split axis and number of primitives in node
		m_nodes[curState.nodeIndex].initInterior(bestAxis, splitVal);
		m_nodes[curState.nodeIndex].numPrims = objCnt;

		curState.nodeIndex = m_nodes.size(); // store current state at end of m_nodes
		rightState.nodeIndex = curState.nodeIndex + 1; // store next node on position after last one

        // ****************** KD-tree **********************************
        // increase recursion depth
        ++curState.recursionDepth;
        rightState.recursionDepth = curState.recursionDepth;
        // *************************************************************

		buildStack.push(rightState); // save right-handed state to build later

		m_nodes.resize(rightState.nodeIndex + 1); // resize nodes by 1 again
	}

    // debug
    std::cout << "Total no. triangles: " << _objects.size() << " Total no. faces: " << numFaces <<
    std::endl << "Time needed to build SAH KD-Tree: " << float(clock()-begin_time)/CLOCKS_PER_SEC << " s. (" << numLeafs << " leafs)" << std::endl;
}

// determines best split axis, mininum cost and split value
// by evaluating the SAH for a given vector of centroids.
// uses Wald and Havran's O(n*log^2(n)) algorithm
// described in "On building fast kd-Trees for Ray Tracing, and on doing that in O(N log N)"
void KDTree::findSplitAxis(float &_bestCost, int &_bestAxis, int &_bestEvent, float &_splitValue, std::vector<size_t>* _primitives, const float totalNodeArea) const
{
    int objCnt = _primitives->size();

    std::vector<SortHelper> sortEvents(1); // store all events here and sort on this vector

    int nl, np; // number of primitives left of spit plane, planar to split plane
    int nr = objCnt; // and right of split plane (initially all primitives)

    // for all axis do
    for (int dim = 0; dim < 3 ; dim++)
        {
            nl = np = 0; // init variables
            nr = objCnt;

            sortEvents.clear(); // clear event list

            // dont evaluate SAH and build leaf instead, if termination criteria is met
            if (objCnt <= maxPrims || curState.recursionDepth >= maxDepth){
                //std::cout << "Skipping SAH, building leaf. ObjCnt: " << objCnt << ", recursion depth: " << curState.recursionDepth << ", max depth: " << maxDepth << "." << std::endl;
                break;
            }

            BBox nodeBox = BBox::empty();

            // build sort Events and sort helpers
            for (int i=0; i<objCnt ; i++)
            {
                // lookup object BBox
                BBox primBox = objectBBoxes[(*_primitives)[i]];
                // determine minima and maxima
                float min = primBox.min[dim];
                float max = primBox.max[dim];

                nodeBox.extend(primBox); // extend node bbox

                SortHelper sh;
                sh.compValue = min;

                // now we have to take care of planar triangles
                if (min == max) {
                    sh.position = 1; // planar
                    sortEvents.push_back(sh);
                } else {
                    sh.position = 0; // left
                    sortEvents.push_back(sh);

                    SortHelper sh2;
                    sh2.compValue = max;
                    sh2.position = 2; // right
                    sortEvents.push_back(sh2);
                }
            }

            // finally sort the list
            sortOnAxis(sortEvents);

            // counting values
            int p_plus, p_minus, p_plan;
            float pos; // current candidate's splitting value

            // count events
            for (int i = 0; i < sortEvents.size();)
            {
                p_plus = p_minus = p_plan = 0;
                pos = sortEvents[i].compValue;

                while(i < sortEvents.size() && sortEvents[i].compValue == pos && sortEvents[i].position == 0) {
                    ++p_minus; ++i;
                }
                while(i < sortEvents.size() && sortEvents[i].compValue == pos && sortEvents[i].position == 1) {
                    ++p_plan; ++i;
                }
                while(i < sortEvents.size() && sortEvents[i].compValue == pos && sortEvents[i].position == 2) {
                    ++p_plus; ++i;
                }

                // move plane onto p
                // and evaluate SAH
                np = p_plan; // number of planar primitives
                nr -= p_plan; // all - planar
                nr -= p_minus; // (all-planar)-left = number of primitives right to plance

                // split bounding box in left and right parts
                BBox left = BBox::empty();
                left.min = Point(nodeBox.min);
                left.max = Point(nodeBox.max);
                left.max[dim] = pos;

                BBox right = BBox::empty();
                right.max = Point(nodeBox.max);
                right.min = Point(nodeBox.min);
                right.min[dim] = pos;

                // evaluate SAH
                float cost = evaluateSAH(left.area(), right.area(), totalNodeArea, nl, nr);

                //std::cout << "cost after evaluating: " << cost << std::endl;

                // cehck if we were lucky
                if (cost < _bestCost) {
                    // hooray, a better split position has been discovered!
                    _bestCost = cost;
                    _bestAxis = dim;
                    _splitValue = pos;
                }

                // prepare variables for next iteration
                nl += p_plus;
                nl += p_plan;
                np = 0;
            }
        }
}

float KDTree::evaluateSAH(float leftArea, float rightArea, float totalNodeArea, int leftObjCnt, int rightObjCnt) const
{
    //std::cout << "evaluating cost!! LA: " << leftArea << " RA: " << rightArea << " totalA: " << totalNodeArea << " leftObjCnt: "
    //<< leftObjCnt << " rightObjCnt: "<< rightObjCnt << std::endl
    //<< "t_tri: " << t_tri << " t_aabb: " << t_aabb << " ebonus: " << e_bonus << std::endl;

    // evaluate cost function
    float cost  = (2.0f*t_aabb)                                          // 2*T_aabb
                + (leftArea / totalNodeArea) * (leftObjCnt) * t_tri      // leftarea/totalarea * count objects in left area * T_tri
                + (rightArea / totalNodeArea) * (rightObjCnt) * t_tri;   // rightarea/totalarea * count objects in right area * T_tri

    if (leftObjCnt == 0 || rightObjCnt == 0)
    // bonus for empty partition
        cost *= e_bonus;

    return cost;
}


// inspired by Chapter 4.5.3 of Physically Based Rendering, 2nd Edition
BVHStruct::IntersectionReturn KDTree::intersect(const Ray &_ray, float _previousBestDistance) const
{
    //std::cout << "************ KD TREE TRAVERSAL ***************" << std::endl;
    // init hit information
    Primitive::IntRet bestHit;
	bestHit.distance = _previousBestDistance;

	Primitive *bestPrimitive = NULL;

    // init return object
    IntersectionReturn ret;
    ret.ret = bestHit;
    ret.primitive = bestPrimitive;

    // check intersection with scene
    std::pair<float,float> sceneIntersection = getSceneBBox().intersect(_ray);

    if (sceneIntersection.first > sceneIntersection.second) {
        // ray misses BBox,
        // return immediately
        return ret;
    }

    float tMin = sceneIntersection.first;
    float tMax = sceneIntersection.second;

    /*
    std::cout << "Scene BBox: MIN: " << getSceneBBox().min[0] << ", " << getSceneBBox().min[1]  << ", " << getSceneBBox().min[2] << std::endl;
    std::cout << "Scene BBox: MAX: " << getSceneBBox().max[0] << ", " << getSceneBBox().max[1]  << ", " << getSceneBBox().max[2] << std::endl;
    std::cout << "tMax: " << tMax << ", tMin: " << tMin << std::endl;
    */

    // precalculate inverse direction of ray
    Vector invDir = Vector(1.f/_ray.d[0], 1.f/_ray.d[1], 1.f/_ray.d[2]);

    // build current traversal element
    KdToDo curNode;
	curNode.nodeIndex = 0;
	curNode.tNear = tMin;
	curNode.tFar = tMax;

    // our traversal stack
	std::stack<KdToDo> traverseStack;

    // .. initialized
	//size_t curNode = 0;
	//size_t closestFace = (size_t)-1;
    //const float _EPS = 0.000001f;

	for(;;) // only leave if traversal stack is empty
	{
	    // check if we already hit a closer primitive,
	    // possibly in an earlier node
	    if (_previousBestDistance < tMin)
            break;

        // if not, go on and get next node to proceed
		const KDNode& node = m_nodes[curNode.nodeIndex];

		// check if node is a leaf
        if (!node.isLeaf()) {
            //std::cout << "processing internal node.. " << curNode.nodeIndex << std::endl;
            // proceed with internal node

            // indices of children
            int firstChild, secondChild;

            // on which axis and value was split?
            int splitAxis = node.getSplitAxis();
            float splitVal = node.getSplitValue();

            // parametric distance to split plane
            float tSplit = (splitVal-_ray.o[splitAxis]) * invDir[splitAxis];

            // check if left child needs to be traversed first,
            // by comparing the position of the ray's origin and the ray's direction to the split value
            int leftFirst = (_ray.o[splitAxis] < splitVal ||
                             _ray.o[splitAxis] == splitVal && _ray.d[splitAxis] >= 0);

			if(leftFirst) {
			    firstChild = node.getLeftChildOrLeaf();
			    secondChild = node.getLeftChildOrLeaf()+1;
			    //curNode.nodeIndex = node.getLeftChildOrLeaf();
			} else {
                firstChild = node.getLeftChildOrLeaf()+1;
                secondChild = node.getLeftChildOrLeaf();
			}

            //int numPrimsLeft = m_nodes[firstChild].getNumPrims();
            //std::cout << "firstChild: " << firstChild << ": " << numPrimsLeft << ", secondChild: " << secondChild << std::endl;
            //std::cout << "ray.o: " << _ray.o[0] <<  "," << _ray.o[1] << "," << _ray.o[2] << std::endl;
            //std::cout << "ray.d: " << _ray.d[0] <<  "," << _ray.d[1] << "," << _ray.d[2] << std::endl;
            //std::cout << "splitPlane: " << splitAxis <<  ", splitVal: " << splitVal << ", tSplit: " << tSplit << std::endl;
            //abort();
            /*
            if (curNode.tNear-tSplit > _EPS) {
                curNode.nodeIndex = secondChild;
                continue;
            } else if (tSplit-curNode.tFar > _EPS) {
                curNode.nodeIndex = firstChild;
            }
            */

			// now check if one of the childs is going to be missed by the ray
            if (tSplit > curNode.tFar || tSplit <= 0) {
                // traverse only first (near child)
                curNode.nodeIndex = firstChild;
/*
                if (bestHit.distance > tSplit) {
                    // could be a planar triangle,
                    // test far node as well!
                    KdToDo trav;
                    trav.nodeIndex = secondChild;
                    trav.tFar = curNode.tFar;
                    trav.tNear = tSplit;

                    traverseStack.push(trav);
                }
*/

                continue;
            } else if (tSplit < curNode.tNear) {
                // traverse only second (far child)
                curNode.nodeIndex = secondChild;
                continue;
            }

            else {
                // traverse both

                // create & push far child on stack
                KdToDo trav;
                trav.nodeIndex = secondChild;
                trav.tFar = curNode.tFar;
                trav.tNear = tSplit;

                traverseStack.push(trav);

                // traverse near child
                curNode.nodeIndex = firstChild;
                curNode.tFar = tSplit;
            }
            // done processing internal node
        } else {
            // we have a leaf,
            // get pointer to start of leaf data
            size_t idx = node.getLeftChildOrLeaf();

            // intersect with all primtives of leaf
			while(m_leafData[idx] != NULL)
			{
				Primitive::IntRet curRet = m_leafData[idx]->intersect(_ray, bestHit.distance);

				if(curRet.distance > Primitive::INTEPS() && curRet.distance < bestHit.distance)
				{
					bestHit = curRet;
					bestPrimitive = m_leafData[idx];
				}
                // ... and to the next primitive!
				idx++;
			}

			// bail out if traverse stack is empty
			if(traverseStack.empty())
				break;

            // .. if not get next node
			curNode = traverseStack.top();
			traverseStack.pop();
        }
	}

    // update information
	ret.ret = bestHit;
	ret.primitive = bestPrimitive;

	// .. and return!
	return ret;
}
