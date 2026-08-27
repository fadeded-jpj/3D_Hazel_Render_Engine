#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Hazel/Renderer/Geometry/Mesh.h"
#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/Model/Model.h"
#include "Hazel/Animation/Animator.h"
#include "Hazel/Renderer/RHI/BoneMatrixBuffer.h"
#include "Hazel/Renderer/Material/MaterialSystem.h"
#include "Hazel/Renderer/Lighting/LightType.h"
#include "Hazel/Renderer/Model/Import/ModelImportTypes.h"
#include "Hazel/Renderer/RenderState.h"

namespace Engine
{

	class TransformComponent
	{
	public:
		TransformComponent() = default;

		inline glm::mat4 GetTransform() const
		{
			glm::mat4 rotation =  glm::rotate(glm::mat4(1.0f), Rotation.x, { 1.0f, 0.0f, 0.0f })
								* glm::rotate(glm::mat4(1.0f), Rotation.y, { 0.0f, 1.0f, 0.0f })
								* glm::rotate(glm::mat4(1.0f), Rotation.z, { 0.0f, 0.0f, 1.0f });
			return glm::translate(glm::mat4(1.0f), Translation) * rotation * glm::scale(glm::mat4(1.0f), Scale);
		}

		inline void SetTranslation(const glm::vec3& translation) { Translation = translation; }
		inline void SetRotation(const glm::vec3& rotation) { Rotation = rotation; }
		inline void SetScale(const glm::vec3& scale) { Scale = scale; }

		const glm::vec3& GetTranslation() const { return Translation; }
		const glm::vec3& GetRotation() const { return Rotation; }
		const glm::vec3& GetScale() const { return Scale; }
		glm::vec3 GetForward() const {
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Rotation.x, { 1.0f, 0.0f, 0.0f })
				* glm::rotate(glm::mat4(1.0f), Rotation.y, { 0.0f, 1.0f, 0.0f })
				* glm::rotate(glm::mat4(1.0f), Rotation.z, { 0.0f, 0.0f, 1.0f });
			return glm::normalize(rotation * glm::vec4(0, 0, -1, 0));
		}
	private:
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0, 1.0f, 1.0f };
	};

	class RenderComponent
	{
	public:
		static constexpr float DefaultBoundsScale = 1.0f;
		static constexpr float AnimatedBoundsScale = 2.0f;

		RenderComponent() = default;
		RenderComponent(const Ref<Model>& ModelAsset)
			: ModelAsset(ModelAsset)
		{
		}

		Ref<MaterialInstance> GetMaterial(unsigned int index) const
		{
			auto it = MaterialOverride.find(index);
			if (it != MaterialOverride.end())
			{
				return it->second;
			}
			return ModelAsset ? ModelAsset->GetMaterial(index) : nullptr;
		}

		Ref<MaterialInstance> GetOrCreateMaterialOverride(unsigned int index)
		{
			auto it = MaterialOverride.find(index);
			if (it != MaterialOverride.end())
			{
				return it->second;
			}

			auto baseMaterial = ModelAsset ? ModelAsset->GetMaterial(index) : nullptr;
			if (!baseMaterial)
				return nullptr;

			auto overrideMaterial = baseMaterial->Clone();
			MaterialOverride.emplace(index, overrideMaterial);

			return overrideMaterial;
		}

		bool SetMaterialBlendMode(unsigned int materialIndex, BlendMode blendMode)
		{
			auto overrideMaterial = GetOrCreateMaterialOverride(materialIndex);
			return MaterialSystem::SetImportedLitBlendMode(overrideMaterial, blendMode);
		}

		bool SetMaterialCullMode(unsigned int materialIndex, CullMode cullmode)
		{
			auto overrideMaterial = GetOrCreateMaterialOverride(materialIndex);
			return MaterialSystem::SetImportedLitCullMode(overrideMaterial, cullmode);
		}

		bool SetMaterialBaseAlpha(unsigned int materialIndex, float alpha)
		{
			auto overrideMaterial = GetOrCreateMaterialOverride(materialIndex);
			return MaterialSystem::SetImportedLitAlpha(overrideMaterial, alpha);
		}

		bool SetToonParmatersOverride(unsigned int materialIndex, ImportedToonMaterialDesc toon)
		{
			auto overrideMaterial = GetOrCreateMaterialOverride(materialIndex);
			return MaterialSystem::SetImportedToonParameters(overrideMaterial, toon);
		}

		bool SetMaterialCastShadow(unsigned int materialIndex, bool castShadow)
		{
			auto overrideMaterial = GetOrCreateMaterialOverride(materialIndex);
			return MaterialSystem::SetImportedLitCastShadow(overrideMaterial, castShadow);
		}

		AABB GetAABB() const
		{
			if (!ModelAsset)
				return {};

			const Bounds& bounds = ModelAsset->GetBounds();
			const float scale = BoundsScale > 0.0f ? BoundsScale : 0.0f;
			const glm::vec3 extent = bounds.BoxExtent * scale;
			return { bounds.Origin - extent, bounds.Origin + extent };
		}

		BoundingSphere GetBoundingSphere() const
		{
			if (!ModelAsset)
				return {};

			const Bounds& bounds = ModelAsset->GetBounds();
			const float scale = BoundsScale > 0.0f ? BoundsScale : 0.0f;
			return { bounds.Origin, bounds.SphereRadius * scale };
		}

		Ref<Model> ModelAsset;
		std::unordered_map<unsigned int, Ref<MaterialInstance>> MaterialOverride;
		float BoundsScale = DefaultBoundsScale;

		RenderOutlineMode OutlineMode = RenderOutlineMode::None;
	};

	class AnimationComponent
	{
	public:
		AnimationComponent(unsigned int maxBones = 512)
		{
			BoneBuffer = BoneMatrixBuffer::Create(maxBones);
			PreBoneBuffer = BoneMatrixBuffer::Create(maxBones);
		}
		Ref<Animator> AnimatorAsset;
		Ref<BoneMatrixBuffer> BoneBuffer;
		Ref<BoneMatrixBuffer> PreBoneBuffer;	// for TAA

		bool BoneHistoryValid = false;
	};


	class LightComponent
	{
	public:
		void SetLightType(LightType type) { Type = type; }

	public:
		LightType Type = LightType::Point;
		bool Enabled = true;
		bool CastShadows = false;
		bool IsMainLight = false;

		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;

		// Point / Spot 使用
		float Range = 10.f;

		// Spot 使用
		float InnerConeAngle = glm::radians(20.0f);
		float OuterConeAngle = glm::radians(30.0f);
	};
}
