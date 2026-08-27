#pragma once

#include "Hazel/Renderer/RHI/FrameBuffer.h"
#include "Hazel/Renderer/Shader/ShaderManager.h"
namespace Engine
{
	class MaterialInstance;

	struct ShadowView
	{
		bool Enabled = false;
		glm::vec3 ViewOrigin = glm::vec3(100.0f);

		glm::mat4 View = glm::mat4(1.0f);
		glm::mat4 Projection = glm::mat4(1.0f);
		glm::mat4 ViewProjection = glm::mat4(1.0f);

		glm::vec3 LightDirection{ 0.0f, -1.0f, 0.0f };
		float DepthBias = 0.001f;
	};

	struct PointShadowView
	{
		bool Enabled = false;
		glm::vec3 ViewOrigin = glm::vec3(0.0f);
		float NearPlane = 0.1f;
		float FarPlane = 25.0f;

		std::array<glm::mat4, 6> FaceViewProjection;
		float DepthBias = 0.001f;
	};

	struct CSMShadowView
	{
		bool Enabled = false;
		static constexpr int CascadeCount = 4;
		std::array<glm::mat4, CascadeCount> ViewProjection;
		std::array<float, CascadeCount> SplitDepths;
		glm::vec3 LightDirection{ 0.0f, -1.0f, 0.0f };
		float DepthBias = 0.001f;
		float OverlapRatio = 0.15f;
	};

	class ShadowMap
	{
	public:
		ShadowMap(unsigned int width = 2048, unsigned int height = 2048);

		void Bind() { m_ShadowMap->Bind(); }
		void UnBind() { m_ShadowMap->UnBind(); }
		void Resize(unsigned int width, unsigned int height)
		{
			m_ShadowMap->Resize(width, height);
		}

		Ref<Texture2D> GetShaderMap() const { return m_ShadowMap->GetDepthAttachment(); }

	private:
		Ref<FrameBuffer> m_ShadowMap;
	};


	class PointShadowMap
	{
	public:
		PointShadowMap(unsigned int size = 1024);

		void BindFace(unsigned int faceIndex)
		{
			m_ShadowMap->AttachDepthCubeFace(m_DepthCubeMap, faceIndex);
			m_ShadowMap->Bind();
		}
		void UnBind() { m_ShadowMap->UnBind(); }

		Ref<TextureCubeMap> GetShaderMap() const { 
			return m_DepthCubeMap;
		}

	private:
		Ref<FrameBuffer> m_ShadowMap;
		Ref<TextureCubeMap> m_DepthCubeMap;
	};
	
	class CSMShadowMap
	{
	public:
		CSMShadowMap(unsigned int width = 2048, unsigned int height = 2048, 
			unsigned int cascadeCount = 4);

		void BindCascade(unsigned int cascadeIndex)
		{
			m_ShadowMap->AttachDepthArray(m_DepthArray, cascadeIndex);
			m_ShadowMap->Bind();
		}
		void UnBind() { m_ShadowMap->UnBind(); }
		Ref<Texture2DArray> GetShaderMap() const {
			return m_DepthArray;
		}
	private:
		Ref<FrameBuffer> m_ShadowMap;
		Ref<Texture2DArray> m_DepthArray;
	};


	struct Shadow2DFrameData
	{
		bool Enabled = false;
		int LightIndex = -1;

		glm::mat4 ViewProjection{ 1.0f };
		float DepthBias = 0.001f;

		Ref<Texture2D> DepthTexture;
	};

	struct PointShadowFrameData
	{
		bool Enabled = false;
		int LightIndex = -1;

		glm::vec3 LightPosition{ 0.0f };
		float NearPlane = 0.1f;
		float FarPlane = 25.0f;
		float DepthBias = 0.001f;

		Ref<TextureCubeMap> DepthTexture;
	};

	// shadow map 所需的参数
	struct ShadowFrameData
	{
		bool Enabled = false;
		int MainLightIndex = -1;

		glm::mat4 ViewProjection{ 1.0f };	// 2D Shadow 使用
		glm::vec3 lightPosition{};			// Point Shadow 使用
		float FarPlane = 1.0f;				// Point Shadow 使用
		glm::mat4 CSMViewProjection[4];		// CSM Shadow 使用
		float OverlapRatio = 0.1f;			// CSM Shadow 使用
		float DepthBias = 0.001f;

		bool IsCSM = false;
		glm::vec4 CascadeSplits{ 0.0f };
	};

	enum ShadowTextureBinding
	{
		Scene2D = 1,
		PointCube = 2,
		SceneCSM = 3,
		Character2D = 4,
		PassLocalBegin = 5
	};

	// shadow map 的资源
	struct ShadowResourceSet
	{
		Ref<Texture2D> Scene2D = nullptr;
		Ref<TextureCubeMap> PointCube = nullptr;
		Ref<Texture2DArray> SceneCSM = nullptr;
		Ref<Texture2D> Character2D = nullptr;

		void Bind() const
		{
			Scene2D->Bind(ShadowTextureBinding::Scene2D);
			PointCube->Bind(ShadowTextureBinding::PointCube);
			SceneCSM->Bind(ShadowTextureBinding::SceneCSM);
			Character2D->Bind(ShadowTextureBinding::Character2D);
		}
	};
}
