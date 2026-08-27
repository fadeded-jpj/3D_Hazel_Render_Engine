#pragma once

#include <glm/glm.hpp>

namespace Engine
{
	class Camera
	{
	public:
		virtual ~Camera() = default;

		virtual void SetPosition(const glm::vec3& position) = 0;
		virtual void SetRotation(const glm::vec3& rotation) = 0;

		virtual const glm::vec3& GetPosition() const = 0;
		virtual const glm::vec3& GetRotation() const = 0;

		virtual const glm::mat4& GetViewProjectonMatrix() const = 0;
		virtual const glm::mat4& GetProjectionMatrix() const = 0;
		virtual const glm::mat4& GetViewMatrix() const = 0;

		virtual const float GetNearClip() const = 0;
		virtual const float GetFarClip() const = 0;
		virtual const float GetFOV() const = 0;
	};

	class OrthographicCamera : public Camera
	{
	public:
		OrthographicCamera(float left, float right, float bottom, float top);

		void SetProjection(float left, float right, float bottom, float top);
		virtual void SetPosition(const glm::vec3& position) override;
		virtual void SetRotation(const glm::vec3& rotation) override;
		inline void SetRotation(float rotation) { SetRotation({ 0.0f, 0.0f, rotation }); }

		virtual const glm::vec3& GetPosition() const override { return m_Position; }
		virtual const glm::vec3& GetRotation() const override { return m_Rotation; }

		virtual const glm::mat4& GetViewProjectonMatrix() const override { return m_ViewProjectionMatrix; }
		virtual const glm::mat4& GetProjectionMatrix() const override { return m_ProjectionMatrix; }
		virtual const glm::mat4& GetViewMatrix() const override { return m_ViewMatrix; }

		virtual const float GetNearClip() const override { return -1.0f; }
		virtual const float GetFarClip() const override { return 1.0f; }
		virtual const float GetFOV() const override { return 0.0f; }
	private:
		void RecalculateViewMatrix();
	private:
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);

		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_Rotation = glm::vec3(0.0f);
	};

	class PerspectiveCamera : public Camera
	{
	public:
		PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void SetProjection(float fov, float aspectRatio, float nearClip, float farClip);
		virtual void SetPosition(const glm::vec3& position) override;
		virtual void SetRotation(const glm::vec3& rotation) override;

		virtual const glm::vec3& GetPosition() const override { return m_Position; }
		virtual const glm::vec3& GetRotation() const override { return m_Rotation; }

		virtual const glm::mat4& GetViewProjectonMatrix() const override { return m_ViewProjectionMatrix; }
		virtual const glm::mat4& GetProjectionMatrix() const override { return m_ProjectionMatrix; }
		virtual const glm::mat4& GetViewMatrix() const override { return m_ViewMatrix; }

		virtual const float GetNearClip() const override { return m_NearClip; }	
		virtual const float GetFarClip() const override { return m_FarClip; }
		virtual const float GetFOV() const override { return m_FOV; }


	private:
		void RecalculateProjectionMatrix();
		void RecalculateViewMatrix();
	private:
		float m_FOV;
		float m_AspectRatio;
		float m_NearClip;
		float m_FarClip;

		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);

		glm::vec3 m_Position = { 0.0f, 0.0f, 5.0f };
		glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };
	};
}

