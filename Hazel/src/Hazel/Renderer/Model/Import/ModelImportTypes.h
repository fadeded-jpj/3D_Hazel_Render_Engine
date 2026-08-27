#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hazel/Animation/Skeleton.h"
#include "Hazel/Renderer/Geometry/MeshTypes.h"
#include "Hazel/Renderer/Material/MaterialType.h"
#include "Hazel/Renderer/Model/ModelTypes.h"
#include "Hazel/Renderer/RenderState.h"
#include "Hazel/Renderer/RHI/Texture.h"

namespace Engine
{
	struct ImportedMeshData
	{
		std::string Name;
		std::vector<MeshVertex> Vertices;
		std::vector<unsigned int> Indices;
		unsigned int MaterialIndex = 0;
	};

	struct ImportedMaterialCommon
	{
		std::string Name;
		BlendMode Blend = BlendMode::Opaque;
		CullMode Cull = CullMode::None;
		
		float AlphaCutoff = 0.5f;
		bool DoubleSided = false;
		bool Unlit = false;
	};
	enum class SphereMapMode : uint8_t
	{
		Disabled = 0,
		Multiply = 1,
		Additive = 2,
		SubTexture = 3
	};

	enum class TextureComponent : uint8_t
	{
		None = 0,

		R = 1 << 0,
		G = 1 << 1,
		B = 1 << 2,
		A = 1 << 3,

		RG = (1 << 0) | (1 << 1),
		GB = (1 << 1) | (1 << 2),
		RGB = (1 << 0) | (1 << 1) | (1 << 2),
		RGBA = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)
	};

	// 表示图片如何加载
	struct ImportedTextureSlot
	{
		std::filesystem::path Path;
		std::optional<AssetPath> ObjectPath;
		unsigned int UVSet = 0;
		TextureColorSpace ColorSpace = TextureColorSpace::Linear;
		TextureComponent Components = TextureComponent::None;	// 记录有效通道
		bool Valid = false;
	};

	struct ImportedSurfaceMaterialDesc
	{
		glm::vec4 BaseColorFactor{ 1.0f };
		glm::vec3 EmissiveFactor{ 0.0f };
		float EmissiveStrength = 1.0f;
		float Opacity = 1.0f;
		float NormalScale = 1.0f;

		ImportedTextureSlot BaseColorTexture;
		ImportedTextureSlot NormalTexture;
		ImportedTextureSlot EmissiveTexture;
		ImportedTextureSlot OpacityTexture;
	};

	struct ImportedPBRMaterialDesc
	{
		float MetallicFactor = 1.0f;
		float RoughnessFactor = 1.0f;
		float SSRStrength = 0.0f;

		float OcclusionStrength = 1.0f;

		glm::vec3 SpecularColorFactor{ 1.0f };
		float SpecularFactor = 1.0f;
		ImportedTextureSlot MetallicRoughnessTexture;
		ImportedTextureSlot OcclusionTexture;
		ImportedTextureSlot SpecularColorTexture;
	};

	// TODO: 现在只留了接口，日后完善这个
	struct ImportedToonMaterialDesc
	{
		ToonMaterialRole Role = ToonMaterialRole::Default;
		glm::vec3 ShadowTint{ 0.55f };
		float Threshold = 0.5f;
		float Softness = 0.03f;
		float LitLevel = 0.6f;
		float ShadowLevel = 0.2f;

		glm::vec3 RimColor = glm::vec3(1.0f);
		float RimIntensity = 0.1f;
		float RimPower = 8.0f;
		float RimLightMask = 0.3f;

		// Toon Face shadow
		float FaceShadowOffset = 0.0f;
		float FaceShadowSoftness = 0.02f;
		float FaceShadowStrength = 1.0f;

		// sphereMap mode
		SphereMapMode SphereMode = SphereMapMode::Disabled;

		// edge setting
		bool EdgeEnabled = false;
		glm::vec4 EdgeColor = glm::vec4(0.0f);
		float EdgeSize = 0.0f;

		ImportedTextureSlot ToonRampTexture;
		ImportedTextureSlot ShadeMaskTexture;
		ImportedTextureSlot HairHighlightMask;
		ImportedTextureSlot RimMaskTexture;
		ImportedTextureSlot MatCapTexture;
		ImportedTextureSlot FaceShadowMap;
		ImportedTextureSlot SphereMap;
	};


	struct ImportedMaterialDesc
	{
		ImportedMaterialCommon Common;
		ImportedSurfaceMaterialDesc Surface;
		std::optional<ImportedPBRMaterialDesc> PBR;
		std::optional<ImportedToonMaterialDesc> Toon;
	};

	struct ModelImportData
	{
		std::filesystem::path filepath;
		std::vector<ImportedMeshData> meshes;
		std::vector<ImportedMaterialDesc> materials;
		ModelNode rootNode;
		Ref<Skeleton> skeleton;
		glm::mat4 GlobalInverseTransform = glm::mat4(1.0f);
	};
}
