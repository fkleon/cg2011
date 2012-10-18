#include "stdafx.h"
#include "bvh.h"

using namespace bvh_build_internal;

//An iterative split in the middle build for BVHs
void BVH::build(const std::vector<Primitive*> &_objects)
{
    const clock_t begin_time = clock(); // for building time measurement
    int numLeafs = 0;

	std::vector<BBox> objectBBoxes(_objects.size()); // vector for storing object BBoxes

	BuildStateStruct curState;
	curState.centroidBBox = BBox::empty(); // start current state with empty centroid box

	std::vector<CentroidWithID> centroids(objectBBoxes.size()); // vector for storing centroid BBoxes

    // build centroid BBox
	for(size_t i = 0; i < objectBBoxes.size(); i++)
	{
		objectBBoxes[i] = _objects[i]->getBBox(); // put boundin gbox of object #i in BBox vector

	    centroids[i].centroid = objectBBoxes[i].min.lerp(objectBBoxes[i].max, 0.5f); // get middle of BBox (centroid)
	    centroids[i].origIndex = i; // set index
	    curState.centroidBBox.extend(centroids[i].centroid); // extend centroid box
	}

    // initialize current state
	curState.segmentStart = 0;
	curState.segmentEnd = objectBBoxes.size();
	curState.nodeIndex = 0;
	m_nodes.resize(1); // initialize nodes list

	std::stack<BuildStateStruct> buildStack;

	const float _EPS = 0.0000001f;

	for(;;) // stop when build stack is empty
	{

	    size_t objCnt = curState.segmentEnd - curState.segmentStart; // number of objects in current segmnt
		Vector boxDiag = curState.centroidBBox.diagonal();

        // determine biggest value and split there
		int splitDim = boxDiag.x > boxDiag.y ? (boxDiag.x > boxDiag.z ? 0 : 2) : (boxDiag.y > boxDiag.z ? 1 : 2);

		if(fabs(boxDiag[splitDim]) < _EPS || objCnt < 3)
		{
            //std::cout << "build leaf with " << objCnt << " triangles." << std::endl;
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

			m_leafData.push_back(NULL); // NULL pointer means no object left

            numLeafs++; // count leafs

			if(buildStack.empty())
				break;

			curState = buildStack.top(); // load new state..
			buildStack.pop(); // .. and delete it from stack

			continue;
		}

        // determine where to split
		float splitVal = (curState.centroidBBox.min[splitDim] + curState.centroidBBox.max[splitDim]) / 2.f;

        // create child nodes

		size_t leftPtr = curState.segmentStart, rightPtr = curState.segmentEnd - 1; // save start and end of current segment

		BBox nodeBBox = BBox::empty(); // new BBox for parent
		BuildStateStruct rightState; // new state for right child
		curState.centroidBBox = BBox::empty(); // centroid bbox left
		rightState.centroidBBox = BBox::empty(); // centroid bbox right

		while(leftPtr < rightPtr)
		{
		    // if centroid left of split point
		    // put centroid in centroid box
		    // and object in node BBox of current state
			while(leftPtr < curState.segmentEnd && centroids[leftPtr].centroid[splitDim] <= splitVal)
			{
				curState.centroidBBox.extend(centroids[leftPtr].centroid);
				nodeBBox.extend(objectBBoxes[centroids[leftPtr].origIndex]);
				leftPtr++;
			}

            // if centroid right of split point
            // put centroid in new centroid box
            // and object in node BBox of right state
			while(rightPtr >= curState.segmentStart && centroids[rightPtr].centroid[splitDim] > splitVal)
			{
				rightState.centroidBBox.extend(centroids[rightPtr].centroid);
				nodeBBox.extend(objectBBoxes[centroids[rightPtr].origIndex]);
				rightPtr--;
			}

            // we are done
			if(leftPtr >= rightPtr)
				break;

            // swap to sort centroids
			std::swap(centroids[leftPtr], centroids[rightPtr]);
		}

		//float const time = 1.f;
		//std::cout << "node index: " << curState.nodeIndex <<", data index: "<< m_nodes.size() << ", no. triangles: " << curState.segmentEnd-curState.segmentStart << std::endl;

        //store stuff (new parent node?)

        // set bbox of current node
		m_nodes[curState.nodeIndex].bbox = nodeBBox;
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

		m_nodes.resize(rightState.nodeIndex + 1); // resize nodes by 1
	}

    std::cout << "Total no. triangles: " << _objects.size() <<
    std::endl << "Time needed to build BVH: " << float(clock()-begin_time)/CLOCKS_PER_SEC << " s. (" << numLeafs << " leafs)"<< std::endl;
    //std::cout << begin_time  << ","<<clock() <<", " << CLOCKS_PER_SEC  << ": " << (float(clock()-begin_time)/CLOCKS_PER_SEC)<<std::endl;

}

//Recursive intersection
BVHStruct::IntersectionReturn BVH::intersect(const Ray &_ray, float _previousBestDistance) const
{
	Primitive::IntRet bestHit;
	bestHit.distance = _previousBestDistance;

	Primitive *bestPrimitive = NULL;

	std::stack<size_t> traverseStack;

	size_t curNode = 0;
	size_t closestFace = (size_t)-1;

	for(;;)
	{
		const BVH::Node& node = m_nodes[curNode];
		if(node.isLeaf())
		{
			size_t idx = node.getLeftChildOrLeaf();
			while(m_leafData[idx] != NULL)
			{
				Primitive::IntRet curRet = m_leafData[idx]->intersect(_ray, bestHit.distance);

				if(curRet.distance > Primitive::INTEPS() && curRet.distance < bestHit.distance)
				{
					bestHit = curRet;
					bestPrimitive = m_leafData[idx];
				}

				idx++;
			}

			if(traverseStack.empty())
				break;

			curNode = traverseStack.top();
			traverseStack.pop();
		}
		else
		{
			const BVH::Node &leftNode = m_nodes[node.getLeftChildOrLeaf()];
			const BVH::Node &rightNode = m_nodes[node.getLeftChildOrLeaf() + 1];

			std::pair<float, float> intLeft = leftNode.bbox.intersect(_ray);
			std::pair<float, float> intRight = rightNode.bbox.intersect(_ray);
			intLeft.first = std::max(Primitive::INTEPS(), intLeft.first);
			intRight.first = std::max(Primitive::INTEPS(), intRight.first);
			intLeft.second = std::min(intLeft.second, bestHit.distance);
			intRight.second = std::min(intRight.second, bestHit.distance);

			bool descendLeft = intLeft.first < intLeft.second + Primitive::INTEPS();
			bool descendRight = intRight.first < intRight.second + Primitive::INTEPS();

			if(descendLeft && !descendRight)
				curNode = node.getLeftChildOrLeaf();
			else if(descendRight && !descendLeft)
				curNode = node.getLeftChildOrLeaf() + 1;
			else if(descendLeft && descendRight)
			{
				curNode = node.getLeftChildOrLeaf();
				size_t farNode = curNode + 1;
				if(intLeft.first > intRight.first)
					std::swap(curNode, farNode);
				traverseStack.push(farNode);
			}
			else
			{
				if(traverseStack.empty())
					break;

				curNode = traverseStack.top();
				traverseStack.pop();
			}
		}
	}

	IntersectionReturn ret;
	ret.ret = bestHit;
	ret.primitive = bestPrimitive;
	return ret;
}

