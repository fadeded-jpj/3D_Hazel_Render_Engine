#include "hzpch.h"
#include "MeshImporter.h"

#include "assimp/scene.h"

namespace Engine
{
	namespace
	{
		glm::mat4 ConvertAssimpMat(const aiMatrix4x4& matrix)
		{
			glm::mat4 result(1.0f);
			result[0] = { matrix.a1, matrix.b1, matrix.c1, matrix.d1 };
			result[1] = { matrix.a2, matrix.b2, matrix.c2, matrix.d2 };
			result[2] = { matrix.a3, matrix.b3, matrix.c3, matrix.d3 };
			result[3] = { matrix.a4, matrix.b4, matrix.c4, matrix.d4 };
			return result;
		}
	}

	ImportedMeshData MeshImporter::ConvertAiMeshToImportedMeshData(aiMesh* mesh, const Ref<Skeleton>& skeleton)
	{
		ImportedMeshData data;
		data.Name = mesh->mName.C_Str();
		data.Vertices.reserve(mesh->mNumVertices);

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			MeshVertex vertex;
			vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
			vertex.normal = mesh->HasNormals()
				? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
				: glm::vec3(0.0f);
			vertex.texCoord = mesh->HasTextureCoords(0)
				? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
				: glm::vec2(0.0f);
			
			vertex.boneIDs = glm::ivec4(-1);
			vertex.boneWeights = glm::vec4(0.0f);


			if (mesh->HasTangentsAndBitangents())
			{
				glm::vec3 tangent = {
					mesh->mTangents[i].x,
					mesh->mTangents[i].y,
					mesh->mTangents[i].z
				};

				glm::vec3 bitangent = {
					mesh->mBitangents[i].x,
					mesh->mBitangents[i].y,
					mesh->mBitangents[i].z
				};

				float handedness =
					glm::dot(glm::cross(vertex.normal, tangent), bitangent) < 0.0f
					? -1.0f : 1.0f;

				vertex.tangent = glm::vec4(tangent, handedness);
			}
			else
			{
				vertex.tangent = glm::vec4(0.0f);
			}

			data.Vertices.push_back(vertex);
		}

		if (skeleton && mesh->HasBones())
		{
			auto getOrCreateBoneIndex = [&](const aiBone* bone)
				{
					const std::string boneName = bone->mName.C_Str();
					auto it = skeleton->BoneNameToIndex.find(boneName);
					if (it != skeleton->BoneNameToIndex.end())
						return it->second;

					const auto boneIndex = static_cast<uint32_t>(skeleton->Bones.size());
					BoneInfo info;
					info.Name = boneName;
					info.OffsetMatrix = ConvertAssimpMat(bone->mOffsetMatrix);
					skeleton->Bones.push_back(info);
					skeleton->BoneNameToIndex[boneName] = boneIndex;
					return boneIndex;
				};

			for (uint32_t i = 0; i < mesh->mNumBones; i++)
			{
				const auto* bone = mesh->mBones[i];
				const auto globalIndex = getOrCreateBoneIndex(bone);
				for (uint32_t j = 0; j < bone->mNumWeights; j++)
				{
					const auto& weight = bone->mWeights[j];
					for (int k = 0; k < 4; k++)
					{
						auto& vertex = data.Vertices[weight.mVertexId];
						if (vertex.boneIDs[k] >= 0)
							continue;
						vertex.boneIDs[k] = globalIndex;
						vertex.boneWeights[k] = weight.mWeight;
						break;
					}
				}
			}
		}

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			const aiFace& face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
				data.Indices.push_back(face.mIndices[j]);
		}

		return data;
	}
}
