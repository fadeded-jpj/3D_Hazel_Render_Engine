#include "hzpch.h"
#include "FullscreenPasses.h"

#include "Hazel/Renderer/RHI/RenderCommand.h"
#include "Hazel/Renderer/Lighting/Shadow/ShadowMap.h"

namespace Engine
{
	namespace
	{
		Ref<Mesh> CreateFullscreenQuad()
		{
			float vertices[] = {
				-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
				 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
				-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
				 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
			};

			uint32_t indices[] = { 0, 1, 2, 1, 3, 2 };

			MeshData data;
			data.VerticesData = vertices;
			data.IndicesData = indices;
			data.Layout = {
				{ ShaderDataType::Float3, "a_Pos" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			data.IndexCount = 6;
			data.VertexCount = 4;
			data.VertexBufferSize = sizeof(vertices);

			return Mesh::Create(data);
		}
	}

	DefferredLightingPass::DefferredLightingPass(const Ref<Shader>& shader)
		: m_FullscrrenQuad(CreateFullscreenQuad())
	{
		m_PBRShader = shader;
		m_ToonShader = ShaderManager::Get("ToonLighting");
		m_MaskShader = ShaderManager::Get("ShadowMask");
		FrameBufferSpecitification spec;
		spec.Width = (spec.Width + 3) / 4;
		spec.Height = (spec.Height + 3) / 4;
		spec.Attachments.clear();
		spec.AllowEmptyAttachments = true;
		m_MaskBuffer = FrameBuffer::Create(spec);
		m_ShadowMask = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RG8);
	}

	void DefferredLightingPass::Execute(const RenderView& view, const SceneLightingData& light,
		const Ref<GBuffer>& gbuffer, const ShadowFrameData& shadow,
		const ShadowFrameData& characterShadow,
		const Ref<Texture2D>& ssao,
		const Ref<FrameBuffer>& target,
		const Ref<Texture2D>& characterDepth)
	{
		// 生成shadow mask
		//const auto& size = (gbuffer->GetSize() + glm::ivec2(3)) / 4;
		//if (m_MaskBuffer->GetSize() != size)
		//{
		//	m_MaskBuffer->Resize(size.x, size.y);
		//	m_ShadowMask = Texture2D::Create(size.x, size.y, TextureFormat::RG8);
		//}
		//m_MaskParameters.Clear();
		//m_MaskBuffer->AttachColor(m_ShadowMask, 0);
		//m_MaskBuffer->SetDrawBuffers({ 0 });
		//m_MaskBuffer->Bind();

		//m_MaskParameters.SetTexture("u_GBufferDepth", gbuffer->GetDepth());
		//m_MaskParameters.SetTexture("u_GBufferNormalRoughness", gbuffer->GetNormalRoughness());
		//m_MaskParameters.SetFloat4("u_CSMSplits", shadow.CascadeSplits);
		//m_MaskParameters.SetFloat("u_OverlapRatio", shadow.OverlapRatio);
		//m_MaskParameters.SetFloat("u_ShadowBias", shadow.DepthBias);
		//m_MaskParameters.SetInt("u_ShadowEnabled", shadow.Enabled ? 1 : 0);
		//m_MaskParameters.SetInt("u_CSMEnabled", shadow.Enabled && shadow.IsCSM ? 1 : 0);
		//m_MaskParameters.SetInt("u_CharacterShadowEnabled", characterShadow.Enabled ? 1 : 0);
		//m_MaskParameters.SetMat4("u_CharacterShadowViewProj", characterShadow.ViewProjection);
		//m_MaskParameters.SetFloat("u_CharacterShadowBias", characterShadow.DepthBias);

		//glm::vec3 mainLightDirection(0.0f, -1.0f, 0.0f);
		//if (shadow.MainLightIndex >= 0 &&
		//	shadow.MainLightIndex < static_cast<int>(light.LightCount))
		//{
		//	mainLightDirection = glm::vec3(
		//		light.Lights[shadow.MainLightIndex].DirectionType);
		//}
		//m_MaskParameters.SetFloat3("u_MainLightDirection", mainLightDirection);

		//for (int i = 0; i < CSMShadowView::CascadeCount; ++i)
		//{
		//	m_MaskParameters.SetMat4("u_CSMViewProj[" + std::to_string(i) + "]", shadow.CSMViewProjection[i]);
		//}

		//RenderCommand::ApplyShaderParameters(
		//	m_MaskParameters,
		//	m_MaskShader,
		//	ShadowTextureBinding::PassLocalBegin);
		//RenderCommand::DrawMesh(m_FullscrrenQuad);


		// 正式lighting
		target->Bind();
		m_PerPassParameters.Clear();
		SubmitLight(m_PerPassParameters, light);

#ifdef HZ_DEBUG
		m_PerPassParameters.SetInt("u_DebugView", static_cast<int>(view.DebugSetting.View));
#endif

		m_PerPassParameters.SetTexture("u_GBufferAlbedoAlpha", gbuffer->GetAlbedo());
		m_PerPassParameters.SetTexture("u_GBufferNormalRoughness", gbuffer->GetNormalRoughness());
		m_PerPassParameters.SetTexture("u_GBufferEmissiveMetallic", gbuffer->GetEmissiveMetallic());
		m_PerPassParameters.SetTexture("u_GBufferDepth", gbuffer->GetDepth());
		m_PerPassParameters.SetTexture("u_GBufferRim", gbuffer->GetRimParamter());

		m_PerPassParameters.SetInt("u_ShadowEnabled", shadow.Enabled ? 1 : 0);
		m_PerPassParameters.SetInt("u_ShadowLightIndex", shadow.MainLightIndex);
		m_PerPassParameters.SetMat4("u_ShadowViewProj", shadow.ViewProjection);
		m_PerPassParameters.SetFloat("u_ShadowBias", shadow.DepthBias);
		m_PerPassParameters.SetFloat("u_ShadowFarPlane", shadow.FarPlane);
		m_PerPassParameters.SetInt("u_CSMEnabled", shadow.Enabled && shadow.IsCSM ? 1 : 0);
		m_PerPassParameters.SetInt("u_CharacterShadowEnabled", characterShadow.Enabled ? 1 : 0);
		m_PerPassParameters.SetMat4("u_CharacterShadowViewProj", characterShadow.ViewProjection);
		m_PerPassParameters.SetFloat("u_CharacterShadowBias", characterShadow.DepthBias);

		m_PerPassParameters.SetFloat4("u_CSMSplits", shadow.CascadeSplits);
		m_PerPassParameters.SetTexture("u_SSAOTexture", ssao);
		m_PerPassParameters.SetInt("u_SSAOEnabled", view.SSAO.Enabled ? 1 : 0);

		if (shadow.Enabled)
		{
			//m_PerPassParameters.SetTexture("u_ShadowMap", shadow.DepthTexture);
			//m_PerPassParameters.SetTexture("u_PointShadowMap", shadow.CubeDepthTexture);
			//m_PerPassParameters.SetTexture("u_CSMShadowMap", shadow.CSMDepthTexture);
			if (shadow.IsCSM)
			{
				m_PerPassParameters.SetFloat("u_OverlapRatio", shadow.OverlapRatio);
				// m_PerPassParameters.SetTexture("u_ShadowMask", m_ShadowMask);
				for (int i = 0; i < CSMShadowView::CascadeCount; i++)
					m_PerPassParameters.SetMat4("u_CSMViewProj[" + std::to_string(i) + "]", shadow.CSMViewProjection[i]);
			}
		}

		auto curShader = m_PBRShader;
		if (view.LightingMode == LightingMode::Toon)
		{
			curShader = m_ToonShader;
			m_PerPassParameters.SetFloat3("u_ToonShadowTint", view.ToonShadowTint);
			m_PerPassParameters.SetTexture("u_GBToonParamters", gbuffer->GetToonParamter());
			m_PerPassParameters.SetInt("u_ToonMainLightIndex", 0);
		}

		RenderCommand::ApplyShaderParameters(
			m_PerPassParameters,
			curShader,
			ShadowTextureBinding::PassLocalBegin);
		RenderCommand::DrawMesh(m_FullscrrenQuad);
	}

	void DefferredLightingPass::SubmitLight(ShaderParameters& parameters,
		const SceneLightingData& light)
	{
		if (light.Skybox)
		{
			parameters.SetInt("u_SkyTextureEnabled", 1);
			parameters.SetTexture("u_SkyTexture", light.Skybox);
		}
		else
			parameters.SetInt("u_SkyTextureEnabled", 0);

		parameters.SetInt("u_LightCount", light.LightCount);
		parameters.SetFloat3("u_AmbientColor", light.AmbientColor * light.AmbientIntensity);

		for (unsigned int i = 0; i < light.LightCount; ++i)
		{
			const auto& source = light.Lights[i];
			const auto prefix = "u_Lights[" + std::to_string(i) + "]";
			parameters.SetFloat4(prefix + ".ColorIntensity", source.ColorIntensity);
			parameters.SetFloat4(prefix + ".PositionRange", source.PositionRange);
			parameters.SetFloat4(prefix + ".DirectionType", source.DirectionType);
			parameters.SetFloat4(prefix + ".SpotAngles", source.SpotAngles);
		}
	}

	ToneMappingPass::ToneMappingPass(const Ref<Shader>& shader)
		: m_FullscrrenQuad(CreateFullscreenQuad()),
		m_Shader(shader)
	{
	}

	ScreenSpaceOutlinePass::ScreenSpaceOutlinePass(const Ref<Shader>& shader)
		: m_FullscrrenQuad(CreateFullscreenQuad()),
		m_Shader(shader)
	{
	}

	void ScreenSpaceOutlinePass::Execute(const RenderView& view, const Ref<Texture2D>& normalTexture)
	{
		m_PerPassParameters.Clear();
		// m_PerPassParameters.SetTexture("u_GBufferDepth", gbuffer->GetDepth());
		m_PerPassParameters.SetTexture("u_GBufferNormalRoughness", normalTexture);

		m_PerPassParameters.SetFloat("u_DepthThreshold", view.OutlineDepthThreshold);
		m_PerPassParameters.SetFloat("u_NormalThreshold", view.OutlineNormalThreshold);
		m_PerPassParameters.SetFloat("u_OutlineAlpha", view.OutlineAlpha);
		m_PerPassParameters.SetFloat3("u_OutlineColor", view.OutlineColor);

		RenderCommand::SetDepthTest(false);
		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetCull(CullMode::None);
		RenderCommand::SetBlend(BlendMode::AlphaBlend);

		RenderCommand::ApplyShaderParameters(m_PerPassParameters, m_Shader);
		RenderCommand::DrawMesh(m_FullscrrenQuad);
	}

	void ToneMappingPass::Execute(const RenderView& view, const Ref<Texture2D>& input, const Ref<Texture2D>& bloom)
	{
		const auto& settings = view.PostProcess.ToneMapping;
		const auto& bloomSettings = view.PostProcess.Bloom;
		const bool bloomEnabled = bloomSettings.Enabled && bloom != nullptr;

		m_PerPassParameters.Clear();
		m_PerPassParameters.SetTexture("u_SceneColor", input);
		m_PerPassParameters.SetTexture("u_BloomTexture", bloom);
		m_PerPassParameters.SetInt("u_BloomEnabled", bloomEnabled ? 1 : 0);
		m_PerPassParameters.SetFloat("u_BloomIntensity", bloomSettings.Intensity);
		m_PerPassParameters.SetInt("u_ToneMappingEnabled", settings.Enabled ? 1 : 0);
		m_PerPassParameters.SetInt("u_ToneMappingOperator", static_cast<int>(settings.Operator));
		m_PerPassParameters.SetFloat("u_Exp", settings.Exposure.CompensationEV);
		m_PerPassParameters.SetFloat("u_ReinhardWhitePoint", settings.Reinhard.WhitePoint);


		RenderCommand::ApplyShaderParameters(m_PerPassParameters, m_Shader);
		RenderCommand::DrawMesh(m_FullscrrenQuad);
	}
	BloomPass::BloomPass(const Ref<Shader>& shader)
		:m_FilterShader(shader), m_FullscreenQuad(CreateFullscreenQuad())
	{
		FrameBufferSpecitification spec;
		spec.Width /= 2;
		spec.Height /= 2;
		spec.Attachments.clear();
		spec.AllowEmptyAttachments = true;

		m_BloomFrameBuffer = FrameBuffer::Create(spec);

		m_BloomTexture = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RGBA16F,
			m_ActiveMipLevels, TextureWrap::ClampToEdge);

		m_DownShader = ShaderManager::Get("BloomDown");
		m_UpShader = ShaderManager::Get("BloomUp");
	}

	TAAPass::TAAPass(const Ref<Shader>& shader)
		:m_Shader(shader)
	{
		m_FullscreenQuad = CreateFullscreenQuad();

		FrameBufferSpecitification spec;
		spec.Attachments.clear();
		spec.AllowEmptyAttachments = true;
		m_Framebuffer = FrameBuffer::Create(spec);
		m_History[0] = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RGBA16F);
		m_History[1] = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RGBA16F);
	}

	SSAOPass::SSAOPass(const Ref<Shader>& shader)
		: m_Shader(shader), m_FullscreenQuad(CreateFullscreenQuad())
	{
		FrameBufferSpecitification spec;
		spec.Width /= 2;
		spec.Height /= 2;
		spec.Attachments.clear();
		spec.Attachments = { { TextureFormat::R8 } };
		m_FrameBuffer = FrameBuffer::Create(spec);
		m_OutBuffer = FrameBuffer::Create(spec);

		m_BlurShader = ShaderManager::Get("SSAOBlur");
	}

	void SSAOPass::Execute(const RenderView& view, const Ref<GBuffer>& input)
	{
		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetDepthTest(false);
		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetCull(CullMode::None);

		const auto& settings = view.SSAO;
		const int resolutionDivisor =
			view.QualityPreset == RenderQualityPreset::Low ? 2 : 1;
		const auto inputSize = input->GetSize();
		const glm::ivec2 size(
			std::max(inputSize.x / resolutionDivisor, 1),
			std::max(inputSize.y / resolutionDivisor, 1));
		if (m_FrameBuffer->GetSize() != size)
		{
			m_FrameBuffer->Resize(size.x, size.y);
			m_OutBuffer->Resize(size.x, size.y);
		}
		// m_FrameBuffer->Bind();
		m_OutBuffer->Bind();

		m_SSAOPassParameters.Clear();
		m_SSAOPassParameters.SetInt("u_Enabled", settings.Enabled ? 1 : 0);
		m_SSAOPassParameters.SetFloat("u_DepthBias", settings.DepthBias);
		m_SSAOPassParameters.SetFloat("u_Radius", settings.Radius);
		m_SSAOPassParameters.SetFloat("u_Intensity", settings.Intensity);
		m_SSAOPassParameters.SetTexture("u_Normal", input->GetNormalRoughness());
		m_SSAOPassParameters.SetTexture("u_Depth", input->GetDepth());
		RenderCommand::ApplyShaderParameters(m_SSAOPassParameters, m_Shader);
		RenderCommand::DrawMesh(m_FullscreenQuad);

		// m_FrameBuffer->UnBind();
		m_OutBuffer->UnBind();

		// SSAO Blur
		// dir 1
		m_FrameBuffer->Bind();
		// m_OutBuffer->Bind();
		m_BlurParameters.SetTexture("u_SSAOColor", m_OutBuffer->GetColorAttachment(0));
		m_BlurParameters.SetInt("u_Enabled", settings.Enabled ? 1 : 0);
		m_BlurParameters.SetTexture("u_Depth", input->GetDepth());
		m_BlurParameters.SetTexture("u_Normal", input->GetNormalRoughness());
		m_BlurParameters.SetFloat2("u_BlurDirection", glm::vec2(0.0, 1.0));
		RenderCommand::ApplyShaderParameters(m_BlurParameters, m_BlurShader);
		RenderCommand::DrawMesh(m_FullscreenQuad);
		m_FrameBuffer->UnBind();

		// SSAO Blur 
		// dir 2
		m_OutBuffer->Bind();
		m_BlurParameters.SetTexture("u_SSAOColor", m_FrameBuffer->GetColorAttachment(0));
		// m_BlurParameters.SetTexture("u_SSAOColor", m_OutBuffer->GetColorAttachment(0));
		m_BlurParameters.SetFloat2("u_BlurDirection", glm::vec2(1.0, 0.0));
		RenderCommand::ApplyShaderParameters(m_BlurParameters, m_BlurShader);
		RenderCommand::DrawMesh(m_FullscreenQuad);
		m_OutBuffer->UnBind();
	}

	void TAAPass::Execute(const RenderView& view, const Ref<FrameBuffer>& input,
		const Ref<Texture2D>& previousDepth, const Ref<Texture2D>& previousVelocity)
	{
		const auto size = input->GetSize();
		if (m_History[0]->GetWidth() != size.x || m_History[0]->GetHeight() != size.y)
		{
			m_History[0] = Texture2D::Create(size.x, size.y, TextureFormat::RGBA16F);
			m_History[1] = Texture2D::Create(size.x, size.y, TextureFormat::RGBA16F);
			m_HistoryIndex = 0;
			m_HistoryValid = false;
		}

		m_PerPassParameters.Clear();
		m_PerPassParameters.SetTexture("u_CurrentColor", input->GetColorAttachment(0));
		m_PerPassParameters.SetTexture("u_CurrentDepth", input->GetDepthAttachment());
		m_PerPassParameters.SetTexture("u_HistoryColor", m_History[m_HistoryIndex]);
		m_PerPassParameters.SetInt("u_HistoryValid", m_HistoryValid ? 1 : 0);
		m_PerPassParameters.SetTexture("u_PreviousDepth", previousDepth);
		m_PerPassParameters.SetTexture("u_PreviousVelocityBuffer", previousVelocity);
		m_PerPassParameters.SetInt("u_HighQualitySampling",
			view.QualityPreset == RenderQualityPreset::High ? 1 : 0);
		m_PerPassParameters.SetTexture("u_VelocityBuffer", input->GetColorAttachment(2));

		m_Framebuffer->AttachColor(m_History[1 - m_HistoryIndex]);
		m_Framebuffer->SetDrawBuffers({ 0 });
		m_Framebuffer->Bind();

		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetDepthTest(false);
		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetCull(CullMode::None);
		RenderCommand::ApplyShaderParameters(m_PerPassParameters, m_Shader);
		RenderCommand::DrawMesh(m_FullscreenQuad);

		m_Framebuffer->UnBind();

		m_HistoryValid = true;

		// Swap the history buffers after writing the current result.
		m_HistoryIndex = 1 - m_HistoryIndex;
	}

	void BloomPass::Execute(const RenderView& view, const Ref<Texture2D>& screenColor)
	{
		const auto& settings = view.PostProcess.Bloom;
		if (!settings.Enabled)
			return;

		const glm::uvec2 bufferSize(
			std::max(screenColor->GetWidth() / 2u, 1u),
			std::max(screenColor->GetHeight() / 2u, 1u));
		if (bufferSize.x == 0 || bufferSize.y == 0)
			return;

		const uint32_t availableMipLevels = Texture::CalculateMipLevelCount(
			bufferSize.x,
			bufferSize.y);
		const uint32_t requestedMipLevels = std::clamp(
			settings.MaxMipLevels,
			1u,
			availableMipLevels);

		if (m_BloomTexture->GetWidth() != bufferSize.x ||
			m_BloomTexture->GetHeight() != bufferSize.y ||
			m_BloomTexture->GetMipLevelCount() != requestedMipLevels)
		{
			m_ActiveMipLevels = requestedMipLevels;
			m_BloomTexture = Texture2D::Create(bufferSize.x, bufferSize.y,
				TextureFormat::RGBA16F, m_ActiveMipLevels, TextureWrap::ClampToEdge);
			m_BloomFrameBuffer->Resize(bufferSize.x, bufferSize.y);
		}

		// 获取第 0 级 Bloom 图
		m_BloomFrameBuffer->AttachColor(m_BloomTexture, 0, 0);
		m_BloomFrameBuffer->SetDrawBuffers({ 0 });
		m_BloomFrameBuffer->Bind();

		m_PerPassParameters.Clear();
		m_PerPassParameters.SetTexture("u_SceneColor", screenColor);
		m_PerPassParameters.SetFloat("u_Threshold", settings.Threshold);
		m_PerPassParameters.SetFloat("u_Knee", settings.Knee);

		RenderCommand::ApplyShaderParameters(m_PerPassParameters, m_FilterShader);
		RenderCommand::DrawMesh(m_FullscreenQuad);

		m_BloomFrameBuffer->UnBind();

		// 下采样生成其他 mip 层级
		for (uint32_t i = 1; i < m_ActiveMipLevels; ++i)
		{
			m_BloomFrameBuffer->AttachColor(m_BloomTexture, 0, i);
			m_BloomFrameBuffer->Bind();

			m_PerPassParameters.Clear();
			m_PerPassParameters.SetTexture("u_SourceTexture", m_BloomTexture);
			m_PerPassParameters.SetInt("u_SourceMip", static_cast<int>(i - 1));
			const auto sourceSize = m_BloomTexture->GetMipSize(i - 1);
			m_PerPassParameters.SetFloat("u_Width", static_cast<float>(sourceSize.x));
			m_PerPassParameters.SetFloat("u_Hight", static_cast<float>(sourceSize.y));

			RenderCommand::ApplyShaderParameters(m_PerPassParameters, m_DownShader);
			RenderCommand::DrawMesh(m_FullscreenQuad);
		}


		// 上采样生成最终的 BloomTexture
		RenderCommand::SetBlend(BlendMode::Add);
		for (int i = static_cast<int>(m_ActiveMipLevels) - 2; i >= 0; --i)
		{
			m_BloomFrameBuffer->AttachColor(m_BloomTexture, 0, static_cast<uint32_t>(i));
			m_BloomFrameBuffer->Bind();

			m_PerPassParameters.Clear();
			m_PerPassParameters.SetTexture("u_SourceTexture", m_BloomTexture);
			m_PerPassParameters.SetInt("u_SourceMip", i + 1);	// 被采样的 mip 层级
			const auto sourceSize = m_BloomTexture->GetMipSize(static_cast<uint32_t>(i + 1));
			m_PerPassParameters.SetFloat("u_Width", static_cast<float>(sourceSize.x));
			m_PerPassParameters.SetFloat("u_Hight", static_cast<float>(sourceSize.y));
			m_PerPassParameters.SetFloat("u_Scatter", settings.Scatter);
			RenderCommand::ApplyShaderParameters(m_PerPassParameters, m_UpShader);
			RenderCommand::DrawMesh(m_FullscreenQuad);
		}
		RenderCommand::SetBlend(BlendMode::Opaque);
		m_BloomFrameBuffer->UnBind();
	}
	SSRPass::SSRPass(const Ref<Shader>& shader)
		:m_Shader(shader), m_FullscreenQuad(CreateFullscreenQuad())
	{
		FrameBufferSpecitification spec;
		spec.AllowEmptyAttachments = true;
		spec.Attachments.clear();
		m_Framebuffer = FrameBuffer::Create(spec);

		m_Color[0] = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RGBA16F);
		m_Color[1] = Texture2D::Create(spec.Width, spec.Height, TextureFormat::RGBA16F);
	
		m_HiZShader = ShaderManager::Get("HiZ");

	}

	void SSRPass::Execute(const RenderView& view, const Ref<GBuffer>& gbuffer,
		const Ref<FrameBuffer>& buffer)
	{
		const auto& size = buffer->GetSize();
		if (m_Framebuffer->GetSize() != size)
		{
			m_Framebuffer->Resize(size.x, size.y);
			m_Color[0] = Texture2D::Create(size.x, size.y, TextureFormat::RGBA16F);
			m_Color[1] = Texture2D::Create(size.x, size.y, TextureFormat::RGBA16F);
		}


		RenderCommand::SetBlend(BlendMode::Opaque);
		RenderCommand::SetDepthTest(false);
		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetCull(CullMode::None);

		const bool useHiZ = view.QualityPreset == RenderQualityPreset::High;
		if (useHiZ && (!m_MipDepth || m_MipDepth->GetWidth() != size.x ||
			m_MipDepth->GetHeight() != size.y))
		{
			m_MipCount = Texture::CalculateMipLevelCount(size.x, size.y);
			m_MipDepth = Texture2D::Create(size.x, size.y, TextureFormat::R32F, m_MipCount);
		}


		for (uint32_t i = 0; useHiZ && i < m_MipCount; ++i)
		{
			m_Framebuffer->AttachColor(m_MipDepth, 0, i);	// write 
			m_Framebuffer->SetDrawBuffers({ 0 });
			m_Framebuffer->Bind();
			m_DepthParameters.SetTexture("u_RawDepth", buffer->GetDepthAttachment());
			m_DepthParameters.SetTexture("u_SourceTexture", m_MipDepth);
			m_DepthParameters.SetInt("u_CurrentMipLevel", static_cast<int>(i));
			RenderCommand::ApplyShaderParameters(m_DepthParameters, m_HiZShader);
			RenderCommand::DrawMesh(m_FullscreenQuad);
		}

		m_Framebuffer->AttachColor(m_Color[m_WriteIndex]);
		m_Framebuffer->SetDrawBuffers({ 0 });
		m_Framebuffer->Bind();
		m_Framebuffer->ClearColor();

		m_Parameters.SetTexture("u_AlbedoAlpha", gbuffer->GetAlbedo());
		m_Parameters.SetTexture("u_ScreenColor", buffer->GetColorAttachment(0));
		m_Parameters.SetTexture("u_Depth", buffer->GetDepthAttachment());
		if (useHiZ)
		{
			m_Parameters.SetTexture("u_MipDepth", m_MipDepth);
			m_Parameters.SetInt("u_MipCount",
				static_cast<int>(std::min(m_MipCount, 7u)));
		}
		m_Parameters.SetInt("u_UseHiZ", useHiZ ? 1 : 0);
		m_Parameters.SetTexture("u_NormalRoughness", gbuffer->GetNormalRoughness());
		m_Parameters.SetTexture("u_EmissiveMetallic", gbuffer->GetEmissiveMetallic());
		m_Parameters.SetTexture("u_CharacterNormalMask", buffer->GetColorAttachment(1));

		RenderCommand::ApplyShaderParameters(m_Parameters, m_Shader);
		RenderCommand::DrawMesh(m_FullscreenQuad);

		m_Framebuffer->UnBind();

		buffer->Bind();
		buffer->AttachColor(m_Color[m_WriteIndex]);
		buffer->SetDrawBuffers({ 0 });
		m_WriteIndex = 1 - m_WriteIndex;
	}
}
