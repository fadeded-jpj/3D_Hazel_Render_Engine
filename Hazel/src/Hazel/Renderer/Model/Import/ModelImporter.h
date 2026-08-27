#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "Hazel/Core/Core.h"
#include "Hazel/AssetsSystem/AssetsManagerType.h"
#include "Hazel/Renderer/Model/Import/ModelImportTypes.h"


namespace Engine
{

	class Material;
	class MaterialInstance;
	class Model;
	class Texture2D;

	enum class ModelType
	{
		PMX, OBJ, FBX, GLTF,Default
	};

	class ModelImporter
	{
	public:
		static Ref<Model> ImportFromFile(const std::filesystem::path& filepath, ModelType type = ModelType::Default);
		static Ref<Model> ImportFromFile(
			const std::filesystem::path& filepath,
			const ModelImportSettings& settings,
			ModelType type = ModelType::Default);
	private:
		

	private:
		static ModelImportData Dispatch(const std::filesystem::path& filepath, ModelType type);
		static Ref<Model> BuildModel(ModelImportData data, const ModelImportSettings& settings,
			MaterialImportMode materialMode);
		static Ref<MaterialInstance> BuildMaterial(const ImportedMaterialDesc& data,
			const Ref<Material>& material, MaterialImportMode materialMode);
		static Ref<MaterialInstance> BuildSurfaceMaterial(const ImportedMaterialCommon& common, const ImportedSurfaceMaterialDesc& surface,
			const Ref<Material>& material);
		static Ref<MaterialInstance> BuildPBRMaterial(const ImportedPBRMaterialDesc& payload,
			const Ref<MaterialInstance>& material);

		static Ref<MaterialInstance> BuildToonMaterial(const ImportedToonMaterialDesc& payload,
			const Ref<MaterialInstance>& material);
		
		static SubMesh BuildSubMesh(const ImportedMeshData& data);
		static void SetTexture(const Ref<MaterialInstance>& instance, const std::string& name, const ImportedTextureSlot& slot);
	};
}
