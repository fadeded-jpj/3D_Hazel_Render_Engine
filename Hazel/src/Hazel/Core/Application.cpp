#include "hzpch.h"
#include "Application.h"

#include "Hazel/Input/Input.h"

#include "glm/glm.hpp"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/AssetsSystem/AssetManager.h"

#include "GLFW/glfw3.h"

namespace Engine
{
	// to use a member function need a objection, so use std::bind to bind it 
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		:m_specification(specification)
	{
		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));


		AssetManagerConfig assetConfig;
		assetConfig.EngineContentRoot = "../Hazel/src/Hazel/Content";
		assetConfig.GameContentRoot = m_specification.GameContextRoot;

		AssetManager::Init(assetConfig);
		Renderer::Init();

		auto layer = std::make_unique<ImGuiLayer>();
		m_ImGuiLayer = layer.get();
		PushOverlayer(std::move(layer));
		
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClosed));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResized));

		//HZ_CORE_TRACE("{0}", e.ToString());
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); it++)
		{
			(*it)->OnEvent(e);
			if (e.Handled())
				break;
		}
	}

	Application::~Application()
	{
		m_LayerStack.Clear();
		m_ImGuiLayer = nullptr;

		Renderer::Shutdown();
		AssetManager::Shutdown();

		s_Instance = nullptr;
	}

	void Application::PushLayer(Scope<Layer> layer)
	{
		layer->OnAttach();
		m_LayerStack.PushLayer(std::move(layer));
	}

	void Application::PushOverlayer(Scope<Layer> overlayer)
	{
		overlayer->OnAttach();
		m_LayerStack.PushOverlay(std::move(overlayer));

	}

	void Application::Run()
	{
		while (m_Running)
		{
			float time = (float)glfwGetTime();
			Timestep deltaTime = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				for (auto& layer : m_LayerStack)
					layer->OnUpdate(deltaTime);			
			}

			m_ImGuiLayer->Begin();
			for(auto& layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}
	
	bool Application::OnWindowClosed(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
	bool Application::OnWindowResized(WindowResizeEvent& e)
	{
		if(e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;

		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
		
		return false;
	}
}