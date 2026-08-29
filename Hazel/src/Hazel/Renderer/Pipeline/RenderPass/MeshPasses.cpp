#include "hzpch.h"
#include "MeshPasses.h"

#include "Hazel/Renderer/Lighting/Shadow/ShadowMap.h"
#include "Hazel/Renderer/RHI/RenderCommand.h"
#include "Hazel/Renderer/RHI/BoneMatrixBuffer.h"

namespace Engine
{
	void OpaquePass::Build(const RenderView& view, const std::vector<RenderObject>& objects)
	{
		m_DrawItems.clear();
		ResetItemStatistics();
		m_DrawItems.reserve(objects.size());

		for (const auto& obj : objects)
		{
			if (!obj.Visible || !IsOpaqueLike(obj.Material->GetRenderConfig().Blend))
				continue;

			RecordVisibilityResult(true);
			m_DrawItems.push_back(CreateDrawItem(obj, view.CameraPosition));
		}

		Sort();
	}

	RendererLog OpaquePass::Execute(const RenderView& view, const SceneLightingData& light,
		const ShadowFrameData& shadow)
	{
		AppliedRenderState currentRenderState;
		RendererLog log;
		log.OpaqueDrawItems = static_cast<unsigned int>(m_DrawItems.size());

		m_PerPassParameters.Clear();
		m_PerItemParameters.Clear();
#ifdef HZ_DEBUG
		m_PerPassParameters.SetInt("u_DebugView", static_cast<int>(view.DebugSetting.View));
#endif
		SubmitLight(m_PerPassParameters, light);
		m_PerPassParameters.SetInt("u_ShadowEnabled", shadow.Enabled ? 1 : 0);
		m_PerPassParameters.SetInt("u_ShadowLightIndex", shadow.MainLightIndex);
		m_PerPassParameters.SetMat4("u_ShadowViewProj", shadow.ViewProjection);
		m_PerPassParameters.SetFloat("u_ShadowBias", shadow.DepthBias);
		//if (shadow.Enabled)
		//	m_PerPassParameters.SetTexture("u_ShadowMap", shadow.DepthTexture);

		Ref<Shader> activeShader;
		uint32_t textureSlotOffset = ShaderTextureSlots::First;
		for (const auto& item : m_DrawItems)
		{
			ApplyRenderState(item.RenderConfig, currentRenderState);

			if (item.ShaderProgram != activeShader)
			{
				textureSlotOffset = RenderCommand::ApplyShaderParameters(
					m_PerPassParameters, item.ShaderProgram);
				activeShader = item.ShaderProgram;
			}

			SubmitPerItem(item, item.ShaderProgram, view, m_PerItemParameters, log,
				textureSlotOffset);
		}

		return log;
	}

	void CharacterPass::Build(const RenderView& view, const std::vector<RenderObject>& objects)
	{
		m_DrawItems.clear();
		ResetItemStatistics();

#ifdef HZ_DEBUG
		if (view.DebugSetting.View == RenderDebugView::CSM)
			return;
#endif

		m_DrawItems.reserve(objects.size());

		for (const auto& obj : objects)
		{
			if (!obj.Visible || !IsOpaqueLike(obj.Material->GetRenderConfig().Blend) ||
				obj.Domain != RenderDomain::Character)
				continue;

			const bool visible = CameraVisibleCull(obj, view.ViewProjection);
			RecordVisibilityResult(visible);
			if (!visible)
				continue;

			auto item = CreateDrawItem(obj, view.CameraPosition);
			if (auto shader = ResolveToonShader(item.ToonRole))
				item.ShaderProgram = shader;
			m_DrawItems.push_back(std::move(item));
		}

		Sort();
	}

	void CharacterPass::SetToonShader(ToonMaterialRole role, const Ref<Shader>& shader)
	{
		if (role == ToonMaterialRole::Count)
			return;

		if (shader)
			m_ToonShaders[role] = shader;
		else
			m_ToonShaders.erase(role);
	}

	Ref<Shader> CharacterPass::ResolveToonShader(ToonMaterialRole role) const
	{
		auto it = m_ToonShaders.find(role);
		if (it != m_ToonShaders.end() && it->second)
			return it->second;

		auto fallback = m_ToonShaders.find(ToonMaterialRole::Default);
		return fallback != m_ToonShaders.end() ? fallback->second : nullptr;
	}

	void CharacterPass::Sort()
	{
		std::sort(m_DrawItems.begin(), m_DrawItems.end(), [](const DrawItem& a, const DrawItem& b)
			{
				if (a.ShaderProgram != b.ShaderProgram)
					return a.ShaderProgram < b.ShaderProgram;
				if (a.Material != b.Material)
					return a.Material < b.Material;
				return a.Mesh < b.Mesh;
			});
	}

	RendererLog CharacterPass::Execute(const RenderView& view, const SceneLightingData& light,
		const ShadowFrameData& shadow)
	{
		return Execute(view, light, shadow, ShadowFrameData{});
	}

	RendererLog CharacterPass::Execute(const RenderView& view, const SceneLightingData& light,
		const ShadowFrameData& shadow, const ShadowFrameData& characterShadow)
	{
		AppliedRenderState currentState;
		RendererLog log;
		log.OpaqueDrawItems = static_cast<unsigned int>(m_DrawItems.size());

		m_PerPassParameters.Clear();
		m_PerItemParameters.Clear();
		m_PerPassParameters.SetInt("u_ShadowEnabled", shadow.Enabled ? 1 : 0);
		m_PerPassParameters.SetInt("u_ShadowLightIndex", shadow.MainLightIndex);
		m_PerPassParameters.SetMat4("u_ShadowViewProj", shadow.ViewProjection);
		m_PerPassParameters.SetFloat("u_ShadowBias", shadow.DepthBias);
		m_PerPassParameters.SetFloat("u_ShadowFarPlane", shadow.FarPlane);
		m_PerPassParameters.SetInt("u_CharacterShadowEnabled", characterShadow.Enabled ? 1 : 0);
		m_PerPassParameters.SetMat4("u_CharacterShadowViewProj", characterShadow.ViewProjection);
		m_PerPassParameters.SetFloat("u_CharacterShadowBias", characterShadow.DepthBias);
		m_PerItemParameters.SetTexture("u_MetalMask", m_MetalMask);
#ifdef HZ_DEBUG
		m_PerPassParameters.SetInt("u_DebugView", static_cast<int>(view.DebugSetting.View));
#endif
		SubmitLight(m_PerPassParameters, light, 1);

		//m_PerPassParameters.SetTexture("u_ShadowMap", shadow.DepthTexture);
		//m_PerPassParameters.SetTexture("u_PointShadowMap", shadow.CubeDepthTexture);
		//m_PerPassParameters.SetTexture("u_CSMShadowMap", shadow.CSMDepthTexture);

		m_PerPassParameters.SetInt("u_CSMEnabled", shadow.Enabled && shadow.IsCSM ? 1 : 0);
		if (shadow.Enabled && shadow.IsCSM)
		{
			m_PerPassParameters.SetFloat4("u_CSMSplits", shadow.CascadeSplits);
			m_PerPassParameters.SetFloat("u_OverlapRatio", shadow.OverlapRatio);
			for (int i = 0; i < CSMShadowView::CascadeCount; i++)
				m_PerPassParameters.SetMat4("u_CSMViewProj[" + std::to_string(i) + "]", shadow.CSMViewProjection[i]);
		}

		m_PerPassParameters.SetFloat3("u_ToonShadowTint", view.ToonShadowTint);
		m_PerPassParameters.SetInt("u_ToonMainLightIndex", 0);

		Ref<Shader> activeShader;
		uint32_t textureSlotOffset = ShadowTextureBinding::PassLocalBegin;
		for (const auto& item : m_DrawItems)
		{
			ApplyRenderState(item.RenderConfig, currentState);

			const Ref<Shader>& shader = item.ShaderProgram;
			if (shader != activeShader)
			{
				textureSlotOffset = RenderCommand::ApplyShaderParameters(
					m_PerPassParameters, shader,
					ShadowTextureBinding::PassLocalBegin);
				activeShader = shader;
			}

			// ------------ 提交 Character head ------------
			const auto& character = item.Object->Character;
			m_PerItemParameters.SetInt("u_HasHeadTransform", character.HasHeadTransform ? 1 : 0);
			if (character.HasHeadTransform)
			{
				m_PerItemParameters.SetFloat3("u_HeadPosition", character.HeadPosition);
				m_PerItemParameters.SetFloat3("u_HeadForward", character.HeadForward);
				m_PerItemParameters.SetFloat3("u_HeadRight", character.HeadRight);
				m_PerItemParameters.SetFloat3("u_HeadUp", character.HeadUp);
				m_PerItemParameters.SetInt("u_HasFaceShadowMap", 1);
				m_PerItemParameters.SetTexture("u_FaceShadowMap", m_FaceShadowMap);
			}

			SubmitPerItem(item, shader, view, m_PerItemParameters, log,
				textureSlotOffset);
		}

		m_DrawItems.clear();
		return log;
	}

	void OpaquePass::Sort()
	{
		std::sort(m_DrawItems.begin(), m_DrawItems.end(), [](const DrawItem& a, const DrawItem& b)
			{
				if (a.ShaderProgram != b.ShaderProgram)
					return a.ShaderProgram < b.ShaderProgram;
				if (a.Material != b.Material)
					return a.Material < b.Material;
				if (a.Mesh != b.Mesh)
					return a.Mesh < b.Mesh;
				return a.CameraDistance > b.CameraDistance;
			});
	}

	void TransparentPass::Build(const RenderView& view, const std::vector<RenderObject>& objects)
	{
		m_DrawItems.clear();
		ResetItemStatistics();
		m_DrawItems.reserve(objects.size());

		for (const auto& obj : objects)
		{
		#ifdef HZ_DEBUG
			if (view.DebugSetting.View == RenderDebugView::CSM &&
				obj.Domain == RenderDomain::Character)
				continue;
		#endif

			if (!obj.Visible || !IsTransparent(obj.Material->GetRenderConfig().Blend))
				continue;

			const bool visible = CameraVisibleCull(obj, view.ViewProjection);
			RecordVisibilityResult(visible);
			if (!visible)
				continue;

			m_DrawItems.push_back(CreateDrawItem(obj, view.CameraPosition));
		}

		Sort();
	}

	RendererLog TransparentPass::Execute(const RenderView& view, const SceneLightingData& light,
		const ShadowFrameData& shadow)
	{
		return Execute(view, light, shadow, ShadowFrameData{});
	}

	RendererLog TransparentPass::Execute(const RenderView& view, const SceneLightingData& light,
		const ShadowFrameData& shadow, const ShadowFrameData& characterShadow)
	{
		AppliedRenderState currentState;
		RendererLog log;
		log.TransparentDrawItems = static_cast<unsigned int>(m_DrawItems.size());

		m_PerPassParameters.Clear();
		m_PerItemParameters.Clear();
		m_PerPassParameters.SetInt("u_ShadowEnabled", shadow.Enabled ? 1 : 0);
		m_PerPassParameters.SetInt("u_ShadowLightIndex", shadow.MainLightIndex);
		m_PerPassParameters.SetMat4("u_ShadowViewProj", shadow.ViewProjection);
		m_PerPassParameters.SetFloat("u_ShadowBias", shadow.DepthBias);
		m_PerPassParameters.SetFloat("u_ShadowFarPlane", shadow.FarPlane);
		m_PerPassParameters.SetInt("u_CharacterShadowEnabled", characterShadow.Enabled ? 1 : 0);
		m_PerPassParameters.SetMat4("u_CharacterShadowViewProj", characterShadow.ViewProjection);
		m_PerPassParameters.SetFloat("u_CharacterShadowBias", characterShadow.DepthBias);
#ifdef HZ_DEBUG
		m_PerPassParameters.SetInt("u_DebugView", static_cast<int>(view.DebugSetting.View));
#endif
		SubmitLight(m_PerPassParameters, light);

		m_PerPassParameters.SetInt("u_CSMEnabled", shadow.Enabled && shadow.IsCSM ? 1 : 0);
		if (shadow.Enabled && shadow.IsCSM)
		{
			m_PerPassParameters.SetFloat4("u_CSMSplits", shadow.CascadeSplits);
			m_PerPassParameters.SetFloat("u_OverlapRatio", shadow.OverlapRatio);
			for (int i = 0; i < CSMShadowView::CascadeCount; i++)
				m_PerPassParameters.SetMat4("u_CSMViewProj[" + std::to_string(i) + "]", shadow.CSMViewProjection[i]);
		}

		// Forward materials choose their own shader. Toon shaders consume these
		// parameters while PBR shaders safely ignore uniforms they do not declare.
		m_PerPassParameters.SetFloat3("u_ToonShadowTint", view.ToonShadowTint);
		m_PerPassParameters.SetInt("u_ToonMainLightIndex", 0);

		Ref<Shader> activeShader;
		uint32_t textureSlotOffset = ShadowTextureBinding::PassLocalBegin;
		for (const auto& item : m_DrawItems)
		{
			ApplyRenderState(item.RenderConfig, currentState);

			const Ref<Shader>& shader = item.ShaderProgram;
			if (shader != activeShader)
			{
				textureSlotOffset = RenderCommand::ApplyShaderParameters(
					m_PerPassParameters, shader,
					ShadowTextureBinding::PassLocalBegin);
				activeShader = shader;
			}

			SubmitPerItem(item, shader, view, m_PerItemParameters, log,
				textureSlotOffset);
		}

		m_DrawItems.clear();
		return log;
	}

	void TransparentPass::Sort()
	{
		std::sort(m_DrawItems.begin(), m_DrawItems.end(), [](const DrawItem& a, const DrawItem& b)
			{
				return a.CameraDistance > b.CameraDistance;
			});
	}

	GBufferPass::GBufferPass(const Ref<Shader>& shader)
		: m_GBufferShader(shader)
	{
	}

	void GBufferPass::Build(const RenderView& view, const std::vector<RenderObject>& objects)
	{
		m_DrawItems.clear();
		ResetItemStatistics();
		m_DrawItems.reserve(objects.size());

		for (const auto& obj : objects)
		{
			if (!obj.Visible || !IsOpaqueLike(obj.Material->GetRenderConfig().Blend) ||
				obj.Domain == RenderDomain::Character)
				continue;

			const bool visible = CameraVisibleCull(obj, view.ViewProjection);
			RecordVisibilityResult(visible);
			if (!visible)
				continue;

			m_DrawItems.push_back(CreateDrawItem(obj, view.CameraPosition));
		}

		Sort();
	}

	RendererLog GBufferPass::Execute(const RenderView& view, const SceneLightingData& light,
		const ShadowFrameData& shadow)
	{
		HZ_CORE_ASSERT(m_GBufferShader, "GBufferPass requires a GBuffer shader");

		AppliedRenderState currentState;
		RendererLog log;
		log.OpaqueDrawItems = static_cast<unsigned int>(m_DrawItems.size());

		m_PerPassParameters.Clear();
		m_PerItemParameters.Clear();
#ifdef HZ_DEBUG
		m_PerPassParameters.SetInt("u_DebugView", static_cast<int>(view.DebugSetting.View));
#endif // HZ_DEBUG
		const uint32_t textureSlotOffset = RenderCommand::ApplyShaderParameters(
			m_PerPassParameters, m_GBufferShader);


		for (const auto& item : m_DrawItems)
		{
			ApplyRenderState(item.RenderConfig, currentState);
			SubmitPerItem(item, m_GBufferShader, view, m_PerItemParameters, log,
				textureSlotOffset);
		}

		m_DrawItems.clear();
		return log;
	}

	void GBufferPass::Sort()
	{
		std::sort(m_DrawItems.begin(), m_DrawItems.end(), [](const DrawItem& a, const DrawItem& b)
			{
				if (a.ShaderProgram != b.ShaderProgram)
					return a.ShaderProgram < b.ShaderProgram;
				if (a.Material != b.Material)
					return a.Material < b.Material;
				if (a.Mesh != b.Mesh)
					return a.Mesh < b.Mesh;
				return a.CameraDistance > b.CameraDistance;
			});
	}
	InvertedHullOutlinePass::InvertedHullOutlinePass(const Ref<Shader>& shader)
		: m_OutlineShader(shader)
	{
	}
	void InvertedHullOutlinePass::Build(const RenderView& view, const std::vector<RenderObject>& objects)
	{
		m_DrawItems.clear();
		ResetItemStatistics();
#ifdef HZ_DEBUG
		if (view.DebugSetting.View != RenderDebugView::Lit)
			return;
#endif

		m_DrawItems.reserve(objects.size());

		for (const auto& item : objects)
		{
			// 仅对人物进行 outline 的计算
			if (item.OutlineMode != RenderOutlineMode::InvertedHull)
				continue;

			const bool visible = CameraVisibleCull(item, view.ViewProjection);
			RecordVisibilityResult(visible);
			if (!visible)
				continue;

			m_DrawItems.push_back(CreateDrawItem(item, view.CameraPosition));
		}
	}
	RendererLog InvertedHullOutlinePass::Execute(const RenderView& view, const SceneLightingData& light, const ShadowFrameData& shadow)
	{
		HZ_CORE_ASSERT(m_OutlineShader, "GBufferPass requires a GBuffer shader");

		RendererLog log;
		log.OpaqueDrawItems = static_cast<unsigned int>(m_DrawItems.size());

		m_PerPassParameters.Clear();
		m_PerItemParameters.Clear();
		const uint32_t textureSlotOffset = RenderCommand::ApplyShaderParameters(
			m_PerPassParameters, m_OutlineShader);

		for (const auto& item : m_DrawItems)
		{
			SubmitPerItem(item, m_OutlineShader, view, m_PerItemParameters, log,
				textureSlotOffset);
		}

		m_DrawItems.clear();
		return log;
	}

	GemotryOutlinePass::GemotryOutlinePass(const Ref<Shader>& shader)
	{
		m_OutlineShader = shader;
	}

	void GemotryOutlinePass::Build(const RenderView& view, const std::vector<RenderObject>& objects)
	{
		m_DrawItems.clear();
		ResetItemStatistics();
#ifdef HZ_DEBUG
		if (view.DebugSetting.View != RenderDebugView::Lit)
			return;
#endif

		for (const auto& obj : objects)
		{
			if (obj.Domain != RenderDomain::Character)
				continue;


			if (!obj.OutlineEdgeMesh)
				continue;

			const bool visible = CameraVisibleCull(obj, view.ViewProjection);
			RecordVisibilityResult(visible);
			if (!visible)
				continue;

			DrawItem item = CreateDrawItem(obj, view.CameraPosition);

			item.Mesh = obj.OutlineEdgeMesh;
			item.ShaderProgram = m_OutlineShader;

			m_DrawItems.push_back(std::move(item));
		}
	}

	RendererLog GemotryOutlinePass::Execute(const RenderView& view, const SceneLightingData& light, const ShadowFrameData& shadow)
	{
		RendererLog log;
		log.OpaqueDrawItems = static_cast<unsigned int>(m_DrawItems.size());

		m_PerPassParameters.Clear();
		m_PerItemParameters.Clear();

		m_PerPassParameters.SetFloat("u_OutlineWidthScale", 0.001f);
		m_PerPassParameters.SetFloat("u_OutlineDepthBias", 0.00005f);
		m_PerPassParameters.SetFloat("u_CreaseCosThreshold", 0.5f);

		const uint32_t textureSlotOffset = RenderCommand::ApplyShaderParameters(
			m_PerPassParameters, m_OutlineShader);

		for (const auto& item : m_DrawItems)
		{
			SubmitPerItem(item, m_OutlineShader, view, m_PerItemParameters, log,
				textureSlotOffset);
		}

		m_DrawItems.clear();
		return log;
	}

}
