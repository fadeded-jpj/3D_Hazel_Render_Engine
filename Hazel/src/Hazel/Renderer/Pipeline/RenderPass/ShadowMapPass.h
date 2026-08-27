#pragma once

#include "Hazel/Renderer/Shader/ShaderManager.h"
#include "RenderPass.h"

#include "Hazel/Renderer/Lighting/Shadow/ShadowMap.h"

namespace Engine
{
	class MaterialInstance;

	class ShadowMapPassBase
	{
	public:
		ShadowMapPassBase(const Ref<Shader>& shader)
			:m_Shader(shader) {
			m_PerItemParameters.Clear();
			m_PerItemParameters.SetMat4("u_Model", glm::mat4(1.0f));
			m_PerItemParameters.SetInt("u_HasSkeleton", 0);
		}
		virtual ~ShadowMapPassBase() = default;

		// func: 将过滤为 true 的 renderobj
		void Build(const std::vector<RenderObject>& objects, std::function<bool(const RenderObject&)> func);

	protected:
		static bool IsOpaqueLike(BlendMode blend);
		static DrawItem CreateDrawItem(const RenderObject& object);

		static void SubmitDrawItem(const DrawItem& item, const Ref<Shader>& shader,
			ShaderParameters& parameters, RendererLog& log, uint32_t textureSlotOffset);
		static void ApplyRenderState(const MaterialRenderConfig& desired, AppliedRenderState& applied);
	protected:
		Ref<Shader> m_Shader;
		std::vector<DrawItem> m_DrawItems;
		ShaderParameters m_PerPassParameters;
		ShaderParameters m_PerItemParameters;
	};

	class ShadowMap2DPass : public ShadowMapPassBase
	{
	public:
		ShadowMap2DPass(unsigned int size, const Ref<Shader>& shader = ShaderManager::Get("ShadowMap"));
		virtual RendererLog Execute(const ShadowView& view, ShadowFrameData& output);

		Ref<Texture2D> GetDepth() const { return m_ShadowMap2D->GetShaderMap(); }


	private:
		Ref<ShadowMap> m_ShadowMap2D;
	};

	class PointShadowPass : public ShadowMapPassBase
	{
	public:
		PointShadowPass(unsigned int size,
			const Ref<Shader>& shader = ShaderManager::Get("PointShadowMap"));

		virtual RendererLog Execute(const PointShadowView& view, ShadowFrameData& output);

		Ref<TextureCubeMap> GetDepth() const { return m_ShadowMap->GetShaderMap(); }
	private:
		Ref<PointShadowMap> m_ShadowMap;
	};

	class DirectionalCSMPass : public ShadowMapPassBase
	{
	public:
		DirectionalCSMPass(unsigned int shadowRes = 256, unsigned int layer = CSMShadowView::CascadeCount,
			const Ref<Shader>& shader = ShaderManager::Get("ShadowMap"));

		virtual RendererLog Execute(const CSMShadowView& view, ShadowFrameData& output);
		Ref<Texture2DArray> GetDepth() const { return m_ShadowMap->GetShaderMap();; }
	private:
		Ref<CSMShadowMap> m_ShadowMap;
		ShaderParameters m_Parameters;
	};
}
