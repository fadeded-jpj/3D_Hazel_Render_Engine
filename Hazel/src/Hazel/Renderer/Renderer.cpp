#include "hzpch.h"
#include "Renderer.h"

#include "Hazel/Camera/Camera.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/RHI/BoneMatrixBuffer.h"
#include "Hazel/Renderer/RHI/RenderCommand.h"
#include "Hazel/Renderer/Resources/RenderResourceCache.h"
#include "Hazel/Renderer/Shader/ShaderManager.h"
#include "Hazel/Profile/RenderGPUProfiler.h"


namespace Engine
{
	Scope<Renderer::SceneData> Renderer::m_SceneData = nullptr;
	Scope<RenderPipeline> Renderer::m_Pipeline = nullptr;
	RendererLog Renderer::s_RendererLog = RendererLog();

	void Renderer::Init()
	{
		
		ShaderManager::Init();
		
		m_SceneData = std::make_unique<Renderer::SceneData>();
		// m_Pipeline = std::make_unique<ForwardRenderPipeline>();
		m_Pipeline = std::make_unique<DeffedRenderPipline>();
		// m_Pipeline = std::make_unique<TestRenderPipeline>();
		s_RendererLog = RendererLog();

		RenderCommand::Init();
		RenderGPUProfiler::Init();
		RenderResourceCache::Init();
	}
	void Renderer::Shutdown()
	{
		m_SceneData->RenderObjects.clear();
		m_Pipeline.reset();

		ShaderManager::Shutdown();
		RenderResourceCache::Shutdown();
		RenderGPUProfiler::Shutdown();
		RenderCommand::Shutdown();

		m_SceneData.reset();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const Camera& camera, const unsigned int& Width, const unsigned int& Hight, const float& time,
		RenderObjectID selectedID)
	{
		//m_SceneData->m_RenderQueue.clear();
		//m_SceneData->ViewProjMat = camera.GetViewProjectonMatrix();
		//m_SceneData->CameraPosition = camera.GetPosition();
		m_SceneData->RenderObjects.clear();
		m_SceneData->CharacterBounds.Reset();
		m_SceneData->View.ViewProjection = camera.GetViewProjectonMatrix();
		m_SceneData->View.Projection = camera.GetProjectionMatrix();
		m_SceneData->View.InverseViewProjection = glm::inverse(m_SceneData->View.ViewProjection);
		m_SceneData->View.CameraPosition = camera.GetPosition();
		m_SceneData->View.ViewportSize = { Width, Hight };
		m_SceneData->View.Time = time;
		m_SceneData->View.SelectedID = selectedID;
		m_SceneData->View.NearClip = camera.GetNearClip();
		m_SceneData->View.FarClip = camera.GetFarClip();
		m_SceneData->View.FOV = camera.GetFOV();
		m_SceneData->View.View = camera.GetViewMatrix();
	}
	void Renderer::EndScene(Ref<FrameBuffer>& target)
	{
		s_RendererLog = m_Pipeline->Render(
			m_SceneData->View,
			target,
			m_SceneData->RenderObjects,
			m_SceneData->Lights,
			m_SceneData->CharacterBounds);
		s_RendererLog.LightCount = m_SceneData->Lights.LightCount;
		m_SceneData->RenderObjects.clear();
	}
	void Renderer::Submit(const SceneSubmitItem& submit)
	{
		RenderObject item;
		item.Material = submit.material;
		item.Mesh = submit.mesh;
		item.OutlineEdgeMesh = submit.outlineEdgeMesh;
		item.Transform = submit.transform;
		item.WorldBounds = submit.worldBounds;
		item.Skinning = submit.skinningData;
		item.Character = submit.CharacterData;
		item.ID = submit.renderObjectID;
		item.OutlineMode = submit.outlinemode;
		item.Domain = submit.domain;

		m_SceneData->RenderObjects.push_back(std::move(item));
	}
	void Renderer::SubmitCharacterBounds(const AABB& worldBounds)
	{
		m_SceneData->CharacterBounds.Encapsulate(worldBounds);
	}
	void Renderer::SetSceneLighting(SceneLightingData& lightData)
	{
		m_SceneData->Lights = std::move(lightData);
	}

#if HZ_DEBUG
	void Renderer::SetRenderDebug(RenderDebugSetting& debug)
	{
		m_SceneData->View.DebugSetting = debug;
	}
#endif // HZ_DEBUG

}
