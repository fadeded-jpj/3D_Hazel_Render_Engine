#include "hzpch.h"
#include "Importers.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/GltfMaterial.h"

#include "Hazel/Animation/PMXHelp/PMXImporter.h"
#include "Hazel/Renderer/Model/Import/MeshImporter.h"
#include "Hazel/Renderer/RHI/Texture.h"
#include "Hazel/Renderer/Resources/ResourceType.h"
#include "Hazel/AssetsSystem/AssetManager.h"
#include "Hazel/AssetsSystem/AssetFileSystem.h"



namespace Engine
{
	namespace
	{
		glm::mat4 ConvertAssimpMat(const aiMatrix4x4& matrix)
		{
			glm::mat4 result(1.0f);

			result[0][0] = matrix.a1;
			result[1][0] = matrix.a2;
			result[2][0] = matrix.a3;
			result[3][0] = matrix.a4;

			result[0][1] = matrix.b1;
			result[1][1] = matrix.b2;
			result[2][1] = matrix.b3;
			result[3][1] = matrix.b4;

			result[0][2] = matrix.c1;
			result[1][2] = matrix.c2;
			result[2][2] = matrix.c3;
			result[3][2] = matrix.c4;

			result[0][3] = matrix.d1;
			result[1][3] = matrix.d2;
			result[2][3] = matrix.d3;
			result[3][3] = matrix.d4;

			return result;
		}

		ImportedTextureSlot MakeTextureSlot(
			const std::filesystem::path& physicalPath,
			TextureColorSpace colorSapce,
			TextureComponent components =
			TextureComponent::RGBA)
		{
			ImportedTextureSlot slot;
			TextureImportSettings settings;
			settings.ColorSpace = colorSapce;
			settings.Usage = TextureUsage::Data;

			if (physicalPath.empty())
				return slot;

			slot.Path = physicalPath.lexically_normal();
			slot.ColorSpace = colorSapce;
			slot.Components = components;

			std::error_code error;
			slot.Valid =
				std::filesystem::exists(slot.Path, error) &&
				!error;

			if (!slot.Valid)
			{
				HZ_CORE_WARN(
					"Imported texture does not exist: {0}",
					slot.Path.string());

				return slot;
			}

			auto assetPath =
				AssetFileSystem::TryMakeAssetPath(slot.Path);

			if (assetPath)
			{
				slot.ObjectPath = *assetPath;

				AssetManager::
					UpdateImportSettings<AssetType::Texture2D>(
						settings,
						*assetPath,
						SettingModify::Auto);
			}

			return slot;
		}
	}


	ModelImportData PMXModelImporter::ImportFromFile(const std::filesystem::path& filepath)
	{
		// 读入模型
		ModelImportData model = AssimpModelImporter().ImportFromFile(filepath);

		// 读入骨骼
		model.skeleton = PMXSkeletonImporter::ImportFromFile(filepath);
		
		// 读入pmx 的材质
		PMXMaterialImportData pmxMaterial = PMXMaterialReader::Read(filepath);
		ApplyPMXMaterialData(filepath, pmxMaterial, model.materials);

		Assimp::Importer importer;
		auto scene = importer.ReadFile(filepath.generic_u8string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (!scene || !scene->HasMeshes())
		{
			HZ_CORE_ASSERT(false, "Failed to load mesh from file: {0} !", filepath.string());
			return model;
		}

		// 将骨骼与 Mesh 绑定
		std::vector<ImportedMeshData> meshData;
		for (uint32_t i = 0; i < scene->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[i];

			auto data = MeshImporter::ConvertAiMeshToImportedMeshData(mesh, model.skeleton);
			data.MaterialIndex = mesh->mMaterialIndex < model.materials.size() ? mesh->mMaterialIndex : 0;

			meshData.push_back(data);
		}
		model.meshes = meshData;
		
		return model;
	}

	void PMXModelImporter::ApplyPMXMaterialData(const std::filesystem::path& path, const PMXMaterialImportData& pmx, std::vector<ImportedMaterialDesc>& materials)
	{
		size_t count = std::min(materials.size(), pmx.Materials.size());

		for (auto i = 0; i < count; i++)
		{
			if (!materials[i].Toon)
				materials[i].Toon.emplace();
			auto& toon = *materials[i].Toon;

			const auto& source = pmx.Materials[i];

			if (!source.Toon.Shared && source.Toon.TextureIndex >= 0
				&& source.Toon.TextureIndex < pmx.Textures.size())
			{
				const auto texturePath = path.parent_path() / pmx.Textures[source.Toon.TextureIndex];
				toon.ToonRampTexture = MakeTextureSlot(texturePath, TextureColorSpace::SRGB);
			}

			// 后续继续合并 Sphere、Edge 和 DrawingFlags。

			// sphere
			if (source.SphereMode != 0 && source.SphereTextureIndex >= 0 &&
				static_cast<size_t>(source.SphereTextureIndex) < pmx.Textures.size())
			{
				const auto texturePath = path.parent_path() / pmx.Textures[source.SphereTextureIndex];
				toon.SphereMap = MakeTextureSlot(texturePath, TextureColorSpace::Linear);
				toon.SphereMode = static_cast<SphereMapMode>(source.SphereMode);
			}

			// edge
			if ((source.DrawingFlags & 0x10) != 0)
			{
				toon.EdgeEnabled = true;
				toon.EdgeColor = source.EdgeColor;
				toon.EdgeSize = source.EdgeSize;
			}
		}
	}

	ImportedMaterialDesc AssimpModelImporter::ReadBaseMaterialDesc(const aiMaterial* material, const std::filesystem::path& filepath)
	{
		ImportedMaterialDesc desc;
		ImportedSurfaceMaterialDesc surface;

		auto readTextureSlot = [&](aiTextureType type, ImportedTextureSlot& slot, const TextureImportSettings& inferred)->bool
			{
				if (material->GetTextureCount(type) == 0)
					return false;
				aiString texturePath;
				unsigned int uvIndex = 0;
				if (material->GetTexture(type, 0, &texturePath, nullptr, &uvIndex) != AI_SUCCESS)
					return false;
				slot.Path = AssimpModelImporter::getTexturePath(filepath, texturePath);
				slot.UVSet = uvIndex;
				slot.Valid = !slot.Path.empty();
				slot.ColorSpace = TextureColorSpace::Linear;

				if (slot.Valid)
				{
					auto virtualPath = AssetFileSystem::TryMakeAssetPath(slot.Path);
					if (virtualPath)
					{
						slot.ObjectPath = *virtualPath;
						AssetManager::UpdateImportSettings<AssetType::Texture2D>(inferred, *virtualPath, SettingModify::Auto);
					}
					else
						HZ_CORE_WARN("virtual path not existed: {0}", slot.Path.string());
				}

				return slot.Valid;
			};

		aiString name;
		if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
			desc.Common.Name = name.C_Str();

		int twoSided = 0;
		if (material->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
			desc.Common.DoubleSided = twoSided != 0;

		desc.Common.Cull = desc.Common.DoubleSided ? CullMode::None : CullMode::Back;

		aiString alphaMode;
		const bool hasExplicitAlphaMode =
			material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS;
		if (hasExplicitAlphaMode)
		{
			const std::string mode = alphaMode.C_Str();
			if (mode == "BLEND")
				desc.Common.Blend = BlendMode::AlphaBlend;
			else if (mode == "MASK")
				desc.Common.Blend = BlendMode::AlphaCutout;
			else
				desc.Common.Blend = BlendMode::Opaque;
		}

		material->Get( AI_MATKEY_GLTF_ALPHACUTOFF, desc.Common.AlphaCutoff);

		TextureImportSettings baseColorSettings;
		baseColorSettings.Usage = TextureUsage::BaseColor;
		baseColorSettings.ColorSpace = TextureColorSpace::SRGB;

		aiColor4D baseColor;
		if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS)
			surface.BaseColorFactor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
		else if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS)
			surface.BaseColorFactor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);

		float opacity = 1.0f;
		if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
			surface.BaseColorFactor.a = opacity;

		if (!hasExplicitAlphaMode && surface.BaseColorFactor.a < 0.999f)
			desc.Common.Blend = BlendMode::AlphaBlend;

		if (!readTextureSlot(aiTextureType_BASE_COLOR, surface.BaseColorTexture, baseColorSettings))
			readTextureSlot(aiTextureType_DIFFUSE, surface.BaseColorTexture, baseColorSettings);

		aiColor4D emissive;
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emissive) == AI_SUCCESS)
			surface.EmissiveFactor = glm::vec3(emissive.r, emissive.g, emissive.b);
		material->Get(AI_MATKEY_EMISSIVE_INTENSITY, surface.EmissiveStrength);

		TextureImportSettings normalSettings;
		normalSettings.Usage = TextureUsage::Normal;
		normalSettings.ColorSpace = TextureColorSpace::Linear;
		readTextureSlot(aiTextureType_NORMALS, surface.NormalTexture, normalSettings);

		if (!readTextureSlot(aiTextureType_EMISSION_COLOR, surface.EmissiveTexture, baseColorSettings))
			readTextureSlot(aiTextureType_EMISSIVE, surface.EmissiveTexture, baseColorSettings);

		desc.Surface = std::move(surface);

		return desc;
	}

	ImportedPBRMaterialDesc AssimpModelImporter::ReadPBRMaterialDesc(const aiMaterial* material, const std::filesystem::path& filepath)
	{
		ImportedPBRMaterialDesc pbr;

		auto readTextureSlot = [&](aiTextureType type, ImportedTextureSlot& slot, const TextureImportSettings& inferred)->bool
			{
				if (material->GetTextureCount(type) == 0)
					return false;
				aiString texturePath;
				unsigned int uvIndex = 0;
				if (material->GetTexture(type, 0, &texturePath, nullptr, &uvIndex) != AI_SUCCESS)
					return false;
				slot.Path = AssimpModelImporter::getTexturePath(filepath, texturePath);
				slot.UVSet = uvIndex;
				slot.Valid = !slot.Path.empty();
				slot.ColorSpace = TextureColorSpace::Linear;

				if (slot.Valid)
				{
					auto virtualPath = AssetFileSystem::TryMakeAssetPath(slot.Path);
					if (virtualPath)
					{
						slot.ObjectPath = *virtualPath;
						AssetManager::UpdateImportSettings<AssetType::Texture2D>(inferred, *virtualPath, SettingModify::Auto);
					}
					else
						HZ_CORE_WARN("virtual path not existed: {0}", slot.Path.string());
				}

				return slot.Valid;
			};



		material->Get(AI_MATKEY_METALLIC_FACTOR, pbr.MetallicFactor);
		material->Get(AI_MATKEY_ROUGHNESS_FACTOR, pbr.RoughnessFactor);


		aiColor4D specular;
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular) == AI_SUCCESS)
			pbr.SpecularColorFactor = { specular.r,specular.g,specular.b };
		material->Get(AI_MATKEY_SPECULAR_FACTOR, pbr.SpecularFactor);

		TextureImportSettings dataSettings;
		dataSettings.Usage = TextureUsage::Data;
		dataSettings.ColorSpace = TextureColorSpace::Linear;
		readTextureSlot(aiTextureType_GLTF_METALLIC_ROUGHNESS, pbr.MetallicRoughnessTexture, dataSettings);
		readTextureSlot(aiTextureType_AMBIENT_OCCLUSION, pbr.OcclusionTexture, dataSettings);

		readTextureSlot(aiTextureType_SPECULAR, pbr.SpecularColorTexture, dataSettings);

		return pbr;
	}

	ImportedToonMaterialDesc AssimpModelImporter::ReadToonMaterialDesc(const aiMaterial* material, const std::filesystem::path& filepath)
	{
		ImportedToonMaterialDesc toon;

		auto readTextureSlot = [&](aiTextureType type, ImportedTextureSlot& slot, const TextureImportSettings& inferred)->bool
			{
				if (material->GetTextureCount(type) == 0)
					return false;
				aiString texturePath;
				unsigned int uvIndex = 0;
				if (material->GetTexture(type, 0, &texturePath, nullptr, &uvIndex) != AI_SUCCESS)
					return false;
				slot.Path = AssimpModelImporter::getTexturePath(filepath, texturePath);
				slot.UVSet = uvIndex;
				slot.Valid = !slot.Path.empty();
				slot.ColorSpace = TextureColorSpace::Linear;

				if (slot.Valid)
				{
					auto virtualPath = AssetFileSystem::TryMakeAssetPath(slot.Path);
					if (virtualPath)
					{
						slot.ObjectPath = *virtualPath;
						AssetManager::UpdateImportSettings<AssetType::Texture2D>(inferred, *virtualPath, SettingModify::Auto);
					}
					else
						HZ_CORE_WARN("virtual path not existed: {0}", slot.Path.string());
				}

				return slot.Valid;
			};

		TextureImportSettings colorSettings;
		colorSettings.Usage = TextureUsage::BaseColor;
		colorSettings.ColorSpace = TextureColorSpace::SRGB;

		TextureImportSettings dataSettings;
		dataSettings.Usage = TextureUsage::Data;
		dataSettings.ColorSpace = TextureColorSpace::Linear;

		if (material->GetTextureCount(aiTextureType_LIGHTMAP) > 0)
			readTextureSlot(aiTextureType_LIGHTMAP, toon.ShadeMaskTexture, dataSettings);

		if (material->GetTextureCount(aiTextureType_REFLECTION) > 0)
			readTextureSlot( aiTextureType_REFLECTION, toon.MatCapTexture, colorSettings);


		return toon;
	}

	std::filesystem::path AssimpModelImporter::getTexturePath(const std::filesystem::path& filepath, const aiString& texturePath)
	{
		std::filesystem::path modelPath(filepath);
		std::filesystem::path modelDirectory = modelPath.parent_path();
		std::filesystem::path relativePath = std::filesystem::u8path(texturePath.C_Str());
		std::filesystem::path fullPath = modelDirectory / relativePath;
		return fullPath;
	}

	ImportedTextureSlot AssimpModelImporter::GetTextureSlot(const std::filesystem::path& filepath, const aiString& texturePath)
	{
		ImportedTextureSlot res;
		res.Path = getTexturePath(filepath, texturePath);
		res.Valid = true;
		return res;
	}
	
	ModelImportData AssimpModelImporter::ImportFromFile(const std::filesystem::path& filepath)
	{
		ModelImportData model;
		model.filepath = filepath;
		model.skeleton = nullptr;

		Assimp::Importer importer;

		auto scene = importer.ReadFile(filepath.generic_u8string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || !scene->HasMeshes())
		{
			HZ_CORE_ASSERT(false, "Failed to load mesh from file: {0} !", filepath.string());
			return model;
		}

		model.materials = processMaterial(scene, filepath);
		model.meshes = processMesh(scene, model.materials.size());

		// build submesh hierarchy
		std::function<ModelNode(const aiNode*)> processNode = [&](const aiNode* node)
			{
				ModelNode res;
				res.Name = node->mName.C_Str();
				res.LocalTransform = ConvertAssimpMat(node->mTransformation);

				res.MeshIndices.reserve(node->mNumMeshes);
				for (uint32_t i = 0; i < node->mNumMeshes; i++)
					res.MeshIndices.push_back(node->mMeshes[i]);

				res.Children.reserve(node->mNumChildren);
				for (uint32_t i = 0; i < node->mNumChildren; i++)
					res.Children.push_back(processNode(node->mChildren[i]));

				return res;
			};

		model.rootNode = processNode(scene->mRootNode);
		model.GlobalInverseTransform = glm::inverse(model.rootNode.LocalTransform);

		return model;
	}

	std::vector<ImportedMaterialDesc> AssimpModelImporter::processMaterial(const aiScene* scene, const std::filesystem::path& filepath)
	{
		std::unordered_map<std::filesystem::path, Ref<Texture2D>> loadedTextures;
		std::vector<ImportedMaterialDesc> res;

		for (uint32_t i = 0; i < scene->mNumMaterials; i++)
		{
			aiMaterial* material = scene->mMaterials[i];
			ImportedMaterialDesc desc = ReadBaseMaterialDesc(material, filepath);
			desc.PBR = ReadPBRMaterialDesc(material, filepath);
			res.push_back(desc);
		}


		return res;
	}
	std::vector<ImportedMeshData> AssimpModelImporter::processMesh(const aiScene* scene, size_t materialSize)
	{
		std::vector<ImportedMeshData> meshData;
		for (uint32_t i = 0; i < scene->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[i];

			auto data = MeshImporter::ConvertAiMeshToImportedMeshData(mesh, nullptr);
			data.MaterialIndex = mesh->mMaterialIndex < materialSize ? mesh->mMaterialIndex : 0;

			meshData.push_back(data);
		}
		return meshData;
	}
}
