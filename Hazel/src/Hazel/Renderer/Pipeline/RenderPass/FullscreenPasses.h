#pragma once

#include "Hazel/Renderer/Geometry/Mesh.h"
#include "Hazel/Renderer/Lighting/LightType.h"
#include "Hazel/Renderer/RHI/FrameBuffer.h"
#include "Hazel/Renderer/RHI/GBuffer.h"
#include "Hazel/Renderer/RenderTypes.h"
#include "Hazel/Renderer/Shader/ShaderParameters.h"

namespace Engine
{
	struct ShadowFrameData;

	class DefferredLightingPass
	{
	public:
		explicit DefferredLightingPass(const Ref<Shader>& shader);

		void Execute(const RenderView& view, const SceneLightingData& light,
			const Ref<GBuffer>& gbuffer, const ShadowFrameData& sceneShadow,
			const ShadowFrameData& characterShadow, const Ref<Texture2D>& ssao,
			const Ref<FrameBuffer>& target,
			const Ref<Texture2D>& characterDepth);

	private:
		static void SubmitLight(ShaderParameters& parameters, const SceneLightingData& light);

	private:
		ShaderParameters m_PerPassParameters;
		ShaderParameters m_MaskParameters;
		Ref<Mesh> m_FullscrrenQuad;

		Ref<Shader> m_PBRShader;
		Ref<Shader> m_ToonShader;

		Ref<FrameBuffer> m_MaskBuffer;
		Ref<Texture2D> m_ShadowMask;
		Ref<Texture2D> m_CharacterShadowMask;
		Ref<Shader> m_MaskShader;
	};

	class ToneMappingPass
	{
	public:
		explicit ToneMappingPass(const Ref<Shader>& shader);

		void Execute(const RenderView& view, const Ref<Texture2D>& input, const Ref<Texture2D>& bloom);

	private:
		ShaderParameters m_PerPassParameters;
		Ref<Mesh> m_FullscrrenQuad;
		Ref<Shader> m_Shader;
	};

	class ScreenSpaceOutlinePass
	{
	public:
		ScreenSpaceOutlinePass(const Ref<Shader>& shader);
		void Execute(const RenderView& view, const Ref<Texture2D>& normal);

	private:
		Ref<Mesh> m_FullscrrenQuad;
		ShaderParameters m_PerPassParameters;
		Ref<Shader> m_Shader;
	};

	class BloomPass
	{
	public:
		BloomPass(const Ref<Shader>& shader);
		void Execute(const RenderView& view, const Ref<Texture2D>& screenColor);

		Ref<Texture2D> GetBloomTexture() const { return m_BloomTexture; }

	private:
		Ref<Mesh> m_FullscreenQuad;
		ShaderParameters m_PerPassParameters;
		Ref<Shader> m_FilterShader;	// 从 SceneColor 提取高亮区域

		Ref<Shader> m_UpShader;		// 上采样着色器
		Ref<Shader> m_DownShader;	// 下采样着色器

		Ref<FrameBuffer> m_BloomFrameBuffer;
		Ref<Texture2D> m_BloomTexture;

		uint32_t m_ActiveMipLevels = 6u;
	};

	class TAAPass
	{
	public:
		TAAPass(const Ref<Shader>& shader);
		void Execute(const RenderView& view, const Ref<FrameBuffer>& screenBuffer,
			const Ref<Texture2D>& previousDepth, const Ref<Texture2D>& previousVelocity);

		Ref<Texture2D> GetHistory() const { return m_History[m_HistoryIndex]; }

	private:
		// TAA 重投影矩阵，用来找当前pixel在上一帧的位置
		bool m_HistoryValid = false;

		Ref<Shader> m_Shader;

		int m_HistoryIndex = 0;

		Ref<Texture2D> m_History[2];

		Ref<Mesh> m_FullscreenQuad;
		ShaderParameters m_PerPassParameters;
		Ref<FrameBuffer> m_Framebuffer;
	};

	class SSAOPass
	{
	public:
		SSAOPass(const Ref<Shader>& shader);
		
		void Execute(const RenderView& view, const Ref<GBuffer>& gbuffer);
		Ref<Texture2D> GetSSAOTexture() { return m_OutBuffer->GetColorAttachment(0); }
	private:
		Ref<FrameBuffer> m_FrameBuffer;
		Ref<FrameBuffer> m_OutBuffer;
		Ref<Shader> m_Shader;
		Ref<Shader> m_BlurShader;

		Ref<Mesh> m_FullscreenQuad;
		ShaderParameters m_SSAOPassParameters;
		ShaderParameters m_BlurParameters;
	};

	class SSRPass
	{
	public:
		SSRPass(const Ref<Shader>& shader);
		void Execute(const RenderView& view, const Ref<GBuffer>& gbuffer,
			const Ref<FrameBuffer>& buffer);
	private:
		Ref<Shader> m_Shader;
		Ref<Shader> m_HiZShader;
		Ref<Mesh> m_FullscreenQuad;
		Ref<Texture2D> m_Color[2];

		Ref<FrameBuffer> m_Framebuffer;
		Ref<Texture2D> m_MipDepth;
		uint32_t m_MipCount = 0;
		ShaderParameters m_Parameters;
		ShaderParameters m_DepthParameters;

		unsigned int m_WriteIndex = 0;
	};
}
