#pragma once

#include "Hazel/Renderer/Model/Import/IModelImporter.h"
#include "Hazel/Renderer/Model/Import/PMX/PMXMaterialReader.h"

struct aiMaterial;
struct aiScene;
struct aiString;

namespace Engine
{

	class PMXModelImporter : public IModelImporter
	{
	public:
		virtual ModelImportData ImportFromFile(const std::filesystem::path& filepath) override;

	private:

		
	private:
		//ImportedMaterialDesc ReadMaterialDesc(const aiMaterial* material, const std::string& filepath);
		//Ref<Material> BuildMaterialFromDesc(const ImportedMaterialDesc& desc, Ref<Shader> shader, std::unordered_map<std::filesystem::path, Ref<Texture2D>>& textureCache);
		void ApplyPMXMaterialData(const std::filesystem::path& path,
			const PMXMaterialImportData& pmxMaterials, std::vector<ImportedMaterialDesc>& materials);
	private:
		//std::vector<Ref<Material>> processMaterial(Ref<Shader> shader, const aiScene* scene, const std::string& filepath);
		
	};

	class AssimpModelImporter : public IModelImporter
	{
	public:
		virtual ModelImportData ImportFromFile(const std::filesystem::path& filepath) override;
	private:
		std::vector<ImportedMaterialDesc> processMaterial(const aiScene* scene, const std::filesystem::path& filepath);
		std::vector<ImportedMeshData> processMesh(const aiScene* scene, size_t materialSize);

		// material Reader
		// base material
		ImportedMaterialDesc ReadBaseMaterialDesc(const aiMaterial* material, const std::filesystem::path& filepath);
		
		// Attach PBR desc
		ImportedPBRMaterialDesc ReadPBRMaterialDesc(const aiMaterial* material, const std::filesystem::path& filepath);

		// Attach Toon desc
		ImportedToonMaterialDesc ReadToonMaterialDesc(const aiMaterial* material, const std::filesystem::path& filepath);

		std::filesystem::path getTexturePath(const std::filesystem::path& filepath, const aiString& texturePath);
		ImportedTextureSlot GetTextureSlot(const std::filesystem::path& filepath, const aiString& texturePath);
		
	};
}
