#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
//#include "assimp/Exporter.hpp"	//OO version Header!
#include "assimp/importerdesc.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

// Create an instance of the Importer class
Assimp::Importer assImpImporter;

// Create an instance of the Importer class
//Assimp::Exporter assImpExporter;

std::string AssImp_getBasePath(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(0, pos + 1);
}

std::string AssImp_getTextureName(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(pos + 1, std::string::npos);
}

#if defined(WIN32) || defined(WIN64)
bool textcolorprotect = true;
/*doesn't let textcolor be the same as backgroung color if true*/

inline void setcolor(concol textcolor, concol backcolor);
inline void setcolor(int textcolor, int backcolor);
int textcolor();/*returns current text color*/
int backcolor();/*returns current background color*/

#define std_con_out GetStdHandle(STD_OUTPUT_HANDLE)

				//-----------------------------------------------------------------------------

int textcolor()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(std_con_out, &csbi);
	int a = csbi.wAttributes;
	return a % 16;
}

int backcolor()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(std_con_out, &csbi);
	int a = csbi.wAttributes;
	return (a / 16) % 16;
}

inline void setcolor(concol textcol, concol backcol)
{
	setcolor(int(textcol), int(backcol));
}

inline void setcolor(int textcol, int backcol)
{
	if (textcolorprotect)
	{
		if ((textcol % 16) == (backcol % 16))textcol++;
	}
	textcol %= 16; backcol %= 16;
	unsigned short wAttributes = ((unsigned)backcol << 4) | (unsigned)textcol;
	SetConsoleTextAttribute(std_con_out, wAttributes);
}

void Q_ColorPrint(char *text)
{
	setcolor(concol_grey, concol_black); // init console color on each new string...

	int len = strlen(text);

	for (int c = 0; c < len; c++)
	{// This could probably be optimized by dumping more than one char at a time... but this will do...
		if (c == len - 1)
		{
			// Final character, just print it...
			printf("%c", text[c]);
			break;
		}
		else {
			// Check for color changes...
			char read[2] = { 0 };
			sprintf(read, "%c%c", text[c], text[c + 1]);

			if (Q_IsColorStringExt(read))
			{// Color swap character...
			 // Set the new console color...
				if (!strcmp(read, S_COLOR_BLACK))
					setcolor(concol_dark_white, concol_black); // never allow true black...
				else if (!strcmp(read, S_COLOR_RED))
					setcolor(concol_dark_red, concol_black); // no equivalent console color for orange, so using red for orange, and dark_red for red
				else if (!strcmp(read, S_COLOR_GREEN))
					setcolor(concol_green, concol_black);
				else if (!strcmp(read, S_COLOR_YELLOW))
					setcolor(concol_yellow, concol_black);
				else if (!strcmp(read, S_COLOR_BLUE))
					setcolor(concol_blue, concol_black);
				else if (!strcmp(read, S_COLOR_CYAN))
					setcolor(concol_cyan, concol_black);
				else if (!strcmp(read, S_COLOR_MAGENTA))
					setcolor(concol_magenta, concol_black);
				else if (!strcmp(read, S_COLOR_WHITE))
					setcolor(concol_white, concol_black);
				else if (!strcmp(read, S_COLOR_ORANGE))
					setcolor(concol_red, concol_black); // no equivalent console color, so using red for orange
				else if (!strcmp(read, S_COLOR_GREY))
					setcolor(concol_grey, concol_black);
				else // Should never happen...
					setcolor(concol_grey, concol_black);

				c++; // skip to after the final color macro character and continue...
				continue;
			}
			else if (!strcmp(read, "\n")) {
				// Newline char here, just dump the 2 chars at once...
				printf("\n");
				c++;
				continue;
			}

			printf("%c", text[c]);
		}
	}

	setcolor(concol_grey, concol_black); // init console color at end of each new string too...
}

int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int retval;

	retval = _vsnprintf(str, size, format, ap);

	if (retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return size;
	}

	return retval;
}

#define	MAXPRINTMSG	4096

void __cdecl Q_Printf(const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	Q_ColorPrint(msg);
}
#else //!defined(WIN32) || defined(WIN64)
inline void setcolor(concol textcol, concol backcol)
{

}

inline void setcolor(int textcol, int backcol)
{

}

#define Q_Printf printf
#endif //defined(WIN32) || defined(WIN64)

void COM_StripExtension(const char *in, char *out, int destsize)
{
	const char *dot = strrchr(in, '.'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot - in + 1 ? destsize : dot - in + 1);

	if (in == out && destsize > 1)
		out[destsize - 1] = '\0';
	else
		strncpy(out, in, destsize);
}

bool StringContainsWord(const char *haystack, const char *needle)
{
	if (!*needle)
	{
		return false;
	}
	for (; *haystack; ++haystack)
	{
		if (toupper(*haystack) == toupper(*needle))
		{
			/*
			* Matched starting char -- loop through remaining chars.
			*/
			const char *h, *n;
			for (h = haystack, n = needle; *h && *n; ++h, ++n)
			{
				if (toupper(*h) != toupper(*n))
				{
					break;
				}
			}
			if (!*n) /* matched all of 'needle' to null termination */
			{
				return true; /* return the start of the match */
			}
		}
	}
	return false;
}

bool FS_FileExists(const char *testpath)
{
	struct stat buffer;
	return (bool)(stat(testpath, &buffer) == 0);
}

char texName[512] = { 0 }; // not thread safe, but we are not threading anywhere here...

char *FS_TextureFileExists(const char *name)
{
	if (!name || !name[0] || name[0] == '\0' || strlen(name) < 1) return NULL;

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.png", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.tga", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.jpg", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.dds", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.gif", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.bmp", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	memset(&texName, 0, sizeof(char) * 512);
	COM_StripExtension(name, texName, sizeof(texName));
	sprintf(texName, "%s.ico", name);

	if (FS_FileExists(texName))
	{
		return texName;
	}

	return NULL;
}

const bool normalizeNormals = true;

//=================================================================================================
// Check if a character is a decimal
//=================================================================================================
inline bool isDecimal(char c)
{
	return c != 0 && c >= '0' && c <= '9';
}

//=================================================================================================
// Check if a character is a whitespace
//=================================================================================================
inline bool isWhitespace(char c)
{
	return c == ' ' || c=='\t';
}

//=================================================================================================
// Check if a character is a forward slash
//=================================================================================================
inline bool isSlash(char c)
{
	return c == '/';
}

//=================================================================================================
// Converts a character to a integer
//=================================================================================================
inline int charToInt(char c)
{
	return static_cast<int>(c) - static_cast<int>('0');
}

//=================================================================================================
// Consumes a slash
//=================================================================================================
inline bool parseSlash(const char*& str)
{
	if(isSlash(*str))
	{
		str++;
		return true;
	}
	else
	{
		return false;
	}
}

//=================================================================================================
// Consumes all whitespaces
//=================================================================================================
inline bool parseWhitespace(const char*& str)
{
	if(!isWhitespace(*str))
	{
		return false;
	}
	while(isWhitespace(*str))
	{
		str++;
	}
	return true;
}

//=================================================================================================
// Consumes a number
//=================================================================================================
bool parseNumber(const char*& str, int& result)
{
	if (!isDecimal(*str))
	{
		return false;
	}

	result = 0;
	while(isDecimal(*str))
	{
		result = result*10 + charToInt(*str);
		str++;
	}

	return true;
}

//=================================================================================================
// Consumes a vertex definition inside a face definition
//=================================================================================================
bool parseVertex(const char*& str, int& vertex, int& normal, int& texcoord)
{
	if(!parseNumber(str, vertex))
	{
		return false;
	}

	normal = vertex;
	texcoord = vertex;

	if(parseSlash(str))
	{
		parseNumber(str, texcoord);
	}

	if(parseSlash(str))
	{
		parseNumber(str, normal);
	}

	return true;
}

//=================================================================================================
// Consumes a face definition
//=================================================================================================
bool parseFace(const char*& str, Vector3i* indices, int& vertexCount, int maxVertexCount)
{
	parseWhitespace(str);

	vertexCount = 0;

	int indexV = 0, indexN = 0, indexT = 0;

	while(parseVertex(str, indexV, indexN, indexT) && vertexCount < maxVertexCount)
	{
		indices[vertexCount].data[0] = indexV;
		indices[vertexCount].data[1] = indexN;
		indices[vertexCount].data[2] = indexT;

		vertexCount++;

		parseWhitespace(str);
	}

	return true;
}

//=================================================================================================
// Parial ordering of faces
//=================================================================================================
struct CompareFaces 
{
	bool operator() (const Vector3i &a, const Vector3i &b) const
	{ 
		if (a.data[0] > b.data[0])
		{
			return true;
		}
		if (a.data[0] < b.data[0])
		{
			return false;
		}
		if (a.data[1] > b.data[1])
		{
			return true;
		}
		if (a.data[1] < b.data[1])
		{
			return false;
		}

		return a.data[2] > b.data[2];
	}
};

//=================================================================================================
// Parses a color
//=================================================================================================
bool parseColor(std::istringstream& stream, Vector3f& color)
{
	for(int i=0; i<3; i++)
	{
		stream >> color.data[i];
	}
	return true;
}

//=================================================================================================
// Loads a material
// newmtl materialName
// illum 2
// Kd 0.000000 0.000000 0.000000
// Ka 0.250000 0.250000 0.250000
// Ks 1.000000 1.000000 1.000000
// Ke 0.000000 0.000000 0.000000
// Ns 0.000000
// map_Kd textureFileName.tga
//=================================================================================================
void loadMaterialFile(const std::string& filename, MaterialMapType& materials)
{
	// Open the file
	std::ifstream infile;
	infile.open(filename.c_str());
	if(!infile.is_open())
	{
		throw std::runtime_error("Unable to open material file: " + filename);
	}

	std::string line;
	int linecount = 0;
	Material currentMaterial;
	std::string currentName;

	while(getline(infile, line))
	{
		linecount++;

		// Skip comments and empty lines
		if(line.length()==0 || line[0] == '#')
		{
			continue;
		}

		// Streamed reads are easier
		std::istringstream stream(line);
		std::string type;

		// Read the command
		stream >> type;
		std::transform(type.begin(), type.end(), type.begin(), tolower); 

		if(type == "newmtl")
		{
			if (!currentName.empty())
			{
				if (materials.find(currentName)!=materials.end())
				{
					std::cerr << "duplicate material definition for " << currentName << std::endl;
				}
				materials[currentName] = currentMaterial;
			}
			currentMaterial.reset();
			stream >> currentName;
		}
		else if (type == "illum")
		{
			stream >> currentMaterial.illuminationModel;
		}
		else if (type == "kd")
		{
			parseColor(stream, currentMaterial.colorDiffuse);
		}
		else if (type == "ks")
		{
			parseColor(stream, currentMaterial.colorSpecular);
		}
		else if (type == "ka")
		{
			parseColor(stream, currentMaterial.colorAmbient);
		}
		else if (type == "ke")
		{
			parseColor(stream, currentMaterial.colorEmissive);
		}
		else if (type == "ns")
		{
			stream >> currentMaterial.shininess;
		}
		else if (type == "d" || type == "Tr")
		{
			stream >> currentMaterial.transparency;
		}
		else if (type == "map_kd")
		{
			stream >> currentMaterial.textureDiffuse;
		}
		else if (type == "map_ks")
		{
			stream >> currentMaterial.textureSpecular;
		}
		else if (type == "map_ka")
		{
			stream >> currentMaterial.textureAmbient;
		}
		else if (type == "map_ke")
		{
			stream >> currentMaterial.textureEmissive;
		}
		else if (type == "map_bump" || type == "bump")
		{
			stream >> currentMaterial.textureBump;
		}
		else if (type == "map_d" || type == "map_tr")
		{
			stream >> currentMaterial.textureTransparency;
		}
		else
		{
			std::cerr << "unknown material command: " << type << std::endl;
		}
	}

	if (!currentName.empty())
	{
		if (materials.find(currentName)!=materials.end())
		{
			std::cerr << "duplicate material definition for " << currentName << std::endl;
		}
		materials[currentName] = currentMaterial;
	}
}

//=================================================================================================
// Loads an obj file
//=================================================================================================
void loadObj(const std::string& filename, Mesh& result)
{	
	// Clear the result
	result.reset();

	// Open the file
	std::ifstream infile;
	infile.open(filename.c_str());
	if(!infile.is_open())
	{
		throw std::runtime_error("Unable to open mesh file: " + filename);
	}

	std::string line;
	bool hasNormals = false;
	bool hasTextureCoordinates = false;

	// Original mesh data.
	// This might get duplicated if two faces partially share vertex data
	std::vector<Vector3f> vertices;
	std::vector<Vector3f> normals;
	std::vector<Vector2f> texcoord;
	std::map<Vector3i, int, CompareFaces> uniqueVertexMap;

	// Loop over all lines
	int unsupportedTypeWarningsLeft = 10;
	int linecount = 0;
	while(getline(infile, line))
	{
		linecount++;

		// Skip comments and empty lines
		if(line.length()==0 || line[0] == '#')
		{
			continue;
		}

		// Read the command
		std::istringstream stream(line);
		std::string type;

		stream >> type;
		std::transform(type.begin(), type.end(), type.begin(), tolower); 

		if(type == "mtllib")
		{
			std::string materialFileName;
			stream >> materialFileName;
			// FIXME: handle relative and absolute file names
			loadMaterialFile(materialFileName, result.materials);
		}
		else if(type == "g")
		{
			result.components.push_back(MeshComponent());
			stream >> result.components.back().componentName;
		}
		else if(type == "usemtl")
		{
			if(result.components.size()==0)
			{
				throw std::runtime_error("material without a group encountered");
			}
			if (!result.components.back().materialName.empty())
			{
				std::cerr << "component " << result.components.back().componentName << " already has a material, replacing the old definition";
			}
			stream >> result.components.back().materialName;
		}
		else if(type == "s")
		{
			// FIXME: Smooth shading
		}
		else if(type == "o")
		{
			// FIXME: object names (no real use)
		}
		else if(type == "v")
		{
			// Vertex position data
			Vector3f v;
			stream >> v.data[0] >> v.data[1] >> v.data[2];
			vertices.push_back(v);
		} 
		else if(type == "vn")
		{
			// Vertex normal data
			Vector3f vn;
			stream >> vn.data[0] >> vn.data[1] >> vn.data[2];
			if (normalizeNormals)
			{
				float normalLength = vn.data[0]*vn.data[0] + vn.data[1]*vn.data[1] + vn.data[2]*vn.data[2];
				if (abs(1.0f-normalLength) > 1e-3f && normalLength > 1e-3f)
				{
					vn.data[0] /= normalLength;
					vn.data[1] /= normalLength;
					vn.data[2] /= normalLength;
				}
			}

			hasNormals = true;
			normals.push_back(vn);
		} 
		else if(type == "vt")
		{
			// Vertex texture coordinate data
			Vector2f vt;
			stream >> vt.data[0] >> vt.data[1];
			hasTextureCoordinates = true;
			texcoord.push_back(vt);
		} 
		else if(type == "f")
		{
			// Face data
			int pos = (int) stream.tellg();

			const char* ptr = line.c_str() + pos;

			// Every vertex may supply up to 3 indexes (vertex index, normal index, texcoord index)
			Vector3i loadedIndices[3];
			int mappedIndices[3];
			int vertexCount;

			if(parseFace(ptr, loadedIndices, vertexCount, 3))
			{
				// If the face is legal, for every vertex, assign the normal and the texcoord
				for (int i = 0; i < vertexCount; ++i)
				{
					if (uniqueVertexMap.find(loadedIndices[i]) != uniqueVertexMap.end())
					{
						mappedIndices[i] = uniqueVertexMap[loadedIndices[i]];
					}
					else
					{
						mappedIndices[i] = (int) result.vertices.size();
						uniqueVertexMap.insert(std::make_pair(loadedIndices[i], (int) uniqueVertexMap.size()));

						int vertexIndex = loadedIndices[i].data[0] - 1;
						if (vertexIndex < (int) vertices.size())
						{
							result.vertices.push_back(vertices[vertexIndex]);	
						}
						else
						{
							throw std::runtime_error("Unknown vertex specified");
						}

						if (hasNormals)
						{
							int normalIndex = loadedIndices[i].data[1] - 1;

							if (normalIndex < (int) normals.size())
							{
								result.normals.push_back(normals[normalIndex]);	
							}
							else
							{
								throw std::runtime_error("Unknown normal specified");
							}
						}


						if (hasTextureCoordinates)
						{
							int texCoordIndex = loadedIndices[i].data[2] - 1;
							
							if (texCoordIndex < (int) texcoord.size())
							{
								result.texcoord.push_back(texcoord[texCoordIndex]);	
							}
							else
							{
								throw std::runtime_error("Unknown texture coordinate specified");
							}
						}
					}
				}

				if (vertexCount == 3)
				{
					if (result.components.size()==0)
					{
						result.components.push_back(MeshComponent());
						result.components.back().componentName = "[default]";
					}

					Vector3i tri(mappedIndices);
					result.components.back().faces.push_back(tri);
				}
				else if (vertexCount == 4)
				{
					throw std::runtime_error("OBJ loader: quads are not supported, convert them to triangles");
				}
				else
				{
					throw std::runtime_error("OBJ loader: face with strange number of vertices encountered");
				}
			}
			else
			{
				throw std::runtime_error("OBJ loader: face could not be parsed");
			}
		}
		else if (unsupportedTypeWarningsLeft > 0)
		{
			std::cerr << "Unsupported type in obj: " << type << " at line " << linecount << std::endl;
			unsupportedTypeWarningsLeft--;
			if(unsupportedTypeWarningsLeft==0)
			{
				std::cerr << "Too many warnings about unsupported types, further warnings are suppressed." << std::endl;
			}
		}
	}

	if (!hasNormals)
	{
		throw std::runtime_error("OBJ loader: mesh without normals");
	}

	if (result.texcoord.size()==0)
	{
		Vector2f defaultTexCoord;
		defaultTexCoord.data[0] = 0;
		defaultTexCoord.data[1] = 0;
		result.texcoord.resize(result.vertices.size(), defaultTexCoord);
	}

	if (result.normals.size() != result.vertices.size())
	{
		throw std::runtime_error("OBJ loader: inconsistent number of normals");
	}
	if (result.texcoord.size() != result.vertices.size())
	{
		throw std::runtime_error("OBJ loader: inconsistent number of texture coordinates");
	}
}

//=================================================================================================
// Loads a model file
//=================================================================================================
void loadModel(const std::string& filename, Mesh& result)
{
	// Clear the result
	result.reset();

	FILE *infile;

	if (!(infile = fopen(filename.c_str(), "rb")))
	{
		printf("ERROR: Failed to open file %s\n", filename.c_str());
		return;
	}

	int bufSize = 0;
	unsigned char *buf;

	// load the file contents into a buffer
	fseek(infile, 0, SEEK_END);
	bufSize = ftell(infile);
	fseek(infile, 0, SEEK_SET);
	buf = (unsigned char*)malloc(bufSize);
	if (!buf)
	{
		printf("^1ERROR^5: Memory allocation failed\n");
		return;
	}
	if (fread(buf, 1, bufSize, infile) != bufSize)
	{
		printf("^1ERROR^5: Failed to read file (%d bytes) into buffer\n", (int)bufSize);
		return;
	}

	fclose(infile);

#define aiProcessPreset_Settings (\
	aiProcess_ImproveCacheLocality	|\
	aiProcess_JoinIdenticalVertices	|\
	aiProcess_GenNormals	|\
	aiProcess_ValidateDataStructure	|\
	aiProcess_FindInvalidData	|\
	aiProcess_FindDegenerates	|\
	aiProcess_GenUVCoords	|\
	aiProcess_TransformUVCoords	|\
	aiProcess_Triangulate	|\
	aiProcess_SortByPType	|\
	aiProcess_FindInstances                  |\
    aiProcess_ValidateDataStructure          |\
	aiProcess_OptimizeMeshes                 |\
	aiProcess_GenSmoothNormals              |  \
    0 )

	const aiScene* scene = assImpImporter.ReadFileFromMemory(buf, bufSize, aiProcessPreset_Settings, "");

	if (!scene)
	{
		printf("ERROR: An import error occurred: %s.\n", assImpImporter.GetErrorString());
		return;
	}
	else if (scene->mNumMeshes < 1)
	{
		printf("An import error occurred: Model has no surfaces\n");
		return;
	}
	else if (strlen(assImpImporter.GetErrorString()) > 0)
	{
		printf("WARNING: An import warning occurred: %s.\n\n", assImpImporter.GetErrorString());
	}
	else
	{
		printf("Model %s was imported without issue.\n\n", filename.c_str());
	}

	std::string basePath = AssImp_getBasePath(filename);

	char surfaceNames[1024][256] = { 0 };
	char surfaceTextures[1024][256] = { 0 };

	Material currentMaterial;
	std::string currentName;

	// geometry - iterate over all the MD3 surfaces
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiString	shaderPath;
		bool		foundName = false;
		aiMesh		*surf = scene->mMeshes[i];

		scene->mMaterials[surf->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &shaderPath);

		if (surf->mName.length > 0 && FS_FileExists(surf->mName.C_Str()))
		{// Original file+path exists... Use it...
			shaderPath = surf->mName;
			foundName = true;
		}

		if (shaderPath.length > 0 && FS_FileExists(shaderPath.C_Str()))
		{// Original file+path exists... Use it...
			foundName = true;
		}

		if (surf->mName.length > 0 && FS_TextureFileExists(surf->mName.C_Str()))
		{// Original file+path exists... Use it...
			char *nExt = FS_TextureFileExists(surf->mName.C_Str());

			if (nExt && nExt[0])
			{
				shaderPath = nExt;
				foundName = true;
			}
			foundName = true;
		}

		if (shaderPath.length > 0 && FS_TextureFileExists(shaderPath.C_Str()))
		{// Original file+path exists... Use it...
			char *nExt = FS_TextureFileExists(shaderPath.C_Str());

			if (nExt && nExt[0])
			{
				shaderPath = nExt;
				foundName = true;
			}
			foundName = true;
		}

		if (!foundName)
		{// Grab the filename from the original path.
			std::string textureName = AssImp_getTextureName(shaderPath.C_Str());

			//Q_Printf("DEBUG: shaderPath is %s.\n", shaderPath.C_Str());

			// Free the shaderPath so that we can replace it with what we find...
			shaderPath.Clear();

			//Q_Printf("DEBUG: textureName is %s.\n", textureName.c_str());

			if (textureName.length() > 0 && !foundName)
			{// Check if the file exists in the current directory...
				if (FS_FileExists(textureName.c_str()))
				{
					shaderPath = textureName.c_str();
					foundName = true;
				}
				//else
				//{
				//	Q_Printf("DEBUG: %s does not exist.\n", textureName.c_str());
				//}
			}

			if (textureName.length() > 0 && !foundName)
			{// Check if filename.ext exists in the current directory...
				char *nExt = FS_TextureFileExists(textureName.c_str());

				if (nExt && nExt[0])
				{
					shaderPath = nExt;
					foundName = true;
				}
				//else
				//{
				//	Q_Printf("DEBUG: %s does not exist.\n", nExt);
				//}
			}

			if (textureName.length() > 0 && !foundName)
			{// Search for the file... Make final dir/filename.ext
				char out[256] = { 0 };
				COM_StripExtension(textureName.c_str(), out, sizeof(out));
				textureName = out;

				char shaderRealPath[256] = { 0 };
				sprintf(shaderRealPath, "%s%s", basePath.c_str(), textureName.c_str());

				char *nExt = FS_TextureFileExists(shaderRealPath);

				if (nExt && nExt[0])
				{
					shaderPath = nExt;
					foundName = true;
				}
				//else
				//{
				//	Q_Printf("DEBUG: %s does not exist.\n", nExt);
				//}
			}

			if (shaderPath.length == 0)
			{// We failed... Set name do "unknown".
				shaderPath = "unknown";
			}
		}

		char surfaceName[256] = { 0 };
		COM_StripExtension(shaderPath.C_Str(), surfaceName, sizeof(surfaceName));
		surfaceName[strlen(surfaceName) - 1] = '\0';
		strcpy(surfaceNames[i], surfaceName);
		strcpy(surfaceTextures[i], shaderPath.C_Str());

		surf->mName.Set(surfaceName/*shaderPath.C_Str()*/);

		// ------
		aiMaterial* pcHelper = new aiMaterial();

		const int iMode = (int)aiShadingMode_Gouraud;
		pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

		// Add a small ambient color value - Quake 3 seems to have one
		aiColor3D clr;
		clr.b = clr.g = clr.r = 0.05f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_AMBIENT);

		clr.b = clr.g = clr.r = 1.0f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_DIFFUSE);
		pcHelper->AddProperty<aiColor3D>(&clr, 1, AI_MATKEY_COLOR_SPECULAR);

		// use surface name + skin_name as material name
		aiString name;
		name.Set(surfaceName);
		pcHelper->AddProperty(&name, AI_MATKEY_NAME);

		aiString szString;
		szString.Set(shaderPath.C_Str());
		pcHelper->AddProperty(&szString, AI_MATKEY_TEXTURE_DIFFUSE(0));
		scene->mMaterials[surf->mMaterialIndex] = (aiMaterial*)pcHelper;

		// ------

		Q_Printf(" ^5+ ^3%s^5. ^7%d^5 faces. ^7%d^5 verts.\n", surf->mName.C_Str(), surf->mNumFaces, surf->mNumVertices);

		// ------

		if (!currentName.empty())
		{
			if (result.materials.find(currentName) != result.materials.end())
			{
				std::cerr << "duplicate material definition for " << currentName << std::endl;
			}
			result.materials[currentName] = currentMaterial;
		}

		currentMaterial.reset();
		currentName = surf->mName.C_Str();
		currentMaterial.textureDiffuse = shaderPath.C_Str();

		// group
		result.components.push_back(MeshComponent());
		result.components.back().componentName = currentName;

		// mats
		result.components.back().materialName = currentName;

		int vStart = result.vertices.size();

		// output the vertex list
		for (unsigned int j = 0; j < surf->mNumVertices; j++)
		{
			// XYZ
			Vector3f v;
			v.data[0] = (float)surf->mVertices[j].x;
			v.data[1] = (float)surf->mVertices[j].y;
			v.data[2] = (float)surf->mVertices[j].z;
			result.vertices.push_back(v);

			// Vertex normal data
			Vector3f vn;
			vn.data[0] = (float)surf->mNormals[j].x;
			vn.data[1] = (float)surf->mNormals[j].y;
			vn.data[2] = (float)surf->mNormals[j].z;
			result.normals.push_back(vn);

			if (surf->mNormals != NULL && surf->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
			{
				Vector2f vt;
				vt.data[0] = (float)surf->mTextureCoords[0][j].x;
				vt.data[1] = (float)surf->mTextureCoords[0][j].y;
				result.texcoord.push_back(vt);
			}
			else
			{
				Vector2f vt;
				vt.data[0] = 0.0;
				vt.data[1] = 1.0;
				result.texcoord.push_back(vt);
			}
		}

		// output the triangle list
		//int numt = 0;
		for (unsigned int j = 0; j < surf->mNumFaces; j++)
		{// Assuming triangles for now... AssImp is currently set to convert everything to triangles anyway...
			Vector3i tri;
			tri.data[0] = surf->mFaces[j].mIndices[0] + vStart;
			tri.data[1] = surf->mFaces[j].mIndices[1] + vStart;
			tri.data[2] = surf->mFaces[j].mIndices[2] + vStart;

			//printf("tri %i - %i %i %i.\n", numt, tri.data[0], tri.data[1], tri.data[2]);
			//numt++;

			result.components.back().faces.push_back(tri);
		}
	}

	if (!currentName.empty())
	{
		if (result.materials.find(currentName) != result.materials.end())
		{
			std::cerr << "duplicate material definition for " << currentName << std::endl;
		}
		result.materials[currentName] = currentMaterial;
	}

	if (result.texcoord.size() == 0)
	{
		Vector2f defaultTexCoord;
		defaultTexCoord.data[0] = 0;
		defaultTexCoord.data[1] = 0;
		result.texcoord.resize(result.vertices.size(), defaultTexCoord);
	}

	if (result.normals.size() != result.vertices.size())
	{
		throw std::runtime_error("OBJ loader: inconsistent number of normals");
	}
	if (result.texcoord.size() != result.vertices.size())
	{
		throw std::runtime_error("OBJ loader: inconsistent number of texture coordinates");
	}
}

template<class T>
void writeVector(std::ofstream& outfile, const std::string& type, const T& v)
{
	outfile << type << " ";
	for(int i=0; i<T::Dimension-1; i++)
	{
		outfile << v.data[i] << " ";
	}
	outfile << v.data[T::Dimension-1];
	outfile << std::endl;
}

void writeVertexIndex(std::ofstream& outfile, int index, bool hasNormals, bool hasTexCoord)
{
	index++;
	if(hasNormals && hasTexCoord)
	{
		outfile << index << "/" << index << "/" << index;
	}
	else if (hasNormals)
	{
		outfile << index << "/" << index;
	}
	else if (hasTexCoord)
	{
		outfile << index << "//" << index;
	}
}

void writeMaterialTexture(std::ofstream& outfile, const std::string& type, const std::string& textureFilename)
{
	if (!textureFilename.empty())
	{
		outfile << type << " " << textureFilename << std::endl;
	}
}

void writeObj(const std::string& filename, const std::string matFilename, const Mesh& mesh)
{
	// Open the file
	std::ofstream outfile;
	outfile.open(filename.c_str());
	if(!outfile.is_open())
	{
		throw std::runtime_error("Unable to open output mesh file: " + filename);
	}

	bool hasVertices = true;
	if(mesh.vertices.size()==0)
	{
		throw std::runtime_error("mesh contains no vertices");
	}

	bool hasNormals = false;
	if (mesh.normals.size() == mesh.vertices.size())
	{
		hasNormals = true;
	}
	else if (mesh.normals.size() > 0)
	{
		throw std::runtime_error("inconsistent number of normals");
	}
	bool hasTexCoord = false;
	if (mesh.texcoord.size() == mesh.vertices.size())
	{
		hasTexCoord = true;
	}
	else if (mesh.texcoord.size() > 0)
	{
		throw std::runtime_error("inconsistent number of vertex coordinates");
	}

	// Write material lib
	outfile << "mtllib " << matFilename << std::endl;

	// Write vertices
	outfile << "#vertices (" << mesh.vertices.size() << ")" << std::endl;
	for(size_t i=0;i<mesh.vertices.size();i++)
	{
		writeVector(outfile, "v", mesh.vertices[i]);
	}

	// Write normals
	if(hasNormals)
	{
		outfile << "#normals (" << mesh.normals.size() << ")" << std::endl;
		for(size_t i=0;i<mesh.normals.size();i++)
		{
			writeVector(outfile, "vn", mesh.normals[i]);
		}
	}
	else
	{
		outfile << "#normals not available" << std::endl;
	}
	
	// Write texture coordinates
	if(hasTexCoord)
	{
		outfile << "#texture coordinates (" << mesh.texcoord.size() << ")" << std::endl;
		for(size_t i=0;i<mesh.texcoord.size();i++)
		{
			writeVector(outfile, "vt", mesh.texcoord[i]);
		}
	}
	else
	{
		outfile << "#texture coordinates not available" << std::endl;
	}

	// Write components
	outfile << "#components (" << mesh.components.size() << ")" << std::endl;
	for(ComponentListType::const_iterator ic=mesh.components.begin(); ic!=mesh.components.end(); ++ic)
	{
		const MeshComponent& comp = *ic;
		// Component name
		outfile << "g " << comp.componentName << std::endl;
		// Component material
		if(!comp.materialName.empty())
		{
			outfile << "usemtl " << comp.materialName << std::endl;
		}
		outfile << "s " << 1 << std::endl;
		// Faces
		for(size_t i=0;i<comp.faces.size();++i)
		{
			outfile << "f ";
			writeVertexIndex(outfile, comp.faces[i].data[0], hasNormals, hasTexCoord);
			outfile << " ";
			writeVertexIndex(outfile, comp.faces[i].data[1], hasNormals, hasTexCoord);
			outfile << " ";
			writeVertexIndex(outfile, comp.faces[i].data[2], hasNormals, hasTexCoord);
			outfile << std::endl;
		}
	}
	outfile.close();

	// Write materials
	std::ofstream matfile;
	matfile.open(matFilename.c_str());
	if(!matfile.is_open())
	{
		throw std::runtime_error("Unable to open output material file: " + matFilename);
	}
	for(MaterialMapType::const_iterator im=mesh.materials.begin(); im!=mesh.materials.end(); ++im)
	{
		const std::string& name = im->first;
		const Material& mat = im->second;

		matfile << "#material" << std::endl;
		matfile << "newmtl " << name << std::endl;
		matfile << "illum " << mat.illuminationModel << std::endl;
		writeVector(matfile, "Ka", mat.colorAmbient);
		writeVector(matfile, "Kd", mat.colorDiffuse);
		writeVector(matfile, "Ks", mat.colorSpecular);
		writeVector(matfile, "Ke", mat.colorEmissive);
		matfile << "Ns " << mat.shininess << std::endl;
		matfile << "d " << mat.transparency << std::endl;
		writeMaterialTexture(matfile, "map_Ka",   mat.textureAmbient);
		writeMaterialTexture(matfile, "map_Kd",   mat.textureDiffuse);
		writeMaterialTexture(matfile, "map_Ks",   mat.textureSpecular);
		writeMaterialTexture(matfile, "map_Ke",   mat.textureEmissive);
		writeMaterialTexture(matfile, "map_bump", mat.textureBump);
		writeMaterialTexture(matfile, "map_d",    mat.textureTransparency);
		matfile << std::endl;
	}
	matfile.close();
}
