#include "hzpch.h"
#include <Hazel.h>

#include "Hazel/Core/EntryPoint.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <limits>
#include <algorithm>


struct TimingStatistics
{
	uint32_t SampleCount = 0;
	double TotalMs = 0.0;
	double MinMs = std::numeric_limits<double>::max();
	double MaxMs = 0.0;

	void Reset()
	{
		SampleCount = 0;
		TotalMs = 0.0;
		MinMs = std::numeric_limits<double>::max();
		MaxMs = 0.0;
	}

	void Add(double valueMs)
	{
		++SampleCount;
		TotalMs += valueMs;
		MinMs = std::min(MinMs, valueMs);
		MaxMs = std::max(MaxMs, valueMs);
	}

	double GetAverageMs() const
	{
		return SampleCount == 0 ? 0.0 : TotalMs / SampleCount;
	}
};

class ExampleLayer : public Engine::Layer
{
public:
	ExampleLayer()
		:Layer("Example"), m_CameraController(1.6f / 0.9f, Engine::CameraController::CameraType::Perspective)
	{
		m_Scene.reset(new Engine::Scene());

		m_ShaderLib.Load("Assets/shaders/Texture.glsl");
		m_ShaderLib.Load("Assets/shaders/Color.glsl");
		m_ShaderLib.Load("Assets/shaders/mesh.glsl");
		m_ShaderLib.Load("Assets/shaders/screen.glsl");

		m_FrameBuffer = Engine::FrameBuffer::Create({ 1280, 720 , { 
			{Engine::TextureFormat::RGBA8},
			{Engine::TextureFormat::Depth24Stencil8}} });

		auto m_MeshMaterial = Engine::Material::Create(m_ShaderLib.Get("mesh"));

		Engine::Ref<Engine::Texture2D> m_Texture = Engine::Texture2D::Create("Assets/textures/icon.jpg");
		Engine::Ref<Engine::Texture2D> m_TestTex = Engine::Texture2D::Create("Assets/textures/test.png");

		Engine::Ref<Engine::VertexArray> m_SqureVA = Engine::VertexArray::Create();

		Engine::TextureLoadOptions options;
		options.ColorSpace = Engine::TextureColorSpace::SRGB;
		options.GenerateMips = true;
		options.FlipVertically = false;
		
		m_SkyBox = Engine::TextureCubeMap::Create(
			"Assets/textures/Skybox/Universe/universe_cubemap_cross.png",
			options
		);
		m_Scene->SetSkyBox(m_SkyBox);


		float vertices[] = {
			-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
			 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
		};

		uint32_t indices[] = {
			0, 1, 2,
			1, 3, 2
		};

		Engine::BufferLayout layout = {
			{Engine::ShaderDataType::Float3, "a_Pos"},
			{Engine::ShaderDataType::Float2, "a_TexCoord" }
		};

		//auto meshData = Engine::MeshData({ layout, vertices, sizeof(vertices), 4, indices, 6});
		//m_ScreenMesh = Engine::Mesh::Create(meshData);
		//m_ScreenMaterial = Engine::Material::Create(m_ShaderLib.Get("screen"));
		

		auto PMXHandle = Engine::AssetManager::ImportAsset("/Game/models/Feiying/Feiying.pmx");
		auto backHandle = Engine::AssetManager::ImportAsset("/Game/models/background/background.gltf");

		auto cubeHandle = Engine::AssetManager::ImportAsset("/Game/models/Cube.fbx");
		auto VMDHandle = Engine::AssetManager::ImportAsset("/Game/Animation/t2.vmd");
		auto planeHandle = Engine::AssetManager::ImportAsset("/Game/models/Plane.gltf");

		// 人物模型加载
		auto m_Mesh = m_Scene->CreateEntity("Mesh");
		m_Scene->SetEntityType(m_Mesh.GetID(), Engine::EntityType::Actor);
		m_Scene->SetEntityTag(m_Mesh.GetID(), Engine::EntityTag::MainCharacter);
		auto& transform = m_Scene->GetStorage<Engine::TransformComponent>().Get(m_Mesh.GetID());
		auto& render = m_Scene->AddComponent<Engine::RenderComponent>(m_Mesh.GetID());
		transform.SetScale({0.1f, 0.1f, 0.1f});
		transform.SetTranslation({ 0.0f, 0.0f, 0.0f });
		render.ModelAsset = Engine::AssetManager::GetAsset<Engine::Model>(PMXHandle);
		render.OutlineMode = Engine::RenderOutlineMode::InvertedHull;
		auto& animator = m_Scene->AddComponent<Engine::AnimationComponent>(m_Mesh.GetID());
		animator.AnimatorAsset = std::make_shared<Engine::Animator>(render.ModelAsset);
		auto clip = Engine::AssetManager::GetAsset<Engine::AnimationClip>(VMDHandle);
		animator.AnimatorAsset->SetAnimationClip(clip);

		// 场景模型加载
		auto m_Background = m_Scene->CreateEntity("Backgroud");
		auto& m_BackTransform = m_Scene->GetStorage<Engine::TransformComponent>().Get(m_Background.GetID());
		auto& m_BackDataRender = m_Scene->AddComponent<Engine::RenderComponent>(m_Background.GetID());
		//m_BackDataRender.ModelAsset = Engine::AssetManager::GetAsset<Engine::Model>(planeHandle);
		m_BackDataRender.ModelAsset = Engine::AssetManager::GetAsset<Engine::Model>(backHandle);

		// 主光源加载
		auto m_MainLight = m_Scene->CreateEntity("MainLight");
		auto& light = m_Scene->AddComponent<Engine::LightComponent>(m_MainLight.GetID());
		auto& m_LightTransform = m_Scene->GetStorage<Engine::TransformComponent>().Get(m_MainLight.GetID());
		m_LightTransform.SetTranslation(glm::vec3(1.0f));
		m_LightTransform.SetRotation(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f));
		light.IsMainLight = true;
		light.CastShadows = true;
		light.Type = Engine::LightType::Directional;

		// 次级光源
		for (int i = 0; i < 1; i++)
		{
			auto& m_light = m_Scene->CreateEntity("Sub Light_" + std::to_string(i));
			auto& light = m_Scene->AddComponent<Engine::LightComponent>(m_light.GetID());
			auto& transform = m_Scene->GetStorage<Engine::TransformComponent>().Get(m_light.GetID());
			transform.SetTranslation(glm::vec3(1.0f, 1.0f * i, 1.0f * i));
			light.Color = glm::vec3(1.0f);
			light.Intensity = 0.3f;
		}
	}

	void OnUpdate(Engine::Timestep ts) override
	{
		//HZ_CLIENT_TRACE("Delta Time: {0}s, {1}ms", ts.GetSeconds(), ts.GetMilliseconds());
		 m_deltaTime = ts;
		 m_RunningTime += ts.GetSeconds();
		 m_AccumulateTime += m_deltaTime;
		 m_AccumulateFrameCount++;
		 if (m_AccumulateTime >= 5.0f)
		 {
			 m_CurrentTimePerFrame = m_AccumulateTime / m_AccumulateFrameCount;
			 m_AccumulateFrameCount = 0;
			 m_AccumulateTime = 0.0f;
		 }

		m_CameraController.OnUpdate(ts);

		m_Scene->OnUpdate(ts);

		if (m_FrameBufferResizePending)
		{
			m_RenderSize = m_PendingRenderSize;
			m_FrameBuffer->Resize(m_RenderSize.x, m_RenderSize.y);
			m_CameraController.SetWindowSize(m_RenderSize.x, m_RenderSize.y);
			m_FrameBufferResizePending = false;
		}


		Engine::RenderCommand::SetClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
		Engine::RenderCommand::Clear();


		static glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

		Engine::Renderer::BeginScene(m_CameraController.GetCamera(), m_RenderSize.x, m_RenderSize.y, m_RunningTime,
			m_Selection.GetRenderObjectID()
		);
#ifdef HZ_DEBUG
		Engine::Renderer::SetRenderDebug(m_DebugSetting);
#endif // HZ_DEBUG

		m_Scene->OnRender();
		Engine::Renderer::EndScene(m_FrameBuffer);

		m_Logs = Engine::Renderer::GetRendererLog();

		//Engine::RenderCommand::SetClearColor({0,0,0,1});
		//Engine::RenderCommand::Clear();

		//m_ScreenMaterial->SetTexture("u_ScreenTexture", m_FrameBuffer->GetColorAttachment());
		//Engine::Renderer::Submit(m_ScreenMesh, m_ScreenMaterial, glm::mat4(1.0f));


		// simple profile
		constexpr uint32_t kWarmupFrames = 120;
		constexpr uint32_t kSampleFrames = 1000;

		if (m_OutlineBenchmarkRunning)
		{
			if (m_OutlineBenchmarkWarmupFrames < kWarmupFrames)
			{
				++m_OutlineBenchmarkWarmupFrames;
			}
			else
			{
				m_OutlineTimingStats.Add(m_Logs.OutlineCPUTime);

				if (m_OutlineTimingStats.SampleCount >= kSampleFrames)
					m_OutlineBenchmarkRunning = false;
			}
		}
	}

	void OnImGuiRender() override
	{
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

		// ------------------ view port --------------------
		ImGui::Begin("Viewport");
		const ImVec2 panelSize = ImGui::GetContentRegionAvail();
		bool renderViewFocused = ImGui::IsWindowFocused(
			ImGuiFocusedFlags_RootAndChildWindows);

		bool renderViewHovered = ImGui::IsWindowHovered(
			ImGuiHoveredFlags_RootAndChildWindows);

		m_CameraController.SetUpdataEnable(renderViewFocused && renderViewHovered);

		const uint32_t width =
			std::max(1u, static_cast<uint32_t>(panelSize.x));
		const uint32_t height =
			std::max(1u, static_cast<uint32_t>(panelSize.y));

		if (width != m_RenderSize.x || height != m_RenderSize.y)
		{
			m_PendingRenderSize = { width, height };
			m_FrameBufferResizePending = true;
		}

		ImGui::Image(
#ifdef HZ_DEBUG
			// Engine::ImGuiRenderer::GetTextureID(m_FrameBuffer->GetDepthAttachment()),
			Engine::ImGuiRenderer::GetTextureID(m_FrameBuffer->GetColorAttachment(0)),
#else
			Engine::ImGuiRenderer::GetTextureID(m_FrameBuffer->GetColorAttachment(0)),
#endif // HZ_DEBUG

			ImVec2(
				static_cast<float>(m_RenderSize.x),
				static_cast<float>(m_RenderSize.y)),
			ImVec2(0, 1),
			ImVec2(1, 0));

		ImGui::End();


		// -------------------------------------------
		// -------------- test setting ---------------
		// ------------------------------------------
		ImGui::Begin("Settings");
		ImGui::ColorEdit4("square color", glm::value_ptr(m_SquareColor));
		
		ImGui::Text("Anima Time %0.2f", m_Scene->GetAnimationSystem().GetTime());
		if (ImGui::Button("Play"))
			m_Scene->GetAnimationSystem().Play();
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			m_Scene->GetAnimationSystem().Stop();
		ImGui::SameLine();
		if (ImGui::Button("Pause"))
			m_Scene->GetAnimationSystem().Pause();

		ImGui::Text("Per Frame: %.2f ms", m_CurrentTimePerFrame * 1000.0f);
		ImGui::Text("FPS: %.2f", 1.0f / (m_CurrentTimePerFrame + 0.0000001f));

		ImGui::Text("Draw Calls: %d", m_Logs.DrawCalls);


		if (ImGui::Button("Benchmark Outline CPU (1000 Frames)"))
		{
			m_OutlineTimingStats.Reset();
			m_OutlineBenchmarkWarmupFrames = 0;
			m_OutlineBenchmarkRunning = true;
		}

		ImGui::Text(
			"Outline Benchmark: %s",
			m_OutlineBenchmarkRunning ? "Warmup / Sampling" : "Idle");

		if (m_OutlineTimingStats.SampleCount > 0)
		{
			ImGui::Text(
				"Outline CPU Avg: %.5f ms",
				m_OutlineTimingStats.GetAverageMs());

			ImGui::Text(
				"Outline CPU Min: %.5f ms",
				m_OutlineTimingStats.MinMs);

			ImGui::Text(
				"Outline CPU Max: %.5f ms",
				m_OutlineTimingStats.MaxMs);

			ImGui::Text(
				"Samples: %u / 1000",
				m_OutlineTimingStats.SampleCount);
		}

		ImGui::End();

		// ------------------------------------------
		// -------------- Inspector -----------------
		// ------------------------------------------
		Engine::SceneHierarchyPanel::OnImGuiRender(m_Scene, m_Selection);
		Engine::EntityInspectorPanel::OnImGuiRender(m_Scene, m_Selection);
		Engine::RenderSettingPanel::OnImGuiRender(Engine::Renderer::GetRenderView());

#ifdef HZ_DEBUG
		Engine::RenderStatsPanel::OnImGuiRender(m_DebugSetting, m_Logs);
#endif // HZ_DEBUG


	}

	void OnEvent(Engine::Event& e) override
	{
		m_CameraController.OnEvent(e);
	}
	

private:
	Engine::ShaderLibrary m_ShaderLib;
	
	Engine::CameraController m_CameraController;
	Engine::Ref<Engine::Scene> m_Scene;
	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.4f, 1.0f };

	//Engine::Ref<Engine::Mesh> m_ScreenMesh;
	//Engine::Ref<Engine::MaterialInstance> m_ScreenMaterial;

	Engine::Ref<Engine::FrameBuffer> m_FrameBuffer;
	glm::uvec2 m_RenderSize = { 1280, 720 };
	glm::uvec2 m_PendingRenderSize = { 1280, 720 };
	bool m_FrameBufferResizePending = false;

	float m_LastTime = 0.0f;
	float m_deltaTime = 0.0f;
	float m_AccumulateTime = 0.0f;
	int m_AccumulateFrameCount = 0;
	float m_CurrentTimePerFrame = 0.0f;

	TimingStatistics m_OutlineTimingStats;
	bool m_OutlineBenchmarkRunning = false;
	uint32_t m_OutlineBenchmarkWarmupFrames = 0;

	Engine::RendererLog m_Logs;

	float m_RunningTime = 0.0f;

	Engine::EditorSelection m_Selection;
	
	// Engine::Ref<Engine::TextureCubeMap> m_CubeTexture;
	Engine::Ref<Engine::TextureCubeMap> m_SkyBox;

#ifdef HZ_DEBUG
	Engine::RenderDebugSetting m_DebugSetting;
#endif // HZ_DEBUG

};



class Sandbox : public Engine::Application
{
public:
	Sandbox()
		:Application(CreateSpecification())
	{
		auto exampleLayer = std::make_unique<ExampleLayer>();
		PushLayer(std::move(exampleLayer));
	}

	~Sandbox()
	{
	}
private:
	static Engine::ApplicationSpecification CreateSpecification()
	{
		Engine::ApplicationSpecification res;
		res.Name = "Sandbox";
		res.ProjectRoot = ".";
		res.GameContextRoot = "Assets";

		return res;
	}
};

Engine::Application* Engine::CreateApplication()
{
	return new Sandbox();
}
