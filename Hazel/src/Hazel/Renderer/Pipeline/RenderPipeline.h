#pragma once

#include <vector>

#include "Hazel/Renderer/RenderTypes.h"
#include "Hazel/Renderer/Pipeline/RenderPass.h"
#include "Hazel/Renderer/RHI/FrameBuffer.h"
#include "Hazel/Renderer/RHI/GBuffer.h"
#include "Hazel/Renderer/RHI/UniformBufferType.h"

namespace Engine
{
	struct RendererLog;
	class RenderPipeline
	{
	public:
		RenderPipeline();
		virtual ~RenderPipeline() = default;
		virtual RendererLog Render(const RenderView& view, Ref<FrameBuffer>& framebuffer,
			const std::vector<RenderObject>& objects, const SceneLightingData& light,
			const CharacterShadowBounds& characterBounds) = 0;

		virtual Ref<Texture2D> GetDebugOutputTexture() const = 0;

		// 处理不同光照下的ShadowMap的view计算
		ShadowView CaculateShadowView(const RenderView& view, const RenderLightResource& light);
		ShadowView CaculateChacterShadowView(const RenderView& view, const RenderLightResource& light, const CharacterShadowBounds& bounds);
		ShadowView CaculateDirectionalShadowView(const RenderView& view, const RenderLightResource& light);
		ShadowView CaculateSpotShadowView(const RenderView& view, const RenderLightResource& light);
		PointShadowView CaculatePointShadowView(const RenderView& view, const RenderLightResource& light);
		CSMShadowView CaculateCSMShadowView(const RenderView& view, const RenderLightResource& light);

	protected:
		void UpdateCameraUniformBuffer(const RenderView& view);

	private:
		Ref<UniformBuffer> m_CameraUniformBuffer;
		CameraUniformData m_CameraUniformBufferData;
	};

	class ForwardRenderPipeline : public RenderPipeline
	{
	public:
		ForwardRenderPipeline();
		RendererLog Render(const RenderView& view, Ref<FrameBuffer>& framebuffer,
			const std::vector<RenderObject>& objects, const SceneLightingData& light,
			const CharacterShadowBounds& characterBounds) override;

		virtual Ref<Texture2D> GetDebugOutputTexture() const override
		{
			return nullptr;
		}
	private:
		OpaquePass m_OpaquePass;
		TransparentPass m_TransparentPass;
		ToneMappingPass m_TonemappingPass;
		ShadowMap2DPass m_ShadowMapPass;
		PointShadowPass m_PointShadowPass;

		Ref<FrameBuffer> m_SceneColorBuffer;
	};


	class DeffedRenderPipline : public RenderPipeline
	{
	public:
		DeffedRenderPipline();
		RendererLog Render(const RenderView& view, Ref<FrameBuffer>& target,
			const std::vector<RenderObject>& objects, const SceneLightingData& light,
			const CharacterShadowBounds& characterBounds) override;

		virtual Ref<Texture2D> GetDebugOutputTexture() const override
		{
			return m_GBuffer->GetAlbedo();
			// return m_GBuffer->GetNormalRoughness();
		}

	private:
		void PrepareCameraJitter(const RenderView& view);
		static float Halton(uint32_t index, uint32_t base);

	private:
		Ref<GBuffer> m_GBuffer;
		Ref<FrameBuffer> m_SceneColorBuffer;

		ShadowResourceSet m_ShadowResources;
		
		GBufferPass m_GBufferPass;
		DefferredLightingPass m_LightPass;
		TransparentPass m_TransparentPass;
		ToneMappingPass m_TonemappingPass;
		ShadowMap2DPass m_ShadowMapPass;
		ShadowMap2DPass m_CharacterShadowPass;
		PointShadowPass m_PointShadowPass;
		DirectionalCSMPass m_CSMShadowMapPass;
		InvertedHullOutlinePass m_InvertedHullOutlinePass;
		ScreenSpaceOutlinePass m_ScreenSpaceOutlinePass;
		CharacterPass m_CharacterPass;
		GemotryOutlinePass m_GemotryOutlinePass;
		SSAOPass m_SSAOPass;
		BloomPass m_BloomPass;
		SSRPass m_SSRPass;

		// for TAA
		TAAPass m_TAAPass;
		uint32_t m_TAAJitterFrameIndex = 0;
		uint32_t m_TAASourceIndex = 0;
		glm::mat4 m_PreviousViewProjection{ 1.0f };
		glm::mat4 m_PreviousJitteredViewProjection{ 1.0f };
		glm::vec2 m_PreviousJitterNDC{ 0.0f };
		bool m_HasPreviousViewProjection = false;

		struct TAAFrameSource
		{
			Ref<Texture2D> Velocity;
			Ref<Texture2D> Depth;
		};

		TAAFrameSource m_TAAFrameSources[2];
	};
}
