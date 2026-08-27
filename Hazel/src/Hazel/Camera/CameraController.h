#pragma once

#include "Camera.h"
#include "Hazel/Core/Core.h"
#include "Hazel/Core/Timestep.h"

#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Events/ApplicationEvent.h"

namespace Engine
{
	class CameraController
	{
	public:
		enum class CameraType
		{
			Orthographic = 0,
			Perspective = 1
		};

		CameraController(float aspectRatio, bool rotation = false);
		CameraController(float aspectRatio, CameraType type, bool rotation = false);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		inline Camera& GetCamera() { return *m_Camera; }
		inline const Camera& GetCamera() const { return *m_Camera; }
		inline CameraType GetCameraType() const { return m_CameraType; }

		void SetZoomLevel(float level);
		inline float GetZoomLevel() const { return m_ZoomLevel; }
		void SetWindowSize(unsigned int W, unsigned H);
		void SetUpdataEnable(bool enable) { m_ApplyingUpdata = enable; }

	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);
		void CreateCamera();
		void UpdateProjection();
		void UpdateOrthographic(Timestep ts);
		void UpdatePerspective(Timestep ts);
	private:
		float m_AspactRatio;
		float m_ZoomLevel = 1.0;
		float m_FOV = 45.0f;
		float m_NearClip = 0.1f;
		float m_FarClip = 1000.0f;

		bool m_ApplyingUpdata = true;

		CameraType m_CameraType = CameraType::Orthographic;
		Scope<Camera> m_Camera;

		bool m_Rotation = false;
		glm::vec3 m_CameraRot = glm::vec3(0.0f);
		glm::vec3 m_CameraPos = glm::vec3(0.0f, 1.0f, 0.0f);
		float m_CameraTranslationSpeed = 1.0f;
		float m_CameraRotationSpeed = 60.0f;
	};
}
