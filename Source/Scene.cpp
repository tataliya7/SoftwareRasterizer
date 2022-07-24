#include "Scene.h"
#include "Logging.h"
#include "Texture.h"

SR_DISABLE_WARNINGS
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/Exceptional.h>
SR_ENABLE_WARNINGS

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace SR
{
	bool LoadTextureFromFile(const char* filename, Texture* texure)
	{
		SR_LOG_INFO("Import texture: {0}", filename);

		int iw = 0, ih = 0, c = 0;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(filename, &iw, &ih, &c, STBI_default);
		if (data == nullptr) 
		{
			return false;
		}
		texure->Resize(iw, ih);

		for (uint32 y = 0; y < (uint32)ih; y++)
		{
			for (uint32 x = 0; x < (uint32)iw; x++)
			{
				glm::u8vec4 pixel;
				uint32 idx = x + y * iw;

				switch (c) 
				{
				case STBI_grey:
				{
					pixel.r = data[idx];
					pixel.g = pixel.b = pixel.r;
					pixel.a = 255;
					break;
				}
				case STBI_grey_alpha:
				{
					pixel.r = data[idx * 2 + 0];
					pixel.g = pixel.b = pixel.r;
					pixel.a = data[idx * 2 + 1];
					break;
				}
				case STBI_rgb: 
				{
					pixel.r = data[idx * 3 + 0];
					pixel.g = data[idx * 3 + 1];
					pixel.b = data[idx * 3 + 2];
					pixel.a = 255;
					break;
				}
				case STBI_rgb_alpha:
				{
					pixel.r = data[idx * 4 + 0];
					pixel.g = data[idx * 4 + 1];
					pixel.b = data[idx * 4 + 2];
					pixel.a = data[idx * 4 + 3];
					break;
				}
				default: break;
				}

				texure->StoreTexel(x, y, Vector4(pixel.r / 255.0f, pixel.g / 255.0f, pixel.b / 255.0f, pixel.a / 255.0f));
			}
		}

		stbi_image_free(data);

		return true;
	}

	bool ImportGLTF2(const char* filename, Mesh* mesh)
	{
		SR_LOG_INFO("Import mesh: {0}", filename);

		static const uint32_t meshImportFlags =
			//aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
			//aiProcess_Triangulate |             // Make sure we're triangles
			//aiProcess_SortByPType |             // Split meshes by primitive type
			//aiProcess_GenNormals |              // Make sure we have legit normals
			//aiProcess_GenUVCoords |             // Convert UVs if required 
			//aiProcess_OptimizeMeshes |          // Batch draws where possible
			//aiProcess_JoinIdenticalVertices |
			//aiProcess_ValidateDataStructure |   // Validation
			//aiProcess_ConvertToLeftHanded |
			aiProcess_CalcTangentSpace | aiProcess_Triangulate;

		std::unique_ptr<Assimp::Importer> importer = std::make_unique<Assimp::Importer>(); 
		const aiScene* aiScene = importer->ReadFile(filename, meshImportFlags);
		if (!aiScene || !aiScene->HasMeshes() || aiScene->mNumMeshes != 1)
		{
			SR_LOG_INFO("Failed to load mesh file: {0}", filename);
			return false;
		}

		std::string dir = std::string(filename).substr(0, std::string(filename).find_last_of('/'));
		for (uint32 i = 0; i < aiScene->mNumMeshes; i++)
		{
			if (i > 0)
			{
				break;
			}

			const aiMesh* aiMesh = aiScene->mMeshes[i];

			uint32 materialID = aiMesh->mMaterialIndex;
			uint32 numVertices = aiMesh->mNumVertices;
			uint32 numIndices = aiMesh->mNumFaces * 3;
		
			if (!aiMesh->HasPositions() || !aiMesh->HasNormals() || !aiMesh->HasTangentsAndBitangents())
			{
				SR_LOG_ERROR("Failed to import mesh.");
				return false;
			}

			mesh->positions.resize(aiMesh->mNumVertices);
			mesh->normals.resize(aiMesh->mNumVertices);
			mesh->tangents.resize(aiMesh->mNumVertices);
			mesh->texCoords.resize(aiMesh->mNumVertices);
			for (uint32 j = 0; j < aiMesh->mNumVertices; j++)
			{
				mesh->positions[j] = Vector3(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z);
				mesh->normals[j] = Vector3(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z);
				mesh->tangents[j] = Vector3(aiMesh->mTangents[j].x, aiMesh->mTangents[j].y, aiMesh->mTangents[j].z);
				if (aiMesh->HasTextureCoords(0))
				{
					mesh->texCoords[j] = Vector2(aiMesh->mTextureCoords[0][j].x, aiMesh->mTextureCoords[0][j].y);
				}
				else
				{
					mesh->texCoords[j] = Vector2(0.0f);
				}
			}


			//mesh->indices.resize(aiMesh->mNumFaces * 3);
			mesh->primitives.resize(aiMesh->mNumFaces);
			for (uint32 j = 0; j < aiMesh->mNumFaces; j++)
			{
				ASSERT(aiMesh->mFaces[j].mNumIndices == 3);
				/*mesh->indices[j * 3 + 0] = aiMesh->mFaces[j].mIndices[0];
				mesh->indices[j * 3 + 1] = aiMesh->mFaces[j].mIndices[1];
				mesh->indices[j * 3 + 2] = aiMesh->mFaces[j].mIndices[2];*/
				mesh->primitives[j].indices[0] = aiMesh->mFaces[j].mIndices[0];
				mesh->primitives[j].indices[1] = aiMesh->mFaces[j].mIndices[1];
				mesh->primitives[j].indices[2] = aiMesh->mFaces[j].mIndices[2];
			}

			mesh->numVertices = aiMesh->mNumVertices;
			//mesh->numIndices = aiMesh->mNumFaces * 3;
			mesh->numPrimitives = aiMesh->mNumFaces;

			{
				const aiMaterial* aiMaterial = aiScene->mMaterials[aiMesh->mMaterialIndex];
				const aiString materialName = aiMaterial->GetName();
				float roughness = 1.0f;
				float metallic = 1.0f;
				Vector4 baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
				aiReturn res = aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
				res = aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
				res = aiMaterial->Get(AI_MATKEY_BASE_COLOR, baseColor);

				PBRMaterial& material = mesh->material;
				aiString aiTexPath;
				aiReturn result = aiMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &aiTexPath);
				if (result == aiReturn_SUCCESS)
				{
					material.baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
					std::string absolutePath = dir + '/' + aiTexPath.C_Str();
					material.baseColorMap = (std::shared_ptr<Texture>)new Texture();
					LoadTextureFromFile(absolutePath.c_str(), material.baseColorMap.get());
				}
				result = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath);
				if (result == aiReturn_SUCCESS)
				{
					std::string absolutePath = dir + '/' + aiTexPath.C_Str();
					material.normalMap = (std::shared_ptr<Texture>)new Texture();
					LoadTextureFromFile(absolutePath.c_str(), material.normalMap.get());
				}
				result = aiMaterial->GetTexture(aiTextureType_UNKNOWN, 0, &aiTexPath);
				if (result == aiReturn_SUCCESS)
				{
					material.metallic = 1.0f; material.roughness = 1.0f;
					std::string absolutePath = dir + '/' + aiTexPath.C_Str();
					material.metallicRoughnessMap = (std::shared_ptr<Texture>)new Texture();
					LoadTextureFromFile(absolutePath.c_str(), material.metallicRoughnessMap.get());
				}
			}
		}
		return true;
	}
}