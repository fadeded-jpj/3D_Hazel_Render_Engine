#include "hzpch.h"
#include "CameraController.h"

#include <algorithm>
#include <cmath>

#include "Hazel/Input/Input.h"
#include "Hazel/Input/KeyCodes.h"

namespace Engine
{
	CameraController::CameraController(float aspectRatio, bool rotation)
		: CameraController(aspectRatio, CameraType::Orthographic, rotation)
	{
	}

	CameraController::CameraController(float aspectRatio, CameraType type, bool rotation)
		: m_AspactRatio(aspectRatio), m_CameraType(type), m_Rotation(rotation)
	{
		if (m_CameraType == CameraType::Perspective)
			m_CameraPos = { 0.0f, 1.5f, 5.0f };
		CreateCamera();
	}

	void CameraController::OnUpdate(Timestep ts)
	{
		if (!m_ApplyingUpdata)
			return;

		if (m_CameraType == CameraType::Orthographic)
			UpdateOrthographic(ts);
		else
			UpdatePerspective(ts);

		m_Camera->SetPosition(m_CameraPos);
		m_Camera->SetRotation(m_CameraRot);
	}

	void CameraController::OnEvent(Event& e)
	{
		EventDispatcher dispatch(e);
		dispatch.Dispatch<MouseScrolledEvent>(HZ_BIND_EVENT_FN(CameraController::OnMouseScrolled));
		dispatch.Dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(CameraController::OnWindowResized));
	}

	void CameraController::SetZoomLevel(float level)
	{
		m_ZoomLevel = level;
		UpdateProjection();
	}

	void CameraController::SetWindowSize(unsigned int W, unsigned H)
	{
		m_AspactRatio = (float)W / (float)H;
		UpdateProjection();
	}

	bool CameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		if (!m_ApplyingUpdata)
			return false;

		if (m_CameraType == CameraType::Orthographic)
		{
			m_ZoomLevel -= e.GetYOffset() * 0.5f;
			m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
		}
		else
		{
			m_FOV -= e.GetYOffset() * 2.0f;
			m_FOV = std::clamp(m_FOV, 15.0f, 90.0f);
		}

		UpdateProjection();
		return false;
	}

	bool CameraController::OnWindowResized(WindowResizeEvent& e)
	{
		if (!m_ApplyingUpdata)
			return false;

		m_AspactRatio = (float)e.GetWidth() / (float)e.GetHeight();
		HZ_CORE_INFO("window resized: {0}x{1}, aspect ratio: {2}", e.GetWidth(), e.GetHeight(), m_AspactRatio);
		UpdateProjection();
		return false;
	}

	void CameraController::CreateCamera()
	{
		if (m_CameraType == CameraType::Orthographic)
		{
			m_Camera = std::make_unique<OrthographicCamera>(
				-m_AspactRatio * m_ZoomLevel,
				m_AspactRatio * m_ZoomLevel,
				-m_ZoomLevel,
				m_ZoomLevel
			);
		}
		else
		{
			m_Camera = std::make_unique<PerspectiveCamera>(m_FOV, m_AspactRatio, m_NearClip, m_FarClip);
		}
		m_Camera->SetPosition(m_CameraPos);
		m_Camera->SetRotation(m_CameraRot);
	}

	void CameraController::UpdateProjection()
	{
		if (m_CameraType == CameraType::Orthographic)
		{
			auto* camera = static_cast<OrthographicCamera*>(m_Camera.get());
			camera->SetProjection(-m_AspactRatio * m_ZoomLevel, m_AspactRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
		}
		else
		{
			auto* camera = static_cast<PerspectiveCamera*>(m_Camera.get());
			camera->SetProjection(m_FOV, m_AspactRatio, m_NearClip, m_FarClip);
		}
	}

	void CameraController::UpdateOrthographic(Timestep ts)
	{
		if (Input::IsKeyPressed(HZ_KEY_A))
			m_CameraPos.x -= m_CameraTranslationSpeed * ts;
		else if (Input::IsKeyPressed(HZ_KEY_D))
			m_CameraPos.x += m_CameraTranslationSpeed * ts;

		if (Input::IsKeyPressed(HZ_KEY_W))
			m_CameraPos.y += m_CameraTranslationSpeed * ts;
		else if (Input::IsKeyPressed(HZ_KEY_S))
			m_CameraPos.y -= m_CameraTranslationSpeed * ts;

		if (m_Rotation)
		{
			if (Input::IsKeyPressed(HZ_KEY_Q))
				m_CameraRot.z += m_CameraRotationSpeed * ts;
			else if (Input::IsKeyPressed(HZ_KEY_E))
				m_CameraRot.z -= m_CameraRotationSpeed * ts;
		}

		m_CameraTranslationSpeed = m_ZoomLevel;
	}

	void CameraController::UpdatePerspective(Timestep ts)
	{
		float yaw = glm::radians(m_CameraRot.y);
		float pitch = glm::radians(m_CameraRot.x);
		glm::vec3 forward = glm::normalize(glm::vec3(
			-std::sin(yaw) * std::cos(pitch),
			std::sin(pitch),
			-std::cos(yaw) * std::cos(pitch)
		));
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

		float translationDelta = m_CameraTranslationSpeed * ts;
		if (Input::IsKeyPressed(HZ_KEY_W))
			m_CameraPos += forward * translationDelta;
		else if (Input::IsKeyPressed(HZ_KEY_S))
			m_CameraPos -= forward * translationDelta;

		if (Input::IsKeyPressed(HZ_KEY_A))
			m_CameraPos -= right * translationDelta;
		else if (Input::IsKeyPressed(HZ_KEY_D))
			m_CameraPos += right * translationDelta;

		if (Input::IsKeyPressed(HZ_KEY_Q))
			m_CameraPos.y -= translationDelta;
		else if (Input::IsKeyPressed(HZ_KEY_E))
			m_CameraPos.y += translationDelta;

		float rotationDelta = m_CameraRotationSpeed * ts;
		if (Input::IsKeyPressed(HZ_KEY_LEFT))
			m_CameraRot.y -= rotationDelta;
		else if (Input::IsKeyPressed(HZ_KEY_RIGHT))
			m_CameraRot.y += rotationDelta;

		if (Input::IsKeyPressed(HZ_KEY_UP))
			m_CameraRot.x -= rotationDelta;
		else if (Input::IsKeyPressed(HZ_KEY_DOWN))
			m_CameraRot.x += rotationDelta;

		m_CameraRot.x = std::clamp(m_CameraRot.x, -89.0f, 89.0f);
	}
}
