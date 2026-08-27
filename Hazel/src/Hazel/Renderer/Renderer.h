#pragma once

#include <cstdint>
#include <vector>

#include "Hazel/Renderer/RenderTypes.h"
#include "Hazel/Renderer/RHI/RendererAPI.h"
#include "Hazel/Renderer/Pipeline/RenderPipeline.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Renderer/Lighting/LightType.h"
#include "Hazel/Renderer/RenderDebug/RenderDebug.h"

namespace Engine
{
	class Camera;
	class MaterialInstance;
	class Mesh;

	struct SceneSubmitItem
	{
		RenderDomain domain;

		Ref<Mesh> mesh = nullptr;
		Ref<Mesh> outlineEdgeMesh = nullptr;
		Ref<MaterialInstance> material = nullptr;
		glm::mat4 transform = glm::mat4(1.0f);
		AABB worldBounds;
		RenderSkinningData skinningData;
		RenderCharacterData CharacterData;
		RenderObjectID renderObjectID = INVALID_RENDER_OBJECT_ID;
		RenderOutlineMode outlinemode = RenderOutlineMode::None;
	};

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(const Camera& camera, const unsigned int& Width, const unsigned int& Hight, const float& time,
			RenderObjectID selectedID = INVALID_RENDER_OBJECT_ID);
		static void EndScene(Ref<FrameBuffer>& target);

		static void Submit(const SceneSubmitItem& item);
		static void SubmitCharacterBounds(const AABB& worldBounds);
		static void SetSceneLighting(SceneLightingData& lightData);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		static RenderView& GetRenderView() { return m_SceneData->View; }

#ifdef HZ_DEBUG
		static Ref<Texture2D> GetDebugOutputTexture()
		{
			return m_Pipeline->GetDebugOutputTexture();
		}
#endif // HZ_DEBUG

		static RenderObjectID MakeRenderObjectID(EntityID entityID, uint32_t subMeshIndex)
		{
			return (static_cast<uint64_t>(entityID) << 32) | subMeshIndex;
		}

#ifdef HZ_DEBUG
#include "Hazel/Renderer/RenderDebug/RenderDebug.h"
		static void SetRenderDebug(RenderDebugSetting& debug);
#endif
		
	private:
		struct SceneData
		{
			RenderView View;
			std::vector<RenderObject> RenderObjects;
			SceneLightingData Lights;
			CharacterShadowBounds CharacterBounds;
		};

		static Scope<SceneData> m_SceneData;
		static Scope<RenderPipeline> m_Pipeline;

		static RendererLog s_RendererLog;
	public:
		static const RendererLog& GetRendererLog() { return s_RendererLog; }
		
	};
}
