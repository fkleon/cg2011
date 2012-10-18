//This file reads from lightwave .obj scenes
#include "stdafx.h"
#include "lwobject.h"
#include "phong_shaders.h"

#ifdef __unix
#include <libgen.h>
#define _strnicmp strncasecmp
#endif

#ifdef _WIN32
#define _PATH_SEPARATOR '\\'
#endif
#ifdef __unix
#define _PATH_SEPARATOR '/'
#endif

namespace objLoaderUtil
{
	typedef std::map<std::string, size_t> t_materialMap;

	void skipWS(const char * &aStr)
	{
		while(isspace(*aStr))
			aStr++;
	}

	std::string endSpaceTrimmed(const char* _str)
	{
		size_t len = strlen(_str);
		const char *firstChar = _str;
		const char *lastChar = firstChar + len - 1;
		while(lastChar >= firstChar && isspace(*lastChar))
			lastChar--;

		return std::string(firstChar, lastChar + 1);
	}


	std::string getDirName(const std::string& _name)
	{
		std::string objDir;
#if _MSC_VER >= 1400
		char fileDir[4096];
		_splitpath_s(_name.c_str(), NULL, 0, fileDir, sizeof(fileDir), NULL, 0, NULL, 0);
		objDir = fileDir;
#endif

#ifdef __unix
		char *fnCopy = strdup(_name.c_str());
		const char* dirName = dirname(fnCopy);
		objDir = dirName;
		free(fnCopy);
		//std::cerr << "Dirname:" << objDir << std::endl;

#endif // __unix

		return objDir;
	}


	void readMtlLib(const std::string &_fileName, LWObject::t_materialVector &_matVector, const t_materialMap &_matMap)
	{
		std::ifstream matInput(_fileName.c_str(), std::ios_base::in);
		std::string buf;

		if(matInput.fail())
			throw std::runtime_error("Error opening .mtl file");

		size_t curMtl = -1, curLine = 0;

		while(!matInput.eof())
		{
			std::getline(matInput, buf);
			curLine++;
			const char* cmd = buf.c_str();
			skipWS(cmd);

			if(_strnicmp(cmd, "newmtl", 6) == 0)
			{
				cmd += 6;

				skipWS(cmd);
				std::string name = endSpaceTrimmed(cmd);
				if(_matMap.find(name) == _matMap.end())
					goto parse_err_found;

				curMtl = _matMap.find(name)->second;
			}
			else if(
				_strnicmp(cmd, "Kd", 2) == 0 || _strnicmp(cmd, "Ks", 2) == 0
				|| _strnicmp(cmd, "Ka", 2) == 0)
			{
				char coeffType = *(cmd + 1);

				if(curMtl == -1)
					goto parse_err_found;

				float4 color;
				color.w = 0;
				cmd += 2;

				char *newCmdString;
				for(int i = 0; i < 3; i++)
				{
					skipWS(cmd);
					((float*)&color)[i] = (float)strtod(cmd, &newCmdString);
					if(newCmdString == cmd) goto parse_err_found;
					cmd = newCmdString;
				}


				switch (coeffType)
				{
					case 'd':
						_matVector[curMtl].diffuseCoeff = color; break;
					case 'a':
						_matVector[curMtl].ambientCoeff = color; break;
					case 's':
						_matVector[curMtl].specularCoeff = color; break;
				}
			}
			else if(_strnicmp(cmd,  "Ns", 2) == 0)
			{
				if(curMtl == -1)
					goto parse_err_found;

				cmd += 2;

				char *newCmdString;
				skipWS(cmd);
				float coeff = (float)strtod(cmd, &newCmdString);
				if(newCmdString == cmd) goto parse_err_found;
				cmd = newCmdString;
				_matVector[curMtl].specularExp = coeff;
			}
			else if(
				_strnicmp(cmd,  "map_Kd", 6) == 0 || _strnicmp(cmd,  "map_Ks", 6) == 0
				|| _strnicmp(cmd,  "map_Ka", 6) == 0 || _strnicmp(cmd,  "map_bump", 7) == 0
				)
			{
				SmartPtr<Texture> &dest =
					(_strnicmp(cmd, "map_Kd", 6) == 0) ?  _matVector[curMtl].diffuseTexture :
					((_strnicmp(cmd, "map_Ka", 6) == 0) ? _matVector[curMtl].ambientTexture :
					((_strnicmp(cmd, "map_Ks", 6) == 0) ? _matVector[curMtl].specularTexture :
					_matVector[curMtl].bumpTexture));


				bool bumpTex = _strnicmp(cmd,  "map_bump", 8) == 0;
				cmd += 6;

				if(bumpTex)
				{
					cmd += 2;
					skipWS(cmd);
					if(_strnicmp(cmd, "-bm", 3) == 0)
					{
						cmd += 3;
						char *newCmdString;
						skipWS(cmd);
						float coeff = (float)strtod(cmd, &newCmdString);
						if(newCmdString == cmd) goto parse_err_found;
						cmd = newCmdString;
						_matVector[curMtl].bumpIntensity = coeff;
					}
				}

				skipWS(cmd);
				std::string fn = endSpaceTrimmed(cmd);
				std::string pngFileName = getDirName(_fileName) + _PATH_SEPARATOR + fn;

				SmartPtr<Image> img = new Image;
				img->readPNG(pngFileName);

				SmartPtr<Texture> tex = new Texture;
				tex->image = img;

                // set type to height map if applicable
				if (bumpTex)
                    tex->textureType = Texture::TT_HateMap;

				dest = tex;
			}

			continue;
parse_err_found:
			std::cerr << "Error at line " << curLine << "in " << _fileName <<std::endl;
		}
	}
}

using namespace objLoaderUtil;

void LWObject::read(const std::string &_fileName, bool _createDefautShaders)
{
	std::ifstream inputStream(_fileName.c_str(), std::ios_base::in);
	std::string buf;

	const size_t _MAX_BUF = 8192;
	const size_t _MAX_IDX = _MAX_BUF / 2;

	float tmpVert[4];
	size_t tmpIdx[_MAX_IDX * 3];
	int tmpVertPointer, tmpIdxPtr, vertexType;
	size_t curMat = 0, curLine = 0;
	std::vector<std::string> matFiles;

	Material defaultMaterial;
	defaultMaterial.name = "Default_{B77D36AF-37CE-4144-B772-E0F00F626DF6}";
	defaultMaterial.diffuseCoeff = float4(1, 1, 1, 0);
	defaultMaterial.specularCoeff = float4(0, 0, 0, 0);
	defaultMaterial.specularExp = 0;
	defaultMaterial.ambientCoeff = float4(0.2f, 0.2f, 0.2f, 0);
	materials.push_back(defaultMaterial);

	materialMap.insert(std::make_pair("defaultMaterial.name", (size_t)0));

	if(inputStream.fail())
		throw std::runtime_error("Error opening .obj file");

	while(!inputStream.eof())
	{
		std::getline(inputStream, buf);
		const char *cmdString = buf.c_str();

		curLine++;
		skipWS(cmdString);
		switch(tolower(*cmdString))
		{
		case 0:
			break;
		case 'v':
			cmdString++;
			switch(tolower(*cmdString))
			{
				case 'n': vertexType = 1; cmdString++; break;
				case 't': vertexType = 2; cmdString++; break;
				default:
					if(isspace(*cmdString))
						vertexType = 0;
					else
						goto parse_err_found;
			}

			tmpVertPointer = 0;
			for(;;)
			{
				skipWS(cmdString);
				if(*cmdString == 0)
					break;

				char *newCmdString;
				float flt = (float)strtod(cmdString, &newCmdString);
				if(newCmdString == cmdString)
					goto parse_err_found;

				cmdString = newCmdString;

				if(tmpVertPointer >= sizeof(tmpVert) / sizeof(float))
					goto parse_err_found;

				tmpVert[tmpVertPointer++] = flt;
			}

			if(vertexType != 2 && tmpVertPointer != 3 || vertexType == 2 && tmpVertPointer < 2)
				goto parse_err_found;


			if(vertexType == 0)
				vertices.push_back(*(Point*)tmpVert);
			else if (vertexType == 1)
				normals.push_back(*(Vector*)tmpVert);
			else
				texCoords.push_back(*(float2*)tmpVert);

			break;

		case 'f':
			cmdString++;
			if(tolower(*cmdString) == 'o')
				cmdString++;
			skipWS(cmdString);

			tmpIdxPtr = 0;
			for(;;)
			{
				if(tmpIdxPtr + 3 >= sizeof(tmpIdx) / sizeof(int))
					goto parse_err_found;

				char *newCmdString;
				int idx = strtol(cmdString, &newCmdString, 10);

				if(cmdString == newCmdString)
					goto parse_err_found;

				cmdString = newCmdString;

				tmpIdx[tmpIdxPtr++] = idx - 1;

				skipWS(cmdString);

				if(*cmdString == '/')
				{
					cmdString++;

					skipWS(cmdString);
					if(*cmdString != '/')
					{
						idx = strtol(cmdString, &newCmdString, 10);

						if(cmdString == newCmdString)
							goto parse_err_found;

						cmdString = newCmdString;

						tmpIdx[tmpIdxPtr++] = idx - 1;
					}
					else
						tmpIdx[tmpIdxPtr++] = -1;


					skipWS(cmdString);
					if(*cmdString == '/')
					{
						cmdString++;
						skipWS(cmdString);
						idx = strtol(cmdString, &newCmdString, 10);

						//Do ahead lookup of one number
						skipWS((const char * &)newCmdString);
						if(isdigit(*newCmdString) || (*newCmdString == 0 || *newCmdString == '#') && cmdString != newCmdString)
						{
							if(cmdString == newCmdString)
								goto parse_err_found;

							cmdString = newCmdString;

							tmpIdx[tmpIdxPtr++] = idx - 1;
						}
						else
							tmpIdx[tmpIdxPtr++] = -1;
					}
					else
						tmpIdx[tmpIdxPtr++] = -1;
				}
				else
				{
					tmpIdx[tmpIdxPtr++] = -1;
					tmpIdx[tmpIdxPtr++] = -1;
				}

				skipWS(cmdString);
				if(*cmdString == 0)
					break;
			}

			if(tmpIdxPtr <= 6)
				goto parse_err_found;

			for(int idx = 3; idx < tmpIdxPtr - 3; idx += 3)
			{
				Face t(this);
				t.material = curMat;
				memcpy(&t.vert1, tmpIdx, 3 * sizeof(size_t));
				memcpy(&t.vert2, tmpIdx + idx, 6 * sizeof(size_t));

				faces.push_back(t);
			}
			break;

		case 'o':
		case 'g':
		case 's': //?
		case '#':
			//Not supported
			break;

		default:
			if(_strnicmp(cmdString, "usemtl", 6) == 0)
			{
				cmdString += 6;
				skipWS(cmdString);
				std::string name = endSpaceTrimmed(cmdString);
				if(name.empty())
					goto parse_err_found;

				if(materialMap.find(name) == materialMap.end())
				{
					materials.push_back(Material(name));
					materialMap[name] = materials.size() - 1;
				}

				curMat = materialMap[name];
			}
			else if(_strnicmp(cmdString, "mtllib", 6) == 0)
			{
				cmdString += 6;
				skipWS(cmdString);
				std::string name = endSpaceTrimmed(cmdString);
				if(name.empty())
					goto parse_err_found;

				matFiles.push_back(name);
			}
			else
			{
				std::cerr << "Unknown entity at line " << curLine << std::endl;
			}
		}

		continue;
parse_err_found:
		std::cerr << "Error at line " << curLine << std::endl;
	}


	std::string objDir = getDirName(_fileName);

	for(std::vector<std::string>::const_iterator it = matFiles.begin(); it != matFiles.end(); it++)
	{
		std::string mtlFileName = objDir + _PATH_SEPARATOR + *it;

		readMtlLib(mtlFileName, materials, materialMap);
	}

	if(_createDefautShaders)
	{
		for(t_materialVector::iterator it = materials.begin(); it != materials.end(); it++)
		{
            //TODO: transparency
		    if (it->bumpTexture.data() != NULL) {
                SmartPtr<TexturedBumpPhongShader> shader = new TexturedBumpPhongShader; // bump shader

                it->shader = shader;

                shader->bumpTexture = it->bumpTexture;
                shader->bumpIntensity = it->bumpIntensity;

                shader->diffuseCoef = it->diffuseCoeff;
                shader->ambientCoef = it->ambientCoeff;
                shader->specularCoef = it->specularCoeff;
                shader->specularExponent = it->specularExp;
                shader->diffTexture = it->diffuseTexture;
                shader->specTexture = it->specularTexture;
                shader->ambientTexture = it->ambientTexture;

                std::cout << "Initialized Bump Texture Shader for " <<  it->name << " with intensity: " << it->bumpIntensity << std::endl;
		    } else {
                SmartPtr<TexturedPhongShader> shader = new TexturedPhongShader; // default shader

                it->shader = shader;

                shader->diffuseCoef = it->diffuseCoeff;
                shader->ambientCoef = it->ambientCoeff;
                shader->specularCoef = it->specularCoeff;
                shader->specularExponent = it->specularExp;
                shader->diffTexture = it->diffuseTexture;
                shader->specTexture = it->specularTexture;
                shader->ambientTexture = it->ambientTexture;

                std::cout << "Initialized Texture Shader for " <<  it->name << std::endl;

		    }

		}

	}

	for(t_faceVector::iterator it = faces.begin(); it != faces.end(); it++)
	{
		if(it->norm1 == -1 || it->norm2 == -1 || it->norm3 == -1)
		{
			Vector e1 = vertices[it->vert2] - vertices[it->vert1];
			Vector e2 = vertices[it->vert3] - vertices[it->vert1];
			Vector n = ~(e1 % e2);
			if(it->norm1 == -1) it->norm1 = normals.size();
			if(it->norm2 == -1) it->norm2 = normals.size();
			if(it->norm3 == -1) it->norm3 = normals.size();

			normals.push_back(n);
		}
	}
}
