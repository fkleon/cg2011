#include "stdafx.h"

#include "geometry_group.h"

SmartPtr<Shader> GeometryGroup::getShader(IntRet _intData) const
{
	//Dereference the intersection info and ask the contained primitive
	//	for the shader
	SmartPtr<GGHitPoint> hit = _intData.hitInfo;
	return hit->innerPrimitive->getShader(hit->intRet);
}

Primitive::IntRet GeometryGroup::intersect(const Ray& _ray, float _previousBestDistance) const
{
    IntRet bestRet;

    bestRet.distance = _previousBestDistance;

	Primitive *bestPrimitive = NULL;
	//Find closest primitive
	for(std::vector<Primitive*>::const_iterator it = m_nonIdxPrimitives.begin(); it != m_nonIdxPrimitives.end(); it++)
	{

		IntRet curRet = (*it)->intersect(_ray, bestRet.distance);

		if(curRet.distance < bestRet.distance && curRet.distance > Primitive::INTEPS())
		{
			bestRet = curRet;
			bestPrimitive = *it;
		}
	}

    // check if index was created before
    // avoids nasty errors
    if (!indexCreated)
    {
        std::cout << "No index has been created for this geometry group. Build index first before traversing indexable primitives!" << std::endl;
        return bestRet;
        //rebuildIndex();
        //std::cout << "Done. Start traversal.." < std::endl;
    }

	BVH::IntersectionReturn intRet = m_bvh->intersect(_ray, bestRet.distance);
	if(intRet.ret.distance < bestRet.distance)
	{
		bestPrimitive = intRet.primitive;
		bestRet = intRet.ret;
	}


	//Encapsulate the intersection info together with the primtive it belongs to
	SmartPtr<GGHitPoint> hp = new GGHitPoint;
	hp->intRet = bestRet;
	hp->innerPrimitive = bestPrimitive;
	bestRet.hitInfo = hp;

	volatile Primitive::IntRet pp = bestRet;

	return bestRet;
}

BBox GeometryGroup::getBBox() const
{
	if(m_nonIdxPrimitives.size() > 0)
		return BBox::empty();
	else
		return m_bvh->getSceneBBox();
}

void GeometryGroup::rebuildIndex()
{
	std::vector<Primitive*> indexPrimitives;

	m_nonIdxPrimitives.clear();
	//Separate the bounded and unbounded primitives
	for(std::vector<Primitive*>::const_iterator it = primitives.begin(); it != primitives.end(); it++)
	{
		BBox box = (*it)->getBBox();
		//Another way of saying !(box.max.x < box.min.x && box.max.y < box.min.y & box.max.z < box.min.z)
		if(((float4(box.max) < float4(box.min)).getMask() & 14) == 0)
			indexPrimitives.push_back(*it);
		else
			m_nonIdxPrimitives.push_back(*it);
	}

	m_bvh->build(indexPrimitives);
	indexCreated = true;
}
