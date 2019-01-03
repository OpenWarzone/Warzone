#include "packer.h"

#include <set>
#include <list>
#include <map>
#include <algorithm>
#include <fstream>

#include "IL/il.h"
//#include "TinyImageLoader/TinyImageLoader.h"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>


class TextureTile;
class TextureTileQuad;
class TextureTileLeaf;
class TextureTileTree;
typedef std::shared_ptr<TextureTile> TextureTilePtr;
typedef std::weak_ptr<TextureTile> TextureTileWeakPtr;
typedef std::shared_ptr<TextureTileQuad> TextureTileQuadPtr;
typedef std::shared_ptr<TextureTileLeaf> TextureTileLeafPtr;

typedef std::list<TextureTilePtr> TextureTileListType;
typedef std::vector<TextureTilePtr> TextureTileArrayType;

// ------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------
class TextureTileTree
{
private:
	TextureTileListType mHeads;
public:
	size_t size() const { return mHeads.size(); }
	typedef TextureTileListType::iterator iterator;
	typedef TextureTileListType::const_iterator const_iterator;
	TextureTileListType::iterator begin() { return mHeads.begin(); }
	TextureTileListType::iterator end() { return mHeads.end(); }
	const TextureTilePtr& front() const { return mHeads.front(); }
public:
	TextureTilePtr getSmallestTile() const { return mHeads.front(); }
public:
	void getTileOffset(TextureTilePtr tile, int& resultX, int& resultY) const;
	TextureTileQuadPtr addQuad(const TextureTileArrayType& children);
	TextureTileLeafPtr addLeaf(const std::string& filename);
};

// ------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------
class TextureTile
{
protected:
	TextureTileWeakPtr mParent;
public:
	inline int getMaxSize() const { return std::max<int>(getSizeX(),getSizeY()); } 
	inline int area() const { return getSizeX()*getSizeY(); }
	virtual int getSizeX() const = 0;
	virtual int getSizeY() const = 0;
public:
	virtual bool getTileOffset(TextureTilePtr tile, int offsetX, int offsetY, int& resultX, int& resultY) const = 0;

protected:
	friend TextureTilePtr;
	friend TextureTileTree;
	TextureTile()
	{
	}
public:
	virtual ~TextureTile(){};
};


bool operator<(const TextureTile& a, const TextureTile& b)
{
	return a.getMaxSize() < b.getMaxSize();
}

bool compareTiles(TextureTilePtr a, TextureTilePtr b)
{
	return *a < *b;
}

// ------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------
class TextureTileQuad: public TextureTile
{
public:	
	TextureTilePtr mChildren[4];
	static int offsetsX[4];
	static int offsetsY[4];
	int mSize;
	virtual int getSizeX() const { return mSize; }
	virtual int getSizeY() const { return mSize; }
public:
	virtual bool getTileOffset(TextureTilePtr tile, int offsetX, int offsetY, int& resultX, int& resultY) const
	{
		int halfSize = mSize / 2;
		bool result = false;
		for(int i=0; i<4; i++)
		{
			result = result || (mChildren[i]!=NULL && 
				mChildren[i]->getTileOffset(tile, offsetX+offsetsX[i]*halfSize, offsetY+offsetsY[i]*halfSize, resultX, resultY));
		}
		return result;
	}
public:
	friend class TextureTileTree;
	TextureTileQuad()
	{
		mSize = 0;
	}
};

int TextureTileQuad::offsetsX[4] = {0,1,0,1};
int TextureTileQuad::offsetsY[4] = {0,0,1,1};

// ------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------
class TextureTileLeaf: public TextureTile
{
public:
	int mExactWidth;
	int mExactHeight;
	int mSizeX;
	int mSizeY;
	ILuint mImage;
public:
	virtual int getSizeX() const { return mSizeX; }
	virtual int getSizeY() const { return mSizeY; }
	int getExactWidth() const { return mExactWidth; }
	int getExactHeight() const { return mExactHeight; }
	ILuint getImage() const { return mImage; }
public:
	virtual bool getTileOffset(TextureTilePtr tile, int offsetX, int offsetY, int& resultX, int& resultY) const
	{
		if (tile.get()==this)
		{
			resultX = offsetX;
			resultY = offsetY;
			return true;
		}
		else return false;
	}
private:
	void loadFromFile(const std::string& filename);
	friend class TextureTileTree;
	TextureTileLeaf()
	{
		mExactWidth = 0;
		mExactHeight = 0;
		mImage = 0;
		ilGenImages(1, &mImage);
	}
public:
	~TextureTileLeaf()
	{
		ilDeleteImages(1, &mImage);
	}
};

TextureTileQuadPtr TextureTileTree::addQuad(const TextureTileArrayType& children)
{
	TextureTileArrayType trimmedChildren(children);
	trimmedChildren.resize(4);

	// Insert empty quad
	TextureTileQuadPtr quad(new TextureTileQuad);
	mHeads.push_back(quad);

	// Move tiles from the list of heads to the local list of children
	int size = 0;
	for(int i=0; i<4; ++i)
	{
		quad->mChildren[i] = trimmedChildren[i];
		if (trimmedChildren[i] != NULL)
		{
			quad->mChildren[i]->mParent = TextureTileWeakPtr(quad);
			mHeads.remove(trimmedChildren[i]);
			size = std::max<int>(size, trimmedChildren[i]->getMaxSize());
		}
	}

	// Set the quad size
	quad->mSize = 2*size;

	mHeads.sort(compareTiles);
	return quad;
}

TextureTileLeafPtr TextureTileTree::addLeaf(const std::string& filename)
{
	TextureTileLeafPtr leaf(new TextureTileLeaf);
	mHeads.push_back(leaf);
	leaf->loadFromFile(filename);

	mHeads.sort(compareTiles);
	return leaf;
}

void TextureTileTree::getTileOffset(TextureTilePtr tile, int& resultX, int& resultY) const
{
	if(mHeads.size()!=1)
	{
		throw std::runtime_error("tree has more than one head");
	}
	if(!mHeads.front()->getTileOffset(tile, 0, 0, resultX, resultY))
	{
		throw std::runtime_error("the tile is not present in the tree");
	}
}

int getNextPoT(int i)
{
	int result = 1;
	while(result<i)
	{
		result *= 2;
	}
	return result;
}

template <typename T>
std::string toString(T arg)
{
	std::stringstream ss;
	ss << arg;
	return ss.str();
}

void TextureTileLeaf::loadFromFile(const std::string& filename)
{
	ilBindImage(mImage);
	std::wstring wFilename(filename.length()+1, 0);
	std::copy(filename.begin(), filename.end(), wFilename.begin());
	
	if(ilLoadImage(wFilename.c_str()) != IL_TRUE)
	{
		throw std::runtime_error("could not load texture file \"" + filename + "\". Error " + toString(ilGetError()) + ".");
	}

	if (ilGetError() != 0)
	{
		throw std::runtime_error("could not load texture file \"" + filename + "\". Error " + toString(ilGetError()) + ".");
	}

	if(ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE) != IL_TRUE)
	{
		throw std::runtime_error("could not convert texture to 32bit RGBA \"" + filename + "\"");
	}

	if (ilGetError() != 0)
	{
		throw std::runtime_error("could not convert texture file \"" + filename + "\". Error " + toString(ilGetError()) + ".");
	}

	mExactWidth = ilGetInteger(IL_IMAGE_WIDTH);
	mExactHeight = ilGetInteger(IL_IMAGE_HEIGHT);
	mSizeX = getNextPoT(mExactWidth);
	mSizeY = getNextPoT(mExactHeight);

	printf("Texture %s loaded ok. Size: %i x %i.\n", filename.c_str(), mExactWidth, mExactHeight);
}


void transformTexcoord(Vector2f& out, const Vector2f& in, float ax, float bx, float ay, float by)
{
	out.data[0] = ax*in.data[0] + bx;
	out.data[1] = ay*in.data[1] + by;
}

bool packTextures(const Mesh& inputMesh, Mesh& outputMesh, const std::string& textureFilename)
{
	ilInit();

	// Collect all actually used materials
	std::set<std::string> usedMaterialNames;
	for(ComponentListType::const_iterator ic=inputMesh.components.begin();ic!=inputMesh.components.end();++ic)
	{
		usedMaterialNames.insert(ic->materialName);
	}

	// Create a tree of all tiles
	TextureTileTree tileTree;

	// Create one leaf for each input material texture
	typedef std::map<std::string, TextureTileLeafPtr> MaterialTileMapType;
	MaterialTileMapType materialTiles;
	for(MaterialMapType::const_iterator im=inputMesh.materials.begin();im!=inputMesh.materials.end();++im)
	{
		const std::string name = im->first;
		const Material& mat = im->second;
		if (!mat.textureDiffuse.empty() && usedMaterialNames.find(name)!=usedMaterialNames.end())
		{
			TextureTileLeafPtr leaf = tileTree.addLeaf(mat.textureDiffuse);
			materialTiles[name] = leaf;
		}
	}

	if (tileTree.size() < 1)
	{
		printf("FAIL! Model has no materials?\n");
		return false;
	}

	// Combine tiles until only one image remains
	TextureTileArrayType candidateTiles;
	candidateTiles.reserve(8);
	while(tileTree.size()>1)
	{
		// Find candidates to combine
		TextureTilePtr smallestTile = tileTree.getSmallestTile();

		candidateTiles.clear();
		for(TextureTileTree::iterator it=tileTree.begin(); it!=tileTree.end(); ++it)
		{
			TextureTilePtr candidate = *it;
			if (smallestTile->getMaxSize() == candidate->getMaxSize())
			{
				candidateTiles.push_back(candidate);
			}
		}

		// Create a new tile
		tileTree.addQuad(candidateTiles);
	}

	int totalSizeX = tileTree.front()->getSizeX();
	int totalSizeY = tileTree.front()->getSizeY();

	// Copy data
	outputMesh.vertices = inputMesh.vertices;
	outputMesh.normals = inputMesh.normals;
	outputMesh.texcoord = inputMesh.texcoord;
	outputMesh.components.push_back(MeshComponent());
	MeshComponent& outputComponent = outputMesh.components.front();
	outputComponent.componentName = "default";
	outputComponent.materialName = "default";

	// Transform texture coordinates
	for(ComponentListType::const_iterator ic=inputMesh.components.begin();ic!=inputMesh.components.end();++ic)
	{
		std::string material = ic->materialName;
		size_t startFace = outputComponent.faces.size();
		outputComponent.faces.reserve(outputComponent.faces.size() + ic->faces.size());
		outputComponent.faces.insert(outputComponent.faces.end(), ic->faces.begin(), ic->faces.end());

		MaterialTileMapType::iterator it = materialTiles.find(material);
		if (it!=materialTiles.end())
		{
			const TextureTileLeafPtr& leaf = it->second;
			int tileSizeX = leaf->getSizeX();
			int tileSizeY = leaf->getSizeY();
			int offsetX, offsetY;
			tileTree.getTileOffset(leaf, offsetX, offsetY);
			float bx = offsetX / float(totalSizeX);
			float by = offsetY / float(totalSizeY);
			float ax = tileSizeX / float(totalSizeX);
			float ay = tileSizeY / float(totalSizeY);
			for(size_t f=0;f<ic->faces.size();++f)
			{
				Vector3i indices = ic->faces[f];
				for(int i=0; i<3; ++i)
				{
					transformTexcoord(outputMesh.texcoord[indices.data[i]], inputMesh.texcoord[indices.data[i]], ax,bx,ay,by);
				}
			}
		}
	}

	// Create the texture atlas
	ILuint atlasImage;
	ilGenImages(1, &atlasImage);
	ilBindImage(atlasImage);
	if(!ilTexImage(ILuint(totalSizeX), ILuint(totalSizeY), 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
	{
		throw std::runtime_error("could not create the output image");
		return false;
	}

	ilDisable(IL_BLIT_BLEND);

	// Stitch the texture atlas
	for(MaterialTileMapType::const_iterator im=materialTiles.begin();im!=materialTiles.end();++im)
	{
		const std::string name = im->first;
		const TextureTileLeafPtr& leaf = im->second;
		int tileSizeX = leaf->getSizeX();
		int tileSizeY = leaf->getSizeY();
		int offsetX, offsetY;
		tileTree.getTileOffset(leaf, offsetX, offsetY);

		if(!ilBlit(leaf->getImage(), offsetX, totalSizeY-(offsetY+tileSizeY), 0, 0, 0, 0, leaf->getExactWidth(), leaf->getExactHeight(), 1))
		{
			throw std::runtime_error("could not blit into the output image");
			return false;
		}
	}

	std::wstring wFilename;
	wFilename.clear();
	wFilename.resize(textureFilename.size()+1,0);
	std::copy(textureFilename.begin(), textureFilename.end(), wFilename.begin());

	ilSaveImage(wFilename.c_str());
	ilDeleteImages(1, &atlasImage);

	Material& mat = outputMesh.materials["default"];
	mat.textureDiffuse = textureFilename;

	return true;
}
