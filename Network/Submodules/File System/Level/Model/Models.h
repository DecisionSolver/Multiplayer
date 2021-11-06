#pragma once
#ifndef __MODELS_H__
#define __MODELS_H__

#if __has_include("Core/pch.h")
	#include "Core/pch.h"
#else
	#include "../../Tools.h"
#endif
#if defined (DS_Engine)
#include "assimp\Importer.hpp"
#include "assimp\scene.h"
#include "assimp\postprocess.h"

#include <Inc/WICTextureLoader.h>
#include <Inc/DDSTextureLoader.h>

#include <Inc/CommonStates.h>
#include <Effects.h>

using namespace Assimp;
#endif

struct Texture
{
	std::string type, path;
#if defined (DS_Engine)
	ID3D11ShaderResourceView *TextureSHRes = nullptr;
	ID3D11Resource *TextureRes = nullptr;
#endif
};
#pragma pack(push, 1)
struct Things
{
	Vector3 Pos;
	Vector2 Tex;
};
#pragma pack()

class Models
{
private:
	class Mesh
	{
	public:
		Mesh(std::vector<Things> vertices, std::vector<UINT> indices, std::vector<Texture> textures)
		{
			Init(vertices, indices, textures);
		}
		Mesh() {}
		~Mesh() {}

		void Init(std::vector<Things> vertices, std::vector<UINT> indices, std::vector<Texture> textures);
		void Draw();

		std::vector<Things> getVertices() { return vertices; }
		std::vector<UINT> getIndices() { return indices; }
	private:
		std::vector<Things> vertices;
		std::vector<UINT> indices;
		std::vector<Texture> textures;

#if defined (DS_Engine)
		ID3D11Buffer *VertexBuffer = nullptr, *IndexBuffer = nullptr;
#endif 
	};
	std::vector<std::shared_ptr<Mesh>> meshes;

public:
	bool LoadFromFile(std::string Filename);
	bool LoadFromAllModels();

#if defined (DS_Engine)
	void Render(Matrix View, Matrix Proj);
#else
	void Update();
#endif
	Models() {}
	Models(std::string Filename);

	void Release();

	void setRotation(Vector3 NewRotate);
	void setScale(Vector3 NewScale);
	void setPosition(Vector3 NewPos);

#if defined (DS_Engine)
	Matrix getWorld() { return World; }
#endif

	std::vector<std::shared_ptr<Mesh>> getMeshes() { return meshes; }

	~Models() {}
protected:
#if defined (DS_Engine)
	Matrix World = Matrix(), position = Matrix(),
		scale = Matrix(), rotate = Matrix();
	ID3D11Buffer *pConstantBuffer = nullptr;

	ID3D11InputLayout *pLayout = nullptr;
	ID3D11SamplerState *TexSamplerState = nullptr;

	ID3D11VertexShader *VS = nullptr;
	ID3D11PixelShader *PS = nullptr;

#pragma pack(push, 1)
	struct ConstantBuffer
	{
		Matrix World = Matrix::Identity, View = Matrix::Identity, Proj = Matrix::Identity;
	} cb;
#pragma pack()

#else
	Vector3 Pos = Vector3::Zero, Scl = Vector3::One, Rot = Vector3::Zero;
#endif

	HRESULT hr = S_OK;

#if defined (DS_Engine)
	Assimp::Importer *importer = nullptr;
	const aiScene *pScene = nullptr;

	std::vector<Texture> Textures_loaded;
	std::string Textype = {};

	aiMesh *mesh = nullptr;

	void processNode(aiNode *node, const aiScene *Scene);

	std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, const aiScene *Scene);
	std::string determineTextureType(const aiScene *Scene, std::string TypeName, aiMaterial *mat);
	int getTextureIndex(aiString *str);

	static aiTextureType getTextureType(std::string TypeName);

	ID3D11ShaderResourceView *getTextureFromModel(const aiScene *Scene, int Textureindex);
#endif
};
#endif // !__MODELS_H__
