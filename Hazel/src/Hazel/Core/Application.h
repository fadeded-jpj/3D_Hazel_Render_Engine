#pragma once

#include "Core.h"

#include "Window.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Core/LayerStack.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Renderer/RHI/Shader.h"
#include "Hazel/Renderer/RHI/Buffer.h"
#include "Hazel/Renderer/RHI/VertexArray.h"
#include "Hazel/Camera/Camera.h"

#include "Hazel/Core/Timestep.h"

namespace Engine
{
	struct ApplicationSpecification
	{
		std::string Name = "Engine";

		std::filesystem::path ProjectRoot;
		std::filesystem::path GameContextRoot;
	};

	class HAZEL_API Application
	{
	public:
		Application(const ApplicationSpecification& specification);
		virtual ~Application();

		void OnEvent(Event& e);

		void Run();

		void PushLayer(Scope<Layer> layer);
		void PushOverlayer(Scope<Layer> overlayer);

		inline static Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_Window; }
	private:
		bool OnWindowClosed(WindowCloseEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);

		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer = nullptr;
		bool m_Running = true;
		bool m_Minimized = false;

		LayerStack m_LayerStack;

		float m_LastFrameTime = 0.0f;
	private:
		static Application* s_Instance;
		ApplicationSpecification m_specification;
	};

	// To be defined in client
	Application* CreateApplication();
}

