#include "objTypes.h"

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/Exporter.hpp"	//OO version Header!
#include "assimp/importerdesc.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

bool packTextures(const Mesh& inputMesh, Mesh& outputMesh, const std::string& textureFilename);
