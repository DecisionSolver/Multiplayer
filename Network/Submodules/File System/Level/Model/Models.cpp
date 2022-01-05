#include "Models.h"
#if defined (DS_Engine)
	class Engine;
	extern shared_ptr<Engine> Application;
	#include "Engine.h"
	#include "Render System/Shader/Shaders.h"
#endif
#if !defined (DS_Engine)
	#include "File System/File_system.h"
#else
	#include "Project Manager/File System/File_system.h"
#endif
extern shared_ptr<File_system> FS;

#if defined (DS_Engine)
extern ID3D11Buffer *CreateConstBuff(D3D11_USAGE Usage, UINT CPUAccessFlags, UINT sizeofStruct);
#endif

bool Models::LoadFromFile(std::string Filename)
{	
#if defined (DS_Engine)
	importer = new Assimp::Importer;
	pScene = importer->ReadFile(Filename.c_str(),
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded
		| aiProcess_OptimizeMeshes | aiProcess_SortByPType | aiProcess_FindInvalidData
		| aiProcess_GenUVCoords | aiProcess_TransformUVCoords | aiProcess_OptimizeGraph);
	if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode || !pScene->HasMeshes())
	{
#if __has_include("logger.h")
		Logger_Error_F("Scene returns nullptr with text: {} and Scene Flags: {}", importer->GetErrorString(),
			pScene ? pScene->mFlags : 0);
#endif
		return false;
	}

	processNode(pScene->mRootNode, pScene);

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NOT_EQUAL;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	Application->getDevice()->CreateSamplerState(&sampDesc, &TexSamplerState);

	std::vector<ID3DBlob *> Buffer_blob;
	std::vector<string> FileShaders =
	{
		FS->GetFile("VertexShader.hlsl")->Path.string(),
		FS->GetFile("PixelShader.hlsl")->Path.string()
	};
	std::vector<string> Functions =
	{
		"Vertex_model_VS",
		"Pixel_model_PS"
	},
		Version =
	{
		"vs_4_0",
		"ps_4_0"
	};
	std::vector<void *> Buffers = Shaders::CompileShaderFromFile(Buffer_blob =
		Shaders::CreateShaderFromFile(FileShaders, Functions, Version));
	VS = (ID3D11VertexShader *)Buffers[0]; // VS
	PS = (ID3D11PixelShader *)Buffers[1]; // PS

	Application->getDevice()->CreateVertexShader(Buffer_blob.at(0)->GetBufferPointer(), Buffer_blob.at(0)->GetBufferSize(),
		NULL, &VS);
	Application->getDevice()->CreatePixelShader(Buffer_blob.at(1)->GetBufferPointer(), Buffer_blob.at(1)->GetBufferSize(),
		NULL, &PS);

	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	Application->getDevice()->CreateInputLayout(ied, 2, Buffer_blob.at(0)->GetBufferPointer(),
		Buffer_blob.at(0)->GetBufferSize(), &pLayout);

	pConstantBuffer = CreateConstBuff(D3D11_USAGE::D3D11_USAGE_DEFAULT, 0, sizeof(cb));
	
	//	TM->EndTime();
	//Console::LogInfo((string("\nCreate Buffers And Shaders For Model Take:" + to_string(TM->GetResultTime().count())
	//	+ string(" Seconds")).c_str()));
#endif
	return true;
}

bool Models::LoadFromAllModels()
{
#if defined (DS_Engine)
	auto Files = FS->GetFileByType(_TypeOfFile::MODELS);
	for (size_t i = 0; i < Files.size(); i++)
	{
		importer = new Assimp::Importer;

		pScene = importer->ReadFile(Files.at(i).first->Path.string().c_str(),
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded 
		| aiProcess_OptimizeMeshes | aiProcess_SortByPType | aiProcess_FindInvalidData
		| aiProcess_GenUVCoords | aiProcess_TransformUVCoords | aiProcess_OptimizeGraph);
		if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode || !pScene->HasMeshes())
		{
#if __has_include("logger.h")
			Logger_Error_F("Scene returns nullptr with text: {} and Scene Flags: {}", importer->GetErrorString(),
				pScene->mFlags);
#endif
			return false;
		}

		processNode(pScene->mRootNode, pScene);

		SAFE_DELETE(importer);
	}
#endif

	return true;
}

#if defined (DS_Engine)
void Models::Render(Matrix View, Matrix Proj)
#else
void Models::Update()
#endif
{
#if defined (DS_Engine)
	if (!Application->getDeviceContext()) return;

	//ConstantBuffer cb;
	auto Mrx = scale * position * rotate;
	cb.World = XMMatrixTranspose(Mrx);
	cb.View = XMMatrixTranspose(View);
	cb.Proj = XMMatrixTranspose(Proj);

	Application->getDeviceContext()->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
	Application->getDeviceContext()->VSSetConstantBuffers(0, 1, &pConstantBuffer);
	Application->getDeviceContext()->PSSetSamplers(0, 1, &TexSamplerState);
	Application->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Application->getDeviceContext()->IASetInputLayout(pLayout);
	Application->getDeviceContext()->VSSetShader(VS, 0, 0);
	Application->getDeviceContext()->PSSetShader(PS, 0, 0);

	for (size_t i = 0; i < meshes.size(); i++)
	{
		meshes.at(i)->Draw();
	}
#endif
}

Models::Models(std::string Filename)
{
	if (Filename.empty())
	{
#if __has_include("logger.h")
		Logger_Error_F("Model File: \"{}\" not found And Can't Be Load!", Filename);
#endif
	}
	if (!LoadFromFile(Filename))
	{
#if __has_include("logger.h")
		Logger_Error_F("Model File: \"{}\" not found And Can't Be Load!", Filename);
#endif
	}
}

void Models::Release()
{
#if defined (DS_Engine)
	while (!Textures_loaded.empty())
	{
#if defined (DS_Engine)
		SAFE_DELETE(Textures_loaded.front().TextureRes);
		SAFE_DELETE(Textures_loaded.front().TextureSHRes);
#endif
		Textures_loaded.erase(Textures_loaded.begin());
	}

	if (importer)
	{
		importer->FreeScene();
		SAFE_DELETE(importer);
	}
	SAFE_DELETE(pScene);
	SAFE_DELETE(mesh);
#endif
}

#if defined (DS_Engine)
std::vector<Texture> Models::loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName,
	const aiScene *Scene)
{
	std::vector<Texture> textures;
	string PathTexture;

	for (UINT i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (size_t j = 0; j < Textures_loaded.size(); j++)
		{
			if (contains(Textures_loaded.at(j).path, path(str.C_Str()).filename().string()))
			{
				textures.push_back(Textures_loaded.at(j));
				skip = true;
				break;
			}
		}
		if (!skip)
		{
			Texture texture;
#if defined (DS_Engine)
			if (Textype == "embedded compressed texture")
				texture.TextureSHRes = getTextureFromModel(Scene, getTextureIndex(&str));
			else
#endif
			{
				string TName = path(str.C_Str()).filename().string();
				to_lower(TName);
				auto textr = FS->GetFile(TName);
				if (textr.operator bool())
				{
					PathTexture = textr->Path.string();
					to_lower(PathTexture);
					if (FindSubStr(textr->Ext.string(), ".dds"))
					{
#if defined (DS_Engine)
						if (FAILED(CreateDDSTextureFromFile(Application->getDevice(), textr->Path.c_str(),
							&texture.TextureRes, &texture.TextureSHRes)))
							Logger_Warn_F("Something is wrong with this texture: {}", textr->FName.string());
#endif
					}
					else
					{
#if defined (DS_Engine)
						if (FAILED(CreateWICTextureFromFile(Application->getDevice(), textr->Path.c_str(),
							&texture.TextureRes, &texture.TextureSHRes)))
							Logger_Warn_F("Something is wrong with Create the texture: {}", textr->FName.string());
#endif
					}
				}
			}
			texture.type = typeName;
			texture.path = PathTexture;

			textures.push_back(texture);

			Textures_loaded.push_back(texture);
		}
	}
	return textures;
}

void Models::processNode(aiNode *node, const aiScene *Scene)
{
	for (UINT IndxMesh = 0; IndxMesh < node->mNumMeshes; IndxMesh++)
	{
		std::vector<Things> vertices;
		std::vector<UINT> indices;
		std::vector<Texture> textures;
		Vector3 Max;
		
		mesh = Scene->mMeshes[node->mMeshes[IndxMesh]];
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial *mat = Scene->mMaterials[mesh->mMaterialIndex];
			string name = string(mat->GetName().C_Str());

			if (Textype.empty())
				Textype = determineTextureType(Scene, name, mat);
		}

		for (UINT i = 0; i < mesh->mNumVertices; i++)
		{
			Things vertex;

			vertex.Pos = Vector3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			if (mesh->mTextureCoords[0])
				vertex.Tex = Vector2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

			vertices.push_back(vertex);
		}

		for (UINT i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];

			for (UINT j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial *material = Scene->mMaterials[mesh->mMaterialIndex];

			std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE,
				"texture_diffuse", Scene);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

			///It doesn't work!
			/*
			std::vector<Texture> Opacity = loadMaterialTextures(material, aiTextureType_OPACITY,
			"texture_opacity", Scene);
			textures.insert(textures.end(), Opacity.begin(), Opacity.end());

			std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR,
			"texture_specular", Scene);
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

			std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT,
			"texture_normal", Scene);
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

			std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT,
			"texture_height", Scene);
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
			*/
		}

		meshes.push_back(make_shared<Mesh>(vertices, indices, textures));
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
		processNode(node->mChildren[i], Scene);
}

aiTextureType Models::getTextureType(string TypeName)
{
	if (contains(TypeName, "diffuse"))
		return aiTextureType_DIFFUSE;
	else if (contains(TypeName, "opacity"))
		return aiTextureType_OPACITY;
	return aiTextureType_UNKNOWN;
}

string Models::determineTextureType(const aiScene *Scene, string TypeName, aiMaterial *mat)
{
	aiString textypeStr;
	mat->GetTexture(Models::getTextureType(TypeName), 0, &textypeStr);
	string textypeteststr = textypeStr.C_Str();
	if (textypeteststr == "*0" || textypeteststr == "*1" || textypeteststr == "*2"
		|| textypeteststr == "*3" || textypeteststr == "*4" || textypeteststr == "*5")
	{
		if (Scene->mTextures[0]->mHeight == 0)
			return "embedded compressed texture";
		else
			return "embedded non-compressed texture";
	}
	if (textypeteststr.find('.') != string::npos)
		return "textures are on disk";

	return "";
}

int Models::getTextureIndex(aiString *str)
{
	return stoi(string(str->C_Str()).substr(1));
}
#endif

#if defined (DS_Engine)
ID3D11ShaderResourceView *Models::getTextureFromModel(const aiScene *Scene, int Textureindex)
{
	ID3D11ShaderResourceView *texture;
	int *size = reinterpret_cast<int *>(&Scene->mTextures[Textureindex]->mWidth);

	if (FAILED(CreateWICTextureFromMemory(Application->getDevice(),
		reinterpret_cast<unsigned char*>(Scene->mTextures[Textureindex]->pcData), *size, nullptr, &texture)))
	{
		Logger_Warn_F("Something is wrong with this texture: {}", Scene->mTextures[Textureindex]->mFilename.C_Str());

		return nullptr;
	}

	return texture;
}
#endif

void Models::setRotation(Vector3 NewRotate)
{
#if defined (DS_Engine)
	rotate = Matrix::CreateRotationX(NewRotate.x) *
		Matrix::CreateRotationY(NewRotate.y) *
		Matrix::CreateRotationZ(NewRotate.z);
#else
	Rot = NewRotate;
#endif
}

void Models::setScale(Vector3 NewScale)
{
#if defined (DS_Engine)
	scale = Matrix::CreateScale(NewScale);
#else
	Scl = NewScale;
#endif
}

void Models::setPosition(Vector3 NewPos)
{
#if defined (DS_Engine)
	position = Matrix::CreateTranslation(NewPos);
#else
	Pos = NewPos;
#endif
}

void Models::Mesh::Init(std::vector<Things> Vertices, std::vector<UINT> Indices, std::vector<Texture> Textures)
{
	this->vertices = Vertices;
	this->indices = Indices;
	this->textures = Textures;

#if defined (DS_Engine)
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Things) * vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &vertices[0];

	Application->getDevice()->CreateBuffer(&vbd, &initData, &VertexBuffer);

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;

	initData.pSysMem = &indices[0];

	Application->getDevice()->CreateBuffer(&ibd, &initData, &IndexBuffer);
#endif
}

void Models::Mesh::Draw()
{
#if defined (DS_Engine)
	UINT stride = sizeof(Things);
	UINT offset = 0;

	Application->getDeviceContext()->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	Application->getDeviceContext()->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	if (!textures.empty() && textures[0].TextureSHRes)
		Application->getDeviceContext()->PSSetShaderResources(0, 1, &textures[0].TextureSHRes);

	if (Application->IsWireFrame())
		Application->getDeviceContext()->RSSetState(Application->GetWireFrame());
	else
		Application->getDeviceContext()->RSSetState(Application->GetNormalFrame());

	Application->getDeviceContext()->DrawIndexed(indices.size(), 0, 0);
#endif
}
