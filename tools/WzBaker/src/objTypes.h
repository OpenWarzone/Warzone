#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

#include <string>
#include <vector>
#include <map>

//! A generic vector
template<typename Type, unsigned int Dim>
class Vector
{
public:
	Type data[Dim];
	enum {Dimension=Dim};

public:
	Vector()
	{
		for(int i=0; i<Dim; i++)
		{
			data[i] = 0;
		}
	}

	Vector(Type* x)
	{
		for(int i=0; i<Dim; i++)
		{
			data[i] = x[i];
		}
	}

	void set(float x)
	{
		for(int i=0; i<Dim; i++)
		{
			data[i] = x;
		}
	}
};

typedef Vector<int, 3> Vector3i;   //!< 3D vector of ints
typedef Vector<float, 2> Vector2f; //!< 2D vector of floats
typedef Vector<float, 3> Vector3f; //!< 3D vector of floats
typedef Vector<float, 4> Vector4f; //!< 4D vector of floats


struct Material
{
	std::string textureAmbient;
	std::string textureDiffuse;
	std::string textureSpecular;
	std::string textureEmissive;
	std::string textureTransparency;
	std::string textureBump;
	Vector3f    colorAmbient;
	Vector3f    colorDiffuse;
	Vector3f    colorSpecular;
	Vector3f    colorEmissive;
	float       transparency;
	float       shininess;
	int         illuminationModel;
	Material()
	{
		illuminationModel = 2;
		colorAmbient.set(0.3f);
		colorDiffuse.set(0.7f);
		colorSpecular.set(0.4f);
		shininess = 10.0f;
		transparency = 1.0f;
	}
	void reset()
	{
		*this = Material();
	}
};

struct MeshComponent
{
	std::string           materialName;
	std::string           componentName;
	std::vector<Vector3i> faces;
	void reset()
	{
		componentName = "default";
		materialName = "";
		faces.clear();
	}
};

typedef std::map<std::string, Material> MaterialMapType;
typedef std::vector<MeshComponent>      ComponentListType;

struct Mesh
{
	ComponentListType          components;
	std::vector<Vector3f>      vertices;
	std::vector<Vector3f>      normals;
	std::vector<Vector2f>      texcoord;
	MaterialMapType            materials;
	void reset()
	{
		components.clear();
		vertices.clear();
	}
};

#endif
