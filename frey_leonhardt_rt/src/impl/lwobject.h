#ifndef __INCLUDE_GUARD_1E951A11_730A_4924_8797_68629AD9ACE3
#define __INCLUDE_GUARD_1E951A11_730A_4924_8797_68629AD9ACE3
#ifdef _MSC_VER
	#pragma once
#endif

#include "../core/algebra.h"
#include "../rt/basic_definitions.h"
#include "../rt/bvh.h"
#include "../rt/shading_basics.h"
#include "../rt/texture.h"


//A LightWave3D object. It provides the functionality of 
//	an indexed triangle mesh. The primitive consists
//	of 5 buffers holding vertex positions, vertex normals,
//	vertex texture coordinates, face materials, and the faces
//	them selfs. Each face is a triangle and only contains indices
//	into the position, normal, texture coordinate, and material
//	buffers.
class LWObject
{
public:
	class Face;

private:
	//This is the structure that is filled from the intersection
	//	routine and is than passed to the getShader routine, in case
	//	the face is the closest to the origin of the ray.
	struct ExtHitPoint : RefCntBase
	{
		//The barycentric coordinate (in .x, .y, .z) + the distance (in .w)
		float4 intResult;
	};

public:

	//Represents a material
	struct Material
	{
		//The unique name of the material
		std::string name;
		//The diffuse color of the material
		float4 diffuseCoeff;
		SmartPtr<Texture> diffuseTexture;

		float4 specularCoeff;
		SmartPtr<Texture> specularTexture;

		float4 ambientCoeff;
		SmartPtr<Texture> ambientTexture;

		float bumpIntensity;
		SmartPtr<Texture> bumpTexture;

		float specularExp;

		//The shader attached to the material. It can be
		//	fully independent of the data stored in the material
		SmartPtr<PluggableShader> shader;

		Material(){}
		Material(std::string _name) : name(_name), diffuseCoeff(0, 0, 0, 0)
		{}
	};


	//A face definition (triangle)
	class Face : public Primitive
	{
		LWObject *m_lwObject;
	public:
		size_t material;
		size_t vert1, tex1, norm1;
		size_t vert2, tex2, norm2;
		size_t vert3, tex3, norm3;

		Face(LWObject * _obj) : m_lwObject(_obj) {}

		virtual IntRet intersect(const Ray& _ray, float _previousBestDistance ) const;

		virtual BBox getBBox() const;

	virtual SmartPtr<Shader> getShader(IntRet _intData) const;
	};

	typedef std::vector<Point> t_pointVector;
	typedef std::vector<Vector> t_vectVector;
	typedef std::vector<Face> t_faceVector;
	typedef std::vector<Material> t_materialVector;
	typedef std::vector<float2> t_texCoordVector;

	t_pointVector vertices;
	t_vectVector normals;
	t_faceVector faces;
	t_materialVector materials;
	t_texCoordVector texCoords;
	std::map<std::string, size_t> materialMap;

	//Reads the LightWave3D object from a file and creates default phong shaders
	//	for its materials
	void read(const std::string& aFileName, bool _createDefautShaders = true);

	//Adds all faces contained in this object to a scene
	void addReferencesToScene(std::vector<Primitive*> &_scene) const;

};


#endif //__INCLUDE_GUARD_1E951A11_730A_4924_8797_68629AD9ACE3
