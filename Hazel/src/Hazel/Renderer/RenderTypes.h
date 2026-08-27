#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "Hazel/Core/Core.h"
//#include "Hazel/Renderer/Material/Material.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/Model/ModelTypes.h"
#include "Hazel/Renderer/PostProcessing/PostProcessType.h"
#include "Hazel/Renderer/RenderDebug/RenderDebug.h"
#include "Hazel/Renderer/Lighting/LightType.h"
#include "Hazel/Profile/RenderProfile.h"
#include "Hazel/Scene/Entity.h"

namespace Engine
{
	class BoneMatrixBuffer;
	class Mesh;

	struct RenderSkinningData
	{
		Ref<BoneMatrixBuffer> BoneBuffer;
		Ref<BoneMatrixBuffer> PreviousBoneBuffer;
	};

	using RenderObjectID = uint64_t;
	constexpr RenderObjectID INVALID_RENDER_OBJECT_ID = 0;

	enum class RenderDomain : uint8_t
	{
		Scene = 0, Character
	};

	enum class RenderQualityPreset : uint8_t
	{
		Low = 0,
		High,
		Count
	};

	struct RenderCharacterData
	{
		bool HasHeadTransform = false;

		glm::vec3 HeadPosition{ 0.0f };
		glm::vec3 HeadForward{ 0.0f, 0.0f, -1.0f };
		glm::vec3 HeadRight{ 1.0f, 0.0f, 0.0f };
		glm::vec3 HeadUp{ 0.0f, 1.0f, 0.0f };
	};

	struct CharacterShadowBounds
	{
		bool Valid = false;
		AABB WorldAABB;

		void Reset()
		{
			Valid = false;
			WorldAABB = {};
		}

		void Encapsulate(const AABB& bounds)
		{
			if (!Valid)
			{
				WorldAABB = bounds;
				Valid = true;
				return;
			}

			WorldAABB.Min = glm::min(WorldAABB.Min, bounds.Min);
			WorldAABB.Max = glm::max(WorldAABB.Max, bounds.Max);
		}
	};

	struct SSAOSettings
	{
		bool Enabled = true;
		float Radius = 0.5f;
		float DepthBias = 0.02f;
		float Intensity = 1.0f;
	};

	struct RenderObject
	{
		RenderDomain Domain = RenderDomain::Scene; 

		Ref<Engine::Mesh> Mesh;
		Ref<Engine::Mesh> OutlineEdgeMesh;
		Ref<MaterialInstance> Material;
		glm::mat4 Transform = glm::mat4(1.0f);
		AABB WorldBounds;

		RenderSkinningData Skinning;
		RenderCharacterData Character;

		bool Visible = true;

		RenderObjectID ID = INVALID_RENDER_OBJECT_ID;
		RenderOutlineMode OutlineMode = RenderOutlineMode::None;
	};

	struct RenderView
	{
		RenderQualityPreset QualityPreset = RenderQualityPreset::High;

		glm::mat4 ViewProjection = glm::mat4(1.0f);
		glm::mat4 Projection = glm::mat4(1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::vec3 CameraPosition = glm::vec3(0.0f);

		glm::ivec2 ViewportSize = { 1280, 960 };
		float Time = 0.0f;
		RenderObjectID SelectedID = INVALID_RENDER_OBJECT_ID;
		glm::mat4 InverseViewProjection = glm::mat4(1.0f);

		// CSM
		bool CSMEnabled = true;
		float NearClip = 0.1f;
		float FarClip = 1000.0f;
		float FOV = 45.0f;
		float OverlapRatio = 0.15f;
		float CSMCasterZOffset = 2.5f;

		// Lighting model
		LightingMode LightingMode = LightingMode::PBR;
		float ToonShadowThreshold = 0.5f;
		float ToonSoftness = 0.03f;
		glm::vec3 ToonShadowTint = glm::vec3(0.5f);
		float ToonLitLeval = 0.7f;
		float ToonShadowLevel = 0.1f;

		// outline 
		glm::vec3 OutlineColor = glm::vec3(0.0f);
		float OutlineDepthThreshold = 0.01f;	// for screen space outline only
		float OutlineNormalThreshold = 0.02f;	// for screen space outline only
		float OutlineAlpha = 0.3f;				// for screen space outline only

		PostProcessSettings PostProcess;
		SSAOSettings SSAO;

		// Shadow map resolution
		static constexpr unsigned int ShadowMapResolution = 1024;

		// TAA main render.
		bool TAAEnabled = true;
		mutable glm::mat4 JitteredViewProjection = glm::mat4(1.0f);
		mutable glm::mat4 InverseJitteredViewProjection = glm::mat4(1.0f);
		mutable glm::mat4 PreviousViewProjection = glm::mat4(1.0f);
		mutable glm::mat4 PreviousJitteredViewProjection = glm::mat4(1.0f);

		mutable glm::vec2 JitterNDC = glm::vec2(0.0f);
		mutable glm::vec2 PreviousJitterNDC = glm::vec2(0.0f);


#ifdef HZ_DEBUG
		RenderDebugSetting DebugSetting;
#endif
	};

	struct DrawItem
	{
		const RenderObject* Object = nullptr;
		Ref<MaterialInstance> Material;
		Ref<Mesh> Mesh;
		MaterialRenderConfig RenderConfig;
		Ref<Shader> ShaderProgram;
		ToonMaterialRole ToonRole = ToonMaterialRole::Default;
		uint64_t SortKey = 0;
		float CameraDistance = 0.0f;
	};

	struct RendererLog
	{
		unsigned int DrawCalls = 0;
		unsigned int OpaqueDrawItems = 0;
		unsigned int TransparentDrawItems = 0;
		unsigned int LightCount = 0;
		unsigned int ShadowDrawCalls = 0;

		RenderProfile Profile;
		double OutlineCPUTime = 0.0;

		RendererLog& operator+=(const RendererLog& other)
		{
			DrawCalls += other.DrawCalls;
			OpaqueDrawItems += other.OpaqueDrawItems;
			TransparentDrawItems += other.TransparentDrawItems;
			return *this;
		}
	};


}
