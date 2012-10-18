#include "stdafx.h"
#include "bvh.h"
#include <algorithm>

#define T_TRI 1.5f  // cost of computing a triangle intersection T_tri
#define T_AABB 3.0f // cost of test a ray and AABB for intersection T_aabb
#define MIN_DISTANCE 0.000001f  // minimum distance between optimal splitting point and first or last centroid of segment
                                // to avoid empty or too small boxes.

using namespace bvh_build_internal;

// SAH based splitting algorithm
// like desribed in "Ray Tracing Deformable Scenes using Bounding Volume Hierarchies"
// by Ingo Wald, Solomon Boulos, Peter Shirley
void BVHSAH::build(const std::vector<Primitive*> &_objects)
{
    const clock_t begin_time = clock(); // for building time measurement for debug output
    int numLeafs = 0; // count leafs for debug output

	std::vector<BBox> objectBBoxes(_objects.size()); // vector for storing object BBoxes

	BuildStateStruct curState;
	curState.centroidBBox = BBox::empty(); // start current state with empty centroid box
    curState.objectBBox = BBox::empty(); // start current state with empty object bbox

	std::vector<CentroidWithID> centroids(objectBBoxes.size()); // vector for storing centroid BBoxes

    // build centroid BBox
	for(size_t i = 0; i < objectBBoxes.size(); i++)
	{
		objectBBoxes[i] = _objects[i]->getBBox(); // put bounding box of object #i in BBox vector

	    centroids[i].centroid = objectBBoxes[i].min.lerp(objectBBoxes[i].max, 0.5f); // get middle of BBox (centroid)
	    centroids[i].origIndex = i; // set index
	    curState.centroidBBox.extend(centroids[i].centroid); // extend centroid box
	    curState.objectBBox.extend(objectBBoxes[i]); // extend object box
	}

    // initialize current state
	curState.segmentStart = 0;
	curState.segmentEnd = objectBBoxes.size();
	curState.nodeIndex = 0;
	m_nodes.resize(1); // initialize nodes list

	std::stack<BuildStateStruct> buildStack;

	const float _EPS = 0.0000001f;

    // vector for sorting stuff
    std::vector<CentroidWithID> sortedCentroids(objectBBoxes.size());

	for(;;) // stop when build stack is empty
	{
	    size_t objCnt = curState.segmentEnd - curState.segmentStart; // number of objects in current segmnt
		Vector boxDiag = curState.centroidBBox.diagonal();

        BBox areaBox = BBox::empty(); // to determine node surface area
        sortedCentroids.clear();        // clear
        sortedCentroids.resize(objCnt); // & resize our work horse vector

        // copy relevant centroid data
        for (size_t j = curState.segmentStart; j<curState.segmentEnd; j++) {
            sortedCentroids[j-curState.segmentStart].origIndex = centroids[j].origIndex;
            sortedCentroids[j-curState.segmentStart].centroid = centroids[j].centroid;
            areaBox.extend(objectBBoxes[sortedCentroids[j-curState.segmentStart].origIndex]); // extend our node bbox
        }

        // Use SAH to determine split axis
        // initialize variables
        float bestCost = objCnt * T_TRI; // initialized with number of triangles * cost to intersect one
        float bestAxis = -1;
        float bestEvent = -1;
        float splitVal = -1;

        float totalNodeArea = 0.f;

        if (curState.nodeIndex == 0) {
            // we are root node, just take scene BBox
            totalNodeArea = curState.objectBBox.area();
            //std::cout << "Current node area (1): " << totalNodeArea  << std::endl;
        } else {
            // take area of just generated BBox
            totalNodeArea = areaBox.area();
        }

        //std::cout << "Current node area (2): " << totalNodeArea << " (was " << curState.objectBBox.area() << ") "<< std::endl;

        //std::cout << "node index: " << curState.nodeIndex-1 << ", total area " << totalNodeArea << std::endl;

        // for all axis do
        for (int dim = 0; dim < 3 ; dim++)
        {
            //std::cout << boxDiag[dim] << " < dim , abs dim > " << fabs(boxDiag[dim]) << std::endl;
            //if (abs(boxDiag[dim])<= 0.00000001)
            //    continue;
            //std::cout << "cur seg start: " << curState.segmentStart << ", cur seg end: " << curState.segmentEnd
            //<< "objct: " << objCnt << " objects: " << _objects.size() << std::endl;

            // sort centroids on selected axis
            sortOnAxis(sortedCentroids, dim);

            // sweep from left
            BBox tempBox = BBox::empty();
            sortedCentroids[0].leftArea = FLT_MAX;
            for (int k = 1; k<objCnt; k++) {
                //std::cout << "dimension: " << i << ", value: " << sortedCentroids[k-1].centroid[i] << " orig index: " << sortedCentroids[k-1].origIndex << std::endl;
                tempBox.extend(objectBBoxes[sortedCentroids[k-1].origIndex]); // extend BBox with centroid to left
                sortedCentroids[k].leftArea = tempBox.area(); // store current size in centroid
            }

            // sweep from right
            tempBox = BBox::empty();
            for (int l = objCnt-1; l>=0; l--) {
                tempBox.extend(objectBBoxes[sortedCentroids[l].origIndex]); // extend BBox with centroid to the right
                sortedCentroids[l].rightArea = tempBox.area(); // store current size in centroid
                //std::cout << "right area " << tempBox.area() << std::endl;

                // evaluate cost function
                float cost  = (2.0f*T_AABB)                                                         // 2*T_aabb
                            + (sortedCentroids[l].leftArea / totalNodeArea) * (l) * T_TRI         // leftarea/totalarea * count objects in left area * T_tri
                            + (sortedCentroids[l].rightArea / totalNodeArea) * (objCnt-l) * T_TRI;// rightarea/totalarea * count objects in right area * T_tri

                if (cost < bestCost) {
                    // we found a new optimum
                    bestCost = cost; // new best costs
                    bestEvent = l; // new best event
                    bestAxis = dim; // current split dimension
                    splitVal = sortedCentroids[l].centroid[dim]; // split coordinates

                    // some constistency checks and stop-conditions:
                    // (1) if all centroid are on same value of axis, don't split anymore
                    if (sortedCentroids[0].centroid[dim] == sortedCentroids[bestEvent].centroid[dim] == sortedCentroids[objCnt-1].centroid[dim]) {
                        std::cout << "Don't split " << objCnt << " triangles!" << std::endl;
                        bestAxis = -1;
                    }

                    // (2) check if our split candidate has same value on axis as first centroid
                    if (sortedCentroids[bestEvent].centroid[dim] == sortedCentroids[0].centroid[dim]) {
                        // ...and if moving the split point would result in an empty/invalid box (running over right side)
                        if (sortedCentroids[bestEvent].centroid[dim] + MIN_DISTANCE >= sortedCentroids[objCnt-1].centroid[dim]) {
                            bestAxis = -1; // no split
                        }
                        // or if we can move the split point to the right without danger
                        else
                            splitVal = sortedCentroids[bestEvent].centroid[dim] + MIN_DISTANCE;
                    }

                    // (3) check if our split candidate has same value on axis as last centroid
                    if (sortedCentroids[bestEvent].centroid[dim] == sortedCentroids[objCnt-1].centroid[dim]) {
                        // ...and if moving the split point would result in an empty/invalid box (running over left side)
                        if (sortedCentroids[bestEvent].centroid[dim] - MIN_DISTANCE <= sortedCentroids[0].centroid[dim]) {
                            bestAxis = -1; // no split
                       }
                        else
                        // or if we can move the split point to the left without danger
                            splitVal = sortedCentroids[bestEvent].centroid[dim] - MIN_DISTANCE;
                    }
                }

            }

            // SAH done
        }

		if(bestAxis < 0) // if no optimal split axis was found, create a leaf here
		{
		    _ASSERT(objCnt > 0); // should never happen!

			//Create a leaf
			const size_t NODE_TYPE_MASK = ((size_t)1 << Node::LEAF_FLAG_BIT); // set leaf flag

			m_nodes[curState.nodeIndex].bbox = BBox::empty();
			m_nodes[curState.nodeIndex].dataIndex = m_leafData.size() | NODE_TYPE_MASK;

			for(size_t i = curState.segmentStart; i < curState.segmentEnd; i++)
			{
			    // extend BBox with all objects
				m_nodes[curState.nodeIndex].bbox.extend(objectBBoxes[centroids[i].origIndex]);
				// put object in leaf
				m_leafData.push_back(_objects[centroids[i].origIndex]);
			}

			m_leafData.push_back(NULL); // NULL pointer means leaf end

            numLeafs++;

			if(buildStack.empty())
				break;

			curState = buildStack.top(); // load new state..
			buildStack.pop(); // .. and delete it from stack

			continue;
		}

        // create child nodes
		size_t leftPtr = curState.segmentStart, rightPtr = curState.segmentEnd - 1; // save start and end of current segment

		BuildStateStruct rightState; // new state for right child
		curState.centroidBBox = BBox::empty(); // centroid bbox left
		rightState.centroidBBox = BBox::empty(); // centroid bbox right
		curState.objectBBox = BBox::empty(); // object bbox left
		rightState.objectBBox = BBox::empty(); // object bbox right

        // sort centroids in split dimension
        while(leftPtr < rightPtr)
		{
		    // if centroid left of split point
		    // put centroid in centroid box
		    // and object in node BBox of current state
			while(leftPtr < curState.segmentEnd && centroids[leftPtr].centroid[bestAxis] <= splitVal)
			{
				curState.centroidBBox.extend(centroids[leftPtr].centroid);
				curState.objectBBox.extend(objectBBoxes[centroids[leftPtr].origIndex]);
				leftPtr++;
			}

            // if centroid right of split point
            // put centroid in new centroid box
            // and object in node BBox of right state
			while(rightPtr >= curState.segmentStart && centroids[rightPtr].centroid[bestAxis] > splitVal)
			{
				rightState.centroidBBox.extend(centroids[rightPtr].centroid);
				rightState.objectBBox.extend(objectBBoxes[centroids[rightPtr].origIndex]);
				rightPtr--;
			}

            // we are done
			if(leftPtr >= rightPtr)
				break;

            // swap to sort centroids
			std::swap(centroids[leftPtr], centroids[rightPtr]);
		}


        //std::cout << "split dimension: " << bestAxis << " node index: " << curState.nodeIndex <<", data index: "<< m_nodes.size() << ", no. triangles: " << curState.segmentEnd-curState.segmentStart << ", bbox area: " << nodeBBox.area() << std::endl;
        //std::cout << "split value: " << splitVal << std::endl;
        //store stuff (new parent node?)

        // set bbox of current node
		m_nodes[curState.nodeIndex].bbox = areaBox;
		// set number of node
		m_nodes[curState.nodeIndex].dataIndex = m_nodes.size();

		_ASSERT(leftPtr > curState.segmentStart && leftPtr < curState.segmentEnd);
		_ASSERT(leftPtr == rightPtr + 1);

        // set boundings of new states
		rightState.segmentStart = leftPtr;
		rightState.segmentEnd = curState.segmentEnd;
		curState.segmentEnd = leftPtr;
		curState.nodeIndex = m_nodes.size(); // store current state at end of m_nodes

		// next node index
		rightState.nodeIndex = curState.nodeIndex + 1; // store next node on position after last one

		buildStack.push(rightState); // save right-handed state to build later

		m_nodes.resize(rightState.nodeIndex + 1); // resize nodes by 1 (actually 2)
	}

    // debug
    std::cout << "Total no. triangles: " << _objects.size() <<
    std::endl << "Time needed to build SAH BVH: " << float(clock()-begin_time)/CLOCKS_PER_SEC << " s. (" << numLeafs << " leafs)" << std::endl;
    //std::cout << begin_time  << ","<<clock() <<", " << CLOCKS_PER_SEC  << ": " << (float(clock()-begin_time)/CLOCKS_PER_SEC)<<std::endl;

}

