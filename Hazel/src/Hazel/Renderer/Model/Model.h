#pragma once

#include "Hazel/Animation/Skeleton.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/Model/ModelTypes.h"

#include "Hazel/AssetsSystem/AssetType.h"

namespace Engine
{
	class ModelImporter;

	class Model : public Asset
	{
	public:
		using SubMesh = Engine::SubMesh;
		using ModelNode = Engine::ModelNode;

		//static Ref<Model> Import(const std::string& filepath);
		
		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		inline const std::filesystem::path& GetFilePath() const { return m_FilePath; }
		inline Ref<Skeleton>& GetSkeleton(){ return m_Skeleton; }
		inline const ModelNode& GetRootNode() const { return m_RootNode; }
		inline const glm::mat4 GetGlobalInverseTransform() const { return m_GlobalInverseTransform; }
		inline const Bounds& GetBounds() const { return m_Bounds; }
		
		Ref<MaterialInstance> GetMaterial(uint32_t idx) const;

		ASSET_TYPE(Model);

	private:
		Model() = default;
		std::filesystem::path m_FilePath;
		std::vector<SubMesh> m_SubMeshes;
		std::vector<Ref<MaterialInstance>> m_Materials;

		ModelNode m_RootNode;

		Ref<Skeleton> m_Skeleton;
		glm::mat4 m_GlobalInverseTransform = glm::mat4(1.0f);
		Bounds m_Bounds;

		friend class ModelImporter;

	private:
	};

}
